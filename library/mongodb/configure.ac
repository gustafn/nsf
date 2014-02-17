#!/bin/bash -norc
dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run during Tcl installation
dnl	to configure the system for the local environment.
#

#-----------------------------------------------------------------------
# Sample configure.in for Tcl Extensions.  The only places you should
# need to modify this file are marked by the string __CHANGE__
#-----------------------------------------------------------------------

configdir=$(srcdir)/../../config

#-----------------------------------------------------------------------
# __CHANGE__
# Set your package name and version numbers here.
#
# This initializes the environment with PACKAGE_NAME and PACKAGE_VERSION
# set as provided.  These will also be added as -D defs in your Makefile
# so you can encode the package version directly into the source files.
#-----------------------------------------------------------------------

AC_INIT([nsfmongo], [0.2])

#--------------------------------------------------------------------
# Call TEA_INIT as the first TEA_ macro to set up initial vars.
# This will define a ${TEA_PLATFORM} variable = "unix" or "windows"
# as well as PKG_LIB_FILE and PKG_STUB_LIB_FILE.
#--------------------------------------------------------------------

TEA_INIT([3.9])

AC_CONFIG_AUX_DIR(../../config)

#--------------------------------------------------------------------
# specify some extra flags
#--------------------------------------------------------------------
AC_ARG_WITH(mongodb,
        [  --with-mongodb=MONGO_INCLUDE_DIR[,MONGO_LIB_DIR]
            absolute path to bson.h and optionally the path to the library, 
           --without-mongodb disables build of the mongo interface],
        [with_mongodb=$withval], [with_mongodb=no])

AC_ARG_WITH(nsf,
        [  --with-nsf=DIR_CONTAINING_NSFCONFIG_SH
            absolute path to nsfConfig.sh, 
           --without-nsf disables, but this is pointless],
        [with_nsf=$withval], [AC_MSG_ERROR([--with-nsf is required])])

#--------------------------------------------------------------------
# Load the tclConfig.sh file
#--------------------------------------------------------------------

TEA_PATH_TCLCONFIG
TEA_LOAD_TCLCONFIG

#--------------------------------------------------------------------
# Load the tkConfig.sh file if necessary (Tk extension)
#--------------------------------------------------------------------

#TEA_PATH_TKCONFIG
#TEA_LOAD_TKCONFIG

#-----------------------------------------------------------------------
# Handle the --prefix=... option by defaulting to what Tcl gave.
# Must be called after TEA_LOAD_TCLCONFIG and before TEA_SETUP_COMPILER.
#-----------------------------------------------------------------------

TEA_PREFIX

#-----------------------------------------------------------------------
# Standard compiler checks.
# This sets up CC by using the CC env var, or looks for gcc otherwise.
# This also calls AC_PROG_CC, AC_PROG_INSTALL and a few others to create
# the basic setup necessary to compile executables.
#-----------------------------------------------------------------------

TEA_SETUP_COMPILER

#-----------------------------------------------------------------------
# __CHANGE__
# Specify the C source files to compile in TEA_ADD_SOURCES,
# public headers that need to be installed in TEA_ADD_HEADERS,
# stub library C source files to compile in TEA_ADD_STUB_SOURCES,
# and runtime Tcl library files in TEA_ADD_TCL_SOURCES.
# This defines PKG(_STUB)_SOURCES, PKG(_STUB)_OBJECTS, PKG_HEADERS
# and PKG_TCL_SOURCES.
#-----------------------------------------------------------------------
if test ! "${with_mongodb}" = no; then
        MONGO_INC_DIR="`echo $with_mongodb |cut -f1 -d,`"
        MONGO_LIB_DIR="`echo $with_mongodb |cut -f2 -d, -s`"
fi
if test -z "$MONGO_INC_DIR" ; then
        mongo_h_ok=0
        MONGO_INC_SPEC=""
else
        MONGO_INC_SPEC="-I${MONGO_INC_DIR}"
        echo "Checking ${MONGO_INC_DIR}/bson.h"
        if test -f "${MONGO_INC_DIR}/bson.h" ; then
                mongo_h_ok=1
        else
                mongo_h_ok=0
        fi
fi
                                                                              
if test "${mongo_h_ok}" = "0" ; then
      AC_MSG_ERROR([
        Could not locate bson.h on your machine to build the nsf mongo interface.
        ])
fi
                                                                              
if test -z "${MONGO_LIB_DIR}" ; then
        MONGO_LIB_SPEC=""
else
        MONGO_LIB_SPEC="-L${MONGO_LIB_DIR}"
fi
                                                                              
#echo "MONGO include spec = '${MONGO_INC_SPEC}'"
#echo "MONGO lib spec = '${MONGO_LIB_SPEC}'"

#--------------------------------------------------------------------
# Load the nsfConfig.sh file
#--------------------------------------------------------------------

AC_MSG_NOTICE([Reading file ${with_nsf}/nsfConfig.sh])
source ${with_nsf}/nsfConfig.sh

