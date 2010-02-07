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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>,
 *          Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#include <glib-object.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>
#include <utmpx.h>

#include "oobs-object-private.h"
#include "oobs-usersconfig.h"
#include "oobs-user.h"
#include "oobs-user-private.h"
#include "oobs-group.h"
#include "oobs-defines.h"
#include "utils.h"

/**
 * SECTION:oobs-user
 * @title: OobsUser
 * @short_description: Object that represents an individual user
 * @see_also: #OobsUsersConfig
 **/

#define USER_REMOTE_OBJECT "UserConfig2"
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

  /* filled from the password flags */
  gboolean passwd_empty;
  gboolean passwd_disabled;

  gboolean           encrypted_home;
  gchar             *locale;
  OobsUserHomeFlags  home_flags;
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

static void oobs_user_commit             (OobsObject *object);
static void oobs_user_update             (OobsObject *object);
static void oobs_user_get_update_message (OobsObject *object);

enum
{
  PROP_0,
  PROP_USERNAME,
  PROP_PASSWORD,
  PROP_UID,
  PROP_HOMEDIR,
  PROP_SHELL,
  PROP_FULL_NAME,
  PROP_ROOM_NO,
  PROP_WORK_PHONE_NO,
  PROP_HOME_PHONE_NO,
  PROP_OTHER_DATA,
  PROP_PASSWD_EMPTY,
  PROP_PASSWD_DISABLED,
  PROP_ENCRYPTED_HOME,
  PROP_LOCALE,
  PROP_HOME_FLAGS,
  PROP_ACTIVE
};

G_DEFINE_TYPE (OobsUser, oobs_user, OOBS_TYPE_OBJECT);

