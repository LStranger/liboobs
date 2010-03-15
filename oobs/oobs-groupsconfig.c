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
#include "oobs-groupsconfig.h"
#include "oobs-usersconfig.h"
#include "oobs-group.h"
#include "oobs-group-private.h"
#include "oobs-defines.h"
#include "utils.h"

/**
 * SECTION:oobs-groupsconfig
 * @title: OobsGroupsConfig
 * @short_description: Object that represents groups configuration
 * @see_also: #OobsGroup
 **/

#define GROUPS_CONFIG_REMOTE_OBJECT "GroupsConfig2"
#define OOBS_GROUPS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_GROUPS_CONFIG, OobsGroupsConfigPrivate))

typedef struct _OobsGroupsConfigPrivate OobsGroupsConfigPrivate;

struct _OobsGroupsConfigPrivate
{
  OobsList *groups_list;

  gid_t     minimum_gid;
  gid_t     maximum_gid;
};

static void oobs_groups_config_class_init  (OobsGroupsConfigClass *class);
static void oobs_groups_config_init        (OobsGroupsConfig      *config);
static void oobs_groups_config_finalize    (GObject               *object);

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

  if (priv)
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

static void
oobs_groups_config_update (OobsObject *object)
{
  OobsGroupsConfigPrivate *priv;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;
  OobsListIter     list_iter;
  GObject         *group;

  priv  = OOBS_GROUPS_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous configuration */
  oobs_list_clear (priv->groups_list);

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      group = G_OBJECT (_oobs_group_create_from_dbus_reply (object, reply, elem_iter));

      oobs_list_append (priv->groups_list, &list_iter);
      oobs_list_set    (priv->groups_list, &list_iter, G_OBJECT (group));

      g_object_unref   (group);

      dbus_message_iter_next (&elem_iter);
    }

  dbus_message_iter_next (&iter);

  priv->minimum_gid = utils_get_uint (&iter);
  priv->maximum_gid = utils_get_uint (&iter);
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
  guint32 minimum_gid, maximum_gid;

  priv = OOBS_GROUPS_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);
  dbus_message_iter_open_container (&iter,
                                    DBUS_TYPE_ARRAY,
                                    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
                                    DBUS_TYPE_STRING_AS_STRING
                                    DBUS_TYPE_STRING_AS_STRING
                                    DBUS_TYPE_UINT32_AS_STRING
                                    DBUS_TYPE_ARRAY_AS_STRING
                                    DBUS_TYPE_STRING_AS_STRING
                                    DBUS_STRUCT_END_CHAR_AS_STRING,
                                    &array_iter);

  valid  = oobs_list_get_iter_first (priv->groups_list, &list_iter);

  while (valid)
    {
      group = oobs_list_get (priv->groups_list, &list_iter);
      _oobs_create_dbus_struct_from_group (OOBS_GROUP (group), message, &array_iter);

      g_object_unref (group);
      valid = oobs_list_iter_next (priv->groups_list, &list_iter);
    }

  dbus_message_iter_close_container (&iter, &array_iter);

  minimum_gid = priv->minimum_gid;
  maximum_gid = priv->maximum_gid;
  utils_append_uint (&iter, minimum_gid);
  utils_append_uint (&iter, maximum_gid);
}

/**
 * oobs_groups_config_get:
 * 
 * Returns the #OobsGroupsConfig singleton, which
 * represents the groups configuration.
 * 
 * Return Value: the singleton #OobsGoupsConfig
 **/
OobsObject*
oobs_groups_config_get (void)
{
  static OobsObject *the_object = NULL;

  if (!the_object)
    the_object = g_object_new (OOBS_TYPE_GROUPS_CONFIG,
                               "remote-object", GROUPS_CONFIG_REMOTE_OBJECT,
                               NULL);

  return the_object;
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

  g_return_val_if_fail (OOBS_IS_GROUPS_CONFIG (config), NULL);

  oobs_object_ensure_update (oobs_users_config_get ());
  priv = config->_priv;

  return priv->groups_list;
}

