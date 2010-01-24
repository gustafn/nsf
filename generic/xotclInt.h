/* -*- Mode: c++ -*-
 *  Extended Object Tcl (XOTcl)
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *  xotclInt.h --
 *
 *  Mostly internally used API Functions
 */

#ifndef _xotcl_int_h_
#define _xotcl_int_h_

#if defined(HAVE_STDINT_H)
# define HAVE_INTPTR_T 
# define HAVE_UINTPTR_T 
#endif

#include <tclInt.h>
#include "xotcl.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(HAVE_TCL_COMPILE_H)
# include <tclCompile.h>
#endif

#if defined(PROFILE)
#  include <sys/time.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef BUILD_xotcl
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
#define XOTCL_METADATA
*/

/*
 * Makros
 */
#if defined(PRE85)
# define TclVarHashTable Tcl_HashTable
#endif

#if defined(PRE86)
# define Tcl_NRCallObjProc(interp, proc, cd, objc, objv) \
  (*(proc))((cd), (interp), (objc), (objv))
#endif

#ifdef XOTCL_MEM_COUNT
Tcl_HashTable xotclMemCount;
extern int xotclMemCountInterpCounter;
typedef struct XOTclMemCounter {
  int peak;
  int count;
} XOTclMemCounter;
#  define MEM_COUNT_ALLOC(id,p) XOTclMemCountAlloc(id,p)
#  define MEM_COUNT_FREE(id,p) XOTclMemCountFree(id,p)
#  define MEM_COUNT_INIT() \
      if (xotclMemCountInterpCounter == 0) { \
        Tcl_InitHashTable(&xotclMemCount, TCL_STRING_KEYS); \
        xotclMemCountInterpCounter = 1; \
      }
#  define MEM_COUNT_DUMP() XOTclMemCountDump(interp)
#  define MEM_COUNT_OPEN_FRAME()
/*if (obj->varTable) noTableBefore = 0*/
#  define MEM_COUNT_CLOSE_FRAME()
/*      if (obj->varTable && noTableBefore) \
	XOTclMemCountAlloc("obj->varTable",NULL)*/
#else
#  define MEM_COUNT_ALLOC(id,p)
#  define MEM_COUNT_FREE(id,p)
#  define MEM_COUNT_INIT()
#  define MEM_COUNT_DUMP()
#  define MEM_COUNT_OPEN_FRAME()
#  define MEM_COUNT_CLOSE_FRAME()
#endif

#define DSTRING_INIT(D) Tcl_DStringInit(D); MEM_COUNT_ALLOC("DString",D)
#define DSTRING_FREE(D) Tcl_DStringFree(D); MEM_COUNT_FREE("DString",D)

