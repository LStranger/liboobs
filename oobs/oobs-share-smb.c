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
#include "oobs-share-smb.h"

/**
 * SECTION:oobs-share-smb
 * @title: OobsShareSMB
 * @short_description: Object that represents SMB configuration.
 * @see_also: #OobsShare, #OobsSMBConfig
 **/

#define OOBS_SHARE_SMB_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SHARE_SMB, OobsShareSMBPrivate))

typedef struct _OobsShareSMBPrivate OobsShareSMBPrivate;

struct _OobsShareSMBPrivate {
  gchar *name;
  gchar *comment;

  OobsShareSMBFlags flags;
};

static void oobs_share_smb_class_init   (OobsShareSMBClass *class);
static void oobs_share_smb_init         (OobsShareSMB      *share);
static void oobs_share_smb_finalize     (GObject          *object);

static void oobs_share_smb_set_property (GObject      *object,
					 guint         prop_id,
					 const GValue *value,
					 GParamSpec   *pspec);
static void oobs_share_smb_get_property (GObject      *object,
					 guint         prop_id,
					 GValue       *value,
					 GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_NAME,
  PROP_COMMENT,
  PROP_FLAGS
};

G_DEFINE_TYPE (OobsShareSMB, oobs_share_smb, OOBS_TYPE_SHARE);

static void
oobs_share_smb_class_init (OobsShareSMBClass *class)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_share_smb_set_property;
  object_class->get_property = oobs_share_smb_get_property;
  object_class->finalize     = oobs_share_smb_finalize;

  g_object_class_install_property (object_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
							NULL,
							NULL,
							NULL,
							G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_COMMENT,
				   g_param_spec_string ("comment",
							"Share comment",
							"Comment for the share",
							NULL,
							G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_FLAGS,
				   g_param_spec_flags ("flags",
						       "Flags",
						       "Property flags for the share",
						       OOBS_TYPE_SHARE_SMB_FLAGS,
						       0,
						       G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsShareSMBPrivate));
}

static void
oobs_share_smb_init (OobsShareSMB *share)
{
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));
  priv = OOBS_SHARE_SMB_GET_PRIVATE (share);

  priv->name    = NULL;
  priv->comment = NULL;
  priv->flags   = 0;
  share->_priv  = priv;
}

static void
oobs_share_smb_finalize (GObject *object)
{
  OobsShareSMB        *share;
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (object));

  share = OOBS_SHARE_SMB (object);
  priv  = share->_priv;

  if (priv)
    {
      g_free (priv->name);
      g_free (priv->comment);
    }

  if (G_OBJECT_CLASS (oobs_share_smb_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_share_smb_parent_class)->finalize) (object);
}

static void
oobs_share_smb_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  OobsShareSMB        *share;
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (object));

  share = OOBS_SHARE_SMB (object);
  priv  = share->_priv;

  switch (prop_id)
    {
    case PROP_NAME:
      priv->name    = (gchar*) g_value_dup_string (value);
      break;
    case PROP_COMMENT:
      priv->comment = (gchar*) g_value_dup_string (value);
      break;
    case PROP_FLAGS:
      priv->flags   = g_value_get_flags (value);
      break;
    }
}

static void
oobs_share_smb_get_property (GObject      *object,
			     guint         prop_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  OobsShareSMB        *share;
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (object));

  share = OOBS_SHARE_SMB (object);
  priv  = share->_priv;

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_COMMENT:
      g_value_set_string (value, priv->comment);
      break;
    case PROP_FLAGS:
      g_value_set_flags  (value, priv->flags);
      break;
    }
}

/**
 * oobs_share_smb_get_name:
 * @share: An #OobsShareSMB.
 * 
 * Returns the share name.
 * 
 * Return Value: A pointer to the share name as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_share_smb_get_name (OobsShareSMB *share)
{
  OobsShareSMBPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE_SMB (share), NULL);
  priv = share->_priv;

  return priv->name;
}

/**
 * oobs_share_smb_set_name:
 * @share: An #OobsShareSMB.
 * @name: new name for the share.
 * 
 * Sets a new name for the share, overwriting the previous one.
 **/
void
oobs_share_smb_set_name (OobsShareSMB *share, const gchar *name)
{
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));

  priv = share->_priv;
  priv->name = g_strdup (name);
  g_object_notify (G_OBJECT (share), "name");
}

/**
 * oobs_share_smb_get_comment:
 * @share: An #OobsShareSMB.
 * 
 * Returns the comment for the share.
 * 
 * Return Value: A pointer to the share comment as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_share_smb_get_comment (OobsShareSMB *share)
{
  OobsShareSMBPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE_SMB (share), NULL);
  priv = share->_priv;

  return priv->comment;
}

/**
 * oobs_share_smb_set_comment:
 * @share: An #OobsShareSMB.
 * @comment: new comment for the share.
 * 
 * Sets a new comment for the share, overwriting the previous one.
 **/
void
oobs_share_smb_set_comment (OobsShareSMB *share, const gchar *comment)
{
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));

  priv = share->_priv;
  priv->comment = g_strdup (comment);
  g_object_notify (G_OBJECT (share), "comment");
}

/**
 * oobs_share_smb_get_flags:
 * @share: An #OobsShareSMB.
 * 
 * Returns the options mask of the share.
 * 
 * Return Value: #OobsShareSMBFlags mask representing the share options.
 **/
OobsShareSMBFlags
oobs_share_smb_get_flags (OobsShareSMB *share)
{
  OobsShareSMBPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE_SMB (share), 0);
  priv = share->_priv;

  return priv->flags;
}

/**
 * oobs_share_smb_set_flags:
 * @share: An #OobsShareSMB.
 * @flags: mask of options for the share.
 * 
 * Sets a new set of options for the share.
 **/
void
oobs_share_smb_set_flags (OobsShareSMB *share, OobsShareSMBFlags flags)
{
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));

  priv = share->_priv;
  priv->flags = flags;
  g_object_notify (G_OBJECT (share), "flags");
}

/**
 * oobs_share_smb_new:
 * @path: folder path for the new share.
 * @name: name for the new share.
 * @comment: comment for the new share.
 * @flags: options mask for the new share.
 * 
 * Creates a new #OobsShareSMB with the given settings.
 * 
 * Return Value: a newly allocated #OobsShareSMB.
 **/
OobsShare*
oobs_share_smb_new (const gchar       *path,
		    const gchar       *name,
		    const gchar       *comment,
		    OobsShareSMBFlags  flags)
{
  g_return_val_if_fail (path != NULL && path[0] == '/', NULL);

  return g_object_new (OOBS_TYPE_SHARE_SMB,
		       "name",    name,
		       "comment", comment,
		       "flags",   flags,
		       "path",    path,
		       NULL);
}
