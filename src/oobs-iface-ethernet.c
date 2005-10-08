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
#include "oobs-iface-ethernet.h"
#include "oobs-iface.h"

#define OOBS_IFACE_ETHERNET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACE_ETHERNET, OobsIfaceEthernetPrivate))

typedef struct _OobsIfaceEthernetPrivate OobsIfaceEthernetPrivate;

struct _OobsIfaceEthernetPrivate
{
  gchar *address;
  gchar *netmask;
  gchar *gateway;

  gchar *network;
  gchar *broadcast;

  OobsIfaceConfigurationMethod configuration_method;
};

static void oobs_iface_ethernet_class_init (OobsIfaceEthernetClass *class);
static void oobs_iface_ethernet_init       (OobsIfaceEthernet      *iface);
static void oobs_iface_ethernet_finalize   (GObject                *object);

static gboolean oobs_iface_ethernet_has_gateway (OobsIface *iface);

static void oobs_iface_ethernet_set_property (GObject      *object,
					      guint         prop_id,
					      const GValue *value,
					      GParamSpec   *pspec);
static void oobs_iface_ethernet_get_property (GObject      *object,
					      guint         prop_id,
					      GValue       *value,
					      GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_ADDRESS,
  PROP_NETMASK,
  PROP_GATEWAY,
  PROP_NETWORK,
  PROP_BROADCAST,
  PROP_CONFIGURATION_METHOD
};

G_DEFINE_TYPE (OobsIfaceEthernet, oobs_iface_ethernet, G_TYPE_OBJECT);

GType
oobs_iface_configuration_method_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
	{
	  { OOBS_METHOD_NONE,   "OOBS_METHOD_NONE",   "none" },
	  { OOBS_METHOD_STATIC, "OOBS_METHOD_STATIC", "static" },
	  { OOBS_METHOD_DHCP,   "OOBS_METHOD_DHCP",   "dhcp" },
	};

      etype = g_enum_register_static ("OobsIfaceConfigurationMethod", values);
    }

  return etype;
}

static void
oobs_iface_ethernet_class_init (OobsIfaceEthernetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsIfaceClass *iface_class = OOBS_IFACE_CLASS (class);

  object_class->set_property = oobs_iface_ethernet_set_property;
  object_class->get_property = oobs_iface_ethernet_get_property;
  object_class->finalize     = oobs_iface_ethernet_finalize;

  iface_class->has_gateway   = oobs_iface_ethernet_has_gateway;

  g_object_class_install_property (object_class,
				   PROP_ADDRESS,
				   g_param_spec_string ("ip_address",
							"Iface address",
							"Address for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_NETMASK,
				   g_param_spec_string ("ip_mask",
							"Iface netmask",
							"Netmask for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_GATEWAY,
				   g_param_spec_string ("gateway_address",
							"Iface gateway",
							"Gateway address for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_NETWORK,
				   g_param_spec_string ("network_address",
							"Iface network address",
							"Network address for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_BROADCAST,
				   g_param_spec_string ("broadcast_address",
							"Iface broadcast",
							"Network broadcast for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_CONFIGURATION_METHOD,
				   g_param_spec_enum ("configuration_method",
						      "Iface configuration method",
						      "Network configuration method for the iface",
						      OOBS_TYPE_IFACE_CONFIGURATION_METHOD,
						      OOBS_METHOD_NONE,
						      G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsIfaceEthernetPrivate));
}

static void
oobs_iface_ethernet_init (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (iface);

  priv->address = NULL;
  priv->netmask = NULL;
  priv->gateway = NULL;
  priv->network = NULL;
  priv->broadcast = NULL;
}

static void
oobs_iface_ethernet_finalize (GObject *object)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (object));

  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (object);

  if (priv)
    {
      g_free (priv->address);
      g_free (priv->netmask);
      g_free (priv->gateway);
      g_free (priv->network);
      g_free (priv->broadcast);
    }

  if (G_OBJECT_CLASS (oobs_iface_ethernet_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_iface_ethernet_parent_class)->finalize) (object);
}

static void
oobs_iface_ethernet_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (object));

  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_free (priv->address);
      priv->address = g_value_dup_string (value);
      break;
    case PROP_NETMASK:
      g_free (priv->netmask);
      priv->netmask = g_value_dup_string (value);
      break;
    case PROP_GATEWAY:
      g_free (priv->gateway);
      priv->gateway = g_value_dup_string (value);
      break;
    case PROP_NETWORK:
      g_free (priv->network);
      priv->network = g_value_dup_string (value);
      break;
    case PROP_BROADCAST:
      g_free (priv->network);
      priv->broadcast = g_value_dup_string (value);
      break;
    case PROP_CONFIGURATION_METHOD:
      priv->configuration_method = g_value_get_enum (value);
      break;
    }
}

static void
oobs_iface_ethernet_get_property (GObject      *object,
				  guint         prop_id,
				  GValue       *value,
				  GParamSpec   *pspec)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (object));

  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_value_set_string (value, priv->address);
      break;
    case PROP_NETMASK:
      g_value_set_string (value, priv->netmask);
      break;
    case PROP_GATEWAY:
      g_value_set_string (value, priv->gateway);
      break;
    case PROP_NETWORK:
      g_value_set_string (value, priv->network);
      break;
    case PROP_BROADCAST:
      g_value_set_string (value, priv->broadcast);
      break;
    case PROP_CONFIGURATION_METHOD:
      g_value_set_enum (value, priv->configuration_method);
      break;
    }
}

static gboolean
oobs_iface_ethernet_has_gateway (OobsIface *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), FALSE);

  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (iface);
  
  return ((priv->configuration_method == OOBS_METHOD_DHCP) ||
	  (priv->gateway));
}

G_CONST_RETURN gchar*
oobs_iface_ethernet_get_ip_address (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (iface);

  return priv->address;
}

void
oobs_iface_ethernet_set_ip_address (OobsIfaceEthernet *iface,
				    const gchar       *address)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "ip-address", address, NULL);
}

G_CONST_RETURN gchar*
oobs_iface_ethernet_get_network_mask (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (iface);

  return priv->netmask;
}

void
oobs_iface_ethernet_set_network_mask (OobsIfaceEthernet *iface,
				      const gchar       *mask)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "ip-mask", mask, NULL);
}

G_CONST_RETURN gchar*
oobs_iface_ethernet_get_gateway_address (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (iface);

  return priv->gateway;
}

void
oobs_iface_ethernet_set_gateway_address (OobsIfaceEthernet *iface,
					 const gchar       *address)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "gateway-address", address, NULL);
}

G_CONST_RETURN gchar*
oobs_iface_ethernet_get_network_address (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (iface);

  return priv->network;
}

void
oobs_iface_ethernet_set_network_address (OobsIfaceEthernet *iface,
					 const gchar       *address)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "network-address", address, NULL);
}

G_CONST_RETURN gchar*
oobs_iface_ethernet_get_broadcast_address (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (iface);

  return priv->broadcast;
}

void
oobs_iface_ethernet_set_broadcast_address (OobsIfaceEthernet *iface,
					   const gchar       *address)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "broadcast-address", address, NULL);
}

OobsIfaceConfigurationMethod
oobs_iface_ethernet_get_configuration_method (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), OOBS_METHOD_NONE);
  
  priv = OOBS_IFACE_ETHERNET_GET_PRIVATE (iface);

  return priv->configuration_method;
}

void
oobs_iface_ethernet_set_configuration_method (OobsIfaceEthernet            *iface,
					      OobsIfaceConfigurationMethod  method)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "configuration-method", method, NULL);
}
