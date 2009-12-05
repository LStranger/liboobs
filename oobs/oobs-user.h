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

#include "oobs-group.h"
	
struct _OobsUser {
  GObject parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsUserClass {
  GObjectClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
};

GType oobs_user_get_type (void);

OobsUser* oobs_user_new (const gchar *name);

G_CONST_RETURN gchar* oobs_user_get_login_name (OobsUser *user);

void  oobs_user_set_password (OobsUser *user, const gchar *password);
void  oobs_user_set_crypted_password (OobsUser *user, const gchar *crypted_password);

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

gboolean oobs_user_get_active  (OobsUser *user);
gboolean oobs_user_is_root     (OobsUser *user);
gboolean oobs_user_is_in_group (OobsUser *user, OobsGroup *group);

G_END_DECLS

#endif /* __OOBS_USER_H__ */
