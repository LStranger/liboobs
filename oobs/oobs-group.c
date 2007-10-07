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

#include <glib-object.h>
#include "oobs-session.h"
#include "oobs-group.h"
#include "oobs-user.h"
#include "oobs-session.h"
#include "oobs-usersconfig.h"
#include "oobs-groupsconfig-private.h"
#include "oobs-defines.h"
#include "utils.h"
#include <crypt.h>

#define OOBS_GROUP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_GROUP, OobsGroupPrivate))

typedef struct _OobsGroupPrivate OobsGroupPrivate;

struct _OobsGroupPrivate {
  OobsObject *config;
  gint   key;
  gchar *groupname;
  gchar *password;
  gid_t  gid;

  GList *users;

  gboolean use_md5;
};

static void oobs_group_class_init (OobsGroupClass *class);
static void oobs_group_init       (OobsGroup      *group);
static void oobs_group_finalize   (GObject       *object);

static void oobs_group_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec);
static void oobs_group_get_property (GObject      *object,
				     guint         prop_id,
				     GValue       *value,
				     GParamSpec   *pspec);
static GObject* oobs_group_constructor (GType                  type,
					guint                  n_construct_properties,
					GObjectConstructParam *construct_params);

enum {
  PROP_0,
  PROP_GROUPNAME,
  PROP_PASSWORD,
  PROP_CRYPTED_PASSWORD,
  PROP_GID,
};

G_DEFINE_TYPE (OobsGroup, oobs_group, G_TYPE_OBJECT);

