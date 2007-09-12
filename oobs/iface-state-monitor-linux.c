/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2007 Carlos Garnacho
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

#include <glib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "iface-state-monitor.h"

#define BUF_SIZE 4096

typedef struct MonitorData MonitorData;

struct MonitorData {
  OobsIfacesConfig      *config;
  IfaceStateMonitorFunc  func;
  GIOChannel            *channel;
  guint                  channel_source_id;
};

static gpointer
get_message_attribute (MonitorData     *data,
		       struct nlmsghdr *msg_netlink,
		       size_t           size,
		       gushort          type)
{
  struct rtattr *rt_attr;

  rt_attr = NLMSG_DATA (msg_netlink) + NLMSG_ALIGN (sizeof (struct ifinfomsg));

  while (RTA_OK (rt_attr, size))
    {
      if (rt_attr->rta_type == type)
	return RTA_DATA (rt_attr);

      rt_attr = RTA_NEXT (rt_attr, size);
    }

  return NULL;
}

static void
read_message (MonitorData     *data,
	      struct nlmsghdr *msg_netlink,
	      size_t           size)
{
  struct ifinfomsg *if_info;
  const gchar *iface_name;
  gboolean iface_active;

  if_info = NLMSG_DATA (msg_netlink);
  iface_name = get_message_attribute (data, msg_netlink, size, IFLA_IFNAME);
  iface_active = if_info->ifi_flags & IFF_UP;

  (data->func) (data->config, iface_name, iface_active);
}

static gboolean
monitor_data_channel_watch (GIOChannel       *channel,
			    GIOCondition      condition,
			    MonitorData      *data)
{
  gint fd;
  ssize_t size;
  char buf[BUF_SIZE];
  struct msghdr msg = { 0, };
  struct iovec iovec = { 0, };
  struct nlmsghdr *msg_netlink;

  fd = g_io_channel_unix_get_fd (channel);

  iovec.iov_base = buf;
  iovec.iov_len = BUF_SIZE;

  /* setup scatter/gather array */
  msg.msg_iov = (void *) &iovec;
  msg.msg_iovlen = 1;

  size = recvmsg (fd, &msg, 0);

  if (size < 0)
    {
      g_warning ("Could not get netlink message\n");
      return TRUE;
    }

  /* point to first message */
  msg_netlink = (struct nlmsghdr *) &buf;

  while (NLMSG_OK (msg_netlink, size))
    {
      read_message (data, msg_netlink, size);
      msg_netlink = NLMSG_NEXT (msg_netlink, size);
    }

  return TRUE;
}

static void
monitor_data_free (MonitorData *data)
{
  if (data->channel_source_id)
    {
      g_source_remove (data->channel_source_id);
      data->channel_source_id = 0;
    }

  g_io_channel_shutdown (data->channel, FALSE, NULL);
  g_io_channel_unref (data->channel);
}

static MonitorData*
init_monitor_data (OobsIfacesConfig      *config,
		   IfaceStateMonitorFunc  func)
{
  MonitorData *data;
  struct sockaddr_nl addr = { 0, };
  gint fd;

  fd = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

  addr.nl_family = AF_NETLINK;
  addr.nl_pad = 0;
  addr.nl_pid = getpid ();
  addr.nl_groups = RTMGRP_LINK;

  if (bind (fd, (struct sockaddr *) &addr, sizeof (addr)) != 0)
    return NULL;

  data = g_new0 (MonitorData, 1);
  data->func = func;
  data->config = config;
  data->channel = g_io_channel_unix_new (fd);
  data->channel_source_id = g_io_add_watch (data->channel,
					    G_IO_IN | G_IO_ERR | G_IO_HUP,
					    (GIOFunc) monitor_data_channel_watch,
					    data);
  return data;
}

void
iface_state_monitor_init (OobsIfacesConfig      *config,
			  IfaceStateMonitorFunc  func)
{
  static GQuark quark = 0;
  MonitorData *data;

  g_return_if_fail (OOBS_IS_IFACES_CONFIG (config));
  g_return_if_fail (func != NULL);

  if (G_UNLIKELY (!quark))
    quark = g_quark_from_static_string ("iface-state-monitor-data");

  data = init_monitor_data (config, func);

  g_object_set_qdata_full (G_OBJECT (config), quark,
			   data, (GDestroyNotify) monitor_data_free);
}
