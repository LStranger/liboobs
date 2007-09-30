INCLUDES = \
	-Wall \
	-DG_LOG_DOMAIN=\"Liboobs\" \
	$(OOBS_CFLAGS) \
	$(HAL_CFLAGS)

libtool_opts = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-export-dynamic -export-symbols-regex "^oobs_.*"

oobsincludedir=$(includedir)/liboobs-1.0/oobs/

lib_LTLIBRARIES= liboobs-1.la

oobsinclude_HEADERS= 	\
	oobs.h			\
	oobs-session.h		\
	oobs-object.h		\
	oobs-list.h		\
	oobs-share.h		\
	oobs-smbconfig.h	\
	oobs-share-smb.h	\
	oobs-nfsconfig.h	\
	oobs-share-nfs.h	\
	oobs-usersconfig.h	\
	oobs-user.h		\
	oobs-groupsconfig.h	\
	oobs-group.h		\
	oobs-timeconfig.h	\
	oobs-ntpconfig.h	\
	oobs-ntpserver.h	\
	oobs-hostsconfig.h	\
	oobs-statichost.h	\
	oobs-ifacesconfig.h	\
	oobs-iface.h		\
	oobs-iface-ethernet.h	\
	oobs-iface-wireless.h	\
	oobs-iface-irlan.h	\
	oobs-iface-plip.h	\
	oobs-iface-isdn.h	\
	oobs-iface-modem.h	\
	oobs-servicesconfig.h	\
	oobs-service.h		\
	oobs-result.h

oobs_private_headers = 	\
	iface-state-monitor.h		\
	oobs-defines.h			\
	oobs-groupsconfig-private.h	\
	oobs-list-private.h		\
	oobs-object-private.h	\
	oobs-session-private.h	\
	oobs-usersconfig-private.h	\
	utils.h

liboobs_1_la_SOURCES = \
	utils.c				\
	oobs-session.c			\
	oobs-object.c			\
	oobs-list.c			\
	oobs-user.c			\
	oobs-usersconfig.c		\
	oobs-group.c			\
	oobs-groupsconfig.c		\
	oobs-timeconfig.c		\
	oobs-ntpserver.c		\
	oobs-ntpconfig.c		\
	oobs-share.c			\
	oobs-smbconfig.c		\
	oobs-share-smb.c		\
	oobs-nfsconfig.c		\
	oobs-share-nfs.c		\
	oobs-statichost.c		\
	oobs-hostsconfig.c		\
	oobs-iface.c			\
	oobs-iface-ethernet.c	\
	oobs-iface-wireless.c	\
	oobs-iface-irlan.c		\
	oobs-iface-plip.c		\
	oobs-iface-isdn.c		\
	oobs-iface-modem.c		\
	oobs-ifacesconfig.c		\
	oobs-service.c			\
	oobs-servicesconfig.c

if HAVE_RTNETLINK
liboobs_1_la_SOURCES += iface-state-monitor-linux.c
else
liboobs_1_la_SOURCES += iface-state-monitor-dummy.c
endif

liboobs_1_la_LIBADD= $(OOBS_LIBS) $(HAL_LIBS)
liboobs_1_la_LDFLAGS= $(libtool_opts)

EXTRA_DIST= $(oobs_private_headers)

