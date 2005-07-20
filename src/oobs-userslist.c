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

#include <dbus/dbus.h>
#include <glib-object.h>
#include <string.h>
#include "oobs-object.h"
#include "oobs-list.h"
#include "oobs-userslist.h"
#include "oobs-user.h"

#define USERSLIST_REMOTE_OBJECT "UsersList"

static void oobs_users_list_class_init (OobsUsersListClass *class);
static void oobs_users_list_init       (OobsUsersList      *users_list);
static void oobs_users_list_finalize   (GObject            *object);

static void oobs_users_list_update     (OobsObject   *object,
					gpointer     data);
static void oobs_users_list_commit     (OobsObject   *object,
					gpointer     data);
static GType oobs_users_list_get_content_type (OobsList *list);

G_DEFINE_TYPE (OobsUsersList, oobs_users_list, OOBS_TYPE_USERS_LIST);

static void
oobs_users_list_class_init (OobsUsersListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);
  OobsListClass *oobs_list_class = OOBS_LIST_CLASS (class);

  oobs_users_list_parent_class = g_type_class_peek_parent (class);

  object_class->finalize    = oobs_users_list_finalize;
  oobs_object_class->commit = oobs_users_list_commit;
  oobs_object_class->update = oobs_users_list_update;

  oobs_list_class->get_content_type = oobs_users_list_get_content_type;
}

static void
oobs_users_list_init (OobsUsersList *object)
{
  g_return_if_fail (OOBS_IS_USERS_LIST (object));
}

static void
oobs_users_list_finalize (GObject *object)
{
  OobsUsersList *users_list;

  g_return_if_fail (OOBS_IS_USERS_LIST (object));

  users_list = OOBS_USERS_LIST (object);

  if (G_OBJECT_CLASS (oobs_users_list_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_users_list_parent_class)->finalize) (object);
}

static GType
oobs_users_list_get_content_type (OobsList *list)
{
  return OOBS_TYPE_USER;
}

static OobsUser*
create_user_from_dbus_reply (OobsObject      *object,
			     DBusMessage     *reply,
			     DBusMessageIter  struct_iter)
{
  DBusMessageIter iter;
  int    id, uid, gid;
  gchar *login, *passwd, *home, *shell;

  dbus_message_iter_recurse (&struct_iter, &iter);

  dbus_message_iter_get_basic (&iter, &id);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &login);
  dbus_message_iter_next (&iter);
  
  dbus_message_iter_get_basic (&iter, &passwd);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &uid);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &gid);
  dbus_message_iter_next (&iter);

  /* FIXME: missing GECOS fields */
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &home);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &shell);
  dbus_message_iter_next (&iter);

  return g_object_new (OOBS_TYPE_USER,
		       "username",       login,
		       "password",       passwd,
		       "uid",            uid,
		       "gid",            gid,
		       "home-directory", home,
		       "shell",          shell,
		       NULL);
}

static void
oobs_users_list_update (OobsObject *object, gpointer data)
{
  DBusMessage      *reply = (DBusMessage *) data;
  DBusMessageIter   iter, elem_iter;
  OobsList         *list;
  OobsListIter      list_iter;
  GObject          *user;

  list = OOBS_LIST (object);

  /* First of all, free the previous list */
  oobs_list_clear (list);

  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      user = G_OBJECT (create_user_from_dbus_reply (object, reply, elem_iter));

      oobs_list_append (list, &list_iter);
      oobs_list_set    (list, &list_iter, G_OBJECT (user));
      g_object_unref   (user);

      dbus_message_iter_next (&elem_iter);
    }
}

static void
oobs_users_list_commit (OobsObject *object, gpointer data)
{
}

OobsObject*
oobs_users_list_new (OobsSession *session)
{
  OobsObject *object;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  object = g_object_new (OOBS_TYPE_USERS_LIST,
			 "remote-object", USERSLIST_REMOTE_OBJECT,
			 "session",       session,
			 NULL);
  oobs_object_update (object);
  return object;
}
