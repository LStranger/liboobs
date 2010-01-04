/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2005 Carlos Garnacho
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

#include <glib-object.h>
#include "oobs-iface-ppp.h"
#include "oobs-iface.h"

#include <string.h>

/**
 * SECTION:oobs-iface-ppp
 * @title: OobsIfacePPP
 * @short_description: Object that represents an individual PPP interface
 * @see_also: #OobsIface, #OobsIfacesConfig, #OobsIfaceEthernet,
 *     #OobsIfaceIRLan, #OobsIfacePlip, #OobsIfaceWireless
 **/

#define OOBS_IFACE_PPP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACE_PPP, OobsIfacePPPPrivate))

typedef struct _OobsIfacePPPPrivate OobsIfacePPPPrivate;

struct _OobsIfacePPPPrivate
{
  gchar *connection_type;
  gchar *login;
  gchar *password;
  gchar *phone_number;
  gchar *dial_prefix;
  gchar *section;
  gchar *serial_port;
  gchar *apn;

  OobsIfaceEthernet *ethernet;

  guint default_gw : 1;
  guint peerdns : 1;
  guint persist : 1;
  guint noauth : 1;
  guint volume : 2;
  guint dial_type : 1;
};

static void oobs_iface_ppp_class_init (OobsIfacePPPClass *class);
static void oobs_iface_ppp_init       (OobsIfacePPP      *iface);
static void oobs_iface_ppp_finalize   (GObject           *object);

static gboolean oobs_iface_ppp_has_gateway   (OobsIface *iface);
static gboolean oobs_iface_ppp_is_configured (OobsIface *iface);

static void oobs_iface_ppp_set_property (GObject      *object,
					 guint         prop_id,
					 const GValue *value,
					 GParamSpec   *pspec);
static void oobs_iface_ppp_get_property (GObject      *object,
					 guint         prop_id,
					 GValue       *value,
					 GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_CONNECTION_TYPE,
  PROP_LOGIN,
  PROP_PASSWORD,
  PROP_PHONE_NUMBER,
  PROP_DIAL_PREFIX,
  PROP_DEFAULT_GW,
  PROP_PEERDNS,
  PROP_PERSIST,
  PROP_SECTION,
  PROP_NOAUTH,
  PROP_SERIAL_PORT,
  PROP_VOLUME,
  PROP_DIAL_TYPE,
  PROP_ETHERNET,
  PROP_APN
};

G_DEFINE_TYPE (OobsIfacePPP, oobs_iface_ppp, OOBS_TYPE_IFACE);

