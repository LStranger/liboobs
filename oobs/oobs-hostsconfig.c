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
#include "oobs-object.h"
#include "oobs-object-private.h"
#include "oobs-list.h"
#include "oobs-list-private.h"
#include "oobs-hostsconfig.h"
#include "oobs-statichost.h"
#include "utils.h"

#define HOSTS_CONFIG_REMOTE_OBJECT "HostsConfig"
#define OOBS_HOSTS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_HOSTS_CONFIG, OobsHostsConfigPrivate))

typedef struct _OobsHostsConfigPrivate OobsHostsConfigPrivate;

struct _OobsHostsConfigPrivate
{
  gchar *hostname;
  gchar *domain;

  OobsList *static_hosts_list;
  GList *dns_list;
  GList *search_domains_list;
};

static void oobs_hosts_config_class_init (OobsHostsConfigClass *class);
static void oobs_hosts_config_init       (OobsHostsConfig      *groups_list);
static void oobs_hosts_config_finalize   (GObject              *object);

static void oobs_hosts_config_update     (OobsObject   *object);
static void oobs_hosts_config_commit     (OobsObject   *object);


G_DEFINE_TYPE (OobsHostsConfig, oobs_hosts_config, OOBS_TYPE_OBJECT);

static void
oobs_hosts_config_class_init (OobsHostsConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize    = oobs_hosts_config_finalize;
  oobs_object_class->commit = oobs_hosts_config_commit;
  oobs_object_class->update = oobs_hosts_config_update;

  g_type_class_add_private (object_class,
			    sizeof (OobsHostsConfigPrivate));
}

static void
oobs_hosts_config_init (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_HOSTS_CONFIG (config));

  priv = OOBS_HOSTS_CONFIG_GET_PRIVATE (config);

  priv->static_hosts_list = _oobs_list_new (OOBS_TYPE_STATIC_HOST);
  config->_priv = priv;
}

static void
free_configuration (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  priv = config->_priv;

  if (priv->hostname)
    {
      g_free (priv->hostname);
      priv->hostname = NULL;
    }

  if (priv->domain)
    {
      g_free (priv->domain);
      priv->domain = NULL;
    }

  if (priv->static_hosts_list)
    oobs_list_clear (priv->static_hosts_list);

  if (priv->dns_list)
    {
      g_list_foreach (priv->dns_list, (GFunc) g_free, NULL);
      g_list_free (priv->dns_list);
      priv->dns_list = NULL;
    }

  if (priv->search_domains_list)
    {
      g_list_foreach (priv->search_domains_list, (GFunc) g_free, NULL);
      g_list_free (priv->search_domains_list);
      priv->search_domains_list = NULL;
    }
}

static void
oobs_hosts_config_finalize (GObject *object)
{
  OobsHostsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_HOSTS_CONFIG (object));

  priv = OOBS_HOSTS_CONFIG (object)->_priv;

  if (priv)
    {
      free_configuration (OOBS_HOSTS_CONFIG (object));

      if (priv->static_hosts_list)
	g_object_unref (priv->static_hosts_list);
    }

  if (G_OBJECT_CLASS (oobs_hosts_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_hosts_config_parent_class)->finalize) (object);
}

static OobsStaticHost*
create_static_host_from_dbus_reply (DBusMessage     *reply,
				    DBusMessageIter  iter)
{
  DBusMessageIter elem_iter;
  const gchar *ip_address;
  GList *aliases;

  dbus_message_iter_recurse (&iter, &elem_iter);

  ip_address = utils_get_string (&elem_iter);
  aliases = utils_get_string_list_from_dbus_reply (reply, &elem_iter);

  return oobs_static_host_new (ip_address, aliases);
}

static void
create_dbus_struct_from_static_host (OobsStaticHost  *host,
				     DBusMessage     *message,
				     DBusMessageIter *iter)
{
  DBusMessageIter struct_iter;
  const gchar *ip_address;
  GList *aliases;

  ip_address = oobs_static_host_get_ip_address (host);
  aliases = oobs_static_host_get_aliases (host);

  dbus_message_iter_open_container (iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);
  utils_append_string (&struct_iter, ip_address);
  utils_create_dbus_array_from_string_list (aliases, message, &struct_iter);
  dbus_message_iter_close_container (iter, &struct_iter);

  g_list_free (aliases);
}

