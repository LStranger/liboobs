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
#include <string.h>

#ifdef HAVE_HAL
#include <libhal.h>
#endif

#include "oobs-session-private.h"
#include "oobs-list-private.h"
#include "oobs-object-private.h"
#include "oobs-ifacesconfig.h"
#include "oobs-iface-ethernet.h"
#include "oobs-iface-wireless.h"
#include "oobs-iface-irlan.h"
#include "oobs-iface-plip.h"
#include "oobs-iface-ppp.h"
#include "iface-state-monitor.h"
#include "utils.h"
#include "config.h"

/**
 * SECTION:oobs-ifacesconfig
 * @title: OobsIfacesConfig
 * @short_description: Object that represents network interfaces configuration
 * @see_also: #OobsIface, #OobsIfaceEthernet, #OobsIfaceIRLan,
 *     #OobsIfacePlip, #OobsIfacePPP, #OobsIfaceWireless
 **/

#define IFACES_CONFIG_REMOTE_OBJECT "IfacesConfig"
#define OOBS_IFACES_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACES_CONFIG, OobsIfacesConfigPrivate))

typedef struct _OobsIfacesConfigPrivate OobsIfacesConfigPrivate;

struct _OobsIfacesConfigPrivate
{
  OobsList *ethernet_ifaces;
  OobsList *wireless_ifaces;
  OobsList *irlan_ifaces;
  OobsList *plip_ifaces;
  OobsList *ppp_ifaces;

  GList *available_config_methods;
  GList *available_key_types;
  GList *available_ppp_types;

  GHashTable *ifaces;

#ifdef HAVE_HAL
  LibHalContext *hal_context;
  GHashTable    *devices;
#endif
};

static void oobs_ifaces_config_class_init (OobsIfacesConfigClass *class);
static void oobs_ifaces_config_init       (OobsIfacesConfig      *config);
static void oobs_ifaces_config_finalize   (GObject              *object);

static void oobs_ifaces_config_update     (OobsObject   *object);
static void oobs_ifaces_config_commit     (OobsObject   *object);


G_DEFINE_TYPE (OobsIfacesConfig, oobs_ifaces_config, OOBS_TYPE_OBJECT);

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
oobs_ifaces_config_iface_monitor (OobsIfacesConfig *config,
				  const gchar      *iface_name,
				  gboolean          iface_active)
{
  OobsIfacesConfigPrivate *priv;
  OobsIface *iface;

  priv = config->_priv;
  iface = g_hash_table_lookup (priv->ifaces, iface_name);

  if (!iface)
    return;

  g_return_if_fail (OOBS_IS_IFACE (iface));

  if (iface_active != oobs_iface_get_active (iface))
    {
      oobs_iface_set_active (iface, iface_active);
      g_signal_emit_by_name (iface, "state-changed");
    }
}

#ifdef HAVE_HAL

static void
hal_context_device_added (LibHalContext *context,
			  const gchar   *udi)
{
  OobsIfacesConfig *config;
  OobsIfacesConfigPrivate *priv;

  config = libhal_ctx_get_user_data (context);
  priv = OOBS_IFACES_CONFIG_GET_PRIVATE (config);

  g_return_if_fail (OOBS_IS_IFACES_CONFIG (config));

  if (libhal_device_query_capability (context, udi, "net", NULL))
    {
      g_hash_table_insert (priv->devices, g_strdup (udi), GINT_TO_POINTER (TRUE));
      g_signal_emit_by_name (config, "changed");
    }
}

static void
hal_context_device_removed (LibHalContext *context,
			    const gchar   *udi)
{
  OobsIfacesConfig *config;
  OobsIfacesConfigPrivate *priv;

  config = libhal_ctx_get_user_data (context);
  priv = OOBS_IFACES_CONFIG_GET_PRIVATE (config);

  g_return_if_fail (OOBS_IS_IFACES_CONFIG (config));

  if (g_hash_table_lookup (priv->devices, udi))
    {
      g_hash_table_remove (priv->devices, udi);
      g_signal_emit_by_name (config, "changed");
    }
}

static GHashTable*
hal_context_get_initial_devices (LibHalContext *context)
{
  GHashTable *devices;
  gint i, n_devices;
  gchar **udis;
  DBusError error;

  devices = g_hash_table_new_full (g_str_hash, g_str_equal,
				   (GDestroyNotify) g_free, NULL);

  dbus_error_init(&error);
  udis = libhal_find_device_by_capability (context, "net", &n_devices, &error);

  /* In case an error occurred, udis would be NULL, possibly leading to crash */
  if (dbus_error_is_set (&error)) {
    g_warning ("Could not find any network device on the system: %s.",
               error.message);
    return devices;
  }

  for (i = 0; i < n_devices; i++)
    g_hash_table_insert (devices, g_strdup (udis[i]), GINT_TO_POINTER (TRUE));

  libhal_free_string_array (udis);

  return devices;
}

