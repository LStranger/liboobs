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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>,
 *          Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#include <dbus/dbus.h>
#include <glib-object.h>
#include <string.h>

#include "oobs-object.h"
#include "oobs-object-private.h"
#include "oobs-list.h"
#include "oobs-list-private.h"
#include "oobs-usersconfig.h"
#include "oobs-user.h"
#include "oobs-user-private.h"
#include "oobs-defines.h"
#include "oobs-groupsconfig.h"
#include "oobs-group.h"
#include "utils.h"

/**
 * SECTION:oobs-usersconfig
 * @title: OobsUsersConfig
 * @short_description: Object that represents users configuration
 * @see_also: #OobsUser
 **/

#define USERS_CONFIG_REMOTE_OBJECT "UsersConfig2"
#define OOBS_USERS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_USERS_CONFIG, OobsUsersConfigPrivate))

typedef struct _OobsUsersConfigPrivate OobsUsersConfigPrivate;

struct _OobsUsersConfigPrivate
{
  OobsList *users_list;

  GList    *shells;

  uid_t     minimum_uid;
  uid_t     maximum_uid;
  gchar    *default_shell;
  gchar    *default_home;

  GHashTable *groups;
  gid_t       default_gid;

  gboolean  encrypted_home;

  OobsGroup *default_group;
};

static void oobs_users_config_class_init  (OobsUsersConfigClass *class);
static void oobs_users_config_init        (OobsUsersConfig      *config);
static void oobs_users_config_constructed (GObject              *object);
static void oobs_users_config_finalize    (GObject              *object);

static void oobs_users_config_set_property (GObject      *object,
					    guint         prop_id,
					    const GValue *value,
					    GParamSpec   *pspec);
static void oobs_users_config_get_property (GObject      *object,
					    guint         prop_id,
					    GValue       *value,
					    GParamSpec   *pspec);

static void oobs_users_config_groups_updated (OobsUsersConfig  *users,
					      OobsGroupsConfig *groups);

static void oobs_users_config_update     (OobsObject   *object);
static void oobs_users_config_commit     (OobsObject   *object);

enum
{
  PROP_0,
  PROP_MINIMUM_UID,
  PROP_MAXIMUM_UID,
  PROP_DEFAULT_SHELL,
  PROP_DEFAULT_HOME,
  PROP_DEFAULT_GROUP,
  PROP_ENCRYPTED_HOME,
};

G_DEFINE_TYPE (OobsUsersConfig, oobs_users_config, OOBS_TYPE_OBJECT);


