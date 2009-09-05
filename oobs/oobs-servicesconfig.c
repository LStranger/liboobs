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
#include "oobs-servicesconfig.h"
#include "oobs-service.h"
#include "utils.h"

/**
 * SECTION:oobs-servicesconfig
 * @title: OobsServicesConfig
 * @short_description: Object that represents the configuration of services that start/stop during init/shutdown
 * @see_also: #OobsService
 **/

#define SERVICES_CONFIG_REMOTE_OBJECT "ServicesConfig"
#define OOBS_SERVICES_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_SERVICES_CONFIG, OobsServicesConfigPrivate))

typedef struct _OobsServicesConfigPrivate OobsServicesConfigPrivate;

struct _OobsServicesConfigPrivate
{
  OobsList *services_list;
  GList *runlevels;
  OobsServicesRunlevel *default_runlevel;
};

static void oobs_services_config_class_init (OobsServicesConfigClass *class);
static void oobs_services_config_init       (OobsServicesConfig      *config);
static void oobs_services_config_finalize   (GObject                 *object);

static void oobs_services_config_update     (OobsObject   *object);
static void oobs_services_config_commit     (OobsObject   *object);


G_DEFINE_TYPE (OobsServicesConfig, oobs_services_config, OOBS_TYPE_OBJECT);

static void
oobs_services_config_class_init (OobsServicesConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->finalize    = oobs_services_config_finalize;
  oobs_object_class->commit = oobs_services_config_commit;
  oobs_object_class->update = oobs_services_config_update;

  g_type_class_add_private (object_class,
			    sizeof (OobsServicesConfigPrivate));
}

static void
oobs_services_config_init (OobsServicesConfig *config)
{
  OobsServicesConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SERVICES_CONFIG (config));

  priv = OOBS_SERVICES_CONFIG_GET_PRIVATE (config);

  priv->services_list = _oobs_list_new (OOBS_TYPE_SERVICE);
  config->_priv = priv;
}

static void
free_runlevel (OobsServicesRunlevel *runlevel)
{
  g_free (runlevel->name);
  g_free (runlevel);
}

static void
oobs_services_config_finalize (GObject *object)
{
  OobsServicesConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_SERVICES_CONFIG (object));

  priv = OOBS_SERVICES_CONFIG (object)->_priv;

  if (priv)
    {
      if (priv->services_list)
	g_object_unref (priv->services_list);

      if (priv->runlevels)
	{
	  g_list_foreach (priv->runlevels, (GFunc) free_runlevel, NULL);
	  g_list_free (priv->runlevels);
	}

      priv->default_runlevel = NULL;
    }

  if (G_OBJECT_CLASS (oobs_services_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_services_config_parent_class)->finalize) (object);
}

static gint
runlevel_to_role (const gchar *runlevel)
{
  if (strcmp (runlevel, "0") == 0)
    return OOBS_RUNLEVEL_HALT;
  else if (strcmp (runlevel, "1") == 0)
    return OOBS_RUNLEVEL_MONOUSER;
  else if (strcmp (runlevel, "6") == 0)
    return OOBS_RUNLEVEL_REBOOT;

  /* fallback value */
  return OOBS_RUNLEVEL_MULTIUSER;
}

static void
create_runlevels_list_from_dbus_reply (OobsObject      *object,
				       DBusMessage     *reply,
				       DBusMessageIter  iter)
{
  OobsServicesConfigPrivate *priv;
  OobsServicesRunlevel *runlevel;
  const gchar *name;

  priv = OOBS_SERVICES_CONFIG (object)->_priv;

  while (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING)
    {
      name = utils_get_string (&iter);

      runlevel = g_new0 (OobsServicesRunlevel, 1);
      runlevel->name = g_strdup (name);
      runlevel->role = runlevel_to_role (runlevel->name);

      priv->runlevels = g_list_prepend (priv->runlevels, runlevel);
    }

  priv->runlevels = g_list_reverse (priv->runlevels);
}

static OobsServicesRunlevel*
get_runlevel (OobsServicesConfig *config,
	      const gchar        *runlevel)
{
  OobsServicesConfigPrivate *priv;
  OobsServicesRunlevel *rl;
  GList *list;

  if (!runlevel)
    return NULL;

  priv = config->_priv;
  list = priv->runlevels;

  while (list)
    {
      rl = list->data;

      if (strcmp (rl->name, runlevel) == 0)
	return rl;

      list = list->next;
    }

  return NULL;
}

