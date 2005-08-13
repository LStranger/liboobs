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
#include "oobs-shellslist.h"
#include "oobs-shell.h"

#define SHELLSLIST_REMOTE_OBJECT "ShellsList"

static void oobs_shells_list_class_init (OobsShellsListClass *class);
static void oobs_shells_list_init       (OobsShellsList      *shells_list);
static void oobs_shells_list_finalize   (GObject             *object);

static void oobs_shells_list_update     (OobsObject   *object,
					 gpointer     data);
static void oobs_shells_list_commit     (OobsObject   *object,
					 gpointer     data);
static GType oobs_shells_list_get_content_type (OobsList *list);

G_DEFINE_TYPE (OobsShellsList, oobs_shells_list, OOBS_TYPE_SHELLS_LIST);

static void
oobs_shells_list_class_init (OobsShellsListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);
  OobsListClass *oobs_list_class = OOBS_LIST_CLASS (class);

  object_class->finalize    = oobs_shells_list_finalize;
  oobs_object_class->commit = oobs_shells_list_commit;
  oobs_object_class->update = oobs_shells_list_update;

  oobs_list_class->get_content_type = oobs_shells_list_get_content_type;
}

static void
oobs_shells_list_init (OobsShellsList *object)
{
  g_return_if_fail (OOBS_IS_SHELLS_LIST (object));
}

static void
oobs_shells_list_finalize (GObject *object)
{
  OobsShellsList *shells_list;

  g_return_if_fail (OOBS_IS_SHELLS_LIST (object));

  shells_list = OOBS_SHELLS_LIST (object);

  if (G_OBJECT_CLASS (oobs_shells_list_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_shells_list_parent_class)->finalize) (object);
}

static GType
oobs_shells_list_get_content_type (OobsList *list)
{
  return OOBS_TYPE_SHELL;
}

static void
oobs_shells_list_update (OobsObject *object, gpointer data)
{
  DBusMessage     *reply = (DBusMessage *) data;
  DBusMessageIter  iter, array_iter;
  GObject         *sh;
  OobsList        *list;
  OobsListIter     list_iter;
  char            *shell;

  list = OOBS_LIST (object);

  /* First of all, free the previous shares list */
  oobs_list_clear (list);

  /* start recursing through the response array */
  dbus_message_iter_init    (reply, &iter);
  dbus_message_iter_recurse (&iter, &array_iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRING)
    {
      dbus_message_iter_get_basic (&array_iter, &shell);
      sh = G_OBJECT (oobs_shell_new (shell));

      oobs_list_append (list, &list_iter);
      oobs_list_set    (list, &list_iter, G_OBJECT (sh));
      g_object_unref   (sh);

      dbus_message_iter_next (&array_iter);
    }
}

static void
oobs_shells_list_commit (OobsObject *object, gpointer data)
{
}

OobsObject*
oobs_shells_list_new (OobsSession *session)
{
  OobsObject *object;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  object = g_object_new (OOBS_TYPE_SHELLS_LIST,
			 "remote-object", SHELLSLIST_REMOTE_OBJECT,
			 "session",       session,
			 NULL);

  oobs_object_update (object);
  return object;
}
