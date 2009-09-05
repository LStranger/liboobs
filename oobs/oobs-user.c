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
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>
#include <utmp.h>

#include "oobs-usersconfig.h"
#include "oobs-user.h"
#include "oobs-group.h"
#include "oobs-defines.h"
#include "utils.h"

/**
 * SECTION:oobs-user
 * @title: OobsUser
 * @short_description: Object that represents an individual user
 * @see_also: #OobsUsersConfig
 **/

#define OOBS_USER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_USER, OobsUserPrivate))

typedef struct _OobsUserPrivate OobsUserPrivate;

struct _OobsUserPrivate {
  OobsObject *config;

  OobsGroup *main_group;
  gchar *username;
  gchar *password;
  uid_t  uid;
  
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
  PROP_HOMEDIR,
  PROP_SHELL,
  PROP_FULL_NAME,
  PROP_ROOM_NO,
  PROP_WORK_PHONE_NO,
  PROP_HOME_PHONE_NO,
  PROP_OTHER_DATA,
  PROP_ACTIVE
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
							G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
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
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_UID,
				   g_param_spec_int ("uid",
						     "UID",
						     "UID for the user",
						     0, OOBS_MAX_UID, OOBS_MAX_UID,
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
  g_object_class_install_property (object_class,
				   PROP_OTHER_DATA,
				   g_param_spec_boolean ("active",
							 "Active",
							 "Whether the user is active",
							 FALSE,
							 G_PARAM_READABLE));
  g_type_class_add_private (object_class,
			    sizeof (OobsUserPrivate));
}

static void
oobs_user_init (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_if_fail (OOBS_IS_USER (user));

  priv = OOBS_USER_GET_PRIVATE (user);
  priv->config        = oobs_users_config_get ();
  priv->username      = NULL;
  priv->password      = NULL;
  priv->homedir       = NULL;
  priv->shell         = NULL;
  priv->full_name     = NULL;
  priv->room_no       = NULL;
  priv->work_phone_no = NULL;
  priv->home_phone_no = NULL;
  priv->other_data    = NULL;
  user->_priv         = priv;
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
  gchar *salt, *str;

  g_return_if_fail (OOBS_IS_USER (object));

  user = OOBS_USER (object);
  priv = user->_priv;

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
	  salt = utils_get_random_string (5);
	  str = g_strdup_printf ("$1$%s", salt);
	  priv->password = g_strdup ((gchar *) crypt (g_value_get_string (value), str));

	  g_free (str);
	}
      else
	{
	  salt = utils_get_random_string (2);
	  priv->password = g_strdup ((gchar *) crypt (g_value_get_string (value), salt));
	}

      g_free (salt);
      break;
    case PROP_CRYPTED_PASSWORD:
      g_free (priv->password);
      priv->password = g_value_dup_string (value);
      break;
    case PROP_UID:
      priv->uid = g_value_get_int (value);
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
  priv = user->_priv;

  switch (prop_id)
    {
    case PROP_USERNAME:
      g_value_set_string (value, priv->username);
      break;
    case PROP_CRYPTED_PASSWORD:
      g_value_set_string (value, priv->password);
      break;
    case PROP_UID:
      g_value_set_int (value, priv->uid);
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
    case PROP_ACTIVE:
      g_value_set_boolean (value, oobs_user_get_active (user));
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
  priv = user->_priv;

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

      if (priv->main_group)
	g_object_unref (priv->main_group);
    }

  if (G_OBJECT_CLASS (oobs_user_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_user_parent_class)->finalize) (object);
}

/**
 * oobs_user_new:
 * @name: login name for the new user.
 * 
 * Returns a new user with the given login name.
 * 
 * Return Value: A newly allocated #OobsUser.
 **/
OobsUser*
oobs_user_new (const gchar *name)
{
  g_return_val_if_fail (name && *name, NULL);

  return g_object_new (OOBS_TYPE_USER,
		       "name", name,
		       NULL);
}

/**
 * oobs_user_get_login_name:
 * @user: An #OobsUser.
 * 
 * Returns the login name of the user.
 * 
 * Return Value: A pointer to the login name as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_user_get_login_name (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;

  return priv->username;
}

/**
 * oobs_user_set_password:
 * @user: An #OobsUser.
 * @password: a new password for the user.
 * 
 * Sets a new password for the user. This password will be
 * interpreted as clean text and encrypted internally, be careful
 * deleting the passed string after using this function.
 **/
void
oobs_user_set_password (OobsUser *user, const gchar *password)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "password", password, NULL);
}

/**
 * oobs_user_set_crypted_password:
 * @user: An #OobsUser.
 * @crypted_password: a new crypted password.
 * 
 * Sets a new password for the user. This password will be
 * considered to be already crypted.
 **/
void
oobs_user_set_crypted_password (OobsUser *user, const gchar *crypted_password)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "crypted-password", crypted_password, NULL);
}

/**
 * oobs_user_get_uid:
 * @user: An #OobsUser.
 * 
 * Returns the UID for this user.
 * 
 * Return Value: user UID.
 **/
uid_t
oobs_user_get_uid (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, OOBS_MAX_UID);
  g_return_val_if_fail (OOBS_IS_USER (user), OOBS_MAX_UID);

  priv = user->_priv;

  return priv->uid;
}

/**
 * oobs_user_set_uid:
 * @user: An #OobsUser.
 * @uid: a new UID for the user.
 * 
 * Sets a new UID for the user. files formerly owned by the user
 * will not be chowned to the new UID, be careful using this function.
 **/
void
oobs_user_set_uid (OobsUser *user, uid_t uid)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "uid", uid, NULL);
}

