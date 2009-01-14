dnl for this code, plz blame http://developer.gnome.org/dotplan/porting/ar01s05.html
AC_INIT(Aldegonde, 0.0.1)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE(aldegonde, ${PACKAGE_VERSION})
PACKAGE_TARNAME=$PACKAGE

AM_CONFIG_HEADER(config.h)

ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AC_PROG_CC
AC_STDC_HEADERS

AM_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL

dnl modify pkg-config path
AC_ARG_WITH(pkg-config-path, 
   AC_HELP_STRING([--with-pkg-config-path],[colon-separated list of pkg-config(1) dirs]),
   [export PKG_CONFIG_PATH=${withval}])

dnl Check for glib2
PKG_CHECK_MODULES(GLIB2, glib-2.0 gobject-2.0 gthread-2.0 gmodule-2.0, ,
  AC_MSG_ERROR(You don't have glib-2.0 (1.3) installed))
GLIB_LIBS=$GLIB2_LIBS
GLIB_CFLAGS=$GLIB2_CFLAGS
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

dnl check for gstreamer
GST_VERSION_MAJOR=0
GST_VERSION_MINOR=10
GST_VERSION_RELEASE=0
GSTREAMER_REQ=$GST_VERSION_MAJOR.$GST_VERSION_MINOR.$GST_VERSION_RELEASE
GST_MAJORMINOR=$GST_VERSION_MAJOR.$GST_VERSION_MINOR
PKG_CHECK_MODULES(GST, gstreamer-$GST_MAJORMINOR >= $GSTREAMER_REQ
                       gstreamer-interfaces-$GST_MAJORMINOR >= $GSTREAMER_REQ)
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_LIBS)

AC_SUBST(GST_VERSION_MAJOR)
AC_SUBST(GST_VERSION_MINOR)
AC_SUBST(GST_MAJORMINOR)

dnl check for the right GNOME
dnl get the genmarshal command for the *.list files
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)
dnl check modules
PKG_CHECK_MODULES(GNOME, libgnome-2.0 libgnomeui-2.0 gnome-vfs-2.0
                         libbonobo-2.0 gconf-2.0)
AC_SUBST(GNOME_LIBS)
AC_SUBST(GNOME_CFLAGS)

dnl be clean and use pure Gtk+-2.0
EXTRA_CFLAGS="-Wall -Werror"
# -DGTK_DISABLE_DEPRECATED=1 -DG_DISABLE_DEPRECATED=1 -DGDK_DISABLE_DEPRECATED=1 -DGDK_PIXBUF_DISABLE_DEPRECATED=1 -DGNOME_DISABLE_DEPRECATED=1 -DGST_DISABLE_DEPRECATED=1"
AC_SUBST(EXTRA_CFLAGS)

AC_OUTPUT([
Makefile
docs/Makefile
pixmaps/Makefile
src/Makefile
aldegonde.spec
])

echo
echo "Now type 'make' to compile ${PACKAGE_NAME}-${PACKAGE_VERSION}."
