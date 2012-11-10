# check required autoconf version
AC_PREREQ([2.59])

# define distribution properties
AC_INIT([ocr-hc-impl], [0.1], [maintainer@todo.edu])

# check if srcdir is correct
AC_CONFIG_SRCDIR([inc/ocr.h])

AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([1.9 tar-ustar no-define foreign dist-bzip2])

AC_COPYRIGHT([todo_copyright_info])

# configure script revision
AC_REVISION([$Revision: 0.1 $])

AC_PROG_CC
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

#
# Config file to process
#
AC_CONFIG_FILES([
Makefile
src/Makefile
])

# Generates and runs config.status, which in turn creates the makefiles 
# and any other files resulting from configuration
AC_OUTPUT