static void
oobs_users_config_class_init (OobsUsersConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->set_property = oobs_users_config_set_property;
  object_class->get_property = oobs_users_config_get_property;
  object_class->constructed  = oobs_users_config_constructed;
  object_class->finalize     = oobs_users_config_finalize;

  oobs_object_class->commit  = oobs_users_config_commit;
  oobs_object_class->update  = oobs_users_config_update;

  g_object_class_install_property (object_class,
				   PROP_MINIMUM_UID,
				   g_param_spec_uint ("minimum-uid",
						      "Minimum UID",
				                      "Minimum UID for non-system users",
						      0, OOBS_MAX_UID, OOBS_MAX_UID,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_MAXIMUM_UID,
				   g_param_spec_uint ("maximum-uid",
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
  g_object_class_install_property (object_class,
				   PROP_DEFAULT_GROUP,
				   g_param_spec_object ("default-group",
							"Default group",
							"Default group for new users",
							OOBS_TYPE_GROUP,
							G_PARAM_READABLE));
  g_object_class_install_property (object_class,
				   PROP_ENCRYPTED_HOME,
				   g_param_spec_boolean ("encrypted-home",
				                         "Encrypted home support",
				                         "Whether encrypted home dirs are supported",
				                         FALSE,
				                         G_PARAM_READABLE));

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
  config->_priv = priv;

  priv->groups = g_hash_table_new (NULL, NULL);
}

static void
free_configuration (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  priv = config->_priv;

  oobs_list_clear (priv->users_list);
  g_free (priv->default_shell);
  g_free (priv->default_home);

  if (priv->shells)
    {
      g_list_foreach (priv->shells, (GFunc) g_free, NULL);
      g_list_free (priv->shells);
      priv->shells = NULL;
    }

  g_hash_table_remove_all (priv->groups);
}

static void
oobs_users_config_constructed (GObject *object)
{
  /* stay tuned to groups config updates */
  g_signal_connect_swapped (oobs_groups_config_get (), "updated",
			    G_CALLBACK (oobs_users_config_groups_updated), object);
}

static void
oobs_users_config_finalize (GObject *object)
{
  OobsUsersConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_USERS_CONFIG (object));

  priv = OOBS_USERS_CONFIG (object)->_priv;

  if (priv)
    {
      free_configuration (OOBS_USERS_CONFIG (object));
      g_hash_table_unref (priv->groups);

      if (priv->users_list)
	g_object_unref (priv->users_list);
    }

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

  priv = OOBS_USERS_CONFIG (object)->_priv;

  switch (prop_id)
    {
    case PROP_MINIMUM_UID:
      priv->minimum_uid = g_value_get_uint (value);
      break;
    case PROP_MAXIMUM_UID:
      priv->maximum_uid = g_value_get_uint (value);
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

  priv = OOBS_USERS_CONFIG (object)->_priv;

  switch (prop_id)
    {
    case PROP_MINIMUM_UID:
      g_value_set_uint (value, priv->minimum_uid);
      break;
    case PROP_MAXIMUM_UID:
      g_value_set_uint (value, priv->maximum_uid);
      break;
    case PROP_DEFAULT_SHELL:
      g_value_set_string (value, priv->default_shell);
      break;
    case PROP_DEFAULT_HOME:
      g_value_set_string (value, priv->default_home);
      break;
    case PROP_ENCRYPTED_HOME:
      g_value_set_boolean (value, priv->encrypted_home);
    }
}

static void
query_groups_foreach (OobsUser *user,
		      gid_t     gid,
		      gpointer  data)
{
  OobsGroupsConfig *groups_config = OOBS_GROUPS_CONFIG (data);
  OobsGroup *group;

  group = oobs_groups_config_get_from_gid (groups_config, gid);
  oobs_user_set_main_group (user, group);

  if (group)
    g_object_unref (group);
}

static void
oobs_users_config_groups_updated (OobsUsersConfig  *users,
				  OobsGroupsConfig *groups)
{
  OobsUsersConfigPrivate *priv;
  OobsGroup *group;

  priv = users->_priv;
  g_hash_table_foreach (priv->groups, (GHFunc) query_groups_foreach, groups);

  /* get the default group */
  if (priv->default_gid > 0)
    {
      group = oobs_groups_config_get_from_gid (groups, priv->default_gid);
      priv->default_group = group;

      if (group)
	g_object_unref (group);
    }
}

static void
oobs_users_config_update (OobsObject *object)
{
  OobsUsersConfigPrivate *priv;
  OobsObject      *groups_config;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;
  OobsListIter     list_iter;
  GObject         *user;
  gid_t            gid;

  priv  = OOBS_USERS_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous configuration */
  free_configuration (OOBS_USERS_CONFIG (object));

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      user = G_OBJECT (_oobs_user_create_from_dbus_reply (NULL, &gid, reply, elem_iter));

      oobs_list_append (priv->users_list, &list_iter);
      oobs_list_set    (priv->users_list, &list_iter, G_OBJECT (user));

      g_object_unref (user);

      /* keep the group name in a hashtable, this will be needed
       * each time the groups configuration changes
       */
      g_hash_table_insert (priv->groups,
                           user,
                           (gpointer) gid);

      dbus_message_iter_next (&elem_iter);
    }

  dbus_message_iter_next (&iter);
  priv->shells = utils_get_string_list_from_dbus_reply (reply, &iter);

  priv->minimum_uid = utils_get_uint (&iter);
  priv->maximum_uid = utils_get_uint (&iter);

  priv->default_home = g_strdup (utils_get_string (&iter));
  priv->default_shell = g_strdup (utils_get_string (&iter));
  priv->default_gid = utils_get_uint (&iter);
  priv->encrypted_home = utils_get_boolean (&iter);

  groups_config = oobs_groups_config_get ();

  /* just update groups if the object was already
   * updated, update will be forced later if required
   */
  if (oobs_object_has_updated (groups_config))
    oobs_users_config_groups_updated (OOBS_USERS_CONFIG (object),
				      OOBS_GROUPS_CONFIG (groups_config));
}

static void
oobs_users_config_commit (OobsObject *object)
{
  OobsUsersConfigPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter;
  guint32 default_gid;

  priv = OOBS_USERS_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);

  utils_create_dbus_array_from_string_list (priv->shells, message, &iter);
  utils_append_uint (&iter, priv->minimum_uid);
  utils_append_uint (&iter, priv->maximum_uid);
  utils_append_string (&iter, priv->default_home);
  utils_append_string (&iter, priv->default_shell);

  default_gid = (priv->default_group) ? oobs_group_get_gid (priv->default_group) : G_MAXUINT32;
  utils_append_uint (&iter, default_gid);

  utils_append_boolean (&iter, priv->encrypted_home);
}

/**
 * oobs_users_config_get:
 * 
 * Returns the #OobsUsersConfig singleton, which represents
 * the system users and their configuration.
 * 
 * Return Value: the singleton #OobsUsersConfig object.
 **/
OobsObject*
oobs_users_config_get (void)
{
  static OobsObject *the_object = NULL;

  if (!the_object)
    the_object = g_object_new (OOBS_TYPE_USERS_CONFIG,
                               "remote-object", USERS_CONFIG_REMOTE_OBJECT,
                               NULL);

  return the_object;
}

/**
 * oobs_users_config_get_users:
 * @config: An #OobsUsersConfig.
 * 
 * Returns an #OobsList containing objects of type #OobsUser.
 * 
 * Return Value: an #OobsList containing the system users.
 **/
OobsList*
oobs_users_config_get_users (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);

  oobs_object_ensure_update (oobs_groups_config_get ());
  priv = config->_priv;

  return priv->users_list;
}