static void
init_hal_context (OobsIfacesConfig *config)
{
  OobsIfacesConfigPrivate *priv;
  OobsSession *session;
  DBusConnection *connection;
  DBusError error;

  priv = OOBS_IFACES_CONFIG_GET_PRIVATE (config);

  dbus_error_init (&error);
  session = oobs_session_get ();
  connection = _oobs_session_get_connection_bus (session);

  priv->hal_context = libhal_ctx_new ();
  libhal_ctx_set_dbus_connection (priv->hal_context, connection);

  libhal_ctx_set_user_data (priv->hal_context, config);

  libhal_ctx_set_device_added (priv->hal_context,
			       hal_context_device_added);
  libhal_ctx_set_device_removed (priv->hal_context,
			       hal_context_device_removed);

  libhal_ctx_init (priv->hal_context, &error);

  if (dbus_error_is_set (&error))
    {
      g_warning (error.message);
      return;
    }

  priv->devices = hal_context_get_initial_devices (priv->hal_context);
}

#endif

static void
oobs_ifaces_config_init (OobsIfacesConfig *config)
{
  OobsIfacesConfigPrivate *priv;

  priv = OOBS_IFACES_CONFIG_GET_PRIVATE (config);

#ifdef HAVE_HAL
  init_hal_context (config);
#endif

  priv->ethernet_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_ETHERNET);
  priv->wireless_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_WIRELESS);
  priv->irlan_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_IRLAN);
  priv->plip_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_PLIP);
  priv->ppp_ifaces = _oobs_list_new (OOBS_TYPE_IFACE_PPP);
  config->_priv = priv;

  iface_state_monitor_init (config, oobs_ifaces_config_iface_monitor);
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
  oobs_list_clear (priv->ppp_ifaces);

  g_list_foreach (priv->available_config_methods, (GFunc) g_free, NULL);
  g_list_free (priv->available_config_methods);

  g_list_foreach (priv->available_key_types, (GFunc) g_free, NULL);
  g_list_free (priv->available_key_types);

  g_list_foreach (priv->available_ppp_types, (GFunc) g_free, NULL);
  g_list_free (priv->available_ppp_types);

  if (priv->ifaces)
    {
      g_hash_table_destroy (priv->ifaces);
      priv->ifaces = NULL;
    }
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
      g_object_unref (priv->ppp_ifaces);

#ifdef HAVE_HAL
      libhal_ctx_free (priv->hal_context);
#endif
    }

  if (G_OBJECT_CLASS (oobs_ifaces_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_ifaces_config_parent_class)->finalize) (object);
}

