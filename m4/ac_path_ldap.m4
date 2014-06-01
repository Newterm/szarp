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
                ldap_config_args_l="-L$ldap_config_prefix/lib"
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

