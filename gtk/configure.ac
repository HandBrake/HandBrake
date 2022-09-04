dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([ghb], [0.1])
AM_INIT_AUTOMAKE([-Wall 1.7.9 foreign dist-bzip2 dist-zip])

AM_MAINTAINER_MODE

AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC(gcc clang)
AC_ISC_POSIX
AC_PROG_CXX(g++ clang++)
AM_PROG_CC_STDC
AC_HEADER_STDC

AM_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([
po/Makefile.in
Makefile
src/Makefile
])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

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

if test $hb_PATHSET = 1 ; then
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

AC_ARG_ENABLE(flatpak,
	AS_HELP_STRING([--enable-flatpak], [enable flatpak packaging]),
	use_flatpak=yes, use_flatpak=no)

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

AC_ARG_ENABLE(numa,
	AS_HELP_STRING([--enable-numa], [enable x265 NUMA support]),
	use_numa=yes, use_numa=no)

AC_ARG_ENABLE(qsv,
	AS_HELP_STRING([--enable-qsv], [enable QSV encoder]),
	use_qsv=yes, use_qsv=no)

AC_ARG_ENABLE(mf,
	AS_HELP_STRING([--enable-mf], [enable MediaFoundation encoder]),
	use_mf=yes, use_mf=no)

AC_ARG_ENABLE(gst,
	AS_HELP_STRING([--disable-gst], [disable gstreamer (live preview)]),
	gst_disable=yes, gst_disable=no)

AC_ARG_ENABLE(update-checks,
	AS_HELP_STRING([--disable-update-checks], [disable update checks]),
	update_checks=no, update_checks=yes)

AC_ARG_ENABLE(gtk4,
	AS_HELP_STRING([--enable-gtk4], [enable building with gtk4]),
	use_gtk4=yes, use_gtk4=no)

# overwrite global variable (used for Makefile generation)
AC_SUBST(GLOBALCXXFLAGS, $CXXFLAGS )
AC_SUBST(GLOBALLDFLAGS, $LDFLAGS )



dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
GETTEXT_PACKAGE=ghb
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

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
				;;
		esac
		GHB_PACKAGES="gthread-2.0 gio-2.0 gmodule-2.0"
		mingw_flag=no
		;;
esac

PKG_PROG_PKG_CONFIG

HAVE_GTK_400=0
HAVE_GTK_316=0
if test "x$use_gtk4" = "xyes" ; then
	PKG_CHECK_MODULES([gtk], [gtk4 >= 3.92], [HAVE_GTK_400=1], [HAVE_GTK_400=0])
else
	PKG_CHECK_MODULES([gtk], [gtk+-3.0 >= 3.16], [HAVE_GTK_316=1], [HAVE_GTK_316=0])
fi

