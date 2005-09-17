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
#include "oobs-list.h"
#include "oobs-list-private.h"
#include "oobs-nfsconfig.h"
#include "oobs-share.h"
#include "oobs-share-nfs.h"

#define NFS_CONFIG_REMOTE_OBJECT "NFSConfig"
#define OOBS_NFS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_NFS_CONFIG, OobsNFSConfigPrivate))

typedef struct _OobsNFSConfigPrivate OobsNFSConfigPrivate;

struct _OobsNFSConfigPrivate
{
  OobsList *shares_list;
};

static void oobs_nfs_config_class_init (OobsNFSConfigClass *class);
static void oobs_nfs_config_init       (OobsNFSConfig      *config);
static void oobs_nfs_config_finalize   (GObject            *object);

static void oobs_nfs_config_update     (OobsObject   *object,
					gpointer     data);
static void oobs_nfs_config_commit     (OobsObject   *object,
					gpointer     data);


G_DEFINE_TYPE (OobsNFSConfig, oobs_nfs_config, OOBS_TYPE_OBJECT);

static void
oobs_nfs_config_class_init (OobsNFSConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize    = oobs_nfs_config_finalize;
  oobs_object_class->commit = oobs_nfs_config_commit;
  oobs_object_class->update = oobs_nfs_config_update;
}

static void
oobs_nfs_config_init (OobsNFSConfig *config)
{
  OobsNFSConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_NFS_CONFIG (config));

  priv = OOBS_NFS_CONFIG_GET_PRIVATE (config);

  priv->shares_list = _oobs_list_new (OOBS_TYPE_SHARE_NFS);
}

static void
oobs_nfs_config_finalize (GObject *object)
{
  OobsNFSConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_NFS_CONFIG (object));

  priv = OOBS_NFS_CONFIG_GET_PRIVATE (object);

  if (priv && priv->shares_list)
    g_object_unref (priv->shares_list);

  if (G_OBJECT_CLASS (oobs_nfs_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_nfs_config_parent_class)->finalize) (object);
}

static OobsShare*
create_share_from_dbus_reply (OobsObject      *object,
			      DBusMessage     *reply,
			      DBusMessageIter  iter)
{
  /*
  DBusMessageIter dict_iter, entries_iter;
  gchar *key, *val, *name, *comment, *path;

  name    = NULL;
  comment = NULL;
  path    = NULL;
  dbus_message_iter_recurse (&iter, &dict_iter);

  while (dbus_message_iter_get_arg_type (&dict_iter) == DBUS_TYPE_DICT_ENTRY)
    {
      /* get into the dict entries */
  /*
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
  */
}

static void
oobs_nfs_config_update (OobsObject *object, gpointer data)
{
  OobsNFSConfigPrivate *priv;
  DBusMessage      *reply = (DBusMessage *) data;
  DBusMessageIter   iter, array_iter;
  OobsListIter      list_iter;
  OobsShare        *share;

  priv = OOBS_NFS_CONFIG_GET_PRIVATE (object);

  /* First of all, free the previous shares list */
  oobs_list_clear (priv->shares_list);

  /* start recursing through the response array */
  dbus_message_iter_init    (reply, &iter);
  dbus_message_iter_recurse (&iter, &array_iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_ARRAY)
    {
      share = create_share_from_dbus_reply (object, reply, array_iter);

      oobs_list_append (priv->shares_list, &list_iter);
      oobs_list_set    (priv->shares_list, &list_iter, G_OBJECT (share));
      g_object_unref   (share);

      dbus_message_iter_next (&array_iter);
    }
}

static void
oobs_nfs_config_commit (OobsObject *object, gpointer data)
{
}

OobsObject*
oobs_nfs_config_new (OobsSession *session)
{
  OobsObject *object;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  object = g_object_new (OOBS_TYPE_NFS_CONFIG,
			 "remote-object", NFS_CONFIG_REMOTE_OBJECT,
			 "session",       session,
			 NULL);

  oobs_object_update (object);
  return object;
}
