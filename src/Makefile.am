INCLUDES= -Wall $(OOBS_CFLAGS)

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
	oobs-shellsconfig.h	\
	oobs-shell.h		\
	oobs-usersconfig.h	\
	oobs-user.h		\
	oobs-groupsconfig.h	\
	oobs-group.h		\
	oobs-timeconfig.h	\
	oobs-ntpconfig.h	\
	oobs-ntpserver.h	\
	oobs-hostsconfig.h	\
	oobs-statichost.h	\
	oobs-iface.h		\
	oobs-iface-ethernet.h	\
	oobs-iface-wireless.h	\
	oobs-iface-plip.h	\
	oobs-iface-isdn.h	\
	oobs-iface-modem.h	\
	oobs-servicesconfig.h	\
	oobs-service.h

liboobs_1_la_SOURCES = \
	md5-crypt.c	\
	md5.h			md5.c			\
	oobs-session.h		oobs-session.c		\
	oobs-object.h		oobs-object.c		\
	oobs-list.h		oobs-list.c		\
	oobs-shell.h		oobs-shell.c		\
	oobs-shellsconfig.h	oobs-shellsconfig.c	\
	oobs-user.h		oobs-user.c		\
	oobs-usersconfig.h	oobs-usersconfig.c	\
	oobs-group.h		oobs-group.c		\
	oobs-groupsconfig.h	oobs-groupsconfig.c	\
	oobs-timeconfig.h	oobs-timeconfig.c	\
	oobs-ntpserver.h	oobs-ntpserver.c	\
	oobs-ntpconfig.h	oobs-ntpconfig.c	\
	oobs-share.h            oobs-share.c            \
	oobs-smbconfig.h        oobs-smbconfig.c        \
	oobs-share-smb.h        oobs-share-smb.c        \
	oobs-nfsconfig.h        oobs-nfsconfig.c        \
	oobs-share-nfs.h        oobs-share-nfs.c	\
	oobs-statichost.h	oobs-statichost.c	\
	oobs-hostsconfig.h      oobs-hostsconfig.c	\
	oobs-iface.h		oobs-iface.c		\
	oobs-iface-ethernet.h	oobs-iface-ethernet.c	\
	oobs-iface-wireless.h	oobs-iface-wireless.c	\
	oobs-iface-plip.h	oobs-iface-plip.c	\
	oobs-iface-isdn.h	oobs-iface-isdn.c	\
	oobs-iface-modem.h	oobs-iface-modem.c	\
	oobs-service.h		oobs-service.c		\
	oobs-servicesconfig.h	oobs-servicesconfig.c

liboobs_1_la_LIBADD= $(OOBS_LIBS)