if test "$HAVE_GTK_316" -eq 0 -a "$HAVE_GTK_400" -eq 0 ; then
	AC_MSG_ERROR("GTK 3.16 or above is required)
fi
HB_LIBS="$HB_LIBS $gtk_LIBS"
HB_CPPFLAGS="$HB_CPPFLAGS $gtk_CFLAGS"

PKG_CHECK_MODULES([GHB], [$GHB_PACKAGES])
HB_LIBS="$HB_LIBS $GHB_LIBS"
HB_CPPFLAGS="$HB_CPPFLAGS $GHB_CFLAGS"


pkg_gudev="gudev-1.0"
PKG_CHECK_MODULES([gudev], [$pkg_gudev], have_gudev=yes, have_gudev=no)
if test "x$have_gudev" = "xyes" ; then
	HB_LIBS="$HB_LIBS $gudev_LIBS"
	HB_CPPFLAGS="$HB_CPPFLAGS $gudev_CFLAGS"

	CXXFLAGS="$CXXFLAGS -D_HAVE_GUDEV"
	CFLAGS="$CFLAGS -D_HAVE_GUDEV"
fi

GST1_MODULES="gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 gstreamer-pbutils-1.0"
if test "x$gst_disable" = "xno" ; then
	PKG_CHECK_MODULES([gstreamer1], [$GST1_MODULES], use_gst1=yes, use_gst1=no)
	if test "x$use_gst1" = "xyes" ; then
		HB_LIBS="$HB_LIBS $gstreamer1_LIBS"
		HB_CPPFLAGS="$HB_CPPFLAGS $gstreamer1_CFLAGS"
	else
		gst_disable="yes"
	fi
fi

if test "x$gst_disable" = "xno" ; then
	CXXFLAGS="$CXXFLAGS -D_ENABLE_GST"
	CFLAGS="$CFLAGS -D_ENABLE_GST"
fi

if test "x$update_checks" = "xyes" ; then
	PKG_CHECK_MODULES([webkitgtk], webkitgtk-3.0, use_webkitgtk=yes, use_webkitgtk=no)
	if test "x$use_webkitgtk" = "xyes" ; then
		HB_LIBS="$HB_LIBS $webkitgtk_LIBS"
		HB_CPPFLAGS="$HB_CPPFLAGS $webkitgtk_CFLAGS"
	else
		update_checks="no"
		CFLAGS="$CFLAGS -D_NO_UPDATE_CHECK"
	fi
else
	CFLAGS="$CFLAGS -D_NO_UPDATE_CHECK"
fi

AM_CONDITIONAL([GHB_GTK_4_00], [test "$HAVE_GTK_400" -eq 1])
AM_CONDITIONAL([GHB_GTK_3_16], [test "$HAVE_GTK_316" -eq 1])

AM_CONDITIONAL([MINGW], [test "x$mingw_flag" = "xyes"])

HB_LIBS="$HB_LIBS -lhandbrake -lavformat -lavfilter -lavcodec -lavutil -ldav1d -lswresample -lpostproc -ldvdnav -ldvdread -lmp3lame -lvorbis -lvorbisenc -logg -lswscale -ltheoraenc -ltheoradec -lvpx -lz -lbz2 -lbluray -lass -lfontconfig -lfreetype -lxml2 -ljansson -lopus -lspeex -lturbojpeg -llzma -lzimg -lSvtAv1Enc"
HB_CPPFLAGS="$HB_CPPFLAGS $HBINC"

PKG_CHECK_MODULES([x264], [x264], sys_x264=yes, sys_x264=no)
if test "x$sys_x264" = "xyes" ; then
	HB_LIBS="$HB_LIBS $x264_LIBS"
	HB_CPPFLAGS="$HB_CPPFLAGS $x264_CFLAGS"
else
	# Assume it's built as a libhandbrake contrib package
	HB_LIBS="$HB_LIBS -lx264"
fi

if test "x$use_fdk_aac" = "xyes" ; then
	HB_LIBS="$HB_LIBS -lfdk-aac"
fi

if test "x$use_x265" = "xyes" ; then
	HB_LIBS="$HB_LIBS -lx265"
fi

if test "x$use_qsv" = "xyes" ; then
	HB_LIBS="$HB_LIBS -lvpl"
fi

case $host in
	*-*-mingw*)
		if test "x$use_libdl" = "xyes" ; then
			HB_LIBS="$HB_LIBS -ldl"
		fi
		if test "x$use_libpthread" = "xyes" ; then
			HB_LIBS="$HB_LIBS -lpthread"
		fi
		HB_LIBS="$HB_LIBS -lbcrypt -lregex -luuid -lole32"
		if test "x$use_mf" = "xyes" ; then
			HB_LIBS="$HB_LIBS -lmfplat -lstrmiids"
		fi
		;;
	*-*-freebsd*)
		HB_LIBS="$HB_LIBS -lpthread"
		if test "x$use_qsv" = "xyes" ; then
			HB_LIBS="$HB_LIBS -lva -lva-drm"
		fi
		;;
	*-*-netbsd*)
		HB_LIBS="$HB_LIBS -pthread"
		;;
	*-*-openbsd*)
		HB_LIBS="$HB_LIBS -lpthread"
		;;
	*-*-linux*)
		if test "x$use_qsv" = "xyes" ; then
			HB_LIBS="$HB_LIBS -lva -lva-drm"
		fi
		if test "x$use_x265" = "xyes" ; then
			if test "x$use_numa" = "xyes" ; then
				HB_LIBS="$HB_LIBS -lnuma"
			fi
		fi
		HB_LIBS="$HB_LIBS -ldl -lpthread"
		;;
	*)
		HB_LIBS="$HB_LIBS -ldl -lpthread"
		;;
esac

AM_CONDITIONAL([GHB_FLATPAK], [test "$use_flatpak" = "yes"])

if test "x$PYTHON" != "x" ; then
	HB_PYTHON="$PYTHON"
else
	HB_PYTHON="python2"
fi

AC_SUBST(HB_PYTHON)
AC_SUBST(HB_LIBS)
AC_SUBST(HB_CPPFLAGS)

AC_OUTPUT
