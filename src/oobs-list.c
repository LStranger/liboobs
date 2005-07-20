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

#define OOBS_LIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_LIST, OobsListPrivate))

typedef struct _OobsListPrivate OobsListPrivate;

struct _OobsListPrivate
{
  GList *list;
  guint  stamp;
};

static void oobs_list_class_init (OobsListClass *class);
static void oobs_list_init       (OobsList      *list);
static void oobs_list_finalize   (GObject      *object);

G_DEFINE_TYPE (OobsList, oobs_list, OOBS_TYPE_LIST);

static void
oobs_list_class_init (OobsListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  oobs_list_parent_class = g_type_class_peek_parent (class);

  class->get_content_type = NULL;
  object_class->finalize = oobs_list_finalize;

  g_type_class_add_private (object_class,
			    sizeof (OobsListPrivate));
}

static void
oobs_list_init (OobsList *object)
{
  OobsListPrivate *priv;

  g_return_if_fail (OOBS_IS_LIST (object));
  priv = OOBS_LIST_GET_PRIVATE (object);

  priv->stamp = 0;
  priv->list  = NULL;
}

static void
free_list (OobsList *list)
{
  OobsListPrivate *priv;

  g_return_if_fail (OOBS_IS_LIST (list));
  priv = OOBS_LIST_GET_PRIVATE (list);

  g_list_foreach (priv->list, (GFunc) g_object_unref, NULL);
  g_list_free    (priv->list);
  priv->list = NULL;
}

static void
oobs_list_finalize (GObject *object)
{
  OobsList *list;
  OobsListPrivate *priv;

  g_return_if_fail (OOBS_IS_LIST (object));

  list = OOBS_LIST (object);
  priv = OOBS_LIST_GET_PRIVATE (list);

  if (priv)
    free_list (list);

  if (G_OBJECT_CLASS (oobs_list_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_list_parent_class)->finalize) (object);
}

static gboolean
check_iter (OobsListPrivate *priv, OobsListIter *iter)
{
  if (priv->stamp != iter->stamp)
    {
      g_warning ("OobsList stamp and OobsListIter stamp differ");
      return FALSE;
    }

  if (g_list_position (priv->list, iter->data) == -1)
    return FALSE;

  return TRUE;
}

gboolean
oobs_list_get_iter_first (OobsList *list, OobsListIter *iter)
{
  OobsListPrivate *priv;

  g_return_val_if_fail (list != NULL, FALSE);
  g_return_val_if_fail (OOBS_IS_LIST (list), FALSE);

  priv = OOBS_LIST_GET_PRIVATE (list);

  if (!priv->list)
    return FALSE;

  iter->stamp = priv->stamp;
  iter->data  = priv->list;
  
  return TRUE;
}

