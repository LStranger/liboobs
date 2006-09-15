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
#include <stdlib.h>

#include "oobs-object.h"
#include "oobs-object-private.h"
#include "oobs-timeconfig.h"
#include "utils.h"

#define TIME_CONFIG_REMOTE_OBJECT "TimeConfig"
#define OOBS_TIME_CONFIG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), OOBS_TYPE_TIME_CONFIG, OobsTimeConfigPrivate))

typedef struct _OobsTimeConfigPrivate OobsTimeConfigPrivate;

struct _OobsTimeConfigPrivate
{
  gboolean  time_is_set;
  GTimeVal  time;
  gchar    *timezone;
};

static void oobs_time_config_class_init (OobsTimeConfigClass *class);
static void oobs_time_config_init       (OobsTimeConfig      *config);
static void oobs_time_config_finalize   (GObject             *object);

static void oobs_time_config_set_property (GObject      *object,
					   guint         prop_id,
					   const GValue *value,
					   GParamSpec   *pspec);
static void oobs_time_config_get_property (GObject      *object,
					   guint         prop_id,
					   GValue       *value,
					   GParamSpec   *pspec);

static void oobs_time_config_update     (OobsObject   *object);
static void oobs_time_config_commit     (OobsObject   *object);

enum
{
  PROP_0,
  PROP_UNIX_TIME,
  PROP_TIMEZONE
};

G_DEFINE_TYPE (OobsTimeConfig, oobs_time_config, OOBS_TYPE_OBJECT);

static void
oobs_time_config_class_init (OobsTimeConfigClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  OobsObjectClass *oobs_object_class = OOBS_OBJECT_CLASS (class);

  object_class->set_property = oobs_time_config_set_property;
  object_class->get_property = oobs_time_config_get_property;
  object_class->finalize     = oobs_time_config_finalize;

  oobs_object_class->commit  = oobs_time_config_commit;
  oobs_object_class->update  = oobs_time_config_update;

  g_object_class_install_property (object_class,
				   PROP_UNIX_TIME,
				   g_param_spec_long ("unix-time",
						      "Unix time",
						      "Current unix time for the computer",
						      0,
						      G_MAXLONG,
						      0,
						      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_TIMEZONE,
				   g_param_spec_string ("timezone",
							"Timezone",
							"Timezone for the computer",
							NULL,
							G_PARAM_READWRITE));
  g_type_class_add_private (object_class,
			    sizeof (OobsTimeConfigPrivate));
}

static void
oobs_time_config_init (OobsTimeConfig *config)
{
  OobsTimeConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (config));

  priv = OOBS_TIME_CONFIG_GET_PRIVATE (config);

  priv->time_is_set = FALSE;
  config->_priv = priv;
}

static void
oobs_time_config_finalize (GObject *object)
{
  OobsTimeConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (object));

  priv = OOBS_TIME_CONFIG (object)->_priv;

  if (priv && priv->timezone)
    g_free (priv->timezone);

  if (G_OBJECT_CLASS (oobs_time_config_parent_class)->finalize)
    (* G_OBJECT_CLASS (oobs_time_config_parent_class)->finalize) (object);
}

static void
oobs_time_config_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  OobsTimeConfigPrivate *priv;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (object));

  priv = OOBS_TIME_CONFIG (object)->_priv;

  switch (prop_id)
    {
    case PROP_UNIX_TIME:
      priv->time.tv_sec  = g_value_get_long (value);
      priv->time.tv_usec = 0;
      priv->time_is_set  = TRUE;
      break;
    case PROP_TIMEZONE:
      g_free (priv->timezone);
      priv->timezone = g_value_dup_string (value);
      break;
    }
}

static void
oobs_time_config_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  OobsTimeConfigPrivate *priv;
  GTimeVal tv;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (object));

  priv = OOBS_TIME_CONFIG (object)->_priv;

  switch (prop_id)
    {
    case PROP_UNIX_TIME:
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

      g_value_set_long (value, tv.tv_sec);
      break;
    case PROP_TIMEZONE:
      g_value_set_string (value, priv->timezone);
      break;
    }
}

