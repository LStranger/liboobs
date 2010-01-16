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

oobs_public_headers = 	\
	oobs.h			\
	oobs-session.h		\
	oobs-object.h		\
	oobs-list.h		\
	oobs-error.h		\
	oobs-defines.h		\
	oobs-enum-types.h	\
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
	oobs-selfconfig.h	\
	oobs-statichost.h	\
	oobs-ifacesconfig.h	\
	oobs-iface.h		\
	oobs-iface-ethernet.h	\
	oobs-iface-wireless.h	\
	oobs-iface-irlan.h	\
	oobs-iface-plip.h	\
	oobs-iface-ppp.h	\
	oobs-servicesconfig.h	\
	oobs-service.h		\
	oobs-result.h

oobsinclude_HEADERS = $(oobs_public_headers)

oobs_private_headers = 	\
	iface-state-monitor.h		\
	oobs-defines.h			\
	oobs-list-private.h		\
	oobs-object-private.h	\
	oobs-session-private.h	\
	oobs-user-private.h	\
	oobs-group-private.h	\
	utils.h

oobs_built_sources = \
       oobs-enum-types.h \
       oobs-enum-types.c

liboobs_1_la_SOURCES = \
	utils.c				\
	oobs-error.c			\
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
	oobs-selfconfig.c		\
	oobs-share-nfs.c		\
	oobs-statichost.c		\
	oobs-hostsconfig.c		\
	oobs-iface.c			\
	oobs-iface-ethernet.c	\
	oobs-iface-wireless.c	\
	oobs-iface-irlan.c		\
	oobs-iface-plip.c		\
	oobs-iface-ppp.c		\
	oobs-ifacesconfig.c		\
	oobs-service.c			\
	oobs-servicesconfig.c		\
	$(oobs_built_sources)

if HAVE_RTNETLINK
liboobs_1_la_SOURCES += iface-state-monitor-linux.c
else
liboobs_1_la_SOURCES += iface-state-monitor-dummy.c
endif

liboobs_1_la_LIBADD= $(OOBS_LIBS) $(HAL_LIBS)
liboobs_1_la_LDFLAGS= $(libtool_opts)

EXTRA_DIST= $(oobs_private_headers) \
	oobs-enum-types.c.in	\
	oobs-enum-types.h.in

BUILT_SOURCES = $(oobs_built_sources)
OOBS_STAMP_FILES = stamp-oobs-enum-types.h
CLEANFILES = $(OOBS_STAMP_FILES)	\
	oobs-enum-types.c		\
	oobs-enum-types.h


oobs-enum-types.h: stamp-oobs-enum-types.h Makefile
	@true
stamp-oobs-enum-types.h: $(oobs_public_headers) oobs-enum-types.h.in
	$(AM_V_GEN) ( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	    --template $(srcdir)/oobs-enum-types.h.in \
	  $(oobs_public_headers) ) >> xgen-teth && \
	(cmp -s xgen-teth oobs-enum-types.h || cp xgen-teth oobs-enum-types.h) && \
	rm -f xgen-teth && \
	echo timestamp > $(@F)

oobs-enum-types.c: stamp-oobs-enum-types.h oobs-enum-types.c.in
	  $(AM_V_GEN) ( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	    --template $(srcdir)/oobs-enum-types.c.in \
	  $(oobs_public_headers) ) >> xgen-tetc && \
	cp xgen-tetc oobs-enum-types.c && \
	rm -f xgen-tetc