static void
oobs_iface_ppp_class_init (OobsIfacePPPClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsIfaceClass *iface_class = OOBS_IFACE_CLASS (class);

  object_class->set_property = oobs_iface_ppp_set_property;
  object_class->get_property = oobs_iface_ppp_get_property;
  object_class->finalize = oobs_iface_ppp_finalize;

  iface_class->has_gateway = oobs_iface_ppp_has_gateway;
  iface_class->is_configured = oobs_iface_ppp_is_configured;

  g_object_class_install_property (object_class,
                                   PROP_CONNECTION_TYPE,
                                   g_param_spec_string ("connection-type",
                                                        "PPP connection type",
                                                        "PPP connection type",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_LOGIN,
                                   g_param_spec_string ("login",
                                                        "Login",
                                                        "Login for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_PASSWORD,
                                   g_param_spec_string ("password",
                                                        "Password",
                                                        "Password for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_PHONE_NUMBER,
                                   g_param_spec_string ("phone-number",
                                                        "Phone number",
                                                        "Phone number for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DIAL_PREFIX,
                                   g_param_spec_string ("phone-prefix",
                                                        "Phone prefix",
                                                        "Phone prefix for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_SECTION,
                                   g_param_spec_string ("iface-section",
                                                         "Iface section",
                                                         "Name of the wvdial section or the provider name",
                                                         NULL,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DEFAULT_GW,
                                   g_param_spec_boolean ("default-gateway",
                                                         "Default gateway",
                                                         "Whether to use the iface as the default gateway",
                                                         TRUE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_PEERDNS,
                                   g_param_spec_boolean ("use-peer-dns",
                                                         "Iface uses peer DNS",
                                                         "Whether to use the ISP DNS",
                                                         TRUE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_PERSIST,
                                   g_param_spec_boolean ("persistent",
                                                         "Iface is persistent",
                                                         "Whether to persist if the connection fails",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_NOAUTH,
                                   g_param_spec_boolean ("peer-noauth",
                                                         "Peer no auth",
                                                         "Whether the ISP has to authenticate itself or not",
                                                         TRUE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_SERIAL_PORT,
                                   g_param_spec_string ("serial-port",
                                                        "Serial port",
                                                        "Serial port for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_VOLUME,
                                   g_param_spec_enum ("volume",
                                                      "Modem volume",
                                                      "Modem volume",
                                                      OOBS_TYPE_MODEM_VOLUME,
                                                      OOBS_MODEM_VOLUME_SILENT,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DIAL_TYPE,
                                   g_param_spec_enum ("dial_type",
                                                      "Modem dial type",
                                                      "Modem dial type",
                                                      OOBS_TYPE_DIAL_TYPE,
                                                      OOBS_DIAL_TYPE_TONES,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_ETHERNET,
                                   g_param_spec_object ("ethernet",
							"Ethernet interface",
							"Ethernet interface used for PPPoE",
							OOBS_TYPE_IFACE_ETHERNET,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_APN,
                                   g_param_spec_string ("apn",
							"Access provider name",
							"Access provider name used for GPRS",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsIfacePPPPrivate));
}

static void
oobs_iface_ppp_init (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  priv = OOBS_IFACE_PPP_GET_PRIVATE (iface);
  iface->_priv = priv;
}

static void
oobs_iface_ppp_finalize (GObject *object)
{
  OobsIfacePPPPrivate *priv;

  priv = OOBS_IFACE_PPP (object)->_priv;

  if (priv)
    {
      g_free (priv->connection_type);
      g_free (priv->login);
      g_free (priv->password);
      g_free (priv->phone_number);
      g_free (priv->dial_prefix);
      g_free (priv->section);
      g_free (priv->serial_port);
      g_free (priv->apn);

      if (priv->ethernet)
	g_object_unref (priv->ethernet);
    }

  if (G_OBJECT_CLASS (oobs_iface_ppp_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_iface_ppp_parent_class)->finalize) (object);
}

static void
oobs_iface_ppp_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  OobsIfacePPPPrivate *priv;

  priv = OOBS_IFACE_PPP (object)->_priv;

  switch (prop_id)
    {
    case PROP_CONNECTION_TYPE:
      oobs_iface_ppp_set_connection_type (OOBS_IFACE_PPP (object),
					  g_value_get_string (value));
      break;
    case PROP_LOGIN:
      g_free (priv->login);
      priv->login = g_value_dup_string (value);
      break;
    case PROP_PASSWORD:
      g_free (priv->password);
      priv->password = g_value_dup_string (value);
      break;
    case PROP_PHONE_NUMBER:
      g_free (priv->phone_number);
      priv->phone_number = g_value_dup_string (value);
      break;
    case PROP_DIAL_PREFIX:
      g_free (priv->dial_prefix);
      priv->dial_prefix = g_value_dup_string (value);
      break;
    case PROP_SECTION:
      priv->section = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_GW:
      priv->default_gw = g_value_get_boolean (value);
      break;
    case PROP_PEERDNS:
      priv->peerdns = g_value_get_boolean (value);
      break;
    case PROP_PERSIST:
      priv->persist = g_value_get_boolean (value);
      break;
    case PROP_NOAUTH:
      priv->noauth = g_value_get_boolean (value);
      break;
    case PROP_SERIAL_PORT:
      g_free (priv->serial_port);
      priv->serial_port = g_value_dup_string (value);
      break;
    case PROP_VOLUME:
      priv->volume = g_value_get_enum (value);
      break;
    case PROP_DIAL_TYPE:
      priv->dial_type = g_value_get_enum (value);
      break;
    case PROP_ETHERNET:
      oobs_iface_ppp_set_ethernet (OOBS_IFACE_PPP (object),
				   OOBS_IFACE_ETHERNET (g_value_get_object (value)));
      break;
    case PROP_APN:
      oobs_iface_ppp_set_apn (OOBS_IFACE_PPP (object),
			      g_value_get_string (value));
      break;
    }
}

static void
oobs_iface_ppp_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  OobsIfacePPPPrivate *priv;

  priv = OOBS_IFACE_PPP (object)->_priv;

  switch (prop_id)
    {
    case PROP_CONNECTION_TYPE:
      g_value_set_string (value, priv->connection_type);
      break;
    case PROP_LOGIN:
      g_value_set_string (value, priv->login);
      break;
    case PROP_PASSWORD:
      g_value_set_string (value, priv->password);
      break;
    case PROP_PHONE_NUMBER:
      g_value_set_string (value, priv->phone_number);
      break;
    case PROP_DIAL_PREFIX:
      g_value_set_string (value, priv->dial_prefix);
      break;
    case PROP_SECTION:
      g_value_set_string (value, priv->section);
      break;
    case PROP_DEFAULT_GW:
      g_value_set_boolean (value, priv->default_gw);
      break;
    case PROP_PEERDNS:
      g_value_set_boolean (value, priv->peerdns);
      break;
    case PROP_PERSIST:
      g_value_set_boolean (value, priv->persist);
      break;
    case PROP_NOAUTH:
      g_value_set_boolean (value, priv->noauth);
      break;
    case PROP_SERIAL_PORT:
      g_value_set_string (value, priv->serial_port);
      break;
    case PROP_VOLUME:
      g_value_set_enum (value, priv->volume);
      break;
    case PROP_DIAL_TYPE:
      g_value_set_enum (value, priv->dial_type);
      break;
    case PROP_ETHERNET:
      g_value_set_object (value,
			  oobs_iface_ppp_get_ethernet (OOBS_IFACE_PPP (object)));
      break;
    case PROP_APN:
      g_value_set_string (value,
			  oobs_iface_ppp_get_apn (OOBS_IFACE_PPP (object)));
      break;
    }
}

static gboolean
oobs_iface_ppp_has_gateway (OobsIface *iface)
{
  OobsIfacePPPPrivate *priv;

  priv = OOBS_IFACE_PPP (iface)->_priv;

  return priv->default_gw;
}

static gboolean
oobs_iface_ppp_is_configured (OobsIface *iface)
{
  OobsIfacePPPPrivate *priv;

  priv = OOBS_IFACE_PPP (iface)->_priv;

  if (!priv->connection_type)
    return FALSE;

  if (strcmp (priv->connection_type, "modem") == 0)
    return (priv->login && priv->phone_number && priv->serial_port);
  else if (strcmp (priv->connection_type, "isdn") == 0)
    return (priv->login && priv->phone_number);
  else if (strcmp (priv->connection_type, "pppoe") == 0)
    return (priv->login && priv->ethernet);
  else if (strcmp (priv->connection_type, "gprs") == 0)
    return (priv->apn && priv->serial_port);

  /* Assume unknown connection types are configured */
  return TRUE;
}

/**
 * oobs_iface_ppp_set_connection_type:
 * @iface: An #OobsIfacePPP
 * @type: A string defining the connection type.
 *
 * Sets the connection type for the PPP interface. The list of possible values
 * can be obtained through oobs_ifaces_config_get_available_ppp_types ().
 **/
void
oobs_iface_ppp_set_connection_type (OobsIfacePPP *iface,
				    const gchar  *type)
{
  OobsIfacePPPPrivate *priv;
  gchar *str;

  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  priv = iface->_priv;
  str = g_strdup (type);
  g_free (priv->connection_type);
  priv->connection_type = str;

  g_object_notify (G_OBJECT (iface), "connection-type");
}

/**
 * oobs_iface_ppp_get_connection_type:
 * @iface: An #OobsIfacePPP
 *
 * Gets the connection type for the PPP interface.
 *
 * Return Value: A string defining the connection type. This string must
 *               not be freed, modified or stored.
 **/
G_CONST_RETURN gchar *
oobs_iface_ppp_get_connection_type (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), NULL);

  priv = iface->_priv;
  return priv->connection_type;
}

/**
 * oobs_iface_ppp_set_login:
 * @iface: An #OobsIfacePPP.
 * @login: a new login for the PPP connection.
 * 
 * Sets a new login for the PPP connection, overwriting
 * the previous one.
 **/
void
oobs_iface_ppp_set_login (OobsIfacePPP *iface,
			  const gchar  *login)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "login", login, NULL);
}

/**
 * oobs_iface_ppp_get_login:
 * @iface: An #OobsIfacePPP.
 * 
 * Returns the login used for this PPP connection.
 * 
 * Return Value: A pointer to the login. This string must
 *               not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_ppp_get_login (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), NULL);

  priv = iface->_priv;
  return priv->login;
}

/**
 * oobs_iface_ppp_set_password:
 * @iface: An #OobsIfacePPP.
 * @password: a new password for the PPP connection.
 * 
 * Sets a new password for the PPP connection, overwriting
 * the previous one.
 **/
void
oobs_iface_ppp_set_password (OobsIfacePPP *iface,
			     const gchar  *password)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "password", password, NULL);
}

/**
 * oobs_iface_ppp_set_phone_number:
 * @iface: An #OobsIfacePPP.
 * @phone_number: a new phone number for the PPP connection.
 *
 * Sets a new phone number for the PPP connection, overwriting
 * the previous one.
 **/
void
oobs_iface_ppp_set_phone_number (OobsIfacePPP *iface,
				 const gchar  *phone_number)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "phone-number", phone_number, NULL);
}

/**
 * oobs_iface_ppp_get_phone_number:
 * @iface: An #OobsIfacePPP.
 * 
 * Returns the phone number used for this PPP connection.
 * 
 * Return Value: A pointer to the phone number. This string must
 *               not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_ppp_get_phone_number (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), NULL);

  priv = iface->_priv;
  return priv->phone_number;
}

/**
 * oobs_iface_ppp_set_phone_prefix:
 * @iface: An #OobsIfacePPP.
 * @phone_prefix: a new phone number prefix for the PPP connection.
 * 
 * Sets a new phone number prefix for the PPP connection,
 * overwriting the previous one.
 **/
void
oobs_iface_ppp_set_phone_prefix (OobsIfacePPP *iface,
				 const gchar  *phone_prefix)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "phone-prefix", phone_prefix, NULL);
}

/**
 * oobs_iface_ppp_get_phone_prefix:
 * @iface: An #OobsIfacePPP.
 * 
 * Returns the phone number prefix used for this PPP connection.
 * 
 * Return Value: A pointer to the phone number prefix. This string
 *               must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_ppp_get_phone_prefix (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), NULL);

  priv = iface->_priv;
  return priv->dial_prefix;
}

/**
 * oobs_iface_ppp_set_default_gateway:
 * @iface: An #OobsIfacePPP.
 * @default_gw: #TRUE to make this interface the default gateway to Internet.
 *
 * Sets whether the interface is the default gateway to Internet.
 **/
void
oobs_iface_ppp_set_default_gateway (OobsIfacePPP *iface,
				    gboolean      default_gw)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "default-gw", default_gw, NULL);
}

