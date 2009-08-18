dnl autoconf macros for SZARP
dnl 
dnl Pawe³ Pa³ucha <pawel@praterm.com.pl>
dnl
dnl $Id: acinclude.m4 6725 2009-05-18 09:16:48Z reksio $
dnl


###############################################################################################

# Adapted from:
# Configure paths for LIBXML2
# Toshio Kuratomi 2001-04-21
# Adapted from:
# Configure paths for GLIB
# Owen Taylor     97-11-3

dnl AM_PATH_XML2([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for XML, and define XML_CFLAGS and XML_LIBS
dnl
AC_DEFUN([AM_PATH_XML2],[
AC_ARG_WITH(xml-prefix,
            [  --with-xml-prefix=PREFIX   Prefix where libxml is installed (optional)],
            xml_config_prefix="$withval", )
AC_ARG_WITH(xml-exec-prefix,
            [  --with-xml-exec-prefix=PREFIX Exec prefix where libxml is installed (optional)],
            xml_config_exec_prefix="$withval", )
AC_ARG_ENABLE(xmltest,
              [  --disable-xmltest       Do not try to compile and run a test LIBXML program],,
              enable_xmltest=yes)

  if test x$xml_config_exec_prefix != x ; then
     xml_config_args="$xml_config_args --exec-prefix=$xml_config_exec_prefix"
     if test x${XML2_CONFIG+set} != xset ; then
        XML2_CONFIG=$xml_config_exec_prefix/bin/xml2-config
     fi
  fi
  if test x$xml_config_prefix != x ; then
     xml_config_args="$xml_config_args --prefix=$xml_config_prefix"
     if test x${XML2_CONFIG+set} != xset ; then
        XML2_CONFIG=$xml_config_prefix/bin/xml2-config
     fi
  fi

  AC_PATH_TOOL(XML2_CONFIG, xml2-config, no)
  min_xml_version=ifelse([$1], ,2.0.0,[$1])
  AC_MSG_CHECKING(for libxml - version >= $min_xml_version)
  no_xml=""
  if test "$XML2_CONFIG" = "no" ; then
    no_xml=yes
  else
    XML_CFLAGS=`$XML2_CONFIG $xml_config_args --cflags`
    XML_LIBS=`$XML2_CONFIG $xml_config_args --libs`
    xml_config_major_version=`$XML2_CONFIG $xml_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    xml_config_minor_version=`$XML2_CONFIG $xml_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    xml_config_micro_version=`$XML2_CONFIG $xml_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_xmltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $XML_CFLAGS"
      LIBS="$XML_LIBS $LIBS"
dnl
dnl Now check if the installed libxml is sufficiently new.
dnl (Also sanity checks the results of xml2-config to some extent)
dnl
      rm -f conf.xmltest
      AC_TRY_RUN([
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/xmlversion.h>

int 
main()
{
  int xml_major_version, xml_minor_version, xml_micro_version;
  int major, minor, micro;
  char *tmp_version;

  system("touch conf.xmltest");

  /* Capture xml2-config output via autoconf/configure variables */
  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = (char *)strdup("$min_xml_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string from xml2-config\n", "$min_xml_version");
     exit(1);
   }
   free(tmp_version);

   /* Capture the version information from the header files */
   tmp_version = (char *)strdup(LIBXML_DOTTED_VERSION);
   if (sscanf(tmp_version, "%d.%d.%d", &xml_major_version, &xml_minor_version, &xml_micro_version) != 3) {
     printf("%s, bad version string from libxml includes\n", "LIBXML_DOTTED_VERSION");
     exit(1);
   }
   free(tmp_version);

 /* Compare xml2-config output to the libxml headers */
  if ((xml_major_version != $xml_config_major_version) ||
      (xml_minor_version != $xml_config_minor_version) ||
      (xml_micro_version != $xml_config_micro_version))
    {
      printf("*** libxml header files (version %d.%d.%d) do not match\n",
         xml_major_version, xml_minor_version, xml_micro_version);
      printf("*** xml2-config (version %d.%d.%d)\n",
         $xml_config_major_version, $xml_config_minor_version, $xml_config_micro_version);
      return 1;
    } 
/* Compare the headers to the library to make sure we match */
  /* Less than ideal -- doesn't provide us with return value feedback, 
   * only exits if there's a serious mismatch between header and library.
   */
    LIBXML_TEST_VERSION;

    /* Test that the library is greater than our minimum version */
    if ((xml_major_version > major) ||
        ((xml_major_version == major) && (xml_minor_version > minor)) ||
        ((xml_major_version == major) && (xml_minor_version == minor) &&
        (xml_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of libxml (%d.%d.%d) was found.\n",
               xml_major_version, xml_minor_version, xml_micro_version);
        printf("*** You need a version of libxml newer than %d.%d.%d. The latest version of\n",
           major, minor, micro);
        printf("*** libxml is always available from ftp://ftp.xmlsoft.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the xml2-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of LIBXML, but you can also set the XML2_CONFIG environment to point to the\n");
        printf("*** correct copy of xml2-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
    }
  return 1;
}
],, no_xml=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi

  if test "x$no_xml" = x ; then
     AC_MSG_RESULT(yes (version $xml_config_major_version.$xml_config_minor_version.$xml_config_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$XML2_CONFIG" = "no" ; then
       echo "*** The xml2-config script installed by LIBXML could not be found"
       echo "*** If libxml was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the XML2_CONFIG environment variable to the"
       echo "*** full path to xml2-config."
     else
       if test -f conf.xmltest ; then
        :
       else
          echo "*** Could not run libxml test program, checking why..."
          CFLAGS="$CFLAGS $XML_CFLAGS"
          LIBS="$LIBS $XML_LIBS"
          AC_TRY_LINK([
#include <libxml/xmlversion.h>
#include <stdio.h>
],      [ LIBXML_TEST_VERSION; return 0;],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding LIBXML or finding the wrong"
          echo "*** version of LIBXML. If it is not finding LIBXML, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
          echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means LIBXML was incorrectly installed"
          echo "*** or that you have moved LIBXML since it was installed. In the latter case, you"
          echo "*** may want to edit the xml2-config script: $XML2_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi

     XML_CFLAGS=""
     XML_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(XML_CFLAGS)
  AC_SUBST(XML_LIBS)
  rm -f conf.xmltest
])


dnl Adapted from wxWindows 2.4.0 m4 macros

#################################################################################################

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
    AC_MSG_WARN(wx-enable-debug = x${wx_enable_debug}x)
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
      WX_LIBS=`$WX_CONFIG_WITH_ARGS --libs`

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
		WXGL_CFLAGS=$($WX_CONFIG_PATH --cflags gl);
		WXGL_LIBS=$($WX_CONFIG_PATH --libs gl);
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
	fi

	AC_SUBST(WXGL_CFLAGS)
	AC_SUBST(WXGL_LIBS)
])

dnl Checks for xsltproc program, with given xslt library version (default is
dnl 10010). Sets XSLTPROC and XSLT_VERSION variable.
dnl AC_PROG_XSLTPROC([VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])
AC_DEFUN([AC_PROG_XSLTPROC], [
        AC_CHECK_PROG([XSLTPROC], [xsltproc], [xsltproc], [])
        ac_xsltproc_ok=yes
        if test "x$XSLTPROC" != xxsltproc ; then
                AC_MSG_WARN([
                        xsltproc program (XSLT processor) was not found in
                        your path. XSLT-based tools won't work. ])
                XSLT_VERSION=00000
                ac_xsltproc_ok=no
        else
		AC_MSG_CHECKING([for xsltproc version])
                XSLT_VERSION=`$XSLTPROC --version | grep -o -m 1 'libxslt [[0-9]]*' | grep -o '[[0-9]]\+'`
                if test -n "$1" ; then
                        ac_xslt_version=$1
                else
                        ac_xslt_version=10010
                fi
                if test "$XSLT_VERSION" -lt "$ac_xslt_version" ; then
                        AC_MSG_WARN([
                        xslproc program (XSLT processor) version found 
                        ($XSLT_VERSION) is too old (libxslt $ac_xslt_version based
                        version required). XSLT-based tools may not work.])
                        ac_xsltproc_ok=no
                else 
                        AC_MSG_RESULT([$XSLT_VERSION])
                fi
        fi
        XSLTPROC=xsltproc
        AC_SUBST(XSLT_VERSION)
        AC_SUBST(XSLTPROC)
        if test x$ac_xsltproc_ok != xyes ; then
               ifelse([$3], , :, [$3])
        else
               ifelse([$2], , :, [$2])
        fi
])


# This macro check if flex function yylex_destroy is available. This function
# is generated by flex, so we have just to check flex version - 2.5.9 is
# the first one with yylex_destroy. On success HAVE_YYLEX_DESTROY is defined.
AC_DEFUN([AC_CHECK_YYLEX_DESTROY], [
	AC_MSG_CHECKING([for yylex_destroy])
	if test "x$LEX" = "xflex" ; then
		ac_flex_version_string=`$LEX --version | tr -d 'a-z '`
		ac_flex_version_major=`echo $ac_flex_version_string \
			| cut -f 1 -d .`
		ac_flex_version_minor=`echo $ac_flex_version_string \
			| cut -f 2 -d .`
		ac_flex_version_sub=`echo $ac_flex_version_string \
			| cut -f 3 -d .`
		if test \( "$ac_flex_version_major" -gt 2 \) ; then
			o1=yes
		else
			o1=false
		fi
		if test	\( "$ac_flex_version_major" = 2 \) -a \
			\( "$ac_flex_version_minor" -gt 5 \) ; then
			o2=yes
		else
			o2=false
		fi
		if test	\( "$ac_flex_version_major" = 2 \) -a  \
			\( "$ac_flex_version_minor" = 5 \) -a \
			\( "$ac_flex_version_sub"  -ge 9 \) ; then
			o3=yes
		else
			o3=false
		fi
			
		if test $o1 = yes -o $o2 = yes -o $o3 = yes ; then
			AC_MSG_RESULT([yes])
			AC_DEFINE(HAVE_YYLEX_DESTROY, 1, 
				[Define to 1 if flex generates yylex_destroy])
		else
			AC_MSG_RESULT([no])
		fi
	fi
])

dnl Checks if we have usable newt library. Sets $NEWT_LIBS variable. 
dnl AC_LIB_NEWT( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_LIB_NEWT], [
	ac_newt=yes
	NEWT_LIBS=""
	AC_CHECK_HEADER([newt.h], , ac_newt=no)
	if test $ac_newt = yes ; then
		ac_libs="$LIBS"
		AC_CHECK_LIB([newt], [newtInit], , [ac_newt=no])
		LIBS="$ac_libs"
	fi
	if test $ac_newt = yes ; then
		NEWT_LIBS="-lnewt"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
	AC_SUBST(NEWT_LIBS)
])

AC_DEFUN([AC_PATH_XMLRPC_EPI], [
	AC_ARG_WITH([xmprpc-epi-prefix],
		AC_HELP_STRING([--with-xmlrpc-epi-prefix=PREFIX], 
		[Prefix where xmlrpc-epi is installed (optional)]),
		xmlrpc_epi_config_prefix="$withval", )
	AC_ARG_WITH([xmlrpc-epi-library-name],
		AC_HELP_STRING([--with-xmlrpc-epi-library-name=NAME], 
		[Name of xmlrpc-epi library name(optional)]),
		xmlrpc_epi_library_name="$withval", )

	XMLRPC_EPI_CFLAGS=""
	XMLRPC_EPI_LIBS=""

	xmlrpc_epi_config_args_l=""
	xmlrpc_epi_config_args_i=""
	if test "x$xmlrpc_epi_config_prefix" != "x"; then
                xmlrpc_epi_config_args_l="-L $xmlrpc_epi_config_prefix/lib"
                xmlrpc_epi_config_args_i="-I $xmlrpc_epi_config_prefix/include"
	fi

	if test "x$xmlrpc_epi_library_name" = "x"; then
		xmlrpc_epi_library_name="xmlrpc-epi"
	fi

	AC_CHECK_LIB($xmlrpc_epi_library_name, XMLRPC_RequestNew, ac_xmlrpc_epi=yes, ac_xmlrpc_epi=no,
		[$xmlrpc_epi_config_args_l])

        xmlrpc_epi_config_args_l="$xmlrpc_epi_config_args_l -l$xmlrpc_epi_library_name"

	if test $ac_xmlrpc_epi = yes ; then
		XMLRPC_EPI_LIBS="$xmlrpc_epi_config_args_l -l$xmlrpc_epi_library_name"
		XMLRPC_EPI_CFLAGS="$xmlrpc_epi_config_args_i"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi


	AC_SUBST(XMLRPC_EPI_LIBS)
	AC_SUBST(XMLRPC_EPI_CFLAGS)

])

dnl Checks if we have usable OpenSSL library. Sets $SSL_LIBS and $SSL_CFLAGS variables. 
dnl AC_PATH_SSL( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_PATH_SSL], [
	AC_ARG_WITH([ssl-prefix],
		AC_HELP_STRING([--with-ssl-prefix=PREFIX], 
		[Prefix where OpenSSL is installed (optional)]),
		ssl_config_prefix="$withval", )
	
	AC_ARG_WITH([ssl-dll-eay-name],
		AC_HELP_STRING([--with-ssl-dll-eay-name=NAME], 
		[Name of OpenSSL eay dll(optional)]),
		ssl_dll_eay_config_name="$withval", )
	
	AC_ARG_WITH([ssl-dll-name],
		AC_HELP_STRING([--with-ssl-dll-name=NAME], 
		[Name of OpenSSL dll(optional)]),
		ssl_dll_config_name="$withval", )

	
	SSL_LIBS=""
	SSL_CFLAGS=""

	ssl_name=""
	if test "$host_os" = "mingw32" -o "$host_os" = "mingw32msvc"; then
		if test x$ssl_dll_config_name != x; then
			ssl_name=$ssl_dll_config_name
		else
			if test -f "$ssl_config_prefix/lib/libssl32.dll" -o -f "$ssl_config_prefix/lib/libssl32.a"; then
				ssl_name="ssl32"
			else
				ssl_name="ssleay32-0.9.8"
			fi
		fi
	else
		ssl_name="ssl"
	fi
	
	ssl_config_args_l=""
	ssl_config_args_i=""
	if test "x$ssl_config_prefix" != "x"; then
                ssl_config_args_l="-L $ssl_config_prefix/lib"
                ssl_config_args_i="-I$ssl_config_prefix/include"
	fi

	AC_CHECK_LIB($ssl_name, SSL_library_init, ac_ssl=yes, ac_ssl=no,
		[$ssl_config_args_l])

	if test $ac_ssl = yes ; then
		if test "$ssl_name" != "ssl"; then
			if test x$ssl_dll_eay_config_name != x; then
				ssl_eay_lib=$ssl_dll_eay_config_name
			else
				if test "$ssl_name" = "ssl32"; then
					ssl_eay_lib="eay32"
				else
					ssl_eay_lib="cryptoeay32-0.9.8"
				fi
			fi
			
			SSL_LIBS="$ssl_config_args_l -l$ssl_name -l$ssl_eay_lib"
		else
			SSL_LIBS="$ssl_config_args_l -l$ssl_name"
		fi
		SSL_CFLAGS="$ssl_config_args_i"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi

	AC_SUBST(SSL_LIBS)
	AC_SUBST(SSL_CFLAGS)
])

dnl Checks if we have usable c-ares library. Sets $CARES_LIBS and $CARES_CFLAGS variables. 
dnl AC_PATH_CARE( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_PATH_CARES], [
	AC_ARG_WITH([c-ares-prefix],
		AC_HELP_STRING([--with-c-ares-prefix=PREFIX], 
		[Prefix where c-ares is installed (optional)]),
		cares_config_prefix="$withval", )
	
	CARES_LIBS=""
	CARES_CFLAGS=""

	cares_config_args_l=""
	cares_config_args_i=""
	if test "x$cares_config_prefix" != "x"; then
                cares_config_args_l="-L $cares_config_prefix/lib"
                cares_config_args_i="-I$cares_config_prefix/include"
	fi

	AC_CHECK_LIB(cares, ares_init, ac_cares=yes, ac_cares=no,
		[$cares_config_args_l])

	if test $ac_cares = yes ; then
		CARES_LIBS="$cares_config_args_l -lcares"
		CARES_CFLAGS="$ssl_config_args_i"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi

	AC_SUBST(CARES_LIBS)
	AC_SUBST(CARES_CFLAGS)
])

dnl Checks if we have usable FREETYPE library. Sets $FREETYPE_LIBS and $FREETYPE_CFLAGS variables. 
dnl AC_PATH_FREETYPE( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_PATH_FREETYPE], [
	AC_ARG_WITH([freetype-prefix],
		AC_HELP_STRING([--with-freetype-prefix=PREFIX], 
		[Prefix where FREETYPE is installed (optional)]),
		freetype_prefix="$withval", )
	
	AC_ARG_WITH([freetype-library-name],
		AC_HELP_STRING([--with-freetype-library-name=NAME], 
		[Name of FREETYPE library(optional)]),
		freetype_library_name="$withval", )

	AC_ARG_WITH([freetype-include],
		AC_HELP_STRING([--with-freetype-include=PREFIX], 
		[Prefix where FREETYPE headers are installed (optional)]),
		freetype_include="$withval", )
	

	FREETYPE_LIBS=""
	FREETYPE_CFLAGS=""

	if test x$freetype_library_name != x; then 
		FREETYPE_LIBS="-l$freetype_library_name";
	fi

	if test x$freetype_prefix != x; then 
		FREETYPE_LIBS="-L ${freetype_prefix}/lib $FREETYPE_LIBS";
		FREETYPE_CFLAGS="-I ${freetype_prefix}/include";
	fi

	if test x$freetype_include != x; then 
		FREETYPE_CFLAGS="$FREETYPE_CFLAGS -I $freetype_include";
	fi
			
	AC_SUBST(FREETYPE_LIBS)
	AC_SUBST(FREETYPE_CFLAGS)

])

dnl Checks if we have usable OpenSSL library. Sets $SSL_LIBS and $SSL_CFLAGS variables. 
dnl AC_PATH_SSL( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_PATH_FTGL], [
	AC_ARG_WITH([ftgl-prefix],
		AC_HELP_STRING([--with-ftgl-prefix=PREFIX], 
		[Prefix where FTGL is installed (optional)]),
		ftgl_prefix="$withval", )
	
	AC_ARG_WITH([ftgl-library-name],
		AC_HELP_STRING([--with-ftgl-library-name=NAME], 
		[Name of FTGL library(optional)]),
		ftgl_library_name="$withval", )

	FTGL_LIBS=""
	FTGL_CFLAGS=""

	if test x$ftgl_library_name != x; then
		ftgl_name=$ftgl_library_name
	else
		ftgl_name="ftgl"
	fi

	no_ftgl="yes";
	if test "$host_os" = "mingw32" -o "$host_os" = "mingw32msvc"; then
		AC_MSG_CHECKING(for ftgl library)

		ftgl_gl_libs_l="-lglu32 -lopengl32"
		ftgl_lib_l="";
		ftgl_headers_i="";
		if test "x$ftgl_config_prefix" != "x"; then
			ftgl_lib_l="-L $ftgl_config_prefix/lib"
			ftgl_headers_i="-I$ftgl_config_prefix/include $ftgl_config_args_i"
		fi

				
		AC_LANG_PUSH(C++)
		AC_CHECK_LIB($ftgl_name, ftglCreateCustomFont, no_ftgl=no, no_ftgl=yes, [$ftgl_lib_l $ftgl_gl_libs_l $FREETYPE_LIBS])
		AC_LANG_POP(C++)
		if test x$no_ftgl = xno; then
			FTGL_LIBS="$ftgl_lib_l -l$ftgl_name $FREETYPE_LIBS";
			FTGL_CFLAGS="$ftgl_headers_location"
		fi
	else
		PKG_CHECK_MODULES(FTGL, $ftgl_name, no_ftgl="no", [AC_MSG_RESULT([
					FTGL library not found on your system,
					configuring without
				])
		])
	fi

	AC_SUBST(FTGL_LIBS)
	AC_SUBST(FTGL_CFLAGS)
	
	if test x$no_ftgl = xno; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi

])


dnl Checks if we have usable vmime library. Sets $VMIME_LIBS and $VMIME_CFLAGS variables. 
dnl AC_PATH_VMIME( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_PATH_VMIME], [
	AC_ARG_WITH([vmime-prefix],
		AC_HELP_STRING([--with-vmime-prefix=PREFIX], 
		[Prefix where vmime is installed (optional)]),
		vmime_config_prefix="$withval", )
	
	VMIME_LIBS=""
	VMIME_CFLAGS=""

	vmime_config_args_l=""
	vmime_config_args_i=""
	if test "x$vmime_config_prefix" != "x"; then
                vmime_config_args_l="-L $vmime_config_prefix/lib"
                vmime_config_args_i="-I$vmime_config_prefix/include"
	fi

	VMIME_CFLAGS=
	VMIME_LIBS=

	no_vmime="yes"

	AC_MSG_CHECKING(for vmime library)
	if test "$host_os" = "mingw32" -o "$host_os" = "mingw32msvc"; then
		CFLAGS=""
		CPPFLAGS=$vmime_config_args_i
		LIBS="$vmime_config_args_l -liconv"

		AC_LANG_PUSH(C++)
		AC_LINK_IFELSE([AC_LANG_SOURCE([[
#include <vmime/vmime.hpp>

int main(int argc, char *argv[]) {
	vmime::ref<vmime::message> msg = vmime::create<vmime::message>();
	return 0;
}

]])], [no_vmime=no], [no_vmime=yes])

		AC_LANG_POP(C++)

		if test "x$no_vmime" = "xno"; then
			VMIME_CFLAGS=$CPPFLAGS;
			VMIME_LIBS=$LIBS;
		fi

		CFLAGS="$ac_save_cflags"
		CPPFLAGS="$ac_save_cppflags"
		LIBS="$ac_save_libs"
	else
		pkg-config --exists vmime;
		if test $? -eq 0; then 
			VMIME_CFLAGS=`pkg-config --cflags vmime`
			VMIME_LIBS=`pkg-config --libs vmime`
			no_vmime=no
		else
			VMIME_CFLAGS=
			VMIME_LIBS=
			no_vmime=yes

		fi
	fi

	AC_SUBST(VMIME_CFLAGS)
	AC_SUBST(VMIME_LIBS)

	if test "x$no_vmime" = "xno"; then
		AC_MSG_RESULT( yes)
     		ifelse([$1], , :, [$1])
	else
		AC_MSG_RESULT( no)
		ifelse([$2], , :, [$2])
	fi

])

dnl Checks if we have usable librsync library. Sets $RSYNC_LIBS and $RSYNC_CFLAGS variables. 
dnl AC_PATH_RSYNC( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_PATH_RSYNC], [
	AC_ARG_WITH([rsync-prefix],
		AC_HELP_STRING([--with-rsync-prefix=PREFIX], 
		[Prefix where librsync is installed (optional)]),
		rsync_config_prefix="$withval", )

	RSYNC_LIBS=""
	RSYNC_CFLAGS=""

	rsync_config_args_l=""
	rsync_config_args_i=""
	if test "x$rsync_config_prefix" != "x"; then
                rsync_config_args_l="-L $rsync_config_prefix/lib"
                rsync_config_args_i="-I$rsync_config_prefix/include"
	fi

	AC_CHECK_LIB(rsync, rs_job_iter, ac_rsync=yes, ac_rsync=no,
		[$rsync_config_args_l])

	if test $ac_rsync = yes ; then
		RSYNC_LIBS="$rsync_config_args_l -lrsync"
		RSYNC_CFLAGS="$rsync_config_args_i"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi

	AC_SUBST(RSYNC_LIBS)
	AC_SUBST(RSYNC_CFLAGS)
])

dnl Checks if we have usable liblua library. Sets $LUA_LIBS and $LUA_CFLAGS variables. 
dnl AC_PATH_LUA( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_PATH_LUA], [
	AC_ARG_WITH([lua-prefix],
		AC_HELP_STRING([--with-lua-prefix=PREFIX], 
		[Prefix where liblua is installed (optional)]),
		lua_config_prefix="$withval", )

	LUA_LIBS=""
	LUA_CFLAGS=""

	lua_config_args_l=""
	lua_config_args_i=""
	if test "x$lua_config_prefix" != "x"; then
                lua_config_args_l="-L $lua_config_prefix/lib"
                lua_config_args_i="-I$lua_config_prefix/include"
	fi

	ac_lua=yes
	if test $ac_lua = yes ; then
		AC_CHECK_HEADER([luaconf.h], , ac_boost=no)
	fi
	if test $ac_lua = yes ; then
		AC_CHECK_LIB(lua, lua_call, ac_lua=yes, ac_lua=no,
			[$lua_config_args_l])
	fi

	if test $ac_lua = yes ; then
		LUA_LIBS="$lua_config_args_l -llua"
		LUA_CFLAGS="$lua_config_args_i"
		ifelse([$1], , :, [$1])
	else
		if pkg-config --exists lua5.1; then
			LUA_LIBS=`pkg-config --libs lua5.1`
			LUA_CFLAGS=`pkg-config --cflags lua5.1`
			ifelse([$1], , :, [$1])
		else
			ifelse([$2], , :, [$2])
		fi
	fi

	AC_SUBST(LUA_LIBS)
	AC_SUBST(LUA_CFLAGS)
])

dnl Checks if we have usable libldap library. Sets $LDAP_LIBS and $LDAP_CFLAGS variables. 
dnl AC_PATH_LDAP( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_PATH_LDAP], [
	AC_ARG_WITH([ldap-prefix],
		AC_HELP_STRING([--with-ldap-prefix=PREFIX], 
		[Prefix where libldap is installed (optional)]),
		ldap_config_prefix="$withval", )

	LDAP_LIBS=""
	LDAP_CFLAGS=""

	ldap_config_args_l=""
	ldap_config_args_i=""
	if test "x$ldap_config_prefix" != "x"; then
                ldap_config_args_l="-L $ldap_config_prefix/lib"
                ldap_config_args_i="-I$ldap_config_prefix/include"
	fi

	ac_ldap=yes
	if test $ac_ldap = yes ; then
		AC_CHECK_HEADER([ldap.h], , ac_boost=no)
	fi
	if test $ac_ldap = yes ; then
		AC_CHECK_LIB(ldap, ldap_initialize, ac_ldap=yes, ac_ldap=no,
			[$ldap_config_args_l])
	fi

	if test $ac_ldap = yes ; then
		LDAP_LIBS="$ldap_config_args_l -lldap"
		LDAP_CFLAGS="$ldap_config_args_i"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi

	AC_SUBST(LDAP_LIBS)
	AC_SUBST(LDAP_CFLAGS)
])

dnl check for base boost library presence
AC_DEFUN([AX_BOOST_BASE],
[
AC_ARG_WITH([boost],
        AS_HELP_STRING([--with-boost@<:@=DIR@:>@], [use boost (default is yes) - it is possible to specify the root directory for boost (optional)]),
        [
    if test "$withval" = "no"; then
                want_boost="no"
    elif test "$withval" = "yes"; then
        want_boost="yes"
        ac_boost_path=""
    else
            want_boost="yes"
        ac_boost_path="$withval"
        fi
    ],
    [want_boost="yes"])

if test "x$want_boost" = "xyes"; then
        boost_lib_version_req=1.20.0
        boost_lib_version_req_shorten=`expr $boost_lib_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
        boost_lib_version_req_major=`expr $boost_lib_version_req : '\([[0-9]]*\)'`
        boost_lib_version_req_minor=`expr $boost_lib_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        boost_lib_version_req_sub_minor=`expr $boost_lib_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$boost_lib_version_req_sub_minor" = "x" ; then
                boost_lib_version_req_sub_minor="0"
        fi
        WANT_BOOST_VERSION=`expr $boost_lib_version_req_major \* 100000 \+  $boost_lib_version_req_minor \* 100 \+ $boost_lib_version_req_sub_minor`
        AC_MSG_CHECKING(for boostlib >= $boost_lib_version_req)
        succeeded=no

        dnl first we check the system location for boost libraries
        dnl this location ist chosen if boost libraries are installed with the --layout=system option
        dnl or if you install boost with RPM
        if test "$ac_boost_path" != ""; then
                BOOST_LDFLAGS="-L$ac_boost_path/lib"
                BOOST_CPPFLAGS="-I$ac_boost_path/include"
        else
                for ac_boost_path_tmp in /usr /usr/local /opt /opt/local ; do
                        if test -d "$ac_boost_path_tmp/include/boost" && test -r "$ac_boost_path_tmp/include/boost"; then
                                BOOST_LDFLAGS="-L$ac_boost_path_tmp/lib"
                                BOOST_CPPFLAGS="-I$ac_boost_path_tmp/include"
                                break;
                        fi
                done
        fi

        CPPFLAGS_SAVED="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
        export CPPFLAGS

        LDFLAGS_SAVED="$LDFLAGS"
        LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
        export LDFLAGS

        AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        @%:@include <boost/version.hpp>
        ]], [[
        #if BOOST_VERSION >= $WANT_BOOST_VERSION
        // Everything is okay
        #else
        #  error Boost version is too old
        #endif
        ]])],[
        AC_MSG_RESULT(yes)
        succeeded=yes
        found_system=yes
        ],[
        ])
        AC_LANG_POP([C++])



        dnl if we found no boost with system layout we search for boost libraries
        dnl built and installed without the --layout=system option or for a staged(not installed) version
        if test "x$succeeded" != "xyes"; then
                _version=0
                if test "$ac_boost_path" != ""; then
                        BOOST_LDFLAGS="-L$ac_boost_path/lib"
                        if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
                                for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
                                        _version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                                        V_CHECK=`expr $_version_tmp \> $_version`
                                        if test "$V_CHECK" = "1" ; then
                                                _version=$_version_tmp
                                        fi
                                        VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                                        BOOST_CPPFLAGS="-I$ac_boost_path/include/boost-$VERSION_UNDERSCORE"
                                done
                        fi
                else
                        for ac_boost_path in /usr /usr/local /opt /opt/local ; do
                                if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
                                        for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
                                                _version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                                                V_CHECK=`expr $_version_tmp \> $_version`
                                                if test "$V_CHECK" = "1" ; then
                                                        _version=$_version_tmp
                                                        best_path=$ac_boost_path
                                                fi
                                        done
                                fi
                        done

                        VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                        BOOST_CPPFLAGS="-I$best_path/include/boost-$VERSION_UNDERSCORE"
                        BOOST_LDFLAGS="-L$best_path/lib"

                        if test "x$BOOST_ROOT" != "x"; then
                                if test -d "$BOOST_ROOT" && test -r "$BOOST_ROOT" && test -d "$BOOST_ROOT/stage/lib" && test -r "$BOOST_ROOT/stage/lib"; then
                                        version_dir=`expr //$BOOST_ROOT : '.*/\(.*\)'`
                                        stage_version=`echo $version_dir | sed 's/boost_//' | sed 's/_/./g'`
                                        stage_version_shorten=`expr $stage_version : '\([[0-9]]*\.[[0-9]]*\)'`
                                        V_CHECK=`expr $stage_version_shorten \>\= $_version`
                                        if test "$V_CHECK" = "1" ; then
                                                AC_MSG_NOTICE(We will use a staged boost library from $BOOST_ROOT)
                                                BOOST_CPPFLAGS="-I$BOOST_ROOT"
                                                BOOST_LDFLAGS="-L$BOOST_ROOT/stage/lib"
                                        fi
                                fi
                        fi
                fi

                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

                AC_LANG_PUSH(C++)
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
                @%:@include <boost/version.hpp>
                ]], [[
                #if BOOST_VERSION >= $WANT_BOOST_VERSION
                // Everything is okay
                #else
                #  error Boost version is too old
                #endif
                ]])],[
                AC_MSG_RESULT(yes)
                succeeded=yes
                found_system=yes
                ],[
                ])
                AC_LANG_POP([C++])
        fi

        if test "$succeeded" != "yes" ; then
                if test "$_version" = "0" ; then
                        AC_MSG_WARN([[We could not detect the boost libraries (version $boost_lib_version_req_shorten or higher). If you have a staged boost library (still not installed) please specify \$BOOST_ROOT in your environment and do not give a PATH to --with-boost option.  If you are sure you have boost installed, then check your version number looking in <boost/version.hpp>. See http://randspringer.de/boost for more documentation.]])
                else
                        AC_MSG_WARN([Your boost libraries seems to old (version $_version).])
                fi
		ifelse([$2], , :, [$2])
        else
		ifelse([$1], , :, [$1])
                AC_SUBST(BOOST_CPPFLAGS)
                AC_SUBST(BOOST_LDFLAGS)
                AC_DEFINE(HAVE_BOOST, 1,[define if the Boost library is available])
        fi

        CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
fi

])

dnl checks for boost date time library

AC_DEFUN([AX_BOOST_DATE_TIME],
[
        AC_ARG_WITH([boost-date-time],
        AS_HELP_STRING([--with-boost-date-time@<:@=special-lib@:>@],
                   [use the Date_Time library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-date-time=boost_date_time-gcc-mt-d-1_33_1 ]),
        [
        if test "$withval" = "no"; then
		want_boost="no"
        elif test "$withval" = "yes"; then
		want_boost="yes"
		ax_boost_user_date_time_lib=""
        else
		want_boost="yes"
		ax_boost_user_date_time_lib="$withval"
	fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
	        AC_REQUIRE([AC_PROG_CC])
		CPPFLAGS_SAVED="$CPPFLAGS"
       		CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
		export CPPFLAGS

       		LDFLAGS_SAVED="$LDFLAGS"
		LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
		export LDFLAGS

	        AC_CACHE_CHECK(whether the Boost::Date_Time library is available,
       	                                    ax_cv_boost_date_time,
			[AC_LANG_PUSH([C++])
				AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/date_time/gregorian/gregorian_types.hpp>]],
					[[	
						using namespace boost::gregorian; date d(2002,Jan,10);
						return 0;
					]]),
				ax_cv_boost_date_time=yes, ax_cv_boost_date_time=no)
				AC_LANG_POP([C++])
			]
		)

		if test "x$ax_cv_boost_date_time" = "xyes"; then
			AC_DEFINE(HAVE_BOOST_DATE_TIME, 1, [define if the Boost::Date_Time library is available])
			BN_BOOST_DATE_TIME_LIB=boost_date_time
			BOOSTLIBDIR=`echo $BOOST_LDFLAGS | sed -e 's/@<:@^\/@:>@*//'`
			if test "x$ax_boost_user_date_time_lib" = "x"; then
				for libextension in `ls $BOOSTLIBDIR/libboost_date_time*.{so,a}* | sed 's,.*/,,' | sed -e 's;^libboost_date_time\(.*\)\.so.*$;\1;' -e 's;^libboost_date_time\(.*\)\.a*$;\1;'` ; do
					ax_lib=${BN_BOOST_DATE_TIME_LIB}${libextension}
					AC_CHECK_LIB($ax_lib, exit,
						[BOOST_DATE_TIME_LIB="-l$ax_lib"; AC_SUBST(BOOST_DATE_TIME_LIB) link_date_time="yes"; break],
						[link_date_time="no"])
				done

			else
				for ax_lib in $ax_boost_user_date_time_lib $BN_BOOST_DATE_TIME_LIB-$ax_boost_user_date_time_lib; do
					AC_CHECK_LIB($ax_lib, main,
						[BOOST_DATE_TIME_LIB="-l$ax_lib"; AC_SUBST(BOOST_DATE_TIME_LIB) link_date_time="yes"; break],
						[link_date_time="no"])
				done

			fi

			if test "x$link_date_time" = "xno"; then
				AC_MSG_WARN(Boost date time library not found)
				ifelse([$2], , :, [$2])
			else
				ifelse([$1], , :, [$1])
			fi
		else
			ifelse([$2], , :, [$2])
		fi

		CPPFLAGS="$CPPFLAGS_SAVED"
		LDFLAGS="$LDFLAGS_SAVED"

	else
		ifelse([$2], , :, [$2])
        fi
])

