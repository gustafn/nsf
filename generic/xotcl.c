/* $Id: xotcl.c,v 1.43 2006/10/04 20:40:23 neumann Exp $
 *
 *  XOTcl - Extended Object Tcl
 *
 *  Copyright (C) 1999-2009 Gustaf Neumann (a), Uwe Zdun (a)
 *
 * (a) Vienna University of Economics and Business Administration
 *     Institute. of Information Systems and New Media
 *     A-1090, Augasse 2-6
 *     Vienna, Austria
 *
 * (b) University of Essen
 *     Specification of Software Systems
 *     Altendorferstrasse 97-101
 *     D-45143 Essen, Germany
 *
 *  Permission to use, copy, modify, distribute, and sell this
 *  software and its documentation for any purpose is hereby granted
 *  without fee, provided that the above copyright notice appear in
 *  all copies and that both that copyright notice and this permission
 *  notice appear in supporting documentation. We make no
 *  representations about the suitability of this software for any
 *  purpose.  It is provided "as is" without express or implied
 *  warranty.
 *
 *
 *  This software is based upon MIT Object Tcl by David Wetherall and
 *  Christopher J. Lindblad, that contains the following copyright
 *  message:
 *
 *   "Copyright 1993 Massachusetts Institute of Technology
 *
 *    Permission to use, copy, modify, distribute, and sell this
 *    software and its documentation for any purpose is hereby granted
 *    without fee, provided that the above copyright notice appear in
 *    all copies and that both that copyright notice and this
 *    permission notice appear in supporting documentation, and that
 *    the name of M.I.T. not be used in advertising or publicity
 *    pertaining to distribution of the software without specific,
 *    written prior permission.  M.I.T. makes no representations about
 *    the suitability of this software for any purpose.  It is
 *    provided "as is" without express or implied warranty."
 * */
#define OO 1

#define XOTCL_C 1
#include "xotclInt.h"
#include "xotclAccessInt.h"

#ifdef COMPILE_XOTCL_STUBS
extern XotclStubs xotclStubs;
#endif

#ifdef XOTCL_MEM_COUNT
int xotclMemCountInterpCounter = 0;
#endif


/*
 * Tcl_Obj Types for XOTcl Objects
 */

#ifdef USE_TCL_STUBS
# define XOTcl_ExprObjCmd(clientData, interp, objc, objv)	\
  XOTclCallCommand(interp, XOTE_EXPR, objc, objv)
# define XOTcl_SubstObjCmd(clientData, interp, objc, objv)	\
  XOTclCallCommand(interp, XOTE_SUBST, objc, objv)
#else
# define XOTcl_ExprObjCmd(clientData, interp, objc, objv)	\
  Tcl_ExprObjCmd(clientData, interp, objc, objv)
# define XOTcl_SubstObjCmd(clientData, interp, objc, objv)	\
  Tcl_SubstObjCmd(clientData, interp, objc, objv)
#endif

/* maybe move to stubs? */
int XOTclObjErrArgCntObj(Tcl_Interp *interp, Tcl_Obj *cmdName,  Tcl_Obj *methodName, Tcl_Obj *msg);

static int createMethod(Tcl_Interp *interp, XOTclClass *cl, char *name, int objc, Tcl_Obj *CONST objv[]);

static Tcl_Obj *NameInNamespaceObj(Tcl_Interp *interp, char *name, Tcl_Namespace *ns);
static Tcl_Namespace *callingNameSpace(Tcl_Interp *interp);
XOTCLINLINE static Tcl_Command NSFindCommand(Tcl_Interp *interp, char *name, Tcl_Namespace *ns);
#ifdef EXPERIMENTAL_CMD_RESOLVER
static int NSisXOTclNamespace(Tcl_Namespace *nsPtr);
#endif

XOTCLINLINE static void GuardAdd(Tcl_Interp *interp, XOTclCmdList *filterCL, Tcl_Obj *guard);
static int GuardCheck(Tcl_Interp *interp, Tcl_Obj *guards);
static int GuardCall(XOTclObject *obj, XOTclClass *cl, Tcl_Command cmd, Tcl_Interp *interp, 
                     Tcl_Obj *guard, XOTclCallStackContent *csc);
static void GuardDel(XOTclCmdList *filterCL);
static int IsMetaClass(Tcl_Interp *interp, XOTclClass *cl, int withMixins);
static int hasMixin(Tcl_Interp *interp, XOTclObject *obj, XOTclClass *cl);
static int isSubType(XOTclClass *subcl, XOTclClass *cl);
static int setInstVar(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *name, Tcl_Obj *value);
static void MixinComputeDefined(Tcl_Interp *interp, XOTclObject *obj);
static XOTclClass *DefaultSuperClass(Tcl_Interp *interp, XOTclClass *cl, XOTclClass *mcl, int isMeta);
static XOTclCallStackContent *CallStackGetFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr);
XOTCLINLINE static void CallStackPop(Tcl_Interp *interp, XOTclCallStackContent *cscPtr);
XOTCLINLINE static void CallStackDoDestroy(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclCInvalidateInterfaceDefinitionMethod(Tcl_Interp *interp, XOTclClass *cl);

typedef enum { CALLING_LEVEL, ACTIVE_LEVEL } CallStackLevel;

typedef struct callFrameContext {
  int framesSaved;
  Tcl_CallFrame *framePtr;
  Tcl_CallFrame *varFramePtr;
} callFrameContext;

typedef struct XOTclProcContext {
  ClientData oldDeleteData;
  Tcl_CmdDeleteProc *oldDeleteProc;
  XOTclNonposArgs *nonposArgs;
} XOTclProcContext;

typedef struct tclCmdClientData {
  XOTclObject *obj;
  Tcl_Obj *cmdName;
} tclCmdClientData;

typedef struct forwardCmdClientData {
  XOTclObject *obj;
  Tcl_Obj *cmdName;
  Tcl_ObjCmdProc *objProc;
  int passthrough;
  int needobjmap;
  int verbose;
  ClientData clientData;
  int nr_args;
  Tcl_Obj *args;
  int objscope;
  Tcl_Obj *onerror;
  Tcl_Obj *prefix;
  int nr_subcommands;
  Tcl_Obj *subcommands;
} forwardCmdClientData;

typedef struct aliasCmdClientData {
  XOTclObject *obj;
  Tcl_Obj *cmdName;
  Tcl_ObjCmdProc *objProc;
  ClientData clientData;
} aliasCmdClientData;

#define PARSE_CONTEXT_PREALLOC 40
typedef struct {
  ClientData *clientData;
  Tcl_Obj **objv;
  Tcl_Obj **full_objv;
  ClientData clientData_static[PARSE_CONTEXT_PREALLOC];
  Tcl_Obj *objv_static[PARSE_CONTEXT_PREALLOC+1];
  int lastobjc;
  int objc;
} parseContext;

#if defined(CANONICAL_ARGS)
int canonicalNonpositionalArgs(parseContext *pcPtr, Tcl_Interp *interp, XOTclNonposArgs *nonposArgs,
                               char *methodName, int objc,  Tcl_Obj *CONST objv[]);
#endif
void parseContextInit(parseContext *pc, int objc, Tcl_Obj *procName) {
  if (objc < PARSE_CONTEXT_PREALLOC) {
    /* the single larger memset below .... */
    memset(pc, 0, sizeof(parseContext));
    /* ... is faster than the two smaller memsets below */
    /* memset(pc->clientData_static, 0, sizeof(ClientData)*(objc));
       memset(pc->objv_static, 0, sizeof(Tcl_Obj*)*(objc+1));*/
    pc->full_objv = &pc->objv_static[0];
    pc->clientData = &pc->clientData_static[0];
  } else {
    pc->full_objv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*)*(objc+1));
    pc->clientData = (ClientData*)ckalloc(sizeof(ClientData)*objc);
    /*fprintf(stderr,"ParseContextMalloc %d objc, %p %p\n",objc,pc->full_objv,pc->clientData);*/
    memset(pc->full_objv, 0, sizeof(Tcl_Obj*)*(objc+1));
    memset(pc->clientData, 0, sizeof(ClientData)*(objc));
  }
  pc->objv = &pc->full_objv[1];
  pc->full_objv[0] = procName;
}

void parseContextRelease(parseContext *pc) {
  if (pc->objv != &pc->objv_static[1]) {
    /*fprintf(stderr,"release free %p %p\n",pc->full_objv,pc->clientData);*/
    ckfree((char *)pc->full_objv);
    ckfree((char *)pc->clientData);
  }
}

XOTCLINLINE static int DoDispatch(ClientData clientData, Tcl_Interp *interp, int objc,
                                  Tcl_Obj *CONST objv[], int flags);
static int XOTclNextMethod(XOTclObject *obj, Tcl_Interp *interp, XOTclClass *givenCl,
                           char *givenMethod, int objc, Tcl_Obj *CONST objv[],
                           int useCSObjs, XOTclCallStackContent *csc);

static int XOTclForwardMethod(ClientData clientData, Tcl_Interp *interp, int objc,
                              Tcl_Obj *CONST objv[]);
static int XOTclObjscopedMethod(ClientData clientData, Tcl_Interp *interp, int objc,
                                Tcl_Obj *CONST objv[]);
static int XOTclSetterMethod(ClientData clientData, Tcl_Interp *interp, int objc,
                             Tcl_Obj *CONST objv[]);

static int callDestroyMethod(Tcl_Interp *interp, XOTclObject *obj, int flags);
static int GetObjectFromObj(Tcl_Interp *interp, register Tcl_Obj *objPtr, XOTclObject **obj);
static XOTclObject *XOTclpGetObject(Tcl_Interp *interp, char *name);
static XOTclClass *XOTclpGetClass(Tcl_Interp *interp, char *name);
#if !defined(NDEBUG)
static void checkAllInstances(Tcl_Interp *interp, XOTclClass *startCl, int lvl);
#endif

#if defined(PRE85)
# define XOTcl_FindHashEntry(tablePtr, key) Tcl_FindHashEntry(tablePtr, key)
#else
# define XOTcl_FindHashEntry(tablePtr, key) Tcl_CreateHashEntry(tablePtr, key, NULL)
#endif


/*
 * Var Reform Compatibility support
 */

#if !defined(TclOffset)
#ifdef offsetof
#define TclOffset(type, field) ((int) offsetof(type, field))
#else
#define TclOffset(type, field) ((int) ((char *) &((type *) 0)->field))
#endif
#endif

#if defined(PRE85) && FORWARD_COMPATIBLE
/*
 * Define the types missing for the forward compatible mode
 */
typedef Var * (Tcl_VarHashCreateVarFunction) _ANSI_ARGS_(
                                                         (TclVarHashTable *tablePtr, Tcl_Obj *key, int *newPtr)
                                                         );
typedef void (Tcl_InitVarHashTableFunction) _ANSI_ARGS_(
                                                        (TclVarHashTable *tablePtr, Namespace *nsPtr)
                                                        );
typedef void (Tcl_CleanupVarFunction) _ANSI_ARGS_ (
                                                   (Var * varPtr, Var *arrayPtr)
                                                   );
typedef Var * (Tcl_DeleteVarFunction) _ANSI_ARGS_ (
                                                   (Interp *iPtr, TclVarHashTable *tablePtr)
                                                   );
typedef Var * (lookupVarFromTableFunction) _ANSI_ARGS_ (
                                                        (TclVarHashTable *varTable, CONST char *simpleName, XOTclObject *obj)
                                                        );


typedef struct TclVarHashTable85 {
  Tcl_HashTable table;
  struct Namespace *nsPtr;
} TclVarHashTable85;

typedef struct Var85 {
  int flags;
  union {
    Tcl_Obj *objPtr;
    TclVarHashTable85 *tablePtr;
    struct Var85 *linkPtr;
  } value;
} Var85;

typedef struct VarInHash {
  Var85 var;
  int refCount;
  Tcl_HashEntry entry;
} VarInHash;


typedef struct Tcl_CallFrame85 {
  Tcl_Namespace *nsPtr;
  int dummy1;
  int dummy2;
  char *dummy3;
  char *dummy4;
  char *dummy5;
  int dummy6;
  char *dummy7;
  char *dummy8;
  int dummy9;
  char *dummy10;
  char *dummy11;
  char *dummy12;
} Tcl_CallFrame85;

typedef struct CallFrame85 {
  Namespace *nsPtr;
  int isProcCallFrame;
  int objc;
  Tcl_Obj *CONST *objv;
  struct CallFrame *callerPtr;
  struct CallFrame *callerVarPtr;
  int level;
  Proc *procPtr;
  TclVarHashTable *varTablePtr;
  int numCompiledLocals;
  Var85 *compiledLocals;
  ClientData clientData;
  void *localCachePtr;
} CallFrame85;

/*
 * These are global variables, but thread-safe, since they
 * are only set during initialzation and they are never changed,
 * and the variables are single words.
 */
static int forwardCompatibleMode;

static Tcl_VarHashCreateVarFunction  *tclVarHashCreateVar;
static Tcl_InitVarHashTableFunction  *tclInitVarHashTable;
static Tcl_CleanupVarFunction        *tclCleanupVar;
static lookupVarFromTableFunction    *lookupVarFromTable;

static int varRefCountOffset;
static int varHashTableSize;

# define VarHashRefCount(varPtr)			\
  (*((int *) (((char *)(varPtr))+varRefCountOffset)))

# define VarHashGetValue(hPtr)					\
  (forwardCompatibleMode ?					\
   (Var *) ((char *)hPtr - TclOffset(VarInHash, entry)) :	\
   (Var *) Tcl_GetHashValue(hPtr)				\
   )

#define VarHashGetKey(varPtr)                   \
  (((VarInHash *)(varPtr))->entry.key.objPtr)

#define VAR_TRACED_READ85        0x10       /* TCL_TRACE_READS  */
#define VAR_TRACED_WRITE85       0x20       /* TCL_TRACE_WRITES */
#define VAR_TRACED_UNSET85       0x40       /* TCL_TRACE_UNSETS */
#define VAR_TRACED_ARRAY85       0x800      /* TCL_TRACE_ARRAY  */
#define VAR_TRACE_ACTIVE85       0x2000
#define VAR_SEARCH_ACTIVE85      0x4000
#define VAR_ALL_TRACES85                                                \
  (VAR_TRACED_READ85|VAR_TRACED_WRITE85|VAR_TRACED_ARRAY85|VAR_TRACED_UNSET85)

#define VAR_ARRAY85            0x1
#define VAR_LINK85             0x2

#define varFlags(varPtr)                        \
  (forwardCompatibleMode ?                      \
   ((Var85 *)varPtr)->flags :                   \
   (varPtr)->flags                              \
   )
#undef TclIsVarScalar
#define TclIsVarScalar(varPtr)					\
  (forwardCompatibleMode ?					\
   !(((Var85 *)varPtr)->flags & (VAR_ARRAY85|VAR_LINK85)) :	\
   ((varPtr)->flags & VAR_SCALAR)				\
   )
#undef TclIsVarArray
#define TclIsVarArray(varPtr)                   \
  (forwardCompatibleMode ?                      \
   (((Var85 *)varPtr)->flags & VAR_ARRAY85) :   \
   ((varPtr)->flags & VAR_ARRAY)                \
   )
#define TclIsVarNamespaceVar(varPtr)			\
  (forwardCompatibleMode ?				\
   (((Var85 *)varPtr)->flags & VAR_NAMESPACE_VAR) :	\
   ((varPtr)->flags & VAR_NAMESPACE_VAR)		\
   )

#define TclIsVarTraced(varPtr)				\
  (forwardCompatibleMode ?				\
   (((Var85 *)varPtr)->flags & VAR_ALL_TRACES85) :	\
   (varPtr->tracePtr != NULL)				\
   )
#undef TclIsVarLink
#define TclIsVarLink(varPtr)                    \
  (forwardCompatibleMode ?                      \
   (((Var85 *)varPtr)->flags & VAR_LINK85) :    \
   (varPtr->flags & VAR_LINK)                   \
   )
#undef TclIsVarUndefined
#define TclIsVarUndefined(varPtr)               \
  (forwardCompatibleMode ?                      \
   (((Var85 *)varPtr)->value.objPtr == NULL) :  \
   (varPtr->flags & VAR_UNDEFINED)              \
   )
#undef TclSetVarLink
#define TclSetVarLink(varPtr)                                           \
  if (forwardCompatibleMode)                                            \
    ((Var85 *)varPtr)->flags = (((Var85 *)varPtr)->flags & ~VAR_ARRAY85) | VAR_LINK85; \
  else                                                                  \
    (varPtr)->flags = ((varPtr)->flags & ~(VAR_SCALAR|VAR_ARRAY)) | VAR_LINK

#undef TclClearVarUndefined
#define TclClearVarUndefined(varPtr)            \
  if (!forwardCompatibleMode)                   \
    (varPtr)->flags &= ~VAR_UNDEFINED

#undef Tcl_CallFrame_compiledLocals
#define Tcl_CallFrame_compiledLocals(cf)		\
  (forwardCompatibleMode ?				\
   (Var *)(((CallFrame85 *)cf)->compiledLocals) :	\
   (((CallFrame*)cf)->compiledLocals)			\
   )

#define getNthVar(varPtr, i)                    \
  (forwardCompatibleMode ?                      \
   (Var *)(((Var85 *)varPtr)+(i)) :             \
   (((Var *)varPtr)+(i))                        \
   )

#define valueOfVar(type, varPtr, field)		\
  (forwardCompatibleMode ?                      \
   (type *)(((Var85 *)varPtr)->value.field) :   \
   (type *)(((Var *)varPtr)->value.field)       \
   )
#endif


#if !FORWARD_COMPATIBLE
# define getNthVar(varPtr, i) (((Var *)varPtr)+(i))
#endif


#define TclIsCompiledLocalArgument(compiledLocalPtr)	\
  ((compiledLocalPtr)->flags & VAR_ARGUMENT)
#define TclIsCompiledLocalTemporary(compiledLocalPtr)   \
  ((compiledLocalPtr)->flags & VAR_TEMPORARY)

#if defined(PRE85) && !FORWARD_COMPATIBLE
# define VarHashGetValue(hPtr) (Var *)Tcl_GetHashValue(hPtr)
# define VarHashRefCount(varPtr) (varPtr)->refCount
# define TclIsVarTraced(varPtr)  (varPtr->tracePtr != NULL)
# define TclIsVarNamespaceVar(varPtr) ((varPtr)->flags & VAR_NAMESPACE_VAR)
# define varHashTableSize sizeof(TclVarHashTable)
# define valueOfVar(type, varPtr, field) (type *)(varPtr)->value.field
#endif

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

#if defined(PRE85)
/*
 * We need NewVar from tclVar.c ... but its not exported
 */
static Var *NewVar84() {
  register Var *varPtr;

  varPtr = (Var *) ckalloc(sizeof(Var));
  varPtr->value.objPtr = NULL;
  varPtr->name = NULL;
  varPtr->nsPtr = NULL;
  varPtr->hPtr = NULL;
  varPtr->refCount = 0;
  varPtr->tracePtr = NULL;
  varPtr->searchPtr = NULL;
  varPtr->flags = (VAR_SCALAR | VAR_UNDEFINED | VAR_IN_HASHTABLE);
  return varPtr;
}

static Var *
VarHashCreateVar84(TclVarHashTable *tablePtr, Tcl_Obj *key, int *newPtr) {
  char *newName = ObjStr(key);
  Tcl_HashEntry *hPtr = Tcl_CreateHashEntry(tablePtr, newName, newPtr);
  Var *varPtr;

  if (newPtr && *newPtr) {
    varPtr = NewVar84();
    Tcl_SetHashValue(hPtr, varPtr);
    varPtr->hPtr = hPtr;
    varPtr->nsPtr = NULL; /* a local variable */
  } else {
    varPtr = (Var *) Tcl_GetHashValue(hPtr);
  }

  return varPtr;
}

static void
InitVarHashTable84(TclVarHashTable *tablePtr, Namespace *nsPtr) {
  /* fprintf(stderr,"InitVarHashTable84\n"); */
  Tcl_InitHashTable((tablePtr), TCL_STRING_KEYS);
}

static void
TclCleanupVar84(Var * varPtr, Var *arrayPtr) {
  if (TclIsVarUndefined(varPtr) && (varPtr->refCount == 0)
      && (varPtr->tracePtr == NULL)
      && (varPtr->flags & VAR_IN_HASHTABLE)) {
    if (varPtr->hPtr) {
      Tcl_DeleteHashEntry(varPtr->hPtr);
    }
    ckfree((char *) varPtr);
  }
  if (arrayPtr) {
    if (TclIsVarUndefined(arrayPtr) && (arrayPtr->refCount == 0)
	&& (arrayPtr->tracePtr == NULL)
	&& (arrayPtr->flags & VAR_IN_HASHTABLE)) {
      if (arrayPtr->hPtr) {
	Tcl_DeleteHashEntry(arrayPtr->hPtr);
      }
      ckfree((char *) arrayPtr);
    }
  }
}
static Var *
LookupVarFromTable84(TclVarHashTable *varTable, CONST char *simpleName,
                     XOTclObject *obj) {
  Var *varPtr = NULL;
  Tcl_HashEntry *entryPtr;

  if (varTable) {
    entryPtr = XOTcl_FindHashEntry(varTable, simpleName);
    if (entryPtr) {
      varPtr = VarHashGetValue(entryPtr);
    }
  }
  return varPtr;
}
#endif


#if defined(PRE85)
# if FORWARD_COMPATIBLE
#  define VarHashCreateVar   (*tclVarHashCreateVar)
#  define InitVarHashTable   (*tclInitVarHashTable)
#  define CleanupVar         (*tclCleanupVar)
#  define LookupVarFromTable (*lookupVarFromTable)
#  define TclCallFrame       Tcl_CallFrame85
# else
#  define VarHashCreateVar   VarHashCreateVar84
#  define InitVarHashTable   InitVarHashTable84
#  define CleanupVar         TclCleanupVar84
#  define LookupVarFromTable LookupVarFromTable84
#  define TclCallFrame       Tcl_CallFrame
# endif
#else
# define VarHashCreateVar   VarHashCreateVar85
# define InitVarHashTable   TclInitVarHashTable
# define CleanupVar         TclCleanupVar
# define LookupVarFromTable LookupVarFromTable85
# define TclCallFrame       Tcl_CallFrame
#endif


#if defined(PRE85)
/*
 * for backward compatibility
 */

#define VarHashTable(t) t
#define TclVarHashTable Tcl_HashTable

static Var *
XOTclObjLookupVar(Tcl_Interp *interp, Tcl_Obj *part1Ptr, CONST char *part2,
                  int flags, const char *msg, int createPart1, int createPart2,
                  Var **arrayPtrPtr) {

  return TclLookupVar(interp, ObjStr(part1Ptr), part2, flags, msg,
                      createPart1, createPart2, arrayPtrPtr);
}

#define ObjFindNamespace(interp, objPtr)                \
  Tcl_FindNamespace((interp), ObjStr(objPtr), NULL, 0);

#else

/*
 *  definitions for tcl 8.5
 */

#define VarHashGetValue(hPtr)					\
  ((Var *) ((char *)hPtr - TclOffset(VarInHash, entry)))
#define VarHashGetKey(varPtr)                   \
  (((VarInHash *)(varPtr))->entry.key.objPtr)
#define VarHashTable(varTable)                  \
  &(varTable)->table
#define XOTclObjLookupVar TclObjLookupVar
#define varHashTableSize sizeof(TclVarHashTable)
#define valueOfVar(type, varPtr, field) (type *)(varPtr)->value.field

XOTCLINLINE static Tcl_Namespace *
ObjFindNamespace(Tcl_Interp *interp, Tcl_Obj *objPtr) {
  Tcl_Namespace *nsPtr;

  if (TclGetNamespaceFromObj(interp, objPtr, &nsPtr) == TCL_OK) {
    return nsPtr;
  } else {
    return NULL;
  }
}
#endif

#if !defined(PRE85) || FORWARD_COMPATIBLE
static XOTCLINLINE Var *
VarHashCreateVar85(TclVarHashTable *tablePtr, Tcl_Obj *key, int *newPtr)
{
  Var *varPtr = NULL;
  Tcl_HashEntry *hPtr;
  hPtr = Tcl_CreateHashEntry((Tcl_HashTable *) tablePtr,
                             (char *) key, newPtr);
  if (hPtr) {
    varPtr = VarHashGetValue(hPtr);
  }
  return varPtr;
}

static XOTCLINLINE Var *
LookupVarFromTable85(TclVarHashTable *tablePtr, CONST char *simpleName,
                     XOTclObject *obj) {
  Var *varPtr = NULL;
  if (tablePtr) {
    Tcl_Obj *keyPtr = Tcl_NewStringObj(simpleName, -1);
    Tcl_IncrRefCount(keyPtr);
    varPtr = VarHashCreateVar85(tablePtr, keyPtr, NULL);
    Tcl_DecrRefCount(keyPtr);
  }
  return varPtr;
}
#endif


/*
 * call an XOTcl method
 */
static int
callMethod(ClientData clientData, Tcl_Interp *interp, Tcl_Obj *method,
           int objc, Tcl_Obj *CONST objv[], int flags) {
  XOTclObject *obj = (XOTclObject*) clientData;
  int result;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);
  /*fprintf(stderr, "%%%% callmethod called with method %p\n", method),*/

  tov[0] = obj->cmdName;
  tov[1] = method;

  if (objc>2)
    memcpy(tov+2, objv, sizeof(Tcl_Obj *)*(objc-2));

  /*fprintf(stderr, "%%%% callMethod cmdname=%s, method=%s, objc=%d\n",
    ObjStr(tov[0]), ObjStr(tov[1]), objc);
    {int i; fprintf(stderr, "\t CALL: %s ", ObjStr(method));for(i=0; i<objc-2; i++) {
    fprintf(stderr,"%s ", ObjStr(objv[i]));} fprintf(stderr,"\n");}*/

  result = DoDispatch(clientData, interp, objc, tov, flags);

  FREE_ON_STACK(tov);
  return result;
}

int
XOTclCallMethodWithArgs(ClientData clientData, Tcl_Interp *interp, Tcl_Obj *method, Tcl_Obj *arg,
                        int givenobjc, Tcl_Obj *CONST objv[], int flags) {
  XOTclObject *obj = (XOTclObject*) clientData;
  int objc = givenobjc + 2;
  int result;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

  assert(objc>1);
  tov[0] = obj->cmdName;
  tov[1] = method;
  if (objc>2) {
    tov[2] = arg;
  }
  if (objc>3)
    memcpy(tov+3, objv, sizeof(Tcl_Obj *)*(objc-3));

  /*fprintf(stderr, "%%%% callMethodWithArg cmdname=%s, method=%s, objc=%d\n",
    ObjStr(tov[0]), ObjStr(tov[1]), objc);*/
  result = DoDispatch(clientData, interp, objc, tov, flags);

  FREE_ON_STACK(tov);
  return result;
}

#if defined(TCL85STACK)
# include "xotclStack85.c"
#else
# include "xotclStack.c"
#endif

/* extern callable GetSelfObj */
XOTcl_Object*
XOTclGetSelfObj(Tcl_Interp *interp) {
  return (XOTcl_Object*)GetSelfObj(interp);
}


/*
 * prints a msg to the screen that oldCmd is deprecated
 * optinal: give a new cmd
 */
static int 
XOTclDeprecatedCmd(Tcl_Interp *interp, char *oldCmd, char *newCmd) {
  fprintf(stderr, "**\n**\n** The command/method <%s> is deprecated.\n", oldCmd);
  if (newCmd)
    fprintf(stderr, "** Use <%s> instead.\n", newCmd);
  fprintf(stderr, "**\n");
  return TCL_OK;
}

#ifdef DISPATCH_TRACE
static void printObjv(int objc, Tcl_Obj *CONST objv[]) {
  int i, j;
  fprintf(stderr, "(%d)", objc);
  if (objc <= 3) j = objc; else j = 3;
  for (i=0;i<j;i++) fprintf(stderr, " %s", ObjStr(objv[i]));
  if (objc > 3) fprintf(stderr," ...");
  fprintf(stderr," (objc=%d)", objc);
}

static void printCall(Tcl_Interp *interp, char *string, int objc, Tcl_Obj *CONST objv[]) {
  fprintf(stderr, "     (%d) >%s: ", Tcl_Interp_numLevels(interp), string);
  printObjv(objc, objv);
  fprintf(stderr, "\n");
}
static void printExit(Tcl_Interp *interp, char *string,
                      int objc, Tcl_Obj *CONST objv[], int result) {
  fprintf(stderr, "     (%d) <%s: ", Tcl_Interp_numLevels(interp), string);
  /*printObjv(objc, objv);*/
  fprintf(stderr, " result=%d '%s'\n", result, ObjStr(Tcl_GetObjResult(interp)));
}
#endif


/*
 *  XOTclObject Reference Accounting
 */
#if defined(XOTCLOBJ_TRACE)
# define XOTclObjectRefCountIncr(obj)                                   \
  obj->refCount++;                                                      \
  fprintf(stderr, "RefCountIncr %p count=%d %s\n", obj, obj->refCount, obj->cmdName?ObjStr(obj->cmdName):"no name"); \
  MEM_COUNT_ALLOC("XOTclObject RefCount", obj)
# define XOTclObjectRefCountDecr(obj)					\
  obj->refCount--;							\
  fprintf(stderr, "RefCountDecr %p count=%d\n", obj, obj->refCount);	\
  MEM_COUNT_FREE("XOTclObject RefCount", obj)
#else
# define XOTclObjectRefCountIncr(obj)           \
  obj->refCount++;                              \
  MEM_COUNT_ALLOC("XOTclObject RefCount", obj)
# define XOTclObjectRefCountDecr(obj)           \
  obj->refCount--;                              \
  MEM_COUNT_FREE("XOTclObject RefCount", obj)
#endif

#if defined(XOTCLOBJ_TRACE)
void objTrace(char *string, XOTclObject *obj) {
  if (obj)
    fprintf(stderr,"--- %s tcl %p %s (%d %p) xotcl %p (%d) %s \n", string,
            obj->cmdName, obj->cmdName->typePtr ? obj->cmdName->typePtr->name : "NULL",
            obj->cmdName->refCount, obj->cmdName->internalRep.twoPtrValue.ptr1,
            obj, obj->refCount, objectName(obj));
  else
    fprintf(stderr,"--- No object: %s\n", string);
}
#else
# define objTrace(a, b)
#endif


/* search for tail of name */
static CONST char *
NSTail(CONST char *string) {
  register char *p = (char *)string+strlen(string);
  while (p > string) {
    if (*p == ':' && *(p-1) == ':') return p+1;
    p--;
  }
  return string;
}

XOTCLINLINE static int
isClassName(char *string) {
  return (strncmp((string), "::xotcl::classes", 16) == 0);
}

/* removes preceding ::xotcl::classes from a string */
XOTCLINLINE static char *
NSCutXOTclClasses(char *string) {
  assert(strncmp((string), "::xotcl::classes", 16) == 0);
  return string+16;
}

XOTCLINLINE static char *
NSCmdFullName(Tcl_Command cmd) {
  Tcl_Namespace *nsPtr = Tcl_Command_nsPtr(cmd);
  return nsPtr ? nsPtr->fullName : "";
}

static void
XOTclCleanupObject(XOTclObject *obj) {
  XOTclObjectRefCountDecr(obj);

  if (obj->refCount <= 0) {
    assert(obj->refCount == 0);
    assert(obj->flags & XOTCL_DELETED);

    MEM_COUNT_FREE("XOTclObject/XOTclClass", obj);
#if defined(XOTCLOBJ_TRACE)
    fprintf(stderr, "CKFREE Object %p refcount=%d\n", obj, obj->refCount);
#endif
#if !defined(NDEBUG)
    memset(obj, 0, sizeof(XOTclObject));
#endif
    /* fprintf(stderr,"CKFREE obj %p\n", obj);*/
    ckfree((char *) obj);
  }
}

/*
 *  Tcl_Obj functions for objects
 */

/* todo more generic */
XOTCLINLINE static Tcl_ObjType *
GetCmdNameType(Tcl_ObjType *cmdType) {
  static Tcl_ObjType *tclCmdNameType = NULL;

  if (tclCmdNameType == NULL) {
    static XOTclMutex initMutex = 0;
    XOTclMutexLock(&initMutex);
    if (tclCmdNameType == NULL)
      tclCmdNameType = Tcl_GetObjType("cmdName");
    XOTclMutexUnlock(&initMutex);
  }
  return tclCmdNameType;
}

static int
IsXOTclTclObj(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclObject **obj) {
  Tcl_ObjType *cmdType = objPtr->typePtr;
  if (cmdType == GetCmdNameType(cmdType)) {
    Tcl_Command cmd = Tcl_GetCommandFromObj(interp, objPtr);
    if (cmd) {
      XOTclObject *o = XOTclGetObjectFromCmdPtr(cmd);
      if (o) {
        *obj = o;
        return 1;
      }
    }
  }
  return 0;
}

/* Lookup an xotcl object from the given objPtr, preferably from an
 * object of type "cmdName". objPtr might be converted in this process.
 */

static int
GetObjectFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclObject **obj) {
  int result;
  XOTclObject *nobj;
  char *string;
  Tcl_Command cmd = Tcl_GetCommandFromObj(interp, objPtr);

  /*fprintf(stderr,"GetObjectFromObj  obj %s is of type tclCmd, cmd=%p\n", ObjStr(objPtr), cmd);*/
  if (cmd) {
    XOTclObject *o = XOTclGetObjectFromCmdPtr(cmd);
    /*fprintf(stderr,"GetObjectFromObj obj %s, o is %p objProc %p XOTclObjDispatch %p\n", ObjStr(objPtr), 
      o, Tcl_Command_objProc(cmd), XOTclObjDispatch);*/
    if (o) {
      if (obj) *obj = o;
      return TCL_OK;
    }
  } 

  /*fprintf(stderr, "GetObjectFromObj convertFromAny for %s type %p %s\n",ObjStr(objPtr), 
    objPtr->typePtr, objPtr->typePtr?objPtr->typePtr->name : "(none)");*/

  /* In case, we have to revolve via the callingNameSpace (i.e. the
   * argument is not fully qualified), we retry here.
   */
  string = ObjStr(objPtr);
  if (!isAbsolutePath(string)) {
    Tcl_Obj *tmpName = NameInNamespaceObj(interp, string, callingNameSpace(interp));
    char *nsString = ObjStr(tmpName);

    INCR_REF_COUNT(tmpName);
    nobj = XOTclpGetObject(interp, nsString);
    /*fprintf(stderr, " RETRY, string '%s' returned %p\n", nsString, nobj);*/
    DECR_REF_COUNT(tmpName);
  } else {
    nobj = NULL;
  }

  if (nobj) {
    if (obj) *obj = nobj;
    result = TCL_OK;
  } else {
    result = TCL_ERROR;
  }
  return result;
}

static int
GetClassFromObj(Tcl_Interp *interp, register Tcl_Obj *objPtr,
		     XOTclClass **cl, XOTclClass *base) {
  XOTclObject *obj;
  XOTclClass *cls = NULL;
  int result = TCL_OK;
  char *objName = ObjStr(objPtr);
  Tcl_Command cmd ;

  /*fprintf(stderr, "GetClassFromObj %s base %p\n", objName, base);*/

  cmd = Tcl_GetCommandFromObj(interp, objPtr);
  if (cmd) {
    cls = XOTclGetClassFromCmdPtr(cmd);
    if (cls) {
      if (cl) *cl = cls;
      return TCL_OK;
    }
  }

  result = GetObjectFromObj(interp, objPtr, &obj);
  if (result == TCL_OK) {
    cls = XOTclObjectToClass(obj);
    if (cls) {
      if (cl) *cl = cls;
      return TCL_OK;      
    }
  }

  /*fprintf(stderr,"try unknown\n");*/
  if (base) {
    Tcl_Obj *ov[3];
    ov[0] = base->object.cmdName;
    ov[1] = XOTclGlobalObjects[XOTE___UNKNOWN];
    if (isAbsolutePath(objName)) {
      ov[2] = objPtr;
    } else {
      ov[2] = NameInNamespaceObj(interp, objName, callingNameSpace(interp));
    }
    INCR_REF_COUNT(ov[2]);
    /*fprintf(stderr,"+++ calling %s __unknown for %s, objPtr=%s\n",
      ObjStr(ov[0]), ObjStr(ov[2]), ObjStr(objPtr)); */

    result = Tcl_EvalObjv(interp, 3, ov, 0);
    if (result == TCL_OK) {
      result = GetClassFromObj(interp, objPtr, cl, 0);
    }
    DECR_REF_COUNT(ov[2]);
  }

  return result;
}


#ifndef NAMESPACEINSTPROCS
static Tcl_Namespace *
GetCallerVarFrame(Tcl_Interp *interp, Tcl_CallFrame *varFramePtr) {
  Tcl_Namespace *nsPtr = NULL;
  if (varFramePtr) {
    Tcl_CallFrame *callerVarPtr = Tcl_CallFrame_callerVarPtr(varFramePtr);
    if (callerVarPtr) {
      nsPtr = (Tcl_Namespace *)callerVarPtr->nsPtr;
    }
  }
  if (nsPtr == NULL)
    nsPtr = Tcl_Interp_globalNsPtr(interp);

  return nsPtr;
}
#endif

static Tcl_Obj *
NameInNamespaceObj(Tcl_Interp *interp, char *name, Tcl_Namespace *nsPtr) {
  Tcl_Obj *objName;
  int len;
  char *p;

  /*fprintf(stderr,"NameInNamespaceObj %s (%p, %s) ", name, nsPtr, nsPtr?nsPtr->fullName:NULL);*/
  if (!nsPtr)
    nsPtr = Tcl_GetCurrentNamespace(interp);
  /*fprintf(stderr," (resolved %p, %s) ", nsPtr, nsPtr?nsPtr->fullName:NULL);*/
  objName = Tcl_NewStringObj(nsPtr->fullName,-1);
  len = Tcl_GetCharLength(objName);
  p = ObjStr(objName);
  if (len == 2 && p[0] == ':' && p[1] == ':') {
  } else {
    Tcl_AppendToObj(objName,"::", 2);
  }
  Tcl_AppendToObj(objName, name, -1);

  /*fprintf(stderr,"returns %s\n", ObjStr(objName));*/
  return objName;
}

extern void
XOTclClassListFree(XOTclClasses *sl) {
  XOTclClasses *n;
  for (; sl; sl = n) {
    n = sl->nextPtr;
    FREE(XOTclClasses, sl);
  }
}

/* reverse class list, caller is responsible for freeing data */
static XOTclClasses*
XOTclReverseClasses(XOTclClasses *sl) {
  XOTclClasses *firstPtr = NULL;
  for (; sl; sl = sl->nextPtr) {
    XOTclClasses *element = NEW(XOTclClasses);
    element->cl = sl->cl;
    element->clientData = sl->clientData;
    element->nextPtr = firstPtr;
    firstPtr = element;
  }
  return firstPtr;
}

extern XOTclClasses**
XOTclClassListAdd(XOTclClasses **cList, XOTclClass *cl, ClientData clientData) {
  XOTclClasses *l = *cList, *element = NEW(XOTclClasses);
  element->cl = cl;
  element->clientData = clientData;
  element->nextPtr = NULL;

  if (l) {
    while (l->nextPtr) l = l->nextPtr;
    l->nextPtr = element;
  } else
    *cList = element;
  return &(element->nextPtr);
}

void
XOTclObjectListFree(XOTclObjects *sl) {
  XOTclObjects *n;
  for (; sl; sl = n) {
    n = sl->nextPtr;
    FREE(XOTclObjects, sl);
  }
}

XOTclObjects**
XOTclObjectListAdd(XOTclObjects **cList, XOTclObject *obj) {
  XOTclObjects *l = *cList, *element = NEW(XOTclObjects);
  element->obj = obj;
  element->nextPtr = NULL;

  if (l) {
    while (l->nextPtr) l = l->nextPtr;
    l->nextPtr = element;
  } else
    *cList = element;
  return &(element->nextPtr);
}


/*
 * precedence ordering functions
 */

enum colors { WHITE, GRAY, BLACK };

static XOTclClasses *Super(XOTclClass *cl) { return cl->super; }
static XOTclClasses *Sub(XOTclClass *cl) { return cl->sub; }


static int
TopoSort(XOTclClass *cl, XOTclClass *base, XOTclClasses *(*next)(XOTclClass*)) {
  /*XOTclClasses *sl = (*next)(cl);*/
  XOTclClasses *sl = next == Super ? cl->super : cl->sub;
  XOTclClasses *pl;

  /*
   * careful to reset the color of unreported classes to
   * white in case we unwind with error, and on final exit
   * reset color of reported classes to white
   */

  cl->color = GRAY;
  for (; sl; sl = sl->nextPtr) {
    XOTclClass *sc = sl->cl;
    if (sc->color == GRAY) { cl->color = WHITE; return 0; }
    if (sc->color == WHITE && !TopoSort(sc, base, next)) {
      cl->color = WHITE;
      if (cl == base) {
        register XOTclClasses *pc;
        for (pc = cl->order; pc; pc = pc->nextPtr) { pc->cl->color = WHITE; }
      }
      return 0;
    }
  }
  cl->color = BLACK;
  pl = NEW(XOTclClasses);
  pl->cl = cl;
  pl->nextPtr = base->order;
  base->order = pl;
  if (cl == base) {
    register XOTclClasses *pc;
    for (pc = cl->order; pc; pc = pc->nextPtr) { pc->cl->color = WHITE; }
  }
  return 1;
}

static XOTclClasses*
TopoOrder(XOTclClass *cl, XOTclClasses *(*next)(XOTclClass*)) {
  if (TopoSort(cl, cl, next))
    return cl->order;
  XOTclClassListFree(cl->order);
  return cl->order = NULL;
}

static XOTclClasses*
ComputeOrder(XOTclClass *cl, XOTclClasses *order, XOTclClasses *(*direction)(XOTclClass*)) {
  if (order)
    return order;
  return cl->order = TopoOrder(cl, direction);
}

extern XOTclClasses*
XOTclComputePrecedence(XOTclClass *cl) {
  return ComputeOrder(cl, cl->order, Super);
}

extern XOTclClasses*
XOTclComputeDependents(XOTclClass *cl) {
  return ComputeOrder(cl, cl->order, Sub);
}


static void
FlushPrecedencesOnSubclasses(XOTclClass *cl) {
  XOTclClasses *pc;
  XOTclClassListFree(cl->order);
  cl->order = NULL;
  pc = ComputeOrder(cl, cl->order, Sub);

  /*
   * ordering doesn't matter here - we're just using toposort
   * to find all lower classes so we can flush their caches
   */

  if (pc) pc = pc->nextPtr;
  for (; pc; pc = pc->nextPtr) {
    XOTclClassListFree(pc->cl->order);
    pc->cl->order = NULL;
  }
  XOTclClassListFree(cl->order);
  cl->order = NULL;
}

static void
AddInstance(XOTclObject *obj, XOTclClass *cl) {
  obj->cl = cl;
  if (cl) {
    int nw;
    (void) Tcl_CreateHashEntry(&cl->instances, (char *)obj, &nw);
  }
}

static int
RemoveInstance(XOTclObject *obj, XOTclClass *cl) {
  if (cl) {
    Tcl_HashEntry *hPtr = XOTcl_FindHashEntry(&cl->instances, (char *)obj);
    if (hPtr) {
      Tcl_DeleteHashEntry(hPtr);
      return 1;
    }
  }
  return 0;
}

/*
 * superclass/subclass list maintenance
 */

static void
AS(XOTclClass *cl, XOTclClass *s, XOTclClasses **sl) {
  register XOTclClasses *l = *sl;
  while (l && l->cl != s) l = l->nextPtr;
  if (!l) {
    XOTclClasses *sc = NEW(XOTclClasses);
    sc->cl = s;
    sc->nextPtr = *sl;
    *sl = sc;
  }
}

static void
AddSuper(XOTclClass *cl, XOTclClass *super) {
  if (cl && super) {
    /*
     * keep corresponding sub in step with super
     */
    AS(cl, super, &cl->super);
    AS(super, cl, &super->sub);
  }
}

static int
RemoveSuper1(XOTclClass *cl, XOTclClass *s, XOTclClasses **sl) {
  XOTclClasses *l = *sl;
  if (!l) return 0;
  if (l->cl == s) {
    *sl = l->nextPtr;
    FREE(XOTclClasses, l);
    return 1;
  }
  while (l->nextPtr && l->nextPtr->cl != s) l = l->nextPtr;
  if (l->nextPtr) {
    XOTclClasses *n = l->nextPtr->nextPtr;
    FREE(XOTclClasses, l->nextPtr);
    l->nextPtr = n;
    return 1;
  }
  return 0;
}

static int
RemoveSuper(XOTclClass *cl, XOTclClass *super) {
  /*
   * keep corresponding sub in step with super
   */
  int sp = RemoveSuper1(cl, super, &cl->super);
  int sb = RemoveSuper1(super, cl, &super->sub);

  return sp && sb;
}

/*
 * internal type checking
 */

extern XOTcl_Class*
XOTclIsClass(Tcl_Interp *interp, ClientData clientData) {
  if (clientData && XOTclObjectIsClass((XOTclObject *)clientData))
    return (XOTcl_Class*) clientData;
  return 0;
}

/*
 * methods lookup
 */
XOTCLINLINE static Tcl_Command
FindMethod(char *methodName, Tcl_Namespace *nsPtr) {
  register Tcl_HashEntry *entryPtr;
  if ((entryPtr = XOTcl_FindHashEntry(Tcl_Namespace_cmdTable(nsPtr), methodName))) {
    return (Tcl_Command) Tcl_GetHashValue(entryPtr);
  }
  /*fprintf(stderr, "find %s in %p returns %p\n", methodName, cmdTable, cmd);*/
  return NULL;
}

static XOTclClass*
SearchPLMethod(XOTclClasses *pl, char *methodName, Tcl_Command *cmd) {
  /* Search the precedence list (class hierarchy) */
#if 1
  for (; pl;  pl = pl->nextPtr) {
    register Tcl_HashEntry *entryPtr = XOTcl_FindHashEntry(Tcl_Namespace_cmdTable(pl->cl->nsPtr), methodName);
    if (entryPtr) {
      *cmd = (Tcl_Command) Tcl_GetHashValue(entryPtr);
      return pl->cl;
    }
  }
#else
  for (; pl;  pl = pl->nextPtr) {
    if ((*cmd = FindMethod(methodName, pl->cl->nsPtr))) {
      return pl->cl;
    }
  }
#endif
  return NULL;
}


static XOTclClass*
SearchCMethod(XOTclClass *cl, char *nm, Tcl_Command *cmd) {
  assert(cl);
  return SearchPLMethod(ComputeOrder(cl, cl->order, Super), nm, cmd);
}

static int
callDestroyMethod(Tcl_Interp *interp, XOTclObject *obj, int flags) {
  int result;

  /* don't call destroy after exit handler started physical
     destruction */
  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound ==
      XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY)
    return TCL_OK;

  /* fprintf(stderr," obj %p flags %.4x %d\n", obj, obj->flags,
     RUNTIME_STATE(interp)->callDestroy);*/

  if (obj->flags & XOTCL_DESTROY_CALLED)
    return TCL_OK;

  PRINTOBJ("callDestroy", obj);

  /* flag, that destroy was called and invoke the method */
  obj->flags |= XOTCL_DESTROY_CALLED;
  result = callMethod(obj, interp, XOTclGlobalObjects[XOTE_DESTROY], 2, 0, flags);

  if (result != TCL_OK) {
    static char cmd[] =
      "puts stderr \"[self]: Error in method destroy\n\
	 $::errorCode $::errorInfo\"";
    Tcl_EvalEx(interp, cmd, -1, 0);
    if (++RUNTIME_STATE(interp)->errorCount > 20)
      Tcl_Panic("too many destroy errors occured. Endless loop?", NULL);
  } else {
    if (RUNTIME_STATE(interp)->errorCount > 0)
      RUNTIME_STATE(interp)->errorCount--;
  }

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "callDestroyMethod for %p exit\n", obj);
#endif
  return result;
}

/*
 * conditional memory allocations of optional storage
 */

extern XOTclObjectOpt *
XOTclRequireObjectOpt(XOTclObject *obj) {
  if (!obj->opt) {
    obj->opt = NEW(XOTclObjectOpt);
    memset(obj->opt, 0, sizeof(XOTclObjectOpt));
  }
  return obj->opt;
}

extern XOTclClassOpt*
XOTclRequireClassOpt(XOTclClass *cl) {
  assert(cl);
  if (!cl->opt) {
    cl->opt = NEW(XOTclClassOpt);
    memset(cl->opt, 0, sizeof(XOTclClassOpt));
    if (cl->object.flags & XOTCL_IS_CLASS) {
      cl->opt->id = cl->object.id;  /* probably a temporary solution */
    }
  }
  return cl->opt;
}




static Tcl_Namespace*
NSGetFreshNamespace(Tcl_Interp *interp, ClientData clientData, char *name);

static void
makeObjNamespace(Tcl_Interp *interp, XOTclObject *obj) {
#ifdef NAMESPACE_TRACE
  fprintf(stderr, "+++ Make Namespace for %s\n", objectName(obj));
#endif
  if (!obj->nsPtr) {
    Tcl_Namespace *nsPtr;
    char *cmdName =  objectName(obj);
    obj->nsPtr = NSGetFreshNamespace(interp, (ClientData)obj, cmdName);
    if (!obj->nsPtr)
      Tcl_Panic("makeObjNamespace: Unable to make namespace", NULL);
    nsPtr = obj->nsPtr;

    /*
     * Copy all obj variables to the newly created namespace
     */
    if (obj->varTable) {
      Tcl_HashSearch  search;
      Tcl_HashEntry   *hPtr;
      TclVarHashTable *varTable = Tcl_Namespace_varTable(nsPtr);
      Tcl_HashTable   *varHashTable = VarHashTable(varTable);
      Tcl_HashTable   *objHashTable = VarHashTable(obj->varTable);
	
      *varHashTable = *objHashTable; /* copy the table */
	
      if (objHashTable->buckets == objHashTable->staticBuckets) {
        varHashTable->buckets = varHashTable->staticBuckets;
      }
      for (hPtr = Tcl_FirstHashEntry(varHashTable, &search);  hPtr;
           hPtr = Tcl_NextHashEntry(&search)) {
#if defined(PRE85)
        Var *varPtr;
# if FORWARD_COMPATIBLE
        if (!forwardCompatibleMode) {
          varPtr = (Var *) Tcl_GetHashValue(hPtr);
          varPtr->nsPtr = (Namespace *)nsPtr;
        }
# else
        varPtr = (Var *) Tcl_GetHashValue(hPtr);
        varPtr->nsPtr = (Namespace *)nsPtr;
# endif
#endif
        hPtr->tablePtr = varHashTable;
      }
	
      ckfree((char *) obj->varTable);
      obj->varTable = NULL;
    }
  }
}

/*
  typedef int (Tcl_ResolveVarProc) _ANSI_ARGS_((
  *	        Tcl_Interp *interp, CONST char * name, Tcl_Namespace *context,
  *	        int flags, Tcl_Var *rPtr));
  */
int
varResolver(Tcl_Interp *interp, CONST char *name, Tcl_Namespace *nsPtr, int flags, Tcl_Var *varPtr) {
  int new;
  Tcl_Obj *key;
  Tcl_CallFrame *varFramePtr;
  Var *newVar;

  /* Case 1: The variable is to be resolved in global scope, proceed in
   * resolver chain (i.e. return TCL_CONTINUE)
   *
   * Note: For now, I am not aware of this case to become effective,
   * it is a mere safeguard measure.
   *
   * TODO: Can it be omitted safely?
   */

  if (flags & TCL_GLOBAL_ONLY) {
    /*fprintf(stderr, "global-scoped var detected '%s' in NS '%s'\n", name, \
      varFramePtr->nsPtr->fullName);*/
    return TCL_CONTINUE;
  }

  /* Case 2: The variable appears as to be proc-local, so proceed in
   * resolver chain (i.e. return TCL_CONTINUE)
   *
   * Note 1: This happens to be a rare occurrence, e.g. for nested
   * object structures which are shadowed by nested Tcl namespaces.
   *
   * TODO: Cannot reproduce the issue found with xotcl::package->require()
   *
   * Note 2: It would be possible to resolve the proc-local variable
   * directly (by digging into compiled and non-compiled locals etc.),
   * however, it would cause further code redundance.
   */

  varFramePtr = nonXotclObjectProcFrame((Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp));
  
  /*fprintf(stderr,"varFramePtr=%p, isProcCallFrame=%.6x %p\n",varFramePtr,
            varFramePtr ? Tcl_CallFrame_isProcCallFrame(varFramePtr): 0,
            varFramePtr ? Tcl_CallFrame_procPtr(varFramePtr): 0);*/

  if (varFramePtr && Tcl_CallFrame_isProcCallFrame(varFramePtr)) {
    /*fprintf(stderr, "proc-scoped var detected '%s' in NS '%s'\n", name,
      varFramePtr->nsPtr->fullName);*/
    return TCL_CONTINUE;
  }

  /*
   * Check for absolutely/relatively qualified variable names, i.e.
   * make sure that the variable name does not contain any namespace qualifiers.
   * Proceed with a TCL_CONTINUE, otherwise.
   */

  if ((*name == ':' && *(name+1) == ':') || NSTail(name) != name) {
    return TCL_CONTINUE;
  }

  /* Case 3: Does the variable exist in the per-object namespace? */
  *varPtr = (Tcl_Var)LookupVarFromTable(Tcl_Namespace_varTable(nsPtr), name, NULL);

  if(*varPtr == NULL) {
    /* We failed to find the variable so far, therefore we create it
     * here in the namespace.  Note that the cases (1), (2) and (3)
     * TCL_CONTINUE care for variable creation if necessary.
     */

    key = Tcl_NewStringObj(name, -1);

    INCR_REF_COUNT(key);
    newVar = VarHashCreateVar(Tcl_Namespace_varTable(nsPtr), key, &new);
    DECR_REF_COUNT(key);

#if defined(PRE85)
    newVar->nsPtr = (Namespace *)nsPtr;
#endif
    *varPtr = (Tcl_Var)newVar;
  }
  return *varPtr ? TCL_OK : TCL_ERROR;
}

static Tcl_Namespace *
requireObjNamespace(Tcl_Interp *interp, XOTclObject *obj) {
  if (!obj->nsPtr) makeObjNamespace(interp, obj);

  /* This puts a per-object namespace resolver into position upon
   * acquiring the namespace. Works for object-scoped commands/procs
   * and object-only ones (set, unset, ...)
   */
  Tcl_SetNamespaceResolvers(obj->nsPtr, (Tcl_ResolveCmdProc*)NULL,
                            varResolver, (Tcl_ResolveCompiledVarProc*)NULL);
  return obj->nsPtr;
}

extern void
XOTclRequireObjNamespace(Tcl_Interp *interp, XOTcl_Object *obj) {
  requireObjNamespace(interp,(XOTclObject*) obj);
}


/*
 * Namespace related commands
 */

static int
NSDeleteCmd(Tcl_Interp *interp, Tcl_Namespace *nsPtr, char *name) {
  /* a simple deletion would delete a global command with
     the same name, if the command is not existing, so
     we use the CmdToken */
  Tcl_Command token;
  assert(nsPtr);
  if ((token = FindMethod(name, nsPtr))) {
    return Tcl_DeleteCommandFromToken(interp, token);
  }
  return -1;
}

static void CallStackDestroyObject(Tcl_Interp *interp, XOTclObject *obj);
static void PrimitiveCDestroy(ClientData clientData);
static void PrimitiveODestroy(ClientData clientData);
static void PrimitiveDestroy(ClientData clientData);

static void
NSDeleteChildren(Tcl_Interp *interp, Tcl_Namespace *ns) {
  Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(ns);
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "NSDeleteChildren %s\n", ns->fullName);
#endif

  Tcl_ForgetImport(interp, ns, "*"); /* don't destroy namespace imported objects */

  for (hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
    if (!Tcl_Command_cmdEpoch(cmd)) {
      char *oname = Tcl_GetHashKey(cmdTable, hPtr);
      Tcl_DString name;
      XOTclObject *obj;
      /* fprintf(stderr, " ... child %s\n", oname); */

      ALLOC_NAME_NS(&name, ns->fullName, oname);
      obj = XOTclpGetObject(interp, Tcl_DStringValue(&name));

      if (obj) {
        /* fprintf(stderr, " ... obj= %s\n", objectName(obj));*/
	
        /* in the exit handler physical destroy --> directly call destroy */
        if (RUNTIME_STATE(interp)->exitHandlerDestroyRound
            == XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY) {
          PrimitiveDestroy((ClientData) obj);
        } else {
          if (obj->teardown && !(obj->flags & XOTCL_DESTROY_CALLED)) {

            if (callDestroyMethod(interp, obj, 0) != TCL_OK) {
              /* destroy method failed, but we have to remove the command
                 anyway. */
              if (obj->teardown) {
                CallStackDestroyObject(interp, obj);
              }
            }
          }
        }
      }
      DSTRING_FREE(&name);
    }
  }
}

/*
 * ensure that a variable exists on object varTable or nsPtr->varTable,
 * if necessary create it. Return Var* if successful, otherwise 0
 */
static Var *
NSRequireVariableOnObj(Tcl_Interp *interp, XOTclObject *obj, char *name, int flgs) {
  XOTcl_FrameDecls;
  Var *varPtr, *arrayPtr;

  XOTcl_PushFrame(interp, obj);
  varPtr = TclLookupVar(interp, name, 0, flgs, "obj vwait",
                        /*createPart1*/ 1, /*createPart2*/ 0, &arrayPtr);
  XOTcl_PopFrame(interp, obj);
  return varPtr;
}

static int
XOTcl_DeleteCommandFromToken(Tcl_Interp *interp, Tcl_Command cmd) {
  CallStackClearCmdReferences(interp, cmd);
  return Tcl_DeleteCommandFromToken(interp, cmd);
}

/*
 * delete all vars & procs in a namespace
 */
static void
NSCleanupNamespace(Tcl_Interp *interp, Tcl_Namespace *ns) {
  TclVarHashTable *varTable = Tcl_Namespace_varTable(ns);
  Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(ns);
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  Tcl_Command cmd;

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "NSCleanupNamespace %p varTable %p\n", ns, varTable);
#endif
  /*
   * Delete all variables and initialize var table again
   * (DeleteVars frees the vartable)
   */
  TclDeleteVars((Interp *)interp, varTable);
  InitVarHashTable(varTable, (Namespace *)ns);

  /*
   * Delete all user-defined procs in the namespace
   */
  for (hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    cmd = (Tcl_Command) Tcl_GetHashValue(hPtr);
    /* objects should not be deleted here to preseve children deletion order*/
    if (!XOTclGetObjectFromCmdPtr(cmd)) {
      /*fprintf(stderr,"NSCleanupNamespace deleting %s %p\n",
        Tcl_Command_nsPtr(cmd)->fullName, cmd);*/
      XOTcl_DeleteCommandFromToken(interp, cmd);
    }
  }
}


static void
NSNamespaceDeleteProc(ClientData clientData) {
  /* dummy for ns identification by pointer comparison */
  XOTclObject *obj = (XOTclObject*) clientData;
  /*fprintf(stderr,"namespacedeleteproc obj=%p\n", clientData);*/
  if (obj) {
    obj->nsPtr = NULL;
  }
}

#ifdef EXPERIMENTAL_CMD_RESOLVER
static int
NSisXOTclNamespace(Tcl_Namespace *nsPtr) {
  return nsPtr->deleteProc == NSNamespaceDeleteProc;
}
#endif

void
XOTcl_DeleteNamespace(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  int activationCount = 0;
  Tcl_CallFrame *f = (Tcl_CallFrame *)Tcl_Interp_framePtr(interp);

  /*
    fprintf(stderr, "  ... correcting ActivationCount for %s was %d ",
    nsPtr->fullName, nsp->activationCount);
  */
  while (f) {
    if (f->nsPtr == nsPtr)
      activationCount++;
    f = Tcl_CallFrame_callerPtr(f);
  }

  Tcl_Namespace_activationCount(nsPtr) = activationCount;

  /*
    fprintf(stderr, "to %d. \n", nsp->activationCount);
  */
  MEM_COUNT_FREE("TclNamespace", nsPtr);
  if (Tcl_Namespace_deleteProc(nsPtr)) {
    /*fprintf(stderr,"calling deteteNamespace\n");*/
    Tcl_DeleteNamespace(nsPtr);
  }
}

static Tcl_Namespace*
NSGetFreshNamespace(Tcl_Interp *interp, ClientData clientData, char *name) {
  Tcl_Namespace *nsPtr = Tcl_FindNamespace(interp, name, NULL, 0);

  if (nsPtr) {
    if (nsPtr->deleteProc || nsPtr->clientData) {
      Tcl_Panic("Namespace '%s' exists already with delProc %p and clientData %p; Can only convert a plain Tcl namespace into an XOTcl namespace",
		name, nsPtr->deleteProc, nsPtr->clientData);
    }
    nsPtr->clientData = clientData;
    nsPtr->deleteProc = (Tcl_NamespaceDeleteProc *)NSNamespaceDeleteProc;
  } else {
    nsPtr = Tcl_CreateNamespace(interp, name, clientData,
                             (Tcl_NamespaceDeleteProc *)NSNamespaceDeleteProc);
  }
  MEM_COUNT_ALLOC("TclNamespace", nsPtr);
  return nsPtr;
}


/*
 * check colons for illegal object/class names
 */
XOTCLINLINE static int
NSCheckColons(char *name, unsigned l) {
  register char *n = name;
  if (*n == '\0') return 0; /* empty name */
  if (l == 0) l=strlen(name);
  if (*(n+l-1) == ':')  return 0; /* name ends with : */
  if (*n == ':' && *(n+1) != ':') return 0; /* name begins with single : */
  for (; *n != '\0'; n++) {
    if (*n == ':' && *(n+1) == ':' && *(n+2) == ':')
      return 0;   /* more than 2 colons in series in a name */
  }
  return 1;
}

/*
 * check for parent namespace existance (used before commands are created)
 */
XOTCLINLINE static int
NSCheckForParent(Tcl_Interp *interp, char *name, unsigned l, XOTclClass *cl) {
  register char *n = name+l;
  int result = 1;

  /*search for last '::'*/
  while ((*n != ':' || *(n-1) != ':') && n-1 > name) {n--; }
  if (*n == ':' && n > name && *(n-1) == ':') {n--;}

  if ((n-name)>0) {
    Tcl_DString parentNSName, *dsp = &parentNSName;
    char *parentName;
    DSTRING_INIT(dsp);

    Tcl_DStringAppend(dsp, name, (n-name));
    parentName = Tcl_DStringValue(dsp);

    if (Tcl_FindNamespace(interp, parentName, (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY) == NULL) {
      XOTclObject *parentObj = (XOTclObject*) XOTclpGetObject(interp, parentName);
      if (parentObj) {
        /* this is for classes */
        requireObjNamespace(interp, parentObj);
      } else {
        /* call unknown and try again */
        Tcl_Obj *ov[3];
        int rc;
        ov[0] = DefaultSuperClass(interp, cl, cl->object.cl, 0)->object.cmdName;
        ov[1] = XOTclGlobalObjects[XOTE___UNKNOWN];
        ov[2] = Tcl_NewStringObj(parentName,-1);
        INCR_REF_COUNT(ov[2]);
        /*fprintf(stderr,"+++ parent... calling __unknown for %s\n", ObjStr(ov[2]));*/
        rc = Tcl_EvalObjv(interp, 3, ov, 0);
	if (rc == TCL_OK) {
          XOTclObject *parentObj = (XOTclObject*) XOTclpGetObject(interp, parentName);
          if (parentObj) {
            requireObjNamespace(interp, parentObj);
          }
          result = (Tcl_FindNamespace(interp, parentName,
                                      (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY) != NULL);
        } else {
          result = 0;
        }
        DECR_REF_COUNT(ov[2]);
      }
    } else {
      XOTclObject *parentObj = (XOTclObject*) XOTclpGetObject(interp, parentName);
      if (parentObj) {
        requireObjNamespace(interp, parentObj);
      }
    }
    DSTRING_FREE(dsp);
  }
  return result;
}

/*
 * Find the "real" command belonging eg. to an XOTcl class or object.
 * Do not return cmds produced by Tcl_Import, but the "real" cmd
 * to which they point.
 */
XOTCLINLINE static Tcl_Command
NSFindCommand(Tcl_Interp *interp, char *name, Tcl_Namespace *ns) {
  Tcl_Command cmd;
  if ((cmd = Tcl_FindCommand(interp, name, ns, 0))) {
    Tcl_Command importedCmd;
    if ((importedCmd = TclGetOriginalCommand(cmd)))
      cmd = importedCmd;
  }
  return cmd;
}



/*
 * C interface routines for manipulating objects and classes
 */


extern XOTcl_Object*
XOTclGetObject(Tcl_Interp *interp, char *name) {
  return (XOTcl_Object*) XOTclpGetObject(interp, name);
}

/*
 * Find an object using a char *name
 */
static XOTclObject*
XOTclpGetObject(Tcl_Interp *interp, char *name) {
  register Tcl_Command cmd;
  assert(name);
  /*fprintf(stderr, "XOTclpGetObject name = '%s'\n",name);*/

  cmd = NSFindCommand(interp, name, NULL);

  /*if (cmd) {
    fprintf(stderr,"+++ XOTclGetObject from %s -> objProc=%p, dispatch=%p OK %d\n",
            name, Tcl_Command_objProc(cmd), XOTclObjDispatch, Tcl_Command_objProc(cmd) == XOTclObjDispatch);
            }*/

  if (cmd && Tcl_Command_objProc(cmd) == XOTclObjDispatch) {
    /*fprintf(stderr, "XOTclpGetObject cd %p\n",Tcl_Command_objClientData(cmd));*/
    return (XOTclObject*)Tcl_Command_objClientData(cmd);
  }
  return 0;
}

/*
 * Find a class using a char *name
 */

extern XOTcl_Class*
XOTclGetClass(Tcl_Interp *interp, char *name) {
  return (XOTcl_Class*)XOTclpGetClass(interp, name);
}

static XOTclClass*
XOTclpGetClass(Tcl_Interp *interp, char *name) {
  XOTclObject *obj = XOTclpGetObject(interp, name);
  return (obj && XOTclObjectIsClass(obj)) ? (XOTclClass*)obj : NULL;
}

Tcl_Command
XOTclAddObjectMethod(Tcl_Interp *interp, XOTcl_Object *object, CONST char *methodName,
                     Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp,
                     int flags) {
  XOTclObject *obj = (XOTclObject *)object;
  Tcl_DString newCmdName, *dsPtr = &newCmdName;
  Tcl_Command newCmd;
  Tcl_Namespace *ns = requireObjNamespace(interp, obj);

  ALLOC_NAME_NS(dsPtr, ns->fullName, methodName);

  newCmd = Tcl_CreateObjCommand(interp, Tcl_DStringValue(dsPtr), proc, clientData, dp);
  if (flags) {
    ((Command *) newCmd)->flags |= flags;
  }
  DSTRING_FREE(dsPtr);
  return newCmd;
}

Tcl_Command
XOTclAddPMethod(Tcl_Interp *interp, XOTcl_Object *object, CONST char *methodName,
                Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp) {
  int flags = 0;
  if (clientData == (ClientData) XOTCL_CMD_NONLEAF_METHOD) {
    fprintf(stderr, "XOTclAddPMethod(,,,, XOTCL_CMD_NONLEAF_METHOD,) deprecated.\n"
	    "Use XOTclAddObjectMethod(,,,,,, XOTCL_CMD_NONLEAF_METHOD) instead.\n");
    flags = XOTCL_CMD_NONLEAF_METHOD;
    clientData = NULL;
  }
  return XOTclAddObjectMethod(interp, object, methodName, proc, clientData, dp, flags);
}


Tcl_Command
XOTclAddInstanceMethod(Tcl_Interp *interp, XOTcl_Class *class, CONST char *methodName,
                       Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp,
                       int flags) {
  XOTclClass *cl = (XOTclClass*) class;
  Tcl_DString newCmdName, *dsPtr = &newCmdName;
  Tcl_Command newCmd;
  ALLOC_NAME_NS(dsPtr, cl->nsPtr->fullName, methodName);
  newCmd = Tcl_CreateObjCommand(interp, Tcl_DStringValue(dsPtr), proc, clientData, dp);
  if (flags) {
    ((Command *) newCmd)->flags |= flags;
  }
  DSTRING_FREE(dsPtr);
  return newCmd;
}

Tcl_Command
XOTclAddIMethod(Tcl_Interp *interp, XOTcl_Class *class, CONST char *methodName,
                Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp) {
  int flags = 0;
  if (clientData == (ClientData) XOTCL_CMD_NONLEAF_METHOD) {
    fprintf(stderr, "XOTclAddIMethod(,,,, XOTCL_CMD_NONLEAF_METHOD,) deprecated.\n"
	    "Use XOTclAddInstanceMethod(,,,,,, XOTCL_CMD_NONLEAF_METHOD) instead.\n");
    flags = XOTCL_CMD_NONLEAF_METHOD;
    clientData = NULL;
  }
  return XOTclAddInstanceMethod(interp, class, methodName, proc, clientData, dp, flags);
}



/*
 * Generic Tcl_Obj List
 */

static void
TclObjListFreeList(XOTclTclObjList *list) {
  XOTclTclObjList *del;
  while (list) {
    del = list;
    list = list->nextPtr;
    DECR_REF_COUNT(del->content);
    FREE(XOTclTclObjList, del);
  }
}

static Tcl_Obj *
TclObjListNewElement(XOTclTclObjList **list, Tcl_Obj *ov) {
  XOTclTclObjList *elt = NEW(XOTclTclObjList);
  INCR_REF_COUNT(ov);
  elt->content = ov;
  elt->nextPtr = *list;
  *list = elt;
  return ov;
}

/*
 * Autonaming
 */

static Tcl_Obj *
AutonameIncr(Tcl_Interp *interp, Tcl_Obj *name, XOTclObject *obj,
             int instanceOpt, int resetOpt) {
  int valueLength, mustCopy = 1, format = 0;
  char *valueString, *c;
  Tcl_Obj *valueObject, *result = NULL, *savedResult = NULL;
  int flgs = TCL_LEAVE_ERR_MSG;
  XOTcl_FrameDecls;

  XOTcl_PushFrame(interp, obj);
  if (obj->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  valueObject = Tcl_ObjGetVar2(interp, XOTclGlobalObjects[XOTE_AUTONAMES], name, flgs);
  if (valueObject) {
    long autoname_counter;
    /* should probably do an overflow check here */
    Tcl_GetLongFromObj(interp, valueObject,&autoname_counter);
    autoname_counter++;
    if (Tcl_IsShared(valueObject)) {
      valueObject = Tcl_DuplicateObj(valueObject);
    }
    Tcl_SetLongObj(valueObject, autoname_counter);
  }
  Tcl_ObjSetVar2(interp, XOTclGlobalObjects[XOTE_AUTONAMES], name,
		 valueObject, flgs);

  if (resetOpt) {
    if (valueObject) { /* we have an entry */
      Tcl_UnsetVar2(interp, XOTclGlobalStrings[XOTE_AUTONAMES], ObjStr(name), flgs);
    }
    result = XOTclGlobalObjects[XOTE_EMPTY];
    INCR_REF_COUNT(result);
  } else {
    if (valueObject == NULL) {
      valueObject = Tcl_ObjSetVar2(interp, XOTclGlobalObjects[XOTE_AUTONAMES],
                                   name, XOTclGlobalObjects[XOTE_ONE], flgs);
    }
    if (instanceOpt) {
      char buffer[1], firstChar, *nextChars;
      nextChars = ObjStr(name);
      firstChar = *(nextChars ++);
      if (isupper((int)firstChar)) {
        buffer[0] = tolower((int)firstChar);
        result = Tcl_NewStringObj(buffer, 1);
        INCR_REF_COUNT(result);
        Tcl_AppendToObj(result, nextChars, -1);
        mustCopy = 0;
      }
    }
    if (mustCopy) {
      result = Tcl_DuplicateObj(name);
      INCR_REF_COUNT(result);
      /*
        fprintf(stderr,"*** copy %p %s = %p\n", name, ObjStr(name), result);
      */
    }
    /* if we find a % in the autoname -> We use Tcl_FormatObjCmd
       to let the autoname string be formated, like Tcl "format"
       command, with the value. E.g.:
       autoname a%06d --> a000000, a000001, a000002, ...
    */
    for (c = ObjStr(result); *c != '\0'; c++) {
      if (*c == '%') {
        if (*(c+1) != '%') {
          format = 1;
          break;
        } else {
          /* when we find a %% we format and then append autoname, e.g.
             autoname a%% --> a%1, a%2, ... */
          c++;
        }
      }
    }
    if (format) {
      ALLOC_ON_STACK(Tcl_Obj*, 3, ov);
      savedResult = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(savedResult);
      ov[0] = XOTclGlobalObjects[XOTE_FORMAT];
      ov[1] = result;
      ov[2] = valueObject;
      if (Tcl_EvalObjv(interp, 3, ov, 0) != TCL_OK) {
        XOTcl_PopFrame(interp, obj);
        DECR_REF_COUNT(savedResult);
        FREE_ON_STACK(ov);
        return 0;
      }
      DECR_REF_COUNT(result);
      result = Tcl_DuplicateObj(Tcl_GetObjResult(interp));
      INCR_REF_COUNT(result);
      Tcl_SetObjResult(interp, savedResult);
      DECR_REF_COUNT(savedResult);
      FREE_ON_STACK(ov);
    } else {
      valueString = Tcl_GetStringFromObj(valueObject,&valueLength);
      Tcl_AppendToObj(result, valueString, valueLength);
      /*fprintf(stderr,"+++ append to obj done\n");*/
    }
  }

  XOTcl_PopFrame(interp, obj);
  assert((resetOpt && result->refCount>=1) || (result->refCount == 1));
  return result;
}

/*
 * XOTcl CallStack
 */

static void
CallStackRestoreSavedFrames(Tcl_Interp *interp, callFrameContext *ctx) {
  if (ctx->framesSaved) {
    Tcl_Interp_varFramePtr(interp) = (CallFrame *)ctx->varFramePtr;
    /*RUNTIME_STATE(interp)->varFramePtr = ctx->varFramePtr;*/

  }
}

XOTCLINLINE static void
CallStackDoDestroy(Tcl_Interp *interp, XOTclObject *obj) {
  Tcl_Command oid;

  PRINTOBJ("CallStackDoDestroy", obj);

  /* Don't do anything, if a recursive DURING_DELETE is for some
   * reason active.
   */
  if (obj->flags & XOTCL_DURING_DELETE) {
    return;
  }
  obj->flags |= XOTCL_DURING_DELETE;

  oid = obj->id;
  if (obj->teardown && oid) {
    Tcl_Obj *savedObjResult = Tcl_GetObjResult(interp);
    INCR_REF_COUNT(savedObjResult);

    PrimitiveDestroy((ClientData) obj);

    /*fprintf(stderr, "    before DeleteCommandFromToken %p %.6x\n",oid,((Command*)oid)->flags);*/
    Tcl_DeleteCommandFromToken(interp, oid); /* this can change the result */
    /*fprintf(stderr, "    after DeleteCommandFromToken %p %.6x\n",oid,((Command*)oid)->flags);*/

    Tcl_SetObjResult(interp, savedObjResult);
    DECR_REF_COUNT(savedObjResult);
  }
}

static void
CallStackDestroyObject(Tcl_Interp *interp, XOTclObject *obj) {

  if ((obj->flags & XOTCL_DESTROY_CALLED) == 0) {
    /* if the destroy method was not called yet, do it now */
#ifdef OBJDELETION_TRACE
    fprintf(stderr, "  CallStackDestroyObject has to call destroy method for %p\n",obj);
#endif
    callDestroyMethod(interp, obj, 0);
    /*fprintf(stderr, "  CallStackDestroyObject after callDestroyMethod %p\n",obj);*/
  }

  /* if the object is not referenced on the callstack anymore
     we have to destroy it directly, because CallStackPop won't
     find the object destroy */
  if (obj->activationCount == 0) {
    CallStackDoDestroy(interp, obj);
  } else {
    /* to prevail the deletion order call delete children now
       -> children destructors are called before parent's
       destructor */
    if (obj->teardown && obj->nsPtr) {
      NSDeleteChildren(interp, obj->nsPtr);
    }
  }
}

/*
 * cmd list handling
 */

/*
 * Cmd List Add/Remove ... returns the new element
 */
static XOTclCmdList*
CmdListAdd(XOTclCmdList **cList, Tcl_Command c, XOTclClass *clorobj, int noDuplicates) {
  XOTclCmdList *l = *cList, *new;

  /*
   * check for duplicates, if necessary
   */
  if (noDuplicates) {
    XOTclCmdList *h = l, **end = NULL;
    while (h) {
      if (h->cmdPtr == c)
        return h;
      end = &(h->nextPtr);
      h = h->nextPtr;
    }
    if (end) {
      /* no duplicates, no need to search below, we are at the end of the list */
      cList = end;
      l = NULL;
    }
  }

  /*
   * ok, we have no duplicates -> append "new"
   * to the end of the list
   */
  new = NEW(XOTclCmdList);
  new->cmdPtr = c;
  Tcl_Command_refCount(new->cmdPtr)++;
  MEM_COUNT_ALLOC("command refCount", new->cmdPtr);
  new->clientData = NULL;
  new->clorobj = clorobj;
  new->nextPtr = NULL;

  if (l) {
    while (l->nextPtr)
      l = l->nextPtr;
    l->nextPtr = new;
  } else
    *cList = new;
  return new;
}

static void
CmdListReplaceCmd(XOTclCmdList *replace, Tcl_Command cmd, XOTclClass *clorobj) {
  Tcl_Command del = replace->cmdPtr;
  replace->cmdPtr = cmd;
  replace->clorobj = clorobj;
  Tcl_Command_refCount(cmd)++;
  MEM_COUNT_ALLOC("command refCount", cmd);
  TclCleanupCommand((Command *)del);
  MEM_COUNT_FREE("command refCount", cmd);
}

#if 0
/** for debug purposes only */
static void
CmdListPrint(Tcl_Interp *interp, char *title, XOTclCmdList *cmdList) {
  if (cmdList)
    fprintf(stderr, title);
  while (cmdList) {
    fprintf(stderr, "   CL=%p, cmdPtr=%p %s, clorobj %p, clientData=%p\n",
            cmdList,
            cmdList->cmdPtr,
            in ? Tcl_GetCommandName(interp, cmdList->cmdPtr) : "",
            cmdList->clorobj,
            cmdList->clientData);
    cmdList = cmdList->next;
  }
}
#endif

/*
 * physically delete an entry 'del'
 */
static void
CmdListDeleteCmdListEntry(XOTclCmdList *del, XOTclFreeCmdListClientData *freeFct) {
  if (freeFct)
    (*freeFct)(del);
  MEM_COUNT_FREE("command refCount", del->cmdPtr);
  TclCleanupCommand((Command *)del->cmdPtr);
  FREE(XOTclCmdList, del);
}

/*
 * remove a command 'delCL' from a command list, but do not
 * free it ... returns the removed XOTclCmdList*
 */
static XOTclCmdList*
CmdListRemoveFromList(XOTclCmdList **cmdList, XOTclCmdList *delCL) {
  register XOTclCmdList *c = *cmdList, *del = NULL;
  if (c == NULL)
    return NULL;
  if (c == delCL) {
    *cmdList = c->nextPtr;
    del = c;
  } else {
    while (c->nextPtr && c->nextPtr != delCL) {
      c = c->nextPtr;
    }
    if (c->nextPtr == delCL) {
      del = delCL;
      c->nextPtr = delCL->nextPtr;
    }
  }
  return del;
}

/*
 * remove all command pointers from a list that have a bumped epoch
 */
static void
CmdListRemoveEpoched(XOTclCmdList **cmdList, XOTclFreeCmdListClientData *freeFct) {
  XOTclCmdList *f = *cmdList, *del;
  while (f) {
    if (Tcl_Command_cmdEpoch(f->cmdPtr)) {
      del = f;
      f = f->nextPtr;
      del = CmdListRemoveFromList(cmdList, del);
      CmdListDeleteCmdListEntry(del, freeFct);
    } else
      f = f->nextPtr;
  }
}


/*
 * delete all cmds with given context class object
 */
static void
CmdListRemoveContextClassFromList(XOTclCmdList **cmdList, XOTclClass *clorobj,
                                  XOTclFreeCmdListClientData *freeFct) {
  XOTclCmdList *c, *del = NULL;
  /*
    CmdListRemoveEpoched(cmdList, freeFct);
  */
  c = *cmdList;
  while (c && c->clorobj == clorobj) {
    del = c;
    *cmdList = c->nextPtr;
    CmdListDeleteCmdListEntry(del, freeFct);
    c = *cmdList;
  }

  while (c) {
    if (c->clorobj == clorobj) {
      del = c;
      c = *cmdList;
      while (c->nextPtr && c->nextPtr != del)
	c = c->nextPtr;
      if (c->nextPtr == del)
	c->nextPtr = del->nextPtr;
      CmdListDeleteCmdListEntry(del, freeFct);
    }
    c = c->nextPtr;
  }
}

/*
 * free the memory of a whole 'cmdList'
 */
static void
CmdListRemoveList(XOTclCmdList **cmdList, XOTclFreeCmdListClientData *freeFct) {
  XOTclCmdList *del;
  while (*cmdList) {
    del = *cmdList;
    *cmdList = (*cmdList)->nextPtr;
    CmdListDeleteCmdListEntry(del, freeFct);
  }
}

/*
 * simple list search proc to search a list of cmds
 * for a command ptr
 */
static XOTclCmdList*
CmdListFindCmdInList(Tcl_Command cmd, XOTclCmdList *l) {
  register XOTclCmdList *h;
  for (h = l; h; h = h->nextPtr) {
    if (h->cmdPtr == cmd)
      return h;
  }
  return 0;
}

/*
 * simple list search proc to search a list of cmds
 * for a simple Name
 */
static XOTclCmdList*
CmdListFindNameInList(Tcl_Interp *interp, char *name, XOTclCmdList *l) {
  register XOTclCmdList *h;
  for (h = l; h; h = h->nextPtr) {
    CONST char *cmdName = Tcl_GetCommandName(interp, h->cmdPtr);
    if (cmdName[0] == name[0] && !strcmp(cmdName, name))
      return h;
  }
  return 0;
}

/*
 * Assertions
 */
static XOTclTclObjList*
AssertionNewList(Tcl_Interp *interp, Tcl_Obj *aObj) {
  Tcl_Obj **ov; int oc;
  XOTclTclObjList *last = NULL;

  if (Tcl_ListObjGetElements(interp, aObj, &oc, &ov) == TCL_OK) {
    if (oc > 0) {
      int i;
      for (i=oc-1; i>=0; i--) {
        TclObjListNewElement(&last, ov[i]);
      }
    }
  }
  return last;
}

static Tcl_Obj *
AssertionList(Tcl_Interp *interp, XOTclTclObjList *alist) {
  Tcl_Obj *newAssStr = Tcl_NewStringObj("", 0);
  for (; alist; alist = alist->nextPtr) {
    Tcl_AppendStringsToObj(newAssStr, "{", ObjStr(alist->content),
                           "}", (char *) NULL);
    if (alist->nextPtr)
      Tcl_AppendStringsToObj(newAssStr, " ", (char *) NULL);
  }
  return newAssStr;
}

/* append a string of pre and post assertions to a proc
   or instproc body */
static void
AssertionAppendPrePost(Tcl_Interp *interp, Tcl_DString *dsPtr, XOTclProcAssertion *procs) {
  if (procs) {
    Tcl_Obj *preAss = AssertionList(interp, procs->pre);
    Tcl_Obj *postAss = AssertionList(interp, procs->post);
    INCR_REF_COUNT(preAss); INCR_REF_COUNT(postAss);
    Tcl_DStringAppendElement(dsPtr, ObjStr(preAss));
    Tcl_DStringAppendElement(dsPtr, ObjStr(postAss));
    DECR_REF_COUNT(preAss); DECR_REF_COUNT(postAss);
  }
}

static int
AssertionListCheckOption(Tcl_Interp *interp, XOTclObject *obj) {
  XOTclObjectOpt *opt = obj->opt;
  if (!opt)
    return TCL_OK;
  if (opt->checkoptions & CHECK_OBJINVAR)
    Tcl_AppendElement(interp, "invar");
  if (opt->checkoptions & CHECK_CLINVAR)
    Tcl_AppendElement(interp, "instinvar");
  if (opt->checkoptions & CHECK_PRE)
    Tcl_AppendElement(interp, "pre");
  if (opt->checkoptions & CHECK_POST)
    Tcl_AppendElement(interp, "post");
  return TCL_OK;
}

static XOTclProcAssertion*
AssertionFindProcs(XOTclAssertionStore *aStore, char *name) {
  Tcl_HashEntry *hPtr;
  if (aStore == NULL) return NULL;
  hPtr = XOTcl_FindHashEntry(&aStore->procs, name);
  if (hPtr == NULL) return NULL;
  return (XOTclProcAssertion*) Tcl_GetHashValue(hPtr);
}

static void
AssertionRemoveProc(XOTclAssertionStore *aStore, char *name) {
  Tcl_HashEntry *hPtr;
  if (aStore) {
    hPtr = XOTcl_FindHashEntry(&aStore->procs, name);
    if (hPtr) {
      XOTclProcAssertion *procAss =
        (XOTclProcAssertion*) Tcl_GetHashValue(hPtr);
      TclObjListFreeList(procAss->pre);
      TclObjListFreeList(procAss->post);
      FREE(XOTclProcAssertion, procAss);
      Tcl_DeleteHashEntry(hPtr);
    }
  }
}

static void
AssertionAddProc(Tcl_Interp *interp, char *name, XOTclAssertionStore *aStore,
                 Tcl_Obj *pre, Tcl_Obj *post) {
  int nw = 0;
  Tcl_HashEntry *hPtr = NULL;
  XOTclProcAssertion *procs = NEW(XOTclProcAssertion);

  AssertionRemoveProc(aStore, name);
  procs->pre = AssertionNewList(interp, pre);
  procs->post = AssertionNewList(interp, post);
  hPtr = Tcl_CreateHashEntry(&aStore->procs, name, &nw);
  if (nw) Tcl_SetHashValue(hPtr, (ClientData)procs);
}

static XOTclAssertionStore*
AssertionCreateStore() {
  XOTclAssertionStore *aStore = NEW(XOTclAssertionStore);
  aStore->invariants = NULL;
  Tcl_InitHashTable(&aStore->procs, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable",&aStore->procs);
  return aStore;
}

static void
AssertionRemoveStore(XOTclAssertionStore *aStore) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  if (aStore) {
    for (hPtr = Tcl_FirstHashEntry(&aStore->procs, &hSrch); hPtr;
         hPtr = Tcl_FirstHashEntry(&aStore->procs, &hSrch)) {
      /*
       * AssertionRemoveProc calls Tcl_DeleteHashEntry(hPtr), thus
       * we get the FirstHashEntry afterwards again to proceed
       */
      AssertionRemoveProc(aStore, Tcl_GetHashKey(&aStore->procs, hPtr));
    }
    Tcl_DeleteHashTable(&aStore->procs);
    MEM_COUNT_FREE("Tcl_InitHashTable",&aStore->procs);
    TclObjListFreeList(aStore->invariants);
    FREE(XOTclAssertionStore, aStore);
  }
}

/*
 * check a given condition in the current callframe's scope
 * it's the responsiblity of the caller to push the intended callframe
 */
static int
checkConditionInScope(Tcl_Interp *interp, Tcl_Obj *condition) {
  int result, success;
  Tcl_Obj *ov[2] = {NULL, condition};

  INCR_REF_COUNT(condition);
  result = XOTcl_ExprObjCmd(NULL, interp, 2, ov);
  DECR_REF_COUNT(condition);

  if (result == TCL_OK) {
    result = Tcl_GetBooleanFromObj(interp, Tcl_GetObjResult(interp),&success);

    if (result == TCL_OK && success == 0)
      result = XOTCL_CHECK_FAILED;
  }
  return result;
}

static int
AssertionCheckList(Tcl_Interp *interp, XOTclObject *obj,
                   XOTclTclObjList *alist, char *methodName) {
  XOTclTclObjList *checkFailed = NULL;
  Tcl_Obj *savedObjResult = Tcl_GetObjResult(interp);
  int savedCheckoptions, acResult = TCL_OK;

  /*
   * no obj->opt -> checkoption == CHECK_NONE
   */
  if (!obj->opt)
    return TCL_OK;

  /* we do not check assertion modifying methods, otherwise
     we can not react in catch on a runtime assertion check failure */
  if (isCheckString(methodName) || isInfoString(methodName) ||
      isInvarString(methodName) || isInstinvarString(methodName) ||
      isProcString(methodName) || isInstprocString(methodName))
    return TCL_OK;

  INCR_REF_COUNT(savedObjResult);

  Tcl_ResetResult(interp);

  while (alist) {
    /* Eval instead of IfObjCmd => the substitutions in the
       conditions will be done by Tcl */
    char *assStr = ObjStr(alist->content), *c = assStr;
    int comment = 0;

    for (; c && *c != '\0'; c++) {
      if (*c == '#') {
        comment = 1; break;
      }
    }

    if (!comment) {
      XOTcl_FrameDecls;
      XOTcl_PushFrame(interp, obj);
#if !defined(TCL85STACK)
      CallStackPush(interp, obj, 0, 0, XOTCL_CSC_TYPE_PLAIN);
#endif

      /* don't check assertion during assertion check */
      savedCheckoptions = obj->opt->checkoptions;
      obj->opt->checkoptions = CHECK_NONE;

      /* fprintf(stderr, "Checking Assertion %s ", assStr); */

      /*
       * now check the assertion in the pushed callframe's scope
       */
      acResult = checkConditionInScope(interp, alist->content);
      if (acResult != TCL_OK)
        checkFailed = alist;

      obj->opt->checkoptions = savedCheckoptions;

      /* fprintf(stderr, "...%s\n", checkFailed ? "failed" : "ok"); */
#if !defined(TCL85STACK)
      CallStackPop(interp, NULL);
#endif
      XOTcl_PopFrame(interp, obj);
    }
    if (checkFailed)
      break;
    alist = alist->nextPtr;
  }

  if (checkFailed) {
    DECR_REF_COUNT(savedObjResult);
    if (acResult == TCL_ERROR) {
      Tcl_Obj *sr = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(sr);	
      XOTclVarErrMsg(interp, "Error in Assertion: {",
		     ObjStr(checkFailed->content), "} in proc '",
		     methodName, "'\n\n", ObjStr(sr), (char *) NULL);
      DECR_REF_COUNT(sr);
      return TCL_ERROR;
    }
    return XOTclVarErrMsg(interp, "Assertion failed check: {",
			  ObjStr(checkFailed->content), "} in proc '",
                          methodName, "'", (char *) NULL);
  }

  Tcl_SetObjResult(interp, savedObjResult);
  DECR_REF_COUNT(savedObjResult);
  return TCL_OK;
}

static int
AssertionCheckInvars(Tcl_Interp *interp, XOTclObject *obj, char *method,
                     CheckOptions checkoptions) {
  int result = TCL_OK;

  if (checkoptions & CHECK_OBJINVAR && obj->opt->assertions) {
    result = AssertionCheckList(interp, obj, obj->opt->assertions->invariants,
                                method);
  }

  if (result != TCL_ERROR && checkoptions & CHECK_CLINVAR) {
    XOTclClasses *clPtr;
    clPtr = ComputeOrder(obj->cl, obj->cl->order, Super);
    while (clPtr && result != TCL_ERROR) {
      XOTclAssertionStore *aStore = (clPtr->cl->opt) ? clPtr->cl->opt->assertions : 0;
      if (aStore) {
        result = AssertionCheckList(interp, obj, aStore->invariants, method);
      }
      clPtr = clPtr->nextPtr;
    }
  }
  return result;
}

static int
AssertionCheck(Tcl_Interp *interp, XOTclObject *obj, XOTclClass *cl,
               char *method, int checkOption) {
  XOTclProcAssertion *procs;
  int result = TCL_OK;
  XOTclAssertionStore *aStore;

  if (cl)
    aStore = cl->opt ? cl->opt->assertions : 0;
  else
    aStore = obj->opt ? obj->opt->assertions : 0;

  assert(obj->opt);

  if (checkOption & obj->opt->checkoptions) {
    procs = AssertionFindProcs(aStore, method);
    if (procs) {
      switch (checkOption) {
      case CHECK_PRE:
	result = AssertionCheckList(interp, obj, procs->pre, method);
        break;
      case CHECK_POST:
        result = AssertionCheckList(interp, obj, procs->post, method);
        break;
      }
    }
    if (result != TCL_ERROR)
      result = AssertionCheckInvars(interp, obj, method, obj->opt->checkoptions);
  }
  return result;
}




/*
 * Per-Object-Mixins
 */

/*
 * push a mixin stack information on this object
 */
static int
MixinStackPush(XOTclObject *obj) {
  register XOTclMixinStack *h = NEW(XOTclMixinStack);
  h->currentCmdPtr = NULL;
  h->nextPtr = obj->mixinStack;
  obj->mixinStack = h;
  return 1;
}

/*
 * pop a mixin stack information on this object
 */
static void
MixinStackPop(XOTclObject *obj) {
  register XOTclMixinStack *h = obj->mixinStack;
  obj->mixinStack = h->nextPtr;
  FREE(XOTclMixinStack, h);
}

/*
 * Appends XOTclClasses *containing the mixin classes and their
 * superclasses to 'mixinClasses' list from a given mixinList
 */
static void
MixinComputeOrderFullList(Tcl_Interp *interp, XOTclCmdList **mixinList,
                          XOTclClasses **mixinClasses,
                          XOTclClasses **checkList, int level) {
  XOTclCmdList *m;
  XOTclClasses *pl, **clPtr = mixinClasses;

  CmdListRemoveEpoched(mixinList, GuardDel);

  for (m = *mixinList; m; m = m->nextPtr) {
    XOTclClass *mCl = XOTclGetClassFromCmdPtr(m->cmdPtr);
    if (mCl) {
      for (pl = ComputeOrder(mCl, mCl->order, Super); pl; pl = pl->nextPtr) {
        /*fprintf(stderr, " %s, ", ObjStr(pl->cl->object.cmdName));*/
        if ((pl->cl->object.flags & XOTCL_IS_ROOT_CLASS) == 0) {
          XOTclClassOpt *opt = pl->cl->opt;
          if (opt && opt->instmixins) {
            /* compute transitively the instmixin classes of this added
               class */
            XOTclClasses *cls;
            int i, found = 0;
            for (i=0, cls = *checkList; cls; i++, cls = cls->nextPtr) {
              /* fprintf(stderr,"+++ c%d: %s\n", i,
                 className(cls->cl));*/
              if (pl->cl == cls->cl) {
                found = 1;
                break;
              }
            }
            if (!found) {
              XOTclClassListAdd(checkList, pl->cl, NULL);
              /*fprintf(stderr, "+++ transitive %s\n",
                ObjStr(pl->cl->object.cmdName));*/

              MixinComputeOrderFullList(interp, &opt->instmixins, mixinClasses,
                                        checkList, level+1);
            }
          }
          /* fprintf(stderr,"+++ add to mixinClasses %p path: %s clPtr %p\n",
             mixinClasses, ObjStr(pl->cl->object.cmdName), clPtr);*/
          clPtr = XOTclClassListAdd(clPtr, pl->cl, m->clientData);
        }
      }
    }
  }
  if (level == 0 && *checkList) {
    XOTclClassListFree(*checkList);
    *checkList = NULL;
  }
}

static void
MixinResetOrder(XOTclObject *obj) {
  /*fprintf(stderr,"removeList %s \n", objectName(obj));*/
  CmdListRemoveList(&obj->mixinOrder, NULL /*GuardDel*/);
  obj->mixinOrder = NULL;
}

/*
 * Computes a linearized order of per-object and per-class mixins. Then
 * duplicates in the full list and with the class inheritance list of
 * 'obj' are eliminated.
 * The precendence rule is that the last occurence makes it into the
 * final list.
 */
static void
MixinComputeOrder(Tcl_Interp *interp, XOTclObject *obj) {
  XOTclClasses *fullList, *checkList = NULL, *mixinClasses = NULL, *nextCl, *pl,
    *checker, *guardChecker;

  if (obj->mixinOrder)  MixinResetOrder(obj);

  /* append per-obj mixins */
  if (obj->opt) {
    MixinComputeOrderFullList(interp, &obj->opt->mixins, &mixinClasses,
                              &checkList, 0);
  }

  /* append per-class mixins */
  for (pl = ComputeOrder(obj->cl, obj->cl->order, Super); pl; pl = pl->nextPtr) {
    XOTclClassOpt *opt = pl->cl->opt;
    if (opt && opt->instmixins) {
      MixinComputeOrderFullList(interp, &opt->instmixins, &mixinClasses,
                                &checkList, 0);
    }
  }
  fullList = mixinClasses;

  /* use no duplicates & no classes of the precedence order
     on the resulting list */
  while (mixinClasses) {
    checker = nextCl = mixinClasses->nextPtr;
    /* fprintf(stderr,"--- checking %s\n",
       ObjStr(mixinClasses->cl->object.cmdName));*/

    while (checker) {
      if (checker->cl == mixinClasses->cl) break;
      checker = checker->nextPtr;
    }
    /* if checker is set, it is a duplicate and ignored */

    if (checker == NULL) {
      /* check obj->cl hierachy */
      for (checker = ComputeOrder(obj->cl, obj->cl->order, Super); checker; checker = checker->nextPtr) {
        if (checker->cl == mixinClasses->cl)
          break;
      }
      /* if checker is set, it was found in the class hierarchy
         and it is ignored */
    }
    if (checker == NULL) {
      /* add the class to the mixinOrder list */
      XOTclCmdList *new;
      /* fprintf(stderr,"--- adding to mixinlist %s\n",
         ObjStr(mixinClasses->cl->object.cmdName));*/
      new = CmdListAdd(&obj->mixinOrder, mixinClasses->cl->object.id, NULL,
                       /*noDuplicates*/ 0);

      /* in the client data of the order list, we require the first
         matching guard ... scan the full list for it. */
      for (guardChecker = fullList; guardChecker; guardChecker = guardChecker->nextPtr) {
        if (guardChecker->cl == mixinClasses->cl) {
          new->clientData = guardChecker->clientData;
          break;
        }
      }
    }
    mixinClasses = nextCl;
  }

  /* ... and free the memory of the full list */
  XOTclClassListFree(fullList);

  /*CmdListPrint(interp,"mixin order\n", obj->mixinOrder);*/

}

/*
 * add a mixin class to 'mixinList' by appending it
 */
static int
MixinAdd(Tcl_Interp *interp, XOTclCmdList **mixinList, Tcl_Obj *name, XOTclClass *base) {
  XOTclClass *mixin;
  Tcl_Obj *guard = NULL;
  int ocName; Tcl_Obj **ovName;
  XOTclCmdList *new;

  if (Tcl_ListObjGetElements(interp, name, &ocName, &ovName) == TCL_OK && ocName > 1) {
    if (ocName == 3 && !strcmp(ObjStr(ovName[1]), XOTclGlobalStrings[XOTE_GUARD_OPTION])) {
      name = ovName[0];
      guard = ovName[2];
      /*fprintf(stderr,"mixinadd name = '%s', guard = '%s'\n", ObjStr(name), ObjStr(guard));*/
    } /*else return XOTclVarErrMsg(interp, "mixin registration '", ObjStr(name),
        "' has too many elements.", (char *) NULL);*/
  }

  if (GetClassFromObj(interp, name, &mixin, base) != TCL_OK)
    return XOTclErrBadVal(interp, "mixin", "a class as mixin", ObjStr(name));


  new = CmdListAdd(mixinList, mixin->object.id, NULL, /*noDuplicates*/ 1);

  if (guard) {
    GuardAdd(interp, new, guard);
  } else {
    if (new->clientData)
      GuardDel(new);
  }

  return TCL_OK;
}

/*
 * call AppendElement for matching values
 */
static void
AppendMatchingElement(Tcl_Interp *interp, Tcl_Obj *name, char *pattern) {
  char *string = ObjStr(name);
  if (!pattern || Tcl_StringMatch(string, pattern)) {
    Tcl_AppendElement(interp, string);
  }
}

/*
 * apply AppendMatchingElement to CmdList
 */
static int
AppendMatchingElementsFromCmdList(Tcl_Interp *interp, XOTclCmdList *cmdl,
                                  char *pattern, XOTclObject *matchObject) {
  int rc = 0;
  for ( ; cmdl; cmdl = cmdl->nextPtr) {
    XOTclObject *obj = XOTclGetObjectFromCmdPtr(cmdl->cmdPtr);
    if (obj) {
      if (matchObject == obj) {
        return 1;
      } else {
        AppendMatchingElement(interp, obj->cmdName, pattern);
      }
    }
  }
  return rc;
}

/*
 * apply AppendMatchingElement to
 */
static int
AppendMatchingElementsFromClasses(Tcl_Interp *interp, XOTclClasses *cls,
				  char *pattern, XOTclObject *matchObject) {
  int rc = 0;

  for ( ; cls; cls = cls->nextPtr) {
    XOTclObject *obj = (XOTclObject *)cls->cl;
    if (obj) {
      if (matchObject && obj == matchObject) {
        /* we have a matchObject and it is identical to obj,
           just return true and don't continue search
        */
        return 1;
        break;
      } else {
        AppendMatchingElement(interp, obj->cmdName, pattern);
      }
    }
  }
  return rc;
}


/*
 * get all instances of a class recursively into an initialized
 * String key hashtable
 */
static void
getAllInstances(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclClass *startCl) {
  Tcl_HashTable *table = &startCl->instances;
  XOTclClasses *sc;
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;

  for (hPtr = Tcl_FirstHashEntry(table, &search);  hPtr;
       hPtr = Tcl_NextHashEntry(&search)) {
    XOTclObject *inst = (XOTclObject *)Tcl_GetHashKey(table, hPtr);
    int new;

    Tcl_CreateHashEntry(destTable, objectName(inst), &new);
    /*
      fprintf (stderr, " -- %s (%s)\n", objectName(inst), ObjStr(startCl->object.cmdName));
    */
  }
  for (sc = startCl->sub; sc; sc = sc->nextPtr) {
    getAllInstances(interp, destTable, sc->cl);
  }
}

/*
 * helper function for getAllClassMixinsOf to add classes to the
 * result set, flagging test for matchObject as result
 */

static int
addToResultSet(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclObject *obj, int *new,
	       int appendResult, char *pattern, XOTclObject *matchObject) {
  Tcl_CreateHashEntry(destTable, (char *)obj, new);
  if (*new) {
    if (matchObject && matchObject == obj) {
      return 1;
    }
    if (appendResult) {
      AppendMatchingElement(interp, obj->cmdName, pattern);
    }
  }
  return 0;
}

/*
 * helper function for getAllClassMixins to add classes with guards
 * to the result set, flagging test for matchObject as result
 */

static int
addToResultSetWithGuards(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclClass *cl, ClientData clientData, int *new,
			 int appendResult, char *pattern, XOTclObject *matchObject) {
  Tcl_CreateHashEntry(destTable, (char *)cl, new);
  if (*new) {
    if (appendResult) {
      if (!pattern || Tcl_StringMatch(className(cl), pattern)) {
        Tcl_Obj *l = Tcl_NewListObj(0, NULL);
        Tcl_Obj *g = (Tcl_Obj*) clientData;
        Tcl_ListObjAppendElement(interp, l, cl->object.cmdName);
        Tcl_ListObjAppendElement(interp, l, XOTclGlobalObjects[XOTE_GUARD_OPTION]);
        Tcl_ListObjAppendElement(interp, l, g);
	Tcl_AppendElement(interp, ObjStr(l));
	DECR_REF_COUNT(l);
      }
    }
    if (matchObject && matchObject == (XOTclObject *)cl) {
      return 1;
    }
  }
  return 0;
}

/*
 * recursively get all per object mixins from an class and its subclasses/isClassMixinOf
 * into an initialized object ptr hashtable (TCL_ONE_WORD_KEYS)
 */

static int
getAllObjectMixinsOf(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclClass *startCl,
		     int isMixin,
		     int appendResult, char *pattern, XOTclObject *matchObject) {
  int rc = 0, new = 0;
  XOTclClasses *sc;

  /*fprintf(stderr, "startCl = %s, opt %p, isMixin %d, pattern '%s', matchObject %p\n",
    className(startCl),startCl->opt, isMixin, pattern, matchObject);*/

  /*
   * check all subclasses of startCl for mixins
   */
  for (sc = startCl->sub; sc; sc = sc->nextPtr) {
    rc = getAllObjectMixinsOf(interp, destTable, sc->cl, isMixin, appendResult, pattern, matchObject);
    if (rc) {return rc;}
  }
  /*fprintf(stderr, "check subclasses of %s done\n",ObjStr(startCl->object.cmdName));*/

  if (startCl->opt) {
    XOTclCmdList *m;
    XOTclClass *cl;
    for (m = startCl->opt->isClassMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == NULL);

      cl = XOTclGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);
      /*fprintf(stderr, "check %s mixinof %s\n",
        className(cl),ObjStr(startCl->object.cmdName));*/
      rc = getAllObjectMixinsOf(interp, destTable, cl, isMixin, appendResult, pattern, matchObject);
      /* fprintf(stderr, "check %s mixinof %s done\n",
      className(cl),ObjStr(startCl->object.cmdName));*/
      if (rc) {return rc;}
    }
  }

  /*
   * check, if startCl has associated per-object mixins
   */
  if (startCl->opt) {
    XOTclCmdList *m;
    XOTclObject *obj;

    for (m = startCl->opt->isObjectMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == NULL);

      obj = XOTclGetObjectFromCmdPtr(m->cmdPtr);
      assert(obj);

      rc = addToResultSet(interp, destTable, obj, &new, appendResult, pattern, matchObject);
      if (rc == 1) {return rc;}
    }
  }
  return rc;
}

/*
 * recursively get all isClassMixinOf of a class into an initialized
 * object ptr hashtable (TCL_ONE_WORD_KEYS)
 */

static int
getAllClassMixinsOf(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclClass *startCl,
		    int isMixin,
                    int appendResult, char *pattern, XOTclObject *matchObject) {
  int rc = 0, new = 0;
  XOTclClass *cl;
  XOTclClasses *sc;

  /*
    fprintf(stderr, "startCl = %s, opt %p, isMixin %d\n",
    ObjStr(startCl->object.cmdName),startCl->opt, isMixin);
  */

  /*
   * the startCl is a per class mixin, add it to the result set
   */
  if (isMixin) {
    rc = addToResultSet(interp, destTable, &startCl->object, &new, appendResult, pattern, matchObject);
    if (rc == 1) {return rc;}

    /*
     * check all subclasses of startCl for mixins
     */
    for (sc = startCl->sub; sc; sc = sc->nextPtr) {
      rc = getAllClassMixinsOf(interp, destTable, sc->cl, isMixin, appendResult, pattern, matchObject);
      if (rc) {return rc;}
    }
  }

  /*
   * check, if startCl is a per-class mixin of some other classes
   */
  if (startCl->opt) {
    XOTclCmdList *m;

    for (m = startCl->opt->isClassMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == NULL);

      cl = XOTclGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);

      rc = addToResultSet(interp, destTable, &cl->object, &new, appendResult, pattern, matchObject);
      if (rc == 1) {return rc;}
      if (new) {
        rc = getAllClassMixinsOf(interp, destTable, cl, 1, appendResult, pattern, matchObject);
        if (rc) {return rc;}
      }
    }
  }

  return rc;
}

/*
 * recursively get all instmixins of a class into an initialized
 * object ptr hashtable (TCL_ONE_WORD_KEYS)
 */

static int
getAllClassMixins(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclClass *startCl,
		  int withGuards, char *pattern, XOTclObject *matchObject) {
  int rc = 0, new = 0;
  XOTclClass *cl;
  XOTclClasses *sc;

  /*
   * check this class for instmixins
   */
  if (startCl->opt) {
    XOTclCmdList *m;

    for (m = startCl->opt->instmixins; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == NULL);

      cl = XOTclGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);

      /* fprintf(stderr,"Instmixin found: %s\n", className(cl)); */

      if ((withGuards) && (m->clientData)) {
        /* fprintf(stderr,"addToResultSetWithGuards: %s\n", className(cl)); */
        rc = addToResultSetWithGuards(interp, destTable, cl, m->clientData, &new, 1, pattern, matchObject);
      } else {
        /* fprintf(stderr,"addToResultSet: %s\n", className(cl)); */
	rc = addToResultSet(interp, destTable, &cl->object, &new, 1, pattern, matchObject);
      }
      if (rc == 1) {return rc;}

      if (new) {
        /* fprintf(stderr,"Instmixin getAllClassMixins for: %s (%s)\n",className(cl),ObjStr(startCl->object.cmdName)); */
        rc = getAllClassMixins(interp, destTable, cl, withGuards, pattern, matchObject);
        if (rc) {return rc;}
      }
    }
  }


  /*
   * check all superclasses of startCl for instmixins
   */
  for (sc = startCl->super; sc; sc = sc->nextPtr) {
    /* fprintf(stderr,"Superclass getAllClassMixins for %s (%s)\n",ObjStr(sc->cl->object.cmdName),ObjStr(startCl->object.cmdName)); */
    rc = getAllClassMixins(interp, destTable, sc->cl, withGuards, pattern, matchObject);
    if (rc) {return rc;}
  }
  return rc;
}


static void
RemoveFromClassMixinsOf(Tcl_Command cmd, XOTclCmdList *cmdlist) {

  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    XOTclClass *ncl = XOTclGetClassFromCmdPtr(cmdlist->cmdPtr);
    XOTclClassOpt *nclopt = ncl ? ncl->opt : NULL;
    if (nclopt) {
      XOTclCmdList *del = CmdListFindCmdInList(cmd, nclopt->isClassMixinOf);
      if (del) {
        /* fprintf(stderr,"Removing class %s from isClassMixinOf of class %s\n",
           className(cl), ObjStr(XOTclGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
        del = CmdListRemoveFromList(&nclopt->isClassMixinOf, del);
        CmdListDeleteCmdListEntry(del, GuardDel);
      }
    }
  }
}

static void
removeFromObjectMixinsOf(Tcl_Command cmd, XOTclCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    XOTclClass *cl = XOTclGetClassFromCmdPtr(cmdlist->cmdPtr);
    XOTclClassOpt *clopt = cl ? cl->opt : NULL;
    if (clopt) {
      XOTclCmdList *del = CmdListFindCmdInList(cmd, clopt->isObjectMixinOf);
      if (del) {
        /* fprintf(stderr,"Removing object %s from isObjectMixinOf of Class %s\n",
           objectName(obj), ObjStr(XOTclGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
        del = CmdListRemoveFromList(&clopt->isObjectMixinOf, del);
        CmdListDeleteCmdListEntry(del, GuardDel);
      }
    } /* else fprintf(stderr,"CleanupDestroyObject %s: NULL pointer in mixins!\n", objectName(obj)); */
  }
}

static void
RemoveFromInstmixins(Tcl_Command cmd, XOTclCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    XOTclClass *cl = XOTclGetClassFromCmdPtr(cmdlist->cmdPtr);
    XOTclClassOpt *clopt = cl ? cl->opt : NULL;
    if (clopt) {
      XOTclCmdList *del = CmdListFindCmdInList(cmd, clopt->instmixins);
      if (del) {
        /* fprintf(stderr,"Removing class %s from mixins of object %s\n",
           className(cl), ObjStr(XOTclGetObjectFromCmdPtr(cmdlist->cmdPtr)->cmdName)); */
        del = CmdListRemoveFromList(&clopt->instmixins, del);
        CmdListDeleteCmdListEntry(del, GuardDel);
	if (cl->object.mixinOrder) MixinResetOrder(&cl->object);
      }
    }
  }
}

static void
RemoveFromMixins(Tcl_Command cmd, XOTclCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    XOTclObject *nobj = XOTclGetObjectFromCmdPtr(cmdlist->cmdPtr);
    XOTclObjectOpt *objopt = nobj ? nobj->opt : NULL;
    if (objopt) {
      XOTclCmdList *del = CmdListFindCmdInList(cmd, objopt->mixins);
      if (del) {
        /* fprintf(stderr,"Removing class %s from mixins of object %s\n",
           className(cl), ObjStr(XOTclGetObjectFromCmdPtr(cmdlist->cmdPtr)->cmdName)); */
        del = CmdListRemoveFromList(&objopt->mixins, del);
        CmdListDeleteCmdListEntry(del, GuardDel);
	if (nobj->mixinOrder) MixinResetOrder(nobj);
      }
    }
  }
}


/*
 * Reset mixin order for instances of a class
 */

static void
MixinResetOrderForInstances(Tcl_Interp *interp, XOTclClass *cl) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  hPtr = Tcl_FirstHashEntry(&cl->instances, &hSrch);

  /*fprintf(stderr,"invalidating instances of class %s\n",
    ObjStr(clPtr->cl->object.cmdName));*/

  /* here we should check, whether this class is used as
     a mixin / instmixin somewhere else and invalidate
     the objects of these as well -- */

  for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    XOTclObject *obj = (XOTclObject *)Tcl_GetHashKey(&cl->instances, hPtr);
    if (obj
        && !(obj->flags & XOTCL_DURING_DELETE)
        && (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID)) {
      MixinResetOrder(obj);
      obj->flags &= ~XOTCL_MIXIN_ORDER_VALID;
    }
  }
}

/* reset mixin order for all objects having this class as per object mixin */
static void
ResetOrderOfClassesUsedAsMixins(XOTclClass *cl) {
  /*fprintf(stderr,"ResetOrderOfClassesUsedAsMixins %s - %p\n",
    className(cl), cl->opt);*/

  if (cl->opt) {
    XOTclCmdList *ml;
    for (ml = cl->opt->isObjectMixinOf; ml; ml = ml->nextPtr) {
      XOTclObject *obj = XOTclGetObjectFromCmdPtr(ml->cmdPtr);
      if (obj) {
        if (obj->mixinOrder) { MixinResetOrder(obj); }
        obj->flags &= ~XOTCL_MIXIN_ORDER_VALID;
      }
    }
  }
}



/*
 * if the class hierarchy or class mixins have changed ->
 * invalidate mixin entries in all dependent instances
 */
static void
MixinInvalidateObjOrders(Tcl_Interp *interp, XOTclClass *cl) {
  XOTclClasses *saved = cl->order, *clPtr;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  Tcl_HashTable objTable, *commandTable = &objTable;

  cl->order = NULL;
  /*
  fprintf(stderr, "MixinInvalidateObjOrders %s calls ifd invalidate\n",className(cl));
  XOTclCInvalidateInterfaceDefinitionMethod(interp, cl); TODO REMOVEMEIFYOUARESURE
  */

  /* reset mixin order for all instances of the class and the
     instances of its subclasses
  */
  for (clPtr = ComputeOrder(cl, cl->order, Sub); clPtr; clPtr = clPtr->nextPtr) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
      Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : NULL;
    /*
    fprintf(stderr, "MixinInvalidateObjOrders subclass %s calls ifd invalidate \n",className(clPtr->cl));
    XOTclCInvalidateInterfaceDefinitionMethod(interp, clPtr->cl); TODO REMOVEMEIFYOUARESURE
    */
    /* reset mixin order for all objects having this class as per object mixin */
    ResetOrderOfClassesUsedAsMixins(clPtr->cl);

    /* fprintf(stderr,"invalidating instances of class %s\n", ObjStr(clPtr->cl->object.cmdName));
     */

    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      XOTclObject *obj = (XOTclObject *)Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      if (obj->mixinOrder) { MixinResetOrder(obj); }
      obj->flags &= ~XOTCL_MIXIN_ORDER_VALID;
    }
  }

  XOTclClassListFree(cl->order);
  cl->order = saved;

  /* Reset mixin order for all objects having this class as a per
     class mixin (instmixin).  This means that we have to work through
     the instmixin hierarchy with its corresponding instances.
  */
  Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
  getAllClassMixinsOf(interp, commandTable, cl, 1, 0, NULL, NULL);

  for (hPtr = Tcl_FirstHashEntry(commandTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    XOTclClass *ncl = (XOTclClass *)Tcl_GetHashKey(commandTable, hPtr);
    /*fprintf(stderr,"Got %s, reset for ncl %p\n",ncl?ObjStr(ncl->object.cmdName):"NULL",ncl);*/
    if (ncl) {
      MixinResetOrderForInstances(interp, ncl);
      fprintf(stderr, "MixinInvalidateObjOrders via instmixin %s calls ifd invalidate \n",className(ncl));
      XOTclCInvalidateInterfaceDefinitionMethod(interp, ncl);
    }
  }
  MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
}


static int MixinInfo(Tcl_Interp *interp, XOTclCmdList *m, char *pattern,
                     int withGuards, XOTclObject *matchObject);
/*
 * the mixin order is either
 *   DEFINED (there are mixins on the instance),
 *   NONE    (there are no mixins for the instance),
 *   or INVALID (a class re-strucuturing has occured, thus it is not clear
 *               whether mixins are defined or not).
 * If it is INVALID MixinComputeDefined can be used to compute the order
 * and set the instance to DEFINED or NONE
 */
static void
MixinComputeDefined(Tcl_Interp *interp, XOTclObject *obj) {
  MixinComputeOrder(interp, obj);
  obj->flags |= XOTCL_MIXIN_ORDER_VALID;
  if (obj->mixinOrder)
    obj->flags |= XOTCL_MIXIN_ORDER_DEFINED;
  else
    obj->flags &= ~XOTCL_MIXIN_ORDER_DEFINED;
}

/*
 * Walk through the command list until the current command is reached.
 * return the next entry.
 *
 */
static XOTclCmdList *
seekCurrent(Tcl_Command currentCmd, register XOTclCmdList *cmdl) {
  if (currentCmd) {
    /* go forward to current class */
    for (; cmdl; cmdl = cmdl->nextPtr) {
      if (cmdl->cmdPtr == currentCmd) {
        return cmdl->nextPtr;
      }
    }
  }
  return cmdl;
}

/*
 * before we can perform a mixin dispatch, MixinSearchProc seeks the
 * current mixin and the relevant calling information
 */
static Tcl_Command
MixinSearchProc(Tcl_Interp *interp, XOTclObject *obj, char *methodName,
                XOTclClass **cl, Tcl_Command *currentCmdPtr) {
  Tcl_Command cmd = NULL;
  XOTclCmdList *cmdList;
  XOTclClass *cls;

  assert(obj);
  assert(obj->mixinStack);

  /* ensure that the mixin order is not invalid, otherwise compute order */
  assert(obj->flags & XOTCL_MIXIN_ORDER_VALID);
  /*MixinComputeDefined(interp, obj);*/
  cmdList = seekCurrent(obj->mixinStack->currentCmdPtr, obj->mixinOrder);

#if defined(ACTIVEMIXIN)
  RUNTIME_STATE(interp)->cmdPtr = cmdList->cmdPtr;
#endif

  /*
    fprintf(stderr, "MixinSearch searching for '%s' %p\n", methodName, cmdList);
  */
  /*CmdListPrint(interp,"MixinSearch CL = \n", cmdList);*/

  while (cmdList) {
    if (Tcl_Command_cmdEpoch(cmdList->cmdPtr)) {
      cmdList = cmdList->nextPtr;
    } else {
      cls = XOTclGetClassFromCmdPtr(cmdList->cmdPtr);
      /*
        fprintf(stderr,"+++ MixinSearch %s->%s in %p cmdPtr %p clientData %p\n",
	objectName(obj), methodName, cmdList,
	cmdList->cmdPtr, cmdList->clientData);
      */
      if (cls) {
        int guardOk = TCL_OK;
        cmd = FindMethod(methodName, cls->nsPtr);
        if (cmd && cmdList->clientData) {
          if (!RUNTIME_STATE(interp)->guardCount) {
            guardOk = GuardCall(obj, cls, (Tcl_Command) cmd, interp,
                                (Tcl_Obj*)cmdList->clientData, NULL);
          }
        }
        if (cmd && guardOk == TCL_OK) {
          /*
           * on success: compute mixin call data
           */
          *cl = cls;
          *currentCmdPtr = cmdList->cmdPtr;
          break;
	} else {
          cmd = NULL;
          cmdList = cmdList->nextPtr;
        }
      }
    }
  }

  return cmd;
}

/*
 * info option for mixins and instmixins
 */
static int
MixinInfo(Tcl_Interp *interp, XOTclCmdList *m, char *pattern,
          int withGuards, XOTclObject *matchObject) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  XOTclClass *mixinClass;

  while (m) {
    /* fprintf(stderr,"   mixin info m=%p, next=%p, pattern %s, matchObject %p\n",
       m, m->next, pattern, matchObject);*/
    mixinClass = XOTclGetClassFromCmdPtr(m->cmdPtr);
    if (mixinClass &&
        (!pattern
         || (matchObject && &(mixinClass->object) == matchObject)
         || (!matchObject && Tcl_StringMatch(ObjStr(mixinClass->object.cmdName), pattern)))) {
      if (withGuards && m->clientData) {
        Tcl_Obj *l = Tcl_NewListObj(0, NULL);
        Tcl_Obj *g = (Tcl_Obj*) m->clientData;
        Tcl_ListObjAppendElement(interp, l, mixinClass->object.cmdName);
        Tcl_ListObjAppendElement(interp, l, XOTclGlobalObjects[XOTE_GUARD_OPTION]);
        Tcl_ListObjAppendElement(interp, l, g);
        Tcl_ListObjAppendElement(interp, list, l);
      } else {
        Tcl_ListObjAppendElement(interp, list, mixinClass->object.cmdName);
      }
      if (matchObject) break;
    }
    m = m->nextPtr;
  }
  Tcl_SetObjResult(interp, list);
  return TCL_OK;
}

/*
 * info option for mixinofs and isClassMixinOf
 */

static Tcl_Command
MixinSearchMethodByName(Tcl_Interp *interp, XOTclCmdList *mixinList, char *name, XOTclClass **cl) {
  Tcl_Command cmd;

  for (; mixinList;  mixinList = mixinList->nextPtr) {
    XOTclClass *foundCl =
      XOTclpGetClass(interp, (char *) Tcl_GetCommandName(interp, mixinList->cmdPtr));
    if (foundCl && SearchCMethod(foundCl, name, &cmd)) {
      if (cl) *cl = foundCl;
      return cmd;
    }
  }
  return 0;
}


/*
 *  Filter-Commands
 */

/*
 * The search method implements filter search order for filter
 * and instfilter: first a given name is interpreted as fully
 * qualified instproc name. If no instproc is found, a proc is
 * search with fully name. Otherwise the simple name is searched
 * on the heritage order: object (only for
 * per-object filters), class, meta-class
 */

static Tcl_Command
FilterSearch(Tcl_Interp *interp, char *name, XOTclObject *startingObj,
             XOTclClass *startingCl, XOTclClass **cl) {
  Tcl_Command cmd = NULL;

  if (startingObj) {
    XOTclObjectOpt *opt = startingObj->opt;
    /*
     * the object-specific filter can also be defined on the object's
     * class, its hierarchy, or the respective instmixins; thus use the
     * object's class as start point for the class-specific search then ...
     */
    startingCl = startingObj->cl;

    /*
     * search for filters on object mixins
     */
    if (opt && opt->mixins) {
      if ((cmd = MixinSearchMethodByName(interp, opt->mixins, name, cl))) {
        return cmd;
      }
    }
  }

  /*
   * search for instfilters on instmixins
   */
  if (startingCl) {
    XOTclClassOpt *opt = startingCl->opt;
    if (opt && opt->instmixins) {
      if ((cmd = MixinSearchMethodByName(interp, opt->instmixins, name, cl))) {
        return cmd;
      }
    }
  }

  /*
   * seach for object procs that are used as filters
   */
  if (startingObj && startingObj->nsPtr) {
    /*fprintf(stderr,"search filter %s as proc \n",name);*/
    if ((cmd = FindMethod(name, startingObj->nsPtr))) {
      *cl = (XOTclClass*)startingObj;
      return cmd;
    }
  }

  /*
   * ok, no filter on obj or mixins -> search class
   */
  if (startingCl) {
    *cl = SearchCMethod(startingCl, name, &cmd);
    if (!*cl) {
      /*
       * If no filter is found yet -> search the meta-class
       */
      *cl = SearchCMethod(startingCl->object.cl, name, &cmd);
    }
  }
  return cmd;
}

/*
 * Filter Guards
 */

/* check a filter guard, return 1 if ok */
static int
GuardCheck(Tcl_Interp *interp, Tcl_Obj *guard) {
  int rc;
  XOTclRuntimeState *rst = RUNTIME_STATE(interp);

  if (guard) {
    /*
     * if there are more than one filter guard for this filter
     * (i.e. they are inherited), then they are OR combined
     * -> if one check succeeds => return 1
     */

    /*fprintf(stderr, "checking guard **%s**\n", ObjStr(guard));*/

    rst->guardCount++;
    rc = checkConditionInScope(interp, guard);
    rst->guardCount--;

    /*fprintf(stderr, "checking guard **%s** returned rc=%d\n",  ObjStr(guard), rc);*/

    if (rc == TCL_OK) {
      /* fprintf(stderr, " +++ OK\n"); */
      return TCL_OK;
    } else if (rc == TCL_ERROR) {
      Tcl_Obj *sr = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(sr);

      /* fprintf(stderr, " +++ ERROR\n");*/

      XOTclVarErrMsg(interp, "Guard Error: '", ObjStr(guard), "'\n\n",
                     ObjStr(sr), (char *) NULL);
      DECR_REF_COUNT(sr);
      return TCL_ERROR;
    }
  }
  /*
    fprintf(stderr, " +++ FAILED\n");
  */
  return XOTCL_CHECK_FAILED;
}

/*
  static void
  GuardPrint(Tcl_Interp *interp, ClientData clientData) {
  Tcl_Obj *guard = (TclObj*) clientData;
  fprintf(stderr, " +++ <GUARDS> \n");
  if (guard) {
  fprintf(stderr, "   *     %s \n", ObjStr(guard));
  }
  fprintf(stderr, " +++ </GUARDS>\n");
  }
*/

static void
GuardDel(XOTclCmdList *CL) {
  /*fprintf(stderr, "GuardDel %p clientData = %p\n",
    CL, CL? CL->clientData : NULL);*/
  if (CL && CL->clientData) {
    DECR_REF_COUNT((Tcl_Obj *)CL->clientData);
    CL->clientData = NULL;
  }
}

XOTCLINLINE static void
GuardAdd(Tcl_Interp *interp, XOTclCmdList *CL, Tcl_Obj *guard) {
  if (guard) {
    GuardDel(CL);
    if (strlen(ObjStr(guard)) != 0) {
      INCR_REF_COUNT(guard);
      CL->clientData = (ClientData) guard;
      /*fprintf(stderr,"guard added to %p cmdPtr=%p, clientData= %p\n",
        CL, CL->cmdPtr, CL->clientData);
      */
    }
  }
}
/*
  static void
  GuardAddList(Tcl_Interp *interp, XOTclCmdList *dest, ClientData source) {
  XOTclTclObjList *s = (XOTclTclObjList*) source;
  GuardAdd(interp, dest, (Tcl_Obj*) s->content);
  s = s->nextPtr;
  } */

static int
GuardCall(XOTclObject *obj, XOTclClass *cl, Tcl_Command cmd,
          Tcl_Interp *interp, Tcl_Obj *guard, XOTclCallStackContent *csc) {
  int rc = TCL_OK;

  if (guard) {
    Tcl_Obj *res = Tcl_GetObjResult(interp); /* save the result */
    XOTcl_FrameDecls;
    INCR_REF_COUNT(res);

    /* GuardPrint(interp, cmdList->clientData); */
    /*
     * For the guard push a fake callframe on the Tcl stack so that
     * e.g. a "self calledproc" and other methods in the guard behave
     * like in the proc.
     */
#if defined(TCL85STACK)
    if (csc) {
      XOTcl_PushFrameCsc(interp, obj, csc);
    } else {
      XOTcl_PushFrame(interp, obj);
    }
#else
    CallStackPush(interp, obj, cl, cmd, XOTCL_CSC_TYPE_GUARD);
    XOTcl_PushFrame(interp, obj);
#endif
    rc = GuardCheck(interp, guard);
    XOTcl_PopFrame(interp, obj);
#if defined(TCL85STACK)
#else
    CallStackPop(interp, NULL);
#endif

    Tcl_SetObjResult(interp, res);  /* restore the result */
    DECR_REF_COUNT(res);
  }

  return rc;
}

static int
GuardAddFromDefinitionList(Tcl_Interp *interp, XOTclCmdList *dest,
                           XOTclObject *obj, Tcl_Command interceptorCmd,
                           XOTclCmdList *interceptorDefList) {
  XOTclCmdList *h;
  if (interceptorDefList) {
    h = CmdListFindCmdInList(interceptorCmd, interceptorDefList);
    if (h) {
      GuardAdd(interp, dest, (Tcl_Obj*) h->clientData);
      /*
       * 1 means we have added a guard successfully "interceptorCmd"
       */
      return 1;
    }
  }
  /*
   * 0 means we have not added a guard successfully "interceptorCmd"
   */
  return 0;
}

static void
GuardAddInheritedGuards(Tcl_Interp *interp, XOTclCmdList *dest,
                        XOTclObject *obj, Tcl_Command filterCmd) {
  XOTclClasses *pl;
  int guardAdded = 0;
  XOTclObjectOpt *opt;

  /* search guards for instfilters registered on mixins */
  if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, obj);
  if (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
    XOTclCmdList *ml;
    XOTclClass *mixin;
    for (ml = obj->mixinOrder; ml && !guardAdded; ml = ml->nextPtr) {
      mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin && mixin->opt) {
        guardAdded = GuardAddFromDefinitionList(interp, dest, obj, filterCmd,
                                                mixin->opt->instfilters);
      }
    }
  }

  /* search per-object filters */
  opt = obj->opt;
  if (!guardAdded && opt && opt->filters) {
    guardAdded = GuardAddFromDefinitionList(interp, dest, obj, filterCmd, opt->filters);
  }

  if (!guardAdded) {
    /* search per-class filters */
    for (pl = ComputeOrder(obj->cl, obj->cl->order, Super); !guardAdded && pl; pl = pl->nextPtr) {
      XOTclClassOpt *opt = pl->cl->opt;
      if (opt) {
        guardAdded = GuardAddFromDefinitionList(interp, dest, obj, filterCmd,
                                                opt->instfilters);
      }
    }


    /*
     * if this is not a registered filter, it is an inherited filter, like:
     *   Class A
     *   A instproc f ...
     *   Class B -superclass A
     *   B instproc {{f {<guard>}}}
     *   B instfilter f
     * -> get the guard from the filter that inherits it (here B->f)
     */
    if (!guardAdded) {
      XOTclCmdList *registeredFilter =
        CmdListFindNameInList(interp,(char *) Tcl_GetCommandName(interp, filterCmd),
                              obj->filterOrder);
      if (registeredFilter) {
        GuardAdd(interp, dest, (Tcl_Obj*) registeredFilter->clientData);
      }
    }
  }
}

static int
GuardList(Tcl_Interp *interp, XOTclCmdList *frl, char *interceptorName) {
  XOTclCmdList *h;
  if (frl) {
    /* try to find simple name first */
    h = CmdListFindNameInList(interp, interceptorName, frl);
    if (!h) {
      /* maybe it is a qualified name */
      Tcl_Command cmd = NSFindCommand(interp, interceptorName, NULL);
      if (cmd) {
        h = CmdListFindCmdInList(cmd, frl);
      }
    }
    if (h) {
      Tcl_ResetResult(interp);
      if (h->clientData) {
        Tcl_Obj *g = (Tcl_Obj*) h->clientData;
        Tcl_SetObjResult(interp, g);
      }
      return TCL_OK;
    }
  }
  return XOTclVarErrMsg(interp, "info (*)guard: can't find filter/mixin ",
                        interceptorName, (char *) NULL);
}

/*
 * append a filter command to the 'filterList' of an obj/class
 */
static int
FilterAdd(Tcl_Interp *interp, XOTclCmdList **filterList, Tcl_Obj *name,
          XOTclObject *startingObj, XOTclClass *startingCl) {
  Tcl_Command cmd;
  int ocName; Tcl_Obj **ovName;
  Tcl_Obj *guard = NULL;
  XOTclCmdList *new;
  XOTclClass *cl;

  if (Tcl_ListObjGetElements(interp, name, &ocName, &ovName) == TCL_OK && ocName > 1) {
    if (ocName == 3 && !strcmp(ObjStr(ovName[1]), XOTclGlobalStrings[XOTE_GUARD_OPTION])) {
      name = ovName[0];
      guard = ovName[2];
    }
  }

  if (!(cmd = FilterSearch(interp, ObjStr(name), startingObj, startingCl, &cl))) {
    if (startingObj)
      return XOTclVarErrMsg(interp, "filter: can't find filterproc on: ",
                            objectName(startingObj), " - proc: ",
                            ObjStr(name), (char *) NULL);
    else
      return XOTclVarErrMsg(interp, "instfilter: can't find filterproc on: ",
                            ObjStr(startingCl->object.cmdName), " - proc: ",
                            ObjStr(name), (char *) NULL);
  }

  /*fprintf(stderr, " +++ adding filter %s cl %p\n", ObjStr(name), cl);*/

  new = CmdListAdd(filterList, cmd, cl, /*noDuplicates*/ 1);

  if (guard) {
    GuardAdd(interp, new, guard);
  } else {
    if (new->clientData)
      GuardDel(new);
  }

  return TCL_OK;
}

/*
 * reset the filter order cached in obj->filterOrder
 */
static void
FilterResetOrder(XOTclObject *obj) {
  CmdListRemoveList(&obj->filterOrder, GuardDel);
  obj->filterOrder = NULL;
}

/*
 * search the filter in the hierarchy again with FilterSearch, e.g.
 * upon changes in the class hierarchy or mixins that carry the filter
 * command, so that we can be sure it is still reachable.
 */
static void
FilterSearchAgain(Tcl_Interp *interp, XOTclCmdList **filters,
                  XOTclObject *startingObj, XOTclClass *startingCl) {
  char *simpleName;
  Tcl_Command cmd;
  XOTclCmdList *cmdList, *del;
  XOTclClass *cl = NULL;

  CmdListRemoveEpoched(filters, GuardDel);
  for (cmdList = *filters; cmdList; ) {
    simpleName = (char *) Tcl_GetCommandName(interp, cmdList->cmdPtr);
    cmd = FilterSearch(interp, simpleName, startingObj, startingCl, &cl);
    if (cmd == NULL) {
      del = CmdListRemoveFromList(filters, cmdList);
      cmdList = cmdList->nextPtr;
      CmdListDeleteCmdListEntry(del, GuardDel);
    } else if (cmd != cmdList->cmdPtr) {
      CmdListReplaceCmd(cmdList, cmd, cl);
      cmdList = cmdList->nextPtr;
    } else {
      cmdList = cmdList->nextPtr;
    }
  }

  /* some entries might be NULL now, if they are not found anymore
     -> delete those
     CmdListRemoveNulledEntries(filters, GuardDel);
  */
}

/*
 * if the class hierarchy or class filters have changed ->
 * invalidate filter entries in all dependent instances
 *
 */
static void
FilterInvalidateObjOrders(Tcl_Interp *interp, XOTclClass *cl) {
  XOTclClasses *saved = cl->order, *clPtr, *savePtr;

  cl->order = NULL;
  savePtr = clPtr = ComputeOrder(cl, cl->order, Sub);
  cl->order = saved;

  for ( ; clPtr; clPtr = clPtr->nextPtr) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
      Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : 0;

    /* recalculate the commands of all instfilter registrations */
    if (clPtr->cl->opt) {
      FilterSearchAgain(interp, &clPtr->cl->opt->instfilters, 0, clPtr->cl);
    }
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      XOTclObject *obj = (XOTclObject *)Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      FilterResetOrder(obj);
      obj->flags &= ~XOTCL_FILTER_ORDER_VALID;

      /* recalculate the commands of all object filter registrations */
      if (obj->opt) {
        FilterSearchAgain(interp, &obj->opt->filters, obj, 0);
      }
    }
  }
  XOTclClassListFree(savePtr);
}

/*
 * from cl on down the hierarchy we remove all filters
 * the refer to "removeClass" namespace. E.g. used to
 * remove filters defined in superclass list from dependent
 * class cl
 */
static void
FilterRemoveDependentFilterCmds(XOTclClass *cl, XOTclClass *removeClass) {
  XOTclClasses *saved = cl->order, *clPtr;
  cl->order = NULL;

  /*fprintf(stderr, "FilterRemoveDependentFilterCmds cl %p %s, removeClass %p %s\n",
    cl, className(cl),
    removeClass, ObjStr(removeClass->object.cmdName));*/

  for (clPtr = ComputeOrder(cl, cl->order, Sub); clPtr; clPtr = clPtr->nextPtr) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
      Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : NULL;
    XOTclClassOpt *opt = clPtr->cl->opt;
    if (opt) {
      CmdListRemoveContextClassFromList(&opt->instfilters, removeClass, GuardDel);
    }
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      XOTclObject *obj = (XOTclObject*)	Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      if (obj->opt) {
        CmdListRemoveContextClassFromList(&obj->opt->filters, removeClass, GuardDel);
      }
    }
  }

  XOTclClassListFree(cl->order);
  cl->order = saved;
}

/*
 * build up a qualifier of the form <obj/cl> proc/instproc <procName>
 * if cl is not NULL, we build an instproc identifier for cl, else a proc
 * with obj
 */
static Tcl_Obj *
getFullProcQualifier(Tcl_Interp *interp, CONST char *cmdName,
                     XOTclObject *obj, XOTclClass *cl, Tcl_Command cmd) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  Tcl_Obj *procObj = Tcl_NewStringObj(cmdName, -1);
  Tcl_ObjCmdProc *objProc = Tcl_Command_objProc(cmd);
  int isTcl = (TclIsProc((Command *)cmd) != NULL);

  if (cl) {
    Tcl_ListObjAppendElement(interp, list, cl->object.cmdName);
    /*fprintf(stderr,"current %p, dispatch %p, forward %p, parametermcd %p, is tcl %p\n",
      objProc, XOTclObjDispatch, XOTclForwardMethod,
      XOTclSetterMethod, TclIsProc((Command *)cmd)); */
    if (isTcl) {
      Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_INSTPROC]);
    } else if (objProc == XOTclForwardMethod) {
      Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_INSTFORWARD]);
    } else if (objProc == XOTclSetterMethod) {
      Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_INSTPARAMETERCMD]);
    } else {
      Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_INSTCMD]);
    }
  } else {
    Tcl_ListObjAppendElement(interp, list, obj->cmdName);
    if (isTcl) {
      Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_PROC]);
    } else if (objProc == XOTclForwardMethod) {
      Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_FORWARD]);
    } else if (objProc == XOTclSetterMethod) {
      Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_PARAMETERCMD]);
    } else {
      Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_CMD]);
    }
  }
  Tcl_ListObjAppendElement(interp, list, procObj);
  return list;
}

/*
 * info option for filters and instfilters
 * withGuards -> if not 0 => append guards
 * fullProcQualifiers -> if not 0 => full names with obj/class proc/instproc
 */
static int
FilterInfo(Tcl_Interp *interp, XOTclCmdList *f, char *pattern,
           int withGuards, int fullProcQualifiers) {
  CONST char *simpleName;
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);

  /*fprintf(stderr,"FilterInfo %p %s %d %d\n", pattern, pattern, withGuards, fullProcQualifiers);*/

  /* guard lists should only have unqualified filter lists
     when withGuards is activated, fullProcQualifiers has not
     effect */
  if (withGuards) {
    fullProcQualifiers = 0;
  }

  while (f) {
    simpleName = Tcl_GetCommandName(interp, f->cmdPtr);
    if (!pattern || Tcl_StringMatch(simpleName, pattern)) {
      if (withGuards && f->clientData) {
        Tcl_Obj *innerList = Tcl_NewListObj(0, NULL);
        Tcl_Obj *g = (Tcl_Obj*) f->clientData;
        Tcl_ListObjAppendElement(interp, innerList,
                                 Tcl_NewStringObj(simpleName, -1));
        Tcl_ListObjAppendElement(interp, innerList, XOTclGlobalObjects[XOTE_GUARD_OPTION]);
        Tcl_ListObjAppendElement(interp, innerList, g);
        Tcl_ListObjAppendElement(interp, list, innerList);
      } else {
        if (fullProcQualifiers) {
	  XOTclClass *fcl;
          XOTclObject *fobj;
          if (f->clorobj && !XOTclObjectIsClass(&f->clorobj->object)) {
            fobj = (XOTclObject *)f->clorobj;
            fcl = NULL;
          } else {
            fobj = NULL;
            fcl = f->clorobj;
          }
          Tcl_ListObjAppendElement(interp, list,
                                   getFullProcQualifier(interp, simpleName,
                                                        fobj, fcl, f->cmdPtr));
        } else {
          Tcl_ListObjAppendElement(interp, list, Tcl_NewStringObj(simpleName, -1));
        }
      }
    }
    f = f->nextPtr;
  }
  Tcl_SetObjResult(interp, list);
  return TCL_OK;
}

/*
 * Appends XOTclCmdPtr *containing the filter cmds and their
 * superclass specializations to 'filterList'
 */
static void
FilterComputeOrderFullList(Tcl_Interp *interp, XOTclCmdList **filters,
                           XOTclCmdList **filterList) {
  XOTclCmdList *f ;
  char *simpleName;
  XOTclClass *fcl;
  XOTclClasses *pl;

  /*
   * ensure that no epoched command is in the filters list
   */
  CmdListRemoveEpoched(filters, GuardDel);

  for (f = *filters; f; f = f->nextPtr) {
    simpleName = (char *) Tcl_GetCommandName(interp, f->cmdPtr);
    fcl = f->clorobj;
    CmdListAdd(filterList, f->cmdPtr, fcl, /*noDuplicates*/ 0);

    if (fcl && !XOTclObjectIsClass(&fcl->object)) {
      /* get the object for per-object filter */
      XOTclObject *fObj = (XOTclObject *)fcl;
      /* and then get class */
      fcl = fObj->cl;
    }

    /* if we have a filter class -> search up the inheritance hierarchy*/
    if (fcl) {
      pl = ComputeOrder(fcl, fcl->order, Super);
      if (pl && pl->nextPtr) {
        /* don't search on the start class again */
        pl = pl->nextPtr;
        /* now go up the hierarchy */
        for(; pl; pl = pl->nextPtr) {
          Tcl_Command pi = FindMethod(simpleName, pl->cl->nsPtr);
          if (pi) {
            CmdListAdd(filterList, pi, pl->cl, /*noDuplicates*/ 0);
            /*
              fprintf(stderr, " %s::%s, ", ObjStr(pl->cl->object.cmdName), simpleName);
            */
          }
        }
      }
    }
  }
  /*CmdListPrint(interp,"FilterComputeOrderFullList....\n", *filterList);*/
}

/*
 * Computes a linearized order of filter and instfilter. Then
 * duplicates in the full list and with the class inheritance list of
 * 'obj' are eliminated.
 * The precendence rule is that the last occurence makes it into the
 * final list.
 */
static void
FilterComputeOrder(Tcl_Interp *interp, XOTclObject *obj) {
  XOTclCmdList *filterList = NULL, *next, *checker, *newlist;
  XOTclClasses *pl;

  if (obj->filterOrder) FilterResetOrder(obj);
  /*
    fprintf(stderr, "<Filter Order obj=%s> List: ", objectName(obj));
  */

  /* append instfilters registered for mixins */
  if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, obj);

  if (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
    XOTclCmdList *ml;
    XOTclClass *mixin;

    for (ml = obj->mixinOrder; ml; ml = ml->nextPtr) {
      mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin && mixin->opt && mixin->opt->instfilters)
        FilterComputeOrderFullList(interp, &mixin->opt->instfilters, &filterList);
    }
  }

  /* append per-obj filters */
  if (obj->opt)
    FilterComputeOrderFullList(interp, &obj->opt->filters, &filterList);

  /* append per-class filters */
  for (pl = ComputeOrder(obj->cl, obj->cl->order, Super); pl; pl=pl->nextPtr) {
    XOTclClassOpt *opt = pl->cl->opt;
    if (opt && opt->instfilters) {
      FilterComputeOrderFullList(interp, &opt->instfilters, &filterList);
    }
  }

  /*
    fprintf(stderr, "\n");
  */
  /* use no duplicates & no classes of the precedence order
     on the resulting list */
  while (filterList) {
    checker = next = filterList->nextPtr;
    while (checker) {
      if (checker->cmdPtr == filterList->cmdPtr) break;
      checker = checker->nextPtr;
    }
    if (checker == NULL) {
      newlist = CmdListAdd(&obj->filterOrder, filterList->cmdPtr, filterList->clorobj,
                           /*noDuplicates*/ 0);
      GuardAddInheritedGuards(interp, newlist, obj, filterList->cmdPtr);
      /*
        fprintf(stderr, "  Adding %s::%s,\n", filterList->cmdPtr->nsPtr->fullName, Tcl_GetCommandName(interp, filterList->cmdPtr));
      */
      /*
        GuardPrint(interp, newlist->clientData);
      */

    }

    CmdListDeleteCmdListEntry(filterList, GuardDel);

    filterList = next;
  }
  /*
    fprintf(stderr, "</Filter Order>\n");
  */
}

/*
 * the filter order is either
 *   DEFINED (there are filter on the instance),
 *   NONE    (there are no filter for the instance),
 *   or INVALID (a class re-strucuturing has occured, thus it is not clear
 *               whether filters are defined or not).
 * If it is INVALID FilterComputeDefined can be used to compute the order
 * and set the instance to DEFINE or NONE
 */
static void
FilterComputeDefined(Tcl_Interp *interp, XOTclObject *obj) {
  FilterComputeOrder(interp, obj);
  obj->flags |= XOTCL_FILTER_ORDER_VALID;
  if (obj->filterOrder)
    obj->flags |= XOTCL_FILTER_ORDER_DEFINED;
  else
    obj->flags &= ~XOTCL_FILTER_ORDER_DEFINED;
}

/*
 * push a filter stack information on this object
 */
static int
FilterStackPush(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *calledProc) {
  register XOTclFilterStack *h = NEW(XOTclFilterStack);

  h->currentCmdPtr = NULL;
  h->calledProc = calledProc;
  INCR_REF_COUNT(h->calledProc);
  h->nextPtr = obj->filterStack;
  obj->filterStack = h;
  return 1;
}

/*
 * pop a filter stack information on this object
 */
static void
FilterStackPop(XOTclObject *obj) {
  register XOTclFilterStack *h = obj->filterStack;
  obj->filterStack = h->nextPtr;

  /* free stack entry */
  DECR_REF_COUNT(h->calledProc);
  FREE(XOTclFilterStack, h);
}

/*
 * search through the filter list on obj and class hierarchy
 * for registration of a command ptr as filter
 *
 * returns a tcl obj list with the filter registration, like:
 * "<obj> filter <filterName>,
 * "<class> instfilter <filterName>,
 * or an empty list, if not registered
 */
static Tcl_Obj *
FilterFindReg(Tcl_Interp *interp, XOTclObject *obj, Tcl_Command cmd) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  XOTclClasses *pl;

  /* search per-object filters */
  if (obj->opt && CmdListFindCmdInList(cmd, obj->opt->filters)) {
    Tcl_ListObjAppendElement(interp, list, obj->cmdName);
    Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_FILTER]);
    Tcl_ListObjAppendElement(interp, list,
                             Tcl_NewStringObj(Tcl_GetCommandName(interp, cmd), -1));
    return list;
  }

  /* search per-class filters */
  for (pl = ComputeOrder(obj->cl, obj->cl->order, Super); pl; pl = pl->nextPtr) {
    XOTclClassOpt *opt = pl->cl->opt;
    if (opt && opt->instfilters) {
      if (CmdListFindCmdInList(cmd, opt->instfilters)) {
        Tcl_ListObjAppendElement(interp, list, pl->cl->object.cmdName);
        Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjects[XOTE_INSTFILTER]);
        Tcl_ListObjAppendElement(interp, list,
                                 Tcl_NewStringObj(Tcl_GetCommandName(interp, cmd), -1));
        return list;
      }
    }
  }
  return list;
}

/*
 * before we can perform a filter dispatch, FilterSearchProc seeks the
 * current filter and the relevant calling information
 */
static Tcl_Command
FilterSearchProc(Tcl_Interp *interp, XOTclObject *obj,
                 Tcl_Command *currentCmd, XOTclClass **cl) {
  XOTclCmdList *cmdList;

  assert(obj);
  assert(obj->filterStack);

  *currentCmd = NULL;

  /* Ensure that the filter order is not invalid, otherwise compute order
     FilterComputeDefined(interp, obj);
  */
  assert(obj->flags & XOTCL_FILTER_ORDER_VALID);
  cmdList = seekCurrent(obj->filterStack->currentCmdPtr, obj->filterOrder);

  while (cmdList) {
    if (Tcl_Command_cmdEpoch(cmdList->cmdPtr)) {
      cmdList = cmdList->nextPtr;
    } else if (FilterActiveOnObj(interp, obj, cmdList->cmdPtr)) {
      /* fprintf(stderr, "Filter <%s> -- Active on: %s\n",
         Tcl_GetCommandName(interp, (Tcl_Command)cmdList->cmdPtr), objectName(obj));
      */
      obj->filterStack->currentCmdPtr = cmdList->cmdPtr;
      cmdList = seekCurrent(obj->filterStack->currentCmdPtr, obj->filterOrder);
    } else {
      /* ok. we found it */
      if (cmdList->clorobj && !XOTclObjectIsClass(&cmdList->clorobj->object)) {
        *cl = NULL;
      } else {
        *cl = cmdList->clorobj;
      }
      *currentCmd = cmdList->cmdPtr;
      /* fprintf(stderr, "FilterSearchProc - found: %s, %p\n",
         Tcl_GetCommandName(interp, (Tcl_Command)cmdList->cmdPtr), cmdList->cmdPtr);
      */
      return cmdList->cmdPtr;
    }
  }
  return NULL;
}


static int
SuperclassAdd(Tcl_Interp *interp, XOTclClass *cl, int oc, Tcl_Obj **ov, Tcl_Obj *arg, XOTclClass *base) {
  XOTclClasses *filterCheck, *osl = NULL;
  XOTclClass **scl;
  int reversed = 0;
  int i, j;

  filterCheck = ComputeOrder(cl, cl->order, Super);
  /*
   * we have to remove all dependent superclass filter referenced
   * by class or one of its subclasses
   *
   * do not check the class "cl" itself (first entry in
   * filterCheck class list)
   */
  if (filterCheck)
    filterCheck = filterCheck->nextPtr;
  for (; filterCheck; filterCheck = filterCheck->nextPtr) {
    FilterRemoveDependentFilterCmds(cl, filterCheck->cl);
  }

  /* invalidate all interceptors orders of instances of this
     and of all depended classes */
  MixinInvalidateObjOrders(interp, cl);
  FilterInvalidateObjOrders(interp, cl);

  scl = NEW_ARRAY(XOTclClass*, oc);
  for (i = 0; i < oc; i++) {
    if (GetClassFromObj(interp, ov[i], &scl[i], base) != TCL_OK) {
      FREE(XOTclClass**, scl);
      return XOTclErrBadVal(interp, "superclass", "a list of classes",
                            ObjStr(arg));
    }
  }

  /*
   * check that superclasses don't precede their classes
   */

  for (i = 0; i < oc; i++) {
    if (reversed) break;
    for (j = i+1; j < oc; j++) {
      XOTclClasses *dl = ComputeOrder(scl[j], scl[j]->order, Super);
      if (reversed) break;
      while (dl) {
	if (dl->cl == scl[i]) break;
	dl = dl->nextPtr;
      }
      if (dl) reversed = 1;
    }
  }

  if (reversed) {
    return XOTclErrBadVal(interp, "superclass", "classes in dependence order",
                          ObjStr(arg));
  }

  while (cl->super) {
    /*
     * build up an old superclass list in case we need to revert
     */

    XOTclClass *sc = cl->super->cl;
    XOTclClasses *l = osl;
    osl = NEW(XOTclClasses);
    osl->cl = sc;
    osl->nextPtr = l;
    (void)RemoveSuper(cl, cl->super->cl);
  }
  for (i=0; i < oc; i++) {
    AddSuper(cl, scl[i]);
  }
  FREE(XOTclClass**, scl);
  FlushPrecedencesOnSubclasses(cl);

  if (!ComputeOrder(cl, cl->order, Super)) {

    /*
     * cycle in the superclass graph, backtrack
     */

    XOTclClasses *l;
    while (cl->super) (void)RemoveSuper(cl, cl->super->cl);
    for (l = osl; l; l = l->nextPtr) AddSuper(cl, l->cl);
    XOTclClassListFree(osl);
    return XOTclErrBadVal(interp, "superclass", "a cycle-free graph", ObjStr(arg));
  }
  XOTclClassListFree(osl);

  /* if there are no more super classes add the Object
     class as superclasses */
  if (cl->super == NULL) {
    fprintf(stderr, "SuperClassAdd super of '%s' is NULL\n", className(cl));
    /*AddSuper(cl, RUNTIME_STATE(interp)->theObject);*/
  }

  Tcl_ResetResult(interp);
  return TCL_OK;
}

extern Tcl_Obj *
XOTcl_ObjSetVar2(XOTcl_Object *obj, Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
                 Tcl_Obj *value, int flgs) {
  Tcl_Obj *result;
  XOTcl_FrameDecls;

  XOTcl_PushFrame(interp, (XOTclObject*)obj);
  if (((XOTclObject*)obj)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_ObjSetVar2(interp, name1, name2, value, flgs);
  XOTcl_PopFrame(interp, (XOTclObject*)obj);
  return result;
}

extern Tcl_Obj *
XOTcl_SetVar2Ex(XOTcl_Object *obj, Tcl_Interp *interp, CONST char *name1, CONST char *name2,
                Tcl_Obj *value, int flgs) {
  Tcl_Obj *result;
  XOTcl_FrameDecls;

  XOTcl_PushFrame(interp, (XOTclObject*)obj);
  if (((XOTclObject*)obj)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_SetVar2Ex(interp, name1, name2, value, flgs);
  XOTcl_PopFrame(interp, (XOTclObject*)obj);
  return result;
}


Tcl_Obj *
XOTclOSetInstVar(XOTcl_Object *obj, Tcl_Interp *interp,
		 Tcl_Obj *name, Tcl_Obj *value, int flgs) {
  return XOTcl_ObjSetVar2(obj, interp, name, (Tcl_Obj *)NULL, value, (flgs|TCL_PARSE_PART1));
}

extern Tcl_Obj *
XOTcl_ObjGetVar2(XOTcl_Object *obj, Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
                 int flgs) {
  Tcl_Obj *result;
  XOTcl_FrameDecls;

  XOTcl_PushFrame(interp, (XOTclObject*)obj);
  if (((XOTclObject*)obj)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_ObjGetVar2(interp, name1, name2, flgs);
  XOTcl_PopFrame(interp, (XOTclObject*)obj);

  return result;
}

extern Tcl_Obj *
XOTcl_GetVar2Ex(XOTcl_Object *obj, Tcl_Interp *interp, CONST char *name1, CONST char *name2,
                int flgs) {
  Tcl_Obj *result;
  XOTcl_FrameDecls;

  XOTcl_PushFrame(interp, (XOTclObject*)obj);
  if (((XOTclObject*)obj)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_GetVar2Ex(interp, name1, name2, flgs);
  XOTcl_PopFrame(interp, (XOTclObject*)obj);
  return result;
}


Tcl_Obj *
XOTclOGetInstVar(XOTcl_Object *obj, Tcl_Interp *interp, Tcl_Obj *name, int flgs) {
  return XOTcl_ObjGetVar2(obj, interp, name, (Tcl_Obj *)NULL, (flgs|TCL_PARSE_PART1));
}

int
XOTclUnsetInstVar(XOTcl_Object *obj, Tcl_Interp *interp, char *name, int flgs) {
  return XOTclUnsetInstVar2 (obj, interp, name,(char *)NULL, flgs);
}

static int
varExists(Tcl_Interp *interp, XOTclObject *obj, CONST char *varName, char *index,
          int triggerTrace, int requireDefined) {
  XOTcl_FrameDecls;
  Var *varPtr, *arrayPtr;
  int result;
  int flags = 0;

  flags = (index == NULL) ? TCL_PARSE_PART1 : 0;

  XOTcl_PushFrame(interp, obj);

  if (triggerTrace)
    varPtr = TclVarTraceExists(interp, varName);
  else
    varPtr = TclLookupVar(interp, varName, index, flags, "access",
                          /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
  /*
    fprintf(stderr, "varExists %s varPtr %p requireDefined %d, triggerTrace %d, isundef %d\n",
    varName,
    varPtr,
    requireDefined, triggerTrace,
    varPtr ? TclIsVarUndefined(varPtr) : 0);
  */
  result = (varPtr && (!requireDefined || !TclIsVarUndefined(varPtr)));

  XOTcl_PopFrame(interp, obj);

  return result;
}

static int
SubstValue(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj **value) {
  Tcl_Obj *ov[2];
  int rc;

  ov[1] = *value;
  Tcl_ResetResult(interp);
#if !defined(TCL85STACK)
  CallStackPush(interp, obj, NULL, 0, XOTCL_CSC_TYPE_PLAIN);
#endif
  rc = XOTcl_SubstObjCmd(NULL, interp, 2, ov);
#if !defined(TCL85STACK)
  CallStackPop(interp, NULL);
#endif
  
  /*fprintf(stderr,"+++++ %s.%s subst returned %d OK %d\n",
    objectName(obj), varName, rc, TCL_OK);*/

  if (rc == TCL_OK) {
    *value = Tcl_GetObjResult(interp);
  }
  return rc;
}

static int
evalValueIfNeeded(Tcl_Interp *interp, XOTclObject *obj, CONST char *varName, Tcl_Obj **newValue) {
  int rc = TCL_OK;
  int doSubst = 0;
  char *value = ObjStr(*newValue), *v;
  /*fprintf(stderr,"+++++ evalValueIfNeeded %s.%s got '%s''\n", objectName(obj), varName, ObjStr(*newValue));*/

  /* TODO: maybe we can do this more elegantely without the need to parse the vars */
  for (v=value; *v; v++) {
    if (*v == '[' && doSubst == 0)
      doSubst = 1;
    else if ((doSubst == 1 && *v == ']') || *v == '$') {
      doSubst = 2;
      break;
    }
  }

  if (doSubst == 2) { /* we have to subst, we overwrite newValue */
    rc = SubstValue(interp, obj, newValue);
  }
  return rc;
}

static int
setDefaultValue(Tcl_Interp *interp, XOTclObject *obj, XOTclObject *slotObj) {
  CONST char *varName  = Tcl_GetCommandName(interp, slotObj->id);
  Tcl_Obj *oldValue;
  int rc = TCL_OK;

  XOTcl_FrameDecls;
  XOTcl_PushFrame(interp, obj); /* make instvars of obj accessible */

  /*
   * caller did a XOTcl_PushFrame(interp, obj),
   * so we have the instvars already accessible;
   */
  oldValue = Tcl_GetVar2Ex(interp, varName, NULL,
			   TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);

  /* Check whether the variable is already set.
   * If yes, we do not set it again.
   */
  if (oldValue == NULL) {
    Tcl_Obj *newValue = XOTcl_GetVar2Ex((XOTcl_Object*)slotObj, interp, "default", NULL,
					TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
    /*fprintf(stderr,"+++++ %s.%s undefined'\n", objectName(obj), varName);*/
    if (newValue) {
      rc = evalValueIfNeeded(interp, obj, varName, &newValue);
      if (rc != TCL_OK) {
	goto leavesetdefaultvalue;
      }

      /*
       * just set the variable, checking is happening later
       */
      /*fprintf(stderr,"+++++ %s.%s := '%s'\n", objectName(obj), varName, ObjStr(newValue));*/

      Tcl_SetVar2Ex(interp, varName, NULL, newValue,
		    TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);

    } else {
      /*fprintf(stderr, "----- we have no new value %s\n", varName);*/
    }
    /*
     * we set the initCmd for the time being unconditionally, if it is available
     */
    {
      /* try to get initcmd
       */
      Tcl_Obj *initCmd = XOTcl_GetVar2Ex((XOTcl_Object*)slotObj, interp, "initcmd", NULL,
					 TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
      if (initCmd) {
	char *cmd = ObjStr(initCmd);
	fprintf(stderr, "----- we have an initcmd %s\n", cmd);
	if (*cmd) {
#if !defined(TCL85STACK)
	  CallStackPush(interp, obj, NULL, 0, XOTCL_CSC_TYPE_PLAIN); /*allow to call self*/
#endif

	  fprintf(stderr,"!!!! evaluating '%s'\n", cmd);
	  rc = Tcl_EvalObjEx(interp, initCmd, TCL_EVAL_DIRECT);
#if !defined(TCL85STACK)
	  CallStackPop(interp, NULL);
#endif

	  if (rc != TCL_OK) {
	    goto  leavesetdefaultvalue;
	  }
	}
      }
    }
  } else {
    /* fprintf(stderr, "+++ value for %s.%s already set\n", objectName(obj), varName);*/
  }
 leavesetdefaultvalue:
  XOTcl_PopFrame(interp, obj);
  return rc;
}

static int
checkRequiredValue(Tcl_Interp *interp, XOTclObject *obj, XOTclObject *slotObj) {
  CONST char *varName  = Tcl_GetCommandName(interp, slotObj->id);
  int rc = TCL_OK, bool;
  Tcl_Obj *requiredFlag = XOTcl_GetVar2Ex((XOTcl_Object*)slotObj, interp, "required", NULL,
					  TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
  if (requiredFlag) {
    rc = Tcl_GetBooleanFromObj(interp, requiredFlag, &bool);
    if (rc == TCL_OK && bool) {
      /*fprintf(stderr,"+++++ %s.%s must check'\n", objectName(obj), varName);*/
      if (!varExists(interp, obj, varName, NULL, 0, 1)) {
	return XOTclVarErrMsg(interp, "required parameter '", varName, "' missing",
			      (char *) NULL);
      }
    }
  }
  return rc;
}

#if !defined(PRE85)
# if defined(WITH_TCL_COMPILE)
#  include <tclCompile.h>
# endif

static void
MakeProcError(
	      Tcl_Interp *interp,		/* The interpreter in which the procedure was
						 * called. */
	      Tcl_Obj *procNameObj)	/* Name of the procedure. Used for error
					 * messages and trace information. */
{
  int overflow, limit = 60, nameLen;
  const char *procName = Tcl_GetStringFromObj(procNameObj, &nameLen);

  overflow = (nameLen > limit);
  Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
						 "\n    (procedure \"%.*s%s\" line %d)",
						 (overflow ? limit : nameLen), procName,
						 (overflow ? "..." : ""), interp->errorLine));
}

/* 
   PushProcCallFrame() compiles conditionally a proc and pushes a
   callframe. Interesting fields:

   clientData: Record describing procedure to be interpreted. 
   isLambda: 1 if this is a call by ApplyObjCmd: it needs special rules for error msg

 */

static int 
PushProcCallFrame(ClientData clientData, register Tcl_Interp *interp, int objc,	Tcl_Obj *CONST objv[], 
                  XOTclCallStackContent *csc) {
  Proc *procPtr = (Proc *) clientData;
  Tcl_Obj *bodyPtr = procPtr->bodyPtr;
  Namespace *nsPtr = procPtr->cmdPtr->nsPtr;
  CallFrame *framePtr;
  int result;
  static Tcl_ObjType *byteCodeType = NULL;

  if (byteCodeType == NULL) {
    static XOTclMutex initMutex = 0;
    XOTclMutexLock(&initMutex);
    if (byteCodeType == NULL) {
      byteCodeType = Tcl_GetObjType("bytecode");
    }
    XOTclMutexUnlock(&initMutex);
  }

  if (bodyPtr->typePtr == byteCodeType) {
# if defined(HAVE_TCL_COMPILE_H)
    ByteCode *codePtr;
    Interp *iPtr = (Interp *) interp;

    /*
     * When we've got bytecode, this is the check for validity. That is,
     * the bytecode must be for the right interpreter (no cross-leaks!),
     * the code must be from the current epoch (so subcommand compilation
     * is up-to-date), the namespace must match (so variable handling
     * is right) and the resolverEpoch must match (so that new shadowed
     * commands and/or resolver changes are considered).
     */

    codePtr = bodyPtr->internalRep.otherValuePtr;
    if (((Interp *) *codePtr->interpHandle != iPtr)
	|| (codePtr->compileEpoch != iPtr->compileEpoch)
	|| (codePtr->nsPtr != nsPtr)
	|| (codePtr->nsEpoch != nsPtr->resolverEpoch)) {

      goto doCompilation;
    }
# endif
  } else {
# if defined(HAVE_TCL_COMPILE_H)
  doCompilation:
# endif
    result = TclProcCompileProc(interp, procPtr, bodyPtr,
				(Namespace *) nsPtr, "body of proc", 
                                TclGetString(objv[0]));
    if (result != TCL_OK) {
      return result;
    }
  }
  /*
   * Set up and push a new call frame for the new procedure invocation.
   * This call frame will execute in the proc's namespace, which might be
   * different than the current namespace. The proc's namespace is that of
   * its command, which can change if the command is renamed from one
   * namespace to another.
   */

#if defined(TCL85STACK_TRACE)
  fprintf(stderr,"PUSH METHOD_FRAME (PushProcCallFrame) csc %p %s obj %s obj refcount %d\n",csc,
          csc ? Tcl_GetCommandName(interp, csc->cmdPtr) : NULL,
          objectName(csc->self),
          csc && csc->self->id ? Tcl_Command_refCount(csc->self->id) : -100
          );
#endif
  /* TODO: we could use Tcl_PushCallFrame(), if we would allocate the tcl stack frame earlier */
  result = TclPushStackFrame(interp, (Tcl_CallFrame **)&framePtr,
			     (Tcl_Namespace *) nsPtr,
			     (FRAME_IS_PROC|FRAME_IS_XOTCL_METHOD));
  if (result != TCL_OK) {
    return result;
  }

  framePtr->objc = objc;
  framePtr->objv = objv;
  framePtr->procPtr = procPtr;
#if defined(TCL85STACK_TRACE)
  fprintf(stderr,"     put csc %p into frame %p flags %.4x\n",csc,framePtr,framePtr->isProcCallFrame);
#endif
  framePtr->clientData = (ClientData)csc;

  return TCL_OK;
}
#endif

static void
getVarAndNameFromHash(Tcl_HashEntry *hPtr, Var **val, Tcl_Obj **varNameObj) {
  *val  = VarHashGetValue(hPtr);
#if defined(PRE85)
# if FORWARD_COMPATIBLE
  if (forwardCompatibleMode) {
    *varNameObj  = VarHashGetKey(*val);
  } else {
    *varNameObj  = Tcl_NewStringObj(Tcl_GetHashKey(hPtr->tablePtr, hPtr),-1);
  }
# else
  *varNameObj  = Tcl_NewStringObj(Tcl_GetHashKey(hPtr->tablePtr, hPtr),-1);
# endif
#else
  *varNameObj  = VarHashGetKey(*val);
#endif
}

void XOTclProcDeleteProc(ClientData clientData) {
  XOTclProcContext *ctxPtr = (XOTclProcContext *)clientData;
  (*ctxPtr->oldDeleteProc)(ctxPtr->oldDeleteData);
  if (ctxPtr->nonposArgs) {
    fprintf(stderr, "would free %p\n",ctxPtr->nonposArgs);
    /*FREE(XOTclProcContext, ctxPtr->clientData);*/
  }
  /*fprintf(stderr, "free %p\n",ctxPtr);*/
  FREE(XOTclProcContext, ctxPtr);
}

static Proc *FindProc(Tcl_Interp *interp, Tcl_HashTable *table, char *name);

static Proc *
getObjectProc(Tcl_Interp *interp, XOTclObject *obj, char *methodName) {
  if (obj->nsPtr) {
    return FindProc(interp, Tcl_Namespace_cmdTable(obj->nsPtr), methodName);
  }
  return NULL;
}

static Proc *
getClassProc(Tcl_Interp *interp, XOTclClass *class, char *methodName) {
  return FindProc(interp, Tcl_Namespace_cmdTable(class->nsPtr), methodName);
}

static XOTclNonposArgs *
getNonposArgs(Tcl_Command cmdPtr) {
  if (Tcl_Command_deleteProc(cmdPtr) == XOTclProcDeleteProc) {
    return ((XOTclProcContext *)Tcl_Command_deleteData(cmdPtr))->nonposArgs;
  }
  return NULL;
}

static int
addNonposArgs(Tcl_Interp *interp, Tcl_Command cmd, XOTclNonposArgs *nonposArgs) {
  Command *cmdPtr = (Command *)cmd;

  if (cmdPtr->deleteProc == TclProcDeleteProc) {
    XOTclProcContext *ctxPtr = NEW(XOTclProcContext);

    ctxPtr->oldDeleteData = (Proc *)cmdPtr->deleteData;
    ctxPtr->oldDeleteProc = cmdPtr->deleteProc;
    cmdPtr->deleteProc = XOTclProcDeleteProc;
    ctxPtr->nonposArgs = nonposArgs;
    cmdPtr->deleteData = (ClientData)ctxPtr;
    return TCL_OK;
  }
  return TCL_ERROR;
}


/*
 * method dispatch
 */

/* invoke a method implemented as a proc/instproc (with assertion checking) */
static int
invokeProcMethod(ClientData cp, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
         char *methodName, XOTclObject *obj, XOTclClass *cl, Tcl_Command cmdPtr, 
         XOTclCallStackContent *csc) {
  int result;
  XOTclObjectOpt *opt = obj->opt;
#if defined(PRE85)
  XOTcl_FrameDecls;
#endif

  assert(obj);
  assert(!obj->teardown);

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "+++ invokeProcMethod %s, csc %p, frametype %d, teardown %p\n",
          methodName, csc, csc->frameType, obj->teardown);
#endif

  /* 
   * if this is a filter, check whether its guard applies,
   * if not: just step forward to the next filter
   */

  if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
    XOTclCmdList *cmdList;
    /*
     * seek cmd in obj's filterOrder
     */
    assert(obj->flags & XOTCL_FILTER_ORDER_VALID);
    /* otherwise: FilterComputeDefined(interp, obj);*/
    
    for (cmdList = obj->filterOrder; cmdList && cmdList->cmdPtr != cmdPtr; cmdList = cmdList->nextPtr);
    
    /*
     * when it is found, check whether it has a filter guard
     */
    if (cmdList) {
      int rc = GuardCall(obj, cl, (Tcl_Command) cmdList->cmdPtr, interp,
                         cmdList->clientData, csc);
      if (rc != TCL_OK) {
        if (rc != TCL_ERROR) {
          /*
           * call next, use the given objv's, not the callstack objv
           * we may not be in a method, thus there may be wrong or
           * no callstackobjs
           */

          /*fprintf(stderr, "... calling nextmethod csc %p\n", csc); */
          
          /* the call stack content is not jet pushed to the tcl
             stack, so we pass it here explicitely */
          rc = XOTclNextMethod(obj, interp, cl, methodName,
                               objc, objv, /*useCallStackObjs*/ 0, csc);
          /*fprintf(stderr, "... after nextmethod\n");*/
        }
        
        return rc;
      }
    }
  }

  if (opt && (opt->checkoptions & CHECK_PRE) &&
      (result = AssertionCheck(interp, obj, cl, methodName, CHECK_PRE)) == TCL_ERROR) {
    goto finish;
  }
  
#ifdef DISPATCH_TRACE
  printCall(interp,"invokeProcMethod", objc, objv);
  fprintf(stderr,"\tproc=%s\n", Tcl_GetCommandName(interp, cmdPtr));
#endif
  
  /*
   * In case, we have Tcl 8.5.* or better, we can avoid calling the
   * standard TclObjInterpProc() and ::xotcl::initProcNS defined in
   * the method, since Tcl 8.5 has a separate functions
   * PushProcCallFrame() and TclObjInterpProcCore(), where the
   * latter is callable from the outside (e.g. from XOTcl). This new
   * interface allows us to setup the XOTcl callframe before the
   * bytecode of the method body (provisioned by PushProcCallFrame)
   * is executed. On the medium range, we do not need the xotcl
   * callframe when we stop supporting Tcl 8.4 (we should simply use
   * the calldata field in the callstack), which should be managed
   * here or in PushProcCallFrame. At the same time, we could do the
   * non-pos-arg handling here as well.
   */
#if !defined(PRE85)
  /*fprintf(stderr,"\tproc=%s cp=%p %d\n", Tcl_GetCommandName(interp, cmd),cp, isTclProc);*/
  
# if defined(CANONICAL_ARGS)
  /* 
     If the method to be invoked hasnonposArgs, we have to call the
     argument parser with the argument definitions obtained from the
     proc context from the cmdPtr.
  */
  {
    XOTclNonposArgs *nonposArgs = Tcl_Command_deleteProc(cmdPtr) == XOTclProcDeleteProc ?
      ((XOTclProcContext *)Tcl_Command_deleteData(cmdPtr))->nonposArgs : NULL;

    if (nonposArgs) {
      parseContext pc;
      result = canonicalNonpositionalArgs(&pc, interp, nonposArgs, methodName, objc, objv);
      if (result == TCL_OK) {
        result = PushProcCallFrame(cp, interp, pc.objc+1, pc.full_objv, csc);
        /* maybe release is to early */
        parseContextRelease(&pc);
      }
    } else {
      result = PushProcCallFrame(cp, interp, objc, objv, csc);
    }
  }
# else /* no CANONICAL ARGS */
  result = PushProcCallFrame(cp, interp, objc, objv, csc);
# endif 

  /*
   * The stack frame is pushed, we could do something here before
   * running the byte code of the body.
   */
  if (result == TCL_OK) {
#if !defined(TCL85STACK)
    RUNTIME_STATE(interp)->cs.top->currentFramePtr = (Tcl_CallFrame *) Tcl_Interp_varFramePtr(interp);
#endif
    result = TclObjInterpProcCore(interp, objv[0], 1, &MakeProcError);
  } else {
    result = TCL_ERROR;
  }
# if defined(TCL85STACK_TRACE)
  fprintf(stderr,"POP  OBJECT_FRAME (implicit) frame %p csc %p obj %s obj refcount %d %d\n", NULL, csc,
          objectName(obj),
          obj->id ? Tcl_Command_refCount(obj->id) : -100,
          obj->refCount
          );
# endif
#else /* BEFORE TCL85 */
  result = (*Tcl_Command_objProc(cmdPtr))(cp, interp, objc, objv);
#endif

#ifdef DISPATCH_TRACE
  printExit(interp,"invokeProcMethod", objc, objv, result);
  /* fprintf(stderr, " returnCode %d xotcl rc %d\n",
     Tcl_Interp_returnCode(interp), result);*/
#endif

  /*    fprintf(stderr, "dispatch returned %d rst = %d\n", result, rst->returnCode);*/

  opt = obj->opt;
  if (opt && obj->teardown &&
      (opt->checkoptions & CHECK_POST)) {
    result = AssertionCheck(interp, obj, cl, methodName, CHECK_POST);
  }

 finish:
  return result;
}

/* Invoke a method implemented as a cmd (with assertion checking) */
static int
invokeCmdMethod(ClientData cp, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
        char *methodName, XOTclObject *obj, Tcl_Command cmdPtr,
        XOTclCallStackContent *csc) {
  CheckOptions co;
  int result;
#if defined(TCL85STACK)
  XOTcl_FrameDecls;
#endif

  assert(obj);
  assert(!obj->teardown);

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "+++ invokeCmdMethodCheck %s, obj %p %s, csc %p, teardown %p\n",
          methodName, obj, objectName(obj), csc, obj->teardown);
#endif

  /*fprintf(stderr,".. calling cmd %s isTclProc %d tearDown %p csc %p\n",methodName,isTclProc,obj->teardown,csc);*/

  if (obj->opt) {
    co = obj->opt->checkoptions;
    if ((co & CHECK_INVAR) &&
        ((result = AssertionCheckInvars(interp, obj, methodName, co)) == TCL_ERROR)) {
      goto finish;
    }
  }
  
#if defined(TCL85STACK)
  if (csc) {
    /* We have a call stack content, but the following dispatch will
     * by itself no stack it; in order to get e.g. self working, we
     * have to stack at least an FRAME_IS_XOTCL_OBJECT.
     * TODO: maybe push should happen already before assertion checking,
     * but we have to check what happens in the finish target etc.
     */
    XOTcl_PushFrameCsc(interp, obj, csc);
    /*XOTcl_PushFrame(interp, obj);*/
  }
#endif
  
#ifdef DISPATCH_TRACE
  printCall(interp,"invokeCmdMethod cmd", objc, objv);
  fprintf(stderr,"\tcmd=%s\n", Tcl_GetCommandName(interp, cmdPtr));
#endif
  result = (*Tcl_Command_objProc(cmdPtr))(cp, interp, objc, objv);
#ifdef DISPATCH_TRACE
  printExit(interp,"invokeCmdMethod cmd", objc, objv, result);
#endif
  
#if defined(TCL85STACK)
  if (csc) {
    XOTcl_PopFrame(interp, obj);
  }
#endif
  
  /* The order of the if-condition below is important, since obj might be already
     freed in case the call was a "dealloc" */
  if (obj->opt) {
    co = obj->opt->checkoptions;
    if ((co & CHECK_INVAR) &&
        ((result = AssertionCheckInvars(interp, obj, methodName, co)) == TCL_ERROR)) {
      goto finish;
    }
  }

 finish:
  return result;
}

#if defined(PROFILE)
static int
InvokeMethod(ClientData clientData, Tcl_Interp *interp,
             int objc, Tcl_Obj *CONST objv[], Tcl_Command cmd, XOTclObject *obj, XOTclClass *cl,
             char *methodName, int frameType) {
  struct timeval trt;
  long int startUsec = (gettimeofday(&trt, NULL), trt.tv_usec), startSec = trt.tv_sec;

  result = __InvokeMethod__(clientData, interp, objc, objv, cmd, obj, cl, methodName, frameType);
  XOTclProfileEvaluateData(interp, startSec, startUsec, obj, cl, methodName);
  return result;
}
# define InvokeMethod __InvokeMethod__
#endif

/*
 * InvokeMethod() calls an XOTcl method. It calls either a
 * Tcl-implemented method (via invokeProcMethod()) or a C-implemented
 * method (via invokeCmdMethod()) and sets up stack and client data
 * accordingly.
 */

static int
InvokeMethod(ClientData clientData, Tcl_Interp *interp,
             int objc, Tcl_Obj *CONST objv[],
             Tcl_Command cmd, XOTclObject *obj, XOTclClass *cl,
             char *methodName, int frameType) {
  ClientData cp = Tcl_Command_objClientData(cmd);
  XOTclCallStackContent csc, *cscPtr = &csc;
  register Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);
  int rc;

  assert (!obj->teardown);
  /* before, we had a logic like the following:
  if (!obj->teardown) {
    return TCL_OK;
    } */

  /*fprintf(stderr, "InvokeMethod method '%s' cmd %p cp=%p objc=%d\n",methodName,cmd, cp, objc);*/

  if (proc == TclObjInterpProc) {
#if defined(TCL85STACK)
    CallStackPush(cscPtr, obj, cl, cmd, frameType);
#else
    if (!(cscPtr = CallStackPush(interp, obj, cl, cmd, frameType))) 
      return TCL_ERROR;
#endif
    rc = invokeProcMethod(cp, interp, objc, objv, methodName, obj, cl, cmd, cscPtr);
    CallStackPop(interp, cscPtr);
    return rc;

  } else if (cp) {
    /* a cmd with client data */
    if (proc == XOTclObjDispatch) {
      assert((TclIsProc((Command *)cmd) == NULL));
      /*fprintf(stderr,"\t ObjDispatch\n");*/
    } else if (proc == XOTclForwardMethod ||
	       proc == XOTclObjscopedMethod) {
      tclCmdClientData *tcd = (tclCmdClientData *)cp;
      tcd->obj = obj;
      assert((TclIsProc((Command *)cmd) == NULL));
    } else if (cp == (ClientData)XOTCL_CMD_NONLEAF_METHOD) {
      cp = clientData;
      assert((TclIsProc((Command *)cmd) == NULL));
    }

#if defined(TCL85STACK)
    CallStackPush(cscPtr, obj, cl, cmd, frameType);
#else
    if (!(cscPtr = CallStackPush(interp, obj, cl, cmd, frameType))) 
      return TCL_ERROR;
#endif
  } else {
    /* a cmd without client data */
    assert((TclIsProc((Command *)cmd) == NULL));
    cp = clientData;
    cscPtr = NULL;
  }

  rc = invokeCmdMethod(cp, interp, objc, objv, methodName, obj, cmd, cscPtr);

  if (cscPtr) {
    CallStackPop(interp, cscPtr);
  }

  return rc;
}


XOTCLINLINE static int
DoDispatch(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *CONST objv[], int flags) {
  register XOTclObject *obj = (XOTclObject*)clientData;
  int result = TCL_OK, mixinStackPushed = 0,
    filterStackPushed = 0, unknown = 0, objflags,
    frameType = XOTCL_CSC_TYPE_PLAIN;
  char *methodName;
  XOTclClass *cl = NULL;
  Tcl_Command cmd = NULL;
  XOTclRuntimeState *rst = RUNTIME_STATE(interp);
  Tcl_Obj *cmdName = obj->cmdName;

  assert(objc>0);
  methodName = ObjStr(objv[1]);

  /*fprintf(stderr,"DoDispatch obj = %s objc = %d 0=%s methodName=%s\n",
    objectName(obj), objc, ObjStr(objv[0]), methodName);*/

#ifdef DISPATCH_TRACE
  printCall(interp,"DISPATCH", objc, objv);
#endif


  objflags = obj->flags; /* avoid stalling */
  INCR_REF_COUNT(cmdName);

  if (!(objflags & XOTCL_FILTER_ORDER_VALID)) {
    FilterComputeDefined(interp, obj);
    objflags = obj->flags;
  }

  if (!(objflags & XOTCL_MIXIN_ORDER_VALID)) {
    MixinComputeDefined(interp, obj);
    objflags = obj->flags;
  }

    /* Only start new filter chain, if
       (a) filters are defined and
       (b) the toplevel csc entry is not an filter on self
    */

  if (((objflags & XOTCL_FILTER_ORDER_DEFINED_AND_VALID) == XOTCL_FILTER_ORDER_DEFINED_AND_VALID)
      && rst->doFilters
      && !rst->guardCount) {
    XOTclCallStackContent *csc = CallStackGetTopFrame(interp, NULL);

    if (csc && (obj != csc->self ||
                csc->frameType != XOTCL_CSC_TYPE_ACTIVE_FILTER)) {
      
      filterStackPushed = FilterStackPush(interp, obj, objv[1]);
      cmd = FilterSearchProc(interp, obj, &obj->filterStack->currentCmdPtr, &cl);
      if (cmd) {
        /*fprintf(stderr,"filterSearchProc returned cmd %p proc %p\n", cmd, proc);*/
        frameType = XOTCL_CSC_TYPE_ACTIVE_FILTER;
        methodName = (char *)Tcl_GetCommandName(interp, cmd);
      } else {
        FilterStackPop(obj);
        filterStackPushed = 0;
      }
    }
  }

  /* check if a mixin is to be called.
     don't use mixins on next method calls, since normally it is not
     intercepted (it is used as a primitive command).
     don't use mixins on init calls, since init is invoked on mixins
     during mixin registration (in XOTclOMixinMethod)
  */
  if ((objflags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) == XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
    
    mixinStackPushed = MixinStackPush(obj);
      
    if (frameType != XOTCL_CSC_TYPE_ACTIVE_FILTER) {
      cmd = MixinSearchProc(interp, obj, methodName, &cl,
                            &obj->mixinStack->currentCmdPtr);
      if (cmd) {
        frameType = XOTCL_CSC_TYPE_ACTIVE_MIXIN;
      } else { /* the else branch could be deleted */
        MixinStackPop(obj);
        mixinStackPushed = 0;
      }
    }
  }
  
  /* if no filter/mixin is found => do ordinary method lookup */
  if (cmd == NULL) {

    /* do we have a object-specific proc? */
    if (obj->nsPtr) {
      cmd = FindMethod(methodName, obj->nsPtr);
      /* fprintf(stderr,"lookup for proc in obj %p method %s nsPtr %p => %p\n",
         obj, methodName, obj->nsPtr, cmd);*/
    }
    /*fprintf(stderr,"findMethod for proc '%s' in %p returned %p\n", methodName, obj->nsPtr, cmd);*/
    
    if (cmd == NULL) {
      /* check for a method */
      XOTclClass *currentClass = obj->cl;
      if (currentClass->order == NULL) currentClass->order = TopoOrder(currentClass, Super);
      cl = SearchPLMethod(currentClass->order, methodName, &cmd);
    }
  }

  if (cmd) {
    result = TCL_OK;

    /*fprintf(stderr,"cmd %p %s flags %x\n", cmd, methodName,
      ((Command *) cmd)->flags && 0x00010000);*/

    /* check, whether we have a protected method, and whether the
       protected method, called on a different object. In this case,
       we call as well the unknown method */

    if ((Tcl_Command_flags(cmd) & XOTCL_CMD_PROTECTED_METHOD) &&
	(flags & XOTCL_CM_NO_UNKNOWN) == 0) {
      XOTclCallStackContent *csc = CallStackGetTopFrame(interp, NULL);
      XOTclObject *o = NULL, *self = csc ? csc->self : NULL;

      GetObjectFromObj(interp, objv[0], &o);
      /*fprintf(stderr,"+++ %s is protected, therefore maybe unknown %p %s self=%p o=%p\n",
	methodName, objv[0], ObjStr(objv[0]),
	csc->self, o);*/
      if (o != self) {
	/*fprintf(stderr,"+++ protected method %s is not invoked\n", methodName);*/
	unknown = 1;
      }
    }

    if (!unknown) {
      /*fprintf(stderr,"DoDispatch calls InvokeMethod with obj = %s frameType %d\n",
	objectName(obj), frameType);*/
      if ((result = InvokeMethod(clientData, interp, objc-1, objv+1, cmd, obj, cl,
                                 methodName, frameType)) == TCL_ERROR) {
	result = XOTclErrInProc(interp, cmdName,
				cl && cl->object.teardown ? cl->object.cmdName : NULL,
				methodName);
      }
      unknown = rst->unknown;
    }
  } else {
    unknown = 1;
  }

  if (result == TCL_OK) {
    /*fprintf(stderr,"after doCallProcCheck unknown == %d\n", unknown);*/
    if (unknown) {
      Tcl_Obj *unknownObj = XOTclGlobalObjects[XOTE_UNKNOWN];

      if (XOTclObjectIsClass(obj) && (flags & XOTCL_CM_NO_UNKNOWN)) {
	return XOTclVarErrMsg(interp, ObjStr(objv[0]),
			      ": unable to dispatch method '",
			      methodName, "'", (char *) NULL);
      } else if (objv[1] != unknownObj) {
	/*
	 * back off and try unknown;
	 */
        XOTclObject *obj = (XOTclObject*)clientData;
        ALLOC_ON_STACK(Tcl_Obj*, objc+1, tov);
        /*
          fprintf(stderr,"calling unknown for %s %s, flgs=%02x,%02x isClass=%d %p %s\n",
          objectName(obj), ObjStr(objv[1]), flags,  XOTCL_CM_NO_UNKNOWN,
          XOTclObjectIsClass(obj), obj, objectName(obj));
        */
        tov[0] = obj->cmdName;
        tov[1] = unknownObj;
        if (objc>1)
          memcpy(tov+2, objv+1, sizeof(Tcl_Obj *)*(objc-1));
        /*
          fprintf(stderr,"?? %s unknown %s\n", objectName(obj), ObjStr(tov[2]));
        */
        result = DoDispatch(clientData, interp, objc+1, tov, flags | XOTCL_CM_NO_UNKNOWN);
        FREE_ON_STACK(tov);
	
      } else { /* unknown failed */
        return XOTclVarErrMsg(interp, ObjStr(objv[0]),
			      ": unable to dispatch method '",
			      ObjStr(objv[2]), "'", (char *) NULL);
      }

    }
  }
  /* be sure to reset unknown flag */
  if (unknown)
    rst->unknown = 0;

#ifdef DISPATCH_TRACE
  printExit(interp,"DISPATCH", objc, objv, result);
#endif

  /*!(obj->flags & XOTCL_DESTROY_CALLED)) */
  if (mixinStackPushed && obj->mixinStack)
    MixinStackPop(obj);

  if (filterStackPushed && obj->filterStack)
    FilterStackPop(obj);

  DECR_REF_COUNT(cmdName); /* must be after last dereferencing of obj */
  return result;
}

int
XOTclObjDispatch(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  int result;
#ifdef STACK_TRACE
  XOTclStackDump(interp);
#endif

#ifdef CALLSTACK_TRACE
  XOTclCallStackDump(interp);
#endif

  if (objc > 1) {
    /* normal dispatch */
    result = DoDispatch(clientData, interp, objc, objv, 0);
  } else {
    Tcl_Obj *tov[2];
    tov[0] = objv[0];
    tov[1] = XOTclGlobalObjects[XOTE_DEFAULTMETHOD];
    result = DoDispatch(clientData, interp, 2, tov, 0);
  }

  return result;
}

#ifdef XOTCL_BYTECODE
int
XOTclDirectSelfDispatch(ClientData clientData, Tcl_Interp *interp,
                        int objc, Tcl_Obj *CONST objv[]) {
  int result;
#ifdef XOTCLOBJ_TRACE
  XOTclObject *obj;
#endif
  objTrace("BEFORE SELF DISPATCH", obj);
  result = XOTclObjDispatch((ClientData)GetSelfObj(interp), interp, objc, objv);
  objTrace("AFTER SELF DISPATCH", obj);
  return result;
}
#endif

/*
 * Non Positional Args
 */

static void argDefinitionsFree(argDefinition *argDefinitions);
static void NonposArgsFree(XOTclNonposArgs *nonposArgs) {
  if (nonposArgs->ifd) {
    argDefinitionsFree(nonposArgs->ifd);
  }
  FREE(XOTclNonposArgs, nonposArgs);
}
static void ParsedInterfaceDefinitionFree(XOTclParsedInterfaceDefinition *parsedIf) {
  /*fprintf(stderr, "ParsedInterfaceDefinitionFree %p, npargs %p\n",parsedIf,parsedIf->nonposArgs);*/
  if (parsedIf->nonposArgs) {
    NonposArgsFree(parsedIf->nonposArgs);
  }
  FREE(XOTclParsedInterfaceDefinition, parsedIf);
}

static Tcl_Obj *
NonposArgsFormat(Tcl_Interp *interp, XOTclNonposArgs *nonposArgs) {
  int first;
  Tcl_Obj *list = Tcl_NewListObj(0, NULL), *innerlist, *nameStringObj;
  argDefinition CONST *aPtr;

  for (aPtr = nonposArgs->ifd; aPtr->name; aPtr++) {
    if (*aPtr->name == '-') {
      first = 1;
      nameStringObj = Tcl_NewStringObj(aPtr->name, -1);
      if ((aPtr->flags & XOTCL_ARG_REQUIRED) || aPtr->type) {
        Tcl_AppendToObj(nameStringObj,":", 1);
        if (aPtr->flags & XOTCL_ARG_REQUIRED) {
          first = 0;
          Tcl_AppendToObj(nameStringObj,"required", 8);
        }
        if (aPtr->type) {
          if (!first)
            Tcl_AppendToObj(nameStringObj,",", 1);
          Tcl_AppendToObj(nameStringObj,aPtr->type, -1);
        }
      }

      innerlist = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, innerlist, nameStringObj);
      if (aPtr->defaultValue) {
        Tcl_ListObjAppendElement(interp, innerlist, aPtr->defaultValue);
      }

      Tcl_ListObjAppendElement(interp, list, innerlist);
    }
  }
  return list;
}

/*
 *  Proc-Creation
 */

static Tcl_Obj *addPrefixToBody(Tcl_Obj *body, int nonposArgs, XOTclParsedInterfaceDefinition *ifPtr) {
  Tcl_Obj *resultBody = Tcl_NewStringObj("", 0);

  INCR_REF_COUNT(resultBody);
#if defined(PRE85)
  Tcl_AppendStringsToObj(resultBody, "::xotcl::initProcNS\n", (char *) NULL);
  if (nonposArgs)
    Tcl_AppendStringsToObj(resultBody, "::eval ::xotcl::interpretNonpositionalArgs $args\n",
			   (char *) NULL);
#else
# if !defined(CANONICAL_ARGS)
  if (nonposArgs)
    Tcl_AppendStringsToObj(resultBody, "::xotcl::interpretNonpositionalArgs {*}$args\n",
			   (char *) NULL);
# else
  if (nonposArgs && ifPtr->possibleUnknowns > 0)
    Tcl_AppendStringsToObj(resultBody, "::xotcl::unsetUnknownArgs\n", (char *) NULL);
# endif
#endif
  Tcl_AppendStringsToObj(resultBody, ObjStr(body), (char *) NULL);
  return resultBody;
}

/* todo: maybe, we will need this for custom type checkers, so leave it for the time being */
static Tcl_Obj*
nonposargType(Tcl_Interp *interp, char *start, int len) {
  Tcl_Obj *result  = Tcl_NewListObj(0, NULL);
  Tcl_Obj *type    = Tcl_NewStringObj(start, len);
  Tcl_Obj *checker = Tcl_NewStringObj("type=", 5);

  Tcl_AppendToObj(checker, start, len);
  Tcl_ListObjAppendElement(interp, result, type);
  Tcl_ListObjAppendElement(interp, result, checker);
  /*fprintf(stderr, "nonposargType TYPE = '%s'\n", ObjStr(result));*/
  return result;
}

#define NEW_STRING(target,p,l)  target = ckalloc(l+1); strncpy(target,p,l); *((target)+l) = '\0'

static argDefinition *argDefinitionsNew(int nr) {
  argDefinition *interface = NEW_ARRAY(argDefinition,nr+1);
  memset(interface, 0, sizeof(argDefinition)*(nr+1));
  return interface;
}

static void argDefinitionsFree(argDefinition *argDefinitions) {
  argDefinition *ifPtr;

  /*fprintf(stderr,"freeing %d argDefinitions\n",nr);*/
  for (ifPtr=argDefinitions; ifPtr->name; ifPtr++) {
    /*fprintf(stderr,".... ifPtr = %p, name=%s, defaultValue %p\n",ifPtr,ifPtr->name,ifPtr->defaultValue);*/
    if (ifPtr->name) ckfree(ifPtr->name);
    if (ifPtr->defaultValue) {DECR_REF_COUNT(ifPtr->defaultValue);}
  }
  FREE(argDefinition*,argDefinitions);
}

XOTCLINLINE static int
noMetaChars(char *pattern) {
  register char c, *p = pattern;
  assert(pattern);
  for (c=*p; c; c = *++p) {
    if (c == '*' || c == '?' || c == '[') {
      return 0;
    }
  }
  return 1;
}

/*
 * type converter
 */
static int convertToString(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  *clientData = (char *)ObjStr(objPtr);
  return TCL_OK;
}
static int convertToTclobj(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  *clientData = (ClientData)objPtr;
  return TCL_OK;
}
static int convertToNothing(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  return TCL_OK;
}
static int convertToBoolean(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  int result, bool;
  result = Tcl_GetBooleanFromObj(interp, objPtr, &bool);
  if (result == TCL_OK) *clientData = (ClientData)bool;
  return result;
}
static int convertToInteger(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  int result, i;
  result = Tcl_GetIntFromObj(interp, objPtr, &i);
  if (result == TCL_OK) *clientData = (ClientData)i;
  return result;
}
static int convertToSwitch(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  return convertToBoolean(interp, objPtr, clientData);
}
static int convertToClass(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  if (GetClassFromObj(interp, objPtr, (XOTclClass **)clientData, 0) == TCL_OK)
    return TCL_OK;
  return XOTclObjErrType(interp, objPtr, "class");
}

static int convertToRelation(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  /*TODO: should we check wheter it is a valid object and/or filter method, somehow?!*/
  return TCL_OK;
}


static int convertToObject(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  if (GetObjectFromObj(interp, objPtr, (XOTclObject **)clientData) == TCL_OK)
    return TCL_OK;
  return XOTclObjErrType(interp, objPtr, "object");
}

static int convertToObjpattern(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  Tcl_Obj *patternObj = objPtr;
  char *pattern = ObjStr(objPtr);

  if (noMetaChars(pattern)) {
    /* we have no meta characters, we try to check for an existing object */
    XOTclObject *obj = NULL;
    GetObjectFromObj(interp, objPtr, &obj);
    if (obj) {
      patternObj = obj->cmdName;
    }
  } else {
    /*
     * We have a pattern and meta characters, we might have
     * to prefix it to ovoid abvious errors: since all object
     * names are prefixed with ::, we add this prefix automatically
     * to the match pattern, if it does not exist
     */
    if (*pattern != ':' && *pattern+1 != ':') {
      patternObj = Tcl_NewStringObj("::", 2);
      Tcl_AppendToObj(patternObj, pattern, -1);
    }
  }
  if (patternObj) {
    INCR_REF_COUNT(patternObj);
  }
  *clientData = (ClientData)patternObj;
  return TCL_OK;
}

static int
parseNonposargsOption(Tcl_Interp *interp, char *option, int length, argDefinition *ifPtr) {
  /*fprintf(stderr, "def %s, option '%s' (%d)\n",ifPtr->name,option,length);*/
  if (strncmp(option,"required",length) == 0) {
    ifPtr->flags |= XOTCL_ARG_REQUIRED;
  } else if (strncmp(option,"substdefault",length) == 0) {
    ifPtr->flags |= XOTCL_ARG_SUBST_DEFAULT;
  } else if (strncmp(option,"switch",length) == 0) {
    ifPtr->nrargs = 0;
    ifPtr->converter = convertToSwitch;
    assert(ifPtr->defaultValue == NULL);
    ifPtr->defaultValue = Tcl_NewBooleanObj(0);
    INCR_REF_COUNT(ifPtr->defaultValue);
    ifPtr->type = "switch";
  } else if (strncmp(option,"integer",length) == 0) {
    ifPtr->nrargs = 1;
    ifPtr->converter = convertToInteger;
    ifPtr->type = "integer";
  } else if (strncmp(option,"boolean",length) == 0) {
    ifPtr->nrargs = 1;
    ifPtr->converter = convertToBoolean;
    ifPtr->type = "boolean";
  } else if (strncmp(option,"object",length) == 0) {
    ifPtr->nrargs = 1;
    ifPtr->converter = convertToObject;
    ifPtr->type = "object";
  } else if (strncmp(option,"class",length) == 0) {
    ifPtr->nrargs = 1;
    ifPtr->converter = convertToClass;
    ifPtr->type = "class";
  } else if (strncmp(option,"relation",length) == 0) {
    ifPtr->nrargs = 1;
    ifPtr->converter = convertToRelation;
    ifPtr->type = "tclobj";
  } else {
    fprintf(stderr, "**** unknown argument option: def %s, option '%s' (%d)\n",ifPtr->name,option,length);
  }
  return TCL_OK;
}

static int
parseArgDefinition(Tcl_Interp *interp, char *procName, Tcl_Obj *arg, int isNonposArgument,
                   argDefinition *ifPtr, int *possibleUnknowns) {
  Tcl_Obj **npav;
  char *argString, *argName;
  int rc, npac, length, j, nameLength;

  rc = Tcl_ListObjGetElements(interp, arg, &npac, &npav);
  if (rc != TCL_OK || npac < 1 || npac > 2) {
    return XOTclVarErrMsg(interp, "wrong # of elements in non-positional args for method",
                          procName, " (should be 1 or 2 list elements): ",
                          ObjStr(arg), (char *) NULL);
  }

  argString = ObjStr(npav[0]);
  length = strlen(argString);

  if (isNonposArgument && *argString != '-') {
    return XOTclVarErrMsg(interp, "non-positional arg '", argString,"' of method ",procName,
                          " does not start with '-': ", argString, (char *) NULL);
  }

  if (isNonposArgument) {
    argName = argString+1;
    nameLength = length-1;
    ifPtr->nrargs = 1; /* per default 1 argument, switches set their arg numbers */
  } else {
    argName = argString;
    nameLength = length;
    ifPtr->flags |= XOTCL_ARG_REQUIRED; /* positional arguments are required unless we have a default */
  }

  /*fprintf(stderr, "... parsing '%s', name '%s' \n",ObjStr(arg),argName);*/

  /* find the first ':' */
  for (j=0; j<length; j++) {
    if (argString[j] == ':') break;
  }

  if (argString[j] == ':') {
    /* we found a ':' */
    int l, start, end;

    NEW_STRING(ifPtr->name,argString,j);
    /* skip space */
    for (start = j+1; start<length && isspace((int)argString[start]); start++) {;}

    /* search for ',' */
    for (l=start; l<length;l++) {
      if (argString[l] == ',') {
        for (end = l; end>0 && isspace((int)argString[end-1]); end--);
        parseNonposargsOption(interp, argString+start, end-start, ifPtr);
        l++;
        /* skip space */
        for (start = l; start<length && isspace((int)argString[start]); start++) {;}
      }
    }
    for (end = l; end>0 && isspace((int)argString[end-1]); end--);
    /* process last option */
    parseNonposargsOption(interp, argString+start, end-start, ifPtr);
  } else {
    /* no ':', the whole arg is the name */
    NEW_STRING(ifPtr->name,argString,length);
    if (isArgsString(argString)) {
      ifPtr->converter = convertToNothing;
      ifPtr->flags &= ~XOTCL_ARG_REQUIRED;
    }
  }

  /* if we have two arguments in the list, the second one is a default value */
  if (npac == 2) {
    /* if we have for some reason already a default value, free it */
    if (ifPtr->defaultValue) {
      /* might be set by parseNonposargsOption */
      DECR_REF_COUNT(ifPtr->defaultValue);
    }
    ifPtr->defaultValue = Tcl_DuplicateObj(npav[1]);
    INCR_REF_COUNT(ifPtr->defaultValue);
    /* the argument will not required for an invocation, since we have a default */
    ifPtr->flags &= ~XOTCL_ARG_REQUIRED;
  }

  /*fprintf(stderr,"%p %s ifPtr->name = '%s', nrargs %d, required %d, converter %p default %s\n",ifPtr,procName,
          ifPtr->name,ifPtr->nrargs,ifPtr->flags & XOTCL_ARG_REQUIRED, ifPtr->converter,
          ifPtr->defaultValue ? ObjStr(ifPtr->defaultValue) : "NONE");*/
  if (ifPtr->converter == NULL) {
    ifPtr->converter = convertToTclobj;
  }

  /*
   * If the argument is not required and no default value is
   * specified, we have to handle in the client code (eg. in the
   * canonical arg handlers for instprocs) the unknown value
   * (e.g. don't set/unset a variable)
   */
  if (!(ifPtr->flags & XOTCL_ARG_REQUIRED) && ifPtr->defaultValue == NULL) {
    (*possibleUnknowns)++;
  }
  return TCL_OK;
}

static int
parseNonposArgs(Tcl_Interp *interp, char *procName, Tcl_Obj *npArgs, Tcl_Obj *ordinaryArgs,
                int *haveNonposArgs, XOTclParsedInterfaceDefinition *parsedIfPtr) {
  int rc, i, nonposArgsDefc, ordinaryArgsDefc, possibleUnknowns = 0;
  Tcl_Obj **nonposArgsDefv, **ordinaryArgsDefv;
  argDefinition *interface, *ifPtr;

  rc = Tcl_ListObjGetElements(interp, npArgs, &nonposArgsDefc, &nonposArgsDefv);
  if (rc != TCL_OK) {
    return XOTclVarErrMsg(interp, "cannot break down non-positional args: ",
			  ObjStr(npArgs), (char *) NULL);
  }
  rc = Tcl_ListObjGetElements(interp, ordinaryArgs, &ordinaryArgsDefc, &ordinaryArgsDefv);
  if (rc != TCL_OK) {
    return XOTclVarErrMsg(interp, "cannot break down ordinary args: ",
			  ObjStr(ordinaryArgs), (char *) NULL);
  }

  ifPtr = interface = argDefinitionsNew(nonposArgsDefc+ordinaryArgsDefc);

  if (nonposArgsDefc > 0) {
    for (i=0; i < nonposArgsDefc; i++, ifPtr++) {
      rc = parseArgDefinition(interp, procName, nonposArgsDefv[i], 1, ifPtr, &possibleUnknowns);
      if (rc != TCL_OK) {
        argDefinitionsFree(interface);
        return rc;
      }
      *haveNonposArgs = 1;
    }

    /* TODO:
       for the time being, process the pos args only when we have nonpos args.
       We have to benchmark the overhead and maybe we have to provide a switch
       via e.g. configure to activate/deactivate pos args handling.
    */
    if (*haveNonposArgs) {
      for (i=0; i< ordinaryArgsDefc; i++, ifPtr++) {
        rc = parseArgDefinition(interp, procName, ordinaryArgsDefv[i], 0, ifPtr, &possibleUnknowns);
        if (rc != TCL_OK) {
          argDefinitionsFree(interface);
          return rc;
        }
      }
    }

    if (*haveNonposArgs) {
      XOTclNonposArgs *nonposArg = NEW(XOTclNonposArgs);
      MEM_COUNT_ALLOC("nonposArg", nonposArg);

      nonposArg->slotObj = NULL;
      nonposArg->ifd = interface;
      nonposArg->ifdSize = ifPtr-interface;
      /*fprintf(stderr, "method %s ifsize %d, possible unknowns = %d,\n",
	procName,ifPtr-interface,possibleUnknowns);*/
      parsedIfPtr->nonposArgs = nonposArg;
      parsedIfPtr->possibleUnknowns = possibleUnknowns;
    } else {
      /* empty definitions */
    }
  }
  return TCL_OK;
}

static int
GenerateIfd(Tcl_Interp *interp, char *methodName, Tcl_Obj *input, 
            XOTclParsedInterfaceDefinition *output, int *hasNpArgs) {
  int result = TCL_OK, argsc, i;
  Tcl_Obj **argsv;

  *hasNpArgs = 0;
  output->nonposArgs = NULL;
  output->possibleUnknowns = 0;

  /* see, if we have nonposArgs in the ordinary argument list */
  result = Tcl_ListObjGetElements(interp, input, &argsc, &argsv);
  if (result != TCL_OK) {
    return XOTclVarErrMsg(interp, "cannot break args into list: ",
                          ObjStr(input), (char *) NULL);
  }
  for (i=0; i<argsc; i++) {
    char *arg;
    int npac, rc;
    Tcl_Obj **npav;

    rc = Tcl_ListObjGetElements(interp, argsv[i], &npac, &npav);
    if (rc == TCL_OK && npac > 0) {
      arg = ObjStr(npav[0]);
      if (*arg == '-') {
        *hasNpArgs = 1;
        continue;
      }
    }
    break;
  }
  if (*hasNpArgs) {
    int nrOrdinaryArgs = argsc - i;
    Tcl_Obj *ordinaryArgs = Tcl_NewListObj(nrOrdinaryArgs, &argsv[i]);
    Tcl_Obj *nonposArgs = Tcl_NewListObj(i, &argsv[0]);
    INCR_REF_COUNT(ordinaryArgs);
    INCR_REF_COUNT(nonposArgs);
    result = parseNonposArgs(interp, methodName, nonposArgs, ordinaryArgs,
                             hasNpArgs, output);
    DECR_REF_COUNT(ordinaryArgs);
    DECR_REF_COUNT(nonposArgs);
  }
  return result;
}

static int
MakeProc(Tcl_Namespace *nsPtr, XOTclAssertionStore *aStore, Tcl_Interp *interp, 
         Tcl_Obj *name, Tcl_Obj *args, Tcl_Obj *body, Tcl_Obj *precondition, 
         Tcl_Obj *postcondition, XOTclObject *obj, int clsns) {
  int result, haveNonposArgs = 0, argsc, i;
  TclCallFrame frame, *framePtr = &frame;
  Tcl_Obj *ov[4],  **argsv;
  char *procName = ObjStr(name);
  XOTclParsedInterfaceDefinition parsedIf;

  //parsedIf.nonposArgs = NULL;
  //parsedIf.possibleUnknowns = 0;

  ov[0] = NULL; /*objv[0];*/
  ov[1] = name;

  /* Obtain an signature description */
  result = GenerateIfd(interp, procName, args, &parsedIf, &haveNonposArgs);
  if (result != TCL_OK)
    return result;

  /* see, if we have nonposArgs in the ordinary argument list */
  /*result = Tcl_ListObjGetElements(interp, args, &argsc, &argsv);
  if (result != TCL_OK) {
    return XOTclVarErrMsg(interp, "cannot break args into list: ",
                          ObjStr(args), (char *) NULL);
  }
  for (i=0; i<argsc; i++) {
    char *arg;
    int npac, rc;
    Tcl_Obj **npav;

    rc = Tcl_ListObjGetElements(interp, argsv[i], &npac, &npav);
    if (rc == TCL_OK && npac > 0) {
      arg = ObjStr(npav[0]);
      // fprintf(stderr, "*** argparse1 arg='%s' rc=%d\n", arg, rc);
      if (*arg == '-') {
        haveNonposArgs = 1;
        continue;
      }
    }
    break;
  }
  if (haveNonposArgs) {
    int nrOrdinaryArgs = argsc - i;
    Tcl_Obj *ordinaryArgs = Tcl_NewListObj(nrOrdinaryArgs, &argsv[i]);
    Tcl_Obj *nonposArgs = Tcl_NewListObj(i, &argsv[0]);
    INCR_REF_COUNT(ordinaryArgs);
    INCR_REF_COUNT(nonposArgs);
    result = parseNonposArgs(interp, procName, nonposArgs, ordinaryArgs,
                             &haveNonposArgs, &parsedIf);
    DECR_REF_COUNT(ordinaryArgs);
    DECR_REF_COUNT(nonposArgs);
    if (result != TCL_OK)
      return result;
  }*/

  if (haveNonposArgs) {
# if defined(CANONICAL_ARGS)
    argDefinition *aPtr;
    Tcl_Obj *argList = Tcl_NewListObj(0, NULL);
    for (aPtr = parsedIf.nonposArgs->ifd; aPtr->name; aPtr++) {
      if (*aPtr->name == '-') {
	Tcl_ListObjAppendElement(interp, argList, Tcl_NewStringObj(aPtr->name+1,-1));
      } else {
	Tcl_ListObjAppendElement(interp, argList, Tcl_NewStringObj(aPtr->name,-1));
      }
    }
    ov[2] = argList;
    INCR_REF_COUNT(ov[2]);
    /*fprintf(stderr, "final arglist = <%s>\n",ObjStr(argList)); */
#else
    ov[2] = XOTclGlobalObjects[XOTE_ARGS];
#endif
    ov[3] = addPrefixToBody(body, 1, &parsedIf);
  } else { /* no nonpos arguments */
    ov[2] = args;
    ov[3] = addPrefixToBody(body, 0, &parsedIf);
  }

  Tcl_PushCallFrame(interp,(Tcl_CallFrame *)framePtr, nsPtr, 0);

  result = Tcl_ProcObjCmd(0, interp, 4, ov) != TCL_OK;
#if defined(NAMESPACEINSTPROCS)
  {
    Proc *procPtr = TclFindProc((Interp *)interp, procName);

    /*** patch the command ****/
    if (procPtr) {
      if (clsns) {
	/* set the namespace of the method as inside of the class */
	if (!obj->nsPtr) {
	  makeObjNamespace(interp, obj);
	}
	/*fprintf(stderr,"obj %s\n", objectName(obj));
	  fprintf(stderr,"ns %p obj->ns %p\n", ns, obj->nsPtr);
	  fprintf(stderr,"ns %s obj->ns %s\n", ns->fullName, obj->nsPtr->fullName);*/
	procPtr->cmdPtr->nsPtr = (Namespace*) obj->nsPtr;
      } else {
	/* set the namespace of the method to the same namespace the class has */
	procPtr->cmdPtr->nsPtr = ((Command *)obj->id)->nsPtr;
      }
    }
    addNonposArgs(interp, (Tcl_Command)procPtr->cmdPtr, parsedIf.nonposArgs);
  }
#endif

  Tcl_PopCallFrame(interp);

  if (precondition || postcondition) {
    AssertionAddProc(interp, ObjStr(name), aStore, precondition, postcondition);
  }

#if defined(CANONICAL_ARGS)
  if (haveNonposArgs) {
    DECR_REF_COUNT(ov[2]);
  }
#endif
  DECR_REF_COUNT(ov[3]);

  return result;
}

static int makeMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *name, Tcl_Obj *args, Tcl_Obj *body,
		      Tcl_Obj *precondition, Tcl_Obj *postcondition, int clsns) {
  XOTclClassOpt *opt = cl->opt;
  int result = TCL_OK;
  char *argStr = ObjStr(args), *bdyStr = ObjStr(body), *nameStr = ObjStr(name);

  if ((cl->object.flags & XOTCL_IS_ROOT_CLASS && isDestroyString(nameStr)) ||
      (cl->object.flags & XOTCL_IS_ROOT_META_CLASS && isDeallocString(nameStr)) ||
      (cl->object.flags & XOTCL_IS_ROOT_META_CLASS && isAllocString(nameStr)) ||
      (cl->object.flags & XOTCL_IS_ROOT_META_CLASS && isCreateString(nameStr)))
    return XOTclVarErrMsg(interp, className(cl), " method '", nameStr, "' of ",
                          className(cl), " can not be overwritten. Derive a ",
                          "sub-class", (char *) NULL);
  if (precondition && !postcondition) {
    return XOTclVarErrMsg(interp, className(cl), " method '", nameStr,
                          "'; when specifying a precondition (", ObjStr(precondition),
                          ") a postcondition must be specified as well",
                          (char *) NULL);
  }

  /* if both, args and body are empty strings, we delete the method */
  if (*argStr == 0 && *bdyStr == 0) {
    result = XOTclRemoveIMethod(interp, (XOTcl_Class *)cl, nameStr);
  } else {
    XOTclAssertionStore *aStore = NULL;
    if (precondition || postcondition) {
      opt = XOTclRequireClassOpt(cl);
      if (!opt->assertions)
        opt->assertions = AssertionCreateStore();
      aStore = opt->assertions;
    }
    result = MakeProc(cl->nsPtr, aStore, 
		      interp, name, args, body, precondition, postcondition,
		      &cl->object, clsns);
  }

  /* could be a filter or filter inheritance ... update filter orders */
  FilterInvalidateObjOrders(interp, cl);

  return result;
}

static int
getMatchObject(Tcl_Interp *interp, Tcl_Obj *patternObj,  Tcl_Obj *origObj,
                XOTclObject **matchObject, char **pattern) {
  if (patternObj) {
    *pattern = ObjStr(patternObj);
    if (IsXOTclTclObj(interp, patternObj, matchObject)) {
    } else if (patternObj == origObj && **pattern != ':') {
      /* no meta chars, but no appropriate xotcl object found, so
         return empty; we could check above with noMetaChars(pattern)
         as well, but the only remaining case are leading colons and
         metachars. */
      return 1;
    }
  }
  return 0;
}

static void forwardCmdDeleteProc(ClientData clientData) {
  forwardCmdClientData *tcd = (forwardCmdClientData *)clientData;
  if (tcd->cmdName)     {DECR_REF_COUNT(tcd->cmdName);}
  if (tcd->subcommands) {DECR_REF_COUNT(tcd->subcommands);}
  if (tcd->onerror)     {DECR_REF_COUNT(tcd->onerror);}
  if (tcd->prefix)      {DECR_REF_COUNT(tcd->prefix);}
  if (tcd->args)        {DECR_REF_COUNT(tcd->args);}
  FREE(forwardCmdClientData, tcd);
}

static int
forwardProcessOptions(Tcl_Interp *interp, Tcl_Obj *name,
                       Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix,
                       int withObjscope, Tcl_Obj *withOnerror, int withVerbose,
                       Tcl_Obj *target, int objc, Tcl_Obj * CONST objv[],
                       forwardCmdClientData **tcdp) {
  forwardCmdClientData *tcd;
  int i, rc = 0;

  tcd = NEW(forwardCmdClientData);
  memset(tcd, 0, sizeof(forwardCmdClientData));

  if (withDefault) {
    tcd->subcommands = withDefault;
    rc = Tcl_ListObjLength(interp, withDefault, &tcd->nr_subcommands);
    INCR_REF_COUNT(tcd->subcommands);
  }
  if (withMethodprefix) {
    tcd->prefix = withMethodprefix;
    INCR_REF_COUNT(tcd->prefix);
  }
  if (withOnerror) {
    tcd->onerror = withOnerror;
    INCR_REF_COUNT(tcd->onerror);
  }
  tcd->objscope = withObjscope;
  tcd->verbose = withVerbose;
  tcd->needobjmap = 0;
  tcd->cmdName = target;
  /*fprintf(stderr, "...forwardprocess objc %d\n",objc);*/

  for (i=0; i<objc; i++) {
    char *element = ObjStr(objv[i]);
    /*fprintf(stderr, "...forwardprocess element '%s'\n",element);*/
    tcd->needobjmap |= (*element == '%' && *(element+1) == '@');
    if (tcd->args == NULL) {
      tcd->args = Tcl_NewListObj(1, &objv[i]);
      tcd->nr_args++;
      INCR_REF_COUNT(tcd->args);
    } else {
      Tcl_ListObjAppendElement(interp, tcd->args, objv[i]);
      tcd->nr_args++;
    }
  }

  if (!tcd->cmdName) {
    tcd->cmdName = name;
  }

  /*fprintf(stderr, "cmdName = %s, args = %s, # = %d\n",
    ObjStr(tcd->cmdName), tcd->args?ObjStr(tcd->args):"NULL", tcd->nr_args);*/

  if (tcd->objscope) {
    /* when we evaluating objscope, and define ...
       o forward append -objscope append
       a call to
       o append ...
       would lead to a recursive call; so we add the appropriate namespace
    */
    char *nameString = ObjStr(tcd->cmdName);
    if (!isAbsolutePath(nameString)) {
      tcd->cmdName = NameInNamespaceObj(interp, nameString, callingNameSpace(interp));
      /*fprintf(stderr,"name %s not absolute, therefore qualifying %s\n", name,
        ObjStr(tcd->cmdName));*/
    }
  }
  INCR_REF_COUNT(tcd->cmdName);

  if (withEarlybinding) {
    Tcl_Command cmd = Tcl_GetCommandFromObj(interp, tcd->cmdName);
    if (cmd == NULL)
      return XOTclVarErrMsg(interp, "cannot lookup command '", ObjStr(tcd->cmdName), "'", (char *) NULL);

    tcd->objProc = Tcl_Command_objProc(cmd);
    if (tcd->objProc == XOTclObjDispatch     /* don't do direct invoke on xotcl objects */
        || tcd->objProc == TclObjInterpProc  /* don't do direct invoke on tcl procs */
        ) {
      /* silently ignore earlybinding flag */
      tcd->objProc = NULL;
    } else {
      tcd->clientData = Tcl_Command_objClientData(cmd);
    }
  }

  tcd->passthrough = !tcd->args && *(ObjStr(tcd->cmdName)) != '%' && tcd->objProc;

  /*fprintf(stderr, "forward args = %p, name = '%s'\n", tcd->args, ObjStr(tcd->cmdName));*/
  if (rc == TCL_OK) {
    *tcdp = tcd;
  } else {
    forwardCmdDeleteProc((ClientData)tcd);
  }
  return rc;
}

static XOTclClasses *
ComputePrecedenceList(Tcl_Interp *interp, XOTclObject *obj, char *pattern,
		      int withMixins, int withRootClass) {
  XOTclClasses *precedenceList = NULL, *pcl, **npl = &precedenceList;

  if (withMixins) {
    if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
      MixinComputeDefined(interp, obj);

    if (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
      XOTclCmdList *ml = obj->mixinOrder;

      while (ml) {
	XOTclClass *mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
	if (pattern) {
	  char *name = className(mixin);
	  if (!Tcl_StringMatch(name, pattern)) continue;
	}
	npl = XOTclClassListAdd(npl, mixin, NULL);
	ml = ml->nextPtr;
      }
    }
  }

  pcl = ComputeOrder(obj->cl, obj->cl->order, Super);
  for (; pcl; pcl = pcl->nextPtr) {
    if (withRootClass == 0 && pcl->cl->object.flags & XOTCL_IS_ROOT_CLASS)
      continue;

    if (pattern) {
      char *name = className(pcl->cl);
      if (!Tcl_StringMatch(name, pattern)) continue;
    }
    npl = XOTclClassListAdd(npl, pcl->cl, NULL);
  }
  return precedenceList;
}


static Proc*
FindProc(Tcl_Interp *interp, Tcl_HashTable *table, char *name) {
  Tcl_HashEntry *hPtr = table ? XOTcl_FindHashEntry(table, name) : 0;
  if (hPtr) {
    Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
    Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);
    if (proc == RUNTIME_STATE(interp)->objInterpProc)
      return (Proc*) Tcl_Command_objClientData(cmd);
#if USE_INTERP_PROC
    else if ((Tcl_CmdProc*)proc == RUNTIME_STATE(interp)->interpProc)
      return (Proc*) Tcl_Command_clientData(cmd);
#endif
  }
  return 0;
}

static void
AppendOrdinaryArgsFromNonposArgs(Tcl_Interp *interp, XOTclNonposArgs *nonposArgs,
				 int varsOnly, Tcl_Obj *argList) {
  argDefinition CONST *aPtr;

  for (aPtr = nonposArgs->ifd; aPtr->name; aPtr++) {
    if (*aPtr->name == '-') continue;
    if (varsOnly || aPtr->defaultValue == NULL) {
      Tcl_ListObjAppendElement(interp, argList, Tcl_NewStringObj(aPtr->name,-1));
    } else {
      Tcl_Obj *pair = Tcl_NewListObj(0,NULL);
      Tcl_ListObjAppendElement(interp, pair, Tcl_NewStringObj(aPtr->name,-1));
      Tcl_ListObjAppendElement(interp, pair, aPtr->defaultValue);
      Tcl_ListObjAppendElement(interp, argList, pair);
    }
  }
}

static int
GetProcDefault(Tcl_Interp *interp, Proc *proc, char *arg, Tcl_Obj **resultObj) {
  *resultObj = NULL;
  if (proc) {
    CompiledLocal *ap;
    for (ap = proc->firstLocalPtr; ap; ap = ap->nextPtr) {
      if (!TclIsCompiledLocalArgument(ap)) continue;
      if (strcmp(arg, ap->name) != 0) continue;
	
      if (ap->defValuePtr) {
        *resultObj = ap->defValuePtr;
        return TCL_OK;
      }
      return TCL_OK;
    }
  }
  return TCL_ERROR;
}

static int
SetProcDefault(Tcl_Interp *interp, Tcl_Obj *var, Tcl_Obj *defVal) {
  int result = TCL_OK;
  callFrameContext ctx = {0};
  CallStackUseActiveFrames(interp, &ctx);

  if (defVal) {
    if (Tcl_ObjSetVar2(interp, var, NULL, defVal, 0)) {
      Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    } else {
      result = TCL_ERROR;
    }
  } else {
    if (Tcl_ObjSetVar2(interp, var, NULL,
                       XOTclGlobalObjects[XOTE_EMPTY], 0)) {
      Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
    } else {
      result = TCL_ERROR;
    }
  }
  CallStackRestoreSavedFrames(interp, &ctx);

  if (result == TCL_ERROR) {
    XOTclVarErrMsg(interp, "couldn't store default value in variable '",
		   var, "'", (char *) NULL);
  }
  return result;
}

static char *
StripBodyPrefix(char *body) {
#if defined(PRE85)
  if (strncmp(body, "::xotcl::initProcNS\n", 20) == 0)
    body+=20;
  if (strncmp(body, "::eval ::xotcl::interpretNonpositionalArgs $args\n", 49) == 0)
    body += 49;
#else
# if !defined(CANONICAL_ARGS)
  if (strncmp(body, "::xotcl::interpretNonpositionalArgs {*}$args\n", 45) == 0)
    body += 45;
# else
  if (strncmp(body, "::xotcl::unsetUnknownArgs\n", 26) == 0)
    body += 26;
# endif
#endif
  return body;
}


static XOTclObjects *
computeSlotObjects(Tcl_Interp *interp, XOTclObject *obj, char *pattern, int withRootClass) {
  XOTclObjects *slotObjects = NULL, **npl = &slotObjects;
  XOTclClasses *pl;
  XOTclObject *childobj, *o;
  Tcl_HashTable slotTable;

  assert(obj);

  Tcl_InitHashTable(&slotTable, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", slotTable);

  pl = ComputePrecedenceList(interp, obj, NULL /* pattern*/, 1, withRootClass);
  for (; pl; pl = pl->nextPtr) {
    Tcl_DString ds, *dsPtr = &ds;

    DSTRING_INIT(dsPtr);
    Tcl_DStringAppend(dsPtr, className(pl->cl), -1);
    Tcl_DStringAppend(dsPtr, "::slot", 6);
    o = XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
    if (o) {
      Tcl_HashSearch hSrch;
      Tcl_HashEntry *hPtr, *slotEntry;
      /*fprintf(stderr,"we have slots %s\n", Tcl_DStringValue(dsPtr));*/
      Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(o->nsPtr);
      Tcl_Command cmd;
      int new;
      hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch);
      for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
	char *key = Tcl_GetHashKey(cmdTable, hPtr);
	slotEntry = Tcl_CreateHashEntry(&slotTable, key, &new);
	if (!new) continue;
	cmd = (Tcl_Command) Tcl_GetHashValue(hPtr);
	childobj = XOTclGetObjectFromCmdPtr(cmd);
	/* (childobj->id && Tcl_Command_nsPtr(childobj->id) == obj->nsPtr)   true children */
	/*fprintf(stderr,"we have true child obj %s\n", objectName(childobj));*/
	npl = XOTclObjectListAdd(npl, childobj);
      }
    }
    DSTRING_FREE(dsPtr);
  }

  Tcl_DeleteHashTable(&slotTable);
  MEM_COUNT_FREE("Tcl_InitHashTable", slotTable);

  XOTclClassListFree(pl);

  return slotObjects;
}

static XOTclClass*
FindCalledClass(Tcl_Interp *interp, XOTclObject *obj) {
  XOTclCallStackContent *csc = CallStackGetTopFrame(interp, NULL);
  char *methodName;
  Tcl_Command cmd;

  if (csc->frameType == XOTCL_CSC_TYPE_PLAIN)
    return csc->cl;

  if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER)
    methodName = ObjStr(csc->filterStackEntry->calledProc);
  else if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_MIXIN && obj->mixinStack)
    methodName = (char *)Tcl_GetCommandName(interp, csc->cmdPtr);
  else
    return NULL;

  if (obj->nsPtr) {
    cmd = FindMethod(methodName, obj->nsPtr);
    if (cmd) {
      /* we called an object specific method */
      return NULL;
    }
  }

  return SearchCMethod(obj->cl, methodName, &cmd);
}

/*
 * Next Primitive Handling
 */
XOTCLINLINE static void
NextSearchMethod(XOTclObject *obj, Tcl_Interp *interp, XOTclCallStackContent *csc,
                 XOTclClass **cl, char **method, Tcl_Command *cmd,
                 int *isMixinEntry, int *isFilterEntry,
                 int *endOfFilterChain, Tcl_Command *currentCmd) {
  XOTclClasses *pl = 0;
  int endOfChain = 0;

  *endOfFilterChain = 0;

  /*
   *  Next in filters
   */
  /*assert(obj->flags & XOTCL_FILTER_ORDER_VALID);   *** strange, worked before ****/

  FilterComputeDefined(interp, obj);

  if ((obj->flags & XOTCL_FILTER_ORDER_VALID) &&
      obj->filterStack &&
      obj->filterStack->currentCmdPtr) {
    *cmd = FilterSearchProc(interp, obj, currentCmd, cl);
    /*fprintf(stderr,"EndOfChain? proc=%p, cmd=%p\n",*proc,*cmd);*/
    /*  XOTclCallStackDump(interp); XOTclStackDump(interp);*/

    if (*cmd == 0) {
      if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
        /* reset the information to the values of method, cl
           to the values they had before calling the filters */
        *method = ObjStr(obj->filterStack->calledProc);
        endOfChain = 1;
        *endOfFilterChain = 1;
        *cl = 0;
        /*fprintf(stderr,"EndOfChain resetting cl\n");*/
      }
    } else {
      *method = (char *) Tcl_GetCommandName(interp, *cmd);
      *isFilterEntry = 1;
      return;
    }
  }

  /*
   *  Next in Mixins
   */
  assert(obj->flags & XOTCL_MIXIN_ORDER_VALID);
  /* otherwise: MixinComputeDefined(interp, obj); */

  /*fprintf(stderr,"nextsearch: mixinorder valid %d stack=%p\n",
    obj->flags & XOTCL_MIXIN_ORDER_VALID,  obj->mixinStack);*/

  if ((obj->flags & XOTCL_MIXIN_ORDER_VALID) &&  obj->mixinStack) {
    *cmd = MixinSearchProc(interp, obj, *method, cl, currentCmd);
    /*fprintf(stderr,"nextsearch: mixinsearch cmd %p, currentCmd %p\n",*cmd, *currentCmd);*/
    if (*cmd == 0) {
      if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_MIXIN) {
        endOfChain = 1;
        *cl = 0;
      }
    } else {
      *isMixinEntry = 1;
      return;
    }
  }

  /*
   * otherwise: normal method dispatch
   *
   * if we are already in the precedence ordering, then advance
   * past our last point; otherwise (if cl==0) begin from the start
   */

  /* if a mixin or filter chain has ended -> we have to search
     the obj-specific methods as well */

  if (obj->nsPtr && endOfChain) {
    *cmd = FindMethod(*method, obj->nsPtr);
  } else {
    *cmd = 0;
  }


  if (!*cmd) {
    for (pl = ComputeOrder(obj->cl, obj->cl->order, Super); pl && *cl; pl = pl->nextPtr) {
      if (pl->cl == *cl)
        *cl = 0;
    }

    /*
     * search for a further class method
     */
    *cl = SearchPLMethod(pl, *method, cmd);
    /*fprintf(stderr, "no cmd, cl = %p %s\n",*cl, className((*cl)));*/
  } else {
    *cl = 0;
  }

  return;
}

static int
XOTclNextMethod(XOTclObject *obj, Tcl_Interp *interp, XOTclClass *givenCl,
                char *givenMethod, int objc, Tcl_Obj *CONST objv[],
                int useCallstackObjs, XOTclCallStackContent *csc) {
  Tcl_Command cmd, currentCmd = NULL;
  int result = TCL_OK,
    frameType = XOTCL_CSC_TYPE_PLAIN,
    isMixinEntry = 0, isFilterEntry = 0,
    endOfFilterChain = 0, decrObjv0 = 0;
  int nobjc; Tcl_Obj **nobjv;
  XOTclClass **cl = &givenCl;
  char **methodName = &givenMethod;
  TclCallFrame *framePtr = NULL;

  if (!csc) {
    csc = CallStackGetTopFrame(interp, &framePtr);
  } else {
    /* 
     * csc was given (i.e. it is not yet on the stack. So we cannot 
     *  get objc from the associated stack frame
     */
    assert(useCallstackObjs == 0);
    /* fprintf(stderr, "XOTclNextMethod csc given, use %d, framePtr %p\n", useCallstackObjs, framePtr); */
  }

  /*fprintf(stderr,"XOTclNextMethod givenMethod = %s, csc = %p, useCallstackObj %d, objc %d cfp %p\n",
    givenMethod, csc, useCallstackObjs, objc, framePtr);*/

  /* if no args are given => use args from stack */
  if (objc < 2 && useCallstackObjs && framePtr) {
    nobjc = Tcl_CallFrame_objc(framePtr);
    nobjv = (Tcl_Obj **)Tcl_CallFrame_objv(framePtr);
  } else {
    nobjc = objc;
    nobjv = (Tcl_Obj **)objv;
    /* We do not want to have "next" as the procname, since this can
       lead to unwanted results e.g. in a forwarder using %proc. So, we
       replace the first word with the value from the callstack to be
       compatible with the case where next is called without args.
    */
    if (useCallstackObjs && framePtr) {
      nobjv[0] = Tcl_CallFrame_objv(framePtr)[0];
      INCR_REF_COUNT(nobjv[0]); /* we seem to need this here */
      decrObjv0 = 1;
    }
  }

  /*
   * Search the next method & compute its method data
   */
  NextSearchMethod(obj, interp, csc, cl, methodName, &cmd,
                   &isMixinEntry, &isFilterEntry, &endOfFilterChain, &currentCmd);

  /*
    fprintf(stderr, "NextSearchMethod -- RETURN: method=%s eoffc=%d,",
    *methodName, endOfFilterChain);

    if (obj)
    fprintf(stderr, " obj=%s,", objectName(obj));
    if ((*cl))
    fprintf(stderr, " cl=%s,", (*cl)->nsPtr->fullName);
    fprintf(stderr, " mixin=%d, filter=%d, proc=%p\n",
    isMixinEntry, isFilterEntry, proc);
  */

  Tcl_ResetResult(interp); /* needed for bytecode support */

  if (cmd) {
    /*
     * change mixin state
     */
    if (obj->mixinStack) {
      if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_MIXIN)
        csc->frameType = XOTCL_CSC_TYPE_INACTIVE_MIXIN;

      /* otherwise move the command pointer forward */
      if (isMixinEntry) {
        frameType = XOTCL_CSC_TYPE_ACTIVE_MIXIN;
        obj->mixinStack->currentCmdPtr = currentCmd;
      }
    }
    /*
     * change filter state
     */
    if (obj->filterStack) {
      if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
	/*fprintf(stderr,"next changes filter state\n");*/
        csc->frameType = XOTCL_CSC_TYPE_INACTIVE_FILTER;
      }

      /* otherwise move the command pointer forward */
      if (isFilterEntry) {
	/*fprintf(stderr,"next moves filter forward\n");*/
        frameType = XOTCL_CSC_TYPE_ACTIVE_FILTER;
        obj->filterStack->currentCmdPtr = currentCmd;
      }
    }

    /*
     * now actually call the "next" method
     */

    /* cut the flag, that no stdargs should be used, if it is there */
    if (nobjc > 1) {
      char *nobjv1 = ObjStr(nobjv[1]);
      if (nobjv1[0] == '-' && !strcmp(nobjv1, "--noArgs"))
        nobjc = 1;
    }
    csc->callType |= XOTCL_CSC_CALL_IS_NEXT;
    RUNTIME_STATE(interp)->unknown = 0;
    result = InvokeMethod((ClientData)obj, interp, nobjc, nobjv, cmd,
                          obj, *cl, *methodName, frameType);

    csc->callType &= ~XOTCL_CSC_CALL_IS_NEXT;

    if (csc->frameType == XOTCL_CSC_TYPE_INACTIVE_FILTER)
      csc->frameType = XOTCL_CSC_TYPE_ACTIVE_FILTER;
    else if (csc->frameType == XOTCL_CSC_TYPE_INACTIVE_MIXIN)
      csc->frameType = XOTCL_CSC_TYPE_ACTIVE_MIXIN;
  } else if (result == TCL_OK && endOfFilterChain) {
    /*fprintf(stderr,"setting unknown to 1\n");*/
    RUNTIME_STATE(interp)->unknown = 1;
  }

  if (decrObjv0) {
    INCR_REF_COUNT(nobjv[0]);
  }

  return result;
}

int
XOTclNextObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclCallStackContent *csc = CallStackGetTopFrame(interp, NULL);

  if (!csc->self)
    return XOTclVarErrMsg(interp, "next: can't find self", (char *) NULL);

  if (!csc->cmdPtr)
    return XOTclErrMsg(interp, "next: no executing proc", TCL_STATIC);

  return XOTclNextMethod(csc->self, interp, csc->cl,
                         (char *)Tcl_GetCommandName(interp, csc->cmdPtr),
                         objc, objv, 1, NULL);
}


int
XOTclQualifyObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  char *string;
  if (objc != 2)
    return XOTclVarErrMsg(interp, "wrong # of args for __qualify", (char *) NULL);

  string = ObjStr(objv[1]);
  if (!isAbsolutePath(string)) {
    Tcl_SetObjResult(interp, NameInNamespaceObj(interp, string, callingNameSpace(interp)));
  } else {
    Tcl_SetObjResult(interp, objv[1]);
  }
  return TCL_OK;
}

/*
 * "self" object command
 */

static int
FindSelfNext(Tcl_Interp *interp, XOTclObject *obj) {
  XOTclCallStackContent *csc = CallStackGetTopFrame(interp, NULL);
  Tcl_Command cmd, currentCmd = 0;
  int isMixinEntry = 0,
    isFilterEntry = 0,
    endOfFilterChain = 0;
  XOTclClass *cl = csc->cl;
  XOTclObject *o = csc->self;
  char *methodName;

  Tcl_ResetResult(interp);

  methodName = (char *)Tcl_GetCommandName(interp, csc->cmdPtr);
  if (!methodName)
    return TCL_OK;

  NextSearchMethod(o, interp, csc, &cl, &methodName, &cmd,
                   &isMixinEntry, &isFilterEntry, &endOfFilterChain, &currentCmd);

  if (cmd) {
    Tcl_SetObjResult(interp, getFullProcQualifier(interp, Tcl_GetCommandName(interp, cmd),
                                                  o, cl, cmd));
  }
  return TCL_OK;
}

static Tcl_Obj *
computeLevelObj(Tcl_Interp *interp, CallStackLevel level) {
  Tcl_CallFrame *framePtr;
  Tcl_Obj *resultObj;

  switch (level) {
  case CALLING_LEVEL: XOTclCallStackFindLastInvocation(interp, 1, &framePtr); break;
  case ACTIVE_LEVEL:  XOTclCallStackFindActiveFrame(interp,    1, &framePtr); break;
  default: framePtr = NULL;
  }

  if (framePtr) {
    /* the call was from an xotcl frame, return absolute frame number */
    char buffer[LONG_AS_STRING];
    int l;

    buffer[0] = '#';
    XOTcl_ltoa(buffer+1,(long)Tcl_CallFrame_level(framePtr), &l);
    /*fprintf(stderr,"*** framePtr=%p buffer %s\n", framePtr, buffer);*/
    resultObj = Tcl_NewStringObj(buffer, l+1);
  } else {
    /* If not called from an xotcl frame, return 1 as default */
    resultObj = Tcl_NewIntObj(1);
  }

  return resultObj;
}

static int
XOTclSelfSubCommand(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *option) {
  int rc = TCL_OK;
  int opt;
  XOTclCallStackContent *csc = NULL;

  static CONST char *opts[] = {
    "proc", "class",
    "activelevel", "args",
#if defined(ACTIVEMIXIN)
    "activemixin",
#endif
    "calledproc", "calledmethod",
    "calledclass", "callingproc",
    "callingclass", "callinglevel",
    "callingobject", "filterreg",
    "isnextcall", "next",
    NULL
  };

  enum selfOptionIdx {
    procIdx, classIdx,
    activelevelIdx, argsIdx,
#if defined(ACTIVEMIXIN)
    activemixinIdx,
#endif
    calledprocIdx, calledmethodIdx,
    calledclassIdx, callingprocIdx,
    callingclassIdx, callinglevelIdx,
    callingobjectIdx, filterregIdx,
    isnextcallIdx, nextIdx
  };

  assert(option);

  if (Tcl_GetIndexFromObj(interp, option, opts, "self option", 0, &opt) != TCL_OK) {
    return TCL_ERROR;
  }

  if (!obj && opt != callinglevelIdx) {
    return XOTclVarErrMsg(interp, "self: no current object", (char *) NULL);
  }

  switch (opt) {
  case procIdx: { /* proc subcommand */
    csc = CallStackGetTopFrame(interp, NULL);
    if (csc) {
      CONST char *procName = Tcl_GetCommandName(interp, csc->cmdPtr);
      Tcl_SetResult(interp, (char *)procName, TCL_VOLATILE);
    } else {
      return XOTclVarErrMsg(interp, "Can't find proc", (char *) NULL);
    }
    break;
  }

  case classIdx: { /* class subcommand */
    csc = CallStackGetTopFrame(interp, NULL);
    Tcl_SetObjResult(interp, csc->cl ? csc->cl->object.cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
    break;
  }

  case activelevelIdx: {
    Tcl_SetObjResult(interp, computeLevelObj(interp, ACTIVE_LEVEL));
    break;
  }

  case argsIdx: {
    int nobjc;
    Tcl_Obj **nobjv;
    Tcl_CallFrame *topFramePtr;

    CallStackGetTopFrame(interp, &topFramePtr);
    nobjc = Tcl_CallFrame_objc(topFramePtr);
    nobjv = (Tcl_Obj **)Tcl_CallFrame_objv(topFramePtr);
    Tcl_SetObjResult(interp, Tcl_NewListObj(nobjc-1, nobjv+1));
    break;
  }

#if defined(ACTIVEMIXIN)
  case activemixinIdx: {
    XOTclObject *o = NULL;
    if (RUNTIME_STATE(interp)->cmdPtr) {
      o = XOTclGetObjectFromCmdPtr(RUNTIME_STATE(interp)->cmdPtr);
    }
    Tcl_SetObjResult(interp, o ? o->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
    break;
  }
#endif
  case calledprocIdx:
  case calledmethodIdx: {
    csc = CallStackFindActiveFilter(interp);
    if (csc) {
      Tcl_SetObjResult(interp, csc->filterStackEntry->calledProc);
    } else {
      rc = XOTclVarErrMsg(interp, "self ", ObjStr(option),
			  " called from outside of a filter",
			  (char *) NULL);
    }
    break;
  }

  case calledclassIdx:
    Tcl_SetResult(interp, className(FindCalledClass(interp, obj)), TCL_VOLATILE);
    break;

  case callingprocIdx:
    csc = XOTclCallStackFindLastInvocation(interp, 1, NULL);
    Tcl_SetResult(interp, csc ? (char *)Tcl_GetCommandName(interp, csc->cmdPtr) : "",
		  TCL_VOLATILE);
    break;

  case callingclassIdx:
    csc = XOTclCallStackFindLastInvocation(interp, 1, NULL);
    Tcl_SetObjResult(interp, csc && csc->cl ? csc->cl->object.cmdName :
		     XOTclGlobalObjects[XOTE_EMPTY]);
    break;

  case callinglevelIdx:
    if (!obj) {
      Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    } else {
      Tcl_SetObjResult(interp, computeLevelObj(interp, CALLING_LEVEL));
    }
    break;

  case callingobjectIdx:
    csc = XOTclCallStackFindLastInvocation(interp, 1, NULL);
    Tcl_SetObjResult(interp, csc ? csc->self->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
    break;

  case filterregIdx:
    csc = CallStackFindActiveFilter(interp);
    if (csc) {
      Tcl_SetObjResult(interp, FilterFindReg(interp, obj, csc->cmdPtr));
    } else {
      rc = XOTclVarErrMsg(interp,
                          "self filterreg called from outside of a filter",
                          (char *) NULL);
    }
    break;

  case isnextcallIdx: {
    Tcl_CallFrame *framePtr;
    csc = CallStackGetTopFrame(interp, &framePtr);
#if defined(TCL85STACK)
    framePtr = nextFrameOfType(Tcl_CallFrame_callerPtr(framePtr), FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD);
    csc = framePtr ? Tcl_CallFrame_clientData(framePtr) : NULL;
#else
    csc--;
    if (csc <= RUNTIME_STATE(interp)->cs.content)
      csc = NULL;
#endif
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), 
                      (csc && (csc->callType & XOTCL_CSC_CALL_IS_NEXT)));
    break;
  }

  case nextIdx:
    rc = FindSelfNext(interp, obj);
    break;
  }

  return rc;
}

/*
  int
  XOTclKObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  if (objc < 2)
  return XOTclVarErrMsg(interp, "wrong # of args for K", (char *) NULL);

  Tcl_SetObjResult(interp, objv[1]);
  return TCL_OK;
  }
*/

static int GetInstvarsIntoCurrentScope(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);

static int
XOTclInstvarCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *obj = GetSelfObj(interp);
  if (!obj) 
    return XOTclVarErrMsg(interp, "instvar: no current object", (char *) NULL);
  return GetInstvarsIntoCurrentScope(interp, obj, objc, objv);
}

int
XOTclGetSelfObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *obj;
  int rc;

  if (objc > 2)
    return XOTclVarErrMsg(interp, "wrong # of args for self", (char *) NULL);

  obj = GetSelfObj(interp);

  /*fprintf(stderr,"getSelfObj returns %p\n", obj);XOTclCallStackDump(interp);*/

  if (objc == 1) {
    if (obj) {
      Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
      rc = TCL_OK;
    } else {
      return XOTclVarErrMsg(interp, "self: no current object", (char *) NULL);
    }
    Tcl_SetObjResult(interp, obj->cmdName);
  } else {
    return XOTclSelfSubCommand(interp, obj, objv[1]);
  }
  return rc;
}


/*
 * object creation & destruction
 */

static int
unsetInAllNamespaces(Tcl_Interp *interp, Namespace *nsPtr, CONST char *name) {
  int rc = 0;
  fprintf(stderr, "### unsetInAllNamespaces variable '%s', current namespace '%s'\n",
          name, nsPtr ? nsPtr->fullName : "NULL");

  if (nsPtr) {
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr = Tcl_FirstHashEntry(&nsPtr->childTable, &search);
    Tcl_Var *varPtr;
    int result;

    varPtr = (Tcl_Var *) Tcl_FindNamespaceVar(interp, name, (Tcl_Namespace *) nsPtr, 0);
    /*fprintf(stderr, "found %s in %s -> %p\n", name, nsPtr->fullName, varPtr);*/
    if (varPtr) {
      Tcl_DString dFullname, *dsPtr = &dFullname;
      Tcl_DStringInit(dsPtr);
      Tcl_DStringAppend(dsPtr, "unset ", -1);
      Tcl_DStringAppend(dsPtr, nsPtr->fullName, -1);
      Tcl_DStringAppend(dsPtr, "::", 2);
      Tcl_DStringAppend(dsPtr, name, -1);
      /*rc = Tcl_UnsetVar2(interp, Tcl_DStringValue(dsPtr), NULL, TCL_LEAVE_ERR_MSG);*/
      result = Tcl_Eval(interp, Tcl_DStringValue(dsPtr));
      /* fprintf(stderr, "fqName = '%s' unset => %d %d\n", Tcl_DStringValue(dsPtr), rc, TCL_OK);*/
      if (result == TCL_OK) {
        rc = 1;
      } else {
        Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
        fprintf(stderr, "   err = '%s'\n", ObjStr(resultObj));
      }
      Tcl_DStringFree(dsPtr);
    }

    while (rc == 0 && entryPtr) {
      Namespace *childNsPtr = (Namespace *) Tcl_GetHashValue(entryPtr);
      /*fprintf(stderr, "child = %s\n", childNsPtr->fullName);*/
      entryPtr = Tcl_NextHashEntry(&search);
      rc |= unsetInAllNamespaces(interp, childNsPtr, name);
    }
  }
  return rc;
}

static int
freeUnsetTraceVariable(Tcl_Interp *interp, XOTclObject *obj) {
  int rc = TCL_OK;
  if (obj->opt && obj->opt->volatileVarName) {
    /*
      Somebody destroys a volatile object manually while
      the vartrace is still active. Destroying the object will
      be a problem in case the variable is deleted later
      and fires the trace. So, we unset the variable here
      which will cause a destroy via var trace, which in
      turn clears the volatileVarName flag.
    */
    /*fprintf(stderr,"### freeUnsetTraceVariable %s\n", obj->opt->volatileVarName);*/

    rc = Tcl_UnsetVar2(interp, obj->opt->volatileVarName, NULL, 0);
    if (rc != TCL_OK) {
      int rc = Tcl_UnsetVar2(interp, obj->opt->volatileVarName, NULL, TCL_GLOBAL_ONLY);
      if (rc != TCL_OK) {
        Namespace *nsPtr = (Namespace *) Tcl_GetCurrentNamespace(interp);
        if (unsetInAllNamespaces(interp, nsPtr, obj->opt->volatileVarName) == 0) {
          fprintf(stderr, "### don't know how to delete variable '%s' of volatile object\n",
                  obj->opt->volatileVarName);
	}
      }
    }
    if (rc == TCL_OK) {
      /*fprintf(stderr, "### success unset\n");*/
    }
  }
  return  rc;
}

static char *
XOTclUnsetTrace(ClientData clientData, Tcl_Interp *interp, CONST char *name, CONST char *name2, int flags)
{
  Tcl_Obj *obj = (Tcl_Obj *)clientData;
  XOTclObject *o;
  char *result = NULL;

  /*fprintf(stderr,"XOTclUnsetTrace %s flags %x %x\n", name, flags,
    flags & TCL_INTERP_DESTROYED);  */

  if ((flags & TCL_INTERP_DESTROYED) == 0) {
    if (GetObjectFromObj(interp, obj, &o) == TCL_OK) {
      Tcl_Obj *res = Tcl_GetObjResult(interp); /* save the result */
      INCR_REF_COUNT(res);

      /* clear variable, destroy is called from trace */
      if (o->opt && o->opt->volatileVarName) {
        o->opt->volatileVarName = NULL;
      }

      if (callDestroyMethod(interp, o, 0) != TCL_OK) {
        result = "Destroy for volatile object failed";
      } else
        result = "No XOTcl Object passed";

      Tcl_SetObjResult(interp, res);  /* restore the result */
      DECR_REF_COUNT(res);
    }
    DECR_REF_COUNT(obj);
  } else {
    /*fprintf(stderr, "omitting destroy on %s %p\n", name);*/
  }
  return result;
}

/*
 * bring an object into a state, as after initialization
 */
static void
CleanupDestroyObject(Tcl_Interp *interp, XOTclObject *obj, int softrecreate) {

  /* remove the instance, but not for ::Class/::Object */
  if ((obj->flags & XOTCL_IS_ROOT_CLASS) == 0 &&
      (obj->flags & XOTCL_IS_ROOT_META_CLASS) == 0 ) {

    if (!softrecreate) {
      (void)RemoveInstance(obj, obj->cl);
    }
  }

  if (obj->nsPtr) {
    NSCleanupNamespace(interp, obj->nsPtr);
    NSDeleteChildren(interp, obj->nsPtr);
  }

  if (obj->varTable) {
    TclDeleteVars(((Interp *)interp), obj->varTable);
    ckfree((char *)obj->varTable);
    /*FREE(obj->varTable, obj->varTable);*/
    obj->varTable = 0;
  }

  if (obj->opt) {
    XOTclObjectOpt *opt = obj->opt;
    AssertionRemoveStore(opt->assertions);
    opt->assertions = NULL;

#ifdef XOTCL_METADATA
    XOTclMetaDataDestroy(obj);
#endif

    if (!softrecreate) {
      /*
       *  Remove this object from all per object mixin lists and clear the mixin list
       */
      removeFromObjectMixinsOf(obj->id, opt->mixins);

      CmdListRemoveList(&opt->mixins, GuardDel);
      CmdListRemoveList(&opt->filters, GuardDel);

      FREE(XOTclObjectOpt, opt);
      opt = obj->opt = 0;
    }
  }

  obj->flags &= ~XOTCL_MIXIN_ORDER_VALID;
  if (obj->mixinOrder)  MixinResetOrder(obj);
  obj->flags &= ~XOTCL_FILTER_ORDER_VALID;
  if (obj->filterOrder) FilterResetOrder(obj);
}

/*
 * do obj initialization & namespace creation
 */
static void
CleanupInitObject(Tcl_Interp *interp, XOTclObject *obj,
                  XOTclClass *cl, Tcl_Namespace *namespacePtr, int softrecreate) {
#ifdef OBJDELETION_TRACE
  fprintf(stderr,"+++ CleanupInitObject\n");
#endif
  obj->teardown = interp;
  obj->nsPtr = namespacePtr;
  if (!softrecreate) {
    AddInstance(obj, cl);
  }
  if (obj->flags & XOTCL_RECREATE) {
    obj->opt = 0;
    obj->varTable = 0;
    obj->mixinOrder = 0;
    obj->filterOrder = 0;
    obj->flags = 0;
  }
  /*
    fprintf(stderr, "cleanupInitObject %s: %p cl = %p\n",
    obj->cmdName ? objectName(obj) : "", obj, obj->cl);*/
}

static void
PrimitiveDestroy(ClientData clientData) {
  XOTclObject *obj = (XOTclObject*)clientData;

  if (XOTclObjectIsClass(obj))
    PrimitiveCDestroy((ClientData) obj);
  else
    PrimitiveODestroy((ClientData) obj);
}

static void
tclDeletesObject(ClientData clientData) {
  XOTclObject *obj = (XOTclObject*)clientData;
  Tcl_Interp *interp;

#ifdef OBJDELETION_TRACE
  fprintf(stderr,"tclDeletesObject %p obj->id %p\n",obj, obj->id);
#endif
  if ((obj->flags & XOTCL_DURING_DELETE) || !obj->teardown) return;
  interp = obj->teardown;
# ifdef OBJDELETION_TRACE
  fprintf(stderr,"... %p %s\n",obj,objectName(obj));
# endif
  CallStackDestroyObject(interp,obj);
}

/*
 * physical object destroy
 */
static void
PrimitiveODestroy(ClientData clientData) {
  XOTclObject *obj = (XOTclObject*)clientData;
  Tcl_Interp *interp;

  /* fprintf(stderr, "****** PrimitiveODestroy %p\n", obj);*/
  assert(obj && !(obj->flags & XOTCL_DELETED));

  /*
   * check and latch against recurrent calls with obj->teardown
   */
  PRINTOBJ("PrimitiveODestroy", obj);

  if (!obj || !obj->teardown) return;
  interp = obj->teardown;

  /*
   * Don't destroy, if the interpreter is destroyed already
   * e.g. TK calls Tcl_DeleteInterp directly, if the window is killed
   */
  if (Tcl_InterpDeleted(interp)) return;
  /*
   * call and latch user destroy with obj->id if we haven't
   */
  if (!(obj->flags & XOTCL_DESTROY_CALLED)) {
    fprintf(stderr, "--- final chance to call destroy ******* NEVER CALLED\n");
    callDestroyMethod(interp, obj, 0);
  }

#ifdef OBJDELETION_TRACE
  fprintf(stderr,"  physical delete of %p id=%p destroyCalled=%d '%s'\n",
          obj, obj->id, (obj->flags & XOTCL_DESTROY_CALLED), objectName(obj));
#endif
  CleanupDestroyObject(interp, obj, 0);

  while (obj->mixinStack)
    MixinStackPop(obj);

  while (obj->filterStack)
    FilterStackPop(obj);

  obj->teardown = NULL;

  if (obj->nsPtr) {
    /*fprintf(stderr,"primitive odestroy calls deletenamespace for obj %p\n", obj);*/
    XOTcl_DeleteNamespace(interp, obj->nsPtr);
    obj->nsPtr = NULL;
  }

  /*fprintf(stderr, " +++ OBJ/CLS free: %s\n", objectName(obj));*/

  obj->flags |= XOTCL_DELETED;
  objTrace("ODestroy", obj);

  DECR_REF_COUNT(obj->cmdName);
  XOTclCleanupObject(obj);

#if !defined(NDEBUG)
  if (obj != (XOTclObject*)RUNTIME_STATE(interp)->theClass)
    checkAllInstances(interp, RUNTIME_STATE(interp)->theClass, 0);
#endif
}

/*
 * reset the object to a fresh, undestroyed state
 */
static void
MarkUndestroyed(XOTclObject *obj) {
  obj->flags &= ~XOTCL_DESTROY_CALLED;
}

static void
PrimitiveOInit(void *mem, Tcl_Interp *interp, char *name, XOTclClass *cl) {
  XOTclObject *obj = (XOTclObject*)mem;
  Tcl_Namespace *nsPtr = NULL;

#ifdef OBJDELETION_TRACE
  fprintf(stderr,"+++ PrimitiveOInit\n");
#endif

#ifdef XOTCLOBJ_TRACE
  fprintf(stderr, "OINIT %s = %p\n", name, obj);
#endif
  XOTclObjectRefCountIncr(obj);
  MarkUndestroyed(obj);

  if (Tcl_FindNamespace(interp, name, NULL, 0)) {
    nsPtr = NSGetFreshNamespace(interp, (ClientData)obj, name);
  }
  CleanupInitObject(interp, obj, cl, nsPtr, 0);

  /*obj->flags = XOTCL_MIXIN_ORDER_VALID | XOTCL_FILTER_ORDER_VALID;*/
  obj->mixinStack = NULL;
  obj->filterStack = NULL;
}

/*
 * Object creation: create object name (full name) and Tcl command
 */
static XOTclObject*
PrimitiveOCreate(Tcl_Interp *interp, char *name, XOTclClass *cl) {
  XOTclObject *obj = (XOTclObject*)ckalloc(sizeof(XOTclObject));
  unsigned length;

#if defined(XOTCLOBJ_TRACE)
  fprintf(stderr, "CKALLOC Object %p %s\n", obj, name);
#endif
#ifdef OBJDELETION_TRACE
  fprintf(stderr,"+++ PrimitiveOCreate\n");
#endif

  memset(obj, 0, sizeof(XOTclObject));
  MEM_COUNT_ALLOC("XOTclObject/XOTclClass", obj);
  assert(obj); /* ckalloc panics, if malloc fails */
  assert(isAbsolutePath(name));

  length = strlen(name);
  if (!NSCheckForParent(interp, name, length, cl)) {
    ckfree((char *) obj);
    return 0;
  }
  obj->id = Tcl_CreateObjCommand(interp, name, XOTclObjDispatch,
                                 (ClientData)obj, tclDeletesObject);

  PrimitiveOInit(obj, interp, name, cl);

  obj->cmdName = Tcl_NewStringObj(name, length);
  /* convert cmdName to Tcl Obj of type cmdName */
  Tcl_GetCommandFromObj(interp, obj->cmdName);

  INCR_REF_COUNT(obj->cmdName);

  objTrace("PrimitiveOCreate", obj);

  return obj;
}

#if 0
static int duringBootstrap(Tcl_Interp *interp) {
  Tcl_Obj *bootstrap = Tcl_GetVar2Ex(interp, "::xotcl::bootstrap", NULL, TCL_GLOBAL_ONLY);
  return (bootstrap != NULL);
}
#endif

static XOTclClass *
DefaultSuperClass(Tcl_Interp *interp, XOTclClass *cl, XOTclClass *mcl, int isMeta) {
  XOTclClass *defaultClass = NULL;

  /*fprintf(stderr, "DefaultSuperClass cl %s, mcl %s, isMeta %d\n",
          className(cl), className(mcl), isMeta );*/

  if (mcl) {
    int result;
    result = setInstVar(interp, (XOTclObject *)mcl, isMeta ?
                        XOTclGlobalObjects[XOTE_DEFAULTMETACLASS] :
                        XOTclGlobalObjects[XOTE_DEFAULTSUPERCLASS], NULL);

    if (result == TCL_OK) {
      Tcl_Obj *nameObj = Tcl_GetObjResult(interp);
      if (GetClassFromObj(interp, nameObj, &defaultClass, 0) != TCL_OK) {
	XOTclErrMsg(interp, "default superclass is not a class", TCL_STATIC);
      }
      /* fprintf(stderr, "DefaultSuperClass got from var %s\n",ObjStr(nameObj));*/

    } else {
      XOTclClass *result;
      XOTclClasses *sc;
      /* check superclasses of metaclass */
      /*fprintf(stderr,"DefaultSuperClass: search in superclasses starting with %p\n",cl->super);*/
      for (sc = mcl->super; sc && sc->cl != cl; sc = sc->nextPtr) {
	/*fprintf(stderr, "  ... check ismeta %d %s root mcl %d root cl %d\n",
	  isMeta, className(sc->cl),
	  sc->cl->object.flags & XOTCL_IS_ROOT_META_CLASS,
	  sc->cl->object.flags & XOTCL_IS_ROOT_CLASS);*/
	if (isMeta) {
	  if (sc->cl->object.flags & XOTCL_IS_ROOT_META_CLASS) {
	    return sc->cl;
	  }
	} else {
	  if (sc->cl->object.flags & XOTCL_IS_ROOT_CLASS) {
	    return sc->cl;
	  }
	}
	result = DefaultSuperClass(interp, cl, sc->cl, isMeta);
	if (result) {
	  return result;
	}
      }
    }
  } else {
    /* during bootstrapping, there might be no meta class defined yet */
    /*fprintf(stderr, "no meta class ismeta %d %s root mcl %d root cl %d\n",
                  isMeta, className(cl),
                  cl->object.flags & XOTCL_IS_ROOT_META_CLASS,
                  cl->object.flags & XOTCL_IS_ROOT_CLASS
                  );*/
    return NULL;
  }
  return defaultClass;
}

/*
 * Cleanup class: remove filters, mixins, assertions, instances ...
 * and remove class from class hierarchy
 */
static void
CleanupDestroyClass(Tcl_Interp *interp, XOTclClass *cl, int softrecreate, int recreate) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  XOTclClassOpt *clopt = cl->opt;
  XOTclClass *defaultClass = NULL;

  PRINTOBJ("CleanupDestroyClass", (XOTclObject *)cl);
  assert(softrecreate? recreate == 1 : 1);

  /*fprintf(stderr, "CleanupDestroyClass softrecreate=%d,recreate=%d, %p\n",
    softrecreate,recreate,clopt); */

  /* do this even with no clopt, since the class might be used as a
     superclass of a per object mixin, so it has no clopt...
  */
  MixinInvalidateObjOrders(interp, cl);
  FilterInvalidateObjOrders(interp, cl);

  /* todo: maybe not needed, of done by MixinInvalidateObjOrders() already */
  XOTclCInvalidateInterfaceDefinitionMethod(interp, cl);

  if (clopt) {
    /*
     *  Remove this class from all isClassMixinOf lists and clear the instmixin list
     */
    RemoveFromClassMixinsOf(clopt->id, clopt->instmixins);

    CmdListRemoveList(&clopt->instmixins, GuardDel);
    /*MixinInvalidateObjOrders(interp, cl);*/

    CmdListRemoveList(&clopt->instfilters, GuardDel);
    /*FilterInvalidateObjOrders(interp, cl);*/

    if (!recreate) {
      /*
       *  Remove this class from all mixin lists and clear the isObjectMixinOf list
       */

      RemoveFromMixins(clopt->id, clopt->isObjectMixinOf);
      CmdListRemoveList(&clopt->isObjectMixinOf, GuardDel);

      /*
       *  Remove this class from all instmixin lists and clear the isClassMixinOf list
       */

      RemoveFromInstmixins(clopt->id, clopt->isClassMixinOf);
      CmdListRemoveList(&clopt->isClassMixinOf, GuardDel);
    }
    /* remove dependent filters of this class from all subclasses*/
    FilterRemoveDependentFilterCmds(cl, cl);
    AssertionRemoveStore(clopt->assertions);
    clopt->assertions = NULL;
#ifdef XOTCL_OBJECTDATA
    XOTclFreeObjectData(cl);
#endif
  }

  Tcl_ForgetImport(interp, cl->nsPtr, "*"); /* don't destroy namespace imported objects */
  NSCleanupNamespace(interp, cl->nsPtr);
  NSDeleteChildren(interp, cl->nsPtr);

  /*fprintf(stderr, "    CleanupDestroyClass softrecreate %d\n",softrecreate);*/

  if (!softrecreate) {
    defaultClass = DefaultSuperClass(interp, cl, cl->object.cl, 0);

    /* Reclass all instances of the current class the the appropriate
       most general class ("baseClass"). The most general class of a
       metaclass is ::xotcl::Class, the most general class of an
       object is ::xotcl::Object. Instances of metaclasses can be only
       reset to ::xotcl::Class (and not to ::xotcl::Object as in
       earlier versions), since otherwise their instances can't be
       deleted, because ::xotcl::Object has no method "dealloc".

       We do not have to reclassing in case, cl == ::xotcl::Object
    */
    if ((cl->object.flags & XOTCL_IS_ROOT_CLASS) == 0) {
      XOTclClass *baseClass = IsMetaClass(interp, cl, 1) ?
        DefaultSuperClass(interp, cl, cl->object.cl, 1)
        : defaultClass;

      /*fprintf(stderr,"    baseclass = %s\n",className(baseClass));*/

      hPtr = &cl->instances ? Tcl_FirstHashEntry(&cl->instances, &hSrch) : 0;
      for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
        XOTclObject *inst = (XOTclObject*)Tcl_GetHashKey(&cl->instances, hPtr);
        /*fprintf(stderr, "    inst %p %s flags %.6x id %p\n",inst,objectName(inst),inst->flags,inst->id);*/
        if (inst && inst != (XOTclObject*)cl && !(inst->flags & XOTCL_DURING_DELETE) /*inst->id*/) {
          if (inst != &(baseClass->object)) {
            (void)RemoveInstance(inst, cl->object.cl);
            AddInstance(inst, baseClass);
          }
        }
      }
    }
    Tcl_DeleteHashTable(&cl->instances);
    MEM_COUNT_FREE("Tcl_InitHashTable",&cl->instances);
  }

  if ((clopt) && (!recreate)) {
    FREE(XOTclClassOpt, clopt);
    clopt = cl->opt = 0;
  }

  /* On a recreate, it might be possible that the newly created class
     has a different superclass. So we have to flush the precedence list
     on a recreate as well.
  */
  FlushPrecedencesOnSubclasses(cl);
  while (cl->super) (void)RemoveSuper(cl, cl->super->cl);

  if (!softrecreate) {
    /*
     * flush all caches, unlink superclasses
     */

    while (cl->sub) {
      XOTclClass *subClass = cl->sub->cl;
      (void)RemoveSuper(subClass, cl);
      /* if there are no more super classes add the Object
       * class as superclasses
       * -> don't do that for Object itself!
       */
      if (subClass->super == 0 && (cl->object.flags & XOTCL_IS_ROOT_CLASS) == 0)
        AddSuper(subClass, defaultClass);
    }
    /*(void)RemoveSuper(cl, cl->super->cl);*/
  }

}

/*
 * do class initialization & namespace creation
 */
static void
CleanupInitClass(Tcl_Interp *interp, XOTclClass *cl, Tcl_Namespace *namespacePtr,
                 int softrecreate, int recreate) {
  XOTclObject *obj = (XOTclObject*)cl;
  XOTclClass *defaultSuperclass;

  assert(softrecreate? recreate == 1 : 1);

#ifdef OBJDELETION_TRACE
  fprintf(stderr,"+++ CleanupInitClass\n");
#endif

  /*
   * during init of Object and Class the theClass value is not set
   */
  /*
    if (RUNTIME_STATE(interp)->theClass != 0)
    obj->type = RUNTIME_STATE(interp)->theClass;
  */
  XOTclObjectSetClass(obj);

  cl->nsPtr = namespacePtr;

  if (!softrecreate) {
    /* subclasses are preserved during recreate, superclasses not (since
       the creation statement defined the superclass, might be different
       the second time)
    */
    cl->sub = NULL;
  }
  cl->super = NULL;

  /* Look for a configured default superclass */
  defaultSuperclass = DefaultSuperClass(interp, cl, cl->object.cl, 0);
    /*
      if (defaultSuperclass) {
      fprintf(stderr, "default superclass= %s\n", className(defaultSuperclass));
      } else {
      fprintf(stderr, "empty super class\n");
      }*/

  AddSuper(cl, defaultSuperclass);
  cl->color = WHITE;
  cl->order = NULL;

  if (!softrecreate) {
    Tcl_InitHashTable(&cl->instances, TCL_ONE_WORD_KEYS);
    MEM_COUNT_ALLOC("Tcl_InitHashTable",&cl->instances);
  }

  if (!recreate) {
    cl->opt = NULL;
  }
}

/*
 * class physical destruction
 */
static void
PrimitiveCDestroy(ClientData clientData) {
  XOTclClass *cl = (XOTclClass*)clientData;
  XOTclObject *obj = (XOTclObject*)clientData;
  Tcl_Interp *interp;
  Tcl_Namespace *saved;

  PRINTOBJ("PrimitiveCDestroy", obj);

  /*
   * check and latch against recurrent calls with obj->teardown
   */
  if (!obj || !obj->teardown) return;
  interp = obj->teardown;

  /*
   * Don't destroy, if the interpreted is destroyed already
   * e.g. TK calls Tcl_DeleteInterp directly, if Window is killed
   */
  if (Tcl_InterpDeleted(interp)) return;

  /*
   * call and latch user destroy with obj->id if we haven't
   */
  /*fprintf(stderr,"PrimitiveCDestroy %s flags %x\n", objectName(obj), obj->flags);*/

  if (!(obj->flags & XOTCL_DESTROY_CALLED))
    fprintf(stderr,"???? PrimitiveCDestroy call destroy\n");
    callDestroyMethod(interp, obj, 0);

  obj->teardown = 0;

  CleanupDestroyClass(interp, cl, 0, 0);

  /*
   * handoff the primitive teardown
   */

  saved = cl->nsPtr;
  obj->teardown = interp;

  /*
   * class object destroy + physical destroy
   */
  /* fprintf(stderr,"primitive cdestroy calls primitive odestroy\n");*/
  PrimitiveODestroy(clientData);

  /*fprintf(stderr,"primitive cdestroy calls deletenamespace for obj %p\n", cl);*/
  saved->clientData = NULL;
  XOTcl_DeleteNamespace(interp, saved);

  return;
}

/*
 * class init
 */
static void
PrimitiveCInit(XOTclClass *cl, Tcl_Interp *interp, char *name) {
  TclCallFrame frame, *framePtr = &frame;
  Tcl_Namespace *nsPtr;

  /*
   * ensure that namespace is newly created during CleanupInitClass
   * ie. kill it, if it exists already
   */
  if (Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr,
                        RUNTIME_STATE(interp)->XOTclClassesNS, 0) != TCL_OK)
    return;
  nsPtr = NSGetFreshNamespace(interp, (ClientData)cl, name);
  Tcl_PopCallFrame(interp);

  CleanupInitClass(interp, cl, nsPtr, 0, 0);
  return;
}

/*
 * class create: creation of namespace + class full name
 * calls class object creation
 */
static XOTclClass*
PrimitiveCCreate(Tcl_Interp *interp, char *name, XOTclClass *class) {
  XOTclClass *cl = (XOTclClass*)ckalloc(sizeof(XOTclClass));
  unsigned length;
  XOTclObject *obj = (XOTclObject*)cl;

  /*fprintf(stderr, "CKALLOC Class %p %s\n", cl, name);*/

  memset(cl, 0, sizeof(XOTclClass));
  MEM_COUNT_ALLOC("XOTclObject/XOTclClass", cl);
  /*
    fprintf(stderr, " +++ CLS alloc: %s\n", name);
  */
  assert(isAbsolutePath(name));
  length = strlen(name);
  /*
    fprintf(stderr,"Class alloc %p '%s'\n", cl, name);
  */
  /* check whether Object parent NS already exists,
     otherwise: error */
  if (!NSCheckForParent(interp, name, length, class)) {
    ckfree((char *) cl);
    return 0;
  }
  obj->id = Tcl_CreateObjCommand(interp, name, XOTclObjDispatch,
                                 (ClientData)cl, tclDeletesObject);

  PrimitiveOInit(obj, interp, name, class);

  obj->cmdName = Tcl_NewStringObj(name, length);
  /* convert cmdName to Tcl Obj of type cmdName */
  Tcl_GetCommandFromObj(interp,obj->cmdName);

  INCR_REF_COUNT(obj->cmdName);
  PrimitiveCInit(cl, interp, name+2);

  objTrace("PrimitiveCCreate", obj);
  return cl;
}

/* change XOTcl class conditionally; obj must not be NULL */

XOTCLINLINE static int
changeClass(Tcl_Interp *interp, XOTclObject *obj, XOTclClass *cl) {
  assert(obj);

  /*fprintf(stderr,"changing %s to class %s ismeta %d\n",
          objectName(obj),
          className(cl),
          IsMetaClass(interp, cl, 1));*/

  if (cl != obj->cl) {
    if (IsMetaClass(interp, cl, 1)) {
      /* Do not allow upgrading from a class to a meta-class (in
	 other words, don't make an object to a class). To allow
	 this, it would be necessary to reallocate the base
	 structures.
      */
      if (!IsMetaClass(interp, obj->cl, 1)) {
	return XOTclVarErrMsg(interp, "cannot turn object into a class",
			      (char *) NULL);
      }
    } else {
      /* The target class is not a meta class. Changing meta-class to
	 meta-class, or class to class, or object to object is fine,
	 but upgrading/downgrading is not allowed */

      /*fprintf(stderr,"target class %s not a meta class, am i a class %d\n",
	className(cl),
	XOTclObjectIsClass(obj) );*/

      if (XOTclObjectIsClass(obj)) {
	return XOTclVarErrMsg(interp, "cannot turn class into an object ",
			      (char *) NULL);
      }
    }
    (void)RemoveInstance(obj, obj->cl);
    AddInstance(obj, cl);

    MixinComputeDefined(interp, obj);
    FilterComputeDefined(interp, obj);
  }
  return TCL_OK;
}


/*
 * Undestroy the object, reclass it, and call "cleanup" afterwards
 */
static int
doCleanup(Tcl_Interp *interp, XOTclObject *newObj, XOTclObject *classobj,
          int objc, Tcl_Obj *CONST objv[]) {
  int result;

  /*
   * Check whether we have a pending destroy on the object; if yes, clear it, 
   * such that the recreated object and won't be destroyed on a POP
   */
  MarkUndestroyed(newObj);

  /*
   *  re-create, first ensure correct class for newObj
   */
  result = changeClass(interp, newObj, (XOTclClass*) classobj);

  if (result == TCL_OK) {
    /*
     * dispatch "cleanup"
     */
    result = callMethod((ClientData) newObj, interp, XOTclGlobalObjects[XOTE_CLEANUP], 2, 0, 0);
  }
  return result;
}

/*
 * Std object initialization:
 *   call parameter default values
 *   apply "-" methods (call "configure" with given arguments)
 *   call constructor "init", if it was not called before
 */
static int
doObjInitialization(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]) {
  int result;
  Tcl_Obj *savedObjResult = Tcl_GetObjResult(interp); /* save the result */
  INCR_REF_COUNT(savedObjResult);

  /*
   * clear INIT_CALLED flag
   */
  obj->flags &= ~XOTCL_INIT_CALLED;

  /*
   * call configure methods (starting with '-')
   */
  result = callMethod((ClientData) obj, interp,
                      XOTclGlobalObjects[XOTE_CONFIGURE], objc, objv+2, 0);

  if (result != TCL_OK) {
    goto objinitexit;
  }

  /*
   * check, whether init was called already
   */
  if (!(obj->flags & XOTCL_INIT_CALLED)) {
    int nobjc = 0;
    Tcl_Obj **nobjv, *resultObj = Tcl_GetObjResult(interp);

    /*
     * Call the user-defined constructor 'init' and pass the result of
     * configure to it as arguments
     */
    INCR_REF_COUNT(resultObj);
    Tcl_ListObjGetElements(interp, resultObj, &nobjc, &nobjv);
    result = callMethod((ClientData) obj, interp, XOTclGlobalObjects[XOTE_INIT],
			nobjc+2, nobjv, 0);
    obj->flags |= XOTCL_INIT_CALLED;
    DECR_REF_COUNT(resultObj);
  }

  if (result == TCL_OK) {
    Tcl_SetObjResult(interp, savedObjResult);
  }
 objinitexit:
  DECR_REF_COUNT(savedObjResult);
  return result;
}


/*
 * experimental resolver implementation -> not used at the moment
 */
#ifdef EXPERIMENTAL_CMD_RESOLVER
static int
XOTclResolveCmd(Tcl_Interp *interp, char *name, Tcl_Namespace *contextNsPtr,
                int flags, Tcl_Command *rPtr) {

  Tcl_Namespace *nsPtr[2], *cxtNsPtr;
  char *simpleName;
  register Tcl_HashEntry *entryPtr;
  register Tcl_Command cmd;
  register int search;

  /*fprintf(stderr, "  ***%s->%s\n", contextNsPtr->fullName, name);*/

  /*
   * Find the namespace(s) that contain the command.
   */
  if (flags & TCL_GLOBAL_ONLY) {
    cxtNsPtr = Tcl_GetGlobalNamespace(interp);
  }
  else if (contextNsPtr) {
    cxtNsPtr = contextNsPtr;
  }
  else {
    cxtNsPtr = Tcl_GetCurrentNamespace(interp);
  }

  TclGetNamespaceForQualName(interp, name, (Namespace *) contextNsPtr,flags,
                             &nsPtr[0], &nsPtr[1], &cxtNsPtr, &simpleName);

  /*fprintf(stderr, "  ***Found %s, %s\n", nsPtr[0]->fullName, nsPtr[0]->fullName);*/

  /*
   * Look for the command in the command table of its namespace.
   * Be sure to check both possible search paths: from the specified
   * namespace context and from the global namespace.
   */

  cmd = NULL;
  for (search = 0;  (search < 2) && (cmd == NULL);  search++) {
    if (nsPtr[search] && simpleName) {
      cmdTable = Tcl_Namespace_cmdTable(nsPtr[search]);
      entryPtr = XOTcl_FindHashEntry(cmdTable, simpleName);
      if (entryPtr) {
        cmd = (Tcl_Command) Tcl_GetHashValue(entryPtr);
      }
    }
  }
  if (cmd) {
    Tcl_ObjCmdProc *objProc = Tcl_Command_objProc(cmd);
    if (NSisXOTclNamespace(cxtNsPtr) &&
        objProc != XOTclObjDispatch &&
        objProc != XOTclNextObjCmd &&
        objProc != XOTclGetSelfObjCmd) {

      /*
       * the cmd is defined in an XOTcl object or class namespace, but
       * not an object & not self/next -> redispatch in
       * global namespace
       */
      cmd = 0;
      nsPtr[0] = Tcl_GetGlobalNamespace(interp);
      if (nsPtr[0] && simpleName) {
        cmdTable = Tcl_Namespace_cmdTable(nsPtr[0]);
        if ((entryPtr = XOTcl_FindHashEntry(cmdTable, simpleName))) {
          cmd = (Tcl_Command) Tcl_GetHashValue(entryPtr);
        }
      }

      /*
        XOTclStackDump(interp);
        XOTclCallStackDump(interp);
      */
    }
    *rPtr = cmd;
    return TCL_OK;
  }

  return TCL_CONTINUE;
}
static int
XOTclResolveVar(Tcl_Interp *interp, char *name, Tcl_Namespace *context,
                Tcl_ResolvedVarInfo *rPtr) {
  /*fprintf(stderr, "Resolving %s in %s\n", name, context->fullName);*/

  return TCL_CONTINUE;
}
#endif



static int
hasMetaProperty(Tcl_Interp *interp, XOTclClass *cl) {
  return cl->object.flags & XOTCL_IS_ROOT_META_CLASS;
}

static int
IsMetaClass(Tcl_Interp *interp, XOTclClass *cl, int withMixins) {
  /* check if class is a meta-class */
  XOTclClasses *pl, *checkList = NULL, *mixinClasses = NULL, *mc;
  int hasMCM = 0;

  /* is the class the most general meta-class? */
  if (hasMetaProperty(interp, cl))
    return 1;

  /* is the class a subclass of a meta-class? */
  for (pl = ComputeOrder(cl, cl->order, Super); pl; pl = pl->nextPtr) {
    if (hasMetaProperty(interp, pl->cl))
      return 1;
  }

  if (withMixins) {
    /* has the class metaclass mixed in? */
    for (pl = ComputeOrder(cl, cl->order, Super); pl; pl = pl->nextPtr) {
      XOTclClassOpt *clopt = pl->cl->opt;
      if (clopt && clopt->instmixins) {
	MixinComputeOrderFullList(interp,
				  &clopt->instmixins,
				  &mixinClasses,
				  &checkList, 0);
      }
    }

    /* TODO: should be a class of isMetaClass, or? */
    for (mc=mixinClasses; mc; mc = mc->nextPtr) {
      /*fprintf(stderr,"- got %s\n", className(mc->cl));*/
      /*if (isSubType(mc->cl, RUNTIME_STATE(interp)->theClass)) {*/
      if (IsMetaClass(interp, mc->cl, 0)) {
	hasMCM = 1;
	break;
      }
    }
    XOTclClassListFree(mixinClasses);
    XOTclClassListFree(checkList);
    /*fprintf(stderr,"has MC returns %d, mixinClasses = %p\n",
      hasMCM, mixinClasses);*/
  }

  return hasMCM;
}

static int
isSubType(XOTclClass *subcl, XOTclClass *cl) {
  XOTclClasses *t;
  int success = 1;
  assert(cl && subcl);

  if (cl != subcl) {
    success = 0;
    for (t = ComputeOrder(subcl, subcl->order, Super); t && t->cl; t = t->nextPtr) {
      if (t->cl == cl) {
        success = 1;
        break;
      }
    }
  }
  return success;
}

static int
XOTclIsCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *obj = NULL;
  XOTclClass *cl = NULL;
  int success = 0, opt;

  static CONST char *opts[] = {
    "type", "object", "class", "metaclass", "mixin",
    NULL
  };
  enum subCmdIdx {
    typeIdx, objectIdx, classIdx, metaclassIdx, mixinIdx
  };

  if (Tcl_GetIndexFromObj(interp, objv[2], opts, "option", 0, &opt) != TCL_OK) {
    return TCL_ERROR;
  }

  switch (opt) {
  case typeIdx:
    if (objc != 4) return XOTclObjErrArgCnt(interp, objv[0], NULL, "type <object> <type>");
    success = (GetObjectFromObj(interp, objv[1], &obj) == TCL_OK
	       && GetClassFromObj(interp, objv[3], &cl, 0) == TCL_OK
	       && isSubType(obj->cl, cl));
    break;

  case objectIdx:
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "object <object>");
    success = (GetObjectFromObj(interp, objv[1], &obj) == TCL_OK);
    break;

  case classIdx:
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "class <class>");
    success = (GetClassFromObj(interp, objv[1], &cl, 0) == TCL_OK);
    break;

  case metaclassIdx:
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "metaclass <class>");

    success = (GetObjectFromObj(interp, objv[1], &obj) == TCL_OK
	       && XOTclObjectIsClass(obj)
	       && IsMetaClass(interp, (XOTclClass*)obj, 1));
    break;

  case mixinIdx:
    if (objc != 4) return XOTclObjErrArgCnt(interp, objv[0], NULL, "mixin <object> <class>");

    success = (GetObjectFromObj(interp, objv[1], &obj) == TCL_OK
	       && GetClassFromObj(interp, objv[3], &cl, 0) == TCL_OK
	       && hasMixin(interp, obj, cl));
    break;
  }

  Tcl_SetIntObj(Tcl_GetObjResult(interp), success);
  return TCL_OK;
}


static int
hasMixin(Tcl_Interp *interp, XOTclObject *obj, XOTclClass *cl) {

  if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, obj);

  if ((obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID)) {
    XOTclCmdList *ml;
    for (ml = obj->mixinOrder; ml; ml = ml->nextPtr) {
      XOTclClass *mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin == cl) {
        return 1;
      }
    }
  }
  return 0;
}


extern int
XOTclCreateObject(Tcl_Interp *interp, Tcl_Obj *name, XOTcl_Class *class) {
  XOTclClass *cl = (XOTclClass*) class;
  int result;

  INCR_REF_COUNT(name);
  result = XOTclCallMethodWithArgs((ClientData)cl, interp,
                                   XOTclGlobalObjects[XOTE_CREATE], name, 1, 0, 0);
  DECR_REF_COUNT(name);
  return result;
}

extern int
XOTclCreateClass(Tcl_Interp *interp, Tcl_Obj *name, XOTcl_Class *class) {
  XOTclClass *cl = (XOTclClass *) class;
  int result;

  INCR_REF_COUNT(name);
  result = XOTclCallMethodWithArgs((ClientData)cl, interp,
                                   XOTclGlobalObjects[XOTE_CREATE], name, 1, 0, 0);
  DECR_REF_COUNT(name);
  return result;
}

extern int
XOTclCreate(Tcl_Interp *interp, XOTcl_Class *class, Tcl_Obj *name, ClientData clientData,
            int objc, Tcl_Obj *CONST objv[]) {
  XOTclClass *cl = (XOTclClass *) class;
  int result;

  INCR_REF_COUNT(name);

  ALLOC_ON_STACK(Tcl_Obj *, objc+2, ov);
  ov[0] = NULL;
  ov[1] = name;
  if (objc>0) {
    memcpy(ov+2, objv, sizeof(Tcl_Obj *)*objc);
  }
  result = createMethod(interp, cl, ObjStr(name), objc+2, ov);

  FREE_ON_STACK(ov);
  DECR_REF_COUNT(name);

  return result;
}

int
XOTclDeleteObject(Tcl_Interp *interp, XOTcl_Object *obji) {
  XOTclObject *obj = (XOTclObject *) obji;
  return callDestroyMethod(interp, obj, 0);
}

int
XOTclDeleteClass(Tcl_Interp *interp, XOTcl_Class *cli) {
  XOTclObject *obj = (XOTclObject *) cli;
  return callDestroyMethod(interp, obj, 0);
}

extern int
XOTclUnsetInstVar2(XOTcl_Object *obji, Tcl_Interp *interp, char *name1, char *name2,
                   int flgs) {
  XOTclObject *obj = (XOTclObject *) obji;
  int result;
  XOTcl_FrameDecls;

  XOTcl_PushFrame(interp, obj);
  if (obj->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_UnsetVar2(interp, name1, name2, flgs);
  XOTcl_PopFrame(interp, obj);
  return result;
}


static int
GetInstVarIntoCurrentScope(Tcl_Interp *interp, XOTclObject *obj,
                           Tcl_Obj *varName, Tcl_Obj *newName) {
  Var *varPtr = NULL, *otherPtr = NULL, *arrayPtr;
  int new, flgs = TCL_LEAVE_ERR_MSG;
  Tcl_CallFrame *varFramePtr;
  TclVarHashTable *tablePtr;
  XOTcl_FrameDecls;

  XOTcl_PushFrame(interp, obj);
  if (obj->nsPtr) {
    flgs = flgs|TCL_NAMESPACE_ONLY;
  }

  otherPtr = XOTclObjLookupVar(interp, varName, NULL, flgs, "define",
                               /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
  XOTcl_PopFrame(interp, obj);

  if (otherPtr == NULL) {
    return XOTclVarErrMsg(interp, "can't make instvar ", ObjStr(varName),
                          ": can't find variable on ", objectName(obj),
			  (char *) NULL);
  }
  /*
   * if newName == NULL -> there is no alias, use varName
   * as target link name
   */
  if (newName == NULL) {
    /*
     * Variable link into namespace cannot be an element in an array.
     * see Tcl_VariableObjCmd ...
     */
    if (arrayPtr) {
      return XOTclVarErrMsg(interp, "can't make instvar ", ObjStr(varName),
                            " on ", objectName(obj),
                            ": variable cannot be an element in an array;",
                            " use an alias or objeval.", (char *) NULL);
    }

    newName = varName;
  }
#if 0
  if (strstr(newName, "::")) {
    return XOTclVarErrMsg(interp, "variable name \"", newName,
                          "\" illegal: must not contain namespace separator",
                          (char *) NULL);
  }
#endif
  varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  /*
   * If we are executing inside a Tcl procedure, create a local
   * variable linked to the new namespace variable "varName".
   */
  if (varFramePtr && Tcl_CallFrame_isProcCallFrame(varFramePtr)) {
    Proc *procPtr           = Tcl_CallFrame_procPtr(varFramePtr);
    int localCt             = procPtr->numCompiledLocals;
    CompiledLocal *localPtr = procPtr->firstLocalPtr;
    Var *localVarPtr        = Tcl_CallFrame_compiledLocals(varFramePtr);
    char *newNameString     = ObjStr(newName);
    int i, nameLen          = strlen(newNameString);
    
    for (i = 0;  i < localCt;  i++) {    /* look in compiled locals */
      /* fprintf(stderr,"%d of %d %s flags %x not isTemp %d\n", i, localCt,
         localPtr->name, localPtr->flags,
         !TclIsCompiledLocalTemporary(localPtr));*/
      
      if (!TclIsCompiledLocalTemporary(localPtr)) {
        char *localName = localPtr->name;
        if ((newNameString[0] == localName[0])
            && (nameLen == localPtr->nameLength)
            && (strcmp(newNameString, localName) == 0)) {
          varPtr = getNthVar(localVarPtr, i);
          new = 0;
          /*fprintf(stderr, "var in locals: %s\n",newNameString);*/
          break;
        }
      }
      localPtr = localPtr->nextPtr;
    }
    
    if (varPtr == NULL) {	/* look in frame's local var hashtable */
      tablePtr = Tcl_CallFrame_varTablePtr(varFramePtr);
      if (tablePtr == NULL) {
        tablePtr = (TclVarHashTable *) ckalloc(varHashTableSize);
        InitVarHashTable(tablePtr, NULL);
        Tcl_CallFrame_varTablePtr(varFramePtr) = tablePtr;
      }
      varPtr = VarHashCreateVar(tablePtr, newName, &new);
    }
    /*
     * if we define an alias (newName != varName), be sure that
     * the target does not exist already
     */
    if (!new) {
      /*fprintf(stderr,"GetIntoScope createalias\n");*/
      if (varPtr == otherPtr)
        return XOTclVarErrMsg(interp, "can't instvar to variable itself",
                              (char *) NULL);
      
      if (TclIsVarLink(varPtr)) {
        /* we try to make the same instvar again ... this is ok */
        Var *linkPtr = valueOfVar(Var, varPtr, linkPtr);
        if (linkPtr == otherPtr) {
          return TCL_OK;
        }
        
        /*fprintf(stderr, "linkvar flags=%x\n", linkPtr->flags);
          Tcl_Panic("new linkvar %s... When does this happen?", newNameString, NULL);*/
        
        /* We have already a variable with the same name imported
           from a different object. Get rid of this old variable
      */
        VarHashRefCount(linkPtr)--;
        if (TclIsVarUndefined(linkPtr)) {
          CleanupVar(linkPtr, (Var *) NULL);
        }
        
      } else if (!TclIsVarUndefined(varPtr)) {
        return XOTclVarErrMsg(interp, "variable '", ObjStr(newName),
                              "' exists already", (char *) NULL);
      } else if (TclIsVarTraced(varPtr)) {
        return XOTclVarErrMsg(interp, "variable '", ObjStr(newName),
                              "' has traces: can't use for instvar", (char *) NULL);
      }
    }
    
    TclSetVarLink(varPtr);
    TclClearVarUndefined(varPtr);
#if FORWARD_COMPATIBLE
    if (forwardCompatibleMode) {
      Var85 *vPtr = (Var85 *)varPtr;
      vPtr->value.linkPtr = (Var85 *)otherPtr;
    } else {
      varPtr->value.linkPtr = otherPtr;
    }
#else
    varPtr->value.linkPtr = otherPtr;
#endif
    VarHashRefCount(otherPtr)++;
    
    /*
      {
      Var85 *p = (Var85 *)varPtr;
      fprintf(stderr,"defining an alias var='%s' in obj %s fwd %d flags %x isLink %d isTraced %d isUndefined %d\n",
      ObjStr(newName), objectName(obj), forwardCompatibleMode,
      varFlags(varPtr),
      TclIsVarLink(varPtr), TclIsVarTraced(varPtr), TclIsVarUndefined(varPtr));
      }
    */
  }
  return TCL_OK;
}

static int XOTclOInstVarMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);

extern int
XOTclInstVar(XOTcl_Object *obji, Tcl_Interp *interp, char *name, char *destName) {
  XOTclObject *obj = (XOTclObject*) obji;
  int result;
  Tcl_Obj *alias = NULL;
  ALLOC_ON_STACK(Tcl_Obj*, 2, objv);

  objv[0] = XOTclGlobalObjects[XOTE_INSTVAR];
  objv[1] = Tcl_NewStringObj(name, -1);
  INCR_REF_COUNT(objv[1]);

  if (destName) {
    alias = Tcl_NewStringObj(destName, -1);
    INCR_REF_COUNT(alias);
    Tcl_ListObjAppendElement(interp, objv[1], alias);
  }

  result = XOTclOInstVarMethod(interp, obj, 2, objv);

  if (destName) {
    DECR_REF_COUNT(alias);
  }
  DECR_REF_COUNT(objv[1]);
  FREE_ON_STACK(objv);
  return result;
}

extern int
XOTclRemovePMethod(Tcl_Interp *interp, XOTcl_Object *obji, char *name) {
  XOTclObject *obj = (XOTclObject*) obji;

  if (obj->opt)
    AssertionRemoveProc(obj->opt->assertions, name);

  if (obj->nsPtr) {
    int rc = NSDeleteCmd(interp, obj->nsPtr, name);
    if (rc < 0)
      return XOTclVarErrMsg(interp, objectName(obj), " cannot delete method '", name,
                            "' of object ", objectName(obj), (char *) NULL);
  }
  return TCL_OK;
}

extern int
XOTclRemoveIMethod(Tcl_Interp *interp, XOTcl_Class *cli, char *name) {
  XOTclClass *cl = (XOTclClass*) cli;
  XOTclClassOpt *opt = cl->opt;
  int rc;

  if (opt && opt->assertions)
    AssertionRemoveProc(opt->assertions, name);

  rc = NSDeleteCmd(interp, cl->nsPtr, name);
  if (rc < 0)
    return XOTclVarErrMsg(interp, className(cl), " cannot delete method '", name,
                          "' of class ", className(cl), (char *) NULL);
  return TCL_OK;
}

/*
 * obj/cl ClientData setter/getter
 */
extern void
XOTclSetObjClientData(XOTcl_Object *obji, ClientData data) {
  XOTclObject *obj = (XOTclObject*) obji;
  XOTclObjectOpt *opt = XOTclRequireObjectOpt(obj);
  opt->clientData = data;
}
extern ClientData
XOTclGetObjClientData(XOTcl_Object *obji) {
  XOTclObject *obj = (XOTclObject*) obji;
  return (obj && obj->opt) ? obj->opt->clientData : 0;
}
extern void
XOTclSetClassClientData(XOTcl_Class *cli, ClientData data) {
  XOTclClass *cl = (XOTclClass*) cli;
  XOTclRequireClassOpt(cl);
  cl->opt->clientData = data;
}
extern ClientData
XOTclGetClassClientData(XOTcl_Class *cli) {
  XOTclClass *cl = (XOTclClass*) cli;
  return (cl && cl->opt) ? cl->opt->clientData : 0;
}

static int
setInstVar(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *name, Tcl_Obj *value) {
  Tcl_Obj *result;
  int flags = (obj->nsPtr) ? TCL_LEAVE_ERR_MSG|TCL_NAMESPACE_ONLY : TCL_LEAVE_ERR_MSG;
  XOTcl_FrameDecls;
  XOTcl_PushFrame(interp, obj);

  if (value == NULL) {
    result = Tcl_ObjGetVar2(interp, name, NULL, flags);
  } else {
    /*fprintf(stderr,"setvar in obj %s: name %s = %s\n",objectName(obj),ObjStr(name),ObjStr(value));*/
    result = Tcl_ObjSetVar2(interp, name, NULL, value, flags);
  }
  XOTcl_PopFrame(interp, obj);

  if (result) {
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
  }
  return TCL_ERROR;
}

static int
XOTclSetterMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *obj = (XOTclObject*)clientData;

  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (objc > 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "?value?");
  return setInstVar(interp, obj, objv[0], objc == 2 ? objv[1] : NULL);
}

static int
forwardArg(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
           Tcl_Obj *o, forwardCmdClientData *tcd, Tcl_Obj **out,
           Tcl_Obj **freeList, int *inputarg, int *mapvalue) {
  char *element = ObjStr(o), *p;
  int totalargs = objc + tcd->nr_args - 1;
  char c = *element, c1;

  p = element;

  if (c == '%' && *(element+1) == '@') {
    char *remainder = NULL;
    long pos;
    element += 2;
    pos = strtol(element,&remainder, 0);
    /*fprintf(stderr,"strtol('%s) returned %ld '%s'\n", element, pos, remainder);*/
    if (element == remainder && *element == 'e' && !strncmp(element,"end", 3)) {
      pos = -1;
      remainder += 3;
    } else if (pos < 0) {
      pos --;
    }
    if (element == remainder || abs(pos) > totalargs) {
      return XOTclVarErrMsg(interp, "forward: invalid index specified in argument ",
                            ObjStr(o), (char *) NULL);
    }    if (!remainder || *remainder != ' ') {
      return XOTclVarErrMsg(interp, "forward: invaild syntax in '",  ObjStr(o),
                            "' use: %@<pos> <cmd>",(char *) NULL);
    }

    element = ++remainder;
    /* in case we address from the end, we reduct further to distinguish from -1 (void) */
    if (pos<0) pos--;
    /*fprintf(stderr,"remainder = '%s' pos = %ld\n", remainder, pos);*/
    *mapvalue = pos;
    element = remainder;
    c = *element;
  }
  /*fprintf(stderr,"c==%c element = '%s'\n", c, element);*/
  if (c == '%') {
    Tcl_Obj *list = NULL, **listElements;
    int nrargs = objc-1, nrElements = 0;
    c = *++element;
    c1 = *(element+1);

    if (c == 's' && !strcmp(element,"self")) {
      *out = tcd->obj->cmdName;
    } else if (c == 'p' && !strcmp(element,"proc")) {
      *out = objv[0];
    } else if (c == '1' && (c1 == '\0' || c1 == ' ')) {
      /*fprintf(stderr, "   nrargs=%d, subcommands=%d inputarg=%d, objc=%d\n",
	nrargs, tcd->nr_subcommands, *inputarg, objc);*/
      if (c1 != '\0') {
        if (Tcl_ListObjIndex(interp, o, 1, &list) != TCL_OK) {
          return XOTclVarErrMsg(interp, "forward: %1 must by a valid list, given: '",
                                ObjStr(o), "'", (char *) NULL);
        }
        if (Tcl_ListObjGetElements(interp, list, &nrElements, &listElements) != TCL_OK) {
          return XOTclVarErrMsg(interp, "forward: %1 contains invalid list '",
                                ObjStr(list),"'", (char *) NULL);
        }
      } else if (tcd->subcommands) { /* deprecated part */
        if (Tcl_ListObjGetElements(interp, tcd->subcommands,&nrElements,&listElements) != TCL_OK) {
          return XOTclVarErrMsg(interp, "forward: %1 contains invalid list '",
                                ObjStr(list),"'", (char *) NULL);
        }
      }
      if (nrElements > nrargs) {
        /* insert default subcommand depending on number of arguments */
        *out = listElements[nrargs];
      } else if (objc<=1) {
	return XOTclObjErrArgCnt(interp, objv[0], NULL, "option");
      } else {
        *out = objv[1];
        *inputarg = 2;
      }
    } else if (c == 'a' && !strncmp(element,"argcl", 4)) {
      if (Tcl_ListObjIndex(interp, o, 1, &list) != TCL_OK) {
        return XOTclVarErrMsg(interp, "forward: %argclindex must by a valid list, given: '",
                              element, "'", (char *) NULL);
      }
      if (Tcl_ListObjGetElements(interp, list, &nrElements, &listElements) != TCL_OK) {
        return XOTclVarErrMsg(interp, "forward: %argclindex contains invalid list '",
                              ObjStr(list),"'", (char *) NULL);
      }
      if (nrargs >= nrElements) {
        return XOTclVarErrMsg(interp, "forward: not enough elements in specified list of ARGC argument ",
                              element, (char *) NULL);
      }
      *out = listElements[nrargs];
    } else if (c == '%') {
      Tcl_Obj *newarg = Tcl_NewStringObj(element,-1);
      *out = newarg;
      goto add_to_freelist;
    } else {
      /* evaluating given command */
      int result;
      /*fprintf(stderr,"evaluating '%s'\n", element);*/
      if ((result = Tcl_EvalEx(interp, element, -1, 0)) != TCL_OK)
        return result;
      *out = Tcl_DuplicateObj(Tcl_GetObjResult(interp));
      /*fprintf(stderr,"result = '%s'\n", ObjStr(*out));*/
      goto add_to_freelist;
    }
  } else {
    if (p == element)
      *out = o;
    else {
      Tcl_Obj *newarg = Tcl_NewStringObj(element,-1);
      *out = newarg;
      goto add_to_freelist;
    }
  }
  return TCL_OK;

 add_to_freelist:
  if (!*freeList) {
    *freeList = Tcl_NewListObj(1, out);
    INCR_REF_COUNT(*freeList);
  } else
    Tcl_ListObjAppendElement(interp, *freeList, *out);
  return TCL_OK;
}


static int
callForwarder(forwardCmdClientData *tcd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ClientData clientData;
  int result;
  XOTclObject *obj = tcd->obj;
  XOTcl_FrameDecls;

  if (tcd->verbose) {
    Tcl_Obj *cmd = Tcl_NewListObj(objc, objv);
    fprintf(stderr,"calling %s\n", ObjStr(cmd));
    DECR_REF_COUNT(cmd);
  }
  if (tcd->objscope) {
    XOTcl_PushFrame(interp, obj);
  }
  if (tcd->objProc) {
    result = (tcd->objProc)(tcd->clientData, interp, objc, objv);
  } else if (IsXOTclTclObj(interp, tcd->cmdName, (XOTclObject**)&clientData)) {
    /*fprintf(stderr, "XOTcl object %s, objc=%d\n", ObjStr(tcd->cmdName), objc);*/
    result = XOTclObjDispatch(clientData, interp, objc, objv);
  } else {
    /*fprintf(stderr, "callForwarder: no XOTcl object %s\n", ObjStr(tcd->cmdName));*/
    result = Tcl_EvalObjv(interp, objc, objv, 0);
  }

  if (tcd->objscope) {
    XOTcl_PopFrame(interp, obj);
  }
  if (result == TCL_ERROR && tcd && tcd->onerror) {
    Tcl_Obj *ov[2];
    ov[0] = tcd->onerror;
    ov[1] = Tcl_GetObjResult(interp);
    INCR_REF_COUNT(ov[1]);
    /*Tcl_EvalObjEx(interp, tcd->onerror, TCL_EVAL_DIRECT);*/
    Tcl_EvalObjv(interp, 2, ov, 0);
    DECR_REF_COUNT(ov[1]);
  }
  return result;
}

static int
XOTclForwardMethod(ClientData clientData, Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[]) {
  forwardCmdClientData *tcd = (forwardCmdClientData *)clientData;
  int result, j, inputarg = 1, outputarg = 0;
#if defined(TCL85STACK)
  /* no need to store varFramePtr in call frame for tcl85stack */
#else
  XOTclCallStackContent *csc = CallStackGetTopFrame(interp, NULL);
  csc->currentFramePtr = (Tcl_CallFrame *) Tcl_Interp_varFramePtr(interp);

  /*fprintf(stderr,"...setting currentFramePtr %p to %p (ForwardMethod)\n",
    RUNTIME_STATE(interp)->cs.top->currentFramePtr, (Tcl_CallFrame *) Tcl_Interp_varFramePtr(interp)); 
  */
#endif

  if (!tcd || !tcd->obj) return XOTclObjErrType(interp, objv[0], "Object");

  if (tcd->passthrough) { /* two short cuts for simple cases */
    /* early binding, cmd *resolved, we have to care only for objscope */
    return callForwarder(tcd, interp, objc, objv);
  } else if (!tcd->args && *(ObjStr(tcd->cmdName)) != '%') {
    /* we have ony to replace the method name with the given cmd name */
    ALLOC_ON_STACK(Tcl_Obj*, objc, ov);
    /*fprintf(stderr,"+++ forwardMethod must subst \n");*/
    memcpy(ov, objv, sizeof(Tcl_Obj *)*objc);
    ov[0] = tcd->cmdName;
    result = callForwarder(tcd, interp, objc, ov);
    FREE_ON_STACK(ov);
    return result;
  } else {
    Tcl_Obj **ov, *freeList=NULL;
    int totalargs = objc + tcd->nr_args + 3;
    ALLOC_ON_STACK(Tcl_Obj*, totalargs, OV);
    ALLOC_ON_STACK(int, totalargs, objvmap);
    /*fprintf(stderr,"+++ forwardMethod standard case, allocated %d args\n",totalargs);*/

    ov = &OV[1];
    if (tcd->needobjmap) {
      memset(objvmap, -1, sizeof(int)*totalargs);
    }

#if 0
    memset(objvmap, -1, sizeof(int)*totalargs);
    fprintf(stderr,"command %s (%p) objc=%d, subcommand=%d, args=%p, nrargs\n",
            ObjStr(objv[0]), tcd, objc,
            tcd->nr_subcommands,
            tcd->args
            );
#endif

    /* the first argument is always the command, to which we forward */

    if ((result = forwardArg(interp, objc, objv, tcd->cmdName, tcd,
                             &ov[outputarg], &freeList, &inputarg,
                             &objvmap[outputarg])) != TCL_OK) {
      goto exitforwardmethod;
    }
    outputarg++;

    if (tcd->args) {
      /* copy argument list from definition */
      Tcl_Obj **listElements;
      int nrElements;
      Tcl_ListObjGetElements(interp, tcd->args, &nrElements, &listElements);

      for (j=0; j<nrElements; j++, outputarg++) {
        if ((result = forwardArg(interp, objc, objv, listElements[j], tcd,
                                 &ov[outputarg], &freeList, &inputarg,
                                 &objvmap[outputarg])) != TCL_OK) {
          goto exitforwardmethod;
        }
      }
    }
    
    /*fprintf(stderr, "objc=%d, tcd->nr_subcommands=%d size=%d\n",
      objc, tcd->nr_subcommands, objc+ 2	    );*/

    if (objc-inputarg>0) {
      /*fprintf(stderr, "  copying remaining %d args starting at [%d]\n",
        objc-inputarg, outputarg);*/
      memcpy(ov+outputarg, objv+inputarg, sizeof(Tcl_Obj *)*(objc-inputarg));
    } else {
      /*fprintf(stderr, "  nothing to copy, objc=%d, inputarg=%d\n", objc, inputarg);*/
    }
    if (tcd->needobjmap) {
      /* we have to set the adressing relative from the end; -2 means
	 last, -3 element before last, etc. */
      int max = objc + tcd->nr_args - inputarg;
      for (j=0; j<totalargs; j++) {
	if (objvmap[j] < -1) {
	  /*fprintf(stderr, "must reduct, v=%d\n",objvmap[j]);*/
	  objvmap[j] = max + objvmap[j] + 2; 
	  /*fprintf(stderr, "... new value=%d, max = %d\n",objvmap[j],max);*/
	}
      }
    }
    objc += outputarg - inputarg;

#if 0
    for(j=0; j<objc; j++) {
      /*fprintf(stderr, "  ov[%d]=%p, objc=%d\n", j, ov[j], objc);*/
      fprintf(stderr, " o[%d]=%p %s (%d),", j, ov[j], ov[j] ? ObjStr(ov[j]) : "NADA", objvmap[j]);
    }
    fprintf(stderr,"\n");
#endif

    if (tcd->needobjmap) {

      for (j=0; j<totalargs; j++) {
        Tcl_Obj *tmp;
        int pos = objvmap[j], i;
        if (pos == -1 || pos == j)
          continue;
        tmp = ov[j];
        if (j>pos) {
          for(i=j; i>pos; i--) {
            /*fprintf(stderr,"...moving right %d to %d\n", i-1, i);*/
            ov[i] = ov[i-1];
            objvmap[i] = objvmap[i-1];
          }
        } else {
          for(i=j; i<pos; i++) {
            /*fprintf(stderr,"...moving left %d to %d\n", i+1, i);*/
            ov[i] = ov[i+1];
            objvmap[i] = objvmap[i+1];
          }
        }
        /*fprintf(stderr,"...setting at %d -> %s\n", pos, ObjStr(tmp));*/
        ov[pos] = tmp;
        objvmap[pos] = -1;
      }
    }

    if (tcd->prefix) {
      /* prepend a prefix for the subcommands to avoid name clashes */
      Tcl_Obj *methodName = Tcl_DuplicateObj(tcd->prefix);
      Tcl_AppendObjToObj(methodName, ov[1]);
      ov[1] = methodName;
      INCR_REF_COUNT(ov[1]);
    }

#if 0
    for(j=0; j<objc; j++) {
      /*fprintf(stderr, "  ov[%d]=%p, objc=%d\n", j, ov[j], objc);*/
      fprintf(stderr, "  ov[%d]=%p '%s' map=%d\n", j, ov[j], ov[j] ? ObjStr(ov[j]) : "NADA", objvmap[j]);
    }
#endif

    OV[0] = tcd->cmdName;
    result = callForwarder(tcd, interp, objc, ov);

    if (tcd->prefix) {DECR_REF_COUNT(ov[1]);}
  exitforwardmethod:
    if (freeList)    {DECR_REF_COUNT(freeList);}
    FREE_ON_STACK(objvmap);
    FREE_ON_STACK(OV);
  }
  return result;
}

/*
 * copied from Tcl, since not exported
 */
static char *
VwaitVarProc(clientData, interp, name1, name2, flags)
     ClientData clientData;	/* Pointer to integer to set to 1. */
     Tcl_Interp *interp;		/* Interpreter containing variable. */
     char *name1;		/* Name of variable. */
     char *name2;		/* Second part of variable name. */
     int flags;			/* Information about what happened. */
{
  int *donePtr = (int *) clientData;

  *donePtr = 1;
  return (char *) NULL;
}

static int
XOTclObjscopedMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  aliasCmdClientData *tcd = (aliasCmdClientData *)clientData;
  XOTclObject *obj = tcd->obj;
  int rc;
  XOTcl_FrameDecls;
  /*fprintf(stderr,"objscopedMethod obj=%p, ptr=%p\n", obj, tcd->objProc);*/

  XOTcl_PushFrame(interp, obj);
  rc = (tcd->objProc)(tcd->clientData, interp, objc, objv);
  XOTcl_PopFrame(interp, obj);

  return rc;
}

static void aliasCmdDeleteProc(ClientData clientData) {
  aliasCmdClientData *tcd = (aliasCmdClientData *)clientData;
  if (tcd->cmdName)     {DECR_REF_COUNT(tcd->cmdName);}
  /*fprintf(stderr,"aliasCmdDeleteProc\n");*/
  FREE(aliasCmdClientData, tcd);
}

static int
XOTclDispatchCmd(ClientData clientData, Tcl_Interp *interp,
                 int objc, Tcl_Obj *CONST objv[]) {
  int result;
  char *method;
  XOTclObject *obj;
  register char *n;

  if (objc < 3) {
    return XOTclObjErrArgCnt(interp, objv[0], NULL, "<obj> <methodName> ?args?");
  }

  GetObjectFromObj(interp, objv[2], &obj);
  if (!obj)
    return XOTclObjErrType(interp, objv[2], "Class|Object");

  method = ObjStr(objv[1]);
  n = method + strlen(method);

  /*fprintf(stderr, "Dispatch obj=%s, o=%p cmd m='%s'\n",ObjStr(objv[2]),obj,method);*/

  /* if the specified method is a fully qualified cmd name like e.g.
     ::xotcl::cmd::Class::alloc, this method is called on the
     specified <Class|Object>, no matter whether it was registered on
     it */

  /*search for last '::'*/
  while ((*n != ':' || *(n-1) != ':') && n-1 > method) {n--; }
  if (*n == ':' && n > method && *(n-1) == ':') {n--;}

  if ((n-method)>1 || *method == ':') {
    Tcl_DString parentNSName, *dsp = &parentNSName;
    Tcl_Namespace *nsPtr;
    Tcl_Command cmd, importedCmd;
    char *parentName, *tail = n+2;
    DSTRING_INIT(dsp);

    if (n-method != 0) {
      Tcl_DStringAppend(dsp, method, (n-method));
      parentName = Tcl_DStringValue(dsp);
      nsPtr = Tcl_FindNamespace(interp, parentName, (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY);
      DSTRING_FREE(dsp);
    } else {
      nsPtr = Tcl_FindNamespace(interp, "::", (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY);
    }
    if (!nsPtr) {
      return XOTclVarErrMsg(interp, "cannot lookup parent namespace '",
			    method, "'", (char *) NULL);
    }
    fprintf(stderr, "    .... findmethod '%s' in %s\n",tail, nsPtr->fullName);
    cmd = FindMethod(tail, nsPtr);
    if (cmd && (importedCmd = TclGetOriginalCommand(cmd))) {
      cmd = importedCmd;
    }

    if (cmd == NULL) {
      return XOTclVarErrMsg(interp, "cannot lookup command '",
			    tail, "'", (char *) NULL);
    }

    result = InvokeMethod((ClientData)obj, interp,
                          objc-2, objv+2, cmd, obj,
                          NULL /*XOTclClass *cl*/, tail,
                          XOTCL_CSC_TYPE_PLAIN);
  } else {
    /* no colons, use method from dispatch order, with filters etc. -
       strictly speaking unneccessary, but can be used to invoke protected methods */
    int nobjc;
    Tcl_Obj *arg;
    Tcl_Obj *CONST *nobjv;

    if (objc >= 3) {
      arg = objv[3];
      nobjv = objv + 2;
    } else {
      arg = NULL;
      nobjv = NULL;
    }
    nobjc = objc-3;
    result = XOTclCallMethodWithArgs((ClientData)obj, interp, objv[1], arg,
				     nobjc, nobjv, XOTCL_CM_NO_UNKNOWN);
  }
  return result;
}

typedef enum {NO_DASH, SKALAR_DASH, LIST_DASH} dashArgType;

static dashArgType
isDashArg(Tcl_Interp *interp, Tcl_Obj *obj, char **methodName, int *objc, Tcl_Obj **objv[]) {
  char *flag;
  static Tcl_ObjType *listType = NULL;

  assert(obj);

  /* fetch list type, if not set already; if used on more places, this should
     be moved into the interpreter state
  */
  if (listType == NULL) {
    static XOTclMutex initMutex = 0;
    XOTclMutexLock(&initMutex);
    if (listType == NULL) {
      listType = Tcl_GetObjType("list");
      /*fprintf(stderr, "fetching listType=%p\n", listType);*/
    }
    XOTclMutexUnlock(&initMutex);
  }

  if (obj->typePtr == listType) {
    if (Tcl_ListObjGetElements(interp, obj, objc, objv) == TCL_OK && *objc>1) {
      flag = ObjStr(*objv[0]);
      /*fprintf(stderr, "we have a list starting with '%s'\n", flag);*/
      if (*flag == '-') {
        *methodName = flag+1;
        return LIST_DASH;
      }
    }
  }
  flag = ObjStr(obj);
  /*fprintf(stderr, "we have a scalar '%s'\n", flag);*/
  if (*flag == '-' && isalpha((int)*((flag)+1))) {
    *methodName = flag+1;
    *objc = 1;
    return SKALAR_DASH;
  }
  return NO_DASH;
}

static int
callConfigureMethod(Tcl_Interp *interp, XOTclObject *obj,
                    char *methodName, int argc, Tcl_Obj *CONST argv[]) {
  int result;
  Tcl_Obj *method = Tcl_NewStringObj(methodName,-1);

  /*fprintf(stderr,"callConfigureMethod method %s->'%s' argc %d\n",
    objectName(obj), methodName, argc);*/

  if (isInitString(methodName))
    obj->flags |= XOTCL_INIT_CALLED;

  Tcl_ResetResult(interp);
  INCR_REF_COUNT(method);
  result = callMethod((ClientData)obj, interp, method, argc, argv, XOTCL_CM_NO_UNKNOWN);
  DECR_REF_COUNT(method);

  /*fprintf(stderr, "method  '%s' called args: %d o=%p, result=%d %d\n",
    methodName, argc+1, obj, result, TCL_ERROR);*/

  if (result != TCL_OK) {
    Tcl_Obj *res =  Tcl_DuplicateObj(Tcl_GetObjResult(interp)); /* save the result */
    INCR_REF_COUNT(res);
    XOTclVarErrMsg(interp, ObjStr(res), " during '", objectName(obj), " ",
		   methodName, "'", (char *) NULL);
    DECR_REF_COUNT(res);
  }

  return result;
}


/*
 * class method implementations
 */

static Tcl_Namespace *
callingNameSpace(Tcl_Interp *interp) {
  Tcl_CallFrame *framePtr = nonXotclObjectProcFrame((Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp));
  Tcl_Namespace *nsPtr = Tcl_CallFrame_nsPtr(framePtr);

  /*fprintf(stderr," **** callingNameSpace\n");*/
  /* tcl85showStack(interp); */

  /* fprintf(stderr, "nonXotclObjectProcFrame returned %p frame %p, currentNs %p %s, xot %p %s\n",
     framePtr,Tcl_CallFrame_callerPtr(csc->currentFramePtr), nsPtr,nsPtr?nsPtr->fullName:NULL,
     RUNTIME_STATE(interp)->XOTclNS,RUNTIME_STATE(interp)->XOTclNS->fullName);
     tcl85showStack(interp);*/
  /*
  * Find last incovation outside the ::xotcl (system) namespace. For
  *  example, the pre defined slot handlers for relations (defined in
  *  the ::xotcl namespace) handle mixin and class
  *  registration. etc. If we would use this namespace, we would
  *  resolve non-fully-qualified names against ::xotcl).
  */
  while (nsPtr == RUNTIME_STATE(interp)->XOTclNS) {
    /*fprintf(stderr, "... ns %s\n",nsPtr->fullName);*/
    if (framePtr) {
      nsPtr = Tcl_CallFrame_nsPtr(framePtr);
      framePtr = nonXotclObjectProcFrame(Tcl_CallFrame_callerPtr(framePtr));
    } else {
      nsPtr = Tcl_GetGlobalNamespace(interp);
    }
  }
  
  /*fprintf(stderr," **** callingNameSpace: returns %p %s\n", nsPtr, nsPtr?nsPtr->fullName:"(null)");*/
  return nsPtr;
}

static int
createMethod(Tcl_Interp *interp, XOTclClass *cl, char *specifiedName, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *newObj = NULL;
  Tcl_Obj *nameObj, *tmpObj = NULL;
  int result;
  char *objName = specifiedName;

  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

  memcpy(tov, objv, sizeof(Tcl_Obj *)*(objc));
  /*
   * complete the name if it is not absolute
   */
  if (!isAbsolutePath(objName)) {
    tmpObj = NameInNamespaceObj(interp, objName, callingNameSpace(interp));
    objName = ObjStr(tmpObj);
    /*fprintf(stderr," **** fixed name is '%s'\n",  objName);*/

    INCR_REF_COUNT(tmpObj);
    tov[1] = tmpObj;
  }

  /*
   * Check whether we have to call recreate (i.e. when the
   * object exists already)
   */
  newObj = XOTclpGetObject(interp, objName);

  /*fprintf(stderr,"+++ createspecifiedName '%s', objName '%s', newObj=%p ismeta(%s) %d, ismeta(%s) %d\n",
          specifiedName, objName, newObj,
          className(cl), IsMetaClass(interp, cl, 1),
          newObj ? ObjStr(newobj->cl->object.cmdName) : "NULL",
          newObj ? IsMetaClass(interp, newObj->cl, 1) : 0
          );*/

  /* don't allow to
     - recreate an object as a class, and to
     - recreate a class as an object

     In these clases, we use destroy + create instead of recrate.
  */

  if (newObj && (IsMetaClass(interp, cl, 1) == IsMetaClass(interp, newObj->cl, 1))) {

    /* fprintf(stderr, "%%%% recreate, call recreate method ... %s, objc=%d\n",
       ObjStr(tov[1]), objc+1);*/

    /* call recreate --> initialization */
    result = callMethod((ClientData) cl, interp,
                        XOTclGlobalObjects[XOTE_RECREATE], objc+1, tov+1, 0);
    if (result != TCL_OK)
      goto create_method_exit;

    Tcl_SetObjResult(interp, newObj->cmdName);
    nameObj = newObj->cmdName;
    objTrace("RECREATE", newObj);

  } else {
    /*
     * newObj might exist here, but will be automatically destroyed by
     * alloc
     */

    /*fprintf(stderr, "alloc ... %s\n", ObjStr(tov[1]));*/

    result = callMethod((ClientData) cl, interp,
                        XOTclGlobalObjects[XOTE_ALLOC], 3, tov+1, 0);
    if (result != TCL_OK)
      goto create_method_exit;

    nameObj = Tcl_GetObjResult(interp);
    if (GetObjectFromObj(interp, nameObj, &newObj) != TCL_OK) {
      result = XOTclErrMsg(interp, "couldn't find result of alloc", TCL_STATIC);
      goto create_method_exit;
    }

    /*(void)RemoveInstance(newObj, newObj->cl);*/ /* TODO needed? remove? */
    AddInstance(newObj, cl);
    objTrace("CREATE", newObj);

    /* in case, the object is destroyed during initialization, we incr refcount */
    INCR_REF_COUNT(nameObj);
    result = doObjInitialization(interp, newObj, objc, objv);
    DECR_REF_COUNT(nameObj);
  }
 create_method_exit:

  /*fprintf(stderr, "create -- end ... %s => %d\n", ObjStr(tov[1]),result);*/
  if (tmpObj)  {DECR_REF_COUNT(tmpObj);}
  FREE_ON_STACK(tov);
  return result;
}

/***********************************
 * objv parser
 ***********************************/

#include "tclAPI.h"

static int
parseObjv(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], Tcl_Obj *procName,
          argDefinition CONST *ifdPtr, int ifdSize, parseContext *pc) {
  int i, o, args = 0, flagCount = 0, nrReq = 0, nrOpt = 0, varArgs = 0, dashdash = 0;
  /* todo benchmark with and without CONST */
  argDefinition CONST *aPtr, *bPtr;

  parseContextInit(pc, ifdSize, procName);

#if defined(PARSE_TRACE)
  fprintf(stderr, "BEGIN (%d) [0]%s ",objc, ObjStr(procName));
  for (o=1; o<objc; o++) {fprintf(stderr, "[%d]%s ",o,ObjStr(objv[o]));}
  fprintf(stderr,"\n");
#endif

  /*fprintf(stderr, "processing from %d to %d\n",1,objc-1);*/
  for (i=0, o=1, aPtr=ifdPtr; aPtr->name && o<objc;) {
#if defined(PARSE_TRACE_FULL)
    fprintf(stderr,"... (%d) processing [%d]: '%s' %s\n",i, o,aPtr->name,aPtr->flags & XOTCL_ARG_REQUIRED ? "req":"not req");
#endif
    if (*aPtr->name == '-') {
      /* the interface defintion has switches, which can be given in
         an arbitrary order */
      int p, found;
      char *objStr;
      for (p = o; p<objc; p++) {
        objStr = ObjStr(objv[p]);
        /*fprintf(stderr,"....checking objv[%d]=%s\n", p, objStr);*/
        if (objStr[0] == '-') {
          found = 0;
          for (bPtr = aPtr; bPtr->name && *bPtr->name == '-'; bPtr ++) {
            if (strcmp(objStr,bPtr->name) == 0) {
              /*fprintf(stderr, "...     flag '%s' o=%d p=%d, objc=%d nrargs %d\n",objStr,o,p,objc,bPtr->nrargs);*/
              if (bPtr->flags & XOTCL_ARG_REQUIRED) nrReq++; else nrOpt++;
              if (bPtr->nrargs == 0) {
                pc->clientData[bPtr-ifdPtr] = (ClientData)1;  /* the flag was given */
                pc->objv[bPtr-ifdPtr] = XOTclGlobalObjects[XOTE_ONE];
              } else {
                /* we assume for now, nrargs is at most 1 */
                o++; p++;
                if (bPtr->flags & XOTCL_ARG_REQUIRED) nrReq++; else nrOpt++;
                if (o<objc) {
#if defined(PARSE_TRACE_FULL)
		  fprintf(stderr, "...     setting cd[%d] '%s' = %s (%d) %s\n",
                          bPtr-ifdPtr, bPtr->name, ObjStr(objv[p]), bPtr->nrargs,
                          bPtr->flags & XOTCL_ARG_REQUIRED ? "req":"not req");
#endif
                  if ((*bPtr->converter)(interp, objv[p], &pc->clientData[bPtr-ifdPtr]) != TCL_OK) {
                    return TCL_ERROR;
                  }
                  pc->objv[bPtr-ifdPtr] = objv[p];
                } else {
                  Tcl_ResetResult(interp);
                  Tcl_AppendResult(interp, "Argument for parameter '", objStr, "' expected", (char *) NULL);
                  return TCL_ERROR;
                }
              }
              flagCount++;
              found = 1;
              break;
            }
          }
          if (!found) {
            /* we did not find the specified flag, the thing starting
               with a '-' must be an argument */
            break;
          }
        }
      }
      /*fprintf(stderr, "... we found %d flags\n",flagCount);*/
      /* skip in interface until the end of the switches */
      while (aPtr->name && *aPtr->name == '-') {aPtr++,i++;};
      /* under the assumption, flags have no arguments */
      o += flagCount;
      /*
       * check double dash --
       */
      if (o<objc) {
        objStr = ObjStr(objv[o]);
        if (*objStr == '-' && *(objStr+1) == '-' && *(objStr+2) == '\0') {
#if defined(PARSE_TRACE_FULL)
          fprintf(stderr, "... skip double dash\n");
#endif
          dashdash++;
          o++;
        }
      }
    } else {
      if (aPtr->flags & XOTCL_ARG_REQUIRED) nrReq++; else nrOpt++;
      args ++;
      /*fprintf(stderr,"... arg %s req %d converter %p try to set on %d: '%s'\n",
        aPtr->name,aPtr->flags & XOTCL_ARG_REQUIRED,aPtr->converter,i, ObjStr(objv[o]));*/
      if ((*aPtr->converter)(interp, objv[o], &pc->clientData[i]) != TCL_OK) {
        return TCL_ERROR;
      }

      /*
       * objv is always passed via pc->objv
       */
#if defined(PARSE_TRACE_FULL)
      fprintf(stderr, "...     setting %s pc->objv[%d] to [%d]'%s'\n",aPtr->name,i,o,ObjStr(objv[o]));
#endif
      pc->objv[i] = objv[o];
      o++; i++; aPtr++;
    }
  }
  pc->lastobjc = aPtr->name ? o : o-1;
  pc->objc = i;

  /* Process all args until end of interface to get correct conters */
  while (aPtr->name) {
    //fprintf(stderr, "end of if def %s\n",aPtr->name);
    if (aPtr->flags & XOTCL_ARG_REQUIRED) nrReq++; else nrOpt++;
    aPtr++;
  }

  /* is last argument a vararg? */
  aPtr--;
  if (!varArgs && aPtr->converter == convertToNothing) {
    varArgs = 1;
    /*fprintf(stderr, "last arg of proc '%s' is varargs\n", ObjStr(procName));*/
  }

  /* fprintf(stderr, "less nrreq %d last arg %s type %s\n", args < nrReq, aPtr->name, aPtr->converter);
  fprintf(stderr, "objc = %d, args = %d, nrReq %d, nrReq + nrOpt = %d, varArgs %d i %d %s\n",
  objc,args,nrReq,nrReq + nrOpt, varArgs, i,aPtr->name);*/

#if defined(PARSE_TRACE)
  fprintf(stderr, "PROC '%s', END lastobjc %d, varargs %d, not enough args (%d<%d) = %d, to many (%d>%d) = %d, check = %d\n", ObjStr(procName),
          pc->lastobjc, varArgs, args, nrReq, args < nrReq,
          objc-dashdash-1, nrReq + nrOpt, objc-dashdash-1 > nrReq + nrOpt, 
          !varArgs && objc-dashdash-1 > nrReq + nrOpt
          );
#endif

  if (pc->lastobjc < nrReq || (!varArgs && objc-dashdash-1 > nrReq + nrOpt)) {
    Tcl_Obj *msg = Tcl_NewStringObj("", 0);
    for (aPtr=ifdPtr; aPtr->name; aPtr++) {
      if (aPtr != ifdPtr) {
        Tcl_AppendToObj(msg, " ", 1);
      }
      if (aPtr->flags & XOTCL_ARG_REQUIRED) {
        Tcl_AppendToObj(msg, aPtr->name, -1);
      } else {
        Tcl_AppendToObj(msg, "?", 1);
        Tcl_AppendToObj(msg, aPtr->name, -1);
	if (aPtr->nrargs >0) {
	  Tcl_AppendToObj(msg, " arg", 4);
	}
        Tcl_AppendToObj(msg, "?", 1);
      }
    }
    return XOTclObjErrArgCntObj(interp, procName,  NULL, msg);
  }

  return TCL_OK;
}

/***********************************
 * Begin result setting commands
 * (essentially List*() and support
 ***********************************/
static int
ListKeys(Tcl_Interp *interp, Tcl_HashTable *table, char *pattern) {
  Tcl_HashEntry *hPtr;
  char *key;

  if (pattern && noMetaChars(pattern)) {
    hPtr = table ? XOTcl_FindHashEntry(table, pattern) : 0;
    if (hPtr) {
      key = Tcl_GetHashKey(table, hPtr);
      Tcl_SetResult(interp, key, TCL_VOLATILE);
    } else {
      Tcl_SetObjResult(interp, XOTclGlobalObjects[XOTE_EMPTY]);
    }
  } else {
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);
    Tcl_HashSearch hSrch;
    hPtr = table ? Tcl_FirstHashEntry(table, &hSrch) : 0;
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      key = Tcl_GetHashKey(table, hPtr);
      if (!pattern || Tcl_StringMatch(key, pattern)) {
        Tcl_ListObjAppendElement(interp, list, Tcl_NewStringObj(key,-1));
      }
    }
    Tcl_SetObjResult(interp, list);
  }
  return TCL_OK;
}

#if !defined(PRE85) || FORWARD_COMPATIBLE
static int
ListVarKeys(Tcl_Interp *interp, Tcl_HashTable *tablePtr, char *pattern) {
  Tcl_HashEntry *hPtr;

  if (pattern && noMetaChars(pattern)) {
    Tcl_Obj *patternObj = Tcl_NewStringObj(pattern, -1);
    INCR_REF_COUNT(patternObj);

    hPtr = tablePtr ? XOTcl_FindHashEntry(tablePtr, (char *)patternObj) : 0;
    if (hPtr) {
      Var  *val = VarHashGetValue(hPtr);
      Tcl_SetObjResult(interp, VarHashGetKey(val));
    } else {
      Tcl_SetObjResult(interp, XOTclGlobalObjects[XOTE_EMPTY]);
    }
    DECR_REF_COUNT(patternObj);
  } else {
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);
    Tcl_HashSearch hSrch;
    hPtr = tablePtr ? Tcl_FirstHashEntry(tablePtr, &hSrch) : 0;
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      Var  *val = VarHashGetValue(hPtr);
      Tcl_Obj *key  = VarHashGetKey(val);
      if (!pattern || Tcl_StringMatch(ObjStr(key), pattern)) {
        Tcl_ListObjAppendElement(interp, list, key);
      }
    }
    Tcl_SetObjResult(interp, list);
  }
  return TCL_OK;
}
#endif

static int
ListMethodKeys(Tcl_Interp *interp, Tcl_HashTable *table, char *pattern,
               int noProcs, int noCmds, Tcl_HashTable *dups, int onlyForwarder, int onlySetter) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr = table ? Tcl_FirstHashEntry(table, &hSrch) : 0;
  for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(table, hPtr);
    Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
    Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);

    if (pattern && !Tcl_StringMatch(key, pattern)) continue;
    if (noCmds  && proc != RUNTIME_STATE(interp)->objInterpProc) continue;
    if (noProcs && proc == RUNTIME_STATE(interp)->objInterpProc) continue;
    if (onlyForwarder && proc != XOTclForwardMethod) continue;
    if (onlySetter && proc != XOTclSetterMethod) continue;
    /* XOTclObjscopedMethod ??? */

    if (dups) {
      int new;
      Tcl_HashEntry *duphPtr;
      duphPtr = Tcl_CreateHashEntry(dups, key, &new);
      if (!new) {
	/*fprintf(stderr,"preexisting entry %s\n", key);*/
	continue;
      } else {
	/*fprintf(stderr,"new entry %s\n", key);*/
      }
    }

    if (((Command *) cmd)->flags & XOTCL_CMD_PROTECTED_METHOD) {
      /*fprintf(stderr, "--- dont list protected name '%s'\n", key);*/
      continue;
    }
    Tcl_AppendElement(interp, key);
  }
  /*fprintf(stderr, "listkeys returns '%s'\n", ObjStr(Tcl_GetObjResult(interp)));*/
  return TCL_OK;
}

static int
ListChildren(Tcl_Interp *interp, XOTclObject *obj, char *pattern, int classesOnly) {
  XOTclObject *childobj;
  Tcl_HashTable *cmdTable;
  XOTcl_FrameDecls;

  if (!obj->nsPtr) return TCL_OK;

  cmdTable = Tcl_Namespace_cmdTable(obj->nsPtr);
  if (pattern && noMetaChars(pattern)) {
    XOTcl_PushFrame(interp, obj);
    if ((childobj = XOTclpGetObject(interp, pattern)) &&
        (!classesOnly || XOTclObjectIsClass(childobj)) &&
        (Tcl_Command_nsPtr(childobj->id) == obj->nsPtr)  /* true children */
        ) {
      Tcl_SetObjResult(interp, childobj->cmdName);
    } else {
      Tcl_SetObjResult(interp, XOTclGlobalObjects[XOTE_EMPTY]);
    }
    XOTcl_PopFrame(interp, obj);
  } else {
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch);
    char *key;
    XOTcl_PushFrame(interp, obj);
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      key = Tcl_GetHashKey(cmdTable, hPtr);
      if (!pattern || Tcl_StringMatch(key, pattern)) {
        if ((childobj = XOTclpGetObject(interp, key)) &&
            (!classesOnly || XOTclObjectIsClass(childobj)) &&
            (Tcl_Command_nsPtr(childobj->id) == obj->nsPtr)  /* true children */
            ) {
          Tcl_ListObjAppendElement(interp, list, childobj->cmdName);
        }
      }
    }
    XOTcl_PopFrame(interp, obj);
    Tcl_SetObjResult(interp, list);
  }
  return TCL_OK;
}

static int
ListForward(Tcl_Interp *interp, Tcl_HashTable *table, char *pattern, int definition) {
  int rc;
  if (definition) {
    Tcl_HashEntry *hPtr = table && pattern ? XOTcl_FindHashEntry(table, pattern) : 0;
    /* notice: we don't use pattern for wildcard matching here;
       pattern can only contain wildcards when used without
       "-definition" */
    if (hPtr) {
      Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
      ClientData clientData = cmd? Tcl_Command_objClientData(cmd) : NULL;
      forwardCmdClientData *tcd = (forwardCmdClientData *)clientData;
      if (tcd) {
        Tcl_Obj *list = Tcl_NewListObj(0, NULL);
        if (tcd->prefix) {
          Tcl_ListObjAppendElement(interp, list, Tcl_NewStringObj("-methodprefix",-1));
          Tcl_ListObjAppendElement(interp, list, tcd->prefix);
        }
        if (tcd->subcommands) {
          Tcl_ListObjAppendElement(interp, list, Tcl_NewStringObj("-default",-1));
          Tcl_ListObjAppendElement(interp, list, tcd->subcommands);
        }
        if (tcd->objscope) {
          Tcl_ListObjAppendElement(interp, list, Tcl_NewStringObj("-objscope",-1));
        }
        Tcl_ListObjAppendElement(interp, list, tcd->cmdName);
        if (tcd->args) {
          Tcl_Obj **args;
          int nrArgs, i;
          Tcl_ListObjGetElements(interp, tcd->args, &nrArgs, &args);
          for (i=0; i<nrArgs; i++) {
            Tcl_ListObjAppendElement(interp, list, args[i]);
          }
        }
        Tcl_SetObjResult(interp, list);
      } else {
        /* TODO: ERROR HANDLING */
      }
    } else {
      /* TODO: ERROR HANDLING TODO */
    }
    rc = TCL_OK;
  } else {
    rc = ListMethodKeys(interp, table, pattern, 1, 0, NULL, 1, 0);
  }
  return rc;
}

static int
ListMethods(Tcl_Interp *interp, XOTclObject *obj, char *pattern,
            int noProcs, int noCmds, int noMixins, int inContext) {
  XOTclClasses *pl;
  Tcl_HashTable dupsTable, *dups = &dupsTable;
  Tcl_InitHashTable(dups, TCL_STRING_KEYS);

  /*fprintf(stderr,"listMethods %s %d %d %d %d\n", pattern, noProcs, noCmds, noMixins, inContext);*/

  if (obj->nsPtr) {
    Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(obj->nsPtr);
    ListMethodKeys(interp, cmdTable, pattern, noProcs, noCmds, dups, 0, 0);
  }

  if (!noMixins) {
    if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
      MixinComputeDefined(interp, obj);
    if (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
      XOTclCmdList *ml;
      XOTclClass *mixin;
      for (ml = obj->mixinOrder; ml; ml = ml->nextPtr) {
	int guardOk = TCL_OK;
        mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
        if (inContext) {
          if (!RUNTIME_STATE(interp)->guardCount) {
            guardOk = GuardCall(obj, 0, 0, interp, ml->clientData, NULL);
          }
        }
        if (mixin && guardOk == TCL_OK) {
          Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(mixin->nsPtr);
          ListMethodKeys(interp, cmdTable, pattern, noProcs, noCmds, dups, 0, 0);
        }
      }
    }
  }

  /* append per-class filters */
  for (pl = ComputeOrder(obj->cl, obj->cl->order, Super); pl; pl = pl->nextPtr) {
    Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(pl->cl->nsPtr);
    ListMethodKeys(interp, cmdTable, pattern, noProcs, noCmds, dups, 0, 0);
  }
  Tcl_DeleteHashTable(dups);
  return TCL_OK;
}

static int
ListSuperclasses(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *pattern, int withClosure) {
  XOTclObject *matchObject = NULL;
  Tcl_Obj *patternObj = NULL;
  char *patternString = NULL;
  int rc;

  if (pattern && convertToObjpattern(interp, pattern, (ClientData *)&patternObj) == TCL_OK) {
    if (getMatchObject(interp, patternObj, pattern, &matchObject, &patternString) == -1) {
      if (patternObj) {
	DECR_REF_COUNT(patternObj);
      }
      return TCL_OK;
    }
  }

  if (withClosure) {
    XOTclClasses *pl = ComputeOrder(cl, cl->order, Super);
    if (pl) pl=pl->nextPtr;
    rc = AppendMatchingElementsFromClasses(interp, pl, patternString, matchObject);
  } else {
    XOTclClasses *clSuper = XOTclReverseClasses(cl->super);
    rc = AppendMatchingElementsFromClasses(interp, clSuper, patternString, matchObject);
    XOTclClassListFree(clSuper);
  }

  if (matchObject) {
    Tcl_SetObjResult(interp, rc ? matchObject->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
  }

  if (patternObj) {
    DECR_REF_COUNT(patternObj);
  }
  return TCL_OK;
}


/* proc/instproc specific code */
static int
ListProcBody(Tcl_Interp *interp, Proc *procPtr, char *methodName) {
  if (procPtr) {
    char *body = ObjStr(procPtr->bodyPtr);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(StripBodyPrefix(body), -1));
    return TCL_OK;
  }
  return XOTclErrBadVal(interp, "info body", "a tcl method name", methodName);
}

static int
ListArgsFromOrdinaryArgs(Tcl_Interp *interp, XOTclNonposArgs *nonposArgs) {
  Tcl_Obj *argList = argList = Tcl_NewListObj(0, NULL);
  AppendOrdinaryArgsFromNonposArgs(interp, nonposArgs, 1, argList);
  Tcl_SetObjResult(interp, argList);
  return TCL_OK;
}

static int
ListProcDefault(Tcl_Interp *interp, Proc *procPtr,
                char *name, char *arg, Tcl_Obj *var) {
  Tcl_Obj *defVal;

  if (GetProcDefault(interp, procPtr, arg, &defVal) == TCL_OK) {
    return SetProcDefault(interp, var, defVal);
  } else {
    return XOTclVarErrMsg(interp, "method '", name,
                          "' doesn't exist or doesn't have an argument '",
                          arg, "'", (char *) NULL);
  }
}

static int
ListDefaultFromOrdinaryArgs(Tcl_Interp *interp, char *procName,
                            XOTclNonposArgs *nonposArgs, char *arg, Tcl_Obj *var) {
  argDefinition CONST *aPtr;

  for (aPtr = nonposArgs->ifd; aPtr->name; aPtr++) {
    if (*aPtr->name == '-') continue;
    if (strcmp(aPtr->name,arg) == 0) {
      return SetProcDefault(interp, var, aPtr->defaultValue);
    }
  }
  return XOTclVarErrMsg(interp, "method '", procName, "' doesn't have an argument '",
                        arg, "'", (char *) NULL);
}

static int
ListProcArgs(Tcl_Interp *interp, Proc *proc, char *name) {
  if (proc) {
    CompiledLocal *args = proc->firstLocalPtr;
    Tcl_ResetResult(interp);
    for ( ; args; args = args->nextPtr) {
      if (TclIsCompiledLocalArgument(args))
	Tcl_AppendElement(interp, args->name);
    }
    return TCL_OK;
  }
  return XOTclErrBadVal(interp, "info args", "a tcl method name", name);
}

/* static int */
/* ListObjPtrHashTable(Tcl_Interp *interp, Tcl_HashTable *table, char *pattern) { */
/*   Tcl_HashEntry *hPtr; */
/*   if (pattern && noMetaChars(pattern)) { */
/*     XOTclObject *childobj = XOTclpGetObject(interp, pattern); */
/*     hPtr = XOTcl_FindHashEntry(table, (char *)childobj); */
/*     if (hPtr) { */
/*       Tcl_SetObjResult(interp, childobj->cmdName); */
/*     } else { */
/*       Tcl_SetObjResult(interp, XOTclGlobalObjects[XOTE_EMPTY]); */
/*     } */
/*   } else { */
/*     Tcl_Obj *list = Tcl_NewListObj(0, NULL); */
/*     Tcl_HashSearch hSrch; */
/*     hPtr = table ? Tcl_FirstHashEntry(table, &hSrch) : 0; */
/*     for (;  hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) { */
/*       XOTclObject *obj = (XOTclObject*)Tcl_GetHashKey(table, hPtr); */
/*       if (!pattern || Tcl_StringMatch(objectName(obj), pattern)) { */
/*         Tcl_ListObjAppendElement(interp, list, obj->cmdName); */
/*       } */
/*     } */
/*     Tcl_SetObjResult(interp, list); */
/*   } */
/*   return TCL_OK; */
/* } */

/********************************
 * End result setting commands
 ********************************/


/*********************************
 * Begin generated XOTcl commands
 *********************************/

static int XOTclAliasCmd(Tcl_Interp *interp, XOTclObject *object, char *methodName,
                         int withObjscope, int withPer_object, int withProtected, Tcl_Obj *cmdName) {
  XOTclClass *cl;
  Tcl_Command cmd, importedCmd;
  Tcl_ObjCmdProc *objProc;
  char allocation;
  Tcl_CmdDeleteProc *dp = NULL;
  aliasCmdClientData *tcd = NULL;
  int flags = 0;

  if (XOTclObjectIsClass(object)) {
    cl = (XOTclClass *)object;
    allocation = 'c';
  } else {
    cl = NULL;
    allocation = 'o';
  }
  cmd = Tcl_GetCommandFromObj(interp, cmdName);
  if (cmd == NULL)
    return XOTclVarErrMsg(interp, "cannot lookup command '",
                          ObjStr(cmdName), "'", (char *) NULL);

  if ((importedCmd = TclGetOriginalCommand(cmd))) {
    cmd = importedCmd;
  }
  objProc = Tcl_Command_objProc(cmd);

  if (withObjscope) {
    tcd = NEW(aliasCmdClientData);
    tcd->cmdName    = NULL;
    tcd->obj        = allocation == 'c' ? &cl->object : object;
    tcd->objProc    = objProc;
    tcd->clientData = Tcl_Command_objClientData(cmd);
    objProc         = XOTclObjscopedMethod;
    dp = aliasCmdDeleteProc;
  } else {
    tcd = Tcl_Command_objClientData(cmd);
  }

  if (withProtected) {
    flags = XOTCL_CMD_PROTECTED_METHOD;
  }

  if (allocation == 'c') {
    XOTclAddInstanceMethod(interp, (XOTcl_Class*)cl, methodName,
                           objProc, tcd, dp, flags);
  } else {
    XOTclAddObjectMethod(interp, (XOTcl_Object*)object, methodName,
                         objProc, tcd, dp, flags);
  }
  return TCL_OK;
}

static int XOTclConfigureCmd(Tcl_Interp *interp, int configureoption, Tcl_Obj *value) {
  int bool;

  if (value) {
    int result = Tcl_GetBooleanFromObj(interp, value, &bool);
    if (result != TCL_OK)
      return result;
  }
  
  switch (configureoption) {
  case configureoptionFilterIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doFilters));
    if (value)
      RUNTIME_STATE(interp)->doFilters = bool;
    break;

  case configureoptionSoftrecreateIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doSoftrecreate));
    if (value)
      RUNTIME_STATE(interp)->doSoftrecreate = bool;
    break;
 case configureoptionCacheinterfaceIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->cacheInterface));
    if (value)
      RUNTIME_STATE(interp)->cacheInterface = bool;
    break;
  }
  return TCL_OK;
}

static int XOTclMethodPropertyCmd(Tcl_Interp *interp, XOTclObject *object, char *methodName,
                                  int withPer_object, int methodproperty, Tcl_Obj *value) {
  XOTclClass *cl;
  Tcl_Command cmd = NULL;
  char allocation;
  int protected = 0;

  /* TODO: introspection for method properties */

  if (XOTclObjectIsClass(object)) {
    cl = (XOTclClass *)object;
    allocation = 'c';
  } else {
    cl = NULL;
    allocation = 'o';
  }

  if (withPer_object) {
    allocation = 'o';
  }

  if (allocation == 'o') {
    if (object->nsPtr)
      cmd = FindMethod(methodName, object->nsPtr);
    if (!cmd) {
      return XOTclVarErrMsg(interp, "Cannot lookup object method '",
			    methodName, "' for object ", objectName(object),
			    (char *) NULL);
    }
  } else {
    if (cl->nsPtr)
      cmd = FindMethod(methodName, cl->nsPtr);
    if (!cmd)
      return XOTclVarErrMsg(interp, "Cannot lookup method '",
			    methodName, "' from class  ", objectName(object),
			    (char *) NULL);
  }

  if (methodproperty == methodpropertyProtectedIdx || methodproperty == methodpropertyPublicIdx) {
    protected = (methodproperty == methodpropertyProtectedIdx);

    if (protected) {
      Tcl_Command_flags(cmd) |= XOTCL_CMD_PROTECTED_METHOD;
    } else {
      Tcl_Command_flags(cmd) &= XOTCL_CMD_PROTECTED_METHOD;
    }
    /* TODO check: what about procs? */

  } else { /* slotobj */
    XOTclNonposArgs *nonposArgs;

    if (value == NULL) {
      return XOTclVarErrMsg(interp, "Option 'slotobj' of method ",methodName,
                            " requires argument '", (char *) NULL);
    }

    nonposArgs = getNonposArgs(cmd);
    if (nonposArgs == NULL) {
      nonposArgs = NEW(XOTclNonposArgs);
      memset(nonposArgs, 0, sizeof(XOTclNonposArgs));
      addNonposArgs(interp, cmd, nonposArgs);
      /* TODO check:
         handle cases: cmd is not a proc.
	 what happens if first method property and then method.
	 what happens if method then property then new method?
      */
    } else {
      fprintf(stderr,"define slotobj for a method with nonpospargs\n slotobj = %s \n", ObjStr(value));
      if (nonposArgs->slotObj) {
	DECR_REF_COUNT(nonposArgs->slotObj);
      }
    }
    nonposArgs->slotObj = value;
    INCR_REF_COUNT(nonposArgs->slotObj);
  }

  return TCL_OK;
}

static int XOTclMyCmd(Tcl_Interp *interp, int withLocal, Tcl_Obj *method, int nobjc, Tcl_Obj *CONST nobjv[]) {
  XOTclObject *self = GetSelfObj(interp);
  int result;

  if (!self) {
    return XOTclVarErrMsg(interp, "Cannot resolve 'self', probably called outside the context of an XOTcl Object",
                          (char *) NULL);
  }

  if (withLocal) {
    XOTclClass *cl = self->cl;
    char *methodName = ObjStr(method);
    Tcl_Command cmd = FindMethod(methodName, cl->nsPtr);
    if (cmd == 0)
      return XOTclVarErrMsg(interp, objectName(self),
                            ": unable to dispatch local method '",
                            methodName, "' in class ", className(cl),
                            (char *) NULL);
    result = InvokeMethod((ClientData)self, interp, nobjc+2, nobjv, cmd, self, cl,
                          methodName, 0);
  } else {
    result = callMethod((ClientData)self, interp, method, nobjc+2, nobjv, 0);
  }
  return result;
}


static int XOTclRelationCmd(Tcl_Interp *interp, XOTclObject *object, int relationtype, Tcl_Obj *value) {
  int oc; Tcl_Obj **ov;
  XOTclObject *nobj = NULL;
  XOTclClass *cl = NULL;
  XOTclObjectOpt *objopt = NULL;
  XOTclClassOpt *clopt = NULL, *nclopt = NULL;
  int i;

  switch (relationtype) {
  case relationtypeObject_mixinIdx:
  case relationtypeMixinIdx:
  case relationtypeObject_filterIdx:
  case relationtypeFilterIdx:
    if (value == NULL) {
      objopt = object->opt;
      switch (relationtype) {
      case relationtypeObject_mixinIdx:
      case relationtypeMixinIdx: return objopt ? MixinInfo(interp, objopt->mixins, NULL, 1, NULL) : TCL_OK;
      case relationtypeObject_filterIdx:
      case relationtypeFilterIdx: return objopt ? FilterInfo(interp, objopt->filters, NULL, 1, 0) : TCL_OK;
      }
    }
    if (Tcl_ListObjGetElements(interp, value, &oc, &ov) != TCL_OK)
      return TCL_ERROR;
    objopt = XOTclRequireObjectOpt(object);
    break;

  case relationtypeClass_mixinIdx:
  case relationtypeInstmixinIdx:
  case relationtypeClass_filterIdx:
  case relationtypeInstfilterIdx:
    if (XOTclObjectIsClass(object)) {
      cl = (XOTclClass *)object;
    } else {
      return XOTclObjErrType(interp, object->cmdName, "Class");
    }

    if (value == NULL) {
      clopt = cl->opt;
      switch (relationtype) {
      case relationtypeClass_mixinIdx:
      case relationtypeInstmixinIdx: return clopt ? MixinInfo(interp, clopt->instmixins, NULL, 1, NULL) : TCL_OK;
      case relationtypeClass_filterIdx:
      case relationtypeInstfilterIdx: return objopt ? FilterInfo(interp, clopt->instfilters, NULL, 1, 0) : TCL_OK;
      }
    }

    if (Tcl_ListObjGetElements(interp, value, &oc, &ov) != TCL_OK)
      return TCL_ERROR;
    clopt = XOTclRequireClassOpt(cl);
    break;

  case relationtypeSuperclassIdx:
    if (!XOTclObjectIsClass(object))
      return XOTclObjErrType(interp, object->cmdName, "Class");
    cl = (XOTclClass *)object;
    if (value == NULL) {
      return ListSuperclasses(interp, cl, NULL, 0);
    }
    if (Tcl_ListObjGetElements(interp, value, &oc, &ov) != TCL_OK)
      return TCL_ERROR;
    return SuperclassAdd(interp, cl, oc, ov, value, cl->object.cl);

  case relationtypeClassIdx:
    if (value == NULL) {
      Tcl_SetObjResult(interp, object->cl->object.cmdName);
      return TCL_OK;
    }
    GetClassFromObj(interp, value, &cl, object->cl);
    if (!cl) return XOTclErrBadVal(interp, "class", "a class", objectName(object));
    return changeClass(interp, object, cl);

  case relationtypeRootclassIdx:
    {
    XOTclClass *metaClass;

    if (!XOTclObjectIsClass(object))
      return XOTclObjErrType(interp, object->cmdName, "Class");
    cl = (XOTclClass *)object;

    if (value == NULL) {
      return XOTclVarErrMsg(interp, "metaclass must be specified as third argument",
                            (char *) NULL);
    }
    GetClassFromObj(interp, value, &metaClass, 0);
    if (!metaClass) return XOTclObjErrType(interp, value, "Class");

    cl->object.flags |= XOTCL_IS_ROOT_CLASS;
    metaClass->object.flags |= XOTCL_IS_ROOT_META_CLASS;

    XOTclClassListAdd(&RUNTIME_STATE(interp)->rootClasses, cl, (ClientData)metaClass);
    return TCL_OK;
    /* todo:
       need to remove these properties?
       allow to delete a classystem at runtime?
    */
    }
  }

  switch (relationtype) {
  case relationtypeObject_mixinIdx:
  case relationtypeMixinIdx:
    if (objopt->mixins) {
      XOTclCmdList *cmdlist, *del;
      for (cmdlist = objopt->mixins; cmdlist; cmdlist = cmdlist->nextPtr) {
        cl = XOTclGetClassFromCmdPtr(cmdlist->cmdPtr);
        clopt = cl ? cl->opt : NULL;
        if (clopt) {
          del = CmdListFindCmdInList(object->id, clopt->isObjectMixinOf);
          if (del) {
            /* fprintf(stderr,"Removing object %s from isObjectMixinOf of class %s\n",
               objectName(object), ObjStr(XOTclGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
            del = CmdListRemoveFromList(&clopt->isObjectMixinOf, del);
            CmdListDeleteCmdListEntry(del, GuardDel);
          }
        }
      }
      CmdListRemoveList(&objopt->mixins, GuardDel);
    }

    object->flags &= ~XOTCL_MIXIN_ORDER_VALID;
    /*
     * since mixin procs may be used as filters -> we have to invalidate
     */
    object->flags &= ~XOTCL_FILTER_ORDER_VALID;

    /*
     * now add the specified mixins
     */
    for (i = 0; i < oc; i++) {
      Tcl_Obj *ocl = NULL;

      if (MixinAdd(interp, &objopt->mixins, ov[i], object->cl->object.cl) != TCL_OK) {
        return TCL_ERROR;
      }
      /* fprintf(stderr,"Added to mixins of %s: %s\n", objectName(obj), ObjStr(ov[i])); */
      Tcl_ListObjIndex(interp, ov[i], 0, &ocl);
      GetObjectFromObj(interp, ocl, &nobj);
      if (nobj) {
        /* fprintf(stderr,"Registering object %s to isObjectMixinOf of class %s\n",
           objectName(obj), objectName(nobj)); */
        nclopt = XOTclRequireClassOpt((XOTclClass*) nobj);
        CmdListAdd(&nclopt->isObjectMixinOf, object->id, NULL, /*noDuplicates*/ 1);
      } /* else fprintf(stderr,"Problem registering %s as a mixinof of %s\n",
           ObjStr(ov[i]), className(cl)); */
    }

    MixinComputeDefined(interp, object);
    FilterComputeDefined(interp, object);
    break;

  case relationtypeObject_filterIdx:
  case relationtypeFilterIdx:

    if (objopt->filters) CmdListRemoveList(&objopt->filters, GuardDel);

    object->flags &= ~XOTCL_FILTER_ORDER_VALID;
    for (i = 0; i < oc; i ++) {
      if (FilterAdd(interp, &objopt->filters, ov[i], object, 0) != TCL_OK)
        return TCL_ERROR;
    }
    /*FilterComputeDefined(interp, obj);*/
    break;

  case relationtypeClass_mixinIdx:
  case relationtypeInstmixinIdx:

    if (clopt->instmixins) {
      RemoveFromClassMixinsOf(cl->object.id, clopt->instmixins);
      CmdListRemoveList(&clopt->instmixins, GuardDel);
    }
    MixinInvalidateObjOrders(interp, cl);
    /*
     * since mixin procs may be used as filters,
     * we have to invalidate the filters as well
     */
    FilterInvalidateObjOrders(interp, cl);

    for (i = 0; i < oc; i++) {
      Tcl_Obj *ocl = NULL;
      if (MixinAdd(interp, &clopt->instmixins, ov[i], cl->object.cl) != TCL_OK) {
        return TCL_ERROR;
      }
      /* fprintf(stderr,"Added to instmixins of %s: %s\n",
         className(cl), ObjStr(ov[i])); */

      Tcl_ListObjIndex(interp, ov[i], 0, &ocl);
      GetObjectFromObj(interp, ocl, &nobj);
      if (nobj) {
        /* fprintf(stderr,"Registering class %s to isClassMixinOf of class %s\n",
           className(cl), objectName(nobj)); */
        nclopt = XOTclRequireClassOpt((XOTclClass*) nobj);
        CmdListAdd(&nclopt->isClassMixinOf, cl->object.id, NULL, /*noDuplicates*/ 1);
      } /* else fprintf(stderr,"Problem registering %s as a instmixinof of %s\n",
           ObjStr(ov[i]), className(cl)); */
    }
    break;

  case relationtypeClass_filterIdx:
  case relationtypeInstfilterIdx:

    if (clopt->instfilters) CmdListRemoveList(&clopt->instfilters, GuardDel);

    FilterInvalidateObjOrders(interp, cl);
    for (i = 0; i < oc; i ++) {
      if (FilterAdd(interp, &clopt->instfilters, ov[i], 0, cl) != TCL_OK)
        return TCL_ERROR;
    }
    break;

  }
  return TCL_OK;
}

static int XOTclSetInstvarCmd(Tcl_Interp *interp, XOTclObject *object, Tcl_Obj *variable, Tcl_Obj *value) {
  return setInstVar(interp, object , variable, value);
}

/***************************
 * End generated XOTcl commands
 ***************************/

/***************************
 * Begin Object Methods
 ***************************/
static int XOTclOAutonameMethod(Tcl_Interp *interp, XOTclObject *obj, int withInstance, int withReset,
                                Tcl_Obj *name) {
  Tcl_Obj *autoname = AutonameIncr(interp, name, obj, withInstance, withReset);
  if (autoname) {
    Tcl_SetObjResult(interp, autoname);
    DECR_REF_COUNT(autoname);
  }
  else
    return XOTclVarErrMsg(interp,
                          "Autoname failed. Probably format string (with %) was not well-formed",
                          (char *) NULL);

  return TCL_OK;
}

static int XOTclOCheckMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *flag) {
  XOTclObjectOpt *opt = XOTclRequireObjectOpt(obj);
  int ocArgs, i;
  Tcl_Obj **ovArgs;
  opt->checkoptions = CHECK_NONE;

  if (Tcl_ListObjGetElements(interp, flag, &ocArgs, &ovArgs) == TCL_OK
      && ocArgs > 0) {
    for (i = 0; i < ocArgs; i++) {
      char *option = ObjStr(ovArgs[i]);
      if (option) {
        switch (*option) {
        case 'i':
          if (strcmp(option, "instinvar") == 0) {
            opt->checkoptions |= CHECK_CLINVAR;
          } else if (strcmp(option, "invar") == 0) {
            opt->checkoptions |= CHECK_OBJINVAR;
          }
          break;
        case 'p':
          if (strcmp(option, "pre") == 0) {
            opt->checkoptions |= CHECK_PRE;
          } else if (strcmp(option, "post") == 0) {
            opt->checkoptions  |= CHECK_POST;
          }
          break;
        case 'a':
          if (strcmp(option, "all") == 0) {
            opt->checkoptions |= CHECK_ALL;
          }
          break;	
        }
      }
    }
  }
  if (opt->checkoptions == CHECK_NONE && ocArgs>0) {
    return XOTclVarErrMsg(interp, "Unknown check option in command '",
                          objectName(obj), " check  ", ObjStr(flag),
                          "', valid: all pre post invar instinvar",
			  (char *) NULL);
  }

  Tcl_ResetResult(interp);
  return TCL_OK;
}

static int XOTclOCleanupMethod(Tcl_Interp *interp, XOTclObject *obj) {
  XOTclClass  *cl  = XOTclObjectToClass(obj);
  char *fn;
  int softrecreate;
  Tcl_Obj *savedNameObj;

#if defined(OBJDELETION_TRACE)
  fprintf(stderr,"+++ XOTclOCleanupMethod\n");
#endif
  PRINTOBJ("XOTclOCleanupMethod", obj);

  fn = objectName(obj);
  savedNameObj = obj->cmdName;
  INCR_REF_COUNT(savedNameObj);

  /* save and pass around softrecreate*/
  softrecreate = obj->flags & XOTCL_RECREATE && RUNTIME_STATE(interp)->doSoftrecreate;

  CleanupDestroyObject(interp, obj, softrecreate);
  CleanupInitObject(interp, obj, obj->cl, obj->nsPtr, softrecreate);

  if (cl) {
    CleanupDestroyClass(interp, cl, softrecreate, 1);
    CleanupInitClass(interp, cl, cl->nsPtr, softrecreate, 1);
  }

  DECR_REF_COUNT(savedNameObj);
  return TCL_OK;
}

/*
typedef struct  {
  char *name;
  int required;
  int nrargs;
  XOTclTypeConverter *converter;
  Tcl_Obj *defaultValue;
  char *type;
} argDefinition;
 */

static int 
GetObjectInterface(Tcl_Interp *interp, char *methodName, XOTclObject *obj,  
                   XOTclParsedInterfaceDefinition *parsedIf, int *hasNonposArgs) {
  int result;
  Tcl_Obj *rawConfArgs;

  /* WARNING:

     a) definitions are freed on a class cleanup, with 
        ParsedInterfaceDefinitionFree(cl->parsedIf)

     What should be done:

    b) the same cleanup should be performed, whenever 
        1) the class structure changes, DONE
        2) instmixins are added DONE
        3) slots are defined, DONE
        4) slots defaults or types are changed
        5) slots removals (destroy on slots)

  */

  if (obj->cl->parsedIf) {
    parsedIf->nonposArgs = obj->cl->parsedIf->nonposArgs;
    parsedIf->possibleUnknowns = obj->cl->parsedIf->possibleUnknowns;
    /*fprintf(stderr, "--- returned cached objif for obj %s from %s: parsedIf->nonposArgs %p  ifdSize %d\n",
      objectName(obj),className(obj->cl), parsedIf->nonposArgs, parsedIf->nonposArgs ? parsedIf->nonposArgs->ifdSize : -1);*/
    result = TCL_OK;
  } else {
    /* get the string representation of the interface */
    result = callMethod((ClientData) obj, interp, XOTclGlobalObjects[XOTE_OBJINTERFACE], 2, 0, 0);
    if (result == TCL_OK) {
      rawConfArgs = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(rawConfArgs);
      /* TODO: this is a dangerous comparison */
      if (rawConfArgs != XOTclGlobalObjects[XOTE_EMPTY]) {
        
        /* Obtain interface structure */
        /* TODO: rather ObjStr(rawConfArgs) or unnecessary */
        result = GenerateIfd(interp, methodName, rawConfArgs, parsedIf, hasNonposArgs);
        /*fprintf(stderr, "GenerateIfd obj %s for '%s' returned parsedIf->nonposArgs %p\n",
          objectName(obj), ObjStr(rawConfArgs), parsedIf->nonposArgs);*/
        if (result == TCL_OK && RUNTIME_STATE(interp)->cacheInterface) {
          XOTclParsedInterfaceDefinition *ifd = NEW(XOTclParsedInterfaceDefinition);
          ifd->nonposArgs = parsedIf->nonposArgs;
          ifd->possibleUnknowns = parsedIf->possibleUnknowns;
          obj->cl->parsedIf = ifd; /* free with ParsedInterfaceDefinitionFree(cl->parsedIf); */
          
          /*fprintf(stderr, "--- GetObjectInterface cache objif for obj %s nonposArgs %p possibleUnknowns %d ifd %p ifdSize %d\n",
                  objectName(obj),className(obj->cl),
                  ifd->nonposArgs,ifd->possibleUnknowns, ifd->nonposArgs ? ifd->nonposArgs->ifdSize : -1);*/
        }
      }
      
      DECR_REF_COUNT(rawConfArgs);
    }
  }
  return result;
}

static int 
XOTclOConfigureMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]) {
  int result;

#if defined(CONFIGURE_ARGS)
  /* TODO: check for CONST, check for mem leaks and cleanups, especially XOTclParsedInterfaceDefinition */
  Tcl_Obj *oldValue, *newValue;
  XOTclParsedInterfaceDefinition parsedIf;
  int haveNonposArgs = 0, i, remainingArgsc;
  argDefinition *iConfigure, *iConfigurePtr, *ifPtr;
  XOTclNonposArgs *nonposArgs;
  parseContext pc;
  Tcl_Obj *argNameObj;
  XOTcl_FrameDecls;

  result = GetObjectInterface(interp, ObjStr(objv[0]), obj, &parsedIf, &haveNonposArgs);
  if (result != TCL_OK || !parsedIf.nonposArgs) {
    fprintf(stderr, "... nothing to do for method %s\n", ObjStr(objv[0]));
    goto configure_exit;
  }

  nonposArgs = parsedIf.nonposArgs;
  iConfigurePtr = iConfigure = nonposArgs->ifd;

  /* allow the retrieval of self (GetSelfObj(); needed in convertToRelation)
   * + make instvars of obj accessible */
  XOTcl_PushFrame(interp, obj);

  /* 2. continue parsing the actual args passed */
  result = canonicalNonpositionalArgs(&pc, interp, nonposArgs, "configure", objc, objv);
  if (result != TCL_OK) {
    parseContextRelease(&pc);
    goto configure_exit;
  }

  /*
   * STEP 3: stage the object under initialisation/ construction; using:
   * pc.objc+1, pc.full_objv
   */
#if defined(CONFIGURE_ARGS_TRACE)
  fprintf(stderr, "*** POPULATE OBJ ''''%s'''': nr of parsed args '%d'\n",objectName(obj),pc.objc);
#endif
  for (i = 1, ifPtr = iConfigure; i < nonposArgs->ifdSize; i++, ifPtr++) {
    char *argName = ifPtr->name;

    if (*argName == '-') argName++;
    newValue = pc.full_objv[i];

    /*fprintf(stderr, "newValue of %s = %p '%s'\n", argName, newValue, ObjStr(newValue));*/
    if (newValue == XOTclGlobalObjects[XOTE___UNKNOWN__]) {
#if defined(CONFIGURE_ARGS_TRACE)
      fprintf(stderr, "*** POPULATE OBJ SKIPPING: arg '%s' would be unset\n",argName);
#endif
      continue;
    }

    argNameObj = Tcl_NewStringObj(argName,strlen(argName));
    INCR_REF_COUNT(argNameObj);
    /* TODO: we dont need oldValue, .... work on this, when we are back in business with regression test */
    oldValue = Tcl_ObjGetVar2(interp, argNameObj, NULL, TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
    /*oldValue = Tcl_GetVar2Ex(interp, argName, NULL, TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);*/
    /*fprintf(stderr, "*** old value for %s => %p\n",argName,oldValue); */

    /*
     * Existing per-object vars take precedence (could have been set
     * through a mixin or filter)
     */
    if (oldValue == NULL) {

      /* TODO: should not be relation handling here and subst handling
         in canonicalNonpositionalArgs(); we do subst handling here due to reference counting */

      if (ifPtr->flags & XOTCL_ARG_SUBST_DEFAULT) {
        result = SubstValue(interp, obj, &newValue);
        if (result != TCL_OK) {
          parseContextRelease(&pc);
          goto configure_exit;
        }
        fprintf(stderr, "substituted value for attribute %s => %p '%s'\n", argName,
                newValue,ObjStr(newValue));
      }

#if defined(CONFIGURE_ARGS_TRACE)
      fprintf(stderr, "*** %s SET %s '%s'\n",objectName(obj),argName, ObjStr(newValue));
#endif
      Tcl_ObjSetVar2(interp, argNameObj, NULL, newValue, TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
    } else {
#if defined(CONFIGURE_ARGS_TRACE)
      fprintf(stderr, "*** no need to set, we have already '%s' for arg '%s'\n",ObjStr(oldValue),argName);
#endif
    }
    DECR_REF_COUNT(argNameObj);
  }

  XOTcl_PopFrame(interp, obj);
  remainingArgsc = pc.objc-(nonposArgs->ifdSize-1);

#if defined(CONFIGURE_ARGS_TRACE)
  fprintf(stderr, "*** POPULATE OBJ SETVALUES with '%d' elements:\n",remainingArgsc);
  { int j;
    for (j = i; j < i + remainingArgsc; j++) {
      fprintf(stderr, "*** SETVALUES[%d] with '%s'\n",j,ObjStr(pc.full_objv[j]));
    }
  }
#endif

  if (remainingArgsc > 0) {
    result = callMethod((ClientData) obj, interp,
                        XOTclGlobalObjects[XOTE_SETVALUES], remainingArgsc+2, pc.full_objv+i, 0);
    if (result != TCL_OK) {
      /* TODO: interp reset achieved by Object->setvalues?*/
      parseContextRelease(&pc);
      goto configure_exit;
    }
  } else {
    Tcl_ResetResult(interp);
    Tcl_SetObjResult(interp,XOTclGlobalObjects[XOTE_EMPTY]);
  }
  parseContextRelease(&pc);
#endif

#if !defined(CONFIGURE_ARGS)
    XOTclObjects *slotObjects, *so;
  /* would be nice to do it here instead of setValue
     XOTcl_FrameDecls;

     XOTcl_PushFrame(interp, obj); make instvars of obj accessible */

  /*
   * Search for default values on slots
   */
  slotObjects = computeSlotObjects(interp, obj, NULL, 0);
  for (so = slotObjects; so; so = so->nextPtr) {
    result = setDefaultValue(interp, obj, so->obj);
    if (result != TCL_OK) {
      goto configure_exit;
    }
  }

  /*
   * call configure methods (starting with '-')
   */
  /*{ int i;
    fprintf(stderr, "call setvalues %d: ",objc+1);
    for (i=0; i<objc; i++) {fprintf(stderr, "[%d]'%s' ",i,ObjStr(objv[i]));}
    fprintf(stderr,"\n");
    }*/
  result = callMethod((ClientData) obj, interp,
		      XOTclGlobalObjects[XOTE_SETVALUES], objc+1, objv+1, 0);
  if (result != TCL_OK) {
    goto configure_exit;
  }
#if 1
  /*
   * Check, if we got the required values
   */
  for (so = slotObjects; so; so = so->nextPtr) {
    result = checkRequiredValue(interp, obj, so->obj);
    if (result != TCL_OK) {
      goto configure_exit;
    }
  }
#endif
#endif
 configure_exit:
#if defined(CONFIGURE_ARGS)
  if(parsedIf.nonposArgs) {
    if (RUNTIME_STATE(interp)->cacheInterface == 0) {
      NonposArgsFree(parsedIf.nonposArgs);
    }
  }
#else
  if (slotObjects) {
    XOTclObjectListFree(slotObjects);
  }
#endif
  return result;
}

static int XOTclODestroyMethod(Tcl_Interp *interp, XOTclObject *obj) {
  PRINTOBJ("XOTclODestroyMethod", obj);
  /* XOTCL_DESTROY_CALLED might be set already be callDestroyMethod(),
   * the implicit destroy calls. It is necessary to set it here for
   * the explicit destroy calls in the script, which reach the
   * Object->destroy. */
  if ((obj->flags & XOTCL_DESTROY_CALLED) == 0) {
    obj->flags |= XOTCL_DESTROY_CALLED;
  }

  if ((obj->flags & XOTCL_DURING_DELETE) == 0) {
    return XOTclCallMethodWithArgs((ClientData)obj->cl, interp,
                                   XOTclGlobalObjects[XOTE_DEALLOC], obj->cmdName,
                                   1, NULL, 0);
  } else {
#if defined(OBJDELETION_TRACE)
    fprintf(stderr, "  Object->destroy already during delete, don't call dealloc %p\n", obj);
#endif
    return TCL_OK;
  }
}

static int XOTclOExistsMethod(Tcl_Interp *interp, XOTclObject *obj, char *var) {
  Tcl_SetIntObj(Tcl_GetObjResult(interp), varExists(interp, obj, var, NULL, 1, 1));
  return TCL_OK;
}

static int XOTclOFilterGuardMethod(Tcl_Interp *interp, XOTclObject *obj, char *filter, Tcl_Obj *guard) {
  XOTclObjectOpt *opt = obj->opt;

  if (opt && opt->filters) {
    XOTclCmdList *h = CmdListFindNameInList(interp, filter, opt->filters);
    if (h) {
      if (h->clientData)
        GuardDel((XOTclCmdList*) h);
      GuardAdd(interp, h, guard);
      obj->flags &= ~XOTCL_FILTER_ORDER_VALID;
      return TCL_OK;
    }
  }

  return XOTclVarErrMsg(interp, "Filterguard: can't find filter ",
                        filter, " on ", objectName(obj), (char *) NULL);
}

/*
 *  Searches for filter on [self] and returns fully qualified name
 *  if it is not found it returns an empty string
 */
static int XOTclOFilterSearchMethod(Tcl_Interp *interp, XOTclObject *obj, char *filter) {
  XOTclCmdList *cmdList;
  XOTclClass *fcl;
  XOTclObject *fobj;

  Tcl_ResetResult(interp);

  if (!(obj->flags & XOTCL_FILTER_ORDER_VALID))
    FilterComputeDefined(interp, obj);
  if (!(obj->flags & XOTCL_FILTER_ORDER_DEFINED))
    return TCL_OK;

  for (cmdList = obj->filterOrder; cmdList;  cmdList = cmdList->nextPtr) {
    CONST char *filterName = Tcl_GetCommandName(interp, cmdList->cmdPtr);
    if (filterName[0] == filter[0] && !strcmp(filterName, filter))
      break;
  }

  if (!cmdList)
    return TCL_OK;

  fcl = cmdList->clorobj;
  if (fcl && XOTclObjectIsClass(&fcl->object)) {
    fobj = NULL;
  } else {
    fobj = (XOTclObject*)fcl;
    fcl = NULL;
  }

  Tcl_SetObjResult(interp, getFullProcQualifier(interp, filter, fobj, fcl,
                                                cmdList->cmdPtr));
  return TCL_OK;
}

static int GetInstvarsIntoCurrentScope(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]) {
  int i, result = TCL_OK;

  for (i=1; i<objc; i++) {
    Tcl_Obj  **ov;
    int oc;

    /*fprintf(stderr,"ListGetElements %p %s\n", objv[i], ObjStr(objv[i]));*/
    if ((result = Tcl_ListObjGetElements(interp, objv[i], &oc, &ov)) == TCL_OK) {
      Tcl_Obj *varname = NULL, *alias = NULL;
      switch (oc) {
      case 0: {varname = objv[i]; break;}
      case 1: {varname = ov[0];   break;}
      case 2: {varname = ov[0];   alias = ov[1]; break;}
      }
      if (varname) {
        result = GetInstVarIntoCurrentScope(interp, obj, varname, alias);
      } else {
        result = XOTclVarErrMsg(interp, "invalid variable specification '",
                                ObjStr(objv[i]), "'", (char *) NULL);
      }
      if (result != TCL_OK) {
        break;	
      }
    } else {
      break;
    }
  }
  return result;
}

static int XOTclOInstVarMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]) {
  callFrameContext ctx = {0};
  int result;

  if (obj && (obj->filterStack || obj->mixinStack) ) {
    CallStackUseActiveFrames(interp, &ctx);
  }
  if (!Tcl_Interp_varFramePtr(interp)) {
    CallStackRestoreSavedFrames(interp, &ctx);
    return XOTclVarErrMsg(interp, "instvar used on ", objectName(obj),
                          ", but callstack is not in procedure scope",
			  (char *) NULL);
  }
  
  result = GetInstvarsIntoCurrentScope(interp, obj, objc, objv);
  CallStackRestoreSavedFrames(interp, &ctx);
  return result;
}

static int XOTclOInvariantsMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *invariantlist) {
  XOTclObjectOpt *opt = XOTclRequireObjectOpt(obj);

  if (opt->assertions)
    TclObjListFreeList(opt->assertions->invariants);
  else
    opt->assertions = AssertionCreateStore();

  opt->assertions->invariants = AssertionNewList(interp, invariantlist);
  return TCL_OK;
}

static int XOTclOIsClassMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *class) {
  XOTclObject *o;
  Tcl_SetIntObj(Tcl_GetObjResult(interp),
                (GetObjectFromObj(interp, class ? class : obj->cmdName, &o) == TCL_OK
		 && XOTclObjectIsClass(o) ));
  return TCL_OK;
}

static int XOTclOIsMetaClassMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *metaclass) {
  XOTclObject *o;
  if (GetObjectFromObj(interp, metaclass ? metaclass : obj->cmdName, &o) == TCL_OK
      && XOTclObjectIsClass(o)
      && IsMetaClass(interp, (XOTclClass*)o, 1)) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
  } else {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
  }
  return TCL_OK;
}

static int XOTclOIsMixinMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *class) {
  XOTclClass *cl;
  int success = 0;

  if (GetClassFromObj(interp, class, &cl, obj->cl) == TCL_OK) {
    success = hasMixin(interp, obj, cl);
  }
  Tcl_ResetResult(interp);
  Tcl_SetIntObj(Tcl_GetObjResult(interp), success);
  return TCL_OK;
}

static int XOTclOIsObjectMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *object) {
  XOTclObject *o;
  Tcl_SetIntObj(Tcl_GetObjResult(interp), (GetObjectFromObj(interp, object, &o) == TCL_OK));
  return TCL_OK;
}

static int XOTclOIsTypeMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *class) {
  XOTclClass *cl;
  int success = 0;

  if (obj->cl && GetClassFromObj(interp, class, &cl, obj->cl) == TCL_OK) {
    success = isSubType(obj->cl, cl);
  }
  Tcl_ResetResult(interp);
  Tcl_SetIntObj(Tcl_GetObjResult(interp), success);
  return TCL_OK;
}

static int XOTclOMixinGuardMethod(Tcl_Interp *interp, XOTclObject *obj, char *mixin, Tcl_Obj *guard) {
  XOTclObjectOpt *opt = obj->opt;

  if (opt && opt->mixins) {
    XOTclClass *mixinCl = XOTclpGetClass(interp, mixin);
    Tcl_Command mixinCmd = NULL;
    if (mixinCl) {
      mixinCmd = Tcl_GetCommandFromObj(interp, mixinCl->object.cmdName);
    }
    if (mixinCmd) {
      XOTclCmdList *h = CmdListFindCmdInList(mixinCmd, opt->mixins);
      if (h) {
        if (h->clientData)
          GuardDel((XOTclCmdList*) h);
        GuardAdd(interp, h, guard);
        obj->flags &= ~XOTCL_MIXIN_ORDER_VALID;
        return TCL_OK;
      }
    }
  }

  return XOTclVarErrMsg(interp, "Mixinguard: can't find mixin ",
                        mixin, " on ", objectName(obj), (char *) NULL);
}

/* method for calling e.g.  $obj __next  */
static int XOTclONextMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]) {
  XOTclCallStackContent *csc = CallStackGetObjectFrame(interp, obj);
  char *methodName;

  if (!csc)
    return XOTclVarErrMsg(interp, "__next: can't find object",
			  objectName(obj), (char *) NULL);
  methodName = (char *)Tcl_GetCommandName(interp, csc->cmdPtr);
  return XOTclNextMethod(obj, interp, csc->cl, methodName, objc-1, &objv[1], 0, NULL);
}

static int XOTclONoinitMethod(Tcl_Interp *interp, XOTclObject *obj) {
  obj->flags |= XOTCL_INIT_CALLED;
  return TCL_OK;
}

static int XOTclOParametercmdMethod(Tcl_Interp *interp, XOTclObject *obj, char *name) {
  XOTclAddObjectMethod(interp, (XOTcl_Object*) obj, name, (Tcl_ObjCmdProc*)XOTclSetterMethod, 0, 0, 0);
  return TCL_OK;
}

static int XOTclOProcMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *name,
                            Tcl_Obj *args, Tcl_Obj *body,
                            Tcl_Obj *precondition, Tcl_Obj *postcondition) {
  char *argStr = ObjStr(args), *bdyStr = ObjStr(body), *nameStr = ObjStr(name);
  int result;

  if (precondition && !postcondition) {
    return XOTclVarErrMsg(interp, objectName(obj), " method '", nameStr,
                          "'; when specifying a precondition (", ObjStr(precondition),
                          ") a postcondition must be specified as well",
                          (char *) NULL);
  }

  /* if both, args and body are empty strings, we delete the method */
  if (*argStr == 0 && *bdyStr == 0) {
    result = XOTclRemovePMethod(interp, (XOTcl_Object *)obj, nameStr);

  } else {
    XOTclAssertionStore *aStore = NULL;
    if (precondition || postcondition) {
      XOTclObjectOpt *opt = XOTclRequireObjectOpt(obj);
      if (!opt->assertions)
	opt->assertions = AssertionCreateStore();
      aStore = opt->assertions;
    }
    requireObjNamespace(interp, obj);
    result = MakeProc(obj->nsPtr, aStore,
                      interp, name, args, body, precondition, postcondition,
		      obj, 0);
  }

  /* could be a filter => recompute filter order */
  FilterComputeDefined(interp, obj);
  return result;
}

static int XOTclOProcSearchMethod(Tcl_Interp *interp, XOTclObject *obj, char *name) {
  XOTclClass *pcl = NULL;
  Tcl_Command cmd = NULL;

  Tcl_ResetResult(interp);

  if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, obj);

  if (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
    XOTclCmdList *mixinList;
    for (mixinList = obj->mixinOrder; mixinList; mixinList = mixinList->nextPtr) {
      XOTclClass *mcl = XOTclpGetClass(interp, (char *)Tcl_GetCommandName(interp, mixinList->cmdPtr));
      if (mcl && (pcl = SearchCMethod(mcl, name, &cmd))) {
        break;
      }
    }
  }

  if (!cmd && obj->nsPtr) {
    cmd = FindMethod(name, obj->nsPtr);
  }

  if (!cmd && obj->cl)
    pcl = SearchCMethod(obj->cl, name, &cmd);

  if (cmd) {
    XOTclObject *pobj = pcl ? NULL : obj;
    char *simpleName = (char *)Tcl_GetCommandName(interp, cmd);
    Tcl_SetObjResult(interp, getFullProcQualifier(interp, simpleName, pobj, pcl, cmd));
  }
  return TCL_OK;
}

static int XOTclORequireNamespaceMethod(Tcl_Interp *interp, XOTclObject *obj) {
  requireObjNamespace(interp, obj);
  return TCL_OK;
}

static int XOTclOSetMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *var, Tcl_Obj *value) {
  return setInstVar(interp, obj, var, value);
}

static int XOTclOSetvaluesMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj **argv, **nextArgv, *resultObj;
  int i, argc, nextArgc, normalArgs, result = TCL_OK, isdasharg = NO_DASH;
  char *methodName, *nextMethodName;

  /* find arguments without leading dash */
  for (i=1; i < objc; i++) {
    if ((isdasharg = isDashArg(interp, objv[i], &methodName, &argc, &argv)))
      break;
  }
  normalArgs = i-1;
  Tcl_ResetResult(interp);

  for( ; i < objc;  argc=nextArgc, argv=nextArgv, methodName=nextMethodName) {
    Tcl_ResetResult(interp);
    switch (isdasharg) {
    case SKALAR_DASH:    /* Argument is a skalar with a leading dash */
      { int j;
        for (j = i+1; j < objc; j++, argc++) {
          if ((isdasharg = isDashArg(interp, objv[j], &nextMethodName, &nextArgc, &nextArgv)))
            break;
        }
        result = callConfigureMethod(interp, obj, methodName, argc+1, objv+i+1);
        if (result != TCL_OK) {
          return result;
	}
	i += argc;
	break;
      }
    case LIST_DASH:  /* Argument is a list with a leading dash, grouping determined by list */
      {	i++;
	if (i<objc)
	  isdasharg = isDashArg(interp, objv[i], &nextMethodName, &nextArgc, &nextArgv);
	result = callConfigureMethod(interp, obj, methodName, argc+1, argv+1);
	if (result != TCL_OK)
	  return result;
	break;
      }
    default:
      {
	return XOTclVarErrMsg(interp, objectName(obj),
			      " configure: unexpected argument '",
			      ObjStr(objv[i]),
			      "' between parameters", (char *) NULL);
      }
    }
  }
  resultObj = Tcl_NewListObj(normalArgs, objv+1);
  //fprintf(stderr,".... setvalues returns %s\n", ObjStr(resultObj));
  Tcl_SetObjResult(interp, resultObj);

  return result;
}

static int XOTclOForwardMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *method,
                               Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix,
                               int withObjscope, Tcl_Obj *withOnerror, int withVerbose, Tcl_Obj *target,
                               int nobjc, Tcl_Obj *CONST nobjv[]) {
  forwardCmdClientData *tcd;
  int rc = forwardProcessOptions(interp, method,
                                 withDefault, withEarlybinding, withMethodprefix,
                                 withObjscope, withOnerror, withVerbose,
                                 target, nobjc, nobjv, &tcd);
  if (rc == TCL_OK) {
    tcd->obj = obj;
    XOTclAddPMethod(interp, (XOTcl_Object *)obj, NSTail(ObjStr(method)),
                    (Tcl_ObjCmdProc*)XOTclForwardMethod,
                    (ClientData)tcd, forwardCmdDeleteProc);
  }
  return rc;
}

static int XOTclOUplevelMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]) {
  int i, result = TCL_ERROR;
  char *frameInfo = NULL;
  Tcl_CallFrame *framePtr = NULL, *savedVarFramePtr;

  /*
   * Find the level to use for executing the command.
   */
  if (objc>2) {
    CallFrame *cf;
    frameInfo = ObjStr(objv[1]);
    result = TclGetFrame(interp, frameInfo, &cf);
    if (result == -1) {
      return TCL_ERROR;
    }
    framePtr = (Tcl_CallFrame *)cf;
    i = result+1;
  } else {
    i = 1;
  }

  objc -= i;
  objv += i;

  if (!framePtr) {
    XOTclCallStackFindLastInvocation(interp, 1, &framePtr);
  }

  savedVarFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  Tcl_Interp_varFramePtr(interp) = (CallFrame *)framePtr;

  /*
   * Execute the residual arguments as a command.
   */

  if (objc == 1) {
    result = Tcl_EvalObjEx(interp, objv[0], TCL_EVAL_DIRECT);
  } else {
    /*
     * More than one argument: concatenate them together with spaces
     * between, then evaluate the result.  Tcl_EvalObjEx will delete
     * the object when it decrements its refcount after eval'ing it.
     */
    Tcl_Obj *objPtr = Tcl_ConcatObj(objc, objv);
    result = Tcl_EvalObjEx(interp, objPtr, TCL_EVAL_DIRECT);
  }
  if (result == TCL_ERROR) {
    char msg[32 + TCL_INTEGER_SPACE];
    sprintf(msg, "\n    (\"uplevel\" body line %d)", interp->errorLine);
    Tcl_AddObjErrorInfo(interp, msg, -1);
  }

  /*
   * Restore the variable frame, and return.
   */

  Tcl_Interp_varFramePtr(interp) = (CallFrame *)savedVarFramePtr;
  return result;
}

static int XOTclOUpvarMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj *frameInfoObj = NULL;
  int i, result = TCL_ERROR;
  char *frameInfo;
  callFrameContext ctx = {0};

  if (objc % 2 == 0) {
    frameInfo = ObjStr(objv[1]);
    i = 2;
  } else {
    frameInfoObj = computeLevelObj(interp, CALLING_LEVEL);
    INCR_REF_COUNT(frameInfoObj);
    frameInfo = ObjStr(frameInfoObj);
    i = 1;
  }

  if (obj && (obj->filterStack || obj->mixinStack)) {
    CallStackUseActiveFrames(interp, &ctx);
  }

  for ( ;  i < objc;  i += 2) {
    result = Tcl_UpVar2(interp, frameInfo, ObjStr(objv[i]), NULL,
                        ObjStr(objv[i+1]), 0 /*flags*/);
    if (result != TCL_OK)
      break;
  }

  if (frameInfoObj) {
    DECR_REF_COUNT(frameInfoObj);
  }
  CallStackRestoreSavedFrames(interp, &ctx);
  return result;
}

static int XOTclOVolatileMethod(Tcl_Interp *interp, XOTclObject *obj) {
  Tcl_Obj *o = obj->cmdName;
  int result = TCL_ERROR;
  CONST char *fullName = ObjStr(o);
  CONST char *vn;
  callFrameContext ctx = {0};

  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound != XOTCL_EXITHANDLER_OFF) {
    fprintf(stderr,"### Can't make objects volatile during shutdown\n");
    return XOTclVarErrMsg(interp, "Can't make objects volatile during shutdown\n", NULL);
  }

  CallStackUseActiveFrames(interp, &ctx);
  vn = NSTail(fullName);

  if (Tcl_SetVar2(interp, vn, NULL, fullName, 0)) {
    XOTclObjectOpt *opt = XOTclRequireObjectOpt(obj);

    /*fprintf(stderr,"### setting trace for %s\n", fullName);*/
    result = Tcl_TraceVar(interp, vn, TCL_TRACE_UNSETS,
			  (Tcl_VarTraceProc*)XOTclUnsetTrace,
                          (ClientData)o);
    opt->volatileVarName = vn;
  }
  CallStackRestoreSavedFrames(interp, &ctx);

  if (result == TCL_OK) {
    INCR_REF_COUNT(o);
  }
  return result;
}

static int XOTclOVwaitMethod(Tcl_Interp *interp, XOTclObject *obj, char *varname) {
  int done, foundEvent;
  int flgs = TCL_TRACE_WRITES|TCL_TRACE_UNSETS;
  XOTcl_FrameDecls;

  /*
   * Make sure the var table exists and the varname is in there
   */
  if (NSRequireVariableOnObj(interp, obj, varname, flgs) == 0)
    return XOTclVarErrMsg(interp, "Can't lookup (and create) variable ",
                          varname, " on ", objectName(obj), (char *) NULL);

  XOTcl_PushFrame(interp, obj);
  /*
   * much of this is copied from Tcl, since we must avoid
   * access with flag TCL_GLOBAL_ONLY ... doesn't work on
   * obj->varTable vars
   */
  if (Tcl_TraceVar(interp, varname, flgs, (Tcl_VarTraceProc *)VwaitVarProc,
                   (ClientData) &done) != TCL_OK) {
    return TCL_ERROR;
  }
  done = 0;
  foundEvent = 1;
  while (!done && foundEvent) {
    foundEvent = Tcl_DoOneEvent(TCL_ALL_EVENTS);
  }
  Tcl_UntraceVar(interp, varname, flgs, (Tcl_VarTraceProc *)VwaitVarProc,
                 (ClientData) &done);
  XOTcl_PopFrame(interp, obj);
  /*
   * Clear out the interpreter's result, since it may have been set
   * by event handlers.
   */
  Tcl_ResetResult(interp);

  if (!foundEvent) {
    return XOTclVarErrMsg(interp, "can't wait for variable '", varname,
			  "':  would wait forever", (char *) NULL);
  }
  return TCL_OK;
}


/***************************
 * End Object Methods
 ***************************/


/***************************
 * Begin Class Methods
 ***************************/

static int XOTclCAllocMethod(Tcl_Interp *interp, XOTclClass *cl, char *name) {
  Tcl_Obj *tmpName = NULL;
  int result;

  /*
   * create a new object from scratch
   */

  /*fprintf(stderr, " **** 0 class '%s' wants to alloc '%s'\n",className(cl),name);*/
  if (!NSCheckColons(name, 0)) {
    return XOTclVarErrMsg(interp, "Cannot allocate object -- illegal name '",
                          name, "'", (char *) NULL);
  }

  /*
   * If the path is not absolute, we add the appropriate namespace
   */
  if (!isAbsolutePath(name)) {
    tmpName = NameInNamespaceObj(interp, name, callingNameSpace(interp));
    /*fprintf(stderr, " **** NoAbsoluteName for '%s' -> determined = '%s'\n",
      name, ObjStr(tmpName));*/
    name = ObjStr(tmpName);
    INCR_REF_COUNT(tmpName);
  }

  if (IsMetaClass(interp, cl, 1)) {
    /*
     * if the base class is a meta-class, we create a class
     */
    XOTclClass *newcl = PrimitiveCCreate(interp, name, cl);
    if (newcl == 0) {
      result = XOTclVarErrMsg(interp, "Class alloc failed for '", name,
                              "' (possibly parent namespace does not exist)",
                              (char *) NULL);
    } else {
      Tcl_SetObjResult(interp, newcl->object.cmdName);
      result = TCL_OK;
    }
  } else {
    /*
     * if the base class is an ordinary class, we create an object
     */
    XOTclObject *newObj = PrimitiveOCreate(interp, name, cl);
    if (newObj == 0)
      result = XOTclVarErrMsg(interp, "Object alloc failed for '", name,
                              "' (possibly parent namespace does not exist)",
                              (char *) NULL);
    else {
      result = TCL_OK;
      Tcl_SetObjResult(interp, newObj->cmdName);
    }
  }

  if (tmpName) {
    DECR_REF_COUNT(tmpName);
  }

  return result;
}

static int XOTclCCreateMethod(Tcl_Interp *interp, XOTclClass *cl, char *name,
                              int objc, Tcl_Obj *CONST objv[]) {
  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound != XOTCL_EXITHANDLER_OFF) {
    fprintf(stderr,"### Can't create object %s during shutdown\n", ObjStr(objv[1]));
    return TCL_OK; /* don't fail, if this happens during destroy, it might be canceled */
  }

  return createMethod(interp, cl, name, objc, objv);
}

static int XOTclCDeallocMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *object) {
  XOTclObject *delobj;
  int rc;

  /*fprintf(stderr,"  dealloc %s\n",ObjStr(object));*/

  if (GetObjectFromObj(interp, object, &delobj) != TCL_OK)
    return XOTclVarErrMsg(interp, "Can't destroy object ",
                          ObjStr(object), " that does not exist.",
			  (char *) NULL);

  /* fprintf(stderr,"dealloc obj=%s, opt=%p\n", objectName(delobj), delobj->opt);*/
  rc = freeUnsetTraceVariable(interp, delobj);
  if (rc != TCL_OK) {
    return rc;
  }

  /*
   * latch, and call delete command if not already in progress
   */
  
  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound !=
      XOTCL_EXITHANDLER_ON_SOFT_DESTROY) {
    CallStackDestroyObject(interp, delobj);
  }

  return TCL_OK;
}

static int XOTclCNewMethod(Tcl_Interp *interp, XOTclClass *cl, XOTclObject *withChildof,
                           int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj *fullname;
  int result, prefixLength;
  Tcl_DString dFullname, *dsPtr = &dFullname;
  XOTclStringIncrStruct *iss = &RUNTIME_STATE(interp)->iss;

  Tcl_DStringInit(dsPtr);
  if (withChildof) {
    Tcl_DStringAppend(dsPtr, objectName(withChildof), -1);
    Tcl_DStringAppend(dsPtr, "::__#", 5);
  } else {
    Tcl_DStringAppend(dsPtr, "::xotcl::__#", 12);
  }
  prefixLength = dsPtr->length;

  while (1) {
    (void)XOTclStringIncr(iss);
    Tcl_DStringAppend(dsPtr, iss->start, iss->length);
    if (!Tcl_FindCommand(interp, Tcl_DStringValue(dsPtr), NULL, 0)) {
      break;
    }
    /* in case the value existed already, reset prefix to the
       original length */
    Tcl_DStringSetLength(dsPtr, prefixLength);
  }

  fullname = Tcl_NewStringObj(Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr));

  INCR_REF_COUNT(fullname);

  {
    ALLOC_ON_STACK(Tcl_Obj*, objc+3, ov);

    ov[0] = objv[0];
    ov[1] = XOTclGlobalObjects[XOTE_CREATE];
    ov[2] = fullname;
    if (objc >= 1)
      memcpy(ov+3, objv, sizeof(Tcl_Obj *)*objc);

    result = DoDispatch((ClientData)cl, interp, objc+3, ov, 0);
    FREE_ON_STACK(ov);
  }

  DECR_REF_COUNT(fullname);
  Tcl_DStringFree(dsPtr);

  return result;
}

static int XOTclCInstFilterGuardMethod(Tcl_Interp *interp, XOTclClass *cl, char *filter, Tcl_Obj *guard) {
  XOTclClassOpt *opt = cl->opt;

  if (opt && opt->instfilters) {
    XOTclCmdList *h = CmdListFindNameInList(interp, filter, opt->instfilters);
    if (h) {
      if (h->clientData)
	GuardDel(h);
      GuardAdd(interp, h, guard);
      FilterInvalidateObjOrders(interp, cl);
      return TCL_OK;
    }
  }

  return XOTclVarErrMsg(interp, "Instfilterguard: can't find filter ",
			filter, " on ", className(cl),	(char *) NULL);
}

static int XOTclCInvariantsMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *invariantlist) {
  XOTclClassOpt *opt = XOTclRequireClassOpt(cl);

  if (opt->assertions)
    TclObjListFreeList(opt->assertions->invariants);
  else
    opt->assertions = AssertionCreateStore();

  opt->assertions->invariants = AssertionNewList(interp, invariantlist);
  return TCL_OK;
}

static int XOTclCInstMixinGuardMethod(Tcl_Interp *interp, XOTclClass *cl, char *mixin, Tcl_Obj *guard) {
  XOTclClassOpt *opt = cl->opt;
  XOTclCmdList *h;

  if (opt && opt->instmixins) {
    XOTclClass *mixinCl = XOTclpGetClass(interp, mixin);
    Tcl_Command mixinCmd = NULL;
    if (mixinCl) {
      mixinCmd = Tcl_GetCommandFromObj(interp, mixinCl->object.cmdName);
    }
    if (mixinCmd) {
      h = CmdListFindCmdInList(mixinCmd, opt->instmixins);
      if (h) {
        if (h->clientData)
          GuardDel((XOTclCmdList*) h);
        GuardAdd(interp, h, guard);
        MixinInvalidateObjOrders(interp, cl);
        return TCL_OK;
      }
    }
  }

  return XOTclVarErrMsg(interp, "Instmixinguard: can't find mixin ",
                        mixin, " on ", className(cl), (char *) NULL);
}

static int XOTclCInstParametercmdMethod(Tcl_Interp *interp, XOTclClass *cl, char *name) {
  XOTclAddInstanceMethod(interp, (XOTcl_Class *)cl, name, (Tcl_ObjCmdProc*)XOTclSetterMethod, 0, 0, 0);
  return TCL_OK;
}

static int XOTclCInstProcMethod(Tcl_Interp *interp, XOTclClass *cl,
                                Tcl_Obj *name, Tcl_Obj *args, Tcl_Obj *body,
                                Tcl_Obj *precondition, Tcl_Obj *postcondition) {
  return makeMethod(interp, cl, name, args, body, precondition, postcondition, 0);
}

static int XOTclCInstProcMethodC(Tcl_Interp *interp, XOTclClass *cl,
                                 Tcl_Obj *name, Tcl_Obj *args, Tcl_Obj *body,
                                 Tcl_Obj *precondition, Tcl_Obj *postcondition) {
  return makeMethod(interp, cl, name, args, body, precondition, postcondition, 1);
}


static int XOTclCInstForwardMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *method,
                                   Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix,
                                   int withObjscope, Tcl_Obj *withOnerror, int withVerbose,
                                   Tcl_Obj *target, int nobjc, Tcl_Obj *CONST nobjv[]) {
  forwardCmdClientData *tcd;
  int rc;

  rc = forwardProcessOptions(interp, method,
			     withDefault, withEarlybinding, withMethodprefix,
			     withObjscope, withOnerror, withVerbose,
			     target, nobjc, nobjv, &tcd);

  if (rc == TCL_OK) {
    tcd->obj = &cl->object;
    XOTclAddIMethod(interp, (XOTcl_Class*)cl, NSTail(ObjStr(method)),
                    (Tcl_ObjCmdProc*)XOTclForwardMethod,
                    (ClientData)tcd, forwardCmdDeleteProc);
  }
  return rc;
}

static int XOTclCInvalidateInterfaceDefinitionMethod(Tcl_Interp *interp, XOTclClass *cl) {
  fprintf(stderr, "   %s invalidate %p\n", className(cl), cl->parsedIf);
  if (cl->parsedIf) {
    ParsedInterfaceDefinitionFree(cl->parsedIf);
    cl->parsedIf = NULL;
  }
  return TCL_OK;
}

static int XOTclCRecreateMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *name,
                                int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *newObj;
  int result;

  if (GetObjectFromObj(interp, name, &newObj) != TCL_OK)
    return XOTclVarErrMsg(interp, "can't recreate non existing object ",
                          ObjStr(name), (char *) NULL);
  INCR_REF_COUNT(name);
  newObj->flags |= XOTCL_RECREATE;
  result = doCleanup(interp, newObj, &cl->object, objc, objv);
  if (result == TCL_OK) {
    result = doObjInitialization(interp, newObj, objc, objv);
    if (result == TCL_OK)
      Tcl_SetObjResult(interp, name);
  }
  DECR_REF_COUNT(name);
  return result;
}

static int XOTclCUnknownMethod(Tcl_Interp *interp, XOTclClass *cl, char *name,
                               int objc, Tcl_Obj *CONST objv[]) {
  if (isCreateString(name))
    return XOTclVarErrMsg(interp, "error ", className(cl), ": unable to dispatch '",
			  name, "'", (char *)NULL);

  return callMethod((ClientData)cl, interp, XOTclGlobalObjects[XOTE_CREATE], objc+1, objv+1, 0);
}



/***************************
 * End Class Methods
 ***************************/


/***************************
 * Begin check Methods
 ***************************/
static int XOTclCheckBooleanArgs(Tcl_Interp *interp, char *name, Tcl_Obj *value) {
  int result, bool;
  Tcl_Obj *boolean;

  if (value == NULL) {
    /* the variable is not yet defined (set), so we cannot check
       whether it is boolean or not */
    return TCL_OK;
  }

  boolean = Tcl_DuplicateObj(value);
  INCR_REF_COUNT(boolean);
  result = Tcl_GetBooleanFromObj(interp, boolean, &bool);
  DECR_REF_COUNT(boolean);

  Tcl_ResetResult(interp);
  Tcl_SetIntObj(Tcl_GetObjResult(interp), (result == TCL_OK));
  return TCL_OK;
}

static int XOTclCheckRequiredArgs(Tcl_Interp *interp, char *name, Tcl_Obj *value) {
  if (value == NULL) {
    return XOTclVarErrMsg(interp, "required arg: '", name, "' missing",
                          (char *) NULL);
  }
  Tcl_ResetResult(interp);
  Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
  return TCL_OK;
}
/***************************
 * End check Methods
 ***************************/


/***************************
 * Begin Object Info Methods
 ***************************/

static int XOTclObjInfoArgsMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName) {
  Proc *proc = getObjectProc(interp, object, methodName);
  XOTclNonposArgs *nonposArgs = proc ? getNonposArgs((Tcl_Command)proc->cmdPtr) : NULL;

  if (nonposArgs) {
    return ListArgsFromOrdinaryArgs(interp, nonposArgs);
  }
  return ListProcArgs(interp, proc, methodName);
}

static int XOTclObjInfoBodyMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName) {
  Proc *proc = getObjectProc(interp, object, methodName);
  return ListProcBody(interp, proc, methodName);
}

static int XOTclObjInfoCheckMethod(Tcl_Interp *interp, XOTclObject *object) {
  return AssertionListCheckOption(interp, object);
}

static int XOTclObjInfoChildrenMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern) {
  return ListChildren(interp, object, pattern, 0);
}

static int XOTclObjInfoClassMethod(Tcl_Interp *interp, XOTclObject *object) {
  Tcl_SetObjResult(interp, object->cl->object.cmdName);
  return TCL_OK;
}

static int XOTclObjInfoCommandsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern) {
  return ListKeys(interp, Tcl_Namespace_cmdTable(object->nsPtr), pattern);
}

static int XOTclObjInfoDefaultMethod(Tcl_Interp *interp, XOTclObject *object, 
                                     char *methodName, char *arg, Tcl_Obj *var) {
  Proc *procPtr = getObjectProc(interp, object, methodName);
  XOTclNonposArgs *nonposArgs = procPtr ? getNonposArgs((Tcl_Command)procPtr->cmdPtr) : NULL;

  if (nonposArgs) {
      return ListDefaultFromOrdinaryArgs(interp, methodName, nonposArgs, arg, var);
  }
  return ListProcDefault(interp, procPtr, methodName, arg, var);
}

static int XOTclObjInfoFilterMethod(Tcl_Interp *interp, XOTclObject *object, 
                                    int withOrder, int withGuards, char *pattern) {
  XOTclObjectOpt *opt = object->opt;
  if (withOrder) {
    if (!(object->flags & XOTCL_FILTER_ORDER_VALID))
      FilterComputeDefined(interp, object);
    return FilterInfo(interp, object->filterOrder, pattern, withGuards, 1);
  }
  return opt ? FilterInfo(interp, opt->filters, pattern, withGuards, 0) : TCL_OK;
}

static int XOTclObjInfoFilterguardMethod(Tcl_Interp *interp, XOTclObject *object, char *filter) {
  return object->opt ? GuardList(interp, object->opt->filters, filter) : TCL_OK;
}

static int XOTclObjInfoForwardMethod(Tcl_Interp *interp, XOTclObject *object, int withDefinition, char *pattern) {
  return object->nsPtr ?
    ListForward(interp, Tcl_Namespace_cmdTable(object->nsPtr), pattern, withDefinition) :
    TCL_OK;
}

static int XOTclObjInfoHasnamespaceMethod(Tcl_Interp *interp, XOTclObject *object) {
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), object->nsPtr != NULL);
  return TCL_OK;
}

static int XOTclObjInfoInvarMethod(Tcl_Interp *interp, XOTclObject *object) {
  if (object->opt && object->opt->assertions) {
    Tcl_SetObjResult(interp, AssertionList(interp, object->opt->assertions->invariants));
  }
  return TCL_OK;
}

static int XOTclObjInfoMethodsMethod(Tcl_Interp *interp, XOTclObject *object, int withNoprocs,
                          int withNocmds, int withNomixins, int withIncontext, char *pattern) {
  return ListMethods(interp, object, pattern, withNoprocs, withNocmds, withNomixins, withIncontext);
}

static int XOTclObjInfoMixinMethod(Tcl_Interp *interp, XOTclObject *object, int withGuards, int withOrder,
                        char *patternString, XOTclObject *patternObj) {
  if (withOrder) {
    if (!(object->flags & XOTCL_MIXIN_ORDER_VALID))
      MixinComputeDefined(interp, object);
    return MixinInfo(interp, object->mixinOrder, patternString,
		     withGuards, patternObj);
  }
  return object->opt ? MixinInfo(interp, object->opt->mixins, patternString, withGuards, patternObj) : TCL_OK;
}

static int XOTclObjInfoMixinguardMethod(Tcl_Interp *interp, XOTclObject *object, char *mixin) {
  return object->opt ? GuardList(interp, object->opt->mixins, mixin) : TCL_OK;
}

static int XOTclObjInfoNonposargsMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName) {
  Proc *procPtr = getObjectProc(interp, object, methodName);
  XOTclNonposArgs *nonposArgs = procPtr ? getNonposArgs((Tcl_Command)procPtr->cmdPtr) : NULL;

  if (nonposArgs) {
    Tcl_SetObjResult(interp, NonposArgsFormat(interp, nonposArgs));
  }
  return TCL_OK;
}

static int XOTclObjInfoParametercmdMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern) {
  if (object->nsPtr) {
    return ListMethodKeys(interp, Tcl_Namespace_cmdTable(object->nsPtr), pattern, 1, 0, 0, 0, 1);
  }
  return TCL_OK;
}

static int XOTclObjInfoParentMethod(Tcl_Interp *interp, XOTclObject *object) {
  if (object->id) {
    Tcl_SetResult(interp, NSCmdFullName(object->id), TCL_VOLATILE);
  }
  return TCL_OK;
}

static int XOTclObjInfoPostMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName) {
  if (object->opt) {
    XOTclProcAssertion *procs = AssertionFindProcs(object->opt->assertions, methodName);
    if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->post));
  }
  return TCL_OK;
}

static int XOTclObjInfoPreMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName) {
  if (object->opt) {
    XOTclProcAssertion *procs = AssertionFindProcs(object->opt->assertions, methodName);
    if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->pre));
  }
  return TCL_OK;
}

static int XOTclObjInfoPrecedenceMethod(Tcl_Interp *interp, XOTclObject *object,
                                        int withIntrinsicOnly, char *pattern) {
  XOTclClasses *precedenceList = NULL, *pl;

  precedenceList = ComputePrecedenceList(interp, object, pattern, !withIntrinsicOnly, 1);
  for (pl = precedenceList; pl; pl = pl->nextPtr) {
    char *name = className(pl->cl);
    Tcl_AppendElement(interp, name);
  }
  XOTclClassListFree(pl);
  return TCL_OK;
}

static int XOTclObjInfoProcsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern) {
  return object->nsPtr ? ListMethodKeys(interp, Tcl_Namespace_cmdTable(object->nsPtr),
			      pattern, /*noProcs*/ 0, /*noCmds*/ 1, NULL, 0, 0) : TCL_OK;
}

static int XOTclObjInfoSlotObjectsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern) {
  XOTclObjects *pl;
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);

  pl = computeSlotObjects(interp, object, pattern /* not used */, 1);
  for (; pl; pl = pl->nextPtr) {
    Tcl_ListObjAppendElement(interp, list, pl->obj->cmdName);
  }

  XOTclObjectListFree(pl);
  Tcl_SetObjResult(interp, list);
  return TCL_OK;
}

static int XOTclObjInfoVarsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern) {
  Tcl_Obj *varlist, *okList, *element;
  int i, length;
  TclVarHashTable *varTable = object->nsPtr ? Tcl_Namespace_varTable(object->nsPtr) : object->varTable;

#if defined(PRE85)
# if FORWARD_COMPATIBLE
  if (forwardCompatibleMode) {
    ListVarKeys(interp, VarHashTable(varTable), pattern);
  } else {
    ListKeys(interp, varTable, pattern);
  }
# else
  ListKeys(interp, varTable, pattern);
# endif
#else
  ListVarKeys(interp, VarHashTable(varTable), pattern);
#endif
  varlist = Tcl_GetObjResult(interp);

  Tcl_ListObjLength(interp, varlist, &length);
  okList = Tcl_NewListObj(0, NULL);
  for (i=0; i<length; i++) {
    Tcl_ListObjIndex(interp, varlist, i, &element);
    if (varExists(interp, object, ObjStr(element), NULL, 0, 1)) {
      Tcl_ListObjAppendElement(interp, okList, element);
    } else {
      /*fprintf(stderr,"must ignore '%s' %d\n", ObjStr(element), i);*/
      /*Tcl_ListObjReplace(interp, varlist, i, 1, 0, NULL);*/
    }
  }
  Tcl_SetObjResult(interp, okList);
  return TCL_OK;
}
/***************************
 * End Object Info Methods
 ***************************/

/***************************
 * Begin Class Info methods
 ***************************/
static int XOTclClassInfoHeritageMethod(Tcl_Interp *interp, XOTclClass *cl, char *pattern) {
  XOTclClasses *pl = ComputeOrder(cl, cl->order, Super);
  Tcl_ResetResult(interp);
  if (pl) pl=pl->nextPtr;
  for (; pl; pl = pl->nextPtr) {
    AppendMatchingElement(interp, pl->cl->object.cmdName, pattern);
  }
  return TCL_OK;
}

/*
 * get all instances of a class recursively into an initialized
 * String key hashtable
 */
static int XOTclClassInfoInstancesMethod1(Tcl_Interp *interp, XOTclClass *startCl,
                              int withClosure, char *pattern, XOTclObject *matchObject) {
  Tcl_HashTable *table = &startCl->instances;
  XOTclClasses *sc;
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;
  int rc = 0;

  /*fprintf(stderr,"XOTclClassInfoInstancesMethod: clo %d pattern %s match %p\n",
    withClosure, pattern, matchObject);*/

  for (hPtr = Tcl_FirstHashEntry(table, &search);  hPtr;
       hPtr = Tcl_NextHashEntry(&search)) {
    XOTclObject *inst = (XOTclObject*) Tcl_GetHashKey(table, hPtr);
    /*fprintf(stderr, "match '%s' %p %p '%s'\n",
      matchObject ? objectName(matchObject) : "NULL" ,matchObject, inst, objectName(inst));*/
    if (matchObject && inst == matchObject) {
      Tcl_SetObjResult(interp, matchObject->cmdName);
      return 1;
    }
    AppendMatchingElement(interp, inst->cmdName, pattern);
  }
  if (withClosure) {
    for (sc = startCl->sub; sc; sc = sc->nextPtr) {
      rc = XOTclClassInfoInstancesMethod1(interp, sc->cl, withClosure, pattern, matchObject);
      if (rc) break;
    }
  }
  return rc;
}

static int XOTclClassInfoInstancesMethod(Tcl_Interp *interp, XOTclClass *startCl,
                              int withClosure, char *pattern, XOTclObject *matchObject) {
  XOTclClassInfoInstancesMethod1(interp, startCl, withClosure, pattern, matchObject);
  return TCL_OK;
}

static int XOTclClassInfoInstargsMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName) {
  Proc *proc = getClassProc(interp, class, methodName);
  XOTclNonposArgs *nonposArgs = proc ? getNonposArgs((Tcl_Command)proc->cmdPtr) : NULL;

  if (nonposArgs) {
    return ListArgsFromOrdinaryArgs(interp, nonposArgs);
  }
  return ListProcArgs(interp, proc, methodName);
}

static int XOTclClassInfoInstbodyMethod(Tcl_Interp *interp, XOTclClass *class, char * methodName) {
  Proc *proc = getClassProc(interp, class, methodName);
  return ListProcBody(interp, proc, methodName);
}

static int XOTclClassInfoInstcommandsMethod(Tcl_Interp *interp, XOTclClass * class, char * pattern) {
  return ListKeys(interp, Tcl_Namespace_cmdTable(class->nsPtr), pattern);
}

static int XOTclClassInfoInstdefaultMethod(Tcl_Interp *interp, XOTclClass *class,
                                char *methodName, char *arg, Tcl_Obj *var) {
  Proc *procPtr = getClassProc(interp, class, methodName);
  XOTclNonposArgs *nonposArgs = procPtr ? getNonposArgs((Tcl_Command)procPtr->cmdPtr) : NULL;

  if (nonposArgs) {
    return ListDefaultFromOrdinaryArgs(interp, methodName, nonposArgs, arg, var);
  }
  return ListProcDefault(interp, procPtr, methodName, arg, var);
}

static int XOTclClassInfoInstfilterMethod(Tcl_Interp *interp, XOTclClass * class, int withGuards, char * pattern) {
  return class->opt ? FilterInfo(interp, class->opt->instfilters, pattern, withGuards, 0) : TCL_OK;
}

static int XOTclClassInfoInstfilterguardMethod(Tcl_Interp *interp, XOTclClass * class, char * filter) {
  return class->opt ? GuardList(interp, class->opt->instfilters, filter) : TCL_OK;
}

static int XOTclClassInfoInstforwardMethod(Tcl_Interp *interp, XOTclClass *class,
				int withDefinition, char *pattern) {
  return ListForward(interp, Tcl_Namespace_cmdTable(class->nsPtr), pattern, withDefinition);
}

static int XOTclClassInfoInstinvarMethod(Tcl_Interp *interp, XOTclClass * class) {
  XOTclClassOpt *opt = class->opt;

  if (opt && opt->assertions) {
    Tcl_SetObjResult(interp, AssertionList(interp, opt->assertions->invariants));
  }
  return TCL_OK;
}

static int XOTclClassInfoInstmixinMethod(Tcl_Interp *interp, XOTclClass * class, int withClosure, int withGuards,
			      char *patternString, XOTclObject *patternObj) {
  XOTclClassOpt *opt = class->opt;
  int rc;

  /*fprintf(stderr, "XOTclClassInfoInstmixinMethod guard %d clo %d set %.4x pattern '%s'\n",
    withGuards,withClosure,patternString);*/

  if (withClosure) {
    Tcl_HashTable objTable, *commandTable = &objTable;
    MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
    Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
    rc = getAllClassMixins(interp, commandTable, class, withGuards, patternString, patternObj);
    if (patternObj && rc && !withGuards) {
      Tcl_SetObjResult(interp, rc ? patternObj->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
    }
    MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
  } else {
    rc = opt ? MixinInfo(interp, opt->instmixins, patternString, withGuards, patternObj) : TCL_OK;
  }

  return TCL_OK;
}

static int XOTclClassInfoInstmixinguardMethod(Tcl_Interp *interp, XOTclClass * class, char * mixin) {
  return class->opt ? GuardList(interp, class->opt->instmixins, mixin) : TCL_OK;
}

static int XOTclClassInfoInstmixinofMethod(Tcl_Interp *interp, XOTclClass * class, int withClosure,
				char *patternString, XOTclObject *patternObj) {
  XOTclClassOpt *opt = class->opt;
  int rc;

  if (opt) {
    if (withClosure) {
      Tcl_HashTable objTable, *commandTable = &objTable;
      MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
      Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
      rc = getAllClassMixinsOf(interp, commandTable, class, 0, 1, patternString, patternObj);
      MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
    } else {
      rc = AppendMatchingElementsFromCmdList(interp, opt->isClassMixinOf,
                                             patternString, patternObj);
    }
    if (patternObj) {
      Tcl_SetObjResult(interp, rc ? patternObj->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
    }
  }
  return TCL_OK;
}

static int XOTclClassInfoInstnonposargsMethod(Tcl_Interp *interp, XOTclClass * class, 
                                              char * methodName) {
  Proc *procPtr = getClassProc(interp, class, methodName);
  XOTclNonposArgs *nonposArgs = procPtr ? getNonposArgs((Tcl_Command)procPtr->cmdPtr) : NULL;

  if (nonposArgs) {
    Tcl_SetObjResult(interp, NonposArgsFormat(interp, nonposArgs));
  }
  return TCL_OK;
}

static int XOTclClassInfoInstparametercmdMethod(Tcl_Interp *interp, XOTclClass * class, char * pattern) {
  return ListMethodKeys(interp, Tcl_Namespace_cmdTable(class->nsPtr), pattern, 1, 0, 0, 0, 1);
}

static int XOTclClassInfoInstpostMethod(Tcl_Interp *interp, XOTclClass * class, char * methodName) {
  if (class->opt) {
    XOTclProcAssertion *procs = AssertionFindProcs(class->opt->assertions, methodName);
    if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->post));
  }
  return TCL_OK;
}

static int XOTclClassInfoInstpreMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName) {
  if (class->opt) {
    XOTclProcAssertion *procs = AssertionFindProcs(class->opt->assertions, methodName);
    if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->pre));
  }
  return TCL_OK;
}

static int XOTclClassInfoInstprocsMethod(Tcl_Interp *interp, XOTclClass * class, char * pattern) {
  return ListMethodKeys(interp, Tcl_Namespace_cmdTable(class->nsPtr),
			pattern, /*noProcs*/ 0, /*noCmds*/ 1, NULL, 0, 0 );
}

static int XOTclClassInfoMixinofMethod(Tcl_Interp *interp, XOTclClass * class, int withClosure,
                            char *patternString, XOTclObject *patternObj) {
  XOTclClassOpt *opt = class->opt;
  int rc = 0;

  if (opt && !withClosure) {
    rc = AppendMatchingElementsFromCmdList(interp, opt->isObjectMixinOf, patternString, patternObj);
  } else if (withClosure) {
    Tcl_HashTable objTable, *commandTable = &objTable;
    MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
    Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
    rc = getAllObjectMixinsOf(interp, commandTable, class, 0, 1, patternString, patternObj);
    MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
  }

  if (patternObj) {
    Tcl_SetObjResult(interp, rc ? patternObj->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
  }

  return TCL_OK;
}

static int XOTclClassInfoParameterMethod(Tcl_Interp *interp, XOTclClass * class) {
  Tcl_DString ds, *dsPtr = &ds;
  XOTclObject *obj;

  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, className(class), -1);
  Tcl_DStringAppend(dsPtr, "::slot", 6);
  obj = XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
  if (obj) {
    Tcl_Obj *varNameObj = Tcl_NewStringObj("__parameter",-1);
    Tcl_Obj *parameters = XOTcl_ObjGetVar2((XOTcl_Object*)obj,
					   interp, varNameObj, NULL,
					   TCL_LEAVE_ERR_MSG);
    if (parameters) {
      Tcl_SetObjResult(interp, parameters);
    }
    DECR_REF_COUNT(varNameObj);
  }
  DSTRING_FREE(dsPtr);
  return TCL_OK;
}

static int XOTclClassInfoSlotsMethod(Tcl_Interp *interp, XOTclClass * class) {
  Tcl_DString ds, *dsPtr = &ds;
  XOTclObject *obj;
  int rc;

  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, className(class), -1);
  Tcl_DStringAppend(dsPtr, "::slot", 6);
  obj = XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
  if (obj) {
    rc = ListChildren(interp, obj, NULL, 0);
  } else {
    rc = TCL_OK;
  }
  DSTRING_FREE(dsPtr);
  return rc;
}

static int XOTclClassInfoSubclassMethod(Tcl_Interp *interp, XOTclClass * class, int withClosure,
                             char *patternString, XOTclObject *patternObj) {
  int rc;
  if (withClosure) {
    XOTclClasses *saved = class->order, *subclasses;
    class->order = NULL;
    subclasses = ComputeOrder(class, class->order, Sub);
    class->order = saved;
    if (subclasses) subclasses=subclasses->nextPtr;
    rc = AppendMatchingElementsFromClasses(interp, subclasses, patternString, patternObj);
    XOTclClassListFree(subclasses);
  } else {
    rc = AppendMatchingElementsFromClasses(interp, class->sub, patternString, patternObj);
  }

  if (patternObj) {
    Tcl_SetObjResult(interp, rc ? patternObj->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
  }

  return TCL_OK;
}

static int XOTclClassInfoSuperclassMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, Tcl_Obj *pattern) {
  return ListSuperclasses(interp, class, pattern, withClosure);
}

/***************************
 * End Class Info methods
 ***************************/

/*
 * New Tcl Commands
 */
static int
XOTcl_NSCopyCmds(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Command cmd;
  Tcl_Obj *newFullCmdName, *oldFullCmdName;
  char *newName, *oldName, *name;
  Tcl_Namespace *nsPtr, *newNsPtr;
  Tcl_HashTable *cmdTable;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  XOTclObject *obj;
  XOTclClass *cl;

  if (objc != 3)
    return XOTclObjErrArgCnt(interp, objv[0], NULL, "<fromNs> <toNs>");

  nsPtr = ObjFindNamespace(interp, objv[1]);
  if (!nsPtr)
    return TCL_OK;

  name = ObjStr(objv[1]);
  /* check, if we work on an object or class namespace */
  if (isClassName(name)) {
    cl  = XOTclpGetClass(interp, NSCutXOTclClasses(name));
    obj = (XOTclObject *)cl;
  } else {
    cl  = NULL;
    obj = XOTclpGetObject(interp, name);
  }

  if (obj == NULL) {
    return XOTclVarErrMsg(interp, "CopyCmds argument 1 (",ObjStr(objv[1]),") is not an object",
                          NULL);
  }
  /*  obj = XOTclpGetObject(interp, ObjStr(objv[1]));*/

  newNsPtr = ObjFindNamespace(interp, objv[2]);
  if (!newNsPtr)
    return XOTclVarErrMsg(interp, "CopyCmds: Destination namespace ",
                          ObjStr(objv[2]), " does not exist", (char *) NULL);
  /*
   * copy all procs & commands in the ns
   */
  cmdTable = Tcl_Namespace_cmdTable(nsPtr);
  hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch);
  while (hPtr) {
    /*fprintf(stderr,"copy cmdTable = %p, first=%p\n", cmdTable, hPtr);*/
    name = Tcl_GetHashKey(cmdTable, hPtr);

    /*
     * construct full cmd names
     */
    newFullCmdName = Tcl_NewStringObj(newNsPtr->fullName,-1);
    oldFullCmdName = Tcl_NewStringObj(nsPtr->fullName,-1);

    INCR_REF_COUNT(newFullCmdName); INCR_REF_COUNT(oldFullCmdName);
    Tcl_AppendStringsToObj(newFullCmdName, "::", name, (char *) NULL);
    Tcl_AppendStringsToObj(oldFullCmdName, "::", name, (char *) NULL);
    newName = ObjStr(newFullCmdName);
    oldName = ObjStr(oldFullCmdName);

    /*fprintf(stderr,"try to copy command from '%s' to '%s'\n", oldName, newName);*/
    /*
     * Make sure that the destination command does not already exist.
     * Otherwise: do not copy
     */
    cmd = Tcl_FindCommand(interp, newName, 0, 0);
    if (cmd) {
      /*fprintf(stderr, "%s already exists\n", newName);*/
      if (!XOTclpGetObject(interp, newName)) {
        /* command or instproc will be deleted & then copied */
        Tcl_DeleteCommandFromToken(interp, cmd);
      } else {
        /* don't overwrite objects -> will be recreated */
        hPtr = Tcl_NextHashEntry(&hSrch);
        DECR_REF_COUNT(newFullCmdName);
        DECR_REF_COUNT(oldFullCmdName);
        continue;
      }
    }

    /*
     * Find the existing command. An error is returned if simpleName can't
     * be found
     */
    cmd = Tcl_FindCommand(interp, oldName, 0, 0);
    if (cmd == NULL) {
      Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), "can't copy ", " \"",
                             oldName, "\": command doesn't exist",
                             (char *) NULL);
      DECR_REF_COUNT(newFullCmdName);
      DECR_REF_COUNT(oldFullCmdName);
      return TCL_ERROR;
    }
    /*
     * Do not copy Objects or Classes
     */
    if (!XOTclpGetObject(interp, oldName)) {
      if (TclIsProc((Command*)cmd)) {
        Proc *procPtr = (Proc*) Tcl_Command_objClientData(cmd);
        Tcl_Obj *arglistObj = NULL;
        CompiledLocal *localPtr;
        XOTclNonposArgs *nonposArgs = NULL;

        /*
         * Build a list containing the arguments of the proc
         */

        nonposArgs = getNonposArgs(cmd);
        if (nonposArgs) {
          arglistObj = NonposArgsFormat(interp, nonposArgs);
          INCR_REF_COUNT(arglistObj);
          AppendOrdinaryArgsFromNonposArgs(interp, nonposArgs, 0, arglistObj);
        }

        if (!arglistObj) {
          arglistObj = Tcl_NewListObj(0, NULL);
          INCR_REF_COUNT(arglistObj);

          for (localPtr = procPtr->firstLocalPtr;  localPtr;
               localPtr = localPtr->nextPtr) {
	
            if (TclIsCompiledLocalArgument(localPtr)) {
              Tcl_Obj *defVal, *defStringObj = Tcl_NewStringObj(localPtr->name, -1);
              INCR_REF_COUNT(defStringObj);

              /* check for default values */
              if ((GetProcDefault(interp, procPtr, localPtr->name, &defVal) == TCL_OK) && defVal) {
                Tcl_AppendStringsToObj(defStringObj, " ", ObjStr(defVal),
                                       (char *) NULL);
              }
              Tcl_ListObjAppendElement(interp, arglistObj, defStringObj);
              DECR_REF_COUNT(defStringObj);
            }
          }
        }
	
        if (Tcl_Command_objProc(cmd) == RUNTIME_STATE(interp)->objInterpProc) {
          Tcl_DString ds, *dsPtr = &ds;

          if (cl) {
            /* we have a class */
            XOTclProcAssertion *procs;

            if (cl) {
              procs = cl->opt ?
                AssertionFindProcs(cl->opt->assertions, name) : 0;
            } else {
              DECR_REF_COUNT(newFullCmdName);
              DECR_REF_COUNT(oldFullCmdName);
              DECR_REF_COUNT(arglistObj);
              return XOTclVarErrMsg(interp, "No class for inst - assertions", (char *) NULL);
            }

            /* XOTcl InstProc */
            DSTRING_INIT(dsPtr);
            Tcl_DStringAppendElement(dsPtr, NSCutXOTclClasses(newNsPtr->fullName));
            Tcl_DStringAppendElement(dsPtr, "instproc");
            Tcl_DStringAppendElement(dsPtr, name);
            Tcl_DStringAppendElement(dsPtr, ObjStr(arglistObj));
            Tcl_DStringAppendElement(dsPtr, StripBodyPrefix(ObjStr(procPtr->bodyPtr)));
            if (procs) {
              XOTclRequireClassOpt(cl);
              AssertionAppendPrePost(interp, dsPtr, procs);
            }
            Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr), 0);
            DSTRING_FREE(dsPtr);
          } else {
            XOTclObject *obj = XOTclpGetObject(interp, nsPtr->fullName);
            XOTclProcAssertion *procs;
            if (obj) {
              procs = obj->opt ?
                AssertionFindProcs(obj->opt->assertions, name) : 0;
            } else {
              DECR_REF_COUNT(newFullCmdName);
              DECR_REF_COUNT(oldFullCmdName);
              DECR_REF_COUNT(arglistObj);
              return XOTclVarErrMsg(interp, "No object for assertions", (char *) NULL);
            }

            /* XOTcl Proc */
            DSTRING_INIT(dsPtr);
            Tcl_DStringAppendElement(dsPtr, newNsPtr->fullName);
            Tcl_DStringAppendElement(dsPtr, "proc");
            Tcl_DStringAppendElement(dsPtr, name);
            Tcl_DStringAppendElement(dsPtr, ObjStr(arglistObj));
            Tcl_DStringAppendElement(dsPtr, StripBodyPrefix(ObjStr(procPtr->bodyPtr)));
            if (procs) {
              XOTclRequireObjectOpt(obj);
              AssertionAppendPrePost(interp, dsPtr, procs);
            }
            /*fprintf(stderr, "new proc = '%s'\n",Tcl_DStringValue(dsPtr));*/
            Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr), 0);
            DSTRING_FREE(dsPtr);
          }
          DECR_REF_COUNT(arglistObj);
        } else {
          /* Tcl Proc */
          Tcl_VarEval(interp, "proc ", newName, " {", ObjStr(arglistObj),"} {\n",
                      ObjStr(procPtr->bodyPtr), "}", (char *) NULL);
        }
      } else {
        /*
         * Otherwise copy command
         */
        Tcl_ObjCmdProc *objProc = Tcl_Command_objProc(cmd);
        Tcl_CmdDeleteProc *deleteProc = Tcl_Command_deleteProc(cmd);
        ClientData clientData;
        if (objProc) {
          clientData = Tcl_Command_objClientData(cmd);
          if (clientData == NULL || clientData == (ClientData)XOTCL_CMD_NONLEAF_METHOD) {
            /* if client data not null, we would have to copy
               the client data; we don't know its size...., so rely
               on introspection for copying */
            Tcl_CreateObjCommand(interp, newName, objProc,
                                 Tcl_Command_objClientData(cmd), deleteProc);
          }
        } else {
          clientData = Tcl_Command_clientData(cmd);
          if (clientData == NULL || clientData == (ClientData)XOTCL_CMD_NONLEAF_METHOD) {
            Tcl_CreateCommand(interp, newName, Tcl_Command_proc(cmd),
                              Tcl_Command_clientData(cmd), deleteProc);
          }
        }
      }
    }
    hPtr = Tcl_NextHashEntry(&hSrch);
    DECR_REF_COUNT(newFullCmdName); DECR_REF_COUNT(oldFullCmdName);
  }
  return TCL_OK;
}

static int
XOTcl_NSCopyVars(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Namespace *nsPtr, *newNsPtr;
  Var *varPtr = NULL;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  TclVarHashTable *varTable;
  int rc = TCL_OK;
  XOTclObject *obj, *destObj;
  char *destFullName;
  Tcl_Obj *destFullNameObj;
  TclCallFrame frame, *framePtr = &frame;
  Tcl_Obj *varNameObj = NULL;

  if (objc != 3)
    return XOTclObjErrArgCnt(interp, objv[0], NULL, "<fromNs> <toNs>");

  nsPtr = ObjFindNamespace(interp, objv[1]);
  /*fprintf(stderr,"copyvars from %s to %s, ns=%p\n", ObjStr(objv[1]), ObjStr(objv[2]), ns);*/

  if (nsPtr) {
    newNsPtr = ObjFindNamespace(interp, objv[2]);
    if (!newNsPtr)
      return XOTclVarErrMsg(interp, "CopyVars: Destination namespace ",
                            ObjStr(objv[2]), " does not exist", (char *) NULL);

    obj = XOTclpGetObject(interp, ObjStr(objv[1]));
    destFullName = newNsPtr->fullName;
    destFullNameObj = Tcl_NewStringObj(destFullName, -1);
    INCR_REF_COUNT(destFullNameObj);
    varTable = Tcl_Namespace_varTable(nsPtr);
    Tcl_PushCallFrame(interp,(Tcl_CallFrame *)framePtr, newNsPtr, 0);
  } else {
    XOTclObject *newObj;
    if (GetObjectFromObj(interp, objv[1], &obj) != TCL_OK) {
      return XOTclVarErrMsg(interp, "CopyVars: Origin object/namespace ",
                            ObjStr(objv[1]), " does not exist", (char *) NULL);
    }
    if (GetObjectFromObj(interp, objv[2], &newObj) != TCL_OK) {
      return XOTclVarErrMsg(interp, "CopyVars: Destination object/namespace ",
                            ObjStr(objv[2]), " does not exist", (char *) NULL);
    }
    varTable = obj->varTable;
    destFullNameObj = newObj->cmdName;
    destFullName = ObjStr(destFullNameObj);
  }

  destObj = XOTclpGetObject(interp, destFullName);

  /* copy all vars in the ns */
  hPtr = varTable ? Tcl_FirstHashEntry(VarHashTable(varTable), &hSrch) : NULL;
  while (hPtr) {

    getVarAndNameFromHash(hPtr, &varPtr, &varNameObj);
    INCR_REF_COUNT(varNameObj);

    if (!TclIsVarUndefined(varPtr) && !TclIsVarLink(varPtr)) {
      if (TclIsVarScalar(varPtr)) {
        /* it may seem odd that we do not copy obj vars with the
         * same SetVar2 as normal vars, but we want to dispatch it in order to
         * be able to intercept the copying */

        if (obj) {
          /* fprintf(stderr, "copy in obj %s var %s val '%s'\n",objectName(obj),ObjStr(varNameObj),
	     ObjStr(valueOfVar(Tcl_Obj, varPtr, objPtr)));*/

          /* can't rely on "set", if there are multiple object systems */
          setInstVar(interp, destObj, varNameObj, valueOfVar(Tcl_Obj, varPtr, objPtr));
        } else {
          Tcl_ObjSetVar2(interp, varNameObj, NULL,
                         valueOfVar(Tcl_Obj, varPtr, objPtr),
                         TCL_NAMESPACE_ONLY);
        }
      } else {
        if (TclIsVarArray(varPtr)) {
          /* HERE!! PRE85 Why not [array get/set] based? Let the core iterate*/
          TclVarHashTable *aTable = valueOfVar(TclVarHashTable, varPtr, tablePtr);
          Tcl_HashSearch ahSrch;
          Tcl_HashEntry *ahPtr = aTable ? Tcl_FirstHashEntry(VarHashTable(aTable), &ahSrch) :0;
          for (; ahPtr; ahPtr = Tcl_NextHashEntry(&ahSrch)) {
            Tcl_Obj *eltNameObj;
            Var *eltVar;

            getVarAndNameFromHash(ahPtr, &eltVar, &eltNameObj);
            INCR_REF_COUNT(eltNameObj);

            if (TclIsVarScalar(eltVar)) {
              if (obj) {
                XOTcl_ObjSetVar2((XOTcl_Object*)destObj, interp, varNameObj, eltNameObj,
				 valueOfVar(Tcl_Obj, eltVar, objPtr), 0);
              } else {
                Tcl_ObjSetVar2(interp, varNameObj, eltNameObj,
                               valueOfVar(Tcl_Obj, eltVar, objPtr),
                               TCL_NAMESPACE_ONLY);
              }
            }
            DECR_REF_COUNT(eltNameObj);
          }
        }
      }
    }
    DECR_REF_COUNT(varNameObj);
    hPtr = Tcl_NextHashEntry(&hSrch);
  }
  if (nsPtr) {
    DECR_REF_COUNT(destFullNameObj);
    Tcl_PopCallFrame(interp);
  }
  return rc;
}


#if defined(PRE85)
int
XOTclInitProcNSCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *) Tcl_Interp_varFramePtr(interp);

  /*RUNTIME_STATE(interp)->varFramePtr = varFramePtr;*/

  if (RUNTIME_STATE(interp)->cs.top->currentFramePtr == NULL) {
    RUNTIME_STATE(interp)->cs.top->currentFramePtr = varFramePtr;
  } /* else {

       fprintf(stderr,"not overwriting currentFramePtr in %p from %p to %p\n",
       RUNTIME_STATE(interp)->cs.top,
       RUNTIME_STATE(interp)->cs.top->currentFramePtr, varFramePtr);
       }     */

#if !defined(NAMESPACEINSTPROCS)
  if (varFramePtr) {
    varFramePtr->nsPtr = GetCallerVarFrame(interp, varFramePtr);
  }
#endif
  return TCL_OK;
}
#endif

#if 0
/*
 * Interpretation of Non-Positional Args
 */
int
isNonposArg(Tcl_Interp *interp, char * argStr,
            int nonposArgsDefc, Tcl_Obj **nonposArgsDefv,
            Tcl_Obj **var,  char **type) {
  int i, npac;
  Tcl_Obj **npav;
  char *varName;

  if (argStr[0] == '-') {
    for (i=0; i < nonposArgsDefc; i++) {
      if (Tcl_ListObjGetElements(interp, nonposArgsDefv[i],
                                 &npac, &npav) == TCL_OK && npac > 0) {
        varName = argStr+1;
        if (!strcmp(varName, ObjStr(npav[0]))) {
          *var = npav[0];
          *type = ObjStr(npav[1]);
          return 1;
        }
      }
    }
  }
  return 0;
}
#endif

#if defined(CANONICAL_ARGS)
int
canonicalNonpositionalArgs(parseContext *pcPtr, Tcl_Interp *interp, XOTclNonposArgs *nonposArgs,
                           char *methodName, int objc,  Tcl_Obj *CONST objv[]) {
  argDefinition CONST *aPtr;
  int i, rc;
  
  rc = parseObjv(interp, objc, objv, objv[0], nonposArgs->ifd, nonposArgs->ifdSize, pcPtr);
  if (rc != TCL_OK) {
    return rc;
  }

  for (aPtr = nonposArgs->ifd, i=0; aPtr->name; aPtr++, i++) {
    char *argName = aPtr->name;
    if (*argName == '-') argName++;
    /*fprintf(stderr, "canonicalNonpositionalArgs got for arg %s (%d) => %p %p, default %s\n",
            aPtr->name, aPtr->flags & XOTCL_ARG_REQUIRED,
            pcPtr->clientData[i], pcPtr->objv[i],
            aPtr->defaultValue ? ObjStr(aPtr->defaultValue) : "NONE");*/

    if (pcPtr->objv[i]) {
      /* got a value, already checked by objv parser */
      /*fprintf(stderr, "setting passed value for %s to '%s'\n",argName,ObjStr(pcPtr->objv[i]));*/
      if (aPtr->converter == convertToSwitch) {
        int bool;
        Tcl_GetBooleanFromObj(interp,  aPtr->defaultValue, &bool);
	pcPtr->objv[i] = Tcl_NewBooleanObj(!bool); 
      } else if(aPtr->converter == convertToRelation) {
        int result = TCL_OK, relIdx;
        XOTclObject *self = GetSelfObj(interp);
        if(self) {
          Tcl_Obj *dummy = Tcl_NewStringObj(argName,strlen(argName));
          INCR_REF_COUNT(dummy);
          result = convertToRelationtype(interp,dummy,(ClientData)&relIdx);
          /*fprintf(stderr, "### convert to reltype of %s => %d (%d, OK=%d)\n",argName,relIdx, result, TCL_OK);*/
          DECR_REF_COUNT(dummy);
          if (result == TCL_OK) {
            result = XOTclRelationCmd(interp, self, relIdx, pcPtr->objv[i]);
            fprintf(stderr, "   relationcmd %s %d %s returned (%d)\n", objectName(self), relIdx, ObjStr(pcPtr->objv[i]), result);
            /* TODO: For the time being, we fall back to an unknown value
             * so that we do not obtain proc-local (through InitArgsAndLocals())
             * or object variables (through XOTclOConfigureMethod) from relational commands
             * ... is this a valid approach?
             */
            pcPtr->objv[i] = XOTclGlobalObjects[XOTE___UNKNOWN__];
          } else {
            return XOTclVarErrMsg(interp, "setting relation '",argName, "' on object '",
                                  objectName(self), "' failed", (char *) NULL);
          }
        } else {
          return XOTclVarErrMsg(interp, "trying to set a relation outside a self-reference", (char *) NULL);
        }
      }
    } else {
      /* no valued passed, check if default is available */
      if (aPtr->defaultValue) {
#if 0
        if (aPtr->flags & XOTCL_ARG_SUBST_DEFAULT) {
          XOTclObject *self = GetSelfObj(interp); /* todo move this out, also above */
          Tcl_Obj *newValue = aPtr->defaultValue;
          rc = SubstValue(interp, self, &newValue);
          /* TODO what to do with fails  yyy */
          fprintf(stderr, "attribute %s default %p %s => %p '%s'\n", aPtr->name, 
                  aPtr->defaultValue, ObjStr(aPtr->defaultValue),
                  newValue,ObjStr(newValue));
          pcPtr->objv[i] = newValue;
        } else {
          pcPtr->objv[i] = aPtr->defaultValue;
        }
#endif
        pcPtr->objv[i] = aPtr->defaultValue;

        /* TODO: default value is not jet checked; should be in arg parsing */
        /*fprintf(stderr,"==> setting default value '%s' for var '%s'\n",ObjStr(aPtr->defaultValue),argName);*/
      } else if (aPtr->flags & XOTCL_ARG_REQUIRED) {
        return XOTclVarErrMsg(interp, "method ", methodName, ": required argument '",
                              argName, "' is missing", (char *) NULL);
      } else {
	/* Use as dummy default value an arbitrary symbol, normally
           not provided returned to the Tcl level level; this value is
           unset later by unsetUnknownArgs */
	pcPtr->objv[i] = XOTclGlobalObjects[XOTE___UNKNOWN__];
      }
    }
  }

  /* Rewind to the last argument in the spec */
  aPtr--;
  i--;

  /* fprintf(stderr, "***PRE: args last objc=%d, objc=%d, elts=%d, ifdSize=%d, pc->objc=%d\n", pcPtr->lastobjc, objc, objc - pcPtr->lastobjc,nonposArgs->ifdSize,pcPtr->objc);
   */

  /* 
   * Set objc of the parse context to the size of the interface.
   * pcPtr->objc and nonposArgs->ifdSize will be equivalent in cases
   * where argument values are passed to the call in absence of var
   * args ('args'). However, there are important points of deviation
   * which need to be handled, e.g.: 
   *
   * 1) No argument values have been passed and defaults are provided
   *    and initialised by parseObjv. objc will then not reflect the
   *    required ifdSize.
   *
   * 2) Var args have been enabled (=specified) but there are either
   *    no args values provided in the call or there are more than 1
   *    var args (see below).
   */
  pcPtr->objc = nonposArgs->ifdSize;

  if (aPtr->converter == convertToNothing) {
      /* 
       * Var args ('args') are expected.
       */
      int elts = objc - pcPtr->lastobjc;
      /*
       * 1) elts = 0: 'args' is specified, but there are no var args
       * passed in the call. At this point, pcPtr->objv[i] has the
       * value XOTclGlobalObjects[XOTE___UNKNOWN__] (see
       * above). However, tclProc.c:InitArgsAndLocals initialises an
       * empty list for 'args' because pcPtr->objc does not reflect
       * the __unknown__ value. The work is so effectively
       * delegated. Note that unsetUnknownArgs is not involved, as the
       * __unknown__ value is not to make it through
       * tclProc.c:InitArgsAndLocals
       *
       */
      if (elts == 0) {
	  pcPtr->objc--;
      }

      /*
       * 2) elts = 1: 'args' is specified, and a single var arg was
       * passed. there is no need to mutate the pcPtr->objv, because
       * this has been achieved in parseObjvs (i.e., pcPtr->objv[i]
       * contains this element). We can so avoid a memcpy operation.
       */

       /* 3) elts > 1: 'args' is specified and more than a single var
       * args were passed. subsequently, pcPtr->objv is only pointing
       * to the first of the var args. First, copy the sublist of var
       * args to pcPtr->objv, second correct pcPtr->objc. The
       * corrected pcPtr->objc will ascertain that
       * tclProc.c:InitArgsAndLocals will set up a list of the
       * appropriate size and content. There is no need to deal with a
       * list representation for 'args' at this point.
       */
      if (elts > 1) {
	  memcpy(pcPtr->objv+i,objv+pcPtr->lastobjc,sizeof(Tcl_Obj *)*elts);
	  pcPtr->objc = pcPtr->objc + elts - 1;
      }
  } 

  return TCL_OK;
}

/* XOTclUnsetUnknownArgsCmd was developed and tested for Tcl 8.5 and
 * needs probably modifications for earlier versions of Tcl. However,
 * since CANONICAL_ARGS requires Tcl 8.5 this is not an issue.
 */
int
XOTclUnsetUnknownArgsCmd(ClientData clientData, Tcl_Interp *interp, int objc,
                                   Tcl_Obj *CONST objv[]) {
  CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
  Proc *proc = Tcl_CallFrame_procPtr(varFramePtr);
  int i;

  if (proc) {
    CompiledLocal *ap;
    Var *varPtr;
    for (ap = proc->firstLocalPtr, i=0; ap; ap = ap->nextPtr, i++) {
      if (!TclIsCompiledLocalArgument(ap)) continue;
      varPtr = &Tcl_CallFrame_compiledLocals(varFramePtr)[i];
      /*fprintf(stderr, "XOTclUnsetUnknownArgsCmd var '%s' i %d fi %d var %p flags %.8x obj %p unk %p\n",
              ap->name, i, ap->frameIndex, varPtr, varPtr->flags, varPtr->value.objPtr,
              XOTclGlobalObjects[XOTE___UNKNOWN__]);*/
      if (varPtr->value.objPtr != XOTclGlobalObjects[XOTE___UNKNOWN__]) continue;
      /*fprintf(stderr, "XOTclUnsetUnknownArgsCmd must unset %s\n", ap->name);*/
      Tcl_UnsetVar2(interp, ap->name, NULL, 0);
    }
  }
  
  return TCL_OK;  
}

#else

int
XOTclInterpretNonpositionalArgsCmd(ClientData clientData, Tcl_Interp *interp, int objc,
                                   Tcl_Obj *CONST objv[]) {
  XOTclCallStackContent *csc = CallStackGetFrame(interp, NULL);
  XOTclNonposArgs *nonposArgs = getNonposArgs(csc->cmdPtr);
  char *procName = (char *)Tcl_GetCommandName(interp, csc->cmdPtr);
  Tcl_Obj *proc = Tcl_NewStringObj(procName, -1);
  argDefinition CONST *aPtr;
  parseContext pc;
  int i, rc;

  /* The arguments are passed via argument vector (not the single
     argument) at least for Tcl 8.5. TODO: Tcl 8.4 support? possible
     via introspection? (this is a possible TODO for optimization)
  */
  /*if (!nonposArgs) {return TCL_OK;}*/

  INCR_REF_COUNT(proc);
  rc = parseObjv(interp, objc, objv, proc, nonposArgs->ifd, objc, &pc);
  DECR_REF_COUNT(proc);

  if (rc != TCL_OK) {
#if defined(CANONICAL_ARGS)
    parseContextRelease(pcPtr);
#endif
    return rc;
  }

  for (aPtr = nonposArgs->ifd, i=0; aPtr->name; aPtr++, i++) {
    char *argName = aPtr->name;
    if (*argName == '-') argName++;
    /*fprintf(stderr, "got for arg %s (%d) => %p %p, default %s\n",
            aPtr->name, aPtr->flags & XOTCL_ARG_REQUIRED,
            pc.clientData[i], pc.objv[i],
            aPtr->defaultValue ? ObjStr(aPtr->defaultValue) : "NONE");*/

    if (pc.objv[i]) {
      /* got a value, already checked by objv parser */
      /*fprintf(stderr, "setting passed value for %s to '%s'\n",argName,ObjStr(pc.objv[i]));*/
      if (aPtr->converter == convertToSwitch) {
        int bool;
        Tcl_GetBooleanFromObj(interp,  aPtr->defaultValue, &bool);
        /*fprintf(stderr, "setting passed value for %s to '%d'\n",argName,!pc.clientData[i]);*/
        Tcl_SetVar2Ex(interp, argName, NULL, Tcl_NewBooleanObj(!bool), 0);
      } else {
        /*fprintf(stderr, "setting passed value for %s to '%s'\n",argName,ObjStr(pc.objv[i]));*/
        Tcl_SetVar2Ex(interp, argName, NULL, pc.objv[i], 0);
      }
    } else {
      /* no valued passed, check if default is available */
      if (aPtr->defaultValue) {
        /* TODO: default value is not jet checked; should be in arg parsing */
        /*fprintf(stderr,"=== setting default value '%s' for var '%s'\n",ObjStr(aPtr->defaultValue),argName);*/
        Tcl_SetVar2Ex(interp, argName, NULL, aPtr->defaultValue, 0);
      } else if (aPtr->flags & XOTCL_ARG_REQUIRED) {
#if defined(CANONICAL_ARGS)
	parseContextRelease(pcPtr);
#endif
        return XOTclVarErrMsg(interp, "method ",procName, ": required argument '",
                              argName, "' is missing", (char *) NULL);
      }
    }
  }

  aPtr--;
  if (aPtr->converter == convertToNothing) {
    /* "args" is always defined as non-required and with convertToNothing */
    int elts = objc - pc.lastobjc;
    /*fprintf(stderr, "args last objc=%d, objc=%d, elts=%d\n", pc.lastobjc, objc, elts);*/
    Tcl_SetVar2Ex(interp, aPtr->name, NULL, Tcl_NewListObj(elts,objv+pc.lastobjc), 0);
  } else {
    Tcl_UnsetVar2(interp, "args", NULL, 0);
  }

#if defined(CANONICAL_ARGS)
  parseContextRelease(pcPtr);
#endif
  return TCL_OK;
}
#endif

/* create a slave interp that calls XOTcl Init */
static int
XOTcl_InterpObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Interp *slave;
  char *subCmd;
  ALLOC_ON_STACK(Tcl_Obj*, objc, ov);

  memcpy(ov, objv, sizeof(Tcl_Obj *)*objc);
  if (objc < 1) {
    XOTclObjErrArgCnt(interp, objv[0], NULL, "name ?args?");
    goto interp_error;
  }

  ov[0] = XOTclGlobalObjects[XOTE_INTERP];
  if (Tcl_EvalObjv(interp, objc, ov, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) != TCL_OK) {
    goto interp_error;
  }

  subCmd = ObjStr(ov[1]);
  if (isCreateString(subCmd)) {
    slave = Tcl_GetSlave(interp, ObjStr(ov[2]));
    if (!slave) {
      XOTclVarErrMsg(interp, "Creation of slave interpreter failed", (char *) NULL);
      goto interp_error;
    }
    if (Xotcl_Init(slave) == TCL_ERROR) {
      goto interp_error;
    }
#ifdef XOTCL_MEM_COUNT
    xotclMemCountInterpCounter++;
#endif
  }
  FREE_ON_STACK(ov);
  return TCL_OK;
 interp_error:
  FREE_ON_STACK(ov);
  return TCL_ERROR;
}

#if !defined(NDEBUG)
static void
checkAllInstances(Tcl_Interp *interp, XOTclClass *cl, int lvl) {
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;
  if (cl && cl->object.refCount>0) {
    /*fprintf(stderr,"checkallinstances %d cl=%p '%s'\n", lvl, cl, className(cl));*/
    for (hPtr = Tcl_FirstHashEntry(&cl->instances, &search);  hPtr;
         hPtr = Tcl_NextHashEntry(&search)) {
      XOTclObject *interpst = (XOTclObject*) Tcl_GetHashKey(&cl->instances, hPtr);
      assert(inst);
      assert(inst->refCount>0);
      assert(inst->cmdName->refCount>0);
      if (XOTclObjectIsClass(inst) && (XOTclClass*)inst != RUNTIME_STATE(interp)->theClass) {
        checkAllInstances(interp, (XOTclClass*) inst, lvl+1);
      }
    }
  }
}
#endif

#ifdef DO_FULL_CLEANUP
/* delete global variables and procs */
static void
deleteProcsAndVars(Tcl_Interp *interp) {
  Tcl_Namespace *nsPtr = Tcl_GetGlobalNamespace(interp);
  Tcl_HashTable *varTable = nsPtr ? Tcl_Namespace_varTable(ns) : NULL;
  Tcl_HashTable *cmdTable = nsPtr ? Tcl_Namespace_cmdTable(ns) : NULL;
  Tcl_HashSearch search;
  Var *varPtr;
  Tcl_Command cmd;
  register Tcl_HashEntry *entryPtr;
  char *varName;

  for (entryPtr = Tcl_FirstHashEntry(varTable, &search); entryPtr; entryPtr = Tcl_NextHashEntry(&search)) {
    Tcl_Obj *nameObj;
    getVarAndNameFromHash(entryPtr, &varPtr, &nameObj);
    if (!TclIsVarUndefined(varPtr) || TclIsVarNamespaceVar(varPtr)) {
      /* fprintf(stderr, "unsetting var %s\n", ObjStr(nameObj));*/
      Tcl_UnsetVar2(interp, ObjStr(nameObj), (char *)NULL, TCL_GLOBAL_ONLY);
    }
  }
  for (entryPtr = Tcl_FirstHashEntry(cmdTable, &search); entryPtr; entryPtr = Tcl_NextHashEntry(&search)) {
    cmd = (Tcl_Command)Tcl_GetHashValue(entryPtr);

    if (Tcl_Command_objProc(cmd) == RUNTIME_STATE(interp)->objInterpProc) {
      char *key = Tcl_GetHashKey(cmdTable, entryPtr);

      /*fprintf(stderr, "cmdname = %s cmd %p proc %p objProc %p %d\n",
        key, cmd, Tcl_Command_proc(cmd), Tcl_Command_objProc(cmd),
        Tcl_Command_proc(cmd)==RUNTIME_STATE(interp)->objInterpProc);*/
	
      Tcl_DeleteCommandFromToken(interp, cmd);
    }
  }
}
#endif


#ifdef DO_CLEANUP
static int
ClassHasSubclasses(XOTclClass *cl) {
  return (cl->sub != NULL);
}

static int
ClassHasInstances(XOTclClass *cl) {
  Tcl_HashSearch hSrch;
  return (Tcl_FirstHashEntry(&cl->instances, &hSrch) != NULL);
}

static int
ObjectHasChildren(Tcl_Interp *interp, XOTclObject *obj) {
  Tcl_Namespace *ns = obj->nsPtr;
  int result = 0;

  if (ns) {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch hSrch;
    Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(ns);
    XOTcl_FrameDecls;

    XOTcl_PushFrame(interp, obj);
    for (hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch); hPtr;
         hPtr = Tcl_NextHashEntry(&hSrch)) {
      char *key = Tcl_GetHashKey(cmdTable, hPtr);
      if (XOTclpGetObject(interp, key)) {
        /*fprintf(stderr,"child = %s\n", key);*/
        result = 1;
        break;
      }
    }
    XOTcl_PopFrame(interp, obj);
  }
  return result;
}

static void
freeAllXOTclObjectsAndClasses(Tcl_Interp *interp, Tcl_HashTable *commandTable,
                              XOTclClass *rootClass, XOTclClass *rootMetaClass) {
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch hSrch;
  XOTclObject *obj;
  XOTclClass  *cl;

  /* fprintf(stderr,"??? freeAllXOTclObjectsAndClasses in %p\n", interp); */

  /***** PHYSICAL DESTROY *****/
  RUNTIME_STATE(interp)->exitHandlerDestroyRound = XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY;
  while (1) {
    int deleted = 0;
    for (hPtr = Tcl_FirstHashEntry(commandTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      char *key = Tcl_GetHashKey(commandTable, hPtr);
      obj = XOTclpGetObject(interp, key);
      if (obj && !XOTclObjectIsClass(obj) && !ObjectHasChildren(interp, obj)) {
        /* fprintf(stderr,"  ... delete object %s %p, class=%s\n", key, obj,
           className(obj->cl));*/
        freeUnsetTraceVariable(interp, obj);
        Tcl_DeleteCommandFromToken(interp, obj->id);
        Tcl_DeleteHashEntry(hPtr);
        deleted++;
      }
    }
    /* fprintf(stderr, "deleted %d Objects\n", deleted);*/
    if (deleted > 0) {
      continue;
    }

    for (hPtr = Tcl_FirstHashEntry(commandTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      char *key = Tcl_GetHashKey(commandTable, hPtr);
      cl = XOTclpGetClass(interp, key);
      /* fprintf(stderr,"cl key = %s %p\n", key, cl); */
      if (cl
          && !ObjectHasChildren(interp, (XOTclObject*)cl)
          && !ClassHasInstances(cl)
          && !ClassHasSubclasses(cl)
          && (cl->object.flags & (XOTCL_IS_ROOT_META_CLASS|XOTCL_IS_ROOT_CLASS)) == 0
          ) {
        /* fprintf(stderr,"  ... delete class %s %p\n", key, cl); */
        freeUnsetTraceVariable(interp, &cl->object);
        Tcl_DeleteCommandFromToken(interp, cl->object.id);
        Tcl_DeleteHashEntry(hPtr);
        deleted++;
      }
    }
    /* fprintf(stderr, "deleted %d Classes\n", deleted);*/
    if (deleted == 0) {
      break;
    }
  }

#ifdef DO_FULL_CLEANUP
  deleteProcsAndVars(interp);
#endif

  RUNTIME_STATE(interp)->callDestroy = 0;
  RemoveSuper(rootMetaClass, rootClass);
  RemoveInstance((XOTclObject*)rootMetaClass, rootMetaClass);
  RemoveInstance((XOTclObject*)rootClass, rootMetaClass);

  Tcl_DeleteCommandFromToken(interp, rootClass->object.id);
  Tcl_DeleteCommandFromToken(interp, rootMetaClass->object.id);



}
#endif /* DO_CLEANUP */

static int
destroyObjectSystem(Tcl_Interp *interp, XOTclClass *rootClass, XOTclClass *rootMetaClass) {
  XOTclObject *obj;
  XOTclClass *cl;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  Tcl_HashTable objTable, *commandTable = &objTable;

  /* deleting in two rounds:
   *  (a) SOFT DESTROY: call all user-defined destroys
   *  (b) PHYSICAL DESTROY: delete the commands, user-defined
   *      destroys are not executed anymore
   *
   * this is to prevent user-defined destroys from overriding physical
   * destroy during exit handler, but still ensure that all
   * user-defined destroys are called.
   */

  Tcl_InitHashTable(commandTable, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
  getAllInstances(interp, commandTable, rootClass);

  /***** SOFT DESTROY *****/
  RUNTIME_STATE(interp)->exitHandlerDestroyRound = XOTCL_EXITHANDLER_ON_SOFT_DESTROY;

  for (hPtr = Tcl_FirstHashEntry(commandTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(commandTable, hPtr);
    obj = XOTclpGetObject(interp, key);
    /* fprintf(stderr,"key = %s %p %d\n",
       key, obj, obj && !XOTclObjectIsClass(obj)); */
    if (obj && !XOTclObjectIsClass(obj)
        && !(obj->flags & XOTCL_DESTROY_CALLED)) {
      callDestroyMethod(interp, obj, 0);
    }
  }

  for (hPtr = Tcl_FirstHashEntry(commandTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(commandTable, hPtr);
    cl = XOTclpGetClass(interp, key);
    if (cl && !(cl->object.flags & XOTCL_DESTROY_CALLED)) {
      callDestroyMethod(interp, (XOTclObject *)cl, 0);
    }
  }

#ifdef DO_CLEANUP
  freeAllXOTclObjectsAndClasses(interp, commandTable, rootClass, rootMetaClass);
#endif

  MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
  Tcl_DeleteHashTable(commandTable);

  return TCL_OK;
}

/*
 * ::xotcl::finalize command
 */
static int
XOTclFinalizeObjCmd(Tcl_Interp *interp) {
  XOTclClasses *os;
  int result;

  /* fprintf(stderr,"+++ call EXIT handler\n");  */

#if defined(PROFILE)
  XOTclProfilePrintData(interp);
#endif
  /*
   * evaluate user-defined exit handler
   */
  result = Tcl_Eval(interp, "::xotcl::__exitHandler");

  if (result != TCL_OK) {
    fprintf(stderr,"User defined exit handler contains errors!\n"
            "Error in line %d: %s\nExecution interrupted.\n",
            interp->errorLine, ObjStr(Tcl_GetObjResult(interp)));
  }

  for (os = RUNTIME_STATE(interp)->rootClasses; os; os = os->nextPtr) {
    destroyObjectSystem(interp, os->cl, (XOTclClass *)os->clientData);
  }

  XOTclClassListFree(RUNTIME_STATE(interp)->rootClasses);

#ifdef DO_CLEANUP
  XOTcl_DeleteNamespace(interp, RUNTIME_STATE(interp)->XOTclClassesNS);
  XOTcl_DeleteNamespace(interp, RUNTIME_STATE(interp)->XOTclNS);
#endif

  return TCL_OK;
}

/*
 *  Exit Handler
 */
static void
ExitHandler(ClientData clientData) {
  Tcl_Interp *interp = (Tcl_Interp *)clientData;
  int i, flags;

  /*
   * Don't use exit handler, if the interpreted is destroyed already
   * Call to exit handler comes after freeing namespaces, commands, etc.
   * e.g. TK calls Tcl_DeleteInterp directly, if Window is killed
   */

  /*
   * Ahem ...
   *
   * Since we *must* be sure that our destroy methods will run
   * we must *cheat* (I mean CHEAT) here: we flip the interp
   * flag, saying, "hey boy, you're not deleted any more".
   * After our handlers are done, we restore the old state...
   * All this is needed so we can do an eval in the interp which
   * is potentially marked for delete when we start working here.
   *
   * I know, I know, this is not really elegant. But...  I'd need a
   * standard way of invoking some code at interpreter delete time
   * but JUST BEFORE the actual deletion process starts. Sadly,
   * there is no such hook in Tcl as of Tcl8.4.*, that I know of.
   *
   * So, for the rest of procedure, assume the interp is alive !
   */
  flags = Tcl_Interp_flags(interp);
  Tcl_Interp_flags(interp) &= ~DELETED;

  CallStackPopAll(interp);

  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound == XOTCL_EXITHANDLER_OFF) {
    XOTclFinalizeObjCmd(interp);
  }

  /* must be before freeing of XOTclGlobalObjects */
  XOTclShadowTclCommands(interp, SHADOW_UNLOAD);

  /* free global objects */
  for (i = 0; i < nr_elements(XOTclGlobalStrings); i++) {
    DECR_REF_COUNT(XOTclGlobalObjects[i]);
  }
  XOTclStringIncrFree(&RUNTIME_STATE(interp)->iss);

#if defined(TCL_MEM_DEBUG)
  TclDumpMemoryInfo(stderr);
  Tcl_DumpActiveMemory("./xotclActiveMem");
  /* Tcl_GlobalEval(interp, "puts {checkmem to checkmemFile};
     checkmem checkmemFile"); */
#endif
  MEM_COUNT_DUMP();

  FREE(Tcl_Obj**, XOTclGlobalObjects);
  FREE(XOTclRuntimeState, RUNTIME_STATE(interp));

  Tcl_Interp_flags(interp) = flags;
  Tcl_Release((ClientData) interp);
}


#if defined(TCL_THREADS)
/*
 * Gets activated at thread-exit
 */
static void
XOTcl_ThreadExitProc(ClientData clientData) {
  /*fprintf(stderr,"+++ XOTcl_ThreadExitProc\n");*/

  void XOTcl_ExitProc(ClientData clientData);
  Tcl_DeleteExitHandler(XOTcl_ExitProc, clientData);
  ExitHandler(clientData);
}
#endif

/*
 * Gets activated at application-exit
 */
void
XOTcl_ExitProc(ClientData clientData) {
  /*fprintf(stderr,"+++ XOTcl_ExitProc\n");*/
#if defined(TCL_THREADS)
  Tcl_DeleteThreadExitHandler(XOTcl_ThreadExitProc, clientData);
#endif
  ExitHandler(clientData);
}


/*
 * Registers thread/appl exit handlers.
 */
static void
RegisterExitHandlers(ClientData clientData) {
  Tcl_Preserve(clientData);
#if defined(TCL_THREADS)
  Tcl_CreateThreadExitHandler(XOTcl_ThreadExitProc, clientData);
#endif
  Tcl_CreateExitHandler(XOTcl_ExitProc, clientData);
}


int
XOTclCreateObjectSystemCmd(Tcl_Interp *interp, char *Object, char *Class) {
  XOTclClass *theobj = 0;
  XOTclClass *thecls = 0;

  /* Create a basic object system with the basic root class Object and
     the basic metaclass Class, and store them in the RUNTIME STATE if
     successful */

  theobj = PrimitiveCCreate(interp, Object, 0);
  thecls = PrimitiveCCreate(interp, Class, 0);
  /* fprintf(stderr, "CreateObjectSystem created base classes \n"); */

#if defined(PROFILE)
  XOTclProfileInit(interp);
#endif

  /* test Object and Class creation */
  if (!theobj || !thecls) {
    int i;
    RUNTIME_STATE(interp)->callDestroy = 0;

    if (thecls) PrimitiveCDestroy((ClientData) thecls);
    if (theobj) PrimitiveCDestroy((ClientData) theobj);

    for (i = 0; i < nr_elements(XOTclGlobalStrings); i++) {
      DECR_REF_COUNT(XOTclGlobalObjects[i]);
    }
    FREE(Tcl_Obj **, XOTclGlobalObjects);
    FREE(XOTclRuntimeState, RUNTIME_STATE(interp));

    return XOTclErrMsg(interp, "Creation of object system failed", TCL_STATIC);
  }
  theobj->object.flags |= XOTCL_IS_ROOT_CLASS;
  thecls->object.flags |= XOTCL_IS_ROOT_META_CLASS;

  XOTclClassListAdd(&RUNTIME_STATE(interp)->rootClasses, theobj, (ClientData)thecls);

  AddInstance((XOTclObject*)theobj, thecls);
  AddInstance((XOTclObject*)thecls, thecls);
  AddSuper(thecls, theobj);

  return TCL_OK;
}

/*
 * Tcl extension initialization routine
 */

extern int
Xotcl_Init(Tcl_Interp *interp) {
  ClientData runtimeState;
  int result, i;
#ifdef XOTCL_BYTECODE
  XOTclCompEnv *interpstructions = XOTclGetCompEnv();
#endif

#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
    return TCL_ERROR;
  }
#endif

#if defined(TCL_MEM_DEBUG)
  TclDumpMemoryInfo(stderr);
#endif

  MEM_COUNT_INIT();

  /*
    fprintf(stderr, "SIZES: obj=%d, tcl_obj=%d, DString=%d, class=%d, namespace=%d, command=%d, HashTable=%d\n", sizeof(XOTclObject), sizeof(Tcl_Obj), sizeof(Tcl_DString), sizeof(XOTclClass), sizeof(Namespace), sizeof(Command), sizeof(Tcl_HashTable));
  */
#if FORWARD_COMPATIBLE
  {
    int major, minor, patchlvl, type;
    Tcl_GetVersion(&major, &minor, &patchlvl, &type);

    if ((major == 8) && (minor < 5)) {
      /*
       * loading a version of xotcl compiled for 8.4 version
       * into a 8.4 Tcl
       */
      /*
        fprintf(stderr, "loading a version of xotcl compiled for 8.4 version into a 8.4 Tcl\n");
      */
      forwardCompatibleMode = 0;
      lookupVarFromTable  = LookupVarFromTable84;
      tclVarHashCreateVar = VarHashCreateVar84;
      tclInitVarHashTable = InitVarHashTable84;
      tclCleanupVar       = TclCleanupVar84;
      varRefCountOffset   = TclOffset(Var, refCount);
      varHashTableSize    = sizeof(Tcl_HashTable);
    } else {
      /*
       * loading a version of xotcl compiled for 8.4 version
       * into a 8.5 Tcl
       */
      /*
        fprintf(stderr, "loading a version of xotcl compiled for 8.4 version into a 8.5 Tcl\n");
      */
      forwardCompatibleMode = 1;
      lookupVarFromTable  = LookupVarFromTable85;
      tclVarHashCreateVar = VarHashCreateVar85;
      tclInitVarHashTable = (Tcl_InitVarHashTableFunction*)*((&tclIntStubsPtr->reserved0)+235);
      tclCleanupVar       = (Tcl_CleanupVarFunction*)*((&tclIntStubsPtr->reserved0)+176);
      varRefCountOffset   = TclOffset(VarInHash, refCount);
      varHashTableSize    = sizeof(TclVarHashTable85);
    }

  }
#endif
  /*
   * Runtime State stored in the client data of the Interp's global
   * Namespace in order to avoid global state information
   */
  runtimeState = (ClientData) NEW(XOTclRuntimeState);
#if USE_ASSOC_DATA
  Tcl_SetAssocData(interp, "XOTclRuntimeState", NULL, runtimeState);
#else
  Tcl_Interp_globalNsPtr(interp)->clientData = runtimeState;
#endif

  memset(RUNTIME_STATE(interp), 0, sizeof(XOTclRuntimeState));

#if !defined(TCL85STACK)
  /* CallStack initialization */
  memset(RUNTIME_STATE(interp)->cs.content, 0, sizeof(XOTclCallStackContent));
  RUNTIME_STATE(interp)->cs.top = RUNTIME_STATE(interp)->cs.content;
#endif

  RUNTIME_STATE(interp)->doFilters = 1;
  RUNTIME_STATE(interp)->callDestroy = 1;
  RUNTIME_STATE(interp)->cacheInterface = 1; /* TODO xxx should not stay */

  /* create xotcl namespace */
  RUNTIME_STATE(interp)->XOTclNS =
    Tcl_CreateNamespace(interp, "::xotcl", (ClientData)NULL, (Tcl_NamespaceDeleteProc*)NULL);

  MEM_COUNT_ALLOC("TclNamespace", RUNTIME_STATE(interp)->XOTclNS);

  /*
   * init an empty, faked proc structure in the RUNTIME state
   */
  RUNTIME_STATE(interp)->fakeProc.iPtr = (Interp *)interp;
  RUNTIME_STATE(interp)->fakeProc.refCount = 1;
  RUNTIME_STATE(interp)->fakeProc.cmdPtr = NULL;
  RUNTIME_STATE(interp)->fakeProc.bodyPtr = NULL;
  RUNTIME_STATE(interp)->fakeProc.numArgs  = 0;
  RUNTIME_STATE(interp)->fakeProc.numCompiledLocals = 0;
  RUNTIME_STATE(interp)->fakeProc.firstLocalPtr = NULL;
  RUNTIME_STATE(interp)->fakeProc.lastLocalPtr = NULL;

  /* XOTclClasses in separate Namespace / Objects */
  RUNTIME_STATE(interp)->XOTclClassesNS =
    Tcl_CreateNamespace(interp, "::xotcl::classes",	(ClientData)NULL,
                        (Tcl_NamespaceDeleteProc*)NULL);
  MEM_COUNT_ALLOC("TclNamespace", RUNTIME_STATE(interp)->XOTclClassesNS);


  /* cache interpreters proc interpretation functions */
  RUNTIME_STATE(interp)->objInterpProc = TclGetObjInterpProc();
#if USE_INTERP_PROC
  RUNTIME_STATE(interp)->interpProc = TclGetInterpProc();
#endif
  RUNTIME_STATE(interp)->exitHandlerDestroyRound = XOTCL_EXITHANDLER_OFF;

  RegisterExitHandlers((ClientData)interp);
  XOTclStringIncrInit(&RUNTIME_STATE(interp)->iss);
  /* initialize global Tcl_Obj */
  XOTclGlobalObjects = NEW_ARRAY(Tcl_Obj*, nr_elements(XOTclGlobalStrings));

  for (i = 0; i < nr_elements(XOTclGlobalStrings); i++) {
    XOTclGlobalObjects[i] = Tcl_NewStringObj(XOTclGlobalStrings[i],-1);
    INCR_REF_COUNT(XOTclGlobalObjects[i]);
  }

  /* create namespaces for the different command types */
  Tcl_CreateNamespace(interp, "::xotcl::cmd", 0, (Tcl_NamespaceDeleteProc*)NULL);
  for (i=0; i < nr_elements(method_command_namespace_names); i++) {
    Tcl_CreateNamespace(interp, method_command_namespace_names[i], 0, (Tcl_NamespaceDeleteProc*)NULL);
  }

  /* create all method commands (will use the namespaces above) */
  for (i=0; i < nr_elements(method_definitions); i++) {
    Tcl_CreateObjCommand(interp, method_definitions[i].methodName, method_definitions[i].proc, 0, 0);
  }

  /*
   * overwritten tcl objs
   */
  result = XOTclShadowTclCommands(interp, SHADOW_LOAD);
  if (result != TCL_OK)
    return result;

  /*
   * new tcl cmds
   */
#ifdef XOTCL_BYTECODE
  instructions[INST_NEXT].cmdPtr = (Command *)
#endif
    Tcl_CreateObjCommand(interp, "::xotcl::next", XOTclNextObjCmd, 0, 0);
#ifdef XOTCL_BYTECODE
  instructions[INST_SELF].cmdPtr = (Command *)
#endif
    Tcl_CreateObjCommand(interp, "::xotcl::self", XOTclGetSelfObjCmd, 0, 0);
  /*Tcl_CreateObjCommand(interp, "::xotcl::K", XOTclKObjCmd, 0, 0);*/

  Tcl_CreateObjCommand(interp, "::xotcl::dispatch", XOTclDispatchCmd, 0, 0);
#if defined(PRE85)
#ifdef XOTCL_BYTECODE
  instructions[INST_INITPROC].cmdPtr = (Command *)
#endif
    Tcl_CreateObjCommand(interp, "::xotcl::initProcNS", XOTclInitProcNSCmd, 0, 0);
#endif
#if !defined(CANONICAL_ARGS)
  Tcl_CreateObjCommand(interp, "::xotcl::interpretNonpositionalArgs",
                       XOTclInterpretNonpositionalArgsCmd, 0, 0);
#else 
  Tcl_CreateObjCommand(interp, "::xotcl::unsetUnknownArgs",
                       XOTclUnsetUnknownArgsCmd, 0,0);
#endif
  Tcl_CreateObjCommand(interp, "::xotcl::interp", XOTcl_InterpObjCmd, 0, 0);
  Tcl_CreateObjCommand(interp, "::xotcl::namespace_copyvars", XOTcl_NSCopyVars, 0, 0);
  Tcl_CreateObjCommand(interp, "::xotcl::namespace_copycmds", XOTcl_NSCopyCmds, 0, 0);
  Tcl_CreateObjCommand(interp, "::xotcl::__qualify", XOTclQualifyObjCmd, 0, 0);
  Tcl_CreateObjCommand(interp, "::xotcl::trace", XOTcl_TraceObjCmd, 0, 0);
  Tcl_CreateObjCommand(interp, "::xotcl::is", XOTclIsCmd, 0, 0);

  Tcl_Export(interp, RUNTIME_STATE(interp)->XOTclNS, "self", 0);
  Tcl_Export(interp, RUNTIME_STATE(interp)->XOTclNS, "next", 0);
  Tcl_Export(interp, RUNTIME_STATE(interp)->XOTclNS, "my", 0);
  Tcl_Export(interp, RUNTIME_STATE(interp)->XOTclNS, "instvar", 0);

#ifdef XOTCL_BYTECODE
  XOTclBytecodeInit();
#endif

  Tcl_SetVar(interp, "::xotcl::version", XOTCLVERSION, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::xotcl::patchlevel", XOTCLPATCHLEVEL, TCL_GLOBAL_ONLY);

  /*
   * with some methods and library procs in tcl - they could go in a
   * xotcl.tcl file, but they're embedded here with Tcl_GlobalEval
   * to avoid the need to carry around a separate file at runtime.
   */
  {

#include "predefined.h"

    /* fprintf(stderr, "predefined=<<%s>>\n", cmd);*/
    if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
      static char cmd[] =
        "puts stderr \"Error in predefined code\n\
	 $::errorInfo\"";
      Tcl_EvalEx(interp, cmd, -1, 0);
      return TCL_ERROR;
    }
  }

#ifndef AOL_SERVER
  /* the AOL server uses a different package loading mechanism */
# ifdef COMPILE_XOTCL_STUBS
  Tcl_PkgProvideEx(interp, "XOTcl", PACKAGE_VERSION, (ClientData)&xotclStubs);
# else
  Tcl_PkgProvide(interp, "XOTcl", PACKAGE_VERSION);
# endif
#endif

#if !defined(TCL_THREADS)
  if ((Tcl_GetVar2(interp, "tcl_platform", "threaded", TCL_GLOBAL_ONLY) != NULL)) {
    /* a non threaded XOTcl version is loaded into a threaded environment */
    fprintf(stderr, "\n A non threaded XOTCL version is loaded into threaded environment\n Please reconfigure XOTcl with --enable-threads!\n\n\n");
  }
#endif

  Tcl_ResetResult(interp);
  Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);

  return TCL_OK;
}


extern int
Xotcl_SafeInit(Tcl_Interp *interp) {
  /*** dummy for now **/
  return Xotcl_Init(interp);
}

