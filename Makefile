# Makefile.in --
#
#	This file is a Makefile for Sample TEA Extension.  If it has the name
#	"Makefile.in" then it is a template for a Makefile;  to generate the
#	actual Makefile, run "./configure", which is a configuration script
#	generated by the "autoconf" program (constructs like "@foo@" will get
#	replaced in the actual Makefile.
#
# Copyright (c) 1999 Scriptics Corporation.
# Copyright (c) 2002-2003 ActiveState Corporation.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# RCS: @(#) $Id: Makefile.in,v 1.21 2006/10/04 20:40:23 neumann Exp $

#========================================================================
# Add additional lines to handle any additional AC_SUBST cases that
# have been added in a customized configure script.
#========================================================================

#XOTCL_VERSION 	= 1.6.0
XOTCL_VERSION 	= 1.6

src_lib_dir 	= ${srcdir}/library
src_doc_dir 	= ${srcdir}/doc
src_test_dir 	= ${srcdir}/tests
src_app_dir 	= ${srcdir}/apps
src_generic_dir = ${srcdir}/generic
TCL_LIB_SPEC    = -L/usr/local/aolserver-4.5/lib -ltcl8.4
TK_LIB_SPEC	= 
subdirs		= 
aol_prefix	= /usr/local/aolserver-4.5

# Requires native paths
PLATFORM_DIR    = `echo $(srcdir)/unix`
target_doc_dir 	= ./doc

src_lib_dir_native	= `echo ${src_lib_dir}`
src_doc_dir_native	= `echo ${src_doc_dir}`
src_test_dir_native 	= `echo ${src_test_dir}`
src_app_dir_native	= `echo ${src_app_dir}`
src_generic_dir_native	= `echo ${src_generic_dir}`

libdirs = comm lib serialize 
libsrc 	= COPYRIGHT pkgIndex.tcl 
appdirs = comm scripts utils 
appsrc 	= COPYRIGHT 

