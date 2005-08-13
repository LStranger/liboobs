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
#include "oobs-shell.h"

#define OOBS_SHELL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SHELL, OobsShellPrivate))

typedef struct _OobsShellPrivate OobsShellPrivate;

struct _OobsShellPrivate {
  gchar *path;
};

static void oobs_shell_class_init (OobsShellClass *class);
static void oobs_shell_init       (OobsShell      *shell);
static void oobs_shell_finalize   (GObject        *object);

static void oobs_shell_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec);
static void oobs_shell_get_property (GObject      *object,
				     guint         prop_id,
				     GValue       *value,
				     GParamSpec   *pspec);
enum
{
  PROP_0,
  PROP_PATH,
};

G_DEFINE_TYPE (OobsShell, oobs_shell, G_TYPE_OBJECT);

static void
oobs_shell_class_init (OobsShellClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = oobs_shell_set_property;
  object_class->get_property = oobs_shell_get_property;
  object_class->finalize     = oobs_shell_finalize;

  g_object_class_install_property (object_class,
				   PROP_PATH,
				   g_param_spec_string ("path",
							"Shell path",
							"Path to the shell",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsShellPrivate));
}

static void
oobs_shell_init (OobsShell *shell)
{
  OobsShellPrivate *priv;

  g_return_if_fail (OOBS_IS_SHELL (shell));

  priv = OOBS_SHELL_GET_PRIVATE (shell);
  priv->path = NULL;
}

static void
oobs_shell_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  OobsShell *shell;
  OobsShellPrivate *priv;

  g_return_if_fail (OOBS_IS_SHELL (object));

  shell  = OOBS_SHELL (object);
  priv   = OOBS_SHELL_GET_PRIVATE (shell);

  switch (prop_id)
    {
    case PROP_PATH:
      g_free (priv->path);
      priv->path = g_value_dup_string (value);
      break;
    }
}

static void
oobs_shell_get_property (GObject      *object,
			 guint         prop_id,
			 GValue       *value,
			 GParamSpec   *pspec)
{
  OobsShell *shell;
  OobsShellPrivate *priv;

  g_return_if_fail (OOBS_IS_SHELL (object));

  shell  = OOBS_SHELL (object);
  priv   = OOBS_SHELL_GET_PRIVATE (shell);

  switch (prop_id)
    {
    case PROP_PATH:
      g_value_set_string (value, priv->path);
      break;
    }
}

static void
oobs_shell_finalize (GObject *object)
{
  OobsShell        *shell;
  OobsShellPrivate *priv;

  g_return_if_fail (OOBS_IS_SHELL (object));

  shell = OOBS_SHELL (object);
  priv  = OOBS_SHELL_GET_PRIVATE (shell);

  if (priv)
    g_free (priv->path);

  if (G_OBJECT_CLASS (oobs_shell_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_shell_parent_class)->finalize) (object);
}

OobsShell*
oobs_shell_new (const gchar *path)
{
  return g_object_new (OOBS_TYPE_SHELL,
		       "path", path,
		       NULL);
}

G_CONST_RETURN gchar*
oobs_shell_get_path (OobsShell *shell)
{
  OobsShellPrivate *priv;

  g_return_val_if_fail (shell != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SHELL (shell), NULL);

  priv = OOBS_SHELL_GET_PRIVATE (shell);

  return priv->path;
}
