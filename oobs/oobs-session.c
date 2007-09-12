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
#include <dbus/dbus-glib-lowlevel.h>
#include <glib-object.h>
#include <glib.h>
#include "oobs-session.h"
#include "oobs-session-private.h"
#include "oobs-object.h"
#include "utils.h"

#define OOBS_SESSION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SESSION, OobsSessionPrivate))
#define PLATFORMS_PATH OOBS_DBUS_PATH_PREFIX "/Platform"
#define PLATFORMS_INTERFACE OOBS_DBUS_METHOD_PREFIX ".Platform"

typedef struct _OobsSessionPrivate OobsSessionPrivate;

struct _OobsSessionPrivate
{
  DBusConnection *connection;
  DBusError       dbus_error;

  GList    *session_objects;
  gboolean  is_authenticated;

  gchar    *platform;
  GList    *supported_platforms;
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
  PROP_PLATFORM
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
				   PROP_PLATFORM,
				   g_param_spec_string ("platform",
							"Platform",
							"Name of the platform the session is running on",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsSessionPrivate));
}

static void
oobs_session_init (OobsSession *session)
{
  OobsSessionPrivate *priv;

  g_return_if_fail (OOBS_IS_SESSION (session));
  priv = OOBS_SESSION_GET_PRIVATE (session);

  dbus_error_init (&priv->dbus_error);
  priv->connection = dbus_bus_get (DBUS_BUS_SYSTEM, &priv->dbus_error);

  if (dbus_error_is_set (&priv->dbus_error))
    g_warning (priv->dbus_error.message);
  else
    dbus_connection_setup_with_g_main (priv->connection, NULL);

  priv->session_objects  = NULL;
  priv->is_authenticated = FALSE;
  session->_priv = priv;
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
  priv    = session->_priv;

  if (priv)
    unregister_objects_list (priv);

  if (G_OBJECT_CLASS (oobs_session_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_session_parent_class)->finalize) (object);
}

static void
oobs_session_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  OobsSession *session;
  OobsSessionPrivate *priv;

  g_return_if_fail (OOBS_IS_SESSION (object));

  session = OOBS_SESSION (object);
  priv    = session->_priv;

  switch (prop_id)
    {
    case PROP_PLATFORM:
      oobs_session_set_platform (session, g_value_get_string (value));
      break;
    }
}

static void
oobs_session_get_property (GObject      *object,
			   guint         prop_id,
			   GValue       *value,
			   GParamSpec   *pspec)
{
  OobsSessionPrivate *priv;

  g_return_if_fail (OOBS_IS_SESSION (object));

  priv = OOBS_SESSION (object)->_priv;

  switch (prop_id)
    {
    case PROP_PLATFORM:
      g_value_set_string (value, priv->platform);
      break;
    }
}

/**
 * oobs_session_get:
 * 
 * Returns the #OobsSession singleton, which represents
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
 * oobs_session_commit:
 * @session: an #OobsSession
 * 
 * Commits inmediately all the changes to the configuration
 * objects that have been requested through this #OobsSession.
 * Note that it will stop if it finds any error.
 *
 * Return Value: An #OobsResult representing the error.
 **/
OobsResult
oobs_session_commit (OobsSession *session)
{
  OobsSessionPrivate *priv;
  GList              *node;
  OobsObject         *object;
  OobsResult          result = OOBS_RESULT_OK;

  g_return_val_if_fail (session != NULL, OOBS_RESULT_ERROR);
  g_return_val_if_fail (OOBS_IS_SESSION (session), OOBS_RESULT_ERROR);

  priv = session->_priv;
  node = priv->session_objects;

  while (node && (result == OOBS_RESULT_OK))
    {
      object = OOBS_OBJECT (node->data);
      result = oobs_object_commit (object);

      node = node->next;
    }

  return result;
}

/**
 * oobs_session_get_connected:
 * @session: An #OobsSession
 * 
 * Returns whether the connection with the backends is established.
 * 
 * Return Value: #TRUE if there's connection with the backends.
 **/
gboolean
oobs_session_get_connected (OobsSession *session)
{
  OobsSessionPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SESSION (session), FALSE);

  priv = session->_priv;
  return (priv->connection != NULL);
}

