/* -*- Mode: c++ -*-
 *  Next Scripting Framework
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *  nsfInt.h --
 *
 *  Mostly internally used API Functions
 */

#ifndef _nsf_int_h_
#define _nsf_int_h_

#if defined(HAVE_STDINT_H)
# define HAVE_INTPTR_T 
# define HAVE_UINTPTR_T 
#endif

#include <tclInt.h>
#include "nsf.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(HAVE_TCL_COMPILE_H)
# include <tclCompile.h>
#endif

#if defined(NSF_PROFILE)
#  include <sys/time.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef BUILD_nsf
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * Makros
 */

#if defined(PRE86)
# define Tcl_NRCallObjProc(interp, proc, cd, objc, objv) \
  (*(proc))((cd), (interp), (objc), (objv))
#endif

#ifdef NSF_MEM_COUNT
Tcl_HashTable nsfMemCount;
extern int nsfMemCountInterpCounter;
typedef struct NsfMemCounter {
  int peak;
  int count;
} NsfMemCounter;
#  define MEM_COUNT_ALLOC(id,p) NsfMemCountAlloc(id,p)
#  define MEM_COUNT_FREE(id,p) NsfMemCountFree(id,p)
#  define MEM_COUNT_INIT() \
      if (nsfMemCountInterpCounter == 0) { \
        Tcl_InitHashTable(&nsfMemCount, TCL_STRING_KEYS); \
        nsfMemCountInterpCounter = 1; \
      }
#  define MEM_COUNT_DUMP() NsfMemCountDump(interp)
#  define MEM_COUNT_OPEN_FRAME()
/*if (obj->varTable) noTableBefore = 0*/
#  define MEM_COUNT_CLOSE_FRAME()
/*      if (obj->varTable && noTableBefore) \
	NsfMemCountAlloc("obj->varTable",NULL)*/
#else
#  define MEM_COUNT_ALLOC(id,p)
#  define MEM_COUNT_FREE(id,p)
#  define MEM_COUNT_INIT()
#  define MEM_COUNT_DUMP()
#  define MEM_COUNT_OPEN_FRAME()
#  define MEM_COUNT_CLOSE_FRAME()
#endif

#define DSTRING_INIT(dsPtr) Tcl_DStringInit(dsPtr); MEM_COUNT_ALLOC("DString",dsPtr)
#define DSTRING_FREE(dsPtr) Tcl_DStringFree(dsPtr); MEM_COUNT_FREE("DString",dsPtr)

#if USE_ASSOC_DATA
# define RUNTIME_STATE(interp) ((NsfRuntimeState*)Tcl_GetAssocData((interp), "NsfRuntimeState", NULL))
#else
# define RUNTIME_STATE(interp) ((NsfRuntimeState*)((Interp*)(interp))->globalNsPtr->clientData)
#endif


#define ALLOC_NAME_NS(DSP, NS, NAME) \
     DSTRING_INIT(DSP);\
     Tcl_DStringAppend(DSP, NS, -1),\
     Tcl_DStringAppend(DSP, "::", 2),\
     Tcl_DStringAppend(DSP, NAME, -1)
#define ALLOC_TOP_NS(DSP, NAME) \
     DSTRING_INIT(DSP);\
     Tcl_DStringAppend(DSP, "::", 2),\
     Tcl_DStringAppend(DSP, NAME, -1)
#define ALLOC_DSTRING(DSP,ENTRY) \
     DSTRING_INIT(DSP);\
     Tcl_DStringAppend(DSP, ENTRY, -1)

#define nr_elements(arr)  ((int) (sizeof(arr) / sizeof(arr[0])))

# define NEW(type) \
	(type *)ckalloc(sizeof(type)); MEM_COUNT_ALLOC(#type, NULL)
# define NEW_ARRAY(type,n) \
	(type *)ckalloc(sizeof(type)*(n)); MEM_COUNT_ALLOC(#type "*", NULL)
# define FREE(type, var) \
	ckfree((char*) var); MEM_COUNT_FREE(#type,var)

#define isAbsolutePath(m) (*m == ':' && m[1] == ':')
#define isArgsString(m) (\
	*m   == 'a' && m[1] == 'r' && m[2] == 'g' && m[3] == 's' && \
	m[4] == '\0')
#define isBodyString(m) (\
	*m   == 'b' && m[1] == 'o' && m[2] == 'd' && m[3] == 'y' && \
	m[4] == '\0')
#define isCheckString(m) (\
	*m   == 'c' && m[1] == 'h' && m[2] == 'e' && m[3] == 'c' && \
	m[4] == 'k' && m[5] == '\0')
