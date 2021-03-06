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

#include <glib-object.h>
#include "oobs-statichost.h"

/**
 * SECTION:oobs-statichost
 * @title: OobsStaticHost
 * @short_description: Object that represents an individual static host settings
 * @see_also: #OobsHostsConfig
 **/

#define OOBS_STATIC_HOST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_STATIC_HOST, OobsStaticHostPrivate))

typedef struct _OobsStaticHostPrivate OobsStaticHostPrivate;

struct _OobsStaticHostPrivate {
  gchar *ip_address;
  GList *aliases;
};

static void oobs_static_host_class_init (OobsStaticHostClass *class);
static void oobs_static_host_init       (OobsStaticHost      *group);
static void oobs_static_host_finalize   (GObject             *object);

static void oobs_static_host_set_property (GObject      *object,
					   guint         prop_id,
					   const GValue *value,
					   GParamSpec   *pspec);
static void oobs_static_host_get_property (GObject      *object,
					   guint         prop_id,
					   GValue       *value,
					   GParamSpec   *pspec);
enum {
  PROP_0,
  PROP_IP_ADDRESS,
};

G_DEFINE_TYPE (OobsStaticHost, oobs_static_host, G_TYPE_OBJECT);

static void
oobs_static_host_class_init (OobsStaticHostClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_static_host_set_property;
  object_class->get_property = oobs_static_host_get_property;
  object_class->finalize     = oobs_static_host_finalize;

  g_object_class_install_property (object_class,
				   PROP_IP_ADDRESS,
				   g_param_spec_string ("ip-address",
							"IP address",
							"IP address of the static host definition",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsStaticHostPrivate));
}

static void
oobs_static_host_init (OobsStaticHost *group)
{
  OobsStaticHostPrivate *priv;

  g_return_if_fail (OOBS_IS_STATIC_HOST (group));

  priv = OOBS_STATIC_HOST_GET_PRIVATE (group);
  priv->ip_address = NULL;
  priv->aliases    = NULL;
  group->_priv     = priv;
}

static void
oobs_static_host_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  OobsStaticHostPrivate *priv;

  g_return_if_fail (OOBS_IS_STATIC_HOST (object));

  priv = OOBS_STATIC_HOST (object)->_priv;

  switch (prop_id)
    {
    case PROP_IP_ADDRESS:
      g_free (priv->ip_address);
      priv->ip_address = g_value_dup_string (value);
      break;
    }
}

static void
oobs_static_host_get_property (GObject      *object,
			       guint         prop_id,
			       GValue       *value,
			       GParamSpec   *pspec)
{
  OobsStaticHostPrivate *priv;

  g_return_if_fail (OOBS_IS_STATIC_HOST (object));

  priv = OOBS_STATIC_HOST (object)->_priv;

  switch (prop_id)
    {
    case PROP_IP_ADDRESS:
      g_value_set_string (value, priv->ip_address);
      break;
    }
}

static void
oobs_static_host_finalize (GObject *object)
{
  OobsStaticHostPrivate *priv;

  g_return_if_fail (OOBS_IS_STATIC_HOST (object));

  priv = OOBS_STATIC_HOST (object)->_priv;

  if (priv)
    {
      g_free (priv->ip_address);

      g_list_foreach (priv->aliases, (GFunc) g_free, NULL);
      g_list_free (priv->aliases);
    }

  if (G_OBJECT_CLASS (oobs_static_host_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_static_host_parent_class)->finalize) (object);
}

/**
 * oobs_static_host_new:
 * @ip_address: IP address for the static host.
 * @aliases: #GList of aliases to #ip_address.
 * 
 * Returns a new #OobsStaticHost defining both the IP address
 * and the list of hostnames that will point to the IP address.
 * 
 * Return Value: A new #OobsStaticHost.
 **/
OobsStaticHost*
oobs_static_host_new (const gchar *ip_address,
		      GList       *aliases)
{
  OobsStaticHost *static_host;

  static_host = g_object_new (OOBS_TYPE_STATIC_HOST,
			      "ip-address", ip_address,
			      NULL);

  oobs_static_host_set_aliases (static_host, aliases);

  return static_host;
}

/**
 * oobs_static_host_get_ip_address:
 * @static_host: An #OobsStaticHost.
 * 
 * Returns the static host IP address.
 * 
 * Return Value: A pointer to the static host IP address as a string.
 *               This string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_static_host_get_ip_address (OobsStaticHost *static_host)
{
  OobsStaticHostPrivate *priv;

  g_return_val_if_fail (OOBS_IS_STATIC_HOST (static_host), NULL);

  priv = static_host->_priv;

  return priv->ip_address;
}

/**
 * oobs_static_host_set_ip_address:
 * @static_host: An #OobsStaticHost.
 * @ip_address: A new IP address for #static_host
 * 
 * Sets the IP address of #static_host to be #ip_address,
 * overwriting the previous one.
 **/
void
oobs_static_host_set_ip_address (OobsStaticHost *static_host,
				 const gchar    *ip_address)
{
  /* FIXME: should check ip address validity */

  g_return_if_fail (OOBS_IS_STATIC_HOST (static_host));

  g_object_set (static_host, "ip-address", ip_address, NULL);
}

/**
 * oobs_static_host_get_aliases:
 * @static_host: An #OobsStaticHost.
 * 
 * Returns the hostname aliases for the #static_host IP address.
 * The returned list must be freed with g_list_free().
 * 
 * Return Value: A #GList of gchar pointers containing the host aliases.
 **/
GList*
oobs_static_host_get_aliases (OobsStaticHost *static_host)
{
  OobsStaticHostPrivate *priv;

  g_return_val_if_fail (OOBS_IS_STATIC_HOST (static_host), NULL);

  priv = static_host->_priv;

  return g_list_copy (priv->aliases);
}

/**
 * oobs_static_host_set_aliases:
 * @static_host: An #OobsStaticHost.
 * @aliases: a #GList of gchar pointers containing the host aliases.
 * 
 * Sets a new list of aliases for the #static_host
 * IP address. overwriting the previous one.
 **/
void
oobs_static_host_set_aliases (OobsStaticHost *static_host, GList *aliases)
{
  OobsStaticHostPrivate *priv;

  g_return_if_fail (OOBS_IS_STATIC_HOST (static_host));

  priv = static_host->_priv;

  if (priv->aliases)
    {
      g_list_foreach (priv->aliases, (GFunc) g_free, NULL);
      g_list_free (priv->aliases);
    }

  priv->aliases = aliases;
}
