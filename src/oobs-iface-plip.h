/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
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

#ifndef __OOBS_IFACE_PLIP_H
#define __OOBS_IFACE_PLIP_H

G_BEGIN_DECLS

#include "oobs-iface.h"

#define OOBS_TYPE_IFACE_PLIP           (oobs_iface_plip_get_type ())
#define OOBS_IFACE_PLIP(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OOBS_TYPE_IFACE_PLIP, OobsIfacePlip))
#define OOBS_IFACE_PLIP_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    OOBS_TYPE_IFACE_PLIP, OobsIfacePlipClass))
#define OOBS_IS_IFACE_PLIP(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OOBS_TYPE_IFACE_PLIP))
#define OOBS_IS_IFACE_PLIP_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    OOBS_TYPE_IFACE_PLIP))
#define OOBS_IFACE_PLIP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  OOBS_TYPE_IFACE_PLIP, OobsIfacePlipClass))

typedef struct _OobsIfacePlip      OobsIfacePlip;
typedef struct _OobsIfacePlipClass OobsIfacePlipClass;

struct _OobsIfacePlip
{
  OobsIface parent;
};

struct _OobsIfacePlipClass
{
  OobsIfaceClass parent_class;
};

GType oobs_iface_plip_get_type (void);

G_CONST_RETURN gchar* oobs_iface_plip_get_address (OobsIfacePlip *iface);
void                  oobs_iface_plip_set_address (OobsIfacePlip *iface, const gchar *address);

G_CONST_RETURN gchar* oobs_iface_plip_get_remote_address (OobsIfacePlip *iface);
void                  oobs_iface_plip_set_remote_address (OobsIfacePlip *iface, const gchar *address);


G_END_DECLS


#endif /* __OOBS_IFACE_PLIP_H */
