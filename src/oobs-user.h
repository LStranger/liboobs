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

#ifndef __OOBS_USER_H__
#define __OOBS_USER_H__

G_BEGIN_DECLS

#include "oobs-object.h"

#define OOBS_TYPE_USER         (oobs_user_get_type())
#define OOBS_USER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_USER, OobsUser))
#define OOBS_USER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_USER, OobsUserClass))
#define OOBS_IS_USER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_USER))
#define OOBS_IS_USER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_USER))
#define OOBS_USER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_USER, OobsUserClass))

typedef struct _OobsUser        OobsUser;
typedef struct _OobsUserClass   OobsUserClass;
	
struct _OobsUser {
  GObject parent;
};

struct _OobsUserClass {
  GObjectClass parent_class;
};

GType oobs_user_get_type (void);

OobsUser* oobs_user_new (void);

G_END_DECLS

#endif /* __OOBS_USER_H__ */
