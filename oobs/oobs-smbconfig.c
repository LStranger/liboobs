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
#include "oobs-object-private.h"
#include "oobs-list.h"
#include "oobs-list-private.h"
#include "oobs-smbconfig.h"
#include "oobs-share.h"
#include "oobs-share-smb.h"
#include "oobs-user.h"
#include "utils.h"

/**
 * SECTION:oobs-smbconfig
 * @title: OobsSMBConfig
 * @short_description: Object that represents SMB configuration
 * @see_also: #OobsShareSMB, #OobsNFSConfig
 **/

#define SMB_CONFIG_REMOTE_OBJECT "SMBConfig"
#define OOBS_SMB_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SMB_CONFIG, OobsSMBConfigPrivate))

typedef struct _OobsSMBConfigPrivate OobsSMBConfigPrivate;

struct _OobsSMBConfigPrivate
{
  OobsList *shares_list;

  gchar *workgroup;
  gchar *desc;
  gchar *wins_server;

  GHashTable *users;

  guint is_wins_server : 1;
};

static void oobs_smb_config_class_init (OobsSMBConfigClass *class);
static void oobs_smb_config_init       (OobsSMBConfig      *config);
static void oobs_smb_config_finalize   (GObject            *object);

static void oobs_smb_config_set_property (GObject      *object,
					  guint         prop_id,
					  const GValue *value,
					  GParamSpec   *pspec);
static void oobs_smb_config_get_property (GObject      *object,
					  guint         prop_id,
					  GValue       *value,
					  GParamSpec   *pspec);

static void oobs_smb_config_update     (OobsObject   *object);
static void oobs_smb_config_commit     (OobsObject   *object);

enum {
  PROP_0,
  PROP_WORKGROUP,
  PROP_DESC,
  PROP_IS_WINS_SERVER,
  PROP_WINS_SERVER
};

G_DEFINE_TYPE (OobsSMBConfig, oobs_smb_config, OOBS_TYPE_OBJECT);

static void
oobs_smb_config_class_init (OobsSMBConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->set_property = oobs_smb_config_set_property;
  object_class->get_property = oobs_smb_config_get_property;
  object_class->finalize     = oobs_smb_config_finalize;

  oobs_object_class->commit  = oobs_smb_config_commit;
  oobs_object_class->update  = oobs_smb_config_update;

  g_object_class_install_property (object_class,
				   PROP_WORKGROUP,
				   g_param_spec_string ("workgroup",
							"Workgroup",
							"Workgroup/domain the host is in",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DESC,
				   g_param_spec_string ("description",
							"Description",
							"Description for this host",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_IS_WINS_SERVER,
				   g_param_spec_boolean ("is_wins_server",
							 "Is WINS server",
							 "Whether the host is the WINS server",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_WINS_SERVER,
				   g_param_spec_string ("wins_server",
							"WINS server",
							"WINS server that the host will use",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsSMBConfigPrivate));
}

static void
oobs_smb_config_init (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));

  priv = OOBS_SMB_CONFIG_GET_PRIVATE (config);

  priv->shares_list = _oobs_list_new (OOBS_TYPE_SHARE_SMB);
  priv->users = g_hash_table_new_full (g_str_hash, g_str_equal,
				       (GDestroyNotify) g_free,
				       (GDestroyNotify) g_free);
  config->_priv = priv;
}

static void
oobs_smb_config_set_property (GObject      *object,
			      guint         prop_id,
			      const GValue *value,
			      GParamSpec   *pspec)
{
  OobsSMBConfigPrivate *priv;

  priv = OOBS_SMB_CONFIG (object)->_priv;

  switch (prop_id)
    {
    case PROP_WORKGROUP:
      priv->workgroup = g_value_dup_string (value);
      break;
    case PROP_DESC:
      priv->desc = g_value_dup_string (value);
      break;
    case PROP_IS_WINS_SERVER:
      priv->is_wins_server = g_value_get_boolean (value);
      break;
    case PROP_WINS_SERVER:
      priv->wins_server = g_value_dup_string (value);
      break;
    }
}

static void
oobs_smb_config_get_property (GObject      *object,
			      guint         prop_id,
			      GValue       *value,
			      GParamSpec   *pspec)
{
  OobsSMBConfigPrivate *priv;

  priv = OOBS_SMB_CONFIG (object)->_priv;

  switch (prop_id)
    {
    case PROP_WORKGROUP:
      g_value_set_string (value, priv->workgroup);
      break;
    case PROP_DESC:
      g_value_set_string (value, priv->desc);
      break;
    case PROP_IS_WINS_SERVER:
      g_value_set_boolean (value, priv->is_wins_server);
      break;
    case PROP_WINS_SERVER:
      g_value_set_string (value, priv->wins_server);
      break;
    }
}

static void
oobs_smb_config_finalize (GObject *object)
{
  OobsSMBConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SMB_CONFIG (object));

  priv = OOBS_SMB_CONFIG (object)->_priv;

  if (priv && priv->shares_list)
    g_object_unref (priv->shares_list);

  if (G_OBJECT_CLASS (oobs_smb_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_smb_config_parent_class)->finalize) (object);
}

