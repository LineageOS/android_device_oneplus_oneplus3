#                                               -*- Autoconf -*-

# configure.ac -- Autoconf script for mm-omxaudio
#

# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([omxaudio],
	1.0.0)
AM_INIT_AUTOMAKE([-Wall -Werror gnu foreign])
AM_MAINTAINER_MODE
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

#release versioning
OMXAUDIO_MAJOR_VERSION=1
OMXAUDIO_MINOR_VERSION=0
OMXAUDIO_MICRO_VERSION=0

OMXAUDIO_LIBRARY_VERSION=$OMXAUDIO_MAJOR_VERSION:$OMXAUDIO_MINOR_VERSION:$OMXAUDIO_MICRO_VERSION
AC_SUBST(OMXAUDIO_LIBRARY_VERSION)

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_CONFIG_FILES([     \
	Makefile \
	adec-aac/Makefile \
	adec-mp3/Makefile \
	aenc-aac/Makefile \
	adec-aac/qdsp6/Makefile \
	adec-mp3/qdsp6/Makefile \
	aenc-aac/qdsp6/Makefile \
	])
AC_OUTPUT
