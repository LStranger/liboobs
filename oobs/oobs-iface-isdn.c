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

#include <glib-object.h>
#include "oobs-iface-isdn.h"
#include "oobs-iface.h"

#define OOBS_IFACE_ISDN_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACE_ISDN, OobsIfaceISDNPrivate))

typedef struct _OobsIfaceISDNPrivate OobsIfaceISDNPrivate;

struct _OobsIfaceISDNPrivate
{
  gchar *login;
  gchar *password;
  gchar *phone_number;
  gchar *dial_prefix;
  gchar *section;
  
  gboolean default_gw : 1;
  gboolean peerdns : 1;
  gboolean persist : 1;
  gboolean noauth : 1;
};

static void oobs_iface_isdn_class_init (OobsIfaceISDNClass *class);
static void oobs_iface_isdn_init       (OobsIfaceISDN      *iface);
static void oobs_iface_isdn_finalize   (GObject           *object);

static gboolean oobs_iface_isdn_has_gateway   (OobsIface *iface);
static gboolean oobs_iface_isdn_is_configured (OobsIface *iface);

static void oobs_iface_isdn_set_property (GObject      *object,
					  guint         prop_id,
					  const GValue *value,
					  GParamSpec   *pspec);
static void oobs_iface_isdn_get_property (GObject      *object,
					  guint         prop_id,
					  GValue       *value,
					  GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_LOGIN,
  PROP_PASSWORD,
  PROP_PHONE_NUMBER,
  PROP_DIAL_PREFIX,
  PROP_DEFAULT_GW,
  PROP_PEERDNS,
  PROP_PERSIST,
  PROP_SECTION,
  PROP_NOAUTH
};

G_DEFINE_TYPE (OobsIfaceISDN, oobs_iface_isdn, OOBS_TYPE_IFACE);


static void
oobs_iface_isdn_class_init (OobsIfaceISDNClass *class)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (class);
  OobsIfaceClass *iface_class  = OOBS_IFACE_CLASS (class);

  object_class->set_property = oobs_iface_isdn_set_property;
  object_class->get_property = oobs_iface_isdn_get_property;
  object_class->finalize = oobs_iface_isdn_finalize;

  iface_class->has_gateway = oobs_iface_isdn_has_gateway;
  iface_class->is_configured = oobs_iface_isdn_is_configured;
  
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
                                   g_param_spec_string ("phone_number",
                                                        "Phone number",
                                                        "Phone number for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DIAL_PREFIX,
                                   g_param_spec_string ("phone_prefix",
                                                        "Phone prefix",
                                                        "Phone prefix for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_SECTION,
                                   g_param_spec_string ("iface_section",
                                                         "Iface section",
                                                         "Name of the wvdial section or the provider name",
                                                         NULL,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DEFAULT_GW,
                                   g_param_spec_boolean ("default_gw",
                                                         "Default gateway",
                                                         "Whether to use the iface as the default gateway",
                                                         TRUE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_PEERDNS,
                                   g_param_spec_boolean ("peer_dns",
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
                                   g_param_spec_boolean ("peer_noauth",
                                                         "Peer no auth",
                                                         "Whether the ISP has to authenticate itself or not",
                                                         TRUE,
                                                         G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsIfaceISDNPrivate));
}

static void
oobs_iface_isdn_init (OobsIfaceISDN *iface)
{
  OobsIfaceISDNPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_ISDN (iface));

  priv = OOBS_IFACE_ISDN_GET_PRIVATE (iface);

  priv->login = NULL;
  priv->password = NULL;
  priv->phone_number = NULL;
  priv->dial_prefix = NULL;
  priv->section = NULL;

  priv->default_gw = TRUE;
  priv->peerdns = TRUE;
  priv->persist = TRUE;
  priv->noauth = TRUE;
  iface->_priv = priv;
}

static void
oobs_iface_isdn_finalize (GObject *object)
{
  OobsIfaceISDNPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_ISDN (object));

  priv = OOBS_IFACE_ISDN (object)->_priv;

  if (priv)
    {
      g_free (priv->login);
      g_free (priv->password);
      g_free (priv->phone_number);
      g_free (priv->dial_prefix);
      g_free (priv->section);
    }

  if (G_OBJECT_CLASS (oobs_iface_isdn_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_iface_isdn_parent_class)->finalize) (object);
}

static void
oobs_iface_isdn_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  OobsIfaceISDNPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_ISDN (object));

  priv = OOBS_IFACE_ISDN (object)->_priv;
  
  switch (prop_id)
    {
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
    }
}

