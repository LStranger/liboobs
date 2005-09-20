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
#include <sys/types.h>
#include <string.h>

#include "oobs-object.h"
#include "oobs-object-private.h"
#include "oobs-list.h"
#include "oobs-list-private.h"
#include "oobs-usersconfig.h"
#include "oobs-user.h"
#include "oobs-defines.h"

#define USERS_CONFIG_REMOTE_OBJECT "UsersConfig"
#define OOBS_USERS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_USERS_CONFIG, OobsUsersConfigPrivate))

typedef struct _OobsUsersConfigPrivate OobsUsersConfigPrivate;

struct _OobsUsersConfigPrivate
{
  OobsList *users_list;

  gboolean  use_md5;
  uid_t     minimum_uid;
  uid_t     maximum_uid;
  gchar    *default_shell;
  gchar    *default_home;
};

static void oobs_users_config_class_init (OobsUsersConfigClass *class);
static void oobs_users_config_init       (OobsUsersConfig      *config);
static void oobs_users_config_finalize   (GObject              *object);

static void oobs_users_config_set_property (GObject      *object,
					    guint         prop_id,
					    const GValue *value,
					    GParamSpec   *pspec);
static void oobs_users_config_get_property (GObject      *object,
					    guint         prop_id,
					    GValue       *value,
					    GParamSpec   *pspec);

static void oobs_users_config_update     (OobsObject   *object);
static void oobs_users_config_commit     (OobsObject   *object);

enum
{
  PROP_0,
  PROP_USE_MD5,
  PROP_MINIMUM_UID,
  PROP_MAXIMUM_UID,
  PROP_DEFAULT_SHELL,
  PROP_DEFAULT_HOME
};

G_DEFINE_TYPE (OobsUsersConfig, oobs_users_config, OOBS_TYPE_OBJECT);


static void
oobs_users_config_class_init (OobsUsersConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->set_property = oobs_users_config_set_property;
  object_class->get_property = oobs_users_config_get_property;
  object_class->finalize     = oobs_users_config_finalize;

  oobs_object_class->commit  = oobs_users_config_commit;
  oobs_object_class->update  = oobs_users_config_update;

  g_object_class_install_property (object_class,
				   PROP_USE_MD5,
				   g_param_spec_boolean ("use-md5",
							 "Use MD5",
							 "Whether users' passwords are stored in MD5",
							 FALSE,
							 G_PARAM_READABLE));
  g_object_class_install_property (object_class,
				   PROP_MINIMUM_UID,
				   g_param_spec_int ("minimum-uid",
						     "Minimum UID",
						     "Minimum UID for non-system users",
						     0, OOBS_MAX_UID, OOBS_MAX_UID,
						     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_MAXIMUM_UID,
				   g_param_spec_int ("maximum-uid",
						     "Maximum UID",
						     "Maximum UID for non-system users",
						     0, OOBS_MAX_UID, OOBS_MAX_UID,
						     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DEFAULT_SHELL,
				   g_param_spec_string ("default-shell",
							"Default shell",
							"Default shell for new users",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DEFAULT_HOME,
				   g_param_spec_string ("default-home",
							"Default home directory",
							"Default home directory for new users",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsUsersConfigPrivate));
}

static void
oobs_users_config_init (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_USERS_CONFIG (config));

  priv = OOBS_USERS_CONFIG_GET_PRIVATE (config);

  priv->users_list = _oobs_list_new (OOBS_TYPE_USER);
}

static void
oobs_users_config_finalize (GObject *object)
{
  OobsUsersConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_USERS_CONFIG (object));

  priv = OOBS_USERS_CONFIG_GET_PRIVATE (object);

  if (priv && priv->users_list)
    g_object_unref (priv->users_list);

  if (G_OBJECT_CLASS (oobs_users_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_users_config_parent_class)->finalize) (object);
}

static void
oobs_users_config_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  OobsUsersConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_USERS_CONFIG (object));

  priv = OOBS_USERS_CONFIG_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_MINIMUM_UID:
      priv->minimum_uid = g_value_get_int (value);
      break;
    case PROP_MAXIMUM_UID:
      priv->maximum_uid = g_value_get_int (value);
      break;
    case PROP_DEFAULT_SHELL:
      g_free (priv->default_shell);
      priv->default_shell = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_HOME:
      g_free (priv->default_home);
      priv->default_home = g_value_dup_string (value);
      break;
    }
}

