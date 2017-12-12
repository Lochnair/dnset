#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dnset], [1.0])
AM_INIT_AUTOMAKE
AC_OUTPUT(Makefile)
AC_CONFIG_SRCDIR([dnset.c])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([string.h])

PKG_CHECK_MODULES(LIBNL3, libnl-3.0 >= 3.1, [have_libnl3=yes], [have_libnl3=no])
if (test "${have_libnl3}" = "yes"); then

        CFLAGS+=" $LIBNL3_CFLAGS"

        LIBS+=" $LIBNL3_LIBS"

fi

PKG_CHECK_MODULES(LIBNL_GENL3, libnl-genl-3.0 >= 3.1, [have_libnl_genl3=yes], [have_libnl_genl3=no])
if (test "${have_libnl_genl3}" = "yes"); then
	CFLAGS+=" $LIBNL_GENL3_CFLAGS"
        LIBS+=" $LIBNL_GENL3_LIBS"
fi

PKG_CHECK_MODULES(LIBIDN2, libidn2 >= 0.1, [have_libidn2=yes], [have_libidn2=no])
if (test "${have_libidn2}" = "yes"); then
        CFLAGS+=" $LIBIDN2_CFLAGS"
        LIBS+=" $LIBIDN2_LIBS"
fi

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT
