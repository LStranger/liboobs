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

#define OOBS_SHARE_NFS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SHARE_NFS, OobsShareNfsPrivate))

typedef struct _OobsShareNfsPrivate OobsShareNfsPrivate;

struct _OobsShareNfsPrivate {
  GSList *acl;
};

static void oobs_share_nfs_class_init (OobsShareNfsClass *class);
static void oobs_share_nfs_init       (OobsShareNfs      *share);
static void oobs_share_nfs_finalize   (GObject          *object);

G_DEFINE_TYPE (OobsShareNfs, oobs_share_nfs, OOBS_TYPE_SHARE);

static void
oobs_share_nfs_class_init (OobsShareNfsClass *class)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = NULL;
  object_class->get_property = NULL;
  object_class->finalize     = oobs_share_nfs_finalize;

  g_type_class_add_private (object_class,
			    sizeof (OobsShareNfsPrivate));
}

static void
oobs_share_nfs_init (OobsShareNfs *share)
{
  OobsShareNfsPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_NFS (share));

  priv = OOBS_SHARE_NFS_GET_PRIVATE (share);
  priv->acl = NULL;
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
  OobsShareNfs        *share;
  OobsShareNfsPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_NFS (object));

  share = OOBS_SHARE_NFS (object);
  priv  = OOBS_SHARE_NFS_GET_PRIVATE (share);

  if (priv)
    {
      g_slist_foreach (priv->acl, (GFunc) acl_element_free, NULL);
      g_slist_free (priv->acl);
    }

  if (G_OBJECT_CLASS (oobs_share_nfs_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_share_nfs_parent_class)->finalize) (object);
}

OobsShare*
oobs_share_nfs_new (const gchar *path)
{
  return g_object_new (OOBS_TYPE_SHARE_NFS,
		       "path", path,
		       NULL);
}

void
oobs_share_nfs_add_acl_element (OobsShareNfs *share,
			       const gchar *element,
			       gboolean     read_only)
{
  OobsShareNfsPrivate *priv;
  OobsShareAclElement *elem;

  g_return_if_fail (share != NULL);
  g_return_if_fail (OOBS_IS_SHARE_NFS (share));
  priv = OOBS_SHARE_NFS_GET_PRIVATE (share);

  elem = g_new0 (OobsShareAclElement, 1);
  elem->element = g_strdup (element);
  elem->read_only = read_only;

  priv->acl = g_slist_append (priv->acl, elem);
}

void
oobs_share_nfs_set_acl (OobsShareNfs *share, GSList *acl)
{
  OobsShareNfsPrivate *priv;

  g_return_if_fail (share != NULL);
  g_return_if_fail (OOBS_IS_SHARE_NFS (share));

  priv = OOBS_SHARE_NFS_GET_PRIVATE (share);

  g_slist_foreach (priv->acl, (GFunc) acl_element_free, NULL);
  g_slist_free (priv->acl);

  priv->acl = acl;
}

GSList*
oobs_share_nfs_get_acl (OobsShareNfs *share)
{
  OobsShareNfsPrivate *priv;

  g_return_val_if_fail (share != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SHARE_NFS (share), NULL);
  priv = OOBS_SHARE_NFS_GET_PRIVATE (share);

  return priv->acl;
}
