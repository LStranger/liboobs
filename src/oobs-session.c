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
#include <dbus/dbus-glib.h>
#include <glib-object.h>
#include <glib.h>
#include "oobs-session.h"
#include "oobs-object.h"

#define OOBS_SESSION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SESSION, OobsSessionPrivate))

typedef struct _OobsSessionPrivate OobsSessionPrivate;

struct _OobsSessionPrivate
{
  DBusConnection *connection;

  GList    *session_objects;
  gboolean  is_authenticated;
  gboolean  commit_on_exit;
};

static void oobs_session_class_init (OobsSessionClass *class);
static void oobs_session_init       (OobsSession      *session);
static void oobs_session_finalize   (GObject         *object);

static void oobs_session_set_property (GObject      *object,
				       guint         prop_id,
				       const GValue *value,
				       GParamSpec   *pspec);
static void oobs_session_get_property (GObject      *object,
				       guint         prop_id,
				       GValue       *value,
				       GParamSpec   *pspec);
enum
{
  PROP_0,
  PROP_COMMIT_ON_EXIT
};

G_DEFINE_TYPE (OobsSession, oobs_session, G_TYPE_OBJECT);

static void
oobs_session_class_init (OobsSessionClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_session_set_property;
  object_class->get_property = oobs_session_get_property;
  object_class->finalize     = oobs_session_finalize;

  g_object_class_install_property (object_class,
				   PROP_COMMIT_ON_EXIT,
				   g_param_spec_boolean ("commit-on-exit",
							 "Commit on exit",
							 "Tells whether the session should commit all the children objects on finalize",
							 FALSE,
							 G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsSessionPrivate));
}

static void
oobs_session_init (OobsSession *session)
{
  OobsSessionPrivate *priv;
  DBusError          error;

  g_return_if_fail (OOBS_IS_SESSION (session));
  priv = OOBS_SESSION_GET_PRIVATE (session);

  dbus_error_init (&error);
  priv->connection = dbus_bus_get (DBUS_BUS_SESSION, NULL);
  dbus_connection_setup_with_g_main (priv->connection, NULL);

  if (!priv->connection)
    g_warning (error.message);

  priv->session_objects  = NULL;
  priv->is_authenticated = FALSE;
  priv->commit_on_exit   = FALSE;
}

static void
unregister_object_node (OobsSessionPrivate *priv, GList *node)
{
  priv->session_objects = g_list_remove_link (priv->session_objects, node);

  /* FIXME: This sucks a bit, there must be a way for getting the OobsObjects notified
   * of the session finalizing after the actual finalize (unlike g_object_weak_ref)
   */
  g_object_set   (G_OBJECT (node->data),
		  "session", NULL,
		  NULL);
  g_object_unref (G_OBJECT (node->data));
  g_list_free_1  (node);
}

static void
unregister_objects_list (OobsSessionPrivate *priv)
{
  while (priv->session_objects)
    unregister_object_node (priv, priv->session_objects);
}

static void
oobs_session_finalize (GObject *object)
{
  OobsSession *session;
  OobsSessionPrivate *priv;

  g_return_if_fail (OOBS_IS_SESSION (object));

  session = OOBS_SESSION (object);
  priv    = OOBS_SESSION_GET_PRIVATE (session);

  if (priv)
    {
      if (priv->commit_on_exit)
	oobs_session_commit (session);

      unregister_objects_list (priv);

      dbus_connection_disconnect (priv->connection);
      dbus_connection_close (priv->connection);
      /* dbus_connection_unref (priv->connection); */
    }

  if (G_OBJECT_CLASS (oobs_session_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_session_parent_class)->finalize) (object);
}

static void
oobs_session_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  OobsSession *obj;
  OobsSessionPrivate *priv;

  g_return_if_fail (OOBS_IS_SESSION (object));

  obj  = OOBS_SESSION (object);
  priv = OOBS_SESSION_GET_PRIVATE (obj);

  switch (prop_id)
    {
    case PROP_COMMIT_ON_EXIT:
      priv->commit_on_exit = g_value_get_boolean (value);
      break;
    }
}

