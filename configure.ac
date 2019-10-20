AC_INIT([odfs], [0.1])
AM_INIT_AUTOMAKE([foreign -Wall])

AC_PROG_CC

PKG_CHECK_MODULES([FUSE], [fuse3 >= 3.2.1])
PKG_CHECK_MODULES([CURL], [libcurl >= 7.66.0])
PKG_CHECK_MODULES([MICROHTTPD], [libmicrohttpd >= 0.9.59])

AC_CONFIG_FILES(
	Makefile
	src/Makefile
)

AC_OUTPUT
