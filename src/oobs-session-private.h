/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
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

#ifndef __OOBS_SESSION_PRIVATE_H
#define __OOBS_SESSION_PRIVATE_H

G_BEGIN_DECLS

#define OOBS_DBUS_DESTINATION   "org.freedesktop.SystemToolsBackends"
#define OOBS_DBUS_PATH_PREFIX   "/org/freedesktop/SystemToolsBackends"
#define OOBS_DBUS_METHOD_PREFIX "org.freedesktop.SystemToolsBackends"

DBusConnection* _oobs_session_get_connection_bus (OobsSession *session);

void            _oobs_session_register_object    (OobsSession *session,
						  OobsObject  *object);
void            _oobs_session_unregister_object  (OobsSession *session,
						  OobsObject  *object);

G_END_DECLS

#endif /* __OOBS_SESSION_PRIVATE_H */
