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

#ifndef __OOBS_USERS_CONFIG_H
#define __OOBS_USERS_CONFIG_H

G_BEGIN_DECLS

#include <glib-object.h>
#include "oobs-object.h"
#include "oobs-list.h"
#include "oobs-session.h"

#define OOBS_TYPE_USERS_CONFIG         (oobs_users_config_get_type ())
#define OOBS_USERS_CONFIG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_USERS_CONFIG, OobsUsersConfig))
#define OOBS_USERS_CONFIG_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_USERS_CONFIG, OobsUsersConfigClass))
#define OOBS_IS_USERS_CONFIG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_USERS_CONFIG))
#define OOBS_IS_USERS_CONFIG_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_USERS_CONFIG))
#define OOBS_USERS_CONFIG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_USERS_CONFIG, OobsUsersConfigClass))

typedef struct _OobsUsersConfig      OobsUsersConfig;
typedef struct _OobsUsersConfigClass OobsUsersConfigClass;

struct _OobsUsersConfig
{
  OobsObject parent;
};

struct _OobsUsersConfigClass
{
  OobsObjectClass parent_class;
};

GType       oobs_users_config_get_type     (void);

OobsObject* oobs_users_config_new          (OobsSession *session);
OobsList*   oobs_users_config_get_users    (OobsUsersConfig *config);


G_END_DECLS

#endif /* __OOBS_USERS_CONFIG_H */
