dnl Adapted from wxWindows 2.4.0 m4 macros

dnl ---------------------------------------------------------------------------
dnl Macros for wxWindows detection. Typically used in configure.in as:
dnl
dnl 	AC_ARG_ENABLE(...)
dnl 	AC_ARG_WITH(...)
dnl	...
dnl	AM_OPTIONS_WXCONFIG
dnl	...
dnl	...
dnl	AM_PATH_WXCONFIG(2.3.4, wxWin=1)
dnl     if test "$wxWin" != 1; then
dnl        AC_MSG_ERROR([
dnl     	   wxWindows must be installed on your system
dnl     	   but wx-config script couldn't be found.
dnl     
dnl     	   Please check that wx-config is in path, the directory
dnl     	   where wxWindows libraries are installed (returned by
dnl     	   'wx-config --libs' command) is in LD_LIBRARY_PATH or
dnl     	   equivalent variable and wxWindows version is 2.3.4 or above.
dnl        ])
dnl     fi
dnl     CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
dnl     CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
dnl     CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
dnl     
dnl     LDFLAGS="$LDFLAGS $WX_LIBS"
dnl ---------------------------------------------------------------------------

dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_WXCONFIG
dnl
dnl adds support for --wx-prefix, --wx-exec-prefix and --wx-config 
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_WXCONFIG],
[
   AC_ARG_WITH(wx-prefix, [  --with-wx-prefix=PREFIX   Prefix where wxWindows is installed (optional)],
               wx_config_prefix="$withval", )
   AC_ARG_WITH(wx-exec-prefix,[  --with-wx-exec-prefix=PREFIX Exec prefix where wxWindows is installed (optional)],
               wx_config_exec_prefix="$withval", )
   AC_ARG_WITH(wx-config,[  --with-wx-config=CONFIG   wx-config script to use (optional)],
               wx_config_name="$withval", wx_config_name="")
   AC_ARG_ENABLE(wx-debug, [ --enable-wx-debug	Enable debug in wx (optional)],
	       wx_enable_debug=yes, wx_enable_debug=no)

])

