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

#include <dbus/dbus.h>
#include <glib-object.h>
#include "oobs-list-private.h"
#include "oobs-object-private.h"
#include "oobs-ifacesconfig.h"
#include "oobs-iface-ethernet.h"
#include "oobs-iface-wireless.h"
#include "oobs-iface-irlan.h"
#include "oobs-iface-plip.h"
#include "oobs-iface-modem.h"
#include "oobs-iface-isdn.h"
#include "utils.h"

#define IFACES_CONFIG_REMOTE_OBJECT "IfacesConfig"
#define OOBS_IFACES_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACES_CONFIG, OobsIfacesConfigPrivate))

typedef struct _OobsIfacesConfigPrivate OobsIfacesConfigPrivate;

struct _OobsIfacesConfigPrivate
{
  OobsList *ethernet_ifaces;
  OobsList *wireless_ifaces;
  OobsList *irlan_ifaces;
  OobsList *plip_ifaces;
  OobsList *modem_ifaces;
  OobsList *isdn_ifaces;

  GList *available_config_methods;
  GList *available_key_types;
};

static void oobs_ifaces_config_class_init (OobsIfacesConfigClass *class);
static void oobs_ifaces_config_init       (OobsIfacesConfig      *config);
static void oobs_ifaces_config_finalize   (GObject              *object);

static void oobs_ifaces_config_update     (OobsObject   *object);
static void oobs_ifaces_config_commit     (OobsObject   *object);


G_DEFINE_TYPE (OobsIfacesConfig, oobs_ifaces_config, OOBS_TYPE_OBJECT);

GType
oobs_iface_type_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
	{
	  { OOBS_IFACE_TYPE_ETHERNET, "OOBS_IFACE_TYPE_ETHERNET", "ethernet" },
	  { OOBS_IFACE_TYPE_WIRELESS, "OOBS_IFACE_TYPE_WIRELESS", "wireless" },
	  { OOBS_IFACE_TYPE_IRLAN,    "OOBS_IFACE_TYPE_IRLAN",    "infrared" },
	  { OOBS_IFACE_TYPE_PLIP,     "OOBS_IFACE_TYPE_PLIP",     "parallel" },
	  { OOBS_IFACE_TYPE_MODEM,    "OOBS_IFACE_TYPE_MODEM",    "modem" },
	  { OOBS_IFACE_TYPE_ISDN,     "OOBS_IFACE_TYPE_ISDN",     "isdn" },
	};

      etype = g_enum_register_static ("OobsIfaceType", values);
    }

  return etype;
}

static void
oobs_ifaces_config_class_init (OobsIfacesConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize     = oobs_ifaces_config_finalize;
  oobs_object_class->commit  = oobs_ifaces_config_commit;
  oobs_object_class->update  = oobs_ifaces_config_update;

  g_type_class_add_private (object_class,
			    sizeof (OobsIfacesConfigPrivate));
}

static void
oobs_ifaces_config_init (OobsIfacesConfig *config)
{
  OobsIfacesConfigPrivate *priv;

  priv = OOBS_IFACES_CONFIG_GET_PRIVATE (config);

  priv->ethernet_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_ETHERNET);
  priv->wireless_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_WIRELESS);
  priv->irlan_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_IRLAN);
  priv->plip_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_PLIP);
  priv->modem_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_MODEM);
  priv->isdn_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_ISDN);
  config->_priv = priv;
}

static void
free_configuration (OobsIfacesConfig *config)
{
  OobsIfacesConfigPrivate *priv;

  priv = config->_priv;

  oobs_list_clear (priv->ethernet_ifaces);
  oobs_list_clear (priv->wireless_ifaces);
  oobs_list_clear (priv->irlan_ifaces);
  oobs_list_clear (priv->plip_ifaces);
  oobs_list_clear (priv->modem_ifaces);
  oobs_list_clear (priv->isdn_ifaces);

  g_list_foreach (priv->available_config_methods, (GFunc) g_free, NULL);
  g_list_free (priv->available_config_methods);

  g_list_foreach (priv->available_key_types, (GFunc) g_free, NULL);
  g_list_free (priv->available_key_types);
}

