dnl Checks for xsltproc program, with given xslt library version (default is
dnl 10010). Sets XSLTPROC and XSLT_VERSION variable.
dnl AC_PROG_XSLTPROC([VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])
AC_DEFUN([AC_PROG_XSLTPROC], [
        AC_CHECK_PROG([XSLTPROC], [xsltproc], [xsltproc], [])
        ac_xsltproc_ok=yes
        if test "x$XSLTPROC" != xxsltproc ; then
                AC_MSG_WARN([
                        xsltproc program (XSLT processor) was not found in
                        your path. XSLT-based tools won't work. ])
                XSLT_VERSION=00000
                ac_xsltproc_ok=no
        else
		AC_MSG_CHECKING([for xsltproc version])
                XSLT_VERSION=`$XSLTPROC --version | grep -o -m 1 'libxslt [[0-9]]*' | grep -o '[[0-9]]\+'`
                if test -n "$1" ; then
                        ac_xslt_version=$1
                else
                        ac_xslt_version=10010
                fi
                if test "$XSLT_VERSION" -lt "$ac_xslt_version" ; then
                        AC_MSG_WARN([
                        xslproc program (XSLT processor) version found 
                        ($XSLT_VERSION) is too old (libxslt $ac_xslt_version based
                        version required). XSLT-based tools may not work.])
                        ac_xsltproc_ok=no
                else 
                        AC_MSG_RESULT([$XSLT_VERSION])
                fi
        fi
        XSLTPROC=xsltproc
        AC_SUBST(XSLT_VERSION)
        AC_SUBST(XSLTPROC)
        if test x$ac_xsltproc_ok != xyes ; then
               ifelse([$3], , :, [$3])
        else
               ifelse([$2], , :, [$2])
        fi
])

