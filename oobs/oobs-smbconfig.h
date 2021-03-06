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

#ifndef __OOBS_SMB_CONFIG_H
#define __OOBS_SMB_CONFIG_H

G_BEGIN_DECLS

#include <glib-object.h>
#include "oobs-object.h"
#include "oobs-list.h"
#include "oobs-share.h"
#include "oobs-user.h"

#define OOBS_TYPE_SMB_CONFIG         (oobs_smb_config_get_type ())
#define OOBS_SMB_CONFIG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_SMB_CONFIG, OobsSMBConfig))
#define OOBS_SMB_CONFIG_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_SMB_CONFIG, OobsSMBConfigClass))
#define OOBS_IS_SMB_CONFIG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_SMB_CONFIG))
#define OOBS_IS_SMB_CONFIG_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_SMB_CONFIG))
#define OOBS_SMB_CONFIG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_SMB_CONFIG, OobsSMBConfigClass))

typedef struct _OobsSMBConfig      OobsSMBConfig;
typedef struct _OobsSMBConfigClass OobsSMBConfigClass;

struct _OobsSMBConfig
{
  OobsObject parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsSMBConfigClass
{
  OobsObjectClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
  void (*_oobs_padding3) (void);
  void (*_oobs_padding4) (void);
};

GType       oobs_smb_config_get_type     (void);

OobsObject* oobs_smb_config_get          (void);

OobsList*   oobs_smb_config_get_shares   (OobsSMBConfig *config);

G_CONST_RETURN gchar* oobs_smb_config_get_workgroup      (OobsSMBConfig *config);
void                  oobs_smb_config_set_workgroup      (OobsSMBConfig *config,
							  const gchar   *workgroup);

G_CONST_RETURN gchar* oobs_smb_config_get_description    (OobsSMBConfig *config);
void                  oobs_smb_config_set_description    (OobsSMBConfig *config,
							  const gchar   *description);

gboolean              oobs_smb_config_get_is_wins_server (OobsSMBConfig *config);
void                  oobs_smb_config_set_is_wins_server (OobsSMBConfig *config,
							  gboolean       is_wins_server);

G_CONST_RETURN gchar* oobs_smb_config_get_wins_server    (OobsSMBConfig *config);
void                  oobs_smb_config_set_wins_server    (OobsSMBConfig *config,
							  const gchar   *wins_server);

gboolean              oobs_smb_config_user_has_password    (OobsSMBConfig *config,
							    OobsUser      *user);
void                  oobs_smb_config_delete_user_password (OobsSMBConfig *config,
							    OobsUser      *user);
void                  oobs_smb_config_set_user_password    (OobsSMBConfig *config,
							    OobsUser      *user,
							    const gchar   *password);

G_END_DECLS

#endif /* __OOBS_SMB_CONFIG_H */
