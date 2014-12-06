# check required autoconf version
AC_PREREQ([2.59])

# define distribution properties
AC_INIT([ocr], [0.8], [ocr-dev@lists.01.org])

# check if srcdir is correct
AC_CONFIG_SRCDIR([../../../inc/ocr.h])

AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([1.9 tar-ustar no-define foreign dist-bzip2 subdir-objects])

AC_COPYRIGHT([todo_copyright_info])

# configure script revision
AC_REVISION([$Revision: 0.1 $])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_LANG(C)

# Variable substitutions.
AC_SUBST([ac_aux_dir])
AC_SUBST([abs_top_srcdir])

PKG_CHECK_MODULES(XML, libxml-2.0 >= 2.4)

#
# Communication support is activated only if one of the following layer is available
#   - mpi
#

# BEGIN MPI support

# Specify which MPI installation should be used
AC_ARG_WITH([mpi], [AS_HELP_STRING([--with-mpi=PATH], [ Specify the MPI installation to use. ])],
[with_mpi=yes;
MPI_HOME=$withval;
MPI_LIB=mpi;
], [with_mpi=no])

# Allows to specify the name of the mpi library to link with
AC_ARG_WITH([mpi-lib], [AS_HELP_STRING([--with-mpi-lib=libname], [ Specify the MPI library to use. ])],
[MPI_LIB=$withval;
], [])

AS_IF([test "x$with_mpi" != xno],
      [AC_DEFINE([HC_MPI], [1], [Have mpi as communication layer])
       MPI_CFLAGS="-I${MPI_HOME}/include";
       MPI_LDFLAGS="-L${MPI_HOME}/lib -l${MPI_LIB}";
      ],
      [ AC_MSG_NOTICE([MPI installation not specified]) ])

AC_SUBST([MPI_CFLAGS])
AC_SUBST([MPI_LDFLAGS])

# END MPI support

# If any of the supported communication layer is set, turn communication support on
AS_IF([test "x$with_mpi" != xno],
      [ AC_DEFINE([HAVE_COMM], [1], [Have communication support]) ],
      [ AC_MSG_NOTICE([No communication support built for OCR]) ])

# Check if custom deque capacity is requested

AC_ARG_ENABLE(deque,
    AS_HELP_STRING([--enable-deque],
    [enable custom deque size, takes storage size as argument]),
    [with_deque=$enableval],
    [with_deque=no;])

# note: does not check deque's argument is legal
AS_IF([test "x$with_deque" != xno],
      [ AC_MSG_NOTICE([deques of size $with_deque requested])
        AC_DEFINE_UNQUOTED([INIT_DEQUE_CAPACITY], [$with_deque], [Set worker deques size]) ],
      [ AC_MSG_NOTICE([Use standard size deques])
        AC_DEFINE_UNQUOTED([INIT_DEQUE_CAPACITY], [128], [Set worker deques size])
      ])

# Check if ELS support is requested

AC_ARG_ENABLE(els,
    AS_HELP_STRING([--enable-els],
    [enable user edt local storage, takes storage size as argument]),
    [with_els=$enableval],
    [with_els=no;])

# note: does not check els' argument is legal
AS_IF([test "x$with_els" != xno],
      [ AC_MSG_NOTICE([ELS support size $with_els requested])
        AC_DEFINE_UNQUOTED([ELS_USER_SIZE], [$with_els], [Set User-ELS size]) ],
      [ AC_MSG_NOTICE([No ELS support requested])
        AC_DEFINE_UNQUOTED([ELS_USER_SIZE], [0], [Set User-ELS size])
      ])

# Check if Runtime headers are requested

AC_ARG_ENABLE(rtapi,
    AS_HELP_STRING([--enable-rtapi],
    [enable runtime API header distribution]),
    [with_rtapi=$enableval],
    [with_rtapi=no;])

AS_IF([test "x$with_rtapi" != xno],
      [ AC_MSG_NOTICE([OCR Runtime API header requested]) ],
      [ AC_MSG_NOTICE([OCR Runtime API header not requested]) ])

# Check if we need to distribute runtime headers
AM_CONDITIONAL([INCLUDE_RUNTIME_ITF_HEADERS], [test x$with_els != xno -o x$with_rtapi != xno])

AC_ARG_ENABLE(ocrlib,
    AS_HELP_STRING([--enable-ocrlib],
    [enable OCR library headers]),
    [with_ocrlib=$enableval],
    [with_ocrlib=no;])

AS_IF([test "x$with_ocrlib" != xno],
      [ AC_MSG_NOTICE([OCR-lib header requested]) ],
      [ AC_MSG_NOTICE([OCR-lib header not requested]) ])

# Check if we need to distribute OCR library headers
AM_CONDITIONAL([INCLUDE_OCR_LIBRARY_ITF_HEADERS], [test x$with_ocrlib != xno])


#
# Config file to process
#
AC_CONFIG_FILES([
Makefile
])

# Generates and runs config.status, which in turn creates the makefiles
# and any other files resulting from configuration
AC_OUTPUT
