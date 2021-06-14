AC_INIT([odfs], [0.1])
AM_INIT_AUTOMAKE([foreign -Wall])

AC_CONFIG_MACRO_DIRS([m4])

AC_PROG_CC
AC_PROG_CXX

AX_CXX_COMPILE_STDCXX([17], [noext], [mandatory])

PKG_CHECK_MODULES([FUSE], [fuse3 >= 3.2.1])
PKG_CHECK_MODULES([MICROHTTPD], [libmicrohttpd >= 0.9.59])
PKG_CHECK_MODULES([OPENSSL], [openssl >= 1.0.2])

AX_BOOST_BASE([1.75], [], AC_MSG_ERROR([cannot find any boost libraries]))
AX_BOOST_ASIO

AC_CONFIG_FILES(
	Makefile
	src/Makefile
)

AC_OUTPUT