/**
 * oobs_groups_config_add_group:
 * @config: An #OobsGroupsConfig.
 * @group: An #Oobsgroup.
 *
 * Add a group to the configuration, immediately committing changes to the system.
 * On success, @group will be appended to the groups list.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_groups_config_add_group (OobsGroupsConfig *config, OobsGroup *group)
{
  OobsGroupsConfigPrivate *priv;
  OobsListIter list_iter;
  OobsResult result;

  g_return_val_if_fail (config != NULL, OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (group != NULL, OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (OOBS_IS_GROUPS_CONFIG (config), OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (OOBS_IS_GROUP (group), OOBS_RESULT_MALFORMED_DATA);

  result = oobs_object_add (OOBS_OBJECT (group));

  if (result != OOBS_RESULT_OK)
    return result;

  priv = config->_priv;

  oobs_list_append (priv->groups_list, &list_iter);
  oobs_list_set (priv->groups_list, &list_iter, G_OBJECT (group));

  return OOBS_RESULT_OK;
}

/**
 * oobs_groups_config_delete_group:
 * @config: An #OobsGroupsConfig.
 * @group: An #OobsGroup.
 *
 * Delete an group from the configuration, immediately committing changes to the system.
 * On success, @group will be removed from the groups list.
 *
 * Return value: an #OobsResult enum with the error code.
 **/