static void
oobs_user_class_init (OobsUserClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_class = OOBS_OBJECT_CLASS (class);

  object_class->set_property = oobs_user_set_property;
  object_class->get_property = oobs_user_get_property;
  object_class->finalize     = oobs_user_finalize;

  oobs_class->commit = oobs_user_commit;
  oobs_class->update = oobs_user_update;
  oobs_class->get_update_message = oobs_user_get_update_message;

  /* override the singleton check */
  oobs_class->singleton = FALSE;

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
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_UID,
				   g_param_spec_uint ("uid",
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
				   PROP_PASSWD_EMPTY,
				   g_param_spec_boolean ("password-empty",
							 "Empty password",
							 "Whether user password is empty",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_PASSWD_DISABLED,
				   g_param_spec_boolean ("password-disabled",
							 "Disabled account",
							 "Whether user is allowed to log in",
							 TRUE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_ENCRYPTED_HOME,
                                   g_param_spec_boolean ("encrypted-home",
                                                         "Encrypted home",
                                                         "Whether user's home is encrypted",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_HOME_FLAGS,
				   g_param_spec_flags ("home-flags",
				                       "Home flags",
				                       "Flags affecting home directory treatment",
				                       OOBS_TYPE_USER_HOME_FLAGS,
				                       0,
				                       G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_LOCALE,
                                   g_param_spec_string ("locale",
                                                        "Locale",
                                                        "Preferred locale for the user",
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
  priv->locale        = NULL;

  /* This value ensures the backends will use the system default
   * if UID were not changed manually. */
  priv->uid = G_MAXUINT32;

  priv->passwd_empty    = FALSE;
  priv->passwd_disabled = FALSE;
  priv->encrypted_home  = FALSE;
  priv->home_flags      = 0;

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
      priv->password = g_value_dup_string (value);
      break;
    case PROP_UID:
      priv->uid = g_value_get_uint (value);
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
    case PROP_PASSWD_EMPTY:
      priv->passwd_empty = g_value_get_boolean (value);
      break;
    case PROP_PASSWD_DISABLED:
      priv->passwd_disabled = g_value_get_boolean (value);
      break;
    case PROP_ENCRYPTED_HOME:
      priv->encrypted_home = g_value_get_boolean (value);
      break;
    case PROP_HOME_FLAGS:
      priv->home_flags = g_value_get_flags (value);
      break;
    case PROP_LOCALE:
      g_free (priv->locale);
      priv->locale = g_value_dup_string (value);
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
    case PROP_PASSWORD:
      g_value_set_string (value, priv->password);
      break;
    case PROP_UID:
      g_value_set_uint (value, priv->uid);
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
    case PROP_PASSWD_EMPTY:
      g_value_set_boolean (value, priv->passwd_empty);
      break;
    case PROP_PASSWD_DISABLED:
      g_value_set_boolean (value, priv->passwd_disabled);
      break;
    case PROP_ENCRYPTED_HOME:
	g_value_set_boolean (value, priv->encrypted_home);
      break;
    case PROP_HOME_FLAGS:
      g_value_set_flags (value, priv->home_flags);
      break;
    case PROP_LOCALE:
      g_value_set_string (value, priv->locale);
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
      g_free (priv->homedir);
      g_free (priv->shell);
      g_free (priv->full_name);
      g_free (priv->room_no);
      g_free (priv->work_phone_no);
      g_free (priv->home_phone_no);
      g_free (priv->other_data);
      g_free (priv->locale);

      if (priv->main_group)
	g_object_unref (priv->main_group);

      /* Erase password field in case it's not done */
      memset (priv->password, 0, strlen (priv->password));
      g_free (priv->password);
    }

  if (G_OBJECT_CLASS (oobs_user_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_user_parent_class)->finalize) (object);
}

OobsUser*
_oobs_user_create_from_dbus_reply (OobsUser        *user,
                                   gid_t           *gid_ptr,
                                   DBusMessage     *reply,
                                   DBusMessageIter  struct_iter)
{
  DBusMessageIter iter, gecos_iter;
  guint32 uid, gid;
  const gchar *login, *passwd, *home, *shell;
  const gchar *name, *room_number, *work_phone, *home_phone, *other_data;
  const gchar *locale;
  gint passwd_flags, home_flags;
  gboolean enc_home, passwd_empty, passwd_disabled;

  dbus_message_iter_recurse (&struct_iter, &iter);

  login = utils_get_string (&iter);
  passwd = utils_get_string (&iter);
  uid = utils_get_uint (&iter);

  gid = utils_get_uint (&iter);
  if (gid_ptr)
    *gid_ptr = gid;

  /* GECOS fields */
  dbus_message_iter_recurse (&iter, &gecos_iter);

  name = utils_get_string (&gecos_iter);
  room_number = utils_get_string (&gecos_iter);
  work_phone = utils_get_string (&gecos_iter);
  home_phone = utils_get_string (&gecos_iter);
  other_data = utils_get_string (&gecos_iter);
  /* end of GECOS fields */

  dbus_message_iter_next (&iter);

  home = utils_get_string (&iter);
  shell = utils_get_string (&iter);

  passwd_flags = utils_get_int (&iter);
  passwd_empty = passwd_flags & 1;
  passwd_disabled = passwd_flags & (1 << 1);

  enc_home = utils_get_boolean (&iter);
  home_flags = utils_get_int (&iter);
  locale = utils_get_string (&iter);

  if (!user)
    user = oobs_user_new (login);

  g_object_set (user,
                "uid", uid,
                "home-directory", home,
                "shell", shell,
                "full-name", name,
                "room-number", room_number,
                "work-phone", work_phone,
                "home-phone", home_phone,
                "other-data", other_data,
                "encrypted-home", enc_home,
                "home-flags", home_flags,
                "password-empty", passwd_empty,
                "password-disabled", passwd_disabled,
                "locale", locale,
                NULL);

  return user;
}

static gboolean
create_dbus_struct_from_user (OobsUser        *user,
			      DBusMessage     *message,
			      DBusMessageIter *iter)
{
  OobsGroup *group;
  guint32 uid, gid;
  gchar *login, *password, *shell, *homedir;
  gchar *name, *room_number, *work_phone, *home_phone, *other_data;
  gchar *locale;
  gboolean enc_home;
  gboolean passwd_empty, passwd_disabled;
  gint passwd_flags, home_flags;
  DBusMessageIter data_iter;

  g_object_get (user,
		"name", &login,
		"password", &password,
		"uid", &uid,
		"home-directory", &homedir,
		"shell", &shell,
		"full-name", &name,
		"room-number", &room_number,
		"work-phone", &work_phone,
		"home-phone", &home_phone,
		"other-data", &other_data,
                "encrypted-home", &enc_home,
                "home-flags", &home_flags,
                "locale", &locale,
                "password-empty", &passwd_empty,
                "password-disabled", &passwd_disabled,
		NULL);

  /* Login is the only required field,
   * since home dir, password and shell are allowed to be empty (see man 5 passwd) */
  g_return_val_if_fail (login, FALSE);

  group = oobs_user_get_main_group (user);

  /* G_MAXUINT32 is used to mean no main group */
  if (group)
    gid = oobs_group_get_gid (group);
  else
    gid = G_MAXUINT32;

  passwd_flags = passwd_empty | (passwd_disabled << 1);

  utils_append_string (iter, login);
  utils_append_string (iter, password);
  utils_append_uint (iter, uid);
  utils_append_uint (iter, gid);

  dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &data_iter);

  /* GECOS fields */
  utils_append_string (&data_iter, name);
  utils_append_string (&data_iter, room_number);
  utils_append_string (&data_iter, work_phone);
  utils_append_string (&data_iter, home_phone);
  utils_append_string (&data_iter, other_data);

  dbus_message_iter_close_container (iter, &data_iter);

  utils_append_string (iter, homedir);
  utils_append_string (iter, shell);
  utils_append_int (iter, passwd_flags);
  utils_append_boolean (iter, enc_home);
  utils_append_boolean (iter, home_flags);
  utils_append_string (iter, locale);
  /* TODO: use location when the backends support it */
  utils_append_string (iter, "");

  g_free (login);
  g_free (password);
  g_free (shell);
  g_free (homedir);
  g_free (name);
  g_free (room_number);
  g_free (work_phone);
  g_free (home_phone);
  g_free (other_data);
  g_free (locale);

  return TRUE;
}

static void
oobs_user_commit (OobsObject *object)
{
  OobsUserPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter, struct_iter;

  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);
  dbus_message_iter_open_container (&iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);
  create_dbus_struct_from_user (OOBS_USER (object), message, &struct_iter);
  dbus_message_iter_close_container (&iter, &struct_iter);

  /* Erase password field as soon as possible */
  priv = OOBS_USER_GET_PRIVATE (OOBS_USER (object));
  memset (priv->password, 0, strlen (priv->password));
  g_free (priv->password);
}

/*
 * We need a custom update message containing the user's UID.
 */
static void
oobs_user_get_update_message (OobsObject *object)
{
  OobsUserPrivate *priv;
  DBusMessageIter iter;
  DBusMessage *message;

  priv = OOBS_USER (object)->_priv;

  message = _oobs_object_get_dbus_message (object);
  dbus_message_iter_init_append (message, &iter);

  utils_append_string (&iter, priv->username);
}

static void
oobs_user_update (OobsObject *object)
{
  DBusMessage *reply;
  DBusMessageIter iter;

  reply = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init (reply, &iter);
  _oobs_user_create_from_dbus_reply (OOBS_USER (object), NULL, reply, iter);
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
		       "remote-object", USER_REMOTE_OBJECT,
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
 * interpreted as clean text and encrypted by the backends using PAM.
 * Be careful deleting the passed string after using this function.
 **/
void
oobs_user_set_password (OobsUser *user, const gchar *password)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "password", password, NULL);
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
 * oobs_user_get_password_empty:
 * @user: An #OobsUser.
 *
 * Returns whether the current password for @user is empty.
 *
 * Return Value: %TRUE if @user is using an empty password, %FALSE otherwise.
 **/
gboolean
oobs_user_get_password_empty (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, FALSE);
  g_return_val_if_fail (OOBS_IS_USER (user), FALSE);

  priv = user->_priv;

  return priv->passwd_empty;
}

/**
 * oobs_user_set_password_empty:
 * @user: An #OobsUser.
 * @empty: whether password for @user should be set to empty.
 *
 * Forces an empty password for @user. (Setting the 'password' property
 * to the empty string is used to keep the current password.)
 **/
void
oobs_user_set_password_empty (OobsUser *user, gboolean empty)
{
  OobsUserPrivate *priv;

  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  priv = user->_priv;

  priv->passwd_empty = empty;
}

/**
 * oobs_user_get_password_disabled:
 * @user: An #OobsUser.
 *
 * Returns whether account for @user is currently disabled,
 * i.e. user is not allowed to log in.
 *
 * Return Value: %TRUE if account is disabled, %FALSE otherwise.
 **/
gboolean
oobs_user_get_password_disabled (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, FALSE);
  g_return_val_if_fail (OOBS_IS_USER (user), FALSE);

  priv = user->_priv;

  return priv->passwd_disabled;
}

/**
 * oobs_user_set_password_disabled:
 * @user: An #OobsUser.
 * @disabled: whether account for @user should be disabled.
 *
 * Disable or enable account, allowing or preventing @user from logging in.
 **/
void
oobs_user_set_password_disabled (OobsUser *user, gboolean disabled)
{
  OobsUserPrivate *priv;

  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  priv = user->_priv;

  priv->passwd_disabled = disabled;
}

/**
 * oobs_user_get_encrypted_home:
 * @user: An #OobsUser.
 *
 * Returns whether home directory for @user is encrypted (e.g. using eCryptfs).
 *
 * Return Value: %TRUE if home is encrypted, %FALSE otherwise.
 **/
gboolean
oobs_user_get_encrypted_home (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, FALSE);
  g_return_val_if_fail (OOBS_IS_USER (user), FALSE);

  priv = user->_priv;

  return priv->encrypted_home;
}

