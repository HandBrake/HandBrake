dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(ghb, 0.1)

AM_INIT_AUTOMAKE([1.7.9 foreign dist-bzip2 dist-zip])
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_PROG_CC(gcc clang)
AC_ISC_POSIX
AC_PROG_CXX(g++ clang++)
AM_PROG_CC_STDC
AC_HEADER_STDC

if test x"$CC_FOR_BUILD" = x; then
	if test x"$cross_compiling" = x"yes"; then
		AC_CHECK_PROGS(CC_FOR_BUILD, gcc clang, cc)
	else
		CC_FOR_BUILD="$CC"
	fi
fi

AC_SUBST(CC_FOR_BUILD)

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
		HBINC="-I$with_hb/libhb -I$with_hb/contrib/include"
		LDFLAGS="$LDFLAGS -L$with_hb/libhb -L$with_hb/contrib/lib"
		AC_SUBST(HB_DIR, "$with_hb")
        ;;
    *)
		HBINC='-I$(top_srcdir)/'"$with_hb/libhb "'-I$(top_srcdir)/'"$with_hb/contrib/include"
		LDFLAGS="$LDFLAGS "'-L$(top_srcdir)/'"$with_hb/libhb "'-L$(top_srcdir)/'"$with_hb/contrib/lib"
		AC_SUBST(HB_DIR, '$(top_srcdir)/'"$with_hb")
        ;;
    esac
else
	HBINC='-I$(top_srcdir)/'"../libhb "'-I$(top_srcdir)/'"../contrib/include"
	LDFLAGS="$LDFLAGS "'-L$(top_srcdir)/'"../libhb "'-L$(top_srcdir)/'"../contrib/lib"
	AC_SUBST(HB_DIR, '$(top_srcdir)/'"..")
fi

AC_ARG_ENABLE(dl,
	AS_HELP_STRING([--enable-dl], [enable libdl]),
	use_libdl=yes, use_libdl=no)

AC_ARG_ENABLE(pthread,
	AS_HELP_STRING([--enable-pthread], [enable libpthread]),
	use_libpthread=yes, use_libpthread=no)

AC_ARG_ENABLE(fdk-aac,
	AS_HELP_STRING([--enable-fdk-aac], [enable fdk aac encoder]),
	use_fdk_aac=yes, use_fdk_aac=no)

AC_ARG_ENABLE(x265,
	AS_HELP_STRING([--enable-x265], [enable x265 encoder]),
	use_x265=yes, use_x265=no)

AC_ARG_ENABLE(qsv,
	AS_HELP_STRING([--enable-qsv], [enable QSV encoder]),
	use_qsv=yes, use_qsv=no)

AC_ARG_ENABLE(gst,
	AS_HELP_STRING([--disable-gst], [disable gstreamer (live preview)]),
	gst_disable=yes, gst_disable=no)

AC_ARG_ENABLE(update-checks,
	AS_HELP_STRING([--disable-update-checks], [disable update checks]),
	update_checks=no, update_checks=yes)

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

AC_SYS_LARGEFILE

case $host in
  *-*-mingw*)
    GHB_PACKAGES="gthread-2.0 gio-2.0 gmodule-2.0"
	mingw_flag=yes
    ;;
  *)
    case "$host_os" in
	linux*)
	  pkg_gudev="gudev-1.0"
	  ;;
    esac
    GHB_PACKAGES="gthread-2.0 gio-2.0 libnotify dbus-glib-1 gmodule-2.0 $pkg_gudev"
	mingw_flag=no
    ;;
esac

PKG_PROG_PKG_CONFIG
GHB_PACKAGES="gtk+-3.0 $GHB_PACKAGES"

GST1_MODULES="gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 gstreamer-pbutils-1.0"

if test "x$gst_disable" = "xno" ; then
    PKG_CHECK_MODULES(GStreamer1, [$GST1_MODULES], use_gst1=yes, use_gst1=no)
    if test "x$use_gst1" = "xyes" ; then
        GHB_PACKAGES="$GHB_PACKAGES $GST1_MODULES"
    else
        gst_disable="yes"
    fi
fi

if test "x$gst_disable" = "xno" ; then
	CXXFLAGS="$CXXFLAGS -D_ENABLE_GST"
	CFLAGS="$CFLAGS -D_ENABLE_GST"
fi

if test "x$update_checks" = "xyes" ; then
    PKG_CHECK_MODULES(WebKitGtk3, webkitgtk-3.0, use_webkitgtk3=yes, use_webkitgtk3=no)
    if test "x$use_webkitgtk3" = "xyes" ; then
        GHB_PACKAGES="$GHB_PACKAGES webkitgtk-3.0"
    else
        update_checks="no"
        CFLAGS="$CFLAGS -D_NO_UPDATE_CHECK"
    fi
else
	CFLAGS="$CFLAGS -D_NO_UPDATE_CHECK"
fi

PKG_CHECK_MODULES([GHB_GTK_3_14], [gtk+-3.0 >= 3.14], [HAVE_GTK_314=1], [HAVE_GTK_314=0])
PKG_CHECK_MODULES([GHB_GTK_3_12], [gtk+-3.0 >= 3.12], [HAVE_GTK_312=1], [HAVE_GTK_312=0])
AM_CONDITIONAL([GHB_GTK_3_14], [test "$HAVE_GTK_314" -eq 1])
AM_CONDITIONAL([GHB_GTK_3_12], [test "$HAVE_GTK_312" -eq 1])

AM_CONDITIONAL([MINGW], [test "x$mingw_flag" = "xyes"])

PKG_CHECK_MODULES(GHB, [$GHB_PACKAGES])

GHB_CFLAGS="$HBINC $GHB_CFLAGS"

HB_LIBS="-lhandbrake -lavresample -lavformat -lavcodec -lavutil -ldvdnav -ldvdread -lmp3lame -lvorbis -lvorbisenc -logg -lsamplerate -lx264 -lswscale -ltheoraenc -ltheoradec -lvpx -lz -lbz2 -lbluray -lass -lfontconfig -lfreetype -lxml2 -ljansson"

case $host in
  *-*-mingw*)
    if test "x$use_libdl" = "xyes" ; then
		HB_LIBS="$HB_LIBS -ldl"
	fi
    if test "x$use_libpthread" = "xyes" ; then
		HB_LIBS="$HB_LIBS -lpthread"
    else
        HB_LIBS="$HB_LIBS -lpthreadGC2"
	fi
    HB_LIBS="$HB_LIBS -lregex -luuid"
    ;;
  *)
    HB_LIBS="$HB_LIBS -ldl -lpthread"
    ;;
esac

if test "x$use_fdk_aac" = "xyes" ; then
    HB_LIBS="$HB_LIBS -lfdk-aac"
fi

if test "x$use_x265" = "xyes" ; then
    HB_LIBS="$HB_LIBS -lx265"
fi

if test "x$use_qsv" = "xyes" ; then
    HB_LIBS="$HB_LIBS -lmfx"
fi

AC_SUBST(HB_LIBS)
AC_SUBST(GHB_CFLAGS)
AC_SUBST(GHB_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
