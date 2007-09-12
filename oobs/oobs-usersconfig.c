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
#include <string.h>

#include "oobs-object.h"
#include "oobs-object-private.h"
#include "oobs-list.h"
#include "oobs-list-private.h"
#include "oobs-usersconfig.h"
#include "oobs-user.h"
#include "oobs-defines.h"
#include "oobs-groupsconfig.h"
#include "oobs-group.h"
#include "utils.h"

#define USERS_CONFIG_REMOTE_OBJECT "UsersConfig"
#define OOBS_USERS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_USERS_CONFIG, OobsUsersConfigPrivate))

typedef struct _OobsUsersConfigPrivate OobsUsersConfigPrivate;

struct _OobsUsersConfigPrivate
{
  OobsList *users_list;

  GList    *shells;

  gboolean  use_md5;
  uid_t     minimum_uid;
  uid_t     maximum_uid;
  gchar    *default_shell;
  gchar    *default_home;
  guint     id;

  OobsGroup *default_group;
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
  PROP_DEFAULT_HOME,
  PROP_DEFAULT_GROUP
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
  g_object_class_install_property (object_class,
				   PROP_DEFAULT_GROUP,
				   g_param_spec_object ("default-group",
							"Default group",
							"Default group for new users",
							OOBS_TYPE_GROUP,
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

  priv = OOBS_USERS_CONFIG (object)->_priv;

  switch (prop_id)
    {
    case PROP_USE_MD5:
      g_value_set_boolean (value, priv->use_md5);
      break;
    case PROP_MINIMUM_UID:
      g_value_set_int (value, priv->minimum_uid);
      break;
    case PROP_MAXIMUM_UID:
      g_value_set_int (value, priv->maximum_uid);
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
			     DBusMessageIter  struct_iter,
			     GHashTable      *hashtable,
			     guint           *max_id)
{
  OobsUser *user;
  DBusMessageIter iter, gecos_iter;
  int    uid, gid;
  guint  id;
  gchar *login, *passwd, *home, *shell;
  gchar *name, *room_number, *work_phone, *home_phone, *other_data;

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

  /* GECOS fields */
  dbus_message_iter_recurse (&iter, &gecos_iter);

  dbus_message_iter_get_basic (&gecos_iter, &name);
  dbus_message_iter_next (&gecos_iter);

  dbus_message_iter_get_basic (&gecos_iter, &room_number);
  dbus_message_iter_next (&gecos_iter);

  dbus_message_iter_get_basic (&gecos_iter, &work_phone);
  dbus_message_iter_next (&gecos_iter);

  dbus_message_iter_get_basic (&gecos_iter, &home_phone);
  dbus_message_iter_next (&gecos_iter);

  dbus_message_iter_get_basic (&gecos_iter, &other_data);
  dbus_message_iter_next (&gecos_iter);
  /* end of GECOS fields */

  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &home);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &shell);
  dbus_message_iter_next (&iter);

  user = g_object_new (OOBS_TYPE_USER,
		       "name", login,
		       "crypted-password", passwd,
		       "uid", uid,
		       "home-directory", home,
		       "shell", shell,
		       "full-name", name,
		       "room-number", room_number,
		       "work-phone", work_phone,
		       "home-phone", home_phone,
		       "other-data", other_data,
		       NULL);

  /* set the id by hand */
  user->id = id;
  *max_id = MAX (id, *max_id);

  /* put the GID in the hashtable, when the users list has
   * been completely generated, we may query the groups safely */
  g_hash_table_insert (hashtable,
		       g_object_ref (user),
		       (gpointer) gid);
  return user;
}

static gboolean
create_dbus_struct_from_user (OobsUser        *user,
			      DBusMessage     *message,
			      DBusMessageIter *array_iter)
{
  OobsGroup *group;
  gint uid, gid;
  gchar *login, *password, *shell, *homedir;
  gchar *name, *room_number, *work_phone, *home_phone, *other_data;
  DBusMessageIter struct_iter, data_iter;

  g_object_get (user,
		"name", &login,
		"crypted-password", &password,
		"uid", &uid,
		"home-directory", &homedir,
		"shell", &shell,
		"full-name", &name,
		"room-number", &room_number,
		"work-phone", &work_phone,
		"home-phone", &home_phone,
		"other-data", &other_data,
		NULL);

  g_return_val_if_fail ((login && password && homedir && shell), FALSE);

  group = oobs_user_get_main_group (user);
  gid = oobs_group_get_gid (group);

  dbus_message_iter_open_container (array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_UINT32,  &user->id);
  utils_append_string (&struct_iter, login);
  utils_append_string (&struct_iter, password);
  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32,  &uid);
  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32,  &gid);
  
  dbus_message_iter_open_container (&struct_iter, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &data_iter);

  /* GECOS fields */
  utils_append_string (&data_iter, name);
  utils_append_string (&data_iter, room_number);
  utils_append_string (&data_iter, work_phone);
  utils_append_string (&data_iter, home_phone);
  utils_append_string (&data_iter, other_data);

  dbus_message_iter_close_container (&struct_iter, &data_iter);

  utils_append_string (&struct_iter, homedir);
  utils_append_string (&struct_iter, shell);
  dbus_message_iter_close_container (array_iter, &struct_iter);

  g_free (login);
  g_free (password);
  g_free (shell);
  g_free (homedir);
  g_free (name);
  g_free (room_number);
  g_free (work_phone);
  g_free (home_phone);
  g_free (other_data);

  return TRUE;
}