GObject*
create_iface_from_message (DBusMessage     *message,
			   DBusMessageIter *iter,
			   gint             type,
			   GHashTable      *ifaces)
{
  GObject *iface = NULL; /* shut up gcc */
  DBusMessageIter struct_iter;
  const gchar *dev;
  gboolean active, is_auto;

  dbus_message_iter_recurse (iter, &struct_iter);

  dev = utils_get_string (&struct_iter);
  active = utils_get_int (&struct_iter);
  is_auto = utils_get_int (&struct_iter);

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
    case OOBS_IFACE_TYPE_PPP:
      iface = g_object_new (OOBS_TYPE_IFACE_PPP, "device", dev, NULL);
      break;
    }

  if (OOBS_IS_IFACE_ETHERNET (iface))
    {
      const gchar *address, *netmask, *gateway, *config_method;

      /* This value is deprecated */
      dbus_message_iter_next (&struct_iter);

      address = utils_get_string (&struct_iter);
      netmask = utils_get_string (&struct_iter);

      /* FIXME: missing network and broadcast */
      dbus_message_iter_next (&struct_iter);
      dbus_message_iter_next (&struct_iter);

      gateway = utils_get_string (&struct_iter);

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

	  /* This value is deprecated */
	  dbus_message_iter_next (&struct_iter);

	  key = utils_get_string (&struct_iter);
	  key_type = utils_get_string (&struct_iter);

	  g_object_set (iface,
			"essid", essid,
			"key_type", key_type,
			"key", key,
			NULL);
	}

      config_method = utils_get_string (&struct_iter);
      g_object_set (iface, "config-method", config_method, NULL);
    }
  else if (OOBS_IS_IFACE_PLIP (iface))
    {
      const gchar *address, *remote_address;

      address = utils_get_string (&struct_iter);
      remote_address = utils_get_string (&struct_iter);

      g_object_set (iface,
		    "auto", is_auto,
		    "active", active,
		    "address", address,
		    "remote-address", remote_address,
		    NULL);
    }
  else if (OOBS_IS_IFACE_PPP (iface))
    {
      const gchar *phone_number, *phone_prefix, *login, *password;
      const gchar *device, *connection_type, *apn;
      gboolean default_gw, peer_dns, persistent, noauth;
      gint volume, dial_type;

      connection_type = utils_get_string (&struct_iter);

      phone_number = utils_get_string (&struct_iter);
      phone_prefix = utils_get_string (&struct_iter);

      device = utils_get_string (&struct_iter);
      volume = utils_get_int (&struct_iter);
      dial_type = utils_get_int (&struct_iter);

      login = utils_get_string (&struct_iter);
      password = utils_get_string (&struct_iter);

      default_gw = utils_get_int (&struct_iter);
      peer_dns = utils_get_int (&struct_iter);
      persistent = utils_get_int (&struct_iter);
      noauth = utils_get_int (&struct_iter);

      apn = utils_get_string (&struct_iter);

      if (connection_type &&
	  strcmp (connection_type, "pppoe") == 0)
	{
	  OobsIface *ethernet;

	  /* in pppoe configuration, the device
	   * contains the ethernet interface name
	   */
	  ethernet = g_hash_table_lookup (ifaces, device);
	  g_object_set (iface, "ethernet", ethernet, NULL);
	}
      else
	g_object_set (iface, "serial-port", device, NULL);

      g_object_set (iface,
		    "auto", is_auto,
		    "active", active,
		    "connection-type", connection_type,
		    "login", login,
		    "password", password,
		    "phone-number", phone_number,
		    "phone-prefix", phone_prefix,
		    "default-gateway", default_gw,
		    "use-peer-dns", peer_dns,
		    "persistent", persistent,
		    "peer-noauth", noauth,
		    "volume", volume,
		    "dial-type", dial_type,
		    "apn", apn,
		    NULL);
    }

  /* FIXME: missing properties */
  return iface;
}