static void
oobs_group_class_init (OobsGroupClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->constructor  = oobs_group_constructor;
  object_class->set_property = oobs_group_set_property;
  object_class->get_property = oobs_group_get_property;
  object_class->finalize     = oobs_group_finalize;

  g_object_class_install_property (object_class,
				   PROP_GROUPNAME,
				   g_param_spec_string ("name",
							"Groupname",
							"Name for the group",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_PASSWORD,
				   g_param_spec_string ("password",
							"Password",
							"Password for the group",
							NULL,
							G_PARAM_WRITABLE));
  g_object_class_install_property (object_class,
				   PROP_CRYPTED_PASSWORD,
				   g_param_spec_string ("crypted-password",
							"Crypted password",
							"Crypted password for the group",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_GID,
				   g_param_spec_int ("gid",
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
  OobsObject *users_config;

  g_return_if_fail (OOBS_IS_GROUP (group));

  priv = OOBS_GROUP_GET_PRIVATE (group);
  priv->config    = oobs_groups_config_get ();
  priv->groupname = NULL;
  priv->password  = NULL;
  priv->users     = NULL;

  users_config = oobs_users_config_get ();
  g_object_get (users_config, "use-md5", &priv->use_md5, NULL);
  group->_priv = priv;
}

static void
oobs_group_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  OobsGroup *group;
  OobsGroupPrivate *priv;
  gchar *salt, *str;

  g_return_if_fail (OOBS_IS_GROUP (object));

  group = OOBS_GROUP (object);
  priv = group->_priv;

  switch (prop_id)
    {
    case PROP_GROUPNAME:
      g_free (priv->groupname);
      priv->groupname = g_value_dup_string (value);
      break;
    case PROP_PASSWORD:
      g_free (priv->password);

      if (priv->use_md5)
	{
	  salt = utils_get_random_string (5);
	  str = g_strdup_printf ("$1$%s", salt);
	  priv->password = g_strdup (crypt (g_value_get_string (value), str));

	  g_free (str);
	}
      else
	{
	  salt = utils_get_random_string (2);
	  priv->password = g_strdup (crypt (g_value_get_string (value), salt));
	}

      g_free (salt);
      break;
    case PROP_CRYPTED_PASSWORD:
      g_free (priv->password);
      priv->password = g_value_dup_string (value);
      break;
    case PROP_GID:
      priv->gid = g_value_get_int (value);
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
    case PROP_CRYPTED_PASSWORD:
      g_value_set_string (value, priv->password);
      break;
    case PROP_GID:
      g_value_set_int (value, priv->gid);
      break;
    }
}

static void
oobs_group_finalize (GObject *object)
{
  OobsGroup        *group;
  OobsGroupPrivate *priv;

  g_print ("jijij\n");

  g_return_if_fail (OOBS_IS_GROUP (object));

  group = OOBS_GROUP (object);
  priv = group->_priv;

  if (priv)
    {
      g_free (priv->groupname);
      g_free (priv->password);

      g_list_foreach (priv->users, (GFunc) g_object_unref, NULL);
      g_list_free (priv->users);
    }

  if (G_OBJECT_CLASS (oobs_group_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_group_parent_class)->finalize) (object);
}

static GObject*
oobs_group_constructor (GType                  type,
			guint                  n_construct_properties,
			GObjectConstructParam *construct_params)
{
  GObject *object;
  OobsGroup *group;
  OobsGroupPrivate *priv;

  object = (* G_OBJECT_CLASS (oobs_group_parent_class)->constructor) (type,
								      n_construct_properties,
								      construct_params);
  group = OOBS_GROUP (object);
  priv = group->_priv;
  group->id = _oobs_groups_config_get_id (OOBS_GROUPS_CONFIG (priv->config));

  return object;

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
		       NULL);
}

/**
 * oobs_group_get_name:
 * @group: An #OobsGroup.
 * 
 * Returns the name of the group represented by #OobsGroup.
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
 * oobs_group_set_name:
 * @group: An #OobsGroup.
 * @name: A new name for #group.
 * 
 * Sets the name of #group to be #name,
 * overwriting the previous one.
 **/
void
oobs_group_set_name (OobsGroup *group, const gchar *name)
{
  g_return_if_fail (group != NULL);
  g_return_if_fail (OOBS_IS_GROUP (group));
  g_return_if_fail (name != NULL);

  /* FIXME: should check name length */

  g_object_set (G_OBJECT (group), "name", name, NULL);
}

/**
 * oobs_group_set_password:
 * @group: An #OobsGroup.
 * @password: A new password for #group.
 * 
 * Sets the group password for the group defined
 * by #OobsGroup, overwriting the previous one.
 **/
void
oobs_group_set_password (OobsGroup *group, const gchar *password)
{
  g_return_if_fail (group != NULL);
  g_return_if_fail (OOBS_IS_GROUP (group));

  g_object_set (G_OBJECT (group), "password", password, NULL);
}

/**
 * oobs_group_set_crypted_password:
 * @group: An #OobsGroup.
 * @crypted_password: a new crypted password for #group.
 * 
 * Sets an already crypted password for the group
 * defined by #OobsGroup, overwriting the previous one.
 **/
void
oobs_group_set_crypted_password (OobsGroup   *group,
				 const gchar *crypted_password)
{
  g_return_if_fail (group != NULL);
  g_return_if_fail (OOBS_IS_GROUP (group));

  g_object_set (G_OBJECT (group), "crypted-password", crypted_password, NULL);
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

  priv = group->_priv;
  return g_list_copy (priv->users);
}

void
oobs_group_clear_users (OobsGroup *group)
{
  OobsGroupPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUP (group));

  priv = group->_priv;

  g_list_foreach (priv->users, (GFunc) g_object_unref, NULL);
  g_list_free (priv->users);
  priv->users = NULL;
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

  g_return_if_fail (OOBS_IS_GROUP (group));
  g_return_if_fail (OOBS_IS_USER (user));
  
  priv = group->_priv;

  /* try to avoid several instances */
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

  g_return_if_fail (OOBS_IS_GROUP (group));
  g_return_if_fail (OOBS_IS_USER (user));
  
  priv = group->_priv;

  /* there might be several instances */
  priv->users = g_list_remove_all (priv->users, user);
}