static void
oobs_iface_isdn_get_property (GObject      *object,
			      guint         prop_id,
			      GValue       *value,
			      GParamSpec   *pspec)
{
  OobsIfaceISDNPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_ISDN (object));

  priv = OOBS_IFACE_ISDN (object)->_priv;
  
  switch (prop_id)
    {
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
    }
}

static gboolean
oobs_iface_isdn_has_gateway (OobsIface *iface)
{
  OobsIfaceISDNPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ISDN (iface), FALSE);

  priv = OOBS_IFACE_ISDN (iface)->_priv;
  
  return priv->default_gw;
}

static gboolean
oobs_iface_isdn_is_configured (OobsIface *iface)
{
  OobsIfaceISDNPrivate *priv;

  priv = OOBS_IFACE_ISDN (iface)->_priv;

  return (priv->login && priv->phone_number);
}

/**
 * oobs_iface_isdn_set_login:
 * @iface: An #OobsIfaceISDN.
 * @login: a new login for the ISDN connection.
 * 
 * Sets a new login for the ISDN connection, overwriting
 * the previous one.
 **/
void
oobs_iface_isdn_set_login (OobsIfaceISDN *iface,
			   const gchar   *login)
{
  g_return_if_fail (OOBS_IS_IFACE_ISDN (iface));

  g_object_set (G_OBJECT (iface), "login", login, NULL);
}

/**
 * oobs_iface_isdn_get_login:
 * @iface: An #OobsIfaceISDN.
 * 
 * Returns the login used for this ISDN connection.
 * 
 * Return Value: A pointer to the login. This string must
 *               not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_isdn_get_login (OobsIfaceISDN *iface)
{
  OobsIfaceISDNPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ISDN (iface), NULL);

  priv = iface->_priv;
  return priv->login;
}

/**
 * oobs_iface_isdn_set_password:
 * @iface: An #OobsIfaceISDN.
 * @password: a new password for the ISDN connection.
 * 
 * Sets a new password for the ISDN connection, overwriting
 * the previous one.
 **/
void
oobs_iface_isdn_set_password (OobsIfaceISDN *iface,
			      const gchar   *password)
{
  g_return_if_fail (OOBS_IS_IFACE_ISDN (iface));

  g_object_set (G_OBJECT (iface), "password", password, NULL);
}

/**
 * oobs_iface_isdn_set_phone_number:
 * @iface: An #OobsIfaceISDN.
 * @phone_number: a new phone number for the ISDN connection.
 *
 * Sets a new phone number for the ISDN connection, overwriting
 * the previous one.
 **/
void
oobs_iface_isdn_set_phone_number (OobsIfaceISDN *iface,
				  const gchar   *phone_number)
{
  g_return_if_fail (OOBS_IS_IFACE_ISDN (iface));

  g_object_set (G_OBJECT (iface), "phone-number", phone_number, NULL);
}

/**
 * oobs_iface_isdn_get_phone_number:
 * @iface: An #OobsIfaceISDN.
 * 
 * Returns the phone number used for this ISDN connection.
 * 
 * Return Value: A pointer to the phone number. This string must
 *               not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_isdn_get_phone_number (OobsIfaceISDN *iface)
{
  OobsIfaceISDNPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ISDN (iface), NULL);

  priv = iface->_priv;
  return priv->phone_number;
}

/**
 * oobs_iface_isdn_set_phone_prefix:
 * @iface: An #OobsIfaceISDN.
 * @phone_prefix: a new phone number prefix for the ISDN connection.
 * 
 * Sets a new phone number prefix for the ISDN connection,
 * overwriting the previous one.
 **/
void
oobs_iface_isdn_set_phone_prefix (OobsIfaceISDN *iface,
				  const gchar   *phone_prefix)
{
  g_return_if_fail (OOBS_IS_IFACE_ISDN (iface));

  g_object_set (G_OBJECT (iface), "phone-prefix", phone_prefix, NULL);
}

