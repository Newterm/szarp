dnl Checks if we have usable liblua library. Sets $LUA_LIBS and $LUA_CFLAGS variables. 
dnl AC_PATH_LUA( [ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]] )
AC_DEFUN([AC_PATH_LUA], [
	AC_ARG_WITH([lua-prefix],
		AC_HELP_STRING([--with-lua-prefix=PREFIX], 
		[Prefix where liblua is installed (optional)]),
		lua_config_prefix="$withval", )

	LUA_LIBS=""
	LUA_CFLAGS=""

	lua_config_args_l=""
	lua_config_args_i=""
	if test "x$lua_config_prefix" != "x"; then
                lua_config_args_l="-L$lua_config_prefix/lib"
                lua_config_args_i="-I$lua_config_prefix/include"
	fi

	ac_lua=yes
	if test $ac_lua = yes ; then
		AC_CHECK_HEADER([luaconf.h], , ac_boost=no)
	fi
	if test $ac_lua = yes ; then
		AC_CHECK_LIB(lua, lua_call, ac_lua=yes, ac_lua=no,
			[$lua_config_args_l])
	fi

	if test $ac_lua = yes ; then
		LUA_LIBS="$lua_config_args_l -llua"
		LUA_CFLAGS="$lua_config_args_i"
		ifelse([$1], , :, [$1])
	else
		if pkg-config --exists lua5.1; then
			LUA_LIBS=`pkg-config --libs lua5.1`
			LUA_CFLAGS=`pkg-config --cflags lua5.1`
			ifelse([$1], , :, [$1])
		else
			ifelse([$2], , :, [$2])
		fi
	fi

	AC_SUBST(LUA_LIBS)
	AC_SUBST(LUA_CFLAGS)
])

