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

#include <string.h>
#include <glib-object.h>
#include <dbus/dbus.h>

#include "oobs-object-private.h"
#include "oobs-session.h"
#include "oobs-group.h"
#include "oobs-group-private.h"
#include "oobs-user.h"
#include "oobs-session.h"
#include "oobs-groupsconfig.h"
#include "oobs-usersconfig.h"
#include "oobs-defines.h"
#include "utils.h"

/**
 * SECTION:oobs-group
 * @title: OobsGroup
 * @short_description: Object that represents an individual group
 * @see_also: #OobsGroupsConfig
 **/

#define GROUP_REMOTE_OBJECT "GroupConfig2"
#define OOBS_GROUP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_GROUP, OobsGroupPrivate))

typedef struct _OobsGroupPrivate OobsGroupPrivate;

struct _OobsGroupPrivate {
  OobsObject *config;
  gint   key;
  gchar *groupname;
  gchar *password;
  gid_t  gid;

  /* List of names received from the backends, possibly with unknown users */
  GList *usernames;
  /* List of OobsUsers updated from the above, only containing known users,
   * and working has a cache to access the above from public API. */
  GList *users;
};

static void oobs_group_class_init  (OobsGroupClass *class);
static void oobs_group_init        (OobsGroup      *group);
static void oobs_group_constructed (GObject        *object);
static void oobs_group_finalize    (GObject        *object);

static void oobs_group_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec);
static void oobs_group_get_property (GObject      *object,
				     guint         prop_id,
				     GValue       *value,
				     GParamSpec   *pspec);

static void oobs_group_commit             (OobsObject *object);
static void oobs_group_update             (OobsObject *object);
static void oobs_group_get_update_message (OobsObject *object);

enum {
  PROP_0,
  PROP_GROUPNAME,
  PROP_PASSWORD,
  PROP_GID,
};

G_DEFINE_TYPE (OobsGroup, oobs_group, OOBS_TYPE_OBJECT);

static void
oobs_group_class_init (OobsGroupClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_class = OOBS_OBJECT_CLASS (class);

  object_class->set_property = oobs_group_set_property;
  object_class->get_property = oobs_group_get_property;
  object_class->constructed  = oobs_group_constructed;
  object_class->finalize     = oobs_group_finalize;

  oobs_class->commit = oobs_group_commit;
  oobs_class->update = oobs_group_update;
  oobs_class->get_update_message = oobs_group_get_update_message;

  /* override the singleton check */
  oobs_class->singleton = FALSE;

  g_object_class_install_property (object_class,
				   PROP_GROUPNAME,
				   g_param_spec_string ("name",
							"Groupname",
							"Name for the group",
							NULL,
							G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_PASSWORD,
				   g_param_spec_string ("password",
							"Password",
							"Password for the group",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_GID,
				   g_param_spec_uint ("gid",
				                      "GID",
				                      "Main group GID for the group",
				                      0, OOBS_MAX_GID, OOBS_MAX_GID,
				                      G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsGroupPrivate));
}

static void
oobs_group_init (OobsGroup *group)
{
  OobsGroupPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUP (group));

  priv = OOBS_GROUP_GET_PRIVATE (group);
  priv->config    = oobs_groups_config_get ();
  priv->groupname = NULL;
  priv->password  = NULL;
  priv->usernames = NULL;
  priv->users     = NULL;

  group->_priv = priv;
}

/*
 * Clear OobsUsers list and fill it with updated references.
 */
static void
oobs_group_users_updated (OobsGroup        *group,
                          OobsUsersConfig  *users_config)
{
  OobsGroupPrivate *priv;
  OobsUser *user;
  GList *l;

  priv = OOBS_GROUP_GET_PRIVATE (group);

  g_list_foreach (priv->users, (GFunc) g_object_unref, NULL);
  g_list_free (priv->users);
  priv->users = NULL;

  for (l = priv->usernames; l; l = l->next)
    {
      user = oobs_users_config_get_from_login (users_config, l->data);

      /* A reference has already been added by the above call,
        * keep it until user is removed from list or the group is destroyed. */
      if (user)
	  priv->users = g_list_prepend (priv->users, user);
    }
}

static void
oobs_group_constructed (GObject *object)
{
  /* stay tuned of changes in users config */
  g_signal_connect_swapped (oobs_users_config_get (), "updated",
                            G_CALLBACK (oobs_group_users_updated), object);
}

static void
oobs_group_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  OobsGroup *group;
  OobsGroupPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUP (object));

  group = OOBS_GROUP (object);
  priv = group->_priv;

  switch (prop_id)
    {
    case PROP_GROUPNAME:
      g_free (priv->groupname);
      priv->groupname = g_value_dup_string (value);
      break;
      break;
    case PROP_PASSWORD:
      g_free (priv->password);
      priv->password = g_value_dup_string (value);
      break;
    case PROP_GID:
      priv->gid = g_value_get_uint (value);
      break;
    }
}