static void
oobs_time_config_update (OobsObject *object)
{
  OobsTimeConfigPrivate *priv;
  DBusMessage *reply;
  DBusMessageIter iter;
  gchar *timezone;

  priv  = OOBS_TIME_CONFIG (object)->_priv;
  reply = _oobs_object_get_dbus_message (object);

  /* First of all, free the previous config */
  g_free (priv->timezone);

  /* FIXME: skip time & date settings, at the moment
   * we rely on the local configuration, this has
   * to change when we allow remote configuration
   */
  dbus_message_iter_init (reply, &iter);
  dbus_message_iter_next (&iter);
  dbus_message_iter_next (&iter);
  dbus_message_iter_next (&iter);
  dbus_message_iter_next (&iter);
  dbus_message_iter_next (&iter);

  dbus_message_iter_next (&iter);
  dbus_message_iter_get_basic (&iter, &timezone);
  priv->timezone = g_strdup (timezone);
}

static void
oobs_time_config_commit (OobsObject *object)
{
  OobsTimeConfigPrivate *priv;
  DBusMessage *message;
  DBusMessageIter iter;
  gint year, month, day, hour, minute, second;

  priv = OOBS_TIME_CONFIG (object)->_priv;
  message = _oobs_object_get_dbus_message (object);

  oobs_time_config_get_utc_time (OOBS_TIME_CONFIG (object),
				 &year, &month,  &day,
				 &hour, &minute, &second);

  dbus_message_iter_init_append (message, &iter);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &year);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &month);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &day);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &hour);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &minute);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &second);

  utils_append_string (&iter, priv->timezone);

  /* we've just synchronized with the system
   * date, so we don't need this anymore
   */
  priv->time_is_set = FALSE;
}

/**
 * oobs_time_config_get:
 * @session: An #OobsSession.
 * 
 * Returns the #OobsTimeConfig singleton, this object
 * represents the date and time configuration.
 * 
 * Return Value: the singleton #OobsTimeConfig.
 **/
OobsObject*
oobs_time_config_get (OobsSession *session)
{
  static OobsObject *object = NULL;

  g_return_val_if_fail (OOBS_IS_SESSION (session), NULL);

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

  g_return_val_if_fail ((year >= 1), FALSE);
  g_return_val_if_fail ((month >= 0 && month <= 11), FALSE);

  is_leap = is_leap_year (year);
  month_days = month_lengths [is_leap][month];

  g_return_val_if_fail ((day >= 1 && day <= month_days), FALSE);
  g_return_val_if_fail ((hour >= 0 && hour <= 23), FALSE);
  g_return_val_if_fail ((minute >= 0 && minute <= 59), FALSE);
  g_return_val_if_fail ((second >= 0 && second <= 59), FALSE);

  return TRUE;
}

/**
 * oobs_time_config_get_unix_time:
 * @config: An #OobsTimeConfig.
 * 
 * Returns the time, measured in seconds, since the "epoch" (1970-01-01T00:00:00Z).
 * 
 * Return Value: The unix time of the system.
 **/
glong
oobs_time_config_get_unix_time (OobsTimeConfig *config)
{
  glong unix_time;

  g_return_val_if_fail (OOBS_IS_TIME_CONFIG (config), 0);

  g_object_get (G_OBJECT (config), "unix-time", &unix_time, NULL);

  return unix_time;
}

/**
 * oobs_time_config_set_unix_time:
 * @config: An #OobsTimeConfig.
 * @unix_time: new Unix time.
 * 
 * This function sets the #config time to be #unix_time. #unix_time
 * is measured in seconds, since the "epoch" (1970-01-01T00:00:00Z).
 **/
void
oobs_time_config_set_unix_time (OobsTimeConfig *config, glong unix_time)
{
  g_return_if_fail (OOBS_IS_TIME_CONFIG (config));

  g_object_set (G_OBJECT (config), "unix-time", unix_time, NULL);
}

static void
real_get_time (OobsTimeConfig *config,
	       gboolean        use_utc,
	       gint           *year,
	       gint           *month,
	       gint           *day,
	       gint           *hour,
	       gint           *minute,
	       gint           *second)
{
  glong unix_time;
  struct tm *tm;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (config));

  unix_time = oobs_time_config_get_unix_time (config);

  if (use_utc)
    tm = gmtime (&unix_time);
  else
    tm = localtime (&unix_time);

  if (year)
    *year = tm->tm_year + 1900;

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
}


/**
 * oobs_time_config_get_time:
 * @config: An #OobsTimeConfig.
 * @year: gint pointer to store the year, or NULL.
 * @month: gint pointer to store the month, or NULL.
 * @day: gint pointer to store the day, or NULL.
 * @hour: gint pointer to store the hour, or NULL.
 * @minute: gint pointer to store the minute, or NULL.
 * @second: gint pointer to store the second, or NULL.
 * 
 * Gets the system time and date in human readable values.
 **/
