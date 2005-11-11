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
} OobsShareSMBFlags;

#define OOBS_TYPE_SHARE_SMB         (oobs_share_smb_get_type())
#define OOBS_TYPE_SHARE_SMB_FLAGS   (oobs_share_smb_flags_get_type())
#define OOBS_SHARE_SMB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_SHARE_SMB, OobsShareSMB))
#define OOBS_SHARE_SMB_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_SHARE_SMB, OobsShareSMBClass))
#define OOBS_IS_SHARE_SMB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_SHARE_SMB))
#define OOBS_IS_SHARE_SMB_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_SHARE_SMB))
#define OOBS_SHARE_SMB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_SHARE_SMB, OobsShareSMBClass))

typedef struct _OobsShareSMB        OobsShareSMB;
typedef struct _OobsShareSMBClass   OobsShareSMBClass;

struct _OobsShareSMB {
  OobsShare parent;
};

struct _OobsShareSMBClass {
  OobsShareClass parent_class;
};

GType             oobs_share_smb_get_type (void);
GType             oobs_share_smb_flags_get_type (void);

G_CONST_RETURN gchar* oobs_share_smb_get_name    (OobsShareSMB*);
void                  oobs_share_smb_set_name    (OobsShareSMB*, const gchar*);

G_CONST_RETURN gchar* oobs_share_smb_get_comment (OobsShareSMB*);
void                  oobs_share_smb_set_comment (OobsShareSMB*, const gchar*);

OobsShareSMBFlags oobs_share_smb_get_flags (OobsShareSMB*);
void              oobs_share_smb_set_flags (OobsShareSMB*, OobsShareSMBFlags);

OobsShare*        oobs_share_smb_new (const gchar *path, const gchar *name, const gchar *comment, OobsShareSMBFlags flags);


G_END_DECLS

#endif /* __OOBS_SHARE_SMB_H__ */
