dnl Checks if we have usable cgal library. Sets $CGAL_LIBS variable. 
dnl AC_LIB_CGAL( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_LIB_CGAL], [
	AC_ARG_WITH([cgal-prefix],
		AC_HELP_STRING([--with-cgal-prefix=PREFIX], 
		[Prefix where CGAL is installed (optional)]),
		cgal_config_prefix="$withval", )
	
	AC_MSG_CHECKING(CGAL)

        CPPFLAGS_SAVED="$CPPFLAGS"
	LIBS_SAVED="$LIBS"
	cgal_prefix_l=""
	cgal_prefix_i=""
	if test "x$cgal_config_prefix" != "x"; then
		cgal_prefix_i="-I$cgal_config_prefix"
		cgal_prefix_l="-L$cgal_config_prefix"
	fi
        CPPFLAGS="$CPPFLAGS $cgal_prefix_i"
        LIBS="$LIBS $cgal_prefix_l"

	CGAL_LIBS=""
	CGAL_CFLAGS=""

	AC_LANG_PUSH(C++)

	AC_CHECK_HEADER(CGAL/Exact_predicates_inexact_constructions_kernel.h, cgal_have_header=yes, cgal_have_header=no)
	if test "$cgal_have_header" == yes; then
		AC_CHECK_LIB(CGAL, main, cgal_have_lib=yes, cgal_have_lib=no)
		if test "$cgal_have_lib" == no; then
			LIBS="$LIBS -lgmp -lmpfr -lm"
        		AC_CHECK_LIB(CGAL, main, [CGAL_LIBS="-lCGAL -lgmp -lmpfr"
				cgal_have_lib=yes], cgal_have_lib=no)
		fi	
		if test "$cgal_have_lib" == yes; then 
			acx_cgal_found=yes
		fi
	fi	

	CPPFLAGS="$CPPFLAGS_SAVED"
	LIBS="$LIBS_SAVED"

	if test x$cgal_have_lib = xyes ; then
		CGAL_LIBS="$cgal_prefix_l -lgmp -lmpfr -lm -lCGAL"
		CGAL_CFLAGS="$cgal_prefix_i"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi

	AC_LANG_POP(C++)

	AC_SUBST(CGAL_CFLAGS)
	AC_SUBST(CGAL_LIBS)
])

