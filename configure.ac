
#--------------------------------------------------------------------
# Sample configure.in for Tcl Extensions.  The only places you should
# need to modify this file are marked by the string __CHANGE__
#--------------------------------------------------------------------

#--------------------------------------------------------------------
# __CHANGE__
# This very first macro is used to verify that the configure script can 
# find the sources.  The argument to AC_INIT should be a unique filename
# for this package, and can be a relative path, such as:
#
#--------------------------------------------------------------------
AC_PREREQ([2.71])

define(NsfVersion, 2.4.0)
AC_INIT([nsf],[NsfVersion],[https://sourceforge.net/p/next-scripting/tickets/])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([tclconfig])

#--------------------------------------------------------------------
# Call TEA_INIT as the first TEA_ macro to set up initial vars.
# This will define a ${TEA_PLATFORM} variable == "unix" or "windows".
#--------------------------------------------------------------------

TEA_INIT([3.10])

#--------------------------------------------------------------------
# specify some extra flags
#--------------------------------------------------------------------
# According to http://www.gnu.org/software/autoconf/manual/autoconf.html
#  - "with" is for external software (optional packages)
#  - "enable" is for optional features
#
AC_ARG_WITH([aolserver3],
	AS_HELP_STRING([--with-aolserver3=AOL_SERVER_DIR],
		[build an AOLserver 3 module; 
		point to directory containing aolsever/include (default: off)]),
        [with_aol3=$withval], [with_aol3=no])
AC_ARG_WITH([dtrace],
	AS_HELP_STRING([--with-dtrace], [build nsf with dtrace (default: without)]),
        [with_dtrace=$withval], [with_dtrace=no])
AC_ARG_WITH([mongoc],
	AS_HELP_STRING([--with-mongoc=MONGOC_INCLUDE_DIR[,MONGOC_LIB_DIR]],
		[build nsf with mongodb c-driver support (default: without)]),
		[with_mongoc=$withval], [with_mongoc=no])
AC_ARG_WITH([bson],
	AS_HELP_STRING([--with-bson=BSON_INCLUDE_DIR[,BSON_LIB_DIR]],
		[build nsf with mongodb bson support (default: without)]),
		[with_bson=$withval], [with_bson=no])
AC_ARG_ENABLE([profile],
	AS_HELP_STRING([--enable-profile], [build nsf with profile support (default: disabled)]),
        [enable_profile=$enableval], [enable_profile=no])
AC_ARG_ENABLE([memcount],
	AS_HELP_STRING([--enable-memcount=yes|trace],
		[build nsf with memcount debug support (default: disabled)]),
        [enable_memcount=$enableval], [enable_memcount=no])
AC_ARG_ENABLE([development],
	AS_HELP_STRING([--enable-development=yes|test],
		[build nsf with development support (intensive runtime checking, etc.; default: disabled)]),
        [enable_development=$enableval], [enable_development=no])
AC_ARG_ENABLE([assertions],
	AS_HELP_STRING([--enable-assertions],
		[build nsf with assertion support (default: enabled)]),
        [enable_assertions=$enableval], [enable_assertions=yes])
AC_ARG_ENABLE([assemble],
	AS_HELP_STRING([--enable-assemble=yes|label|call],
		[build nsf with assemble support (default: disabled)]),
        [enable_assemble=$enableval], [enable_assemble=no])

subdirs=""

if ! test "$with_mongoc" = no; then
   test_mongodb=test-mongdb
   subdirs="$subdirs ${srcdir}/library/mongodb"
fi

test_actiweb=""
libdirs_actiweb=""
apps_actiweb=""

AC_SUBST([subdirs])


#--------------------------------------------------------------------
# __CHANGE__
# Set your package name and version numbers here.  The NODOT_VERSION is
# required for constructing the library name on systems that don't like
# dots in library names (Windows).  The VERSION variable is used on the
# other systems.
#--------------------------------------------------------------------

NSF_MAJOR_VERSION=2
NSF_MINOR_VERSION=4

NSF_PATCH_LEVEL=$PACKAGE_VERSION

NSF_VERSION=${NSF_MAJOR_VERSION}.${NSF_MINOR_VERSION}
NODOT_VERSION=${NSF_MAJOR_VERSION}${NSF_MINOR_VERSION}

AC_SUBST([NSF_VERSION])
AC_SUBST([NSF_MAJOR_VERSION])
AC_SUBST([NSF_MINOR_VERSION])
AC_SUBST([NSF_PATCH_LEVEL])

echo "Configuring NSF Version $PACKAGE_VERSION"

AC_CHECK_PROG(git_installed,git,"yes", "no")

if test "$git_installed" = "yes" -a -d ".git" ; then
   nsf_commit=`git describe --always --dirty`
elif test -f "COMMIT" ; then
   nsf_commit=`cat COMMIT`
else
   nsf_commit=unknown-${PACKAGE_VERSION}
fi

NSF_COMMIT=$nsf_commit
AC_SUBST([NSF_COMMIT])

# AC_DEFINE_UNQUOTED([NSF_COMMIT], "$nsf_commit", 
#       	     [Commit ID at the time of the last configure run.])

AC_CHECK_TYPES([intptr_t, uintptr_t])


#--------------------------------------------------------------------
# Load the tclConfig.sh file
#--------------------------------------------------------------------

TEA_PATH_TCLCONFIG
TEA_LOAD_TCLCONFIG

AC_CHECK_FUNCS([strnstr])

#--------------------------------------------------------------------
# check for TK
#--------------------------------------------------------------------


#--------------------------------------------------------------------
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

#--------------------------------------------------------------------
# check for extra flags
#
# FORCE_NO_STUBS is deactivated for now
if ! test "$with_aol3" = "no"; then
	echo "Pre-configuring AOL-Server (nsf)"
	AOL_DEFINES="-DAOL_SERVER -DUSE_TCL8X -I$with_aol3/include -I$TCL_SRC_DIR/generic"
	FORCE_NO_STUBS=1
else
	AOL_DEFINES=""
	FORCE_NO_STUBS=0
fi

# set the default aol_prefix
aol_prefix="/usr/local/aolserver"
# if we have under ${prefix} either modules/tcl/init.tcl (aolserver
# style) or modules/tcl/init.tcl (naviserver style), then use
# ${prefix} as aol_prefix (path for the installation of
# aolserver/naviserver specific tcl files).
if test -f "${prefix}/modules/tcl/init.tcl" ; then 
   aol_prefix="${prefix}"
else
   if test -f "${prefix}/tcl/init.tcl" ; then 
      aol_prefix="${prefix}"
   fi
fi
AC_SUBST([aol_prefix])

#--------------------------------------------------------------------
# check for tclCompile.h (needed, when compiled without full source)
if test -f "$TCL_SRC_DIR/generic/tclCompile.h"; then
   AC_DEFINE([HAVE_TCL_COMPILE_H], [1], [Do we have tclCompile.h installed?])
fi

if test "$enable_assertions" = yes; then
   AC_DEFINE([NSF_WITH_ASSERTIONS], [1], [Are we building with assertions support?])
fi

if test "$enable_profile" = yes; then
   AC_DEFINE([NSF_PROFILE], [1], [Are we building with profile support?])
fi

if test "$enable_development" = yes; then
   AC_DEFINE([NSF_DEVELOPMENT], [1], [Are we building with development support?])
fi
if test "$enable_development" = test; then
   AC_DEFINE([NSF_DEVELOPMENT_TEST], [1], [Are we building with development support and intesive testing?])
fi

if test "$enable_memcount" = yes; then
   AC_DEFINE([NSF_MEM_COUNT], [1], [Are we building with memcount support?])
fi
if test "$enable_memcount" = trace; then
   AC_DEFINE([NSF_MEM_TRACE], [1], [Are we building with memcount tracing support?])
fi

if test "$enable_assemble" = yes; then
   AC_DEFINE([NSF_ASSEMBLE], [1], [Are we building with assembly support?])
fi
if test "$enable_assemble" = call; then
   AC_DEFINE([NSF_ASSEMBLE_CT], [1], [Are we building with assembly call threading support?])
fi
if test "$enable_assemble" = call; then
   AC_DEFINE([NSF_ASSEMBLE_LT], [1], [Are we building with assembly label threading support?])
fi

DTRACE_OBJ=
if test "$with_dtrace" = yes; then
   AC_DEFINE([NSF_DTRACE], [1], [Are we building with DTrace support?])
   # Under macOS, we need no nsfDTrace.o
   if test "`uname -s`" != "Darwin" ; then
      DTRACE_OBJ=nsfDTrace.o
   fi
   DTRACE_HDR='$(src_generic_dir)/nsfDTrace.h'
   DTRACE_SRC='$(src_generic_dir)/nsfDTrace.d'
fi
AC_SUBST([DTRACE_OBJ])
AC_SUBST([DTRACE_HDR])
AC_SUBST([DTRACE_SRC])

#-----------------------------------------------------------------------
# __CHANGE__
# Specify the C source files to compile in TEA_ADD_SOURCES,
# public headers that need to be installed in TEA_ADD_HEADERS,
# stub library C source files to compile in TEA_ADD_STUB_SOURCES,
# and runtime Tcl library files in TEA_ADD_TCL_SOURCES.
# This defines PKG(_STUB)_SOURCES, PKG(_STUB)_OBJECTS, PKG_HEADERS
# and PKG_TCL_SOURCES.
#-----------------------------------------------------------------------
stubdir=stubs${TCL_MAJOR_VERSION}.${TCL_MINOR_VERSION}
TEA_ADD_SOURCES([nsf.c nsfError.c nsfObjectData.c nsfProfile.c \
	nsfDebug.c nsfUtil.c nsfObj.c nsfPointer.c nsfEnumerationType.c \
        nsfCmdDefinitions.c nsfFunPtrHashTable.c nsfShadow.c nsfCompile.c aolstub.c \${srcdir}/generic/${stubdir}/nsfStubInit.${OBJEXT}])
TEA_ADD_HEADERS([generic/nsf.h generic/nsfInt.h generic/${stubdir}/nsfDecls.h generic/${stubdir}/nsfIntDecls.h])
TEA_ADD_INCLUDES([])
TEA_ADD_LIBS([])
TEA_ADD_CFLAGS([-DNSF_VERSION=\\\"$NSF_VERSION\\\" -DNSF_PATCHLEVEL=\\\"$NSF_PATCH_LEVEL\\\" \
-DNSF_COMMIT=\\\"$NSF_COMMIT\\\" $AOL_DEFINES ])
TEA_ADD_STUB_SOURCES([nsfStubLib.c])
TEA_ADD_TCL_SOURCES([])

#--------------------------------------------------------------------
# __CHANGE__
#
# You can add more files to clean if your extension creates any extra
# files by extending CLEANFILES.
# Add pkgIndex.tcl if it is generated in the Makefile instead of ./configure
# and change Makefile.in to move it from CONFIG_CLEAN_FILES to BINARIES var.
#
# A few miscellaneous platform-specific items:
# TEA_ADD_* any platform specific compiler/build info here.
#--------------------------------------------------------------------

if test "${TEA_PLATFORM}" = "windows" ; then
    if test "$GCC" != "yes" ; then
       AC_DEFINE([VISUAL_CC])
    fi
    CLEANFILES="*.lib *.dll *.exp *.ilk *.pdb vc50.pch vc60.pch "
    #TEA_ADD_SOURCES([win/winFile.c])
    #TEA_ADD_INCLUDES([-I\"$(${CYGPATH} ${srcdir}/win)\"])
else
    CLEANFILES="*.a *.so *~ core gmon.out"
    #TEA_ADD_SOURCES([unix/unixFile.c])
    #TEA_ADD_LIBS([-lsuperfly])
fi
CLEANFILES="$CLEANFILES *.${OBJEXT} pkgIndex.tcl"

#--------------------------------------------------------------------
# __CHANGE__
# Choose which headers you need.  Extension authors should try very
# hard to only rely on the Tcl public header files.  Internal headers
# contain private data structures and are subject to change without
# notice.
# This must be done AFTER calling TEA_PATH_TCLCONFIG/TEA_LOAD_TCLCONFIG
# so that we can extract TCL_SRC_DIR from the config file (in the case
# of private headers
#--------------------------------------------------------------------

#TEA_PUBLIC_TCL_HEADERS
TEA_PRIVATE_TCL_HEADERS

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

# Without the following two eval statements, NSF_SHARED_LIB_SUFFIX 
# in nsfConfig.sh has $PACKAGE_VERSION unresolved. When another
# app links against nsf, the PACKAGE_VERSIONs are confused.
#
# Without the first eval, we get
#   NSF_SHARED_LIB_SUFFIX=${PACKAGE_VERSION}\$\{DBGX\}${SHLIB_SUFFIX}
#   NSF_UNSHARED_LIB_SUFFIX=${PACKAGE_VERSION}\$\{DBGX\}.a
#
# after the first eval, we get
#   NSF_SHARED_LIB_SUFFIX=1.2.1${DBGX}.so
#   NSF_UNSHARED_LIB_SUFFIX=1.2.1${DBGX}.a
# after the second eval, all variables are resolved.


eval "SHARED_LIB_SUFFIX=${SHARED_LIB_SUFFIX}"
eval "UNSHARED_LIB_SUFFIX=${UNSHARED_LIB_SUFFIX}"

#eval "SHARED_LIB_SUFFIX=${SHARED_LIB_SUFFIX}"
#eval "UNSHARED_LIB_SUFFIX=${UNSHARED_LIB_SUFFIX}"

#--------------------------------------------------------------------
# Set the default compiler switches based on the --enable-symbols 
# option.
#--------------------------------------------------------------------

TEA_ENABLE_SYMBOLS

#--------------------------------------------------------------------
# Everyone should be linking against the Tcl stub library.  If you
# can't for some reason, remove this definition.  If you aren't using
# stubs, you also need to modify the SHLIB_LD_LIBS setting below to
# link against the non-stubbed Tcl library.
#--------------------------------------------------------------------

AC_DEFINE([USE_TCL_STUBS], [1], [Use Tcl stubs])

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
#--------------------------------------------------------------------

TEA_PROG_TCLSH

# make this available, for such as nsfConfig.sh
NSF_COMPATIBLE_TCLSH=${TCLSH_PROG}
AC_SUBST([NSF_COMPATIBLE_TCLSH])
AC_SUBST([TCL_EXEC_PREFIX])

#
# Next Scripting specific configs
#
#NSF_SRC_DIR=$srcdir
#NSF_SRC_DIR="`pwd`"

NSF_BUILD_DIR=${PWD}
cd ${srcdir}
NSF_SRC_DIR=${PWD}
cd ${NSF_BUILD_DIR}

AC_SUBST([NSF_SRC_DIR])

eval "NSF_PKG_LIBDIR=\"${libdir}/${PACKAGE_NAME}${PACKAGE_VERSION}\""
AC_SUBST([NSF_PKG_LIBDIR])

if test "${TEA_PLATFORM}" != "windows" ; then

NSF_BUILD_LIB_SPEC="-L${NSF_BUILD_DIR} -lnsf${PACKAGE_VERSION}"
NSF_LIB_SPEC="-L${NSF_PKG_LIBDIR} -lnsf${PACKAGE_VERSION}"

# If someone wants to build without stubs, as it was the case for
# earlier Tcl versions, we keep the following block as a reference.
if test "${TCL_MAJOR_VERSION}" = "8" -a "${TCL_MINOR_VERSION}" = "0"; then
	NSF_BUILD_STUB_LIB_PATH=""
	NSF_STUB_LIB_PATH=""
	NSF_BUILD_STUB_LIB_SPEC=""
	NSF_STUB_LIB_SPEC=""
else
	NSF_BUILD_STUB_LIB_PATH="${NSF_SRC_DIR}/${PKG_STUB_LIB_FILE}"
	NSF_STUB_LIB_PATH="${NSF_PKG_LIBDIR}/${PKG_STUB_LIB_FILE}"
	NSF_BUILD_STUB_LIB_SPEC="-L${NSF_BUILD_DIR} -lnsfstub${PACKAGE_VERSION}"
	NSF_STUB_LIB_SPEC="-L${NSF_PKG_LIBDIR} -lnsfstub${PACKAGE_VERSION}"
	AC_DEFINE([COMPILE_NSF_STUBS])
fi

else

	NSF_BUILD_LIB_SPEC="${NSF_SRC_DIR}/${PKG_LIB_FILE}"
	NSF_LIB_SPEC="${NSF_PKG_LIBDIR}/${PKG_LIB_FILE}"
	NSF_BUILD_STUB_LIB_PATH="${NSF_BUILD_DIR}/${PKG_STUB_LIB_FILE}"
	NSF_STUB_LIB_PATH="${NSF_PKG_LIBDIR}/${PKG_STUB_LIB_FILE}"
	NSF_BUILD_STUB_LIB_SPEC="${NSF_BUILD_DIR}/${PKG_STUB_LIB_FILE}"
	NSF_STUB_LIB_SPEC="${NSF_PKG_LIBDIR}/${PKG_STUB_LIB_FILE}"
	AC_DEFINE([COMPILE_NSF_STUBS])

fi

AC_SUBST([SHARED_LIB_SUFFIX])
AC_SUBST([UNSHARED_LIB_SUFFIX])
AC_SUBST([NSF_BUILD_LIB_SPEC])
AC_SUBST([NSF_LIB_SPEC])
AC_SUBST([NSF_BUILD_STUB_LIB_SPEC])
AC_SUBST([NSF_STUB_LIB_SPEC])
AC_SUBST([NSF_BUILD_STUB_LIB_PATH])
AC_SUBST([NSF_STUB_LIB_PATH])
AC_SUBST([NXSH])
AC_SUBST([XOWISH])

# include dirs for nsf during build process (i.e., does not assume installed)
NSF_BUILD_INCLUDE_DIR="${NSF_SRC_DIR}/generic"
NSF_BUILD_INCLUDE_SPEC="-I${NSF_BUILD_INCLUDE_DIR}"
AC_SUBST([NSF_BUILD_INCLUDE_DIR])
AC_SUBST([NSF_BUILD_INCLUDE_SPEC])


eval "NSF_INCLUDE_DIR=\"$includedir\""
AC_SUBST([NSF_INCLUDE_DIR])

AC_SUBST([test_actiweb])
AC_SUBST([libdirs_actiweb])
AC_SUBST([apps_actiweb])

AC_SUBST([TEA_PLATFORM])

dnl macro expanding to the names of files ./configure is to generate.
dnl reasoning: this is a factoring; I use this value elsewhere.
dnl
dnl Change the value of -this- macro if you want to add or remove
dnl such files.

AC_DEFUN([CONFIG_OUTPUT_FILES], [[Makefile ${srcdir}/nsfConfig.sh ${srcdir}/library/xotcl/xotclsh ${srcdir}/library/xotcl/xowish nxsh nxwish ${srcdir}/doc/version.inc]])

#--------------------------------------------------------------------
# the value of this variable is set to the files which are to be
# removed when the user invokes 'make distclean' (i.e., those 
# files generated by ./configure) and is used in the make distclean
# target, defined in Makefile.in
#--------------------------------------------------------------------

CONFIG_CLEAN_FILES="CONFIG_OUTPUT_FILES autom4te.cache/"
AC_SUBST([CONFIG_CLEAN_FILES])
AC_CONFIG_FILES(CONFIG_OUTPUT_FILES)

#--------------------------------------------------------------------
# Finally, substitute all of the various values into the Makefile,
# and generate the other output files. (this is done by invoking
# config.status)
# 
# NOTE the lack of parameters! AC_OUTPUT with params is deprecated;
# use macros such as AC_CONFIG_FILES, AC_HEADER_FILES, etc to add
# to the files output by ./configure.
#--------------------------------------------------------------------

AC_OUTPUT

chmod +x nxsh nxwish ${srcdir}/library/xotcl/xotclsh ${srcdir}/library/xotcl/xowish

for subdir in ${subdirs}
do
	echo "==================== configure $subdir"
	if test x"${srcdir}" = x. ; then
		confdir=.
	else
 		#mkdir -p $subdir
		confdir=${srcdir}/$subdir
	fi
    (cd $subdir; echo $SHELL ${confdir}/configure ${ac_configure_args} --prefix=${prefix} --with-nsf=${NSF_SRC_DIR}; eval $SHELL ${confdir}/configure ${ac_configure_args} --prefix=${prefix} --with-nsf=${NSF_SRC_DIR})
done