static void
oobs_ifaces_config_finalize (GObject *object)
{
  OobsIfacesConfigPrivate *priv;

  priv = OOBS_IFACES_CONFIG (object)->_priv;

  if (priv)
    {
      free_configuration (OOBS_IFACES_CONFIG (object));
      g_object_unref (priv->ethernet_ifaces);
      g_object_unref (priv->wireless_ifaces);
      g_object_unref (priv->irlan_ifaces);
      g_object_unref (priv->plip_ifaces);
      g_object_unref (priv->modem_ifaces);
      g_object_unref (priv->isdn_ifaces);
    }

  if (G_OBJECT_CLASS (oobs_ifaces_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_ifaces_config_parent_class)->finalize) (object);
}

GObject*
create_iface_from_message (DBusMessage     *message,
			   DBusMessageIter *iter,
			   gint             type)
{
  GObject *iface = NULL; /* shut up gcc */
  DBusMessageIter struct_iter;
  const gchar *dev;
  gboolean active, is_auto;

  dbus_message_iter_recurse (iter, &struct_iter);

  dev = utils_get_string (&struct_iter);
  dbus_message_iter_next (&struct_iter);

  switch (type)
    {
    case OOBS_IFACE_TYPE_ETHERNET:
      iface = g_object_new (OOBS_TYPE_IFACE_ETHERNET, "device", dev, NULL);
      break;
    case OOBS_IFACE_TYPE_WIRELESS:
      iface = g_object_new (OOBS_TYPE_IFACE_WIRELESS, "device", dev, NULL);
      break;
    case OOBS_IFACE_TYPE_IRLAN:
      iface = g_object_new (OOBS_TYPE_IFACE_IRLAN, "device", dev, NULL);
      break;
    case OOBS_IFACE_TYPE_PLIP:
      iface = g_object_new (OOBS_TYPE_IFACE_PLIP, "device", dev, NULL);
      break;
    case OOBS_IFACE_TYPE_MODEM:
      iface = g_object_new (OOBS_TYPE_IFACE_MODEM, "device", dev, NULL);
      break;
    case OOBS_IFACE_TYPE_ISDN:
      iface = g_object_new (OOBS_TYPE_IFACE_ISDN, "device", dev, NULL);
      break;
    }

  if (OOBS_IS_IFACE_ETHERNET (iface))
    {
      const gchar *address, *netmask, *gateway, *config_method;

      dbus_message_iter_get_basic (&struct_iter, &is_auto);
      dbus_message_iter_next (&struct_iter);

      dbus_message_iter_get_basic (&struct_iter, &active);
      dbus_message_iter_next (&struct_iter);

      /* This value is deprecated */
      dbus_message_iter_next (&struct_iter);

      address = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      netmask = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      /* FIXME: missing network and broadcast */
      dbus_message_iter_next (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      gateway = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      g_object_set (iface,
		    "auto", is_auto,
		    "active", active,
		    "ip-address", address,
		    "ip-mask", netmask,
		    "gateway-address", gateway,
		    NULL);

      if (type == OOBS_IFACE_TYPE_WIRELESS)
	{
	  const gchar *essid, *key, *key_type;

	  essid = utils_get_string (&struct_iter);
	  dbus_message_iter_next (&struct_iter);

	  /* This value is deprecated */
	  dbus_message_iter_next (&struct_iter);

	  key = utils_get_string (&struct_iter);
	  dbus_message_iter_next (&struct_iter);

	  key_type = utils_get_string (&struct_iter);
	  dbus_message_iter_next (&struct_iter);

	  g_object_set (iface,
			"essid", essid,
			"key_type", key_type,
			"key", key,
			NULL);
	}

      config_method = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);
      g_object_set (iface, "config-method", config_method, NULL);
    }
  else if (OOBS_IS_IFACE_PLIP (iface))
    {
      const gchar *address, *remote_address;

      dbus_message_iter_get_basic (&struct_iter, &is_auto);
      dbus_message_iter_next (&struct_iter);

      dbus_message_iter_get_basic (&struct_iter, &active);
      dbus_message_iter_next (&struct_iter);

      address = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      remote_address = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      g_object_set (iface,
		    "auto", is_auto,
		    "active", active,
		    "address", address,
		    "remote-address", remote_address,
		    NULL);
    }
  else if (OOBS_IS_IFACE_ISDN (iface))
    {
      const gchar *phone_number, *phone_prefix, *login, *password;
      gboolean default_gw, peer_dns, persistent, noauth;

      dbus_message_iter_get_basic (&struct_iter, &is_auto);
      dbus_message_iter_next (&struct_iter);

      dbus_message_iter_get_basic (&struct_iter, &active);
      dbus_message_iter_next (&struct_iter);

      phone_number = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      phone_prefix = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      if (OOBS_IS_IFACE_MODEM (iface))
	{
	  const gchar *serial_port;
	  gint volume, dial_type;

	  serial_port = utils_get_string (&struct_iter);
	  dbus_message_iter_next (&struct_iter);

	  dbus_message_iter_get_basic (&struct_iter, &volume);
	  dbus_message_iter_next (&struct_iter);

	  dbus_message_iter_get_basic (&struct_iter, &dial_type);
	  dbus_message_iter_next (&struct_iter);

	  g_object_set (iface,
			"serial-port", serial_port,
			"volume", volume,
			"dial-type", dial_type,
			NULL);
	}

      login = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      password = utils_get_string (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      dbus_message_iter_get_basic (&struct_iter, &default_gw);
      dbus_message_iter_next (&struct_iter);

      dbus_message_iter_get_basic (&struct_iter, &peer_dns);
      dbus_message_iter_next (&struct_iter);

      dbus_message_iter_get_basic (&struct_iter, &persistent);
      dbus_message_iter_next (&struct_iter);

      dbus_message_iter_get_basic (&struct_iter, &noauth);
      dbus_message_iter_next (&struct_iter);

      g_object_set (iface,
		    "auto", is_auto,
		    "active", active,
		    "login", login,
		    "password", password,
		    "phone-number", phone_number,
		    "phone-prefix", phone_prefix,
		    "default-gw", default_gw,
		    "peer-dns", peer_dns,
		    "persistent", persistent,
		    "peer-noauth", noauth,
		    NULL);
    }

  /* FIXME: missing properties */
  return iface;
}

static void
create_ifaces_list (DBusMessage     *reply,
		    DBusMessageIter *iter,
		    OobsIfaceType    type,
		    OobsList        *list)
{
  GObject *iface;
  OobsListIter list_iter;
  DBusMessageIter elem_iter;

  dbus_message_iter_recurse (iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      iface = create_iface_from_message (reply, &elem_iter, type);

      oobs_list_append (list, &list_iter);
      oobs_list_set (list, &list_iter, iface);
      g_object_unref (iface);

      dbus_message_iter_next (&elem_iter);
    }
}

static void
oobs_ifaces_config_update (OobsObject *object)
{
  OobsIfacesConfigPrivate *priv;
  DBusMessage *reply;
  DBusMessageIter iter;

  priv = OOBS_IFACES_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous configuration */
  free_configuration (OOBS_IFACES_CONFIG (object));

  dbus_message_iter_init (reply, &iter);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_ETHERNET, priv->ethernet_ifaces);

  dbus_message_iter_next (&iter);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_WIRELESS, priv->wireless_ifaces);

  dbus_message_iter_next (&iter);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_IRLAN, priv->irlan_ifaces);

  dbus_message_iter_next (&iter);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_PLIP, priv->plip_ifaces);

  dbus_message_iter_next (&iter);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_MODEM, priv->modem_ifaces);

  dbus_message_iter_next (&iter);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_ISDN, priv->isdn_ifaces);

  dbus_message_iter_next (&iter);
  priv->available_config_methods = utils_get_string_list_from_dbus_reply (reply, iter);

  dbus_message_iter_next (&iter);
  priv->available_key_types = utils_get_string_list_from_dbus_reply (reply, iter);
}

