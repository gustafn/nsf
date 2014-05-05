# nsf.m4 --
#
#	This file provides a set of autoconf macros to help TEA-enable
#	a Tcl extension.
#
# Copyright (c) 1999 Scriptics Corporation.
# Copyright (c) 1999-2008 Uwe Zdun
# Copyright (c) 1999-2014 Gustaf Neumann
#
# See the file "tcl-license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

#------------------------------------------------------------------------
# SC_PATH_NSFCONFIG --
#
#	Locate the nsfConfig.sh file and perform a sanity check on
#	the Tcl compile flags
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-nsf=...
#
#	Defines the following vars:
#		NX_BIN_DIR	Full path to the directory containing
#				the nsfConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN(SC_PATH_NSFCONFIG, [
    #
    # Ok, lets find the tcl configuration
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-tcl
    #
    if test x"${no_nsf}" = x ; then
	# we reset no_nsf in case something fails here
	no_nsf=true
	AC_ARG_WITH(nsf, [  --with-nsf              directory containing nsf configuration (nsfConfig.sh)], with_nsfconfig=${withval})
	AC_MSG_CHECKING([for nsf configuration])
	AC_CACHE_VAL(ac_cv_c_nsfconfig,[

	    # First check to see if --with-nsf was specified.
	    if test x"${with_nsfconfig}" != x ; then
		if test -f "${with_nsfconfig}/nsfConfig.sh" ; then
		    ac_cv_c_nsfconfig=`(cd ${with_nsfconfig}; pwd)`
		else
		    AC_MSG_ERROR([${with_nsfconfig} directory doesn't contain nsfConfig.sh])
		fi
	    fi

	    # then check for a private Tcl installation
	    if test x"${ac_cv_c_nsfconfig}" = x ; then
		for i in \
			${srcdir}/../nsf \
			`ls -dr ${srcdir}/../nsf-* 2>/dev/null` \
			${srcdir}/../../nsf \
			`ls -dr ${srcdir}/../../nsf-* 2>/dev/null` \
			${srcdir}/../../../nsf \
			`ls -dr ${srcdir}/../../../nsf-* 2>/dev/null` \
			${srcdir}/../../../../nsf \
			`ls -dr ${srcdir}/../../../../nsf-* 2>/dev/null` \
			${srcdir}/../../../../../nsf \
			`ls -dr ${srcdir}/../../../../../nsf-* 2>/dev/null` ; do
		    if test -f "$i/nsfConfig.sh" ; then
			ac_cv_c_nsfconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_nsfconfig}" = x ; then
		for i in `ls -d ${prefix}/lib 2>/dev/null` \
			`ls -d /usr/local/lib 2>/dev/null` ; do
		    if test -f "$i/nsfConfig.sh" ; then
			ac_cv_c_nsfconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi

	])

	if test x"${ac_cv_c_nsfconfig}" = x ; then
	    NX_BIN_DIR="# no nsf configs found"
	    AC_MSG_WARN(Can't find nsf configuration definitions)
	    exit 0
	else
	    no_nsf=
	    NX_BIN_DIR=${ac_cv_c_nsfconfig}
	    AC_MSG_RESULT(found $NX_BIN_DIR/nsfConfig.sh)
	fi
    fi
])

#------------------------------------------------------------------------
# SC_LOAD_NSFCONFIG --
#
#	Load the tclConfig.sh file
#
# Arguments:
#	
#	Requires the following vars to be set:
#		NX_BIN_DIR
#
# Results:
#
#	Subst the vars:
#
#------------------------------------------------------------------------

AC_DEFUN(SC_LOAD_NSFCONFIG, [
    AC_MSG_CHECKING([for existence of $NX_BIN_DIR/nsfConfig.sh])

    if test -f "$NX_BIN_DIR/nsfConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. $NX_BIN_DIR/nsfConfig.sh
    else
        AC_MSG_RESULT([file not found])
    fi

    #
    # The eval is required to do the TCL_DBGX substitution in the
    # TCL_LIB_FILE variable
    #
    AC_SUBST(NX_VERSION)
    AC_SUBST(NX_MAJOR_VERSION)
    AC_SUBST(NX_MINOR_VERSION)
    AC_SUBST(NX_RELEASE_LEVEL)
    AC_SUBST(NX_LIB_FILE)
    AC_SUBST(NX_BUILD_LIB_SPEC)
    AC_SUBST(NX_LIB_SPEC)
    AC_SUBST(NX_STUB_LIB_FILE)
    AC_SUBST(NX_BUILD_STUB_LIB_SPEC)
    AC_SUBST(NX_STUB_LIB_SPEC)
    AC_SUBST(NX_SRC_DIR)
])

