dnl 
dnl Pawe³ Pa³ucha <pawel@praterm.com.pl>
dnl
dnl $Id$
dnl

AC_PREREQ(2.54)
AC_INIT(szarp, 3.1, pawel@praterm.com.pl)

AC_CONFIG_MACRO_DIR([m4])

# Cross compilation - this must be early in configure.in
AC_CANONICAL_TARGET
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([-Wno-portability])
AC_CONFIG_HEADERS(config.h)

# Set default prefix
AC_PREFIX_DEFAULT([/opt/szarp])

# Sets prefix where libraries are stored. This is designed for cross complatoin
# where we usualy want to set --prefix=/usr/<arch> but this also affects installation
# directory. This option only sets where to search libraries. So cross compilation
# can be simply configured using:
#
#  ./configure --host=arm-linux-gnueabi --with-prefix=/usr/arm-linux-gnueabi
#
AC_ARG_WITH([prefix],
	AC_HELP_STRING([--with-prefix=<dir>],
	       [Set default path where libraries are stored. Defult is /usr/]),
	[
		if test "$withval" = "no" ; then
			lib_prefix="/usr/"
		else
			lib_prefix="$withval"
		fi
	], [lib_prefix="/usr/"])

# Debugging of configure script
AC_ARG_ENABLE([debug-config],
              AC_HELP_STRING([--enable-debug-config],
                             [Print some configure script debbuging info (for
                              maintainers only)]),
               debug_config=yes, )

# Locales
ALL_LINGUAS="pl"


# Compilers

# default CFLAGS and CXXFLAGS to empty
: ${CFLAGS=""} 
: ${CXXFLAGS=""} 

# use AM_FLAGS for default CXXFLAGS
: ${AM_CXXFLAGS="-O2 -g -Wall"} 
AC_SUBST([AM_CXXFLAGS])


# C compiler (sets CC)
AC_PROG_CC([gcc3 gcc-3 gcc cc])
AC_PROG_CC_C_O
# C preprocesor (sets CPP)
AC_PROG_CPP

# C++ compiler (sets CXX)
AC_PROG_CXX([g++3 g++-3 g++])

# C++ preprocesor (sets CXXCPP)
AC_PROG_CXXCPP

#libtool
AC_PROG_LIBTOOL
#enable exe extensions in conditionaly built programs on pre 2.50 autoconf's
AC_EXEEXT
# we use pkg-config for configure, so check it first
PKG_PROG_PKG_CONFIG
if test "x$PKG_CONFIG" = "x" ; then
	AC_MSG_ERROR([pkg-config is needed for configuring Szarp])
fi

# Programs needed for build.

# flex (sets LEX and LEXLIB)
AC_PROG_LEX

# check if function yylex_destroy is available (define HAVE_YYLEX_DESTROY)
AC_CHECK_YYLEX_DESTROY()

# ln -s (sets LN_S)
AC_PROG_LN_S

# bison (sets YACC)
AC_CHECK_PROG([YACC], [bison], [bison], [])
if test "x$YACC" = "x" ; then
        AC_MSG_WARN([
                      bison program is needed for compiling Szarp
                      ])
fi

AC_CHECK_PROG([DIALOG], [dialog], [dialog], [])
if test "x$DIALOG" = "x" ; then
        AC_MSG_WARN([
                      dialog program is needed for configuring Szarp
                      ])
fi

AC_CHECK_PROG([PROTOC], [protoc], [protoc], [])
if test "x$PROTOC" = "x" ; then
        AC_MSG_WARN([
                      protoc compiler not present, it is needed
		      for compiling Szarp4 components
                      ])
fi

AC_CHECK_PROG([ICONV], [iconv], [iconv], [])
if test "x$ICONV" = "x" ; then
        AC_MSG_WARN([
                      iconv program not found, you will have problems but
		      we'll try to proceed...
                      ])
fi

AC_CHECK_PROGS([JADE], [jade openjade], [])
if test "x$JADE" = "x" ; then
        AC_MSG_WARN([
                      jade program not found - docs generation won't work
                     ])
fi
AC_SUBST(JADE)

AC_CHECK_PROG([CONVERT], [convert], [convert], [])
if test "x$CONVERT" = "x" ; then
        AC_MSG_WARN([
                      convert program not found - docs generation won't work
                     ])
fi

AC_CHECK_PROG([KONWERT], [konwert], [konwert], [])
if test "x$KONWERT" = "x" ; then
        AC_MSG_WARN([
                      konwert program not found - docs generation won't work
                     ])
fi

AC_CHECK_PROG([PDFJADETEX], [pdfjadetex], [pdfjadetex], [])
if test "x$PDFJADETEX" = "x" ; then
        AC_MSG_WARN([
                      pdfjadetex program not found - PDF docs generation won't work
                     ])
fi

AC_CHECK_PROG([PYBABEL], [pybabel], [pybabel], [])
if test "x$PYBABEL" = "x" ; then
        AC_MSG_WARN([
                      pybabel program not found - pylons apps will not be included
                     ])
fi