DOC_SOURCE = \
	$(src_doc_dir)/langRef.xotcl \
	$(src_lib_dir)/lib/*.xotcl \
	$(src_lib_dir)/store/*.xotcl \
	$(src_lib_dir)/serialize/Serializer.xotcl \
	$(src_test_dir)/*.xotcl \
	$(src_app_dir)/scripts/*.xotcl \
	$(src_app_dir)/comm/[flsw]*.xotcl \
	$(src_app_dir)/actiweb/univ/UNIVERSAL.xotcl \
	$(src_app_dir)/utils/xo-*[a-z0-9] 

#export TCLLIBPATH=. ${srcdir}
mkinstalldirs= mkdir -p
#========================================================================
# Nothing of the variables below this line should need to be changed.
# Please check the TARGETS section below to make sure the make targets
# are correct.
#========================================================================

#========================================================================
# The names of the source files is defined in the configure script.
# The object files are used for linking into the final library.
# This will be used when a dist target is added to the Makefile.
# It is not important to specify the directory, as long as it is the
# $(srcdir) or in the generic, win or unix subdirectory.
#========================================================================

PKG_SOURCES	=  xotcl.c xotclError.c xotclMetaData.c xotclObjectData.c xotclProfile.c xotclTrace.c xotclUtil.c xotclShadow.c xotclCompile.c aolstub.c xotclStubInit.c
PKG_OBJECTS	=  xotcl.o xotclError.o xotclMetaData.o xotclObjectData.o xotclProfile.o xotclTrace.o xotclUtil.o xotclShadow.o xotclCompile.o aolstub.o xotclStubInit.o

PKG_STUB_SOURCES =  xotclStubLib.c
PKG_STUB_OBJECTS =  xotclStubLib.o

#========================================================================
# PKG_TCL_SOURCES identifies Tcl runtime files that are associated with
# this package that need to be installed, if any.
#========================================================================

PKG_TCL_SOURCES = 

#========================================================================
# This is a list of public header files to be installed, if any.
#========================================================================

PKG_HEADERS	=  generic/xotcl.h generic/xotclInt.h generic/xotclDecls.h generic/xotclIntDecls.h

#========================================================================
# "PKG_LIB_FILE" refers to the library (dynamic or static as per
# configuration options) composed of the named objects.
#========================================================================

PKG_LIB_FILE	= libxotcl1.6.0.dylib
PKG_STUB_LIB_FILE = libxotclstub1.6.0.a

lib_BINARIES	= $(PKG_LIB_FILE) $(PKG_STUB_LIB_FILE)
BINARIES	= $(lib_BINARIES)

SHELL		= /bin/sh

srcdir		= .
prefix		= /usr/local/aolserver-4.5
exec_prefix	= /usr/local/aolserver-4.5

bindir		= ${exec_prefix}/bin
libdir		= /usr/local/aolserver-4.5/lib
datadir		= /usr/local/aolserver-4.5/share
mandir		= ${prefix}/man
includedir	= /usr/local/aolserver-4.5/include

DESTDIR		=

top_builddir	= .

INSTALL		= /usr/bin/install -c
INSTALL_PROGRAM	= ${INSTALL}
INSTALL_DATA	= ${INSTALL} -m 644
INSTALL_SCRIPT	= ${INSTALL}

PACKAGE_NAME	= xotcl
PACKAGE_VERSION	= 1.6.0
CC		= gcc -pipe
CFLAGS_DEFAULT	= -g
CFLAGS_WARNING	= -Wall -Wno-implicit-int
CLEANFILES	= *.o *.a *.so *~ core gmon.out
EXEEXT		= 
LDFLAGS_DEFAULT	=  -prebind -headerpad_max_install_names -Wl,-search_paths_first 
MAKE_LIB	= ${SHLIB_LD} -o $@ $(PKG_OBJECTS) ${SHLIB_LD_LIBS} 
MAKE_SHARED_LIB	= ${SHLIB_LD} -o $@ $(PKG_OBJECTS) ${SHLIB_LD_LIBS}
MAKE_STATIC_LIB	= ${STLIB_LD} $@ $(PKG_OBJECTS)
MAKE_STUB_LIB	= ${STLIB_LD} $@ $(PKG_STUB_OBJECTS)
OBJEXT		= o
RANLIB		= :
RANLIB_STUB	= ranlib
SHLIB_CFLAGS	= -fno-common
SHLIB_LD	= ${CC} -dynamiclib ${CFLAGS} ${LDFLAGS_DEFAULT} -Wl,-single_module
SHLIB_LD_FLAGS	= @SHLIB_LD_FLAGS@
SHLIB_LD_LIBS	= ${LIBS} -L/usr/local/aolserver-4.5/lib -ltclstub8.4
STLIB_LD	= ${AR} cr
TCL_DEFS	=  -DNO_VALUES_H=1 -DHAVE_LIMITS_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SYS_PARAM_H=1 -DUSE_THREAD_ALLOC=1 -D_REENTRANT=1 -D_THREAD_SAFE=1 -DHAVE_PTHREAD_ATTR_SETSTACKSIZE=1 -DHAVE_PTHREAD_ATFORK=1 -DTCL_THREADS=1 -DHAVE_COREFOUNDATION=1 -DMAC_OSX_TCL=1 -DTCL_WIDE_INT_TYPE=long\ long -DHAVE_GETCWD=1 -DHAVE_OPENDIR=1 -DHAVE_STRSTR=1 -DHAVE_STRTOL=1 -DHAVE_STRTOLL=1 -DHAVE_STRTOULL=1 -DHAVE_TMPNAM=1 -DHAVE_WAITPID=1 -DHAVE_GETPWUID_R_5=1 -DHAVE_GETPWUID_R=1 -DHAVE_GETPWNAM_R_5=1 -DHAVE_GETPWNAM_R=1 -DHAVE_GETGRGID_R_5=1 -DHAVE_GETGRGID_R=1 -DHAVE_GETGRNAM_R_5=1 -DHAVE_GETGRNAM_R=1 -DHAVE_MTSAFE_GETHOSTBYNAME=1 -DHAVE_MTSAFE_GETHOSTBYADDR=1 -DUSE_TERMIOS=1 -DHAVE_SYS_TIME_H=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_TM_ZONE=1 -DHAVE_GMTIME_R=1 -DHAVE_LOCALTIME_R=1 -DHAVE_TM_GMTOFF=1 -DHAVE_ST_BLKSIZE=1 -DSTDC_HEADERS=1 -DHAVE_SIGNED_CHAR=1 -DHAVE_PUTENV_THAT_COPIES=1 -DHAVE_LANGINFO=1 -DHAVE_COPYFILE=1 -DHAVE_LIBKERN_OSATOMIC_H=1 -DHAVE_OSSPINLOCKLOCK=1 -DHAVE_PTHREAD_ATFORK=1 -DUSE_VFORK=1 -DTCL_DEFAULT_ENCODING=\"utf-8\" -DTCL_LOAD_FROM_MEMORY=1 -DHAVE_AVAILABILITYMACROS_H=1 -DHAVE_FTS=1 -DHAVE_SYS_IOCTL_H=1 -DHAVE_SYS_FILIO_H=1 
TCL_BIN_DIR	= /usr/local/aolserver-4.5/lib
TCL_SRC_DIR	= /usr/local/src/tcl8.4.14
# This is necessary for packages that use private Tcl headers
#TCL_TOP_DIR_NATIVE	= "/usr/local/src/tcl8.4.14"
# Not used, but retained for reference of what libs Tcl required
TCL_LIBS	= ${DL_LIBS} ${LIBS} ${MATH_LIBS}

pkgdatadir	= /usr/local/aolserver-4.5/share/xotcl1.6.0
pkglibdir	= /usr/local/aolserver-4.5/lib/xotcl1.6.0
pkgincludedir	= /usr/local/aolserver-4.5/include/xotcl1.6.0

# XOTCLSH = xotclsh

#========================================================================
# TCLLIBPATH seeds the auto_path in Tcl's init.tcl so we can test our
# package without installing.  The other environment variables allow us
# to test against an uninstalled Tcl.  Add special env vars that you
# require for testing here (like TCLX_LIBRARY).
#========================================================================

EXTRA_PATH	= $(top_builddir):$(TCL_BIN_DIR)
TCLSH_ENV	= TCL_LIBRARY=`echo $(TCL_SRC_DIR)/library` \
		  DYLD_LIBRARY_PATH="$(EXTRA_PATH):$(DYLD_LIBRARY_PATH)" \
		  PATH="$(EXTRA_PATH):$(PATH)" \
		  TCLLIBPATH="$(top_builddir) ${srcdir}"
TCLSH_PROG	= /usr/local/aolserver-4.5/bin/tclsh8.4
TCLSH		= $(TCLSH_ENV) $(TCLSH_PROG)
SHARED_BUILD	= 1

INCLUDES	=  -I"/usr/local/src/tcl8.4.14/generic" -I"/usr/local/src/tcl8.4.14/unix" -I./generic
EXTRA_CFLAGS	=  -DXOTCLVERSION=\"1.6\" -DXOTCLPATCHLEVEL=\".0\" 	 -DHAVE_TCL_COMPILE_H=1

# TCL_DEFS is not strictly need here, but if you remove it, then you
# must make sure that configure.in checks for the necessary components
# that your library may use.  TCL_DEFS can actually be a problem if
# you do not compile with a similar machine setup as the Tcl core was
# compiled with.
#DEFS		= $(TCL_DEFS) -DPACKAGE_NAME=\"xotcl\" -DPACKAGE_TARNAME=\"xotcl\" -DPACKAGE_VERSION=\"1.6.0\" -DPACKAGE_STRING=\"xotcl\ 1.6.0\" -DPACKAGE_BUGREPORT=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DNO_VALUES_H=1 -DHAVE_LIMITS_H=1 -DHAVE_SYS_PARAM_H=1 -DUSE_THREAD_ALLOC=1 -D_REENTRANT=1 -D_THREAD_SAFE=1 -DTCL_THREADS=1 -DTCL_WIDE_INT_TYPE=long\ long -DUSE_TCL_STUBS=1 -DCOMPILE_XOTCL_STUBS=1  $(EXTRA_CFLAGS)
DEFS		= -DPACKAGE_NAME=\"xotcl\" -DPACKAGE_TARNAME=\"xotcl\" -DPACKAGE_VERSION=\"1.6.0\" -DPACKAGE_STRING=\"xotcl\ 1.6.0\" -DPACKAGE_BUGREPORT=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DNO_VALUES_H=1 -DHAVE_LIMITS_H=1 -DHAVE_SYS_PARAM_H=1 -DUSE_THREAD_ALLOC=1 -D_REENTRANT=1 -D_THREAD_SAFE=1 -DTCL_THREADS=1 -DTCL_WIDE_INT_TYPE=long\ long -DUSE_TCL_STUBS=1 -DCOMPILE_XOTCL_STUBS=1  $(EXTRA_CFLAGS) 

CONFIG_CLEAN_FILES = Makefile xotclConfig.sh apps/utils/xotclsh apps/utils/xowish unix/xotcl.spec unix/pkgIndex.unix autom4te.cache/

CPPFLAGS	= 
LIBS		=  
AR		= ar
CFLAGS		=  ${CFLAGS_DEFAULT} ${CFLAGS_WARNING} ${SHLIB_CFLAGS}
COMPILE		= $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)

#========================================================================
# Start of user-definable TARGETS section
#========================================================================

#========================================================================
# TEA TARGETS.  Please note that the "libraries:" target refers to platform
# independent files, and the "binaries:" target inclues executable programs and
# platform-dependent libraries.  Modify these targets so that they install
# the various pieces of your package.  The make and install rules
# for the BINARIES that you specified above have already been done.
#========================================================================

all: binaries libraries doc end

#========================================================================
# The binaries target builds executable programs, Windows .dll's, unix
# shared/static libraries, and any other platform-dependent files.
# The list of targets to build for "binaries:" is specified at the top
# of the Makefile, in the "BINARIES" variable.
#========================================================================

binaries: $(BINARIES) xotclsh  pkgIndex.tcl
	@if test ! "x$(subdirs)" = "x" ; then dirs="$(subdirs)" ; \
	for dir in $$dirs ; do \
	   if (cd $$dir; $(MAKE) $@) ; then true ; else exit 1 ; fi ; \
	done; fi;

libraries: 
	@if test ! "x$(subdirs)" = "x" ; then dirs="$(subdirs)" ; \
	for dir in $$dirs ; do \
	   if (cd $$dir; $(MAKE) $@) ; then true ; else exit 1 ; fi ; \
	done; fi;

libraries-pkgindex:
	@$(TCLSH) $(src_lib_dir_native)/lib/make.xotcl -dir $(src_lib_dir_native) -all

fulldoc: doc pdf
# use language reference as sample file to trigger generation of documentation files
doc: $(target_doc_dir)/langRef-xotcl.html

$(target_doc_dir)/langRef-xotcl.html: $(src_doc_dir)/langRef.xotcl $(DOC_SOURCE)
	@docs=""; \
	for i in $(DOC_SOURCE); do docs="$$docs `echo $$i`"; done; \
	$(TCLSH) $(src_lib_dir_native)/lib/makeDoc.xotcl \
		$(target_doc_dir) $$docs

pdf:
	-(cd $(src_doc_dir); htmldoc --webpage --format pdf14 --title \
		-f tutorial.pdf tutorial.html )
	-(cd $(src_doc_dir); htmldoc --webpage --format pdf14  \
		-f langRef-xotcl.pdf langRef-xotcl.html )

install: install-binaries install-shells install-libraries install-doc
	@if test ! "x$(subdirs)" = "x" ; then dirs="$(subdirs)" ; \
	for dir in $$dirs ; do \
	   if (cd $$dir; $(MAKE) $@) ; then true ; else exit 1 ; fi ; \
	done; fi;

install-binaries: binaries install-lib-binaries install-bin-binaries install-pkgIndex

install-aol: install-binaries install-libraries
	$(INSTALL) $(src_generic_dir)/aol-xotcl.tcl \
		$(DESTDIR)/$(aol_prefix)/modules/tcl/xotcl.tcl


#========================================================================
# This rule installs platform-independent files, such as header files.
#========================================================================
install-libraries: libraries $(DESTDIR)$(includedir) $(DESTDIR)$(pkglibdir)
	@echo "Installing header files in $(DESTDIR)$(includedir)"
	@for i in $(PKG_HEADERS) ; do \
	    echo "    Installing $$i" ; \
	    $(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(includedir) ; \
	done;
	@echo "Installing Libraries to $(DESTDIR)$(pkglibdir)/"
	@for i in $(libdirs) ; do \
	    echo "    Installing $$i/" ; \
	    rm -rf $(DESTDIR)$(pkglibdir)/$$i ; \
	    mkdir -p $(DESTDIR)$(pkglibdir)/$$i; \
	    chmod 755 $(DESTDIR)$(pkglibdir)/$$i; \
	    for j in $(src_lib_dir)/$$i/*.*tcl ; do \
		$(INSTALL_DATA) $$j $(DESTDIR)$(pkglibdir)/$$i/; \
	    done; \
	done;
	@for i in $(libsrc) ; do \
	    echo "    Installing $$i" ; \
	    rm -rf $(DESTDIR)$(pkglibdir)/$$i ; \
	    $(INSTALL_DATA) $(src_lib_dir)/$$i $(DESTDIR)$(pkglibdir)/$$i ; \
	done;
	cat unix/pkgIndex.unix >> $(DESTDIR)$(pkglibdir)/pkgIndex.tcl
	$(INSTALL_DATA) xotclConfig.sh $(DESTDIR)$(libdir)/
	@echo "Installing Applications to $(DESTDIR)$(pkglibdir)/apps/"
	@for i in $(appdirs) ; do \
	    echo "    Installing $$i/" ; \
	    rm -rf $(DESTDIR)$(pkglibdir)/apps/$$i ; \
	    mkdir -p $(DESTDIR)$(pkglibdir)/apps/$$i; \
	    chmod 755 $(DESTDIR)$(pkglibdir)/apps/$$i; \
  	    for j in $(src_app_dir)/$$i/* ; do \
		if test -d $$j; then \
		    mkdir -p $(DESTDIR)$(pkglibdir)/$$j; \
		    chmod 755 $(DESTDIR)$(pkglibdir)/$$j; \
		    for k in $$j/* ; do \
			$(INSTALL) $$k $(DESTDIR)$(pkglibdir)/$$j ; \
		done; \
		else \
		$(INSTALL) $$j $(DESTDIR)$(pkglibdir)/apps/$$i/; \
		fi; \
  	    done; \
	done;
	@for i in $(appsrc) ; do \
	    echo "    Installing $$i" ; \
	    rm -rf $(DESTDIR)$(pkglibdir)/apps/$$i ; \
	    $(INSTALL_DATA) $(src_app_dir)/$$i $(DESTDIR)$(pkglibdir)/apps ; \
	done;
	@rm -rf $(DESTDIR)$(pkglibdir)/store/XOTclGdbm
	@rm -rf $(DESTDIR)$(pkglibdir)/store/XOTclSdbm
	@rm -rf $(DESTDIR)$(pkglibdir)/xml/TclExpat-1.1

#========================================================================
# Install documentation.  Unix manpages should go in the $(DESTDIR)$(mandir)
# directory.
#========================================================================

install-doc: doc $(DESTDIR)$(mandir)/man1 $(DESTDIR)$(mandir)/man3 $(DESTDIR)$(mandir)/mann
	@if test ! "x$(XOTCLSH)" = "x" ; then \
	(cd $(src_man_dir)/ ; \
	 for i in *.1; do \
	    echo "Installing $$i"; \
	    rm -f $(DESTDIR)$(mandir)/man1/$$i; \
	    sed -e '/man\.macros/r man.macros' -e '/man\.macros/d' \
		    $$i > $(DESTDIR)$(mandir)/man1/$$i; \
	    chmod 444 $(DESTDIR)$(mandir)/man1/$$i; \
	    done) ; \
	fi

shell: binaries libraries
	@$(TCLSH) $(SCRIPT)

gdb:
	$(TCLSH_ENV) gdb $(TCLSH_PROG) $(SCRIPT)

test: binaries libraries test-core test-http 
test-nohttp: binaries libraries test-core

#TESTFLAGS = -srcdir $(srcdir)
test-core: $(TCLSH_PROG)
	$(TCLSH) $(src_test_dir_native)/testx.xotcl \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)
	$(TCLSH) $(src_test_dir_native)/testo.xotcl \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)
	$(TCLSH) $(src_test_dir_native)/speedtest.xotcl \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)
	$(TCLSH) $(src_test_dir_native)/forwardtest.xotcl \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)
	$(TCLSH) $(src_test_dir_native)/slottest.xotcl \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)

test-http: $(TCLSH_PROG)
	$(TCLSH) $(src_test_dir_native)/xocomm.test \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)

test-actiweb: $(TCLSH_PROG)
	$(TCLSH) $(src_test_dir_native)/actiweb.test \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)
	$(TCLSH) $(src_test_dir_native)/persistence.test \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)
	$(TCLSH) $(src_test_dir_native)/UNIVERSAL.test \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)
	$(TCLSH) $(src_test_dir_native)/xoRDF.test \
		-libdir $(PLATFORM_DIR) $(TESTFLAGS)
	@rm -rf receiver

depend:


#========================================================================
# $(PKG_LIB_FILE) should be listed as part of the BINARIES variable
# mentioned above.  That will ensure that this target is built when you
# run "make binaries".
#
# The $(PKG_OBJECTS) objects are created and linked into the final
# library.  In most cases these object files will correspond to the
# source files above.
#========================================================================

$(PKG_LIB_FILE): $(PKG_OBJECTS)
	-rm -f $(PKG_LIB_FILE)
	${MAKE_LIB}
	$(RANLIB) $(PKG_LIB_FILE)

$(PKG_STUB_LIB_FILE): $(PKG_STUB_OBJECTS)
	-rm -f $(PKG_STUB_LIB_FILE)
	${MAKE_STUB_LIB}
	$(RANLIB_STUB) $(PKG_STUB_LIB_FILE)

#========================================================================
# We need to enumerate the list of .c to .o lines here.
#
# In the following lines, $(srcdir) refers to the toplevel directory
# containing your extension.  If your sources are in a subdirectory,
# you will have to modify the paths to reflect this:
#
# sample.$(OBJEXT): $(srcdir)/generic/sample.c
# 	$(COMPILE) -c `echo $(srcdir)/generic/sample.c` -o $@
#
# Setting the VPATH variable to a list of paths will cause the makefile
# to look into these paths when resolving .c to .obj dependencies.
# As necessary, add $(srcdir):$(srcdir)/compat:....
#========================================================================

VPATH = $(srcdir)/generic:$(srcdir)/unix:$(srcdir)/win

.c.o:
	$(COMPILE) -c `echo $<` -o $@

#========================================================================
# xotcl shells
#========================================================================

pkgIndex.tcl: $(PKG_LIB_FILE)
	@echo package ifneeded XOTcl $(PACKAGE_VERSION) [list load [file join \$$dir . $(PKG_LIB_FILE)] XOTcl] > pkgIndex.tcl

install-pkgIndex:
#	@echo package ifneeded XOTcl $(PACKAGE_VERSION) [list load [file join \$$dir .. "$(PKG_LIB_FILE)"] XOTcl] > "$(pkglibdir)/pkgIndex.tcl"

xotclsh: tclAppInit.o $(PKG_OBJECTS) $(CONDITIONAL_STUB_OBJECTS)
	$(CC) -rdynamic -o $@ tclAppInit.o \
		$(PKG_OBJECTS) \
		$(CFLAGS)  $(TCL_LIB_SPEC) \
		$(DMALLOC_LIB) $(CONDITIONAL_STUB_OBJECTS)

xowish: tkAppInit.o $(PKG_OBJECTS) $(CONDITIONAL_STUB_OBJECTS)
	$(CC) -rdynamic -o $@ tkAppInit.o \
		$(PKG_OBJECTS) \
		$(CFLAGS)  $(TCL_LIB_SPEC) $(TK_LIB_SPEC) \
		$(DMALLOC_LIB) $(CONDITIONAL_STUB_OBJECTS)

install-shells:
	@if test -f xotclsh; then \
		$(INSTALL_PROGRAM) xotclsh $(DESTDIR)$(bindir); \
	fi
	@if test -f xowish; then \
		$(INSTALL_PROGRAM) xowish $(DESTDIR)$(bindir); \
	fi

#========================================================================
# We need to enumerate the list of .c to .o lines here.
# Unfortunately, there does not seem to be any other way to do this
# in a Makefile-independent way.  We can't use VPATH because it picks up
# object files that may be located in the source directory.
#
# In the following lines, $(srcdir) refers to the toplevel directory
# containing your extension.  If your sources are in a subdirectory,
# you will have to modify the paths to reflect this:
#
# exampleA.$(OBJEXT): $(srcdir)/src/win/exampleA.c
# 	$(COMPILE) -c `echo $(srcdir)/src/win/exampleA.c` -o $@
#========================================================================

$(src_generic_dir)/predefined.h: $(src_generic_dir)/mk_predefined.xotcl $(src_generic_dir)/predefined.xotcl
	(cd $(src_generic_dir); $(TCLSH) mk_predefined.xotcl > predefined.h)

xotclStubInit.$(OBJEXT): $(PKG_HEADERS)
xotclStubLib.$(OBJEXT): $(src_generic_dir)/xotclStubLib.c $(PKG_HEADERS)
xotcl.$(OBJEXT): $(src_generic_dir)/xotcl.c $(src_generic_dir)/predefined.h $(PKG_HEADERS)
xotclError.$(OBJEXT): $(src_generic_dir)/xotclError.c $(PKG_HEADERS)
xotclMetaData.$(OBJEXT): $(src_generic_dir)/xotclMetaData.c $(PKG_HEADERS)
xotclObjectData.$(OBJEXT): $(src_generic_dir)/xotclObjectData.c $(PKG_HEADERS)
xotclProfile.$(OBJEXT): $(src_generic_dir)/xotclProfile.c $(PKG_HEADERS)
xotclTrace.$(OBJEXT): $(src_generic_dir)/xotclTrace.c $(PKG_HEADERS)
xotclUtil.$(OBJEXT): $(src_generic_dir)/xotclUtil.c $(PKG_HEADERS)
xotclShadow.$(OBJEXT): $(src_generic_dir)/xotclShadow.c $(PKG_HEADERS)
aolstub.$(OBJEXT): $(src_generic_dir)/aolstub.c $(PKG_HEADERS)

#
# Target to regenerate header files and stub files from the *.decls tables.
#

genstubs:
	$(TCLSH) $(TCL_SRC_DIR)/tools/genStubs.tcl $(src_generic_dir) \
		$(src_generic_dir)/xotcl.decls $(src_generic_dir)/xotclInt.decls

#
# Target to check that all exported functions have an entry in the stubs
# tables.
#

checkstubs:
	-@for i in `nm -p $(PKG_LIB_FILE) | awk '$$2 ~ /T/ { print $$3 }' \
		| sort -n`; do \
		match=0; \
		for j in $(TCL_DECLS); do \
		    if [ `grep -c $$i $$j` -gt 0 ]; then \
			match=1; \
		    fi; \
		done; \
		if [ $$match -eq 0 ]; then echo $$i; fi \
	done

#========================================================================
# End of user-definable section
#========================================================================

#========================================================================
# Don't modify the file to clean here.  Instead, set the "CLEANFILES"
# variable in configure.in
#========================================================================

clean:  
	-rm -rf $(BINARIES) $(CLEANFILES) xotclsh  pkgIndex.tcl ./receiver \
		$(target_doc_dir)/*-xotcl.html
	find ${srcdir} -type f -name \*~ -exec rm \{} \;
	@if test ! "x$(subdirs)" = "x" ; then dirs="$(subdirs)" ; \
	for dir in $$dirs ; do \
	   if (cd $$dir; $(MAKE) $@) ; then true ; else exit 1 ; fi ; \
	done; fi

distclean: clean
	-rm -rf Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log config.status
	@if test ! "x$(subdirs)" = "x" ; then dirs="$(subdirs)" ; \
	for dir in $$dirs ; do \
	   if (cd $$dir; $(MAKE) $@) ; then true ; else exit 1 ; fi ; \
	done; fi

#========================================================================
# Install binary object libraries.  On Windows this includes both .dll and
# .lib files.  Because the .lib files are not explicitly listed anywhere,
# we need to deduce their existence from the .dll file of the same name.
# Library files go into the lib directory.
# In addition, this will generate the pkgIndex.tcl
# file in the install location (assuming it can find a usable tclsh shell)
#
# You should not have to modify this target.
#========================================================================

install-lib-binaries:
	@mkdir -p $(DESTDIR)$(pkglibdir)
	@list='$(lib_BINARIES)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(INSTALL_PROGRAM) $$p $(DESTDIR)$(pkglibdir)/$$p"; \
	    $(INSTALL_PROGRAM) $$p $(DESTDIR)$(pkglibdir)/$$p; \
	    stub=`echo $$p|sed -e "s/.*\(stub\).*/\1/"`; \
	    if test "x$$stub" = "xstub"; then \
		echo " $(RANLIB_STUB) $(DESTDIR)$(pkglibdir)/$$p"; \
		$(RANLIB_STUB) $(DESTDIR)$(pkglibdir)/$$p; \
	    else \
		echo " $(RANLIB) $(DESTDIR)$(pkglibdir)/$$p"; \
		$(RANLIB) $(DESTDIR)$(pkglibdir)/$$p; \
		ln -s $(DESTDIR)$(pkglibdir)/$$p $(DESTDIR)$(libdir)/$$p; \
	    fi; \
	    ext=`echo $$p|sed -e "s/.*\.//"`; \
	    if test "x$$ext" = "xdll"; then \
		lib=`basename $$p|sed -e 's/.[^.]*$$//'`.lib; \
		if test -f $$lib; then \
		    echo " $(INSTALL_DATA) $$lib $(DESTDIR)$(pkglibdir)/$$lib"; \
	            $(INSTALL_DATA) $$lib $(DESTDIR)$(pkglibdir)/$$lib; \
		fi; \
	    fi; \
	  fi; \
	done
	@list='$(PKG_TCL_SOURCES)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    destp=`basename $$p`; \
	    echo " Install $$destp $(DESTDIR)$(pkglibdir)/$$destp"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(pkglibdir)/$$destp; \
	  fi; \
	done


