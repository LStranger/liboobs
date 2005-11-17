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

#ifndef __OOBS_TIME_CONFIG_H
#define __OOBS_TIME_CONFIG_H

G_BEGIN_DECLS


#include <glib-object.h>
#include "oobs-object.h"
#include "oobs-session.h"

#define OOBS_TYPE_TIME_CONFIG         (oobs_time_config_get_type ())
#define OOBS_TIME_CONFIG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_TIME_CONFIG, OobsTimeConfig))
#define OOBS_TIME_CONFIG_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_TIME_CONFIG, OobsTimeConfigClass))
#define OOBS_IS_TIME_CONFIG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_TIME_CONFIG))
#define OOBS_IS_TIME_CONFIG_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_TIME_CONFIG))
#define OOBS_TIME_CONFIG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_TIME_CONFIG, OobsTimeConfigClass))

typedef struct _OobsTimeConfig      OobsTimeConfig;
typedef struct _OobsTimeConfigClass OobsTimeConfigClass;

struct _OobsTimeConfig
{
  OobsObject parent;
};

struct _OobsTimeConfigClass
{
  OobsObjectClass parent_class;
};

GType       oobs_time_config_get_type      (void);

OobsObject* oobs_time_config_get           (OobsSession *session);

glong       oobs_time_config_get_unix_time (OobsTimeConfig *config);
void        oobs_time_config_set_unix_time (OobsTimeConfig *config, glong unix_time);

void        oobs_time_config_get_time      (OobsTimeConfig *config, gint *year, gint *month, gint *day, gint *hour, gint *minute, gint *second);
void        oobs_time_config_set_time      (OobsTimeConfig *config, gint  year, gint  month, gint  day, gint  hour, gint  minute, gint  second);

G_CONST_RETURN gchar* oobs_time_get_timezone (OobsTimeConfig *config);
void oobs_time_set_timezone (OobsTimeConfig *config, const gchar *timezone);


G_END_DECLS

#endif /* __OOBS_TIME_CONFIG_H */
