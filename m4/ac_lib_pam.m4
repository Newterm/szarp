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

