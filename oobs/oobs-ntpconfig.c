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
#include "oobs-ntpconfig.h"
#include "oobs-list-private.h"
#include "oobs-ntpserver.h"
#include "utils.h"

/**
 * SECTION:oobs-ntpconfig
 * @title: OobsNTPConfig
 * @short_description: Object that represents NTP configuration
 * @see_also: #OobsNTPServer, #OobsTimeConfig
 **/

#define NTP_CONFIG_REMOTE_OBJECT "NTPConfig"
#define OOBS_NTP_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_NTP_CONFIG, OobsNTPConfigPrivate))

typedef struct _OobsNTPConfigPrivate OobsNTPConfigPrivate;

struct _OobsNTPConfigPrivate
{
  OobsList *servers_list;
};

static void oobs_ntp_config_class_init (OobsNTPConfigClass *class);
static void oobs_ntp_config_init       (OobsNTPConfig      *config);
static void oobs_ntp_config_finalize   (GObject            *object);

static void oobs_ntp_config_update     (OobsObject   *object);
static void oobs_ntp_config_commit     (OobsObject   *object);


G_DEFINE_TYPE (OobsNTPConfig, oobs_ntp_config, OOBS_TYPE_OBJECT);


static void
oobs_ntp_config_class_init (OobsNTPConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize     = oobs_ntp_config_finalize;

  oobs_object_class->commit  = oobs_ntp_config_commit;
  oobs_object_class->update  = oobs_ntp_config_update;

  g_type_class_add_private (object_class,
			    sizeof (OobsNTPConfigPrivate));
}

static void
oobs_ntp_config_init (OobsNTPConfig *config)
{
  OobsNTPConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_NTP_CONFIG (config));

  priv = OOBS_NTP_CONFIG_GET_PRIVATE (config);

  priv->servers_list = _oobs_list_new (OOBS_TYPE_NTP_SERVER);
  config->_priv = priv;
}

static void
oobs_ntp_config_finalize (GObject *object)
{
  OobsNTPConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_NTP_CONFIG (object));

  priv = OOBS_NTP_CONFIG (object)->_priv;

  if (priv && priv->servers_list)
    g_object_unref (priv->servers_list);

  if (G_OBJECT_CLASS (oobs_ntp_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_ntp_config_parent_class)->finalize) (object);
}

static void
oobs_ntp_config_update (OobsObject *object)
{
  OobsNTPConfigPrivate *priv;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;
  OobsListIter     list_iter;
  GObject         *ntp_server;
  const gchar     *server;

  g_return_if_fail (OOBS_IS_NTP_CONFIG (object));

  priv  = OOBS_NTP_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous list */
  oobs_list_clear (priv->servers_list);

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRING)
    {
      server = utils_get_string (&elem_iter);
      ntp_server = G_OBJECT (oobs_ntp_server_new (server));

      oobs_list_append (priv->servers_list, &list_iter);
      oobs_list_set    (priv->servers_list, &list_iter, G_OBJECT (ntp_server));
      g_object_unref   (ntp_server);
    }
}

static void
oobs_ntp_config_commit (OobsObject *object)
{
  OobsNTPConfigPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter, array_iter;
  OobsListIter list_iter;
  GObject *ntp_server;
  const gchar *hostname;
  gboolean valid;

  priv = OOBS_NTP_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);
  dbus_message_iter_open_container (&iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_TYPE_STRING_AS_STRING,
				    &array_iter);
  valid = oobs_list_get_iter_first (priv->servers_list, &list_iter);

  while (valid)
    {
      ntp_server = oobs_list_get (priv->servers_list, &list_iter);
      hostname = oobs_ntp_server_get_hostname (OOBS_NTP_SERVER (ntp_server));

      utils_append_string (&array_iter, hostname);
      g_object_unref (ntp_server);
      valid = oobs_list_iter_next (priv->servers_list, &list_iter);
    }

  dbus_message_iter_close_container (&iter, &array_iter);
}

/**
 * oobs_ntp_config_get:
 * 
 * Returns the #OobsNTPConfig singleton, which represents
 * the NTP (Network time servers) system configuration.
 * 
 * Return Value: the singleton #OobsNTPConfig object.
 **/
OobsObject*
oobs_ntp_config_get (void)
{
  static OobsObject *the_object = NULL;

  if (!the_object)
    the_object = g_object_new (OOBS_TYPE_NTP_CONFIG,
                               "remote-object", NTP_CONFIG_REMOTE_OBJECT,
                               NULL);

  return the_object;
}

/**
 * oobs_ntp_config_get_servers:
 * @config: An #OobsNTPConfig.
 * 
 * Returns an #OobsList containing objects of type #OobsNTPServer.
 * 
 * Return Value: an #OobsList containing the NTP servers information.
 **/
OobsList*
oobs_ntp_config_get_servers (OobsNTPConfig *config)
{
  OobsNTPConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_NTP_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->servers_list;
}
