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
                rsync_config_args_l="-L$rsync_config_prefix/lib"
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