static void
create_dbus_struct_from_iface (DBusMessage     *message,
			       DBusMessageIter *array_iter,
			       OobsIface       *iface)
{
  DBusMessageIter iter;
  gchar *dev;
  gboolean configured, active, is_auto;

  g_object_get (G_OBJECT (iface),
		"device", &dev,
		"configured", &configured,
		"auto", &is_auto,
		"active", &active,
		NULL);

  dbus_message_iter_open_container (array_iter, DBUS_TYPE_STRUCT, NULL, &iter);

  utils_append_string (&iter, dev);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &active);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &is_auto);

  if (OOBS_IS_IFACE_ETHERNET (iface))
    {
      gchar *address, *netmask, *gateway;
      gint config_method;

      g_object_get (G_OBJECT (iface),
		    "ip-address", &address,
		    "ip-mask", &netmask,
		    "gateway-address", &gateway,
		    "config-method", &config_method,
		    NULL);

      /* FIXME: can pass config_method like that? */
      dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &config_method);
      utils_append_string (&iter, (configured) ? address : NULL);
      utils_append_string (&iter, (configured) ? netmask : NULL);

      /* FIXME */
      utils_append_string (&iter, NULL);
      utils_append_string (&iter, NULL);

      utils_append_string (&iter, (configured) ? gateway : NULL);

      if (OOBS_IS_IFACE_WIRELESS (iface))
	{
	  gchar *essid, *key;
	  gint key_type;

	  g_object_get (G_OBJECT (iface),
			"essid", &essid,
			"key-type", &key_type,
			"key", &key,
			NULL);

	  utils_append_string (&iter, (configured) ? essid : NULL);
	  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &key_type);
	  utils_append_string (&iter, (configured) ? key : NULL);

	  g_free (essid);
	  g_free (key);
	}

      g_free (address);
      g_free (netmask);
      g_free (gateway);
    }
  else if (OOBS_IS_IFACE_PLIP (iface))
    {
      gchar *address, *remote_address;

      g_object_get (G_OBJECT (iface),
		    "address", &address,
		    "remote-address", &remote_address,
		    NULL);

      utils_append_string (&iter, (configured) ? address : NULL);
      utils_append_string (&iter, (configured) ? remote_address : NULL);

      g_free (address);
      g_free (remote_address);
    }
  else if (OOBS_IS_IFACE_ISDN (iface))
    {
      gchar *phone_number, *prefix, *login, *password;
      gboolean default_gw, peer_dns, persistent, noauth;

      g_object_get (G_OBJECT (iface),
		    "login", &login,
		    "password", &password,
		    "phone-number", &phone_number,
		    "phone-prefix", &prefix,
		    "default-gw", &default_gw,
		    "peer-dns", &peer_dns,
		    "persistent", &persistent,
		    "peer-noauth", &noauth,
		    NULL);

      utils_append_string (&iter, (configured) ? phone_number : NULL);
      utils_append_string (&iter, (configured) ? prefix : NULL);

      if (OOBS_IS_IFACE_MODEM (iface))
	{
	  gchar *serial_port;
	  gint volume, dial_type;

	  g_object_get (G_OBJECT (iface),
			"serial-port", &serial_port,
			"volume", &volume,
			"dial-type", &dial_type,
			NULL);

	  utils_append_string (&iter, (configured) ? serial_port : NULL);
	  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &volume);
	  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &dial_type);

	  g_free (serial_port);
	}

      utils_append_string (&iter, (configured) ? login : NULL);
      utils_append_string (&iter, (configured) ? password : NULL);
      dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &default_gw);
      dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &peer_dns);
      dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &persistent);
      dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &noauth);

      g_free (phone_number);
      g_free (prefix);
      g_free (login);
      g_free (password);
    }

  dbus_message_iter_close_container (array_iter, &iter);
  g_free (dev);
}

