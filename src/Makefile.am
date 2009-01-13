INCLUDES = \
	$(EXTRA_GNOME_CFLAGS)                           \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"        \
	-DDATA_DIR=\""$(pkgdatadir)"\"                  \
	-DPIX_DIR=\""$(pkgdatadir)/pixmaps"\"		\
	-DICON_DIR=\""$(datadir)/pixmaps"\"

bin_PROGRAMS = aldegonde

aldegonde_SOURCES = \
	disc.c \
	main.c \
	properties.c \
	timer.c \
	video.c \
	window.c

aldegonde_CFLAGS = \
	$(EXTRA_CFLAGS) $(GLIB_CFLAGS) $(GST_CFLAGS) $(GNOME_CFLAGS) \
	-I$(top_srcdir)/pixmaps

aldegonde_LDFLAGS = \
	$(GLIB_LIBS) $(GST_LIBS) $(GNOME_LIBS)

noinst_HEADERS = \
	disc.h \
	properties.h \
	stock.h \
	timer.h \
	video.h \
	window.h
