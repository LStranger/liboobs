/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2010 Milan Bouchet-Valat
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
 * Authors: Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#ifndef __OOBS_SERVICES_CONFIG_PRIVATE_H
#define __OOBS_SERVICES_CONFIG_PRIVATE_H

G_BEGIN_DECLS

#include "oobs-servicesconfig.h"

OobsServicesRunlevel* _oobs_services_config_get_runlevel (OobsServicesConfig *config,
                                                          const gchar        *runlevel);

G_END_DECLS

#endif /* __OOBS_SERVICES_CONFIG_PRIVATE_H */
