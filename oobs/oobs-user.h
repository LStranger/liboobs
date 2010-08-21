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

#ifndef __OOBS_USER_H__
#define __OOBS_USER_H__

G_BEGIN_DECLS

#include <sys/types.h>


#define OOBS_TYPE_USER         (oobs_user_get_type())
#define OOBS_USER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_USER, OobsUser))
#define OOBS_USER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_USER, OobsUserClass))
#define OOBS_IS_USER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_USER))
#define OOBS_IS_USER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_USER))
#define OOBS_USER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_USER, OobsUserClass))

typedef struct _OobsUser        OobsUser;
typedef struct _OobsUserClass   OobsUserClass;

#include "oobs-object.h"
#include "oobs-group.h"
#include "oobs-enum-types.h"
	
struct _OobsUser {
  OobsObject parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsUserClass {
  OobsObjectClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
  void (*_oobs_padding3) (void);
  void (*_oobs_padding4) (void);
};

/**
 * OobsUserHomeFlags:
 * @OOBS_USER_REMOVE_HOME: When removing an #OobsUser, also remove home dir; when
 *     committing, if home dir changes, remove old home dir.
 * @OOBS_USER_CHOWN_HOME: When creating or committing an #OobsUser, recursively
 *     change the owner of home dir to user and main group.
 * @OOBS_USER_COPY_HOME: When committing an #OobsUser and if home dir changes,
 *     copy old home to new path, overwriting files if needed (!).
 * @OOBS_USER_ERASE_HOME: When creating an #OobsUser and a path for home dir is given,
 *     or when committing a user and home dir changes, delete already present directory.
 *
 * Determine special behaviors regarding the home directory. Flags only apply to
 * some operations (commit, add or delete user), and have no effect in other cases.
 */
typedef enum {
  OOBS_USER_REMOVE_HOME  = 1,
  OOBS_USER_CHOWN_HOME   = 1 << 1,
  OOBS_USER_COPY_HOME    = 1 << 2,
  OOBS_USER_ERASE_HOME   = 1 << 3
} OobsUserHomeFlags;

GType oobs_user_get_type (void);

OobsUser* oobs_user_new (const gchar *name);

G_CONST_RETURN gchar* oobs_user_get_login_name (OobsUser *user);

void  oobs_user_set_password (OobsUser *user, const gchar *password);

uid_t oobs_user_get_uid (OobsUser *user);
void  oobs_user_set_uid (OobsUser *user, uid_t uid);

OobsGroup* oobs_user_get_main_group (OobsUser *user);
void       oobs_user_set_main_group (OobsUser  *user, OobsGroup *main_group);

G_CONST_RETURN gchar* oobs_user_get_home_directory (OobsUser *user);
void oobs_user_set_home_directory (OobsUser *user, const gchar *home_directory);

G_CONST_RETURN gchar* oobs_user_get_shell (OobsUser *user);
void oobs_user_set_shell (OobsUser *user, const gchar *shell);

G_CONST_RETURN gchar* oobs_user_get_full_name (OobsUser *user);
void oobs_user_set_full_name (OobsUser *user, const gchar *full_name);

G_CONST_RETURN gchar* oobs_user_get_room_number (OobsUser *user);
void oobs_user_set_room_number (OobsUser *user, const gchar *room_number);

G_CONST_RETURN gchar* oobs_user_get_work_phone_number (OobsUser *user);
void oobs_user_set_work_phone_number (OobsUser *user, const gchar *phone_number);

G_CONST_RETURN gchar* oobs_user_get_home_phone_number (OobsUser *user);
void oobs_user_set_home_phone_number (OobsUser *user, const gchar *phone_number);

G_CONST_RETURN gchar* oobs_user_get_other_data (OobsUser *user);
void oobs_user_set_other_data (OobsUser *user, const gchar *data);

gboolean oobs_user_get_password_empty (OobsUser *user);
void     oobs_user_set_password_empty (OobsUser *user, gboolean empty);

gboolean oobs_user_get_password_disabled (OobsUser *user);
void     oobs_user_set_password_disabled (OobsUser *user, gboolean disabled);

gboolean oobs_user_get_encrypted_home (OobsUser *user);
void     oobs_user_set_encrypted_home (OobsUser *user, gboolean encrypted_home);

void     oobs_user_set_home_flags (OobsUser *user, OobsUserHomeFlags home_flags);

G_CONST_RETURN gchar* oobs_user_get_locale (OobsUser *user);
void                  oobs_user_set_locale (OobsUser *user, const gchar *locale);

gboolean oobs_user_get_active  (OobsUser *user);
gboolean oobs_user_is_root     (OobsUser *user);
gboolean oobs_user_is_in_group (OobsUser *user, OobsGroup *group);

G_END_DECLS

#endif /* __OOBS_USER_H__ */