/**
 * oobs_iface_ppp_get_default_gateway:
 * @iface: An #OobsIfacePPP.
 *
 * Returns whether the interface is the default gateway to Internet.
 *
 * Return Value: #TRUE if the interface is the default gateway.
 **/
gboolean
oobs_iface_ppp_get_default_gateway (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), FALSE);

  priv = iface->_priv;
  return priv->default_gw;
}

/**
 * oobs_iface_ppp_set_use_peer_dns:
 * @iface: An #OobsIfacePPP.
 * @use_peer_dns: #TRUE to use DNS servers specified by the ISP.
 *
 * Sets whether to use the DNS servers that the ISP specifies.
 **/
void
oobs_iface_ppp_set_use_peer_dns (OobsIfacePPP *iface,
				 gboolean      use_peer_dns)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "peer-dns", use_peer_dns, NULL);
}

/**
 * oobs_iface_ppp_get_use_peer_dns:
 * @iface: An #OobsIfacePPP.
 *
 * Returns whether the DNS servers specified by the ISP
 * will be used.
 *
 * Return Value: #TRUE if the DNS servers will be used.
 **/
gboolean
oobs_iface_ppp_get_use_peer_dns (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), FALSE);

  priv = iface->_priv;
  return priv->peerdns;
}

/**
 * oobs_iface_ppp_set_persistent:
 * @iface: An #OobsIfacePPP.
 * @persistent: #TRUE to try to reconnect if the connection fails.
 *
 * Sets whether the interface will try to reconnect if the connection fails.
 **/
