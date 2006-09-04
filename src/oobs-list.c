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

  GType  contained_type;
  gboolean locked;
};

static void oobs_list_class_init (OobsListClass *class);
static void oobs_list_init       (OobsList      *list);
static void oobs_list_finalize   (GObject      *object);

static void oobs_list_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec);
enum
{
  PROP_0,
  PROP_CONTAINED_TYPE
};

G_DEFINE_TYPE (OobsList, oobs_list, G_TYPE_OBJECT);

GType
oobs_list_iter_get_type (void)
{
  static GType iter_type = 0;
  
  if (iter_type == 0)
    iter_type = g_boxed_type_register_static ("OobsListIter",
					      (GBoxedCopyFunc) oobs_list_iter_copy,
					      (GBoxedFreeFunc) oobs_list_iter_free);

  return iter_type;
}

static void
oobs_list_class_init (OobsListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = oobs_list_finalize;
  object_class->set_property = oobs_list_set_property;

  g_object_class_install_property (object_class,
				   PROP_CONTAINED_TYPE,
				   g_param_spec_pointer ("contained-type",
							 "Contained type",
							 "GType contained in the list",
							 G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_type_class_add_private (object_class,
			    sizeof (OobsListPrivate));
}

static void
oobs_list_init (OobsList *object)
{
  OobsListPrivate *priv;

  g_return_if_fail (OOBS_IS_LIST (object));
  priv = OOBS_LIST_GET_PRIVATE (object);

  priv->stamp  = 0;
  priv->list   = NULL;
  priv->locked = FALSE;
}

static void
oobs_list_finalize (GObject *object)
{
  OobsList *list;
  OobsListPrivate *priv;

  g_return_if_fail (OOBS_IS_LIST (object));

  list = OOBS_LIST (object);
  priv = OOBS_LIST_GET_PRIVATE (list);

  /* set locking to FALSE, the object
   * is already being finalized anyway */
  priv->locked = FALSE;

  if (priv && priv->list)
    oobs_list_clear (list);

  if (G_OBJECT_CLASS (oobs_list_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_list_parent_class)->finalize) (object);
}

static void
oobs_list_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  OobsList *list;
  OobsListPrivate *priv;
  GType *type;

  g_return_if_fail (OOBS_IS_LIST (object));

  list = OOBS_LIST (object);
  priv = OOBS_LIST_GET_PRIVATE (list);

  switch (prop_id)
    {
    case PROP_CONTAINED_TYPE:
      type = g_value_get_pointer (value);
      priv->contained_type = *type;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
check_iter (OobsListPrivate *priv, OobsListIter *iter)
{
  if (priv->stamp != iter->stamp)
    {
      g_critical ("OobsList stamp and OobsListIter stamp differ");
      return FALSE;
    }

  if (g_list_position (priv->list, iter->data) == -1)
    return FALSE;

  return TRUE;
}

GObject*
_oobs_list_new (const GType contained_type)
{
  return g_object_new (OOBS_TYPE_LIST,
		       "contained-type", &contained_type,
		       NULL);
}

void
_oobs_list_set_locked (OobsList *list, gboolean locked)
{
  OobsListPrivate *priv;

  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);
  priv->locked = locked;
}

/**
 * oobs_list_get_iter_first:
 * @list: An #OobsList
 * @iter: An uninitialized #OobsListIter
 * 
 * Initializes @iter the iterator pointing to the
 * first element in @list. if @list is empty, then FALSE
 * is returned, and @iter is not initialized.
 * 
 * Return Value: #TRUE if @iter was set
 **/
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

/**
 * oobs_list_iter_next:
 * @list: An #OobsList
 * @iter: A valid #OobsListIter pointing to an element in @list
 * 
 * Sets @iter to point to the element following it. If there's
 * no next @iter, of if @iter was invalid for @list, #FALSE is
 * returned, and @iter is set to invalid.
 * 
 * Return Value: #TRUE if iter has been correctly changed to the next element
 **/
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

/**
 * oobs_list_remove:
 * @list: An #OobsList
 * @iter: A valid #OobsListIter pointing to an element in @list
 * 
 * Removes an element pointed by @iter from @list. This function will not
 * be effective if @list is locked, or if @iter doesn't point to an element
 * contained in @list.
 * 
 * Return Value: #TRUE if the element was correctly removed
 **/
gboolean
oobs_list_remove (OobsList *list, OobsListIter *iter)
{
  OobsListPrivate *priv;
  GList *data, *next;
  gboolean list_locked;

  g_return_val_if_fail (list != NULL, FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (iter->data != NULL, FALSE);
  g_return_val_if_fail (OOBS_IS_LIST (list), FALSE);

  priv = OOBS_LIST_GET_PRIVATE (list);

  list_locked = priv->locked;
  g_return_val_if_fail (list_locked != TRUE, FALSE);

  if (!check_iter (priv, iter))
    return FALSE;

  data = (GList *) iter->data;

  /* point to the next element */
  next = data->next;

  g_object_unref (data->data);
  priv->list = g_list_delete_link (priv->list, data);

  iter->data = next;

  return (iter->data != NULL);
}

/**
 * oobs_list_append:
 * @list: An #OobsList
 * @iter: An unset #OobsListIter to set to the new element
 * 
 * Appends a new element to @list. @iter will be changed
 * to point to this element, to fill in values, you need to call
 * oobs_list_set().
 **/
void
oobs_list_append (OobsList *list, OobsListIter *iter)
{
  OobsListPrivate *priv;
  gboolean list_locked;

  g_return_if_fail (list != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);

  list_locked = priv->locked;
  g_return_if_fail (list_locked != TRUE);

  /* Change the stamp if the list was empty */
  if (!priv->list)
    priv->stamp++;

  priv->list = g_list_append (priv->list, NULL);

  iter->data = g_list_last (priv->list);
  iter->stamp = priv->stamp;
}

/**
 * oobs_list_prepend:
 * @list: An #OobsList
 * @iter: An unset #OobsListIter to set to the new element
 * 
 * Prepends a new element to @list. @iter will be changed
 * to point to this element, to fill in values, you need to call
 * oobs_list_set().
 **/
void
oobs_list_prepend (OobsList *list, OobsListIter *iter)
{
  OobsListPrivate *priv;
  gboolean list_locked;

  g_return_if_fail (list != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);

  list_locked = priv->locked;
  g_return_if_fail (list_locked != TRUE);

  /* Change the stamp if the list was empty */
  if (!priv->list)
    priv->stamp++;

  priv->list = g_list_prepend (priv->list, NULL);

  iter->data = priv->list;
  iter->stamp = priv->stamp;
}

/**
 * oobs_list_insert_after:
 * @list: An #OobsList
 * @anchor: A valid #OobsListIter
 * @iter: An unset #OobsListIter to set to the new element
 * 
 * Inserts a new element after @anchor. @iter will be changed
 * to point to this element, to fill in values, you need to call
 * oobs_list_set().
 **/
void
oobs_list_insert_after (OobsList     *list,
			OobsListIter *anchor,
			OobsListIter *iter)
{
  OobsListPrivate *priv;
  GList *node, *anchor_node;
  gboolean list_locked;

  g_return_if_fail (list != NULL);
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (anchor->data != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);
  
  list_locked = priv->locked;
  g_return_if_fail (list_locked != TRUE);

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

/**
 * oobs_list_insert_before:
 * @list: An #OobsList
 * @anchor: A valid #OobsListIter
 * @iter: An unset #OobsListIter to set to the new element
 * 
 * Inserts a new element before @anchor. @iter will be changed
 * to point to this element, to fill in values, you need to call
 * oobs_list_set().
 **/
void
oobs_list_insert_before (OobsList     *list,
			 OobsListIter *anchor,
			 OobsListIter *iter)
{
  OobsListPrivate *priv;
  GList *node, *anchor_node;
  gboolean list_locked;

  /* FIXME: make it match with the
     gtk_list_store behavior and api */

  g_return_if_fail (list != NULL);
  g_return_if_fail (anchor != NULL);
  g_return_if_fail (anchor->data != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);
  
  list_locked = priv->locked;
  g_return_if_fail (list_locked != TRUE);

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

/**
 * oobs_list_get:
 * @list: an #OobsList
 * @iter: a valid #OobsListIter for the element being get
 * 
 * Retrieves a reference to the element
 * referenced by #OobsListIter.
 * 
 * Return Value: the element referenced by @iter
 **/
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
  OobsListPrivate *priv;

  priv = OOBS_LIST_GET_PRIVATE (list);
  
  if (!G_TYPE_CHECK_INSTANCE_TYPE (data, priv->contained_type))
    {
      g_critical ("Trying to store a different object type in the list");
      return FALSE;
    }

  return TRUE;
}

/**
 * oobs_list_set:
 * @list: an #OobsList
 * @iter: a valid #OobsListIter for the element being set
 * @data: a pointer to the data being set
 * 
 * Sets the data for the element referenced by #OobsListIter.
 * This function will not be effective if the list is locked, or
 * if @data GType is different to the data contained in #OobsList.
 **/
void
oobs_list_set (OobsList     *list,
	       OobsListIter *iter,
	       gpointer      data)
{
  OobsListPrivate *priv;
  GList *node;
  gboolean list_locked;

  g_return_if_fail (list != NULL);
  g_return_if_fail (iter != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));
  g_return_if_fail (G_IS_OBJECT (data));

  node = iter->data;
  priv = OOBS_LIST_GET_PRIVATE (list);

  list_locked = priv->locked;
  g_return_if_fail (list_locked != TRUE);

  g_return_if_fail (node->data == NULL);

  if (!check_iter (priv, iter))
    return;

  if (!check_types (list, data))
    return;

  node->data = g_object_ref (data);
}

/**
 * oobs_list_clear:
 * @list: an #OobsList
 * 
 * Removes all contents from an #OobsList. This function will
 * not be effective if the list is locked.
 **/
void
oobs_list_clear (OobsList *list)
{
  OobsListPrivate *priv;
  gboolean list_locked;
  
  g_return_if_fail (list != NULL);
  g_return_if_fail (OOBS_IS_LIST (list));

  priv = OOBS_LIST_GET_PRIVATE (list);

  list_locked = priv->locked;
  g_return_if_fail (list_locked != TRUE);

  if (priv->list)
    {
      g_list_foreach (priv->list, (GFunc) g_object_unref, NULL);
      g_list_free    (priv->list);
      priv->list = NULL;
    }
}

/**
 * oobs_list_get_n_items:
 * @list: An #OobsList.
 * 
 * Returns the number of elements that the list contains.
 * 
 * Return Value: the number of elements.
 **/
gint
oobs_list_get_n_items (OobsList *list)
{
  OobsListPrivate *priv;

  g_return_val_if_fail (OOBS_IS_LIST (list), 0);

  priv = OOBS_LIST_GET_PRIVATE (list);

  return g_list_length (priv->list);
}

/**
 * oobs_list_iter_copy:
 * @iter: An #OobsListIter.
 * 
 * Returns a newly allocated copy of the given iterator. This function is not
 * intended for use in applications, because you can just copy the structs by
 * value (OobsListIter new_iter = iter;). You must free this iter with
 * oobs_list_iter_free().
 * 
 * Return Value: A newly allocated iterator.
 **/
OobsListIter*
oobs_list_iter_copy (OobsListIter *iter)
{
  OobsListIter *copy;

  copy = g_new0 (OobsListIter, 1);
  copy->stamp = iter->stamp;
  copy->data  = iter->data;

  return copy;
}

/**
 * oobs_list_iter_free:
 * @iter: An #OobsListIter.
 * 
 * Frees an iterator that has been allocated in the heap.
 **/
void
oobs_list_iter_free (OobsListIter *iter)
{
  g_free (iter);
}
