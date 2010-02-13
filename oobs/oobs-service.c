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

#include "oobs-service.h"
#include "oobs-object-private.h"
#include "oobs-service-private.h"
#include "oobs-servicesconfig-private.h"
#include "utils.h"

/**
 * SECTION:oobs-service
 * @title: OobsService
 * @short_description: Object that represents an individual init.d service
 * @see_also: #OobsServicesConfig
 **/

#define SERVICE_REMOTE_OBJECT "ServiceConfig2"
#define OOBS_SERVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SERVICE, OobsServicePrivate))

typedef struct _OobsServicePrivate  OobsServicePrivate;
typedef struct _OobsServiceRunlevel OobsServiceRunlevel;

struct _OobsServiceRunlevel {
  guint status;
  guint priority;
};
	
struct _OobsServicePrivate {
  OobsServicesConfig *config;
  gchar *name;
  GHashTable *runlevels_config;
};

static void oobs_service_class_init (OobsServiceClass *class);
static void oobs_service_init       (OobsService      *service);
static void oobs_service_finalize   (GObject          *object);

static void oobs_service_set_property (GObject      *object,
				       guint         prop_id,
				       const GValue *value,
				       GParamSpec   *pspec);
static void oobs_service_get_property (GObject      *object,
				       guint         prop_id,
				       GValue       *value,
				       GParamSpec   *pspec);

static void oobs_service_commit             (OobsObject *object);
static void oobs_service_update             (OobsObject *object);
static void oobs_service_get_update_message (OobsObject *object);

enum
{
  PROP_0,
  PROP_NAME
};

G_DEFINE_TYPE (OobsService, oobs_service, OOBS_TYPE_OBJECT);

static void
oobs_service_class_init (OobsServiceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->set_property = oobs_service_set_property;
  object_class->get_property = oobs_service_get_property;
  object_class->finalize     = oobs_service_finalize;

  oobs_object_class->commit              = oobs_service_commit;
  oobs_object_class->update              = oobs_service_update;
  oobs_object_class->get_update_message  = oobs_service_get_update_message;

  g_object_class_install_property (object_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
							"Service name",
							"Name of the service",
							NULL,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_type_class_add_private (object_class,
			    sizeof (OobsServicePrivate));
}

static void
oobs_service_init (OobsService *service)
{
  OobsServicePrivate *priv;

  g_return_if_fail (OOBS_IS_SERVICE (service));

  priv = OOBS_SERVICE_GET_PRIVATE (service);

  priv->config = OOBS_SERVICES_CONFIG (oobs_services_config_get ());
  priv->name = NULL;
  priv->runlevels_config = g_hash_table_new_full (NULL, NULL, NULL,
						  (GDestroyNotify) g_free);
  service->_priv = priv;
}

static void
oobs_service_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  OobsServicePrivate *priv;

  priv = OOBS_SERVICE (object)->_priv;

  switch (prop_id)
    {
    case PROP_NAME:
      g_free (priv->name);
      priv->name  = g_value_dup_string (value);
      break;
    }
}

static void
oobs_service_get_property (GObject      *object,
			   guint         prop_id,
			   GValue       *value,
			   GParamSpec   *pspec)
{
  OobsServicePrivate *priv;

  priv = OOBS_SERVICE (object)->_priv;

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    }
}

static void
oobs_service_finalize (GObject *object)
{
  OobsServicePrivate *priv;

  priv = OOBS_SERVICE (object)->_priv;

  if (priv)
    {
      g_free (priv->name);
      g_hash_table_unref (priv->runlevels_config);
    }

  if (G_OBJECT_CLASS (oobs_service_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_service_parent_class)->finalize) (object);
}

static void
oobs_service_commit (OobsObject *object)
{
  OobsServicePrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter;
  gboolean correct;
  GList *runlevels;

  correct = TRUE;
  priv = OOBS_SERVICE (object)->_priv;
  message = _oobs_object_get_dbus_message (object);
  dbus_message_iter_init_append (message, &iter);

  runlevels = oobs_services_config_get_runlevels (priv->config);
  correct = _oobs_create_dbus_struct_from_service (OOBS_SERVICE (object),
                                                   runlevels,
                                                   message, &iter);
  g_list_free (runlevels);

  if (!correct)
    {
      /* malformed data, unset the message */
      _oobs_object_set_dbus_message (object, NULL);
    }
}

/*
 * We need a custom update message containing the service name.
 */
static void
oobs_service_get_update_message (OobsObject *object)
{
  OobsServicePrivate *priv;
  DBusMessageIter iter;
  DBusMessage *message;

  priv = OOBS_SERVICE (object)->_priv;

  message = _oobs_object_get_dbus_message (object);
  dbus_message_iter_init_append (message, &iter);

  utils_append_string (&iter, priv->name);
}

