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
#include "oobs-share.h"
#include "oobs-share-nfs.h"

#define OOBS_SHARE_NFS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SHARE_NFS, OobsShareNFSPrivate))

typedef struct _OobsShareNFSPrivate OobsShareNFSPrivate;

struct _OobsShareNFSPrivate {
  GSList *acl;
};

static void oobs_share_nfs_class_init (OobsShareNFSClass *class);
static void oobs_share_nfs_init       (OobsShareNFS      *share);
static void oobs_share_nfs_finalize   (GObject          *object);

G_DEFINE_TYPE (OobsShareNFS, oobs_share_nfs, OOBS_TYPE_SHARE);

static void
oobs_share_nfs_class_init (OobsShareNFSClass *class)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = NULL;
  object_class->get_property = NULL;
  object_class->finalize     = oobs_share_nfs_finalize;

  g_type_class_add_private (object_class,
			    sizeof (OobsShareNFSPrivate));
}

static void
oobs_share_nfs_init (OobsShareNFS *share)
{
  OobsShareNFSPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_NFS (share));

  priv = OOBS_SHARE_NFS_GET_PRIVATE (share);
  priv->acl = NULL;
  share->_priv = priv;
}

static void
acl_element_free (OobsShareAclElement *element, gpointer data)
{
  g_free (element->element);
  g_free (element);
}

static void
oobs_share_nfs_finalize (GObject *object)
{
  OobsShareNFS        *share;
  OobsShareNFSPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_NFS (object));

  share = OOBS_SHARE_NFS (object);
  priv  = share->_priv;

  if (priv)
    {
      g_slist_foreach (priv->acl, (GFunc) acl_element_free, NULL);
      g_slist_free (priv->acl);
    }

  if (G_OBJECT_CLASS (oobs_share_nfs_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_share_nfs_parent_class)->finalize) (object);
}

/**
 * oobs_share_nfs_new:
 * @path: share path.
 * 
 * Returns a new NFS share for the given path.
 * 
 * Return Value: A newly allocated #OobsShareNFS.
 **/
OobsShare*
oobs_share_nfs_new (const gchar *path)
{
  return g_object_new (OOBS_TYPE_SHARE_NFS,
		       "path", path,
		       NULL);
}

/**
 * oobs_share_nfs_add_acl_element:
 * @share: An #OobsShareNFS.
 * @element: Host in the share ACL.
 * @read_only: Whether the share is read only for the element.
 * 
 * Adds an ACL entry for a host, @element may be a host name,
 * an IP address or a combination in the form "IP address/Network mask".
 **/
void
oobs_share_nfs_add_acl_element (OobsShareNFS *share,
			       const gchar *element,
			       gboolean     read_only)
{
  OobsShareNFSPrivate *priv;
  OobsShareAclElement *elem;

  g_return_if_fail (share != NULL);
  g_return_if_fail (OOBS_IS_SHARE_NFS (share));
  priv = share->_priv;

  elem = g_new0 (OobsShareAclElement, 1);
  elem->element = g_strdup (element);
  elem->read_only = read_only;

  priv->acl = g_slist_append (priv->acl, elem);
}

/**
 * oobs_share_nfs_set_acl:
 * @share: An #OobsShareNFS.
 * @acl: A #GList of #OobsShareAclElement.
 * 
 * Overwrites the list of ACL entries for the share. The previous list and its
 * contents will be free, so any merging will have to be done by hand.
 * Alternatively, you can use oobs_share_nfs_add_acl_element().
 **/
void
oobs_share_nfs_set_acl (OobsShareNFS *share, GSList *acl)
{
  OobsShareNFSPrivate *priv;

  g_return_if_fail (share != NULL);
  g_return_if_fail (OOBS_IS_SHARE_NFS (share));

  priv = share->_priv;

  g_slist_foreach (priv->acl, (GFunc) acl_element_free, NULL);
  g_slist_free (priv->acl);

  priv->acl = acl;
}

/**
 * oobs_share_nfs_get_acl:
 * @share: An #OobsShareNFS.
 * 
 * Returns the ACL defined for this share.
 * 
 * Return Value: A #GList containing #OobsShareAclElement structs,
 *               this list must be freed with g_list_free().
 **/
GSList*
oobs_share_nfs_get_acl (OobsShareNFS *share)
{
  OobsShareNFSPrivate *priv;

  g_return_val_if_fail (share != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SHARE_NFS (share), NULL);
  priv = share->_priv;

  return g_slist_copy (priv->acl);
}
