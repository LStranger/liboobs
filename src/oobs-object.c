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
#include "oobs-object-private.h"
#include "oobs-session.h"
#include "oobs-session-private.h"

#define OOBS_OBJECT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_OBJECT, OobsObjectPrivate))

typedef struct _OobsObjectPrivate OobsObjectPrivate;
typedef struct _OobsObjectAsyncCallbackData OobsObjectAsyncCallbackData;

struct _OobsObjectPrivate
{
  OobsSession *session;
  DBusError    dbus_error;

  gchar       *remote_object;
  gchar       *path;
  gchar       *method;
};

struct _OobsObjectAsyncCallbackData
{
  GObject *object;
  gboolean update;
  OobsObjectAsyncFunc func;
  gpointer data;
};

static void oobs_object_class_init (OobsObjectClass *class);
static void oobs_object_init       (OobsObject      *object);
static void oobs_object_finalize   (GObject         *object);

static void oobs_object_set_property (GObject       *object,
				      guint          prop_id,
				      const GValue  *value,
				      GParamSpec    *pspec);
static void oobs_object_get_property (GObject       *object,
				      guint          prop_id,
				      GValue        *value,
				      GParamSpec    *pspec);
enum
{
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

  object->_priv = priv;
}

static void
oobs_object_finalize (GObject *object)
{
  OobsObject *obj;
  OobsObjectPrivate *priv;

  g_return_if_fail (OOBS_IS_OBJECT (object));

  obj  = OOBS_OBJECT (object);
  priv = OOBS_OBJECT (object)->_priv;

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
  priv   = OOBS_OBJECT (object)->_priv;

  if (dbus_message_is_signal (message, priv->method, "changed") &&
      dbus_message_has_path (message, priv->path))
    g_signal_emit (object, object_signals [CHANGED], 0);

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

  priv = OOBS_OBJECT (object)->_priv;
  connection = _oobs_session_get_connection_bus (priv->session);

  _oobs_session_register_object (priv->session, object);
  dbus_connection_add_filter (connection, changed_signal_filter, object, NULL);

  rule = g_strdup_printf ("type='signal',interface='%s',path='%s'",
			  priv->method, priv->path);
  dbus_bus_add_match (connection, rule, &priv->dbus_error);

  if (dbus_error_is_set (&priv->dbus_error))
    {
      g_critical ("There was an error adding the match function: %s", priv->dbus_error.message);
      dbus_error_free (&priv->dbus_error);
    }

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
  priv = obj->_priv;

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
      priv->method = g_strdup (OOBS_DBUS_METHOD_PREFIX);
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
  priv = obj->_priv;

  switch (prop_id)
    {
    case PROP_SESSION:
      g_value_set_object (value, G_OBJECT (priv->session));
      break;
    }
}

DBusMessage*
_oobs_object_get_dbus_message (OobsObject *object)
{
  return (DBusMessage *) g_object_get_qdata (G_OBJECT (object), dbus_connection_quark);
}

void
_oobs_object_set_dbus_message (OobsObject *object, DBusMessage *message)
{
  g_object_set_qdata_full (G_OBJECT (object), dbus_connection_quark,
			   message, (GDestroyNotify) dbus_message_unref);
}

static OobsResult
update_object_from_message (OobsObject  *object,
			    DBusMessage *message)
{
  OobsObjectClass *class;

  class = OOBS_OBJECT_GET_CLASS (object);

  if (!class->update)
    {
      g_critical ("There is no update() implementation for this object");
      return OOBS_RESULT_MALFORMED_DATA;
    }

  g_object_set_qdata (G_OBJECT (object), dbus_connection_quark, message);
  class->update (object);
  g_object_steal_qdata (G_OBJECT (object), dbus_connection_quark);

  return OOBS_RESULT_OK;
}