static void
get_static_hosts_list_from_dbus_reply (OobsObject      *object,
				       DBusMessage     *reply,
				       DBusMessageIter  iter)
{
  OobsHostsConfigPrivate *priv;
  DBusMessageIter struct_iter;
  OobsStaticHost *static_host;
  OobsListIter     list_iter;

  priv = OOBS_HOSTS_CONFIG (object)->_priv;
  dbus_message_iter_recurse (&iter, &struct_iter);

  while (dbus_message_iter_get_arg_type (&struct_iter) == DBUS_TYPE_STRUCT)
    {
      static_host = create_static_host_from_dbus_reply (reply, struct_iter);

      oobs_list_append (priv->static_hosts_list, &list_iter);
      oobs_list_set    (priv->static_hosts_list, &list_iter, G_OBJECT (static_host));
      g_object_unref   (static_host);

      dbus_message_iter_next (&struct_iter);
    }
}

static void
oobs_hosts_config_update (OobsObject *object)
{
  OobsHostsConfigPrivate *priv;
  DBusMessage *reply;
  DBusMessageIter iter;

  priv = OOBS_HOSTS_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous config */
  free_configuration (OOBS_HOSTS_CONFIG (object));

  dbus_message_iter_init (reply, &iter);

  priv->hostname = g_strdup (utils_get_string (&iter));
  priv->domain = g_strdup (utils_get_string (&iter));

  get_static_hosts_list_from_dbus_reply (object, reply, iter);
  dbus_message_iter_next (&iter);

  priv->dns_list = utils_get_string_list_from_dbus_reply (reply, &iter);
  priv->search_domains_list = utils_get_string_list_from_dbus_reply (reply, &iter);
}

static void
oobs_hosts_config_commit (OobsObject *object)
{
  OobsHostsConfigPrivate *priv;
  GObject *host;
  DBusMessage *message;
  DBusMessageIter iter, array_iter;
  OobsListIter list_iter;
  gboolean valid;

  priv = OOBS_HOSTS_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);

  utils_append_string (&iter, priv->hostname);
  utils_append_string (&iter, priv->domain);

  dbus_message_iter_open_container (&iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_ARRAY_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING
				    DBUS_TYPE_ARRAY_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_ARRAY_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING,
				    &array_iter);
  valid  = oobs_list_get_iter_first (priv->static_hosts_list, &list_iter);

  while (valid)
    {
      host = oobs_list_get (priv->static_hosts_list, &list_iter);
      create_dbus_struct_from_static_host (OOBS_STATIC_HOST (host), message, &array_iter);

      g_object_unref (host);
      valid = oobs_list_iter_next (priv->static_hosts_list, &list_iter);
    }

  dbus_message_iter_close_container (&iter, &array_iter);

  utils_create_dbus_array_from_string_list (priv->dns_list, message, &iter);
  utils_create_dbus_array_from_string_list (priv->search_domains_list, message, &iter);
}

/**
 * oobs_hosts_config_get:
 * 
 * Returns the #OobsHostsConfig singleton, this object
 * represents the hosts resolution configuration.
 * 
 * Return Value: the singleton #OobsHostsConfig.
 **/
OobsObject*
oobs_hosts_config_get (void)
{
  return g_object_new (OOBS_TYPE_HOSTS_CONFIG,
		       "remote-object", HOSTS_CONFIG_REMOTE_OBJECT,
		       NULL);
}

/**
 * oobs_hosts_config_get_hostname:
 * @config: An #OobsHostsConfig.
 * 
 * Returns the hostname for the machine.
 * 
 * Return Value: A pointer to the hostname as a string. This
 * string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_hosts_config_get_hostname (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_HOSTS_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->hostname;
}

/**
 * oobs_hosts_config_set_hostname:
 * @config: An #OobsHostsConfig.
 * @hostname: A new hostname.
 * 
 * Sets a new hostname, overwriting the previous one.
 **/
