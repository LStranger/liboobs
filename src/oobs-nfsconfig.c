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

static void oobs_nfs_config_update     (OobsObject   *object);
static void oobs_nfs_config_commit     (OobsObject   *object);


G_DEFINE_TYPE (OobsNFSConfig, oobs_nfs_config, OOBS_TYPE_OBJECT);

static void
oobs_nfs_config_class_init (OobsNFSConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize    = oobs_nfs_config_finalize;
  oobs_object_class->commit = oobs_nfs_config_commit;
  oobs_object_class->update = oobs_nfs_config_update;

  g_type_class_add_private (object_class,
			    sizeof (OobsNFSConfigPrivate));
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
			      DBusMessageIter  struct_iter)
{
  DBusMessageIter iter, array_iter, client_iter;
  OobsShare *share;
  gchar *path, *pattern;
  gboolean rw;

  dbus_message_iter_recurse (&struct_iter, &iter);

  dbus_message_iter_get_basic (&iter, &path);
  dbus_message_iter_next (&iter);

  share = oobs_share_nfs_new (path);
  dbus_message_iter_recurse (&iter, &array_iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRUCT)
    {
      dbus_message_iter_recurse (&array_iter, &client_iter);

      dbus_message_iter_get_basic (&client_iter, &pattern);
      dbus_message_iter_next (&client_iter);
      
      dbus_message_iter_get_basic (&client_iter, &rw);
      dbus_message_iter_next (&client_iter);

      oobs_share_nfs_add_acl_element (OOBS_SHARE_NFS (share), pattern, rw);
      dbus_message_iter_next (&array_iter);
    }

  return share;
}

static void
create_dbus_struct_from_share (GObject         *share,
			       DBusMessage     *message,
			       DBusMessageIter *array_iter)
{
  DBusMessageIter struct_iter, acl_iter, elem_iter;
  gchar *path;
  GSList *acl;
  OobsShareAclElement *acl_element;

  path = oobs_share_get_path (OOBS_SHARE (share));
  acl = oobs_share_nfs_get_acl (OOBS_SHARE_NFS (share));

  dbus_message_iter_open_container (array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);
  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_STRING, &path);

  dbus_message_iter_open_container (&struct_iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING,
				    &acl_iter);
  while (acl)
    {
      acl_element = (OobsShareAclElement*) acl->data;

      dbus_message_iter_open_container  (&acl_iter, DBUS_TYPE_STRUCT, NULL, &elem_iter);
      dbus_message_iter_append_basic    (&elem_iter, DBUS_TYPE_STRING, &acl_element->element);
      dbus_message_iter_append_basic    (&elem_iter, DBUS_TYPE_INT32,  &acl_element->read_only);
      dbus_message_iter_close_container (&acl_iter, &elem_iter);
      acl = acl->next;
    }

  dbus_message_iter_close_container (&struct_iter, &acl_iter);
  dbus_message_iter_close_container (array_iter, &struct_iter);
}

static void
oobs_nfs_config_update (OobsObject *object)
{
  OobsNFSConfigPrivate *priv;
  DBusMessage      *reply;
  DBusMessageIter   iter, array_iter;
  OobsListIter      list_iter;
  OobsShare        *share;

  priv  = OOBS_NFS_CONFIG_GET_PRIVATE (object);
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous shares list */
  oobs_list_clear (priv->shares_list);

  /* start recursing through the response array */
  dbus_message_iter_init    (reply, &iter);
  dbus_message_iter_recurse (&iter, &array_iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRUCT)
    {
      share = create_share_from_dbus_reply (object, reply, array_iter);

      oobs_list_append (priv->shares_list, &list_iter);
      oobs_list_set    (priv->shares_list, &list_iter, G_OBJECT (share));
      g_object_unref   (share);

      dbus_message_iter_next (&array_iter);
    }
}

static void
oobs_nfs_config_commit (OobsObject *object)
{
  OobsNFSConfigPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter, array_iter;
  OobsListIter list_iter;
  GObject *share;
  gboolean valid;

  priv = OOBS_NFS_CONFIG_GET_PRIVATE (object);
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);
  dbus_message_iter_open_container (&iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_ARRAY_AS_STRING
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING,
				    &array_iter);

  valid  = oobs_list_get_iter_first (priv->shares_list, &list_iter);

  while (valid)
    {
      share = oobs_list_get (priv->shares_list, &list_iter);
      create_dbus_struct_from_share (share, message, &array_iter);

      g_object_unref (share);
      valid = oobs_list_iter_next (priv->shares_list, &list_iter);
    }

  dbus_message_iter_close_container (&iter, &array_iter);
  _oobs_object_set_dbus_message (object, message);
}

/**
 * oobs_nfs_config_get:
 * @session: An #OobsSession.
 * 
 * Returns the #OobsNFSConfig singleton, which represents
 * the NFS system configuration.
 * 
 * Return Value: the singleton #OobsNFSConfig object.
 **/
OobsObject*
oobs_nfs_config_get (OobsSession *session)
{
  static OobsObject *object = NULL;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  if (!object)
    {
      object = g_object_new (OOBS_TYPE_NFS_CONFIG,
			     "remote-object", NFS_CONFIG_REMOTE_OBJECT,
			     "session",       session,
			     NULL);

      oobs_object_update (object);
    }

  return object;
}

/**
 * oobs_nfs_config_get_shares:
 * @config: An #OobsNFSConfig.
 * 
 * Returns an #OobsList containing objects of type #OobsShareNFS.
 * 
 * Return Value: an #OobsList containing the NFS shares information.
 **/
OobsList*
oobs_nfs_config_get_shares (OobsNFSConfig *config)
{
  OobsNFSConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_NFS_CONFIG (config), NULL);

  priv = OOBS_NFS_CONFIG_GET_PRIVATE (config);

  return priv->shares_list;
}
