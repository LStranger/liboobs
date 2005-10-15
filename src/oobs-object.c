/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2005 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#include <dbus/dbus.h>
#include <glib-object.h>
#include "oobs-object.h"
#include "oobs-session.h"
#include "oobs-session-private.h"

#define OOBS_OBJECT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_OBJECT, OobsObjectPrivate))

typedef struct _OobsObjectPrivate OobsObjectPrivate;

struct _OobsObjectPrivate
{
  OobsSession *session;
  DBusError    dbus_error;

  gchar       *remote_object;
  gchar       *path;
  gchar       *method;
};

static void oobs_object_class_init (OobsObjectClass *class);
static void oobs_object_init       (OobsObject      *object);
static void oobs_object_finalize   (GObject        *object);

static void oobs_object_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec);
static void oobs_object_get_property (GObject      *object,
				      guint         prop_id,
				      GValue       *value,
				      GParamSpec   *pspec);
enum
{
  CHANGING,
  CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SESSION,
  PROP_REMOTE_OBJECT
};

static GQuark dbus_connection_quark;

static guint object_signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (OobsObject, oobs_object, G_TYPE_OBJECT);

static void
oobs_object_class_init (OobsObjectClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = oobs_object_get_property;
  object_class->set_property = oobs_object_set_property;
  object_class->finalize     = oobs_object_finalize;

  class->commit   = NULL;
  class->update   = NULL;
  class->changing = oobs_object_update;
  class->changed  = NULL;

  dbus_connection_quark = g_quark_from_static_string ("oobs-dbus-connection");

  g_object_class_install_property (object_class,
				   PROP_SESSION,
				   g_param_spec_object ("session",
							"Session to connect to",
							"Holds the OobsSession that the object will use",
							OOBS_TYPE_SESSION,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_REMOTE_OBJECT,
				   g_param_spec_string ("remote-object",
							"Remote object to deal with",
							"Name of the remote object at the other side of the connection",
							NULL,
							G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  object_signals [CHANGING] = g_signal_new ("changing",
					    G_OBJECT_CLASS_TYPE (object_class),
					    G_SIGNAL_RUN_LAST,
					    G_STRUCT_OFFSET (OobsObjectClass, changing),
					    NULL, NULL,
					    g_cclosure_marshal_VOID__VOID,
					    G_TYPE_NONE, 0);
  object_signals [CHANGED] = g_signal_new ("changed",
					   G_OBJECT_CLASS_TYPE (object_class),
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET (OobsObjectClass, changed),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__VOID,
					   G_TYPE_NONE, 0);

  g_type_class_add_private (object_class,
			    sizeof (OobsObjectPrivate));
}

static void
oobs_object_init (OobsObject *object)
{
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));

  priv = OOBS_OBJECT_GET_PRIVATE (object);
  priv->session = NULL;
  priv->remote_object = NULL;

  dbus_error_init (&priv->dbus_error);
}

static void
oobs_object_finalize (GObject *object)
{
  OobsObject *obj;
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));

  obj  = OOBS_OBJECT (object);
  priv = OOBS_OBJECT_GET_PRIVATE (obj);

  if (priv)
    {
      _oobs_session_unregister_object (priv->session, obj);

      g_free (priv->remote_object);
      g_free (priv->path);
      g_free (priv->method);
    }

  if (G_OBJECT_CLASS (oobs_object_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_object_parent_class)->finalize) (object);
}

