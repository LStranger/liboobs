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

#include <dbus/dbus.h>
#include <glib-object.h>
#include <string.h>
#include "oobs-object.h"
#include "oobs-list.h"
#include "oobs-list-private.h"
#include "oobs-shellsconfig.h"
#include "oobs-shell.h"

#define SHELLS_CONFIG_REMOTE_OBJECT "ShellsConfig"
#define OOBS_SHELLS_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SHELLS_CONFIG, OobsShellsConfigPrivate))

typedef struct _OobsShellsConfigPrivate OobsShellsConfigPrivate;

struct _OobsShellsConfigPrivate
{
  OobsList *shells_list;
};


static void oobs_shells_config_class_init (OobsShellsConfigClass *class);
static void oobs_shells_config_init       (OobsShellsConfig      *shells_list);
static void oobs_shells_config_finalize   (GObject               *object);

static void oobs_shells_config_update     (OobsObject   *object,
					   gpointer     data);
static void oobs_shells_config_commit     (OobsObject   *object,
					   gpointer     data);


G_DEFINE_TYPE (OobsShellsConfig, oobs_shells_config, OOBS_TYPE_OBJECT);

static void
oobs_shells_config_class_init (OobsShellsConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize    = oobs_shells_config_finalize;
  oobs_object_class->commit = oobs_shells_config_commit;
  oobs_object_class->update = oobs_shells_config_update;

  g_type_class_add_private (object_class,
			    sizeof (OobsShellsConfigPrivate));
}

static void
oobs_shells_config_init (OobsShellsConfig *object)
{
  OobsShellsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SHELLS_CONFIG (object));

  priv = OOBS_SHELLS_CONFIG_GET_PRIVATE (object);

  priv->shells_list = _oobs_list_new (OOBS_TYPE_SHELL);
}

static void
oobs_shells_config_finalize (GObject *object)
{
  OobsShellsConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SHELLS_CONFIG (object));

  priv = OOBS_SHELLS_CONFIG_GET_PRIVATE (object);

  if (priv && priv->shells_list)
    g_object_unref (priv->shells_list);

  if (G_OBJECT_CLASS (oobs_shells_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_shells_config_parent_class)->finalize) (object);
}

static void
oobs_shells_config_update (OobsObject *object, gpointer data)
{
  OobsShellsConfigPrivate *priv;
  DBusMessage      *reply = (DBusMessage *) data;
  DBusMessageIter   iter, array_iter;
  GObject          *sh;
  OobsListIter      list_iter;
  char             *shell;

  priv = OOBS_SHELLS_CONFIG_GET_PRIVATE (object);

  /* First of all, free the previous shares list */
  oobs_list_clear (priv->shells_list);

  /* start recursing through the response array */
  dbus_message_iter_init    (reply, &iter);
  dbus_message_iter_recurse (&iter, &array_iter);

  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRING)
    {
      dbus_message_iter_get_basic (&array_iter, &shell);
      sh = G_OBJECT (oobs_shell_new (shell));

      oobs_list_append (priv->shells_list, &list_iter);
      oobs_list_set    (priv->shells_list, &list_iter, G_OBJECT (sh));
      g_object_unref   (sh);

      dbus_message_iter_next (&array_iter);
    }
}

static void
oobs_shells_config_commit (OobsObject *object, gpointer data)
{
}

OobsObject*
oobs_shells_config_new (OobsSession *session)
{
  static OobsObject *object = NULL;

  g_return_val_if_fail (session != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

  if (!object)
    {
      object = g_object_new (OOBS_TYPE_SHELLS_CONFIG,
			     "remote-object", SHELLS_CONFIG_REMOTE_OBJECT,
			     "session",       session,
			     NULL);

      oobs_object_update (object);
    }

  return object;
}

OobsList*
oobs_shells_config_get_shells (OobsShellsConfig *config)
{
  OobsShellsConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SHELLS_CONFIG (config), NULL);

  priv = OOBS_SHELLS_CONFIG_GET_PRIVATE (config);

  return priv->shells_list;
}