static void
oobs_service_update (OobsObject *object)
{
  OobsServicePrivate *priv;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;

  priv  = OOBS_SERVICE (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init (reply, &iter);

  _oobs_service_create_from_dbus_reply (OOBS_SERVICE (object),
                                        reply, elem_iter);
}

static void
create_service_runlevels_from_dbus_reply (OobsService        *service,
                                          DBusMessage        *reply,
                                          DBusMessageIter     struct_iter)
{
  OobsServicePrivate *priv;
  DBusMessageIter runlevel_iter;
  OobsServicesRunlevel *rl;
  OobsServiceStatus status;
  const gchar *runlevel;
  gint priority;

  priv = service->_priv;

  while (dbus_message_iter_get_arg_type (&struct_iter) == DBUS_TYPE_STRUCT)
    {
      dbus_message_iter_recurse (&struct_iter, &runlevel_iter);

      runlevel = utils_get_string (&runlevel_iter);
      status = utils_get_int (&runlevel_iter);
      priority = utils_get_int (&runlevel_iter);

      rl = _oobs_services_config_get_runlevel (priv->config, runlevel);

      if (rl)
	oobs_service_set_runlevel_configuration (service, rl, status, priority);

      dbus_message_iter_next (&struct_iter);
    }
}

OobsService*
_oobs_service_create_from_dbus_reply (OobsService        *service,
                                      DBusMessage        *reply,
                                      DBusMessageIter     struct_iter)
{
  DBusMessageIter iter, runlevels_iter;
  const gchar *name;

  dbus_message_iter_recurse (&struct_iter, &iter);

  name = utils_get_string (&iter);

  if (!service)
    service = g_object_new (OOBS_TYPE_SERVICE,
                            "remote-object", SERVICE_REMOTE_OBJECT,
                            "name", name,
                            NULL);

  dbus_message_iter_recurse (&iter, &runlevels_iter);
  create_service_runlevels_from_dbus_reply (OOBS_SERVICE (service),
					    reply, runlevels_iter);
  return service;
}


static void
create_dbus_struct_from_service_runlevels (OobsService     *service,
					   GList           *runlevels,
					   DBusMessage     *message,
					   DBusMessageIter *iter)
{
  DBusMessageIter runlevels_iter, struct_iter;
  OobsServicesRunlevel *runlevel;
  OobsServiceStatus status;
  gint priority;

  dbus_message_iter_open_container (iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING,
				    &runlevels_iter);

  while (runlevels)
    {
      runlevel = runlevels->data;
      runlevels = runlevels->next;

      oobs_service_get_runlevel_configuration (service, runlevel, &status, &priority);

      if (status == OOBS_SERVICE_IGNORE)
	continue;

      dbus_message_iter_open_container (&runlevels_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

      utils_append_string (&struct_iter, runlevel->name);
      utils_append_int (&struct_iter, status);
      utils_append_int (&struct_iter, priority);

      dbus_message_iter_close_container (&runlevels_iter, &struct_iter);
    }

  dbus_message_iter_close_container (iter, &runlevels_iter);
}

gboolean
_oobs_create_dbus_struct_from_service (OobsService     *service,
                                       GList           *runlevels,
                                       DBusMessage     *message,
                                       DBusMessageIter *array_iter)
{
  DBusMessageIter struct_iter;
  gchar *name;

  g_object_get (G_OBJECT (service),
		"name", &name,
		NULL);

  g_return_val_if_fail (name, FALSE);

  dbus_message_iter_open_container (array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

  utils_append_string (&struct_iter, name);
  create_dbus_struct_from_service_runlevels (service, runlevels, message, &struct_iter);

  dbus_message_iter_close_container (array_iter, &struct_iter);

  g_free (name);

  return TRUE;
}

/**
 * oobs_service_get_name:
 * @service: An #OobsService.
 * 
 * Returns the service name
 * 
 * Return Value: A pointer to the service name as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_service_get_name (OobsService *service)
{
  OobsServicePrivate *priv;

  g_return_val_if_fail (OOBS_IS_SERVICE (service), NULL);

  priv = service->_priv;
  return priv->name;
}

/**
 * oobs_service_set_runlevel_configuration:
 * @service: An #OobsService.
 * @runlevel: A #OobsServicesRunlevel.
 * @status: status for the service in the given runlevel
 * @priority: priority for the service in the given runlevel.
 * Use 0 to either keep the previous priority, or get the default value in case the script is new.
 *
 * Sets the configuration of a service for a given runlevel.
 **/
void
oobs_service_set_runlevel_configuration (OobsService          *service,
					 OobsServicesRunlevel *runlevel,
					 OobsServiceStatus     status,
					 gint                  priority)
{
  OobsServicePrivate *priv;
  OobsServiceRunlevel *service_runlevel;

  g_return_if_fail (OOBS_IS_SERVICE (service));
  g_return_if_fail (runlevel != NULL);
  g_return_if_fail (priority >= 0 && priority <= 99);

  priv = service->_priv;

  service_runlevel = g_hash_table_lookup (priv->runlevels_config, runlevel);

  if (!service_runlevel)
    {
      service_runlevel = g_new0 (OobsServiceRunlevel, 1);
      g_hash_table_insert (priv->runlevels_config,
                           runlevel, service_runlevel);
    }

  service_runlevel->status = status;

  /* Keep previous priority. If the script was not used previously,
   * the backends will use a default value. */
  if (priority != 0)
    service_runlevel->priority = priority;
}

/**
 * oobs_service_get_runlevel_configuration:
 * @service: An #OobsService.
 * @runlevel: An #OobsServiceRunlevel.
 * @status: return value for the current service status.
 * @priority: return value for the current service priority.
 * 
 * Gets the status and priority of a service in a given runlevel.
 **/
void
oobs_service_get_runlevel_configuration (OobsService          *service,
					 OobsServicesRunlevel *runlevel,
					 OobsServiceStatus    *status,
					 gint                 *priority)
{
  OobsServicePrivate *priv;
  OobsServiceRunlevel *service_runlevel;

  g_return_if_fail (OOBS_IS_SERVICE (service));
  g_return_if_fail (runlevel != NULL);

  priv = service->_priv;

  service_runlevel = g_hash_table_lookup (priv->runlevels_config, runlevel);

  if (status)
    *status = (service_runlevel) ? service_runlevel->status : OOBS_SERVICE_STOP;

  if (priority)
    *priority = (service_runlevel) ? service_runlevel->priority : 0;
}
