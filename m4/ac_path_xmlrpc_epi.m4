AC_DEFUN([AC_PATH_XMLRPC_EPI], [
	AC_ARG_WITH([xmprpc-epi-prefix],
		AC_HELP_STRING([--with-xmlrpc-epi-prefix=PREFIX], 
		[Prefix where xmlrpc-epi is installed (optional)]),
		xmlrpc_epi_config_prefix="$withval", )
	AC_ARG_WITH([xmlrpc-epi-library-name],
		AC_HELP_STRING([--with-xmlrpc-epi-library-name=NAME], 
		[Name of xmlrpc-epi library name(optional)]),
		xmlrpc_epi_library_name="$withval", )

	XMLRPC_EPI_CFLAGS=""
	XMLRPC_EPI_LIBS=""

	xmlrpc_epi_config_args_l=""
	xmlrpc_epi_config_args_i=""
	if test "x$xmlrpc_epi_config_prefix" != "x"; then
                xmlrpc_epi_config_args_l="-L$xmlrpc_epi_config_prefix/lib"
                xmlrpc_epi_config_args_i="-I$xmlrpc_epi_config_prefix/include"
	fi

	if test "x$xmlrpc_epi_library_name" = "x"; then
		xmlrpc_epi_library_name="xmlrpc-epi"
	fi

	AC_CHECK_LIB($xmlrpc_epi_library_name, XMLRPC_RequestNew, ac_xmlrpc_epi=yes, ac_xmlrpc_epi=no,
		[$xmlrpc_epi_config_args_l])

        xmlrpc_epi_config_args_l="$xmlrpc_epi_config_args_l -l$xmlrpc_epi_library_name"

	if test $ac_xmlrpc_epi = yes ; then
		XMLRPC_EPI_LIBS="$xmlrpc_epi_config_args_l -l$xmlrpc_epi_library_name"
		XMLRPC_EPI_CFLAGS="$xmlrpc_epi_config_args_i"
		ifelse([$1], , :, [$1])
	else
		ifelse([$2], , :, [$2])
	fi


	AC_SUBST(XMLRPC_EPI_LIBS)
	AC_SUBST(XMLRPC_EPI_CFLAGS)

])