static void
create_ifaces_list (DBusMessage     *reply,
		    DBusMessageIter *iter,
		    OobsIfaceType    type,
		    OobsList        *list,
		    GHashTable      *ifaces)
{
  GObject *iface;
  OobsListIter list_iter;
  DBusMessageIter elem_iter;
  const gchar *name;

  dbus_message_iter_recurse (iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      iface = create_iface_from_message (reply, &elem_iter, type, ifaces);

      oobs_list_append (list, &list_iter);
      oobs_list_set (list, &list_iter, iface);

      name = oobs_iface_get_device_name (OOBS_IFACE (iface));
      g_hash_table_insert (ifaces, (gpointer) name, iface);

      g_object_unref (iface);
      dbus_message_iter_next (&elem_iter);
    }

  dbus_message_iter_next (iter);
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

  priv->ifaces = g_hash_table_new (g_str_hash, g_str_equal);

  dbus_message_iter_init (reply, &iter);

  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_ETHERNET, priv->ethernet_ifaces, priv->ifaces);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_WIRELESS, priv->wireless_ifaces, priv->ifaces);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_IRLAN, priv->irlan_ifaces, priv->ifaces);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_PLIP, priv->plip_ifaces, priv->ifaces);
  create_ifaces_list (reply, &iter, OOBS_IFACE_TYPE_PPP, priv->ppp_ifaces, priv->ifaces);

  priv->available_config_methods = utils_get_string_list_from_dbus_reply (reply, &iter);
  priv->available_key_types = utils_get_string_list_from_dbus_reply (reply, &iter);
  priv->available_ppp_types = utils_get_string_list_from_dbus_reply (reply, &iter);
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
  utils_append_int (&iter, active);
  utils_append_int (&iter, is_auto);

  if (OOBS_IS_IFACE_ETHERNET (iface))
    {
      gchar *address, *netmask, *gateway, *config_method;

      g_object_get (G_OBJECT (iface),
		    "ip-address", &address,
		    "ip-mask", &netmask,
		    "gateway-address", &gateway,
		    "config-method", &config_method,
		    NULL);

      /* This field is deprecated */
      utils_append_int (&iter, 0);
      utils_append_string (&iter, (configured) ? address : NULL);
      utils_append_string (&iter, (configured) ? netmask : NULL);

      /* FIXME */
      utils_append_string (&iter, NULL);
      utils_append_string (&iter, NULL);

      utils_append_string (&iter, (configured) ? gateway : NULL);

      if (OOBS_IS_IFACE_WIRELESS (iface))
	{
	  gchar *essid, *key, *key_type;

	  g_object_get (G_OBJECT (iface),
			"essid", &essid,
			"key-type", &key_type,
			"key", &key,
			NULL);

	  utils_append_string (&iter, (configured) ? essid : NULL);

	  /* This field is deprecated */
	  utils_append_int (&iter, 0);
	  utils_append_string (&iter, (configured) ? key : NULL);
	  utils_append_string (&iter, (configured) ? key_type : NULL);

	  g_free (key_type);
	  g_free (essid);
	  g_free (key);
	}

      utils_append_string (&iter, (configured) ? config_method : NULL);

      g_free (address);
      g_free (netmask);
      g_free (gateway);
      g_free (config_method);
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
  else if (OOBS_IS_IFACE_PPP (iface))
    {
      gchar *phone_number, *prefix, *login, *password;
      gchar *serial_port, *connection_type, *apn;
      gboolean default_gw, peer_dns, persistent, noauth;
      gint volume, dial_type;
      OobsIface *ethernet;

      g_object_get (G_OBJECT (iface),
		    "connection-type", &connection_type,
		    "login", &login,
		    "password", &password,
		    "phone-number", &phone_number,
		    "phone-prefix", &prefix,
		    "default-gateway", &default_gw,
		    "use-peer-dns", &peer_dns,
		    "persistent", &persistent,
		    "peer-noauth", &noauth,
		    "serial-port", &serial_port,
		    "volume", &volume,
		    "dial-type", &dial_type,
		    "ethernet", &ethernet,
		    "apn", &apn,
		    NULL);

      utils_append_string (&iter, (configured) ? connection_type : NULL);

      utils_append_string (&iter, (configured) ? phone_number : NULL);
      utils_append_string (&iter, (configured) ? prefix : NULL);

      if (connection_type &&
	  strcmp (connection_type, "pppoe") == 0)
	{
	  if (ethernet)
	    utils_append_string (&iter, oobs_iface_get_device_name (ethernet));
	  else
	    utils_append_string (&iter, NULL);
	}
      else
	utils_append_string (&iter, (configured) ? serial_port : NULL);

      utils_append_int (&iter, volume);
      utils_append_int (&iter, dial_type);

      utils_append_string (&iter, (configured) ? login : NULL);
      utils_append_string (&iter, (configured) ? password : NULL);
      utils_append_int (&iter, default_gw);
      utils_append_int (&iter, peer_dns);
      utils_append_int (&iter, persistent);
      utils_append_int (&iter, noauth);
      utils_append_string (&iter, (configured) ? apn : NULL);

      if (ethernet)
	g_object_unref (ethernet);

      g_free (connection_type);
      g_free (phone_number);
      g_free (prefix);
      g_free (login);
      g_free (password);
      g_free (serial_port);
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
  const gchar *signature = NULL;

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
	DBUS_TYPE_STRING_AS_STRING
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
    case OOBS_IFACE_TYPE_PPP:
      signature =
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_INT32_AS_STRING
	DBUS_TYPE_STRING_AS_STRING
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
	DBUS_TYPE_STRING_AS_STRING
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
  create_dbus_struct_from_ifaces_list (object, message, &iter, priv->ppp_ifaces, OOBS_IFACE_TYPE_PPP);
}

/**
 * oobs_ifaces_config_get:
 * 
 * Returns the #OobsIfacesConfig singleton, which represents
 * the network interfaces and their configuration.
 * 
 * Return Value: the singleton #OobsIfacesConfig object.
 **/
OobsObject*
oobs_ifaces_config_get (void)
{
  static OobsObject *the_object = NULL;

  if (!the_object)
    the_object = g_object_new (OOBS_TYPE_IFACES_CONFIG,
                               "remote-object", IFACES_CONFIG_REMOTE_OBJECT,
                               NULL);

  return the_object;
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
    case OOBS_IFACE_TYPE_PPP:
      return priv->ppp_ifaces;
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

/**
 * oobs_ifaces_config_get_available_ppp_types:
 * @config: An #OobsIfaceConfig.
 *
 * Retrieves the list of available PPP interface types.
 *
 * Return Value: A #GList of strings. This must not be modified or freed.
 **/
GList*
oobs_ifaces_config_get_available_ppp_types (OobsIfacesConfig *config)
{
  OobsIfacesConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACES_CONFIG (config), NULL);

  priv = config->_priv;
  return priv->available_ppp_types;
}
