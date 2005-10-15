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

#ifndef __OOBS_STATIC_HOST_H__
#define __OOBS_STATIC_HOST_H__

G_BEGIN_DECLS

#include "oobs-object.h"

#define OOBS_TYPE_STATIC_HOST         (oobs_static_host_get_type())
#define OOBS_STATIC_HOST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_STATIC_HOST, OobsStaticHost))
#define OOBS_STATIC_HOST_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_STATIC_HOST, OobsStaticHostClass))
#define OOBS_IS_STATIC_HOST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_STATIC_HOST))
#define OOBS_IS_STATIC_HOST_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_STATIC_HOST))
#define OOBS_STATIC_HOST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_STATIC_HOST, OobsStaticHostClass))

typedef struct _OobsStaticHost        OobsStaticHost;
typedef struct _OobsStaticHostClass   OobsStaticHostClass;
	
struct _OobsStaticHost {
  GObject parent;
};

struct _OobsStaticHostClass {
  GObjectClass parent_class;
};

GType oobs_static_host_get_type (void);

OobsStaticHost* oobs_static_host_new (const gchar *ip_address, GList *aliases);

G_CONST_RETURN gchar* oobs_static_host_get_ip_address (OobsStaticHost *static_host);
void                  oobs_static_host_set_ip_address (OobsStaticHost *static_host, const gchar *ip_address);

GList*                oobs_static_host_get_aliases (OobsStaticHost *static_host);
void                  oobs_static_host_set_aliases (OobsStaticHost *static_host, GList *aliases);


G_END_DECLS

#endif /* __OOBS_STATIC_HOST_H__ */
