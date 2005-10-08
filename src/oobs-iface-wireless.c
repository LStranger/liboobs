/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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
#include "oobs-iface-wireless.h"
#include "oobs-iface.h"

#define OOBS_IFACE_WIRELESS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACE_WIRELESS, OobsIfaceWirelessPrivate))

typedef struct _OobsIfaceWirelessPrivate OobsIfaceWirelessPrivate;

struct _OobsIfaceWirelessPrivate
{
  gchar *essid;
  gchar *wep_key;
  OobsWirelessKeyType key_type;
};

static void oobs_iface_wireless_class_init (OobsIfaceWirelessClass *class);
static void oobs_iface_wireless_init       (OobsIfaceWireless      *iface);
static void oobs_iface_wireless_finalize   (GObject                *object);

static void oobs_iface_wireless_set_property (GObject      *object,
					      guint         prop_id,
					      const GValue *value,
					      GParamSpec   *pspec);
static void oobs_iface_wireless_get_property (GObject      *object,
					      guint         prop_id,
					      GValue       *value,
					      GParamSpec   *pspec);
enum {
  PROP_0,
  PROP_ESSID,
  PROP_WEP_KEY_TYPE,
  PROP_WEP_KEY
};

GType
oobs_wireless_key_type_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
	{
	  { OOBS_WIRELESS_KEY_ASCII,       "OOBS_WIRELESS_KEY_ASCII",       "ascii" },
	  { OOBS_WIRELESS_KEY_HEXADECIMAL, "OOBS_WIRELESS_KEY_HEXADECIMAL", "hexadecimal" },
	};

      etype = g_enum_register_static ("OobsWirelessKeyType", values);
    }

  return etype;
}

G_DEFINE_TYPE (OobsIfaceWireless, oobs_iface_wireless, OOBS_TYPE_IFACE_ETHERNET);


static void
oobs_iface_wireless_class_init (OobsIfaceWirelessClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_iface_wireless_set_property;
  object_class->get_property = oobs_iface_wireless_get_property;
  object_class->finalize     = oobs_iface_wireless_finalize;

  g_object_class_install_property (object_class,
				   PROP_ESSID,
				   g_param_spec_string ("iface_essid",
							"Iface ESSID",
							"ESSID",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_WEP_KEY_TYPE,
				   g_param_spec_enum ("iface_wep_key_type",
						      "Iface WEP key type",
						      "key type",
						      OOBS_TYPE_WIRELESS_KEY_TYPE,
						      OOBS_WIRELESS_KEY_HEXADECIMAL,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_WEP_KEY,
				   g_param_spec_string ("iface_wep_key",
							"Iface WEP key",
							"WEP key",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsIfaceWirelessPrivate));
}

static void
oobs_iface_wireless_init (OobsIfaceWireless *iface)
{
  OobsIfaceWirelessPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_WIRELESS (iface));

  priv = OOBS_IFACE_WIRELESS_GET_PRIVATE (iface);

  priv->essid = NULL;
  priv->wep_key = NULL;
}

static void
oobs_iface_wireless_finalize (GObject *object)
{
  OobsIfaceWirelessPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_WIRELESS (object));

  priv = OOBS_IFACE_WIRELESS_GET_PRIVATE (object);

  if (priv)
    {
      g_free (priv->essid);
      g_free (priv->wep_key);
    }

  if (G_OBJECT_CLASS (oobs_iface_wireless_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_iface_wireless_parent_class)->finalize) (object);
}

static void
oobs_iface_wireless_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  OobsIfaceWirelessPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_WIRELESS (object));

  priv = OOBS_IFACE_WIRELESS_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ESSID:
      g_free (priv->essid);
      priv->essid = g_value_dup_string (value);
      break;
    case PROP_WEP_KEY_TYPE:
      priv->key_type = g_value_get_enum (value);
      break;
    case PROP_WEP_KEY:
      g_free (priv->wep_key);
      priv->wep_key = g_value_dup_string (value);
      break;
    }
}

static void
oobs_iface_wireless_get_property (GObject      *object,
				  guint         prop_id,
				  GValue       *value,
				  GParamSpec   *pspec)
{
  OobsIfaceWirelessPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_WIRELESS (object));

  priv = OOBS_IFACE_WIRELESS_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ESSID:
      g_value_set_string (value, priv->essid);
      break;
    case PROP_WEP_KEY_TYPE:
      g_value_set_enum (value, priv->key_type);
      break;
    case PROP_WEP_KEY:
      g_value_set_string (value, priv->wep_key);
      break;
    }
}

G_CONST_RETURN gchar*
oobs_iface_wireless_get_essid (OobsIfaceWireless *iface)
{
  OobsIfaceWirelessPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_WIRELESS (iface), NULL);

  priv = OOBS_IFACE_WIRELESS_GET_PRIVATE (iface);

  return priv->essid;
}

void
oobs_iface_wireless_set_essid (OobsIfaceWireless *iface, const gchar *essid)
{
  g_return_if_fail (OOBS_IS_IFACE_WIRELESS (iface));

  g_object_set (G_OBJECT (iface), "iface-essid", essid, NULL);
}

G_CONST_RETURN gchar*
oobs_iface_wireless_get_wep_key (OobsIfaceWireless *iface)
{
  OobsIfaceWirelessPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_WIRELESS (iface), NULL);

  priv = OOBS_IFACE_WIRELESS_GET_PRIVATE (iface);

  return priv->wep_key;
}

void
oobs_iface_wireless_set_wep_key (OobsIfaceWireless *iface, const gchar *wep_key)
{
  g_return_if_fail (OOBS_IS_IFACE_WIRELESS (iface));

  g_object_set (G_OBJECT (iface), "iface-wep-key", wep_key, NULL);
}

OobsWirelessKeyType
oobs_iface_wireless_get_wep_key_type (OobsIfaceWireless *iface)
{
  OobsIfaceWirelessPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_WIRELESS (iface), OOBS_WIRELESS_KEY_ASCII);

  priv = OOBS_IFACE_WIRELESS_GET_PRIVATE (iface);

  return priv->key_type;
}

void
oobs_iface_wireless_set_wep_key_type (OobsIfaceWireless *iface, OobsWirelessKeyType key_type)
{
  g_return_if_fail (OOBS_IS_IFACE_WIRELESS (iface));

  g_object_set (G_OBJECT (iface), "iface-wep-key-type", key_type, NULL);
}