static void
oobs_group_get_property (GObject      *object,
			 guint         prop_id,
			 GValue       *value,
			 GParamSpec   *pspec)
{
  OobsGroup *group;
  OobsGroupPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUP (object));

  group = OOBS_GROUP (object);
  priv = group->_priv;

  switch (prop_id)
    {
    case PROP_GROUPNAME:
      g_value_set_string (value, priv->groupname);
      break;
    case PROP_PASSWORD:
      g_value_set_string (value, priv->password);
      break;
    case PROP_GID:
      g_value_set_uint (value, priv->gid);
      break;
    }
}

static void
oobs_group_finalize (GObject *object)
{
  OobsGroup        *group;
  OobsGroupPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUP (object));

  group = OOBS_GROUP (object);
  priv = group->_priv;

  if (priv)
    {
      g_free (priv->groupname);

      g_list_foreach (priv->usernames, (GFunc) g_free, NULL);
      g_list_free (priv->usernames);

      g_list_foreach (priv->users, (GFunc) g_object_unref, NULL);
      g_list_free (priv->users);

      /* Erase password field in case it's not done */
      if (priv->password) {
	memset (priv->password, 0, strlen (priv->password));
	g_free (priv->password);
      }
    }

  if (G_OBJECT_CLASS (oobs_group_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_group_parent_class)->finalize) (object);
}

OobsGroup*
_oobs_group_create_from_dbus_reply (OobsObject      *object,
                                    DBusMessage     *reply,
                                    DBusMessageIter  struct_iter)
{
  DBusMessageIter iter;
  guint32 gid;
  const gchar *groupname, *passwd;
  OobsGroup *group;
  OobsGroupPrivate *priv;
  OobsObject *users_config;

  dbus_message_iter_recurse (&struct_iter, &iter);

  groupname = utils_get_string (&iter);
  passwd = utils_get_string (&iter);
  gid = utils_get_uint (&iter);

  group = oobs_group_new (groupname);
  g_object_set (G_OBJECT (group),
                "password", passwd,
                "gid", gid,
                NULL);

  /* This list is kept in this form rather than as OobsUsers* because
   * we don't want to remove unknown users from groups (users not in
   * /etc/passwd such as that from LDAP). */
  priv = OOBS_GROUP_GET_PRIVATE (group);
  priv->usernames = utils_get_string_list_from_dbus_reply (reply, &iter);

  /* just update users if the object was already
   * updated, update will be forced later if required
   */
  users_config = oobs_users_config_get ();
  if (oobs_object_has_updated (users_config))
    oobs_group_users_updated (group,
                              OOBS_USERS_CONFIG (users_config));

  return OOBS_GROUP (group);
}

