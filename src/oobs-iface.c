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
  gboolean  explicitly_not_configured;
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

G_DEFINE_ABSTRACT_TYPE (OobsIface, oobs_iface, G_TYPE_OBJECT);

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
				   g_param_spec_boolean ("active",
							 "Iface is active",
							 "Whether the interface is active",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_CONFIGURED,
				   g_param_spec_boolean ("configured",
							 "Iface is configured",
							 "Whether the interface is fully configured",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DEV,
				   g_param_spec_string ("device",
							"Iface device",
							"Device name of the iface",
							NULL,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_HWADDR,
				   g_param_spec_string ("hardware-address",
							"Iface hwaddr",
							"MAC address of the iface",
							NULL,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
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
  priv->explicitly_not_configured = FALSE;
  iface->_priv = priv;
}

static void
oobs_iface_finalize (GObject *object)
{
  OobsIfacePrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE (object));

  priv = OOBS_IFACE (object)->_priv;

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

  priv = OOBS_IFACE (object)->_priv;

  switch (prop_id)
    {
    case PROP_AUTO:
      priv->is_auto = g_value_get_boolean (value);
      break;
    case PROP_ENABLED:
      priv->is_enabled = g_value_get_boolean (value);
      break;
    case PROP_CONFIGURED:
      oobs_iface_set_configured (OOBS_IFACE (object), g_value_get_boolean (value));
      break;
    case PROP_DEV:
      priv->dev = g_value_dup_string (value);
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

  priv = OOBS_IFACE (object)->_priv;

  switch (prop_id)
    {
    case PROP_AUTO:
      g_value_set_boolean (value, priv->is_auto);
      break;
    case PROP_ENABLED:
      g_value_set_boolean (value, priv->is_enabled);
      break;
    case PROP_CONFIGURED:
      g_value_set_boolean (value, oobs_iface_get_configured (OOBS_IFACE (object)));
      break;
    case PROP_DEV:
      g_value_set_string (value, priv->dev);
      break;
    case PROP_HWADDR:
      g_value_set_string (value, priv->hwaddr);
      break;
    }
}

/**
 * oobs_iface_get_auto:
 * @iface: An #OobsIface.
 * 
 * Returns whether the interface is started automatically at boot time.
 * 
 * Return Value: #TRUE if the interface starts during system boot.
 **/
gboolean
oobs_iface_get_auto (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = iface->_priv;

  return priv->is_auto;
}

/**
 * oobs_iface_set_auto:
 * @iface: An #OobsIface.
 * @is_auto: #TRUE to make the interface start at boot time.
 * 
 * Sets whether the interface is started automatically at boot time.
 **/
void
oobs_iface_set_auto (OobsIface *iface, gboolean is_auto)
{
  g_return_if_fail (OOBS_IS_IFACE (iface));

  g_object_set (G_OBJECT (iface), "auto", is_auto, NULL);
}

/**
 * oobs_iface_get_active:
 * @iface: An #OobsIface.
 * 
 * Returns whether the interface is active.
 * 
 * Return Value: #TRUE if the interface is active.
 **/
gboolean
oobs_iface_get_active (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = iface->_priv;

  return priv->is_enabled;
}

/**
 * oobs_iface_set_active:
 * @iface: An #OobsIface.
 * @is_active: #TRUE to enable the interface.
 * 
 * Sets whether the interface is currently active.
 **/
void
oobs_iface_set_active (OobsIface *iface, gboolean is_active)
{
  g_return_if_fail (OOBS_IS_IFACE (iface));

  g_object_set (G_OBJECT (iface), "active", is_active, NULL);
}

/**
 * oobs_iface_get_device_name:
 * @iface: An #OobsIface.
 * 
 * Returns the device name for the interface.
 * 
 * Return Value: A string containing the device name.
 * This string must not be freed or modified.
 **/
G_CONST_RETURN gchar*
oobs_iface_get_device_name (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = iface->_priv;

  return priv->dev;
}

G_CONST_RETURN gchar*
oobs_iface_get_hwaddr (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = iface->_priv;

  return priv->hwaddr;
}

/**
 * oobs_iface_get_configured:
 * @iface: An #OobsIface.
 * 
 * Returns whether the interface has a valid (i.e.: complete)
 * configuration and is explicitly marked as configured
 * (see oobs_iface_set_configured ()).
 * 
 * Return Value: #TRUE if its configuration is valid.
 **/
gboolean
oobs_iface_get_configured (OobsIface *iface)
{
  OobsIfacePrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  priv = iface->_priv;

  if (priv->explicitly_not_configured)
    return FALSE;

  return (* OOBS_IFACE_GET_CLASS (iface)->is_configured) (iface);
}

/**
 * oobs_iface_set_configured:
 * @iface: An #OobsIface.
 * @is_configured: #FALSE to explictitly mark the interface as not configured.
 * 
 * If @is_configured is #FALSE, the function explicitly marks the interface
 * as not configured. If @is_configured is #TRUE, the explicit mark will be
 * removed, but the interface may still have an incomplete/invalid configuration.
 **/
void
oobs_iface_set_configured (OobsIface *iface, gboolean is_configured)
{
  OobsIfacePrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE (iface));

  priv = iface->_priv;
  priv->explicitly_not_configured = (is_configured == FALSE);
  g_object_notify (G_OBJECT (iface), "configured");
}

gboolean
oobs_iface_has_gateway (OobsIface *iface)
{
  g_return_val_if_fail (OOBS_IS_IFACE (iface), FALSE);

  if (OOBS_IFACE_GET_CLASS (iface)->has_gateway == NULL)
    return FALSE;

  return OOBS_IFACE_GET_CLASS (iface)->has_gateway (iface);
}