static void
create_dbus_struct_from_ifaces_list (OobsObject      *object,
				     DBusMessage     *message,
				     DBusMessageIter *iter,
				     OobsList        *list,
				     OobsIfaceType    type)
{
  OobsListIter list_iter;
  DBusMessageIter array_iter;
  GObject *iface;
  gboolean valid;
  const gchar *signature;

  switch (type)
    {
    case OOBS_IFACE_TYPE_ETHERNET:
    case OOBS_IFACE_TYPE_IRLAN:
      signature =
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_STRUCT_END_CHAR_AS_STRING;
      break;
    case OOBS_IFACE_TYPE_WIRELESS:
      signature =
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_STRUCT_END_CHAR_AS_STRING;
      break;
    case OOBS_IFACE_TYPE_PLIP:
      signature =
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_STRUCT_END_CHAR_AS_STRING;
      break;
    case OOBS_IFACE_TYPE_MODEM:
      signature =
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_STRUCT_END_CHAR_AS_STRING;
      break;
    case OOBS_IFACE_TYPE_ISDN:
      signature =
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_STRUCT_END_CHAR_AS_STRING;
      break;
    default:
      g_assert_not_reached ();
    }

  dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, signature, &array_iter);
  valid = oobs_list_get_iter_first (list, &list_iter);

  while (valid)
    {
      iface = oobs_list_get (list, &list_iter);
      create_dbus_struct_from_iface (message, &array_iter, OOBS_IFACE (iface));
      g_object_unref (iface);

      valid = oobs_list_iter_next (list, &list_iter);
    }

  dbus_message_iter_close_container (iter, &array_iter);
}

