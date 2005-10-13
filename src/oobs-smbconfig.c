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

#define SMB_CONFIG_REMOTE_OBJECT "SMBConfig"
#define OOBS_SMB_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SMB_CONFIG, OobsSMBConfigPrivate))

typedef struct _OobsSMBConfigPrivate OobsSMBConfigPrivate;

struct _OobsSMBConfigPrivate
{
  OobsList *shares_list;
};

static void oobs_smb_config_class_init (OobsSMBConfigClass *class);
static void oobs_smb_config_init       (OobsSMBConfig      *config);
static void oobs_smb_config_finalize   (GObject            *object);

static void oobs_smb_config_update     (OobsObject   *object);
static void oobs_smb_config_commit     (OobsObject   *object);


G_DEFINE_TYPE (OobsSMBConfig, oobs_smb_config, OOBS_TYPE_OBJECT);

static void
oobs_smb_config_class_init (OobsSMBConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize    = oobs_smb_config_finalize;
  oobs_object_class->commit = oobs_smb_config_commit;
  oobs_object_class->update = oobs_smb_config_update;

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
}

static void
oobs_smb_config_finalize (GObject *object)
{
  OobsSMBConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SMB_CONFIG (object));

  priv = OOBS_SMB_CONFIG_GET_PRIVATE (object);

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

  priv  = OOBS_SMB_CONFIG_GET_PRIVATE (object);
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous shares list */
  oobs_list_clear (priv->shares_list);

  /* start recursing through the response array */
  dbus_message_iter_init    (reply, &iter);
  dbus_message_iter_recurse (&iter, &array_iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRUCT)
    {
      share = create_share_from_dbus_reply (object, reply, array_iter);

      oobs_list_append (priv->shares_list, &list_iter);
      oobs_list_set    (priv->shares_list, &list_iter, G_OBJECT (share));
      g_object_unref   (share);

      dbus_message_iter_next (&array_iter);
    }
}

static void
oobs_smb_config_commit (OobsObject *object)
{
}

OobsObject*
oobs_smb_config_new (OobsSession *session)
{
  OobsObject *object;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  object = g_object_new (OOBS_TYPE_SMB_CONFIG,
			 "remote-object", SMB_CONFIG_REMOTE_OBJECT,
			 "session",       session,
			 NULL);

  oobs_object_update (object);
  return object;
}

OobsList*
oobs_smb_config_get_shares (OobsSMBConfig *config)
{
  OobsSMBConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SMB_CONFIG (config), NULL);

  priv = OOBS_SMB_CONFIG_GET_PRIVATE (config);

  return priv->shares_list;
}