void
oobs_time_config_get_time (OobsTimeConfig *config,
			   gint           *year,
			   gint           *month,
			   gint           *day,
			   gint           *hour,
			   gint           *minute,
			   gint           *second)
{
  real_get_time (config, FALSE,
		 year, month,  day,
		 hour, minute, second);
}

/**
 * oobs_time_config_get_utc_time:
 * @config: An #OobsTimeConfig.
 * @year: gint pointer to store the year, or NULL.
 * @month: gint pointer to store the month, or NULL.
 * @day: gint pointer to store the day, or NULL.
 * @hour: gint pointer to store the hour, or NULL.
 * @minute: gint pointer to store the minute, or NULL.
 * @second: gint pointer to store the second, or NULL.
 * 
 * Gets the system time and date in human readable values (UTC).
 **/
void
oobs_time_config_get_utc_time (OobsTimeConfig *config,
			       gint           *year,
			       gint           *month,
			       gint           *day,
			       gint           *hour,
			       gint           *minute,
			       gint           *second)
{
  real_get_time (config, TRUE,
		 year, month,  day,
		 hour, minute, second);
}

static glong
get_utc_unix_time (struct tm *tm)
{
  gchar *tz;
  glong unix_time;

  tz = getenv ("TZ");
  setenv ("TZ", "", 1);
  tzset ();

  unix_time = mktime (tm);

  if (tz)
    setenv ("TZ", tz, 1);
  else
    unsetenv ("TZ");

  tzset ();

  return unix_time;
}

static void
real_set_time (OobsTimeConfig *config,
	       gboolean        use_utc,
	       gint            year,
	       gint            month,
	       gint            day,
	       gint            hour,
	       gint            minute,
	       gint            second)
{
  struct tm tm;
  glong unix_time;

  g_return_if_fail (OOBS_IS_TIME_CONFIG (config));
  g_return_if_fail (date_is_sane (year, month, day, hour, minute, second));

  tm.tm_year = year - 1900;
  tm.tm_mon  = month;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min  = minute;
  tm.tm_sec  = second;

  if (use_utc)
    unix_time = get_utc_unix_time (&tm);
  else
    unix_time = mktime (&tm);

  oobs_time_config_set_unix_time (config, unix_time);
}

/**
 * oobs_time_config_set_time:
 * @config: An #OobsTimeConfig.
 * @year: year.
 * @month: month.
 * @day: day.
 * @hour: hour.
 * @minute: minute.
 * @second: second.
 * 
 * Sets the time and date of #config to be the specified in the parameters.
 **/
void
oobs_time_config_set_time (OobsTimeConfig *config,
			   gint            year,
			   gint            month,
			   gint            day,
			   gint            hour,
			   gint            minute,
			   gint            second)
{
  real_set_time (config, FALSE,
		 year, month,  day,
		 hour, minute, second);
}

/**
 * oobs_time_config_set_utc_time:
 * @config: An #OobsTimeConfig.
 * @year: year.
 * @month: month.
 * @day: day.
 * @hour: hour.
 * @minute: minute.
 * @second: second.
 * 
 * Sets the time and date of @config to be the specified in the parameters (assuming they're in UTC).
 **/
void
oobs_time_config_set_utc_time (OobsTimeConfig *config,
			       gint            year,
			       gint            month,
			       gint            day,
			       gint            hour,
			       gint            minute,
			       gint            second)
{
  real_set_time (config, TRUE,
		 year, month,  day,
		 hour, minute, second);
}

/**
 * oobs_time_config_get_timezone:
 * @config: An #OobsTimeConfig.
 * 
 * Returns the timezone set for #config.
 * 
 * Return Value: A pointer to the timezone as a string.
 *               This string must not be freed, modified or stored.
 **/
G_CONST_RETURN gchar*
oobs_time_config_get_timezone (OobsTimeConfig *config)
{
  OobsTimeConfigPrivate *priv;

  g_return_val_if_fail (OOBS_IS_TIME_CONFIG (config), NULL);

  priv = config->_priv;

  return priv->timezone;
}

/**
 * oobs_time_config_set_timezone:
 * @config: An #OobsTimeConfig.
 * @timezone: A new timezone for #config.
 * 
 * Sets the timezone of #config to be #timezone,
 * overwriting the previous one.
 **/
void
oobs_time_config_set_timezone (OobsTimeConfig *config,
			       const gchar    *timezone)
{
  g_return_if_fail (OOBS_IS_TIME_CONFIG (config));

  g_object_set (G_OBJECT (config), "timezone", timezone, NULL);
}