void
oobs_iface_ppp_set_persistent (OobsIfacePPP *iface,
			       gboolean      persistent)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "persistent", persistent, NULL);
}

/**
 * oobs_iface_ppp_get_persistent:
 * @iface: An #OobsIfacePPP.
 *
 * Returns whether the interface will try to reconnect if the
 * connection fails.
 *
 * Return Value: #TRUE if the interface will try to reconnect.
 **/
gboolean
oobs_iface_ppp_get_persistent (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), FALSE);

  priv = iface->_priv;
  return priv->persist;
}

/**
 * oobs_iface_ppp_set_peer_noauth:
 * @iface: An #OobsIfacePPP.
 * @use_peer_dns: #TRUE if the peer has to authenticate itself.
 *
 * Sets whether the peer has to authenticate itself.
 **/
void
oobs_iface_ppp_set_peer_noauth (OobsIfacePPP *iface,
				gboolean      use_peer_dns)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "peer-dns", use_peer_dns, NULL);
}

/**
 * oobs_iface_ppp_get_peer_noauth:
 * @iface: An #OobsIfacePPP.
 *
 * Returns whether the peer is required to authenticate itself.
 *
 * Return Value: #TRUE if the peer is required to authenticate itself.
 **/
gboolean
oobs_iface_ppp_get_peer_noauth (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), FALSE);

  priv = iface->_priv;
  return priv->noauth;
}

