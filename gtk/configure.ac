dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(ghb, 0.1)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
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
	    [try this for the bh-library prefix install directory]
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



PKG_CHECK_MODULES(GHB, [gtk+-2.0 >= 2.8 gio-2.0 hal hal-storage])
AC_SUBST(GHB_CFLAGS)
AC_SUBST(GHB_LIBS)




AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
