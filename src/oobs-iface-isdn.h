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

#ifndef __OOBS_IFACE_ISDN_H
#define __OOBS_IFACE_ISDN_H

G_BEGIN_DECLS

#include "oobs-iface.h"

#define OOBS_TYPE_IFACE_ISDN           (oobs_iface_isdn_get_type ())
#define OOBS_IFACE_ISDN(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OOBS_TYPE_IFACE_ISDN, OobsIfaceISDN))
#define OOBS_IFACE_ISDN_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    OOBS_TYPE_IFACE_ISDN, OobsIfaceISDNClass))
#define OOBS_IS_IFACE_ISDN(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OOBS_TYPE_IFACE_ISDN))
#define OOBS_IS_IFACE_ISDN_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    OOBS_TYPE_IFACE_ISDN))
#define OOBS_IFACE_ISDN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  OOBS_TYPE_IFACE_ISDN, OobsIfaceISDNClass))

typedef struct _OobsIfaceISDN      OobsIfaceISDN;
typedef struct _OobsIfaceISDNClass OobsIfaceISDNClass;

struct _OobsIfaceISDN
{
  OobsIface parent;
};

struct _OobsIfaceISDNClass
{
  OobsIfaceClass parent_class;
};

GType oobs_iface_isdn_get_type ();


G_END_DECLS

#endif /* __OOBS_IFACE_ISDN_H */
