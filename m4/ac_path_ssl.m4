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
				ssl_name="ssleay32"
			fi
		fi
	else
		ssl_name="ssl"
	fi
	
	ssl_config_args_l=""
	ssl_config_args_i=""
	if test "x$ssl_config_prefix" != "x"; then
                ssl_config_args_l="-L$ssl_config_prefix/lib"
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
			
			SSL_LIBS="$ssl_config_args_l -l$ssl_name -l$ssl_eay_lib -lcrypto"
		else
			SSL_LIBS="$ssl_config_args_l -l$ssl_name -lcrypto"
		fi
		SSL_CFLAGS="$ssl_config_args_i"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi

	AC_SUBST(SSL_LIBS)
	AC_SUBST(SSL_CFLAGS)
])

