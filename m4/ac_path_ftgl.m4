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
			ftgl_lib_l="-L$ftgl_config_prefix/lib"
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

