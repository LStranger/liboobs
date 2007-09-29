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
#include "oobs-groupsconfig.h"
#include "oobs-usersconfig.h"
#include "oobs-group.h"
#include "oobs-defines.h"
#include "utils.h"

#define GROUPS_CONFIG_REMOTE_OBJECT "GroupsConfig"
#define OOBS_GROUPS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_GROUPS_CONFIG, OobsGroupsConfigPrivate))

typedef struct _OobsGroupsConfigPrivate OobsGroupsConfigPrivate;

struct _OobsGroupsConfigPrivate
{
  OobsList *groups_list;

  gid_t     minimum_gid;
  gid_t     maximum_gid;
  guint     id;
};

static void oobs_groups_config_class_init (OobsGroupsConfigClass *class);
static void oobs_groups_config_init       (OobsGroupsConfig      *config);
static void oobs_groups_config_finalize   (GObject               *object);

static void oobs_groups_config_set_property (GObject      *object,
					     guint         prop_id,
					     const GValue *value,
					     GParamSpec   *pspec);
static void oobs_groups_config_get_property (GObject      *object,
					     guint         prop_id,
					     GValue       *value,
					     GParamSpec   *pspec);

static void oobs_groups_config_update     (OobsObject   *object);
static void oobs_groups_config_commit     (OobsObject   *object);

enum {
  PROP_0,
  PROP_MINIMUM_GID,
  PROP_MAXIMUM_GID
};

G_DEFINE_TYPE (OobsGroupsConfig, oobs_groups_config, OOBS_TYPE_OBJECT);


static void
oobs_groups_config_class_init (OobsGroupsConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->set_property = oobs_groups_config_set_property;
  object_class->get_property = oobs_groups_config_get_property;
  object_class->finalize    = oobs_groups_config_finalize;

  oobs_object_class->commit = oobs_groups_config_commit;
  oobs_object_class->update = oobs_groups_config_update;

  g_object_class_install_property (object_class,
				   PROP_MINIMUM_GID,
				   g_param_spec_int ("minimum-gid",
						     "Minimum GID",
						     "Minimum GID for non-system groups",
						     0, OOBS_MAX_GID, OOBS_MAX_GID,
						     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_MAXIMUM_GID,
				   g_param_spec_int ("maximum-gid",
						     "Maximum GID",
						     "Maximum GID for non-system groups",
						     0, OOBS_MAX_GID, OOBS_MAX_GID,
						     G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsGroupsConfigPrivate));
}

static void
oobs_groups_config_init (OobsGroupsConfig *config)
{
  OobsGroupsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUPS_CONFIG (config));

  priv = OOBS_GROUPS_CONFIG_GET_PRIVATE (config);

  config->_priv = priv;
  priv->groups_list = _oobs_list_new (OOBS_TYPE_GROUP);
}

static void
oobs_groups_config_finalize (GObject *object)
{
  OobsGroupsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUPS_CONFIG (object));

  priv = OOBS_GROUPS_CONFIG (object)->_priv;

  if (priv && priv->groups_list)
    g_object_unref (priv->groups_list);

  if (G_OBJECT_CLASS (oobs_groups_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_groups_config_parent_class)->finalize) (object);
}

static void
oobs_groups_config_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  OobsGroupsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUPS_CONFIG (object));

  priv = OOBS_GROUPS_CONFIG (object)->_priv;

  switch (prop_id)
    {
    case PROP_MINIMUM_GID:
      priv->minimum_gid = g_value_get_int (value);
      break;
    case PROP_MAXIMUM_GID:
      priv->maximum_gid = g_value_get_int (value);
      break;
    }
}

static void
oobs_groups_config_get_property (GObject      *object,
				 guint         prop_id,
				 GValue       *value,
				 GParamSpec   *pspec)
{
  OobsGroupsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUPS_CONFIG (object));

  priv = OOBS_GROUPS_CONFIG (object)->_priv;

  switch (prop_id)
    {
    case PROP_MINIMUM_GID:
      g_value_set_int (value, priv->minimum_gid);
      break;
    case PROP_MAXIMUM_GID:
      g_value_set_int (value, priv->maximum_gid);
      break;
    }
}

static OobsGroup*
create_group_from_dbus_reply (OobsObject      *object,
			      DBusMessage     *reply,
			      DBusMessageIter  struct_iter,
			      GHashTable      *hashtable,
			      guint           *max_id)
{
  DBusMessageIter iter;
  int      gid;
  guint    id;
  const gchar *groupname, *passwd;
  GList   *users;
  OobsGroup *group;

  dbus_message_iter_recurse (&struct_iter, &iter);

  id = utils_get_uint (&iter);
  groupname = utils_get_string (&iter);
  passwd = utils_get_string (&iter);
  gid = utils_get_int (&iter);

  users = utils_get_string_list_from_dbus_reply (reply, &iter);

  group = g_object_new (OOBS_TYPE_GROUP,
			"name", groupname,
			"crypted-password", passwd,
			"gid", gid,
			NULL);

  /* set the id by hand */
  group->id = id;
  *max_id = MAX (id, *max_id);

  /* put the users list in the hashtable, when the groups list has
   * been completely generated, we may query the users config safely */
  g_hash_table_insert (hashtable,
		       g_object_ref (group),
		       users);

  return OOBS_GROUP (group);
}

static GList*
get_users_list (OobsGroup *group)
{
  GList *users, *elem, *usernames = NULL;
  OobsUser *user;

  users = elem = oobs_group_get_users (group);

  while (elem)
    {
      user = elem->data;
      usernames = g_list_prepend (usernames, (gpointer) oobs_user_get_login_name (user));

      elem = elem->next;
    }

  g_list_free (users);
  return usernames;
}

