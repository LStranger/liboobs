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

#ifndef __OOBS_OBJECT_H
#define __OOBS_OBJECT_H

G_BEGIN_DECLS

#include <glib-object.h>

#define OOBS_TYPE_OBJECT         (oobs_object_get_type ())
#define OOBS_OBJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_OBJECT, OobsObject))
#define OOBS_OBJECT_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_OBJECT, OobsObjectClass))
#define OOBS_IS_OBJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_OBJECT))
#define OOBS_IS_OBJECT_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_OBJECT))
#define OOBS_OBJECT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_OBJECT, OobsObjectClass))

typedef struct _OobsObject      OobsObject;
typedef struct _OobsObjectClass OobsObjectClass;

struct _OobsObject
{
  GObject parent;
};

struct _OobsObjectClass
{
  GObjectClass parent_class;

  /* virtual methods */
  void (*commit) (OobsObject*, gpointer);
  void (*update) (OobsObject*, gpointer);

  /* signals */
  void (*changing) (OobsObject *object);
  void (*changed)  (OobsObject *object);
};

GType oobs_object_get_type (void);

void  oobs_object_commit (OobsObject *object);
void  oobs_object_update (OobsObject *object);

G_END_DECLS

#endif /* __OOBS_OBJECT_H */
