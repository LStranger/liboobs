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

#include <glib-object.h>
#include "oobs-share.h"

#define OOBS_SHARE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SHARE, OobsSharePrivate))

typedef struct _OobsSharePrivate OobsSharePrivate;

struct _OobsSharePrivate {
  gchar *path;
};

static void oobs_share_class_init   (OobsShareClass *class);
static void oobs_share_init         (OobsShare *share);
static void oobs_share_finalize     (GObject  *object);

static void oobs_share_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec);
static void oobs_share_get_property (GObject      *object,
				     guint         prop_id,
				     GValue       *value,
				     GParamSpec   *pspec);
enum
{
  PROP_0,
  PROP_PATH
};

G_DEFINE_TYPE (OobsShare, oobs_share, G_TYPE_OBJECT);

static void
oobs_share_class_init (OobsShareClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_share_set_property;
  object_class->get_property = oobs_share_get_property;
  object_class->finalize     = oobs_share_finalize;

  g_object_class_install_property (object_class,
				   PROP_PATH,
				   g_param_spec_string ("path",
							"Share path",
							"Path for the share",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsSharePrivate));
}

static void
oobs_share_init (OobsShare *share)
{
  OobsSharePrivate *priv;
  
  g_return_if_fail (OOBS_IS_SHARE (share));
  priv = OOBS_SHARE_GET_PRIVATE (share);
  
  priv->path = NULL;
}

static void
oobs_share_finalize (GObject *object)
{
  OobsShare        *share;
  OobsSharePrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE (object));
  share = OOBS_SHARE (object);
  priv  = OOBS_SHARE_GET_PRIVATE (share);

  if (priv)
    g_free (priv->path);

  if (G_OBJECT_CLASS (oobs_share_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_share_parent_class)->finalize) (object);
}

static void
oobs_share_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  OobsShare        *share;
  OobsSharePrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE (object));
  share = OOBS_SHARE (object);
  priv  = OOBS_SHARE_GET_PRIVATE (share);

  switch (prop_id)
    {
    case PROP_PATH:
      g_free (priv->path);
      priv->path = (gchar*) g_value_dup_string (value);
      break;
    }
}

static void
oobs_share_get_property (GObject      *object,
			 guint         prop_id,
			 GValue       *value,
			 GParamSpec   *pspec)
{
  OobsShare        *share;
  OobsSharePrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE (object));
  share = OOBS_SHARE (object);
  priv  = OOBS_SHARE_GET_PRIVATE (share);

  switch (prop_id)
    {
    case PROP_PATH:
      g_value_set_string (value, priv->path);
      break;
    }
}

/**
 * oobs_share_get_path:
 * @share: An #OobsShare.
 * 
 * Returns the path that #share shares.
 * 
 * Return Value: A pointer to the shared path as a string.
 *               This string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_share_get_path (OobsShare *share)
{
  OobsSharePrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE (share), NULL);
  priv = OOBS_SHARE_GET_PRIVATE (share);

  return priv->path;
}

/**
 * oobs_share_set_path:
 * @share: An #OobsShare.
 * @path: A new shared path for #share.
 *
 * Sets the shared path of #share to be #path,
 * overwriting the previous one.
 **/
void
oobs_share_set_path (OobsShare *share, const gchar *path)
{
  OobsSharePrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE (share));

  priv = OOBS_SHARE_GET_PRIVATE (share);
  priv->path = g_strdup (path);
}
