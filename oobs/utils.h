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

#ifndef __OOBS_UTILS_H__
#define __OOBS_UTILS_H__

G_BEGIN_DECLS

#include <dbus/dbus.h>
#include <glib.h>

void   utils_create_dbus_array_from_string_list (GList *list, DBusMessage *message, DBusMessageIter *iter);
GList *utils_get_string_list_from_dbus_reply    (DBusMessage *reply, DBusMessageIter *iter);
gchar *utils_get_random_string                  (gint len);

void   utils_append_string                      (DBusMessageIter *iter, const gchar *str);
void   utils_append_int                         (DBusMessageIter *iter, gint value);
void   utils_append_uint                        (DBusMessageIter *iter, guint value);
void   utils_append_boolean                     (DBusMessageIter *iter, gboolean value);

G_CONST_RETURN gchar* utils_get_string          (DBusMessageIter *iter);
gchar *               utils_dup_string          (DBusMessageIter *iter);
gint     utils_get_int                          (DBusMessageIter *iter);
guint    utils_get_uint                         (DBusMessageIter *iter);
gboolean utils_get_boolean                      (DBusMessageIter *iter);

G_END_DECLS

#endif /* __OOBS_UTILS_H__ */