dnl checks for boost filesystem library
AC_DEFUN([AX_BOOST_FILESYSTEM],
[
        AC_ARG_WITH([boost-filesystem],
        AS_HELP_STRING([--with-boost-filesystem@<:@=special-lib@:>@],
                   [use the Filesystem library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-filesystem=boost_filesystem-gcc-mt ]),
        [
        if test "$withval" = "no"; then
		want_boost="no"
        elif test "$withval" = "yes"; then
		want_boost="yes"
		ax_boost_user_filesystem_lib=""
        else
		want_boost="yes"
                ax_boost_user_filesystem_lib="$withval"
	fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
	        AC_REQUIRE([AC_PROG_CC])
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

	        AC_CACHE_CHECK(whether the Boost::Filesystem library is available,
       	                                    ax_cv_boost_filesystem,
		        [AC_LANG_PUSH([C++])
				AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/filesystem/path.hpp>]],
					[[
					using namespace boost::filesystem;
					path my_path( "foo/bar/data.txt" );
					return 0;
				]]),
				ax_cv_boost_filesystem=yes, ax_cv_boost_filesystem=no)
				AC_LANG_POP([C++])
                	]
		)

                if test "x$ax_cv_boost_filesystem" = "xyes"; then
			AC_DEFINE(HAVE_BOOST_FILESYSTEM, 1, [define if the Boost::Filesystem library is available])
			BN_BOOSTFILESYSTEM_LIB=boost_filesystem
			BOOSTLIBDIR=`echo $BOOST_LDFLAGS | sed -e 's/@<:@^\/@:>@*//'`
			if test "x$ax_boost_user_filesystem_lib" = "x"; then
		                for libextension in `ls $BOOSTLIBDIR/libboost_filesystem*.{so,a}* | sed 's,.*/,,' | sed -e 's;^libboost_filesystem\(.*\)\.so.*$;\1;' -e 's;^libboost_filesystem\(.*\)\.a*$;\1;'` ; do
					ax_lib=${BN_BOOSTFILESYSTEM_LIB}${libextension}
					AC_CHECK_LIB($ax_lib, exit,
						[BOOST_FILESYSTEM_LIB="-l$ax_lib"; AC_SUBST(BOOST_FILESYSTEM_LIB) link_filesystem="yes"; break],
						[link_filesystem="no"])
				done
			else
				for ax_lib in $ax_boost_user_filesystem_lib $BN_BOOSTFILESYSTEM_LIB-$ax_boost_user_filesystem_lib; do
					AC_CHECK_LIB($ax_lib, exit,
					[BOOST_FILESYSTEM_LIB="-l$ax_lib"; AC_SUBST(BOOST_FILESYSTEM_LIB) link_filesystem="yes"; break],
					[link_filesystem="no"])
				done
			fi
                        if test "x$link_filesystem" = "xno"; then
                                AC_MSG_ERROR(Library boost::filesytem not found!)
				ifelse([$2], , :, [$2])
			else
				ifelse([$1], , :, [$1])
                        fi
		else
			ifelse([$2], , :, [$2])
                fi

		CPPFLAGS="$CPPFLAGS_SAVED"
		LDFLAGS="$LDFLAGS_SAVED"
	else
		ifelse([$2], , :, [$2])

        fi
])