static void
oobs_users_config_get_property (GObject      *object,
				guint         prop_id,
				GValue       *value,
				GParamSpec   *pspec)
{
  OobsUsersConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_USERS_CONFIG (object));

  priv = OOBS_USERS_CONFIG_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_USE_MD5:
      g_value_set_boolean (value, priv->use_md5);
      break;
    case PROP_MINIMUM_UID:
      g_value_set_int (value, priv->minimum_uid);
      break;
    case PROP_MAXIMUM_UID:
      g_value_set_int (value, priv->minimum_uid);
      break;
    case PROP_DEFAULT_SHELL:
      g_value_set_string (value, priv->default_shell);
      break;
    case PROP_DEFAULT_HOME:
      g_value_set_string (value, priv->default_home);
      break;
    }
}

static OobsUser*
create_user_from_dbus_reply (OobsObject      *object,
			     DBusMessage     *reply,
			     DBusMessageIter  struct_iter)
{
  DBusMessageIter iter;
  int    id, uid, gid;
  gchar *login, *passwd, *home, *shell;

  dbus_message_iter_recurse (&struct_iter, &iter);

  dbus_message_iter_get_basic (&iter, &id);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &login);
  dbus_message_iter_next (&iter);
  
  dbus_message_iter_get_basic (&iter, &passwd);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &uid);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &gid);
  dbus_message_iter_next (&iter);

  /* FIXME: missing GECOS fields */
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &home);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &shell);
  dbus_message_iter_next (&iter);

  return g_object_new (OOBS_TYPE_USER,
		       "name", login,
		       "crypted-password", passwd,
		       "uid", uid,
		       "gid", gid,
		       "home-directory", home,
		       "shell", shell,
		       NULL);
}

static void
oobs_users_config_update (OobsObject *object)
{
  OobsUsersConfigPrivate *priv;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;
  OobsListIter     list_iter;
  GObject         *user;

  priv  = OOBS_USERS_CONFIG_GET_PRIVATE (object);
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous list */
  oobs_list_clear (priv->users_list);

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      user = G_OBJECT (create_user_from_dbus_reply (object, reply, elem_iter));

      oobs_list_append (priv->users_list, &list_iter);
      oobs_list_set    (priv->users_list, &list_iter, G_OBJECT (user));
      g_object_unref   (user);

      dbus_message_iter_next (&elem_iter);
    }

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &priv->use_md5);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &priv->minimum_uid);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &priv->maximum_uid);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &priv->default_home);
}

static void
oobs_users_config_commit (OobsObject *object)
{
}

OobsObject*
oobs_users_config_new (OobsSession *session)
{
  static OobsObject *object = NULL;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  if (!object)
    {
      object = g_object_new (OOBS_TYPE_USERS_CONFIG,
			     "remote-object", USERS_CONFIG_REMOTE_OBJECT,
			     "session",       session,
			     NULL);
      oobs_object_update (object);
    }

  return object;
}

OobsList*
oobs_users_config_get_users (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);

  priv = OOBS_USERS_CONFIG_GET_PRIVATE (config);

  return priv->users_list;
}

uid_t
oobs_users_config_get_minimum_users_uid (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, OOBS_MAX_UID);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), OOBS_MAX_UID);

  priv = OOBS_USERS_CONFIG_GET_PRIVATE (config);

  return priv->minimum_uid;
}

void
oobs_users_config_set_minimum_users_uid (OobsUsersConfig *config, uid_t uid)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (OOBS_IS_USERS_CONFIG (config));

  g_object_set (G_OBJECT (config), "minimum-uid", uid, NULL);
}

uid_t
oobs_users_config_get_maximum_users_uid (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, OOBS_MAX_UID);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), OOBS_MAX_UID);

  priv = OOBS_USERS_CONFIG_GET_PRIVATE (config);

  return priv->maximum_uid;
}

void
oobs_users_config_set_maximum_users_uid (OobsUsersConfig *config, uid_t uid)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (OOBS_IS_USERS_CONFIG (config));

  g_object_set (G_OBJECT (config), "maximum-uid", uid, NULL);
}

G_CONST_RETURN gchar*
oobs_users_config_get_default_shell (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);

  priv = OOBS_USERS_CONFIG_GET_PRIVATE (config);

  return priv->default_shell;
}

void
oobs_users_config_set_default_shell (OobsUsersConfig *config, const gchar *shell)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (OOBS_IS_USERS_CONFIG (config));

  g_object_set (G_OBJECT (config), "default-shell", shell, NULL);
}

G_CONST_RETURN gchar*
oobs_users_config_get_default_home_dir (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);

  priv = OOBS_USERS_CONFIG_GET_PRIVATE (config);

  return priv->default_home;
}

void
oobs_users_config_set_default_home_dir (OobsUsersConfig *config, const gchar *home_dir)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (OOBS_IS_USERS_CONFIG (config));

  g_object_set (G_OBJECT (config), "default-home", home_dir, NULL);
}
