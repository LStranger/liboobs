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

#ifndef __OOBS_NTP_SERVER_H__
#define __OOBS_NTP_SERVER_H__

G_BEGIN_DECLS

#include "oobs-object.h"

#define OOBS_TYPE_NTP_SERVER         (oobs_ntp_server_get_type())
#define OOBS_NTP_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_NTP_SERVER, OobsNTPServer))
#define OOBS_NTP_SERVER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_NTP_SERVER, OobsNTPServerClass))
#define OOBS_IS_NTP_SERVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_NTP_SERVER))
#define OOBS_IS_NTP_SERVER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_NTP_SERVER))
#define OOBS_NTP_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_NTP_SERVER, OobsNTPServerClass))

typedef struct _OobsNTPServer        OobsNTPServer;
typedef struct _OobsNTPServerClass   OobsNTPServerClass;
	
struct _OobsNTPServer {
  GObject parent;
};

struct _OobsNTPServerClass {
  GObjectClass parent_class;
};

GType                  oobs_ntp_server_get_type   (void);

OobsNTPServer*         oobs_ntp_server_new        (const gchar *hostname);

G_CONST_RETURN gchar*  oobs_ntp_server_get_path   (OobsNTPServer *ntp_server);
void                   oobs_ntp_server_set_path   (OobsNTPServer *ntp_server, const gchar *hostname);


G_END_DECLS

#endif /* __OOBS_NTP_SERVER_H__ */
