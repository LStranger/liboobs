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
#include <string.h>
#include "oobs-ifacesconfig.h"
#include "oobs-iface-ethernet.h"
#include "oobs-iface.h"

/**
 * SECTION:oobs-iface-ethernet
 * @title: OobsIfaceEthernet
 * @short_description: Object that represents an individual Ethernet interface
 * @see_also: #OobsIface, #OobsIfacesConfig, #OobsIfaceIRLan,
 *     #OobsIfacePlip, #OobsIfacePPP, #OobsIfaceWireless
 **/

#define OOBS_IFACE_ETHERNET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACE_ETHERNET, OobsIfaceEthernetPrivate))

typedef struct _OobsIfaceEthernetPrivate OobsIfaceEthernetPrivate;

struct _OobsIfaceEthernetPrivate
{
  OobsObject *config;
  gchar *configuration_method;

  gchar *address;
  gchar *netmask;
  gchar *gateway;

  gchar *network;
  gchar *broadcast;
};

static void oobs_iface_ethernet_class_init (OobsIfaceEthernetClass *class);
static void oobs_iface_ethernet_init       (OobsIfaceEthernet      *iface);
static void oobs_iface_ethernet_finalize   (GObject                *object);

static gboolean oobs_iface_ethernet_has_gateway   (OobsIface *iface);
static gboolean oobs_iface_ethernet_is_configured (OobsIface *iface);

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

G_DEFINE_TYPE (OobsIfaceEthernet, oobs_iface_ethernet, OOBS_TYPE_IFACE);

static void
oobs_iface_ethernet_class_init (OobsIfaceEthernetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsIfaceClass *iface_class = OOBS_IFACE_CLASS (class);

  object_class->set_property = oobs_iface_ethernet_set_property;
  object_class->get_property = oobs_iface_ethernet_get_property;
  object_class->finalize     = oobs_iface_ethernet_finalize;

  iface_class->has_gateway   = oobs_iface_ethernet_has_gateway;
  iface_class->is_configured = oobs_iface_ethernet_is_configured;

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
				   g_param_spec_string ("config_method",
							"Iface configuration method",
							"Network configuration method for the iface",
							NULL,
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

  priv->config  = oobs_ifaces_config_get ();
  priv->address = NULL;
  priv->netmask = NULL;
  priv->gateway = NULL;
  priv->network = NULL;
  priv->broadcast = NULL;
  priv->configuration_method = NULL;
  iface->_priv = priv;
}

static void
oobs_iface_ethernet_finalize (GObject *object)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (object));

  priv = OOBS_IFACE_ETHERNET (object)->_priv;

  if (priv)
    {
      g_free (priv->address);
      g_free (priv->netmask);
      g_free (priv->gateway);
      g_free (priv->network);
      g_free (priv->broadcast);
      g_free (priv->configuration_method);
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

  priv = OOBS_IFACE_ETHERNET (object)->_priv;

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
      g_free (priv->broadcast);
      priv->broadcast = g_value_dup_string (value);
      break;
    case PROP_CONFIGURATION_METHOD:
      g_free (priv->configuration_method);
      priv->configuration_method = g_value_dup_string (value);
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

  priv = OOBS_IFACE_ETHERNET (object)->_priv;

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
      g_value_set_string (value, priv->configuration_method);
      break;
    }
}

static gboolean
oobs_iface_ethernet_has_gateway (OobsIface *iface)
{
  OobsIfaceEthernetPrivate *priv;
  GList *methods = NULL;

  priv = OOBS_IFACE_ETHERNET (iface)->_priv;
  methods = oobs_ifaces_config_get_available_configuration_methods (OOBS_IFACES_CONFIG (priv->config));

  /* assume that only the "static" configuration
   * method needs setting a gateway by hand
   */
  return (priv->configuration_method &&
	  ((strcmp (priv->configuration_method, "static") == 0 && priv->gateway) ||
	   g_list_find_custom (methods, priv->configuration_method, (GCompareFunc) strcmp)));
}

static gboolean
oobs_iface_ethernet_is_configured (OobsIface *iface)
{
  OobsIfaceEthernetPrivate *priv;
  GList *methods = NULL;

  priv = OOBS_IFACE_ETHERNET (iface)->_priv;
  methods = oobs_ifaces_config_get_available_configuration_methods (OOBS_IFACES_CONFIG (priv->config));

  /* assume that all supported configuration methods
   * except "static" do not require aditional configuration
   */
  if (!priv->configuration_method)
    return FALSE;

  if (strcmp (priv->configuration_method, "static") == 0)
    return (priv->address && priv->netmask);

  return (g_list_find_custom (methods, priv->configuration_method, (GCompareFunc) strcmp) != NULL);
}