OobsResult
oobs_groups_config_delete_group (OobsGroupsConfig *config, OobsGroup *group)
{
  OobsGroupsConfigPrivate *priv;
  OobsGroup *list_group;
  OobsListIter list_iter;
  gboolean valid;
  OobsResult result;

  g_return_val_if_fail (config != NULL, OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (group != NULL, OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (OOBS_IS_GROUPS_CONFIG (config), OOBS_RESULT_MALFORMED_DATA);
  g_return_val_if_fail (OOBS_IS_GROUP (group), OOBS_RESULT_MALFORMED_DATA);

  result = oobs_object_delete (OOBS_OBJECT (group));

  if (result != OOBS_RESULT_OK)
    return result;

  priv = config->_priv;

  valid = oobs_list_get_iter_first (priv->groups_list, &list_iter);

  while (valid) {
    list_group = OOBS_GROUP (oobs_list_get (priv->groups_list, &list_iter));

    if (list_group == group)
      break;

    valid = oobs_list_iter_next (priv->groups_list, &list_iter);
  }

  oobs_list_remove (priv->groups_list, &list_iter);

  return OOBS_RESULT_OK;
}

/**
 * oobs_groups_config_get_from_name:
 * @config: An #OobsGroupsConfig.
 * @name: the name of the wanted group.
 *
 * Gets the (first) group called @name. This is a convenience function
 * to avoid walking manually over the groups list.
 *
 * Return value: an #OobsGroup corresponding to the passed named,
 * or %NULL if no such group exists. Don't forget to unref group when you're done.
 **/
OobsGroup *
oobs_groups_config_get_from_name (OobsGroupsConfig *config, const gchar *name)
{
  OobsList *groups_list;
  OobsListIter iter;
  OobsGroup *group;
  gboolean valid;
  const gchar *group_name;

  groups_list = oobs_groups_config_get_groups (config);
  valid = oobs_list_get_iter_first (groups_list, &iter);

  while (valid) {
    group = OOBS_GROUP (oobs_list_get (groups_list, &iter));
    group_name = oobs_group_get_name (group);

    if (group_name && strcmp (name, group_name) == 0)
      return group;

    /* only the returned group is not unreferenced here */
    g_object_unref (group);

    valid = oobs_list_iter_next (groups_list, &iter);
  }

  return NULL;
}

/**
 * oobs_groups_config_get_from_uid:
 * @config: An #OobsGroupsConfig.
 * @gid: the UID of the wanted group.
 *
 * Gets the (first) group whose GID is @gid. This is a convenience function
 * to avoid walking manually over the groups list.
 *
 * Return value: an #OobsGroup corresponding to the passed GID,
 * or %NULL if no such group exists. Don't forget to group user when you're done.
 **/
OobsGroup *
oobs_groups_config_get_from_gid (OobsGroupsConfig *config, gid_t gid)
{
  OobsGroupsConfigPrivate *priv;
  OobsGroup *group;
  OobsListIter iter;
  gboolean valid;
  gid_t group_gid;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_GROUPS_CONFIG (config), NULL);

  priv = config->_priv;

  valid = oobs_list_get_iter_first (priv->groups_list, &iter);

  while (valid) {
    group = OOBS_GROUP (oobs_list_get (priv->groups_list, &iter));
    group_gid = oobs_group_get_gid (group);

    if (group_gid == gid)
      return group;

    /* only the returned group is not unreferenced here */
    g_object_unref (group);

    valid = oobs_list_iter_next (priv->groups_list, &iter);
  }

  return NULL;
}

/**
 * oobs_groups_config_is_name_used:
 * @config: An #OobsGroupsConfig.
 * @name: the name to check.
 *
 * Check whether @name is already used by an existing group or not. This is
 * a convenience function to avoid walking manually over the groups list.
 *
 * Return value: %TRUE if a group called @name already exists, %FALSE otherwise.
 **/
gboolean
oobs_groups_config_is_name_used (OobsGroupsConfig *config, const gchar *name)
{
  OobsGroup *group;
  gboolean name_used;

  group = oobs_groups_config_get_from_name (config, name);
  name_used = (group != NULL);

  if (group)
    g_object_unref (group);

  return name_used;
}

/**
 * oobs_groups_config_is_gid_used:
 * @config: An #OobsGroupsConfig.
 * @gid: the gid to check.
 *
 * Check whether @gid is already used by an existing group or not. This is
 * a convenience function to avoid walking manually over the groups list.
 *
 * Return value: %TRUE if an group with such an gid already exists, %FALSE otherwise.
 **/
gboolean
oobs_groups_config_is_gid_used (OobsGroupsConfig *config, gid_t gid)
{
  OobsGroup *group;
  gboolean gid_used;

  group = oobs_groups_config_get_from_gid (config, gid);
  gid_used = (group != NULL);

  if (group)
    g_object_unref (group);

  return gid_used;
}


/**
 * oobs_groups_config_find_free_uid:
 * @config: An #OobsGroupsConfig.
 * @gid_min: the minimum wanted GID.
 * @gid_max: the maximum wanted GID.
 *
 * Finds a GID that is not used by any user in the list. The returned GID is
 * the highest used GID in the range plus one if @gid_max is not used.
 * Else, the first free GID in the range is returned.
 *
 * If both @gid_min and @gid_max are equal to 0, the default range is used.
 *
 * Return value: a free GID in the requested range,
 * or @gid_max to indicate wrong use or failure to find a free GID.
 **/
gid_t
oobs_groups_config_find_free_gid (OobsGroupsConfig *config, gid_t gid_min, gid_t gid_max)
{
  OobsGroupsConfigPrivate *priv;
  OobsList *list;
  OobsListIter list_iter;
  GObject *group;
  gboolean valid;
  gid_t new_gid, gid;

  g_return_val_if_fail (config != NULL, gid_max);
  g_return_val_if_fail (OOBS_IS_GROUPS_CONFIG (config), gid_max);
  g_return_val_if_fail (gid_min <= gid_max, gid_max);

  priv = config->_priv;

  if (gid_min == 0 && gid_max == 0) {
    gid_min = priv->minimum_gid;
    gid_max = priv->maximum_gid;
  }

  new_gid = gid_min - 1;

  list = oobs_groups_config_get_groups (config);
  valid = oobs_list_get_iter_first (list, &list_iter);

  /* Find the highest used GID in the range */
  while (valid) {
    group = oobs_list_get (list, &list_iter);
    gid = oobs_group_get_gid (OOBS_GROUP (group));
    g_object_unref (group);

    if (gid < gid_max && gid >= gid_min && new_gid < gid)
      new_gid = gid;

    valid = oobs_list_iter_next (list, &list_iter);
  }

  new_gid++;

  if (!oobs_groups_config_is_gid_used (config, new_gid))
    return new_gid;


  /* If the fast method failed, iterate over the whole range */
  new_gid = gid_min;
  while (oobs_groups_config_is_gid_used (config, new_gid) && new_gid < gid_max)
    new_gid++;

  /* In the extreme case where no GID is free in the range,
   * we return the gid_max, which is the best we can do */
  return new_gid;
}
