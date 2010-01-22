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

#ifndef __OOBS_ERROR_H
#define __OOBS_ERROR_H

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * OOBS_ERROR:
 *
 * Error domain for errors when using Oobs. Errors in this domain will be from the #OobsError
 * enumeration. See #GError for information on error domains
 */
#define OOBS_ERROR oobs_error_quark()

GQuark oobs_error_quark (void);

/**
 * OobsError:
 * @OOBS_ERROR_AUTHENTICATION_FAILED: Error in the backends when authenticating via PolicyKit.
 * @OOBS_ERROR_AUTHENTICATION_CANCELLED: User manually cancelled the authentication.
 *
 * Error codes possible with liboobs.
 */
typedef enum
{
  OOBS_ERROR_AUTHENTICATION_FAILED,
  OOBS_ERROR_AUTHENTICATION_CANCELLED,
} OobsError;

G_END_DECLS

#endif /* __OOBS_ERROR_H */
