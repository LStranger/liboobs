/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004-2005 Carlos Garnacho
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
#include "oobs-iface.h"

#define OOBS_IFACE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACE, OobsIfacePrivate))

typedef struct _OobsIfacePrivate OobsIfacePrivate;

struct _OobsIfacePrivate
{
  gboolean  is_auto;
  gboolean  is_enabled;
  gboolean  is_configured;
  gchar    *dev;
  gchar    *hwaddr;
  gchar    *file;
};

static void oobs_iface_class_init (OobsIfaceClass *class);
static void oobs_iface_init       (OobsIface      *iface);
static void oobs_iface_finalize   (GObject        *object);

static void oobs_iface_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec);
static void oobs_iface_get_property (GObject      *object,
				     guint         prop_id,
				     GValue       *value,
				     GParamSpec   *pspec);
enum {
  PROP_0,
  PROP_AUTO,
  PROP_ENABLED,
  PROP_CONFIGURED,
  PROP_DEV,
  PROP_HWADDR
};

G_DEFINE_TYPE (OobsIface, oobs_iface, G_TYPE_OBJECT);

static void
oobs_iface_class_init (OobsIfaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_iface_set_property;
  object_class->get_property = oobs_iface_get_property;
  object_class->finalize     = oobs_iface_finalize;

  g_object_class_install_property (object_class,
				   PROP_AUTO,
				   g_param_spec_boolean ("auto",
							 "Iface is auto",
							 "Whether the interface starts at boot time",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_ENABLED,
				   g_param_spec_boolean ("enabled",
							 "Iface is enabled",
							 "Whether the interface is enabled",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_CONFIGURED,
				   g_param_spec_boolean ("configured",
							 "Iface is configured",
							 "Whether the interface is configured",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DEV,
				   g_param_spec_string ("device",
							"Iface device",
							"Device name of the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_HWADDR,
				   g_param_spec_string ("hardware-address",
							"Iface hwaddr",
							"MAC address of the iface",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsIfacePrivate));
}

static void
oobs_iface_init (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE (iface));

  priv = OOBS_IFACE_GET_PRIVATE (iface);

  priv->dev = NULL;
  priv->hwaddr = NULL;
  priv->file = NULL;
}

static void
oobs_iface_finalize (GObject *object)
{
  OobsIfacePrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE (object));

  priv = OOBS_IFACE_GET_PRIVATE (object);

  if (priv)
    {
      g_free (priv->dev);
      g_free (priv->hwaddr);
      g_free (priv->file);
    }

  if (G_OBJECT_CLASS (oobs_iface_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_iface_parent_class)->finalize) (object);
}

static void
oobs_iface_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  OobsIfacePrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE (object));

  priv = OOBS_IFACE_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_AUTO:
      priv->is_auto = g_value_get_boolean (value);
      break;
    case PROP_ENABLED:
      priv->is_enabled = g_value_get_boolean (value);
      break;
    case PROP_CONFIGURED:
      priv->is_configured = g_value_get_boolean (value);
      break;
    case PROP_DEV:
      g_free (priv->dev);
      priv->dev = g_value_dup_string (value);
      break;
    case PROP_HWADDR:
      g_free (priv->hwaddr);
      priv->hwaddr = g_value_dup_string (value);
      break;
    }
}

static void
oobs_iface_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  OobsIfacePrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE (object));

  priv = OOBS_IFACE_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_AUTO:
      g_value_set_boolean (value, priv->is_auto);
      break;
    case PROP_ENABLED:
      g_value_set_boolean (value, priv->is_enabled);
      break;
    case PROP_CONFIGURED:
      g_value_set_boolean (value, priv->is_configured);
      break;
    case PROP_DEV:
      g_value_set_string (value, priv->dev);
      break;
    case PROP_HWADDR:
      g_value_set_string (value, priv->hwaddr);
      break;
    }
}

gboolean
oobs_iface_get_auto (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = OOBS_IFACE_GET_PRIVATE (iface);

  return priv->is_auto;
}

void
oobs_iface_set_auto (OobsIface *iface, gboolean is_auto)
{
  g_return_if_fail (OOBS_IS_IFACE (iface));

  g_object_set (G_OBJECT (iface), "auto", is_auto, NULL);
}

gboolean
oobs_iface_get_enabled (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = OOBS_IFACE_GET_PRIVATE (iface);

  return priv->is_enabled;
}

void
oobs_iface_set_enabled (OobsIface *iface, gboolean is_enabled)
{
  g_return_if_fail (OOBS_IS_IFACE (iface));

  g_object_set (G_OBJECT (iface), "enabled", is_enabled, NULL);
}

G_CONST_RETURN gchar*
oobs_iface_get_dev (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = OOBS_IFACE_GET_PRIVATE (iface);

  return priv->dev;
}

void
oobs_iface_set_dev (OobsIface *iface, const gchar *dev)
{
  g_return_if_fail (OOBS_IS_IFACE (iface));

  g_object_set (G_OBJECT (iface), "device", dev, NULL);
}

G_CONST_RETURN gchar*
oobs_iface_get_hwaddr (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = OOBS_IFACE_GET_PRIVATE (iface);

  return priv->hwaddr;
}

void
oobs_iface_set_hwaddr (OobsIface *iface, const gchar *hwaddr)
{
  g_return_if_fail (OOBS_IS_IFACE (iface));

  g_object_set (G_OBJECT (iface), "hardware-address", hwaddr, NULL);
}

gboolean
oobs_iface_is_configured (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = OOBS_IFACE_GET_PRIVATE (iface);

  return priv->is_configured;
}

void
oobs_iface_set_configured (OobsIface *iface, gboolean is_configured)
{
  g_return_if_fail (OOBS_IS_IFACE (iface));

  g_object_set (G_OBJECT (iface), "configured", is_configured, NULL);
}

gboolean
oobs_iface_has_gateway (OobsIface *iface)
{
  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  if (OOBS_IFACE_GET_CLASS (iface)->has_gateway == NULL)
    return FALSE;

  return OOBS_IFACE_GET_CLASS (iface)->has_gateway (iface);
}
