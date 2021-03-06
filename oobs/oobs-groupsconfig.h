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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>,
 *          Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#ifndef __OOBS_GROUPS_CONFIG_H
#define __OOBS_GROUPS_CONFIG_H

G_BEGIN_DECLS

#include "oobs-object.h"
#include "oobs-list.h"
#include "oobs-group.h"

#define OOBS_TYPE_GROUPS_CONFIG         (oobs_groups_config_get_type ())
#define OOBS_GROUPS_CONFIG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_GROUPS_CONFIG, OobsGroupsConfig))
#define OOBS_GROUPS_CONFIG_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_GROUPS_CONFIG, OobsGroupsConfigClass))
#define OOBS_IS_GROUPS_CONFIG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_GROUPS_CONFIG))
#define OOBS_IS_GROUPS_CONFIG_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_GROUPS_CONFIG))
#define OOBS_GROUPS_CONFIG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_GROUPS_CONFIG, OobsGroupsConfigClass))

typedef struct _OobsGroupsConfig      OobsGroupsConfig;
typedef struct _OobsGroupsConfigClass OobsGroupsConfigClass;

struct _OobsGroupsConfig
{
  OobsObject parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsGroupsConfigClass
{
  OobsObjectClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
  void (*_oobs_padding3) (void);
  void (*_oobs_padding4) (void);
};

GType       oobs_groups_config_get_type      (void);

OobsObject* oobs_groups_config_get           (void);
OobsList*   oobs_groups_config_get_groups    (OobsGroupsConfig *config);

OobsResult  oobs_groups_config_add_group     (OobsGroupsConfig *config,
                                              OobsGroup        *group);
OobsResult  oobs_groups_config_delete_group  (OobsGroupsConfig *config,
                                              OobsGroup        *group);

OobsGroup*  oobs_groups_config_get_from_name (OobsGroupsConfig *config,
                                              const gchar      *name);
OobsGroup*  oobs_groups_config_get_from_gid  (OobsGroupsConfig *config,
                                              gid_t             gid);

gboolean    oobs_groups_config_is_name_used  (OobsGroupsConfig *config,
                                              const gchar      *name);
gboolean    oobs_groups_config_is_gid_used   (OobsGroupsConfig *config,
                                              gid_t             gid);

gid_t       oobs_groups_config_find_free_gid (OobsGroupsConfig *config,
                                              gid_t             gid_min,
                                              gid_t             gid_max);

G_END_DECLS

#endif /* __OOBS_GROUPS_CONFIG_H */
