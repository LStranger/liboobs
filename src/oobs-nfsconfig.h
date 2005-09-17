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

#ifndef __OOBS_NFS_CONFIG_H
#define __OOBS_NFS_CONFIG_H

G_BEGIN_DECLS

#include <glib-object.h>
#include "oobs-object.h"
#include "oobs-list.h"
#include "oobs-session.h"
#include "oobs-share.h"

#define OOBS_TYPE_NFS_CONFIG         (oobs_nfs_config_get_type ())
#define OOBS_NFS_CONFIG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_NFS_CONFIG, OobsNFSConfig))
#define OOBS_NFS_CONFIG_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_NFS_CONFIG, OobsNFSConfigClass))
#define OOBS_IS_NFS_CONFIG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_NFS_CONFIG))
#define OOBS_IS_NFS_CONFIG_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_NFS_CONFIG))
#define OOBS_NFS_CONFIG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_NFS_CONFIG, OobsNFSConfigClass))

typedef struct _OobsNFSConfig      OobsNFSConfig;
typedef struct _OobsNFSConfigClass OobsNFSConfigClass;

struct _OobsNFSConfig
{
  OobsList parent;
};

struct _OobsNFSConfigClass
{
  OobsListClass parent_class;
};

GType       oobs_nfs_config_get_type     (void);

OobsObject* oobs_nfs_config_new          (OobsSession *session);

G_END_DECLS

#endif /* __OOBS_NFS_CONFIG_H */