TEA_ADD_SOURCES([nsfmongo.c])
TEA_ADD_HEADERS([])
TEA_ADD_INCLUDES([-I${with_nsf}/generic ${NSF_BUILD_INCLUDE_SPEC} ${MONGO_INC_SPEC}])
#TEA_ADD_LIBS([$NSF_BUILD_STUB_LIB_SPEC $MONGO_LIB_SPEC -Wl,-rpath,${MONGO_LIB_DIR} -L${MONGO_LIB_DIR} -lmongoc -lbson])
TEA_ADD_LIBS([$NSF_BUILD_STUB_LIB_SPEC $MONGO_LIB_SPEC -L${MONGO_LIB_DIR} -lmongoc -lbson])
TEA_ADD_CFLAGS([])
TEA_ADD_STUB_SOURCES([])
TEA_ADD_TCL_SOURCES([])

#--------------------------------------------------------------------
# __CHANGE__
# A few miscellaneous platform-specific items:
#
# Define a special symbol for Windows (BUILD_sample in this case) so
# that we create the export library with the dll.
#
# Windows creates a few extra files that need to be cleaned up.
# You can add more files to clean if your extension creates any extra
# files.
#
# TEA_ADD_* any platform specific compiler/build info here.
#--------------------------------------------------------------------

if test "${TEA_PLATFORM}" = "windows" ; then
    CLEANFILES="pkgIndex.tcl *.lib *.dll *.exp *.ilk *.pdb vc*.pch"
    #TEA_ADD_SOURCES([win/winFile.c])
    #TEA_ADD_INCLUDES([-I\"$(${CYGPATH} ${srcdir}/win)\"])
else
    CLEANFILES="pkgIndex.tcl"
    #TEA_ADD_SOURCES([unix/unixFile.c])
    #TEA_ADD_LIBS([-lsuperfly])
fi
AC_SUBST(CLEANFILES)

#--------------------------------------------------------------------
# __CHANGE__
# Choose which headers you need.  Extension authors should try very
# hard to only rely on the Tcl public header files.  Internal headers
# contain private data structures and are subject to change without
# notice.
# This MUST be called after TEA_LOAD_TCLCONFIG / TEA_LOAD_TKCONFIG
#--------------------------------------------------------------------

TEA_PUBLIC_TCL_HEADERS
#TEA_PRIVATE_TCL_HEADERS

#TEA_PUBLIC_TK_HEADERS
#TEA_PRIVATE_TK_HEADERS
#TEA_PATH_X

#--------------------------------------------------------------------
# Check whether --enable-threads or --disable-threads was given.
#--------------------------------------------------------------------

TEA_ENABLE_THREADS

#--------------------------------------------------------------------
# The statement below defines a collection of symbols related to
# building as a shared library instead of a static library.
#--------------------------------------------------------------------

TEA_ENABLE_SHARED

#--------------------------------------------------------------------
# This macro figures out what flags to use with the compiler/linker
# when building shared/static debug/optimized objects.  This information
# can be taken from the tclConfig.sh file, but this figures it all out.
#--------------------------------------------------------------------

TEA_CONFIG_CFLAGS

#--------------------------------------------------------------------
# Set the default compiler switches based on the --enable-symbols option.
#--------------------------------------------------------------------

TEA_ENABLE_SYMBOLS

#--------------------------------------------------------------------
# Everyone should be linking against the Tcl stub library.  If you
# can't for some reason, remove this definition.  If you aren't using
# stubs, you also need to modify the SHLIB_LD_LIBS setting below to
# link against the non-stubbed Tcl library.  Add Tk too if necessary.
#--------------------------------------------------------------------

AC_DEFINE(USE_TCL_STUBS)
#AC_DEFINE(USE_TK_STUBS)

#--------------------------------------------------------------------
# This macro generates a line to use when building a library.  It
# depends on values set by the TEA_ENABLE_SHARED, TEA_ENABLE_SYMBOLS,
# and TEA_LOAD_TCLCONFIG macros above.
#--------------------------------------------------------------------

TEA_MAKE_LIB

#--------------------------------------------------------------------
# Find tclsh so that we can run pkg_mkIndex to generate the pkgIndex.tcl
# file during the install process.  Don't run the TCLSH_PROG through
# ${CYGPATH} because it's being used directly by make.
# Require that we use a tclsh shell version 8.2 or later since earlier
# versions have bugs in the pkg_mkIndex routine.
# Add WISH as well if this is a Tk extension.
#--------------------------------------------------------------------

TEA_PROG_TCLSH
#TEA_PROG_WISH

#--------------------------------------------------------------------
# Finally, substitute all of the various values into the Makefile.
# You may alternatively have a special pkgIndex.tcl.in or other files
# which require substituting th AC variables in.  Include these here.
#--------------------------------------------------------------------

AC_CONFIG_FILES([Makefile])

AC_OUTPUT








































































