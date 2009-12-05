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

#include <math.h>

/* The system-tools-backends protocol is using an int32
 * to store [UG]IDs, thus we can't go beyond this,
 * even if the system supports it.
 *
 * Computing the size of [ug]id_t is tricky, because
 * no system constant exists for that.*/

#define OOBS_MAX_UID MIN (G_MAXINT32, pow (2, 8 * sizeof (uid_t) - 1))
#define OOBS_MAX_GID MIN (G_MAXINT32, pow (2, 8 * sizeof (gid_t) - 1))
