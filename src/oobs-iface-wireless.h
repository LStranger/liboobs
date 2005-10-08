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

#ifndef __OOBS_IFACE_WIRELESS_H__
#define __OOBS_IFACE_WIRELESS_H__

G_BEGIN_DECLS

#include "oobs-object.h"
#include "oobs-iface-ethernet.h"

typedef enum {
  OOBS_WIRELESS_KEY_ASCII,
  OOBS_WIRELESS_KEY_HEXADECIMAL
} OobsWirelessKeyType;

#define OOBS_TYPE_WIRELESS_KEY_TYPE      (oobs_wireless_key_type_get_type ())
#define OOBS_TYPE_IFACE_WIRELESS         (oobs_iface_wireless_get_type())
#define OOBS_IFACE_WIRELESS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_IFACE_WIRELESS, OobsIfaceWireless))
#define OOBS_IFACE_WIRELESS_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_IFACE_WIRELESS, OobsIfaceWirelessClass))
#define OOBS_IS_IFACE_WIRELESS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_IFACE_WIRELESS))
#define OOBS_IS_IFACE_WIRELESS_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_IFACE_WIRELESS))
#define OOBS_IFACE_WIRELESS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_IFACE_WIRELESS, OobsIfaceWirelessClass))

typedef struct _OobsIfaceWireless        OobsIfaceWireless;
typedef struct _OobsIfaceWirelessClass   OobsIfaceWirelessClass;
	
struct _OobsIfaceWireless {
  OobsIfaceEthernet parent;
};

struct _OobsIfaceWirelessClass {
  OobsIfaceEthernetClass parent_class;
};

GType oobs_wireless_key_type_get_type (void);
GType oobs_iface_wireless_get_type (void);

G_CONST_RETURN gchar* oobs_iface_wireless_get_essid (OobsIfaceWireless *iface);
void                  oobs_iface_wireless_set_essid (OobsIfaceWireless *iface, const gchar *essid);

G_CONST_RETURN gchar* oobs_iface_wireless_get_wep_key (OobsIfaceWireless *iface);
void                  oobs_iface_wireless_set_wep_key (OobsIfaceWireless *iface, const gchar *wep_key);

OobsWirelessKeyType   oobs_iface_wireless_get_wep_key_type (OobsIfaceWireless *iface);
void                  oobs_iface_wireless_set_wep_key_type (OobsIfaceWireless *iface, OobsWirelessKeyType key_type);


G_END_DECLS


#endif /* __OOBS_IFACE_WIRELESS_H__ */