/**
 * oobs_iface_ppp_set_serial_port:
 * @iface: An #OobsIfacePPP.
 * @serial_port: a new serial port for the interface.
 *
 * Sets a serial port to connect with the modem device.
 **/
void
oobs_iface_ppp_set_serial_port (OobsIfacePPP *iface,
				const gchar  *serial_port)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "serial-port", serial_port, NULL);
}

/**
 * oobs_iface_ppp_get_serial_port:
 * @iface: An #OobsIfacePPP.
 *
 * Returns the serial port used to communicate with the modem device.
 *
 * Return Value: A pointer to the serial port as a string. This string
 *               must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_ppp_get_serial_port (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), NULL);

  priv = iface->_priv;
  return priv->serial_port;
}

/**
 * oobs_iface_ppp_set_volume:
 * @iface: An #OobsIfacePPP.
 * @volume: the modem volume.
 *
 * Sets the modem volume.
 **/
void
oobs_iface_ppp_set_volume (OobsIfacePPP    *iface,
			   OobsModemVolume  volume)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "volume", volume, NULL);
}

/**
 * oobs_iface_ppp_get_volume:
 * @iface: An #OobsIfacePPP.
 *
 * Returns the modem volume.
 *
 * Return Value: the modem volume.
 **/
OobsModemVolume
oobs_iface_ppp_get_volume (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), OOBS_MODEM_VOLUME_SILENT);

  priv = iface->_priv;
  return priv->volume;
}

