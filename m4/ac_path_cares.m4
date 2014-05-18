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
                cares_config_args_l="-L$cares_config_prefix/lib"
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

