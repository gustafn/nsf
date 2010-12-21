/* 
 *  Next Scripting Framework 
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

#define NSF_C 1
#include "nsfInt.h"
#include "nsfAccessInt.h"

#ifdef COMPILE_NSF_STUBS
# if defined(PRE86)
extern NsfStubs nsfStubs;
# else
MODULE_SCOPE const NsfStubs * const nsfConstStubPtr;
# endif
#endif

#ifdef NSF_MEM_COUNT
int nsfMemCountInterpCounter = 0;
#endif

#ifdef USE_TCL_STUBS
# define Nsf_ExprObjCmd(clientData, interp, objc, objv)	\
  NsfCallCommand(interp, NSF_EXPR, objc, objv)
# define Nsf_SubstObjCmd(clientData, interp, objc, objv)	\
  NsfCallCommand(interp, NSF_SUBST, objc, objv)
#else
# define Nsf_ExprObjCmd(clientData, interp, objc, objv)	\
  Tcl_ExprObjCmd(clientData, interp, objc, objv)
# define Nsf_SubstObjCmd(clientData, interp, objc, objv)	\
  Tcl_SubstObjCmd(clientData, interp, objc, objv)
#endif

/*
 * Call Stack specific definitions
 */
typedef enum { CALLING_LEVEL, ACTIVE_LEVEL } CallStackLevel;

typedef struct callFrameContext {
  int frameSaved;
  Tcl_CallFrame *framePtr;
  Tcl_CallFrame *varFramePtr;
} callFrameContext;

typedef struct NsfProcContext {
  ClientData oldDeleteData;
  Tcl_CmdDeleteProc *oldDeleteProc;
  NsfParamDefs *paramDefs;
} NsfProcContext;

/*
 * TclCmdClientdata is an incomplete type containing the common
 * field(s) of ForwardCmdClientData, AliasCmdClientData and
 * SetterCmdClientData used for filling in at runtime the actual
 * object.
 */
typedef struct TclCmdClientData {
  NsfObject *object;
} TclCmdClientData;

typedef struct SetterCmdClientData {
  NsfObject *object;
  NsfParam *paramsPtr;
} SetterCmdClientData;

typedef struct ForwardCmdClientData {
  NsfObject *object;
  Tcl_Obj *cmdName;
  Tcl_ObjCmdProc *objProc;
  ClientData clientData;
  int passthrough;
  int needobjmap;
  int verbose;
  int hasNonposArgs;
  int nr_args;
  Tcl_Obj *args;
  int objframe;
  Tcl_Obj *onerror;
  Tcl_Obj *prefix;
  int nr_subcommands;
  Tcl_Obj *subcommands;
} ForwardCmdClientData;

typedef struct AliasCmdClientData {
  NsfObject *object;
  Tcl_Obj *cmdName;
  Tcl_ObjCmdProc *objProc;
  ClientData clientData;
  NsfClass *class;
  Tcl_Interp *interp;
  Tcl_Command aliasedCmd;
  Tcl_Command aliasCmd;
} AliasCmdClientData;

/*
 * Argv parsing specific definitions
 */

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
  int status;
  int varArgs;
  NsfObject *object;
} ParseContext;

static NsfTypeConverter ConvertToNothing, ConvertViaCmd, ConvertToClass;

typedef struct {
  NsfTypeConverter *converter;
  char *domain;
} enumeratorConverterEntry;
static enumeratorConverterEntry enumeratorConverterEntries[];

/*
 * Tcl_Obj Types for Next Scripting Objects
 */

static Tcl_ObjType CONST86
  *Nsf_OT_byteCodeType = NULL,
  *Nsf_OT_tclCmdNameType = NULL,
  *Nsf_OT_listType = NULL;

/*
 * Function prototypes
 */

/* Prototypes for method definitions */
static Tcl_ObjCmdProc NsfForwardMethod;
static Tcl_ObjCmdProc NsfObjscopedMethod;
static Tcl_ObjCmdProc NsfSetterMethod;
static Tcl_ObjCmdProc NsfProcAliasMethod;

/* prototypes for methods called directly when CallDirectly() returns NULL */
static int NsfCAllocMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *nameObj);
static int NsfCCreateMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *name, int objc, Tcl_Obj *CONST objv[]);
static int NsfOCleanupMethod(Tcl_Interp *interp, NsfObject *object);
static int NsfOConfigureMethod(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]);
static int NsfODestroyMethod(Tcl_Interp *interp, NsfObject *object);
static int NsfOResidualargsMethod(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]);
static int MethodDispatch(ClientData clientData, Tcl_Interp *interp,
			  int objc, Tcl_Obj *CONST objv[],
			  Tcl_Command cmd, NsfObject *object, NsfClass *cl,
			  CONST char *methodName, int frameType, int flags);
static int DispatchDefaultMethod(ClientData clientData, Tcl_Interp *interp,
				 int objc, Tcl_Obj *CONST objv[], int flags);
static int DispatchDestroyMethod(Tcl_Interp *interp, NsfObject *object, int flags);
static int DispatchUnknownMethod(ClientData clientData, Tcl_Interp *interp,
				 int objc, Tcl_Obj *CONST objv[], NsfObject *delegator,
				 Tcl_Obj *methodObj, int flags);

NSF_INLINE static int ObjectDispatch(ClientData clientData, Tcl_Interp *interp, int objc,
                                      Tcl_Obj *CONST objv[], int flags);
//TODO remove string, methodName
NSF_INLINE static int ObjectDispatchFinalize(Tcl_Interp *interp, NsfCallStackContent *cscPtr,
					     int result /*, char *string , CONST char *methodName*/);

/* prototypes for object life-cycle management */
static int RecreateObject(Tcl_Interp *interp, NsfClass *cl, NsfObject *object, int objc, Tcl_Obj *CONST objv[]);
static void FinalObjectDeletion(Tcl_Interp *interp, NsfObject *object);
static void FreeAllNsfObjectsAndClasses(Tcl_Interp *interp, Tcl_HashTable *commandNameTablePtr);
static void CallStackDestroyObject(Tcl_Interp *interp, NsfObject *object);
static void PrimitiveCDestroy(ClientData clientData);
static void PrimitiveODestroy(ClientData clientData);
static void PrimitiveDestroy(ClientData clientData);
static void NsfCleanupObject_(NsfObject *object);

/* prototypes for object and command lookup */
static NsfObject *GetObjectFromString(Tcl_Interp *interp, CONST char *name);
static NsfClass *GetClassFromString(Tcl_Interp *interp, CONST char *name);
static void GetAllInstances(Tcl_Interp *interp, Tcl_HashTable *destTablePtr, NsfClass *startClass);
NSF_INLINE static Tcl_Command FindMethod(Tcl_Namespace *nsPtr, CONST char *methodName);

/* prototypes for namespace specific calls */
static Tcl_Obj *NameInNamespaceObj(Tcl_Interp *interp, CONST char *name, Tcl_Namespace *ns);
static Tcl_Namespace *CallingNameSpace(Tcl_Interp *interp);
NSF_INLINE static Tcl_Command NSFindCommand(Tcl_Interp *interp, CONST char *name);
static Tcl_Namespace *NSGetFreshNamespace(Tcl_Interp *interp, ClientData clientData,
					  CONST char *name);

/* prototypes for filters and mixins */
static void FilterComputeDefined(Tcl_Interp *interp, NsfObject *object);
static void MixinComputeDefined(Tcl_Interp *interp, NsfObject *object);
NSF_INLINE static void GuardAdd(Tcl_Interp *interp, NsfCmdList *filterCL, Tcl_Obj *guardObj);
static int GuardCall(NsfObject *object, NsfClass *cl, Tcl_Command cmd, Tcl_Interp *interp,
                     Tcl_Obj *guardObj, NsfCallStackContent *cscPtr);
static void GuardDel(NsfCmdList *filterCL);

/* properties of objects and classes */
static int IsBaseClass(NsfClass *cl);
static int IsMetaClass(Tcl_Interp *interp, NsfClass *cl, int withMixins);
static int IsSubType(NsfClass *subcl, NsfClass *cl);
static NsfClass *DefaultSuperClass(Tcl_Interp *interp, NsfClass *cl, NsfClass *mcl, int isMeta);

/* prototypes for call stack specific calls */
NSF_INLINE static void CscInit(NsfCallStackContent *cscPtr, NsfObject *object, NsfClass *cl,
			       Tcl_Command cmd, int frameType, int flags);
NSF_INLINE static void CscFinish_(Tcl_Interp *interp, NsfCallStackContent *cscPtr);
NSF_INLINE static void CallStackDoDestroy(Tcl_Interp *interp, NsfObject *object);

/* prototypes for  parameter and argument management */
static int NsfInvalidateObjectParameterCmd(Tcl_Interp *interp, NsfClass *cl);
static int ProcessMethodArguments(ParseContext *pcPtr, Tcl_Interp *interp,
                                  NsfObject *object, int pushFrame, NsfParamDefs *paramDefs,
                                  CONST char *methodName, int objc, Tcl_Obj *CONST objv[]);
static int ArgumentCheck(Tcl_Interp *interp, Tcl_Obj *objPtr, struct NsfParam CONST *pPtr, int doCheck,
			 int *flags, ClientData *clientData, Tcl_Obj **outObjPtr);
static int ParameterCheck(Tcl_Interp *interp, Tcl_Obj *objPtr, Tcl_Obj *valueObj,
			  const char *varNamePrefix, int doCheck, NsfParam **paramPtrPtr);
static void ParamDefsFree(NsfParamDefs *paramDefs);
static int ParamSetFromAny(Tcl_Interp *interp,	register Tcl_Obj *objPtr);


/* prototypes for alias management */
static int AliasDelete(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object);
static Tcl_Obj *AliasGet(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object);
static int AliasDeleteObjectReference(Tcl_Interp *interp, Tcl_Command cmd);

/* prototypes for (class) list handling */
extern NsfClasses ** NsfClassListAdd(NsfClasses **firstPtrPtr, NsfClass *cl, ClientData clientData);
extern void NsfClassListFree(NsfClasses *firstPtr);

/* misc prototypes */
static int NsfDeprecatedCmd(Tcl_Interp *interp, CONST char *what, CONST char *oldCmd, CONST char *newCmd);
static int SetInstVar(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *nameObj, Tcl_Obj *valueObj);
static int ListDefinedMethods(Tcl_Interp *interp, NsfObject *object, CONST char *pattern,
			      int withPer_object, int methodType, int withCallproctection,
			      int withExpand, int noMixins, int inContext);
static int NextSearchAndInvoke(Tcl_Interp *interp,
			       CONST char *methodName, int objc, Tcl_Obj *CONST objv[],
			       NsfCallStackContent *cscPtr, int freeArgumentVector);

/*
 *----------------------------------------------------------------------
 *
 * NsfLog --
 *
 *      Produce a formatted warning by calling an external function
 *      ::nsf::log. It is defined static to allow for inlining.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static void
NsfLog(Tcl_Interp *interp, int requiredLevel, CONST char *fmt, ...) {
  va_list ap;

  if (RUNTIME_STATE(interp)->debugLevel >= requiredLevel) {
    CONST char *level = requiredLevel == NSF_LOG_WARN ? "Warning" : "Notice";
    Tcl_DString cmdString, ds;

    Tcl_DStringInit(&ds);
    va_start(ap, fmt);
    NsfDStringPrintf(&ds, fmt, ap);
    va_end(ap);

    Tcl_DStringInit(&cmdString);
    Tcl_DStringAppendElement(&cmdString, "nsf::log");
    Tcl_DStringAppendElement(&cmdString, level);
    Tcl_DStringAppendElement(&cmdString, Tcl_DStringValue(&ds));

    int result = Tcl_EvalEx(interp, Tcl_DStringValue(&cmdString), Tcl_DStringLength(&cmdString), 0);
    if (result == TCL_ERROR) {
      static char cmdString[] =
	"puts stderr \"Error in logger\n\
      $::errorCode $::errorInfo\"";
      Tcl_EvalEx(interp, cmdString, -1, 0);
    }
    Tcl_DStringFree(&cmdString);
    Tcl_DStringFree(&ds);
  }
}

/*
 * argv parsing
 */

static void
ParseContextInit(ParseContext *pcPtr, int objc, NsfObject *object, Tcl_Obj *procName) {
  if (objc < PARSE_CONTEXT_PREALLOC) {
    /* the single larger memset below .... */
    memset(pcPtr, 0, sizeof(ParseContext));
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
    pcPtr->status     = NSF_PC_STATUS_FREE_OBJV|NSF_PC_STATUS_FREE_CD;
    pcPtr->varArgs    = 0;
  }
  pcPtr->objv = &pcPtr->full_objv[1];
  pcPtr->full_objv[0] = procName;
  pcPtr->object = object;
}

static void
ParseContextExtendObjv(ParseContext *pcPtr, int from, int elts, Tcl_Obj *CONST source[]) {
  int requiredSize = from + elts + 1;

  /*NsfPrintObjv("BEFORE: ", pcPtr->objc, pcPtr->full_objv);*/

  if (requiredSize >= PARSE_CONTEXT_PREALLOC) {
    if (pcPtr->objv == &pcPtr->objv_static[1]) {
      /* realloc from preallocated memory */
      pcPtr->full_objv  = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*) * requiredSize);
      memcpy(pcPtr->full_objv, &pcPtr->objv_static[0], sizeof(Tcl_Obj*) * PARSE_CONTEXT_PREALLOC);
      /*fprintf(stderr, "alloc %d new objv=%p pcPtr %p\n", requiredSize, pcPtr->full_objv, pcPtr);*/
      pcPtr->status     |= NSF_PC_STATUS_FREE_OBJV;
    } else {
      /* realloc from mallocated memory */
      pcPtr->full_objv = (Tcl_Obj **)ckrealloc((char *)pcPtr->full_objv, sizeof(Tcl_Obj*) * requiredSize);
      /*fprintf(stderr, "realloc %d  new objv=%p pcPtr %p\n", requiredSize, pcPtr->full_objv, pcPtr);*/
    }
    pcPtr->objv = &pcPtr->full_objv[1];
  }

  memcpy(pcPtr->objv + from, source, sizeof(Tcl_Obj *) * (elts));
  pcPtr->objc += elts;

  /*NsfPrintObjv("AFTER:  ", pcPtr->objc, pcPtr->full_objv);*/
}

static void
ParseContextRelease(ParseContext *pcPtr) {
  int status = pcPtr->status;

  if (status) {
    if (status & NSF_PC_STATUS_MUST_DECR) {
      int i;
      for (i = 0; i < pcPtr->lastobjc; i++) {
	if (pcPtr->flags[i] & NSF_PC_MUST_DECR) {
	  DECR_REF_COUNT(pcPtr->objv[i]);
	}
      }
    }
    
    /* objv can be separately extended */
    if (status & NSF_PC_STATUS_FREE_OBJV) {
      /*fprintf(stderr, "ParseContextRelease %p free %p %p\n", pcPtr, pcPtr->full_objv, pcPtr->clientData);*/
      ckfree((char *)pcPtr->full_objv);
    }
    /* if the parameter definition was extended, both clientData and flags are extended */
    if (status & NSF_PC_STATUS_FREE_CD) {
      /*fprintf(stderr, "free clientdata and flags\n");*/
      ckfree((char *)pcPtr->clientData);
      ckfree((char *)pcPtr->flags);
    }
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
#define VarHashTablePtr(varTablePtr)  &(varTablePtr)->table
#define valueOfVar(type, varPtr, field) (type *)(varPtr)->value.field

NSF_INLINE static Tcl_Namespace *
ObjFindNamespace(Tcl_Interp *interp, Tcl_Obj *objPtr) {
  Tcl_Namespace *nsPtr;

  if (TclGetNamespaceFromObj(interp, objPtr, &nsPtr) == TCL_OK) {
    return nsPtr;
  } else {
    return NULL;
  }
}

static NSF_INLINE Var *
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

/*
 * call an Next Scripting method
 */
static int
CallMethod(ClientData clientData, Tcl_Interp *interp, Tcl_Obj *methodObj,
           int objc, Tcl_Obj *CONST objv[], int flags) {
  NsfObject *object = (NsfObject*) clientData;
  int result;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);
  /*fprintf(stderr, "%%%% callmethod called with method %p\n", methodObj),*/

  tov[0] = object->cmdName;
  tov[1] = methodObj;

  if (objc>2)
    memcpy(tov+2, objv, sizeof(Tcl_Obj *)*(objc-2));

  /*fprintf(stderr, "%%%% CallMethod cmdname=%s, method=%s, objc=%d\n",
    ObjStr(tov[0]), ObjStr(tov[1]), objc);
    {int i; fprintf(stderr, "\t CALL: %s ", ObjStr(methodObj));for(i=0; i<objc-2; i++) {
    fprintf(stderr, "%s ", ObjStr(objv[i]));} fprintf(stderr, "\n");}*/

  result = ObjectDispatch(clientData, interp, objc, tov, flags);

  FREE_ON_STACK(Tcl_Obj*, tov);
  return result;
}

int
NsfCallMethodWithArgs(ClientData clientData, Tcl_Interp *interp, Tcl_Obj *methodObj, Tcl_Obj *arg,
		      int givenobjc, Tcl_Obj *CONST objv[], int flags) {
  NsfObject *object = (NsfObject*) clientData;
  int objc = givenobjc + 2;
  int result;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

  assert(objc>1);
  tov[0] = object->cmdName;
  tov[1] = methodObj;
  if (objc>2) {
    tov[2] = arg;
  }
  if (objc>3) {
    memcpy(tov+3, objv, sizeof(Tcl_Obj *)*(objc-3));
  }

  /*fprintf(stderr, "%%%% CallMethodWithArgs cmdname=%s, method=%s, arg1 %s objc=%d\n",
	  ObjStr(tov[0]), ObjStr(tov[1]), 
	  objc>2 ? ObjStr(tov[2]) : "",
	  objc);*/
  result = ObjectDispatch(clientData, interp, objc, tov, flags);

  FREE_ON_STACK(Tcl_Obj*, tov);
  return result;
}

#include "nsfStack.c"

/* extern callable GetSelfObj */
Nsf_Object*
NsfGetSelfObj(Tcl_Interp *interp) {
  return (Nsf_Object*)GetSelfObj(interp);
}

/*
 *  NsfObject Reference Accounting
 */
#if defined(NSFOBJ_TRACE)
# define NsfObjectRefCountIncr(obj)					\
  (obj)->refCount++;                                                    \
  fprintf(stderr, "RefCountIncr %p count=%d %s\n", obj, obj->refCount, obj->cmdName?ObjStr(obj->cmdName):"no name"); \
  MEM_COUNT_ALLOC("NsfObject RefCount", obj)
# define NsfObjectRefCountDecr(obj)					\
  (obj)->refCount--;							\
  fprintf(stderr, "RefCountDecr %p count=%d\n", obj, obj->refCount);	\
  MEM_COUNT_FREE("NsfObject RefCount", obj)
#else
# define NsfObjectRefCountIncr(obj)           \
  (obj)->refCount++;                            \
  MEM_COUNT_ALLOC("NsfObject RefCount", obj)
# define NsfObjectRefCountDecr(obj)           \
  (obj)->refCount--;                            \
  MEM_COUNT_FREE("NsfObject RefCount", obj)
#endif

#if defined(NSFOBJ_TRACE)
void
ObjTrace(char *string, NsfObject *object) {
  if (object)
    fprintf(stderr, "--- %s tcl %p %s (%d %p) nsf %p (%d) %s \n", string,
            object->cmdName, object->cmdName->typePtr ? object->cmdName->typePtr->name : "NULL",
            object->cmdName->refCount, object->cmdName->internalRep.twoPtrValue.ptr1,
            object, obj->refCount, ObjectName(object));
  else
    fprintf(stderr, "--- No object: %s\n", string);
}
#else
# define ObjTrace(a, b)
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

NSF_INLINE static int
IsClassNsName(CONST char *string) {
  return (strncmp((string), "::nsf::classes", 14) == 0);
}

/* removes preceding ::nsf::classes from a string */
NSF_INLINE static CONST char *
NSCutNsfClasses(CONST char *string) {
  assert(strncmp((string), "::nsf::classes", 14) == 0);
  return string+14;
}

NSF_INLINE static NsfObject *
GetObjectFromNsName(Tcl_Interp *interp, CONST char *string, int *fromClassNS) {
  /*
   * Get object or class from a fully qualified cmd name, such as
   * e.g. ::nsf::classes::X
   */
  if (IsClassNsName(string)) {
    *fromClassNS = 1;
    return (NsfObject *)GetClassFromString(interp, NSCutNsfClasses(string));
  } else {
    *fromClassNS = 0;
    return GetObjectFromString(interp, string);
  }
}

NSF_INLINE static char *
NSCmdFullName(Tcl_Command cmd) {
  Tcl_Namespace *nsPtr = Tcl_Command_nsPtr(cmd);
  return nsPtr ? nsPtr->fullName : "";
}

static void
NsfCleanupObject_(NsfObject *object) {
  NsfObjectRefCountDecr(object);
  /* fprintf(stderr, "obj refCount of %p after decr %d\n",object,object->refCount);*/

  if (object->refCount <= 0) {
    /*fprintf(stderr, "NsfCleanupObject %p refcount %d\n", object, object->refCount);*/
    assert(object->refCount == 0);
    assert(object->flags & NSF_DELETED);

    MEM_COUNT_FREE("NsfObject/NsfClass", object);

#if defined(NSFOBJ_TRACE)
    fprintf(stderr, "CKFREE Object %p refcount=%d\n", object, object->refCount);
#endif
#if !defined(NDEBUG)
    memset(object, 0, sizeof(NsfObject));
#endif
    ckfree((char *) object);
  }
}


/*
 *  Tcl_Obj functions for objects
 */

static int
IsNsfTclObj(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfObject **objectPtr) {
  Tcl_ObjType CONST86 *cmdType = objPtr->typePtr;
  if (cmdType == Nsf_OT_tclCmdNameType) {
    Tcl_Command cmd = Tcl_GetCommandFromObj(interp, objPtr);
    if (cmd) {
      NsfObject *object = NsfGetObjectFromCmdPtr(cmd);
      if (object) {
        *objectPtr = object;
        return 1;
      }
    }
  }
  return 0;
}

/* Lookup an Next Scripting object from the given objPtr, preferably
 * from an object of type "cmdName". objPtr might be converted in this
 * process.
 */

static int
GetObjectFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfObject **objectPtr) {
  int result;
  NsfObject *nobject;
  CONST char *string;
  Tcl_Command cmd;

  /*fprintf(stderr, "GetObjectFromObj obj %p %s is of type %s\n",
    objPtr, ObjStr(objPtr), objPtr->typePtr ? objPtr->typePtr->name : "(null)");*/

  /* in case, objPtr was not of type cmdName, try to convert */
  cmd = Tcl_GetCommandFromObj(interp, objPtr);
  /*fprintf(stderr, "GetObjectFromObj obj %s => cmd=%p (%d)\n",
    ObjStr(objPtr), cmd, cmd ? Tcl_Command_refCount(cmd):-1);*/
  if (cmd) {
    NsfObject *object = NsfGetObjectFromCmdPtr(cmd);

    /*fprintf(stderr, "GetObjectFromObj obj %s, o is %p objProc %p NsfObjDispatch %p\n", ObjStr(objPtr),
      object, Tcl_Command_objProc(cmd), NsfObjDispatch);*/
    if (object) {
      if (objectPtr) *objectPtr = object;
      return TCL_OK;
    }
  }

  /*fprintf(stderr, "GetObjectFromObj convertFromAny for %s type %p %s\n", ObjStr(objPtr),
    objPtr->typePtr, objPtr->typePtr ? objPtr->typePtr->name : "(none)");*/

  /* In case, we have to revolve via the CallingNameSpace (i.e. the
   * argument is not fully qualified), we retry here.
   */
  string = ObjStr(objPtr);
  if (isAbsolutePath(string)) {
    nobject = NULL;
  } else {
    Tcl_Obj *tmpName = NameInNamespaceObj(interp, string, CallingNameSpace(interp));
    CONST char *nsString = ObjStr(tmpName);

    INCR_REF_COUNT(tmpName);
    nobject = GetObjectFromString(interp, nsString);
    /* fprintf(stderr, " RETRY, string '%s' returned %p\n", nsString, nobject);*/
    DECR_REF_COUNT(tmpName);
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
		     NsfClass **cl, NsfClass *baseClass) {
  NsfObject *object;
  NsfClass *cls = NULL;
  int result = TCL_OK;
  CONST char *objName = ObjStr(objPtr);
  Tcl_Command cmd;

  cmd = Tcl_GetCommandFromObj(interp, objPtr);
  /*fprintf(stderr, "GetClassFromObj %p %s base %p cmd %p\n", objPtr, objName, baseClass, cmd);*/

  if (cmd) {
    cls = NsfGetClassFromCmdPtr(cmd);
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
	nameObj = NameInNamespaceObj(interp, objName, CallingNameSpace(interp));
	objName = ObjStr(nameObj);
	/* adjust path for documented nx.tcl */
      }

      result = Tcl_GetAliasObj(interp, objName,
			       &alias_interp, &alias_cmd_name, &alias_oc, &alias_ov);
      /* we only want aliases with 0 args */
      if (result == TCL_OK && alias_oc == 0) {
	cmd = NSFindCommand(interp, alias_cmd_name);
	/*fprintf(stderr, "..... alias arg 0 '%s' cmd %p\n", alias_cmd_name, cmd);*/
	if (cmd) {
	  cls = NsfGetClassFromCmdPtr(cmd);
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
    cls = NsfObjectToClass(object);
    if (cls) {
      if (cl) *cl = cls;
      return TCL_OK;
    } else {
      /* flag, that we could not convert so far */
      result = TCL_ERROR;
    }
  }

  /*fprintf(stderr, "try __unknown for %s, result so far is %d\n", objName, result);*/
  if (baseClass) {
    Tcl_Obj *methodObj, *nameObj = isAbsolutePath(objName) ? objPtr :
      NameInNamespaceObj(interp, objName, CallingNameSpace(interp));

    INCR_REF_COUNT(nameObj);

    methodObj = NsfMethodObj(interp, &baseClass->object, NSF_c_requireobject_idx);
    if (methodObj) {
      /*fprintf(stderr, "+++ calling __unknown for %s name=%s\n",
	ClassName(baseClass), ObjStr(nameObj));*/

      result = CallMethod((ClientData) baseClass, interp, methodObj,
                          3, &nameObj, NSF_CM_NO_PROTECT|NSF_CSC_IMMEDIATE);
      if (result == TCL_OK) {
        result = GetClassFromObj(interp, objPtr, cl, NULL);
      }
    }
    DECR_REF_COUNT(nameObj);
  }

  return result;
}

/*
 *----------------------------------------------------------------------
 * NameInNamespaceObj --
 *
 *    Create a fully qualified name in the provided namespace or in
 *    the current namespace in form of an Tcl_Obj (with 0 refcount);
 *
 * Results:
 *    Tcl_Obj containing fully qualified name
 *
 * Side effects:
 *    Allocates fresh copies of list elements
 *
 *----------------------------------------------------------------------
 */
static Tcl_Obj *
NameInNamespaceObj(Tcl_Interp *interp, CONST char *name, Tcl_Namespace *nsPtr) {
  Tcl_Obj *objPtr;
  Tcl_DString ds, *dsPtr = &ds;

  /*fprintf(stderr, "NameInNamespaceObj %s (%p, %s) ", name, nsPtr, nsPtr ? nsPtr->fullName:NULL);*/
  if (!nsPtr) {
    nsPtr = Tcl_GetCurrentNamespace(interp);
  }
  /* fprintf(stderr, " (resolved %p, %s) ", nsPtr, nsPtr ? nsPtr->fullName:NULL);*/
  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, nsPtr->fullName, -1);
  
  if (Tcl_DStringLength(dsPtr) > 2) {
    Tcl_DStringAppend(dsPtr, "::", 2);
  }
  Tcl_DStringAppend(dsPtr, name, -1);
  objPtr = Tcl_NewStringObj(Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr));

  /*fprintf(stderr, "returns %s\n", ObjStr(objPtr));*/
  DSTRING_FREE(dsPtr);
  return objPtr;
}

/*
 *----------------------------------------------------------------------
 * NsfReverseClasses --
 *
 *    Reverse class list. Caller is responsible for freeing data.
 *
 * Results:
 *    Pointer to start of the reversed list
 *
 * Side effects:
 *    Allocates fresh copies of list elements
 *
 *----------------------------------------------------------------------
 */
static NsfClasses *
NsfReverseClasses(NsfClasses *sl) {
  NsfClasses *firstPtr = NULL;
  for (; sl; sl = sl->nextPtr) {
    NsfClasses *element = NEW(NsfClasses);
    element->cl = sl->cl;
    element->clientData = sl->clientData;
    element->nextPtr = firstPtr;
    firstPtr = element;
  }
  return firstPtr;
}

/*
 *----------------------------------------------------------------------
 * NsfClassListFree --
 *
 *    Frees all elements of the provided
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Frees memory.
 *
 *----------------------------------------------------------------------
 */
extern void
NsfClassListFree(NsfClasses *sl) {
  NsfClasses *n;
  for (; sl; sl = n) {
    n = sl->nextPtr;
    FREE(NsfClasses, sl);
  }
}

/*
 *----------------------------------------------------------------------
 * NsfClassListAdd --
 *
 *    Add class list entry to the specified list. In case the initial
 *    list is empty, *firstPtrPtr is updated as well.
 *
 * Results:
 *    Returns address of next pointer.
 *
 * Side effects:
 *    New list element is allocated.
 *
 *----------------------------------------------------------------------
 */

extern NsfClasses **
NsfClassListAdd(NsfClasses **firstPtrPtr, NsfClass *cl, ClientData clientData) {
  NsfClasses *l = *firstPtrPtr, *element = NEW(NsfClasses);
  element->cl = cl;
  element->clientData = clientData;
  element->nextPtr = NULL;

  if (l) {
    while (l->nextPtr) l = l->nextPtr;
    l->nextPtr = element;
  } else
    *firstPtrPtr = element;
  return &(element->nextPtr);
}

#if defined(CHECK_ACTIVATION_COUNTS)
/*
 *----------------------------------------------------------------------
 * NsfClassListUnlink --
 *
 *    Return removed item with matching key form nsfClasses.
 *    Key is void to allow not only class pointers as keys.
 *
 * Results:
 *    unlinked element or NULL.
 *    In case the first element is unlinked, *firstPtrPtr 
 *    is updated.
 *
 * Side effects:
 *    none.
 *
 *----------------------------------------------------------------------
 */

static NsfClasses *
NsfClassListUnlink(NsfClasses **firstPtrPtr, void *key) {
  NsfClasses *entryPtr = NULL, *prevPtr = NULL;

  if (*firstPtrPtr != NULL) {
    /* list is non-empty */
    for (entryPtr = *firstPtrPtr; entryPtr; prevPtr = entryPtr, entryPtr = entryPtr->nextPtr) {
      if ((void *)entryPtr->cl == key) {
	/* found entry */
	if (prevPtr) {
	  /* later item */
	  prevPtr->nextPtr = entryPtr->nextPtr;
	} else {
	  /* first item */
	  *firstPtrPtr = entryPtr->nextPtr;
	}
	break;
      }
    }
  }

  return entryPtr;
}
#endif

void
NsfObjectListFree(NsfObjects *sl) {
  NsfObjects *n;
  for (; sl; sl = n) {
    n = sl->nextPtr;
    FREE(NsfObjects, sl);
  }
}

NsfObjects**
NsfObjectListAdd(NsfObjects **cList, NsfObject *object) {
  NsfObjects *l = *cList, *element = NEW(NsfObjects);
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

static NsfClasses *
Super(NsfClass *cl) { return cl->super; }

static NsfClasses *
Sub(NsfClass *cl) { return cl->sub; }

static int
TopoSort(NsfClass *cl, NsfClass *baseClass, NsfClasses *(*next)(NsfClass*)) {
  /*NsfClasses *sl = (*next)(cl);*/
  NsfClasses *sl = next == Super ? cl->super : cl->sub;
  NsfClasses *pl;

  /*
   * careful to reset the color of unreported classes to
   * white in case we unwind with error, and on final exit
   * reset color of reported classes to white
   */

  cl->color = GRAY;
  for (; sl; sl = sl->nextPtr) {
    NsfClass *sc = sl->cl;
    if (sc->color == GRAY) { cl->color = WHITE; return 0; }
    if (sc->color == WHITE && !TopoSort(sc, baseClass, next)) {
      cl->color = WHITE;
      if (cl == baseClass) {
        register NsfClasses *pc;
        for (pc = cl->order; pc; pc = pc->nextPtr) { pc->cl->color = WHITE; }
      }
      return 0;
    }
  }
  cl->color = BLACK;
  pl = NEW(NsfClasses);
  pl->cl = cl;
  pl->nextPtr = baseClass->order;
  baseClass->order = pl;
  if (cl == baseClass) {
    register NsfClasses *pc;
    for (pc = cl->order; pc; pc = pc->nextPtr) { pc->cl->color = WHITE; }
  }
  return 1;
}

static NsfClasses *
TopoOrder(NsfClass *cl, NsfClasses *(*next)(NsfClass*)) {
  if (TopoSort(cl, cl, next))
    return cl->order;
  NsfClassListFree(cl->order);
  return cl->order = NULL;
}

static NsfClasses *
ComputeOrder(NsfClass *cl, NsfClasses *order, NsfClasses *(*direction)(NsfClass*)) {
  if (order)
    return order;
  return cl->order = TopoOrder(cl, direction);
}

extern NsfClasses *
NsfComputePrecedence(NsfClass *cl) {
  return ComputeOrder(cl, cl->order, Super);
}

extern NsfClasses *
NsfComputeDependents(NsfClass *cl) {
  return ComputeOrder(cl, cl->order, Sub);
}


static void
FlushPrecedencesOnSubclasses(NsfClass *cl) {
  NsfClasses *pc;
  NsfClassListFree(cl->order);
  cl->order = NULL;
  pc = ComputeOrder(cl, cl->order, Sub);

  /*
   * ordering doesn't matter here - we're just using toposort
   * to find all lower classes so we can flush their caches
   */

  if (pc) pc = pc->nextPtr;
  for (; pc; pc = pc->nextPtr) {
    NsfClassListFree(pc->cl->order);
    pc->cl->order = NULL;
  }
  NsfClassListFree(cl->order);
  cl->order = NULL;
}

static void
AddInstance(NsfObject *object, NsfClass *cl) {
  object->cl = cl;
  if (cl) {
    int nw;
    (void) Tcl_CreateHashEntry(&cl->instances, (char *)object, &nw);
  }
}

static int
RemoveInstance(NsfObject *object, NsfClass *cl) {
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
AS(NsfClass *cl, NsfClass *s, NsfClasses **sl) {
  register NsfClasses *l = *sl;
  while (l && l->cl != s) l = l->nextPtr;
  if (!l) {
    NsfClasses *sc = NEW(NsfClasses);
    sc->cl = s;
    sc->nextPtr = *sl;
    *sl = sc;
  }
}

static void
AddSuper(NsfClass *cl, NsfClass *super) {
  if (cl && super) {
    /*
     * keep corresponding sub in step with super
     */
    AS(cl, super, &cl->super);
    AS(super, cl, &super->sub);
  }
}

static int
RemoveSuper1(NsfClass *cl, NsfClass *s, NsfClasses **sl) {
  NsfClasses *l = *sl;
  if (!l) return 0;
  if (l->cl == s) {
    *sl = l->nextPtr;
    FREE(NsfClasses, l);
    return 1;
  }
  while (l->nextPtr && l->nextPtr->cl != s) l = l->nextPtr;
  if (l->nextPtr) {
    NsfClasses *n = l->nextPtr->nextPtr;
    FREE(NsfClasses, l->nextPtr);
    l->nextPtr = n;
    return 1;
  }
  return 0;
}

static int
RemoveSuper(NsfClass *cl, NsfClass *super) {
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

extern Nsf_Class *
NsfIsClass(Tcl_Interp *interp, ClientData clientData) {
  if (clientData && NsfObjectIsClass((NsfObject *)clientData))
    return (Nsf_Class *) clientData;
  return 0;
}

/*
 * methods lookup
 */


/*
 *----------------------------------------------------------------------
 * GetEnsembeObjectFromName --
 *
 *    Get an ensemble object from a method name.  If the method name
 *    is fully qualified, just use a Tcl lookup, otherwise get it from
 *    the provided namespace,
 *
 * Results:
 *    ensemble object or NULL
 *
 * Side effects:
 *    none
 *
 *----------------------------------------------------------------------
 */
static NsfObject *
GetEnsembeObjectFromName(Tcl_Interp *interp, Tcl_Namespace *nsPtr, Tcl_Obj *name,
			 Tcl_Command *cmdPtr, int *fromClassNS) {
  Tcl_Command cmd;
  char *nameString = ObjStr(name);

  if (*nameString == ':') {
    cmd = Tcl_GetCommandFromObj(interp, name);
    *fromClassNS = IsClassNsName(nameString);
  } else {
    cmd = nsPtr ? FindMethod(nsPtr, nameString) : NULL;
  }

  if (cmd) {
    *cmdPtr = cmd;
    return NsfGetObjectFromCmdPtr(cmd);
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * GetRegObject --
 *
 *    Try to get the object, on which the method was registered from a
 *    fully qualified method handle
 *
 * Results:
 *    NsfObject * or NULL on failure
 *
 * Side effects:
 *    none
 *
 *----------------------------------------------------------------------
 */
static NsfObject *
GetRegObject(Tcl_Interp *interp, Tcl_Command cmd, CONST char *methodName,
	     CONST char **methodName1, int *fromClassNS) {
  NsfObject *regObject = NULL;

  if (cmd && *methodName == ':') {
    CONST char *procName = Tcl_GetCommandName(interp, cmd);
    size_t objNameLength = strlen(methodName) - strlen(procName) - 2;

    if (objNameLength > 0) {
      Tcl_DString ds, *dsPtr = &ds;
      /* obtain parent name */
      Tcl_DStringInit(dsPtr);
      Tcl_DStringAppend(dsPtr, methodName, objNameLength);
      regObject = GetObjectFromNsName(interp, Tcl_DStringValue(dsPtr), fromClassNS);
      if (regObject) {
	*methodName1 = procName;
      }
      Tcl_DStringFree(dsPtr);
    }
  }

  /*fprintf(stderr, "GetRegObject cmd %p methodName '%s' => %p\n", cmd, methodName, regObject);*/
  return regObject;
}

/*
 *----------------------------------------------------------------------
 * ResolveMethodName --
 *
 *    Resolve a method name relative to a provided namespace.
 *    The method name can be
 *      a) a fully qualified name
 *      b) a list of method name and subcommands
 *      c) a simple name
 *
 * Results:
 *    Tcl_Command or NULL on failure
 *
 * Side effects:
 *    none
 *
 *----------------------------------------------------------------------
 */
static Tcl_Command
ResolveMethodName(Tcl_Interp *interp, Tcl_Namespace *nsPtr, Tcl_Obj *methodObj,
		  Tcl_DString *methodNameDs,
		  NsfObject **regObject,
		  NsfObject **defObject,
		  CONST char **methodName1, int *fromClassNS) {
  Tcl_Command cmd;
  NsfObject *referencedObject;
  char* methodName = ObjStr(methodObj);

  if (strchr(methodName, ' ') > 0) {
    Tcl_Namespace *parentNsPtr;
    NsfObject *ensembleObject;
    Tcl_Obj *methodHandleObj;
    int oc, result, i;
    Tcl_Obj **ov;

    /*fprintf(stderr, "name '%s' contains space \n", methodName);*/

    if ((result = Tcl_ListObjGetElements(interp, methodObj, &oc, &ov) != TCL_OK)
	|| ((referencedObject = GetEnsembeObjectFromName(interp, nsPtr, ov[0],
							 &cmd, fromClassNS)) == NULL)
	) {
      *methodName1 = NULL;
      *regObject = NULL;
      *defObject = NULL;
      return NULL;
    }

    /*
     * We have an ensemble object. First, figure out, on which
     * object/class the ensemble object was registered. We determine
     * the regObject on the first element of the list. If we can't,
     * then the current object is the regObject.
     */
    *regObject = GetRegObject(interp, cmd, ObjStr(ov[0]), methodName1, fromClassNS);

    /*fprintf(stderr, "... regObject object '%s' reg %p, fromClassNS %d\n",
      ObjectName(referencedObject), *regObject, *fromClassNS);*/

    /*
     * Build a fresh methodHandleObj to held method name and names of
     * subcmds.
     */
    methodHandleObj = Tcl_DuplicateObj(referencedObject->cmdName);
    Tcl_DStringAppend(methodNameDs, Tcl_GetCommandName(interp, cmd), -1);
    parentNsPtr = NULL;

    /*
     * Iterate over the objects and append to the handle and methodObj
     */
    for (i = 1; i < oc; i++) {
      cmd = Tcl_GetCommandFromObj(interp, methodHandleObj);
      ensembleObject = cmd ? NsfGetObjectFromCmdPtr(cmd) : NULL;

      if (!ensembleObject) {
	DECR_REF_COUNT(methodHandleObj);
	*methodName1 = NULL;
	*regObject = NULL;
	*defObject = NULL;
	return NULL;
      }

      if (parentNsPtr && Tcl_Command_nsPtr(ensembleObject->id) != parentNsPtr) {
	/* fprintf(stderr, "*** parent change saved parent %p %s computed parent %p %s\n",
		parentNsPtr, parentNsPtr->fullName,
		Tcl_Command_nsPtr(ensembleObject->id),
		Tcl_Command_nsPtr(ensembleObject->id)->fullName);*/
	DECR_REF_COUNT(methodHandleObj);
	methodHandleObj = Tcl_DuplicateObj(ensembleObject->cmdName);
      }
      parentNsPtr = ensembleObject->nsPtr;

      Tcl_AppendLimitedToObj(methodHandleObj, "::", 2, INT_MAX, NULL);
      Tcl_AppendLimitedToObj(methodHandleObj, ObjStr(ov[i]), -1, INT_MAX, NULL);
      Tcl_DStringAppendElement(methodNameDs, ObjStr(ov[i]));
    }

    /*
     * cmd contains now the parent-obj, on which the method was
     * defined. Get from this cmd the defObj.
     */
    *defObject = NsfGetObjectFromCmdPtr(cmd);

    /*fprintf(stderr, "... handle '%s' last cmd %p defObject %p\n",
      ObjStr(methodHandleObj), cmd, *defObject);*/

    /*
     * Obtain the command from the method handle and report back the
     * final methodName,
     */
    cmd = Tcl_GetCommandFromObj(interp, methodHandleObj);
    *methodName1 = Tcl_DStringValue(methodNameDs);

    /*fprintf(stderr, "... methodname1 '%s' cmd %p\n", *methodName1, cmd);*/
    DECR_REF_COUNT(methodHandleObj);

  } else if (*methodName == ':') {
    cmd = Tcl_GetCommandFromObj(interp, methodObj);
    referencedObject = GetRegObject(interp, cmd, methodName, methodName1, fromClassNS);
    *regObject = referencedObject;
    *defObject = referencedObject;
    *methodName1 = Tcl_GetCommandName(interp, cmd);
    if (referencedObject == NULL) {
      /* 
       * The cmd was not registered on an object or class, but we
       * still report back the cmd (might be e.g. a primitive cmd).
       */
    }
  } else {
    *methodName1 = methodName;
    cmd = nsPtr ? FindMethod(nsPtr, methodName) : NULL;
    *regObject = NULL;
    *defObject = NULL;
  }

  return cmd;
}

/*
 *----------------------------------------------------------------------
 * CmdIsProc --
 *
 *    Check, whether the cmd is interpreted
 *
 * Results:
 *    Boolean
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static int
CmdIsProc(Tcl_Command cmd) {
  /* In 8.6: TclIsProc((Command*)cmd) is not equiv to the definition below */
  return (Tcl_Command_objProc(cmd) == TclObjInterpProc);
}

/*
 *----------------------------------------------------------------------
 * GetTclProcFromCommand --
 *
 *    Check if cmd is interpreted, and if so, return the proc
 *    definition.
 *
 * Results:
 *    The found proc of cmd or NULL.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */
static Proc *
GetTclProcFromCommand(Tcl_Command cmd) {
  Tcl_ObjCmdProc *proc;

  assert(cmd);
  proc = Tcl_Command_objProc(cmd);
  if (proc == TclObjInterpProc) {
    return (Proc*) Tcl_Command_objClientData(cmd);
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * FindMethod --
 *
 *    Lookup the cmd for methodName in a namespace.
 *
 * Results:
 *    The found cmd of the method or NULL.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

NSF_INLINE static Tcl_Command
FindMethod(Tcl_Namespace *nsPtr, CONST char *methodName) {
  register Tcl_HashEntry *entryPtr;
  if ((entryPtr = Tcl_CreateHashEntry(Tcl_Namespace_cmdTablePtr(nsPtr), methodName, NULL))) {
    return (Tcl_Command) Tcl_GetHashValue(entryPtr);
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * FindProcMethod --
 *
 *    Lookup the proc for methodName in a namespace.
 *
 * Results:
 *    The found proc of the method or NULL.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static Proc *
FindProcMethod(Tcl_Namespace *nsPtr, CONST char *methodName) {
  Tcl_Command cmd = FindMethod(nsPtr, methodName);
  return cmd ? GetTclProcFromCommand(cmd) : NULL;
}

/*
 *----------------------------------------------------------------------
 * SearchPLMethod --
 *
 *    Search a method along a provided class list.
 *    The methodName must be simple (must not contain
 *    space).
 *
 * Results:
 *    The found class defining the method or NULL.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */
static NsfClass *
SearchPLMethod(register NsfClasses *pl, CONST char *methodName, Tcl_Command *cmdPtr) {
  /* Search the precedence list (class hierarchy) */
#if 1
  for (; pl;  pl = pl->nextPtr) {
    register Tcl_HashEntry *entryPtr =
      Tcl_CreateHashEntry(Tcl_Namespace_cmdTablePtr(pl->cl->nsPtr), methodName, NULL);
    if (entryPtr) {
      *cmdPtr = (Tcl_Command) Tcl_GetHashValue(entryPtr);
      return pl->cl;
    }
  }
#else
  for (; pl;  pl = pl->nextPtr) {
    if ((*cmdPtr = FindMethod(pl->cl->nsPtr, methodName))) {
      return pl->cl;
    }
  }
#endif
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * SearchCMethod --
 *
 *    Search a method along the superclass hierarchy of the provided
 *    class. The methodObj must be simple (must not contain
 *    space). The method has the interface for internal calls during
 *    interpretation, while SearchSimpleCMethod() has the interface
 *    with more overhead for introspection.
 *
 * Results:
 *    The found class defining the method or NULL.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */
static NsfClass *
SearchCMethod(/*@notnull@*/ NsfClass *cl, CONST char *methodName, Tcl_Command *cmdPtr) {
  assert(cl);
  return SearchPLMethod(ComputeOrder(cl, cl->order, Super), methodName, cmdPtr);
}

/*
 *----------------------------------------------------------------------
 * SearchSimpleCMethod --
 *
 *    Search a method along the superclass hierarchy of the provided
 *    class. The methodObj must be simple (must not contain
 *    space). The method has the same interface as
 *    SearchComplexCMethod().
 *
 * Results:
 *    The found class defining the method or NULL.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */
static NsfClass *
SearchSimpleCMethod(Tcl_Interp *interp, /*@notnull@*/ NsfClass *cl,
		    Tcl_Obj *methodObj, Tcl_Command *cmdPtr) {
  assert(cl);
  return SearchPLMethod(ComputeOrder(cl, cl->order, Super), ObjStr(methodObj), cmdPtr);
}

/*
 *----------------------------------------------------------------------
 * SearchComplexCMethod --
 *
 *    Search a method along the superclass hierarchy of the provided
 *    class. The methodObj can refer to an ensemble object (can
 *    contain space). The method has the same interface as
 *    SearchSimpleCMethod().
 *
 * Results:
 *    The found class defining the method or NULL.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */
static NsfClass *
SearchComplexCMethod(Tcl_Interp *interp, /*@notnull@*/ NsfClass *cl,
		     Tcl_Obj *methodObj, Tcl_Command *cmdPtr) {
  NsfClasses *pl;
  CONST char *methodName1 = NULL;
  Tcl_DString ds, *dsPtr = &ds;
  int fromClassNS = 1;

  assert(cl);

  for (pl = ComputeOrder(cl, cl->order, Super); pl;  pl = pl->nextPtr) {
    NsfObject *regObject, *defObject;
    Tcl_Command cmd;

    Tcl_DStringInit(dsPtr);
    cmd = ResolveMethodName(interp, pl->cl->nsPtr, methodObj,
			    dsPtr, &regObject, &defObject, &methodName1, &fromClassNS);
    Tcl_DStringFree(dsPtr);

    if (cmd) {
      *cmdPtr = cmd;
      return pl->cl;
    }
  }

  return NULL;
}

/*
 *----------------------------------------------------------------------
 * ObjectFindMethod --
 *
 *    Find a method for a given object in the precedence path. The
 *    provided methodObj might be an ensemble object. This function
 *    tries to optimize access by calling different implementations
 *    for simple and ensemble method names.
 *
 * Results:
 *    Tcl command.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Command
ObjectFindMethod(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *methodObj, NsfClass **pcl) {
  Tcl_Command cmd = NULL;
  Tcl_DString ds, *dsPtr = &ds;
  int containsSpace = strchr(ObjStr(methodObj), ' ') > 0;
  NsfClass *(*lookupFunction)(Tcl_Interp *interp, NsfClass *cl,
			      Tcl_Obj *methodObj, Tcl_Command *cmdPtr) =
    containsSpace ? SearchComplexCMethod : SearchSimpleCMethod;

  if (!(object->flags & NSF_MIXIN_ORDER_VALID)) {
    MixinComputeDefined(interp, object);
  }

  if (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) {
    NsfCmdList *mixinList;
    for (mixinList = object->mixinOrder; mixinList; mixinList = mixinList->nextPtr) {
      NsfClass *mixin = NsfGetClassFromCmdPtr(mixinList->cmdPtr);
      if (mixin && (*pcl = (*lookupFunction)(interp, mixin, methodObj, &cmd))) {
	if (Tcl_Command_flags(cmd) & NSF_CMD_CLASS_ONLY_METHOD && !NsfObjectIsClass(object)) {
	  cmd = NULL;
	  continue;
	}
        break;
      }
    }
  }

  if (!cmd && object->nsPtr) {
    int fromClassNS = 0;
    NsfObject *regObject, *defObject;
    CONST char *methodName1 = NULL;

    Tcl_DStringInit(dsPtr);
    cmd = ResolveMethodName(interp, object->nsPtr, methodObj,
			    dsPtr, &regObject, &defObject, &methodName1, &fromClassNS);
    Tcl_DStringFree(dsPtr);
  }

  if (!cmd && object->cl) {
    *pcl = (*lookupFunction)(interp, object->cl, methodObj, &cmd);
  }

  return cmd;
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
static NsfObjectSystem *
GetObjectSystem(NsfObject *object) {
  assert(object);
  if (NsfObjectIsClass(object)) {
    return ((NsfClass *)object)->osPtr;
  }
  return object->cl->osPtr;
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
ObjectSystemFree(Tcl_Interp *interp, NsfObjectSystem *osPtr) {
  int i;

  for (i=0; i<=NSF_o_unknown_idx; i++) {
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
    RemoveInstance((NsfObject*)osPtr->rootMetaClass, osPtr->rootMetaClass);
    RemoveInstance((NsfObject*)osPtr->rootClass, osPtr->rootMetaClass);

    FinalObjectDeletion(interp, &osPtr->rootClass->object);
    FinalObjectDeletion(interp, &osPtr->rootMetaClass->object);
  }

  FREE(NsfObjectSystem *, osPtr);
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
ObjectSystemAdd(Tcl_Interp *interp, NsfObjectSystem *osPtr) {
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
ObjectSystemsCheckSystemMethod(Tcl_Interp *interp, CONST char *methodName, NsfObject *object) {
  NsfObjectSystem *osPtr, *defOsPtr = GetObjectSystem(object);
  int i;

  for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = osPtr->nextPtr) {
    for (i=0; i<=NSF_o_unknown_idx; i++) {
      Tcl_Obj *methodObj = osPtr->methods[i];
      if (methodObj && !strcmp(methodName, ObjStr(methodObj))) {
        int flag = 1<<i;
        if (osPtr->definedMethods & flag) {
	  /* 
	   *  If for some reason (e.g. reload) we redefine the base
	   *  methods, these never count as overloads.
	   */
	  if ((*(Nsf_SytemMethodOpts[i]+1) == 'o' && object == &defOsPtr->rootClass->object)
	      || (*(Nsf_SytemMethodOpts[i]+1) == 'c' && object == &defOsPtr->rootMetaClass->object) ) {
	    /*fprintf(stderr, "+++ %s %.6x NOT overloading %s.%s %s (is root %d, is meta %d)\n", 
		    ClassName(defOsPtr->rootClass), 
		    osPtr->overloadedMethods, ObjectName(object), methodName, Nsf_SytemMethodOpts[i], 
		    object == &defOsPtr->rootClass->object, 
		    object == &defOsPtr->rootMetaClass->object);*/
	  } else {
	    osPtr->overloadedMethods |= flag;
	    /*fprintf(stderr, "+++ %s %.6x overloading %s.%s %s (is root %d, is meta %d)\n", 
		    ClassName(defOsPtr->rootClass),
		    osPtr->overloadedMethods, ObjectName(object), methodName, Nsf_SytemMethodOpts[i], 
		    object == &defOsPtr->rootClass->object, 
		    object == &defOsPtr->rootMetaClass->object);*/
	  }
        }
        if (osPtr == defOsPtr && ((osPtr->definedMethods & flag) == 0)) {
          osPtr->definedMethods |= flag;
          /*fprintf(stderr, "+++ %s %.6x defining %s.%s %s\n", ClassName(defOsPtr->rootClass),
	    osPtr->definedMethods, ObjectName(object), methodName, Nsf_SytemMethodOpts[i]);*/
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
 *    is to be called when an Next Scripting process or thread exists.
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
  NsfObjectSystem *osPtr, *nPtr;

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
    /*fprintf(stderr, "destroyObjectSystem deletes %s\n", ClassName(osPtr->rootClass));*/
    GetAllInstances(interp, commandNameTable, osPtr->rootClass);
  }

  /***** SOFT DESTROY *****/
  RUNTIME_STATE(interp)->exitHandlerDestroyRound = NSF_EXITHANDLER_ON_SOFT_DESTROY;
  /*fprintf(stderr, "===CALL destroy on OBJECTS\n");*/

  for (hPtr = Tcl_FirstHashEntry(commandNameTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(commandNameTable, hPtr);
    NsfObject *object = GetObjectFromString(interp, key);
    /* fprintf(stderr, "key = %s %p %d\n",
       key, obj, obj && !NsfObjectIsClass(object)); */
    if (object && !NsfObjectIsClass(object)
        && !(object->flags & NSF_DESTROY_CALLED)) {

      DispatchDestroyMethod(interp, object, 0);
    }
  }

  /*fprintf(stderr, "===CALL destroy on CLASSES\n");*/

  for (hPtr = Tcl_FirstHashEntry(commandNameTable, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(commandNameTable, hPtr);
    NsfClass *cl = GetClassFromString(interp, key);
    if (cl && !(cl->object.flags & NSF_DESTROY_CALLED)) {
      DispatchDestroyMethod(interp, (NsfObject *)cl, 0);
    }
  }

  /* now, turn of filters, all destroy callbacks are done */
  RUNTIME_STATE(interp)->doFilters = 0;

#ifdef DO_CLEANUP
  FreeAllNsfObjectsAndClasses(interp, commandNameTable);

# ifdef DO_FULL_CLEANUP
  DeleteProcsAndVars(interp);
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
static int
CallDirectly(Tcl_Interp *interp, NsfObject *object, int methodIdx, Tcl_Obj **methodObjPtr) {
  /*
     We can/must call a C-implemented method directly, when
     a) the object system has no such appropriate method defined

     b) the script does not contain a method with the appropriate
        name, and

     c) filters are not active on the object
  */
  NsfObjectSystem *osPtr = GetObjectSystem(object);
  int callDirectly = 1;
  Tcl_Obj *methodObj;

  methodObj = osPtr->methods[methodIdx];
  if (methodObj) {
    int flag = 1 << methodIdx;
    if ((osPtr->overloadedMethods & flag) != 0) {
      /* overloaded, we must dispatch */
      /*fprintf(stderr, "overloaded\n");*/
      callDirectly = 0;
    } else if ((osPtr->definedMethods & flag) == 0) {
      /* not defined, we must call directly */
      fprintf(stderr, "Warning: CallDirectly object %s idx %s not defined\n",
	      ObjectName(object), Nsf_SytemMethodOpts[methodIdx]+1);
    } else {
      if (!(object->flags & NSF_FILTER_ORDER_VALID)) {
        FilterComputeDefined(interp, object);
      }
      /*fprintf(stderr, "CallDirectly object %s idx %s object flags %.6x %.6x \n",
        ObjectName(object), sytemMethodOpts[methodIdx]+1,
        (object->flags & NSF_FILTER_ORDER_DEFINED_AND_VALID),
        NSF_FILTER_ORDER_DEFINED_AND_VALID
        );*/
      if ((object->flags & NSF_FILTER_ORDER_DEFINED_AND_VALID) == NSF_FILTER_ORDER_DEFINED_AND_VALID) {
        /*fprintf(stderr, "CallDirectly object %s idx %s has filter \n",
          ObjectName(object), sytemMethodOpts[methodIdx]+1);*/
        callDirectly = 0;
      }
    }
  }

#if 0
  fprintf(stderr, "CallDirectly object %s idx %d returns %s => %d\n",
          ObjectName(object), methodIdx,
          methodObj ? ObjStr(methodObj) : "(null)", callDirectly);
#endif
  /* return the methodObj in every case */
  *methodObjPtr = methodObj;
  return callDirectly;
}

/*
 *----------------------------------------------------------------------
 * NsfMethodObj --
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
Tcl_Obj *
NsfMethodObj(Tcl_Interp *interp, NsfObject *object, int methodIdx) {
  NsfObjectSystem *osPtr = GetObjectSystem(object);
  /*
  fprintf(stderr, "NsfMethodObj object %s os %p idx %d %s methodObj %p\n",
          ObjectName(object), osPtr, methodIdx,
          Nsf_SytemMethodOpts[methodIdx]+1,
          osPtr->methods[methodIdx]);
  */
  return osPtr->methods[methodIdx];
}


/*
 * conditional memory allocations of optional storage
 */

extern NsfObjectOpt *
NsfRequireObjectOpt(NsfObject *object) {
  if (!object->opt) {
    object->opt = NEW(NsfObjectOpt);
    memset(object->opt, 0, sizeof(NsfObjectOpt));
  }
  return object->opt;
}

extern NsfClassOpt *
NsfRequireClassOpt(/*@notnull@*/ NsfClass *cl) {
  assert(cl);
  if (!cl->opt) {
    cl->opt = NEW(NsfClassOpt);
    memset(cl->opt, 0, sizeof(NsfClassOpt));
    if (cl->object.flags & NSF_IS_CLASS) {
      cl->opt->id = cl->object.id;  /* probably a temporary solution */
    }
  }
  return cl->opt;
}


static void
MakeObjNamespace(Tcl_Interp *interp, NsfObject *object) {
#ifdef NAMESPACE_TRACE
  fprintf(stderr, "+++ MakeObjNamespace for %s\n", ObjectName(object));
#endif
  if (!object->nsPtr) {
    Tcl_Namespace *nsPtr;
    object->nsPtr = NSGetFreshNamespace(interp, (ClientData)object,
                                        ObjectName(object));
    if (!object->nsPtr)
      Tcl_Panic("MakeObjNamespace: Unable to make namespace", NULL);
    nsPtr = object->nsPtr;

    /*
     * Copy all obj variables to the newly created namespace
     */
    if (object->varTablePtr) {
      Tcl_HashSearch  search;
      Tcl_HashEntry   *hPtr;
      TclVarHashTable *varTablePtr = Tcl_Namespace_varTablePtr(nsPtr);
      Tcl_HashTable   *varHashTablePtr = VarHashTablePtr(varTablePtr);
      Tcl_HashTable   *objHashTablePtr = VarHashTablePtr(object->varTablePtr);
	
      *varHashTablePtr = *objHashTablePtr; /* copy the table */
	
      if (objHashTablePtr->buckets == objHashTablePtr->staticBuckets) {
        varHashTablePtr->buckets = varHashTablePtr->staticBuckets;
      }
      for (hPtr = Tcl_FirstHashEntry(varHashTablePtr, &search);  hPtr;
           hPtr = Tcl_NextHashEntry(&search)) {
        hPtr->tablePtr = varHashTablePtr;
      }
      CallStackReplaceVarTableReferences(interp, object->varTablePtr,
                                         (TclVarHashTable *)varHashTablePtr);

      ckfree((char *) object->varTablePtr);
      object->varTablePtr = NULL;
    }
  }
}

static Tcl_Var
CompiledLocalsLookup(CallFrame *varFramePtr, CONST char *varName) {
  int i, localCt = varFramePtr->numCompiledLocals;
  Tcl_Obj **objPtrPtr = &varFramePtr->localCachePtr->varName0;

  /* fprintf(stderr, ".. search #local vars %d for %s\n", localCt, varName);*/
  for (i=0 ; i<localCt ; i++, objPtrPtr++) {
    register Tcl_Obj *objPtr = *objPtrPtr;
    if (objPtr) {
      char *localName = TclGetString(objPtr);
      if ((varName[0] == localName[0])
	  && (varName[1] == localName[1])
          && (strcmp(varName, localName) == 0)) {
        return (Tcl_Var) &varFramePtr->compiledLocals[i];
      }
    }
  }
  return NULL;
}

/*********************************************************
 *
 * Variable resolvers
 *
 *********************************************************/
#define FOR_COLON_RESOLVER(ptr) (*(ptr) == ':' && *(ptr+1) != ':')

/*
 *----------------------------------------------------------------------
 * MethodName --
 *
 *    Return the methodName from a Tcl_Obj, strips potentially the
 *    colon prefix
 *
 * Results:
 *    method name
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static CONST char *
MethodName(Tcl_Obj *methodObj) {
  char *methodName = ObjStr(methodObj);

  if (FOR_COLON_RESOLVER(methodName)) {
    methodName ++;
  }
  return methodName;
}

/*
 *----------------------------------------------------------------------
 * NsColonVarResolver --
 *
 *    Namespace resolver for namespace specific variable lookup.
 *    colon prefix
 *
 * Results:
 *    method name
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static int
NsColonVarResolver(Tcl_Interp *interp, CONST char *varName, Tcl_Namespace *nsPtr, 
		   int flags, Tcl_Var *varPtr) {
  Tcl_CallFrame *varFramePtr;
  TclVarHashTable *varTablePtr;
  NsfObject *object;
  int new, frameFlags;
  Tcl_Obj *key;
  Var *newVar;

#if defined (VAR_RESOLVER_TRACE)
  fprintf(stderr, "NsColonVarResolver '%s' flags %.6x\n", varName, flags);
#endif

  /* 
   * Case 1: The variable is to be resolved in global scope, proceed in
   * resolver chain
   */
  if (flags & TCL_GLOBAL_ONLY) {
    /*fprintf(stderr, "global-scoped lookup for var '%s' in NS '%s'\n", varName,
      nsPtr->fullName);*/
    return TCL_CONTINUE;
  }

  /* 
   * Case 2: The lookup happens in a proc frame (lookup in compiled
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

  /* 
   * FRAME_IS_NSF_CMETHOD has always FRAME_IS_PROC set, so it is
   * handeled already above
   */
  assert((frameFlags & FRAME_IS_NSF_CMETHOD) == 0);

  if ((frameFlags & FRAME_IS_NSF_OBJECT) == 0) {
    /*
     * Case 3: we are not in an Next Scripting frame, so proceed as well
     */
    return TCL_CONTINUE;

  } else {
    /*
     *  Case 4: we are in an Next Scripting object frame
     */

    if (*varName == ':') {
      if (*(varName+1) != ':') {
        /*
         * Case 4a: The variable name starts with a single ":". Skip
         * the char, but stay in the resolver.
         */
        varName ++;
      } else {
        /*
	 * Case 4b: Names starting  with "::" are not for us
	 */
        return TCL_CONTINUE;
      }
    } else if (NSTail(varName) != varName) {
      /*
       * Case 4c: Names containing "::" are not for us
       */
      return TCL_CONTINUE;
    }

    /* 
     * Since we know that we are here always in an object frame, we
     * can blindly get the object from the client data .
     */
    object = (NsfObject *)Tcl_CallFrame_clientData(varFramePtr);
  }

  /* 
   * We have an object and create the variable if not found 
   */
  assert(object);

  varTablePtr = object->nsPtr ? Tcl_Namespace_varTablePtr(object->nsPtr) : object->varTablePtr;
  assert(varTablePtr);

  /*
   * Does the variable exist in the object's namespace?
   */
  key = Tcl_NewStringObj(varName, -1);
  INCR_REF_COUNT(key);

  *varPtr = (Tcl_Var)VarHashCreateVar(varTablePtr, key, NULL);

#if defined (VAR_RESOLVER_TRACE)
  fprintf(stderr, "...... lookup of '%s' for object '%s' returns %p\n",
          varName, ObjectName(object), *varPtr);
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

typedef struct nsfResolvedVarInfo {
  Tcl_ResolvedVarInfo vInfo;        /* This must be the first element. */
  NsfObject *lastObject;
  Tcl_Var var;
  Tcl_Obj *nameObj;
} nsfResolvedVarInfo;

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
 *    This function is the actual variable resolution handler for a
 *    colon-prefixed (":/varName/") found in a compiled script
 *    registered by the compiling var resolver (see
 *    InterpCompiledColonResolver()). When initialising a call frame,
 *    this handler is called, crawls the object's var table (creating
 *    a variable, if needed), and returns a Var structure. Based on
 *    this, a link variable ":/varName/" pointing to this object
 *    variable (i.e., "varName") is created and is stored in the
 *    compiled locals array of the call frame. Beware that these link
 *    variables interact with the family of link-creating commands
 *    ([variable], [global], [upvar]) by being subject to
 *    "retargeting" upon name conflicts (see
 *    tests/varresolutiontest.tcl for some examples).
 *
 * Results:
 *    Tcl_Var containing value or NULL.
 *
 * Side effects:
 *    Updates of Variable structure cache in necessary.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Var
CompiledColonVarFetch(Tcl_Interp *interp, Tcl_ResolvedVarInfo *vinfoPtr) {
  nsfResolvedVarInfo *resVarInfo = (nsfResolvedVarInfo *)vinfoPtr;
  NsfCallStackContent *cscPtr = CallStackGetTopFrame(interp, NULL);
  NsfObject *object = cscPtr ? cscPtr->self : NULL;
  TclVarHashTable *varTablePtr;
  Tcl_Var var = resVarInfo->var;
  int new;

#if defined(VAR_RESOLVER_TRACE)
  int flags = var ? ((Var*)var)->flags : 0;
  fprintf(stderr,"CompiledColonVarFetch var '%s' var %p flags = %.4x dead? %.4x\n",
	  ObjStr(resVarInfo->nameObj), var, flags, flags&VAR_DEAD_HASH);
#endif

  /*
   * We cache lookups based on nsf objects; we have to care about
   * cases, where the instance variables are in some delete states.
   *
   */

  if (var && object == resVarInfo->lastObject && 
      (((((Var*)var)->flags) & VAR_DEAD_HASH)) == 0) {
    /*
     * The variable is valid.
     */
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, ".... cached var '%s' var %p flags = %.4x\n",
            ObjStr(resVarInfo->nameObj), var, ((Var*)var)->flags);
#endif
    return var;
  }

  if (!object) {
    return NULL;
  }

  if (var) {
    /*
     * The variable is not valid anymore. Clean it up.
     */
    HashVarFree(var);
  }

  varTablePtr = object->nsPtr ? Tcl_Namespace_varTablePtr(object->nsPtr) : object->varTablePtr;
  assert(varTablePtr);

  resVarInfo->lastObject = object;
#if defined(VAR_RESOLVER_TRACE)
  fprintf(stderr,"Fetch var %s in object %s\n",TclGetString(resVarInfo->nameObj),ObjectName(object));
#endif
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
    fprintf(stderr, ".... looked up existing var %s var %p flags = %.6x undefined %d\n",
	    ObjStr(resVarInfo->nameObj),
	    v, v->flags,
	    TclIsVarUndefined(v));
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
static void
CompiledColonVarFree(Tcl_ResolvedVarInfo *vInfoPtr) {
  nsfResolvedVarInfo *resVarInfo = (nsfResolvedVarInfo *)vInfoPtr;

  DECR_REF_COUNT(resVarInfo->nameObj);
  if (resVarInfo->var) {HashVarFree(resVarInfo->var);}
  ckfree((char *) vInfoPtr);
}

/*
 *----------------------------------------------------------------------
 * InterpCompiledColonVarResolver --
 *
 *    For colon-prefixed (":/varName/") variables, we provide our own
 *    var resolver for compiling scripts and evaluating compiled
 *    scripts (e.g., proc bodies). At the time of first compilation
 *    (or re-compilation), this resolver is processed (see
 *    tclProc.c:InitResolvedLocals()). It registers two handlers for a
 *    given, colon-prefixed variable found in the script: the actual
 *    variable fetcher and a variable cleanup handler. The variable
 *    fetcher is executed whenever a Tcl call frame is intialised and
 *    the array of compiled locals is constructed (see also
 *    InitResolvedLocals()).
 *
 *    The Tcl var resolver protocol dictates that per-namespace
 *    compiling var resolvers take precedence over this per-interp
 *    compiling var resolver. That is, per-namespace resolvers are
 *    processed first and can effectively outrule per-interp
 *    resolvers by signalling TCL_OK or TCL_BREAK.
 *
 * Results:
 *    TCL_OK or TCL_CONTINUE (according to Tcl's var resolver protocol)
 *
 * Side effects:
 *    Registers per-variable resolution and cleanup handlers.
 *
 *----------------------------------------------------------------------
 */
static int
InterpCompiledColonVarResolver(Tcl_Interp *interp,
			CONST84 char *name, int length, Tcl_Namespace *context,
			Tcl_ResolvedVarInfo **rPtr) {
  /*
   *  The variable handler is registered, when we have an active Next Scripting
   *  object and the variable starts with the appropriate prefix. Note
   *  that getting the "self" object is a weak protection against
   *  handling of wrong vars
   */
  NsfObject *object = GetSelfObj(interp);

#if defined(VAR_RESOLVER_TRACE)
  fprintf(stderr, "compiled var resolver for %s, obj %p\n", name, object);
#endif

  if (object && FOR_COLON_RESOLVER(name)) {
    nsfResolvedVarInfo *vInfoPtr = (nsfResolvedVarInfo *) ckalloc(sizeof(nsfResolvedVarInfo));

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
 *    For accessing object (instance) variables using the colon-prefix
 *    notation (":/varName/"), we provide our own var resolvers. This
 *    function is the non-compiling var resolver; its services are
 *    requested in two situations: a) when evaluating non-compiled
 *    statements, b) when executing slow-path bytecode instructions,
 *    with "slow path" referring to bytecode instructions not making
 *    use of the compiled locals array (and, e.g., reverting to
 *    TclObjLookupVar*() calls).
 *
 *    The Tcl var resolver protocol dictates that per-namespace,
 *    non-compiling var resolvers take precedence over this per-interp
 *    non-compiling var resolver. That is, per-namespace resolvers are
 *    processed first and can effectively outrule per-interp resolvers
 *    by signalling TCL_OK or TCL_BREAK. See
 *    e.g. TclLookupSimpleVar().
 *
 * Results:
 *    TCL_OK or TCL_CONTINUE (according to on Tcl's var resolver protocol)
 *
 * Side effects:
 *    If successful, return varPtr, pointing to instance variable.
 *
 *----------------------------------------------------------------------
 */
static int
InterpColonVarResolver(Tcl_Interp *interp, CONST char *varName, Tcl_Namespace *nsPtr, 
		       int flags, Tcl_Var *varPtr) {
  int new, frameFlags;
  CallFrame *varFramePtr;
  TclVarHashTable *varTablePtr;
  NsfObject *object;
  Tcl_Obj *keyObj;
  Tcl_Var var;

  if (!FOR_COLON_RESOLVER(varName) || (flags & (TCL_GLOBAL_ONLY|TCL_NAMESPACE_ONLY))) {
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

  if (frameFlags & FRAME_IS_NSF_METHOD) {
    if ((*varPtr = CompiledLocalsLookup(varFramePtr, varName))) {
	/*
	 * This section is reached under notable circumstances and
	 * represents a point of interaction between our resolvers for
	 * non-compiled (i.e., InterpColonVarResolver()) and compiled script
	 * execution (i.e., InterpCompiledColonVarResolver()).
	 *
	 * Expect this branch to be hit iff...
	 *
	 * 1. ... InterpCompiledColonVarResolver() is called from within
	 * the Tcl bytecode interpreter when executing a
	 * bytecode-compiled script on a *slow path* (i.e., involving
	 * a TclObjLookupVarEx() call)
	 *
	 * 2. ... the act of variable resolution (i.e.,
	 * TclObjLookupVarEx()) has not been restricted to the global
	 * (TCL_GLOBAL_ONLY) or an effective namespace
	 * (TCL_NAMESPACE_ONLY)
	 *
	 * 3. ..., resulting from the fact of participating in an
	 * bytecode interpretation, CompiledColonVarFetch() stored a
	 * link variable (pointing to the actual/real object variable,
	 * whether defined or not) under the given varName value into
	 * the current call frame's array of compiled locals (when
	 * initialising the call frame; see
	 * tclProc.c:InitResolvedLocals()).
	 */
#if defined(VAR_RESOLVER_TRACE)
      fprintf(stderr, ".... found local %s varPtr %p flags %.6x\n", 
	      varName, *varPtr, flags);
#endif
      /*
       * By looking up the compiled-local directly and signalling
       * TCL_OK, we optimise a little by avoiding further lookups down
       * the Tcl var resolution infrastructure. Note that signalling
       * TCL_CONTINUE would work too, however, it would involve extra
       * resolution overhead.
       */
      return TCL_OK;
    }

    object = ((NsfCallStackContent *)varFramePtr->clientData)->self;

  } else if (frameFlags & FRAME_IS_NSF_CMETHOD) {
    object = ((NsfCallStackContent *)varFramePtr->clientData)->self;

  } else if (frameFlags & FRAME_IS_NSF_OBJECT) {
    object = (NsfObject *)(varFramePtr->clientData);

  } else {
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, ".... not found %s\n", varName);
#endif
    return TCL_CONTINUE;
  }
  
  /*
   * Trim the varName for the colon prefix (":").
   */
  varName ++;
    
  /* 
   * We have an object and create the variable if not found 
   */
  assert(object);
  if (object->nsPtr) {
    varTablePtr = Tcl_Namespace_varTablePtr(object->nsPtr);
  } else if (object->varTablePtr) {
    varTablePtr = object->varTablePtr;
  } else {
    /*
     * In most situations, we have a varTablePtr through the clauses
     * above. However, if someone redefines e.g. the method
     * "configure" or "objectparameter", we might find an object with
     * an still empty varTable, since these are lazy initiated.
     */
    varTablePtr = object->varTablePtr = VarHashTableCreate();
  }
  assert(varTablePtr);

  /*fprintf(stderr, "Object Var Resolver, name=%s, obj %p, nsPtr %p, varTablePtr %p\n",
    varName, object, object->nsPtr, varTablePtr);*/

  keyObj = Tcl_NewStringObj(varName, -1);
  INCR_REF_COUNT(keyObj);

  var = (Tcl_Var)VarHashCreateVar(varTablePtr, keyObj, NULL);
  if (var) {
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, ".... found in hashtable %s %p flags %.6x ns %p\n", 
	    varName, var, ((Var *)var)->flags,  object->nsPtr);
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
 *    Resolve command names. If the command starts with the Next
 *    Scripting specific prefix and we are on an Next Scripting stack
 *    frame, treat command as OO method.
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
InterpColonCmdResolver(Tcl_Interp *interp, CONST char *cmdName, Tcl_Namespace *nsPtr,
		       int flags, Tcl_Command *cmdPtr) {
  CallFrame *varFramePtr;
  int frameFlags;

  /*fprintf(stderr, "InterpColonCmdResolver %s flags %.6x\n", cmdName, flags);*/

  if ((*cmdName == ':' && *(cmdName + 1) == ':') || flags & TCL_GLOBAL_ONLY) {
    /* fully qualified names and global lookups are not for us */
    return TCL_CONTINUE;
  }

  varFramePtr = Tcl_Interp_varFramePtr(interp);
  frameFlags = Tcl_CallFrame_isProcCallFrame(varFramePtr);

  /*fprintf(stderr, "InterpColonCmdResolver frame cmdName %s flags %.6x, frame flags %.6x lambda %d\n", 
    cmdName, flags, frameFlags, frameFlags & FRAME_IS_LAMBDA);*/

  /*
   * If the resolver is called from a lambda frame, use always the parent frame
   */
  if ((frameFlags & FRAME_IS_LAMBDA)) {
    varFramePtr = (CallFrame *)Tcl_CallFrame_callerPtr(varFramePtr);
    frameFlags = Tcl_CallFrame_isProcCallFrame(varFramePtr);
  }

  /* 
   * The resolver is called as well, when a body of a method is
   * compiled.  In these situations, Tcl stacks a nonproc frame, that
   * we have to skip. In order to safely identify such situations, we
   * stuff into the call flags of the proc frame during the
   * compilation step NSF_CSC_CALL_IS_COMPILE.
   */
  if (frameFlags == 0 && Tcl_CallFrame_callerPtr(varFramePtr)) {
    varFramePtr = (CallFrame *)Tcl_CallFrame_callerPtr(varFramePtr);
    frameFlags = Tcl_CallFrame_isProcCallFrame(varFramePtr);

    if ((frameFlags & (FRAME_IS_NSF_METHOD)) == 0
	|| (((NsfCallStackContent *)varFramePtr->clientData)->flags & NSF_CSC_CALL_IS_COMPILE) == 0
	) {
      frameFlags = 0;
    } else {
#if defined(CMD_RESOLVER_TRACE)
      fprintf(stderr, "InterpColonCmdResolver got parent frame cmdName %s flags %.6x, frame flags %.6x\n", 
	      cmdName, flags, Tcl_CallFrame_isProcCallFrame(varFramePtr));
#endif
    }
 }

#if defined(CMD_RESOLVER_TRACE)
  fprintf(stderr, "InterpColonCmdResolver cmdName %s flags %.6x, frame flags %.6x\n", 
	  cmdName, flags, Tcl_CallFrame_isProcCallFrame(varFramePtr));
#endif

  if (frameFlags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_OBJECT|FRAME_IS_NSF_CMETHOD )) {
    if (*cmdName == ':') {
#if defined(CMD_RESOLVER_TRACE)
      fprintf(stderr, "    ... call colonCmd for %s\n", cmdName);
#endif
      /*
       * We have a cmd starting with ':', we are in an nsf frame, so
       * forward to the colonCmd.
       */
      *cmdPtr = RUNTIME_STATE(interp)->colonCmd;
      return TCL_OK;
    } else {
      //xxxxx
#if 1
      /*
       * Experimental Object-System specific resolver: If an
       * unprefixed method name is found in a body of a method, we try
       * to perform a lookup for this method in the namespace of the
       * object system for the current object. If this lookup is not
       * successful the standard lookups are performed. The
       * object-system specific resolver allows to use the "right"
       * (unprefixed) "self" or "next" calls without namespace
       * imports.
       */ 
      NsfObject *object;
      NsfObjectSystem *osPtr;
      Tcl_Command cmd;
      Tcl_HashTable *cmdTablePtr;
      Tcl_HashEntry *entryPtr;

      if (frameFlags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
	object = ((NsfCallStackContent *)varFramePtr->clientData)->self;
      } else if (frameFlags & (FRAME_IS_NSF_OBJECT)) {
	object = (NsfObject *)(varFramePtr->clientData);
      } else {
	object = NULL;
      }
      if (object) {
	//xxx
	osPtr = GetObjectSystem(object);
	cmd = osPtr->rootClass->object.id;
	cmdTablePtr = Tcl_Namespace_cmdTablePtr(((Command *)cmd)->nsPtr);
	entryPtr = Tcl_CreateHashEntry(cmdTablePtr, cmdName, NULL);
	/* fprintf(stderr, "InterpColonCmdResolver OS specific resolver tried to lookup %s for os %s in ns %s\n", 
	   cmdName, ClassName(osPtr->rootClass), ((Command *)cmd)->nsPtr->fullName);*/
	if (entryPtr) {
	  /*fprintf(stderr, "InterpColonCmdResolver OS specific resolver found %s::%s\n",
	    ((Command *)cmd)->nsPtr->fullName, cmdName);*/
	  *cmdPtr = Tcl_GetHashValue(entryPtr);
	  return TCL_OK;
	}
      }
#endif
    }
  }

#if defined(CMD_RESOLVER_TRACE)
  fprintf(stderr, "    ... not found %s\n", cmdName);
  NsfShowStack(interp);
#endif
  return TCL_CONTINUE;
}
/*********************************************************
 *
 * End of cmd resolver
 *
 *********************************************************/

/*
 *----------------------------------------------------------------------
 * NsfNamespaceInit --
 *
 *    Initialize a provided namespace by setting its resolvers and
 *    namespace path
 *
 * Results:
 *    none
 *
 * Side effects:
 *    change ns behavior
 *
 *----------------------------------------------------------------------
 */

static void
NsfNamespaceInit(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {

  assert(nsPtr);
  /* 
   * This puts a per-object namespace resolver into position upon
   * acquiring the namespace. Works for object-scoped commands/procs
   * and object-only ones (set, unset, ...)
   */
  Tcl_SetNamespaceResolvers(nsPtr, /*(Tcl_ResolveCmdProc*)NsColonCmdResolver*/ NULL,
                            NsColonVarResolver,
                            /*(Tcl_ResolveCompiledVarProc*)NsCompiledColonVarResolver*/NULL);
#if defined(INHERIT_NAMESPACES_TO_CHILD_OBJECTS)
  /* 
   * In case there is a namespace path set for the parent namespace,
   * apply this as well to the object namespace to avoid surprises
   * with "namespace path nx".
   */
  { Namespace *parentNsPtr = Tcl_Namespace_parentPtr(nsPtr);
    int i, pathLength = Tcl_Namespace_commandPathLength(parentNsPtr);

    if (pathLength>0) {
      Namespace **pathArray = (Namespace **)ckalloc(sizeof(Namespace *) * pathLength);
      NamespacePathEntry *tmpPathArray = Tcl_Namespace_commandPathArray(parentNsPtr);
      
      for (i=0; i<pathLength; i++) {
	pathArray[i] = tmpPathArray[i].nsPtr;
      }
      TclSetNsPath((Namespace *)nsPtr, pathLength, (Tcl_Namespace **)pathArray);
      ckfree((char*)pathArray);
    }
  }
#endif
}

/*
 *----------------------------------------------------------------------
 * RequireObjNamespace --
 *
 *    Obtain for an object a namespace if necessary and initialize it.
 *    In this function, variables existing outside of the namespace
 *    get copied over to thew fresh namespace.
 *
 * Results:
 *    Tcl_Namespace
 *
 * Side effects:
 *    Allocate pot. a namespace
 *
 *----------------------------------------------------------------------
 */

static Tcl_Namespace *
RequireObjNamespace(Tcl_Interp *interp, NsfObject *object) {

  if (!object->nsPtr) {
    MakeObjNamespace(interp, object);
    NsfNamespaceInit(interp, object->nsPtr);
  }
  assert(object->nsPtr);

  return object->nsPtr;
}

extern void
NsfRequireObjNamespace(Tcl_Interp *interp, Nsf_Object *object) {
  RequireObjNamespace(interp, (NsfObject*) object);
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


/*
 *----------------------------------------------------------------------
 * NSDeleteChild --
 *
 *    Delete a child of an object in cases, when the parent object is
 *    deleted. It is designed to delete either objects or classes to
 *    be a little bit more graceful on destuctors. Not perfect yet.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Might destroy an object.
 *
 *----------------------------------------------------------------------
 */
static void
NSDeleteChild(Tcl_Interp *interp, Tcl_Command cmd, int deleteObjectsOnly) {

  /*fprintf(stderr, "NSDeleteChildren child %p (%s) epoch %d\n", 
    cmd, Tcl_GetCommandName(interp, cmd), Tcl_Command_cmdEpoch(cmd));*/

  assert(Tcl_Command_cmdEpoch(cmd) == 0);

  if (!Tcl_Command_cmdEpoch(cmd)) {
    NsfObject *object = NsfGetObjectFromCmdPtr(cmd);

    if (object == NULL) {
      /* 
       * This is just a plain Tcl command; let Tcl handle the
       * deletion.
       */
      return;
    }

    /*fprintf(stderr, "NSDeleteChild check %p %s true child %d\n",  
      object, ObjectName(object), object->id == cmd);*/
    
    /* delete here just true children */
    if (object->id == cmd) {

      if (deleteObjectsOnly && NsfObjectIsClass(object)) {
	return;
      }

      /*fprintf(stderr, "NSDeleteChild destroy %p %s\n", object, ObjectName(object));*/

      /* in the exit handler physical destroy --> directly call destroy */
      if (RUNTIME_STATE(interp)->exitHandlerDestroyRound
	  == NSF_EXITHANDLER_ON_PHYSICAL_DESTROY) {
	PrimitiveDestroy((ClientData) object);
      } else {
	if (object->teardown && !(object->flags & NSF_DESTROY_CALLED)) {
	  int result = DispatchDestroyMethod(interp, object, 0);

	  if (result != TCL_OK) {
	    /*
	     * The destroy method failed. However, we have to remove
	     * the command anyway, since its parent is currently being
	     * deleted.
	     */
	    NsfLog(interp, NSF_LOG_NOTICE, "Destroy failed for object %s, perform low level deletion",
		   ObjectName(object));

	    if (object->teardown) {
	      CallStackDestroyObject(interp, object);
	    }
	  }
	}
      }
    } else {
      /*fprintf(stderr, "NSDeleteChild remove alias %p %s\n", object, Tcl_GetCommandName(interp, cmd));*/
      AliasDeleteObjectReference(interp, cmd);
    }
  } 
}

/*
 *----------------------------------------------------------------------
 * NSDeleteChildren --
 *
 *    Delete the child objects of a namespace.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Might destroy child objects.
 *
 *----------------------------------------------------------------------
 */

static void
NSDeleteChildren(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(nsPtr);
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;


#ifdef OBJDELETION_TRACE
  fprintf(stderr, "NSDeleteChildren %p %s\n", nsPtr, nsPtr->fullName);
#endif

  /* 
   * First, get rid of namespace imported objects; don't delete the
   * object, but the reference.
   */
  Tcl_ForgetImport(interp, nsPtr, "*"); /* don't destroy namespace imported objects */
  
  /*
   * Second, delete the objects.
   */
  for (hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    NSDeleteChild(interp, (Tcl_Command)Tcl_GetHashValue(hPtr), 1);
  }
 /*
  * Finally, delete the classes.
   */
  for (hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    NSDeleteChild(interp, (Tcl_Command)Tcl_GetHashValue(hPtr), 0);
  }
}

/*
 * ensure that a variable exists on object varTablePtr or nsPtr->varTablePtr,
 * if necessary create it. Return Var* if successful, otherwise 0
 */
static Var *
NSRequireVariableOnObj(Tcl_Interp *interp, NsfObject *object, CONST char *name, int flgs) {
  CallFrame frame, *framePtr = &frame;
  Var *varPtr, *arrayPtr;

  Nsf_PushFrameObj(interp, object, framePtr);
  varPtr = TclLookupVar(interp, name, 0, flgs, "obj vwait",
                        /*createPart1*/ 1, /*createPart2*/ 0, &arrayPtr);
  Nsf_PopFrameObj(interp, framePtr);
  return varPtr;
}

static int
Nsf_DeleteCommandFromToken(Tcl_Interp *interp, Tcl_Command cmd) {
  CallStackClearCmdReferences(interp, cmd);
  return Tcl_DeleteCommandFromToken(interp, cmd);
}

/*
 * delete all vars & procs in a namespace
 */
static void
NSCleanupNamespace(Tcl_Interp *interp, Tcl_Namespace *ns) {
  TclVarHashTable *varTablePtr = Tcl_Namespace_varTablePtr(ns);
  Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(ns);
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "NSCleanupNamespace %p\n", ns);
  fprintf(stderr, "NSCleanupNamespace %p %.6x varTablePtr %p\n", ns, ((Namespace *)ns)->flags, varTablePtr);
#endif
  /*
   * Delete all variables and initialize var table again
   * (DeleteVars frees the vartable)
   */
  TclDeleteVars((Interp *)interp, varTablePtr);
  TclInitVarHashTable(varTablePtr, (Namespace *)ns);

  /*
   * Delete all user-defined procs in the namespace
   */
  for (hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
      Tcl_Command cmd = (Tcl_Command) Tcl_GetHashValue(hPtr);
      Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);

      if (proc == NsfObjDispatch) {
	/*
	 * Sub-objects should not be deleted here to preseve children
	 * deletion order. Just delete aliases.
	 */
	AliasDeleteObjectReference(interp, cmd);
	continue;
      }
      /* fprintf(stderr, "NSCleanupNamespace calls DeleteCommandFromToken for %p flags %.6x invokeObj %p obj %p\n",
                cmd, ((Command *)cmd)->flags, invokeObj,object);
        fprintf(stderr, "    cmd = %s\n", Tcl_GetCommandName(interp,cmd));
        fprintf(stderr, "    nsPtr = %p\n", ((Command *)cmd)->nsPtr);
        fprintf(stderr, "    flags %.6x\n", ((Namespace *)((Command *)cmd)->nsPtr)->flags);*/

      Nsf_DeleteCommandFromToken(interp, cmd);
  }
}


static void
NSNamespaceDeleteProc(ClientData clientData) {
  /* dummy for ns identification by pointer comparison */
  NsfObject *object = (NsfObject*) clientData;
  /*fprintf(stderr, "namespacedeleteproc obj=%p ns=%p\n",
    clientData,object ? object->nsPtr : NULL);*/
  if (object) {
    object->nsPtr = NULL;
  }
}

void
Nsf_DeleteNamespace(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  int activationCount = 0;
  Tcl_CallFrame *f = (Tcl_CallFrame *)Tcl_Interp_framePtr(interp);

  /*fprintf(stderr, "Nsf_DeleteNamespace %p ", nsPtr);*/

  while (f) {
    if (f->nsPtr == nsPtr)
      activationCount++;
    f = Tcl_CallFrame_callerPtr(f);
  }

#if !defined(NDEBUG)
  if (Tcl_Namespace_activationCount(nsPtr) != activationCount) {
    fprintf(stderr, "WE HAVE TO FIX ACTIVATIONCOUNT\n");
    Tcl_Namespace_activationCount(nsPtr) = activationCount;
  }
#endif
  assert(Tcl_Namespace_activationCount(nsPtr) == activationCount);

  /*fprintf(stderr, "to %d. \n", ((Namespace *)nsPtr)->activationCount);*/

  MEM_COUNT_FREE("TclNamespace", nsPtr);
  if (Tcl_Namespace_deleteProc(nsPtr)) {
    /*fprintf(stderr, "calling deteteNamespace %s\n", nsPtr->fullName);*/
    Tcl_DeleteNamespace(nsPtr);
  }
}

/*
 *----------------------------------------------------------------------
 * NSCheckColons --
 *
 *    Check the provided colons in an object name. If the name is
 *    valid, the function returns 1, otherwise 0.
 *
 * Results:
 *    returns 1 on success
 *
 * Side effects:
 *    none
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static int
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
 *----------------------------------------------------------------------
 * NSGetFreshNamespace --
 *
 *    Create an object namespace, provide a deleteProc (avoid
 *    interference between object and namespace deletion order) and
 *    keep the object as client data.
 *
 * Results:
 *    Tcl_Namespace
 *
 * Side effects:
 *    might allocate a namespace
 *
 *----------------------------------------------------------------------
 */
static Tcl_Namespace*
NSGetFreshNamespace(Tcl_Interp *interp, ClientData clientData, CONST char *name) {
  Tcl_Namespace *nsPtr;
  Namespace *dummy1Ptr, *dummy2Ptr;
  const char *dummy;

  TclGetNamespaceForQualName(interp, name, NULL, TCL_FIND_ONLY_NS|TCL_CREATE_NS_IF_UNKNOWN, 
			     (Namespace **)&nsPtr,
			     &dummy1Ptr, &dummy2Ptr, &dummy);

  if (nsPtr->deleteProc != NSNamespaceDeleteProc) {
    /* reuse the namespace */
    if (nsPtr->deleteProc || nsPtr->clientData) {
      Tcl_Panic("Namespace '%s' exists already with delProc %p and clientData %p; "
		"Can only convert a plain Tcl namespace into an nsf namespace, my delete Proc %p",
		name, nsPtr->deleteProc, nsPtr->clientData, NSNamespaceDeleteProc);
    }
    nsPtr->clientData = clientData;
    nsPtr->deleteProc = (Tcl_NamespaceDeleteProc *)NSNamespaceDeleteProc;
  }

  MEM_COUNT_ALLOC("TclNamespace", nsPtr);
  return nsPtr;
}

/*
 *----------------------------------------------------------------------
 * NSRequireParentObject --
 *
 *    Try to require a parent object (e.g. during ttrace).  This function tries
 *    to load a parent object via __unknown, in case such a method is defined.
 *
 * Results:
 *    returns 1 on success
 *
 * Side effects:
 *    might create an object
 *
 *----------------------------------------------------------------------
 */
static int
NSRequireParentObject(Tcl_Interp *interp, CONST char *parentName, NsfClass *cl) {
  NsfClass *defaultSuperClass = DefaultSuperClass(interp, cl, cl->object.cl, 1);
  Tcl_Obj *methodObj = NsfMethodObj(interp, &defaultSuperClass->object, NSF_c_requireobject_idx);
  int rc = 0;
  
  /*fprintf(stderr, "NSRequireParentObject %s cl %p (%s) methodObj %p defaultSc %p %s\n",
    parentName, cl, ClassName(cl), methodObj, defaultSuperClass, ClassName(defaultSuperClass));*/

  if (methodObj) {
    /* call requireObject and try again */
    Tcl_Obj *ov[3];
    int result;

    ov[0] = defaultSuperClass->object.cmdName;
    ov[1] = methodObj;
    ov[2] = Tcl_NewStringObj(parentName, -1);
    INCR_REF_COUNT(ov[2]);

    /*fprintf(stderr, "+++ parent... calling %s __unknown for %s\n", 
      ClassName(defaultSuperClass), ObjStr(ov[2]));*/

    result = Tcl_EvalObjv(interp, 3, ov, 0);
    if (result == TCL_OK) {
      NsfObject *parentObj = (NsfObject*) GetObjectFromString(interp, parentName);
      if (parentObj) {
	RequireObjNamespace(interp, parentObj);
      }
      rc = (Tcl_FindNamespace(interp, parentName,
			      (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY) != NULL);
    }
    DECR_REF_COUNT(ov[2]);
  }
  
  return rc;
}

/*
 *----------------------------------------------------------------------
 * NSCheckNamespace --
 *
 *    Check if a namespace with the given name exists. If not, make
 *    sure that a potential parent object has already required a
 *    namespace. If there is no parent namespace yet, try to create a
 *    parent object via __unknown. 

 *    If the provided parentNsPtr is not NULL, we know, that (a) the
 *    provided name was relative and simple (contains no ":"
 *    characters) and that (b) this namepace was used to build a fully
 *    qualified name. In theses cases, the parentNsPtr points already
 *    to the parentName, containing potentially a parent Object. In
 *    all other cases, the parent name is either obtained from the
 *    full namespace, or from string operations working on the
 *    provided name.
 *
 * Results:
 *    Tcl_Namespace for the provided name
 *
 * Side effects:
 *    might create parent objects
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static Tcl_Namespace *
NSCheckNamespace(Tcl_Interp *interp, CONST char *nameString, Tcl_Namespace *parentNsPtr, NsfClass *cl) {
  Tcl_Namespace *nsPtr, *dummy1Ptr, dummy2Ptr;
  CONST char *parentName, *dummy, *n;
  Tcl_DString ds, *dsPtr = &ds;
  int parentNameLength;

  /*fprintf(stderr, "NSCheckNamespace %s parentNsPtr %p\n", nameString, parentNsPtr);*/

  /* 
   * Check, if there is a already a namespace for the full name. The
   * namespace will be seldomly here, but we have to make this check
   * in every case. If there is a full namespace, we can use it to
   * determine the parent name.
   */
  TclGetNamespaceForQualName(interp, nameString, NULL, 
			     TCL_GLOBAL_ONLY|TCL_FIND_ONLY_NS, 
			     (Namespace **)&nsPtr,
			     (Namespace **)&dummy1Ptr, (Namespace **)&dummy1Ptr, &dummy);
  /*fprintf(stderr, 
	  "beforecreate calls TclGetNamespaceForQualName with %s => %p (%s) %p %s %p %s %p %s\n", 
	  nameString, nsPtr, nsPtr ? nsPtr->fullName : "",
	  dummy1Ptr,  dummy1Ptr ? dummy1Ptr->fullName : "",
	  parentNsPtr,  parentNsPtr ? parentNsPtr->fullName : "",
	  dummy, dummy ? dummy : "");*/

  /*
   * If there is a parentNs provided (or obtained from the full
   * namespace), we can determine the parent name from it. Otherwise,
   * whe have to to perform the string operations.
   */

  if (parentNsPtr == NULL && nsPtr) {
    parentNsPtr = (Tcl_Namespace *)Tcl_Namespace_parentPtr(nsPtr);
  }

  if (parentNsPtr) {
    parentNameLength = 0;
    parentName = parentNsPtr->fullName;
    if (*(parentName + 2) == '\0') {
      parentName = NULL;
    }
    /*fprintf(stderr, "NSCheckNamespace parentNs %s parentName of '%s' => '%s'\n", 
      parentNsPtr->fullName, nameString, parentName);*/
  } else {
    n = nameString + strlen(nameString);
    /*search for last '::'*/
    while ((*n != ':' || *(n-1) != ':') && n-1 > nameString) {n--; }
    if (*n == ':' && n > nameString && *(n-1) == ':') {n--;}
    parentNameLength = n-nameString;
    if (parentNameLength > 0) {
      DSTRING_INIT(dsPtr);
      Tcl_DStringAppend(dsPtr, nameString, parentNameLength);
      parentName = Tcl_DStringValue(dsPtr);
    } else {
      parentName = NULL;
    }
  }

  if (parentName) {
    NsfObject *parentObj;
    parentObj = (NsfObject*) GetObjectFromString(interp, parentName);
    /*fprintf(stderr, "parentName %s parentObj %p\n", parentName, parentObj);*/

    if (parentObj) {
      RequireObjNamespace(interp, parentObj);
    } else if (nsPtr == NULL && parentNsPtr == NULL) {
      TclGetNamespaceForQualName(interp, parentName, NULL,
				 TCL_GLOBAL_ONLY|TCL_FIND_ONLY_NS, 
				 (Namespace **)&parentNsPtr, (Namespace **)&dummy1Ptr, 
				 (Namespace **)&dummy2Ptr, &dummy);
      if (parentNsPtr == NULL) {
	/*fprintf(stderr, "===== calling NSRequireParentObject %s %p\n", parentName, cl);*/
	NSRequireParentObject(interp, parentName, cl);
      }
    }

    if (parentNameLength) {
      DSTRING_FREE(dsPtr);
    }
  }

  return nsPtr;
}


/*
 * Find the "real" command belonging eg. to an Next Scripting class or object.
 * Do not return cmds produced by Tcl_Import, but the "real" cmd
 * to which they point.
 */
NSF_INLINE static Tcl_Command
NSFindCommand(Tcl_Interp *interp, CONST char *name) {
  Tcl_Command cmd;

  assert(name);
  assert(*name == ':' && *(name + 1) == ':');

  cmd = Tcl_FindCommand(interp, name, NULL, TCL_GLOBAL_ONLY);
  if (cmd) {
    Tcl_Command importedCmd;
    if ((importedCmd = TclGetOriginalCommand(cmd)))
      cmd = importedCmd;
  }
  return cmd;
}

/*
 * C interface routines for manipulating objects and classes
 */

extern Nsf_Object*
NsfGetObject(Tcl_Interp *interp, CONST char *name) {
  return (Nsf_Object*) GetObjectFromString(interp, name);
}

/*
 * Find an object using a char *name
 */
static NsfObject *
GetObjectFromString(Tcl_Interp *interp, CONST char *name) {
  register Tcl_Command cmd;
  assert(name);
  /*fprintf(stderr, "GetObjectFromString name = '%s'\n", name);*/

  cmd = NSFindCommand(interp, name);

  /*if (cmd) {
    fprintf(stderr, "+++ NsfGetObject from %s -> objProc=%p, dispatch=%p OK %d\n",
            name, Tcl_Command_objProc(cmd), NsfObjDispatch, Tcl_Command_objProc(cmd) == NsfObjDispatch);
            }*/

  if (cmd && Tcl_Command_objProc(cmd) == NsfObjDispatch) {
    /*fprintf(stderr, "GetObjectFromString cd %p\n", Tcl_Command_objClientData(cmd));*/
    return (NsfObject*)Tcl_Command_objClientData(cmd);
  }
  return 0;
}

/*
 * Find a class using a char *name
 */

extern Nsf_Class *
NsfGetClass(Tcl_Interp *interp, CONST char *name) {
  return (Nsf_Class *)GetClassFromString(interp, name);
}

static NsfClass *
GetClassFromString(Tcl_Interp *interp, CONST char *name) {
  NsfObject *object = GetObjectFromString(interp, name);
  return (object && NsfObjectIsClass(object)) ? (NsfClass*)object : NULL;
}

static int
CanRedefineCmd(Tcl_Interp *interp, Tcl_Namespace *nsPtr, NsfObject *object, CONST char *methodName) {
  int result, ok;
  Tcl_Command cmd = FindMethod(nsPtr, methodName);

  ok = cmd ? (Tcl_Command_flags(cmd) & NSF_CMD_REDEFINE_PROTECTED_METHOD) == 0 : 1;
  if (ok) {
    result = TCL_OK;
  } else {
    /* 
     * We could test, whether we are bootstrapping the "right" object
     * system, and allow only overwrites for the current bootstrap
     * object system, but this seems neccessary by now.
     */
    Tcl_Obj *bootstrapObj = Tcl_GetVar2Ex(interp, "::nsf::bootstrap", NULL, TCL_GLOBAL_ONLY);
    if (bootstrapObj == NULL) {
      result = NsfPrintError(interp, "Method '%s' of %s cannot be overwritten. "
			     "Derive e.g. a sub-class!", methodName, ObjectName(object));
    } else {
      result = TCL_OK;
    }
  }

  if (result == TCL_OK) {
    ObjectSystemsCheckSystemMethod(interp, methodName, object);
  }
  return result;
}

int
NsfAddObjectMethod(Tcl_Interp *interp, Nsf_Object *object1, CONST char *methodName,
                     Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp,
                     int flags) {
  NsfObject *object = (NsfObject *)object1;
  Tcl_DString newCmdName, *dsPtr = &newCmdName;
  Tcl_Namespace *ns = RequireObjNamespace(interp, object);
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
NsfAddClassMethod(Tcl_Interp *interp, Nsf_Class *class, CONST char *methodName,
                       Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp,
                       int flags) {
  NsfClass *cl = (NsfClass *)class;
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
TclObjListFreeList(NsfTclObjList *list) {
  NsfTclObjList *del;
  while (list) {
    del = list;
    list = list->nextPtr;
    DECR_REF_COUNT(del->content);
    FREE(NsfTclObjList, del);
  }
}

static Tcl_Obj *
TclObjListNewElement(NsfTclObjList **list, Tcl_Obj *ov) {
  NsfTclObjList *elt = NEW(NsfTclObjList);
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
AutonameIncr(Tcl_Interp *interp, Tcl_Obj *nameObj, NsfObject *object,
             int instanceOpt, int resetOpt) {
  int valueLength, mustCopy = 1, format = 0;
  char *valueString, *c;
  Tcl_Obj *valueObj, *result = NULL, *savedResult = NULL;
  int flgs = TCL_LEAVE_ERR_MSG;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, object, framePtr);
  if (object->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  valueObj = Tcl_ObjGetVar2(interp, NsfGlobalObjs[NSF_AUTONAMES], nameObj, flgs);
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
  Tcl_ObjSetVar2(interp, NsfGlobalObjs[NSF_AUTONAMES], nameObj,
		 valueObj, flgs);

  if (resetOpt) {
    if (valueObj) { /* we have an entry */
      Tcl_UnsetVar2(interp, NsfGlobalStrings[NSF_AUTONAMES], ObjStr(nameObj), flgs);
    }
    result = NsfGlobalObjs[NSF_EMPTY];
    INCR_REF_COUNT(result);
  } else {
    if (valueObj == NULL) {
      valueObj = Tcl_ObjSetVar2(interp, NsfGlobalObjs[NSF_AUTONAMES],
                                   nameObj, NsfGlobalObjs[NSF_ONE], flgs);
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
      if (NsfCallCommand(interp, NSF_FORMAT, 3, ov) != TCL_OK) {
        Nsf_PopFrameObj(interp, framePtr);
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

  Nsf_PopFrameObj(interp, framePtr);
  assert((resetOpt && result->refCount>=1) || (result->refCount == 1));
  return result;
}

/*
 * Next Scripting CallStack
 */

NSF_INLINE static void
CallStackDoDestroy(Tcl_Interp *interp, NsfObject *object) {
  Tcl_Command oid;

  /* fprintf(stderr, "CallStackDoDestroy %p flags %.6x\n", object, object->flags);*/
  PRINTOBJ("CallStackDoDestroy", object);

  /* Don't do anything, if a recursive DURING_DELETE is for some
   * reason active.
   */
  if (object->flags & NSF_DURING_DELETE) {
    return;
  }
  /*fprintf(stderr, "CallStackDoDestroy %p flags %.6x activation %d rc %d cmd %p \n",
    object, object->flags, object->activationCount, object->refCount, object->id);*/

  object->flags |= NSF_DURING_DELETE;
  oid = object->id;
  /* oid might be freed already, we can't even use (((Command*)oid)->flags & CMD_IS_DELETED) */

  if (object->teardown && oid) {

    /* PrimitiveDestroy() has to be before DeleteCommandFromToken(),
       otherwise e.g. unset traces on this object cannot be executed
       from Tcl. We make sure via refcounting that the object structure
       is kept until after DeleteCommandFromToken().
    */
    object->refCount ++;
    /*fprintf(stderr, "obj refCount of %p after incr %d (CallStackDoDestroy) dodestroy\n",
      object,object->refCount);*/

    /*fprintf(stderr, "CallStackDoDestroy %p after refCount ++ %d teardown %p\n",
      object, object->refCount, object->teardown);*/

    PrimitiveDestroy((ClientData) object);

    if (!(object->flags & NSF_TCL_DELETE)) {
      Tcl_Obj *savedObjResult = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(savedObjResult);
      /*fprintf(stderr, "    before DeleteCommandFromToken %p object flags %.6x\n", oid, object->flags);*/
      /*fprintf(stderr, "cmd dealloc %p refcount %d dodestroy \n", oid, Tcl_Command_refCount(oid));*/
      Tcl_DeleteCommandFromToken(interp, oid); /* this can change the result */
      /*fprintf(stderr, "    after DeleteCommandFromToken %p %.6x\n", oid, ((Command*)oid)->flags);*/
      Tcl_SetObjResult(interp, savedObjResult);
      DECR_REF_COUNT(savedObjResult);
    }
    NsfCleanupObject(object, "CallStackDoDestroy");
  }
}

static void
CallStackDestroyObject(Tcl_Interp *interp, NsfObject *object) {

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "CallStackDestroyObject %p %s activationcount %d flags %.6x\n",
          object, ObjectName(object), object->activationCount, object->flags);
#endif

  if ((object->flags & NSF_DESTROY_CALLED) == 0) {
    int activationCount = object->activationCount;
    /* if the destroy method was not called yet, do it now */
#ifdef OBJDELETION_TRACE
    fprintf(stderr, "  CallStackDestroyObject has to DispatchDestroyMethod %p activationCount %d\n",
            object, activationCount);
#endif
    DispatchDestroyMethod(interp, object, 0);

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
static NsfCmdList *
CmdListAdd(NsfCmdList **cList, Tcl_Command c, NsfClass *clorobj, int noDuplicates) {
  NsfCmdList *l = *cList, *new;

  /*
   * check for duplicates, if necessary
   */
  if (noDuplicates) {
    NsfCmdList *h = l, **end = NULL;
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
  new = NEW(NsfCmdList);
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
CmdListReplaceCmd(NsfCmdList *replace, Tcl_Command cmd, NsfClass *clorobj) {
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
CmdListPrint(Tcl_Interp *interp, CONST char *title, NsfCmdList *cmdList) {
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
CmdListDeleteCmdListEntry(NsfCmdList *del, NsfFreeCmdListClientData *freeFct) {
  if (freeFct)
    (*freeFct)(del);
  MEM_COUNT_FREE("command refCount", del->cmdPtr);
  TclCleanupCommand((Command *)del->cmdPtr);
  FREE(NsfCmdList, del);
}

/*
 * remove a command 'delCL' from a command list, but do not
 * free it ... returns the removed NsfCmdList*
 */
static NsfCmdList *
CmdListRemoveFromList(NsfCmdList **cmdList, NsfCmdList *delCL) {
  register NsfCmdList *c = *cmdList, *del = NULL;
  if (c == NULL) {
    return NULL;
  }
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
CmdListRemoveEpoched(NsfCmdList **cmdList, NsfFreeCmdListClientData *freeFct) {
  NsfCmdList *f = *cmdList, *del;
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
CmdListRemoveContextClassFromList(NsfCmdList **cmdList, NsfClass *clorobj,
                                  NsfFreeCmdListClientData *freeFct) {
  NsfCmdList *c, *del = NULL;
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
CmdListRemoveList(NsfCmdList **cmdList, NsfFreeCmdListClientData *freeFct) {
  NsfCmdList *del;
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
static NsfCmdList *
CmdListFindCmdInList(Tcl_Command cmd, NsfCmdList *l) {
  register NsfCmdList *h;
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
static NsfCmdList *
CmdListFindNameInList(Tcl_Interp *interp, CONST char *name, NsfCmdList *l) {
  register NsfCmdList *h;
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
static NsfTclObjList *
AssertionNewList(Tcl_Interp *interp, Tcl_Obj *aObj) {
  Tcl_Obj **ov; int oc;
  NsfTclObjList *last = NULL;

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
AssertionList(Tcl_Interp *interp, NsfTclObjList *alist) {
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
  for (; alist; alist = alist->nextPtr) {
    Tcl_ListObjAppendElement(interp, listObj, alist->content);
  }
  return listObj;
}



/* append a string of pre and post assertions to a method body */
static void
AssertionAppendPrePost(Tcl_Interp *interp, Tcl_DString *dsPtr, NsfProcAssertion *procs) {
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
AssertionListCheckOption(Tcl_Interp *interp, NsfObject *object) {
  NsfObjectOpt *opt = object->opt;
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

static NsfProcAssertion *
AssertionFindProcs(NsfAssertionStore *aStore, CONST char *name) {
  Tcl_HashEntry *hPtr;
  if (aStore == NULL) return NULL;
  hPtr = Tcl_CreateHashEntry(&aStore->procs, name, NULL);
  if (hPtr == NULL) return NULL;
  return (NsfProcAssertion *) Tcl_GetHashValue(hPtr);
}

static void
AssertionRemoveProc(NsfAssertionStore *aStore, CONST char *name) {
  Tcl_HashEntry *hPtr;
  if (aStore) {
    hPtr = Tcl_CreateHashEntry(&aStore->procs, name, NULL);
    if (hPtr) {
      NsfProcAssertion *procAss =
        (NsfProcAssertion *) Tcl_GetHashValue(hPtr);
      TclObjListFreeList(procAss->pre);
      TclObjListFreeList(procAss->post);
      FREE(NsfProcAssertion, procAss);
      Tcl_DeleteHashEntry(hPtr);
    }
  }
}

static void
AssertionAddProc(Tcl_Interp *interp, CONST char *name, NsfAssertionStore *aStore,
                 Tcl_Obj *pre, Tcl_Obj *post) {
  int nw = 0;
  Tcl_HashEntry *hPtr = NULL;
  NsfProcAssertion *procs = NEW(NsfProcAssertion);

  AssertionRemoveProc(aStore, name);
  procs->pre = AssertionNewList(interp, pre);
  procs->post = AssertionNewList(interp, post);
  hPtr = Tcl_CreateHashEntry(&aStore->procs, name, &nw);
  if (nw) Tcl_SetHashValue(hPtr, (ClientData)procs);
}

static NsfAssertionStore *
AssertionCreateStore() {
  NsfAssertionStore *aStore = NEW(NsfAssertionStore);
  aStore->invariants = NULL;
  Tcl_InitHashTable(&aStore->procs, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", &aStore->procs);
  return aStore;
}

static void
AssertionRemoveStore(NsfAssertionStore *aStore) {
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
    FREE(NsfAssertionStore, aStore);
  }
}

/*
 * check a given condition in the current callframe's scope
 * it's the responsiblity of the caller to push the intended callframe
 */
static int
CheckConditionInScope(Tcl_Interp *interp, Tcl_Obj *condition) {
  int result, success;
  Tcl_Obj *ov[2] = {NULL, condition};

  INCR_REF_COUNT(condition);
  result = Nsf_ExprObjCmd(NULL, interp, 2, ov);
  DECR_REF_COUNT(condition);

  if (result == TCL_OK) {
    result = Tcl_GetBooleanFromObj(interp, Tcl_GetObjResult(interp), &success);

    if (result == TCL_OK && success == 0)
      result = NSF_CHECK_FAILED;
  }
  return result;
}

static int
AssertionCheckList(Tcl_Interp *interp, NsfObject *object,
                   NsfTclObjList *alist, CONST char *methodName) {
  NsfTclObjList *checkFailed = NULL;
  Tcl_Obj *savedObjResult = Tcl_GetObjResult(interp);
  int savedCheckoptions, acResult = TCL_OK;

  /*
   * no obj->opt -> checkoption == CHECK_NONE
   */
  if (!object->opt)
    return TCL_OK;

  /* we do not check assertion modifying methods, otherwise
     we cannot react in catch on a runtime assertion check failure */

#if 1
  /* TODO: the following check operations is XOTcl1 legacy and is not
     generic. it should be replaced by another methodproperty.
     Most of the is*String()
     definition are then obsolete and should be deleted from
     nsfInt.h as well.
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
      CallFrame frame, *framePtr = &frame;
      Nsf_PushFrameObj(interp, object, framePtr);

      /* don't check assertion during assertion check */
      savedCheckoptions = object->opt->checkoptions;
      object->opt->checkoptions = CHECK_NONE;

      /* fprintf(stderr, "Checking Assertion %s ", assStr); */

      /*
       * now check the assertion in the pushed callframe's scope
       */
      acResult = CheckConditionInScope(interp, alist->content);
      if (acResult != TCL_OK)
        checkFailed = alist;

      object->opt->checkoptions = savedCheckoptions;
      /* fprintf(stderr, "...%s\n", checkFailed ? "failed" : "ok"); */
      Nsf_PopFrameObj(interp, framePtr);
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
      NsfPrintError(interp, "Error in Assertion: {%s} in proc '%s'\n%s",
		    ObjStr(checkFailed->content), methodName, ObjStr(sr));
      DECR_REF_COUNT(sr);
      return TCL_ERROR;
    }
    return NsfPrintError(interp, "Assertion failed check: {%s} in proc '%s'",
			 ObjStr(checkFailed->content), methodName);
  }

  Tcl_SetObjResult(interp, savedObjResult);
  DECR_REF_COUNT(savedObjResult);
  return TCL_OK;
}

static int
AssertionCheckInvars(Tcl_Interp *interp, NsfObject *object,
                     CONST char *methodName,
                     CheckOptions checkoptions) {
  int result = TCL_OK;

  if (checkoptions & CHECK_OBJINVAR && object->opt->assertions) {
    result = AssertionCheckList(interp, object, object->opt->assertions->invariants,
                                methodName);
  }

  if (result != TCL_ERROR && checkoptions & CHECK_CLINVAR) {
    NsfClasses *clPtr;
    clPtr = ComputeOrder(object->cl, object->cl->order, Super);
    while (clPtr && result != TCL_ERROR) {
      NsfAssertionStore *aStore = (clPtr->cl->opt) ? clPtr->cl->opt->assertions : NULL;
      if (aStore) {
        result = AssertionCheckList(interp, object, aStore->invariants, methodName);
      }
      clPtr = clPtr->nextPtr;
    }
  }
  return result;
}

static int
AssertionCheck(Tcl_Interp *interp, NsfObject *object, NsfClass *cl,
               CONST char *method, int checkOption) {
  NsfProcAssertion *procs;
  int result = TCL_OK;
  NsfAssertionStore *aStore;

  if (cl)
    aStore = cl->opt ? cl->opt->assertions : NULL;
  else
    aStore = object->opt ? object->opt->assertions : NULL;

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

static int
AssertionSetCheckOptions(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *arg) {
  NsfObjectOpt *opt = NsfRequireObjectOpt(object);
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
    return NsfPrintError(interp, "Unknown check option in command '%s' check %s, ", 
			 "valid: all pre post object-invar class-invar",
			 ObjectName(object), ObjStr(arg));
  }
  return TCL_OK;
}

static void
AssertionSetInvariants(Tcl_Interp *interp, NsfAssertionStore **assertions, Tcl_Obj *arg) {
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
MixinStackPush(NsfObject *object) {
  register NsfMixinStack *h = NEW(NsfMixinStack);

  h->currentCmdPtr = NULL;
  h->nextPtr = object->mixinStack;
  object->mixinStack = h;
  return 1;
}

/*
 * pop a mixin stack information on this object
 */
static void
MixinStackPop(NsfObject *object) {
  register NsfMixinStack *h = object->mixinStack;

  object->mixinStack = h->nextPtr;
  FREE(NsfMixinStack, h);
}

/*
 * Appends NsfClasses *containing the mixin classes and their
 * superclasses to 'mixinClasses' list from a given mixinList
 */
static void
MixinComputeOrderFullList(Tcl_Interp *interp, NsfCmdList **mixinList,
                          NsfClasses **mixinClasses,
                          NsfClasses **checkList, int level) {
  NsfCmdList *m;
  NsfClasses *pl, **clPtr = mixinClasses;

  CmdListRemoveEpoched(mixinList, GuardDel);

  for (m = *mixinList; m; m = m->nextPtr) {
    NsfClass *mCl = NsfGetClassFromCmdPtr(m->cmdPtr);
    if (mCl) {
      for (pl = ComputeOrder(mCl, mCl->order, Super); pl; pl = pl->nextPtr) {
        /*fprintf(stderr, " %s, ", ObjStr(pl->cl->object.cmdName));*/
        if ((pl->cl->object.flags & NSF_IS_ROOT_CLASS) == 0) {
          NsfClassOpt *opt = pl->cl->opt;
          if (opt && opt->classmixins) {
            /* compute transitively the (class) mixin classes of this
               added class */
            NsfClasses *cls;
            int i, found = 0;
            for (i=0, cls = *checkList; cls; i++, cls = cls->nextPtr) {
              /* fprintf(stderr, "+++ c%d: %s\n", i,
                 ClassName(cls->cl));*/
              if (pl->cl == cls->cl) {
                found = 1;
                break;
              }
            }
            if (!found) {
              NsfClassListAdd(checkList, pl->cl, NULL);
              /*fprintf(stderr, "+++ transitive %s\n",
                ObjStr(pl->cl->object.cmdName));*/

              MixinComputeOrderFullList(interp, &opt->classmixins, mixinClasses,
                                        checkList, level+1);
            }
          }
          /* fprintf(stderr, "+++ add to mixinClasses %p path: %s clPtr %p\n",
             mixinClasses, ObjStr(pl->cl->object.cmdName), clPtr);*/
          clPtr = NsfClassListAdd(clPtr, pl->cl, m->clientData);
        }
      }
    }
  }
  if (level == 0 && *checkList) {
    NsfClassListFree(*checkList);
    *checkList = NULL;
  }
}

static void
MixinResetOrder(NsfObject *object) {
  /*fprintf(stderr, "removeList %s \n", ObjectName(object));*/
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
MixinComputeOrder(Tcl_Interp *interp, NsfObject *object) {
  NsfClasses *fullList, *checkList = NULL, *mixinClasses = NULL, *nextCl, *pl,
    *checker, *guardChecker;

  if (object->mixinOrder)  MixinResetOrder(object);

  /* append per-obj mixins */
  if (object->opt) {
    MixinComputeOrderFullList(interp, &object->opt->mixins, &mixinClasses,
                              &checkList, 0);
  }

  /* append per-class mixins */
  for (pl = ComputeOrder(object->cl, object->cl->order, Super); pl; pl = pl->nextPtr) {
    NsfClassOpt *opt = pl->cl->opt;
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
      NsfCmdList *new;
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
  NsfClassListFree(fullList);

  /*CmdListPrint(interp, "mixin order\n", obj->mixinOrder);*/

}

/*
 * add a mixin class to 'mixinList' by appending it
 */
static int
MixinAdd(Tcl_Interp *interp, NsfCmdList **mixinList, Tcl_Obj *nameObj, NsfClass *baseClass) {
  NsfClass *mixin;
  Tcl_Obj *guardObj = NULL;
  int ocName; Tcl_Obj **ovName;
  NsfCmdList *new;

  if (Tcl_ListObjGetElements(interp, nameObj, &ocName, &ovName) == TCL_OK && ocName > 1) {
    if (ocName == 3 && !strcmp(ObjStr(ovName[1]), NsfGlobalStrings[NSF_GUARD_OPTION])) {
      nameObj = ovName[0];
      guardObj = ovName[2];
      /*fprintf(stderr, "mixinadd name = '%s', guard = '%s'\n", ObjStr(name), ObjStr(guard));*/
    } /*else return NsfPrintError(interp, "mixin registration '%s' has too many elements", 
	ObjStr(name));*/
  }

  if (GetClassFromObj(interp, nameObj, &mixin, baseClass) != TCL_OK)
    return NsfObjErrType(interp, "mixin", nameObj, "a class as mixin", NULL);

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
 *----------------------------------------------------------------------
 * AppendMatchingElement --
 *
 *    Call AppendElement for values matching the specified pattern
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Appends element to the result object
 *
 *----------------------------------------------------------------------
 */
static void
AppendMatchingElement(Tcl_Interp *interp, Tcl_Obj *nameObj, CONST char *pattern) {
  CONST char *string = ObjStr(nameObj);
  if (!pattern || Tcl_StringMatch(string, pattern)) {
    Tcl_AppendElement(interp, string);
  }
}

/*
 *----------------------------------------------------------------------
 * AppendMatchingElementsFromCmdList --
 *
 *    Apply AppendMatchingElement() to all elements of the passed
 *    Cmdlist
 *
 * Results:
 *    1 iff a matching object was provided and it was found; 0 otherwise
 *
 * Side effects:
 *    Appends elements to the result
 *
 *----------------------------------------------------------------------
 */
static int
AppendMatchingElementsFromCmdList(Tcl_Interp *interp, NsfCmdList *cmdl,
                                  CONST char *pattern, NsfObject *matchObject) {
  int rc = 0;
  for ( ; cmdl; cmdl = cmdl->nextPtr) {
    NsfObject *object = NsfGetObjectFromCmdPtr(cmdl->cmdPtr);
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
 *----------------------------------------------------------------------
 * AppendMatchingElementsFromClasses --
 *
 *    Apply AppendMatchingElement() to all elements of the passed
 *    class list
 *
 * Results:
 *    1 iff a matching object was provided and it was found; 0 otherwise
 *
 * Side effects:
 *    Appends elements to the result
 *
 *----------------------------------------------------------------------
 */
static int
AppendMatchingElementsFromClasses(Tcl_Interp *interp, NsfClasses *cls,
				  CONST char *pattern, NsfObject *matchObject) {
  int rc = 0;

  for ( ; cls; cls = cls->nextPtr) {
    NsfObject *object = (NsfObject *)cls->cl;
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
 *----------------------------------------------------------------------
 * GetAllInstances --
 *
 *    Get all instances of a class recursively into an initialized
 *    String key hashtable
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Passed hash table contains instances
 *
 *----------------------------------------------------------------------
 */
static void
GetAllInstances(Tcl_Interp *interp, Tcl_HashTable *destTablePtr, NsfClass *startCl) {
  NsfClasses *sc;
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;
  Tcl_HashTable *tablePtr = &startCl->instances;

  for (hPtr = Tcl_FirstHashEntry(tablePtr, &search);  hPtr;
       hPtr = Tcl_NextHashEntry(&search)) {
    NsfObject *inst = (NsfObject *)Tcl_GetHashKey(tablePtr, hPtr);
    Command *cmdPtr;
    int new;

    if (inst->flags & NSF_TCL_DELETE) {
      NsfLog(interp, NSF_LOG_NOTICE, "Object %s is apparently deleted", ObjectName(inst));
      continue;
    }

    cmdPtr = (Command *)inst->id;
    assert(cmdPtr);

    if (cmdPtr && (cmdPtr->nsPtr->flags & NS_DYING)) {
      NsfLog(interp, NSF_LOG_WARN, "Namespace of %s is apparently deleted", ObjectName(inst));
      continue;
    }

#if !defined(NDEBUG)
    {
      NsfObject *object = GetObjectFromString(interp, ObjectName(inst));
      assert(object);
    }
#endif

    /*fprintf (stderr, " -- %p flags %.6x activation %d %s id %p id->flags %.6x "
	     "nsPtr->flags %.6x (instance of %s)\n", 
	     inst, inst->flags, inst->activationCount, 
	     ObjectName(inst), inst->id, cmdPtr->flags, cmdPtr->nsPtr ? cmdPtr->nsPtr->flags : 0,
	     ObjStr(startCl->object.cmdName));*/

    Tcl_CreateHashEntry(destTablePtr, ObjectName(inst), &new);
    
  }
  for (sc = startCl->sub; sc; sc = sc->nextPtr) {
    GetAllInstances(interp, destTablePtr, sc->cl);
  }
}

/*
 *----------------------------------------------------------------------
 * AddToResultSet --
 *
 *    Helper function to add classes to the result set (implemented as
 *    a hash table), flagging test for matchObject as result
 *
 * Results:
 *    1 iff a matching object was provided and it was found; 0 otherwise
 *
 * Side effects:
 *    Appends optionally element to the result object
 *
 *----------------------------------------------------------------------
 */
static int
AddToResultSet(Tcl_Interp *interp, Tcl_HashTable *destTablePtr, NsfObject *object, int *new,
	       int appendResult, CONST char *pattern, NsfObject *matchObject) {
  Tcl_CreateHashEntry(destTablePtr, (char *)object, new);
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
 *----------------------------------------------------------------------
 * AddToResultSet --
 *
 *    Helper function to add classes with guards to the result set
 *    (implemented as a hash table), flagging test for matchObject as
 *    result.
 *
 * Results:
 *    1 iff a matching object was provided and it was found; 0 otherwise
 *
 * Side effects:
 *    Appends optionally element to the result object
 *
 *----------------------------------------------------------------------
 */
static int
AddToResultSetWithGuards(Tcl_Interp *interp, Tcl_HashTable *destTablePtr, NsfClass *cl,
			 ClientData clientData, int *new, int appendResult,
			 CONST char *pattern, NsfObject *matchObject) {
  Tcl_CreateHashEntry(destTablePtr, (char *)cl, new);
  if (*new) {
    if (appendResult) {
      if (!pattern || Tcl_StringMatch(ClassName(cl), pattern)) {
        Tcl_Obj *l = Tcl_NewListObj(0, NULL);
        Tcl_Obj *g = (Tcl_Obj*) clientData;
        Tcl_ListObjAppendElement(interp, l, cl->object.cmdName);
        Tcl_ListObjAppendElement(interp, l, NsfGlobalObjs[NSF_GUARD_OPTION]);
        Tcl_ListObjAppendElement(interp, l, g);
	Tcl_AppendElement(interp, ObjStr(l));
	DECR_REF_COUNT(l);
      }
    }
    if (matchObject && matchObject == (NsfObject *)cl) {
      return 1;
    }
  }
  return 0;
}

/*
 *----------------------------------------------------------------------
 * GetAllObjectMixinsOf --
 *
 *    Computes a set of classes, into which this class was mixed in
 *    via per object mixin. The function gets recursively all per
 *    object mixins from an class and its subclasses/isClassMixinOf
 *    and adds it into an initialized object ptr hashtable
 *    (TCL_ONE_WORD_KEYS)
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    The set of classes is returned in the provided hash table
 *
 *----------------------------------------------------------------------
 */
static int
GetAllObjectMixinsOf(Tcl_Interp *interp, Tcl_HashTable *destTablePtr, NsfClass *startCl,
		     int isMixin,
		     int appendResult, CONST char *pattern, NsfObject *matchObject) {
  int rc = 0, new = 0;
  NsfClasses *sc;

  /*fprintf(stderr, "startCl = %s, opt %p, isMixin %d, pattern '%s', matchObject %p\n",
    ClassName(startCl), startCl->opt, isMixin, pattern, matchObject);*/

  /*
   * check all subclasses of startCl for mixins
   */
  for (sc = startCl->sub; sc; sc = sc->nextPtr) {
    rc = GetAllObjectMixinsOf(interp, destTablePtr, sc->cl, isMixin, appendResult,
			      pattern, matchObject);
    if (rc) {return rc;}
  }
  /*fprintf(stderr, "check subclasses of %s done\n", ObjStr(startCl->object.cmdName));*/

  if (startCl->opt) {
    NsfCmdList *m;
    NsfClass *cl;
    for (m = startCl->opt->isClassMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == 0);

      cl = NsfGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);
      /*fprintf(stderr, "check %s mixinof %s\n",
        ClassName(cl), ObjStr(startCl->object.cmdName));*/
      rc = GetAllObjectMixinsOf(interp, destTablePtr, cl, isMixin, appendResult,
				pattern, matchObject);
      /* fprintf(stderr, "check %s mixinof %s done\n",
      ClassName(cl), ObjStr(startCl->object.cmdName));*/
      if (rc) {return rc;}
    }
  }

  /*
   * check, if startCl has associated per-object mixins
   */
  if (startCl->opt) {
    NsfCmdList *m;
    NsfObject *object;

    for (m = startCl->opt->isObjectMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == 0);

      object = NsfGetObjectFromCmdPtr(m->cmdPtr);
      assert(object);

      rc = AddToResultSet(interp, destTablePtr, object, &new, appendResult,
			  pattern, matchObject);
      if (rc == 1) {return rc;}
    }
  }
  return rc;
}


/*
 *----------------------------------------------------------------------
 * GetAllClassMixinsOf --
 *
 *    Computes a set of classes, into which this class was mixed in
 *    via as a class mixin. The function gets recursively all per
 *    class mixins from an class and its subclasses and adds it
 *    into an initialized object ptr hashtable (TCL_ONE_WORD_KEYS)
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    The set of classes is returned in the provided hash table
 *
 *----------------------------------------------------------------------
 */
static int
GetAllClassMixinsOf(Tcl_Interp *interp, Tcl_HashTable *destTablePtr,
		    /*@notnull@*/ NsfClass *startCl, int isMixin,
                    int appendResult, CONST char *pattern, NsfObject *matchObject) {
  int rc = 0, new = 0;
  NsfClass *cl;
  NsfClasses *sc;

  assert(startCl);

  /*fprintf(stderr, "startCl = %p %s, opt %p, isMixin %d\n",
    startCl, ClassName(startCl), startCl->opt, isMixin);*/

  /*
   * the startCl is a per class mixin, add it to the result set
   */
  if (isMixin) {
    rc = AddToResultSet(interp, destTablePtr, &startCl->object, &new, appendResult,
			pattern, matchObject);
    if (rc == 1) {return rc;}

    /*
     * check all subclasses of startCl for mixins
     */
    for (sc = startCl->sub; sc; sc = sc->nextPtr) {
#if !defined(NDEBUG)
      if (sc->cl == startCl) {
        /* 
	 * Sanity check: it seems that we can create via
         *  __default_superclass a class which has itself as subclass!
	 */
        fprintf(stderr, "... STRANGE %p is subclass of %p %s, sub %p\n", sc->cl,
                startCl, ClassName(startCl), startCl->sub);
	continue;
      }
#endif
      assert(sc->cl != startCl);
      rc = GetAllClassMixinsOf(interp, destTablePtr, sc->cl, isMixin, appendResult,
			       pattern, matchObject);
      if (rc) {
	return rc;
      }
    }
  }

  /*
   * check, if startCl is a per-class mixin of some other classes
   */
  if (startCl->opt) {
    NsfCmdList *m;

    for (m = startCl->opt->isClassMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == 0);

      cl = NsfGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);

      rc = AddToResultSet(interp, destTablePtr, &cl->object, &new, appendResult,
			  pattern, matchObject);
      if (rc == 1) {return rc;}
      if (new) {
        /*fprintf(stderr, "... new\n");*/
        rc = GetAllClassMixinsOf(interp, destTablePtr, cl, 1, appendResult,
				 pattern, matchObject);
        if (rc) {return rc;}
      }
    }
  }

  return rc;
}

/*
 *----------------------------------------------------------------------
 * GetAllClassMixins --
 *
 *    Computes a set class-mixins of a given class and handles
 *    transitive cases. The classes are added it into an initialized
 *    object ptr hashtable (TCL_ONE_WORD_KEYS)
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    The set of classes is returned in the provided hash table
 *
 *----------------------------------------------------------------------
 */

static int
GetAllClassMixins(Tcl_Interp *interp, Tcl_HashTable *destTablePtr, NsfClass *startCl,
		  int withGuards, CONST char *pattern, NsfObject *matchObject) {
  int rc = 0, new = 0;
  NsfClass *cl;
  NsfClasses *sc;

  /*
   * check this class for classmixins
   */
  if (startCl->opt) {
    NsfCmdList *m;

    for (m = startCl->opt->classmixins; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == 0);

      cl = NsfGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);

      /* fprintf(stderr, "class mixin found: %s\n", ClassName(cl)); */

      if ((withGuards) && (m->clientData)) {
        /* fprintf(stderr, "AddToResultSetWithGuards: %s\n", ClassName(cl)); */
        rc = AddToResultSetWithGuards(interp, destTablePtr, cl, m->clientData, &new, 1,
				      pattern, matchObject);
      } else {
        /* fprintf(stderr, "AddToResultSet: %s\n", ClassName(cl)); */
	rc = AddToResultSet(interp, destTablePtr, &cl->object, &new, 1,
			    pattern, matchObject);
      }
      if (rc == 1) {return rc;}

      if (new) {
        /* fprintf(stderr, "class mixin GetAllClassMixins for: %s (%s)\n",
	   ClassName(cl), ObjStr(startCl->object.cmdName)); */
        rc = GetAllClassMixins(interp, destTablePtr, cl, withGuards,
			       pattern, matchObject);
        if (rc) {return rc;}
      }
    }
  }


  /*
   * check all superclasses of startCl for classmixins
   */
  for (sc = startCl->super; sc; sc = sc->nextPtr) {
    /* fprintf(stderr, "Superclass GetAllClassMixins for %s (%s)\n",
       ObjStr(sc->cl->object.cmdName), ObjStr(startCl->object.cmdName)); */
    rc = GetAllClassMixins(interp, destTablePtr, sc->cl, withGuards,
			   pattern, matchObject);
    if (rc) {return rc;}
  }
  return rc;
}


static void
RemoveFromClassMixinsOf(Tcl_Command cmd, NsfCmdList *cmdlist) {

  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    NsfClass *ncl = NsfGetClassFromCmdPtr(cmdlist->cmdPtr);
    NsfClassOpt *nclopt = ncl ? ncl->opt : NULL;
    if (nclopt) {
      NsfCmdList *del = CmdListFindCmdInList(cmd, nclopt->isClassMixinOf);
      if (del) {
        /* fprintf(stderr, "Removing class %s from isClassMixinOf of class %s\n",
           ClassName(cl), ObjStr(NsfGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
        del = CmdListRemoveFromList(&nclopt->isClassMixinOf, del);
        CmdListDeleteCmdListEntry(del, GuardDel);
      }
    }
  }
}

static void
RemoveFromObjectMixinsOf(Tcl_Command cmd, NsfCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    NsfClass *cl = NsfGetClassFromCmdPtr(cmdlist->cmdPtr);
    NsfClassOpt *clopt = cl ? cl->opt : NULL;
    if (clopt) {
      NsfCmdList *del = CmdListFindCmdInList(cmd, clopt->isObjectMixinOf);
      if (del) {
        /* fprintf(stderr, "Removing object %s from isObjectMixinOf of Class %s\n",
           ObjectName(object), ObjStr(NsfGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
        del = CmdListRemoveFromList(&clopt->isObjectMixinOf, del);
        CmdListDeleteCmdListEntry(del, GuardDel);
      }
    } /* else fprintf(stderr, "CleanupDestroyObject %s: NULL pointer in mixins!\n", ObjectName(object)); */
  }
}

static void
RemoveFromClassmixins(Tcl_Command cmd, NsfCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    NsfClass *cl = NsfGetClassFromCmdPtr(cmdlist->cmdPtr);
    NsfClassOpt *clopt = cl ? cl->opt : NULL;
    if (clopt) {
      NsfCmdList *del = CmdListFindCmdInList(cmd, clopt->classmixins);
      if (del) {
        /* fprintf(stderr, "Removing class %s from mixins of object %s\n",
           ClassName(cl), ObjStr(NsfGetObjectFromCmdPtr(cmdlist->cmdPtr)->cmdName)); */
        del = CmdListRemoveFromList(&clopt->classmixins, del);
        CmdListDeleteCmdListEntry(del, GuardDel);
	if (cl->object.mixinOrder) MixinResetOrder(&cl->object);
      }
    }
  }
}

static void
RemoveFromMixins(Tcl_Command cmd, NsfCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    NsfObject *nobj = NsfGetObjectFromCmdPtr(cmdlist->cmdPtr);
    NsfObjectOpt *objopt = nobj ? nobj->opt : NULL;
    if (objopt) {
      NsfCmdList *del = CmdListFindCmdInList(cmd, objopt->mixins);
      if (del) {
        /* fprintf(stderr, "Removing class %s from mixins of object %s\n",
           ClassName(cl), ObjStr(NsfGetObjectFromCmdPtr(cmdlist->cmdPtr)->cmdName)); */
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
MixinResetOrderForInstances(Tcl_Interp *interp, NsfClass *cl) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  hPtr = Tcl_FirstHashEntry(&cl->instances, &hSrch);

  /*fprintf(stderr, "invalidating instances of class %s\n",
    ObjStr(clPtr->cl->object.cmdName));*/

  /* Here we should check, whether this class is used as an object or
     class mixin somewhere else and invalidate the objects of these as
     well -- */

  for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    NsfObject *object = (NsfObject *)Tcl_GetHashKey(&cl->instances, hPtr);
    if (object
        && !(object->flags & NSF_DURING_DELETE)
        && (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID)) {
      MixinResetOrder(object);
      object->flags &= ~NSF_MIXIN_ORDER_VALID;
    }
  }
}

/* reset mixin order for all objects having this class as per object mixin */
static void
ResetOrderOfClassesUsedAsMixins(NsfClass *cl) {
  /*fprintf(stderr, "ResetOrderOfClassesUsedAsMixins %s - %p\n",
    ClassName(cl), cl->opt);*/

  if (cl->opt) {
    NsfCmdList *ml;
    for (ml = cl->opt->isObjectMixinOf; ml; ml = ml->nextPtr) {
      NsfObject *object = NsfGetObjectFromCmdPtr(ml->cmdPtr);
      if (object) {
        if (object->mixinOrder) { MixinResetOrder(object); }
        object->flags &= ~NSF_MIXIN_ORDER_VALID;
      }
    }
  }
}



/*
 * if the class hierarchy or class mixins have changed ->
 * invalidate mixin entries in all dependent instances
 */
static void
MixinInvalidateObjOrders(Tcl_Interp *interp, NsfClass *cl) {
  NsfClasses *saved = cl->order, *clPtr;
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
      NsfObject *object = (NsfObject *)Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      if (object->mixinOrder) { MixinResetOrder(object); }
      object->flags &= ~NSF_MIXIN_ORDER_VALID;
    }
  }

  NsfClassListFree(cl->order);
  cl->order = saved;

  /* Reset mixin order for all objects having this class as a per
     class mixin.  This means that we have to work through
     the class mixin hierarchy with its corresponding instances.
  */
  Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
  GetAllClassMixinsOf(interp, commandTable, cl, 1, 0, NULL, NULL);

  for (hPtr = Tcl_FirstHashEntry(commandTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    NsfClass *ncl = (NsfClass *)Tcl_GetHashKey(commandTable, hPtr);
    /*fprintf(stderr, "Got %s, reset for ncl %p\n", ncl?ObjStr(ncl->object.cmdName):"NULL", ncl);*/
    if (ncl) {
      MixinResetOrderForInstances(interp, ncl);
      /* this place seems to be sufficient to invalidate the computed object parameter definitions */
      /*fprintf(stderr, "MixinInvalidateObjOrders via class mixin %s calls ifd invalidate \n", ClassName(ncl));*/
      NsfInvalidateObjectParameterCmd(interp, ncl);
    }
  }
  Tcl_DeleteHashTable(commandTable);
  MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
}

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
MixinComputeDefined(Tcl_Interp *interp, NsfObject *object) {
  MixinComputeOrder(interp, object);
  object->flags |= NSF_MIXIN_ORDER_VALID;
  if (object->mixinOrder)
    object->flags |= NSF_MIXIN_ORDER_DEFINED;
  else
    object->flags &= ~NSF_MIXIN_ORDER_DEFINED;
}

/*
 * Walk through the command list until the current command is reached.
 * return the next entry.
 *
 */
static NsfCmdList *
SeekCurrent(Tcl_Command currentCmd, register NsfCmdList *cmdl) {
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
MixinSearchProc(Tcl_Interp *interp, NsfObject *object, CONST char *methodName,
                NsfClass **clPtr, Tcl_Command *currentCmdPtr, Tcl_Command *cmdPtr) {
  Tcl_Command cmd = NULL;
  NsfCmdList *cmdList;
  NsfClass *cl;
  int result = TCL_OK;

  assert(object);
  assert(object->mixinStack);

  /* ensure that the mixin order is not invalid, otherwise compute order */
  assert(object->flags & NSF_MIXIN_ORDER_VALID);
  /*MixinComputeDefined(interp, object);*/
  cmdList = SeekCurrent(object->mixinStack->currentCmdPtr, object->mixinOrder);
  RUNTIME_STATE(interp)->cmdPtr = cmdList ? cmdList->cmdPtr : NULL;

  /*fprintf(stderr, "MixinSearch searching for '%s' %p\n", methodName, cmdList);*/
  /*CmdListPrint(interp, "MixinSearch CL = \n", cmdList);*/

  for (; cmdList; cmdList = cmdList->nextPtr) {

    if (Tcl_Command_cmdEpoch(cmdList->cmdPtr)) {
      continue;
    }
    cl = NsfGetClassFromCmdPtr(cmdList->cmdPtr);
    assert(cl);
    /*
      fprintf(stderr, "+++ MixinSearch %s->%s in %p cmdPtr %p clientData %p\n",
      ObjectName(object), methodName, cmdList,
      cmdList->cmdPtr, cmdList->clientData);
    */
    cmd = FindMethod(cl->nsPtr, methodName);
    if (cmd == NULL) {
      continue;
    }

    if (Tcl_Command_flags(cmd) & NSF_CMD_CLASS_ONLY_METHOD) {
      /*fprintf(stderr, "we found class specific method %s on class %s object %s, isclass %d\n",
	methodName, ClassName(cl), ObjectName(object), NsfObjectIsClass(object));*/
      if (!NsfObjectIsClass(object)) {
	/* the command is not for us; skip it */
	cmd = NULL;
	continue;
      }
    }

    if (cmdList->clientData) {
      if (!RUNTIME_STATE(interp)->guardCount) {
	/*fprintf(stderr, "guardcall\n");*/
	result = GuardCall(object, cl, (Tcl_Command) cmd, interp,
			   (Tcl_Obj*)cmdList->clientData, NULL);
      }
    }
    if (result == TCL_OK) {
      /*
       * on success: compute mixin call data
       */
      *clPtr = cl;
      *currentCmdPtr = cmdList->cmdPtr;
      /*fprintf(stderr, "mixinsearch returns %p (cl %s)\n",cmd, ClassName(cl));*/
      break;
    } else if (result == TCL_ERROR) {
      break;
    } else {
      if (result == NSF_CHECK_FAILED) result = TCL_OK;
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
MixinInfo(Tcl_Interp *interp, NsfCmdList *m, CONST char *pattern,
          int withGuards, NsfObject *matchObject) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  NsfClass *mixinClass;

  /*fprintf(stderr, "   mixin info m=%p, pattern %s, matchObject %p\n",
    m, pattern, matchObject);*/

  while (m) {
    /* fprintf(stderr, "   mixin info m=%p, next=%p, pattern %s, matchObject %p\n",
       m, m->next, pattern, matchObject);*/
    mixinClass = NsfGetClassFromCmdPtr(m->cmdPtr);
    if (mixinClass &&
        (!pattern
         || (matchObject && &(mixinClass->object) == matchObject)
         || (!matchObject && Tcl_StringMatch(ObjStr(mixinClass->object.cmdName), pattern)))) {
      if (withGuards && m->clientData) {
        Tcl_Obj *l = Tcl_NewListObj(0, NULL);
        Tcl_Obj *g = (Tcl_Obj*) m->clientData;
        Tcl_ListObjAppendElement(interp, l, mixinClass->object.cmdName);
        Tcl_ListObjAppendElement(interp, l, NsfGlobalObjs[NSF_GUARD_OPTION]);
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
MixinSearchMethodByName(Tcl_Interp *interp, NsfCmdList *mixinList, CONST char *name, NsfClass **cl) {
  Tcl_Command cmd;

  for (; mixinList;  mixinList = mixinList->nextPtr) {
    NsfClass *foundCl = NsfGetClassFromCmdPtr(mixinList->cmdPtr);
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
FilterSearch(Tcl_Interp *interp, CONST char *name, NsfObject *startingObject,
             NsfClass *startingClass, NsfClass **cl) {
  Tcl_Command cmd = NULL;

  if (startingObject) {
    NsfObjectOpt *opt = startingObject->opt;
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
    NsfClassOpt *opt = startingClass->opt;
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
      *cl = (NsfClass*)startingObject;
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
  NsfRuntimeState *rst = RUNTIME_STATE(interp);

  if (guardObj) {
    /*
     * if there are more than one filter guard for this filter
     * (i.e. they are inherited), then they are OR combined
     * -> if one check succeeds => return 1
     */

    /*fprintf(stderr, "checking guard **%s**\n", ObjStr(guardObj));*/

    rst->guardCount++;
    result = CheckConditionInScope(interp, guardObj);
    rst->guardCount--;

    /*fprintf(stderr, "checking guard **%s** returned rc=%d\n", ObjStr(guardObj), rc);*/

    if (result == TCL_OK) {
      /* fprintf(stderr, " +++ OK\n"); */
      return TCL_OK;
    } else if (result == TCL_ERROR) {
      Tcl_Obj *sr = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(sr);
      NsfPrintError(interp, "Guard Error: '%s'\n%s", ObjStr(guardObj), ObjStr(sr));
      DECR_REF_COUNT(sr);
      return TCL_ERROR;
    }
  }
  /*
    fprintf(stderr, " +++ FAILED\n");
  */
  return NSF_CHECK_FAILED;
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
GuardDel(NsfCmdList *CL) {
  /*fprintf(stderr, "GuardDel %p clientData = %p\n",
    CL, CL? CL->clientData : NULL);*/
  if (CL && CL->clientData) {
    DECR_REF_COUNT((Tcl_Obj *)CL->clientData);
    CL->clientData = NULL;
  }
}

NSF_INLINE static void
GuardAdd(Tcl_Interp *interp, NsfCmdList *CL, Tcl_Obj *guardObj) {
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
  GuardAddList(Tcl_Interp *interp, NsfCmdList *dest, ClientData source) {
  NsfTclObjList *s = (NsfTclObjList *)source;
  GuardAdd(interp, dest, (Tcl_Obj *)s->content);
  s = s->nextPtr;
  } */

static int
GuardCall(NsfObject *object, NsfClass *cl, Tcl_Command cmd,
          Tcl_Interp *interp, Tcl_Obj *guardObj, NsfCallStackContent *cscPtr) {
  int result = TCL_OK;

  if (guardObj) {
    Tcl_Obj *res = Tcl_GetObjResult(interp); /* save the result */
    CallFrame frame, *framePtr = &frame;

    INCR_REF_COUNT(res);

    /* GuardPrint(interp, cmdList->clientData); */
    /*
     * For the guard push a fake callframe on the Tcl stack so that
     * e.g. a "self calledproc" and other methods in the guard behave
     * like in the proc.
     */
    if (cscPtr) {
      Nsf_PushFrameCsc(interp, cscPtr, framePtr);
    } else {
      Nsf_PushFrameObj(interp, object, framePtr);
    }
    result = GuardCheck(interp, guardObj);

    if (cscPtr) {
      Nsf_PopFrameCsc(interp, framePtr);
    } else {
      Nsf_PopFrameObj(interp, framePtr);
    }

    if (result != TCL_ERROR) {
      Tcl_SetObjResult(interp, res);  /* restore the result */
    }
    DECR_REF_COUNT(res);
  }

  return result;
}

static int
GuardAddFromDefinitionList(Tcl_Interp *interp, NsfCmdList *dest,
                           Tcl_Command interceptorCmd,
                           NsfCmdList *interceptorDefList) {
  NsfCmdList *h;
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
GuardAddInheritedGuards(Tcl_Interp *interp, NsfCmdList *dest,
                        NsfObject *object, Tcl_Command filterCmd) {
  NsfClasses *pl;
  int guardAdded = 0;
  NsfObjectOpt *opt;

  /* search guards for classfilters registered on mixins */
  if (!(object->flags & NSF_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, object);
  if (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) {
    NsfCmdList *ml;
    NsfClass *mixin;
    for (ml = object->mixinOrder; ml && !guardAdded; ml = ml->nextPtr) {
      mixin = NsfGetClassFromCmdPtr(ml->cmdPtr);
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
      NsfClassOpt *opt = pl->cl->opt;
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
      NsfCmdList *registeredFilter =
        CmdListFindNameInList(interp, (char *) Tcl_GetCommandName(interp, filterCmd),
                              object->filterOrder);
      if (registeredFilter) {
        GuardAdd(interp, dest, (Tcl_Obj*) registeredFilter->clientData);
      }
    }
  }
}

static int
GuardList(Tcl_Interp *interp, NsfCmdList *frl, CONST char *interceptorName) {
  NsfCmdList *h;
  if (frl) {
    /* try to find simple name first */
    h = CmdListFindNameInList(interp, interceptorName, frl);
    if (!h) {
      /* maybe it is a qualified name */
      Tcl_Command cmd = NSFindCommand(interp, interceptorName);
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
  return NsfPrintError(interp, "info guard: can't find filter/mixin %s", interceptorName);
}

/*
 * append a filter command to the 'filterList' of an obj/class
 */
static int
FilterAdd(Tcl_Interp *interp, NsfCmdList **filterList, Tcl_Obj *nameObj,
          NsfObject *startingObject, NsfClass *startingClass) {
  Tcl_Command cmd;
  int ocName; Tcl_Obj **ovName;
  Tcl_Obj *guardObj = NULL;
  NsfCmdList *new;
  NsfClass *cl;

  if (Tcl_ListObjGetElements(interp, nameObj, &ocName, &ovName) == TCL_OK && ocName > 1) {
    if (ocName == 3 && !strcmp(ObjStr(ovName[1]), NsfGlobalStrings[NSF_GUARD_OPTION])) {
      nameObj = ovName[0];
      guardObj = ovName[2];
    }
  }

  if (!(cmd = FilterSearch(interp, ObjStr(nameObj), startingObject, startingClass, &cl))) {
    if (startingObject) {
      return NsfPrintError(interp, "object filter: can't find filterproc '%s' on %s ",
			   ObjStr(nameObj), ObjectName(startingObject));
    } else {
      return NsfPrintError(interp, "class filter: can't find filterproc '%s' on %s ",
			   ObjStr(nameObj), ClassName(startingClass));
    }
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
FilterResetOrder(NsfObject *object) {
  CmdListRemoveList(&object->filterOrder, GuardDel);
  object->filterOrder = NULL;
}

/*
 * search the filter in the hierarchy again with FilterSearch, e.g.
 * upon changes in the class hierarchy or mixins that carry the filter
 * command, so that we can be sure it is still reachable.
 */
static void
FilterSearchAgain(Tcl_Interp *interp, NsfCmdList **filters,
                  NsfObject *startingObject, NsfClass *startingClass) {
  char *simpleName;
  Tcl_Command cmd;
  NsfCmdList *cmdList, *del;
  NsfClass *cl = NULL;

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
FilterInvalidateObjOrders(Tcl_Interp *interp, NsfClass *cl) {
  NsfClasses *saved = cl->order, *clPtr, *savePtr;

  cl->order = NULL;
  savePtr = clPtr = ComputeOrder(cl, cl->order, Sub);
  cl->order = saved;

  for ( ; clPtr; clPtr = clPtr->nextPtr) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
      Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : NULL;

    /* recalculate the commands of all class-filter registrations */
    if (clPtr->cl->opt) {
      FilterSearchAgain(interp, &clPtr->cl->opt->classfilters, 0, clPtr->cl);
    }
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      NsfObject *object = (NsfObject *)Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      FilterResetOrder(object);
      object->flags &= ~NSF_FILTER_ORDER_VALID;

      /* recalculate the commands of all object filter registrations */
      if (object->opt) {
        FilterSearchAgain(interp, &object->opt->filters, object, 0);
      }
    }
  }
  NsfClassListFree(savePtr);
}

/*
 * from cl on down the hierarchy we remove all filters
 * the refer to "removeClass" namespace. E.g. used to
 * remove filters defined in superclass list from dependent
 * class cl
 */
static void
FilterRemoveDependentFilterCmds(NsfClass *cl, NsfClass *removeClass) {
  NsfClasses *saved = cl->order, *clPtr;
  cl->order = NULL;

  /*fprintf(stderr, "FilterRemoveDependentFilterCmds cl %p %s, removeClass %p %s\n",
    cl, ClassName(cl),
    removeClass, ObjStr(removeClass->object.cmdName));*/

  for (clPtr = ComputeOrder(cl, cl->order, Sub); clPtr; clPtr = clPtr->nextPtr) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
      Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : NULL;
    NsfClassOpt *opt = clPtr->cl->opt;
    if (opt) {
      CmdListRemoveContextClassFromList(&opt->classfilters, removeClass, GuardDel);
    }
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      NsfObject *object = (NsfObject*) Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      if (object->opt) {
        CmdListRemoveContextClassFromList(&object->opt->filters, removeClass, GuardDel);
      }
    }
  }

  NsfClassListFree(cl->order);
  cl->order = saved;
}

/*
 *----------------------------------------------------------------------
 * MethodHandleObj --
 *
 *    Builds a methodHandle from a method name.  In case the method
 *    name is fully qualified, it is simply returned.
 *
 * Results:
 *    fresh Tcl_Obj
 *
 * Side effects:
 *    none
 *
 *----------------------------------------------------------------------
 */
static Tcl_Obj *
MethodHandleObj(NsfObject *object, int withPer_object, CONST char *methodName) {
  Tcl_Obj *resultObj;

  if (*methodName == ':') {
    /*
     * if we have a methodname starting with ":" and we made it so far,
     * we assume it is correct
     */
    resultObj = Tcl_NewStringObj(methodName, -1);
  } else {
    resultObj = Tcl_NewStringObj(withPer_object ? "" : "::nsf::classes", -1);
    assert(object);
    Tcl_AppendObjToObj(resultObj, object->cmdName);
    Tcl_AppendStringsToObj(resultObj, "::", methodName, (char *) NULL);
  }
  return resultObj;
}

/*
 * info option for filters and classfilters
 * withGuards -> if not 0 => append guards
 * withMethodHandles -> if not 0 => return method handles
 */
static int
FilterInfo(Tcl_Interp *interp, NsfCmdList *f, CONST char *pattern,
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
        Tcl_ListObjAppendElement(interp, innerList, NsfGlobalObjs[NSF_GUARD_OPTION]);
        Tcl_ListObjAppendElement(interp, innerList, g);
        Tcl_ListObjAppendElement(interp, list, innerList);
      } else {
        if (withMethodHandles) {
	  NsfClass *filterClass = f->clorobj;
          Tcl_ListObjAppendElement(interp, list,
				   MethodHandleObj((NsfObject *)filterClass,
						   !NsfObjectIsClass(&filterClass->object), simpleName));
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
 * Appends NsfCmdPtr *containing the filter cmds and their
 * superclass specializations to 'filterList'
 */
static void
FilterComputeOrderFullList(Tcl_Interp *interp, NsfCmdList **filters,
                           NsfCmdList **filterList) {
  NsfCmdList *f ;
  char *simpleName;
  NsfClass *fcl;
  NsfClasses *pl;

  /*
   * ensure that no epoched command is in the filters list
   */
  CmdListRemoveEpoched(filters, GuardDel);

  for (f = *filters; f; f = f->nextPtr) {
    simpleName = (char *) Tcl_GetCommandName(interp, f->cmdPtr);
    fcl = f->clorobj;
    CmdListAdd(filterList, f->cmdPtr, fcl, /*noDuplicates*/ 0);

    if (fcl && !NsfObjectIsClass(&fcl->object)) {
      /* get the class from the object for per-object filter */
      fcl = ((NsfObject *)fcl)->cl;
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
FilterComputeOrder(Tcl_Interp *interp, NsfObject *object) {
  NsfCmdList *filterList = NULL, *next, *checker, *newlist;
  NsfClasses *pl;

  if (object->filterOrder) FilterResetOrder(object);
  /*
    fprintf(stderr, "<Filter Order obj=%s> List: ", ObjectName(object));
  */

  /* append classfilters registered for mixins */
  if (!(object->flags & NSF_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, object);

  if (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) {
    NsfCmdList *ml;
    NsfClass *mixin;

    for (ml = object->mixinOrder; ml; ml = ml->nextPtr) {
      mixin = NsfGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin && mixin->opt && mixin->opt->classfilters)
        FilterComputeOrderFullList(interp, &mixin->opt->classfilters, &filterList);
    }
  }

  /* append per-obj filters */
  if (object->opt)
    FilterComputeOrderFullList(interp, &object->opt->filters, &filterList);

  /* append per-class filters */
  for (pl = ComputeOrder(object->cl, object->cl->order, Super); pl; pl=pl->nextPtr) {
    NsfClassOpt *opt = pl->cl->opt;
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
FilterComputeDefined(Tcl_Interp *interp, NsfObject *object) {
  FilterComputeOrder(interp, object);
  object->flags |= NSF_FILTER_ORDER_VALID;
  if (object->filterOrder)
    object->flags |= NSF_FILTER_ORDER_DEFINED;
  else
    object->flags &= ~NSF_FILTER_ORDER_DEFINED;
}

/*
 * push a filter stack information on this object
 */
static int
FilterStackPush(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *calledProc) {
  register NsfFilterStack *h = NEW(NsfFilterStack);

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
FilterStackPop(NsfObject *object) {
  register NsfFilterStack *h = object->filterStack;
  object->filterStack = h->nextPtr;

  /* free stack entry */
  DECR_REF_COUNT(h->calledProc);
  FREE(NsfFilterStack, h);
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
FilterFindReg(Tcl_Interp *interp, NsfObject *object, Tcl_Command cmd) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  NsfClasses *pl;

  /* search per-object filters */
  if (object->opt && CmdListFindCmdInList(cmd, object->opt->filters)) {
    Tcl_ListObjAppendElement(interp, list, object->cmdName);
    Tcl_ListObjAppendElement(interp, list, NsfGlobalObjs[NSF_OBJECT]);
    Tcl_ListObjAppendElement(interp, list, NsfGlobalObjs[NSF_FILTER]);
    Tcl_ListObjAppendElement(interp, list,
                             Tcl_NewStringObj(Tcl_GetCommandName(interp, cmd), -1));
    return list;
  }

  /* search per-class filters */
  for (pl = ComputeOrder(object->cl, object->cl->order, Super); pl; pl = pl->nextPtr) {
    NsfClassOpt *opt = pl->cl->opt;
    if (opt && opt->classfilters) {
      if (CmdListFindCmdInList(cmd, opt->classfilters)) {
        Tcl_ListObjAppendElement(interp, list, pl->cl->object.cmdName);
        Tcl_ListObjAppendElement(interp, list, NsfGlobalObjs[NSF_FILTER]);
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
FilterSearchProc(Tcl_Interp *interp, NsfObject *object,
                 Tcl_Command *currentCmd, NsfClass **cl) {
  NsfCmdList *cmdList;

  assert(object);
  assert(object->filterStack);

  *currentCmd = NULL;

  /* Ensure that the filter order is not invalid, otherwise compute order
     FilterComputeDefined(interp, object);
  */
  assert(object->flags & NSF_FILTER_ORDER_VALID);
  cmdList = SeekCurrent(object->filterStack->currentCmdPtr, object->filterOrder);

  while (cmdList) {
    if (Tcl_Command_cmdEpoch(cmdList->cmdPtr)) {
      cmdList = cmdList->nextPtr;
    } else if (FilterActiveOnObj(interp, object, cmdList->cmdPtr)) {
      /* fprintf(stderr, "Filter <%s> -- Active on: %s\n",
         Tcl_GetCommandName(interp, (Tcl_Command)cmdList->cmdPtr), ObjectName(object));
      */
      object->filterStack->currentCmdPtr = cmdList->cmdPtr;
      cmdList = SeekCurrent(object->filterStack->currentCmdPtr, object->filterOrder);
    } else {
      /* ok. we found it */
      if (cmdList->clorobj && !NsfObjectIsClass(&cmdList->clorobj->object)) {
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
SuperclassAdd(Tcl_Interp *interp, NsfClass *cl, int oc, Tcl_Obj **ov, Tcl_Obj *arg, NsfClass *baseClass) {
  NsfClasses *filterCheck, *osl = NULL;
  NsfClass **scl;
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

  scl = NEW_ARRAY(NsfClass*, oc);
  for (i = 0; i < oc; i++) {
    if (GetClassFromObj(interp, ov[i], &scl[i], baseClass) != TCL_OK) {
      FREE(NsfClass**, scl);
      return NsfObjErrType(interp, "superclass", arg, "a list of classes", NULL);
    }
  }

  /*
   * check that superclasses don't precede their classes
   */

  for (i = 0; i < oc; i++) {
    if (reversed) break;
    for (j = i+1; j < oc; j++) {
      NsfClasses *dl = ComputeOrder(scl[j], scl[j]->order, Super);
      if (reversed) break;
      while (dl) {
	if (dl->cl == scl[i]) break;
	dl = dl->nextPtr;
      }
      if (dl) reversed = 1;
    }
  }

  if (reversed) {
    return NsfObjErrType(interp, "superclass", arg, "classes in dependence order", NULL);
  }

  while (cl->super) {
    /*
     * build up an old superclass list in case we need to revert
     */

    NsfClass *sc = cl->super->cl;
    NsfClasses *l = osl;
    osl = NEW(NsfClasses);
    osl->cl = sc;
    osl->nextPtr = l;
    (void)RemoveSuper(cl, cl->super->cl);
  }
  for (i=0; i < oc; i++) {
    AddSuper(cl, scl[i]);
  }
  FREE(NsfClass**, scl);
  FlushPrecedencesOnSubclasses(cl);

  if (!ComputeOrder(cl, cl->order, Super)) {

    /*
     * cycle in the superclass graph, backtrack
     */

    NsfClasses *l;
    while (cl->super) (void)RemoveSuper(cl, cl->super->cl);
    for (l = osl; l; l = l->nextPtr) AddSuper(cl, l->cl);
    NsfClassListFree(osl);
    return NsfObjErrType(interp, "superclass", arg, "a cycle-free graph", NULL);
  }
  NsfClassListFree(osl);

  /* if there are no more super classes add the Object
     class as superclasses */
  assert(cl->super);
#if 0
  if (cl->super == NULL) {
    fprintf(stderr, "SuperClassAdd super of '%s' is NULL\n", ClassName(cl));
    /*AddSuper(cl, RUNTIME_STATE(interp)->theObject);*/
  }
#endif

  Tcl_ResetResult(interp);
  return TCL_OK;
}

extern Tcl_Obj *
Nsf_ObjSetVar2(Nsf_Object *object, Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
                 Tcl_Obj *valueObj, int flgs) {
  Tcl_Obj *result;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, (NsfObject*)object, framePtr);
  if (((NsfObject*)object)->nsPtr) {
    flgs |= TCL_NAMESPACE_ONLY;
  }
  result = Tcl_ObjSetVar2(interp, name1, name2, valueObj, flgs);
  Nsf_PopFrameObj(interp, framePtr);
  return result;
}

extern Tcl_Obj *
Nsf_SetVar2Ex(Nsf_Object *object, Tcl_Interp *interp, CONST char *name1, CONST char *name2,
                Tcl_Obj *valueObj, int flgs) {
  Tcl_Obj *result;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, (NsfObject*)object, framePtr);
  if (((NsfObject*)object)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_SetVar2Ex(interp, name1, name2, valueObj, flgs);
  Nsf_PopFrameObj(interp, framePtr);
  return result;
}


extern Tcl_Obj *
NsfOSetInstVar(Nsf_Object *object, Tcl_Interp *interp,
		 Tcl_Obj *nameObj, Tcl_Obj *valueObj, int flgs) {
  return Nsf_ObjSetVar2(object, interp, nameObj, (Tcl_Obj *)NULL, valueObj, (flgs|TCL_PARSE_PART1));
}

extern Tcl_Obj *
Nsf_ObjGetVar2(Nsf_Object *object, Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
                 int flgs) {
  Tcl_Obj *result;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, (NsfObject*)object, framePtr);
  if (((NsfObject*)object)->nsPtr) {
    flgs |= TCL_NAMESPACE_ONLY;
  }
  result = Tcl_ObjGetVar2(interp, name1, name2, flgs);
  Nsf_PopFrameObj(interp, framePtr);

  return result;
}

extern Tcl_Obj *
Nsf_GetVar2Ex(Nsf_Object *object, Tcl_Interp *interp, CONST char *name1, CONST char *name2,
                int flgs) {
  Tcl_Obj *result;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, (NsfObject*)object, framePtr);
  if (((NsfObject*)object)->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_GetVar2Ex(interp, name1, name2, flgs);
  Nsf_PopFrameObj(interp, framePtr);
  return result;
}


Tcl_Obj *
NsfOGetInstVar(Nsf_Object *object, Tcl_Interp *interp, Tcl_Obj *nameObj, int flgs) {
  return Nsf_ObjGetVar2(object, interp, nameObj, (Tcl_Obj *)NULL, (flgs|TCL_PARSE_PART1));
}

int
NsfUnsetInstVar(Nsf_Object *object, Tcl_Interp *interp, CONST char *name, int flgs) {
  return NsfUnsetInstVar2(object, interp, name, NULL, flgs);
}


/*
 *----------------------------------------------------------------------
 * CheckVarName --
 *
 *    Check, whether the provided name is free of namepace markup.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    none
 *
 *----------------------------------------------------------------------
 */
static int
CheckVarName(Tcl_Interp *interp, const char *varNameString) {
  /*
   * We want to have a plain variable name, since we do not want to
   * get interferences with namespace resolver and such.  In an first
   * attempt, we disallowed occurances of "::", but we have to deal as
   * well with e.g. arrayName(::x::y)
   *
   * TODO: more general and efficient solution to disallow e.g. a::b
   * (check for :: until parens)
   */
  /*if (strstr(varNameString, "::") || *varNameString == ':') {*/
  if (*varNameString == ':') {
    return NsfPrintError(interp, "variable name \"%s\" must not contain "
			 "namespace separator or colon prefix",
			 varNameString);
  }
  return TCL_OK;
}

static int
VarExists(Tcl_Interp *interp, NsfObject *object, CONST char *varName, CONST char *index,
          int triggerTrace, int requireDefined) {
  CallFrame frame, *framePtr = &frame;
  Var *varPtr, *arrayPtr;
  int result;
  int flags = 0;

  flags = (index == NULL) ? TCL_PARSE_PART1 : 0;

  Nsf_PushFrameObj(interp, object, framePtr);

  if (triggerTrace)
    varPtr = TclVarTraceExists(interp, varName);
  else
    varPtr = TclLookupVar(interp, varName, index, flags, "access",
                          /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
  /*
    fprintf(stderr, "VarExists %s varPtr %p requireDefined %d, triggerTrace %d, isundef %d\n",
    varName,
    varPtr,
    requireDefined, triggerTrace,
    varPtr ? TclIsVarUndefined(varPtr) : NULL);
  */
  result = (varPtr && (!requireDefined || !TclIsVarUndefined(varPtr)));

  Nsf_PopFrameObj(interp, framePtr);

  return result;
}

static int
SubstValue(Tcl_Interp *interp, NsfObject *object, Tcl_Obj **value) {
  Tcl_Obj *ov[2];
  int result;

  ov[1] = *value;
  Tcl_ResetResult(interp);

  result = Nsf_SubstObjCmd(NULL, interp, 2, ov);

  /*fprintf(stderr, "+++++ %s.%s subst returned %d OK %d\n",
    ObjectName(object), varName, rc, TCL_OK);*/

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
  const char *procName;

  /*fprintf(stderr, "MakeProcError %p type %p refCount %d\n",
    procNameObj, procNameObj->typePtr, procNameObj->refCount);*/

  procName = Tcl_GetStringFromObj(procNameObj, &nameLen);
  overflow = (nameLen > limit);
  Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
						 "\n    (procedure \"%.*s%s\" line %d)",
						 (overflow ? limit : nameLen), procName,
						 (overflow ? "..." : ""), Tcl_GetErrorLine(interp)));
}

static int
ByteCompiled(Tcl_Interp *interp, unsigned short *cscFlagsPtr, Proc *procPtr, CONST char *body) {
  Tcl_Obj *bodyPtr = procPtr->bodyPtr;
  Namespace *nsPtr = procPtr->cmdPtr->nsPtr;
  int result;

  if (bodyPtr->typePtr == Nsf_OT_byteCodeType) {
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
    *cscFlagsPtr |= NSF_CSC_CALL_IS_COMPILE;
    result = TclProcCompileProc(interp, procPtr, bodyPtr,
                              (Namespace *) nsPtr, "body of proc",
                              body);
    *cscFlagsPtr &= ~NSF_CSC_CALL_IS_COMPILE;
    return result;
  }
}

/*
 *----------------------------------------------------------------------
 * PushProcCallFrame --
 *
 *    Set up and push a new call frame for the procedure invocation.
 *    callframe. The proc is passed via clientData.
 *
 * Results:
 *    Tcl result code
 *
 * Side effects:
 *    compiles body conditionally
 *
 *----------------------------------------------------------------------
 */
static int
PushProcCallFrame(ClientData clientData, register Tcl_Interp *interp,
		  int objc, Tcl_Obj *CONST objv[],
                  NsfCallStackContent *cscPtr) {
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

  /* TODO: we could use Tcl_PushCallFrame(), if we would allocate the tcl stack frame earlier */
  result = TclPushStackFrame(interp, (Tcl_CallFrame **)&framePtr,
			     (Tcl_Namespace *)  procPtr->cmdPtr->nsPtr,
			     (FRAME_IS_PROC|FRAME_IS_NSF_METHOD));

  if (result != TCL_OK) {
    return result;
  }

  framePtr->objc = objc;
  framePtr->objv = objv;
  framePtr->procPtr = procPtr;
  framePtr->clientData = cscPtr;
  return ByteCompiled(interp, &cscPtr->flags, procPtr, TclGetString(objv[0]));
}

static void
GetVarAndNameFromHash(Tcl_HashEntry *hPtr, Var **val, Tcl_Obj **varNameObj) {
  *val = VarHashGetValue(hPtr);
  *varNameObj = VarHashGetKey(*val);
}

void
NsfProcDeleteProc(ClientData clientData) {
  NsfProcContext *ctxPtr = (NsfProcContext *)clientData;
  (*ctxPtr->oldDeleteProc)(ctxPtr->oldDeleteData);
  if (ctxPtr->paramDefs) {
    /*fprintf(stderr, "free ParamDefs %p\n", ctxPtr->paramDefs);*/
    ParamDefsFree(ctxPtr->paramDefs);
  }
  /*fprintf(stderr, "free %p\n", ctxPtr);*/
  FREE(NsfProcContext, ctxPtr);
}

static NsfParam *
ParamsNew(int nr) {
  NsfParam *paramsPtr = NEW_ARRAY(NsfParam, nr+1);
  memset(paramsPtr, 0, sizeof(NsfParam)*(nr+1));
  return paramsPtr;
}

static void
ParamsFree(NsfParam *paramsPtr) {
  NsfParam *paramPtr;

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
  FREE(NsfParam*, paramsPtr);
}

static NsfParamDefs *
ParamDefsGet(Tcl_Command cmdPtr) {
  assert(cmdPtr);
  if (Tcl_Command_deleteProc(cmdPtr) == NsfProcDeleteProc) {
    return ((NsfProcContext *)Tcl_Command_deleteData(cmdPtr))->paramDefs;
  }
  return NULL;
}

static int
ParamDefsStore(Tcl_Interp *interp, Tcl_Command cmd, NsfParamDefs *paramDefs) {
  Command *cmdPtr = (Command *)cmd;

  if (cmdPtr->deleteProc != NsfProcDeleteProc) {
    NsfProcContext *ctxPtr = NEW(NsfProcContext);

    /*fprintf(stderr, "paramDefsStore replace deleteProc %p by %p\n",
      cmdPtr->deleteProc, NsfProcDeleteProc);*/

    ctxPtr->oldDeleteData = (Proc *)cmdPtr->deleteData;
    ctxPtr->oldDeleteProc = cmdPtr->deleteProc;
    cmdPtr->deleteProc = NsfProcDeleteProc;
    ctxPtr->paramDefs = paramDefs;
    cmdPtr->deleteData = (ClientData)ctxPtr;
    return TCL_OK;
  } else {
    /*fprintf(stderr, "paramDefsStore cmd %p has already NsfProcDeleteProc deleteData %p\n",
      cmd, cmdPtr->deleteData);*/
    if (cmdPtr->deleteData) {
      NsfProcContext *ctxPtr = cmdPtr->deleteData;
      assert(ctxPtr->paramDefs == NULL);
      ctxPtr->paramDefs = paramDefs;
    }
  }
  return TCL_ERROR;
}

static NsfParamDefs *
ParamDefsNew() {
  NsfParamDefs *paramDefs;

  paramDefs = NEW(NsfParamDefs);
  memset(paramDefs, 0, sizeof(NsfParamDefs));
  /*fprintf(stderr, "ParamDefsNew %p\n", paramDefs);*/

  return paramDefs;
}


static void
ParamDefsFree(NsfParamDefs *paramDefs) {
  /*fprintf(stderr, "ParamDefsFree %p returns %p\n", paramDefs, paramDefs->returns);*/

  if (paramDefs->paramsPtr) {
    ParamsFree(paramDefs->paramsPtr);
  }
  if (paramDefs->slotObj) {DECR_REF_COUNT(paramDefs->slotObj);}
  if (paramDefs->returns) {DECR_REF_COUNT(paramDefs->returns);}
  FREE(NsfParamDefs, paramDefs);
}

/*
 * Non Positional Parameter
 */

static void
ParamDefsFormatOption(Tcl_Interp *interp, Tcl_Obj *nameStringObj, CONST char *option,
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

static Tcl_Obj *
ParamDefsFormat(Tcl_Interp *interp, NsfParam CONST *paramsPtr) {
  int first, colonWritten;
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL), *innerListObj, *nameStringObj;
  NsfParam CONST *pPtr;

  for (pPtr = paramsPtr; pPtr->name; pPtr++) {
    if (pPtr -> paramObj) {
      innerListObj = pPtr->paramObj;
    } else {
      /* We need this part only for C-defined parameter definitions,
         defined via genTclAPI.

         TODO: we could streamline this by defining as well C-API via
         the same syntax as for accepted for tcl obj types
         "nsfParam"
      */
      int isNonpos = *pPtr->name == '-';
      int outputRequired = (isNonpos && (pPtr->flags & NSF_ARG_REQUIRED));
      int outputOptional = (!isNonpos && !(pPtr->flags & NSF_ARG_REQUIRED)
                            && !pPtr->defaultValue &&
                            pPtr->converter != ConvertToNothing);
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
      if ((pPtr->flags & NSF_ARG_SUBST_DEFAULT)) {
        ParamDefsFormatOption(interp, nameStringObj, "substdefault", &colonWritten, &first);
      }
      if ((pPtr->flags & NSF_ARG_ALLOW_EMPTY) || (pPtr->flags & NSF_ARG_MULTIVALUED)) {
	char option[10] = "....";
	option[0] = (pPtr->flags & NSF_ARG_ALLOW_EMPTY) ? '0' : '1';
	option[3] = (pPtr->flags & NSF_ARG_MULTIVALUED) ? '*' : '1';
        ParamDefsFormatOption(interp, nameStringObj, option, &colonWritten, &first);
      }
      if ((pPtr->flags & NSF_ARG_IS_CONVERTER)) {
        ParamDefsFormatOption(interp, nameStringObj, "convert", &colonWritten, &first);
      }
      if ((pPtr->flags & NSF_ARG_INITCMD)) {
        ParamDefsFormatOption(interp, nameStringObj, "initcmd", &colonWritten, &first);
      } else if ((pPtr->flags & NSF_ARG_METHOD)) {
        ParamDefsFormatOption(interp, nameStringObj, "method", &colonWritten, &first);
      } else if ((pPtr->flags & NSF_ARG_NOARG)) {
        ParamDefsFormatOption(interp, nameStringObj, "noarg", &colonWritten, &first);
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
ParamDefsList(Tcl_Interp *interp, NsfParam CONST *paramsPtr) {
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
  NsfParam CONST *pPtr;

  for (pPtr = paramsPtr; pPtr->name; pPtr++) {
    Tcl_ListObjAppendElement(interp, listObj, pPtr->nameObj);
  }
  return listObj;
}

static CONST char *
ParamGetType(NsfParam CONST *paramPtr) {
  CONST char *result = "value";

  assert(paramPtr);
  if (paramPtr->type) {
    if (paramPtr->converter == ConvertViaCmd) {
      result = paramPtr->type + 5;
    } else if (paramPtr->converter == ConvertToClass && 
	       (paramPtr->flags & (NSF_ARG_BASECLASS|NSF_ARG_METACLASS)) ) {
      if (paramPtr->flags & NSF_ARG_BASECLASS) {
	result = "baseclass";
      } else {
	result = "metaclass";
      }
    } else if (strcmp(paramPtr->type, "stringtype") == 0) {
      if (paramPtr->converterArg) {
	result = ObjStr(paramPtr->converterArg);
      }
    } else {
      result = paramPtr->type;
    }
  }
  
  return result;
}

static CONST char *
ParamGetDomain(NsfParam CONST *paramPtr) {
  CONST char *result = "value";

  assert(paramPtr);
  if ((paramPtr->flags & NSF_ARG_IS_ENUMERATION)) {
    enumeratorConverterEntry *ePtr;
    for (ePtr = &enumeratorConverterEntries[0]; ePtr->converter; ePtr++) {
      if (ePtr->converter == paramPtr->converter) {
	result = ePtr->domain;
	break;
      }
    }
  } else {
    result = ParamGetType(paramPtr);
  }
  return result;
}

static Tcl_Obj *
ParamDefsSyntax(Tcl_Interp *interp, NsfParam CONST *paramPtr) {
  Tcl_Obj *argStringObj = Tcl_NewStringObj("", 0);
  NsfParam CONST *pPtr;

  for (pPtr = paramPtr; pPtr->name; pPtr++) {
    if (pPtr != paramPtr) {
      Tcl_AppendLimitedToObj(argStringObj, " ", 1, INT_MAX, NULL);
    }
    if (pPtr->converter == ConvertToNothing && strcmp(pPtr->name, "args") == 0) {
      Tcl_AppendLimitedToObj(argStringObj, "?arg ...?", 9, INT_MAX, NULL);
    } else if (pPtr->flags & NSF_ARG_REQUIRED) {
      if ((pPtr->flags & NSF_ARG_IS_ENUMERATION)) {
	Tcl_AppendLimitedToObj(argStringObj, ParamGetDomain(pPtr), -1, INT_MAX, NULL);
      } else {
	Tcl_AppendLimitedToObj(argStringObj, pPtr->name, -1, INT_MAX, NULL);
      }
    } else {
      Tcl_AppendLimitedToObj(argStringObj, "?", 1, INT_MAX, NULL);
      Tcl_AppendLimitedToObj(argStringObj, pPtr->name, -1, INT_MAX, NULL);
      if (pPtr->nrArgs >0) {
	Tcl_AppendLimitedToObj(argStringObj, " ", 1, INT_MAX, NULL);
	Tcl_AppendLimitedToObj(argStringObj, ParamGetDomain(pPtr), -1, INT_MAX, NULL);
	if (pPtr->flags & NSF_ARG_MULTIVALUED) {
	  Tcl_AppendLimitedToObj(argStringObj, " ...", 4, INT_MAX, NULL);	
	}
      }
      Tcl_AppendLimitedToObj(argStringObj, "?", 1, INT_MAX, NULL);
    }
  }
  /* caller has to decr */
  return argStringObj;
}

static void
ParsedParamFree(NsfParsedParam *parsedParamPtr) {
  /*fprintf(stderr, "ParsedParamFree %p, npargs %p\n", parsedParamPtr, parsedParamPtr->paramDefs);*/
  if (parsedParamPtr->paramDefs) {
    ParamDefsFree(parsedParamPtr->paramDefs);
  }
  FREE(NsfParsedParam, parsedParamPtr);
}



/*
 * method dispatch
 */
/*
 *----------------------------------------------------------------------
 * ProcMethodDispatchFinalize --
 *
 *    Finalization function for ProcMethodDispatch which executes
 *    scripted methods. Essentially it handles post-assertions and
 *    frees per-invocation memory. The function was developed for NRE
 *    enabled Tcl versions but is used in the same way for non-NRE
 *    enabled versions.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    indirect effects by calling Tcl code
 *
 *----------------------------------------------------------------------
 */
static int
ProcMethodDispatchFinalize(ClientData data[], Tcl_Interp *interp, int result) {
  ParseContext *pcPtr = data[0];
  NsfCallStackContent *cscPtr = data[1];
  /*CONST char *methodName = data[2];*/
  NsfObject *object = cscPtr->self;
  NsfObjectOpt *opt = object->opt;
  int rc;

  /*fprintf(stderr, "ProcMethodDispatchFinalize %s.%s flags %.6x isNRE %d\n",
    ObjectName(object), methodName
    cscPtr->flags, (cscPtr->flags & NSF_CSC_CALL_IS_NRE));*/

  if (opt && object->teardown && (opt->checkoptions & CHECK_POST)) {
    /*
     * Even, when the returned result != TCL_OK, run assertion to report
     * the highest possible method from the callstack (e.g. "set" would not
     * be very meaningful; however, do not flush a TCL_ERROR.
     */
    rc = AssertionCheck(interp, object, cscPtr->cl, data[2], CHECK_POST);
    if (result == TCL_OK) {
      result = rc;
    }
  }

  if ((cscPtr->flags & NSF_CSC_CALL_IS_NRE)) {
    if (pcPtr) {
      ParseContextRelease(pcPtr);
      NsfTclStackFree(interp, pcPtr, "release parse context");
    }
#if defined(NRE)
    result = ObjectDispatchFinalize(interp, cscPtr, result /*, "NRE" , methodName*/);
#endif

    CscFinish(interp, cscPtr, "scripted finalize");
  }

  return result;
}

/*
 *----------------------------------------------------------------------
 * ProcMethodDispatch --
 *
 *    Invoke a scripted method (with assertion checking and filters).
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Indirect effects by calling Tcl code
 *
 *----------------------------------------------------------------------
 */
static int
ProcMethodDispatch(ClientData cp, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
         CONST char *methodName, NsfObject *object, NsfClass *cl, Tcl_Command cmdPtr,
         NsfCallStackContent *cscPtr) {
  int result, releasePc = 0;
  NsfObjectOpt *opt = object->opt;
  NsfParamDefs *paramDefs;
#if defined(NRE)
  ParseContext *pcPtr = NULL;
#else
  ParseContext pc, *pcPtr = &pc;
#endif

  assert(object);
  assert(object->teardown);
#if defined(NRE)
  assert(cscPtr->flags & NSF_CSC_CALL_IS_NRE);
#endif

  /*
   * if this is a filter, check whether its guard applies,
   * if not: just step forward to the next filter
   */

  if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER) {
    NsfCmdList *cmdList;
    /*
     * seek cmd in obj's filterOrder
     */
    assert(object->flags & NSF_FILTER_ORDER_VALID);
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
           * The guard failed (but no error), and we call "next".
           * Since we may not be in a method with already provided
           * arguments, we call next with the actual arguments and
           * perform no argument substitution.
           *
           * The call stack content is not jet pushed to the Tcl
           * stack, we pass it already to search-and-invoke.
           */

          /*fprintf(stderr, "... calling nextmethod cscPtr %p\n", cscPtr);*/
	  result = NextSearchAndInvoke(interp, methodName, objc, objv, cscPtr, 0);
          /*fprintf(stderr, "... after nextmethod result %d\n", result);*/
        }

	/*
	 * Next might have succeeded or not, but we are done. In the
	 * NRE-case, we need a CscFinish for all return codes.
	 */
#if defined(NRE)
	//CscListRemove(interp, cscPtr);
	CscFinish(interp, cscPtr, "guard failed");
#endif
	return result;
      }
    }
  }

  if (opt && (opt->checkoptions & CHECK_PRE) &&
      (result = AssertionCheck(interp, object, cl, methodName, CHECK_PRE)) == TCL_ERROR) {
    goto prep_done;
  }

  /*
   *  If the method to be invoked has paramDefs, we have to call the
   *  argument parser with the argument definitions obtained from the
   *  proc context from the cmdPtr.
  */
  paramDefs = ParamDefsGet(cmdPtr);

  /*Tcl_Command_deleteProc(cmdPtr) == NsfProcDeleteProc ?
    ((NsfProcContext *)Tcl_Command_deleteData(cmdPtr))->paramDefs : NULL;*/

  if (paramDefs && paramDefs->paramsPtr) {
#if defined(NRE)
    pcPtr = (ParseContext *) NsfTclStackAlloc(interp, sizeof(ParseContext), "parse context");
#endif
    result = ProcessMethodArguments(pcPtr, interp, object, 1, paramDefs, methodName, objc, objv);
    cscPtr->objc = objc;
    cscPtr->objv = (Tcl_Obj **)objv;

    if (result == TCL_OK) {
      releasePc = 1;
      result = PushProcCallFrame(cp, interp, pcPtr->objc, pcPtr->full_objv, cscPtr);
    } else {
#if defined(NRE)
      NsfTclStackFree(interp, pcPtr, "parse context (proc prep failed)");
      pcPtr = NULL;
#endif
    }
  } else {
    result = PushProcCallFrame(cp, interp, objc, objv, cscPtr);
  }

  /*
   * The stack frame is pushed, we could do something here before
   * running the byte code of the body.
   */

  /* we could consider to run here ARG_METHOD or ARG_INITCMD
  if (result == TCL_OK) {

  }
  */

 prep_done:

  if (result == TCL_OK) {
#if defined(NRE)
    /*fprintf(stderr, "CALL TclNRInterpProcCore %s method '%s'\n",
      ObjectName(object), ObjStr(objv[0]));*/
    Tcl_NRAddCallback(interp, ProcMethodDispatchFinalize,
		      releasePc ? pcPtr : NULL, cscPtr, (ClientData)methodName, NULL);
    cscPtr->flags |= NSF_CSC_CALL_IS_NRE;
    result = TclNRInterpProcCore(interp, objv[0], 1, &MakeProcError);
#else
    ClientData data[3] = {
	releasePc ? pcPtr : NULL,
	cscPtr,
	(ClientData)methodName
    };
    result = TclObjInterpProcCore(interp, objv[0], 1, &MakeProcError);
    result = ProcMethodDispatchFinalize(data, interp, result);
#endif
  } else /* result != OK */ {
#if defined(NRE)
    CscFinish(interp, cscPtr, "nre, prep failed");
#endif
  }

  return result;
}

/*
 *----------------------------------------------------------------------
 * CmdMethodDispatch --
 *
 *    Invoke a method implemented as a cmd.  Essentially it stacks
 *    optionally a frame, calls the method, pops the frame and runs
 *    invariants.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Indirect effects by calling cmd
 *
 *----------------------------------------------------------------------
 */
static int
CmdMethodDispatch(ClientData cp, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
        CONST char *methodName, NsfObject *object, Tcl_Command cmdPtr,
        NsfCallStackContent *cscPtr) {
  CallFrame frame, *framePtr = &frame;
  CheckOptions co;
  int result;

  assert(object);
  assert(object->teardown);
#if defined(NRE)
  assert(!cscPtr || (cscPtr->flags & NSF_CSC_CALL_IS_NRE) == 0);
#endif

  if (cscPtr) {
    /*
     * We have a call stack content, but the following dispatch will
     * by itself not stack it; in order to get e.g. self working, we
     * have to stack at least an FRAME_IS_NSF_OBJECT.
     */
    /*fprintf(stderr, "Nsf_PushFrameCsc %s %s\n",ObjectName(object), methodName);*/
    Nsf_PushFrameCsc(interp, cscPtr, framePtr);
  }

  /*fprintf(stderr, "CmdMethodDispatch obj %p %p %s\n", obj, methodName, methodName);*/
  result = Tcl_NRCallObjProc(interp, Tcl_Command_objProc(cmdPtr), cp, objc, objv);

  if (cscPtr) {
    Nsf_PopFrameCsc(interp, framePtr);
  }

  /*
   * Reference counting in the calling ObjectDispatch() makes sure
   * that obj->opt is still accessible even after "dealloc"
   */
  if (object->opt) {
    co = object->opt->checkoptions;
    if ((co & CHECK_INVAR)) {
      result = AssertionCheckInvars(interp, object, methodName, co);
    }
  }

  return result;
}

#if defined(NSF_PROFILE)
/* TODO: should be adjusted to MethodDispatchCsc() */
static int
MethodDispatch(ClientData clientData, Tcl_Interp *interp,
             int objc, Tcl_Obj *CONST objv[], Tcl_Command cmd, NsfObject *object, NsfClass *cl,
             CONST char *methodName, int frameType) {
  struct timeval trt;
  long int startUsec = (gettimeofday(&trt, NULL), trt.tv_usec), startSec = trt.tv_sec;

  result = __MethodDispatch__(clientData, interp, objc, objv, cmd, object, cl, methodName, frameType);
  NsfProfileEvaluateData(interp, startSec, startUsec, object, cl, methodName);
  return result;
}
# define MethodDispatch __MethodDispatch__
#endif

/*
 *----------------------------------------------------------------------
 * MethodDispatchCsc --
 *
 *    Dispatch a method (scripted or cmd) with an already allocated
 *    call stack content. The method calls either ProcMethodDispatch()
 *    (for scripted methods) or CmdMethodDispatch() (otherwise).
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Indirect effects by calling methods
 *
 *----------------------------------------------------------------------
 */
static int
MethodDispatchCsc(ClientData clientData, Tcl_Interp *interp,
		  int objc, Tcl_Obj *CONST objv[],
		  NsfCallStackContent *cscPtr, CONST char *methodName) {
  Tcl_Command cmd = cscPtr->cmdPtr;
  NsfObject *object = cscPtr->self;
  ClientData cp = Tcl_Command_objClientData(cmd);
  Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);
  int result;

  /*fprintf(stderr, "MethodDispatch method '%s' cmd %p cp=%p objc=%d\n", 
    methodName, cmd, cp, objc);*/
  assert(object->teardown);

  if (proc == TclObjInterpProc) {
#if defined(NRE)
    TEOV_callback *rootPtr = TOP_CB(interp);
#endif
    /*
     * The cmd is a scripted method
     */

    result = ProcMethodDispatch(cp, interp, objc, objv, methodName,
				  object, cscPtr->cl, cmd, cscPtr);

#if defined(NRE)
    if ((cscPtr->flags & NSF_CSC_IMMEDIATE)) {
# if defined(NRE_CALLBACK_TRACE)
      fprintf(stderr, ".... manual run callbacks rootPtr = %p, result %d methodName %s.%s\n",
	      rootPtr, result, cscPtr->cl?ClassName(cscPtr->cl):"NULL", methodName);
# endif	
      result = NsfNRRunCallbacks(interp, result, rootPtr);
    } else {
# if defined(NRE_CALLBACK_TRACE)
      fprintf(stderr, ".... don't run callbacks rootPtr = %p, result %d methodName %s.%s\n",
	      rootPtr, result, cscPtr->cl?ClassName(cscPtr->cl):"NULL", methodName);
# endif
    }
#endif
    /*
     * scriped cmd done
     */
    return result;

  } else if (cp 
	     || (Tcl_Command_flags(cmd) & NSF_CMD_NONLEAF_METHOD) 
	     || (cscPtr->flags & NSF_CSC_FORCE_FRAME)) {
    /*
     * The cmd has client data or we force the frame either via
     * cmd-flag or csc-flag
     */
    if (proc == NsfObjDispatch) {
      /*
       * invoke an aliased object (ensemble object) via method interface
       */
      NsfRuntimeState *rst = RUNTIME_STATE(interp);
      NsfObject *invokeObj = (NsfObject *)cp;

      if (invokeObj->flags & NSF_DELETED) {
        /*
         * When we try to call a deleted object, the cmd (alias) is
         * automatically removed.
         */
        Tcl_DeleteCommandFromToken(interp, cmd);
        NsfCleanupObject(invokeObj, "alias-delete1");
        return NsfPrintError(interp, "Trying to dispatch deleted object via method '%s'",
                              methodName);
      }

      /*
       * The client data cp is still the obj of the called method
       */
      /*fprintf(stderr, "ensemble dispatch %s objc %d\n", methodName, objc);*/
      if (objc < 2) {
	CallFrame frame, *framePtr = &frame;
	Nsf_PushFrameCsc(interp, cscPtr, framePtr);
	result = DispatchDefaultMethod(cp, interp, objc, objv, NSF_CSC_IMMEDIATE);
	Nsf_PopFrameCsc(interp, framePtr);
      } else {
	CallFrame frame, *framePtr = &frame;
	NsfObject *self = (NsfObject *)cp;
	char *methodName = ObjStr(objv[1]);

	cscPtr->objc = objc;
	cscPtr->objv = objv;
	cscPtr->flags |= NSF_CSC_CALL_IS_ENSEMBLE;
	Nsf_PushFrameCsc(interp, cscPtr, framePtr);

	if (self->nsPtr) {
	  cmd = FindMethod(self->nsPtr, methodName);
	  /*fprintf(stderr, "... method %p %s csc %p\n", cmd, methodName, cscPtr); */
	  if (cmd) {
	    /*
	     * In order to allow next to be called on the
	     * ensemble-method, a call-frame entry is needed. The
	     * associated calltype is flagged as an ensemble to be
	     * able to distinguish frames during next.
	     *
	     * The invocation requires NSF_CSC_IMMEDIATE to ensure,
	     * that scripted methods are executed before the ensemble
	     * ends. If they would be exeecuted lated, their parent
	     * frame (CMETHOD) would have disappeared from the stack
	     * already.
	     */

	    /*fprintf(stderr, ".... ensemble dispatch on %s.%s csc %p\n",
	      ObjectName(object),methodName, cscPtr);*/
	    result = MethodDispatch(object, interp, objc-1, objv+1,
				    cmd, object, NULL, methodName,
				    cscPtr->frameType|NSF_CSC_TYPE_ENSEMBLE,
				    cscPtr->flags|NSF_CSC_IMMEDIATE);
	    goto obj_dispatch_ok;
	  }
	}

	/*
	 * The method to be called was not part of this ensemble. Call
	 * next to try to call such methods along the next path.
	 */
	/*fprintf(stderr, "call next instead of unknown %s.%s \n",
	  ObjectName(cscPtr->self),methodName);*/
	{
	  Tcl_CallFrame *framePtr1;
	  NsfCallStackContent *cscPtr1 = CallStackGetTopFrame(interp, &framePtr1);

	  if ((cscPtr1->frameType & NSF_CSC_TYPE_ENSEMBLE)) {
	    /*
	     * We are in an ensemble method. The next works here not on the
	     * actual methodName + frame, but on the ensemble above it. We
	     * locate the appropriate callstack content and continue next on
	     * that.
	     */
	    cscPtr1 = CallStackFindEnsembleCsc(framePtr1, &framePtr1);
	    assert(cscPtr1);
	  }

	  /*
	   * The method name for next might be colon-prefixed. In
	   * these cases, we have to skip the single colon.
	   */
	  result = NextSearchAndInvoke(interp, MethodName(cscPtr1->objv[0]),
				       cscPtr1->objc, cscPtr1->objv, cscPtr1, 0);
	}
	
	/*fprintf(stderr, "==> next %s.%s (obj %s) csc %p returned %d unknown %d\n",
	  ObjectName(self),methodName, ObjectName(object), cscPtr, result, rst->unknown);*/
	if (rst->unknown) {
	  /*
	   * The appropriate unknown class is registered on the
	   * EnsembleObject class, from where it is currently not
	   * possible to determine the true calling object. Therefore,
	   * we pass the object as first argument of the unknown
	   * handler.
	   */

	  result = DispatchUnknownMethod(self, interp, objc, objv, object,
					 objv[1], NSF_CM_NO_OBJECT_METHOD|NSF_CSC_IMMEDIATE);
	}
      obj_dispatch_ok:
	Nsf_PopFrameCsc(interp, framePtr);

      }
      return result;

    } else if (proc == NsfForwardMethod ||
	       proc == NsfObjscopedMethod ||
	       proc == NsfSetterMethod
               ) {
      TclCmdClientData *tcd = (TclCmdClientData *)cp;
      tcd->object = object;
      assert((CmdIsProc(cmd) == 0));
    } else if (proc == NsfProcAliasMethod) {
      TclCmdClientData *tcd = (TclCmdClientData *)cp;
      tcd->object = object;
      assert((CmdIsProc(cmd) == 0));
      cscPtr->flags |= NSF_CSC_CALL_IS_TRANSPARENT;
    } else if (cp == (ClientData)NSF_CMD_NONLEAF_METHOD) {
      cp = clientData;
      assert((CmdIsProc(cmd) == 0));
    }

  } else {
    /*
     * The cmd has no client data. In these situations, no stack frame
     * is needed. Dispatch the method without the cscPtr, such
     * CmdMethodDispatch () does not stack a frame.
     */

    CscListAdd(interp, cscPtr);

    /*fprintf(stderr, "cmdMethodDispatch %s.%s, nothing stacked, objflags %.6x\n",
      ObjectName(object), methodName, object->flags); */

    return CmdMethodDispatch(clientData, interp, objc, objv, methodName, object, cmd, NULL);
  }

  return CmdMethodDispatch(cp, interp, objc, objv, methodName, object, cmd, cscPtr);
}

/*
 *----------------------------------------------------------------------
 * MethodDispatch --
 *
 *    Conveniance wrapper for MethodDispatchCsc(). It allocates a call
 *    stack content and invokes MethodDispatchCsc.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Indirect effects by calling methods
 *
 *----------------------------------------------------------------------
 */
static int
MethodDispatch(ClientData clientData, Tcl_Interp *interp,
	       int objc, Tcl_Obj *CONST objv[],
	       Tcl_Command cmd, NsfObject *object, NsfClass *cl,
	       CONST char *methodName, int frameType, int flags) {
  NsfCallStackContent csc, *cscPtr;		
  int result;

  assert (object->teardown);
  assert (cmd);

  /* fprintf(stderr, "MethodDispatch method '%s.%s' objc %d flags %.6x call %d\n",
     ObjectName(object),methodName, objc, flags, call); */

  cscPtr = CscAlloc(interp, &csc, cmd);

  /*
   * We would not need CscInit when
   * cp == NULL && !(Tcl_Command_flags(cmd) & NSF_CMD_NONLEAF_METHOD)
   * TODO: We could pass cmd==NULL, but is this worth it?
   */
  CscInit(cscPtr, object, cl, cmd, frameType, flags);

  result = MethodDispatchCsc(clientData, interp, objc, objv,
			     cscPtr, methodName);

#if defined(NRE)
  if ((cscPtr->flags & NSF_CSC_CALL_IS_NRE) == 0) {
    CscListRemove(interp, cscPtr);
    CscFinish(interp, cscPtr, "csc cleanup");
  }
#else
  CscListRemove(interp, cscPtr);
  CscFinish(interp, cscPtr, "csc cleanup");
#endif

  return result;
}

/*
 *----------------------------------------------------------------------
 * ObjectDispatchFinalize --
 *
 *    Finalization function for ObjectDispatch() which performs method
 *    lookup and call all kind of methods. The function runs after
 *    ObjectDispatch() and calls the unknown handler if necessary and
 *    resets the filter and mixin stacks.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Maybe side effects by the cmd called by ParameterCheck()
 *    or DispatchUnknownMethod()
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static int
ObjectDispatchFinalize(Tcl_Interp *interp, NsfCallStackContent *cscPtr,
		       int result /*, char *msg, CONST char *methodName*/) {
  NsfRuntimeState *rst = RUNTIME_STATE(interp);
  NsfObject *object;
  int flags;

  assert(cscPtr);
  object = cscPtr->self;
  flags = cscPtr->flags;

  assert(object);
  assert(object->id);

  /*fprintf(stderr, "ObjectDispatchFinalize %p %s.%s flags %.6x (%d) frame %.6x rst %d %s\n",
	  cscPtr, ObjectName(object), methodName, flags,
	  result, cscPtr->frameType, RUNTIME_STATE(interp)->unknown,
	  msg);*/

  /*
   * When the active command is deleted, the cmdPtr in the call stack
   * content structure is set to NULL. We are not able to check
   * parameter in such situations.
   */
  if (result == TCL_OK && cscPtr->cmdPtr) {
    NsfParamDefs *paramDefs = ParamDefsGet(cscPtr->cmdPtr);

    if (paramDefs && paramDefs->returns) {
      Tcl_Obj *valueObj = Tcl_GetObjResult(interp);
      result = ParameterCheck(interp, paramDefs->returns, valueObj, "return-value:",
			      rst->doCheckResults, NULL);
    }
  } else {
    /*fprintf(stderr, "We have no cmdPtr in cscPtr %p %s.%s",  cscPtr, ObjectName(object), methodName);
      fprintf(stderr, "... cannot check return values!\n");*/
  }


  /*
   * On success (no error occured) check for unknown cases.
   */
  if (result == TCL_OK) {

    if ((flags & NSF_CSC_UNKNOWN)
	|| ((cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER) && rst->unknown)
	) {
      result = DispatchUnknownMethod(object, interp,
				     cscPtr->objc, cscPtr->objv, NULL, cscPtr->objv[0],
				     NSF_CSC_IMMEDIATE
				     /*flags&NSF_CSC_IMMEDIATE*/);
      /*
       * Final reset of unknown flag
       */
      rst->unknown = 0;
    }
  }

  /*
   * Resetting mixin and filter stacks
   */
  if ((flags & NSF_CSC_MIXIN_STACK_PUSHED) && object->mixinStack) {
    /*fprintf(stderr, "MixinStackPop %s.%s %p %s\n",
      ObjectName(object),methodName, object->mixinStack, msg);*/
    MixinStackPop(object);
  }
  if ((flags & NSF_CSC_FILTER_STACK_PUSHED) && object->filterStack) {
    /* fprintf(stderr, "FilterStackPop %s.%s %p %s\n",
       ObjectName(object),methodName, object->filterStack, msg);*/
    FilterStackPop(object);
  }

  return result;
}

/*
 *----------------------------------------------------------------------
 * ObjectDispatch --
 *
 *    This function performs the method lookup and call all kind of
 *    methods. It checks, whether a filter or mixin has to be
 *    applied. in these cases, the effective method lookup is
 *    performed by next.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Maybe side effects by the cmd called by ParameterCheck()
 *    or DispatchUnknownMethod()
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static int
ObjectDispatch(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *CONST objv[], int flags) {
  register NsfObject *object = (NsfObject*)clientData;
  int result = TCL_OK, objflags, shift,
    frameType = NSF_CSC_TYPE_PLAIN;
  CONST char *methodName;
  NsfClass *cl = NULL;
  Tcl_Command cmd = NULL;
  Tcl_Obj *cmdName = object->cmdName, *methodObj, *cmdObj;
  NsfCallStackContent csc, *cscPtr = NULL;

  if (flags & NSF_CM_NO_SHIFT) {
    shift = 0;
    cmdObj = cmdName;
    methodObj = objv[0];
    methodName = MethodName(methodObj);
  } else {
    assert(objc>1);
    shift = 1;
    cmdObj = objv[0];
    methodObj = objv[1];
    methodName =  ObjStr(methodObj);
    if (FOR_COLON_RESOLVER(methodName)) {
      return NsfPrintError(interp, "%s: methodname '%s' must not start with a colon", 
			   ObjectName(object), methodName); 
    }
  }

  /* none of the higher copy-flags must be passed */
  assert((flags & (NSF_CSC_COPY_FLAGS & 0xFF00)) == 0);

  /*fprintf(stderr, "ObjectDispatch obj = %s objc = %d 0=%s methodName=%s\n",
    ObjectName(object), objc, ObjStr(cmdObj), methodName);*/

  objflags = object->flags; /* avoid stalling */

  /*
   * Make sure, cmdName and obj survive this method until the end of
   * this function.
   */
  INCR_REF_COUNT(cmdName);
  object->refCount ++;

  /*fprintf(stderr, "obj refCount of %p after incr %d (ObjectDispatch) %s\n",
    object,object->refCount, methodName);*/

  if (!(objflags & NSF_FILTER_ORDER_VALID)) {
    FilterComputeDefined(interp, object);
    objflags = object->flags;
  }

  if (!(objflags & NSF_MIXIN_ORDER_VALID)) {
    MixinComputeDefined(interp, object);
    objflags = object->flags;
  }

  /* Only start new filter chain, if
     (a) filters are defined and
     (b) the toplevel csc entry is not an filter on self
  */

  /*fprintf(stderr, "call %s, objflags %.6x, defined and valid %.6x doFilters %d guard count %d\n",
          methodName, objflags, NSF_FILTER_ORDER_DEFINED_AND_VALID,
          rst->doFilters, rst->guardCount);*/

  assert((flags & (NSF_CSC_MIXIN_STACK_PUSHED|NSF_CSC_FILTER_STACK_PUSHED)) == 0);

  if (((objflags & NSF_FILTER_ORDER_DEFINED_AND_VALID) == NSF_FILTER_ORDER_DEFINED_AND_VALID)) {
    NsfRuntimeState *rst = RUNTIME_STATE(interp);
    if (rst->doFilters && !rst->guardCount) {
      NsfCallStackContent *cscPtr1 = CallStackGetTopFrame(interp, NULL);
      
      if (!cscPtr1 ||
	  (object != cscPtr1->self || (cscPtr1->frameType != NSF_CSC_TYPE_ACTIVE_FILTER))) {
	// TODO could use methodObj sometimes, therefore the new obj is not needed always
	// use MethodName() for the few occurances, where ->calledProc is referenced.
	//FilterStackPush(interp, object, Tcl_NewStringObj(methodName,-1));
	FilterStackPush(interp, object, methodObj);
	flags |= NSF_CSC_FILTER_STACK_PUSHED;

	cmd = FilterSearchProc(interp, object, &object->filterStack->currentCmdPtr, &cl);
	if (cmd) {
	  /*fprintf(stderr, "filterSearchProc returned cmd %p\n", cmd);*/
	  frameType = NSF_CSC_TYPE_ACTIVE_FILTER;
	  methodName = (char *)Tcl_GetCommandName(interp, cmd);
	}
      }
    }
  }

  /*
   * Check if a mixed in method has to be called.
   */
  if ((objflags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) == NSF_MIXIN_ORDER_DEFINED_AND_VALID) {

    /*
     * The current logic allocates first an entry on the per-object
     * stack and searches then for a mixin. This could be improved by
     * allocating a stack entry just when an mixin is found. The same
     * holds for the filters above, but there, the hit-rate is much
     * larger.
     */

    MixinStackPush(object);
    flags |= NSF_CSC_MIXIN_STACK_PUSHED;

    if (frameType != NSF_CSC_TYPE_ACTIVE_FILTER) {
      /*
       * The entry is just searched and pushed on the stack when we
       * have no filter; in the filter case, the search happens in
       * next
       */
      result = MixinSearchProc(interp, object, methodName, &cl,
                               &object->mixinStack->currentCmdPtr, &cmd);
      if (result != TCL_OK) {
	/*fprintf(stderr, "mixinsearch returned an error for %p %s.%s\n",
	  object, ObjectName(object),methodName);*/
	cscPtr = CscAlloc(interp, &csc, NULL);
	CscInit(cscPtr, object, cl, NULL, frameType, flags);
        goto exit_object_dispatch;
      }
      if (cmd) {
        frameType = NSF_CSC_TYPE_ACTIVE_MIXIN;
      }
    }
  }

  /* check if an absolute method name was provided */
  if (*methodName == ':') {
    cmd = Tcl_GetCommandFromObj(interp, methodObj);
    if (cmd) {
      Tcl_ObjCmdProc *procPtr = Tcl_Command_objProc(cmd);
      
      if (procPtr == NsfObjDispatch) {
	/*
	 * Don't allow to call objects as methods (for the time being)
	 * via absolute names. Otherwise, in line {2} below, ::State
	 * is interpreted as an ensemble object, and the method
	 * "unknown" won't be called (in the XOTcl tradition) and
	 * wierd things will happen.
	 *
	 *  {1} Class ::State
	 *  {2} Class ::State -parameter x
	 */
	NsfLog(interp, NSF_LOG_NOTICE, "Don't invoke object %s this way. Register object via alias...", methodName);
	cmd = NULL;

      } else if (IsClassNsName(methodName)) {
	CONST char *className = NSCutNsfClasses(methodName);
	CONST char *mn = Tcl_GetCommandName(interp, cmd);
	Tcl_DString ds, *dsPtr = &ds;

	DSTRING_INIT(dsPtr);
	Tcl_DStringAppend(dsPtr, className, strlen(className) - strlen(mn) - 2);
	cl = (NsfClass *)GetObjectFromString(interp, Tcl_DStringValue(dsPtr));
	DSTRING_FREE(dsPtr);
      }
    }
  }

  /*
   * If no filter/mixin is found => do ordinary method lookup
   */
  if (cmd == NULL) {
    /* do we have a object-specific proc? */
    if (object->nsPtr && (flags & NSF_CM_NO_OBJECT_METHOD) == 0) {
      cmd = FindMethod(object->nsPtr, methodName);
      /*fprintf(stderr, "lookup for proc in obj %p method %s nsPtr %p => %p\n",
	object, methodName, object->nsPtr, cmd);*/
    }

    if (cmd == NULL) {
      /* check for a method inherited from a class */
      NsfClass *currentClass = object->cl;
      if (currentClass->order == NULL) currentClass->order = TopoOrder(currentClass, Super);
      cl = SearchPLMethod(currentClass->order, methodName, &cmd);
    }
  }

  /*
   * Check, whether we have a protected method, and whether the
   * protected method, called on a different object. In this case, we
   * treat it as unknown.
   */

  if (cmd && (Tcl_Command_flags(cmd) & NSF_CMD_PROTECTED_METHOD) &&
      (flags & (NSF_CM_NO_UNKNOWN|NSF_CM_NO_PROTECT)) == 0) {
    NsfObject *lastSelf = GetSelfObj(interp);

    if (object != lastSelf) {
      NsfLog(interp, NSF_LOG_WARN, "'%s %s' fails since method %s.%s is protected", 
	     ObjectName(object), methodName, 
	     cl ? ClassName(cl) : ObjectName(object), methodName);
      /* reset cmd, since it is still unknown */
      cmd = NULL;
    }
  }

  assert(result == TCL_OK);

  if (cmd) {
    /*
     * We found the method to dispatch.
     */
    cscPtr = CscAlloc(interp, &csc, cmd);
    CscInit(cscPtr, object, cl, cmd, frameType, flags);

    if ((cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER)) {
      /* run filters not NRE enabled */
      cscPtr->flags |= NSF_CSC_IMMEDIATE;
      /* needed for invoking UNKNOWN from ProcMethodDispatchFinalize() */
      cscPtr->objc = objc-shift;
      cscPtr->objv = objv+shift;
    }

    result = MethodDispatchCsc(clientData, interp, objc-shift, objv+shift,
			       cscPtr, methodName);

    if (result == TCL_ERROR) {
      /*fprintf(stderr, "Call ErrInProc cl = %p, cmd %p, flags %.6x\n",
	cl, cl ? cl->object.id : NULL, cl ? cl->object.flags : 0);*/
      result = NsfErrInProc(interp, cmdName,
			    cl && cl->object.teardown ? cl->object.cmdName : NULL,
			    methodName);
    }
  } else {
    /*
     * The method to be dispatched is unknown
     */
    cscPtr = CscAlloc(interp, &csc, cmd);
    CscInit(cscPtr, object, cl, cmd, frameType, flags);

    flags |= NSF_CSC_UNKNOWN;
    cscPtr->flags |= NSF_CSC_UNKNOWN;
    cscPtr->objc = objc-shift;
    cscPtr->objv = objv+shift;
  }

 exit_object_dispatch:
  /*
   * In every situation, we have a cscPtr containing all context information
   */
  assert(cscPtr);

  if (!(cscPtr->flags & NSF_CSC_CALL_IS_NRE)) {
    result = ObjectDispatchFinalize(interp, cscPtr, result /*, "immediate" , methodName*/);
    CscListRemove(interp, cscPtr);
    CscFinish(interp, cscPtr, "non-scripted finalize");
  }

  /*fprintf(stderr, "ObjectDispatch %s.%s returns %d\n",
    ObjectName(object), methodName, result);*/

  NsfCleanupObject(object, "ObjectDispatch");
  /*fprintf(stderr, "ObjectDispatch call NsfCleanupObject %p DONE\n", object);*/
  DECR_REF_COUNT(cmdName); /* must be after last dereferencing of obj */

  return result;
}

/*
 *----------------------------------------------------------------------
 * DispatchDefaultMethod --
 *
 *    Dispatch the default method (when object is called without arguments)
 *    in case the object system has it defined.
 *
 * Results:
 *    result code.
 *
 * Side effects:
 *    indirect effects by calling Tcl code
 *
 *----------------------------------------------------------------------
 */
static int
DispatchDefaultMethod(ClientData clientData, Tcl_Interp *interp,
		      int objc, Tcl_Obj *CONST objv[], int flags) {
  int result;
  Tcl_Obj *methodObj = NsfMethodObj(interp, (NsfObject *)clientData, NSF_o_defaultmethod_idx);

  if (methodObj) {
    Tcl_Obj *tov[2];
    tov[0] = objv[0];
    tov[1] = methodObj;

    result = ObjectDispatch(clientData, interp, 2, tov, flags|NSF_CM_NO_UNKNOWN);
  } else {
    result = TCL_OK;
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 * DispatchDestroyMethod --
 *
 *    Dispatch the method "destroy" in case the object system has it
 *    defined. During the final cleanup of the object system, the
 *    destroy is called separately from deallocation. Normally,
 *    Object.destroy() calls dealloc, which is responsible for the
 *    physical deallocation.
 *
 * Results:
 *    result code
 *
 * Side effects:
 *    indirect effects by calling Tcl code
 *
 *----------------------------------------------------------------------
 */

static int
DispatchDestroyMethod(Tcl_Interp *interp, NsfObject *object, int flags) {
  int result;
  Tcl_Obj *methodObj;

  /*
   * Don't call destroy after exit handler started physical
   * destruction, or when it was called already before
   */
  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound ==
      NSF_EXITHANDLER_ON_PHYSICAL_DESTROY
      || (object->flags & NSF_DESTROY_CALLED)
      )
    return TCL_OK;

  /*fprintf(stderr, "    DispatchDestroyMethod obj %p flags %.6x active %d\n",
    object, object->flags,  object->activationCount); */

  PRINTOBJ("DispatchDestroyMethod", object);

  /* flag, that destroy was called and invoke the method */
  object->flags |= NSF_DESTROY_CALLED;

  if (CallDirectly(interp, object, NSF_o_destroy_idx, &methodObj)) {
    result = NsfODestroyMethod(interp, object);
  } else {
    result = CallMethod(object, interp, methodObj, 2, 0, NSF_CM_NO_PROTECT|NSF_CSC_IMMEDIATE|flags);
  }
  if (result != TCL_OK) {
    /*
     * The object might be already gone here, since we have no stack frame.
     * Therefore, we can't even use nsf::current object safely.
     */
    static char cmdString[] =
      "puts stderr \"Error in method destroy\n\
      $::errorCode $::errorInfo\"";
    Tcl_EvalEx(interp, cmdString, -1, 0);

    if (++RUNTIME_STATE(interp)->errorCount > 20)
      Tcl_Panic("too many destroy errors occured. Endless loop?", NULL);
  } else {
    if (RUNTIME_STATE(interp)->errorCount > 0)
      RUNTIME_STATE(interp)->errorCount--;
  }

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "DispatchDestroyMethod for %p exit\n", object);
#endif
  return result;
}

/*
 *----------------------------------------------------------------------
 * DispatchUnknownMethod --
 *
 *    Dispatch the method "unknown" in case the object system has it
 *    defined and the application program contains an unknown handler.
 *
 * Results:
 *    result code
 *
 * Side effects:
 *    indirect effects by calling Tcl code
 *
 *----------------------------------------------------------------------
 */

static int
DispatchUnknownMethod(ClientData clientData,
		      Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
		      NsfObject *delegator, Tcl_Obj *methodObj, int flags) {
  int result, offset;
  NsfObject *object = (NsfObject*)clientData;

  Tcl_Obj *unknownObj = NsfMethodObj(interp, object, NSF_o_unknown_idx);

  /*fprintf(stderr, "compare unknownObj %p with methodObj %p '%s' %p %p %s\n",
	  unknownObj, methodObj, ObjStr(methodObj), delegator,
	  delegator?objv[1]:NULL,
	  delegator?ObjStr(objv[1]) : NULL );*/

  if (unknownObj && methodObj != unknownObj && (flags & NSF_CM_NO_UNKNOWN) == 0) {
    /*
     * back off and try unknown;
     */
    ALLOC_ON_STACK(Tcl_Obj*, objc+3, tov);

    /*fprintf(stderr, "calling unknown for %s %s, flgs=%02x,%02x isClass=%d %p %s objc %d\n",
	    ObjectName(object), ObjStr(methodObj), flags, NSF_CM_NO_UNKNOWN,
	    NsfObjectIsClass(object), object, ObjectName(object), objc);*/

    tov[0] = object->cmdName;
    tov[1] = unknownObj;
    offset = 2;
    if (delegator) {
      tov[2] = delegator->cmdName;
      offset ++;
    }
    if (objc>0) {
      memcpy(tov + offset, objv, sizeof(Tcl_Obj *)*(objc));
    }

    flags &= ~NSF_CM_NO_SHIFT;
    result = ObjectDispatch(clientData, interp, objc+offset, tov, flags|NSF_CM_NO_UNKNOWN);

    FREE_ON_STACK(Tcl_Obj*, tov);
  } else { /* no unknown called, this is the built-in unknown handler */

    /*fprintf(stderr, "--- No unknown method Name %s objv[%d] %s\n",
      ObjStr(methodObj), 1, ObjStr(objv[1]));*/
    result = NsfPrintError(interp, "%s: unable to dispatch method '%s'",
			   ObjectName(object), MethodName(objv[1]));
  }

  return result;
}

/*
 *----------------------------------------------------------------------
 * NsfObjDispatch --
 *
 *    This function is called on every object dispatch (when an object
 *    is invoked). It calls either the passed method, or dispatches
 *    some default method.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Maybe side effects by the cmd called by ParameterCheck()
 *    or DispatchUnknownMethod()
 *
 *----------------------------------------------------------------------
 */
int
NsfObjDispatch(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  int result;
#ifdef STACK_TRACE
  NsfStackDump(interp);
#endif

#ifdef CALLSTACK_TRACE
  NsfCallStackDump(interp);
#endif

  if (objc > 1) {
    /*
     * Normal dispatch; we must not use NSF_CSC_IMMEDIATE here,
     * otherwise coroutines won't work.
     */
    result = ObjectDispatch(clientData, interp, objc, objv, 0);
  } else {
    result = DispatchDefaultMethod(clientData, interp, objc, objv, 0);
  }
  return result;
}

/*
 *  Proc-Creation
 */

static Tcl_Obj *
AddPrefixToBody(Tcl_Obj *body, int paramDefs, NsfParsedParam *paramPtr) {
  Tcl_Obj *resultBody = Tcl_NewStringObj("", 0);

  INCR_REF_COUNT(resultBody);

  if (paramDefs && paramPtr->possibleUnknowns > 0)
    Tcl_AppendStringsToObj(resultBody, "::nsf::__unset_unknown_args\n", (char *) NULL);

  Tcl_AppendStringsToObj(resultBody, ObjStr(body), (char *) NULL);
  return resultBody;
}

#define NEW_STRING(target, p, l)  target = ckalloc(l+1); strncpy(target, p, l); *((target)+l) = '\0'

NSF_INLINE static int
NoMetaChars(CONST char *pattern) {
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
static int
ConvertToString(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr,
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

static int
ConvertToTclobj(Tcl_Interp *interp, Tcl_Obj *objPtr,  NsfParam CONST *pPtr,
			   ClientData *clientData, Tcl_Obj **outObjPtr) {
  Tcl_Obj *objv[3];
  int result;

  if (pPtr->converterArg) {
    /*fprintf(stderr, "ConvertToTclobj %s (must be %s)\n", ObjStr(objPtr), ObjStr(pPtr->converterArg));*/

    objv[1] = pPtr->converterArg;
    objv[2] = objPtr;

    result = NsfCallCommand(interp, NSF_IS, 3, objv);
    if (result == TCL_OK) {
      int success;
      Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &success);
      if (success == 1) {
	*clientData = (ClientData)objPtr;
      } else {
	result = NsfObjErrType(interp, NULL, objPtr, ObjStr(pPtr->converterArg), (Nsf_Param *)pPtr);
      }
    }
  } else {
    
    if (RUNTIME_STATE(interp)->debugLevel > 0) {
      char *value = ObjStr(objPtr);
      if (*value == '-' 
	  && (pPtr->flags & NSF_ARG_CHECK_NONPOS) 
	  && isalpha(*(value+1)) 
	  /* && strchr(value+1, ' ') == 0 */
	  ) {
	NsfLog(interp, NSF_LOG_WARN, "Value '%s' of parameter '%s' could be a non-positional argument",
	       value, pPtr->name);
      }
    } 

    *clientData = (ClientData)objPtr;
    result = TCL_OK;
  }
  *outObjPtr = objPtr;
  return result;
}

static int
ConvertToNothing(Tcl_Interp *interp, Tcl_Obj *objPtr,  NsfParam CONST *pPtr,
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  *outObjPtr = objPtr;
  return TCL_OK;
}

static int
ConvertToBoolean(Tcl_Interp *interp, Tcl_Obj *objPtr,  NsfParam CONST *pPtr,
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result, bool;
  result = Tcl_GetBooleanFromObj(interp, objPtr, &bool);

  if (result == TCL_OK) {
    *clientData = (ClientData)INT2PTR(bool);
  } else {
    NsfObjErrType(interp, NULL, objPtr, "boolean", (Nsf_Param *)pPtr);
  }
  *outObjPtr = objPtr;
  return result;
}

static int
ConvertToInteger(Tcl_Interp *interp, Tcl_Obj *objPtr,  NsfParam CONST *pPtr,
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result, i;

  result = Tcl_GetIntFromObj(interp, objPtr, &i);

  if (result == TCL_OK) {
    *clientData = (ClientData)INT2PTR(i);
    *outObjPtr = objPtr;
  } else {
    NsfObjErrType(interp, NULL, objPtr, "integer", (Nsf_Param *)pPtr);
  }
  return result;
}

static int
ConvertToSwitch(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr,
			   ClientData *clientData, Tcl_Obj **outObjPtr) {
  return ConvertToBoolean(interp, objPtr, pPtr, clientData, outObjPtr);
}

static int
IsObjectOfType(Tcl_Interp *interp, NsfObject *object, CONST char *what, Tcl_Obj *objPtr,
			NsfParam CONST *pPtr) {
  NsfClass *cl;
  Tcl_DString ds, *dsPtr = &ds;

  if ((pPtr->flags & NSF_ARG_BASECLASS) && !IsBaseClass((NsfClass *)object)) {
    what = "baseclass";
    goto type_error;
  }
  if ((pPtr->flags & NSF_ARG_METACLASS) && !IsMetaClass(interp, (NsfClass *)object, 1)) {
    what = "metaclass";
    goto type_error;
  }

  if (pPtr->converterArg == NULL)
    return TCL_OK;

  if ((GetClassFromObj(interp, pPtr->converterArg, &cl, NULL) == TCL_OK)
      && IsSubType(object->cl, cl)) {
    return TCL_OK;
  }

 type_error:
  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, what, -1);
  if (pPtr->converterArg) {
    Tcl_DStringAppend(dsPtr, " of type ", -1);
    Tcl_DStringAppend(dsPtr, ObjStr(pPtr->converterArg), -1);
  }
  NsfObjErrType(interp, NULL, objPtr, Tcl_DStringValue(dsPtr), (Nsf_Param *)pPtr);
  DSTRING_FREE(dsPtr);

  return TCL_ERROR;
}

static int
ConvertToObject(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr,
		ClientData *clientData, Tcl_Obj **outObjPtr) {
  *outObjPtr = objPtr;
  if (GetObjectFromObj(interp, objPtr, (NsfObject **)clientData) == TCL_OK) {
    return IsObjectOfType(interp, (NsfObject *)*clientData, "object", objPtr, pPtr);
  }
  return NsfObjErrType(interp, NULL, objPtr, "object", (Nsf_Param *)pPtr);
}

static int
ConvertToClass(Tcl_Interp *interp, Tcl_Obj *objPtr,  NsfParam CONST *pPtr,
	       ClientData *clientData, Tcl_Obj **outObjPtr) {
  *outObjPtr = objPtr;
  if (GetClassFromObj(interp, objPtr, (NsfClass **)clientData, NULL) == TCL_OK) {
    return IsObjectOfType(interp, (NsfObject *)*clientData, "class", objPtr, pPtr);
  }
  return NsfObjErrType(interp, NULL, objPtr, "class", (Nsf_Param *)pPtr);
}

static int
ConvertToRelation(Tcl_Interp *interp, Tcl_Obj *objPtr,  NsfParam CONST *pPtr,
		  ClientData *clientData, Tcl_Obj **outObjPtr) {
  /* NsfRelationCmd is the real setter, which checks the values
     according to the relation type (Class, List of Class, list of
     filters; we treat it here just like a tclobj */
  *clientData = (ClientData)objPtr;
  *outObjPtr = objPtr;
  return TCL_OK;
}

static int
ConvertToParameter(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr,
		   ClientData *clientData, Tcl_Obj **outObjPtr) {
  CONST char *value = ObjStr(objPtr);

  *outObjPtr = objPtr;
  /*fprintf(stderr, "convert to parameter '%s' t '%s'\n", value, pPtr->type);*/
  if (*value == ':' || (*value == '-' && *(value + 1) == ':')) {
    return NsfObjErrType(interp, NULL, objPtr, pPtr->type, (Nsf_Param *)pPtr);
  }

  *clientData = (char *)ObjStr(objPtr);
  *outObjPtr = objPtr;
  return TCL_OK;
}

static int
ConvertViaCmd(Tcl_Interp *interp, Tcl_Obj *objPtr,  NsfParam CONST *pPtr,
	      ClientData *clientData, Tcl_Obj **outObjPtr) {
  Tcl_Obj *ov[5], *savedResult;
  NsfObject *object;
  int result, oc;

  /*
   * In general, when the converter is used e.g. for result checking,
   * we do not want to alter the result just when the converter sets a
   * result. So, for non-converter, we save the old result and restore
   * it before the return in case of success. Strictly speaking,
   * result-overwritng just harms for result-converters, but saving is
   * always semantic correct.
   */
  if ((pPtr->flags & NSF_ARG_IS_CONVERTER) == 0) {
    savedResult = Tcl_GetObjResult(interp); /* save the result */
    INCR_REF_COUNT(savedResult);
  } else {
    savedResult = NULL;
  }

  ov[0] = pPtr->slotObj ? pPtr->slotObj : NsfGlobalObjs[NSF_METHOD_PARAMETER_SLOT_OBJ];
  ov[1] = pPtr->converterName;
  ov[2] = pPtr->nameObj;
  ov[3] = objPtr;

  /*fprintf(stderr, "ConvertViaCmd call converter %s (refCount %d) on %s paramPtr %p\n",
    ObjStr(pPtr->converterName), pPtr->converterName->refCount, ObjStr(ov[0]), pPtr);*/
  oc = 4;
  if (pPtr->converterArg) {
    ov[4] = pPtr->converterArg;
    oc++;
  }

  INCR_REF_COUNT(ov[1]);
  INCR_REF_COUNT(ov[2]);

  /* result = Tcl_EvalObjv(interp, oc, ov, 0); */
  GetObjectFromObj(interp, ov[0], &object);
  result = ObjectDispatch(object, interp, oc, ov, NSF_CSC_IMMEDIATE|NSF_CM_NO_PROTECT);

  DECR_REF_COUNT(ov[1]);
  DECR_REF_COUNT(ov[2]);

  /* per default, the input arg is the output arg */
  *outObjPtr = objPtr;

  if (result == TCL_OK) {
    /*fprintf(stderr, "ConvertViaCmd could convert %s to '%s' paramPtr %p, is_converter %d\n",
	    ObjStr(objPtr), ObjStr(Tcl_GetObjResult(interp)),pPtr,
	    pPtr->flags & NSF_ARG_IS_CONVERTER);*/
    if (pPtr->flags & NSF_ARG_IS_CONVERTER) {
      /*
       * If we want to convert, the resulting obj is the result of the
       * converter. incr refCount is necessary e.g. for e.g.
       *     return [expr {$value + 1}]
       */
      *outObjPtr = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(*outObjPtr);
    }
    *clientData = (ClientData) *outObjPtr;

    if (savedResult) {
      /*fprintf(stderr, "restore savedResult %p\n", savedResult);*/
      Tcl_SetObjResult(interp, savedResult);  /* restore the result */
    }
  }

  if (savedResult) {
    DECR_REF_COUNT(savedResult);
  }

  return result;
}

static int
ConvertToObjpattern(Tcl_Interp *interp, Tcl_Obj *objPtr,  NsfParam CONST *pPtr,
			       ClientData *clientData, Tcl_Obj **outObjPtr) {
  Tcl_Obj *patternObj = objPtr;
  CONST char *pattern = ObjStr(objPtr);

  if (NoMetaChars(pattern)) {
    /* we have no meta characters, we try to check for an existing object */
    NsfObject *object = NULL;
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

static Tcl_Obj *
ParamCheckObj(Tcl_Interp *interp, CONST char *start, size_t len) {
  Tcl_Obj *checker = Tcl_NewStringObj("type=", 5);
  Tcl_AppendLimitedToObj(checker, start, len, INT_MAX, NULL);
  return checker;
}

static int
ParamOptionSetConverter(Tcl_Interp *interp, NsfParam *paramPtr,
                        CONST char *typeName, NsfTypeConverter *converter) {
  if (paramPtr->converter) {
    return NsfPrintError(interp, "Refuse to redefine parameter converter to use %s",
			 typeName);
  }
  paramPtr->converter = converter;
  paramPtr->nrArgs = 1;
  paramPtr->type = typeName;
  return TCL_OK;
}

static int
ParamOptionParse(Tcl_Interp *interp, CONST char *argString, 
		 size_t start, size_t remainder, 
		 int disallowedOptions, NsfParam *paramPtr) {
  int optionLength, result = TCL_OK;
  CONST char *dotdot, *option = argString + start;
  char *firstComma = memchr(option, ',', remainder);

  if (firstComma == NULL) {
    optionLength = remainder;
  } else {
    optionLength = firstComma - option;
  }

  /*fprintf(stderr, "ParamOptionParse name %s, option '%s' (%d) disallowed %.6x\n",
    paramPtr->name, option, remainder, disallowedOptions);*/
  if (strncmp(option, "required", MAX(3,optionLength)) == 0) {
    paramPtr->flags |= NSF_ARG_REQUIRED;
  } else if (strncmp(option, "optional",  MAX(3,optionLength)) == 0) {
    paramPtr->flags &= ~NSF_ARG_REQUIRED;
  } else if (strncmp(option, "substdefault", 12) == 0) {
    paramPtr->flags |= NSF_ARG_SUBST_DEFAULT;
  } else if (strncmp(option, "allowempty", 10) == 0) {
    fprintf(stderr, "******* allowempty is deprecated, use instead multiplicity 0..1\n");
    paramPtr->flags |= NSF_ARG_ALLOW_EMPTY;
  } else if (strncmp(option, "convert", 7) == 0) {
    paramPtr->flags |= NSF_ARG_IS_CONVERTER;
  } else if (strncmp(option, "initcmd", 7) == 0) {
    paramPtr->flags |= NSF_ARG_INITCMD;
  } else if (strncmp(option, "method", 6) == 0) {
    paramPtr->flags |= NSF_ARG_METHOD;
  } else if ((dotdot = strnstr(option, "..", optionLength))) {
    /* check lower bound */
    if (*option == '0') {
      paramPtr->flags |= NSF_ARG_ALLOW_EMPTY;
    } else if (*option != '1') {
      return NsfPrintError(interp, "lower bound of multiplicty in %s not supported", argString);
    }
    /* check upper bound */
    option = dotdot + 2;
    if (*option == '*' || *option == 'n') {
      if ((paramPtr->flags & (NSF_ARG_INITCMD|NSF_ARG_RELATION|NSF_ARG_METHOD|NSF_ARG_SWITCH)) != 0) {
	return NsfPrintError(interp, 
			     "option multivalued not allowed for \"initcmd\", \"method\", \"relation\" or \"switch\"\n");
      }
      paramPtr->flags |= NSF_ARG_MULTIVALUED;
    } else if (*option != '1') {
      return NsfPrintError(interp, "upper bound of multiplicty in %s not supported", argString);
    }
    //fprintf(stderr, "%s set multivalued option %s\n", paramPtr->name, option);
  } else if (strncmp(option, "multivalued", 11) == 0) {
    fprintf(stderr, "******* multivalued is deprecated, use instead multiplicity 1..*\n");
    if ((paramPtr->flags & (NSF_ARG_INITCMD|NSF_ARG_RELATION|NSF_ARG_METHOD|NSF_ARG_SWITCH)) != 0)
	return NsfPrintError(interp, 
			     "option multivalued not allowed for \"initcmd\", \"method\", \"relation\" or \"switch\"\n");
    paramPtr->flags |= NSF_ARG_MULTIVALUED;
  } else if (strncmp(option, "noarg", 5) == 0) {
    if ((paramPtr->flags & NSF_ARG_METHOD) == 0) {
      return NsfPrintError(interp, "option noarg only allowed for parameter type \"method\"");
    }
    paramPtr->flags |= NSF_ARG_NOARG;
    paramPtr->nrArgs = 0;
  } else if (optionLength >= 4 && strncmp(option, "arg=", 4) == 0) {
    if ((paramPtr->flags & (NSF_ARG_METHOD|NSF_ARG_RELATION)) == 0
        && paramPtr->converter != ConvertViaCmd)
      return NsfPrintError(interp, 
			   "option arg= only allowed for \"method\", \"relation\" or user-defined converter");
    paramPtr->converterArg =  Tcl_NewStringObj(option + 4, optionLength - 4);
    INCR_REF_COUNT(paramPtr->converterArg);
  } else if (strncmp(option, "switch", 6) == 0) {
    if (*paramPtr->name != '-') {
      return NsfPrintError(interp,
			  "invalid parameter type \"switch\" for argument \"%s\"; "
			   "type \"switch\" only allowed for non-positional arguments",
			   paramPtr->name);
    }
    result = ParamOptionSetConverter(interp, paramPtr, "switch", ConvertToSwitch);
    paramPtr->flags |= NSF_ARG_SWITCH;
    paramPtr->nrArgs = 0;
    assert(paramPtr->defaultValue == NULL);
    paramPtr->defaultValue = Tcl_NewBooleanObj(0);
    INCR_REF_COUNT(paramPtr->defaultValue);
  } else if (strncmp(option, "integer", MAX(3,optionLength)) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "integer", ConvertToInteger);
  } else if (strncmp(option, "boolean", 7) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "boolean", ConvertToBoolean);
  } else if (strncmp(option, "object", 6) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "object", ConvertToObject);
  } else if (strncmp(option, "class", 5) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "class", ConvertToClass);
  } else if (strncmp(option, "metaclass", 9) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "class", ConvertToClass);
    paramPtr->flags |= NSF_ARG_METACLASS;
  } else if (strncmp(option, "baseclass", 9) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "class", ConvertToClass);
    paramPtr->flags |= NSF_ARG_BASECLASS;
  } else if (strncmp(option, "relation", 8) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "relation", ConvertToRelation);
    paramPtr->flags |= NSF_ARG_RELATION;
    /*paramPtr->type = "tclobj";*/
  } else if (strncmp(option, "parameter", 9) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "parameter", ConvertToParameter);
  } else if (optionLength >= 6 && strncmp(option, "type=", 5) == 0) {
    if (paramPtr->converter != ConvertToObject &&
        paramPtr->converter != ConvertToClass)
      return NsfPrintError(interp, "option type= only allowed for object or class");
    paramPtr->converterArg = Tcl_NewStringObj(option + 5, optionLength - 5);
    INCR_REF_COUNT(paramPtr->converterArg);
  } else if (optionLength >= 6 && strncmp(option, "slot=", 5) == 0) {
    paramPtr->slotObj = Tcl_NewStringObj(option + 5, optionLength - 5);
    INCR_REF_COUNT(paramPtr->slotObj);
  } else {
    int i, found = -1;

    for (i=0; stringTypeOpts[i]; i++) {
      /* Do not allow abbreviations, so the additional strlen checks
	 for a full match */
      if (strncmp(option, stringTypeOpts[i], optionLength) == 0 && strlen(stringTypeOpts[i]) == optionLength) {
	found = i;
	break;
      }
    }
    if (found > -1) {
      /* converter is stringType */
      result = ParamOptionSetConverter(interp, paramPtr, "stringtype", ConvertToTclobj);
      paramPtr->converterArg =  Tcl_NewStringObj(stringTypeOpts[i], -1);
      INCR_REF_COUNT(paramPtr->converterArg);
    } else {
      /* must be a converter defined via method */
      paramPtr->converterName = ParamCheckObj(interp, option, optionLength);
      INCR_REF_COUNT(paramPtr->converterName);
      result = ParamOptionSetConverter(interp, paramPtr, ObjStr(paramPtr->converterName), ConvertViaCmd);
    }
  }

  if ((paramPtr->flags & disallowedOptions)) {
    return NsfPrintError(interp, "Parameter option '%s' not allowed", option);
  }

  return result;
}

static int
ParamParse(Tcl_Interp *interp, Tcl_Obj *procNameObj, Tcl_Obj *arg, int disallowedFlags,
           NsfParam *paramPtr, int *possibleUnknowns, int *plainParams, int *nrNonposArgs) {
  int result, npac, isNonposArgument;
  size_t nameLength, length, j;
  CONST char *argString, *argName;
  Tcl_Obj **npav;

  paramPtr->paramObj = arg;
  INCR_REF_COUNT(paramPtr->paramObj);

  result = Tcl_ListObjGetElements(interp, arg, &npac, &npav);
  if (result != TCL_OK || npac < 1 || npac > 2) {
    return NsfPrintError(interp, "wrong # of elements in parameter definition for method '%s'"
			 " (should be 1 or 2 list elements): %s",
			 ObjStr(procNameObj), ObjStr(arg));
  }

  argString = ObjStr(npav[0]);
  length = strlen(argString);

  isNonposArgument = *argString == '-';

  if (isNonposArgument) {
    argName = argString+1;
    nameLength = length-1;
    paramPtr->nrArgs = 1; /* per default 1 argument, switches set their arg numbers */
    (*nrNonposArgs) ++;
  } else {
    argName = argString;
    nameLength = length;
    paramPtr->flags |= NSF_ARG_REQUIRED; /* positional arguments are required unless we have a default */
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
        result = ParamOptionParse(interp, argString, start, end-start, disallowedFlags, paramPtr);
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
    result = ParamOptionParse(interp, argString, start, end-start, disallowedFlags, paramPtr);
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

    if (disallowedFlags & NSF_ARG_HAS_DEFAULT) {
      NsfPrintError(interp, "parameter \"%s\" is not allowed to have default \"%s\"",
		    argString, ObjStr(npav[1]));
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
    paramPtr->flags &= ~NSF_ARG_REQUIRED;
  } else if (paramPtr->flags & NSF_ARG_SUBST_DEFAULT) {
    NsfPrintError(interp, "parameter option substdefault specified for parameter \"%s\""
		  " without default value", paramPtr->name);
    goto param_error;
  }

  /* postprocessing the parameter options */

  if (paramPtr->converter == NULL) {
    /* ConvertToTclobj() is the default converter */
    paramPtr->converter = ConvertToTclobj;
  } /*else if (paramPtr->converter == ConvertViaCmd) {*/

  if ((paramPtr->slotObj || paramPtr->converter == ConvertViaCmd) && paramPtr->type) {
    Tcl_Obj *converterNameObj;
    CONST char *converterNameString;
    NsfObject *paramObj;
    NsfClass *pcl;
    Tcl_Command cmd;

    result = GetObjectFromObj(interp, paramPtr->slotObj ? paramPtr->slotObj :
			      NsfGlobalObjs[NSF_METHOD_PARAMETER_SLOT_OBJ],
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

    cmd = ObjectFindMethod(interp, paramObj, converterNameObj, &pcl);
    if (cmd == NULL) {
      if (paramPtr->converter == ConvertViaCmd) {

	NsfLog(interp, NSF_LOG_WARN, "Could not find value checker %s defined on %s",
	       converterNameString, ObjectName(paramObj));

        paramPtr->flags |= NSF_ARG_CURRENTLY_UNKNOWN;
        /* TODO: for the time being, we do not return an error here */
      }
    } else if (paramPtr->converter != ConvertViaCmd &&
               strcmp(ObjStr(paramPtr->slotObj),
		      NsfGlobalStrings[NSF_METHOD_PARAMETER_SLOT_OBJ]) != 0) {

      NsfLog(interp, NSF_LOG_WARN, "Checker method %s defined on %s shadows built-in converter",
	     converterNameString, ObjectName(paramObj));

      if (paramPtr->converterName == NULL) {
        paramPtr->converterName = converterNameObj;
        paramPtr->converter = NULL;
        result = ParamOptionSetConverter(interp, paramPtr, converterNameString, ConvertViaCmd);
      }
    }
    if ((paramPtr->flags & NSF_ARG_IS_CONVERTER) && paramPtr->converter != ConvertViaCmd) {
      return NsfPrintError(interp, "option 'convert' only allowed for application-defined converters");
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
  if (!(paramPtr->flags & NSF_ARG_REQUIRED) && paramPtr->defaultValue == NULL) {
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
ParamDefsParse(Tcl_Interp *interp, Tcl_Obj *procNameObj, Tcl_Obj *args,
               int allowedOptinons, NsfParsedParam *parsedParamPtr) {
  Tcl_Obj **argsv;
  int result, argsc;

  parsedParamPtr->paramDefs = NULL;
  parsedParamPtr->possibleUnknowns = 0;

  result = Tcl_ListObjGetElements(interp, args, &argsc, &argsv);
  if (result != TCL_OK) {
    return NsfPrintError(interp, "cannot break down non-positional args: %s", ObjStr(args));
  }

  if (argsc > 0) {
    NsfParam *paramsPtr, *paramPtr, *lastParamPtr;
    int i, possibleUnknowns = 0, plainParams = 0, nrNonposArgs = 0;
    NsfParamDefs *paramDefs;

    paramPtr = paramsPtr = ParamsNew(argsc);

    for (i=0; i < argsc; i++, paramPtr++) {
      result = ParamParse(interp, procNameObj, argsv[i], allowedOptinons,
			  paramPtr, &possibleUnknowns, &plainParams, &nrNonposArgs);
      if (result != TCL_OK) {
        ParamsFree(paramsPtr);
        return result;
      }
    }
    if (nrNonposArgs > 0 && argsc > 1) {
      for (i=0; i < argsc; i++) {
	(paramsPtr + i)->flags |= NSF_ARG_CHECK_NONPOS;
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
      lastParamPtr->converter = ConvertToNothing;
      lastParamPtr->flags &= ~NSF_ARG_REQUIRED;
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
ListMethodHandle(Tcl_Interp *interp, NsfObject *object, int withPer_object, CONST char *methodName) {
  Tcl_SetObjResult(interp, MethodHandleObj(object, withPer_object, methodName));
  return TCL_OK;
}

static int
MakeProc(Tcl_Namespace *nsPtr, NsfAssertionStore *aStore, Tcl_Interp *interp,
         Tcl_Obj *nameObj, Tcl_Obj *args, Tcl_Obj *body, Tcl_Obj *precondition,
         Tcl_Obj *postcondition, NsfObject *object,
         int withPublic, int withPer_object, int clsns) {
  Tcl_CallFrame frame, *framePtr = &frame;
  CONST char *methodName = ObjStr(nameObj);
  NsfParsedParam parsedParam;
  Tcl_Obj *ov[4];
  int result;

  /* Check, if we are allowed to redefine the method */
  result = CanRedefineCmd(interp, nsPtr, object, methodName);
  if (result == TCL_OK) {
    /* Yes, so obtain an method parameter definitions */
    result = ParamDefsParse(interp, nameObj, args, NSF_DISALLOWED_ARG_METHOD_PARAMETER, &parsedParam);
  }
  if (result != TCL_OK) {
    return result;
  }

  ov[0] = NULL; /*objv[0];*/
  ov[1] = nameObj;

  if (parsedParam.paramDefs) {
    NsfParam *pPtr;
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
    ov[3] = AddPrefixToBody(body, 1, &parsedParam);
  } else { /* no nonpos arguments */
    ov[2] = args;
    ov[3] = AddPrefixToBody(body, 0, &parsedParam);
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
          MakeObjNamespace(interp, object);
        }
        /*fprintf(stderr, "obj %s\n", ObjectName(object));
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
        Tcl_Command_flags((Tcl_Command)procPtr->cmdPtr) |= NSF_CMD_PROTECTED_METHOD;
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
MakeMethod(Tcl_Interp *interp, NsfObject *object, NsfClass *cl, Tcl_Obj *nameObj,
           Tcl_Obj *args, Tcl_Obj *body,
           Tcl_Obj *precondition, Tcl_Obj *postcondition,
           int withPublic, int clsns) {
  CONST char *argsStr = ObjStr(args), *bodyStr = ObjStr(body), *nameStr = ObjStr(nameObj);
  int result;

  if (precondition && !postcondition) {
    return NsfPrintError(interp, "%s method '%s'; when specifying a precondition (%s)"
			 " a postcondition must be specified as well",
			 ClassName(cl), nameStr, ObjStr(precondition));
  }

  /* if both, args and body are empty strings, we delete the method */
  if (*argsStr == 0 && *bodyStr == 0) {
    result = cl ?
      NsfRemoveClassMethod(interp, (Nsf_Class *)cl, nameStr) :
      NsfRemoveObjectMethod(interp, (Nsf_Object *)object, nameStr);
  } else {
    NsfAssertionStore *aStore = NULL;
    if (precondition || postcondition) {
      if (cl) {
        NsfClassOpt *opt = NsfRequireClassOpt(cl);
        if (!opt->assertions)
          opt->assertions = AssertionCreateStore();
        aStore = opt->assertions;
      } else {
        NsfObjectOpt *opt = NsfRequireObjectOpt(object);
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
GetMatchObject(Tcl_Interp *interp, Tcl_Obj *patternObj, Tcl_Obj *origObj,
                NsfObject **matchObject, CONST char **pattern) {
  if (patternObj) {
    *pattern = ObjStr(patternObj);
    if (IsNsfTclObj(interp, patternObj, matchObject)) {
    } else if (patternObj == origObj && **pattern != ':') {
      /* no meta chars, but no appropriate nsf object found, so
         return empty; we could check above with NoMetaChars(pattern)
         as well, but the only remaining case are leading colons and
         metachars. */
      return 1;
    }
  }
  return 0;
}

static void
ForwardCmdDeleteProc(ClientData clientData) {
  ForwardCmdClientData *tcd = (ForwardCmdClientData *)clientData;
  if (tcd->cmdName)     {DECR_REF_COUNT(tcd->cmdName);}
  if (tcd->subcommands) {DECR_REF_COUNT(tcd->subcommands);}
  if (tcd->onerror)     {DECR_REF_COUNT(tcd->onerror);}
  if (tcd->prefix)      {DECR_REF_COUNT(tcd->prefix);}
  if (tcd->args)        {DECR_REF_COUNT(tcd->args);}
  FREE(forwardCmdClientData, tcd);
}

static int
ForwardProcessOptions(Tcl_Interp *interp, Tcl_Obj *nameObj,
                       Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix,
                       int withObjframe, Tcl_Obj *withOnerror, int withVerbose,
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
    NsfDeprecatedCmd(interp, "forward option","-default ...", Tcl_DStringValue(dsPtr));
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
  tcd->objframe = withObjframe;
  tcd->verbose = withVerbose;
  tcd->needobjmap = 0;
  tcd->cmdName = target;
  /*fprintf(stderr, "...forwardprocess objc %d, cmdName %p %s\n", objc, target, ObjStr(target));*/

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

  /*fprintf(stderr, "+++ cmdName = %s, args = %s, # = %d\n",
    ObjStr(tcd->cmdName), tcd->args?ObjStr(tcd->args):"NULL", tcd->nr_args);*/

  if (tcd->objframe) {
    /* when we evaluating objscope, and define ...
          o forward append -objframe append
       a call to
          o append ...
       would lead to a recursive call; so we add the appropriate namespace
    */
    CONST char *nameString = ObjStr(tcd->cmdName);
    if (isAbsolutePath(nameString)) {
    } else {
      tcd->cmdName = NameInNamespaceObj(interp, nameString, CallingNameSpace(interp));
      /*fprintf(stderr, "+++ name %s not absolute, therefore qualifying %s\n", nameString,
	ObjStr(tcd->cmdName));*/
    }
  }
  INCR_REF_COUNT(tcd->cmdName);

  if (withEarlybinding) {
    Tcl_Command cmd = Tcl_GetCommandFromObj(interp, tcd->cmdName);
    if (cmd == NULL) {
      result = NsfPrintError(interp, "cannot lookup command '%s'", ObjStr(tcd->cmdName));
      goto forward_process_options_exit;
    }
    tcd->objProc = Tcl_Command_objProc(cmd);
    if (tcd->objProc == NsfObjDispatch     /* don't do direct invoke on nsf objects */
        || tcd->objProc == TclObjInterpProc  /* don't do direct invoke on tcl procs */
        ) {
      /* silently ignore earlybinding flag */
      tcd->objProc = NULL;
    } else {
      tcd->clientData = Tcl_Command_objClientData(cmd);
    }
  }

  tcd->passthrough = !tcd->args && *(ObjStr(tcd->cmdName)) != '%' && tcd->objProc;
  
 forward_process_options_exit:
  /*fprintf(stderr, "forward args = %p, name = '%s'\n", tcd->args, ObjStr(tcd->cmdName));*/
  if (result == TCL_OK) {
    *tcdp = tcd;
  } else {
    ForwardCmdDeleteProc((ClientData)tcd);
  }
  return result;
}

static NsfClasses *
ComputePrecedenceList(Tcl_Interp *interp, NsfObject *object, CONST char *pattern,
		      int withMixins, int withRootClass) {
  NsfClasses *precedenceList = NULL, *pcl, **npl = &precedenceList;

  if (withMixins) {
    if (!(object->flags & NSF_MIXIN_ORDER_VALID))
      MixinComputeDefined(interp, object);

    if (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) {
      NsfCmdList *ml = object->mixinOrder;

      while (ml) {
	NsfClass *mixin = NsfGetClassFromCmdPtr(ml->cmdPtr);
	if (pattern) {
	  if (!Tcl_StringMatch(ClassName(mixin), pattern)) continue;
	}
	npl = NsfClassListAdd(npl, mixin, NULL);
	ml = ml->nextPtr;
      }
    }
  }

  pcl = ComputeOrder(object->cl, object->cl->order, Super);
  for (; pcl; pcl = pcl->nextPtr) {
    if (withRootClass == 0 && pcl->cl->object.flags & NSF_IS_ROOT_CLASS)
      continue;

    if (pattern) {
      if (!Tcl_StringMatch(ClassName(pcl->cl), pattern)) continue;
    }
    npl = NsfClassListAdd(npl, pcl->cl, NULL);
  }
  return precedenceList;
}

static CONST char *
StripBodyPrefix(CONST char *body) {
  if (strncmp(body, "::nsf::__unset_unknown_args\n", 28) == 0)
    body += 28;
  return body;
}


static NsfObjects *
ComputeSlotObjects(Tcl_Interp *interp, NsfObject *object, NsfClass *type, int withRootClass) {
  NsfObjects *slotObjects = NULL, **npl = &slotObjects;
  NsfClasses *pl, *fullPrecendenceList;
  NsfObject *childObject, *tmpObject;
  Tcl_HashTable slotTable;

  assert(object);

  Tcl_InitHashTable(&slotTable, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", slotTable);

  fullPrecendenceList = ComputePrecedenceList(interp, object, NULL /* pattern*/, 1, withRootClass);
  for (pl=fullPrecendenceList; pl; pl = pl->nextPtr) {
    Tcl_DString ds, *dsPtr = &ds;

    DSTRING_INIT(dsPtr);
    Tcl_DStringAppend(dsPtr, ClassName(pl->cl), -1);
    Tcl_DStringAppend(dsPtr, "::slot", 6);
    tmpObject = GetObjectFromString(interp, Tcl_DStringValue(dsPtr));
    if (tmpObject) {
      Tcl_HashSearch hSrch;
      Tcl_HashEntry *hPtr, *slotEntry;
      Tcl_HashTable *cmdTablePtr;
      Tcl_Command cmd;
      int new;

      if (!tmpObject->nsPtr) continue;
      cmdTablePtr = Tcl_Namespace_cmdTablePtr(tmpObject->nsPtr);

      hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch);
      for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
	char *key = Tcl_GetHashKey(cmdTablePtr, hPtr);
	slotEntry = Tcl_CreateHashEntry(&slotTable, key, &new);
	if (!new) continue;
	cmd = (Tcl_Command) Tcl_GetHashValue(hPtr);
	childObject = NsfGetObjectFromCmdPtr(cmd);
	/*fprintf(stderr, "we have true child obj %s\n", ObjectName(childObject));*/
	if (type && !IsSubType(childObject->cl, type)) continue;
	npl = NsfObjectListAdd(npl, childObject);
      }
    }
    DSTRING_FREE(dsPtr);
  }

  Tcl_DeleteHashTable(&slotTable);
  MEM_COUNT_FREE("Tcl_InitHashTable", slotTable);

  NsfClassListFree(fullPrecendenceList);

  return slotObjects;
}

static NsfClass *
FindCalledClass(Tcl_Interp *interp, NsfObject *object) {
  NsfCallStackContent *cscPtr = CallStackGetTopFrame(interp, NULL);
  CONST char *methodName;
  Tcl_Command cmd;

  if (cscPtr->frameType == NSF_CSC_TYPE_PLAIN)
    return cscPtr->cl;

  if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER)
    methodName = MethodName(cscPtr->filterStackEntry->calledProc);
  else if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_MIXIN && object->mixinStack)
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
NSF_INLINE static int
NextSearchMethod(NsfObject *object, Tcl_Interp *interp, NsfCallStackContent *cscPtr,
                 NsfClass **clPtr, CONST char **methodNamePtr, Tcl_Command *cmdPtr,
                 int *isMixinEntry, int *isFilterEntry,
                 int *endOfFilterChain, Tcl_Command *currentCmdPtr) {
  int endOfChain = 0, objflags;

  /*
   *  Next in filters
   */

  objflags = object->flags; /* avoid stalling */
  if (!(objflags & NSF_MIXIN_ORDER_VALID)) {
    MixinComputeDefined(interp, object);
    objflags = object->flags; /* avoid stalling */
  }

  if ((objflags & NSF_FILTER_ORDER_VALID) &&
      object->filterStack &&
      object->filterStack->currentCmdPtr) {
    *cmdPtr = FilterSearchProc(interp, object, currentCmdPtr, clPtr);
    /* fprintf(stderr, "EndOfChain? cmd=%p\n",*cmd);*/
    /*  NsfCallStackDump(interp); NsfStackDump(interp);*/

    if (*cmdPtr == NULL) {
      if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER) {
        /*
	 * Reset the information to the values of method, clPtr
	 * to the values they had before calling the filters.
	 */
        *methodNamePtr = MethodName(object->filterStack->calledProc);
        endOfChain = 1;
        *endOfFilterChain = 1;
        *clPtr = NULL;
        /*fprintf(stderr, "EndOfChain resetting cl\n");*/
      }
    } else {
      *methodNamePtr = (char *) Tcl_GetCommandName(interp, *cmdPtr);
      *endOfFilterChain = 0;
      *isFilterEntry = 1;
      return TCL_OK;
    }
  }

  /*
   *  Next in Mixins
   */
  assert(objflags & NSF_MIXIN_ORDER_VALID);
  /* otherwise: MixinComputeDefined(interp, object); */

  if ((objflags & NSF_MIXIN_ORDER_VALID) && object->mixinStack) {
    int result = MixinSearchProc(interp, object, *methodNamePtr, clPtr, currentCmdPtr, cmdPtr);
    if (result != TCL_OK) {
      return result;
    }

    if (*cmdPtr == NULL) {
      if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_MIXIN) {
        endOfChain = 1;
        *clPtr = NULL;
      }
    } else {
      *isMixinEntry = 1;
      return TCL_OK;
    }
  }

  /*fprintf(stderr, "nextsearch: object %s nsPtr %p endOfChain %d\n",
    ObjectName(object), object->nsPtr, endOfChain);*/

  /*
   * Otherwise: normal method dispatch
   *
   * If we are already in the precedence ordering, then advance
   * past our last point; otherwise (if clPtr==NULL) begin from the start.
   *
   * When a mixin or filter chain reached its end, we have to search
   * the obj-specific methods as well.
   */

  if (object->nsPtr && endOfChain) {
    *cmdPtr = FindMethod(object->nsPtr, *methodNamePtr);
  } else {
    *cmdPtr = NULL;
  }

  /* printf(stderr, "NEXT methodName %s *clPtr %p %s *cmd %p\n",
   *methodNamePtr, *clPtr, ClassName((*clPtr)), *cmdPtr);*/

  if (!*cmdPtr) {
    NsfClasses *pl;

    for (pl = ComputeOrder(object->cl, object->cl->order, Super); *clPtr && pl; pl = pl->nextPtr) {
      if (pl->cl == *clPtr) {
        *clPtr = NULL;
      }
    }

    /*
     * search for a further class method
     */
    *clPtr = SearchPLMethod(pl, *methodNamePtr, cmdPtr);

  } else {
    *clPtr = NULL;
  }

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * NextGetArguments --
 *
 *    Obtain arguments for a method invoked via next either from the
 *    argument vector or from the stack (call stack content or Tcl
 *    stack). In case of ensemble calls the stack entries of the
 *    ensemble invocation are used. The function returns the arguments
 *    4 to 8.
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    none
 *
 *----------------------------------------------------------------------
 */
static int
NextGetArguments(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
		 NsfCallStackContent **cscPtrPtr, CONST char **methodNamePtr,
		 int *outObjc, Tcl_Obj ***outObjv, int *freeArgumentVector) {
  Tcl_Obj **nobjv;
  int nobjc, oc, inEnsemble;
  Tcl_CallFrame *framePtr;
  NsfCallStackContent *cscPtr = CallStackGetTopFrame(interp, &framePtr);

  /* always make sure, we only decrement when necessary */
  *freeArgumentVector = 0;

  if (!cscPtr) {
    return NsfPrintError(interp, "next: can't find self");
  }

  if (!cscPtr->cmdPtr) {
    return NsfPrintError(interp, "next: no executing proc");
  }

  oc = Tcl_CallFrame_objc(framePtr);

  if ((cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE)) {
    /*
     * We are in an ensemble method. The next works here not on the
     * actual methodName + frame, but on the ensemble above it. We
     * locate the appropriate callstack content and continue next on
     * that.
     */
    cscPtr = CallStackFindEnsembleCsc(framePtr, &framePtr);
    assert(cscPtr);
    inEnsemble = 1;
    *methodNamePtr = ObjStr(cscPtr->objv[0]);
  } else {
    inEnsemble = 0;
    *methodNamePtr = Tcl_GetCommandName(interp, cscPtr->cmdPtr);
  }

  /*fprintf(stderr, "NextGetArguments oc %d objc %d inEnsemble %d objv %p\n",
    oc, objc, inEnsemble, cscPtr->objv); */

  if (objc > -1) {
    int methodNameLength;
    /*
     * Arguments were provided. We have to construct an argument
     * vector with the first argument(s) as the method name. In an
     * ensemble, we have to insert the objs of the full ensemble name.
     */
    if (inEnsemble) {
      methodNameLength = 1 + cscPtr->objc - oc;
      nobjc = objc + methodNameLength;
      nobjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * nobjc);
      /*
       * copy the ensemble name
       */
      memcpy((char *)nobjv, cscPtr->objv, sizeof(Tcl_Obj *) * methodNameLength);

     } else {
      methodNameLength = 1;
      nobjc = objc + methodNameLength;
      nobjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * nobjc);
      /*
       * copy the method name
       */
      if (cscPtr->objv) {
	nobjv[0] = cscPtr->objv[0];
      } else if (Tcl_CallFrame_objv(framePtr)) {
	nobjv[0] = Tcl_CallFrame_objv(framePtr)[0];
      }
    }
    /*
     * copy the remaining argument vector
     */
    memcpy(nobjv + methodNameLength, objv, sizeof(Tcl_Obj *) * objc);

    INCR_REF_COUNT(nobjv[0]); /* we seem to need this here */
    *freeArgumentVector = 1;
  } else {
    /*
     * no arguments were provided
     */
    if (cscPtr->objv) {
      nobjv = (Tcl_Obj **)cscPtr->objv;
      nobjc = cscPtr->objc;
    } else {
      nobjc = Tcl_CallFrame_objc(framePtr);
      nobjv = (Tcl_Obj **)Tcl_CallFrame_objv(framePtr);
    }
  }

  *cscPtrPtr = cscPtr;
  *outObjc = nobjc;
  *outObjv = nobjv;

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * NextInvokeFinalize --
 *
 *    This finalize function is either called via NRE callback or
 *    directly (from NextSearchAndInvoke). It resets after a successul
 *    lookup and invation the continuation context (filter flags etc)
 *    and cleans up optionally the argument vector (inverse operation
 *    of NextGetArguments).
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    freeing memory
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static int
NextInvokeFinalize(ClientData data[], Tcl_Interp *interp, int result) {
  Tcl_Obj **nobjv = data[0];
  NsfCallStackContent *cscPtr = data[1];

  /*fprintf(stderr, "***** NextInvokeFinalize cscPtr %p flags %.6x is next %d result %d unk %d\n",
	  cscPtr, cscPtr->flags, cscPtr->flags & NSF_CSC_CALL_IS_NEXT, result,
	  RUNTIME_STATE(interp)->unknown);*/

  if (cscPtr->flags & NSF_CSC_CALL_IS_NEXT) {
    /* fprintf(stderr, "..... it was a successful next\n"); */
    cscPtr->flags &= ~NSF_CSC_CALL_IS_NEXT;

    if (cscPtr->frameType == NSF_CSC_TYPE_INACTIVE_FILTER)
      cscPtr->frameType = NSF_CSC_TYPE_ACTIVE_FILTER;
    else if (cscPtr->frameType == NSF_CSC_TYPE_INACTIVE_MIXIN)
      cscPtr->frameType = NSF_CSC_TYPE_ACTIVE_MIXIN;
  }

  if (nobjv) {
    INCR_REF_COUNT(nobjv[0]);
    ckfree((char *)nobjv);
  }

  if (result == TCL_ERROR && RUNTIME_STATE(interp)->unknown) {
    /* fprintf(stderr, "don't report unknown error\n"); */
    /*
     * Don't report "unknown" errors via next.
     */
    result = TCL_OK;
  }

  return result;
}

/*
 *----------------------------------------------------------------------
 * NextSearchAndInvoke --
 *
 *    The function is called with a final argument vector and searches
 *    for an possible shadowed method. In case is successful, it
 *    updates the continuation context (filter flags etc), invokes
 *    the found method, and performs cleanup.
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    The invoked method might produce side effects
 *
 *----------------------------------------------------------------------
 */
static int
NextSearchAndInvoke(Tcl_Interp *interp, CONST char *methodName,
		    int objc, Tcl_Obj *CONST objv[],
		    NsfCallStackContent *cscPtr,
		    int freeArgumentVector) {
  Tcl_Command cmd, currentCmd = NULL;
  int result, frameType = NSF_CSC_TYPE_PLAIN,
    isMixinEntry = 0, isFilterEntry = 0,
    endOfFilterChain = 0;
  NsfRuntimeState *rst = RUNTIME_STATE(interp);
  NsfObject *object = cscPtr->self;
  NsfClass *cl;

  /*
   * Search the next method & compute its method data
   */
  cl = cscPtr->cl;
  result = NextSearchMethod(object, interp, cscPtr, &cl, &methodName, &cmd,
                            &isMixinEntry, &isFilterEntry, &endOfFilterChain, &currentCmd);

  /* fprintf(stderr, "NEXT search on %s.%s cl %p cmd %p endOfFilterChain %d result %d\n",
     ObjectName(object), methodName, cl, cmd, endOfFilterChain, result);*/

  if (result != TCL_OK) {
    goto next_search_and_invoke_cleanup;
  }

#if 0
  Tcl_ResetResult(interp); /* needed for bytecode support */
#endif
  if (cmd) {
    /*
     * change mixin state
     */
    if (object->mixinStack) {
      if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_MIXIN)
        cscPtr->frameType = NSF_CSC_TYPE_INACTIVE_MIXIN;

      /* otherwise move the command pointer forward */
      if (isMixinEntry) {
        frameType = NSF_CSC_TYPE_ACTIVE_MIXIN;
        object->mixinStack->currentCmdPtr = currentCmd;
      }
    }
    /*
     * change filter state
     */
    if (object->filterStack) {
      if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER) {
	/*fprintf(stderr, "next changes filter state\n");*/
        cscPtr->frameType = NSF_CSC_TYPE_INACTIVE_FILTER;
      }

      /* otherwise move the command pointer forward */
      if (isFilterEntry) {
	/*fprintf(stderr, "next moves filter forward\n");*/
        frameType = NSF_CSC_TYPE_ACTIVE_FILTER;
        object->filterStack->currentCmdPtr = currentCmd;
      }
    }

    /*
     * now actually call the "next" method
     */

    cscPtr->flags |= NSF_CSC_CALL_IS_NEXT;
    rst->unknown = 0;
#if defined(NRE)
    { int flags;
      /*
       * Allow call only without immediate flag, when caller has NRE without immediate
       */
      flags = NsfImmediateFromCallerFlags(cscPtr->flags);

      /*fprintf(stderr, "MethodDispatch in next flags %.6x NRE %d immediate %d next-flags %.6x\n",
	cscPtr->flags,
	(cscPtr->flags & NSF_CSC_CALL_IS_NRE) != 0,
	(cscPtr->flags & NSF_CSC_IMMEDIATE) != 0,
	flags
	);*/

      if (flags == 0) {
	/*
	 * The call is NRE-enabled. We register the callback and return
	 * here immediately.  All other exists form this functions have
	 * to call NextInvokeFinalize manually on return.
	 */
	Tcl_NRAddCallback(interp, NextInvokeFinalize,
			  freeArgumentVector ? (ClientData)objv : NULL, cscPtr, NULL, NULL);
	return MethodDispatch((ClientData)object, interp, objc, objv, cmd,
			      object, cl, methodName, frameType, flags);
      } else {
	result = MethodDispatch((ClientData)object, interp, objc, objv, cmd,
				object, cl, methodName, frameType, flags);
      }
    }
#else
    result = MethodDispatch((ClientData)object, interp, objc, objv, cmd,
			    object, cl, methodName, frameType, 0);
#endif
  } else if (result == TCL_OK) {
    /*
     * We could not find a cmd, but there was no error on the call.
     * When we are at the end of a filter-chain, or within a next from
     * an ensemble, set the unknown flag to allow higher levels to
     * handle this case.
     */

    /*fprintf(stderr, "--- no cmd, csc %p frameType %.6x callType %.6x endOfFilterChain %d\n",
      cscPtr, cscPtr->frameType, cscPtr->flags, endOfFilterChain);*/
    rst->unknown = endOfFilterChain || (cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE);
    /*fprintf(stderr, "******** setting unknown to %d\n",  rst->unknown );*/
  }

 next_search_and_invoke_cleanup:
  /*
   * We come here, whenever the NRE callback is NOT registered
   */
  {ClientData data[2] = {
      freeArgumentVector ? (ClientData)objv : NULL,
      cscPtr
    };
    return NextInvokeFinalize(data, interp, result);
  }
}

/*
 *----------------------------------------------------------------------
 * NsfNextCmd --
 *
 *    nsf::next calls the next shadowed method. It might get a single
 *    argument which is used as argument vector for that method. If no
 *    argument is provided, the argument vector of the last invocation
 *    is used.
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    The invoked method might produce side effects
 *
 *----------------------------------------------------------------------
 */
/*
nsfCmd next NsfNextCmd {
  {-argName "arguments" -required 0 -type tclobj}
}
*/
static int
NsfNextCmd(Tcl_Interp *interp, Tcl_Obj *arguments) {
  int freeArgumentVector, oc, nobjc, result;
  NsfCallStackContent *cscPtr;
  CONST char *methodName;
  Tcl_Obj **nobjv, **ov;

  if (arguments) {
    /* Arguments were provided. */
    int result = Tcl_ListObjGetElements(interp, arguments, &oc, &ov);
    if (result != TCL_OK) {return result;}
  } else {
    /* No arguments were provided. */
    oc = -1;
  }

  result = NextGetArguments(interp, oc, ov, &cscPtr, &methodName,
			    &nobjc, &nobjv, &freeArgumentVector);
  if (result == TCL_OK) {
    result = NextSearchAndInvoke(interp, methodName, nobjc, nobjv, cscPtr, freeArgumentVector);
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 * NsfNextObjCmd --
 *
 *    nsf::xotclnext is for backwards compatibility to the next
 *    implementation in XOTcl.  It receives an argument vector which
 *    is used for the invocation. if no argument vector is provided,
 *    the argument vector of the last invocation is used. If the
 *    argument vector starts with "--noArgs", then no arguments are
 *    passed to the shadowed method.
 *
 *    TODO: On the longer range, this function should go into an external
 *    library (e.g. XOTcl compatibility library)
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    The invoked method might produce side effects
 *
 *----------------------------------------------------------------------
 */
int
NsfNextObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  int freeArgumentVector, nobjc, result;
  NsfCallStackContent *cscPtr;
  CONST char *methodName;
  Tcl_Obj **nobjv;

  if (objc < 2) {
    /* No arguments were provided */
    objc = 0;
  } else {
    /* in case --noargs is used, remove the flag and provide an empty argument list */
    CONST char *arg1String = ObjStr(objv[1]);
    if (*arg1String == '-' && !strcmp(arg1String, "--noArgs")) {
      objc = 1;
    }
  }

  result = NextGetArguments(interp, objc-1, &objv[1], &cscPtr, &methodName,
			    &nobjc, &nobjv, &freeArgumentVector);
  if (result == TCL_OK) {
    result = NextSearchAndInvoke(interp, methodName, nobjc, nobjv, cscPtr, freeArgumentVector);
  }
  return result;
}


/*
 * "self" object command
 */

static int
FindSelfNext(Tcl_Interp *interp) {
  NsfCallStackContent *cscPtr = CallStackGetTopFrame(interp, NULL);
  Tcl_Command cmd, currentCmd = NULL;
  int result, isMixinEntry = 0,
    isFilterEntry = 0,
    endOfFilterChain = 0;
  NsfClass *cl = cscPtr->cl;
  NsfObject *object = cscPtr->self;
  CONST char *methodName;

  Tcl_ResetResult(interp);

  methodName = (char *)Tcl_GetCommandName(interp, cscPtr->cmdPtr);
  if (!methodName) {
    return TCL_OK;
  }

  result = NextSearchMethod(object, interp, cscPtr, &cl, &methodName, &cmd,
                   &isMixinEntry, &isFilterEntry, &endOfFilterChain, &currentCmd);
  if (cmd) {
    Tcl_SetObjResult(interp, MethodHandleObj(cl ? (NsfObject*)cl : object,
					     cl == NULL, methodName));
  }
  return result;
}

static Tcl_Obj *
ComputeLevelObj(Tcl_Interp *interp, CallStackLevel level) {
  Tcl_CallFrame *framePtr;
  Tcl_Obj *resultObj;

  switch (level) {
  case CALLING_LEVEL: NsfCallStackFindLastInvocation(interp, 1, &framePtr); break;
  case ACTIVE_LEVEL:  NsfCallStackFindActiveFrame(interp,    1, &framePtr); break;
  default: framePtr = NULL;
  }

  if (framePtr) {
    /* the call was from an nsf frame, return absolute frame number */
    char buffer[LONG_AS_STRING];
    int l;

    buffer[0] = '#';
    Nsf_ltoa(buffer+1, (long)Tcl_CallFrame_level(framePtr), &l);
    /*fprintf(stderr, "*** framePtr=%p buffer %s\n", framePtr, buffer);*/
    resultObj = Tcl_NewStringObj(buffer, l+1);
  } else {
    /* If not called from an nsf frame, return 1 as default */
    resultObj = Tcl_NewIntObj(1);
  }

  return resultObj;
}

/*
  int
  NsfKObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  if (objc < 2)
  return NsfPrintError(interp, "wrong # of args for K");

  Tcl_SetObjResult(interp, objv[1]);
  return TCL_OK;
  }
*/

/*
 * object creation & destruction
 */

static int
UnsetInAllNamespaces(Tcl_Interp *interp, Namespace *nsPtr, CONST char *name) {
  int rc = 0;
  fprintf(stderr, "### UnsetInAllNamespaces variable '%s', current namespace '%s'\n",
          name, nsPtr ? nsPtr->fullName : "NULL");

  if (nsPtr) {
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr = Tcl_FirstHashEntry(Tcl_Namespace_childTablePtr(nsPtr), &search);
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
      rc |= UnsetInAllNamespaces(interp, childNsPtr, name);
    }
  }
  return rc;
}

static int
FreeUnsetTraceVariable(Tcl_Interp *interp, NsfObject *object) {
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
    /*fprintf(stderr, "### FreeUnsetTraceVariable %s\n", obj->opt->volatileVarName);*/

    result = Tcl_UnsetVar2(interp, object->opt->volatileVarName, NULL, 0);
    if (result != TCL_OK) {
      int result = Tcl_UnsetVar2(interp, object->opt->volatileVarName, NULL, TCL_GLOBAL_ONLY);
      if (result != TCL_OK) {
        Namespace *nsPtr = (Namespace *) Tcl_GetCurrentNamespace(interp);
        if (UnsetInAllNamespaces(interp, nsPtr, object->opt->volatileVarName) == 0) {
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
NsfUnsetTrace(ClientData clientData, Tcl_Interp *interp, CONST char *name, CONST char *name2, int flags)
{
  Tcl_Obj *obj = (Tcl_Obj *)clientData;
  NsfObject *object;
  char *resultMsg = NULL;

  /*fprintf(stderr, "NsfUnsetTrace %s flags %.4x %.4x\n", name, flags,
    flags & TCL_INTERP_DESTROYED); **/

  if ((flags & TCL_INTERP_DESTROYED) == 0) {
    if (GetObjectFromObj(interp, obj, &object) == TCL_OK) {
      Tcl_Obj *res = Tcl_GetObjResult(interp); /* save the result */
      INCR_REF_COUNT(res);

      /* clear variable, destroy is called from trace */
      if (object->opt && object->opt->volatileVarName) {
        object->opt->volatileVarName = NULL;
      }

      if (DispatchDestroyMethod(interp, object, 0) != TCL_OK) {
        resultMsg = "Destroy for volatile object failed";
      } else
        resultMsg = "No nsf Object passed";

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
CleanupDestroyObject(Tcl_Interp *interp, NsfObject *object, int softrecreate) {
  /*fprintf(stderr, "CleanupDestroyObject obj %p softrecreate %d nsPtr %p\n",
    object, softrecreate, object->nsPtr);*/

  /* remove the instance, but not for ::Class/::Object */
  if ((object->flags & NSF_IS_ROOT_CLASS) == 0 &&
      (object->flags & NSF_IS_ROOT_META_CLASS) == 0 ) {

    if (!softrecreate) {
      (void)RemoveInstance(object, object->cl);
    }
  }

  if (object->nsPtr) {
    NSCleanupNamespace(interp, object->nsPtr);
    NSDeleteChildren(interp, object->nsPtr);
  }

  if (object->varTablePtr) {
    TclDeleteVars(((Interp *)interp), object->varTablePtr);

    ckfree((char *)object->varTablePtr);
    /*FREE(obj->varTablePtr, obj->varTablePtr);*/
    object->varTablePtr = 0;
  }

  if (object->opt) {
    NsfObjectOpt *opt = object->opt;
    AssertionRemoveStore(opt->assertions);
    opt->assertions = NULL;

    if (!softrecreate) {
      /*
       *  Remove this object from all per object mixin lists and clear the mixin list
       */
      RemoveFromObjectMixinsOf(object->id, opt->mixins);

      CmdListRemoveList(&opt->mixins, GuardDel);
      CmdListRemoveList(&opt->filters, GuardDel);
      FREE(NsfObjectOpt, opt);
      opt = object->opt = 0;
    }
  }

  object->flags &= ~NSF_MIXIN_ORDER_VALID;
  if (object->mixinOrder)  MixinResetOrder(object);
  object->flags &= ~NSF_FILTER_ORDER_VALID;
  if (object->filterOrder) FilterResetOrder(object);
}

/*
 * do obj initialization & namespace creation
 */
static void
CleanupInitObject(Tcl_Interp *interp, NsfObject *object,
                  NsfClass *cl, Tcl_Namespace *nsPtr, int softrecreate) {

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "+++ CleanupInitObject\n");
#endif
  object->teardown = interp;
  object->nsPtr = nsPtr;
  if (!softrecreate) {
    AddInstance(object, cl);
  }
  if (object->flags & NSF_RECREATE) {
    object->opt = NULL;
    object->varTablePtr = NULL;
    object->mixinOrder = NULL;
    object->filterOrder = NULL;
    object->flags = 0;
  }
  /*
    fprintf(stderr, "cleanupInitObject %s: %p cl = %p\n",
    obj->cmdName ? ObjectName(object) : "", object, object->cl);*/
}

static void
PrimitiveDestroy(ClientData clientData) {
  NsfObject *object = (NsfObject*)clientData;

  if (NsfObjectIsClass(object))
    PrimitiveCDestroy((ClientData) object);
  else
    PrimitiveODestroy((ClientData) object);
}

static void
TclDeletesObject(ClientData clientData) {
  NsfObject *object = (NsfObject*)clientData;
  Tcl_Interp *interp;

  object->flags |= NSF_TCL_DELETE;
  /*fprintf(stderr, "cmd dealloc %p TclDeletesObject (%d)\n",
    object->id,  Tcl_Command_refCount(object->id));*/

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "TclDeletesObject %p obj->id %p flags %.6x\n", object, object->id, object->flags);
#endif
  if ((object->flags & NSF_DURING_DELETE) || !object->teardown) return;
  interp = object->teardown;
# ifdef OBJDELETION_TRACE
  fprintf(stderr, "... %p %s\n", object, ObjectName(object));
# endif

  CallStackDestroyObject(interp, object);
  /*fprintf(stderr, "TclDeletesObject %p DONE\n", object);*/
}

/*
 * physical object destroy
 */
static void
PrimitiveODestroy(ClientData clientData) {
  NsfObject *object = (NsfObject*)clientData;
  Tcl_Interp *interp;

  if (!object || !object->teardown) return;

  /*fprintf(stderr, "****** PrimitiveODestroy %p flags %.6x\n", object, object->flags);*/
  assert(!(object->flags & NSF_DELETED));

  /* destroy must have been called already */
  assert(object->flags & NSF_DESTROY_CALLED);

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
          object, object->id, (object->flags & NSF_DESTROY_CALLED), ObjectName(object));
#endif
  CleanupDestroyObject(interp, object, 0);

  while (object->mixinStack)
    MixinStackPop(object);

  while (object->filterStack)
    FilterStackPop(object);

  object->teardown = NULL;
  if (object->nsPtr) {
    /*fprintf(stderr, "PrimitiveODestroy calls deleteNamespace for object %p nsPtr %p\n", object, object->nsPtr);*/
    Nsf_DeleteNamespace(interp, object->nsPtr);
    object->nsPtr = NULL;
  }

  /*fprintf(stderr, " +++ OBJ/CLS free: %s\n", ObjectName(object));*/

  object->flags |= NSF_DELETED;
  ObjTrace("ODestroy", object);

  DECR_REF_COUNT(object->cmdName);
  NsfCleanupObject(object, "PrimitiveODestroy");

}

/*
 *----------------------------------------------------------------------
 * DoDealloc --
 *
 *    Perform deallocation of an object/class. This function is called
 *    from the dealloc method and interanally to get rid of an
 *    abject. It cares about volatile and frees/triggers free
 *    operation depending on the stack references.
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    freed object or object is marked to be freed.
 *
 *----------------------------------------------------------------------
 */
static int
DoDealloc(Tcl_Interp *interp, NsfObject *object) {
  int result;

  /*fprintf(stderr, "DoDealloc obj= %s %p flags %.6x activation %d cmd %p opt=%p\n",
          ObjectName(object), object, object->flags, object->activationCount,
          object->id, object->opt);*/

  result = FreeUnsetTraceVariable(interp, object);
  if (result != TCL_OK) {
    return result;
  }

  /*
   * latch, and call delete command if not already in progress
   */
  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound !=
      NSF_EXITHANDLER_ON_SOFT_DESTROY) {
    CallStackDestroyObject(interp, object);
  }

  return TCL_OK;
}

/*
 * reset the object to a fresh, undestroyed state
 */
static void
MarkUndestroyed(NsfObject *object) {
  object->flags &= ~NSF_DESTROY_CALLED;
}

static void
PrimitiveOInit(NsfObject *object, Tcl_Interp *interp, CONST char *name, 
	       Tcl_Namespace *nsPtr, NsfClass *cl) {
  
#ifdef OBJDELETION_TRACE
  fprintf(stderr, "+++ PrimitiveOInit\n");
#endif

#ifdef NSFOBJ_TRACE
  fprintf(stderr, "OINIT %s = %p\n", name, object);
#endif
  NsfObjectRefCountIncr(object);
  MarkUndestroyed(object);

  /*
   * There might be already a namespace with name name; if this is the
   * case, use this namepsace as object namespace. The preexisting
   * namespace might contain Next Scripting objects. If we would not use the
   * namespace as child namespace, we would not recognize the objects
   * as child objects, deletions of the object might lead to a crash.
   * 
   * We can use here the provided nsPtr, except in cases, where this
   * namepaces is being destroyed (e.g. recreate a new object from a
   * different object system).
   */

  if (nsPtr && (((Namespace *)nsPtr)->flags & NS_DYING)) {
    Tcl_Namespace *dummy1Ptr, *dummy2Ptr;
    const char *dummy;
    TclGetNamespaceForQualName(interp, name, 
			       NULL, TCL_GLOBAL_ONLY|TCL_FIND_ONLY_NS, 
			       (Namespace **)&nsPtr,
			       (Namespace **)&dummy1Ptr, (Namespace **)&dummy2Ptr, &dummy);
    /*fprintf(stderr, "PrimitiveOInit %p calls TclGetNamespaceForQualName with %s => %p given %p object->nsPtr %p\n", 
	    object, name, 
	    nsPtr, nsPtr, object->nsPtr);*/
  }

  if (nsPtr) {
    NsfNamespaceInit(interp, nsPtr);
  }
  
  /* fprintf(stderr, "PrimitiveOInit %p %s, ns %p\n", object, name, nsPtr); */
  CleanupInitObject(interp, object, cl, nsPtr, 0);

  // TODO: would be nice, if we could init object flags */
  /* object->flags = NSF_MIXIN_ORDER_VALID | NSF_FILTER_ORDER_VALID;*/
  object->mixinStack = NULL;
  object->filterStack = NULL;
}

/*
 * Object creation: create object name (full name) and Tcl command
 */
static NsfObject *
PrimitiveOCreate(Tcl_Interp *interp, Tcl_Obj *nameObj, Tcl_Namespace *parentNsPtr, NsfClass *cl) {
  NsfObject *object = (NsfObject*)ckalloc(sizeof(NsfObject));
  CONST char *nameString = ObjStr(nameObj);
  Tcl_Namespace *nsPtr;

  /*fprintf(stderr, "PrimitiveOCreate %s parentNs %p\n",nameString, parentNsPtr);*/

#if defined(NSFOBJ_TRACE)
  fprintf(stderr, "CKALLOC Object %p %s\n", object, nameString);
#endif
#ifdef OBJDELETION_TRACE
  fprintf(stderr, "+++ PrimitiveOCreate\n");
#endif

  memset(object, 0, sizeof(NsfObject));
  MEM_COUNT_ALLOC("NsfObject/NsfClass", object);
  assert(object); /* ckalloc panics, if malloc fails */
  assert(isAbsolutePath(nameString));

  nsPtr = NSCheckNamespace(interp, nameString, parentNsPtr, cl);
  object->id = Tcl_CreateObjCommand(interp, nameString, NsfObjDispatch,
				    (ClientData)object, TclDeletesObject);

  /*fprintf(stderr, "cmd alloc %p %d (%s)\n", object->id, 
    Tcl_Command_refCount(object->id), nameString);*/

  PrimitiveOInit(object, interp, nameString, nsPtr, cl);
  object->cmdName = nameObj;
  /* convert cmdName to Tcl Obj of type cmdName */
  /*Tcl_GetCommandFromObj(interp, obj->cmdName);*/

  INCR_REF_COUNT(object->cmdName);
  ObjTrace("PrimitiveOCreate", object);

  return object;
}

/*
 *----------------------------------------------------------------------
 * DefaultSuperClass --
 *
 *    Determine the default Superclass of the class (specified as
 *    second argument) and meta class (third argument). The function
 *    searches for the variable NSF_DEFAULTMETACLASS or
 *    NSF_DEFAULTSUPERCLASS and uses it if present.
 *
 * Results:
 *    Default superclass or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static NsfClass *
DefaultSuperClass(Tcl_Interp *interp, NsfClass *cl, NsfClass *mcl, int isMeta) {
  NsfClass *resultClass = NULL;

  /*fprintf(stderr, "DefaultSuperClass cl %s, mcl %s, isMeta %d\n",
    ClassName(cl), ClassName(mcl), isMeta );*/

  if (mcl) {
    Tcl_Obj *resultObj = Nsf_ObjGetVar2((Nsf_Object *)mcl, interp, isMeta ?
					NsfGlobalObjs[NSF_DEFAULTMETACLASS] :
					NsfGlobalObjs[NSF_DEFAULTSUPERCLASS], NULL, 0);

    if (resultObj) {
      if (GetClassFromObj(interp, resultObj, &resultClass, NULL) != TCL_OK) {
	NsfPrintError(interp, "default superclass is not a class");
      }
      /* fprintf(stderr, "DefaultSuperClass for %s got from var %s\n", ClassName(cl), ObjStr(nameObj)); */
      
    } else {
      NsfClasses *sc;

      /* fprintf(stderr, "DefaultSuperClass for %s: search in superclasses starting with %p meta %d\n",
	 ClassName(cl), cl->super, isMeta); */
      
      if (isMeta) {
	/* 
	 * Is this already the root metaclass ?
	 */
        if (mcl->object.cl->object.flags & NSF_IS_ROOT_META_CLASS) {
          return mcl->object.cl;
        }
      }
      /*
       * check superclasses of metaclass
       */
      for (sc = mcl->super; sc && sc->cl != cl; sc = sc->nextPtr) {
	/* fprintf(stderr, "  ... check ismeta %d %s root mcl %d root cl %d\n",
                isMeta, ClassName(sc->cl),
                sc->cl->object.flags & NSF_IS_ROOT_META_CLASS,
                sc->cl->object.flags & NSF_IS_ROOT_CLASS); */
	if (isMeta) {
	  if (sc->cl->object.flags & NSF_IS_ROOT_META_CLASS) {
	    return sc->cl;
	  }
	} else {
	  if (sc->cl->object.flags & NSF_IS_ROOT_CLASS) {
            /* fprintf(stderr, "found root class %p %s\n", sc->cl, ClassName(sc->cl)); */
	    return sc->cl;
	  }
	}
	resultClass = DefaultSuperClass(interp, cl, sc->cl, isMeta);
	if (resultClass) {
	  break;
	}
      }
    }
  } else {
    /* 
     * During bootstrapping, there might be no meta class defined yet 
     */
    /* fprintf(stderr, "no meta class ismeta %d %s root mcl %d root cl %d\n",
                  isMeta, ClassName(cl),
                  cl->object.flags & NSF_IS_ROOT_META_CLASS,
                  cl->object.flags & NSF_IS_ROOT_CLASS); */
  }

  return resultClass;
}

/*
 * Cleanup class: remove filters, mixins, assertions, instances ...
 * and remove class from class hierarchy
 */
static void
CleanupDestroyClass(Tcl_Interp *interp, NsfClass *cl, int softrecreate, int recreate) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  NsfClassOpt *clopt = cl->opt;
  NsfClass *baseClass = NULL;

  PRINTOBJ("CleanupDestroyClass", (NsfObject *)cl);
  assert(softrecreate ? recreate == 1 : 1);

  /* fprintf(stderr, "CleanupDestroyClass %p %s (ismeta=%d) softrecreate=%d, recreate=%d, %p\n", cl,ClassName(cl),IsMetaClass(interp, cl, 1),
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
#ifdef NSF_OBJECTDATA
    NsfFreeObjectData(cl);
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
    if ((cl->object.flags & NSF_IS_ROOT_CLASS) == 0) {

      hPtr = &cl->instances ? Tcl_FirstHashEntry(&cl->instances, &hSrch) : NULL;
      for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
        NsfObject *inst = (NsfObject*)Tcl_GetHashKey(&cl->instances, hPtr);
        /*fprintf(stderr, "    inst %p %s flags %.6x id %p baseClass %p %s\n",
	  inst, ObjectName(inst), inst->flags, inst->id,baseClass,ClassName(baseClass));*/
        if (inst && inst != (NsfObject*)cl && !(inst->flags & NSF_DURING_DELETE) /*inst->id*/) {
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
    FREE(NsfClassOpt, clopt);
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
      NsfClass *subClass = cl->sub->cl;
      (void)RemoveSuper(subClass, cl);
      /*
       * If there are no more super classes add the Object
       * class as superclasses
       * -> don't do that for Object itself!
       */
      if (subClass->super == 0 && (cl->object.flags & NSF_IS_ROOT_CLASS) == 0) {
	/* fprintf(stderr,"subClass %p %s baseClass %p %s\n",
	   cl,ClassName(cl),baseClass,ClassName(baseClass)); */
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
CleanupInitClass(Tcl_Interp *interp, NsfClass *cl, Tcl_Namespace *nsPtr,
                 int softrecreate, int recreate) {
  NsfClass *defaultSuperclass;

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
  NsfObjectSetClass((NsfObject*)cl);

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
  NsfClass *cl = (NsfClass*)clientData;
  NsfObject *object = (NsfObject*)clientData;
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
  /*fprintf(stderr, "PrimitiveCDestroy %s flags %.6x\n", ObjectName(object), object->flags);*/

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
  Nsf_DeleteNamespace(interp, saved);
  /*fprintf(stderr, "primitive cdestroy %p DONE\n",cl);*/
  return;
}

/*
 * class init
 */
static void
PrimitiveCInit(NsfClass *cl, Tcl_Interp *interp, CONST char *name) {
  Tcl_CallFrame frame, *framePtr = &frame;
  Tcl_Namespace *nsPtr;

  /*
   * ensure that namespace is newly created during CleanupInitClass
   * ie. kill it, if it exists already
   */
  if (Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr,
                        RUNTIME_STATE(interp)->NsfClassesNS, 0) != TCL_OK) {
    return;
  }
  nsPtr = NSGetFreshNamespace(interp, (ClientData)cl, name);
  Tcl_PopCallFrame(interp);

  CleanupInitClass(interp, cl, nsPtr, 0, 0);
  return;
}

/*
 * class create: creation of namespace + class full name
 * calls class object creation
 */
static NsfClass *
PrimitiveCCreate(Tcl_Interp *interp, Tcl_Obj *nameObj, Tcl_Namespace *parentNsPtr, NsfClass *class) {
  NsfClass *cl = (NsfClass*)ckalloc(sizeof(NsfClass));
  Tcl_Namespace *nsPtr;
  CONST char *nameString = ObjStr(nameObj);
  size_t length;
  NsfObject *object = (NsfObject*)cl;

  /* fprintf(stderr, "PrimitiveCCreate %s parentNs %p\n",nameString, parentNsPtr); */

  /*fprintf(stderr, "CKALLOC Class %p %s\n", cl, nameString);*/
  memset(cl, 0, sizeof(NsfClass));
  MEM_COUNT_ALLOC("NsfObject/NsfClass", cl);

  /* pass object system from meta class */
  if (class) {
    cl->osPtr = class->osPtr;
  }

  assert(isAbsolutePath(nameString));
  length = strlen(nameString);
  /*
    fprintf(stderr, "Class alloc %p '%s'\n", cl, nameString);
  */
  nsPtr = NSCheckNamespace(interp, nameString, parentNsPtr, cl);
  object->id = Tcl_CreateObjCommand(interp, nameString, NsfObjDispatch,
                                 (ClientData)cl, TclDeletesObject);
  PrimitiveOInit(object, interp, nameString, nsPtr, class);
  object->cmdName = nameObj;

  /* convert cmdName to Tcl Obj of type cmdName */
  /* Tcl_GetCommandFromObj(interp, obj->cmdName);*/

  INCR_REF_COUNT(object->cmdName);
  PrimitiveCInit(cl, interp, nameString+2);

  ObjTrace("PrimitiveCCreate", object);
  return cl;
}


/*
 *----------------------------------------------------------------------
 * ChangeClass --
 *
 *    Change class of a Next Scripting object. This function takes
 *    care that one tries not to change an object into a class or vice
 *    versa. Changing meta-class to meta-class, or class to class, or
 *    object to object is fine, but upgrading/downgrading is not
 *    allowed
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    changes class of object if possible and updates instance relation.
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static int
ChangeClass(Tcl_Interp *interp, NsfObject *object, NsfClass *cl) {
  assert(object);

  /*fprintf(stderr, "changing %s to class %s ismeta %d\n",
          ObjectName(object), ClassName(cl),
          IsMetaClass(interp, cl, 1));*/

  if (cl != object->cl) {
    if (IsMetaClass(interp, cl, 1)) {
      /* Do not allow upgrading from a class to a meta-class (in
	 other words, don't make an object to a class). To allow
	 this, it would be necessary to reallocate the base
	 structures.
      */
      if (!IsMetaClass(interp, object->cl, 1)) {
	return NsfPrintError(interp, "cannot turn object into a class");
      }
    } else {
      /* The target class is not a meta class.  */

      /*fprintf(stderr, "target class %s not a meta class, am i a class %d\n",
	ClassName(cl), NsfObjectIsClass(object) );*/

      if (NsfObjectIsClass(object)) {
	return NsfPrintError(interp, "cannot turn class into an object ");
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
DoObjInitialization(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj *methodObj, *savedObjResult = Tcl_GetObjResult(interp); /* save the result */
  int result;

  INCR_REF_COUNT(savedObjResult);
  /*
   * clear INIT_CALLED flag
   */
  object->flags &= ~NSF_INIT_CALLED;
  /*
   * Make sure, the object survives initialization; the initcmd might
   * destroy it.
   */
  object->refCount ++;

  /*
   * call configure method
   */
  if (CallDirectly(interp, object, NSF_o_configure_idx, &methodObj)) {
    ALLOC_ON_STACK(Tcl_Obj*, objc, tov);
    memcpy(tov+1, objv+2, sizeof(Tcl_Obj *)*(objc-2));
    /* the provided name of the method is just for error reporting */
    tov[0] = methodObj ? methodObj : NsfGlobalObjs[NSF_CONFIGURE];
    result = NsfOConfigureMethod(interp, object, objc-1, tov);
    FREE_ON_STACK(Tcl_Obj*, tov);
  } else {
    result = CallMethod((ClientData) object, interp, methodObj, objc, objv+2, NSF_CSC_IMMEDIATE);
  }

  if (result != TCL_OK) {
    goto objinitexit;
  }

  /*
   * check, whether init was called already
   */
  if (!(object->flags & (NSF_INIT_CALLED|NSF_DESTROY_CALLED))) {
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
    methodObj = NsfMethodObj(interp, object, NSF_o_init_idx);
    if (methodObj) {
      result = CallMethod((ClientData) object, interp, methodObj,
			  nobjc+2, nobjv, NSF_CM_NO_PROTECT|NSF_CSC_IMMEDIATE);
    }
    object->flags |= NSF_INIT_CALLED;
    DECR_REF_COUNT(resultObj);
  }

  if (result == TCL_OK) {
    Tcl_SetObjResult(interp, savedObjResult);
  }

 objinitexit:
  NsfCleanupObject(object, "obj init");
  DECR_REF_COUNT(savedObjResult);
  return result;
}


static int
HasMetaProperty(Tcl_Interp *interp, NsfClass *cl) {
  return cl->object.flags & NSF_IS_ROOT_META_CLASS;
}

static int
IsBaseClass(NsfClass *cl) {
  return cl->object.flags & (NSF_IS_ROOT_META_CLASS|NSF_IS_ROOT_CLASS);
}


static int
IsMetaClass(Tcl_Interp *interp, NsfClass *cl, int withMixins) {
  /* check if class is a meta-class */
  NsfClasses *pl;

  /* is the class the most general meta-class? */
  if (HasMetaProperty(interp, cl)) {
    return 1;
  }

  /* is the class a subclass of a meta-class? */
  for (pl = ComputeOrder(cl, cl->order, Super); pl; pl = pl->nextPtr) {
    if (HasMetaProperty(interp, pl->cl))
      return 1;
  }

  if (withMixins) {
    NsfClasses *checkList = NULL, *mixinClasses = NULL, *mc;
    int hasMCM = 0;

    /* has the class metaclass mixed in? */
    for (pl = ComputeOrder(cl, cl->order, Super); pl; pl = pl->nextPtr) {
      NsfClassOpt *clopt = pl->cl->opt;
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
    NsfClassListFree(mixinClasses);
    NsfClassListFree(checkList);
    /*fprintf(stderr, "has MC returns %d, mixinClasses = %p\n",
      hasMCM, mixinClasses);*/

    return hasMCM;
  } else {
    return 0;
  }

}

static int
IsSubType(NsfClass *subcl, NsfClass *cl) {
  NsfClasses *t;
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
HasMixin(Tcl_Interp *interp, NsfObject *object, NsfClass *cl) {

  if (!(object->flags & NSF_MIXIN_ORDER_VALID))
    MixinComputeDefined(interp, object);

  if ((object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID)) {
    NsfCmdList *ml;
    for (ml = object->mixinOrder; ml; ml = ml->nextPtr) {
      NsfClass *mixin = NsfGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin == cl) {
        return 1;
      }
    }
  }
  return 0;
}

extern int
NsfCreateObject(Tcl_Interp *interp, Tcl_Obj *nameObj, Nsf_Class *class) {
  NsfClass *cl = (NsfClass*) class;
  Tcl_Obj *methodObj;
  int result;

  INCR_REF_COUNT(nameObj);

  if (CallDirectly(interp, &cl->object, NSF_c_create_idx, &methodObj)) {
    result = NsfCCreateMethod(interp, cl, ObjStr(nameObj), 1, &nameObj);
  } else {
    result = NsfCallMethodWithArgs((ClientData)cl, interp, methodObj,
                                     nameObj, 1, 0, NSF_CSC_IMMEDIATE);
  }
  DECR_REF_COUNT(nameObj);
  return result;
}

extern int
NsfCreate(Tcl_Interp *interp, Nsf_Class *class, Tcl_Obj *nameObj, ClientData clientData,
            int objc, Tcl_Obj *CONST objv[]) {
  NsfClass *cl = (NsfClass *) class;
  int result;
  ALLOC_ON_STACK(Tcl_Obj *, objc+2, ov);

  INCR_REF_COUNT(nameObj);

  ov[0] = NULL;
  ov[1] = nameObj;
  if (objc>0) {
    memcpy(ov+2, objv, sizeof(Tcl_Obj *)*objc);
  }
  result = NsfCCreateMethod(interp, cl, ObjStr(nameObj), objc+2, ov);

  FREE_ON_STACK(Tcl_Obj*, ov);
  DECR_REF_COUNT(nameObj);

  return result;
}

int
NsfDeleteObject(Tcl_Interp *interp, Nsf_Object *object1) {
  NsfObject *object = (NsfObject *) object1;

  return DispatchDestroyMethod(interp, object, 0);
}

extern int
NsfUnsetInstVar2(Nsf_Object *object1, Tcl_Interp *interp,
                   CONST char *name1, CONST char *name2,
                   int flgs) {
  NsfObject *object = (NsfObject *) object1;
  int result;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, object, framePtr);
  if (object->nsPtr)
    flgs |= TCL_NAMESPACE_ONLY;

  result = Tcl_UnsetVar2(interp, name1, name2, flgs);
  Nsf_PopFrameObj(interp, framePtr);
  return result;
}

static int
GetInstVarIntoCurrentScope(Tcl_Interp *interp, const char *cmdName, NsfObject *object,
                           Tcl_Obj *varName, Tcl_Obj *newName) {
  Var *varPtr = NULL, *otherPtr = NULL, *arrayPtr;
  int new = 0, flgs = TCL_LEAVE_ERR_MSG;
  Tcl_CallFrame *varFramePtr;
  CallFrame frame, *framePtr = &frame;
  char *varNameString;

  if (CheckVarName(interp, ObjStr(varName)) != TCL_OK) {
    return TCL_ERROR;
  }

  Nsf_PushFrameObj(interp, object, framePtr);
  if (object->nsPtr) {
    flgs = flgs|TCL_NAMESPACE_ONLY;
  }

  otherPtr = TclObjLookupVar(interp, varName, NULL, flgs, "define",
                             /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
  Nsf_PopFrameObj(interp, framePtr);

  if (otherPtr == NULL) {
    return NsfPrintError(interp, "can't import variable %s into method scope: "
			 "can't find variable on %s", 
			 ObjStr(varName), ObjectName(object));
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
      return NsfPrintError(interp, "can't make instance variable %s on %s: " 
			   "Variable cannot be an element in an array; use e.g. an alias.", 
			   ObjStr(varName), ObjectName(object));
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
      if (varPtr == otherPtr) {
        return NsfPrintError(interp, "can't instvar to variable itself");
      }
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
        return NsfPrintError(interp, "varname '%s' exists already", varNameString);
      } else if (TclIsVarTraced(varPtr)) {
        return NsfPrintError(interp, "varname '%s' has traces: can't use for instvar", varNameString);
      }
    }

    TclSetVarLink(varPtr);
    TclClearVarUndefined(varPtr);
    varPtr->value.linkPtr = otherPtr;
    VarHashRefCount(otherPtr)++;

    /* fprintf(stderr, "defining an alias var='%s' in obj %s fwd %d flags %x isLink %d isTraced %d isUndefined %d\n",
            ObjStr(newName), ObjectName(object),
            0,
            varPtr->flags,
            TclIsVarLink(varPtr), TclIsVarTraced(varPtr), TclIsVarUndefined(varPtr));
    */
  } else {
    return NsfPrintError(interp, "%s cannot import variable '%s' into method scope; "
			 "not called from a method frame", cmdName, varNameString);
  }
  return TCL_OK;
}

extern int
NsfRemoveObjectMethod(Tcl_Interp *interp, Nsf_Object *object1, CONST char *methodName) {
  NsfObject *object = (NsfObject*) object1;

  AliasDelete(interp, object->cmdName, methodName, 1);

  if (object->opt)
    AssertionRemoveProc(object->opt->assertions, methodName);

  if (object->nsPtr) {
    int rc = NSDeleteCmd(interp, object->nsPtr, methodName);
    if (rc < 0)
      return NsfPrintError(interp, "%s cannot delete method '%s' of object %s", 
			   ObjectName(object), methodName, ObjectName(object));
  }
  return TCL_OK;
}

extern int
NsfRemoveClassMethod(Tcl_Interp *interp, Nsf_Class *class, CONST char *methodName) {
  NsfClass *cl = (NsfClass*) class;
  NsfClassOpt *opt = cl->opt;
  int rc;

  AliasDelete(interp, class->object.cmdName, methodName, 0);

  if (opt && opt->assertions)
    AssertionRemoveProc(opt->assertions, methodName);

  rc = NSDeleteCmd(interp, cl->nsPtr, methodName);
  if (rc < 0) {
    return NsfPrintError(interp, "%s: cannot delete method '%s'", ClassName(cl), methodName);
  }
  return TCL_OK;
}

/*
 * obj/cl ClientData setter/getter
 */
extern void
NsfSetObjClientData(Nsf_Object *object1, ClientData data) {
  NsfObject *object = (NsfObject*) object1;
  NsfObjectOpt *opt = NsfRequireObjectOpt(object);
  opt->clientData = data;
}
extern ClientData
NsfGetObjClientData(Nsf_Object *object1) {
  NsfObject *object = (NsfObject*) object1;
  return (object && object->opt) ? object->opt->clientData : NULL;
}
extern void
NsfSetClassClientData(Nsf_Class *cli, ClientData data) {
  NsfClass *cl = (NsfClass*) cli;
  NsfRequireClassOpt(cl);
  cl->opt->clientData = data;
}
extern ClientData
NsfGetClassClientData(Nsf_Class *cli) {
  NsfClass *cl = (NsfClass*) cli;
  return (cl && cl->opt) ? cl->opt->clientData : NULL;
}

static int
SetInstVar(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *nameObj, Tcl_Obj *valueObj) {
  Tcl_Obj *resultObj;
  int flags = (object->nsPtr) ? TCL_LEAVE_ERR_MSG|TCL_NAMESPACE_ONLY : TCL_LEAVE_ERR_MSG;
  CallFrame frame, *framePtr = &frame;
  Nsf_PushFrameObj(interp, object, framePtr);

  if (valueObj == NULL) {
    resultObj = Tcl_ObjGetVar2(interp, nameObj, NULL, flags);
  } else {
    /*fprintf(stderr, "setvar in obj %s: name %s = %s\n", ObjectName(object), ObjStr(nameObj), ObjStr(value));*/
    resultObj = Tcl_ObjSetVar2(interp, nameObj, NULL, valueObj, flags);
  }
  Nsf_PopFrameObj(interp, framePtr);

  if (resultObj) {
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
  }
  return TCL_ERROR;
}

static int
NsfSetterMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  SetterCmdClientData *cd = (SetterCmdClientData*)clientData;
  NsfObject *object = cd->object;

  if (!object) return NsfObjErrType(interp, "setter", objv[0], "object", NULL);
  if (objc > 2) return NsfObjWrongArgs(interp, "wrong # args", object->cmdName, objv[0], "?value?");

  if (cd->paramsPtr && objc == 2) {
    Tcl_Obj *outObjPtr;
    int result, flags = 0;
    ClientData checkedData;

    result = ArgumentCheck(interp, objv[1], cd->paramsPtr,
			   RUNTIME_STATE(interp)->doCheckArguments,
			   &flags, &checkedData, &outObjPtr);

    if (result == TCL_OK) {
      result = SetInstVar(interp, object, objv[0], outObjPtr);

      if (flags & NSF_PC_MUST_DECR) {
        DECR_REF_COUNT(outObjPtr);
      }
    }
    return result;

  } else {
    return SetInstVar(interp, object, objv[0], objc == 2 ? objv[1] : NULL);
  }
}

static int
ForwardArg(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
           Tcl_Obj *ForwardArgObj, ForwardCmdClientData *tcd, Tcl_Obj **out,
           Tcl_Obj **freeList, int *inputArg, int *mapvalue,
           int firstPosArg, int *outputincr) {
  CONST char *ForwardArgString = ObjStr(ForwardArgObj), *p;
  int totalargs = objc + tcd->nr_args - 1;
  char c = *ForwardArgString, c1;

  /* per default every ForwardArgString from the processed list corresponds to exactly
     one ForwardArgString in the computed final list */
  *outputincr = 1;
  p = ForwardArgString;

  /*fprintf(stderr, "ForwardArg: processing '%s'\n", ForwardArgString);*/

  if (c == '%' && *(ForwardArgString+1) == '@') {
    char *remainder = NULL;
    long pos;
    ForwardArgString += 2;
    pos = strtol(ForwardArgString, &remainder, 0);
    /*fprintf(stderr, "strtol('%s) returned %ld '%s'\n", ForwardArgString, pos, remainder);*/
    if (ForwardArgString == remainder && *ForwardArgString == 'e'
        && !strncmp(ForwardArgString, "end", 3)) {
      pos = -1;
      remainder += 3;
    } else if (pos < 0) {
      pos --;
    }
    if (ForwardArgString == remainder || abs(pos) > totalargs) {
      return NsfPrintError(interp, "forward: invalid index specified in argument %s",
			   ObjStr(ForwardArgObj));
    }    if (!remainder || *remainder != ' ') {
      return NsfPrintError(interp, "forward: invalid syntax in '%s'; use: %@<pos> <cmd>", 
			   ObjStr(ForwardArgObj));
    }

    ForwardArgString = ++remainder;
    /* in case we address from the end, we reduct further to distinguish from -1 (void) */
    if (pos<0) pos--;
    /*fprintf(stderr, "remainder = '%s' pos = %ld\n", remainder, pos);*/
    *mapvalue = pos;
    ForwardArgString = remainder;
    c = *ForwardArgString;
  }

  if (c == '%') {
    Tcl_Obj *list = NULL, **listElements;
    int nrArgs = objc-1, nrPosArgs = objc-firstPosArg, nrElements = 0;
    char *firstActualArgument = nrArgs>0 ? ObjStr(objv[1]) : NULL;
    c = *++ForwardArgString;
    c1 = *(ForwardArgString+1);

    if (c == 's' && !strcmp(ForwardArgString, "self")) {
      *out = tcd->object->cmdName;
    } else if ((c == 'p' && !strcmp(ForwardArgString, "proc"))
	       || (c == 'm' && !strcmp(ForwardArgString, "method"))
	       ) {
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
        if (Tcl_ListObjIndex(interp, ForwardArgObj, 1, &list) != TCL_OK) {
          return NsfPrintError(interp, "forward: %%1 must be followed by a valid list, given: '%s'",
			       ObjStr(ForwardArgObj));
        }
        if (Tcl_ListObjGetElements(interp, list, &nrElements, &listElements) != TCL_OK) {
          return NsfPrintError(interp, "forward: %%1 contains invalid list '%s'", ObjStr(list));
        }
      } else if (tcd->subcommands) { /* deprecated part */
        if (Tcl_ListObjGetElements(interp, tcd->subcommands, &nrElements, &listElements) != TCL_OK) {
          return NsfPrintError(interp, "forward: %%1 contains invalid list '%s'", ObjStr(list));
        }
      }
      /*fprintf(stderr, "nrElements=%d, nra=%d firstPos %d objc %d\n",
        nrElements, nrArgs, firstPosArg, objc);*/

      if (nrElements > nrPosArgs) {
        /* insert default subcommand depending on number of arguments */
        /*fprintf(stderr, "inserting listElements[%d] '%s'\n", nrPosArgs,
          ObjStr(listElements[nrPosArgs]));*/
        *out = listElements[nrPosArgs];
      } else if (objc <= 1) {
	return NsfObjWrongArgs(interp, "wrong # args", objv[0], NULL, "option");
      } else {
        /*fprintf(stderr, "copying %%1: '%s'\n", ObjStr(objv[firstPosArg]));*/
        *out = objv[firstPosArg];
        *inputArg = firstPosArg+1;
      }
    } else if (c == '-') {
      CONST char *firstElementString;
      int i, insertRequired, done = 0;

      /*fprintf(stderr, "process flag '%s'\n", firstActualArgument);*/
      if (Tcl_ListObjGetElements(interp, ForwardArgObj, &nrElements, &listElements) != TCL_OK) {
        return NsfPrintError(interp, "forward: '%s' is not a valid list", ForwardArgString);
      }
      if (nrElements < 1 || nrElements > 2) {
        return NsfPrintError(interp, "forward: '%s': must contain 1 or 2 arguments", ForwardArgString);
      }
      firstElementString = ObjStr(listElements[0]);
      firstElementString++; /* we skip the dash */

      if (firstActualArgument && *firstActualArgument == '-') {
        /*fprintf(stderr, "we have a flag in first argument '%s'\n", firstActualArgument);*/

        for (i = 1; i < firstPosArg; i++) {
          if (strcmp(firstElementString, ObjStr(objv[i])) == 0) {
            /*fprintf(stderr, "We have a MATCH for '%s' oldInputArg %d\n", ForwardArgString, *inputArg);*/
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

    } else if (c == 'a' && !strncmp(ForwardArgString, "argcl", 4)) {
      if (Tcl_ListObjIndex(interp, ForwardArgObj, 1, &list) != TCL_OK) {
        return NsfPrintError(interp, "forward: %%argclindex must by a valid list, given: '%s'",
			     ForwardArgString);
      }
      if (Tcl_ListObjGetElements(interp, list, &nrElements, &listElements) != TCL_OK) {
        return NsfPrintError(interp, "forward: %%argclindex contains invalid list '%s'", ObjStr(list));
      }
      if (nrArgs >= nrElements) {
        return NsfPrintError(interp, "forward: not enough elements in specified list of ARGC argument %s",
			     ForwardArgString);
      }
      *out = listElements[nrArgs];
    } else if (c == '%') {
      Tcl_Obj *newarg = Tcl_NewStringObj(ForwardArgString, -1);
      *out = newarg;
      goto add_to_freelist;
    } else {
      /* evaluating given command */
      int result;
      /*fprintf(stderr, "evaluating '%s'\n", ForwardArgString);*/
      if ((result = Tcl_EvalEx(interp, ForwardArgString, -1, 0)) != TCL_OK)
        return result;
      *out = Tcl_DuplicateObj(Tcl_GetObjResult(interp));
      /*fprintf(stderr, "result = '%s'\n", ObjStr(*out));*/
      goto add_to_freelist;
    }
  } else {
    if (p == ForwardArgString)
      *out = ForwardArgObj;
    else {
      Tcl_Obj *newarg = Tcl_NewStringObj(ForwardArgString, -1);
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
CallForwarder(ForwardCmdClientData *tcd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ClientData clientData;
  int result;
  NsfObject *object = tcd->object;
  CallFrame frame, *framePtr = &frame;

  if (tcd->verbose) {
    Tcl_Obj *cmd = Tcl_NewListObj(objc, objv);
    fprintf(stderr, "forwarder calls '%s'\n", ObjStr(cmd));
    DECR_REF_COUNT(cmd);
  }
  if (tcd->objframe) {
    Nsf_PushFrameObj(interp, object, framePtr);
  }
  if (tcd->objProc) {
    /* fprintf(stderr, "CallForwarder Tcl_NRCallObjProc %p\n", clientData);*/
    result = Tcl_NRCallObjProc(interp, tcd->objProc, tcd->clientData, objc, objv);
  } else if (IsNsfTclObj(interp, tcd->cmdName, (NsfObject**)&clientData)) {
    /*fprintf(stderr, "CallForwarder NsfObjDispatch object %s, objc=%d\n",
      ObjStr(tcd->cmdName), objc);*/
    if (objc > 1) {
      result = ObjectDispatch(clientData, interp, objc, objv, NSF_CSC_IMMEDIATE);
    } else {
      result = DispatchDefaultMethod(clientData, interp, objc, objv, NSF_CSC_IMMEDIATE);
    }
  } else {
    /*fprintf(stderr, "CallForwarder: no nsf object %s\n", ObjStr(tcd->cmdName));*/
    result = Tcl_EvalObjv(interp, objc, objv, 0);
  }

  if (tcd->objframe) {
    Nsf_PopFrameObj(interp, framePtr);
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
NsfForwardMethod(ClientData clientData, Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[]) {
  ForwardCmdClientData *tcd = (ForwardCmdClientData *)clientData;
  int result, j, inputArg = 1, outputArg = 0;

  if (!tcd || !tcd->object) return NsfObjErrType(interp, "forwarder", objv[0], "object", NULL);

  if (tcd->passthrough) { /* two short cuts for simple cases */
    /* early binding, cmd *resolved, we have to care only for objscope */
    return CallForwarder(tcd, interp, objc, objv);
  } else if (!tcd->args && *(ObjStr(tcd->cmdName)) != '%') {
    /* we have ony to replace the method name with the given cmd name */
    ALLOC_ON_STACK(Tcl_Obj*, objc, ov);
    /*fprintf(stderr, "+++ forwardMethod must subst \n");*/
    memcpy(ov, objv, sizeof(Tcl_Obj *)*objc);
    ov[0] = tcd->cmdName;
    result = CallForwarder(tcd, interp, objc, ov);
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
    if ((result = ForwardArg(interp, objc, objv, tcd->cmdName, tcd,
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
        if ((result = ForwardArg(interp, objc, objv, listElements[j], tcd,
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

    result = CallForwarder(tcd, interp, objc, ov);

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
NsfProcAliasMethod(ClientData clientData,
                     Tcl_Interp *interp, int objc,
                     Tcl_Obj *CONST objv[]) {
  AliasCmdClientData *tcd = (AliasCmdClientData *)clientData;
  NsfObject *self = tcd->object;
  CONST char *methodName = ObjStr(objv[0]);

  assert(tcd->object==GetSelfObj(interp));

  if (self == NULL) {
    return NsfPrintError(interp, "no object active for alias '%s'; "
			 "don't call aliased methods via namespace paths",
			 Tcl_GetCommandName(interp, tcd->aliasCmd));
  }
  return MethodDispatch((ClientData)self, interp, objc, objv, tcd->aliasedCmd, self, tcd->class,
                        methodName, 0, 0);
}

static int
NsfObjscopedMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  AliasCmdClientData *tcd = (AliasCmdClientData *)clientData;
  NsfObject *object = tcd->object;
  CallFrame frame, *framePtr = &frame;
  int result;

  /*fprintf(stderr, "objscopedMethod obj=%p %s, ptr=%p\n", object, ObjectName(object), tcd->objProc);*/

  Nsf_PushFrameObj(interp, object, framePtr);
  result = Tcl_NRCallObjProc(interp, tcd->objProc, tcd->clientData, objc, objv);
  Nsf_PopFrameObj(interp, framePtr);

  return result;
}

static void
SetterCmdDeleteProc(ClientData clientData) {
  SetterCmdClientData *setterClientData = (SetterCmdClientData *)clientData;

  if (setterClientData->paramsPtr) {
    ParamsFree(setterClientData->paramsPtr);
  }
  FREE(SetterCmdClientData, setterClientData);
}

static void
AliasCmdDeleteProc(ClientData clientData) {
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
      RUNTIME_STATE(tcd->interp)->exitHandlerDestroyRound != NSF_EXITHANDLER_ON_PHYSICAL_DESTROY) {
    CONST char *methodName = Tcl_GetCommandName(tcd->interp, tcd->aliasCmd);
    AliasDelete(tcd->interp, tcd->cmdName, methodName, tcd->class == NULL);
  }

  /*fprintf(stderr, "AliasCmdDeleteProc\n");*/
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
IsDashArg(Tcl_Interp *interp, Tcl_Obj *obj, int firstArg, CONST char **methodName, int *objc, Tcl_Obj **objv[]) {
  CONST char *flag;
  assert(obj);

  if (obj->typePtr == Nsf_OT_listType) {
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
CallConfigureMethod(Tcl_Interp *interp, NsfObject *object, CONST char *methodName,
                    int argc, Tcl_Obj *CONST argv[]) {
  int result;
  Tcl_Obj *methodObj = Tcl_NewStringObj(methodName, -1);

  /* fprintf(stderr, "CallConfigureMethod method %s->'%s' level %d, argc %d\n",
     ObjectName(object), methodName, level, argc);*/

  if (isInitString(methodName)) {
    object->flags |= NSF_INIT_CALLED;
  }

  Tcl_ResetResult(interp);
  INCR_REF_COUNT(methodObj);
  result = CallMethod((ClientData)object, interp, methodObj, argc, argv,
		      NSF_CM_NO_UNKNOWN|NSF_CSC_IMMEDIATE);
  DECR_REF_COUNT(methodObj);

  /*fprintf(stderr, "method  '%s' called args: %d o=%p, result=%d %d\n",
    methodName, argc+1, obj, result, TCL_ERROR);*/

  if (result != TCL_OK) {
    Tcl_Obj *res =  Tcl_DuplicateObj(Tcl_GetObjResult(interp)); /* save the result */
    INCR_REF_COUNT(res);
    NsfPrintError(interp, "%s during '%s.%s", ObjStr(res), ObjectName(object), methodName);
    DECR_REF_COUNT(res);
  }

  return result;
}


/*
 * class method implementations
 */

static int
IsRootNamespace(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  NsfObjectSystem *osPtr;

  for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = osPtr->nextPtr) {
    Tcl_Command cmd = osPtr->rootClass->object.id;
    if ((Tcl_Namespace *)((Command *)cmd)->nsPtr == nsPtr) {
      return 1;
    }
  }
  return 0;
}

static Tcl_Namespace *
CallingNameSpace(Tcl_Interp *interp) {
  Tcl_CallFrame *framePtr;
  Tcl_Namespace *nsPtr;

  /*NsfShowStack(interp);*/

  /*
  * Find last incovation outside the Next Scripting system namespaces. For
  * example, the pre defined slot handlers for relations (defined in
  * the too namespace) handle mixin and class registration. etc. If we
  * would use this namespace, we would resolve non-fully-qualified
  * names against the root namespace).
  */
  framePtr = CallStackGetActiveProcFrame((Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp));
  //framePtr = BeginOfCallChain(interp, GetSelfObj(interp));

  for (; framePtr; framePtr = Tcl_CallFrame_callerVarPtr(framePtr)) {
    nsPtr = Tcl_CallFrame_nsPtr(framePtr);
	
    if (IsRootNamespace(interp, nsPtr)) {
      /*fprintf(stderr, "... %p skip %s\n", framePtr, nsPtr->fullName);*/
      continue;
    }
    /*fprintf(stderr, "... %p take %s\n", framePtr, nsPtr->fullName); */
    break;
  }

  if (!framePtr) {
    nsPtr = Tcl_GetGlobalNamespace(interp);
  }

  /*fprintf(stderr, " **** CallingNameSpace: returns %p %s framePtr %p\n",
    nsPtr, nsPtr ? nsPtr->fullName:"(null)", framePtr);*/
  return nsPtr;
}

/***********************************
 * argument handling
 ***********************************/

static int
ArgumentError(Tcl_Interp *interp, CONST char *errorMsg, NsfParam CONST *paramPtr,
              Tcl_Obj *cmdNameObj, Tcl_Obj *methodObj) {
  Tcl_Obj *argStringObj = ParamDefsSyntax(interp, paramPtr);

  NsfObjWrongArgs(interp, errorMsg, cmdNameObj, methodObj, ObjStr(argStringObj));
  DECR_REF_COUNT(argStringObj);

  return TCL_ERROR;
}

#include "tclAPI.h"

static int
ArgumentCheckHelper(Tcl_Interp *interp, Tcl_Obj *objPtr, struct NsfParam CONST *pPtr, int *flags,
                        ClientData *clientData, Tcl_Obj **outObjPtr) {
  int objc, i, result;
  Tcl_Obj **ov;

  /*fprintf(stderr, "ArgumentCheckHelper\n");*/
  assert(pPtr->flags & NSF_ARG_MULTIVALUED);

  result = Tcl_ListObjGetElements(interp, objPtr, &objc, &ov);
  if (result != TCL_OK) {
    return result;
  }

  *outObjPtr = Tcl_NewListObj(0, NULL);
  INCR_REF_COUNT(*outObjPtr);

  for (i=0; i<objc; i++) {
    Tcl_Obj *elementObjPtr;
    const char *valueString = ObjStr(ov[i]);

    if (pPtr->flags & NSF_ARG_ALLOW_EMPTY && *valueString == '\0') {
      result = ConvertToString(interp, ov[i], pPtr, clientData, &elementObjPtr);
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
      NsfPrintError(interp, "invalid value in \"%s\": %s", ObjStr(objPtr), ObjStr(resultObj));
      DECR_REF_COUNT(resultObj);
      DECR_REF_COUNT(*outObjPtr);
      *flags &= ~NSF_PC_MUST_DECR;
      *outObjPtr = objPtr;
      break;
    }
  }
  return result;
}

static int
ArgumentCheck(Tcl_Interp *interp, Tcl_Obj *objPtr, struct NsfParam CONST *pPtr, int doCheck,
	      int *flags, ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result;

  if (doCheck == 0 && (pPtr->flags & (NSF_ARG_IS_CONVERTER|NSF_ARG_INITCMD)) == 0) {
    /*fprintf(stderr, "*** omit  argument check for arg %s flags %.6x\n",pPtr->name, pPtr->flags);*/
    *outObjPtr = objPtr;
    *clientData = ObjStr(objPtr);
    *flags = 0;
    return TCL_OK;
  }

  if (pPtr->flags & NSF_ARG_MULTIVALUED) {
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

    if (objc == 0 && ((pPtr->flags & NSF_ARG_ALLOW_EMPTY) == 0)) {
      return NsfPrintError(interp, "invalid parameter value: list is not allowed to be empty");
    }

    /*
     * Default assumption: outObjPtr is not modified, in cases where
     * necessary, we switch to the helper function
     */
    *outObjPtr = objPtr;

    for (i=0; i<objc; i++) {
      Tcl_Obj *elementObjPtr;
      const char *valueString = ObjStr(ov[i]);

      if (0 && /* TODO: REMOVE ME */ pPtr->flags & NSF_ARG_ALLOW_EMPTY && *valueString == '\0') {
	result = ConvertToString(interp, ov[i], pPtr, clientData, &elementObjPtr);
      } else {
	result = (*pPtr->converter)(interp, ov[i], pPtr, clientData, &elementObjPtr);
      }

      if (result == TCL_OK) {
        if (ov[i] != elementObjPtr) {
          /*
	   * The elementObjPtr differs from the input Tcl_Obj, we
           * switch to the version of this handler building an output
           * list.
	   */
          /*fprintf(stderr, "switch to output list construction for value %s\n",
	    ObjStr(elementObjPtr));*/
          *flags |= NSF_PC_MUST_DECR;
          result = ArgumentCheckHelper(interp, objPtr, pPtr, flags, clientData, outObjPtr);
          break;
        }
      } else {
        Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
        INCR_REF_COUNT(resultObj);
        NsfPrintError(interp, "invalid value in \"%s\": %s", ObjStr(objPtr), ObjStr(resultObj));
        DECR_REF_COUNT(resultObj);
        break;
      }
    }
  } else {
    const char *valueString = ObjStr(objPtr);
    if (pPtr->flags & NSF_ARG_ALLOW_EMPTY && *valueString == '\0') {
      result = ConvertToString(interp, objPtr, pPtr, clientData, outObjPtr);
    } else {
      result = (*pPtr->converter)(interp, objPtr, pPtr, clientData, outObjPtr);
    }
  }
  return result;
}

static int
ArgumentDefaults(ParseContext *pcPtr, Tcl_Interp *interp,
                 NsfParam CONST *ifd, int nrParams) {
  NsfParam CONST *pPtr;
  int i;

  for (pPtr = ifd, i=0; i<nrParams; pPtr++, i++) {
    /*fprintf(stderr, "ArgumentDefaults got for arg %s (%d) %p => %p %p, default %s\n",
            pPtr->name, pPtr->flags & NSF_ARG_REQUIRED, pPtr,
            pcPtr->clientData[i], pcPtr->objv[i],
            pPtr->defaultValue ? ObjStr(pPtr->defaultValue) : "NONE");*/

    if (pcPtr->objv[i]) {
      /* we got an actual value, which was already checked by objv parser */
      /*fprintf(stderr, "setting passed value for %s to '%s'\n", pPtr->name, ObjStr(pcPtr->objv[i]));*/
      if (pPtr->converter == ConvertToSwitch) {
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
	
	/*
	 * Mark that this argument gets the default value
	 */
	pcPtr->flags[i] |= NSF_PC_IS_DEFAULT;

        /* we have a default, do we have to subst it? */
        if (pPtr->flags & NSF_ARG_SUBST_DEFAULT) {
          int result = SubstValue(interp, pcPtr->object, &newValue);
          if (result != TCL_OK) {
            return result;
          }
          /*fprintf(stderr, "attribute %s default %p %s => %p '%s'\n", pPtr->name,
                  pPtr->defaultValue, ObjStr(pPtr->defaultValue),
                  newValue, ObjStr(newValue));*/

          /* the according DECR is performed by ParseContextRelease() */
          INCR_REF_COUNT(newValue);
          mustDecrNewValue = 1;
          pcPtr->flags[i] |= NSF_PC_MUST_DECR;
          pcPtr->status |= NSF_PC_STATUS_MUST_DECR;
        } else {
          mustDecrNewValue = 0;
        }

        pcPtr->objv[i] = newValue;
        /*fprintf(stderr, "==> setting default value '%s' for var '%s' flag %d type %s conv %p\n",
                ObjStr(newValue), pPtr->name, pPtr->flags & NSF_ARG_INITCMD,
                pPtr->type, pPtr->converter);*/

        /* Check the default value, unless we have an INITCMD or METHOD */
        if ((pPtr->flags & (NSF_ARG_INITCMD|NSF_ARG_METHOD)) == 0) {
          int mustDecrList = 0;
          if (ArgumentCheck(interp, newValue, pPtr,
			    RUNTIME_STATE(interp)->doCheckArguments,
			    &mustDecrList, &checkedData, &pcPtr->objv[i]) != TCL_OK) {
            return TCL_ERROR;
          }

	  if (pcPtr->objv[i] != newValue) {
	    /*
	     * The output Tcl_Obj differs from the input, so the
	     * Tcl_Obj was converted; in case we have set prevously
	     * must_decr on newValue, we decr the refcount on newValue
	     * here and clear the flag.
	     */
	    if (mustDecrNewValue) {
	      DECR_REF_COUNT(newValue);
	      pcPtr->flags[i] &= ~NSF_PC_MUST_DECR;
	    }
            /*
	     * The new output value itself might require a decr, so
             * set the flag here if required; this is just necessary
             * for multivalued converted output.
	     */
            if (mustDecrList) {
	      pcPtr->flags[i] |= NSF_PC_MUST_DECR;
	      pcPtr->status |= NSF_PC_STATUS_MUST_DECR;
            }
	  }
        }
      } else if (pPtr->flags & NSF_ARG_REQUIRED) {
        return NsfPrintError(interp, "%s%s%s: required argument '%s' is missing",
			     pcPtr->object ? ObjectName(pcPtr->object) : "",
			     pcPtr->object ? " " : "",
			     ObjStr(pcPtr->full_objv[0]),
			     pPtr->nameObj ? ObjStr(pPtr->nameObj) : pPtr->name);
      } else {
        /*
	 * Use as dummy default value an arbitrary symbol, which must
         * not be returned to the Tcl level level; this value is unset
         * later by NsfUnsetUnknownArgsCmd().
         */
        pcPtr->objv[i] = NsfGlobalObjs[NSF___UNKNOWN__];
      }
    }
  }
  return TCL_OK;
}

static int
ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
              NsfObject *object, Tcl_Obj *procNameObj,
              NsfParam CONST *paramPtr, int nrParams, int doCheck,
              ParseContext *pcPtr) {
  int i, o, flagCount, nrReq = 0, nrOpt = 0, dashdash = 0, nrDashdash = 0;
  NsfParam CONST *pPtr;

  ParseContextInit(pcPtr, nrParams, object, procNameObj);

#if defined(PARSE_TRACE)
  fprintf(stderr, "BEGIN (%d) [0]%s ", objc, ObjStr(procNameObj));
  for (o=1; o<objc; o++) {fprintf(stderr, "[%d]%s ", o, ObjStr(objv[o]));}
  fprintf(stderr, "\n");
#endif

  for (i = 0, o = 1, pPtr = paramPtr; pPtr->name && o < objc;) {
#if defined(PARSE_TRACE_FULL)
    fprintf(stderr, "... (%d) processing [%d]: '%s' %s\n", i, o,
            pPtr->name, pPtr->flags & NSF_ARG_REQUIRED ? "req":"not req");
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
          NsfParam CONST *nppPtr;
	  /* We have an argument starting with a "-"; is it really one of the specified flags? */

          for (nppPtr = pPtr; nppPtr->name && *nppPtr->name == '-'; nppPtr ++) {
            if (strcmp(objStr, nppPtr->name) == 0) {
	      int j = nppPtr-paramPtr;
              /*fprintf(stderr, "...     flag '%s' o=%d p=%d, objc=%d nrArgs %d\n", objStr, o, p, objc, nppPtr->nrArgs);*/
              if (nppPtr->flags & NSF_ARG_REQUIRED) nrReq++; else nrOpt++;
              if (nppPtr->nrArgs == 0) {
                pcPtr->clientData[j] = (ClientData)1;  /* the flag was given */
                pcPtr->objv[j] = NsfGlobalObjs[NSF_ONE];
              } else {
                /* we assume for now, nrArgs is at most 1 */
                o++; p++;
                if (nppPtr->flags & NSF_ARG_REQUIRED) nrReq++; else nrOpt++;

                if (p < objc) {
#if defined(PARSE_TRACE_FULL)
		  fprintf(stderr, "...     setting cd[%d] '%s' = %s (%d) %s converter %p\n",
                          i, nppPtr->name, ObjStr(objv[p]), nppPtr->nrArgs,
                          nppPtr->flags & NSF_ARG_REQUIRED ? "req" : "not req", nppPtr->converter);
#endif
                  if (ArgumentCheck(interp, objv[p], nppPtr, doCheck,
				    &pcPtr->flags[j], &pcPtr->clientData[j], &pcPtr->objv[j]) != TCL_OK) {
                    return TCL_ERROR;
                  }

		  if (pcPtr->flags[j] & NSF_ARG_SET) {
		    NsfLog(interp, NSF_LOG_WARN, "Non-positional parameter %s was passed more than once",
			   nppPtr->name);
		  }
		  pcPtr->flags[j] |= NSF_ARG_SET;

                  if (pcPtr->flags[j] & NSF_PC_MUST_DECR) {
		    pcPtr->status |= NSF_PC_STATUS_MUST_DECR;
		  }
		
                } else {
                  return NsfPrintError(interp, "Argument for parameter '%s' expected", objStr);
                }
              }
              flagCount++;
              found = 1;
              break;
            }
          }
          if (!found) {
            /*
	     * We did not find the specified flag, the thing starting
	     * with a '-' must be an argument
	     */
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

      if (pPtr->flags & NSF_ARG_REQUIRED) nrReq++; else nrOpt++;
      /*fprintf(stderr, "... arg %s req %d converter %p try to set on %d: '%s' ConvertViaCmd %p\n",
              pPtr->name, pPtr->flags & NSF_ARG_REQUIRED, pPtr->converter, i, ObjStr(objv[o]),
              ConvertViaCmd);*/

      if (ArgumentCheck(interp, objv[o], pPtr, doCheck,
			&pcPtr->flags[i], &pcPtr->clientData[i], &pcPtr->objv[i]) != TCL_OK) {
        return TCL_ERROR;
      }
      if (pcPtr->flags[i] & NSF_PC_MUST_DECR) {
	pcPtr->status |= NSF_PC_STATUS_MUST_DECR;
      }
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
  for (; pPtr->name; pPtr++) {
    if (pPtr->flags & NSF_ARG_REQUIRED) nrReq++; else nrOpt++;
  }

  /* is last argument a vararg? */
  pPtr--;
  if (pPtr->converter == ConvertToNothing) {
    pcPtr->varArgs = 1;
    /*fprintf(stderr, "last arg of proc '%s' is varargs\n", ObjStr(procNameObj));*/
  }

  /* 
   * Handle missing or unexpected arguments for methods and cmds 
   */
  if (pcPtr->lastobjc < nrReq) {
    return ArgumentError(interp, "not enough arguments:", paramPtr, 
			 object ? object->cmdName : NULL, 
			 procNameObj); 
  }
  if (!pcPtr->varArgs && objc-nrDashdash-1 > nrReq + nrOpt) {
    Tcl_DString ds, *dsPtr = &ds;
    DSTRING_INIT(dsPtr);
    Tcl_DStringAppend(dsPtr, "Invalid argument '", -1);
    Tcl_DStringAppend(dsPtr, ObjStr(objv[pcPtr->lastobjc+1]), -1);
    Tcl_DStringAppend(dsPtr, "', maybe too many arguments;", -1);
    return ArgumentError(interp, Tcl_DStringValue(dsPtr), paramPtr, 
			 object ? object->cmdName : NULL, 
			 procNameObj); 
    DSTRING_FREE(dsPtr);
    return TCL_ERROR;
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

  if (pattern && NoMetaChars(pattern)) {
    Tcl_Obj *patternObj = Tcl_NewStringObj(pattern, -1);
    INCR_REF_COUNT(patternObj);

    hPtr = tablePtr ? Tcl_CreateHashEntry(tablePtr, (char *)patternObj, NULL) : NULL;
    if (hPtr) {
      Var  *val = VarHashGetValue(hPtr);
      Tcl_SetObjResult(interp, VarHashGetKey(val));
    } else {
      Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_EMPTY]);
    }
    DECR_REF_COUNT(patternObj);
  } else {
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);
    Tcl_HashSearch hSrch;
    hPtr = tablePtr ? Tcl_FirstHashEntry(tablePtr, &hSrch) : NULL;
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
    /* dereference the Next Scripting alias chain */
    if (Tcl_Command_deleteProc(cmd) == AliasCmdDeleteProc) {
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
  Tcl_Obj *methodObj;
  if (procPtr) {
    CONST char *body = ObjStr(procPtr->bodyPtr);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(StripBodyPrefix(body), -1));
    return TCL_OK;
  }
  methodObj = Tcl_NewStringObj(methodName, -1);
  INCR_REF_COUNT(methodObj);
  NsfObjErrType(interp, "info body", methodObj, "a name of a scriped method", NULL);
  DECR_REF_COUNT(methodObj);
  return TCL_ERROR;
}

static Tcl_Obj *
ListParamDefs(Tcl_Interp *interp, NsfParam CONST *paramsPtr, int style) {
  Tcl_Obj *listObj;

  switch (style) {
  case 0: listObj = ParamDefsFormat(interp, paramsPtr); break;
  case 1: listObj = ParamDefsList(interp, paramsPtr); break;
  case 2: listObj = ParamDefsSyntax(interp, paramsPtr); break;
  default: listObj = NULL;
  }

  return listObj;
}


/*
 *----------------------------------------------------------------------
 * ListCmdParams --
 *
 *    Obtains a cmd and a method name and sets as side effect the Tcl
 *    result to either the list of the parameters (withVarnames == 0),
 *    to the args (withVarnames == 1) or to the parametersyntax
 *    (withVarnames == 2).
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Sets interp result
 *
 *----------------------------------------------------------------------
 */

static int
ListCmdParams(Tcl_Interp *interp, Tcl_Command cmd, CONST char *methodName, int withVarnames) {
  Proc *procPtr = GetTclProcFromCommand(cmd);

  assert(methodName);

  if (procPtr) {
    NsfParamDefs *paramDefs = procPtr ? ParamDefsGet((Tcl_Command)procPtr->cmdPtr) : NULL;
    Tcl_Obj *list;

    if (paramDefs && paramDefs->paramsPtr) {
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

	if (withVarnames == 2 && strcmp(args->name, "args") == 0) {
	  if (args != procPtr->firstLocalPtr) {
	    Tcl_AppendToObj(list, " ", 1);
	  }
	  Tcl_AppendToObj(list, "?arg ...?", 9);
	} else {
	  innerlist = Tcl_NewListObj(0, NULL);
	  Tcl_ListObjAppendElement(interp, innerlist, Tcl_NewStringObj(args->name, -1));
	  if (!withVarnames && args->defValuePtr) {
	    Tcl_ListObjAppendElement(interp, innerlist, args->defValuePtr);
	  }
	  Tcl_ListObjAppendElement(interp, list, innerlist);
	}
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
        NsfParamDefs paramDefs = {mdPtr->paramDefs, mdPtr->nrParameters};
	Tcl_Obj *list = ListParamDefs(interp, paramDefs.paramsPtr, withVarnames);

        Tcl_SetObjResult(interp, list);
        return TCL_OK;
      }
    }

    if (((Command *)cmd)->objProc == NsfSetterMethod) {
      SetterCmdClientData *cd = (SetterCmdClientData *)Tcl_Command_objClientData(cmd);
      if (cd->paramsPtr) {
        Tcl_Obj *list;
        NsfParamDefs paramDefs;
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
    }

    /*
     * In case, we failed so far to obtain a result, try to use the
     * object-system implementors definitions in the gobal array
     * ::nsf::parametersyntax. Note that we can only obtain the
     * parametersyntax this way.
     */
    if (withVarnames == 2) {
      Command *cmdPtr = (Command *)cmd;
      Tcl_DString ds, *dsPtr = &ds;
      Tcl_Obj *parameterSyntaxObj;

      Tcl_DStringInit(dsPtr);
      if (strcmp("::", cmdPtr->nsPtr->fullName) != 0) {
	Tcl_DStringAppend(dsPtr, cmdPtr->nsPtr->fullName, -1);
      }
      Tcl_DStringAppend(dsPtr, "::", 2);
      Tcl_DStringAppend(dsPtr, methodName, -1);
      
      /*fprintf(stderr,"Looking up ::nsf::parametersyntax(%s) ...\n",Tcl_DStringValue(dsPtr));*/
      parameterSyntaxObj = Tcl_GetVar2Ex(interp, "::nsf::parametersyntax", 
					 Tcl_DStringValue(dsPtr), TCL_GLOBAL_ONLY);
      
      /*fprintf(stderr, "No parametersyntax so far methodname %s cmd name %s ns %s\n", 
	methodName, Tcl_GetCommandName(interp,cmd), Tcl_DStringValue(dsPtr));*/

      Tcl_DStringFree(dsPtr);
      if (parameterSyntaxObj) {
        Tcl_SetObjResult(interp, parameterSyntaxObj);
	return TCL_OK;
      }
    }

    if (((Command *)cmd)->objProc == NsfForwardMethod) {
      return NsfPrintError(interp, "info params: could not obtain parameter definition for forwarder '%s'",
                            methodName);
    } else if (((Command *)cmd)->objProc != NsfObjDispatch) {
      return NsfPrintError(interp, "info params: could not obtain parameter definition for method '%s'",
			   methodName);
    } else {
      /* procPtr == NsfObjDispatch, be quiet */
      return TCL_OK;
    }
  }
  
  {
    Tcl_Obj *methodObj = Tcl_NewStringObj(methodName, -1);
    INCR_REF_COUNT(methodObj);
    NsfObjErrType(interp, "info params", methodObj, "a method name", NULL);
    DECR_REF_COUNT(methodObj);
  }
  return TCL_ERROR;
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
  if (tcd->objProc) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("-earlybinding", -1));
  }
  if (tcd->objframe) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("-objframe", -1));
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
                         NsfObject *object, CONST char *methodName, Tcl_Command cmd,
                         int withObjframe, int withPer_object, int withProtection) {
  Tcl_ListObjAppendElement(interp, listObj, object->cmdName);
  if (withProtection) {
    Tcl_ListObjAppendElement(interp, listObj,
			     Tcl_Command_flags(cmd) & NSF_CMD_PROTECTED_METHOD
			     ? Tcl_NewStringObj("protected", 9)
			     : Tcl_NewStringObj("public", 6));
  }
  if (withPer_object) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("class-object", 12));
  }
  Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj(registerCmdName, -1));
  Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj(methodName, -1));

  if (withObjframe) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("-frame", 6));
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("object", 6));
  }
  if (Tcl_Command_flags(cmd) & NSF_CMD_NONLEAF_METHOD) {
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("-frame", 6));
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("method", 6));
  }
}

static int
ListMethod(Tcl_Interp *interp,
	   NsfObject *regObject,
	   NsfObject *defObject,
	   CONST char *methodName, Tcl_Command cmd,
           int subcmd, int withPer_object) {

  assert(methodName);

  /*fprintf(stderr, "ListMethod %s %s cmd %p subcmd %d per-object %d\n",
    ObjectName(regObject), methodName, cmd, subcmd, withPer_object);*/

  if (!cmd) {
    Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_EMPTY]);
  } else {
    Tcl_ObjCmdProc *procPtr = Tcl_Command_objProc(cmd);
    int outputPerObject = 0;
    Tcl_Obj *resultObj;

    if (!NsfObjectIsClass(regObject)) {
      withPer_object = 1;
      /* don't output "object" modifier, if regObject is not a class */
      outputPerObject = 0;
    } else {
      outputPerObject = withPer_object;
    }

    switch (subcmd) {
    case InfomethodsubcmdHandleIdx:
      {
        return ListMethodHandle(interp, regObject, withPer_object, methodName);
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
        NsfProcAssertion *procs;
        if (withPer_object) {
          procs = regObject->opt ? AssertionFindProcs(regObject->opt->assertions, methodName) : NULL;
        } else {
          NsfClass *class = (NsfClass *)regObject;
          procs = class->opt ? AssertionFindProcs(class->opt->assertions, methodName) : NULL;
        }
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->pre));
        return TCL_OK;
      }
    case InfomethodsubcmdPostconditionIdx:
      {
        NsfProcAssertion *procs;
        if (withPer_object) {
          procs = regObject->opt ? AssertionFindProcs(regObject->opt->assertions, methodName) : NULL;
        } else {
          NsfClass *class = (NsfClass *)regObject;
          procs = class->opt ? AssertionFindProcs(class->opt->assertions, methodName) : NULL;
        }
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->post));
        return TCL_OK;
      }
    case InfomethodsubcmdSubmethodsIdx:
      {
	if (procPtr == NsfObjDispatch) {
	  NsfObject *subObject = NsfGetObjectFromCmdPtr(cmd);
	  if (subObject) {
	    return ListDefinedMethods(interp, subObject, NULL, 1 /* per-object */,
				      NSF_METHODTYPE_ALL, CallprotectionAllIdx, 0, 1, 0);
	  }
	}
	/* all other cases return emtpy */
	Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_EMPTY]);
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
          NsfAssertionStore *assertions;

          resultObj = Tcl_NewListObj(0, NULL);
          /* todo: don't hard-code registering command name "method" / NSF_METHOD */
          AppendMethodRegistration(interp, resultObj, NsfGlobalStrings[NSF_METHOD],
                                   regObject, methodName, cmd, 0, outputPerObject, 1);
          ListCmdParams(interp, cmd, methodName, 0);
          Tcl_ListObjAppendElement(interp, resultObj, Tcl_GetObjResult(interp));
          ListProcBody(interp, GetTclProcFromCommand(cmd), methodName);
          Tcl_ListObjAppendElement(interp, resultObj, Tcl_GetObjResult(interp));

          if (withPer_object) {
            assertions = regObject->opt ? regObject->opt->assertions : NULL;
          } else {
            NsfClass *class = (NsfClass *)regObject;
            assertions = class->opt ? class->opt->assertions : NULL;
          }
          if (assertions) {
            NsfProcAssertion *procs = AssertionFindProcs(assertions, methodName);
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

    } else if (procPtr == NsfForwardMethod) {
      /* forwarder */
      switch (subcmd) {
      case InfomethodsubcmdTypeIdx:
        Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_FORWARD]);
        break;
      case InfomethodsubcmdDefinitionIdx:
        {
          ClientData clientData = cmd ? Tcl_Command_objClientData(cmd) : NULL;

          if (clientData) {
            resultObj = Tcl_NewListObj(0, NULL);
            /* todo: don't hard-code registering command name "forward" / NSF_FORWARD*/
            AppendMethodRegistration(interp, resultObj, NsfGlobalStrings[NSF_FORWARD],
                                     regObject, methodName, cmd, 0, outputPerObject, 1);
            AppendForwardDefinition(interp, resultObj, clientData);
            Tcl_SetObjResult(interp, resultObj);
            break;
          }
        }
      }

    } else if (procPtr == NsfSetterMethod) {
      /* setter methods */
      switch (subcmd) {
      case InfomethodsubcmdTypeIdx:
        Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_SETTER]);
        break;
      case InfomethodsubcmdDefinitionIdx: {
        SetterCmdClientData *cd = (SetterCmdClientData *)Tcl_Command_objClientData(cmd);

        resultObj = Tcl_NewListObj(0, NULL);
        /* todo: don't hard-code registering command name "setter" / NSF_SETTER */

        AppendMethodRegistration(interp, resultObj, NsfGlobalStrings[NSF_SETTER], regObject,
                                 cd->paramsPtr ? ObjStr(cd->paramsPtr->paramObj) : methodName,
                                 cmd, 0, outputPerObject, 1);
        Tcl_SetObjResult(interp, resultObj);
        break;
      }
      }
    } else {
      /*
       * The cmd must be an alias or object.
       *
       * Note that some aliases come with procPtr == NsfObjDispatch.
       * In order to dinstinguish between "object" and alias, we have
       * to do the lookup for the entryObj to determine wether it is
       * really an alias.
       */

      Tcl_Obj *entryObj;

      entryObj = AliasGet(interp, defObject->cmdName,
			  Tcl_GetCommandName(interp, cmd),
			  regObject != defObject ? 1 : withPer_object);
      /*
      fprintf(stderr, "aliasGet %s -> %s/%s (%d) returned %p\n",
	      ObjectName(defObject), methodName, Tcl_GetCommandName(interp, cmd),
	      withPer_object, entryObj);
      fprintf(stderr, "... regObject %p %s\n",regObject,ObjectName(regObject));
      fprintf(stderr, "... defObject %p %s\n",defObject,ObjectName(defObject));
      */

      if (entryObj) {
	/* is an alias */
	switch (subcmd) {
	case InfomethodsubcmdTypeIdx:
	  Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_ALIAS]);
	  break;
	case InfomethodsubcmdDefinitionIdx:
	  {
            int nrElements;
            Tcl_Obj **listElements;
            resultObj = Tcl_NewListObj(0, NULL);
            Tcl_ListObjGetElements(interp, entryObj, &nrElements, &listElements);
            /* todo: don't hard-code registering command name "alias" / NSF_ALIAS */
            AppendMethodRegistration(interp, resultObj, NsfGlobalStrings[NSF_ALIAS],
                                     regObject, methodName, cmd, 
				     procPtr == NsfObjscopedMethod,
				     outputPerObject, 1);
            Tcl_ListObjAppendElement(interp, resultObj, listElements[nrElements-1]);
            Tcl_SetObjResult(interp, resultObj);
            break;
          }
        }
      } else {
	/* check, to be on the safe side */
	if (procPtr == NsfObjDispatch) {
	  /* the command is an object */
	  switch (subcmd) {
	  case InfomethodsubcmdTypeIdx:
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("object", -1));
	    break;
	  case InfomethodsubcmdDefinitionIdx:
	    {
	      NsfObject *subObject = NsfGetObjectFromCmdPtr(cmd);
	      assert(subObject);
	      resultObj = Tcl_NewListObj(0, NULL);
	      /* we can make
		    <class> create <parent::child>
		 or something similar to the other definition cmds
                     <parent> createChild <child> <class>
	      */
	      AppendMethodRegistration(interp, resultObj, "create",
				       &(subObject->cl)->object,
				       ObjStr(subObject->cmdName), cmd, 0, 0, 0);
	      /*
	      AppendMethodRegistration(interp, resultObj, "subobject",
				       object, methodName, cmd, 0, 0);
	      Tcl_ListObjAppendElement(interp, resultObj, subObject->cmdName);*/

	      Tcl_SetObjResult(interp, resultObj);
	      break;
	    }
	  }
	} else {
	  /* should never happen */
	  NsfLog(interp, NSF_LOG_WARN, "Could not obtain alias definition for %s. "
		 "Maybe someone deleted the alias %s for object %s?",
		 methodName, methodName, ObjectName(regObject));
	  Tcl_ResetResult(interp);
	}
      }
    }
  }
  return TCL_OK;
}

static int
ProtectionMatches(Tcl_Interp *interp, int withCallprotection, Tcl_Command cmd) {
  int result, isProtected = Tcl_Command_flags(cmd) & NSF_CMD_PROTECTED_METHOD;
  if (withCallprotection == CallprotectionNULL) {
    withCallprotection = CallprotectionPublicIdx;
  }
  switch (withCallprotection) {
  case CallprotectionAllIdx: result = 1; break;
  case CallprotectionPublicIdx: result = (isProtected == 0); break;
  case CallprotectionProtectedIdx: result = (isProtected != 0); break;
  default: result = 1;
  }
  return result;
}

static int MethodSourceMatches(Tcl_Interp *interp, int withSource, NsfClass *cl, NsfObject *object) {
  int isBaseClass;
  if (withSource == SourceAllIdx) {
    return 1;
  }
  if (cl == NULL) {
    /* If the method is object specific, it can't be from a baseclass
     * and must be application specfic.
     */
    return (withSource == SourceApplicationIdx && !IsBaseClass((NsfClass *)object));
  }
  isBaseClass = IsBaseClass(cl);
  if (withSource == SourceBaseclassesIdx && isBaseClass) {
    return 1;
  } else if (withSource == SourceApplicationIdx && !isBaseClass) {
    return 1;
  }
  return 0;
}

static int
MethodTypeMatches(Tcl_Interp *interp, int methodType, Tcl_Command cmd,
                  NsfObject *object, CONST char *key, int withPer_object, int *isObject) {
  Tcl_Command importedCmd;
  Tcl_ObjCmdProc *proc, *resolvedProc;

  proc = Tcl_Command_objProc(cmd);
  importedCmd = GetOriginalCommand(cmd);
  resolvedProc = Tcl_Command_objProc(importedCmd);

  /*
   * Return always state isObject, since the cmd might be an ensemble,
   * where we have to search further
   */
  *isObject = (resolvedProc == NsfObjDispatch);

  if (methodType == NSF_METHODTYPE_ALIAS) {
    if (!(proc == NsfProcAliasMethod || AliasGet(interp, object->cmdName, key, withPer_object))) {
      return 0;
    }
  } else {
    if (proc == NsfProcAliasMethod) {
      if ((methodType & NSF_METHODTYPE_ALIAS) == 0) return 0;
    }
    /* the following cases are disjoint */
    if (CmdIsProc(importedCmd)) {
      /*fprintf(stderr,"%s scripted %d\n", key, methodType & NSF_METHODTYPE_SCRIPTED);*/
      if ((methodType & NSF_METHODTYPE_SCRIPTED) == 0) return 0;
    } else if (resolvedProc == NsfForwardMethod) {
      if ((methodType & NSF_METHODTYPE_FORWARDER) == 0) return 0;
    } else if (resolvedProc == NsfSetterMethod) {
      if ((methodType & NSF_METHODTYPE_SETTER) == 0) return 0;
    } else if (resolvedProc == NsfObjDispatch) {
      if ((methodType & NSF_METHODTYPE_OBJECT) == 0) return 0;
    } else if ((methodType & NSF_METHODTYPE_OTHER) == 0) {
      /* fprintf(stderr,"OTHER %s not wanted %.4x\n", key, methodType);*/
      return 0;
    }
    /* NsfObjscopedMethod ??? */
  }
  return 1;
}

static int
ListMethodKeys(Tcl_Interp *interp, Tcl_HashTable *tablePtr,
	       Tcl_DString *prefix, CONST char *pattern,
               int methodType, int withCallprotection, int withExpand,
               Tcl_HashTable *dups, NsfObject *object, int withPer_object) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr, *duphPtr;
  Tcl_Command cmd;
  char *key;
  int new, isObject, methodTypeMatch;
  int prefixLength = prefix ? Tcl_DStringLength(prefix) : 0;

  if (pattern && NoMetaChars(pattern) && strchr(pattern, ' ') == 0) {
    /*
     * We have a pattern that can be used for direct lookup; no need
     * to iterate
     */
    hPtr = tablePtr ? Tcl_CreateHashEntry(tablePtr, pattern, NULL) : NULL;
    if (hPtr) {
      key = Tcl_GetHashKey(tablePtr, hPtr);
      cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
      methodTypeMatch = MethodTypeMatches(interp, methodType, cmd, object, key,
					  withPer_object, &isObject);

      if (Tcl_Command_flags(cmd) & NSF_CMD_CLASS_ONLY_METHOD && !NsfObjectIsClass(object)) {
	return TCL_OK;
      }
      if (isObject && withExpand) {
	return TCL_OK;
      }

      if (ProtectionMatches(interp, withCallprotection, cmd) && methodTypeMatch) {
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
    hPtr = tablePtr ? Tcl_FirstHashEntry(tablePtr, &hSrch) : NULL;

    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      key = Tcl_GetHashKey(tablePtr, hPtr);
      cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
      if (prefixLength) {Tcl_DStringTrunc(prefix, prefixLength);}
      methodTypeMatch = MethodTypeMatches(interp, methodType, cmd, object, key,
					  withPer_object, &isObject);

      if (isObject && withExpand) {
	Tcl_DString ds, *dsPtr = &ds;
	NsfObject *ensembleObject = NsfGetObjectFromCmdPtr(cmd);
	Tcl_HashTable *cmdTablePtr = ensembleObject && ensembleObject->nsPtr ?
	  Tcl_Namespace_cmdTablePtr(ensembleObject->nsPtr) : NULL;

	if (ensembleObject->flags & NSF_IS_SLOT_CONTAINER) {
	  /* Don't report slot container */
	  continue;
	}

	if (prefix == NULL) {
	  DSTRING_INIT(dsPtr);
	  Tcl_DStringAppend(dsPtr, key, -1);
	  Tcl_DStringAppend(dsPtr, " ", 1);
	  ListMethodKeys(interp, cmdTablePtr, dsPtr, pattern, methodType, withCallprotection,
			 1, dups, object, withPer_object);
	  DSTRING_FREE(dsPtr);
	} else {
	  Tcl_DStringAppend(prefix, key, -1);
	  Tcl_DStringAppend(prefix, " ", 1);
	  ListMethodKeys(interp, cmdTablePtr, prefix, pattern, methodType, withCallprotection,
			 1, dups, object, withPer_object);
	}
	/* don't list ensembles by themselves */
	continue;
      }

      if (Tcl_Command_flags(cmd) & NSF_CMD_CLASS_ONLY_METHOD && !NsfObjectIsClass(object)) continue;
      if (!ProtectionMatches(interp, withCallprotection, cmd) || !methodTypeMatch) continue;

      if (prefixLength) {
	Tcl_DStringAppend(prefix, key, -1);
	key = Tcl_DStringValue(prefix);
      }

      if (pattern && !Tcl_StringMatch(key, pattern)) continue;
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
ListChildren(Tcl_Interp *interp, NsfObject *object, CONST char *pattern, 
	     int classesOnly, NsfClass *type) {
  NsfObject *childObject;

  if (!object->nsPtr) return TCL_OK;

  if (pattern && NoMetaChars(pattern)) {
    Tcl_DString ds, *dsPtr = &ds;
    Tcl_DStringInit(dsPtr);
   
    if (*pattern != ':') {
      /* build a fully qualified name */
      Tcl_DStringAppend(dsPtr, object->nsPtr->fullName, -1);
      Tcl_DStringAppend(dsPtr, "::", 2);    
      Tcl_DStringAppend(dsPtr, pattern, -1);
      pattern = Tcl_DStringValue(dsPtr);
    }

    if ((childObject = GetObjectFromString(interp, pattern)) &&
        (!classesOnly || NsfObjectIsClass(childObject)) &&
	(!type || IsSubType(childObject->cl, type)) &&
        (Tcl_Command_nsPtr(childObject->id) == object->nsPtr)  /* true children */
        ) {
      Tcl_SetObjResult(interp, childObject->cmdName);
    } else {
      Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_EMPTY]);
    }
    Tcl_DStringFree(dsPtr);

  } else {
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);
    Tcl_HashSearch hSrch;
    Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(object->nsPtr);
    Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch);
    char *key;

    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      key = Tcl_GetHashKey(cmdTablePtr, hPtr);
      if (!pattern || Tcl_StringMatch(key, pattern)) {
        Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);

        /*fprintf(stderr, "... check %s child key %s child object %p %p\n",
                ObjectName(object),key,GetObjectFromString(interp, key),
                NsfGetObjectFromCmdPtr(cmd));*/

        if ((childObject = NsfGetObjectFromCmdPtr(cmd)) &&
            (!classesOnly || NsfObjectIsClass(childObject)) &&
	    (!type || IsSubType(childObject->cl, type)) &&
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
ListForward(Tcl_Interp *interp, Tcl_HashTable *tablePtr, CONST char *pattern, int withDefinition) {
  if (withDefinition) {
    Tcl_HashEntry *hPtr = tablePtr && pattern ? Tcl_CreateHashEntry(tablePtr, pattern, NULL) : NULL;
    /* notice: we don't use pattern for wildcard matching here;
       pattern can only contain wildcards when used without
       "-definition" */
    if (hPtr) {
      Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
      ClientData clientData = cmd ? Tcl_Command_objClientData(cmd) : NULL;
      ForwardCmdClientData *tcd = (ForwardCmdClientData *)clientData;
      if (tcd && Tcl_Command_objProc(cmd) == NsfForwardMethod) {
        Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
        AppendForwardDefinition(interp, listObj, tcd);
        Tcl_SetObjResult(interp, listObj);
        return TCL_OK;
      }
    }
    return NsfPrintError(interp, "'%s' is not a forwarder", pattern);
  }
  return ListMethodKeys(interp, tablePtr, NULL, pattern, NSF_METHODTYPE_FORWARDER,
			CallprotectionAllIdx, 0, NULL, NULL, 0);
}

static int
ListDefinedMethods(Tcl_Interp *interp, NsfObject *object, CONST char *pattern,
                   int withPer_object, int methodType, int withCallproctection,
                   int withExpand, int noMixins, int inContext) {
  Tcl_HashTable *cmdTablePtr;

  if (NsfObjectIsClass(object) && !withPer_object) {
    cmdTablePtr = Tcl_Namespace_cmdTablePtr(((NsfClass *)object)->nsPtr);
  } else {
    cmdTablePtr = object->nsPtr ? Tcl_Namespace_cmdTablePtr(object->nsPtr) : NULL;
  }
  ListMethodKeys(interp, cmdTablePtr, NULL, pattern, methodType, withCallproctection, withExpand,
                 NULL, object, withPer_object);
  return TCL_OK;
}

static int
ListSuperclasses(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *pattern, int withClosure) {
  NsfObject *matchObject = NULL;
  Tcl_Obj *patternObj = NULL, *outObjPtr;
  CONST char *patternString = NULL;
  int rc;

  if (pattern &&
      ConvertToObjpattern(interp, pattern, NULL, (ClientData *)&patternObj, &outObjPtr) == TCL_OK) {
    if (GetMatchObject(interp, patternObj, pattern, &matchObject, &patternString) == -1) {
      if (patternObj) {
	DECR_REF_COUNT(patternObj);
      }
      return TCL_OK;
    }
  }

  if (withClosure) {
    NsfClasses *pl = ComputeOrder(cl, cl->order, Super);
    if (pl) pl=pl->nextPtr;
    rc = AppendMatchingElementsFromClasses(interp, pl, patternString, matchObject);
  } else {
    NsfClasses *clSuper = NsfReverseClasses(cl->super);
    rc = AppendMatchingElementsFromClasses(interp, clSuper, patternString, matchObject);
    NsfClassListFree(clSuper);
  }

  if (matchObject) {
    Tcl_SetObjResult(interp, rc ? matchObject->cmdName : NsfGlobalObjs[NSF_EMPTY]);
  }

  if (patternObj) {
    DECR_REF_COUNT(patternObj);
  }
  return TCL_OK;
}


/********************************
 * End result setting commands
 ********************************/

static CONST char *
AliasIndex(Tcl_DString *dsPtr, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object) {
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

static int
AliasAdd(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object,
	 CONST char *cmd) {
  Tcl_DString ds, *dsPtr = &ds;
  Tcl_SetVar2Ex(interp, NsfGlobalStrings[NSF_ALIAS_ARRAY],
                AliasIndex(dsPtr, cmdName, methodName, withPer_object),
                Tcl_NewStringObj(cmd, -1),
                TCL_GLOBAL_ONLY);
  /*fprintf(stderr, "aliasAdd ::nsf::alias(%s) '%s' returned %p\n",
    AliasIndex(dsPtr, cmdName, methodName, withPer_object), cmd, 1);*/
  Tcl_DStringFree(dsPtr);
  return TCL_OK;
}

static int
AliasDelete(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object) {
  Tcl_DString ds, *dsPtr = &ds;
  int result = Tcl_UnsetVar2(interp, NsfGlobalStrings[NSF_ALIAS_ARRAY],
                             AliasIndex(dsPtr, cmdName, methodName, withPer_object),
                             TCL_GLOBAL_ONLY);
  /*fprintf(stderr, "aliasDelete ::nsf::alias(%s) returned %d (%d)\n",
    AliasIndex(dsPtr, cmdName, methodName, withPer_object), result);*/
  Tcl_DStringFree(dsPtr);
  return result;
}

static Tcl_Obj *
AliasGet(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object) {
  Tcl_DString ds, *dsPtr = &ds;
  Tcl_Obj *obj = Tcl_GetVar2Ex(interp, NsfGlobalStrings[NSF_ALIAS_ARRAY],
                               AliasIndex(dsPtr, cmdName, methodName, withPer_object),
                               TCL_GLOBAL_ONLY);
  /*fprintf(stderr, "aliasGet methodName '%s' returns %p\n", methodName, obj);*/
  Tcl_DStringFree(dsPtr);
  return obj;
}



/*
 *----------------------------------------------------------------------
 * AliasDeleteObjectReference --
 *
 *    Delete an alias to an referenced object. Such aliases are
 *    created by registering an alias to an object. This funciton
 *    distinguishes between a sub-object and an alias to an object,
 *    deletes the alias but never the referenced object.
 *
 * Results:
 *    1 when alias is deleted.
 *
 * Side effects:
 *    Deletes cmd sometimes
 *
 *----------------------------------------------------------------------
 */
static int
AliasDeleteObjectReference(Tcl_Interp *interp, Tcl_Command cmd) {
  NsfObject *referencedObject = NsfGetObjectFromCmdPtr(cmd);

  if (referencedObject
      && referencedObject->refCount >= 2
      && cmd != referencedObject->id) {
    /*
     * The cmd is an aliased object, reduce the refcount of the
     * object, delete the cmd.
     */
    /*fprintf(stderr, "remove alias %s to %s\n",
      Tcl_GetCommandName(interp, cmd), ObjectName(referencedObject));*/
    NsfCleanupObject(referencedObject, "AliasDeleteObjectReference");
    Nsf_DeleteCommandFromToken(interp, cmd);
    return 1;
  }
  return 0;
}


/*******************************************
 * Begin generated Next Scripting commands
 *******************************************/
/*
nsfCmd __db_show_stack NsfShowStackCmd {
}
*/
static int
NsfShowStackCmd(Tcl_Interp *interp) {
  NsfShowStack(interp);
  return TCL_OK;
}

/*
nsfCmd __db_run_assertions NsfDebugRunAssertionsCmd {
}
*/
static int
NsfDebugRunAssertionsCmd(Tcl_Interp *interp) {
  Tcl_HashTable table, *tablePtr = &table;
  NsfObjectSystem *osPtr;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  /* collect all instances from all object systems */
  Tcl_InitHashTable(tablePtr, TCL_STRING_KEYS);
  for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = osPtr->nextPtr) {
    GetAllInstances(interp, tablePtr, osPtr->rootClass);
  }

  for (hPtr = Tcl_FirstHashEntry(tablePtr, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(tablePtr, hPtr);
    NsfObject *object = GetObjectFromString(interp, key);

    if (!object) {
      fprintf(stderr,"key %s\n", key);
    }

    assert(object);
    assert(object->refCount>0);
    assert(object->cmdName->refCount>0);
    assert(object->activationCount >= 0);


#if defined(CHECK_ACTIVATION_COUNTS)
    if (object->activationCount > 0) {
      Tcl_CallFrame *framePtr;
      int count = 0;
      NsfClasses *unstackedEntries = RUNTIME_STATE(interp)->cscList;

      /*fprintf(stderr, "DEBUG obj %p %s activationcount %d\n",
	object, ObjectName(object), object->activationCount);*/

      framePtr = (Tcl_CallFrame *)Tcl_Interp_framePtr(interp);
      for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
	int frameFlags = Tcl_CallFrame_isProcCallFrame(framePtr);
	NsfCallStackContent *cscPtr =
	  (frameFlags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) ?
	  ((NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr)) : NULL;
	if (cscPtr && cscPtr->self == object) count ++;
	if (cscPtr && (NsfObject*)cscPtr->cl == object) count ++;
      }
      for (; unstackedEntries; unstackedEntries = unstackedEntries->nextPtr) {
	NsfCallStackContent *cscPtr = (NsfCallStackContent *)unstackedEntries->cl;
	if (cscPtr && cscPtr->self == object) count ++;
	if (cscPtr && (NsfObject*)cscPtr->cl == object) count ++;	  
      }

      if (count != object->activationCount) {
	fprintf(stderr, "DEBUG obj %p %s activationcount %d on stack %d; "
		"might be from non-stacked but active callstack content\n",
		object, ObjectName(object), object->activationCount, count);

	fprintf(stderr, "fixed count %d\n", count);
	/*NsfShowStack(interp);*/
	/*return NsfPrintError(interp, "wrong activation count for object %s", ObjectName(object));*/
      }
    }
#endif

  }
  /*fprintf(stderr, "all assertions passed\n");*/
  Tcl_DeleteHashTable(tablePtr);

  return TCL_OK;
}

/*
nsfCmd alias NsfAliasCmd {
  {-argName "object" -type object}
  {-argName "-per-object"}
  {-argName "methodName"}
  {-argName "-frame" -required 0 -nrargs 1 -type "method|object|default" -default "default"}
  {-argName "cmdName" -required 1 -type tclobj}
}
*/
static int
NsfAliasCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object,
                         CONST char *methodName, int withFrame,
                         Tcl_Obj *cmdName) {
  Tcl_ObjCmdProc *objProc, *newObjProc = NULL;
  Tcl_CmdDeleteProc *deleteProc = NULL;
  AliasCmdClientData *tcd = NULL; /* make compiler happy */
  Tcl_Command cmd, newCmd = NULL;
  Tcl_Namespace *nsPtr;
  int flags, result;
  NsfClass *cl = (withPer_object || ! NsfObjectIsClass(object)) ? NULL : (NsfClass *)object;

  cmd = Tcl_GetCommandFromObj(interp, cmdName);
  if (cmd == NULL) {
    return NsfPrintError(interp, "cannot lookup command '%s'", ObjStr(cmdName));
  }

  cmd = GetOriginalCommand(cmd);
  objProc = Tcl_Command_objProc(cmd);

  /* objProc is either ...

     1. NsfObjDispatch: a command representing an Next Scripting object

     2. TclObjInterpProc: a cmd standing for a Tcl proc (including
        Next Scripting methods), verified through CmdIsProc() -> to be
        wrapped by NsfProcAliasMethod()

     3. NsfForwardMethod: an Next Scripting forwarder

     4. NsfSetterMethod: an Next Scripting setter

     5. arbitrary Tcl commands (e.g. set, ..., ::nsf::relation, ...)

     TODO GN: i think, we should use NsfProcAliasMethod, whenever the clientData
     is not 0. These are the cases, where the clientData will be freed,
     when the original command is deleted.
  */

  if (withFrame == FrameObjectIdx) {
    newObjProc = NsfObjscopedMethod;
  }

  if (objProc == NsfObjDispatch) {
    /*
     * When we register an alias for an object, we have to take care to
     * handle cases, where the aliased object is destroyed and the
     * alias points to nowhere. We realize this via using the object
     * refcount.
     */
    /*fprintf(stderr, "registering an object %p\n", tcd);*/

    NsfObjectRefCountIncr((NsfObject *)Tcl_Command_objClientData(cmd));

    /*newObjProc = NsfProcAliasMethod;*/

  } else if (CmdIsProc(cmd)) {
    /*
     * if we have a tcl proc|nsf-method as alias, then use the
     * wrapper, which will be deleted automatically when the original
     * proc/method is deleted.
     */
    newObjProc = NsfProcAliasMethod;
    if (withFrame && withFrame != FrameDefaultIdx) {
      return NsfPrintError(interp, 
			  "cannot use -frame object|method in alias for scripted command '%s'",
			   ObjStr(cmdName));
    }
  }

  if (newObjProc) {
    /* add a wrapper */
    tcd = NEW(AliasCmdClientData);
    tcd->cmdName    = object->cmdName;
    tcd->interp     = interp; /* just for deleting the associated variable */
    tcd->object     = object;
    tcd->class	    = cl ? (NsfClass *) object : NULL;
    tcd->objProc    = objProc;
    tcd->aliasedCmd = cmd;
    tcd->clientData = Tcl_Command_objClientData(cmd);
    objProc         = newObjProc;
    deleteProc      = AliasCmdDeleteProc;
    if (tcd->cmdName) {INCR_REF_COUNT(tcd->cmdName);}
  } else {
    /* call the command directly (must be a c-implemented command not
     * depending on a volatile client data)
     */
    tcd = Tcl_Command_objClientData(cmd);
  }

  flags = 0;

  if (cl) {
    result = NsfAddClassMethod(interp, (Nsf_Class *)cl, methodName,
                                    objProc, tcd, deleteProc, flags);
    nsPtr = cl->nsPtr;
  } else {
    result = NsfAddObjectMethod(interp, (Nsf_Object*)object, methodName,
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
    AliasAdd(interp, object->cmdName, methodName, cl == NULL, ObjStr(cmdName));

    if (withFrame == FrameMethodIdx) {
      Tcl_Command_flags(newCmd) |= NSF_CMD_NONLEAF_METHOD;
      /*fprintf(stderr, "setting aliased for cmd %p %s flags %.6x, tcd = %p\n",
        newCmd,methodName,Tcl_Command_flags(newCmd), tcd);*/
    }

    result = ListMethodHandle(interp, object, cl == NULL, methodName);
  }

  return result;
}

/*
nsfCmd assertion NsfAssertionCmd {
  {-argName "object" -type object}
  {-argName "assertionsubcmd" -required 1 -type "check|object-invar|class-invar"}
  {-argName "arg" -required 0 -type tclobj}
}

  Make "::nsf::assertion" a cmd rather than a method, otherwise we
  cannot define e.g. a "method check options {...}" to reset the check
  options in case of a failed option, since assertion checking would
  be applied on the sketched method already.
*/

static int
NsfAssertionCmd(Tcl_Interp *interp, NsfObject *object, int subcmd, Tcl_Obj *arg) {
  NsfClass *class;

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
      NsfObjectOpt *opt = NsfRequireObjectOpt(object);
      AssertionSetInvariants(interp, &opt->assertions, arg);
    } else {
      if (object->opt && object->opt->assertions) {
        Tcl_SetObjResult(interp, AssertionList(interp, object->opt->assertions->invariants));
      }
    }
    break;

  case AssertionsubcmdClass_invarIdx:
    class = (NsfClass *)object;
    if (arg) {
      NsfClassOpt *opt = NsfRequireClassOpt(class);
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
nsfCmd configure NsfConfigureCmd {
  {-argName "configureoption" -required 1 -type "debug|filter|softrecreate|objectsystems|keepinitcmd|checkresult"}
  {-argName "value" -required 0 -type tclobj}
}
*/
static int
NsfConfigureCmd(Tcl_Interp *interp, int configureoption, Tcl_Obj *valueObj) {
  int bool;

  if (configureoption == ConfigureoptionObjectsystemsIdx) {
    NsfObjectSystem *osPtr;
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);

    for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = osPtr->nextPtr) {
      Tcl_Obj *osObj = Tcl_NewListObj(0, NULL);
      Tcl_Obj *systemMethods = Tcl_NewListObj(0, NULL);
      int idx;

      Tcl_ListObjAppendElement(interp, osObj, osPtr->rootClass->object.cmdName);
      Tcl_ListObjAppendElement(interp, osObj, osPtr->rootMetaClass->object.cmdName);

      for (idx = 0; Nsf_SytemMethodOpts[idx]; idx++) {
	/*fprintf(stderr, "opt %s %s\n", Nsf_SytemMethodOpts[idx], 
	  osPtr->methods[idx] ? ObjStr(osPtr->methods[idx]) : "NULL");*/
	if (osPtr->methods[idx] == NULL) {
	  continue;
	}
	Tcl_ListObjAppendElement(interp, systemMethods, Tcl_NewStringObj(Nsf_SytemMethodOpts[idx], -1));
	Tcl_ListObjAppendElement(interp, systemMethods, osPtr->methods[idx]);
      }
      Tcl_ListObjAppendElement(interp, osObj, systemMethods);
      Tcl_ListObjAppendElement(interp, list, osObj);
    }
    Tcl_SetObjResult(interp, list);
    return TCL_OK;
  }

  if (configureoption == ConfigureoptionDebugIdx) {
    int level;

    if (valueObj) {
      int result = Tcl_GetIntFromObj(interp, valueObj, &level);
      if (result != TCL_OK) {
	return result;
      }
      RUNTIME_STATE(interp)->debugLevel = level;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp),
                      RUNTIME_STATE(interp)->debugLevel);

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

  case ConfigureoptionCheckresultsIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doCheckResults));
    if (valueObj)
      RUNTIME_STATE(interp)->doCheckResults = bool;
    break;

  case ConfigureoptionCheckargumentsIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doCheckArguments));
    if (valueObj)
      RUNTIME_STATE(interp)->doCheckArguments = bool;
    break;
  }
  return TCL_OK;
}


/*
nsfCmd createobjectsystem NsfCreateObjectSystemCmd {
  {-argName "rootClass" -required 1 -type tclobj}
  {-argName "rootMetaClass" -required 1 -type tclobj}
  {-argName "systemMethods" -required 0 -type tclobj}
}
*/
static int
NsfCreateObjectSystemCmd(Tcl_Interp *interp, Tcl_Obj *Object, Tcl_Obj *Class, Tcl_Obj *systemMethodsObj) {
  NsfClass *theobj = NULL, *thecls = NULL;
  Tcl_Obj *object, *class;
  char *objectName = ObjStr(Object);
  char *className = ObjStr(Class);
  NsfObjectSystem *osPtr = NEW(NsfObjectSystem);

  memset(osPtr, 0, sizeof(NsfObjectSystem));

  object = isAbsolutePath(objectName) ? Object :
    NameInNamespaceObj(interp, objectName, CallingNameSpace(interp));
  class = isAbsolutePath(className) ? Class :
    NameInNamespaceObj(interp, className, CallingNameSpace(interp));

  GetClassFromObj(interp, object, &theobj, NULL);
  GetClassFromObj(interp, class, &thecls, NULL);

  if (theobj || thecls) {
    ObjectSystemFree(interp, osPtr);
    NsfLog(interp, NSF_LOG_WARN, "Base class '%s' exists already; ignoring definition", 
	   theobj ? objectName : className);
    return TCL_OK;
  }

  if (systemMethodsObj) {
    int oc, i, idx, result;
    Tcl_Obj **ov;

    if ((result = Tcl_ListObjGetElements(interp, systemMethodsObj, &oc, &ov)) == TCL_OK) {
      if (oc % 2) {
        ObjectSystemFree(interp, osPtr);
        return NsfPrintError(interp, "System methods must be provided as pairs");
      }
      for (i=0; i<oc; i += 2) {
        result = Tcl_GetIndexFromObj(interp, ov[i], Nsf_SytemMethodOpts, "system method", 0, &idx);
        if (result != TCL_OK) {
          ObjectSystemFree(interp, osPtr);
          return NsfPrintError(interp, "invalid system method '%s'", ObjStr(ov[i]));
        }
        /*fprintf(stderr, "NsfCreateObjectSystemCmd [%d] = %p %s (max %d, given %d)\n",
          idx, ov[i+1], ObjStr(ov[i+1]), XO_unknown_idx, oc);*/
        osPtr->methods[idx] = ov[i+1];
        INCR_REF_COUNT(osPtr->methods[idx]);
      }
    } else {
      ObjectSystemFree(interp, osPtr);
      return NsfPrintError(interp, "Provided system methods are not a proper list");
    }
  }
  /*
     Create a basic object system with the basic root class Object and
     the basic metaclass Class, and store them in the RUNTIME STATE if
     successful
  */
  theobj = PrimitiveCCreate(interp, object, NULL, NULL);
  thecls = PrimitiveCCreate(interp, class, NULL, NULL);
  /* fprintf(stderr, "CreateObjectSystem created base classes \n"); */

#if defined(NSF_PROFILE)
  NsfProfileInit(interp);
#endif

  /* check whether Object and Class creation was successful */
  if (!theobj || !thecls) {
    int i;

    if (thecls) PrimitiveCDestroy((ClientData) thecls);
    if (theobj) PrimitiveCDestroy((ClientData) theobj);

    for (i = 0; i < nr_elements(NsfGlobalStrings); i++) {
      DECR_REF_COUNT(NsfGlobalObjs[i]);
    }
    FREE(Tcl_Obj **, NsfGlobalObjs);
    FREE(NsfRuntimeState, RUNTIME_STATE(interp));
    ObjectSystemFree(interp, osPtr);

    return NsfPrintError(interp, "Creation of object system failed");
  }

  theobj->osPtr = osPtr;
  thecls->osPtr = osPtr;
  osPtr->rootClass = theobj;
  osPtr->rootMetaClass = thecls;

  theobj->object.flags |= NSF_IS_ROOT_CLASS;
  thecls->object.flags |= NSF_IS_ROOT_META_CLASS;

  ObjectSystemAdd(interp, osPtr);

  AddInstance((NsfObject*)theobj, thecls);
  AddInstance((NsfObject*)thecls, thecls);
  AddSuper(thecls, theobj);

  return TCL_OK;
}

/*
nsfCmd deprecated NsfDeprecatedCmd {
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
NsfDeprecatedCmd(Tcl_Interp *interp, CONST char *what, CONST char *oldCmd, CONST char *newCmd) {
  fprintf(stderr, "**\n**\n** The %s <%s> is deprecated.\n", what, oldCmd);
  if (newCmd)
    fprintf(stderr, "** Use <%s> instead.\n", newCmd);
  fprintf(stderr, "**\n");
  return TCL_OK;
}

/*
nsfCmd dispatch NsfDispatchCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-frame" -required 0 -nrargs 1 -type "method|object|default" -default "default"}
  {-argName "command" -required 1 -type tclobj}
  {-argName "args"  -type args}
}
*/
static int
NsfDispatchCmd(Tcl_Interp *interp, NsfObject *object, int withFrame,
                 Tcl_Obj *command, int nobjc, Tcl_Obj *CONST nobjv[]) {
  int result;
  CONST char *methodName = ObjStr(command);

  /*fprintf(stderr, "Dispatch obj=%s, cmd m='%s'\n", ObjectName(object), methodName);*/

  /*
   * If the specified method is a fully qualified cmd name like
   * e.g. ::nsf::cmd::Class::alloc, this method is called on the
   * specified <Class|Object>, no matter whether it was registered on
   * it.
   */

  if (*methodName == ':') {
    Tcl_Command cmd, importedCmd;
    CallFrame frame, *framePtr = &frame;
    int flags = 0;

    /*
     * We have an absolute name. We assume, the name is the name of a
     * Tcl command, that will be dispatched. If "withFrame == instance" is
     * specified, a callstack frame is pushed to make instvars
     * accessible for the command.
     */

    cmd = Tcl_GetCommandFromObj(interp, command);
    /* fprintf(stderr, "colon name %s cmd %p\n", methodName, cmd);*/

    if (cmd && (importedCmd = TclGetOriginalCommand(cmd))) {
      cmd = importedCmd;
    }

    if (cmd == NULL) {
      return NsfPrintError(interp, "cannot lookup command '%s'", methodName);
    }

    if (withFrame && withFrame != FrameDefaultIdx) {
      Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);
      if (proc == TclObjInterpProc || 
	  proc == NsfForwardMethod ||
	  proc == NsfObjscopedMethod ||
	  proc == NsfSetterMethod ||
	  proc == NsfObjDispatch) {
	return NsfPrintError(interp, "cannot use -frame object|method in dispatch for command '%s'",
			     methodName);
      }

      if (withFrame == FrameObjectIdx) {
	Nsf_PushFrameObj(interp, object, framePtr);
	flags = NSF_CSC_IMMEDIATE;
      } else if (withFrame == FrameMethodIdx) {
	flags = NSF_CSC_FORCE_FRAME|NSF_CSC_IMMEDIATE;
      }
    }
    /*
     * Since we know, that we are always called with a full argument
     * vector, we can include the cmd name in the objv by using
     * nobjv-1; this way, we avoid a memcpy()
     */
    result = MethodDispatch((ClientData)object, interp,
			    nobjc+1, nobjv-1, cmd, object,
			    NULL /*NsfClass *cl*/,
			    Tcl_GetCommandName(interp,cmd),
			    NSF_CSC_TYPE_PLAIN, flags);
    if (withFrame == FrameObjectIdx) {
      Nsf_PopFrameObj(interp, framePtr);
    }
  } else {
    /*
     * No colons in command name, use method from the precedence
     * order, with filters etc. -- strictly speaking unneccessary,
     * since we could dispatch the method also without
     * NsfDispatchCmd(), but it can be used to invoke protected
     * methods. 'withFrame == FrameObjectIdx' is here a no-op.
     */

    Tcl_Obj *arg;
    Tcl_Obj *CONST *objv;
    
    if (withFrame && withFrame != FrameDefaultIdx) {
      return NsfPrintError(interp, 
			   "cannot use -frame object|method in dispatch for plain method name '%s'",
			   methodName);
    }

    if (nobjc >= 1) {
      arg = nobjv[0];
      objv = nobjv+1;
    } else {
      arg = NULL;
      objv = NULL;
    }
    result = NsfCallMethodWithArgs((ClientData)object, interp, command, arg,
				     nobjc, objv, NSF_CM_NO_UNKNOWN|NSF_CSC_IMMEDIATE);
  }

  return result;
}

/*
nsfCmd colon NsfColonCmd {
  {-argName "args" -type allargs}
}
*/
static int
NsfColonCmd(Tcl_Interp *interp, int nobjc, Tcl_Obj *CONST nobjv[]) {
  NsfObject *self = GetSelfObj(interp);
  if (!self) {
    return NsfPrintError(interp, "Cannot resolve 'self', "
			 "probably called outside the context of an Next Scripting Object");
  }
  /* fprintf(stderr, "Colon dispatch %s.%s\n", ObjectName(self),ObjStr(nobjv[0]));*/
  return ObjectDispatch(self, interp, nobjc, nobjv, NSF_CM_NO_SHIFT);
}

/*
nsfCmd existsvar NsfExistsVarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "varname" -required 1}
}
*/
static int
NsfExistsVarCmd(Tcl_Interp *interp, NsfObject *object, CONST char *varName) {

  if (CheckVarName(interp, varName) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_SetIntObj(Tcl_GetObjResult(interp), VarExists(interp, object, varName, NULL, 1, 1));

  return TCL_OK;
}


/*
nsfCmd finalize NsfFinalizeObjCmd {
}
*/
/*
 * ::nsf::finalize command
 */
static int
NsfFinalizeObjCmd(Tcl_Interp *interp) {
  int result;

  /*fprintf(stderr, "+++ call tcl-defined exit handler\n");  */

#if defined(NSF_PROFILE)
  NsfProfilePrintData(interp);
#endif
  /*
   * evaluate user-defined exit handler
   */
  result = Tcl_Eval(interp, "::nsf::__exithandler");

  if (result != TCL_OK) {
    fprintf(stderr, "User defined exit handler contains errors!\n"
            "Error in line %d: %s\nExecution interrupted.\n",
            Tcl_GetErrorLine(interp), ObjStr(Tcl_GetObjResult(interp)));
  }

  ObjectSystemsCleanup(interp);

#ifdef DO_CLEANUP
# if defined(CHECK_ACTIVATION_COUNTS)
  assert(RUNTIME_STATE(interp)->cscList == NULL);
# endif
  /*fprintf(stderr, "CLEANUP TOP NS\n");*/
  Tcl_Export(interp, RUNTIME_STATE(interp)->NsfNS, "", 1);
  Tcl_DeleteNamespace(RUNTIME_STATE(interp)->NsfClassesNS);
  Tcl_DeleteNamespace(RUNTIME_STATE(interp)->NsfNS);
#endif

  return TCL_OK;
}

/*
nsfCmd forward NsfForwardCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "method" -required 1 -type tclobj}
  {-argName "-default" -nrargs 1 -type tclobj}
  {-argName "-earlybinding"}
  {-argName "-methodprefix" -nrargs 1 -type tclobj}
  {-argName "-objframe"}
  {-argName "-onerror" -nrargs 1 -type tclobj}
  {-argName "-verbose"}
  {-argName "target" -type tclobj}
  {-argName "args" -type args}
}
*/
static int
NsfForwardCmd(Tcl_Interp *interp,
	      NsfObject *object, int withPer_object,
	      Tcl_Obj *methodObj,
	      Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix,
	      int withObjframe, Tcl_Obj *withOnerror, int withVerbose,
	      Tcl_Obj *target, int nobjc, Tcl_Obj *CONST nobjv[]) {
  ForwardCmdClientData *tcd = NULL;
  int result;

  result = ForwardProcessOptions(interp, methodObj,
                                 withDefault, withEarlybinding, withMethodprefix,
                                 withObjframe, withOnerror, withVerbose,
                                 target, nobjc, nobjv, &tcd);
  if (result == TCL_OK) {
    CONST char *methodName = NSTail(ObjStr(methodObj));
    NsfClass *cl =
      (withPer_object || ! NsfObjectIsClass(object)) ?
      NULL : (NsfClass *)object;

    tcd->object = object;
    if (cl == NULL) {
      result = NsfAddObjectMethod(interp, (Nsf_Object *)object, methodName,
                                    (Tcl_ObjCmdProc*)NsfForwardMethod,
                                    (ClientData)tcd, ForwardCmdDeleteProc, 0);
    } else {
      result = NsfAddClassMethod(interp, (Nsf_Class *)cl, methodName,
                                      (Tcl_ObjCmdProc*)NsfForwardMethod,
                                      (ClientData)tcd, ForwardCmdDeleteProc, 0);
    }
    if (result == TCL_OK) {
      result = ListMethodHandle(interp, object, cl == NULL, methodName);
    }
  }

  if (result != TCL_OK && tcd) {
    ForwardCmdDeleteProc((ClientData)tcd);
  }
  return result;
}

/*
nsfCmd importvar NsfImportvarCmd {
  {-argName "object" -type object}
  {-argName "args" -type args}
}
*/
static int
NsfImportvar(Tcl_Interp *interp, NsfObject *object, const char *cmdName, int objc, Tcl_Obj *CONST objv[]) {
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
        result = NsfPrintError(interp, "invalid variable specification '%s'", ObjStr(objv[i]));
      }
    }
  }
  return result;
}

static int
NsfImportvarCmd(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]) {
  return NsfImportvar(interp, object, "importvar", objc, objv);
}


/*
nsfCmd interp NsfInterpObjCmd {
  {-argName "name"}
  {-argName "args" -type allargs}
}
*/
/* create a slave interp that calls Next Scripting Init */
static int
NsfInterpObjCmd(Tcl_Interp *interp, CONST char *name, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Interp *slave;

  /* create a fresh Tcl interpreter, or pass command to an existing one */
  if (NsfCallCommand(interp, NSF_INTERP, objc, objv) != TCL_OK) {
    return TCL_ERROR;
  }

  if (isCreateString(name)) {
    /*
     * The command was an interp create, so perform an Nsf_Init() on
     * the new interpreter
     */
    slave = Tcl_GetSlave(interp, ObjStr(objv[2]));
    if (!slave) {
      return NsfPrintError(interp, "Creation of slave interpreter failed");
    }
    if (Nsf_Init(slave) == TCL_ERROR) {
      return TCL_ERROR;
    }
#ifdef NSF_MEM_COUNT
    nsfMemCountInterpCounter++;
#endif
  }
  return TCL_OK;
}

/*
nsfCmd invalidateobjectparameter NsfInvalidateObjectParameterCmd {
  {-argName "class" -type class}
}
*/
static int
NsfInvalidateObjectParameterCmd(Tcl_Interp *interp, NsfClass *cl) {
  if (cl->parsedParamPtr) {
    /*fprintf(stderr, "   %s invalidate %p\n", ClassName(cl), cl->parsedParamPtr);*/
    ParsedParamFree(cl->parsedParamPtr);
    cl->parsedParamPtr = NULL;
  }
  return TCL_OK;
}

/*
nsfCmd is NsfIsCmd {
  {-argName "-complain"}
  {-argName "constraint" -required 1 -type tclobj}
  {-argName "value" -required 1 -type tclobj}
}
*/
static int
NsfIsCmd(Tcl_Interp *interp, int withComplain, Tcl_Obj *constraintObj, Tcl_Obj *valueObj) {
  NsfParam *paramPtr = NULL;
  int result;

  result = ParameterCheck(interp, constraintObj, valueObj, "value:", 1, &paramPtr);

  if (paramPtr == NULL) {
    /*
     * We could not convert the arguments. Even with noComplain, we
     * report the invalid converter spec as exception
     */
    return TCL_ERROR;
  }

  if (paramPtr->converter == ConvertViaCmd
      && (withComplain == 0 || result == TCL_OK)) {
    Tcl_ResetResult(interp);
  }

  if (withComplain == 0) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), (result == TCL_OK));
    result = TCL_OK;
  } else if (result == TCL_OK) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
  }

  return result;
}

/*
nsfCmd isobject NsfIsObjectCmd {
  {-argName "object" -required 1 -type tclobj}
}
*/
static int
NsfIsObjectCmd(Tcl_Interp *interp, Tcl_Obj *valueObj) {
  NsfObject *object;
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), GetObjectFromObj(interp, valueObj, &object) == TCL_OK);
  return TCL_OK;
}


/*
nsfCmd method NsfMethodCmd {
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
static int
NsfMethodCmd(Tcl_Interp *interp, NsfObject *object,
	     int withInner_namespace, int withPer_object, int withPublic,
	     Tcl_Obj *nameObj, Tcl_Obj *args, Tcl_Obj *body,
	     Tcl_Obj *withPrecondition, Tcl_Obj *withPostcondition) {
  NsfClass *cl =
    (withPer_object || ! NsfObjectIsClass(object)) ?
    NULL : (NsfClass *)object;

  if (cl == 0) {
    RequireObjNamespace(interp, object);
  }
  return MakeMethod(interp, object, cl, nameObj, args, body,
                    withPrecondition, withPostcondition,
                    withPublic, withInner_namespace);
}

/*
nsfCmd methodproperty NsfMethodPropertyCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "methodproperty" -required 1 -type "class-only|protected|redefine-protected|returns|slotcontainer|slotobj"}
  {-argName "value" -type tclobj}
}
*/
static int
NsfMethodPropertyCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object,
		     Tcl_Obj *methodObj, int methodproperty, Tcl_Obj *valueObj) {
  CONST char *methodName = ObjStr(methodObj), *methodName1 = NULL;
  NsfObject *regObject, *defObject;
  Tcl_DString ds, *dsPtr = &ds;
  Tcl_Command cmd = NULL;
  NsfClass *cl = withPer_object == 0 && NsfObjectIsClass(object) ? (NsfClass *)object : NULL;
  int flag, fromClassNS = cl != NULL;

  Tcl_DStringInit(dsPtr);
  cmd = ResolveMethodName(interp, cl ? cl->nsPtr : object->nsPtr, methodObj,
			  dsPtr, &regObject, &defObject, &methodName1, &fromClassNS);

  /*fprintf(stderr, "methodProperty for method '%s' prop %d value %s => cl %p cmd %p\n",
    methodName, methodproperty, valueObj ? ObjStr(valueObj) : "NULL", cl, cmd);*/

  if (!cmd) {
    Tcl_DStringFree(dsPtr);
    return NsfPrintError(interp, "Cannot lookup object method '%s' for object %s", 
			 methodName, ObjectName(object));
  }
  Tcl_DStringFree(dsPtr);

  switch (methodproperty) {
  case MethodpropertyClass_onlyIdx: /* fall through */
  case MethodpropertyCall_protectedIdx:  /* fall through */
  case MethodpropertyRedefine_protectedIdx:  /* fall through */
    {
      switch (methodproperty) {
      case MethodpropertyClass_onlyIdx: flag = NSF_CMD_CLASS_ONLY_METHOD; break;
      case MethodpropertyCall_protectedIdx:  flag = NSF_CMD_PROTECTED_METHOD; break;
      case MethodpropertyRedefine_protectedIdx: flag = NSF_CMD_REDEFINE_PROTECTED_METHOD; break;
      default: flag = 0;
      }

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
  case MethodpropertySlotcontainerIdx:
    {
      NsfObject *containerObject = NsfGetObjectFromCmdPtr(cmd);
      if (containerObject == NULL) {
	return NsfPrintError(interp, "slot container must be an object");
      }
      flag = NSF_IS_SLOT_CONTAINER;
      if (valueObj) {
	int bool, result;
	result = Tcl_GetBooleanFromObj(interp, valueObj, &bool);
	if (result != TCL_OK) {
	  return result;
	}
	if (bool) {
	  containerObject->flags |= flag;
	} else {
	  containerObject->flags &= ~flag;
	}
      }
      Tcl_SetIntObj(Tcl_GetObjResult(interp), (containerObject->flags & flag) != 0);
      break;
    }
  case MethodpropertySlotobjIdx:
  case MethodpropertyReturnsIdx:
    {
      NsfParamDefs *paramDefs;
      Tcl_Obj **objPtr;

      if (valueObj == NULL && methodproperty == MethodpropertySlotobjIdx) {
	return NsfPrintError(interp, "Option 'slotobj' of method '%s' requires argument", 
			     methodName);
      }

      paramDefs = ParamDefsGet(cmd);
      /*fprintf(stderr, "MethodProperty, ParamDefsGet cmd %p paramDefs %p returns %p\n",
	cmd, paramDefs, paramDefs?paramDefs->returns:NULL);*/

      if (valueObj == NULL) {
	/* a query for "returns" or "slotobj" */
	Tcl_Obj *resultObj;

	if (paramDefs == NULL) {
	  resultObj = NsfGlobalObjs[NSF_EMPTY];
	} else {
	  objPtr = methodproperty == MethodpropertySlotobjIdx ? &paramDefs->slotObj : &paramDefs->returns;
	  resultObj = *objPtr ? *objPtr : NsfGlobalObjs[NSF_EMPTY];
	}
	Tcl_SetObjResult(interp, resultObj);

      } else {
	/* setting "returns" or "slotobj" */
	const char *valueString = ObjStr(valueObj);

	if (paramDefs == NULL) {
	  /* acquire new paramDefs */
	  paramDefs = ParamDefsNew();
	  ParamDefsStore(interp, cmd, paramDefs);
	  /*fprintf(stderr, "new param defs %p for cmd %p %s\n", paramDefs, cmd, methodName);*/
	}
	objPtr = methodproperty == MethodpropertySlotobjIdx ? &paramDefs->slotObj : &paramDefs->returns;

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
nsfCmd my NsfMyCmd {
  {-argName "-local"}
  {-argName "method" -required 1 -type tclobj}
  {-argName "args" -type args}
}
*/
static int
NsfMyCmd(Tcl_Interp *interp, int withLocal, Tcl_Obj *methodObj, int nobjc, Tcl_Obj *CONST nobjv[]) {
  NsfObject *self = GetSelfObj(interp);
  int result;

  if (!self) {
    return NsfPrintError(interp, "Cannot resolve 'self', "
			 "probably called outside the context of an Next Scripting Object");
  }

  if (withLocal) {
    NsfClass *cl = self->cl;
    CONST char *methodName = ObjStr(methodObj);
    Tcl_Command cmd = FindMethod(cl->nsPtr, methodName);
    if (cmd == NULL) {
      return NsfPrintError(interp, "%s: unable to dispatch local method '%s' in class %s", 
			   ObjectName(self), methodName, ClassName(cl));
    }
    result = MethodDispatch((ClientData)self, interp, nobjc+2, nobjv, cmd, self, cl,
			    methodName, 0, 0);
  } else {
#if 0
    /* TODO attempt to make "my" NRE-enabled, failed so far (crash in mixinInheritanceTest) */
    int flags;
    NsfCallStackContent *cscPtr = CallStackGetTopFrame(interp, NULL);
    if (!cscPtr || self != cscPtr->self) {
      flags = NSF_CSC_IMMEDIATE;
    } else {
      flags = NsfImmediateFromCallerFlags(cscPtr->flags);
      fprintf(stderr, "XXX MY %s.%s frame has flags %.6x -> next-flags %.6x\n",
	      ObjectName(self), ObjStr(methodObj), cscPtr->flags, flags);
    }
    result = CallMethod((ClientData)self, interp, methodObj, nobjc+2, nobjv, flags);
#else
    result = CallMethod((ClientData)self, interp, methodObj, nobjc+2, nobjv, NSF_CSC_IMMEDIATE);
#endif
  }
  return result;
}

/*
nsfCmd namespace_copycmds NsfNSCopyCmdsCmd {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
*/
static int
NsfNSCopyCmdsCmd(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs) {
  Tcl_Command cmd;
  Tcl_Obj *newFullCmdName, *oldFullCmdName;
  CONST char *newName, *oldName, *name;
  Tcl_Namespace *fromNsPtr, *toNsPtr;
  Tcl_HashTable *cmdTablePtr;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  NsfObject *object;
  NsfClass *cl;
  int fromClassNS;

  fromNsPtr = ObjFindNamespace(interp, fromNs);
  if (!fromNsPtr)
    return TCL_OK;

  name = ObjStr(fromNs);

  /* check, if we work on an object or class namespace */
  object = GetObjectFromNsName(interp, name, &fromClassNS);

  if (object == NULL) {
    return NsfPrintError(interp, "argument 1 '%s' is not an object", ObjStr(fromNs));
  }

  cl = fromClassNS ? (NsfClass *)object : NULL;

  /*  object = GetObjectFromString(interp, ObjStr(fromNs));*/

  toNsPtr = ObjFindNamespace(interp, toNs);
  if (!toNsPtr)
    return NsfPrintError(interp, "CopyCmds: Destination namespace %s does not exist", 
			 ObjStr(toNs));
  /*
   * copy all procs & commands in the ns
   */
  cmdTablePtr = Tcl_Namespace_cmdTablePtr(fromNsPtr);
  hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch);
  while (hPtr) {
    /*fprintf(stderr, "copy cmdTablePtr = %p, first=%p\n", cmdTablePtr, hPtr);*/
    name = Tcl_GetHashKey(cmdTablePtr, hPtr);

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
    cmd = Tcl_FindCommand(interp, newName, NULL, TCL_GLOBAL_ONLY);
    if (cmd) {
      /*fprintf(stderr, "%s already exists\n", newName);*/
      if (!GetObjectFromString(interp, newName)) {
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
    cmd = Tcl_FindCommand(interp, oldName, NULL, TCL_GLOBAL_ONLY);
    if (cmd == NULL) {
      NsfPrintError(interp, "can't copy \"%s\": commend doesn't exist", oldName);
      DECR_REF_COUNT(newFullCmdName);
      DECR_REF_COUNT(oldFullCmdName);
      return TCL_ERROR;
    }
    /*
     * Do not copy Objects or Classes
     */
    if (!GetObjectFromString(interp, oldName)) {

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
            /* Next Scripting class-methods */
            NsfProcAssertion *procs;
            procs = cl->opt ? AssertionFindProcs(cl->opt->assertions, name) : NULL;

            DSTRING_INIT(dsPtr);
            Tcl_DStringAppendElement(dsPtr, "::nsf::method");
            Tcl_DStringAppendElement(dsPtr, NSCutNsfClasses(toNsPtr->fullName));
            Tcl_DStringAppendElement(dsPtr, name);
            Tcl_DStringAppendElement(dsPtr, ObjStr(arglistObj));
            Tcl_DStringAppendElement(dsPtr, StripBodyPrefix(ObjStr(procPtr->bodyPtr)));
            if (procs) {
              NsfRequireClassOpt(cl);
              AssertionAppendPrePost(interp, dsPtr, procs);
            }
            Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr), 0);
            DSTRING_FREE(dsPtr);

          } else {
            /* Next Scripting object-methods */
            NsfObject *object = GetObjectFromString(interp, fromNsPtr->fullName);
            NsfProcAssertion *procs;

            if (object) {
              procs = object->opt ? AssertionFindProcs(object->opt->assertions, name) : NULL;
            } else {
              DECR_REF_COUNT(newFullCmdName);
              DECR_REF_COUNT(oldFullCmdName);
              DECR_REF_COUNT(arglistObj);
              return NsfPrintError(interp, "No object for assertions");
            }

            DSTRING_INIT(dsPtr);
            Tcl_DStringAppendElement(dsPtr, "::nsf::method");
            Tcl_DStringAppendElement(dsPtr, toNsPtr->fullName);
            Tcl_DStringAppendElement(dsPtr, "-per-object");
            Tcl_DStringAppendElement(dsPtr, name);
            Tcl_DStringAppendElement(dsPtr, ObjStr(arglistObj));
            Tcl_DStringAppendElement(dsPtr, StripBodyPrefix(ObjStr(procPtr->bodyPtr)));
            if (procs) {
              NsfRequireObjectOpt(object);
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
          if (clientData == NULL || clientData == (ClientData)NSF_CMD_NONLEAF_METHOD) {
            /* if client data is not null, we would have to copy
               the client data; we don't know its size...., so rely
               on introspection for copying */
            Tcl_CreateObjCommand(interp, newName, objProc,
                                 Tcl_Command_objClientData(cmd), deleteProc);
          }
        } else {
          clientData = Tcl_Command_clientData(cmd);
          if (clientData == NULL || clientData == (ClientData)NSF_CMD_NONLEAF_METHOD) {
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
nsfCmd namespace_copyvars NsfNSCopyVars {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
*/
static int
NsfNSCopyVarsCmd(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs) {
  Tcl_Namespace *fromNsPtr, *toNsPtr;
  Var *varPtr = NULL;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  TclVarHashTable *varTablePtr;
  NsfObject *object, *destObject;
  CONST char *destFullName;
  Tcl_Obj *destFullNameObj;
  Tcl_CallFrame frame, *framePtr = &frame;
  Tcl_Obj *varNameObj = NULL;

  fromNsPtr = ObjFindNamespace(interp, fromNs);
  /*fprintf(stderr, "copyvars from %s to %s, ns=%p\n", ObjStr(objv[1]), ObjStr(objv[2]), ns);*/

  if (fromNsPtr) {
    toNsPtr = ObjFindNamespace(interp, toNs);
    if (!toNsPtr)
      return NsfPrintError(interp, "CopyVars: Destination namespace %s does not exist", 
			   ObjStr(toNs));

    object = GetObjectFromString(interp, ObjStr(fromNs));
    destFullName = toNsPtr->fullName;
    destFullNameObj = Tcl_NewStringObj(destFullName, -1);
    INCR_REF_COUNT(destFullNameObj);
    varTablePtr = Tcl_Namespace_varTablePtr(fromNsPtr);
    Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, toNsPtr, 0);
  } else {
    NsfObject *newObject;
    if (GetObjectFromObj(interp, fromNs, &object) != TCL_OK) {
      return NsfPrintError(interp, "CopyVars: Origin object/namespace %s does not exist",
			   ObjStr(fromNs));
    }
    if (GetObjectFromObj(interp, toNs, &newObject) != TCL_OK) {
      return NsfPrintError(interp, "CopyVars: Destination object/namespace %s does not exist",
			   ObjStr(toNs));
    }
    varTablePtr = object->varTablePtr;
    destFullNameObj = newObject->cmdName;
    destFullName = ObjStr(destFullNameObj);
  }

  destObject = GetObjectFromString(interp, destFullName);

  /* copy all vars in the ns */
  hPtr = varTablePtr ? Tcl_FirstHashEntry(VarHashTablePtr(varTablePtr), &hSrch) : NULL;
  while (hPtr) {

    GetVarAndNameFromHash(hPtr, &varPtr, &varNameObj);
    INCR_REF_COUNT(varNameObj);

    if (!TclIsVarUndefined(varPtr) && !TclIsVarLink(varPtr)) {
      if (TclIsVarScalar(varPtr)) {
        /* 
	 * Copy scalar variables from the namespace, which might be
	 * either object or namespace variables.
	 */

        if (object) {
          /* fprintf(stderr, "copy in obj %s var %s val '%s'\n", ObjectName(object), ObjStr(varNameObj),
	     ObjStr(valueOfVar(Tcl_Obj, varPtr, objPtr)));*/

	  Nsf_ObjSetVar2((Nsf_Object*)destObject, interp, varNameObj, NULL, 
			 valueOfVar(Tcl_Obj, varPtr, objPtr), 0);
        } else {
          Tcl_ObjSetVar2(interp, varNameObj, NULL,
                         valueOfVar(Tcl_Obj, varPtr, objPtr),
                         TCL_NAMESPACE_ONLY);
        }
      } else {
        if (TclIsVarArray(varPtr)) {
          /* HERE!! PRE85 Why not [array get/set] based? Let the core iterate */
          TclVarHashTable *aTable = valueOfVar(TclVarHashTable, varPtr, tablePtr);
          Tcl_HashSearch ahSrch;
          Tcl_HashEntry *ahPtr = aTable ? Tcl_FirstHashEntry(VarHashTablePtr(aTable), &ahSrch) :0;
          for (; ahPtr; ahPtr = Tcl_NextHashEntry(&ahSrch)) {
            Tcl_Obj *eltNameObj;
            Var *eltVar;

            GetVarAndNameFromHash(ahPtr, &eltVar, &eltNameObj);
            INCR_REF_COUNT(eltNameObj);

            if (TclIsVarScalar(eltVar)) {
              if (object) {
                Nsf_ObjSetVar2((Nsf_Object*)destObject, interp, varNameObj, eltNameObj,
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
nsfCmd __qualify NsfQualifyObjCmd {
  {-argName "name" -required 1 -type tclobj}
}
*/
static int
NsfQualifyObjCmd(Tcl_Interp *interp, Tcl_Obj *nameObj) {
  CONST char *nameString = ObjStr(nameObj);

  if (isAbsolutePath(nameString)) {
    Tcl_SetObjResult(interp, nameObj);
  } else {
    Tcl_SetObjResult(interp, NameInNamespaceObj(interp, nameString, CallingNameSpace(interp)));
  }
  return TCL_OK;
}

/*
nsfCmd relation NsfRelationCmd {
  {-argName "object" -type object}
  {-argName "relationtype" -required 1 -type "object-mixin|class-mixin|object-filter|class-filter|class|superclass|rootclass"}
  {-argName "value" -required 0 -type tclobj}
}
*/
static int
NsfRelationCmd(Tcl_Interp *interp, NsfObject *object,
	       int relationtype, Tcl_Obj *valueObj) {
  int oc; Tcl_Obj **ov;
  NsfObject *nObject = NULL;
  NsfClass *cl = NULL;
  NsfObjectOpt *objopt = NULL;
  NsfClassOpt *clopt = NULL, *nclopt = NULL;
  int i;

  /*fprintf(stderr, "NsfRelationCmd %s rel=%d val='%s'\n",
    ObjectName(object), relationtype, valueObj ? ObjStr(valueObj) : "NULL");*/

  if (relationtype == RelationtypeClass_mixinIdx ||
      relationtype == RelationtypeClass_filterIdx) {
    if (NsfObjectIsClass(object)) {
      cl = (NsfClass *)object;
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
    objopt = NsfRequireObjectOpt(object);
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
    clopt = NsfRequireClassOpt(cl);
    break;

  case RelationtypeSuperclassIdx:
    if (!NsfObjectIsClass(object))
      return NsfObjErrType(interp, "superclass", object->cmdName, "class", NULL);
    cl = (NsfClass *)object;
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
    if (!cl) return NsfObjErrType(interp, "class", object->cmdName, "a class", NULL);
    return ChangeClass(interp, object, cl);

  case RelationtypeRootclassIdx:
    {
    NsfClass *metaClass;

    if (!NsfObjectIsClass(object))
      return NsfObjErrType(interp, "rootclass", object->cmdName, "class", NULL);
    cl = (NsfClass *)object;

    if (valueObj == NULL) {
      return NsfPrintError(interp, "metaclass must be specified as third argument");
    }
    GetClassFromObj(interp, valueObj, &metaClass, NULL);
    if (!metaClass) return NsfObjErrType(interp, "rootclass", valueObj, "class", NULL);

    cl->object.flags |= NSF_IS_ROOT_CLASS;
    metaClass->object.flags |= NSF_IS_ROOT_META_CLASS;

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
      NsfCmdList *newMixinCmdList = NULL;

      for (i = 0; i < oc; i++) {
        if (MixinAdd(interp, &newMixinCmdList, ov[i], object->cl->object.cl) != TCL_OK) {
          CmdListRemoveList(&newMixinCmdList, GuardDel);
          return TCL_ERROR;
        }
      }

      if (objopt->mixins) {
        NsfCmdList *cmdlist, *del;
        for (cmdlist = objopt->mixins; cmdlist; cmdlist = cmdlist->nextPtr) {
          cl = NsfGetClassFromCmdPtr(cmdlist->cmdPtr);
          clopt = cl ? cl->opt : NULL;
          if (clopt) {
            del = CmdListFindCmdInList(object->id, clopt->isObjectMixinOf);
            if (del) {
              /* fprintf(stderr, "Removing object %s from isObjectMixinOf of class %s\n",
                 ObjectName(object), ObjStr(NsfGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
              del = CmdListRemoveFromList(&clopt->isObjectMixinOf, del);
              CmdListDeleteCmdListEntry(del, GuardDel);
            }
          }
        }
        CmdListRemoveList(&objopt->mixins, GuardDel);
      }

      object->flags &= ~NSF_MIXIN_ORDER_VALID;
      /*
       * since mixin procs may be used as filters -> we have to invalidate
       */
      object->flags &= ~NSF_FILTER_ORDER_VALID;

      /*
       * now add the specified mixins
       */
      objopt->mixins = newMixinCmdList;
      for (i = 0; i < oc; i++) {
        Tcl_Obj *ocl = NULL;

        /* fprintf(stderr, "Added to mixins of %s: %s\n", ObjectName(object), ObjStr(ov[i])); */
        Tcl_ListObjIndex(interp, ov[i], 0, &ocl);
        GetObjectFromObj(interp, ocl, &nObject);
        if (nObject) {
          /* fprintf(stderr, "Registering object %s to isObjectMixinOf of class %s\n",
             ObjectName(object), ObjectName(nObject)); */
          nclopt = NsfRequireClassOpt((NsfClass*)nObject);
          CmdListAdd(&nclopt->isObjectMixinOf, object->id, NULL, /*noDuplicates*/ 1);
        } /* else fprintf(stderr, "Problem registering %s as a mixinof of %s\n",
             ObjStr(ov[i]), ClassName(cl)); */
      }

      MixinComputeDefined(interp, object);
      FilterComputeDefined(interp, object);
      break;
    }

  case RelationtypeObject_filterIdx:

    if (objopt->filters) CmdListRemoveList(&objopt->filters, GuardDel);

    object->flags &= ~NSF_FILTER_ORDER_VALID;
    for (i = 0; i < oc; i ++) {
      if (FilterAdd(interp, &objopt->filters, ov[i], object, 0) != TCL_OK)
        return TCL_ERROR;
    }
    /*FilterComputeDefined(interp, object);*/
    break;

  case RelationtypeClass_mixinIdx:
    {
      NsfCmdList *newMixinCmdList = NULL;

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
           ClassName(cl), ObjStr(ov[i])); */

        Tcl_ListObjIndex(interp, ov[i], 0, &ocl);
        GetObjectFromObj(interp, ocl, &nObject);
        if (nObject) {
          /* fprintf(stderr, "Registering class %s to isClassMixinOf of class %s\n",
             ClassName(cl), ObjectName(nObject)); */
          nclopt = NsfRequireClassOpt((NsfClass*) nObject);
          CmdListAdd(&nclopt->isClassMixinOf, cl->object.id, NULL, /*noDuplicates*/ 1);
        } /* else fprintf(stderr, "Problem registering %s as a class-mixin of %s\n",
             ObjStr(ov[i]), ClassName(cl)); */
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
nsfCmd current NsfCurrentCmd {
  {-argName "currentoption" -required 0 -type "proc|method|methodpath|object|class|activelevel|args|activemixin|calledproc|calledmethod|calledclass|callingproc|callingmethod|callingclass|callinglevel|callingobject|filterreg|isnextcall|next"}
}
*/
static int
NsfCurrentCmd(Tcl_Interp *interp, int selfoption) {
  NsfObject *object =  GetSelfObj(interp);
  NsfCallStackContent *cscPtr;
  Tcl_CallFrame *framePtr;
  int result = TCL_OK;

  /*fprintf(stderr, "getSelfObj returns %p\n", object); NsfShowStack(interp);*/

  if (selfoption == 0 || selfoption == CurrentoptionObjectIdx) {
    if (object) {
      Tcl_SetObjResult(interp, object->cmdName);
      return TCL_OK;
    } else {
      return NsfPrintError(interp,  "No current object");
    }
  }

  if (!object && selfoption != CurrentoptionCallinglevelIdx) {
    return NsfPrintError(interp,  "No current object");
  }

  switch (selfoption) {
  case CurrentoptionMethodIdx: /* fall through */
  case CurrentoptionProcIdx:
    cscPtr = CallStackGetTopFrame(interp, NULL);
    if (cscPtr) {
      CONST char *procName = Tcl_GetCommandName(interp, cscPtr->cmdPtr);
      Tcl_SetResult(interp, (char *)procName, TCL_VOLATILE);
    } else {
      return NsfPrintError(interp,  "Can't find proc");
    }
    break;

  case CurrentoptionMethodpathIdx:
    cscPtr = CallStackGetTopFrame(interp, &framePtr);
    Tcl_SetObjResult(interp, 
		     CallStackMethodPath(interp, framePtr, Tcl_NewListObj(0, NULL)));
    break;

  case CurrentoptionClassIdx: /* class subcommand */
    cscPtr = CallStackGetTopFrame(interp, NULL);
    Tcl_SetObjResult(interp, cscPtr->cl ? cscPtr->cl->object.cmdName : NsfGlobalObjs[NSF_EMPTY]);
    break;

  case CurrentoptionActivelevelIdx:
    Tcl_SetObjResult(interp, ComputeLevelObj(interp, ACTIVE_LEVEL));
    break;

  case CurrentoptionArgsIdx: {
    int nobjc;
    Tcl_Obj **nobjv;

    cscPtr = CallStackGetTopFrame(interp, &framePtr);
    if (cscPtr->objv) {
      nobjc = cscPtr->objc;
      nobjv = (Tcl_Obj **)cscPtr->objv;
    } else {
      nobjc = Tcl_CallFrame_objc(framePtr);
      nobjv = (Tcl_Obj **)Tcl_CallFrame_objv(framePtr);
    }
    Tcl_SetObjResult(interp, Tcl_NewListObj(nobjc-1, nobjv+1));
    break;
  }

  case CurrentoptionActivemixinIdx: {
    NsfObject *object = NULL;
    if (RUNTIME_STATE(interp)->cmdPtr) {
      object = NsfGetObjectFromCmdPtr(RUNTIME_STATE(interp)->cmdPtr);
    }
    Tcl_SetObjResult(interp, object ? object->cmdName : NsfGlobalObjs[NSF_EMPTY]);
    break;
  }

  case CurrentoptionCalledprocIdx:
  case CurrentoptionCalledmethodIdx:
    cscPtr = CallStackFindActiveFilter(interp);
    if (cscPtr) {
      Tcl_SetObjResult(interp, 
		       Tcl_NewStringObj(MethodName(cscPtr->filterStackEntry->calledProc), -1));
    } else {
      result = NsfPrintError(interp, "called from outside of a filter");
    }
    break;

  case CurrentoptionCalledclassIdx:
    Tcl_SetResult(interp, ClassName(FindCalledClass(interp, object)), TCL_VOLATILE);
    break;

  case CurrentoptionCallingmethodIdx:
  case CurrentoptionCallingprocIdx:
    cscPtr = NsfCallStackFindLastInvocation(interp, 1, NULL);
    Tcl_SetResult(interp, cscPtr ? (char *)Tcl_GetCommandName(interp, cscPtr->cmdPtr) : "",
		  TCL_VOLATILE);
    break;

  case CurrentoptionCallingclassIdx:
    cscPtr = NsfCallStackFindLastInvocation(interp, 1, NULL);
    Tcl_SetObjResult(interp, cscPtr && cscPtr->cl ? cscPtr->cl->object.cmdName :
		     NsfGlobalObjs[NSF_EMPTY]);
    break;

  case CurrentoptionCallinglevelIdx:
    if (!object) {
      Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    } else {
      Tcl_SetObjResult(interp, ComputeLevelObj(interp, CALLING_LEVEL));
    }
    break;

  case CurrentoptionCallingobjectIdx:
    cscPtr = NsfCallStackFindLastInvocation(interp, 1, NULL);
    Tcl_SetObjResult(interp, cscPtr ? cscPtr->self->cmdName : NsfGlobalObjs[NSF_EMPTY]);
    break;

  case CurrentoptionFilterregIdx:
    cscPtr = CallStackFindActiveFilter(interp);
    if (cscPtr) {
      Tcl_SetObjResult(interp, FilterFindReg(interp, object, cscPtr->cmdPtr));
    } else {
      result = NsfPrintError(interp, "called from outside of a filter");
    }
    break;

  case CurrentoptionIsnextcallIdx: {
    cscPtr = CallStackGetTopFrame(interp, &framePtr);
    framePtr = CallStackNextFrameOfType(Tcl_CallFrame_callerPtr(framePtr), 
					FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD);
    cscPtr = framePtr ? Tcl_CallFrame_clientData(framePtr) : NULL;

    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (cscPtr && (cscPtr->flags & NSF_CSC_CALL_IS_NEXT)));
    break;
  }

  case CurrentoptionNextIdx:
    result = FindSelfNext(interp);
    break;
  }

  return result;
}

/*
nsfCmd self NsfSelfCmd {
}
*/
static int
NsfSelfCmd(Tcl_Interp *interp) {
  NsfObject *object = GetSelfObj(interp);

  if (object) {
    Tcl_SetObjResult(interp, object->cmdName);
    return TCL_OK;
  } else {
    return NsfPrintError(interp,  "No current object");
  }
}

/*
nsfCmd setvar NsfSetVarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "varname" -required 1 -type tclobj}
  {-argName "value" -required 0 -type tclobj}
}
*/
static int
NsfSetVarCmd(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *varname, Tcl_Obj *valueObj) {

  if (CheckVarName(interp, ObjStr(varname)) != TCL_OK) {
    return TCL_ERROR;
  }

  return SetInstVar(interp, object, varname, valueObj);
}

/*
nsfCmd setter NsfSetterCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "parameter" -type tclobj}
  }
*/
static int
NsfSetterCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, Tcl_Obj *parameter) {
  NsfClass *cl = (withPer_object || ! NsfObjectIsClass(object)) ? NULL : (NsfClass *)object;
  CONST char *methodName = ObjStr(parameter);
  SetterCmdClientData *setterClientData;
  size_t j, length;
  int result;

  if (*methodName == '-' || *methodName == ':') {
    return NsfPrintError(interp, "invalid setter name \"%s\" (must not start with a dash or colon)", 
			 methodName);
  }

  setterClientData = NEW(SetterCmdClientData);
  setterClientData->paramsPtr = NULL;
  length = strlen(methodName);

  for (j=0; j<length; j++) {
    if (methodName[j] == ':' || methodName[j] == ' ') break;
  }

  if (j < length) {
    /* looks as if we have a parameter specification */
    int result, possibleUnknowns = 0, plainParams = 0, nrNonposArgs = 0;

    setterClientData->paramsPtr = ParamsNew(1);
    result = ParamParse(interp, NsfGlobalObjs[NSF_SETTER], parameter,
                        NSF_DISALLOWED_ARG_SETTER|NSF_ARG_HAS_DEFAULT,
                        setterClientData->paramsPtr, &possibleUnknowns, 
			&plainParams, &nrNonposArgs);

    if (result != TCL_OK) {
      SetterCmdDeleteProc((ClientData)setterClientData);
      return result;
    }
    methodName = setterClientData->paramsPtr->name;
  } else {
    setterClientData->paramsPtr = NULL;
  }

  if (cl) {
    result = NsfAddClassMethod(interp, (Nsf_Class *)cl, methodName,
                                 (Tcl_ObjCmdProc*)NsfSetterMethod,
                                 (ClientData)setterClientData, SetterCmdDeleteProc, 0);
  } else {
    result = NsfAddObjectMethod(interp, (Nsf_Object *)object, methodName,
                                  (Tcl_ObjCmdProc*)NsfSetterMethod,
                                  (ClientData)setterClientData, SetterCmdDeleteProc, 0);
  }
  if (result == TCL_OK) {
    result = ListMethodHandle(interp, object, cl == NULL, methodName);
  } else {
    SetterCmdDeleteProc((ClientData)setterClientData);
  }
  return result;
}

typedef struct NsfParamWrapper {
  NsfParam *paramPtr;
  int refCount;
  int canFree;
} NsfParamWrapper;

static Tcl_DupInternalRepProc	ParamDupInteralRep;
static Tcl_FreeInternalRepProc	ParamFreeInternalRep;
static Tcl_UpdateStringProc	ParamUpdateString;

static void
ParamUpdateString(Tcl_Obj *objPtr) {
    Tcl_Panic("%s of type %s should not be called", "updateStringProc",
	    objPtr->typePtr->name);
}

static void
ParamDupInteralRep(Tcl_Obj *srcPtr, Tcl_Obj *dupPtr) {
  Tcl_Panic("%s of type %s should not be called", "dupStringProc",
	    srcPtr->typePtr->name);
}

static Tcl_ObjType paramObjType = {
    "nsfParam",			/* name */
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
  NsfParamWrapper *paramWrapperPtr = (NsfParamWrapper *)objPtr->internalRep.twoPtrValue.ptr1;

  if (paramWrapperPtr != NULL) {
    /* fprintf(stderr, "ParamFreeInternalRep freeing wrapper %p paramPtr %p refCount %dcanFree %d\n",
            paramWrapperPtr, paramWrapperPtr->paramPtr, paramWrapperPtr->refCount,
            paramWrapperPtr->canFree);*/

    if (paramWrapperPtr->canFree) {
      ParamsFree(paramWrapperPtr->paramPtr);
      FREE(NsfParamWrapper, paramWrapperPtr);
    } else {
      paramWrapperPtr->refCount--;
    }
  }
}

/*
 *----------------------------------------------------------------------
 * ParamSetFromAny2 --
 *
 *    Convert the second argment argument (e.g. "x:integer") into the
 *    internal representation of a Tcl_Obj of the type parameter. The
 *    conversion is performed by the usual ParamParse() function, used
 *    e.g. for the parameter passing for arguments.
 *
 * Results:
 *    Result code.
 *
 * Side effects:
 *    Converted internal rep of Tcl_Obj
 *
 *----------------------------------------------------------------------
 */

static int
ParamSetFromAny2(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    const char *varNamePrefix,	/* shows up as varname in error message */
    register Tcl_Obj *objPtr)	/* The object to convert. */
{
  NsfParamWrapper *paramWrapperPtr = NEW(NsfParamWrapper);
  Tcl_Obj *fullParamObj = Tcl_NewStringObj(varNamePrefix, -1);
  int result, possibleUnknowns = 0, plainParams = 0, nrNonposArgs = 0;

  paramWrapperPtr->paramPtr = ParamsNew(1);
  paramWrapperPtr->refCount = 1;
  paramWrapperPtr->canFree = 0;
  /*fprintf(stderr, "allocating  %p\n",paramWrapperPtr->paramPtr);*/

  Tcl_AppendLimitedToObj(fullParamObj, ObjStr(objPtr), -1, INT_MAX, NULL);
  INCR_REF_COUNT(fullParamObj);
  result = ParamParse(interp, NsfGlobalObjs[NSF_VALUECHECK], fullParamObj,
                      NSF_DISALLOWED_ARG_VALUECHECK /* disallowed options */,
                      paramWrapperPtr->paramPtr, &possibleUnknowns, 
		      &plainParams, &nrNonposArgs);
  /* Here, we want to treat currently unknown user level converters as
     error.
  */
  if (paramWrapperPtr->paramPtr->flags & NSF_ARG_CURRENTLY_UNKNOWN) {
    ParamsFree(paramWrapperPtr->paramPtr);
    FREE(NsfParamWrapper, paramWrapperPtr);
    result = TCL_ERROR;
  } else if (result == TCL_OK) {
    /*fprintf(stderr, "ParamSetFromAny2 sets unnamed %p\n",paramWrapperPtr->paramPtr);*/

    paramWrapperPtr->paramPtr->flags |= NSF_ARG_UNNAMED;
    if (*(paramWrapperPtr->paramPtr->name) == 'r') {
      paramWrapperPtr->paramPtr->flags |= NSF_ARG_IS_RETURNVALUE;
      /*fprintf(stderr, "ParamSetFromAny2 sets returnvalue %p\n",paramWrapperPtr->paramPtr);*/
    }
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

/*
 *----------------------------------------------------------------------
 * GetObjectParameterDefinition --
 *
 *    Obtain the parameter definitions for an object by calling the
 *    scripted method "objectparameter" if the value is not cached
 *    already.
 *
 * Results:
 *    Tcl return code, parsed structure in last argument
 *
 * Side effects:
 *    Updates potentially cl->parsedParamPtr
 *
 *----------------------------------------------------------------------
 */

static int
GetObjectParameterDefinition(Tcl_Interp *interp, Tcl_Obj *procNameObj, NsfObject *object,
                             NsfParsedParam *parsedParamPtr) {
  int result;
  Tcl_Obj *rawConfArgs;
  NsfParsedParam *clParsedParamPtr = object->cl->parsedParamPtr;

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
  if (clParsedParamPtr) {
    parsedParamPtr->paramDefs = clParsedParamPtr->paramDefs;
    parsedParamPtr->possibleUnknowns = clParsedParamPtr->possibleUnknowns;
    result = TCL_OK;
  } else {
    /*
     * There is no parameter definition available, get a new one in
     * the the string representation.
     */
    Tcl_Obj *methodObj = NsfMethodObj(interp, object, NSF_o_objectparameter_idx);

    if (methodObj) {
      /* fprintf(stderr, "=== calling %s objectparameter\n", ObjectName(object));*/
      result = CallMethod((ClientData) object, interp, methodObj,
			  2, 0, NSF_CM_NO_PROTECT|NSF_CSC_IMMEDIATE);

      if (result == TCL_OK) {
	rawConfArgs = Tcl_GetObjResult(interp);
	/*fprintf(stderr, ".... rawConfArgs for %s => '%s'\n",
	  ObjectName(object), ObjStr(rawConfArgs));*/
	INCR_REF_COUNT(rawConfArgs);
	
	/* 
	 * Parse the string representation to obtain the internal
	 * representation.
	 */
	result = ParamDefsParse(interp, procNameObj, rawConfArgs,
				NSF_DISALLOWED_ARG_OBJECT_PARAMETER, parsedParamPtr);
	if (result == TCL_OK) {
	  NsfParsedParam *ppDefPtr = NEW(NsfParsedParam);
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
ParameterCheck(Tcl_Interp *interp, Tcl_Obj *objPtr, Tcl_Obj *valueObj,
	       const char *varNamePrefix, int doCheck, NsfParam **paramPtrPtr) {
  NsfParamWrapper *paramWrapperPtr;
  Tcl_Obj *outObjPtr = NULL;
  NsfParam *paramPtr;
  ClientData checkedData;
  int result, flags = 0;

  /*fprintf(stderr, "ParamSetFromAny %s value %p %s\n",
    ObjStr(objPtr), valueObj, ObjStr(valueObj));*/

  if (objPtr->typePtr == &paramObjType) {
    paramWrapperPtr = (NsfParamWrapper *) objPtr->internalRep.twoPtrValue.ptr1;
  } else {
    result = ParamSetFromAny2(interp, varNamePrefix, objPtr);
    if (result == TCL_OK) {
      paramWrapperPtr = (NsfParamWrapper *) objPtr->internalRep.twoPtrValue.ptr1;
    } else {
      return NsfPrintError(interp, "invalid value constraints \"%s\"", ObjStr(objPtr));
    }
  }
  paramPtr = paramWrapperPtr->paramPtr;
  if (paramPtrPtr) *paramPtrPtr = paramPtr;

  result = ArgumentCheck(interp, valueObj, paramPtr, doCheck, &flags, &checkedData, &outObjPtr);
  /*fprintf(stderr, "ParamSetFromAny paramPtr %p final refcount of wrapper %d can free %d\n",
    paramPtr, paramWrapperPtr->refCount,  paramWrapperPtr->canFree);*/

  if (paramWrapperPtr->refCount == 0) {
    /* fprintf(stderr, "ParamSetFromAny paramPtr %p manual free\n",paramPtr);*/
    ParamsFree(paramWrapperPtr->paramPtr);
    FREE(NsfParamWrapper, paramWrapperPtr);
  } else {
    paramWrapperPtr->canFree = 1;
  }

  if (flags & NSF_PC_MUST_DECR) {
    DECR_REF_COUNT(outObjPtr);
  }

  return result;
}

/*****************************************
 * End generated Next Scripting  commands
 *****************************************/

/***************************
 * Begin Object Methods
 ***************************/
/*
objectMethod autoname NsfOAutonameMethod {
  {-argName "-instance"}
  {-argName "-reset"}
  {-argName "name" -required 1 -type tclobj}
}
*/
static int
NsfOAutonameMethod(Tcl_Interp *interp, NsfObject *object, int withInstance, int withReset,
                                Tcl_Obj *nameObj) {
  Tcl_Obj *autoname = AutonameIncr(interp, nameObj, object, withInstance, withReset);
  if (autoname) {
    Tcl_SetObjResult(interp, autoname);
    DECR_REF_COUNT(autoname);

    return TCL_OK;
  }
  return NsfPrintError(interp, "Autoname failed. Probably format string (with %%) was not well-formed");
}

/*
objectMethod cleanup NsfOCleanupMethod {
}
*/
static int
NsfOCleanupMethod(Tcl_Interp *interp, NsfObject *object) {
  NsfClass  *cl  = NsfObjectToClass(object);
  int softrecreate;
  Tcl_Obj *savedNameObj;

#if defined(OBJDELETION_TRACE)
  fprintf(stderr, "+++ NsfOCleanupMethod\n");
#endif
  PRINTOBJ("NsfOCleanupMethod", object);

  savedNameObj = object->cmdName;
  INCR_REF_COUNT(savedNameObj);

  /* save and pass around softrecreate*/
  softrecreate = object->flags & NSF_RECREATE && RUNTIME_STATE(interp)->doSoftrecreate;

  CleanupDestroyObject(interp, object, softrecreate);
  CleanupInitObject(interp, object, object->cl, object->nsPtr, softrecreate);

  if (cl) {
    CleanupDestroyClass(interp, cl, softrecreate, 1);
    CleanupInitClass(interp, cl, cl->nsPtr, softrecreate, 1);
  }

  DECR_REF_COUNT(savedNameObj);
  return TCL_OK;
}

/*
objectMethod configure NsfOConfigureMethod {
  {-argName "args" -type allargs}
}
*/

static int
NsfOConfigureMethod(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]) {
  int result, i, remainingArgsc;
  NsfParsedParam parsedParam;
  NsfParam *paramPtr;
  NsfParamDefs *paramDefs;
  Tcl_Obj *newValue;
  ParseContext pc;
  CallFrame frame, *framePtr = &frame;

#if 0
  fprintf(stderr, "NsfOConfigureMethod %s %d ",ObjectName(object), objc);

  for(i=0; i<objc; i++) {
    /*fprintf(stderr, "  ov[%d]=%p, objc=%d\n", j, ov[j], objc);*/
    fprintf(stderr, " o[%d]=%s,", i, ObjStr(objv[i]));
  }
  fprintf(stderr, "\n");
#endif

  /* Get the object parameter definition */
  result = GetObjectParameterDefinition(interp, objv[0], object, &parsedParam);
  if (result != TCL_OK || !parsedParam.paramDefs) {
    /*fprintf(stderr, "... nothing to do for method %s\n", ObjStr(objv[0]));*/
    goto configure_exit;
  }

  /* Push frame to allow for [self] and make instvars of obj accessible as locals */
  Nsf_PushFrameObj(interp, object, framePtr);

  /* Process the actual arguments based on the parameter definitions */
  paramDefs = parsedParam.paramDefs;
  result = ProcessMethodArguments(&pc, interp, object, 0, paramDefs, "configure", objc, objv);

  if (result != TCL_OK) {
    Nsf_PopFrameObj(interp, framePtr);
    ParseContextRelease(&pc);
    goto configure_exit;
  }

  /*
   * At this point, the arguments are valid (according to the
   * parameter definitions) and the defaults are set. Now we have to
   * apply the arguments (mostly setting instance variables).
   */
#if defined(CONFIGURE_ARGS_TRACE)
  fprintf(stderr, "*** POPULATE OBJ '%s': nr of parsed args %d\n", ObjectName(object), pc.objc);
#endif

  for (i=1, paramPtr = paramDefs->paramsPtr; paramPtr->name; paramPtr++, i++) {

    /* 
     * Set the new value always when the object is not yet initialized
     * and the new value was specified (was not taken from the default
     * value definition).  The second part of the test is needed to
     * avoid overwriting with default values when e.g. "o configure"
     * is called lated without arguments.
     */
    if ((object->flags & NSF_INIT_CALLED) && (pc.flags[i-1] & NSF_PC_IS_DEFAULT)) {
      continue;
    }

    newValue = pc.full_objv[i];
    /*fprintf(stderr, "new Value of %s = [%d] %p '%s', type %s\n",
            ObjStr(paramPtr->nameObj), i,
            newValue, newValue ? ObjStr(newValue) : "(null)", paramPtr->type); */

    if (newValue == NsfGlobalObjs[NSF___UNKNOWN__]) {
      /* nothing to do here */
      continue;
    }

    /* previous code to handle relations */
    if (paramPtr->converter == ConvertToRelation) {
      ClientData relIdx;
      Tcl_Obj *relationObj = paramPtr->converterArg ? paramPtr->converterArg : paramPtr->nameObj,
	*outObjPtr;
      CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
      
      /*
       * Execute relation cmd in the context above the object frame,
       * since the object frame changes the current namespace as
       * well. References to classes with implicit namespaces might
       * fail otherwise.
       */
      Tcl_Interp_varFramePtr(interp) = varFramePtr->callerPtr;
      result = ConvertToRelationtype(interp, relationObj, paramPtr, &relIdx, &outObjPtr);

      if (result == TCL_OK) {
        result = NsfRelationCmd(interp, object, PTR2INT(relIdx), newValue);
      }
      Tcl_Interp_varFramePtr(interp) = varFramePtr;

      if (result != TCL_OK) {
        Nsf_PopFrameObj(interp, framePtr);
        ParseContextRelease(&pc);
        goto configure_exit;
      }
      /* done with relation handling */
      continue;
    }

    /* special setter for init commands */
    if (paramPtr->flags & (NSF_ARG_INITCMD|NSF_ARG_METHOD)) {
      CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
      NsfCallStackContent csc, *cscPtr = &csc;
      CallFrame frame2, *framePtr2 = &frame2;

      /* The current callframe of configure uses an objframe, such
         that setvar etc.  are able to access variables like "a" as a
         local variable.  However, in the init block, we do not like
         that behavior, since this should look like like a proc body.
         So we push yet another callframe without providing the
         varframe.

         The new frame will have the namespace of the caller to avoid
         the current objframe. Nsf_PushFrameCsc() will establish
         a CMETHOD frame.
      */

      Tcl_Interp_varFramePtr(interp) = varFramePtr->callerPtr;
      cscPtr->flags = 0;
      CscInit(cscPtr, object, NULL /*cl*/, NULL/*cmd*/, NSF_CSC_TYPE_PLAIN, 0);
      Nsf_PushFrameCsc(interp, cscPtr, framePtr2);

      if (paramPtr->flags & NSF_ARG_INITCMD) {
	/* cscPtr->cmdPtr = NSFindCommand(interp, "::eval"); */
        result = Tcl_EvalObjEx(interp, newValue, TCL_EVAL_DIRECT);

      } else /* must be NSF_ARG_METHOD */ {
        Tcl_Obj *ov[3];
        int oc = 0;

	/*
	 * Mark the current frame as inactive such that e.g. volatile
	 * does not use this as a base frame, and methods like
	 * activelevel ignore it.
	 */
	cscPtr->frameType = NSF_CSC_TYPE_INACTIVE;
        if (paramPtr->converterArg) {
          /* if arg= was given, pass it as first argument */
          ov[0] = paramPtr->converterArg;
          oc = 1;
        }
        if (paramPtr->nrArgs == 1) {
          ov[oc] = newValue;
          oc ++;
        }
        result = NsfCallMethodWithArgs((ClientData) object, interp, paramPtr->nameObj,
                                         ov[0], oc, &ov[1], NSF_CSC_IMMEDIATE);
      }
      /*
       * Pop previously stacked frame for eval context and set the
       * varFramePtr to the previous value.
       */
      Nsf_PopFrameCsc(interp, framePtr2);
      CscListRemove(interp, cscPtr);
      CscFinish(interp, cscPtr, "converter object frame");
      Tcl_Interp_varFramePtr(interp) = varFramePtr;

      /*fprintf(stderr, "NsfOConfigureMethod_ attribute %s evaluated %s => (%d)\n",
        ObjStr(paramPtr->nameObj), ObjStr(newValue), result);*/

      if (result != TCL_OK) {
        Nsf_PopFrameObj(interp, framePtr);
        ParseContextRelease(&pc);
        goto configure_exit;
      }

      if (paramPtr->flags & NSF_ARG_INITCMD && RUNTIME_STATE(interp)->doKeepinitcmd) {
	Tcl_ObjSetVar2(interp, paramPtr->nameObj, NULL, newValue, TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
      }

      /* done with init command handling */
      continue;
    }

    /* 
     * Set the instance variable unless the last argument of the
     * definition is varArgs.
     */
    if (i < paramDefs->nrParams || !pc.varArgs) {
#if defined(CONFIGURE_ARGS_TRACE)
      fprintf(stderr, "*** %s SET %s '%s'\n", ObjectName(object), ObjStr(paramPtr->nameObj), ObjStr(newValue));
#endif
      /* Actually set instance variable */
      Tcl_ObjSetVar2(interp, paramPtr->nameObj, NULL, newValue, TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
    }
  }

  Nsf_PopFrameObj(interp, framePtr);
  remainingArgsc = pc.objc - paramDefs->nrParams;

  /*
   * Call residualargs when we have varargs and left over arguments
   */
  if (pc.varArgs && remainingArgsc > 0) {
    Tcl_Obj *methodObj;

    if (CallDirectly(interp, object, NSF_o_residualargs_idx, &methodObj)) {
      i -= 2;
      if (methodObj) {pc.full_objv[i] = methodObj;}
      result = NsfOResidualargsMethod(interp, object, remainingArgsc+1, pc.full_objv + i);
    } else {
      result = CallMethod((ClientData) object, interp,
                          methodObj, remainingArgsc+2, pc.full_objv + i-1, NSF_CSC_IMMEDIATE);
    }

    if (result != TCL_OK) {
      ParseContextRelease(&pc);
      goto configure_exit;
    }
  } else {
    Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_EMPTY]);
  }

  ParseContextRelease(&pc);

 configure_exit:
  return result;
}

/*
objectMethod destroy NsfODestroyMethod {
}
*/
static int
NsfODestroyMethod(Tcl_Interp *interp, NsfObject *object) {
  PRINTOBJ("NsfODestroyMethod", object);

  /*fprintf(stderr,"NsfODestroyMethod %p %s flags %.6x activation %d cmd %p cmd->flags %.6x\n",
          object, ((Command*)object->id)->flags == 0 ? ObjectName(object) : "(deleted)",
          object->flags, object->activationCount, object->id, ((Command*)object->id)->flags);*/

  /*
   * NSF_DESTROY_CALLED might be set already be DispatchDestroyMethod(),
   * the implicit destroy calls. It is necessary to set it here for
   * the explicit destroy calls in the script, which reach the
   * Object->destroy.
   */

  if ((object->flags & NSF_DESTROY_CALLED) == 0) {
    object->flags |= NSF_DESTROY_CALLED;
    /*fprintf(stderr, "NsfODestroyMethod %p sets DESTROY_CALLED %.6x\n",object,object->flags);*/
  }
  object->flags |= NSF_DESTROY_CALLED_SUCCESS;

  if ((object->flags & NSF_DURING_DELETE) == 0) {
    int result;
    Tcl_Obj *methodObj;

    /*fprintf(stderr, "   call dealloc on %p %s\n", object,
      ((Command*)object->id)->flags == 0 ? ObjectName(object) : "(deleted)");*/

    if (CallDirectly(interp, &object->cl->object, NSF_c_dealloc_idx, &methodObj)) {
      result = DoDealloc(interp, object);
    } else {
      /*fprintf(stderr, "call dealloc\n");*/
      result = NsfCallMethodWithArgs((ClientData)object->cl, interp, methodObj,
                                       object->cmdName, 1, NULL, NSF_CSC_IMMEDIATE);
      if (result != TCL_OK) {
        /*
	 * In case, the call of the dealloc method has failed above (e.g. NS_DYING),
         * we have to call dealloc manually, otherwise we have a memory leak
         */
        /*fprintf(stderr, "*** dealloc failed for %p %s flags %.6x, retry\n",
	  object, ObjectName(object), object->flags);*/
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

/*
objectMethod exists NsfOExistsMethod {
  {-argName "varname" -required 1}
}
*/
static int
NsfOExistsMethod(Tcl_Interp *interp, NsfObject *object, CONST char *var) {
  Tcl_SetIntObj(Tcl_GetObjResult(interp), VarExists(interp, object, var, NULL, 1, 1));
  return TCL_OK;
}

/*
objectMethod filterguard NsfOFilterGuardMethod {
  {-argName "filter" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}
*/

static int
NsfOFilterGuardMethod(Tcl_Interp *interp, NsfObject *object, CONST char *filter, Tcl_Obj *guardObj) {
  NsfObjectOpt *opt = object->opt;

  if (opt && opt->filters) {
    NsfCmdList *h = CmdListFindNameInList(interp, filter, opt->filters);
    if (h) {
      if (h->clientData)
        GuardDel((NsfCmdList *) h);
      GuardAdd(interp, h, guardObj);
      object->flags &= ~NSF_FILTER_ORDER_VALID;
      return TCL_OK;
    }
  }

  return NsfPrintError(interp, "Filterguard: can't find filter %s on %s",
		       filter, ObjectName(object));
}

/*
objectMethod instvar NsfOInstvarMethod {
  {-argName "args" -type allargs}
}
*/

static int
NsfOInstvarMethod(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]) {
  callFrameContext ctx = {0};
  int result;

  if (object && (object->filterStack || object->mixinStack) ) {
    CallStackUseActiveFrame(interp, &ctx);
  }

  if (!Tcl_Interp_varFramePtr(interp)) {
    CallStackRestoreSavedFrames(interp, &ctx);
    return NsfPrintError(interp, "instvar used on %s, but callstack is not in procedure scope",
			 ObjectName(object));
  }

  result = NsfImportvar(interp, object, ObjStr(objv[0]), objc-1, objv+1);
  CallStackRestoreSavedFrames(interp, &ctx);
  return result;
}

/*
objectMethod mixinguard NsfOMixinGuardMethod {
  {-argName "mixin" -required 1 -type tclobj}
  {-argName "guard" -required 1 -type tclobj}
}
*/

static int
NsfOMixinGuardMethod(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *mixin, Tcl_Obj *guardObj) {
  NsfObjectOpt *opt = object->opt;

  if (opt && opt->mixins) {
    Tcl_Command mixinCmd = Tcl_GetCommandFromObj(interp, mixin);
    if (mixinCmd) {
      NsfClass *mixinCl = NsfGetClassFromCmdPtr(mixinCmd);
      if (mixinCl) {
	NsfCmdList *h = CmdListFindCmdInList(mixinCmd, opt->mixins);
	if (h) {
	  if (h->clientData) {
	    GuardDel((NsfCmdList *) h);
	  }
	  GuardAdd(interp, h, guardObj);
	  object->flags &= ~NSF_MIXIN_ORDER_VALID;
	  return TCL_OK;
	}
      }
    }
  }

  return NsfPrintError(interp, "Mixinguard: can't find mixin %s on %s",
		       ObjStr(mixin), ObjectName(object));
}

/*
objectMethod noinit NsfONoinitMethod {
}
*/
static int
NsfONoinitMethod(Tcl_Interp *interp, NsfObject *object) {
  object->flags |= NSF_INIT_CALLED;
  return TCL_OK;
}

/*
objectMethod requirenamespace NsfORequireNamespaceMethod {
}
*/
static int
NsfORequireNamespaceMethod(Tcl_Interp *interp, NsfObject *object) {
  RequireObjNamespace(interp, object);
  return TCL_OK;
}

/*
objectMethod residualargs NsfOResidualargsMethod {
  {-argName "args" -type allargs}
}
*/
static int
NsfOResidualargsMethod(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj **argv, **nextArgv, *resultObj;
  int i, start = 1, argc, nextArgc, normalArgs, result = TCL_OK, isdasharg = NO_DASH;
  CONST char *methodName, *nextMethodName;

  /* find arguments without leading dash */
  for (i=start; i < objc; i++) {
    if ((isdasharg = IsDashArg(interp, objv[i], 1, &methodName, &argc, &argv))) {
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
          if ((isdasharg = IsDashArg(interp, objv[j], j==i+1, &nextMethodName, &nextArgc, &nextArgv))) {
            break;
          }
        }
        result = CallConfigureMethod(interp, object, methodName, argc+1, objv+i+1);
        if (result != TCL_OK) {
          return result;
	}
	i += argc;
	break;
      }
    case LIST_DASH:  /* Argument is a list with a leading dash, grouping determined by list */
      {	i++;
	if (i<objc) {
	  isdasharg = IsDashArg(interp, objv[i], 1, &nextMethodName, &nextArgc, &nextArgv);
        }
	result = CallConfigureMethod(interp, object, methodName, argc+1, argv+1);
	if (result != TCL_OK) {
	  return result;
        }
	break;
      }
    default:
      {
	return NsfPrintError(interp, "%s configure: unexpected argument '%s' between parameters", 
			     ObjectName(object), ObjStr(objv[i]));
      }
    }
  }
  resultObj = Tcl_NewListObj(normalArgs, objv+1);
  Tcl_SetObjResult(interp, resultObj);

  return result;
}

/*
objectMethod uplevel NsfOUplevelMethod {
  {-argName "args" -type allargs}
}
*/
static int
NsfOUplevelMethod(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]) {
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
    NsfCallStackFindLastInvocation(interp, 1, &framePtr);
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

/*
objectMethod upvar NsfOUpvarMethod {
  {-argName "args" -type allargs}
}
*/
static int
NsfOUpvarMethod(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj *frameInfoObj = NULL;
  int i, result = TCL_ERROR;
  CONST char *frameInfo;
  callFrameContext ctx = {0};

  if (objc % 2 == 0) {
    frameInfo = ObjStr(objv[1]);
    i = 2;
  } else {
    frameInfoObj = ComputeLevelObj(interp, CALLING_LEVEL);
    INCR_REF_COUNT(frameInfoObj);
    frameInfo = ObjStr(frameInfoObj);
    i = 1;
  }

  if (object && (object->filterStack || object->mixinStack)) {
    CallStackUseActiveFrame(interp, &ctx);
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

/*
objectMethod volatile NsfOVolatileMethod {
}
*/
static int
NsfOVolatileMethod(Tcl_Interp *interp, NsfObject *object) {
  Tcl_Obj *objPtr = object->cmdName;
  int result = TCL_ERROR;
  CONST char *fullName = ObjStr(objPtr);
  CONST char *vn;
  callFrameContext ctx = {0};

  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound != NSF_EXITHANDLER_OFF) {
    fprintf(stderr, "### Can't make objects volatile during shutdown\n");
    return NsfPrintError(interp, "Can't make objects volatile during shutdown");
  }

  CallStackUseActiveFrame(interp, &ctx);

  vn = NSTail(fullName);

  if (Tcl_SetVar2(interp, vn, NULL, fullName, 0)) {
    NsfObjectOpt *opt = NsfRequireObjectOpt(object);

    /*fprintf(stderr, "### setting trace for %s on frame %p\n", fullName,
      Tcl_Interp_varFramePtr(interp));
      NsfShowStack(interp);*/
    result = Tcl_TraceVar(interp, vn, TCL_TRACE_UNSETS,
			  (Tcl_VarTraceProc*)NsfUnsetTrace,
                          (ClientData)objPtr);
    opt->volatileVarName = vn;
  }
  CallStackRestoreSavedFrames(interp, &ctx);

  if (result == TCL_OK) {
    INCR_REF_COUNT(objPtr);
  }
  return result;
}

/*
objectMethod vwait NsfOVwaitMethod {
  {-argName "varname" -required 1}
}
*/
static int
NsfOVwaitMethod(Tcl_Interp *interp, NsfObject *object, CONST char *varname) {
  int done, foundEvent;
  int flgs = TCL_TRACE_WRITES|TCL_TRACE_UNSETS;
  CallFrame frame, *framePtr = &frame;

  /*
   * Make sure the var table exists and the varname is in there
   */
  if (NSRequireVariableOnObj(interp, object, varname, flgs) == 0) {
    return NsfPrintError(interp, "Can't lookup (and create) variable %s on %s",
			 varname, ObjectName(object));
  }
  Nsf_PushFrameObj(interp, object, framePtr);
  /*
   * much of this is copied from Tcl, since we must avoid
   * access with flag TCL_GLOBAL_ONLY ... doesn't work on
   * obj->varTablePtr vars
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
  Nsf_PopFrameObj(interp, framePtr);
  /*
   * Clear out the interpreter's result, since it may have been set
   * by event handlers.
   */
  Tcl_ResetResult(interp);

  if (!foundEvent) {
    return NsfPrintError(interp, "can't wait for variable '%s'; would wait forever", 
			 varname);
  }
  return TCL_OK;
}

/***************************
 * End Object Methods
 ***************************/


/***************************
 * Begin Class Methods
 ***************************/

static int
NsfCAllocMethod_(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *nameObj, Tcl_Namespace *parentNsPtr) {
  CONST char *nameString = ObjStr(nameObj);
  NsfObject *newObj;

  /*
   * create a new object from scratch
   */
  assert(isAbsolutePath(nameString));

  /* TODO the following should be pushed to the outer methods (method create and alloc) 
     instead of being checked here in the internal function
  */

  /*fprintf(stderr, " **** class '%s' wants to alloc '%s'\n", ClassName(cl), nameString);*/
  if (!NSCheckColons(nameString, 0)) {
    return NsfPrintError(interp, "Cannot allocate object - illegal name '%s'", nameString);
  }

  if (IsMetaClass(interp, cl, 1) == 0) {
    /*
     * If the base class is an ordinary class, we create an object.
     */
    newObj = PrimitiveOCreate(interp, nameObj, parentNsPtr, cl);
  } else {
    /*
     * If the base class is a meta-class, we create a class.
     */
    newObj = (NsfObject *)PrimitiveCCreate(interp, nameObj, parentNsPtr, cl);
  }

  if (newObj == NULL) {
    return NsfPrintError(interp, "alloc failed to create '%s' "
			 "(possibly parent namespace does not exist)",
			 nameString);
  }
  
  Tcl_SetObjResult(interp, nameObj);
  return TCL_OK;
}

/*
classMethod alloc NsfCAllocMethod {
  {-argName "name" -required 1 -type tclobj}
}
*/
static int
NsfCAllocMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *nameObj) {
  CONST char *nameString = ObjStr(nameObj);
  Tcl_Namespace *parentNsPtr;
  Tcl_Obj *tmpName;
  int result;

  /*
   * create a new object from scratch
   */

  /*fprintf(stderr, " **** class '%s' wants to alloc '%s'\n", ClassName(cl), nameString);*/
  if (!NSCheckColons(nameString, 0)) {
    return NsfPrintError(interp, "Cannot allocate object - illegal name '%s'", nameString);
  }

  /*
   * If the path is not absolute, we add the appropriate namespace
   */
  if (isAbsolutePath(nameString)) {
    tmpName = NULL;
    parentNsPtr = NULL;
  } else {
    parentNsPtr = CallingNameSpace(interp);
    nameObj = tmpName = NameInNamespaceObj(interp, nameString, parentNsPtr);
    if (strchr(nameString, ':') > 0) {
      parentNsPtr = NULL;
    }
    INCR_REF_COUNT(tmpName);
    /*fprintf(stderr, " **** NoAbsoluteName for '%s' -> determined = '%s' parentNs %s\n",
      nameString, ObjStr(tmpName), parentNsPtr->fullName);*/
    nameString = ObjStr(tmpName);
  }

  result = NsfCAllocMethod_(interp, cl, nameObj, parentNsPtr);

  if (tmpName) {
    DECR_REF_COUNT(tmpName);
  }

  return result;
}

/*
classMethod create NsfCCreateMethod {
  {-argName "name" -required 1}
  {-argName "args" -type allargs}
}
*/
static int
NsfCCreateMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *specifiedName, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *newObject = NULL;
  Tcl_Obj *nameObj, *methodObj, *tmpObj = NULL;
  Tcl_Obj **nobjv;
  int result;
  CONST char *nameString = specifiedName;
  Tcl_Namespace *parentNsPtr;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound != NSF_EXITHANDLER_OFF) {
    fprintf(stderr, "### Can't create object %s during shutdown\n", ObjStr(objv[1]));
    return TCL_OK; /* don't fail, if this happens during destroy, it might be canceled */
  }

  /*fprintf(stderr, "NsfCCreateMethod specifiedName %s\n", specifiedName);*/
  /*
   * complete the name if it is not absolute
   */
  if (!isAbsolutePath(nameString)) {
    parentNsPtr = CallingNameSpace(interp);
    tmpObj = NameInNamespaceObj(interp, nameString, parentNsPtr);
    /* 
     * If the name contains colons, the parentNsPtr is not appropriate
     * for determining the parent 
     */
    if (strchr(nameString, ':') > 0) {
      parentNsPtr = NULL;
    }
    nameString = ObjStr(tmpObj);
    /* fprintf(stderr, " **** fixed name is '%s'\n", nameString);*/
    INCR_REF_COUNT(tmpObj);
    memcpy(tov, objv, sizeof(Tcl_Obj *)*(objc));
    tov[1] = tmpObj;
    nameObj = tmpObj;
    nobjv = tov;
  } else {
    parentNsPtr = NULL;
    nameObj = objv[1];
    nobjv = (Tcl_Obj **)objv;
  }

  /*
   * Check whether we have to call recreate (i.e. when the
   * object exists already)
   */
  newObject = GetObjectFromString(interp, nameString);

  /*fprintf(stderr, "+++ createspecifiedName '%s', nameString '%s', newObject=%p ismeta(%s) %d, ismeta(%s) %d\n",
          specifiedName, nameString, newObject,
          ClassName(cl), IsMetaClass(interp, cl, 1),
          newObject ? ObjStr(newObject->cl->object.cmdName) : "NULL",
          newObject ? IsMetaClass(interp, newObject->cl, 1) : NULL
          );*/

  /* don't allow to
     - recreate an object as a class,
     - recreate a class as an object, and to
     - recreate an object in a different object system

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
    if (CallDirectly(interp, &cl->object, NSF_c_recreate_idx, &methodObj)) {
      result = RecreateObject(interp, cl, newObject, objc, nobjv);
    } else {
      result = CallMethod((ClientData) cl, interp, methodObj,
                          objc+1, nobjv+1, NSF_CM_NO_PROTECT|NSF_CSC_IMMEDIATE);
    }
    if (result != TCL_OK) {
      goto create_method_exit;
    }

    Tcl_SetObjResult(interp, newObject->cmdName);
    nameObj = newObject->cmdName;
    ObjTrace("RECREATE", newObject);

  } else {
    /*
     * newObject might exist here, but will be automatically destroyed by
     * alloc
     */

    /*fprintf(stderr, "alloc ... %s newObject %p \n", ObjStr(nameObj), newObject);*/

    if (CallDirectly(interp, &cl->object, NSF_c_alloc_idx, &methodObj)) {
      result = NsfCAllocMethod_(interp, cl, nameObj, parentNsPtr);
    } else {
      result = CallMethod((ClientData) cl, interp, methodObj,
                          3, &nameObj, NSF_CSC_IMMEDIATE);
    }

    if (result != TCL_OK) {
      goto create_method_exit;
    }
    nameObj = Tcl_GetObjResult(interp);

    if (GetObjectFromObj(interp, nameObj, &newObject) != TCL_OK) {
      result = NsfPrintError(interp, "couldn't find result of alloc");
      goto create_method_exit;
    }

    AddInstance(newObject, cl);

    ObjTrace("CREATE", newObject);

    /* in case, the object is destroyed during initialization, we incr refcount */
    INCR_REF_COUNT(nameObj);
    result = DoObjInitialization(interp, newObject, objc, objv);
    /*fprintf(stderr, "DoObjInitialization %p %s (id %p) returned %d\n",
      newObject, ObjStr(nameObj), newObject->id,  result);*/
    DECR_REF_COUNT(nameObj);
  }
 create_method_exit:

  /*fprintf(stderr, "create -- end ... %s => %d\n", ObjStr(nameObj), result);*/
  if (tmpObj)  {DECR_REF_COUNT(tmpObj);}
  FREE_ON_STACK(Tcl_Obj *, tov);
  return result;
}

/*
classMethod dealloc NsfCDeallocMethod {
  {-argName "object" -required 1 -type tclobj}
}
*/

static int
NsfCDeallocMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *obj) {
  NsfObject *object;

  if (GetObjectFromObj(interp, obj, &object) != TCL_OK) {
    return NsfPrintError(interp, "Can't destroy object %s that does not exist", 
			 ObjStr(obj));
  }

  return DoDealloc(interp, object);
}

/*
classMethod filterguard NsfCFilterGuardMethod {
  {-argName "filter" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}
*/

static int
NsfCFilterGuardMethod(Tcl_Interp *interp, NsfClass *cl,
		      CONST char *filter, Tcl_Obj *guardObj) {
  NsfClassOpt *opt = cl->opt;

  if (opt && opt->classfilters) {
    NsfCmdList *h = CmdListFindNameInList(interp, filter, opt->classfilters);
    if (h) {
      if (h->clientData)
	GuardDel(h);
      GuardAdd(interp, h, guardObj);
      FilterInvalidateObjOrders(interp, cl);
      return TCL_OK;
    }
  }

  return NsfPrintError(interp, "filterguard: can't find filter %s on %s",
		       filter, ClassName(cl));
}

/*
classMethod mixinguard NsfCMixinGuardMethod {
  {-argName "mixin" -required 1 -type tclobj}
  {-argName "guard" -required 1 -type tclobj}
}
*/
static int
NsfCMixinGuardMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *mixin, Tcl_Obj *guardObj) {
  NsfClassOpt *opt = cl->opt;

  if (opt && opt->classmixins) {
    Tcl_Command mixinCmd = Tcl_GetCommandFromObj(interp, mixin);

    if (mixinCmd) {
      NsfClass *mixinCl = NsfGetClassFromCmdPtr(mixinCmd);

      if (mixinCl) {
	NsfCmdList *h = CmdListFindCmdInList(mixinCmd, opt->classmixins);
	if (h) {
	  if (h->clientData) {
	    GuardDel((NsfCmdList *) h);
	  }
	  GuardAdd(interp, h, guardObj);
	  MixinInvalidateObjOrders(interp, cl);
	  return TCL_OK;
	}
      }
    }
  }

  return NsfPrintError(interp, "mixinguard: can't find mixin %s on %s",
		       ObjStr(mixin), ClassName(cl));
}

/*
classMethod new NsfCNewMethod {
  {-argName "-childof" -type object -nrargs 1}
  {-argName "args" -required 0 -type args}
}
*/

static int
NsfCNewMethod(Tcl_Interp *interp, NsfClass *cl, NsfObject *withChildof,
	      int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj *fullnameObj;
  int result, prefixLength;
  Tcl_DString dFullname, *dsPtr = &dFullname;
  NsfStringIncrStruct *iss = &RUNTIME_STATE(interp)->iss;

  Tcl_DStringInit(dsPtr);
  if (withChildof) {
    Tcl_DStringAppend(dsPtr, ObjectName(withChildof), -1);
    Tcl_DStringAppend(dsPtr, "::__#", 5);
  } else {
    Tcl_DStringAppend(dsPtr, "::nsf::__#", 10);
  }
  prefixLength = dsPtr->length;

  while (1) {
    (void)NsfStringIncr(iss);
    Tcl_DStringAppend(dsPtr, iss->start, iss->length);
    if (!Tcl_FindCommand(interp, Tcl_DStringValue(dsPtr), NULL, TCL_GLOBAL_ONLY)) {
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

    callDirectly = CallDirectly(interp, &cl->object, NSF_c_create_idx, &methodObj);

    ov[0] = objv[0];
    ov[1] = methodObj;
    ov[2] = fullnameObj;
    if (objc >= 1)
      memcpy(ov+3, objv, sizeof(Tcl_Obj *)*objc);

    if (callDirectly) {
      result = NsfCCreateMethod(interp, cl, ObjStr(fullnameObj), objc+2, ov+1);
    } else {
      result = ObjectDispatch((ClientData)cl, interp, objc+3, ov, NSF_CSC_IMMEDIATE);
    }

    FREE_ON_STACK(Tcl_Obj *, ov);
  }

  DECR_REF_COUNT(fullnameObj);
  Tcl_DStringFree(dsPtr);

  return result;
}

/*
classMethod recreate NsfCRecreateMethod {
  {-argName "name" -required 1 -type tclobj}
  {-argName "args" -type allargs}
}
*/
static int
RecreateObject(Tcl_Interp *interp, NsfClass *class, NsfObject *object,
	       int objc, Tcl_Obj *CONST objv[]) {
  int result;

  object->flags |= NSF_RECREATE;

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
  result = ChangeClass(interp, object, class);

  if (result == TCL_OK) {
    Tcl_Obj *methodObj;
    /*
     * dispatch "cleanup" method
     */
    if (CallDirectly(interp, object, NSF_o_cleanup_idx, &methodObj)) {
      result = NsfOCleanupMethod(interp, object);
    } else {
      result = CallMethod((ClientData) object, interp, methodObj,
                          2, 0, NSF_CM_NO_PROTECT|NSF_CSC_IMMEDIATE);
    }
  }

  /*
   * Second: if cleanup was successful, initialize the object as usual
   */
  if (result == TCL_OK) {
    result = DoObjInitialization(interp, object, objc, objv);
    if (result == TCL_OK) {
      Tcl_SetObjResult(interp, object->cmdName);
    } else {
      /* fprintf(stderr, "recreate DoObjInitialization returned %d\n", result);*/
    }
  }
  return result;
}

static int
NsfCRecreateMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *nameObj,
		   int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *object;

  if (GetObjectFromObj(interp, nameObj, &object) != TCL_OK)
    return NsfPrintError(interp, "can't recreate non existing object %s", ObjStr(nameObj));

  return RecreateObject(interp, cl, object, objc, objv);
}

/***************************
 * End Class Methods
 ***************************/

#if 0
/***************************
 * Begin check Methods
 ***************************/
static int
NsfCheckBooleanArgs(Tcl_Interp *interp, CONST char *name, Tcl_Obj *valueObj) {
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

static int
NsfCheckRequiredArgs(Tcl_Interp *interp, CONST char *name, Tcl_Obj *valueObj) {
  if (value == NULL) {
    return NsfPrintError(interp, "required arg: '%s' missing", name);
  }
  Tcl_ResetResult(interp);
  Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
  return TCL_OK;
}

/***************************
 * End check Methods
 ***************************/
#endif

static int
AggregatedMethodType(int methodType) {
  switch (methodType) {
  case MethodtypeNULL: /* default */
  case MethodtypeAllIdx:
    methodType = NSF_METHODTYPE_ALL;
    break;
  case MethodtypeScriptedIdx:
    /*methodType = NSF_METHODTYPE_SCRIPTED|NSF_METHODTYPE_ALIAS;*/
    methodType = NSF_METHODTYPE_SCRIPTED;
    break;
  case MethodtypeBuiltinIdx:
    methodType = NSF_METHODTYPE_BUILTIN|NSF_METHODTYPE_OBJECT;
    break;
  case MethodtypeForwarderIdx:
    methodType = NSF_METHODTYPE_FORWARDER;
    break;
  case MethodtypeAliasIdx:
    methodType = NSF_METHODTYPE_ALIAS;
    break;
  case MethodtypeSetterIdx:
    methodType = NSF_METHODTYPE_SETTER;
    break;
  case MethodtypeObjectIdx:
    methodType = NSF_METHODTYPE_OBJECT;
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
objectInfoMethod children NsfObjInfoChildrenMethod {
  {-argName "-type" -required 0 -nrargs 1 -type class}
  {-argName "pattern" -required 0}
}
*/
static int
NsfObjInfoChildrenMethod(Tcl_Interp *interp, NsfObject *object, NsfClass *type, CONST char *pattern) {
  return ListChildren(interp, object, pattern, 0, type);
}

/*
objectInfoMethod class NsfObjInfoClassMethod {
}
*/
static int
NsfObjInfoClassMethod(Tcl_Interp *interp, NsfObject *object) {
  Tcl_SetObjResult(interp, object->cl->object.cmdName);
  return TCL_OK;
}

/*
objectInfoMethod filterguard NsfObjInfoFilterguardMethod {
  {-argName "filter" -required 1}
}
*/
static int
NsfObjInfoFilterguardMethod(Tcl_Interp *interp, NsfObject *object, CONST char *filter) {
  return object->opt ? GuardList(interp, object->opt->filters, filter) : TCL_OK;
}

/*
objectInfoMethod filtermethods NsfObjInfoFiltermethodsMethod {
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern"}
}
*/
static int
NsfObjInfoFiltermethodsMethod(Tcl_Interp *interp, NsfObject *object,
                                    int withGuards, int withOrder,
				    CONST char *pattern) {
  NsfObjectOpt *opt = object->opt;

  if (withOrder) {
    if (!(object->flags & NSF_FILTER_ORDER_VALID))
      FilterComputeDefined(interp, object);
    return FilterInfo(interp, object->filterOrder, pattern, withGuards, 1);
  }
  return opt ? FilterInfo(interp, opt->filters, pattern, withGuards, 0) : TCL_OK;
}

/*
objectInfoMethod forward NsfObjInfoForwardMethod {
  {-argName "-definition"}
  {-argName "name"}
}
*/
static int
NsfObjInfoForwardMethod(Tcl_Interp *interp, NsfObject *object, int withDefinition, CONST char *pattern) {
  return object->nsPtr ?
    ListForward(interp, Tcl_Namespace_cmdTablePtr(object->nsPtr), pattern, withDefinition) :
    TCL_OK;
}

/*
objectInfoMethod hasmixin NsfObjInfoHasMixinMethod {
  {-argName "class" -required 1 -type class}
}
*/
static int
NsfObjInfoHasMixinMethod(Tcl_Interp *interp, NsfObject *object, NsfClass *mixinClass) {
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), HasMixin(interp, object, mixinClass));
  return TCL_OK;
}

/*
objectInfoMethod hasnamespace NsfObjInfoHasnamespaceMethod {
}
*/
static int
NsfObjInfoHasnamespaceMethod(Tcl_Interp *interp, NsfObject *object) {
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), object->nsPtr != NULL);
  return TCL_OK;
}

/*
objectInfoMethod hastype NsfObjInfoHasTypeMethod {
  {-argName "class" -required 1 -type class}
}
*/
static int
NsfObjInfoHasTypeMethod(Tcl_Interp *interp, NsfObject *object, NsfClass *typeClass) {
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), IsSubType(object->cl, typeClass));
  return TCL_OK;
}

/*
objectInfoMethod is NsfObjInfoIsMethod {
  {-argName "objectkind" -required 1 -type "class|baseclass|metaclass"}
}
*/
static int
NsfObjInfoIsMethod(Tcl_Interp *interp, NsfObject *object, int objectkind) {
  int success = 0;

  switch (objectkind) {
  case ObjectkindClassIdx:
    success = (NsfObjectIsClass(object) > 0);
    break;

  case ObjectkindMetaclassIdx:
    success = NsfObjectIsClass(object)
      && IsMetaClass(interp, (NsfClass *)object, 1);
    break;

  case ObjectkindBaseclassIdx:
    success = NsfObjectIsClass(object)
      && IsBaseClass((NsfClass *)object);
    break;
  }
  Tcl_SetIntObj(Tcl_GetObjResult(interp), success);
  return TCL_OK;
}

/*
objectInfoMethod lookupfilter NsfObjInfoLookupFilterMethod {
  {-argName "filter" -required 1}
}
*/
static int
NsfObjInfoLookupFilterMethod(Tcl_Interp *interp, NsfObject *object, CONST char *filter) {
  CONST char *filterName;
  NsfCmdList *cmdList;
  NsfClass *fcl;

  /*
   *  Searches for filter on [self] and returns fully qualified name
   *  if it is not found it returns an empty string
   */
  Tcl_ResetResult(interp);

  if (!(object->flags & NSF_FILTER_ORDER_VALID))
    FilterComputeDefined(interp, object);
  if (!(object->flags & NSF_FILTER_ORDER_DEFINED))
    return TCL_OK;

  for (cmdList = object->filterOrder; cmdList;  cmdList = cmdList->nextPtr) {
    filterName = Tcl_GetCommandName(interp, cmdList->cmdPtr);
    if (filterName[0] == filter[0] && !strcmp(filterName, filter))
      break;
  }

  if (!cmdList)
    return TCL_OK;

  fcl = cmdList->clorobj;
  return ListMethodHandle(interp, (NsfObject*)fcl, !NsfObjectIsClass(&fcl->object), filterName);
}

/*
objectInfoMethod lookupmethod NsfObjInfoLookupMethodMethod {
  {-argName "name" -required 1 -type tclobj}
}
*/
static int
NsfObjInfoLookupMethodMethod(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *methodObj) {
  NsfClass *pcl = NULL;
  Tcl_Command cmd = ObjectFindMethod(interp, object, methodObj, &pcl);

  if (cmd) {
    NsfObject *pobj = pcl ? &pcl->object : object;
    int perObject = (pcl == NULL);

    ListMethod(interp, pobj, pobj, ObjStr(methodObj), cmd, InfomethodsubcmdHandleIdx, perObject);
  }
  return TCL_OK;
}


/*
objectInfoMethod lookupmethods NsfObjInfoLookupMethodsMethod {
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default all}
  {-argName "-incontext"}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-nomixins"}
  {-argName "-path"}
  {-argName "-source" -nrargs 1 -type "all|application|baseclasses"}
  {-argName "pattern" -required 0}
}
*/
static int
NsfObjInfoLookupMethodsMethod(Tcl_Interp *interp, NsfObject *object,
			      int withCallprotection,
			      int withIncontext,
			      int withMethodtype,
			      int withNomixins,
			      int withPath,
			      int withSource,
			      CONST char *pattern) {
  NsfClasses *pl;
  int withPer_object = 1;
  Tcl_HashTable *cmdTablePtr, dupsTable, *dups = &dupsTable;
  int methodType = AggregatedMethodType(withMethodtype);

  /*
   * TODO: we could make this faster for patterns without metachars
   * by letting ListMethodKeys() to signal us when an entry was found.
   * we wait, until the we decided about "info methods defined"
   * vs. "info method search" vs. "info defined" etc.
   */
  if (withCallprotection == CallprotectionNULL) {
    withCallprotection = CallprotectionPublicIdx;
  }
  if (withSource == SourceNULL) {
    withSource = SourceAllIdx;
  }

  Tcl_InitHashTable(dups, TCL_STRING_KEYS);
  if (object->nsPtr) {
    cmdTablePtr = Tcl_Namespace_cmdTablePtr(object->nsPtr);
    if (MethodSourceMatches(interp, withSource, NULL, object)) {
      ListMethodKeys(interp, cmdTablePtr, NULL, pattern, methodType,
		     withCallprotection, withPath,
		     dups, object, withPer_object);
    }
  }

  if (!withNomixins) {
    if (!(object->flags & NSF_MIXIN_ORDER_VALID))
      MixinComputeDefined(interp, object);
    if (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) {
      NsfCmdList *ml;
      NsfClass *mixin;
      for (ml = object->mixinOrder; ml; ml = ml->nextPtr) {
	int guardOk = TCL_OK;
        mixin = NsfGetClassFromCmdPtr(ml->cmdPtr);
	assert(mixin);
        if (withIncontext) {
          if (!RUNTIME_STATE(interp)->guardCount) {
            guardOk = GuardCall(object, 0, 0, interp, ml->clientData, NULL);
          }
        }
        if (mixin && guardOk == TCL_OK) {
          Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(mixin->nsPtr);
	  if (!MethodSourceMatches(interp, withSource, mixin, NULL)) continue;
          ListMethodKeys(interp, cmdTablePtr, NULL, pattern, methodType,
			 withCallprotection, withPath,
                         dups, object, withPer_object);
        }
      }
    }
  }

  /* append method keys from inheritance order */
  for (pl = ComputeOrder(object->cl, object->cl->order, Super); pl; pl = pl->nextPtr) {
    Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(pl->cl->nsPtr);
    if (!MethodSourceMatches(interp, withSource, pl->cl, NULL)) continue;
    ListMethodKeys(interp, cmdTablePtr, NULL, pattern, methodType,
		   withCallprotection, withPath,
                   dups, object, withPer_object);
  }
  Tcl_DeleteHashTable(dups);
  return TCL_OK;
}

/*
objectInfoMethod lookupslots NsfObjInfoLookupSlotsMethod {
  {-argName "-type" -required 0 -nrargs 1 -type class}
}
*/
static int
NsfObjInfoLookupSlotsMethod(Tcl_Interp *interp, NsfObject *object, NsfClass *type) {
  NsfObjects *pl, *slotObjects;
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);

  slotObjects = ComputeSlotObjects(interp, object, type, 1);
  for (pl=slotObjects; pl; pl = pl->nextPtr) {
    Tcl_ListObjAppendElement(interp, list, pl->obj->cmdName);
  }

  NsfObjectListFree(slotObjects);
  Tcl_SetObjResult(interp, list);
  return TCL_OK;
}

/*
objectInfoMethod method NsfObjInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|definition|handle|parameter|parametersyntax|type|precondition|postcondition|subcommands"}
  {-argName "name" -required 1 -type tclobj}
}
*/

static int
NsfObjInfoMethodMethod(Tcl_Interp *interp, NsfObject *object,
		       int subcmd, Tcl_Obj *methodNameObj) {
  NsfObject *regObject, *defObject;
  CONST char *methodName1 = NULL;
  int fromClassNS = 0, result;
  Tcl_DString ds, *dsPtr = &ds;
  Tcl_Command cmd;

  Tcl_DStringInit(dsPtr);
  cmd = ResolveMethodName(interp, object->nsPtr, methodNameObj,
			  dsPtr, &regObject, &defObject, &methodName1, &fromClassNS);
  /*fprintf(stderr,
	  "NsfObjInfoMethodMethod method %s / %s object %p regObject %p defObject %p fromClass %d\n",
	  ObjStr(methodNameObj), methodName1, object,regObject,defObject,fromClassNS);*/
  result = ListMethod(interp,
		      regObject ? regObject : object,
		      defObject ? defObject : object,
		      methodName1, cmd, subcmd, fromClassNS ? 0 : 1);
  Tcl_DStringFree(dsPtr);

  return result;
}

/*
objectInfoMethod methods NsfObjInfoMethodsMethod {
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-incontext"}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-nomixins"}
  {-argName "-path"}
  {-argName "pattern"}
}
*/
static int
NsfObjInfoMethodsMethod(Tcl_Interp *interp, NsfObject *object,
			int withCallproctection, 
			int withIncontext, 
			int withMethodtype, 
			int withNomixins, 
			int withPath,
			CONST char *pattern) {
  return ListDefinedMethods(interp, object, pattern, 1 /* per-object */,
                            AggregatedMethodType(withMethodtype), withCallproctection,
                            withPath, withNomixins, withIncontext);
}

/*
objectInfoMethod mixinclasses NsfObjInfoMixinclassesMethod {
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern" -type objpattern}
}
*/
static int
NsfObjInfoMixinclassesMethod(Tcl_Interp *interp, NsfObject *object,
                                   int withGuards, int withOrder,
                                   CONST char *patternString, NsfObject *patternObj) {

  if (withOrder) {
    if (!(object->flags & NSF_MIXIN_ORDER_VALID))
      MixinComputeDefined(interp, object);
    return MixinInfo(interp, object->mixinOrder, patternString,
		     withGuards, patternObj);
  }
  return object->opt ? MixinInfo(interp, object->opt->mixins, patternString, withGuards, patternObj) : TCL_OK;
}

/*
objectInfoMethod mixinguard NsfObjInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
*/
static int
NsfObjInfoMixinguardMethod(Tcl_Interp *interp, NsfObject *object, CONST char *mixin) {
  return object->opt ? GuardList(interp, object->opt->mixins, mixin) : TCL_OK;
}

/*
objectInfoMethod parent NsfObjInfoParentMethod {
}
*/
static int
NsfObjInfoParentMethod(Tcl_Interp *interp, NsfObject *object) {
  if (object->id) {
    Tcl_SetResult(interp, NSCmdFullName(object->id), TCL_VOLATILE);
  }
  return TCL_OK;
}

/*
objectInfoMethod precedence NsfObjInfoPrecedenceMethod {
  {-argName "-intrinsic"}
  {-argName "pattern" -required 0}
}
*/
static int
NsfObjInfoPrecedenceMethod(Tcl_Interp *interp, NsfObject *object,
                                        int withIntrinsicOnly, CONST char *pattern) {
  NsfClasses *precedenceList = NULL, *pl;

  precedenceList = ComputePrecedenceList(interp, object, pattern, !withIntrinsicOnly, 1);
  for (pl = precedenceList; pl; pl = pl->nextPtr) {
    Tcl_AppendElement(interp, ClassName(pl->cl));
  }
  NsfClassListFree(precedenceList);
  return TCL_OK;
}

/*
objectInfoMethod vars NsfObjInfoVarsMethod {
  {-argName "pattern" -required 0}
}
*/
static int
NsfObjInfoVarsMethod(Tcl_Interp *interp, NsfObject *object, CONST char *pattern) {
  Tcl_Obj *varlist, *okList, *element;
  int i, length;
  TclVarHashTable *varTablePtr = object->nsPtr ?
    Tcl_Namespace_varTablePtr(object->nsPtr) :
    object->varTablePtr;

  ListVarKeys(interp, VarHashTablePtr(varTablePtr), pattern);
  varlist = Tcl_GetObjResult(interp);

  Tcl_ListObjLength(interp, varlist, &length);
  okList = Tcl_NewListObj(0, NULL);
  for (i=0; i<length; i++) {
    Tcl_ListObjIndex(interp, varlist, i, &element);
    if (VarExists(interp, object, ObjStr(element), NULL, 0, 1)) {
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
classInfoMethod filterguard NsfClassInfoFilterguardMethod {
  {-argName "filter" -required 1}
  }
*/
static int
NsfClassInfoFilterguardMethod(Tcl_Interp *interp, NsfClass *class, CONST char *filter) {
  return class->opt ? GuardList(interp, class->opt->classfilters, filter) : TCL_OK;
}

/*
classInfoMethod filtermethods NsfClassInfoFiltermethodsMethod {
  {-argName "-guards"}
  {-argName "pattern"}
}
*/
static int
NsfClassInfoFiltermethodsMethod(Tcl_Interp *interp, NsfClass *class,
				int withGuards, CONST char *pattern) {
  return class->opt ? FilterInfo(interp, class->opt->classfilters, pattern, withGuards, 0) : TCL_OK;
}

/*
classInfoMethod forward NsfClassInfoForwardMethod {
  {-argName "-definition"}
  {-argName "name"}
}
*/
static int
NsfClassInfoForwardMethod(Tcl_Interp *interp, NsfClass *class,
			  int withDefinition, CONST char *pattern) {
  return ListForward(interp, Tcl_Namespace_cmdTablePtr(class->nsPtr), pattern, withDefinition);
}

/*
classInfoMethod heritage NsfClassInfoHeritageMethod {
  {-argName "pattern"}
}
*/
static int
NsfClassInfoHeritageMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *pattern) {
  NsfClasses *pl = ComputeOrder(cl, cl->order, Super);

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
static int
NsfClassInfoInstancesMethod1(Tcl_Interp *interp, NsfClass *startCl,
			     int withClosure, CONST char *pattern, NsfObject *matchObject) {
  Tcl_HashTable *tablePtr = &startCl->instances;
  NsfClasses *sc;
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;
  int rc = 0;

  /*fprintf(stderr, "NsfClassInfoInstancesMethod: clo %d pattern %s match %p\n",
    withClosure, pattern, matchObject);*/

  for (hPtr = Tcl_FirstHashEntry(tablePtr, &search);  hPtr;
       hPtr = Tcl_NextHashEntry(&search)) {
    NsfObject *inst = (NsfObject*) Tcl_GetHashKey(tablePtr, hPtr);
    /*fprintf(stderr, "match '%s' %p %p '%s'\n",
      ObjectName(matchObject), matchObject, inst, ObjectName(inst));*/
    if (matchObject && inst == matchObject) {
      Tcl_SetObjResult(interp, matchObject->cmdName);
      return 1;
    }
    AppendMatchingElement(interp, inst->cmdName, pattern);
  }
  if (withClosure) {
    for (sc = startCl->sub; sc; sc = sc->nextPtr) {
      rc = NsfClassInfoInstancesMethod1(interp, sc->cl, withClosure, pattern, matchObject);
      if (rc) break;
    }
  }
  return rc;
}

/*
classInfoMethod instances NsfClassInfoInstancesMethod {
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
*/
static int
NsfClassInfoInstancesMethod(Tcl_Interp *interp, NsfClass *startCl,
			    int withClosure, CONST char *pattern, NsfObject *matchObject) {
  NsfClassInfoInstancesMethod1(interp, startCl, withClosure, pattern, matchObject);
  return TCL_OK;
}

/*
classInfoMethod method NsfClassInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|definition|handle|parameter|parametersyntax|type|precondition|postcondition|subcommands"}
  {-argName "name" -required 1 -type tclobj}
}
*/

static int
NsfClassInfoMethodMethod(Tcl_Interp *interp, NsfClass *class,
			 int subcmd, Tcl_Obj *methodNameObj) {
  NsfObject *regObject, *defObject;
  CONST char *methodName1 = NULL;
  int fromClassNS = 1, result;
  Tcl_DString ds, *dsPtr = &ds;
  Tcl_Command cmd;

  Tcl_DStringInit(dsPtr);
  cmd = ResolveMethodName(interp, class->nsPtr, methodNameObj,
			  dsPtr, &regObject, &defObject, &methodName1, &fromClassNS);
  /*fprintf(stderr,
	  "NsfClassInfoMethodMethod object %p regObject %p defObject %p fromClass %d cmd %p method %s\n",
	  &class->object,regObject,defObject,fromClassNS, cmd, methodName1);*/
  result = ListMethod(interp,
		      regObject ? regObject : &class->object,
		      defObject ? defObject : &class->object,
		      methodName1,
		      cmd, subcmd, fromClassNS ? 0 : 1);
  Tcl_DStringFree(dsPtr);

  return result;
}

/*
classInfoMethod methods NsfClassInfoMethodsMethod {
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-incontext"}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-nomixins"}
  {-argName "-path"}
  {-argName "pattern"}
}
*/
static int
NsfClassInfoMethodsMethod(Tcl_Interp *interp, NsfClass *class,
			  int withCallproctection, 
			  int withIncontext, 
			  int withMethodtype, 
			  int withNomixins, 
			  int withPath,
			  CONST char *pattern) {
  return ListDefinedMethods(interp, &class->object, pattern, 0 /* per-object */,
                            AggregatedMethodType(withMethodtype), withCallproctection,
			    withPath, withNomixins, withIncontext);
}

/*
classInfoMethod mixinclasses NsfClassInfoMixinclassesMethod {
  {-argName "-closure"}
  {-argName "-guards"}
  {-argName "pattern" -type objpattern}
}
*/
static int
NsfClassInfoMixinclassesMethod(Tcl_Interp *interp, NsfClass *class,
			       int withClosure, int withGuards,
			       CONST char *patternString, NsfObject *patternObj) {
  NsfClassOpt *opt = class->opt;
  int rc;

  if (withClosure) {
    Tcl_HashTable objTable, *commandTable = &objTable;
    MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
    Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
    rc = GetAllClassMixins(interp, commandTable, class, withGuards, patternString, patternObj);
    if (patternObj && rc && !withGuards) {
      Tcl_SetObjResult(interp, rc ? patternObj->cmdName : NsfGlobalObjs[NSF_EMPTY]);
    }
    Tcl_DeleteHashTable(commandTable);
    MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
  } else {
    rc = opt ? MixinInfo(interp, opt->classmixins, patternString, withGuards, patternObj) : TCL_OK;
  }

  return TCL_OK;
}

/*
classInfoMethod mixinguard NsfClassInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
*/
static int
NsfClassInfoMixinguardMethod(Tcl_Interp *interp, NsfClass *class, CONST char *mixin) {
  return class->opt ? GuardList(interp, class->opt->classmixins, mixin) : TCL_OK;
}

/*
classInfoMethod mixinof  NsfClassInfoMixinOfMethod {
  {-argName "-closure"}
  {-argName "-scope" -required 0 -nrargs 1 -type "all|class|object"}
  {-argName "pattern" -type objpattern}
}
*/
static int
NsfClassInfoMixinOfMethod(Tcl_Interp *interp, NsfClass *class, int withClosure, int withScope,
			  CONST char *patternString, NsfObject *patternObj) {
  NsfClassOpt *opt = class->opt;
  int perClass, perObject, rc = TCL_OK;

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
      rc = GetAllClassMixinsOf(interp, commandTable, class, 0, 1, patternString, patternObj);
      if (rc && patternObj) {goto finished;}
    }
    if (perObject) {
      rc = GetAllObjectMixinsOf(interp, commandTable, class, 0, 1, patternString, patternObj);
    }
    Tcl_DeleteHashTable(commandTable);
    MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
  }

 finished:
  if (patternObj) {
    Tcl_SetObjResult(interp, rc ? patternObj->cmdName : NsfGlobalObjs[NSF_EMPTY]);
  }
  return TCL_OK;
}

/*
classInfoMethod subclass NsfClassInfoSubclassMethod {
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
*/
static int
NsfClassInfoSubclassMethod(Tcl_Interp *interp, NsfClass *class, int withClosure,
                             CONST char *patternString, NsfObject *patternObj) {
  int rc;
  if (withClosure) {
    NsfClasses *saved = class->order, *subclasses;
    class->order = NULL;
    subclasses = ComputeOrder(class, class->order, Sub);
    class->order = saved;
    rc = AppendMatchingElementsFromClasses(interp, subclasses ? subclasses->nextPtr:NULL,
					   patternString, patternObj);
    NsfClassListFree(subclasses);
  } else {
    rc = AppendMatchingElementsFromClasses(interp, class->sub, patternString, patternObj);
  }

  if (patternObj) {
    Tcl_SetObjResult(interp, rc ? patternObj->cmdName : NsfGlobalObjs[NSF_EMPTY]);
  }

  return TCL_OK;
}

/*
classInfoMethod superclass NsfClassInfoSuperclassMethod {
  {-argName "-closure"}
  {-argName "pattern" -type tclobj}
}
*/
static int
NsfClassInfoSuperclassMethod(Tcl_Interp *interp, NsfClass *class, int withClosure, Tcl_Obj *pattern) {
  return ListSuperclasses(interp, class, pattern, withClosure);
}

/***************************
 * End Class Info methods
 ***************************/

/*
 * New Tcl Commands
 */

static int
ProcessMethodArguments(ParseContext *pcPtr, Tcl_Interp *interp,
                       NsfObject *object, int pushFrame,
                       NsfParamDefs *paramDefs,
                       CONST char *methodName, int objc, Tcl_Obj *CONST objv[]) {
  int result;
  CallFrame frame, *framePtr = &frame;

  if (object && pushFrame) {
    Nsf_PushFrameObj(interp, object, framePtr);
  }

  result = ArgumentParse(interp, objc, objv, object, objv[0],
                         paramDefs->paramsPtr, paramDefs->nrParams,
			 RUNTIME_STATE(interp)->doCheckArguments,
			 pcPtr);
  if (object && pushFrame) {
    Nsf_PopFrameObj(interp, framePtr);
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

      /*NsfPrintObjv("actual:  ", objc, objv);*/
      ParseContextExtendObjv(pcPtr, paramDefs->nrParams, elts-1, objv + 1 + pcPtr->lastobjc);
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

/* NsfUnsetUnknownArgsCmd was developed and tested for Tcl 8.5 and
 * needs probably modifications for earlier versions of Tcl. However,
 * since CANONICAL_ARGS requires Tcl 8.5 this is not an issue.
 */
/*
 *----------------------------------------------------------------------
 * NsfUnsetUnknownArgsCmd --
 *
 *    Unset variables set from arguments with the default dummy
 *    default value. The dummy default values are set by
 *    ArgumentDefaults()
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    unsets some variables
 *
 *----------------------------------------------------------------------
 */
static int
NsfUnsetUnknownArgsCmd(ClientData clientData, Tcl_Interp *interp, int objc,
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
      /*fprintf(stderr, "NsfUnsetUnknownArgsCmd var '%s' i %d fi %d var %p flags %.8x obj %p unk %p\n",
              ap->name, i, ap->frameIndex, varPtr, varPtr->flags, varPtr->value.objPtr,
              NsfGlobalObjs[NSF___UNKNOWN__]);*/
      if (varPtr->value.objPtr != NsfGlobalObjs[NSF___UNKNOWN__]) continue;
      /*fprintf(stderr, "NsfUnsetUnknownArgsCmd must unset %s\n", ap->name);*/
      Tcl_UnsetVar2(interp, ap->name, NULL, 0);
    }
  }

  return TCL_OK;
}

#ifdef DO_FULL_CLEANUP
/* delete global variables and procs */
static void
DeleteProcsAndVars(Tcl_Interp *interp) {
  Tcl_Namespace *nsPtr = Tcl_GetGlobalNamespace(interp);
  Tcl_HashTable *varTablePtr = nsPtr ? Tcl_Namespace_varTablePtr(ns) : NULL;
  Tcl_HashTable *cmdTablePtr = nsPtr ? Tcl_Namespace_cmdTablePtr(ns) : NULL;
  Tcl_HashSearch search;
  Var *varPtr;
  Tcl_Command cmd;
  register Tcl_HashEntry *entryPtr;
  char *varName;

  for (entryPtr = Tcl_FirstHashEntry(varTablePtr, &search); entryPtr; entryPtr = Tcl_NextHashEntry(&search)) {
    Tcl_Obj *nameObj;
    GetVarAndNameFromHash(entryPtr, &varPtr, &nameObj);
    if (!TclIsVarUndefined(varPtr) || TclIsVarNamespaceVar(varPtr)) {
      /* fprintf(stderr, "unsetting var %s\n", ObjStr(nameObj));*/
      Tcl_UnsetVar2(interp, ObjStr(nameObj), (char *)NULL, TCL_GLOBAL_ONLY);
    }
  }
  for (entryPtr = Tcl_FirstHashEntry(cmdTablePtr, &search); entryPtr; entryPtr = Tcl_NextHashEntry(&search)) {
    cmd = (Tcl_Command)Tcl_GetHashValue(entryPtr);

    if (Tcl_Command_objProc(cmd) == RUNTIME_STATE(interp)->objInterpProc) {
      char *key = Tcl_GetHashKey(cmdTablePtr, entryPtr);

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
ClassHasSubclasses(NsfClass *cl) {
  return (cl->sub != NULL);
}

static int
ClassHasInstances(NsfClass *cl) {
  Tcl_HashSearch hSrch;
  return (Tcl_FirstHashEntry(&cl->instances, &hSrch) != NULL);
}

static int
ObjectHasChildren(Tcl_Interp *interp, NsfObject *object) {
  Tcl_Namespace *ns = object->nsPtr;
  int result = 0;

  if (ns) {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch hSrch;
    Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(ns);

    for (hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch); hPtr;
         hPtr = Tcl_NextHashEntry(&hSrch)) {
      Tcl_Command cmd = Tcl_GetHashValue(hPtr);
      NsfObject *childObject = NsfGetObjectFromCmdPtr(cmd);

      if (childObject) {
        result = 1;
        break;
      }
    }
  }
  return result;
}

static void
FinalObjectDeletion(Tcl_Interp *interp, NsfObject *object) {
  /* 
   * If a call to exit happens from a higher stack frame, the object
   * refcount might not be decremented correctly. If we are in the
   * physical destroy round, we can set the counter to an appropriate
   * value to ensure deletion.
   */
  if (object->refCount != 1) {
    if (object->refCount > 1) {
      NsfLog(interp, NSF_LOG_WARN,  "Have to fix refcount for obj %p refcount %d  (name %s)",
	     object, object->refCount, ObjectName(object));
    } else {
      NsfLog(interp, NSF_LOG_WARN,  "Have to fix refcount for obj %p refcount %d",
	     object, object->refCount);
    }
    object->refCount = 1;
  }

#if !defined(NDEBUG)
  if (object->activationCount != 0)
    fprintf(stderr, "FinalObjectDeletion obj %p activationcount %d\n", object, object->activationCount);
#endif
  assert(object->activationCount == 0);

  if (object->id) {
    /*fprintf(stderr, "cmd dealloc %p final delete refCount %d\n", 
      object->id, Tcl_Command_refCount(object->id));*/
    Tcl_DeleteCommandFromToken(interp, object->id);
  }
}

static void
FreeAllNsfObjectsAndClasses(Tcl_Interp *interp, Tcl_HashTable *commandNameTablePtr) {
  Tcl_HashEntry *hPtr, *hPtr2;
  Tcl_HashSearch hSrch, hSrch2;
  NsfObject *object;
  int deleted = 0;

  /*fprintf(stderr, "FreeAllNsfObjectsAndClasses in %p\n", interp);*/

  RUNTIME_STATE(interp)->exitHandlerDestroyRound = NSF_EXITHANDLER_ON_PHYSICAL_DESTROY;

  /*
   * First delete all child commands of all objects, which are not
   * objects themselves. This will for example delete namespace
   * imported commands and objects and will resolve potential loops in
   * the dependency graph. The result is a plain object/class tree.
   */
  for (hPtr = Tcl_FirstHashEntry(commandNameTablePtr, &hSrch); hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
    char *key = Tcl_GetHashKey(commandNameTablePtr, hPtr);
    object = GetObjectFromString(interp, key);

    /* delete per-object methods */
    if (object && object->nsPtr) {
      for (hPtr2 = Tcl_FirstHashEntry(Tcl_Namespace_cmdTablePtr(object->nsPtr), &hSrch2); hPtr2;
           hPtr2 = Tcl_NextHashEntry(&hSrch2)) {
        Tcl_Command cmd = Tcl_GetHashValue(hPtr2);
        if (cmd) {
	  if (Tcl_Command_objProc(cmd) == NsfObjDispatch) {
	    AliasDeleteObjectReference(interp, cmd);
	    continue;
	  }
	  Tcl_DeleteCommandFromToken(interp, cmd);
	  deleted ++;
        }
      }
    }

    /*
     * Delete class methods; these methods might have aliases (dependencies) to
     * objects, which will resolved this way.
     */
    if (object && NsfObjectIsClass(object)) {
      for (hPtr2 = Tcl_FirstHashEntry(Tcl_Namespace_cmdTablePtr(((NsfClass *)object)->nsPtr),
				      &hSrch2); hPtr2;
           hPtr2 = Tcl_NextHashEntry(&hSrch2)) {
        Tcl_Command cmd = Tcl_GetHashValue(hPtr2);
        if (cmd) {
	  if (Tcl_Command_objProc(cmd) == NsfObjDispatch) {
	    AliasDeleteObjectReference(interp, cmd);
	    continue;
	  }
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
    for (hPtr = Tcl_FirstHashEntry(commandNameTablePtr, &hSrch); hPtr;
	 hPtr = Tcl_NextHashEntry(&hSrch)) {
      char *key = Tcl_GetHashKey(commandNameTablePtr, hPtr);

      object = GetObjectFromString(interp, key);
      if (object && !NsfObjectIsClass(object) && !ObjectHasChildren(interp, object)) {
        /*fprintf(stderr, "  ... delete object %s %p, class=%s id %p\n", key, object,
	  ClassName(object->cl), object->id);*/

        FreeUnsetTraceVariable(interp, object);
        if (object->id) FinalObjectDeletion(interp, object);
        Tcl_DeleteHashEntry(hPtr);
        deleted++;
      }
    }
    /* fprintf(stderr, "deleted %d Objects without dependencies\n", deleted);*/
    if (deleted > 0) {
      continue;
    }

    /*
     * Delete all classes without dependencies
     */
    for (hPtr = Tcl_FirstHashEntry(commandNameTablePtr, &hSrch); hPtr;
	 hPtr = Tcl_NextHashEntry(&hSrch)) {
      char *key = Tcl_GetHashKey(commandNameTablePtr, hPtr);
      NsfClass *cl = GetClassFromString(interp, key);

      /*fprintf(stderr, "cl key = %s %p\n", key, cl);*/
      if (cl
          && !ObjectHasChildren(interp, (NsfObject*)cl)
          && !ClassHasInstances(cl)
          && !ClassHasSubclasses(cl)
          && !IsBaseClass(cl)
          ) {
        /*fprintf(stderr, "  ... delete class %s %p\n", key, cl); */
        FreeUnsetTraceVariable(interp, &cl->object);
        if (cl->object.id) FinalObjectDeletion(interp, &cl->object);

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

  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound == NSF_EXITHANDLER_OFF) {
    NsfFinalizeObjCmd(interp);
  }

  /* must be before freeing of NsfGlobalObjs */
  NsfShadowTclCommands(interp, SHADOW_UNLOAD);

  /* free global objects */
  for (i = 0; i < nr_elements(NsfGlobalStrings); i++) {
    DECR_REF_COUNT(NsfGlobalObjs[i]);
  }
  NsfStringIncrFree(&RUNTIME_STATE(interp)->iss);

#if defined(TCL_MEM_DEBUG)
  TclDumpMemoryInfo(stderr);
  Tcl_DumpActiveMemory("./nsfActiveMem");
  /* Tcl_GlobalEval(interp, "puts {checkmem to checkmemFile};
     checkmem checkmemFile"); */
#endif
  MEM_COUNT_DUMP();

  FREE(Tcl_Obj**, NsfGlobalObjs);
  FREE(NsfRuntimeState, RUNTIME_STATE(interp));

  Tcl_Interp_flags(interp) = flags;
  Tcl_Release((ClientData) interp);
}


#if defined(TCL_THREADS)
/*
 * Gets activated at thread-exit
 */
static void
Nsf_ThreadExitProc(ClientData clientData) {
  /*fprintf(stderr, "+++ Nsf_ThreadExitProc\n");*/

  void Nsf_ExitProc(ClientData clientData);
  Tcl_DeleteExitHandler(Nsf_ExitProc, clientData);
  ExitHandler(clientData);
}
#endif

/*
 * Gets activated at application-exit
 */
void
Nsf_ExitProc(ClientData clientData) {
  /*fprintf(stderr, "+++ Nsf_ExitProc\n");*/
#if defined(TCL_THREADS)
  Tcl_DeleteThreadExitHandler(Nsf_ThreadExitProc, clientData);
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
  Tcl_CreateThreadExitHandler(Nsf_ThreadExitProc, clientData);
#endif
  Tcl_CreateExitHandler(Nsf_ExitProc, clientData);
}

/*
 * Tcl extension initialization routine
 */

extern int
Nsf_Init(Tcl_Interp *interp) {
  ClientData runtimeState;
  int result, i;
#ifdef NSF_BYTECODE
  NsfCompEnv *interpstructions = NsfGetCompEnv();
#endif
  static NsfMutex initMutex = 0;

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
  NsfMutexLock(&initMutex);
  Nsf_OT_byteCodeType = Tcl_GetObjType("bytecode");
  Nsf_OT_tclCmdNameType = Tcl_GetObjType("cmdName");
  Nsf_OT_listType = Tcl_GetObjType("list");
  NsfMutexUnlock(&initMutex);

  /*
    fprintf(stderr, "SIZES: obj=%d, tcl_obj=%d, DString=%d, class=%d, namespace=%d, command=%d, HashTable=%d\n",
    sizeof(NsfObject), sizeof(Tcl_Obj), sizeof(Tcl_DString), sizeof(NsfClass),
    sizeof(Namespace), sizeof(Command), sizeof(Tcl_HashTable));
  */

  /*
   * Runtime State stored in the client data of the Interp's global
   * Namespace in order to avoid global state information
   */
  runtimeState = (ClientData) NEW(NsfRuntimeState);
  memset(runtimeState, 0, sizeof(NsfRuntimeState));

#if USE_ASSOC_DATA
  Tcl_SetAssocData(interp, "NsfRuntimeState", NULL, runtimeState);
#else
  Tcl_Interp_globalNsPtr(interp)->clientData = runtimeState;
#endif

  RUNTIME_STATE(interp)->doFilters = 1;
  RUNTIME_STATE(interp)->doCheckResults = 1;
  RUNTIME_STATE(interp)->doCheckArguments = 1;

  /* create nsf namespace */
  RUNTIME_STATE(interp)->NsfNS =
    Tcl_CreateNamespace(interp, "::nsf", (ClientData)NULL, (Tcl_NamespaceDeleteProc*)NULL);

  MEM_COUNT_ALLOC("TclNamespace", RUNTIME_STATE(interp)->NsfNS);

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

  /* NsfClasses in separate Namespace / Objects */
  RUNTIME_STATE(interp)->NsfClassesNS =
    Tcl_CreateNamespace(interp, "::nsf::classes",	(ClientData)NULL,
                        (Tcl_NamespaceDeleteProc*)NULL);
  MEM_COUNT_ALLOC("TclNamespace", RUNTIME_STATE(interp)->NsfClassesNS);


  /* cache interpreters proc interpretation functions */
  RUNTIME_STATE(interp)->objInterpProc = TclGetObjInterpProc();
  RUNTIME_STATE(interp)->exitHandlerDestroyRound = NSF_EXITHANDLER_OFF;

  RegisterExitHandlers((ClientData)interp);
  NsfStringIncrInit(&RUNTIME_STATE(interp)->iss);
  /* initialize global Tcl_Obj */
  NsfGlobalObjs = NEW_ARRAY(Tcl_Obj*, nr_elements(NsfGlobalStrings));

  for (i = 0; i < nr_elements(NsfGlobalStrings); i++) {
    NsfGlobalObjs[i] = Tcl_NewStringObj(NsfGlobalStrings[i], -1);
    INCR_REF_COUNT(NsfGlobalObjs[i]);
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
  result = NsfShadowTclCommands(interp, SHADOW_LOAD);
  if (result != TCL_OK)
    return result;

  /*
   * new tcl cmds
   */
#ifdef NSF_BYTECODE
  instructions[INST_NEXT].cmdPtr = (Command *)
#endif
    Tcl_CreateObjCommand(interp, "::nsf::xotclnext", NsfNextObjCmd, 0, 0);
#ifdef NSF_BYTECODE
  instructions[INST_SELF].cmdPtr = 
    (Command *)Tcl_FindCommand(interp, "::nsf::current", NULL, TCL_GLOBAL_ONLY);
#endif
  /*Tcl_CreateObjCommand(interp, "::nsf::K", NsfKObjCmd, 0, 0);*/
  
  Tcl_CreateObjCommand(interp, "::nsf::__unset_unknown_args",
		       NsfUnsetUnknownArgsCmd, NULL, NULL);

#ifdef NSF_BYTECODE
  NsfBytecodeInit();
#endif

  Tcl_SetVar(interp, "::nsf::version", NSF_VERSION, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::patchLevel", NSF_PATCHLEVEL, TCL_GLOBAL_ONLY);

  Tcl_AddInterpResolvers(interp,"nxt",
                         (Tcl_ResolveCmdProc*)InterpColonCmdResolver,
                         InterpColonVarResolver,
                         (Tcl_ResolveCompiledVarProc*)InterpCompiledColonVarResolver);
  RUNTIME_STATE(interp)->colonCmd = 
    Tcl_FindCommand(interp, "::nsf::colon", NULL, TCL_GLOBAL_ONLY);

  /*
   * SS: Tcl occassionally resolves a proc's cmd structure (e.g., in
   *  [info frame /number/] or TclInfoFrame()) without
   *  verification. However, NSF non-proc frames, in particular
   *  initcmd blocks, point to the fakeProc structure which does not
   *  come with an initialised Command pointer. For now, we default to
   *  an internal command. However, we need to revisit this decision
   *  as non-proc frames (e.g., initcmds) report a "proc" entry
   *  indicating "::nsf::colon" (which is sufficiently misleading and
   *  reveals internals not to be revealed ...).
  */
  RUNTIME_STATE(interp)->fakeProc.cmdPtr = (Command *)RUNTIME_STATE(interp)->colonCmd;

  {
    /*
     * the file "predefined.h. contains some methods and library procs
     * implemented in Tcl - they could go in a nsf.tcl file, but
     * they're embedded here with Tcl_GlobalEval to avoid the need to
     * carry around a separate file at runtime.
     */

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
# ifdef COMPILE_NSF_STUBS
#  if defined(PRE86)
  Tcl_PkgProvideEx(interp, "nsf", PACKAGE_VERSION, (ClientData)&nsfStubs);
#  else
  Tcl_PkgProvideEx(interp, "nsf", PACKAGE_VERSION, (ClientData)&nsfConstStubPtr);
#  endif
# else
  Tcl_PkgProvide(interp, "nsf", PACKAGE_VERSION);
# endif
#endif

#if !defined(TCL_THREADS)
  if ((Tcl_GetVar2(interp, "tcl_platform", "threaded", TCL_GLOBAL_ONLY) != NULL)) {
    /* a non threaded version of nsf is loaded into a threaded environment */
    fprintf(stderr, "\n A non threaded version of the Next Scripting Framework "
	    "is loaded into threaded environment.\n"
	    "Please reconfigure nsf with --enable-threads!\n\n\n");
  }
#endif

  Tcl_ResetResult(interp);
  Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);

  return TCL_OK;
}


extern int
Nsf_SafeInit(Tcl_Interp *interp) {
  /*** dummy for now **/
  return Nsf_Init(interp);
}

