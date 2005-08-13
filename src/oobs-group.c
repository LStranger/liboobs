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
#include "oobs-group.h"
#include "oobs-defines.h"

#define OOBS_GROUP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_GROUP, OobsGroupPrivate))

typedef struct _OobsGroupPrivate OobsGroupPrivate;

struct _OobsGroupPrivate {
  gint   key;
  gchar *groupname;
  gchar *password;
  gid_t  gid;
};

static void oobs_group_class_init (OobsGroupClass *class);
static void oobs_group_init       (OobsGroup      *group);
static void oobs_group_finalize   (GObject       *object);

static void oobs_group_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec);
static void oobs_group_get_property (GObject      *object,
				     guint         prop_id,
				     GValue       *value,
				     GParamSpec   *pspec);
enum {
  PROP_0,
  PROP_GROUPNAME,
  PROP_PASSWORD,
  PROP_CRYPTED_PASSWORD,
  PROP_GID,
};

G_DEFINE_TYPE (OobsGroup, oobs_group, G_TYPE_OBJECT);

static void
oobs_group_class_init (OobsGroupClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_group_set_property;
  object_class->get_property = oobs_group_get_property;
  object_class->finalize     = oobs_group_finalize;

  g_object_class_install_property (object_class,
				   PROP_GROUPNAME,
				   g_param_spec_string ("name",
							"Groupname",
							"Name for the group",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_PASSWORD,
				   g_param_spec_string ("password",
							"Password",
							"Password for the group",
							NULL,
							G_PARAM_WRITABLE));
  g_object_class_install_property (object_class,
				   PROP_PASSWORD,
				   g_param_spec_string ("crypted-password",
							"Crypted password",
							"Crypted password for the group",
							NULL,
							G_PARAM_WRITABLE));
  g_object_class_install_property (object_class,
				   PROP_GID,
				   g_param_spec_int ("gid",
						     "GID",
						     "Main group GID for the group",
						     0, OOBS_MAX_GID, OOBS_MAX_GID,
						     G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsGroupPrivate));
}

static void
oobs_group_init (OobsGroup *group)
{
  OobsGroupPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUP (group));

  priv = OOBS_GROUP_GET_PRIVATE (group);
  priv->groupname = NULL;
  priv->password  = NULL;
}

static void
oobs_group_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  OobsGroup *group;
  OobsGroupPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUP (object));

  group = OOBS_GROUP (object);
  priv = OOBS_GROUP_GET_PRIVATE (group);

  switch (prop_id)
    {
    case PROP_GROUPNAME:
      g_free (priv->groupname);
      priv->groupname = g_value_dup_string (value);
      break;
    case PROP_PASSWORD:
      g_free (priv->password);
      priv->password = g_value_dup_string (value);
      break;
    case PROP_CRYPTED_PASSWORD:
      g_free (priv->password);
      priv->password = g_value_dup_string (value);
      break;
    case PROP_GID:
      priv->gid = g_value_get_int (value);
      break;
    }
}

static void
oobs_group_get_property (GObject      *object,
			 guint         prop_id,
			 GValue       *value,
			 GParamSpec   *pspec)
{
  OobsGroup *group;
  OobsGroupPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUP (object));

  group = OOBS_GROUP (object);
  priv = OOBS_GROUP_GET_PRIVATE (group);

  switch (prop_id)
    {
    case PROP_GROUPNAME:
      g_value_set_string (value, priv->groupname);
      break;
    case PROP_GID:
      g_value_set_int (value, priv->gid);
      break;
    }
}

static void
oobs_group_finalize (GObject *object)
{
  OobsGroup        *group;
  OobsGroupPrivate *priv;

  g_return_if_fail (OOBS_IS_GROUP (object));

  group = OOBS_GROUP (object);
  priv = OOBS_GROUP_GET_PRIVATE (group);

  if (priv)
    {
      g_free (priv->groupname);
      g_free (priv->password);
    }

  if (G_OBJECT_CLASS (oobs_group_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_group_parent_class)->finalize) (object);
}

OobsGroup*
oobs_group_new (void)
{
  return g_object_new (OOBS_TYPE_GROUP,
		       NULL);
}

G_CONST_RETURN gchar*
oobs_group_get_name (OobsGroup *group)
{
  OobsGroupPrivate *priv;

  g_return_val_if_fail (group != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_GROUP (group), NULL);

  priv = OOBS_GROUP_GET_PRIVATE (group);

  return priv->groupname;
}

void
oobs_group_set_name (OobsGroup *group, const gchar *name)
{
  g_return_if_fail (group != NULL);
  g_return_if_fail (OOBS_IS_GROUP (group));
  g_return_if_fail (name != NULL);

  /* FIXME: should check name length */

  g_object_set (G_OBJECT (group), "name", name, NULL);
}

void
oobs_group_set_password (OobsGroup *group, const gchar *password)
{
  g_return_if_fail (group != NULL);
  g_return_if_fail (OOBS_IS_GROUP (group));

  g_object_set (G_OBJECT (group), "password", password, NULL);
}

gid_t
oobs_group_get_gid (OobsGroup *group)
{
  OobsGroupPrivate *priv;

  g_return_val_if_fail (group != NULL, OOBS_MAX_GID);
  g_return_val_if_fail (OOBS_IS_GROUP (group), OOBS_MAX_GID);

  priv = OOBS_GROUP_GET_PRIVATE (group);

  return priv->gid;
}

void
oobs_group_set_gid (OobsGroup *group, gid_t gid)
{
  g_return_if_fail (group != NULL);
  g_return_if_fail (OOBS_IS_GROUP (group));

  g_object_set (G_OBJECT (group), "gid", gid, NULL);
}
