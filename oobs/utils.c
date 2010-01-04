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

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include "utils.h"

void
utils_create_dbus_array_from_string_list (GList           *list,
					  DBusMessage     *message,
					  DBusMessageIter *iter)
{
  DBusMessageIter array_iter;

  dbus_message_iter_open_container (iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_TYPE_STRING_AS_STRING,
				    &array_iter);
  while (list)
    {
      dbus_message_iter_append_basic (&array_iter, DBUS_TYPE_STRING, &list->data);
      list = list->next;
    }

  dbus_message_iter_close_container (iter, &array_iter);
}

GList*
utils_get_string_list_from_dbus_reply (DBusMessage     *reply,
				       DBusMessageIter *iter)
{
  DBusMessageIter elem_iter;
  GList *l = NULL;
  gchar *elem;

  dbus_message_iter_recurse (iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRING)
    {
      dbus_message_iter_get_basic (&elem_iter, &elem);
      l = g_list_prepend (l, g_strdup (elem));
      dbus_message_iter_next (&elem_iter);
    }

  dbus_message_iter_next (iter);

  return g_list_reverse (l);
}

gchar*
utils_get_random_string (gint len)
{
  gchar  alphanum[] = "abcdefghijklmnopqrstuvwxyz0AB1CD2EF3GH4IJ5KL6MN7OP8QR9ST0UVWXYZ";
  gchar *str;
  gint   i, alnum_len;

  str = (gchar *) g_malloc0 (len + 1);
  alnum_len = strlen (alphanum);

  for (i = 0; i < len; i++)
    str[i] = alphanum [(gint) (((float) alnum_len * rand ()) / (RAND_MAX + 1.0))];

  return str;
}

void
utils_append_string (DBusMessageIter *iter, const gchar *str)
{
  const gchar *empty_str = "";

  /* allow null strings */
  dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, (str) ? &str : &empty_str); 
}

void
utils_append_int (DBusMessageIter *iter, gint value)
{
  dbus_message_iter_append_basic (iter, DBUS_TYPE_INT32, &value);
}

void
utils_append_uint (DBusMessageIter *iter, guint value)
{
  dbus_message_iter_append_basic (iter, DBUS_TYPE_UINT32, &value);
}

void
utils_append_boolean (DBusMessageIter *iter, gboolean value)
{
  dbus_message_iter_append_basic (iter, DBUS_TYPE_BOOLEAN, &value);
}

static void
utils_get_basic (DBusMessageIter *iter,
		 gint             type,
		 gpointer         value)
{
  if (G_UNLIKELY (dbus_message_iter_get_arg_type (iter) != type))
    {
      g_critical ("Different type while parsing message, found %c, expecting %c\n",
		  dbus_message_iter_get_arg_type (iter), type);
      g_assert_not_reached ();
    }

  dbus_message_iter_get_basic (iter, value);
  dbus_message_iter_next (iter);
}

G_CONST_RETURN gchar*
utils_get_string (DBusMessageIter *iter)
{
  const gchar *str;

  utils_get_basic (iter, DBUS_TYPE_STRING, &str);

  if (str && *str)
    return str;

  return NULL;
}

gchar *
utils_dup_string (DBusMessageIter *iter)
{
  return g_strdup (utils_get_string (iter));
}

gint
utils_get_int (DBusMessageIter *iter)
{
  gint value = 0;

  utils_get_basic (iter, DBUS_TYPE_INT32, &value);
  return value;
}

guint
utils_get_uint (DBusMessageIter *iter)
{
  guint value = 0;

  utils_get_basic (iter, DBUS_TYPE_UINT32, &value);
  return value;
}

gboolean
utils_get_boolean (DBusMessageIter *iter)
{
  gboolean value = FALSE;

  utils_get_basic (iter, DBUS_TYPE_BOOLEAN, &value);
  return value;
}
