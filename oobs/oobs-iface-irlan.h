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

#ifndef __OOBS_IFACE_IRLAN_H__
#define __OOBS_IFACE_IRLAN_H__

G_BEGIN_DECLS

#include <glib-object.h>
#include "oobs-iface-ethernet.h"

#define OOBS_TYPE_IFACE_IRLAN         (oobs_iface_irlan_get_type())
#define OOBS_IFACE_IRLAN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_IFACE_IRLAN, OobsIfaceIRLan))
#define OOBS_IFACE_IRLAN_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_IFACE_IRLAN, OobsIfaceIRLanClass))
#define OOBS_IS_IFACE_IRLAN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_IFACE_IRLAN))
#define OOBS_IS_IFACE_IRLAN_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_IFACE_IRLAN))
#define OOBS_IFACE_IRLAN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_IFACE_IRLAN, OobsIfaceIRLanEthernetClass))

typedef struct _OobsIfaceIRLan        OobsIfaceIRLan;
typedef struct _OobsIfaceIRLanClass   OobsIfaceIRLanClass;
	
struct _OobsIfaceIRLan {
  OobsIfaceEthernet parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsIfaceIRLanClass {
  OobsIfaceEthernetClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
};

GType oobs_iface_irlan_get_type (void);


G_END_DECLS

#endif /* __OOBS_IFACE_IRLAN_H__ */
