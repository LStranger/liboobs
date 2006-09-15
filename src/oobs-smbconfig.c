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
#include "utils.h"

#define SMB_CONFIG_REMOTE_OBJECT "SMBConfig"
#define OOBS_SMB_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SMB_CONFIG, OobsSMBConfigPrivate))

typedef struct _OobsSMBConfigPrivate OobsSMBConfigPrivate;

struct _OobsSMBConfigPrivate
{
  OobsList *shares_list;

  gchar *workgroup;
  gchar *desc;
  gboolean is_wins_server;
  gchar *wins_server;
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
  gchar *name, *comment, *path;
  OobsShareSMBFlags flags;
  gboolean value;

  name    = NULL;
  comment = NULL;
  path    = NULL;
  flags   = 0;

  dbus_message_iter_recurse (&struct_iter, &iter);

  dbus_message_iter_get_basic (&iter, &name);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &path);
  dbus_message_iter_next (&iter);
  
  dbus_message_iter_get_basic (&iter, &comment);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &value);
  dbus_message_iter_next (&iter);

  if (value)
    flags |= OOBS_SHARE_SMB_ENABLED;

  dbus_message_iter_get_basic (&iter, &value);
  dbus_message_iter_next (&iter);

  if (value)
    flags |= OOBS_SHARE_SMB_BROWSABLE;
  
  dbus_message_iter_get_basic (&iter, &value);
  dbus_message_iter_next (&iter);

  if (value)
    flags |= OOBS_SHARE_SMB_PUBLIC;
  
  dbus_message_iter_get_basic (&iter, &value);
  dbus_message_iter_next (&iter);

  if (value)
    flags |= OOBS_SHARE_SMB_WRITABLE;

  return oobs_share_smb_new (path, name, comment, flags);
}

static void
oobs_smb_config_update (OobsObject *object)
{
  OobsSMBConfigPrivate *priv;
  DBusMessage      *reply;
  DBusMessageIter   iter, array_iter;
  OobsListIter      list_iter;
  OobsShare        *share;
  const gchar      *str;

  priv  = OOBS_SMB_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous shares list */
  oobs_list_clear (priv->shares_list);

  /* start recursing through the response array */
  dbus_message_iter_init    (reply, &iter);
  dbus_message_iter_recurse (&iter, &array_iter);
  dbus_message_iter_next (&iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRUCT)
    {
      share = create_share_from_dbus_reply (object, reply, array_iter);

      oobs_list_append (priv->shares_list, &list_iter);
      oobs_list_set    (priv->shares_list, &list_iter, G_OBJECT (share));
      g_object_unref   (share);

      dbus_message_iter_next (&array_iter);
    }

  str = utils_get_string (&iter);
  priv->workgroup = g_strdup (str);
  dbus_message_iter_next (&iter);

  str = utils_get_string (&iter);
  priv->desc = g_strdup (str);
  dbus_message_iter_next (&iter);

  dbus_message_iter_get_basic (&iter, &priv->is_wins_server);
  dbus_message_iter_next (&iter);

  str = utils_get_string (&iter);
  priv->wins_server = g_strdup (str);
  dbus_message_iter_next (&iter);
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

  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_STRING, &name);
  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_STRING, &path);
  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_STRING, &comment);

  value = (flags & OOBS_SHARE_SMB_ENABLED);
  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &value);
  
  value = (flags & OOBS_SHARE_SMB_BROWSABLE);
  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &value);

  value = (flags & OOBS_SHARE_SMB_PUBLIC);
  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &value);

  value = (flags & OOBS_SHARE_SMB_WRITABLE);
  dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &value);

  dbus_message_iter_close_container (iter, &struct_iter);

  g_free (name);
  g_free (comment);
  g_free (path);
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
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &priv->is_wins_server);
  utils_append_string (&iter, priv->wins_server);
}

/**
 * oobs_smb_config_get:
 * @session: An #OobsSession.
 * 
 * Returns the #OobsSMBConfig singleton, which represents
 * the SMB system configuration.
 * 
 * Return Value: the singleton #OobsSMBConfig object.
 **/
OobsObject*
oobs_smb_config_get (OobsSession *session)
{
  static OobsObject *object = NULL;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  if (!object)
    {
      object = g_object_new (OOBS_TYPE_SMB_CONFIG,
			     "remote-object", SMB_CONFIG_REMOTE_OBJECT,
			     "session",       session,
			     NULL);

      oobs_object_update (object);
    }

  return object;
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

G_CONST_RETURN gchar*
oobs_smb_config_get_workgroup (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), NULL);

  priv = config->_priv;
  return priv->workgroup;
}

void
oobs_smb_config_set_workgroup (OobsSMBConfig *config,
			       const gchar   *workgroup)
{
  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));

  g_object_set (config, "workgroup", workgroup, NULL);
}

G_CONST_RETURN gchar*
oobs_smb_config_get_description (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), NULL);

  priv = config->_priv;
  return priv->desc;
}

void
oobs_smb_config_set_description (OobsSMBConfig *config,
				 const gchar   *description)
{
  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));

  g_object_set (config, "description", description, NULL);
}

gboolean
oobs_smb_config_get_is_wins_server (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), FALSE);

  priv = config->_priv;
  return priv->is_wins_server;
}

void
oobs_smb_config_set_is_wins_server (OobsSMBConfig *config,
				    gboolean       is_wins_server)
{
  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));

  g_object_set (config, "is-wins-server", is_wins_server, NULL);
}

G_CONST_RETURN gchar*
oobs_smb_config_get_wins_server (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), NULL);

  priv = config->_priv;
  return priv->wins_server;
}

void
oobs_smb_config_set_wins_server (OobsSMBConfig *config,
				 const gchar   *wins_server)
{
  g_return_if_fail (OOBS_IS_SMB_CONFIG (config));

  g_object_set (config, "wins-server", wins_server, NULL);
}
