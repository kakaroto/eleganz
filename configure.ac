#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.66])
AC_INIT([Eleganz], [0.1], [])
AC_CONFIG_SRCDIR([src/Eleganz.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall])

# Checks for programs.
AC_PROG_CC

dnl use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
  [AM_DEFAULT_VERBOSITY=1
   AC_SUBST(AM_DEFAULT_VERBOSITY)])


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getpagesize memset strrchr strstr strtoul strtoull])

AC_ARG_WITH([edje-cc],
   [AC_HELP_STRING([--with-edje-cc=PATH], [specify a specific path to edje_cc])],
   [edje_cc=$withval],
   [edje_cc=$(pkg-config --variable=prefix edje)/bin/edje_cc])
AC_SUBST(edje_cc)

### Checks for programs
AC_PROG_CC
AM_PROG_CC_C_O

# pkg-config
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(EFL, [edje ecore ecore-file ecore-evas evas eet eina exquisite])

ELEGANZ_CFLAGS=""
AC_ARG_ENABLE([cobra-ode],
        AC_HELP_STRING([--enable-cobra-ode], [Enable COBRA ODE support]),
        [case "${enableval}" in
            yes) ENABLE_COBRA=yes ; ELEGANZ_CFLAGS="-DCOBRA_ODE" ;;
            no)  ENABLE_COBRA=no ;;
            *) AC_MSG_ERROR(bad value ${enableval} for --enable-cobra-ode) ;;
        esac],
        ENABLE_COBRA=no)

AM_CONDITIONAL([ENABLE_COBRA_ODE], test "x$ENABLE_COBRA" != "xno")
AC_SUBST(ELEGANZ_CFLAGS)

AC_CONFIG_FILES([
        Makefile
        src/Makefile
        data/Makefile
        data/themes/Makefile
        data/themes/default/Makefile
        data/themes/light/Makefile
])
AC_OUTPUT