static DBusMessage*
run_message (OobsObject  *object,
	     DBusMessage *message,
	     OobsResult  *result)
{
  OobsObjectPrivate *priv;
  DBusConnection    *connection;
  DBusMessage       *reply;

  priv = object->_priv;
  g_return_val_if_fail (oobs_session_get_connected (priv->session), NULL);
  connection = _oobs_session_get_connection_bus (priv->session);

  reply = dbus_connection_send_with_reply_and_block (connection, message, -1, &priv->dbus_error);

  if (dbus_error_is_set (&priv->dbus_error))
    {
      if (dbus_error_has_name (&priv->dbus_error, DBUS_ERROR_ACCESS_DENIED))
	*result = OOBS_RESULT_ACCESS_DENIED;
      else
	g_critical ("There was an unknown error communicating with the backends: %s", priv->dbus_error.message);

      dbus_error_free (&priv->dbus_error);
      return NULL;
    }

  *result = OOBS_RESULT_OK;
  return reply;
}

static void
async_message_cb (DBusPendingCall *pending_call, gpointer data)
{
  OobsObjectAsyncCallbackData *async_data;
  OobsResult result = OOBS_RESULT_MALFORMED_DATA;
  DBusMessage *reply;
  DBusError error;

  dbus_error_init (&error);
  async_data = (OobsObjectAsyncCallbackData*) data;
  reply = dbus_pending_call_steal_reply (pending_call);

  if (dbus_set_error_from_message (&error, reply))
    {
      if (dbus_error_has_name (&error, DBUS_ERROR_ACCESS_DENIED))
	result = OOBS_RESULT_ACCESS_DENIED;
      else
	{
	  /* FIXME: process error */
	  result = OOBS_RESULT_MALFORMED_DATA;
	}

      dbus_error_free (&error);
    }
  else
    {
      if (async_data->update)
	result = update_object_from_message (OOBS_OBJECT (async_data->object), reply);
      else
	result = OOBS_RESULT_OK;
    }

  if (async_data->func)
    (* async_data->func) (OOBS_OBJECT (async_data->object), result, async_data->data);

  dbus_message_unref (reply);
  g_object_unref (async_data->object);
}

static void
run_message_async (OobsObject          *object,
		   DBusMessage         *message,
		   gboolean             update,
		   OobsObjectAsyncFunc  func,
		   gpointer             data)
{
  OobsObjectPrivate *priv;
  DBusPendingCall *call;
  OobsObjectAsyncCallbackData *async_data;
  DBusConnection *connection;

  priv = object->_priv;
  g_return_if_fail (oobs_session_get_connected (priv->session));
  connection = _oobs_session_get_connection_bus (priv->session);
  dbus_connection_send_with_reply (connection, message, &call, -1);

  async_data = g_new0 (OobsObjectAsyncCallbackData, 1);
  async_data->object = g_object_ref (G_OBJECT (object));
  async_data->update = update;
  async_data->func = func;
  async_data->data = data;

  dbus_pending_call_set_notify (call, async_message_cb, async_data, g_free);
}

static DBusMessage*
get_commit_message (OobsObject *object)
{
  OobsObjectClass   *class;
  OobsObjectPrivate *priv;
  DBusMessage       *message;

  priv  = object->_priv;
  class = OOBS_OBJECT_GET_CLASS (object);

  if (!priv->session)
    {
      g_critical ("Trying to commit changes after the session has terminated, "
		  "this reflects a bug in the application");
      return NULL;
    }

  if (!class->commit)
    {
      g_critical ("There is no commit() implementation for this object");
      return NULL;
    }

  message = dbus_message_new_method_call (OOBS_DBUS_DESTINATION, priv->path, priv->method, "set");

  /* Let the commit() implementation fill the message */
  _oobs_object_set_dbus_message (object, message);
  class->commit (object);
  message = g_object_steal_qdata (G_OBJECT (object), dbus_connection_quark);

  if (!message)
    {
      /* a NULL message means malformed configuration */
      g_critical ("Not committing due to inconsistencies in the "
		  "configuration, this reflects a bug in the application\n");
    }

  return message;
}

