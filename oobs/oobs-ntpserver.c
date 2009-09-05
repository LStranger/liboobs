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
#include "oobs-ntpserver.h"

/**
 * SECTION:oobs-ntpserver
 * @title: OobsNTPServer
 * @short_description: Object that represents an individual NTP server
 * @see_also: #OobsNTPConfig
 **/

#define OOBS_NTP_SERVER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_NTP_SERVER, OobsNTPServerPrivate))

typedef struct _OobsNTPServerPrivate OobsNTPServerPrivate;

struct _OobsNTPServerPrivate {
  gchar *hostname;
};

static void oobs_ntp_server_class_init (OobsNTPServerClass *class);
static void oobs_ntp_server_init       (OobsNTPServer      *ntp_server);
static void oobs_ntp_server_finalize   (GObject            *object);

static void oobs_ntp_server_set_property (GObject      *object,
					  guint         prop_id,
					  const GValue *value,
					  GParamSpec   *pspec);
static void oobs_ntp_server_get_property (GObject      *object,
					  guint         prop_id,
					  GValue       *value,
					  GParamSpec   *pspec);
enum
{
  PROP_0,
  PROP_HOSTNAME,
};

G_DEFINE_TYPE (OobsNTPServer, oobs_ntp_server, G_TYPE_OBJECT);

static void
oobs_ntp_server_class_init (OobsNTPServerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_ntp_server_set_property;
  object_class->get_property = oobs_ntp_server_get_property;
  object_class->finalize     = oobs_ntp_server_finalize;

  g_object_class_install_property (object_class,
				   PROP_HOSTNAME,
				   g_param_spec_string ("hostname",
							"Hostname",
							"Server hostname",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsNTPServerPrivate));
}

static void
oobs_ntp_server_init (OobsNTPServer *ntp_server)
{
  OobsNTPServerPrivate *priv;

  g_return_if_fail (OOBS_IS_NTP_SERVER (ntp_server));

  priv = OOBS_NTP_SERVER_GET_PRIVATE (ntp_server);
  priv->hostname = NULL;
  ntp_server->_priv = priv;
}

static void
oobs_ntp_server_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  OobsNTPServerPrivate *priv;

  g_return_if_fail (OOBS_IS_NTP_SERVER (object));

  priv = OOBS_NTP_SERVER (object)->_priv;

  switch (prop_id)
    {
    case PROP_HOSTNAME:
      g_free (priv->hostname);
      priv->hostname = g_value_dup_string (value);
      break;
    }
}

static void
oobs_ntp_server_get_property (GObject      *object,
			      guint         prop_id,
			      GValue       *value,
			      GParamSpec   *pspec)
{
  OobsNTPServerPrivate *priv;

  g_return_if_fail (OOBS_IS_NTP_SERVER (object));

  priv = OOBS_NTP_SERVER (object)->_priv;

  switch (prop_id)
    {
    case PROP_HOSTNAME:
      g_value_set_string (value, priv->hostname);
      break;
    }
}

static void
oobs_ntp_server_finalize (GObject *object)
{
  OobsNTPServerPrivate *priv;

  g_return_if_fail (OOBS_IS_NTP_SERVER (object));

  priv = OOBS_NTP_SERVER (object)->_priv;

  if (priv)
    g_free (priv->hostname);

  if (G_OBJECT_CLASS (oobs_ntp_server_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_ntp_server_parent_class)->finalize) (object);
}

/**
 * oobs_ntp_server_new:
 * @hostname: Hostname of the NTP server.
 * 
 * Returns a new #OobsNTPServer with hostname set to #hostname.
 * #hostname can specify an IP address too.
 * 
 * Return Value: A new #OobsNTPServer.
 **/
OobsNTPServer*
oobs_ntp_server_new (const gchar *hostname)
{
  return g_object_new (OOBS_TYPE_NTP_SERVER,
		       "hostname", hostname,
		       NULL);
}

/**
 * oobs_ntp_server_get_hostname:
 * @ntp_server: An #OobsNTPServer.
 * 
 * Returns the #OobsNTPServer hostname or IP address.
 * 
 * Return Value: A pointer to the NTP server hostname (or IP address) as a string.
 *               This string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_ntp_server_get_hostname (OobsNTPServer *ntp_server)
{
  OobsNTPServerPrivate *priv;

  g_return_val_if_fail (OOBS_IS_NTP_SERVER (ntp_server), NULL);

  priv = ntp_server->_priv;

  return priv->hostname;
}

/**
 * oobs_ntp_server_set_hostname:
 * @ntp_server: An #OobsNTPServer.
 * @hostname: a new hostname (or IP address) for #ntp_server.
 * 
 * Sets the hostname of #ntp_server to be #hostname.
 * #hostname can specify an IP address too.
 **/
void
oobs_ntp_server_set_hostname (OobsNTPServer *ntp_server, const gchar *hostname)
{
  g_return_if_fail (OOBS_IS_NTP_SERVER (ntp_server));

  g_object_set (G_OBJECT (ntp_server), "hostname", hostname, NULL);
}