static OobsGroup*
get_group_with_gid (OobsGroupsConfig *config,
		    gint              gid)
{
  OobsList *groups_list;
  OobsListIter iter;
  OobsGroup *group;
  gboolean valid;

  groups_list = oobs_groups_config_get_groups (config);
  valid = oobs_list_get_iter_first (groups_list, &iter);

  while (valid)
    {
      group = OOBS_GROUP (oobs_list_get (groups_list, &iter));

      if (oobs_group_get_gid (group) == gid)
	return group;

      g_object_unref (group);
      valid = oobs_list_iter_next (groups_list, &iter);
    }

  return NULL;
}

static void
query_groups_foreach (OobsUser *user,
		      gint      gid,
		      gpointer  data)
{
  OobsGroupsConfig *groups_config = OOBS_GROUPS_CONFIG (data);
  OobsGroup *group;

  group = get_group_with_gid (groups_config, gid);
  oobs_user_set_main_group (user, group);

  if (group)
    g_object_unref (group);
}

static void
query_groups (OobsUsersConfig *users_config,
	      GHashTable      *hashtable,
	      gint             default_gid)
{
  OobsObject *groups_config;
  OobsSession *session;
  OobsGroup *group;

  g_object_get (G_OBJECT (users_config),
		"session", &session,
		NULL);

  groups_config = oobs_groups_config_get (session);
  g_hash_table_foreach (hashtable, (GHFunc) query_groups_foreach, groups_config);

  /* get the default group */
  if (default_gid > 0)
    {
      group = get_group_with_gid (OOBS_GROUPS_CONFIG (groups_config), default_gid);
      ((OobsUsersConfigPrivate *) users_config->_priv)->default_group = group;

      if (group)
	g_object_unref (group);
    }

  g_object_unref (session);
}

static void
oobs_users_config_update (OobsObject *object)
{
  OobsUsersConfigPrivate *priv;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;
  OobsListIter     list_iter;
  GObject         *user;
  gchar           *str;
  GHashTable      *hashtable;
  gint             default_gid;
  guint            id;

  priv  = OOBS_USERS_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);
  hashtable = g_hash_table_new_full (NULL, NULL, g_object_unref, NULL);
  id = 0;

  /* First of all, free the previous configuration */
  free_configuration (OOBS_USERS_CONFIG (object));

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      user = G_OBJECT (create_user_from_dbus_reply (object, reply,
						    elem_iter, hashtable, &id));

      oobs_list_append (priv->users_list, &list_iter);
      oobs_list_set    (priv->users_list, &list_iter, G_OBJECT (user));
      g_object_unref   (user);

      dbus_message_iter_next (&elem_iter);
    }

  priv->id = id;

  dbus_message_iter_next (&iter);
  priv->shells = utils_get_string_list_from_dbus_reply (reply, iter);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &priv->use_md5);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &priv->minimum_uid);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &priv->maximum_uid);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &str);
  priv->default_home = g_strdup (str);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &str);
  priv->default_shell = g_strdup (str);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &default_gid);

  /* last of all, query the groups now that the list is generated */
  query_groups (OOBS_USERS_CONFIG (object), hashtable, default_gid);
  g_hash_table_unref (hashtable);
}

static void
oobs_users_config_commit (OobsObject *object)
{
  OobsUsersConfigPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter, array_iter;
  OobsListIter list_iter;
  GObject *user;
  gboolean valid, correct;
  gint default_gid;

  priv = OOBS_USERS_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);
  dbus_message_iter_open_container (&iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_UINT32_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_ARRAY_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING,
				    &array_iter);
  valid = oobs_list_get_iter_first (priv->users_list, &list_iter);
  correct = TRUE;

  while (valid && correct)
    {
      user = oobs_list_get (priv->users_list, &list_iter);
      correct = create_dbus_struct_from_user (OOBS_USER (user), message, &array_iter);

      g_object_unref (user);
      valid = oobs_list_iter_next (priv->users_list, &list_iter);
    }

  dbus_message_iter_close_container (&iter, &array_iter);

  utils_create_dbus_array_from_string_list (priv->shells, message, &iter);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,  &priv->use_md5);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,  &priv->minimum_uid);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32,  &priv->maximum_uid);
  utils_append_string (&iter, priv->default_home);
  utils_append_string (&iter, priv->default_shell);

  default_gid = (priv->default_group) ? oobs_group_get_gid (priv->default_group) : -1;
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &default_gid);

  if (!correct)
    {
      /* malformed data, unset the message */
      _oobs_object_set_dbus_message (object, NULL);
    }
}

/**
 * oobs_users_config_get:
 * @session: An #OobsSession.
 * 
 * Returns the #OobsUsersConfig singleton, which represents
 * the system users and their configuration.
 * 
 * Return Value: the singleton #OobsUsersConfig object.
 **/
OobsObject*
oobs_users_config_get (OobsSession *session)
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

  priv = config->_priv;

  return priv->users_list;
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
 * @config: An #OobsIfacesConfig.
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

guint
_oobs_users_config_get_id (OobsUsersConfig *config)
{
  OobsUsersConfigPrivate *priv;

  priv = config->_priv;

  /* FIXME: this could overflow */
  return ++priv->id;
}