/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2010 Milan Bouchet-Valat
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
 * Authors: Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#ifndef __OOBS_USER_PRIVATE_H
#define __OOBS_USER_PRIVATE_H

G_BEGIN_DECLS

#include <dbus/dbus.h>

#include "oobs-user.h"

OobsUser *
_oobs_user_create_from_dbus_reply (OobsUser        *user,
                                   gid_t           *gid_ptr,
                                   DBusMessage     *reply,
                                   DBusMessageIter  iter);

G_END_DECLS

#endif /* __OOBS_USER_PRIVATE_H */