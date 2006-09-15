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

      type = g_flags_register_static ("OobsShareSMBFlags", values);
    }

  return type;
}

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

G_CONST_RETURN gchar*
oobs_share_smb_get_name (OobsShareSMB *share)
{
  OobsShareSMBPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE_SMB (share), NULL);
  priv = share->_priv;

  return priv->name;
}

void
oobs_share_smb_set_name (OobsShareSMB *share, const gchar *name)
{
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));

  priv = share->_priv;
  priv->name = g_strdup (name);
  g_object_notify (G_OBJECT (share), "name");
}

G_CONST_RETURN gchar*
oobs_share_smb_get_comment (OobsShareSMB *share)
{
  OobsShareSMBPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE_SMB (share), NULL);
  priv = share->_priv;

  return priv->comment;
}

void
oobs_share_smb_set_comment (OobsShareSMB *share, const gchar *comment)
{
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));

  priv = share->_priv;
  priv->comment = g_strdup (comment);
  g_object_notify (G_OBJECT (share), "comment");
}

OobsShareSMBFlags
oobs_share_smb_get_flags (OobsShareSMB *share)
{
  OobsShareSMBPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SHARE_SMB (share), 0);
  priv = share->_priv;

  return priv->flags;
}

void
oobs_share_smb_set_flags (OobsShareSMB *share, OobsShareSMBFlags flags)
{
  OobsShareSMBPrivate *priv;

  g_return_if_fail (OOBS_IS_SHARE_SMB (share));

  priv = share->_priv;
  priv->flags = flags;
  g_object_notify (G_OBJECT (share), "flags");
}

OobsShare*
oobs_share_smb_new (const gchar       *path,
		    const gchar       *name,
		    const gchar       *comment,
		    OobsShareSMBFlags  flags)
{
  g_return_val_if_fail (path[0] == '/', NULL);

  return g_object_new (OOBS_TYPE_SHARE_SMB,
		       "name",    name,
		       "comment", comment,
		       "flags",   flags,
		       "path",    path,
		       NULL);
}
