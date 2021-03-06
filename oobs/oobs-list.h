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

#ifndef __OOBS_LIST_H
#define __OOBS_LIST_H

G_BEGIN_DECLS

#include <glib-object.h>

#define OOBS_TYPE_LIST         (oobs_list_get_type ())
#define OOBS_TYPE_LIST_ITER    (oobs_list_iter_get_type ())
#define OOBS_LIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_LIST, OobsList))
#define OOBS_LIST_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_LIST, OobsListClass))
#define OOBS_IS_LIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_LIST))
#define OOBS_IS_LIST_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_LIST))
#define OOBS_LIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_LIST, OobsListClass))

typedef struct _OobsList      OobsList;
typedef struct _OobsListClass OobsListClass;
typedef struct _OobsListIter  OobsListIter;

struct _OobsList
{
  GObject parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsListClass
{
  GObjectClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
};

struct _OobsListIter
{
  guint stamp;
  gpointer data;
};

GType      oobs_list_get_type       (void);
GType      oobs_list_iter_get_type  (void);

gboolean   oobs_list_get_iter_first (OobsList *list, OobsListIter *iter);
gboolean   oobs_list_iter_next      (OobsList *list, OobsListIter *iter);

gboolean   oobs_list_remove         (OobsList *list, OobsListIter *iter);

void       oobs_list_append         (OobsList *list, OobsListIter *iter);
void       oobs_list_prepend        (OobsList *list, OobsListIter *iter);
void       oobs_list_insert_after   (OobsList *list, OobsListIter *anchor, OobsListIter *iter);
void       oobs_list_insert_before  (OobsList *list, OobsListIter *anchor, OobsListIter *iter);

GObject*   oobs_list_get            (OobsList *list, OobsListIter *iter);
void       oobs_list_set            (OobsList *list, OobsListIter *iter, gpointer data);

void       oobs_list_clear          (OobsList *list);
gint       oobs_list_get_n_items    (OobsList *list);

OobsListIter *oobs_list_iter_copy   (OobsListIter *iter);
void          oobs_list_iter_free   (OobsListIter *iter);


G_END_DECLS

#endif /* __OOBS_LIST_H */