static void
oobs_session_get_property (GObject      *object,
			   guint         prop_id,
			   GValue       *value,
			   GParamSpec   *pspec)
{
  OobsSession *obj;
  OobsSessionPrivate *priv;

  g_return_if_fail (OOBS_IS_SESSION (object));

  obj  = OOBS_SESSION (object);
  priv = OOBS_SESSION_GET_PRIVATE (obj);

  switch (prop_id)
    {
    case PROP_COMMIT_ON_EXIT:
      g_value_set_boolean (value, priv->commit_on_exit);
      break;
    }
}

/**
 * oobs_session_get:
 * 
 * Returns the global #OobsSession singleton, which represents
 * the session with the system tools backends.
 * 
 * Return Value: the singleton #OobSession object.
 **/
OobsSession*
oobs_session_get (void)
{
  static OobsSession *session = NULL;

  if (!session)
    session = g_object_new (OOBS_TYPE_SESSION, NULL);

  return session;
}

/**
 * oobs_session_set_autocommit_on_exit:
 * @session: an #OobsSession
 * @commit: #TRUE to make the session commit all
 *          changes when the session is being destroyed
 * 
 * When set to #TRUE, it tells the #OobsSession
 * object to save all changes before being destroyed
 **/
void
oobs_session_set_autocommit_on_exit (OobsSession *session, gboolean commit)
{
  OobsSessionPrivate *priv;

  g_return_if_fail (session != NULL);
  g_return_if_fail (OOBS_IS_SESSION (session));

  priv = OOBS_SESSION_GET_PRIVATE (session);
  priv->commit_on_exit = commit;
  g_object_notify (G_OBJECT (session), "commit-on-exit");
}

/**
 * oobs_session_get_autocommit_on_exit:
 * @session: an #OobsSession
 * 
 * Returns whether the #OobsSession object is going to save all configuration
 * when it's being destroyed
 * 
 * Return Value: #TRUE if the #OobsSession object is going to commit the
 *               configuration before being destroyed
 **/
gboolean
oobs_session_get_autocommit_on_exit (OobsSession *session)
{
  OobsSessionPrivate *priv;

  g_return_val_if_fail (session != NULL, FALSE);
  g_return_val_if_fail (OOBS_IS_SESSION (session), FALSE);

  priv = OOBS_SESSION_GET_PRIVATE (session);
  return priv->commit_on_exit;
}

/**
 * oobs_session_commit:
 * @session: an #OobsSession
 * 
 * Commits inmediately all the changes to the configuration
 * objects that have been requested through this #OobsSession
 **/
void
oobs_session_commit (OobsSession *session)
{
  OobsSessionPrivate *priv;
  GList              *node;
  OobsObject         *object;

  g_return_if_fail (session != NULL);
  g_return_if_fail (OOBS_IS_SESSION (session));

  priv = OOBS_SESSION_GET_PRIVATE (session);
  node = priv->session_objects;

  while (node)
    {
      object = OOBS_OBJECT (node->data);
      oobs_object_commit (object);

      node = node->next;
    }
}

/* protected methods */
DBusConnection*
_oobs_session_get_connection_bus (OobsSession *session)
{
  OobsSessionPrivate *priv;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  priv = OOBS_SESSION_GET_PRIVATE (session);
  return priv->connection;
}

void
_oobs_session_register_object (OobsSession *session, OobsObject *object)
{
  OobsSessionPrivate *priv;

  if (!session || !object)
    return;

  priv = OOBS_SESSION_GET_PRIVATE (session);
  priv->session_objects = g_list_prepend (priv->session_objects,
					  g_object_ref (object));
}

void
_oobs_session_unregister_object (OobsSession *session, OobsObject *object)
{
  OobsSessionPrivate *priv;
  GList              *node;
  gboolean            found;

  if (!session || !object)
    return;

  priv  = OOBS_SESSION_GET_PRIVATE (session);
  node  = priv->session_objects;
  found = FALSE;

  while (node && !found)
    {
      if (node->data == object)
        {
	  found = TRUE;
	  unregister_object_node (priv, node);
	}
      else
	node = node->next;
    }
}
