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

#ifndef __OOBS_SERVICES_CONFIG_H
#define __OOBS_SERVICES_CONFIG_H

G_BEGIN_DECLS

#include "oobs-object.h"
#include "oobs-list.h"
#include "oobs-session.h"

#define OOBS_TYPE_SERVICES_CONFIG         (oobs_services_config_get_type ())
#define OOBS_SERVICES_CONFIG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_SERVICES_CONFIG, OobsServicesConfig))
#define OOBS_SERVICES_CONFIG_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_SERVICES_CONFIG, OobsServicesConfigClass))
#define OOBS_IS_SERVICES_CONFIG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_SERVICES_CONFIG))
#define OOBS_IS_SERVICES_CONFIG_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_SERVICES_CONFIG))
#define OOBS_SERVICES_CONFIG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_SERVICES_CONFIG, OobsServicesConfigClass))

typedef struct _OobsServicesConfig      OobsServicesConfig;
typedef struct _OobsServicesConfigClass OobsServicesConfigClass;
typedef struct _OobsServicesRunlevel    OobsServicesRunlevel;

typedef enum
{
  OOBS_RUNLEVEL_HALT,
  OOBS_RUNLEVEL_REBOOT,
  OOBS_RUNLEVEL_MONOUSER,
  OOBS_RUNLEVEL_MULTIUSER
} OobsRunlevelRole;

struct _OobsServicesRunlevel
{
  gchar *name;
  guint role;
};

struct _OobsServicesConfig
{
  OobsObject parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsServicesConfigClass
{
  OobsObjectClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
  void (*_oobs_padding3) (void);
  void (*_oobs_padding4) (void);
};

GType       oobs_services_config_get_type      (void);

OobsObject* oobs_services_config_get           (OobsSession *session);
OobsList*   oobs_services_config_get_services  (OobsServicesConfig *config);

GList*      oobs_services_config_get_runlevels (OobsServicesConfig *config);
G_CONST_RETURN OobsServicesRunlevel* oobs_services_config_get_default_runlevel (OobsServicesConfig *config);


G_END_DECLS

#endif /* __OOBS_SERVICES_CONFIG_H */