/**
 * oobs_user_set_encrypted_home:
 * @user: An #OobsUser.
 * @encrypted_home: whether home directory for @user should be encrypted.
 *
 * Set whether home directory for @user should be encrypted.
 * This function should only be used on new users before committing them,
 * and when the system supports it for the change to take effect.
 **/
void
oobs_user_set_encrypted_home (OobsUser *user, gboolean encrypted_home)
{
  OobsUserPrivate *priv;

  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  priv = user->_priv;

  priv->encrypted_home = encrypted_home;
}

/**
 * oobs_user_set_home_flags:
 * @user: An #OobsUser.
 * @home_flags: how home directory for @user should be treated.
 *
 * Set the flags affecting treatment of the user's home directory (remove home
 * when deleting user, chown directory to user...).
 **/
void
oobs_user_set_home_flags (OobsUser *user, OobsUserHomeFlags home_flags)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "home-flags", home_flags, NULL);
}

/**
 * oobs_user_set_locale:
 * @user: An #OobsUser.
 * @locale: Preferred locale for @user.
 *
 * Get the ISO 639 code representing the current locale for @user.
 **/
G_CONST_RETURN gchar*
oobs_user_get_locale (OobsUser *user)
{
  OobsUserPrivate *priv;

  g_return_val_if_fail (user != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_USER (user), NULL);

  priv = user->_priv;

  return priv->locale;
}