gboolean
oobs_list_iter_next (OobsList *list, OobsListIter *iter)
{
  OobsListPrivate *priv;
  GList *data;

  g_return_val_if_fail (list != NULL, FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (iter->data != NULL, FALSE);
  g_return_val_if_fail (OOBS_IS_LIST (list), FALSE);

  priv = OOBS_LIST_GET_PRIVATE (list);

  if (!check_iter (priv, iter))
    return FALSE;

  data = (GList *) iter->data;
  iter->data = data->next;
  return (iter->data != NULL);
}

gboolean
oobs_list_remove_iter (OobsList *list, OobsListIter *iter)
{
  OobsListPrivate *priv;
  GList *data;

  g_return_val_if_fail (list != NULL, FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (iter->data != NULL, FALSE);
  g_return_val_if_fail (OOBS_IS_LIST (list), FALSE);

  priv = OOBS_LIST_GET_PRIVATE (list);

  if (!check_iter (priv, iter))
    return FALSE;

  data = (GList *) iter->data;
  iter->data = data->next;
  priv->list = g_list_delete_link (priv->list, data);

  return (iter->data != NULL);
}

void
oobs_list_append (OobsList *list, OobsListIter *iter)
{
  OobsListPrivate *priv;
  GList *node, *l;

  g_return_if_fail (list != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);
  l = priv->list;

  node = g_list_alloc ();
  node->data = NULL;

  /* append the node */
  if (!l)
    {
      /* Change the stamp if the list was empty */
      priv->stamp++;
      priv->list = node;
    }
  else
    {
      while (l->next)
	l = l->next;

      l->next = node;
    }

  iter->stamp = priv->stamp;
  iter->data  = node;
}

void
oobs_list_prepend (OobsList *list, OobsListIter *iter)
{
  OobsListPrivate *priv;
  GList *node;

  g_return_if_fail (list != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);

  /* Change the stamp if the list was empty */
  if (!priv->list)
    priv->stamp++;

  node = g_list_alloc ();
  node->data = NULL;
  node->next = priv->list;
  priv->list = node;

  iter->stamp = priv->stamp;
  iter->data  = node;
}

void
oobs_list_insert_after (OobsList     *list,
			OobsListIter *anchor,
			OobsListIter *iter)
{
  OobsListPrivate *priv;
  GList *node, *anchor_node;

  g_return_if_fail (list != NULL);
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (anchor->data != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);
  
  if (!check_iter (priv, anchor))
    return;

  anchor_node = anchor->data;
  node = g_list_alloc ();
  node->data = NULL;
  
  anchor_node->next->prev = node;
  node->next = anchor_node->next;
  anchor_node->next = node;
  node->prev = anchor_node;

  iter->stamp = priv->stamp;
  iter->data  = node;
}

void
oobs_list_insert_before (OobsList     *list,
			 OobsListIter *anchor,
			 OobsListIter *iter)
{
  OobsListPrivate *priv;
  GList *node, *anchor_node;

  g_return_if_fail (list != NULL);
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (anchor->data != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);
  
  if (!check_iter (priv, anchor))
    return;

  anchor_node = anchor->data;
  node = g_list_alloc ();
  node->data = NULL;

  anchor_node->prev->next = node;
  node->prev = anchor_node->prev;
  anchor_node->prev = node;
  node->next = anchor_node;

  iter->stamp = priv->stamp;
  iter->data  = node;
}

GObject*
oobs_list_get (OobsList      *list,
	       OobsListIter  *iter)
{
  OobsListPrivate *priv;
  GList *node;

  g_return_val_if_fail (list != NULL, NULL);
  g_return_val_if_fail (iter != NULL, NULL);
  g_return_val_if_fail (iter->data != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_LIST (list), NULL);

  node = iter->data;
  priv = OOBS_LIST_GET_PRIVATE (list);

  g_return_val_if_fail (node->data != NULL, NULL);

  if (!check_iter (priv, iter))
    return NULL;

  return g_object_ref (node->data);
}

static gboolean
check_types (OobsList *list,
	     GObject *data)
{
  GType object_type;

  if (!OOBS_LIST_GET_CLASS (list)->get_content_type)
    {
      g_critical ("List does not implement get_content_type ()");
      return FALSE;
    }

  object_type = OOBS_LIST_GET_CLASS (list)->get_content_type (list);

  if (!G_TYPE_CHECK_INSTANCE_TYPE (data, object_type))
    {
      g_warning ("Trying to store a different object type in the list");
      return FALSE;
    }

  return TRUE;
}

void
oobs_list_set (OobsList     *list,
	       OobsListIter *iter,
	       GObject     *data)
{
  OobsListPrivate *priv;
  GList *node;

  g_return_if_fail (list != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (iter->data != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  node = iter->data;
  priv = OOBS_LIST_GET_PRIVATE (list);

  g_return_if_fail (node->data == NULL);

  if (!check_iter (priv, iter))
    return;

  if (!check_types (list, data))
    return;

  node->data = g_object_ref (data);
}

void
unref_data (GObject *object, gpointer data)
{
  g_return_if_fail (object != NULL);
  g_object_unref (object);
}

void
oobs_list_clear (OobsList *list)
{
  OobsListPrivate *priv;
  
  g_return_if_fail (list != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);

  if (priv->list)
    {
      g_list_foreach (priv->list, (GFunc) unref_data, NULL);
      g_list_free    (priv->list);
      priv->list = NULL;
    }
}