static OobsShare*
create_share_from_dbus_reply (OobsObject      *object,
			      DBusMessage     *reply,
			      DBusMessageIter  struct_iter)
{
  DBusMessageIter iter;
  const gchar *name, *comment, *path;
  OobsShareSMBFlags flags;
  gboolean value;

  name    = NULL;
  comment = NULL;
  path    = NULL;
  flags   = 0;

  dbus_message_iter_recurse (&struct_iter, &iter);

  name = utils_get_string (&iter);
  path = utils_get_string (&iter);
  comment = utils_get_string (&iter);
  value = utils_get_int (&iter);

  if (value)
    flags |= OOBS_SHARE_SMB_ENABLED;

  value = utils_get_int (&iter);

  if (value)
    flags |= OOBS_SHARE_SMB_BROWSABLE;

  value = utils_get_int (&iter);

  if (value)
    flags |= OOBS_SHARE_SMB_PUBLIC;

  value = utils_get_int (&iter);

  if (value)
    flags |= OOBS_SHARE_SMB_WRITABLE;

  return oobs_share_smb_new (path, name, comment, flags);
}

static void
update_smb_users (OobsObject      *object,
		  DBusMessageIter *iter)
{
  OobsSMBConfigPrivate *priv;
  DBusMessageIter array_iter, struct_iter;
  gchar *user;

  priv = OOBS_SMB_CONFIG (object)->_priv;
  dbus_message_iter_recurse (iter, &array_iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRUCT)
    {
      dbus_message_iter_recurse (&array_iter, &struct_iter);
      user = utils_dup_string (&struct_iter);
      g_hash_table_insert (priv->users, user, g_strdup (""));

      dbus_message_iter_next (&array_iter);
    }
}

static void
oobs_smb_config_update (OobsObject *object)
{
  OobsSMBConfigPrivate *priv;
  DBusMessage      *reply;
  DBusMessageIter   iter, array_iter;
  OobsListIter      list_iter;
  OobsShare        *share;

  priv  = OOBS_SMB_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous shares config */
  oobs_list_clear (priv->shares_list);
  g_hash_table_remove_all (priv->users);

  /* start recursing through the response array */
  dbus_message_iter_init    (reply, &iter);
  dbus_message_iter_recurse (&iter, &array_iter);
  dbus_message_iter_next (&iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRUCT)
    {
      share = create_share_from_dbus_reply (object, reply, array_iter);

      if (share)
	{
	  oobs_list_append (priv->shares_list, &list_iter);
	  oobs_list_set    (priv->shares_list, &list_iter, G_OBJECT (share));
	  g_object_unref   (share);
	}

      dbus_message_iter_next (&array_iter);
    }

  priv->workgroup = g_strdup (utils_get_string (&iter));
  priv->desc = g_strdup (utils_get_string (&iter));
  priv->is_wins_server = utils_get_int (&iter);
  priv->wins_server = g_strdup (utils_get_string (&iter));

  update_smb_users (object, &iter);
}