static void
create_service_runlevels_from_dbus_reply (OobsServicesConfig *config,
					  OobsService        *service,
					  DBusMessage        *reply,
					  DBusMessageIter     struct_iter)
{
  DBusMessageIter runlevel_iter;
  OobsServicesRunlevel *rl;
  OobsServiceStatus status;
  const gchar *runlevel;
  gint priority;

  while (dbus_message_iter_get_arg_type (&struct_iter) == DBUS_TYPE_STRUCT)
    {
      dbus_message_iter_recurse (&struct_iter, &runlevel_iter);

      runlevel = utils_get_string (&runlevel_iter);
      status = utils_get_int (&runlevel_iter);
      priority = utils_get_int (&runlevel_iter);

      rl = get_runlevel (config, runlevel);

      if (rl)
	oobs_service_set_runlevel_configuration (service, rl, status, priority);

      dbus_message_iter_next (&struct_iter);
    }
}

static OobsService*
create_service_from_dbus_reply (OobsServicesConfig *config,
				DBusMessage        *reply,
				DBusMessageIter     struct_iter)
{
  GObject *service;
  DBusMessageIter iter, runlevels_iter;
  const gchar *name;

  dbus_message_iter_recurse (&struct_iter, &iter);

  name = utils_get_string (&iter);

  service = g_object_new (OOBS_TYPE_SERVICE,
			  "name", name,
			  NULL);

  dbus_message_iter_recurse (&iter, &runlevels_iter);
  create_service_runlevels_from_dbus_reply (config,
					    OOBS_SERVICE (service),
					    reply, runlevels_iter);
  return OOBS_SERVICE (service);
}

static void
oobs_services_config_update (OobsObject *object)
{
  OobsServicesConfigPrivate *priv;
  DBusMessage     *reply;
  DBusMessageIter  iter, elem_iter;
  OobsListIter     list_iter;
  OobsService     *service;
  const gchar     *default_runlevel;

  priv  = OOBS_SERVICES_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  _oobs_list_set_locked (priv->services_list, FALSE);

  /* First of all, free the previous config */
  oobs_list_clear (priv->services_list);

  dbus_message_iter_init (reply, &iter);

  dbus_message_iter_recurse (&iter, &elem_iter);
  create_runlevels_list_from_dbus_reply (object, reply, elem_iter);
  dbus_message_iter_next (&iter);

  default_runlevel = utils_get_string (&iter);
  priv->default_runlevel = get_runlevel (OOBS_SERVICES_CONFIG (object), default_runlevel);

  dbus_message_iter_recurse (&iter, &elem_iter);

  while (dbus_message_iter_get_arg_type (&elem_iter) == DBUS_TYPE_STRUCT)
    {
      service = create_service_from_dbus_reply (OOBS_SERVICES_CONFIG (object),
						reply, elem_iter);

      oobs_list_append (priv->services_list, &list_iter);
      oobs_list_set    (priv->services_list, &list_iter, G_OBJECT (service));
      g_object_unref   (service);

      dbus_message_iter_next (&elem_iter);
    }

  _oobs_list_set_locked (priv->services_list, TRUE);
}

