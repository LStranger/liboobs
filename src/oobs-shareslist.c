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
#include "oobs-shareslist.h"
#include "oobs-share.h"
#include "oobs-share-smb.h"
#include "oobs-share-nfs.h"

#define SHARESLIST_REMOTE_OBJECT "SharesList"

static void oobs_shares_list_class_init (OobsSharesListClass *class);
static void oobs_shares_list_init       (OobsSharesList      *shares_list);
static void oobs_shares_list_finalize   (GObject             *object);

static void oobs_shares_list_update     (OobsObject   *object,
					 gpointer     data);
static void oobs_shares_list_commit     (OobsObject   *object,
					 gpointer     data);
static GType oobs_shares_list_get_content_type (OobsList *list);

G_DEFINE_TYPE (OobsSharesList, oobs_shares_list, OOBS_TYPE_SHARES_LIST);

static void
oobs_shares_list_class_init (OobsSharesListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);
  OobsListClass *oobs_list_class = OOBS_LIST_CLASS (class);

  oobs_shares_list_parent_class = g_type_class_peek_parent (class);

  object_class->finalize    = oobs_shares_list_finalize;
  oobs_object_class->commit = oobs_shares_list_commit;
  oobs_object_class->update = oobs_shares_list_update;

  oobs_list_class->get_content_type = oobs_shares_list_get_content_type;
}

static void
oobs_shares_list_init (OobsSharesList *object)
{
  g_return_if_fail (OOBS_IS_SHARES_LIST (object));
}

static void
oobs_shares_list_finalize (GObject *object)
{
  OobsSharesList *shares_list;

  g_return_if_fail (OOBS_IS_SHARES_LIST (object));

  shares_list = OOBS_SHARES_LIST (object);

  if (G_OBJECT_CLASS (oobs_shares_list_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_shares_list_parent_class)->finalize) (object);
}

static GType
oobs_shares_list_get_content_type (OobsList *list)
{
  return OOBS_TYPE_SHARE;
}

static OobsShare*
create_share_from_dbus_reply (OobsObject      *object,
			      DBusMessage     *reply,
			      DBusMessageIter  iter)
{
  DBusMessageIter  dict_iter, entries_iter;
  gchar           *key, *val;
  gchar           *name, *comment, *path;

  name    = NULL;
  comment = NULL;
  path    = NULL;
  dbus_message_iter_recurse (&iter, &dict_iter);

  while (dbus_message_iter_get_arg_type (&dict_iter) == DBUS_TYPE_DICT_ENTRY)
    {
      /* get into the dict entries */
      dbus_message_iter_recurse (&dict_iter, &entries_iter);

      dbus_message_iter_get_basic (&entries_iter, &key);
      dbus_message_iter_next (&entries_iter);
      dbus_message_iter_get_basic (&entries_iter, &val);
      dbus_message_iter_next (&entries_iter);

      if (strcmp (key, "point") == 0)
	path = val;
      else if (strcmp (key, "name") == 0)
	name = val;

      dbus_message_iter_next (&dict_iter);
    }

  return oobs_share_smb_new (name, "bar", path, 0);
}

static void
oobs_shares_list_update (OobsObject *object, gpointer data)
{
  DBusMessage      *reply = (DBusMessage *) data;
  DBusMessageIter   iter, array_iter;
  OobsList         *list;
  OobsListIter      list_iter;
  OobsShare        *share;

  list = OOBS_LIST (object);

  /* First of all, free the previous shares list */
  oobs_list_clear (list);

  /* start recursing through the response array */
  dbus_message_iter_init    (reply, &iter);
  dbus_message_iter_recurse (&iter, &array_iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_ARRAY)
    {
      share = create_share_from_dbus_reply (object, reply, array_iter);

      oobs_list_append (list, &list_iter);
      oobs_list_set    (list, &list_iter, G_OBJECT (share));
      g_object_unref   (share);

      dbus_message_iter_next (&array_iter);
    }
}

static void
oobs_shares_list_commit (OobsObject *object, gpointer data)
{
}

OobsObject*
oobs_shares_list_new (OobsSession *session)
{
  OobsObject *object;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  object = g_object_new (OOBS_TYPE_SHARES_LIST,
			 "remote-object", SHARESLIST_REMOTE_OBJECT,
			 "session",       session,
			 NULL);

  oobs_object_update (object);
  return object;
}
