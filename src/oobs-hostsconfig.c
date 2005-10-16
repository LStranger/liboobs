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

#define HOSTS_CONFIG_REMOTE_OBJECT "HostsConfig"
#define OOBS_HOSTS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_HOSTS_CONFIG, OobsHostsConfigPrivate))

typedef struct _OobsHostsConfigPrivate OobsHostsConfigPrivate;

struct _OobsHostsConfigPrivate
{
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
}

static void
free_configuration (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  priv = OOBS_HOSTS_CONFIG_GET_PRIVATE (config);

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

  priv = OOBS_HOSTS_CONFIG_GET_PRIVATE (object);

  if (priv)
    {
      free_configuration (OOBS_HOSTS_CONFIG (object));

      if (priv->static_hosts_list)
	g_object_unref (priv->static_hosts_list);
    }

  if (G_OBJECT_CLASS (oobs_hosts_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_hosts_config_parent_class)->finalize) (object);
}

static GList*
get_string_list_from_dbus_reply (DBusMessage     *reply,
				 DBusMessageIter  iter)
{
  DBusMessageIter elem_iter;
  GList *l = NULL;
  gchar *elem;

  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRING)
    {
      dbus_message_iter_get_basic (&elem_iter, &elem);
      l = g_list_prepend (l, g_strdup (elem));
      dbus_message_iter_next (&elem_iter);
    }

  return g_list_reverse (l);
}

static OobsStaticHost*
create_static_host_from_dbus_reply (DBusMessage     *reply,
				    DBusMessageIter  iter)
{
  DBusMessageIter elem_iter;
  gchar *ip_address;
  GList *aliases;

  dbus_message_iter_recurse (&iter, &elem_iter);

  dbus_message_iter_get_basic (&elem_iter, &ip_address);
  dbus_message_iter_next (&elem_iter);

  aliases = get_string_list_from_dbus_reply (reply, elem_iter);
  dbus_message_iter_next (&elem_iter);

  return oobs_static_host_new (ip_address, aliases);
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

  priv = OOBS_HOSTS_CONFIG_GET_PRIVATE (object);
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

  priv = OOBS_HOSTS_CONFIG_GET_PRIVATE (object);
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous config */
  free_configuration (OOBS_HOSTS_CONFIG (object));

  dbus_message_iter_init (reply, &iter);

  get_static_hosts_list_from_dbus_reply (object, reply, iter);
  dbus_message_iter_next (&iter);

  priv->dns_list = get_string_list_from_dbus_reply (reply, iter);
  dbus_message_iter_next (&iter);

  priv->search_domains_list = get_string_list_from_dbus_reply (reply, iter);
  dbus_message_iter_next (&iter);
}

static void
oobs_hosts_config_commit (OobsObject *object)
{
}

OobsObject*
oobs_hosts_config_new (OobsSession *session)
{
  static OobsObject *object = NULL;

  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  if (!object)
    {
      object = g_object_new (OOBS_TYPE_HOSTS_CONFIG,
			     "remote-object", HOSTS_CONFIG_REMOTE_OBJECT,
			     "session",       session,
			     NULL);

      oobs_object_update (object);
    }

  return object;
}

OobsList*
oobs_hosts_config_get_static_hosts (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_HOSTS_CONFIG (config), NULL);

  priv = OOBS_HOSTS_CONFIG_GET_PRIVATE (config);

  return priv->static_hosts_list;
}

GList*
oobs_hosts_config_get_dns_servers (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_HOSTS_CONFIG (config), NULL);

  priv = OOBS_HOSTS_CONFIG_GET_PRIVATE (config);

  return priv->dns_list;
}

GList*
oobs_hosts_config_get_search_domains (OobsHostsConfig *config)
{
  OobsHostsConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_HOSTS_CONFIG (config), NULL);

  priv = OOBS_HOSTS_CONFIG_GET_PRIVATE (config);

  return priv->search_domains_list;
}