static void
create_dbus_struct_from_share (GObject         *share,
			       DBusMessage     *message,
			       DBusMessageIter *iter)
{
  DBusMessageIter struct_iter;
  gchar *name, *comment, *path;
  OobsShareSMBFlags flags;
  gboolean value;

  g_object_get (share,
		"name", &name,
		"comment", &comment,
		"path", &path,
		"flags", &flags,
		NULL);

  dbus_message_iter_open_container (iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

  utils_append_string (&struct_iter, name);
  utils_append_string (&struct_iter, path);
  utils_append_string (&struct_iter, comment);

  value = (flags & OOBS_SHARE_SMB_ENABLED);
  utils_append_int (&struct_iter, value);

  value = (flags & OOBS_SHARE_SMB_BROWSABLE);
  utils_append_int (&struct_iter, value);

  value = (flags & OOBS_SHARE_SMB_PUBLIC);
  utils_append_int (&struct_iter, value);

  value = (flags & OOBS_SHARE_SMB_WRITABLE);
  utils_append_int (&struct_iter, value);

  dbus_message_iter_close_container (iter, &struct_iter);

  g_free (name);
  g_free (comment);
  g_free (path);
}

static void
append_smb_users (OobsObject      *object,
		  DBusMessageIter *iter)
{
  OobsSMBConfigPrivate *priv;
  DBusMessageIter array_iter, struct_iter;
  GList *keys, *k;

  priv = OOBS_SMB_CONFIG (object)->_priv;
  keys = g_hash_table_get_keys (priv->users);

  dbus_message_iter_open_container (iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING,
				    &array_iter);

  for (k = keys; k; k = k->next)
    {
      dbus_message_iter_open_container (&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

      utils_append_string (&struct_iter, k->data);
      utils_append_string (&struct_iter, g_hash_table_lookup (priv->users, k->data));

      dbus_message_iter_close_container (&array_iter, &struct_iter);
    }

  dbus_message_iter_close_container (iter, &array_iter);

  g_list_free (keys);
}

static void
oobs_smb_config_commit (OobsObject *object)
{
  OobsSMBConfigPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter, array_iter;
  OobsListIter list_iter;
  GObject *share;
  gboolean valid;

  priv = OOBS_SMB_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  dbus_message_iter_init_append (message, &iter);
  dbus_message_iter_open_container (&iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING,
				    &array_iter);

  valid = oobs_list_get_iter_first (priv->shares_list, &list_iter);

  while (valid)
    {
      share = oobs_list_get (priv->shares_list, &list_iter);
      create_dbus_struct_from_share (share, message, &array_iter);

      g_object_unref (share);
      valid = oobs_list_iter_next (priv->shares_list, &list_iter);
    }

  dbus_message_iter_close_container (&iter, &array_iter);

  utils_append_string (&iter, priv->workgroup);
  utils_append_string (&iter, priv->desc);
  utils_append_int (&iter, priv->is_wins_server);
  utils_append_string (&iter, priv->wins_server);

  append_smb_users (object, &iter);
}

/**
 * oobs_smb_config_get:
 * 
 * Returns the #OobsSMBConfig singleton, which represents
 * the SMB system configuration.
 * 
 * Return Value: the singleton #OobsSMBConfig object.
 **/
OobsObject*
oobs_smb_config_get (void)
{
  static OobsObject *the_object = NULL;

  if (!the_object)
    the_object = g_object_new (OOBS_TYPE_SMB_CONFIG,
                               "remote-object", SMB_CONFIG_REMOTE_OBJECT,
                               NULL);

  return the_object;
}

/**
 * oobs_smb_config_get_shares:
 * @config: An #OobsSMBConfig.
 * 
 * Returns an #OobsList containing objects of type #OobsShareSMB.
 * 
 * Return Value: an #OobsList containing the SMB shares information.
 **/
OobsList*
oobs_smb_config_get_shares (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->shares_list;
}

/**
 * oobs_smb_config_get_workgroup:
 * @config: An #OobsSMBConfig.
 * 
 * Returns the SMB workgroup for the host.
 * 
 * Return Value: A pointer to the workgroup as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_smb_config_get_workgroup (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), NULL);

  priv = config->_priv;
  return priv->workgroup;
}

/**
 * oobs_smb_config_set_workgroup:
 * @config: An #OobsSMBConfig.
 * @workgroup: a new SMB workgroup for the host.
 * 
 * Sets a new SMB workgroup for the host, overwriting the
 * previous one.
 **/
void
oobs_smb_config_set_workgroup (OobsSMBConfig *config,
			       const gchar   *workgroup)
{
  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));

  g_object_set (config, "workgroup", workgroup, NULL);
}

