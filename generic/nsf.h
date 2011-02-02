/* -*- Mode: c++ -*-
 *
 *  Next Scripting Framework
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *  This software is based upon MIT Object Tcl by David Wetherall and
 *  Christopher J. Lindblad, that contains the following copyright
 *  message:
 * 
 *  Copyright 1993 Massachusetts Institute of Technology
 * 
 *  Permission to use, copy, modify, distribute, and sell this
 *  software and its documentation for any purpose is hereby granted
 *  without fee, provided that the above copyright notice appear in
 *  all copies and that both that copyright notice and this permission
 *  notice appear in supporting documentation, and that the name of
 *  M.I.T. not be used in advertising or publicity pertaining to
 *  distribution of the software without specific, written prior
 *  permission.  M.I.T. makes no representations about the suitability
 *  of this software for any purpose.  It is provided "as is" without
 *  express or implied warranty.
 * */

#ifndef _nsf_h_
#define _nsf_h_

#include "tcl.h"

#undef TCL_STORAGE_CLASS
#ifdef BUILD_nsf
# define TCL_STORAGE_CLASS DLLEXPORT
#else
# ifdef USE_NSF_STUBS
#  define TCL_STORAGE_CLASS
# else
#  define TCL_STORAGE_CLASS DLLIMPORT
# endif
#endif

/*
 * prevent old TCL-versions
 */

#if TCL_MAJOR_VERSION < 8
# error Tcl distribution is TOO OLD, we require at least tcl8.5
#endif

#if TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION<5
# error Tcl distribution is TOO OLD, we require at least tcl8.5
#endif

#if TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION<6
# define PRE86
#endif

#if defined(PRE86)
# define CONST86
# define Tcl_GetErrorLine(interp) (interp)->errorLine
#else
# define NRE
#endif

/*
 * Feature activation/deactivation
 */

/* activate bytecode support 
#define NSF_BYTECODE
*/

/* activate/deacticate profiling information at the end
   of running the program
#define NSF_PROFILE 1
*/

/* are we developing?
#define NSF_DEVELOPMENT 1
*/
#define NSF_DEVELOPMENT 1

/* activate/deacticate assert 
#define NDEBUG 1
*/

/* additional language features
#define NSF_WITH_INHERIT_NAMESPACES 1
*/

#define NSF_WITH_OS_RESOLVER 1
#define NSF_WITH_ASSERTIONS 1
#define NSF_WITH_VALUE_WARNINGS 1

/* activate/deacticate memory tracing 
#define NSF_MEM_TRACE 1
#define NSF_MEM_COUNT 1
*/

/* turn  tracing output on/off
#define NSFOBJ_TRACE 1
#define CALLSTACK_TRACE 1
#define NAMESPACE_TRACE 1
#define OBJDELETION_TRACE 1
#define STACK_TRACE 1
#define PARSE_TRACE 1
#define PARSE_TRACE_FULL 1
#define CONFIGURE_ARGS_TRACE 1
#define TCL_STACK_ALLOC_TRACE 1
#define VAR_RESOLVER_TRACE 1
#define CMD_RESOLVER_TRACE 1
#define NRE_CALLBACK_TRACE 1
*/


/*
 * Sanity checks and dependencies for optional compile flags
 */
#if defined(PARSE_TRACE_FULL)
# define PARSE_TRACE 1
#endif

#ifdef NSF_MEM_COUNT
# define DO_FULL_CLEANUP 1
#endif

#ifdef AOL_SERVER
# ifndef TCL_THREADS
#  define TCL_THREADS
# endif
#endif

#ifdef TCL_THREADS
# define DO_CLEANUP
#endif

#ifdef DO_FULL_CLEANUP
# define DO_CLEANUP
#endif

#ifdef NSF_DEVELOPMENT
# define CHECK_ACTIVATION_COUNTS 1
# define NsfCleanupObject(object,string)				\
  /*fprintf(stderr, "NsfCleanupObject %p %s\n",object,string);*/	\
  NsfCleanupObject_(object)
# define CscFinish(interp,cscPtr,string)				\
  /*fprintf(stderr, "CscFinish %p %s\n",cscPtr,string);*/		\
  CscFinish_(interp, cscPtr)
#else
# define NDEBUG 1
# define NsfCleanupObject(object,string)	\
  NsfCleanupObject_(object)
# define CscFinish(interp,cscPtr,string)	\
  CscFinish_(interp, cscPtr)
#endif

#if defined(NSF_PROFILE)
# define CscInit(cscPtr, object, cl, cmd, frametype, flags, method) \
  CscInit_(cscPtr, object, cl, cmd, frametype, flags); cscPtr->methodName = (method);
#else
# define CscInit(cscPtr, object, cl, cmd, frametype, flags, methodName) \
  CscInit_(cscPtr, object, cl, cmd, frametype, flags)
#endif

#if !defined(CHECK_ACTIVATION_COUNTS)
# define CscListAdd(interp, cscPtr)
# define CscListRemove(interp, cscPtr)
#endif


/* 
 * A special definition used to allow this header file to be included 
 * in resource files so that they can get obtain version information from
 * this file.  Resource compilers don't like all the C stuff, like typedefs
 * and procedure declarations, that occur below.
 */

#ifndef RC_INVOKED

/*
 * The structures Nsf_Object and Nsf_Class define mostly opaque 
 * data structures for the internal use strucures NsfObject and 
 * NsfClass (both defined in NsfInt.h). Modification of elements 
 * visible elements must be mirrored in both incarnations.
 */

typedef struct Nsf_Object {
  Tcl_Obj *cmdName;
} Nsf_Object;

typedef struct Nsf_Class {
  struct Nsf_Object object;
} Nsf_Class;

typedef struct Nsf_Param {
} Nsf_Param;

/*
 * Include the public function declarations that are accessible via
 * the stubs table.
 */
#include "nsfDecls.h"

/*
 * Nsf_InitStubs is used by extensions  that can be linked
 * against the nsf stubs library.  If we are not using stubs
 * then this reduces to package require.
 */

#ifdef USE_NSF_STUBS

# ifdef __cplusplus
extern "C"
# endif
CONST char *
Nsf_InitStubs _ANSI_ARGS_((Tcl_Interp *interp, CONST char *version, int exact));
#else
# define Nsf_InitStubs(interp, version, exact) \
      Tcl_PkgRequire(interp, "nx", version, exact)
#endif

#endif /* RC_INVOKED */

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _nsf_h_ */
