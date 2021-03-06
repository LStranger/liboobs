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

#ifndef __OOBS_SESSION_H
#define __OOBS_SESSION_H

G_BEGIN_DECLS

#include <glib-object.h>
#include "oobs-result.h"

#define OOBS_TYPE_SESSION         (oobs_session_get_type ())
#define OOBS_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_SESSION, OobsSession))
#define OOBS_SESSION_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_SESSION, OobsSessionClass))
#define OOBS_IS_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_SESSION))
#define OOBS_IS_SESSION_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_SESSION))
#define OOBS_SESSION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_SESSION, OobsSessionClass))

typedef struct _OobsPlatform OobsPlatform;
struct _OobsPlatform
{
  const gchar *id;
  const gchar *name;
  const gchar *version;
  const gchar *codename;
};

typedef struct _OobsSession      OobsSession;
typedef struct _OobsSessionClass OobsSessionClass;

struct _OobsSession
{
  GObject parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsSessionClass
{
  GObjectClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
};

GType        oobs_session_get_type (void);

OobsSession *oobs_session_get      (void);
OobsResult   oobs_session_commit   (OobsSession *session);

gboolean     oobs_session_get_connected (OobsSession  *session);
OobsResult   oobs_session_get_supported_platforms (OobsSession  *session,
						   GList       **platforms);
OobsResult   oobs_session_get_platform            (OobsSession  *session,
						   gchar       **platform);
OobsResult   oobs_session_set_platform            (OobsSession  *session,
						   const gchar  *platform);

void         oobs_session_process_requests  (OobsSession *session);

G_CONST_RETURN gchar * oobs_session_get_authentication_action (OobsSession *session);

G_END_DECLS

#endif /* __OOBS_SESSION_H */
