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

static gboolean oobs_iface_modem_is_configured (OobsIface *iface);

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
          { OOBS_MODEM_VOLUME_LOUD,   "OOBS_MODEM_VOLUME_LOUD",   "loud" },
          { 0, NULL, NULL }
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
	  { OOBS_DIAL_TYPE_PULSES, "OOBS_DIAL_TYPE_PULSES", "pulses" },
 	  { 0, NULL, NULL }
        };

      etype = g_enum_register_static ("OobsDialType", values);
    }

  return etype;
}

static void
oobs_iface_modem_class_init (OobsIfaceModemClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsIfaceClass *iface_class = OOBS_IFACE_CLASS (class);

  object_class->set_property = oobs_iface_modem_set_property;
  object_class->get_property = oobs_iface_modem_get_property;
  object_class->finalize = oobs_iface_modem_finalize;

  iface_class->is_configured = oobs_iface_modem_is_configured;

  g_object_class_install_property (object_class,
                                   PROP_SERIAL_PORT,
                                   g_param_spec_string ("serial_port",
                                                        "Serial port",
                                                        "Serial port for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_VOLUME,
                                   g_param_spec_enum ("volume",
                                                      "Modem volume",
                                                      "Modem volume",
                                                      OOBS_TYPE_MODEM_VOLUME,
                                                      OOBS_MODEM_VOLUME_SILENT,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DIAL_TYPE,
                                   g_param_spec_enum ("dial_type",
                                                      "Modem dial type",
                                                      "Modem dial type",
                                                      OOBS_TYPE_DIAL_TYPE,
                                                      OOBS_DIAL_TYPE_TONES,
                                                      G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsIfaceModemPrivate));
}

static void
oobs_iface_modem_init (OobsIfaceModem *iface)
{
  OobsIfaceModemPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_MODEM (iface));

  priv = OOBS_IFACE_MODEM_GET_PRIVATE (iface);

  priv->serial_port = NULL;
  iface->_priv = priv;
}

static void
oobs_iface_modem_finalize (GObject *object)
{
  OobsIfaceModemPrivate *priv;

  g_return_if_fail (OOBS_IS_IFACE_MODEM (object));

  priv = OOBS_IFACE_MODEM (object)->_priv;

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

  priv = OOBS_IFACE_MODEM (object)->_priv;
  
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

  priv = OOBS_IFACE_MODEM (object)->_priv;
  
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

static gboolean
oobs_iface_modem_is_configured (OobsIface *iface)
{
  OobsIfaceModemPrivate *priv;

  priv = OOBS_IFACE_MODEM (iface)->_priv;

  return (priv->serial_port &&
	  (* OOBS_IFACE_CLASS (oobs_iface_modem_parent_class)->is_configured) (iface));
}

/**
 * oobs_iface_modem_set_serial_port:
 * @iface: An #OobsIfaceModem.
 * @serial_port: a new serial port for the interface.
 * 
 * Sets a serial port to connect with the modem device.
 **/
void
oobs_iface_modem_set_serial_port (OobsIfaceModem *iface,
				  const gchar    *serial_port)
{
  g_return_if_fail (OOBS_IS_IFACE_MODEM (iface));

  g_object_set (G_OBJECT (iface), "serial-port", serial_port, NULL);
}

/**
 * oobs_iface_modem_get_serial_port:
 * @iface: An #OobsIfaceModem.
 * 
 * Returns the serial port used to communicate with the modem device.
 * 
 * Return Value: A pointer to the serial port as a string. This string
 *               must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_iface_modem_get_serial_port (OobsIfaceModem *iface)
{
  OobsIfaceModemPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_MODEM (iface), NULL);

  priv = iface->_priv;
  return priv->serial_port;
}

/**
 * oobs_iface_modem_set_volume:
 * @iface: An #OobsIfaceModem.
 * @volume: the modem volume.
 * 
 * Sets the modem volume.
 **/
void
oobs_iface_modem_set_volume (OobsIfaceModem  *iface,
			     OobsModemVolume  volume)
{
  g_return_if_fail (OOBS_IS_IFACE_MODEM (iface));

  g_object_set (G_OBJECT (iface), "volume", volume, NULL);
}

/**
 * oobs_iface_modem_get_volume:
 * @iface: An #OobsIfaceModem.
 * 
 * Returns the modem volume.
 * 
 * Return Value: the modem volume.
 **/
OobsModemVolume
oobs_iface_modem_get_volume (OobsIfaceModem *iface)
{
  OobsIfaceModemPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_MODEM (iface), OOBS_MODEM_VOLUME_SILENT);

  priv = iface->_priv;
  return priv->volume;
}

/**
 * oobs_iface_modem_set_dial_type:
 * @iface: An #OobsIfaceModem.
 * @dial_type: dialing type for the modem connection.
 * 
 * Sets the dialing type for the modem connection.
 **/
void
oobs_iface_modem_set_dial_type (OobsIfaceModem  *iface,
				OobsDialType     dial_type)
{
  g_return_if_fail (OOBS_IS_IFACE_MODEM (iface));

  g_object_set (G_OBJECT (iface), "dial-type", dial_type, NULL);
}

/**
 * oobs_iface_modem_get_dial_type:
 * @iface: An #OobsIfaceModem.
 * 
 * Returns the dialing type for the modem connection.
 * 
 * Return Value: the dialing type.
 **/
OobsDialType
oobs_iface_modem_get_dial_type (OobsIfaceModem *iface)
{
  OobsIfaceModemPrivate *priv;

  g_return_val_if_fail (OOBS_IS_IFACE_MODEM (iface), OOBS_DIAL_TYPE_TONES);

  priv = iface->_priv;
  return priv->dial_type;
}
