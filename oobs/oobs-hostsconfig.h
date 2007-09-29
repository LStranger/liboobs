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

#ifndef __OOBS_HOSTS_CONFIG_H
#define __OOBS_HOSTS_CONFIG_H

G_BEGIN_DECLS

#include "oobs-object.h"
#include "oobs-list.h"

#define OOBS_TYPE_HOSTS_CONFIG         (oobs_hosts_config_get_type ())
#define OOBS_HOSTS_CONFIG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OOBS_TYPE_HOSTS_CONFIG, OobsHostsConfig))
#define OOBS_HOSTS_CONFIG_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    OOBS_TYPE_HOSTS_CONFIG, OobsHostsConfigClass))
#define OOBS_IS_HOSTS_CONFIG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OOBS_TYPE_HOSTS_CONFIG))
#define OOBS_IS_HOSTS_CONFIG_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((o),    OOBS_TYPE_HOSTS_CONFIG))
#define OOBS_HOSTS_CONFIG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  OOBS_TYPE_HOSTS_CONFIG, OobsHostsConfigClass))

typedef struct _OobsHostsConfig      OobsHostsConfig;
typedef struct _OobsHostsConfigClass OobsHostsConfigClass;

struct _OobsHostsConfig
{
  OobsObject parent;

  /*<private>*/
  gpointer _priv;
};

struct _OobsHostsConfigClass
{
  OobsObjectClass parent_class;

  void (*_oobs_padding1) (void);
  void (*_oobs_padding2) (void);
  void (*_oobs_padding3) (void);
  void (*_oobs_padding4) (void);
};

GType       oobs_hosts_config_get_type     (void);

OobsObject* oobs_hosts_config_get          (void);

G_CONST_RETURN gchar*  oobs_hosts_config_get_hostname   (OobsHostsConfig *config);
void                   oobs_hosts_config_set_hostname   (OobsHostsConfig *config,
							 const gchar     *hostname);

G_CONST_RETURN gchar*  oobs_hosts_config_get_domainname (OobsHostsConfig *config);
void                   oobs_hosts_config_set_domainname (OobsHostsConfig *config,
							 const gchar     *domainname);

OobsList*   oobs_hosts_config_get_static_hosts   (OobsHostsConfig *config);

GList*      oobs_hosts_config_get_dns_servers    (OobsHostsConfig *config);
void        oobs_hosts_config_set_dns_servers    (OobsHostsConfig *config,
						  GList           *dns_list);
GList*      oobs_hosts_config_get_search_domains (OobsHostsConfig *config);
void        oobs_hosts_config_set_search_domains (OobsHostsConfig *config,
						  GList           *search_domains_list);

G_END_DECLS

#endif /* __OOBS_HOSTS_CONFIG_H */
