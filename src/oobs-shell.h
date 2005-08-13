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

#ifndef __OOBS_SHELL_H__
#define __OOBS_SHELL_H__

G_BEGIN_DECLS

#include "oobs-object.h"

#define OOBS_TYPE_SHELL         (oobs_shell_get_type())
#define OOBS_SHELL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_SHELL, OobsShell))
#define OOBS_SHELL_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_SHELL, OobsShellClass))
#define OOBS_IS_SHELL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_SHELL))
#define OOBS_IS_SHELL_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    OOBS_TYPE_SHELL))
#define OOBS_SHELL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_SHELL, OobsShellClass))

typedef struct _OobsShell        OobsShell;
typedef struct _OobsShellClass   OobsShellClass;
	
struct _OobsShell {
  GObject parent;
};

struct _OobsShellClass {
  GObjectClass parent_class;
};

GType                  oobs_shell_get_type   (void);

OobsShell*             oobs_shell_new        (const gchar *path);
G_CONST_RETURN gchar*  oobs_shell_get_path   (OobsShell *shell);


G_END_DECLS

#endif /* __OOBS_SHELL_H__ */
