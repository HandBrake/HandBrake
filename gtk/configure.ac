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

if test x"$CC_FOR_BUILD" = x; then
	if test x"$cross_compiling" = x"yes"; then
		AC_CHECK_PROGS(CC_FOR_BUILD, gcc, cc)
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

AC_ARG_ENABLE(fdk-aac,
	AS_HELP_STRING([--enable-fdk-aac], [enable fdk aac encoder]),
	use_fdk_aac=yes, use_fdk_aac=no)

AC_ARG_ENABLE(faac,
	AS_HELP_STRING([--enable-faac], [enable faac encoder]),
	use_faac=yes, use_faac=no)

AC_ARG_ENABLE(mp4v2,
	AS_HELP_STRING([--enable-mp4v2], [enable mp4v2 muxer]),
	use_mp4v2=yes, use_mp4v2=no)

AC_ARG_ENABLE(libmkv,
	AS_HELP_STRING([--enable-libmkv], [enable libmkv muxer]),
	use_libmkv=yes, use_libmkv=no)

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

case $host in
  *-*-mingw*)
    GHB_PACKAGES="gthread-2.0 gio-2.0"
	mingw_flag=yes
    ;;
  *)
    case "$host_os" in
	linux*)
	  pkg_gudev="gudev-1.0"
	  ;;
    esac
    GHB_PACKAGES="gthread-2.0 gio-2.0 libnotify dbus-glib-1 $pkg_gudev"
	mingw_flag=no
    ;;
esac

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(Gtk3, [gtk+-3.0], use_gtk3=yes, use_gtk3=no)
if test "x$use_gtk3" = "xyes" ; then
    GHB_PACKAGES="gtk+-3.0 $GHB_PACKAGES"
else
    GHB_PACKAGES="gtk+-2.0 >= 2.10 $GHB_PACKAGES"
fi

GST1_MODULES="gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 gstreamer-pbutils-1.0"
GST0_MODULES="gstreamer-0.10 gstreamer-interfaces-0.10 gstreamer-video-0.10 gstreamer-audio-0.10 gstreamer-pbutils-0.10"

if test "x$gst_disable" = "xno" ; then
    PKG_CHECK_MODULES(GStreamer1, [$GST1_MODULES], use_gst1=yes, use_gst1=no)
    if test "x$use_gst1" = "xyes" ; then
        GHB_PACKAGES="$GHB_PACKAGES $GST1_MODULES"
    else
        PKG_CHECK_MODULES(GStreamer0, [$GST0_MODULES], use_gst0=yes, use_gst0=no)
        if test "x$use_gst0" = "xyes" ; then
            GHB_PACKAGES="$GHB_PACKAGES $GST0_MODULES"
        else
            gst_disable="yes"
        fi
    fi
fi

if test "x$gst_disable" = "xno" ; then
	CXXFLAGS="$CXXFLAGS -D_ENABLE_GST"
	CFLAGS="$CFLAGS -D_ENABLE_GST"
fi

if test "x$update_checks" = "xyes" ; then
    if test "x$use_gtk3" = "xyes" ; then
        PKG_CHECK_MODULES(WebKitGtk3, webkitgtk-3.0, use_webkitgtk3=yes, use_webkitgtk3=no)
        if test "x$use_webkitgtk3" = "xyes" ; then
            GHB_PACKAGES="$GHB_PACKAGES webkitgtk-3.0"
        else
            update_checks="no"
	        CFLAGS="$CFLAGS -D_NO_UPDATE_CHECK"
        fi
    else
        PKG_CHECK_MODULES(OldWebKitGtk1, WebKitGtk, old_webkit=yes, old_webkit=no)
        if test "x$old_webkit" = "xyes" ; then
            if test "x$mingw_flag" = "xno" ; then
                GHB_PACKAGES="$GHB_PACKAGES WebKitGtk"
                CXXFLAGS="$CXXFLAGS -D_OLD_WEBKIT"
                CFLAGS="$CFLAGS -D_OLD_WEBKIT"
            else
                update_checks="no"
                CFLAGS="$CFLAGS -D_NO_UPDATE_CHECK"
            fi
        else
            PKG_CHECK_MODULES(WebKitGtk1, webkit-1.0, use_webkit1=yes, use_webkit1=no)
            if test "x$use_webkit1" = "xyes" ; then
                GHB_PACKAGES="$GHB_PACKAGES webkit-1.0"
            else
                update_checks="no"
                CFLAGS="$CFLAGS -D_NO_UPDATE_CHECK"
            fi
        fi
    fi
else
	CFLAGS="$CFLAGS -D_NO_UPDATE_CHECK"
fi

if test "x$use_gtk3" = "xyes" ; then
    PKG_CHECK_MODULES(libappindicator3, appindicator3-0.1, have_appind=yes, have_appind=no)
    if test "x$have_appind" = "xyes" ; then
        GHB_PACKAGES="$GHB_PACKAGES appindicator3-0.1"
        CXXFLAGS="$CXXFLAGS -D_USE_APP_IND"
        CFLAGS="$CFLAGS -D_USE_APP_IND"
    fi
else
    PKG_CHECK_MODULES(libappindicator, appindicator-0.1, have_appind=yes, have_appind=no)
    if test "x$have_appind" = "xyes" ; then
        GHB_PACKAGES="$GHB_PACKAGES appindicator-0.1"
        CXXFLAGS="$CXXFLAGS -D_USE_APP_IND"
        CFLAGS="$CFLAGS -D_USE_APP_IND"
    fi
fi

AM_CONDITIONAL([MINGW], [test "x$mingw_flag" = "xyes"])

PKG_CHECK_MODULES(GHB, [$GHB_PACKAGES])

GHB_CFLAGS="$HBINC $GHB_CFLAGS"

AC_PATH_PROG(BUILD_PKG_CONFIG, pkg-config, no)
if test x"$BUILD_PKG_CONFIG" = x"no"; then
	AC_MSG_ERROR([You need to install pkg-config])
fi

GHB_TOOLS_CFLAGS=`$BUILD_PKG_CONFIG --cflags glib-2.0 gdk-pixbuf-2.0`
GHB_TOOLS_LIBS=`$BUILD_PKG_CONFIG --libs glib-2.0 gdk-pixbuf-2.0`

case $host in
  *-*-mingw*)
    if test "x$use_libdl" = "xyes" ; then
		GHB_LIBS="$GHB_LIBS -ldl"
	fi
    ;;
esac

HB_LIBS="-lhb -la52 -lavresample -lavformat -lavcodec -lavutil -ldvdnav -ldvdread -lmp3lame -lmpeg2 -lvorbis -lvorbisenc -logg -lsamplerate -lx264 -lswscale -ltheoraenc -ltheoradec -lz -lbz2 -lpthread -lbluray -lass -lfontconfig -lfreetype -lxml2"

if test "x$use_fdk_aac" = "xyes" ; then
    HB_LIBS+=" -lfdk-aac"
fi

if test "x$use_faac" = "xyes" ; then
    HB_LIBS+=" -lfaac"
fi

if test "x$use_mp4v2" = "xyes" ; then
    HB_LIBS+=" -lmp4v2"
fi

if test "x$use_libmkv" = "xyes" ; then
    HB_LIBS+=" -lmkv"
fi

AC_SUBST(HB_LIBS)
AC_SUBST(GHB_TOOLS_CFLAGS)
AC_SUBST(GHB_TOOLS_LIBS)
AC_SUBST(GHB_CFLAGS)
AC_SUBST(GHB_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
