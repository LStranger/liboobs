/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2005 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
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
#include "oobs-list.h"
#include "oobs-groupslist.h"
#include "oobs-group.h"

#define GROUPSLIST_REMOTE_OBJECT "GroupsList"

static void oobs_groups_list_class_init (OobsGroupsListClass *class);
static void oobs_groups_list_init       (OobsGroupsList      *groups_list);
static void oobs_groups_list_finalize   (GObject            *object);

static void oobs_groups_list_update     (OobsObject   *object,
								 gpointer     data);
static void oobs_groups_list_commit     (OobsObject   *object,
								 gpointer     data);
static GType oobs_groups_list_get_content_type (OobsList *list);

G_DEFINE_TYPE (OobsGroupsList, oobs_groups_list, OOBS_TYPE_GROUPS_LIST);

static void
oobs_groups_list_class_init (OobsGroupsListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);
  OobsListClass *oobs_list_class = OOBS_LIST_CLASS (class);

  oobs_groups_list_parent_class = g_type_class_peek_parent (class);

  object_class->finalize    = oobs_groups_list_finalize;
  oobs_object_class->commit = oobs_groups_list_commit;
  oobs_object_class->update = oobs_groups_list_update;

  oobs_list_class->get_content_type = oobs_groups_list_get_content_type;
}

static void
oobs_groups_list_init (OobsGroupsList *object)
{
  g_return_if_fail (OOBS_IS_GROUPS_LIST (object));
}

static void
oobs_groups_list_finalize (GObject *object)
{
  OobsGroupsList *groups_list;

  g_return_if_fail (OOBS_IS_GROUPS_LIST (object));

  groups_list = OOBS_GROUPS_LIST (object);

  if (G_OBJECT_CLASS (oobs_groups_list_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_groups_list_parent_class)->finalize) (object);
}

static GType
oobs_groups_list_get_content_type (OobsList *list)
{
  return OOBS_TYPE_GROUP;
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
				   "groupname", groupname,
				   "password",  passwd,
				   "gid",       gid,
				   NULL);
}

static void
oobs_groups_list_update (OobsObject *object, gpointer data)
{
  DBusMessage     *reply = (DBusMessage *) data;
  DBusMessageIter  iter, elem_iter;
  OobsList        *list;
  OobsListIter     list_iter;
  GObject         *group;

  list = OOBS_LIST (object);

  /* First of all, free the previous list */
  oobs_list_clear (list);

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      group = G_OBJECT (create_group_from_dbus_reply (object, reply, elem_iter));

      oobs_list_append (list, &list_iter);
      oobs_list_set    (list, &list_iter, G_OBJECT (group));
      g_object_unref   (group);

      dbus_message_iter_next (&elem_iter);
    }
}

static void
oobs_groups_list_commit (OobsObject *object, gpointer data)
{
}

OobsObject*
oobs_groups_list_new (OobsSession *session)
{
  OobsObject *object;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  object = g_object_new (OOBS_TYPE_GROUPS_LIST,
					"remote-object", GROUPSLIST_REMOTE_OBJECT,
					"session",       session,
					NULL);
  oobs_object_update (object);
  return object;
}
