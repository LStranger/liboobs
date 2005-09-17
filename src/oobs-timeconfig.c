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
#include <sys/time.h>
#include <time.h>

#include "oobs-object.h"
#include "oobs-timeconfig.h"

#define TIME_CONFIG_REMOTE_OBJECT "TimeConfig"
#define OOBS_TIME_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_TIME_CONFIG, OobsTimeConfigPrivate))

typedef struct _OobsTimeConfigPrivate OobsTimeConfigPrivate;

struct _OobsTimeConfigPrivate
{
  gboolean time_is_set;
  GTimeVal time;
};

static void oobs_time_config_class_init (OobsTimeConfigClass *class);
static void oobs_time_config_init       (OobsTimeConfig      *config);
static void oobs_time_config_finalize   (GObject             *object);
/*
static void oobs_time_config_set_property (GObject      *object,
					   guint         prop_id,
					   const GValue *value,
					   GParamSpec   *pspec);
static void oobs_time_config_get_property (GObject      *object,
					   guint         prop_id,
					   GValue       *value,
					   GParamSpec   *pspec);
*/
static void oobs_time_config_update     (OobsObject   *object,
					 gpointer     data);
static void oobs_time_config_commit     (OobsObject   *object,
					 gpointer     data);



G_DEFINE_TYPE (OobsTimeConfig, oobs_time_config, OOBS_TYPE_OBJECT);

static void
oobs_time_config_class_init (OobsTimeConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  /*
  object_class->set_property = oobs_users_config_set_property;
  object_class->get_property = oobs_users_config_get_property;
  */
  object_class->finalize     = oobs_time_config_finalize;

  oobs_object_class->commit  = oobs_time_config_commit;
  oobs_object_class->update  = oobs_time_config_update;
}

static void
oobs_time_config_init (OobsTimeConfig *config)
{
  OobsTimeConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (config));

  priv = OOBS_TIME_CONFIG_GET_PRIVATE (config);

  priv->time_is_set = FALSE;
}

static void
oobs_time_config_finalize (GObject *object)
{
  OobsTimeConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (object));

  priv = OOBS_TIME_CONFIG_GET_PRIVATE (object);

  if (G_OBJECT_CLASS (oobs_time_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_time_config_parent_class)->finalize) (object);
}

static void
oobs_time_config_update (OobsObject *object, gpointer data)
{
}

static void
oobs_time_config_commit (OobsObject *object, gpointer data)
{
}

OobsObject*
oobs_time_config_new (OobsSession *session)
{
  static OobsObject *object = NULL;

  if (!object)
    {
      object = g_object_new (OOBS_TYPE_TIME_CONFIG,
			     "remote-object", TIME_CONFIG_REMOTE_OBJECT,
			     "session",       session,
			     NULL);

      oobs_object_update (object);
    }

  return object;
}

static gboolean 
is_leap_year (gint year)
{
  return ((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0));
}

static gboolean
date_is_sane (gint year,
	      gint month,
	      gint day,
	      gint hour,
	      gint minute,
	      gint second)
{
  static const gint month_lengths[2][12] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
  };

  gint month_days;
  gboolean is_leap;

  g_return_val_if_fail ((year < 1), FALSE);
  g_return_val_if_fail ((month < 1 || month > 12), FALSE);

  is_leap = is_leap_year (year);
  month_days = month_lengths [is_leap][month];

  g_return_val_if_fail ((day < 1 || day > month_days), FALSE);
  g_return_val_if_fail ((hour < 0 || hour > 23), FALSE);
  g_return_val_if_fail ((minute < 0 || minute > 59), FALSE);
  g_return_val_if_fail ((second < 0 || second > 59), FALSE);

  return TRUE;
}

void
oobs_time_get_time (OobsTimeConfig *config,
		    gint           *year,
		    gint           *month,
		    gint           *day,
		    gint           *hour,
		    gint           *minute,
		    gint           *second)
{
  OobsTimeConfigPrivate *priv;
  GTimeVal tv;
  struct tm *tm;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (config));

  priv = OOBS_TIME_CONFIG_GET_PRIVATE (config);

  if (!priv->time_is_set)
    {
      /* return the current time */
      /* FIXME: using local information instead
	 of backend data, this avoids latency, but
	 won't work when remote configuration is
	 possible */
      g_get_current_time (&tv);
    }
  else
    {
      /* once set, the time configuration will
	 stop getting the current time */
      tv.tv_sec  = priv->time.tv_sec;
      tv.tv_usec = priv->time.tv_usec;
    }

  tm = localtime (&tv.tv_sec);

  if (year)
    *year = tm->tm_year;

  if (month)
    *month = tm->tm_mon;

  if (day)
    *day = tm->tm_mday;

  if (hour)
    *hour = tm->tm_hour;

  if (minute)
    *minute = tm->tm_min;

  if (second)
    *second = tm->tm_sec;

  g_free (tm);
}

void
oobs_time_set_time (OobsTimeConfig *config,
		    gint            year,
		    gint            month,
		    gint            day,
		    gint            hour,
		    gint            minute,
		    gint            second)
{
  OobsTimeConfigPrivate *priv;
  struct tm tm;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (config));
  g_return_if_fail (date_is_sane (year, month, day, hour, minute, second));

  priv = OOBS_TIME_CONFIG_GET_PRIVATE (config);

  tm.tm_year = year;
  tm.tm_mon  = month;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min  = minute;
  tm.tm_sec  = second;

  priv->time.tv_sec  = mktime (&tm);
  priv->time.tv_usec = 0;
  priv->time_is_set  = TRUE;
}

G_CONST_RETURN gchar*
oobs_time_get_timezone (OobsTimeConfig *config)
{
  return NULL;
}

void
oobs_time_set_timezone (OobsTimeConfig *config,
			const gchar    *timezone)
{

}
