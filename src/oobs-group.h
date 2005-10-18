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

#ifndef __OOBS_GROUP_H__
#define __OOBS_GROUP_H__

G_BEGIN_DECLS

#include <sys/types.h>
#include "oobs-object.h"

#define OOBS_TYPE_GROUP         (oobs_group_get_type())
#define OOBS_GROUP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_GROUP, OobsGroup))
#define OOBS_GROUP_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_GROUP, OobsGroupClass))
#define OOBS_IS_GROUP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_GROUP))
#define OOBS_IS_GROUP_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_GROUP))
#define OOBS_GROUP_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_GROUP, OobsGroupClass))

typedef struct _OobsGroup        OobsGroup;
typedef struct _OobsGroupClass   OobsGroupClass;
	
struct _OobsGroup {
  GObject parent;
};

struct _OobsGroupClass {
  GObjectClass parent_class;
};

GType oobs_group_get_type (void);

OobsGroup* oobs_group_new (const gchar *name);

G_CONST_RETURN gchar* oobs_group_get_name (OobsGroup *group);
void       oobs_group_set_name (OobsGroup *group, const gchar *name);

void       oobs_group_set_password (OobsGroup *group, const gchar *password);
void       oobs_group_set_crypted_password (OobsGroup *group, const gchar *crypted_password);

gid_t      oobs_group_get_gid      (OobsGroup *group);
void       oobs_group_set_gid      (OobsGroup *group, gid_t gid);


G_END_DECLS

#endif /* __OOBS_GROUP_H__ */
