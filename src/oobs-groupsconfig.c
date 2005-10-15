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
#include "oobs-group.h"

#define GROUPS_CONFIG_REMOTE_OBJECT "GroupsConfig"
#define OOBS_GROUPS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_GROUPS_CONFIG, OobsGroupsConfigPrivate))

typedef struct _OobsGroupsConfigPrivate OobsGroupsConfigPrivate;

struct _OobsGroupsConfigPrivate
{
  OobsList *groups_list;
};

static void oobs_groups_config_class_init (OobsGroupsConfigClass *class);
static void oobs_groups_config_init       (OobsGroupsConfig      *config);
static void oobs_groups_config_finalize   (GObject               *object);

static void oobs_groups_config_update     (OobsObject   *object);
static void oobs_groups_config_commit     (OobsObject   *object);


G_DEFINE_TYPE (OobsGroupsConfig, oobs_groups_config, OOBS_TYPE_OBJECT);


static void
oobs_groups_config_class_init (OobsGroupsConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize    = oobs_groups_config_finalize;
  oobs_object_class->commit = oobs_groups_config_commit;
  oobs_object_class->update = oobs_groups_config_update;

  g_type_class_add_private (object_class,
			    sizeof (OobsGroupsConfigPrivate));
}

static void
oobs_groups_config_init (OobsGroupsConfig *config)
{
  OobsGroupsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUPS_CONFIG (config));

  priv = OOBS_GROUPS_CONFIG_GET_PRIVATE (config);

  priv->groups_list = _oobs_list_new (OOBS_TYPE_GROUP);
}

static void
oobs_groups_config_finalize (GObject *object)
{
  OobsGroupsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUPS_CONFIG (object));

  priv = OOBS_GROUPS_CONFIG_GET_PRIVATE (object);

  if (priv && priv->groups_list)
    g_object_unref (priv->groups_list);

  if (G_OBJECT_CLASS (oobs_groups_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_groups_config_parent_class)->finalize) (object);
}

static OobsGroup*
create_group_from_dbus_reply (OobsObject      *object,
			      DBusMessage     *reply,
			      DBusMessageIter  struct_iter)
{
  DBusMessageIter iter;
  int    id, gid;
  gchar *groupname, *passwd;

  dbus_message_iter_recurse (&struct_iter, &iter);

  dbus_message_iter_get_basic (&iter, &id);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &groupname);
  dbus_message_iter_next (&iter);
  
  dbus_message_iter_get_basic (&iter, &passwd);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &gid);
  dbus_message_iter_next (&iter);

  return g_object_new (OOBS_TYPE_GROUP,
		       "name", groupname,
		       "crypted-password", passwd,
		       "gid", gid,
		       NULL);
}

static void
oobs_groups_config_update (OobsObject *object)
{
  OobsGroupsConfigPrivate *priv;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;
  OobsListIter     list_iter;
  GObject         *group;

  priv  = OOBS_GROUPS_CONFIG_GET_PRIVATE (object);
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous list */
  oobs_list_clear (priv->groups_list);

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      group = G_OBJECT (create_group_from_dbus_reply (object, reply, elem_iter));

      oobs_list_append (priv->groups_list, &list_iter);
      oobs_list_set    (priv->groups_list, &list_iter, G_OBJECT (group));
      g_object_unref   (group);

      dbus_message_iter_next (&elem_iter);
    }
}

static void
oobs_groups_config_commit (OobsObject *object)
{
}

OobsObject*
oobs_groups_config_new (OobsSession *session)
{
  static OobsObject *object = NULL;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  if (!object)
    {
      object = g_object_new (OOBS_TYPE_GROUPS_CONFIG,
			     "remote-object", GROUPS_CONFIG_REMOTE_OBJECT,
			     "session",       session,
			     NULL);

      oobs_object_update (object);
    }

  return object;
}

OobsList*
oobs_groups_config_get_groups (OobsGroupsConfig *config)
{
  OobsGroupsConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_GROUPS_CONFIG (config), NULL);

  priv = OOBS_GROUPS_CONFIG_GET_PRIVATE (config);

  return priv->groups_list;
}
