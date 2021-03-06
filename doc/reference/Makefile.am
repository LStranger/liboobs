## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=liboobs

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--deprecated-guards="OOBS_DISABLE_DEPRECATED" --rebuild-sections --rebuild-types

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../oobs

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/oobs/*.h
CFILE_GLOB=$(top_srcdir)/oobs/*.c

# Header files to ignore when scanning.
IGNORE_HFILES=	\
	oobs.h			\
	oobs-defines.h		\
	oobs-list-private.h	\
	oobs-object-private.h	\
	oobs-session-private.h	\
	oobs-user-private.h	\
	oobs-group-private.h	\
	utils.h

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
INCLUDES = -I$(top_builddir) -I$(top_srcdir) $(OOBS_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/oobs/liboobs-1.la $(OOBS_LIBS)

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--output-format=xml

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =	version.xml

# Images to copy into HTML directory.
# HTML_IMAGES =

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

include $(top_srcdir)/gtk-doc.make

# Other files to distribute.
EXTRA_DIST += version.xml.in

CLEANFILES +=   *~				\
		liboobs-scan.*		        \
		liboobs.args		        \
		liboobs.hierarchy		\
		liboobs.interfaces		\
		liboobs.prerequisites	        \
		liboobs.signals		        \
		*.bak				\
		liboobs-decl-list.txt	        \
		liboobs-decl.txt		\
		liboobs-overrides.txt	        \
		liboobs-undeclared.txt	        \
		liboobs-undocumented.txt	\
		*.stamp				\
		version.xml			\
		-rf html xml			\
		$(NULL)