AC_DEFUN([AX_BOOST_THREAD],
[
        AC_ARG_WITH([boost-thread],
        AS_HELP_STRING([--with-boost-thread@<:@=special-lib@:>@],
                   [use the Thread library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-thread=boost_thread-gcc-mt ]),
        [
        if test "$withval" = "no"; then
                        want_boost="no"
        elif test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_thread_lib=""
        else
                    want_boost="yes"
                ax_boost_user_thread_lib="$withval"
                fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
		AC_REQUIRE([AC_PROG_CC])
		AC_REQUIRE([AC_CANONICAL_BUILD])
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

        	AC_CACHE_CHECK(whether the Boost::Thread library is available,
                                           ax_cv_boost_thread,
			[
				AC_LANG_PUSH([C++])
				CXXFLAGS_SAVE=$CXXFLAGS

				if test "x$build_os" = "xsolaris" ; then
					CXXFLAGS="-pthreads $CXXFLAGS"
				elif test "x$build_os" = "xmingw32" ; then
					CXXFLAGS="-mthreads $CXXFLAGS"
				else
                                	CXXFLAGS="-pthread $CXXFLAGS"
				fi
				AC_COMPILE_IFELSE(AC_LANG_PROGRAM(
					[
						[@%:@include <boost/thread/thread.hpp>]],
						[[boost::thread_group thrds;
	                                   	return 0;]
					]),
					ax_cv_boost_thread=yes, 
					ax_cv_boost_thread=no)
				CXXFLAGS=$CXXFLAGS_SAVE
				AC_LANG_POP([C++])
			]
		)
                if test "x$ax_cv_boost_thread" = "xyes"; then
			if test "x$build_os" = "xsolaris" ; then
				BOOST_CPPFLAGS="-pthreads $BOOST_CPPFLAGS"
			elif test "x$build_os" = "xmingw32" ; then
				BOOST_CPPFLAGS="-mthreads $BOOST_CPPFLAGS"
			else
				BOOST_CPPFLAGS="-pthread $BOOST_CPPFLAGS"
			fi

                        AC_SUBST(BOOST_CPPFLAGS)

                        AC_DEFINE(HAVE_BOOST_THREAD,,[define if the Boost::Thread library is available])
                        BN=boost_thread
                        LDFLAGS_SAVE=$LDFLAGS
                        case "x$build_os" in
				*bsd* )
					LDFLAGS="-pthread $LDFLAGS"
					break;
					;;
			esac
			if test "x$ax_boost_user_thread_lib" = "x"; then
				for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
					lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
					$BN-mt $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
					AC_CHECK_LIB(
						$ax_lib, 
						main, 
						[
							BOOST_THREAD_LIB="-l$ax_lib" 
							AC_SUBST(BOOST_THREAD_LIB) 
							link_thread="yes" 
							break
						],
                                 		[link_thread="no"]
					)
                                done
			else
				for ax_lib in $ax_boost_user_thread_lib $BN-$ax_boost_user_thread_lib; do
					AC_CHECK_LIB(
						$ax_lib, 
						main,
						[
							BOOST_THREAD_LIB="-l$ax_lib" 
							AC_SUBST(BOOST_THREAD_LIB) 
							link_thread="yes" 
							break
						],
						[link_thread="no"]
					)
				done
			fi
			if test "x$link_thread" = "xno"; then
				AC_MSG_ERROR(Could not link against $ax_lib !)
				ifelse([$2], , :, [$2])
			else
				case "x$build_os" in
					*bsd* )
						BOOST_LDFLAGS="-pthread $BOOST_LDFLAGS"
						break;
						;;
				esac				
				ifelse([$1], , :, [$1])
                        fi
		else
			ifelse([$2], , :, [$2])
                fi
                CPPFLAGS="$CPPFLAGS_SAVED"
		LDFLAGS="$LDFLAGS_SAVED"
        fi
])