#define isCheckObjString(m) (\
        *m   == 'c' && m[1] == 'h' && m[2] == 'e' && m[3] == 'c' && \
	m[4] == 'k' && m[5] == 'o' && m[6] == 'b' && m[7] == 'j' && \
	m[8] == '\0')
#define isCreateString(m) (\
	*m   == 'c' && m[1] == 'r' && m[2] == 'e' && m[3] == 'a' && \
	m[4] == 't' && m[5] == 'e' && m[6] == '\0')
#define isInitString(m) (\
	*m   == 'i' && m[1] == 'n' && m[2] == 'i' && m[3] == 't' && \
	m[4] == '\0')
#define isTypeString(m) (\
	*m   == 't' && m[1] == 'y' && m[2] == 'p' && m[3] == 'e' && \
	m[4] == '\0')
#define isObjectString(m) (\
	*m   == 'o' && m[1] == 'b' && m[2] == 'j' && m[3] == 'e' && \
	m[4] == 'c' && m[5] == 't' && m[6] == '\0')
#define isClassString(m) (\
	*m   == 'c' && m[1] == 'l' && m[2] == 'a' && m[3] == 's' && \
	m[4] == 's' && m[5] == '\0')

#if (defined(sun) || defined(__hpux)) && !defined(__GNUC__)
#  define USE_ALLOCA
#endif

#if defined(__IBMC__) && !defined(__GNUC__)
# if __IBMC__ >= 0x0306
#  define USE_ALLOCA
# else
#  define USE_MALLOC
# endif
#endif

#if defined(VISUAL_CC)
#  define USE_MALLOC
#endif

#if defined(__GNUC__) && !defined(USE_ALLOCA) && !defined(USE_MALLOC)
# if !defined(NDEBUG)
#  define ALLOC_ON_STACK(type,n,var) \
    int __##var##_count = (n); type __##var[n+2]; \
    type *var = __##var + 1; var[-1] = var[__##var##_count] = (type)0xdeadbeaf
