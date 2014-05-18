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
		FREETYPE_LIBS="-L${freetype_prefix}/lib $FREETYPE_LIBS";
		FREETYPE_CFLAGS="-I ${freetype_prefix}/include";
	fi

	if test x$freetype_include != x; then 
		FREETYPE_CFLAGS="$FREETYPE_CFLAGS -I $freetype_include";
	fi
			
	AC_SUBST(FREETYPE_LIBS)
	AC_SUBST(FREETYPE_CFLAGS)

])

