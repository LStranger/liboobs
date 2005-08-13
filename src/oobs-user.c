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
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "oobs-usersconfig.h"
#include "oobs-user.h"
#include "oobs-defines.h"
#include "md5.h"

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#define OOBS_USER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_USER, OobsUserPrivate))

typedef struct _OobsUserPrivate OobsUserPrivate;

struct _OobsUserPrivate {
  OobsObject *config;

  gint   key;
  gchar *username;
  gchar *password;
  uid_t  uid;
  gid_t  gid;
  
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
  PROP_CRYPTED_PASSWORD,
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

G_DEFINE_TYPE (OobsUser, oobs_user, G_TYPE_OBJECT);

static void
oobs_user_class_init (OobsUserClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_user_set_property;
  object_class->get_property = oobs_user_get_property;
  object_class->finalize     = oobs_user_finalize;

  g_object_class_install_property (object_class,
				   PROP_USERNAME,
				   g_param_spec_string ("name",
							"Username",
							"Login name for the user",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_PASSWORD,
				   g_param_spec_string ("password",
							"Password",
							"Password for the user",
							NULL,
							G_PARAM_WRITABLE));
  g_object_class_install_property (object_class,
				   PROP_CRYPTED_PASSWORD,
				   g_param_spec_string ("crypted-password",
							"Crypted password",
							"Crypted password for the user",
							NULL,
							G_PARAM_WRITABLE));
  g_object_class_install_property (object_class,
				   PROP_UID,
				   g_param_spec_int ("uid",
						     "UID",
						     "UID for the user",
						     0, OOBS_MAX_UID, OOBS_MAX_UID,
						     G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_GID,
				   g_param_spec_int ("gid",
						     "GID",
						     "Main group GID for the user",
						     0, OOBS_MAX_GID, OOBS_MAX_GID,
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
  OobsSession *session;

  g_return_if_fail (OOBS_IS_USER (user));

  session = oobs_session_new ();

  priv = OOBS_USER_GET_PRIVATE (user);
  priv->config        = oobs_users_config_new (session);
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

static gchar *
get_random_string (gint len)
{
  gchar  alphanum[] = "abcdefghijklmnopqrstuvwxyz0AB1CD2EF3GH4IJ5KL6MN7OP8QR9ST0UVWXYZ";
  gchar *str;
  gint   i, alnum_len;

  str = (gchar *) g_malloc0 (len + 1);
  alnum_len = strlen (alphanum);

  for (i = 0; i < len; i++)
    str[i] = alphanum [(gint) (((float) alnum_len * rand ()) / (RAND_MAX + 1.0))];

  return str;
}

static void
oobs_user_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  OobsUser *user;
  OobsUserPrivate *priv;
  gboolean use_md5;
  gchar *salt;

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
      g_object_get (priv->config, "use-md5", &use_md5, NULL);

      if (use_md5)
	{
	  salt = get_random_string (8);
	  priv->password = crypt_md5 (g_value_get_string (value), salt);
	}
      else
	{
	  salt = get_random_string (2);
	  priv->password = crypt (g_value_get_string (value), salt);
	}

      break;
    case PROP_CRYPTED_PASSWORD:
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
oobs_user_new (const gchar *name)
{
  return g_object_new (OOBS_TYPE_USER,
		       "name", name,
		       NULL);
}

G_CONST_RETURN gchar*
oobs_user_get_login_name (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->username;
}

void
oobs_user_set_login_name (OobsUser *user, const gchar *login)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  /* FIXME: should check name length */

  g_object_set (G_OBJECT (user), "name", login, NULL);
}

void
oobs_user_set_password (OobsUser *user, const gchar *password)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "password", password, NULL);
}

void
oobs_user_set_crypted_password (OobsUser *user, const gchar *crypted_password)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "crypted-password", crypted_password, NULL);
}

uid_t
oobs_user_get_uid (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, OOBS_MAX_UID);
  g_return_val_if_fail (OOBS_IS_USER (user), OOBS_MAX_UID);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->uid;
}

void
oobs_user_set_uid (OobsUser *user, uid_t uid)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "uid", uid, NULL);
}

gid_t
oobs_user_get_gid (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, OOBS_MAX_GID);
  g_return_val_if_fail (OOBS_IS_USER (user), OOBS_MAX_GID);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->gid;
}

void
oobs_user_set_gid (OobsUser *user, gid_t gid)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "gid", gid, NULL);
}

G_CONST_RETURN gchar*
oobs_user_get_home_directory (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->homedir;
}

void
oobs_user_set_home_directory (OobsUser *user, const gchar *home_directory)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "home-directory", home_directory, NULL);
}

G_CONST_RETURN gchar*
oobs_user_get_shell (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->shell;
}

void
oobs_user_set_shell (OobsUser *user, const gchar *shell)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "shell", shell, NULL);
}

G_CONST_RETURN gchar*
oobs_user_get_full_name (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->full_name;
}

void
oobs_user_set_full_name (OobsUser *user, const gchar *full_name)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "full-name", full_name, NULL);
}

G_CONST_RETURN gchar*
oobs_user_get_room_number (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->room_no;
}

void
oobs_user_set_room_number (OobsUser *user, const gchar *room_number)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "room-number", room_number, NULL);
}

G_CONST_RETURN gchar*
oobs_user_get_work_phone_number (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->work_phone_no;
}

void
oobs_user_set_work_phone_number (OobsUser *user, const gchar *phone_number)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "work-phone", phone_number, NULL);
}

G_CONST_RETURN gchar*
oobs_user_get_home_phone_number (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->home_phone_no;
}

void
oobs_user_set_home_phone_number (OobsUser *user, const gchar *phone_number)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "home-phone", phone_number, NULL);
}

G_CONST_RETURN gchar*
oobs_user_get_other_data (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = OOBS_USER_GET_PRIVATE (user);

  return priv->other_data;
}

void
oobs_user_set_other_data (OobsUser *user, const gchar *data)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "other-data", data, NULL);
}