/**
 * oobs_user_set_locale:
 * @user: An #OobsUser.
 * @locale: Preferred locale for @user.
 *
 * Sets the ISO 639 code representing the current locale for @user.
 **/
void
oobs_user_set_locale (OobsUser *user, const gchar *locale)
{
  g_return_if_fail (user != NULL);
  g_return_if_fail (OOBS_IS_USER (user));

  g_object_set (G_OBJECT (user), "locale", locale, NULL);
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
  struct utmpx *entry;
  const gchar *login;
  gboolean match = FALSE;

  g_return_val_if_fail (OOBS_IS_USER (user), FALSE);

  login = oobs_user_get_login_name (user);

  setutxent ();

  while (!match && (entry = getutxent ()) != NULL)
     {
       match = (entry->ut_type == USER_PROCESS &&
               strcmp (entry->ut_user, login) == 0);
     }

  /* close utmp */
  endutxent ();

  return match;
}

/**
 * oobs_user_is_root:
 * @user: An #OobsUser.
 *
 * Checks whether a group is the superuser, according to its name.
 *
 * Return value: %TRUE if @user is the root user, %FALSE otherwise.
 **/
gboolean
oobs_user_is_root (OobsUser *user)
{
  const gchar *login;

  g_return_val_if_fail (OOBS_IS_USER (user), FALSE);

  login = oobs_user_get_login_name (user);

  if (!login)
    return FALSE;

  return (strcmp (login, "root") == 0);
}

/**
 * oobs_user_is_in_group:
 * @user: An #OobsUser.
 * @group: An #OobsGroup.
 *
 * Checks whether a user member of @group.
 *
 * Return value: %TRUE if @user is in @group, %FALSE otherwise.
 **/
gboolean
oobs_user_is_in_group (OobsUser *user, OobsGroup *group)
{
  OobsUser *tmp_user;
  GList *users = NULL;
  GList *l;

  g_return_val_if_fail (OOBS_IS_USER (user), FALSE);
  g_return_val_if_fail (OOBS_IS_GROUP (group), FALSE);

  users = oobs_group_get_users (group);

  for (l = users; l; l = l->next) {
    tmp_user = l->data;
    if (tmp_user == user)
      break;
  }

  g_list_free (users);

  return l != NULL;
}
