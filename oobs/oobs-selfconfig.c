/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2007 Carlos Garnacho
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
#include <string.h>

#include "oobs-object.h"
#include "oobs-object-private.h"
#include "oobs-selfconfig.h"
#include "oobs-usersconfig.h"
#include "oobs-user.h"
#include "utils.h"

/**
 * SECTION:oobs-selfconfig
 * @title: OobsSelfConfig
 * @short_description: Object that represents the current user
 **/

#define SELF_CONFIG_REMOTE_OBJECT "SelfConfig2"
#define OOBS_SELF_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SELF_CONFIG, OobsSelfConfigPrivate))
#define POLICY_KIT_SELF_ACTION "org.freedesktop.systemtoolsbackends.self.set"

typedef struct _OobsSelfConfigPrivate OobsSelfConfigPrivate;

struct _OobsSelfConfigPrivate
{
  uid_t     uid;
  OobsUser *user;
};

static void oobs_self_config_class_init  (OobsSelfConfigClass *class);
static void oobs_self_config_init        (OobsSelfConfig      *config);
static void oobs_self_config_constructed (GObject             *object);
static void oobs_self_config_finalize    (GObject             *object);

static void oobs_self_config_update     (OobsObject   *object);
static void oobs_self_config_commit     (OobsObject   *object);

static const gchar * oobs_self_config_get_authentication_action (OobsObject *object);


G_DEFINE_TYPE (OobsSelfConfig, oobs_self_config, OOBS_TYPE_OBJECT);


static void
oobs_self_config_class_init (OobsSelfConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->constructed  = oobs_self_config_constructed;
  object_class->finalize     = oobs_self_config_finalize;

  oobs_object_class->commit  = oobs_self_config_commit;
  oobs_object_class->update  = oobs_self_config_update;
  oobs_object_class->get_authentication_action = oobs_self_config_get_authentication_action;

  g_type_class_add_private (object_class,
			    sizeof (OobsSelfConfigPrivate));
}

static void
oobs_self_config_init (OobsSelfConfig *config)
{
  config->_priv = OOBS_SELF_CONFIG_GET_PRIVATE (config);
}

static void
oobs_self_config_users_updated (OobsSelfConfig  *self,
				OobsUsersConfig *users)
{
  OobsSelfConfigPrivate *priv;
  OobsList *list;
  OobsListIter iter;
  gboolean valid;

  priv = self->_priv;
  list = oobs_users_config_get_users (users);
  valid = oobs_list_get_iter_first (list, &iter);

  if (priv->user)
    {
      g_object_unref (priv->user);
      priv->user = NULL;
    }

  while (valid && !priv->user)
    {
      OobsUser *user;

      user = OOBS_USER (oobs_list_get (list, &iter));

      if (oobs_user_get_uid (user) == priv->uid)
	priv->user = g_object_ref (user);

      g_object_unref (user);
      valid = oobs_list_iter_next (list, &iter);
    }
}

static void
oobs_self_config_constructed (GObject *object)
{
  /* stay tuned to users config updates */
  g_signal_connect_swapped (oobs_users_config_get (), "updated",
			    G_CALLBACK (oobs_self_config_users_updated), object);
}

static void
oobs_self_config_finalize (GObject *object)
{
  OobsSelfConfigPrivate *priv;

  priv = OOBS_SELF_CONFIG (object)->_priv;

  if (priv->user)
    g_object_unref (priv->user);

  G_OBJECT_CLASS (oobs_self_config_parent_class)->finalize (object);
}

static void
oobs_self_config_update (OobsObject *object)
{
  OobsSelfConfigPrivate *priv;
  OobsObject *users_config;
  DBusMessage *message;
  DBusMessageIter iter;

  priv = OOBS_SELF_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init (message, &iter);
  priv->uid = utils_get_uint (&iter);

  users_config = oobs_users_config_get ();

  /* just update user if the users object was already
   * updated, update will be forced later if required
   */
  if (oobs_object_has_updated (users_config))
    oobs_self_config_users_updated (OOBS_SELF_CONFIG (object),
				    OOBS_USERS_CONFIG (users_config));
}

static void
oobs_self_config_commit (OobsObject *object)
{
  OobsSelfConfigPrivate *priv;
  DBusMessageIter iter, array_iter;
  DBusMessage *message;

  priv = OOBS_SELF_CONFIG (object)->_priv;

  if (!priv->user)
    return;

  message = _oobs_object_get_dbus_message (object);
  dbus_message_iter_init_append (message, &iter);

  utils_append_uint (&iter, oobs_user_get_uid (priv->user));

  /* GECOS fields */
  dbus_message_iter_open_container (&iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_TYPE_STRING_AS_STRING,
				    &array_iter);

  utils_append_string (&array_iter, oobs_user_get_full_name (priv->user));
  utils_append_string (&array_iter, oobs_user_get_room_number (priv->user));
  utils_append_string (&array_iter, oobs_user_get_work_phone_number (priv->user));
  utils_append_string (&array_iter, oobs_user_get_home_phone_number (priv->user));
  utils_append_string (&array_iter, oobs_user_get_other_data (priv->user));

  dbus_message_iter_close_container (&iter, &array_iter);

  utils_append_string (&iter, oobs_user_get_locale (priv->user));
  /* TODO: use location when the backends support it */
  utils_append_string (&iter, "");
}

static const gchar *
oobs_self_config_get_authentication_action (OobsObject *object)
{
  return POLICY_KIT_SELF_ACTION;
}

/**
 * oobs_self_config_get:
 *
 * Returns the #OobsSelfConfig singleton, which represents
 * the user configuration for the requester uid.
 *
 * Return Value: the singleton #OobsSelfConfig object.
 **/
OobsObject*
oobs_self_config_get (void)
{
  return g_object_new (OOBS_TYPE_SELF_CONFIG,
		       "remote-object", SELF_CONFIG_REMOTE_OBJECT,
		       NULL);
}

/**
 * oobs_self_config_get_user:
 * @config: An #OobsSelfConfig.
 * 
 * Returns the #OobsUser that represents the requester user.
 * 
 * Return Value: An #OobsUser, you must not reference this object.
 **/
OobsUser*
oobs_self_config_get_user (OobsSelfConfig *config)
{
  OobsSelfConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SELF_CONFIG (config), NULL);

  oobs_object_ensure_update (oobs_users_config_get ());
  priv = config->_priv;

  return priv->user;
}
