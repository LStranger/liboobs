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

#ifndef __OOBS_SHARE_SMB_H__
#define __OOBS_SHARE_SMB_H__

G_BEGIN_DECLS

#include <glib-object.h>
#include "oobs-share.h"

typedef enum {
  OOBS_SHARE_SMB_ENABLED   = 1 << 0,
  OOBS_SHARE_SMB_BROWSABLE = 1 << 1,
  OOBS_SHARE_SMB_PUBLIC    = 1 << 2,
  OOBS_SHARE_SMB_WRITABLE  = 1 << 3,
} OobsShareSmbFlags;

#define OOBS_TYPE_SHARE_SMB         (oobs_share_smb_get_type())
#define OOBS_TYPE_SHARE_SMB_FLAGS   (oobs_share_smb_flags_get_type())
#define OOBS_SHARE_SMB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_SHARE_SMB, OobsShareSmb))
#define OOBS_SHARE_SMB_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_SHARE_SMB, OobsShareSmbClass))
#define OOBS_IS_SHARE_SMB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_SHARE_SMB))
#define OOBS_IS_SHARE_SMB_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_SHARE_SMB))
#define OOBS_SHARE_SMB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_SHARE_SMB, OobsShareSmbClass))

typedef struct _OobsShareSmb        OobsShareSmb;
typedef struct _OobsShareSmbClass   OobsShareSmbClass;

struct _OobsShareSmb {
  OobsShare parent;
};

struct _OobsShareSmbClass {
  OobsShareClass parent_class;
};

GType             oobs_share_smb_get_type (void);
GType             oobs_share_smb_flags_get_type (void);

G_CONST_RETURN gchar* oobs_share_smb_get_name    (OobsShareSmb*);
void              oobs_share_smb_set_name    (OobsShareSmb*, const gchar*);

G_CONST_RETURN gchar* oobs_share_smb_get_comment (OobsShareSmb*);
void              oobs_share_smb_set_comment (OobsShareSmb*, const gchar*);

OobsShareSmbFlags oobs_share_smb_get_flags (OobsShareSmb*);
void              oobs_share_smb_set_flags (OobsShareSmb*, OobsShareSmbFlags);

OobsShare*        oobs_share_smb_new (const gchar*, const gchar*, const gchar*, OobsShareSmbFlags);

G_END_DECLS

#endif /* __SHARE_SMB_EXPORT_H__ */
