/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
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
#include "oobs-iface-plip.h"
#include "oobs-iface.h"

#define OOBS_IFACE_PLIP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACE_PLIP, OobsIfacePlipPrivate))

typedef struct _OobsIfacePlipPrivate OobsIfacePlipPrivate;

struct _OobsIfacePlipPrivate
{
  gchar *address;
  gchar *remote_address;
};

static void oobs_iface_plip_class_init (OobsIfacePlipClass *class);
static void oobs_iface_plip_init       (OobsIfacePlip      *iface);
static void oobs_iface_plip_finalize   (GObject           *object);

static gboolean oobs_iface_plip_has_gateway (OobsIface *iface);

static void oobs_iface_plip_set_property (GObject      *object,
					  guint         prop_id,
					  const GValue *value,
					  GParamSpec   *pspec);
static void oobs_iface_plip_get_property (GObject      *object,
					  guint         prop_id,
					  GValue       *value,
					  GParamSpec   *pspec);
enum {
  PROP_0,
  PROP_ADDRESS,
  PROP_REMOTE_ADDRESS,
};

G_DEFINE_TYPE (OobsIfacePlip, oobs_iface_plip, OOBS_TYPE_IFACE);


static void
oobs_iface_plip_class_init (OobsIfacePlipClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsIfaceClass *iface_class = OOBS_IFACE_CLASS (class);

  object_class->set_property = oobs_iface_plip_set_property;
  object_class->get_property = oobs_iface_plip_get_property;
  object_class->finalize     = oobs_iface_plip_finalize;

  iface_class->has_gateway   = oobs_iface_plip_has_gateway;

  g_object_class_install_property (object_class,
				   PROP_ADDRESS,
				   g_param_spec_string ("iface_local_address",
							"Iface address",
							"Address for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_REMOTE_ADDRESS,
				   g_param_spec_string ("iface_remote_address",
							"Iface remote address",
							"Remote address for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsIfacePlipPrivate));
}

static void
oobs_iface_plip_init (OobsIfacePlip *iface)
{
  OobsIfacePlipPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_PLIP (iface));

  priv = OOBS_IFACE_PLIP_GET_PRIVATE (iface);

  priv->address = NULL;
  priv->remote_address = NULL;
}

static void
oobs_iface_plip_finalize (GObject *object)
{
  OobsIfacePlipPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_PLIP (object));

  priv = OOBS_IFACE_PLIP_GET_PRIVATE (object);

  if (priv)
    {
      g_free (priv->address);
      g_free (priv->remote_address);
    }

  if (G_OBJECT_CLASS (oobs_iface_plip_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_iface_plip_parent_class)->finalize) (object);
}

static void
oobs_iface_plip_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  OobsIfacePlipPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_PLIP (object));

  priv = OOBS_IFACE_PLIP_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_free (priv->address);
      priv->address = g_value_dup_string (value);
      break;
    case PROP_REMOTE_ADDRESS:
      g_free (priv->remote_address);
      priv->remote_address = g_value_dup_string (value);
      break;
    }
}

static void
oobs_iface_plip_get_property (GObject      *object,
			      guint         prop_id,
			      GValue       *value,
			      GParamSpec   *pspec)
{
  OobsIfacePlipPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_PLIP (object));

  priv = OOBS_IFACE_PLIP_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_value_set_string (value, priv->address);
      break;
    case PROP_REMOTE_ADDRESS:
      g_value_set_string (value, priv->remote_address);
      break;
    }
}

static gboolean
oobs_iface_plip_has_gateway (OobsIface *iface)
{
  return TRUE;
}

/**
 * oobs_iface_plip_get_address:
 * @iface: An #OobsIfacePlip.
 * 
 * Returns the local IP address for the interface.
 * 
 * Return Value: A pointer to the local IP address as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_plip_get_address (OobsIfacePlip *iface)
{
  OobsIfacePlipPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PLIP (iface), NULL);

  priv = OOBS_IFACE_PLIP_GET_PRIVATE (iface);

  return priv->address;
}

/**
 * oobs_iface_plip_set_address:
 * @iface: An #OobsIfacePlip.
 * @address: a new local IP address for the interface.
 * 
 * Sets a new local IP address for the interface,
 * overwriting the previous one.
 **/
void
oobs_iface_plip_set_address (OobsIfacePlip *iface, const gchar *address)
{
  g_return_if_fail (OOBS_IS_IFACE_PLIP (iface));

  /* FIXME: should validate IP address */
  g_object_set (G_OBJECT (iface), "iface-local-address", address, NULL);
}

/**
 * oobs_iface_plip_get_remote_address:
 * @iface: An #OobsIfacePlip.
 * 
 * Returns the remote IP address for the interface.
 * 
 * Return Value: A pointer to the remote IP address as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_plip_get_remote_address (OobsIfacePlip *iface)
{
  OobsIfacePlipPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_PLIP (iface), NULL);

  priv = OOBS_IFACE_PLIP_GET_PRIVATE (iface);

  return priv->remote_address;
}

/**
 * oobs_iface_plip_set_remote_address:
 * @iface: An #OobsIfacePlip.
 * @address: a new remote IP address for the interface.
 * 
 * Sets a new remote IP address for the interface,
 * overwriting the previous one.
 **/
void
oobs_iface_plip_set_remote_address (OobsIfacePlip *iface, const gchar *address)
{
  g_return_if_fail (OOBS_IS_IFACE_PLIP (iface));

  /* FIXME: should validate IP address */
  g_object_set (G_OBJECT (iface), "iface-remote-address", address, NULL);
}
