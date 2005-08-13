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

#include <glib-object.h>
#include "oobs-share.h"
#include "oobs-share-smb.h"

#define OOBS_SHARE_SMB_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SHARE_SMB, OobsShareSmbPrivate))

typedef struct _OobsShareSmbPrivate OobsShareSmbPrivate;

struct _OobsShareSmbPrivate {
  gchar *name;
  gchar *comment;

  OobsShareSmbFlags flags;
};

static void oobs_share_smb_class_init   (OobsShareSmbClass *class);
static void oobs_share_smb_init         (OobsShareSmb      *share);
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

G_DEFINE_TYPE (OobsShareSmb, oobs_share_smb, OOBS_TYPE_SHARE);

GType
oobs_share_smb_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GFlagsValue values[] =
	{
	  { OOBS_SHARE_SMB_ENABLED,   "OOBS_SHARE_SMB_ENABLED",   "smb-enabled" },
	  { OOBS_SHARE_SMB_BROWSABLE, "OOBS_SHARE_SMB_BROWSABLE", "smb-browsable" },
	  { OOBS_SHARE_SMB_PUBLIC,    "OOBS_SHARE_SMB_PUBLIC",    "smb-public" },
	  { OOBS_SHARE_SMB_WRITABLE,  "OOBS_SHARE_SMB_WRITABLE",  "smb-writable" },
	  { 0, NULL, NULL }
	};

      type = g_flags_register_static ("OobsShareSmbFlags", values);
    }

  return type;
}

static void
oobs_share_smb_class_init (OobsShareSmbClass *class)
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
			    sizeof (OobsShareSmbPrivate));
}

static void
oobs_share_smb_init (OobsShareSmb *share)
{
  OobsShareSmbPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));
  priv = OOBS_SHARE_SMB_GET_PRIVATE (share);

  priv->name    = NULL;
  priv->comment = NULL;
  priv->flags   = 0;
}

static void
oobs_share_smb_finalize (GObject *object)
{
  OobsShareSmb        *share;
  OobsShareSmbPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (object));

  share = OOBS_SHARE_SMB (object);
  priv  = OOBS_SHARE_SMB_GET_PRIVATE (share);

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
  OobsShareSmb        *share;
  OobsShareSmbPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (object));

  share = OOBS_SHARE_SMB (object);
  priv  = OOBS_SHARE_SMB_GET_PRIVATE (share);

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
  OobsShareSmb        *share;
  OobsShareSmbPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (object));

  share = OOBS_SHARE_SMB (object);
  priv  = OOBS_SHARE_SMB_GET_PRIVATE (share);

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

const gchar*
oobs_share_smb_get_name (OobsShareSmb *share)
{
  OobsShareSmbPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE_SMB (share), NULL);
  priv = OOBS_SHARE_SMB_GET_PRIVATE (share);

  return priv->name;
}

void
oobs_share_smb_set_name (OobsShareSmb *share, const gchar *name)
{
  OobsShareSmbPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));

  priv = OOBS_SHARE_SMB_GET_PRIVATE (share);
  priv->name = g_strdup (name);
  g_object_notify (G_OBJECT (share), "name");
}

const gchar*
oobs_share_smb_get_comment (OobsShareSmb *share)
{
  OobsShareSmbPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE_SMB (share), NULL);
  priv = OOBS_SHARE_SMB_GET_PRIVATE (share);

  return priv->comment;
}

void
oobs_share_smb_set_comment (OobsShareSmb *share, const gchar *comment)
{
  OobsShareSmbPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));

  priv = OOBS_SHARE_SMB_GET_PRIVATE (share);
  priv->comment = g_strdup (comment);
  g_object_notify (G_OBJECT (share), "comment");
}

OobsShareSmbFlags
oobs_share_smb_get_flags (OobsShareSmb *share)
{
  OobsShareSmbPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE_SMB (share), 0);
  priv = OOBS_SHARE_SMB_GET_PRIVATE (share);

  return priv->flags;
}

void
oobs_share_smb_set_flags (OobsShareSmb *share, OobsShareSmbFlags flags)
{
  OobsShareSmbPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));

  priv = OOBS_SHARE_SMB_GET_PRIVATE (share);
  priv->flags = flags;
  g_object_notify (G_OBJECT (share), "flags");
}

OobsShare*
oobs_share_smb_new (const gchar       *name,
		    const gchar       *comment,
		    const gchar       *path,
		    OobsShareSmbFlags  flags)
{
  return g_object_new (OOBS_TYPE_SHARE_SMB,
		       "name",    name,
		       "comment", comment,
		       "flags",   flags,
		       "path",    path,
		       NULL);
}