/**
 * oobs_session_get_platform:
 * @session: An #OobsSession.
 * @platform: location to store the current platform, or #NULL. This
 *            string is of internal use, and must not be freed or modified.
 * 
 * Retrieves the platform your system has been identified with, or
 * #NULL in case your platform is not recognized or other error happens.
 * 
 * Return Value: An #OobsResult representing the error.
 **/
OobsResult
oobs_session_get_platform (OobsSession  *session,
			   gchar       **platform)
{
  OobsSessionPrivate *priv;
  DBusMessage *message, *reply;
  DBusMessageIter iter;
  OobsResult result;
  const gchar *str;

  g_return_val_if_fail (OOBS_IS_SESSION (session), OOBS_RESULT_ERROR);

  priv = session->_priv;
  g_return_val_if_fail (priv->connection != NULL, OOBS_RESULT_ERROR);

  message = dbus_message_new_method_call (OOBS_DBUS_DESTINATION,
					  PLATFORMS_PATH,
					  PLATFORMS_INTERFACE,
					  "getPlatform");

  reply = dbus_connection_send_with_reply_and_block (priv->connection,
						     message, -1, &priv->dbus_error);
  dbus_message_unref (message);

  if (dbus_error_is_set (&priv->dbus_error))
    {
      if (dbus_error_has_name (&priv->dbus_error, DBUS_ERROR_ACCESS_DENIED))
	result = OOBS_RESULT_ACCESS_DENIED;
      else
	result = OOBS_RESULT_ERROR;

      dbus_error_free (&priv->dbus_error);

      if (platform)
	*platform = NULL;

      return result;
    }

  dbus_message_iter_init (reply, &iter);
  str = utils_get_string (&iter);
  priv->platform = (str) ? g_strdup (str) : NULL;

  if (platform)
    *platform = priv->platform;

  dbus_message_unref (reply);
  return (priv->platform) ? OOBS_RESULT_OK : OOBS_RESULT_NO_PLATFORM;
}

/**
 * oobs_session_set_platform:
 * @session: An #OobsSession.
 * @platform: A string defining the platform. see
 *            oobs_session_get_platforms_list() to know where to get this string.
 * 
 * Identifies your platform as the one set in @platform. This is only necessary if
 * your platform could not be guessed (and thus oobs_session_get_platform() would
 * return #OOBS_RESULT_NO_PLATFORM in this case).
 * 
 * Return Value: An #OobsResult representing the error.
 **/
OobsResult
oobs_session_set_platform (OobsSession *session,
			   const gchar *platform)
{
  OobsSessionPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter;
  DBusError error;
  OobsResult result;

  g_return_val_if_fail (OOBS_IS_SESSION (session), OOBS_RESULT_ERROR);
  g_return_val_if_fail (platform != NULL, OOBS_RESULT_ERROR);

  priv = session->_priv;
  g_return_val_if_fail (priv->connection != NULL, OOBS_RESULT_ERROR);
  dbus_error_init (&error);

  priv->platform = g_strdup (platform);
  g_object_notify (G_OBJECT (session), "platform");

  message = dbus_message_new_method_call (OOBS_DBUS_DESTINATION,
					  PLATFORMS_PATH,
					  PLATFORMS_INTERFACE,
					  "setPlatform");
  dbus_message_iter_init_append (message, &iter);
  utils_append_string (&iter, priv->platform);

  dbus_connection_send_with_reply_and_block (priv->connection, message, -1, &error);

  if (dbus_error_is_set (&error))
    {
      if (dbus_error_has_name (&error, DBUS_ERROR_NO_REPLY))
	result = OOBS_RESULT_OK;
      if (dbus_error_has_name (&error, DBUS_ERROR_ACCESS_DENIED))
	result = OOBS_RESULT_ACCESS_DENIED;
      else
	result = OOBS_RESULT_ERROR;

      dbus_error_free (&error);
    }
  else
    result = OOBS_RESULT_OK;

  return result;
}