static DBusMessage*
get_update_message (OobsObject *object)
{
  OobsObjectPrivate *priv;

  priv = object->_priv;

  if (!priv->session)
    {
      g_critical ("Trying to update the object after the session has terminated, "
		  "this reflects a bug in the application");
      return NULL;
    }

  return dbus_message_new_method_call (OOBS_DBUS_DESTINATION, priv->path, priv->method, "get");
}

/**
 * oobs_object_commit:
 * @object: an #OobsObject
 * 
 * Commits to the system all the changes done
 * to the configuration held by an #OobsObject.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_object_commit (OobsObject *object)
{
  DBusMessage *message;
  OobsResult result;

  g_return_val_if_fail (OOBS_IS_OBJECT (object), OOBS_RESULT_MALFORMED_DATA);

  message = get_commit_message (object);

  if (!message)
    return OOBS_RESULT_MALFORMED_DATA;

  run_message (object, message, &result);
  dbus_message_unref (message);

  return result;
}

/**
 * oobs_object_commit_async:
 * @object: An #OobsObject.
 * @func: An #OobsObjectAsyncFunc that will be called when the asynchronous operation has ended.
 * @data: Aditional data to pass to @func.
 * 
 * Commits to the system all the changes done to the configuration held by an #OobsObject.
 * This change will be asynchronous, being run the function @func when the change has been done.
 * 
 * Return value: an #OobsResult enum with the error code. Due to the asynchronous nature
 * of the function, only OOBS_RESULT_MALFORMED and OOBS_RESULT_OK can be returned.
 **/
OobsResult
oobs_object_commit_async (OobsObject          *object,
			  OobsObjectAsyncFunc  func,
			  gpointer             data)
{
  DBusMessage *message;

  g_return_val_if_fail (OOBS_IS_OBJECT (object), OOBS_RESULT_MALFORMED_DATA);

  message = get_commit_message (object);

  if (!message)
    return OOBS_RESULT_MALFORMED_DATA;

  run_message_async (object, message, FALSE, func, data);
  dbus_message_unref (message);

  return OOBS_RESULT_OK;
}

/**
 * oobs_object_update:
 * @object: an #OobsObject
 * 
 * Synchronizes the configuration held by the #OobsObject
 * with the actual system configuration. All the changes done
 * to the configuration held by the #OobsObject will be forgotten.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_object_update (OobsObject *object)
{
  DBusMessage *message, *reply;
  OobsResult result = OOBS_RESULT_MALFORMED_DATA;

  g_return_val_if_fail (OOBS_IS_OBJECT (object), OOBS_RESULT_MALFORMED_DATA);

  message = get_update_message (object);

  if (!message)
    return OOBS_RESULT_MALFORMED_DATA;

  reply = run_message (object, message, &result);

  if (reply)
    {
      result = update_object_from_message (object, reply);
      dbus_message_unref (reply);
    }
      
  dbus_message_unref (message);
  return result;
}

/**
 * oobs_object_update_async:
 * @object: An #OobsObject
 * @func: An #OobsObjectAsyncFunc that will be called when the asynchronous operation has ended.
 * @data: Aditional data to pass to @func.
 * 
 * Synchronizes the configuration held by the #OobsObject
 * with the actual system configuration. All the changes done
 * to the configuration held by the #OobsObject will be forgotten.
 * The update operation will be asynchronous, being run the
 * function @func when the update has been done.
 * 
 * Return value: an #OobsResult enum with the error code. Due to the asynchronous nature
 * of the function, only OOBS_RESULT_MALFORMED and OOBS_RESULT_OK can be returned.
 **/
OobsResult
oobs_object_update_async (OobsObject          *object,
			  OobsObjectAsyncFunc  func,
			  gpointer             data)
{
  DBusMessage *message;

  message = get_update_message (object);

  if (!message)
    return OOBS_RESULT_MALFORMED_DATA;

  run_message_async (object, message, TRUE, func, data);
  dbus_message_unref (message);

  return OOBS_RESULT_OK;
}
