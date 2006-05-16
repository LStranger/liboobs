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

#ifndef __OOBS_IFACE_H__
#define __OOBS_IFACE_H__

G_BEGIN_DECLS

#include "oobs-object.h"

#define OOBS_TYPE_IFACE         (oobs_iface_get_type())
#define OOBS_IFACE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_IFACE, OobsIface))
#define OOBS_IFACE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_IFACE, OobsIfaceClass))
#define OOBS_IS_IFACE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_IFACE))
#define OOBS_IS_IFACE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_IFACE))
#define OOBS_IFACE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_IFACE, OobsIfaceClass))

typedef struct _OobsIface        OobsIface;
typedef struct _OobsIfaceClass   OobsIfaceClass;
	
struct _OobsIface {
  GObject parent;
};

struct _OobsIfaceClass {
  GObjectClass parent_class;

  gboolean (*is_configured) (OobsIface*);
  gboolean (*has_gateway) (OobsIface*);
};

GType oobs_iface_get_type (void);


gboolean oobs_iface_get_auto (OobsIface *iface);
void     oobs_iface_set_auto (OobsIface *iface, gboolean is_auto);

gboolean oobs_iface_get_active (OobsIface *iface);
void     oobs_iface_set_active (OobsIface *iface, gboolean is_active);

G_CONST_RETURN gchar* oobs_iface_get_device_name (OobsIface *iface);
G_CONST_RETURN gchar* oobs_iface_get_hwaddr (OobsIface *iface);

gboolean     oobs_iface_get_configured (OobsIface *iface);
void         oobs_iface_set_configured (OobsIface *iface, gboolean is_configured);

gboolean     oobs_iface_has_gateway (OobsIface *iface);

G_END_DECLS

#endif /* __OOBS_IFACE_H__ */
