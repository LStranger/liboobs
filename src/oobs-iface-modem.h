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

#ifndef __OOBS_IFACE_MODEM_H
#define __OOBS_IFACE_MODEM_H

G_BEGIN_DECLS

#include "oobs-iface.h"
#include "oobs-iface-isdn.h"

typedef enum {
  OOBS_MODEM_VOLUME_SILENT,
  OOBS_MODEM_VOLUME_LOW,
  OOBS_MODEM_VOLUME_MEDIUM,
  OOBS_MODEM_VOLUME_LOUD
} OobsModemVolume;

typedef enum {
  OOBS_DIAL_TYPE_TONES,
  OOBS_DIAL_TYPE_PULSES
} OobsDialType;

#define OOBS_TYPE_MODEM_VOLUME          (oobs_modem_volume_get_type ())
#define OOBS_TYPE_DIAL_TYPE             (oobs_dial_type_get_type ())

#define OOBS_TYPE_IFACE_MODEM           (oobs_iface_modem_get_type ())
#define OOBS_IFACE_MODEM(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OOBS_TYPE_IFACE_MODEM, OobsIfaceModem))
#define OOBS_IFACE_MODEM_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    OOBS_TYPE_IFACE_MODEM, OobsIfaceModemClass))
#define OOBS_IS_IFACE_MODEM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OOBS_TYPE_IFACE_MODEM))
#define OOBS_IS_IFACE_MODEM_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    OOBS_TYPE_IFACE_MODEM))
#define OOBS_IFACE_MODEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  OOBS_TYPE_IFACE_MODEM, OobsIfaceModemClass))

typedef struct _OobsIfaceModem      OobsIfaceModem;
typedef struct _OobsIfaceModemClass OobsIfaceModemClass;

struct _OobsIfaceModem
{
  OobsIfaceISDN parent;
};

struct _OobsIfaceModemClass
{
  OobsIfaceISDNClass parent_class;
};

GType oobs_modem_volume_get_type ();
GType oobs_dial_type_get_type ();
GType oobs_iface_modem_get_type ();


G_END_DECLS

#endif /* __OOBS_IFACE_MODEM_H */
