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

#ifndef __OOBS_IFACE_PPP_H
#define __OOBS_IFACE_PPP_H

G_BEGIN_DECLS

#include "oobs-iface.h"
#include "oobs-iface-ethernet.h"
#include "oobs-enum-types.h"

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

#define OOBS_TYPE_IFACE_PPP           (oobs_iface_ppp_get_type ())
#define OOBS_IFACE_PPP(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OOBS_TYPE_IFACE_PPP, OobsIfacePPP))
#define OOBS_IFACE_PPP_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    OOBS_TYPE_IFACE_PPP, OobsIfacePPPClass))
#define OOBS_IS_IFACE_PPP(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OOBS_TYPE_IFACE_PPP))
#define OOBS_IS_IFACE_PPP_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    OOBS_TYPE_IFACE_PPP))
#define OOBS_IFACE_PPP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  OOBS_TYPE_IFACE_PPP, OobsIfacePPPClass))

typedef struct _OobsIfacePPP      OobsIfacePPP;
typedef struct _OobsIfacePPPClass OobsIfacePPPClass;

struct _OobsIfacePPP
{
  OobsIface parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsIfacePPPClass
{
  OobsIfaceClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
};

GType oobs_modem_volume_get_type ();
GType oobs_dial_type_get_type ();
GType oobs_iface_ppp_get_type ();

void                   oobs_iface_ppp_set_connection_type (OobsIfacePPP *iface, const gchar *type);
G_CONST_RETURN gchar * oobs_iface_ppp_get_connection_type (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_login (OobsIfacePPP *iface, const gchar *login);
G_CONST_RETURN gchar*  oobs_iface_ppp_get_login (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_password (OobsIfacePPP *iface, const gchar *password);

void                   oobs_iface_ppp_set_phone_number (OobsIfacePPP *iface, const gchar *phone_number);
G_CONST_RETURN gchar*  oobs_iface_ppp_get_phone_number (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_phone_prefix (OobsIfacePPP *iface, const gchar *phone_prefix);
G_CONST_RETURN gchar*  oobs_iface_ppp_get_phone_prefix (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_default_gateway (OobsIfacePPP *iface, gboolean default_gw);
gboolean               oobs_iface_ppp_get_default_gateway (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_use_peer_dns (OobsIfacePPP *iface, gboolean use_peer_dns);
gboolean               oobs_iface_ppp_get_use_peer_dns (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_persistent (OobsIfacePPP *iface, gboolean persistent);
gboolean               oobs_iface_ppp_get_persistent (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_peer_noauth (OobsIfacePPP *iface, gboolean use_peer_dns);
gboolean               oobs_iface_ppp_get_peer_noauth (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_serial_port (OobsIfacePPP *iface, const gchar *serial_port);
G_CONST_RETURN gchar*  oobs_iface_ppp_get_serial_port (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_volume (OobsIfacePPP  *iface, OobsModemVolume volume);
OobsModemVolume        oobs_iface_ppp_get_volume (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_dial_type (OobsIfacePPP  *iface, OobsDialType dial_type);
OobsDialType           oobs_iface_ppp_get_dial_type (OobsIfacePPP *iface);

void                   oobs_iface_ppp_set_ethernet  (OobsIfacePPP      *iface,
						     OobsIfaceEthernet *ethernet);
OobsIfaceEthernet *    oobs_iface_ppp_get_ethernet  (OobsIfacePPP      *iface);

void                   oobs_iface_ppp_set_apn       (OobsIfacePPP      *iface,
						     const gchar       *apn);
G_CONST_RETURN gchar * oobs_iface_ppp_get_apn       (OobsIfacePPP      *iface);


G_END_DECLS

#endif /* __OOBS_IFACE_PPP_H */
