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

#ifndef __OOBS_IFACE_ISDN_H
#define __OOBS_IFACE_ISDN_H

G_BEGIN_DECLS

#include "oobs-iface.h"

#define OOBS_TYPE_IFACE_ISDN           (oobs_iface_isdn_get_type ())
#define OOBS_IFACE_ISDN(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), OOBS_TYPE_IFACE_ISDN, OobsIfaceISDN))
#define OOBS_IFACE_ISDN_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    OOBS_TYPE_IFACE_ISDN, OobsIfaceISDNClass))
#define OOBS_IS_IFACE_ISDN(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OOBS_TYPE_IFACE_ISDN))
#define OOBS_IS_IFACE_ISDN_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    OOBS_TYPE_IFACE_ISDN))
#define OOBS_IFACE_ISDN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  OOBS_TYPE_IFACE_ISDN, OobsIfaceISDNClass))

typedef struct _OobsIfaceISDN      OobsIfaceISDN;
typedef struct _OobsIfaceISDNClass OobsIfaceISDNClass;

struct _OobsIfaceISDN
{
  OobsIface parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsIfaceISDNClass
{
  OobsIfaceClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
};

GType oobs_iface_isdn_get_type ();

void                   oobs_iface_isdn_set_login (OobsIfaceISDN *iface, const gchar *login);
G_CONST_RETURN gchar*  oobs_iface_isdn_get_login (OobsIfaceISDN *iface);

void                   oobs_iface_isdn_set_password (OobsIfaceISDN *iface, const gchar *password);

void                   oobs_iface_isdn_set_phone_number (OobsIfaceISDN *iface, const gchar *phone_number);
G_CONST_RETURN gchar*  oobs_iface_isdn_get_phone_number (OobsIfaceISDN *iface);

void                   oobs_iface_isdn_set_phone_prefix (OobsIfaceISDN *iface, const gchar *phone_prefix);
G_CONST_RETURN gchar*  oobs_iface_isdn_get_phone_prefix (OobsIfaceISDN *iface);

void                   oobs_iface_isdn_set_default_gateway (OobsIfaceISDN *iface, gboolean default_gw);
gboolean               oobs_iface_isdn_get_default_gateway (OobsIfaceISDN *iface);

void                   oobs_iface_isdn_set_use_peer_dns (OobsIfaceISDN *iface, gboolean use_peer_dns);
gboolean               oobs_iface_isdn_get_use_peer_dns (OobsIfaceISDN *iface);

void                   oobs_iface_isdn_set_persistent (OobsIfaceISDN *iface, gboolean persistent);
gboolean               oobs_iface_isdn_get_persistent (OobsIfaceISDN *iface);

void                   oobs_iface_isdn_set_peer_noauth (OobsIfaceISDN *iface, gboolean use_peer_dns);
gboolean               oobs_iface_isdn_get_peer_noauth (OobsIfaceISDN *iface);


G_END_DECLS

#endif /* __OOBS_IFACE_ISDN_H */