/**
 * oobs_users_config_add_user:
 * @config: An #OobsUsersConfig.
 * @user: An #OobsUser.
 *
 * Add an user to the configuration, immediately committing changes to the system.
 * On success, @user will be appended to the users list.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_users_config_add_user (OobsUsersConfig *config, OobsUser *user)
{
  OobsUsersConfigPrivate *priv;
  OobsListIter list_iter;
  OobsResult result;

  g_return_val_if_fail (config != NULL, OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (user != NULL, OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (OOBS_IS_USER (user), OOBS_RESULT_MALFORMED_DATA);

  result = oobs_object_add (OOBS_OBJECT (user));

  if (result != OOBS_RESULT_OK)
    return result;

  priv = config->_priv;

  oobs_list_append (priv->users_list, &list_iter);
  oobs_list_set (priv->users_list, &list_iter, G_OBJECT (user));

  return OOBS_RESULT_OK;
}

/**
 * oobs_users_config_delete_user:
 * @config: An #OobsUsersConfig.
 * @user: An #OobsUser.
 *
 * Delete an user from the configuration, immediately committing changes to the system.
 * On success, @user will be removed from the users list.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_users_config_delete_user (OobsUsersConfig *config, OobsUser *user)
{
  OobsUsersConfigPrivate *priv;
  OobsUser *list_user;
  OobsListIter list_iter;
  gboolean valid;
  OobsResult result;
  OobsList *groups_list;
  OobsGroup *group;

  g_return_val_if_fail (config != NULL, OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (user != NULL, OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (OOBS_IS_USER (user), OOBS_RESULT_MALFORMED_DATA);

  /* Try to commit changes */
  result = oobs_object_delete (OOBS_OBJECT (user));

  if (result != OOBS_RESULT_OK)
    return result;


  priv = config->_priv;

  /* Remove user from all groups, to avoid committing to /etc/group
   * the name of a non-existent user */
  groups_list = oobs_groups_config_get_groups (OOBS_GROUPS_CONFIG (oobs_groups_config_get ()));

  valid = oobs_list_get_iter_first (groups_list, &list_iter);

  while (valid) {
    group = OOBS_GROUP (oobs_list_get (groups_list, &list_iter));

    oobs_group_remove_user (group, user);

    g_object_unref (group);

    valid = oobs_list_iter_next (priv->users_list, &list_iter);
  }


  /* Then remove user from the list */
  valid = oobs_list_get_iter_first (priv->users_list, &list_iter);

  while (valid) {
    list_user = OOBS_USER (oobs_list_get (priv->users_list, &list_iter));

    if (list_user == user) {
      g_object_unref (list_user);
      break;
    }

    g_object_unref (list_user);

    valid = oobs_list_iter_next (priv->users_list, &list_iter);
  }

  oobs_list_remove (priv->users_list, &list_iter);

  return OOBS_RESULT_OK;
}

