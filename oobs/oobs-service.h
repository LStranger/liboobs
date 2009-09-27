/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2005 Carlos Garnacho
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

#ifndef __OOBS_SERVICE_H__
#define __OOBS_SERVICE_H__

G_BEGIN_DECLS

#include "oobs-servicesconfig.h"


#define OOBS_TYPE_SERVICE         (oobs_service_get_type())
#define OOBS_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_SERVICE, OobsService))
#define OOBS_SERVICE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_SERVICE, OobsServiceClass))
#define OOBS_IS_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_SERVICE))
#define OOBS_IS_SERVICE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_SERVICE))
#define OOBS_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_SERVICE, OobsServiceClass))

typedef struct _OobsService      OobsService;
typedef struct _OobsServiceClass OobsServiceClass;

typedef enum
{
  OOBS_SERVICE_START,
  OOBS_SERVICE_STOP,
  /* For services not listed in that runlevel */
  OOBS_SERVICE_IGNORE
} OobsServiceStatus;

struct _OobsService {
  GObject parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsServiceClass {
  GObjectClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
};

GType oobs_service_get_type (void);

void oobs_service_set_runlevel_configuration     (OobsService          *service,
						  OobsServicesRunlevel *runlevel,
						  OobsServiceStatus     status,
						  gint                  priority);
void oobs_service_get_runlevel_configuration     (OobsService          *service,
						  OobsServicesRunlevel *runlevel,
						  OobsServiceStatus    *status,
						  gint                 *priority);
G_CONST_RETURN gchar* oobs_service_get_name      (OobsService *service);


G_END_DECLS

#endif /* __OOBS_SERVICE_H__ */