dnl checks for boost regex library
AC_DEFUN([AX_BOOST_REGEX],
[
        AC_ARG_WITH([boost-regex],
        AS_HELP_STRING([--with-boost-regex@<:@=special-lib@:>@],
                   [use the Regex library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-regex=boost_regex-gcc-mt-d-1_33_1 ]),
        [
        if test "$withval" = "no"; then
                        want_boost="no"
        elif test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_regex_lib=""
        else
                    want_boost="yes"
                ax_boost_user_regex_lib="$withval"
                fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

        AC_CACHE_CHECK(whether the Boost::Regex library is available,
                                           ax_cv_boost_regex,
        [AC_LANG_PUSH([C++])
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/regex.hpp>
                                                                                                ]],
                                   [[boost::regex r(); return 0;]]),
                   ax_cv_boost_regex=yes, ax_cv_boost_regex=no)
         AC_LANG_POP([C++])
                ])
                if test "x$ax_cv_boost_regex" = "xyes"; then
                        AC_DEFINE(HAVE_BOOST_REGEX,,[define if the Boost::Regex library is available])
                        BN_BOOST_REGEX=boost_regex
            BOOSTLIBDIR=`echo $BOOST_LDFLAGS | sed -e 's/@<:@^\/@:>@*//'`
            if test "x$ax_boost_user_regex_lib" = "x"; then
                for libextension in `ls $BOOSTLIBDIR/libboost_regex*.{so,a}* | sed 's,.*/,,' | sed -e 's;^libboost_regex\(.*\)\.so.*$;\1;' -e 's;^libboost_regex\(.*\)\.a*$;\1;'` ; do
                     ax_lib=${BN_BOOST_REGEX}${libextension}
                                    AC_CHECK_LIB($ax_lib, exit,
                                 [BOOST_REGEX_LIB="-l$ax_lib"; AC_SUBST(BOOST_REGEX_LIB) link_regex="yes"; break],
                                 [link_regex="no"])
                                done
            else
               for ax_lib in $ax_boost_user_regex_lib $BN_BOOST_REGEX-$ax_boost_user_regex_lib; do
                                      AC_CHECK_LIB($ax_lib, main,
                                   [BOOST_REGEX_LIB="-l$ax_lib"; AC_SUBST(BOOST_REGEX_LIB) link_regex="yes"; break],
                                   [link_regex="no"])
               done
            fi
                        if test "x$link_regex" = "xno"; then
                                AC_MSG_ERROR(Could not link against $ax_lib !)
				ifelse([$2], , :, [$2])
			else
				ifelse([$1], , :, [$1])
                        fi
		else
			ifelse([$2], , :, [$2])
                fi

                CPPFLAGS="$CPPFLAGS_SAVED"
                LDFLAGS="$LDFLAGS_SAVED"
	else
		ifelse([$2], , :, [$2])

        fi
])