/**
 * oobs_users_config_get_minimum_users_uid:
 * @config: An #OobsUsersConfig.
 * 
 * Returns the default minimum UID for non-system users.
 * 
 * Return Value: minimum UID for non-system users.
 **/
uid_t
oobs_users_config_get_minimum_users_uid (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, OOBS_MAX_UID);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), OOBS_MAX_UID);

  priv = config->_priv;

  return priv->minimum_uid;
}

/**
 * oobs_users_config_set_minimum_users_uid:
 * @config: An #OobsUsersConfig.
 * @uid: new minimum UID for non-system users.
 * 
 * Sets the minimum UID for non-system users.
 **/
void
oobs_users_config_set_minimum_users_uid (OobsUsersConfig *config, uid_t uid)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (OOBS_IS_USERS_CONFIG (config));

  g_object_set (G_OBJECT (config), "minimum-uid", uid, NULL);
}

/**
 * oobs_users_config_get_maximum_users_uid:
 * @config: An #OobsUsersConfig.
 * 
 * Returns the default maximum UID for non-system users.
 * 
 * Return Value: maximum UID for non-system users.
 **/
uid_t
oobs_users_config_get_maximum_users_uid (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, OOBS_MAX_UID);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), OOBS_MAX_UID);

  priv = config->_priv;

  return priv->maximum_uid;
}

/**
 * oobs_users_config_set_maximum_users_uid:
 * @config: An #OobsUsersConfig.
 * @uid: a new maximum UID for non-system users.
 * 
 * Sets the maximum UID for non-system users.
 **/
void
oobs_users_config_set_maximum_users_uid (OobsUsersConfig *config, uid_t uid)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (OOBS_IS_USERS_CONFIG (config));

  g_object_set (G_OBJECT (config), "maximum-uid", uid, NULL);
}

/**
 * oobs_users_config_get_default_shell:
 * @config: An #OobsUsersConfig
 * 
 * Returns the default shell that will be used for new users.
 * 
 * Return Value: A pointer to the default shell as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_users_config_get_default_shell (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->default_shell;
}

/**
 * oobs_users_config_set_default_shell:
 * @config: An #OobsUsersConfig
 * @shell: new default shell for new users.
 * 
 * Sets a new default shell for new users, replacing the old one.
 **/
void
oobs_users_config_set_default_shell (OobsUsersConfig *config, const gchar *shell)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (OOBS_IS_USERS_CONFIG (config));

  g_object_set (G_OBJECT (config), "default-shell", shell, NULL);
}