#if USE_ASSOC_DATA
# define RUNTIME_STATE(interp) ((XOTclRuntimeState*)Tcl_GetAssocData((interp), "XOTclRuntimeState", NULL))
#else
# define RUNTIME_STATE(interp) ((XOTclRuntimeState*)((Interp*)(interp))->globalNsPtr->clientData)
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
#  define FREE_ON_STACK(var) \
    assert(var[-1] == var[__##var##_count] && (void *)var[-1] == (void*)0xdeadbeaf)
# else
#  define ALLOC_ON_STACK(type,n,var) type var[(n)]
#  define FREE_ON_STACK(var)
# endif
#elif defined(USE_ALLOCA)
#  define ALLOC_ON_STACK(type,n,var) type *var = (type *)alloca((n)*sizeof(type))
#  define FREE_ON_STACK(var)
#else
#  define ALLOC_ON_STACK(type,n,var) type *var = (type *)ckalloc((n)*sizeof(type))
#  define FREE_ON_STACK(var) ckfree((char*)var)
#endif

#ifdef USE_ALLOCA
# include <alloca.h>
#endif

#ifdef  __WIN32__
# define XOTCLINLINE
# define XOTclNewObj(A) A=Tcl_NewObj()
# define DECR_REF_COUNT(A) \
	MEM_COUNT_FREE("INCR_REF_COUNT",A); Tcl_DecrRefCount(A)
#else
/*
 * This was defined to be inline for anything !sun or __IBMC__ >= 0x0306,
 * but __hpux should also be checked - switched to only allow in gcc - JH
 */
# if defined(__GNUC__)
#  define XOTCLINLINE inline
# else
#  define XOTCLINLINE
# endif
# ifdef USE_TCL_STUBS
#  define XOTclNewObj(A) A=Tcl_NewObj()
#  define DECR_REF_COUNT(A) \
	MEM_COUNT_FREE("INCR_REF_COUNT",A); assert((A)->refCount > -1); \
        Tcl_DecrRefCount(A)
# else
#  define XOTclNewObj(A) TclNewObj(A)
#  define DECR_REF_COUNT(A) \
	MEM_COUNT_FREE("INCR_REF_COUNT",A); TclDecrRefCount(A)
# endif
#endif

#if defined(TCL_THREADS)
# define XOTclMutex Tcl_Mutex
# define XOTclMutexLock(a) Tcl_MutexLock(a)
# define XOTclMutexUnlock(a) Tcl_MutexUnlock(a)
#else
# define XOTclMutex int
# define XOTclMutexLock(a)   (*(a))++
# define XOTclMutexUnlock(a) (*(a))--
#endif

#define ObjStr(obj) (obj)->bytes ? (obj)->bytes : Tcl_GetString(obj)

#define INCR_REF_COUNT(A) MEM_COUNT_ALLOC("INCR_REF_COUNT",A); Tcl_IncrRefCount(A)

#ifdef OBJDELETION_TRACE
# define PRINTOBJ(ctx,obj) \
  fprintf(stderr, "  %s %p %s oid=%p teardown=%p destroyCalled=%d\n", \
          ctx,obj,(obj)->teardown?ObjStr((obj)->cmdName):"(deleted)", \
          (obj)->id, (obj)->teardown,                                 \
          ((obj)->flags & XOTCL_DESTROY_CALLED))
#else
# define PRINTOBJ(ctx,obj)
#endif

#define className(cl) (cl ? ObjStr(cl->object.cmdName) : "")
#define objectName(obj) (ObjStr(obj->cmdName))


#define LONG_AS_STRING 32

/* TCL_CONTINUE is defined as 4, from 5 on we can
   use app-specific return codes */
#define XOTCL_CHECK_FAILED 6

/* flags for call method */
#define XOTCL_CM_NO_UNKNOWN 1
#define XOTCL_CM_NO_SHIFT   2
#define XOTCL_CM_NO_PROTECT 4

/*
 *
 * XOTcl Structures
 *
 */

/*
 * Filter structures
 */
typedef struct XOTclFilterStack {
  Tcl_Command currentCmdPtr;
  Tcl_Obj *calledProc;
  struct XOTclFilterStack *nextPtr;
} XOTclFilterStack;

typedef struct XOTclTclObjList {
  Tcl_Obj *content;
  struct XOTclTclObjList *nextPtr;
} XOTclTclObjList;

/*
 * Assertion structures
 */

typedef struct XOTclProcAssertion {
  XOTclTclObjList *pre;
  XOTclTclObjList *post;
} XOTclProcAssertion;

typedef struct XOTclAssertionStore {
  XOTclTclObjList *invariants;
  Tcl_HashTable procs;
} XOTclAssertionStore;

typedef enum { /* powers of 2; add to ALL, if default; */
  CHECK_NONE  = 0, CHECK_CLINVAR = 1, CHECK_OBJINVAR = 2,
  CHECK_PRE   = 4, CHECK_POST = 8,
  CHECK_INVAR = CHECK_CLINVAR + CHECK_OBJINVAR,
  CHECK_ALL   = CHECK_INVAR   + CHECK_PRE + CHECK_POST
} CheckOptions;

void XOTclAssertionRename(Tcl_Interp *interp, Tcl_Command cmd,
			  XOTclAssertionStore *as,
			  char *oldSimpleCmdName, char *newName);
/*
 * mixins
 */
typedef struct XOTclMixinStack {
  Tcl_Command currentCmdPtr;
  struct XOTclMixinStack *nextPtr;
} XOTclMixinStack;

/*
 * Generic command pointer list
 */
typedef struct XOTclCmdList {
  Tcl_Command cmdPtr;
  ClientData clientData;
  struct XOTclClass *clorobj;
  struct XOTclCmdList *nextPtr;
} XOTclCmdList;

typedef void (XOTclFreeCmdListClientData) _ANSI_ARGS_((XOTclCmdList*));

/* for incr string */
typedef struct XOTclStringIncrStruct {
  char *buffer;
  char *start;
  size_t bufSize;
  int length;
} XOTclStringIncrStruct;

/* 
 * cmd flags
 */

#define XOTCL_CMD_PROTECTED_METHOD 		0x00010000
#define XOTCL_CMD_REDEFINE_PROTECTED_METHOD	0x00020000
/* XOTCL_CMD_NONLEAF_METHOD is used to flag, if a Method implemented via cmd calls "next" */
#define XOTCL_CMD_NONLEAF_METHOD		0x00040000

/*
 * object flags ...
 */

/* DESTROY_CALLED indicates that destroy was called on obj */
#define XOTCL_DESTROY_CALLED                 0x0001
/* INIT_CALLED indicates that init was called on obj */
#define XOTCL_INIT_CALLED                    0x0002
/* MIXIN_ORDER_VALID set when mixin order is valid */
#define XOTCL_MIXIN_ORDER_VALID              0x0004
/* MIXIN_ORDER_DEFINED set, when mixins are defined for obj */
#define XOTCL_MIXIN_ORDER_DEFINED            0x0008
#define XOTCL_MIXIN_ORDER_DEFINED_AND_VALID  0x000c
/* FILTER_ORDER_VALID set, when filter order is valid */
#define XOTCL_FILTER_ORDER_VALID             0x0010
/* FILTER_ORDER_DEFINED set, when filters are defined for obj */
#define XOTCL_FILTER_ORDER_DEFINED           0x0020
#define XOTCL_FILTER_ORDER_DEFINED_AND_VALID 0x0030
/* CLASS properties for objects */
#define XOTCL_IS_CLASS                       0x0040
#define XOTCL_IS_ROOT_META_CLASS             0x0080
#define XOTCL_IS_ROOT_CLASS                  0x0100
/* DESTROYED set, when object is physically destroyed with PrimitiveODestroy  */
#define XOTCL_CMD_NOT_FOUND                  0x1000
#define XOTCL_DURING_DELETE                  0x2000
#define XOTCL_DELETED                        0x4000
#define XOTCL_RECREATE                       0x8000

/* flags for XOTclParams */

#define XOTCL_ARG_REQUIRED		     0x0001
#define XOTCL_ARG_MULTIVALUED		     0x0002
#define XOTCL_ARG_NOARG 		     0x0004
#define XOTCL_ARG_SUBST_DEFAULT		     0x0010
#define XOTCL_ARG_INITCMD		     0x0020
#define XOTCL_ARG_METHOD		     0x0040
#define XOTCL_ARG_RELATION		     0x0100
#define XOTCL_ARG_SWITCH		     0x0200

/* disallowed options */
#define XOTCL_DISALLOWED_ARG_METHOD_PARAMETER	     (XOTCL_ARG_METHOD|XOTCL_ARG_INITCMD|XOTCL_ARG_RELATION)
#define XOTCL_DISALLOWED_ARG_OBJECT_PARAMETER	     0
#define XOTCL_DISALLOWED_ARG_VALUEECHECK	     (XOTCL_ARG_SUBST_DEFAULT|XOTCL_ARG_METHOD|XOTCL_ARG_INITCMD|XOTCL_ARG_RELATION|XOTCL_ARG_SWITCH)


/* method types */
#define XOTCL_METHODTYPE_ALIAS     0x0001
#define XOTCL_METHODTYPE_SCRIPTED  0x0002
#define XOTCL_METHODTYPE_SETTER    0x0004
#define XOTCL_METHODTYPE_FORWARDER 0x0008
#define XOTCL_METHODTYPE_OBJECT    0x0010
#define XOTCL_METHODTYPE_OTHER     0x0100
#define XOTCL_METHODTYPE_BUILTIN   XOTCL_METHODTYPE_ALIAS|XOTCL_METHODTYPE_SETTER|XOTCL_METHODTYPE_FORWARDER|XOTCL_METHODTYPE_OBJECT|XOTCL_METHODTYPE_OTHER


/* flags for parseContext */
#define XOTCL_PC_MUST_DECR		     0x0001

#define XOTclObjectSetClass(obj) \
	(obj)->flags |= XOTCL_IS_CLASS
#define XOTclObjectClearClass(obj) \
	(obj)->flags &= ~XOTCL_IS_CLASS
#define XOTclObjectIsClass(obj) \
	((obj)->flags & XOTCL_IS_CLASS)
#define XOTclObjectToClass(obj) \
	(XOTclClass*)((((XOTclObject*)obj)->flags & XOTCL_IS_CLASS)?obj:0)


/*
 * object and class internals
 */
struct XOTclParam;
typedef int (XOTclTypeConverter)(Tcl_Interp *interp, Tcl_Obj *obj,
                                 struct XOTclParam CONST *pPtr, ClientData *clientData);

typedef struct XOTclParam {
  char *name;
  int flags;
  int nrArgs;
  XOTclTypeConverter *converter;
  Tcl_Obj *converterArg;
  Tcl_Obj *defaultValue;
  char *type;
  Tcl_Obj *nameObj;
  Tcl_Obj *arg;
} XOTclParam;

typedef struct XOTclParamDefs {
  XOTclParam *paramsPtr;
  int nrParams;
  Tcl_Obj *slotObj;
} XOTclParamDefs;

typedef struct XOTclParsedParam {
  XOTclParamDefs *paramDefs;
  int possibleUnknowns;
} XOTclParsedParam;

typedef struct XOTclObjectOpt {
  XOTclAssertionStore *assertions;
  XOTclCmdList *filters;
  XOTclCmdList *mixins;
#ifdef XOTCL_METADATA
  Tcl_HashTable metaData;
#endif
  ClientData clientData;
  CONST char *volatileVarName;
  short checkoptions;
} XOTclObjectOpt;

typedef struct XOTclObject {
  Tcl_Obj *cmdName;
  Tcl_Command id;
  Tcl_Interp *teardown;
  struct XOTclClass *cl;
  TclVarHashTable *varTable;
  Tcl_Namespace *nsPtr;
  XOTclObjectOpt *opt;
  struct XOTclCmdList *filterOrder;
  struct XOTclCmdList *mixinOrder;
  XOTclFilterStack *filterStack;
  XOTclMixinStack *mixinStack;
  int refCount;
  short flags;
  short activationCount;
} XOTclObject;

typedef struct XOTclObjects {
  struct XOTclObject *obj;
  struct XOTclObjects *nextPtr;
} XOTclObjects;

typedef struct XOTclClassOpt {
  XOTclCmdList *classfilters;
  XOTclCmdList *classmixins;
  XOTclCmdList *isObjectMixinOf;
  XOTclCmdList *isClassMixinOf;
  XOTclAssertionStore *assertions;
#ifdef XOTCL_OBJECTDATA
  Tcl_HashTable *objectdata;
#endif
  Tcl_Command id;
  ClientData clientData;
} XOTclClassOpt;

typedef struct XOTclClass {
  struct XOTclObject object;
  struct XOTclClasses *super;
  struct XOTclClasses *sub;
  short color;
  struct XOTclClasses *order;
  Tcl_HashTable instances;
  Tcl_Namespace *nsPtr;
  XOTclParsedParam *parsedParamPtr;
  XOTclClassOpt *opt;
} XOTclClass;

typedef struct XOTclClasses {
  struct XOTclClass *cl;
  ClientData clientData;
  struct XOTclClasses *nextPtr;
} XOTclClasses;

/* XOTcl global names and strings */
/* these are names and contents for global (corresponding) Tcl_Objs
   and Strings - otherwise these "constants" would have to be built
   every time they are used; now they are built once in XOTcl_Init */
typedef enum {
  XOTE_EMPTY, XOTE_ONE,
  /* methods called internally */
  XOTE_ALLOC, XOTE_CLEANUP, XOTE_CONFIGURE, XOTE_CREATE, 
  XOTE_DEFAULTMETHOD, XOTE_DESTROY, XOTE_DEALLOC,
  XOTE_INIT, XOTE_MOVE, XOTE_OBJECTPARAMETER,
  XOTE_RECREATE, XOTE_RESIDUALARGS,
  XOTE_UNKNOWN, XOTE___UNKNOWN,
  /* var names */
  XOTE_AUTONAMES, XOTE_DEFAULTMETACLASS, XOTE_DEFAULTSUPERCLASS, 
  XOTE_ALIAS_ARRAY,
  /* object/class names */
  XOTE_PARAMETER_TYPE_OBJ, 
  /* constants */
  XOTE_ALIAS, XOTE_ARGS, XOTE_CMD, XOTE_FILTER, XOTE_FORWARD, 
  XOTE_METHOD, XOTE_OBJECT, XOTE_SETTER, 
  XOTE_GUARD_OPTION, XOTE___UNKNOWN__, 
  /* Patly redefined Tcl commands; leave them together at the end */
  XOTE_EXPR, XOTE_FORMAT, XOTE_INFO, XOTE_INTERP, XOTE_RENAME, XOTE_SUBST
} XOTclGlobalNames;
#if !defined(XOTCL_C)
extern char *XOTclGlobalStrings[];
#else
char *XOTclGlobalStrings[] = {
  "", "1", 
  /* methods called internally */
  "alloc", "cleanup", "configure", "create", 
  "defaultmethod", "destroy", "dealloc",
  "init", "move", "objectparameter", 
  "recreate", "residualargs",
  "unknown", "__unknown", 
  /* var names */
  "__autonames", "__default_metaclass", "__default_superclass", 
  "::xotcl::alias",
  /* object/class names */
  "::xotcl::parameterType", 
  /* constants */
  "alias", "args", "cmd", "filter",  "forward", 
  "method", "object", "setter", 
  "-guard", "__unknown__",
  /* tcl commands */
  "expr", "format", "info", "interp", "rename", "subst",
};
#endif

#define XOTclGlobalObjects RUNTIME_STATE(interp)->methodObjNames

/* XOTcl ShadowTclCommands */
typedef struct XOTclShadowTclCommandInfo {
  TclObjCmdProcType proc;
  ClientData clientData;
} XOTclShadowTclCommandInfo;
typedef enum {SHADOW_LOAD=1, SHADOW_UNLOAD=0, SHADOW_REFETCH=2} XOTclShadowOperations;

int XOTclCallCommand(Tcl_Interp *interp, XOTclGlobalNames name,
		     int objc, Tcl_Obj *CONST objv[]);
int XOTclShadowTclCommands(Tcl_Interp *interp, XOTclShadowOperations load);


/*
 * XOTcl CallStack
 */
typedef struct XOTclCallStackContent {
  XOTclObject *self;
  XOTclClass *cl;
  Tcl_Command cmdPtr;
#if !defined(TCL85STACK)
  Tcl_CallFrame *currentFramePtr;
#endif
  XOTclFilterStack *filterStackEntry;
  Tcl_Obj ** objv;
  int objc;
  unsigned short frameType;
  unsigned short callType;
} XOTclCallStackContent;

#define XOTCL_CSC_TYPE_PLAIN 0
#define XOTCL_CSC_TYPE_ACTIVE_MIXIN 1
#define XOTCL_CSC_TYPE_ACTIVE_FILTER 2
#define XOTCL_CSC_TYPE_INACTIVE 4
#define XOTCL_CSC_TYPE_INACTIVE_MIXIN 5
#define XOTCL_CSC_TYPE_INACTIVE_FILTER 6
#define XOTCL_CSC_TYPE_GUARD 16

#define XOTCL_CSC_CALL_IS_NEXT 1
#define XOTCL_CSC_CALL_IS_GUARD 2

#if !defined(TCL85STACK)
typedef struct XOTclCallStack {
  XOTclCallStackContent content[MAX_NESTING_DEPTH];
  XOTclCallStackContent *top;
} XOTclCallStack;
#endif

#if defined(PROFILE)
typedef struct XOTclProfile {
  long int overallTime;
  Tcl_HashTable objectData;
  Tcl_HashTable methodData;
} XOTclProfile;
#endif

typedef struct XOTclRuntimeState {
#if !defined(TCL85STACK)
  XOTclCallStack cs;
#endif
  Tcl_Namespace *XOTclClassesNS;
  Tcl_Namespace *XOTclNS;
  /*
   * definitions of the main xotcl objects
   */
  struct XOTclClasses *rootClasses;
  Tcl_ObjCmdProc *objInterpProc;
  Tcl_Obj **methodObjNames;
  struct XOTclShadowTclCommandInfo *tclCommands;
  int errorCount;
  /* these flags could move into a bitarray, but are used only once per interp*/
  int unknown;
  int doFilters;
  int doSoftrecreate;
  int cacheInterface;
  int exitHandlerDestroyRound;
  int returnCode;
  long newCounter;
  XOTclStringIncrStruct iss;
  Proc fakeProc;
  Tcl_Namespace *fakeNS;
  XotclStubs *xotclStubs;
  Tcl_CallFrame *varFramePtr;
  Tcl_Command cmdPtr; /* used for ACTIVE_MIXIN */
  Tcl_Command dotCmd;
  Tcl_Command dotDotCmd;
#if defined(PROFILE)
  XOTclProfile profile;
#endif
  short guardCount;
  ClientData clientData;
} XOTclRuntimeState;

#define XOTCL_EXITHANDLER_OFF 0
#define XOTCL_EXITHANDLER_ON_SOFT_DESTROY 1
#define XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY 2


#ifdef XOTCL_OBJECTDATA
extern void
XOTclSetObjectData(struct XOTclObject *obj, struct XOTclClass *cl,
		  ClientData data);
extern int
XOTclGetObjectData(struct XOTclObject *obj, struct XOTclClass *cl,
		  ClientData *data);
extern int
XOTclUnsetObjectData(struct XOTclObject *obj, struct XOTclClass *cl);
extern void
XOTclFreeObjectData(XOTclClass *cl);
#endif

/*
 *
 *  internally used API functions
 *
 */

#include "xotclIntDecls.h"

/*
 * Profiling functions
 */

#if defined(PROFILE)
extern void
XOTclProfileFillTable(Tcl_HashTable *table, Tcl_DString *key,
		 double totalMicroSec);
extern void
XOTclProfileEvaluateData(Tcl_Interp *interp, long int startSec, long int startUsec,
		    XOTclObject *obj, XOTclClass *cl, char *methodName);
extern void
XOTclProfilePrintTable(Tcl_HashTable *table);

extern void
XOTclProfilePrintData(Tcl_Interp *interp);

extern void
XOTclProfileInit(Tcl_Interp *interp);
#endif

/*
 * MEM Counting
 */
#ifdef XOTCL_MEM_COUNT
void XOTclMemCountAlloc(char *id, void *);
void XOTclMemCountFree(char *id, void *);
void XOTclMemCountDump();
#endif /* XOTCL_MEM_COUNT */
/*
 * old, deprecated meta-data command
 */
#if defined(XOTCL_METADATA)
extern void
XOTclMetaDataDestroy(XOTclObject *obj);
extern void
XOTclMetaDataInit(XOTclObject *obj);
extern int
XOTclOMetaDataMethod (ClientData cd, Tcl_Interp *interp,
		      int objc, Tcl_Obj *objv[]);
#endif /* XOTCL_METADATA */


/*
 * bytecode support
 */
#ifdef XOTCL_BYTECODE
typedef struct XOTclCompEnv {
  int bytecode;
  Command *cmdPtr;
  CompileProc *compileProc;
  Tcl_ObjCmdProc *callProc;
} XOTclCompEnv;

typedef enum {INST_INITPROC, INST_NEXT, INST_SELF, INST_SELF_DISPATCH,
	      LAST_INSTRUCTION} XOTclByteCodeInstructions;


extern XOTclCompEnv *XOTclGetCompEnv();

Tcl_ObjCmdProc XOTclInitProcNSCmd, XOTclSelfDispatchCmd,
  XOTclNextObjCmd, XOTclGetSelfObjCmd;

int XOTclDirectSelfDispatch(ClientData cd, Tcl_Interp *interp,
		     int objc, Tcl_Obj *CONST objv[]);
#endif

int
XOTclObjDispatch(ClientData cd, Tcl_Interp *interp,
		 int objc, Tcl_Obj *CONST objv[]);

/* functions from xotclUtil.c */
char *XOTcl_ltoa(char *buf, long i, int *len);
char *XOTclStringIncr(XOTclStringIncrStruct *iss);
void XOTclStringIncrInit(XOTclStringIncrStruct *iss);
void XOTclStringIncrFree(XOTclStringIncrStruct *iss);


#if defined(TCL85STACK)
/*
   Tcl uses 01 and 02, TclOO uses 04 and 08, so leave some space free
   for further extensions of tcl and tcloo...
*/
# define FRAME_IS_XOTCL_OBJECT  0x10000
# define FRAME_IS_XOTCL_METHOD  0x20000
# define FRAME_IS_XOTCL_CMETHOD 0x40000
#else
# define FRAME_IS_XOTCL_OBJECT  0x0
# define FRAME_IS_XOTCL_METHOD  0x0
# define FRAME_IS_XOTCL_CMETHOD 0x0
#endif

#if !defined(NDEBUG)
/*# define XOTCLINLINE*/
#endif

/*** common win sermon ***/
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _xotcl_int_h_ */
