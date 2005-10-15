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

#define OOBS_SERVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SERVICE, OobsServicePrivate))

typedef struct _OobsServicePrivate OobsServicePrivate;

struct _OobsServicePrivate {
  gchar *name;
  gchar *role;
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
  PROP_NAME,
  PROP_ROLE
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
  g_object_class_install_property (object_class,
				   PROP_ROLE,
				   g_param_spec_string ("role",
							"Service role",
							"Role that defines what the service does",
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
  priv->role = NULL;
}

static void
oobs_service_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  OobsServicePrivate *priv;

  priv = OOBS_SERVICE_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_free (priv->name);
      priv->name  = g_value_dup_string (value);
      break;
    case PROP_ROLE:
      g_free (priv->role);
      priv->role = g_value_dup_string (value);
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

  priv = OOBS_SERVICE_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_ROLE:
      g_value_set_string (value, priv->role);
      break;
    }
}

static void
oobs_service_finalize (GObject *object)
{
  OobsServicePrivate *priv;

  priv = OOBS_SERVICE_GET_PRIVATE (object);

  if (priv)
    {
      g_free (priv->name);
      g_free (priv->role);
    }
  
  if (G_OBJECT_CLASS (oobs_service_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_service_parent_class)->finalize) (object);
}

OobsService*
oobs_service_new (const gchar *name, const gchar *role)
{
  return g_object_new (OOBS_TYPE_SERVICE,
		       "name", name,
		       "role", role,
		       NULL);
}
