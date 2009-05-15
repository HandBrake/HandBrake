dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(ghb, 0.1)

AM_INIT_AUTOMAKE([1.7.9 foreign dist-bzip2 dist-zip])
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_STDC
AC_HEADER_STDC

# introduce the optional configure parameter for the path of libXXX.a
AC_ARG_WITH(hb,
	AC_HELP_STRING(
	    [--with-hb=prefix],
	    [try this for the hb-library prefix install directory]
	  ),
	  hb_PATHSET=1,
	  hb_PATHSET=0
)

if test  $hb_PATHSET = 1 ; then
    case ${with_hb} in
    /*)
		CXXFLAGS="$CXXFLAGS -I$with_hb/libhb"
		CFLAGS="$CFLAGS -I$with_hb/libhb"
		LDFLAGS="$LDFLAGS -L$with_hb/libhb -L$with_hb/contrib/lib"
		AC_SUBST(HB_DIR, "$with_hb")
        ;;
    *)
		CXXFLAGS="$CXXFLAGS "'-I$(top_srcdir)/'"$with_hb/libhb"
		CFLAGS="$CFLAGS "'-I$(top_srcdir)/'"$with_hb/libhb"
		LDFLAGS="$LDFLAGS "'-L$(top_srcdir)/'"$with_hb/libhb "'-L$(top_srcdir)/'"$with_hb/contrib/lib"
		AC_SUBST(HB_DIR, '$(top_srcdir)/'"$with_hb")
        ;;
    esac
else
	CXXFLAGS="$CXXFLAGS "'-I$(top_srcdir)/'"../libhb"
	CFLAGS="$CFLAGS "'-I$(top_srcdir)/'"../libhb"
	LDFLAGS="$LDFLAGS "'-L$(top_srcdir)/'"../libhb "'-L$(top_srcdir)/'"../contrib/lib"
	AC_SUBST(HB_DIR, '$(top_srcdir)/'"..")
fi

AC_ARG_ENABLE(gst,
	AS_HELP_STRING([--enable-gst], [enable gstreamer on Win32]),
	w32_gst=yes, w32_gst=no)

# overwrite global variable (used for Makefile generation)
AC_SUBST(GLOBALCXXFLAGS, $CXXFLAGS )
AC_SUBST(GLOBALLDFLAGS, $LDFLAGS )



dnl ***************************************************************************
dnl Internatinalization
dnl ***************************************************************************
GETTEXT_PACKAGE=ghb
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

AM_PROG_LIBTOOL

case $host in
  *-*-mingw*)
    if test "x$w32_gst" = "xyes" ; then
		GHB_PACKAGES="gtk+-2.0 >= 2.8 gthread-2.0 gstreamer-0.10 gstreamer-interfaces-0.10 gstreamer-video-0.10 gstreamer-pbutils-0.10 gio-2.0 hal hal-storage libgtkhtml-3.14"
	else
        GHB_PACKAGES="gtk+-2.0 >= 2.8 gthread-2.0 gio-2.0"
	fi
	mingw_flag=yes
    ;;
  *)
	GHB_PACKAGES="gtk+-2.0 >= 2.8 gthread-2.0 gstreamer-0.10 gstreamer-interfaces-0.10 gstreamer-video-0.10 gstreamer-pbutils-0.10 gio-2.0 hal hal-storage libgtkhtml-3.14 libnotify"
	mingw_flag=no
    ;;
esac

if test "x$w32_gst" = "xyes" -o  "x$mingw_flag" != "xyes" ; then
	CXXFLAGS="$CXXFLAGS -D_ENABLE_GST"
	CFLAGS="$CFLAGS -D_ENABLE_GST"
fi

AM_CONDITIONAL([MINGW], [test "x$mingw_flag" = "xyes"])

PKG_CHECK_MODULES(GHB, [$GHB_PACKAGES])

AC_SUBST(GHB_CFLAGS)
AC_SUBST(GHB_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