/**
 * oobs_iface_isdn_get_phone_prefix:
 * @iface: An #OobsIfaceISDN.
 * 
 * Returns the phone number prefix used for this ISDN connection.
 * 
 * Return Value: A pointer to the phone number prefix. This string
 *               must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_isdn_get_phone_prefix (OobsIfaceISDN *iface)
{
  OobsIfaceISDNPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ISDN (iface), NULL);

  priv = iface->_priv;
  return priv->dial_prefix;
}

/**
 * oobs_iface_isdn_set_default_gateway:
 * @iface: An #OobsIfaceISDN.
 * @default_gw: #TRUE to make this interface the default gateway to Internet.
 * 
 * Sets whether the interface is the default gateway to internet.
 **/
void
oobs_iface_isdn_set_default_gateway (OobsIfaceISDN *iface,
				     gboolean       default_gw)
{
  g_return_if_fail (OOBS_IS_IFACE_ISDN (iface));

  g_object_set (G_OBJECT (iface), "default-gw", default_gw, NULL);
}

/**
 * oobs_iface_isdn_get_default_gateway:
 * @iface: An #OobsIfaceISDN.
 * 
 * Returns whether the interface is the default gateway to Internet.
 * 
 * Return Value: #TRUE if the interface is the default gateway.
 **/
gboolean
oobs_iface_isdn_get_default_gateway (OobsIfaceISDN *iface)
{
  OobsIfaceISDNPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ISDN (iface), FALSE);

  priv = iface->_priv;
  return priv->default_gw;
}

/**
 * oobs_iface_isdn_set_use_peer_dns:
 * @iface: An #OobsIfaceISDN.
 * @use_peer_dns: #TRUE to use DNS servers specified by the ISP.
 * 
 * Sets whether to use the DNS servers that the ISP specifies.
 **/
void
oobs_iface_isdn_set_use_peer_dns (OobsIfaceISDN *iface,
				  gboolean       use_peer_dns)
{
  g_return_if_fail (OOBS_IS_IFACE_ISDN (iface));

  g_object_set (G_OBJECT (iface), "peer-dns", use_peer_dns, NULL);
}

/**
 * oobs_iface_isdn_get_use_peer_dns:
 * @iface: An #OobsIfaceISDN.
 * 
 * Returns whether the DNS servers specified by the ISP
 * will be used.
 * 
 * Return Value: #TRUE if the DNS servers will be used.
 **/
gboolean
oobs_iface_isdn_get_use_peer_dns (OobsIfaceISDN *iface)
{
  OobsIfaceISDNPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ISDN (iface), FALSE);

  priv = iface->_priv;
  return priv->peerdns;
}

/**
 * oobs_iface_isdn_set_persistent:
 * @iface: An #OobsIfaceISDN.
 * @persistent: #TRUE to try to reconnect if the connection fails.
 * 
 * Sets whether the interface will try to reconnect if the connection fails.
 **/
void
oobs_iface_isdn_set_persistent (OobsIfaceISDN *iface,
				gboolean       persistent)
{
  g_return_if_fail (OOBS_IS_IFACE_ISDN (iface));

  g_object_set (G_OBJECT (iface), "peersistent", persistent, NULL);
}

/**
 * oobs_iface_isdn_get_persistent:
 * @iface: An #OobsIfaceISDN.
 * 
 * Returns whether the interface will try to reconnect if the
 * connection fails.
 * 
 * Return Value: #TRUE if the interface will try to reconnect.
 **/
gboolean
oobs_iface_isdn_get_persistent (OobsIfaceISDN *iface)
{
  OobsIfaceISDNPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ISDN (iface), FALSE);

  priv = iface->_priv;
  return priv->persist;
}

/**
 * oobs_iface_isdn_set_peer_noauth:
 * @iface: An #OobsIfaceISDN.
 * @use_peer_dns: #TRUE if the peer has to authenticate itself.
 * 
 * Sets whether the peer has to authenticate itself.
 **/
void
oobs_iface_isdn_set_peer_noauth (OobsIfaceISDN *iface,
				 gboolean       use_peer_dns)
{
  g_return_if_fail (OOBS_IS_IFACE_ISDN (iface));

  g_object_set (G_OBJECT (iface), "peer-dns", use_peer_dns, NULL);
}

/**
 * oobs_iface_isdn_get_peer_noauth:
 * @iface: An #OobsIfaceISDN.
 * 
 * Returns whether the peer is required to authenticate itself.
 * 
 * Return Value: #TRUE if the peer is required to authenticate itself.
 **/
gboolean
oobs_iface_isdn_get_peer_noauth (OobsIfaceISDN *iface)
{
  OobsIfaceISDNPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_ISDN (iface), FALSE);

  priv = iface->_priv;
  return priv->noauth;
}
