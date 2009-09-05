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

/**
 * SECTION:oobs-service
 * @title: OobsService
 * @short_description: Object that represents an individual init.d service
 * @see_also: #OobsServicesConfig
 **/

#define OOBS_SERVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SERVICE, OobsServicePrivate))

typedef struct _OobsServicePrivate  OobsServicePrivate;
typedef struct _OobsServiceRunlevel OobsServiceRunlevel;

struct _OobsServiceRunlevel {
  guint status;
  guint priority;
};
	
struct _OobsServicePrivate {
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
enum
{
  PROP_0,
  PROP_NAME
};

G_DEFINE_TYPE (OobsService, oobs_service, G_TYPE_OBJECT);

static void
oobs_service_class_init (OobsServiceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_service_set_property;
  object_class->get_property = oobs_service_get_property;
  object_class->finalize     = oobs_service_finalize;

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

  if (status == OOBS_SERVICE_IGNORE)
    g_hash_table_remove (priv->runlevels_config, runlevel);
  else
    {
      service_runlevel = g_hash_table_lookup (priv->runlevels_config, runlevel);

      if (!service_runlevel)
	{
	  service_runlevel = g_new0 (OobsServiceRunlevel, 1);
	  g_hash_table_insert (priv->runlevels_config,
			       runlevel, service_runlevel);
	}

      service_runlevel->status = status;
      service_runlevel->priority = priority;
    }
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
    *status = (service_runlevel) ? service_runlevel->status : OOBS_SERVICE_IGNORE;

  if (priority)
    *priority = (service_runlevel) ? service_runlevel->priority : -1;
}