static void
oobs_ifaces_config_commit (OobsObject *object)
{
  OobsIfacesConfigPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter;

  priv = OOBS_IFACES_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);

  create_dbus_struct_from_ifaces_list (object, message, &iter, priv->ethernet_ifaces, OOBS_IFACE_TYPE_ETHERNET);
  create_dbus_struct_from_ifaces_list (object, message, &iter, priv->wireless_ifaces, OOBS_IFACE_TYPE_WIRELESS);
  create_dbus_struct_from_ifaces_list (object, message, &iter, priv->irlan_ifaces, OOBS_IFACE_TYPE_IRLAN);
  create_dbus_struct_from_ifaces_list (object, message, &iter, priv->plip_ifaces, OOBS_IFACE_TYPE_PLIP);
  create_dbus_struct_from_ifaces_list (object, message, &iter, priv->modem_ifaces, OOBS_IFACE_TYPE_MODEM);
  create_dbus_struct_from_ifaces_list (object, message, &iter, priv->isdn_ifaces, OOBS_IFACE_TYPE_ISDN);
}

/**
 * oobs_ifaces_config_get:
 * @session: An #OobsSession.
 * 
 * Returns the #OobsIfacesConfig singleton, which represents
 * the network interfaces and their configuration.
 * 
 * Return Value: the singleton #OobsIfacesConfig object.
 **/
OobsObject*
oobs_ifaces_config_get (OobsSession *session)
{
  static OobsObject *object = NULL;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  if (!object)
    {
      object = g_object_new (OOBS_TYPE_IFACES_CONFIG,
			     "session", session,
			     "remote-object", IFACES_CONFIG_REMOTE_OBJECT,
			     NULL);
      oobs_object_update (object);
    }

  return object;
}

/**
 * oobs_ifaces_config_get_ifaces:
 * @config: An #OobsIfacesConfig.
 * @type: An #OobsIfaceType.
 * 
 * Returns an #OobsList containing the interfaces that match the
 * type defined by @type.
 * 
 * Return Value: An #OobsList, you must not unref this object.
 **/
OobsList*
oobs_ifaces_config_get_ifaces (OobsIfacesConfig *config,
			       OobsIfaceType     type)
{
  OobsIfacesConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACES_CONFIG (config), NULL);

  priv = config->_priv;

  switch (type)
    {
    case OOBS_IFACE_TYPE_ETHERNET:
      return priv->ethernet_ifaces;
    case OOBS_IFACE_TYPE_WIRELESS:
      return priv->wireless_ifaces;
    case OOBS_IFACE_TYPE_IRLAN:
      return priv->irlan_ifaces;
    case OOBS_IFACE_TYPE_PLIP:
      return priv->plip_ifaces;
    case OOBS_IFACE_TYPE_MODEM:
      return priv->modem_ifaces;
    case OOBS_IFACE_TYPE_ISDN:
      return priv->isdn_ifaces;
    default:
      g_critical ("Unknown interface type");
      return NULL;
    }
}

/**
 * oobs_ifaces_config_get_available_configuration_methods:
 * @config: An #OobsIfaceConfig.
 * 
 * Retrieves the list of available configuration methods for ethernet
 * based interfaces.
 * 
 * Return Value: A #GList of strings. This must not be modified or freed.
 **/
GList*
oobs_ifaces_config_get_available_configuration_methods (OobsIfacesConfig *config)
{
  OobsIfacesConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACES_CONFIG (config), NULL);

  priv = config->_priv;
  return priv->available_config_methods;
}

/**
 * oobs_ifaces_config_get_available_key_types:
 * @config: An #OobsIfaceConfig.
 * 
 * Retrieves the list of available key types methods for wireless
 * interfaces.
 * 
 * Return Value: A #GList of strings. This must not be modified or freed.
 **/
GList*
oobs_ifaces_config_get_available_key_types (OobsIfacesConfig *config)
{
  OobsIfacesConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACES_CONFIG (config), NULL);

  priv = config->_priv;
  return priv->available_key_types;
}
