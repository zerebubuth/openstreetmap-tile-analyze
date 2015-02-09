#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([openstreetmap-tile-analyze], [0.0.1], [http://github.com/zerebubuth/openstreetmap-tile-analyze/issues])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([Makefile])

# Checks for programs.
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11
AC_PROG_RANLIB

# Checks for libraries.
AX_BOOST_BASE([1.54])
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_SYSTEM

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.

AC_OUTPUT