#ICU
no_icu=
AX_CHECK_ICU([3], [ no_icu="no" ], [ no_icu="yes" ])
if test $no_icu = "yes"; then
	AC_MSG_WARN([icu libarary not found. SZARP won't compile])
fi

# Headers

# Standard headers - stdlib.h and so on (sets STDC_HEADERS)
AC_HEADER_STDC
AC_UNISTD_H

# obsolete types
AC_CHECK_TYPE(uchar, unsigned char)
AC_CHECK_TYPE(ushort, unsigned short)
AC_CHECK_TYPE(ulong, unsigned long)

# return type for signal handlers, defines RETSIGTYPE
AC_TYPE_SIGNAL

AC_CHECK_LIB(gen, main)
AC_CHECK_LIB(m, sin)

# Check endianness
big_endian=
AC_C_BIGENDIAN([big_endian=true],[],[])
if test -n "$big_endian" ; then
	AC_DEFINE(SZARP_BIG_ENDIAN, 1, [Define to 1 if current architecture is big-endian])
fi

# Workaround for Moxa UC7410/7420 glibc bug.
# TODO: make a real check for this bug.
if test "$host_cpu" = "xscale" ; then
	AC_MSG_WARN([Included workaround for arm/xscale glibc atomicitiy.h bug])
	AC_DEFINE(ARM_ATOMICITY_BUG, 1, [Define to 1 if arm/xscale glibc bug is detected])
fi

if test "$build_os" != "linux-gnu" ; then
        AC_MSG_WARN([
                       The only supported and tested build platform is
                       linux-gnu. Anything else is not supposed to work.
                       ])
fi

AC_ARG_WITH([mingw32], AC_HELP_STRING([--with-mingw32],
	[compile for mingw32 platform]), [
		if test "$withval" = "no" ; then
			with_mingw32="no"
		else
			with_mingw32="yes"
		fi
	], [with_mingw32="no"])

if test "$host_os" = "mingw32" -o "$host_os" = "mingw32msvc"; then
	with_mingw32="yes"
fi

if test "$host_os" != "linux-gnu" -a "$with_mingw32" != "yes" ; then
        AC_MSG_WARN([
                       The only supported and tested target platform are
                       linux-gnu and mingw-32. Anything else is not supposed
                       to work.
                       ])
fi

AM_CONDITIONAL(MINGW32_OPT, test "x$with_mingw32" = "xyes")

# posix thread switches
if test "$with_mingw32" != "yes" ; then
	AX_PTHREAD
fi

# Check for Python
no_python=""
AC_ARG_WITH([python], AC_HELP_STRING([--without-python], [do not use python]),
[
	if test "$withval" = "no" ; then
		no_python="yes"
	else
		no_python=""
	fi
],)
if test "$no_python" != "yes" ; then
	AM_PATH_PYTHON([2.4], [no_python=], [no_python="yes"])
	if test "$no_python" = "yes" ; then
		AC_MSG_WARN([
			     Python not found.
			     Check if 'python' program in installed in
			     your path.
			     ])
	fi

	# test if python version is >= 2.6. This is needed cause pyipk doesnt
	# work with python 2.5, but sharp has to be build on debian lenny
	AM_CONDITIONAL(PYTHON_26, test `$PYTHON -c "py=map(int,'$PYTHON_VERSION'.split('.'));print(int(py.pop(0)>=2 and py.pop(0)>=6))"` -eq 1 )
fi

# X windows tests (sets 'X_CFLAGS', 'X_PRE_LIBS', 'X_LIBS' and
# 'X_EXTRA_LIBS)
if test "$with_mingw32" != "yes" ; then
        # X includes (sets 'x_libraries' or 'no_x', checks for '--without-x' option)
        AC_PATH_X
        # Extra X options (sets X_CFLAGS, X_LIBS, X_EXTRA_LIBS, X_PRE_LIBS)
        AC_PATH_XTRA
	X_LIBS="$X_EXTRA_LIBS $X_LIBS $X_PRE_LIBS"
	if test -n "$x_includes" ; then
		X_CFLAGS="$X_CFLAGS -I$x_includes"
	fi
else
        no_x=yes
fi

if test x$no_x != xyes ; then
	no_x="yes"
	PKG_CHECK_MODULES(X11, x11, no_x="no", [AC_MSG_RESULT([
			x11-dev library not found on your system,
			configuring without X Windows, error is: $X11_PKG_ERRORS
		])
	])
	X_LIBS="$X11_LIBS $X_LIBS"
	X_CFLAGS="$X11_CFLAGS $X_CFLAGS"
fi

if test x$no_x != xyes ; then
	no_x="yes"
	PKG_CHECK_MODULES(XT, xt, no_x="no", [AC_MSG_RESULT([
			xt-dev library not found on your system,
			configuring without X Windows, error is: $XT_PKG_ERRORS
		])
	])
	X_LIBS="$XT_LIBS $X_LIBS"
	X_CFLAGS="$XT_CFLAGS $X_CFLAGS"
fi

if test x$no_x != xyes ; then
	no_x="yes"
	PKG_CHECK_MODULES(SM, sm, no_x="no", [AC_MSG_RESULT([
			sm-dev library not found on your system,
			configuring without X Windows, error is: $SM_PKG_ERRORS
		])
	])
	X_LIBS="$SM_LIBS $X_LIBS"
	X_CFLAGS="$SM_CFLAGS $X_CFLAGS"
fi

if test x$no_x != xyes ; then
	no_x="yes"
	PKG_CHECK_MODULES(ICE, ice, no_x="no", [AC_MSG_RESULT([
			ice-dev library not found on your system,
			configuring without X Windows, error is: $ICE_PKG_ERRORS
		])
	])
	X_LIBS="$ICE_LIBS $X_LIBS"
	X_CFLAGS="$ICE_CFLAGS $X_CFLAGS"
fi

if test x$no_x != xyes ; then
	no_x="yes"
	PKG_CHECK_MODULES(XEXT, xext, no_x="no", [AC_MSG_RESULT([
			xext-dev library not found on your system,
			configuring without X Windows, error is: $XEXT_PKG_ERRORS
		])
	])
	X_LIBS="$XEXT_LIBS $X_LIBS"
	X_CFLAGS="$XEXT_CFLAGS $X_CFLAGS"
fi

if test x$no_x != xyes ; then
	no_x="yes"
	PKG_CHECK_MODULES(XPM, xpm, no_x="no", [AC_MSG_RESULT([
			xpm-dev library not found on your system,
			configuring without X Windows, error is: $XPM_PKG_ERRORS
		])
	])
	X_LIBS="$XPM_LIBS $X_LIBS"
	X_CFLAGS="$XPM_CFLAGS $X_CFLAGS"
fi

no_protobuf="yes"
if test x$with_mingw32 = xno; then
	PKG_CHECK_MODULES(PROTOBUF, protobuf, no_protobuf="no", [AC_MSG_RESULT([
			libprotobuf-dev library not found on your system,	
			it is required to build Szarp 4 components
		])
	])
fi

no_zmq="yes"
if test x$with_mingw32 = xno; then
	PKG_CHECK_MODULES(ZMQ, libzmq, no_zmq="no", [AC_MSG_RESULT([
			libzmq-dev library not found on your system,	
			it is required to build Szarp 4 components
		])
	])
fi


AH_VERBATIM([_GNU_SOURCE],
[/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
	# define _GNU_SOURCE
#endif])

AH_VERBATIM([strfind],[
#define strfind(a,b) (strstr(a,b) ? (char *)(strstr(a,b)-a) : (char*) -1) /* This suxx...*/])

# wxWidgets options
AC_ARG_WITH([wx], AC_HELP_STRING([--without-wx], [do not use wxWidgets]),
        [
		if test "$withval" = "no" ; then
			no_wx="yes"
		else
			no_wx=""
		fi
	], no_wx="" )

if test "x$no_x" = "xyes" -a "$with_mingw32" != "yes" ; then
	no_wx="yes"
fi

# check for wxWidgets (sets WX_CPPFLAGS, WX_CXXFLAGS, WX_CFLAGS_ONLY, WX_LIBS)
AM_OPTIONS_WXCONFIG
wc_config_prefix="${lib_prefix}"
if test "$no_wx" != "yes" ; then
        AM_PATH_WXCONFIG(2.8.0, [], [no_wx=yes])
        if test "$no_wx" = "yes" ; then
                AC_MSG_WARN([
                              wxWidgets must be installed on your system but wx-config
                              script couldn't be found.

                              Please check that wx-config is in path, the directory
                              where wxWidgets libraries are installed (returned by
                              'wx-config --libs' command) is in LD_LIBRARY_PATH or
                              equivalent variable and wxWidgets version is 2.8.0 or
                              above.

                              Configuring without wxWidgets.
                              ])
        fi
        # Check for msgfmt to compile po files.
	if test "x$no_wx" != "xyes" ; then
	        AC_PATH_PROG([MSGFMT], [msgfmt])
	        if test "x$MSGFMT" = "x" ; then
        	        AC_MSG_WARN([
                              msgfmt program must be installed on your system
                              but couldn't be found.

                              Please check that getext package is properly
                              installed. Configuring without wxWidgets.
                              ])
			no_wx="yes"
	        fi
	fi
	if test "x$no_wx" != "xyes" ; then
	        AC_PATH_PROG([XGETTEXT], [xgettext])
	        if test "x$XGETTEXT" = "x" ; then
        	        AC_MSG_WARN([
                              xgettext program must be installed on your system
                              but couldn't be found.

                              Please check that getext package is properly
                              installed. Configuring without wxWidgets.
                              ])
			no_wx="yes"
	        fi
	fi
	if test "x$no_wx" != "xyes" ; then
	        AC_PATH_PROG([MSGMERGE], [msgmerge])
	        if test "x$MSGMERGE" = "x" ; then
        	        AC_MSG_WARN([
                              msgmerge program must be installed on your system
                              but couldn't be found.

                              Please check that getext package is properly
                              installed. Configuring without wxWidgets.
                              ])
			no_wx="yes"
	        fi
	fi
	if test "x$no_wx" != "xyes" ; then
	        AC_PATH_PROG([WXRC], [wxrc])
	        if test "x$WXRC" = "x" ; then
        	        AC_MSG_WARN([
                              wxrc program couldn't be found. ])
	        fi
	fi
	if test "x$no_wx" != "xyes" ; then
	        AM_WX_GLCANVAS(no_wxglcanvas=no, no_wxglcanvas=yes)
	        if test "x$no_wxglcanvas" = "xyes" ; then
        	        AC_MSG_WARN([
                              wxWidgets is configured without OpenGL support, disabling wx support.
                              ])
			AC_DEFINE(HAVE_GLCANVAS, 0, [Define to 1 when wxWidgets is compiled with GL support])
			no_wx=yes
		else
			AC_DEFINE(HAVE_GLCANVAS, 1, [Define to 1 when wxWidgets is compiled with GL support])
	        fi
	fi
fi
if test "$no_wx" != "yes" -a "$with_mingw32" = "yes" ; then
	AC_CHECK_TARGET_TOOL([WINDRES], [windres])
fi
AC_SUBST(MSGFMT)
AC_SUBST(XGETTEXT)
AC_SUBST(MSGMERGE)
AC_SUBST(WINDRES)

if test "$with_mingw32" = "yes" ; then
        CFLAGS="$CFLAGS -D__GNUWIN32__ -D__WIN32_IE=0x400 -D_WIN32_WINT=0x0502"
        CXXFLAGS="$CXXFLAGS -D__GNUWIN32__ -D__WIN32_IE=0x400 -D_WIN32_WINNT=0x0502"
        WX_CXXFLAGS="$WX_CXXFLAGS -DwxUSE_OWNER_DRAWN=1 -DwxUSE_GRAPHICS_CONTEXT=1"
	MINGW32_LIBS="-lwsock32 -lshlwapi -liphlpapi -lws2_32 -liconv -lexpat"
	AC_DEFINE(MINGW32, 1, [Define to 1 when compiling under mingw32
	enviroment])
fi
AC_SUBST(MINGW32_LIBS)

# check for libxml2 (sets XML_CLAGS and XML_LIBS)
PKG_CHECK_MODULES([XML], [libxml-2.0], [no_xml=], [no_xml="yes"])
if test "$no_xml" = "yes" ; then
	AC_MSG_WARN([
                              libxml2 must be installed on your system but xml2-config
                              script couldn't be found.

                              Please check that xml2-config is in path, the directory
                              where libxml2 libraries are installed (returned by
                              'xml2-config --libs' command) is in LD_LIBRARY_PATH or
                              equivalent variable and libxml2 version is 2.6.5 or
                              above.

			      Aborting configure.
                              ])
fi

AC_PROG_XSLTPROC()
AC_CHECK_PROG([XMLLINT], [xmllint], [xmllint], [])

PKG_CHECK_MODULES([XSLT], [libxslt], [no_xslt=], [no_xslt="yes"])
if test "$no_xslt" = "yes" ; then
	AC_MSG_WARN([
                              libxslt must be installed on your system but xslt-config
                              script couldn't be found.

                              Please check that xslt-config is in path, the directory
                              where libxslt libraries are installed (returned by
                              'xslt-config --libs' command) is in LD_LIBRARY_PATH or
                              equivalent variable and libxslt version is 1.1.0  or
                              above.

			      Aborting configure..
                              ])
fi

# check for newt library
no_newt=""
AC_ARG_WITH([newt], AC_HELP_STRING([--without-newt], [do not use newt library]),
        [
		if test "$withval" = "no" ; then
			no_newt="yes"
		else
			no_newt=""
		fi
	],[no_newt=""])
if test "$no_newt" != "yes" ; then
	AC_LIB_NEWT( [], [no_newt=yes] )
	if test x$no_newt = xyes ; then
                AC_MSG_WARN([
                              newt library not found on your system, check if
			      libnewt development package is installed;
			      configuring without
			    ])
	fi
fi
if test "x$no_newt" = "xyes" ; then
        AC_DEFINE(NO_NEWT, 1, [Define to 1 if libnewt library is not available])
fi

# Check for OpenSSL library
no_ssl=""
AC_ARG_WITH([ssl], AC_HELP_STRING([--without-ssl], [do not use OpenSSL library]),
[
	if test "$withval" = "no" ; then
		no_ssl="yes"
	else
		no_ssl=""
	fi
],)
if test "$no_ssl" != "yes" ; then
	AC_PATH_SSL( [], [no_ssl=yes] )
		if test x$no_ssl = xyes ; then
                AC_MSG_WARN([
			OpenSSL library not found on your system, check if
			libssl development package is installed;
			configuring without
		])
        fi
fi

if test "x$no_ssl" = "xyes" ; then
        AC_DEFINE(NO_SSL, 1, [Define to 1 if OpenSSL library is not available])
fi

# Check for c-ares library
no_cares="yes"
PKG_CHECK_MODULES(CARES, libcares, no_cares="no", [AC_MSG_RESULT([
			c-ares library not found on your system, check if
			c-ares development package is installed;
			configuring without
		])
])
if test "x$no_cares" = "xyes" ; then
        AC_DEFINE(NO_CARES, 1, [Define to 1 if c-ares library is not available])
fi

no_xmlrpc_epi=""
AC_ARG_WITH([xmlrpc_epi], AC_HELP_STRING([--without-xmlrpc-epi], [do not use xmlrpc-epi library]),
[
	if test "$withval" = "no" ; then
		no_xmlrpc_epi="yes"
	fi
],)

if test "$no_xmlrpc_epi" != "yes" ; then
	AC_PATH_XMLRPC_EPI( [], [no_xmlrpc_epi=yes] )
		if test x$no_xmlrpc_epi = xyes ; then
                AC_MSG_WARN([
			xmlrpc-epi library not found on your system, check if
			xmlrpc-epi development package is installed;
			configuring without
		])
        fi
fi

if test "x$no_xmlrpc_epi" = "xyes" ; then
        AC_DEFINE(NO_XMLRPC_EPI, 1, [Define to 1 if xmlrpc-epi library is not available])
fi


# Check for PAM library
no_pam=""
AC_ARG_WITH([pam], AC_HELP_STRING([--without-pam], [do not use PAM library]),
	[
		if test "$withval" = "no" ; then
			no_pam="yes"
		else
			no_pam=""
		fi
	],[no_pam=""])
	if test "$no_pam" != "yes" ; then
	AC_LIB_PAM( [], [no_pam=yes] )
        if test x$no_pam = xyes ; then
                AC_MSG_WARN([
                              PAM library not found on your system, check if
                              libpam development package is installed;
                              configuring without
                            ])
        fi
fi
if test "x$no_pam" = "xyes" ; then
        AC_DEFINE(NO_PAM, 1, [Define to 1 if PAM library is not available])
fi

# curl options
AC_ARG_WITH([curl], AC_HELP_STRING([--without-curl], [do not use curl library]),
        [
		if test "$withval" = "no" ; then
			no_curl="yes"
		else
			no_curl=""
		fi
	], [no_curl=""])
if test "$no_curl" != "yes" ; then
	no_curl="yes"
	PKG_CHECK_MODULES(CURL, libcurl, no_curl="no",[
		AC_MSG_WARN([
		      curl library not found on your system, check if
		      libcurl development package is installed;
		      configuring without
		])
	])
fi
AM_CONDITIONAL(HAVE_CURL, test x$no_curl != xyes)

# Check for librsync library
no_rsync=""
AC_ARG_WITH([rsync], AC_HELP_STRING([--without-rsync], [do not use librsync library]),
[
	if test "$withval" = "no" ; then
		no_rsync="yes"
	else
		no_rsync=""
	fi
],)
if test "$no_rsync" != "yes" ; then
	AC_PATH_RSYNC( [], [no_rsync=yes] )
		if test x$no_rsync = xyes ; then
                AC_MSG_WARN([
			librsync library not found on your system, check if
			librsync development package is installed;
			configuring without
		])
        fi
fi

if test "x$no_rsync" = "xyes" ; then
        AC_DEFINE(NO_RSYNC, 1, [Define to 1 if librsync library is not available])
fi

# Check for lua library
no_lua=""
AC_ARG_WITH([luajit], AC_HELP_STRING([--without-luajit], [do not try to use LuaJIT]),
[
 	if test "$withval" = "no"; then
		no_luajit="yes"
	else
		no_luajit=""
	fi
],)
AC_ARG_WITH([lua], AC_HELP_STRING([--without-lua], [do not use lua]),
[
	if test "$withval" = "no" ; then
		no_lua="yes"
	else
		no_lua=""
	fi
],)
if test "$no_luajit" != "yes"; then
	no_luajit="yes"
	PKG_CHECK_MODULES(LUA, luajit, no_luajit="no", [AC_MSG_RESULT([
			LuaJIT not found on your system, will search for liblua
								      ])
	])
fi
if test "$no_lua" != "yes" -a "$no_luajit" != "no"; then
	no_lua="yes"
	PKG_CHECK_MODULES(LUA, lua5.1, no_lua="no", [AC_MSG_RESULT([
			liblua library not found on your system,
			configuring without
								      ])
	])
fi
if test "x$no_lua" = "xyes" ; then
        AC_DEFINE(NO_LUA, 1, [Define to 1 if lua is not available])
fi
AM_CONDITIONAL(NO_LUA, test x$no_lua = xyes)

# Check for ldap library
no_ldap=""
AC_ARG_WITH([ldap], AC_HELP_STRING([--without-ldap], [do not use ldap]),
[
	if test "$withval" = "no" ; then
		no_ldap="yes"
	else
		no_ldap=""
	fi
],)
if test "$no_ldap" != "yes" ; then
	AC_PATH_LDAP( [], [no_ldap=yes] )
		if test x$no_ldap = xyes ; then
                AC_MSG_WARN([
			ldap library not found on your system,
			configuring without
		])
        fi
fi

# Check for lber library
no_lber=""
AC_ARG_WITH([lber], AC_HELP_STRING([--without-lber], [do not use lber]),
[
	if test "$withval" = "no" ; then
		no_lber="yes"
	else
		no_lber=""
	fi
],)
if test "$no_lber" != "yes" ; then
	AC_CHECK_HEADER(lber.h, [], [no_lber=yes])

	if test x$no_lber != xyes; then
		AC_CHECK_LIB(lber, ber_printf, [LBER_LIBS=-llber], [no_lber=yes])
	fi

	if test x$no_lber = xyes ; then
                AC_MSG_WARN([
			lber library not found on your system,
			configuring without
		])
        fi
fi
AM_CONDITIONAL(NO_LBER, test x$no_lber = xyes)
AC_SUBST(LBER_LIBS)

#Check for FTGL library
AC_PATH_FREETYPE()
no_ftgl="yes"
AC_PATH_FTGL(no_ftgl="no", no_ftgl="yes")
if test "x$no_ftgl" != "xyes" ; then
        AC_DEFINE(HAVE_FTGL, 1, [Define to 1 if ftgl is available])
fi

has_boost_base="no"
ac_boost_path="${lib_prefix}"
# On squeeze AX_BOOST_BASE do not execute ACTION-IF-FOUND :(
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([boost/foreach.hpp], [has_boost_base="yes"], [has_boost_base="no"])
AC_LANG_POP([C++])

AX_BOOST_BASE("1.42.0", [has_boost_base="yes"], [has_boost_base="no"])
if test "x$has_boost_base" = "xyes" ; then
	AX_BOOST_FILESYSTEM
	AX_BOOST_DATE_TIME
	AX_BOOST_REGEX
	AX_BOOST_THREAD
	AX_BOOST_SYSTEM
	AX_BOOST_PROGRAM_OPTIONS

	AX_PYTHON
	AX_BOOST_PYTHON

fi
has_boost_filesystem=${link_filesystem:-no}
has_boost_datetime=${link_date_time:-no}
has_boost_regex=${link_regex:-no}
has_boost_thread=${link_thread:-no}
has_boost_system=${link_system:-no}
has_boost_program_options=${link_program_options:-no}


#Check for LIBEVENT
no_libevent=""
AC_ARG_WITH([event], AC_HELP_STRING([--without-libevent], [do not use libevent library]),
[
	if test "$withval" = "no" ; then
		no_libevent="yes"
	else
		no_libevent=""
	fi
],)

if test "x$no_libevent" != "xyes"; then
	AC_CHECK_LIB(event, event_dispatch, [ EVENT_LIBS=-levent], [no_libevent=yes])

	if test "x$no_libevent" = "xyes"; then
	        AC_MSG_WARN([
                      libevent library is not available, some programs will not be built
                      ])
	fi

fi
AC_SUBST(EVENT_LIBS)

#Check for gstreamer library
no_gstreamer="yes"
if test "$with_mingw32" != "yes" ; then
	PKG_CHECK_MODULES(GSTREAMER, gstreamer-plugins-base-0.10, no_gstreamer="no", [AC_MSG_RESULT([
				gstreamer not found on your system,
				configuring without
			])
	])
fi
if test "x$no_gstreamer" = "xyes" ; then
        AC_DEFINE(NO_GSTREAMER, 1, [Define to 1 if  gstreamer is not available])
fi

#Check for glib library
no_glib="yes";
if test "x$with_mingw32" != "xyes"; then
	PKG_CHECK_MODULES(GLIB, glib, no_glib="no", [AC_MSG_RESULT(no)])
fi

#Check for GDK library
no_gdk="yes";
if test "x$with_mingw32" != "xyes"; then
	PKG_CHECK_MODULES(GDK, gdk-2.0, no_gdk="no", [AC_MSG_RESULT(no)])
fi

#Check for gtk library
no_gtk20="yes";
if test "x$with_mingw32" != "xyes"; then
	PKG_CHECK_MODULES(GTK, gtk+-2.0, no_gtk20="no", [AC_MSG_RESULT(no)])
fi

#Check for wnck library
no_wnck="yes"
AC_ARG_WITH([wnck], AC_HELP_STRING([--with-wnck],
	[compile with WNCK library]), [
		if test "$withval" = "no" ; then
			with_wnck="no"
		else
			with_wnck="yes"
		fi
	], with_wnck="no")
if test "x$with_wnck" = "xyes" ; then
	PKG_CHECK_MODULES(WNCK, libwnck-1.0, no_wnck="no", [AC_MSG_RESULT([
			libwnck library is not available, some programs will not be built
							   ])])
fi
AM_CONDITIONAL(HAVE_WNCK, test x$no_wnck != xyes)

#Check for sqlite library
no_sqlite="yes";
PKG_CHECK_MODULES(SQLITE3, sqlite3, no_sqlite="no", [AC_MSG_RESULT([
			sqllite3 library not found on your system,
			configuring without
		])
])
if test "x$no_sqlite" = "xyes"; then
	AC_DEFINE(NO_SQLITE3, 1, [ Define to 1 if sqlite3 is not available ])
fi

# Check for libzip library
no_zip="yes"
PKG_CHECK_MODULES(ZIP, libzip, no_zip="no", [AC_MSG_RESULT([
			libzip-dev library not found on your system,
			configuring without libzip, error is $ZIP_PKG_ERRORS
		])
	])

# Debian packages building
AC_ARG_ENABLE([deb-build],
              AC_HELP_STRING([--enable-deb-build],
                             [Configure for .deb packages build (creates slightly
			      modified install targets)]),
               deb_build=yes, )

AM_CONDITIONAL(DEB_BUILD, test x$deb_build = xyes)

# xmlTextReader conditional building
AC_ARG_WITH([xmldom], 
	AC_HELP_STRING([--with-xmldom], 
		[Use legacy XML DOM instead of xmlTextReader]),
		yes_xmldom=yes, )

if test "x$yes_xmldom" != "xyes"
then
	AC_DEFINE(USE_XMLREADER, 1, [ Define to 1 when use xmlTextReader instead of XML DOM])
fi

# Debian packages building

AC_CHECK_FUNCS(index rindex scandir alphasort asprintf localtime_r gmtime_r timegm dirname basename pow10)

AH_VERBATIM([index],
[
#ifndef HAVE_INDEX
#define index strchr
#endif

#ifndef HAVE_RINDEX
#define rindex strrchr
#endif
])

AC_CHECK_HEADERS(libgen.h)

no_sys_queue=""
AC_CHECK_HEADERS(sys/queue.h, [no_sys_queue="no"], [no_sys_queue="yes"])

if test $no_sys_queue = "no"; then
        AC_MSG_CHECKING(for TAILQ_FOREACH in sys/queue.h)
        AC_EGREP_CPP(yes,
[
#include <sys/queue.h>
#ifdef TAILQ_FOREACH
 yes
#endif
],      [AC_MSG_RESULT(yes)
         AC_DEFINE(HAVE_TAILQFOREACH, 1,
                [Define if TAILQ_FOREACH is defined in <sys/queue.h>])],
        [AC_MSG_RESULT(no)]
        )
fi

AH_VERBATIM([tailq],
[
#ifndef HAVE_TAILQFOREACH
#define TAILQ_FIRST(head)               ((head)->tqh_first)
#define TAILQ_END(head)                 NULL
#define TAILQ_NEXT(elm, field)          ((elm)->field.tqe_next)
#define TAILQ_FOREACH(var, head, field)                                 \
        for((var) = TAILQ_FIRST(head);                                  \
            (var) != TAILQ_END(head);                                   \
            (var) = TAILQ_NEXT(var, field))
#define TAILQ_INSERT_BEFORE(listelm, elm, field) do {                   \
        (elm)->field.tqe_prev = (listelm)->field.tqe_prev;              \
        (elm)->field.tqe_next = (listelm);                              \
        *(listelm)->field.tqe_prev = (elm);                             \
        (listelm)->field.tqe_prev = &(elm)->field.tqe_next;             \
} while (0)
#endif /* TAILQ_FOREACH */
])

#############################################################################
# FOR DEVELOPERS
#############################################################################
# Subdirs included.
# Each subdir is tested for inclusion (some are always included).
# Every optional program has defined value PROGRAM_NAME. If program has to be
# compiled, PROGRAM_NAME value is set to program_name. Otherwise it is empty.
# Example:
#    if test x$compile_optional_program = xyes ; then
#        OPTIONAL_PROGRAM=optional_program
#    fi
#    AC_SUBST(OPTIONAL_PROGRAM)
# In Makefile.am use something like this:
#  bin_PROGRAMS = @OPTIONAL_PROGRAM@
#  EXTRA_PROGRAMS = optional_program

# Add subdirs
SUBDIRS=""

# Always included
SUBDIRS="$SUBDIRS libSzarp libSzarp2"

# Included under linux
if test x$with_mingw32 != xyes ; then
        SUBDIRS="$SUBDIRS python"
	EXTRSZB=extrszb
fi

AC_SUBST(EXTRSZB)

# Sender program included under Linux
if test "$with_mingw32" != "yes" ; then
        SUBDIRS="$SUBDIRS sender"
fi


# Programs from parcook directory included under Linux
if test "$with_mingw32" != "yes" ; then
        SUBDIRS="$SUBDIRS parcook"
fi

# ISL - needs Linux, SSL, and PAM.
if test "$with_mingw32" != "yes" -a "$no_ssl" != "yes" -a "$no_pam" != "yes" ; then
	SUBDIRS="$SUBDIRS isl"
fi

# New meaner3 daemon, works under Linux.
if test "$with_mingw32" != "yes" ; then
	SUBDIRS="$SUBDIRS meaner3"
fi

# szarp base viewer, needs libnewt
if test x$no_newt != xyes ; then
	SZBEDIT=szbedit
fi
AC_SUBST(SZBEDIT)

# WX programs needs wxWidgets
if test x$no_wx != xyes ; then
        SUBDIRS="$SUBDIRS extern wx"
        DRAW3=draw3
        if test x$host_os != xmingw32 ; then
                IPKEDIT=ipkedit
        fi
	HELPCOMMAND="\${lib_prefix}/bin/wxhelp"
else
	HELPCOMMAND="konqueror"
fi
AC_SUBST(DRAW3)
AC_SUBST(IPKEDIT)
AC_SUBST(HELPCOMMAND)

# Utils directory, some of them needs CodeBase.
SUBDIRS="$SUBDIRS utils"
if test x$with_mingw32 != xyes ; then
	AGREGATOR=agregator
	ANALIZA2IPK=analiza2ipk
        HASHTEST=hashtest
	IPK2SZARP=ipk2szarp
	IPKREP=ipkrep
	IPKMAX=ipkmax
	PRECCONV=precconv
	SZARP2IPK=szarp2ipk
fi
AC_SUBST(AGREGATOR)
AC_SUBST(ANALIZA2IPK)
AC_SUBST(HASHTEST)
AC_SUBST(IPK2SZARP)
AC_SUBST(IPKREP)
AC_SUBST(IPKMAX)
AC_SUBST(PRECCONV)
AC_SUBST(OLDCREATE)
AC_SUBST(RECONF)
AC_SUBST(SZARP2IPK)

#unitedmn and stampd needs libevent
if test "x$no_libevent" != "xyes"; then
	UNITEDMN=unitedmn;
	STAMPD=stampd
fi
AC_SUBST(UNITEDMN)
AC_SUBST(STAMPD)

if test "x$no_xmlrpc_epi" != "xyes" -a "x$no_curl" != "xyes"; then
	DDEDMN=ddedmn;
fi
AC_SUBST(DDEDMN)

if test "x$no_lber" != "xyes"; then
	DLMSDMN=dlmsdmn;	
fi
AC_SUBST(DLMSDMN)

#PSET needs ldap and ssl
if test "x$no_ldap" != "xyes" -a "x$no_ssl" != "xyes"; then
	PSETD=psetd;
fi
AC_SUBST(PSETD)

#szarp sync applications
want_ssc=""
want_sss=""
if test x$no_ssl != xyes -a x$no_rsync != xyes ; then
	if test x$no_wx != xyes  -a x$no_cares != xyes; then
		want_ssc=yes;
	fi
	if test x$with_mingw32 != xyes; then
		want_sss=yes;
	fi
fi

no_cppnit="yes"
PKG_CHECK_MODULES(CPPUNIT, x11, no_cppunit="no", [AC_MSG_RESULT([
	cppunit library not found on your system,
	configuring without cppunit, draw test app will not be built
	])
])
AM_CONDITIONAL(HAVE_CPPUNIT, test "x$no_cppunit" != xyes)

AC_LIB_CGAL( [], [no_cgal=yes] )
if test "x$no_cgal" = "xyes" ; then
        AC_DEFINE(NO_CGAL, 1, [Define to 1 if CGAL library is not available])
fi


# Python/pylons
if test x$no_python != xyes -a "x$PYBABEL" != "x" -a x$with_mingw32 != xyes; then
	SUBDIRS="$SUBDIRS www"
fi

lua_optimise=yes
# Debian packages building
AC_ARG_WITH([lua-param-optimiser], AC_HELP_STRING([--without-lua-param-optimiser], [Build without Lua param optimiser]),
[
	if test "$withval" = "no" ; then
		lua_optimise="yes"
	else
		lua_optimise="no"
	fi
])

if test x$lua_optimise = xyes; then 
	AC_DEFINE(LUA_PARAM_OPTIMISE, 1, [Define if lua param optimiser should be built in])
fi

# Resources directory
SUBDIRS="$SUBDIRS resources script"

# pyipk directory
if test x$no_python != xyes -a x$with_mingw32 != xyes; then
	SUBDIRS="$SUBDIRS pyipk"
fi

AC_SUBST(SUBDIRS)

#############################################################################
# FOR DEVELOPERS
#############################################################################
# Makefiles to create

AC_OUTPUT( \
	Makefile \
	extern/Makefile \
	extern/wxscintilla/Makefile \
	isl/Makefile \
	isl/server/Makefile \
	isl/paramd/Makefile \
	isl/python/Makefile \
	libSzarp/Makefile \
	libSzarp2/Makefile \
	libSzarp2/tools/ipk_config \
	python/Makefile \
	python/pyszbase/Makefile \
	python/protobuf/Makefile \
	python/meaner4/Makefile \
	python/meaner4/unittests/Makefile \
	meaner3/Makefile \
	parcook/Makefile \
	parcook/analiza/Makefile \
	pyipk/Makefile \
	resources/Makefile \
	resources/documentation/Makefile \
	resources/documentation/new/Makefile \
	script/Makefile \
	script/sysinfo \
	sender/Makefile \
	utils/szarpinfo \
	utils/Makefile \
	utils/nagios/Makefile \
	www/Makefile \
	www/pylons/Makefile \
	www/pylons/sssweb/Makefile \
	wx/Makefile \
	wx/common/Makefile \
	wx/draw3/Makefile \
	wx/ekstraktor3/Makefile \
	wx/filler/Makefile \
	wx/isledit/Makefile \
	wx/scc/Makefile \
	wx/raporter3/Makefile \
	wx/kontroler3/Makefile \
	wx/viszio/Makefile \
	wx/confedit/Makefile \
	wx/wxhelp/Makefile \
	wx/sync/Makefile \
	wx/koper/Makefile \
	wx/szau/Makefile \
	wx/szast/Makefile \
	wx/setter/Makefile \
	)

# Print configure raport
echo
echo "SZARP $VERSION configured."
echo "Build host type:              $build"
echo "Configured for host type:     $host"

echo -n "Configured with ICU:               "
if test "$no_icu" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with X windows:         "
if test "$no_x" != "yes" ; then
        echo "yes"
else
        echo "no"
fi
echo -n "Configured with wxWidgets:         "
if test "$no_wx" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with newt:              "
if test "$no_newt" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with OpenSSL:           "
if test "$no_ssl" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with xmlrpc-epi:        "
if test "x$no_xmlrpc_epi" != "xyes" ; then
        echo "yes"
else
        echo "no"
fi
echo -n "Configured with PAM:               "
if test "$no_pam" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with curl:              "
if test "$no_curl" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with rsync:             "
if test "$no_rsync" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with lua:               "
if test "$no_lua" != "yes" ; then
	if test "$no_luajit" != "yes"; then
		echo "yes (using LuaJIT)"
	else
		echo "yes (not using LuaJIT)"
	fi
else
        echo "no"
fi

echo -n "Configured with FTGL:              "
if test "$no_ftgl" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with GDK-2.0:           "
if test "$no_gdk" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with GLIB:              "
if test "$no_glib" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with WNCK:              "
if test "$no_wnck" != "yes" ; then
        echo "yes"
else
        echo "no"
fi


echo -n "Configured with python 2.4         "
if test "$no_python" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with ldap               "
if test "$no_ldap" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with lber               "
if test "$no_lber" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with sqlite3            "
if test "$no_sqlite" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with gstreamer          "
if test "$no_gstreamer" != "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with libevent           "
if test "x$no_libevent" != "xyes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with c-ares             "
if test "x$no_cares" != "xyes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configure with cppunit             "
if test "x$no_cppunit" != "xyes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with boost              "
if test "x$has_boost_base" = "xyes" ; then
        echo "yes"

	echo "Configured with boost filesystem   $has_boost_filesystem"

	echo "Configured with boost date time    $has_boost_datetime"

	echo "Configured with boost regex        $has_boost_regex"

	echo "Configured with boost thread       $has_boost_thread"

	echo "Configured with boost system       $has_boost_system"

	echo "Configured with boost program opts $has_boost_program_options"

else
        echo "no"
fi

echo -n "Configured with param optimiser    "
if test "x$lua_optimise" = "xyes" ; then
        echo "yes"
else
	echo "no"
fi

echo -n "Configured with zip                "
if test "x$no_zip" != "xyes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured with zmq                "
if test "x$no_zmq" != "xyes" ; then
        echo "yes"
else
        echo "no"
fi

echo -n "Configured for .deb build:         "
if test "$deb_build" = "yes" ; then
        echo "yes"
else
        echo "no"
fi

echo "Source directory:             $srcdir"
echo "Install prefix:               $prefix"
echo "Subdirs to include:          $SUBDIRS"
if test x$debug_config = xyes ; then
        echo "Configure debug info: "
	echo "Distro: $distro"
        echo "CC: $CC"
        echo "CPP: $CPP"
        echo "CXX: $CXX"
        echo "CXXCPP: $CXXCPP"
        echo "LDFLAGS: $LDFLAGS"
        echo "INSTALL: $INSTALL"
        echo "CFLAGS: $CFLAGS"
        echo "CXXFLAGS: $CXXFLAGS"
        echo "CPPFLAGS: $CPPFLAGS"
        echo "LIBS: $LIBS"
        echo "X_CFLAGS: $X_CFLAGS"
        echo "X_LIBS: $X_LIBS"
        echo "X_EXTRALIBS: $X_EXTRALIBS"
        echo "X_PRELIBS: $X_PRELIBS"
        echo "WX_CPPFLAGS: $WX_CPPFLAGS"
        echo "WX_CXXFLAGS: $WX_CXXFLAGS"
        echo "WX_CFLAGS_ONLY: $WX_CFLAGS_ONLY"
	echo "WC_LIBS: $WX_LIBS"
        echo "NEWT_LIBS: $NEWT_LIBS"
        echo "CURL_CFLAGS: $CURL_CFLAGS"
        echo "CURL_LIBS: $CURL_LIBS"
	echo "LUA_CFLAGS: $LUA_CFLAGS"
	echo "LUA_LIBS: $LUA_LIBS"
        echo "GDK_CFLAGS: $GDK_CFLAGS"
        echo "GDK_LIBS: $GDK_LIBS"
        echo "GTK_CFLAGS: $GTK_CFLAGS"
        echo "GTK_LIBS: $GTK_LIBS"
        echo "XML_CFLAGS: $XML_CFLAGS"
        echo "XML_LIBS: $XML_LIBS"
        echo "XSLT_CFLAGS: $XSLT_CFLAGS"
        echo "XSLT_LIBS: $XSLT_LIBS"
        echo "WNCK_CFLAGS: $WNCK_CFLAGS"
        echo "WNCK_LIBS: $WNCK_LIBS"
        echo "SQLITE3_CFLAGS: $SQLITE3_CFLAGS"
        echo "SQLITE3_LIBS: $SQLITE3_LIBS"
        echo "GSTREAMER_CFLAGS: $GSTREAMER_CFLAGS"
        echo "GSTREAMER_LIBS: $GSTREAMER_LIBS"
        echo "EVENT_CFLAGS: $EVENT_CFLAGS"
        echo "EVENT_LIBS: $EVENT_LIBS"
        echo "WNCK_CFLAGS: $WNCK_CFLAGS"
        echo "WNCK_LIBS: $WNCK_LIBS"
        echo "BOOST_CPPFLAGS: $BOOST_CPPFLAGS"
        echo "BOOST_LIBS: $BOOST_LDFLAGS"
        echo "BOOST_SYSTEM_LIB: $BOOST_SYSTEM_LIB"
        echo "BOOST_PROGRAM_OPTIONS_LIB: $BOOST_PROGRAM_OPTIONS_LIB"
        echo "BOOST_DATE_TIME_LIB: $BOOST_DATE_TIME_LIB"
        echo "BOOST_PYTHON_LIB: $BOOST_PYTHON_LIB"
        echo "ZIP_CFLAGS: $ZIP_CFLAGS"
        echo "ZIP_LIBS: $ZIP_LIBS"
fi
echo
echo "Type 'make' to build SZARP"
echo