static DBusHandlerResult
changed_signal_filter (DBusConnection *connection,
		       DBusMessage    *message,
		       void           *user_data)
{
  OobsObject        *object;
  OobsObjectPrivate *priv;

  object = OOBS_OBJECT (user_data);
  priv   = OOBS_OBJECT_GET_PRIVATE (object);

  if (dbus_message_is_signal (message, priv->method, "changed"))
    g_signal_emit (object, object_signals [CHANGING], 0);

  /* we want the rest of the objects of
   * the same type to get the signal too
   */
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
connect_object_to_session (OobsObject *object)
{
  OobsObjectPrivate *priv;
  DBusConnection    *connection;
  gchar             *rule;

  priv = OOBS_OBJECT_GET_PRIVATE (object);
  connection = _oobs_session_get_connection_bus (priv->session);

  _oobs_session_register_object (priv->session, object);
  dbus_connection_add_filter (connection, changed_signal_filter, object, NULL);

  rule = g_strdup_printf ("type='signal',sender='%s',interface='%s',path='%s'",
			  OOBS_DBUS_DESTINATION, priv->method, priv->path);
  dbus_bus_add_match (connection, rule, &priv->dbus_error);

  if (dbus_error_is_set (&priv->dbus_error))
    g_critical ("There was an error adding the match function: %s", priv->dbus_error.message);
    
  g_free (rule);
}

static void
oobs_object_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
  OobsObject *obj;
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));

  obj  = OOBS_OBJECT (object);
  priv = OOBS_OBJECT_GET_PRIVATE (obj);

  switch (prop_id)
    {
    case PROP_SESSION:
      if (priv->session)
	_oobs_session_unregister_object (priv->session, obj);

      priv->session = g_value_get_object (value);

      if (priv->session)
	connect_object_to_session (obj);
      break;
    case PROP_REMOTE_OBJECT:
      priv->remote_object = g_value_dup_string (value);
      priv->path   = g_strconcat (OOBS_DBUS_PATH_PREFIX, "/", priv->remote_object, NULL);
      priv->method = g_strconcat (OOBS_DBUS_METHOD_PREFIX, ".", priv->remote_object, NULL);
      break;
    }
}

static void
oobs_object_get_property (GObject      *object,
			  guint         prop_id,
			  GValue       *value,
			  GParamSpec   *pspec)
{
  OobsObject *obj;
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));

  obj  = OOBS_OBJECT (object);
  priv = OOBS_OBJECT_GET_PRIVATE (obj);

  switch (prop_id)
    {
    case PROP_SESSION:
      g_value_set_object (value, G_OBJECT (priv->session));
      break;
    }
}

static DBusMessage*
run_get_message (OobsObject *object)
{
  OobsObjectPrivate *priv;
  DBusConnection   *connection;
  DBusMessage      *message, *reply;

  priv   = OOBS_OBJECT_GET_PRIVATE (object);
  connection = _oobs_session_get_connection_bus (priv->session);

  g_return_val_if_fail (connection != NULL, NULL);

  message = dbus_message_new_method_call (OOBS_DBUS_DESTINATION, priv->path, priv->method, "get");
  reply   = dbus_connection_send_with_reply_and_block (connection, message, -1, &priv->dbus_error);
  dbus_message_unref (message);

  if (dbus_error_is_set (&priv->dbus_error))
    {
      g_critical ("There was an error communicating with the backends: %s", priv->dbus_error.message);
      return NULL;
    }

  return reply;
}

void
oobs_object_commit (OobsObject *object)
{
  OobsObjectClass   *class;
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));
  priv  = OOBS_OBJECT_GET_PRIVATE  (object);
  class = OOBS_OBJECT_GET_CLASS    (object);

  if (!priv->session)
    {
      g_critical ("Trying to commit changes after the session has terminated, "
		  "this reflects a bug in the application");
      return;
    }

  if (class->commit)
    class->commit (object);
}

void
oobs_object_update (OobsObject *object)
{
  OobsObjectClass   *class;
  OobsObjectPrivate *priv;
  DBusMessage       *reply;

  g_return_if_fail (OOBS_IS_OBJECT (object));
  priv = OOBS_OBJECT_GET_PRIVATE   (object);
  class = OOBS_OBJECT_GET_CLASS    (object);

  if (!priv->session)
    {
      g_critical ("Trying to update the object after the session has terminated, "
		  "this reflects a bug in the application");
      return;
    }

  if (!class->update)
    return;

  reply = run_get_message (object);

  if (reply)
    {
      g_object_set_qdata (G_OBJECT (object), dbus_connection_quark, reply);
      class->update (object);
      g_signal_emit (object, object_signals [CHANGED], 0);

      /* free the reply */
      reply = g_object_steal_qdata (G_OBJECT (object), dbus_connection_quark);
      dbus_message_unref (reply);
    }
}

DBusMessage*
_oobs_object_get_dbus_message (OobsObject *object)
{
  return (DBusMessage *) g_object_get_qdata (G_OBJECT (object), dbus_connection_quark);
}