static void
create_dbus_struct_from_group (GObject         *group,
			       DBusMessage     *message,
			       DBusMessageIter *array_iter)
{
  DBusMessageIter struct_iter;
  int    gid;
  gchar *groupname, *passwd;
  GList *users;

  g_object_get (group,
		"name", &groupname,
		"crypted-password", &passwd,
		"gid",  &gid,
		NULL);

  users = get_users_list (OOBS_GROUP (group));

  dbus_message_iter_open_container (array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

  utils_append_uint (&struct_iter, OOBS_GROUP (group)->id);
  utils_append_string (&struct_iter, groupname);
  utils_append_string (&struct_iter, passwd);
  utils_append_int (&struct_iter, gid);

  utils_create_dbus_array_from_string_list (users, message, &struct_iter);

  dbus_message_iter_close_container (array_iter, &struct_iter);

  g_list_free (users);
  g_free (groupname);
  g_free (passwd);
}

static OobsUser*
find_user (OobsList *users_list, gchar *username)
{
  OobsListIter iter;
  OobsUser *user;
  gboolean valid;

  valid = oobs_list_get_iter_first (users_list, &iter);

  while (valid)
    {
      user = OOBS_USER (oobs_list_get (users_list, &iter));

      if (strcmp (username, oobs_user_get_login_name (user)) == 0)
	return user;

      valid = oobs_list_iter_next (users_list, &iter);
      g_object_unref (user);
    }

  return NULL;
}

static void
query_users_foreach (OobsGroup *group,
		     GList     *users,
		     gpointer   data)
{
  OobsUsersConfig *users_config = OOBS_USERS_CONFIG (data);
  OobsList *users_list = oobs_users_config_get_users (users_config);
  OobsUser *user;

  while (users)
    {
      user = find_user (users_list, users->data);

      if (user)
	{
	  oobs_group_add_user (group, user);
	  g_object_unref (user);
	}

      users = users->next;
    }
}

static void
query_users (OobsGroupsConfig *groups_config,
	      GHashTable      *hashtable)
{
  OobsObject *users_config;
  OobsSession *session;

  g_object_get (G_OBJECT (groups_config),
		"session", &session,
		NULL);

  users_config = oobs_users_config_get (session);
  g_hash_table_foreach (hashtable, (GHFunc) query_users_foreach, users_config);
  g_object_unref (session);
}

static void
free_users_foreach (OobsGroup *group,
		    GList     *users,
		    gpointer   data)
{
  g_list_foreach (users, (GFunc) g_free, NULL);
}

static void
oobs_groups_config_update (OobsObject *object)
{
  OobsGroupsConfigPrivate *priv;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;
  OobsListIter     list_iter;
  GObject         *group;
  GHashTable      *hashtable;
  guint            id;

  priv  = OOBS_GROUPS_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);
  hashtable = g_hash_table_new_full (NULL, NULL,
				     (GDestroyNotify) g_object_unref,
				     (GDestroyNotify) g_list_free);
  id = 0;

  /* First of all, free the previous list */
  oobs_list_clear (priv->groups_list);

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      group = G_OBJECT (create_group_from_dbus_reply (object, reply,
						      elem_iter, hashtable, &id));

      oobs_list_append (priv->groups_list, &list_iter);
      oobs_list_set    (priv->groups_list, &list_iter, G_OBJECT (group));
      g_object_unref   (group);

      dbus_message_iter_next (&elem_iter);
    }

  priv->id = id;

  dbus_message_iter_next (&iter);

  priv->minimum_gid = utils_get_int (&iter);
  priv->maximum_gid = utils_get_int (&iter);

  /* last of all, query the groups now that the list is generated */
  query_users (OOBS_GROUPS_CONFIG (object), hashtable);
  g_hash_table_foreach (hashtable, (GHFunc) free_users_foreach, NULL);
  g_hash_table_unref (hashtable);
}

static void
oobs_groups_config_commit (OobsObject *object)
{
  OobsGroupsConfigPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter, array_iter;
  OobsListIter list_iter;
  GObject *group;
  gboolean valid;

  priv = OOBS_GROUPS_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);
  dbus_message_iter_open_container (&iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_UINT32_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_ARRAY_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING,
				    &array_iter);

  valid  = oobs_list_get_iter_first (priv->groups_list, &list_iter);

  while (valid)
    {
      group = oobs_list_get (priv->groups_list, &list_iter);
      create_dbus_struct_from_group (group, message, &array_iter);

      g_object_unref (group);
      valid = oobs_list_iter_next (priv->groups_list, &list_iter);
    }

  dbus_message_iter_close_container (&iter, &array_iter);
}

/**
 * oobs_groups_config_get:
 * @session: An #OobsSession.
 * 
 * Returns the #OobsGroupsConfig singleton, which
 * represents the groups configuration.
 * 
 * Return Value: the singleton #OobsGoupsConfig
 **/
OobsObject*
oobs_groups_config_get (OobsSession *session)
{
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  return g_object_new (OOBS_TYPE_GROUPS_CONFIG,
		       "remote-object", GROUPS_CONFIG_REMOTE_OBJECT,
		       "session",       session,
		       NULL);
}

/**
 * oobs_groups_config_get_groups:
 * @config: An #OobsGroupsConfig.
 * 
 * Returns an #OobsList containing objects of type #OobsGroup.
 * 
 * Return Value: An OobsList containing the groups configuration.
 **/
OobsList*
oobs_groups_config_get_groups (OobsGroupsConfig *config)
{
  OobsGroupsConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_GROUPS_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->groups_list;
}

guint
_oobs_groups_config_get_id (OobsGroupsConfig *config)
{
  OobsGroupsConfigPrivate *priv;

  priv = config->_priv;

  /* FIXME: this could overflow */
  return ++priv->id;
}