dnl checks for boost program options library
AC_DEFUN([AX_BOOST_PROGRAM_OPTIONS],
[
        AC_ARG_WITH([boost-program-options],
                AS_HELP_STRING([--with-boost-program-options@<:@=special-lib@:>@],
                       [use the program options library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-program-options=boost_program_options-gcc-mt-1_33_1 ]),
        [
        if test "$withval" = "no"; then
                        want_boost="no"
        elif test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_program_options_lib=""
        else
                    want_boost="yes"
                ax_boost_user_program_options_lib="$withval"
                fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
            export want_boost
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS
                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS
                AC_CACHE_CHECK([whether the Boost::Program_Options library is available],
                                           ax_cv_boost_program_options,
                                           [AC_LANG_PUSH(C++)
                                AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/program_options.hpp>
                                                          ]],
                                  [[boost::program_options::options_description generic("Generic options");
                                   return 0;]]),
                           ax_cv_boost_program_options=yes, ax_cv_boost_program_options=no)
                                        AC_LANG_POP([C++])
                ])
                if test "$ax_cv_boost_program_options" = yes; then
                                AC_DEFINE(HAVE_BOOST_PROGRAM_OPTIONS,,[define if the Boost::PROGRAM_OPTIONS library is available])
                                  BN_BOOST_PROGRAM_OPTIONS_LIB=boost_program_options
                  BOOSTLIBDIR=`echo $BOOST_LDFLAGS | sed -e 's/@<:@^\/@:>@*//'`
                if test "x$ax_boost_user_program_options_lib" = "x"; then
                for libextension in `ls $BOOSTLIBDIR/libboost_program_options*.{so,a}* | sed 's,.*/,,' | sed -e 's;^libboost_program_options\(.*\)\.so.*$;\1;' -e 's;^libboost_program_options\(.*\)\.a*$;\1;'` ; do
                     ax_lib=${BN_BOOST_PROGRAM_OPTIONS_LIB}${libextension}
                                    AC_CHECK_LIB($ax_lib, exit,
                                 [BOOST_PROGRAM_OPTIONS_LIB="-l$ax_lib"; AC_SUBST(BOOST_PROGRAM_OPTIONS_LIB) link_program_options="yes"; break],
                                 [link_program_options="no"])
                                done

                else
                  for ax_lib in $ax_boost_user_program_options_lib $BN_BOOST_PROGRAM_OPTIONS_LIB-$ax_boost_user_program_options_lib; do
                                      AC_CHECK_LIB($ax_lib, main,
                                   [BOOST_PROGRAM_OPTIONS_LIB="-l$ax_lib"; AC_SUBST(BOOST_PROGRAM_OPTIONS_LIB) link_program_options="yes"; break],
                                   [link_program_options="no"])
                  done
                fi
                                if test "x$link_program_options" = "xno"; then
                                        AC_MSG_ERROR([Could not link against [$ax_lib] !])
				ifelse([$2], , :, [$2])
			else
				ifelse([$1], , :, [$1])
                        fi
		else
			ifelse([$2], , :, [$2])
                fi
                CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
	else
		ifelse([$2], , :, [$2])
        fi
])

