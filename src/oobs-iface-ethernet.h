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

#ifndef __OOBS_IFACE_ETHERNET_H__
#define __OOBS_IFACE_ETHERNET_H__

G_BEGIN_DECLS

#include "oobs-object.h"

typedef enum {
  OOBS_METHOD_NONE,
  OOBS_METHOD_STATIC,
  OOBS_METHOD_DHCP
} OobsIfaceConfigurationMethod;

#define OOBS_TYPE_IFACE_CONFIGURATION_METHOD (oobs_iface_configuration_method_get_type ())

#define OOBS_TYPE_IFACE_ETHERNET         (oobs_iface_ethernet_get_type())
#define OOBS_IFACE_ETHERNET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_IFACE_ETHERNET, OobsIfaceEthernet))
#define OOBS_IFACE_ETHERNET_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_IFACE_ETHERNET, OobsIfaceEthernetClass))
#define OOBS_IS_IFACE_ETHERNET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_IFACE_ETHERNET))
#define OOBS_IS_IFACE_ETHERNET_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_IFACE_ETHERNET))
#define OOBS_IFACE_ETHERNET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_IFACE_ETHERNET, OobsIfaceEthernetClass))

typedef struct _OobsIfaceEthernet        OobsIfaceEthernet;
typedef struct _OobsIfaceEthernetClass   OobsIfaceEthernetClass;
	
struct _OobsIfaceEthernet {
  GObject parent;
};

struct _OobsIfaceEthernetClass {
  GObjectClass parent_class;
};

GType oobs_iface_configuration_method_get_type (void);
GType oobs_iface_ethernet_get_type (void);

G_CONST_RETURN gchar*   oobs_iface_ethernet_get_ip_address (OobsIfaceEthernet *iface);
void           oobs_iface_ethernet_set_ip_address (OobsIfaceEthernet *iface, const gchar *address);

G_CONST_RETURN gchar*   oobs_iface_ethernet_get_network_mask (OobsIfaceEthernet *iface);
void           oobs_iface_ethernet_set_network_mask (OobsIfaceEthernet *iface, const gchar *mask);

G_CONST_RETURN gchar*   oobs_iface_ethernet_get_gateway_address (OobsIfaceEthernet *iface);
void           oobs_iface_ethernet_set_gateway_address (OobsIfaceEthernet *iface, const gchar *address);

G_CONST_RETURN gchar*   oobs_iface_ethernet_get_network_address (OobsIfaceEthernet *iface);
void           oobs_iface_ethernet_set_network_address (OobsIfaceEthernet *iface, const gchar *address);

G_CONST_RETURN gchar*   oobs_iface_ethernet_get_broadcast_address (OobsIfaceEthernet *iface);
void           oobs_iface_ethernet_set_broadcast_address (OobsIfaceEthernet *iface, const gchar *address);

OobsIfaceConfigurationMethod  oobs_iface_ethernet_get_configuration_method (OobsIfaceEthernet *iface);
void  oobs_iface_ethernet_set_configuration_method (OobsIfaceEthernet *iface, OobsIfaceConfigurationMethod method);


G_END_DECLS


#endif /* __OOBS_IFACE_ETHERNET_H__ */