/**
 * oobs_iface_ethernet_get_ip_address:
 * @iface: An #OobsIfaceEthernet.
 * 
 * Returns the IP address that this interface uses.
 * 
 * Return Value: A pointer to the IP address as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_ethernet_get_ip_address (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = iface->_priv;

  return priv->address;
}

/**
 * oobs_iface_ethernet_set_ip_address:
 * @iface: An #OobsIfaceEthernet.
 * @address: a new IP address for the interface.
 * 
 * Sets a new IP address for the interface,
 * overwriting the previous one.
 **/
void
oobs_iface_ethernet_set_ip_address (OobsIfaceEthernet *iface,
				    const gchar       *address)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "ip-address", address, NULL);
}

/**
 * oobs_iface_ethernet_get_network_mask:
 * @iface: An #OobsIfaceEthernet.
 * 
 * Returns the IP network mask that this interface uses.
 * 
 * Return Value: A pointer to the network mask as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_ethernet_get_network_mask (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = iface->_priv;

  return priv->netmask;
}

/**
 * oobs_iface_ethernet_set_network_mask:
 * @iface: An #OobsIfaceEthernet.
 * @mask: a new IP network mask for the interface.
 * 
 * Sets a new IP network mask for the interface,
 * overwriting the previous one.
 **/
void
oobs_iface_ethernet_set_network_mask (OobsIfaceEthernet *iface,
				      const gchar       *mask)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "ip-mask", mask, NULL);
}

/**
 * oobs_iface_ethernet_get_gateway_address:
 * @iface: An #OobsIfaceEthernet.
 * 
 * Returns the gateway IP address that this interface uses.
 * 
 * Return Value: A pointer to the gateway address as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_ethernet_get_gateway_address (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = iface->_priv;

  return priv->gateway;
}

/**
 * oobs_iface_ethernet_set_gateway_address:
 * @iface: An #OobsIfaceEthernet.
 * @address: a new gateway IP address for the interface.
 * 
 * Sets a new gateway IP address for the interface,
 * overwriting the previous one.
 **/
void
oobs_iface_ethernet_set_gateway_address (OobsIfaceEthernet *iface,
					 const gchar       *address)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "gateway-address", address, NULL);
}

/**
 * oobs_iface_ethernet_get_network_address:
 * @iface: An #OobsIfaceEthernet.
 * 
 * Returns the network address for this interface.
 * 
 * Return Value: A pointer to the network address as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_ethernet_get_network_address (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = iface->_priv;

  return priv->network;
}

/**
 * oobs_iface_ethernet_set_network_address:
 * @iface: An #OobsIfaceEthernet.
 * @address: a new network address for the interface.
 * 
 * Sets a new network address for the interface,
 * overwriting the previous one.
 **/
void
oobs_iface_ethernet_set_network_address (OobsIfaceEthernet *iface,
					 const gchar       *address)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "network-address", address, NULL);
}

/**
 * oobs_iface_ethernet_get_broadcast_address:
 * @iface: An #OobsIfaceEthernet.
 * 
 * Returns the broadcast address for this interface.
 * 
 * Return Value: A pointer to the broadcast address as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_ethernet_get_broadcast_address (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = iface->_priv;

  return priv->broadcast;
}

/**
 * oobs_iface_ethernet_set_broadcast_address:
 * @iface: An #OobsIfaceEthernet.
 * @address: a new broadcast address for the interface.
 * 
 * Sets a new broadcast address for the interface,
 * overwriting the previous one.
 **/
void
oobs_iface_ethernet_set_broadcast_address (OobsIfaceEthernet *iface,
					   const gchar       *address)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  /* FIXME: should validate IP address */

  g_object_set (G_OBJECT (iface), "broadcast-address", address, NULL);
}

/**
 * oobs_iface_ethernet_get_configuration_method:
 * @iface: An #OobsIfaceEthernet.
 * 
 * Returns the configuration method for the interface.
 * 
 * Return Value: The configuration method that the interface uses.
 * This value must not be modified nor freed.
 **/
G_CONST_RETURN gchar*
oobs_iface_ethernet_get_configuration_method (OobsIfaceEthernet *iface)
{
  OobsIfaceEthernetPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ETHERNET (iface), NULL);
  
  priv = iface->_priv;

  return priv->configuration_method;
}

/**
 * oobs_iface_ethernet_set_configuration_method:
 * @iface: An #OobsIfaceEthernet.
 * @method: A new configuration method for the interface, or %NULL.
 * 
 * Sets the configuration method that the interface will use. The valid methods are
 * provided by oobs_ifaces_config_get_available_configuration_methods().
 **/
void
oobs_iface_ethernet_set_configuration_method (OobsIfaceEthernet *iface,
					      const gchar       *method)
{
  g_return_if_fail (OOBS_IS_IFACE_ETHERNET (iface));

  g_object_set (G_OBJECT (iface), "config-method", method, NULL);
}