#========================================================================
# Install binary executables (e.g. .exe files and dependent .dll files)
# This is for files that must go in the bin directory (located next to
# wish and tclsh), like dependent .dll files on Windows.
#
# You should not have to modify this target, except to define bin_BINARIES
# above if necessary.
#========================================================================

install-bin-binaries:
	@mkdir -p $(DESTDIR)$(bindir)
	@list='$(bin_BINARIES)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/$$p"; \
	    $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/$$p; \
	  fi; \
	done

.SUFFIXES: .c .$(OBJEXT)

#Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
#	cd $(top_builddir) \
#	  && CONFIG_FILES=$@ CONFIG_HEADERS= $(SHELL) ./config.status

uninstall-binaries:
	list='$(lib_BINARIES)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(pkglibdir)/$$p; \
	done
	list='$(PKG_TCL_SOURCES)'; for p in $$list; do \
	  p=`basename $$p`; \
	  rm -f $(DESTDIR)$(pkglibdir)/$$p; \
	done
	list='$(bin_BINARIES)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/$$p; \
	done

$(DESTDIR)$(includedir):
	$(mkinstalldirs) $@
$(DESTDIR)$(bindir):
	$(mkinstalldirs)  $@
$(DESTDIR)$(libdir):
	$(mkinstalldirs)  $@
