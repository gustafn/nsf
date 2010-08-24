/* 
 *  XOTcl - Extended Object Tcl
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann (a), Uwe Zdun (a)
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

#define XOTCL_C 1
#include "xotclInt.h"
#include "xotclAccessInt.h"

#ifdef COMPILE_XOTCL_STUBS
# if defined(PRE86)
extern NxStubs nxStubs;
# else
MODULE_SCOPE const NxStubs * const nxConstStubPtr;
# endif
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

typedef enum { CALLING_LEVEL, ACTIVE_LEVEL } CallStackLevel;

typedef struct callFrameContext {
  int framesSaved;
  Tcl_CallFrame *framePtr;
  Tcl_CallFrame *varFramePtr;
} callFrameContext;

typedef struct XOTclProcContext {
  ClientData oldDeleteData;
  Tcl_CmdDeleteProc *oldDeleteProc;
  XOTclParamDefs *paramDefs;
} XOTclProcContext;

/* tclCmdClientdata is an incomplete type containing the common field(s)
   of ForwardCmdClientData, AliasCmdClientData and SetterCmdClientData
   used for filling in at runtime the actual object. */
typedef struct TclCmdClientData {
  XOTclObject *object;
} TclCmdClientData;

typedef struct SetterCmdClientData {
  XOTclObject *object;
  XOTclParam *paramsPtr;
} SetterCmdClientData;

typedef struct ForwardCmdClientData {
  XOTclObject *object;
  Tcl_Obj *cmdName;
  Tcl_ObjCmdProc *objProc;
  ClientData clientData;
  int passthrough;
  int needobjmap;
  int verbose;
  int hasNonposArgs;
  int nr_args;
  Tcl_Obj *args;
  int objscope;
  Tcl_Obj *onerror;
  Tcl_Obj *prefix;
  int nr_subcommands;
  Tcl_Obj *subcommands;
} ForwardCmdClientData;

typedef struct AliasCmdClientData {
  XOTclObject *object;
  Tcl_Obj *cmdName;
  Tcl_ObjCmdProc *objProc;
  ClientData clientData;
  XOTclClass *class;
  Tcl_Interp *interp;
  Tcl_Command aliasedCmd;
  Tcl_Command aliasCmd;
} AliasCmdClientData;

#define PARSE_CONTEXT_PREALLOC 20
typedef struct {
  ClientData *clientData;
  Tcl_Obj **objv;
  Tcl_Obj **full_objv;
  int *flags;
  ClientData clientData_static[PARSE_CONTEXT_PREALLOC];
  Tcl_Obj *objv_static[PARSE_CONTEXT_PREALLOC+1];
  int flags_static[PARSE_CONTEXT_PREALLOC+1];
  int lastobjc;
  int objc;
  int mustDecr;
  int varArgs;
  XOTclObject *object;
} parseContext;

static Tcl_ObjType CONST86 *byteCodeType = NULL, *tclCmdNameType = NULL, *listType = NULL;

int XOTclObjWrongArgs(Tcl_Interp *interp, CONST char *msg, Tcl_Obj *cmdName, Tcl_Obj *methodObj, CONST char *arglist);
static int XOTclDeprecatedCmd(Tcl_Interp *interp, CONST char *what, CONST char *oldCmd, CONST char *newCmd);

/* methods called directly when CallDirectly() returns NULL */
static int XOTclCAllocMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *nameObj);
static int XOTclCCreateMethod(Tcl_Interp *interp, XOTclClass *cl, CONST char *name, int objc, Tcl_Obj *CONST objv[]);
static int XOTclOCleanupMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclOConfigureMethod(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]);
static int XOTclODestroyMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclOResidualargsMethod(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]);
static int callDestroyMethod(Tcl_Interp *interp, XOTclObject *object, int flags);

static int XOTclNextMethod(XOTclObject *object, Tcl_Interp *interp, XOTclClass *givenCl,
                           CONST char *givenMethodName, int objc, Tcl_Obj *CONST objv[],
                           int useCSObjs, XOTclCallStackContent *cscPtr);
static int XOTclForwardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int XOTclObjscopedMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int XOTclSetterMethod(ClientData clientData, Tcl_Interp *interp, int objc,Tcl_Obj *CONST objv[]);
XOTCLINLINE static int ObjectDispatch(ClientData clientData, Tcl_Interp *interp, int objc,
                                      Tcl_Obj *CONST objv[], int flags);
static int DispatchDefaultMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

static int DoDealloc(Tcl_Interp *interp, XOTclObject *object);
static int RecreateObject(Tcl_Interp *interp, XOTclClass *cl, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]);
static void XOTclCleanupObject(XOTclObject *object);
static void finalObjectDeletion(Tcl_Interp *interp, XOTclObject *object);

static int GetObjectFromObj(Tcl_Interp *interp, register Tcl_Obj *objPtr, XOTclObject **obj);
static XOTclObject *XOTclpGetObject(Tcl_Interp *interp, CONST char *name);
static XOTclClass *XOTclpGetClass(Tcl_Interp *interp, CONST char *name);
#if !defined(NDEBUG)
static void checkAllInstances(Tcl_Interp *interp, XOTclClass *startCl, int lvl);
#endif

static int ObjectSystemsCleanup(Tcl_Interp *interp);
static void ObjectSystemsCheckSystemMethod(Tcl_Interp *interp, CONST char *methodName, XOTclObjectSystem *defOsPtr);
static XOTclObjectSystem *GetObjectSystem(XOTclObject *object);

static void getAllInstances(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclClass *startClass);
static void freeAllXOTclObjectsAndClasses(Tcl_Interp *interp, Tcl_HashTable *commandNameTable);

static Tcl_Obj *NameInNamespaceObj(Tcl_Interp *interp, CONST char *name, Tcl_Namespace *ns);
static Tcl_Namespace *callingNameSpace(Tcl_Interp *interp);
XOTCLINLINE static Tcl_Command NSFindCommand(Tcl_Interp *interp, CONST char *name, Tcl_Namespace *ns);
static int setInstVar(Tcl_Interp *interp, XOTclObject *object, Tcl_Obj *nameObj, Tcl_Obj *valueObj);

static void FilterComputeDefined(Tcl_Interp *interp, XOTclObject *object);
static void MixinComputeDefined(Tcl_Interp *interp, XOTclObject *object);
XOTCLINLINE static void GuardAdd(Tcl_Interp *interp, XOTclCmdList *filterCL, Tcl_Obj *guardObj);
static int GuardCheck(Tcl_Interp *interp, Tcl_Obj *guardObjs);
static int GuardCall(XOTclObject *object, XOTclClass *cl, Tcl_Command cmd, Tcl_Interp *interp,
                     Tcl_Obj *guardObj, XOTclCallStackContent *cscPtr);
static void GuardDel(XOTclCmdList *filterCL);

static int IsMetaClass(Tcl_Interp *interp, XOTclClass *cl, int withMixins);
static int hasMixin(Tcl_Interp *interp, XOTclObject *object, XOTclClass *cl);
static int isSubType(XOTclClass *subcl, XOTclClass *cl);
static XOTclClass *DefaultSuperClass(Tcl_Interp *interp, XOTclClass *cl, XOTclClass *mcl, int isMeta);

XOTCLINLINE static void CscInit(XOTclCallStackContent *cscPtr, XOTclObject *object, XOTclClass *cl, 
                                Tcl_Command cmd, int frameType);
XOTCLINLINE static void CscFinish(Tcl_Interp *interp, XOTclCallStackContent *cscPtr);
static XOTclCallStackContent *CallStackGetFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr);
XOTCLINLINE static void CallStackDoDestroy(Tcl_Interp *interp, XOTclObject *object);

static int XOTclInvalidateObjectParameterCmd(Tcl_Interp *interp, XOTclClass *cl);
static int ProcessMethodArguments(parseContext *pcPtr, Tcl_Interp *interp,
                                  XOTclObject *object, int pushFrame, XOTclParamDefs *paramDefs,
                                  CONST char *methodName, int objc, Tcl_Obj *CONST objv[]);
static int ArgumentCheck(Tcl_Interp *interp, Tcl_Obj *objPtr, struct XOTclParam CONST *pPtr, int *flags,
                         ClientData *clientData, Tcl_Obj **outObjPtr);
static int Parametercheck(Tcl_Interp *interp, Tcl_Obj *objPtr, Tcl_Obj *valueObj, 
			  const char *varNamePrefix, XOTclParam **paramPtrPtr);

static CONST char* AliasIndex(Tcl_DString *dsPtr, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object);
static int AliasAdd(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object, CONST char *cmd);
static int AliasDelete(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object);
static Tcl_Obj *AliasGet(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object);
static int ListMethodHandle(Tcl_Interp *interp, XOTclObject *object, int withPer_object, 
                          CONST char *methodName);

static void
parseContextInit(parseContext *pcPtr, int objc, XOTclObject *object, Tcl_Obj *procName) {
  if (objc < PARSE_CONTEXT_PREALLOC) {
    /* the single larger memset below .... */
    memset(pcPtr, 0, sizeof(parseContext));
    /* ... is faster than the two smaller memsets below */
    /* memset(pcPtr->clientData_static, 0, sizeof(ClientData)*(objc));
       memset(pcPtr->objv_static, 0, sizeof(Tcl_Obj*)*(objc+1));*/
    pcPtr->full_objv  = &pcPtr->objv_static[0];
    pcPtr->clientData = &pcPtr->clientData_static[0];
    pcPtr->flags      = &pcPtr->flags_static[0];
  } else {
    pcPtr->full_objv  = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*)*(objc+1));
    pcPtr->flags      = (int*)ckalloc(sizeof(int)*(objc+1));
    pcPtr->clientData = (ClientData*)ckalloc(sizeof(ClientData)*objc);
    /*fprintf(stderr, "ParseContextMalloc %d objc, %p %p\n", objc, pcPtr->full_objv, pcPtr->clientData);*/
    memset(pcPtr->full_objv, 0, sizeof(Tcl_Obj*)*(objc+1));
    memset(pcPtr->flags, 0, sizeof(int)*(objc+1));
    memset(pcPtr->clientData, 0, sizeof(ClientData)*(objc));
  }
  pcPtr->objv = &pcPtr->full_objv[1];
  pcPtr->full_objv[0] = procName;
  pcPtr->object = object;
  pcPtr->varArgs = 0;
  pcPtr->mustDecr = 0;
}

static void parseContextExtendObjv(parseContext *pcPtr, int from, int elts, Tcl_Obj *CONST source[]) {
  int requiredSize = from + elts + 1;

  /*XOTclPrintObjv("BEFORE: ", pcPtr->objc, pcPtr->full_objv);*/

  if (requiredSize >= PARSE_CONTEXT_PREALLOC) {
    if (pcPtr->objv == &pcPtr->objv_static[1]) {
      /* realloc from preallocated memory */
      pcPtr->full_objv  = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*) * requiredSize);
      memcpy(pcPtr->full_objv, &pcPtr->objv_static[0], sizeof(Tcl_Obj*) * PARSE_CONTEXT_PREALLOC);
      /*fprintf(stderr, "alloc %d new objv=%p pcPtr %p\n", requiredSize, pcPtr->full_objv, pcPtr);*/
    } else {
      /* realloc from mallocated memory */
      pcPtr->full_objv = (Tcl_Obj **)ckrealloc((char *)pcPtr->full_objv, sizeof(Tcl_Obj*) * requiredSize);
      /*fprintf(stderr, "realloc %d  new objv=%p pcPtr %p\n", requiredSize, pcPtr->full_objv, pcPtr);*/
    }
    pcPtr->objv = &pcPtr->full_objv[1];
  }

  memcpy(pcPtr->objv + from, source, sizeof(Tcl_Obj *) * (elts));
  pcPtr->objc += elts;

  /*XOTclPrintObjv("AFTER:  ", pcPtr->objc, pcPtr->full_objv);*/
}

static void parseContextRelease(parseContext *pcPtr) {
  if (pcPtr->mustDecr) {
    int i;
    for (i = 0; i < pcPtr->lastobjc; i++) {
      if (pcPtr->flags[i] & XOTCL_PC_MUST_DECR) {
        DECR_REF_COUNT(pcPtr->objv[i]);
      }
    }
  }

  /* objv can be separately extended */
  if (pcPtr->objv != &pcPtr->objv_static[1]) {
    /*fprintf(stderr, "parseContextRelease %p free %p %p\n", pcPtr, pcPtr->full_objv, pcPtr->clientData);*/
    ckfree((char *)pcPtr->full_objv);
  }
  /* if the parameter definition was extended, both clientData and flags are extended */
  if (pcPtr->clientData != &pcPtr->clientData_static[0]) {
    /*fprintf(stderr, "free clientdata and flags\n");*/
    ckfree((char *)pcPtr->clientData);
    ckfree((char *)pcPtr->flags);
  }
}

/*
 * Var Reform Compatibility support.
 *
 *   Definitions for accessing Tcl variable structures after varreform
 *   in Tcl 8.5.
 */

#define TclIsCompiledLocalArgument(compiledLocalPtr)  ((compiledLocalPtr)->flags & VAR_ARGUMENT)
#define TclIsCompiledLocalTemporary(compiledLocalPtr) ((compiledLocalPtr)->flags & VAR_TEMPORARY)

#define VarHashGetValue(hPtr)	((Var *) ((char *)hPtr - TclOffset(VarInHash, entry)))
#define VarHashGetKey(varPtr)   (((VarInHash *)(varPtr))->entry.key.objPtr)
#define VarHashTable(varTable)  &(varTable)->table
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

static XOTCLINLINE Var *
VarHashCreateVar(TclVarHashTable *tablePtr, Tcl_Obj *key, int *newPtr) {
  Var *varPtr = NULL;
  Tcl_HashEntry *hPtr;

  hPtr = Tcl_CreateHashEntry((Tcl_HashTable *) tablePtr,
                             (char *) key, newPtr);
  if (hPtr) {
    varPtr = VarHashGetValue(hPtr);
  }
  return varPtr;
}

static TclVarHashTable *
VarHashTableCreate() {
  TclVarHashTable *varTablePtr = (TclVarHashTable *) ckalloc(sizeof(TclVarHashTable));
  TclInitVarHashTable(varTablePtr, NULL);
  return varTablePtr;
}

#if 0
static int duringBootstrap(Tcl_Interp *interp) {
  Tcl_Obj *bootstrap = Tcl_GetVar2Ex(interp, "::nsf::bootstrap", NULL, TCL_GLOBAL_ONLY);
  return (bootstrap != NULL);
}
#endif

/*
 * call an XOTcl method
 */
static int
callMethod(ClientData clientData, Tcl_Interp *interp, Tcl_Obj *methodObj,
           int objc, Tcl_Obj *CONST objv[], int flags) {
  XOTclObject *object = (XOTclObject*) clientData;
  int result;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);
  /*fprintf(stderr, "%%%% callmethod called with method %p\n", method),*/

  tov[0] = object->cmdName;
  tov[1] = methodObj;

  if (objc>2)
    memcpy(tov+2, objv, sizeof(Tcl_Obj *)*(objc-2));

  /*fprintf(stderr, "%%%% callMethod cmdname=%s, method=%s, objc=%d\n",
    ObjStr(tov[0]), ObjStr(tov[1]), objc);
    {int i; fprintf(stderr, "\t CALL: %s ", ObjStr(method));for(i=0; i<objc-2; i++) {
    fprintf(stderr, "%s ", ObjStr(objv[i]));} fprintf(stderr, "\n");}*/

  result = ObjectDispatch(clientData, interp, objc, tov, flags);

  FREE_ON_STACK(Tcl_Obj*, tov);
  return result;
}

int
XOTclCallMethodWithArgs(ClientData clientData, Tcl_Interp *interp, Tcl_Obj *methodObj, Tcl_Obj *arg,
                        int givenobjc, Tcl_Obj *CONST objv[], int flags) {
  XOTclObject *object = (XOTclObject*) clientData;
  int objc = givenobjc + 2;
  int result;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

  assert(objc>1);
  tov[0] = object->cmdName;
  tov[1] = methodObj;
  if (objc>2) {
    tov[2] = arg;
  }
  if (objc>3)
    memcpy(tov+3, objv, sizeof(Tcl_Obj *)*(objc-3));

  /*fprintf(stderr, "%%%% callMethodWithArg cmdname=%s, method=%s, objc=%d\n",
    ObjStr(tov[0]), ObjStr(tov[1]), objc);*/
  result = ObjectDispatch(clientData, interp, objc, tov, flags);

  FREE_ON_STACK(Tcl_Obj*, tov);
  return result;
}

#include "xotclStack85.c"

/* extern callable GetSelfObj */
XOTcl_Object*
XOTclGetSelfObj(Tcl_Interp *interp) {
  return (XOTcl_Object*)GetSelfObj(interp);
}

#ifdef DISPATCH_TRACE
static void printObjv(int objc, Tcl_Obj *CONST objv[]) {
  int i, j;
  fprintf(stderr, "(%d)", objc);
  if (objc <= 3) j = objc; else j = 3;
  for (i=0;i<j;i++) fprintf(stderr, " %s", ObjStr(objv[i]));
  if (objc > 3) fprintf(stderr, " ...");
  fprintf(stderr, " (objc=%d)", objc);
}

static void printCall(Tcl_Interp *interp, CONST char *string, int objc, Tcl_Obj *CONST objv[]) {
  fprintf(stderr, "     (%d) >%s: ", Tcl_Interp_numLevels(interp), string);
  printObjv(objc, objv);
  fprintf(stderr, "\n");
}
static void printExit(Tcl_Interp *interp, CONST char *string,
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
  (obj)->refCount++;                                                    \
  fprintf(stderr, "RefCountIncr %p count=%d %s\n", obj, obj->refCount, obj->cmdName?ObjStr(obj->cmdName):"no name"); \
  MEM_COUNT_ALLOC("XOTclObject RefCount", obj)
# define XOTclObjectRefCountDecr(obj)					\
  (obj)->refCount--;							\
  fprintf(stderr, "RefCountDecr %p count=%d\n", obj, obj->refCount);	\
  MEM_COUNT_FREE("XOTclObject RefCount", obj)
#else
# define XOTclObjectRefCountIncr(obj)           \
  (obj)->refCount++;                            \
  MEM_COUNT_ALLOC("XOTclObject RefCount", obj)
# define XOTclObjectRefCountDecr(obj)           \
  (obj)->refCount--;                            \
  MEM_COUNT_FREE("XOTclObject RefCount", obj)
#endif

#if defined(XOTCLOBJ_TRACE)
void objTrace(char *string, XOTclObject *object) {
  if (object)
    fprintf(stderr, "--- %s tcl %p %s (%d %p) xotcl %p (%d) %s \n", string,
            object->cmdName, object->cmdName->typePtr ? object->cmdName->typePtr->name : "NULL",
            object->cmdName->refCount, object->cmdName->internalRep.twoPtrValue.ptr1,
            object, obj->refCount, objectName(object));
  else
    fprintf(stderr, "--- No object: %s\n", string);
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
isClassName(CONST char *string) {
  return (strncmp((string), "::nsf::classes", 14) == 0);
}

/* removes preceding ::nsf::classes from a string */
XOTCLINLINE static CONST char *
NSCutXOTclClasses(CONST char *string) {
  assert(strncmp((string), "::nsf::classes", 14) == 0);
  return string+14;
}

XOTCLINLINE static XOTclObject *
GetObjectFromNsName(Tcl_Interp *interp, CONST char *string, int *fromClassNS) {
  /*
   * Get object or class from a fully qualified cmd name, such as
   * e.g. ::nsf::classes::X
   */
  if (isClassName(string)) {
    *fromClassNS = 1;
    return (XOTclObject *)XOTclpGetClass(interp, NSCutXOTclClasses(string));
  } else {
    *fromClassNS = 0;
    return XOTclpGetObject(interp, string);
  }
}

XOTCLINLINE static char *
NSCmdFullName(Tcl_Command cmd) {
  Tcl_Namespace *nsPtr = Tcl_Command_nsPtr(cmd);
  return nsPtr ? nsPtr->fullName : "";
}

static void
XOTclCleanupObject(XOTclObject *object) {
  XOTclObjectRefCountDecr(object);

  if (object->refCount <= 0) {
    assert(object->refCount == 0);
    assert(object->flags & XOTCL_DELETED);

    MEM_COUNT_FREE("XOTclObject/XOTclClass", object);
#if defined(XOTCLOBJ_TRACE)
    fprintf(stderr, "CKFREE Object %p refcount=%d\n", object, object->refCount);
#endif
#if !defined(NDEBUG)
    memset(object, 0, sizeof(XOTclObject));
#endif
    ckfree((char *) object);
  }
}


/*
 *  Tcl_Obj functions for objects
 */

static int
IsXOTclTclObj(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclObject **objectPtr) {
  Tcl_ObjType CONST86 *cmdType = objPtr->typePtr;
  if (cmdType == tclCmdNameType) {
    Tcl_Command cmd = Tcl_GetCommandFromObj(interp, objPtr);
    if (cmd) {
      XOTclObject *object = XOTclGetObjectFromCmdPtr(cmd);
      if (object) {
        *objectPtr = object;
        return 1;
      }
    }
  }
  return 0;
}

/* Lookup an XOTcl object from the given objPtr, preferably from an
 * object of type "cmdName". objPtr might be converted in this process.
 */

static int
GetObjectFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclObject **objectPtr) {
  int result;
  XOTclObject *nobject;
  CONST char *string;
  Tcl_Command cmd;

  /*fprintf(stderr, "GetObjectFromObj obj %p %s is of type %s\n", 
    objPtr, ObjStr(objPtr), objPtr->typePtr ? objPtr->typePtr->name : "(null)");*/

  /* in case, objPtr was not of type cmdName, try to convert */
  cmd = Tcl_GetCommandFromObj(interp, objPtr);
  /*fprintf(stderr, "GetObjectFromObj obj %s => cmd=%p (%d)\n", 
    ObjStr(objPtr), cmd, cmd ? Tcl_Command_refCount(cmd):-1);*/
  if (cmd) {
    XOTclObject *object = XOTclGetObjectFromCmdPtr(cmd);

    /*fprintf(stderr, "GetObjectFromObj obj %s, o is %p objProc %p XOTclObjDispatch %p\n", ObjStr(objPtr),
      object, Tcl_Command_objProc(cmd), XOTclObjDispatch);*/
    if (object) {
      if (objectPtr) *objectPtr = object;
      return TCL_OK;
    }
  }

  /*fprintf(stderr, "GetObjectFromObj convertFromAny for %s type %p %s\n", ObjStr(objPtr),
    objPtr->typePtr, objPtr->typePtr ? objPtr->typePtr->name : "(none)");*/

  /* In case, we have to revolve via the callingNameSpace (i.e. the
   * argument is not fully qualified), we retry here.
   */
  string = ObjStr(objPtr);
  if (!isAbsolutePath(string)) {
    Tcl_Obj *tmpName = NameInNamespaceObj(interp, string, callingNameSpace(interp));
    CONST char *nsString = ObjStr(tmpName);

    INCR_REF_COUNT(tmpName);
    nobject = XOTclpGetObject(interp, nsString);
    /*fprintf(stderr, " RETRY, string '%s' returned %p\n", nsString, nobj);*/
    DECR_REF_COUNT(tmpName);
  } else {
    nobject = NULL;
  }

  if (nobject) {
    if (objectPtr) *objectPtr = nobject;
    result = TCL_OK;
  } else {
    result = TCL_ERROR;
  }
  return result;
}

static int
GetClassFromObj(Tcl_Interp *interp, register Tcl_Obj *objPtr,
		     XOTclClass **cl, XOTclClass *baseClass) {
  XOTclObject *object;
  XOTclClass *cls = NULL;
  int result = TCL_OK;
  CONST char *objName = ObjStr(objPtr);
  Tcl_Command cmd;

  /*fprintf(stderr, "GetClassFromObj %s base %p\n", objName, baseClass);*/

  cmd = Tcl_GetCommandFromObj(interp, objPtr);

  if (cmd) {
    cls = XOTclGetClassFromCmdPtr(cmd);
    if (cls == NULL) {
      /* 
       * We have a cmd, but no class; namesspace-imported classes are
       * already resolved, but we have to care, if a class is
       * "imported" via "interp alias".
       */
      Tcl_Interp *alias_interp;
      const char *alias_cmd_name;
      Tcl_Obj *nameObj = objPtr;
      Tcl_Obj **alias_ov;
      int alias_oc = 0;

      if (!isAbsolutePath(objName)) {
	nameObj = NameInNamespaceObj(interp, objName, callingNameSpace(interp));
	objName = ObjStr(nameObj);
	/* adjust path for documented nx.tcl */
      }

      result = Tcl_GetAliasObj(interp, objName, 
			       &alias_interp, &alias_cmd_name, &alias_oc, &alias_ov);
      /* we only want aliases with 0 args */
      if (result == TCL_OK && alias_oc == 0) {
	cmd = NSFindCommand(interp, alias_cmd_name, NULL);
	/*fprintf(stderr, "..... alias arg 0 '%s' cmd %p\n", alias_cmd_name, cmd);*/
	if (cmd) {
	  cls = XOTclGetClassFromCmdPtr(cmd);
	}
      }
      /*fprintf(stderr, "..... final cmd %p, cls %p\n", cmd , cls);*/
      if (nameObj != objPtr) {
	DECR_REF_COUNT(nameObj);
      }
    }
    if (cls) {
      if (cl) *cl = cls;
      return TCL_OK;
    }
  }

  result = GetObjectFromObj(interp, objPtr, &object);
  if (result == TCL_OK) {
    cls = XOTclObjectToClass(object);
    if (cls) {
      if (cl) *cl = cls;
      return TCL_OK;
    } else {
      /* flag, that we could not convert so far */
      result = TCL_ERROR;
    }
  }

  /*fprintf(stderr, "try unknown for %s, result so far is %d\n", objName, result);*/
  if (baseClass) {
    Tcl_Obj *methodObj, *nameObj = isAbsolutePath(objName) ? objPtr :
      NameInNamespaceObj(interp, objName, callingNameSpace(interp));

    INCR_REF_COUNT(nameObj);

    methodObj = XOTclMethodObj(interp, &baseClass->object, XO_c_requireobject_idx);
    if (methodObj) {
      /*fprintf(stderr, "+++ calling __unknown for %s name=%s\n", 
	className(baseClass), ObjStr(nameObj));*/
      result = callMethod((ClientData) baseClass, interp, methodObj,
                          3, &nameObj, XOTCL_CM_NO_PROTECT);
      if (result == TCL_OK) {
        result = GetClassFromObj(interp, objPtr, cl, NULL);
      }
    }
    DECR_REF_COUNT(nameObj);
  }

  return result;
}

static Tcl_Obj *
NameInNamespaceObj(Tcl_Interp *interp, CONST char *name, Tcl_Namespace *nsPtr) {
  Tcl_Obj *objPtr;
  int len;
  CONST char *objString;

  /*fprintf(stderr, "NameInNamespaceObj %s (%p, %s) ", name, nsPtr, nsPtr ? nsPtr->fullName:NULL);*/
  if (!nsPtr)
    nsPtr = Tcl_GetCurrentNamespace(interp);
  /* fprintf(stderr, " (resolved %p, %s) ", nsPtr, nsPtr ? nsPtr->fullName:NULL);*/
  objPtr = Tcl_NewStringObj(nsPtr->fullName, -1);
  len = Tcl_GetCharLength(objPtr);
  objString = ObjStr(objPtr);
  if (len == 2 && objString[0] == ':' && objString[1] == ':') {
  } else {
    Tcl_AppendLimitedToObj(objPtr, "::", 2, INT_MAX, NULL);
  }
  Tcl_AppendLimitedToObj(objPtr, name, -1, INT_MAX, NULL);

  /*fprintf(stderr, "returns %s\n", ObjStr(objPtr));*/
  return objPtr;
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
XOTclObjectListAdd(XOTclObjects **cList, XOTclObject *object) {
  XOTclObjects *l = *cList, *element = NEW(XOTclObjects);
  element->obj = object;
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
TopoSort(XOTclClass *cl, XOTclClass *baseClass, XOTclClasses *(*next)(XOTclClass*)) {
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
    if (sc->color == WHITE && !TopoSort(sc, baseClass, next)) {
      cl->color = WHITE;
      if (cl == baseClass) {
        register XOTclClasses *pc;
        for (pc = cl->order; pc; pc = pc->nextPtr) { pc->cl->color = WHITE; }
      }
      return 0;
    }
  }
  cl->color = BLACK;
  pl = NEW(XOTclClasses);
  pl->cl = cl;
  pl->nextPtr = baseClass->order;
  baseClass->order = pl;
  if (cl == baseClass) {
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
AddInstance(XOTclObject *object, XOTclClass *cl) {
  object->cl = cl;
  if (cl) {
    int nw;
    (void) Tcl_CreateHashEntry(&cl->instances, (char *)object, &nw);
  }
}

static int
RemoveInstance(XOTclObject *object, XOTclClass *cl) {
  if (cl) {
    Tcl_HashEntry *hPtr = Tcl_CreateHashEntry(&cl->instances, (char *)object, NULL);
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
static int CmdIsProc(Tcl_Command cmd) {
  /* In 8.6: TclIsProc((Command*)cmd) is not equiv to the definition below */
  return (Tcl_Command_objProc(cmd) == TclObjInterpProc);
}

static Proc *GetTclProcFromCommand(Tcl_Command cmd) {
  if (cmd) {
    Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);
    if (proc == TclObjInterpProc)
      return (Proc*) Tcl_Command_objClientData(cmd);
  }
  return NULL;
}

XOTCLINLINE static Tcl_Command
FindMethod(Tcl_Namespace *nsPtr, CONST char *methodName) {
  register Tcl_HashEntry *entryPtr;
  if ((entryPtr = Tcl_CreateHashEntry(Tcl_Namespace_cmdTable(nsPtr), methodName, NULL))) {
    return (Tcl_Command) Tcl_GetHashValue(entryPtr);
  }
  /*fprintf(stderr, "find %s in %p returns %p\n", methodName, cmdTable, cmd);*/
  return NULL;
}

static Proc *
FindProcMethod(Tcl_Namespace *nsPtr, CONST char *methodName) {
  return GetTclProcFromCommand(FindMethod(nsPtr, methodName));
}

static XOTclClass*
SearchPLMethod(register XOTclClasses *pl, CONST char *methodName, Tcl_Command *cmd) {
  /* Search the precedence list (class hierarchy) */
#if 1
  for (; pl;  pl = pl->nextPtr) {
    register Tcl_HashEntry *entryPtr = Tcl_CreateHashEntry(Tcl_Namespace_cmdTable(pl->cl->nsPtr), methodName, NULL);
    if (entryPtr) {
      *cmd = (Tcl_Command) Tcl_GetHashValue(entryPtr);
      return pl->cl;
    }
  }
#else
  for (; pl;  pl = pl->nextPtr) {
    if ((*cmd = FindMethod(pl->cl->nsPtr, methodName))) {
      return pl->cl;
    }
  }
#endif
  return NULL;
}


static XOTclClass*
SearchCMethod(/*@notnull@*/ XOTclClass *cl, CONST char *nm, Tcl_Command *cmd) {
  assert(cl);
  return SearchPLMethod(ComputeOrder(cl, cl->order, Super), nm, cmd);
}

/*
 * Find a method for a given object in the precedence path
 */
static Tcl_Command
ObjectFindMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *name, XOTclClass **pcl) {
  Tcl_Command cmd = NULL;

  if (!(object->flags & XOTCL_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, object);

  if (object->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
    XOTclCmdList *mixinList;
    for (mixinList = object->mixinOrder; mixinList; mixinList = mixinList->nextPtr) {
      XOTclClass *mixin = XOTclGetClassFromCmdPtr(mixinList->cmdPtr);
      if (mixin && (*pcl = SearchCMethod(mixin, name, &cmd))) {
	if (Tcl_Command_flags(cmd) & XOTCL_CMD_CLASS_ONLY_METHOD && !XOTclObjectIsClass(object)) {
	  cmd = NULL;
	  continue;
	}
        break;
      }
    }
  }

  if (!cmd && object->nsPtr) {
    cmd = FindMethod(object->nsPtr, name);
  }

  if (!cmd && object->cl)
    *pcl = SearchCMethod(object->cl, name, &cmd);

  return cmd;
}

/*
 *----------------------------------------------------------------------
 * ObjectSystemFree --
 *
 *    Free a single object system structure including its root classes.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Free memory of structure, free the root classes.
 *
 *----------------------------------------------------------------------
 */

static void 
ObjectSystemFree(Tcl_Interp *interp, XOTclObjectSystem *osPtr) {
  int i;

  for (i=0; i<=XO_o_unknown_idx; i++) {
    Tcl_Obj *methodObj = osPtr->methods[i];
    /*fprintf(stderr, "ObjectSystemFree [%d] %p ", i, methodObj);*/
    if (methodObj) {
      /*fprintf(stderr, "%s refCount %d", ObjStr(methodObj), methodObj->refCount);*/
      DECR_REF_COUNT(methodObj);
    }
    /*fprintf(stderr, "\n");*/
  }

  if (osPtr->rootMetaClass && osPtr->rootClass) {
    RemoveSuper(osPtr->rootMetaClass, osPtr->rootClass);
    RemoveInstance((XOTclObject*)osPtr->rootMetaClass, osPtr->rootMetaClass);
    RemoveInstance((XOTclObject*)osPtr->rootClass, osPtr->rootMetaClass);

    finalObjectDeletion(interp, &osPtr->rootClass->object);
    finalObjectDeletion(interp, &osPtr->rootMetaClass->object);
  }

  FREE(XOTclObjectSystem *, osPtr);
}

/*
 *----------------------------------------------------------------------
 * ObjectSystemAdd --
 *
 *    Add and entry to the list of object systems of the interpreter.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Updating the per interp list of object systems.
 *
 *----------------------------------------------------------------------
 */
static void 
ObjectSystemAdd(Tcl_Interp *interp, XOTclObjectSystem *osPtr) {
  osPtr->nextPtr = RUNTIME_STATE(interp)->objectSystems;
  RUNTIME_STATE(interp)->objectSystems = osPtr;
}

/*
 *----------------------------------------------------------------------
 * ObjectSystemsCheckSystemMethod --
 *
 *    Mark in all object systems the specified method as 
 *    (potentially) overloaded and mark it in the specified
 *    object system as defined.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Updating the object system structure(s).
 *
 *----------------------------------------------------------------------
 */
static void
ObjectSystemsCheckSystemMethod(Tcl_Interp *interp, CONST char *methodName, XOTclObjectSystem *defOsPtr) {
  XOTclObjectSystem *osPtr;
  int i;

  for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = osPtr->nextPtr) {
    for (i=0; i<=XO_o_unknown_idx; i++) {
      Tcl_Obj *methodObj = osPtr->methods[i];
      if (methodObj && !strcmp(methodName, ObjStr(methodObj))) {
        int flag = 1<<i;
        if (osPtr->definedMethods & flag) {
          osPtr->overloadedMethods |= flag;
          /*fprintf(stderr, "+++ %s %.6x overloading %s\n", className(defOsPtr->rootClass), 
            osPtr->overloadedMethods, methodName);*/
        }
        if (osPtr == defOsPtr && ((osPtr->definedMethods & flag) == 0)) {
          osPtr->definedMethods |= flag;
          /*fprintf(stderr, "+++ %s %.6x defining %s\n", className(defOsPtr->rootClass), 
            osPtr->definedMethods, methodName);*/
        }
      }
    }
  }
}

/*
 *----------------------------------------------------------------------
 * ObjectSystemsCleanup --
 *
 *    Delete all objects from all defined object systems.  This method
 *    is to be called when an XOTcl process or thread exists.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    All commands and objects are deleted, memory is freed.
 *
 *----------------------------------------------------------------------
 */
static int
ObjectSystemsCleanup(Tcl_Interp *interp) {
  Tcl_HashTable objTable, *commandNameTable = &objTable;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  XOTclObjectSystem *osPtr, *nPtr;

  /* Deletion is performed in two rounds:
   *  (a) SOFT DESTROY: invoke all user-defined destroy methods
   *      without destroying objects
   *  (b) PHYSICAL DESTROY: delete the objects and classes, 
   *      destroy methods are not invoked anymore
   *
   * This is to prevent that the destroy order causes classes to be
   * deleted before the methods invoked by destroy are executed.  Note
   * that it is necessary to iterate over all object systems
   * simultaneous, since the might be dependencies between objects of
   * different object systems.
   */

  Tcl_InitHashTable(commandNameTable, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", commandNameTable);

  /* collect all instances from all object systems */
  for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = osPtr->nextPtr) {
    /*fprintf(stderr, "destroyObjectSystem deletes %s\n", className(osPtr->rootClass));*/
    getAllInstances(interp, commandNameTable, osPtr->rootClass);
  }

  /***** SOFT DESTROY *****/
  RUNTIME_STATE(interp)->exitHandlerDestroyRound = XOTCL_EXITHANDLER_ON_SOFT_DESTROY;
  /*fprintf(stderr, "===CALL destroy on OBJECTS\n");*/

  for (hPtr = Tcl_FirstHashEntry(commandNameTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(commandNameTable, hPtr);
    XOTclObject *object = XOTclpGetObject(interp, key);
    /* fprintf(stderr, "key = %s %p %d\n",
       key, obj, obj && !XOTclObjectIsClass(object)); */
    if (object && !XOTclObjectIsClass(object)
        && !(object->flags & XOTCL_DESTROY_CALLED)) {
      callDestroyMethod(interp, object, 0);
    }
  }

  /*fprintf(stderr, "===CALL destroy on CLASSES\n");*/

  for (hPtr = Tcl_FirstHashEntry(commandNameTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(commandNameTable, hPtr);
    XOTclClass *cl = XOTclpGetClass(interp, key);
    if (cl && !(cl->object.flags & XOTCL_DESTROY_CALLED)) {
      callDestroyMethod(interp, (XOTclObject *)cl, 0);
    }
  }

  /* now, turn of filters, all destroy callbacks are done */
  RUNTIME_STATE(interp)->doFilters = 0;

#ifdef DO_CLEANUP
  freeAllXOTclObjectsAndClasses(interp, commandNameTable);

# ifdef DO_FULL_CLEANUP
  deleteProcsAndVars(interp);
# endif
#endif

  MEM_COUNT_FREE("Tcl_InitHashTable", commandNameTable);
  Tcl_DeleteHashTable(commandNameTable);

  /* now free all objects systems with their root classes */
  for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = nPtr) {
    nPtr = osPtr->nextPtr;
    ObjectSystemFree(interp, osPtr);
  }

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * GetObjectSystem --
 *
 *    Return the object system for which the object was defined
 *
 * Results:
 *    Object system pointer
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static XOTclObjectSystem * 
GetObjectSystem(XOTclObject *object) {
  if (XOTclObjectIsClass(object)) {
    return ((XOTclClass *)object)->osPtr;
  }
  return object->cl->osPtr;
}
   
/*
 *----------------------------------------------------------------------
 * CallDirectly --
 *
 *    Determine when it is possible/necessary to call a method
 *    implementation directly or via method dispatch.
 *
 * Results:
 *    1 is returned when command should be invoked directly, 0
 *    otherwise.
 *
 * Side effects:
 *    methodObjPtr is set with the Tcl_Obj of the name of the method,
 *    if there is one defined.
 *
 *----------------------------------------------------------------------
 */
static int CallDirectly(Tcl_Interp *interp, XOTclObject *object, int methodIdx, Tcl_Obj **methodObjPtr) {
  /* 
     We can/must call a C-implemented method directly, when 
     a) the object system has no such appropriate method defined

     b) the script does not contain a method with the appropriate
        name, and

     c) filters are not active on the object
  */
  XOTclObjectSystem *osPtr = GetObjectSystem(object);
  Tcl_Obj *methodObj = osPtr->methods[methodIdx];
  int callDirectly = 1;

  if (methodObj) {

    if ((osPtr->overloadedMethods & 1<<methodIdx) != 0) {
      /* overloaded, we must dispatch */
      /*fprintf(stderr, "overloaded\n");*/
      callDirectly = 0;
    } else if ((osPtr->definedMethods & 1<<methodIdx) == 0) {
      /* not defined, we must call directly */
      fprintf(stderr, "CallDirectly object %s idx %s not defined\n", 
              objectName(object), XOTcl_SytemMethodOpts[methodIdx]+1);
    } else {
      if (!(object->flags & XOTCL_FILTER_ORDER_VALID)) {
        FilterComputeDefined(interp, object);
      }
      /*fprintf(stderr, "CallDirectly object %s idx %s obejct flags %.6x %.6x \n", 
        objectName(object), sytemMethodOpts[methodIdx]+1,
        (object->flags & XOTCL_FILTER_ORDER_DEFINED_AND_VALID),
        XOTCL_FILTER_ORDER_DEFINED_AND_VALID
        );*/
      if ((object->flags & XOTCL_FILTER_ORDER_DEFINED_AND_VALID) == XOTCL_FILTER_ORDER_DEFINED_AND_VALID) {
        /*fprintf(stderr, "CallDirectly object %s idx %s has filter \n", 
          objectName(object), sytemMethodOpts[methodIdx]+1);*/
        callDirectly = 0;
      }
    }
  }

#if 0
  fprintf(stderr, "CallDirectly object %s idx %s returns %s => %d\n", 
          objectName(object), sytemMethodOpts[methodIdx]+1, 
          methodObj ? ObjStr(methodObj) : "(null)", callDirectly);
#endif
  /* return the methodObj in every case */
  *methodObjPtr = methodObj;
  return callDirectly;
}

/*
 *----------------------------------------------------------------------
 * XOTclMethodObj --
 *
 *    Return the methodObj for a given method index.
 *
 * Results:
 *    Returns Tcl_Obj* or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
Tcl_Obj * XOTclMethodObj(Tcl_Interp *interp, XOTclObject *object, int methodIdx) {
  XOTclObjectSystem *osPtr = GetObjectSystem(object);
  /*
  fprintf(stderr, "XOTclMethodObj object %s os %p idx %d %s methodObj %p\n",
          objectName(object), osPtr, methodIdx, 
          XOTcl_SytemMethodOpts[methodIdx]+1,
          osPtr->methods[methodIdx]);
  */
  return osPtr->methods[methodIdx];
}

static int
callDestroyMethod(Tcl_Interp *interp, XOTclObject *object, int flags) {
  int result;
  Tcl_Obj *methodObj;

  /* don't call destroy after exit handler started physical
     destruction, or when it was called already before */
  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound ==
      XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY
      || (object->flags & XOTCL_DESTROY_CALLED)
      )
    return TCL_OK;

  /*fprintf(stderr, "    callDestroy obj %p flags %.6x active %d\n", object, object->flags,
    object->activationCount);*/

  PRINTOBJ("callDestroy", object);

  /* flag, that destroy was called and invoke the method */
  object->flags |= XOTCL_DESTROY_CALLED;

  if (CallDirectly(interp, object, XO_o_destroy_idx, &methodObj)) {
    result = XOTclODestroyMethod(interp, object);
  } else {
    result = callMethod(object, interp, methodObj, 2, 0, flags);
  }

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
  fprintf(stderr, "callDestroyMethod for %p exit\n", object);
#endif
  return result;
}

/*
 * conditional memory allocations of optional storage
 */

extern XOTclObjectOpt *
XOTclRequireObjectOpt(XOTclObject *object) {
  if (!object->opt) {
    object->opt = NEW(XOTclObjectOpt);
    memset(object->opt, 0, sizeof(XOTclObjectOpt));
  }
  return object->opt;
}

extern XOTclClassOpt*
XOTclRequireClassOpt(/*@notnull@*/ XOTclClass *cl) {
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
NSGetFreshNamespace(Tcl_Interp *interp, ClientData clientData, CONST char *name, int create);

static void
makeObjNamespace(Tcl_Interp *interp, XOTclObject *object) {
#ifdef NAMESPACE_TRACE
  fprintf(stderr, "+++ Make Namespace for %s\n", objectName(object));
#endif
  if (!object->nsPtr) {
    Tcl_Namespace *nsPtr;
    object->nsPtr = NSGetFreshNamespace(interp, (ClientData)object, 
                                        objectName(object), 1);
    if (!object->nsPtr)
      Tcl_Panic("makeObjNamespace: Unable to make namespace", NULL);
    nsPtr = object->nsPtr;

    /*
     * Copy all obj variables to the newly created namespace
     */
    if (object->varTable) {
      Tcl_HashSearch  search;
      Tcl_HashEntry   *hPtr;
      TclVarHashTable *varTable = Tcl_Namespace_varTable(nsPtr);
      Tcl_HashTable   *varHashTable = VarHashTable(varTable);
      Tcl_HashTable   *objHashTable = VarHashTable(object->varTable);
	
      *varHashTable = *objHashTable; /* copy the table */
	
      if (objHashTable->buckets == objHashTable->staticBuckets) {
        varHashTable->buckets = varHashTable->staticBuckets;
      }
      for (hPtr = Tcl_FirstHashEntry(varHashTable, &search);  hPtr;
           hPtr = Tcl_NextHashEntry(&search)) {
        hPtr->tablePtr = varHashTable;
      }
      CallStackReplaceVarTableReferences(interp, object->varTable, 
                                         (TclVarHashTable *)varHashTable);

      ckfree((char *) object->varTable);
      object->varTable = NULL;
    }
  }
}

static Tcl_Var
CompiledLocalsLookup(CallFrame *varFramePtr, CONST char *varName) {
  int i, localCt = varFramePtr->numCompiledLocals;
  Tcl_Obj **objPtrPtr = &varFramePtr->localCachePtr->varName0;

  /*fprintf(stderr, ".. search #local vars %d\n", localCt);*/
  for (i=0 ; i<localCt ; i++, objPtrPtr++) {
    register Tcl_Obj *objPtr = *objPtrPtr;
    if (objPtr) {
      char *localName = TclGetString(objPtr);
      if ((varName[0] == localName[0])
          && (strcmp(varName, localName) == 0)) {
        return (Tcl_Var) &varFramePtr->compiledLocals[i];
      }
    }
  }
  return NULL;
}


static int
NsColonVarResolver(Tcl_Interp *interp, CONST char *varName, Tcl_Namespace *nsPtr, int flags, Tcl_Var *varPtr) {
  Tcl_CallFrame *varFramePtr;
  TclVarHashTable *varTablePtr;
  XOTclObject *object;
  int new, frameFlags;
  char firstChar, secondChar;
  Tcl_Obj *key;
  Var *newVar;

#if defined (VAR_RESOLVER_TRACE)
  fprintf(stderr, "NsColonVarResolver '%s' flags %.6x\n", varName, flags);
#endif

  /* Case 1: The variable is to be resolved in global scope, proceed in
   * resolver chain (i.e. return TCL_CONTINUE)
   */
  if (flags & TCL_GLOBAL_ONLY) {
    /*fprintf(stderr, "global-scoped lookup for var '%s' in NS '%s'\n", varName,
      nsPtr->fullName);*/
    return TCL_CONTINUE;
  }

  /* Case 2: The lookup happens in a proc frame (lookup in compiled
   * locals and hash table vars).  We are not interested to handle
   * these cases here, so proceed in resolver chain.
   */
  varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  assert(varFramePtr);

  frameFlags = Tcl_CallFrame_isProcCallFrame(varFramePtr);
#if defined (VAR_RESOLVER_TRACE)
  fprintf(stderr, "NsColonVarResolver '%s' frame flags %.6x\n", varName, 
          Tcl_CallFrame_isProcCallFrame(varFramePtr));
#endif

  if (frameFlags & FRAME_IS_PROC) {
#if defined (VAR_RESOLVER_TRACE)
    fprintf(stderr, "...... forwarding to next resolver\n");
#endif
    /*fprintf(stderr, "proc-scoped var '%s' assumed, frame %p flags %.6x\n",
      name, varFramePtr, Tcl_CallFrame_isProcCallFrame(varFramePtr));*/
    return TCL_CONTINUE;
  }

  firstChar = *varName;
  secondChar = *(varName+1);

  if (frameFlags & (FRAME_IS_XOTCL_CMETHOD|FRAME_IS_XOTCL_OBJECT)) {
    /* 
       Case 3: we are in an XOTcl frame 
    */
    if (firstChar == ':') {
      if (secondChar != ':') {
        /*
         * Case 3a: The variable name starts with a single ":". Skip
         * the char, but stay in the resolver.
         */
        varName ++;
      } else {
        /* 
           Case 3b: Names starting  with "::" are not for us 
        */
        return TCL_CONTINUE;
      }
    } else if (NSTail(varName) != varName) {
      /* 
         Case 3c: Names containing "::" are not for us 
      */
      return TCL_CONTINUE;
    }

    object = (frameFlags & FRAME_IS_XOTCL_CMETHOD) 
      ? ((XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr))->self 
      : (XOTclObject *)Tcl_CallFrame_clientData(varFramePtr);
    
  } else {
    /*
     * Case 4: we are not in an XOTcl frame, so proceed with a
     * TCL_CONTINUE.
     */
    return TCL_CONTINUE;
  } 

  /* We have an object and create the variable if not found */
  assert(object);
  
  varTablePtr = object->nsPtr ? Tcl_Namespace_varTable(object->nsPtr) : object->varTable;
  assert(varTablePtr);

  /* 
   * Does the variable exist in the object's namespace? 
   */

  key = Tcl_NewStringObj(varName, -1);
  INCR_REF_COUNT(key);

  *varPtr = (Tcl_Var)VarHashCreateVar(varTablePtr, key, NULL);
  
#if defined (VAR_RESOLVER_TRACE)
  fprintf(stderr, "...... lookup of '%s' for object '%s' returns %p\n", 
          varName, objectName(object), *varPtr);
#endif
  if (*varPtr == NULL) {
    /* 
     * We failed to find the variable so far, therefore we create it
     * in this var table.  Note that in several cases above,
     * TCL_CONTINUE takes care for variable creation.
     */

    newVar = VarHashCreateVar(varTablePtr, key, &new);
    *varPtr = (Tcl_Var)newVar;
  }
  DECR_REF_COUNT(key);

  return *varPtr ? TCL_OK : TCL_ERROR;
}

/*********************************************************
 *
 * Begin of compiled var resolver
 *
 *********************************************************/
#define FOR_COLON_RESOLVER(ptr) (*(ptr) == ':' && *(ptr+1) != ':')

typedef struct xotclResolvedVarInfo {
  Tcl_ResolvedVarInfo vInfo;        /* This must be the first element. */
  XOTclObject *lastObject;
  Tcl_Var var;
  Tcl_Obj *nameObj;
} xotclResolvedVarInfo;

/*
 *----------------------------------------------------------------------
 * HashVarFree --
 *
 *    Free hashed variables based on refcount.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *   Changed refCount or freed variable.
 *
 *----------------------------------------------------------------------
 */
static void
HashVarFree(Tcl_Var var) {
  if (VarHashRefCount(var) < 2) {
    /*fprintf(stderr,"#### free %p\n", var);*/
    ckfree((char *) var);
  } else {
    VarHashRefCount(var)--;
  }
}

/*
 *----------------------------------------------------------------------
 * CompiledColonVarFetch --
 *
 *    Fetch value of a a compiled XOTcl instance variable at runtime.
 *
 * Results:
 *    Tcl_Var containing value or NULL.
 *
 * Side effects:
 *   Updates of Variable structure cache in necessary.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Var
CompiledColonVarFetch(Tcl_Interp *interp, Tcl_ResolvedVarInfo *vinfoPtr) {
  xotclResolvedVarInfo *resVarInfo = (xotclResolvedVarInfo *)vinfoPtr;
  XOTclCallStackContent *cscPtr = CallStackGetFrame(interp, NULL);
  XOTclObject *object = cscPtr ? cscPtr->self : NULL;
  TclVarHashTable *varTablePtr;
  Tcl_Var var = resVarInfo->var;
  int new, flags = var ? ((Var*)var)->flags : 0;

#if defined(VAR_RESOLVER_TRACE)
  fprintf(stderr,"CompiledColonVarFetch var '%s' var %p flags = %.4x dead? %.4x\n",
          ObjStr(resVarInfo->nameObj), var, flags, flags&VAR_DEAD_HASH);
#endif

  /*
   * We cache lookups based on xotcl objects; we have to care about
   * cases, where the instance variables are in some delete states.
   *
   */

  if (object == resVarInfo->lastObject && ((flags & VAR_DEAD_HASH)) == 0) {
    /*
     * The variable is valid.
     */
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, ".... cached var '%s' var %p flags = %.4x\n", 
            ObjStr(resVarInfo->nameObj), var, flags);
#endif
    return var;
  }

  if (var) {
    /*
     * The variable is not valid anymore. Clean it up.
     */
    HashVarFree(var);
  }

  varTablePtr = object->nsPtr ? Tcl_Namespace_varTable(object->nsPtr) : object->varTable;
  assert(varTablePtr);

  resVarInfo->lastObject = object;
  resVarInfo->var = var = (Tcl_Var) VarHashCreateVar(varTablePtr, resVarInfo->nameObj, &new);
  /*
   * Increment the reference counter to avoid ckfree() of the variable
   * in Tcl's FreeVarEntry(); for cleanup, we provide our own
   * HashVarFree();
   */
  VarHashRefCount(var)++;

#if defined(VAR_RESOLVER_TRACE)
  {
    Var *v = (Var*)(resVarInfo->var);
    fprintf(stderr, ".... looked up var %s var %p flags = %.6x\n", 
            ObjStr(resVarInfo->nameObj),
            v, v->flags);
  }
#endif
  return var;
}

/*
 *----------------------------------------------------------------------
 * CompiledColonVarFree --
 *
 *    DeleteProc of the compiled variable handler.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *   Free compiled variable structure and variable.
 *
 *----------------------------------------------------------------------
 */
void CompiledColonVarFree(Tcl_ResolvedVarInfo *vinfoPtr) {
  xotclResolvedVarInfo *resVarInfo = (xotclResolvedVarInfo *)vinfoPtr;
  DECR_REF_COUNT(resVarInfo->nameObj);
  if (resVarInfo->var) {HashVarFree(resVarInfo->var);}
  ckfree((char *) vinfoPtr);
}

/*
 *----------------------------------------------------------------------
 * InterpCompiledColonVarResolver --
 *
 *    Register for prefixed variables our own compiled var handler.
 *
 * Results:
 *    TCL_OK or TCL_CONTINUE (based on Tcl's var resolver protocol)
 *
 * Side effects:
 *   Registered var handler or none.
 *
 *----------------------------------------------------------------------
 */
int InterpCompiledColonVarResolver(Tcl_Interp *interp,
			CONST84 char *name, int length, Tcl_Namespace *context,
			Tcl_ResolvedVarInfo **rPtr) {
  /* 
   *  The variable handler is registered, when we have an active XOTcl
   *  object and the variable starts with the appropriate prefix. Note
   *  that getting the "self" object is a weak protection against
   *  handling of wrong vars 
   */
  XOTclObject *object = GetSelfObj(interp);

#if defined(VAR_RESOLVER_TRACE)
  fprintf(stderr, "compiled var resolver for %s, obj %p\n", name, object);
#endif

  if (object && FOR_COLON_RESOLVER(name)) {
    xotclResolvedVarInfo *vInfoPtr = (xotclResolvedVarInfo *) ckalloc(sizeof(xotclResolvedVarInfo));

    vInfoPtr->vInfo.fetchProc = CompiledColonVarFetch;
    vInfoPtr->vInfo.deleteProc = CompiledColonVarFree; /* if NULL, tcl does a ckfree on proc clean up */
    vInfoPtr->lastObject = NULL;
    vInfoPtr->var = NULL;
    vInfoPtr->nameObj = Tcl_NewStringObj(name+1, length-1);
    INCR_REF_COUNT(vInfoPtr->nameObj);
    *rPtr = (Tcl_ResolvedVarInfo *)vInfoPtr;

    return TCL_OK;
  }
  return TCL_CONTINUE;
}

/*
 *----------------------------------------------------------------------
 * InterpColonVarResolver --
 *
 *    Resolve varnames as instance variables. These might be compiled
 *    locals or variables to be created (e.g. during an eval) in the
 *    objects vartables.  If the command starts with the XOTcl
 *    specific prefix and we are on an XOTcl stack frame, treat
 *    command as instance varname.
 *
 * Results:
 *    TCL_OK or TCL_CONTINUE (based on Tcl's var resolver protocol)
 *
 * Side effects:
 *   If successful, return varPtr, pointing to instance variable.
 *
 *----------------------------------------------------------------------
 */
static int
InterpColonVarResolver(Tcl_Interp *interp, CONST char *varName, Tcl_Namespace *nsPtr, int flags, Tcl_Var *varPtr) {
  int new, frameFlags;
  CallFrame *varFramePtr;
  TclVarHashTable *varTablePtr;
  XOTclObject *object;
  Tcl_Obj *keyObj;
  Tcl_Var var;

  if (!FOR_COLON_RESOLVER(varName) || (flags & TCL_GLOBAL_ONLY)) {
    /* ordinary names and global lookups are not for us */
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, "InterpColonVarResolver '%s' flags %.6x not for us nsPtr %p\n", 
            varName, flags, nsPtr);
#endif
    return TCL_CONTINUE;
  }

  varFramePtr = Tcl_Interp_varFramePtr(interp);
  frameFlags = Tcl_CallFrame_isProcCallFrame(varFramePtr);

#if defined(VAR_RESOLVER_TRACE)
  fprintf(stderr, "InterpColonVarResolver called var '%s' flags %.4x frame flags %.6x\n", 
          varName, flags, frameFlags);
#endif
  varName ++;

  if (frameFlags & FRAME_IS_XOTCL_METHOD) {
    if ((*varPtr = CompiledLocalsLookup(varFramePtr, varName))) {
#if defined(VAR_RESOLVER_TRACE)
      fprintf(stderr, ".... found local %s\n", varName);
#endif
      return TCL_OK;
    }
    
    object = ((XOTclCallStackContent *)varFramePtr->clientData)->self;
    
  } else if (frameFlags & FRAME_IS_XOTCL_CMETHOD) {
    object = ((XOTclCallStackContent *)varFramePtr->clientData)->self;
    
  } else if (frameFlags & FRAME_IS_XOTCL_OBJECT) {
    object = (XOTclObject *)(varFramePtr->clientData);
    
  } else {
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, ".... not found %s\n", varName);
#endif
    return TCL_CONTINUE;
  }

  /* We have an object and create the variable if not found */
  assert(object);
  
  varTablePtr = object->nsPtr ? Tcl_Namespace_varTable(object->nsPtr) : object->varTable;
  assert(varTablePtr);

  /*fprintf(stderr, "Object Var Resolver, name=%s, obj %p, nsPtr %p, varTable %p\n",
    varName, object, object->nsPtr, varTablePtr);*/

  keyObj = Tcl_NewStringObj(varName, -1);
  INCR_REF_COUNT(keyObj);

  var = (Tcl_Var)VarHashCreateVar(varTablePtr, keyObj, NULL);
  if (var) {
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, ".... found in hashtable %s %p\n", varName, var);
#endif
  } else {
    /* 
       We failed to find the variable, therefore we create it new 
    */
    var = (Tcl_Var)VarHashCreateVar(varTablePtr, keyObj, &new);
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, ".... var %p %s created in hashtable %p\n", var, varName, varTablePtr);
#endif
  }
  *varPtr = var;
  DECR_REF_COUNT(keyObj);

  return TCL_OK;
}
/*********************************************************
 *
 * End of var resolvers
 *
 *********************************************************/

/*********************************************************
 *
 * Begin of cmd resolver
 *
 *********************************************************/
/*
 *----------------------------------------------------------------------
 * InterpColonCmdResolver --
 *
 *    Resolve command names. If the command starts with the XOTcl
 *    specific prefix and we are on an XOTcl stack frame, treat
 *    command as OO method.
 *
 * Results:
 *    TCL_OK or TCL_CONTINUE (based on Tcl's command resolver protocol)
 *
 * Side effects:
 *   If successful, return cmdPtr, pointing to method.
 *
 *----------------------------------------------------------------------
 */
static int
InterpColonCmdResolver(Tcl_Interp *interp, CONST char *cmdName, Tcl_Namespace *nsPtr, int flags, Tcl_Command *cmdPtr) {
  CallFrame *varFramePtr;
  int frameFlags;

  if (!FOR_COLON_RESOLVER(cmdName) || flags & TCL_GLOBAL_ONLY) {
    /* ordinary names and global lookups are not for us */
    return TCL_CONTINUE;
  }

  varFramePtr = Tcl_Interp_varFramePtr(interp);
  frameFlags = Tcl_CallFrame_isProcCallFrame(varFramePtr);

  /* skip over a nonproc frame, in case Tcl stacks it */
 if (frameFlags == 0 && Tcl_CallFrame_callerPtr(varFramePtr)) {
    varFramePtr = (CallFrame *)Tcl_CallFrame_callerPtr(varFramePtr);
    frameFlags = Tcl_CallFrame_isProcCallFrame(varFramePtr);
#if defined(CMD_RESOLVER_TRACE)
    fprintf(stderr, "InterpColonCmdResolver uses parent frame\n");
#endif
  }
#if defined(CMD_RESOLVER_TRACE)
  fprintf(stderr, "InterpColonCmdResolver cmdName %s flags %.6x, frame flags %.6x\n",cmdName,
          flags, Tcl_CallFrame_isProcCallFrame(varFramePtr));
#endif

  if (frameFlags & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_OBJECT|FRAME_IS_XOTCL_CMETHOD )) {
#if defined(CMD_RESOLVER_TRACE)
    fprintf(stderr, "    ... call colonCmd for %s\n", cmdName);
#endif
    /*
     * We have a cmd starting with ':', we are in an xotcl frame, so
     * forward to the colonCmd.
     */
    *cmdPtr = RUNTIME_STATE(interp)->colonCmd;
    return TCL_OK;
  }

#if defined(CMD_RESOLVER_TRACE)
  fprintf(stderr, "    ... not found %s\n", cmdName);
  tcl85showStack(interp);
#endif
  return TCL_CONTINUE;
}
/*********************************************************
 *
 * End of cmd resolver
 *
 *********************************************************/

static Tcl_Namespace *
requireObjNamespace(Tcl_Interp *interp, XOTclObject *object) {

  if (!object->nsPtr) {
    makeObjNamespace(interp, object);
  }
  /* This puts a per-object namespace resolver into position upon
   * acquiring the namespace. Works for object-scoped commands/procs
   * and object-only ones (set, unset, ...)
   */
  Tcl_SetNamespaceResolvers(object->nsPtr, /*(Tcl_ResolveCmdProc*)NsColonCmdResolver*/ NULL,
                            NsColonVarResolver, 
                            /*(Tcl_ResolveCompiledVarProc*)NsCompiledColonVarResolver*/NULL);
  return object->nsPtr;
}

extern void
XOTclRequireObjNamespace(Tcl_Interp *interp, XOTcl_Object *object) {
  requireObjNamespace(interp, (XOTclObject*) object);
}


/*
 * Namespace related commands
 */

static int
NSDeleteCmd(Tcl_Interp *interp, Tcl_Namespace *nsPtr, CONST char *name) {
  /* a simple deletion would delete a global command with
     the same name, if the command is not existing, so
     we use the CmdToken */
  Tcl_Command token;
  assert(nsPtr);
  if ((token = FindMethod(nsPtr, name))) {
    return Tcl_DeleteCommandFromToken(interp, token);
  }
  return -1;
}

static void CallStackDestroyObject(Tcl_Interp *interp, XOTclObject *object);
static void PrimitiveCDestroy(ClientData clientData);
static void PrimitiveODestroy(ClientData clientData);
static void PrimitiveDestroy(ClientData clientData);

static void
NSDeleteChildren(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(nsPtr);
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "NSDeleteChildren %p %s\n", nsPtr, nsPtr->fullName);
#endif

  Tcl_ForgetImport(interp, nsPtr, "*"); /* don't destroy namespace imported objects */

  for (hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);

    if (!Tcl_Command_cmdEpoch(cmd)) {
      XOTclObject *object = XOTclGetObjectFromCmdPtr(cmd);

      /*fprintf(stderr, "... check %p %s\n",  object, object ? objectName(object) : "(null)");*/

      if (object) {
          /*fprintf(stderr, " ... child %s %p -- %s\n", oname, object, object ? objectName(object):"(null)");*/
          /*fprintf(stderr, " ... obj=%s flags %.4x\n", objectName(object), object->flags);*/
	
        /* in the exit handler physical destroy --> directly call destroy */
        if (RUNTIME_STATE(interp)->exitHandlerDestroyRound
            == XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY) {
          PrimitiveDestroy((ClientData) object);
        } else {
          if (object->teardown && !(object->flags & XOTCL_DESTROY_CALLED)) {
            /*fprintf(stderr, " ... call destroy obj=%s flags %.4x\n", objectName(object), object->flags);*/

            if (callDestroyMethod(interp, object, 0) != TCL_OK) {
              /* destroy method failed, but we have to remove the command
                 anyway. */
              if (object->teardown) {
                CallStackDestroyObject(interp, object);
              }
            }
          }
        }
      }
    }
  }
}

/*
 * ensure that a variable exists on object varTable or nsPtr->varTable,
 * if necessary create it. Return Var* if successful, otherwise 0
 */
static Var *
NSRequireVariableOnObj(Tcl_Interp *interp, XOTclObject *object, CONST char *name, int flgs) {
  Tcl_CallFrame frame, *framePtr = &frame;
  Var *varPtr, *arrayPtr;

  XOTcl_PushFrameObj(interp, object, framePtr);
  varPtr = TclLookupVar(interp, name, 0, flgs, "obj vwait",
                        /*createPart1*/ 1, /*createPart2*/ 0, &arrayPtr);
  XOTcl_PopFrameObj(interp, framePtr);
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

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "NSCleanupNamespace %p\n", ns);
  fprintf(stderr, "NSCleanupNamespace %p %.6x varTable %p\n", ns, ((Namespace *)ns)->flags, varTable);
#endif
  /*
   * Delete all variables and initialize var table again
   * (DeleteVars frees the vartable)
   */
  TclDeleteVars((Interp *)interp, varTable);
  TclInitVarHashTable(varTable, (Namespace *)ns);

  /*
   * Delete all user-defined procs in the namespace
   */
  for (hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
      Tcl_Command cmd = (Tcl_Command) Tcl_GetHashValue(hPtr);
      Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);
      XOTclObject *invokeObj = proc == XOTclObjDispatch ? (XOTclObject *)Tcl_Command_objClientData(cmd) : NULL;

      /* objects should not be deleted here to preseve children deletion order */
      if (invokeObj && cmd != invokeObj->id) {
        /*
         * cmd is an aliased object, reduce the refcount
         */
        /*fprintf(stderr, "NSCleanupNamespace cleanup aliased object %p\n", invokeObj); */
        XOTclCleanupObject(invokeObj);
        XOTcl_DeleteCommandFromToken(interp, cmd);
      }
      if (invokeObj) {
        /* 
         * cmd is a child object 
         */
        continue;
      }

      /* fprintf(stderr, "NSCleanupNamespace calls DeleteCommandFromToken for %p flags %.6x invokeObj %p obj %p\n",
                cmd, ((Command *)cmd)->flags, invokeObj,object);
        fprintf(stderr, "    cmd = %s\n", Tcl_GetCommandName(interp,cmd));
        fprintf(stderr, "    nsPtr = %p\n", ((Command *)cmd)->nsPtr);
        fprintf(stderr, "    flags %.6x\n", ((Namespace *)((Command *)cmd)->nsPtr)->flags);*/
        
      XOTcl_DeleteCommandFromToken(interp, cmd);
  }
}


static void
NSNamespaceDeleteProc(ClientData clientData) {
  /* dummy for ns identification by pointer comparison */
  XOTclObject *object = (XOTclObject*) clientData;
  /*fprintf(stderr, "namespacedeleteproc obj=%p ns=%p\n", 
    clientData,object ? object->nsPtr : NULL);*/
  if (object) {
    object->nsPtr = NULL;
  }
}

void
XOTcl_DeleteNamespace(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  int activationCount = 0;
  Tcl_CallFrame *f = (Tcl_CallFrame *)Tcl_Interp_framePtr(interp);

  /*fprintf(stderr, "XOTcl_DeleteNamespace %p ", nsPtr);*/

  while (f) {
    if (f->nsPtr == nsPtr)
      activationCount++;
    f = Tcl_CallFrame_callerPtr(f);
  }
  
  /* todo remove debug line */
  if (Tcl_Namespace_activationCount(nsPtr) != activationCount) {
    fprintf(stderr, "WE HAVE TO FIX ACTIVATIONCOUNT\n");
    Tcl_Namespace_activationCount(nsPtr) = activationCount;
  }

  /*fprintf(stderr, "to %d. \n", ((Namespace *)nsPtr)->activationCount);*/

  MEM_COUNT_FREE("TclNamespace", nsPtr);
  if (Tcl_Namespace_deleteProc(nsPtr)) {
    /*fprintf(stderr, "calling deteteNamespace %s\n", nsPtr->fullName);*/
    Tcl_DeleteNamespace(nsPtr);
  }
}

static Tcl_Namespace*
NSGetFreshNamespace(Tcl_Interp *interp, ClientData clientData, CONST char *name, int create) {
  Tcl_Namespace *nsPtr = Tcl_FindNamespace(interp, name, NULL, 0);

  if (nsPtr) {
    if (nsPtr->deleteProc || nsPtr->clientData) {
      Tcl_Panic("Namespace '%s' exists already with delProc %p and clientData %p; Can only convert a plain Tcl namespace into an XOTcl namespace, my delete Proc %p",
		name, nsPtr->deleteProc, nsPtr->clientData, NSNamespaceDeleteProc);
    }
    nsPtr->clientData = clientData;
    nsPtr->deleteProc = (Tcl_NamespaceDeleteProc *)NSNamespaceDeleteProc;
  } else if (create) {
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
NSCheckColons(CONST char *name, size_t l) {
  register CONST char *n = name;
  if (*n == '\0') return 0; /* empty name */
  if (l == 0) l = strlen(name);
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
NSCheckForParent(Tcl_Interp *interp, CONST char *name, size_t l, XOTclClass *cl) {
  register CONST char *n = name+l;
  int rc = 1;

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
        XOTclClass *defaultSuperClass = DefaultSuperClass(interp, cl, cl->object.cl, 0);
        Tcl_Obj *methodObj = XOTclMethodObj(interp, &defaultSuperClass->object, XO_c_requireobject_idx);

        if (methodObj) {
          /* call requireObject and try again */
          Tcl_Obj *ov[3];
          int result;
          
          ov[0] = defaultSuperClass->object.cmdName;
          ov[1] = methodObj;
          ov[2] = Tcl_NewStringObj(parentName, -1);
          INCR_REF_COUNT(ov[2]);
          /*fprintf(stderr, "+++ parent... calling __unknown for %s\n", ObjStr(ov[2]));*/
          result = Tcl_EvalObjv(interp, 3, ov, 0);
          if (result == TCL_OK) {
            XOTclObject *parentObj = (XOTclObject*) XOTclpGetObject(interp, parentName);
            if (parentObj) {
              requireObjNamespace(interp, parentObj);
            }
            rc = (Tcl_FindNamespace(interp, parentName,
                                  (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY) != NULL);
          } else {
            rc = 0;
          }
          DECR_REF_COUNT(ov[2]);
        }
      }
    } else {
      XOTclObject *parentObj = (XOTclObject*) XOTclpGetObject(interp, parentName);
      if (parentObj) {
        requireObjNamespace(interp, parentObj);
      }
    }
    DSTRING_FREE(dsp);
  }
  return rc;
}

/*
 * Find the "real" command belonging eg. to an XOTcl class or object.
 * Do not return cmds produced by Tcl_Import, but the "real" cmd
 * to which they point.
 */
XOTCLINLINE static Tcl_Command
NSFindCommand(Tcl_Interp *interp, CONST char *name, Tcl_Namespace *ns) {
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
XOTclGetObject(Tcl_Interp *interp, CONST char *name) {
  return (XOTcl_Object*) XOTclpGetObject(interp, name);
}

/*
 * Find an object using a char *name
 */
static XOTclObject*
XOTclpGetObject(Tcl_Interp *interp, CONST char *name) {
  register Tcl_Command cmd;
  assert(name);
  /*fprintf(stderr, "XOTclpGetObject name = '%s'\n", name);*/

  cmd = NSFindCommand(interp, name, NULL);

  /*if (cmd) {
    fprintf(stderr, "+++ XOTclGetObject from %s -> objProc=%p, dispatch=%p OK %d\n",
            name, Tcl_Command_objProc(cmd), XOTclObjDispatch, Tcl_Command_objProc(cmd) == XOTclObjDispatch);
            }*/

  if (cmd && Tcl_Command_objProc(cmd) == XOTclObjDispatch) {
    /*fprintf(stderr, "XOTclpGetObject cd %p\n", Tcl_Command_objClientData(cmd));*/
    return (XOTclObject*)Tcl_Command_objClientData(cmd);
  }
  return 0;
}

/*
 * Find a class using a char *name
 */

extern XOTcl_Class*
XOTclGetClass(Tcl_Interp *interp, CONST char *name) {
  return (XOTcl_Class*)XOTclpGetClass(interp, name);
}

static XOTclClass*
XOTclpGetClass(Tcl_Interp *interp, CONST char *name) {
  XOTclObject *object = XOTclpGetObject(interp, name);
  return (object && XOTclObjectIsClass(object)) ? (XOTclClass*)object : NULL;
}

static int
CanRedefineCmd(Tcl_Interp *interp, Tcl_Namespace *nsPtr, XOTclObject *object, CONST char *methodName) {
  int result, ok;
  Tcl_Command cmd = FindMethod(nsPtr, methodName);

  ok = cmd ? (Tcl_Command_flags(cmd) & XOTCL_CMD_REDEFINE_PROTECTED_METHOD) == 0 : 1;
  if (ok) {
    result = TCL_OK;
  } else {
    result = XOTclVarErrMsg(interp, "Method '", methodName, "' of ", objectName(object),
                            " can not be overwritten. Derive e.g. a sub-class!", 
                            (char *) NULL);
  }
  ObjectSystemsCheckSystemMethod(interp, methodName, GetObjectSystem(object));

  return result;
}

int
XOTclAddObjectMethod(Tcl_Interp *interp, XOTcl_Object *object1, CONST char *methodName,
                     Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp,
                     int flags) {
  XOTclObject *object = (XOTclObject *)object1;
  Tcl_DString newCmdName, *dsPtr = &newCmdName;
  Tcl_Namespace *ns = requireObjNamespace(interp, object);
  Tcl_Command newCmd;
  int result;

  /* Check, if we are allowed to redefine the method */
  result = CanRedefineCmd(interp, object->nsPtr, object, (char*)methodName);
  if (result != TCL_OK) {
    return result;
  }

  /* delete an alias definition, if it exists */
  AliasDelete(interp, object->cmdName, methodName, 1);

  ALLOC_NAME_NS(dsPtr, ns->fullName, methodName);

  newCmd = Tcl_CreateObjCommand(interp, Tcl_DStringValue(dsPtr), proc, clientData, dp);
  if (flags) {
    ((Command *) newCmd)->flags |= flags;
  }
  DSTRING_FREE(dsPtr);
  return TCL_OK;
}

int
XOTclAddClassMethod(Tcl_Interp *interp, XOTcl_Class *class, CONST char *methodName,
                       Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp,
                       int flags) {
  XOTclClass *cl = (XOTclClass *)class;
  Tcl_DString newCmdName, *dsPtr = &newCmdName;
  Tcl_Command newCmd;
  int result;

   /* Check, if we are allowed to redefine the method */
  result = CanRedefineCmd(interp, cl->nsPtr, &cl->object, (char*)methodName);
  if (result != TCL_OK) {
    return result;
  }

 /* delete an alias definition, if it exists */
  AliasDelete(interp, class->object.cmdName, methodName, 0);

  ALLOC_NAME_NS(dsPtr, cl->nsPtr->fullName, methodName);
  newCmd = Tcl_CreateObjCommand(interp, Tcl_DStringValue(dsPtr), proc, clientData, dp);

  if (flags) {
    ((Command *) newCmd)->flags |= flags;
  }
  DSTRING_FREE(dsPtr);
  return TCL_OK;
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
AutonameIncr(Tcl_Interp *interp, Tcl_Obj *nameObj, XOTclObject *object,
             int instanceOpt, int resetOpt) {
  int valueLength, mustCopy = 1, format = 0;
  char *valueString, *c;
  Tcl_Obj *valueObj, *result = NULL, *savedResult = NULL;
  int flgs = TCL_LEAVE_ERR_MSG;
  Tcl_CallFrame frame, *framePtr = &frame;

  XOTcl_PushFrameObj(interp, object, framePtr);
  if (object->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  valueObj = Tcl_ObjGetVar2(interp, XOTclGlobalObjs[XOTE_AUTONAMES], nameObj, flgs);
  if (valueObj) {
    long autoname_counter;
    /* should probably do an overflow check here */
    Tcl_GetLongFromObj(interp, valueObj, &autoname_counter);
    autoname_counter++;
    if (Tcl_IsShared(valueObj)) {
      valueObj = Tcl_DuplicateObj(valueObj);
    }
    Tcl_SetLongObj(valueObj, autoname_counter);
  }
  Tcl_ObjSetVar2(interp, XOTclGlobalObjs[XOTE_AUTONAMES], nameObj,
		 valueObj, flgs);

  if (resetOpt) {
    if (valueObj) { /* we have an entry */
      Tcl_UnsetVar2(interp, XOTclGlobalStrings[XOTE_AUTONAMES], ObjStr(nameObj), flgs);
    }
    result = XOTclGlobalObjs[XOTE_EMPTY];
    INCR_REF_COUNT(result);
  } else {
    if (valueObj == NULL) {
      valueObj = Tcl_ObjSetVar2(interp, XOTclGlobalObjs[XOTE_AUTONAMES],
                                   nameObj, XOTclGlobalObjs[XOTE_ONE], flgs);
    }
    if (instanceOpt) {
      char buffer[1], firstChar;
      CONST char *nextChars = ObjStr(nameObj);
      firstChar = *(nextChars ++);
      if (isupper((int)firstChar)) {
        buffer[0] = tolower((int)firstChar);
        result = Tcl_NewStringObj(buffer, 1);
        INCR_REF_COUNT(result);
        Tcl_AppendLimitedToObj(result, nextChars, -1, INT_MAX, NULL);
        mustCopy = 0;
      }
    }
    if (mustCopy) {
      result = Tcl_DuplicateObj(nameObj);
      INCR_REF_COUNT(result);
      /*
        fprintf(stderr, "*** copy %p %s = %p\n", name, ObjStr(name), result);
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
      ov[1] = result;
      ov[2] = valueObj;
      if (XOTclCallCommand(interp, XOTE_FORMAT, 3, ov) != TCL_OK) {
        XOTcl_PopFrameObj(interp, framePtr);
        DECR_REF_COUNT(savedResult);
        FREE_ON_STACK(Tcl_Obj*, ov);
        return 0;
      }
      DECR_REF_COUNT(result);
      result = Tcl_DuplicateObj(Tcl_GetObjResult(interp));
      INCR_REF_COUNT(result);
      Tcl_SetObjResult(interp, savedResult);
      DECR_REF_COUNT(savedResult);
      FREE_ON_STACK(Tcl_Obj*, ov);
    } else {
      valueString = Tcl_GetStringFromObj(valueObj, &valueLength);
      Tcl_AppendLimitedToObj(result, valueString, valueLength, INT_MAX, NULL);
      /*fprintf(stderr, "+++ append to obj done\n");*/
    }
  }

  XOTcl_PopFrameObj(interp, framePtr);
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
CallStackDoDestroy(Tcl_Interp *interp, XOTclObject *object) {
  Tcl_Command oid;

  PRINTOBJ("CallStackDoDestroy", object);

  /* Don't do anything, if a recursive DURING_DELETE is for some
   * reason active.
   */
  if (object->flags & XOTCL_DURING_DELETE) {
    return;
  }
  /*fprintf(stderr, "CallStackDoDestroy %p flags %.6x activation %d cmd %p \n", 
    object, object->flags, object->activationCount, object->id);*/
  object->flags |= XOTCL_DURING_DELETE;
  oid = object->id;
  /* oid might be freed already, we can't even use (((Command*)oid)->flags & CMD_IS_DELETED) */

  if (object->teardown && oid) {

    /* PrimitiveDestroy() has to be before DeleteCommandFromToken(),
       otherwise e.g. unset traces on this object cannot be executed
       from Tcl. We make sure via refcounting that the object structure 
       is kept until after DeleteCommandFromToken().
    */

    object->refCount ++;

    /*fprintf(stderr, "CallStackDoDestroy %p after refCount ++ %d teardown %p\n", 
      object, object->refCount, object->teardown);*/

    PrimitiveDestroy((ClientData) object);
;
    if (!(object->flags & XOTCL_TCL_DELETE) /*&& !(object->flags & XOTCL_CMD_NOT_FOUND)*/) {
      Tcl_Obj *savedObjResult = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(savedObjResult);
      /*fprintf(stderr, "    before DeleteCommandFromToken %p object flags %.6x\n", oid, object->flags);*/
      /*fprintf(stderr, "cmd dealloc %p refcount %d dodestroy \n", oid, Tcl_Command_refCount(oid));*/
      Tcl_DeleteCommandFromToken(interp, oid); /* this can change the result */
      /*fprintf(stderr, "    after DeleteCommandFromToken %p %.6x\n", oid, ((Command*)oid)->flags);*/
      Tcl_SetObjResult(interp, savedObjResult);
      DECR_REF_COUNT(savedObjResult);
    }
    XOTclCleanupObject(object);
  }
}

static void
CallStackDestroyObject(Tcl_Interp *interp, XOTclObject *object) {

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "CallStackDestroyObject %p %s activationcount %d flags %.6x\n",
          object, objectName(object), object->activationCount, object->flags);
#endif

  if ((object->flags & XOTCL_DESTROY_CALLED) == 0) {
    int activationCount = object->activationCount;
    /* if the destroy method was not called yet, do it now */
#ifdef OBJDELETION_TRACE
    fprintf(stderr, "  CallStackDestroyObject has to callDestroyMethod %p activationCount %d\n", 
            object, activationCount);
#endif
    callDestroyMethod(interp, object, 0);

    if (activationCount == 0) {
      /* We assume, the object is now freed. if the oobjectbj is already
         freed, we cannot access activation count, and we cannot call
         CallStackDoDestroy */
      /*fprintf(stderr, "  CallStackDestroyObject %p done\n",  obj);*/
      return;
    }
  }

  /* If the object is not referenced on the callstack anymore
     we have to destroy it directly, because CscFinish won't
     find the object destroy */
  if (object->activationCount == 0) {
    CallStackDoDestroy(interp, object);
  } else {
    /* to prevail the deletion order call delete children now
       -> children destructors are called before parent's
       destructor */
    if (object->teardown && object->nsPtr) {
      /*fprintf(stderr, "  CallStackDestroyObject calls NSDeleteChildren\n");*/
      NSDeleteChildren(interp, object->nsPtr);
    }
  }
  /*fprintf(stderr, "  CallStackDestroyObject %p DONE\n",  object);*/
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
CmdListPrint(Tcl_Interp *interp, CONST char *title, XOTclCmdList *cmdList) {
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
CmdListFindNameInList(Tcl_Interp *interp, CONST char *name, XOTclCmdList *l) {
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
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
  for (; alist; alist = alist->nextPtr) {
    Tcl_ListObjAppendElement(interp, listObj, alist->content);
  }
  return listObj;
}



/* append a string of pre and post assertions to a method body */
static void
AssertionAppendPrePost(Tcl_Interp *interp, Tcl_DString *dsPtr, XOTclProcAssertion *procs) {
  if (procs) {
    Tcl_Obj *preCondition = AssertionList(interp, procs->pre);
    Tcl_Obj *postCondition = AssertionList(interp, procs->post);
    INCR_REF_COUNT(preCondition); INCR_REF_COUNT(postCondition);
    Tcl_DStringAppendElement(dsPtr, "-precondition");
    Tcl_DStringAppendElement(dsPtr, ObjStr(preCondition));
    Tcl_DStringAppendElement(dsPtr, "-postcondition");
    Tcl_DStringAppendElement(dsPtr, ObjStr(postCondition));
    DECR_REF_COUNT(preCondition); DECR_REF_COUNT(postCondition);
  }
}

static int
AssertionListCheckOption(Tcl_Interp *interp, XOTclObject *object) {
  XOTclObjectOpt *opt = object->opt;
  if (!opt)
    return TCL_OK;
  if (opt->checkoptions & CHECK_OBJINVAR)
    Tcl_AppendElement(interp, "object-invar");
  if (opt->checkoptions & CHECK_CLINVAR)
    Tcl_AppendElement(interp, "class-invar");
  if (opt->checkoptions & CHECK_PRE)
    Tcl_AppendElement(interp, "pre");
  if (opt->checkoptions & CHECK_POST)
    Tcl_AppendElement(interp, "post");
  return TCL_OK;
}

static XOTclProcAssertion*
AssertionFindProcs(XOTclAssertionStore *aStore, CONST char *name) {
  Tcl_HashEntry *hPtr;
  if (aStore == NULL) return NULL;
  hPtr = Tcl_CreateHashEntry(&aStore->procs, name, NULL);
  if (hPtr == NULL) return NULL;
  return (XOTclProcAssertion*) Tcl_GetHashValue(hPtr);
}

static void
AssertionRemoveProc(XOTclAssertionStore *aStore, CONST char *name) {
  Tcl_HashEntry *hPtr;
  if (aStore) {
    hPtr = Tcl_CreateHashEntry(&aStore->procs, name, NULL);
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
AssertionAddProc(Tcl_Interp *interp, CONST char *name, XOTclAssertionStore *aStore,
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
  MEM_COUNT_ALLOC("Tcl_InitHashTable", &aStore->procs);
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
    MEM_COUNT_FREE("Tcl_InitHashTable", &aStore->procs);
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
    result = Tcl_GetBooleanFromObj(interp, Tcl_GetObjResult(interp), &success);

    if (result == TCL_OK && success == 0)
      result = XOTCL_CHECK_FAILED;
  }
  return result;
}

static int
AssertionCheckList(Tcl_Interp *interp, XOTclObject *object,
                   XOTclTclObjList *alist, CONST char *methodName) {
  XOTclTclObjList *checkFailed = NULL;
  Tcl_Obj *savedObjResult = Tcl_GetObjResult(interp);
  int savedCheckoptions, acResult = TCL_OK;

  /*
   * no obj->opt -> checkoption == CHECK_NONE
   */
  if (!object->opt)
    return TCL_OK;

  /* we do not check assertion modifying methods, otherwise
     we can not react in catch on a runtime assertion check failure */

#if 1
  /* TODO: the following check operations is xotcl1 legacy and is not
     generic. it should be replaced by another methodproperty.
     Most of the is*String()
     definition are then obsolete and should be deleted from
     xotclInt.h as well.
  */

  if (isCheckString(methodName)) {
    return TCL_OK;
  }
#endif

  INCR_REF_COUNT(savedObjResult);

  Tcl_ResetResult(interp);

  while (alist) {
    /* Eval instead of IfObjCmd => the substitutions in the
       conditions will be done by Tcl */
    CONST char *assStr = ObjStr(alist->content), *c = assStr;
    int comment = 0;

    for (; c && *c != '\0'; c++) {
      if (*c == '#') {
        comment = 1; break;
      }
    }

    if (!comment) {
      Tcl_CallFrame frame, *framePtr = &frame;
      XOTcl_PushFrameObj(interp, object, framePtr);

      /* don't check assertion during assertion check */
      savedCheckoptions = object->opt->checkoptions;
      object->opt->checkoptions = CHECK_NONE;

      /* fprintf(stderr, "Checking Assertion %s ", assStr); */

      /*
       * now check the assertion in the pushed callframe's scope
       */
      acResult = checkConditionInScope(interp, alist->content);
      if (acResult != TCL_OK)
        checkFailed = alist;

      object->opt->checkoptions = savedCheckoptions;
      /* fprintf(stderr, "...%s\n", checkFailed ? "failed" : "ok"); */
      XOTcl_PopFrameObj(interp, framePtr);
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
AssertionCheckInvars(Tcl_Interp *interp, XOTclObject *object, 
                     CONST char *methodName,
                     CheckOptions checkoptions) {
  int result = TCL_OK;

  if (checkoptions & CHECK_OBJINVAR && object->opt->assertions) {
    result = AssertionCheckList(interp, object, object->opt->assertions->invariants,
                                methodName);
  }

  if (result != TCL_ERROR && checkoptions & CHECK_CLINVAR) {
    XOTclClasses *clPtr;
    clPtr = ComputeOrder(object->cl, object->cl->order, Super);
    while (clPtr && result != TCL_ERROR) {
      XOTclAssertionStore *aStore = (clPtr->cl->opt) ? clPtr->cl->opt->assertions : 0;
      if (aStore) {
        result = AssertionCheckList(interp, object, aStore->invariants, methodName);
      }
      clPtr = clPtr->nextPtr;
    }
  }
  return result;
}

static int
AssertionCheck(Tcl_Interp *interp, XOTclObject *object, XOTclClass *cl,
               CONST char *method, int checkOption) {
  XOTclProcAssertion *procs;
  int result = TCL_OK;
  XOTclAssertionStore *aStore;

  if (cl)
    aStore = cl->opt ? cl->opt->assertions : 0;
  else
    aStore = object->opt ? object->opt->assertions : 0;

  assert(object->opt);

  if (checkOption & object->opt->checkoptions) {
    procs = AssertionFindProcs(aStore, method);
    if (procs) {
      switch (checkOption) {
      case CHECK_PRE:
	result = AssertionCheckList(interp, object, procs->pre, method);
        break;
      case CHECK_POST:
        result = AssertionCheckList(interp, object, procs->post, method);
        break;
      }
    }
    if (result != TCL_ERROR)
      result = AssertionCheckInvars(interp, object, method, object->opt->checkoptions);
  }
  return result;
}

static int AssertionSetCheckOptions(Tcl_Interp *interp, XOTclObject *object, Tcl_Obj *arg) {
  XOTclObjectOpt *opt = XOTclRequireObjectOpt(object);
  int ocArgs, i;
  Tcl_Obj **ovArgs;
  opt->checkoptions = CHECK_NONE;

  if (Tcl_ListObjGetElements(interp, arg, &ocArgs, &ovArgs) == TCL_OK
      && ocArgs > 0) {
    for (i = 0; i < ocArgs; i++) {
      CONST char *option = ObjStr(ovArgs[i]);
      if (option) {
        switch (*option) {
        case 'c':
          if (strcmp(option, "class-invar") == 0) {
            opt->checkoptions |= CHECK_CLINVAR;
          }
          break;
        case 'o':
          if (strcmp(option, "object-invar") == 0) {
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
                          objectName(object), " check  ", ObjStr(arg),
                          "', valid: all pre post object-invar class-invar",
                          (char *) NULL);
  }
  return TCL_OK;
}

static void AssertionSetInvariants(Tcl_Interp *interp, XOTclAssertionStore **assertions, Tcl_Obj *arg) {
  if (*assertions)
    TclObjListFreeList((*assertions)->invariants);
  else
    *assertions = AssertionCreateStore();

  (*assertions)->invariants = AssertionNewList(interp, arg);
}





/*
 * Per-Object-Mixins
 */

/*
 * push a mixin stack information on this object
 */
static int
MixinStackPush(XOTclObject *object) {
  register XOTclMixinStack *h = NEW(XOTclMixinStack);
  h->currentCmdPtr = NULL;
  h->nextPtr = object->mixinStack;
  object->mixinStack = h;
  return 1;
}

/*
 * pop a mixin stack information on this object
 */
static void
MixinStackPop(XOTclObject *object) {
  register XOTclMixinStack *h = object->mixinStack;
  object->mixinStack = h->nextPtr;
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
          if (opt && opt->classmixins) {
            /* compute transitively the (class) mixin classes of this
               added class */
            XOTclClasses *cls;
            int i, found = 0;
            for (i=0, cls = *checkList; cls; i++, cls = cls->nextPtr) {
              /* fprintf(stderr, "+++ c%d: %s\n", i,
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

              MixinComputeOrderFullList(interp, &opt->classmixins, mixinClasses,
                                        checkList, level+1);
            }
          }
          /* fprintf(stderr, "+++ add to mixinClasses %p path: %s clPtr %p\n",
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
MixinResetOrder(XOTclObject *object) {
  /*fprintf(stderr, "removeList %s \n", objectName(object));*/
  CmdListRemoveList(&object->mixinOrder, NULL /*GuardDel*/);
  object->mixinOrder = NULL;
}

/*
 * Computes a linearized order of per-object and per-class mixins. Then
 * duplicates in the full list and with the class inheritance list of
 * 'obj' are eliminated.
 * The precendence rule is that the last occurence makes it into the
 * final list.
 */
static void
MixinComputeOrder(Tcl_Interp *interp, XOTclObject *object) {
  XOTclClasses *fullList, *checkList = NULL, *mixinClasses = NULL, *nextCl, *pl,
    *checker, *guardChecker;

  if (object->mixinOrder)  MixinResetOrder(object);

  /* append per-obj mixins */
  if (object->opt) {
    MixinComputeOrderFullList(interp, &object->opt->mixins, &mixinClasses,
                              &checkList, 0);
  }

  /* append per-class mixins */
  for (pl = ComputeOrder(object->cl, object->cl->order, Super); pl; pl = pl->nextPtr) {
    XOTclClassOpt *opt = pl->cl->opt;
    if (opt && opt->classmixins) {
      MixinComputeOrderFullList(interp, &opt->classmixins, &mixinClasses,
                                &checkList, 0);
    }
  }
  fullList = mixinClasses;

  /* use no duplicates & no classes of the precedence order
     on the resulting list */
  while (mixinClasses) {
    checker = nextCl = mixinClasses->nextPtr;
    /* fprintf(stderr, "--- checking %s\n",
       ObjStr(mixinClasses->cl->object.cmdName));*/

    while (checker) {
      if (checker->cl == mixinClasses->cl) break;
      checker = checker->nextPtr;
    }
    /* if checker is set, it is a duplicate and ignored */

    if (checker == NULL) {
      /* check obj->cl hierachy */
      for (checker = ComputeOrder(object->cl, object->cl->order, Super); checker; checker = checker->nextPtr) {
        if (checker->cl == mixinClasses->cl)
          break;
      }
      /* if checker is set, it was found in the class hierarchy
         and it is ignored */
    }
    if (checker == NULL) {
      /* add the class to the mixinOrder list */
      XOTclCmdList *new;
      /* fprintf(stderr, "--- adding to mixinlist %s\n",
         ObjStr(mixinClasses->cl->object.cmdName));*/
      new = CmdListAdd(&object->mixinOrder, mixinClasses->cl->object.id, NULL,
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

  /*CmdListPrint(interp, "mixin order\n", obj->mixinOrder);*/

}

/*
 * add a mixin class to 'mixinList' by appending it
 */
static int
MixinAdd(Tcl_Interp *interp, XOTclCmdList **mixinList, Tcl_Obj *nameObj, XOTclClass *baseClass) {
  XOTclClass *mixin;
  Tcl_Obj *guardObj = NULL;
  int ocName; Tcl_Obj **ovName;
  XOTclCmdList *new;

  if (Tcl_ListObjGetElements(interp, nameObj, &ocName, &ovName) == TCL_OK && ocName > 1) {
    if (ocName == 3 && !strcmp(ObjStr(ovName[1]), XOTclGlobalStrings[XOTE_GUARD_OPTION])) {
      nameObj = ovName[0];
      guardObj = ovName[2];
      /*fprintf(stderr, "mixinadd name = '%s', guard = '%s'\n", ObjStr(name), ObjStr(guard));*/
    } /*else return XOTclVarErrMsg(interp, "mixin registration '", ObjStr(name),
        "' has too many elements.", (char *) NULL);*/
  }

  if (GetClassFromObj(interp, nameObj, &mixin, baseClass) != TCL_OK)
    return XOTclErrBadVal(interp, "mixin", "a class as mixin", ObjStr(nameObj));


  new = CmdListAdd(mixinList, mixin->object.id, NULL, /*noDuplicates*/ 1);

  if (guardObj) {
    GuardAdd(interp, new, guardObj);
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
AppendMatchingElement(Tcl_Interp *interp, Tcl_Obj *nameObj, CONST char *pattern) {
  CONST char *string = ObjStr(nameObj);
  if (!pattern || Tcl_StringMatch(string, pattern)) {
    Tcl_AppendElement(interp, string);
  }
}

/*
 * apply AppendMatchingElement to CmdList
 */
static int
AppendMatchingElementsFromCmdList(Tcl_Interp *interp, XOTclCmdList *cmdl,
                                  CONST char *pattern, XOTclObject *matchObject) {
  int rc = 0;
  for ( ; cmdl; cmdl = cmdl->nextPtr) {
    XOTclObject *object = XOTclGetObjectFromCmdPtr(cmdl->cmdPtr);
    if (object) {
      if (matchObject == object) {
        return 1;
      } else {
        AppendMatchingElement(interp, object->cmdName, pattern);
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
				  CONST char *pattern, XOTclObject *matchObject) {
  int rc = 0;

  for ( ; cls; cls = cls->nextPtr) {
    XOTclObject *object = (XOTclObject *)cls->cl;
    if (object) {
      if (matchObject && object == matchObject) {
        /* we have a matchObject and it is identical to obj,
           just return true and don't continue search
        */
        return 1;
        break;
      } else {
        AppendMatchingElement(interp, object->cmdName, pattern);
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
addToResultSet(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclObject *object, int *new,
	       int appendResult, CONST char *pattern, XOTclObject *matchObject) {
  Tcl_CreateHashEntry(destTable, (char *)object, new);
  if (*new) {
    if (matchObject && matchObject == object) {
      return 1;
    }
    if (appendResult) {
      AppendMatchingElement(interp, object->cmdName, pattern);
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
			 int appendResult, CONST char *pattern, XOTclObject *matchObject) {
  Tcl_CreateHashEntry(destTable, (char *)cl, new);
  if (*new) {
    if (appendResult) {
      if (!pattern || Tcl_StringMatch(className(cl), pattern)) {
        Tcl_Obj *l = Tcl_NewListObj(0, NULL);
        Tcl_Obj *g = (Tcl_Obj*) clientData;
        Tcl_ListObjAppendElement(interp, l, cl->object.cmdName);
        Tcl_ListObjAppendElement(interp, l, XOTclGlobalObjs[XOTE_GUARD_OPTION]);
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
		     int appendResult, CONST char *pattern, XOTclObject *matchObject) {
  int rc = 0, new = 0;
  XOTclClasses *sc;

  /*fprintf(stderr, "startCl = %s, opt %p, isMixin %d, pattern '%s', matchObject %p\n",
    className(startCl), startCl->opt, isMixin, pattern, matchObject);*/

  /*
   * check all subclasses of startCl for mixins
   */
  for (sc = startCl->sub; sc; sc = sc->nextPtr) {
    rc = getAllObjectMixinsOf(interp, destTable, sc->cl, isMixin, appendResult, pattern, matchObject);
    if (rc) {return rc;}
  }
  /*fprintf(stderr, "check subclasses of %s done\n", ObjStr(startCl->object.cmdName));*/

  if (startCl->opt) {
    XOTclCmdList *m;
    XOTclClass *cl;
    for (m = startCl->opt->isClassMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == 0);

      cl = XOTclGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);
      /*fprintf(stderr, "check %s mixinof %s\n",
        className(cl), ObjStr(startCl->object.cmdName));*/
      rc = getAllObjectMixinsOf(interp, destTable, cl, isMixin, appendResult, pattern, matchObject);
      /* fprintf(stderr, "check %s mixinof %s done\n",
      className(cl), ObjStr(startCl->object.cmdName));*/
      if (rc) {return rc;}
    }
  }

  /*
   * check, if startCl has associated per-object mixins
   */
  if (startCl->opt) {
    XOTclCmdList *m;
    XOTclObject *object;

    for (m = startCl->opt->isObjectMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == 0);

      object = XOTclGetObjectFromCmdPtr(m->cmdPtr);
      assert(object);

      rc = addToResultSet(interp, destTable, object, &new, appendResult, pattern, matchObject);
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
getAllClassMixinsOf(Tcl_Interp *interp, Tcl_HashTable *destTable, /*@notnull@*/ XOTclClass *startCl,
		    int isMixin,
                    int appendResult, CONST char *pattern, XOTclObject *matchObject) {
  int rc = 0, new = 0;
  XOTclClass *cl;
  XOTclClasses *sc;

  assert(startCl);
  
  /*fprintf(stderr, "startCl = %p %s, opt %p, isMixin %d\n",
    startCl, className(startCl), startCl->opt, isMixin);*/

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
      if (sc->cl != startCl) {
        rc = getAllClassMixinsOf(interp, destTable, sc->cl, isMixin, appendResult, pattern, matchObject);
        if (rc) {return rc;}
      } else {
        /* TODO: sanity check; it seems that we can create via
           __default_superclass a class which has itself als
           subclass */
        fprintf(stderr, "... STRANGE %p is subclass of %p %s, sub %p\n", sc->cl, 
                startCl, className(startCl), startCl->sub);
      }
    }
  }

  /*
   * check, if startCl is a per-class mixin of some other classes
   */
  if (startCl->opt) {
    XOTclCmdList *m;

    for (m = startCl->opt->isClassMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == 0);

      cl = XOTclGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);

      rc = addToResultSet(interp, destTable, &cl->object, &new, appendResult, pattern, matchObject);
      if (rc == 1) {return rc;}
      if (new) {
        /*fprintf(stderr, "... new\n");*/
        rc = getAllClassMixinsOf(interp, destTable, cl, 1, appendResult, pattern, matchObject);
        if (rc) {return rc;}
      }
    }
  }

  return rc;
}

/*
 * recursively get all classmixins of a class into an initialized
 * object ptr hashtable (TCL_ONE_WORD_KEYS)
 */

static int
getAllClassMixins(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclClass *startCl,
		  int withGuards, CONST char *pattern, XOTclObject *matchObject) {
  int rc = 0, new = 0;
  XOTclClass *cl;
  XOTclClasses *sc;

  /*
   * check this class for classmixins
   */
  if (startCl->opt) {
    XOTclCmdList *m;

    for (m = startCl->opt->classmixins; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == 0);

      cl = XOTclGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);

      /* fprintf(stderr, "class mixin found: %s\n", className(cl)); */

      if ((withGuards) && (m->clientData)) {
        /* fprintf(stderr, "addToResultSetWithGuards: %s\n", className(cl)); */
        rc = addToResultSetWithGuards(interp, destTable, cl, m->clientData, &new, 1, pattern, matchObject);
      } else {
        /* fprintf(stderr, "addToResultSet: %s\n", className(cl)); */
	rc = addToResultSet(interp, destTable, &cl->object, &new, 1, pattern, matchObject);
      }
      if (rc == 1) {return rc;}

      if (new) {
        /* fprintf(stderr, "class mixin getAllClassMixins for: %s (%s)\n", className(cl), ObjStr(startCl->object.cmdName)); */
        rc = getAllClassMixins(interp, destTable, cl, withGuards, pattern, matchObject);
        if (rc) {return rc;}
      }
    }
  }


  /*
   * check all superclasses of startCl for classmixins
   */
  for (sc = startCl->super; sc; sc = sc->nextPtr) {
    /* fprintf(stderr, "Superclass getAllClassMixins for %s (%s)\n", ObjStr(sc->cl->object.cmdName), ObjStr(startCl->object.cmdName)); */
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
        /* fprintf(stderr, "Removing class %s from isClassMixinOf of class %s\n",
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
        /* fprintf(stderr, "Removing object %s from isObjectMixinOf of Class %s\n",
           objectName(object), ObjStr(XOTclGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
        del = CmdListRemoveFromList(&clopt->isObjectMixinOf, del);
        CmdListDeleteCmdListEntry(del, GuardDel);
      }
    } /* else fprintf(stderr, "CleanupDestroyObject %s: NULL pointer in mixins!\n", objectName(object)); */
  }
}

static void
RemoveFromClassmixins(Tcl_Command cmd, XOTclCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    XOTclClass *cl = XOTclGetClassFromCmdPtr(cmdlist->cmdPtr);
    XOTclClassOpt *clopt = cl ? cl->opt : NULL;
    if (clopt) {
      XOTclCmdList *del = CmdListFindCmdInList(cmd, clopt->classmixins);
      if (del) {
        /* fprintf(stderr, "Removing class %s from mixins of object %s\n",
           className(cl), ObjStr(XOTclGetObjectFromCmdPtr(cmdlist->cmdPtr)->cmdName)); */
        del = CmdListRemoveFromList(&clopt->classmixins, del);
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
        /* fprintf(stderr, "Removing class %s from mixins of object %s\n",
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

  /*fprintf(stderr, "invalidating instances of class %s\n",
    ObjStr(clPtr->cl->object.cmdName));*/

  /* Here we should check, whether this class is used as an object or
     class mixin somewhere else and invalidate the objects of these as
     well -- */

  for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    XOTclObject *object = (XOTclObject *)Tcl_GetHashKey(&cl->instances, hPtr);
    if (object
        && !(object->flags & XOTCL_DURING_DELETE)
        && (object->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID)) {
      MixinResetOrder(object);
      object->flags &= ~XOTCL_MIXIN_ORDER_VALID;
    }
  }
}

/* reset mixin order for all objects having this class as per object mixin */
static void
ResetOrderOfClassesUsedAsMixins(XOTclClass *cl) {
  /*fprintf(stderr, "ResetOrderOfClassesUsedAsMixins %s - %p\n",
    className(cl), cl->opt);*/

  if (cl->opt) {
    XOTclCmdList *ml;
    for (ml = cl->opt->isObjectMixinOf; ml; ml = ml->nextPtr) {
      XOTclObject *object = XOTclGetObjectFromCmdPtr(ml->cmdPtr);
      if (object) {
        if (object->mixinOrder) { MixinResetOrder(object); }
        object->flags &= ~XOTCL_MIXIN_ORDER_VALID;
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

  /* reset mixin order for all instances of the class and the
     instances of its subclasses
  */
  for (clPtr = ComputeOrder(cl, cl->order, Sub); clPtr; clPtr = clPtr->nextPtr) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
      Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : NULL;

    /* reset mixin order for all objects having this class as per object mixin */
    ResetOrderOfClassesUsedAsMixins(clPtr->cl);

    /* fprintf(stderr, "invalidating instances of class %s\n", ObjStr(clPtr->cl->object.cmdName));
     */

    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      XOTclObject *object = (XOTclObject *)Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      if (object->mixinOrder) { MixinResetOrder(object); }
      object->flags &= ~XOTCL_MIXIN_ORDER_VALID;
    }
  }

  XOTclClassListFree(cl->order);
  cl->order = saved;

  /* Reset mixin order for all objects having this class as a per
     class mixin.  This means that we have to work through
     the class mixin hierarchy with its corresponding instances.
  */
  Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
  getAllClassMixinsOf(interp, commandTable, cl, 1, 0, NULL, NULL);

  for (hPtr = Tcl_FirstHashEntry(commandTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    XOTclClass *ncl = (XOTclClass *)Tcl_GetHashKey(commandTable, hPtr);
    /*fprintf(stderr, "Got %s, reset for ncl %p\n", ncl?ObjStr(ncl->object.cmdName):"NULL", ncl);*/
    if (ncl) {
      MixinResetOrderForInstances(interp, ncl);
      /* this place seems to be sufficient to invalidate the computed object parameter definitions */
      /*fprintf(stderr, "MixinInvalidateObjOrders via class mixin %s calls ifd invalidate \n", className(ncl));*/
      XOTclInvalidateObjectParameterCmd(interp, ncl);
    }
  }
  Tcl_DeleteHashTable(commandTable);
  MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
}


static int MixinInfo(Tcl_Interp *interp, XOTclCmdList *m, CONST char *pattern,
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
MixinComputeDefined(Tcl_Interp *interp, XOTclObject *object) {
  MixinComputeOrder(interp, object);
  object->flags |= XOTCL_MIXIN_ORDER_VALID;
  if (object->mixinOrder)
    object->flags |= XOTCL_MIXIN_ORDER_DEFINED;
  else
    object->flags &= ~XOTCL_MIXIN_ORDER_DEFINED;
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
static int
MixinSearchProc(Tcl_Interp *interp, XOTclObject *object, CONST char *methodName,
                XOTclClass **cl, Tcl_Command *currentCmdPtr, Tcl_Command *cmdPtr) {
  Tcl_Command cmd = NULL;
  XOTclCmdList *cmdList;
  XOTclClass *cls;
  int result = TCL_OK;

  assert(object);
  assert(object->mixinStack);

  /* ensure that the mixin order is not invalid, otherwise compute order */
  assert(object->flags & XOTCL_MIXIN_ORDER_VALID);
  /*MixinComputeDefined(interp, object);*/
  cmdList = seekCurrent(object->mixinStack->currentCmdPtr, object->mixinOrder);
  RUNTIME_STATE(interp)->cmdPtr = cmdList ? cmdList->cmdPtr : NULL;

  /* fprintf(stderr, "MixinSearch searching for '%s' %p\n", methodName, cmdList); */
  /*CmdListPrint(interp, "MixinSearch CL = \n", cmdList);*/

  for (; cmdList; cmdList = cmdList->nextPtr) {

    if (Tcl_Command_cmdEpoch(cmdList->cmdPtr)) {
      continue;
    } 
    cls = XOTclGetClassFromCmdPtr(cmdList->cmdPtr);
    assert(cls);
    /*
      fprintf(stderr, "+++ MixinSearch %s->%s in %p cmdPtr %p clientData %p\n",
      objectName(object), methodName, cmdList,
      cmdList->cmdPtr, cmdList->clientData);
    */
    cmd = FindMethod(cls->nsPtr, methodName);
    if (cmd == NULL) {
      continue;
    }

    if (Tcl_Command_flags(cmd) & XOTCL_CMD_CLASS_ONLY_METHOD) {
      /*fprintf(stderr, "we found class specific method %s on class %s object %s, isclass %d\n",
	methodName, className(cls), objectName(object), XOTclObjectIsClass(object));*/
      if (!XOTclObjectIsClass(object)) {
	/* the command is not for us; skip it */
	cmd = NULL;
	continue;
      }
    }
    
    if (cmdList->clientData) {
      if (!RUNTIME_STATE(interp)->guardCount) {
	result = GuardCall(object, cls, (Tcl_Command) cmd, interp,
			   (Tcl_Obj*)cmdList->clientData, NULL);
      }
    }
    if (result == TCL_OK) {
      /*
       * on success: compute mixin call data
       */
      *cl = cls;
      *currentCmdPtr = cmdList->cmdPtr;
      break;
    } else if (result == TCL_ERROR) {
      break;
    } else {
      if (result == XOTCL_CHECK_FAILED) result = TCL_OK;
      cmd = NULL;
    }
  }

  *cmdPtr = cmd;
  return result;
}

/*
 * info option for mixins and classmixins
 */
static int
MixinInfo(Tcl_Interp *interp, XOTclCmdList *m, CONST char *pattern,
          int withGuards, XOTclObject *matchObject) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  XOTclClass *mixinClass;

  /*fprintf(stderr, "   mixin info m=%p, pattern %s, matchObject %p\n",
    m, pattern, matchObject);*/

  while (m) {
    /* fprintf(stderr, "   mixin info m=%p, next=%p, pattern %s, matchObject %p\n",
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
        Tcl_ListObjAppendElement(interp, l, XOTclGlobalObjs[XOTE_GUARD_OPTION]);
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
MixinSearchMethodByName(Tcl_Interp *interp, XOTclCmdList *mixinList, CONST char *name, XOTclClass **cl) {
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
 * The search method implements filter search order for object and
 * class ilter: first a given name is interpreted as fully qualified
 * method name. If no method is found, a proc is searched with fully
 * name. Otherwise the simple name is searched on the heritage order:
 * object (only for per-object filters), class, meta-class
 */

static Tcl_Command
FilterSearch(Tcl_Interp *interp, CONST char *name, XOTclObject *startingObject,
             XOTclClass *startingClass, XOTclClass **cl) {
  Tcl_Command cmd = NULL;

  if (startingObject) {
    XOTclObjectOpt *opt = startingObject->opt;
    /*
     * the object-specific filter can also be defined on the object's
     * class, its hierarchy, or the respective classmixins; thus use the
     * object's class as start point for the class-specific search then ...
     */
    startingClass = startingObject->cl;

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
   * search for classfilters on classmixins
   */
  if (startingClass) {
    XOTclClassOpt *opt = startingClass->opt;
    if (opt && opt->classmixins) {
      if ((cmd = MixinSearchMethodByName(interp, opt->classmixins, name, cl))) {
        return cmd;
      }
    }
  }

  /*
   * seach for object procs that are used as filters
   */
  if (startingObject && startingObject->nsPtr) {
    /*fprintf(stderr, "search filter %s as proc \n", name);*/
    if ((cmd = FindMethod(startingObject->nsPtr, name))) {
      *cl = (XOTclClass*)startingObject;
      return cmd;
    }
  }

  /*
   * ok, no filter on obj or mixins -> search class
   */
  if (startingClass) {
    *cl = SearchCMethod(startingClass, name, &cmd);
    if (!*cl) {
      /*
       * If no filter is found yet -> search the meta-class
       */
      *cl = SearchCMethod(startingClass->object.cl, name, &cmd);
    }
  }
  return cmd;
}

/*
 * Filter Guards
 */

/* check a filter guard, return 1 if ok */
static int
GuardCheck(Tcl_Interp *interp, Tcl_Obj *guardObj) {
  int result;
  XOTclRuntimeState *rst = RUNTIME_STATE(interp);

  if (guardObj) {
    /*
     * if there are more than one filter guard for this filter
     * (i.e. they are inherited), then they are OR combined
     * -> if one check succeeds => return 1
     */

    /*fprintf(stderr, "checking guard **%s**\n", ObjStr(guardObj));*/

    rst->guardCount++;
    result = checkConditionInScope(interp, guardObj);
    rst->guardCount--;

    /*fprintf(stderr, "checking guard **%s** returned rc=%d\n", ObjStr(guardObj), rc);*/

    if (result == TCL_OK) {
      /* fprintf(stderr, " +++ OK\n"); */
      return TCL_OK;
    } else if (result == TCL_ERROR) {
      Tcl_Obj *sr = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(sr);

      /* fprintf(stderr, " +++ ERROR\n");*/

      XOTclVarErrMsg(interp, "Guard Error: '", ObjStr(guardObj), "'\n\n",
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
  Tcl_Obj *guardObj = (TclObj*) clientData;
  fprintf(stderr, " +++ <GUARDS> \n");
  if (guardObj) {
  fprintf(stderr, "   *     %s \n", ObjStr(guardObj));
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
GuardAdd(Tcl_Interp *interp, XOTclCmdList *CL, Tcl_Obj *guardObj) {
  if (guardObj) {
    GuardDel(CL);
    if (strlen(ObjStr(guardObj)) != 0) {
      INCR_REF_COUNT(guardObj);
      CL->clientData = (ClientData) guardObj;
      /*fprintf(stderr, "guard added to %p cmdPtr=%p, clientData= %p\n",
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
GuardCall(XOTclObject *object, XOTclClass *cl, Tcl_Command cmd,
          Tcl_Interp *interp, Tcl_Obj *guardObj, XOTclCallStackContent *cscPtr) {
  int result = TCL_OK;

  if (guardObj) {
    Tcl_Obj *res = Tcl_GetObjResult(interp); /* save the result */
    Tcl_CallFrame frame, *framePtr = &frame;

    INCR_REF_COUNT(res);

    /* GuardPrint(interp, cmdList->clientData); */
    /*
     * For the guard push a fake callframe on the Tcl stack so that
     * e.g. a "self calledproc" and other methods in the guard behave
     * like in the proc.
     */
    if (cscPtr) {
      XOTcl_PushFrameCsc(interp, cscPtr, framePtr);
    } else {
      XOTcl_PushFrameObj(interp, object, framePtr);
    }
    result = GuardCheck(interp, guardObj);

    if (cscPtr) {
      XOTcl_PopFrameCsc(interp, framePtr);
    } else {
      XOTcl_PopFrameObj(interp, framePtr);
    }

    if (result != TCL_ERROR) {
      Tcl_SetObjResult(interp, res);  /* restore the result */
    }
    DECR_REF_COUNT(res);
  }

  return result;
}

static int
GuardAddFromDefinitionList(Tcl_Interp *interp, XOTclCmdList *dest,
                           Tcl_Command interceptorCmd,
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
                        XOTclObject *object, Tcl_Command filterCmd) {
  XOTclClasses *pl;
  int guardAdded = 0;
  XOTclObjectOpt *opt;

  /* search guards for classfilters registered on mixins */
  if (!(object->flags & XOTCL_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, object);
  if (object->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
    XOTclCmdList *ml;
    XOTclClass *mixin;
    for (ml = object->mixinOrder; ml && !guardAdded; ml = ml->nextPtr) {
      mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin && mixin->opt) {
        guardAdded = GuardAddFromDefinitionList(interp, dest, filterCmd,
                                                mixin->opt->classfilters);
      }
    }
  }

  /* search per-object filters */
  opt = object->opt;
  if (!guardAdded && opt && opt->filters) {
    guardAdded = GuardAddFromDefinitionList(interp, dest, filterCmd, opt->filters);
  }

  if (!guardAdded) {
    /* search per-class filters */
    for (pl = ComputeOrder(object->cl, object->cl->order, Super); !guardAdded && pl; pl = pl->nextPtr) {
      XOTclClassOpt *opt = pl->cl->opt;
      if (opt) {
        guardAdded = GuardAddFromDefinitionList(interp, dest, filterCmd,
                                                opt->classfilters);
      }
    }


    /*
     * if this is not a registered filter, it is an inherited filter, like:
     *   Class create A
     *   A method f ...
     *   Class create B -superclass A
     *   B method {{f {<guard>}}}
     *   B filter f
     * -> get the guard from the filter that inherits it (here B->f)
     */
    if (!guardAdded) {
      XOTclCmdList *registeredFilter =
        CmdListFindNameInList(interp, (char *) Tcl_GetCommandName(interp, filterCmd),
                              object->filterOrder);
      if (registeredFilter) {
        GuardAdd(interp, dest, (Tcl_Obj*) registeredFilter->clientData);
      }
    }
  }
}

static int
GuardList(Tcl_Interp *interp, XOTclCmdList *frl, CONST char *interceptorName) {
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
FilterAdd(Tcl_Interp *interp, XOTclCmdList **filterList, Tcl_Obj *nameObj,
          XOTclObject *startingObject, XOTclClass *startingClass) {
  Tcl_Command cmd;
  int ocName; Tcl_Obj **ovName;
  Tcl_Obj *guardObj = NULL;
  XOTclCmdList *new;
  XOTclClass *cl;

  if (Tcl_ListObjGetElements(interp, nameObj, &ocName, &ovName) == TCL_OK && ocName > 1) {
    if (ocName == 3 && !strcmp(ObjStr(ovName[1]), XOTclGlobalStrings[XOTE_GUARD_OPTION])) {
      nameObj = ovName[0];
      guardObj = ovName[2];
    }
  }

  if (!(cmd = FilterSearch(interp, ObjStr(nameObj), startingObject, startingClass, &cl))) {
    if (startingObject)
      return XOTclVarErrMsg(interp, "object filter: can't find filterproc on: ",
                            objectName(startingObject), " - proc: ",
                            ObjStr(nameObj), (char *) NULL);
    else
      return XOTclVarErrMsg(interp, "class filter: can't find filterproc on: ",
                            className(startingClass), " - proc: ",
                            ObjStr(nameObj), (char *) NULL);
  }

  /*fprintf(stderr, " +++ adding filter %s cl %p\n", ObjStr(nameObj), cl);*/

  new = CmdListAdd(filterList, cmd, cl, /*noDuplicates*/ 1);

  if (guardObj) {
    GuardAdd(interp, new, guardObj);
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
FilterResetOrder(XOTclObject *object) {
  CmdListRemoveList(&object->filterOrder, GuardDel);
  object->filterOrder = NULL;
}

/*
 * search the filter in the hierarchy again with FilterSearch, e.g.
 * upon changes in the class hierarchy or mixins that carry the filter
 * command, so that we can be sure it is still reachable.
 */
static void
FilterSearchAgain(Tcl_Interp *interp, XOTclCmdList **filters,
                  XOTclObject *startingObject, XOTclClass *startingClass) {
  char *simpleName;
  Tcl_Command cmd;
  XOTclCmdList *cmdList, *del;
  XOTclClass *cl = NULL;

  CmdListRemoveEpoched(filters, GuardDel);
  for (cmdList = *filters; cmdList; ) {
    simpleName = (char *) Tcl_GetCommandName(interp, cmdList->cmdPtr);
    cmd = FilterSearch(interp, simpleName, startingObject, startingClass, &cl);
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

    /* recalculate the commands of all class-filter registrations */
    if (clPtr->cl->opt) {
      FilterSearchAgain(interp, &clPtr->cl->opt->classfilters, 0, clPtr->cl);
    }
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      XOTclObject *object = (XOTclObject *)Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      FilterResetOrder(object);
      object->flags &= ~XOTCL_FILTER_ORDER_VALID;

      /* recalculate the commands of all object filter registrations */
      if (object->opt) {
        FilterSearchAgain(interp, &object->opt->filters, object, 0);
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
      CmdListRemoveContextClassFromList(&opt->classfilters, removeClass, GuardDel);
    }
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      XOTclObject *object = (XOTclObject*) Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      if (object->opt) {
        CmdListRemoveContextClassFromList(&object->opt->filters, removeClass, GuardDel);
      }
    }
  }

  XOTclClassListFree(cl->order);
  cl->order = saved;
}

static Tcl_Obj *
MethodHandleObj(XOTclObject *object, int withPer_object, CONST char *methodName) {
  Tcl_Obj *resultObj = Tcl_NewStringObj(withPer_object ? "" : "::nsf::classes", -1);
  assert(object);
  Tcl_AppendObjToObj(resultObj, object->cmdName);
  Tcl_AppendStringsToObj(resultObj, "::", methodName, (char *) NULL);
  return resultObj;
}

/*
 * info option for filters and classfilters
 * withGuards -> if not 0 => append guards
 * withMethodHandles -> if not 0 => return method handles
 */
static int
FilterInfo(Tcl_Interp *interp, XOTclCmdList *f, CONST char *pattern,
           int withGuards, int withMethodHandles) {
  CONST char *simpleName;
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);

  /*fprintf(stderr, "FilterInfo %p %s %d %d\n", pattern, pattern, 
    withGuards, withMethodHandles);*/

  /* guard lists should only have unqualified filter lists when
     withGuards is activated, withMethodHandles has no effect 
  */
  if (withGuards) {
    withMethodHandles = 0;
  }

  while (f) {
    simpleName = Tcl_GetCommandName(interp, f->cmdPtr);
    if (!pattern || Tcl_StringMatch(simpleName, pattern)) {
      if (withGuards && f->clientData) {
        Tcl_Obj *innerList = Tcl_NewListObj(0, NULL);
        Tcl_Obj *g = (Tcl_Obj*) f->clientData;
        Tcl_ListObjAppendElement(interp, innerList,
                                 Tcl_NewStringObj(simpleName, -1));
        Tcl_ListObjAppendElement(interp, innerList, XOTclGlobalObjs[XOTE_GUARD_OPTION]);
        Tcl_ListObjAppendElement(interp, innerList, g);
        Tcl_ListObjAppendElement(interp, list, innerList);
      } else {
        if (withMethodHandles) {
	  XOTclClass *filterClass = f->clorobj;
          Tcl_ListObjAppendElement(interp, list,
				   MethodHandleObj((XOTclObject *)filterClass, 
						   !XOTclObjectIsClass(&filterClass->object), simpleName));
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
      /* get the class from the object for per-object filter */
      fcl = ((XOTclObject *)fcl)->cl;
    }

    /* if we have a filter class -> search up the inheritance hierarchy*/
    if (fcl) {
      pl = ComputeOrder(fcl, fcl->order, Super);
      if (pl && pl->nextPtr) {
        /* don't search on the start class again */
        pl = pl->nextPtr;
        /* now go up the hierarchy */
        for(; pl; pl = pl->nextPtr) {
          Tcl_Command pi = FindMethod(pl->cl->nsPtr, simpleName);
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
  /*CmdListPrint(interp, "FilterComputeOrderFullList....\n", *filterList);*/
}

/*
 * Computes a linearized order of object and class filter. Then
 * duplicates in the full list and with the class inheritance list of
 * 'obj' are eliminated.  The precendence rule is that the last
 * occurence makes it into the final list.
 */
static void
FilterComputeOrder(Tcl_Interp *interp, XOTclObject *object) {
  XOTclCmdList *filterList = NULL, *next, *checker, *newlist;
  XOTclClasses *pl;

  if (object->filterOrder) FilterResetOrder(object);
  /*
    fprintf(stderr, "<Filter Order obj=%s> List: ", objectName(object));
  */

  /* append classfilters registered for mixins */
  if (!(object->flags & XOTCL_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, object);

  if (object->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
    XOTclCmdList *ml;
    XOTclClass *mixin;

    for (ml = object->mixinOrder; ml; ml = ml->nextPtr) {
      mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin && mixin->opt && mixin->opt->classfilters)
        FilterComputeOrderFullList(interp, &mixin->opt->classfilters, &filterList);
    }
  }

  /* append per-obj filters */
  if (object->opt)
    FilterComputeOrderFullList(interp, &object->opt->filters, &filterList);

  /* append per-class filters */
  for (pl = ComputeOrder(object->cl, object->cl->order, Super); pl; pl=pl->nextPtr) {
    XOTclClassOpt *opt = pl->cl->opt;
    if (opt && opt->classfilters) {
      FilterComputeOrderFullList(interp, &opt->classfilters, &filterList);
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
      newlist = CmdListAdd(&object->filterOrder, filterList->cmdPtr, filterList->clorobj,
                           /*noDuplicates*/ 0);
      GuardAddInheritedGuards(interp, newlist, object, filterList->cmdPtr);
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
FilterComputeDefined(Tcl_Interp *interp, XOTclObject *object) {
  FilterComputeOrder(interp, object);
  object->flags |= XOTCL_FILTER_ORDER_VALID;
  if (object->filterOrder)
    object->flags |= XOTCL_FILTER_ORDER_DEFINED;
  else
    object->flags &= ~XOTCL_FILTER_ORDER_DEFINED;
}

/*
 * push a filter stack information on this object
 */
static int
FilterStackPush(Tcl_Interp *interp, XOTclObject *object, Tcl_Obj *calledProc) {
  register XOTclFilterStack *h = NEW(XOTclFilterStack);

  h->currentCmdPtr = NULL;
  h->calledProc = calledProc;
  INCR_REF_COUNT(h->calledProc);
  h->nextPtr = object->filterStack;
  object->filterStack = h;
  return 1;
}

/*
 * pop a filter stack information on this object
 */
static void
FilterStackPop(XOTclObject *object) {
  register XOTclFilterStack *h = object->filterStack;
  object->filterStack = h->nextPtr;

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
 * "<class> filter <filterName>,
 * or an empty list, if not registered
 */
static Tcl_Obj *
FilterFindReg(Tcl_Interp *interp, XOTclObject *object, Tcl_Command cmd) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  XOTclClasses *pl;

  /* search per-object filters */
  if (object->opt && CmdListFindCmdInList(cmd, object->opt->filters)) {
    Tcl_ListObjAppendElement(interp, list, object->cmdName);
    Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjs[XOTE_OBJECT]);
    Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjs[XOTE_FILTER]);
    Tcl_ListObjAppendElement(interp, list,
                             Tcl_NewStringObj(Tcl_GetCommandName(interp, cmd), -1));
    return list;
  }

  /* search per-class filters */
  for (pl = ComputeOrder(object->cl, object->cl->order, Super); pl; pl = pl->nextPtr) {
    XOTclClassOpt *opt = pl->cl->opt;
    if (opt && opt->classfilters) {
      if (CmdListFindCmdInList(cmd, opt->classfilters)) {
        Tcl_ListObjAppendElement(interp, list, pl->cl->object.cmdName);
        Tcl_ListObjAppendElement(interp, list, XOTclGlobalObjs[XOTE_FILTER]);
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
FilterSearchProc(Tcl_Interp *interp, XOTclObject *object,
                 Tcl_Command *currentCmd, XOTclClass **cl) {
  XOTclCmdList *cmdList;

  assert(object);
  assert(object->filterStack);

  *currentCmd = NULL;

  /* Ensure that the filter order is not invalid, otherwise compute order
     FilterComputeDefined(interp, object);
  */
  assert(object->flags & XOTCL_FILTER_ORDER_VALID);
  cmdList = seekCurrent(object->filterStack->currentCmdPtr, object->filterOrder);

  while (cmdList) {
    if (Tcl_Command_cmdEpoch(cmdList->cmdPtr)) {
      cmdList = cmdList->nextPtr;
    } else if (FilterActiveOnObj(interp, object, cmdList->cmdPtr)) {
      /* fprintf(stderr, "Filter <%s> -- Active on: %s\n",
         Tcl_GetCommandName(interp, (Tcl_Command)cmdList->cmdPtr), objectName(object));
      */
      object->filterStack->currentCmdPtr = cmdList->cmdPtr;
      cmdList = seekCurrent(object->filterStack->currentCmdPtr, object->filterOrder);
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
SuperclassAdd(Tcl_Interp *interp, XOTclClass *cl, int oc, Tcl_Obj **ov, Tcl_Obj *arg, XOTclClass *baseClass) {
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
    if (GetClassFromObj(interp, ov[i], &scl[i], baseClass) != TCL_OK) {
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
XOTcl_ObjSetVar2(XOTcl_Object *object, Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
                 Tcl_Obj *valueObj, int flgs) {
  Tcl_Obj *result;
  Tcl_CallFrame frame, *framePtr = &frame;

  XOTcl_PushFrameObj(interp, (XOTclObject*)object, framePtr);
  if (((XOTclObject*)object)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_ObjSetVar2(interp, name1, name2, valueObj, flgs);
  XOTcl_PopFrameObj(interp, framePtr);
  return result;
}

extern Tcl_Obj *
XOTcl_SetVar2Ex(XOTcl_Object *object, Tcl_Interp *interp, CONST char *name1, CONST char *name2,
                Tcl_Obj *valueObj, int flgs) {
  Tcl_Obj *result;
  Tcl_CallFrame frame, *framePtr = &frame;

  XOTcl_PushFrameObj(interp, (XOTclObject*)object, framePtr);
  if (((XOTclObject*)object)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_SetVar2Ex(interp, name1, name2, valueObj, flgs);
  XOTcl_PopFrameObj(interp, framePtr);
  return result;
}


Tcl_Obj *
XOTclOSetInstVar(XOTcl_Object *object, Tcl_Interp *interp,
		 Tcl_Obj *nameObj, Tcl_Obj *valueObj, int flgs) {
  return XOTcl_ObjSetVar2(object, interp, nameObj, (Tcl_Obj *)NULL, valueObj, (flgs|TCL_PARSE_PART1));
}

extern Tcl_Obj *
XOTcl_ObjGetVar2(XOTcl_Object *object, Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
                 int flgs) {
  Tcl_Obj *result;
  Tcl_CallFrame frame, *framePtr = &frame;

  XOTcl_PushFrameObj(interp, (XOTclObject*)object, framePtr);
  if (((XOTclObject*)object)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_ObjGetVar2(interp, name1, name2, flgs);
  XOTcl_PopFrameObj(interp, framePtr);

  return result;
}

extern Tcl_Obj *
XOTcl_GetVar2Ex(XOTcl_Object *object, Tcl_Interp *interp, CONST char *name1, CONST char *name2,
                int flgs) {
  Tcl_Obj *result;
  Tcl_CallFrame frame, *framePtr = &frame;

  XOTcl_PushFrameObj(interp, (XOTclObject*)object, framePtr);
  if (((XOTclObject*)object)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_GetVar2Ex(interp, name1, name2, flgs);
  XOTcl_PopFrameObj(interp, framePtr);
  return result;
}


Tcl_Obj *
XOTclOGetInstVar(XOTcl_Object *object, Tcl_Interp *interp, Tcl_Obj *nameObj, int flgs) {
  return XOTcl_ObjGetVar2(object, interp, nameObj, (Tcl_Obj *)NULL, (flgs|TCL_PARSE_PART1));
}

int
XOTclUnsetInstVar(XOTcl_Object *object, Tcl_Interp *interp, CONST char *name, int flgs) {
  return XOTclUnsetInstVar2(object, interp, name, NULL, flgs);
}

static int
CheckVarName(Tcl_Interp *interp, const char *varNameString) {
  /* 
   * Check, whether the provided name is save to be used in the
   * resolver.  We do not want to get interferences with namespace
   * resolver and such.  In an first attempt, we disallowed occurances
   * of "::", but we have to deal as well with e.g. arrayName(::x::y)
   * 
   * TODO: more general and efficient solution to disallow e.g. a::b
   * (check for :: until parens)
   */
  /*if (strstr(varNameString, "::") || *varNameString == ':') {*/
  if (*varNameString == ':') {
    return XOTclVarErrMsg(interp, "variable name \"", varNameString,
                          "\" must not contain namespace separator or colon prefix",
                          (char *) NULL);
  }
  return TCL_OK;
}

static int
varExists(Tcl_Interp *interp, XOTclObject *object, CONST char *varName, CONST char *index,
          int triggerTrace, int requireDefined) {
  Tcl_CallFrame frame, *framePtr = &frame;
  Var *varPtr, *arrayPtr;
  int result;
  int flags = 0;

  flags = (index == NULL) ? TCL_PARSE_PART1 : 0;

  XOTcl_PushFrameObj(interp, object, framePtr);

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

  XOTcl_PopFrameObj(interp, framePtr);

  return result;
}

static int
SubstValue(Tcl_Interp *interp, XOTclObject *object, Tcl_Obj **value) {
  Tcl_Obj *ov[2];
  int result;

  ov[1] = *value;
  Tcl_ResetResult(interp);

  result = XOTcl_SubstObjCmd(NULL, interp, 2, ov);

  /*fprintf(stderr, "+++++ %s.%s subst returned %d OK %d\n",
    objectName(object), varName, rc, TCL_OK);*/

  if (result == TCL_OK) {
    *value = Tcl_GetObjResult(interp);
  }
  return result;
}

#if defined(WITH_TCL_COMPILE)
# include <tclCompile.h>
#endif

static void
MakeProcError(
	      Tcl_Interp *interp,	/* The interpreter in which the procedure was called. */
	      Tcl_Obj *procNameObj)	/* Name of the procedure. Used for error
					 * messages and trace information. */
{
  int overflow, limit = 60, nameLen;
  const char *procName = Tcl_GetStringFromObj(procNameObj, &nameLen);

  overflow = (nameLen > limit);
  Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
						 "\n    (procedure \"%.*s%s\" line %d)",
						 (overflow ? limit : nameLen), procName,
						 (overflow ? "..." : ""), Tcl_GetErrorLine(interp)));
}

static int 
ByteCompiled(register Tcl_Interp *interp, Proc *procPtr, CONST char *body) {
  Tcl_Obj *bodyPtr = procPtr->bodyPtr;
  Namespace *nsPtr = procPtr->cmdPtr->nsPtr;

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
    return TCL_OK;
# endif
  } else {

# if defined(HAVE_TCL_COMPILE_H)
  doCompilation:
# endif
    return TclProcCompileProc(interp, procPtr, bodyPtr,
                              (Namespace *) nsPtr, "body of proc",
                              body);
  }
}

/*
   PushProcCallFrame() compiles conditionally a proc and pushes a
   callframe. Interesting fields:

   clientData: Record describing procedure to be interpreted.
   isLambda: 1 if this is a call by ApplyObjCmd: it needs special rules for error msg

 */

static int
PushProcCallFrame(ClientData clientData, register Tcl_Interp *interp, int objc,	Tcl_Obj *CONST objv[],
                  XOTclCallStackContent *cscPtr) {
  Proc *procPtr = (Proc *) clientData;
  CallFrame *framePtr;
  int result;

  /*
   * Set up and push a new call frame for the new procedure invocation.
   * This call frame will execute in the proc's namespace, which might be
   * different than the current namespace. The proc's namespace is that of
   * its command, which can change if the command is renamed from one
   * namespace to another.
   */

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "PUSH METHOD_FRAME (PushProcCallFrame) csc %p %s obj %s obj refcount %d\n", cscPtr,
          cscPtr ? Tcl_GetCommandName(interp, cscPtr->cmdPtr) : NULL,
          objectName(cscPtr->self),
          cscPtr && cscPtr->self->id ? Tcl_Command_refCount(cscPtr->self->id) : -100
          );
#endif
  
  /* TODO: we could use Tcl_PushCallFrame(), if we would allocate the tcl stack frame earlier */
  result = TclPushStackFrame(interp, (Tcl_CallFrame **)&framePtr,
			     (Tcl_Namespace *)  procPtr->cmdPtr->nsPtr,
			     (FRAME_IS_PROC|FRAME_IS_XOTCL_METHOD));
  if (result != TCL_OK) {
    return result;
  }

  framePtr->objc = objc;
  framePtr->objv = objv;
  framePtr->procPtr = procPtr;
#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "     put csc %p into frame %p flags %.4x\n", cscPtr, framePtr, framePtr->isProcCallFrame);
#endif
  framePtr->clientData = (ClientData)cscPtr;

  return ByteCompiled(interp, procPtr, TclGetString(objv[0]));
}

static void
getVarAndNameFromHash(Tcl_HashEntry *hPtr, Var **val, Tcl_Obj **varNameObj) {
  *val  = VarHashGetValue(hPtr);
  *varNameObj  = VarHashGetKey(*val);
}

static void ParamDefsFree(XOTclParamDefs *paramDefs);

void XOTclProcDeleteProc(ClientData clientData) {
  XOTclProcContext *ctxPtr = (XOTclProcContext *)clientData;
  (*ctxPtr->oldDeleteProc)(ctxPtr->oldDeleteData);
  if (ctxPtr->paramDefs) {
    /*fprintf(stderr, "free ParamDefs %p\n", ctxPtr->paramDefs);*/
    ParamDefsFree(ctxPtr->paramDefs);
  }
  /*fprintf(stderr, "free %p\n", ctxPtr);*/
  FREE(XOTclProcContext, ctxPtr);
}

static XOTclParam *ParamsNew(int nr) {
  XOTclParam *paramsPtr = NEW_ARRAY(XOTclParam, nr+1);
  memset(paramsPtr, 0, sizeof(XOTclParam)*(nr+1));
  return paramsPtr;
}

static void ParamsFree(XOTclParam *paramsPtr) {
  XOTclParam *paramPtr;
  
  /*fprintf(stderr, "ParamsFree %p\n", paramsPtr);*/
  for (paramPtr=paramsPtr; paramPtr->name; paramPtr++) {
    /*fprintf(stderr, ".... paramPtr = %p, name=%s, defaultValue %p\n", paramPtr, paramPtr->name, paramPtr->defaultValue);*/
    if (paramPtr->name) ckfree(paramPtr->name);
    if (paramPtr->nameObj) {DECR_REF_COUNT(paramPtr->nameObj);}
    if (paramPtr->defaultValue) {DECR_REF_COUNT(paramPtr->defaultValue);}
    if (paramPtr->converterName) {DECR_REF_COUNT(paramPtr->converterName);}
    if (paramPtr->converterArg) {DECR_REF_COUNT(paramPtr->converterArg);}
    if (paramPtr->paramObj) {DECR_REF_COUNT(paramPtr->paramObj);}
    if (paramPtr->slotObj) {DECR_REF_COUNT(paramPtr->slotObj);}
  }
  FREE(XOTclParam*, paramsPtr);
}

static XOTclParamDefs *
ParamDefsGet(Tcl_Command cmdPtr) {
  if (Tcl_Command_deleteProc(cmdPtr) == XOTclProcDeleteProc) {
    return ((XOTclProcContext *)Tcl_Command_deleteData(cmdPtr))->paramDefs;
  }
  return NULL;
}

static int
ParamDefsStore(Tcl_Interp *interp, Tcl_Command cmd, XOTclParamDefs *paramDefs) {
  Command *cmdPtr = (Command *)cmd;

  if (cmdPtr->deleteProc != XOTclProcDeleteProc) {
    XOTclProcContext *ctxPtr = NEW(XOTclProcContext);

    /*fprintf(stderr, "paramDefsStore replace deleteProc %p by %p\n",
      cmdPtr->deleteProc, XOTclProcDeleteProc);*/

    ctxPtr->oldDeleteData = (Proc *)cmdPtr->deleteData;
    ctxPtr->oldDeleteProc = cmdPtr->deleteProc;
    cmdPtr->deleteProc = XOTclProcDeleteProc;
    ctxPtr->paramDefs = paramDefs;
    cmdPtr->deleteData = (ClientData)ctxPtr;
    return TCL_OK;
  } else {
    /*fprintf(stderr, "paramDefsStore cmd %p has already XOTclProcDeleteProc deleteData %p\n", 
      cmd, cmdPtr->deleteData);*/
    if (cmdPtr->deleteData) {
      XOTclProcContext *ctxPtr = cmdPtr->deleteData;
      assert(ctxPtr->paramDefs == NULL);
      ctxPtr->paramDefs = paramDefs;
    }
  }
  return TCL_ERROR;
}

static XOTclParamDefs *
ParamDefsNew() {
  XOTclParamDefs *paramDefs;

  paramDefs = NEW(XOTclParamDefs);
  memset(paramDefs, 0, sizeof(XOTclParamDefs));
  /*fprintf(stderr, "ParamDefsNew %p\n", paramDefs);*/

  return paramDefs;
}


static void
ParamDefsFree(XOTclParamDefs *paramDefs) {
  /*fprintf(stderr, "ParamDefsFree %p returns %p\n", paramDefs, paramDefs->returns);*/

  if (paramDefs->paramsPtr) {
    ParamsFree(paramDefs->paramsPtr);
  }
  if (paramDefs->slotObj) {DECR_REF_COUNT(paramDefs->slotObj);}
  if (paramDefs->returns) {DECR_REF_COUNT(paramDefs->returns);}
  FREE(XOTclParamDefs, paramDefs);
}

/*
 * Non Positional Parameter
 */

static void
ParamDefsFormatOption(Tcl_Interp *interp, Tcl_Obj *nameStringObj, CONST char* option,
                      int *colonWritten, int *firstOption) {
  if (!*colonWritten) {
    Tcl_AppendLimitedToObj(nameStringObj, ":", 1, INT_MAX, NULL);
    *colonWritten = 1;
  }
  if (*firstOption) {
    *firstOption = 0;
  } else {
    Tcl_AppendLimitedToObj(nameStringObj, ",", 1, INT_MAX, NULL);
  }
  Tcl_AppendLimitedToObj(nameStringObj, option, -1, INT_MAX, NULL);
}

static int convertToNothing(Tcl_Interp *interp, Tcl_Obj *objPtr, struct XOTclParam CONST *pPtr, ClientData *clientData, Tcl_Obj **outObjPtr);

static Tcl_Obj *
ParamDefsFormat(Tcl_Interp *interp, XOTclParam CONST *paramsPtr) {
  int first, colonWritten;
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL), *innerListObj, *nameStringObj;
  XOTclParam CONST *pPtr;

  for (pPtr = paramsPtr; pPtr->name; pPtr++) {
    if (pPtr -> paramObj) {
      innerListObj = pPtr->paramObj;
    } else {
      /* We need this part only for C-defined parameter definitions,
         defined via genTclAPI.

         TODO: we could streamline this by defining as well C-API via
         the same syntax as for accepted for tcl obj types
         "xotclParam"
      */
      int isNonpos = *pPtr->name == '-';
      int outputRequired = (isNonpos && (pPtr->flags & XOTCL_ARG_REQUIRED));
      int outputOptional = (!isNonpos && !(pPtr->flags & XOTCL_ARG_REQUIRED)
                            && !pPtr->defaultValue &&
                            pPtr->converter != convertToNothing);
      first = 1;
      colonWritten = 0;
      
      nameStringObj = Tcl_NewStringObj(pPtr->name, -1);
      if (pPtr->type) {
        ParamDefsFormatOption(interp, nameStringObj, pPtr->type, &colonWritten, &first);
      }
      if (outputRequired) {
        ParamDefsFormatOption(interp, nameStringObj, "required", &colonWritten, &first);
      } else if (outputOptional) {
        ParamDefsFormatOption(interp, nameStringObj, "optional", &colonWritten, &first);
      }
      if ((pPtr->flags & XOTCL_ARG_SUBST_DEFAULT)) {
        ParamDefsFormatOption(interp, nameStringObj, "substdefault", &colonWritten, &first);
      }
      if ((pPtr->flags & XOTCL_ARG_ALLOW_EMPTY)) {
        ParamDefsFormatOption(interp, nameStringObj, "allowempty", &colonWritten, &first);
      }
      if ((pPtr->flags & XOTCL_ARG_INITCMD)) {
        ParamDefsFormatOption(interp, nameStringObj, "initcmd", &colonWritten, &first);
      } else if ((pPtr->flags & XOTCL_ARG_METHOD)) {
        ParamDefsFormatOption(interp, nameStringObj, "method", &colonWritten, &first);
      } else if ((pPtr->flags & XOTCL_ARG_NOARG)) {
        ParamDefsFormatOption(interp, nameStringObj, "noarg", &colonWritten, &first);
      } else if ((pPtr->flags & XOTCL_ARG_MULTIVALUED)) {
        ParamDefsFormatOption(interp, nameStringObj, "multivalued", &colonWritten, &first);
      }
      
      innerListObj = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, innerListObj, nameStringObj);
      if (pPtr->defaultValue) {
        Tcl_ListObjAppendElement(interp, innerListObj, pPtr->defaultValue);
      }
    }
    
    Tcl_ListObjAppendElement(interp, listObj, innerListObj);
  }

  return listObj;
}

static Tcl_Obj *
ParamDefsList(Tcl_Interp *interp, XOTclParam CONST *paramsPtr) {
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
  XOTclParam CONST *pPtr;

  for (pPtr = paramsPtr; pPtr->name; pPtr++) {
    Tcl_ListObjAppendElement(interp, listObj, pPtr->nameObj);
  }
  return listObj;
}

static Tcl_Obj*
ParamDefsSyntax(Tcl_Interp *interp, XOTclParam CONST *paramPtr) {
  Tcl_Obj *argStringObj = Tcl_NewStringObj("", 0);
  XOTclParam CONST *pPtr;

  for (pPtr = paramPtr; pPtr->name; pPtr++) {
    if (pPtr != paramPtr) {
      Tcl_AppendLimitedToObj(argStringObj, " ", 1, INT_MAX, NULL);
    }
    if (pPtr->flags & XOTCL_ARG_REQUIRED) {
      Tcl_AppendLimitedToObj(argStringObj, pPtr->name, -1, INT_MAX, NULL);
    } else {
      Tcl_AppendLimitedToObj(argStringObj, "?", 1, INT_MAX, NULL);
      Tcl_AppendLimitedToObj(argStringObj, pPtr->name, -1, INT_MAX, NULL);
      if (pPtr->nrArgs >0) {
        Tcl_AppendLimitedToObj(argStringObj, " arg", 4, INT_MAX, NULL);
      }
      Tcl_AppendLimitedToObj(argStringObj, "?", 1, INT_MAX, NULL);
    }
  }
  /* caller has to decr */
  return argStringObj;
}

static void ParsedParamFree(XOTclParsedParam *parsedParamPtr) {
  /*fprintf(stderr, "ParsedParamFree %p, npargs %p\n", parsedParamPtr, parsedParamPtr->paramDefs);*/
  if (parsedParamPtr->paramDefs) {
    ParamDefsFree(parsedParamPtr->paramDefs);
  }
  FREE(XOTclParsedParam, parsedParamPtr);
}


/*
 * method dispatch
 */
#if defined(NRE)
static int
FinalizeProcMethod(ClientData data[], Tcl_Interp *interp, int result) {
  parseContext *pcPtr = data[0];
  XOTclCallStackContent *cscPtr = data[1];
  CONST char *methodName = data[2];
  XOTclObject *object = cscPtr->self;
  XOTclObjectOpt *opt = object->opt;
  int rc;

  /*fprintf(stderr, "---- FinalizeProcMethod result %d, csc %p, pcPtr %p, obj %p\n",
    result, cscPtr, pcPtr, object);*/
# if defined(TCL85STACK_TRACE)
  fprintf(stderr, "POP  FRAME (implicit)  csc %p obj %s obj refcount %d %d\n",
          cscPtr, objectName(object),
          obj->id ? Tcl_Command_refCount(object->id) : -100,
          obj->refCount
          );
# endif

  { XOTclParamDefs *paramDefs = ParamDefsGet(cscPtr->cmdPtr);

    if (result == TCL_OK && paramDefs && paramDefs->returns) {
      Tcl_Obj *valueObj = Tcl_GetObjResult(interp);
      /*fprintf(stderr, "***** we have returns for method '%s' check %s, value %p\n", 
	methodName, ObjStr(paramDefs->returns), valueObj);*/
      result = Parametercheck(interp, paramDefs->returns, valueObj, "return-value:", NULL);
    }
  }

  if (opt && object->teardown && (opt->checkoptions & CHECK_POST)) {
    /* even, when the passed result != TCL_OK, run assertion to report
     * the highest possible method from the callstack (e.g. "set" would not
     * be very meaningful; however, do not flush a TCL_ERROR.
     */
    rc = AssertionCheck(interp, object, cscPtr->cl, methodName, CHECK_POST);
    if (result == TCL_OK) {
      result = rc;
    }
  }

  if (pcPtr) {
# if defined(TCL_STACK_ALLOC_TRACE)
    fprintf(stderr, "---- FinalizeProcMethod calls releasePc, stackFree %p\n", pcPtr);
# endif
    parseContextRelease(pcPtr);
    TclStackFree(interp, pcPtr);
  }

# if defined(TCL_STACK_ALLOC_TRACE)
  fprintf(stderr, "---- FinalizeProcMethod calls pop, csc free %p method %s\n", cscPtr, methodName);
# endif
  CscFinish(interp, cscPtr);
  TclStackFree(interp, cscPtr);

  return result;
}
#endif

/* invoke a scripted method (with assertion checking) */
static int
ProcMethodDispatch(ClientData cp, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
         CONST char *methodName, XOTclObject *object, XOTclClass *cl, Tcl_Command cmdPtr,
         XOTclCallStackContent *cscPtr) {
  int result, releasePc = 0;
  XOTclObjectOpt *opt = object->opt;
  XOTclParamDefs *paramDefs;
#if defined(NRE)
  parseContext *pcPtr = NULL;
#else
  parseContext pc, *pcPtr = &pc;
#endif

  assert(object);
  assert(object->teardown);

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "+++ ProcMethodDispatch %s, cscPtr %p, frametype %d, teardown %p\n",
          methodName, cscPtr, cscPtr->frameType, object->teardown);
#endif

  /*
   * if this is a filter, check whether its guard applies,
   * if not: just step forward to the next filter
   */

  if (cscPtr->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
    XOTclCmdList *cmdList;
    /*
     * seek cmd in obj's filterOrder
     */
    assert(object->flags & XOTCL_FILTER_ORDER_VALID);
    /* otherwise: FilterComputeDefined(interp, object);*/

    for (cmdList = object->filterOrder; cmdList && cmdList->cmdPtr != cmdPtr; cmdList = cmdList->nextPtr);

    if (cmdList) {
      /*
       * A filter was found, check whether it has a guard.
       */
      result = GuardCall(object, cl, (Tcl_Command) cmdList->cmdPtr, interp,
                         cmdList->clientData, cscPtr);

      if (result != TCL_OK) {
        /*fprintf(stderr, "Filter GuardCall in invokeProc returned %d\n", result);*/

        if (result != TCL_ERROR) {
          /*
           * The guard failed (but no error); call "next", use the
           * actual objv's, not the callstack objv, since we may not
           * be in a method resulting in invalid callstackobjs.
           *
           * The call stack content is not jet pushed to the Tcl
           * stack, so we pass it here explicitly.
           */

          /*fprintf(stderr, "... calling nextmethod cscPtr %p\n", cscPtr);*/
          result = XOTclNextMethod(object, interp, cl, methodName,
                               objc, objv, /*useCallStackObjs*/ 0, cscPtr);
          /*fprintf(stderr, "... after nextmethod result %d\n", result);*/
        }
#if defined(NRE)
# if defined(TCL_STACK_ALLOC_TRACE)
        fprintf(stderr, "---- GuardFailed calls pop, cscPtr free %p method %s\n", cscPtr, methodName);
# endif
        CscFinish(interp, cscPtr);
        TclStackFree(interp, cscPtr);
        /* todo check mixin guards for same case? */
#endif
        return result;
      }
    }
  }

  if (opt && (opt->checkoptions & CHECK_PRE) &&
      (result = AssertionCheck(interp, object, cl, methodName, CHECK_PRE)) == TCL_ERROR) {
    goto finish;
  }

#ifdef DISPATCH_TRACE
  printCall(interp, "ProcMethodDispatch", objc, objv);
  fprintf(stderr, "\tproc=%s\n", Tcl_GetCommandName(interp, cmdPtr));
#endif

  /*
     If the method to be invoked has paramDefs, we have to call the
     argument parser with the argument definitions obtained from the
     proc context from the cmdPtr.
  */
  paramDefs = ParamDefsGet(cmdPtr);

  /*Tcl_Command_deleteProc(cmdPtr) == XOTclProcDeleteProc ?
    ((XOTclProcContext *)Tcl_Command_deleteData(cmdPtr))->paramDefs : NULL;*/
  
  if (paramDefs && paramDefs->paramsPtr) {
#if defined(NRE)
    pcPtr = (parseContext *) TclStackAlloc(interp, sizeof(parseContext));
# if defined(TCL_STACK_ALLOC_TRACE)
    fprintf(stderr, "---- parseContext alloc %p\n", pcPtr);
# endif
#endif
    result = ProcessMethodArguments(pcPtr, interp, object, 1, paramDefs, methodName, objc, objv);
    cscPtr->objc = objc;
    cscPtr->objv = (Tcl_Obj **)objv;
    if (result == TCL_OK) {
      releasePc = 1;
      result = PushProcCallFrame(cp, interp, pcPtr->objc, pcPtr->full_objv, cscPtr);
    }
  } else {
    result = PushProcCallFrame(cp, interp, objc, objv, cscPtr);
  }

  /* we could consider to run here ARG_METHOD or ARG_INITCMD
  if (result == TCL_OK) {

  } */

  if (result != TCL_OK) {
#if defined(NRE)
    if (pcPtr) TclStackFree(interp, pcPtr);
# if defined(TCL_STACK_ALLOC_TRACE)
    fprintf(stderr, "---- ProcPrep fails and calls pop, cscPtr free %p method %s\n", cscPtr, methodName);
# endif
    CscFinish(interp, cscPtr);
    TclStackFree(interp, cscPtr);
#endif
  }

  /*
   * The stack frame is pushed, we could do something here before
   * running the byte code of the body.
   */
  if (result == TCL_OK) {
#if !defined(NRE)
    result = TclObjInterpProcCore(interp, objv[0], 1, &MakeProcError);
    if (releasePc) {
      parseContextRelease(&pc);
    }
#else
    {
      TEOV_callback *rootPtr = TOP_CB(interp);
      /*fprintf(stderr, "CALL TclNRInterpProcCore %s method '%s'\n", objectName(object), ObjStr(objv[0]));*/
      Tcl_NRAddCallback(interp, FinalizeProcMethod,
                        releasePc ? pcPtr : NULL, cscPtr, methodName, NULL);
      result = TclNRInterpProcCore(interp, objv[0], 1, &MakeProcError);
      /*fprintf(stderr, ".... run callbacks rootPtr = %p, result %d methodName %s\n", rootPtr, result, methodName);*/
      result = TclNRRunCallbacks(interp, result, rootPtr, 0);
      /*fprintf(stderr, ".... run callbacks DONE result %d methodName %s\n", result, methodName);*/
    }
#endif
  }
# if defined(TCL85STACK_TRACE)
  fprintf(stderr, "POP  OBJECT_FRAME (implicit) frame %p cscPtr %p obj %s obj refcount %d %d\n", NULL, cscPtr,
          objectName(object),
          object->id ? Tcl_Command_refCount(object->id) : -100,
          object->refCount
          );
# endif

#if defined(PRE86)
# ifdef DISPATCH_TRACE
  printExit(interp, "ProcMethodDispatch", objc, objv, result);
  /* fprintf(stderr, " returnCode %d xotcl rc %d\n",
     Tcl_Interp_returnCode(interp), result);*/
# endif

  if (result == TCL_OK && paramDefs && paramDefs->returns) {
    Tcl_Obj *valueObj = Tcl_GetObjResult(interp);
    /*fprintf(stderr, "***** we have returns for method '%s' check %s, value %p is shared %d\n", 
      methodName, ObjStr(paramDefs->returns), valueObj, Tcl_IsShared(valueObj));*/
    result = Parametercheck(interp, paramDefs->returns, valueObj, "return-value:", NULL);
  }

  opt = object->opt;
  if (opt && object->teardown &&
      (opt->checkoptions & CHECK_POST)) {
    int rc = AssertionCheck(interp, object, cscPtr->cl, methodName, CHECK_POST);
    /* don't clobber error codes */
    if (result == TCL_OK) {
      result = rc;
    }
  }
#endif
 finish:
  return result;
}

/* Invoke a method implemented as a cmd (with assertion checking) */
static int
CmdMethodDispatch(ClientData cp, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
        CONST char *methodName, XOTclObject *object, Tcl_Command cmdPtr,
        XOTclCallStackContent *cscPtr) {
  CheckOptions co;
  int result;
  Tcl_CallFrame frame, *framePtr = &frame;

  assert(object);
  assert(object->teardown);

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "+++ CmdMethodDispatchCheck %s, obj %p %s, cscPtr %p, teardown %p\n",
          methodName, object, objectName(object), cscPtr, object->teardown);
#endif

  /* fprintf(stderr, ".. calling cmd %s cscPtr %p\n", methodName, cscPtr);*/

  if (object->opt) {
    co = object->opt->checkoptions;
    if ((co & CHECK_INVAR) &&
        ((result = AssertionCheckInvars(interp, object, methodName, co)) == TCL_ERROR)) {
      goto finish;
    }
  }

  if (cscPtr) {
    /* We have a call stack content, but the following dispatch will
     * by itself not stack it; in order to get e.g. self working, we
     * have to stack at least an FRAME_IS_XOTCL_OBJECT.
     * TODO: maybe push should happen already before assertion checking,
     * but we have to check what happens in the finish target etc.
     */
    /*fprintf(stderr, "XOTcl_PushFrameCsc %s %s\n",objectName(object), methodName);*/
    XOTcl_PushFrameCsc(interp, cscPtr, framePtr);
  }

#ifdef DISPATCH_TRACE
  printCall(interp, "CmdMethodDispatch cmd", objc, objv);
  fprintf(stderr, "\tcmd=%s\n", Tcl_GetCommandName(interp, cmdPtr));
#endif

  /*fprintf(stderr, "CmdDispatch obj %p %p %s\n", obj, methodName, methodName);*/
  result = Tcl_NRCallObjProc(interp, Tcl_Command_objProc(cmdPtr), cp, objc, objv);

#ifdef DISPATCH_TRACE
  printExit(interp, "CmdMethodDispatch cmd", objc, objv, result);
#endif

  if (cscPtr) {
    XOTcl_PopFrameCsc(interp, framePtr);
  }

  /* Reference counting in the calling ObjectDispatch() makes sure
     that obj->opt is still accessible even after "dealloc" */
  if (object->opt) {
    co = object->opt->checkoptions;
    if ((co & CHECK_INVAR) &&
        ((result = AssertionCheckInvars(interp, object, methodName, co)) == TCL_ERROR)) {
      goto finish;
    }
  }

  { XOTclParamDefs *paramDefs = ParamDefsGet(cmdPtr);

    if (result == TCL_OK && paramDefs && paramDefs->returns) {
      Tcl_Obj *valueObj = Tcl_GetObjResult(interp);
      /* fprintf(stderr, "***** CMD we have returns for method '%s' check %s, value %p\n", 
	 methodName, ObjStr(paramDefs->returns), valueObj);*/
      result = Parametercheck(interp, paramDefs->returns, valueObj, "return-value:", NULL);
    }
  }

 finish:
  return result;
}

#if defined(PROFILE)
static int
MethodDispatch(ClientData clientData, Tcl_Interp *interp,
             int objc, Tcl_Obj *CONST objv[], Tcl_Command cmd, XOTclObject *object, XOTclClass *cl,
             CONST char *methodName, int frameType) {
  struct timeval trt;
  long int startUsec = (gettimeofday(&trt, NULL), trt.tv_usec), startSec = trt.tv_sec;

  result = __MethodDispatch__(clientData, interp, objc, objv, cmd, object, cl, methodName, frameType);
  XOTclProfileEvaluateData(interp, startSec, startUsec, object, cl, methodName);
  return result;
}
# define MethodDispatch __MethodDispatch__
#endif

static Tcl_Obj*
SubcmdObj(Tcl_Interp *interp, CONST char *start, size_t len) {
  Tcl_Obj *checker = Tcl_NewStringObj("sub=", 4);
  Tcl_AppendLimitedToObj(checker, start, len, INT_MAX, NULL);
  return checker;
}

/*
 * MethodDispatch() calls an XOTcl method. It calls either a
 * Tcl-implemented method (via ProcMethodDispatch()) or a C-implemented
 * method (via CmdMethodDispatch()) and sets up stack and client data
 * accordingly.
 */

static int
MethodDispatch(ClientData clientData, Tcl_Interp *interp,
             int objc, Tcl_Obj *CONST objv[],
             Tcl_Command cmd, XOTclObject *object, XOTclClass *cl,
             CONST char *methodName, int frameType) {
  ClientData cp = Tcl_Command_objClientData(cmd);
  XOTclCallStackContent csc, *cscPtr;
  register Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);

  int result;

  assert (object->teardown);
  /*fprintf(stderr, "MethodDispatch method '%s' cmd %p cp=%p objc=%d\n", methodName, cmd, cp, objc);*/

  if (proc == TclObjInterpProc) {
    /* 
       The cmd is a scripted method 
    */
#if defined(NRE)
    cscPtr = (XOTclCallStackContent *) TclStackAlloc(interp, sizeof(XOTclCallStackContent));
# if defined(TCL_STACK_ALLOC_TRACE)
    fprintf(stderr, "---- csc alloc %p method %s\n", cscPtr, methodName);
# endif
#else
    cscPtr = &csc;
#endif
    CscInit(cscPtr, object, cl, cmd, frameType);
    result = ProcMethodDispatch(cp, interp, objc, objv, methodName, object, cl, cmd, cscPtr);
#if defined(NRE)
    /* CscFinish() is performed by the callbacks or in error case base ProcMethodDispatch */
    /*fprintf(stderr, "no pop for %s\n", methodName);*/
#else
    CscFinish(interp, cscPtr);
#endif
    return result;

  } else if (cp || Tcl_Command_flags(cmd) & XOTCL_CMD_NONLEAF_METHOD) {
    /* 
       The cmd has client data or is an aliased method
    */
    cscPtr = &csc;

    /*fprintf(stderr, "we could stuff obj %p %s\n", object, objectName(object));*/

    if (proc == XOTclObjDispatch) {
      /*
       * invoke an aliased object via method interface
       */
      XOTclRuntimeState *rst = RUNTIME_STATE(interp);
      XOTclObject *invokeObj = (XOTclObject *)cp;

      if (invokeObj->flags & XOTCL_DELETED) {
        /*
         * When we try to call a deleted object, the cmd (alias) is
         * automatically removed.
         */
        Tcl_DeleteCommandFromToken(interp, cmd);
        XOTclCleanupObject(invokeObj);
        return XOTclVarErrMsg(interp, "Trying to dispatch deleted object via method '",
                              methodName, "'", (char *) NULL);
      }

      /* 
       * The client data cp is still the obj of the called method,
       * i.e. self changes. In order to prevent this, we save the
       * actual object in the runtime state, flag ObjectDispatch via
       * XOTCL_CM_DELGATE to use it.
       */
      /*xxxx*/
      /*fprintf(stderr, "save self %p %s\n", object, objectName(object));*/ 
      rst->delegatee = object;
      if (objc < 2) {
	result = DispatchDefaultMethod(cp, interp, objc, objv);
      } else {
#if 0
        ALLOC_ON_STACK(Tcl_Obj*, objc, tov);
	memcpy(tov, objv, sizeof(Tcl_Obj *)*(objc));
	tov[1] = SubcmdObj(interp, ObjStr(objv[1]), -1);
	INCR_REF_COUNT(tov[1]);
	result = ObjectDispatch(cp, interp, objc, tov, XOTCL_CM_DELGATE);
	DECR_REF_COUNT(tov[1]);
#else
	XOTclObject *self = (XOTclObject *)cp;
	char *methodName;
	if (self->nsPtr) {
	  methodName = ObjStr(objv[1]);
	  cmd = FindMethod(self->nsPtr, methodName);
	  if (cmd) {
	    result = MethodDispatch(object, interp, objc-1, objv+1, 
				    cmd, object, NULL, methodName, frameType);
	    goto obj_dispatch_ok;
	  }
	}
	result = XOTclVarErrMsg(interp, objectName(self),
                                ": unable to dispatch method '",
                                methodName, "'", (char *) NULL);
      obj_dispatch_ok:;
	/*result = ObjectDispatch(cp, interp, objc, objv, XOTCL_CM_DELGATE);*/
#endif
      }
      return result;
    } else if (proc == XOTclForwardMethod ||
	       proc == XOTclObjscopedMethod ||
	       proc == XOTclSetterMethod
               ) {
      TclCmdClientData *tcd = (TclCmdClientData *)cp;
      tcd->object = object;
      assert((CmdIsProc(cmd) == 0));
    } else if (cp == (ClientData)XOTCL_CMD_NONLEAF_METHOD) {
      cp = clientData;
      assert((CmdIsProc(cmd) == 0));
    }
    CscInit(cscPtr, object, cl, cmd, frameType);

  } else {
    /* 
       The cmd has no client data
    */
    /*fprintf(stderr, "cmdMethodDispatch %s %s, nothing stacked\n",objectName(object), methodName);*/

    return CmdMethodDispatch(clientData, interp, objc, objv, methodName, object, cmd, NULL);
  }
  
  result = CmdMethodDispatch(cp, interp, objc, objv, methodName, object, cmd, cscPtr);
  /* make sure, that csc is still in the scope; therefore, csc is
     currently on the top scope of this function */
  CscFinish(interp, cscPtr);

  return result;
}

XOTCLINLINE static int
ObjectDispatch(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *CONST objv[], int flags) {
  register XOTclObject *object = (XOTclObject*)clientData;
  int result = TCL_OK, mixinStackPushed = 0,
    filterStackPushed = 0, unknown = 0, objflags, shift,
    frameType = XOTCL_CSC_TYPE_PLAIN;
  CONST char *methodName;
  XOTclClass *cl = NULL;
  Tcl_Command cmd = NULL;
  XOTclRuntimeState *rst = RUNTIME_STATE(interp);
  Tcl_Obj *cmdName = object->cmdName, *methodObj, *cmdObj;

  assert(objc>0);

  if (flags & XOTCL_CM_NO_SHIFT) {
    shift = 0;
    cmdObj = object->cmdName;
    methodObj = objv[0];
  } else {
    assert(objc>1);
    shift = 1;
    cmdObj = objv[0];
    methodObj = objv[1];
  }

  methodName = ObjStr(methodObj);
  if (FOR_COLON_RESOLVER(methodName)) {
    methodName ++;
  }

  /*fprintf(stderr, "ObjectDispatch obj = %s objc = %d 0=%s methodName=%s\n",
    objectName(object), objc, ObjStr(cmdObj), methodName);*/

#ifdef DISPATCH_TRACE
  printCall(interp, "DISPATCH", objc, objv);
#endif

  objflags = object->flags; /* avoid stalling */

  /* make sure, cmdName and obj survive this method until the end */
  INCR_REF_COUNT(cmdName);
  object->refCount ++; 

  if (!(objflags & XOTCL_FILTER_ORDER_VALID)) {
    FilterComputeDefined(interp, object);
    objflags = object->flags;
  }

  if (!(objflags & XOTCL_MIXIN_ORDER_VALID)) {
    MixinComputeDefined(interp, object);
    objflags = object->flags;
  }

  /* Only start new filter chain, if
     (a) filters are defined and
     (b) the toplevel csc entry is not an filter on self
  */

  /*fprintf(stderr, "call %s, objflags %.6x, defined and valid %.6x doFilters %d guard count %d\n",
          methodName, objflags, XOTCL_FILTER_ORDER_DEFINED_AND_VALID,
          rst->doFilters, rst->guardCount);*/

  if (((objflags & XOTCL_FILTER_ORDER_DEFINED_AND_VALID) == XOTCL_FILTER_ORDER_DEFINED_AND_VALID)
      && rst->doFilters
      && !rst->guardCount) {
    XOTclCallStackContent *cscPtr = CallStackGetTopFrame(interp, NULL);

    /*fprintf(stderr, "... check ok, cscPtr = %p\n", cscPtr);
    if (!cscPtr) {
      tcl85showStack(interp);
      }*/
    if (!cscPtr || (object != cscPtr->self ||
                    cscPtr->frameType != XOTCL_CSC_TYPE_ACTIVE_FILTER)) {
      filterStackPushed = FilterStackPush(interp, object, methodObj);
      cmd = FilterSearchProc(interp, object, &object->filterStack->currentCmdPtr, &cl);
      if (cmd) {
        /*fprintf(stderr, "filterSearchProc returned cmd %p\n", cmd);*/
        frameType = XOTCL_CSC_TYPE_ACTIVE_FILTER;
        methodName = (char *)Tcl_GetCommandName(interp, cmd);
      } else {
        /*fprintf(stderr, "filterSearchProc returned no cmd\n");*/
        FilterStackPop(object);
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

    mixinStackPushed = MixinStackPush(object);

    if (frameType != XOTCL_CSC_TYPE_ACTIVE_FILTER) {
      result = MixinSearchProc(interp, object, methodName, &cl,
                               &object->mixinStack->currentCmdPtr, &cmd);
      if (result != TCL_OK) {
        goto exit_dispatch;
      }
      if (cmd) {
        frameType = XOTCL_CSC_TYPE_ACTIVE_MIXIN;
      } else { /* the else branch could be deleted */
        MixinStackPop(object);
        mixinStackPushed = 0;
      }
    }
  }

  /* check if an absolute method name was provided */
  if (*methodName == ':') {
    cmd = Tcl_GetCommandFromObj(interp, methodObj);
    if (cmd) {
      CONST char *mn = Tcl_GetCommandName(interp, cmd);
      if (isClassName(methodName)) {
	CONST char *className = NSCutXOTclClasses(methodName);
	Tcl_DString ds, *dsPtr = &ds;
	DSTRING_INIT(dsPtr);
	Tcl_DStringAppend(dsPtr, className, strlen(className)-strlen(mn)-2);
	cl = (XOTclClass *)XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
	DSTRING_FREE(dsPtr);
      }
    }
  }

  /* if no filter/mixin is found => do ordinary method lookup */
  if (cmd == NULL) {

    /* do we have a object-specific proc? */
    if (object->nsPtr) {
      cmd = FindMethod(object->nsPtr, methodName);
      /* fprintf(stderr, "lookup for proc in obj %p method %s nsPtr %p => %p\n",
         object, methodName, object->nsPtr, cmd);*/
    }
    /*fprintf(stderr, "findMethod for proc '%s' in %p returned %p\n", methodName, object->nsPtr, cmd);*/

    if (cmd == NULL) {
      /* check for a method */
      XOTclClass *currentClass = object->cl;
      if (currentClass->order == NULL) currentClass->order = TopoOrder(currentClass, Super);
      cl = SearchPLMethod(currentClass->order, methodName, &cmd);
    }
  }

  if (cmd) {
    result = TCL_OK;

    /*fprintf(stderr, "cmd %p %s flags %x\n", cmd, methodName,
      ((Command *) cmd)->flags && 0x00010000);*/

    /* check, whether we have a protected method, and whether the
       protected method, called on a different object. In this case,
       we call as well the unknown method */

    if ((Tcl_Command_flags(cmd) & XOTCL_CMD_PROTECTED_METHOD) &&
	(flags & (XOTCL_CM_NO_UNKNOWN|XOTCL_CM_NO_PROTECT)) == 0) {
      XOTclObject *o, *lastSelf = GetSelfObj(interp);

      /* we do not want to rely on clientData, so get obj from cmdObj */
      GetObjectFromObj(interp, cmdObj, &o);
      if (o != lastSelf) {
	/*fprintf(stderr, "+++ protected method %s is not invoked\n", methodName);*/
        /* allow unknown-handler to handle this case */
	unknown = 1;
        fprintf(stderr, "+++ %s is protected, therefore maybe unknown %p %s lastself=%p o=%p cd %p flags = %.6x\n",
                methodName, cmdObj, ObjStr(cmdObj), lastSelf, o, clientData, flags);
        /*tcl85showStack(interp);*/
      }
    }

    if (!unknown) {
      /* xxxx */
      /*fprintf(stderr, "ObjectDispatch calls MethodDispatch with obj = %s frameType %d method %s flags %.6x\n",
	objectName(object), frameType, methodName, flags);*/
      if (flags & XOTCL_CM_DELGATE && rst->delegatee) {
	/*
	 * We want to execute the method on the delegatee, so we have
	 * to flip the object.
	 *
	 * Note: there is a object->refCount ++; at the begin of this
	 * function and a XOTclCleanupObject(object) at the end. So,
	 * we have to keep track of the refcounts here. Either mangle
	 * refcounts, or save originator.
	 * 
	 */
	result = MethodDispatch(rst->delegatee, interp, objc-shift, objv+shift, 
				cmd, rst->delegatee, cl,
				methodName, frameType);
      } else {
	result = MethodDispatch(clientData, interp, objc-shift, objv+shift, cmd, object, cl,
				methodName, frameType);
      }
      if (result == TCL_ERROR) {
        /*fprintf(stderr, "Call ErrInProc cl = %p, cmd %p, flags %.6x\n",
          cl, cl ? cl->object.id : 0, cl ? cl->object.flags : 0);*/
	result = XOTclErrInProc(interp, cmdName,
				cl && cl->object.teardown ? cl->object.cmdName : NULL,
				methodName);
      }

      unknown = rst->unknown;
    }
  } else {
    unknown = 1;
  }

  /* fprintf(stderr, "cmd %p unknown %d result %d\n", cmd, unknown, result);*/

  if (result == TCL_OK) {
    /*fprintf(stderr, "after doCallProcCheck unknown == %d\n", unknown);*/
    if (unknown) {
      Tcl_Obj *unknownObj = XOTclMethodObj(interp, object, XO_o_unknown_idx);

      if (unknownObj == NULL || (flags & XOTCL_CM_NO_UNKNOWN)) {
	result = XOTclVarErrMsg(interp, objectName(object),
                                ": unable to dispatch method '",
                                methodName, "'", (char *) NULL);
        goto exit_dispatch;
      } else if (methodObj != unknownObj) {
	/*
	 * back off and try unknown;
	 */
        XOTclObject *object = (XOTclObject*)clientData;
        ALLOC_ON_STACK(Tcl_Obj*, objc+2, tov);

	/*fprintf(stderr, "calling unknown for %s %s, flgs=%02x,%02x isClass=%d %p %s objc %d shift %d\n",
		objectName(object), methodName, flags, XOTCL_CM_NO_UNKNOWN,
		XOTclObjectIsClass(object), object, objectName(object), objc, shift);*/

        tov[0] = object->cmdName;
        tov[1] = unknownObj;
        if (objc-shift>0) {
          memcpy(tov+2, objv+shift, sizeof(Tcl_Obj *)*(objc-shift));
	}
        /*
          fprintf(stderr, "?? %s unknown %s\n", objectName(object), ObjStr(tov[2]));
        */
	flags &= ~XOTCL_CM_NO_SHIFT;
        result = ObjectDispatch(clientData, interp, objc+2-shift, tov, flags | XOTCL_CM_NO_UNKNOWN);
        FREE_ON_STACK(Tcl_Obj*, tov);
	
      } else { /* unknown failed */
        result = XOTclVarErrMsg(interp, objectName(object),
                                ": unable to dispatch method '",
                                ObjStr(objv[shift+1]), "'", (char *) NULL);
        goto exit_dispatch; 
      }

    }
  }
  /* be sure to reset unknown flag */
  if (unknown)
    rst->unknown = 0;

 exit_dispatch:
#ifdef DISPATCH_TRACE
  printExit(interp, "DISPATCH", objc, objv, result);
#endif

  if (mixinStackPushed && object->mixinStack)
    MixinStackPop(object);
    
  if (filterStackPushed && object->filterStack)
    FilterStackPop(object);

  XOTclCleanupObject(object);
  /*fprintf(stderr, "ObjectDispatch call XOTclCleanupObject %p DONE\n", object);*/
  DECR_REF_COUNT(cmdName); /* must be after last dereferencing of obj */
  return result;
}


static int
DispatchDefaultMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  int result;
  Tcl_Obj *methodObj = XOTclMethodObj(interp, (XOTclObject *)clientData, XO_o_defaultmethod_idx);

  if (methodObj) {
    Tcl_Obj *tov[2];
    tov[0] = objv[0];
    tov[1] = methodObj;
    result = ObjectDispatch(clientData, interp, 2, tov, XOTCL_CM_NO_UNKNOWN);
  } else {
    result = TCL_OK;
  }
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
    result = ObjectDispatch(clientData, interp, objc, objv, 0);
  } else {
    result = DispatchDefaultMethod(clientData, interp, objc, objv);
  }
  return result;
}

/*
 *  Proc-Creation
 */

static Tcl_Obj *addPrefixToBody(Tcl_Obj *body, int paramDefs, XOTclParsedParam *paramPtr) {
  Tcl_Obj *resultBody = Tcl_NewStringObj("", 0);

  INCR_REF_COUNT(resultBody);

  if (paramDefs && paramPtr->possibleUnknowns > 0)
    Tcl_AppendStringsToObj(resultBody, "::nsf::unsetUnknownArgs\n", (char *) NULL);

  Tcl_AppendStringsToObj(resultBody, ObjStr(body), (char *) NULL);
  return resultBody;
}

#define NEW_STRING(target, p, l)  target = ckalloc(l+1); strncpy(target, p, l); *((target)+l) = '\0'

XOTCLINLINE static int
noMetaChars(CONST char *pattern) {
  register char c;
  CONST char *p = pattern;

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
/* we could define parameterTypes with a converter, setter, canCheck, name */
static int convertToString(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, 
			   ClientData *clientData, Tcl_Obj **outObjPtr) {
  *clientData = (char *)ObjStr(objPtr);
  *outObjPtr = objPtr;
  return TCL_OK;
}

enum stringTypeIdx {StringTypeAlnum, StringTypeAlpha, StringTypeAscii, StringTypeBoolean, StringTypeControl, 
		    StringTypeDigit, StringTypeDouble, StringTypeFalse,StringTypeGraph, StringTypeInteger, 
		    StringTypeLower, StringTypePrint, StringTypePunct, StringTypeSpace, StringTypeTrue, 
		    StringTypeUpper, StringTypeWordchar, StringTypeXdigit };
static CONST char *stringTypeOpts[] = {"alnum", "alpha", "ascii", "boolean", "control", 
			       "digit", "double", "false", "graph", "integer", 
			       "lower", "print", "punct", "space", "true", 
			       "upper", "wordchar", "xdigit", NULL};

static int convertToTclobj(Tcl_Interp *interp, Tcl_Obj *objPtr,  XOTclParam CONST *pPtr, 
			   ClientData *clientData, Tcl_Obj **outObjPtr) {
  Tcl_Obj *objv[3];
  int result;

  if (pPtr->converterArg) {
    /*fprintf(stderr, "convertToStringType %s (must be %s)\n", ObjStr(objPtr), ObjStr(pPtr->converterArg));*/

    objv[1] = pPtr->converterArg;
    objv[2] = objPtr;

    result = XOTclCallCommand(interp, XOTE_IS, 3, objv);
    if (result == TCL_OK) {
      int success;
      Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &success);
      if (success == 1) {
	*clientData = (ClientData)objPtr;
      } else {
	result = XOTclVarErrMsg(interp, "expected ", ObjStr(pPtr->converterArg), 
				" but got \"", ObjStr(objPtr), 
                                "\" for parameter ", pPtr->name, NULL);
      }
    }
  } else {
    *clientData = (ClientData)objPtr;
    result = TCL_OK;
  }
  *outObjPtr = objPtr;
  return result;
}

static int convertToNothing(Tcl_Interp *interp, Tcl_Obj *objPtr,  XOTclParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  *outObjPtr = objPtr;
  return TCL_OK;
}

static int convertToBoolean(Tcl_Interp *interp, Tcl_Obj *objPtr,  XOTclParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result, bool;
  result = Tcl_GetBooleanFromObj(interp, objPtr, &bool);

  if (result == TCL_OK) {
    *clientData = (ClientData)INT2PTR(bool);
  } else {
    XOTclVarErrMsg(interp, "expected boolean value but got \"", ObjStr(objPtr), 
                   "\" for parameter ", pPtr->name, NULL);
  }
  *outObjPtr = objPtr;
  return result;
}

static int convertToInteger(Tcl_Interp *interp, Tcl_Obj *objPtr,  XOTclParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result, i;
  
  result = Tcl_GetIntFromObj(interp, objPtr, &i);

  if (result == TCL_OK) {
    *clientData = (ClientData)INT2PTR(i);
    *outObjPtr = objPtr;
  } else {
    XOTclVarErrMsg(interp, "expected integer but got \"", ObjStr(objPtr), 
                   "\" for parameter ", pPtr->name, NULL);
  }
  return result;
}

static int convertToSwitch(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, 
			   ClientData *clientData, Tcl_Obj **outObjPtr) {
  return convertToBoolean(interp, objPtr, pPtr, clientData, outObjPtr);
}

static int objectOfType(Tcl_Interp *interp, XOTclObject *object, CONST char *what, Tcl_Obj *objPtr, 
			XOTclParam CONST *pPtr) {
  XOTclClass *cl;
  Tcl_DString ds, *dsPtr = &ds;
  
  if (pPtr->converterArg == NULL) 
    return TCL_OK;

  if ((GetClassFromObj(interp, pPtr->converterArg, &cl, NULL) == TCL_OK)
      && isSubType(object->cl, cl)) {
    return TCL_OK;
  }

  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, what, -1);
  Tcl_DStringAppend(dsPtr, " of type ", -1);
  Tcl_DStringAppend(dsPtr, ObjStr(pPtr->converterArg), -1);
  XOTclObjErrType(interp, objPtr, Tcl_DStringValue(dsPtr), pPtr->name);
  DSTRING_FREE(dsPtr);

  return TCL_ERROR;
}

static int convertToObject(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, 
			   ClientData *clientData, Tcl_Obj **outObjPtr) {
  *outObjPtr = objPtr;
  if (GetObjectFromObj(interp, objPtr, (XOTclObject **)clientData) == TCL_OK) {
    return objectOfType(interp, (XOTclObject *)*clientData, "object", objPtr, pPtr);
  }
  return XOTclObjErrType(interp, objPtr, "object", pPtr->name);
}

static int convertToClass(Tcl_Interp *interp, Tcl_Obj *objPtr,  XOTclParam CONST *pPtr, 
			  ClientData *clientData, Tcl_Obj **outObjPtr) {
  *outObjPtr = objPtr;
  if (GetClassFromObj(interp, objPtr, (XOTclClass **)clientData, NULL) == TCL_OK) {
    return objectOfType(interp, (XOTclObject *)*clientData, "class", objPtr, pPtr);
  }
  return XOTclObjErrType(interp, objPtr, "class", pPtr->name);
}

static int convertToRelation(Tcl_Interp *interp, Tcl_Obj *objPtr,  XOTclParam CONST *pPtr, 
			     ClientData *clientData, Tcl_Obj **outObjPtr) {
  /* XOTclRelationCmd is the real setter, which checks the values
     according to the relation type (Class, List of Class, list of
     filters; we treat it here just like a tclobj */
  *clientData = (ClientData)objPtr;
  *outObjPtr = objPtr;
  return TCL_OK;
}

static int convertViaCmd(Tcl_Interp *interp, Tcl_Obj *objPtr,  XOTclParam CONST *pPtr, 
			 ClientData *clientData, Tcl_Obj **outObjPtr) {
  Tcl_Obj *ov[5];
  int result, oc;
  
  ov[0] = pPtr->slotObj ? pPtr->slotObj : XOTclGlobalObjs[XOTE_METHOD_PARAMETER_SLOT_OBJ];
  ov[1] = pPtr->converterName;
  ov[2] = pPtr->nameObj;
  ov[3] = objPtr;

  /*fprintf(stderr, "convertViaCmd call converter %s (refCount %d) on %s paramPtr %p\n", 
    ObjStr(pPtr->converterName), pPtr->converterName->refCount, ObjStr(ov[0]), pPtr);*/
  oc = 4;
  if (pPtr->converterArg) {
    ov[4] = pPtr->converterArg;
    oc++;
  }

  result = Tcl_EvalObjv(interp, oc, ov, 0);
  if (result == TCL_OK) {
    /*fprintf(stderr, "convertViaCmd converts %s to '%s' paramPtr %p\n", 
      ObjStr(objPtr), ObjStr(Tcl_GetObjResult(interp)),pPtr);*/
    *outObjPtr = Tcl_GetObjResult(interp);
    *clientData = (ClientData) *outObjPtr;

    /* incr refCount is necessary e.g. for 
       return [expr {$value + 1}]
    */
    INCR_REF_COUNT(*outObjPtr);   
  } else {
    *outObjPtr = objPtr;
  }
  return result;
}

static int convertToObjpattern(Tcl_Interp *interp, Tcl_Obj *objPtr,  XOTclParam CONST *pPtr, 
			       ClientData *clientData, Tcl_Obj **outObjPtr) {
  Tcl_Obj *patternObj = objPtr;
  CONST char *pattern = ObjStr(objPtr);

  if (noMetaChars(pattern)) {
    /* we have no meta characters, we try to check for an existing object */
    XOTclObject *object = NULL;
    GetObjectFromObj(interp, objPtr, &object);
    if (object) {
      patternObj = object->cmdName;
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
      Tcl_AppendLimitedToObj(patternObj, pattern, -1, INT_MAX, NULL);
    }
  }
  if (patternObj) {
    INCR_REF_COUNT(patternObj);
  }
  *clientData = (ClientData)patternObj;
  *outObjPtr = objPtr;
  return TCL_OK;
}

static Tcl_Obj*
ParamCheckObj(Tcl_Interp *interp, CONST char *start, size_t len) {
  Tcl_Obj *checker = Tcl_NewStringObj("type=", 5);
  Tcl_AppendLimitedToObj(checker, start, len, INT_MAX, NULL);
  return checker;
}

static int
ParamOptionSetConverter(Tcl_Interp *interp, XOTclParam *paramPtr, 
                        CONST char *typeName, XOTclTypeConverter *converter) {
  if (paramPtr->converter) {
    return XOTclVarErrMsg(interp, "Refuse to redefine parameter converter to use ",
                          typeName, (char *) NULL);
  }
  paramPtr->converter = converter;
  paramPtr->nrArgs = 1;
  paramPtr->type = typeName;
  return TCL_OK;
}

static int
ParamOptionParse(Tcl_Interp *interp, CONST char *option, size_t length, int disallowedOptions, XOTclParam *paramPtr) {
  int result = TCL_OK; 
  /*fprintf(stderr, "ParamOptionParse name %s, option '%s' (%d) disallowed %.6x\n",
    paramPtr->name, option, length, disallowedOptions);*/
  if (strncmp(option, "required", MAX(3,length)) == 0) {
    paramPtr->flags |= XOTCL_ARG_REQUIRED;
  } else if (strncmp(option, "optional",  MAX(3,length)) == 0) {
    paramPtr->flags &= ~XOTCL_ARG_REQUIRED;
  } else if (strncmp(option, "substdefault", 12) == 0) {
    paramPtr->flags |= XOTCL_ARG_SUBST_DEFAULT;
  } else if (strncmp(option, "allowempty", 10) == 0) {
    paramPtr->flags |= XOTCL_ARG_ALLOW_EMPTY;
  } else if (strncmp(option, "initcmd", 7) == 0) {
    paramPtr->flags |= XOTCL_ARG_INITCMD;
  } else if (strncmp(option, "method", 6) == 0) {
    paramPtr->flags |= XOTCL_ARG_METHOD;
  } else if (strncmp(option, "multivalued", 11) == 0) {
    if ((paramPtr->flags & (XOTCL_ARG_INITCMD|XOTCL_ARG_RELATION|XOTCL_ARG_METHOD|XOTCL_ARG_SWITCH)) != 0)
      return XOTclVarErrMsg(interp, 
                            "option multivalued not allowed for \"initcmd\", \"method\", \"relation\" or \"switch\"\n", 
                            (char *) NULL);
    paramPtr->flags |= XOTCL_ARG_MULTIVALUED;
  } else if (strncmp(option, "noarg", 5) == 0) {
    if ((paramPtr->flags & XOTCL_ARG_METHOD) == 0) {
      return XOTclVarErrMsg(interp, "option noarg only allowed for parameter type \"method\"", 
                            (char *) NULL);
    }
    paramPtr->flags |= XOTCL_ARG_NOARG;
    paramPtr->nrArgs = 0;
  } else if (length >= 4 && strncmp(option, "arg=", 4) == 0) {
    if ((paramPtr->flags & (XOTCL_ARG_METHOD|XOTCL_ARG_RELATION)) == 0
        && paramPtr->converter != convertViaCmd)
      return XOTclVarErrMsg(interp, 
                            "option arg= only allowed for \"method\", \"relation\" or \"user-defined converter\"", 
                            (char *) NULL);
    paramPtr->converterArg =  Tcl_NewStringObj(option+4, length-4);
    INCR_REF_COUNT(paramPtr->converterArg);
  } else if (strncmp(option, "switch", 6) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "switch", convertToSwitch);
    paramPtr->flags |= XOTCL_ARG_SWITCH;
    paramPtr->nrArgs = 0;
    assert(paramPtr->defaultValue == NULL);
    paramPtr->defaultValue = Tcl_NewBooleanObj(0);
    INCR_REF_COUNT(paramPtr->defaultValue);
  } else if (strncmp(option, "integer", MAX(3,length)) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "integer", convertToInteger);
  } else if (strncmp(option, "boolean", 7) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "boolean", convertToBoolean);
  } else if (strncmp(option, "object", 6) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "object", convertToObject);
  } else if (strncmp(option, "class", 5) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "class", convertToClass);
  } else if (strncmp(option, "relation", 8) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "relation", convertToRelation);
    paramPtr->flags |= XOTCL_ARG_RELATION;
    /*paramPtr->type = "tclobj";*/
  } else if (length >= 6 && strncmp(option, "type=", 5) == 0) {
    if (paramPtr->converter != convertToObject && 
        paramPtr->converter != convertToClass) 
      return XOTclVarErrMsg(interp, "option type= only allowed for object or class", (char *) NULL);
    paramPtr->converterArg = Tcl_NewStringObj(option+5, length-5);
    INCR_REF_COUNT(paramPtr->converterArg);
  } else if (length >= 6 && strncmp(option, "slot=", 5) == 0) {
    paramPtr->slotObj = Tcl_NewStringObj(option+5, length-5);
    INCR_REF_COUNT(paramPtr->slotObj);
  } else {
    int i, found = -1;

    for (i=0; stringTypeOpts[i]; i++) {
      /* Do not allow abbreviations, so the additional strlen checks
	 for a full match */
      if (strncmp(option, stringTypeOpts[i], length) == 0 && strlen(stringTypeOpts[i]) == length) {
	found = i;
	break;
      }
    }
    if (found > -1) {
      /* converter is stringType */
      result = ParamOptionSetConverter(interp, paramPtr, "stringtype", convertToTclobj);
      paramPtr->converterArg =  Tcl_NewStringObj(stringTypeOpts[i], -1);
      INCR_REF_COUNT(paramPtr->converterArg);      
    } else {
      /* must be a converter defined via method */
      paramPtr->converterName = ParamCheckObj(interp, option, length);
      INCR_REF_COUNT(paramPtr->converterName);
      result = ParamOptionSetConverter(interp, paramPtr, ObjStr(paramPtr->converterName), convertViaCmd);
    }
  }

  if ((paramPtr->flags & disallowedOptions)) {
    return XOTclVarErrMsg(interp, "Parameter option '", option, "' not allowed", (char *) NULL);
  }
  
  return result;
}

static int
ParamParse(Tcl_Interp *interp, CONST char *procName, Tcl_Obj *arg, int disallowedFlags,
           XOTclParam *paramPtr, int *possibleUnknowns, int *plainParams) {
  int result, npac, isNonposArgument;
  size_t nameLength, length, j;
  CONST char *argString, *argName;
  Tcl_Obj **npav;

  paramPtr->paramObj = arg;
  INCR_REF_COUNT(paramPtr->paramObj);

  result = Tcl_ListObjGetElements(interp, arg, &npac, &npav);
  if (result != TCL_OK || npac < 1 || npac > 2) {
    return XOTclVarErrMsg(interp, "wrong # of elements in parameter definition for method ",
                          procName, " (should be 1 or 2 list elements): ",
                          ObjStr(arg), (char *) NULL);
  }

  argString = ObjStr(npav[0]);
  length = strlen(argString);

  isNonposArgument = *argString == '-';

  if (isNonposArgument) {
    argName = argString+1;
    nameLength = length-1;
    paramPtr->nrArgs = 1; /* per default 1 argument, switches set their arg numbers */
  } else {
    argName = argString;
    nameLength = length;
    paramPtr->flags |= XOTCL_ARG_REQUIRED; /* positional arguments are required unless we have a default */
  }

  /* fprintf(stderr, "... parsing '%s', name '%s' \n", ObjStr(arg), argName);*/

  /* find the first ':' */
  for (j=0; j<length; j++) {
    if (argString[j] == ':') break;
  }

  if (argString[j] == ':') {
    /* we found a ':' */
    size_t l, start, end;

    /* get parameter name */
    NEW_STRING(paramPtr->name, argString, j);
    paramPtr->nameObj = Tcl_NewStringObj(argName, isNonposArgument ? j-1 : j);
    INCR_REF_COUNT(paramPtr->nameObj);

    /* skip space at begin */
    for (start = j+1; start<length && isspace((int)argString[start]); start++) {;}

    /* search for ',' */
    for (l=start; l<length; l++) {
      if (argString[l] == ',') {
	/* skip space from end */
        for (end = l; end>0 && isspace((int)argString[end-1]); end--);
        result = ParamOptionParse(interp, argString+start, end-start, disallowedFlags, paramPtr);
        if (result != TCL_OK) {
          goto param_error;
        }
        l++;
        /* skip space from begin */
        for (start = l; start<length && isspace((int)argString[start]); start++) {;}
      }
    }
    /* skip space from end */
    for (end = l; end>0 && isspace((int)argString[end-1]); end--);
    /* process last option */
    result = ParamOptionParse(interp, argString+start, end-start, disallowedFlags, paramPtr);
    if (result != TCL_OK) {
      goto param_error;
    }
  } else {
    /* no ':', the whole arg is the name, we have not options */
    NEW_STRING(paramPtr->name, argString, length);
    if (isNonposArgument) {
      paramPtr->nameObj = Tcl_NewStringObj(argName, length-1);
    } else {
      (*plainParams) ++;
      paramPtr->nameObj = Tcl_NewStringObj(argName, length);
    }
    INCR_REF_COUNT(paramPtr->nameObj);
  }

  /* if we have two arguments in the list, the second one is a default value */
  if (npac == 2) {

    if (disallowedFlags & XOTCL_ARG_HAS_DEFAULT) {
      XOTclVarErrMsg(interp, "parameter \"", argString, 
                     "\" is not allowed to have default \"",
                     ObjStr(npav[1]), "\"", (char *) NULL);
      goto param_error;
    }

    /* if we have for some reason already a default value, free it */
    if (paramPtr->defaultValue) {
      DECR_REF_COUNT(paramPtr->defaultValue);
    }
    paramPtr->defaultValue = Tcl_DuplicateObj(npav[1]);
    INCR_REF_COUNT(paramPtr->defaultValue);
    /*
     * The argument will be not required for an invocation, since we
     * have a default.
     */
    paramPtr->flags &= ~XOTCL_ARG_REQUIRED;
  } else if (paramPtr->flags & XOTCL_ARG_SUBST_DEFAULT) {
    XOTclVarErrMsg(interp, "parameter option substdefault specified for parameter \"", 
		   paramPtr->name, "\" without default value", (char *) NULL);
    goto param_error;
  }

  /* postprocessing the parameter options */

  if (paramPtr->converter == NULL) {
    /* convertToTclobj() is the default converter */
    paramPtr->converter = convertToTclobj;
  } /*else if (paramPtr->converter == convertViaCmd) {*/

  if ((paramPtr->slotObj || paramPtr->converter == convertViaCmd) && paramPtr->type) {
    Tcl_Obj *converterNameObj;
    CONST char *converterNameString;
    XOTclObject *paramObj;
    XOTclClass *pcl;
    Tcl_Command cmd;

    result = GetObjectFromObj(interp, paramPtr->slotObj ? paramPtr->slotObj : 
			      XOTclGlobalObjs[XOTE_METHOD_PARAMETER_SLOT_OBJ], 
			      &paramObj);
    if (result != TCL_OK)
      return result;
    
    if (paramPtr->converterName == NULL) {
      converterNameObj = ParamCheckObj(interp, paramPtr->type, strlen(paramPtr->type));
      INCR_REF_COUNT(converterNameObj);
    } else {
      converterNameObj = paramPtr->converterName;
    }
    converterNameString = ObjStr(converterNameObj);

    cmd = ObjectFindMethod(interp, paramObj, converterNameString, &pcl);
    if (cmd == NULL) {
      if (paramPtr->converter == convertViaCmd) {
        fprintf(stderr, "**** could not find checker method %s defined on %s\n",
                converterNameString, objectName(paramObj));
        paramPtr->flags |= XOTCL_ARG_CURRENTLY_UNKNOWN;
        /* TODO: for the time being, we do not return an error here */
      }
    } else if (paramPtr->converter != convertViaCmd &&
               strcmp(ObjStr(paramPtr->slotObj),
		      XOTclGlobalStrings[XOTE_METHOD_PARAMETER_SLOT_OBJ]) != 0) {
      /* todo remove me */
      fprintf(stderr, "**** checker method %s defined on %s shadows built-in converter\n",
              converterNameString, objectName(paramObj));
      if (paramPtr->converterName == NULL) {
        paramPtr->converterName = converterNameObj;
        paramPtr->converter = NULL;
        result = ParamOptionSetConverter(interp, paramPtr, converterNameString, convertViaCmd);
      }
    }
    if (converterNameObj != paramPtr->converterName) {
      DECR_REF_COUNT(converterNameObj);
    }
    
  }

  /*
   * If the argument is not required and no default value is
   * specified, we have to handle in the client code (eg. in the
   * canonical arg handlers for scripted methods) the unknown value
   * (e.g. don't set/unset a variable)
   */
  if (!(paramPtr->flags & XOTCL_ARG_REQUIRED) && paramPtr->defaultValue == NULL) {
    (*possibleUnknowns)++;
  }
  return TCL_OK;

 param_error:
  ckfree((char *)paramPtr->name);
  paramPtr->name = NULL
  DECR_REF_COUNT(paramPtr->nameObj);
  return TCL_ERROR;
}

static int
ParamDefsParse(Tcl_Interp *interp, CONST char *procName, Tcl_Obj *args, 
               int allowedOptinons, XOTclParsedParam *parsedParamPtr) {
  Tcl_Obj **argsv;
  int result, argsc;

  parsedParamPtr->paramDefs = NULL;
  parsedParamPtr->possibleUnknowns = 0;

  result = Tcl_ListObjGetElements(interp, args, &argsc, &argsv);
  if (result != TCL_OK) {
    return XOTclVarErrMsg(interp, "cannot break down non-positional args: ",
			  ObjStr(args), (char *) NULL);
  }

  if (argsc > 0) {
    XOTclParam *paramsPtr, *paramPtr, *lastParamPtr;
    int i, possibleUnknowns = 0, plainParams = 0;
    XOTclParamDefs *paramDefs;

    paramPtr = paramsPtr = ParamsNew(argsc);

    for (i=0; i < argsc; i++, paramPtr++) {
      result = ParamParse(interp, procName, argsv[i], allowedOptinons,
                      paramPtr, &possibleUnknowns, &plainParams);
      if (result != TCL_OK) {
        ParamsFree(paramsPtr);
        return result;
      }
    }

    /*
     * If all arguments are good old Tcl arguments, there is no need
     * to use the parameter definition structure.
     */
    if (plainParams == argsc) {
      ParamsFree(paramsPtr);
      return TCL_OK;
    }
    /*
    fprintf(stderr, "we need param definition structure for {%s}, argsc %d plain %d\n",
            ObjStr(args), argsc, plainParams);
    */
    /*
     * Check the last argument. If the last argument is named 'args',
     * force converter and make it non-required.
     */
    lastParamPtr = paramPtr - 1;
    if (isArgsString(lastParamPtr->name)) {
      lastParamPtr->converter = convertToNothing;
      lastParamPtr->flags &= ~XOTCL_ARG_REQUIRED;
    }

    paramDefs = ParamDefsNew();
    paramDefs->paramsPtr = paramsPtr;
    paramDefs->nrParams = paramPtr-paramsPtr;
    /*fprintf(stderr, "method %s ifsize %d, possible unknowns = %d,\n",
      procName, paramPtr-paramDefsPtr, possibleUnknowns);*/
    parsedParamPtr->paramDefs = paramDefs;
    parsedParamPtr->possibleUnknowns = possibleUnknowns;
  }
  return TCL_OK;
}

static int
MakeProc(Tcl_Namespace *nsPtr, XOTclAssertionStore *aStore, Tcl_Interp *interp,
         Tcl_Obj *nameObj, Tcl_Obj *args, Tcl_Obj *body, Tcl_Obj *precondition,
         Tcl_Obj *postcondition, XOTclObject *object, 
         int withPublic, int withPer_object, int clsns) {
  Tcl_CallFrame frame, *framePtr = &frame;
  CONST char *methodName = ObjStr(nameObj);
  XOTclParsedParam parsedParam;
  Tcl_Obj *ov[4];
  int result;
  
  /* Check, if we are allowed to redefine the method */
  result = CanRedefineCmd(interp, nsPtr, object, methodName);
  if (result == TCL_OK) {
    /* Yes, so obtain an method parameter definitions */
    result = ParamDefsParse(interp, methodName, args, XOTCL_DISALLOWED_ARG_METHOD_PARAMETER, &parsedParam);
  }
  if (result != TCL_OK) {
    return result;
  }

  ov[0] = NULL; /*objv[0];*/
  ov[1] = nameObj;

  if (parsedParam.paramDefs) {
    XOTclParam *pPtr;
    Tcl_Obj *argList = Tcl_NewListObj(0, NULL);

    for (pPtr = parsedParam.paramDefs->paramsPtr; pPtr->name; pPtr++) {
      if (*pPtr->name == '-') {
	Tcl_ListObjAppendElement(interp, argList, Tcl_NewStringObj(pPtr->name+1, -1));
      } else {
	Tcl_ListObjAppendElement(interp, argList, Tcl_NewStringObj(pPtr->name, -1));
      }
    }
    ov[2] = argList;
    INCR_REF_COUNT(ov[2]);
    /*fprintf(stderr, "final arglist = <%s>\n", ObjStr(argList)); */
    ov[3] = addPrefixToBody(body, 1, &parsedParam);
  } else { /* no nonpos arguments */
    ov[2] = args;
    ov[3] = addPrefixToBody(body, 0, &parsedParam);
  }
  
  Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, nsPtr, 0);
  /* create the method in the provided namespace */
  result = Tcl_ProcObjCmd(0, interp, 4, ov) != TCL_OK;
  if (result == TCL_OK) {
    /* retrieve the defined proc */
    Proc *procPtr = FindProcMethod(nsPtr, methodName);
    if (procPtr) {
      /* modify the cmd of the proc to set the current namespace for the body */
      if (clsns) {
        /*
         * Set the namespace of the method as inside of the class
         */
        if (!object->nsPtr) {
          makeObjNamespace(interp, object);
        }
        /*fprintf(stderr, "obj %s\n", objectName(object));
          fprintf(stderr, "ns %p object->ns %p\n", ns, object->nsPtr);
          fprintf(stderr, "ns %s object->ns %s\n", ns->fullName, object->nsPtr->fullName);*/
        procPtr->cmdPtr->nsPtr = (Namespace*) object->nsPtr;
      } else {
        /*
         * Set the namespace of the method to the same namespace the class has
       */
        procPtr->cmdPtr->nsPtr = ((Command *)object->id)->nsPtr;
      }
      
      ParamDefsStore(interp, (Tcl_Command)procPtr->cmdPtr, parsedParam.paramDefs);
#if 0
      if (!withPublic) {
        Tcl_Command_flags((Tcl_Command)procPtr->cmdPtr) |= XOTCL_CMD_PROTECTED_METHOD;
      }
#endif
      result = ListMethodHandle(interp, object, withPer_object, methodName);
    }
  }
  Tcl_PopCallFrame(interp);

  if (result == TCL_OK && (precondition || postcondition)) {
    AssertionAddProc(interp, methodName, aStore, precondition, postcondition);
  }

  if (parsedParam.paramDefs) {
    DECR_REF_COUNT(ov[2]);
  }
  DECR_REF_COUNT(ov[3]);

  return result;
}

static int
MakeMethod(Tcl_Interp *interp, XOTclObject *object, XOTclClass *cl, Tcl_Obj *nameObj,
           Tcl_Obj *args, Tcl_Obj *body,
           Tcl_Obj *precondition, Tcl_Obj *postcondition,
           int withPublic, int clsns) {
  CONST char *argsStr = ObjStr(args), *bodyStr = ObjStr(body), *nameStr = ObjStr(nameObj);
  int result;

  if (precondition && !postcondition) {
    return XOTclVarErrMsg(interp, className(cl), " method '", nameStr,
                          "'; when specifying a precondition (", ObjStr(precondition),
                          ") a postcondition must be specified as well",
                          (char *) NULL);
  }

  /* if both, args and body are empty strings, we delete the method */
  if (*argsStr == 0 && *bodyStr == 0) {
    result = cl ?
      XOTclRemoveClassMethod(interp, (XOTcl_Class *)cl, nameStr) :
      XOTclRemoveObjectMethod(interp, (XOTcl_Object *)object, nameStr);
  } else {
    XOTclAssertionStore *aStore = NULL;
    if (precondition || postcondition) {
      if (cl) {
        XOTclClassOpt *opt = XOTclRequireClassOpt(cl);
        if (!opt->assertions)
          opt->assertions = AssertionCreateStore();
        aStore = opt->assertions;
      } else {
        XOTclObjectOpt *opt = XOTclRequireObjectOpt(object);
        if (!opt->assertions)
          opt->assertions = AssertionCreateStore();
        aStore = opt->assertions;
      }
    }
    result = MakeProc(cl ? cl->nsPtr : object->nsPtr, aStore,
		      interp, nameObj, args, body, precondition, postcondition,
		      object, withPublic, cl == NULL, clsns);
  }

  if (cl) {
    /* could be a filter or filter inheritance ... update filter orders */
    FilterInvalidateObjOrders(interp, cl);
  } else {
    /* could be a filter => recompute filter order */
    FilterComputeDefined(interp, object);
  }

  return result;
}

static int
getMatchObject(Tcl_Interp *interp, Tcl_Obj *patternObj, Tcl_Obj *origObj,
                XOTclObject **matchObject, CONST char **pattern) {
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
  ForwardCmdClientData *tcd = (ForwardCmdClientData *)clientData;
  if (tcd->cmdName)     {DECR_REF_COUNT(tcd->cmdName);}
  if (tcd->subcommands) {DECR_REF_COUNT(tcd->subcommands);}
  if (tcd->onerror)     {DECR_REF_COUNT(tcd->onerror);}
  if (tcd->prefix)      {DECR_REF_COUNT(tcd->prefix);}
  if (tcd->args)        {DECR_REF_COUNT(tcd->args);}
  FREE(forwardCmdClientData, tcd);
}

static int
forwardProcessOptions(Tcl_Interp *interp, Tcl_Obj *nameObj,
                       Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix,
                       int withObjscope, Tcl_Obj *withOnerror, int withVerbose,
                       Tcl_Obj *target, int objc, Tcl_Obj * CONST objv[],
                       ForwardCmdClientData **tcdp) {
  ForwardCmdClientData *tcd;
  int i, result = 0;

  tcd = NEW(ForwardCmdClientData);
  memset(tcd, 0, sizeof(ForwardCmdClientData));

  if (withDefault) {
    Tcl_DString ds, *dsPtr = &ds;
    DSTRING_INIT(dsPtr);
    Tcl_DStringAppend(dsPtr, "%1 {", 4);
    Tcl_DStringAppend(dsPtr, ObjStr(withDefault), -1);
    Tcl_DStringAppend(dsPtr, "}", 1);
    XOTclDeprecatedCmd(interp, "forward option","-default ...", Tcl_DStringValue(dsPtr));
    DSTRING_FREE(dsPtr);

    tcd->subcommands = withDefault;
    result = Tcl_ListObjLength(interp, withDefault, &tcd->nr_subcommands);
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
  /*fprintf(stderr, "...forwardprocess objc %d\n", objc);*/

  for (i=0; i<objc; i++) {
    CONST char *element = ObjStr(objv[i]);
    /*fprintf(stderr, "... [%d] forwardprocess element '%s'\n", i, element);*/
    tcd->needobjmap |= (*element == '%' && *(element+1) == '@');
    tcd->hasNonposArgs |= (*element == '%' && *(element+1) == '-');
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
    tcd->cmdName = nameObj;
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
    CONST char *nameString = ObjStr(tcd->cmdName);
    if (!isAbsolutePath(nameString)) {
      tcd->cmdName = NameInNamespaceObj(interp, nameString, callingNameSpace(interp));
      /*fprintf(stderr, "name %s not absolute, therefore qualifying %s\n", nameObj,
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
  if (result == TCL_OK) {
    *tcdp = tcd;
  } else {
    forwardCmdDeleteProc((ClientData)tcd);
  }
  return result;
}

static XOTclClasses *
ComputePrecedenceList(Tcl_Interp *interp, XOTclObject *object, CONST char *pattern,
		      int withMixins, int withRootClass) {
  XOTclClasses *precedenceList = NULL, *pcl, **npl = &precedenceList;

  if (withMixins) {
    if (!(object->flags & XOTCL_MIXIN_ORDER_VALID))
      MixinComputeDefined(interp, object);

    if (object->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
      XOTclCmdList *ml = object->mixinOrder;

      while (ml) {
	XOTclClass *mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
	if (pattern) {
	  if (!Tcl_StringMatch(className(mixin), pattern)) continue;
	}
	npl = XOTclClassListAdd(npl, mixin, NULL);
	ml = ml->nextPtr;
      }
    }
  }

  pcl = ComputeOrder(object->cl, object->cl->order, Super);
  for (; pcl; pcl = pcl->nextPtr) {
    if (withRootClass == 0 && pcl->cl->object.flags & XOTCL_IS_ROOT_CLASS)
      continue;

    if (pattern) {
      if (!Tcl_StringMatch(className(pcl->cl), pattern)) continue;
    }
    npl = XOTclClassListAdd(npl, pcl->cl, NULL);
  }
  return precedenceList;
}

static CONST char *
StripBodyPrefix(CONST char *body) {
  if (strncmp(body, "::nsf::unsetUnknownArgs\n", 24) == 0)
    body += 29;
  return body;
}


static XOTclObjects *
computeSlotObjects(Tcl_Interp *interp, XOTclObject *object, CONST char *pattern, int withRootClass) {
  XOTclObjects *slotObjects = NULL, **npl = &slotObjects;
  XOTclClasses *pl, *fullPrecendenceList;
  XOTclObject *childObject, *tmpObject;
  Tcl_HashTable slotTable;

  assert(object);

  Tcl_InitHashTable(&slotTable, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", slotTable);

  fullPrecendenceList = ComputePrecedenceList(interp, object, NULL /* pattern*/, 1, withRootClass);
  for (pl=fullPrecendenceList; pl; pl = pl->nextPtr) {
    Tcl_DString ds, *dsPtr = &ds;

    DSTRING_INIT(dsPtr);
    Tcl_DStringAppend(dsPtr, className(pl->cl), -1);
    Tcl_DStringAppend(dsPtr, "::slot", 6);
    tmpObject = XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
    if (tmpObject) {
      Tcl_HashSearch hSrch;
      Tcl_HashEntry *hPtr, *slotEntry;
      Tcl_HashTable *cmdTable;
      Tcl_Command cmd;
      int new;

      if (!tmpObject->nsPtr) continue;
      cmdTable = Tcl_Namespace_cmdTable(tmpObject->nsPtr);

      hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch);
      for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
	char *key = Tcl_GetHashKey(cmdTable, hPtr);
	slotEntry = Tcl_CreateHashEntry(&slotTable, key, &new);
	if (!new) continue;
	cmd = (Tcl_Command) Tcl_GetHashValue(hPtr);
	childObject = XOTclGetObjectFromCmdPtr(cmd);
	/*fprintf(stderr, "we have true child obj %s\n", objectName(childObject));*/
	npl = XOTclObjectListAdd(npl, childObject);
      }
    }
    DSTRING_FREE(dsPtr);
  }

  Tcl_DeleteHashTable(&slotTable);
  MEM_COUNT_FREE("Tcl_InitHashTable", slotTable);

  XOTclClassListFree(fullPrecendenceList);

  return slotObjects;
}

static XOTclClass*
FindCalledClass(Tcl_Interp *interp, XOTclObject *object) {
  XOTclCallStackContent *cscPtr = CallStackGetTopFrame(interp, NULL);
  CONST char *methodName;
  Tcl_Command cmd;

  if (cscPtr->frameType == XOTCL_CSC_TYPE_PLAIN)
    return cscPtr->cl;

  if (cscPtr->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER)
    methodName = ObjStr(cscPtr->filterStackEntry->calledProc);
  else if (cscPtr->frameType == XOTCL_CSC_TYPE_ACTIVE_MIXIN && object->mixinStack)
    methodName = (char *)Tcl_GetCommandName(interp, cscPtr->cmdPtr);
  else
    return NULL;

  if (object->nsPtr) {
    cmd = FindMethod(object->nsPtr, methodName);
    if (cmd) {
      /* we called an object specific method */
      return NULL;
    }
  }

  return SearchCMethod(object->cl, methodName, &cmd);
}

/*
 * Next Primitive Handling
 */
XOTCLINLINE static int
NextSearchMethod(XOTclObject *object, Tcl_Interp *interp, XOTclCallStackContent *cscPtr,
                 XOTclClass **cl, CONST char **methodName, Tcl_Command *cmd,
                 int *isMixinEntry, int *isFilterEntry,
                 int *endOfFilterChain, Tcl_Command *currentCmd) {
  int endOfChain = 0, objflags;

  /*
   *  Next in filters
   */

  objflags = object->flags; /* avoid stalling */
  if (!(objflags & XOTCL_MIXIN_ORDER_VALID)) {
    MixinComputeDefined(interp, object);
    objflags = object->flags; /* avoid stalling */
  }

  if ((objflags & XOTCL_FILTER_ORDER_VALID) &&
      object->filterStack &&
      object->filterStack->currentCmdPtr) {
    *cmd = FilterSearchProc(interp, object, currentCmd, cl);
    /*fprintf(stderr, "EndOfChain? proc=%p, cmd=%p\n",*proc,*cmd);*/
    /*  XOTclCallStackDump(interp); XOTclStackDump(interp);*/

    if (*cmd == 0) {
      if (cscPtr->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
        /* reset the information to the values of method, cl
           to the values they had before calling the filters */
        *methodName = ObjStr(object->filterStack->calledProc);
        endOfChain = 1;
        *endOfFilterChain = 1;
        *cl = 0;
        /*fprintf(stderr, "EndOfChain resetting cl\n");*/
      }
    } else {
      *methodName = (char *) Tcl_GetCommandName(interp, *cmd);
      *endOfFilterChain = 0;
      *isFilterEntry = 1;
      return TCL_OK;
    }
  }

  /*
   *  Next in Mixins
   */
  assert(objflags & XOTCL_MIXIN_ORDER_VALID);
  /* otherwise: MixinComputeDefined(interp, object); */

  /*fprintf(stderr, "nextsearch: mixinorder valid %d stack=%p\n",
    obj->flags & XOTCL_MIXIN_ORDER_VALID, obj->mixinStack);*/

  if ((objflags & XOTCL_MIXIN_ORDER_VALID) &&  object->mixinStack) {
    int result = MixinSearchProc(interp, object, *methodName, cl, currentCmd, cmd);
    if (result != TCL_OK) {
      return result;
    }
    /*fprintf(stderr, "nextsearch: mixinsearch cmd %p, currentCmd %p\n",*cmd, *currentCmd);*/
    if (*cmd == 0) {
      if (cscPtr->frameType == XOTCL_CSC_TYPE_ACTIVE_MIXIN) {
        endOfChain = 1;
        *cl = 0;
      }
    } else {
      *isMixinEntry = 1;
      return TCL_OK;
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

  if (object->nsPtr && endOfChain) {
    *cmd = FindMethod(object->nsPtr, *methodName);
  } else {
    *cmd = NULL;
  }


  if (!*cmd) {
    XOTclClasses *pl;
#if 0
    /* a more explicit version, but slower */
    pl = ComputeOrder(object->cl, object->cl->order, Super);
    /* if we have a class, skip to the next class in the precedence order */
    if (*cl) {
      for (; pl; pl = pl->nextPtr) {
        if (pl->cl == *cl) {
          pl = pl->nextPtr;
          break;
        }
      }
    }
#else
    for (pl = ComputeOrder(object->cl, object->cl->order, Super); *cl && pl; pl = pl->nextPtr) {
      if (pl->cl == *cl) {
        *cl = NULL;
      }
    }
#endif

    /*
     * search for a further class method
     */
    *cl = SearchPLMethod(pl, *methodName, cmd);
    /*fprintf(stderr, "no cmd, cl = %p %s\n",*cl, className((*cl)));*/
  } else {
    *cl = 0;
  }

  return TCL_OK;
}

static int
XOTclNextMethod(XOTclObject *object, Tcl_Interp *interp, XOTclClass *givenCl,
                CONST char *givenMethodName, int objc, Tcl_Obj *CONST objv[],
                int useCallstackObjs, XOTclCallStackContent *cscPtr) {
  Tcl_Command cmd, currentCmd = NULL;
  int result, frameType = XOTCL_CSC_TYPE_PLAIN,
    isMixinEntry = 0, isFilterEntry = 0,
    endOfFilterChain = 0, decrObjv0 = 0;
  int nobjc; Tcl_Obj **nobjv;
  XOTclClass **cl = &givenCl;
  CONST char **methodName = &givenMethodName;
  Tcl_CallFrame *framePtr;

  if (!cscPtr) {
    cscPtr = CallStackGetTopFrame(interp, &framePtr);
  } else {
    /*
     * cscPtr was given (i.e. it is not yet on the stack. So we cannot
     *  get objc from the associated stack frame
     */
    framePtr = NULL;
    assert(useCallstackObjs == 0);
    /* fprintf(stderr, "XOTclNextMethod csc given, use %d, framePtr %p\n", useCallstackObjs, framePtr); */
  }

  /*fprintf(stderr, "XOTclNextMethod givenMethod = %s, csc = %p, useCallstackObj %d, objc %d cfp %p\n",
    givenMethodName, cscPtr, useCallstackObjs, objc, framePtr);*/

  /* if no args are given => use args from stack */
  if (objc < 2 && useCallstackObjs && framePtr) {
    if (cscPtr->objv) {
      nobjv = cscPtr->objv;
      nobjc = cscPtr->objc;
    } else {
      nobjc = Tcl_CallFrame_objc(framePtr);
      nobjv = (Tcl_Obj **)Tcl_CallFrame_objv(framePtr);
    }
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
  result = NextSearchMethod(object, interp, cscPtr, cl, methodName, &cmd,
                            &isMixinEntry, &isFilterEntry, &endOfFilterChain, &currentCmd);
  if (result != TCL_OK) {
    return result;
  }

  /*
    fprintf(stderr, "NextSearchMethod -- RETURN: method=%s eoffc=%d,",
    *methodName, endOfFilterChain);

    if (obj)
    fprintf(stderr, " obj=%s,", objectName(object));
    if ((*cl))
    fprintf(stderr, " cl=%s,", (*cl)->nsPtr->fullName);
    fprintf(stderr, " mixin=%d, filter=%d, proc=%p\n",
    isMixinEntry, isFilterEntry, proc);
  */
#if 0
  Tcl_ResetResult(interp); /* needed for bytecode support */
#endif
  if (cmd) {
    /*
     * change mixin state
     */
    if (object->mixinStack) {
      if (cscPtr->frameType == XOTCL_CSC_TYPE_ACTIVE_MIXIN)
        cscPtr->frameType = XOTCL_CSC_TYPE_INACTIVE_MIXIN;

      /* otherwise move the command pointer forward */
      if (isMixinEntry) {
        frameType = XOTCL_CSC_TYPE_ACTIVE_MIXIN;
        object->mixinStack->currentCmdPtr = currentCmd;
      }
    }
    /*
     * change filter state
     */
    if (object->filterStack) {
      if (cscPtr->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
	/*fprintf(stderr, "next changes filter state\n");*/
        cscPtr->frameType = XOTCL_CSC_TYPE_INACTIVE_FILTER;
      }

      /* otherwise move the command pointer forward */
      if (isFilterEntry) {
	/*fprintf(stderr, "next moves filter forward\n");*/
        frameType = XOTCL_CSC_TYPE_ACTIVE_FILTER;
        object->filterStack->currentCmdPtr = currentCmd;
      }
    }

    /*
     * now actually call the "next" method
     */

    /* cut the flag, that no stdargs should be used, if it is there */
    if (nobjc > 1) {
      CONST char *nobjv1 = ObjStr(nobjv[1]);
      if (nobjv1[0] == '-' && !strcmp(nobjv1, "--noArgs"))
        nobjc = 1;
    }
    cscPtr->callType |= XOTCL_CSC_CALL_IS_NEXT;
    RUNTIME_STATE(interp)->unknown = 0;
    result = MethodDispatch((ClientData)object, interp, nobjc, nobjv, cmd,
                            object, *cl, *methodName, frameType);
    cscPtr->callType &= ~XOTCL_CSC_CALL_IS_NEXT;

    if (cscPtr->frameType == XOTCL_CSC_TYPE_INACTIVE_FILTER)
      cscPtr->frameType = XOTCL_CSC_TYPE_ACTIVE_FILTER;
    else if (cscPtr->frameType == XOTCL_CSC_TYPE_INACTIVE_MIXIN)
      cscPtr->frameType = XOTCL_CSC_TYPE_ACTIVE_MIXIN;
  } else if (result == TCL_OK && endOfFilterChain) {
    /*fprintf(stderr, "setting unknown to 1\n");*/
    RUNTIME_STATE(interp)->unknown = 1;
  }

  if (decrObjv0) {
    INCR_REF_COUNT(nobjv[0]);
  }

  return result;
}

int
XOTclNextObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclCallStackContent *cscPtr = CallStackGetTopFrame(interp, NULL);

  if (!cscPtr)
    return XOTclVarErrMsg(interp, "next: can't find self", (char *) NULL);

  if (!cscPtr->cmdPtr)
    return XOTclErrMsg(interp, "next: no executing proc", TCL_STATIC);

  return XOTclNextMethod(cscPtr->self, interp, cscPtr->cl,
                         (char *)Tcl_GetCommandName(interp, cscPtr->cmdPtr),
                         objc, objv, 1, NULL);
}


/*
 * "self" object command
 */

static int
FindSelfNext(Tcl_Interp *interp) {
  XOTclCallStackContent *cscPtr = CallStackGetTopFrame(interp, NULL);
  Tcl_Command cmd, currentCmd = 0;
  int result, isMixinEntry = 0,
    isFilterEntry = 0,
    endOfFilterChain = 0;
  XOTclClass *cl = cscPtr->cl;
  XOTclObject *object = cscPtr->self;
  CONST char *methodName;

  Tcl_ResetResult(interp);

  methodName = (char *)Tcl_GetCommandName(interp, cscPtr->cmdPtr);
  if (!methodName) {
    return TCL_OK;
  }

  result = NextSearchMethod(object, interp, cscPtr, &cl, &methodName, &cmd,
                   &isMixinEntry, &isFilterEntry, &endOfFilterChain, &currentCmd);
  if (cmd) {
    Tcl_SetObjResult(interp, MethodHandleObj(cl ? (XOTclObject*)cl : object, 
					     cl == NULL, methodName));
  }
  return result;
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
    XOTcl_ltoa(buffer+1, (long)Tcl_CallFrame_level(framePtr), &l);
    /*fprintf(stderr, "*** framePtr=%p buffer %s\n", framePtr, buffer);*/
    resultObj = Tcl_NewStringObj(buffer, l+1);
  } else {
    /* If not called from an xotcl frame, return 1 as default */
    resultObj = Tcl_NewIntObj(1);
  }

  return resultObj;
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
freeUnsetTraceVariable(Tcl_Interp *interp, XOTclObject *object) {
  int result = TCL_OK;
  if (object->opt && object->opt->volatileVarName) {
    /*
      Somebody destroys a volatile object manually while
      the vartrace is still active. Destroying the object will
      be a problem in case the variable is deleted later
      and fires the trace. So, we unset the variable here
      which will cause a destroy via var trace, which in
      turn clears the volatileVarName flag.
    */
    /*fprintf(stderr, "### freeUnsetTraceVariable %s\n", obj->opt->volatileVarName);*/

    result = Tcl_UnsetVar2(interp, object->opt->volatileVarName, NULL, 0);
    if (result != TCL_OK) {
      int result = Tcl_UnsetVar2(interp, object->opt->volatileVarName, NULL, TCL_GLOBAL_ONLY);
      if (result != TCL_OK) {
        Namespace *nsPtr = (Namespace *) Tcl_GetCurrentNamespace(interp);
        if (unsetInAllNamespaces(interp, nsPtr, object->opt->volatileVarName) == 0) {
          fprintf(stderr, "### don't know how to delete variable '%s' of volatile object\n",
                  object->opt->volatileVarName);
	}
      }
    }
    if (result == TCL_OK) {
      /*fprintf(stderr, "### success unset\n");*/
    }
  }
  return  result;
}

static char *
XOTclUnsetTrace(ClientData clientData, Tcl_Interp *interp, CONST char *name, CONST char *name2, int flags)
{
  Tcl_Obj *obj = (Tcl_Obj *)clientData;
  XOTclObject *object;
  char *resultMsg = NULL;

  /*fprintf(stderr, "XOTclUnsetTrace %s flags %.4x %.4x\n", name, flags,
    flags & TCL_INTERP_DESTROYED); */

  if ((flags & TCL_INTERP_DESTROYED) == 0) {
    if (GetObjectFromObj(interp, obj, &object) == TCL_OK) {
      Tcl_Obj *res = Tcl_GetObjResult(interp); /* save the result */
      INCR_REF_COUNT(res);

      /* clear variable, destroy is called from trace */
      if (object->opt && object->opt->volatileVarName) {
        object->opt->volatileVarName = NULL;
      }

      if (callDestroyMethod(interp, object, 0) != TCL_OK) {
        resultMsg = "Destroy for volatile object failed";
      } else
        resultMsg = "No XOTcl Object passed";

      Tcl_SetObjResult(interp, res);  /* restore the result */
      DECR_REF_COUNT(res);
    }
    DECR_REF_COUNT(obj);
  } else {
    /*fprintf(stderr, "omitting destroy on %s %p\n", name);*/
  }
  return resultMsg;
}

/*
 * bring an object into a state, as after initialization
 */
static void
CleanupDestroyObject(Tcl_Interp *interp, XOTclObject *object, int softrecreate) {
  /*fprintf(stderr, "CleanupDestroyObject obj %p softrecreate %d nsPtr %p\n", 
    object, softrecreate, object->nsPtr);*/

  /* remove the instance, but not for ::Class/::Object */
  if ((object->flags & XOTCL_IS_ROOT_CLASS) == 0 &&
      (object->flags & XOTCL_IS_ROOT_META_CLASS) == 0 ) {

    if (!softrecreate) {
      (void)RemoveInstance(object, object->cl);
    }
  }

  if (object->nsPtr) {
    NSCleanupNamespace(interp, object->nsPtr);
    NSDeleteChildren(interp, object->nsPtr);
  }

  if (object->varTable) {
    TclDeleteVars(((Interp *)interp), object->varTable);

    ckfree((char *)object->varTable);
    /*FREE(obj->varTable, obj->varTable);*/
    object->varTable = 0;
  }

  if (object->opt) {
    XOTclObjectOpt *opt = object->opt;
    AssertionRemoveStore(opt->assertions);
    opt->assertions = NULL;

#ifdef XOTCL_METADATA
    XOTclMetaDataDestroy(object);
#endif

    if (!softrecreate) {
      /*
       *  Remove this object from all per object mixin lists and clear the mixin list
       */
      removeFromObjectMixinsOf(object->id, opt->mixins);

      CmdListRemoveList(&opt->mixins, GuardDel);
      CmdListRemoveList(&opt->filters, GuardDel);
      FREE(XOTclObjectOpt, opt);
      opt = object->opt = 0;
    }
  }

  object->flags &= ~XOTCL_MIXIN_ORDER_VALID;
  if (object->mixinOrder)  MixinResetOrder(object);
  object->flags &= ~XOTCL_FILTER_ORDER_VALID;
  if (object->filterOrder) FilterResetOrder(object);
}

/*
 * do obj initialization & namespace creation
 */
static void
CleanupInitObject(Tcl_Interp *interp, XOTclObject *object,
                  XOTclClass *cl, Tcl_Namespace *nsPtr, int softrecreate) {

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "+++ CleanupInitObject\n");
#endif
  object->teardown = interp;
  object->nsPtr = nsPtr;
  if (!softrecreate) {
    AddInstance(object, cl);
  }
  if (object->flags & XOTCL_RECREATE) {
    object->opt = 0;
    object->varTable = 0;
    object->mixinOrder = 0;
    object->filterOrder = 0;
    object->flags = 0;
  }
  /*
    fprintf(stderr, "cleanupInitObject %s: %p cl = %p\n",
    obj->cmdName ? objectName(object) : "", object, object->cl);*/
}

static void
PrimitiveDestroy(ClientData clientData) {
  XOTclObject *object = (XOTclObject*)clientData;

  if (XOTclObjectIsClass(object))
    PrimitiveCDestroy((ClientData) object);
  else
    PrimitiveODestroy((ClientData) object);
}

static void
tclDeletesObject(ClientData clientData) {
  XOTclObject *object = (XOTclObject*)clientData;
  Tcl_Interp *interp;

  object->flags |= XOTCL_TCL_DELETE;
  /*fprintf(stderr, "cmd dealloc %p tclDeletesObject (2d)\n", object->id,  Tcl_Command_refCount(object->id));
   */

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "tclDeletesObject %p obj->id %p flags %.6x\n", object, object->id, object->flags);
#endif
  if ((object->flags & XOTCL_DURING_DELETE) || !object->teardown) return;
  interp = object->teardown;
# ifdef OBJDELETION_TRACE
  fprintf(stderr, "... %p %s\n", object, objectName(object));
# endif
  CallStackDestroyObject(interp, object);
  /*fprintf(stderr, "tclDeletesObject %p DONE\n", object);*/
}

/*
 * physical object destroy
 */
static void
PrimitiveODestroy(ClientData clientData) {
  XOTclObject *object = (XOTclObject*)clientData;
  Tcl_Interp *interp;

  if (!object || !object->teardown) return;

  /*fprintf(stderr, "****** PrimitiveODestroy %p flags %.6x\n", object, object->flags);*/
  assert(!(object->flags & XOTCL_DELETED));

  /* destroy must have been called already */
  assert(object->flags & XOTCL_DESTROY_CALLED);

  /*
   * check and latch against recurrent calls with object->teardown
   */
  PRINTOBJ("PrimitiveODestroy", object);
  interp = object->teardown;

  /*
   * Don't destroy, if the interpreter is destroyed already
   * e.g. TK calls Tcl_DeleteInterp directly, if the window is killed
   */
  if (Tcl_InterpDeleted(interp)) return;

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "  physical delete of %p id=%p destroyCalled=%d '%s'\n",
          object, object->id, (object->flags & XOTCL_DESTROY_CALLED), objectName(object));
#endif
  CleanupDestroyObject(interp, object, 0);

  while (object->mixinStack)
    MixinStackPop(object);

  while (object->filterStack)
    FilterStackPop(object);

  object->teardown = NULL;
  if (object->nsPtr) {
    /*fprintf(stderr, "PrimitiveODestroy calls deleteNamespace for object %p nsPtr %p\n", object, object->nsPtr);*/
    XOTcl_DeleteNamespace(interp, object->nsPtr);
    object->nsPtr = NULL;
  }

  /*fprintf(stderr, " +++ OBJ/CLS free: %s\n", objectName(object));*/

  object->flags |= XOTCL_DELETED;
  objTrace("ODestroy", object);

  DECR_REF_COUNT(object->cmdName);
  XOTclCleanupObject(object);

}

/*
 * reset the object to a fresh, undestroyed state
 */
static void
MarkUndestroyed(XOTclObject *object) {
  object->flags &= ~XOTCL_DESTROY_CALLED;
}

static void
PrimitiveOInit(void *mem, Tcl_Interp *interp, CONST char *name, XOTclClass *cl) {
  XOTclObject *object = (XOTclObject*)mem;
  Tcl_Namespace *nsPtr;

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "+++ PrimitiveOInit\n");
#endif

#ifdef XOTCLOBJ_TRACE
  fprintf(stderr, "OINIT %s = %p\n", name, object);
#endif
  XOTclObjectRefCountIncr(object);
  MarkUndestroyed(object);

  /* 
   * There might be already a namespace with name name; if this is the
   * case, use this namepsace as object namespace. The preexisting
   * namespace might contain XOTcl objects. If we would not use the
   * namespace as child namespace, we would not recognize the objects
   * as child objects, deletions of the object might lead to a crash.
   */

  nsPtr = Tcl_FindNamespace(interp, name, (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY);
  /*fprintf(stderr, "PrimitiveOInit %p %s, ns %p\n", object, name, nsPtr); */

  CleanupInitObject(interp, object, cl, nsPtr, 0);

  /*obj->flags = XOTCL_MIXIN_ORDER_VALID | XOTCL_FILTER_ORDER_VALID;*/
  object->mixinStack = NULL;
  object->filterStack = NULL;
}

/*
 * Object creation: create object name (full name) and Tcl command
 */
static XOTclObject*
PrimitiveOCreate(Tcl_Interp *interp, Tcl_Obj *nameObj, XOTclClass *cl) {
  XOTclObject *object = (XOTclObject*)ckalloc(sizeof(XOTclObject));
  CONST char *nameString = ObjStr(nameObj);
  size_t length;

#if defined(XOTCLOBJ_TRACE)
  fprintf(stderr, "CKALLOC Object %p %s\n", object, nameString);
#endif
#ifdef OBJDELETION_TRACE
  fprintf(stderr, "+++ PrimitiveOCreate\n");
#endif

  memset(object, 0, sizeof(XOTclObject));
  MEM_COUNT_ALLOC("XOTclObject/XOTclClass", object);
  assert(object); /* ckalloc panics, if malloc fails */
  assert(isAbsolutePath(nameString));
  length = strlen(nameString);
  if (!NSCheckForParent(interp, nameString, length, cl)) {
    ckfree((char *) object);
    return NULL;
  }

  object->id = Tcl_CreateObjCommand(interp, nameString, XOTclObjDispatch,
                                 (ClientData)object, tclDeletesObject);
  /*fprintf(stderr, "cmd alloc %p %d (%s)\n", object->id, Tcl_Command_refCount(object->id), nameString);*/

  PrimitiveOInit(object, interp, nameString, cl);
  object->cmdName = nameObj;
  /* convert cmdName to Tcl Obj of type cmdName */
  /*Tcl_GetCommandFromObj(interp, obj->cmdName);*/

  INCR_REF_COUNT(object->cmdName);
  objTrace("PrimitiveOCreate", object);

  return object;
}

static XOTclClass *
DefaultSuperClass(Tcl_Interp *interp, XOTclClass *cl, XOTclClass *mcl, int isMeta) {
  XOTclClass *defaultClass = NULL;

  /*fprintf(stderr, "DefaultSuperClass cl %s, mcl %s, isMeta %d\n",
          className(cl), className(mcl), isMeta );*/

  if (mcl) {
    int result;
    result = setInstVar(interp, (XOTclObject *)mcl, isMeta ?
                        XOTclGlobalObjs[XOTE_DEFAULTMETACLASS] :
                        XOTclGlobalObjs[XOTE_DEFAULTSUPERCLASS], NULL);

    if (result == TCL_OK) {
      Tcl_Obj *nameObj = Tcl_GetObjResult(interp);
      if (GetClassFromObj(interp, nameObj, &defaultClass, NULL) != TCL_OK) {
	XOTclErrMsg(interp, "default superclass is not a class", TCL_STATIC);
      }
      /*fprintf(stderr, "DefaultSuperClass for %s got from var %s\n", className(cl), ObjStr(nameObj));*/

    } else {
      XOTclClass *result;
      XOTclClasses *sc;

      /*fprintf(stderr, "DefaultSuperClass for %s: search in superclasses starting with %p meta %d\n", 
        className(cl), cl->super, isMeta);*/

      /* 
       * check superclasses of metaclass 
       */
      if (isMeta) {
	/*fprintf(stderr, "  ... is %s already root meta %d\n",
                className(mcl->object.cl), 
                mcl->object.cl->object.flags & XOTCL_IS_ROOT_META_CLASS);*/
        if (mcl->object.cl->object.flags & XOTCL_IS_ROOT_META_CLASS) {
          return mcl->object.cl;
        }
      }
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
            /*fprintf(stderr, "found root class %p\n", sc->cl);*/
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
  XOTclClass *baseClass = NULL;

  PRINTOBJ("CleanupDestroyClass", (XOTclObject *)cl);
  assert(softrecreate ? recreate == 1 : 1);

  /* fprintf(stderr, "CleanupDestroyClass %p %s (ismeta=%d) softrecreate=%d, recreate=%d, %p\n", cl,className(cl),IsMetaClass(interp, cl, 1),
     softrecreate, recreate, clopt);*/

  /* 
   * Perform the next steps even with clopt == NULL, since the class
   * might be used as a superclass of a per object mixin, so it might
   * have no clopt...
   */
  MixinInvalidateObjOrders(interp, cl);
  FilterInvalidateObjOrders(interp, cl);

  if (clopt) {
    /*
     *  Remove this class from all isClassMixinOf lists and clear the
     *  class mixin list
     */
    RemoveFromClassMixinsOf(clopt->id, clopt->classmixins);

    CmdListRemoveList(&clopt->classmixins, GuardDel);
    /*MixinInvalidateObjOrders(interp, cl);*/

    CmdListRemoveList(&clopt->classfilters, GuardDel);
    /*FilterInvalidateObjOrders(interp, cl);*/

    if (!recreate) {
      /*
       *  Remove this class from all mixin lists and clear the isObjectMixinOf list
       */
      RemoveFromMixins(clopt->id, clopt->isObjectMixinOf);
      CmdListRemoveList(&clopt->isObjectMixinOf, GuardDel);

      /*
       *  Remove this class from all class mixin lists and clear the
       *  isClassMixinOf list
       */
      RemoveFromClassmixins(clopt->id, clopt->isClassMixinOf);
      CmdListRemoveList(&clopt->isClassMixinOf, GuardDel);
    }

    /* 
     * Remove dependent filters of this class from all subclasses
     */
    FilterRemoveDependentFilterCmds(cl, cl);
    AssertionRemoveStore(clopt->assertions);
    clopt->assertions = NULL;
#ifdef XOTCL_OBJECTDATA
    XOTclFreeObjectData(cl);
#endif
  }
  
  NSCleanupNamespace(interp, cl->nsPtr);
  NSDeleteChildren(interp, cl->nsPtr);

  /*fprintf(stderr, "    CleanupDestroyClass softrecreate %d\n", softrecreate);*/

  if (!softrecreate) {

    /* 
     * Reclass all instances of the current class the the appropriate
     * most general class ("baseClass"). The most general class of a
     * metaclass is the root meta class, the most general class of an
     * object is the root class. Instances of metaclasses can be only
     * reset to the root meta class (and not to to the root base
     * class).
     */

    baseClass = DefaultSuperClass(interp, cl, cl->object.cl, 
				  IsMetaClass(interp, cl, 1));
    /* 
     * We do not have to reclassing in case, cl is a root class
     */
    if ((cl->object.flags & XOTCL_IS_ROOT_CLASS) == 0) {

      hPtr = &cl->instances ? Tcl_FirstHashEntry(&cl->instances, &hSrch) : 0;
      for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
        XOTclObject *inst = (XOTclObject*)Tcl_GetHashKey(&cl->instances, hPtr);
        /*fprintf(stderr, "    inst %p %s flags %.6x id %p baseClass %p %s\n", 
	  inst, objectName(inst), inst->flags, inst->id,baseClass,className(baseClass));*/
        if (inst && inst != (XOTclObject*)cl && !(inst->flags & XOTCL_DURING_DELETE) /*inst->id*/) {
          if (inst != &(baseClass->object)) {
            (void)RemoveInstance(inst, cl->object.cl);
            AddInstance(inst, baseClass);
          }
        }
      }
    }
    Tcl_DeleteHashTable(&cl->instances);
    MEM_COUNT_FREE("Tcl_InitHashTable", &cl->instances);
  }

  if ((clopt) && (!recreate)) {
    FREE(XOTclClassOpt, clopt);
    clopt = cl->opt = 0;
  }

  /* 
   * On a recreate, it might be possible that the newly created class
   * has a different superclass. So we have to flush the precedence
   * list on a recreate as well.
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
      /* 
       * If there are no more super classes add the Object
       * class as superclasses
       * -> don't do that for Object itself!
       */
      if (subClass->super == 0 && (cl->object.flags & XOTCL_IS_ROOT_CLASS) == 0) {
	/* fprintf(stderr,"subClass %p %s baseClass %p %s\n",
	   cl,className(cl),baseClass,className(baseClass)); */
	AddSuper(subClass, baseClass);
      }
    }
    /*(void)RemoveSuper(cl, cl->super->cl);*/
  }

}

/*
 * do class initialization & namespace creation
 */
static void
CleanupInitClass(Tcl_Interp *interp, XOTclClass *cl, Tcl_Namespace *nsPtr,
                 int softrecreate, int recreate) {
  XOTclClass *defaultSuperclass;

  assert(softrecreate ? recreate == 1 : 1);

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "+++ CleanupInitClass\n");
#endif

  /*
   * During init of Object and Class the theClass value is not set
   */
  /*
    if (RUNTIME_STATE(interp)->theClass != 0)
    obj->type = RUNTIME_STATE(interp)->theClass;
  */
  XOTclObjectSetClass((XOTclObject*)cl);

  cl->nsPtr = nsPtr;

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
  if (cl != defaultSuperclass) {
    AddSuper(cl, defaultSuperclass);
  }
  
  cl->color = WHITE;
  cl->order = NULL;

  if (!softrecreate) {
    Tcl_InitHashTable(&cl->instances, TCL_ONE_WORD_KEYS);
    MEM_COUNT_ALLOC("Tcl_InitHashTable", &cl->instances);
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
  XOTclObject *object = (XOTclObject*)clientData;
  Tcl_Interp *interp;
  Tcl_Namespace *saved;

  PRINTOBJ("PrimitiveCDestroy", object);

  /*
   * check and latch against recurrent calls with obj->teardown
   */
  if (!object || !object->teardown) return;
  interp = object->teardown;

  /*
   * Don't destroy, if the interpreted is destroyed already
   * e.g. TK calls Tcl_DeleteInterp directly, if Window is killed
   */
  if (Tcl_InterpDeleted(interp)) return;

  /*
   * call and latch user destroy with object->id if we haven't
   */
  /*fprintf(stderr, "PrimitiveCDestroy %s flags %.6x\n", objectName(object), object->flags);*/

  object->teardown = NULL;
  CleanupDestroyClass(interp, cl, 0, 0);

  /*
   * handoff the primitive teardown
   */

  saved = cl->nsPtr;
  object->teardown = interp;

  /*
   * class object destroy + physical destroy
   */
  /*fprintf(stderr, "primitive cdestroy %p %.6x calls primitive odestroy\n", cl, flags);*/
  PrimitiveODestroy(clientData);

  /*fprintf(stderr, "primitive cdestroy calls deletenamespace for obj %p, nsPtr %p flags %.6x\n", 
    cl, saved, ((Namespace *)saved)->flags);*/
  saved->clientData = NULL;
  XOTcl_DeleteNamespace(interp, saved);
  /*fprintf(stderr, "primitive cdestroy %p DONE\n",cl);*/
  return;
}

/*
 * class init
 */
static void
PrimitiveCInit(XOTclClass *cl, Tcl_Interp *interp, CONST char *name) {
  Tcl_CallFrame frame, *framePtr = &frame;
  Tcl_Namespace *nsPtr;

  /*
   * ensure that namespace is newly created during CleanupInitClass
   * ie. kill it, if it exists already
   */
  if (Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr,
                        RUNTIME_STATE(interp)->XOTclClassesNS, 0) != TCL_OK) {
    return;
  }
  nsPtr = NSGetFreshNamespace(interp, (ClientData)cl, name, 1);
  Tcl_PopCallFrame(interp);

  CleanupInitClass(interp, cl, nsPtr, 0, 0);
  return;
}

/*
 * class create: creation of namespace + class full name
 * calls class object creation
 */
static XOTclClass*
PrimitiveCCreate(Tcl_Interp *interp, Tcl_Obj *nameObj, XOTclClass *class) {
  XOTclClass *cl = (XOTclClass*)ckalloc(sizeof(XOTclClass));
  CONST char *nameString = ObjStr(nameObj);
  size_t length;
  XOTclObject *object = (XOTclObject*)cl;

  /*fprintf(stderr, "CKALLOC Class %p %s\n", cl, nameString);*/

  memset(cl, 0, sizeof(XOTclClass));
  MEM_COUNT_ALLOC("XOTclObject/XOTclClass", cl);

  /* pass object system from meta class */
  if (class) {
    cl->osPtr = class->osPtr;
  }

  assert(isAbsolutePath(nameString));
  length = strlen(nameString);
  /*
    fprintf(stderr, "Class alloc %p '%s'\n", cl, nameString);
  */
  /* check whether Object parent NS already exists,
     otherwise: error */
  if (!NSCheckForParent(interp, nameString, length, class)) {
    ckfree((char *) cl);
    return 0;
  }
  object->id = Tcl_CreateObjCommand(interp, nameString, XOTclObjDispatch,
                                 (ClientData)cl, tclDeletesObject);
  /*fprintf(stderr, "cmd alloc %p %d (%s) cl\n", object->id, Tcl_Command_refCount(object->id), nameString);*/

  PrimitiveOInit(object, interp, nameString, class);
  object->cmdName = nameObj;

  /* convert cmdName to Tcl Obj of type cmdName */
  /* Tcl_GetCommandFromObj(interp, obj->cmdName);*/

  INCR_REF_COUNT(object->cmdName);
  PrimitiveCInit(cl, interp, nameString+2);

  objTrace("PrimitiveCCreate", object);
  return cl;
}

/* change XOTcl class conditionally; obj must not be NULL */

XOTCLINLINE static int
changeClass(Tcl_Interp *interp, XOTclObject *object, XOTclClass *cl) {
  assert(object);

  /*fprintf(stderr, "changing %s to class %s ismeta %d\n",
          objectName(object),
          className(cl),
          IsMetaClass(interp, cl, 1));*/

  if (cl != object->cl) {
    if (IsMetaClass(interp, cl, 1)) {
      /* Do not allow upgrading from a class to a meta-class (in
	 other words, don't make an object to a class). To allow
	 this, it would be necessary to reallocate the base
	 structures.
      */
      if (!IsMetaClass(interp, object->cl, 1)) {
	return XOTclVarErrMsg(interp, "cannot turn object into a class",
			      (char *) NULL);
      }
    } else {
      /* The target class is not a meta class. Changing meta-class to
	 meta-class, or class to class, or object to object is fine,
	 but upgrading/downgrading is not allowed */

      /*fprintf(stderr, "target class %s not a meta class, am i a class %d\n",
	className(cl),
	XOTclObjectIsClass(object) );*/

      if (XOTclObjectIsClass(object)) {
	return XOTclVarErrMsg(interp, "cannot turn class into an object ",
			      (char *) NULL);
      }
    }
    (void)RemoveInstance(object, object->cl);
    AddInstance(object, cl);

    MixinComputeDefined(interp, object);
    FilterComputeDefined(interp, object);
  }
  return TCL_OK;
}


/*
 * Std object initialization:
 *   call parameter default values
 *   apply "-" methods (call "configure" with given arguments)
 *   call constructor "init", if it was not called before
 */
static int
doObjInitialization(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj *methodObj, *savedObjResult = Tcl_GetObjResult(interp); /* save the result */
  int result;

  INCR_REF_COUNT(savedObjResult);
  /*
   * clear INIT_CALLED flag
   */
  object->flags &= ~XOTCL_INIT_CALLED;

  /*
   * call configure methods (starting with '-')
   */
  if (CallDirectly(interp, object, XO_o_configure_idx, &methodObj)) {
    ALLOC_ON_STACK(Tcl_Obj*, objc, tov);
    memcpy(tov+1, objv+2, sizeof(Tcl_Obj *)*(objc-2));
    /* the provided name of the method is just for error reporting */
    tov[0] = methodObj ? methodObj : XOTclGlobalObjs[XOTE_CONFIGURE];
    result = XOTclOConfigureMethod(interp, object, objc-1, tov);
    FREE_ON_STACK(Tcl_Obj*, tov);
  } else {
    result = callMethod((ClientData) object, interp, methodObj, objc, objv+2, 0);
  }

  if (result != TCL_OK) {
    goto objinitexit;
  }

  /*
   * check, whether init was called already
   */
  if (!(object->flags & XOTCL_INIT_CALLED)) {
    int nobjc = 0;
    Tcl_Obj **nobjv, *resultObj = Tcl_GetObjResult(interp);

    /*
     * Call the scripted constructor and pass the result of
     * configure to it as arguments
     */
    INCR_REF_COUNT(resultObj);
    Tcl_ListObjGetElements(interp, resultObj, &nobjc, &nobjv);
    /* CallDirectly does not make much sense, since init is already
       defined in predefined */
    methodObj = XOTclMethodObj(interp, object, XO_o_init_idx);
    if (methodObj) {
      result = callMethod((ClientData) object, interp, methodObj,
			  nobjc+2, nobjv, XOTCL_CM_NO_PROTECT);
    }
    object->flags |= XOTCL_INIT_CALLED;
    DECR_REF_COUNT(resultObj);
  }

  if (result == TCL_OK) {
    Tcl_SetObjResult(interp, savedObjResult);
  }
 objinitexit:
  DECR_REF_COUNT(savedObjResult);
  return result;
}


static int
hasMetaProperty(Tcl_Interp *interp, XOTclClass *cl) {
  return cl->object.flags & XOTCL_IS_ROOT_META_CLASS;
}

static int
IsBaseClass(XOTclClass *cl) {
  return cl->object.flags & (XOTCL_IS_ROOT_META_CLASS|XOTCL_IS_ROOT_CLASS);
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
      if (clopt && clopt->classmixins) {
	MixinComputeOrderFullList(interp,
				  &clopt->classmixins,
				  &mixinClasses,
				  &checkList, 0);
      }
    }

    for (mc=mixinClasses; mc; mc = mc->nextPtr) {
      if (IsMetaClass(interp, mc->cl, 0)) {
	hasMCM = 1;
	break;
      }
    }
    XOTclClassListFree(mixinClasses);
    XOTclClassListFree(checkList);
    /*fprintf(stderr, "has MC returns %d, mixinClasses = %p\n",
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
hasMixin(Tcl_Interp *interp, XOTclObject *object, XOTclClass *cl) {

  if (!(object->flags & XOTCL_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, object);

  if ((object->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID)) {
    XOTclCmdList *ml;
    for (ml = object->mixinOrder; ml; ml = ml->nextPtr) {
      XOTclClass *mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin == cl) {
        return 1;
      }
    }
  }
  return 0;
}


extern int
XOTclCreateObject(Tcl_Interp *interp, Tcl_Obj *nameObj, XOTcl_Class *class) {
  XOTclClass *cl = (XOTclClass*) class;
  Tcl_Obj *methodObj;
  int result;

  INCR_REF_COUNT(nameObj);

  if (CallDirectly(interp, &cl->object, XO_c_create_idx, &methodObj)) {
    result = XOTclCCreateMethod(interp, cl, ObjStr(nameObj), 1, &nameObj);
  } else {
    result = XOTclCallMethodWithArgs((ClientData)cl, interp, methodObj, 
                                     nameObj, 1, 0, 0);  
  }
  DECR_REF_COUNT(nameObj);
  return result;
}

extern int
XOTclCreate(Tcl_Interp *interp, XOTcl_Class *class, Tcl_Obj *nameObj, ClientData clientData,
            int objc, Tcl_Obj *CONST objv[]) {
  XOTclClass *cl = (XOTclClass *) class;
  int result;
  ALLOC_ON_STACK(Tcl_Obj *, objc+2, ov);

  INCR_REF_COUNT(nameObj);

  ov[0] = NULL;
  ov[1] = nameObj;
  if (objc>0) {
    memcpy(ov+2, objv, sizeof(Tcl_Obj *)*objc);
  }
  result = XOTclCCreateMethod(interp, cl, ObjStr(nameObj), objc+2, ov);

  FREE_ON_STACK(Tcl_Obj*, ov);
  DECR_REF_COUNT(nameObj);

  return result;
}

int
XOTclDeleteObject(Tcl_Interp *interp, XOTcl_Object *object1) {
  XOTclObject *object = (XOTclObject *) object1;
  return callDestroyMethod(interp, object, 0);
}

extern int
XOTclUnsetInstVar2(XOTcl_Object *object1, Tcl_Interp *interp, 
                   CONST char *name1, CONST char *name2,
                   int flgs) {
  XOTclObject *object = (XOTclObject *) object1;
  int result;
  Tcl_CallFrame frame, *framePtr = &frame;

  XOTcl_PushFrameObj(interp, object, framePtr);
  if (object->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_UnsetVar2(interp, name1, name2, flgs);
  XOTcl_PopFrameObj(interp, framePtr);
  return result;
}

static int
GetInstVarIntoCurrentScope(Tcl_Interp *interp, const char *cmdName, XOTclObject *object, 
                           Tcl_Obj *varName, Tcl_Obj *newName) {
  Var *varPtr = NULL, *otherPtr = NULL, *arrayPtr;
  int new = 0, flgs = TCL_LEAVE_ERR_MSG;
  Tcl_CallFrame *varFramePtr;
  Tcl_CallFrame frame, *framePtr = &frame;
  char *varNameString;

  if (CheckVarName(interp, ObjStr(varName)) != TCL_OK) {
    return TCL_ERROR;
  }

  XOTcl_PushFrameObj(interp, object, framePtr);
  if (object->nsPtr) {
    flgs = flgs|TCL_NAMESPACE_ONLY;
  }

  otherPtr = TclObjLookupVar(interp, varName, NULL, flgs, "define",
                             /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
  XOTcl_PopFrameObj(interp, framePtr);

  if (otherPtr == NULL) {
    return XOTclVarErrMsg(interp, "can't import variable ", ObjStr(varName),
                          " into method scope: can't find variable on ", objectName(object),
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
      return XOTclVarErrMsg(interp, "can't make instance variable ", ObjStr(varName),
                            " on ", objectName(object),
                            ": Variable cannot be an element in an array;",
                            " use e.g. an alias.", (char *) NULL);
    }

    newName = varName;
  }
  varNameString = ObjStr(newName);



  varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  /*
   * If we are executing inside a Tcl procedure, create a local
   * variable linked to the new namespace variable "varName".
   */
  if (varFramePtr && (Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_PROC)) {
    varPtr = (Var *)CompiledLocalsLookup((CallFrame *)varFramePtr, varNameString);

    if (varPtr == NULL) {	
      /* look in frame's local var hashtable */
      TclVarHashTable *varTablePtr = Tcl_CallFrame_varTablePtr(varFramePtr);

      if (varTablePtr == NULL) {
        /*
         * The variable table does not exist. This seems to be is the
         * first access to a variable on this frame. We create the and
         * initialize the variable hash table and update the object
         */
        /*fprintf(stderr, "+++ create varTable in GetInstVarIntoCurrentScope\n");*/
        Tcl_CallFrame_varTablePtr(varFramePtr) = varTablePtr = VarHashTableCreate();
      }
      varPtr = VarHashCreateVar(varTablePtr, newName, &new);
    }

    /*
     * if we define an alias (newName != varName), be sure that
     * the target does not exist already
     */
    if (!new) {
      /*fprintf(stderr, "GetIntoScope createalias\n");*/
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
          Tcl_Panic("new linkvar %s... When does this happen?", ObjStr(newName), NULL);*/

        /* We have already a variable with the same name imported
           from a different object. Get rid of this old variable
        */
        VarHashRefCount(linkPtr)--;
        if (TclIsVarUndefined(linkPtr)) {
          TclCleanupVar(linkPtr, (Var *) NULL);
        }

      } else if (!TclIsVarUndefined(varPtr)) {
        return XOTclVarErrMsg(interp, "variable '", varNameString,
                              "' exists already", (char *) NULL);
      } else if (TclIsVarTraced(varPtr)) {
        return XOTclVarErrMsg(interp, "variable '", varNameString,
                              "' has traces: can't use for instvar", (char *) NULL);
      }
    }

    TclSetVarLink(varPtr);
    TclClearVarUndefined(varPtr);
    varPtr->value.linkPtr = otherPtr;
    VarHashRefCount(otherPtr)++;

    /* fprintf(stderr, "defining an alias var='%s' in obj %s fwd %d flags %x isLink %d isTraced %d isUndefined %d\n",
            ObjStr(newName), objectName(object), 
            0,
            varPtr->flags,
            TclIsVarLink(varPtr), TclIsVarTraced(varPtr), TclIsVarUndefined(varPtr));
    */
  } else {
    return XOTclVarErrMsg(interp, cmdName, 
			  " cannot import variable '", varNameString, 
			  "' into method scope; not called from a method frame", (char *) NULL);
  }
  return TCL_OK;
}

extern int
XOTclRemoveObjectMethod(Tcl_Interp *interp, XOTcl_Object *object1, CONST char *methodName) {
  XOTclObject *object = (XOTclObject*) object1;

  AliasDelete(interp, object->cmdName, methodName, 1);

  if (object->opt)
    AssertionRemoveProc(object->opt->assertions, methodName);

  if (object->nsPtr) {
    int rc = NSDeleteCmd(interp, object->nsPtr, methodName);
    if (rc < 0)
      return XOTclVarErrMsg(interp, objectName(object), " cannot delete method '", methodName,
                            "' of object ", objectName(object), (char *) NULL);
  }
  return TCL_OK;
}

extern int
XOTclRemoveClassMethod(Tcl_Interp *interp, XOTcl_Class *class, CONST char *methodName) {
  XOTclClass *cl = (XOTclClass*) class;
  XOTclClassOpt *opt = cl->opt;
  int rc;

  AliasDelete(interp, class->object.cmdName, methodName, 0);
  
  if (opt && opt->assertions)
    AssertionRemoveProc(opt->assertions, methodName);

  rc = NSDeleteCmd(interp, cl->nsPtr, methodName);
  if (rc < 0)
    return XOTclVarErrMsg(interp, className(cl), " cannot delete method '", methodName,
                          "' of class ", className(cl), (char *) NULL);
  return TCL_OK;
}

/*
 * obj/cl ClientData setter/getter
 */
extern void
XOTclSetObjClientData(XOTcl_Object *object1, ClientData data) {
  XOTclObject *object = (XOTclObject*) object1;
  XOTclObjectOpt *opt = XOTclRequireObjectOpt(object);
  opt->clientData = data;
}
extern ClientData
XOTclGetObjClientData(XOTcl_Object *object1) {
  XOTclObject *object = (XOTclObject*) object1;
  return (object && object->opt) ? object->opt->clientData : 0;
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
setInstVar(Tcl_Interp *interp, XOTclObject *object, Tcl_Obj *nameObj, Tcl_Obj *valueObj) {
  Tcl_Obj *result;
  int flags = (object->nsPtr) ? TCL_LEAVE_ERR_MSG|TCL_NAMESPACE_ONLY : TCL_LEAVE_ERR_MSG;
  Tcl_CallFrame frame, *framePtr = &frame;
  XOTcl_PushFrameObj(interp, object, framePtr);

  if (valueObj == NULL) {
    result = Tcl_ObjGetVar2(interp, nameObj, NULL, flags);
  } else {
    /*fprintf(stderr, "setvar in obj %s: name %s = %s\n", objectName(object), ObjStr(nameObj), ObjStr(value));*/
    result = Tcl_ObjSetVar2(interp, nameObj, NULL, valueObj, flags);
  }
  XOTcl_PopFrameObj(interp, framePtr);

  if (result) {
    Tcl_SetObjResult(interp, result);
    return TCL_OK;
  }
  return TCL_ERROR;
}

static int
XOTclSetterMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  SetterCmdClientData *cd = (SetterCmdClientData*)clientData;
  XOTclObject *object = cd->object;
  
  if (!object) return XOTclObjErrType(interp, objv[0], "object", ObjStr(objv[0]));
  if (objc > 2) return XOTclObjErrArgCnt(interp, object->cmdName, objv[0], "?value?");

  if (cd->paramsPtr && objc == 2) {
    Tcl_Obj *outObjPtr;
    int result, flags = 0;
    ClientData checkedData;

    result = ArgumentCheck(interp, objv[1], cd->paramsPtr, &flags, &checkedData, &outObjPtr);

    if (result == TCL_OK) {
      result = setInstVar(interp, object, objv[0], outObjPtr);

      if (flags & XOTCL_PC_MUST_DECR) {
        DECR_REF_COUNT(outObjPtr);
      }
    }
    return result;

  } else {
    return setInstVar(interp, object, objv[0], objc == 2 ? objv[1] : NULL);
  }
}

static int
forwardArg(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
           Tcl_Obj *forwardArgObj, ForwardCmdClientData *tcd, Tcl_Obj **out,
           Tcl_Obj **freeList, int *inputArg, int *mapvalue,
           int firstPosArg, int *outputincr) {
  CONST char *forwardArgString = ObjStr(forwardArgObj), *p;
  int totalargs = objc + tcd->nr_args - 1;
  char c = *forwardArgString, c1;
  
  /* per default every forwardArgString from the processed list corresponds to exactly
     one forwardArgString in the computed final list */
  *outputincr = 1; 
  p = forwardArgString;

  /*fprintf(stderr, "ForwardArg: processing '%s'\n", forwardArgString);*/

  if (c == '%' && *(forwardArgString+1) == '@') {
    char *remainder = NULL;
    long pos;
    forwardArgString += 2;
    pos = strtol(forwardArgString, &remainder, 0);
    /*fprintf(stderr, "strtol('%s) returned %ld '%s'\n", forwardArgString, pos, remainder);*/
    if (forwardArgString == remainder && *forwardArgString == 'e' 
        && !strncmp(forwardArgString, "end", 3)) {
      pos = -1;
      remainder += 3;
    } else if (pos < 0) {
      pos --;
    }
    if (forwardArgString == remainder || abs(pos) > totalargs) {
      return XOTclVarErrMsg(interp, "forward: invalid index specified in argument ",
                            ObjStr(forwardArgObj), (char *) NULL);
    }    if (!remainder || *remainder != ' ') {
      return XOTclVarErrMsg(interp, "forward: invaild syntax in '", ObjStr(forwardArgObj),
                            "' use: %@<pos> <cmd>", (char *) NULL);
    }

    forwardArgString = ++remainder;
    /* in case we address from the end, we reduct further to distinguish from -1 (void) */
    if (pos<0) pos--;
    /*fprintf(stderr, "remainder = '%s' pos = %ld\n", remainder, pos);*/
    *mapvalue = pos;
    forwardArgString = remainder;
    c = *forwardArgString;
  }

  if (c == '%') {
    Tcl_Obj *list = NULL, **listElements;
    int nrArgs = objc-1, nrPosArgs = objc-firstPosArg, nrElements = 0;
    char *firstActualArgument = nrArgs>0 ? ObjStr(objv[1]) : NULL;
    c = *++forwardArgString;
    c1 = *(forwardArgString+1);

    if (c == 's' && !strcmp(forwardArgString, "self")) {
      *out = tcd->object->cmdName;
    } else if (c == 'p' && !strcmp(forwardArgString, "proc")) {
      CONST char *methodName = ObjStr(objv[0]);
      /* if we dispatch a method via ".", we do not want to see the
	 "." in the %proc, e.g. for the interceptor slots (such as
	 .mixin, ... */
      if (FOR_COLON_RESOLVER(methodName)) {
	*out = Tcl_NewStringObj(methodName + 1, -1);
      } else {
	*out = objv[0];
      }
    } else if (c == '1' && (c1 == '\0' || c1 == ' ')) {

      if (c1 != '\0') {
        if (Tcl_ListObjIndex(interp, forwardArgObj, 1, &list) != TCL_OK) {
          return XOTclVarErrMsg(interp, "forward: %1 must be followed by a valid list, given: '",
                                ObjStr(forwardArgObj), "'", (char *) NULL);
        }
        if (Tcl_ListObjGetElements(interp, list, &nrElements, &listElements) != TCL_OK) {
          return XOTclVarErrMsg(interp, "forward: %1 contains invalid list '",
                                ObjStr(list), "'", (char *) NULL);
        }
      } else if (tcd->subcommands) { /* deprecated part */
        if (Tcl_ListObjGetElements(interp, tcd->subcommands, &nrElements, &listElements) != TCL_OK) {
          return XOTclVarErrMsg(interp, "forward: %1 contains invalid list '",
                                ObjStr(list), "'", (char *) NULL);
        }
      }
      /*fprintf(stderr, "nrElements=%d, nra=%d firstPos %d objc %d\n",
        nrElements, nrArgs, firstPosArg, objc);*/

      if (nrElements > nrPosArgs) {
        /* insert default subcommand depending on number of arguments */
        /*fprintf(stderr, "inserting listElements[%d] '%s'\n", nrPosArgs, 
          ObjStr(listElements[nrPosArgs]));*/
        *out = listElements[nrPosArgs];
      } else if (objc<=1) {
	return XOTclObjErrArgCnt(interp, objv[0], NULL, "option");
      } else {
        /*fprintf(stderr, "copying %%1: '%s'\n", ObjStr(objv[firstPosArg]));*/
        *out = objv[firstPosArg];
        *inputArg = firstPosArg+1;
      }
    } else if (c == '-') {
      CONST char *firstElementString;
      int i, insertRequired, done = 0;

      /*fprintf(stderr, "process flag '%s'\n", firstActualArgument);*/
      if (Tcl_ListObjGetElements(interp, forwardArgObj, &nrElements, &listElements) != TCL_OK) {
        return XOTclVarErrMsg(interp, "forward: '", forwardArgString, "' is not a valid list",
                              (char *) NULL);
      }
      if (nrElements < 1 || nrElements > 2) {
        return XOTclVarErrMsg(interp, "forward: '", forwardArgString, 
                              "' must contain 1 or 2 arguments",
                              (char *) NULL);
      }
      firstElementString = ObjStr(listElements[0]);
      firstElementString++; /* we skip the dash */

      if (firstActualArgument && *firstActualArgument == '-') {
        /*fprintf(stderr, "we have a flag in first argument '%s'\n", firstActualArgument);*/
        
        for (i = 1; i < firstPosArg; i++) {
          if (strcmp(firstElementString, ObjStr(objv[i])) == 0) {
            /*fprintf(stderr, "We have a MATCH for '%s' oldInputArg %d\n", forwardArgString, *inputArg);*/
            *out = objv[i];
            /* %1 will start at a different place. Proceed if necessary to firstPosArg */
            if (*inputArg < firstPosArg) {
              *inputArg = firstPosArg;
            }
            done = 1;
            break;
          }
        } 
      }

      if (!done) {
        /* We have a flag in the actual arguments that does not match.
         * We proceed to the actual arguments without dashes.
         */        
        if (*inputArg < firstPosArg) {
          *inputArg = firstPosArg;
        }
        /* 
         * If the user requested we output the argument also when not
         * given in the argument list.
         */
        if (nrElements == 2 
            && Tcl_GetIntFromObj(interp, listElements[1], &insertRequired) == TCL_OK
            && insertRequired) {
          /* no match, but insert of flag is required */
          /*fprintf(stderr, "no match, but insert of %s required\n", firstElementString);*/
          *out = Tcl_NewStringObj(firstElementString, -1);
          *outputincr = 1;
          goto add_to_freelist;
        } else {
          /* no match, no insert of flag required, we skip the
           * forwarder option and output nothing
           */
          /*fprintf(stderr, "no match, nrElements %d insert req %d\n", nrElements, insertRequired);*/
          *outputincr = 0;
        }
      }

    } else if (c == 'a' && !strncmp(forwardArgString, "argcl", 4)) {
      if (Tcl_ListObjIndex(interp, forwardArgObj, 1, &list) != TCL_OK) {
        return XOTclVarErrMsg(interp, "forward: %argclindex must by a valid list, given: '",
                              forwardArgString, "'", (char *) NULL);
      }
      if (Tcl_ListObjGetElements(interp, list, &nrElements, &listElements) != TCL_OK) {
        return XOTclVarErrMsg(interp, "forward: %argclindex contains invalid list '",
                              ObjStr(list), "'", (char *) NULL);
      }
      if (nrArgs >= nrElements) {
        return XOTclVarErrMsg(interp, "forward: not enough elements in specified list of ARGC argument ",
                              forwardArgString, (char *) NULL);
      }
      *out = listElements[nrArgs];
    } else if (c == '%') {
      Tcl_Obj *newarg = Tcl_NewStringObj(forwardArgString, -1);
      *out = newarg;
      goto add_to_freelist;
    } else {
      /* evaluating given command */
      int result;
      /*fprintf(stderr, "evaluating '%s'\n", forwardArgString);*/
      if ((result = Tcl_EvalEx(interp, forwardArgString, -1, 0)) != TCL_OK)
        return result;
      *out = Tcl_DuplicateObj(Tcl_GetObjResult(interp));
      /*fprintf(stderr, "result = '%s'\n", ObjStr(*out));*/
      goto add_to_freelist;
    }
  } else {
    if (p == forwardArgString)
      *out = forwardArgObj;
    else {
      Tcl_Obj *newarg = Tcl_NewStringObj(forwardArgString, -1);
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
callForwarder(ForwardCmdClientData *tcd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ClientData clientData;
  int result;
  XOTclObject *object = tcd->object;
  Tcl_CallFrame frame, *framePtr = &frame;

  if (tcd->verbose) {
    Tcl_Obj *cmd = Tcl_NewListObj(objc, objv);
    fprintf(stderr, "forwarder calls '%s'\n", ObjStr(cmd));
    DECR_REF_COUNT(cmd);
  }
  if (tcd->objscope) {
    XOTcl_PushFrameObj(interp, object, framePtr);
  }
  if (tcd->objProc) {
#if 1 || !defined(NRE)
    result = (*tcd->objProc)(tcd->clientData, interp, objc, objv);
#else
    result = Tcl_NRCallObjProc(interp, tcd->objProc, tcd->clientData, objc, objv);
#endif
  } else if (IsXOTclTclObj(interp, tcd->cmdName, (XOTclObject**)&clientData)) {
    /*fprintf(stderr, "XOTcl object %s, objc=%d\n", ObjStr(tcd->cmdName), objc);*/
    result = XOTclObjDispatch(clientData, interp, objc, objv);
  } else {
    /*fprintf(stderr, "callForwarder: no XOTcl object %s\n", ObjStr(tcd->cmdName));*/
    result = Tcl_EvalObjv(interp, objc, objv, 0);
  }

  if (tcd->objscope) {
    XOTcl_PopFrameObj(interp, framePtr);
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
  ForwardCmdClientData *tcd = (ForwardCmdClientData *)clientData;
  int result, j, inputArg = 1, outputArg = 0;

  if (!tcd || !tcd->object) return XOTclObjErrType(interp, objv[0], "object", "");

  if (tcd->passthrough) { /* two short cuts for simple cases */
    /* early binding, cmd *resolved, we have to care only for objscope */
    return callForwarder(tcd, interp, objc, objv);
  } else if (!tcd->args && *(ObjStr(tcd->cmdName)) != '%') {
    /* we have ony to replace the method name with the given cmd name */
    ALLOC_ON_STACK(Tcl_Obj*, objc, ov);
    /*fprintf(stderr, "+++ forwardMethod must subst \n");*/
    memcpy(ov, objv, sizeof(Tcl_Obj *)*objc);
    ov[0] = tcd->cmdName;
    result = callForwarder(tcd, interp, objc, ov);
    FREE_ON_STACK(Tcl_Obj *, ov);
    return result;
  } else {
    Tcl_Obj **ov, *freeList=NULL;
    int outputincr, firstPosArg=1, totalargs = objc + tcd->nr_args + 3;
    ALLOC_ON_STACK(Tcl_Obj*, totalargs, OV);
    ALLOC_ON_STACK(int, totalargs, objvmap);
    /*fprintf(stderr, "+++ forwardMethod standard case, allocated %d args\n", totalargs);*/

    ov = &OV[1];
    if (tcd->needobjmap) {
      memset(objvmap, -1, sizeof(int)*totalargs);
    }

    /* the first argument is always the command, to which we forward */
    if ((result = forwardArg(interp, objc, objv, tcd->cmdName, tcd,
                             &ov[outputArg], &freeList, &inputArg,
                             &objvmap[outputArg], 
                             firstPosArg, &outputincr)) != TCL_OK) {
      goto exitforwardmethod;
    }
    outputArg += outputincr;

    /* if we have nonpos args, determine the first pos arg position for %1 */
    if (tcd->hasNonposArgs) {
      firstPosArg = objc;
      for (j=outputArg; j<objc; j++) {
        CONST char *arg = ObjStr(objv[j]);
        if (*arg != '-') {
          firstPosArg = j;
          break;
        }
      }
    }

    if (tcd->args) {
      /* copy argument list from definition */
      Tcl_Obj **listElements;
      int nrElements;
      Tcl_ListObjGetElements(interp, tcd->args, &nrElements, &listElements);

      for (j=0; j<nrElements; j++, outputArg += outputincr) {
        if ((result = forwardArg(interp, objc, objv, listElements[j], tcd,
                                 &ov[outputArg], &freeList, &inputArg,
                                 &objvmap[outputArg], 
                                 firstPosArg, &outputincr)) != TCL_OK) {
          goto exitforwardmethod;
        }
      }
    }

    /*fprintf(stderr, "objc=%d, tcd->nr_subcommands=%d size=%d\n",
      objc, tcd->nr_subcommands, objc+ 2	    );*/

    if (objc-inputArg>0) {
      /*fprintf(stderr, "  copying remaining %d args starting at [%d]\n",
        objc-inputArg, outputArg);*/
      memcpy(ov+outputArg, objv+inputArg, sizeof(Tcl_Obj *)*(objc-inputArg));
    } else {
      /*fprintf(stderr, "  nothing to copy, objc=%d, inputArg=%d\n", objc, inputArg);*/
    }
    if (tcd->needobjmap) {
      /* we have to set the adressing relative from the end; -2 means
	 last, -3 element before last, etc. */
      int max = objc + tcd->nr_args - inputArg;
      for (j=0; j<totalargs; j++) {
	if (objvmap[j] < -1) {
	  /*fprintf(stderr, "must reduct, v=%d\n", objvmap[j]);*/
	  objvmap[j] = max + objvmap[j] + 2;
	  /*fprintf(stderr, "... new value=%d, max = %d\n", objvmap[j], max);*/
	}
      }
    }
    objc += outputArg - inputArg;

#if 0
    for(j=0; j<objc; j++) {
      /*fprintf(stderr, "  ov[%d]=%p, objc=%d\n", j, ov[j], objc);*/
      fprintf(stderr, " o[%d]=%p %s (%d),", j, ov[j], ov[j] ? ObjStr(ov[j]) : "NADA", objvmap[j]);
    }
    fprintf(stderr, "\n");
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
            /*fprintf(stderr, "...moving right %d to %d\n", i-1, i);*/
            ov[i] = ov[i-1];
            objvmap[i] = objvmap[i-1];
          }
        } else {
          for(i=j; i<pos; i++) {
            /*fprintf(stderr, "...moving left %d to %d\n", i+1, i);*/
            ov[i] = ov[i+1];
            objvmap[i] = objvmap[i+1];
          }
        }
        /*fprintf(stderr, "...setting at %d -> %s\n", pos, ObjStr(tmp));*/
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
    FREE_ON_STACK(int,objvmap);
    FREE_ON_STACK(Tcl_Obj*,OV);
  }
  return result;
}

/*
 * copied from Tcl, since not exported
 */
static char *
VwaitVarProc(
             ClientData clientData,	/* Pointer to integer to set to 1. */
             Tcl_Interp *interp,	/* Interpreter containing variable. */
             char *name1,		/* Name of variable. */
             char *name2,		/* Second part of variable name. */
             int flags)			/* Information about what happened. */
{
  int *donePtr = (int *) clientData;

  *donePtr = 1;
  return (char *) NULL;
}

static int
XOTclProcAliasMethod(ClientData clientData, 
                     Tcl_Interp *interp, int objc, 
                     Tcl_Obj *CONST objv[]) {
  AliasCmdClientData *tcd = (AliasCmdClientData *)clientData;
  XOTclObject *self = GetSelfObj(interp);
  CONST char *methodName = ObjStr(objv[0]);

  if (self == NULL) {
    return XOTclVarErrMsg(interp, "no object active for alias '",
                          Tcl_GetCommandName(interp, tcd->aliasCmd), 
                          "'; don't call aliased methods via namespace paths", 
                          (char *) NULL);
  }
  return MethodDispatch((ClientData)self, interp, objc, objv, tcd->aliasedCmd, self, tcd->class,
                        methodName, 0);
}

static int
XOTclObjscopedMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  AliasCmdClientData *tcd = (AliasCmdClientData *)clientData;
  XOTclObject *object = tcd->object;
  Tcl_CallFrame frame, *framePtr = &frame;
  int result;

  /*fprintf(stderr, "objscopedMethod obj=%p %s, ptr=%p\n", object, objectName(object), tcd->objProc);*/

  XOTcl_PushFrameObj(interp, object, framePtr);

#if !defined(NRE)
  result = (*tcd->objProc)(tcd->clientData, interp, objc, objv);
#else
  result = Tcl_NRCallObjProc(interp, tcd->objProc, tcd->clientData, objc, objv);
#endif

  XOTcl_PopFrameObj(interp, framePtr);
  return result;
}

static void setterCmdDeleteProc(ClientData clientData) {
  SetterCmdClientData *setterClientData = (SetterCmdClientData *)clientData;

  if (setterClientData->paramsPtr) {
    ParamsFree(setterClientData->paramsPtr);
  }
  FREE(SetterCmdClientData, setterClientData);
}

static void aliasCmdDeleteProc(ClientData clientData) {
  AliasCmdClientData *tcd = (AliasCmdClientData *)clientData;
  ImportRef *refPtr, *prevPtr = NULL;

  /* 
   * Since we just get the clientData, we have to obtain interp,
   * object, methodName and per-object from tcd; the obj might be
   * deleted already. We need as well at least still the global
   * namespace.
   */
  if (tcd->interp && 
      ((Interp *)(tcd->interp))->globalNsPtr &&
      RUNTIME_STATE(tcd->interp)->exitHandlerDestroyRound != XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY) {
    CONST char *methodName = Tcl_GetCommandName(tcd->interp, tcd->aliasCmd);
    AliasDelete(tcd->interp, tcd->cmdName, methodName, tcd->class == NULL);
  }

  /*fprintf(stderr, "aliasCmdDeleteProc\n");*/
  if (tcd->cmdName)     {DECR_REF_COUNT(tcd->cmdName);}
  if (tcd->aliasedCmd) {
    Command *aliasedCmd = (Command *)(tcd->aliasedCmd);
    /* 
     * Clear the aliasCmd from the imported-ref chain of the aliased
     * (or real) cmd.  This widely resembles what happens in the
     * DeleteImportedCmd() (see tclNamesp.c), however, as we do not
     * provide for ImportedCmdData client data etc., we cannot
     * directly use it.
     */
    for (refPtr = aliasedCmd->importRefPtr; refPtr; refPtr = refPtr->nextPtr) {
      if (refPtr->importedCmdPtr == (Command *) tcd->aliasCmd) {
        if (prevPtr == NULL) {
          aliasedCmd->importRefPtr = refPtr->nextPtr;
        } else {
          prevPtr->nextPtr = refPtr->nextPtr;
        }
        ckfree((char *) refPtr);
        break;
      }
      prevPtr = refPtr;
    }
  }

  FREE(AliasCmdClientData, tcd);
}


typedef enum {NO_DASH, SKALAR_DASH, LIST_DASH} dashArgType;

static dashArgType
isDashArg(Tcl_Interp *interp, Tcl_Obj *obj, int firstArg, CONST char **methodName, int *objc, Tcl_Obj **objv[]) {
  CONST char *flag;
  assert(obj);

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
  if ((*flag == '-') && isalpha(*((flag)+1))) {
    if (firstArg) {
      /* if the argument contains a space, try to split */
      CONST char *p= flag+1;
      while (*p && *p != ' ') p++;
      if (*p == ' ') {
        if (Tcl_ListObjGetElements(interp, obj, objc, objv) == TCL_OK) {
          *methodName = ObjStr(*objv[0]);
          if (**methodName == '-') {(*methodName)++ ;}
          return LIST_DASH;
        }
      }
    }
    *methodName = flag+1;
    *objc = 1;
    return SKALAR_DASH;
  }
  return NO_DASH;
}

static int
callConfigureMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *methodName,
                    int argc, Tcl_Obj *CONST argv[]) {
  int result;
  Tcl_Obj *methodObj = Tcl_NewStringObj(methodName, -1);

  /* fprintf(stderr, "callConfigureMethod method %s->'%s' level %d, argc %d\n",
     objectName(object), methodName, level, argc);*/

  if (isInitString(methodName)) {
    object->flags |= XOTCL_INIT_CALLED;
  }

  Tcl_ResetResult(interp);
  INCR_REF_COUNT(methodObj);
  result = callMethod((ClientData)object, interp, methodObj, argc, argv, XOTCL_CM_NO_UNKNOWN);
  DECR_REF_COUNT(methodObj);

  /*fprintf(stderr, "method  '%s' called args: %d o=%p, result=%d %d\n",
    methodName, argc+1, obj, result, TCL_ERROR);*/

  if (result != TCL_OK) {
    Tcl_Obj *res =  Tcl_DuplicateObj(Tcl_GetObjResult(interp)); /* save the result */
    INCR_REF_COUNT(res);
    XOTclVarErrMsg(interp, ObjStr(res), " during '", objectName(object), " ",
		   methodName, "'", (char *) NULL);
    DECR_REF_COUNT(res);
  }

  return result;
}


/*
 * class method implementations
 */

static int isRootNamespace(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  XOTclObjectSystem *osPtr;

  for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = osPtr->nextPtr) {
    Tcl_Command cmd = osPtr->rootClass->object.id;
    if ((Tcl_Namespace *)((Command *)cmd)->nsPtr == nsPtr) {
      return 1;
    }
  }
  return 0;
}

static Tcl_Namespace *
callingNameSpace(Tcl_Interp *interp) {
  Tcl_CallFrame *framePtr;
  Tcl_Namespace *nsPtr;

  /*tcl85showStack(interp);*/

  /*
  * Find last incovation outside the XOTcl system namespaces. For
  * example, the pre defined slot handlers for relations (defined in
  * the too namespace) handle mixin and class registration. etc. If we
  * would use this namespace, we would resolve non-fully-qualified
  * names against the root namespace).
  */
  for (framePtr = activeProcFrame((Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp));
       framePtr;
       framePtr = Tcl_CallFrame_callerVarPtr(framePtr)) {
    nsPtr = Tcl_CallFrame_nsPtr(framePtr);
    if (isRootNamespace(interp, nsPtr)) {
      /*fprintf(stderr, "... %p skip %s\n", framePtr, nsPtr->fullName);*/
      continue;
    }
    /*fprintf(stderr, "... %p take %s\n", framePtr, nsPtr->fullName); */
    break;
  }

  if (!framePtr) {
    nsPtr = Tcl_GetGlobalNamespace(interp);
  }

  /*fprintf(stderr, " **** callingNameSpace: returns %p %s framePtr %p\n",
    nsPtr, nsPtr ? nsPtr->fullName:"(null)", framePtr);*/
  return nsPtr;
}

/***********************************
 * argument parser
 ***********************************/

#include "tclAPI.h"

static int
ArgumentError(Tcl_Interp *interp, CONST char *errorMsg, XOTclParam CONST *paramPtr,
              Tcl_Obj *cmdNameObj, Tcl_Obj *methodObj) {
  Tcl_Obj *argStringObj = ParamDefsSyntax(interp, paramPtr);

  XOTclObjWrongArgs(interp, errorMsg, cmdNameObj, methodObj, ObjStr(argStringObj));
  DECR_REF_COUNT(argStringObj);

  return TCL_ERROR;
}

static int 
ArgumentCheckHelper(Tcl_Interp *interp, Tcl_Obj *objPtr, struct XOTclParam CONST *pPtr, int *flags,
                        ClientData *clientData, Tcl_Obj **outObjPtr) {
  int objc, i, result;
  Tcl_Obj **ov;
  
  /*fprintf(stderr, "ArgumentCheckHelper\n");*/
  assert(pPtr->flags & XOTCL_ARG_MULTIVALUED);

  result = Tcl_ListObjGetElements(interp, objPtr, &objc, &ov);
  if (result != TCL_OK) {
    return result;
  }
  
  *outObjPtr = Tcl_NewListObj(0, NULL);
  INCR_REF_COUNT(*outObjPtr);
  
  for (i=0; i<objc; i++) {
    Tcl_Obj *elementObjPtr;
    const char *valueString = ObjStr(ov[i]);

    if (pPtr->flags & XOTCL_ARG_ALLOW_EMPTY && *valueString == '\0') {
      result = convertToString(interp, ov[i], pPtr, clientData, &elementObjPtr);
    } else {
      result = (*pPtr->converter)(interp, ov[i], pPtr, clientData, &elementObjPtr);
    }

    /*fprintf(stderr, "ArgumentCheckHelper convert %s result %d (%s)\n", 
      valueString, result, ObjStr(elementObjPtr));*/

    if (result == TCL_OK) {
      Tcl_ListObjAppendElement(interp, *outObjPtr, elementObjPtr);
    } else {
      Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(resultObj);
      XOTclVarErrMsg(interp, "invalid value in \"", ObjStr(objPtr), "\": ",
                     ObjStr(resultObj), (char *) NULL);
      DECR_REF_COUNT(resultObj);
      DECR_REF_COUNT(*outObjPtr);
      *flags &= ~XOTCL_PC_MUST_DECR;
      *outObjPtr = objPtr;
      break;
    }
  }
  return result;
}

static int
ArgumentCheck(Tcl_Interp *interp, Tcl_Obj *objPtr, struct XOTclParam CONST *pPtr, int *flags,
	      ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result;

  if (pPtr->flags & XOTCL_ARG_MULTIVALUED) {
    int objc, i;
    Tcl_Obj **ov;
    
    /*
     * In the multivalued case, we have either to check a list of
     * values or to build a new list of values (in case, the converter
     * normalizes the values).
     */
    result = Tcl_ListObjGetElements(interp, objPtr, &objc, &ov);
    if (result != TCL_OK) {
      return result;
    }

    /* 
     * Default assumption: outObjPtr is not modified, in cases where
     * necessary, we switch to the helper function
     */
    *outObjPtr = objPtr;

    for (i=0; i<objc; i++) {
      Tcl_Obj *elementObjPtr;
      const char *valueString = ObjStr(ov[i]);

      if (pPtr->flags & XOTCL_ARG_ALLOW_EMPTY && *valueString == '\0') {
	result = convertToString(interp, ov[i], pPtr, clientData, &elementObjPtr);
      } else {
	result = (*pPtr->converter)(interp, ov[i], pPtr, clientData, &elementObjPtr);
      }

      if (result == TCL_OK) {
        if (ov[i] != elementObjPtr) {
          /* 
             The elementObjPtr differs from the input tcl_obj, we
             switch to the version of this handler building an output
             list 
          */
          fprintf(stderr, "switch to output list construction for value %s\n",
		  ObjStr(elementObjPtr));
          *flags |= XOTCL_PC_MUST_DECR;
          result = ArgumentCheckHelper(interp, objPtr, pPtr, flags, clientData, outObjPtr);
          break;
        }
      } else {
        Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
        INCR_REF_COUNT(resultObj);
        XOTclVarErrMsg(interp, "invalid value in \"", ObjStr(objPtr), "\": ",
                       ObjStr(resultObj), (char *) NULL);
        DECR_REF_COUNT(resultObj);
        break;
      }
    }
  } else {
    const char *valueString = ObjStr(objPtr);
    if (pPtr->flags & XOTCL_ARG_ALLOW_EMPTY && *valueString == '\0') {
      result = convertToString(interp, objPtr, pPtr, clientData, outObjPtr);
    } else {
      result = (*pPtr->converter)(interp, objPtr, pPtr, clientData, outObjPtr);
    }
  }
  return result;
}

static int
ArgumentDefaults(parseContext *pcPtr, Tcl_Interp *interp,
                 XOTclParam CONST *ifd, int nrParams) {
  XOTclParam CONST *pPtr;
  int i;

  for (pPtr = ifd, i=0; i<nrParams; pPtr++, i++) {
    /*fprintf(stderr, "ArgumentDefaults got for arg %s (%d) %p => %p %p, default %s\n",
            pPtr->name, pPtr->flags & XOTCL_ARG_REQUIRED, pPtr,
            pcPtr->clientData[i], pcPtr->objv[i],
            pPtr->defaultValue ? ObjStr(pPtr->defaultValue) : "NONE");*/

    if (pcPtr->objv[i]) {
      /* we got an actual value, which was already checked by objv parser */
      /*fprintf(stderr, "setting passed value for %s to '%s'\n", pPtr->name, ObjStr(pcPtr->objv[i]));*/
      if (pPtr->converter == convertToSwitch) {
        int bool;
        Tcl_GetBooleanFromObj(interp, pPtr->defaultValue, &bool);
	pcPtr->objv[i] = Tcl_NewBooleanObj(!bool);
      }
    } else {
      /* no valued passed, check if default is available */

      if (pPtr->defaultValue) {
        int mustDecrNewValue;
        Tcl_Obj *newValue = pPtr->defaultValue;
        ClientData checkedData;
	
        /* we have a default, do we have to subst it? */
        if (pPtr->flags & XOTCL_ARG_SUBST_DEFAULT) {
          int result = SubstValue(interp, pcPtr->object, &newValue);
          if (result != TCL_OK) {
            return result;
          }
          /*fprintf(stderr, "attribute %s default %p %s => %p '%s'\n", pPtr->name,
                  pPtr->defaultValue, ObjStr(pPtr->defaultValue),
                  newValue, ObjStr(newValue));*/

          /* the according DECR is performed by parseContextRelease() */
          INCR_REF_COUNT(newValue);
          mustDecrNewValue = 1;
          pcPtr->flags[i] |= XOTCL_PC_MUST_DECR;
          pcPtr->mustDecr = 1;
        } else {
          mustDecrNewValue = 0;
        }

        pcPtr->objv[i] = newValue;
        /*fprintf(stderr, "==> setting default value '%s' for var '%s' flag %d type %s conv %p\n",
                ObjStr(newValue), pPtr->name, pPtr->flags & XOTCL_ARG_INITCMD,
                pPtr->type, pPtr->converter);*/

        /* Check the default value, unless we have an INITCMD or METHOD */
        if ((pPtr->flags & (XOTCL_ARG_INITCMD|XOTCL_ARG_METHOD)) == 0) {
          int mustDecrList = 0;
          if (ArgumentCheck(interp, newValue, pPtr, &mustDecrList, &checkedData, &pcPtr->objv[i]) != TCL_OK) {
            return TCL_ERROR;
          }
          
	  if (pcPtr->objv[i] != newValue) {
	    /* The output tcl_obj differs from the input, so the tcl_obj
	       was converted; in case we have set prevously must_decr
	       on newValue, we decr the refcount on newValue here and
	       clear the flag */
	    if (mustDecrNewValue) {
	      DECR_REF_COUNT(newValue);
	      pcPtr->flags[i] &= ~XOTCL_PC_MUST_DECR;
	    }
            /* the new output value itself might require a decr, so
               set the flag here if required; this is just necessary
               for multivalued converted output */
            if (mustDecrList) {
	      pcPtr->flags[i] |= XOTCL_PC_MUST_DECR;
              pcPtr->mustDecr = 1;
            }
	  }
        }
      } else if (pPtr->flags & XOTCL_ARG_REQUIRED) {
        return XOTclVarErrMsg(interp,
                              pcPtr->object ? objectName(pcPtr->object) : "", 
                              pcPtr->object ? " " : "",
                              ObjStr(pcPtr->full_objv[0]), ": required argument '",
                              pPtr->nameObj ? ObjStr(pPtr->nameObj) : pPtr->name,
                              "' is missing", (char *) NULL);
      } else {
        /* Use as dummy default value an arbitrary symbol, which must not be
         * returned to the Tcl level level; this value is
         * unset later by unsetUnknownArgs
         */
        pcPtr->objv[i] = XOTclGlobalObjs[XOTE___UNKNOWN__];
      }
    }
  }
  return TCL_OK;
}

static int
ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
              XOTclObject *object, Tcl_Obj *procNameObj,
              XOTclParam CONST *paramPtr, int nrParams,
              parseContext *pcPtr) {
  int i, o, flagCount, nrReq = 0, nrOpt = 0, dashdash = 0, nrDashdash = 0;
  XOTclParam CONST *pPtr;

  parseContextInit(pcPtr, nrParams, object, procNameObj);

#if defined(PARSE_TRACE)
  fprintf(stderr, "BEGIN (%d) [0]%s ", objc, ObjStr(procNameObj));
  for (o=1; o<objc; o++) {fprintf(stderr, "[%d]%s ", o, ObjStr(objv[o]));}
  fprintf(stderr, "\n");
#endif

  for (i = 0, o = 1, pPtr = paramPtr; pPtr->name && o < objc;) {
#if defined(PARSE_TRACE_FULL)
    fprintf(stderr, "... (%d) processing [%d]: '%s' %s\n", i, o,
            pPtr->name, pPtr->flags & XOTCL_ARG_REQUIRED ? "req":"not req");
#endif
    flagCount = 0;
    if (*pPtr->name == '-') {
      int p, found;
      CONST char *objStr;
      /*
       * We expect now a non-positional (named) parameter, starting
       * with a "-"; such arguments can be given in an arbitrary order
       */
      for (p = o; p<objc; p++) {
        objStr = ObjStr(objv[p]);
	found = 0;

        /*fprintf(stderr, "....checking objv[%d]=%s\n", p, objStr);*/
        if (objStr[0] != '-') {
	  /* there is no positional arg in the given argument vector */
	  break;
	} else {
          XOTclParam CONST *nppPtr;
	  /* We have an argument starting with a "-"; is it really one of the specified flags? */

          for (nppPtr = pPtr; nppPtr->name && *nppPtr->name == '-'; nppPtr ++) {
            if (strcmp(objStr, nppPtr->name) == 0) {
	      int j = nppPtr-paramPtr;
              /*fprintf(stderr, "...     flag '%s' o=%d p=%d, objc=%d nrArgs %d\n", objStr, o, p, objc, nppPtr->nrArgs);*/
              if (nppPtr->flags & XOTCL_ARG_REQUIRED) nrReq++; else nrOpt++;
              if (nppPtr->nrArgs == 0) {
                pcPtr->clientData[j] = (ClientData)1;  /* the flag was given */
                pcPtr->objv[j] = XOTclGlobalObjs[XOTE_ONE];
              } else {
                /* we assume for now, nrArgs is at most 1 */
                o++; p++;
                if (nppPtr->flags & XOTCL_ARG_REQUIRED) nrReq++; else nrOpt++;

                if (o < objc) {
#if defined(PARSE_TRACE_FULL)
		  fprintf(stderr, "...     setting cd[%d] '%s' = %s (%d) %s converter %p\n",
                          i, nppPtr->name, ObjStr(objv[p]), nppPtr->nrArgs,
                          nppPtr->flags & XOTCL_ARG_REQUIRED ? "req":"not req", nppPtr->converter);
#endif
                  if (ArgumentCheck(interp, objv[p], nppPtr, &pcPtr->flags[j],
				    &pcPtr->clientData[j], &pcPtr->objv[j]) != TCL_OK) {
                    return TCL_ERROR;
                  }

                  if (pcPtr->flags[j] & XOTCL_PC_MUST_DECR) 
                    pcPtr->mustDecr = 1;
		  
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
      /*fprintf(stderr, "... we found %d flags\n", flagCount);*/
      /* skip in parameter definition until the end of the switches */
      while (pPtr->name && *pPtr->name == '-') {pPtr++, i++;};
      /* under the assumption, flags have no arguments */
      o += flagCount;
      /*
       * check double dash --
       */
      if (o<objc) {
        objStr = ObjStr(objv[o]);
        if (*objStr == '-' && *(objStr+1) == '-' && *(objStr+2) == '\0' && dashdash == 0) {
#if defined(PARSE_TRACE_FULL)
          fprintf(stderr, "... skip double dash once\n");
#endif
          dashdash++;
          nrDashdash++;
          o++;
        }
      }
    } else {

      /* Handle positional (unnamed) parameters, starting without a
       * "-"; arguments must be always in same order
       */

      /* reset dashdash, if needed */
      if (dashdash) {dashdash = 0;}

      if (pPtr->flags & XOTCL_ARG_REQUIRED) nrReq++; else nrOpt++;
      /*fprintf(stderr, "... arg %s req %d converter %p try to set on %d: '%s' convertViaCmd %p\n",
              pPtr->name, pPtr->flags & XOTCL_ARG_REQUIRED, pPtr->converter, i, ObjStr(objv[o]), 
              convertViaCmd);*/
      if (ArgumentCheck(interp, objv[o], pPtr, &pcPtr->flags[i], &pcPtr->clientData[i], &pcPtr->objv[i]) != TCL_OK) {
        return TCL_ERROR;
      }
      if (pcPtr->flags[i] & XOTCL_PC_MUST_DECR) 
        pcPtr->mustDecr = 1;

      /*
       * objv is always passed via pcPtr->objv
       */
#if defined(PARSE_TRACE_FULL)
      fprintf(stderr, "...     setting %s pPtr->objv[%d] to [%d]'%s' converter %p\n", 
              pPtr->name, i, o, ObjStr(objv[o]), pPtr->converter);
#endif
      o++; i++; pPtr++;
    }
  }
  pcPtr->lastobjc = pPtr->name ? o : o-1;
  pcPtr->objc = i + 1;

  /* Process all args until end of parameter definitions to get correct counters */
  while (pPtr->name) {
    if (pPtr->flags & XOTCL_ARG_REQUIRED) nrReq++; else nrOpt++;
    pPtr++;
  }

  /* is last argument a vararg? */
  pPtr--;
  if (pPtr->converter == convertToNothing) {
    pcPtr->varArgs = 1;
    /*fprintf(stderr, "last arg of proc '%s' is varargs\n", ObjStr(procNameObj));*/
  }

  /* handle missing or unexpected arguments */
  if (pcPtr->lastobjc < nrReq) {
    return ArgumentError(interp, "not enough arguments:", paramPtr, NULL, procNameObj); /* for methods and cmds */
  }
  if (!pcPtr->varArgs && objc-nrDashdash-1 > nrReq + nrOpt) {
    return ArgumentError(interp, "too many arguments:", paramPtr, NULL, procNameObj); /* for methods and cmds */
  }

  return ArgumentDefaults(pcPtr, interp, paramPtr, nrParams);
}


/***********************************
 * Begin result setting commands
 * (essentially List*() and support
 ***********************************/
static int
ListVarKeys(Tcl_Interp *interp, Tcl_HashTable *tablePtr, CONST char *pattern) {
  Tcl_HashEntry *hPtr;

  if (pattern && noMetaChars(pattern)) {
    Tcl_Obj *patternObj = Tcl_NewStringObj(pattern, -1);
    INCR_REF_COUNT(patternObj);

    hPtr = tablePtr ? Tcl_CreateHashEntry(tablePtr, (char *)patternObj, NULL) : NULL;
    if (hPtr) {
      Var  *val = VarHashGetValue(hPtr);
      Tcl_SetObjResult(interp, VarHashGetKey(val));
    } else {
      Tcl_SetObjResult(interp, XOTclGlobalObjs[XOTE_EMPTY]);
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

static Tcl_Command
GetOriginalCommand(Tcl_Command cmd) /* The imported command for which the original
                                     * command should be returned. */
{
  Tcl_Command importedCmd;
  
  while (1) {
    /* dereference the namespace import reference chain */
    if ((importedCmd = TclGetOriginalCommand(cmd))) {
      cmd = importedCmd;
    }
    /* dereference the XOtcl alias chain */
    if (Tcl_Command_deleteProc(cmd) == aliasCmdDeleteProc) {
      AliasCmdClientData *tcd = (AliasCmdClientData *)Tcl_Command_objClientData(cmd);
      cmd = tcd->aliasedCmd;
      continue;
    } 
    break;
  }
  return cmd;
}

static int
ListProcBody(Tcl_Interp *interp, Proc *procPtr, CONST char *methodName) {
  if (procPtr) {
    CONST char *body = ObjStr(procPtr->bodyPtr);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(StripBodyPrefix(body), -1));
    return TCL_OK;
  }
  return XOTclErrBadVal(interp, "info body", "a tcl method name", methodName);
}

static Tcl_Obj*
ListParamDefs(Tcl_Interp *interp, XOTclParam CONST *paramsPtr, int style) {
  Tcl_Obj *listObj;

  switch (style) {
  case 0: listObj = ParamDefsFormat(interp, paramsPtr); break;
  case 1: listObj = ParamDefsList(interp, paramsPtr); break;
  case 2: listObj = ParamDefsSyntax(interp, paramsPtr); break;
  }

  return listObj;
}

static int
ListCmdParams(Tcl_Interp *interp, Tcl_Command cmd, CONST char *methodName, int withVarnames) {
  Proc *procPtr = GetTclProcFromCommand(cmd);
  if (procPtr) {
    XOTclParamDefs *paramDefs = procPtr ? ParamDefsGet((Tcl_Command)procPtr->cmdPtr) : NULL;
    Tcl_Obj *list;

    if (paramDefs) {
      /*
       * Obtain parameter info from paramDefs
       */
      list = ListParamDefs(interp, paramDefs->paramsPtr, withVarnames);
      
    } else {
      /*
       * Obtain parameter info from compiled locals
       */
      CompiledLocal *args = procPtr->firstLocalPtr;

      list = Tcl_NewListObj(0, NULL);
      for ( ; args; args = args->nextPtr) {
        Tcl_Obj *innerlist;

        if (!TclIsCompiledLocalArgument(args)) {
          continue;
        }

        innerlist = Tcl_NewListObj(0, NULL);
        Tcl_ListObjAppendElement(interp, innerlist, Tcl_NewStringObj(args->name, -1));
        if (!withVarnames && args->defValuePtr) {
          Tcl_ListObjAppendElement(interp, innerlist, args->defValuePtr);
        }
        Tcl_ListObjAppendElement(interp, list, innerlist);
      }
    }

    Tcl_SetObjResult(interp, list);
    return TCL_OK;

  } else if (cmd) {
    /*
     * If a command is found for the object|class, check whether we
     * find the parameter definitions for the C-defined method.
     */
    methodDefinition *mdPtr = &method_definitions[0];

    for (; mdPtr->methodName; mdPtr ++) {

      /*fprintf(stderr, "... comparing %p with %p => %s\n", ((Command *)cmd)->objProc, mdPtr->proc, 
        mdPtr->methodName);*/

      if (((Command *)cmd)->objProc == mdPtr->proc) {
        XOTclParamDefs paramDefs = {mdPtr->paramDefs, mdPtr->nrParameters};
	Tcl_Obj *list = ListParamDefs(interp, paramDefs.paramsPtr, withVarnames);

        Tcl_SetObjResult(interp, list);
        return TCL_OK;
      }
    }

    if (((Command *)cmd)->objProc == XOTclSetterMethod) {
      SetterCmdClientData *cd = (SetterCmdClientData *)Tcl_Command_objClientData(cmd);
      if (cd->paramsPtr) {
        Tcl_Obj *list;
        XOTclParamDefs paramDefs;
        paramDefs.paramsPtr = cd->paramsPtr;
        paramDefs.nrParams = 1;
        paramDefs.slotObj = NULL;
	list = ListParamDefs(interp, paramDefs.paramsPtr, withVarnames);
        Tcl_SetObjResult(interp, list);
        return TCL_OK;
      } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(methodName, -1));
        return TCL_OK;
      }
    } else if (((Command *)cmd)->objProc == XOTclForwardMethod) {
      return XOTclVarErrMsg(interp, "info params: could not obtain parameter definition for forwarder '",
                            methodName, "'", (char *) NULL);
    } else {
      return XOTclVarErrMsg(interp, "info params: could not obtain parameter definition for method '",
                            methodName, "'", (char *) NULL);
    }
  }
  return XOTclErrBadVal(interp, "info params", "a method name", methodName);
}

static void
AppendForwardDefinition(Tcl_Interp *interp, Tcl_Obj *listObj, ForwardCmdClientData *tcd) {
  if (tcd->prefix) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("-methodprefix", -1));
    Tcl_ListObjAppendElement(interp, listObj, tcd->prefix);
  }
  if (tcd->subcommands) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("-default", -1));
    Tcl_ListObjAppendElement(interp, listObj, tcd->subcommands);
  }
  if (tcd->objscope) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("-objscope", -1));
  }
  Tcl_ListObjAppendElement(interp, listObj, tcd->cmdName);
  if (tcd->args) {
    Tcl_Obj **args;
    int nrArgs, i;
    Tcl_ListObjGetElements(interp, tcd->args, &nrArgs, &args);
    for (i=0; i<nrArgs; i++) {
      Tcl_ListObjAppendElement(interp, listObj, args[i]);
    }
  }
}

static void
AppendMethodRegistration(Tcl_Interp *interp, Tcl_Obj *listObj, CONST char *registerCmdName,
                         XOTclObject *object, CONST char *methodName, Tcl_Command cmd, 
                         int withObjscope, int withPer_object) {
  Tcl_ListObjAppendElement(interp, listObj, object->cmdName);
  if (withPer_object) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("object", 6));
  }
  Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj(registerCmdName, -1));
  if (withObjscope) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("-objscope", 9));
  }
  if (Tcl_Command_flags(cmd) & XOTCL_CMD_NONLEAF_METHOD) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("-nonleaf", 8));
  }
  Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj(methodName, -1));
}

static int
ListMethodHandle(Tcl_Interp *interp, XOTclObject *object, int withPer_object, CONST char *methodName) {
  Tcl_SetObjResult(interp, MethodHandleObj(object, withPer_object, methodName));
  return TCL_OK;
}

static int
ListMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *methodName, Tcl_Command cmd, 
           int subcmd, int withPer_object) {

  /*fprintf(stderr, "ListMethodtype %s %s %p subcmd %d per-object %d\n",
    objectName(object), methodName, cmd, subcmd, withPer_object);*/

  if (!cmd) {
    Tcl_SetObjResult(interp, XOTclGlobalObjs[XOTE_EMPTY]);
  } else {
    Tcl_ObjCmdProc *procPtr = Tcl_Command_objProc(cmd);
    int outputPerObject = 0;
    Tcl_Obj *resultObj;

    if (*methodName == ':') {
      /* 
       * We have a fully qualified method name, maybe an object handle 
       */
      CONST char *procName = Tcl_GetCommandName(interp, cmd);
      size_t objNameLength = strlen(methodName) - strlen(procName) - 2;
      Tcl_DString ds, *dsPtr = &ds;

      if (objNameLength > 0) {
	XOTclObject *object1;
	int fromClassNS;

	Tcl_DStringInit(dsPtr);
	Tcl_DStringAppend(dsPtr, methodName, objNameLength);
	object1 = GetObjectFromNsName(interp, Tcl_DStringValue(dsPtr), &fromClassNS);
	if (object1) {
	  /* 
	   * The command was from an object, return therefore this
	   * object as reference.
	   */
	  /*fprintf(stderr, "We are flipping the object to %s, method %s to %s !fromClassNS %d\n",
	    objectName(object1), methodName, procName, !fromClassNS);*/
	  object = object1;
	  methodName = procName;
	  withPer_object = fromClassNS ? 0 : 1;
	}
	Tcl_DStringFree(dsPtr);
      }
    }

    if (!XOTclObjectIsClass(object)) {
      withPer_object = 1;
      /* don't output "object" modifier, if object is not a class */
      outputPerObject = 0;
    } else {
      outputPerObject = withPer_object;
    }

    switch (subcmd) {
    case InfomethodsubcmdHandleIdx: 
      {
        return ListMethodHandle(interp, object, withPer_object, methodName);
      }
    case InfomethodsubcmdArgsIdx:
      {
        Tcl_Command importedCmd = GetOriginalCommand(cmd);
        return ListCmdParams(interp, importedCmd, methodName, 1);
      }
    case InfomethodsubcmdParameterIdx:
      {
        Tcl_Command importedCmd = GetOriginalCommand(cmd);
        return ListCmdParams(interp, importedCmd, methodName, 0);
      }
    case InfomethodsubcmdParametersyntaxIdx:
      {
        Tcl_Command importedCmd = GetOriginalCommand(cmd);
        return ListCmdParams(interp, importedCmd, methodName, 2);
      }
    case InfomethodsubcmdPreconditionIdx:
      {
        XOTclProcAssertion *procs;
        if (withPer_object) {
          procs = object->opt ? AssertionFindProcs(object->opt->assertions, methodName) : NULL;
        } else {
          XOTclClass *class = (XOTclClass *)object;
          procs = class->opt ? AssertionFindProcs(class->opt->assertions, methodName) : NULL;
        }
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->pre));
        return TCL_OK;
      }
    case InfomethodsubcmdPostconditionIdx:
      {
        XOTclProcAssertion *procs;
        if (withPer_object) {
          procs = object->opt ? AssertionFindProcs(object->opt->assertions, methodName) : NULL;
        } else {
          XOTclClass *class = (XOTclClass *)object;
          procs = class->opt ? AssertionFindProcs(class->opt->assertions, methodName) : NULL;
        }
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->post));
        return TCL_OK;
      }

    }

    /* 
     * Subcommands different per type of method. The Converter in
     * InfoMethods defines the types:
     *
     *    "all|scripted|system|alias|forwarder|object|setter"
     */
    if (GetTclProcFromCommand(cmd)) {
      /* a scripted method */
      switch (subcmd) {

      case InfomethodsubcmdTypeIdx:
        Tcl_SetObjResult(interp, Tcl_NewStringObj("scripted", -1));
        break;
        
      case InfomethodsubcmdBodyIdx: 
        ListProcBody(interp, GetTclProcFromCommand(cmd), methodName);
        break;

      case InfomethodsubcmdDefinitionIdx: 
        {
          XOTclAssertionStore *assertions;

          resultObj = Tcl_NewListObj(0, NULL);
          /* todo: don't hard-code registering command name "method" / XOTE_METHOD */
          AppendMethodRegistration(interp, resultObj, XOTclGlobalStrings[XOTE_METHOD], 
                                   object, methodName, cmd, 0, outputPerObject);
          ListCmdParams(interp, cmd, methodName, 0);
          Tcl_ListObjAppendElement(interp, resultObj, Tcl_GetObjResult(interp));
          ListProcBody(interp, GetTclProcFromCommand(cmd), methodName);
          Tcl_ListObjAppendElement(interp, resultObj, Tcl_GetObjResult(interp));

          if (withPer_object) {
            assertions = object->opt ? object->opt->assertions : NULL;
          } else {
            XOTclClass *class = (XOTclClass *)object;
            assertions = class->opt ? class->opt->assertions : NULL;
          }
          if (assertions) {
            XOTclProcAssertion *procs = AssertionFindProcs(assertions, methodName);
            if (procs) {
              Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("-precondition", -1));
              Tcl_ListObjAppendElement(interp, resultObj, AssertionList(interp, procs->pre));
              Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("-postcondition", -1));
              Tcl_ListObjAppendElement(interp, resultObj, AssertionList(interp, procs->post));
            }
          }
          Tcl_SetObjResult(interp, resultObj);
          break;
        }
      }

    } else if (procPtr == XOTclForwardMethod) {
      /* forwarder */
      switch (subcmd) {
      case InfomethodsubcmdTypeIdx: 
        Tcl_SetObjResult(interp, XOTclGlobalObjs[XOTE_FORWARD]);
        break;
      case InfomethodsubcmdDefinitionIdx:
        {
          ClientData clientData = cmd ? Tcl_Command_objClientData(cmd) : NULL;

          if (clientData) {
            resultObj = Tcl_NewListObj(0, NULL);
            /* todo: don't hard-code registering command name "forward" / XOTE_FORWARD*/
            AppendMethodRegistration(interp, resultObj, XOTclGlobalStrings[XOTE_FORWARD], 
                                     object, methodName, cmd, 0, outputPerObject);
            AppendForwardDefinition(interp, resultObj, clientData);
            Tcl_SetObjResult(interp, resultObj);
            break;
          }
        }
      }

    } else if (procPtr == XOTclSetterMethod) {
      /* setter methods */
      switch (subcmd) {
      case InfomethodsubcmdTypeIdx: 
        Tcl_SetObjResult(interp, XOTclGlobalObjs[XOTE_SETTER]);
        break;
      case InfomethodsubcmdDefinitionIdx: {
        SetterCmdClientData *cd = (SetterCmdClientData *)Tcl_Command_objClientData(cmd);

        resultObj = Tcl_NewListObj(0, NULL);
        /* todo: don't hard-code registering command name "setter" / XOTE_SETTER */

        AppendMethodRegistration(interp, resultObj, XOTclGlobalStrings[XOTE_SETTER], object, 
                                 cd->paramsPtr ? ObjStr(cd->paramsPtr->paramObj) : methodName, 
                                 cmd, 0, outputPerObject);
        Tcl_SetObjResult(interp, resultObj);        
        break;
      }
      }

    } else {
      /* must be an alias */
      switch (subcmd) {
      case InfomethodsubcmdTypeIdx: 
        Tcl_SetObjResult(interp, XOTclGlobalObjs[XOTE_ALIAS]);
        break;
      case InfomethodsubcmdDefinitionIdx:
        {
          Tcl_Obj *entryObj = AliasGet(interp, object->cmdName, methodName, withPer_object);
	  /*fprintf(stderr, "aliasGet %s -> %s (%d) returned %p\n",
	    objectName(object), methodName, withPer_object, entryObj);*/
          if (entryObj) {
            int nrElements;
            Tcl_Obj **listElements;
            resultObj = Tcl_NewListObj(0, NULL);
            Tcl_ListObjGetElements(interp, entryObj, &nrElements, &listElements);
            /* todo: don't hard-code registering command name "alias" / XOTE_ALIAS */
            AppendMethodRegistration(interp, resultObj, XOTclGlobalStrings[XOTE_ALIAS], 
                                     object, methodName, cmd, nrElements!=1, outputPerObject);
            Tcl_ListObjAppendElement(interp, resultObj, listElements[nrElements-1]);
            Tcl_SetObjResult(interp, resultObj);
            break;
          }
        }
      }
    }
  }
  return TCL_OK;
}

static int
ProtectionMatches(Tcl_Interp *interp, int withCallprotection, Tcl_Command cmd) {
  int result, isProtected = Tcl_Command_flags(cmd) & XOTCL_CMD_PROTECTED_METHOD;
  if (withCallprotection == CallprotectionNULL) {
    withCallprotection = CallprotectionPublicIdx;
  }
  switch (withCallprotection) {
  case CallprotectionAllIdx: result = 1; break;
  case CallprotectionPublicIdx: result = (isProtected == 0); break;
  case CallprotectionProtectedIdx: result = (isProtected == 1); break;
  default: result = 1;
  }
  return result;
}

static int
MethodTypeMatches(Tcl_Interp *interp, int methodType, Tcl_Command cmd, 
                  XOTclObject *object, CONST char *key, int withPer_object) {
  Tcl_Command importedCmd;
  Tcl_ObjCmdProc *proc, *resolvedProc;

  proc = Tcl_Command_objProc(cmd);
  importedCmd = GetOriginalCommand(cmd);
  resolvedProc = Tcl_Command_objProc(importedCmd);

  if (methodType == XOTCL_METHODTYPE_ALIAS) {
    if (!(proc == XOTclProcAliasMethod || AliasGet(interp, object->cmdName, key, withPer_object))) {
      return 0;
    }
  } else {
    if (proc == XOTclProcAliasMethod) {
      if ((methodType & XOTCL_METHODTYPE_ALIAS) == 0) return 0;
    }
    /* the following cases are disjoint */
    if (CmdIsProc(importedCmd)) {
      /*fprintf(stderr,"%s scripted %d\n", key, methodType & XOTCL_METHODTYPE_SCRIPTED);*/
      if ((methodType & XOTCL_METHODTYPE_SCRIPTED) == 0) return 0;
    } else if (resolvedProc == XOTclForwardMethod) {
      if ((methodType & XOTCL_METHODTYPE_FORWARDER) == 0) return 0;
    } else if (resolvedProc == XOTclSetterMethod) {
      if ((methodType & XOTCL_METHODTYPE_SETTER) == 0) return 0;
    } else if (resolvedProc == XOTclObjDispatch) {
      if ((methodType & XOTCL_METHODTYPE_OBJECT) == 0) return 0;
    } else if ((methodType & XOTCL_METHODTYPE_OTHER) == 0) {
      /* fprintf(stderr,"OTHER %s not wanted %.4x\n", key, methodType);*/
      return 0;
    } 
    /* XOTclObjscopedMethod ??? */
  }
  return 1;
}

static int
ListMethodKeys(Tcl_Interp *interp, Tcl_HashTable *table, CONST char *pattern, 
               int methodType, int withCallprotection,
               Tcl_HashTable *dups, XOTclObject *object, int withPer_object) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr, *duphPtr;
  Tcl_Command cmd;
  char *key;
  int new;

  if (pattern && noMetaChars(pattern)) {
    /* We have a pattern that can be used for direct lookup; 
     * no need to iterate 
     */
    hPtr = table ? Tcl_CreateHashEntry(table, pattern, NULL) : NULL;
    if (hPtr) {
      key = Tcl_GetHashKey(table, hPtr);
      cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);

      if (Tcl_Command_flags(cmd) & XOTCL_CMD_CLASS_ONLY_METHOD && !XOTclObjectIsClass(object)) {
	return TCL_OK;
      }

      if (ProtectionMatches(interp, withCallprotection, cmd) 
          && MethodTypeMatches(interp, methodType, cmd, object, key, withPer_object)) {
        if (dups) {
          duphPtr = Tcl_CreateHashEntry(dups, key, &new);
          if (new) {
            Tcl_AppendElement(interp, key);
          }
        } else {
          Tcl_AppendElement(interp, key);
        }
      }
    }
    return TCL_OK;

  } else {
    hPtr = table ? Tcl_FirstHashEntry(table, &hSrch) : 0;

    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      key = Tcl_GetHashKey(table, hPtr);
      cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);

      if (Tcl_Command_flags(cmd) & XOTCL_CMD_CLASS_ONLY_METHOD && !XOTclObjectIsClass(object)) continue;
      if (pattern && !Tcl_StringMatch(key, pattern)) continue;
      if (!ProtectionMatches(interp, withCallprotection, cmd)
          || !MethodTypeMatches(interp, methodType, cmd, object, key, withPer_object)
          ) continue;
      
      if (dups) {
        duphPtr = Tcl_CreateHashEntry(dups, key, &new);
        if (!new) continue;
      }
      Tcl_AppendElement(interp, key);
    }
  }
  /*fprintf(stderr, "listkeys returns '%s'\n", ObjStr(Tcl_GetObjResult(interp)));*/
  return TCL_OK;
}

static int
ListChildren(Tcl_Interp *interp, XOTclObject *object, CONST char *pattern, int classesOnly) {
  XOTclObject *childObject;
  Tcl_HashTable *cmdTable;

  if (!object->nsPtr) return TCL_OK;

  cmdTable = Tcl_Namespace_cmdTable(object->nsPtr);
  if (pattern && noMetaChars(pattern)) {

    if ((childObject = XOTclpGetObject(interp, pattern)) &&
        (!classesOnly || XOTclObjectIsClass(childObject)) &&
        (Tcl_Command_nsPtr(childObject->id) == object->nsPtr)  /* true children */
        ) {
      Tcl_SetObjResult(interp, childObject->cmdName);
    } else {
      Tcl_SetObjResult(interp, XOTclGlobalObjs[XOTE_EMPTY]);
    }

  } else {
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch);
    char *key;

    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      key = Tcl_GetHashKey(cmdTable, hPtr);
      if (!pattern || Tcl_StringMatch(key, pattern)) {
        Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);

        /*fprintf(stderr, "... check %s child key %s child object %p %p\n",
                objectName(object),key,XOTclpGetObject(interp, key),
                XOTclGetObjectFromCmdPtr(cmd));*/

        if ((childObject = XOTclGetObjectFromCmdPtr(cmd)) &&
            (!classesOnly || XOTclObjectIsClass(childObject)) &&
            (Tcl_Command_nsPtr(childObject->id) == object->nsPtr)  /* true children */
            ) {
          Tcl_ListObjAppendElement(interp, list, childObject->cmdName);
        }
      }
    }
    Tcl_SetObjResult(interp, list);
  }

  return TCL_OK;
}

static int
ListForward(Tcl_Interp *interp, Tcl_HashTable *table, CONST char *pattern, int withDefinition) {
  if (withDefinition) {
    Tcl_HashEntry *hPtr = table && pattern ? Tcl_CreateHashEntry(table, pattern, NULL) : NULL;
    /* notice: we don't use pattern for wildcard matching here;
       pattern can only contain wildcards when used without
       "-definition" */
    if (hPtr) {
      Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
      ClientData clientData = cmd ? Tcl_Command_objClientData(cmd) : NULL;
      ForwardCmdClientData *tcd = (ForwardCmdClientData *)clientData;
      if (tcd && Tcl_Command_objProc(cmd) == XOTclForwardMethod) {
        Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
        AppendForwardDefinition(interp, listObj, tcd);
        Tcl_SetObjResult(interp, listObj);
        return TCL_OK;
      }
    }
    return XOTclVarErrMsg(interp, "'", pattern, "' is not a forwarder", (char *) NULL);
  }
  return ListMethodKeys(interp, table, pattern, XOTCL_METHODTYPE_FORWARDER, CallprotectionAllIdx, NULL, NULL, 0);
}

static int
ListDefinedMethods(Tcl_Interp *interp, XOTclObject *object, CONST char *pattern,
                   int withPer_object, int methodType, int withCallproctection,
                   int noMixins, int inContext) {
  Tcl_HashTable *cmdTable;

  if (XOTclObjectIsClass(object) && !withPer_object) {
    cmdTable = Tcl_Namespace_cmdTable(((XOTclClass *)object)->nsPtr);
  } else {
    cmdTable = object->nsPtr ? Tcl_Namespace_cmdTable(object->nsPtr) : NULL;
  }
  ListMethodKeys(interp, cmdTable, pattern, methodType, withCallproctection, 
                 NULL, object, withPer_object);
  return TCL_OK;
}

static int
ListCallableMethods(Tcl_Interp *interp, XOTclObject *object, CONST char *pattern,
                    int methodType, int withCallprotection,
                    int withApplication, int noMixins, int inContext) {
  XOTclClasses *pl;
  int withPer_object = 1;
  Tcl_HashTable *cmdTable, dupsTable, *dups = &dupsTable;

  /* 
   * TODO: we could make this faster for patterns without metachars 
   * by letting ListMethodKeys() to signal us when an entry was found.
   * we wait, until the we decided about "info methods defined" 
   * vs. "info method search" vs. "info defined" etc.
   */
  if (withCallprotection == CallprotectionNULL) {
    withCallprotection = CallprotectionPublicIdx;
  }

  if (withApplication && object->flags & IsBaseClass((XOTclClass*)object)) {
    return TCL_OK;
  }

  Tcl_InitHashTable(dups, TCL_STRING_KEYS);
  if (object->nsPtr) {
    cmdTable = Tcl_Namespace_cmdTable(object->nsPtr);
    ListMethodKeys(interp, cmdTable, pattern, methodType, withCallprotection, 
                   dups, object, withPer_object);
  }

  if (!noMixins) {
    if (!(object->flags & XOTCL_MIXIN_ORDER_VALID))
      MixinComputeDefined(interp, object);
    if (object->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
      XOTclCmdList *ml;
      XOTclClass *mixin;
      for (ml = object->mixinOrder; ml; ml = ml->nextPtr) {
	int guardOk = TCL_OK;
        mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
	assert(mixin);

        if (inContext) {
          if (!RUNTIME_STATE(interp)->guardCount) {
            guardOk = GuardCall(object, 0, 0, interp, ml->clientData, NULL);
          }
        }
        if (mixin && guardOk == TCL_OK) {
          Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(mixin->nsPtr);
          ListMethodKeys(interp, cmdTable, pattern, methodType, withCallprotection, 
                         dups, object, withPer_object);
        }
      }
    }
  }

  /* append method keys from inheritance order */
  for (pl = ComputeOrder(object->cl, object->cl->order, Super); pl; pl = pl->nextPtr) {
    Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(pl->cl->nsPtr);
    if (withApplication && IsBaseClass(pl->cl)) {
      break;
    }
    ListMethodKeys(interp, cmdTable, pattern, methodType, withCallprotection, 
                   dups, object, withPer_object);
  }
  Tcl_DeleteHashTable(dups);
  return TCL_OK;
}

static int
ListSuperclasses(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *pattern, int withClosure) {
  XOTclObject *matchObject = NULL;
  Tcl_Obj *patternObj = NULL, *outObjPtr;
  CONST char *patternString = NULL;
  int rc;

  if (pattern && 
      convertToObjpattern(interp, pattern, NULL, (ClientData *)&patternObj, &outObjPtr) == TCL_OK) {
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
    Tcl_SetObjResult(interp, rc ? matchObject->cmdName : XOTclGlobalObjs[XOTE_EMPTY]);
  }

  if (patternObj) {
    DECR_REF_COUNT(patternObj);
  }
  return TCL_OK;
}


/********************************
 * End result setting commands
 ********************************/

static CONST char* AliasIndex(Tcl_DString *dsPtr, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object) {
  Tcl_DStringInit(dsPtr);
  Tcl_DStringAppend(dsPtr,  ObjStr(cmdName), -1);
  Tcl_DStringAppend(dsPtr,  ",", 1);
  Tcl_DStringAppend(dsPtr,  methodName, -11);
  if (withPer_object) {
    Tcl_DStringAppend(dsPtr,  ",1", 2);
  } else {
    Tcl_DStringAppend(dsPtr,  ",0", 2);
  }
  /*fprintf(stderr, "AI %s\n", Tcl_DStringValue(dsPtr));*/
  return Tcl_DStringValue(dsPtr);
}

static int AliasAdd(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object, 
                    CONST char *cmd) {
  Tcl_DString ds, *dsPtr = &ds;
  Tcl_SetVar2Ex(interp, XOTclGlobalStrings[XOTE_ALIAS_ARRAY], 
                AliasIndex(dsPtr, cmdName, methodName, withPer_object), 
                Tcl_NewStringObj(cmd, -1), 
                TCL_GLOBAL_ONLY);
  /*fprintf(stderr, "aliasAdd ::nsf::alias(%s) '%s' returned %p\n",
    AliasIndex(dsPtr, cmdName, methodName, withPer_object), cmd, 1);*/
  Tcl_DStringFree(dsPtr);
  return TCL_OK;
}

static int AliasDelete(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object) {
  Tcl_DString ds, *dsPtr = &ds;
  int result = Tcl_UnsetVar2(interp, XOTclGlobalStrings[XOTE_ALIAS_ARRAY], 
                             AliasIndex(dsPtr, cmdName, methodName, withPer_object), 
                             TCL_GLOBAL_ONLY);
  /*fprintf(stderr, "aliasDelete ::nsf::alias(%s) returned %d (%d)\n",
    AliasIndex(dsPtr, cmdName, methodName, withPer_object), result);*/
  Tcl_DStringFree(dsPtr);
  return result;
}

static Tcl_Obj *AliasGet(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object) {
  Tcl_DString ds, *dsPtr = &ds;
  Tcl_Obj *obj = Tcl_GetVar2Ex(interp, XOTclGlobalStrings[XOTE_ALIAS_ARRAY], 
                               AliasIndex(dsPtr, cmdName, methodName, withPer_object), 
                               TCL_GLOBAL_ONLY);
  /*fprintf(stderr, "aliasGet returns %p\n", object);*/
  Tcl_DStringFree(dsPtr);
  return obj;
}


/*********************************
 * Begin generated XOTcl commands
 *********************************/
/*
xotclCmd alias XOTclAliasCmd {
  {-argName "object" -type object}
  {-argName "-per-object"}
  {-argName "methodName"}
  {-argName "-nonleaf"}
  {-argName "-objscope"}
  {-argName "cmdName" -required 1 -type tclobj}
}
*/
static int XOTclAliasCmd(Tcl_Interp *interp, XOTclObject *object, int withPer_object, 
                         CONST char *methodName, int withNonleaf, int withObjscope, 
                         Tcl_Obj *cmdName) {
  Tcl_ObjCmdProc *objProc, *newObjProc = NULL;
  Tcl_CmdDeleteProc *deleteProc = NULL;
  AliasCmdClientData *tcd = NULL; /* make compiler happy */
  Tcl_Command cmd, newCmd = NULL;
  Tcl_Namespace *nsPtr;
  int flags, result;
  XOTclClass *cl = (withPer_object || ! XOTclObjectIsClass(object)) ? NULL : (XOTclClass *)object;

  cmd = Tcl_GetCommandFromObj(interp, cmdName);
  if (cmd == NULL) {
    return XOTclVarErrMsg(interp, "cannot lookup command '",
                          ObjStr(cmdName), "'", (char *) NULL);
  }

  cmd = GetOriginalCommand(cmd);
  objProc = Tcl_Command_objProc(cmd);

  /* objProc is either ...  

     1. XOTclObjDispatch: a command representing an XOTcl object 

     2. TclObjInterpProc: a cmd standing for a
        Tcl proc (including XOTcl methods), verified through 
        CmdIsProc() -> to be wrapped by XOTclProcAliasMethod()
     
     3. XOTclForwardMethod: an XOTcl forwarder 

     4. XOTclSetterMethod: an XOTcl setter 

     5. arbitrary Tcl commands (e.g. set, ..., ::nsf::relation, ...)

     TODO GN: i think, we should use XOTclProcAliasMethod, whenever the clientData
     is not 0. These are the cases, where the clientData will be freed,
     when the original command is deleted.
  */

  if (withObjscope) {
    newObjProc = XOTclObjscopedMethod;
  }

  if (objProc == XOTclObjDispatch) {
    /*
     * if we register an alias for an object, we have to take care to
     * handle cases, where the aliased object is destroyed and the
     * alias points to nowhere. We realize this via using the object
     * refcount.
     */
    /*fprintf(stderr, "registering an object %p\n", tcd);*/
    
    XOTclObjectRefCountIncr((XOTclObject *)Tcl_Command_objClientData(cmd));
    
    /*newObjProc = XOTclProcAliasMethod;*/

  } else if (CmdIsProc(cmd)) {
    /* 
     * if we have a tcl proc|xotcl-method as alias, then use the 
     * wrapper, which will be deleted automatically when the original
     * proc/method is deleted. 
     */
    newObjProc = XOTclProcAliasMethod;

    if (withObjscope) {
      return XOTclVarErrMsg(interp, "cannot use -objscope for tcl implemented command '",
                            ObjStr(cmdName), "'", (char *) NULL);
    }
  }

  if (newObjProc) {
    /* add a wrapper */
    tcd = NEW(AliasCmdClientData);
    tcd->cmdName    = object->cmdName;
    tcd->interp     = interp; /* just for deleting the associated variable */
    tcd->object     = object;
    tcd->class	    = cl ? (XOTclClass *) object : NULL;
    tcd->objProc    = objProc;
    tcd->aliasedCmd = cmd;
    tcd->clientData = Tcl_Command_objClientData(cmd);
    objProc         = newObjProc;
    deleteProc      = aliasCmdDeleteProc;
    if (tcd->cmdName) {INCR_REF_COUNT(tcd->cmdName);}
  } else {
    /* call the command directly (must be a c-implemented command not 
     * depending on a volatile client data) 
     */
    tcd = Tcl_Command_objClientData(cmd);
  }

  flags = 0;

  if (cl) {
    result = XOTclAddClassMethod(interp, (XOTcl_Class *)cl, methodName,
                                    objProc, tcd, deleteProc, flags);
    nsPtr = cl->nsPtr;
  } else {
    result = XOTclAddObjectMethod(interp, (XOTcl_Object*)object, methodName,
                                  objProc, tcd, deleteProc, flags);
    nsPtr = object->nsPtr;
  }

  if (result == TCL_OK) {
    newCmd = FindMethod(nsPtr, methodName);
  }
  
  if (newObjProc) {
    /* 
     * Define the reference chain like for 'namespace import' to
     *  obtain automatic deletes when the original command is deleted.
     */
    ImportRef *refPtr = (ImportRef *) ckalloc(sizeof(ImportRef));
    refPtr->importedCmdPtr = (Command *) newCmd;
    refPtr->nextPtr = ((Command *) tcd->aliasedCmd)->importRefPtr;
    ((Command *) tcd->aliasedCmd)->importRefPtr = refPtr;
    tcd->aliasCmd = newCmd;
  }

  if (newCmd) {  
    Tcl_DString ds, *dsPtr = &ds;
    Tcl_DStringInit(dsPtr);
    /*if (withPer_object) {Tcl_DStringAppend(dsPtr, "-per-object ", -1);}*/
    if (withObjscope) {Tcl_DStringAppend(dsPtr, "-objscope ", -1);}
    Tcl_DStringAppend(dsPtr, ObjStr(cmdName), -1);
    AliasAdd(interp, object->cmdName, methodName, cl == NULL, Tcl_DStringValue(dsPtr));
    Tcl_DStringFree(dsPtr);

    if (!withObjscope && withNonleaf) {
      Tcl_Command_flags(newCmd) |= XOTCL_CMD_NONLEAF_METHOD;
      /*fprintf(stderr, "setting aliased for cmd %p %s flags %.6x, tcd = %p\n",
        newCmd,methodName,Tcl_Command_flags(newCmd), tcd);*/
    }

    result = ListMethodHandle(interp, object, cl == NULL, methodName);
  }

  return result;
}

/*
xotclCmd assertion XOTclAssertionCmd {
  {-argName "object" -type object}
  {-argName "assertionsubcmd" -required 1 -type "check|object-invar|class-invar"}
  {-argName "arg" -required 0 -type tclobj}
}

  Make "::nsf::assertion" a cmd rather than a method, otherwise we
  cannot define e.g. a "method check options {...}" to reset the check
  options in case of a failed option, since assertion checking would
  be applied on the sketched method already.
*/

static int XOTclAssertionCmd(Tcl_Interp *interp, XOTclObject *object, int subcmd, Tcl_Obj *arg) {
  XOTclClass *class;

  switch (subcmd) {
  case AssertionsubcmdCheckIdx:
    if (arg) {
      return AssertionSetCheckOptions(interp, object, arg);
    } else {
      return AssertionListCheckOption(interp, object);
    }
    break;
    
  case AssertionsubcmdObject_invarIdx:
    if (arg) {
      XOTclObjectOpt *opt = XOTclRequireObjectOpt(object);
      AssertionSetInvariants(interp, &opt->assertions, arg);
    } else {
      if (object->opt && object->opt->assertions) {
        Tcl_SetObjResult(interp, AssertionList(interp, object->opt->assertions->invariants));
      }
    }
    break;

  case AssertionsubcmdClass_invarIdx:
    class = (XOTclClass *)object;
    if (arg) {
      XOTclClassOpt *opt = XOTclRequireClassOpt(class);
      AssertionSetInvariants(interp, &opt->assertions, arg);
    } else {
      if (class->opt && class->opt->assertions) {
        Tcl_SetObjResult(interp, AssertionList(interp, class->opt->assertions->invariants));
      }
    }
  }
  return TCL_OK;
}

/*
xotclCmd configure XOTclConfigureCmd {
  {-argName "configureoption" -required 1 -type "filter|softrecreate|objectsystems|keepinitcmd"}
  {-argName "value" -required 0 -type tclobj}
}
*/
static int XOTclConfigureCmd(Tcl_Interp *interp, int configureoption, Tcl_Obj *valueObj) {
  int bool;

  if (configureoption == ConfigureoptionObjectsystemsIdx) {
    XOTclObjectSystem *osPtr;
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);

    for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = osPtr->nextPtr) {
      Tcl_Obj *osObj = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, osObj, osPtr->rootClass->object.cmdName);
      Tcl_ListObjAppendElement(interp, osObj, osPtr->rootMetaClass->object.cmdName);
      Tcl_ListObjAppendElement(interp, list, osObj);
    }
    Tcl_SetObjResult(interp, list);
    return TCL_OK;
  }

  if (valueObj) {
    int result = Tcl_GetBooleanFromObj(interp, valueObj, &bool);
    if (result != TCL_OK)
      return result;
  }

  switch (configureoption) {
  case ConfigureoptionFilterIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doFilters));
    if (valueObj)
      RUNTIME_STATE(interp)->doFilters = bool;
    break;

  case ConfigureoptionSoftrecreateIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doSoftrecreate));
    if (valueObj)
      RUNTIME_STATE(interp)->doSoftrecreate = bool;
    break;

  case ConfigureoptionKeepinitcmdIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doKeepinitcmd));
    if (valueObj)
      RUNTIME_STATE(interp)->doKeepinitcmd = bool;
    break;
  }
  return TCL_OK;
}


/*
xotclCmd createobjectsystem XOTclCreateObjectSystemCmd {
  {-argName "rootClass" -required 1 -type tclobj}
  {-argName "rootMetaClass" -required 1 -type tclobj}
  {-argName "systemMethods" -required 0 -type tclobj}
}
*/
static int
XOTclCreateObjectSystemCmd(Tcl_Interp *interp, Tcl_Obj *Object, Tcl_Obj *Class, Tcl_Obj *systemMethodsObj) {
  XOTclClass *theobj;
  XOTclClass *thecls;
  XOTclObjectSystem *osPtr = NEW(XOTclObjectSystem);

  memset(osPtr, 0, sizeof(XOTclObjectSystem));

  if (systemMethodsObj) {
    int oc, i, idx, result;
    Tcl_Obj **ov;

    if ((result = Tcl_ListObjGetElements(interp, systemMethodsObj, &oc, &ov)) == TCL_OK) {
      if (oc % 2) {
        ObjectSystemFree(interp, osPtr);
        return XOTclErrMsg(interp, "System methods must be provided as pairs", TCL_STATIC);
      }
      for (i=0; i<oc; i += 2) {
        result = Tcl_GetIndexFromObj(interp, ov[i], XOTcl_SytemMethodOpts, "system method", 0, &idx);
        if (result != TCL_OK) {
          ObjectSystemFree(interp, osPtr);
          return XOTclVarErrMsg(interp, "invalid system method '",
                                ObjStr(ov[i]), "'", (char *) NULL);
        }
        /*fprintf(stderr, "XOTclCreateObjectSystemCmd [%d] = %p %s (max %d, given %d)\n", 
          idx, ov[i+1], ObjStr(ov[i+1]), XO_unknown_idx, oc);*/
        osPtr->methods[idx] = ov[i+1];
        INCR_REF_COUNT(osPtr->methods[idx]);
      }
    } else {
      ObjectSystemFree(interp, osPtr);
      return XOTclErrMsg(interp, "Provided system methods are not a proper list", TCL_STATIC);
    }
  }
  /* 
     Create a basic object system with the basic root class Object and
     the basic metaclass Class, and store them in the RUNTIME STATE if
     successful 
  */
  theobj = PrimitiveCCreate(interp, Object, NULL);
  thecls = PrimitiveCCreate(interp, Class, NULL);
  /* fprintf(stderr, "CreateObjectSystem created base classes \n"); */

#if defined(PROFILE)
  XOTclProfileInit(interp);
#endif

  /* check whether Object and Class creation was successful */
  if (!theobj || !thecls) {
    int i;

    if (thecls) PrimitiveCDestroy((ClientData) thecls);
    if (theobj) PrimitiveCDestroy((ClientData) theobj);

    for (i = 0; i < nr_elements(XOTclGlobalStrings); i++) {
      DECR_REF_COUNT(XOTclGlobalObjs[i]);
    }
    FREE(Tcl_Obj **, XOTclGlobalObjs);
    FREE(XOTclRuntimeState, RUNTIME_STATE(interp));
    ObjectSystemFree(interp, osPtr);

    return XOTclErrMsg(interp, "Creation of object system failed", TCL_STATIC);
  }

  theobj->osPtr = osPtr;
  thecls->osPtr = osPtr;
  osPtr->rootClass = theobj;
  osPtr->rootMetaClass = thecls;

  theobj->object.flags |= XOTCL_IS_ROOT_CLASS;
  thecls->object.flags |= XOTCL_IS_ROOT_META_CLASS;

  ObjectSystemAdd(interp, osPtr);

  AddInstance((XOTclObject*)theobj, thecls);
  AddInstance((XOTclObject*)thecls, thecls);
  AddSuper(thecls, theobj);

  return TCL_OK;
}

/*
xotclCmd deprecated XOTclDeprecatedCmd {
  {-argName "what" -required 1}
  {-argName "oldCmd" -required 1}
  {-argName "newCmd" -required 0}
}
*/
/*
 * Prints a msg to the screen that oldCmd is deprecated
 * optinal: give a new cmd
 */
static int
XOTclDeprecatedCmd(Tcl_Interp *interp, CONST char *what, CONST char *oldCmd, CONST char *newCmd) {
  fprintf(stderr, "**\n**\n** The %s <%s> is deprecated.\n", what, oldCmd);
  if (newCmd)
    fprintf(stderr, "** Use <%s> instead.\n", newCmd);
  fprintf(stderr, "**\n");
  return TCL_OK;
}

/*
xotclCmd dispatch XOTclDispatchCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-objscope"}
  {-argName "command" -required 1 -type tclobj}
  {-argName "args"  -type args}
}
*/
static int
XOTclDispatchCmd(Tcl_Interp *interp, XOTclObject *object, int withObjscope, 
                 Tcl_Obj *command, int nobjc, Tcl_Obj *CONST nobjv[]) {
  int result;
  CONST char *methodName = ObjStr(command);
  register CONST char *n = methodName + strlen(methodName);

  /* fprintf(stderr, "Dispatch obj=%s, o=%p cmd m='%s'\n", objectName(object), object, methodName);*/

  /*
   * If the specified method is a fully qualified cmd name like
   * e.g. ::nsf::cmd::Class::alloc, this method is called on the
   * specified <Class|Object>, no matter whether it was registered on
   * it.
   */

  /*search for last '::'*/
  while ((*n != ':' || *(n-1) != ':') && n-1 > methodName) {n--; }
  if (*n == ':' && n > methodName && *(n-1) == ':') {n--;}

  if ((n-methodName)>1 || *methodName == ':') {
    Tcl_DString parentNSName, *dsp = &parentNSName;
    Tcl_Namespace *nsPtr;
    Tcl_Command cmd, importedCmd;
    CONST char *parentName, *tail = n+2;
    DSTRING_INIT(dsp);

    /*
     * We have an absolute name. We assume, the name is the name of a
     * tcl command, that will be dispatched. If "withObjscope is
     * specified, a callstack frame is pushed to make instvars
     * accessible for the command.
     */

    /*fprintf(stderr, "colon name %s\n", tail);*/
    if (n-methodName != 0) {
      Tcl_DStringAppend(dsp, methodName, (n-methodName));
      parentName = Tcl_DStringValue(dsp);
      nsPtr = Tcl_FindNamespace(interp, parentName, (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY);
      DSTRING_FREE(dsp);
    } else {
      nsPtr = Tcl_FindNamespace(interp, "::", (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY);
    }
    if (!nsPtr) {
      return XOTclVarErrMsg(interp, "cannot lookup parent namespace '",
			    methodName, "'", (char *) NULL);
    }
    cmd = FindMethod(nsPtr, tail);
    if (cmd && (importedCmd = TclGetOriginalCommand(cmd))) {
      cmd = importedCmd;
    }
    /*fprintf(stderr, "    .... findmethod '%s' in %s returns %p\n", tail, nsPtr->fullName, cmd);*/

    if (cmd == NULL) {
      return XOTclVarErrMsg(interp, "cannot lookup command '",
			    tail, "'", (char *) NULL);
    }
    {  Tcl_CallFrame frame, *framePtr = &frame;
      
      if (withObjscope) {
        XOTcl_PushFrameObj(interp, object, framePtr);
      }
      /*
       * Since we know, that we are always called with a full argument
       * vector, we can include the cmd name in the objv by using
       * nobjv-1; this way, we avoid a memcpy()
       */

      result = MethodDispatch((ClientData)object, interp,
                            nobjc+1, nobjv-1, cmd, object,
                            NULL /*XOTclClass *cl*/, tail,
                            XOTCL_CSC_TYPE_PLAIN);
      if (withObjscope) {
        XOTcl_PopFrameObj(interp, framePtr);
      }
    }
  } else {
    /*
     * No colons in command name, use method from the precedence
     * order, with filters etc. -- strictly speaking unneccessary,
     * since we could dispatch the method also without
     * XOTclDispatchCmd(), but it can be used to invoke protected
     * methods. 'withObjscope' is here a no-op.
     */
    Tcl_Obj *arg;
    Tcl_Obj *CONST *objv;

    if (nobjc >= 1) {
      arg = nobjv[0];
      objv = nobjv+1;
    } else {
      arg = NULL;
      objv = NULL;
    }
    result = XOTclCallMethodWithArgs((ClientData)object, interp, command, arg,
				     nobjc, objv, XOTCL_CM_NO_UNKNOWN);
  }

  return result;
}

/*
xotclCmd colon XOTclColonCmd {
  {-argName "args" -type allargs}
}
*/
static int XOTclColonCmd(Tcl_Interp *interp, int nobjc, Tcl_Obj *CONST nobjv[]) {
  XOTclObject *self = GetSelfObj(interp);
  if (!self) {
    return XOTclVarErrMsg(interp, "Cannot resolve 'self', probably called outside the context of an XOTcl Object",
                          (char *) NULL);
  }
  /*fprintf(stderr, "Colon dispatch %s on %s\n", ObjStr(nobjv[0]), objectName(self));*/

  return ObjectDispatch(self, interp, nobjc, nobjv, XOTCL_CM_NO_SHIFT);
}

/*
xotclCmd existsvar XOTclExistsVarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "var" -required 1}
}
*/
static int XOTclExistsVarCmd(Tcl_Interp *interp, XOTclObject *object, CONST char *varName) {
  if (CheckVarName(interp, varName) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_SetIntObj(Tcl_GetObjResult(interp), varExists(interp, object, varName, NULL, 1, 1));
  return TCL_OK;
}


/*
xotclCmd finalize XOTclFinalizeObjCmd {
}
*/
/*
 * ::nsf::finalize command
 */
static int
XOTclFinalizeObjCmd(Tcl_Interp *interp) {
  int result;

  /*fprintf(stderr, "+++ call tcl-defined exit handler\n");  */

#if defined(PROFILE)
  XOTclProfilePrintData(interp);
#endif
  /*
   * evaluate user-defined exit handler
   */
  result = Tcl_Eval(interp, "::nsf::__exitHandler");

  if (result != TCL_OK) {
    fprintf(stderr, "User defined exit handler contains errors!\n"
            "Error in line %d: %s\nExecution interrupted.\n",
            Tcl_GetErrorLine(interp), ObjStr(Tcl_GetObjResult(interp)));
  }

  ObjectSystemsCleanup(interp);

#ifdef DO_CLEANUP
  /*fprintf(stderr, "CLEANUP TOP NS\n");*/
  Tcl_Export(interp, RUNTIME_STATE(interp)->XOTclNS, "", 1);
  Tcl_DeleteNamespace(RUNTIME_STATE(interp)->XOTclClassesNS);
  Tcl_DeleteNamespace(RUNTIME_STATE(interp)->XOTclNS);
#endif

  return TCL_OK;
}

/*
xotclCmd forward XOTclForwardCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "method" -required 1 -type tclobj}
  {-argName "-default" -nrargs 1 -type tclobj}
  {-argName "-earlybinding"}
  {-argName "-methodprefix" -nrargs 1 -type tclobj}
  {-argName "-objscope"}
  {-argName "-onerror" -nrargs 1 -type tclobj}
  {-argName "-verbose"}
  {-argName "target" -type tclobj}
  {-argName "args" -type args}
}
*/
static int XOTclForwardCmd(Tcl_Interp *interp, 
                           XOTclObject *object, int withPer_object,
                           Tcl_Obj *methodObj,
                           Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix,
                           int withObjscope, Tcl_Obj *withOnerror, int withVerbose, 
                           Tcl_Obj *target, int nobjc, Tcl_Obj *CONST nobjv[]) {
  ForwardCmdClientData *tcd = NULL;
  int result;

  result = forwardProcessOptions(interp, methodObj,
                                 withDefault, withEarlybinding, withMethodprefix,
                                 withObjscope, withOnerror, withVerbose,
                                 target, nobjc, nobjv, &tcd);
  if (result == TCL_OK) {
    CONST char *methodName = NSTail(ObjStr(methodObj));
    XOTclClass *cl = 
      (withPer_object || ! XOTclObjectIsClass(object)) ? 
      NULL : (XOTclClass *)object;
    
    tcd->object = object;
    if (cl == NULL) {
      result = XOTclAddObjectMethod(interp, (XOTcl_Object *)object, methodName,
                                    (Tcl_ObjCmdProc*)XOTclForwardMethod,
                                    (ClientData)tcd, forwardCmdDeleteProc, 0);
    } else {
      result = XOTclAddClassMethod(interp, (XOTcl_Class*)cl, methodName,
                                      (Tcl_ObjCmdProc*)XOTclForwardMethod,
                                      (ClientData)tcd, forwardCmdDeleteProc, 0);
    }
    if (result == TCL_OK) {
      result = ListMethodHandle(interp, object, cl == NULL, methodName);
    }
  } 

  if (result != TCL_OK) {
    forwardCmdDeleteProc((ClientData)tcd);
  }
  return result;
}

/*
xotclCmd importvar XOTclImportvarCmd {
  {-argName "object" -type object}
  {-argName "args" -type args}
}
*/
static int
XOTclImportvar(Tcl_Interp *interp, XOTclObject *object, const char *cmdName, int objc, Tcl_Obj *CONST objv[]) {
  int i, result = TCL_OK;

  for (i=0; i<objc && result == TCL_OK; i++) {
    Tcl_Obj  **ov;
    int oc;

    /*fprintf(stderr, "ListGetElements %p %s\n", objv[i], ObjStr(objv[i]));*/
    if ((result = Tcl_ListObjGetElements(interp, objv[i], &oc, &ov)) == TCL_OK) {
      Tcl_Obj *varname = NULL, *alias = NULL;
      switch (oc) {
      case 0: {varname = objv[i]; break;}
      case 1: {varname = ov[0];   break;}
      case 2: {varname = ov[0];   alias = ov[1]; break;}
      }
      if (varname) {
        result = GetInstVarIntoCurrentScope(interp, cmdName, object, varname, alias);
      } else {
        result = XOTclVarErrMsg(interp, "invalid variable specification '",
                                ObjStr(objv[i]), "'", (char *) NULL);
      }
    }
  }
  return result;
}

static int
XOTclImportvarCmd(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]) {
  return XOTclImportvar(interp, object, "importvar", objc, objv);
}


/*
xotclCmd interp XOTclInterpObjCmd {
  {-argName "name"}
  {-argName "args" -type allargs}
}
*/
/* create a slave interp that calls XOTcl Init */
static int
XOTclInterpObjCmd(Tcl_Interp *interp, CONST char *name, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Interp *slave;

  /* create a fresh Tcl interpreter, or pass command to an existing one */
  if (XOTclCallCommand(interp, XOTE_INTERP, objc, objv) != TCL_OK) {
    return TCL_ERROR;
  }

  if (isCreateString(name)) {
    /*
     * The command was an interp create, so perform an Nx_Init() on
     * the new interpreter
     */
    slave = Tcl_GetSlave(interp, ObjStr(objv[2]));
    if (!slave) {
      return XOTclVarErrMsg(interp, "Creation of slave interpreter failed", (char *) NULL);
    }
    if (Nx_Init(slave) == TCL_ERROR) {
      return TCL_ERROR;
    }
#ifdef XOTCL_MEM_COUNT
    xotclMemCountInterpCounter++;
#endif
  }
  return TCL_OK;
}

/*
xotclCmd invalidateobjectparameter XOTclInvalidateObjectParameterCmd {
  {-argName "class" -type class}
}
*/
static int XOTclInvalidateObjectParameterCmd(Tcl_Interp *interp, XOTclClass *cl) {
  if (cl->parsedParamPtr) {
    /*fprintf(stderr, "   %s invalidate %p\n", className(cl), cl->parsedParamPtr);*/
    ParsedParamFree(cl->parsedParamPtr);
    cl->parsedParamPtr = NULL;
  }
  return TCL_OK;
}

/*
xotclCmd is XOTclIsCmd {
  {-argName "value" -required 1 -type tclobj}
  {-argName "constraint" -required 1 -type tclobj}
  {-argName "-hasmixin" -required 0 -nrargs 1 -type tclobj}
  {-argName "-type" -required 0 -nrargs 1 -type tclobj}
  {-argName "arg" -required 0 -type tclobj}
}
*/
static int XOTclIsCmd(Tcl_Interp *interp, Tcl_Obj *valueObj, Tcl_Obj *constraintObj, 
                       Tcl_Obj *withHasmixin, Tcl_Obj *withType, Tcl_Obj *arg) {
  int result = TCL_OK, success;
  CONST char *constraintString = ObjStr(constraintObj);
  XOTclObject *object;
  XOTclClass *typeClass, *mixinClass;

  if (isTypeString(constraintString)) {
    if (arg== NULL) return XOTclObjErrArgCnt(interp, NULL, NULL, "type <object> <type>");
    success = (GetObjectFromObj(interp, valueObj, &object) == TCL_OK)
      && (GetClassFromObj(interp, arg, &typeClass, NULL) == TCL_OK)
      && isSubType(object->cl, typeClass);

    Tcl_SetIntObj(Tcl_GetObjResult(interp), success);

  } else if (withHasmixin || withType) {
    if ((!isObjectString(constraintString) && !isClassString(constraintString)) || arg != NULL) {
      return XOTclObjErrArgCnt(interp, NULL, NULL, "object|class <object> ?-hasmixin cl? ?-type cl?");
    }
    if (*constraintString == 'o') {
      success = (GetObjectFromObj(interp, valueObj, &object) == TCL_OK);
    } else {
      success = (GetClassFromObj(interp, valueObj, (XOTclClass **)&object, NULL) == TCL_OK);
    }
    if (success && withType) {
      success = (GetClassFromObj(interp, withType, &typeClass, NULL) == TCL_OK)
        && isSubType(object->cl, typeClass);
    }
    if (success && withHasmixin) {
      success = (GetClassFromObj(interp, withHasmixin, &mixinClass, NULL) == TCL_OK)
        && hasMixin(interp, object, mixinClass);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), success);

  } else if (arg != NULL) {
    Tcl_Obj *paramObj =   Tcl_DuplicateObj(valueObj);

    INCR_REF_COUNT(paramObj);
    Tcl_AppendLimitedToObj(paramObj, ",arg=", 5, INT_MAX, NULL);
    Tcl_AppendObjToObj(paramObj, arg);
    
    result = XOTclParametercheckCmd(interp, 1, paramObj, valueObj);
    DECR_REF_COUNT(paramObj);
  } else {
    result = XOTclParametercheckCmd(interp, 1, constraintObj, valueObj);
  }

  return result;
}

/*
xotclCmd method XOTclMethodCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-inner-namespace"}
  {-argName "-per-object"}
  {-argName "-public"}
  {-argName "name" -required 1 -type tclobj}
  {-argName "args" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
  {-argName "-precondition"  -nrargs 1 -type tclobj}
  {-argName "-postcondition" -nrargs 1 -type tclobj}
}
*/
static int XOTclMethodCmd(Tcl_Interp *interp, XOTclObject *object,
                          int withInner_namespace, int withPer_object, int withPublic,
                          Tcl_Obj *nameObj, Tcl_Obj *args, Tcl_Obj *body,
                          Tcl_Obj *withPrecondition, Tcl_Obj *withPostcondition) {
  XOTclClass *cl = 
    (withPer_object || ! XOTclObjectIsClass(object)) ? 
    NULL : (XOTclClass *)object;

  if (cl == 0) {
    requireObjNamespace(interp, object);
  }
  return MakeMethod(interp, object, cl, nameObj, args, body,
                    withPrecondition, withPostcondition,
                    withPublic, withInner_namespace);
}

/*
xotclCmd methodproperty XOTclMethodPropertyCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "methodproperty" -required 1 -type "class-only|protected|redefine-protected|returns|slotobj"}
  {-argName "value" -type tclobj}
}
*/
static int XOTclMethodPropertyCmd(Tcl_Interp *interp, XOTclObject *object, int withPer_object,
                                  Tcl_Obj *methodObj, int methodproperty, Tcl_Obj *valueObj) {
  CONST char *methodName = ObjStr(methodObj);
  Tcl_Command cmd = NULL;
  
  /*fprintf(stderr, "methodProperty for method '%s' prop %d value %s\n",
    methodName, methodproperty, valueObj ? ObjStr(valueObj) : "NULL");*/

  if (*methodName == ':') {
    cmd = Tcl_GetCommandFromObj(interp, methodObj);
    if (!cmd) {
      return XOTclVarErrMsg(interp, "Cannot lookup object method '",
                            methodName, "' for object ", objectName(object),
                            (char *) NULL);
    }
  } else {
    XOTclClass *cl;

    if (withPer_object) {
      cl = NULL;
    } else {
      cl = XOTclObjectIsClass(object) ? (XOTclClass *)object : NULL;
    }

    if (cl == NULL) {
      if (object->nsPtr)
        cmd = FindMethod(object->nsPtr, methodName);
      if (!cmd) {
        return XOTclVarErrMsg(interp, "Cannot lookup object method '",
                              methodName, "' for object ", objectName(object),
                              (char *) NULL);
      }
    } else {
      if (cl->nsPtr)
        cmd = FindMethod(cl->nsPtr, methodName);
      if (!cmd)
        return XOTclVarErrMsg(interp, "Cannot lookup method '",
                              methodName, "' from class  ", objectName(object),
                              (char *) NULL);
    }
  }

  switch (methodproperty) {
  case MethodpropertyClass_onlyIdx: /* fall through */
  case MethodpropertyProtectedIdx: /* fall through */
  case MethodpropertyRedefine_protectedIdx: 
    {
      int flag = methodproperty == MethodpropertyProtectedIdx ?
	XOTCL_CMD_PROTECTED_METHOD :
	methodproperty == MethodpropertyRedefine_protectedIdx ?
	XOTCL_CMD_REDEFINE_PROTECTED_METHOD 
	:XOTCL_CMD_CLASS_ONLY_METHOD;
      
      if (valueObj) {
	int bool, result;
	result = Tcl_GetBooleanFromObj(interp, valueObj, &bool);
	if (result != TCL_OK) {
	  return result;
	}
	if (bool) {
	  Tcl_Command_flags(cmd) |= flag;
	} else {
	  Tcl_Command_flags(cmd) &= ~flag;
	}
      }
      Tcl_SetIntObj(Tcl_GetObjResult(interp), (Tcl_Command_flags(cmd) & flag) != 0);
      break;
    }
  case MethodpropertySlotobjIdx: 
  case MethodpropertyReturnsIdx: 
    {
      XOTclParamDefs *paramDefs;
      Tcl_Obj **objPtr;

      if (valueObj == NULL && methodproperty == MethodpropertySlotobjIdx) {
	return XOTclVarErrMsg(interp, "Option 'slotobj' of method ", methodName,
			      " requires argument '", (char *) NULL);
      }

      paramDefs = ParamDefsGet(cmd);
      /*fprintf(stderr, "MethodProperty, ParamDefsGet cmd %p paramDefs %p returns %p\n", 
	cmd, paramDefs, paramDefs?paramDefs->returns:NULL);*/

      if (paramDefs == NULL) {
	paramDefs = ParamDefsNew();
	ParamDefsStore(interp, cmd, paramDefs);
	/*fprintf(stderr, "new param defs %p for cmd %p %s\n", paramDefs, cmd, methodName);*/
      }
      objPtr = methodproperty == MethodpropertySlotobjIdx ? &paramDefs->slotObj : &paramDefs->returns;
      if (valueObj == NULL) {
	/* must be a returns query */
	Tcl_SetObjResult(interp, *objPtr ? *objPtr : XOTclGlobalObjs[XOTE_EMPTY]);
      } else {
	const char *valueString = ObjStr(valueObj);
	/* Set a new value; if there is already a value, free it */
	if (*objPtr) {
	  DECR_REF_COUNT(*objPtr);
	}
	if (*valueString == '\0') {
	  /* set the value to NULL */
	  *objPtr = NULL;
	} else {
	  *objPtr = valueObj;
	  INCR_REF_COUNT(*objPtr);
	}
      }
      break;
    }
  }

  return TCL_OK;
}

/*
xotclCmd my XOTclMyCmd {
  {-argName "-local"}
  {-argName "method" -required 1 -type tclobj}
  {-argName "args" -type args}
}
*/
static int XOTclMyCmd(Tcl_Interp *interp, int withLocal, Tcl_Obj *methodObj, int nobjc, Tcl_Obj *CONST nobjv[]) {
  XOTclObject *self = GetSelfObj(interp);
  int result;

  if (!self) {
    return XOTclVarErrMsg(interp, "Cannot resolve 'self', probably called outside the context of an XOTcl Object",
                          (char *) NULL);
  }

  if (withLocal) {
    XOTclClass *cl = self->cl;
    CONST char *methodName = ObjStr(methodObj);
    Tcl_Command cmd = FindMethod(cl->nsPtr, methodName);
    if (cmd == NULL) {
      return XOTclVarErrMsg(interp, objectName(self),
                            ": unable to dispatch local method '",
                            methodName, "' in class ", className(cl),
                            (char *) NULL);
    }
    result = MethodDispatch((ClientData)self, interp, nobjc+2, nobjv, cmd, self, cl,
                          methodName, 0);
  } else {
    result = callMethod((ClientData)self, interp, methodObj, nobjc+2, nobjv, 0);
  }
  return result;
}

/*
xotclCmd namespace_copycmds XOTclNSCopyCmds {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
*/
static int XOTclNSCopyCmds(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs) {
  Tcl_Command cmd;
  Tcl_Obj *newFullCmdName, *oldFullCmdName;
  CONST char *newName, *oldName, *name;
  Tcl_Namespace *fromNsPtr, *toNsPtr;
  Tcl_HashTable *cmdTable;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  XOTclObject *object;
  XOTclClass *cl;
  int fromClassNS;

  fromNsPtr = ObjFindNamespace(interp, fromNs);
  if (!fromNsPtr)
    return TCL_OK;

  name = ObjStr(fromNs);

  /* check, if we work on an object or class namespace */
  object = GetObjectFromNsName(interp, name, &fromClassNS);

  if (object == NULL) {
    return XOTclVarErrMsg(interp, "argument 1 '", ObjStr(fromNs), "' is not an object",
                          NULL);
  }

  cl = fromClassNS ? (XOTclClass *)object : NULL;

  /*  object = XOTclpGetObject(interp, ObjStr(fromNs));*/

  toNsPtr = ObjFindNamespace(interp, toNs);
  if (!toNsPtr)
    return XOTclVarErrMsg(interp, "CopyCmds: Destination namespace ",
                          ObjStr(toNs), " does not exist", (char *) NULL);
  /*
   * copy all procs & commands in the ns
   */
  cmdTable = Tcl_Namespace_cmdTable(fromNsPtr);
  hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch);
  while (hPtr) {
    /*fprintf(stderr, "copy cmdTable = %p, first=%p\n", cmdTable, hPtr);*/
    name = Tcl_GetHashKey(cmdTable, hPtr);

    /*
     * construct full cmd names
     */
    newFullCmdName = Tcl_NewStringObj(toNsPtr->fullName, -1);
    oldFullCmdName = Tcl_NewStringObj(fromNsPtr->fullName, -1);

    INCR_REF_COUNT(newFullCmdName); INCR_REF_COUNT(oldFullCmdName);
    Tcl_AppendStringsToObj(newFullCmdName, "::", name, (char *) NULL);
    Tcl_AppendStringsToObj(oldFullCmdName, "::", name, (char *) NULL);
    newName = ObjStr(newFullCmdName);
    oldName = ObjStr(oldFullCmdName);

    /*fprintf(stderr, "try to copy command from '%s' to '%s'\n", oldName, newName);*/
    /*
     * Make sure that the destination command does not already exist.
     * Otherwise: do not copy
     */
    cmd = Tcl_FindCommand(interp, newName, 0, 0);
    if (cmd) {
      /*fprintf(stderr, "%s already exists\n", newName);*/
      if (!XOTclpGetObject(interp, newName)) {
        /* command or scripted method will be deleted & then copied */
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

      if (CmdIsProc(cmd)) {
        Proc *procPtr = (Proc*) Tcl_Command_objClientData(cmd);
        Tcl_Obj *arglistObj;
        int result;

        /*
         * Build a list containing the arguments of the proc
         */
        result = ListCmdParams(interp, cmd, oldName, 0);
        if (result != TCL_OK) {
          return result;
        }

        arglistObj = Tcl_GetObjResult(interp);
        INCR_REF_COUNT(arglistObj);

        if (Tcl_Command_objProc(cmd) == RUNTIME_STATE(interp)->objInterpProc) {
          Tcl_DString ds, *dsPtr = &ds;

          if (cl) {
            /* XOTcl class-methods */
            XOTclProcAssertion *procs;
            procs = cl->opt ? AssertionFindProcs(cl->opt->assertions, name) : 0;
            
            DSTRING_INIT(dsPtr);
            Tcl_DStringAppendElement(dsPtr, "::nsf::method");
            Tcl_DStringAppendElement(dsPtr, NSCutXOTclClasses(toNsPtr->fullName));
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
            /* XOTcl object-methods */
            XOTclObject *object = XOTclpGetObject(interp, fromNsPtr->fullName);
            XOTclProcAssertion *procs;

            if (object) {
              procs = object->opt ? AssertionFindProcs(object->opt->assertions, name) : 0;
            } else {
              DECR_REF_COUNT(newFullCmdName);
              DECR_REF_COUNT(oldFullCmdName);
              DECR_REF_COUNT(arglistObj);
              return XOTclVarErrMsg(interp, "No object for assertions", (char *) NULL);
            }

            DSTRING_INIT(dsPtr);
            Tcl_DStringAppendElement(dsPtr, "::nsf::method");
            Tcl_DStringAppendElement(dsPtr, toNsPtr->fullName);
            Tcl_DStringAppendElement(dsPtr, "-per-object");
            Tcl_DStringAppendElement(dsPtr, name);
            Tcl_DStringAppendElement(dsPtr, ObjStr(arglistObj));
            Tcl_DStringAppendElement(dsPtr, StripBodyPrefix(ObjStr(procPtr->bodyPtr)));
            if (procs) {
              XOTclRequireObjectOpt(object);
              AssertionAppendPrePost(interp, dsPtr, procs);
            }
            Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr), 0);
            DSTRING_FREE(dsPtr);
          }
          DECR_REF_COUNT(arglistObj);
        } else {
          /* Tcl Proc */
          Tcl_VarEval(interp, "proc ", newName, " {", ObjStr(arglistObj), "} {\n",
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
            /* if client data is not null, we would have to copy
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

/*
xotclCmd namespace_copyvars XOTclNSCopyVars {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
*/
static int
XOTclNSCopyVars(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs) {
  Tcl_Namespace *fromNsPtr, *toNsPtr;
  Var *varPtr = NULL;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  TclVarHashTable *varTable;
  XOTclObject *object, *destObject;
  CONST char *destFullName;
  Tcl_Obj *destFullNameObj;
  Tcl_CallFrame frame, *framePtr = &frame;
  Tcl_Obj *varNameObj = NULL;

  fromNsPtr = ObjFindNamespace(interp, fromNs);
  /*fprintf(stderr, "copyvars from %s to %s, ns=%p\n", ObjStr(objv[1]), ObjStr(objv[2]), ns);*/

  if (fromNsPtr) {
    toNsPtr = ObjFindNamespace(interp, toNs);
    if (!toNsPtr)
      return XOTclVarErrMsg(interp, "CopyVars: Destination namespace ",
                            ObjStr(toNs), " does not exist", (char *) NULL);

    object = XOTclpGetObject(interp, ObjStr(fromNs));
    destFullName = toNsPtr->fullName;
    destFullNameObj = Tcl_NewStringObj(destFullName, -1);
    INCR_REF_COUNT(destFullNameObj);
    varTable = Tcl_Namespace_varTable(fromNsPtr);
    Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, toNsPtr, 0);
  } else {
    XOTclObject *newObject;
    if (GetObjectFromObj(interp, fromNs, &object) != TCL_OK) {
      return XOTclVarErrMsg(interp, "CopyVars: Origin object/namespace ",
                            ObjStr(fromNs), " does not exist", (char *) NULL);
    }
    if (GetObjectFromObj(interp, toNs, &newObject) != TCL_OK) {
      return XOTclVarErrMsg(interp, "CopyVars: Destination object/namespace ",
                            ObjStr(toNs), " does not exist", (char *) NULL);
    }
    varTable = object->varTable;
    destFullNameObj = newObject->cmdName;
    destFullName = ObjStr(destFullNameObj);
  }

  destObject = XOTclpGetObject(interp, destFullName);

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

        if (object) {
          /* fprintf(stderr, "copy in obj %s var %s val '%s'\n", objectName(object), ObjStr(varNameObj),
	     ObjStr(valueOfVar(Tcl_Obj, varPtr, objPtr)));*/

          /* can't rely on "set", if there are multiple object systems */
          setInstVar(interp, destObject, varNameObj, valueOfVar(Tcl_Obj, varPtr, objPtr));
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
              if (object) {
                XOTcl_ObjSetVar2((XOTcl_Object*)destObject, interp, varNameObj, eltNameObj,
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
  if (fromNsPtr) {
    DECR_REF_COUNT(destFullNameObj);
    Tcl_PopCallFrame(interp);
  }
  return TCL_OK;
}

/*
xotclCmd objectproperty XOTclObjectpropertyCmd {
  {-argName "object" -required 1 -type tclobj}
  {-argName "objectkind" -type "type|object|class|baseclass|metaclass|hasmixin"}
  {-argName "value" -required 0 -type tclobj}
}
*/
static int XOTclObjectpropertyCmd(Tcl_Interp *interp, Tcl_Obj *obj, int objectkind, Tcl_Obj *valueObj) {
  int success = TCL_ERROR;
  XOTclObject *object;
  XOTclClass *cl;

  /* fprintf(stderr, "XOTclObjectpropertyCmd\n");*/

  switch (objectkind) {
  case ObjectkindTypeIdx:
    if (valueObj == NULL) return XOTclObjErrArgCnt(interp, NULL, NULL, "<object> type <type>");
    if (GetObjectFromObj(interp, obj, &object) != TCL_OK) {
      return XOTclObjErrType(interp, obj, "object", "object");
    }
    if (GetClassFromObj(interp, valueObj, &cl, NULL) != TCL_OK) {
	return XOTclObjErrType(interp, valueObj, "class", "type");
    }
    success = isSubType(object->cl, cl);
    break;

  case ObjectkindObjectIdx:
    if (valueObj != NULL) return XOTclObjErrArgCnt(interp, NULL, NULL, "<object> object");
    success = (GetObjectFromObj(interp, obj, &object) == TCL_OK);
    break;

  case ObjectkindClassIdx:
    if (valueObj != NULL) return XOTclObjErrArgCnt(interp, NULL, NULL, "<class> class");
    success = (GetObjectFromObj(interp, obj, &object) == TCL_OK) && XOTclObjectIsClass(object);
    break;

  case ObjectkindMetaclassIdx:
    if (valueObj != NULL) return XOTclObjErrArgCnt(interp, NULL, NULL, "<class> metaclass");
    success = (GetObjectFromObj(interp, obj, &object) == TCL_OK)
      && XOTclObjectIsClass(object)
      && IsMetaClass(interp, (XOTclClass*)object, 1);
    break;

  case ObjectkindBaseclassIdx:
    if (valueObj != NULL) return XOTclObjErrArgCnt(interp, NULL, NULL, "<object> baseclass");
    success = (GetObjectFromObj(interp, obj, &object) == TCL_OK)
      && XOTclObjectIsClass(object)
      && IsBaseClass((XOTclClass*)object);
    break;

  case ObjectkindHasmixinIdx:
    if (valueObj == NULL) return XOTclObjErrArgCnt(interp, NULL, NULL, "<object> hasmixin <class>");
    success = (GetObjectFromObj(interp, obj, &object) == TCL_OK)
      && (GetClassFromObj(interp, valueObj, &cl, NULL) == TCL_OK)
      && hasMixin(interp, object, cl);
    break;
  }


  Tcl_SetIntObj(Tcl_GetObjResult(interp), success);
  return TCL_OK;
}

/*
xotclCmd __qualify XOTclQualifyObjCmd {
  {-argName "name" -required 1 -type tclobj}
}
*/
static int XOTclQualifyObjCmd(Tcl_Interp *interp, Tcl_Obj *nameObj) {
  CONST char *nameString = ObjStr(nameObj);

  if (isAbsolutePath(nameString)) {
    Tcl_SetObjResult(interp, nameObj);
  } else {
    Tcl_SetObjResult(interp, NameInNamespaceObj(interp, nameString, callingNameSpace(interp)));
  }
  return TCL_OK;
}

/*
xotclCmd relation XOTclRelationCmd {
  {-argName "object" -type object}
  {-argName "relationtype" -required 1 -type "object-mixin|class-mixin|object-filter|class-filter|class|superclass|rootclass"}
  {-argName "value" -required 0 -type tclobj}
}
*/
static int XOTclRelationCmd(Tcl_Interp *interp, XOTclObject *object, 
                            int relationtype, Tcl_Obj *valueObj) {
  int oc; Tcl_Obj **ov;
  XOTclObject *nObject = NULL;
  XOTclClass *cl = NULL;
  XOTclObjectOpt *objopt = NULL;
  XOTclClassOpt *clopt = NULL, *nclopt = NULL;
  int i;

  /* fprintf(stderr, "XOTclRelationCmd %s rel=%d val='%s'\n",
     objectName(object), relationtype, valueObj ? ObjStr(valueObj) : "NULL");*/

  if (relationtype == RelationtypeClass_mixinIdx || 
      relationtype == RelationtypeClass_filterIdx) {
    if (XOTclObjectIsClass(object)) {
      cl = (XOTclClass *)object;
    } else {
      /* fall back to per-object case */
      relationtype = (relationtype == RelationtypeClass_mixinIdx) ?
	RelationtypeObject_mixinIdx :
	RelationtypeObject_filterIdx ;
    }
  }
    
  switch (relationtype) {
  case RelationtypeObject_filterIdx:
  case RelationtypeObject_mixinIdx:
    if (valueObj == NULL) {
      objopt = object->opt;
      switch (relationtype) {
      case RelationtypeObject_mixinIdx:
        return objopt ? MixinInfo(interp, objopt->mixins, NULL, 1, NULL) : TCL_OK;
      case RelationtypeObject_filterIdx:
        return objopt ? FilterInfo(interp, objopt->filters, NULL, 1, 0) : TCL_OK;
      }
    }
    if (Tcl_ListObjGetElements(interp, valueObj, &oc, &ov) != TCL_OK)
      return TCL_ERROR;
    objopt = XOTclRequireObjectOpt(object);
    break;

  case RelationtypeClass_mixinIdx:
  case RelationtypeClass_filterIdx:

    if (valueObj == NULL) {
      clopt = cl->opt;
      switch (relationtype) {
      case RelationtypeClass_mixinIdx: 
        return clopt ? MixinInfo(interp, clopt->classmixins, NULL, 1, NULL) : TCL_OK;
      case RelationtypeClass_filterIdx:
        return objopt ? FilterInfo(interp, clopt->classfilters, NULL, 1, 0) : TCL_OK;
      }
    }

    if (Tcl_ListObjGetElements(interp, valueObj, &oc, &ov) != TCL_OK)
      return TCL_ERROR;
    clopt = XOTclRequireClassOpt(cl);
    break;

  case RelationtypeSuperclassIdx:
    if (!XOTclObjectIsClass(object))
      return XOTclObjErrType(interp, object->cmdName, "class", "relationtype");
    cl = (XOTclClass *)object;
    if (valueObj == NULL) {
      return ListSuperclasses(interp, cl, NULL, 0);
    }
    if (Tcl_ListObjGetElements(interp, valueObj, &oc, &ov) != TCL_OK)
      return TCL_ERROR;
    return SuperclassAdd(interp, cl, oc, ov, valueObj, cl->object.cl);

  case RelationtypeClassIdx:
    if (valueObj == NULL) {
      Tcl_SetObjResult(interp, object->cl->object.cmdName);
      return TCL_OK;
    }
    GetClassFromObj(interp, valueObj, &cl, object->cl);
    if (!cl) return XOTclErrBadVal(interp, "class", "a class", objectName(object));
    return changeClass(interp, object, cl);

  case RelationtypeRootclassIdx:
    {
    XOTclClass *metaClass;

    if (!XOTclObjectIsClass(object))
      return XOTclObjErrType(interp, object->cmdName, "class", "relationtype");
    cl = (XOTclClass *)object;

    if (valueObj == NULL) {
      return XOTclVarErrMsg(interp, "metaclass must be specified as third argument",
                            (char *) NULL);
    }
    GetClassFromObj(interp, valueObj, &metaClass, NULL);
    if (!metaClass) return XOTclObjErrType(interp, valueObj, "class", "");

    cl->object.flags |= XOTCL_IS_ROOT_CLASS;
    metaClass->object.flags |= XOTCL_IS_ROOT_META_CLASS;

    return TCL_OK;

    /* todo:
       need to remove these properties?
       allow to delete a classystem at runtime?
    */
    }
  }

  switch (relationtype) {
  case RelationtypeObject_mixinIdx:
    {
      XOTclCmdList *newMixinCmdList = NULL;

      for (i = 0; i < oc; i++) {
        if (MixinAdd(interp, &newMixinCmdList, ov[i], object->cl->object.cl) != TCL_OK) {
          CmdListRemoveList(&newMixinCmdList, GuardDel);
          return TCL_ERROR;
        }
      }

      if (objopt->mixins) {
        XOTclCmdList *cmdlist, *del;
        for (cmdlist = objopt->mixins; cmdlist; cmdlist = cmdlist->nextPtr) {
          cl = XOTclGetClassFromCmdPtr(cmdlist->cmdPtr);
          clopt = cl ? cl->opt : NULL;
          if (clopt) {
            del = CmdListFindCmdInList(object->id, clopt->isObjectMixinOf);
            if (del) {
              /* fprintf(stderr, "Removing object %s from isObjectMixinOf of class %s\n",
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
      objopt->mixins = newMixinCmdList;
      for (i = 0; i < oc; i++) {
        Tcl_Obj *ocl = NULL;
        
        /* fprintf(stderr, "Added to mixins of %s: %s\n", objectName(object), ObjStr(ov[i])); */
        Tcl_ListObjIndex(interp, ov[i], 0, &ocl);
        GetObjectFromObj(interp, ocl, &nObject);
        if (nObject) {
          /* fprintf(stderr, "Registering object %s to isObjectMixinOf of class %s\n",
             objectName(object), objectName(nObject)); */
          nclopt = XOTclRequireClassOpt((XOTclClass*)nObject);
          CmdListAdd(&nclopt->isObjectMixinOf, object->id, NULL, /*noDuplicates*/ 1);
        } /* else fprintf(stderr, "Problem registering %s as a mixinof of %s\n",
             ObjStr(ov[i]), className(cl)); */
      }
      
      MixinComputeDefined(interp, object);
      FilterComputeDefined(interp, object);
      break;
    }

  case RelationtypeObject_filterIdx:

    if (objopt->filters) CmdListRemoveList(&objopt->filters, GuardDel);

    object->flags &= ~XOTCL_FILTER_ORDER_VALID;
    for (i = 0; i < oc; i ++) {
      if (FilterAdd(interp, &objopt->filters, ov[i], object, 0) != TCL_OK)
        return TCL_ERROR;
    }
    /*FilterComputeDefined(interp, object);*/
    break;

  case RelationtypeClass_mixinIdx:
    {
      XOTclCmdList *newMixinCmdList = NULL;

      for (i = 0; i < oc; i++) {
        if (MixinAdd(interp, &newMixinCmdList, ov[i], cl->object.cl) != TCL_OK) {
          CmdListRemoveList(&newMixinCmdList, GuardDel);
          return TCL_ERROR;
        }
      }
      if (clopt->classmixins) {
        RemoveFromClassMixinsOf(cl->object.id, clopt->classmixins);
        CmdListRemoveList(&clopt->classmixins, GuardDel);
      }

      MixinInvalidateObjOrders(interp, cl);
      /*
       * since mixin procs may be used as filters,
       * we have to invalidate the filters as well
       */
      FilterInvalidateObjOrders(interp, cl);
      clopt->classmixins = newMixinCmdList;
      for (i = 0; i < oc; i++) {
        Tcl_Obj *ocl = NULL;
        /* fprintf(stderr, "Added to classmixins of %s: %s\n",
           className(cl), ObjStr(ov[i])); */
        
        Tcl_ListObjIndex(interp, ov[i], 0, &ocl);
        GetObjectFromObj(interp, ocl, &nObject);
        if (nObject) {
          /* fprintf(stderr, "Registering class %s to isClassMixinOf of class %s\n",
             className(cl), objectName(nObject)); */
          nclopt = XOTclRequireClassOpt((XOTclClass*) nObject);
          CmdListAdd(&nclopt->isClassMixinOf, cl->object.id, NULL, /*noDuplicates*/ 1);
        } /* else fprintf(stderr, "Problem registering %s as a class-mixin of %s\n",
             ObjStr(ov[i]), className(cl)); */
      }
      break;
    }

  case RelationtypeClass_filterIdx:

    if (clopt->classfilters) CmdListRemoveList(&clopt->classfilters, GuardDel);

    FilterInvalidateObjOrders(interp, cl);
    for (i = 0; i < oc; i ++) {
      if (FilterAdd(interp, &clopt->classfilters, ov[i], 0, cl) != TCL_OK)
        return TCL_ERROR;
    }
    break;

  }
  return TCL_OK;
}

/*
xotclCmd current XOTclCurrentCmd {
  {-argName "currentoption" -required 0 -type "proc|method|object|class|activelevel|args|activemixin|calledproc|calledmethod|calledclass|callingproc|callingmethod|callingclass|callinglevel|callingobject|filterreg|isnextcall|next"}
}
*/
static int XOTclCurrentCmd(Tcl_Interp *interp, int selfoption) {
  XOTclObject *object =  GetSelfObj(interp);
  XOTclCallStackContent *cscPtr;
  int result = TCL_OK;

  /*fprintf(stderr, "getSelfObj returns %p\n", object); tcl85showStack(interp);*/

  if (selfoption == 0 || selfoption == CurrentoptionObjectIdx) {
    if (object) {
      Tcl_SetObjResult(interp, object->cmdName);
      return TCL_OK;
    } else {
      return XOTclVarErrMsg(interp,  "No current object", (char *) NULL);
    }
  }

  if (!object && selfoption != CurrentoptionCallinglevelIdx) {
    return XOTclVarErrMsg(interp, "No current object", (char *) NULL);
  }

  switch (selfoption) {
  case CurrentoptionMethodIdx: /* fall through */
  case CurrentoptionProcIdx:
    cscPtr = CallStackGetTopFrame(interp, NULL);
    if (cscPtr) {
      CONST char *procName = Tcl_GetCommandName(interp, cscPtr->cmdPtr);
      Tcl_SetResult(interp, (char *)procName, TCL_VOLATILE);
    } else {
      return XOTclVarErrMsg(interp, "Can't find proc", (char *) NULL);
    }
    break;

  case CurrentoptionClassIdx: /* class subcommand */
    cscPtr = CallStackGetTopFrame(interp, NULL);
    Tcl_SetObjResult(interp, cscPtr->cl ? cscPtr->cl->object.cmdName : XOTclGlobalObjs[XOTE_EMPTY]);
    break;

  case CurrentoptionActivelevelIdx:
    Tcl_SetObjResult(interp, computeLevelObj(interp, ACTIVE_LEVEL));
    break;

  case CurrentoptionArgsIdx: {
    int nobjc;
    Tcl_Obj **nobjv;
    Tcl_CallFrame *topFramePtr;

    cscPtr = CallStackGetTopFrame(interp, &topFramePtr);
    if (cscPtr->objv) {
      nobjc = cscPtr->objc;
      nobjv = cscPtr->objv;
    } else {
      nobjc = Tcl_CallFrame_objc(topFramePtr);
      nobjv = (Tcl_Obj **)Tcl_CallFrame_objv(topFramePtr);
    }
    Tcl_SetObjResult(interp, Tcl_NewListObj(nobjc-1, nobjv+1));
    break;
  }

  case CurrentoptionActivemixinIdx: {
    XOTclObject *object = NULL;
    if (RUNTIME_STATE(interp)->cmdPtr) {
      object = XOTclGetObjectFromCmdPtr(RUNTIME_STATE(interp)->cmdPtr);
    }
    Tcl_SetObjResult(interp, object ? object->cmdName : XOTclGlobalObjs[XOTE_EMPTY]);
    break;
  }

  case CurrentoptionCalledprocIdx:
  case CurrentoptionCalledmethodIdx: 
    cscPtr = CallStackFindActiveFilter(interp);
    if (cscPtr) {
      Tcl_SetObjResult(interp, cscPtr->filterStackEntry->calledProc);
    } else {
      result = XOTclVarErrMsg(interp, "called from outside of a filter",
			  (char *) NULL);
    }
    break;
    
  case CurrentoptionCalledclassIdx:
    Tcl_SetResult(interp, className(FindCalledClass(interp, object)), TCL_VOLATILE);
    break;

  case CurrentoptionCallingmethodIdx:
  case CurrentoptionCallingprocIdx:
    cscPtr = XOTclCallStackFindLastInvocation(interp, 1, NULL);
    Tcl_SetResult(interp, cscPtr ? (char *)Tcl_GetCommandName(interp, cscPtr->cmdPtr) : "",
		  TCL_VOLATILE);
    break;

  case CurrentoptionCallingclassIdx:
    cscPtr = XOTclCallStackFindLastInvocation(interp, 1, NULL);
    Tcl_SetObjResult(interp, cscPtr && cscPtr->cl ? cscPtr->cl->object.cmdName :
		     XOTclGlobalObjs[XOTE_EMPTY]);
    break;

  case CurrentoptionCallinglevelIdx:
    if (!object) {
      Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    } else {
      Tcl_SetObjResult(interp, computeLevelObj(interp, CALLING_LEVEL));
    }
    break;

  case CurrentoptionCallingobjectIdx:
    cscPtr = XOTclCallStackFindLastInvocation(interp, 1, NULL);
    Tcl_SetObjResult(interp, cscPtr ? cscPtr->self->cmdName : XOTclGlobalObjs[XOTE_EMPTY]);
    break;

  case CurrentoptionFilterregIdx:
    cscPtr = CallStackFindActiveFilter(interp);
    if (cscPtr) {
      Tcl_SetObjResult(interp, FilterFindReg(interp, object, cscPtr->cmdPtr));
    } else {
      result = XOTclVarErrMsg(interp,
                          "called from outside of a filter",
                          (char *) NULL);
    }
    break;

  case CurrentoptionIsnextcallIdx: {
    Tcl_CallFrame *framePtr;
    cscPtr = CallStackGetTopFrame(interp, &framePtr);
    framePtr = nextFrameOfType(Tcl_CallFrame_callerPtr(framePtr), FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD);
    cscPtr = framePtr ? Tcl_CallFrame_clientData(framePtr) : NULL;

    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (cscPtr && (cscPtr->callType & XOTCL_CSC_CALL_IS_NEXT)));
    break;
  }

  case CurrentoptionNextIdx:
    result = FindSelfNext(interp);
    break;
  }

  return result;
}

/*
xotclCmd setvar XOTclSetVarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "variable" -required 1 -type tclobj}
  {-argName "value" -required 0 -type tclobj}
}
*/
static int XOTclSetVarCmd(Tcl_Interp *interp, XOTclObject *object, Tcl_Obj *variable, Tcl_Obj *valueObj) {
  if (CheckVarName(interp, ObjStr(variable)) != TCL_OK) {
    return TCL_ERROR;
  }
  return setInstVar(interp, object, variable, valueObj);
}

/* 
xotclCmd setter XOTclSetterCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "parameter" -type tclobj}
  }
*/
static int XOTclSetterCmd(Tcl_Interp *interp, XOTclObject *object, int withPer_object, Tcl_Obj *parameter) {
  XOTclClass *cl = (withPer_object || ! XOTclObjectIsClass(object)) ? NULL : (XOTclClass *)object;
  CONST char *methodName = ObjStr(parameter);
  SetterCmdClientData *setterClientData;
  size_t j, length;
  int result;

  if (*methodName == '-') {
    return XOTclVarErrMsg(interp,
                          "method name \"", methodName, "\" must not start with a dash",
                          (char *) NULL);
    
  }

  setterClientData = NEW(SetterCmdClientData);
  setterClientData->paramsPtr = NULL;
  length = strlen(methodName);

  for (j=0; j<length; j++) {
    if (methodName[j] == ':' || methodName[j] == ' ') break;
  }

  if (j < length) {
    /* looks as if we have a parameter specification */
    int result, possibleUnknowns = 0, plainParams = 0;

    setterClientData->paramsPtr = ParamsNew(1);
    result = ParamParse(interp, "setter", parameter, 
                        XOTCL_DISALLOWED_ARG_SETTER|XOTCL_ARG_HAS_DEFAULT,
                        setterClientData->paramsPtr, &possibleUnknowns, &plainParams);

    if (result != TCL_OK) {
      setterCmdDeleteProc((ClientData)setterClientData);
      return result;
    }
    methodName = setterClientData->paramsPtr->name;
  } else {
    setterClientData->paramsPtr = NULL;
  }

  if (cl) {
    result = XOTclAddClassMethod(interp, (XOTcl_Class *)cl, methodName,
                                 (Tcl_ObjCmdProc*)XOTclSetterMethod, 
                                 (ClientData)setterClientData, setterCmdDeleteProc, 0);
  } else {
    result = XOTclAddObjectMethod(interp, (XOTcl_Object *)object, methodName, 
                                  (Tcl_ObjCmdProc*)XOTclSetterMethod, 
                                  (ClientData)setterClientData, setterCmdDeleteProc, 0);
  }
  if (result == TCL_OK) {
    result = ListMethodHandle(interp, object, cl == NULL, methodName);
  } else {
    setterCmdDeleteProc((ClientData)setterClientData);
  }
  return result;
}

typedef struct XOTclParamWrapper {
  XOTclParam *paramPtr;
  int refCount;
  int canFree;
} XOTclParamWrapper;

static Tcl_DupInternalRepProc	ParamDupInteralRep;
static Tcl_FreeInternalRepProc	ParamFreeInternalRep;
static Tcl_UpdateStringProc	ParamUpdateString;

static void ParamUpdateString(Tcl_Obj *objPtr) {
    Tcl_Panic("%s of type %s should not be called", "updateStringProc",
	    objPtr->typePtr->name);
}

static void ParamDupInteralRep(Tcl_Obj *srcPtr, Tcl_Obj *dupPtr) {
  Tcl_Panic("%s of type %s should not be called", "dupStringProc",
	    srcPtr->typePtr->name);
}

static int ParamSetFromAny(Tcl_Interp *interp,	register Tcl_Obj *objPtr);
static Tcl_ObjType paramObjType = {
    "xotclParam",			/* name */
    ParamFreeInternalRep,		/* freeIntRepProc */
    ParamDupInteralRep,			/* dupIntRepProc */
    ParamUpdateString,			/* updateStringProc */
    ParamSetFromAny			/* setFromAnyProc */
};

static void
ParamFreeInternalRep(
    register Tcl_Obj *objPtr)	/* Param structure object with internal
				 * representation to free. */
{
  XOTclParamWrapper *paramWrapperPtr = (XOTclParamWrapper *)objPtr->internalRep.twoPtrValue.ptr1;

  if (paramWrapperPtr != NULL) {
    /* fprintf(stderr, "ParamFreeInternalRep freeing wrapper %p paramPtr %p refCount %dcanFree %d\n",
            paramWrapperPtr, paramWrapperPtr->paramPtr, paramWrapperPtr->refCount, 
            paramWrapperPtr->canFree);*/

    if (paramWrapperPtr->canFree) {
      ParamsFree(paramWrapperPtr->paramPtr);
      FREE(XOTclParamWrapper, paramWrapperPtr);
    } else {
      paramWrapperPtr->refCount--;
    }
  }
}

static int
ParamSetFromAny2(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    const char *varNamePrefix,	/* shows up as varname in error message */
    register Tcl_Obj *objPtr)	/* The object to convert. */
{
  XOTclParamWrapper *paramWrapperPtr = NEW(XOTclParamWrapper);
  Tcl_Obj *fullParamObj = Tcl_NewStringObj(varNamePrefix, -1);
  int result, possibleUnknowns = 0, plainParams = 0;

  paramWrapperPtr->paramPtr = ParamsNew(1);
  paramWrapperPtr->refCount = 1;
  paramWrapperPtr->canFree = 0;
  /*fprintf(stderr, "allocating  %p\n",paramWrapperPtr->paramPtr);*/

  Tcl_AppendLimitedToObj(fullParamObj, ObjStr(objPtr), -1, INT_MAX, NULL);
  INCR_REF_COUNT(fullParamObj);
  result = ParamParse(interp, "valuecheck", fullParamObj, 
                      XOTCL_DISALLOWED_ARG_VALUEECHECK /* disallowed options */,
                      paramWrapperPtr->paramPtr, &possibleUnknowns, &plainParams);
  /* Here, we want to treat currently unknown user level converters as
     error. 
  */
  if (paramWrapperPtr->paramPtr->flags & XOTCL_ARG_CURRENTLY_UNKNOWN) {
    ParamsFree(paramWrapperPtr->paramPtr);
    FREE(XOTclParamWrapper, paramWrapperPtr);
    result = TCL_ERROR;
  } else if (result == TCL_OK) {
    TclFreeIntRep(objPtr);
    objPtr->internalRep.twoPtrValue.ptr1 = (void *)paramWrapperPtr;
    objPtr->internalRep.twoPtrValue.ptr2 = NULL;
    objPtr->typePtr = &paramObjType;    
  }

  DECR_REF_COUNT(fullParamObj);
  return result;
}

static int
ParamSetFromAny(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr)	/* The object to convert. */
{
  return ParamSetFromAny2(interp, "value:", objPtr);
}

static int Parametercheck(Tcl_Interp *interp, Tcl_Obj *objPtr, Tcl_Obj *valueObj,  
			  const char *varNamePrefix, XOTclParam **paramPtrPtr) {
  XOTclParamWrapper *paramWrapperPtr;
  Tcl_Obj *outObjPtr = NULL;
  XOTclParam *paramPtr;
  ClientData checkedData;
  int result, flags = 0;

  /*fprintf(stderr, "XOTclParametercheckCmd %s value %p %s\n",
    ObjStr(objPtr), valueObj, ObjStr(valueObj));*/

  if (objPtr->typePtr == &paramObjType) {
    paramWrapperPtr = (XOTclParamWrapper *) objPtr->internalRep.twoPtrValue.ptr1;
  } else {
    result = ParamSetFromAny2(interp, varNamePrefix, objPtr);
    if (result == TCL_OK) {
      paramWrapperPtr = (XOTclParamWrapper *) objPtr->internalRep.twoPtrValue.ptr1;
    } else {
      return XOTclVarErrMsg(interp,
                            "invalid value constraints \"", ObjStr(objPtr), "\"",
                            (char *) NULL);
    }
  }
  paramPtr = paramWrapperPtr->paramPtr;
  if (paramPtrPtr) *paramPtrPtr = paramPtr;
  result = ArgumentCheck(interp, valueObj, paramPtr, &flags, &checkedData, &outObjPtr);

  /*fprintf(stderr, "XOTclParametercheckCmd paramPtr %p final refcount of wrapper %d can free %d\n",
    paramPtr, paramWrapperPtr->refCount,  paramWrapperPtr->canFree);*/

  if (paramWrapperPtr->refCount == 0) {
    /* fprintf(stderr, "XOTclParametercheckCmd paramPtr %p manual free\n",paramPtr);*/
    ParamsFree(paramWrapperPtr->paramPtr);
    FREE(XOTclParamWrapper, paramWrapperPtr);
  } else {
    paramWrapperPtr->canFree = 1;
  }

  if (flags & XOTCL_PC_MUST_DECR) {
    DECR_REF_COUNT(outObjPtr);
  }

  return result;
}

/*
xotclCmd parametercheck XOTclParametercheckCmd {
  {-argName "param" -type tclobj}
  {-argName "-nocomplain"}
  {-argName "value" -required 0 -type tclobj}
  } 
*/
static int XOTclParametercheckCmd(Tcl_Interp *interp, int withNocomplain, Tcl_Obj *objPtr, Tcl_Obj *valueObj) {
  XOTclParam *paramPtr = NULL;
  int result;

  result = Parametercheck(interp, objPtr, valueObj, "value:", &paramPtr);

  /*fprintf(stderr, "after convert\n");*/

  if (paramPtr && paramPtr->converter == convertViaCmd && 
      (withNocomplain || result == TCL_OK)) {
    Tcl_ResetResult(interp);
  } 

  if (withNocomplain) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), (result == TCL_OK));
    result = TCL_OK;
  } else if (result == TCL_OK) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
  }

  return result;
}

/***************************
 * End generated XOTcl commands
 ***************************/

/***************************
 * Begin Object Methods
 ***************************/
static int XOTclOAutonameMethod(Tcl_Interp *interp, XOTclObject *object, int withInstance, int withReset,
                                Tcl_Obj *nameObj) {
  Tcl_Obj *autoname = AutonameIncr(interp, nameObj, object, withInstance, withReset);
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

static int XOTclOCleanupMethod(Tcl_Interp *interp, XOTclObject *object) {
  XOTclClass  *cl  = XOTclObjectToClass(object);
  int softrecreate;
  Tcl_Obj *savedNameObj;

#if defined(OBJDELETION_TRACE)
  fprintf(stderr, "+++ XOTclOCleanupMethod\n");
#endif
  PRINTOBJ("XOTclOCleanupMethod", object);

  savedNameObj = object->cmdName;
  INCR_REF_COUNT(savedNameObj);

  /* save and pass around softrecreate*/
  softrecreate = object->flags & XOTCL_RECREATE && RUNTIME_STATE(interp)->doSoftrecreate;

  CleanupDestroyObject(interp, object, softrecreate);
  CleanupInitObject(interp, object, object->cl, object->nsPtr, softrecreate);

  if (cl) {
    CleanupDestroyClass(interp, cl, softrecreate, 1);
    CleanupInitClass(interp, cl, cl->nsPtr, softrecreate, 1);
  }

  DECR_REF_COUNT(savedNameObj);
  return TCL_OK;
}

static int
GetObjectParameterDefinition(Tcl_Interp *interp, CONST char *methodName, XOTclObject *object,
                             XOTclParsedParam *parsedParamPtr) {
  int result;
  Tcl_Obj *rawConfArgs;

  /*
   * Parameter definitions are cached in the class, for which
   * instances are created. The parameter definitions are flushed in
   * the following situations:
   *
   * a) on class cleanup: ParsedParamFree(cl->parsedParamPtr)
   * b) on class structure changes,
   * c) when classmixins are added,
   * d) when new slots are defined,
   * e) when slots are removed
   *
   * When slot defaults or types are changed, the slots have to
   * perform a manual "::nsf::invalidateobjectparameter $domain"
   */

  /*
   * Check, if there is already a parameter definition available for
   * creating objects of this class.
   */
  if (object->cl->parsedParamPtr) {
    parsedParamPtr->paramDefs = object->cl->parsedParamPtr->paramDefs;
    parsedParamPtr->possibleUnknowns = object->cl->parsedParamPtr->possibleUnknowns;
    result = TCL_OK;
  } else {
    /*
     * There is no parameter definition available, get a new one in
     * the the string representation.
     */
    /*fprintf(stderr, "calling %s objectparameter\n", objectName(object));*/
    Tcl_Obj *methodObj = XOTclMethodObj(interp, object, XO_o_objectparameter_idx);

    if (methodObj) {
      result = callMethod((ClientData) object, interp, methodObj, 
			  2, 0, XOTCL_CM_NO_PROTECT);
      
      if (result == TCL_OK) {
	rawConfArgs = Tcl_GetObjResult(interp);
	/*fprintf(stderr, ".... rawConfArgs for %s => %s\n", objectName(object), ObjStr(rawConfArgs));*/
	INCR_REF_COUNT(rawConfArgs);
	
	/* Parse the string representation to obtain the internal representation */
	result = ParamDefsParse(interp, methodName, rawConfArgs, XOTCL_DISALLOWED_ARG_OBJECT_PARAMETER, parsedParamPtr);
	if (result == TCL_OK) {
	  XOTclParsedParam *ppDefPtr = NEW(XOTclParsedParam);
	  ppDefPtr->paramDefs = parsedParamPtr->paramDefs;
	  ppDefPtr->possibleUnknowns = parsedParamPtr->possibleUnknowns;
	  object->cl->parsedParamPtr = ppDefPtr;
	}
	DECR_REF_COUNT(rawConfArgs);
      }
    } else {
      parsedParamPtr->paramDefs = NULL;
      parsedParamPtr->possibleUnknowns = 0;
      result = TCL_OK;
    }
  }
  return result;
}

static int
XOTclOConfigureMethod(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]) {
  int result, i, remainingArgsc;
  XOTclParsedParam parsedParam;
  XOTclParam *paramPtr;
  XOTclParamDefs *paramDefs;
  Tcl_Obj *newValue;
  parseContext pc;
  Tcl_CallFrame frame, *framePtr = &frame;

#if 0
  fprintf(stderr, "XOTclOConfigureMethod %s %d ",objectName(object), objc);

  for(i=0; i<objc; i++) {
    /*fprintf(stderr, "  ov[%d]=%p, objc=%d\n", j, ov[j], objc);*/
    fprintf(stderr, " o[%d]=%s,", i, ObjStr(objv[i]));
  }
  fprintf(stderr, "\n");
#endif

  /* Get the object parameter definition */
  result = GetObjectParameterDefinition(interp, ObjStr(objv[0]), object, &parsedParam);
  if (result != TCL_OK || !parsedParam.paramDefs) {
    /*fprintf(stderr, "... nothing to do for method %s\n", ObjStr(objv[0]));*/
    goto configure_exit;
  }

  /* Push frame to allow for [self] and make instvars of obj accessible as locals */
  XOTcl_PushFrameObj(interp, object, framePtr);

  /* Process the actual arguments based on the parameter definitions */
  paramDefs = parsedParam.paramDefs;
  result = ProcessMethodArguments(&pc, interp, object, 0, paramDefs, "configure", objc, objv);

  if (result != TCL_OK) {
    XOTcl_PopFrameObj(interp, framePtr);
    parseContextRelease(&pc);
    goto configure_exit;
  }

  /*
   * At this point, the arguments are valid (according to the
   * parameter definitions) and the defaults are set. Now we have to
   * apply the arguments (mostly setting instance variables).
   */
#if defined(CONFIGURE_ARGS_TRACE)
  fprintf(stderr, "*** POPULATE OBJ '%s': nr of parsed args %d\n", objectName(object), pc.objc);
#endif

  for (i=1, paramPtr = paramDefs->paramsPtr; paramPtr->name; paramPtr++, i++) {

    newValue = pc.full_objv[i];
    /*fprintf(stderr, "new Value of %s = %p '%s', type %s", 
            ObjStr(paramPtr->nameObj),
            newValue, newValue ? ObjStr(newValue) : "(null)", paramPtr->type); */

    if (newValue == XOTclGlobalObjs[XOTE___UNKNOWN__]) {
      /* nothing to do here */
      continue;
    }

    /* special setter due to relation handling */
    if (paramPtr->converter == convertToRelation) {
      ClientData relIdx; 
      Tcl_Obj *relationObj = paramPtr->converterArg ? paramPtr->converterArg : paramPtr->nameObj,
	*outObjPtr;

      result = convertToRelationtype(interp, relationObj, paramPtr, &relIdx, &outObjPtr);

      if (result == TCL_OK) {
        result = XOTclRelationCmd(interp, object, PTR2INT(relIdx), newValue);
      }

      if (result != TCL_OK) {
        XOTcl_PopFrameObj(interp, framePtr);
        parseContextRelease(&pc);
        goto configure_exit;
      }
      /* done with relation handling */
      continue;
    }

    /* special setter for init commands */
    if (paramPtr->flags & (XOTCL_ARG_INITCMD|XOTCL_ARG_METHOD)) {
      CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
      XOTclCallStackContent csc, *cscPtr = &csc;
      Tcl_CallFrame frame2, *framePtr2 = &frame2;

      /* The current callframe of configure uses an objscope, such
         that setvar etc.  are able to access variables like "a" as a
         local variable.  However, in the init block, we do not like
         that behavior, since this should look like like a proc body.
         So we push yet another callframe without providing the
         varframe. 

         The new frame will have the namespace of the caller to avoid
         the current objscope. XOTcl_PushFrameCsc() will establish
         a CMETHOD frame. 
      */

      Tcl_Interp_varFramePtr(interp) = varFramePtr->callerPtr;
      CscInit(cscPtr, object, NULL /*cl*/, NULL/*cmd*/, XOTCL_CSC_TYPE_PLAIN);
      XOTcl_PushFrameCsc(interp, cscPtr, framePtr2);

      if (paramPtr->flags & XOTCL_ARG_INITCMD) {
        result = Tcl_EvalObjEx(interp, newValue, TCL_EVAL_DIRECT);

      } else /* must be XOTCL_ARG_METHOD */ {
        Tcl_Obj *ov[3];
        int oc = 0;
        if (paramPtr->converterArg) {
          /* if arg= was given, pass it as first argument */
          ov[0] = paramPtr->converterArg;
          oc = 1;
        }
        if (paramPtr->nrArgs == 1) {
          ov[oc] = newValue;
          oc ++;
        }
        result = XOTclCallMethodWithArgs((ClientData) object, interp, paramPtr->nameObj, 
                                         ov[0], oc, &ov[1], 0);
      }
      /* 
         Pop previously stacked frame for eval context and set the
         varFramePtr to the previous value.
      */
      XOTcl_PopFrameCsc(interp, framePtr2); 
      CscFinish(interp, cscPtr);
      Tcl_Interp_varFramePtr(interp) = varFramePtr;

      /*fprintf(stderr, "XOTclOConfigureMethod_ attribute %s evaluated %s => (%d)\n",
        ObjStr(paramPtr->nameObj), ObjStr(newValue), result);*/

      if (result != TCL_OK) {
        XOTcl_PopFrameObj(interp, framePtr);
        parseContextRelease(&pc);
        goto configure_exit;
      }

      if (paramPtr->flags & XOTCL_ARG_INITCMD && RUNTIME_STATE(interp)->doKeepinitcmd) {
	Tcl_ObjSetVar2(interp, paramPtr->nameObj, NULL, newValue, TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
      }

      /* done with init command handling */
      continue;
    }

    /* set the variables unless the last argument of the definition is varArgs */
    if (i < paramDefs->nrParams || !pc.varArgs) {
      /* standard setter */
#if defined(CONFIGURE_ARGS_TRACE)
      fprintf(stderr, "*** %s SET %s '%s'\n", objectName(object), ObjStr(paramPtr->nameObj), ObjStr(newValue));
#endif
      Tcl_ObjSetVar2(interp, paramPtr->nameObj, NULL, newValue, TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
    }
  }

  XOTcl_PopFrameObj(interp, framePtr);

  remainingArgsc = pc.objc - paramDefs->nrParams;

  /* 
     Call residualargs when we have varargs and left over arguments 
  */
  if (pc.varArgs && remainingArgsc > 0) {
    Tcl_Obj *methodObj;

    if (CallDirectly(interp, object, XO_o_residualargs_idx, &methodObj)) {
      i -= 2;
      if (methodObj) {pc.full_objv[i] = methodObj;}
      result = XOTclOResidualargsMethod(interp, object, remainingArgsc+1, pc.full_objv + i);
    } else {
      result = callMethod((ClientData) object, interp,
                          methodObj, remainingArgsc+2, pc.full_objv + i-1, 0);
    }
    if (result != TCL_OK) {
      parseContextRelease(&pc);
      goto configure_exit;
    }
  } else {
    Tcl_SetObjResult(interp, XOTclGlobalObjs[XOTE_EMPTY]);
  }

  parseContextRelease(&pc);

 configure_exit:
  return result;
}

static int XOTclODestroyMethod(Tcl_Interp *interp, XOTclObject *object) {
  PRINTOBJ("XOTclODestroyMethod", object);

  /*fprintf(stderr,"XOTclODestroyMethod %p %s flags %.6x activation %d cmd %p cmd->flags %.6x\n",
          object, ((Command*)object->id)->flags == 0 ? objectName(object) : "(deleted)", 
          object->flags, object->activationCount, object->id, ((Command*)object->id)->flags);*/

  /*
   * XOTCL_DESTROY_CALLED might be set already be callDestroyMethod(),
   * the implicit destroy calls. It is necessary to set it here for
   * the explicit destroy calls in the script, which reach the
   * Object->destroy.
   */

  if ((object->flags & XOTCL_DESTROY_CALLED) == 0) {
    object->flags |= XOTCL_DESTROY_CALLED;
  }

  if ((object->flags & XOTCL_DURING_DELETE) == 0) {
    int result;
    Tcl_Obj *methodObj;

    /*fprintf(stderr, "   call dealloc on %p %s\n", object, 
      ((Command*)object->id)->flags == 0 ? objectName(object) : "(deleted)");*/

    if (CallDirectly(interp, &object->cl->object, XO_c_dealloc_idx, &methodObj)) {
      result = DoDealloc(interp, object);
    } else {
      /*fprintf(stderr, "call dealloc\n");*/
      result = XOTclCallMethodWithArgs((ClientData)object->cl, interp, methodObj, 
                                       object->cmdName, 1, NULL, 0);
      if (result != TCL_OK) {
        /* 
	 * In case, the call of the dealloc method has failed above (e.g. NS_DYING), 
         * we have to call dealloc manually, otherwise we have a memory leak 
         */
        /*object->flags |= XOTCL_CMD_NOT_FOUND;*/
        /*fprintf(stderr, "*** dealloc failed for %p %s flags %.6x, retry\n", 
	  object, objectName(object), object->flags);*/
        result = DoDealloc(interp, object);
      }
    }
    return result;
  } else {
#if defined(OBJDELETION_TRACE)
    fprintf(stderr, "  Object->destroy already during delete, don't call dealloc %p\n", object);
#endif
  }
  return TCL_OK;
}

static int XOTclOExistsMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *var) {
  Tcl_SetIntObj(Tcl_GetObjResult(interp), varExists(interp, object, var, NULL, 1, 1));
  return TCL_OK;
}

static int XOTclOFilterGuardMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *filter, Tcl_Obj *guardObj) {
  XOTclObjectOpt *opt = object->opt;

  if (opt && opt->filters) {
    XOTclCmdList *h = CmdListFindNameInList(interp, filter, opt->filters);
    if (h) {
      if (h->clientData)
        GuardDel((XOTclCmdList*) h);
      GuardAdd(interp, h, guardObj);
      object->flags &= ~XOTCL_FILTER_ORDER_VALID;
      return TCL_OK;
    }
  }

  return XOTclVarErrMsg(interp, "Filterguard: can't find filter ",
                        filter, " on ", objectName(object), (char *) NULL);
}

/*
 *  Searches for filter on [self] and returns fully qualified name
 *  if it is not found it returns an empty string
 */
static int FilterSearchMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *filter) {
  CONST char *filterName;
  XOTclCmdList *cmdList;
  XOTclClass *fcl;

  Tcl_ResetResult(interp);

  if (!(object->flags & XOTCL_FILTER_ORDER_VALID))
    FilterComputeDefined(interp, object);
  if (!(object->flags & XOTCL_FILTER_ORDER_DEFINED))
    return TCL_OK;

  for (cmdList = object->filterOrder; cmdList;  cmdList = cmdList->nextPtr) {
    filterName = Tcl_GetCommandName(interp, cmdList->cmdPtr);
    if (filterName[0] == filter[0] && !strcmp(filterName, filter))
      break;
  }

  if (!cmdList)
    return TCL_OK;

  fcl = cmdList->clorobj;
  return ListMethodHandle(interp, (XOTclObject*)fcl, !XOTclObjectIsClass(&fcl->object), filterName);
}

static int XOTclOInstVarMethod(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]) {
  callFrameContext ctx = {0};
  int result;

  if (object && (object->filterStack || object->mixinStack) ) {
    CallStackUseActiveFrames(interp, &ctx);
  }
  if (!Tcl_Interp_varFramePtr(interp)) {
    CallStackRestoreSavedFrames(interp, &ctx);
    return XOTclVarErrMsg(interp, "instvar used on ", objectName(object),
                          ", but callstack is not in procedure scope",
			  (char *) NULL);
  }

  result = XOTclImportvar(interp, object, ObjStr(objv[0]), objc-1, objv+1);
  CallStackRestoreSavedFrames(interp, &ctx);
  return result;
}

static int XOTclOMixinGuardMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *mixin, Tcl_Obj *guardObj) {
  XOTclObjectOpt *opt = object->opt;

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
        GuardAdd(interp, h, guardObj);
        object->flags &= ~XOTCL_MIXIN_ORDER_VALID;
        return TCL_OK;
      }
    }
  }

  return XOTclVarErrMsg(interp, "Mixinguard: can't find mixin ",
                        mixin, " on ", objectName(object), (char *) NULL);
}

#if 0
/* method for calling e.g.  $obj __next  */
static int XOTclONextMethod(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]) {
  XOTclCallStackContent *cscPtr = CallStackGetObjectFrame(interp, object);
  CONST char *methodName;

  if (!cscPtr)
    return XOTclVarErrMsg(interp, "__next: can't find object",
			  objectName(object), (char *) NULL);
  methodName = (char *)Tcl_GetCommandName(interp, cscPtr->cmdPtr);
  /* fprintf(stderr, "methodName %s\n", methodName);*/
  return XOTclNextMethod(object, interp, cscPtr->cl, methodName, objc-1, &objv[1], 0, NULL);
}
#endif

static int XOTclONoinitMethod(Tcl_Interp *interp, XOTclObject *object) {
  object->flags |= XOTCL_INIT_CALLED;
  return TCL_OK;
}


static int XOTclORequireNamespaceMethod(Tcl_Interp *interp, XOTclObject *object) {
  requireObjNamespace(interp, object);
  return TCL_OK;
}

static int XOTclOResidualargsMethod(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj **argv, **nextArgv, *resultObj;
  int i, start = 1, argc, nextArgc, normalArgs, result = TCL_OK, isdasharg = NO_DASH;
  CONST char *methodName, *nextMethodName;
  
  /* find arguments without leading dash */
  for (i=start; i < objc; i++) {
    if ((isdasharg = isDashArg(interp, objv[i], 1, &methodName, &argc, &argv))) {
      break;
    }
  }
  normalArgs = i-1;

  for( ; i < objc;  argc=nextArgc, argv=nextArgv, methodName=nextMethodName) {
    Tcl_ResetResult(interp);
    switch (isdasharg) {
    case SKALAR_DASH:    /* Argument is a skalar with a leading dash */
      { int j;
        for (j = i+1; j < objc; j++, argc++) {
          if ((isdasharg = isDashArg(interp, objv[j], j==i+1, &nextMethodName, &nextArgc, &nextArgv))) {
            break;
          }
        }
        result = callConfigureMethod(interp, object, methodName, argc+1, objv+i+1);
        if (result != TCL_OK) {
          return result;
	}
	i += argc;
	break;
      }
    case LIST_DASH:  /* Argument is a list with a leading dash, grouping determined by list */
      {	i++;
	if (i<objc) {
	  isdasharg = isDashArg(interp, objv[i], 1, &nextMethodName, &nextArgc, &nextArgv);
        }
	result = callConfigureMethod(interp, object, methodName, argc+1, argv+1);
	if (result != TCL_OK) {
	  return result;
        }
	break;
      }
    default:
      {
	return XOTclVarErrMsg(interp, objectName(object),
			      " configure: unexpected argument '",
			      ObjStr(objv[i]),
			      "' between parameters", (char *) NULL);
      }
    }
  }
  resultObj = Tcl_NewListObj(normalArgs, objv+1);
  Tcl_SetObjResult(interp, resultObj);

  return result;
}

static int XOTclOUplevelMethod(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]) {
  int i, result = TCL_ERROR;
  CONST char *frameInfo = NULL;
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
    if (!framePtr) {
      framePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp)->callerVarPtr;
      if (!framePtr) {
        framePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
      }
    }
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
    sprintf(msg, "\n    (\"uplevel\" body line %d)", Tcl_GetErrorLine(interp));
    Tcl_AddObjErrorInfo(interp, msg, -1);
  }

  /*
   * Restore the variable frame, and return.
   */

  Tcl_Interp_varFramePtr(interp) = (CallFrame *)savedVarFramePtr;
  return result;
}

static int XOTclOUpvarMethod(Tcl_Interp *interp, XOTclObject *object, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj *frameInfoObj = NULL;
  int i, result = TCL_ERROR;
  CONST char *frameInfo;
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

  if (object && (object->filterStack || object->mixinStack)) {
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

static int XOTclOVolatileMethod(Tcl_Interp *interp, XOTclObject *object) {
  Tcl_Obj *objPtr = object->cmdName;
  int result = TCL_ERROR;
  CONST char *fullName = ObjStr(objPtr);
  CONST char *vn;
  callFrameContext ctx = {0};

  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound != XOTCL_EXITHANDLER_OFF) {
    fprintf(stderr, "### Can't make objects volatile during shutdown\n");
    return XOTclVarErrMsg(interp, "Can't make objects volatile during shutdown\n", NULL);
  }

  CallStackUseActiveFrames(interp, &ctx);
  vn = NSTail(fullName);

  if (Tcl_SetVar2(interp, vn, NULL, fullName, 0)) {
    XOTclObjectOpt *opt = XOTclRequireObjectOpt(object);

    /*fprintf(stderr, "### setting trace for %s on frame %p\n", fullName, 
      Tcl_Interp_varFramePtr(interp));
      tcl85showStack(interp);*/
    result = Tcl_TraceVar(interp, vn, TCL_TRACE_UNSETS,
			  (Tcl_VarTraceProc*)XOTclUnsetTrace,
                          (ClientData)objPtr);
    opt->volatileVarName = vn;
  }
  CallStackRestoreSavedFrames(interp, &ctx);

  if (result == TCL_OK) {
    INCR_REF_COUNT(objPtr);
  }
  return result;
}

static int XOTclOVwaitMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *varname) {
  int done, foundEvent;
  int flgs = TCL_TRACE_WRITES|TCL_TRACE_UNSETS;
  Tcl_CallFrame frame, *framePtr = &frame;

  /*
   * Make sure the var table exists and the varname is in there
   */
  if (NSRequireVariableOnObj(interp, object, varname, flgs) == 0)
    return XOTclVarErrMsg(interp, "Can't lookup (and create) variable ",
                          varname, " on ", objectName(object), (char *) NULL);

  XOTcl_PushFrameObj(interp, object, framePtr);
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
  XOTcl_PopFrameObj(interp, framePtr);
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

/* todo temporary, remove me yyy */
static int XOTclOVarsMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *pattern) {
  return XOTclObjInfoVarsMethod(interp, object, pattern);
}

/***************************
 * End Object Methods
 ***************************/


/***************************
 * Begin Class Methods
 ***************************/

static int XOTclCAllocMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *nameObj) {
  Tcl_Obj *tmpName = NULL;
  CONST char *nameString = ObjStr(nameObj);
  int result;

  /*
   * create a new object from scratch
   */

  /*fprintf(stderr, " **** 0 class '%s' wants to alloc '%s'\n", className(cl), nameString);*/
  if (!NSCheckColons(nameString, 0)) {
    return XOTclVarErrMsg(interp, "Cannot allocate object -- illegal name '",
                          nameString, "'", (char *) NULL);
  }

  /*
   * If the path is not absolute, we add the appropriate namespace
   */
  if (!isAbsolutePath(nameString)) {
    nameObj = tmpName = NameInNamespaceObj(interp, nameString, callingNameSpace(interp));
    INCR_REF_COUNT(tmpName);
    /*fprintf(stderr, " **** NoAbsoluteName for '%s' -> determined = '%s'\n",
      name, ObjStr(tmpName));*/
    nameString = ObjStr(tmpName);
  }

  if (IsMetaClass(interp, cl, 1)) {
    /*
     * if the base class is a meta-class, we create a class
     */
    XOTclClass *newcl = PrimitiveCCreate(interp, nameObj, cl);
    if (newcl == 0) {
      result = XOTclVarErrMsg(interp, "Class alloc failed for '", nameString,
                              "' (possibly parent namespace does not exist)",
                              (char *) NULL);
    } else {
      Tcl_SetObjResult(interp, nameObj);
      result = TCL_OK;
    }
  } else {
    /*
     * if the base class is an ordinary class, we create an object
     */
    XOTclObject *newObj = PrimitiveOCreate(interp, nameObj, cl);
    if (newObj == 0)
      result = XOTclVarErrMsg(interp, "Object alloc failed for '", nameString,
                              "' (possibly parent namespace does not exist)",
                              (char *) NULL);
    else {
      Tcl_SetObjResult(interp, nameObj);
      result = TCL_OK;
    }
  }

  if (tmpName) {
    DECR_REF_COUNT(tmpName);
  }

  return result;
}

static int
XOTclCCreateMethod(Tcl_Interp *interp, XOTclClass *cl, CONST char *specifiedName, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *newObject = NULL;
  Tcl_Obj *nameObj, *methodObj, *tmpObj = NULL;
  Tcl_Obj **nobjv;
  int result;
  CONST char *nameString = specifiedName;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound != XOTCL_EXITHANDLER_OFF) {
    fprintf(stderr, "### Can't create object %s during shutdown\n", ObjStr(objv[1]));
    return TCL_OK; /* don't fail, if this happens during destroy, it might be canceled */
  }

  /*
   * complete the name if it is not absolute
   */
  if (!isAbsolutePath(nameString)) {
    tmpObj = NameInNamespaceObj(interp, nameString, callingNameSpace(interp));
    nameString = ObjStr(tmpObj);
    /*fprintf(stderr, " **** fixed name is '%s'\n", nameString);*/
    INCR_REF_COUNT(tmpObj);
    memcpy(tov, objv, sizeof(Tcl_Obj *)*(objc));
    tov[1] = tmpObj;
    nameObj = tmpObj;
    nobjv = tov;
  } else {
    nameObj = objv[1];
    nobjv = (Tcl_Obj **)objv;
  }

  /*
   * Check whether we have to call recreate (i.e. when the
   * object exists already)
   */
  newObject = XOTclpGetObject(interp, nameString);

  /*fprintf(stderr, "+++ createspecifiedName '%s', nameString '%s', newObject=%p ismeta(%s) %d, ismeta(%s) %d\n",
          specifiedName, nameString, newObject,
          className(cl), IsMetaClass(interp, cl, 1),
          newObject ? ObjStr(newObject->cl->object.cmdName) : "NULL",
          newObject ? IsMetaClass(interp, newObject->cl, 1) : 0
          );*/

  /* don't allow to
     - recreate an object as a class, 
     - recreate a class as an object, and to
     - recreate an object in a different obejct system

     In these clases, we use destroy + create instead of recrate.
  */

  if (newObject 
      && (IsMetaClass(interp, cl, 1) == IsMetaClass(interp, newObject->cl, 1))
      && GetObjectSystem(newObject) == cl->osPtr) {

    /*fprintf(stderr, "%%%% recreate, call recreate method ... %s, objc=%d oldOs %p != newOs %p EQ %d\n",
            ObjStr(nameObj), objc+1,
            GetObjectSystem(newObject), cl->osPtr,
            GetObjectSystem(newObject) != cl->osPtr
            );
    */

    /* call recreate --> initialization */
    if (CallDirectly(interp, &cl->object, XO_c_recreate_idx, &methodObj)) {
      result = RecreateObject(interp, cl, newObject, objc, nobjv);
    } else {
      result = callMethod((ClientData) cl, interp, methodObj, 
                          objc+1, nobjv+1, XOTCL_CM_NO_PROTECT);
    }

    if (result != TCL_OK)
      goto create_method_exit;

    Tcl_SetObjResult(interp, newObject->cmdName);
    nameObj = newObject->cmdName;
    objTrace("RECREATE", newObject);

  } else {
    /*
     * newObject might exist here, but will be automatically destroyed by
     * alloc
     */

    /*fprintf(stderr, "alloc ... %s\n", ObjStr(nameObj));*/
    if (CallDirectly(interp, &cl->object, XO_c_alloc_idx, &methodObj)) {
      result = XOTclCAllocMethod(interp, cl, nameObj);
    } else {
      result = callMethod((ClientData) cl, interp, methodObj, 
                          3, &nameObj, 0);
    }
    if (result != TCL_OK)
      goto create_method_exit;

    nameObj = Tcl_GetObjResult(interp);

    if (GetObjectFromObj(interp, nameObj, &newObject) != TCL_OK) {
      result = XOTclErrMsg(interp, "couldn't find result of alloc", TCL_STATIC);
      goto create_method_exit;
    }

    /*(void)RemoveInstance(newObject, newObject->cl);*/ /* TODO needed? remove? */
    AddInstance(newObject, cl);

    objTrace("CREATE", newObject);

    /* in case, the object is destroyed during initialization, we incr refcount */
    INCR_REF_COUNT(nameObj);
    result = doObjInitialization(interp, newObject, objc, objv);
    DECR_REF_COUNT(nameObj);
  }
 create_method_exit:

  /*fprintf(stderr, "create -- end ... %s => %d\n", ObjStr(nameObj), result);*/
  if (tmpObj)  {DECR_REF_COUNT(tmpObj);}
  FREE_ON_STACK(Tcl_Obj *, tov);
  return result;
}

static int DoDealloc(Tcl_Interp *interp, XOTclObject *object) {
  int result;

  /*fprintf(stderr, "DoDealloc obj= %s %p flags %.6x activation %d cmd %p opt=%p\n",
          objectName(object), object, object->flags, object->activationCount, 
          object->id, object->opt);*/

  result = freeUnsetTraceVariable(interp, object);
  if (result != TCL_OK) {
    return result;
  }

  /*
   * latch, and call delete command if not already in progress
   */
  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound !=
      XOTCL_EXITHANDLER_ON_SOFT_DESTROY) {
    CallStackDestroyObject(interp, object);
  }

  return TCL_OK;
}


static int XOTclCDeallocMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *obj) {
  XOTclObject *object;

  /* fprintf(stderr, "XOTclCDeallocMethod obj %p %s\n",obj, ObjStr(obj));*/
  
  if (GetObjectFromObj(interp, obj, &object) != TCL_OK) {
    fprintf(stderr, "XOTcl object %s does not exist\n", ObjStr(obj));
    return XOTclVarErrMsg(interp, "Can't destroy object ",
                          ObjStr(obj), " that does not exist.", (char *) NULL);
  }
  
  return DoDealloc(interp, object);
}

static int XOTclCNewMethod(Tcl_Interp *interp, XOTclClass *cl, XOTclObject *withChildof,
                           int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj *fullnameObj;
  int result, prefixLength;
  Tcl_DString dFullname, *dsPtr = &dFullname;
  XOTclStringIncrStruct *iss = &RUNTIME_STATE(interp)->iss;

  Tcl_DStringInit(dsPtr);
  if (withChildof) {
    Tcl_DStringAppend(dsPtr, objectName(withChildof), -1);
    Tcl_DStringAppend(dsPtr, "::__#", 5);
  } else {
    Tcl_DStringAppend(dsPtr, "::nsf::__#", 10);
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

  fullnameObj = Tcl_NewStringObj(Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr));

  INCR_REF_COUNT(fullnameObj);

  {
    Tcl_Obj *methodObj;
    int callDirectly;
    ALLOC_ON_STACK(Tcl_Obj*, objc+3, ov);

    callDirectly = CallDirectly(interp, &cl->object, XO_c_create_idx, &methodObj);

    ov[0] = objv[0];
    ov[1] = methodObj;
    ov[2] = fullnameObj;
    if (objc >= 1)
      memcpy(ov+3, objv, sizeof(Tcl_Obj *)*objc);

    if (callDirectly) {
      result = XOTclCCreateMethod(interp, cl, ObjStr(fullnameObj), objc+2, ov+1);
    } else {
      result = ObjectDispatch((ClientData)cl, interp, objc+3, ov, 0);
    }

    FREE_ON_STACK(Tcl_Obj *, ov);
  }

  DECR_REF_COUNT(fullnameObj);
  Tcl_DStringFree(dsPtr);

  return result;
}

static int XOTclCFilterGuardMethod(Tcl_Interp *interp, XOTclClass *cl, 
                                   CONST char *filter, Tcl_Obj *guardObj) {
  XOTclClassOpt *opt = cl->opt;
  
  if (opt && opt->classfilters) {
    XOTclCmdList *h = CmdListFindNameInList(interp, filter, opt->classfilters);
    if (h) {
      if (h->clientData)
	GuardDel(h);
      GuardAdd(interp, h, guardObj);
      FilterInvalidateObjOrders(interp, cl);
      return TCL_OK;
    }
  }

  return XOTclVarErrMsg(interp, "filterguard: can't find filter ",
			filter, " on ", className(cl),	(char *) NULL);
}

static int XOTclCMixinGuardMethod(Tcl_Interp *interp, XOTclClass *cl, CONST char *mixin, Tcl_Obj *guardObj) {
  XOTclClassOpt *opt = cl->opt;

  if (opt && opt->classmixins) {
    XOTclClass *mixinCl = XOTclpGetClass(interp, mixin);
    Tcl_Command mixinCmd = NULL;
    if (mixinCl) {
      mixinCmd = Tcl_GetCommandFromObj(interp, mixinCl->object.cmdName);
    }
    if (mixinCmd) {
      XOTclCmdList *h = CmdListFindCmdInList(mixinCmd, opt->classmixins);
      if (h) {
        if (h->clientData)
          GuardDel((XOTclCmdList*) h);
        GuardAdd(interp, h, guardObj);
        MixinInvalidateObjOrders(interp, cl);
        return TCL_OK;
      }
    }
  }

  return XOTclVarErrMsg(interp, "mixinguard: can't find mixin ",
                        mixin, " on ", className(cl), (char *) NULL);
}

static int RecreateObject(Tcl_Interp *interp, XOTclClass *class, XOTclObject *object,
                          int objc, Tcl_Obj *CONST objv[]) {
  int result;

  object->flags |= XOTCL_RECREATE;

  /*
   * First, cleanup the data from the object. 
   * 
   * Check whether we have a pending destroy on the object; if yes,
   * clear it, such that the recreated object and won't be destroyed
   * on a POP
   */
  MarkUndestroyed(object);

  /*
   *  ensure correct class for object
   */
  result = changeClass(interp, object, class);

  if (result == TCL_OK) {
    Tcl_Obj *methodObj;
    /*
     * dispatch "cleanup" method
     */
    if (CallDirectly(interp, object, XO_o_cleanup_idx, &methodObj)) {
      result = XOTclOCleanupMethod(interp, object);
    } else {
      result = callMethod((ClientData) object, interp, methodObj, 
                          2, 0, XOTCL_CM_NO_PROTECT);
    }
  }

  /*
   * Second: if cleanup was successful, initialize the object as usual
   */
  if (result == TCL_OK) {
    result = doObjInitialization(interp, object, objc, objv);
    if (result == TCL_OK) {
      Tcl_SetObjResult(interp, object->cmdName);
    }
  }
  return result;
}

static int XOTclCRecreateMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *nameObj,
                                int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *object;

  if (GetObjectFromObj(interp, nameObj, &object) != TCL_OK)
    return XOTclVarErrMsg(interp, "can't recreate non existing object ",
                          ObjStr(nameObj), (char *) NULL);

  return RecreateObject(interp, cl, object, objc, objv);
}

/***************************
 * End Class Methods
 ***************************/

#if 0
/***************************
 * Begin check Methods
 ***************************/
static int XOTclCheckBooleanArgs(Tcl_Interp *interp, CONST char *name, Tcl_Obj *valueObj) {
  int result, bool;
  Tcl_Obj *boolean;

  if (value == NULL) {
    /* the variable is not yet defined (set), so we cannot check
       whether it is boolean or not */
    return TCL_OK;
  }

  boolean = Tcl_DuplicateObj(valueObj);
  INCR_REF_COUNT(boolean);
  result = Tcl_GetBooleanFromObj(interp, boolean, &bool);
  DECR_REF_COUNT(boolean);

  Tcl_ResetResult(interp);
  Tcl_SetIntObj(Tcl_GetObjResult(interp), (result == TCL_OK));
  return TCL_OK;
}

static int XOTclCheckRequiredArgs(Tcl_Interp *interp, CONST char *name, Tcl_Obj *valueObj) {
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
#endif

static int AggregatedMethodType(int methodType) {
  switch (methodType) {
  case MethodtypeNULL: /* default */
    methodType = XOTCL_METHODTYPE_SCRIPTED|XOTCL_METHODTYPE_BUILTIN;
    break;
  case MethodtypeAllIdx: 
    methodType = XOTCL_METHODTYPE_SCRIPTED|XOTCL_METHODTYPE_BUILTIN|XOTCL_METHODTYPE_OBJECT;
    break;
  case MethodtypeScriptedIdx:
    /*methodType = XOTCL_METHODTYPE_SCRIPTED|XOTCL_METHODTYPE_ALIAS;*/
    methodType = XOTCL_METHODTYPE_SCRIPTED;
    break;
  case MethodtypeBuiltinIdx:
    methodType = XOTCL_METHODTYPE_BUILTIN;
    break;
  case MethodtypeForwarderIdx:
    methodType = XOTCL_METHODTYPE_FORWARDER;
    break;
  case MethodtypeAliasIdx:
    methodType = XOTCL_METHODTYPE_ALIAS;
    break;
  case MethodtypeSetterIdx:
    methodType = XOTCL_METHODTYPE_SETTER;
    break;
  case MethodtypeObjectIdx:
    methodType = XOTCL_METHODTYPE_OBJECT;
    break;
  default:
    methodType = 0;
  }

  return methodType;
}

/***************************
 * Begin Object Info Methods
 ***************************/
/*
infoObjectMethod callable XOTclObjInfoCallableMethod {
  {-argName "object" -type object}
  {-argName "infocallablesubcmd" -nrargs 1 -type "filter|method|methods" -required 1}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default all}
  {-argName "-application"}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern" -required 0}
*/
static int XOTclObjInfoCallableMethod(Tcl_Interp *interp, XOTclObject *object, 
				      int subcmd,
                                      int withMethodtype, int withCallprotection,
                                      int withApplication,
                                      int withNomixins, int withIncontext, CONST char *pattern) {

  if (subcmd != InfocallablesubcmdMethodsIdx) {
    if (withMethodtype || withCallprotection || withApplication || withNomixins || withIncontext) {
      return XOTclVarErrMsg(interp, "options -methodtype, -callprotection, -application, ",
			    "-nomixins, -incontext are only valued for subcommand 'methods'",
			    (char *) NULL);
    }
    if (pattern == NULL) {
      return XOTclVarErrMsg(interp, "methodname must be provided as last argument",
			    (char *) NULL);
    }
  }
  switch (subcmd) {
  case InfocallablesubcmdMethodIdx: 
    {
      XOTclClass *pcl = NULL;
      Tcl_Command cmd = ObjectFindMethod(interp, object, pattern, &pcl);

      if (cmd) {
	XOTclObject *pobj = pcl ? &pcl->object : object;
	int perObject = (pcl == NULL);
	ListMethod(interp, pobj, pattern, cmd, InfomethodsubcmdHandleIdx, perObject);
      }
      return TCL_OK;
    }
  case InfocallablesubcmdMethodsIdx: 
    {
      return ListCallableMethods(interp, object, pattern,
				 AggregatedMethodType(withMethodtype), withCallprotection,
				 withApplication, withNomixins, withIncontext);
    }
  case InfocallablesubcmdFilterIdx: 
    {
      return FilterSearchMethod(interp, object, pattern);
    }
  default:
    fprintf(stderr, "should never happen, subcmd %d pattern '%s'\n", subcmd, pattern);

    assert(0); /* should never happen */
  }
}

/*
infoObjectMethod children XOTclObjInfoChildrenMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
*/
static int XOTclObjInfoChildrenMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *pattern) {
  return ListChildren(interp, object, pattern, 0);
}

/*
infoObjectMethod class XOTclObjInfoClassMethod {
  {-argName "object" -required 1 -type object}
}
*/
static int XOTclObjInfoClassMethod(Tcl_Interp *interp, XOTclObject *object) {
  Tcl_SetObjResult(interp, object->cl->object.cmdName);
  return TCL_OK;
}

/*
infoObjectMethod filter XOTclObjInfoFilterMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-guard"}
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern"}
}
*/
static int XOTclObjInfoFilterMethod(Tcl_Interp *interp, XOTclObject *object,
                                    int withGuard, int withGuards, int withOrder, 
				    CONST char *pattern) {
  XOTclObjectOpt *opt = object->opt;

  if (withGuard) {
    return opt ? GuardList(interp, object->opt->filters, pattern) : TCL_OK;
  }
  if (withOrder) {
    if (!(object->flags & XOTCL_FILTER_ORDER_VALID))
      FilterComputeDefined(interp, object);
    return FilterInfo(interp, object->filterOrder, pattern, withGuards, 1);
  }
  return opt ? FilterInfo(interp, opt->filters, pattern, withGuards, 0) : TCL_OK;
}

/*
infoObjectMethod forward XOTclObjInfoForwardMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-definition"}
  {-argName "name"}
}
*/
static int XOTclObjInfoForwardMethod(Tcl_Interp *interp, XOTclObject *object, int withDefinition, CONST char *pattern) {
  return object->nsPtr ?
    ListForward(interp, Tcl_Namespace_cmdTable(object->nsPtr), pattern, withDefinition) :
    TCL_OK;
}

/*
infoObjectMethod hasnamespace XOTclObjInfoHasnamespaceMethod {
  {-argName "object" -required 1 -type object}
}
*/
static int XOTclObjInfoHasnamespaceMethod(Tcl_Interp *interp, XOTclObject *object) {
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), object->nsPtr != NULL);
  return TCL_OK;
}

/*
infoObjectMethod method XOTclObjInfoMethodMethod {
  {-argName "object" -type object}
  {-argName "infomethodsubcmd" -type "args|definition|handle|parameter|parametersyntax|type|precondition|postcondition"}
  {-argName "name"}
}
*/
static int XOTclObjInfoMethodMethod(Tcl_Interp *interp, XOTclObject *object, 
                                    int subcmd, CONST char *methodName) {
  Tcl_Namespace *nsPtr = object->nsPtr;
  Tcl_Command cmd;

  if (*methodName == ':') {
    Tcl_Obj *methodObj = Tcl_NewStringObj(methodName, -1);
    cmd = Tcl_GetCommandFromObj(interp, methodObj);
  } else {
    cmd = nsPtr ? FindMethod(nsPtr, methodName) : NULL;
  }
  return ListMethod(interp, object, methodName, cmd, subcmd, 1);
}

/*
infoObjectMethod methods XOTclObjInfoMethodsMethod {
  {-argName "object" -type object}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern"}
  }
*/
static int XOTclObjInfoMethodsMethod(Tcl_Interp *interp, XOTclObject *object, 
				     int withMethodtype, int withCallproctection,
                                     int withNomixins, int withIncontext, CONST char *pattern) {
  return ListDefinedMethods(interp, object, pattern, 1 /* per-object */, 
                            AggregatedMethodType(withMethodtype), withCallproctection,
                            withNomixins, withIncontext);
}

/*
infoObjectMethod mixin XOTclObjInfoMixinMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-guard"}
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern" -type objpattern}
}
*/
static int XOTclObjInfoMixinMethod(Tcl_Interp *interp, XOTclObject *object, 
                                   int withGuard, int withGuards, int withOrder,
                                   CONST char *patternString, XOTclObject *patternObj) {

  if (withGuard) {
    return object->opt ? GuardList(interp, object->opt->mixins, patternString) : TCL_OK;
  }
  if (withOrder) {
    if (!(object->flags & XOTCL_MIXIN_ORDER_VALID))
      MixinComputeDefined(interp, object);
    return MixinInfo(interp, object->mixinOrder, patternString,
		     withGuards, patternObj);
  }
  return object->opt ? MixinInfo(interp, object->opt->mixins, patternString, withGuards, patternObj) : TCL_OK;
}

/*
infoObjectMethod parent XOTclObjInfoParentMethod {
  {-argName "object" -required 1 -type object}
}
*/
static int XOTclObjInfoParentMethod(Tcl_Interp *interp, XOTclObject *object) {
  if (object->id) {
    Tcl_SetResult(interp, NSCmdFullName(object->id), TCL_VOLATILE);
  }
  return TCL_OK;
}

/*
infoObjectMethod precedence XOTclObjInfoPrecedenceMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-intrinsic"}
  {-argName "pattern" -required 0}
}
*/
static int XOTclObjInfoPrecedenceMethod(Tcl_Interp *interp, XOTclObject *object,
                                        int withIntrinsicOnly, CONST char *pattern) {
  XOTclClasses *precedenceList = NULL, *pl;

  precedenceList = ComputePrecedenceList(interp, object, pattern, !withIntrinsicOnly, 1);
  for (pl = precedenceList; pl; pl = pl->nextPtr) {
    Tcl_AppendElement(interp, className(pl->cl));
  }
  XOTclClassListFree(precedenceList);
  return TCL_OK;
}

/*
infoObjectMethod slotobjects XOTclObjInfoSlotObjectsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
*/
static int XOTclObjInfoSlotObjectsMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *pattern) {
  XOTclObjects *pl, *slotObjects;
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);

  slotObjects = computeSlotObjects(interp, object, pattern /* not used */, 1);
  for (pl=slotObjects; pl; pl = pl->nextPtr) {
    Tcl_ListObjAppendElement(interp, list, pl->obj->cmdName);
  }

  XOTclObjectListFree(slotObjects);
  Tcl_SetObjResult(interp, list);
  return TCL_OK;
}

/*
infoObjectMethod vars XOTclObjInfoVarsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
*/
static int XOTclObjInfoVarsMethod(Tcl_Interp *interp, XOTclObject *object, CONST char *pattern) {
  Tcl_Obj *varlist, *okList, *element;
  int i, length;
  TclVarHashTable *varTable = object->nsPtr ? Tcl_Namespace_varTable(object->nsPtr) : object->varTable;

  ListVarKeys(interp, VarHashTable(varTable), pattern);
  varlist = Tcl_GetObjResult(interp);

  Tcl_ListObjLength(interp, varlist, &length);
  okList = Tcl_NewListObj(0, NULL);
  for (i=0; i<length; i++) {
    Tcl_ListObjIndex(interp, varlist, i, &element);
    if (varExists(interp, object, ObjStr(element), NULL, 0, 1)) {
      Tcl_ListObjAppendElement(interp, okList, element);
    } else {
      /*fprintf(stderr, "must ignore '%s' %d\n", ObjStr(element), i);*/
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
/*
infoClassMethod heritage XOTclClassInfoHeritageMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "pattern"}
}
*/
static int XOTclClassInfoHeritageMethod(Tcl_Interp *interp, XOTclClass *cl, CONST char *pattern) {
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
                              int withClosure, CONST char *pattern, XOTclObject *matchObject) {
  Tcl_HashTable *table = &startCl->instances;
  XOTclClasses *sc;
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;
  int rc = 0;

  /*fprintf(stderr, "XOTclClassInfoInstancesMethod: clo %d pattern %s match %p\n",
    withClosure, pattern, matchObject);*/

  for (hPtr = Tcl_FirstHashEntry(table, &search);  hPtr;
       hPtr = Tcl_NextHashEntry(&search)) {
    XOTclObject *inst = (XOTclObject*) Tcl_GetHashKey(table, hPtr);
    /*fprintf(stderr, "match '%s' %p %p '%s'\n",
      matchObject ? objectName(matchObject) : "NULL", matchObject, inst, objectName(inst));*/
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

/*
infoClassMethod instances XOTclClassInfoInstancesMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
*/
static int XOTclClassInfoInstancesMethod(Tcl_Interp *interp, XOTclClass *startCl,
                              int withClosure, CONST char *pattern, XOTclObject *matchObject) {
  XOTclClassInfoInstancesMethod1(interp, startCl, withClosure, pattern, matchObject);
  return TCL_OK;
}

/*
infoClassMethod filter XOTclClassInfoFilterMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "-guard"}
  {-argName "-guards"}
  {-argName "pattern"}
}
*/

static int XOTclClassInfoFilterMethod(Tcl_Interp *interp, XOTclClass *class, 
				      int withGuard, int withGuards, 
				      CONST char *pattern) {
  if (withGuard) {
    return class->opt ? GuardList(interp, class->opt->classfilters, pattern) : TCL_OK;
  }
  return class->opt ? FilterInfo(interp, class->opt->classfilters, pattern, withGuards, 0) : TCL_OK;
}

/*
infoClassMethod forward XOTclClassInfoForwardMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-definition"}
  {-argName "name"}
}
*/
static int XOTclClassInfoForwardMethod(Tcl_Interp *interp, XOTclClass *class,
				int withDefinition, CONST char *pattern) {
  return ListForward(interp, Tcl_Namespace_cmdTable(class->nsPtr), pattern, withDefinition);
}

/*
infoClassMethod method XOTclClassInfoMethodMethod {
  {-argName "class" -type class}
  {-argName "infomethodsubcmd" -type "args|body|definition|handle|parameter|type|precondition|postcondition"}
  {-argName "name"}
}
*/
static int XOTclClassInfoMethodMethod(Tcl_Interp *interp, XOTclClass *class, 
                                      int subcmd, CONST char *methodName) {
  Tcl_Namespace *nsPtr = class->nsPtr;
  Tcl_Command cmd;

  if (*methodName == ':') {
    Tcl_Obj *methodObj = Tcl_NewStringObj(methodName, -1);
    cmd = Tcl_GetCommandFromObj(interp, methodObj);
  } else {
    cmd = nsPtr ? FindMethod(nsPtr, methodName) : NULL;
  }
  return ListMethod(interp, &class->object, methodName, cmd, subcmd, 0);
}

/*
infoClassMethod methods XOTclClassInfoMethodsMethod {
  {-argName "object" -type class}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern"}
}
*/
static int XOTclClassInfoMethodsMethod(Tcl_Interp *interp, XOTclClass *class, 
                                       int withMethodtype, int withCallproctection,
                                       int withNomixins, int withIncontext, CONST char *pattern) {
  return ListDefinedMethods(interp, &class->object, pattern, 0 /* per-object */, 
                            AggregatedMethodType(withMethodtype), withCallproctection,
                            withNomixins, withIncontext);
}

/*
infoClassMethod mixin XOTclClassInfoMixinMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "-guard"}
  {-argName "-guards"}
  {-argName "pattern" -type objpattern}
}
*/
static int XOTclClassInfoMixinMethod(Tcl_Interp *interp, XOTclClass *class, 
				     int withClosure, int withGuard, int withGuards,
			      CONST char *patternString, XOTclObject *patternObj) {
  XOTclClassOpt *opt = class->opt;
  int rc;

  if (withGuard) {
    return opt ? GuardList(interp, opt->classmixins, patternString) : TCL_OK;
  }
  if (withClosure) {
    Tcl_HashTable objTable, *commandTable = &objTable;
    MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
    Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
    rc = getAllClassMixins(interp, commandTable, class, withGuards, patternString, patternObj);
    if (patternObj && rc && !withGuards) {
      Tcl_SetObjResult(interp, rc ? patternObj->cmdName : XOTclGlobalObjs[XOTE_EMPTY]);
    }
    Tcl_DeleteHashTable(commandTable);
    MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
  } else {
    rc = opt ? MixinInfo(interp, opt->classmixins, patternString, withGuards, patternObj) : TCL_OK;
  }

  return TCL_OK;
}

/*
infoClassMethod mixinof  XOTclClassInfoMixinOfMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "-scope" -required 0 -nrargs 1 -type "all|class|object"}
  {-argName "pattern" -type objpattern}
}
*/
static int XOTclClassInfoMixinOfMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, int withScope,
                                       CONST char *patternString, XOTclObject *patternObj) {
  XOTclClassOpt *opt = class->opt;
  int perClass, perObject;
  int rc;

  if (withScope == ScopeNULL || withScope == ScopeAllIdx) {
    perClass = 1;
    perObject = 1;
  } else if (withScope == ScopeClassIdx) {
    perClass = 1;
    perObject = 0;
  } else {
    perClass = 0;
    perObject = 1;
  }

  if (opt && !withClosure) {
    if (perClass) {
      rc = AppendMatchingElementsFromCmdList(interp, opt->isClassMixinOf, patternString, patternObj);
      if (rc && patternObj) {goto finished;}
    }
    if (perObject) {
      rc = AppendMatchingElementsFromCmdList(interp, opt->isObjectMixinOf, patternString, patternObj);
    }
  } else if (withClosure) {
    Tcl_HashTable objTable, *commandTable = &objTable;
    MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
    Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
    if (perClass) {
      rc = getAllClassMixinsOf(interp, commandTable, class, 0, 1, patternString, patternObj);
      if (rc && patternObj) {goto finished;}
    }
    if (perObject) {
      rc = getAllObjectMixinsOf(interp, commandTable, class, 0, 1, patternString, patternObj);
    }
    Tcl_DeleteHashTable(commandTable);
    MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
  }
  
 finished:
  if (patternObj) {
    Tcl_SetObjResult(interp, rc ? patternObj->cmdName : XOTclGlobalObjs[XOTE_EMPTY]);
  }
  return TCL_OK;
}

/*
infoClassMethod slots XOTclClassInfoSlotsMethod {
  {-argName "class"  -required 1 -type class}
}
*/
static int XOTclClassInfoSlotsMethod(Tcl_Interp *interp, XOTclClass *class) {
  Tcl_DString ds, *dsPtr = &ds;
  XOTclObject *object;
  int result;

  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, className(class), -1);
  Tcl_DStringAppend(dsPtr, "::slot", 6);
  object = XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
  if (object) {
    result = ListChildren(interp, object, NULL, 0);
  } else {
    result = TCL_OK;
  }
  DSTRING_FREE(dsPtr);
  return result;
}

/*
infoClassMethod subclass XOTclClassInfoSubclassMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
*/
static int XOTclClassInfoSubclassMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure,
                             CONST char *patternString, XOTclObject *patternObj) {
  int rc;
  if (withClosure) {
    XOTclClasses *saved = class->order, *subclasses;
    class->order = NULL;
    subclasses = ComputeOrder(class, class->order, Sub);
    class->order = saved;
    rc = AppendMatchingElementsFromClasses(interp, subclasses ? subclasses->nextPtr:NULL, 
					   patternString, patternObj);
    XOTclClassListFree(subclasses);
  } else {
    rc = AppendMatchingElementsFromClasses(interp, class->sub, patternString, patternObj);
  }

  if (patternObj) {
    Tcl_SetObjResult(interp, rc ? patternObj->cmdName : XOTclGlobalObjs[XOTE_EMPTY]);
  }

  return TCL_OK;
}

/*
infoClassMethod superclass XOTclClassInfoSuperclassMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type tclobj}
}
*/
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
ProcessMethodArguments(parseContext *pcPtr, Tcl_Interp *interp,
                       XOTclObject *object, int pushFrame,
                       XOTclParamDefs *paramDefs,
                       CONST char *methodName, int objc, Tcl_Obj *CONST objv[]) {
  int result;
  Tcl_CallFrame frame, *framePtr = &frame;

  if (object && pushFrame) {
    XOTcl_PushFrameObj(interp, object, framePtr);
  }

  result = ArgumentParse(interp, objc, objv, object, objv[0],
                         paramDefs->paramsPtr, paramDefs->nrParams, pcPtr);
  if (object && pushFrame) {
    XOTcl_PopFrameObj(interp, framePtr);
  }
  if (result != TCL_OK) {
    return result;
  }

  /*
   * Set objc of the parse context to the number of defined parameters.
   * pcPtr->objc and paramDefs->nrParams will be equivalent in cases
   * where argument values are passed to the call in absence of var
   * args ('args'). Treating "args is more involved (see below).
   */
  pcPtr->objc = paramDefs->nrParams + 1;

  if (pcPtr->varArgs) {
    /*
     * The last argument was "args".
     */
    int elts = objc - pcPtr->lastobjc;

    if (elts == 0) {
      /*
       * No arguments were passed to "args".  We simply decrement objc.
       */
      pcPtr->objc--;
    } else if (elts > 1) {
      /*
       * Multiple arguments were passed to "args".  pcPtr->objv is
       * pointing to the first of the var args. We have to copy the
       * remaining actual argument vector objv to the parse context.
       */

      /*XOTclPrintObjv("actual:  ", objc, objv);*/
      parseContextExtendObjv(pcPtr, paramDefs->nrParams, elts-1, objv + 1 + pcPtr->lastobjc);
    } else {
      /*
       * A single argument was passed to "args". There is no need to
       * mutate the pcPtr->objv, because this has been achieved in
       * ArgumentParse (i.e., pcPtr->objv[i] contains this element).
       */
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
              XOTclGlobalObjs[XOTE___UNKNOWN__]);*/
      if (varPtr->value.objPtr != XOTclGlobalObjs[XOTE___UNKNOWN__]) continue;
      /*fprintf(stderr, "XOTclUnsetUnknownArgsCmd must unset %s\n", ap->name);*/
      Tcl_UnsetVar2(interp, ap->name, NULL, 0);
    }
  }

  return TCL_OK;
}

#if !defined(NDEBUG)
static void
checkAllInstances(Tcl_Interp *interp, XOTclClass *cl, int lvl) {
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;
  if (cl && cl->object.refCount>0) {
    /*fprintf(stderr, "checkallinstances %d cl=%p '%s'\n", lvl, cl, className(cl));*/
    for (hPtr = Tcl_FirstHashEntry(&cl->instances, &search);  hPtr;
         hPtr = Tcl_NextHashEntry(&search)) {
      XOTclObject *inst = (XOTclObject*) Tcl_GetHashKey(&cl->instances, hPtr);
      assert(inst);
      assert(inst->refCount>0);
      assert(inst->cmdName->refCount>0);
      if (XOTclObjectIsClass(inst)) {
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
ObjectHasChildren(Tcl_Interp *interp, XOTclObject *object) {
  Tcl_Namespace *ns = object->nsPtr;
  int result = 0;

  if (ns) {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch hSrch;
    Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(ns);

    for (hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch); hPtr;
         hPtr = Tcl_NextHashEntry(&hSrch)) {
      Tcl_Command cmd = Tcl_GetHashValue(hPtr);
      XOTclObject *childObject = XOTclGetObjectFromCmdPtr(cmd);
      
      if (childObject) {
        result = 1;
        break;
      }
    }
  }
  return result;
}

static void 
finalObjectDeletion(Tcl_Interp *interp, XOTclObject *object) {
  /* If a call to exit happens from a higher stack frame, the
     obejct refcount might not be decremented corectly. If we are
     in the phyical destroy round, we can set the counter to an
     appropriate value to ensure deletion.
     
     todo: remove debug line
  */
  if (object->refCount != 1) {
    fprintf(stderr, "*** have to fix refcount for obj %p refcount %d\n",object, object->refCount);
    object->refCount = 1;
  }
  assert(object->activationCount == 0);
  /*fprintf(stderr, "finalObjectDeletion obj %p activationcount %d\n", object, object->activationCount);*/
  if (object->id) {
    /*fprintf(stderr, "cmd dealloc %p final delete refCount %d\n", object->id, Tcl_Command_refCount(object->id));*/
    Tcl_DeleteCommandFromToken(interp, object->id);
  }
}

static void
freeAllXOTclObjectsAndClasses(Tcl_Interp *interp, Tcl_HashTable *commandNameTable) {
  Tcl_HashEntry *hPtr, *hPtr2;
  Tcl_HashSearch hSrch, hSrch2;
  XOTclObject *object;
  int deleted = 0;
 
  /*fprintf(stderr, "freeAllXOTclObjectsAndClasses in %p\n", interp);*/

  RUNTIME_STATE(interp)->exitHandlerDestroyRound = XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY;

  /*
   * First delete all child commands of all objects, which are not
   * objects themselves. This will for example delete namespace
   * imprted commands and objects and will resolve potential loops in
   * the dependency graph. The result is a plain object/class tree.
   */
  for (hPtr = Tcl_FirstHashEntry(commandNameTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(commandNameTable, hPtr);
    object = XOTclpGetObject(interp, key);

    if (object && object->nsPtr) {
      for (hPtr2 = Tcl_FirstHashEntry(Tcl_Namespace_cmdTable(object->nsPtr), &hSrch2); hPtr2;
           hPtr2 = Tcl_NextHashEntry(&hSrch2)) {
        Tcl_Command cmd = Tcl_GetHashValue(hPtr2);
        if (cmd &&  Tcl_Command_objProc(cmd) != XOTclObjDispatch) {
          Tcl_DeleteCommandFromToken(interp, cmd);
          deleted ++;
        }
      }
    }
  }
  /*fprintf(stderr, "deleted %d cmds\n", deleted);*/

  /*
   * Finally delete the object/class tree in a bottom up manner,
   * deleteing all objects without dependencies first. Finally, only
   * the root classes of the object system will remain, which are
   * deleted separately.
   */

  while (1) {
    /*
     * Delete all plain objects without dependencies
     */
    deleted = 0;
    for (hPtr = Tcl_FirstHashEntry(commandNameTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      char *key = Tcl_GetHashKey(commandNameTable, hPtr);

      object = XOTclpGetObject(interp, key);
      if (object && !XOTclObjectIsClass(object) && !ObjectHasChildren(interp, object)) {
        /*fprintf(stderr, "  ... delete object %s %p, class=%s id %p\n", key, object,
          className(object->cl), object->id);*/

        freeUnsetTraceVariable(interp, object);
        if (object->id) finalObjectDeletion(interp, object);
        Tcl_DeleteHashEntry(hPtr);
        deleted++;
      }
    }
    /*fprintf(stderr, "deleted %d Objects\n", deleted);*/
    if (deleted > 0) {
      continue;
    }

    /*
     * Delete all classes without dependencies
     */
    for (hPtr = Tcl_FirstHashEntry(commandNameTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      char *key = Tcl_GetHashKey(commandNameTable, hPtr);
      XOTclClass *cl = XOTclpGetClass(interp, key);
      
      /*fprintf(stderr, "cl key = %s %p\n", key, cl);*/
      if (cl
          && !ObjectHasChildren(interp, (XOTclObject*)cl)
          && !ClassHasInstances(cl)
          && !ClassHasSubclasses(cl)
          && !IsBaseClass(cl)
          ) {
        /*fprintf(stderr, "  ... delete class %s %p\n", key, cl); */
        freeUnsetTraceVariable(interp, &cl->object);
        if (cl->object.id) finalObjectDeletion(interp, &cl->object);

        Tcl_DeleteHashEntry(hPtr);
        deleted++;
      }
    }
    /*fprintf(stderr, "deleted %d Classes\n", deleted);*/
    if (deleted == 0) {
      break;
    }
  }
}

#endif /* DO_CLEANUP */

/*
 *  Exit Handler
 */
static void
ExitHandler(ClientData clientData) {
  Tcl_Interp *interp = (Tcl_Interp *)clientData;
  int i, flags;

  /*fprintf(stderr, "ExitHandler\n");*/

  /*
   * Don't use exit handler, if the interpreter is alread destroyed.
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

  /* must be before freeing of XOTclGlobalObjs */
  XOTclShadowTclCommands(interp, SHADOW_UNLOAD);

  /* free global objects */
  for (i = 0; i < nr_elements(XOTclGlobalStrings); i++) {
    DECR_REF_COUNT(XOTclGlobalObjs[i]);
  }
  XOTclStringIncrFree(&RUNTIME_STATE(interp)->iss);

#if defined(TCL_MEM_DEBUG)
  TclDumpMemoryInfo(stderr);
  Tcl_DumpActiveMemory("./xotclActiveMem");
  /* Tcl_GlobalEval(interp, "puts {checkmem to checkmemFile};
     checkmem checkmemFile"); */
#endif
  MEM_COUNT_DUMP();

  FREE(Tcl_Obj**, XOTclGlobalObjs);
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
  /*fprintf(stderr, "+++ XOTcl_ThreadExitProc\n");*/

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
  /*fprintf(stderr, "+++ XOTcl_ExitProc\n");*/
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

/*
 * Tcl extension initialization routine
 */

extern int
Nx_Init(Tcl_Interp *interp) {
  ClientData runtimeState;
  int result, i;
#ifdef XOTCL_BYTECODE
  XOTclCompEnv *interpstructions = XOTclGetCompEnv();
#endif
  static XOTclMutex initMutex = 0;

#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
    return TCL_ERROR;
  }
#endif

#if defined(TCL_MEM_DEBUG)
  TclDumpMemoryInfo(stderr);
#endif

  MEM_COUNT_INIT();
  
  /* init global variables for tcl types */
  XOTclMutexLock(&initMutex);
  byteCodeType = Tcl_GetObjType("bytecode");
  tclCmdNameType = Tcl_GetObjType("cmdName");
  listType = Tcl_GetObjType("list");
  XOTclMutexUnlock(&initMutex);

  /*
    fprintf(stderr, "SIZES: obj=%d, tcl_obj=%d, DString=%d, class=%d, namespace=%d, command=%d, HashTable=%d\n", 
    sizeof(XOTclObject), sizeof(Tcl_Obj), sizeof(Tcl_DString), sizeof(XOTclClass), 
    sizeof(Namespace), sizeof(Command), sizeof(Tcl_HashTable));
  */

  /*
   * Runtime State stored in the client data of the Interp's global
   * Namespace in order to avoid global state information
   */
  runtimeState = (ClientData) NEW(XOTclRuntimeState);
  memset(runtimeState, 0, sizeof(XOTclRuntimeState));

#if USE_ASSOC_DATA
  Tcl_SetAssocData(interp, "XOTclRuntimeState", NULL, runtimeState);
#else
  Tcl_Interp_globalNsPtr(interp)->clientData = runtimeState;
#endif

  RUNTIME_STATE(interp)->doFilters = 1;

  /* create xotcl namespace */
  RUNTIME_STATE(interp)->XOTclNS =
    Tcl_CreateNamespace(interp, "::nsf", (ClientData)NULL, (Tcl_NamespaceDeleteProc*)NULL);

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
    Tcl_CreateNamespace(interp, "::nsf::classes",	(ClientData)NULL,
                        (Tcl_NamespaceDeleteProc*)NULL);
  MEM_COUNT_ALLOC("TclNamespace", RUNTIME_STATE(interp)->XOTclClassesNS);


  /* cache interpreters proc interpretation functions */
  RUNTIME_STATE(interp)->objInterpProc = TclGetObjInterpProc();
  RUNTIME_STATE(interp)->exitHandlerDestroyRound = XOTCL_EXITHANDLER_OFF;

  RegisterExitHandlers((ClientData)interp);
  XOTclStringIncrInit(&RUNTIME_STATE(interp)->iss);
  /* initialize global Tcl_Obj */
  XOTclGlobalObjs = NEW_ARRAY(Tcl_Obj*, nr_elements(XOTclGlobalStrings));

  for (i = 0; i < nr_elements(XOTclGlobalStrings); i++) {
    XOTclGlobalObjs[i] = Tcl_NewStringObj(XOTclGlobalStrings[i], -1);
    INCR_REF_COUNT(XOTclGlobalObjs[i]);
  }

  /* create namespaces for the different command types */
  Tcl_CreateNamespace(interp, "::nsf::cmd", 0, (Tcl_NamespaceDeleteProc*)NULL);
  for (i=0; i < nr_elements(method_command_namespace_names); i++) {
    Tcl_CreateNamespace(interp, method_command_namespace_names[i], 0, (Tcl_NamespaceDeleteProc*)NULL);
  }

  /* create all method commands (will use the namespaces above) */
  for (i=0; i < nr_elements(method_definitions)-1; i++) {
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
    Tcl_CreateObjCommand(interp, "::nsf::next", XOTclNextObjCmd, 0, 0);
#ifdef XOTCL_BYTECODE
  instructions[INST_SELF].cmdPtr = (Command *)Tcl_FindCommand(interp, "::nsf::current", 0, 0);
#endif
  /*Tcl_CreateObjCommand(interp, "::nsf::K", XOTclKObjCmd, 0, 0);*/

  Tcl_CreateObjCommand(interp, "::nsf::unsetUnknownArgs", XOTclUnsetUnknownArgsCmd, 0, 0);

#ifdef XOTCL_BYTECODE
  XOTclBytecodeInit();
#endif

  Tcl_SetVar(interp, "::nsf::version", NXVERSION, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::patchlevel", NXPATCHLEVEL, TCL_GLOBAL_ONLY);

  Tcl_AddInterpResolvers(interp,"nxt", 
                         (Tcl_ResolveCmdProc*)InterpColonCmdResolver,
                         InterpColonVarResolver, 
                         (Tcl_ResolveCompiledVarProc*)InterpCompiledColonVarResolver);
  RUNTIME_STATE(interp)->colonCmd = Tcl_FindCommand(interp, "::nsf::colon", 0, 0);

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
#  if defined(PRE86)
  Tcl_PkgProvideEx(interp, "nsf", PACKAGE_VERSION, (ClientData)&xotclStubs);
#  else
  Tcl_PkgProvideEx(interp, "nsf", PACKAGE_VERSION, (ClientData)&xotclConstStubPtr);
#  endif
# else
  Tcl_PkgProvide(interp, "nsf", PACKAGE_VERSION);
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
Next_SafeInit(Tcl_Interp *interp) {
  /*** dummy for now **/
  return Nx_Init(interp);
}