/**
 * oobs_users_config_get_default_home_dir:
 * @config: An #OobsUsersConfig.
 * 
 * Returns the default home directory prefix for new users. when new users
 * are created a directory with the user login name is created in that prefix.
 * 
 * Return Value: A pointer to the default home directory prefix as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_users_config_get_default_home_dir (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->default_home;
}

/**
 * oobs_users_config_set_default_home_dir:
 * @config: An #OobsUsersConfig.
 * @home_dir: new default home directory prefix.
 * 
 * Sets a new home directory prefix used for newly created users, replacing the old one.
 **/
void
oobs_users_config_set_default_home_dir (OobsUsersConfig *config, const gchar *home_dir)
{
  g_return_if_fail (config != NULL);
  g_return_if_fail (OOBS_IS_USERS_CONFIG (config));

  g_object_set (G_OBJECT (config), "default-home", home_dir, NULL);
}

/**
 * oobs_users_config_get_default_group:
 * @config: An #OobsUsersConfig.
 * 
 * Returns an #OobsGroup defining the default group used for new users.
 * 
 * Return Value: An #OobsGroup, you must not unref this object.
 **/
OobsGroup*
oobs_users_config_get_default_group (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);

  oobs_object_ensure_update (oobs_groups_config_get ());
  priv = config->_priv;

  return priv->default_group;
}

/**
 * oobs_users_config_get_available_shells:
 * @config: An #OobsUsersConfig
 * 
 * Returns a #GList containing strings with paths to the available shells.
 * 
 * Return Value: a #GList containing strings, you must not free
 * neither this list or its elements.
 **/
GList*
oobs_users_config_get_available_shells (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->shells;
}

/**
 * oobs_users_config_get_encrypted_home_support:
 * @config: An #OobsUsersConfig
 *
 * Returns whether encrypted home directories are supported by the platform
 * (e.g. using eCryptfs).
 *
 * Return Value: %TRUE if encrypted home dirs are supported, %FALSE otherwise.
 **/
gboolean
oobs_users_config_get_encrypted_home_support (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, FALSE);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), FALSE);

  priv = config->_priv;

  return priv->encrypted_home;
}

/*
 * Convenience functions to make working with the users list easier.
 */

/**
 * oobs_users_config_get_from_login:
 * @config: An #OobsUsersConfig.
 * @login: the login name of the wanted user.
 *
 * Gets the (first) user whose login is @login. This is a convenience function
 * to avoid walking manually over the users list.
 *
 * Return value: an #OobsUser corresponding to the passed login,
 * or %NULL if no such user exists. Don't forget to unref user when you're done.
 **/
OobsUser *
oobs_users_config_get_from_login (OobsUsersConfig *config, const gchar *login)
{
  OobsUsersConfigPrivate *priv;
  OobsUser *user;
  OobsListIter iter;
  gboolean valid;
  const gchar *user_login;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);
  g_return_val_if_fail (login != NULL, NULL);

  priv = config->_priv;

  valid = oobs_list_get_iter_first (priv->users_list, &iter);

  while (valid) {
    user = OOBS_USER (oobs_list_get (priv->users_list, &iter));
    user_login = oobs_user_get_login_name (user);

    if (user_login && strcmp (login, user_login) == 0)
      return user;

    /* only the returned user is not unreferenced here */
    g_object_unref (user);

    valid = oobs_list_iter_next (priv->users_list, &iter);
  }

  return NULL;
}

/**
 * oobs_users_config_get_from_uid:
 * @config: An #OobsUsersConfig.
 * @uid: the UID of the wanted user.
 *
 * Gets the (first) user whose UID is @uid. This is a convenience function
 * to avoid walking manually over the users list.
 *
 * Return value: an #OobsUser corresponding to the passed UID,
 * or %NULL if no such user exists. Don't forget to unref user when you're done.
 **/
OobsUser *
oobs_users_config_get_from_uid (OobsUsersConfig *config, uid_t uid)
{
  OobsUsersConfigPrivate *priv;
  OobsUser *user;
  OobsListIter iter;
  gboolean valid;
  uid_t user_uid;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), NULL);

  priv = config->_priv;

  valid = oobs_list_get_iter_first (priv->users_list, &iter);

  while (valid) {
    user = OOBS_USER (oobs_list_get (priv->users_list, &iter));
    user_uid = oobs_user_get_uid (user);

    if (user_uid == uid)
      return user;

    /* only the returned user is not unreferenced here */
    g_object_unref (user);

    valid = oobs_list_iter_next (priv->users_list, &iter);
  }

  return NULL;
}