dnl ---------------------------------------------------------------------------
dnl AM_PATH_WXCONFIG(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Test for wxWindows, and define WX_C*FLAGS and WX_LIBS. Set WX_CONFIG_NAME
dnl environment variable to override the default name of the wx-config script
dnl to use. Set WX_CONFIG_PATH to specify the full path to wx-config - in this
dnl case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the wx-config script
dnl
AC_DEFUN([AM_PATH_WXCONFIG],
[
  dnl do we have wx-config name: it can be wx-config or wxd-config or ...
  if test x${WX_CONFIG_NAME+set} != xset ; then
     WX_CONFIG_NAME=wx-config
  fi
  if test "x$wx_config_name" != x ; then
     WX_CONFIG_NAME="$wx_config_name"
  fi

  dnl deal with optional prefixes
  if test x$wx_config_exec_prefix != x ; then
     wx_config_args="$wx_config_args --exec-prefix=$wx_config_exec_prefix"
     WX_LOOKUP_PATH="$wx_config_exec_prefix/bin"
  fi
  if test x$wx_config_prefix != x ; then
     wx_config_args="$wx_config_args --prefix=$wx_config_prefix"
     WX_LOOKUP_PATH="$WX_LOOKUP_PATH:$wx_config_prefix/bin"
  fi

  dnl don't search the PATH if WX_CONFIG_NAME is absolute filename
  if test -x "$WX_CONFIG_NAME" ; then
     AC_MSG_CHECKING(for wx-config)
     WX_CONFIG_PATH="$WX_CONFIG_NAME"
     AC_MSG_RESULT($WX_CONFIG_PATH)
  else
     AC_PATH_TOOL(WX_CONFIG_PATH, $WX_CONFIG_NAME, no, "$WX_LOOKUP_PATH:$PATH")
  fi

  if test "$WX_CONFIG_PATH" != "no" ; then
    WX_VERSION=""
    no_wx=""

    dnl use libwxgtk release for default
    if test -z $wx_config_name ; then
	wx_config_args="$wx_config_args --toolkit=gtk2"
    fi

    dnl use debug if enabled
    AC_MSG_WARN(wx-enable-debug = ${wx_enable_debug})
    if test "$wx_enable_debug" == "yes" ; then
	wx_config_args="$wx_config_args --debug=yes"
    fi

    WX_CONFIG_WITH_ARGS="$WX_CONFIG_PATH $wx_config_args"

    min_wx_version=ifelse([$1], ,2.2.1,$1)
    AC_MSG_CHECKING(for wxWindows version >= $min_wx_version)


    WX_VERSION=`$WX_CONFIG_WITH_ARGS --version`
    wx_config_major_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wx_config_minor_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wx_config_micro_version=`echo $WX_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    wx_requested_major_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    wx_requested_minor_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    wx_requested_micro_version=`echo $min_wx_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    wx_ver_ok=""
    if test $wx_config_major_version -gt $wx_requested_major_version; then
      wx_ver_ok=yes
    else
      if test $wx_config_major_version -eq $wx_requested_major_version; then
         if test $wx_config_minor_version -gt $wx_requested_minor_version; then
            wx_ver_ok=yes
         else
            if test $wx_config_minor_version -eq $wx_requested_minor_version; then
               if test $wx_config_micro_version -ge $wx_requested_micro_version; then
                  wx_ver_ok=yes
               fi
            fi
         fi
      fi
    fi

    if test "x$wx_ver_ok" = x ; then
      no_wx=yes
    else
      WX_LIBS=`$WX_CONFIG_WITH_ARGS --libs std,aui`
      if echo `$WX_CONFIG_WITH_ARGS --basename` | grep -q 'gtk' ; then
        WX_LIBS="$WX_LIBS -lgdk-x11-2.0 -lgtk-x11-2.0 -lgobject-2.0"
      fi

      dnl starting with version 2.2.6 wx-config has --cppflags argument
      wx_has_cppflags=""
      if test $wx_config_major_version -gt 2; then
        wx_has_cppflags=yes
      else
        if test $wx_config_major_version -eq 2; then
           if test $wx_config_minor_version -gt 2; then
              wx_has_cppflags=yes
           else
              if test $wx_config_minor_version -eq 2; then
                 if test $wx_config_micro_version -ge 6; then
                    wx_has_cppflags=yes
                 fi
              fi
           fi
        fi
      fi

      if test "x$wx_has_cppflags" = x ; then
         dnl no choice but to define all flags like CFLAGS
         WX_CFLAGS=`$WX_CONFIG_WITH_ARGS --cflags`
         WX_CPPFLAGS=$WX_CFLAGS
         WX_CXXFLAGS=$WX_CFLAGS

         WX_CFLAGS_ONLY=$WX_CFLAGS
         WX_CXXFLAGS_ONLY=$WX_CFLAGS
      else
         dnl we have CPPFLAGS included in CFLAGS included in CXXFLAGS
         WX_CPPFLAGS=`$WX_CONFIG_WITH_ARGS --cppflags`
         WX_CXXFLAGS=`$WX_CONFIG_WITH_ARGS --cxxflags`
         WX_CFLAGS=`$WX_CONFIG_WITH_ARGS --cflags`

         WX_CFLAGS_ONLY=`echo $WX_CFLAGS | sed "s@^$WX_CPPFLAGS *@@"`
         WX_CXXFLAGS_ONLY=`echo $WX_CXXFLAGS | sed "s@^$WX_CFLAGS *@@"`
      fi
      WX_CFLAGS+=" -fno-strict-aliasing "
      WX_CXXFLAGS+=" -fno-strict-aliasing "
      WX_CPPFLAGS+=" -fno-strict-aliasing "
    fi

    if test "x$no_wx" = x ; then
       AC_MSG_RESULT(yes (version $WX_VERSION))
       ifelse([$2], , :, [$2])
    else
       if test "x$WX_VERSION" = x; then
	  dnl no wx-config at all
	  AC_MSG_RESULT(no)
       else
	  AC_MSG_RESULT(no (version $WX_VERSION is not new enough))
       fi

       WX_CFLAGS=""
       WX_CPPFLAGS=""
       WX_CXXFLAGS=""
       WX_LIBS=""
       ifelse([$3], , :, [$3])
    fi
  else
    ifelse([$3], , :, [$3])
  fi

  AC_SUBST(WX_CPPFLAGS)
  AC_SUBST(WX_CFLAGS)
  AC_SUBST(WX_CXXFLAGS)
  AC_SUBST(WX_CFLAGS_ONLY)
  AC_SUBST(WX_CXXFLAGS_ONLY)
  AC_SUBST(WX_LIBS)
  AC_SUBST(WX_VERSION)
])


# Pawe³ Pa³ucha
##################################################################################################

dnl Tests if wx-config is compiled with GLCanvas support
dnl assumes that AM_PATH_WXCONFIG was called and wx_* variables are set
dnl

AC_DEFUN([AM_WX_GLCANVAS], [
	ac_save_cflags="$CFLAGS"
	ac_save_cppflags="$CPPFLAGS"
	ac_save_libs="$LIBS"

	CFLAGS=""
	CPPFLAGS=$($WX_CONFIG_PATH --cflags base,gl,core);
	LIBS=$($WX_CONFIG_PATH --libs base,gl,core);

	AC_MSG_CHECKING(for GLCanvas support in wxWidgets)

	AC_LANG_PUSH(C++)
	AC_LINK_IFELSE([AC_LANG_SOURCE([[
#include <wx/wx.h>		
#include <wx/glcanvas.h>

class A : public wxApp {
	wxGLCanvas c();	
};

IMPLEMENT_APP(A)

]])], [no_wxglcanvas=no], [no_wxglanvas=yes])

	
	AC_LANG_POP(C++)
	WXGL_CFLAGS=
	WXGL_LIBS=
	if test "x$no_wxglcanvas" = "xno"; then
		WXGL_CFLAGS=$($WX_CONFIG_PATH --cflags gl)
		WXGL_LIBS="$($WX_CONFIG_PATH --libs gl) -lGLU -lGL"
		AC_MSG_RESULT( yes)
     		ifelse([$1], , :, [$1])     
	else
		AC_MSG_RESULT( no)
		ifelse([$2], , :, [$2])
	fi

	CFLAGS="$ac_save_cflags"
	CPPFLAGS="$ac_save_cppflags"
	LIBS="$ac_save_libs"


	if test "$host_os" = "mingw32" -o "$host_os" = "mingw32msvc"; then
		WXGL_LIBS="-lglu32 -lopengl32 $WXGL_LIBS"
		_tmp_wxgl=`echo $WXGL_LIBS | sed -e "s/-lGLU//g" | sed -e "s/-lGL//g"`
		WXGL_LIBS=$_tmp_wxgl
	fi

	AC_SUBST(WXGL_CFLAGS)
	AC_SUBST(WXGL_LIBS)
])