static OobsResult
get_supported_platforms (OobsSession *session, GList **list)
{
  OobsSessionPrivate *priv;
  DBusMessage *message, *reply;
  DBusMessageIter list_iter, iter;
  OobsPlatform *platform;
  OobsResult result;
  GList *platforms = NULL;
  const gchar *str;

  priv = session->_priv;
  g_return_val_if_fail (priv->connection != NULL, OOBS_RESULT_ERROR);

  message = dbus_message_new_method_call (OOBS_DBUS_DESTINATION,
					  PLATFORMS_PATH,
					  PLATFORMS_INTERFACE,
					  "getPlatformList");

  reply = dbus_connection_send_with_reply_and_block (priv->connection,
						     message, -1, &priv->dbus_error);
  dbus_message_unref (message);

  if (dbus_error_is_set (&priv->dbus_error))
    {
      if (dbus_error_has_name (&priv->dbus_error, DBUS_ERROR_ACCESS_DENIED))
	result = OOBS_RESULT_ACCESS_DENIED;
      else
	result = OOBS_RESULT_ERROR;

      dbus_error_free (&priv->dbus_error);
      *list = NULL;
      return result;
    }

  dbus_message_iter_init (reply, &list_iter);
  dbus_message_iter_recurse (&list_iter, &list_iter);

  while (dbus_message_iter_get_arg_type (&list_iter) == DBUS_TYPE_STRUCT)
    {
      platform = g_new0 (OobsPlatform, 1);
      dbus_message_iter_recurse (&list_iter, &iter);

      str = utils_get_string (&iter);
      platform->name = g_strdup (str);
      dbus_message_iter_next (&iter);

      str = utils_get_string (&iter);
      platform->version = g_strdup (str);
      dbus_message_iter_next (&iter);
	  
      str = utils_get_string (&iter);
      platform->codename = g_strdup (str);
      dbus_message_iter_next (&iter);

      str = utils_get_string (&iter);
      platform->id = g_strdup (str);

      platforms = g_list_prepend (platforms, platform);
      dbus_message_iter_next (&list_iter);
    }

  *list = g_list_reverse (platforms);
  dbus_message_unref (reply);

  return OOBS_RESULT_OK;
}

/**
 * oobs_session_get_supported_platforms:
 * @session: An #OobsSession.
 * @platforms: return location for the list of platforms. It's a
 *             #GList of #OobsPlatform structs. You must free
 *             this list with g_list_free().
 * 
 * Retrieves the list of supported platforms, this is only necessary when
 + oobs_session_get_platform() has returned #OOBS_RESULT_NO_PLATFORM. To
 * specify a platform, you must use oobs_session_set_platform(), being
 * the platform string in that function the platform->id value inside
 * the #OobsPlatform struct.
 * 
 * Return Value: An #OobsResult representing the error.
 **/
OobsResult
oobs_session_get_supported_platforms (OobsSession  *session,
				      GList       **platforms)
{
  OobsSessionPrivate *priv;
  OobsResult result;

  /* it doesn't make any sense to call this function with platforms = NULL */
  g_return_val_if_fail (platforms != NULL, OOBS_RESULT_ERROR);
  g_return_val_if_fail (OOBS_IS_SESSION (session), OOBS_RESULT_ERROR);

  priv = session->_priv;

  if (!priv->supported_platforms)
    result = get_supported_platforms (session, &priv->supported_platforms);
  else
    {
      /* list is cached */
      result = OOBS_RESULT_OK;
    }

  *platforms = (priv->supported_platforms) ? g_list_copy (priv->supported_platforms) : NULL;
  return result;
}

/**
 * oobs_session_process_requests:
 * @session: An #OobsSession
 * 
 * Blocks until all pending asynchronous requests have been processed.
 **/
void
oobs_session_process_requests (OobsSession *session)
{
  OobsSessionPrivate *priv;

  g_return_if_fail (OOBS_IS_SESSION (session));

  priv = session->_priv;
  g_list_foreach (priv->session_objects, (GFunc) oobs_object_process_requests, NULL);
}

/* protected methods */
DBusConnection*
_oobs_session_get_connection_bus (OobsSession *session)
{
  OobsSessionPrivate *priv;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  priv = session->_priv;
  return priv->connection;
}

void
_oobs_session_register_object (OobsSession *session, OobsObject *object)
{
  OobsSessionPrivate *priv;

  if (!session || !object)
    return;

  priv = session->_priv;
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

  priv  = session->_priv;
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