/**
 * oobs_users_config_is_login_used:
 * @config: An #OobsUsersConfig.
 * @login: the login name to check.
 *
 * Check whether @login is already used by an existing user or not. This is
 * a convenience function to avoid walking manually over the users list.
 *
 * Return value: %TRUE if an user named @login already exists, %FALSE otherwise.
 **/
gboolean
oobs_users_config_is_login_used (OobsUsersConfig *config, const gchar *login)
{
  OobsUser *user;
  gboolean login_used;

  user = oobs_users_config_get_from_login (config, login);
  login_used = (user != NULL);

  if (user)
    g_object_unref (user);

  return login_used;
}

/**
 * oobs_users_config_is_uid_used:
 * @config: An #OobsUsersConfig.
 * @uid: the UID to check.
 *
 * Check whether @uid is already used by an existing user or not. This is
 * a convenience function to avoid walking manually over the users list.
 *
 * Return value: %TRUE if an user with such an UID already exists, %FALSE otherwise.
 **/
gboolean
oobs_users_config_is_uid_used (OobsUsersConfig *config, uid_t uid)
{
  OobsUser *user;
  gboolean uid_used;

  user = oobs_users_config_get_from_uid (config, uid);
  uid_used = (user != NULL);

  if (user)
    g_object_unref (user);

  return uid_used;
}

/**
 * oobs_users_config_find_free_uid:
 * @config: An #OobsUsersConfig.
 * @uid_min: the minimum wanted UID.
 * @uid_max: the maximum wanted UID.
 *
 * Finds an UID that is not used by any user in the list. The returned UID is
 * the highest used UID in the range plus one if @uid_max is not used.
 * Else, the first free UID in the range is returned.
 *
 * If both @uid_min and @uid_max are equal to 0, the default range is used.
 *
 * Return value: a free UID in the requested range,
 * or @uid_max to indicate wrong use or failure to find a free UID.
 **/
uid_t
oobs_users_config_find_free_uid (OobsUsersConfig *config, uid_t uid_min, uid_t uid_max)
{
  OobsUsersConfigPrivate *priv;
  OobsList *list;
  OobsListIter list_iter;
  OobsUser *user;
  gboolean valid;
  uid_t new_uid, user_uid;

  g_return_val_if_fail (config != NULL, uid_max);
  g_return_val_if_fail (OOBS_IS_USERS_CONFIG (config), uid_max);
  g_return_val_if_fail (uid_min <= uid_max, uid_max);

  priv = config->_priv;

  if (uid_min == 0 && uid_max == 0) {
    uid_min = priv->minimum_uid;
    uid_max = priv->maximum_uid;
  }

  new_uid = uid_min - 1;

  list = oobs_users_config_get_users (config);
  valid = oobs_list_get_iter_first (list, &list_iter);

  /* Find the highest used UID in the range */
  while (valid) {
    user = OOBS_USER (oobs_list_get (list, &list_iter));
    user_uid = oobs_user_get_uid (user);
    g_object_unref (user);

    if (user_uid < uid_max && user_uid >= uid_min && user_uid > new_uid)
      new_uid = user_uid;

    valid = oobs_list_iter_next (list, &list_iter);
  }

  new_uid++;

  if (!oobs_users_config_is_uid_used (config, new_uid))
    return new_uid;


  /* If the fast method failed, iterate over the whole range */
  new_uid = uid_min;
  while (oobs_users_config_is_uid_used (config, new_uid) && new_uid < uid_max)
    new_uid++;

  /* In the extreme case where no UID is free in the range,
   * we return the uid_max, which is the best we can do */
  return new_uid;
}
