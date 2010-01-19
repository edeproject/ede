dnl
dnl found in https://jim.sh/svn/jim/vendor/sdcc/2.5.0/configure.in
dnl Examples: AC_EXPAND(prefix, "/usr/local", expanded_prefix)

AC_DEFUN([EDE_EXPAND], [
	test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
	ac_expand=[$]$1
	test "x$ac_expand" = xNONE && ac_expand="[$]$2"
	ac_expand=`eval echo [$]ac_expand`
	$3=`eval echo [$]ac_expand`
])
