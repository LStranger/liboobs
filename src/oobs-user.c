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
#include "oobs-user.h"

#define OOBS_USER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_USER, OobsUserPrivate))
#define DEFAULT_UID 65534
#define DEFAULT_GID 65534

typedef struct _OobsUserPrivate OobsUserPrivate;

struct _OobsUserPrivate {
  gint   key;
  gchar *username;
  gchar *password;
  gint   uid;
  gint   gid;
  
  gchar *homedir;
  gchar *shell;

  /* GECOS Fields */
  gchar *full_name;
  gchar *room_no;
  gchar *work_phone_no;
  gchar *home_phone_no;
  gchar *other_data;
};

static void oobs_user_class_init (OobsUserClass *class);
static void oobs_user_init       (OobsUser      *user);
static void oobs_user_finalize   (GObject       *object);

static void oobs_user_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec);
static void oobs_user_get_property (GObject      *object,
				    guint         prop_id,
				    GValue       *value,
				    GParamSpec   *pspec);
enum
{
  PROP_0,
  PROP_USERNAME,
  PROP_PASSWORD,
  PROP_UID,
  PROP_GID,
  PROP_HOMEDIR,
  PROP_SHELL,
  PROP_FULL_NAME,
  PROP_ROOM_NO,
  PROP_WORK_PHONE_NO,
  PROP_HOME_PHONE_NO,
  PROP_OTHER_DATA
};

G_DEFINE_TYPE (OobsUser, oobs_user, OOBS_TYPE_USER);

static void
oobs_user_class_init (OobsUserClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  oobs_user_parent_class = g_type_class_peek_parent (class);

  object_class->set_property = oobs_user_set_property;
  object_class->get_property = oobs_user_get_property;
  object_class->finalize     = oobs_user_finalize;

  g_object_class_install_property (object_class,
				   PROP_USERNAME,
				   g_param_spec_string ("username",
							"Username",
							"Login name for the user",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_PASSWORD,
				   g_param_spec_string ("password",
							"Passwod",
							"Password for the user",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_UID,
				   g_param_spec_int ("uid",
						     "UID",
						     "UID for the user",
						     0, DEFAULT_UID, DEFAULT_UID,
						     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_GID,
				   g_param_spec_int ("gid",
						     "GID",
						     "Main group GID for the user",
						     0, DEFAULT_GID, DEFAULT_GID,
						     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_HOMEDIR,
				   g_param_spec_string ("home-directory",
							"Home directory",
							"Home directory for the user",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_SHELL,
				   g_param_spec_string ("shell",
							"Default shell",
							"Default shell for the user",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_FULL_NAME,
				   g_param_spec_string ("full-name",
							"Full name",
							"User's full name",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_ROOM_NO,
				   g_param_spec_string ("room-number",
							"Room number",
							"User's room number",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_WORK_PHONE_NO,
				   g_param_spec_string ("work-phone",
							"Work phone",
							"User's work phone",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_HOME_PHONE_NO,
				   g_param_spec_string ("home-phone",
							"Home phone",
							"User's home phone",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_OTHER_DATA,
				   g_param_spec_string ("other-data",
							"Other data",
							"Aditional data for the user",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsUserPrivate));
}

static void
oobs_user_init (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_if_fail (OOBS_IS_USER (user));

  priv = OOBS_USER_GET_PRIVATE (user);
  priv->username      = NULL;
  priv->password      = NULL;
  priv->homedir       = NULL;
  priv->shell         = NULL;
  priv->full_name     = NULL;
  priv->room_no       = NULL;
  priv->work_phone_no = NULL;
  priv->home_phone_no = NULL;
  priv->other_data    = NULL;
}

static void
oobs_user_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  OobsUser *user;
  OobsUserPrivate *priv;

  g_return_if_fail (OOBS_IS_USER (object));

  user = OOBS_USER (object);
  priv = OOBS_USER_GET_PRIVATE (user);

  switch (prop_id)
    {
    case PROP_USERNAME:
      g_free (priv->username);
      priv->username = g_value_dup_string (value);
      break;
    case PROP_PASSWORD:
      g_free (priv->password);
      priv->password = g_value_dup_string (value);
      break;
    case PROP_UID:
      priv->uid = g_value_get_int (value);
      break;
    case PROP_GID:
      priv->gid = g_value_get_int (value);
      break;
    case PROP_HOMEDIR:
      g_free (priv->homedir);
      priv->homedir = g_value_dup_string (value);
      break;
    case PROP_SHELL:
      g_free (priv->shell);
      priv->shell = g_value_dup_string (value);
      break;
    case PROP_FULL_NAME:
      g_free (priv->full_name);
      priv->full_name = g_value_dup_string (value);
      break;
    case PROP_ROOM_NO:
      g_free (priv->room_no);
      priv->room_no = g_value_dup_string (value);
      break;
    case PROP_WORK_PHONE_NO:
      g_free (priv->work_phone_no);
      priv->work_phone_no = g_value_dup_string (value);
      break;
    case PROP_HOME_PHONE_NO:
      g_free (priv->home_phone_no);
      priv->home_phone_no = g_value_dup_string (value);
      break;
    case PROP_OTHER_DATA:
      g_free (priv->other_data);
      priv->other_data = g_value_dup_string (value);
      break;
    }
}

static void
oobs_user_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  OobsUser *user;
  OobsUserPrivate *priv;

  g_return_if_fail (OOBS_IS_USER (object));

  user = OOBS_USER (object);
  priv = OOBS_USER_GET_PRIVATE (user);

  switch (prop_id)
    {
    case PROP_USERNAME:
      g_value_set_string (value, priv->username);
      break;
    case PROP_PASSWORD:
      g_value_set_string (value, priv->password);
      break;
    case PROP_UID:
      g_value_set_int (value, priv->uid);
      break;
    case PROP_GID:
      g_value_set_int (value, priv->gid);
      break;
    case PROP_HOMEDIR:
      g_value_set_string (value, priv->homedir);
      break;
    case PROP_SHELL:
      g_value_set_string (value, priv->shell);
      break;
    case PROP_FULL_NAME:
      g_value_set_string (value, priv->full_name);
      break;
    case PROP_ROOM_NO:
      g_value_set_string (value, priv->room_no);
      break;
    case PROP_WORK_PHONE_NO:
      g_value_set_string (value, priv->work_phone_no);
      break;
    case PROP_HOME_PHONE_NO:
      g_value_set_string (value, priv->home_phone_no);
      break;
    case PROP_OTHER_DATA:
      g_value_set_string (value, priv->other_data);
      break;
    }
}

static void
oobs_user_finalize (GObject *object)
{
  OobsUser        *user;
  OobsUserPrivate *priv;

  g_return_if_fail (OOBS_IS_USER (object));

  user = OOBS_USER (object);
  priv = OOBS_USER_GET_PRIVATE (user);

  if (priv)
    {
      g_free (priv->username);
      g_free (priv->password);
      g_free (priv->homedir);
      g_free (priv->shell);
      g_free (priv->full_name);
      g_free (priv->room_no);
      g_free (priv->work_phone_no);
      g_free (priv->home_phone_no);
      g_free (priv->other_data);
    }

  if (G_OBJECT_CLASS (oobs_user_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_user_parent_class)->finalize) (object);
}

OobsUser*
oobs_user_new (void)
{
  return g_object_new (OOBS_TYPE_USER,
		       NULL);
}