#  define FREE_ON_STACK(type,var)                                       \
  assert(var[-1] == var[__##var##_count] && var[-1] == (type)0xdeadbeaf)
# else
#  define ALLOC_ON_STACK(type,n,var) type var[(n)]
#  define FREE_ON_STACK(type,var)
# endif
#elif defined(USE_ALLOCA)
#  define ALLOC_ON_STACK(type,n,var) type *var = (type *)alloca((n)*sizeof(type))
#  define FREE_ON_STACK(type,var)
#else
#  define ALLOC_ON_STACK(type,n,var) type *var = (type *)ckalloc((n)*sizeof(type))
#  define FREE_ON_STACK(type,var) ckfree((char*)var)
#endif

#ifdef USE_ALLOCA
# include <alloca.h>
#endif

#ifdef  __WIN32__
# define NSF_INLINE
# define NsfNewObj(A) A=Tcl_NewObj()
# define DECR_REF_COUNT(A) \
	MEM_COUNT_FREE("INCR_REF_COUNT",A); Tcl_DecrRefCount(A)
#else
/*
 * This was defined to be inline for anything !sun or __IBMC__ >= 0x0306,
 * but __hpux should also be checked - switched to only allow in gcc - JH
 */
# if defined(__GNUC__)
#  define NSF_INLINE inline
# else
#  define NSF_INLINE
# endif
# ifdef USE_TCL_STUBS
#  define NsfNewObj(A) A=Tcl_NewObj()
#  define DECR_REF_COUNT(A) \
	MEM_COUNT_FREE("INCR_REF_COUNT",A); assert((A)->refCount > -1); \
        Tcl_DecrRefCount(A)
# else
#  define NsfNewObj(A) TclNewObj(A)
#  define DECR_REF_COUNT(A) \
	MEM_COUNT_FREE("INCR_REF_COUNT",A); TclDecrRefCount(A)
# endif
#endif

#if defined(TCL_THREADS)
# define NsfMutex Tcl_Mutex
# define NsfMutexLock(a) Tcl_MutexLock(a)
# define NsfMutexUnlock(a) Tcl_MutexUnlock(a)
#else
# define NsfMutex int
# define NsfMutexLock(a)   (*(a))++
# define NsfMutexUnlock(a) (*(a))--
#endif

#define ObjStr(obj) (obj)->bytes ? (obj)->bytes : Tcl_GetString(obj)

#define INCR_REF_COUNT(A) MEM_COUNT_ALLOC("INCR_REF_COUNT",A); Tcl_IncrRefCount(A)

#ifdef OBJDELETION_TRACE
# define PRINTOBJ(ctx,obj) \
  fprintf(stderr, "  %s %p %s oid=%p teardown=%p destroyCalled=%d\n", \
          ctx,obj,(obj)->teardown?ObjStr((obj)->cmdName):"(deleted)", \
          (obj)->id, (obj)->teardown,                                 \
          ((obj)->flags & NSF_DESTROY_CALLED))
#else
# define PRINTOBJ(ctx,obj)
#endif

#define className(cl) (((cl) ? ObjStr(cl->object.cmdName) : "NULL"))
#define objectName(obj) (((obj) ? ObjStr(obj->cmdName) : "NULL"))


#define LONG_AS_STRING 32

/* TCL_CONTINUE is defined as 4, from 5 on we can
   use app-specific return codes */
#define NSF_CHECK_FAILED 6

/*
 *
 * Next Scripting Structures
 *
 */

/*
 * Filter structures
 */
typedef struct NsfFilterStack {
  Tcl_Command currentCmdPtr;
  Tcl_Obj *calledProc;
  struct NsfFilterStack *nextPtr;
} NsfFilterStack;

typedef struct NsfTclObjList {
  Tcl_Obj *content;
  struct NsfTclObjList *nextPtr;
} NsfTclObjList;

/*
 * Assertion structures
 */

typedef struct NsfProcAssertion {
  NsfTclObjList *pre;
  NsfTclObjList *post;
} NsfProcAssertion;

typedef struct NsfAssertionStore {
  NsfTclObjList *invariants;
  Tcl_HashTable procs;
} NsfAssertionStore;

typedef enum { /* powers of 2; add to ALL, if default; */
  CHECK_NONE  = 0, CHECK_CLINVAR = 1, CHECK_OBJINVAR = 2,
  CHECK_PRE   = 4, CHECK_POST = 8,
  CHECK_INVAR = CHECK_CLINVAR + CHECK_OBJINVAR,
  CHECK_ALL   = CHECK_INVAR   + CHECK_PRE + CHECK_POST
} CheckOptions;

void NsfAssertionRename(Tcl_Interp *interp, Tcl_Command cmd,
			  NsfAssertionStore *as,
			  char *oldSimpleCmdName, char *newName);
/*
 * mixins
 */
typedef struct NsfMixinStack {
  Tcl_Command currentCmdPtr;
  struct NsfMixinStack *nextPtr;
} NsfMixinStack;

/*
 * Generic command pointer list
 */
typedef struct NsfCmdList {
  Tcl_Command cmdPtr;
  ClientData clientData;
  struct NsfClass *clorobj;
  struct NsfCmdList *nextPtr;
} NsfCmdList;

typedef void (NsfFreeCmdListClientData) _ANSI_ARGS_((NsfCmdList*));

/* for incr string */
typedef struct NsfStringIncrStruct {
  char *buffer;
  char *start;
  size_t bufSize;
  int length;
} NsfStringIncrStruct;

/* 
 * cmd flags
 */

#define NSF_CMD_PROTECTED_METHOD 		0x00010000
#define NSF_CMD_REDEFINE_PROTECTED_METHOD	0x00020000
/* NSF_CMD_NONLEAF_METHOD is used to flag, if a Method implemented via cmd calls "next" */
#define NSF_CMD_NONLEAF_METHOD			0x00040000
#define NSF_CMD_CLASS_ONLY_METHOD		0x00080000
/*
 * object flags ...
 */

/* DESTROY_CALLED indicates that destroy was called on obj */
#define NSF_DESTROY_CALLED                 0x0001
/* INIT_CALLED indicates that init was called on obj */
#define NSF_INIT_CALLED                    0x0002
/* MIXIN_ORDER_VALID set when mixin order is valid */
#define NSF_MIXIN_ORDER_VALID              0x0004
/* MIXIN_ORDER_DEFINED set, when mixins are defined for obj */
#define NSF_MIXIN_ORDER_DEFINED            0x0008
#define NSF_MIXIN_ORDER_DEFINED_AND_VALID  0x000c
/* FILTER_ORDER_VALID set, when filter order is valid */
#define NSF_FILTER_ORDER_VALID             0x0010
/* FILTER_ORDER_DEFINED set, when filters are defined for obj */
#define NSF_FILTER_ORDER_DEFINED           0x0020
#define NSF_FILTER_ORDER_DEFINED_AND_VALID 0x0030
/* class and object properties for objects */
#define NSF_IS_CLASS                       0x0040
#define NSF_IS_ROOT_META_CLASS             0x0080
#define NSF_IS_ROOT_CLASS                  0x0100
#define NSF_IS_SLOT_CONTAINER              0x0200
/* deletion state */
#define NSF_TCL_DELETE                     0x0400
#define NSF_DESTROY_CALLED_SUCCESS         0x0800
#define NSF_DURING_DELETE                  0x2000
#define NSF_DELETED                        0x4000
#define NSF_RECREATE                       0x8000


/* flags for NsfParams */

#define NSF_ARG_REQUIRED		0x0001
#define NSF_ARG_MULTIVALUED		0x0002
#define NSF_ARG_NOARG 		     	0x0004
#define NSF_ARG_CURRENTLY_UNKNOWN	0x0008
#define NSF_ARG_SUBST_DEFAULT		0x0010
#define NSF_ARG_ALLOW_EMPTY		0x0020
#define NSF_ARG_INITCMD		     	0x0040
#define NSF_ARG_METHOD		     	0x0080
#define NSF_ARG_RELATION		0x0100
#define NSF_ARG_SWITCH		     	0x0200
#define NSF_ARG_BASECLASS	     	0x0400
#define NSF_ARG_METACLASS	     	0x0800
#define NSF_ARG_HAS_DEFAULT		0x1000
#define NSF_ARG_IS_CONVERTER		0x2000
#define NSF_ARG_IS_ENUMERATION		0x4000

/* disallowed options */
#define NSF_DISALLOWED_ARG_METHOD_PARAMETER	     (NSF_ARG_METHOD|NSF_ARG_INITCMD|NSF_ARG_RELATION)
#define NSF_DISALLOWED_ARG_SETTER	     	     (NSF_ARG_SUBST_DEFAULT|NSF_DISALLOWED_ARG_METHOD_PARAMETER)
#define NSF_DISALLOWED_ARG_OBJECT_PARAMETER	     0
#define NSF_DISALLOWED_ARG_VALUEECHECK	     (NSF_ARG_SUBST_DEFAULT|NSF_ARG_METHOD|NSF_ARG_INITCMD|NSF_ARG_RELATION|NSF_ARG_SWITCH|NSF_ARG_CURRENTLY_UNKNOWN)


/* method types */
#define NSF_METHODTYPE_ALIAS     0x0001
#define NSF_METHODTYPE_SCRIPTED  0x0002
#define NSF_METHODTYPE_SETTER    0x0004
#define NSF_METHODTYPE_FORWARDER 0x0008
#define NSF_METHODTYPE_OBJECT    0x0010
#define NSF_METHODTYPE_OTHER     0x0100
#define NSF_METHODTYPE_BUILTIN   NSF_METHODTYPE_ALIAS|NSF_METHODTYPE_SETTER|NSF_METHODTYPE_FORWARDER|NSF_METHODTYPE_OTHER
#define NSF_METHODTYPE_ALL       NSF_METHODTYPE_SCRIPTED|NSF_METHODTYPE_BUILTIN|NSF_METHODTYPE_OBJECT

/* flags for ParseContext */
#define NSF_PC_MUST_DECR		     0x0001

#define NsfObjectSetClass(obj) \
	(obj)->flags |= NSF_IS_CLASS
#define NsfObjectClearClass(obj) \
	(obj)->flags &= ~NSF_IS_CLASS
#define NsfObjectIsClass(obj) \
	((obj)->flags & NSF_IS_CLASS)
#define NsfObjectToClass(obj) \
	(NsfClass*)((((NsfObject*)obj)->flags & NSF_IS_CLASS)?obj:0)


/*
 * object and class internals
 */
struct NsfParam;
typedef int (NsfTypeConverter)(Tcl_Interp *interp, 
				 Tcl_Obj *obj,
                                 struct NsfParam CONST *pPtr, 
				 ClientData *clientData, 
				 Tcl_Obj **outObjPtr);

typedef struct NsfParam {
  char *name;
  int flags;
  int nrArgs;
  NsfTypeConverter *converter;
  Tcl_Obj *converterArg;
  Tcl_Obj *defaultValue;
  CONST char *type;
  Tcl_Obj *nameObj;
  Tcl_Obj *converterName;
  Tcl_Obj *paramObj;
  Tcl_Obj *slotObj;
} NsfParam;

typedef struct NsfParamDefs {
  NsfParam *paramsPtr;
  int nrParams;
  Tcl_Obj *slotObj;
  Tcl_Obj *returns;
} NsfParamDefs;

typedef struct NsfParsedParam {
  NsfParamDefs *paramDefs;
  int possibleUnknowns;
} NsfParsedParam;

typedef struct NsfObjectOpt {
  NsfAssertionStore *assertions;
  NsfCmdList *filters;
  NsfCmdList *mixins;
  ClientData clientData;
  CONST char *volatileVarName;
  short checkoptions;
} NsfObjectOpt;

typedef struct NsfObject {
  Tcl_Obj *cmdName;
  Tcl_Command id;
  Tcl_Interp *teardown;
  struct NsfClass *cl;
  TclVarHashTable *varTablePtr;
  Tcl_Namespace *nsPtr;
  NsfObjectOpt *opt;
  struct NsfCmdList *filterOrder;
  struct NsfCmdList *mixinOrder;
  NsfFilterStack *filterStack;
  NsfMixinStack *mixinStack;
  int refCount;
  short flags;
  short activationCount;
} NsfObject;

typedef struct NsfObjects {
  struct NsfObject *obj;
  struct NsfObjects *nextPtr;
} NsfObjects;

typedef struct NsfClassOpt {
  NsfCmdList *classfilters;
  NsfCmdList *classmixins;
  NsfCmdList *isObjectMixinOf;
  NsfCmdList *isClassMixinOf;
  NsfAssertionStore *assertions;
#ifdef NSF_OBJECTDATA
  Tcl_HashTable *objectdata;
#endif
  Tcl_Command id;
  ClientData clientData;
} NsfClassOpt;

typedef struct NsfClass {
  struct NsfObject object;
  struct NsfClasses *super;
  struct NsfClasses *sub;
  struct NsfObjectSystem *osPtr;
  struct NsfClasses *order;
  Tcl_HashTable instances;
  Tcl_Namespace *nsPtr;
  NsfParsedParam *parsedParamPtr;
  NsfClassOpt *opt;
  short color;
} NsfClass;

typedef struct NsfClasses {
  struct NsfClass *cl;
  ClientData clientData;
  struct NsfClasses *nextPtr;
} NsfClasses;

typedef enum SystemMethodsIdx {
  NSF_c_alloc_idx, 
  NSF_c_create_idx, 
  NSF_c_dealloc_idx,
  NSF_c_recreate_idx, 
  NSF_c_requireobject_idx, 
  NSF_o_cleanup_idx, 
  NSF_o_configure_idx, 
  NSF_o_defaultmethod_idx, 
  NSF_o_destroy_idx, 
  NSF_o_init_idx, 
  NSF_o_move_idx, 
  NSF_o_objectparameter_idx, 
  NSF_o_residualargs_idx,
  NSF_o_unknown_idx
} SystemMethodsIdx;

#if !defined(NSF_C)
extern CONST char *Nsf_SytemMethodOpts[];
#else 
CONST char *Nsf_SytemMethodOpts[] = {
  "-class.alloc", 
  "-class.create", 
  "-class.dealloc",
  "-class.recreate", 
  "-class.requireobject",
  "-object.cleanup", 
  "-object.configure", 
  "-object.defaultmethod", 
  "-object.destroy", 
  "-object.init", 
  "-object.move", 
  "-object.objectparameter", 
  "-object.residualargs", 
  "-object.unknown",  
  NULL
};
#endif

typedef struct NsfObjectSystem {
  NsfClass *rootClass;
  NsfClass *rootMetaClass;
  int overloadedMethods;
  int definedMethods;
  Tcl_Obj *methods[NSF_o_unknown_idx+1];
  struct NsfObjectSystem *nextPtr;
} NsfObjectSystem;




/* Next Scripting global names and strings */
/* these are names and contents for global (corresponding) Tcl_Objs
   and Strings - otherwise these "constants" would have to be built
   every time they are used; now they are built once in Nsf_Init */
typedef enum {
  NSF_EMPTY, NSF_ONE,
  /* methods called internally */
  NSF_CONFIGURE, 
  /* var names */
  NSF_AUTONAMES, NSF_DEFAULTMETACLASS, NSF_DEFAULTSUPERCLASS, 
  NSF_ALIAS_ARRAY,
  /* object/class names */
  NSF_METHOD_PARAMETER_SLOT_OBJ, 
  /* constants */
  NSF_ALIAS, NSF_ARGS, NSF_CMD, NSF_FILTER, NSF_FORWARD, 
  NSF_METHOD, NSF_OBJECT, NSF_SETTER, 
  NSF_GUARD_OPTION, NSF___UNKNOWN__, 
  /* Partly redefined Tcl commands; leave them together at the end */
  NSF_EXPR, NSF_FORMAT, NSF_INFO, NSF_INFO_FRAME, NSF_INTERP, NSF_IS, 
  NSF_RENAME, NSF_SUBST, NSF_VARIABLE
} NsfGlobalNames;
#if !defined(NSF_C)
extern char *NsfGlobalStrings[];
#else
char *NsfGlobalStrings[] = {
  "", "1", 
  /* methods called internally */
  "configure", 
  /* var names */
  "__autonames", "__default_metaclass", "__default_superclass", 
  "::nsf::alias",
  /* object/class names */
  "::nx::methodParameterSlot", 
  /* constants */
  "alias", "args", "cmd", "filter",  "forward", 
  "method", "object", "setter", 
  "-guard", "__unknown__",
  /* tcl commands */
  "expr", "format", "info", "::tcl::info::frame", "interp", "::tcl::string::is", 
  "rename", "subst", "variable"
};
#endif

#define NsfGlobalObjs RUNTIME_STATE(interp)->methodObjNames

/* Next Scripting ShadowTclCommands */
typedef struct NsfShadowTclCommandInfo {
  TclObjCmdProcType proc;
  ClientData clientData;
} NsfShadowTclCommandInfo;
typedef enum {SHADOW_LOAD=1, SHADOW_UNLOAD=0, SHADOW_REFETCH=2} NsfShadowOperations;

int NsfCallCommand(Tcl_Interp *interp, NsfGlobalNames name,
		     int objc, Tcl_Obj *CONST objv[]);
int NsfShadowTclCommands(Tcl_Interp *interp, NsfShadowOperations load);
Tcl_Obj * NsfMethodObj(Tcl_Interp *interp, NsfObject *object, int methodIdx);


/*
 * Next Scripting CallStack
 */
typedef struct NsfCallStackContent {
  NsfObject *self;
  NsfClass *cl;
  Tcl_Command cmdPtr;
  NsfFilterStack *filterStackEntry;
  Tcl_Obj *CONST* objv;
  int objc;
  unsigned short frameType;
  unsigned short flags;
} NsfCallStackContent;

#define NSF_CSC_TYPE_PLAIN              0
#define NSF_CSC_TYPE_ACTIVE_MIXIN       1
#define NSF_CSC_TYPE_ACTIVE_FILTER      2
#define NSF_CSC_TYPE_INACTIVE           4
#define NSF_CSC_TYPE_INACTIVE_MIXIN     5
#define NSF_CSC_TYPE_INACTIVE_FILTER    6
#define NSF_CSC_TYPE_GUARD           0x10
#define NSF_CSC_TYPE_ENSEMBLE        0x20

#define NSF_CSC_CALL_IS_NEXT             1
#define NSF_CSC_CALL_IS_GUARD            2
#define NSF_CSC_CALL_IS_ENSEMBLE         4
#define NSF_CSC_IMMEDIATE           0x0020
#define NSF_CSC_FORCE_FRAME         0x0040
#define NSF_CSC_CALL_IS_NRE         0x0100
#define NSF_CSC_MIXIN_STACK_PUSHED  0x0200
#define NSF_CSC_FILTER_STACK_PUSHED 0x0400
#define NSF_CSC_UNKNOWN             0x0800
#define NSF_CSC_CALL_IS_TRANSPARENT 0x1000
#define NSF_CSC_OBJECT_ACTIVATED    0x2000
#define NSF_CSC_COPY_FLAGS          (NSF_CSC_MIXIN_STACK_PUSHED|NSF_CSC_FILTER_STACK_PUSHED|NSF_CSC_IMMEDIATE|NSF_CSC_CALL_IS_TRANSPARENT|NSF_CSC_FORCE_FRAME)

/* flags for call method */
#define NSF_CM_NO_UNKNOWN 1
#define NSF_CM_NO_SHIFT   2
#define NSF_CM_NO_PROTECT 4
#define NSF_CM_NO_OBJECT_METHOD 8

#if defined(NRE)
# define NsfImmediateFromCallerFlags(flags) \
  (((flags) & (NSF_CSC_CALL_IS_NRE|NSF_CSC_IMMEDIATE)) == NSF_CSC_CALL_IS_NRE ? 0 : NSF_CSC_IMMEDIATE)

//#define NRE_SANE_PATCH 1

#if defined(NRE_SANE_PATCH)
# define NsfNRRunCallbacks(interp, result, rootPtr) TclNRRunCallbacks(interp, result, rootPtr)
#else
# define NsfNRRunCallbacks(interp, result, rootPtr) TclNRRunCallbacks(interp, result, rootPtr, 0)
#endif

#endif

#if defined(NSF_PROFILE)
typedef struct NsfProfile {
  long int overallTime;
  Tcl_HashTable objectData;
  Tcl_HashTable methodData;
} NsfProfile;
#endif

typedef struct NsfRuntimeState {
  Tcl_Namespace *NsfClassesNS;
  Tcl_Namespace *NsfNS;
  /*
   * definitions of the main nsf objects
   */
  struct NsfObjectSystem *objectSystems;
  Tcl_ObjCmdProc *objInterpProc;
  Tcl_Obj **methodObjNames;
  struct NsfShadowTclCommandInfo *tclCommands;
  int errorCount;
  /* these flags could move into a bitarray, but are used only once per interp*/
  int unknown;
  int doSoftrecreate;
  int doKeepinitcmd;
  int doCheckResults;
  int doCheckArguments;
  int doFilters;
  int debugLevel;
  int exitHandlerDestroyRound;
  int returnCode;
  int overloadedMethods;
  long newCounter;
  NsfStringIncrStruct iss;
  Proc fakeProc;
  Tcl_Namespace *fakeNS;
  NsfStubs *nsfStubs;
  Tcl_CallFrame *varFramePtr;
  Tcl_Command cmdPtr; /* used for ACTIVE_MIXIN */
  Tcl_Command colonCmd;
#if defined(NSF_PROFILE)
  NsfProfile profile;
#endif
  short guardCount;
  ClientData clientData;
} NsfRuntimeState;

#define NSF_EXITHANDLER_OFF 0
#define NSF_EXITHANDLER_ON_SOFT_DESTROY 1
#define NSF_EXITHANDLER_ON_PHYSICAL_DESTROY 2


#ifdef NSF_OBJECTDATA
extern void
NsfSetObjectData(struct NsfObject *obj, struct NsfClass *cl, ClientData data);
extern int
NsfGetObjectData(struct NsfObject *obj, struct NsfClass *cl, ClientData *data);
extern int
NsfUnsetObjectData(struct NsfObject *obj, struct NsfClass *cl);
extern void
NsfFreeObjectData(NsfClass *cl);
#endif

/*
 *
 *  internally used API functions
 *
 */

#include "nsfIntDecls.h"

/*
 * Profiling functions
 */

#if defined(NSF_PROFILE)
extern void
NsfProfileFillTable(Tcl_HashTable *table, Tcl_DString *key,
		 double totalMicroSec);
extern void
NsfProfileEvaluateData(Tcl_Interp *interp, long int startSec, long int startUsec,
		    NsfObject *obj, NsfClass *cl, char *methodName);
extern void
NsfProfilePrintTable(Tcl_HashTable *table);

extern void
NsfProfilePrintData(Tcl_Interp *interp);

extern void
NsfProfileInit(Tcl_Interp *interp);
#endif

/*
 * MEM Counting
 */
#ifdef NSF_MEM_COUNT
void NsfMemCountAlloc(char *id, void *);
void NsfMemCountFree(char *id, void *);
void NsfMemCountDump();
#endif /* NSF_MEM_COUNT */

/*
 * TCL_STACK_ALLOC_TRACE
 */
#if defined(TCL_STACK_ALLOC_TRACE)
# define NsfTclStackFree(interp,ptr,msg) \
  fprintf(stderr, "---- TclStackFree %p %s\n", ptr, msg);\
  TclStackFree(interp,ptr)

static char *
NsfTclStackAlloc(Tcl_Interp *interp, size_t size, char *msg) {
  char *ptr = TclStackAlloc(interp, size);
  fprintf(stderr, "---- TclStackAlloc %p %s\n", ptr, msg);
  return ptr;
}
#else
# define NsfTclStackFree(interp,ptr,msg) TclStackFree(interp,ptr)
# define NsfTclStackAlloc(interp,size,msg) TclStackAlloc(interp,size)
#endif

/*
 * bytecode support
 */
#ifdef NSF_BYTECODE
typedef struct NsfCompEnv {
  int bytecode;
  Command *cmdPtr;
  CompileProc *compileProc;
  Tcl_ObjCmdProc *callProc;
} NsfCompEnv;

typedef enum {INST_INITPROC, INST_NEXT, INST_SELF, INST_SELF_DISPATCH,
	      LAST_INSTRUCTION} NsfByteCodeInstructions;


extern NsfCompEnv *NsfGetCompEnv();

Tcl_ObjCmdProc NsfInitProcNSCmd, NsfSelfDispatchCmd,
  NsfNextObjCmd, NsfGetSelfObjCmd;

int NsfDirectSelfDispatch(ClientData cd, Tcl_Interp *interp,
		     int objc, Tcl_Obj *CONST objv[]);
#endif

int
NsfObjDispatch(ClientData cd, Tcl_Interp *interp,
		 int objc, Tcl_Obj *CONST objv[]);
extern int
NsfObjWrongArgs(Tcl_Interp *interp, CONST char *msg, Tcl_Obj *cmdName, 
		Tcl_Obj *methodName, char *arglist);

/* functions from nsfUtil.c */
char *Nsf_ltoa(char *buf, long i, int *len);
char *NsfStringIncr(NsfStringIncrStruct *iss);
void NsfStringIncrInit(NsfStringIncrStruct *iss);
void NsfStringIncrFree(NsfStringIncrStruct *iss);


/*
   Tcl uses 01 and 02, TclOO uses 04 and 08, so leave some space free
   for further extensions of tcl and tcloo...
*/
#define FRAME_IS_NSF_OBJECT  0x10000
#define FRAME_IS_NSF_METHOD  0x20000
#define FRAME_IS_NSF_CMETHOD 0x40000

#if !defined(NDEBUG)
/*# define NSF_INLINE*/
#endif

/*** common win sermon ***/
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _nsf_int_h_ */