static void
create_dbus_struct_from_service_runlevels (OobsService     *service,
					   GList           *runlevels,
					   DBusMessage     *message,
					   DBusMessageIter *iter)
{
  DBusMessageIter runlevels_iter, struct_iter;
  OobsServicesRunlevel *runlevel;
  OobsServiceStatus status;
  gint priority;

  dbus_message_iter_open_container (iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING,
				    &runlevels_iter);

  while (runlevels)
    {
      runlevel = runlevels->data;
      runlevels = runlevels->next;

      oobs_service_get_runlevel_configuration (service, runlevel, &status, &priority);

      if (status == OOBS_SERVICE_IGNORE)
	continue;

      dbus_message_iter_open_container (&runlevels_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

      utils_append_string (&struct_iter, runlevel->name);
      utils_append_int (&struct_iter, status);
      utils_append_int (&struct_iter, priority);

      dbus_message_iter_close_container (&runlevels_iter, &struct_iter);
    }

  dbus_message_iter_close_container (iter, &runlevels_iter);
}

static gboolean
create_dbus_struct_from_service (OobsService     *service,
				 GList           *runlevels,
				 DBusMessage     *message,
				 DBusMessageIter *array_iter)
{
  DBusMessageIter struct_iter;
  gchar *name;

  g_object_get (G_OBJECT (service),
		"name", &name,
		NULL);

  g_return_val_if_fail (name, FALSE);

  dbus_message_iter_open_container (array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter);

  utils_append_string (&struct_iter, name);
  create_dbus_struct_from_service_runlevels (service, runlevels, message, &struct_iter);

  dbus_message_iter_close_container (array_iter, &struct_iter);

  g_free (name);

  return TRUE;
}

static void
oobs_services_config_commit (OobsObject *object)
{
  OobsServicesConfigPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter, array_iter;
  OobsListIter list_iter;
  gboolean valid, correct;
  GObject *service;

  correct = TRUE;
  priv = OOBS_SERVICES_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);
  dbus_message_iter_init_append (message, &iter);

  /* FIXME: nothing inserted here, backends ignore this */
  utils_create_dbus_array_from_string_list (NULL, message, &iter);

  /* FIXME: this is ignored by the backend too */
  utils_append_string (&iter, NULL);

  dbus_message_iter_open_container (&iter,
				    DBUS_TYPE_ARRAY,
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_ARRAY_AS_STRING
				    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
				    DBUS_TYPE_STRING_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_TYPE_INT32_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING
				    DBUS_STRUCT_END_CHAR_AS_STRING,
				    &array_iter);

  valid = oobs_list_get_iter_first (priv->services_list, &list_iter);

  while (correct && valid)
    {
      service = oobs_list_get (priv->services_list, &list_iter);
      correct = create_dbus_struct_from_service (OOBS_SERVICE (service),
						 priv->runlevels,
						 message, &array_iter);
      g_object_unref (service);
      valid = oobs_list_iter_next (priv->services_list, &list_iter);
    }

  dbus_message_iter_close_container (&iter, &array_iter);

  if (!correct)
    {
      /* malformed data, unset the message */
      _oobs_object_set_dbus_message (object, NULL);
    }
}

/**
 * oobs_services_config_get:
 * 
 * Returns the #OobsServicesConfig singleton, which represents
 * the services that are run during system init.
 * 
 * Return Value: the singleton #OobsServicesConfig object.
 **/
OobsObject*
oobs_services_config_get (void)
{
  return g_object_new (OOBS_TYPE_SERVICES_CONFIG,
		       "remote-object", SERVICES_CONFIG_REMOTE_OBJECT,
		       NULL);
}

/**
 * oobs_services_config_get_services:
 * @config: An #OobsServicesConfig.
 * 
 * Returns an #OobsList containing objects of type #OobsService.
 * The returned #OobsList is locked, meaning that new elements
 * can't be added nor removed.
 * 
 * Return Value: an #OobsList containing the services list.
 **/
OobsList*
oobs_services_config_get_services (OobsServicesConfig *config)
{
  OobsServicesConfigPrivate *priv;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (OOBS_IS_SERVICES_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->services_list;
}

/**
 * oobs_services_config_get_runlevels:
 * @config: An #OobsServicesConfig.
 * 
 * Returns a list of #OobsServicesRunlevel describing the available
 * runlevels.
 * 
 * Return Value: list of runlevels. the list must be freed with
 *               g_list_free ();
 **/
GList*
oobs_services_config_get_runlevels (OobsServicesConfig *config)
{
  OobsServicesConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SERVICES_CONFIG (config), NULL);
  
  priv = config->_priv;

  return g_list_copy (priv->runlevels);
}

/**
 * oobs_services_config_get_default_runlevel:
 * @config: An #OobsServicesConfig.
 * 
 * Returns the current runlevel.
 * 
 * Return Value: An #OobsServicesRunlevel describing the current
 *               runlevel. This value must not be freed, modified,
 *               or stored.
 **/
G_CONST_RETURN OobsServicesRunlevel*
oobs_services_config_get_default_runlevel (OobsServicesConfig *config)
{
  OobsServicesConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_SERVICES_CONFIG (config), NULL);
  
  priv = config->_priv;

  return priv->default_runlevel;
}