/**
 * oobs_smb_config_get_description:
 * @config: An #OobsSMBConfig.
 * 
 * Returns the SMB host description.
 * 
 * Return Value: A pointer to the host description as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_smb_config_get_description (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), NULL);

  priv = config->_priv;
  return priv->desc;
}

/**
 * oobs_smb_config_set_description:
 * @config: An #OobsSMBConfig.
 * @description: a new host description.
 * 
 * Sets a new SMB host description, overwriting the previous one.
 **/
void
oobs_smb_config_set_description (OobsSMBConfig *config,
				 const gchar   *description)
{
  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));

  g_object_set (config, "description", description, NULL);
}

/**
 * oobs_smb_config_get_is_wins_server:
 * @config: An #OobsSMBConfig.
 * 
 * Returns whether the host is a WINS server for the network.
 * 
 * Return Value: #TRUE if the host is a WINS server.
 **/
gboolean
oobs_smb_config_get_is_wins_server (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), FALSE);

  priv = config->_priv;
  return priv->is_wins_server;
}

/**
 * oobs_smb_config_set_is_wins_server:
 * @config: An #OobsSMBConfig.
 * @is_wins_server: #TRUE if the host is the WINS server for the network.
 * 
 * Sets whether the host is a WINS server for the network. Note that there
 * must be only one WINS server, so if there's already a WINS server in the
 * network, there wouldn't be the need to set the host as another WINS server.
 **/
void
oobs_smb_config_set_is_wins_server (OobsSMBConfig *config,
				    gboolean       is_wins_server)
{
  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));

  g_object_set (config, "is-wins-server", is_wins_server, NULL);
}

/**
 * oobs_smb_config_get_wins_server:
 * @config: An #OobsSMBConfig.
 * 
 * Returns the WINS server the host is using. 
 * 
 * Return Value: A pointer to the wins server as a string. This
 *               string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_smb_config_get_wins_server (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), NULL);

  priv = config->_priv;
  return priv->wins_server;
}

/**
 * oobs_smb_config_set_wins_server:
 * @config: An #OobsSMBConfig.
 * @wins_server: a new WINS server.
 * 
 * Sets the WINS server that the host will use. This option is
 * mutually exclusive with oobs_smb_config_set_is_wins_server().
 **/
void
oobs_smb_config_set_wins_server (OobsSMBConfig *config,
				 const gchar   *wins_server)
{
  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));

  g_object_set (config, "wins-server", wins_server, NULL);
}

/**
 * oobs_smb_config_user_has_password:
 * @config: An #OobsSMBConfig.
 * @user: An #OobsUser
 *
 * Returns whether the user has a SMB password or not.
 *
 * Return Value: #TRUE if the user has a SMB password set.
 **/
gboolean
oobs_smb_config_user_has_password (OobsSMBConfig *config,
				   OobsUser      *user)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), FALSE);
  g_return_val_if_fail (OOBS_IS_USER (user), FALSE);

  priv = OOBS_SMB_CONFIG_GET_PRIVATE (config);

  return (g_hash_table_lookup (priv->users, oobs_user_get_login_name (user)) != NULL);
}

/**
 * oobs_smb_config_delete_user_password:
 * @config: An #OobsSMBConfig
 * @user: An #OobsUser
 *
 * Deletes the user from the SMB password database.
 **/
void
oobs_smb_config_delete_user_password (OobsSMBConfig *config,
				      OobsUser      *user)
{
  OobsSMBConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));
  g_return_if_fail (OOBS_IS_USER (user));

  priv = OOBS_SMB_CONFIG_GET_PRIVATE (config);

  g_hash_table_remove (priv->users, oobs_user_get_login_name (user));
}

/**
 * oobs_smb_config_set_user_password:
 * @config: An #OobsSMBConfig
 * @user: An #OobsUser
 * @password: new SMB password for the user.
 *
 * Sets a SMB password for the user.
 **/
void
oobs_smb_config_set_user_password (OobsSMBConfig *config,
				   OobsUser      *user,
				   const gchar   *password)
{
  OobsSMBConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));
  g_return_if_fail (OOBS_IS_USER (user));
  g_return_if_fail (password != NULL);

  priv = OOBS_SMB_CONFIG_GET_PRIVATE (config);

  g_hash_table_insert (priv->users,
		       g_strdup (oobs_user_get_login_name (user)),
		       g_strdup (password));
}
