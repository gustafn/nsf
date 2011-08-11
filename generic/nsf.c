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

#ifdef USE_TCL_STUBS
# define Nsf_ExprObjCmd(clientData, interp, objc, objv)	\
  NsfCallCommand(interp, NSF_EXPR, objc, objv)
#else
# define Nsf_ExprObjCmd(clientData, interp, objc, objv)	\
  Tcl_ExprObjCmd(clientData, interp, objc, objv)
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
  INTERP_MEMBER_DECL
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
  Nsf_Param *paramsPtr;
  INTERP_MEMBER_DECL
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
  INTERP_MEMBER_DECL
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
 * When NSF_MEM_COUNT is set, we want to trace as well the mem-count frees
 * associated with the interp. Therefore, we need in this case a special
 * client data structure.
 */
#ifdef NSF_MEM_COUNT
typedef struct NsfNamespaceClientData {
  NsfObject *object;
  Tcl_Namespace *nsPtr;
  Tcl_Interp *interp;
} NsfNamespaceClientData;
#endif

/*
 * Argv parsing specific definitions
 */

#define PARSE_CONTEXT_PREALLOC 20
typedef struct {
  ClientData *clientData;
  int status;
  Tcl_Obj **objv;
  Tcl_Obj **full_objv;
  int *flags;
  ClientData clientData_static[PARSE_CONTEXT_PREALLOC];
  Tcl_Obj *objv_static[PARSE_CONTEXT_PREALLOC+1];
  int flags_static[PARSE_CONTEXT_PREALLOC+1];
  int lastObjc;
  int objc;
  int varArgs;
  NsfObject *object;
} ParseContext;

static Nsf_TypeConverter ConvertToNothing, ConvertViaCmd;

typedef struct {
  Nsf_TypeConverter *converter;
  char *domain;
} enumeratorConverterEntry;
static enumeratorConverterEntry enumeratorConverterEntries[];

/*
 * Tcl_Obj Types for Next Scripting Objects
 */

static Tcl_ObjType CONST86
  *Nsf_OT_byteCodeType = NULL,
  *Nsf_OT_tclCmdNameType = NULL,
  *Nsf_OT_listType = NULL,
  *Nsf_OT_doubleType = NULL,
  *Nsf_OT_intType = NULL;

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
static int MethodDispatch(ClientData clientData, Tcl_Interp *interp,
			  int objc, Tcl_Obj *CONST objv[],
			  Tcl_Command cmd, NsfObject *object, NsfClass *cl,
			  CONST char *methodName, int frameType, int flags);
static int DispatchDefaultMethod(Tcl_Interp *interp, NsfObject *object, 
				 Tcl_Obj *obj, int flags);
static int DispatchDestroyMethod(Tcl_Interp *interp, NsfObject *object, int flags);
static int DispatchUnknownMethod(Tcl_Interp *interp, NsfObject *object,
				 int objc, Tcl_Obj *CONST objv[], Tcl_Obj *callInfo,
				 Tcl_Obj *methodObj, int flags);

NSF_INLINE static int ObjectDispatch(ClientData clientData, Tcl_Interp *interp, int objc,
                                      Tcl_Obj *CONST objv[], int flags);
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
static void NsfCleanupObject_(INTERP_DECL NsfObject *object);

/* prototypes for object and command lookup */
static NsfObject *GetObjectFromString(Tcl_Interp *interp, CONST char *name);
static NsfClass *GetClassFromString(Tcl_Interp *interp, CONST char *name);
static int GetClassFromObj(Tcl_Interp *interp, register Tcl_Obj *objPtr, NsfClass **clPtr, int withUnknown);
static void GetAllInstances(Tcl_Interp *interp, Tcl_HashTable *destTablePtr, NsfClass *startClass);
NSF_INLINE static Tcl_Command FindMethod(Tcl_Namespace *nsPtr, CONST char *methodName);

/* prototypes for namespace specific calls */
static Tcl_Obj *NameInNamespaceObj(Tcl_Interp *interp, CONST char *name, Tcl_Namespace *ns);
static Tcl_Namespace *CallingNameSpace(Tcl_Interp *interp);
NSF_INLINE static Tcl_Command NSFindCommand(Tcl_Interp *interp, CONST char *name);
static Tcl_Namespace *NSGetFreshNamespace(Tcl_Interp *interp, NsfObject *object,
					  CONST char *name);
static Tcl_Namespace *RequireObjNamespace(Tcl_Interp *interp, NsfObject *object);
static int NSDeleteCmd(Tcl_Interp *interp, Tcl_Namespace *nsPtr, CONST char *methodName);
static void NSNamespacePreserve(INTERP_DECL Tcl_Namespace *nsPtr);
static void NSNamespaceRelease(INTERP_DECL Tcl_Namespace *nsPtr);

/* prototypes for filters and mixins */
static void FilterComputeDefined(Tcl_Interp *interp, NsfObject *object);
static void MixinComputeDefined(Tcl_Interp *interp, NsfObject *object);
NSF_INLINE static void GuardAdd(INTERP_DECL NsfCmdList *filterCL, Tcl_Obj *guardObj);
static int GuardCall(NsfObject *object, Tcl_Interp *interp,
                     Tcl_Obj *guardObj, NsfCallStackContent *cscPtr);
static void GuardDel(INTERP_DECL NsfCmdList *filterCL);

/* properties of objects and classes */
static int IsBaseClass(NsfClass *cl);
static int IsMetaClass(Tcl_Interp *interp, NsfClass *cl, int withMixins);
static int IsSubType(INTERP_DECL NsfClass *subcl, NsfClass *cl);
static NsfClass *DefaultSuperClass(Tcl_Interp *interp, NsfClass *cl, NsfClass *mcl, int isMeta);

/* prototypes for call stack specific calls */
NSF_INLINE static void CscInit_(INTERP_DECL NsfCallStackContent *cscPtr, NsfObject *object, NsfClass *cl,
			       Tcl_Command cmd, int frameType, int flags);
NSF_INLINE static void CscFinish_(Tcl_Interp *interp, NsfCallStackContent *cscPtr);
NSF_INLINE static void CallStackDoDestroy(Tcl_Interp *interp, NsfObject *object);

/* prototypes for  parameter and argument management */
static int NsfInvalidateObjectParameterCmd(Tcl_Interp *interp, NsfClass *cl);
static int ProcessMethodArguments(ParseContext *pcPtr, Tcl_Interp *interp,
                                  NsfObject *object, int pushFrame, NsfParamDefs *paramDefs,
                                  Tcl_Obj *methodNameObj, int objc, Tcl_Obj *CONST objv[]);
static int ParameterCheck(Tcl_Interp *interp, Tcl_Obj *paramObjPtr, Tcl_Obj *valueObj,
			  const char *argNamePrefix, int doCheck, Nsf_Param **paramPtrPtr);
static void ParamDefsRefCountIncr(INTERP_DECL NsfParamDefs *paramDefs);
static void ParamDefsRefCountDecr(INTERP_DECL NsfParamDefs *paramDefs);
static int ParamSetFromAny(Tcl_Interp *interp,	register Tcl_Obj *objPtr);
static int ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
                         NsfObject *obj, Tcl_Obj *procName,
                         Nsf_Param CONST *paramPtr, int nrParameters, int doCheck,
			 ParseContext *pc);
static int ArgumentCheck(Tcl_Interp *interp, Tcl_Obj *objPtr, struct Nsf_Param CONST *pPtr, int doCheck,
			 int *flags, ClientData *clientData, Tcl_Obj **outObjPtr);

/* prototypes for alias management */
static int AliasDelete(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object);
static Tcl_Obj *AliasGet(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName,
			 int withPer_object, int leaveError);
static int AliasDeleteObjectReference(Tcl_Interp *interp, Tcl_Command cmd);
static int NsfMethodAliasCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object,
			     CONST char *methodName, int withFrame, Tcl_Obj *cmdName);

/* prototypes for (class) list handling */
static NsfClasses ** NsfClassListAdd(INTERP_DECL NsfClasses **firstPtrPtr, NsfClass *cl, ClientData clientData);
static void NsfClassListFree(INTERP_DECL NsfClasses *firstPtr);

/* misc prototypes */
static int SetInstVar(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *nameObj, Tcl_Obj *valueObj);
static int ListDefinedMethods(Tcl_Interp *interp, NsfObject *object, CONST char *pattern,
			      int withPer_object, int methodType, int withCallproctection,
			      int withExpand);
static int NextSearchAndInvoke(Tcl_Interp *interp,
			       CONST char *methodName, int objc, Tcl_Obj *CONST objv[],
			       NsfCallStackContent *cscPtr, int freeArgumentVector);
static void AssertionRemoveProc(INTERP_DECL NsfAssertionStore *aStore, CONST char *name);

static void NsfCommandPreserve(INTERP_DECL Tcl_Command cmd);
static void NsfCommandRelease(INTERP_DECL Tcl_Command cmd);
static Tcl_Command GetOriginalCommand(Tcl_Command cmd);
void NsfDStringArgv(Tcl_DString *dsPtr, int objc, Tcl_Obj *CONST objv[]);
static int MethodSourceMatches(int withSource, NsfClass *cl, NsfObject *object);
static void DeleteNsfProcs(Tcl_Interp *interp, Tcl_Namespace *nsPtr);

#ifdef DO_FULL_CLEANUP
static void DeleteProcsAndVars(Tcl_Interp *interp, Tcl_Namespace *nsPtr);
#endif

/*
 *----------------------------------------------------------------------
 *
 * NsfErrorContext --
 *
 *      Print the current errorCode and errorInfo to stderr.
 *      This should be used as the last ressor, when e.g. logging fails
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Output to stderr
 *
 *----------------------------------------------------------------------
 */
static void
NsfErrorContext(Tcl_Interp *interp, CONST char *context) {
  Tcl_DString ds, *dsPtr = &ds;

  Tcl_DStringInit(dsPtr);
  Tcl_DStringAppend(dsPtr, "puts stderr \"Error in ", -1);
  Tcl_DStringAppend(dsPtr, context, -1);
  Tcl_DStringAppend(dsPtr, ":\n$::errorCode $::errorInfo\"", -1);
  Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr), 0);
  Tcl_DStringFree(dsPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * NsfDStringEval --
 *
 *      Evaluate the provided Tcl_DString as a Tcl command and output
 *      the error stack in case of a failure.
 *
 * Results:
 *      Tcl result code.
 *
 * Side effects:
 *      Output to stderr possible.
 *
 *----------------------------------------------------------------------
 */
static int
NsfDStringEval(Tcl_Interp *interp, Tcl_DString *dsPtr, CONST char *context) {
  int result = Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr), 0);
  if (result == TCL_ERROR) {
    NsfErrorContext(interp, context);
  }
  return result;
}

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
 *      Output of the warning.
 *
 *----------------------------------------------------------------------
 */

void
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
    Tcl_DStringAppendElement(&cmdString, "::nsf::log");
    Tcl_DStringAppendElement(&cmdString, level);
    Tcl_DStringAppendElement(&cmdString, Tcl_DStringValue(&ds));
    NsfDStringEval(interp, &cmdString, "log command");
    Tcl_DStringFree(&cmdString);
    Tcl_DStringFree(&ds);
  }
}


/*
 *----------------------------------------------------------------------
 *
 * NsfDeprecatedCmd --
 *
 *      Provide a warning about a depracted command or method. The
 *      message is produced via calling the external Tcl function
 *      ::nsf::deprecated.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Output of the warning.
 *
 *----------------------------------------------------------------------
 */
static void
NsfDeprecatedCmd(Tcl_Interp *interp, CONST char *what, CONST char *oldCmd, CONST char *newCmd) {
  Tcl_DString ds, *dsPtr = &ds;

  assert(what);
  assert(oldCmd);

  Tcl_DStringInit(dsPtr);
  Tcl_DStringAppendElement(dsPtr, "::nsf::deprecated");
  Tcl_DStringAppendElement(dsPtr, what);
  Tcl_DStringAppendElement(dsPtr, oldCmd);
  Tcl_DStringAppendElement(dsPtr, newCmd ? newCmd : "");
  NsfDStringEval(interp, dsPtr, "log command");
  Tcl_DStringFree(dsPtr);
}


/***********************************************************************
 * argv parsing
 ***********************************************************************/
/*
 *----------------------------------------------------------------------
 *
 * ParseContextInit --
 *
 *      Initialize a ParseContext with default values and allocate
 *      memory if needed. Every ParseContext has to be initialized
 *      before usage and has to be freed with ParseContextRelease().
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Allocate potentially memory.
 *
 *----------------------------------------------------------------------
 */
static void
ParseContextInit(INTERP_DECL ParseContext *pcPtr, int objc, NsfObject *object, Tcl_Obj *procName) {
  if (objc < PARSE_CONTEXT_PREALLOC) {
    /* the single larger memset below .... */
    memset(pcPtr, 0, sizeof(ParseContext));
    /* ... is faster than the two smaller memsets below */
    /* memset(pcPtr->clientData_static, 0, sizeof(ClientData)*(objc));
       memset(pcPtr->objv_static, 0, sizeof(Tcl_Obj *)*(objc+1));*/
    pcPtr->full_objv  = &pcPtr->objv_static[0];
    pcPtr->clientData = &pcPtr->clientData_static[0];
    pcPtr->flags      = &pcPtr->flags_static[0];
  } else {
    pcPtr->full_objv  = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+1));
    pcPtr->flags      = (int *)ckalloc(sizeof(int)*(objc+1));
    MEM_COUNT_ALLOC("pcPtr.objv", pcPtr->full_objv);
    pcPtr->clientData = (ClientData *)ckalloc(sizeof(ClientData)*objc);
    MEM_COUNT_ALLOC("pcPtr.clientData", pcPtr->clientData);
    /*fprintf(stderr, "ParseContextMalloc %d objc, %p %p\n", objc, pcPtr->full_objv, pcPtr->clientData);*/
    memset(pcPtr->full_objv, 0, sizeof(Tcl_Obj *)*(objc+1));
    memset(pcPtr->flags, 0, sizeof(int)*(objc+1));
    memset(pcPtr->clientData, 0, sizeof(ClientData)*(objc));
    pcPtr->status     = NSF_PC_STATUS_FREE_OBJV|NSF_PC_STATUS_FREE_CD;
    pcPtr->varArgs    = 0;
  }
  pcPtr->objv = &pcPtr->full_objv[1];
  pcPtr->full_objv[0] = procName;
  pcPtr->object = object;
}

/*
 *----------------------------------------------------------------------
 *
 * ParseContextExtendObjv --
 *
 *      Extend Tcl_Obj array at runtime, when more elements are
 *      needed. This function is called to exten an already
 *      initialized ParseContext.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Allocate potentially memory.
 *
 *----------------------------------------------------------------------
 */
static void
ParseContextExtendObjv(INTERP_DECL ParseContext *pcPtr, int from, int elts, Tcl_Obj *CONST source[]) {
  int requiredSize = from + elts + 1;

  /*NsfPrintObjv("BEFORE: ", pcPtr->objc, pcPtr->full_objv);*/

  if (requiredSize >= PARSE_CONTEXT_PREALLOC) {
    if (pcPtr->objv == &pcPtr->objv_static[1]) {
      /* realloc from preallocated memory */
      pcPtr->full_objv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * requiredSize);
      pcPtr->flags     = (int *)     ckalloc(sizeof(int) * requiredSize);
      MEM_COUNT_ALLOC("pcPtr.objv", pcPtr->full_objv);
      memcpy(pcPtr->full_objv, &pcPtr->objv_static[0], sizeof(Tcl_Obj *) * PARSE_CONTEXT_PREALLOC);
      memcpy(pcPtr->flags, &pcPtr->flags_static[0], sizeof(int) * PARSE_CONTEXT_PREALLOC);
      /*fprintf(stderr, "extend %p alloc %d new objv=%p pcPtr %p\n",
	pcPtr, requiredSize, pcPtr->full_objv, pcPtr);*/

      pcPtr->status     |= NSF_PC_STATUS_FREE_OBJV;
    } else {
      /* realloc from mallocated memory */
      pcPtr->full_objv = (Tcl_Obj **)ckrealloc((char *)pcPtr->full_objv, sizeof(Tcl_Obj *) * requiredSize);
      pcPtr->flags     = (int *)     ckrealloc((char *)pcPtr->flags,     sizeof(int) * requiredSize);
      /*fprintf(stderr, "extend %p realloc %d  new objv=%p pcPtr %p\n",
	pcPtr, requiredSize, pcPtr->full_objv, pcPtr);*/
    }
    pcPtr->objv = &pcPtr->full_objv[1];
  }

  memcpy(pcPtr->objv + from, source, sizeof(Tcl_Obj *) * elts);
  memset(pcPtr->flags + from, 0, sizeof(int) * elts);
  pcPtr->objc += elts;

  /*NsfPrintObjv("AFTER:  ", pcPtr->objc, pcPtr->full_objv);*/
}

/*
 *----------------------------------------------------------------------
 *
 * ParseContextRelease --
 *
 *      Release (and potentially free) the content of a
 *      ParseContext. This function is the counterpart of
 *      ParseContextInit(),
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Free potentially memory.
 *
 *----------------------------------------------------------------------
 */
static void
ParseContextRelease(INTERP_DECL ParseContext *pcPtr) {
  int status = pcPtr->status;

  /*fprintf(stderr, "ParseContextRelease %p status %.6x %d elements, "
	  "called from %s\n",
	  pcPtr, status, pcPtr->objc, msg);*/

#if !defined(NDEBUG)
  {
    /*
     * Perform a general consistency check: although the conents of the parse
     * context are at release time sometimes only partially initializized, the
     * following holds true for ensuring correct release of Tcl_Objs:
     *
     *  1) if one of the objv-flags has  NSF_PC_MUST_DECR set,
     *     then the status flag NSF_PC_STATUS_MUST_DECR has to 
     *     be set as well.
     *
     *  2) if objc>0 then for all objv entries having a flag
     *     different from 0  must have a
     *     TCL_OBJ in the vector.
     *
     *  3) for preallocated objvs, all elements of the objv 
     *     after the argument vector must be 0 or 
     *     NSF_PC_IS_DEFAULT (sanity check)
     */
    /*
     * (1) make sure, that the status correctly reflects MUST_DECR
     */
    int i;

    if (status == 0 || (status & NSF_PC_STATUS_MUST_DECR) == 0) {
      for (i = 0; i < pcPtr->objc - 1; i++) {
	assert((pcPtr->flags[i] & NSF_PC_MUST_DECR) == 0);
      }
    }
    
    /*
     * (2) make sure, Tcl_Objs are set when needed for reclaming memory
     */
    if (pcPtr->objc > 0) {
      /*fprintf(stderr, "%s ", ObjStr(pcPtr->full_objv[0]));*/
      for (i = 0; i < pcPtr->objc - 1; i++) {
	if (pcPtr->flags[i]) {
	  assert(pcPtr->objv[i]);
	  /*fprintf(stderr, "[%d]%s %.6x ", i, ObjStr(pcPtr->objv[i]), pcPtr->flags[i]);*/
	}
      }
    }
    /* 
     * (3) All later flags must be empty or DEFAULT
     */
    if (pcPtr->full_objv == &pcPtr->objv_static[0] && pcPtr->objc > 0) {
      for (i = pcPtr->objc - 1; i < PARSE_CONTEXT_PREALLOC; i++) {
	assert(pcPtr->flags[i] == 0 || pcPtr->flags[i] == NSF_PC_IS_DEFAULT);
      }
    }
  }
#endif
    
  if (status) {
    if (status & NSF_PC_STATUS_MUST_DECR) {
      int i;
      /*fprintf(stderr, "ParseContextRelease %p loop from 0 to %d\n", pcPtr, pcPtr->objc-1);*/
      for (i = 0; i < pcPtr->objc - 1; i++) {
	/*fprintf(stderr, "ParseContextRelease %p check [%d] obj %p flags %.6x & %p\n",
		pcPtr, i, pcPtr->objv[i], 
		pcPtr->flags[i], &(pcPtr->flags[i]));*/
	if (pcPtr->flags[i] & NSF_PC_MUST_DECR) {
	  assert(pcPtr->objv[i]);
	  assert(pcPtr->objv[i]->refCount > 0);
	  /*fprintf(stderr, "... decr ref count on %p\n", pcPtr->objv[i]);*/
	  DECR_REF_COUNT2("valueObj", pcPtr->objv[i]);
	}
      }
    }
    /*
     * Objv can be separately extended; also flags are extend when this
     * happens.
     */
    if (status & NSF_PC_STATUS_FREE_OBJV) {
      /*fprintf(stderr, "ParseContextRelease %p free %p %p\n",
	pcPtr, pcPtr->full_objv, pcPtr->clientData);*/
      MEM_COUNT_FREE("pcPtr.objv", pcPtr->full_objv);
      ckfree((char *)pcPtr->full_objv);
      ckfree((char *)pcPtr->flags);
    }
    /*
     * If the parameter definition was extended at creation time also
     * clientData is extended.
     */
    if (status & NSF_PC_STATUS_FREE_CD) {
      /*fprintf(stderr, "free clientdata for %p\n", pcPtr);*/
      MEM_COUNT_FREE("pcPtr.clientData", pcPtr->clientData);
      ckfree((char *)pcPtr->clientData);
    }
  }
}

/*
 * call an Next Scripting method
 */
static int
CallMethod(ClientData clientData, Tcl_Interp *interp, Tcl_Obj *methodObj,
           int objc, Tcl_Obj *CONST objv[], int flags) {
  NsfObject *object = (NsfObject *) clientData;
  int result;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);
  /*fprintf(stderr, "%%%% callmethod called with method %p\n", methodObj),*/

  tov[0] = object->cmdName;
  tov[1] = methodObj;

  if (objc>2) {
    memcpy(tov+2, objv, sizeof(Tcl_Obj *)*(objc-2));
  }

  /*fprintf(stderr, "%%%% CallMethod cmdname=%s, method=%s, objc=%d\n",
    ObjStr(tov[0]), ObjStr(tov[1]), objc);
    {int i; fprintf(stderr, "\t CALL: %s ", ObjStr(methodObj));for(i=0; i<objc-2; i++) {
    fprintf(stderr, "%s ", ObjStr(objv[i]));} fprintf(stderr, "\n");}*/

  result = ObjectDispatch(clientData, interp, objc, tov, flags);

  FREE_ON_STACK(Tcl_Obj*, tov);
  return result;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfCallMethodWithArgs --
 *
 *      Call method (passed in methodObj) on the object, with the always
 *      provided arg1 and the optional remaining args (passed vis objv).  This
 *      way, we save the memcopy in case no argument or an single argument are
 *      provided (common cases).
 *
 * Results:
 *      Tcl result.
 *
 * Side effects:
 *      Called method might side effect.
 *
 *----------------------------------------------------------------------
 */

extern int
NsfCallMethodWithArgs(Tcl_Interp *interp, Nsf_Object *object, Tcl_Obj *methodObj,
		      Tcl_Obj *arg1, int givenobjc, Tcl_Obj *CONST objv[], int flags) {
  int objc = givenobjc + 2;
  int result;
  ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

  assert(objc>1);
  tov[0] = object->cmdName;
  tov[1] = methodObj;
  if (objc>2) {
    tov[2] = arg1;
  }
  if (objc>3) {
    memcpy(tov+3, objv, sizeof(Tcl_Obj *)*(objc-3));
  }

  /*fprintf(stderr, "%%%% CallMethodWithArgs cmdname=%s, method=%s, arg1 %s objc=%d\n",
	  ObjStr(tov[0]), ObjStr(tov[1]),
	  objc>2 ? ObjStr(tov[2]) : "",  objc);*/
  result = ObjectDispatch(object, interp, objc, tov, flags);

  FREE_ON_STACK(Tcl_Obj*, tov);
  return result;
}

#include "nsfStack.c"


/***********************************************************************
 * Value added replacements of Tcl functions
 ***********************************************************************/
/*
 *----------------------------------------------------------------------
 * Nsf_NextHashEntry --
 *
 *    Function very similar to Tcl_NextHashEntry. If during the iteration of
 *    hash entries some of these entries are removed, Tcl_NextHashEntry() can
 *    lead to a valid looking but invalid hPtr, when the next entry was
 *    already deleted. This seem to occur only, when there are more than 12
 *    hash entries in the table (multiple buckets).  Therefore, we use
 *    numEntries to check, if it is sensible to return a an hash entry. We can
 *    trigger refetch of the hSrchPtr, when the number of expected entries
 *    differs from the numbers of the actual entries.
 *
 * Results:
 *    Hash Entry or NULL.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static Tcl_HashEntry *
Nsf_NextHashEntry(Tcl_HashTable *tablePtr, int expected, Tcl_HashSearch *hSrchPtr) {
  /*fprintf(stderr, "Nsf_NextHashEntry %p expected %d numEntries %d\n",
    tablePtr, expected, tablePtr->numEntries);*/
  if (tablePtr->numEntries < 1) {
    return NULL;
  } else if (tablePtr->numEntries != expected) {
    return Tcl_FirstHashEntry(tablePtr, hSrchPtr);
  } else {
    return Tcl_NextHashEntry(hSrchPtr);
  }
}

/*
 *----------------------------------------------------------------------
 * NsfCommandPreserve --
 *
 *    Increment Tcl's command refCount
 *
 * Results:
 *    void
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static void
NsfCommandPreserve(INTERP_DECL Tcl_Command cmd) {
  Tcl_Command_refCount(cmd)++;
  MEM_COUNT_ALLOC("command.refCount", cmd);
}

/*
 *----------------------------------------------------------------------
 * NsfCommandRelease --
 *
 *    Decrement Tcl command refCount and free it if necessary
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Free pot. memory
 *
 *----------------------------------------------------------------------
 */
static void
NsfCommandRelease(INTERP_DECL Tcl_Command cmd) {
  /*fprintf(stderr,"NsfCommandRelease %p\n", cmd);*/
  MEM_COUNT_FREE("command.refCount", cmd);
  TclCleanupCommandMacro((Command *)cmd);
}

/***********************************************************************
 * Extern callable routines for the preliminary C interface
 ***********************************************************************/
extern Nsf_Object *
NsfGetSelfObj(Tcl_Interp *interp) {
  return (Nsf_Object *) GetSelfObj(interp);
}
extern Nsf_Object *
NsfGetObject(Tcl_Interp *interp, CONST char *name) {
  return (Nsf_Object *) GetObjectFromString(interp, name);
}
extern Nsf_Class *
NsfGetClass(Tcl_Interp *interp, CONST char *name) {
  return (Nsf_Class *)GetClassFromString(interp, name);
}
extern Nsf_Class *
NsfIsClass(Tcl_Interp *UNUSED(interp), ClientData clientData) {
  if (clientData && NsfObjectIsClass((NsfObject *)clientData)) {
    return (Nsf_Class *) clientData;
  }
  return NULL;
}
extern void
NsfRequireObjNamespace(Tcl_Interp *interp, Nsf_Object *object) {
  RequireObjNamespace(interp, (NsfObject *) object);
}
extern Tcl_Obj *
Nsf_ObjSetVar2(Nsf_Object *object, Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
                 Tcl_Obj *valueObj, int flgs) {
  Tcl_Obj *result;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, (NsfObject *)object, framePtr);
  if (((NsfObject *)object)->nsPtr) {
    flgs |= TCL_NAMESPACE_ONLY;
  }
  result = Tcl_ObjSetVar2(interp, name1, name2, valueObj, flgs);
  Nsf_PopFrameObj(interp, framePtr);
  return result;
}
extern Tcl_Obj *
Nsf_ObjGetVar2(Nsf_Object *object, Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
                 int flgs) {
  Tcl_Obj *result;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, (NsfObject *)object, framePtr);
  if (((NsfObject *)object)->nsPtr) {
    flgs |= TCL_NAMESPACE_ONLY;
  }
  result = Tcl_ObjGetVar2(interp, name1, name2, flgs);
  Nsf_PopFrameObj(interp, framePtr);

  return result;
}
extern int
Nsf_UnsetVar2(Nsf_Object *object1, Tcl_Interp *interp,
                   CONST char *name1, CONST char *name2, int flgs) {
  NsfObject *object = (NsfObject *) object1;
  int result;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, object, framePtr);
  if (object->nsPtr) {
    flgs |= TCL_NAMESPACE_ONLY;
  }
  result = Tcl_UnsetVar2(interp, name1, name2, flgs);
  Nsf_PopFrameObj(interp, framePtr);
  return result;
}
extern int
NsfCreate(Tcl_Interp *interp, Nsf_Class *class, Tcl_Obj *nameObj,
	  int objc, Tcl_Obj *CONST objv[]) {
  NsfClass *cl = (NsfClass *) class;
  int result;
  ALLOC_ON_STACK(Tcl_Obj *, objc+2, ov);

  INCR_REF_COUNT2("nameObj", nameObj);

  ov[0] = NULL;
  ov[1] = nameObj;
  if (objc>0) {
    memcpy(ov+2, objv, sizeof(Tcl_Obj *)*objc);
  }
  result = NsfCCreateMethod(interp, cl, ObjStr(nameObj), objc+2, ov);

  FREE_ON_STACK(Tcl_Obj*, ov);
  DECR_REF_COUNT2("nameObj", nameObj);

  return result;
}
extern int
NsfDeleteObject(Tcl_Interp *interp, Nsf_Object *object) {
  return DispatchDestroyMethod(interp, (NsfObject *)object, 0);
}
extern int
NsfRemoveObjectMethod(Tcl_Interp *interp, Nsf_Object *object1, CONST char *methodName) {
  NsfObject *object = (NsfObject *) object1;

  AliasDelete(interp, object->cmdName, methodName, 1);

#if defined(NSF_WITH_ASSERTIONS)
  if (object->opt) {
    AssertionRemoveProc(INTERP object->opt->assertions, methodName);
  }
#endif

  if (object->nsPtr) {
    int rc = NSDeleteCmd(interp, object->nsPtr, methodName);
    if (rc < 0) {
      return NsfPrintError(interp, "%s: cannot delete object specific method '%s'",
			   ObjectName(object), methodName);
    }
  }
  return TCL_OK;
}
extern int
NsfRemoveClassMethod(Tcl_Interp *interp, Nsf_Class *class, CONST char *methodName) {
  NsfClass *cl = (NsfClass *) class;
  int rc;
#if defined(NSF_WITH_ASSERTIONS)
  NsfClassOpt *opt = cl->opt;
#endif

  AliasDelete(interp, class->object.cmdName, methodName, 0);

#if defined(NSF_WITH_ASSERTIONS)
  if (opt && opt->assertions) {
    AssertionRemoveProc(INTERP opt->assertions, methodName);
  }
#endif

  rc = NSDeleteCmd(interp, cl->nsPtr, methodName);
  if (rc < 0) {
    return NsfPrintError(interp, "%s: cannot delete method '%s'", ClassName(cl), methodName);
  }
  return TCL_OK;
}


#if defined(NSFOBJ_TRACE)
void
ObjTrace(char *string, NsfObject *object) {
  if (object) {
    fprintf(stderr, "--- %s tcl %p %s (%d %p) nsf %p (%d) %s \n", string,
            object->cmdName, object->cmdName->typePtr ? object->cmdName->typePtr->name : "NULL",
            object->cmdName->refCount, object->cmdName->internalRep.twoPtrValue.ptr1,
            object, object->refCount, ObjectName(object));
  } else {
    fprintf(stderr, "--- No object: %s\n", string);
  }
}
#else
# define ObjTrace(a, b)
#endif


/*
 *----------------------------------------------------------------------
 * NSTail --
 *
 *    Return the namespace tail of a name.
 *
 * Results:
 *    String.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
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

/*
 *----------------------------------------------------------------------
 * IsClassNsName --
 *
 *    Check, if the provided string starts with the prefix of the
 *    classes namespace.
 *
 * Results:
 *    Boolean.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static int
IsClassNsName(CONST char *string) {
  return (strncmp((string), "::nsf::classes", 14) == 0);
}

/*
 *----------------------------------------------------------------------
 * NSCutNsfClasses --
 *
 *    Removes preceding ::nsf::classes from a string
 *
 * Results:
 *    NsfObject and *fromClasses
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static CONST char *
NSCutNsfClasses(CONST char *string) {
  assert(strncmp((string), "::nsf::classes", 14) == 0);
  return string+14;
}

/*
 *----------------------------------------------------------------------
 * GetObjectFromNsName --
 *
 *    Get object or class from a fully qualified cmd name, such as
 *    e.g. ::nsf::classes::X
 *
 * Results:
 *    NsfObject and *fromClasses
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static NsfObject *
GetObjectFromNsName(Tcl_Interp *interp, CONST char *string, int *fromClassNS) {
  /*

   */
  if (IsClassNsName(string)) {
    *fromClassNS = 1;
    return (NsfObject *)GetClassFromString(interp, NSCutNsfClasses(string));
  } else {
    *fromClassNS = 0;
    return GetObjectFromString(interp, string);
  }
}

/*
 *----------------------------------------------------------------------
 * DStringAppendQualName --
 *
 *    Append to initialized DString the name of the namespace followed
 *    by a simple name (methodName, cmdName).
 *
 * Results:
 *    String pointing to DString value.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static char *
DStringAppendQualName(Tcl_DString *dsPtr, Tcl_Namespace *nsPtr, CONST char *name) {
  int oldLength = Tcl_DStringLength(dsPtr);

  Tcl_DStringAppend(dsPtr, nsPtr->fullName, -1);
  if (Tcl_DStringLength(dsPtr) > (oldLength + 2)) {
    Tcl_DStringAppend(dsPtr, "::", 2);
  }
  Tcl_DStringAppend(dsPtr, name, -1);
  return Tcl_DStringValue(dsPtr);
}

static void
NsfCleanupObject_(INTERP_DECL NsfObject *object) {

  NsfObjectRefCountDecr(object);
  /*fprintf(stderr, "NsfCleanupObject obj refCount of %p after decr %d id %p interp %p flags %.6x\n",
    object, object->refCount, object->id, object->teardown, object->flags);*/

  if (object->refCount <= 0) {
    /*fprintf(stderr, "NsfCleanupObject %p refcount %d\n", object, object->refCount);*/
    assert(object->refCount == 0);
    assert(object->flags & NSF_DELETED);

    /*
     * During FinalObjectDeletion(), object->teardown is NULL, we cannot access
     * the object and class names anymore.
     */
    if (object->teardown && NSF_DTRACE_OBJECT_FREE_ENABLED()) {
      NSF_DTRACE_OBJECT_FREE(ObjectName(object), ClassName(object->cl));
    }

    MEM_COUNT_FREE("NsfObject/NsfClass", object);
#if defined(NSFOBJ_TRACE)
    fprintf(stderr, "CKFREE Object %p refCount=%d\n", object, object->refCount);
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

/*
 *----------------------------------------------------------------------
 * TclObjIsNsfObject --
 *
 *    Check, if the provided Tcl_Obj is bound to a nsf object. If so, return
 *    the NsfObject in the third argument.
 *
 * Results:
 *    True or false,
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static int
TclObjIsNsfObject(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfObject **objectPtr) {
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

/*
 *----------------------------------------------------------------------
 * GetObjectFromObj --
 *
 *    Lookup an Next Scripting object from the given objPtr, preferably from
 *    an object of type "cmdName". On success the NsfObject is returned in the
 *    third argument. The objPtr might be converted by this function.
 *
 * Results:
 *    True or false,
 *
 * Side effects:
 *    object type of objPtr might be changed
 *
 *----------------------------------------------------------------------
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

/*
 *----------------------------------------------------------------------
 * NsfCallUnknownHandler --
 *
 *    Call ::nsf::object::unknown; this function is typically called, when an unknown
 *    object or class is passed as an argument.
 *
 * Results:
 *    Tcl result code
 *
 * Side effects:
 *    Called handler might side effect.
 *
 *----------------------------------------------------------------------
 */

static int
NsfCallUnknownHandler(Tcl_Interp *interp, Tcl_Obj *nameObj) {
  int result = 0;
  Tcl_Obj *ov[3];

  /*fprintf(stderr, "try ::nsf::object::unknown for '%s'\n", ObjStr(nameObj));*/

  ov[0] = NsfGlobalObjs[NSF_UNKNOWN_HANDLER];
  ov[1] = nameObj;

  INCR_REF_COUNT(ov[1]);
  result = Tcl_EvalObjv(interp, 2, ov, 0);
  DECR_REF_COUNT(ov[1]);

  return result;
}

/*
 *----------------------------------------------------------------------
 * GetClassFromObj --
 *
 *    Lookup an Next Scripting class from the given objPtr. If the class could
 *    not be directly converted and withUnknown is true, the function calls
 *    the unknown function (::nsf::object::unknown) to fetch the class on
 *    demand and retries the conversion.  On success the NsfClass is returned
 *    in the third argument. The objPtr might be converted by this function.
 *
 * Results:
 *    True or false,
 *
 * Side effects:
 *    object type of objPtr might be changed
 *
 *----------------------------------------------------------------------
 */

static int
GetClassFromObj(Tcl_Interp *interp, register Tcl_Obj *objPtr,
		NsfClass **clPtr, int withUnknown) {
  NsfObject *object;
  NsfClass *cls = NULL;
  int result = TCL_OK;
  CONST char *objName = ObjStr(objPtr);
  Tcl_Command cmd;

  cmd = Tcl_GetCommandFromObj(interp, objPtr);
  /*fprintf(stderr, "GetClassFromObj %p %s unknown %d cmd %p\n", objPtr, objName, withUnknown, cmd);*/

  if (cmd) {
    cls = NsfGetClassFromCmdPtr(cmd);
#if 1
    if (cls == NULL) {
      /*
       * We have a cmd, but no class; namespace-imported classes are already
       * resolved, but we have to care, if a class is "imported" via "interp
       * alias".
       */
      Tcl_Interp *alias_interp;
      const char *alias_cmd_name;
      Tcl_Obj *nameObj = objPtr;
      Tcl_Obj **alias_ov;
      int alias_oc = 0;

      if (!isAbsolutePath(objName)) {
	nameObj = NameInNamespaceObj(interp, objName, CallingNameSpace(interp));
	objName = ObjStr(nameObj);
	INCR_REF_COUNT(nameObj);
      }

      result = Tcl_GetAliasObj(interp, objName,
			       &alias_interp, &alias_cmd_name, &alias_oc, &alias_ov);
      Tcl_ResetResult(interp);
      //fprintf(stderr, "alias retuns oc %s\n", alias_oc);

      /* we only want interp-aliases with 0 args */
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
#endif
    if (cls) {
      if (clPtr) *clPtr = cls;
      return TCL_OK;
    }
  }

  result = GetObjectFromObj(interp, objPtr, &object);
  if (result == TCL_OK) {
    cls = NsfObjectToClass(object);
    if (cls) {
      if (clPtr) *clPtr = cls;
      return TCL_OK;
    } else {
      /* flag, that we could not convert so far */
      result = TCL_ERROR;
    }
  }

  if (withUnknown) {
    result = NsfCallUnknownHandler(interp, isAbsolutePath(objName) ? objPtr :
				   NameInNamespaceObj(interp,
						      objName,
						      CallingNameSpace(interp)));

    if (result == TCL_OK) {
      /* Retry, but now, the last argument (withUnknown) has to be 0 */
      result = GetClassFromObj(interp, objPtr, clPtr, 0);
    }
    /*fprintf(stderr, "... ::nsf::object::unknown for '%s',
      result %d cl %p\n", objName, result, cl);*/
  }

  return result;
}

/*
 * Version of GetClassFromObj() with external symbol
 */
extern int
NsfGetClassFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
		   NsfClass **clPtr, int withUnknown) {
  return GetClassFromObj(interp, objPtr, clPtr, withUnknown);
}
/*
 *----------------------------------------------------------------------
 * IsObjectOfType --
 *
 *    Check, if the provided NsfObject is of a cetain type. The arguments
 *    "what" and "objPtr" are just used for the error messages. objPtr is the
 *    value from which the object was converted from.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */

static int
IsObjectOfType(Tcl_Interp *interp, NsfObject *object, CONST char *what, Tcl_Obj *objPtr,
			Nsf_Param CONST *pPtr) {
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

  if (pPtr->converterArg == NULL) {
    return TCL_OK;
  }
  if ((GetClassFromObj(interp, pPtr->converterArg, &cl, 0) == TCL_OK)
      && IsSubType(INTERP object->cl, cl)) {
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

/*
 *----------------------------------------------------------------------
 * NameInNamespaceObj --
 *
 *    Create a fully qualified name in the provided namespace or in
 *    the current namespace in form of an Tcl_Obj (with 0 refCount);
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
  DStringAppendQualName(dsPtr, nsPtr, name);
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
NsfReverseClasses(INTERP_DECL NsfClasses *sl) {
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
 *    Frees all elements of the provided class list
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Frees memory.
 *
 *----------------------------------------------------------------------
 */
static void
NsfClassListFree(INTERP_DECL NsfClasses *sl) {
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

static NsfClasses **
NsfClassListAdd(INTERP_DECL NsfClasses **firstPtrPtr, NsfClass *cl, ClientData clientData) {
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

/*
 *----------------------------------------------------------------------
 * NsfClassListAddNoDup --
 *
 *    Add class list entry to the specified list without duplicates. In case
 *    the initial list is empty, *firstPtrPtr is updated as well.
 *
 * Results:
 *    Returns address of next pointer.
 *
 * Side effects:
 *    New list element is allocated.
 *
 *----------------------------------------------------------------------
 */

static NsfClasses **
NsfClassListAddNoDup(INTERP_DECL NsfClasses **firstPtrPtr, NsfClass *cl, ClientData clientData, int *isNewPtr) {
  NsfClasses *clPtr = *firstPtrPtr, **nextPtr;

  if (clPtr) {
    for (; clPtr->nextPtr && clPtr->cl != cl; clPtr = clPtr->nextPtr);
    nextPtr = &clPtr->nextPtr;
  } else {
    nextPtr = firstPtrPtr;
  }

  if (*nextPtr == NULL) {
    NsfClasses *element = NEW(NsfClasses);
    element->cl = cl;
    element->clientData = clientData;
    element->nextPtr = NULL;
    *nextPtr = element;
    if (isNewPtr) *isNewPtr = 1;
  } else {
    if (isNewPtr) *isNewPtr = 0;
  }
  return nextPtr;
}

/*
 *----------------------------------------------------------------------
 * NsfClassListFind --
 *
 *    Find an element in the class list and return it if found.
 *
 * Results:
 *    Found element or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static NsfClasses *
NsfClassListFind(NsfClasses *classList, NsfClass *cl) {
  for (; classList; classList = classList->nextPtr) {
    if (classList->cl == cl) break;
  }
  return classList;
}

#if 0
/* debugging purposes only */
static void
NsfClassListPrint(CONST char *title, NsfClasses *clsList) {
  if (title) {
    fprintf(stderr, "%s", title);
  }
  fprintf(stderr, " %p: ", clsList);
  while (clsList) {
    fprintf(stderr, "%p %s ", clsList->cl, ClassName(clsList->cl));
    clsList = clsList->nextPtr;
  }
  fprintf(stderr, "\n");
}
#endif

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


/*
 * precedence ordering functions
 */

enum colors { WHITE, GRAY, BLACK };

static NsfClasses *
Super(NsfClass *cl) { return cl->super; }

static NsfClasses *
Sub(NsfClass *cl) { return cl->sub; }

static int
TopoSort(INTERP_DECL NsfClass *cl, NsfClass *baseClass, NsfClasses *(*next)(NsfClass *)) {
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
    if (sc->color == WHITE && !TopoSort(INTERP sc, baseClass, next)) {
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
TopoOrder(INTERP_DECL NsfClass *cl, NsfClasses *(*next)(NsfClass *)) {
  if (TopoSort(INTERP cl, cl, next)) {
    return cl->order;
  }
  NsfClassListFree(INTERP cl->order);
  return cl->order = NULL;
}

static NsfClasses *
ComputeOrder(INTERP_DECL NsfClass *cl, NsfClasses *order, NsfClasses *(*direction)(NsfClass *)) {
  if (likely(order != NULL)) {
    return order;
  }
  return cl->order = TopoOrder(INTERP cl, direction);
}

static void
FlushPrecedencesOnSubclasses(INTERP_DECL NsfClass *cl) {
  NsfClasses *pc;
  NsfClassListFree(INTERP cl->order);
  cl->order = NULL;
  pc = ComputeOrder(INTERP cl, cl->order, Sub);

  /*
   * ordering doesn't matter here - we're just using TopoSort
   * to find all lower classes so we can flush their caches
   */

  if (pc) pc = pc->nextPtr;
  for (; pc; pc = pc->nextPtr) {
    NsfClassListFree(INTERP pc->cl->order);
    pc->cl->order = NULL;
  }
  NsfClassListFree(INTERP cl->order);
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
AS(INTERP_DECL NsfClass *s, NsfClasses **sl) {
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
AddSuper(INTERP_DECL NsfClass *cl, NsfClass *super) {
  if (cl && super) {
    /*
     * keep corresponding sub in step with super
     */
    AS(INTERP super, &cl->super);
    AS(INTERP cl, &super->sub);
  }
}

static int
RemoveSuper1(INTERP_DECL NsfClass *s, NsfClasses **sl) {
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
RemoveSuper(INTERP_DECL NsfClass *cl, NsfClass *super) {
  /*
   * keep corresponding sub in step with super
   */
  int sp = RemoveSuper1(INTERP super, &cl->super);
  int sb = RemoveSuper1(INTERP cl, &super->sub);

  return sp && sb;
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
  char *methodName = ObjStr(methodObj);

  if (strchr(methodName, ' ')) {
    Tcl_Namespace *parentNsPtr;
    NsfObject *ensembleObject;
    Tcl_Obj *methodHandleObj;
    int oc, i;
    Tcl_Obj **ov;

    /*fprintf(stderr, "name '%s' contains space \n", methodName);*/

    if ((Tcl_ListObjGetElements(interp, methodObj, &oc, &ov) != TCL_OK)
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
    INCR_REF_COUNT(methodHandleObj);
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
  /* In 8.6: TclIsProc((Command *)cmd) is not equiv to the definition below */
  return (Tcl_Command_objProc(cmd) == TclObjInterpProc);
}

/*
 *----------------------------------------------------------------------
 * GetTclProcFromCommand --
 *
 *    Check if cmd refers to a Tcl proc, and if so, return the proc
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
    return (Proc *)Tcl_Command_objClientData(cmd);
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
    if (entryPtr != NULL) {
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
SearchCMethod(INTERP_DECL /*@notnull@*/ NsfClass *cl, CONST char *methodName, Tcl_Command *cmdPtr) {
  assert(cl);
  return SearchPLMethod(ComputeOrder(INTERP cl, cl->order, Super), methodName, cmdPtr);
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
  return SearchPLMethod(ComputeOrder(INTERP cl, cl->order, Super), ObjStr(methodObj), cmdPtr);
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

  for (pl = ComputeOrder(INTERP cl, cl->order, Super); pl;  pl = pl->nextPtr) {
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
  int containsSpace = strchr(ObjStr(methodObj), ' ') != NULL;
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
  int idx;

  for (idx=0; idx<=NSF_o_unknown_idx; idx++) {
    if (osPtr->methods[idx]) { DECR_REF_COUNT(osPtr->methods[idx]); }
    if (osPtr->handles[idx]) { DECR_REF_COUNT(osPtr->handles[idx]); }
  }

  if (osPtr->rootMetaClass && osPtr->rootClass) {
    RemoveSuper(INTERP osPtr->rootMetaClass, osPtr->rootClass);
    RemoveInstance((NsfObject *)osPtr->rootMetaClass, osPtr->rootMetaClass);
    RemoveInstance((NsfObject *)osPtr->rootClass, osPtr->rootMetaClass);

    FinalObjectDeletion(interp, &osPtr->rootClass->object);
    FinalObjectDeletion(interp, &osPtr->rootMetaClass->object);
  }

  FREE(NsfObjectSystem, osPtr);
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

  for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = osPtr->nextPtr) {
    int i, rootClassMethod, flag = 0;

    for (i=0; i<=NSF_o_unknown_idx; i++) {
      Tcl_Obj *methodObj = osPtr->methods[i];

      if (methodObj && !strcmp(methodName, ObjStr(methodObj))) {
        flag = 1<<i;
	break;
      }
    }
    if (flag == 0) continue;

    rootClassMethod = *(Nsf_SytemMethodOpts[i]+1) == 'o';
	
    if (osPtr->definedMethods & flag) {
      /*
       *  If for some reason (e.g. reload) we redefine the base
       *  methods, these never count as overloads.
       */
      if ((rootClassMethod && object == &defOsPtr->rootClass->object)
	  || (!rootClassMethod && object == &defOsPtr->rootMetaClass->object) ) {
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
      /*
       * Mark the method das defined
       */
      osPtr->definedMethods |= flag;

      /*fprintf(stderr, "+++ %s %.6x defining %s.%s %s osPtr %p defined %.8x flag %.8x\n",
	ClassName(defOsPtr->rootClass),  osPtr->definedMethods, ObjectName(object),
	methodName, Nsf_SytemMethodOpts[i], osPtr, osPtr->definedMethods, flag);*/

      /*
       * If there is a method-handle provided for this system method,
       * register it as a fallback unless the method being defined is
       * already at the root class.
       */
      if (osPtr->handles[i]) {
	NsfObject *defObject = rootClassMethod
	  ? &osPtr->rootClass->object
	  : &osPtr->rootMetaClass->object;
	
	if (defObject != object) {
	  int result = NsfMethodAliasCmd(interp, defObject, 0, methodName, 0, osPtr->handles[i]);
	
	  /*
	   * Since the defObject is not equals the overloaded method, the
	   * definition above is effectively an overload of the alias.
	   */
	  osPtr->overloadedMethods |= flag;
	
	  NsfLog(interp, NSF_LOG_NOTICE, "Define automatically alias %s for %s",
		 ObjStr(osPtr->handles[i]), Nsf_SytemMethodOpts[i]);
	  /*
	   * If the definition was ok, make the method protected.
	   */
	  if (result == TCL_OK) {
	    Tcl_Obj *methodObj = Tcl_GetObjResult(interp);
	    Tcl_Command cmd = Tcl_GetCommandFromObj(interp, methodObj);
	    if (cmd) { Tcl_Command_flags(cmd) |= NSF_CMD_PROTECTED_METHOD; }
	    Tcl_ResetResult(interp);
	  } else {
	    NsfLog(interp, NSF_LOG_WARN, "Could not define alias %s for %s",
		   ObjStr(osPtr->handles[i]), Nsf_SytemMethodOpts[i]);
	  }
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

  for (hPtr = Tcl_FirstHashEntry(commandNameTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
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

  for (hPtr = Tcl_FirstHashEntry(commandNameTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
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
  DeleteProcsAndVars(interp, Tcl_GetGlobalNamespace(interp));
# endif
#endif

  MEM_COUNT_FREE("Tcl_InitHashTable", commandNameTable);
  Tcl_DeleteHashTable(commandNameTable);

  /* now free all objects systems with their root classes */
  for (osPtr = RUNTIME_STATE(interp)->objectSystems; osPtr; osPtr = nPtr) {
    nPtr = osPtr->nextPtr;
    ObjectSystemFree(interp, osPtr);
  }

#ifdef DO_CLEANUP
  /* finally, free all nsfprocs */
  DeleteNsfProcs(interp, NULL);
#endif

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
   * We can/must call a C-implemented method directly, when
   *
   *   a) the object system has no such appropriate method defined
   *
   *   b) the script does not contain a method with the appropriate
   *     name, and
   *
   *   c) filters are not active on the object
   */
  NsfObjectSystem *osPtr = GetObjectSystem(object);
  int callDirectly = 1;
  Tcl_Obj *methodObj;

  methodObj = osPtr->methods[methodIdx];
  /*fprintf(stderr, "OS of %s is %s, method %s methodObj %p osPtr %p defined %.8x %.8x overloaded %.8x %.8x flags %.8x\n",
	  ObjectName(object), ObjectName((&osPtr->rootClass->object)),
	  Nsf_SytemMethodOpts[methodIdx]+1, methodObj,
	  osPtr,
	  osPtr->definedMethods, osPtr->definedMethods & (1 << methodIdx),
	  osPtr->overloadedMethods, osPtr->overloadedMethods & (1 << methodIdx),
	  1 << methodIdx );*/

  if (methodObj) {
    int flag = 1 << methodIdx;
    if ((osPtr->overloadedMethods & flag) != 0) {
      /* overloaded, we must dispatch */
      /*fprintf(stderr, "overloaded\n");*/
      callDirectly = 0;
    } else if ((osPtr->definedMethods & flag) == 0) {
      /* not defined, we must call directly */
      /*fprintf(stderr, "Warning: CallDirectly object %s idx %s not defined\n",
	ObjectName(object), Nsf_SytemMethodOpts[methodIdx]+1);*/
    } else {
#if DISPATCH_ALWAYS_DEFINED_METHODS
      callDirectly = 0;
#else
      if (!(object->flags & NSF_FILTER_ORDER_VALID)) {
        FilterComputeDefined(interp, object);
      }
      /*fprintf(stderr, "CallDirectly object %s idx %s object flags %.6x %.6x \n",
	      ObjectName(object), Nsf_SytemMethodOpts[methodIdx]+1,
	      (object->flags & NSF_FILTER_ORDER_DEFINED_AND_VALID),
	      NSF_FILTER_ORDER_DEFINED_AND_VALID);*/
      if ((object->flags & NSF_FILTER_ORDER_DEFINED_AND_VALID) == NSF_FILTER_ORDER_DEFINED_AND_VALID) {
        /*fprintf(stderr, "CallDirectly object %s idx %s has filter \n",
	  ObjectName(object), Nsf_SytemMethodOpts[methodIdx]+1);*/
        callDirectly = 0;
      }
#endif
    }
  }

  /*fprintf(stderr, "CallDirectly object %s idx %d returns %s => %d\n",
          ObjectName(object), methodIdx,
          methodObj ? ObjStr(methodObj) : "(null)", callDirectly);*/

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
NsfMethodObj(NsfObject *object, int methodIdx) {
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

static NsfObjectOpt *
NsfRequireObjectOpt(INTERP_DECL NsfObject *object) {
  if (!object->opt) {
    object->opt = NEW(NsfObjectOpt);
    memset(object->opt, 0, sizeof(NsfObjectOpt));
  }
  return object->opt;
}

static NsfClassOpt *
NsfRequireClassOpt(INTERP_DECL /*@notnull@*/ NsfClass *cl) {
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
    nsPtr = object->nsPtr = NSGetFreshNamespace(interp, object,
						ObjectName(object));
    assert(nsPtr);

    /*
     * Copy all obj variables to the newly created namespace
     */
    if (object->varTablePtr) {
      Tcl_HashSearch  search;
      Tcl_HashEntry   *hPtr;
      TclVarHashTable *varTablePtr = Tcl_Namespace_varTablePtr(nsPtr);
      Tcl_HashTable   *varHashTablePtr = TclVarHashTablePtr(varTablePtr);
      Tcl_HashTable   *objHashTablePtr = TclVarHashTablePtr(object->varTablePtr);
	
      *varHashTablePtr = *objHashTablePtr; /* copy the table */
	
      if (objHashTablePtr->buckets == objHashTablePtr->staticBuckets) {
        varHashTablePtr->buckets = varHashTablePtr->staticBuckets;
      }
      for (hPtr = Tcl_FirstHashEntry(varHashTablePtr, &search); hPtr;
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
CONST char *
MethodName(Tcl_Obj *methodObj) {
  char *methodName;

  assert(methodObj);
  methodName = ObjStr(methodObj);
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
NsColonVarResolver(Tcl_Interp *interp, CONST char *varName, Tcl_Namespace *UNUSED(nsPtr),
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

typedef struct NsfResolvedVarInfo {
  Tcl_ResolvedVarInfo vInfo;        /* This must be the first element. */
  NsfObject *lastObject;
  Tcl_Var var;
  Tcl_Obj *nameObj;
  INTERP_MEMBER_DECL
} NsfResolvedVarInfo;

/*
 *----------------------------------------------------------------------
 * HashVarFree --
 *
 *    Free hashed variables based on refCount.
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
  NsfResolvedVarInfo *resVarInfo = (NsfResolvedVarInfo *)vinfoPtr;
  NsfCallStackContent *cscPtr = CallStackGetTopFrame0(interp);
  NsfObject *object = cscPtr ? cscPtr->self : NULL;
  TclVarHashTable *varTablePtr;
  Tcl_Var var = resVarInfo->var;
  int new;

#if defined(VAR_RESOLVER_TRACE)
  int flags = var ? ((Var *)var)->flags : 0;
  fprintf(stderr,"CompiledColonVarFetch var '%s' var %p flags = %.4x dead? %.4x\n",
	  ObjStr(resVarInfo->nameObj), var, flags, flags & VAR_DEAD_HASH);
#endif

  /*
   * We cache lookups based on nsf objects; we have to care about
   * cases, where the instance variables are in some delete states.
   *
   */

  if (likely(var && object == resVarInfo->lastObject &&
	     (((((Var *)var)->flags) & VAR_DEAD_HASH)) == 0)) {
    /*
     * The variable is valid.
     */
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, ".... cached var '%s' var %p flags = %.4x\n",
            ObjStr(resVarInfo->nameObj), var, ((Var *)var)->flags);
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

  resVarInfo->lastObject = object;
#if defined(VAR_RESOLVER_TRACE)
  fprintf(stderr,"Fetch var %s in object %s\n", TclGetString(resVarInfo->nameObj), ObjectName(object));
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
    Var *v = (Var *)(resVarInfo->var);
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
  NsfResolvedVarInfo *resVarInfo = (NsfResolvedVarInfo *)vInfoPtr;
  INTERP_MEMBER_GET(resVarInfo)

#if defined(VAR_RESOLVER_TRACE)
  fprintf(stderr, "CompiledColonVarFree %p for variable '%s'\n", 
	  resVarInfo, ObjStr(resVarInfo->nameObj));
#endif

  DECR_REF_COUNT(resVarInfo->nameObj);
  if (resVarInfo->var) {HashVarFree(resVarInfo->var);}
  FREE(NsfResolvedVarInfo, vInfoPtr);
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
 *    fetcher is executed whenever a Tcl call frame is intialized and
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
			       CONST84 char *name, int length, Tcl_Namespace *UNUSED(context),
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
    NsfResolvedVarInfo *resVarInfo = NEW(NsfResolvedVarInfo);

    resVarInfo->vInfo.fetchProc = CompiledColonVarFetch;
    resVarInfo->vInfo.deleteProc = CompiledColonVarFree; /* if NULL, Tcl does a ckfree on proc clean up */
    resVarInfo->lastObject = NULL;
    resVarInfo->var = NULL;
    resVarInfo->nameObj = Tcl_NewStringObj(name+1, length-1);
    INTERP_MEMBER_SET(resVarInfo, interp)
    INCR_REF_COUNT(resVarInfo->nameObj);

#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, "... resVarInfo %p nameObj %p '%s' obj %p %s\n",
	    resVarInfo, resVarInfo->nameObj, ObjStr(resVarInfo->nameObj), 
	    object, ObjectName(object));
#endif

    *rPtr = (Tcl_ResolvedVarInfo *)resVarInfo;

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
InterpColonVarResolver(Tcl_Interp *interp, CONST char *varName, Tcl_Namespace *UNUSED(nsPtr),
		       int flags, Tcl_Var *varPtr) {
  int new, frameFlags;
  CallFrame *varFramePtr;
  TclVarHashTable *varTablePtr;
  NsfObject *object;
  Tcl_Obj *keyObj;
  Tcl_Var var;

  /*
   * TCL_GLOBAL_ONLY is removed, since "vwait :varName" is called with
   * with this flag.
   */
  if (!FOR_COLON_RESOLVER(varName) || (flags & (/*TCL_GLOBAL_ONLY|*/TCL_NAMESPACE_ONLY))) {
    /* ordinary names and global lookups are not for us */
#if defined(VAR_RESOLVER_TRACE)
    fprintf(stderr, "InterpColonVarResolver '%s' flags %.6x not for us\n",
            varName, flags);
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
InterpColonCmdResolver(Tcl_Interp *interp, CONST char *cmdName, Tcl_Namespace *UNUSED(nsPtr),
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

#if defined(NSF_WITH_OS_RESOLVER)
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
NsfNamespaceInit(Tcl_Namespace *nsPtr) {

  assert(nsPtr);
  /*
   * This puts a per-object namespace resolver into position upon
   * acquiring the namespace. Works for object-scoped commands/procs
   * and object-only ones (set, unset, ...)
   */
  Tcl_SetNamespaceResolvers(nsPtr, /*(Tcl_ResolveCmdProc *)NsColonCmdResolver*/ NULL,
                            NsColonVarResolver,
                            /*(Tcl_ResolveCompiledVarProc *)NsCompiledColonVarResolver*/NULL);
#if defined(NSF_WITH_INHERIT_NAMESPACES)
  /*
   * In case there is a namespace path set for the parent namespace,
   * apply this as well to the object namespace to avoid surprises
   * with "namespace path nx".
   */
  { Namespace *parentNsPtr = Tcl_Namespace_parentPtr(nsPtr);
    int pathLength = Tcl_Namespace_commandPathLength(parentNsPtr);

    if (pathLength>0) {
      Namespace **pathArray = (Namespace **)ckalloc(sizeof(Namespace *) * pathLength);
      NamespacePathEntry *tmpPathArray = Tcl_Namespace_commandPathArray(parentNsPtr);
      int i;

      for (i=0; i<pathLength; i++) {
	pathArray[i] = tmpPathArray[i].nsPtr;
      }
      TclSetNsPath((Namespace *)nsPtr, pathLength, (Tcl_Namespace **)pathArray);
      ckfree((char *)pathArray);
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
    NsfNamespaceInit(object->nsPtr);
  }
  assert(object->nsPtr);

  return object->nsPtr;
}

/*
 * Namespace related commands
 */
/*
 *----------------------------------------------------------------------
 * NSNamespacePreserve --
 *
 *    Increment namespace refCount
 *
 * Results:
 *    void
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static void
NSNamespacePreserve(INTERP_DECL Tcl_Namespace *nsPtr) {
  assert(nsPtr);
  MEM_COUNT_ALLOC("NSNamespace", nsPtr);
  Tcl_Namespace_refCount(nsPtr)++;
}
/*
 *----------------------------------------------------------------------
 * NSNamespaceRelease --
 *
 *    Decrement namespace refCount and free namespace if necessary
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Free pot. memory
 *
 *----------------------------------------------------------------------
 */
static void
NSNamespaceRelease(INTERP_DECL Tcl_Namespace *nsPtr) {

  assert(nsPtr);
  MEM_COUNT_FREE("NSNamespace", nsPtr);
  Tcl_Namespace_refCount(nsPtr)--;
  if (Tcl_Namespace_refCount(nsPtr) == 0 && (Tcl_Namespace_flags(nsPtr) & NS_DEAD)) {
    /*
     * The namespace refCount has reached 0, we have to free
     * it. unfortunately, NamespaceFree() is not exported
     */
    fprintf(stderr, "HAVE TO FREE namespace %p\n", nsPtr); 
    /*NamespaceFree(nsPtr);*/
    ckfree(nsPtr->fullName);
    ckfree(nsPtr->name);
    ckfree((char *)nsPtr);
  }
}

/*
 *----------------------------------------------------------------------
 * NSDeleteCmd --
 *
 *    Delete the Tcl command for the provided methodName located in
 *    the provided namespace.
 *
 * Results:
 *    Tcl result or -1, if no such method exists int.
 *
 * Side effects:
 *    Command is deleted.
 *
 *----------------------------------------------------------------------
 */
static int
NSDeleteCmd(Tcl_Interp *interp, Tcl_Namespace *nsPtr, CONST char *methodName) {
  Tcl_Command token;

  assert(nsPtr);
  if ((token = FindMethod(nsPtr, methodName))) {
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
static int
NSDeleteChild(Tcl_Interp *interp, Tcl_Command cmd, int deleteObjectsOnly) {

  /*fprintf(stderr, "NSDeleteChildren child %p flags %.6x epoch %d\n",
	  cmd, Tcl_Command_flags(cmd), Tcl_Command_cmdEpoch(cmd));*/

  /*
   * In some situations (e.g. small buckets, less than 12 entries), we
   * get from the cmd-table already deleted cmds; we had previously an
   * assert(Tcl_Command_cmdEpoch(cmd) == 0);
   * which will fail in such cases.
   */

  if (!Tcl_Command_cmdEpoch(cmd)) {
    NsfObject *object = NsfGetObjectFromCmdPtr(cmd);

    /*fprintf(stderr, "NSDeleteChildren child %p (%s) epoch %d\n",
      cmd, Tcl_GetCommandName(interp, cmd), Tcl_Command_cmdEpoch(cmd));*/

    if (object == NULL) {
      /*
       * This is just a plain Tcl command; let Tcl handle the
       * deletion.
       */
      return 0;
    }

    /*fprintf(stderr, "NSDeleteChild check %p %s true child %d\n",
      object, ObjectName(object), object->id == cmd);*/

    /* delete here just true children */
    if (object->id == cmd) {

      if (deleteObjectsOnly && NsfObjectIsClass(object)) {
	return 0;
      }

      /*fprintf(stderr, "NSDeleteChild destroy %p %s\n", object, ObjectName(object));*/

      /* in the exit handler physical destroy --> directly call destroy */
      if (RUNTIME_STATE(interp)->exitHandlerDestroyRound
	  == NSF_EXITHANDLER_ON_PHYSICAL_DESTROY) {
	PrimitiveDestroy(object);
	return 1;
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
	  return 1;
	}
      }
    } else {
      /*fprintf(stderr, "NSDeleteChild remove alias %p %s\n", object, Tcl_GetCommandName(interp, cmd));*/
      return AliasDeleteObjectReference(interp, cmd);
    }
  }
  return 0;
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
  int expected;

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "NSDeleteChildren %p %s activationCount %d\n",
	  nsPtr, nsPtr->fullName, Tcl_Namespace_activationCount(nsPtr));
#endif

  /*
   * First, get rid of namespace imported objects; don't delete the
   * object, but the reference.
   */
  Tcl_ForgetImport(interp, nsPtr, "*"); /* don't destroy namespace imported objects */


#if OBJDELETION_TRACE
  /*
   * Deletion is always tricky. Show, what elements should be deleted
   * in this loop. The actually deleted elements might be acutally
   * less, if a deletion of one item triggers the destroy of another
   * item.
   */
  for (hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
    fprintf(stderr, "will destroy %p %s\n", cmd, Tcl_GetCommandName(interp, cmd));
  }
#endif

  /*
   * Second, delete the objects.
   */

  /*
   * A destroy of one element of the hash table can trigger the
   * destroy of another item of the same table. Therefore we use
   * Nsf_NextHashEntry(), which handles this case.
   */
  for (hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch); hPtr;
       hPtr = Nsf_NextHashEntry(cmdTablePtr, expected, &hSrch)) {
    /*Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
    fprintf(stderr, "NSDeleteChild %p table %p numEntries before %d\n",
    cmd, hPtr->tablePtr, cmdTablePtr->numEntries );*/
    expected = cmdTablePtr->numEntries -
      NSDeleteChild(interp, (Tcl_Command)Tcl_GetHashValue(hPtr), 1);
  }
 /*
  * Finally, delete the classes.
   */
  for (hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch); hPtr;
       hPtr = Nsf_NextHashEntry(cmdTablePtr, expected, &hSrch)) {
    expected = cmdTablePtr->numEntries -
      NSDeleteChild(interp, (Tcl_Command)Tcl_GetHashValue(hPtr), 0);
  }
}


/*
 * delete all vars & procs in a namespace
 */
static void
NSCleanupNamespace(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  TclVarHashTable *varTablePtr = Tcl_Namespace_varTablePtr(nsPtr);
  Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(nsPtr);
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "NSCleanupNamespace %p flags %.6x\n", nsPtr, Tcl_Namespace_flags(nsPtr));
  fprintf(stderr, "NSCleanupNamespace %p %.6x varTablePtr %p\n", nsPtr, ((Namespace *)nsPtr)->flags, varTablePtr);
#endif
  /*
   * Delete all variables and initialize var table again
   * (DeleteVars frees the vartable)
   */
  TclDeleteVars((Interp *)interp, varTablePtr);
  TclInitVarHashTable(varTablePtr, (Namespace *)nsPtr);

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
      /*fprintf(stderr, "NSCleanupNamespace calls DeleteCommandFromToken for %p flags %.6x\n",
                cmd, ((Command *)cmd)->flags);
      fprintf(stderr, "    cmd = %s\n", Tcl_GetCommandName(interp, cmd));
      fprintf(stderr, "    nsPtr = %p\n", ((Command *)cmd)->nsPtr);
      fprintf(stderr, "    epoch = %d\n", Tcl_Command_cmdEpoch(cmd));
      fprintf(stderr, "    refCount = %d\n", Tcl_Command_refCount(cmd));
      fprintf(stderr, "    flags %.6x\n", ((Namespace *)((Command *)cmd)->nsPtr)->flags);*/

      Tcl_DeleteCommandFromToken(interp, cmd);
  }
}

static void
NSNamespaceDeleteProc(ClientData clientData) {
#ifdef NSF_MEM_COUNT
  NsfNamespaceClientData *nsClientData = (NsfNamespaceClientData *)clientData;
  NsfObject *object;

  assert(clientData);
  /*fprintf(stderr, "NSNamespaceDeleteProc cd %p\n", clientData);
    fprintf(stderr, "... nsPtr %p name '%s'\n", nsClientData->nsPtr, nsClientData->nsPtr->fullName);*/

  object = nsClientData->object;
  INTERP_MEMBER_GET(nsClientData)

  ckfree((char *)nsClientData);
#else
  NsfObject *object = (NsfObject *) clientData;
#endif

  assert(object);

  /*fprintf(stderr, "namespacedeleteproc obj=%p ns=%p\n",
    clientData, object ? object->nsPtr : NULL);*/

  MEM_COUNT_FREE("NSNamespace", object->nsPtr);
  object->nsPtr = NULL;
}

void
Nsf_DeleteNamespace(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  int activationCount = 0;
  Tcl_CallFrame *f = (Tcl_CallFrame *)Tcl_Interp_framePtr(interp);

  /*fprintf(stderr, "Nsf_DeleteNamespace %p ", nsPtr);*/

  while (f) {
    if (f->nsPtr == nsPtr) {
      activationCount++;
    }
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

  //MEM_COUNT_FREE("TclNamespace", nsPtr);
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
    if (*n == ':' && *(n+1) == ':' && *(n+2) == ':') {
      return 0;   /* more than 2 colons in series in a name */
    }
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
NSGetFreshNamespace(Tcl_Interp *interp, NsfObject *object, CONST char *name) {
  Namespace *dummy1Ptr, *dummy2Ptr, *nsPtr;
  const char *dummy;

  TclGetNamespaceForQualName(interp, name, NULL, TCL_FIND_ONLY_NS|TCL_CREATE_NS_IF_UNKNOWN,
			     &nsPtr, &dummy1Ptr, &dummy2Ptr, &dummy);

  if (nsPtr->deleteProc != NSNamespaceDeleteProc) {
    /* 
     * Avoid hijacking a namespace with different client data 
     */
    if (nsPtr->deleteProc || nsPtr->clientData) {
      Tcl_Panic("Namespace '%s' exists already with delProc %p and clientData %p; "
		"Can only convert a plain Tcl namespace into an nsf namespace, my delete Proc %p",
		name, nsPtr->deleteProc, nsPtr->clientData, NSNamespaceDeleteProc);
    }

    {
#ifdef NSF_MEM_COUNT
      NsfNamespaceClientData *nsClientData = (NsfNamespaceClientData *)ckalloc(sizeof(NsfNamespaceClientData));

      nsClientData->object = object;
      nsClientData->nsPtr = (Tcl_Namespace *)nsPtr;
      INTERP_MEMBER_SET(nsClientData, interp)
      nsPtr->clientData = nsClientData;

      /*fprintf(stderr, "Adding NsfNamespaceClientData nsPtr %p cd %p name '%s'\n", 
	nsPtr, nsClientData, nsPtr->fullName);*/
#else
      nsPtr->clientData = object;
#endif
      nsPtr->deleteProc = (Tcl_NamespaceDeleteProc *)NSNamespaceDeleteProc;
    }
    MEM_COUNT_ALLOC("NSNamespace", nsPtr);
  } else {
    fprintf(stderr, "NSGetFreshNamespace: reusing namespace %p %s\n", nsPtr, nsPtr->fullName);
  }

  return (Tcl_Namespace *)nsPtr;
}

/*
 *----------------------------------------------------------------------
 * NSRequireParentObject --
 *
 *    Try to require a parent object (e.g. during ttrace).  This function
 *    tries to load a parent object via ::nsf::object::unknown.
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
NSRequireParentObject(Tcl_Interp *interp, CONST char *parentName) {
  int result;

  result = NsfCallUnknownHandler(interp, Tcl_NewStringObj(parentName, -1));

  if (result == TCL_OK) {
    NsfObject *parentObj = (NsfObject *) GetObjectFromString(interp, parentName);
    if (parentObj) {
      RequireObjNamespace(interp, parentObj);
    }
    result = (Tcl_FindNamespace(interp, parentName,
			    (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY) != NULL);
  }
  return result;

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
NSCheckNamespace(Tcl_Interp *interp, CONST char *nameString, Tcl_Namespace *parentNsPtr1) {
  Namespace *nsPtr, *dummy1Ptr, *dummy2Ptr, *parentNsPtr = (Namespace *)parentNsPtr1;
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
			     &nsPtr, &dummy1Ptr, &dummy2Ptr, &dummy);
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
    parentNsPtr = Tcl_Namespace_parentPtr(nsPtr);
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
    /*
     * search for last '::'
     */
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
    parentObj = (NsfObject *) GetObjectFromString(interp, parentName);
    /*fprintf(stderr, "parentName %s parentObj %p\n", parentName, parentObj);*/

    if (parentObj) {
      RequireObjNamespace(interp, parentObj);
    } else if (nsPtr == NULL && parentNsPtr == NULL) {
      TclGetNamespaceForQualName(interp, parentName, NULL,
				 TCL_GLOBAL_ONLY|TCL_FIND_ONLY_NS,
				 &parentNsPtr, &dummy1Ptr,
				 &dummy2Ptr, &dummy);
      if (parentNsPtr == NULL) {
	/*fprintf(stderr, "===== calling NSRequireParentObject %s %p\n", parentName, cl);*/
	NSRequireParentObject(interp, parentName);
      }
    }

    if (parentNameLength) {
      DSTRING_FREE(dsPtr);
    }
  }

  return (Tcl_Namespace *)nsPtr;
}


/*
 *----------------------------------------------------------------------
 * NSFindCommand --
 *
 *    Find the "real" command belonging eg. to an Next Scripting class or object.
 *    Do not return cmds produced by Tcl_Import, but the "real" cmd
 *    to which they point.
 *
 * Results:
 *    Tcl_Command or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

NSF_INLINE static Tcl_Command
NSFindCommand(Tcl_Interp *interp, CONST char *name) {
  Tcl_Command cmd;

  assert(name);
  assert(*name == ':' && *(name + 1) == ':');

  cmd = Tcl_FindCommand(interp, name, NULL, TCL_GLOBAL_ONLY);
  if (cmd) {
    Tcl_Command importedCmd = TclGetOriginalCommand(cmd);
    if (importedCmd) {
      cmd = importedCmd;
    }
  }
  return cmd;
}

/*
 *----------------------------------------------------------------------
 * GetObjectFromString --
 *
 *    Lookup an object from a given string. The function performs a
 *    command lookup (every object is a command) and checks, if the
 *    command is bound to an nsf object.
 *
 * Results:
 *    NsfObject* or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static NsfObject *
GetObjectFromString(Tcl_Interp *interp, CONST char *name) {
  register Tcl_Command cmd;

  assert(name);
  /*fprintf(stderr, "GetObjectFromString name = '%s'\n", name);*/
  cmd = NSFindCommand(interp, name);

  if (cmd && Tcl_Command_objProc(cmd) == NsfObjDispatch) {
    /*fprintf(stderr, "GetObjectFromString cd %p\n", Tcl_Command_objClientData(cmd));*/
    return (NsfObject *)Tcl_Command_objClientData(cmd);
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * GetClassFromString --
 *
 *    Lookup a class from a given string. The function performs an
 *    object lookup and checks, if the object is a class
 *
 * Results:
 *    NsfClass* or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static NsfClass *
GetClassFromString(Tcl_Interp *interp, CONST char *name) {
  NsfObject *object = GetObjectFromString(interp, name);
  return (object && NsfObjectIsClass(object)) ? (NsfClass *)object : NULL;
}

/*
 *----------------------------------------------------------------------
 * CanRedefineCmd --
 *
 *    This function tests, whether a method (provided as a string) is
 *    allowed to be redefined in a provided namespace.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
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

/*
 *----------------------------------------------------------------------
 * NsfAddObjectMethod --
 *
 *    Externally callable function to register an object level method
 *    for the provided object.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Newly created Tcl command.
 *
 *----------------------------------------------------------------------
 */
extern int
NsfAddObjectMethod(Tcl_Interp *interp, Nsf_Object *object1, CONST char *methodName,
                     Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp,
                     int flags) {
  NsfObject *object = (NsfObject *)object1;
  Tcl_DString newCmdName, *dsPtr = &newCmdName;
  Tcl_Namespace *ns = RequireObjNamespace(interp, object);
  Tcl_Command newCmd;
  int result;

  /* Check, if we are allowed to redefine the method */
  result = CanRedefineCmd(interp, object->nsPtr, object, (char *)methodName);
  if (unlikely(result != TCL_OK)) {
    return result;
  }

  /* delete an alias definition, if it exists */
  AliasDelete(interp, object->cmdName, methodName, 1);

  Tcl_DStringInit(dsPtr);
  DStringAppendQualName(dsPtr, ns, methodName);

  newCmd = Tcl_CreateObjCommand(interp, Tcl_DStringValue(dsPtr), proc, clientData, dp);

  if (flags) {
    ((Command *) newCmd)->flags |= flags;
  }
  Tcl_DStringFree(dsPtr);
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * NsfAddClassMethod --
 *
 *    Externally callable function to register an class level method
 *    for the provided class.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Newly created Tcl command.
 *
 *----------------------------------------------------------------------
 */
extern int
NsfAddClassMethod(Tcl_Interp *interp, Nsf_Class *class, CONST char *methodName,
                       Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp,
                       int flags) {
  NsfClass *cl = (NsfClass *)class;
  Tcl_DString newCmdName, *dsPtr = &newCmdName;
  Tcl_Command newCmd;
  int result;

   /* Check, if we are allowed to redefine the method */
  result = CanRedefineCmd(interp, cl->nsPtr, &cl->object, (char *)methodName);
  if (unlikely(result != TCL_OK)) {
    return result;
  }

 /* delete an alias definition, if it exists */
  AliasDelete(interp, class->object.cmdName, methodName, 0);

  Tcl_DStringInit(dsPtr);
  DStringAppendQualName(dsPtr, cl->nsPtr, methodName);

  newCmd = Tcl_CreateObjCommand(interp, Tcl_DStringValue(dsPtr), proc, clientData, dp);

  if (flags) {
    ((Command *) newCmd)->flags |= flags;
  }
  Tcl_DStringFree(dsPtr);
  return TCL_OK;
}

/*
 * Autonaming
 */

static Tcl_Obj *
AutonameIncr(Tcl_Interp *interp, Tcl_Obj *nameObj, NsfObject *object,
             int instanceOpt, int resetOpt) {
  int valueLength;
  char *valueString, *c;
  Tcl_Obj *valueObj, *resultObj = NULL, *savedResultObj = NULL;
  int flgs = TCL_LEAVE_ERR_MSG;
  CallFrame frame, *framePtr = &frame;

  Nsf_PushFrameObj(interp, object, framePtr);
  if (object->nsPtr) {
    flgs |= TCL_NAMESPACE_ONLY;
  }
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
    Tcl_ObjSetVar2(interp, NsfGlobalObjs[NSF_AUTONAMES], nameObj,
		   valueObj, flgs);
  }

  if (resetOpt) {
    if (valueObj) { /* we have an entry */
      Tcl_UnsetVar2(interp, NsfGlobalStrings[NSF_AUTONAMES], ObjStr(nameObj), flgs);
    }
    resultObj = NsfGlobalObjs[NSF_EMPTY];
    INCR_REF_COUNT2("autoname", resultObj);
  } else {
    int mustCopy = 1, format = 0;

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
        resultObj = Tcl_NewStringObj(buffer, 1);
        INCR_REF_COUNT2("autoname", resultObj);
        Tcl_AppendLimitedToObj(resultObj, nextChars, -1, INT_MAX, NULL);
        mustCopy = 0;
      }
    }
    if (mustCopy) {
      resultObj = Tcl_DuplicateObj(nameObj);
      INCR_REF_COUNT2("autoname", resultObj);
      /*
        fprintf(stderr, "*** copy %p %s = %p\n", name, ObjStr(name), resultObj);
      */
    }
    /* if we find a % in the autoname -> We use Tcl_FormatObjCmd
       to let the autoname string be formated, like Tcl "format"
       command, with the value. E.g.:
       autoname a%06d --> a000000, a000001, a000002, ...
    */
    for (c = ObjStr(resultObj); *c != '\0'; c++) {
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
      savedResultObj = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(savedResultObj);
      ov[1] = resultObj;
      ov[2] = valueObj;
      if (NsfCallCommand(interp, NSF_FORMAT, 3, ov) != TCL_OK) {
        Nsf_PopFrameObj(interp, framePtr);
        DECR_REF_COUNT(savedResultObj);
        FREE_ON_STACK(Tcl_Obj*, ov);
        return NULL;
      }
      DECR_REF_COUNT(resultObj);
      resultObj = Tcl_DuplicateObj(Tcl_GetObjResult(interp));
      INCR_REF_COUNT2("autoname", resultObj);
      Tcl_SetObjResult(interp, savedResultObj);
      DECR_REF_COUNT(savedResultObj);
      FREE_ON_STACK(Tcl_Obj*, ov);
    } else {
      valueString = Tcl_GetStringFromObj(valueObj, &valueLength);
      Tcl_AppendLimitedToObj(resultObj, valueString, valueLength, INT_MAX, NULL);
      /*fprintf(stderr, "+++ append to obj done\n");*/
    }
  }

  Nsf_PopFrameObj(interp, framePtr);
  assert((resetOpt && resultObj->refCount>=1) || (resultObj->refCount == 1));
  return resultObj;
}

/*
 * Next Scripting CallStack functions
 */

NSF_INLINE static void
CallStackDoDestroy(Tcl_Interp *interp, NsfObject *object) {
  Tcl_Command oid;

  /*fprintf(stderr, "CallStackDoDestroy %p flags %.6x\n", object, object->flags);*/
  PRINTOBJ("CallStackDoDestroy", object);

  /* Don't do anything, if a recursive DURING_DELETE is for some
   * reason active.
   */
  if (object->flags & NSF_DURING_DELETE) {
    return;
  }
  /*fprintf(stderr, "CallStackDoDestroy %p flags %.6x activation %d object->refCount %d cmd %p \n",
    object, object->flags, object->activationCount, object->refCount, object->id);*/

  object->flags |= NSF_DURING_DELETE;
  oid = object->id;
  /* oid might be freed already, we can't even use (((Command *)oid)->flags & CMD_IS_DELETED) */

  if (object->teardown && oid) {
    /*
     * PrimitiveDestroy() has to be before DeleteCommandFromToken(),
     * otherwise e.g. unset traces on this object cannot be executed
     * from Tcl. We make sure via refCounting that the object
     * structure is kept until after DeleteCommandFromToken().
     */
    NsfObjectRefCountIncr(object);

    PrimitiveDestroy(object);

    if (!(object->flags & NSF_TCL_DELETE)) {
      Tcl_Obj *savedResultObj = Tcl_GetObjResult(interp);
      INCR_REF_COUNT(savedResultObj);
      /*fprintf(stderr, "    before DeleteCommandFromToken %p object flags %.6x\n", oid, object->flags);*/
      /*fprintf(stderr, "cmd dealloc %p refCount %d dodestroy \n", oid, Tcl_Command_refCount(oid));*/
      Tcl_DeleteCommandFromToken(interp, oid); /* this can change the result */
      /*fprintf(stderr, "    after DeleteCommandFromToken %p %.6x\n", oid, ((Command *)oid)->flags);*/
      Tcl_SetObjResult(interp, savedResultObj);
      DECR_REF_COUNT(savedResultObj);
    }
    NsfCleanupObject(INTERP object, "CallStackDoDestroy");
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
CmdListAdd(INTERP_DECL NsfCmdList **cList, Tcl_Command c, NsfClass *clorobj, int noDuplicates) {
  NsfCmdList *l = *cList, *new;

  /*
   * check for duplicates, if necessary
   */
  if (noDuplicates) {
    NsfCmdList *h = l, **end = NULL;
    while (h) {
      if (h->cmdPtr == c) {
        return h;
      }
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
  NsfCommandPreserve(INTERP new->cmdPtr);
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
CmdListReplaceCmd(INTERP_DECL NsfCmdList *replace, Tcl_Command cmd, NsfClass *clorobj) {
  Tcl_Command del = replace->cmdPtr;
  replace->cmdPtr = cmd;
  replace->clorobj = clorobj;
  NsfCommandPreserve(INTERP cmd);
  NsfCommandRelease(INTERP del);
}

#if 0
/** for debug purposes only */
static void
CmdListPrint(Tcl_Interp *interp, CONST char *title, NsfCmdList *cmdList) {
  if (title) {
    fprintf(stderr, "%s %p:\n", title, cmdList);
  }
  while (cmdList) {
    fprintf(stderr, "   CL=%p, cmdPtr=%p %s, clorobj %p, clientData=%p\n",
            cmdList,
            cmdList->cmdPtr,
            interp ? Tcl_GetCommandName(interp, cmdList->cmdPtr) : "",
            cmdList->clorobj,
            cmdList->clientData);
    cmdList = cmdList->nextPtr;
  }
}
#endif

/*
 * physically delete an entry 'del'
 */
static void
CmdListDeleteCmdListEntry(INTERP_DECL NsfCmdList *del, NsfFreeCmdListClientData *freeFct) {
  if (freeFct) {
    (*freeFct)(INTERP del);
  }
  NsfCommandRelease(INTERP del->cmdPtr);
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
CmdListRemoveEpoched(INTERP_DECL NsfCmdList **cmdList, NsfFreeCmdListClientData *freeFct) {
  NsfCmdList *f = *cmdList, *del;
  while (f) {
    if (Tcl_Command_cmdEpoch(f->cmdPtr)) {
      del = f;
      f = f->nextPtr;
      del = CmdListRemoveFromList(cmdList, del);
      CmdListDeleteCmdListEntry(INTERP del, freeFct);
    } else
      f = f->nextPtr;
  }
}


/*
 * delete all cmds with given context class object
 */
static void
CmdListRemoveContextClassFromList(INTERP_DECL NsfCmdList **cmdList, NsfClass *clorobj,
                                  NsfFreeCmdListClientData *freeFct) {
  NsfCmdList *c, *del = NULL;
  /*
    CmdListRemoveEpoched(INTERP cmdList, freeFct);
  */
  c = *cmdList;
  while (c && c->clorobj == clorobj) {
    del = c;
    *cmdList = c->nextPtr;
    CmdListDeleteCmdListEntry(INTERP del, freeFct);
    c = *cmdList;
  }

  while (c) {
    if (c->clorobj == clorobj) {
      del = c;
      c = *cmdList;
      while (c->nextPtr && c->nextPtr != del) {
	c = c->nextPtr;
      }
      if (c->nextPtr == del) {
	c->nextPtr = del->nextPtr;
      }
      CmdListDeleteCmdListEntry(INTERP del, freeFct);
    }
    c = c->nextPtr;
  }
}

/*
 * free the memory of a whole 'cmdList'
 */
static void
CmdListRemoveList(INTERP_DECL NsfCmdList **cmdList, NsfFreeCmdListClientData *freeFct) {
  NsfCmdList *del;
  while (*cmdList) {
    del = *cmdList;
    *cmdList = (*cmdList)->nextPtr;
    CmdListDeleteCmdListEntry(INTERP del, freeFct);
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
    if (h->cmdPtr == cmd) return h;
  }
  return NULL;
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
    if (cmdName[0] == name[0] && !strcmp(cmdName, name)) return h;
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * CheckConditionInScope --
 *
 *    Check a given condition in the current callframe's scope. It is
 *    the responsiblity of the caller to push the intended callframe.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
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
    if (result == TCL_OK && success == 0) {
      result = NSF_CHECK_FAILED;
    }
  }
  return result;
}

#if defined(NSF_WITH_ASSERTIONS)
/*********************************************************************
 * Assertions
 **********************************************************************/
/*
 * Generic List handling functions, just used in assertion handling
 */

static void
TclObjListFreeList(INTERP_DECL NsfTclObjList *list) {
  NsfTclObjList *del;
  while (list) {
    del = list;
    list = list->nextPtr;
    DECR_REF_COUNT2("listContent", del->content);
    FREE(NsfTclObjList, del);
  }
}

static Tcl_Obj *
TclObjListNewElement(INTERP_DECL NsfTclObjList **list, Tcl_Obj *ov) {
  NsfTclObjList *elt = NEW(NsfTclObjList);
  INCR_REF_COUNT2("listContent", ov);
  elt->content = ov;
  elt->nextPtr = *list;
  *list = elt;
  return ov;
}

static NsfTclObjList *
AssertionNewList(Tcl_Interp *interp, Tcl_Obj *aObj) {
  Tcl_Obj **ov; int oc;
  NsfTclObjList *last = NULL;

  if (Tcl_ListObjGetElements(interp, aObj, &oc, &ov) == TCL_OK) {
    if (oc > 0) {
      int i;
      for (i=oc-1; i>=0; i--) {
        TclObjListNewElement(INTERP &last, ov[i]);
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
  Tcl_Obj *resultObj;

  if (!opt) return TCL_OK;

  resultObj = Tcl_GetObjResult(interp);

  if (opt->checkoptions & CHECK_OBJINVAR)
    Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("object-invar", -1));
  if (opt->checkoptions & CHECK_CLINVAR)
    Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("class-invar", -1));
  if (opt->checkoptions & CHECK_PRE)
    Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("pre", -1));
  if (opt->checkoptions & CHECK_POST)
    Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("post", -1));

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
AssertionRemoveProc(INTERP_DECL NsfAssertionStore *aStore, CONST char *name) {
  Tcl_HashEntry *hPtr;
  if (aStore) {
    hPtr = Tcl_CreateHashEntry(&aStore->procs, name, NULL);
    if (hPtr) {
      NsfProcAssertion *procAss =
        (NsfProcAssertion *) Tcl_GetHashValue(hPtr);
      TclObjListFreeList(INTERP procAss->pre);
      TclObjListFreeList(INTERP procAss->post);
      FREE(NsfProcAssertion, procAss);
      Tcl_DeleteHashEntry(hPtr);
    }
  }
}

static void
AssertionAddProc(Tcl_Interp *interp, CONST char *name, NsfAssertionStore *aStore,
                 Tcl_Obj *pre, Tcl_Obj *post) {
  int new = 0;
  Tcl_HashEntry *hPtr = NULL;
  NsfProcAssertion *procs = NEW(NsfProcAssertion);

  AssertionRemoveProc(INTERP aStore, name);
  procs->pre = AssertionNewList(interp, pre);
  procs->post = AssertionNewList(interp, post);
  hPtr = Tcl_CreateHashEntry(&aStore->procs, name, &new);
  if (new) {
    Tcl_SetHashValue(hPtr, procs);
  }
}

static NsfAssertionStore *
AssertionCreateStore(INTERP_DECL1) {
  NsfAssertionStore *aStore = NEW(NsfAssertionStore);
  aStore->invariants = NULL;
  Tcl_InitHashTable(&aStore->procs, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", &aStore->procs);
  return aStore;
}

static void
AssertionRemoveStore(INTERP_DECL NsfAssertionStore *aStore) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  if (aStore) {
    for (hPtr = Tcl_FirstHashEntry(&aStore->procs, &hSrch); hPtr;
         hPtr = Tcl_FirstHashEntry(&aStore->procs, &hSrch)) {
      /*
       * AssertionRemoveProc calls Tcl_DeleteHashEntry(hPtr), thus
       * we get the FirstHashEntry afterwards again to proceed
       */
      AssertionRemoveProc(INTERP aStore, Tcl_GetHashKey(&aStore->procs, hPtr));
    }
    Tcl_DeleteHashTable(&aStore->procs);
    MEM_COUNT_FREE("Tcl_InitHashTable", &aStore->procs);
    TclObjListFreeList(INTERP aStore->invariants);
    FREE(NsfAssertionStore, aStore);
  }
}

static int
AssertionCheckList(Tcl_Interp *interp, NsfObject *object,
                   NsfTclObjList *alist, CONST char *methodName) {
  NsfTclObjList *checkFailed = NULL;
  Tcl_Obj *savedResultObj = Tcl_GetObjResult(interp);
  int savedCheckoptions, acResult = TCL_OK;

  /*
   * no obj->opt -> checkoption == CHECK_NONE
   */
  if (!object->opt) {
    return TCL_OK;
  }
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

  INCR_REF_COUNT(savedResultObj);

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
      if (acResult != TCL_OK) {
        checkFailed = alist;
      }
      object->opt->checkoptions = savedCheckoptions;
      /* fprintf(stderr, "...%s\n", checkFailed ? "failed" : "ok"); */
      Nsf_PopFrameObj(interp, framePtr);
    }
    if (checkFailed) {
      break;
    }
    alist = alist->nextPtr;
  }

  if (checkFailed) {
    DECR_REF_COUNT(savedResultObj);
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

  Tcl_SetObjResult(interp, savedResultObj);
  DECR_REF_COUNT(savedResultObj);
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
    clPtr = ComputeOrder(INTERP object->cl, object->cl->order, Super);
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

  if (cl) {
    aStore = cl->opt ? cl->opt->assertions : NULL;
  } else {
    aStore = object->opt ? object->opt->assertions : NULL;
  }
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
    if (result != TCL_ERROR) {
      result = AssertionCheckInvars(interp, object, method, object->opt->checkoptions);
    }
  }
  return result;
}

static int
AssertionSetCheckOptions(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *arg) {
  NsfObjectOpt *opt = NsfRequireObjectOpt(INTERP object);
  int ocArgs;
  Tcl_Obj **ovArgs;
  opt->checkoptions = CHECK_NONE;

  if (Tcl_ListObjGetElements(interp, arg, &ocArgs, &ovArgs) == TCL_OK
      && ocArgs > 0) {
    int i;
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
  if (*assertions) {
    TclObjListFreeList(INTERP (*assertions)->invariants);
  } else {
    *assertions = AssertionCreateStore(INTERP1);
  }
  (*assertions)->invariants = AssertionNewList(interp, arg);
}
#endif /* NSF_WITH_ASSERTIONS */




/***********************************************************************
 * Mixin support
 ***********************************************************************/

/*
 * push a mixin stack information on this object
 */
static int
MixinStackPush(INTERP_DECL NsfObject *object) {
  register NsfMixinStack *h = NEW(NsfMixinStack);

  h->currentCmdPtr = NULL;
  h->nextPtr = object->mixinStack;
  object->mixinStack = h;
  /*fprintf(stderr, "MixinStackPush %p %s\n", object, ObjectName(object));*/
  return 1;
}

/*
 * pop a mixin stack information on this object
 */
static void
MixinStackPop(INTERP_DECL NsfObject *object) {
  register NsfMixinStack *h = object->mixinStack;
  /*fprintf(stderr, "MixinStackPop %p %s\n", object, ObjectName(object));*/
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

  CmdListRemoveEpoched(INTERP mixinList, GuardDel);

  for (m = *mixinList; m; m = m->nextPtr) {
    NsfClass *mCl = NsfGetClassFromCmdPtr(m->cmdPtr);

    if (mCl) {
      for (pl = ComputeOrder(INTERP mCl, mCl->order, Super); pl; pl = pl->nextPtr) {
        if ((pl->cl->object.flags & NSF_IS_ROOT_CLASS) == 0) {
          NsfClassOpt *opt = pl->cl->opt;

	  /* fprintf(stderr, "find %p %s in checklist 1 %p\n",
	     pl->cl, ClassName(pl->cl), *checkList);*/
	  if (NsfClassListFind(*checkList, pl->cl)) {
	    /*fprintf(stderr, "+++ never add %s\n", ClassName(pl->cl));*/
	  } else {
	    if (opt && opt->classmixins) {
	      /*
	       * Compute transitively the (class) mixin classes of this
	       * added class.
	       */
	      NsfClassListAdd(INTERP checkList, pl->cl, NULL);
	      /*fprintf(stderr, "+++ transitive %s\n", ClassName(pl->cl));*/
	      MixinComputeOrderFullList(interp, &opt->classmixins, mixinClasses,
					checkList, level+1);
	    }
	    /*fprintf(stderr, "+++ add to mixinClasses %p path: %s clPtr %p\n",
	      mixinClasses, ClassName(pl->cl), clPtr);*/
	    clPtr = NsfClassListAddNoDup(INTERP clPtr, pl->cl, m->clientData, NULL);
	  }
        }
      }
    }
  }

  if (level == 0 && *checkList) {
    NsfClassListFree(INTERP *checkList);
    *checkList = NULL;
  }
}

/*
 *----------------------------------------------------------------------
 * MixinResetOrder --
 *
 *    Free the mixin order of the provided object if it exists.
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Frees potentially the mixinOrder list.
 *
 *----------------------------------------------------------------------
 */
static void
MixinResetOrder(INTERP_DECL NsfObject *object) {
  /*fprintf(stderr, "MixinResetOrder for object %s \n", ObjectName(object));*/
  CmdListRemoveList(INTERP &object->mixinOrder, NULL /*GuardDel*/);
  object->mixinOrder = NULL;
}

/*
 *----------------------------------------------------------------------
 * NsfClassListAddPerClassMixins --
 *
 *    Append the class mixins to the proivded list. CheckList is used to
 *    eliminate potential duplicates.
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Appends potentially elements to classList and checkList
 *
 *----------------------------------------------------------------------
 */
static void
NsfClassListAddPerClassMixins(Tcl_Interp *interp, NsfClass *cl,
			      NsfClasses **classList, NsfClasses **checkList) {
  NsfClasses *pl;

  for (pl = ComputeOrder(INTERP cl, cl->order, Super); pl; pl = pl->nextPtr) {
    NsfClassOpt *clopt = pl->cl->opt;
    if (clopt && clopt->classmixins) {
      MixinComputeOrderFullList(interp, &clopt->classmixins,
				classList, checkList, 1);
    }
  }
}

/*
 *----------------------------------------------------------------------
 * MixinComputeOrder --
 *
 *    Compute a duplicate-free linearized order of per-object and per-class
 *    mixins and the class inheritance. The precendence rule is that the last
 *    occurence makes it into the final list.
 *
 * Results:
 *    void
 *
 * Side effects:
 *    object->mixinOrder is updated.
 *
 *----------------------------------------------------------------------
 */
static void
MixinComputeOrder(Tcl_Interp *interp, NsfObject *object) {
  NsfClasses *fullList, *checkList = NULL, *mixinClasses = NULL, *clPtr;

  if (object->mixinOrder)  MixinResetOrder(INTERP object);

  /* Append per-obj mixins */
  if (object->opt) {
    NsfCmdList *m;

    MixinComputeOrderFullList(interp, &object->opt->mixins, &mixinClasses,
                              &checkList, 1);
    /*
     * Add per-object mixins to checkList to avoid that theses classes in the
     * class mixins.
     *
     * TODO: we could add this already in MixinComputeOrderFullList() if we
     * provide an additional flag.
     */
    for (m = object->opt->mixins; m; m = m->nextPtr) {
      NsfClass *mCl = NsfGetClassFromCmdPtr(m->cmdPtr);
      if (mCl) {
	NsfClassListAddNoDup(INTERP &checkList, mCl, NULL, NULL);
      }
    }
  }
  /*fprintf(stderr, "%s ", ObjectName(object));
  NsfClassListPrint("MixinComputeOrder poms", mixinClasses);
  NsfClassListPrint("MixinComputeOrder poms checkList", checkList);*/

  /* append per-class mixins */
  NsfClassListAddPerClassMixins(interp, object->cl, &mixinClasses, &checkList);

  /*fprintf(stderr, "%s ", ObjectName(object));
  NsfClassListPrint("MixinComputeOrder poms+pcms", mixinClasses);
  CmdListPrint(interp, "mixinOrder", object->mixinOrder);*/

  NsfClassListFree(INTERP checkList);
  fullList = mixinClasses;

  /*
   * Don't add duplicates or classes of the precedence order to the resulting
   * list.
   */
  for (clPtr = mixinClasses; clPtr; clPtr = clPtr->nextPtr) {
    NsfClass *cl = clPtr->cl;
    NsfClasses *checker;

    /*fprintf(stderr, "--- Work on %s\n", ClassName(cl));
      CmdListPrint(interp, "mixinOrder", object->mixinOrder);*/

    checker = NsfClassListFind(clPtr->nextPtr, cl);

    /*
     * if checker is set, it is a duplicate and ignored
     */
    if (checker == NULL) {
      /* check object->cl hierachy */
      checker = NsfClassListFind(ComputeOrder(INTERP object->cl, object->cl->order, Super), cl);
      /*
       * if checker is set, it was found in the class hierarchy and it is ignored
       */
    }
    if (checker == NULL) {
      /* add the class to the mixinOrder list */
      NsfCmdList *new;

      /*fprintf(stderr, "--- adding to mixinOrder %s to cmdlist %p of object %s\n",
	ClassName(cl), object->mixinOrder, ObjectName(object));*/
      new = CmdListAdd(INTERP &object->mixinOrder, cl->object.id, NULL, /*noDuplicates*/ 0);
      /*CmdListPrint(interp, "mixinOrder", object->mixinOrder);*/

      /*
       * We require the first matching guard of the full list in the new
       * client data
       */
      checker = NsfClassListFind(fullList, cl);
      if (checker) {
	new->clientData = checker->clientData;
      }
    }

  }

  /* ... and free the memory of the full list */
  NsfClassListFree(INTERP fullList);

  /*CmdListPrint(interp, "mixin order\n", obj->mixinOrder);*/
}


/*
 *----------------------------------------------------------------------
 * MixinAdd --
 *
 *    Add a mixinreg (mixin class with a potential guard) provided as a
 *    Tcl_Obj* to 'mixinList' by appending it to the provided cmdList.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Potentially allocating cmd list elements added to the mixinList
 *
 *----------------------------------------------------------------------
 */
static int
MixinAdd(Tcl_Interp *interp, NsfCmdList **mixinList, Tcl_Obj *nameObj, NsfClass *baseClass) {
  NsfClass *mixinCl;
  Tcl_Obj *guardObj;
  NsfCmdList *new;

  /*fprintf(stderr, "MixinAdd gets obj %p type %p %s\n", nameObj, nameObj->typePtr,
    nameObj->typePtr?nameObj->typePtr->name : "NULL");*/
  /*
   * When the provided nameObj is of type NsfMixinregObjType, the nsf specific
   * converter was called already; otherwise call the converter here.
   */
  if (nameObj->typePtr != &NsfMixinregObjType) {
    if (Tcl_ConvertToType(interp, nameObj, &NsfMixinregObjType) != TCL_OK) {
      return TCL_ERROR;
    }
  }

  NsfMixinregGet(nameObj, &mixinCl, &guardObj);

  new = CmdListAdd(INTERP mixinList, mixinCl->object.id, NULL, /*noDuplicates*/ 1);
  if (guardObj) {
    GuardAdd(INTERP new, guardObj);
  } else if (new->clientData) {
    GuardDel(INTERP new);
  }

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * AppendMatchingElement --
 *
 *    Call AppendElement to the resultObj for values matching the specified
 *    pattern.
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
AppendMatchingElement(Tcl_Interp *interp, Tcl_Obj *resultObj, Tcl_Obj *nameObj, CONST char *pattern) {
  CONST char *string = ObjStr(nameObj);
  if (!pattern || Tcl_StringMatch(string, pattern)) {
    Tcl_ListObjAppendElement(interp, resultObj, nameObj);
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
				  Tcl_Obj *resultObj,
                                  CONST char *pattern, NsfObject *matchObject) {
  int rc = 0;

  for ( ; cmdl; cmdl = cmdl->nextPtr) {
    NsfObject *object = NsfGetObjectFromCmdPtr(cmdl->cmdPtr);
    if (object) {
      if (matchObject == object) {
        return 1;
      } else {
        AppendMatchingElement(interp, resultObj, object->cmdName, pattern);
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
  Tcl_Obj *resultObj = Tcl_GetObjResult(interp);

  for ( ; cls; cls = cls->nextPtr) {
    NsfObject *object = (NsfObject *)cls->cl;
    if (object) {
      if (matchObject && object == matchObject) {
        /*
	 * We have a matchObject and it is identical to obj,
         * just return true and don't continue search
	 */
        return 1;
        break;
      } else {
        AppendMatchingElement(interp, resultObj, object->cmdName, pattern);
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
	     ClassName(startCl));*/

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
AddToResultSet(Tcl_Interp *interp, Tcl_HashTable *destTablePtr,
	       Tcl_Obj *resultSet, NsfObject *object, int *new,
	       int appendResult, CONST char *pattern, NsfObject *matchObject) {
  Tcl_CreateHashEntry(destTablePtr, (char *)object, new);
  if (*new) {
    if (matchObject && matchObject == object) {
      return 1;
    }
    if (appendResult) {
      AppendMatchingElement(interp, resultSet, object->cmdName, pattern);
    }
  }
  return 0;
}

/*
 *----------------------------------------------------------------------
 * AddToResultSetWithGuards --
 *
 *    Helper function to add classes with guards to the result set
 *    (implemented as a hash table, full version as a Tcl list), flagging test
 *    for matchObject as result.
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
AddToResultSetWithGuards(Tcl_Interp *interp, Tcl_HashTable *destTablePtr,
			 Tcl_Obj *resultSet, NsfClass *cl,
			 ClientData clientData, int *new, int appendResult,
			 CONST char *pattern, NsfObject *matchObject) {
  Tcl_CreateHashEntry(destTablePtr, (char *)cl, new);
  if (*new) {
    if (appendResult) {
      if (!pattern || Tcl_StringMatch(ClassName(cl), pattern)) {
        Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
        Tcl_Obj *g = (Tcl_Obj *) clientData;
	INCR_REF_COUNT(listObj);
        Tcl_ListObjAppendElement(interp, listObj, cl->object.cmdName);
        Tcl_ListObjAppendElement(interp, listObj, NsfGlobalObjs[NSF_GUARD_OPTION]);
        Tcl_ListObjAppendElement(interp, listObj, g);
	Tcl_ListObjAppendElement(interp, resultSet, listObj);
	DECR_REF_COUNT(listObj);
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
GetAllObjectMixinsOf(Tcl_Interp *interp, Tcl_HashTable *destTablePtr,
		     Tcl_Obj *resultSet, NsfClass *startCl, int isMixin,
		     int appendResult, CONST char *pattern, NsfObject *matchObject) {
  int rc = 0, new = 0;
  NsfClasses *sc;

  /*fprintf(stderr, "startCl = %s, opt %p, isMixin %d, pattern '%s', matchObject %p\n",
    ClassName(startCl), startCl->opt, isMixin, pattern, matchObject);*/

  /*
   * check all subclasses of startCl for mixins
   */
  for (sc = startCl->sub; sc; sc = sc->nextPtr) {
    rc = GetAllObjectMixinsOf(interp, destTablePtr, resultSet,
			      sc->cl, isMixin, appendResult,
			      pattern, matchObject);
    if (rc) {return rc;}
  }
  /*fprintf(stderr, "check subclasses of %s done\n", ClassName(startCl));*/

  if (startCl->opt) {
    NsfCmdList *m;
    NsfClass *cl;
    for (m = startCl->opt->isClassMixinOf; m; m = m->nextPtr) {

      /* we should have no deleted commands in the list */
      assert(Tcl_Command_cmdEpoch(m->cmdPtr) == 0);

      cl = NsfGetClassFromCmdPtr(m->cmdPtr);
      assert(cl);
      /*fprintf(stderr, "check %s mixinof %s\n", ClassName(cl), ClassName((startCl)));*/
      rc = GetAllObjectMixinsOf(interp, destTablePtr, resultSet,
				cl, isMixin, appendResult,
				pattern, matchObject);
      /* fprintf(stderr, "check %s mixinof %s done\n",
	 ClassName(cl), ClassName(startCl));*/
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

      rc = AddToResultSet(interp, destTablePtr, resultSet,
			  object, &new, appendResult,
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
		    Tcl_Obj *resultSet, /*@notnull@*/ NsfClass *startCl,
		    int isMixin, int appendResult,
		    CONST char *pattern, NsfObject *matchObject) {
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
    rc = AddToResultSet(interp, destTablePtr, resultSet,
			&startCl->object, &new,
			appendResult, pattern, matchObject);
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
      rc = GetAllClassMixinsOf(interp, destTablePtr, resultSet,
			       sc->cl, isMixin,
			       appendResult, pattern, matchObject);
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

      rc = AddToResultSet(interp, destTablePtr, resultSet,
			  &cl->object, &new,
			  appendResult, pattern, matchObject);
      if (rc == 1) {return rc;}
      if (new) {
        /*fprintf(stderr, "... new\n");*/
        rc = GetAllClassMixinsOf(interp, destTablePtr, resultSet,
				 cl, 1,
				 appendResult, pattern, matchObject);
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
GetAllClassMixins(Tcl_Interp *interp, Tcl_HashTable *destTablePtr,
		  Tcl_Obj *resultObj, NsfClass *startCl,
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
        rc = AddToResultSetWithGuards(interp, destTablePtr, resultObj,
				      cl, m->clientData, &new,
				      1, pattern, matchObject);
      } else {
        /* fprintf(stderr, "AddToResultSet: %s\n", ClassName(cl)); */
	rc = AddToResultSet(interp, destTablePtr, resultObj,
			    &cl->object, &new,
			    1, pattern, matchObject);
      }
      if (rc == 1) {return rc;}

      if (new) {
        /* fprintf(stderr, "class mixin GetAllClassMixins for: %s (%s)\n",
	   ClassName(cl), ClassName(startCl)); */
        rc = GetAllClassMixins(interp, destTablePtr, resultObj,
			       cl, withGuards,
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
       ClassName(sc->cl), ClassName(startCl)); */
    rc = GetAllClassMixins(interp, destTablePtr, resultObj,
			   sc->cl, withGuards,
			   pattern, matchObject);
    if (rc) {return rc;}
  }
  return rc;
}

/*
 *----------------------------------------------------------------------
 * RemoveFromClassMixinsOf --
 *
 *    Remove the class (provided as a cmd) from all isClassMixinOf definitions
 *    from the provided classes (provided as cmdlist).
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Deletes potentially some entries in the isClassMixinOf lists.
 *
 *----------------------------------------------------------------------
 */

static void
RemoveFromClassMixinsOf(INTERP_DECL Tcl_Command cmd, NsfCmdList *cmdlist) {

  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    NsfClass *ncl = NsfGetClassFromCmdPtr(cmdlist->cmdPtr);
    NsfClassOpt *nclopt = ncl ? ncl->opt : NULL;
    if (nclopt) {
      NsfCmdList *del = CmdListFindCmdInList(cmd, nclopt->isClassMixinOf);
      if (del) {
        /* fprintf(stderr, "Removing class %s from isClassMixinOf of class %s\n",
           ClassName(cl), ObjStr(NsfGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
        del = CmdListRemoveFromList(&nclopt->isClassMixinOf, del);
        CmdListDeleteCmdListEntry(INTERP del, GuardDel);
      }
    }
  }
}

/*
 *----------------------------------------------------------------------
 * RemoveFromObjectMixinsOf --
 *
 *    Remove the class (provided as a cmd) from all isObjectMixinOf definitions
 *    from the provided classes (provided as cmdlist).
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Deletes potentially some entries in the isObjectMixinOf lists.
 *
 *----------------------------------------------------------------------
 */

static void
RemoveFromObjectMixinsOf(INTERP_DECL Tcl_Command cmd, NsfCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    NsfClass *cl = NsfGetClassFromCmdPtr(cmdlist->cmdPtr);
    NsfClassOpt *clopt = cl ? cl->opt : NULL;
    if (clopt) {
      NsfCmdList *del = CmdListFindCmdInList(cmd, clopt->isObjectMixinOf);
      if (del) {
        /* fprintf(stderr, "Removing object %s from isObjectMixinOf of Class %s\n",
           ObjectName(object), ObjStr(NsfGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
        del = CmdListRemoveFromList(&clopt->isObjectMixinOf, del);
        CmdListDeleteCmdListEntry(INTERP del, GuardDel);
      }
    } /* else fprintf(stderr, "CleanupDestroyObject %s: NULL pointer in mixins!\n", ObjectName(object)); */
  }
}

/*
 *----------------------------------------------------------------------
 * RemoveFromClassmixins --
 *
 *    Remove the class (provided as a cmd) from all classmixins lists
 *    from the provided classes (provided as cmdlist).
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Deletes potentially some entries in the classmixins lists.
 *
 *----------------------------------------------------------------------
 */

static void
RemoveFromClassmixins(INTERP_DECL Tcl_Command cmd, NsfCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    NsfClass *cl = NsfGetClassFromCmdPtr(cmdlist->cmdPtr);
    NsfClassOpt *clopt = cl ? cl->opt : NULL;
    if (clopt) {
      NsfCmdList *del = CmdListFindCmdInList(cmd, clopt->classmixins);
      if (del) {
        /* fprintf(stderr, "Removing class %s from mixins of object %s\n",
           ClassName(cl), ObjStr(NsfGetObjectFromCmdPtr(cmdlist->cmdPtr)->cmdName)); */
        del = CmdListRemoveFromList(&clopt->classmixins, del);
        CmdListDeleteCmdListEntry(INTERP del, GuardDel);
	if (cl->object.mixinOrder) MixinResetOrder(INTERP &cl->object);
      }
    }
  }
}

/*
 *----------------------------------------------------------------------
 * RemoveFromObjectMixins --
 *
 *    Remove the class (provided as a cmd) from all object mixin lists
 *    from the provided classes (provided as cmdlist).
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Deletes potentially some entries in the objectmixins lists.
 *
 *----------------------------------------------------------------------
 */
static void
RemoveFromObjectMixins(INTERP_DECL Tcl_Command cmd, NsfCmdList *cmdlist) {
  for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
    NsfObject *nobj = NsfGetObjectFromCmdPtr(cmdlist->cmdPtr);
    NsfObjectOpt *objopt = nobj ? nobj->opt : NULL;
    if (objopt) {
      NsfCmdList *del = CmdListFindCmdInList(cmd, objopt->mixins);
      if (del) {
        /* fprintf(stderr, "Removing class %s from mixins of object %s\n",
           ClassName(cl), ObjStr(NsfGetObjectFromCmdPtr(cmdlist->cmdPtr)->cmdName)); */
        del = CmdListRemoveFromList(&objopt->mixins, del);
        CmdListDeleteCmdListEntry(INTERP del, GuardDel);
	if (nobj->mixinOrder) MixinResetOrder(INTERP nobj);
      }
    }
  }
}


/*
 *----------------------------------------------------------------------
 * MixinResetOrderForInstances --
 *
 *    Reset the per-object mixin order for the instances of the provided
 *    class.
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Deletes potentially the mixin list for the objects.
 *
 *----------------------------------------------------------------------
 */

static void
MixinResetOrderForInstances(INTERP_DECL NsfClass *cl) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  /*fprintf(stderr, "invalidating instances of class %s\n", ClassName(clPtr->cl));*/

  /* Here we should check, whether this class is used as an object or
     class mixin somewhere else and invalidate the objects of these as
     well -- */

  for (hPtr = Tcl_FirstHashEntry(&cl->instances, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    NsfObject *object = (NsfObject *)Tcl_GetHashKey(&cl->instances, hPtr);
    if (object
        && !(object->flags & NSF_DURING_DELETE)
        && (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID)) {
      MixinResetOrder(INTERP object);
      object->flags &= ~NSF_MIXIN_ORDER_VALID;
    }
  }
}

/*
 *----------------------------------------------------------------------
 * ResetOrderOfClassesUsedAsMixins --
 *
 *    Reset the per-object mixin order for all objects having this class as
 *    per-object mixin.
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Deletes potentially the mixin list for the objects.
 *
 *----------------------------------------------------------------------
 */
static void
ResetOrderOfClassesUsedAsMixins(INTERP_DECL NsfClass *cl) {
  /*fprintf(stderr, "ResetOrderOfClassesUsedAsMixins %s - %p\n",
    ClassName(cl), cl->opt);*/

  if (cl->opt) {
    NsfCmdList *ml;
    for (ml = cl->opt->isObjectMixinOf; ml; ml = ml->nextPtr) {
      NsfObject *object = NsfGetObjectFromCmdPtr(ml->cmdPtr);
      if (object) {
        if (object->mixinOrder) { MixinResetOrder(INTERP object); }
        object->flags &= ~NSF_MIXIN_ORDER_VALID;
      }
    }
  }
}

/*
 *----------------------------------------------------------------------
 * MixinInvalidateObjOrders --
 *
 *    Reset mixin order for all instances of the class and the instances of
 *    its subclasses. This function is typically callled, when the the class
 *    hierarchy or the class mixins have changed and invalidate mixin entries
 *    in all dependent instances.
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Deletes potentially the mixin list for the objects and classes.
 *
 *----------------------------------------------------------------------
 */

static void
MixinInvalidateObjOrders(Tcl_Interp *interp, NsfClass *cl) {
  NsfClasses *saved = cl->order, *clPtr;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  Tcl_HashTable objTable, *commandTable = &objTable, *instanceTablePtr;

  cl->order = NULL;

  /*
   * Iterate over the subclass hierarchy.
   */
  for (clPtr = ComputeOrder(INTERP cl, cl->order, Sub); clPtr; clPtr = clPtr->nextPtr) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr;

    /*
     * Reset mixin order for all objects having this class as per object mixin
     */
    ResetOrderOfClassesUsedAsMixins(INTERP clPtr->cl);

    /* fprintf(stderr, "invalidating instances of class %s\n", ClassName(clPtr));
     */
    instanceTablePtr = &clPtr->cl->instances;
    for (hPtr = Tcl_FirstHashEntry(instanceTablePtr, &hSrch); hPtr;
	 hPtr = Tcl_NextHashEntry(&hSrch)) {
      NsfObject *object = (NsfObject *)Tcl_GetHashKey(instanceTablePtr, hPtr);
      if (object->mixinOrder) { MixinResetOrder(INTERP object); }
      object->flags &= ~NSF_MIXIN_ORDER_VALID;
    }
  }

  NsfClassListFree(INTERP cl->order);
  cl->order = saved;

  /*
   * Reset the mixin order for all objects having this class as a per class
   * mixin.  This means that we have to work through the class mixin hierarchy
   * with its corresponding instances.
   */
  Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
  GetAllClassMixinsOf(interp, commandTable, Tcl_GetObjResult(interp),
		      cl, 1, 0, NULL, NULL);

  for (hPtr = Tcl_FirstHashEntry(commandTable, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    NsfClass *ncl = (NsfClass *)Tcl_GetHashKey(commandTable, hPtr);
    /*fprintf(stderr, "Got %s, reset for ncl %p\n", ncl?ClassName(ncl):"NULL", ncl);*/
    if (ncl) {
      MixinResetOrderForInstances(INTERP ncl);
      /*
       * This place seems to be sufficient to invalidate the computed object
       * parameter definitions.
       */
      /*fprintf(stderr, "MixinInvalidateObjOrders via class mixin %s calls ifd invalidate \n", ClassName(ncl));*/
      NsfInvalidateObjectParameterCmd(interp, ncl);
    }
  }
  Tcl_DeleteHashTable(commandTable);
  MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
}

/*
 *----------------------------------------------------------------------
 * MixinComputeDefined --
 *
 *    This function computes the mixin order for the provided object and
 *    adjusts the mixin flags accordingly. The mixin order is either
 *
 *       DEFINED (there are mixins on the instance),
 *       NONE    (there are no mixins for the instance),
 *       or INVALID (a class re-strucuturing has occured,
 *               thus it is not clear whether mixins are defined or not).
 *
 *    If the mixin order is INVALID, MixinComputeDefined can be used to
 *    compute the order and set the instance to DEFINED or NONE
 *
 * Results:
 *    void
 *
 * Side effects:
 *    Might alter the mixin order.
 *
 *----------------------------------------------------------------------
 */
static void
MixinComputeDefined(Tcl_Interp *interp, NsfObject *object) {
  MixinComputeOrder(interp, object);
  object->flags |= NSF_MIXIN_ORDER_VALID;
  if (object->mixinOrder) {
    object->flags |= NSF_MIXIN_ORDER_DEFINED;
  } else {
    object->flags &= ~NSF_MIXIN_ORDER_DEFINED;
  }
}

/*
 *----------------------------------------------------------------------
 * ComputePrecedenceList --
 *
 *    Returns the precedence list for the provided object.  The precedence
 *    list can optionally include the mixins and the root class. If pattern is
 *    provided, this is used as well for filtering. The caller has to free the
 *    resulting list via NsfClassListFree();
 *
 * Results:
 *    Precendence list inf form of a class list.
 *
 * Side effects:
 *    Allocated class list.
 *
 *----------------------------------------------------------------------
 */

static NsfClasses *
ComputePrecedenceList(Tcl_Interp *interp, NsfObject *object,
		      CONST char *pattern,
		      int withMixins, int withRootClass) {
  NsfClasses *precedenceList = NULL, *pcl, **npl = &precedenceList;

  if (withMixins) {
    if (!(object->flags & NSF_MIXIN_ORDER_VALID)) {
      MixinComputeDefined(interp, object);
    }
    if (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) {
      NsfCmdList *ml = object->mixinOrder;

      while (ml) {
	NsfClass *mixin = NsfGetClassFromCmdPtr(ml->cmdPtr);
	if (pattern) {
	  if (!Tcl_StringMatch(ClassName(mixin), pattern)) continue;
	}
	npl = NsfClassListAdd(INTERP npl, mixin, NULL);
	ml = ml->nextPtr;
      }
    }
  }

  pcl = ComputeOrder(INTERP object->cl, object->cl->order, Super);
  for (; pcl; pcl = pcl->nextPtr) {
    if (withRootClass == 0 && pcl->cl->object.flags & NSF_IS_ROOT_CLASS) {
      continue;
    }
    if (pattern && !Tcl_StringMatch(ClassName(pcl->cl), pattern)) {
      continue;
    }
    npl = NsfClassListAdd(INTERP npl, pcl->cl, NULL);
  }
  return precedenceList;
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
  RUNTIME_STATE(interp)->currentMixinCmdPtr = cmdList ? cmdList->cmdPtr : NULL;

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
	result = GuardCall(object, interp, (Tcl_Obj *)cmdList->clientData, NULL);
      }
    }
    if (result == TCL_OK) {
      /*
       * on success: compute mixin call data
       */
      *clPtr = cl;
      *currentCmdPtr = cmdList->cmdPtr;
      /*fprintf(stderr, "mixinsearch returns %p (cl %s)\n", cmd, ClassName(cl));*/
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
        Tcl_Obj *g = (Tcl_Obj *) m->clientData;
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
MixinSearchMethodByName(INTERP_DECL NsfCmdList *mixinList, CONST char *name, NsfClass **cl) {
  Tcl_Command cmd;

  for (; mixinList;  mixinList = mixinList->nextPtr) {
    NsfClass *foundCl = NsfGetClassFromCmdPtr(mixinList->cmdPtr);
    if (foundCl && SearchCMethod(INTERP foundCl, name, &cmd)) {
      if (cl) *cl = foundCl;
      return cmd;
    }
  }
  return NULL;
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
FilterSearch(INTERP_DECL CONST char *name, NsfObject *startingObject,
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
      if ((cmd = MixinSearchMethodByName(INTERP opt->mixins, name, cl))) {
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
      if ((cmd = MixinSearchMethodByName(INTERP opt->classmixins, name, cl))) {
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
      *cl = (NsfClass *)startingObject;
      return cmd;
    }
  }

  /*
   * ok, no filter on obj or mixins -> search class
   */
  if (startingClass) {
    *cl = SearchCMethod(INTERP startingClass, name, &cmd);
    if (!*cl) {
      /*
       * If no filter is found yet -> search the meta-class
       */
      *cl = SearchCMethod(INTERP startingClass->object.cl, name, &cmd);
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
  NsfRuntimeState *rst = RUNTIME_STATE(interp);

  if (guardObj) {
    int result;
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
  Tcl_Obj *guardObj = (TclObj *) clientData;
  fprintf(stderr, " +++ <GUARDS> \n");
  if (guardObj) {
  fprintf(stderr, "   *     %s \n", ObjStr(guardObj));
  }
  fprintf(stderr, " +++ </GUARDS>\n");
  }
*/

static void
GuardDel(INTERP_DECL NsfCmdList *CL) {
  /*fprintf(stderr, "GuardDel %p clientData = %p\n",
    CL, CL? CL->clientData : NULL);*/
  if (CL && CL->clientData) {
    DECR_REF_COUNT2("guardObj", (Tcl_Obj *)CL->clientData);
    CL->clientData = NULL;
  }
}

NSF_INLINE static void
GuardAdd(INTERP_DECL NsfCmdList *CL, Tcl_Obj *guardObj) {
  if (guardObj) {
    GuardDel(INTERP CL);
    if (strlen(ObjStr(guardObj)) != 0) {
      INCR_REF_COUNT2("guardObj", guardObj);
      CL->clientData = guardObj;
      /*fprintf(stderr, "guard added to %p cmdPtr=%p, clientData= %p\n",
        CL, CL->cmdPtr, CL->clientData);
      */
    }
  }
}

static int
GuardCall(NsfObject *object, Tcl_Interp *interp, Tcl_Obj *guardObj, NsfCallStackContent *cscPtr) {
  int result = TCL_OK;

  if (guardObj) {
    Tcl_Obj *res = Tcl_GetObjResult(interp); /* save the result */
    CallFrame frame, *framePtr = &frame;

    INCR_REF_COUNT(res);

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
GuardAddFromDefinitionList(INTERP_DECL NsfCmdList *dest, Tcl_Command interceptorCmd,
                           NsfCmdList *interceptorDefList) {
  NsfCmdList *h;
  if (interceptorDefList) {
    h = CmdListFindCmdInList(interceptorCmd, interceptorDefList);
    if (h) {
      GuardAdd(INTERP dest, (Tcl_Obj *) h->clientData);
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
  if (!(object->flags & NSF_MIXIN_ORDER_VALID)) {
    MixinComputeDefined(interp, object);
  }
  if (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) {
    NsfCmdList *ml;
    NsfClass *mixin;
    for (ml = object->mixinOrder; ml && !guardAdded; ml = ml->nextPtr) {
      mixin = NsfGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin && mixin->opt) {
        guardAdded = GuardAddFromDefinitionList(INTERP dest, filterCmd,
                                                mixin->opt->classfilters);
      }
    }
  }

  /* search per-object filters */
  opt = object->opt;
  if (!guardAdded && opt && opt->filters) {
    guardAdded = GuardAddFromDefinitionList(INTERP dest, filterCmd, 
					    opt->filters);
  }

  if (!guardAdded) {
    /* search per-class filters */
    for (pl = ComputeOrder(INTERP object->cl, object->cl->order, Super); !guardAdded && pl; pl = pl->nextPtr) {
      NsfClassOpt *clopt = pl->cl->opt;
      if (clopt) {
        guardAdded = GuardAddFromDefinitionList(INTERP dest, filterCmd,
                                                clopt->classfilters);
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
        GuardAdd(INTERP dest, (Tcl_Obj *) registeredFilter->clientData);
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
        Tcl_Obj *g = (Tcl_Obj *) h->clientData;
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
FilterAdd(Tcl_Interp *interp, NsfCmdList **filterList, Tcl_Obj *filterregObj,
          NsfObject *startingObject, NsfClass *startingClass) {
  Tcl_Obj *filterObj = NULL;
  Tcl_Obj *guardObj = NULL;
  Tcl_Command cmd;
  NsfCmdList *new;
  NsfClass *cl;

  /*
   * When the provided nameObj is of type NsfFilterregObjType, the nsf specific
   * converter was called already; otherwise call the converter here.
   */
  if (filterregObj->typePtr != &NsfFilterregObjType) {
    /*fprintf(stderr, "FilterAdd: convert %s in FilterAdd\n", ObjStr(filterregObj));*/
    if (Tcl_ConvertToType(interp, filterregObj, &NsfFilterregObjType) != TCL_OK) {
      return TCL_ERROR;
    }
  } else {
    /*fprintf(stderr, "FilterAdd: %s already converted\n", ObjStr(filterregObj));*/
  }

  NsfFilterregGet(filterregObj, &filterObj, &guardObj);

  if (!(cmd = FilterSearch(INTERP ObjStr(filterObj), startingObject, startingClass, &cl))) {
    if (startingObject) {
      return NsfPrintError(interp, "object filter: can't find filterproc '%s' on %s ",
			   ObjStr(filterObj), ObjectName(startingObject));
    } else {
      return NsfPrintError(interp, "class filter: can't find filterproc '%s' on %s ",
			   ObjStr(filterObj), ClassName(startingClass));
    }
  }

  /*fprintf(stderr, " +++ adding filter %s cl %p\n", ObjStr(nameObj), cl);*/

  new = CmdListAdd(INTERP filterList, cmd, cl, /*noDuplicates*/ 1);

  if (guardObj) {
    GuardAdd(INTERP new, guardObj);
  } else if (new->clientData) {
    GuardDel(INTERP new);
  }

  return TCL_OK;
}

/*
 * reset the filter order cached in obj->filterOrder
 */
static void
FilterResetOrder(INTERP_DECL NsfObject *object) {
  CmdListRemoveList(INTERP &object->filterOrder, GuardDel);
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

  CmdListRemoveEpoched(INTERP filters, GuardDel);
  for (cmdList = *filters; cmdList; ) {
    simpleName = (char *) Tcl_GetCommandName(interp, cmdList->cmdPtr);
    cmd = FilterSearch(INTERP simpleName, startingObject, startingClass, &cl);
    if (cmd == NULL) {
      del = CmdListRemoveFromList(filters, cmdList);
      cmdList = cmdList->nextPtr;
      CmdListDeleteCmdListEntry(INTERP del, GuardDel);
    } else if (cmd != cmdList->cmdPtr) {
      CmdListReplaceCmd(INTERP cmdList, cmd, cl);
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
  savePtr = clPtr = ComputeOrder(INTERP cl, cl->order, Sub);
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
      FilterResetOrder(INTERP object);
      object->flags &= ~NSF_FILTER_ORDER_VALID;

      /* recalculate the commands of all object filter registrations */
      if (object->opt) {
        FilterSearchAgain(interp, &object->opt->filters, object, 0);
      }
    }
  }
  NsfClassListFree(INTERP savePtr);
}

/*
 * from cl on down the hierarchy we remove all filters
 * the refer to "removeClass" namespace. E.g. used to
 * remove filters defined in superclass list from dependent
 * class cl
 */
static void
FilterRemoveDependentFilterCmds(INTERP_DECL NsfClass *cl, NsfClass *removeClass) {
  NsfClasses *saved = cl->order, *clPtr;
  cl->order = NULL;

  /*fprintf(stderr, "FilterRemoveDependentFilterCmds cl %p %s, removeClass %p %s\n",
    cl, ClassName(cl),
    removeClass, ObjStr(removeClass->object.cmdName));*/

  for (clPtr = ComputeOrder(INTERP cl, cl->order, Sub); clPtr; clPtr = clPtr->nextPtr) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
      Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : NULL;
    NsfClassOpt *opt = clPtr->cl->opt;
    if (opt) {
      CmdListRemoveContextClassFromList(INTERP &opt->classfilters, removeClass, GuardDel);
    }
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      NsfObject *object = (NsfObject *) Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
      if (object->opt) {
        CmdListRemoveContextClassFromList(INTERP &object->opt->filters, removeClass, GuardDel);
      }
    }
  }

  NsfClassListFree(INTERP cl->order);
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
        Tcl_Obj *g = (Tcl_Obj *) f->clientData;
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
  CmdListRemoveEpoched(INTERP filters, GuardDel);

  for (f = *filters; f; f = f->nextPtr) {
    simpleName = (char *) Tcl_GetCommandName(interp, f->cmdPtr);
    fcl = f->clorobj;
    CmdListAdd(INTERP filterList, f->cmdPtr, fcl, /*noDuplicates*/ 0);

    if (fcl && !NsfObjectIsClass(&fcl->object)) {
      /* get the class from the object for per-object filter */
      fcl = ((NsfObject *)fcl)->cl;
    }

    /* if we have a filter class -> search up the inheritance hierarchy*/
    if (fcl) {
      pl = ComputeOrder(INTERP fcl, fcl->order, Super);
      if (pl && pl->nextPtr) {
        /* don't search on the start class again */
        pl = pl->nextPtr;
        /* now go up the hierarchy */
        for(; pl; pl = pl->nextPtr) {
          Tcl_Command pi = FindMethod(pl->cl->nsPtr, simpleName);
          if (pi) {
            CmdListAdd(INTERP filterList, pi, pl->cl, /*noDuplicates*/ 0);
            /*
              fprintf(stderr, " %s::%s, ", ClassName(pl->cl), simpleName);
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

  if (object->filterOrder) FilterResetOrder(INTERP object);
  /*
    fprintf(stderr, "<Filter Order obj=%s> List: ", ObjectName(object));
  */

  /* append classfilters registered for mixins */
  if (!(object->flags & NSF_MIXIN_ORDER_VALID)) {
    MixinComputeDefined(interp, object);
  }
  if (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) {
    NsfCmdList *ml;
    NsfClass *mixin;

    for (ml = object->mixinOrder; ml; ml = ml->nextPtr) {
      mixin = NsfGetClassFromCmdPtr(ml->cmdPtr);
      if (mixin && mixin->opt && mixin->opt->classfilters) {
        FilterComputeOrderFullList(interp, &mixin->opt->classfilters, &filterList);
      }
    }
  }

  /* append per-obj filters */
  if (object->opt) {
    FilterComputeOrderFullList(interp, &object->opt->filters, &filterList);
  }
  /* append per-class filters */
  for (pl = ComputeOrder(INTERP object->cl, object->cl->order, Super); pl; pl = pl->nextPtr) {
    NsfClassOpt *clopt = pl->cl->opt;
    if (clopt && clopt->classfilters) {
      FilterComputeOrderFullList(interp, &clopt->classfilters, &filterList);
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
      newlist = CmdListAdd(INTERP &object->filterOrder, filterList->cmdPtr, filterList->clorobj,
                           /*noDuplicates*/ 0);
      GuardAddInheritedGuards(interp, newlist, object, filterList->cmdPtr);
      /*
        fprintf(stderr, "  Adding %s::%s,\n", filterList->cmdPtr->nsPtr->fullName, Tcl_GetCommandName(interp, filterList->cmdPtr));
      */
      /*
        GuardPrint(interp, newlist->clientData);
      */

    }

    CmdListDeleteCmdListEntry(INTERP filterList, GuardDel);

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
  if (object->filterOrder) {
    object->flags |= NSF_FILTER_ORDER_DEFINED;
  } else {
    object->flags &= ~NSF_FILTER_ORDER_DEFINED;
  }
}

/*
 * push a filter stack information on this object
 */
static int
FilterStackPush(INTERP_DECL NsfObject *object, Tcl_Obj *calledProc) {
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
FilterStackPop(INTERP_DECL NsfObject *object) {
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
  for (pl = ComputeOrder(INTERP object->cl, object->cl->order, Super); pl; pl = pl->nextPtr) {
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
  NsfObjectSystem *osPtr;
  NsfClass **scl;
  int reversed = 0;
  int i, j;

  filterCheck = ComputeOrder(INTERP cl, cl->order, Super);
  /*
   * we have to remove all dependent superclass filter referenced
   * by class or one of its subclasses
   *
   * do not check the class "cl" itself (first entry in
   * filterCheck class list)
   */
  if (filterCheck) {
    filterCheck = filterCheck->nextPtr;
  }
  for (; filterCheck; filterCheck = filterCheck->nextPtr) {
    FilterRemoveDependentFilterCmds(INTERP cl, filterCheck->cl);
  }

  /* invalidate all interceptors orders of instances of this
     and of all depended classes */
  MixinInvalidateObjOrders(interp, cl);
  FilterInvalidateObjOrders(interp, cl);

  scl = NEW_ARRAY(NsfClass*, oc);
  for (i = 0; i < oc; i++) {
    if (GetClassFromObj(interp, ov[i], &scl[i], 1) != TCL_OK) {
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
      NsfClasses *dl = ComputeOrder(INTERP scl[j], scl[j]->order, Super);
      if (reversed) break;
      dl = NsfClassListFind(dl, scl[i]);
      if (dl) reversed = 1;
    }
  }

  if (reversed) {
    return NsfObjErrType(interp, "superclass", arg, "classes in dependence order", NULL);
  }

  /*
   * Ensure that the current class and new superclasses are from the
   * same object system.
   */
  osPtr = GetObjectSystem(&cl->object);
  for (i = 0; i < oc; i++) {
    if (osPtr != GetObjectSystem(&scl[i]->object)) {
      return NsfPrintError(interp, "class \"%s\" has different object system as class  \"%s\"",
			   ClassName(cl), ClassName(scl[i]));
    }
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
    (void)RemoveSuper(INTERP cl, cl->super->cl);
  }
  for (i=0; i < oc; i++) {
    AddSuper(INTERP cl, scl[i]);
  }
  FREE(NsfClass**, scl);
  FlushPrecedencesOnSubclasses(INTERP cl);

  if (!ComputeOrder(INTERP cl, cl->order, Super)) {
    NsfClasses *l;
    /*
     * cycle in the superclass graph, backtrack
     */
    while (cl->super) {
      (void)RemoveSuper(INTERP cl, cl->super->cl);
    }
    for (l = osl; l; l = l->nextPtr) {
      AddSuper(INTERP cl, l->cl);
    }
    NsfClassListFree(INTERP osl);
    return NsfObjErrType(interp, "superclass", arg, "a cycle-free graph", NULL);
  }
  NsfClassListFree(INTERP osl);

  /* if there are no more super classes add the Object
     class as superclasses */
  assert(cl->super);
#if 0
  if (cl->super == NULL) {
    fprintf(stderr, "SuperClassAdd super of '%s' is NULL\n", ClassName(cl));
    /*AddSuper(INTERP cl, RUNTIME_STATE(interp)->theObject);*/
  }
#endif

  Tcl_ResetResult(interp);
  return TCL_OK;
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
          int flags) {
  CallFrame frame, *framePtr = &frame;
  Var *varPtr, *arrayPtr;
  int result;

  Nsf_PushFrameObj(interp, object, framePtr);

  if (flags & NSF_VAR_TRIGGER_TRACE) {
    varPtr = TclVarTraceExists(interp, varName);
  } else {
    int flags = (index == NULL) ? TCL_PARSE_PART1 : 0;
    varPtr = TclLookupVar(interp, varName, index, flags, "access",
                          /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
  }
  /*
    fprintf(stderr, "VarExists %s varPtr %p flags %.4x isundef %d\n",
    varName,
    varPtr,
    flags,
    varPtr ? TclIsVarUndefined(varPtr) : NULL);
  */
  result = (varPtr && ((flags & NSF_VAR_REQUIRE_DEFINED) == 0 || !TclIsVarUndefined(varPtr)));
  if (result && (flags & NSF_VAR_ISARRAY) && !TclIsVarArray(varPtr)) {
    result = 0;
  }
  Nsf_PopFrameObj(interp, framePtr);

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
ByteCompiled(Tcl_Interp *interp, unsigned short *flagsPtr,
	     Proc *procPtr, CONST char *procName) {
  Namespace *nsPtr = procPtr->cmdPtr->nsPtr;
  Tcl_Obj *bodyObj = procPtr->bodyPtr;

  if (likely(bodyObj->typePtr == Nsf_OT_byteCodeType)) {
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

    codePtr = bodyObj->internalRep.otherValuePtr;
    if (((Interp *) *codePtr->interpHandle != iPtr)
	|| (codePtr->compileEpoch != iPtr->compileEpoch)
	|| (codePtr->nsPtr != nsPtr)
	|| (codePtr->nsEpoch != nsPtr->resolverEpoch)) {

#if defined(VAR_RESOLVER_TRACE)
      fprintf(stderr, "ByteCompiled bytecode not valid proc %p cmd %p method %s\n",
	      procPtr, procPtr->cmdPtr, Tcl_GetCommandName(interp, procPtr->cmdPtr));
      fprintf(stderr, "    %d %d %d %d\n",
	      ((Interp *) *codePtr->interpHandle != iPtr),
	       (codePtr->compileEpoch != iPtr->compileEpoch),
	       (codePtr->nsPtr != nsPtr),
	       (codePtr->nsEpoch != nsPtr->resolverEpoch));
	      
      {
	CompiledLocal *localPtr = procPtr->firstLocalPtr;
	for (; localPtr != NULL; localPtr = localPtr->nextPtr) {
	  fprintf(stderr, "... local %p '%s' resolveInfo %p deleteProc %p\n",
		  localPtr, localPtr->name, localPtr->resolveInfo, 
		  localPtr->resolveInfo ? localPtr->resolveInfo->deleteProc : NULL);
	}
      }
#endif
      goto doCompilation;
    }
    return TCL_OK;
# endif
  } else {
    int result;
# if defined(HAVE_TCL_COMPILE_H)
  doCompilation:
# endif

    *flagsPtr |= NSF_CSC_CALL_IS_COMPILE;
    result = TclProcCompileProc(interp, procPtr, bodyObj,
                              (Namespace *) nsPtr, "body of proc",
                              procName);
    *flagsPtr &= ~NSF_CSC_CALL_IS_COMPILE;
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
PushProcCallFrame(Proc *procPtr, Tcl_Interp *interp,
		  int objc, Tcl_Obj *CONST objv[],
                  NsfCallStackContent *cscPtr) {
  Tcl_CallFrame *framePtr;
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
			     (Tcl_Namespace *) procPtr->cmdPtr->nsPtr,
			     (FRAME_IS_PROC|FRAME_IS_NSF_METHOD));

  if (unlikely(result != TCL_OK)) {
    return result;
  }

  Tcl_CallFrame_objc(framePtr) = objc;
  Tcl_CallFrame_objv(framePtr) = objv;
  Tcl_CallFrame_procPtr(framePtr) = procPtr;
  Tcl_CallFrame_clientData(framePtr) = cscPtr;

  /*fprintf(stderr, "Stack Frame %p procPtr %p compiledLocals %p firstLocal %p\n",
    framePtr, procPtr, Tcl_CallFrame_compiledLocals(framePtr), procPtr->firstLocalPtr);*/

  return ByteCompiled(interp, &cscPtr->flags, procPtr, ObjStr(objv[0]));
}

static void
GetVarAndNameFromHash(Tcl_HashEntry *hPtr, Var **val, Tcl_Obj **varNameObj) {
  *val = TclVarHashGetValue(hPtr);
  *varNameObj = TclVarHashGetKey(*val);
}

void
NsfProcDeleteProc(ClientData clientData) {
  NsfProcContext *ctxPtr = (NsfProcContext *)clientData;
  INTERP_MEMBER_GET(ctxPtr)

  (*ctxPtr->oldDeleteProc)(ctxPtr->oldDeleteData);
  if (ctxPtr->paramDefs) {
    /*fprintf(stderr, "free ParamDefs %p\n", ctxPtr->paramDefs);*/
    ParamDefsRefCountDecr(INTERP ctxPtr->paramDefs);
  }
  /*fprintf(stderr, "free %p\n", ctxPtr);*/
  FREE(NsfProcContext, ctxPtr);
}

static Nsf_Param *
ParamsNew(INTERP_DECL int nr) {
  Nsf_Param *paramsPtr = NEW_ARRAY(Nsf_Param, nr+1);
  memset(paramsPtr, 0, sizeof(Nsf_Param)*(nr+1));
  return paramsPtr;
}

/*----------------------------------------------------------------------
 * ParamFree --
 *
 *    Deallocate the contents of a single Nsf_Param*
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Free the parameter definition.
 *
 *----------------------------------------------------------------------
 */
static void
ParamFree(INTERP_DECL Nsf_Param *paramPtr) {
  /*fprintf(stderr, "ParamFree %p\n", paramPtr);*/
  if (paramPtr->name) {STRING_FREE("paramPtr->name", paramPtr->name);}
  if (paramPtr->nameObj) {DECR_REF_COUNT(paramPtr->nameObj);}
  if (paramPtr->defaultValue) {DECR_REF_COUNT(paramPtr->defaultValue);}
  if (paramPtr->converterName) {DECR_REF_COUNT2("converterNameObj", paramPtr->converterName);}
  if (paramPtr->converterArg) {DECR_REF_COUNT(paramPtr->converterArg);}
  if (paramPtr->paramObj) {DECR_REF_COUNT(paramPtr->paramObj);}
  if (paramPtr->slotObj) {DECR_REF_COUNT(paramPtr->slotObj);}
  if (paramPtr->method) {DECR_REF_COUNT(paramPtr->method);}
}

/*----------------------------------------------------------------------
 * ParamsFree --
 *
 *    Deallocate a block of multiple Nsf_Param*
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Free the parameter definition.
 *
 *----------------------------------------------------------------------
 */
static void
ParamsFree(INTERP_DECL Nsf_Param *paramsPtr) {
  Nsf_Param *paramPtr;

  /*fprintf(stderr, "ParamsFree %p\n", paramsPtr);*/
  for (paramPtr=paramsPtr; paramPtr->name; paramPtr++) {
    ParamFree(INTERP paramPtr);
  }
  FREE(Nsf_Param*, paramsPtr);
}

static NsfParamDefs *
ParamDefsGet(Tcl_Command cmdPtr) {
  assert(cmdPtr);
  if (Tcl_Command_deleteProc(cmdPtr) == NsfProcDeleteProc) {
    return ((NsfProcContext *)Tcl_Command_deleteData(cmdPtr))->paramDefs;
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * ParamDefsStore --
 *
 *    Store the provided parameter definitions in the provided
 *    command. It stores a new deleteProc which will call the original
 *    delete proc automatically.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    None
 *
 *----------------------------------------------------------------------
 */
static int
ParamDefsStore(INTERP_DECL Tcl_Command cmd, NsfParamDefs *paramDefs) {
  Command *cmdPtr = (Command *)cmd;

  if (cmdPtr->deleteProc != NsfProcDeleteProc) {
    NsfProcContext *ctxPtr = NEW(NsfProcContext);

    /*fprintf(stderr, "ParamDefsStore %p replace deleteProc %p by %p\n",
      paramDefs, cmdPtr->deleteProc, NsfProcDeleteProc);*/

    ctxPtr->oldDeleteData = (Proc *)cmdPtr->deleteData;
    ctxPtr->oldDeleteProc = cmdPtr->deleteProc;
    cmdPtr->deleteProc = NsfProcDeleteProc;
    ctxPtr->paramDefs = paramDefs;
    cmdPtr->deleteData = ctxPtr;
    INTERP_MEMBER_SET(ctxPtr, interp)

    return TCL_OK;
  } else {
    /*fprintf(stderr, "ParamDefsStore cmd %p has already NsfProcDeleteProc deleteData %p\n",
      cmd, cmdPtr->deleteData);*/
    if (cmdPtr->deleteData) {
      NsfProcContext *ctxPtr = cmdPtr->deleteData;
      assert(ctxPtr->paramDefs == NULL);
      ctxPtr->paramDefs = paramDefs;
    }
  }
  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 * ParamDefsNew --
 *
 *    Allocate a new paramDefs structure and initialize it with zeros. The
 *    allocated structure should be freed with ParamDefsFree().
 *
 * Results:
 *    pointer to paramDefs structure
 *
 * Side effects:
 *    Allocating memory
 *
 *----------------------------------------------------------------------
 */
static NsfParamDefs *
ParamDefsNew(INTERP_DECL1) {
  NsfParamDefs *paramDefs;

  paramDefs = NEW(NsfParamDefs);
  memset(paramDefs, 0, sizeof(NsfParamDefs));
  /*fprintf(stderr, "ParamDefsNew %p\n", paramDefs);*/

  return paramDefs;
}
/*
 *----------------------------------------------------------------------
 * ParamDefsFree --
 *
 *    Actually free the parameter definitions. Since the parameter definitions
 *    are refcounted, this function should be just called via
 *    ParamDefsRefCountDecr.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Free the parameter definitions.
 *
 *----------------------------------------------------------------------
 */

static void
ParamDefsFree(INTERP_DECL NsfParamDefs *paramDefs) {
  /*fprintf(stderr, "ParamDefsFree %p slotObj %p returns %p\n",
    paramDefs, paramDefs->slotObj, paramDefs->returns);*/

  if (paramDefs->paramsPtr) {
    ParamsFree(INTERP paramDefs->paramsPtr);
  }
  if (paramDefs->slotObj) {DECR_REF_COUNT2("paramDefsObj", paramDefs->slotObj);}
  if (paramDefs->returns) {DECR_REF_COUNT2("paramDefsObj", paramDefs->returns);}
  FREE(NsfParamDefs, paramDefs);
}

/*
 *----------------------------------------------------------------------
 * ParamDefsRefCountIncr --
 * ParamDefsRefCountDecr --
 *
 *    Perform book keeping on the parameter definitions. RefCounting is
 *    necessary, since it might be possible that during the processing of the
 *    e.g. object parameters, these might be redefined (when an object
 *    parameter calls a method, redefining the
 *    strucures). ParamDefsRefCountDecr() is responsible for actually freeing
 *    the structure.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    No direct.
 *
 *----------------------------------------------------------------------
 */


static void
ParamDefsRefCountIncr(INTERP_DECL NsfParamDefs *paramDefs) {
  paramDefs->refCount ++;
}
static void
ParamDefsRefCountDecr(INTERP_DECL NsfParamDefs *paramDefs) {
  paramDefs->refCount --;
  if (paramDefs->refCount < 1) {
    ParamDefsFree(INTERP paramDefs);
  }
}

/*
 *----------------------------------------------------------------------
 * ParamDefsFormatOption --
 *
 *    Append a parameter option to the nameStringObj representing the
 *    syntax of the parameter definition.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    none
 *
 *----------------------------------------------------------------------
 */
static void
ParamDefsFormatOption(Tcl_Obj *nameStringObj, CONST char *option,
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

/*
 *----------------------------------------------------------------------
 * ParamDefsFormat --
 *
 *    Produce a Tcl_Obj representing a single parameter in the syntax
 *    of the parameter definition.
 *
 * Results:
 *    Tcl_Obj
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static Tcl_Obj *
ParamDefsFormat(Tcl_Interp *interp, Nsf_Param CONST *paramsPtr) {
  int first, colonWritten;
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL), *innerListObj, *nameStringObj;
  Nsf_Param CONST *paramPtr;
  
  INCR_REF_COUNT2("paramDefsObj", listObj);

  for (paramPtr = paramsPtr; paramPtr->name; paramPtr++) {
    if (paramPtr -> paramObj) {
      innerListObj = paramPtr->paramObj;
    } else {
      /*
       * We need this part only for C-defined parameter definitions, defined
       * via genTclAPI.
       *
       * TODO: we could streamline this by defining as well C-API via the same
       * syntax as for accepted for tcl obj types "nsfParam"
       */
      int isNonpos = *paramPtr->name == '-';
      int outputRequired = (isNonpos && (paramPtr->flags & NSF_ARG_REQUIRED));
      int outputOptional = (!isNonpos && !(paramPtr->flags & NSF_ARG_REQUIRED)
                            && !paramPtr->defaultValue &&
                            paramPtr->converter != ConvertToNothing);
      first = 1;
      colonWritten = 0;

      nameStringObj = Tcl_NewStringObj(paramPtr->name, -1);
      if (paramPtr->type) {
        ParamDefsFormatOption(nameStringObj, paramPtr->type, &colonWritten, &first);
      } else if (isNonpos && paramPtr->nrArgs == 0) {
        ParamDefsFormatOption(nameStringObj, "switch", &colonWritten, &first);
      }
      if (outputRequired) {
        ParamDefsFormatOption(nameStringObj, "required", &colonWritten, &first);
      } else if (outputOptional) {
        ParamDefsFormatOption(nameStringObj, "optional", &colonWritten, &first);
      }
      if ((paramPtr->flags & NSF_ARG_SUBST_DEFAULT)) {
        ParamDefsFormatOption(nameStringObj, "substdefault", &colonWritten, &first);
      }
      if ((paramPtr->flags & NSF_ARG_ALLOW_EMPTY) || (paramPtr->flags & NSF_ARG_MULTIVALUED)) {
	char option[10] = "....";
	option[0] = (paramPtr->flags & NSF_ARG_ALLOW_EMPTY) ? '0' : '1';
	option[3] = (paramPtr->flags & NSF_ARG_MULTIVALUED) ? '*' : '1';
        ParamDefsFormatOption(nameStringObj, option, &colonWritten, &first);
      }
      if ((paramPtr->flags & NSF_ARG_IS_CONVERTER)) {
        ParamDefsFormatOption(nameStringObj, "convert", &colonWritten, &first);
      }
      if ((paramPtr->flags & NSF_ARG_INITCMD)) {
        ParamDefsFormatOption(nameStringObj, "initcmd", &colonWritten, &first);
      } else if ((paramPtr->flags & NSF_ARG_ALIAS)) {
        ParamDefsFormatOption(nameStringObj, "alias", &colonWritten, &first);
      } else if ((paramPtr->flags & NSF_ARG_FORWARD)) {
        ParamDefsFormatOption(nameStringObj, "forward", &colonWritten, &first);
      } else if ((paramPtr->flags & NSF_ARG_NOARG)) {
        ParamDefsFormatOption(nameStringObj, "noarg", &colonWritten, &first);
      } else if ((paramPtr->flags & NSF_ARG_NOCONFIG)) {
        ParamDefsFormatOption(nameStringObj, "noconfig", &colonWritten, &first);
      }

      innerListObj = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, innerListObj, nameStringObj);
      if (paramPtr->defaultValue) {
        Tcl_ListObjAppendElement(interp, innerListObj, paramPtr->defaultValue);
      }
    }

    Tcl_ListObjAppendElement(interp, listObj, innerListObj);
  }

  return listObj;
}

/*
 *----------------------------------------------------------------------
 * ParamDefsList --
 *
 *    Produce a Tcl_ListObj containing the list of the parameters
 *    based on a parameter structure.
 *
 * Results:
 *    Tcl_Obj
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static Tcl_Obj *
ParamDefsList(Tcl_Interp *interp, Nsf_Param CONST *paramsPtr) {
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
  Nsf_Param CONST *paramPtr;

  INCR_REF_COUNT2("paramDefsObj", listObj);
  for (paramPtr = paramsPtr; paramPtr->name; paramPtr++) {
    if ((paramPtr->flags & NSF_ARG_NOCONFIG) == 0) {
      Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj(paramPtr->name, -1));
    }
  }
  return listObj;
}


/*
 *----------------------------------------------------------------------
 * ParamDefsNames --
 *
 *    Produce a Tcl_ListObj containing the names of the parameters
 *    based on a parameter structure.
 *
 * Results:
 *    Tcl_Obj
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static Tcl_Obj *
ParamDefsNames(Tcl_Interp *interp, Nsf_Param CONST *paramsPtr) {
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
  Nsf_Param CONST *paramPtr;

  INCR_REF_COUNT2("paramDefsObj", listObj);
  for (paramPtr = paramsPtr; paramPtr->name; paramPtr++) {
    Tcl_ListObjAppendElement(interp, listObj, paramPtr->nameObj);
  }
  return listObj;
}

/*
 *----------------------------------------------------------------------
 * ParamGetType --
 *
 *    Obtain the type of a single parameter and return it as a string.
 *
 * Results:
 *    Type of the parameter in form of a string
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static CONST char *
ParamGetType(Nsf_Param CONST *paramPtr) {
  CONST char *result = "value";

  assert(paramPtr);
  if (paramPtr->type) {
    if (paramPtr->converter == ConvertViaCmd) {
      result = paramPtr->type + 5;
    } else if (paramPtr->converter == Nsf_ConvertToClass &&
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

/*
 *----------------------------------------------------------------------
 * ParamGetDomain --
 *
 *    Obtain the domain of a single parameter and return it as a
 *    string. The domain is an approximate type used in the parameter
 *    syntax.
 *
 * Results:
 *    Domain of the parameter in form of a string
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static CONST char *
ParamGetDomain(Nsf_Param CONST *paramPtr) {
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

/*
 *----------------------------------------------------------------------
 * NsfParamDefsSyntax --
 *
 *    Return the parameter definitions of a sequence of parameters in
 *    the form of the "parametersyntax", inspired by the Tcl manual
 *    pages.
 *
 * Results:
 *    Tcl_Obj containing the parameter syntax
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
NsfParamDefsSyntax(INTERP_DECL Nsf_Param CONST *paramsPtr) {
  Tcl_Obj *argStringObj = Tcl_NewObj();
  Nsf_Param CONST *pPtr;

  INCR_REF_COUNT2("paramDefsObj", argStringObj);

  for (pPtr = paramsPtr; pPtr->name; pPtr++) {

    if ((pPtr->flags & NSF_ARG_NOCONFIG)) {
      /* 
       * Don't output non-configurable parameters
       */
      continue;
    }
    if (pPtr != paramsPtr) {
      /* 
       * Don't output non-consuming parameters (i.e. positional, and no args)
       */
      if (*pPtr->name != '-' && pPtr->nrArgs == 0) {
	continue;
      }
      Tcl_AppendLimitedToObj(argStringObj, " ", 1, INT_MAX, NULL);
    }
    if (pPtr->converter == ConvertToNothing && strcmp(pPtr->name, "args") == 0) {
      Tcl_AppendLimitedToObj(argStringObj, "?arg ...?", 9, INT_MAX, NULL);
    } else if (pPtr->flags & NSF_ARG_REQUIRED) {
      if ((pPtr->flags & NSF_ARG_IS_ENUMERATION)) {
	Tcl_AppendLimitedToObj(argStringObj, ParamGetDomain(pPtr), -1, INT_MAX, NULL);
      } else {
	Tcl_AppendLimitedToObj(argStringObj, pPtr->name, -1, INT_MAX, NULL); // aaaa
      }
    } else {
      Tcl_AppendLimitedToObj(argStringObj, "?", 1, INT_MAX, NULL);
      Tcl_AppendLimitedToObj(argStringObj, pPtr->name, -1, INT_MAX, NULL);
      if (pPtr->nrArgs > 0 && *pPtr->name == '-') {
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

/*
 *----------------------------------------------------------------------
 * ParsedParamFree --
 *
 *    Free the provided information of the parsed parameters.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Freed Memory.
 *
 *----------------------------------------------------------------------
 */
static void
ParsedParamFree(INTERP_DECL NsfParsedParam *parsedParamPtr) {
  /*fprintf(stderr, "ParsedParamFree %p, npargs %p\n",
   parsedParamPtr, parsedParamPtr->paramDefs);*/
  if (parsedParamPtr->paramDefs) {
    ParamDefsRefCountDecr(INTERP parsedParamPtr->paramDefs);
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
#if defined(NSF_WITH_ASSERTIONS)
  NsfObject *object = cscPtr->self;
  NsfObjectOpt *opt = object->opt;
#endif

  /*fprintf(stderr, "ProcMethodDispatchFinalize %s %s flags %.6x isNRE %d pcPtr %p\n",
	  ObjectName(object), methodName,
	  cscPtr->flags, (cscPtr->flags & NSF_CSC_CALL_IS_NRE), pcPtr);*/

#if defined(NSF_WITH_ASSERTIONS)
  if (opt && object->teardown && (opt->checkoptions & CHECK_POST)) {
    int rc;
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
#endif

#if defined(NRE)
  if ((cscPtr->flags & NSF_CSC_CALL_IS_NRE)) {
    if (pcPtr) {
      ParseContextRelease(INTERP pcPtr);
      NsfTclStackFree(interp, pcPtr, "release parse context");
    }
    result = ObjectDispatchFinalize(interp, cscPtr, result /*, "NRE" , methodName*/);

    CscFinish(interp, cscPtr, result, "scripted finalize");
  }
#else
  if (pcPtr) {
    ParseContextRelease(INTERP pcPtr);
  }
#endif

  return result;
}

/*
 *----------------------------------------------------------------------
 * ProcDispatchFinalize --
 *
 *    Finalization function for nsf::proc. Simplified version of
 *    ProcMethodDispatchFinalize().
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
ProcDispatchFinalize(ClientData data[], Tcl_Interp *interp, int result) {
  ParseContext *pcPtr = data[1];
  /*CONST char *methodName = data[0];

    fprintf(stderr, "ProcDispatchFinalize of method %s\n", methodName);*/

# if defined(NSF_PROFILE)
  long int startUsec = (long int)data[2];
  long int startSec = (long int)data[3];
  char *methodName = data[0];
  NsfRuntimeState *rst = RUNTIME_STATE(interp);
  if (rst->doProfile) {
    NsfProfileRecordProcData(interp, methodName, startSec, startUsec);
  }
# endif

  ParseContextRelease(INTERP pcPtr);
  NsfTclStackFree(interp, pcPtr, "nsf::proc dispatch finialize release parse context");

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
  NsfParamDefs *paramDefs;
#if defined(NSF_WITH_ASSERTIONS)
  NsfObjectOpt *opt = object->opt;
#endif
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
   * If this is a filter, check whether its guard applies,
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
      result = GuardCall(object, interp, cmdList->clientData, cscPtr);

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
	CscFinish(interp, cscPtr, result, "guard failed");
#endif
	return result;
      }
    }
  }

#if defined(NSF_WITH_ASSERTIONS)
  if (opt && (opt->checkoptions & CHECK_PRE) &&
      (result = AssertionCheck(interp, object, cl, methodName, CHECK_PRE)) == TCL_ERROR) {
    goto prep_done;
  }
#endif

  /*
   *  If the method to be invoked has paramDefs, we have to call the
   *  argument parser with the argument definitions obtained from the
   *  proc context from the cmdPtr.
  */
  paramDefs = ParamDefsGet(cmdPtr);

  if (paramDefs && paramDefs->paramsPtr) {
#if defined(NRE)
    pcPtr = (ParseContext *) NsfTclStackAlloc(interp, sizeof(ParseContext), "parse context");
#endif
    result = ProcessMethodArguments(pcPtr, interp, object, 1, paramDefs, objv[0], objc, objv);
    cscPtr->objc = objc;
    cscPtr->objv = (Tcl_Obj **)objv;

    if (result == TCL_OK) {
      releasePc = 1;
      result = PushProcCallFrame(cp, interp, pcPtr->objc, pcPtr->full_objv, cscPtr);
    } else {
#if defined(NRE)
      ParseContextRelease(INTERP pcPtr);
      NsfTclStackFree(interp, pcPtr, "parse context (proc prep failed)");
      pcPtr = NULL;
#else
      //fprintf(stderr, "error\n");
      ParseContextRelease(INTERP pcPtr);
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

#if defined(NSF_WITH_ASSERTIONS)
 prep_done:
#endif

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
    CscFinish(interp, cscPtr, result, "nre, prep failed");
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
  int result;

  assert(object);
  assert(object->teardown);
#if defined(NRE)
  assert(!cscPtr || (cscPtr->flags & NSF_CSC_CALL_IS_NRE) == 0);
#endif

  if (cscPtr) {
    /*
     * We have a callstack content, but the requested dispatch will not store
     * the callstack content in a corresponding callframe on its own. To get,
     * for example, self introspection working for the requested dispatch, we
     * introduce a CMETHOD frame.
     */
    /*fprintf(stderr, "Nsf_PushFrameCsc %s %s\n", ObjectName(object), methodName);*/
    Nsf_PushFrameCsc(interp, cscPtr, framePtr);
  }

  /*fprintf(stderr, "CmdMethodDispatch obj %p %p %s\n", obj, methodName, methodName);*/
  result = Tcl_NRCallObjProc(interp, Tcl_Command_objProc(cmdPtr), cp, objc, objv);

  if (cscPtr) {
    Nsf_PopFrameCsc(interp, framePtr);
  }

#if defined(NSF_WITH_ASSERTIONS)
  if (object->opt) {
    CheckOptions co = object->opt->checkoptions;
    if ((co & CHECK_INVAR)) {
      result = AssertionCheckInvars(interp, object, methodName, co);
    }
  }
#endif

  /*
   * Reference counting in the calling ObjectDispatch() makes sure
   * that obj->opt is still accessible even after "dealloc"
   */
  return result;
}

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
		  NsfCallStackContent *cscPtr, CONST char *methodName,
		  int *validCscPtr) {
  Tcl_Command cmd = cscPtr->cmdPtr;
  NsfObject *object = cscPtr->self;
  ClientData cp = Tcl_Command_objClientData(cmd);
  Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);
  int result;

  if (NSF_DTRACE_METHOD_ENTRY_ENABLED()) {
    NSF_DTRACE_METHOD_ENTRY(ObjectName(object),
			    cscPtr->cl ? ClassName(cscPtr->cl) : ObjectName(object),
			    (char *)methodName,
			    objc-1, (Tcl_Obj **)objv+1);
  }

  /*fprintf(stderr, "MethodDispatch method '%s' cmd %p cp=%p objc=%d cscPtr %p flags %.6x\n",
    methodName, cmd, cp, objc, cscPtr, cscPtr->flags);*/
  assert(object->teardown);

  /*
   * The default assumption is that the CscPtr is valid after this function
   * finishes.
   */

  if (proc == TclObjInterpProc) {
#if defined(NRE)
    NRE_callback *rootPtr = TOP_CB(interp);
    int isImmediate = (cscPtr->flags & NSF_CSC_IMMEDIATE);
# if defined(NRE_CALLBACK_TRACE)
    NsfClass *cl = cscPtr->cl;
# endif
#endif
    /*
     * The cmd is a scripted method
     */

    result = ProcMethodDispatch(cp, interp, objc, objv, methodName,
				  object, cscPtr->cl, cmd, cscPtr);
#if defined(NRE)
    /*
     * In the NRE case, there is no trust in the cscPtr anymore, it might be already gone.
     */
    *validCscPtr = 0;

    if (isImmediate) {
# if defined(NRE_CALLBACK_TRACE)
      fprintf(stderr, ".... manual run callbacks rootPtr = %p, result %d methodName %s.%s\n",
	      rootPtr, result, cl?ClassName(cl):"NULL", methodName);
# endif	
      result = NsfNRRunCallbacks(interp, result, rootPtr);
    } else {
# if defined(NRE_CALLBACK_TRACE)
      fprintf(stderr, ".... don't run callbacks rootPtr = %p, result %d methodName %s.%s\n",
	      rootPtr, result, cl?ClassName(cl):"NULL", methodName);
# endif
    }
#endif
    /*
     * scripted method done
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
         * automatically removed. Note that the cmd might be still
         * referenced in various entries in the callstack. The
         * reference counting on these elements takes care that the
         * cmdPtr is deleted on a pop operation (although we do a
         * Tcl_DeleteCommandFromToken() below.
         */
	/*fprintf(stderr, "methodName %s FOUND deleted object with cmd %p my cscPtr %p\n",
	  methodName, cmd, cscPtr);*/
	assert(cscPtr->cmdPtr == cmd);
        Tcl_DeleteCommandFromToken(interp, cmd);

        NsfCleanupObject(INTERP invokeObj, "alias-delete1");
        return NsfPrintError(interp, "Trying to dispatch deleted object via method '%s'",
                              methodName);
      }

      /*
       * Make sure, that the current call is marked as an ensemble call, both
       * for dispatching to the defaultmethod and for dispatching the method
       * interface of the given object. Otherwise, current introspection
       * specific to submethods fails (e.g., a [current methodpath] in the
       * defaultmethod).
       */
      cscPtr->flags |= NSF_CSC_CALL_IS_ENSEMBLE;

      /*
       * The client data cp is still the obj of the called method
       */
      
      /*fprintf(stderr, "ensemble dispatch %s objc %d\n", methodName, objc);*/
      
      if (objc < 2) {
	CallFrame frame, *framePtr = &frame;
	Nsf_PushFrameCsc(interp, cscPtr, framePtr);
	result = DispatchDefaultMethod(interp, invokeObj, objv[0], NSF_CSC_IMMEDIATE);
	Nsf_PopFrameCsc(interp, framePtr);
      } else {
	CallFrame frame, *framePtr = &frame;
	NsfObject *self = (NsfObject *)cp;
	char *methodName = ObjStr(objv[1]);

	cscPtr->objc = objc;
	cscPtr->objv = objv;
	Nsf_PushFrameCsc(interp, cscPtr, framePtr);

	if (self->nsPtr) {
	  cmd = FindMethod(self->nsPtr, methodName);
	  /*fprintf(stderr, "... method %p %s csc %p\n", cmd, methodName, cscPtr); */
	  if (cmd) {
	    /*
	     * In order to allow [next] to be called in an ensemble method,
	     * an extra callframe is needed. This CSC frame is typed as
	     * NSF_CSC_TYPE_ENSEMBLE. Note that the associated call is flagged
	     * additionally (NSF_CSC_CALL_IS_ENSEMBLE; see above) to be able
	     * to identify ensemble-specific frames during [next] execution.
	     *
	     * The dispatch requires NSF_CSC_IMMEDIATE to be set, ensuring
	     * that scripted methods are executed before the ensemble ends. If
	     * they were executed later, they would find their parent frame
	     * (CMETHOD) being popped from the stack already.
	     */

	    /*fprintf(stderr, ".... ensemble dispatch on %s.%s cscPtr %p base flags %.6x cl %s\n",
	      ObjectName(object), methodName, cscPtr, 
	      (0xFF & cscPtr->flags), cscPtr->cl?ObjStr(cscPtr->cl->object.cmdName):NULL);*/
	    result = MethodDispatch(object, interp, objc-1, objv+1,
				    cmd, object, cscPtr->cl, methodName,
				    cscPtr->frameType|NSF_CSC_TYPE_ENSEMBLE,
				    (cscPtr->flags & 0xFF)|NSF_CSC_IMMEDIATE);
	    goto obj_dispatch_ok;
	  }
	}

	/*
	 * The method to be called was not part of this ensemble. Call
	 * next to try to call such methods along the next path.
	 */
	/*fprintf(stderr, "call next instead of unknown %s.%s \n",
	  ObjectName(cscPtr->self), methodName);*/
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
	  ObjectName(self), methodName, ObjectName(object), cscPtr, result, rst->unknown);*/
	if (rst->unknown) {
	  /*
	   * Unknown handling: We trigger a dispatch to an unknown method. The
	   * appropriate unknown handler is either provided for the current
	   * object (at the class or the mixin level), or the default unknown
	   * handler takes it from there. The application-level unknown
	   * handler cannot determine the top-level calling object (referred
	   * to as the delegator). Therefore, we assemble all the neccesssary
	   * call data as the first argument passed to the unknown
	   * handler. Call data include the calling object (delegator), the
	   * method path, and the unknown final method.
	   */
	  Tcl_Obj *callInfoObj = Tcl_NewListObj(1, &object->cmdName);
	  Tcl_Obj *methodPathObj = CallStackMethodPath(interp, (Tcl_CallFrame *)framePtr);

	  INCR_REF_COUNT(methodPathObj);
	  /*fprintf(stderr, "next calls DispatchUnknownMethod\n");*/
	  Tcl_ListObjAppendList(interp, callInfoObj, methodPathObj);
	  DECR_REF_COUNT(methodPathObj);
	  Tcl_ListObjAppendElement(interp, callInfoObj, objv[0]);
	  Tcl_ListObjAppendElement(interp, callInfoObj, objv[1]);

	  result = DispatchUnknownMethod(interp, self, objc-1, objv+1, callInfoObj,
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

  /*fprintf(stderr, "cmdMethodDispatch %s.%s, cscPtr %p objflags %.6x\n",
    ObjectName(object), methodName, cscPtr, object->flags); */

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
  int result, validCscPtr = 1;

  assert (object->teardown);
  assert (cmd);

  /*fprintf(stderr, "MethodDispatch method '%s.%s' objc %d flags %.6x call %d\n",
     ObjectName(object), methodName, objc, flags, call); */

  cscPtr = CscAlloc(interp, &csc, cmd);

  /*
   * We would not need CscInit when
   * cp == NULL && !(Tcl_Command_flags(cmd) & NSF_CMD_NONLEAF_METHOD)
   * TODO: We could pass cmd==NULL, but is this worth it?
   */
  CscInit(INTERP cscPtr, object, cl, cmd, frameType, flags, methodName);

  result = MethodDispatchCsc(clientData, interp, objc, objv,
			     cscPtr, methodName, &validCscPtr);

#if defined(NRE)
  if (validCscPtr) {
    //fprintf(stderr, "forced finalize 2 cscPtr %p\n", cscPtr);
    CscListRemove(interp, cscPtr, NULL);
    CscFinish(interp, cscPtr, result, "csc cleanup");
  }
#else
  CscListRemove(interp, cscPtr, NULL);
  CscFinish(interp, cscPtr, result, "csc cleanup");
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

  /*fprintf(stderr, "ObjectDispatchFinalize %p %s flags %.6x (%d) frame %.6x unk %d m %s\n",
	  cscPtr, ObjectName(object), flags,
	  result, cscPtr->frameType, RUNTIME_STATE(interp)->unknown,
	  cscPtr->cmdPtr ? Tcl_GetCommandName(interp, cscPtr->cmdPtr) : "");*/

  /*
   * Check the return value if wanted
   */
  if (result == TCL_OK && cscPtr->cmdPtr && Tcl_Command_cmdEpoch(cscPtr->cmdPtr) == 0) {
    NsfParamDefs *paramDefs = ParamDefsGet(cscPtr->cmdPtr);

    if (paramDefs && paramDefs->returns) {
      Tcl_Obj *valueObj = Tcl_GetObjResult(interp);
      result = ParameterCheck(interp, paramDefs->returns, valueObj, "return-value:",
			      rst->doCheckResults, NULL);
    }
  } else {
    /*fprintf(stderr, "We have no cmdPtr in cscPtr %p %s",  cscPtr, ObjectName(object));
    fprintf(stderr, "... cannot check return values!\n");*/
  }


  /*
   * On success (no error occured) check for unknown cases.
   */
  if (likely(result == TCL_OK)) {

    if ((flags & NSF_CSC_METHOD_IS_UNKNOWN)
	|| ((cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER) && rst->unknown)
	) {
      result = DispatchUnknownMethod(interp, object, 
				     cscPtr->objc, cscPtr->objv, NULL, cscPtr->objv[0],
				     (cscPtr->flags & NSF_CSC_CALL_NO_UNKNOWN)|NSF_CSC_IMMEDIATE);
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
    /* fprintf(stderr, "MixinStackPop %s.%s %p %s\n",
       ObjectName(object), methodName, object->mixinStack, msg);*/
    MixinStackPop(INTERP object);
  }
  if ((flags & NSF_CSC_FILTER_STACK_PUSHED) && object->filterStack) {
    /* fprintf(stderr, "FilterStackPop %s.%s %p %s\n",
       ObjectName(object), methodName, object->filterStack, msg);*/
    FilterStackPop(INTERP object);
  }

  return result;
}

/*#define INHERIT_CLASS_METHODS 1*/

#if defined(INHERIT_CLASS_METHODS)
static Tcl_Command
NsfFindClassMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *methodName) {
  Tcl_Command cmd;
  NsfClasses *p;

  /*fprintf(stderr, "NsfFindClassMethod %s %s\n", ClassName(cl), methodName);*/
  if (cl->order == NULL) cl->order = TopoOrder(INTERP cl, Super);

  for(p = cl->order; p; p = p->nextPtr) {
    NsfClass *currentClass = p->cl;
    Tcl_Namespace *nsPtr = currentClass->object.nsPtr;

    /*fprintf(stderr, "1 check for obj ns in class %s => %p\n",
      ClassName(currentClass), nsPtr);*/
    if (nsPtr) {
      cmd = FindMethod(nsPtr, methodName);
      /*fprintf(stderr, "1 lookup for method %s in class %s => %p\n",
	methodName, ClassName(currentClass), cmd);*/
      if (cmd) {return cmd;}
    }
  }
  return NULL;
}
#endif

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
ObjectDispatch(ClientData clientData, Tcl_Interp *interp,
	       int objc, Tcl_Obj *CONST objv[],
	       int flags) {
  register NsfObject *object = (NsfObject *)clientData;
  int result = TCL_OK, objflags, shift,
    frameType = NSF_CSC_TYPE_PLAIN;
  CONST char *methodName;
  NsfClass *cl = NULL;
  Tcl_Command cmd = NULL;
  Tcl_Obj *cmdName = object->cmdName, *methodObj, *cmdObj;
  NsfCallStackContent csc, *cscPtr = NULL;
  int validCscPtr = 1;

  if (unlikely(flags & NSF_CM_NO_SHIFT)) {
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
    object ? ObjectName(object) : NULL, objc, cmdObj ? ObjStr(cmdObj) : NULL, methodName);*/
  objflags = object->flags; /* avoid stalling */

  /*
   * Make sure, cmdName and obj survive this method until the end of
   * this function.
   */
  INCR_REF_COUNT(cmdName);
  NsfObjectRefCountIncr(object);

  /*fprintf(stderr, "obj refCount of %p after incr %d (ObjectDispatch) %s\n",
    object, object->refCount, methodName);*/

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
      NsfCallStackContent *cscPtr1 = CallStackGetTopFrame0(interp);

      if (!cscPtr1 ||
	  (object != cscPtr1->self || (cscPtr1->frameType != NSF_CSC_TYPE_ACTIVE_FILTER))) {
	FilterStackPush(INTERP object, methodObj);
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

  /*fprintf(stderr, "MixinStackPush check for %p %s.%s objflags %.6x == %d\n",
	  object, ObjectName(object), methodName, objflags & NSF_MIXIN_ORDER_DEFINED_AND_VALID,
	  (objflags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) == NSF_MIXIN_ORDER_DEFINED_AND_VALID);*/
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

    MixinStackPush(INTERP object);
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
	  object, ObjectName(object), methodName);*/
	cscPtr = CscAlloc(interp, &csc, NULL);
	CscInit(INTERP cscPtr, object, cl, NULL, frameType, flags, methodName);
        goto exit_object_dispatch;
      }
      if (cmd) {
        frameType = NSF_CSC_TYPE_ACTIVE_MIXIN;
      }
    }
  }

  /* check if an absolute method name was provided */
  if (unlikely(*methodName == ':')) {
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
	NsfLog(interp, NSF_LOG_NOTICE,
	       "Don't invoke object %s this way. Register object via alias...",
	       methodName);
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
  if (likely(cmd == NULL)) {
    /* do we have a object-specific proc? */
    if (object->nsPtr && (flags & NSF_CM_NO_OBJECT_METHOD) == 0) {
      cmd = FindMethod(object->nsPtr, methodName);
      /*fprintf(stderr, "lookup for proc in obj %p method %s nsPtr %p => %p\n",
	object, methodName, object->nsPtr, cmd);*/
    }
#if defined(INHERIT_CLASS_METHODS)
    /* this is not optimized yet, since current class might be checked twice,
       but easier to maintain */
    if ((flags & NSF_CM_NO_OBJECT_METHOD) == 0 && cmd == NULL && NsfObjectIsClass(object)) {
      cmd = NsfFindClassMethod(interp, (NsfClass *)object, methodName);
    }
#endif

    if (likely(cmd == NULL)) {
      /* check for a method inherited from a class */
      NsfClass *currentClass = object->cl;
      if (unlikely(currentClass->order == NULL)) {
	currentClass->order = TopoOrder(INTERP currentClass, Super);
      }
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

    if (unlikely(object != lastSelf)) {
      NsfLog(interp, NSF_LOG_WARN, "'%s %s' fails since method %s.%s is protected",
	     ObjectName(object), methodName,
	     cl ? ClassName(cl) : ObjectName(object), methodName);
      /* reset cmd, since it is still unknown */
      cmd = NULL;
    }
  }

  assert(result == TCL_OK);

  if (likely(cmd != NULL)) {
    /*
     * We found the method to dispatch.
     */
    cscPtr = CscAlloc(interp, &csc, cmd);
    CscInit(INTERP cscPtr, object, cl, cmd, frameType, flags, methodName);

    if ((cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER)) {
      /* run filters not NRE enabled */
      cscPtr->flags |= NSF_CSC_IMMEDIATE;
      /* needed for invoking UNKNOWN from ProcMethodDispatchFinalize() */
      cscPtr->objc = objc-shift;
      cscPtr->objv = objv+shift;
    }

    /*fprintf(stderr, "MethodDispatch %s.%s %p flags %.6x cscPtr %p\n",
	    ObjectName(object), methodName, object->mixinStack, cscPtr->flags,
	    cscPtr);*/

    result = MethodDispatchCsc(clientData, interp, objc-shift, objv+shift,
			       cscPtr, methodName, &validCscPtr);

    if (unlikely(result == TCL_ERROR)) {
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
    CscInit(INTERP cscPtr, object, cl, cmd, frameType, flags, methodName);
    cscPtr->flags |= NSF_CSC_METHOD_IS_UNKNOWN;
    if ((flags & NSF_CM_NO_UNKNOWN)) {
      cscPtr->flags |= NSF_CSC_CALL_NO_UNKNOWN;
    }
    cscPtr->objc = objc-shift;
    cscPtr->objv = objv+shift;
  }

 exit_object_dispatch:
  if (likely(validCscPtr)) {
    /*
     * In every situation, we have a cscPtr containing all context information
     */
    assert(cscPtr);

    //fprintf(stderr, "forced finalize 1 cscPtr %p objc %d %p\n", cscPtr, cscPtr->objc, cscPtr->objv);
    result = ObjectDispatchFinalize(interp, cscPtr, result /*, "immediate" , methodName*/);
    CscListRemove(interp, cscPtr, NULL);
    CscFinish(interp, cscPtr, result, "non-scripted finalize");
  }

  /*fprintf(stderr, "ObjectDispatch %s.%s returns %d\n",
    ObjectName(object), methodName, result);*/

  NsfCleanupObject(INTERP object, "ObjectDispatch");
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
DispatchDefaultMethod(Tcl_Interp *interp, NsfObject *object, 
		      Tcl_Obj *obj, int flags) {
  int result;
  Tcl_Obj *methodObj;

  assert(object);

  if (CallDirectly(interp, object, NSF_o_defaultmethod_idx, &methodObj)) {

    Tcl_SetObjResult(interp, object->cmdName);
    result = TCL_OK;

  } else {
    Tcl_Obj *tov[2];

    tov[0] = obj;
    tov[1] = methodObj;
    result = ObjectDispatch(object, interp, 2, tov, flags|NSF_CM_NO_UNKNOWN);
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
  NsfRuntimeState *rst = RUNTIME_STATE(interp);

  /*
   * Don't call destroy after exit handler started physical
   * destruction, or when it was called already before
   */
  if (rst->exitHandlerDestroyRound == NSF_EXITHANDLER_ON_PHYSICAL_DESTROY
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
    NsfErrorContext(interp, "method destroy");

    if (++rst->errorCount > 20) {
      Tcl_Panic("too many destroy errors occured. Endless loop?");
    }
  } else if (rst->errorCount > 0) {
    rst->errorCount--;
  }

#ifdef OBJDELETION_TRACE
  fprintf(stderr, "DispatchDestroyMethod for %p exit\n", object);
#endif
  return result;
}

/*
 *----------------------------------------------------------------------
 * DispatchInitMethod --
 *
in case the object system has it
 *    defined and it was not already called on the object,
 *
 * Results:
 *    Result code.
 *
 * Side effects:
 *    Indirect effects by calling Tcl code
 *
 *----------------------------------------------------------------------
 */
static int
DispatchInitMethod(Tcl_Interp *interp, NsfObject *object, 
		   int objc, Tcl_Obj *CONST objv[],
		   int flags) {
  int result;
  Tcl_Obj *methodObj;

  assert(object);

  /*
   * check, whether init was called already
   */
  if (!(object->flags & (NSF_INIT_CALLED|NSF_DESTROY_CALLED))) {
    
    /*
     * Flag the call to "init" before the dispatch, such that a call to
     * "configure" within init does not clear the already set instance
     * variables.
     */

    object->flags |= NSF_INIT_CALLED;

    if (CallDirectly(interp, object, NSF_o_init_idx, &methodObj)) {
      /*fprintf(stderr, "%s init directly\n", ObjectName(object));*/
      /*
       * Actually, nothing to do.
       */
      result = TCL_OK;
    } else {
      /*fprintf(stderr, "%s init dispatch\n", ObjectName(object));*/
      result = CallMethod(object, interp, methodObj,
			  objc+2, objv, flags|NSF_CM_NO_PROTECT|NSF_CSC_IMMEDIATE);
    }

  } else {
    result = TCL_OK;
  }

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
DispatchUnknownMethod(Tcl_Interp *interp, NsfObject *object,
		      int objc, Tcl_Obj *CONST objv[],
		      Tcl_Obj *callInfoObj, Tcl_Obj *methodObj, int flags) {
  int result;
  Tcl_Obj *unknownObj = NsfMethodObj(object, NSF_o_unknown_idx);
  CONST char *methodName = MethodName(methodObj);

  /*fprintf(stderr, "compare unknownObj %p with methodObj %p '%s' %p %p %s -- %s\n",
    unknownObj, methodObj, ObjStr(methodObj), delegator,
    delegator?objv[1]:NULL,
    delegator?ObjStr(objv[1]) : NULL,
    methodName);*/
  
  if (unknownObj && methodObj != unknownObj && (flags & NSF_CSC_CALL_NO_UNKNOWN) == 0) {
    /*
     * back off and try unknown;
     */
    int mustCopy = *(ObjStr(methodObj)) == ':';
    ALLOC_ON_STACK(Tcl_Obj*, objc+3, tov);

    if (callInfoObj == NULL) {
      callInfoObj = mustCopy ? Tcl_NewStringObj(methodName, -1) : methodObj;
    }
    INCR_REF_COUNT(callInfoObj);
      
    /*fprintf(stderr, "calling unknown for %s %s, flgs=%.6x,%.6x/%.6x isClass=%d %p %s objc %d\n",
      ObjectName(object), ObjStr(methodObj), flags, NSF_CM_NO_UNKNOWN, NSF_CSC_CALL_NO_UNKNOWN,
      NsfObjectIsClass(object), object, ObjectName(object), objc);*/
    
    tov[0] = object->cmdName;
    tov[1] = unknownObj;
    tov[2] = callInfoObj;
    if (objc > 1) {
      memcpy(tov + 3, objv + 1, sizeof(Tcl_Obj *) * (objc - 1));
    }
    
    flags &= ~NSF_CM_NO_SHIFT;
    
    /*fprintf(stderr, "call unknown via dispatch mustCopy %d delegator %p method %s (%s)\n",
      mustCopy, delegator, ObjStr(tov[offset]), ObjStr(methodObj));*/
    
    result = ObjectDispatch(object, interp, objc+2, tov, flags|NSF_CM_NO_UNKNOWN);

    DECR_REF_COUNT(callInfoObj);
    FREE_ON_STACK(Tcl_Obj*, tov);
    
  } else { /* no unknown called, this is the built-in unknown handler */
    Tcl_Obj *tailMethodObj = NULL;

    if (objc > 1) {
      int length;

      if (Tcl_ListObjLength(interp, objv[1], &length) == TCL_OK && length > 0) {
	Tcl_ListObjIndex(interp, objv[1], length - 1, &tailMethodObj);
      }
    }
    
    result = NsfPrintError(interp, "%s: unable to dispatch method '%s'",
			   ObjectName(object), 
			   tailMethodObj ? MethodName(tailMethodObj) : methodName);
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
extern int
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
    result = DispatchDefaultMethod(interp, (NsfObject *)clientData, objv[0], 0);
  }
  return result;
}

/*
 *  Proc-Creation
 */

/*
 *----------------------------------------------------------------------
 * AddPrefixToBody --
 *
 *    Create a fresh TclObj* containing the body with an potential prefix.
 *    The caller has to decrement the refcount on this Tcl_Obj*.
 *
 * Results:
 *    Tcl_Obj
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
AddPrefixToBody(INTERP_DECL Tcl_Obj *body, int paramDefs, NsfParsedParam *paramPtr) {
  Tcl_Obj *resultBody = Tcl_NewObj();

  INCR_REF_COUNT2("resultBody", resultBody);

  if (paramDefs && paramPtr->possibleUnknowns > 0) {
    Tcl_AppendStringsToObj(resultBody, "::nsf::__unset_unknown_args\n", (char *) NULL);
  }

  Tcl_AppendStringsToObj(resultBody, ObjStr(body), (char *) NULL);
  return resultBody;
}

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

/***********************************************************************
 * Nsf_TypeConverter
 ***********************************************************************/

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToString --
 *
 *    Minimal Nsf_TypeConverter setting the client data (passed to C
 *    functions) to the ObjStr of the object.
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
Nsf_ConvertToString(Tcl_Interp *UNUSED(interp), Tcl_Obj *objPtr, Nsf_Param CONST *UNUSED(pPtr),
			   ClientData *clientData, Tcl_Obj **outObjPtr) {
  *clientData = (char *)ObjStr(objPtr);
  *outObjPtr = objPtr;
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * ConvertToNothing --
 *
 *    Minimalistic Nsf_TypeConverter, even setting the client data (passed to
 *    C functions).
 *
 * Results:
 *    Tcl result code, **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
ConvertToNothing(Tcl_Interp *UNUSED(interp), Tcl_Obj *objPtr,  Nsf_Param CONST *UNUSED(pPtr),
		 ClientData *UNUSED(clientData), Tcl_Obj **outObjPtr) {
  *outObjPtr = objPtr;
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToTclobj --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    passed Tcl_Obj. Optionally this converter checks if the Tcl_Obj has
 *    permissible content via the Tcl "string is" checkers.
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
enum stringTypeIdx {StringTypeAlnum, StringTypeAlpha, StringTypeAscii, StringTypeBoolean, StringTypeControl,
		    StringTypeDigit, StringTypeDouble, StringTypeFalse, StringTypeGraph, StringTypeInteger,
		    StringTypeLower, StringTypePrint, StringTypePunct, StringTypeSpace, StringTypeTrue,
		    StringTypeUpper, StringTypeWideinteger, StringTypeWordchar, StringTypeXdigit };
static CONST char *stringTypeOpts[] = {"alnum", "alpha", "ascii", "boolean", "control",
				       "digit", "double", "false", "graph", "integer",
				       "lower", "print", "punct", "space",  "true",
				       "upper", "wideinteger", "wordchar", "xdigit",
				       NULL};

int
Nsf_ConvertToTclobj(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param CONST *pPtr,
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
	*clientData = objPtr;
      } else {
	result = NsfObjErrType(interp, NULL, objPtr, ObjStr(pPtr->converterArg), (Nsf_Param *)pPtr);
      }
    }
  } else {
    result = TCL_OK;
#if defined(NSF_WITH_VALUE_WARNINGS)
    if (RUNTIME_STATE(interp)->debugLevel > 0) {
      char *value = ObjStr(objPtr);
      if (*value == '-'
	  && (pPtr->flags & NSF_ARG_CHECK_NONPOS)
	  && isalpha(*(value+1))
	  && strchr(value+1, ' ') == NULL
	  ) {
	/*
	 * In order to flag a warning, we set the error message and
	 * return TCL_CONTINUE
	 */
	(void)NsfPrintError(interp, "Value '%s' of parameter '%s' could be a non-positional argument",
		      value, pPtr->name);
	result = TCL_CONTINUE;
      }
    }
#endif
    *clientData = objPtr;
  }
  *outObjPtr = objPtr;
  return result;
}

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToBoolean --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    interal repesentation of a boolean. This converter checks the passed
 *    value via Tcl_GetBooleanFromObj().
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
Nsf_ConvertToBoolean(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param CONST *pPtr,
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result, bool;
  result = Tcl_GetBooleanFromObj(interp, objPtr, &bool);

  if (result == TCL_OK) {
    *clientData = (ClientData)INT2PTR(bool);
  } else {
    NsfObjErrType(interp, NULL, objPtr, "boolean", pPtr);
  }
  *outObjPtr = objPtr;
  return result;
}

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToInt32 --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    interal repesentation of an integer. This converter checks the passed
 *    value via Tcl_GetIntFromObj().
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
int
Nsf_ConvertToInt32(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param CONST *pPtr,
		   ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result;
  int i;

  result = Tcl_GetIntFromObj(interp, objPtr, &i);

  if (result == TCL_OK) {
    *clientData = (ClientData)INT2PTR(i);
    *outObjPtr = objPtr;
  } else {
    NsfObjErrType(interp, NULL, objPtr, "int32", (Nsf_Param *)pPtr);
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToInteger --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    TclObj containng the bignum value. This converter checks the passed
 *    value via Tcl_GetBignumFromObj().
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

#include <tclTomMath.h>
int
Nsf_ConvertToInteger(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param CONST *pPtr,
		     ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result;

  /*
   * Try to short_cut common cases to avoid conversion to bignums, since
   * Tcl_GetBignumFromObj returns a value, which has to be freed.
   */
  if (objPtr->typePtr == Nsf_OT_intType) {
    /*
     * We know already, that the value is an int
     */
    result = TCL_OK;
  } else if (objPtr->typePtr == Nsf_OT_doubleType) {
    /*
     * We know already, that the value is not an int
     */
    result = TCL_ERROR;
  } else {
    mp_int bignumValue;

    /*
     * We have to figure out, whether the value is an int. We perform this
     * test via Tcl_GetBignumFromObj(), which tries to keep the type small if
     * possible (e.g. it might return type "int" or "float" when appropriate.
     */

    /*if (objPtr->typePtr) {
      fprintf(stderr, "type is on call %p %s value %s \n",
          objPtr->typePtr, objPtr->typePtr? objPtr->typePtr->name:"NULL", ObjStr(objPtr));
	  }*/

    if ((result = Tcl_GetBignumFromObj(interp, objPtr, &bignumValue)) == TCL_OK) {
      mp_clear(&bignumValue);
    }
  }

  if (result == TCL_OK) {
    *clientData = (ClientData)objPtr;
    *outObjPtr = objPtr;
  } else {
    NsfObjErrType(interp, NULL, objPtr, "integer", (Nsf_Param *)pPtr);
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToSwitch --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    interal repesentation of an Boolean. This converter simply calls
 *    Tcl_ConvertToBoolean(). The distinction between "switch" and boolean is
 *    made on the semantics of which arguments/defaults are passed to the real
 *    converter.
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
Nsf_ConvertToSwitch(Tcl_Interp *interp, Tcl_Obj *objPtr, Nsf_Param CONST *pPtr,
			   ClientData *clientData, Tcl_Obj **outObjPtr) {
  return Nsf_ConvertToBoolean(interp, objPtr, pPtr, clientData, outObjPtr);
}

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToObject --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    interal repesentation of an object. This converter checks the passed
 *    value via IsObjectOfType().
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
Nsf_ConvertToObject(Tcl_Interp *interp, Tcl_Obj *objPtr, Nsf_Param CONST *pPtr,
		ClientData *clientData, Tcl_Obj **outObjPtr) {
  *outObjPtr = objPtr;
  if (GetObjectFromObj(interp, objPtr, (NsfObject **)clientData) == TCL_OK) {
    return IsObjectOfType(interp, (NsfObject *)*clientData, "object", objPtr, pPtr);
  }
  return NsfObjErrType(interp, NULL, objPtr, "object", (Nsf_Param *)pPtr);
}

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToClass --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    interal repesentation of a class. This converter checks the passed
 *    value via IsObjectOfType().
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
Nsf_ConvertToClass(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param CONST *pPtr,
	       ClientData *clientData, Tcl_Obj **outObjPtr) {
  *outObjPtr = objPtr;
  if (GetClassFromObj(interp, objPtr, (NsfClass **)clientData, 0) == TCL_OK) {
    return IsObjectOfType(interp, (NsfObject *)*clientData, "class", objPtr, pPtr);
  }
  return NsfObjErrType(interp, NULL, objPtr, "class", (Nsf_Param *)pPtr);
}



/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToFilterreg --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    Tcl_Obj. This nsf type converter checks the passed value via the
 *    NsfFilterregObjType tcl_obj converter, which provides an internal
 *    representation for the client function.
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
Nsf_ConvertToFilterreg(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param CONST *pPtr,
	       ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result;
  *outObjPtr = objPtr;
  result = Tcl_ConvertToType(interp, objPtr, &NsfFilterregObjType);
  if (result == TCL_OK) {
    *clientData = objPtr;
    return result;
  }
  return NsfObjErrType(interp, NULL, objPtr, "filterreg", (Nsf_Param *)pPtr);
}

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToMixinreg --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    Tcl_Obj. This nsf type converter checks the passed value via the
 *    NsfMixinregObjType tcl_obj converter, which provides an internal
 *    representation for the client function.
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
Nsf_ConvertToMixinreg(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param CONST *pPtr,
	       ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result;
  *outObjPtr = objPtr;
  result = Tcl_ConvertToType(interp, objPtr, &NsfMixinregObjType);
  if (result == TCL_OK) {
    *clientData = objPtr;
    return result;
  }
  return NsfObjErrType(interp, NULL, objPtr, "mixinreg", (Nsf_Param *)pPtr);
}

/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToParameter --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions) to the
 *    Tcl_Obj. This nsf type converter checks if the provided value could be a
 *    valid parameter spec (i.e. start with no ":", is not an unnamed spec
 *    "-:int"). This converter performs just a rough syntactic check.
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int
Nsf_ConvertToParameter(Tcl_Interp *interp, Tcl_Obj *objPtr, Nsf_Param CONST *pPtr,
		   ClientData *clientData, Tcl_Obj **outObjPtr) {
  CONST char *value = ObjStr(objPtr);

  *outObjPtr = objPtr;
  /*fprintf(stderr, "convert to parameter '%s' t '%s'\n", value, pPtr->type);*/
  if (*value == ':' ||  (*value == '-' && *(value + 1) == ':')) {
    return NsfPrintError(interp, "leading colon in '%s' not allowed as in parameter specification '%s'",
			 ObjStr(objPtr), pPtr->name);
    //NsfObjErrType(interp, NULL, objPtr, pPtr->type, (Nsf_Param *)pPtr);
  }

  *clientData = (char *)ObjStr(objPtr);
  *outObjPtr = objPtr;
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * ConvertViaCmd --
 *
 *    Nsf_TypeConverter calling a used-defined checking/conversion
 *    function. It sets the client data (passed to C functions) to the
 *    Tcl_Obj.
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
ConvertViaCmd(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param CONST *pPtr,
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
   * always semantically correct.
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

  oc = 4;
  if (pPtr->converterArg) {
    ov[4] = pPtr->converterArg;
    oc++;
  }

  /*fprintf(stderr, "ConvertViaCmd call converter %s (refCount %d) on %s paramPtr %p arg %p oc %d\n",
	  ObjStr(pPtr->converterName), pPtr->converterName->refCount, ObjStr(ov[0]),
	  pPtr, pPtr->converterArg, oc);*/

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
	    ObjStr(objPtr), ObjStr(Tcl_GetObjResult(interp)), pPtr,
	    pPtr->flags & NSF_ARG_IS_CONVERTER);*/
    if (pPtr->flags & NSF_ARG_IS_CONVERTER) {
      /*
       * If we want to convert, the resulting obj is the result of the
       * converter. incr refCount is necessary e.g. for e.g.
       *     return [expr {$value + 1}]
       */
      *outObjPtr = Tcl_GetObjResult(interp);
      INCR_REF_COUNT2("valueObj", *outObjPtr);
      //fprintf(stderr, "**** NSF_ARG_IS_CONVERTER\n"); ///yyyy;
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

/*
 *----------------------------------------------------------------------
 * ConvertToObjpattern --
 *
 *    This function obtains a Tcl_Obj *, which contains the pattern if an Next
 *    Scripting Object. When this pattern contains no meta characters, we
 *    check if the object exists. If it exists, the Tcl_Obj is converted to
 *    the cmd-type. If it does not exit, the function using this pattern will
 *    fail. If the pattern contains meta characters, we prepend to the pattern
 *    "::" if necessary to avoid errors, if one specifies a pattern object
 *    without the prefix. In this case, the patternObj is is of plain type.
 *    The resulting patternObj has always the refCount incremented, which has
 *    to be decremented by the caller.x
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Incremented refCount for the patternObj.
 *
 *----------------------------------------------------------------------
 */
static int
ConvertToObjpattern(Tcl_Interp *interp, Tcl_Obj *objPtr, Nsf_Param CONST *UNUSED(pPtr),
		    ClientData *clientData, Tcl_Obj **outObjPtr) {
  Tcl_Obj *patternObj = objPtr;
  CONST char *pattern = ObjStr(objPtr);

  if (NoMetaChars(pattern)) {
    /*
     * We have no meta characters, we try to check for an existing object
     */
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
    INCR_REF_COUNT2("patternObj", patternObj);
  }
  *clientData = (ClientData)patternObj;
  *outObjPtr = objPtr;
  return TCL_OK;
}

static Tcl_Obj *
ParamCheckObj(CONST char *start, size_t len) {
  Tcl_Obj *checker = Tcl_NewStringObj("type=", 5);
  Tcl_AppendLimitedToObj(checker, start, len, INT_MAX, NULL);
  return checker;
}

static int
ParamOptionSetConverter(Tcl_Interp *interp, Nsf_Param *paramPtr,
                        CONST char *typeName, Nsf_TypeConverter *converter) {
  if (paramPtr->converter) {
    return NsfPrintError(interp, "Refuse to redefine parameter type of '%s' from type '%s' to type '%s'",
			 paramPtr->name, paramPtr->type, typeName);
  }
  paramPtr->converter = converter;
  paramPtr->nrArgs = 1;
  paramPtr->type = typeName;
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * ParamOptionParse --
 *
 *    Parse a single parameter option of a parameter. The parameter option
 *    string is passed in as second argument, the sizes start and remainder
 *    flag the offsets in the string. As a result, the fields of the parameter
 *    structure are updated.
 *
 * Results:
 *    Tcl result code, updated fields in the Nsf_Param structure.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
ParamOptionParse(Tcl_Interp *interp, CONST char *argString,
		 size_t start, size_t remainder,
		 int disallowedOptions, Nsf_Param *paramPtr) {
  CONST char *dotdot, *option = argString + start;
  char *firstComma = memchr(option, ',', remainder);
  size_t optionLength;
  int result = TCL_OK;

  if (firstComma == NULL) {
    optionLength = remainder;
  } else {
    optionLength = firstComma - option;
  }

  /*fprintf(stderr, "ParamOptionParse name %s, option '%s' (%ld) disallowed %.6x\n",
    paramPtr->name, option, remainder, disallowedOptions);*/

  if (strncmp(option, "required", MAX(3, optionLength)) == 0) {
    paramPtr->flags |= NSF_ARG_REQUIRED;

  } else if (strncmp(option, "optional",  MAX(3, optionLength)) == 0) {
    paramPtr->flags &= ~NSF_ARG_REQUIRED;

  } else if (strncmp(option, "substdefault", 12) == 0) {
    paramPtr->flags |= NSF_ARG_SUBST_DEFAULT;

  } else if (strncmp(option, "convert", 7) == 0) {
    paramPtr->flags |= NSF_ARG_IS_CONVERTER;

  } else if (strncmp(option, "initcmd", 7) == 0) {
    paramPtr->flags |= NSF_ARG_INITCMD;

  } else if (strncmp(option, "alias", 5) == 0) {
    paramPtr->flags |= NSF_ARG_ALIAS;

  } else if (strncmp(option, "forward", 7) == 0) {
    paramPtr->flags |= NSF_ARG_FORWARD;

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
      if ((paramPtr->flags & (NSF_ARG_SWITCH)) != 0) {
	return NsfPrintError(interp,
			     "upper bound of multiplicity of '%c' not allowed for \"switch\"\n", *option);
      }
      paramPtr->flags |= NSF_ARG_MULTIVALUED;
    } else if (*option != '1') {
      return NsfPrintError(interp, "upper bound of multiplicty in %s not supported", argString);
    }

  } else if (strncmp(option, "noarg", 5) == 0) {
    if ((paramPtr->flags & NSF_ARG_ALIAS) == 0) {
      return NsfPrintError(interp, "option \"noarg\" only allowed for parameter type \"alias\"");
    }
    paramPtr->flags |= NSF_ARG_NOARG;
    paramPtr->nrArgs = 0;

  } else if (strncmp(option, "noconfig", 8) == 0) {
    paramPtr->flags |= NSF_ARG_NOCONFIG;

  } else if (strncmp(option, "args", 4) == 0) {
    if ((paramPtr->flags & NSF_ARG_ALIAS) == 0) {
      return NsfPrintError(interp, "option \"args\" only allowed for parameter type \"alias\"");
    }
    result = ParamOptionSetConverter(interp, paramPtr, "args", ConvertToNothing);

  } else if (optionLength >= 4 && strncmp(option, "arg=", 4) == 0) {
    if (paramPtr->converter != ConvertViaCmd) {
      fprintf(stderr, "type %s flags %.6x\n", paramPtr->type, paramPtr->flags);
      return NsfPrintError(interp,
			   "Parameter option 'arg=' only allowed for user-defined converter");
    }
    if (paramPtr->converterArg) {DECR_REF_COUNT(paramPtr->converterArg);}
    paramPtr->converterArg = Tcl_NewStringObj(option + 4, optionLength - 4);
    INCR_REF_COUNT(paramPtr->converterArg);

  } else if (strncmp(option, "switch", 6) == 0) {
    if (*paramPtr->name != '-') {
      return NsfPrintError(interp,
			  "invalid parameter type \"switch\" for argument \"%s\"; "
			   "type \"switch\" only allowed for non-positional arguments",
			   paramPtr->name);
    } else if (paramPtr->flags & NSF_ARG_METHOD_INVOCATION) {
      return NsfPrintError(interp, "Parameter invocation types cannot be used with option 'switch'");
    }
    result = ParamOptionSetConverter(interp, paramPtr, "switch", Nsf_ConvertToSwitch);
    paramPtr->flags |= NSF_ARG_SWITCH;
    paramPtr->nrArgs = 0;
    assert(paramPtr->defaultValue == NULL);
    paramPtr->defaultValue = Tcl_NewBooleanObj(0);
    INCR_REF_COUNT(paramPtr->defaultValue);

  } else if (strncmp(option, "integer", MAX(3, optionLength)) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "integer", Nsf_ConvertToInteger);

  } else if (strncmp(option, "int32", 5) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "int32", Nsf_ConvertToInt32);

  } else if (strncmp(option, "boolean", 7) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "boolean", Nsf_ConvertToBoolean);

  } else if (strncmp(option, "object", 6) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "object", Nsf_ConvertToObject);

  } else if (strncmp(option, "class", 5) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "class", Nsf_ConvertToClass);

  } else if (strncmp(option, "metaclass", 9) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "class", Nsf_ConvertToClass);
    paramPtr->flags |= NSF_ARG_METACLASS;

  } else if (strncmp(option, "baseclass", 9) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "class", Nsf_ConvertToClass);
    paramPtr->flags |= NSF_ARG_BASECLASS;

  } else if (strncmp(option, "mixinreg", 8) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "mixinreg", Nsf_ConvertToMixinreg);

 } else if (strncmp(option, "filterreg", 9) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "filterreg", Nsf_ConvertToFilterreg);

  } else if (strncmp(option, "parameter", 9) == 0) {
    result = ParamOptionSetConverter(interp, paramPtr, "parameter", Nsf_ConvertToParameter);

  } else if (optionLength >= 6 && strncmp(option, "type=", 5) == 0) {
    if (paramPtr->converter != Nsf_ConvertToObject &&
        paramPtr->converter != Nsf_ConvertToClass)
	return NsfPrintError(interp, "Parameter option 'type=' only allowed for parameter types 'object' and 'class'");
    if (paramPtr->converterArg) {DECR_REF_COUNT(paramPtr->converterArg);}
    paramPtr->converterArg = Tcl_NewStringObj(option + 5, optionLength - 5);
    INCR_REF_COUNT(paramPtr->converterArg);

  } else if (optionLength >= 6 && strncmp(option, "slot=", 5) == 0) {
    if (paramPtr->slotObj) {DECR_REF_COUNT(paramPtr->slotObj);}
    paramPtr->slotObj = Tcl_NewStringObj(option + 5, optionLength - 5);
    INCR_REF_COUNT(paramPtr->slotObj);

  } else if (optionLength >= 6 && strncmp(option, "method=", 7) == 0) {
    if ((paramPtr->flags & (NSF_ARG_ALIAS|NSF_ARG_FORWARD)) == 0) {
      return NsfPrintError(interp, "Parameter option 'method=' only allowed for parameter types 'alias' and 'forward'");
    }
    if (paramPtr->method) {DECR_REF_COUNT(paramPtr->method);}
    paramPtr->method = Tcl_NewStringObj(option + 7, optionLength - 7);
    INCR_REF_COUNT(paramPtr->method);

  } else {
    Tcl_DString ds, *dsPtr = &ds;

    Tcl_DStringInit(dsPtr);
    Tcl_DStringAppend(dsPtr, option, optionLength);

    if (paramPtr->converter) {
      NsfPrintError(interp, "Parameter option '%s' unknown for parameter type '%s'",
		    Tcl_DStringValue(dsPtr), paramPtr->type);
      Tcl_DStringFree(dsPtr);
      return TCL_ERROR;
    }

    if (Nsf_PointerTypeLookup(interp, Tcl_DStringValue(dsPtr))) {
      /*
       * Check, if the option refers to a pointer converter
       */
      ParamOptionSetConverter(interp, paramPtr,  Tcl_DStringValue(dsPtr), Nsf_ConvertToPointer);
      Tcl_DStringFree(dsPtr);

    } else {
      int i, found = -1;

      /*
       * The option is still unknown, check the Tcl string-is checkers
       */
      Tcl_DStringFree(dsPtr);

      for (i=0; stringTypeOpts[i]; i++) {
	/* 
	 * Do not allow abbreviations, so the additional strlen checks
	 * for a full match 
	 */
	if (strncmp(option, stringTypeOpts[i], optionLength) == 0
	    && strlen(stringTypeOpts[i]) == optionLength) {
	  found = i;
	  break;
	}
      }

      if (found > -1) {
	/* converter is stringType */
	result = ParamOptionSetConverter(interp, paramPtr, "stringtype", Nsf_ConvertToTclobj);
	if (paramPtr->converterArg) {DECR_REF_COUNT(paramPtr->converterArg);}
	paramPtr->converterArg =  Tcl_NewStringObj(stringTypeOpts[i], -1);
	INCR_REF_COUNT(paramPtr->converterArg);
      } else {
	
	/* 
	 * The parameter option is still unknown. We assume that the parameter
	 * option identifies a user-defined argument checker, implemented as a
	 * method.
	 */
	if (paramPtr->converterName) {DECR_REF_COUNT2("converterNameObj",paramPtr->converterName);}
	paramPtr->converterName = ParamCheckObj(option, optionLength);
	INCR_REF_COUNT2("converterNameObj", paramPtr->converterName);
	result = ParamOptionSetConverter(interp, paramPtr, ObjStr(paramPtr->converterName), ConvertViaCmd);
      }
    }
  }

  if ((paramPtr->flags & disallowedOptions)) {
    return NsfPrintError(interp, "Parameter option '%s' not allowed", option);
  }

  if ((paramPtr->flags & NSF_ARG_METHOD_INVOCATION) && (paramPtr->flags & NSF_ARG_NOCONFIG)) {
    return NsfPrintError(interp, "Option 'noconfig' cannot used together with this type of object parameter");
  } else if ((paramPtr->flags & (NSF_ARG_ALIAS|NSF_ARG_FORWARD)) == (NSF_ARG_ALIAS|NSF_ARG_FORWARD)) {
    return NsfPrintError(interp, "Parameter types 'alias' and 'forward' cannot be used together");
  } else if ((paramPtr->flags & (NSF_ARG_ALIAS|NSF_ARG_INITCMD)) == (NSF_ARG_ALIAS|NSF_ARG_INITCMD)) {
    return NsfPrintError(interp, "Parameter types 'alias' and 'initcmd' cannot be used together");
  } else if ((paramPtr->flags & (NSF_ARG_FORWARD|NSF_ARG_INITCMD)) == (NSF_ARG_FORWARD|NSF_ARG_INITCMD)) {
    return NsfPrintError(interp, "Parameter types 'forward' and 'initcmd' cannot be used together");
  } 

  return result;
}

/*
 *----------------------------------------------------------------------
 * ParamParse --
 *
 *    Parse a a single parameter with a possible default provided in the form
 *    of an Tcl_Obj.
 *
 * Results:
 *    Tcl result code
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
ParamParse(Tcl_Interp *interp, Tcl_Obj *procNameObj, Tcl_Obj *arg, int disallowedFlags,
           Nsf_Param *paramPtr, int *possibleUnknowns, int *plainParams, int *nrNonposArgs) {
  int result, npac, isNonposArgument;
  size_t length, j;
  CONST char *argString, *argName;
  Tcl_Obj **npav;

  paramPtr->paramObj = arg;
  INCR_REF_COUNT(paramPtr->paramObj);

  result = Tcl_ListObjGetElements(interp, arg, &npac, &npav);
  if (result != TCL_OK || npac < 1 || npac > 2) {
    DECR_REF_COUNT(paramPtr->paramObj);
    return NsfPrintError(interp, "wrong # of elements in parameter definition for method '%s'"
			 " (should be 1 or 2 list elements): %s",
			 ObjStr(procNameObj), ObjStr(arg));
  }

  argString = ObjStr(npav[0]);
  length = strlen(argString);

  /*
   * Per default parameter have exactly one argument; types without arguments
   * (like "switch") have to set their nrArgs explicitly.
   */
  paramPtr->nrArgs = 1;

  isNonposArgument = *argString == '-';
  if (isNonposArgument) {
    argName = argString+1;
    (*nrNonposArgs) ++;
  } else {
    argName = argString;
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
    STRING_NEW(paramPtr->name, argString, j);
    paramPtr->nameObj = Tcl_NewStringObj(argName, isNonposArgument ? j-1 : j);
    INCR_REF_COUNT(paramPtr->nameObj);

    /* skip space at begin */
    for (start = j+1; start<length && isspace((int)argString[start]); start++) {;}

    /* search for ',' */
    for (l = start; l < length; l++) {
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
    if (end-start > 0) {
      result = ParamOptionParse(interp, argString, start, end-start, disallowedFlags, paramPtr);
      if (result != TCL_OK) {
	goto param_error;
      }
    }

  } else {
    /* no ':', the whole arg is the name, we have no options */
    STRING_NEW(paramPtr->name, argString, length);
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
    NsfPrintError(interp,
		  "parameter option substdefault specified for parameter \"%s\""
		  " without default value", paramPtr->name);
    goto param_error;
  }

  /* postprocessing the parameter options */

  if (paramPtr->converter == NULL) {
    /* 
     * If no converter is set, use the default converter 
     */
    paramPtr->converter = Nsf_ConvertToTclobj;
  } else if (paramPtr->converter == ConvertToNothing) {
    if (paramPtr->flags & (NSF_ARG_ALLOW_EMPTY|NSF_ARG_MULTIVALUED)) {
      NsfPrintError(interp,
		    "Multiplicity settings for variable argument parameter \"%s\" not allowed",
		    paramPtr->name);
      goto param_error;
    }
  }

  /*
   * Check for application specific value checkers and converters
   */
  /*fprintf(stderr, "parm %s: slotObj %p viaCmd? %d\n",
	  paramPtr->name, paramPtr->slotObj, paramPtr->converter == ConvertViaCmd);*/

  if ((paramPtr->slotObj || paramPtr->converter == ConvertViaCmd) && paramPtr->type) {
    CONST char *converterNameString;
    Tcl_Obj *converterNameObj;
    NsfObject *paramObject;
    Tcl_Command cmd;
    NsfClass *pcl = NULL;

    result = GetObjectFromObj(interp, paramPtr->slotObj ? paramPtr->slotObj :
			      NsfGlobalObjs[NSF_METHOD_PARAMETER_SLOT_OBJ],
			      &paramObject);
    if (result != TCL_OK) {
      goto param_error;
    }
    if (paramPtr->converterName == NULL) {
      converterNameObj = ParamCheckObj(paramPtr->type, strlen(paramPtr->type));
      INCR_REF_COUNT2("converterNameObj",converterNameObj);
    } else {
      converterNameObj = paramPtr->converterName;
    }
    converterNameString = ObjStr(converterNameObj);

    cmd = ObjectFindMethod(interp, paramObject, converterNameObj, &pcl);
    /*fprintf(stderr, "locating %s on %s returns %p (%s)\n", 
      ObjStr(converterNameObj), ObjectName(paramObject), cmd, ClassName(pcl));*/
    if (cmd == NULL) {
      if (paramPtr->converter == ConvertViaCmd) {

	NsfLog(interp, NSF_LOG_WARN, "Could not find value checker %s defined on %s",
	       converterNameString, ObjectName(paramObject));

        paramPtr->flags |= NSF_ARG_CURRENTLY_UNKNOWN;
        /* TODO: for the time being, we do not return an error here */
      }
    } else if (paramPtr->converter != ConvertViaCmd &&
               strcmp(ObjStr(paramPtr->slotObj),
		      NsfGlobalStrings[NSF_METHOD_PARAMETER_SLOT_OBJ]) != 0) {

      NsfLog(interp, NSF_LOG_WARN, "Checker method %s defined on %s shadows built-in converter",
	     converterNameString, ObjectName(paramObject));
      
      if (paramPtr->converterName == NULL) {
        paramPtr->converterName = converterNameObj;
        paramPtr->converter = NULL;
        result = ParamOptionSetConverter(interp, paramPtr, converterNameString, ConvertViaCmd);
	if (result != TCL_OK) {
	  if (converterNameObj != paramPtr->converterName) {
	    DECR_REF_COUNT2("converterNameObj", converterNameObj);
	  }
	  goto param_error;
	}
      }
    }
    if ((paramPtr->flags & NSF_ARG_IS_CONVERTER) && paramPtr->converter != ConvertViaCmd) {
      NsfPrintError(interp, "option 'convert' only allowed for application-defined converters");
      if (converterNameObj != paramPtr->converterName) {
	DECR_REF_COUNT2("converterNameObj",converterNameObj);
      }
      goto param_error;
    }

    if (converterNameObj != paramPtr->converterName) {
      DECR_REF_COUNT2("converterNameObj", converterNameObj);
    }
  }

  /*
   * If the argument has no arguments and it is positional, it can't be
   * required.
   */
  if (paramPtr->nrArgs == 0 && *paramPtr->name != '-' &&
      paramPtr->flags & NSF_ARG_REQUIRED) {
    paramPtr->flags &= ~NSF_ARG_REQUIRED;
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
  ParamFree(INTERP paramPtr);
  paramPtr->name = NULL;

  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 * ParamDefsParse --
 *
 *    Parse a list of parameters in the form of Tcl_Objs into a parsedParamPtr
 *    structure (last argument). The argument allowedOptions is used to flag,
 *    what parameter options are generally allowed (typically different for
 *    method and object parameters). Unless forceParamdefs is set, the parsed
 *    parameter structure is only returned when needed (i.e. when not all
 *    parameters are plain parameters).
 *
 * Results:
 *    Tcl result code, parsedParameter structure in last argument (allocated
 *    by the caller).
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static int
ParamDefsParse(Tcl_Interp *interp, Tcl_Obj *procNameObj, Tcl_Obj *paramSpecObjs,
               int allowedOptinons, int forceParamdefs, NsfParsedParam *parsedParamPtr) {
  Tcl_Obj **argsv;
  int result, argsc;

  parsedParamPtr->paramDefs = NULL;
  parsedParamPtr->possibleUnknowns = 0;

  result = Tcl_ListObjGetElements(interp, paramSpecObjs, &argsc, &argsv);
  if (result != TCL_OK) {
    return NsfPrintError(interp, "cannot break down non-positional args: %s", ObjStr(paramSpecObjs));
  }

  if (argsc > 0) {
    Nsf_Param *paramsPtr, *paramPtr, *lastParamPtr;
    int i, possibleUnknowns = 0, plainParams = 0, nrNonposArgs = 0;
    NsfParamDefs *paramDefs;

    paramPtr = paramsPtr = ParamsNew(INTERP argsc);

    for (i=0; i < argsc; i++, paramPtr++) {
      result = ParamParse(interp, procNameObj, argsv[i], allowedOptinons,
			  paramPtr, &possibleUnknowns, &plainParams, &nrNonposArgs);
      if (result == TCL_OK && paramPtr->converter == ConvertToNothing && i < argsc-1) {
	result =  NsfPrintError(interp,
				"parameter option \"args\" invalid for parameter \"%s\"; only allowed for last parameter",
				paramPtr->name);
      }
      if (result != TCL_OK) {
        ParamsFree(INTERP paramsPtr);
        return result;
      }
      /* every parameter must have at least a name set */
      assert(paramPtr->name);
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
    if (plainParams == argsc && !forceParamdefs) {
      ParamsFree(INTERP paramsPtr);
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

    paramDefs = ParamDefsNew(INTERP1);
    paramDefs->paramsPtr = paramsPtr;
    paramDefs->nrParams = paramPtr-paramsPtr;
    /*fprintf(stderr, "method %s paramDefs %p ifsize %ld, possible unknowns = %d,\n",
      ObjStr(procNameObj), paramDefs, paramPtr-paramsPtr, possibleUnknowns);*/
    parsedParamPtr->paramDefs = paramDefs;
    parsedParamPtr->possibleUnknowns = possibleUnknowns;
  }
  return TCL_OK;
}

static int
MakeProc(Tcl_Namespace *nsPtr, NsfAssertionStore *aStore, Tcl_Interp *interp,
         Tcl_Obj *nameObj, Tcl_Obj *args, Tcl_Obj *body, Tcl_Obj *precondition,
         Tcl_Obj *postcondition, NsfObject *defObject, NsfObject *regObject,
         int withPer_object, int withInner_namespace) {
  Tcl_CallFrame frame, *framePtr = &frame;
  CONST char *methodName = ObjStr(nameObj);
  NsfParsedParam parsedParam;
  Tcl_Obj *ov[4];
  int result;

  if (regObject == NULL) {regObject = defObject;}
  /* Check, if we are allowed to redefine the method */
  result = CanRedefineCmd(interp, nsPtr, defObject, methodName);
  if (likely(result == TCL_OK)) {
    /* Yes, so obtain an method parameter definitions */
    result = ParamDefsParse(interp, nameObj, args,
			    NSF_DISALLOWED_ARG_METHOD_PARAMETER, 0,
			    &parsedParam);
  }
  if (unlikely(result != TCL_OK)) {
    return result;
  }

  ov[0] = NULL; /*objv[0];*/
  ov[1] = nameObj;

  if (parsedParam.paramDefs) {
    Nsf_Param *pPtr;
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
    ov[3] = AddPrefixToBody(INTERP body, 1, &parsedParam);
  } else { /* no parameter handling needed */
    ov[2] = args;
    ov[3] = AddPrefixToBody(INTERP body, 0, &parsedParam);
  }

  Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, nsPtr, 0);
  /* create the method in the provided namespace */
  result = Tcl_ProcObjCmd(0, interp, 4, ov);
  if (result == TCL_OK) {
    /* retrieve the defined proc */
    Proc *procPtr = FindProcMethod(nsPtr, methodName);
    if (procPtr) {
      /* modify the cmd of the proc to set the current namespace for the body */
      if (withInner_namespace) {
        /*
         * Set the namespace of the method as inside of the class
         */
        if (!regObject->nsPtr) {
          MakeObjNamespace(interp, regObject);
        }
        /*fprintf(stderr, "obj %s\n", ObjectName(defObject));
	fprintf(stderr, "ns %p defObject->ns %p\n", nsPtr, defObject->nsPtr);
	fprintf(stderr, "ns %s defObject->ns %s\n", nsPtr->fullName, defObject->nsPtr->fullName);
	fprintf(stderr, "old %s\n", procPtr->cmdPtr->nsPtr->fullName);*/
        procPtr->cmdPtr->nsPtr = (Namespace *)regObject->nsPtr;
      } else {
        /*
         * Set the namespace of the method to the same namespace the cmd of
         * the defObject has.
	 */
        procPtr->cmdPtr->nsPtr = ((Command *)regObject->id)->nsPtr;
      }

      ParamDefsStore(INTERP (Tcl_Command)procPtr->cmdPtr, parsedParam.paramDefs);
      Tcl_SetObjResult(interp, MethodHandleObj(defObject, withPer_object, methodName));
      result = TCL_OK;
    }
  }
  Tcl_PopCallFrame(interp);

#if defined(NSF_WITH_ASSERTIONS)
  if (result == TCL_OK && (precondition || postcondition)) {
    AssertionAddProc(interp, methodName, aStore, precondition, postcondition);
  }
#endif

  if (parsedParam.paramDefs) {
    DECR_REF_COUNT(ov[2]);
  }
  DECR_REF_COUNT2("resultBody", ov[3]);

  return result;
}

static int
MakeMethod(Tcl_Interp *interp, NsfObject *defObject, NsfObject *regObject,
	   NsfClass *cl, Tcl_Obj *nameObj, Tcl_Obj *args, Tcl_Obj *body,
           Tcl_Obj *precondition, Tcl_Obj *postcondition,
           int withInner_namespace) {
  CONST char *argsStr = ObjStr(args), *bodyStr = ObjStr(body), *nameStr = ObjStr(nameObj);
  int result;

  if (precondition && !postcondition) {
    return NsfPrintError(interp, "%s method '%s'; when specifying a precondition (%s)"
			 " a postcondition must be specified as well",
			 ClassName(cl), nameStr, ObjStr(precondition));
  }

  if (*argsStr == 0 && *bodyStr == 0) {
    /*
     * Both, args and body are empty strings. This means we should delete the
     * method.
     */

    if (RUNTIME_STATE(interp)->exitHandlerDestroyRound == NSF_EXITHANDLER_OFF) {
      /*
       * Don't delete methods via scripting during shutdown
       */
      result = cl ?
	NsfRemoveClassMethod(interp, (Nsf_Class *)cl, nameStr) :
	NsfRemoveObjectMethod(interp, (Nsf_Object *)defObject, nameStr);
    } else {
      /* fprintf(stderr, "don't delete method %s during shutdown\n", nameStr); */
      result = TCL_OK;
    }

  } else {
#if defined(NSF_WITH_ASSERTIONS)
    NsfAssertionStore *aStore = NULL;
    if (precondition || postcondition) {
      if (cl) {
        NsfClassOpt *opt = NsfRequireClassOpt(INTERP cl);
        if (!opt->assertions) {
          opt->assertions = AssertionCreateStore(INTERP1);
	}
        aStore = opt->assertions;
      } else {
        NsfObjectOpt *opt = NsfRequireObjectOpt(INTERP defObject);
        if (!opt->assertions) {
          opt->assertions = AssertionCreateStore(INTERP1);
	}
        aStore = opt->assertions;
      }
    }
    result = MakeProc(cl ? cl->nsPtr : defObject->nsPtr, aStore,
		      interp, nameObj, args, body, precondition, postcondition,
		      defObject, regObject, cl == NULL, withInner_namespace);
#else
    result = MakeProc(cl ? cl->nsPtr : defObject->nsPtr, NULL,
		      interp, nameObj, args, body, NULL, NULL,
		      defObject, regObject, cl == NULL, withInner_namespace);
#endif
  }

  if (cl) {
    /* could be a filter or filter inheritance ... update filter orders */
    FilterInvalidateObjOrders(interp, cl);
  } else {
    /* could be a filter => recompute filter order */
    FilterComputeDefined(interp, defObject);
  }

  return result;
}

/**************************************************************************
 * Begin Definition of Parameter procs (Tcl Procs with Parameter handling)
 **************************************************************************/
/*
 *----------------------------------------------------------------------
 * NsfProcStubDeleteProc --
 *
 *    Tcl_CmdDeleteProc for NsfProcStubs. Is called, whenever a
 *    NsfProcStub is deleted and frees the associated client data.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Frees clientdata
 *
 *----------------------------------------------------------------------
 */
static void
NsfProcStubDeleteProc(ClientData clientData) {
  NsfProcClientData *tcd = clientData;
  INTERP_MEMBER_GET(tcd)

  /*fprintf(stderr, "NsfProcStubDeleteProc received %p\n", clientData);
    fprintf(stderr, "... procName %s paramDefs %p\n", ObjStr(tcd->procName), tcd->paramDefs);*/

  DECR_REF_COUNT2("procNameObj",tcd->procName);
  if (tcd->cmd) {
    NsfCommandRelease(INTERP tcd->cmd);
  }
  /* tcd->paramDefs is freed by NsfProcDeleteProc() */
  FREE(NsfProcClientData, tcd);
}

/*
 *----------------------------------------------------------------------
 * InvokeShadowedProc --
 *
 *    Call the proc specified in objc/objv; procNameObj should be used
 *    for error messages.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
// #define NSF_INVOKE_SHADOWED_TRADITIONAL 1
static int
InvokeShadowedProc(Tcl_Interp *interp, Tcl_Obj *procNameObj, Tcl_Command cmd, ParseContext *pcPtr) {
  Tcl_Obj *CONST *objv = pcPtr->full_objv;
  int objc = pcPtr->objc;
  int result;

#if defined(NSF_PROFILE)
  struct timeval trt;
  NsfRuntimeState *rst = RUNTIME_STATE(interp);

  if (rst->doProfile) {
    gettimeofday(&trt, NULL);
  }
#endif

#if defined(NSF_INVOKE_SHADOWED_TRADITIONAL)
  /*
   * For the time being, we call the shadowed proc defined with a
   * mutated name. It should be possible to compile and call the
   * proc body directly, similar as for scripted methods.
   */
  /*fprintf(stderr, "NsfProcStub: call proc arguments oc %d [0] '%s' \n",
    objc, ObjStr(objv[0]));*/

  result = Tcl_EvalObjv(interp, objc, objv, 0);
# if defined(NSF_PROFILE)
  if (rst->doProfile) {
    NsfProfileRecordProcData(interp, ObjStr(procNameObj), trt.tv_sec, trt.tv_usec);
  }
# endif
  ParseContextRelease(INTERP pcPtr);
  NsfTclStackFree(interp, pcPtr, "release parse context");
#else
  /*
   * The code below is derived from the scripted method dispatch and just
   * slightly adapted to remove object dependencies.
   */
  CONST char *fullMethodName = ObjStr(procNameObj);
  Tcl_CallFrame *framePtr;
  unsigned short dummy;
  Proc *procPtr;

  /*fprintf(stderr, "fullMethodName %s epoch %d\n", fullMethodName, Tcl_Command_cmdEpoch(cmd));*/

  if (Tcl_Command_cmdEpoch(cmd)) {
#if 1
    /*
     * It seems, as someone messed around with the shadowed proc. For now, we
     * give up.
     */
    return NsfPrintError(interp, "command '%s' is epoched", fullMethodName);
#else
    /*
     * We could refetch the command ...
     */

    cmd = Tcl_GetCommandFromObj(interp, procNameObj);
    if (!cmd) {
      return NsfPrintError(interp, "cannot lookup command '%s'", fullMethodName);
    }
    if (!CmdIsProc(cmd)) {
      return NsfPrintError(interp, "command '%s' is not a proc", fullMethodName);
    }
    /*
     * ... and update the refCounts
     */
    NsfCommandRelease(INTERP tcd->cmd);
    tcd->cmd = cmd;
    NsfCommandPreserve(INTERP tcd->cmd);
#endif
  }

  procPtr = (Proc *)Tcl_Command_objClientData(cmd);
  result = TclPushStackFrame(interp, &framePtr,
			     (Tcl_Namespace *) procPtr->cmdPtr->nsPtr,
			     (FRAME_IS_PROC));

  if (result == TCL_OK) {
    result = ByteCompiled(interp, &dummy, procPtr, fullMethodName);
  }
  if (result != TCL_OK) {
    /* todo: really? error msg? */
    return result;
  }

  Tcl_CallFrame_objc(framePtr) = objc;
  Tcl_CallFrame_objv(framePtr) = objv;
  Tcl_CallFrame_procPtr(framePtr) = procPtr;

# if defined(NRE)
  /*fprintf(stderr, "CALL TclNRInterpProcCore proc '%s' %s nameObj %p %s\n",
    ObjStr(objv[0]), fullMethodName, procNameObj, ObjStr(procNameObj));*/
  Tcl_NRAddCallback(interp, ProcDispatchFinalize,
		    (ClientData)fullMethodName, pcPtr,
#  if defined(NSF_PROFILE)
		    (ClientData)(unsigned long)trt.tv_usec,
		    (ClientData)(unsigned long)trt.tv_sec
#  else
		    NULL,
		    NULL
#  endif
		    );
  result = TclNRInterpProcCore(interp, procNameObj, 1, &MakeProcError);
# else
  {
  ClientData data[4] = {
    (ClientData)fullMethodName,
    pcPtr,
#  if defined(NSF_PROFILE)
    (ClientData)(unsigned long)trt.tv_usec,
    (ClientData)(unsigned long)trt.tv_sec
#  else
    NULL,
    NULL
#  endif
  };
  result = TclObjInterpProcCore(interp, procNameObj, 1, &MakeProcError);
  result = ProcDispatchFinalize(data, interp, result);
  }
# endif
#endif
  return result;
}

/*
 *----------------------------------------------------------------------
 * NsfProcStub --
 *
 *    Tcl_ObjCmdProc implementing Proc Stubs. This function processes
 *    the argument list in accordance with the parameter definitions
 *    and calls in case of success the shadowed proc.
 *
 * Results:
 *    Tcl return code.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
extern int
NsfProcStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfProcClientData *tcd = clientData;
  int result;

  assert(tcd);
  /*fprintf(stderr, "NsfProcStub %s is called, tcd %p\n", ObjStr(objv[0]), tcd);*/

  if (tcd->paramDefs && tcd->paramDefs->paramsPtr) {
    ParseContext *pcPtr = (ParseContext *) NsfTclStackAlloc(interp, sizeof(ParseContext), "parse context");
    ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

    /*
     * We have to substitute the first element of objv with the name
     * of the function to be called. Since objv is immutable, we have
     * to copy the full argument vector and replace the element on
     * position [0]
     */
    memcpy(tov+1, objv+1, sizeof(Tcl_Obj *)*(objc-1));
    tov[0] = tcd->procName;

    /* If the argument parsing is ok, the shadowed proc will be called */
    result = ProcessMethodArguments(pcPtr, interp, NULL, 0,
				    tcd->paramDefs, objv[0],
				    objc, tov);

    if (result == TCL_OK) {
      result = InvokeShadowedProc(interp, tcd->procName, tcd->cmd, pcPtr);
    } else {
      /*Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
      fprintf(stderr, "NsfProcStub: incorrect arguments (%s)\n", ObjStr(resultObj));*/
      ParseContextRelease(INTERP pcPtr);
      NsfTclStackFree(interp, pcPtr, "release parse context");
    }

    /*fprintf(stderr, "NsfProcStub free on stack %p\n", tov);*/
    FREE_ON_STACK(Tcl_Obj *, tov);
  } else {
    fprintf(stderr, "no parameters\n");
    assert(0); /* should never happen */
    result = TCL_ERROR;
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 * NsfAddParameterProc --
 *
 *    Add a command for implementing a Tcl proc with next scripting
 *    parameter handling.
 *
 *    For the time being, this function adds two things, (a) a Tcl cmd
 *    functioning as a stub for the argument processing (in accordance
 *    with the parameter definitions) and (b) the shadowed Tcl proc
 *    with a mutated name.
 *
 *    TODO: the current 1 cmd + 1 proc implementation is not robust
 *    against renaming and partial deletions (deletion of the
 *    stub). The sketched variant should be better and should be
 *    examined first in detail.
 *
 * Results:
 *    Tcl return code.
 *
 * Side effects:
 *    Adding one Tcl command and one Tcl proc
 *
 *----------------------------------------------------------------------
 */
static int
NsfAddParameterProc(Tcl_Interp *interp, NsfParsedParam *parsedParamPtr,
		    CONST char *procName, Tcl_Obj *body, int with_ad) {
  NsfParamDefs *paramDefs = parsedParamPtr->paramDefs;
  Tcl_Namespace *cmdNsPtr;
  NsfProcClientData *tcd;
  Tcl_Obj *argList = Tcl_NewListObj(0, NULL);
  Tcl_Obj *procNameObj;
  Tcl_DString ds, *dsPtr = &ds;
  Nsf_Param *paramPtr;
  Tcl_Obj *ov[4];
  int result;
  Tcl_Command cmd;

  Tcl_DStringInit(dsPtr);

  /*
   * Create a fully qualified procName
   */
  if (*procName != ':') {
    DStringAppendQualName(dsPtr, Tcl_GetCurrentNamespace(interp), procName);
    procName = Tcl_DStringValue(dsPtr);
  }
  /*
   * Create first the ProcStub to obtain later its namespace, which is
   * needed as the inner namespace of the shadowed proc.
   */
  tcd = NEW(NsfProcClientData);
  INTERP_MEMBER_SET(tcd, interp)
  cmd = Tcl_CreateObjCommand(interp, procName, NsfProcStub,
			     tcd, NsfProcStubDeleteProc);
  if (cmd == NULL) {
    /*
     * For some reason, the command could not be created. Let us hope,
     * we have a useful error message.
     */
    Tcl_DStringFree(dsPtr);
    FREE(NsfProcClientData, tcd);
    return TCL_ERROR;
  }

  cmdNsPtr = Tcl_Command_nsPtr(cmd);
  ParamDefsStore(INTERP cmd, paramDefs);

  /*fprintf(stderr, "NsfAddParameterProc procName '%s' define cmd '%s' %p in namespace %s\n",
    procName, Tcl_GetCommandName(interp, cmd), cmd, cmdNsPtr->fullName);*/

  /*
   * Let us create the shadowed Tcl proc, which is stored under
   * ::nsf::procs::*. First build the fully qualified name
   * procNameObj.
   */
  Tcl_DStringSetLength(dsPtr, 0);
  Tcl_DStringAppend(dsPtr, "::nsf::procs", -1);
  DStringAppendQualName(dsPtr, cmdNsPtr, Tcl_GetCommandName(interp, cmd));
  procNameObj = Tcl_NewStringObj(Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr));

  INCR_REF_COUNT2("procNameObj", procNameObj); /* will be freed, when NsfProcStub is deleted */

  /*
   * Make sure to create the target namespace under "::nsf::procs::", if
   * it does not exist.
   */
  {
    Namespace *nsPtr, *dummy1Ptr, *dummy2Ptr;
    const char *dummy;
    /* create the target namespace, if it does not exist */
    TclGetNamespaceForQualName(interp, ObjStr(procNameObj), NULL, TCL_CREATE_NS_IF_UNKNOWN,
			       &nsPtr, &dummy1Ptr,
			       &dummy2Ptr, &dummy);
  }

  /*
   * Create the client data, which links the stub cmd with the proc.
   */
  tcd->procName = procNameObj;
  tcd->paramDefs = paramDefs;
  tcd->with_ad = with_ad;
  tcd->cmd = NULL;

  /*fprintf(stderr, "NsfAddParameterProc %s tcd %p paramdefs %p\n",
    ObjStr(procNameObj), tcd, tcd->paramDefs);*/

  /*
   * Build an argument list for the shadowed proc.
   */
  argList = Tcl_NewListObj(0, NULL);
  INCR_REF_COUNT(argList);

  for (paramPtr = paramDefs->paramsPtr; paramPtr->name; paramPtr++) {
    if (*paramPtr->name == '-') {
      Tcl_Obj *varNameObj = Tcl_NewStringObj(paramPtr->name+1, -1);
      /*
       * If we have the -ad (for ars digita) flag set, we provide the
       * OpenACS semantics. This is (a) to use the name "boolean" for
       * a switch and (b) to name the automatic variable with the
       * prefix "_p".
       */
      if (with_ad && paramPtr->converter == Nsf_ConvertToBoolean && paramPtr->nrArgs == 1) {
	/*fprintf(stderr, "... ad handling: proc %s param %s type %s nrargs %d default %p\n",
	  procName, paramPtr->name, paramPtr->type, paramPtr->nrArgs, paramPtr->defaultValue);*/
	paramPtr->nrArgs = 0;
	/*paramPtr->converter = Nsf_ConvertToSwitch;*/
	Tcl_AppendToObj(varNameObj, "_p", 2);
	if (paramPtr->defaultValue == NULL) {
	  paramPtr->defaultValue = Tcl_NewBooleanObj(0);
	  INCR_REF_COUNT(paramPtr->defaultValue);
	}
      }
      Tcl_ListObjAppendElement(interp, argList, varNameObj);
    } else {
      Tcl_ListObjAppendElement(interp, argList, Tcl_NewStringObj(paramPtr->name, -1));
    }
  }
  ov[0] = NULL;
  ov[1] = procNameObj;
  ov[2] = argList;
  ov[3] = AddPrefixToBody(INTERP body, 1, parsedParamPtr);

  /*fprintf(stderr, "NsfAddParameterProc define proc %s arglist '%s'\n",
    ObjStr(ov[1]), ObjStr(ov[2])); */

  result = Tcl_ProcObjCmd(0, interp, 4, ov);
  DECR_REF_COUNT(argList);
  DECR_REF_COUNT2("resultBody", ov[3]);

  if (result == TCL_OK) {
    /*
     * The shadowed proc was created successfully. Retrieve the
     * defined proc and set its namespace to the namespace of the stub
     * cmd
     */
    Tcl_Command procCmd = Tcl_GetCommandFromObj(interp, procNameObj);
    assert(procCmd);
    ((Command *)procCmd)->nsPtr = (Namespace *)cmdNsPtr;
    tcd->cmd = procCmd;
    NsfCommandPreserve(INTERP tcd->cmd);

  } else {
    /*
     * We could not define the shadowed proc. In this case, cleanup by
     * removing the stub cmd.
     */
    Tcl_DeleteCommandFromToken(interp, cmd);
  }

  Tcl_DStringFree(dsPtr);
  return result;
}

static int
ProcessMethodArguments(ParseContext *pcPtr, Tcl_Interp *interp,
                       NsfObject *object, int pushFrame, NsfParamDefs *paramDefs,
                       Tcl_Obj *methodNameObj, int objc, Tcl_Obj *CONST objv[]) {
  int result;
  CallFrame frame, *framePtr = &frame;

  if (object && pushFrame) {
    Nsf_PushFrameObj(interp, object, framePtr);
  }

  result = ArgumentParse(interp, objc, objv, object, methodNameObj,
                         paramDefs->paramsPtr, paramDefs->nrParams,
			 RUNTIME_STATE(interp)->doCheckArguments,
			 pcPtr);
#if 0
  {
    int i;
    fprintf(stderr, "after ArgumentParse\n");
    for (i = 0; i < pcPtr->objc-1; i++) {
      fprintf(stderr, "... pcPtr %p check [%d] obj %p refCount %d (%s) flags %.6x & %p\n",
	      pcPtr, i, pcPtr->objv[i], pcPtr->objv[i]->refCount, 
	      ObjStr(pcPtr->objv[i]), pcPtr->flags[i], &(pcPtr->flags[i]));
    }
  }
#endif

  if (object && pushFrame) {
    Nsf_PopFrameObj(interp, framePtr);
  }

  /*
   * Set objc of the parse context to the number of defined parameters.
   * pcPtr->objc and paramDefs->nrParams will be equivalent in cases
   * where argument values are passed to the call in absence of var
   * args ('args'). Treating "args is more involved (see below).
   */
  // TODO: do we need setting of  pcPtr->objc earlier? yyyy
  pcPtr->objc = paramDefs->nrParams + 1;

  if (result != TCL_OK) {
    return result;
  }

  if (pcPtr->varArgs) {
    /*
     * The last argument was "args".
     */
    int elts = objc - pcPtr->lastObjc;

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
      ParseContextExtendObjv(INTERP pcPtr, paramDefs->nrParams, elts-1, objv + 1 + pcPtr->lastObjc);
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
/**************************************************************************
 * End Definition of Parameter procs (Tcl Procs with Parameter handling)
 **************************************************************************/

/*
 *----------------------------------------------------------------------
 * ForwardCmdDeleteProc --
 *
 *    This Tcl_CmdDeleteProc is called, when a forward method is deleted
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Frees client data of the setter command.
 *
 *----------------------------------------------------------------------
 */

static void
ForwardCmdDeleteProc(ClientData clientData) {
  ForwardCmdClientData *tcd = (ForwardCmdClientData *)clientData;
  INTERP_MEMBER_GET(tcd)

  if (tcd->cmdName)     {DECR_REF_COUNT(tcd->cmdName);}
  if (tcd->subcommands) {DECR_REF_COUNT(tcd->subcommands);}
  if (tcd->onerror)     {DECR_REF_COUNT(tcd->onerror);}
  if (tcd->prefix)      {DECR_REF_COUNT(tcd->prefix);}
  if (tcd->args)        {DECR_REF_COUNT(tcd->args);}
  FREE(ForwardCmdClientData, tcd);
}


/*
 *----------------------------------------------------------------------
 * SetterCmdDeleteProc --
 *
 *    This Tcl_CmdDeleteProc is called, when a setter method is deleted
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Frees client data of the setter command.
 *
 *----------------------------------------------------------------------
 */

static void
SetterCmdDeleteProc(ClientData clientData) {
  SetterCmdClientData *setterClientData = (SetterCmdClientData *)clientData;
  INTERP_MEMBER_GET(setterClientData)

  if (setterClientData->paramsPtr) {
    ParamsFree(INTERP setterClientData->paramsPtr);
  }
  FREE(SetterCmdClientData, setterClientData);
}


/*
 *----------------------------------------------------------------------
 * AliasCmdDeleteProc --
 *
 *    This Tcl_CmdDeleteProc is called, when an alias method is deleted
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Frees client data of the setter command.
 *
 *----------------------------------------------------------------------
 */

static void
AliasCmdDeleteProc(ClientData clientData) {
  AliasCmdClientData *tcd = (AliasCmdClientData *)clientData;
  INTERP_MEMBER_GET(tcd)

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

  /*fprintf(stderr, "AliasCmdDeleteProc aliasedCmd %p\n", tcd->aliasedCmd);*/
  if (tcd->cmdName)     {DECR_REF_COUNT(tcd->cmdName);}
  if (tcd->aliasedCmd) {

#if defined(WITH_IMPORT_REFS)
    ImportRef *refPtr, *prevPtr = NULL;
    Command *aliasedCmd = (Command *)(tcd->aliasedCmd);

    /*fprintf(stderr, "AliasCmdDeleteProc aliasedCmd %p epoch %d refCount %d\n",
      aliasedCmd, Tcl_Command_cmdEpoch(tcd->aliasedCmd), aliasedCmd->refCount);*/
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
#endif
    NsfCommandRelease(INTERP tcd->aliasedCmd);
  }
  FREE(AliasCmdClientData, tcd);
}

/*
 *----------------------------------------------------------------------
 * GetMatchObject --
 *
 *    Helper method used by nsfAPI.h and the info methods to check if the the
 *    Tcl_Obj patternObj was provided and can be looked up. If this is the
 *    case, wild card matching etc. does not have to be performed, but just
 *    the properties of the object have to be tested.
 *
 * Results:
 *    0 or 1 or -1, potentially the matchObject (when 0 is returned)
 *    0: we have wild-card characters, iterate to get matches
 *    1: we have an existing object
 *   -1: we no wild-card characters and a non-existing object
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
GetMatchObject(Tcl_Interp *interp, Tcl_Obj *patternObj, Tcl_Obj *origObj,
                NsfObject **matchObject, CONST char **pattern) {
  if (patternObj) {
    *pattern = ObjStr(patternObj);
    if (TclObjIsNsfObject(interp, patternObj, matchObject)) {
      return 1;
    }
    if (patternObj == origObj && **pattern != ':') {
      return -1;
    }
  }
  return 0;
}


/*
 *----------------------------------------------------------------------
 * ForwardProcessOptions --
 *
 *    Process the options provided by the forward method and turn these into
 *    the ForwardCmdClientData structure.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Allocated and initialized ForwardCmdClientData
 *
 *----------------------------------------------------------------------
 */

static int
ForwardProcessOptions(Tcl_Interp *interp, Tcl_Obj *nameObj,
                       Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix,
                       int withObjframe, Tcl_Obj *withOnerror, int withVerbose,
                       Tcl_Obj *target, int objc, Tcl_Obj * CONST objv[],
                       ForwardCmdClientData **tcdPtr) {
  ForwardCmdClientData *tcd;
  int i, result = 0;

  tcd = NEW(ForwardCmdClientData);
  memset(tcd, 0, sizeof(ForwardCmdClientData));
  INTERP_MEMBER_SET(tcd, interp)
  
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
    *tcdPtr = tcd;
  } else {
    ForwardCmdDeleteProc(tcd);
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 * StripBodyPrefix --
 *
 *    Strip the prefix of the body, which might have been added by nsf.
 *
 * Results:
 *    The string of the body without the prefix.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static CONST char *
StripBodyPrefix(CONST char *body) {
  if (strncmp(body, "::nsf::__unset_unknown_args\n", 28) == 0) {
    body += 28;
  }
  return body;
}


/*
 *----------------------------------------------------------------------
 * AddSlotObjects --
 *
 *    Compute the slot objects (children of the slot container) for a provided
 *    object. The objects can be filtered via a pattern.
 *
 * Results:
 *    The function appends results to the provide listObj
 *
 * Side effects:
 *    Might add as well to the hash table to avoid duplicates
 *
 *----------------------------------------------------------------------
 */
static void
AddSlotObjects(Tcl_Interp *interp, NsfObject *parent, CONST char *prefix,
	       Tcl_HashTable *slotTablePtr,
	       int withSource, NsfClass *type, CONST char *pattern,
	       Tcl_Obj *listObj) {
  NsfObject *childObject, *slotContainerObject;
  Tcl_DString ds, *dsPtr = &ds;
  int fullQualPattern = (pattern && *pattern == ':');

  /*fprintf(stderr, "AddSlotObjects parent %s prefix %s\n", ObjectName(parent), prefix);*/

  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, ObjectName(parent), -1);
  Tcl_DStringAppend(dsPtr, prefix, -1);
  slotContainerObject = GetObjectFromString(interp, Tcl_DStringValue(dsPtr));

  if (slotContainerObject && slotContainerObject->nsPtr &&
      (slotContainerObject->flags & NSF_IS_SLOT_CONTAINER)) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr;
    Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(slotContainerObject->nsPtr);
    Tcl_Command cmd;
    int new;

    hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch);
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      char *key = Tcl_GetHashKey(cmdTablePtr, hPtr);

      if (slotTablePtr) {
	/*
	 * Check, if we have and entry with this key already processed. We
	 * never want to report shadowed entries.
	 */
	Tcl_CreateHashEntry(slotTablePtr, key, &new);
	if (!new) continue;
      }

      /*
       * Obtain the childObject
       */
      cmd = (Tcl_Command) Tcl_GetHashValue(hPtr);
      childObject = NsfGetObjectFromCmdPtr(cmd);
      
      /* 
       * Report just the already fully initialized slot objects, not the one
       * being right now created.
       */
      if (!childObject || (childObject->flags & NSF_INIT_CALLED) == 0) {
	continue;
      }

      /*
       * Check the pattern.
       */
      if (pattern) {
	int match;
	/*
	 * If the pattern looks like fully qualified, we match against the
	 * fully qualified name.
	 */
	match = fullQualPattern ?
	  Tcl_StringMatch(ObjectName(childObject), pattern) :
	  Tcl_StringMatch(key, pattern);
	
	if (!match) {
	  continue;
	}
      }

      /*
       * Check, if the entry is from the right type
       */
      if (type && !IsSubType(INTERP childObject->cl, type)) {
	continue;
      }

      /*
       * Add finaly the entry to the returned list.
       */
      Tcl_ListObjAppendElement(interp, listObj, childObject->cmdName);
    }
  }
  DSTRING_FREE(dsPtr);
}


static NsfClass *
FindCalledClass(Tcl_Interp *interp, NsfObject *object) {
  NsfCallStackContent *cscPtr = CallStackGetTopFrame0(interp);
  CONST char *methodName;
  Tcl_Command cmd;

  if (cscPtr->frameType == NSF_CSC_TYPE_PLAIN) {
    return cscPtr->cl;
  }
  if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER) {
    methodName = MethodName(cscPtr->filterStackEntry->calledProc);
  } else if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_MIXIN && object->mixinStack) {
    methodName = (char *)Tcl_GetCommandName(interp, cscPtr->cmdPtr);
  } else {
    return NULL;
  }
  if (object->nsPtr) {
    cmd = FindMethod(object->nsPtr, methodName);
    if (cmd) {
      /* we called an object specific method */
      return NULL;
    }
  }

  return SearchCMethod(INTERP object->cl, methodName, &cmd);
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

    /*fprintf(stderr, "EndOfChain? cmd=%p\n",*cmdPtr);*/
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

  /*fprintf(stderr, "... mixinstack %p => %p\n", object, object->mixinStack);*/

  if (object->mixinStack) {
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
   * When a mixin or filter chain reached its end, we have to check for
   * absolute method names and search the obj-specific methods as well.
   */
  if (endOfChain) {
    if (**methodNamePtr == ':') {
      *cmdPtr = Tcl_FindCommand(interp, *methodNamePtr, NULL, TCL_GLOBAL_ONLY);
      /* fprintf(stderr, "NEXT found abolute cmd %s => %p\n", *methodNamePtr, *cmdPtr); */
    } else if (object->nsPtr) {
      *cmdPtr = FindMethod(object->nsPtr, *methodNamePtr);
    } else {
      *cmdPtr = NULL;
    }
  } else {
    *cmdPtr = NULL;
  }

  /*fprintf(stderr, "NEXT methodName %s *clPtr %p %s *cmd %p\n",
   *methodNamePtr, *clPtr, ClassName((*clPtr)), *cmdPtr); */

  if (!*cmdPtr) {
    NsfClasses *pl;

    for (pl = ComputeOrder(INTERP object->cl, object->cl->order, Super); *clPtr && pl; pl = pl->nextPtr) {
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
      MEM_COUNT_ALLOC("nextArgumentVector", nobjv);
      /*
       * copy the ensemble path name
       */
      memcpy((char *)nobjv, cscPtr->objv, sizeof(Tcl_Obj *) * methodNameLength);

     } else {
      methodNameLength = 1;
      nobjc = objc + methodNameLength;
      nobjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * nobjc);
      MEM_COUNT_ALLOC("nextArgumentVector", nobjv);
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

    if (cscPtr->frameType == NSF_CSC_TYPE_INACTIVE_FILTER) {
      cscPtr->frameType = NSF_CSC_TYPE_ACTIVE_FILTER;
    } else if (cscPtr->frameType == NSF_CSC_TYPE_INACTIVE_MIXIN) {
      cscPtr->frameType = NSF_CSC_TYPE_ACTIVE_MIXIN;
    }
  }

  if (nobjv) {
    DECR_REF_COUNT(nobjv[0]);
    MEM_COUNT_FREE("nextArgumentVector", nobjv);
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
 *    The function is called with a final argument vector and searches for a
 *    possibly shadowed method. If a target method is found, this dispatcher
 *    function updates the continuation context (filter flags etc.), invokes
 *    upon the target method, and performs a cleanup.
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    The invoked method might produce side effects. Also, the interp's unknown
 *    state may be modified.
 *
 *----------------------------------------------------------------------
 */
static int
NextSearchAndInvoke(Tcl_Interp *interp, CONST char *methodName,
		    int objc, Tcl_Obj *CONST objv[],
		    NsfCallStackContent *cscPtr,
		    int freeArgumentVector) {
  Tcl_Command cmd, currentCmd = NULL;
  int result, isMixinEntry = 0, isFilterEntry = 0,
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

  /*fprintf(stderr, "NEXT search on %s.%s cl %p cmd %p endOfFilterChain %d result %d IS OK %d\n",
    ObjectName(object), methodName, cl, cmd, endOfFilterChain, result, (result == TCL_OK));*/

  if (unlikely(result != TCL_OK)) {
    goto next_search_and_invoke_cleanup;
  }

#if 0
  Tcl_ResetResult(interp); /* needed for bytecode support */
#endif
  if (cmd) {
    int frameType = NSF_CSC_TYPE_PLAIN;
    /*
     * change mixin state
     */
    if (object->mixinStack) {
      if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_MIXIN) {
        cscPtr->frameType = NSF_CSC_TYPE_INACTIVE_MIXIN;
      }
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
	return MethodDispatch(object, interp, objc, objv, cmd,
			      object, cl, methodName, frameType, flags);
      } else {
	result = MethodDispatch(object, interp, objc, objv, cmd,
				object, cl, methodName, frameType, flags);
      }
    }
#else
    result = MethodDispatch(object, interp, objc, objv, cmd,
			    object, cl, methodName, frameType, 0);
#endif
  } else if (result == TCL_OK) {
    NsfCallStackContent *topCscPtr = NULL;
    int isLeafNext;
    
    /*
     * We could not find a cmd, yet the dispatch attempt did not result
     * in an error. This means that we find ourselves in either of three
     * situations at this point:
     *
     * 1) A next cmd (NsfNextCmd()) at the end of a filter chain: Dispatch to
     * unknown as there is no implementation for the requested selector
     * available.  2) A next cmd from within a leaf submethod (a "leaf next"):
     * Remain silent, do not dispatch to unknown.  3) MethodDispatchCsc()
     * realises the actual "ensemble next": Dispatch to unknown, the
     * requested sub-selector is not resolvable to a cmd.
     *
     * For the cases 1) and 3), set the interp's unknown flag signalling to
     * higher levels (e.g., in MethodDispatchCsc(), in NsfNextCmd()) the need
     * for dispatching to unknown.
     */
    
    topCscPtr = CallStackGetTopFrame(interp, NULL);
    assert(topCscPtr);
    
    /* case 2 */
    isLeafNext = (cscPtr != topCscPtr) && (topCscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) && 
      (topCscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE) == 0;

    /*fprintf(stderr, "--- no cmd, csc %p frameType %.6x callType %.6x endOfFilterChain %d NSF_CSC_CALL_IS_ENSEMBLE %d NSF_CSC_TYPE_ENSEMBLE % d NSF_CSC_CALL_IS_NEXT %d rst->unknown %d isLeafNext %d\n", cscPtr, cscPtr->frameType, cscPtr->flags, endOfFilterChain, (cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE) != 0, (cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) != 0, (cscPtr->flags & NSF_CSC_CALL_IS_NEXT) != 0, rst->unknown, isLeafNext);*/
    
    rst->unknown = /* case 1 */ endOfFilterChain || 
      /* case 3 */ (!isLeafNext && (cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE)); 

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
static int
NsfNextObjCmd(ClientData UNUSED(clientData), Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  int freeArgumentVector, nobjc, result;
  NsfCallStackContent *cscPtr;
  CONST char *methodName;
  Tcl_Obj **nobjv;

  if (objc < 2) {
    /* No arguments were provided */
    objc = 0;
  } else {
    /* in case --noArgs is used, remove the flag and provide an empty argument list */
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
 *----------------------------------------------------------------------
 * FindSelfNext --
 *
 *    This function is called via [current next] to set the result of the
 *    interp to the method which would be called by [next]. If there are more
 *    shadowed methods along the precedence path, it sets the result of the
 *    next method in form of a method handle. If there are no more shadowed
 *    procs, the result is set to empty.
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    Set Tcl result.
 *
 *----------------------------------------------------------------------
 */

static int
FindSelfNext(Tcl_Interp *interp) {
  NsfCallStackContent *cscPtr = CallStackGetTopFrame0(interp);
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
    Tcl_SetObjResult(interp, MethodHandleObj(cl ? (NsfObject *)cl : object,
					     cl == NULL, methodName));
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 * ComputeLevelObj --
 *
 *    This function comptes a fresh Tcl_Obj refering to the interp level. The
 *    caller has to care about freeing the returned Tcl_Obj.
 *
 * Results:
 *    Tcl_Obj *
 *
 * Side effects:
 *    Allocates a new Tcl_Obj
 *
 *----------------------------------------------------------------------
 */

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
  if (objc < 2) return NsfPrintError(interp, "wrong # of args for K");

  Tcl_SetObjResult(interp, objv[1]);
  return TCL_OK;
  }
*/

/*
 * object creation & destruction
 */

static int
UnsetInAllNamespaces(Tcl_Interp *interp, Tcl_Namespace *nsPtr, CONST char *name) {
  int rc = 0;
  fprintf(stderr, "### UnsetInAllNamespaces variable '%s', current namespace '%s'\n",
          name, nsPtr ? nsPtr->fullName : "NULL");

  if (nsPtr) {
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr = Tcl_FirstHashEntry(Tcl_Namespace_childTablePtr(nsPtr), &search);
    Tcl_Var *varPtr;
    int result;

    varPtr = (Tcl_Var *) Tcl_FindNamespaceVar(interp, name, nsPtr, 0);
    /*fprintf(stderr, "found %s in %s -> %p\n", name, nsPtr->fullName, varPtr);*/
    if (varPtr) {
      Tcl_DString dFullname, *dsPtr = &dFullname;
      Tcl_DStringInit(dsPtr);
      Tcl_DStringAppend(dsPtr, "unset ", -1);
      DStringAppendQualName(dsPtr, nsPtr, name);
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
      Tcl_Namespace *childNsPtr = (Tcl_Namespace *) Tcl_GetHashValue(entryPtr);
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
        Tcl_Namespace *nsPtr = Tcl_GetCurrentNamespace(interp);
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
NsfUnsetTrace(ClientData clientData, Tcl_Interp *interp,
	      CONST char *UNUSED(name), CONST char *UNUSED(name2), int flags)
{
  Tcl_Obj *objPtr = (Tcl_Obj *)clientData;
  NsfObject *object;
  char *resultMsg = NULL;

  /*fprintf(stderr, "NsfUnsetTrace %s flags %.4x %.4x\n", name, flags,
    flags & TCL_INTERP_DESTROYED); **/

  if ((flags & TCL_INTERP_DESTROYED) == 0) {
    if (GetObjectFromObj(interp, objPtr, &object) == TCL_OK) {
      Tcl_Obj *savedResultObj = Tcl_GetObjResult(interp); /* save the result */

      INCR_REF_COUNT(savedResultObj);

      /* clear variable, destroy is called from trace */
      if (object->opt && object->opt->volatileVarName) {
        object->opt->volatileVarName = NULL;
      }

      if (DispatchDestroyMethod(interp, object, 0) != TCL_OK) {
        resultMsg = "Destroy for volatile object failed";
      } else {
        resultMsg = "No nsf Object passed";
      }

      Tcl_SetObjResult(interp, savedResultObj);  /* restore the result */
      DECR_REF_COUNT(savedResultObj);
    }
    DECR_REF_COUNT(objPtr);
  } else {
    /*fprintf(stderr, "omitting destroy on %s %p\n", name);*/
  }
  return resultMsg;
}

/*
 * bring an object into a state, as after initialization
 */
/*
 *----------------------------------------------------------------------
 * CleanupDestroyObject --
 *
 *    Perform cleanup of object; after the function is executed, the object is
 *    in the same fresh state as after initialization.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Possibly freeing memory.
 *
 *----------------------------------------------------------------------
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
#if defined(NSF_WITH_ASSERTIONS)
    AssertionRemoveStore(INTERP opt->assertions);
    opt->assertions = NULL;
#endif

    if (!softrecreate) {
      /*
       *  Remove this object from all per object mixin lists and clear the mixin list
       */
      RemoveFromObjectMixinsOf(INTERP object->id, opt->mixins);

      CmdListRemoveList(INTERP &opt->mixins, GuardDel);
      CmdListRemoveList(INTERP &opt->filters, GuardDel);
      FREE(NsfObjectOpt, opt);
      opt = object->opt = 0;
    }
  }

  object->flags &= ~NSF_MIXIN_ORDER_VALID;
  if (object->mixinOrder)  MixinResetOrder(INTERP object);
  object->flags &= ~NSF_FILTER_ORDER_VALID;
  if (object->filterOrder) FilterResetOrder(INTERP object);
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
  if (NsfObjectIsClass((NsfObject *)clientData)) {
    PrimitiveCDestroy(clientData);
  } else {
    PrimitiveODestroy(clientData);
  }
}

static void
TclDeletesObject(ClientData clientData) {
  NsfObject *object = (NsfObject *)clientData;
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
  NsfObject *object = (NsfObject *)clientData;
  Tcl_Interp *interp;

  if (!object || !object->teardown) return;

  /*fprintf(stderr, "****** PrimitiveODestroy %p cmd %p flags %.6x\n",
    object, object->id, object->flags);*/
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
  {Command *cmdPtr = object->id;
  fprintf(stderr, "  physical delete of %p id=%p (cmd->refCount %d) destroyCalled=%d '%s'\n",
          object, object->id, cmdPtr->refCount, (object->flags & NSF_DESTROY_CALLED), ObjectName(object));
  }
#endif
  CleanupDestroyObject(interp, object, 0);

  while (object->mixinStack)
    MixinStackPop(INTERP object);

  while (object->filterStack)
    FilterStackPop(INTERP object);

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
  NsfCleanupObject(INTERP object, "PrimitiveODestroy");

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

  //fprintf(stderr, "nsPtr %p\n", nsPtr);
  //fprintf(stderr, "nsPtr->flags %.6x\n", nsPtr ? (((Namespace *)nsPtr)->flags) : 0);

  if (nsPtr && (((Namespace *)nsPtr)->flags & NS_DYING)) {
    Namespace *dummy1Ptr, *dummy2Ptr, *nsPtr1 = (Namespace *)nsPtr;
    const char *dummy;
    TclGetNamespaceForQualName(interp, name,
			       NULL, TCL_GLOBAL_ONLY|TCL_FIND_ONLY_NS,
			       &nsPtr1, &dummy1Ptr, &dummy2Ptr, &dummy);
    nsPtr = (Tcl_Namespace *)nsPtr1;
    /*fprintf(stderr, "PrimitiveOInit %p calls TclGetNamespaceForQualName with %s => %p given %p object->nsPtr %p\n",
	    object, name,
	    nsPtr, nsPtr, object->nsPtr);*/
  }

  if (nsPtr) {
    NsfNamespaceInit(nsPtr);
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
  NsfObject *object = (NsfObject *)ckalloc(sizeof(NsfObject));
  CONST char *nameString = ObjStr(nameObj);
  Tcl_Namespace *nsPtr;

  /*fprintf(stderr, "PrimitiveOCreate %s parentNs %p\n", nameString, parentNsPtr);*/

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

  nsPtr = NSCheckNamespace(interp, nameString, parentNsPtr);
  if (nsPtr) {
    NSNamespacePreserve(INTERP nsPtr);
  }
#if defined(NRE)
  object->id = Tcl_NRCreateCommand(interp, nameString, NsfObjDispatch, NsfObjDispatch,
				   object, TclDeletesObject);
#else
  object->id = Tcl_CreateObjCommand(interp, nameString, NsfObjDispatch,
				    object, TclDeletesObject);
#endif

  /*fprintf(stderr, "cmd alloc %p %d (%s)\n", object->id,
    Tcl_Command_refCount(object->id), nameString);*/

  PrimitiveOInit(object, interp, nameString, nsPtr, cl);
  if (nsPtr) {
    NSNamespaceRelease(INTERP nsPtr);
  }

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
      if (GetClassFromObj(interp, resultObj, &resultClass, 0) != TCL_OK) {
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

  /*fprintf(stderr, "CleanupDestroyClass %p %s (ismeta=%d) softrecreate=%d, recreate=%d, %p\n",
	  cl, ClassName(cl), IsMetaClass(interp, cl, 1),
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
    RemoveFromClassMixinsOf(INTERP clopt->id, clopt->classmixins);

    CmdListRemoveList(INTERP &clopt->classmixins, GuardDel);
    /*MixinInvalidateObjOrders(interp, cl);*/

    CmdListRemoveList(INTERP &clopt->classfilters, GuardDel);
    /*FilterInvalidateObjOrders(interp, cl);*/

    if (!recreate) {
      /*
       *  Remove this class from all mixin lists and clear the isObjectMixinOf list
       */
      RemoveFromObjectMixins(INTERP clopt->id, clopt->isObjectMixinOf);
      CmdListRemoveList(INTERP &clopt->isObjectMixinOf, GuardDel);

      /*
       *  Remove this class from all class mixin lists and clear the
       *  isClassMixinOf list
       */
      RemoveFromClassmixins(INTERP clopt->id, clopt->isClassMixinOf);
      CmdListRemoveList(INTERP &clopt->isClassMixinOf, GuardDel);
    }

    /*
     * Remove dependent filters of this class from all subclasses
     */
    FilterRemoveDependentFilterCmds(INTERP cl, cl);

#if defined(NSF_WITH_ASSERTIONS)
    AssertionRemoveStore(INTERP clopt->assertions);
    clopt->assertions = NULL;
#endif

#ifdef NSF_OBJECTDATA
    NsfFreeObjectData(cl);
#endif
  }

  NSCleanupNamespace(interp, cl->nsPtr);
  NSDeleteChildren(interp, cl->nsPtr);

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
      Tcl_HashTable *instanceTablePtr = &cl->instances;

      for (hPtr = Tcl_FirstHashEntry(instanceTablePtr, &hSrch); hPtr;
	   hPtr = Tcl_NextHashEntry(&hSrch)) {
        NsfObject *inst = (NsfObject *)Tcl_GetHashKey(instanceTablePtr, hPtr);
        /*fprintf(stderr, "    inst %p %s flags %.6x id %p baseClass %p %s\n",
	  inst, ObjectName(inst), inst->flags, inst->id, baseClass, ClassName(baseClass));*/
        if (inst && inst != (NsfObject *)cl && !(inst->flags & NSF_DURING_DELETE) /*inst->id*/) {
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
  FlushPrecedencesOnSubclasses(INTERP cl);
  while (cl->super) (void)RemoveSuper(INTERP cl, cl->super->cl);

  if (!softrecreate) {
    /*
     * flush all caches, unlink superclasses
     */

    while (cl->sub) {
      NsfClass *subClass = cl->sub->cl;
      (void)RemoveSuper(INTERP subClass, cl);
      /*
       * If there are no more super classes add the Object
       * class as superclasses
       * -> don't do that for Object itself!
       */
      if (subClass->super == 0 && (cl->object.flags & NSF_IS_ROOT_CLASS) == 0) {
	/* fprintf(stderr,"subClass %p %s baseClass %p %s\n",
	   cl, ClassName(cl), baseClass, ClassName(baseClass)); */
	AddSuper(INTERP subClass, baseClass);
      }
    }
    /*(void)RemoveSuper(INTERP cl, cl->super->cl);*/
  }

}

/*
 *----------------------------------------------------------------------
 * CleanupInitClass --
 *
 *    Basic initialization of a class, setting namespace, super- and
 *    sub-classes, and setup optionally instances table.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Makes a class structure usuable.
 *
 *----------------------------------------------------------------------
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
   * Record, that cl is a class and set its namespace
   */
  NsfObjectSetClass((NsfObject *)cl);
  cl->nsPtr = nsPtr;

  if (!softrecreate) {
    /*
     * Subclasses are preserved during recreate, superclasses not (since the
     * creation statement defined the superclass, might be different the
     * second time)
     */
    cl->sub = NULL;
  }
  cl->super = NULL;

  /* Look for a configured default superclass */
  defaultSuperclass = DefaultSuperClass(interp, cl, cl->object.cl, 0);
  if (cl != defaultSuperclass) {
    AddSuper(INTERP cl, defaultSuperclass);
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
  NsfClass *cl = (NsfClass *)clientData;
  NsfObject *object = (NsfObject *)clientData;
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
  /* fprintf(stderr, "PrimitiveCDestroy %s flags %.6x\n", ObjectName(object), object->flags);*/

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
  PrimitiveODestroy(clientData);

  /*fprintf(stderr, "primitive cdestroy calls deletenamespace for obj %p, nsPtr %p flags %.6x\n",
    cl, saved, ((Namespace *)saved)->flags);*/
  Nsf_DeleteNamespace(interp, saved);
  /*fprintf(stderr, "primitive cdestroy %p DONE\n", cl);*/
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
  nsPtr = NSGetFreshNamespace(interp, &cl->object, name);
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
  NsfClass *cl = (NsfClass *)ckalloc(sizeof(NsfClass));
  Tcl_Namespace *nsPtr;
  CONST char *nameString = ObjStr(nameObj);
  NsfObject *object = (NsfObject *)cl;

  /* fprintf(stderr, "PrimitiveCCreate %s parentNs %p\n", nameString, parentNsPtr); */

#if defined(NSFOBJ_TRACE)
  fprintf(stderr, "CKALLOC Class %p %s\n", cl, nameString);
#endif

  memset(cl, 0, sizeof(NsfClass));
  MEM_COUNT_ALLOC("NsfObject/NsfClass", cl);

  /* pass object system from meta class */
  if (class) {
    cl->osPtr = class->osPtr;
  }

  assert(isAbsolutePath(nameString));
  /*
    fprintf(stderr, "Class alloc %p '%s'\n", cl, nameString);
  */
  nsPtr = NSCheckNamespace(interp, nameString, parentNsPtr);
  if (nsPtr) {
    NSNamespacePreserve(INTERP nsPtr);
  }
#if defined(NRE)
  object->id = Tcl_NRCreateCommand(interp, nameString, NsfObjDispatch, NsfObjDispatch,
				   cl, TclDeletesObject);
#else
  object->id = Tcl_CreateObjCommand(interp, nameString, NsfObjDispatch,
				    cl, TclDeletesObject);
#endif

  PrimitiveOInit(object, interp, nameString, nsPtr, class);
  if (nsPtr) {
    NSNamespaceRelease(INTERP nsPtr);
  }
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
 *----------------------------------------------------------------------
 * DoObjInitialization --
 *
 *    Perform the object initialization: first call "configure" and the
 *    constructor "init", if not called already from configure. The function
 *    will make sure that the called methods do not change the result passed
 *    into this function.
 *
 * Results:
 *    Tcl return code
 *
 * Side effects:
 *    Indirect effects by calling Tcl code
 *
 *----------------------------------------------------------------------
 */
static int
DoObjInitialization(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]) {
  Tcl_Obj *methodObj, *savedObjResult;
  int result;

  /*
   * Save the result we have so far to return it in case of success 
   */
  savedObjResult = Tcl_GetObjResult(interp);
  INCR_REF_COUNT(savedObjResult);

  /*
   * clear INIT_CALLED flag
   */
  object->flags &= ~NSF_INIT_CALLED;
  /*
   * Make sure, the object survives initialization; the initcmd might
   * destroy it.
   */
  NsfObjectRefCountIncr(object);

  /*
   * Call configure method
   */
  if (CallDirectly(interp, object, NSF_o_configure_idx, &methodObj)) {
    ALLOC_ON_STACK(Tcl_Obj*, objc, tov);
    memcpy(tov+1, objv+2, sizeof(Tcl_Obj *)*(objc-2));
    /* the provided name of the method is just for error reporting */
    tov[0] = methodObj ? methodObj : NsfGlobalObjs[NSF_CONFIGURE];
    result = NsfOConfigureMethod(interp, object, objc-1, tov);
    FREE_ON_STACK(Tcl_Obj*, tov);
  } else {
    result = CallMethod(object, interp, methodObj, objc, objv+2, NSF_CSC_IMMEDIATE);
  }

  if (result == TCL_OK) {
    /*
     * Call constructor when needed
     */
    if (!(object->flags & (NSF_INIT_CALLED|NSF_DESTROY_CALLED))) {
      result = DispatchInitMethod(interp, object, 0, NULL, 0);
    }

    if (result == TCL_OK) {
      Tcl_SetObjResult(interp, savedObjResult);
    }
  }

  NsfCleanupObject(INTERP object, "obj init");
  DECR_REF_COUNT(savedObjResult);
  return result;
}


static int
HasMetaProperty(NsfClass *cl) {
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
  if (HasMetaProperty(cl)) {
    return 1;
  }

  /* is the class a subclass of a meta-class? */
  for (pl = ComputeOrder(INTERP cl, cl->order, Super); pl; pl = pl->nextPtr) {
    if (HasMetaProperty(pl->cl)) {
      return 1;
    }
  }

  if (withMixins) {
    NsfClasses *checkList = NULL, *mixinClasses = NULL, *mc;
    int hasMCM = 0;

    /* has the class metaclass mixed in? */
    NsfClassListAddPerClassMixins(interp, cl, &mixinClasses, &checkList);

    for (mc = mixinClasses; mc; mc = mc->nextPtr) {
      if (IsMetaClass(interp, mc->cl, 0)) {
	hasMCM = 1;
	break;
      }
    }
    NsfClassListFree(INTERP mixinClasses);
    NsfClassListFree(INTERP checkList);
    /*fprintf(stderr, "has MC returns %d, mixinClasses = %p\n",
      hasMCM, mixinClasses);*/

    return hasMCM;
  } else {
    return 0;
  }

}

static int
IsSubType(INTERP_DECL NsfClass *subcl, NsfClass *cl) {

  assert(cl && subcl);

  if (cl != subcl) {
    return NsfClassListFind(ComputeOrder(INTERP subcl, subcl->order, Super), cl) != NULL;
  }
  return 1;
}

static int
HasMixin(Tcl_Interp *interp, NsfObject *object, NsfClass *cl) {

  if (!(object->flags & NSF_MIXIN_ORDER_VALID)) {
    MixinComputeDefined(interp, object);
  }
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
        Var *linkPtr = TclVarValue(Var, varPtr, linkPtr);
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

/*
 * obj/cl ClientData setter/getter
 */
extern void
NsfSetObjClientData(Tcl_Interp *interp, Nsf_Object *object1, ClientData data) {
  NsfObject *object = (NsfObject *) object1;
  NsfObjectOpt *opt = NsfRequireObjectOpt(INTERP object);
  opt->clientData = data;
}
extern ClientData
NsfGetObjClientData(Tcl_Interp *interp, Nsf_Object *object1) {
  NsfObject *object = (NsfObject *) object1;
  return (object && object->opt) ? object->opt->clientData : NULL;
}
extern void
NsfSetClassClientData(Tcl_Interp *interp, Nsf_Class *cli, ClientData data) {
  NsfClass *cl = (NsfClass *) cli;
  NsfRequireClassOpt(INTERP cl);
  cl->opt->clientData = data;
}
extern ClientData
NsfGetClassClientData(Tcl_Interp *interp, Nsf_Class *cli) {
  NsfClass *cl = (NsfClass *) cli;
  return (cl && cl->opt) ? cl->opt->clientData : NULL;
}

/*
 *----------------------------------------------------------------------
 * SetInstVar --
 *
 *    Set an instance variable of the specified object to the given value.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Set instance variable.
 *
 *----------------------------------------------------------------------
 */
static int
SetInstVar(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *nameObj, Tcl_Obj *valueObj) {
  CallFrame frame, *framePtr = &frame;
  Tcl_Obj *resultObj;
  int flags;

  assert(object);
  flags = (object->nsPtr) ? TCL_LEAVE_ERR_MSG|TCL_NAMESPACE_ONLY : TCL_LEAVE_ERR_MSG;
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

/*
 *----------------------------------------------------------------------
 * SetInstArray --
 *
 *    Set an instance variable array of the specified object to the given
 *    value. This function performs essentially an "array set" or "array get"
 *    operation.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Set instance variable.
 *
 *----------------------------------------------------------------------
 */
static int
SetInstArray(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *arrayNameObj, Tcl_Obj *valueObj) {
  CallFrame frame, *framePtr = &frame;
  int result;
  Tcl_Obj *ov[4];

  assert(object);
  //flags = (object->nsPtr) ? TCL_LEAVE_ERR_MSG|TCL_NAMESPACE_ONLY : TCL_LEAVE_ERR_MSG;
  Nsf_PushFrameObj(interp, object, framePtr);
  
  ov[0] = NsfGlobalObjs[NSF_ARRAY];
  ov[2] = arrayNameObj;

  INCR_REF_COUNT(arrayNameObj);
  if (valueObj == NULL) {
    /*
     * perform an array get
     */
    ov[1] = NsfGlobalObjs[NSF_GET];
    result = Tcl_EvalObjv(interp, 3, ov, 0);
  } else {
    /*
     * perform an array get
     */
    ov[1] = NsfGlobalObjs[NSF_SET];
    ov[3] = valueObj;
    INCR_REF_COUNT(valueObj);
    result = Tcl_EvalObjv(interp, 4, ov, 0);
    DECR_REF_COUNT(valueObj);
  }
  DECR_REF_COUNT(arrayNameObj);
  Nsf_PopFrameObj(interp, framePtr);

  return result;
}


/*
 *----------------------------------------------------------------------
 * UnsetInstVar --
 *
 *    Unset an instance variable of the specified object.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Variable unset.
 *
 *----------------------------------------------------------------------
 */
static int
UnsetInstVar(Tcl_Interp *interp, int withNocomplain, NsfObject *object, CONST char *name) {
  CallFrame frame, *framePtr = &frame;
  int flags, result;

  assert(object);
  flags = withNocomplain ? 0 : TCL_LEAVE_ERR_MSG;
  if (object->nsPtr) {flags |= TCL_NAMESPACE_ONLY;}

  Nsf_PushFrameObj(interp, object, framePtr);
  result = Tcl_UnsetVar2(interp, name, NULL, flags);
  Nsf_PopFrameObj(interp, framePtr);

  return withNocomplain ? TCL_OK : result;
}

/*
 *----------------------------------------------------------------------
 * NsfSetterMethod --
 *
 *    This Tcl_ObjCmdProc is called, when a setter is invoked. A setter is a
 *    method that access the same-named inance variable. If the setter is
 *    called without arguments, it returns the values, if it is called with
 *    one argument, the argument is used as new value.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Can set an instance variable.
 *
 *----------------------------------------------------------------------
 */
static int
NsfSetterMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  SetterCmdClientData *cd = (SetterCmdClientData *)clientData;
  NsfObject *object = cd->object;

  if (objc > 2) return NsfObjWrongArgs(interp, "wrong # args", object->cmdName, objv[0], "?value?");
  if (!object) return NsfDispatchClientDataError(interp, clientData, "object", ObjStr(objv[0]));

  if (cd->paramsPtr && objc == 2) {
    Tcl_Obj *outObjPtr;
    int result, flags = 0;
    ClientData checkedData;

    result = ArgumentCheck(interp, objv[1], cd->paramsPtr,
			   RUNTIME_STATE(interp)->doCheckArguments,
			   &flags, &checkedData, &outObjPtr);

    if (result == TCL_OK) {
      result = SetInstVar(interp, object, objv[0], outObjPtr);
    }

    if (flags & NSF_PC_MUST_DECR) {
      DECR_REF_COUNT2("valueObj", outObjPtr);
    }
    return result;

  } else {
    return SetInstVar(interp, object, objv[0], objc == 2 ? objv[1] : NULL);
  }
}


/*
 *----------------------------------------------------------------------
 * ForwardArg --
 *
 *    This function is a helper function of NsfForwardMethod() and processes a
 *    single entry (ForwardArgObj) of the forward spec. Essentially, it
 *    performs the percent substitution of the forward spec.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Updates the provided output arguments.
 *
 *----------------------------------------------------------------------
 */

static int
ForwardArg(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
           Tcl_Obj *ForwardArgObj, ForwardCmdClientData *tcd, Tcl_Obj **out,
           Tcl_Obj **freeList, int *inputArg, int *mapvalue,
           int firstPosArg, int *outputincr) {
  CONST char *ForwardArgString = ObjStr(ForwardArgObj), *p;
  int totalargs = objc + tcd->nr_args - 1;
  char c = *ForwardArgString;

  /*
   * Per default every ForwardArgString from the processed list corresponds to
   * exactly one ForwardArgString in the computed final list
   */
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
    /*
     * In case we address from the end, we reduct further to distinguish from -1 (void)
     */
    if (pos<0) pos--;
    /*fprintf(stderr, "remainder = '%s' pos = %ld\n", remainder, pos);*/
    *mapvalue = pos;
    ForwardArgString = remainder;
    c = *ForwardArgString;
  }

  if (c == '%') {
    Tcl_Obj *list = NULL, **listElements;
    int nrArgs = objc-1, nrPosArgs = objc-firstPosArg, nrElements = 0;
    char c1, *firstActualArgument = nrArgs>0 ? ObjStr(objv[1]) : NULL;

    c = *++ForwardArgString;
    c1 = *(ForwardArgString+1);

    if (c == 's' && !strcmp(ForwardArgString, "self")) {
      *out = tcd->object->cmdName;
    } else if ((c == 'p' && !strcmp(ForwardArgString, "proc"))
	       || (c == 'm' && !strcmp(ForwardArgString, "method"))
	       ) {
      CONST char *methodName = ObjStr(objv[0]);
      /*
       * if we dispatch a method via ".", we do not want to see the "." in the
       * %proc, e.g. for the interceptor slots (such as mixin, ...)
       */
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
        /*
	 * Insert default subcommand depending on number of arguments.
	 */
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
      int insertRequired, done = 0;

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
	int i;
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
        /*
	 * We have a flag in the actual arguments that does not match.  We
         * proceed to the actual arguments without dashes.
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
          /*
	   * No match, but insert of flag is required
	   */
          /*fprintf(stderr, "no match, but insert of %s required\n", firstElementString);*/
          *out = Tcl_NewStringObj(firstElementString, -1);
          *outputincr = 1;
          goto add_to_freelist;
        } else {
          /*
	   * No match, no insert of flag required, we skip the forwarder
           * option and output nothing
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
      /*
       * Evaluate the given command
       */
      int result;
      /*fprintf(stderr, "evaluating '%s'\n", ForwardArgString);*/
      if ((result = Tcl_EvalEx(interp, ForwardArgString, -1, 0)) != TCL_OK) {
        return result;
      }
      *out = Tcl_DuplicateObj(Tcl_GetObjResult(interp));
      /*fprintf(stderr, "result = '%s'\n", ObjStr(*out));*/
      goto add_to_freelist;
    }
  } else {
    if (p == ForwardArgString) {
      *out = ForwardArgObj;
    } else {
      Tcl_Obj *newarg = Tcl_NewStringObj(ForwardArgString, -1);
      *out = newarg;
      goto add_to_freelist;
    }
  }
  return TCL_OK;

 add_to_freelist:
  if (!*freeList) {
    *freeList = Tcl_NewListObj(1, out);
    INCR_REF_COUNT2("freeList", *freeList);
  } else {
    Tcl_ListObjAppendElement(interp, *freeList, *out);
  }
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * CallForwarder --
 *
 *    Invoke the method to which the forwarder points. This function receives
 *    the already transformed argument vector, calls the method and performs
 *    error handling.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Maybe through the invoked command.
 *
 *----------------------------------------------------------------------
 */

static int
CallForwarder(ForwardCmdClientData *tcd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  int result;
  NsfObject *object = tcd->object;
  CallFrame frame, *framePtr = &frame;

  tcd->object = NULL;

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
  } else if (TclObjIsNsfObject(interp, tcd->cmdName, &object)) {
    /*fprintf(stderr, "CallForwarder NsfObjDispatch object %s, objc=%d\n",
      ObjStr(tcd->cmdName), objc);*/
    if (objc > 1) {
      result = ObjectDispatch(object, interp, objc, objv, NSF_CSC_IMMEDIATE);
    } else {
      result = DispatchDefaultMethod(interp, object, objv[0], NSF_CSC_IMMEDIATE);
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

/*
 *----------------------------------------------------------------------
 * NsfForwardMethod --
 *
 *    This Tcl_ObjCmdProc is called, when a forwarder is invoked. It performs
 *    argument substitution through ForwardArg() and calls finally the method,
 *    to which the call was forwarded via CallForwarder().
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Maybe through the invoked command.
 *
 *----------------------------------------------------------------------
 */

static int
NsfForwardMethod(ClientData clientData, Tcl_Interp *interp,
		   int objc, Tcl_Obj *CONST objv[]) {
  ForwardCmdClientData *tcd = (ForwardCmdClientData *)clientData;
  int result, inputArg = 1;

  if (!tcd || !tcd->object) {
    return NsfDispatchClientDataError(interp, tcd, "object", "forwarder");
  }

  /*
   * First, we handle two short cuts for simple cases
   */

  if (tcd->passthrough) {
    /*
     * This is set for early binding. This means, that the cmd is already
     * resolved, we have to care only for objscope.
     */
    return CallForwarder(tcd, interp, objc, objv);

  } else if (!tcd->args && *(ObjStr(tcd->cmdName)) != '%') {
    /*
     * we have have no args, therefore we have only to replace the method name
     * with the given cmd name.
     */
    ALLOC_ON_STACK(Tcl_Obj*, objc, ov);
    /*fprintf(stderr, "+++ forwardMethod must subst \n");*/
    memcpy(ov, objv, sizeof(Tcl_Obj *)*objc);
    ov[0] = tcd->cmdName;
    result = CallForwarder(tcd, interp, objc, ov);
    FREE_ON_STACK(Tcl_Obj *, ov);
    return result;

  } else {
    Tcl_Obj **ov, *freeList = NULL;
    int j, outputincr, outputArg = 0, firstPosArg=1,
      totalargs = objc + tcd->nr_args + 3;

    ALLOC_ON_STACK(Tcl_Obj*, totalargs, OV);
    ALLOC_ON_STACK(int, totalargs, objvmap);
    /*fprintf(stderr, "+++ forwardMethod standard case, allocated %d args\n", totalargs);*/

    ov = &OV[1];
    if (tcd->needobjmap) {
      memset(objvmap, -1, sizeof(int)*totalargs);
    }

    /*
     * The first argument is always the command, to which we forward
     */
    if ((result = ForwardArg(interp, objc, objv, tcd->cmdName, tcd,
                             &ov[outputArg], &freeList, &inputArg,
                             &objvmap[outputArg],
                             firstPosArg, &outputincr)) != TCL_OK) {
      goto exitforwardmethod;
    }
    outputArg += outputincr;

    /*
     * if we have nonpos args, determine the first pos arg position for %1
     */
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
      Tcl_Obj **listElements;
      int nrElements;

      /*
       * copy argument list from the definitions
       */
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
      /*
       * The objmap can shuffle the argument list. We have to set the
       * adressing relative from the end; -2 means last, -3 element before
       * last, etc.
       */
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
        if (pos == -1 || pos == j) {
          continue;
	}
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
      /*
       * If a prefix was provided, prepend a prefix for the subcommands to
       * avoid name clashes.
       */
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
    if (freeList)    {DECR_REF_COUNT2("freeList", freeList);}

    FREE_ON_STACK(int, objvmap);
    FREE_ON_STACK(Tcl_Obj*, OV);
  }
  return result;
}


/*
 *----------------------------------------------------------------------
 * NsfProcAliasMethod --
 *
 *    This Tcl_ObjCmdProc is called, when an alias to a proc is invoked. It
 *    handled epoched procs and dispatches finally the target method.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Maybe through the invoked command.
 *
 *----------------------------------------------------------------------
 */

static int
NsfProcAliasMethod(ClientData clientData,
                     Tcl_Interp *interp, int objc,
                     Tcl_Obj *CONST objv[]) {
  AliasCmdClientData *tcd = (AliasCmdClientData *)clientData;
  CONST char *methodName = ObjStr(objv[0]);
  NsfObject *self;

  assert(tcd);
  self = tcd->object;

  if (!self) {
    return NsfDispatchClientDataError(interp, self, "object",
				      Tcl_GetCommandName(interp, tcd->aliasCmd));
  }
  tcd->object = NULL;

  assert(self == GetSelfObj(interp));

  if (Tcl_Command_cmdEpoch(tcd->aliasedCmd)) {
    NsfObject *defObject = tcd->class ? &(tcd->class->object) : self;
    Tcl_Obj **listElements, *entryObj, *targetObj;
    int nrElements, withPer_object;
    Tcl_Command cmd;

    /*
     * Get the targetObject. Currently, we can get it just via the
     * alias array.
     */
    withPer_object = tcd->class ?  0 : 1;
    entryObj = AliasGet(interp, defObject->cmdName, methodName, withPer_object, 1);
    if (entryObj == NULL) {
      return TCL_ERROR;
    }
    INCR_REF_COUNT(entryObj);

    Tcl_ListObjGetElements(interp, entryObj, &nrElements, &listElements);
    targetObj = listElements[nrElements-1];

    NsfLog(interp, NSF_LOG_NOTICE,
	   "trying to dispatch an epoched cmd %p as %s -- cmdName %s\n",
	   tcd->aliasedCmd, methodName, ObjStr(targetObj));

    /*
     * Replace cmd and its objProc and clientData with a newly fetched
     * version.
     */
    cmd = Tcl_GetCommandFromObj(interp, targetObj);
    if (cmd == NULL) {
      int result = NsfPrintError(interp, "target \"%s\" of alias %s apparently disappeared",
			     ObjStr(targetObj), methodName);
      DECR_REF_COUNT(entryObj);
      return result;
    }
    cmd = GetOriginalCommand(cmd);

    NsfCommandRelease(INTERP tcd->aliasedCmd);
    tcd->objProc    = Tcl_Command_objProc(cmd);
    tcd->aliasedCmd = cmd;
    tcd->clientData = Tcl_Command_objClientData(cmd);
    NsfCommandPreserve(INTERP tcd->aliasedCmd);

    DECR_REF_COUNT(entryObj);
    /*
     * Now, we should be able to proceed as planned, we have an
     * non-epoched aliasCmd.
     */
  }

  return MethodDispatch(self, interp, objc, objv, tcd->aliasedCmd, self, tcd->class,
                        methodName, 0, 0);
}


/*
 *----------------------------------------------------------------------
 * NsfObjscopedMethod --
 *
 *    This Tcl_ObjCmdProc is called, when an objscoped alias is invoked.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Maybe through the invoked command.
 *
 *----------------------------------------------------------------------
 */

static int
NsfObjscopedMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  AliasCmdClientData *tcd = (AliasCmdClientData *)clientData;
  NsfObject *object = tcd->object;
  CallFrame frame, *framePtr = &frame;
  int result;

  /*fprintf(stderr, "objscopedMethod obj=%p %s, ptr=%p\n", object, ObjectName(object), tcd->objProc);*/
  tcd->object = NULL;

  Nsf_PushFrameObj(interp, object, framePtr);
  result = Tcl_NRCallObjProc(interp, tcd->objProc, tcd->clientData, objc, objv);
  Nsf_PopFrameObj(interp, framePtr);

  return result;
}

/*
 *----------------------------------------------------------------------
 * IsDashArg --
 *
 *    Check, whether the provided argument (pointed to be the index firstArg)
 *    starts with a "-", or is a list starting with a "-". The method returns
 *    via **methodName the name of the dashed argument (without the dash).
 *
 * Results:
 *    Enum value dashArgType.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

typedef enum {NO_DASH, SKALAR_DASH, LIST_DASH} dashArgType;

static dashArgType
IsDashArg(Tcl_Interp *interp, Tcl_Obj *obj, int firstArg, CONST char **methodName,
	  int *objc, Tcl_Obj **objv[]) {
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

/*
 *----------------------------------------------------------------------
 * CallConfigureMethod --
 *
 *    Call a method identified by a string selector; or provide an error
 *    message. This dispatcher function records as well contructor (init)
 *    calls via this interface. The dispatcher is used in XOTcl's
 *    configure(), interpretating arguments with a leading dash as method dispatches. 
 *    This behaviour is now implemented in NsfOResidualargsMethod().
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Maybe side effects from the called methods.
 *
 *----------------------------------------------------------------------
 */
static int
CallConfigureMethod(Tcl_Interp *interp, NsfObject *object, CONST char *initString,
		    CONST char *methodName,
                    int argc, Tcl_Obj *CONST argv[]) {
  int result;
  Tcl_Obj *methodObj = Tcl_NewStringObj(methodName, -1);

  /* fprintf(stderr, "CallConfigureMethod method %s->'%s' level %d, argc %d\n",
     ObjectName(object), methodName, level, argc);*/

  /*
   * When configure gets "-init" passed, we call "init" and notice the fact it
   * in the object's flags.
   */

  if (initString && *initString == *methodName && strcmp(methodName, initString) == 0) {
    object->flags |= NSF_INIT_CALLED;
  }

  Tcl_ResetResult(interp);
  INCR_REF_COUNT(methodObj);
  result = CallMethod(object, interp, methodObj, argc, argv,
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

/*
 *----------------------------------------------------------------------
 * IsRootNamespace --
 *
 *    Check, if the provided namespace is the namespace of the base
 *    class of an object system.
 *
 * Results:
 *    Boolean value.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
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

/*
 *----------------------------------------------------------------------
 * CallingNameSpace --
 *
 *    Find the last incovation outside the Next Scripting system
 *    namespaces. This funciton return the namesapce of the caller but
 *    skips system-specific namespaces (e.g. the namespaces of the
 *    pre-defined slot handlers for mixin and class
 *    registration. etc.) If we would use such namespaces, we would
 *    resolve non-fully-qualified names against the root namespace).
 *
 * Results:
 *    Tcl_Namespace or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static Tcl_Namespace *
CallingNameSpace(Tcl_Interp *interp) {
  Tcl_CallFrame *framePtr;
  Tcl_Namespace *nsPtr;

  /*NsfShowStack(interp);*/
  framePtr = CallStackGetActiveProcFrame((Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp));
  /* framePtr = BeginOfCallChain(interp, GetSelfObj(interp));*/

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

#include "nsfAPI.h"

static void
ArgumentResetRefCounts(INTERP_DECL struct Nsf_Param CONST *pPtr, Tcl_Obj *valueObj) {
  if ((pPtr->flags & NSF_ARG_IS_CONVERTER)) {
    //fprintf(stderr, "manually decrementing CONVERTER %p\n", valueObj);
    DECR_REF_COUNT2("valueObj", valueObj);
  }
}

static int
ArgumentCheckHelper(Tcl_Interp *interp, Tcl_Obj *objPtr, struct Nsf_Param CONST *pPtr, int *flags,
                        ClientData *clientData, Tcl_Obj **outObjPtr) {
  int objc, i, result;
  Tcl_Obj **ov;

  /*fprintf(stderr, "ArgumentCheckHelper\n");*/
  assert(pPtr->flags & NSF_ARG_MULTIVALUED);
  assert(*flags & NSF_PC_MUST_DECR);

  result = Tcl_ListObjGetElements(interp, objPtr, &objc, &ov);
  if (result != TCL_OK) {
    return result;
  }

  *outObjPtr = Tcl_NewListObj(0, NULL);
  INCR_REF_COUNT2("valueObj", *outObjPtr);

  for (i=0; i<objc; i++) {
    Tcl_Obj *elementObjPtr;
    const char *valueString = ObjStr(ov[i]);

    if (pPtr->flags & NSF_ARG_ALLOW_EMPTY && *valueString == '\0') {
      result = Nsf_ConvertToString(interp, ov[i], pPtr, clientData, &elementObjPtr);
    } else {
      result = (*pPtr->converter)(interp, ov[i], pPtr, clientData, &elementObjPtr);
    }

    /*fprintf(stderr, "ArgumentCheckHelper convert %s result %d (%s)\n",
      valueString, result, ObjStr(elementObjPtr));*/

    if (result == TCL_OK || result == TCL_CONTINUE) {
      Tcl_ListObjAppendElement(interp, *outObjPtr, elementObjPtr);
      /* 
       * If the refCount of the valueObj was already incremented, we have to
       * decrement it here, since we want the valuObj reclaimed when the list
       * containing the valueObj is freed.
       */
      ArgumentResetRefCounts(INTERP pPtr, elementObjPtr); 
    } else { 
      Tcl_Obj *resultObj = Tcl_GetObjResult(interp);

      INCR_REF_COUNT(resultObj);
      NsfPrintError(interp, "invalid value in \"%s\": %s", ObjStr(objPtr), ObjStr(resultObj));
      *flags &= ~NSF_PC_MUST_DECR;
      *outObjPtr = objPtr;
      DECR_REF_COUNT2("valueObj", *outObjPtr);
      DECR_REF_COUNT(resultObj);

      break;
    }
  }
  return result;
}

static int
ArgumentCheck(Tcl_Interp *interp, Tcl_Obj *objPtr, struct Nsf_Param CONST *pPtr, int doCheck,
	      int *flags, ClientData *clientData, Tcl_Obj **outObjPtr) {
  int result;

  /*
   * Default assumption: outObjPtr is not modified.
   */
  *outObjPtr = objPtr;
  //fprintf(stderr, "ArgumentCheck valueObj %p\n", objPtr);

  /*
   * If argument checking is turned off, and we do not have an converter, do nothing.
   */
  if (doCheck == 0 && (pPtr->flags & (NSF_ARG_IS_CONVERTER|NSF_ARG_INITCMD)) == 0) {
    /*fprintf(stderr, "*** omit  argument check for arg %s flags %.6x\n", pPtr->name, pPtr->flags);*/
    *clientData = ObjStr(objPtr);
    //*flags = 0; //yyyy;
    return TCL_OK;
  }

  /*
   * If the argument is multivalued, perform the check for every element of
   * the list (pure checker), or we have to build a new list of values (in
   * case, the converter alters the values).
   */
  if (unlikely(pPtr->flags & NSF_ARG_MULTIVALUED)) {
    int objc, i;
    Tcl_Obj **ov;

    result = Tcl_ListObjGetElements(interp, objPtr, &objc, &ov);
    if (unlikely(result != TCL_OK)) {
      return result;
    }

    if (objc == 0 && ((pPtr->flags & NSF_ARG_ALLOW_EMPTY) == 0)) {
      return NsfPrintError(interp,
			   "invalid value for parameter '%s': list is not allowed to be empty",
			   pPtr->name);
    }

    /*
     * In cases where necessary (the output element changed), switch to the
     * helper function
     */
    for (i=0; i<objc; i++) {
      Tcl_Obj *elementObjPtr;

      result = (*pPtr->converter)(interp, ov[i], pPtr, clientData, &elementObjPtr);
      if (likely(result == TCL_OK || result == TCL_CONTINUE)) {
        if (ov[i] != elementObjPtr) {
          fprintf(stderr, "ArgumentCheck: switch to output list construction for value %s\n",
		  ObjStr(elementObjPtr));
          /*
	   * The elementObjPtr differs from the input Tcl_Obj, we switch to
           * the version of this handler building an output list. But first,
           * we have to reset the refcounts from the first conversion.
	   */
	  ArgumentResetRefCounts(INTERP pPtr, elementObjPtr);
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
    CONST char *valueString = ObjStr(objPtr);

    if (pPtr->flags & NSF_ARG_ALLOW_EMPTY && *valueString == '\0') {
      result = Nsf_ConvertToString(interp, objPtr, pPtr, clientData, outObjPtr);
    } else {
      result = (*pPtr->converter)(interp, objPtr, pPtr, clientData, outObjPtr);
    }
    
    /*fprintf(stderr, "ArgumentCheck param %s type %s is converter %d flags %.6x "
	    "outObj changed %d (%p %p) isok %d\n", 
	    pPtr->name, pPtr->type, pPtr->flags & NSF_ARG_IS_CONVERTER, pPtr->flags, 
	    objPtr != *outObjPtr, objPtr, *outObjPtr, result == TCL_OK);*/
    if ((pPtr->flags & NSF_ARG_IS_CONVERTER) && objPtr != *outObjPtr) {
      *flags |= NSF_PC_MUST_DECR;//yyyy
    } else {
      /* 
       * If the output obj differs from the input obj, ensure we have
       * MUST_DECR set
       */
      assert( *flags & NSF_PC_MUST_DECR || objPtr == *outObjPtr );
    }
  }

  if (result == TCL_CONTINUE) {
    *flags |= NSF_ARG_WARN;
    result = TCL_OK;
  }

  //fprintf(stderr, "ArgumentCheck valueObj %p flags %.6x addr %p\n", objPtr, *flags, flags);
  return result;
}

static int
ArgumentDefaults(ParseContext *pcPtr, Tcl_Interp *interp,
                 Nsf_Param CONST *ifd, int nrParams) {
  Nsf_Param CONST *pPtr;
  int i;

  for (pPtr = ifd, i=0; i<nrParams; pPtr++, i++) {
    /*fprintf(stderr, "ArgumentDefaults got for arg %s (req %d, nrArgs %d) %p => %p %p, default '%s' \n",
            pPtr->name, pPtr->flags & NSF_ARG_REQUIRED, pPtr->nrArgs, pPtr,
            pcPtr->clientData[i], pcPtr->objv[i],
            pPtr->defaultValue ? ObjStr(pPtr->defaultValue) : "NONE");*/

    if (pcPtr->objv[i]) {
      /*
       * We got an actual value, which was already checked by ArgumentParse()
       * In case the value is a boolean, NSF_PC_MUST_INVERT is set. We invert
       * the value in place.
       */
      if ((pcPtr->flags[i] & NSF_PC_MUST_INVERT)) {
        int bool;
        Tcl_GetBooleanFromObj(interp, pPtr->defaultValue, &bool);
	pcPtr->objv[i] = Tcl_NewBooleanObj(!bool);
	/*
	 * Perform bookkeeping to avoid that someone releases the new obj
	 * before we are done. The according DECR is performed by
	 * ParseContextRelease() 
	 */
	INCR_REF_COUNT2("valueObj", pcPtr->objv[i]);
	pcPtr->flags[i] |= NSF_PC_MUST_DECR;
	pcPtr->status |= NSF_PC_STATUS_MUST_DECR;
      }
    } else {
      /*
       * No valued was passed, check if a default is available
       */

      if (pPtr->defaultValue) {
        int mustDecrNewValue;
        Tcl_Obj *newValue = pPtr->defaultValue;
        ClientData checkedData;

	/*
	 * We have a default value for the argument.  Mark that this argument
	 * gets the default value.
	 */
	pcPtr->flags[i] |= NSF_PC_IS_DEFAULT;

	/* Is it necessary to substitute the default value? */
        if (pPtr->flags & NSF_ARG_SUBST_DEFAULT) {
	  Tcl_Obj *obj = Tcl_SubstObj(interp, newValue, TCL_SUBST_ALL);

	  if (obj) {
	     newValue = obj;
	  } else {
	    pcPtr->flags[i] = 0;
	    return TCL_ERROR;
	  }
	
          /* The matching DECR is performed by ParseContextRelease() */
          INCR_REF_COUNT2("valueObj", newValue);
	  /*fprintf(stderr, "SUBST_DEFAULT increments %p refCount %d\n",
	    newValue,newValue->refCount);*/
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
        /*
	 * Check the default value if necessary
	 */
        if (pPtr->type || (pPtr->flags & NSF_ARG_MULTIVALUED)) {
          int mustDecrList = 0;
          if (ArgumentCheck(interp, newValue, pPtr,
			    RUNTIME_STATE(interp)->doCheckArguments,
			    &mustDecrList, &checkedData, &pcPtr->objv[i]) != TCL_OK) {
	    if (mustDecrNewValue) {
	      DECR_REF_COUNT2("valueObj", newValue);
	      pcPtr->flags[i] &= ~NSF_PC_MUST_DECR;
	    }
            return TCL_ERROR;
          }

	  if (pcPtr->objv[i] != newValue) {
	    /*
	     * The output Tcl_Obj differs from the input, so the
	     * Tcl_Obj was converted; in case we have set prevously
	     * must_decr on newValue, we decr the refCount on newValue
	     * here and clear the flag.
	     */
	    if (mustDecrNewValue) {
	      DECR_REF_COUNT2("valueObj", newValue);
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
        } else {
	  /*fprintf(stderr, "Param %s default %s type %s\n",
	    pPtr->name, ObjStr(pPtr->defaultValue), pPtr->type);*/
	  assert(pPtr->type ? pPtr->defaultValue == NULL : 1);
	}
      } else if (pPtr->flags & NSF_ARG_REQUIRED) {
	Tcl_Obj *paramDefsObj = NsfParamDefsSyntax(INTERP ifd);

        NsfPrintError(interp, "required argument '%s' is missing, should be:\n\t%s%s%s %s",
		      pPtr->nameObj ? ObjStr(pPtr->nameObj) : pPtr->name,
		      pcPtr->object ? ObjectName(pcPtr->object) : "",
		      pcPtr->object ? " " : "",
		      ObjStr(pcPtr->full_objv[0]),
		      ObjStr(paramDefsObj));
	DECR_REF_COUNT2("paramDefsObj", paramDefsObj);
	return TCL_ERROR;

      } else {
        /*
	 * Use as dummy default value an arbitrary symbol, which must
         * not be returned to the Tcl level level; this value is unset
         * later typically by NsfUnsetUnknownArgsCmd().
         */
        pcPtr->objv[i] = NsfGlobalObjs[NSF___UNKNOWN__];
      }
    }
  }
  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * ArgumentParse --
 *
 *    Parse the argument vector based on the parameter definitions.
 *    The parsed argument vector is returned in a normalized order
 *    in the parse context.
 *
 * Results:
 *    Tcl return code.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
extern int
Nsf_ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
		  Nsf_Object *object, Tcl_Obj *procNameObj,
		  Nsf_Param CONST *paramPtr, int nrParams, int doCheck,
		  Nsf_ParseContext *pcPtr) {
  return ArgumentParse(interp, objc, objv, (NsfObject *)object, procNameObj,
		       paramPtr, nrParams, doCheck,
		       (ParseContext *)pcPtr);
}

static int
ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
              NsfObject *object, Tcl_Obj *procNameObj,
              Nsf_Param CONST *paramPtr, int nrParams, int doCheck,
              ParseContext *pcPtr) {
  int i, o, flagCount, dashdash = 0, nrDashdash = 0;
  Nsf_Param CONST *pPtr;

  ParseContextInit(INTERP pcPtr, nrParams, object, objv[0]);

#if defined(PARSE_TRACE)
  fprintf(stderr, "PARAMETER ");
  for (o = 0, pPtr = paramPtr; pPtr->name; o++, pPtr++) {
    fprintf(stderr, "[%d]%s (nrargs %d %s) ", o,
	    pPtr->name, pPtr->nrArgs,
	    pPtr->flags & NSF_ARG_REQUIRED ? "req":"not req");
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "BEGIN (%d) [0]%s ", objc, ObjStr(procNameObj));
  for (o = 1; o < objc; o++) {fprintf(stderr, "[%d]%s ", o, ObjStr(objv[o]));}
  fprintf(stderr, "\n");
#endif

  /*
   * Process the argument vector according to the parameter
   * definitions.
   */
  for (i = 0, o = 1, pPtr = paramPtr; pPtr->name && o < objc;) {

#if defined(PARSE_TRACE_FULL)
    fprintf(stderr, "... (%d) processing [%d]: '%s' %s\n", i, o,
            pPtr->name, pPtr->flags & NSF_ARG_REQUIRED ? "req":"not req");
#endif
    flagCount = 0;
    if (*pPtr->name == '-') {
      int p, found;
      CONST char *argument;
      /*
       * We expect now a non-positional (named) parameter, starting
       * with a "-"; such arguments can be given in an arbitrary
       * order.  Therefore we iterate over the actual argument list
       * unti we find a matching flag.
       */
      for (p = o; p < objc; p++) {
        argument = ObjStr(objv[p]);
	found = 0;

        /*fprintf(stderr, "....checking objv[%d]=%s\n", p, argument);*/

        if (argument[0] != '-') {
	  /* there is no positional arg in the given argument vector */
	  break;
	} else {
	  /*
	   * We have an argument starting with a "-"; is it really one
	   * of the specified flags?
	   */
          Nsf_Param CONST *nppPtr;
	  CONST char *valueInArgument = strchr(argument, '=');
	  char ch1 = *(argument+1);

	  /* Is there a "--" ? */
	  if (ch1 == '-' && *(argument+2) == '\0' && dashdash == 0) {
#if defined(PARSE_TRACE_FULL)
	    fprintf(stderr, "... skip double dash once\n");
#endif
	    dashdash++;
	    nrDashdash++;
	    o++;
	    break;
	  }

	  if (valueInArgument) {
	    int equalOffset = valueInArgument - argument;
	    /*
	     * Handle parameter like -flag=1
	     *
	     * Just iterate over flags without arguments here.
	     */
	    for (nppPtr = pPtr; nppPtr->name && *nppPtr->name == '-'; nppPtr ++) {
	      if (nppPtr->nrArgs > 0) continue;
	      if ((nppPtr->flags & NSF_ARG_NOCONFIG) == 0
		  && ch1 == nppPtr->name[1]
		  && strncmp(argument, nppPtr->name, equalOffset) == 0
		  && *(nppPtr->name+equalOffset) == '\0') {
		found = 1;
		break;
	      }
	    }
	  } else {
	    /*
	     * Must be a classical nonpos arg; check for the string in
	     * the parameter definitions.
	     */
	    for (nppPtr = pPtr; nppPtr->name && *nppPtr->name == '-'; nppPtr ++) {
	      if ((nppPtr->flags & NSF_ARG_NOCONFIG) == 0 
		  && ch1 == nppPtr->name[1]
		  && strcmp(argument, nppPtr->name) == 0) {
		found = 1;
		break;
	      }
	    }
	  }

	  /*fprintf(stderr, "... nonpos arg '%s' found %d\n", argument, found);*/
	
	  if (found) {
	    Tcl_Obj *valueObj;
	    int j = nppPtr-paramPtr;

	    /*
	     * A matching non-positional argument was found.
	     */

	    /*fprintf(stderr, "...     flag '%s' nth param %d o=%d p=%d, objc=%d nrArgs %d\n",
	      argument, j, o, p, objc, nppPtr->nrArgs);*/

	    if (nppPtr->nrArgs == 0) {
	      /*
	       * No argument expected. Take value either from flag or
	       * use constant ONE.
	       */

	      if (valueInArgument) {
		valueObj = Tcl_NewStringObj(valueInArgument+1,-1);
		INCR_REF_COUNT2("valueObj", valueObj);
		pcPtr->flags[j] |= NSF_PC_MUST_DECR;
		pcPtr->status |= NSF_PC_STATUS_MUST_DECR;
		/*fprintf(stderr, "setting flag MUST_DECR for valueObj %p refCount %d "
			"pcPtr %p flags[%d] %.6x &flags %p\n", 
			valueObj, valueObj->refCount, pcPtr, j, 
			pcPtr->flags[j], &(pcPtr->flags[j]));*/
	      } else {
		if (nppPtr->converter == Nsf_ConvertToSwitch) {
		  /*fprintf(stderr,"set MUST_INVERT for '%s' flags %.6x\n",
		    nppPtr->name, nppPtr->flags);*/
		  pcPtr->flags[j] |= NSF_PC_MUST_INVERT;
		}
		valueObj = NsfGlobalObjs[NSF_ONE];
	      }
	    } else {
	      /*
	       * nrArgs is 0 or 1; therfore we expect one argument (currently,
	       * it has to be exactly one argument). Increment the counters
	       * and get the argument from the provided argument vector.
	       */
	      assert(nppPtr->nrArgs == 1);
	      o++; p++;

	      if (likely(p < objc)) {
#if defined(PARSE_TRACE_FULL)
		fprintf(stderr, "...     setting cd[%d] '%s' = %s (%d) %s converter %p\n",
			i, nppPtr->name, ObjStr(objv[p]), nppPtr->nrArgs,
			nppPtr->flags & NSF_ARG_REQUIRED ? "req" : "not req", nppPtr->converter);
#endif
		valueObj = objv[p];
	      } else {
		return NsfPrintError(interp, "Argument for parameter '%s' expected", nppPtr->name);
	      }
	    }
	    /*
	     * The value for the flag is now in the valueObj. We
	     * check, whether it is value is permissible.
	     */
	    if (unlikely(ArgumentCheck(interp, valueObj, nppPtr, doCheck,
				       &pcPtr->flags[j],
				       &pcPtr->clientData[j], 
				       &pcPtr->objv[j]) != TCL_OK)) {
	      if (pcPtr->flags[j] & NSF_PC_MUST_DECR) {pcPtr->status |= NSF_PC_STATUS_MUST_DECR;}
	      return TCL_ERROR;
	    }
	    /*fprintf(stderr, "... nonpositional pcPtr %p check [%d] obj %p flags %.6x & %p\n",
	      pcPtr, j, pcPtr->objv[j], pcPtr->flags[j], &(pcPtr->flags[j]));	*/

	    /*
	     * Provide warnings for double-settings.
	     */
	    if (pcPtr->flags[j] & NSF_ARG_SET) {
	      Tcl_Obj *cmdLineObj = Tcl_NewListObj(objc-1, objv+1);
	      INCR_REF_COUNT(cmdLineObj);
	      NsfLog(interp, NSF_LOG_WARN, "Non-positional parameter %s was passed more than once (%s%s%s %s)",
		     nppPtr->name,
		     object ? ObjectName(object) : "", object ? " method " : "",
		     ObjStr(procNameObj), ObjStr(cmdLineObj));
	      DECR_REF_COUNT(cmdLineObj);
	    }
	    pcPtr->flags[j] |= NSF_ARG_SET;

	    /*
	     * Provide context for warning messages
	     */
	    if (pcPtr->flags[j] & NSF_ARG_WARN) {
	      Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
	      Tcl_DString ds, *dsPtr = &ds;

	      Tcl_DStringInit(dsPtr);
	      INCR_REF_COUNT(resultObj);
	      NsfDStringArgv(dsPtr, objc, objv);
	      NsfLog(interp, NSF_LOG_WARN, "%s during:\n%s %s",
		     ObjStr(resultObj), ObjectName(object), Tcl_DStringValue(dsPtr));
	      DECR_REF_COUNT(resultObj);
	      Tcl_DStringFree(dsPtr);
	    }
		
	    if (pcPtr->flags[j] & NSF_PC_MUST_DECR) {
	      //fprintf(stderr, "pcPtr %p setting NSF_PC_STATUS_MUST_DECR\n", pcPtr);
	      pcPtr->status |= NSF_PC_STATUS_MUST_DECR;
	    }
		
	    flagCount++;
	  } else {
            /*
	     * We did not find the specified flag, the thing starting
	     * with a '-' must be an argument
	     */
	    assert(found == 0);
            break;
          }
        }
      }
      /*fprintf(stderr, "... we found %d flags\n", flagCount);*/
      /* skip in parameter definition until the end of the switches */
      while (pPtr->name && *pPtr->name == '-') {pPtr++, i++;};
      /* under the assumption, flags have no arguments */
      o += flagCount;

    } else {

      /* Handle positional (unnamed) parameters, starting without a
       * "-"; arguments must be always in same order
       */

      /* reset dashdash, if needed */
      if (dashdash) {dashdash = 0;}

      /*fprintf(stderr, "positional arg %s nrargs %d required %d\n",
	pPtr->name, pPtr->nrArgs, pPtr->flags & NSF_ARG_REQUIRED);*/

      if (pPtr->nrArgs > 0) {

	/*fprintf(stderr, "... arg %s req %d converter %p try to set on %d: '%s' ConvertViaCmd %p\n",
		pPtr->name, pPtr->flags & NSF_ARG_REQUIRED, pPtr->converter, i, ObjStr(objv[o]),
		ConvertViaCmd);*/
	
	if (unlikely(ArgumentCheck(interp, objv[o], pPtr, doCheck,
				   &pcPtr->flags[i], 
				   &pcPtr->clientData[i], 
				   &pcPtr->objv[i]) != TCL_OK)) {
	  if (pcPtr->flags[i] & NSF_PC_MUST_DECR) {pcPtr->status |= NSF_PC_STATUS_MUST_DECR;}
	  return TCL_ERROR;
	}

	/*fprintf(stderr, "... positional pcPtr %p check [%d] obj %p flags %.6x & %p\n",
	  pcPtr, i, pcPtr->objv[i], pcPtr->flags[i], &(pcPtr->flags[i]));*/

	if (pcPtr->flags[i] & NSF_PC_MUST_DECR) {
	  //fprintf(stderr, "pcPtr %p setting NSF_PC_STATUS_MUST_DECR 2\n", pcPtr);
	  pcPtr->status |= NSF_PC_STATUS_MUST_DECR;
	}
	/*
	 * objv is always passed via pcPtr->objv
	 */
#if defined(PARSE_TRACE_FULL)
	fprintf(stderr, "...     setting %s pPtr->objv[%d] to [%d]'%s' converter %p\n",
		pPtr->name, i, o, ObjStr(objv[o]), pPtr->converter);
#endif
	o++;
      }
      i++; pPtr++;
    }
  }
  pcPtr->lastObjc = pPtr->name ? o : o-1;
  pcPtr->objc = i + 1;

  /*
   * Process remaining parameters to the end of parameter definitions
   */
  for (; pPtr->name; pPtr++) {}

  /*
   * Is the last argument a vararg?
   */
  pPtr--;
  if (pPtr->converter == ConvertToNothing) {
    pcPtr->varArgs = 1;
    /*fprintf(stderr, "last arg of proc '%s' is varargs\n", ObjStr(procNameObj));*/
  }

  /*
   * Handle unexpected arguments for methods and cmds
   */
  if (unlikely(!pcPtr->varArgs && o < objc)) {
    Tcl_DString ds, *dsPtr = &ds;
    DSTRING_INIT(dsPtr);
    Tcl_DStringAppend(dsPtr, "Invalid argument '", -1);
    Tcl_DStringAppend(dsPtr, ObjStr(objv[pcPtr->lastObjc+1]), -1);
    Tcl_DStringAppend(dsPtr, "', maybe too many arguments;", -1);
    NsfArgumentError(interp, Tcl_DStringValue(dsPtr), paramPtr,
		     object ? object->cmdName : NULL,
		     procNameObj); // aaaa
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
      Var  *val = TclVarHashGetValue(hPtr);
      Tcl_SetObjResult(interp, TclVarHashGetKey(val));
    } else {
      Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_EMPTY]);
    }
    DECR_REF_COUNT(patternObj);
  } else {
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);
    Tcl_HashSearch hSrch;
    hPtr = tablePtr ? Tcl_FirstHashEntry(tablePtr, &hSrch) : NULL;
    for (; hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
      Var  *val = TclVarHashGetValue(hPtr);
      Tcl_Obj *key  = TclVarHashGetKey(val);
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
ListParamDefs(Tcl_Interp *interp, Nsf_Param CONST *paramsPtr, NsfParamsPrintStyle style) {
  Tcl_Obj *listObj;

  switch (style) {
  case NSF_PARAMS_PARAMETER: listObj = ParamDefsFormat(interp, paramsPtr); break;
  case NSF_PARAMS_LIST:      listObj = ParamDefsList(interp, paramsPtr); break;
  case NSF_PARAMS_NAMES:     listObj = ParamDefsNames(interp, paramsPtr); break;
  case NSF_PARAMS_SYNTAX:    listObj = NsfParamDefsSyntax(INTERP paramsPtr); break;
  default: listObj = NULL;
  }

  return listObj;
}

/*
 *----------------------------------------------------------------------
 * ListCmdParams --
 *
 *    Obtains a cmd and a method name and sets as side effect the Tcl result
 *    to either the list. The printstyle NSF_PARAMS_NAMES, NSF_PARAMS_LIST,
 *    NSF_PARAMS_PARAMETER, NSF_PARAMS_SYNTAX controls the elements of the
 *    list.
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
ListCmdParams(Tcl_Interp *interp, Tcl_Command cmd, CONST char *methodName,
	      NsfParamsPrintStyle printStyle) {
  NsfParamDefs *paramDefs;
  Tcl_Obj *list;
  Proc *procPtr;

  assert(methodName);
  assert(cmd);

  paramDefs = ParamDefsGet(cmd);

  if (paramDefs && paramDefs->paramsPtr) {
    /*
     * Obtain parameter info from paramDefs
     */
    list = ListParamDefs(interp, paramDefs->paramsPtr, printStyle);
    Tcl_SetObjResult(interp, list);
    DECR_REF_COUNT2("paramDefsObj", list);
    return TCL_OK;
  }

  procPtr = GetTclProcFromCommand(cmd);
  if (procPtr) {
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

      if (printStyle == NSF_PARAMS_SYNTAX && strcmp(args->name, "args") == 0) {
	if (args != procPtr->firstLocalPtr) {
	  Tcl_AppendToObj(list, " ", 1);
	}
	Tcl_AppendToObj(list, "?arg ...?", 9);
      } else {
	innerlist = Tcl_NewListObj(0, NULL);
	Tcl_ListObjAppendElement(interp, innerlist, Tcl_NewStringObj(args->name, -1));
	if (printStyle == NSF_PARAMS_PARAMETER && args->defValuePtr) {
	  Tcl_ListObjAppendElement(interp, innerlist, args->defValuePtr);
	}
	Tcl_ListObjAppendElement(interp, list, innerlist);
	}
    }

    Tcl_SetObjResult(interp, list);
    return TCL_OK;
  }

  {
    /*
     * If a command is found for the object|class, check whether we
     * find the parameter definitions for the C-defined method.
     */
    Nsf_methodDefinition *mdPtr = &method_definitions[0];

    for (; mdPtr->methodName; mdPtr ++) {

      /*fprintf(stderr, "... comparing %p with %p => %s\n", ((Command *)cmd)->objProc, mdPtr->proc,
	mdPtr->methodName);*/

      if (((Command *)cmd)->objProc == mdPtr->proc) {
	NsfParamDefs paramDefs = {mdPtr->paramDefs, mdPtr->nrParameters, 1, NULL, NULL};
	Tcl_Obj *list = ListParamDefs(interp, paramDefs.paramsPtr, printStyle);
	
	Tcl_SetObjResult(interp, list);
	DECR_REF_COUNT2("paramDefsObj", list);
	return TCL_OK;
      }
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
      list = ListParamDefs(interp, paramDefs.paramsPtr, printStyle);
      Tcl_SetObjResult(interp, list);
      DECR_REF_COUNT2("paramDefsObj", list);
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
  if (printStyle == NSF_PARAMS_SYNTAX) {
    Tcl_DString ds, *dsPtr = &ds;
    Tcl_Obj *parameterSyntaxObj;

    Tcl_DStringInit(dsPtr);
    DStringAppendQualName(dsPtr, Tcl_Command_nsPtr(cmd), methodName);
    /*fprintf(stderr,"Looking up ::nsf::parametersyntax(%s) ...\n", Tcl_DStringValue(dsPtr));*/
    parameterSyntaxObj = Tcl_GetVar2Ex(interp, "::nsf::parametersyntax",
				       Tcl_DStringValue(dsPtr), TCL_GLOBAL_ONLY);

    /*fprintf(stderr, "No parametersyntax so far methodname %s cmd name %s ns %s\n",
      methodName, Tcl_GetCommandName(interp, cmd), Tcl_DStringValue(dsPtr));*/

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
    Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("class", 5));
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

  Tcl_ResetResult(interp);

  if (!cmd) {
    if (subcmd == InfomethodsubcmdExistsIdx) {
      Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
    }
  } else {
    Tcl_ObjCmdProc *procPtr = Tcl_Command_objProc(cmd);
    int outputPerObject = 0;
    Tcl_Obj *resultObj;

    assert(methodName);
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
	Tcl_SetObjResult(interp, MethodHandleObj(regObject, withPer_object, methodName));
	return TCL_OK;
      }
    case InfomethodsubcmdOriginIdx:
      {
	Tcl_SetObjResult(interp, MethodHandleObj(defObject, 
						 NsfObjectIsClass(defObject) ? withPer_object : 1, 
						 Tcl_GetCommandName(interp, cmd)));
	return TCL_OK;
      }
    case InfomethodsubcmdExistsIdx:
      {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
	return TCL_OK;
      }
    case InfomethodsubcmdArgsIdx:
      {
        Tcl_Command importedCmd = GetOriginalCommand(cmd);
        return ListCmdParams(interp, importedCmd, methodName, NSF_PARAMS_NAMES);
      }
    case InfomethodsubcmdParameterIdx:
      {
        Tcl_Command importedCmd = GetOriginalCommand(cmd);
        return ListCmdParams(interp, importedCmd, methodName, NSF_PARAMS_PARAMETER);
      }
    case InfomethodsubcmdParametersyntaxIdx:
      {
        Tcl_Command importedCmd = GetOriginalCommand(cmd);
        return ListCmdParams(interp, importedCmd, methodName, NSF_PARAMS_SYNTAX);
      }
    case InfomethodsubcmdPreconditionIdx:
      {
#if defined(NSF_WITH_ASSERTIONS)
        NsfProcAssertion *procs;
        if (withPer_object) {
          procs = regObject->opt ? AssertionFindProcs(regObject->opt->assertions, methodName) : NULL;
        } else {
          NsfClass *class = (NsfClass *)regObject;
          procs = class->opt ? AssertionFindProcs(class->opt->assertions, methodName) : NULL;
        }
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->pre));
#endif
        return TCL_OK;
      }
    case InfomethodsubcmdPostconditionIdx:
      {
#if defined(NSF_WITH_ASSERTIONS)
        NsfProcAssertion *procs;
        if (withPer_object) {
          procs = regObject->opt ? AssertionFindProcs(regObject->opt->assertions, methodName) : NULL;
        } else {
          NsfClass *class = (NsfClass *)regObject;
          procs = class->opt ? AssertionFindProcs(class->opt->assertions, methodName) : NULL;
        }
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->post));
#endif
        return TCL_OK;
      }
    case InfomethodsubcmdSubmethodsIdx:
      {
	if (procPtr == NsfObjDispatch) {
	  NsfObject *subObject = NsfGetObjectFromCmdPtr(cmd);
	  if (subObject) {
	    return ListDefinedMethods(interp, subObject, NULL, 1 /* per-object */,
				      NSF_METHODTYPE_ALL, CallprotectionAllIdx, 0);
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
          ListCmdParams(interp, cmd, methodName, NSF_PARAMS_PARAMETER);
          Tcl_ListObjAppendElement(interp, resultObj, Tcl_GetObjResult(interp));
          ListProcBody(interp, GetTclProcFromCommand(cmd), methodName);
          Tcl_ListObjAppendElement(interp, resultObj, Tcl_GetObjResult(interp));

          if (withPer_object) {
            assertions = regObject->opt ? regObject->opt->assertions : NULL;
          } else {
            NsfClass *class = (NsfClass *)regObject;
            assertions = class->opt ? class->opt->assertions : NULL;
          }
#if defined(NSF_WITH_ASSERTIONS)
          if (assertions) {
            NsfProcAssertion *procs = AssertionFindProcs(assertions, methodName);
            if (procs) {
              Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("-precondition", -1));
              Tcl_ListObjAppendElement(interp, resultObj, AssertionList(interp, procs->pre));
              Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("-postcondition", -1));
              Tcl_ListObjAppendElement(interp, resultObj, AssertionList(interp, procs->post));
            }
          }
#endif
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
    } else if (procPtr == NsfProcStub) {
      /*
       * special nsfproc handling
       */
      NsfProcClientData *tcd = Tcl_Command_objClientData(cmd);
      if (tcd && tcd->procName) {
	Tcl_Command procCmd = Tcl_GetCommandFromObj(interp, tcd->procName);
	Tcl_DString ds, *dsPtr = &ds;
	Tcl_Obj *resultObj;

	switch (subcmd) {

	case InfomethodsubcmdTypeIdx:
	  Tcl_SetObjResult(interp, Tcl_NewStringObj("nsfproc", -1));
	  break;

	case InfomethodsubcmdBodyIdx:
	  ListProcBody(interp, GetTclProcFromCommand(procCmd), methodName);
	  break;

	case InfomethodsubcmdDefinitionIdx:
          resultObj = Tcl_NewListObj(0, NULL);
	  Tcl_DStringInit(dsPtr);
	  DStringAppendQualName(dsPtr, Tcl_Command_nsPtr(cmd), methodName);
	  /* don't hardcode names */
	  Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("::nsf::proc", -1));
	  if (tcd->with_ad) {
	    Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("-ad", 3));
	  }
	  Tcl_ListObjAppendElement(interp, resultObj,
				   Tcl_NewStringObj(Tcl_DStringValue(dsPtr),
						    Tcl_DStringLength(dsPtr)));
          ListCmdParams(interp, cmd, Tcl_DStringValue(dsPtr), NSF_PARAMS_PARAMETER);
          Tcl_ListObjAppendElement(interp, resultObj, Tcl_GetObjResult(interp));
          ListProcBody(interp, GetTclProcFromCommand(procCmd), methodName);
          Tcl_ListObjAppendElement(interp, resultObj, Tcl_GetObjResult(interp));
	  Tcl_SetObjResult(interp, resultObj);
	  Tcl_DStringFree(dsPtr);
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
			  regObject != defObject ? 1 : withPer_object, 0);
      /*
      fprintf(stderr, "aliasGet %s -> %s/%s (%d) returned %p\n",
	      ObjectName(defObject), methodName, Tcl_GetCommandName(interp, cmd),
	      withPer_object, entryObj);
      fprintf(stderr, "... regObject %p %s\n", regObject, ObjectName(regObject));
      fprintf(stderr, "... defObject %p %s\n", defObject, ObjectName(defObject));
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
	      AppendMethodRegistration(interp, resultObj, "create",
				       &(subObject->cl)->object,
				       ObjStr(subObject->cmdName), cmd, 0, 0, 0);
	      Tcl_SetObjResult(interp, resultObj);
	      break;
	    }
	  }
	} else {
	  /*
	   * Should never happen
	   *
	   * The warning is just a guess, so we don't raise an error here.
	   */
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


/*
 *----------------------------------------------------------------------
 * MethodSourceMatches --
 *
 *    Check, whether the provided class or object (mutually exclusive) matches
 *    with the required method source (typically all|application|baseclasses).
 *
 * Results:
 *    Returns true or false
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int MethodSourceMatches(int withSource, NsfClass *cl, NsfObject *object) {
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


/*
 *----------------------------------------------------------------------
 * MethodTypeMatches --
 *
 *    Check, whether the provided method (specified as a cmd) matches with the
 *    required method type (typically
 *    all|scripted|builtin|alias|forwarder|object|setter).
 *
 * Results:
 *    Returns true or false
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
MethodTypeMatches(Tcl_Interp *interp, int methodType, Tcl_Command cmd,
                  NsfObject *object, CONST char *methodName, int withPer_object, int *isObject) {
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
    if (!(proc == NsfProcAliasMethod || AliasGet(interp, object->cmdName, methodName, withPer_object, 0))) {
      return 0;
    }
  } else {
    if (proc == NsfProcAliasMethod) {
      if ((methodType & NSF_METHODTYPE_ALIAS) == 0) return 0;
    }
    /* the following cases are disjoint */
    if (CmdIsProc(importedCmd)) {
      /*fprintf(stderr,"%s scripted %d\n", methodName, methodType & NSF_METHODTYPE_SCRIPTED);*/
      if ((methodType & NSF_METHODTYPE_SCRIPTED) == 0) return 0;
    } else if (resolvedProc == NsfForwardMethod) {
      if ((methodType & NSF_METHODTYPE_FORWARDER) == 0) return 0;
    } else if (resolvedProc == NsfSetterMethod) {
      if ((methodType & NSF_METHODTYPE_SETTER) == 0) return 0;
    } else if (resolvedProc == NsfObjDispatch) {
      if ((methodType & NSF_METHODTYPE_OBJECT) == 0) return 0;
    } else if (resolvedProc == NsfProcStub) {
      if ((methodType & NSF_METHODTYPE_NSFPROC) == 0) return 0;
    } else if ((methodType & NSF_METHODTYPE_OTHER) == 0) {
      /* fprintf(stderr,"OTHER %s not wanted %.4x\n", methodName, methodType);*/
      return 0;
    }
    /* NsfObjscopedMethod ??? */
  }
  return 1;
}

/*
 *----------------------------------------------------------------------
 * ProtectionMatches --
 *
 *    Check, whether the provided method (specified as a cmd) matches with the
 *    required call-protection (typically all|protected|public).
 *
 * Results:
 *    Returns true or false
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
ProtectionMatches(int withCallprotection, Tcl_Command cmd) {
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

/*
 *----------------------------------------------------------------------
 *
 * ListMethodKeys --
 *
 *      List the method names contained in the specified hash table
 *      according to the filtering options (types, pattern,
 *      protection, etc.). Optionally, a name prefix can be provided
 *      in form of a Tcl_DString. The result is placed into the interp
 *      result.
 *
 * Results:
 *      Tcl result code.
 *
 * Side effects:
 *      Setting interp result.
 *
 *----------------------------------------------------------------------
 */
static int
ListMethodKeys(Tcl_Interp *interp, Tcl_HashTable *tablePtr,
	       Tcl_DString *prefix, CONST char *pattern,
               int methodType, int withCallprotection, int withExpand,
               Tcl_HashTable *dups, NsfObject *object, int withPer_object) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  Tcl_Command cmd;
  char *key;
  int new, isObject, methodTypeMatch;
  int prefixLength = prefix ? Tcl_DStringLength(prefix) : 0;
  Tcl_Obj *resultObj = Tcl_GetObjResult(interp);

  assert(tablePtr);

  if (pattern && NoMetaChars(pattern) && strchr(pattern, ' ') == NULL) {
    /*
     * We have a pattern that can be used for direct lookup; no need
     * to iterate
     */
    hPtr = Tcl_CreateHashEntry(tablePtr, pattern, NULL);
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

      if (ProtectionMatches(withCallprotection, cmd) && methodTypeMatch) {
	if (prefixLength) {
	  Tcl_DStringAppend(prefix, key, -1);
	  key = Tcl_DStringValue(prefix);
	}
        if (dups) {
          Tcl_CreateHashEntry(dups, key, &new);
          if (new) {
            Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj(key, -1));
          }
        } else {
	  Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj(key, -1));
        }
      }
    }
    return TCL_OK;

  } else {

    for (hPtr = Tcl_FirstHashEntry(tablePtr, &hSrch);
	 hPtr;
	 hPtr = Tcl_NextHashEntry(&hSrch)) {
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
	if (cmdTablePtr == NULL) {
	  /* nothing to do */
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
      if (!ProtectionMatches(withCallprotection, cmd) || !methodTypeMatch) continue;

      if (prefixLength) {
	Tcl_DStringAppend(prefix, key, -1);
	key = Tcl_DStringValue(prefix);
      }

      if (pattern && !Tcl_StringMatch(key, pattern)) continue;
      if (dups) {
        Tcl_CreateHashEntry(dups, key, &new);
        if (!new) continue;
      }
      Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj(key, -1));
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
      DStringAppendQualName(dsPtr, object->nsPtr, pattern);
      pattern = Tcl_DStringValue(dsPtr);
    }

    if ((childObject = GetObjectFromString(interp, pattern)) &&
        (!classesOnly || NsfObjectIsClass(childObject)) &&
	(!type || IsSubType(INTERP childObject->cl, type)) &&
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
    Tcl_HashEntry *hPtr;
    char *key;

    for (hPtr = Tcl_FirstHashEntry(cmdTablePtr, &hSrch);
	 hPtr;
	 hPtr = Tcl_NextHashEntry(&hSrch)) {
      key = Tcl_GetHashKey(cmdTablePtr, hPtr);
      if (!pattern || Tcl_StringMatch(key, pattern)) {
        Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);

        /*fprintf(stderr, "... check %s child key %s child object %p %p\n",
                ObjectName(object), key, GetObjectFromString(interp, key),
                NsfGetObjectFromCmdPtr(cmd));*/

        if ((childObject = NsfGetObjectFromCmdPtr(cmd)) &&
            (!classesOnly || NsfObjectIsClass(childObject)) &&
	    (!type || IsSubType(INTERP childObject->cl, type)) &&
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
ListForward(Tcl_Interp *interp, Tcl_HashTable *tablePtr,
	    CONST char *pattern, int withDefinition) {

  if (tablePtr == NULL) {
    return TCL_OK;
  }

  if (withDefinition) {
    Tcl_HashEntry *hPtr = pattern ? Tcl_CreateHashEntry(tablePtr, pattern, NULL) : NULL;
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

/*
 *----------------------------------------------------------------------
 *
 * ListDefinedMethods --
 *
 *      List the methods defined by the specified object/class
 *      according to the filtering options (types, pattern,
 *      protection, etc.). The result is placed into the interp
 *      result.
 *
 * Results:
 *      Tcl result code.
 *
 * Side effects:
 *      Setting interp result.
 *
 *----------------------------------------------------------------------
 */
static int
ListDefinedMethods(Tcl_Interp *interp, NsfObject *object, CONST char *pattern,
                   int withPer_object, int methodType, int withCallproctection,
                   int withExpand) {
  Tcl_HashTable *cmdTablePtr;
  Tcl_DString ds, *dsPtr = NULL;

  if (pattern && *pattern == ':' && *(pattern + 1) == ':') {
    Namespace *nsPtr, *dummy1Ptr, *dummy2Ptr;
    CONST char *remainder;

    /*fprintf(stderr, "we have a colon pattern '%s' methodtype %.6x\n", pattern, methodType);*/

    TclGetNamespaceForQualName(interp, pattern, NULL, 0,
			       &nsPtr, &dummy1Ptr, &dummy2Ptr, &remainder);
    /*fprintf(stderr,
	    "TclGetNamespaceForQualName with %s => (%p %s) (%p %s) (%p %s) (%p %s)\n",
	    pattern,
	    nsPtr, nsPtr ? nsPtr->fullName : "",
	    dummy1Ptr,  dummy1Ptr ? dummy1Ptr->fullName : "",
	    dummy2Ptr,  dummy2Ptr ? dummy2Ptr->fullName : "",
	    remainder, remainder ? remainder : "");*/
    if (nsPtr) {
      cmdTablePtr = Tcl_Namespace_cmdTablePtr(nsPtr);
      dsPtr = &ds;
      Tcl_DStringInit(dsPtr);
      Tcl_DStringAppend(dsPtr, nsPtr->fullName, -1);
      if (Tcl_DStringLength(dsPtr) > 2) {
	Tcl_DStringAppend(dsPtr, "::", 2);
      }
      pattern = remainder;
    } else {
      cmdTablePtr = NULL;
    }
  } else if (NsfObjectIsClass(object) && !withPer_object) {
    cmdTablePtr = Tcl_Namespace_cmdTablePtr(((NsfClass *)object)->nsPtr);
  } else {
    cmdTablePtr = object->nsPtr ? Tcl_Namespace_cmdTablePtr(object->nsPtr) : NULL;
  }

  if (cmdTablePtr) {
    ListMethodKeys(interp, cmdTablePtr, dsPtr, pattern, methodType, withCallproctection, withExpand,
		   NULL, object, withPer_object);
    if (dsPtr) {
      Tcl_DStringFree(dsPtr);
    }
  }
  return TCL_OK;
}

static int
ListSuperclasses(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *pattern, int withClosure) {
  NsfObject *matchObject = NULL;
  Tcl_Obj *patternObj = NULL, *outObjPtr;
  CONST char *patternString = NULL;
  ClientData clientData;
  int rc;

  if (pattern &&
      ConvertToObjpattern(interp, pattern, NULL, &clientData, &outObjPtr) == TCL_OK) {
    patternObj = (Tcl_Obj *)clientData;
    if (GetMatchObject(interp, patternObj, pattern, &matchObject, &patternString) == -1) {
      /*
       * The pattern has no meta chars and does not correspond to an existing
       * object. Therefore, it can't be a superclass.
       */
      if (patternObj) {
	DECR_REF_COUNT2("patternObj", patternObj);
      }
      return TCL_OK;
    }
  }

  if (withClosure) {
    NsfClasses *pl = ComputeOrder(INTERP cl, cl->order, Super);
    if (pl) pl=pl->nextPtr;
    rc = AppendMatchingElementsFromClasses(interp, pl, patternString, matchObject);
  } else {
    NsfClasses *clSuper = NsfReverseClasses(INTERP cl->super);
    rc = AppendMatchingElementsFromClasses(interp, clSuper, patternString, matchObject);
    NsfClassListFree(INTERP clSuper);
  }

  if (matchObject) {
    Tcl_SetObjResult(interp, rc ? matchObject->cmdName : NsfGlobalObjs[NSF_EMPTY]);
  }

  if (patternObj) {
    DECR_REF_COUNT2("patternObj", patternObj);
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
AliasGet(Tcl_Interp *interp, Tcl_Obj *cmdName, CONST char *methodName, int withPer_object, int leaveError) {
  Tcl_DString ds, *dsPtr = &ds;
  Tcl_Obj *obj = Tcl_GetVar2Ex(interp, NsfGlobalStrings[NSF_ALIAS_ARRAY],
                               AliasIndex(dsPtr, cmdName, methodName, withPer_object),
                               TCL_GLOBAL_ONLY);
  /*fprintf(stderr, "aliasGet methodName '%s' returns %p\n", methodName, obj);*/
  Tcl_DStringFree(dsPtr);
  if (obj == NULL && leaveError) {
    NsfPrintError(interp, "Could not obtain alias definition for %s %s.",
		  ObjStr(cmdName), methodName);
  }
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

  /*fprintf(stderr, "AliasDeleteObjectReference on %p obj %p\n", cmd, referencedObject);*/
  if (referencedObject
      && referencedObject->refCount > 0
      && cmd != referencedObject->id) {
    /*
     * The cmd is an aliased object, reduce the refCount of the
     * object, delete the cmd.
     */
    /*fprintf(stderr, "remove alias %s to %s\n",
      Tcl_GetCommandName(interp, cmd), ObjectName(referencedObject));*/
    NsfCleanupObject(INTERP referencedObject, "AliasDeleteObjectReference");
    Tcl_DeleteCommandFromToken(interp, cmd);
    return 1;
  }
  return 0;
}


/***********************************************************************
 * Begin generated Next Scripting commands
 ***********************************************************************/

/*
cmd __db_compile_epoch NsfDebugCompileEpoch {}
 */
static int 
NsfDebugCompileEpoch(Tcl_Interp *interp) {
  Interp *iPtr = (Interp *) interp;

  Tcl_SetObjResult(interp, Tcl_NewIntObj(iPtr->compileEpoch));
  return TCL_OK;
}

/*
cmd __db_show_stack NsfShowStackCmd {}
*/
static int
NsfShowStackCmd(Tcl_Interp *interp) {
  NsfShowStack(interp);
  return TCL_OK;
}

/*
cmd __db_run_assertions NsfDebugRunAssertionsCmd {}
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
    assert(object->refCount > 0);
    assert(object->cmdName->refCount > 0);
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
	if (cscPtr && (NsfObject *)cscPtr->cl == object) count ++;
      }
      for (; unstackedEntries; unstackedEntries = unstackedEntries->nextPtr) {
	NsfCallStackContent *cscPtr = (NsfCallStackContent *)unstackedEntries->cl;
	if (cscPtr && cscPtr->self == object) count ++;
	if (cscPtr && (NsfObject *)cscPtr->cl == object) count ++;	
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
cmd __profile_clear_data NsfProfileClearDataStub {}
*/
static int
NsfProfileClearDataStub(Tcl_Interp *interp) {
#if defined(NSF_PROFILE)
  NsfProfileClearData(interp);
#endif
  return TCL_OK;
}

/*
cmd __profile_get_data NsfProfileGetDataStub {}
*/
static int
NsfProfileGetDataStub(Tcl_Interp *interp) {
#if defined(NSF_PROFILE)
  NsfProfileGetData(interp);
#endif
  return TCL_OK;
}

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
/*
cmd __unset_unknown_args NsfUnsetUnknownArgsCmd {}
*/

static int
NsfUnsetUnknownArgsCmd(Tcl_Interp *interp) {
  CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
  Proc *proc = Tcl_CallFrame_procPtr(varFramePtr);

  if (proc) {
    CompiledLocal *ap;
    Var *varPtr;
    int i;

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

/*
cmd configure NsfConfigureCmd {
  {-argName "configureoption" -required 1 -type "debug|dtrace|filter|profile|softrecreate|objectsystems|keepinitcmd|checkresults|checkarguments"}
  {-argName "value" -required 0 -type tclobj}
}
*/
static int
NsfConfigureCmd(Tcl_Interp *interp, int configureoption, Tcl_Obj *valueObj) {
  int bool;

#if defined(NSF_DTRACE)
  if (NSF_DTRACE_CONFIGURE_PROBE_ENABLED()) {
    /* TODO: opts copied from tclAPI.h; maybe make global value? */
    static CONST char *opts[] = {
      "debug", "dtrace", "filter", "profile", "softrecreate",
      "objectsystems", "keepinitcmd", "checkresults", "checkarguments", NULL};
    NSF_DTRACE_CONFIGURE_PROBE((char *)opts[configureoption-1], valueObj ? ObjStr(valueObj) : NULL);
  }
#endif

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

  /*
   * All other configure options are boolean
   */
  if (valueObj) {
    int result = Tcl_GetBooleanFromObj(interp, valueObj, &bool);
    if (result != TCL_OK) {
      return result;
    }
  }

  switch (configureoption) {
  case ConfigureoptionFilterIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doFilters));
    if (valueObj) {
      RUNTIME_STATE(interp)->doFilters = bool;
    }
    break;

  case ConfigureoptionProfileIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doProfile));
    if (valueObj) {
#if defined(NSF_PROFILE)
      RUNTIME_STATE(interp)->doProfile = bool;
#else
      NsfLog(interp, NSF_LOG_WARN, "No profile support compiled in");
#endif
    }
    break;

  case ConfigureoptionSoftrecreateIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doSoftrecreate));
    if (valueObj) {
      RUNTIME_STATE(interp)->doSoftrecreate = bool;
    }
    break;

  case ConfigureoptionKeepinitcmdIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doKeepinitcmd));
    if (valueObj) {
      RUNTIME_STATE(interp)->doKeepinitcmd = bool;
    }
    break;

  case ConfigureoptionCheckresultsIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doCheckResults));
    if (valueObj) {
      RUNTIME_STATE(interp)->doCheckResults = bool;
    }
    break;

  case ConfigureoptionCheckargumentsIdx:
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                      (RUNTIME_STATE(interp)->doCheckArguments));
    if (valueObj) {
      RUNTIME_STATE(interp)->doCheckArguments = bool;
    }
    break;
  }
  return TCL_OK;
}


/*
cmd colon NsfColonCmd {
  {-argName "args" -type allargs}
}
*/
static int
NsfColonCmd(Tcl_Interp *interp, int nobjc, Tcl_Obj *CONST nobjv[]) {
  NsfObject *self = GetSelfObj(interp);
  if (!self) {
    return NsfNoCurrentObjectError(interp, ObjStr(nobjv[0]));
  }
  /* fprintf(stderr, "Colon dispatch %s.%s\n", ObjectName(self), ObjStr(nobjv[0]));*/
  return ObjectDispatch(self, interp, nobjc, nobjv, NSF_CM_NO_SHIFT);
}

/*
cmd finalize NsfFinalizeObjCmd {
}
*/
static int
NsfFinalizeObjCmd(Tcl_Interp *interp) {
  int result;

  /*fprintf(stderr, "+++ call tcl-defined exit handler\n");  */

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
  MEM_COUNT_FREE("TclNamespace",RUNTIME_STATE(interp)->NsfClassesNS);
  MEM_COUNT_FREE("TclNamespace",RUNTIME_STATE(interp)->NsfNS);
  Tcl_DeleteNamespace(RUNTIME_STATE(interp)->NsfClassesNS);
  Tcl_DeleteNamespace(RUNTIME_STATE(interp)->NsfNS);
#endif

  return TCL_OK;
}


/*
cmd interp NsfInterpObjCmd {
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
  }
  return TCL_OK;
}

/*
cmd invalidateobjectparameter NsfInvalidateObjectParameterCmd {
  {-argName "class" -type class}
}
*/
static int
NsfInvalidateObjectParameterCmd(Tcl_Interp *interp, NsfClass *cl) {
  if (cl->parsedParamPtr) {
    /*fprintf(stderr, "   %s invalidate %p\n", ClassName(cl), cl->parsedParamPtr);*/
    ParsedParamFree(INTERP cl->parsedParamPtr);
    cl->parsedParamPtr = NULL;
  }
  return TCL_OK;
}

/*
cmd is NsfIsCmd {
  {-argName "-complain"}
  {-argName "constraint" -required 1 -type tclobj}
  {-argName "value" -required 1 -type tclobj}
}
*/
static int
NsfIsCmd(Tcl_Interp *interp, int withComplain, Tcl_Obj *constraintObj, Tcl_Obj *valueObj) {
  Nsf_Param *paramPtr = NULL;
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
cmd method::alias NsfMethodAliasCmd {
  {-argName "object" -type object}
  {-argName "-per-object"}
  {-argName "methodName"}
  {-argName "-frame" -required 0 -nrargs 1 -type "method|object|default" -default "default"}
  {-argName "cmdName" -required 1 -type tclobj}
}
*/
static int
NsfMethodAliasCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object,
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
     * refCount.
     */
    /*fprintf(stderr, "registering an object %p\n", tcd);*/

    NsfObjectRefCountIncr((NsfObject *)Tcl_Command_objClientData(cmd));

    /*newObjProc = NsfProcAliasMethod;*/

  } else if (CmdIsProc(cmd)) {
    /*
     * When we have a Tcl proc|nsf-method as alias, then use the
     * wrapper, which will be deleted automatically when the original
     * proc/method is deleted.
     */
    newObjProc = NsfProcAliasMethod;

    if (objProc == TclObjInterpProc) {
      /*
       * We have an alias to a tcl proc;
       */
      Proc *procPtr = (Proc *)Tcl_Command_objClientData(cmd);
      Tcl_Obj *bodyObj = procPtr->bodyPtr;

      if (bodyObj->typePtr == Nsf_OT_byteCodeType) {
	/*
	 * Flush old byte code
	 */
	/*fprintf(stderr, "flush byte code\n");*/
	bodyObj->typePtr->freeIntRepProc(bodyObj);
      }
    }

    if (withFrame && withFrame != FrameDefaultIdx) {
      return NsfPrintError(interp,
			  "cannot use -frame object|method in alias for scripted command '%s'",
			   ObjStr(cmdName));
    }
  }

  if (newObjProc) {
    /* add a wrapper */
    /*fprintf(stderr, "NsfMethodAliasCmd cmd %p\n", cmd);*/
    NsfCommandPreserve(INTERP cmd);
    tcd = NEW(AliasCmdClientData);
    tcd->cmdName    = object->cmdName;
    tcd->interp     = interp; /* just for deleting the associated variable */
    tcd->object     = NULL;
    tcd->class	    = cl ? (NsfClass *) object : NULL;
    tcd->objProc    = objProc;
    tcd->aliasedCmd = cmd;
    tcd->clientData = Tcl_Command_objClientData(cmd);

    objProc         = newObjProc;
    deleteProc      = AliasCmdDeleteProc;
    if (tcd->cmdName) {INCR_REF_COUNT(tcd->cmdName);}
  } else {
    /*
     * Call the command directly (must be a c-implemented command not
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
    result = NsfAddObjectMethod(interp, (Nsf_Object *)object, methodName,
                                  objProc, tcd, deleteProc, flags);
    nsPtr = object->nsPtr;
  }

  if (result == TCL_OK) {
    newCmd = FindMethod(nsPtr, methodName);
  }

#if defined(WITH_IMPORT_REFS)
  if (newObjProc) {
    /*
     * Define the reference chain like for 'namespace import' to
     * obtain automatic deletes when the original command is deleted.
     */
    ImportRef *refPtr = (ImportRef *) ckalloc(sizeof(ImportRef));
    refPtr->importedCmdPtr = (Command *) newCmd;
    refPtr->nextPtr = ((Command *) tcd->aliasedCmd)->importRefPtr;
    ((Command *) tcd->aliasedCmd)->importRefPtr = refPtr;
    tcd->aliasCmd = newCmd;
  }
#else
  if (newObjProc) {
    tcd->aliasCmd = newCmd;
  }
#endif

  if (newCmd) {
    AliasAdd(interp, object->cmdName, methodName, cl == NULL, ObjStr(cmdName));

    if (withFrame == FrameMethodIdx) {
      Tcl_Command_flags(newCmd) |= NSF_CMD_NONLEAF_METHOD;
      /*fprintf(stderr, "setting aliased for cmd %p %s flags %.6x, tcd = %p\n",
        newCmd, methodName, Tcl_Command_flags(newCmd), tcd);*/
    }

    Tcl_SetObjResult(interp, MethodHandleObj(object, cl == NULL, methodName));
    result = TCL_OK;
  }

  return result;
}

/*
cmd method::assertion NsfMethodAssertionCmd {
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
NsfMethodAssertionCmd(Tcl_Interp *interp, NsfObject *object, int subcmd, Tcl_Obj *arg) {
#if defined(NSF_WITH_ASSERTIONS)
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
      NsfObjectOpt *opt = NsfRequireObjectOpt(INTERP object);
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
      NsfClassOpt *opt = NsfRequireClassOpt(INTERP class);
      AssertionSetInvariants(interp, &opt->assertions, arg);
    } else {
      if (class->opt && class->opt->assertions) {
        Tcl_SetObjResult(interp, AssertionList(interp, class->opt->assertions->invariants));
      }
    }
  }
#endif
  return TCL_OK;
}

/*
cmd method::create NsfMethodCreateCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-inner-namespace"}
  {-argName "-per-object"}
  {-argName "-reg-object" -required 0 -nrargs 1 -type object}
  {-argName "name" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
  {-argName "-precondition"  -nrargs 1 -type tclobj}
  {-argName "-postcondition" -nrargs 1 -type tclobj}
}
*/
static int
NsfMethodCreateCmd(Tcl_Interp *interp, NsfObject *defObject,
	     int withInner_namespace, int withPer_object, NsfObject *regObject,
	     Tcl_Obj *nameObj, Tcl_Obj *arguments, Tcl_Obj *body,
	     Tcl_Obj *withPrecondition, Tcl_Obj *withPostcondition) {
  NsfClass *cl =
    (withPer_object || ! NsfObjectIsClass(defObject)) ?
    NULL : (NsfClass *)defObject;

  if (cl == 0) {
    RequireObjNamespace(interp, defObject);
  }
  return MakeMethod(interp, defObject, regObject, cl,
		    nameObj, arguments, body,
                    withPrecondition, withPostcondition,
                    withInner_namespace);
}

/*
cmd "method::delete" NsfMethodDeleteCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "methodName" -required 1 -type tclobj}
}
*/
static int
NsfMethodDeleteCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object,
		   Tcl_Obj *methodNameObj) {
  return NsfMethodCreateCmd(interp, object, 0, withPer_object, NULL, methodNameObj,
			    NsfGlobalObjs[NSF_EMPTY],  NsfGlobalObjs[NSF_EMPTY],
			    NULL, NULL);
}

/*
cmd method::forward NsfMethodForwardCmd {
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
NsfMethodForwardCmd(Tcl_Interp *interp,
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
				  (Tcl_ObjCmdProc *)NsfForwardMethod,
				  tcd, ForwardCmdDeleteProc, 0);
    } else {
      result = NsfAddClassMethod(interp, (Nsf_Class *)cl, methodName,
				 (Tcl_ObjCmdProc *)NsfForwardMethod,
				 tcd, ForwardCmdDeleteProc, 0);
    }
    if (result == TCL_OK) {
      Tcl_SetObjResult(interp, MethodHandleObj(object, withPer_object, methodName));
    }
  }

  if (result != TCL_OK && tcd) {
    ForwardCmdDeleteProc(tcd);
  }
  return result;
}

/*
cmd ::method::property NsfMethodPropertyCmd {
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
  Tcl_Command cmd;
  NsfClass *cl = withPer_object == 0 && NsfObjectIsClass(object) ? (NsfClass *)object : NULL;
  int flag, fromClassNS = cl != NULL;

  Tcl_DStringInit(dsPtr);
  cmd = ResolveMethodName(interp, cl ? cl->nsPtr : object->nsPtr, methodObj,
			  dsPtr, &regObject, &defObject, &methodName1, &fromClassNS);

  /*fprintf(stderr, "methodProperty for method '%s' prop %d value %s => cl %p cmd %p\n",
    methodName, methodproperty, valueObj ? ObjStr(valueObj) : "NULL", cl, cmd);*/

  if (!cmd) {
    Tcl_DStringFree(dsPtr);
    return NsfPrintError(interp, "Cannot lookup %smethod '%s' for %s",
			 cl == 0 ? "object " : "",
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
	  paramDefs = ParamDefsNew(INTERP1);
	  ParamDefsStore(INTERP cmd, paramDefs);
	  /*fprintf(stderr, "new param defs %p for cmd %p %s\n", paramDefs, cmd, methodName);*/
	}
	objPtr = 
	  methodproperty == MethodpropertySlotobjIdx ?
	  &paramDefs->slotObj : &paramDefs->returns;

	/* Set a new value; if there is already a value, free it */
	if (*objPtr) {
	  DECR_REF_COUNT2("paramDefsObj", *objPtr);
	}
	if (*valueString == '\0') {
	  /* set the value to NULL */
	  *objPtr = NULL;
	} else {
	  *objPtr = valueObj;
	  INCR_REF_COUNT2("paramDefsObj", *objPtr);
	}
      }
      break;
    }
  }

  return TCL_OK;
}

/*
cmd "method::registered" NsfMethodRegisteredCmd {
  {-argName "handle" -required 1 -type tclobj}
}
*/
static int
NsfMethodRegisteredCmd(Tcl_Interp *interp, Tcl_Obj *methodNameObj) {
  NsfObject *regObject, *defObject;
  CONST char *methodName1 = NULL;
  int fromClassNS = 0;
  Tcl_DString ds, *dsPtr = &ds;
  Tcl_Command cmd;

  Tcl_DStringInit(dsPtr);
  cmd = ResolveMethodName(interp, NULL, methodNameObj,
			  dsPtr, &regObject, &defObject, &methodName1, &fromClassNS);
  /*
   * In case the provided cmd is fully qualified and refers to a registered
   * method, the function returns the object, on which the method was
   * registed.
   */
  Tcl_SetObjResult(interp, (cmd && regObject) ? regObject->cmdName : NsfGlobalObjs[NSF_EMPTY]);

  Tcl_DStringFree(dsPtr);
  return TCL_OK;
}

/*
cmd method::setter NsfMethodSetterCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "parameter" -type tclobj}
  }
*/
static int
NsfMethodSetterCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, Tcl_Obj *parameter) {
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
  setterClientData->object = NULL;
  setterClientData->paramsPtr = NULL;
  INTERP_MEMBER_SET(setterClientData, interp)
  length = strlen(methodName);

  for (j=0; j<length; j++) {
    if (methodName[j] == ':' || methodName[j] == ' ') break;
  }

  if (j < length) {
    /* looks as if we have a parameter specification */
    int result, possibleUnknowns = 0, plainParams = 0, nrNonposArgs = 0;

    setterClientData->paramsPtr = ParamsNew(INTERP 1);
    result = ParamParse(interp, NsfGlobalObjs[NSF_SETTER], parameter,
                        NSF_DISALLOWED_ARG_SETTER|NSF_ARG_HAS_DEFAULT,
                        setterClientData->paramsPtr, &possibleUnknowns,
			&plainParams, &nrNonposArgs);

    if (result != TCL_OK) {
      SetterCmdDeleteProc(setterClientData);
      return result;
    }
    methodName = setterClientData->paramsPtr->name;
  } else {
    setterClientData->paramsPtr = NULL;
  }

  if (cl) {
    result = NsfAddClassMethod(interp, (Nsf_Class *)cl, methodName,
			       (Tcl_ObjCmdProc *)NsfSetterMethod,
			       setterClientData, SetterCmdDeleteProc, 0);
  } else {
    result = NsfAddObjectMethod(interp, (Nsf_Object *)object, methodName,
				(Tcl_ObjCmdProc *)NsfSetterMethod,
				setterClientData, SetterCmdDeleteProc, 0);
  }
  if (result == TCL_OK) {
    Tcl_SetObjResult(interp, MethodHandleObj(object, cl == NULL, methodName));
  } else {
    SetterCmdDeleteProc(setterClientData);
  }
  return result;
}



/*
cmd "object::dispatch" NsfObjectDispatchCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-frame" -required 0 -nrargs 1 -type "method|object|default" -default "default"}
  {-argName "command" -required 1 -type tclobj}
  {-argName "args"  -type args}
}
*/
static int
NsfObjectDispatchCmd(Tcl_Interp *interp, NsfObject *object, int withFrame,
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
    result = MethodDispatch(object, interp,
			    nobjc+1, nobjv-1, cmd, object,
			    NULL /*NsfClass *cl*/,
			    Tcl_GetCommandName(interp, cmd),
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
    result = NsfCallMethodWithArgs(interp, (Nsf_Object *)object, command, arg,
				   nobjc, objv, NSF_CM_NO_UNKNOWN|NSF_CSC_IMMEDIATE);
  }

  return result;
}

/*
cmd "object::exists" NsfObjectExistsCmd {
  {-argName "value" -required 1 -type tclobj}
}
*/
static int
NsfObjectExistsCmd(Tcl_Interp *interp, Tcl_Obj *valueObj) {
  NsfObject *object;

  /*
   * Pass the object as Tcl_Obj, since we do not want to raise an error in
   * case the object does not exist.
   */
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), GetObjectFromObj(interp, valueObj, &object) == TCL_OK);
  return TCL_OK;
}

/*
cmd "object::property" NsfObjectPropertyCmd {
  {-argName "objectName" -required 1 -type object}
  {-argName "objectproperty" -type "initialized|class|rootmetaclass|rootclass|slotcontainer" -required 1}
}
*/
static int
NsfObjectPropertyCmd(Tcl_Interp *interp, NsfObject *object, int objectproperty) {
  int flags = 0;;

  switch (objectproperty) {
  case ObjectpropertyInitializedIdx: flags = NSF_INIT_CALLED; break;
  case ObjectpropertyClassIdx: flags = NSF_IS_CLASS; break;
  case ObjectpropertyRootmetaclassIdx: flags = NSF_IS_ROOT_META_CLASS; break;
  case ObjectpropertyRootclassIdx: flags = NSF_IS_ROOT_CLASS; break;
  case ObjectpropertySlotcontainerIdx: flags = NSF_IS_SLOT_CONTAINER; break;
  }
  
   Tcl_SetObjResult(interp, 
		   NsfGlobalObjs[(object->flags & flags) ? 
				 NSF_ONE : NSF_ZERO]);
  return TCL_OK;
}

/*
cmd "object::qualify" NsfObjectQualifyCmd {
  {-argName "objectName" -required 1 -type tclobj}
}
*/
static int
NsfObjectQualifyCmd(Tcl_Interp *interp, Tcl_Obj *nameObj) {
  CONST char *nameString = ObjStr(nameObj);

  if (isAbsolutePath(nameString)) {
    Tcl_SetObjResult(interp, nameObj);
  } else {
    Tcl_SetObjResult(interp, NameInNamespaceObj(interp, nameString, CallingNameSpace(interp)));
  }
  return TCL_OK;
}

/*
cmd "objectsystem::create" NsfObjectSystemCreateCmd {
  {-argName "rootClass" -required 1 -type tclobj}
  {-argName "rootMetaClass" -required 1 -type tclobj}
  {-argName "systemMethods" -required 0 -type tclobj}
}
*/
static int
NsfObjectSystemCreateCmd(Tcl_Interp *interp, Tcl_Obj *Object, Tcl_Obj *Class, Tcl_Obj *systemMethodsObj) {
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

  GetClassFromObj(interp, object, &theobj, 0);
  GetClassFromObj(interp, class, &thecls, 0);

  if (theobj || thecls) {
    ObjectSystemFree(interp, osPtr);
    NsfLog(interp, NSF_LOG_WARN, "Base class '%s' exists already; ignoring definition",
	   theobj ? objectName : className);
    return TCL_OK;
  }

  if (systemMethodsObj) {
    int oc, idx, result;
    Tcl_Obj **ov;

    if ((result = Tcl_ListObjGetElements(interp, systemMethodsObj, &oc, &ov)) == TCL_OK) {
      int i;

      if (oc % 2) {
        ObjectSystemFree(interp, osPtr);
        return NsfPrintError(interp, "System methods must be provided as pairs");
      }
      for (i=0; i<oc; i += 2) {
	Tcl_Obj *arg, **arg_ov;
	int arg_oc = -1;

	arg = ov[i+1];
        result = Tcl_GetIndexFromObj(interp, ov[i], Nsf_SytemMethodOpts, "system method", 0, &idx);
	if (result == TCL_OK) {
	  result = Tcl_ListObjGetElements(interp, arg, &arg_oc, &arg_ov);
	}
        if (result != TCL_OK) {
          ObjectSystemFree(interp, osPtr);
          return NsfPrintError(interp, "invalid system method '%s'", ObjStr(ov[i]));
        } else if (arg_oc < 1 || arg_oc > 2) {
          ObjectSystemFree(interp, osPtr);
          return NsfPrintError(interp, "invalid system method argument '%s'", ObjStr(ov[i]), ObjStr(arg));
	}
        /*fprintf(stderr, "NsfCreateObjectSystemCmd [%d] = %p %s (max %d, given %d)\n",
          idx, ov[i+1], ObjStr(ov[i+1]), XO_unknown_idx, oc);*/

	if (arg_oc == 1) {
	  osPtr->methods[idx] = arg;
	} else { /* (arg_oc == 2) */
	  osPtr->methods[idx] = arg_ov[0];
	  osPtr->handles[idx] = arg_ov[1];
	  INCR_REF_COUNT(osPtr->handles[idx]);
	}
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

  /* check whether Object and Class creation was successful */
  if (!theobj || !thecls) {

    if (thecls) PrimitiveCDestroy(thecls);
    if (theobj) PrimitiveCDestroy(theobj);

    ObjectSystemFree(interp, osPtr);
    return NsfPrintError(interp, "Creation of object system failed");
  }

  theobj->osPtr = osPtr;
  thecls->osPtr = osPtr;
  osPtr->rootClass = theobj;
  osPtr->rootMetaClass = thecls;

  theobj->object.flags |= NSF_IS_ROOT_CLASS|NSF_INIT_CALLED;
  thecls->object.flags |= NSF_IS_ROOT_META_CLASS|NSF_INIT_CALLED;

  ObjectSystemAdd(interp, osPtr);

  AddInstance((NsfObject *)theobj, thecls);
  AddInstance((NsfObject *)thecls, thecls);
  AddSuper(INTERP thecls, theobj);

  if (NSF_DTRACE_OBJECT_ALLOC_ENABLED()) {
    NSF_DTRACE_OBJECT_ALLOC(ObjectName(((NsfObject *)theobj)), ClassName(((NsfObject *)theobj)->cl));
    NSF_DTRACE_OBJECT_ALLOC(ObjectName(((NsfObject *)thecls)), ClassName(((NsfObject *)thecls)->cl));
  }

  return TCL_OK;
}


/*
cmd my NsfMyCmd {
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
    return NsfNoCurrentObjectError(interp, ObjStr(nobjv[0]));
  }

  if (withLocal) {
    NsfClass *cl = self->cl;
    CONST char *methodName = ObjStr(methodObj);
    Tcl_Command cmd = FindMethod(cl->nsPtr, methodName);
    if (cmd == NULL) {
      return NsfPrintError(interp, "%s: unable to dispatch local method '%s' in class %s",
			   ObjectName(self), methodName, ClassName(cl));
    }
    result = MethodDispatch(self, interp, nobjc+2, nobjv, cmd, self, cl,
			    methodName, 0, 0);
  } else {
#if 0
    /* TODO attempt to make "my" NRE-enabled, failed so far (crash in mixinInheritanceTest) */
    int flags;
    NsfCallStackContent *cscPtr = CallStackGetTopFrame0(interp);
    if (!cscPtr || self != cscPtr->self) {
      flags = NSF_CSC_IMMEDIATE;
    } else {
      flags = NsfImmediateFromCallerFlags(cscPtr->flags);
      fprintf(stderr, "XXX MY %s.%s frame has flags %.6x -> next-flags %.6x\n",
	      ObjectName(self), ObjStr(methodObj), cscPtr->flags, flags);
    }
    result = CallMethod(self, interp, methodObj, nobjc+2, nobjv, flags);
#else
    result = CallMethod(self, interp, methodObj, nobjc+2, nobjv, NSF_CSC_IMMEDIATE);
#endif
  }
  return result;
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
cmd next NsfNextCmd {
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
cmd nscopycmds NsfNSCopyCmdsCmd {
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

  if (TclGetNamespaceFromObj(interp, fromNs, &fromNsPtr) != TCL_OK) {
    return TCL_OK;
  }

  name = ObjStr(fromNs);

  /* check, if we work on an object or class namespace */
  object = GetObjectFromNsName(interp, name, &fromClassNS);

  if (object == NULL) {
    return NsfPrintError(interp, "argument 1 '%s' is not an object", ObjStr(fromNs));
  }

  cl = fromClassNS ? (NsfClass *)object : NULL;

  if (TclGetNamespaceFromObj(interp, toNs, &toNsPtr) != TCL_OK) {
    return NsfPrintError(interp, "CopyCmds: Destination namespace %s does not exist",
			 ObjStr(toNs));
  }
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
        Proc *procPtr = (Proc *)Tcl_Command_objClientData(cmd);
        Tcl_Obj *arglistObj;
        int result;

        /*
         * Build a list containing the arguments of the proc
         */
        result = ListCmdParams(interp, cmd, oldName, NSF_PARAMS_PARAMETER);
        if (result != TCL_OK) {
          return result;
        }

        arglistObj = Tcl_GetObjResult(interp);
        INCR_REF_COUNT(arglistObj);

        if (Tcl_Command_objProc(cmd) == RUNTIME_STATE(interp)->objInterpProc) {
          Tcl_DString ds, *dsPtr = &ds;

          if (cl) {
            /* Next Scripting class-methods */
#if defined(NSF_WITH_ASSERTIONS)
            NsfProcAssertion *procs;
            procs = cl->opt ? AssertionFindProcs(cl->opt->assertions, name) : NULL;
#endif

            DSTRING_INIT(dsPtr);
            Tcl_DStringAppendElement(dsPtr, "::nsf::method::create");
            Tcl_DStringAppendElement(dsPtr, NSCutNsfClasses(toNsPtr->fullName));
            Tcl_DStringAppendElement(dsPtr, name);
            Tcl_DStringAppendElement(dsPtr, ObjStr(arglistObj));
            Tcl_DStringAppendElement(dsPtr, StripBodyPrefix(ObjStr(procPtr->bodyPtr)));
#if defined(NSF_WITH_ASSERTIONS)
            if (procs) {
              NsfRequireClassOpt(INTERP cl);
              AssertionAppendPrePost(interp, dsPtr, procs);
            }
#endif
            Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr), 0);
            DSTRING_FREE(dsPtr);

          } else {
            /* Next Scripting object-methods */
            NsfObject *object = GetObjectFromString(interp, fromNsPtr->fullName);
#if defined(NSF_WITH_ASSERTIONS)
            NsfProcAssertion *procs;
#endif

            if (object) {
#if defined(NSF_WITH_ASSERTIONS)
              procs = object->opt ? AssertionFindProcs(object->opt->assertions, name) : NULL;
#endif
            } else {
              DECR_REF_COUNT(newFullCmdName);
              DECR_REF_COUNT(oldFullCmdName);
              DECR_REF_COUNT(arglistObj);
              return NsfPrintError(interp, "No object for assertions");
            }

            DSTRING_INIT(dsPtr);
            Tcl_DStringAppendElement(dsPtr, "::nsf::method::create");
            Tcl_DStringAppendElement(dsPtr, toNsPtr->fullName);
            Tcl_DStringAppendElement(dsPtr, "-per-object");
            Tcl_DStringAppendElement(dsPtr, name);
            Tcl_DStringAppendElement(dsPtr, ObjStr(arglistObj));
            Tcl_DStringAppendElement(dsPtr, StripBodyPrefix(ObjStr(procPtr->bodyPtr)));
#if defined(NSF_WITH_ASSERTIONS)
            if (procs) {
              NsfRequireObjectOpt(INTERP object);
              AssertionAppendPrePost(interp, dsPtr, procs);
            }
#endif
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
cmd nscopyvars NsfNSCopyVars {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
*/
static int
NsfNSCopyVarsCmd(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs) {
  Tcl_Namespace *fromNsPtr = NULL, *toNsPtr;
  Var *varPtr = NULL;
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;
  TclVarHashTable *varTablePtr;
  NsfObject *object, *destObject;
  CONST char *destFullName;
  Tcl_Obj *destFullNameObj;
  Tcl_CallFrame frame, *framePtr = &frame;
  Tcl_Obj *varNameObj = NULL;

  TclGetNamespaceFromObj(interp, fromNs, &fromNsPtr);

  if (fromNsPtr) {
    if (TclGetNamespaceFromObj(interp, toNs, &toNsPtr) != TCL_OK) {
      return NsfPrintError(interp, "CopyVars: Destination namespace %s does not exist",
			   ObjStr(toNs));
    }

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
  hPtr = varTablePtr ? Tcl_FirstHashEntry(TclVarHashTablePtr(varTablePtr), &hSrch) : NULL;
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
	     ObjStr(TclVarValue(Tcl_Obj, varPtr, objPtr)));*/

	  Nsf_ObjSetVar2((Nsf_Object *)destObject, interp, varNameObj, NULL,
			 TclVarValue(Tcl_Obj, varPtr, objPtr), 0);
        } else {
          Tcl_ObjSetVar2(interp, varNameObj, NULL,
                         TclVarValue(Tcl_Obj, varPtr, objPtr),
                         TCL_NAMESPACE_ONLY);
        }
      } else {
        if (TclIsVarArray(varPtr)) {
          /* HERE!! PRE85 Why not [array get/set] based? Let the core iterate */
          TclVarHashTable *aTable = TclVarValue(TclVarHashTable, varPtr, tablePtr);
          Tcl_HashSearch ahSrch;
          Tcl_HashEntry *ahPtr = aTable ? Tcl_FirstHashEntry(TclVarHashTablePtr(aTable), &ahSrch) :0;
          for (; ahPtr; ahPtr = Tcl_NextHashEntry(&ahSrch)) {
            Tcl_Obj *eltNameObj;
            Var *eltVar;

            GetVarAndNameFromHash(ahPtr, &eltVar, &eltNameObj);
            INCR_REF_COUNT(eltNameObj);

            if (TclIsVarScalar(eltVar)) {
              if (object) {
                Nsf_ObjSetVar2((Nsf_Object *)destObject, interp, varNameObj, eltNameObj,
				 TclVarValue(Tcl_Obj, eltVar, objPtr), 0);
              } else {
                Tcl_ObjSetVar2(interp, varNameObj, eltNameObj,
                               TclVarValue(Tcl_Obj, eltVar, objPtr),
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
cmd proc NsfProcCmd {
  {-argName "-ad" -required 0}
  {-argName "procName" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
}
*/
static int
NsfProcCmd(Tcl_Interp *interp, int with_ad, Tcl_Obj *nameObj, Tcl_Obj *arguments, Tcl_Obj *body) {
  NsfParsedParam parsedParam;
  int result;
  /*
   * Parse argument list "arguments" to determine if we should provide
   * nsf parameter handling.
   */
  result = ParamDefsParse(interp, nameObj, arguments,
			  NSF_DISALLOWED_ARG_METHOD_PARAMETER, 0,
			  &parsedParam);
  if (result != TCL_OK) {
    return result;
  }

  if (parsedParam.paramDefs) {
    /*
     * We need parameter handling. In such cases, a thin C-based layer
     * is added which handles the parameter passing and calls the proc
     * later.
     */
    result = NsfAddParameterProc(interp, &parsedParam, ObjStr(nameObj), body, with_ad);

  } else {
    /*
     * No parameter handling needed. A plain Tcl proc is added.
     */
    Tcl_Obj *ov[4];

    ov[0] = NULL;
    ov[1] = nameObj;
    ov[2] = arguments;
    ov[3] = body;
    result = Tcl_ProcObjCmd(0, interp, 4, ov);
  }

  return result;
}

/*
cmd relation NsfRelationCmd {
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
    if (Tcl_ListObjGetElements(interp, valueObj, &oc, &ov) != TCL_OK) {
      return TCL_ERROR;
    }
    objopt = NsfRequireObjectOpt(INTERP object);
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

    if (Tcl_ListObjGetElements(interp, valueObj, &oc, &ov) != TCL_OK) {
      return TCL_ERROR;
    }
    clopt = NsfRequireClassOpt(INTERP cl);
    break;

  case RelationtypeSuperclassIdx:
    if (!NsfObjectIsClass(object)) {
      return NsfObjErrType(interp, "superclass", object->cmdName, "class", NULL);
    }
    cl = (NsfClass *)object;
    if (valueObj == NULL) {
      return ListSuperclasses(interp, cl, NULL, 0);
    }
    if (Tcl_ListObjGetElements(interp, valueObj, &oc, &ov) != TCL_OK) {
      return TCL_ERROR;
    }
    return SuperclassAdd(interp, cl, oc, ov, valueObj, cl->object.cl);

  case RelationtypeClassIdx:
    if (valueObj == NULL) {
      Tcl_SetObjResult(interp, object->cl->object.cmdName);
      return TCL_OK;
    }
    GetClassFromObj(interp, valueObj, &cl, 1);
    if (!cl) return NsfObjErrType(interp, "class", valueObj, "a class", NULL);
    i = ChangeClass(interp, object, cl);
    if (i == TCL_OK) {
      Tcl_SetObjResult(interp, object->cl->object.cmdName);
    }
    return i;

  case RelationtypeRootclassIdx:
    {
    NsfClass *metaClass;

    if (!NsfObjectIsClass(object)) {
      return NsfObjErrType(interp, "rootclass", object->cmdName, "class", NULL);
    }
    cl = (NsfClass *)object;

    if (valueObj == NULL) {
      return NsfPrintError(interp, "metaclass must be specified as third argument");
    }
    GetClassFromObj(interp, valueObj, &metaClass, 0);
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
          CmdListRemoveList(INTERP &newMixinCmdList, GuardDel);
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
              CmdListDeleteCmdListEntry(INTERP del, GuardDel);
            }
          }
        }
        CmdListRemoveList(INTERP &objopt->mixins, GuardDel);
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
          nclopt = NsfRequireClassOpt(INTERP (NsfClass *)nObject);
          CmdListAdd(INTERP &nclopt->isObjectMixinOf, object->id, NULL, /*noDuplicates*/ 1);
        } /* else fprintf(stderr, "Problem registering %s as a mixinof of %s\n",
             ObjStr(ov[i]), ClassName(cl)); */
      }

      MixinComputeDefined(interp, object);
      FilterComputeDefined(interp, object);

      break;
    }

  case RelationtypeObject_filterIdx:

    if (objopt->filters) {
      CmdListRemoveList(INTERP &objopt->filters, GuardDel);
    }
    object->flags &= ~NSF_FILTER_ORDER_VALID;
    for (i = 0; i < oc; i ++) {
      if (FilterAdd(interp, &objopt->filters, ov[i], object, 0) != TCL_OK) {
        return TCL_ERROR;
      }
    }
    /*FilterComputeDefined(interp, object);*/
    break;

  case RelationtypeClass_mixinIdx:
    {
      NsfCmdList *newMixinCmdList = NULL;

      for (i = 0; i < oc; i++) {
        if (MixinAdd(interp, &newMixinCmdList, ov[i], cl->object.cl) != TCL_OK) {
          CmdListRemoveList(INTERP &newMixinCmdList, GuardDel);
          return TCL_ERROR;
        }
      }
      if (clopt->classmixins) {
        RemoveFromClassMixinsOf(INTERP cl->object.id, clopt->classmixins);
        CmdListRemoveList(INTERP &clopt->classmixins, GuardDel);
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
          nclopt = NsfRequireClassOpt(INTERP (NsfClass *) nObject);
          CmdListAdd(INTERP &nclopt->isClassMixinOf, cl->object.id, NULL, /*noDuplicates*/ 1);
        } /* else fprintf(stderr, "Problem registering %s as a class-mixin of %s\n",
             ObjStr(ov[i]), ClassName(cl)); */
      }
      break;
    }

  case RelationtypeClass_filterIdx:

    if (clopt->classfilters) {
      CmdListRemoveList(INTERP &clopt->classfilters, GuardDel);
    }
    FilterInvalidateObjOrders(interp, cl);
    for (i = 0; i < oc; i ++) {
      if (FilterAdd(interp, &clopt->classfilters, ov[i], 0, cl) != TCL_OK) {
        return TCL_ERROR;
      }
    }
    break;

  }
  NsfRelationCmd(interp, object, relationtype, NULL);
  return TCL_OK;
}

/*
cmd current NsfCurrentCmd {
  {-argName "currentoption" -required 0 -type "proc|method|methodpath|object|class|activelevel|args|activemixin|calledproc|calledmethod|calledclass|callingproc|callingmethod|callingclass|callinglevel|callingobject|filterreg|isnextcall|next"}
}
*/
static int
NsfCurrentCmd(Tcl_Interp *interp, int selfoption) {
  NsfObject *object =  GetSelfObj(interp);
  NsfCallStackContent *cscPtr;
  Tcl_CallFrame *framePtr;
  int result = TCL_OK;

  if (selfoption == 0 || selfoption == CurrentoptionObjectIdx) {
    if (object) {
      Tcl_SetObjResult(interp, object->cmdName);
      return TCL_OK;
    } else {
      return NsfNoCurrentObjectError(interp, NULL);
    }
  }

  if (!object && selfoption != CurrentoptionCallinglevelIdx) {
    return NsfNoCurrentObjectError(interp, NULL);
  }

  switch (selfoption) {
  case CurrentoptionMethodIdx: /* fall through */
  case CurrentoptionProcIdx:
    cscPtr = CallStackGetTopFrame0(interp);
    if (cscPtr) {
      CONST char *procName = Tcl_GetCommandName(interp, cscPtr->cmdPtr);
      Tcl_SetResult(interp, (char *)procName, TCL_VOLATILE);
    } else {
      return NsfPrintError(interp,  "Can't find proc");
    }
    break;

  case CurrentoptionMethodpathIdx:
    cscPtr = CallStackGetTopFrame(interp, &framePtr);
    Tcl_SetObjResult(interp, CallStackMethodPath(interp, framePtr));
    break;

  case CurrentoptionClassIdx: /* class subcommand */
    cscPtr = CallStackGetTopFrame0(interp);
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
    if (RUNTIME_STATE(interp)->currentMixinCmdPtr) {
      object = NsfGetObjectFromCmdPtr(RUNTIME_STATE(interp)->currentMixinCmdPtr);
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
  case CurrentoptionCallingprocIdx: {
    Tcl_Obj *resultObj;

    cscPtr = NsfCallStackFindLastInvocation(interp, 1, &framePtr);
    if (cscPtr && cscPtr->cmdPtr) {
      Tcl_Obj *methodNameObj = Tcl_NewStringObj(Tcl_GetCommandName(interp, cscPtr->cmdPtr), -1);
      /* 
       * By checking the characteristic frame and call type pattern for "leaf"
       * ensemble dispatches, we make sure that the method path is only
       * reported for these cases. Otherwise, we constrain the result to the
       * method name.
       */
      if ((cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) && 
	  (cscPtr->flags & NSF_CSC_CALL_IS_COMPILE) == 0) {
	resultObj = CallStackMethodPath(interp, framePtr);
	result = Tcl_ListObjAppendElement(interp, resultObj, methodNameObj);
	if (result != TCL_OK) {
	  DECR_REF_COUNT(resultObj);
	  DECR_REF_COUNT(methodNameObj);
	  break;
	}
      } else {
	resultObj = methodNameObj;
      }
    } else {
      resultObj = NsfGlobalObjs[NSF_EMPTY];
    }
    Tcl_SetObjResult(interp, resultObj);
    break;
  }
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
cmd self NsfSelfCmd {
}
*/
static int
NsfSelfCmd(Tcl_Interp *interp) {
  NsfObject *object = GetSelfObj(interp);

  if (object) {
    Tcl_SetObjResult(interp, object->cmdName);
    return TCL_OK;
  } else {
    return NsfNoCurrentObjectError(interp, NULL);
  }
}

/*
cmd var::exists NsfVarExistsCmd {
  {-argName "-array" -required 0 -nrargs 0}
  {-argName "object" -required 1 -type object}
  {-argName "varname" -required 1}
}
*/
static int
NsfVarExistsCmd(Tcl_Interp *interp, int withArray, NsfObject *object, CONST char *varName) {
  int flags =  
    NSF_VAR_TRIGGER_TRACE|NSF_VAR_REQUIRE_DEFINED|
    (withArray ? NSF_VAR_ISARRAY : 0);

  if (CheckVarName(interp, varName) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_SetIntObj(Tcl_GetObjResult(interp), VarExists(interp, object, varName, NULL, flags));

  return TCL_OK;
}

/*
cmd var::import NsfVarImportCmd {
  {-argName "object" -type object}
  {-argName "args" -type args}
}
*/
static int
NsfVarImport(Tcl_Interp *interp, NsfObject *object, const char *cmdName, int objc, Tcl_Obj *CONST objv[]) {
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
NsfVarImportCmd(Tcl_Interp *interp, NsfObject *object, int objc, Tcl_Obj *CONST objv[]) {
  return NsfVarImport(interp, object, "importvar", objc, objv);
}

/*
cmd var::set NsfVarSetCmd {
  {-argName "-array" -required 0 -nrargs 0}
  {-argName "object" -required 1 -type object}
  {-argName "varname" -required 1 -type tclobj}
  {-argName "value" -required 0 -type tclobj}
}
*/
static int
NsfVarSetCmd(Tcl_Interp *interp, int withArray,
	     NsfObject *object, Tcl_Obj *varname, Tcl_Obj *valueObj) {

  if (CheckVarName(interp, ObjStr(varname)) != TCL_OK) {
    return TCL_ERROR;
  }
  if (withArray) {
    return SetInstArray(interp, object, varname, valueObj);
  } else {
    return SetInstVar(interp, object, varname, valueObj);
  }
}

/*
cmd var::unset NsfVarUnsetCmd {
  {-argName "-nocomplain" -required 0 -nrargs 0}
  {-argName "object" -required 1 -type object}
  {-argName "varname" -required 1 -type tclobj}
}
*/
static int
NsfVarUnsetCmd(Tcl_Interp *interp, int withNocomplain, NsfObject *object, Tcl_Obj *varNameObj) {
  char *varName = ObjStr(varNameObj);

  if (CheckVarName(interp, varName) != TCL_OK) {
    return TCL_ERROR;
  }

  return UnsetInstVar(interp, withNocomplain, object, varName);
}
/***********************************************************************
 * End generated Next Scripting  commands
 ***********************************************************************/

/*
 * Parameter support functions
 */

typedef struct NsfParamWrapper {
  Nsf_Param *paramPtr;
  int refCount;
  int canFree;
  INTERP_MEMBER_DECL
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
ParamDupInteralRep(Tcl_Obj *srcPtr, Tcl_Obj *UNUSED(dupPtr)) {

  Tcl_Panic("%s of type %s should not be called", "dupStringProc",
	    srcPtr->typePtr->name);
}

static Tcl_ObjType paramObjType = {
    "nsfParam",				/* name */
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
    INTERP_MEMBER_GET(paramWrapperPtr)

    if (paramWrapperPtr->canFree) {
      ParamsFree(INTERP paramWrapperPtr->paramPtr);
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
  Tcl_Obj *fullParamObj = Tcl_NewStringObj(varNamePrefix, -1);
  int result, possibleUnknowns = 0, plainParams = 0, nrNonposArgs = 0;
  NsfParamWrapper *paramWrapperPtr = NEW(NsfParamWrapper);

  paramWrapperPtr->paramPtr = ParamsNew(INTERP 1);
  paramWrapperPtr->refCount = 1;
  paramWrapperPtr->canFree = 0;
  INTERP_MEMBER_SET(paramWrapperPtr, interp)
  /*fprintf(stderr, "allocating  %p\n", paramWrapperPtr->paramPtr);*/

  Tcl_AppendLimitedToObj(fullParamObj, ObjStr(objPtr), -1, INT_MAX, NULL);
  INCR_REF_COUNT(fullParamObj);
  result = ParamParse(interp, NsfGlobalObjs[NSF_VALUECHECK], fullParamObj,
                      NSF_DISALLOWED_ARG_VALUECHECK /* disallowed options */,
                      paramWrapperPtr->paramPtr, &possibleUnknowns,
		      &plainParams, &nrNonposArgs);
#if defined(DEBUG86B2)
  fprintf(stderr, "ParamParse3 %s paramPtr %p returned ok? %d\n", 
	  ObjStr(fullParamObj),
	  paramWrapperPtr->paramPtr, result==TCL_OK);
#endif
  /*
   * We treat currently unknown user level converters as error.
   */
  if (paramWrapperPtr->paramPtr->flags & NSF_ARG_CURRENTLY_UNKNOWN) {
    result = TCL_ERROR;
  }

  if (result == TCL_OK) {
    /*
     * In success cases, the memory allocated by this function is freed via
     * the tcl_obj type.
     */
    paramWrapperPtr->paramPtr->flags |= NSF_ARG_UNNAMED;
    if (*(paramWrapperPtr->paramPtr->name) == 'r') {
      paramWrapperPtr->paramPtr->flags |= NSF_ARG_IS_RETURNVALUE;
    }
    TclFreeIntRep(objPtr);
    objPtr->internalRep.twoPtrValue.ptr1 = (void *)paramWrapperPtr;
    objPtr->internalRep.twoPtrValue.ptr2 = NULL;
    objPtr->typePtr = &paramObjType;
  } else {
    /*
     * In error cases, free manually memory allocated by this function.
     */
    ParamsFree(INTERP paramWrapperPtr->paramPtr);
    FREE(NsfParamWrapper, paramWrapperPtr);
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
GetObjectParameterDefinition(Tcl_Interp *interp, Tcl_Obj *procNameObj, NsfClass *class,
                             NsfParsedParam *parsedParamPtr) {
  int result;
  Tcl_Obj *rawConfArgs;
  NsfParsedParam *clParsedParamPtr = class->parsedParamPtr;

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
    Tcl_Obj *methodObj = NsfMethodObj(&class->object, NSF_c_objectparameter_idx);

    if (methodObj) {
      /*fprintf(stderr, "=== calling %s objectparameter\n", ClassName(class));*/
      result = CallMethod(class, interp, methodObj,
			  2, 0, NSF_CM_NO_PROTECT|NSF_CSC_IMMEDIATE);

      if (likely(result == TCL_OK)) {
	rawConfArgs = Tcl_GetObjResult(interp);
	/*fprintf(stderr, ".... rawConfArgs for %s => '%s'\n",
	  ClassName(class), ObjStr(rawConfArgs));*/
	INCR_REF_COUNT(rawConfArgs);
	
	/*
	 * Parse the string representation to obtain the internal
	 * representation.
	 */
	result = ParamDefsParse(interp, procNameObj, rawConfArgs,
				NSF_DISALLOWED_ARG_OBJECT_PARAMETER, 1,
				parsedParamPtr);
	if (likely(result == TCL_OK)) {
	  NsfParsedParam *ppDefPtr = NEW(NsfParsedParam);
	  ppDefPtr->paramDefs = parsedParamPtr->paramDefs;
	  ppDefPtr->possibleUnknowns = parsedParamPtr->possibleUnknowns;
	  class->parsedParamPtr = ppDefPtr;
	  if (ppDefPtr->paramDefs) {
	    ParamDefsRefCountIncr(INTERP ppDefPtr->paramDefs);
	  }
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

/*
 *----------------------------------------------------------------------
 * ParameterCheck --
 *
 *    Check the provided valueObj against the parameter specification provided
 *    in the second argument (paramObjPtr), when doCheck is true. This
 *    function is used e.g. by nsf::is, where only the right hand side of a
 *    parameter specification (after the colon) is specified. The argument
 *    Name (before the colon in a parameter spec) is provided via
 *    argNamePrefix. The converted parameter structure is returned optionally
 *    via the last argument.
 *
 * Results:
 *    Tcl return code, parsed structure in last argument
 *
 * Side effects:
 *    Converts potentially tcl_obj type of paramObjPtr
 *
 *----------------------------------------------------------------------
 */

static int
ParameterCheck(Tcl_Interp *interp, Tcl_Obj *paramObjPtr, Tcl_Obj *valueObj,
	       const char *argNamePrefix, int doCheck, Nsf_Param **paramPtrPtr) {
  Nsf_Param *paramPtr;
  NsfParamWrapper *paramWrapperPtr;
  Tcl_Obj *outObjPtr = NULL;
  ClientData checkedData;
  int result, flags = 0;

  /*fprintf(stderr, "ParameterCheck %s value %p %s\n",
    ObjStr(paramObjPtr), valueObj, ObjStr(valueObj));*/

  if (paramObjPtr->typePtr == &paramObjType) {
    paramWrapperPtr = (NsfParamWrapper *) paramObjPtr->internalRep.twoPtrValue.ptr1;
  } else {
    /*
     * We could use in principle Tcl_ConvertToType(..., &paramObjType) instead
     * of checking the type manually, be we want to pass the argNamePrefix
     * explicitly.
     */
    result = ParamSetFromAny2(interp, argNamePrefix, paramObjPtr);
    if (result == TCL_OK) {
      paramWrapperPtr = (NsfParamWrapper *) paramObjPtr->internalRep.twoPtrValue.ptr1;
    } else {
      return NsfPrintError(interp, "invalid value constraints \"%s\"", ObjStr(paramObjPtr));
    }
  }
  paramPtr = paramWrapperPtr->paramPtr;
  if (paramPtrPtr) *paramPtrPtr = paramPtr;

  result = ArgumentCheck(interp, valueObj, paramPtr, doCheck, &flags, &checkedData, &outObjPtr);
  /*fprintf(stderr, "ParameterCheck paramPtr %p final refCount of wrapper %d can free %d flags %.6x\n",
    paramPtr, paramWrapperPtr->refCount,  paramWrapperPtr->canFree, flags);*/

  if (paramWrapperPtr->refCount == 0) {
    fprintf(stderr, "#### ParamSetFromAny2 paramPtr %p manual free\n", paramPtr);
    ParamsFree(INTERP paramWrapperPtr->paramPtr);
    FREE(NsfParamWrapper, paramWrapperPtr);
  } else {
    paramWrapperPtr->canFree = 1;
  }

  if (flags & NSF_PC_MUST_DECR) {
    DECR_REF_COUNT2("valueObj", outObjPtr);
  }

  return result;
}



/***********************************************************************
 * Begin Object Methods
 ***********************************************************************/
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
    DECR_REF_COUNT2("autoname", autoname);

    return TCL_OK;
  }
  return NsfPrintError(interp, "Autoname failed. Probably format string (with %%) was not well-formed");
}

/*
objectMethod class NsfOClassMethod {
  {-argName "class" -required 0 -type tclobj}
}
*/
static int
NsfOClassMethod(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *classObj) {
  return NsfRelationCmd(interp, object, RelationtypeClassIdx, classObj);
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
  int result, i, remainingArgsc, varArgsProcessed = 0;
  NsfParsedParam parsedParam;
  Nsf_Param *paramPtr;
  NsfParamDefs *paramDefs;
  Tcl_Obj *newValue, *initMethodObj;
  CONST char *initString;
  ParseContext pc;
  CallFrame frame, *framePtr = &frame, *uplevelVarFramePtr;

#if 0
  fprintf(stderr, "NsfOConfigureMethod %s %2d ", ObjectName(object), objc);
  for(i=0; i<objc; i++) {fprintf(stderr, " [%d]=%s,", i, ObjStr(objv[i]));}
  fprintf(stderr, "\n");
#endif

  /* Get the object parameter definition */
  result = GetObjectParameterDefinition(interp, objv[0], object->cl, &parsedParam);
  if (result != TCL_OK || !parsedParam.paramDefs) {
    /*fprintf(stderr, "... nothing to do for method %s\n", ObjStr(objv[0]));*/
    return result;
  }

  /*
   * Get the initMethodObj/initString outside the loop iterating over the
   * arguments.
   */
  if (CallDirectly(interp, object, NSF_o_init_idx, &initMethodObj)) {
    initString = NULL;
  } else {
    initString = ObjStr(initMethodObj);
  }
  
  /* 
   * The effective call site of the configure() method (e.g., a proc or a
   * method) can result from upleveling the object creation procedure;
   * therefore, the *effective* call site can deviate from the *declaring*
   * call site (e.g. as in XOTcl2's unknown method). In such a scenario, the
   * configure() dispatch finds itself in a particular callstack
   * configuration: The top-most frame reflects the declaring call site
   * (interp->framePtr), while the effective call site (interp->varFramePtr)
   * is identified by a lower callstack level.
   *
   * Since configure pushes an object frame (for accessing the instance
   * variables) and sometimes a CMETHOD frame (for method invocations) we
   * record a) whether there was a preceding uplevel (identifiable through
   * deviating interp->framePtr and interp->varFramePtr) and, in case, b) the
   * ruling variable frame context. The preserved callframe reference can
   * later be used to restore the uplevel'ed call frame context.
  */

  uplevelVarFramePtr = 
    (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp) != Tcl_Interp_framePtr(interp) 
    ? Tcl_Interp_varFramePtr(interp) 
    : NULL; 
    
  /* 
   * Push frame to allow for [self] and make instvars of obj accessible as
   * locals
   */
  Nsf_PushFrameObj(interp, object, framePtr);

  /* Process the actual arguments based on the parameter definitions */
  paramDefs = parsedParam.paramDefs;
  ParamDefsRefCountIncr(INTERP paramDefs);
  result = ProcessMethodArguments(&pc, interp, object, 0, paramDefs,
				  NsfGlobalObjs[NSF_CONFIGURE], objc, objv);

  if (result != TCL_OK) {
    Nsf_PopFrameObj(interp, framePtr);
    goto configure_exit;
  }

  /*
   * At this point, the arguments are tested to be valid (according to the
   * parameter definitions) and the defaults are set. Now we have to apply the
   * arguments (mostly setting instance variables).
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
    /*fprintf(stderr, "[%d] param %s, object init called %d is default %d value = '%s' nrArgs %d\n",
	    i, paramPtr->name, (object->flags & NSF_INIT_CALLED),
	    (pc.flags[i-1] & NSF_PC_IS_DEFAULT),
	    ObjStr(pc.full_objv[i]), paramPtr->nrArgs);*/

    if ((object->flags & NSF_INIT_CALLED) && (pc.flags[i-1] & NSF_PC_IS_DEFAULT)) {
      Tcl_Obj *varObj;

      /*
       * Object parameter method calls (when the flag
       * NSF_ARG_METHOD_INVOCATION is set) do not set instance variables, so
       * we do not have to check for existing variables.
       */
      if (paramPtr->flags & NSF_ARG_METHOD_INVOCATION) {
	continue;
      }

      varObj = Tcl_ObjGetVar2(interp, paramPtr->nameObj, NULL, TCL_PARSE_PART1);
      if (varObj) {
	/*
	 * The value exists already, ignore this parameter.
	 */
	/*fprintf(stderr, "a variable for %s exists already, ignore param flags %.6x valueObj %p\n",
	  paramPtr->name, paramPtr->flags, pc.full_objv[i]);*/
	continue;
      }
    }

    newValue = pc.full_objv[i];
    /*fprintf(stderr, "     new Value of %s = [%d] %p '%s', type %s addr %p\n",
            ObjStr(paramPtr->nameObj), i,
            newValue, newValue ? ObjStr(newValue) : "(null)", paramPtr->type,
	    &(pc.full_objv[i]));*/

    /*
     * Special setter methods for invoking methods calls; handles types
     * "initcmd", "alias" and "forward".
     */
    if (paramPtr->flags & NSF_ARG_METHOD_INVOCATION) {
      CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
      NsfCallStackContent csc, *cscPtr = &csc;
      CallFrame frame2, *framePtr2 = &frame2;
      int consuming = (*paramPtr->name == '-' || paramPtr->nrArgs > 0);

      if (consuming && newValue == NsfGlobalObjs[NSF___UNKNOWN__]) {
	/*
	 * In the case we have a consuming parameter, but we have no value
	 * provided and not default, there is no reason to call the invocation
	 * parameter.
	 */
	/*fprintf(stderr, "%s consuming nrargs %d no value\n", paramPtr->name, paramPtr->nrArgs);*/
	continue;
      }

      /*
       * The current callframe of configure uses an objframe, such
       * that setvar etc.  are able to access variables like "a" as a
       * local variable.  However, in the init block, we do not like
       * that behavior, since this should look like like a proc body.
       * So we push yet another callframe without providing the
       * varframe.
       *
       * The new frame will have the namespace of the caller to avoid
       * the current objframe. Nsf_PushFrameCsc() will establish a
       * CMETHOD frame.
       */

      Tcl_Interp_varFramePtr(interp) = varFramePtr->callerVarPtr;
      cscPtr->flags = 0;
      CscInit(INTERP cscPtr, object, object->cl /*cl*/, NULL /*cmd*/,
	      NSF_CSC_TYPE_PLAIN, 0, NsfGlobalStrings[NSF_CONFIGURE]);
      Nsf_PushFrameCsc(interp, cscPtr, framePtr2);

      if (paramPtr->flags & NSF_ARG_INITCMD) {
	/* cscPtr->cmdPtr = NSFindCommand(interp, "::eval"); */
        result = Tcl_EvalObjEx(interp, newValue, TCL_EVAL_DIRECT);

      } else if (paramPtr->flags & NSF_ARG_ALIAS) {
        Tcl_Obj *methodObj, **ovPtr, *ov0;
	CONST char *methodString;
        int oc = 0;

	/*
	 * Restore the variable frame context as found at the original call
	 * site of configure(). Note that we do not have to revert this
	 * context change when leaving this configure() context because a
	 * surrounding [uplevel] will correct the callstack context for us ...
	 */
	if (uplevelVarFramePtr) {
	  Tcl_Interp_varFramePtr(interp) = uplevelVarFramePtr;
	}

	/*
	 * Mark the intermittent CSC frame as INACTIVE, so that, e.g.,
	 * callstack traversals seeking active frames ignore it.
	 */
	cscPtr->frameType = NSF_CSC_TYPE_INACTIVE;
	
	/*
	 * If "method=" was given, use it as method name
	 */
	methodObj = paramPtr->method ? paramPtr->method : paramPtr->nameObj;
	methodString = ObjStr(methodObj);

	/*fprintf(stderr, "ALIAS %s, nrargs %d converter %p toNothing %d i %d oc %d,  pcPtr->lastObjc %d\n",
		paramPtr->name, paramPtr->nrArgs, paramPtr->converter,
		paramPtr->converter == ConvertToNothing,
		i, objc,  pc.lastObjc);*/

	if (paramPtr->converter == ConvertToNothing) {
	  /*
	   * We are using the varargs interface; pass all remaining args into
	   * the called method.
	   */
	  if (newValue == paramPtr->defaultValue) {
	    /*
	     * use the default
	     */
	    if (Tcl_ListObjGetElements(interp, paramPtr->defaultValue, &oc, &ovPtr) != TCL_OK) {
	      goto method_arg_done;
	    }
	    ov0 = *ovPtr;
	    ovPtr ++;
	  } else {
	    /*
	     * use actual args
	     */
	    ov0 = objv[pc.lastObjc];
	    ovPtr = (Tcl_Obj **)&objv[pc.lastObjc + 1];
	    oc = objc - pc.lastObjc;
	  }
	  varArgsProcessed = 1;
	} else {
	  /*
	   * A simple alias, receives no (when noarg was specified) or a
	   * single argument (default).
	   */
	  if (paramPtr->nrArgs == 1) {
	    oc = 1;
	    ov0 = newValue;
	  } else {
	    oc = 0;
	    ov0 = NULL;
	  }
	  ovPtr = NULL;
	}

	/*
	 * Check, if we have an object parameter alias for the constructor.
	 * Since we require the object system for the current object to
	 * determine its object system configuration, we can't do this at
	 * parameter compile time.
	 */
	if (initString && *initString == *methodString && strcmp(initString, methodString) == 0) {
	  result = DispatchInitMethod(interp, object, oc, &ov0, 0);
	} else {
	
	  /*fprintf(stderr, "call alias %s with methodObj %s.%s oc %d, nrArgs %d '%s'\n",
	    paramPtr->name, ObjectName(object), ObjStr(methodObj), oc,
	    paramPtr->nrArgs, ObjStr(newValue));*/
	  
	  result = NsfCallMethodWithArgs(interp, (Nsf_Object*)object, methodObj,
					 ov0, oc, ovPtr, NSF_CSC_IMMEDIATE);
	}
      } else /* must be NSF_ARG_FORWARD */ {
	Tcl_Obj *forwardSpec = paramPtr->method ? paramPtr->method : NULL; /* different default? */
	Tcl_Obj **nobjv, *ov[3];
	int nobjc;
	
	assert(paramPtr->flags & NSF_ARG_FORWARD);

	/*
	 * The current implementation performs for every object
	 * parameter forward the full cycle of
	 *
	 *  (a) splitting the spec,
	 *  (b) convert it to a the client data structure,
	 *  (c) invoke forward,
	 *  (d) free client data structure
	 *
	 * In the future, it should convert to the client data
	 * structure just once and free it with the disposal of the
	 * parameter. This could be achieved
	 */
	if (forwardSpec == NULL) {
	  result = NsfPrintError(interp, "no forward spec available\n");
	  goto method_arg_done;
	}
	result = Tcl_ListObjGetElements(interp, forwardSpec, &nobjc, &nobjv);
        if (result != TCL_OK) {
	  goto method_arg_done;
	} else {
	  Tcl_Obj *methodObj = paramPtr->nameObj;
	  ForwardCmdClientData *tcd = NULL;
	  int oc = 1;

	  result = ForwardProcessOptions(interp, methodObj,
					 NULL /*withDefault*/, 0 /*withEarlybinding*/,
					 NULL /*withMethodprefix*/, 0 /*withObjframe*/,
					 NULL /*withOnerror*/, 0 /*withVerbose*/,
					 nobjv[0], nobjc-1, nobjv+1, &tcd);
	  if (result != TCL_OK) {
	    if (tcd) ForwardCmdDeleteProc(tcd);
	    goto method_arg_done;
	  }

	  /*fprintf(stderr, "parameter %s forward spec <%s> After Options obj %s method %s\n",
		  ObjStr(paramPtr->nameObj), ObjStr(forwardSpec),
		  ObjectName(object), ObjStr(methodObj));*/

	  tcd->object = object;
	  ov[0] = methodObj;
	  if (paramPtr->nrArgs == 1) {
	    ov[oc] = newValue;
	    oc ++;
	  }

	  /*
	   * Mark the intermittent CSC frame as INACTIVE, so that, e.g.,
	   * callstack traversals seeking active frames ignore it.
	   */	
	  cscPtr->frameType = NSF_CSC_TYPE_INACTIVE;

	  result = NsfForwardMethod(tcd, interp, oc, ov);
	  ForwardCmdDeleteProc(tcd);
	}
      }
    method_arg_done:
      /*
       * Pop previously stacked frame for eval context and set the
       * varFramePtr to the previous value.
       */
      Nsf_PopFrameCsc(interp, framePtr2);
      CscListRemove(interp, cscPtr, NULL);
      CscFinish(interp, cscPtr, result, "converter object frame");
      Tcl_Interp_varFramePtr(interp) = varFramePtr;

      /* fprintf(stderr, "NsfOConfigureMethod_ attribute %s evaluated %s => (%d)\n",
	 ObjStr(paramPtr->nameObj), ObjStr(newValue), result);*/

      if (result != TCL_OK) {
        Nsf_PopFrameObj(interp, framePtr);
        goto configure_exit;
      }

      if (paramPtr->flags & NSF_ARG_INITCMD && RUNTIME_STATE(interp)->doKeepinitcmd) {
	Tcl_ObjSetVar2(interp, paramPtr->nameObj, NULL, newValue, TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
      }

      /* done with parameter method handling */
      continue;
    }

    if (newValue == NsfGlobalObjs[NSF___UNKNOWN__]) {
      /*
       * Nothing to do, we have a value setter, but no value is specified and
       * no default was provided.
       */
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
      /*
       * Actually set instance variable with the provided or default value.
       */
      Tcl_ObjSetVar2(interp, paramPtr->nameObj, NULL, newValue, TCL_LEAVE_ERR_MSG|TCL_PARSE_PART1);
    }
  }

  Nsf_PopFrameObj(interp, framePtr);

  /*
   * Check, if varargs were processed. 
   */
  remainingArgsc = pc.objc - paramDefs->nrParams;
  if (pc.varArgs && remainingArgsc > 0) {

    assert(varArgsProcessed);

  }

 configure_exit:

  ParamDefsRefCountDecr(INTERP paramDefs);
  ParseContextRelease(INTERP &pc);
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
          object, ((Command *)object->id)->flags == 0 ? ObjectName(object) : "(deleted)",
          object->flags, object->activationCount, object->id, ((Command *)object->id)->flags);*/

  /*
   * NSF_DESTROY_CALLED might be set already be DispatchDestroyMethod(),
   * the implicit destroy calls. It is necessary to set it here for
   * the explicit destroy calls in the script, which reach the
   * Object->destroy.
   */

  if ((object->flags & NSF_DESTROY_CALLED) == 0) {
    object->flags |= NSF_DESTROY_CALLED;
    /*fprintf(stderr, "NsfODestroyMethod %p sets DESTROY_CALLED %.6x\n", object, object->flags);*/
  }
  object->flags |= NSF_DESTROY_CALLED_SUCCESS;

  if ((object->flags & NSF_DURING_DELETE) == 0) {
    int result;
    Tcl_Obj *methodObj;

    /*fprintf(stderr, "   call dealloc on %p %s\n", object,
      ((Command *)object->id)->flags == 0 ? ObjectName(object) : "(deleted)");*/

    if (CallDirectly(interp, &object->cl->object, NSF_c_dealloc_idx, &methodObj)) {
      result = DoDealloc(interp, object);
    } else {
      /*fprintf(stderr, "call dealloc\n");*/
      result = NsfCallMethodWithArgs(interp, (Nsf_Object *)object->cl, methodObj,
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
  Tcl_SetIntObj(Tcl_GetObjResult(interp), 
		VarExists(interp, object, var, NULL, 
			  NSF_VAR_TRIGGER_TRACE|NSF_VAR_REQUIRE_DEFINED));
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
      if (h->clientData) {
        GuardDel(INTERP (NsfCmdList *) h);
      }
      GuardAdd(INTERP h, guardObj);
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
  callFrameContext ctx = {0, NULL, NULL};
  int result;

  if (object && (object->filterStack || object->mixinStack) ) {
    CallStackUseActiveFrame(interp, &ctx);
  }

  if (!Tcl_Interp_varFramePtr(interp)) {
    CallStackRestoreSavedFrames(interp, &ctx);
    return NsfPrintError(interp, "instvar used on %s, but callstack is not in procedure scope",
			 ObjectName(object));
  }

  result = NsfVarImport(interp, object, ObjStr(objv[0]), objc-1, objv+1);
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
	    GuardDel(INTERP (NsfCmdList *) h);
	  }
	  GuardAdd(INTERP h, guardObj);
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
NsfONoinitMethod(Tcl_Interp *UNUSED(interp), NsfObject *object) {
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
  int i, start = 1, argc, nextArgc, normalArgs, result = TCL_OK, isdasharg = NO_DASH;
  CONST char *methodName, *nextMethodName, *initString = NULL;
  Tcl_Obj **argv, **nextArgv;

#if 0
  fprintf(stderr, "NsfOResidualargsMethod %s %2d ", ObjectName(object), objc);
  for(i=0; i<objc; i++) {fprintf(stderr, " [%d]=%s,", i, ObjStr(objv[i]));}
  fprintf(stderr, "\n");
#endif

  /* skip arguments without leading dash */
  for (i=start; i < objc; i++) {
    if ((isdasharg = IsDashArg(interp, objv[i], 1, &methodName, &argc, &argv))) {
      break;
    }
  }
  normalArgs = i-1;

  /*
   * Get the init string; do it once, outside the loop.
   */
  if (i<objc) {
    NsfObjectSystem *osPtr = GetObjectSystem(object);
    Tcl_Obj *initObj = osPtr->methods[NSF_o_init_idx];
    if (initObj) {
      initString = ObjStr(initObj);
    }
  }

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
        result = CallConfigureMethod(interp, object, initString, methodName, argc+1, objv+i+1);
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
	result = CallConfigureMethod(interp, object, initString, methodName, argc+1, argv+1);
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

  /*
   * Call init with residual args in case it was not called yet
   */
  result = DispatchInitMethod(interp, object, normalArgs, objv+1, 0);

  /*
   * Return the non-processed leading arguments (XOTcl convention)
   */
  Tcl_SetObjResult(interp, Tcl_NewListObj(normalArgs, objv+1));

  return result;
}

/*
objectMethod uplevel NsfOUplevelMethod {
  {-argName "args" -type allargs}
}
*/
static int
NsfOUplevelMethod(Tcl_Interp *interp, NsfObject *UNUSED(object), int objc, Tcl_Obj *CONST objv[]) {
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
     * the object when it decrements its refCount after eval'ing it.
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
  callFrameContext ctx = {0, NULL, NULL};

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
    if (result != TCL_OK) {
      break;
    }
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
  callFrameContext ctx = {0, NULL, NULL};

  if (RUNTIME_STATE(interp)->exitHandlerDestroyRound != NSF_EXITHANDLER_OFF) {
    fprintf(stderr, "### Can't make objects volatile during shutdown\n");
    return NsfPrintError(interp, "Can't make objects volatile during shutdown");
  }

  CallStackUseActiveFrame(interp, &ctx);

  vn = NSTail(fullName);

  if (Tcl_SetVar2(interp, vn, NULL, fullName, 0)) {
    NsfObjectOpt *opt = NsfRequireObjectOpt(INTERP object);

    /*fprintf(stderr, "### setting trace for %s on frame %p\n", fullName,
      Tcl_Interp_varFramePtr(interp));
      NsfShowStack(interp);*/
    result = Tcl_TraceVar(interp, vn, TCL_TRACE_UNSETS,
			  (Tcl_VarTraceProc *)NsfUnsetTrace,
                          objPtr);
    opt->volatileVarName = vn;
  }
  CallStackRestoreSavedFrames(interp, &ctx);

  if (result == TCL_OK) {
    INCR_REF_COUNT(objPtr);
  }
  return result;
}

/***********************************************************************
 * End Object Methods
 ***********************************************************************/


/***********************************************************************
 * Begin Class Methods
 ***********************************************************************/

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

  if (NSF_DTRACE_OBJECT_ALLOC_ENABLED()) {
    NSF_DTRACE_OBJECT_ALLOC(ObjectName(newObj), ClassName(cl));
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
    if (strchr(nameString, ':')) {
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
    if (strchr(nameString, ':')) {
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
          newObject ? ClassName(newObject->cl) : "NULL",
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
      result = CallMethod(cl, interp, methodObj,
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
      result = CallMethod(cl, interp, methodObj,
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

    /* in case, the object is destroyed during initialization, we incr refCount */
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
NsfCDeallocMethod(Tcl_Interp *interp, NsfClass *UNUSED(cl), Tcl_Obj *obj) {
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
      if (h->clientData) {
	GuardDel(INTERP h);
      }
      GuardAdd(INTERP h, guardObj);
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
	    GuardDel(INTERP (NsfCmdList *) h);
	  }
	  GuardAdd(INTERP h, guardObj);
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
    /*
     * In case the symbol existed already, reset prefix to the
     * original length
     */
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
    if (objc >= 1) {
      memcpy(ov+3, objv, sizeof(Tcl_Obj *)*objc);
    }
    if (callDirectly) {
      result = NsfCCreateMethod(interp, cl, ObjStr(fullnameObj), objc+2, ov+1);
    } else {
      result = ObjectDispatch(cl, interp, objc+3, ov, NSF_CSC_IMMEDIATE);
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
      /*fprintf(stderr, "RECREATE calls cleanup directly for object %s\n", ObjectName(object));*/
      result = NsfOCleanupMethod(interp, object);
    } else {
      /*NsfObjectSystem *osPtr = GetObjectSystem(object);
      fprintf(stderr, "RECREATE calls method cleanup for object %p %s OS %s\n",
              object, ObjectName(object), ObjectName((&osPtr->rootClass->object)));*/
      result = CallMethod(object, interp, methodObj,
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

  if (GetObjectFromObj(interp, nameObj, &object) != TCL_OK) {
    return NsfPrintError(interp, "can't recreate non existing object %s", ObjStr(nameObj));
  }
  return RecreateObject(interp, cl, object, objc, objv);
}

/*
classMethod superclass NsfCSuperclassMethod {
  {-argName "superclasses" -required 0 -type tclobj}
}
*/
static int
NsfCSuperclassMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *superclassesObj) {
  return NsfRelationCmd(interp, &cl->object, RelationtypeSuperclassIdx, superclassesObj);
}

/***********************************************************************
 * End Class Methods
 ***********************************************************************/

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
  case MethodtypeNsfprocIdx:
    methodType = NSF_METHODTYPE_NSFPROC;
    break;
  default:
    methodType = 0;
  }

  return methodType;
}

/***********************************************************************
 * Begin Object Info Methods
 ***********************************************************************/

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
    if (!(object->flags & NSF_FILTER_ORDER_VALID)) {
      FilterComputeDefined(interp, object);
    }
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
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), IsSubType(INTERP object->cl, typeClass));
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

  if (!(object->flags & NSF_FILTER_ORDER_VALID)) {
    FilterComputeDefined(interp, object);
  }
  if (!(object->flags & NSF_FILTER_ORDER_DEFINED)) {
    return TCL_OK;
  }
  for (cmdList = object->filterOrder; cmdList;  cmdList = cmdList->nextPtr) {
    filterName = Tcl_GetCommandName(interp, cmdList->cmdPtr);
    if (filterName[0] == filter[0] && !strcmp(filterName, filter)) {
      break;
    }
  }

  if (!cmdList) {
    return TCL_OK;
  }
  fcl = cmdList->clorobj;
  Tcl_SetObjResult(interp, MethodHandleObj((NsfObject *)fcl, !NsfObjectIsClass(&fcl->object), filterName));
  return TCL_OK;
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
    if (MethodSourceMatches(withSource, NULL, object)) {
      ListMethodKeys(interp, cmdTablePtr, NULL, pattern, methodType,
		     withCallprotection, withPath,
		     dups, object, withPer_object);
    }
  }

  if (!withNomixins) {
    if (!(object->flags & NSF_MIXIN_ORDER_VALID)) {
      MixinComputeDefined(interp, object);
    }
    if (object->flags & NSF_MIXIN_ORDER_DEFINED_AND_VALID) {
      NsfCmdList *ml;
      NsfClass *mixin;
      for (ml = object->mixinOrder; ml; ml = ml->nextPtr) {
	int guardOk = TCL_OK;
        mixin = NsfGetClassFromCmdPtr(ml->cmdPtr);
	assert(mixin);
        if (withIncontext) {
          if (!RUNTIME_STATE(interp)->guardCount) {
            guardOk = GuardCall(object, interp, ml->clientData, NULL);
          }
        }
        if (mixin && guardOk == TCL_OK) {
          Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(mixin->nsPtr);
	  if (!MethodSourceMatches(withSource, mixin, NULL)) continue;
          ListMethodKeys(interp, cmdTablePtr, NULL, pattern, methodType,
			 withCallprotection, withPath,
                         dups, object, withPer_object);
        }
      }
    }
  }

  /* append method keys from inheritance order */
  for (pl = ComputeOrder(INTERP object->cl, object->cl->order, Super); pl; pl = pl->nextPtr) {
    Tcl_HashTable *cmdTablePtr = Tcl_Namespace_cmdTablePtr(pl->cl->nsPtr);
    if (!MethodSourceMatches(withSource, pl->cl, NULL)) continue;
    ListMethodKeys(interp, cmdTablePtr, NULL, pattern, methodType,
		   withCallprotection, withPath,
                   dups, object, withPer_object);
  }
  Tcl_DeleteHashTable(dups);
  return TCL_OK;
}

/*
objectInfoMethod lookupslots NsfObjInfoLookupSlotsMethod {
  {-argName "-source" -nrargs 1 -type "all|application|baseclasses" -default all}
  {-argName "-type" -required 0 -nrargs 1 -type class}
  {-argName "pattern" -required 0}
}
*/
static int
NsfObjInfoLookupSlotsMethod(Tcl_Interp *interp, NsfObject *object,
			    int withSource, NsfClass *type,
			    CONST char *pattern) {
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
  NsfClasses *precendenceList, *clPtr;
  Tcl_HashTable slotTable;

  precendenceList = ComputePrecedenceList(interp, object, NULL /* pattern*/, 1, 1);
  if (withSource == 0) {withSource = 1;}

  Tcl_InitHashTable(&slotTable, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", &slotTable);

  /*
   * First add the per-object slot objects
   */
  if (MethodSourceMatches(withSource, NULL, object)) {
    AddSlotObjects(interp, object, "::per-object-slot", &slotTable,
		   withSource, type, pattern, listObj);
  }

  /*
   * Then add the class provided slot objects
   */
  for (clPtr = precendenceList; clPtr; clPtr = clPtr->nextPtr) {
    if (MethodSourceMatches(withSource, clPtr->cl, NULL)) {
      AddSlotObjects(interp, &clPtr->cl->object, "::slot", &slotTable,
		     withSource, type, pattern, listObj);
    }
  }

  Tcl_DeleteHashTable(&slotTable);
  MEM_COUNT_FREE("Tcl_InitHashTable", &slotTable);

  NsfClassListFree(INTERP precendenceList);
  Tcl_SetObjResult(interp, listObj);

  return TCL_OK;
}

/*
objectInfoMethod method NsfObjInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|exists|definition|handle|origin|parameter|parametersyntax|type|precondition|postcondition|subcommands"}
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
	  ObjStr(methodNameObj), methodName1, object, regObject, defObject, fromClassNS);*/
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
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-path"}
  {-argName "pattern"}
}
*/
static int
NsfObjInfoMethodsMethod(Tcl_Interp *interp, NsfObject *object,
			int withCallproctection,
			int withMethodtype,
			int withPath,
			CONST char *pattern) {
  return ListDefinedMethods(interp, object, pattern, 1 /* per-object */,
                            AggregatedMethodType(withMethodtype), withCallproctection,
                            withPath);
}

/*
objectInfoMethod mixinclasses NsfObjInfoMixinclassesMethod {
  {-argName "-guards"}
  {-argName "-heritage"}
  {-argName "pattern" -type objpattern}
}
}
*/
static int
NsfObjInfoMixinclassesMethod(Tcl_Interp *interp, NsfObject *object,
			     int withGuards, int withHeritage,
			     CONST char *patternString, NsfObject *patternObj) {

  if (withHeritage) {
    if (!(object->flags & NSF_MIXIN_ORDER_VALID)) {
      MixinComputeDefined(interp, object);
    }
    return MixinInfo(interp, object->mixinOrder, patternString, withGuards, patternObj);
  }
  return object->opt ?
    MixinInfo(interp, object->opt->mixins, patternString, withGuards, patternObj) :
    TCL_OK;
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
    Tcl_Namespace *nsPtr = Tcl_Command_nsPtr(object->id);
    Tcl_SetResult(interp, nsPtr ? nsPtr->fullName : "", TCL_VOLATILE);
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
  Tcl_Obj *resultObj = Tcl_NewObj();

  precedenceList = ComputePrecedenceList(interp, object, pattern, !withIntrinsicOnly, 1);
  for (pl = precedenceList; pl; pl = pl->nextPtr) {
    Tcl_ListObjAppendElement(interp, resultObj, pl->cl->object.cmdName);
  }
  NsfClassListFree(INTERP precedenceList);

  Tcl_SetObjResult(interp, resultObj);
  return TCL_OK;
}

/*
objectInfoMethod slots NsfObjInfoSlotsMethod {
  {-argName "-type" -required 0 -nrargs 1 -type class}
  {-argName "pattern" -required 0}
}
*/
static int
NsfObjInfoSlotsMethod(Tcl_Interp *interp, NsfObject *object,
		      NsfClass *type, CONST char *pattern) {
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);

  AddSlotObjects(interp, object, "::per-object-slot", NULL,
		 SourceAllIdx, type, pattern, listObj);

  Tcl_SetObjResult(interp, listObj);
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

  ListVarKeys(interp, TclVarHashTablePtr(varTablePtr), pattern);
  varlist = Tcl_GetObjResult(interp);

  Tcl_ListObjLength(interp, varlist, &length);
  okList = Tcl_NewListObj(0, NULL);
  for (i=0; i<length; i++) {
    Tcl_ListObjIndex(interp, varlist, i, &element);
    if (VarExists(interp, object, ObjStr(element), NULL, NSF_VAR_REQUIRE_DEFINED)) {
      Tcl_ListObjAppendElement(interp, okList, element);
    } else {
      /*fprintf(stderr, "must ignore '%s' %d\n", ObjStr(element), i);*/
      /*Tcl_ListObjReplace(interp, varlist, i, 1, 0, NULL);*/
    }
  }
  Tcl_SetObjResult(interp, okList);
  return TCL_OK;
}
/***********************************************************************
 * End Object Info Methods
 ***********************************************************************/

/***********************************************************************
 * Begin Class Info methods
 ***********************************************************************/

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
  NsfClasses *pl, *intrinsic,  *checkList = NULL, *mixinClasses = NULL;
  Tcl_Obj *resultObj;

  resultObj = Tcl_NewObj();
  intrinsic = ComputeOrder(INTERP cl, cl->order, Super);

  NsfClassListAddPerClassMixins(interp, cl, &mixinClasses, &checkList);
  for (pl = mixinClasses; pl; pl = pl->nextPtr) {
    if (NsfClassListFind(pl->nextPtr, pl->cl) == NULL &&
	NsfClassListFind(intrinsic, pl->cl) == NULL) {
      AppendMatchingElement(interp, resultObj, pl->cl->object.cmdName, pattern);
    }
  }

  if (intrinsic) {
    for (pl = intrinsic->nextPtr; pl; pl = pl->nextPtr) {
      AppendMatchingElement(interp, resultObj, pl->cl->object.cmdName, pattern);
    }
  }

  NsfClassListFree(INTERP mixinClasses);
  NsfClassListFree(INTERP checkList);

  Tcl_SetObjResult(interp, resultObj);
  return TCL_OK;
}

/*
 * get all instances of a class recursively into an initialized
 * String key hashtable
 */
static int
NsfClassInfoInstancesMethod1(Tcl_Interp *interp, NsfClass *startCl, Tcl_Obj *resultObj,
			     int withClosure, CONST char *pattern, NsfObject *matchObject) {
  Tcl_HashTable *tablePtr = &startCl->instances;
  NsfClasses *sc;
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;
  int rc = 0;

  /*fprintf(stderr, "NsfClassInfoInstancesMethod: clo %d pattern %s match %p\n",
    withClosure, pattern, matchObject);*/

  for (hPtr = Tcl_FirstHashEntry(tablePtr, &search); hPtr;
       hPtr = Tcl_NextHashEntry(&search)) {
    NsfObject *inst = (NsfObject *) Tcl_GetHashKey(tablePtr, hPtr);
    /*fprintf(stderr, "match '%s' %p %p '%s'\n",
      ObjectName(matchObject), matchObject, inst, ObjectName(inst));*/
    if (matchObject && inst == matchObject) {
      Tcl_SetStringObj(resultObj, ObjStr(matchObject->cmdName), -1);
      return 1;
    }
    AppendMatchingElement(interp, resultObj, inst->cmdName, pattern);
  }
  if (withClosure) {
    for (sc = startCl->sub; sc; sc = sc->nextPtr) {
      rc = NsfClassInfoInstancesMethod1(interp, sc->cl, resultObj, withClosure, pattern, matchObject);
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
  Tcl_Obj *resultObj = Tcl_NewObj();

  NsfClassInfoInstancesMethod1(interp, startCl, resultObj, withClosure, pattern, matchObject);
  Tcl_SetObjResult(interp, resultObj);

  return TCL_OK;
}

/*
classInfoMethod method NsfClassInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|exists|definition|handle|origin|parameter|parametersyntax|type|precondition|postcondition|subcommands"}
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
	  "NsfClassInfoMethodMethod object %p regObject %p defObject %p %s fromClass %d cmd %p method %s\n",
	  &class->object, regObject, defObject, ObjectName(defObject), fromClassNS, cmd, methodName1);*/
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
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-path"}
  {-argName "pattern"}
}
*/
static int
NsfClassInfoMethodsMethod(Tcl_Interp *interp, NsfClass *class,
			  int withCallproctection,
			  int withMethodtype,
			  int withPath,
			  CONST char *pattern) {
  return ListDefinedMethods(interp, &class->object, pattern, 0 /* per-object */,
                            AggregatedMethodType(withMethodtype), withCallproctection,
			    withPath);
}

/*
classInfoMethod mixinclasses NsfClassInfoMixinclassesMethod {
  {-argName "-closure"}
  {-argName "-guards"}
  {-argName "-heritage"}
  {-argName "pattern" -type objpattern}
}
*/
static int
NsfClassInfoMixinclassesMethod(Tcl_Interp *interp, NsfClass *class,
			       int withClosure, int withGuards, int withHeritage,
			       CONST char *patternString, NsfObject *patternObj) {
  NsfClassOpt *opt = class->opt;
  Tcl_Obj *resultObj;
  int rc;

  Tcl_ResetResult(interp);
  resultObj = Tcl_GetObjResult(interp);

  if (withHeritage) {
    NsfClasses *checkList = NULL, *mixinClasses = NULL, *clPtr;

    if (withGuards) {
      return NsfPrintError(interp, "-guards cannot be used together with -heritage\n");
    }

    NsfClassListAddPerClassMixins(interp, class, &mixinClasses, &checkList);
    for (clPtr = mixinClasses; clPtr; clPtr = clPtr->nextPtr) {
      if (NsfClassListFind(clPtr->nextPtr, clPtr->cl)) continue;
      AppendMatchingElement(interp, resultObj, clPtr->cl->object.cmdName, patternString);
    }

    NsfClassListFree(INTERP checkList);
    NsfClassListFree(INTERP mixinClasses);

  } else if (withClosure) {
    Tcl_HashTable objTable, *commandTable = &objTable;

    MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
    Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
    rc = GetAllClassMixins(interp, commandTable, resultObj,
			   class, withGuards,
			   patternString, patternObj);
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
  Tcl_Obj *resultObj;

  Tcl_ResetResult(interp);
  resultObj = Tcl_GetObjResult(interp);

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
      rc = AppendMatchingElementsFromCmdList(interp, opt->isClassMixinOf, resultObj,
					     patternString, patternObj);
      if (rc && patternObj) {goto finished;}
    }
    if (perObject) {
      rc = AppendMatchingElementsFromCmdList(interp, opt->isObjectMixinOf, resultObj,
					     patternString, patternObj);
    }
  } else if (withClosure) {
    Tcl_HashTable objTable, *commandTable = &objTable;
    MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);
    Tcl_InitHashTable(commandTable, TCL_ONE_WORD_KEYS);
    if (perClass) {
      rc = GetAllClassMixinsOf(interp, commandTable, resultObj,
			       class, 0, 1, patternString, patternObj);
      if (rc && patternObj) {goto finished;}
    }
    if (perObject) {
      rc = GetAllObjectMixinsOf(interp, commandTable, resultObj,
				class, 0, 1, patternString, patternObj);
    }
    Tcl_DeleteHashTable(commandTable);
    MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
  }

 finished:
  if (patternObj) {
    Tcl_SetObjResult(interp, rc ? patternObj->cmdName : NsfGlobalObjs[NSF_EMPTY]);
  } else {
    Tcl_SetObjResult(interp, resultObj);
  }
  return TCL_OK;
}


/*
classInfoMethod objectparameter NsfClassInfoObjectparameterMethod {
  {-argName "infoobjectparametersubcmd" -type "name|parameter|parametersyntax" -required 1}
  {-argName "name" -required 0}
}
*/

static int
NsfClassInfoObjectparameterMethod(Tcl_Interp *interp, NsfClass *class,
				  int subcmd, CONST char *name) {
  NsfParsedParam parsedParam;
  Tcl_Obj *listObj = NULL;
  Nsf_Param CONST *paramsPtr;
  Nsf_Param paramList[2];
  int result;

  result = GetObjectParameterDefinition(interp, NsfGlobalObjs[NSF_EMPTY],
					class, &parsedParam);

  if (result != TCL_OK || !parsedParam.paramDefs) {
    return result;
  }

  paramsPtr = parsedParam.paramDefs->paramsPtr;

  /*
   * If a single paramter name is given, we construct a filtered parameter
   * list on the fly and provide it to the output functions. Note, that the
   * first matching parameter is queried.
   */
  if (name) {
    Nsf_Param CONST *pPtr;

    for (pPtr = paramsPtr; pPtr->name; pPtr++) {
      if (Tcl_StringMatch( ObjStr(pPtr->nameObj), name)) {
	paramsPtr = (Nsf_Param CONST *)&paramList;
	paramList[0] = *pPtr;
	paramList[1].name = NULL;
	break;
      }
    }
    if (paramsPtr == parsedParam.paramDefs->paramsPtr) {
      /* 
       * The named parameter was NOT found 
       */
      Tcl_SetObjResult(interp, NsfGlobalObjs[NSF_EMPTY]);
      return TCL_OK;
    }
  }

  switch (subcmd) {
  case InfoobjectparametersubcmdListIdx:
    listObj = ParamDefsList(interp, paramsPtr);
    break;
  case InfoobjectparametersubcmdNameIdx:
    listObj = ParamDefsNames(interp, paramsPtr);
    break;
  case InfoobjectparametersubcmdParameterIdx:
    listObj = ParamDefsFormat(interp, paramsPtr);
    break;
  case InfoobjectparametersubcmdParametersyntaxIdx:
    listObj = NsfParamDefsSyntax(INTERP paramsPtr);
    break;
  }
  Tcl_SetObjResult(interp, listObj);
  DECR_REF_COUNT2("paramDefsObj", listObj);

  return TCL_OK;
}

/*
classInfoMethod slots NsfClassInfoSlotsMethod {
  {-argName "-closure"}
  {-argName "-source" -nrargs 1 -type "all|application|baseclasses"}
  {-argName "-type" -required 0 -nrargs 1 -type class}
  {-argName "pattern" -required 0}
}
*/
static int
NsfClassInfoSlotsMethod(Tcl_Interp *interp, NsfClass *class,
			int withClosure, int withSource, NsfClass *type,
			CONST char *pattern) {
  NsfClasses *clPtr, *intrinsic,  *precedenceList = NULL;
  Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);
  Tcl_HashTable slotTable;

  Tcl_ResetResult(interp);
  intrinsic = ComputeOrder(INTERP class, class->order, Super);

  if (withClosure) {
    NsfClasses *checkList = NULL, *mixinClasses = NULL;
    /*
     * Compute the closure: first the transitive mixin classes...
     */
    NsfClassListAddPerClassMixins(interp, class, &mixinClasses, &checkList);
    for (clPtr = mixinClasses; clPtr; clPtr = clPtr->nextPtr) {
      if (NsfClassListFind(clPtr->nextPtr, clPtr->cl) == NULL &&
	  NsfClassListFind(intrinsic, clPtr->cl) == NULL) {
	NsfClassListAdd(INTERP &precedenceList, clPtr->cl, NULL);
      }
    }
    /*
     * ... followed by the intrinsic classes
     */
    NsfClassListAdd(INTERP &precedenceList, class, NULL);
    for (clPtr = intrinsic->nextPtr; clPtr; clPtr = clPtr->nextPtr) {
      NsfClassListAdd(INTERP &precedenceList, clPtr->cl, NULL);
    }
    NsfClassListFree(INTERP checkList);
    NsfClassListFree(INTERP mixinClasses);

  } else {
    NsfClassListAdd(INTERP &precedenceList, class, NULL);
  }
  /* NsfClassListPrint("precedence", precedenceList);*/
  if (withSource == 0) {withSource = 1;}

  /*
   * Use a hash table to eliminate potential duplicates.
   */
  Tcl_InitHashTable(&slotTable, TCL_STRING_KEYS);
  MEM_COUNT_ALLOC("Tcl_InitHashTable", &slotTable);

  for (clPtr = precedenceList; clPtr; clPtr = clPtr->nextPtr) {
    if (MethodSourceMatches(withSource, clPtr->cl, NULL)) {
      AddSlotObjects(interp, &clPtr->cl->object, "::slot", &slotTable,
		     withSource, type, pattern, listObj);
    }
  }

  Tcl_DeleteHashTable(&slotTable);
  MEM_COUNT_FREE("Tcl_InitHashTable", &slotTable);

  NsfClassListFree(INTERP precedenceList);
  Tcl_SetObjResult(interp, listObj);

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
    subclasses = ComputeOrder(INTERP class, class->order, Sub);
    class->order = saved;
    rc = AppendMatchingElementsFromClasses(interp, subclasses ? subclasses->nextPtr : NULL,
					   patternString, patternObj);
    NsfClassListFree(INTERP subclasses);
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

/***********************************************************************
 * End Class Info methods
 ***********************************************************************/

/*
 * Initialization and Exit handlers
 */

#ifdef DO_FULL_CLEANUP
/* delete global variables and procs */
static void
DeleteProcsAndVars(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  Tcl_HashTable *varTablePtr, *cmdTablePtr, *childTablePtr;
  Tcl_HashSearch search;
  Tcl_Command cmd;
  Var *varPtr;
  register Tcl_HashEntry *entryPtr;

  assert(nsPtr);

  //fprintf(stderr, "DeleteProcsAndVars in %s\n", nsPtr->fullName);

  varTablePtr = (Tcl_HashTable *)Tcl_Namespace_varTablePtr(nsPtr);
  cmdTablePtr = Tcl_Namespace_cmdTablePtr(nsPtr);
  childTablePtr = Tcl_Namespace_childTablePtr(nsPtr);

  /* 
   * Deleting the procs and vars in the child namespaces does not seem to be
   * necessary, but we do it anyway
   */
  for (entryPtr = Tcl_FirstHashEntry(childTablePtr, &search); entryPtr;
       entryPtr = Tcl_NextHashEntry(&search)) {
    Tcl_Namespace *childNsPtr = (Tcl_Namespace *) Tcl_GetHashValue(entryPtr);
    DeleteProcsAndVars(interp, childNsPtr);
  }

  for (entryPtr = Tcl_FirstHashEntry(varTablePtr, &search); entryPtr;
       entryPtr = Tcl_NextHashEntry(&search)) {
    Tcl_Obj *nameObj;
    GetVarAndNameFromHash(entryPtr, &varPtr, &nameObj);
    if (!TclIsVarUndefined(varPtr) || TclIsVarNamespaceVar(varPtr)) {
      /* fprintf(stderr, "unsetting var %s\n", ObjStr(nameObj));*/
      Tcl_UnsetVar2(interp, ObjStr(nameObj), (char *)NULL, TCL_GLOBAL_ONLY);
    }
  }
  for (entryPtr = Tcl_FirstHashEntry(cmdTablePtr, &search); entryPtr;
       entryPtr = Tcl_NextHashEntry(&search)) {
    cmd = (Tcl_Command)Tcl_GetHashValue(entryPtr);

    if (Tcl_Command_objProc(cmd) == RUNTIME_STATE(interp)->objInterpProc) {
      /*fprintf(stderr, "cmdname = %s cmd %p proc %p objProc %p %d\n",
        Tcl_GetHashKey(cmdTablePtr, entryPtr), cmd, Tcl_Command_proc(cmd), Tcl_Command_objProc(cmd),
        Tcl_Command_proc(cmd)==RUNTIME_STATE(interp)->objInterpProc);*/
	
      Tcl_DeleteCommandFromToken(interp, cmd);
    }
  }
}
#endif


#ifdef DO_CLEANUP
/*
 *----------------------------------------------------------------------
 *
 * DeleteNsfProcs --
 *
 *      Delete all nsfprocs in the namespaces rooted by the second
 *      argument. If it is NULL, the globale namespace is used a root of the
 *      namespace tree. The function is necessary to trigger the freeing of
 *      the parameter defs.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Deletion of nsfprocs.
 *
 *----------------------------------------------------------------------
 */
static void
DeleteNsfProcs(Tcl_Interp *interp, Tcl_Namespace *nsPtr) {
  Tcl_HashTable *cmdTablePtr, *childTablePtr;
  register Tcl_HashEntry *entryPtr;
  Tcl_HashSearch search;
  Tcl_Command cmd;
  
  if (nsPtr == NULL) {
    nsPtr = Tcl_GetGlobalNamespace(interp);
  }

  assert(nsPtr);
  /*fprintf(stderr, "### DeleteNsfProcs current namespace '%s'\n", 
    nsPtr ? nsPtr->fullName : "NULL");*/

  cmdTablePtr = Tcl_Namespace_cmdTablePtr(nsPtr);
  childTablePtr = Tcl_Namespace_childTablePtr(nsPtr);

  for (entryPtr = Tcl_FirstHashEntry(cmdTablePtr, &search); entryPtr;
       entryPtr = Tcl_NextHashEntry(&search)) {
    cmd = (Tcl_Command)Tcl_GetHashValue(entryPtr);
    if (Tcl_Command_objProc(cmd) == NsfProcStub) {
      /*fprintf(stderr, "cmdname = %s cmd %p\n",
	Tcl_GetHashKey(cmdTablePtr, entryPtr), cmd);*/
      Tcl_DeleteCommandFromToken(interp, cmd);
    }
  }
  for (entryPtr = Tcl_FirstHashEntry(childTablePtr, &search); entryPtr;
       entryPtr = Tcl_NextHashEntry(&search)) {
    Tcl_Namespace *childNsPtr = (Tcl_Namespace *) Tcl_GetHashValue(entryPtr);
    DeleteNsfProcs(interp, childNsPtr);
  }   
}

/*
 *----------------------------------------------------------------------
 *
 * ClassHasSubclasses --
 *
 *      Check, whether the given class has subclasses.
 *
 * Results:
 *      boolean
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static int
ClassHasSubclasses(NsfClass *cl) {
  return (cl->sub != NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * ClassHasInstances --
 *
 *      Check, whether the given class has instances.
 *
 * Results:
 *      boolean
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static int
ClassHasInstances(NsfClass *cl) {
  Tcl_HashSearch hSrch;
  return (Tcl_FirstHashEntry(&cl->instances, &hSrch) != NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * ObjectHasChildren --
 *
 *      Check, whether the given object has children
 *
 * Results:
 *      boolean
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static int
ObjectHasChildren(NsfObject *object) {
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
   * refCount might not be decremented correctly. If we are in the
   * physical destroy round, we can set the counter to an appropriate
   * value to ensure deletion.
   */
  if (object->refCount != 1) {
    if (object->refCount > 1) {
      NsfLog(interp, NSF_LOG_WARN,  "Have to fix refCount for obj %p refCount %d  (name %s)",
	     object, object->refCount, ObjectName(object));
    } else {
      NsfLog(interp, NSF_LOG_WARN,  "Have to fix refCount for obj %p refCount %d",
	     object, object->refCount);
    }
    object->refCount = 1;
  }

#if !defined(NDEBUG)
  if (object->activationCount != 0) {
    fprintf(stderr, "FinalObjectDeletion obj %p activationcount %d\n", object, object->activationCount);
  }
#endif
  assert(object->activationCount == 0);

  if (object->id) {
    /*fprintf(stderr, "  ... cmd dealloc %p final delete refCount %d\n",
      object->id, Tcl_Command_refCount(object->id));*/

    if (NSF_DTRACE_OBJECT_FREE_ENABLED()) {
      NSF_DTRACE_OBJECT_FREE(ObjectName(object), ClassName(object->cl));
    }

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
  for (hPtr = Tcl_FirstHashEntry(commandNameTablePtr, &hSrch); hPtr;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
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
	  /*fprintf(stderr, "Class %p %s deletes cmd %p %s\n", 
	    object, ObjectName(object), cmd, Tcl_GetCommandName(interp, cmd));*/
	  /*Tcl_DeleteCommandFromToken(interp, cmd);
	    deleted ++;	  */
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
      if (object && !NsfObjectIsClass(object) && !ObjectHasChildren(object)) {
	/*if (object->id) {
	  fprintf(stderr, "  ... delete object %s %p, class=%s id %p ns %p\n", key, object,
		  ClassName(object->cl), object->id, object->nsPtr);
		  }*/
        FreeUnsetTraceVariable(interp, object);
        if (object->id) FinalObjectDeletion(interp, object);
        Tcl_DeleteHashEntry(hPtr);
        deleted++;
      }
    }
    /*fprintf(stderr, "deleted %d Objects without dependencies\n", deleted);*/
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
          && !ObjectHasChildren((NsfObject *)cl)
          && !ClassHasInstances(cl)
          && !ClassHasSubclasses(cl)
          && !IsBaseClass(cl)
          ) {
        /*fprintf(stderr, "  ... delete class %s %p\n", key, cl); */

        FreeUnsetTraceVariable(interp, &cl->object);
        if (cl->object.id) {
	  FinalObjectDeletion(interp, &cl->object);
	}
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

  /*
   * Free all data in the pointer converter
   */
  Nsf_PointerExit(interp);

#if defined(NSF_PROFILE)
  NsfProfileFree(interp);
#endif

  FREE(Tcl_Obj**, NsfGlobalObjs);

#if defined(TCL_MEM_DEBUG)
  TclDumpMemoryInfo((ClientData) stderr, 0);
  Tcl_DumpActiveMemory("./nsfActiveMem");
  /* Tcl_GlobalEval(interp, "puts {checkmem to checkmemFile};
     checkmem checkmemFile"); */
#endif
  MEM_COUNT_RELEASE(interp);
  ckfree((char *) RUNTIME_STATE(interp));

  Tcl_Interp_flags(interp) = flags;
  Tcl_Release(interp);
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
  /*NsfCompEnv *interpstructions = NsfGetCompEnv();*/
#endif
  static NsfMutex initMutex = 0;

#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8.5", 0) == NULL) {
    return TCL_ERROR;
  }
  if (Tcl_TomMath_InitStubs(interp, "8.5") == NULL) {
    return TCL_ERROR;
  }
#endif

#if defined(TCL_MEM_DEBUG)
  TclDumpMemoryInfo((ClientData) stderr, 0);
#endif

  /*
   * Runtime State stored in the client data of the Interp's global namespace
   * in order to avoid global state information. All fields are per default
   * set to zero.
   */
  runtimeState = ckalloc(sizeof(NsfRuntimeState));
  memset(runtimeState, 0, sizeof(NsfRuntimeState));

#if USE_ASSOC_DATA
  Tcl_SetAssocData(interp, "NsfRuntimeState", NULL, runtimeState);
#else
  Tcl_Interp_globalNsPtr(interp)->clientData = runtimeState;
#endif

  /*
   * If MEM_COUNT is activated, the tables have to be initialized before the
   * first call to the MEM_COUNT macros (including e.g. INCR_REF_COUNT), but
   * it requires that the runtimeState is already associated with the interp.
   */
  MEM_COUNT_INIT(interp);

  /*
   * Init global variables for Tcl_Obj types
   */
  NsfMutexLock(&initMutex);
  Nsf_OT_byteCodeType = Tcl_GetObjType("bytecode");
  assert(Nsf_OT_byteCodeType);

  Nsf_OT_tclCmdNameType = Tcl_GetObjType("cmdName");
  assert(Nsf_OT_tclCmdNameType);

  Nsf_OT_listType = Tcl_GetObjType("list");
  assert(Nsf_OT_listType);

  Nsf_OT_intType = Tcl_GetObjType("int");
  assert(Nsf_OT_intType);

  Nsf_OT_doubleType = Tcl_GetObjType("double");
  assert(Nsf_OT_doubleType);

  NsfMutexUnlock(&initMutex);


  /*
   * Initialize the pointer converter
   */
  Nsf_PointerInit(interp);
  /*
    fprintf(stderr, "SIZES: obj=%d, tcl_obj=%d, DString=%d, class=%d, namespace=%d, command=%d, HashTable=%d\n",
    sizeof(NsfObject), sizeof(Tcl_Obj), sizeof(Tcl_DString), sizeof(NsfClass),
    sizeof(Namespace), sizeof(Command), sizeof(Tcl_HashTable));
  */


#if defined(NSF_PROFILE)
  NsfProfileInit(interp);
#endif

  RUNTIME_STATE(interp)->doFilters = 1;
  RUNTIME_STATE(interp)->doCheckResults = 1;
  RUNTIME_STATE(interp)->doCheckArguments = 1;

  /* create nsf namespace */
  RUNTIME_STATE(interp)->NsfNS =
    Tcl_CreateNamespace(interp, "::nsf", NULL, (Tcl_NamespaceDeleteProc *)NULL);

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
    Tcl_CreateNamespace(interp, "::nsf::classes", NULL,
                        (Tcl_NamespaceDeleteProc *)NULL);
  MEM_COUNT_ALLOC("TclNamespace", RUNTIME_STATE(interp)->NsfClassesNS);


  /* cache interpreters proc interpretation functions */
  RUNTIME_STATE(interp)->objInterpProc = TclGetObjInterpProc();
  RUNTIME_STATE(interp)->exitHandlerDestroyRound = NSF_EXITHANDLER_OFF;

  RegisterExitHandlers(interp);
  NsfStringIncrInit(&RUNTIME_STATE(interp)->iss);
  /* initialize global Tcl_Obj */
  NsfGlobalObjs = NEW_ARRAY(Tcl_Obj*, nr_elements(NsfGlobalStrings));

  for (i = 0; i < nr_elements(NsfGlobalStrings); i++) {
    NsfGlobalObjs[i] = Tcl_NewStringObj(NsfGlobalStrings[i], -1);
    INCR_REF_COUNT(NsfGlobalObjs[i]);
  }

  /* create namespaces for the different command types */
  Tcl_CreateNamespace(interp, "::nsf::cmd", 0, (Tcl_NamespaceDeleteProc *)NULL);
  for (i=0; i < nr_elements(method_command_namespace_names); i++) {
    Tcl_CreateNamespace(interp, method_command_namespace_names[i], 0, (Tcl_NamespaceDeleteProc *)NULL);
  }

  /* create all method commands (will use the namespaces above) */
  for (i=0; i < nr_elements(method_definitions)-1; i++) {
    Tcl_CreateObjCommand(interp, method_definitions[i].methodName, method_definitions[i].proc, 0, 0);
  }

  /*
   * overwritten Tcl cmds
   */
  result = NsfShadowTclCommands(interp, SHADOW_LOAD);
  if (result != TCL_OK) {
    return result;
  }
  /*
   * new Tcl cmds
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

#ifdef NSF_BYTECODE
  NsfBytecodeInit();
#endif

  Tcl_SetVar(interp, "::nsf::version", NSF_VERSION, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::patchLevel", NSF_PATCHLEVEL, TCL_GLOBAL_ONLY);

  Tcl_AddInterpResolvers(interp,"nxt",
                         (Tcl_ResolveCmdProc *)InterpColonCmdResolver,
                         InterpColonVarResolver,
                         (Tcl_ResolveCompiledVarProc *)InterpCompiledColonVarResolver);
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
  Tcl_PkgProvideEx(interp, "nsf", PACKAGE_VERSION, &nsfStubs);
#  else
  Tcl_PkgProvideEx(interp, "nsf", PACKAGE_VERSION, &nsfConstStubPtr);
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

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