dnl Checks if we have usable PAM library. Sets $PAM_LIBS variable. 
dnl AC_LIB_PAM( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_LIB_PAM], [
	ac_pam=yes
	PAM_LIBS=""
	AC_CHECK_HEADER([security/pam_appl.h], , ac_pam=no)
	if test $ac_pam = yes ; then
		ac_libs="$LIBS"
		AC_CHECK_LIB([pam], [pam_start], , [ac_pam=no])
		LIBS="$ac_libs"
	fi
	if test $ac_pam = yes ; then
		PAM_LIBS="-lpam"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi
	AC_SUBST(PAM_LIBS)
])


dnl Adapted from libxml2 m4 macros.

dnl AM_PATH_XSLT([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for XSLT, and define XSLT_CFLAGS and XSLT_LIBS
dnl Should be called after AM_PATH_XML2 and only if libxml2 is available.
dnl
AC_DEFUN([AM_PATH_XSLT],[
xslt_config_prefix=""
xslt_config_exec_prefix=""
AC_ARG_WITH(xslt-prefix,
            [  --with-xslt-prefix=PREFIX   Prefix where libxslt is installed (optional)],
            xslt_config_prefix="$withval", )
AC_ARG_WITH(xslt-exec-prefix,
            [  --with-xslt-exec-prefix=PREFIX Exec prefix where libxslt is installed (optional)],
            xslt_config_exec_prefix="$withval", )
AC_ARG_ENABLE(xslttest,
              [  --disable-xsltest       Do not try to compile and run a test LIBXSLT program],,
              enable_xsltest=yes)

  if test x$xslt_config_exec_prefix != x ; then
     xslt_config_args="$xslt_config_args --exec-prefix=$xslt_config_exec_prefix"
     if test x${XSLT2_CONFIG+set} != xset ; then
        XSLT2_CONFIG=$xslt_config_exec_prefix/bin/xslt-config
     fi
  fi
  if test x$xslt_config_prefix != x ; then
     xslt_config_args="$xslt_config_args --prefix=$xslt_config_prefix"
     if test x${XSLT_CONFIG+set} != xset ; then
        XSLT_CONFIG=$xslt_config_prefix/bin/xslt-config
     fi
  fi

  AC_PATH_TOOL(XSLT_CONFIG, xslt-config, no)
  min_xslt_version=ifelse([$1], ,1.0.17,[$1])
  AC_MSG_CHECKING(for libxslt - version >= $min_xslt_version)
  no_xslt=""
  if test "$XSLT_CONFIG" = "no" ; then
    no_xslt=yes
  else
    XSLT_CFLAGS=`$XSLT_CONFIG $xslt_config_args --cflags`
    XSLT_LIBS=`$XSLT_CONFIG $xslt_config_args --libs`
    xslt_config_major_version=`$XSLT_CONFIG $xslt_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    xslt_config_minor_version=`$XSLT_CONFIG $xslt_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    xslt_config_micro_version=`$XSLT_CONFIG $xslt_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_xsltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $XML_FLAGS $XSLT_CFLAGS"
      LIBS="$XSLT_LIBS $XML_LIBS $LIBS"
dnl
dnl Now check if the installed libxslt is sufficiently new.
dnl (Also sanity checks the results of xslt-config to some extent)
dnl
      rm -f conf.xslttest
      AC_TRY_RUN([
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxslt/xsltconfig.h>

int 
main()
{
  int xslt_major_version, xslt_minor_version, xslt_micro_version;
  int major, minor, micro;
  char *tmp_version;

  system("touch conf.xslttest");

  /* Capture xslt-config output via autoconf/configure variables */
  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = (char *)strdup("$min_xslt_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string from xslt-config\n", "$min_xslt_version");
     exit(1);
   }
   free(tmp_version);

   /* Capture the version information from the header files */
   tmp_version = (char *)strdup(LIBXSLT_DOTTED_VERSION);
   if (sscanf(tmp_version, "%d.%d.%d", &xslt_major_version, &xslt_minor_version, &xslt_micro_version) != 3) {
     printf("%s, bad version string from libxslt includes\n", "LIBXSLT_DOTTED_VERSION");
     exit(1);
   }
   free(tmp_version);

 /* Compare xslt-config output to the libxslt headers */
  if ((xslt_major_version != $xslt_config_major_version) ||
      (xslt_minor_version != $xslt_config_minor_version) ||
      (xslt_micro_version != $xslt_config_micro_version))
    {
      printf("*** libxslt header files (version %d.%d.%d) do not match\n",
         xslt_major_version, xslt_minor_version, xslt_micro_version);
      printf("*** xslt-config (version %d.%d.%d)\n",
         $xslt_config_major_version, $xslt_config_minor_version, $xslt_config_micro_version);
      return 1;
    } 

    /* Test that the library is greater than our minimum version */
    if ((xslt_major_version > major) ||
        ((xslt_major_version == major) && (xslt_minor_version > minor)) ||
        ((xslt_major_version == major) && (xslt_minor_version == minor) &&
        (xslt_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of libxslt (%d.%d.%d) was found.\n",
               xslt_major_version, xslt_minor_version, xslt_micro_version);
        printf("*** You need a version of libxslt newer than %d.%d.%d. The latest version of\n",
           major, minor, micro);
        printf("*** libxslt is always available from ftp://ftp.xmlsoft.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the xslt-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of LIBXSLT, but you can also set the XSLT_CONFIG environment to point to the\n");
        printf("*** correct copy of xslt-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
    }
  return 1;
}
],, no_xslt=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi

  if test "x$no_xslt" = x ; then
     AC_MSG_RESULT(yes (version $xslt_config_major_version.$xslt_config_minor_version.$xslt_config_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$XSLT_CONFIG" = "no" ; then
       echo "*** The xslt-config script installed by LIBXSLT could not be found"
       echo "*** If libxslt was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the XSLT_CONFIG environment variable to the"
       echo "*** full path to xslt-config."
     else
       if test -f conf.xslttest ; then
        :
       else
          echo "*** Could not run libxslt test program, checking why..."
          CFLAGS="$CFLAGS $XML_CFLAGS $XSLT_CFLAGS"
          LIBS="$LIBS $XML_LIBS $XSLT_LIBS"
          AC_TRY_LINK([
#include <libxslt/xsltconfig.h>
#include <stdio.h>
],      [ LIBXSLT_TEST_VERSION; return 0;],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding LIBXSLT or finding the wrong"
          echo "*** version of LIBXSLT. If it is not finding LIBXSLT, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
          echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means LIBXSLT was incorrectly installed"
          echo "*** or that you have moved LIBXSLT since it was installed. In the latter case, you"
          echo "*** may want to edit the xslt-config script: $XSLT_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi

     XSLT_CFLAGS=""
     XSLT_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(XSLT_CFLAGS)
  AC_SUBST(XSLT_LIBS)
  rm -f conf.xslttest
])

###############################################################################################
# http://autoconf-archive.cryp.to/acx_pthread.html

# Author: Steven G. Johnson <stevenj@alum.mit.edu>

AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
acx_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all, and "pthread-config"
# which is a program returning the flags for the Pth emulation library.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt pthread-config"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
# pthread: Linux, etcetera
# --thread-safe: KAI C++
# pthread-config: use pthread-config program (for GNU Pth library)

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthread or
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthread -pthreads pthread -mt $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

                pthread-config)
                AC_CHECK_PROG(acx_pthread_config, pthread-config, yes, no)
                if test x"$acx_pthread_config" = xno; then continue; fi
                PTHREAD_CFLAGS="`pthread-config --cflags`"
                PTHREAD_LIBS="`pthread-config --ldflags` `pthread-config --libs`"
                ;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: JOINABLE attribute is called UNDETACHED.
        AC_MSG_CHECKING([for joinable pthread attribute])
        attr_name=unknown
        for attr in PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_UNDETACHED; do
            AC_TRY_LINK([#include <pthread.h>], [int attr=$attr;],
                        [attr_name=$attr; break])
        done
        AC_MSG_RESULT($attr_name)
        if test "$attr_name" != PTHREAD_CREATE_JOINABLE; then
            AC_DEFINE_UNQUOTED(PTHREAD_CREATE_JOINABLE, $attr_name,
                               [Define to necessary symbol if this constant
                                uses a non-standard name on your system.])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
            *-aix* | *-freebsd* | *-darwin*) flag="-D_THREAD_SAFE";;
            *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
            PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with cc_r
        AC_CHECK_PROG(PTHREAD_CC, cc_r, cc_r, ${CC})
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl ACX_PTHREAD


dnl AM_PATH_CURL([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for curl, and defines:
dnl CURL_CFLAGS 
dnl CURL_LIBS
dnl CURL_SSL ('yes' if curl is builded with SSL support)
dnl CURL_VERSION
dnl Config variables:
dnl NO_CURL - defined to 1 if curl library is not available
dnl CURL_SSL - defined to 1 if curl library is not available
dnl
AC_DEFUN([AM_PATH_CURL],[
	if test "x$host" != "x$build" ; then
        	curl_config_prefix="/usr/$host"
	else
        	curl_config_prefix=""
	fi
	AC_ARG_WITH(curl-prefix,
            [  --with-curl-prefix=PREFIX   Prefix where curl library is installed (optional)],
            curl_config_prefix="$withval", )

	if test x$curl_config_prefix != x ; then
		if test x${CURL_CONFIG+set} != xset ; then
			CURL_CONFIG=$curl_config_prefix/bin/curl-config
		fi
	fi
	AC_PATH_TOOL(CURL_CONFIG, curl-config, no)
	AC_MSG_CHECKING(for curl library)
	no_curl=""
	if test "$CURL_CONFIG" = "no" -o ! -x $CURL_CONFIG ; then
		no_curl=yes
	else
		CURL_VERSION=`$CURL_CONFIG --version`
		CURL_CFLAGS=`$CURL_CONFIG --cflags`
		CURL_LIBS=`$CURL_CONFIG --libs`
	fi
	if test "x$no_curl" = x ; then
		AC_MSG_RESULT(yes (version $CURL_VERSION))
	else
		AC_MSG_RESULT(no)
		echo "*** The curl-config script could not be found"
		echo "*** If libcurl was installed in PREFIX, make sure PREFIX/bin is in"
		echo "*** your path, or set the CURL_CONFIG environment variable to the"
		echo "*** full path to curl-config."
     		ifelse([$2], , :, [$2])
     		CURL_CFLAGS=""
		CURL_LIBS=""
		CURL_SSL=""
		CURL_VERSION=""
	fi

	if test "x$no_curl" = x ; then
		AC_MSG_CHECKING(for curl http support)
		if $CURL_CONFIG --protocols | grep HTTP &> /dev/null ; then
			AC_MSG_RESULT(yes)
		else
			AC_MSG_RESULT(no, marking curl as unusable)
			no_curl=yes
		fi
	fi

	if test "x$no_curl" = x ; then
		AC_MSG_CHECKING(for curl SSL support)
		if $CURL_CONFIG --features | grep SSL &> /dev/null && \
			$CURL_CONFIG --protocols | grep HTTPS &> /dev/null ; then
			AC_MSG_RESULT(yes)
			CURL_SSL=yes
			AC_DEFINE(CURL_SSL, 1, [Define to 1 if curl library has SSL support])
		else
			AC_MSG_RESULT(no)
			CURL_SSL=
		fi
	fi

	if test "x$no_curl" = x ; then
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
     		CURL_CFLAGS=""
		CURL_LIBS=""
		CURL_SSL=""
		CURL_VERSION=""
		AC_DEFINE(NO_CURL, 1, [Define to 1 if curl library is not available])
	fi
	

	AC_SUBST(CURL_CFLAGS)
	AC_SUBST(CURL_LIBS)
	AC_SUBST(CURL_VERSION)
	AC_SUBST(CURL_SSL)
]) dnl AM_PATH_CURL



AC_DEFUN([AC_CHECK_ICU], [
  succeeded=no

  if test -z "$ICU_CONFIG"; then
    AC_PATH_PROG(ICU_CONFIG, icu-config, no)
  fi

  if test "$ICU_CONFIG" = "no" ; then
    echo "*** The icu-config script could not be found. Make sure it is"
    echo "*** in your path, and that taglib is properly installed."
    echo "*** Or see http://ibm.com/software/globalization/icu/"
  else
    ICU_VERSION=`$ICU_CONFIG --version`
    AC_MSG_CHECKING(for ICU >= $1)
        VERSION_CHECK=`expr $ICU_VERSION \>\= $1`
        if test "$VERSION_CHECK" = "1" ; then
            AC_MSG_RESULT(yes)
            succeeded=yes

            AC_MSG_CHECKING(ICU_CFLAGS)
            ICU_CFLAGS=`$ICU_CONFIG --cflags`
            AC_MSG_RESULT($ICU_CFLAGS)

            AC_MSG_CHECKING(ICU_CXXFLAGS)
            ICU_CXXFLAGS=`$ICU_CONFIG --cxxflags`
            AC_MSG_RESULT($ICU_CXXFLAGS)

            AC_MSG_CHECKING(ICU_LIBS)
            ICU_LIBS=`$ICU_CONFIG --ldflags`
            AC_MSG_RESULT($ICU_LIBS)
        else
            ICU_CFLAGS=""
            ICU_CXXFLAGS=""
            ICU_LIBS=""
            ## If we have a custom action on failure, don't print errors, but
            ## do set a variable so people can do so.
            ifelse([$3], ,echo "can't find ICU >= $1",)
        fi

        AC_SUBST(ICU_CFLAGS)
        AC_SUBST(ICU_CXXFLAGS)
        AC_SUBST(ICU_LIBS)
  fi

  if test $succeeded = yes; then
     ifelse([$2], , :, [$2])
  else
     ifelse([$3], , :, [$3])
  fi
])