$(DESTDIR)$(pkglibdir):
	$(mkinstalldirs)  $@
$(DESTDIR)$(pkglibdir)/apps: $(DESTDIR)$(pkglibdir)
	$(mkinstalldirs)  $@
$(DESTDIR)$(mandir)/man1:
	$(mkinstalldirs)  $@
$(DESTDIR)$(mandir)/man3:
	$(mkinstalldirs)  $@
$(DESTDIR)$(mandir)/mann:
	$(mkinstalldirs)  $@

end:
	@echo "" 
	@echo "************************************************************"
	@echo " Make completed. In order to test XOTcl, invoke:"
	@echo "   make test"
	@echo ""
	@echo " In order install XOTcl, invoke:"
	@echo "   make install"
	@echo ""
	@echo " In order to install XOTcl for AOLserver 4.x, invoke:"
	@echo "   make install-aol"
	@echo "" 
	@echo " In order to invoke XOTcl interactively (before install), use:" 
	@echo "   export TCLLIBPATH=\"$(TCLLIBPATH)\"   or    " 
	@echo "   setenv TCLLIBPATH \"$(TCLLIBPATH)\""
	@echo " and"
	@if test "x$(XOTCLSH)" = "x" ; then \
	  echo "   /usr/local/aolserver-4.5/bin/tclsh8.4" ; \
	  echo "   package require XOTcl; namespace import -force xotcl::*" ; \
	  echo " or" ; \
	  echo "   put the 'package require' line into your ~/.tclshrc" ; \
	else \
	  echo "   ./xotclsh" ; \
	fi
	@echo "************************************************************"