void
oobs_hosts_config_set_hostname (OobsHostsConfig *config,
				const gchar     *hostname)
{
  OobsHostsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_HOSTS_CONFIG (config));
  g_return_if_fail (hostname && *hostname);

  priv = config->_priv;

  if (priv->hostname)
    g_free (priv->hostname);

  priv->hostname = g_strdup (hostname);
}

/**
 * oobs_hosts_config_get_domainname:
 * @config: An #OobsHostsConfig.
 * 
 * Returns the domain name for the machine.
 * 
 * Return Value: A pointer to the domain name as a string. This
 * string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_hosts_config_get_domainname (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_HOSTS_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->domain;
}

/**
 * oobs_hosts_config_set_domainname:
 * @config: An #OobsHostsConfig.
 * @domainname: A new domain name.
 * 
 * Sets a new domain name, overwriting the previous one.
 **/
void
oobs_hosts_config_set_domainname (OobsHostsConfig *config,
				  const gchar     *domainname)
{
  OobsHostsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_HOSTS_CONFIG (config));

  priv = config->_priv;

  if (priv->domain)
    g_free (priv->domain);

  priv->domain = (domainname && *domainname) ? g_strdup (domainname) : NULL;
}

/**
 * oobs_hosts_config_get_static_hosts:
 * @config: An #OobsHostsConfig.
 * 
 * Returns an #OobsList containing objects of type #OobsStaticHost.
 * 
 * Return Value: An #OobsList containing the static hosts configuration.
 **/
OobsList*
oobs_hosts_config_get_static_hosts (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_HOSTS_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->static_hosts_list;
}

/**
 * oobs_hosts_config_get_dns_servers:
 * @config: An #OobsHostsConfig.
 * 
 * Returns a #GList containing the IP addresses of the DNS servers.
 * The returned list must be freed with g_list_free().
 * 
 * Return Value: a #GList of gchar pointers
 **/
GList*
oobs_hosts_config_get_dns_servers (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_HOSTS_CONFIG (config), NULL);

  priv = config->_priv;

  return g_list_copy (priv->dns_list);
}

/**
 * oobs_hosts_config_set_dns_servers:
 * @config: An #OobsHostsConfig.
 * @dns_list: a #GList containing strings with the IP addresses of the DNS servers.
 * 
 * Overwrites the list of DNS servers. The previous list and its contents will
 * be freed, so any merging will have to be done by hand.
 **/
void
oobs_hosts_config_set_dns_servers (OobsHostsConfig *config,
				   GList           *dns_list)
{
  OobsHostsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_HOSTS_CONFIG (config));

  priv = config->_priv;

  if (priv->dns_list)
    {
      g_list_foreach (priv->dns_list, (GFunc) g_free, NULL);
      g_list_free (priv->dns_list);
    }

  priv->dns_list = dns_list;
}

/**
 * oobs_hosts_config_get_search_domains:
 * @config: An #OobsHostsConfig.
 * 
 * Returns a #GList containing the search domains.
 * The returned list musts be freed with g_list_free().
 * 
 * Return Value: a #GList of gchar pointers.
 **/
GList*
oobs_hosts_config_get_search_domains (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_HOSTS_CONFIG (config), NULL);

  priv = config->_priv;

  return g_list_copy (priv->search_domains_list);
}

/**
 * oobs_hosts_config_set_search_domains:
 * @config: An #OobsHostsConfig
 * @search_domains_list: a #GList containing strings with the search domains.
 * 
 * Overwrites the list of search domains. The previous list and its contents will
 * be freed, so any merging will have to be done by hand.
 **/
void
oobs_hosts_config_set_search_domains (OobsHostsConfig *config,
				      GList           *search_domains_list)
{
  OobsHostsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_HOSTS_CONFIG (config));

  priv = config->_priv;

  if (priv->search_domains_list)
    {
      g_list_foreach (priv->search_domains_list, (GFunc) g_free, NULL);
      g_list_free (priv->search_domains_list);
    }

  priv->search_domains_list = search_domains_list;
}
