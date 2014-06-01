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

