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
#include "oobs-servicesconfig.h"
#include "oobs-service.h"

#define SERVICES_CONFIG_REMOTE_OBJECT "ServicesConfig"
#define OOBS_SERVICES_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SERVICES_CONFIG, OobsServicesConfigPrivate))

typedef struct _OobsServicesConfigPrivate OobsServicesConfigPrivate;

struct _OobsServicesConfigPrivate
{
  OobsList *services_list;
};

static void oobs_services_config_class_init (OobsServicesConfigClass *class);
static void oobs_services_config_init       (OobsServicesConfig      *config);
static void oobs_services_config_finalize   (GObject                 *object);

static void oobs_services_config_update     (OobsObject   *object);
static void oobs_services_config_commit     (OobsObject   *object);


G_DEFINE_TYPE (OobsServicesConfig, oobs_services_config, OOBS_TYPE_OBJECT);

static void
oobs_services_config_class_init (OobsServicesConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize    = oobs_services_config_finalize;
  oobs_object_class->commit = oobs_services_config_commit;
  oobs_object_class->update = oobs_services_config_update;

  g_type_class_add_private (object_class,
			    sizeof (OobsServicesConfigPrivate));
}

static void
oobs_services_config_init (OobsServicesConfig *config)
{
  OobsServicesConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SERVICES_CONFIG (config));

  priv = OOBS_SERVICES_CONFIG_GET_PRIVATE (config);

  priv->services_list = _oobs_list_new (OOBS_TYPE_SERVICE);
}

static void
oobs_services_config_finalize (GObject *object)
{
  OobsServicesConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SERVICES_CONFIG (object));

  priv = OOBS_SERVICES_CONFIG_GET_PRIVATE (object);

  if (priv && priv->services_list)
    g_object_unref (priv->services_list);

  if (G_OBJECT_CLASS (oobs_services_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_services_config_parent_class)->finalize) (object);
}

static OobsService*
create_service_from_dbus_reply (DBusMessage     *reply,
				DBusMessageIter  struct_iter)
{
  DBusMessageIter iter;
  gchar *name, *role;

  dbus_message_iter_recurse (&struct_iter, &iter);

  dbus_message_iter_get_basic (&iter, &name);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &role);
  dbus_message_iter_next (&iter);

  /* FIXME: missing runlevels, priorities, etc... */

  return oobs_service_new (name, role);
}

static void
oobs_services_config_update (OobsObject *object)
{
  OobsServicesConfigPrivate *priv;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;
  OobsListIter     list_iter;
  OobsService     *service;

  priv  = OOBS_SERVICES_CONFIG_GET_PRIVATE (object);
  reply = _oobs_object_get_dbus_message (object);

  _oobs_list_set_locked (priv->services_list, FALSE);

  /* First of all, free the previous config */
  oobs_list_clear (priv->services_list);

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      service = create_service_from_dbus_reply (reply, elem_iter);

      oobs_list_append (priv->services_list, &list_iter);
      oobs_list_set    (priv->services_list, &list_iter, G_OBJECT (service));
      g_object_unref   (service);

      dbus_message_iter_next (&elem_iter);
    }

  _oobs_list_set_locked (priv->services_list, TRUE);
}

static void
oobs_services_config_commit (OobsObject *object)
{
}

OobsObject*
oobs_services_config_get (OobsSession *session)
{
  static OobsObject *object = NULL;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  if (!object)
    {
      object = g_object_new (OOBS_TYPE_SERVICES_CONFIG,
			     "remote-object", SERVICES_CONFIG_REMOTE_OBJECT,
			     "session",       session,
			     NULL);

      oobs_object_update (object);
    }

  return object;
}

OobsList*
oobs_services_config_get_services (OobsServicesConfig *config)
{
  OobsServicesConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SERVICES_CONFIG (config), NULL);

  priv = OOBS_SERVICES_CONFIG_GET_PRIVATE (config);

  return priv->services_list;
}
