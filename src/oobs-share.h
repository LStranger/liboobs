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

#ifndef __OOBS_SHARE_H__
#define __OOBS_SHARE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define OOBS_TYPE_SHARE         (oobs_share_get_type())
#define OOBS_SHARE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_SHARE, OobsShare))
#define OOBS_SHARE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_SHARE, OobsShareClass))
#define OOBS_IS_SHARE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_SHARE))
#define OOBS_IS_SHARE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_SHARE))
#define OOBS_SHARE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_SHARE, OobsShareClass))

typedef struct _OobsShare        OobsShare;
typedef struct _OobsShareClass   OobsShareClass;

struct _OobsShare {
  GObject parent;
};

struct _OobsShareClass {
  GObjectClass parent_class;
};

GType         oobs_share_get_type    (void);

const gchar*  oobs_share_get_path    (OobsShare *share);
void          oobs_share_set_path    (OobsShare *share, const gchar *path);

G_END_DECLS

#endif /* __OOBS_SHARE_H__ */
