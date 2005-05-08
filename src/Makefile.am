INCLUDES= -Wall $(OOBS_CFLAGS)

oobsincludedir=$(includedir)/liboobs-1.0/

lib_LTLIBRARIES= liboobs-1.la

oobsinclude_HEADERS= 	\
	oobs-session.h		\
	oobs-object.h		\
	oobs-list.h		\
	oobs-shareslist.h	\
	oobs-share.h		\
	oobs-share-smb.h	\
	oobs-share-nfs.h	\
	oobs-shellslist.h	\
	oobs-shell.h		\
	oobs-userslist.h	\
	oobs-user.h

liboobs_1_la_SOURCES = \
	oobs-session.h		oobs-session.c		\
	oobs-object.h		oobs-object.c		\
	oobs-list.h		oobs-list.c		\
	oobs-shareslist.h	oobs-shareslist.c	\
	oobs-share.h		oobs-share.c		\
	oobs-share-smb.h	oobs-share-smb.c	\
	oobs-share-nfs.h	oobs-share-nfs.c	\
	oobs-shellslist.h	oobs-shellslist.c	\
	oobs-shell.h		oobs-shell.c		\
	oobs-userslist.h	oobs-userslist.c	\
	oobs-user.h		oobs-user.c

liboobs_1_la_LIBADD= $(OOBS_LIBS)