void
_oobs_create_dbus_struct_from_group (OobsGroup       *group,
                                     DBusMessage     *message,
                                     DBusMessageIter *array_iter)
{
  OobsGroupPrivate *priv;
  DBusMessageIter struct_iter;
  guint32 gid;
  gchar *groupname, *passwd;

  priv = OOBS_GROUP_GET_PRIVATE (group);

  g_object_get (group,
		"name", &groupname,
		"password", &passwd,
		"gid",  &gid,
		NULL);

  dbus_message_iter_open_container (array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

  utils_append_string (&struct_iter, groupname);
  utils_append_string (&struct_iter, passwd);
  utils_append_uint (&struct_iter, gid);

  utils_create_dbus_array_from_string_list (priv->usernames, message, &struct_iter);

  dbus_message_iter_close_container (array_iter, &struct_iter);

  g_free (groupname);
  g_free (passwd);
}

static void
oobs_group_commit (OobsObject *object)
{
  OobsGroupPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter;

  message = _oobs_object_get_dbus_message (object);
  dbus_message_iter_init_append (message, &iter);
  _oobs_create_dbus_struct_from_group (OOBS_GROUP (object), message, &iter);

  /* Erase password field as soon as possible */
  priv = OOBS_GROUP_GET_PRIVATE (OOBS_GROUP (object));
  if (priv->password)
    memset (priv->password, 0, strlen (priv->password));
}

/*
 * We need a custom update message containing the group name.
 */
static void
oobs_group_get_update_message (OobsObject *object)
{
  OobsGroupPrivate *priv;
  DBusMessageIter iter;
  DBusMessage *message;

  priv = OOBS_GROUP (object)->_priv;

  message = _oobs_object_get_dbus_message (object);
  dbus_message_iter_init_append (message, &iter);

  utils_append_string (&iter, priv->groupname);
}

static void
oobs_group_update (OobsObject *object)
{
  DBusMessage *reply;
  DBusMessageIter iter;

  reply = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init (reply, &iter);
  _oobs_group_create_from_dbus_reply (object, reply, iter);
}

/**
 * oobs_group_new:
 * @name: group name.
 * 
 * Returns a newly allocated #OobsGroup with the name specified by @name.
 * 
 * Return Value: A new #OobsGroup.
 **/
OobsGroup*
oobs_group_new (const gchar *name)
{
  /* FIXME: should check name length */

  return g_object_new (OOBS_TYPE_GROUP,
		       "name", name,
                       "remote-object", GROUP_REMOTE_OBJECT,
		       NULL);
}

/**
 * oobs_group_get_name:
 * @group: An #OobsGroup.
 * 
 * Returns the name of the group represented by @group.
 * 
 * Return Value: A pointer to the group name as a string.
 *               This string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_group_get_name (OobsGroup *group)
{
  OobsGroupPrivate *priv;

  g_return_val_if_fail (group != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_GROUP (group), NULL);

  priv = group->_priv;

  return priv->groupname;
}

/**
 * oobs_group_set__password:
 * @group: An #OobsGroup.
 * @crypted_password: a new password for @group.
 * 
 * Sets clear text password for the group
 * defined by #OobsGroup, overwriting the previous one.
 **/
void
oobs_group_set_password (OobsGroup   *group,
			 const gchar *password)
{
  g_return_if_fail (group != NULL);
  g_return_if_fail (OOBS_IS_GROUP (group));

  g_object_set (G_OBJECT (group), "password", password, NULL);
}

/**
 * oobs_group_get_gid:
 * @group: An #OobsGroup.
 * 
 * Returns the group ID (GID) associated to #OobsGroup
 * 
 * Return Value: the #group GID.
 **/
gid_t
oobs_group_get_gid (OobsGroup *group)
{
  OobsGroupPrivate *priv;

  g_return_val_if_fail (group != NULL, OOBS_MAX_GID);
  g_return_val_if_fail (OOBS_IS_GROUP (group), OOBS_MAX_GID);

  priv = group->_priv;

  return priv->gid;
}

/**
 * oobs_group_set_gid:
 * @group: An #OobsGroup.
 * @gid: A new GID for #group.
 * 
 * Sets the group ID (GID) of #group to be #gid.
 **/
void
oobs_group_set_gid (OobsGroup *group, gid_t gid)
{
  g_return_if_fail (group != NULL);
  g_return_if_fail (OOBS_IS_GROUP (group));

  g_object_set (G_OBJECT (group), "gid", gid, NULL);
}

/**
 * oobs_group_get_users:
 * @group: An #OobsGroup.
 * 
 * Returns a #GList containing pointers to the #OobsUser objects
 * that represent the users represented by the group.
 * 
 * Return Value: a newly allocated #GList, use g_list_free() to free it.
 **/
GList*
oobs_group_get_users (OobsGroup *group)
{
  OobsGroupPrivate *priv;

  g_return_val_if_fail (OOBS_IS_GROUP (group), NULL);

  priv = OOBS_GROUP_GET_PRIVATE (group);

  return g_list_copy (priv->users);
}

/**
 * oobs_group_add_user:
 * @group: An #OobsGroup.
 * @user: An #OobsUser to add to the group.
 * 
 * Adds a new user to the group. If the user is
 * already in the group, it does nothing.
 **/
void
oobs_group_add_user (OobsGroup *group,
		     OobsUser  *user)
{
  OobsGroupPrivate *priv;
  const char *login;

  g_return_if_fail (OOBS_IS_GROUP (group));
  g_return_if_fail (OOBS_IS_USER (user));
  
  priv = OOBS_GROUP_GET_PRIVATE (group);

  login = oobs_user_get_login_name (user);

  /* Update usernames list and OobsUsers list. First is used to commit,
   * second is used for public API. */

  /* Try to avoid several occurrences */
  if (!g_list_find_custom (priv->usernames, login, (GCompareFunc) strcmp))
    priv->usernames = g_list_prepend (priv->usernames, g_strdup (login));

  if (!g_list_find (priv->users, user))
    priv->users = g_list_prepend (priv->users, g_object_ref (user));
}

/**
 * oobs_group_remove_user:
 * @group: An #OobsGroup.
 * @user: An #OobsUser to remove from the group.
 * 
 * Removes an user from the group. If the user isn't a
 * member of this group, this function does nothing.
 **/
void
oobs_group_remove_user (OobsGroup *group,
			OobsUser  *user)
{
  OobsGroupPrivate *priv;
  GList *l;
  const char *login;

  g_return_if_fail (OOBS_IS_GROUP (group));
  g_return_if_fail (OOBS_IS_USER (user));
  
  priv = OOBS_GROUP_GET_PRIVATE (group);

  login = oobs_user_get_login_name (user);

  /* Update usernames list and OobsUsers list. First is used to commit,
   * second is used for public API. */

  /* There might be several instances */
  while ((l = g_list_find_custom (priv->usernames, login, (GCompareFunc) strcmp)))
    {
      g_free (l->data);
      priv->usernames = g_list_delete_link (priv->usernames, l);
    }

  while ((l = g_list_find (priv->users, user)))
    {
      g_object_unref (user);
      priv->users = g_list_delete_link (priv->users, l);
    }
}

/**
 * oobs_group_is_root:
 * @group: An #OobsGroup.
 *
 * Checks whether a group is the root group, according to its name.
 *
 * Return value: %TRUE if @group is the root group, %FALSE otherwise.
 **/
gboolean
oobs_group_is_root (OobsGroup *group)
{
  const gchar *name;

  g_return_val_if_fail (OOBS_IS_GROUP (group), FALSE);

  name = oobs_group_get_name (group);

  if (!name)
    return FALSE;

  return (strcmp (name, "root") == 0);
}
