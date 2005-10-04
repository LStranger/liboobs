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

#define HOSTS_CONFIG_REMOTE_OBJECT "HostsConfig"
#define OOBS_HOSTS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_HOSTS_CONFIG, OobsHostsConfigPrivate))

typedef struct _OobsHostsConfigPrivate OobsHostsConfigPrivate;

struct _OobsHostsConfigPrivate
{
  OobsList *hosts_list;
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

  /* FIXME: not just a objects list */
  priv->hosts_list = _oobs_list_new (G_TYPE_OBJECT);
}

static void
oobs_hosts_config_finalize (GObject *object)
{
  OobsHostsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_HOSTS_CONFIG (object));

  priv = OOBS_HOSTS_CONFIG_GET_PRIVATE (object);

  if (priv && priv->hosts_list)
    g_object_unref (priv->hosts_list);

  if (G_OBJECT_CLASS (oobs_hosts_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_hosts_config_parent_class)->finalize) (object);
}

static void
oobs_hosts_config_update (OobsObject *object)
{
  DBusMessage *reply;

  reply = _oobs_object_get_dbus_message (object);
}

static void
oobs_hosts_config_commit (OobsObject *object)
{
}

OobsObject*
oobs_hosts_config_new (OobsSession *session)
{
  OobsObject *object;

  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  object = g_object_new (OOBS_TYPE_HOSTS_CONFIG,
			 "remote-object", HOSTS_CONFIG_REMOTE_OBJECT,
			 "session",       session,
			 NULL);

  oobs_object_update (object);

  return object;
}

OobsList*
oobs_hosts_config_get_static_hosts (OobsHostsConfig *config)
{
  return NULL;
}

OobsList*
oobs_hosts_config_get_dns_servers (OobsHostsConfig *config)
{
  return NULL;
}

OobsList*
oobs_hosts_config_get_search_domains (OobsHostsConfig *config)
{
  return NULL;
}
