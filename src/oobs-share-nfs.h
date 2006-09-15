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

#ifndef __OOBS_SHARE_NFS_H__
#define __OOBS_SHARE_NFS_H__

#include "oobs-share.h"

G_BEGIN_DECLS

#define OOBS_TYPE_SHARE_NFS         (oobs_share_nfs_get_type())
#define OOBS_SHARE_NFS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_SHARE_NFS, OobsShareNFS))
#define OOBS_SHARE_NFS_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),  OOBS_TYPE_SHARE_NFS, OobsShareNFSClass))
#define OOBS_IS_SHARE_NFS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_SHARE_NFS))
#define OOBS_IS_SHARE_NFS_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),  OOBS_TYPE_SHARE_NFS))
#define OOBS_SHARE_NFS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_SHARE_NFS, OobsShareNFSClass))

typedef struct _OobsShareNFS        OobsShareNFS;
typedef struct _OobsShareNFSClass   OobsShareNFSClass;
typedef struct _OobsShareAclElement OobsShareAclElement;
	
struct _OobsShareNFS {
  OobsShare parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsShareNFSClass {
  OobsShareClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
};

struct _OobsShareAclElement {
  gchar    *element;
  gboolean  read_only;
};

GType oobs_share_nfs_get_type (void);

OobsShare*     oobs_share_nfs_new             (const gchar *path);
void           oobs_share_nfs_add_acl_element (OobsShareNFS *share, const gchar *element, gboolean read_only);
void           oobs_share_nfs_set_acl         (OobsShareNFS *share, GSList *acl);
GSList*        oobs_share_nfs_get_acl         (OobsShareNFS *share);

G_END_DECLS

#endif /* __SHARE_NFS_EXPORT_H__ */