/**
 * oobs_iface_ppp_set_dial_type:
 * @iface: An #OobsIfacePPP.
 * @dial_type: dialing type for the modem connection.
 *
 * Sets the dialing type for the modem connection.
 **/
void
oobs_iface_ppp_set_dial_type (OobsIfacePPP *iface,
			      OobsDialType  dial_type)
{
  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  g_object_set (G_OBJECT (iface), "dial-type", dial_type, NULL);
}

/**
 * oobs_iface_ppp_get_dial_type:
 * @iface: An #OobsIfacePPP.
 *
 * Returns the dialing type for the modem connection.
 *
 * Return Value: the dialing type.
 **/
OobsDialType
oobs_iface_ppp_get_dial_type (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), OOBS_DIAL_TYPE_TONES);

  priv = iface->_priv;
  return priv->dial_type;
}

/**
 * oobs_iface_ppp_set_ethernet:
 * @iface: An #OobsIfacePPP
 * @ethernet: An #OobsIfaceEthernet to use in the PPPoE setup.
 *
 * Sets the ethernet interface that will be used if the
 * PPP interface is configured as PPPoE.
 **/
void
oobs_iface_ppp_set_ethernet (OobsIfacePPP      *iface,
			     OobsIfaceEthernet *ethernet)
{
  OobsIfacePPPPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));
  g_return_if_fail (!ethernet || OOBS_IS_IFACE_ETHERNET (ethernet));

  priv = iface->_priv;

  if (priv->ethernet)
    g_object_unref (priv->ethernet);

  if (ethernet)
    g_object_ref (ethernet);

  priv->ethernet = ethernet;
  g_object_notify (G_OBJECT (iface), "ethernet");
}

/**
 * oobs_iface_ppp_get_ethernet:
 * @iface: An #OobsIfacePPP
 *
 * Returns the ethernet interface used by the PPP interface
 * if it's configured as PPPoE, or NULL if none is set.
 * You must not unref this.
 *
 * Return Value: An #OobsIfaceEthernet
 **/
OobsIfaceEthernet *
oobs_iface_ppp_get_ethernet (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), NULL);

  priv = iface->_priv;
  return priv->ethernet;
}

/**
 * oobs_iface_ppp_set_apn:
 * @iface: An #OobsIfacePPP
 * @apn: Access point name of the GPRS provider.
 *
 * Sets the Access point name that will be used if the
 * PPP interface is configured as GPRS.
 **/
void
oobs_iface_ppp_set_apn (OobsIfacePPP *iface,
			const gchar  *apn)
{
  OobsIfacePPPPrivate *priv;
  gchar *str;

  g_return_if_fail (OOBS_IS_IFACE_PPP (iface));

  priv = iface->_priv;

  str = g_strdup (apn);
  g_free (priv->apn);
  priv->apn = str;

  g_object_notify (G_OBJECT (iface), "apn");
}

/**
 * oobs_iface_ppp_get_apn:
 * @iface: An #OobsIfacePPP
 *
 * Returns the Access point name for the GPRS provider.
 *
 * Return Value: The Access point name. This string must
 *               not be freed, modified or stored.
 **/
G_CONST_RETURN gchar *
oobs_iface_ppp_get_apn (OobsIfacePPP *iface)
{
  OobsIfacePPPPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PPP (iface), NULL);

  priv = iface->_priv;
  return priv->apn;
}
