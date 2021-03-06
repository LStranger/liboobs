/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004-2005 Carlos Garnacho
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

#include <glib-object.h>
#include "oobs-iface-irlan.h"
#include "oobs-iface-ethernet.h"

/**
 * SECTION:oobs-iface-irlan
 * @title: OobsIfaceIRLan
 * @short_description: Object that represents an individual IRLan interface
 * @see_also: #OobsIface, #OobsIfacesConfig, #OobsIfaceEthernet,
 *     #OobsIfacePlip, #OobsIfacePPP, #OobsIfaceWireless
 **/

static void oobs_iface_irlan_class_init (OobsIfaceIRLanClass *class);
static void oobs_iface_irlan_init (OobsIfaceIRLan *iface);

G_DEFINE_TYPE (OobsIfaceIRLan, oobs_iface_irlan, OOBS_TYPE_IFACE_ETHERNET);

static void
oobs_iface_irlan_class_init (OobsIfaceIRLanClass *class)
{
  /* Inherits completely from OobsIfaceEthernet */
}

static void
oobs_iface_irlan_init (OobsIfaceIRLan *iface)
{
  /* Inherits completely from OobsIfaceEthernet */
}