RPMSOURCES=/usr/src/redhat/SOURCES
RPMSPECS=/usr/src/redhat/SPECS

rpm:
	@if test ! -d $(RPMSOURCES); then mkdir -p $(RPMSOURCES); fi
	@if test ! -d $(RPMSPECS); then mkdir -p $(RPMSPECS); fi
	cp unix/xotcl.spec $(RPMSPECS)/xotcl-$(PACKAGE_VERSION).spec
	make tar
	cp ../xotcl-$(PACKAGE_VERSION).tar.gz $(RPMSOURCES)
	rpmbuild -ba $(RPMSPECS)/xotcl-$(PACKAGE_VERSION).spec

bin-tar: 
	(cd ..; tar zcvf xotcl-$(PACKAGE_VERSION)-bin-linux-i686-glibc.tar.gz \
                `find $(exec_prefix)/bin/xotclsh $(exec_prefix)/bin/xowish \
                  $(prefix)/lib/xotcl* \
                  $(prefix)/lib/libxotcl* \
		  $(prefix)/include/xotcl*.h \
                  $(DESTDIR)$(pkglibdir) $(prefix)/man/man1/xo* \
                -type f -o -type l | fgrep -v CVS | fgrep -v SCCS | fgrep -v .junk| fgrep -v .db | fgrep -v "~" | fgrep -v "#" | fgrep -v /receiver/` \
	)

tar:  libraries-pkgindex
	sh ./config/mktar.sh


.PHONY: all binaries clean depend distclean doc install libraries \
	test test-core test-actiweb 

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