/**
 * oobs_user_get_main_group:
 * @user: An #OobsUser.
 * 
 * Returns the main group of this user.
 * 
 * Return Value: main group for the user. this value is owned
 *               by the #OobsUser object, you do not have to
 *               unref it.
 **/
OobsGroup*
oobs_user_get_main_group (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;
  return priv->main_group;
}

/**
 * oobs_user_set_main_group:
 * @user: An #OobsUser.
 * @main_group: an #OobsGroup, new main group for the user.
 * 
 * Sets the main group for the user, adds a reference to
 * the new main group.
 **/
void
oobs_user_set_main_group (OobsUser  *user,
			  OobsGroup *main_group)
{
  OobsUserPrivate *priv;

  g_return_if_fail (OOBS_IS_USER (user));

  priv = user->_priv;

  if (priv->main_group)
    g_object_unref (priv->main_group);

  priv->main_group = (main_group) ? g_object_ref (main_group) : NULL;
}

/**
 * oobs_user_get_home_directory:
 * @user: An #OobsUser.
 * 
 * Returns the home directory path of the user.
 * 
 * Return Value: A pointer to the home directory as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_user_get_home_directory (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;

  return priv->homedir;
}

/**
 * oobs_user_set_home_directory:
 * @user: An #OobsUser.
 * @home_directory: new home directory path for the user.
 * 
 * Sets a new home directory for the user. files stored in the previous
 * home directory will not be moved, be careful using this function.
 **/
void
oobs_user_set_home_directory (OobsUser *user, const gchar *home_directory)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "home-directory", home_directory, NULL);
}

/**
 * oobs_user_get_shell:
 * @user: An #OobsUser.
 * 
 * Returns the default shell used by the user.
 * 
 * Return Value: A pointer to the default shell as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_user_get_shell (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;

  return priv->shell;
}

/**
 * oobs_user_set_shell:
 * @user: An #OobsUser.
 * @shell: a new default shell for the user.
 * 
 * Sets a new default shell for the user.
 **/
void
oobs_user_set_shell (OobsUser *user, const gchar *shell)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "shell", shell, NULL);
}

/**
 * oobs_user_get_full_name:
 * @user: An #OobsUser.
 * 
 * Returns the first GECOS field, usually the full name of the user.
 * 
 * Return Value: A pointer to the full name as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_user_get_full_name (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;

  return priv->full_name;
}

/**
 * oobs_user_set_full_name:
 * @user: An #OobsUser.
 * @full_name: new full name for the user.
 * 
 * Sets a new full name for the user.
 **/
void
oobs_user_set_full_name (OobsUser *user, const gchar *full_name)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "full-name", full_name, NULL);
}

/**
 * oobs_user_get_room_number:
 * @user: An #OobsUser.
 * 
 * Returns the second GECOS field, usually the room number.
 * 
 * Return Value: A pointer to the room number as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_user_get_room_number (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;

  return priv->room_no;
}

/**
 * oobs_user_set_room_number:
 * @user: An #OobsUser.
 * @room_number: new room number for the user.
 * 
 * Sets a new room number for the user.
 **/
void
oobs_user_set_room_number (OobsUser *user, const gchar *room_number)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "room-number", room_number, NULL);
}

/**
 * oobs_user_get_work_phone_number:
 * @user: An #OobsUser.
 * 
 * Returns the third GECOS field, usually the work phone number.
 * 
 * Return Value: A pointer to the work phone number as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_user_get_work_phone_number (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;

  return priv->work_phone_no;
}

/**
 * oobs_user_set_work_phone_number:
 * @user: An #OobsUser.
 * @phone_number: new work phone number for the user.
 * 
 * Sets a new work phone number for the user.
 **/
void
oobs_user_set_work_phone_number (OobsUser *user, const gchar *phone_number)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "work-phone", phone_number, NULL);
}

/**
 * oobs_user_get_home_phone_number:
 * @user: An #OobsUser.
 * 
 * Returns the fourth GECOS field, usually the home phone number.
 * 
 * Return Value: A pointer to the home phone number as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_user_get_home_phone_number (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;

  return priv->home_phone_no;
}

/**
 * oobs_user_set_home_phone_number:
 * @user: An #OobsUser.
 * @phone_number: new home phone number for the user.
 * 
 * Sets a new home phone number for the user.
 **/
void
oobs_user_set_home_phone_number (OobsUser *user, const gchar *phone_number)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "home-phone", phone_number, NULL);
}

/**
 * oobs_user_get_other_data:
 * @user: An #OobsUser.
 * 
 * Returns the fifth field of GECOS fields, reserved for additional data.
 * 
 * Return Value: A pointer to the fifth GECOS field as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_user_get_other_data (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;

  return priv->other_data;
}

/**
 * oobs_user_set_other_data:
 * @user: An #OobsUser.
 * @data: data in the fifth GECOS field.
 * 
 * Sets the data in the fifth GECOS field.
 **/
void
oobs_user_set_other_data (OobsUser *user, const gchar *data)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "other-data", data, NULL);
}

/**
 * oobs_user_get_active:
 * @user: An #OobsUser
 *
 * Returns whether the use is currently logged in the system.
 *
 * Return Value: #TRUE if the user is logged in the system.
 **/
gboolean
oobs_user_get_active (OobsUser *user)
{
  struct utmp *entry;
  const gchar *login;
  gboolean match = FALSE;

  g_return_val_if_fail (OOBS_IS_USER (user), FALSE);

  login = oobs_user_get_login_name (user);

  while (!match && (entry = getutent ()) != NULL)
    {
      match = (entry->ut_type == USER_PROCESS &&
	       strcmp (entry->ut_user, login) == 0);
    }

  /* close utmp */
  endutent ();

  return match;
}
