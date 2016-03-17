dnl **************************************************************************
dnl Versioning Macros
dnl **************************************************************************
m4_define([redis_major_version], [0])
m4_define([redis_minor_version], [1])
m4_define([redis_micro_version], [3])
m4_define([redis_version],
          [redis_major_version.redis_minor_version.redis_micro_version])
m4_define([redis_binary_age], [m4_eval(100 * redis_minor_version +
           redis_micro_version)])
m4_define([lt_current], [m4_eval(100 * redis_minor_version +
           redis_micro_version - redis_interface_age)])
m4_define([lt_revision], [redis_interface_age])
m4_define([lt_age], [m4_eval(redis_binary_age - redis_interface_age)])


dnl **************************************************************************
dnl Initialize Automake
dnl **************************************************************************
AC_PREREQ([2.64])
AC_INIT([redis],[redis_version])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([build/m4])

AM_INIT_AUTOMAKE([-Wno-portability])
AM_MAINTAINER_MODE([enable])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AS_AM_REALLY_SILENT

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AM_PATH_GLIB_2_0
GLIB_GSETTINGS

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT


dnl **************************************************************************
dnl Versioning Definitions
dnl **************************************************************************
REDIS_MAJOR_VERSION=redis_major_version
REDIS_MINOR_VERSION=redis_minor_version
REDIS_MICRO_VERSION=redis_micro_version
REDIS_API_VERSION=1.0
REDIS_VERSION=redis_version
AC_SUBST(REDIS_MAJOR_VERSION)
AC_SUBST(REDIS_MINOR_VERSION)
AC_SUBST(REDIS_MICRO_VERSION)
AC_SUBST(REDIS_API_VERSION)
AC_SUBST(REDIS_VERSION)


dnl **************************************************************************
dnl Check for Required Modules
dnl **************************************************************************
PKG_CHECK_MODULES(GIO,     [gio-2.0 >= 2.36])
PKG_CHECK_MODULES(GOBJECT, [gobject-2.0 >= 2.36])



dnl ************************************************************************
dnl Additional Headers
dnl ************************************************************************
AC_CHECK_HEADERS([hiredis/hiredis.h hiredis/async.h],
		 [],
		 [AC_MSG_ERROR([hiredis headers could not be found.])])


dnl **************************************************************************
dnl Enable extra debugging options
dnl **************************************************************************
m4_define([debug_default],
	  [m4_if(m4_eval(redis_minor_version % 2), [1],
	  [yes], [minimum])])
AC_ARG_ENABLE([debug],
	      [AS_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
	      		      [turn on debugging @<:@default=debug_default@:>@])],
	      		      [],
	      		      [enable_debug=debug_default])
AS_CASE([$enable_debug],
	[yes], [
		CFLAGS="$CFLAGS -g"
		CFLAGS="$CFLAGS -Wall"
		CFLAGS="$CFLAGS -Werror"
		CFLAGS="$CFLAGS -Wold-style-definition"
		CFLAGS="$CFLAGS -Wdeclaration-after-statement"
		CFLAGS="$CFLAGS -Wredundant-decls"
		CFLAGS="$CFLAGS -Wmissing-noreturn"
		CFLAGS="$CFLAGS -Wcast-align"
		CFLAGS="$CFLAGS -Wwrite-strings"
		CFLAGS="$CFLAGS -Winline"
		CFLAGS="$CFLAGS -Wformat-nonliteral"
		CFLAGS="$CFLAGS -Wformat-security"
		CFLAGS="$CFLAGS -Wswitch-enum"
		CFLAGS="$CFLAGS -Wswitch-default"
		CFLAGS="$CFLAGS -Winit-self"
		CFLAGS="$CFLAGS -Wmissing-include-dirs"
		CFLAGS="$CFLAGS -Wundef"
		CFLAGS="$CFLAGS -Waggregate-return"
		CFLAGS="$CFLAGS -Wmissing-format-attribute"
		CFLAGS="$CFLAGS -Wnested-externs"
		CFLAGS="$CFLAGS -Wshadow"
	],
	[minimum], [
		CFLAGS="$CFLAGS -DG_DISABLE_CAST_CHECKS"
	],
	[no], [
		CFLAGS="$CFLAGS -DG_DISABLE_ASSERT"
		CFLAGS="$CFLAGS -DG_DISABLE_CHECKS"
		CFLAGS="$CFLAGS -DG_DISABLE_CAST_CHECKS"
	],
	[AC_MSG_ERROR([Unknown argument to --enable-debug])]
)
AC_SUBST(REDIS_DEBUG_CFLAGS)


dnl **************************************************************************
dnl Unit Tests
dnl **************************************************************************
AC_PATH_PROG([GTESTER], [gtester])
AC_PATH_PROG([GTESTER_REPORT], [gtester-report])
AM_CONDITIONAL(ENABLE_GLIB_TEST, test "x$enable_glibtest" = "xyes")


dnl **************************************************************************
dnl Miscellaneous
dnl **************************************************************************
GTK_DOC_CHECK([1.11])
GOBJECT_INTROSPECTION_CHECK([0.6.7])
VAPIGEN_CHECK
SHAMROCK_EXPAND_LIBDIR
SHAMROCK_EXPAND_BINDIR
SHAMROCK_EXPAND_DATADIR


dnl **************************************************************************
dnl Output
dnl **************************************************************************
AC_CONFIG_FILES([
	Makefile
	data/redis-glib-1.0.pc
])
AC_OUTPUT

echo ""
echo "Redis-GLib $VERSION"
echo ""
echo "  Prefix.....................: ${prefix}"
echo "  Debug Level................: ${enable_debug}"
echo "  Compiler Flags.............: ${CFLAGS}"
echo "  Enable API Reference.......: ${enable_gtk_doc}"
echo "  Enable Test Suite..........: ${enable_glibtest}"
echo "  Enable Vala Bindings.......: ${enable_vala}"
echo ""
