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

