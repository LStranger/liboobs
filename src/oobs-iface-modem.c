/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2005 Carlos Garnacho
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
#include "oobs-iface-modem.h"
#include "oobs-iface-isdn.h"
#include "oobs-iface.h"

#define OOBS_IFACE_MODEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_IFACE_MODEM, OobsIfaceModemPrivate))

typedef struct _OobsIfaceModemPrivate OobsIfaceModemPrivate;

struct _OobsIfaceModemPrivate
{
  gchar *serial_port;

  gint volume;
  gint dial_type;
};

static void oobs_iface_modem_class_init (OobsIfaceModemClass *class);
static void oobs_iface_modem_init       (OobsIfaceModem      *iface);
static void oobs_iface_modem_finalize   (GObject             *object);

static void oobs_iface_modem_set_property (GObject      *object,
					   guint         prop_id,
					   const GValue *value,
					   GParamSpec   *pspec);
static void oobs_iface_modem_get_property (GObject      *object,
					   guint         prop_id,
					   GValue       *value,
					   GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_SERIAL_PORT,
  PROP_VOLUME,
  PROP_DIAL_TYPE,
};

G_DEFINE_TYPE (OobsIfaceModem, oobs_iface_modem, OOBS_TYPE_IFACE_ISDN);

GType
oobs_modem_volume_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
        {
          { OOBS_MODEM_VOLUME_SILENT, "OOBS_MODEM_VOLUME_SILENT", "silent" },
          { OOBS_MODEM_VOLUME_LOW,    "OOBS_MODEM_VOLUME_LOW",    "low" },
          { OOBS_MODEM_VOLUME_MEDIUM, "OOBS_MODEM_VOLUME_MEDIUM", "medium" },
          { OOBS_MODEM_VOLUME_LOUD,   "OOBS_MODEM_VOLUME_LOUD",   "loud" }
        };

      etype = g_enum_register_static ("OobsModemVolume", values);
    }

  return etype;
}

GType
oobs_dial_type_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
        {
	  { OOBS_DIAL_TYPE_TONES,  "OOBS_DIAL_TYPE_TONES",  "tones" },
	  { OOBS_DIAL_TYPE_PULSES, "OOBS_DIAL_TYPE_PULSES", "pulses" }
        };

      etype = g_enum_register_static ("OobsDialType", values);
    }

  return etype;
}

static void
oobs_iface_modem_class_init (OobsIfaceModemClass *class)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_iface_modem_set_property;
  object_class->get_property = oobs_iface_modem_get_property;
  object_class->finalize = oobs_iface_modem_finalize;

  g_object_class_install_property (object_class,
                                   PROP_SERIAL_PORT,
                                   g_param_spec_string ("iface_serial_port",
                                                        "Iface serial port",
                                                        "Serial port for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_VOLUME,
                                   g_param_spec_enum ("iface_volume",
                                                      "Iface volume",
                                                      "Volume for the connection",
                                                      OOBS_TYPE_MODEM_VOLUME,
                                                      OOBS_MODEM_VOLUME_SILENT,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DIAL_TYPE,
                                   g_param_spec_enum ("iface_dial_type",
                                                      "Iface dial type",
                                                      "Dial type for the connection",
                                                      OOBS_TYPE_DIAL_TYPE,
                                                      OOBS_DIAL_TYPE_TONES,
                                                      G_PARAM_READWRITE));
}

static void
oobs_iface_modem_init (OobsIfaceModem *iface)
{
  OobsIfaceModemPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_MODEM (iface));

  priv = OOBS_IFACE_MODEM_GET_PRIVATE (iface);

  priv->serial_port = NULL;
}

static void
oobs_iface_modem_finalize (GObject *object)
{
  OobsIfaceModemPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_MODEM (object));

  priv = OOBS_IFACE_MODEM_GET_PRIVATE (object);

  if (priv)
    g_free (priv->serial_port);

  if (G_OBJECT_CLASS (oobs_iface_modem_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_iface_modem_parent_class)->finalize) (object);
}

static void
oobs_iface_modem_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  OobsIfaceModemPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_MODEM (object));

  priv = OOBS_IFACE_MODEM_GET_PRIVATE (object);
  
  switch (prop_id)
    {
    case PROP_SERIAL_PORT:
      g_free (priv->serial_port);
      priv->serial_port = g_value_dup_string (value);
      break;
    case PROP_VOLUME:
      priv->volume = g_value_get_enum (value);
      break;
    case PROP_DIAL_TYPE:
      priv->dial_type = g_value_get_enum (value);
      break;
    }
}

static void
oobs_iface_modem_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  OobsIfaceModemPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_MODEM (object));

  priv = OOBS_IFACE_MODEM_GET_PRIVATE (object);
  
  switch (prop_id)
    {
    case PROP_SERIAL_PORT:
      g_value_set_string (value, priv->serial_port);
      break;
    case PROP_VOLUME:
      g_value_set_enum (value, priv->volume);
      break;
    case PROP_DIAL_TYPE:
      g_value_set_enum (value, priv->dial_type);
      break;
    }
}

/* FIXME: add getters and setters */