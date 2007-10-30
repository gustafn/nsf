/* $Id: xotcl.c,v 1.43 2006/10/04 20:40:23 neumann Exp $
 *
 *  XOTcl - Extended Object Tcl
 *
 *  Copyright (C) 1999-2007 Gustaf Neumann (a), Uwe Zdun (a)
 *
 * (a) Vienna University of Economics and Business Administration
 *     Institute. of Information Systems and New Media
 *     A-1090, Augasse 2-6
 *     Vienna, Austria
 *
 * (b) University of Essen
 *     Specification of Software Systems
 *     Altendorferstraße 97-101 
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

#ifdef KEEP_TCL_CMD_TYPE
/*# include "tclCompile.h"*/
#endif

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
# define XOTcl_ExprObjCmd(clientData, interp, objc, objv)  \
    XOTclCallCommand(interp, XOTE_EXPR, objc, objv)
# define XOTcl_SubstObjCmd(clientData, interp, objc, objv) \
    XOTclCallCommand(interp, XOTE_SUBST, objc, objv)
#else
# define XOTcl_ExprObjCmd(clientData, interp, objc, objv)  \
    Tcl_ExprObjCmd(clientData, interp, objc, objv)
# define XOTcl_SubstObjCmd(clientData, interp, objc, objv) \
    Tcl_SubstObjCmd(clientData, interp, objc, objv)
#endif

static int createMethod(Tcl_Interp *interp, XOTclClass *cl, ClientData clientData,
                        int objc, Tcl_Obj *CONST objv[]);
static int SetXOTclObjectFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);
static void UpdateStringOfXOTclObject(Tcl_Obj *objPtr);
static void FreeXOTclObjectInternalRep(Tcl_Obj *objPtr);
static void DupXOTclObjectInternalRep(Tcl_Obj *src, Tcl_Obj *cpy);

static Tcl_Obj *NameInNamespaceObj(Tcl_Interp *interp, char *name, Tcl_Namespace *ns);
static Tcl_Namespace *callingNameSpace(Tcl_Interp *interp);
XOTCLINLINE static Tcl_Command NSFindCommand(Tcl_Interp *interp, char *name, Tcl_Namespace *ns);

XOTCLINLINE static void GuardAdd(Tcl_Interp *interp, XOTclCmdList *filterCL, Tcl_Obj *guard);
static int GuardCheck(Tcl_Interp *interp, Tcl_Obj *guards);
static int GuardCall(XOTclObject *obj, XOTclClass *cl, Tcl_Command cmd, Tcl_Interp *interp, Tcl_Obj *guard, int push);
static void GuardDel(XOTclCmdList *filterCL);
static int IsMetaClass(Tcl_Interp *interp, XOTclClass *cl);
static int hasMixin(Tcl_Interp *interp, XOTclObject *obj, XOTclClass *cl);
static int isSubType(XOTclClass *subcl, XOTclClass *cl);
static int setInstVar(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *name, Tcl_Obj *value);
static void MixinComputeDefined(Tcl_Interp *interp, XOTclObject *obj);

static Tcl_ObjType XOTclObjectType = {
    "XOTclObject",
    FreeXOTclObjectInternalRep,
    DupXOTclObjectInternalRep,
    UpdateStringOfXOTclObject,
    SetXOTclObjectFromAny
};

typedef enum { CALLING_LEVEL, ACTIVE_LEVEL } CallStackLevel;

typedef struct callFrameContext {
    int framesSaved;
    Tcl_CallFrame *framePtr;
    Tcl_CallFrame *varFramePtr;
} callFrameContext;
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

static int ObjDispatch(ClientData clientData, Tcl_Interp *interp, int objc,
		       Tcl_Obj *CONST objv[],  int flags);
XOTCLINLINE static int DoDispatch(ClientData clientData, Tcl_Interp *interp, int objc,
				  Tcl_Obj *CONST objv[], int flags);
static int XOTclNextMethod(XOTclObject *obj, Tcl_Interp *interp, XOTclClass *givenCl,
			   char *givenMethod, int objc, Tcl_Obj *CONST objv[],
			   int useCSObjs);

static int XOTclForwardMethod(ClientData clientData, Tcl_Interp *interp, int objc,
			      Tcl_Obj *CONST objv[]);
static int XOTclObjscopedMethod(ClientData clientData, Tcl_Interp *interp, int objc,
				Tcl_Obj *CONST objv[]);
static int XOTclSetterMethod(ClientData clientData, Tcl_Interp *interp, int objc, 
			     Tcl_Obj *CONST objv[]);

static int callDestroyMethod(ClientData clientData, Tcl_Interp *interp, XOTclObject *obj, int flags);
static int XOTclObjConvertObject(Tcl_Interp *interp, register Tcl_Obj *objPtr, XOTclObject **obj);

static XOTclObject *XOTclpGetObject(Tcl_Interp *interp, char *name);
static XOTclClass *XOTclpGetClass(Tcl_Interp *interp, char *name);
static XOTclCallStackContent *CallStackGetFrame(Tcl_Interp *interp);
#if !defined(NDEBUG)
static void checkAllInstances(Tcl_Interp *interp, XOTclClass *startCl, int lvl);
#endif






#ifdef PRE81
/* for backward compatibility only
 */
static int
Tcl_EvalObjv(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int flags) {
    int i, result;
    Tcl_DString ds, *dsp = &ds;

    assert(flags == 0);
    DSTRING_INIT(dsp);
    for (i = 0; i < objc; i++) {
        Tcl_DStringAppendElement(dsp, ObjStr(objv[i]));
    }
    result = Tcl_Eval(interp, Tcl_DStringValue(dsp));
    DSTRING_FREE(dsp);
    return result;
}
static int
Tcl_EvalEx(Tcl_Interp *interp, char *cmd, int len, int flags) {
    return Tcl_Eval(interp, cmd);
}
static int
Tcl_SubstObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    char *ov[20];
    int i;
    assert(objc<19);
    for (i=0; i<objc; i++)
        ov[i] = ObjStr(objv[i]);

    return Tcl_SubstCmd(clientData, interp, objc, ov);
}
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

# define VarHashRefCount(varPtr)                        \
    (*((int *) (((char *)(varPtr))+varRefCountOffset)))

# define VarHashGetValue(hPtr)                                  \
    (forwardCompatibleMode ?                                    \
     (Var *) ((char *)hPtr - TclOffset(VarInHash, entry)) :     \
     (Var *) Tcl_GetHashValue(hPtr)                             \
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
    (forwardCompatibleMode ?                    \
     ((Var85 *)varPtr)->flags :                 \
     (varPtr)->flags                            \
     )
#undef TclIsVarScalar
#define TclIsVarScalar(varPtr)                                  \
    (forwardCompatibleMode ?                                    \
     !(((Var85 *)varPtr)->flags & (VAR_ARRAY85|VAR_LINK85)) :   \
     ((varPtr)->flags & VAR_SCALAR)                             \
     )
#undef TclIsVarArray
#define TclIsVarArray(varPtr)                   \
    (forwardCompatibleMode ?                    \
     (((Var85 *)varPtr)->flags & VAR_ARRAY85) : \
     ((varPtr)->flags & VAR_ARRAY)              \
     )
#define TclIsVarNamespaceVar(varPtr)                    \
    (forwardCompatibleMode ?                            \
     (((Var85 *)varPtr)->flags & VAR_NAMESPACE_VAR) :   \
     ((varPtr)->flags & VAR_NAMESPACE_VAR)              \
     )

#define TclIsVarTraced(varPtr)                          \
    (forwardCompatibleMode ?                            \
     (((Var85 *)varPtr)->flags & VAR_ALL_TRACES85) :    \
     (varPtr->tracePtr != NULL)                         \
     )
#undef TclIsVarLink
#define TclIsVarLink(varPtr)                    \
    (forwardCompatibleMode ?                    \
     (((Var85 *)varPtr)->flags & VAR_LINK85) :  \
     (varPtr->flags & VAR_LINK)                 \
     )
#undef TclIsVarUndefined
#define TclIsVarUndefined(varPtr)                       \
    (forwardCompatibleMode ?                            \
     (((Var85 *)varPtr)->value.objPtr == NULL) :        \
     (varPtr->flags & VAR_UNDEFINED)                    \
     )
#undef TclSetVarLink
#define TclSetVarLink(varPtr)                                           \
    if (forwardCompatibleMode)                                          \
        ((Var85 *)varPtr)->flags = (((Var85 *)varPtr)->flags & ~VAR_ARRAY85) | VAR_LINK85; \
    else                                                                \
        (varPtr)->flags = ((varPtr)->flags & ~(VAR_SCALAR|VAR_ARRAY)) | VAR_LINK

#undef TclClearVarUndefined
#define TclClearVarUndefined(varPtr)            \
    if (!forwardCompatibleMode)                 \
        (varPtr)->flags &= ~VAR_UNDEFINED

#undef Tcl_CallFrame_compiledLocals
#define Tcl_CallFrame_compiledLocals(cf)                \
    (forwardCompatibleMode ?                            \
     (Var *)(((CallFrame85 *)cf)->compiledLocals) :     \
     (((CallFrame*)cf)->compiledLocals)                 \
     )

#define getNthVar(varPtr, i)                     \
    (forwardCompatibleMode ?                    \
     (Var *)(((Var85 *)varPtr)+(i)) :           \
     (((Var *)varPtr)+(i))                      \
     )

#define valueOfVar(type, varPtr, field)           \
    (forwardCompatibleMode ?                    \
     (type *)(((Var85 *)varPtr)->value.field) : \
     (type *)(((Var *)varPtr)->value.field)     \
     )
#endif


#if !FORWARD_COMPATIBLE
# define getNthVar(varPtr, i) (((Var *)varPtr)+(i))
#endif


#define TclIsCompiledLocalArgument(compiledLocalPtr)    \
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
        if (varPtr->hPtr != NULL) {
            Tcl_DeleteHashEntry(varPtr->hPtr);
        }
        ckfree((char *) varPtr);
    }
    if (arrayPtr != NULL) {
        if (TclIsVarUndefined(arrayPtr) && (arrayPtr->refCount == 0)
            && (arrayPtr->tracePtr == NULL)
            && (arrayPtr->flags & VAR_IN_HASHTABLE)) {
            if (arrayPtr->hPtr != NULL) {
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
        entryPtr = Tcl_FindHashEntry(varTable, simpleName);
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

#define ObjFindNamespace(interp, objPtr)                        \
    Tcl_FindNamespace((interp), ObjStr(objPtr), NULL, 0);

#else

/* 
 *  definitions for tcl 8.5
 */

#define VarHashGetValue(hPtr)                                   \
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
    ALLOC_ON_STACK(Tcl_Obj *, objc, tov);
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
    ALLOC_ON_STACK(Tcl_Obj *, objc, tov);

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

/*
 *  realize self, class, proc through the [self] command
 */

XOTCLINLINE static CONST84 char *
GetSelfProc(Tcl_Interp *interp) {
    /*return Tcl_GetCommandName(interp, RUNTIME_STATE(interp)->cs.top->cmdPtr);*/
    return Tcl_GetCommandName(interp, CallStackGetFrame(interp)->cmdPtr);

}

XOTCLINLINE static XOTclClass*
GetSelfClass(Tcl_Interp *interp) {
    /*return RUNTIME_STATE(interp)->cs.top->cl;*/
    return CallStackGetFrame(interp)->cl;
}

XOTCLINLINE static XOTclObject*
GetSelfObj(Tcl_Interp *interp) {
    return CallStackGetFrame(interp)->self;
}

/* extern callable GetSelfObj */
XOTcl_Object*
XOTclGetSelfObj(Tcl_Interp *interp) {
    return (XOTcl_Object*)GetSelfObj(interp);
}

XOTCLINLINE static Tcl_Command
GetSelfProcCmdPtr(Tcl_Interp *interp) {
    /*return RUNTIME_STATE(interp)->cs.top->cmdPtr;*/
    return CallStackGetFrame(interp)->cmdPtr;
}

/*
 * prints a msg to the screen that oldCmd is deprecated
 * optinal: give a new cmd
 */
extern void
XOTclDeprecatedMsg(char *oldCmd, char *newCmd) {
    fprintf(stderr, "**\n**\n** The command/method <%s> is deprecated.\n", oldCmd);
    if (newCmd)
        fprintf(stderr, "** Use <%s> instead.\n", newCmd);
    fprintf(stderr, "**\n");
}

static int
XOTcl_DeprecatedCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    char *new;
    if (objc == 2)
        new = 0;
    else if (objc == 3)
        new = ObjStr(objv[2]);
    else
        return XOTclObjErrArgCnt(interp, NULL, NULL, "deprecated oldcmd ?newcmd?");
    XOTclDeprecatedMsg(ObjStr(objv[1]), new);
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
    obj->refCount++;                                                    \
    fprintf(stderr, "RefCountIncr %p count=%d %s\n", obj, obj->refCount, obj->cmdName?ObjStr(obj->cmdName):"no name"); \
    MEM_COUNT_ALLOC("XOTclObject RefCount", obj)
# define XOTclObjectRefCountDecr(obj)                                   \
    obj->refCount--;                                                    \
    fprintf(stderr, "RefCountDecr %p count=%d\n", obj, obj->refCount);  \
    MEM_COUNT_FREE("XOTclObject RefCount", obj)
#else
# define XOTclObjectRefCountIncr(obj)           \
    obj->refCount++;                            \
    MEM_COUNT_ALLOC("XOTclObject RefCount", obj)
# define XOTclObjectRefCountDecr(obj)           \
    obj->refCount--;                            \
    MEM_COUNT_FREE("XOTclObject RefCount", obj)
#endif

#if defined(XOTCLOBJ_TRACE)
void objTrace(char *string, XOTclObject *obj) {
    if (obj)
        fprintf(stderr,"--- %s tcl %p %s (%d %p) xotcl %p (%d) %s \n", string,
                obj->cmdName, obj->cmdName->typePtr ? obj->cmdName->typePtr->name : "NULL",
                obj->cmdName->refCount, obj->cmdName->internalRep.twoPtrValue.ptr1,
                obj, obj->refCount, ObjStr(obj->cmdName));
    else
        fprintf(stderr,"--- No object: %s\n", string);
}
#else
# define objTrace(a, b)
#endif


/* search for tail of name */
static char *
NSTail(char *string) {
    register char *p = string + strlen(string);
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
#if REFCOUNT_TRACE
    fprintf(stderr,"###CLNO %p refcount = %d\n", obj, obj->refCount);
#endif
    if (obj->refCount <= 0) {
        assert(obj->refCount == 0);
        assert(obj->flags & XOTCL_DESTROYED);
#if REFCOUNT_TRACE
        fprintf(stderr,"###CLNO %p flags %x rc %d destr %d dc %d\n",
                obj, obj->flags,
                (obj->flags & XOTCL_REFCOUNTED) != 0,
                (obj->flags & XOTCL_DESTROYED) != 0,
                (obj->flags & XOTCL_DESTROY_CALLED) != 0
                );
#endif

        MEM_COUNT_FREE("XOTclObject/XOTclClass", obj);
#if defined(XOTCLOBJ_TRACE) || defined(REFCOUNT_TRACE)
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
static void
RegisterObjTypes() {
    Tcl_RegisterObjType(&XOTclObjectType);
}

static void
FreeXOTclObjectInternalRep(register Tcl_Obj *objPtr) {
    XOTclObject *obj = (XOTclObject*) objPtr->internalRep.otherValuePtr;

    /*  fprintf(stderr,"FIP objPtr %p obj %p obj->cmd %p '%s', bytes='%s'\n",
        objPtr, obj, obj->cmdName, ObjStr(obj->cmdName), objPtr->bytes);
    */
#if defined(XOTCLOBJ_TRACE)
    if (obj)
        fprintf(stderr,"FIP --- tcl %p (%d)\n", objPtr, objPtr->refCount);
#endif

#if !defined(REFCOUNTED)
    if (obj != NULL) {
        XOTclCleanupObject(obj);
    }
#else
    if (obj != NULL) {
#if REFCOUNT_TRACE
        fprintf(stderr, "FIP in %p\n", obj->teardown);
        fprintf(stderr, "FIP call is destroy %d\n", RUNTIME_STATE(obj->teardown)->callIsDestroy);
        fprintf(stderr,"FIP %p flags %x rc %d destr %d dc %d refcount = %d\n",
                obj, obj->flags,
                (obj->flags & XOTCL_REFCOUNTED) != 0,
                (obj->flags & XOTCL_DESTROYED) != 0,
                (obj->flags & XOTCL_DESTROY_CALLED) != 0,
                obj->refCount
                );
#endif
        if (obj->flags & XOTCL_REFCOUNTED &&
            !(obj->flags & XOTCL_DESTROY_CALLED)) {
            Tcl_Interp *interp = obj->teardown;
            INCR_REF_COUNT(obj->cmdName);
            callDestroyMethod((ClientData)obj, interp, obj, 0);
            /* the call to cleanup is the counterpart of the
               INCR_REF_COUNT(obj->cmdName) above */
            XOTclCleanupObject(obj);
        } else {
            fprintf(stderr, "BEFORE CLEANUPOBJ %x\n", (obj->flags & XOTCL_REFCOUNTED));
            XOTclCleanupObject(obj);
            fprintf(stderr, "AFTER CLEANUPOBJ\n");
        }
    }
#endif
    objPtr->internalRep.otherValuePtr = NULL;
    objPtr->typePtr = NULL;
}

static void
DupXOTclObjectInternalRep(Tcl_Obj *src, Tcl_Obj *cpy) {
    XOTclObject *obj = (XOTclObject*)src->internalRep.otherValuePtr;
#if defined(XOTCLOBJ_TRACE)
    if (obj) fprintf(stderr,"DIP --- tcl %p (%d)\n", src, src->refCount);
#endif
    XOTclObjectRefCountIncr(obj);
    cpy->internalRep.otherValuePtr = src->internalRep.otherValuePtr;
    cpy->typePtr = src->typePtr;
}

static int
SetXOTclObjectFromAny(Tcl_Interp *interp, register Tcl_Obj *objPtr) {
    Tcl_ObjType *oldTypePtr = objPtr->typePtr;
    char *string = ObjStr(objPtr);
    XOTclObject *obj;
    Tcl_Obj *tmpName = NULL;
    int result = TCL_OK;

#ifdef XOTCLOBJ_TRACE
    fprintf(stderr,"SetXOTclObjectFromAny %p '%s' %p\n",
            objPtr, string, objPtr->typePtr);
    if (oldTypePtr)
        fprintf(stderr,"   convert %s to XOTclObject\n", oldTypePtr->name);
#endif

    if (!isAbsolutePath(string)) {
        char *nsString;
        tmpName = NameInNamespaceObj(interp, string, callingNameSpace(interp));

        nsString = ObjStr(tmpName);
        INCR_REF_COUNT(tmpName);
        obj = XOTclpGetObject(interp, nsString);
        DECR_REF_COUNT(tmpName);
        if (!obj) {
            /* retry with global namespace */
            tmpName = Tcl_NewStringObj("::", 2);
            Tcl_AppendToObj(tmpName, string,-1);
            INCR_REF_COUNT(tmpName);
            obj = XOTclpGetObject(interp, ObjStr(tmpName));
            DECR_REF_COUNT(tmpName);
        }
    } else {
        obj = XOTclpGetObject(interp, string);
    }

#if 0
    obj = XOTclpGetObject(interp, string);
#endif

    if (obj) {
        if ((oldTypePtr != NULL) && (oldTypePtr->freeIntRepProc != NULL)) {
#ifdef XOTCLOBJ_TRACE
            fprintf(stderr,"   freeing type=%p, type=%s\n",
                    objPtr->typePtr, objPtr->typePtr ? objPtr->typePtr->name : "");
#endif
            oldTypePtr->freeIntRepProc(objPtr);
        }
        XOTclObjectRefCountIncr(obj);
#if defined(XOTCLOBJ_TRACE)
        fprintf(stderr, "SetXOTclObjectFromAny tcl %p (%d) xotcl %p (%d)\n",
                objPtr, objPtr->refCount, obj, obj->refCount);
#endif
        objPtr->internalRep.otherValuePtr = (XOTclObject*) obj;
        objPtr->typePtr = &XOTclObjectType;
    } else
        result = TCL_ERROR;

    return result;
}

static void
UpdateStringOfXOTclObject(register Tcl_Obj *objPtr) {
    XOTclObject *obj = (XOTclObject *)objPtr->internalRep.otherValuePtr;
    char *nsFullName = NULL;

#ifdef XOTCLOBJ_TRACE
    fprintf(stderr,"UpdateStringOfXOTclObject %p refCount %d\n",
            objPtr, objPtr->refCount);
    fprintf(stderr,"    teardown %p id %p destroyCalled %d\n",
            obj->teardown, obj->id, (obj->flags & XOTCL_DESTROY_CALLED));
#endif

    /* Here we use GetCommandName, because it doesnt need
       Interp*, but Tcl_GetCommandFullName(interp, obj->id, ObjName); does*/
    if (obj && !(obj->flags & XOTCL_DESTROY_CALLED)) {
        Tcl_DString ds, *dsp = &ds;
        unsigned l;
        DSTRING_INIT(dsp);
        nsFullName = NSCmdFullName(obj->id);
        if (!(*nsFullName==':' && *(nsFullName+1)==':' &&
              *(nsFullName+2)=='\0')) {
            Tcl_DStringAppend(dsp, nsFullName, -1);
        }
        Tcl_DStringAppend(dsp, "::", 2);
        Tcl_DStringAppend(dsp, Tcl_GetCommandName(NULL, obj->id), -1);

        l = (unsigned) Tcl_DStringLength(dsp)+1;
        objPtr->bytes = (char *) ckalloc(l);
        memcpy(objPtr->bytes, Tcl_DStringValue(dsp), l);
        objPtr->length = Tcl_DStringLength(dsp);
        DSTRING_FREE(dsp);
    } else if (obj) {
        fprintf(stderr,"try to read string of deleted command\n");
        FreeXOTclObjectInternalRep(objPtr);
        objPtr->bytes = NULL;
        objPtr->length = 0;
    } else {
        objPtr->bytes = NULL;
        objPtr->length = 0;
    }
    /*
      fprintf(stderr, "+++UpdateStringOfXOTclObject bytes='%s', length=%d\n",
      objPtr->bytes, objPtr->length);
    */
}

#ifdef NOTUSED
static Tcl_Obj *
NewXOTclObjectObj(register XOTclObject *obj) {
    register Tcl_Obj *objPtr = 0;
    XOTclNewObj(objPtr);
    objPtr->bytes = NULL;
    objPtr->internalRep.otherValuePtr = obj;
    objPtr->typePtr = &XOTclObjectType;
#ifdef XOTCLOBJ_TRACE
    fprintf(stderr,"NewXOTclObjectObj %p\n", objPtr);
#endif
    return objPtr;
}
#endif

static Tcl_Obj *
NewXOTclObjectObjName(register XOTclObject *obj, char *name, unsigned l)
{
    register Tcl_Obj *objPtr = 0;

    XOTclNewObj(objPtr);

    objPtr->length = l;
    objPtr->bytes = ckalloc(l+1);
    memcpy(objPtr->bytes, name, l);
    *(objPtr->bytes+l) = 0;
    objPtr->internalRep.otherValuePtr = obj;
    objPtr->typePtr = &XOTclObjectType;

#ifdef XOTCLOBJ_TRACE
    fprintf(stderr,"NewXOTclObjectObjName tcl %p (%d) xotcl %p (%d) %s\n",
	    objPtr, objPtr->refCount, obj, obj->refCount, objPtr->bytes);
#endif
    XOTclObjectRefCountIncr(obj);

    return objPtr;
}

#ifdef KEEP_TCL_CMD_TYPE
XOTCLINLINE static Tcl_ObjType *
GetCmdNameType(Tcl_ObjType *cmdType) {
    static Tcl_ObjType *tclCmdNameType = NULL;
  
    if (tclCmdNameType == NULL) {
# if defined(PRE82)
        if (cmdType 
            && cmdType != &XOTclObjectType 
            && !strcmp(cmdType->name,"cmdName")) {
            tclCmdNameType = cmdType;
        }
# else
        static XOTclMutex initMutex = 0;
        XOTclMutexLock(&initMutex);
        if (tclCmdNameType == NULL)
            tclCmdNameType = Tcl_GetObjType("cmdName");
        XOTclMutexUnlock(&initMutex);
# endif
    }
    return tclCmdNameType;
}
#endif

#if NOTUSED
static int 
XOTclObjGetObject(Tcl_Interp *interp, register Tcl_Obj *objPtr, XOTclObject **obj) {
    int result;
    register Tcl_ObjType *cmdType = objPtr->typePtr;
    XOTclObject *o;

    if (cmdType == &XOTclObjectType) {
        o = (XOTclObject*) objPtr->internalRep.otherValuePtr;
        if (!(o->flags & XOTCL_DESTROYED)) {
            *obj = o;
            return TCL_OK;
        }
    }

    if (cmdType == GetCmdNameType(cmdType)) {
        Tcl_Command cmd = Tcl_GetCommandFromObj(interp, objPtr);
        /*fprintf(stderr,"obj is of type tclCmd\n");*/
        if (cmd) {
            o = XOTclGetObjectFromCmdPtr(cmd);
            if (o) {
                *obj = o;
                return TCL_OK;
            }
        }
    }
  
    o = XOTclpGetObject(interp, ObjStr(objPtr));
    if (o) {
        *obj = o;
        return TCL_OK;
    }
    return TCL_ERROR;
}
#endif

static int
XOTclObjConvertObject(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclObject **obj) {
    int result;
    register Tcl_ObjType *cmdType = objPtr->typePtr;
    /*
     * Only really share the "::x" Tcl_Objs but not "x" because we so not have
     * references upon object kills and then will get dangling
     * internalRep references to killed XOTclObjects
     */
    if (cmdType == &XOTclObjectType) {
        /*fprintf(stderr,"obj is of type XOTclObjectType\n");*/
        if (obj) {
            XOTclObject *o = (XOTclObject*) objPtr->internalRep.otherValuePtr;
            int refetch = 0;
            if (o->flags & XOTCL_DESTROYED) {
                /* fprintf(stderr,"????? calling free by hand\n"); */
                FreeXOTclObjectInternalRep(objPtr);
                refetch = 1;
                result = SetXOTclObjectFromAny(interp, objPtr);
                if (result == TCL_OK) {
                    o = (XOTclObject*) objPtr->internalRep.otherValuePtr;
                    assert(o && !(o->flags & XOTCL_DESTROYED));
                }
            } else {
                result = TCL_OK;
            }

            *obj = o;

#ifdef XOTCLOBJ_TRACE
            if (result == TCL_OK)
                fprintf(stderr,"XOTclObjConvertObject tcl %p (%d) xotcl %p (%d) r=%d %s\n",
                        objPtr, objPtr->refCount, o, o->refCount, refetch, objPtr->bytes);
            else
                fprintf(stderr,"XOTclObjConvertObject tcl %p (%d) **** rc=%d r=%d %s\n",
                        objPtr, objPtr->refCount, result, refetch, objPtr->bytes);
#endif
        } else {
            result = TCL_OK;
        }
#ifdef KEEP_TCL_CMD_TYPE
    } else if (cmdType == GetCmdNameType(cmdType)) {
        Tcl_Command cmd = Tcl_GetCommandFromObj(interp, objPtr);
        /*fprintf(stderr,"obj %s is of type tclCmd, cmd=%p\n", ObjStr(objPtr), cmd);*/
        if (cmd) {
            XOTclObject *o = XOTclGetObjectFromCmdPtr(cmd);

            /*fprintf(stderr,"Got Object from '%s' %p\n", objPtr->bytes, o);
              fprintf(stderr,"cmd->objProc %p == %p, proc=%p\n",
	      Tcl_Command_objProc(cmd), XOTclObjDispatch,
	      Tcl_Command_proc(cmd) );*/

            if (o) {
                if (obj) *obj = o;
                result = TCL_OK;
            } else {
                goto convert_to_xotcl_object;
            }
        } else goto convert_to_xotcl_object;
#endif
    } else {
#ifdef KEEP_TCL_CMD_TYPE
    convert_to_xotcl_object:
#endif
        result = SetXOTclObjectFromAny(interp, objPtr);
        if (result == TCL_OK && obj) {
            *obj = (XOTclObject*) objPtr->internalRep.otherValuePtr;
        }
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
NameInNamespaceObj(Tcl_Interp *interp, char *name, Tcl_Namespace *ns) {
    Tcl_Obj *objName;
    int len;
    char *p;

    /*fprintf(stderr,"NameInNamespaceObj %s (%p) ", name, ns);*/
    if (!ns)
        ns = Tcl_GetCurrentNamespace(interp);
    objName = Tcl_NewStringObj(ns->fullName,-1);
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



static int
GetXOTclClassFromObj(Tcl_Interp *interp, register Tcl_Obj *objPtr,
		     XOTclClass **cl, XOTclClass *base) {
    XOTclObject *obj;
    XOTclClass *cls = NULL;
    int result = TCL_OK;
    char *objName = ObjStr(objPtr);

    /*fprintf(stderr, "GetXOTclClassFromObj %s base %p\n", objName, base);*/

    if (base) {
        /* we refer to an existing object; use command resolver */
        if (!isAbsolutePath(objName)) {
            Tcl_Command cmd = NSFindCommand(interp, objName, callingNameSpace(interp));

            /*fprintf(stderr, "GetXOTclClassFromObj %s cmd = %p cl=%p base=%p\n",
              objName, cmd, cmd ? XOTclGetClassFromCmdPtr(cmd) : NULL, base);*/
            if (cmd) {
                cls = XOTclGetClassFromCmdPtr(cmd);
                if (cl) *cl = cls;
            }
        }
    }

    if (!cls) {
        result = XOTclObjConvertObject(interp, objPtr, &obj);
        if (result == TCL_OK) {
            cls = XOTclObjectToClass(obj);
            if (cls) {
                if (cl) *cl = cls;
            } else {
                /* we have an object, but no class */
                result = TCL_ERROR;
            }
        }
    }

    if (!cls && base) {
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
            result = GetXOTclClassFromObj(interp, objPtr, cl, 0);
        }
        DECR_REF_COUNT(ov[2]);
    }

    /*fprintf(stderr, "GetXOTclClassFromObj %s returns %d cls = %p *cl = %p\n",
      objName, result, cls, cl?*cl:NULL);*/
    return result;
}

extern void
XOTclClassListFree(XOTclClasses *sl) {
    XOTclClasses *n;
    for (; sl; sl = n) {
        n = sl->nextPtr;
        FREE(XOTclClasses, sl);
    }
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
    for (; sl != 0; sl = sl->nextPtr) {
        XOTclClass *sc = sl->cl;
        if (sc->color == GRAY) { cl->color = WHITE; return 0; }
        if (sc->color == WHITE && !TopoSort(sc, base, next)) {
            cl->color = WHITE;
            if (cl == base) {
                XOTclClasses *pc = cl->order;
                while (pc != 0) { pc->cl->color = WHITE; pc = pc->nextPtr; }
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
        XOTclClasses *pc = cl->order;
        while (pc != 0) { pc->cl->color = WHITE; pc = pc->nextPtr; }
    }
    return 1;
}

static XOTclClasses*
TopoOrder(XOTclClass *cl, XOTclClasses *(*next)(XOTclClass*)) {
    if (TopoSort(cl, cl, next))
        return cl->order;
    XOTclClassListFree(cl->order);
    cl->order = 0;
    return 0;
}

static XOTclClasses*
ComputeOrder(XOTclClass *cl, XOTclClasses *order, XOTclClasses *(*direction)(XOTclClass*)) {
    if (order)
        return order;
    return (cl->order = TopoOrder(cl, direction));
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
FlushPrecedences(XOTclClass *cl) {
    XOTclClasses *pc;
    XOTclClassListFree(cl->order);
    cl->order = 0;
    pc = ComputeOrder(cl, cl->order, Sub);

    /*
     * ordering doesn't matter here - we're just using toposort
     * to find all lower classes so we can flush their caches
     */

    if (pc) pc = pc->nextPtr;
    for (; pc; pc = pc->nextPtr) {
        XOTclClassListFree(pc->cl->order);
        pc->cl->order = 0;
    }
    XOTclClassListFree(cl->order);
    cl->order = 0;
}

static void
AddInstance(XOTclObject *obj, XOTclClass *cl) {
    obj->cl = cl;
    if (cl != 0) {
        int nw;
        (void) Tcl_CreateHashEntry(&cl->instances, (char *)obj, &nw);
    }
}

static int
RemoveInstance(XOTclObject *obj, XOTclClass *cl) {
    if (cl) {
        Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&cl->instances, (char *)obj);
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
/*XOTCLINLINE*/
static Tcl_Command
FindMethod(char *methodName, Tcl_Namespace *nsPtr) {
    Tcl_HashEntry *entryPtr;

    /* if somebody messes around with the deleteProc, we could conclude that the
       entries of the cmdTable are not ours ... we did that in earlier versions */
    /*fprintf(stderr,"FindMethod '%s', ns=%p \n", methodName, nsPtr);*/

    if ((entryPtr = Tcl_FindHashEntry(Tcl_Namespace_cmdTable(nsPtr), methodName))) {
        return (Tcl_Command) Tcl_GetHashValue(entryPtr);
    } 
    return NULL;
}

static XOTclClass*
SearchPLMethod(register XOTclClasses *pl, char *nm, Tcl_Command *cmd) {
    /* Search the class hierarchy */
    for (; pl;  pl = pl->nextPtr) {
        Tcl_Command pi = FindMethod(nm, pl->cl->nsPtr);
        if (pi) {
            *cmd = pi;
            return pl->cl;
        }
    }
    return NULL;
}


static XOTclClass*
SearchCMethod(XOTclClass *cl, char *nm, Tcl_Command *cmd) {
    assert(cl);
    return SearchPLMethod(ComputeOrder(cl, cl->order, Super), nm, cmd);
}

static int
callDestroyMethod(ClientData clientData, Tcl_Interp *interp, XOTclObject *obj, int flags) {
    int result;

    /* don't call destroy after exit handler started physical
       destruction */
    if (RUNTIME_STATE(interp)->exitHandlerDestroyRound ==
        XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY)
        return TCL_OK;

    /* fprintf(stderr," obj %p flags %.4x %d\n", obj, obj->flags,
       RUNTIME_STATE(interp)->callDestroy);*/
    /* we don't call destroy, if we're in the exit handler
       during destruction of Object and Class */
    if (!RUNTIME_STATE(interp)->callDestroy) {
        obj->flags |= XOTCL_DESTROY_CALLED;
        /* return TCL_ERROR so that clients know we haven't deleted the
           associated command yet */
        return TCL_ERROR;
    }
    /*fprintf(stderr, "+++ calldestroy flags=%d\n", flags);*/
    if (obj->flags & XOTCL_DESTROY_CALLED)
        return TCL_OK;

#if !defined(NDEBUG)
    {char *cmdName = ObjStr(obj->cmdName);
        assert(cmdName != NULL);
        /*fprintf(stderr,"findCommand %s -> %p obj->id %p\n", cmdName,
          Tcl_FindCommand(interp, cmdName, NULL, 0), obj->id);*/
        /*assert(Tcl_FindCommand(interp, cmdName, NULL, 0) != NULL);*/
        /*fprintf(stderr,"callDestroyMethod: %p command to be destroyed '%s' does not exist\n",
          obj, cmdName);*/
    }
#endif


#ifdef OBJDELETION_TRACE
    fprintf(stderr, "   command found\n");
    PRINTOBJ("callDestroy", obj);
#endif
    result = callMethod(clientData, interp, XOTclGlobalObjects[XOTE_DESTROY], 2, 0, flags);
    if (result != TCL_OK) {
        static char cmd[] =
            "puts stderr \"[self]: Error in method destroy\n\
	 $::errorCode $::errorInfo\"";
        Tcl_EvalEx(interp, cmd, -1, 0);
        if (++RUNTIME_STATE(interp)->errorCount > 20)
            panic("too many destroy errors occured. Endless loop?", NULL);
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
    }
    return cl->opt;
}




static Tcl_Namespace*
NSGetFreshNamespace(Tcl_Interp *interp, ClientData clientData, char *name);

static void
makeObjNamespace(Tcl_Interp *interp, XOTclObject *obj) {
#ifdef NAMESPACE_TRACE
    fprintf(stderr, "+++ Make Namespace for %s\n", ObjStr(obj->cmdName));
#endif
    if (!obj->nsPtr) {
        Tcl_Namespace *nsPtr;
        char *cmdName =  ObjStr(obj->cmdName);
        obj->nsPtr = NSGetFreshNamespace(interp, (ClientData)obj, cmdName);
        if (!obj->nsPtr)
            panic("makeObjNamespace: Unable to make namespace", 0);
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
            for (hPtr = Tcl_FirstHashEntry(varHashTable, &search);  hPtr != NULL;
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
            obj->varTable = 0;
        }
    }
}
/*
  typedef int (Tcl_ResolveVarProc) _ANSI_ARGS_((
  *	        Tcl_Interp *interp, CONST char  *name, Tcl_Namespace *context,
  *	        int flags, Tcl_Var *rPtr));
  */
int
varResolver(Tcl_Interp *interp, CONST char *name, Tcl_Namespace *ns, int flags, Tcl_Var *varPtr) {
    *varPtr = (Tcl_Var)LookupVarFromTable(Tcl_Namespace_varTable(ns), name, NULL);
    /*fprintf(stderr,"lookup '%s' successful %d\n", name, *varPtr != NULL);*/
    return *varPtr ? TCL_OK : TCL_ERROR;
}

static Tcl_Namespace *
requireObjNamespace(Tcl_Interp *interp, XOTclObject *obj) {
    if (!obj->nsPtr) makeObjNamespace(interp, obj);
    /*
      Tcl_SetNamespaceResolvers(obj->nsPtr, (Tcl_ResolveCmdProc*)NULL,
      varResolver, (Tcl_ResolveCompiledVarProc*)NULL);
    */
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
NSDeleteCmd(Tcl_Interp *interp, Tcl_Namespace *ns, char *name) {
    /* a simple deletion would delete a global command with
       the same name, if the command is not existing, so
       we use the CmdToken */
    Tcl_Command token;
    assert(ns);
    if ((token = FindMethod(name, ns))) {
        return Tcl_DeleteCommandFromToken(interp, token);
    }
    return -1;
}

static void
CallStackDestroyObject(Tcl_Interp *interp, XOTclObject *obj);
static void
PrimitiveCDestroy(ClientData clientData);
static void
PrimitiveODestroy(ClientData clientData);

static void
NSDeleteChildren(Tcl_Interp *interp, Tcl_Namespace *ns) {
    Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(ns);
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr;

#ifdef OBJDELETION_TRACE
    fprintf(stderr, "NSDeleteChildren %s\n", ns->fullName);
#endif

    Tcl_ForgetImport(interp, ns, "*"); /* don't destroy namespace imported objects */

    for (hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch); hPtr != 0; 
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
                /* fprintf(stderr, " ... obj= %s\n", ObjStr(obj->cmdName));*/
	
                /* in the exit handler physical destroy --> directly call destroy */
                if (RUNTIME_STATE(interp)->exitHandlerDestroyRound
                    == XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY) {
                    if (XOTclObjectIsClass(obj))
                        PrimitiveCDestroy((ClientData) obj);
                    else
                        PrimitiveODestroy((ClientData) obj);
                } else {
                    if (obj->teardown != 0 && obj->id &&
                        !(obj->flags & XOTCL_DESTROY_CALLED)) {
                        if (callDestroyMethod((ClientData)obj, interp, obj, 0) != TCL_OK) {
                            /* destroy method failed, but we have to remove the command
                               anyway. */
                            obj->flags |= XOTCL_DESTROY_CALLED;
	
                            if (obj->teardown) {
                                CallStackDestroyObject(interp, obj);
                            }
                            /*(void*) Tcl_DeleteCommandFromToken(interp, oid);*/
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

/* delete a namespace recursively, but check whether the
   namespace is an object or not */
static void
NSDeleteNamespace(Tcl_Interp *interp, Tcl_Namespace *ns) {
    Tcl_HashTable *childTable = Tcl_Namespace_childTable(ns);
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr;

    NSDeleteChildren(interp, ns);
    for (hPtr = Tcl_FirstHashEntry(childTable, &hSrch); hPtr != 0;
         hPtr = Tcl_NextHashEntry(&hSrch)) {
        Tcl_Namespace *child = ((Tcl_Namespace*)Tcl_GetHashValue(hPtr));
        NSDeleteNamespace(interp, child);
    }
    /*	
        fprintf(stderr, "NSDeleteNamespace deleting %s\n", ns->fullName);
    */
    MEM_COUNT_FREE("TclNamespace", ns);
    Tcl_DeleteNamespace(ns);
}

static int
XOTcl_DeleteCommandFromToken(Tcl_Interp *interp, Tcl_Command cmd) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    XOTclCallStackContent *csc = cs->top;

    for (; csc > cs->content; csc--) {
        if (csc->cmdPtr == cmd) {
            csc->cmdPtr = NULL;
        }
    }
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
    /*
     * Delete all variables and initialize var table again
     * (DeleteVars frees the vartable)
     */
    TclDeleteVars((Interp *)interp, varTable);
    InitVarHashTable(varTable, (Namespace *)ns);

    /*
     * Delete all user-defined procs in the namespace
     */
    for (hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch); hPtr != 0;
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
    /* fprintf(stderr,"namespacedeleteproc obj=%p\n", clientData);*/
    if (obj) {
        obj->flags |= XOTCL_NS_DESTROYED;
        obj->nsPtr = NULL;
    }
}

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
    if (Tcl_Namespace_deleteProc(nsPtr) != NULL) {
        /*fprintf(stderr,"calling deteteNamespace\n");*/
        Tcl_DeleteNamespace(nsPtr);
    }
}

static Tcl_Namespace*
NSGetFreshNamespace(Tcl_Interp *interp, ClientData clientData, char *name) {
    Tcl_Namespace *ns = Tcl_FindNamespace(interp, name, NULL, 0);

    if (ns) {
        if (ns->deleteProc != NULL || ns->clientData != NULL) {
            panic("Namespace '%s' exists already with delProc %p and clientData %p; Can only convert a plain Tcl namespace into an XOTcl namespace",
                  name, ns->deleteProc, ns->clientData);
        }
        ns->clientData = clientData;
        ns->deleteProc = (Tcl_NamespaceDeleteProc*) NSNamespaceDeleteProc;
    } else {
        ns = Tcl_CreateNamespace(interp, name, clientData,
                                 (Tcl_NamespaceDeleteProc*) NSNamespaceDeleteProc);
    }
    MEM_COUNT_ALLOC("TclNamespace", ns);
    return ns;
}


/*
 * check colons for illegal object/class names
 */
XOTCLINLINE static int
NSCheckColons(char *name, unsigned l) {
    register char *n = name;
    if (*n == '\0') return 0; /* empty name */
    if (l==0) l=strlen(name);
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
NSCheckForParent(Tcl_Interp *interp, char *name, unsigned l) {
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

        if (Tcl_FindNamespace(interp, parentName, (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY) == 0) {
            XOTclObject *parentObj = (XOTclObject*) XOTclpGetObject(interp, parentName);
            if (parentObj) {
                /* this is for classes */
                requireObjNamespace(interp, parentObj);
            } else {
                /* call unknown and try again */
                Tcl_Obj *ov[3];
                int rc;
                ov[0] = RUNTIME_STATE(interp)->theClass->object.cmdName;
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
                                                (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY) != 0);
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
    cmd = NSFindCommand(interp, name, NULL);
  
    /*if (cmd) {
      fprintf(stderr,"+++ XOTclGetObject from %s -> objProc=%p, dispatch=%p\n",
      name, Tcl_Command_objProc(cmd), XOTclObjDispatch);
      }*/

    if (cmd && Tcl_Command_objProc(cmd) == XOTclObjDispatch) {
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
XOTclAddObjectMethod(Tcl_Interp *interp, XOTcl_Object *object, char *methodName, 
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
XOTclAddPMethod(Tcl_Interp *interp, XOTcl_Object *object, char *methodName, 
                Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp) {
    int flags = 0;
    if (clientData == (ClientData) XOTCL_NONLEAF_METHOD) {
        fprintf(stderr, "XOTclAddPMethod(,,,, XOTCL_NONLEAF_METHOD,) deprecated.\n"
                "Use XOTclAddObjectMethod(,,,,,, XOTCL_NONLEAF_METHOD) instead.\n");
        flags = XOTCL_NONLEAF_METHOD;
        clientData = NULL;
    }
    return XOTclAddObjectMethod(interp, object, methodName, proc, clientData, dp, flags);
}

Tcl_Command
XOTclAddInstanceMethod(Tcl_Interp *interp, XOTcl_Class *class, char *methodName,
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
XOTclAddIMethod(Tcl_Interp *interp, XOTcl_Class *class, char *methodName, 
                Tcl_ObjCmdProc *proc, ClientData clientData, Tcl_CmdDeleteProc *dp) {
    int flags = 0;
    if (clientData == (ClientData) XOTCL_NONLEAF_METHOD) {
        fprintf(stderr, "XOTclAddIMethod(,,,, XOTCL_NONLEAF_METHOD,) deprecated.\n"
                "Use XOTclAddInstanceMethod(,,,,,, XOTCL_NONLEAF_METHOD) instead.\n");
        flags = XOTCL_NONLEAF_METHOD;
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
#ifdef PRE83
    int flgs = 0;
#else
    int flgs = TCL_LEAVE_ERR_MSG;
#endif
    XOTcl_FrameDecls;

    XOTcl_PushFrame(interp, obj);
    if (obj->nsPtr)
        flgs |= TCL_NAMESPACE_ONLY;

    valueObject = Tcl_ObjGetVar2(interp, XOTclGlobalObjects[XOTE_AUTONAMES], name, flgs);
    if (valueObject != NULL ) {
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
        if (valueObject != NULL) { /* we have an entry */
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
            ALLOC_ON_STACK(Tcl_Obj *, 3, ov);
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

XOTclCallStackContent *
XOTclCallStackFindLastInvocation(Tcl_Interp *interp, int offset) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    register XOTclCallStackContent *csc = cs->top;
    int topLevel = csc->currentFramePtr ? Tcl_CallFrame_level(csc->currentFramePtr) :0;
    int deeper = offset;

    /* skip through toplevel inactive filters, do this offset times */
    for (csc=cs->top; csc > cs->content; csc--) {
        if ((csc->callType & XOTCL_CSC_CALL_IS_NEXT) ||
            (csc->frameType & XOTCL_CSC_TYPE_INACTIVE))
            continue;
        if (offset)
            offset--;
        else {
            if (!deeper) {
                return csc;
            }
            if (csc->currentFramePtr && Tcl_CallFrame_level(csc->currentFramePtr) < topLevel) {
                return csc;
            }
        }
    }
    /* for some reasons, we could not find invocation (topLevel, destroy) */
    return NULL;
}

static XOTclCallStackContent *
CallStackFindActiveFilter(Tcl_Interp *interp) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    register XOTclCallStackContent *csc;

    /* search for first active frame and set tcl frame pointers */
    for (csc=cs->top; csc > cs->content; csc --) {
        if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) return csc;
    }
    /* for some reasons, we could not find invocation (topLevel, destroy) */
    return NULL;
}

XOTclCallStackContent *
XOTclCallStackFindActiveFrame(Tcl_Interp *interp, int offset) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    register XOTclCallStackContent *csc;

    /* search for first active frame and set tcl frame pointers */
    for (csc=cs->top-offset; csc > cs->content; csc --) {
        if (!(csc->frameType & XOTCL_CSC_TYPE_INACTIVE)) {
            /* we found the highest active frame */
            return csc;
        }
    }
    /* we could not find an active frame; called from toplevel? */
    return NULL;
}

static void
CallStackUseActiveFrames(Tcl_Interp *interp, callFrameContext *ctx) {
    XOTclCallStackContent *active, *top = RUNTIME_STATE(interp)->cs.top;
    Tcl_CallFrame *inFramePtr =  (Tcl_CallFrame *) Tcl_Interp_varFramePtr(interp);

    active = XOTclCallStackFindActiveFrame(interp, 0);
    /*fprintf(stderr,"active %p, varFrame(interp) %p, topVarFrame %p, active->curr %p\n",
      active, inFramePtr, top->currentFramePtr,
      active? active->currentFramePtr : NULL);*/

    if (active == top || inFramePtr == NULL || Tcl_CallFrame_level(inFramePtr) == 0) {
        /* top frame is a active frame, or we could not find a calling
           frame, call frame pointers are fine */
        ctx->framesSaved = 0;
    } else if (active == NULL) {
        Tcl_CallFrame *cf = inFramePtr;
        /*fprintf(stderr,"active == NULL\n"); */
        /* find a proc frame, which is not equal the top level cmd */
        /* XOTclStackDump(interp);*/
        for (; cf && Tcl_CallFrame_level(cf); cf = Tcl_CallFrame_callerPtr(cf)) {
            if (Tcl_CallFrame_isProcCallFrame(cf) && cf != top->currentFramePtr)
                break;
        }
        ctx->varFramePtr = inFramePtr;
        Tcl_Interp_varFramePtr(interp) = (CallFrame *) cf;
        ctx->framesSaved = 1;
    } else {
        Tcl_CallFrame *framePtr;
        /*fprintf(stderr,"active == deeper active=%p frame %p, active+1 %p frame %p\n",
          active, active->currentFramePtr,
          active+1, (active+1)->currentFramePtr);*/
        /* search returned a deeper pointer, use stored tcl frame pointers;
           If Tcl is mixed with XOTcl it is needed to use instead of
           active->currentFrame the callerPtr of the last inactive frame
           unless the last inactive is NULL */
        if ((framePtr = (active+1)->currentFramePtr))
            framePtr = Tcl_CallFrame_callerPtr(framePtr);
        else
            framePtr = active->currentFramePtr;
        ctx->varFramePtr = inFramePtr;
        Tcl_Interp_varFramePtr(interp) = (CallFrame *) framePtr;
        ctx->framesSaved = 1;
    }
}

static void
CallStackRestoreSavedFrames(Tcl_Interp *interp, callFrameContext *ctx) {
    if (ctx->framesSaved) {
        Tcl_Interp_varFramePtr(interp) = (CallFrame *)ctx->varFramePtr;
        /*RUNTIME_STATE(interp)->varFramePtr = ctx->varFramePtr;*/

    }
}


XOTCLINLINE static int
CallStackPush(Tcl_Interp *interp, XOTclObject *obj, XOTclClass *cl,
	      Tcl_Command cmd, int objc, Tcl_Obj *CONST objv[], int frameType) {
    XOTclCallStack *cs;
    register XOTclCallStackContent *csc;

    cs = &RUNTIME_STATE(interp)->cs;
    if (cs->top >= &cs->content[MAX_NESTING_DEPTH-1]) {
        Tcl_SetResult(interp, "too many nested calls to Tcl_EvalObj (infinite loop?)",
                      TCL_STATIC);
        return TCL_ERROR;
    }
    /*fprintf(stderr, "CallStackPush sets self\n");*/
    csc = ++cs->top;
    csc->self          = obj;
    csc->cl            = cl;
    csc->cmdPtr        = cmd;
    csc->destroyedCmd  = 0;
    csc->frameType     = frameType;
    csc->callType      = 0;
    csc->currentFramePtr = NULL; /* this will be set by InitProcNSCmd */

    if (frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER)
        csc->filterStackEntry = obj->filterStack;
    else
        csc->filterStackEntry = 0;

    /*fprintf(stderr, "PUSH obj %s, self=%p cmd=%p (%s) id=%p (%s) frame=%p\n",
      ObjStr(obj->cmdName), obj,
      cmd, (char *) Tcl_GetCommandName(interp, cmd),
      obj->id, Tcl_GetCommandName(interp, obj->id), csc);*/

    MEM_COUNT_ALLOC("CallStack", NULL);
    return TCL_OK;
}

XOTCLINLINE static void
CallStackDoDestroy(Tcl_Interp *interp, XOTclObject *obj) {
    Tcl_Command oid;

    PRINTOBJ("CallStackDoDestroy", obj);
    oid = obj->id;
    obj->id = 0;
    if (obj->teardown && oid) {
        Tcl_Obj *savedObjResult = Tcl_GetObjResult(interp);
        INCR_REF_COUNT(savedObjResult);
        Tcl_DeleteCommandFromToken(interp, oid); /* this can change the result */
        Tcl_SetObjResult(interp, savedObjResult);
        DECR_REF_COUNT(savedObjResult);
    }
}


static void
CallStackDestroyObject(Tcl_Interp *interp, XOTclObject *obj) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    XOTclCallStackContent *csc;
    int countSelfs = 0;
    Tcl_Command oid = obj->id;

    for (csc = &cs->content[1]; csc <= cs->top; csc++) {
        if (csc->self == obj) {
            csc->destroyedCmd = oid;
            csc->callType |= XOTCL_CSC_CALL_IS_DESTROY;
            /*fprintf(stderr,"setting destroy on frame %p for obj %p\n", csc, obj);*/
            if (csc->destroyedCmd) {
                Tcl_Command_refCount(csc->destroyedCmd)++;
                MEM_COUNT_ALLOC("command refCount", csc->destroyedCmd);
            }
            countSelfs++;
        }
    }
    /* if the object is not referenced at the callstack anymore
       we have to directly destroy it, because CallStackPop won't
       find the object destroy */
    if (countSelfs == 0) {
        /*fprintf(stderr,"directdestroy %p\n", obj);*/
        CallStackDoDestroy(interp, obj);
    } else {
        /*fprintf(stderr,"selfcount for %p = %d\n", obj, countSelfs);*/
        /* to prevail the deletion order call delete children now
           -> children destructors are called before parent's
           destructor */
        if (obj->teardown && obj->nsPtr) {
            NSDeleteChildren(interp, obj->nsPtr);
        }
    }
}

XOTCLINLINE static int
CallStackIsDestroyed(Tcl_Interp *interp) {
    return (RUNTIME_STATE(interp)->cs.top->destroyedCmd == NULL) ? 0 : 1;
}

XOTCLINLINE static void
CallStackPop(Tcl_Interp *interp) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    XOTclCallStackContent *csc;
    XOTclCallStackContent *h = cs->top;

    assert(cs->top > cs->content);
    csc = cs->top;

    /*fprintf(stderr, "POP frame=%p\n", csc);*/

    if (csc->destroyedCmd != 0) {
        int destroy = 1;
        TclCleanupCommand((Command *)csc->destroyedCmd);
        MEM_COUNT_FREE("command refCount", csc->destroyedCmd);
        /* do not physically destroy, when callstack still contains "self"
           entries of the object */
        while (--h > cs->content) {
            if (h->self == csc->self) {
                destroy = 0;
                break;
            }
        }
        if (destroy) {
            CallStackDoDestroy(interp, csc->self);
        }
    }

    cs->top--;
    MEM_COUNT_FREE("CallStack", NULL);
}



XOTCLINLINE static XOTclCallStackContent*
CallStackGetTopFrame(Tcl_Interp *interp) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    return cs->top;
}

static XOTclCallStackContent*
CallStackGetFrame(Tcl_Interp *interp) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    register XOTclCallStackContent *top = cs->top;
    Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

    if (Tcl_Interp_framePtr(interp) != varFramePtr && top->currentFramePtr) {
        XOTclCallStackContent *bot = cs->content + 1;

        /* we are in a uplevel */
        while (varFramePtr != top->currentFramePtr && top>bot) {
            top--;
        }
    }
    return top;
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
        cmdList = cmdList->nextPtr;
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
    register XOTclCmdList *c = *cmdList, *del = 0;
    if (c == 0)
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
    XOTclCmdList *c, *del = 0;
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
    register XOTclCmdList *h = l;
    while (h != 0) {
        if (h->cmdPtr == cmd)
            return h;
        h = h->nextPtr;
    }
    return 0;
}

/*
 * simple list search proc to search a list of cmds
 * for a simple Name
 */
static XOTclCmdList*
CmdListFindNameInList(Tcl_Interp *interp, char *name, XOTclCmdList *l) {
    register XOTclCmdList *h = l;
    while (h != 0) {
        CONST84 char *cmdName = Tcl_GetCommandName(interp, h->cmdPtr);
        if (cmdName[0] == name[0] && !strcmp(cmdName, name))
            return h;
        h = h->nextPtr;
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
    for (; alist!=NULL; alist = alist->nextPtr) {
        Tcl_AppendStringsToObj(newAssStr, "{", ObjStr(alist->content),
                               "}", (char *) NULL);
        if (alist->nextPtr != NULL)
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
    hPtr = Tcl_FindHashEntry(&aStore->procs, name);
    if (hPtr == NULL) return NULL;
    return (XOTclProcAssertion*) Tcl_GetHashValue(hPtr);
}

static void
AssertionRemoveProc(XOTclAssertionStore *aStore, char *name) {
    Tcl_HashEntry *hPtr;
    if (aStore) {
        hPtr = Tcl_FindHashEntry(&aStore->procs, name);
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
        hPtr = Tcl_FirstHashEntry(&aStore->procs, &hSrch);
        while (hPtr) {
            /*
             * AssertionRemoveProc calls Tcl_DeleteHashEntry(hPtr), thus
             * we get the FirstHashEntry afterwards again to proceed
             */
            AssertionRemoveProc(aStore, Tcl_GetHashKey(&aStore->procs, hPtr));
            hPtr = Tcl_FirstHashEntry(&aStore->procs, &hSrch);
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
        result = Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp),&success);
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

    while (alist != NULL) {
        /* Eval instead of IfObjCmd => the substitutions in the
           conditions will be done by Tcl */
        char *assStr = ObjStr(alist->content), *c = assStr;
        int comment = 0;

        while (c != 0 && *c != '\0') {
            if (*c == '#') {
                comment = 1; break;
            }
            c++;
        }
        if (!comment) {
            XOTcl_FrameDecls;
            XOTcl_PushFrame(interp, obj);
            CallStackPush(interp, obj, 0, 0, 0, 0, XOTCL_CSC_TYPE_PLAIN);

            /* don't check assertion during assertion check */
            savedCheckoptions = obj->opt->checkoptions;
            obj->opt->checkoptions = CHECK_NONE;

            /*fprintf(stderr, "Checking Assertion %s ", assStr);*/

            /*
             * now check the assertion in the pushed callframe's scope
             */
            acResult = checkConditionInScope(interp, alist->content);
            if (acResult != TCL_OK)
                checkFailed = alist;

            obj->opt->checkoptions = savedCheckoptions;

            /*fprintf(stderr, "...%s\n", checkFailed ? "failed" : "ok");*/

            CallStackPop(interp);
            XOTcl_PopFrame(interp, obj);
        }
        if (checkFailed)
            break;
        alist = alist->nextPtr;
    }

    if (checkFailed != NULL) {
        DECR_REF_COUNT(savedObjResult);
        if (acResult == TCL_ERROR) {
            Tcl_Obj *sr = Tcl_GetObjResult(interp);
            INCR_REF_COUNT(sr);	
            XOTclVarErrMsg(interp, "Error in Assertion: {",
                           ObjStr(checkFailed->content), "} in proc '",
                           GetSelfProc(interp), "'\n\n", ObjStr(sr), (char *) NULL);
            DECR_REF_COUNT(sr);
            return TCL_ERROR;
        }

        return XOTclVarErrMsg(interp, "Assertion failed check: {",
                              ObjStr(checkFailed->content), "} in proc '",
                              GetSelfProc(interp), "'", (char *) NULL);
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
        while (clPtr != 0 && result != TCL_ERROR) {
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
    h->currentCmdPtr = 0;
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
 * Appends XOTclClasses* containing the mixin classes and their
 * superclasses to 'mixinClasses' list from a given mixinList
 */
static void
MixinComputeOrderFullList(Tcl_Interp *interp, XOTclCmdList **mixinList,
			  XOTclClasses **mixinClasses,
			  XOTclClasses **checkList, int level) {
    XOTclCmdList *m;
    XOTclClasses *pl, **clPtr = mixinClasses;

    CmdListRemoveEpoched(mixinList, GuardDel);
    m = *mixinList;

    while (m) {
        XOTclClass *mCl = XOTclGetClassFromCmdPtr(m->cmdPtr);
        if (mCl) {
            for (pl = ComputeOrder(mCl, mCl->order, Super); pl; pl = pl->nextPtr) {
                /*fprintf(stderr, " %s, ", ObjStr(pl->cl->object.cmdName));*/
                if (!(pl->cl == RUNTIME_STATE(interp)->theObject)) {
                    XOTclClassOpt *opt = pl->cl->opt;
                    if (opt && opt->instmixins != 0) {
                        /* compute transitively the instmixin classes of this added
                           class */
                        XOTclClasses *cls;
                        int i, found=0;
                        for (i=0, cls = *checkList; cls; i++, cls = cls->nextPtr) {
                            /* fprintf(stderr,"+++ c%d: %s\n", i,
                               ObjStr(cls->cl->object.cmdName));*/
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
        m = m->nextPtr;
    }
    if (level == 0 && *checkList) {
        XOTclClassListFree(*checkList);
        *checkList = NULL;
    }
}

static void
MixinResetOrder(XOTclObject *obj) {
    /*fprintf(stderr,"removeList %s \n", ObjStr(obj->cmdName));*/
    CmdListRemoveList(&obj->mixinOrder, NULL /*GuardDel*/);
    obj->mixinOrder = 0;
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
    XOTclClasses *fullList, *checkList=0, *mixinClasses = 0, *nextCl, *pl,
        *checker, *guardChecker;

    if (obj->mixinOrder)  MixinResetOrder(obj);
    /*fprintf(stderr, "Mixin Order:\n First List: ");*/

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

        if (checker == 0) {
            /* check obj->cl hierachy */
            for (checker = ComputeOrder(obj->cl, obj->cl->order, Super); checker; checker = checker->nextPtr) {
                if (checker->cl == mixinClasses->cl)
                    break;
            }
            /* if checker is set, it was found in the class hierarchy
               and it is ignored */
        }
        if (checker == 0) {
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

    if (GetXOTclClassFromObj(interp, name, &mixin, base) != TCL_OK)
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
 * get all instances of a class recursively into an initialized
 * String key hashtable
 */
static void
getAllInstances(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclClass *startCl, int onlyWithMixins) {
    Tcl_HashTable *table = &startCl->instances;
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    int collect = 1;

    for (hPtr = Tcl_FirstHashEntry(table, &search);  hPtr != NULL;
         hPtr = Tcl_NextHashEntry(&search)) {
        XOTclObject *inst = (XOTclObject*) Tcl_GetHashKey(table, hPtr);
        Tcl_HashEntry *hPtrDest;
        int new = 0;

        if (onlyWithMixins) {
            if (! (collect = XOTclObjectIsClass(inst))) {
                if (!(inst->flags & XOTCL_MIXIN_ORDER_VALID))
                    MixinComputeDefined(interp, inst);
                if ((inst->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID)) {
                    collect = (inst->mixinOrder != NULL);
                }
            }
        }
        if (collect) {
            hPtrDest = Tcl_CreateHashEntry(destTable, ObjStr(inst->cmdName), &new);
      
            /*fprintf (stderr, " -- %s (%s) opt %d instopt %d\n", 
              ObjStr(inst->cmdName), ObjStr(startCl->object.cmdName),
              inst->opt != NULL, (XOTclObjectIsClass(inst) && ((XOTclClass*)inst)->opt));*/
        }
    
        if (new) {
            if (XOTclObjectIsClass(inst)) {
                XOTclClasses *sl = ((XOTclClass*)inst)->sub, *sc;
                getAllInstances(interp, destTable, (XOTclClass*) inst, onlyWithMixins);
                for (sc = sl; sc != 0; sc = sc->nextPtr)
                    getAllInstances(interp, destTable, sc->cl, onlyWithMixins);
            }
        }
    }
}

/*
 * recursively get all mixinofs of a class into an initialized
 * String key hashtable
 */
 
static void
getAllMixinofs(Tcl_Interp *interp, Tcl_HashTable *destTable, XOTclClass *startCl) {
  
    if (startCl->opt) {
        XOTclClass *cl;
        XOTclCmdList *m;
        int new;
    
        for (m = startCl->opt->mixinofs; m; m = m->nextPtr) {
            Tcl_CreateHashEntry(destTable, Tcl_GetCommandName(interp, m->cmdPtr), &new);
            /*if (new) fprintf (stderr, " -- %s (%s)\n", Tcl_GetCommandName(in, m->cmdPtr), ObjStr(startCl->object.cmdName));*/
            cl = XOTclGetClassFromCmdPtr(m->cmdPtr);
            if (cl) {
                getAllMixinofs(interp, destTable, cl);
            }
        }
    }
}

static void
RemoveFromInstmixinsofs(Tcl_Command cmd, XOTclCmdList *cmdlist) {
    for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
        XOTclClass *ncl = XOTclGetClassFromCmdPtr(cmdlist->cmdPtr);
        XOTclClassOpt *nclopt = ncl ? ncl->opt : NULL;
        if (nclopt) {
            XOTclCmdList *del = CmdListFindCmdInList(cmd, nclopt->instmixinofs);
            if (del) {
                /* fprintf(stderr,"Removing class %s from instmixinofs of class %s\n",
                   ObjStr(cl->object.cmdName), ObjStr(XOTclGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
                del = CmdListRemoveFromList(&nclopt->instmixinofs, del);
                CmdListDeleteCmdListEntry(del, GuardDel);
            }
        }
    }
}

static void 
RemoveFromMixinofs(Tcl_Command cmd, XOTclCmdList *cmdlist) {
    for ( ; cmdlist; cmdlist = cmdlist->nextPtr) {
        XOTclClass *cl = XOTclGetClassFromCmdPtr(cmdlist->cmdPtr);
        XOTclClassOpt *clopt = cl ? cl->opt : NULL;
        if (clopt) {
            XOTclCmdList *del = CmdListFindCmdInList(cmd, clopt->mixinofs);
            if (del) {
                /* fprintf(stderr,"Removing object %s from mixinofs of Class %s\n",
                   ObjStr(obj->cmdName), ObjStr(XOTclGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
                del = CmdListRemoveFromList(&clopt->mixinofs, del);
                CmdListDeleteCmdListEntry(del, GuardDel);
            }
        } /* else fprintf(stderr,"CleanupDestroyObject %s: NULL pointer in mixins!\n", ObjStr(obj->cmdName)); */
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
                   ObjStr(cl->object.cmdName), ObjStr(XOTclGetObjectFromCmdPtr(cmdlist->cmdPtr)->cmdName)); */
                del = CmdListRemoveFromList(&clopt->instmixins, del);
                CmdListDeleteCmdListEntry(del, GuardDel);
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
                   ObjStr(cl->object.cmdName), ObjStr(XOTclGetObjectFromCmdPtr(cmdlist->cmdPtr)->cmdName)); */
                del = CmdListRemoveFromList(&objopt->mixins, del);
                CmdListDeleteCmdListEntry(del, GuardDel);
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
    XOTclObject *obj;

    cl->order = 0;  
  
    for (clPtr = ComputeOrder(cl, cl->order, Sub); clPtr; clPtr = clPtr->nextPtr) {
        Tcl_HashSearch hSrch;
        Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
            Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : 0;
    
        /*fprintf(stderr,"invalidating instances of class %s\n",
          ObjStr(clPtr->cl->object.cmdName));*/

        /* here we should check, whether this class is used as
           a mixin / instmixin somewhere else and invalidate
           the objects of these as well -- */

        for (; hPtr != 0; hPtr = Tcl_NextHashEntry(&hSrch)) {
            XOTclObject *obj = (XOTclObject*)
                Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
            MixinResetOrder(obj);
            obj->flags &= ~XOTCL_MIXIN_ORDER_VALID;
        }
    }

    XOTclClassListFree(cl->order);
    cl->order = saved;
#if 1
    /*fprintf(stderr,"invalidate order brute force %d\n",
      (RUNTIME_STATE(interp)->exitHandlerDestroyRound == XOTCL_EXITHANDLER_OFF));*/

    if (RUNTIME_STATE(interp)->exitHandlerDestroyRound == XOTCL_EXITHANDLER_OFF) {
        /* invalidate the mixins on all instances that have this mixin (cl)
           at the moment
        */
        Tcl_InitHashTable(commandTable, TCL_STRING_KEYS);
        MEM_COUNT_ALLOC("Tcl_InitHashTable", commandTable);

        /*getAllInstances(interp, commandTable, RUNTIME_STATE(interp)->theClass, 0);*/
        getAllMixinofs(interp, commandTable, cl);
        hPtr = Tcl_FirstHashEntry(commandTable, &hSrch);
        while (hPtr) {
            char *key = Tcl_GetHashKey(commandTable, hPtr);
    
            obj = XOTclpGetObject(interp, key);
            /*fprintf(stderr,"invalidate order brute force check %s\n", key);*/
            if (obj && !XOTclObjectIsClass(obj)
                && !(obj->flags & XOTCL_DESTROY_CALLED)
                && (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID)) {
                XOTclCmdList *ml;
                for (ml = obj->mixinOrder; ml; ml = ml->nextPtr) {
                    XOTclClass *mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
                    if (mixin == cl) {
                        /*fprintf(stderr,"invalidate order brute force reset %s\n", ObjStr(obj->cmdName));*/
                        MixinResetOrder(obj);
                        obj->flags &= ~XOTCL_MIXIN_ORDER_VALID;
                        break;
                    }
                }
            }
            hPtr = Tcl_NextHashEntry(&hSrch);
        }
        MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
        Tcl_DeleteHashTable(commandTable);
    }
#endif
}

static int 
MixinInfo(Tcl_Interp *interp, XOTclCmdList *m, char *pattern, int withGuards);
/*
 * the mixin order is either
 *   DEFINED (there are mixins on the instance),
 *   NONE    (there are no mixins for the instance),
 *   or INVALID (a class re-strucuturing has occured, thus it is not clear
 *               whether mixins are defined or not).
 * If it is INVALID MixinComputeDefined can be used to compute the order
 * and set the instance to DEFINE or NONE
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
 * walk through the mixin order until the current mixin is reached.
 * then use the next mixin as current mixin.
 *
 * precondition: obj->mixinStack is not NULL
 */
static void
MixinSeekCurrent(Tcl_Interp *interp, XOTclObject *obj, XOTclCmdList **cmdList) {
    Tcl_Command currentCmdPtr;

    /* ensure that the mixin order is not invalid, otherwise compute order */
    assert(obj->flags & XOTCL_MIXIN_ORDER_VALID);
    /*MixinComputeDefined(interp, obj);*/
    currentCmdPtr = obj->mixinStack->currentCmdPtr;

    /*
      {
      XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
      XOTclCallStackContent *csc = cs->top;
      fprintf(stderr, "%p == %p ==> %d \n", csc->cl, currentCmdPtr, 
      csc->cmdPtr == currentCmdPtr);
      }
    */

    /***
        { Tcl_Obj *sr;

        MixinInfo(interp, obj->mixinOrder, NULL, 0);
        sr = Tcl_GetObjResult(interp);
        fprintf(stderr,"INFO->%s order %p next %p\n", ObjStr(sr), obj->mixinOrder, obj->mixinOrder->nextPtr);
        }
    ***/

    *cmdList = obj->mixinOrder;
    /*  
        fprintf(stderr, "->1 mixin seek current = %p next = %p %s\n",
        currentCmdPtr,
        (*cmdList)->nextPtr,
        (*cmdList)->nextPtr ? Tcl_GetCommandName(interp, (*cmdList)->nextPtr->cmdPtr) : "");
    */
  
#if defined(ACTIVEMIXIN)
    /*RUNTIME_STATE(interp)->cmdPtr = (*cmdList)->nextPtr ? (*cmdList)->nextPtr->cmdPtr : NULL;*/
    RUNTIME_STATE(interp)->cmdPtr = (*cmdList)->cmdPtr;
#endif

    /* go forward to current class */
    while (*cmdList && currentCmdPtr) {
        /* fprintf(stderr, "->2 mixin seek current = %p next = %p\n", currentCmdPtr, (*cmdList)->nextPtr);*/
        if ((*cmdList)->cmdPtr == currentCmdPtr)
            currentCmdPtr = 0;
        *cmdList = (*cmdList)->nextPtr;

#if defined(ACTIVEMIXIN)
        /*RUNTIME_STATE(interp)->cmdPtr = (*cmdList)->nextPtr ? (*cmdList)->nextPtr->cmdPtr : NULL;*/
        RUNTIME_STATE(interp)->cmdPtr = (*cmdList)->cmdPtr;
#endif
    }
}

/*
 * before we can perform a mixin dispatch, MixinSearchProc seeks the
 * current mixin and the relevant calling information
 */
static Tcl_Command
MixinSearchProc(Tcl_Interp *interp, XOTclObject *obj, char *methodName,
		XOTclClass **cl, Tcl_ObjCmdProc **proc, ClientData *cp,
		Tcl_Command *currentCmdPtr) {
    Tcl_Command cmd = NULL;
    XOTclCmdList *cmdList;
    XOTclClass *cls;

    assert(obj);
    assert(obj->mixinStack);

    MixinSeekCurrent(interp, obj, &cmdList);

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
	      ObjStr(obj->cmdName), methodName, cmdList,
	      cmdList->cmdPtr, cmdList->clientData);
            */
            if (cls) {
                int guardOk = TCL_OK;
                cmd = FindMethod(methodName, cls->nsPtr);
                if (cmd && cmdList->clientData) {
                    if (!RUNTIME_STATE(interp)->cs.guardCount) {
                        guardOk = GuardCall(obj, cls, (Tcl_Command) cmd, interp, 
                                            (Tcl_Obj*)cmdList->clientData, 1);
                    }
                }
                if (cmd && guardOk == TCL_OK) {
                    /*
                     * on success: compute mixin call data
                     */
                    *cl = cls;
                    *proc = Tcl_Command_objProc(cmd);
                    *cp   = Tcl_Command_objClientData(cmd);
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
MixinInfo(Tcl_Interp *interp, XOTclCmdList *m, char *pattern, int withGuards) {
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);
    XOTclClass *mixinClass;
    while (m) {
        /* fprintf(stderr,"   mixin info m=%p, next=%p\n", m, m->nextPtr); */
        mixinClass = XOTclGetClassFromCmdPtr(m->cmdPtr);
        if (mixinClass &&
            (!pattern ||
             Tcl_StringMatch(ObjStr(mixinClass->object.cmdName), pattern))) {
            if (withGuards && m->clientData) {
                Tcl_Obj *l = Tcl_NewListObj(0, NULL);
                Tcl_Obj *g = (Tcl_Obj *) m->clientData;
                Tcl_ListObjAppendElement(interp, l, mixinClass->object.cmdName);
                Tcl_ListObjAppendElement(interp, l, XOTclGlobalObjects[XOTE_GUARD_OPTION]);
                Tcl_ListObjAppendElement(interp, l, g);
                Tcl_ListObjAppendElement(interp, list, l);
            } else
                Tcl_ListObjAppendElement(interp, list, mixinClass->object.cmdName);
        }
        m = m->nextPtr;
    }
    Tcl_SetObjResult(interp, list);
    return TCL_OK;
}

/*
 * info option for mixinofs and instmixinofs
 */
  
static int
MixinOfInfo(Tcl_Interp *interp, XOTclCmdList *m, char *pattern) {
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);
    XOTclObject *mixinObject;
    for ( ; m; m = m->nextPtr) {
        /* fprintf(stderr,"   mixinof info m=%p, next=%p\n", m, m->nextPtr); */
        mixinObject = XOTclGetObjectFromCmdPtr(m->cmdPtr);
        if (mixinObject &&
            (!pattern ||
             Tcl_StringMatch(ObjStr(mixinObject->cmdName), pattern))) {
            Tcl_ListObjAppendElement(interp, list, mixinObject->cmdName);
        }
    }
    Tcl_SetObjResult(interp, list);
    return TCL_OK;
}

static Tcl_Command
MixinSearchMethodByName(Tcl_Interp *interp, XOTclCmdList *mixinList, char *name, XOTclClass **cl) {
    Tcl_Command cmd;
    while (mixinList) {
        XOTclClass *foundCl, *mcl =
            XOTclpGetClass(interp, (char *) Tcl_GetCommandName(interp, mixinList->cmdPtr));
        if (mcl && (foundCl = SearchCMethod(mcl, name, &cmd))) {
            if (cl) *cl = foundCl;
            return cmd;
        }

        mixinList = mixinList->nextPtr;
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
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;

    if (guard) {
        /*
         * if there are more than one filter guard for this filter
         * (i.e. they are inherited), then they are OR combined
         * -> if one check succeeds => return 1
         */

        /*fprintf(stderr, "checking guard **%s**\n", ObjStr(guard));*/

        cs->guardCount++;
        rc = checkConditionInScope(interp, guard);
        cs->guardCount--;

        /*fprintf(stderr, "checking guard **%s** returned rc=%d\n",
          ObjStr(fr->content), rc);*/

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

static int
GuardCall(XOTclObject *obj, XOTclClass *cl, Tcl_Command cmd,
	  Tcl_Interp *interp, Tcl_Obj *guard, int push) {
    int rc = TCL_OK;

    if (guard) {
        Tcl_Obj *res = Tcl_GetObjResult(interp); /* save the result */
        INCR_REF_COUNT(res);

        /* GuardPrint(interp, cmdList->clientData); */
        /*
         * ok, there is a guard ... we have to push a
         * fake callframe on the tcl stack so that uplevel
         * is in sync with the XOTcl callstack, and we can uplevel
         * into the above pushed CallStack entry
         */
        if (push) {
            CallStackPush(interp, obj, cl, cmd, 0, 0, XOTCL_CSC_TYPE_GUARD);
            rc = GuardCheck(interp, guard);
            CallStackPop(interp);
        } else {
            rc = GuardCheck(interp, guard);
        }
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
    if (interceptorDefList != 0) {
        h = CmdListFindCmdInList(interceptorCmd, interceptorDefList);
        if (h) {
            GuardAdd(interp, dest, (Tcl_Obj *) h->clientData);
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
        XOTclCmdList *ml = obj->mixinOrder;
        XOTclClass *mixin;
        while (ml && ! guardAdded) {
            mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
            if (mixin && mixin->opt) {
                guardAdded = GuardAddFromDefinitionList(interp, dest, obj, filterCmd,
                                                        mixin->opt->instfilters);
            }
            ml = ml->nextPtr;
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
                GuardAdd(interp, dest, (Tcl_Obj *) registeredFilter->clientData);
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
                Tcl_Obj *g = (Tcl_Obj *) h->clientData;
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
                                  ObjStr(startingObj->cmdName), " - proc: ",
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
    obj->filterOrder = 0;
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
    cmdList = *filters;
    while (cmdList) {
        simpleName = (char *) Tcl_GetCommandName(interp, cmdList->cmdPtr);
        cmd = FilterSearch(interp, simpleName, startingObj, startingCl, &cl);
        if (cmd == NULL) {
            del = cmdList;
            cmdList = cmdList->nextPtr;
            del = CmdListRemoveFromList(filters, del);
            CmdListDeleteCmdListEntry(del, GuardDel);
        } else {
            if (cmd != cmdList->cmdPtr)
                CmdListReplaceCmd(cmdList, cmd, cl);
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

    cl->order = 0;
    savePtr = clPtr = ComputeOrder(cl, cl->order, Sub);
    cl->order = saved;
  
    while (clPtr != 0) {
        Tcl_HashSearch hSrch;
        Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
            Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : 0;

        /* recalculate the commands of all instfilter registrations */
        if (clPtr->cl->opt) {
            FilterSearchAgain(interp, &clPtr->cl->opt->instfilters, 0, clPtr->cl);
        }
        for (; hPtr != 0; hPtr = Tcl_NextHashEntry(&hSrch)) {
            XOTclObject *obj = (XOTclObject *)Tcl_GetHashKey(&clPtr->cl->instances, hPtr);
            FilterResetOrder(obj);
            obj->flags &= ~XOTCL_FILTER_ORDER_VALID;

            /* recalculate the commands of all object filter registrations */
            if (obj->opt) {
                FilterSearchAgain(interp, &obj->opt->filters, obj, 0);
            }
        }
        clPtr = clPtr->nextPtr;
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
    cl->order = 0;

    /*fprintf(stderr, "FilterRemoveDependentFilterCmds cl %p %s, removeClass %p %s\n",
      cl, ObjStr(cl->object.cmdName),
      removeClass, ObjStr(removeClass->object.cmdName));*/

    for (clPtr = ComputeOrder(cl, cl->order, Sub); clPtr; clPtr = clPtr->nextPtr) {
        Tcl_HashSearch hSrch;
        Tcl_HashEntry *hPtr = &clPtr->cl->instances ?
            Tcl_FirstHashEntry(&clPtr->cl->instances, &hSrch) : 0;
        XOTclClassOpt *opt = clPtr->cl->opt;
        if (opt) {
            CmdListRemoveContextClassFromList(&opt->instfilters, removeClass, GuardDel);
        }
        for (; hPtr != 0; hPtr = Tcl_NextHashEntry(&hSrch)) {
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
getFullProcQualifier(Tcl_Interp *interp, CONST84 char *cmdName,
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
    CONST84 char *simpleName;
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
                Tcl_Obj *g = (Tcl_Obj *) f->clientData;
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
 * Appends XOTclCmdPtr* containing the filter cmds and their
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
    XOTclCmdList *filterList = 0, *next, *checker, *newlist;
    XOTclClasses *pl;

    if (obj->filterOrder) FilterResetOrder(obj);
    /*
      fprintf(stderr, "<Filter Order obj=%s> List: ", ObjStr(obj->cmdName));
    */

    /* append instfilters registered for mixins */
    if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
        MixinComputeDefined(interp, obj);

    if (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
        XOTclCmdList *ml = obj->mixinOrder;
        XOTclClass *mixin;
        while (ml) {
            mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
            if (mixin && mixin->opt && mixin->opt->instfilters)
                FilterComputeOrderFullList(interp, &mixin->opt->instfilters, &filterList);
            ml = ml->nextPtr;
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
        if (checker == 0) {
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

    h->currentCmdPtr = 0;
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
 * walk through the filter order until the current filter is reached.
 * then use the next filter as current filter.
 *
 * precondition: obj->filterStack is not NULL
 */
static void
FilterSeekCurrent(Tcl_Interp *interp, XOTclObject *obj, XOTclCmdList **cmdList) {
    Tcl_Command currentCmd = obj->filterStack->currentCmdPtr;

    assert(obj->flags & XOTCL_FILTER_ORDER_VALID);
    /* ensure that the filter order is not invalid, otherwise compute order
       FilterComputeDefined(interp, obj);
    */

    *cmdList = obj->filterOrder;

    /* go forward to current class */
    while (*cmdList && currentCmd) {
        if ((*cmdList)->cmdPtr == currentCmd)
            currentCmd = 0;
        *cmdList = (*cmdList)->nextPtr;
    }
}

/*
 * seek through the filters active for "obj" and check whether cmdPtr
 * is among them
 */
XOTCLINLINE static int
FilterActiveOnObj(Tcl_Interp *interp, XOTclObject *obj, Tcl_Command cmd) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    XOTclCallStackContent *bot = cs->content;
    register XOTclCallStackContent *csc = cs->top;
    while (csc > bot) {
        /* only check the callstack entries for this object &&
           only check the callstack entries for the given cmd */
        if (obj == csc->self && cmd == csc->cmdPtr &&
            csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
            return 1;
        }
        csc--;
    }
    return 0;
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
FilterSearchProc(Tcl_Interp *interp, XOTclObject *obj, Tcl_ObjCmdProc **proc, ClientData *cp,
		 Tcl_Command *currentCmd, XOTclClass **cl) {
    XOTclCmdList *cmdList;

    assert(obj);
    assert(obj->filterStack);

    *currentCmd = 0;
    *proc = 0;
    *cp = 0;

    FilterSeekCurrent(interp, obj, &cmdList);
    while (cmdList) {
        if (Tcl_Command_cmdEpoch(cmdList->cmdPtr)) {
            cmdList = cmdList->nextPtr;
        } else if (FilterActiveOnObj(interp, obj, cmdList->cmdPtr)) {
            /* fprintf(stderr, "Filter <%s> -- Active on: %s\n",
               Tcl_GetCommandName(interp, (Tcl_Command)cmdList->cmdPtr), ObjStr(obj->cmdName));
            */
            obj->filterStack->currentCmdPtr = cmdList->cmdPtr;
            FilterSeekCurrent(interp, obj, &cmdList);
        } else {
            /* ok. we found it */
            *proc = Tcl_Command_objProc(cmdList->cmdPtr);
            *cp   = Tcl_Command_objClientData(cmdList->cmdPtr);
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
    return 0;
}


static int
SuperclassAdd(Tcl_Interp *interp, XOTclClass *cl, int oc, Tcl_Obj **ov, Tcl_Obj *arg, XOTclClass *base) {
    XOTclClasses *filterCheck, *osl = 0;
    XOTclClass **scl = 0;
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
    while (filterCheck) {
        FilterRemoveDependentFilterCmds(cl, filterCheck->cl);
        filterCheck = filterCheck->nextPtr;
    }

    /* invalidate all interceptors orders of instances of this
       and of all depended classes */
    MixinInvalidateObjOrders(interp, cl);
    FilterInvalidateObjOrders(interp, cl);

    scl = NEW_ARRAY(XOTclClass*, oc);
    for (i = 0; i < oc; i++) {
        if (GetXOTclClassFromObj(interp, ov[i], &scl[i], base) != TCL_OK) {
            FREE(XOTclClass**, scl);
            return XOTclErrBadVal(interp, "superclass", "a list of classes",
                                  ObjStr(arg));
        }
    }

    /*
     * check that superclasses don't precede their classes
     */

    for (i = 0; i < oc; i++) {
        if (reversed != 0) break;
        for (j = i+1; j < oc; j++) {
            XOTclClasses *dl = ComputeOrder(scl[j], scl[j]->order, Super);
            if (reversed != 0) break;
            while (dl != 0) {
                if (dl->cl == scl[i]) break;
                dl = dl->nextPtr;
            }
            if (dl != 0) reversed = 1;
        }
    }

    if (reversed != 0) {
        return XOTclErrBadVal(interp, "superclass", "classes in dependence order", 
                              ObjStr(arg));
    }

    while (cl->super != 0) {

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
    for (i = 0; i < oc; i++)
        AddSuper(cl, scl[i]);
    FREE(XOTclClass**, scl);
    FlushPrecedences(cl);

    if (!ComputeOrder(cl, cl->order, Super)) {

        /*
         * cycle in the superclass graph, backtrack
         */

        XOTclClasses *l;
        while (cl->super != 0) (void)RemoveSuper(cl, cl->super->cl);
        for (l = osl; l != 0; l = l->nextPtr) AddSuper(cl, l->cl);
        XOTclClassListFree(osl);
        return XOTclErrBadVal(interp, "superclass", "a cycle-free graph", ObjStr(arg));
    }
    XOTclClassListFree(osl);

    /* if there are no more super classes add the Object
       class as superclasses */
    if (cl->super == 0)
        AddSuper(cl, RUNTIME_STATE(interp)->theObject);

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
    int flags;
  
    flags = (index == NULL) ? TCL_PARSE_PART1 : 0;
  
    if (obj->nsPtr) {
        Tcl_SetNamespaceResolvers(obj->nsPtr, (Tcl_ResolveCmdProc*)NULL,
                                  varResolver, (Tcl_ResolveCompiledVarProc*)NULL);
    }

    XOTcl_PushFrame(interp, obj);

#if defined(PRE83)
    varPtr = TclLookupVar(interp, varName, index, flags, "access",
                          /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
#else
    if (triggerTrace) 
        varPtr = TclVarTraceExists(interp, varName);
    else 
        varPtr = TclLookupVar(interp, varName, index, flags, "access",
                              /*createPart1*/ 0, /*createPart2*/ 0, &arrayPtr);
#endif
    result = ((varPtr != NULL) && 
              (!requireDefined || !TclIsVarUndefined(varPtr)));

    XOTcl_PopFrame(interp, obj);

    if (obj->nsPtr) {
        Tcl_SetNamespaceResolvers(obj->nsPtr, (Tcl_ResolveCmdProc*)NULL,
                                  (Tcl_ResolveVarProc *)NULL,
                                  (Tcl_ResolveCompiledVarProc*)NULL);
    }
    return result;
}

static int
evalValueIfNeeded(Tcl_Interp *interp, XOTclObject *obj, CONST char *varName, Tcl_Obj **newValue) {
    int rc = TCL_OK;
    int doSubst = 0;
    char *value = ObjStr(*newValue), *v;
    /*fprintf(stderr,"+++++ %s.%s got '%s''\n", ObjStr(obj->cmdName), varName, ObjStr(newValue));*/

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
        Tcl_Obj *ov[2];
        ov[1] = *newValue;
        Tcl_ResetResult(interp);

        CallStackPush(interp, obj, NULL, 0, 0, 0, XOTCL_CSC_TYPE_PLAIN);
        rc = XOTcl_SubstObjCmd(NULL, interp, 2, ov);
        CallStackPop(interp);

        /*fprintf(stderr,"+++++ %s.%s subst returned %d OK %d\n", 
          ObjStr(obj->cmdName), varName, rc, TCL_OK);*/

        if (rc == TCL_OK) {
            *newValue = Tcl_GetObjResult(interp);
        }
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
        /*fprintf(stderr,"+++++ %s.%s undefined'\n", ObjStr(obj->cmdName), varName);*/
        if (newValue) {
            rc = evalValueIfNeeded(interp, obj, varName, &newValue);
            if (rc != TCL_OK) {
                goto leavesetdefaultvalue;
            }

            /*
             * just set the variable, checking is happening later 
             */
            /*fprintf(stderr,"+++++ %s.%s := '%s'\n", ObjStr(obj->cmdName), varName, ObjStr(newValue));*/

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
                /*fprintf(stderr, "----- we have an initcmd %s\n", cmd);*/
                if (*cmd) {
                    CallStackPush(interp, obj, NULL, 0, 0,
                                  0, XOTCL_CSC_TYPE_PLAIN); /*allow to call self*/
  
                    /*fprintf(stderr,"!!!! evaluating '%s'\n", cmd); */
                    rc = Tcl_EvalObjEx(interp, initCmd, TCL_EVAL_DIRECT);
                    CallStackPop(interp);

                    if (rc != TCL_OK) {
                        goto  leavesetdefaultvalue;
                    }
                }
            }
        }
    } else {
        /* fprintf(stderr, "+++ value for %s.%s already set\n", ObjStr(obj->cmdName), varName);*/
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
            /*fprintf(stderr,"+++++ %s.%s must check'\n", ObjStr(obj->cmdName), varName);*/
            if (!varExists(interp, obj, varName, NULL, 0, 1)) {
                return XOTclVarErrMsg(interp,
                                      "required parameter '", varName, "' missing",
                                      (char *) NULL);
            }
        }
    }
    return rc;
}

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

/*
 * method dispatch
 */

/* actually call a method (with assertion checking) */
static int
callProcCheck(ClientData cp, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
	      Tcl_Command cmd, XOTclObject *obj, XOTclClass *cl, 
              char *methodName, int frameType, int isTclProc) {
    int result = TCL_OK;
    XOTclRuntimeState *rst = RUNTIME_STATE(interp);
    CheckOptions co;

#if defined(PROFILE)
    long int startUsec, startSec;
    struct timeval trt;

    gettimeofday(&trt, NULL);
    startSec  = trt.tv_sec;
    startUsec = trt.tv_usec;
#endif
    assert(obj);

    rst->callIsDestroy = 0;
    /*fprintf(stderr,"callProcCheck: setting callIsDestroy = 0, m=%s obj=%p (%s)\n",
      methodName, obj, ObjStr(obj->cmdName));*/
  
    /*fprintf(stderr,"*** callProcCheck: cmd = %p objproc = %p\n", cmd, Tcl_Command_objProc(cmd));
      fprintf(stderr,
      "cp=%p, isTclProc=%d %p %s, dispatch=%d %p, forward=%d %p, scoped %p, ov[0]=%p oc=%d\n",
      cp,
      isTclProc, cmd,
      Tcl_GetCommandName(interp, cmd),
      Tcl_Command_objProc(cmd) == XOTclObjDispatch, XOTclObjDispatch,
      Tcl_Command_objProc(cmd) == XOTclForwardMethod, XOTclForwardMethod,
      XOTclObjscopedMethod,
      objv[0], objc
      );
      {int i; fprintf(stderr, "\tCALL ");for(i=0; i<objc; i++) {fprintf(stderr,"%s ", ObjStr(objv[i]));} fprintf(stderr,"\n");}
    */  

#ifdef CALLSTACK_TRACE
    XOTclCallStackDump(interp);
#endif

    if (!isTclProc && obj->teardown) {
        co = 0;
        if (obj->opt) co = obj->opt->checkoptions;
        if ((co & CHECK_INVAR) &&
            ((result = AssertionCheckInvars(interp, obj, methodName, co)) == TCL_ERROR)) {
            goto finish;
        }

#ifdef DISPATCH_TRACE
        printCall(interp,"callProcCheck cmd", objc, objv);
        /*fprintf(stderr,"\tproc=%s\n", Tcl_GetCommandName(interp, cmd));*/
#endif
        result = (*Tcl_Command_objProc(cmd))(cp, interp, objc, objv);
    
#ifdef DISPATCH_TRACE
        printExit(interp,"callProcCheck cmd", objc, objv, result);
        /*fprintf(stderr, " returnCode %d xotcl rc %d\n",
          Tcl_Interp_returnCode(interp), rst->returnCode);*/
#endif

        /*
          if (obj && obj->teardown && cl && !(obj->flags & XOTCL_DESTROY_CALLED)) {
          fprintf(stderr, "Obj= %s ", ObjStr(obj->cmdName));
          fprintf(stderr, "CL= %s ", ObjStr(cl->object.cmdName));
          fprintf(stderr, "method=%s\n", methodName);
          }
        */
        co = 0;
        if (!rst->callIsDestroy && obj->opt) co = obj->opt->checkoptions;
        if ((co & CHECK_INVAR) &&
            ((result = AssertionCheckInvars(interp, obj, methodName, co)) == TCL_ERROR)) {
            goto finish;
        }
    } else {
        /* isTclProc == 1
         * if this is a filter, check whether its guard applies,
         * if not: just step forward to the next filter
         */
        if (frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
            XOTclCmdList *cmdList;
            /*
             * seek cmd in obj's filterOrder
             */
            assert(obj->flags & XOTCL_FILTER_ORDER_VALID);
            /* otherwise: FilterComputeDefined(interp, obj);*/

            cmdList = obj->filterOrder;
            while (cmdList && cmdList->cmdPtr != cmd)
                cmdList = cmdList->nextPtr;

            /*
             * when it is found, check whether it has a filter guard
             */
            if (cmdList) {
                int rc = GuardCall(obj, cl, (Tcl_Command) cmdList->cmdPtr, interp,
                                   (Tcl_Obj*)cmdList->clientData, 0);
                if (rc != TCL_OK) {
                    if (rc != TCL_ERROR) {
                        /*
                         * call next, use the given objv's, not the callstack objv
                         * we may not be in a method, thus there may be wrong or
                         * no callstackobjs
                         */
                        /*fprintf(stderr, "... calling nextmethod\n");  XOTclCallStackDump(interp);*/

                        rc = XOTclNextMethod(obj, interp, cl, methodName,
                                             objc, objv, /*useCallStackObjs*/ 0);
                        /*fprintf(stderr, "... after nextmethod\n");
                          XOTclCallStackDump(interp);*/

                    }
	
                    return rc;
                }
            }
        }

        if (!rst->callIsDestroy && obj->teardown
            && !(obj->flags & XOTCL_DESTROY_CALLED)) {
            if (obj->opt &&
                (obj->opt->checkoptions & CHECK_PRE) &&
                (result = AssertionCheck(interp, obj, cl, methodName, CHECK_PRE)) == TCL_ERROR) {
                goto finish;
            }
        }

        if (Tcl_Interp_numLevels(interp) <= 2)
            rst->returnCode = TCL_OK;
#ifdef DISPATCH_TRACE
        printCall(interp,"callProcCheck tclCmd", objc, objv);
        fprintf(stderr,"\tproc=%s\n", Tcl_GetCommandName(interp, cmd));
#endif

        result = (*Tcl_Command_objProc(cmd))(cp, interp, objc, objv);

#ifdef DISPATCH_TRACE
        printExit(interp,"callProcCheck tclCmd", objc, objv, result);
        /* fprintf(stderr, " returnCode %d xotcl rc %d\n",
           Tcl_Interp_returnCode(interp), rst->returnCode);*/
#endif
        if (result == TCL_BREAK && rst->returnCode == TCL_OK) {
            rst->returnCode = result;
        }
    
        /*    fprintf(stderr, "dispatch returned %d rst = %d\n", result, rst->returnCode);*/

        /* we give the information whether the call has destroyed the
           object back to the caller, because after CallStackPop it
           cannot be retrieved via the call stack */
        /* if the object is destroyed -> the assertion structs's are already
           destroyed */
        if (rst->cs.top->callType & XOTCL_CSC_CALL_IS_DESTROY) {
            rst->callIsDestroy = 1;
            /*fprintf(stderr,"callProcCheck: setting callIsDestroy = 1 method = %s\n", 
              methodName);*/
        }

        if (obj->opt && !rst->callIsDestroy && obj->teardown && 
            (obj->opt->checkoptions & CHECK_POST) &&
            (result = AssertionCheck(interp, obj, cl, methodName, CHECK_POST) == TCL_ERROR)) {
            goto finish;
        }
    }

 finish:

#if defined(PROFILE)
    if (rst->callIsDestroy == 0) {
        XOTclProfileEvaluateData(interp, startSec, startUsec, obj, cl, methodName);
    }
#endif
    return result;
}

static int
DoCallProcCheck(ClientData cp, ClientData clientData, Tcl_Interp *interp,
		int objc, Tcl_Obj *CONST objv[],
		Tcl_Command cmd, XOTclObject *obj, 
                XOTclClass *cl, char *methodName,
		int frameType, int fromNext, int forcePush) {
    int rc, push = 1, isTclProc = 0;

    /*fprintf(stderr,"DoCallProcCheck %s cp %p cmd %p proc %p\n", methodName, cp, cmd, Tcl_Command_objProc(cmd));*/
    if (Tcl_Command_flags(cmd) & XOTCL_NONLEAF_METHOD) {
        cp = clientData;
    } else if (cp) {
        Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);
        if (proc == XOTclObjDispatch) {
            assert((TclIsProc((Command *)cmd) == NULL));
            /*fprintf(stderr,"\t ObjDispatch\n");*/
        } else if (proc == XOTclForwardMethod || 
                   proc == XOTclObjscopedMethod) {
            tclCmdClientData *tcd = (tclCmdClientData *)cp;
            tcd->obj = obj;
            assert((TclIsProc((Command *)cmd) == NULL));
            /*fprintf(stderr,"\t forwarder\n");*/
        } else {
            /*fprintf(stderr,"\t isproc\n");*/
            assert((TclIsProc((Command *)cmd) != NULL));
            isTclProc = 1;
        }
    } else {
        push = forcePush;
        assert((TclIsProc((Command *)cmd) == NULL));
        cp = clientData;
    }

    if (!fromNext) {
        objc--;
        objv++;
    }

    /*fprintf(stderr,"DoCallProcCheck push=%d (%d), obj=%s fromNext %d\n",
      push, forcePush, ObjStr(obj->cmdName), fromNext);*/

    if (push) {
        /* push the xotcl info */
        if ((CallStackPush(interp, obj, cl, cmd, objc, objv, frameType)) != TCL_OK)
            return TCL_ERROR;
    }

    /*{int i; fprintf(stderr, "\tCALL ");for(i=0; i<objc; i++) {fprintf(stderr,"%s ", ObjStr(objv[i]));} fprintf(stderr,"\n");}*/
  
    rc = callProcCheck(cp, interp, objc, objv, cmd, obj, cl,
                       methodName, frameType, isTclProc);
    if (push) {
        CallStackPop(interp);
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
#ifdef OBJDELETION_TRACE
    Tcl_Obj *method;
#endif
    char *methodName, *callMethod;
    XOTclClass *cl = 0;
    ClientData cp = 0;
    Tcl_ObjCmdProc *proc = 0;
    Tcl_Command cmd = 0;
    XOTclRuntimeState *rst = RUNTIME_STATE(interp);
    Tcl_Obj *cmdName = obj->cmdName;
    XOTclCallStack *cs = &rst->cs;
#ifdef AUTOVARS
    int isNext;
#endif

    assert(objc>0);
    methodName = ObjStr(objv[1]);

    /*fprintf(stderr,"DoDispatch obj = %s objc = %d 0=%s\n",
      ObjStr(obj->cmdName), objc, ObjStr(objv[0]));*/

#ifdef AUTOVARS
    isNext = isNextString(methodName);
#endif
#ifdef DISPATCH_TRACE
    printCall(interp,"DISPATCH", objc, objv);
#endif

#ifdef OBJDELETION_TRACE
    method = objv[1];
    if (method == XOTclGlobalObjects[XOTE_CLEANUP] ||
        method == XOTclGlobalObjects[XOTE_DESTROY]) {
        fprintf(stderr, "%s->%s id=%p destroyCalled=%d\n",
                ObjStr(cmdName), methodName, obj,
                (obj->flags & XOTCL_DESTROY_CALLED));
    }
#endif

    objflags = obj->flags; /* avoid stalling */
    INCR_REF_COUNT(cmdName);

    if (!(objflags & XOTCL_FILTER_ORDER_VALID))
        FilterComputeDefined(interp, obj);

    if (!(objflags & XOTCL_MIXIN_ORDER_VALID))
        MixinComputeDefined(interp, obj);

    callMethod = methodName; 
#ifdef AUTOVARS
    if (!isNext) {
#endif
        /* Only start new filter chain, if
           (a) filters are defined and
           (b) the toplevel csc entry is not an filter on self
        */
        if (RUNTIME_STATE(interp)->doFilters &&
            !(flags & XOTCL_CM_NO_FILTERS) && !cs->guardCount &&
            ((obj->flags & XOTCL_FILTER_ORDER_DEFINED_AND_VALID) ==
             XOTCL_FILTER_ORDER_DEFINED_AND_VALID)) {
            XOTclObject *self = GetSelfObj(interp);
            if (obj != self ||
                cs->top->frameType != XOTCL_CSC_TYPE_ACTIVE_FILTER) {
	
                filterStackPushed = FilterStackPush(interp, obj, objv[1]);
                cmd = FilterSearchProc(interp, obj, &proc, &cp,
                                       &obj->filterStack->currentCmdPtr, &cl);
                /*fprintf(stderr,"filterSearchProc returned cmd %p proc %p\n", cmd, proc);*/
                if (cmd) { /* 'proc' and the other output vars are set as well */
                    frameType = XOTCL_CSC_TYPE_ACTIVE_FILTER;
                    callMethod = (char *) Tcl_GetCommandName(interp, cmd);
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

        if ((obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) ==
            XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {

            mixinStackPushed = MixinStackPush(obj);

            if (frameType != XOTCL_CSC_TYPE_ACTIVE_FILTER) {
                cmd = MixinSearchProc(interp, obj, methodName, &cl, &proc, &cp,
                                      &obj->mixinStack->currentCmdPtr);
                if (cmd) { /* 'proc' and the other output vars are set as well */
                    frameType = XOTCL_CSC_TYPE_ACTIVE_MIXIN;
                } else { /* the else branch could be deleted */
                    MixinStackPop(obj);
                    mixinStackPushed = 0;
                }
            }
        }
#ifdef AUTOVARS
    }
#endif

    /* if no filter/mixin is found => do ordinary method lookup */
    if (proc == 0) {

        /*fprintf(stderr,"ordinary lookup for obj %p method %s nsPtr %p\n",
          obj, methodName, obj->nsPtr);*/

        /*if (obj->nsPtr && !(obj->flags & XOTCL_NS_DESTROYED))*/
        if (obj->nsPtr)
            cmd = FindMethod(methodName, obj->nsPtr);
        /*fprintf(stderr,"findMethod for proc '%s' in %p returned %p\n", methodName, obj->nsPtr, cmd);*/

        if (cmd == NULL)
            cl = SearchCMethod(obj->cl, methodName, &cmd);

        if (cmd) {
            proc = Tcl_Command_objProc(cmd);
            cp   = Tcl_Command_objClientData(cmd);
        } else {
            assert(cp == 0);
        }
    }

    if (proc) {
        result = TCL_OK;
      
        /*fprintf(stderr,"cmd %p %s flags %x\n", cmd, methodName, 
          ((Command *) cmd)->flags && 0x00010000);*/

        if ((Tcl_Command_flags(cmd) & XOTCL_PROTECTED_METHOD) && 
            (flags & XOTCL_CM_NO_UNKNOWN) == 0) { 
            XOTclCallStackContent *csc = CallStackGetTopFrame(interp);
            XOTclObject *o = NULL;
            XOTclObjConvertObject(interp, objv[0], &o);
            /*XOTclCallStackDump(interp);*/
            /*fprintf(stderr,"+++ %s is protected, therefore maybe unknown %p %s self=%p o=%p\n",
              methodName, objv[0], ObjStr(objv[0]),
              csc->self, o);*/
            if (o != csc->self) {
                /*fprintf(stderr,"+++ protected method %s is not invoked\n", methodName);*/
                unknown = 1;
            }
        }
        if (!unknown) {
            /*fprintf(stderr,"DoDispatch calls DoCallProcCheck with obj = %s frameType %d\n",
              ObjStr(obj->cmdName), frameType);*/
            if ((result = DoCallProcCheck(cp, clientData, interp, objc, objv, cmd, obj, cl,
                                          callMethod, frameType,
                                          0 /* fromNext */, 0)) == TCL_ERROR) {
                result = XOTclErrInProc(interp, cmdName, 
                                        cl ? cl->object.cmdName : NULL, 
                                        callMethod);
            }
            unknown = RUNTIME_STATE(interp)->unknown;
        }
    } else {
        unknown = 1;
    }

    if (result == TCL_OK) {
        /* fprintf(stderr,"after doCallProcCheck %s unknown == %d\n", methodName, unknown);*/
        if (unknown) {

            if (XOTclObjectIsClass(obj) && (flags & XOTCL_CM_NO_UNKNOWN)) {
                return XOTclVarErrMsg(interp, ObjStr(objv[0]), 
                                      ": unable to dispatch method '",
                                      callMethod, "'", (char *) NULL);
            } else if (objv[1] != XOTclGlobalObjects[XOTE_UNKNOWN]) {
                /*
                 * back off and try unknown;
                 */
                XOTclObject *obj = (XOTclObject*)clientData;
                ALLOC_ON_STACK(Tcl_Obj *, objc+1, tov);
                /*
                  fprintf(stderr,"calling unknown for %s %s, flgs=%02x,%02x isClass=%d %p %s\n",
                  ObjStr(obj->cmdName), ObjStr(objv[1]), flags,  XOTCL_CM_NO_UNKNOWN,
                  XOTclObjectIsClass(obj), obj, ObjStr(obj->cmdName));
                */
                tov[0] = obj->cmdName;
                tov[1] = XOTclGlobalObjects[XOTE_UNKNOWN];
                if (objc>1)
                    memcpy(tov+2, objv+1, sizeof(Tcl_Obj *)*(objc-1));
                /*
                  fprintf(stderr,"?? %s unknown %s\n", ObjStr(obj->cmdName), ObjStr(tov[2]));
                */
                result = DoDispatch(clientData, interp, objc+1, tov, flags | XOTCL_CM_NO_UNKNOWN);
                FREE_ON_STACK(tov);
	
            } else { /* unknown failed */
                /*fprintf(stderr,"unknown fails\n");*/
                return XOTclVarErrMsg(interp, ObjStr(objv[0]), 
                                      ": unable to dispatch method '",
                                      ObjStr(objv[2]), "'", (char *) NULL);
            }

        }
    }
    /* be sure to reset unknown flag */
    if (unknown)
        RUNTIME_STATE(interp)->unknown = 0;

#ifdef DISPATCH_TRACE
    printExit(interp,"DISPATCH", objc, objv, result);
    fprintf(stderr,"obj %p mixinStackPushed %d mixinStack %p\n",
	    obj, mixinStackPushed, obj->mixinStack);
#endif


    /*if (!rst->callIsDestroy )
      fprintf(stderr, "obj freed? %p destroy %p self %p %s %d [%d] reference=%d,%d\n", obj,
      cs->top->destroyedCmd, cs->top->self, ObjStr(objv[1]),
      rst->callIsDestroy,
      cs->top->callType & XOTCL_CSC_CALL_IS_DESTROY,
      !rst->callIsDestroy,
      isdestroy);*/


    if (!rst->callIsDestroy) {
        /*!(obj->flags & XOTCL_DESTROY_CALLED)) {*/
        if (mixinStackPushed && obj->mixinStack)
            MixinStackPop(obj);

        if (filterStackPushed && obj->filterStack)
            FilterStackPop(obj);
    }

    DECR_REF_COUNT(cmdName); /* must be after last dereferencing of obj */
    return result;
}

static int
ObjDispatch(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
	    int flags) {
    int result;

#ifdef STACK_TRACE
    XOTclStackDump(interp);
#endif

#ifdef CALLSTACK_TRACE
    XOTclCallStackDump(interp);
#endif

    if (objc == 1) {
        Tcl_Obj *tov[2];
        tov[0] = objv[0];
        tov[1] = XOTclGlobalObjects[XOTE_DEFAULTMETHOD];
        result = DoDispatch(clientData, interp, 2, tov, flags);
    } else {
        /* normal dispatch */
        result = DoDispatch(clientData, interp, objc, objv, flags);
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
    result =  ObjDispatch((ClientData)GetSelfObj(interp), interp, objc, objv, 0);
    objTrace("AFTER SELF DISPATCH", obj);
    return result;
}
#endif

int
XOTclObjDispatch(ClientData clientData, Tcl_Interp *interp, 
		 int objc, Tcl_Obj *CONST objv[]) {
    return ObjDispatch(clientData, interp, objc, objv, 0);
}

/*
 * Non Positional Args
 */

static void
NonposArgsDeleteHashEntry(Tcl_HashEntry *hPtr) {
    XOTclNonposArgs *nonposArg = (XOTclNonposArgs*) Tcl_GetHashValue(hPtr);
    if (nonposArg) {
        if (nonposArg->nonposArgs) {
            DECR_REF_COUNT(nonposArg->nonposArgs);
        }
        if (nonposArg->ordinaryArgs) {
            DECR_REF_COUNT(nonposArg->ordinaryArgs);
        }
        if (nonposArg->slotObj) {
            DECR_REF_COUNT(nonposArg->slotObj);
        }
        MEM_COUNT_FREE("nonposArg", nonposArg);
        ckfree((char *) nonposArg);
        Tcl_DeleteHashEntry(hPtr);
    }
}

static Tcl_HashTable*
NonposArgsCreateTable() {
    Tcl_HashTable *nonposArgsTable =
        (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
    MEM_COUNT_ALLOC("Tcl_HashTable", nonposArgsTable);
    Tcl_InitHashTable(nonposArgsTable, TCL_STRING_KEYS);
    MEM_COUNT_ALLOC("Tcl_InitHashTable", nonposArgsTable);
    return nonposArgsTable;
}

static void
NonposArgsFreeTable(Tcl_HashTable *nonposArgsTable) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = nonposArgsTable ?
        Tcl_FirstHashEntry(nonposArgsTable, &hSrch) : 0;
    for (; hPtr != 0; hPtr = Tcl_NextHashEntry(&hSrch)) {
        NonposArgsDeleteHashEntry(hPtr);
    }
}

static XOTclNonposArgs*
NonposArgsGet(Tcl_HashTable *nonposArgsTable, char  *methodName) {
    Tcl_HashEntry *hPtr;
    if (nonposArgsTable &&
        ((hPtr = Tcl_FindHashEntry(nonposArgsTable, methodName)))) {
        return (XOTclNonposArgs*) Tcl_GetHashValue(hPtr);
    }
    return NULL;
}

static Tcl_Obj *
NonposArgsFormat(Tcl_Interp *interp, Tcl_Obj *nonposArgsData) {
    int r1, npalistc, npac, checkc, i, j, first;
    Tcl_Obj **npalistv, **npav, **checkv,
        *list = Tcl_NewListObj(0, NULL), *innerlist,
        *nameStringObj;

    /*fprintf(stderr, "nonposargsformat '%s'\n", ObjStr(nonposArgsData));*/

    if (nonposArgsData) {
        r1 = Tcl_ListObjGetElements(interp, nonposArgsData, &npalistc, &npalistv);
        if (r1 == TCL_OK) {
            for (i=0; i < npalistc; i++) {
                r1 = Tcl_ListObjGetElements(interp, npalistv[i], &npac, &npav);
                if (r1 == TCL_OK) {
                    nameStringObj = Tcl_NewStringObj("-", 1);
                    Tcl_AppendStringsToObj(nameStringObj, ObjStr(npav[0]), (char *) NULL);
                    if (npac > 1 && *(ObjStr(npav[1])) != '\0') {
                        first = 1;
                        r1 = Tcl_ListObjGetElements(interp, npav[1], &checkc, &checkv);
                        if (r1 == TCL_OK) {
                            for (j=0; j < checkc; j++) {
                                if (first) {
                                    Tcl_AppendToObj(nameStringObj,":", 1);
                                    first = 0;
                                } else {
                                    Tcl_AppendToObj(nameStringObj,",", 1);
                                }
                                Tcl_AppendToObj(nameStringObj, ObjStr(checkv[j]), -1);
                            }
                        }
                    }
                    /* fprintf(stderr, "nonposargsformat namestring '%s'\n", 
                       ObjStr(nameStringObj));*/
                    
#if 1
                    innerlist = Tcl_NewListObj(0, NULL);
                    Tcl_ListObjAppendElement(interp, innerlist, nameStringObj);
                    if (npac > 2) {
                        Tcl_ListObjAppendElement(interp, innerlist, npav[2]);
                    }
#else
                    {
                        Tcl_DString ds, *dsPtr = &ds;
                        DSTRING_INIT(dsPtr);
                        Tcl_DStringAppend(dsPtr, ObjStr(nameStringObj), -1);
                        if (npac > 2) {
                            Tcl_DStringAppendElement(dsPtr, ObjStr(npav[2]));
                        }
                        innerlist = Tcl_NewStringObj(Tcl_DStringValue(dsPtr), 
                                                     Tcl_DStringLength(dsPtr));
                        DSTRING_FREE(dsPtr);
                    }
#endif
                    Tcl_ListObjAppendElement(interp, list, innerlist);
                }
            }
        }
    }
    return list;
}

/*
 *  Proc-Creation
 */

static Tcl_Obj *addPrefixToBody(Tcl_Obj *body, int nonposArgs) {
    Tcl_Obj *resultBody;
    resultBody = Tcl_NewStringObj("", 0);
    INCR_REF_COUNT(resultBody);
    Tcl_AppendStringsToObj(resultBody, "::xotcl::initProcNS\n", (char *) NULL);
    if (nonposArgs) {
        Tcl_AppendStringsToObj(resultBody,
                               "::xotcl::interpretNonpositionalArgs $args\n",
                               (char *) NULL);
    }
    Tcl_AppendStringsToObj(resultBody, ObjStr(body), (char *) NULL);
    return resultBody;
}

static Tcl_Obj* 
nonposargType(char *start, int len) {
    Tcl_Obj *result = Tcl_NewStringObj("type=", 5);
    Tcl_AppendToObj(result, start, len);
    fprintf(stderr, "TYPE = '%s'\n", ObjStr(result));
    return result;
}

static int
parseNonposArgs(Tcl_Interp *interp,
		char *procName, Tcl_Obj *npArgs, Tcl_Obj *ordinaryArgs,
		Tcl_HashTable **nonposArgsTable,
		int *haveNonposArgs) {
    int rc, nonposArgsDefc, npac;
    Tcl_Obj **nonposArgsDefv;

    rc = Tcl_ListObjGetElements(interp, npArgs, &nonposArgsDefc, &nonposArgsDefv);
    if (rc != TCL_OK) {
        return XOTclVarErrMsg(interp, "cannot break down non-positional args: ",
                              ObjStr(npArgs), (char *) NULL);
    }
    if (nonposArgsDefc > 0) {
        int start, end, length, i, j, nw = 0;
        char *arg;
        Tcl_Obj *npaObj, **npav, *nonposArgsObj = Tcl_NewListObj(0, NULL);
        Tcl_HashEntry *hPtr;

        INCR_REF_COUNT(nonposArgsObj);
        for (i=0; i < nonposArgsDefc; i++) {
            rc = Tcl_ListObjGetElements(interp, nonposArgsDefv[i], &npac, &npav);
            if (rc == TCL_ERROR || npac < 1 || npac > 2) {
                DECR_REF_COUNT(nonposArgsObj);
                return XOTclVarErrMsg(interp, "wrong # of elements in non-positional args ",
                                      "(should be 1 or 2 list elements): ",
                                      ObjStr(npArgs), (char *) NULL);
            }
            npaObj = Tcl_NewListObj(0, NULL);
            arg = ObjStr(npav[0]);
            if (arg[0] != '-') {
                DECR_REF_COUNT(npaObj);
                DECR_REF_COUNT(nonposArgsObj);
                return XOTclVarErrMsg(interp, "non-positional args does not start with '-': ",
                                      arg, " in: ", ObjStr(npArgs), (char *) NULL);
            }
	
            length = strlen(arg);
            for (j=0; j<length; j++) {
                if (arg[j] == ':') break;
            }
            if (arg[j] == ':') {
                int l;
                Tcl_Obj *list = Tcl_NewListObj(0, NULL);
                
                /* first the var name */
                Tcl_ListObjAppendElement(interp, npaObj, Tcl_NewStringObj(arg+1, j-1));

                start = j+1;
                while(start<length && isspace((int)arg[start])) start++;
                for (l=start; l<length;l++) {
                    if (arg[l] == ',') {
                        for (end = l;end>0 && isspace((int)arg[end-1]); end--);

                        Tcl_ListObjAppendElement(interp, list, 
                                                 nonposargType(arg+start, end-start));
                        l++;
                        start = l;
                        while(start<length && isspace((int)arg[start])) start++;
                    }
                }
                /* append last arg, this is a type */
                for (end = l;end>0 && isspace((int)arg[end-1]); end--);
                Tcl_ListObjAppendElement(interp, list, 
                                         nonposargType(arg+start, end-start));
                /* append the whole thing to the list */
                Tcl_ListObjAppendElement(interp, npaObj, list);
                /*  fprintf(stderr," appending list npa='%s'\n", ObjStr(npaObj));*/
            } else {
                Tcl_ListObjAppendElement(interp, npaObj, Tcl_NewStringObj(arg+1, length));
                Tcl_ListObjAppendElement(interp, npaObj, Tcl_NewStringObj("", 0));
                /*  fprintf(stderr," no colon npa='%s'\n", ObjStr(npaObj));*/
            }
            if (npac == 2) {
                Tcl_ListObjAppendElement(interp, npaObj, npav[1]);
                /*  fprintf(stderr," npac==2 ='%s'\n", ObjStr(npaObj)); */
            }
            Tcl_ListObjAppendElement(interp, nonposArgsObj, npaObj);
            *haveNonposArgs = 1;
        }

        if (*haveNonposArgs) {
            XOTclNonposArgs *nonposArg;
	
            if (*nonposArgsTable == 0) {
                *nonposArgsTable = NonposArgsCreateTable();
            }

            hPtr = Tcl_CreateHashEntry(*nonposArgsTable, procName, &nw);
            assert(nw);

            MEM_COUNT_ALLOC("nonposArg", nonposArg);
            nonposArg = (XOTclNonposArgs*)ckalloc(sizeof(XOTclNonposArgs));
            nonposArg->slotObj = NULL;
            nonposArg->nonposArgs = nonposArgsObj;
            { /* FIX AND REMOVE ME */
                int i, r2, ordinaryArgsDefc;
                Tcl_Obj **ordinaryArgsDefv;
	
                r2 = Tcl_ListObjGetElements(interp, ordinaryArgs,
                                            &ordinaryArgsDefc, &ordinaryArgsDefv);
                if (r2 == TCL_OK) {
                    Tcl_Obj *objv[ordinaryArgsDefc];
                    for (i=0; i< ordinaryArgsDefc; i++) {
                        objv[i] = Tcl_DuplicateObj(ordinaryArgsDefv[i]);
                    }
                    nonposArg->ordinaryArgs = Tcl_NewListObj(ordinaryArgsDefc, objv);
                    INCR_REF_COUNT(nonposArg->ordinaryArgs);
                } else {
                    fprintf(stderr, "could not split ordinaryArgs\n");
                }
            }
            Tcl_SetHashValue(hPtr, (ClientData)nonposArg);
        } else {
            /* for strange reasons, we did not find nonpos-args, although we
               have definitions */
            DECR_REF_COUNT(nonposArgsObj);
        }
    }
    return TCL_OK;
}


static int
MakeProc(Tcl_Namespace *ns, XOTclAssertionStore *aStore,
	 Tcl_HashTable **nonposArgsTable,
	 Tcl_Interp *interp, int objc, Tcl_Obj *objv[], XOTclObject *obj, int clsns) {
    int result, incr, haveNonposArgs=0;
    TclCallFrame frame, *framePtr = &frame;
    Tcl_Obj *ov[4];
    Tcl_HashEntry *hPtr = NULL;
    char *procName = ObjStr(objv[1]);

    if (*nonposArgsTable && (hPtr = Tcl_FindHashEntry(*nonposArgsTable, procName))) {
        NonposArgsDeleteHashEntry(hPtr);
    }

    ov[0] = objv[0];
    ov[1] = objv[1];

    if (objc == 5 || objc == 7) {
        if ((result = parseNonposArgs(interp, procName, objv[2], objv[3],
                                      nonposArgsTable, &haveNonposArgs)) != TCL_OK)
            return result;

        if (haveNonposArgs) {
            ov[2] = XOTclGlobalObjects[XOTE_ARGS];
            ov[3] = addPrefixToBody(objv[4], 1);
        } else { /* no explicit nonpos arguments */
            ov[2] = objv[3];
            ov[3] = addPrefixToBody(objv[4], 0);
        }
    } else {
        int argsc, i;
        Tcl_Obj **argsv;

        /**** begin joined nonpos + pos argument list ***/

        /* see, if we have nonposArgs in the ordinary argument list */
        result = Tcl_ListObjGetElements(interp, objv[2], &argsc, &argsv);
        if (result != TCL_OK) {
            return XOTclVarErrMsg(interp, "cannot break args into list: ",
                                  ObjStr(objv[2]), (char *) NULL);
        }
        for (i=0; i<argsc; i++) {
            char *arg;
            int npac, rc;
            Tcl_Obj **npav;
            /* arg = ObjStr(argsv[i]);
               fprintf(stderr, "*** argparse0 arg='%s'\n", arg);*/

            rc = Tcl_ListObjGetElements(interp, argsv[i], &npac, &npav);
            if (rc == TCL_OK && npac > 0) {
                arg = ObjStr(npav[0]);
                /*fprintf(stderr, "*** argparse1 arg='%s' rc=%d\n", arg, rc);*/
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
            /*fprintf(stderr, "nonpos <%s> ordinary <%s>\n",
              ObjStr(nonposArgs), ObjStr(ordinaryArgs));*/
            result = parseNonposArgs(interp, procName, nonposArgs, ordinaryArgs,
                                     nonposArgsTable, &haveNonposArgs);
            DECR_REF_COUNT(ordinaryArgs);
            DECR_REF_COUNT(nonposArgs);
            if (result != TCL_OK)
                return result;
        }

        /**** end joined nonpos + pos argument list ***/

        if (haveNonposArgs) {
            ov[2] = XOTclGlobalObjects[XOTE_ARGS];
            ov[3] = addPrefixToBody(objv[3], 1);
        } else { /* no nonpos arguments */
            ov[2] = objv[2];
            ov[3] = addPrefixToBody(objv[3], 0);
        }

    }

#ifdef AUTOVARS
    { char *p, *body;
        body = ObjStr(ov[3]);
        if ((p = strstr(body, "self")) && p != body && *(p-1) != '[')
            Tcl_AppendStringsToObj(ov[3], "::set self [self]\n", (char *) NULL);
        if (strstr(body, "proc"))
            Tcl_AppendStringsToObj(ov[3], "::set proc [self proc]\n", (char *) NULL);
        if (strstr(body, "class"))
            Tcl_AppendStringsToObj(ov[3], "::set class [self class]\n", (char *) NULL);
    }
#endif

    Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, ns, 0);

    result = Tcl_ProcObjCmd(0, interp, 4, ov) != TCL_OK;
#if defined(NAMESPACEINSTPROCS)
    {
        Proc *procPtr = TclFindProc((Interp *)interp, procName);
        /*
          fprintf(stderr,"proc=%p cmd=%p\n", procPtr, procPtr->cmdPtr);
          fprintf(stderr,"proc=%p cmd=%p ns='%s' objns=%s clsns=%s\n", procPtr, procPtr->cmdPtr,
          procPtr->cmdPtr->nsPtr->fullName,((Command*)obj->id)->nsPtr->fullName,
          ((Command*)obj->cl->object.id)->nsPtr->fullName);
        */
        /*** patch the command ****/
        if (procPtr) {
            if (clsns) {
                /* set the namespace of the method as inside of the class */
                if (!obj->nsPtr) {
                    makeObjNamespace(interp, obj);
                }
                /*fprintf(stderr,"obj %s\n", ObjStr(obj->cmdName));
                  fprintf(stderr,"ns %p obj->ns %p\n", ns, obj->nsPtr);
                  fprintf(stderr,"ns %s obj->ns %s\n", ns->fullName, obj->nsPtr->fullName);*/
                procPtr->cmdPtr->nsPtr = (Namespace*) obj->nsPtr;
            } else {
                /* set the namespace of the method to the same namespace the class has */
                procPtr->cmdPtr->nsPtr = ((Command *)obj->id)->nsPtr;
            }
        }
    }
#endif

    Tcl_PopCallFrame(interp);

    if (objc == 6 || objc == 7) {
        incr = (objc == 6) ? 0:1;
        AssertionAddProc(interp, ObjStr(objv[1]), aStore, objv[4+incr], objv[5+incr]);
    }

    DECR_REF_COUNT(ov[3]);

    return result;
}

/*
 * List-Functions for Info
 */
static int
ListInfo(Tcl_Interp *interp, int isclass) {
    Tcl_ResetResult(interp);
    Tcl_AppendElement(interp, "vars"); Tcl_AppendElement(interp, "body");
    Tcl_AppendElement(interp, "default"); Tcl_AppendElement(interp, "args");
    Tcl_AppendElement(interp, "procs"); Tcl_AppendElement(interp, "commands");
    Tcl_AppendElement(interp, "class"); Tcl_AppendElement(interp, "children");
    Tcl_AppendElement(interp, "filter"); Tcl_AppendElement(interp, "filterguard");
    Tcl_AppendElement(interp, "forward");
    Tcl_AppendElement(interp, "info");
    Tcl_AppendElement(interp, "invar"); Tcl_AppendElement(interp, "mixin");
    Tcl_AppendElement(interp, "methods");
    Tcl_AppendElement(interp, "parent");
    Tcl_AppendElement(interp, "pre"); Tcl_AppendElement(interp, "post");
    Tcl_AppendElement(interp, "precedence");
    if (isclass) {
        Tcl_AppendElement(interp, "superclass"); Tcl_AppendElement(interp, "subclass");
        Tcl_AppendElement(interp, "heritage"); Tcl_AppendElement(interp, "instances");
        Tcl_AppendElement(interp, "instcommands"); Tcl_AppendElement(interp, "instprocs");
        Tcl_AppendElement(interp, "instdefault"); Tcl_AppendElement(interp, "instbody");
        Tcl_AppendElement(interp, "instmixin");
        Tcl_AppendElement(interp, "instforward");
        Tcl_AppendElement(interp, "instmixinof"); Tcl_AppendElement(interp, "mixinof");
        Tcl_AppendElement(interp, "classchildren"); Tcl_AppendElement(interp, "classparent");
        Tcl_AppendElement(interp, "instfilter"); Tcl_AppendElement(interp, "instfilterguard");
        Tcl_AppendElement(interp, "instinvar");
        Tcl_AppendElement(interp, "instpre"); Tcl_AppendElement(interp, "instpost");
        Tcl_AppendElement(interp, "parameter");
    }
    return TCL_OK;
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

static int
ListKeys(Tcl_Interp *interp, Tcl_HashTable *table, char *pattern) {
    Tcl_HashEntry *hPtr;
    char *key;

    if (pattern && noMetaChars(pattern)) {
        hPtr = table ? Tcl_FindHashEntry(table, pattern) : 0;
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

        hPtr = tablePtr ? Tcl_CreateHashEntry(tablePtr, (char *)patternObj, NULL) : 0;
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
ListVars(Tcl_Interp *interp, XOTclObject *obj, char *pattern) {
    Tcl_Obj *varlist, *okList, *element;
    int i, length;
    TclVarHashTable *varTable = obj->nsPtr ? Tcl_Namespace_varTable(obj->nsPtr) : obj->varTable;
  
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
        if (varExists(interp, obj, ObjStr(element), NULL, 0, 1)) {
            Tcl_ListObjAppendElement(interp, okList, element);
        } else {
            /*fprintf(stderr,"must ignore '%s' %d\n", ObjStr(element), i);*/
            /*Tcl_ListObjReplace(interp, varlist, i, 1, 0, NULL);*/
        }
    }
    Tcl_SetObjResult(interp, okList);
    return TCL_OK;
}

static int
ListObjPtrHashTable(Tcl_Interp *interp, Tcl_HashTable *table, char *pattern) {
    Tcl_HashEntry *hPtr;
    if (pattern && noMetaChars(pattern)) {
        XOTclObject *childobj = XOTclpGetObject(interp, pattern);
        hPtr = Tcl_FindHashEntry(table, (char *)childobj);
        if (hPtr) {
            Tcl_SetObjResult(interp, childobj->cmdName);
        } else {
            Tcl_SetObjResult(interp, XOTclGlobalObjects[XOTE_EMPTY]);
        }
    } else {
        Tcl_Obj *list = Tcl_NewListObj(0, NULL);
        Tcl_HashSearch hSrch;
        hPtr = table ? Tcl_FirstHashEntry(table, &hSrch) : 0;
        for (;  hPtr; hPtr = Tcl_NextHashEntry(&hSrch)) {
            XOTclObject *obj = (XOTclObject*)Tcl_GetHashKey(table, hPtr);
            if (!pattern || Tcl_StringMatch(ObjStr(obj->cmdName), pattern)) {
                Tcl_ListObjAppendElement(interp, list, obj->cmdName);
            }
        }
        Tcl_SetObjResult(interp, list);
    }
    return TCL_OK;
}

static int
ListMethodKeys(Tcl_Interp *interp, Tcl_HashTable *table, char *pattern,
	       int noProcs, int noCmds, Tcl_HashTable *dups, int onlyForwarder) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr = table ? Tcl_FirstHashEntry(table, &hSrch) : 0;

    for (; hPtr != 0; hPtr = Tcl_NextHashEntry(&hSrch)) {
        char *key = Tcl_GetHashKey(table, hPtr);
        Tcl_Command cmd = (Tcl_Command)Tcl_GetHashValue(hPtr);
        Tcl_ObjCmdProc *proc = Tcl_Command_objProc(cmd);

        if (pattern && !Tcl_StringMatch(key, pattern)) continue;
        if (noCmds  && proc != RUNTIME_STATE(interp)->objInterpProc) continue;
        if (noProcs && proc == RUNTIME_STATE(interp)->objInterpProc) continue;
        if (onlyForwarder && proc != XOTclForwardMethod) continue;
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

        if (((Command *) cmd)->flags & XOTCL_PROTECTED_METHOD) {
            /*fprintf(stderr, "--- dont list protected name '%s'\n", key);*/
            continue;
        }
        Tcl_AppendElement(interp, key);
    }
    /*fprintf(stderr, "listkeys returns '%s'\n", ObjStr(Tcl_GetObjResult(interp)));*/
    return TCL_OK;
}

static int
forwardList(Tcl_Interp *interp, Tcl_HashTable *table, char *pattern,
	    int definition) {
    int rc;
    if (definition) {
        Tcl_HashEntry *hPtr = table ? Tcl_FindHashEntry(table, pattern) : 0;
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
                /* ERROR HANDLING TODO protected missing ****GN**** */
            }
        } else {
            /* ERROR HANDLING TODO ****GN**** */
        }
        rc = TCL_OK;
    } else {
        rc = ListMethodKeys(interp, table, pattern, 1, 0, NULL, 1);
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
        ListMethodKeys(interp, cmdTable, pattern, noProcs, noCmds, dups, 0);
    }

    if (!noMixins) {
        if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
            MixinComputeDefined(interp, obj);
        if (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
            XOTclCmdList *ml = obj->mixinOrder;
            XOTclClass *mixin;
            while (ml) {
                int guardOk = TCL_OK;
                mixin = XOTclGetClassFromCmdPtr(ml->cmdPtr);
                if (inContext) {
                    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
                    if (!cs->guardCount) {
                        guardOk = GuardCall(obj, 0, 0, interp, (Tcl_Obj*)ml->clientData, 1);
                    }
                }
                if (mixin && guardOk == TCL_OK) {
                    Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(mixin->nsPtr);
                    ListMethodKeys(interp, cmdTable, pattern, noProcs, noCmds, dups, 0);
                }
                ml = ml->nextPtr;
            }
        }
    }

    /* append per-class filters */
    for (pl = ComputeOrder(obj->cl, obj->cl->order, Super); pl; pl = pl->nextPtr) {
        Tcl_HashTable *cmdTable = Tcl_Namespace_cmdTable(pl->cl->nsPtr);
        ListMethodKeys(interp, cmdTable, pattern, noProcs, noCmds, dups, 0);
    }
    Tcl_DeleteHashTable(dups);
    return TCL_OK;
}

static int XOTclCInfoMethod(ClientData, Tcl_Interp *, int, Tcl_Obj *CONST v[]);

static int
ListClass(Tcl_Interp *interp, XOTclObject *obj, char *pattern,
	  int objc, Tcl_Obj *CONST objv[]) {
    if (pattern == NULL) {
        Tcl_SetObjResult(interp, obj->cl->object.cmdName);
        return TCL_OK;
    } else {
        int result;
        ALLOC_ON_STACK(Tcl_Obj *, objc, ov);

        memcpy(ov, objv, sizeof(Tcl_Obj *)*objc);
        ov[1] = Tcl_NewStringObj("superclass", 10);
        INCR_REF_COUNT(ov[1]);
        result = XOTclCInfoMethod((ClientData)obj->cl, interp, objc, ov);
        DECR_REF_COUNT(ov[1]);
        FREE_ON_STACK(ov);
        return result;
    }
}

static int
ListSuperclasses(Tcl_Interp *interp, XOTclClass *cl, char *pattern) {
    XOTclClasses *sl = cl->super;
    XOTclClasses *sc = 0;

    if (pattern == NULL) {
        /*
         * reverse the list to obtain presentation order
         */

        Tcl_ResetResult(interp);
        while (sc != sl) {
            XOTclClasses *nl = sl;
            while (nl->nextPtr != sc) nl = nl->nextPtr;
            Tcl_AppendElement(interp, className(nl->cl));
            sc = nl;
        }
    } else if (!noMetaChars(pattern)) {
        char *key;
        Tcl_ResetResult(interp);
        while (sc != sl) {
            XOTclClasses *nl = sl;
            while (nl->nextPtr != sc) nl = nl->nextPtr;
            key = className(nl->cl);
            if (Tcl_StringMatch(key, pattern)) {
                Tcl_AppendElement(interp, key);
            }
            sc = nl;
        }
    } else {
        XOTclClass *isc = XOTclpGetClass(interp, pattern);
        if (isc == 0) {
            /* return XOTclErrBadVal(interp, "info superclass", "a class", pattern); */
            Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
        } else {
            XOTclClasses *pl;
            /*
             * search precedence to see if we're related or not
             */
            for (pl = ComputeOrder(cl, cl->order, Super); pl; pl = pl->nextPtr) {
                if (pl->cl == isc) {
                    Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
                    break;
                }
            }
            if (pl == NULL) {
                Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
            }
        }
    }
    return TCL_OK;
}

static int
ListSubclasses(Tcl_Interp *interp, XOTclClass *cl, char *pattern) {
    XOTclClasses *sl = cl->sub;
    XOTclClasses *sc = 0;

    if (pattern == NULL) {
        /*
         * order unimportant
         */
        Tcl_ResetResult(interp);
        for (sc = sl; sc != 0; sc = sc->nextPtr)
            Tcl_AppendElement(interp, className(sc->cl));
    } else if (!noMetaChars(pattern)) {
        Tcl_ResetResult(interp);
        for (sc = sl; sc != 0; sc = sc->nextPtr) {
            char *key = className(sc->cl);
            if (Tcl_StringMatch(key, pattern)) {
                Tcl_AppendElement(interp, key);
            }
        }
    } else {
        XOTclClass *isc = XOTclpGetClass(interp, pattern);
        XOTclClasses *pl;
        XOTclClasses *saved;

        if (isc == 0) 
            return XOTclErrBadVal(interp, "info subclass", "a class", pattern);
        saved = cl->order;
        cl->order = 0;

        /*
         * search precedence to see if we're related or not
         */
        for (pl = ComputeOrder(cl, cl->order, Sub); pl; pl = pl->nextPtr) {
            if (pl->cl == isc) {
                Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
                break;
            }
        }
        if (pl == 0)
            Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);

        XOTclClassListFree(cl->order);
        cl->order = saved;
    }
    return TCL_OK;
}



static int
ListHeritage(Tcl_Interp *interp, XOTclClass *cl, char *pattern) {
    XOTclClasses *pl = ComputeOrder(cl, cl->order, Super);
    Tcl_ResetResult(interp);
    if (pl) pl=pl->nextPtr;
    for (; pl != 0; pl = pl->nextPtr) {
        char *name = className(pl->cl);
        if (pattern && !Tcl_StringMatch(name, pattern)) continue;
        Tcl_AppendElement(interp, name);
    }
    return TCL_OK;
}


static XOTclClasses *
ComputePrecedenceWithMixins(Tcl_Interp *interp, XOTclObject *obj, char *pattern) {
    XOTclClasses *precedenceList = NULL, *pcl, **npl = &precedenceList;

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
    pcl = ComputeOrder(obj->cl, obj->cl->order, Super);
    for (; pcl != 0; pcl = pcl->nextPtr) {
        if (pattern) {
            char *name = className(pcl->cl);
            if (!Tcl_StringMatch(name, pattern)) continue;
        }
        npl = XOTclClassListAdd(npl, pcl->cl, NULL);
    }
    return precedenceList;
}


static int
ListPrecedence(Tcl_Interp *interp, XOTclObject *obj, char *pattern) {
    XOTclClasses *pl, *precedenceList;
    Tcl_ResetResult(interp);
    precedenceList = ComputePrecedenceWithMixins(interp, obj, pattern);
    for (pl = precedenceList; pl != 0; pl = pl->nextPtr) {
        char *name = className(pl->cl);
        Tcl_AppendElement(interp, name);
    }
    XOTclClassListFree(pl);
    return TCL_OK;
}


static Proc*
FindProc(Tcl_Interp *interp, Tcl_HashTable *table, char *name) {
    Tcl_HashEntry *hPtr = table ? Tcl_FindHashEntry(table, name) : 0;
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

static int
ListProcArgs(Tcl_Interp *interp, Tcl_HashTable *table, char *name) {
    Proc *proc = FindProc(interp, table, name);
    if (proc) {
        CompiledLocal *args = proc->firstLocalPtr;
        Tcl_ResetResult(interp);
        for (;args != NULL; args = args->nextPtr) {
            if (TclIsCompiledLocalArgument(args))
                Tcl_AppendElement(interp, args->name);

        }
        return TCL_OK;
    }
    return XOTclErrBadVal(interp, "info args", "a tcl method name", name);
}

static int
ListArgsFromOrdinaryArgs(Tcl_Interp *interp, XOTclNonposArgs *nonposArgs) {
    int i, rc, ordinaryArgsDefc, defaultValueObjc;
    Tcl_Obj **ordinaryArgsDefv, **defaultValueObjv, *ordinaryArg,
        *argList = Tcl_NewListObj(0, NULL);
    rc = Tcl_ListObjGetElements(interp, nonposArgs->ordinaryArgs,
                                &ordinaryArgsDefc, &ordinaryArgsDefv);
    if (rc == TCL_OK) {
        for (i=0; i < ordinaryArgsDefc; i++) {
            ordinaryArg = ordinaryArgsDefv[i];
            rc = Tcl_ListObjGetElements(interp, ordinaryArg,
                                        &defaultValueObjc, &defaultValueObjv);
            if (rc == TCL_OK && defaultValueObjc == 2) {
                ordinaryArg = defaultValueObjv[0];
            }
            Tcl_ListObjAppendElement(interp, argList, ordinaryArg);
        }
        Tcl_SetObjResult(interp, argList);
    }
    return rc;
}

static int
GetProcDefault(Tcl_Interp *interp, Tcl_HashTable *table,
	       char *name, char *arg, Tcl_Obj **resultObj) {
    Proc *proc = FindProc(interp, table, name);
    *resultObj = 0;
    if (proc) {
        CompiledLocal *ap;
        for (ap = proc->firstLocalPtr; ap != 0; ap = ap->nextPtr) {
            if (!TclIsCompiledLocalArgument(ap)) continue;
            if (strcmp(arg, ap->name) != 0) continue;
	
            if (ap->defValuePtr != NULL) {
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
    CallStackUseActiveFrames(interp,&ctx);

    if (defVal != NULL) {
        if (Tcl_ObjSetVar2(interp, var, NULL, defVal, 0) != NULL) {
            Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
        } else {
            result = TCL_ERROR;
        }
    } else {
        if (Tcl_ObjSetVar2(interp, var, NULL,
                           XOTclGlobalObjects[XOTE_EMPTY], 0) != NULL) {
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

static int
ListProcDefault(Tcl_Interp *interp, Tcl_HashTable *table,
                char *name, char *arg, Tcl_Obj *var) {
    Tcl_Obj *defVal;
    int result;
    if (GetProcDefault(interp, table, name, arg, &defVal) == TCL_OK) {
        result = SetProcDefault(interp, var, defVal);
    } else {
        XOTclVarErrMsg(interp, "method '", name,
                       "' doesn't exist or doesn't have an argument '",
                       arg, "'", (char *) NULL);
        result = TCL_ERROR;
    }
    return result;
}

static int
ListDefaultFromOrdinaryArgs(Tcl_Interp *interp, char *procName,
			    XOTclNonposArgs *nonposArgs, char *arg, Tcl_Obj *var) {
    int i, rc, ordinaryArgsDefc, defaultValueObjc;
    Tcl_Obj **ordinaryArgsDefv, **defaultValueObjv, *ordinaryArg;

    rc = Tcl_ListObjGetElements(interp, nonposArgs->ordinaryArgs,
                                &ordinaryArgsDefc, &ordinaryArgsDefv);
    if (rc != TCL_OK)
        return TCL_ERROR;

    for (i=0; i < ordinaryArgsDefc; i++) {
        ordinaryArg = ordinaryArgsDefv[i];
        rc = Tcl_ListObjGetElements(interp, ordinaryArg,
                                    &defaultValueObjc, &defaultValueObjv);
        if (rc == TCL_OK && !strcmp(arg, ObjStr(defaultValueObjv[0]))) {
            return SetProcDefault(interp, var, defaultValueObjc == 2 ?
                                  defaultValueObjv[1] : NULL);
        }
    }
    XOTclVarErrMsg(interp, "method '", procName, "' doesn't have an argument '",
                   arg, "'", (char *) NULL);
    return TCL_ERROR;
}

static char *
StripBodyPrefix(char *body) {
    if (strncmp(body, "::xotcl::initProcNS\n", 20) == 0)
        body+=20;
    if (strncmp(body, "::xotcl::interpretNonpositionalArgs $args\n", 42) == 0)
        body+=42;
    return body;
}


static int
ListProcBody(Tcl_Interp *interp, Tcl_HashTable *table, char *name) {
    Proc *proc = FindProc(interp, table, name);

    if (proc) {
        char *body = ObjStr(proc->bodyPtr);
        Tcl_SetObjResult(interp, Tcl_NewStringObj(StripBodyPrefix(body), -1));
        return TCL_OK;
    }
    return XOTclErrBadVal(interp, "info body", "a tcl method name", name);
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
            (childobj->id && Tcl_Command_nsPtr(childobj->id) == obj->nsPtr)  /* true children */
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
        for (; hPtr != 0; hPtr = Tcl_NextHashEntry(&hSrch)) {
            key = Tcl_GetHashKey(cmdTable, hPtr);
            if (!pattern || Tcl_StringMatch(key, pattern)) {
                if ((childobj = XOTclpGetObject(interp, key)) &&
                    (!classesOnly || XOTclObjectIsClass(childobj)) &&
                    (childobj->id && Tcl_Command_nsPtr(childobj->id) == obj->nsPtr)  /* true children */
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

static XOTclObjects *
computeSlotObjects(Tcl_Interp *interp, XOTclObject *obj, char *pattern) {
    XOTclObjects *slotObjects = NULL, **npl = &slotObjects;
    XOTclClasses *pl;
    XOTclObject *childobj, *o;
    Tcl_HashTable slotTable;

    assert(obj);

    Tcl_InitHashTable(&slotTable, TCL_STRING_KEYS);
    MEM_COUNT_ALLOC("Tcl_InitHashTable", slotTable);

    pl = ComputePrecedenceWithMixins(interp, obj, NULL /* pattern*/ );
    for (; pl != 0; pl = pl->nextPtr) {
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
            for (; hPtr != 0; hPtr = Tcl_NextHashEntry(&hSrch)) {
                char *key = Tcl_GetHashKey(cmdTable, hPtr);
                slotEntry = Tcl_CreateHashEntry(&slotTable, key, &new);
                if (!new) continue;
                cmd = (Tcl_Command) Tcl_GetHashValue(hPtr);
                childobj = XOTclGetObjectFromCmdPtr(cmd);
                /* (childobj->id && Tcl_Command_nsPtr(childobj->id) == obj->nsPtr)   true children */
                /*fprintf(stderr,"we have true child obj %s\n", ObjStr(childobj->cmdName));*/
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

static int
ListSlotObjects(Tcl_Interp *interp, XOTclObject *obj, char *pattern) {
    XOTclObjects *pl;
    Tcl_Obj *list = Tcl_NewListObj(0, NULL);

    assert(obj);

    pl = computeSlotObjects(interp, obj, pattern /* not used */ );
    for (; pl != 0; pl = pl->nextPtr) {
        Tcl_ListObjAppendElement(interp, list, pl->obj->cmdName);
    }
    
    XOTclObjectListFree(pl);
    Tcl_SetObjResult(interp, list);

    return TCL_OK;
}



static int
ListParent(Tcl_Interp *interp, XOTclObject *obj) {
    if (obj->id) {
        Tcl_SetResult(interp, NSCmdFullName(obj->id), TCL_VOLATILE);
    }
    return TCL_OK;
}

static XOTclClass*
FindCalledClass(Tcl_Interp *interp, XOTclObject *obj) {
    char *methodName = 0;
    XOTclClass *cl;
    Tcl_Command cmd = NULL;
    XOTclCallStackContent *csc = CallStackGetTopFrame(interp);

    if (csc->frameType == XOTCL_CSC_TYPE_PLAIN)
        return GetSelfClass(interp);

    if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER)
        methodName = ObjStr(csc->filterStackEntry->calledProc);
    else if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_MIXIN && obj->mixinStack)
        methodName = (char *) GetSelfProc(interp);

    if (!methodName) methodName = "";

    if (obj->nsPtr)
        cmd = FindMethod(methodName, obj->nsPtr);

    if (cmd) {
        cl = 0;
    } else {
        cl = SearchCMethod(obj->cl, methodName, &cmd);
    }
    return cl;
}

/*
 * Next Primitive Handling
 */
XOTCLINLINE static void
NextSearchMethod(XOTclObject *obj, Tcl_Interp *interp, XOTclCallStackContent *csc,
		 XOTclClass **cl, char **method, Tcl_ObjCmdProc **proc, Tcl_Command *cmd,
		 ClientData *cp, int *isMixinEntry, int *isFilterEntry,
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
        *cmd = FilterSearchProc(interp, obj, proc, cp, currentCmd, cl);
        /*fprintf(stderr,"EndOfChain? proc=%p, cmd=%p\n",*proc,*cmd);*/
        /*  XOTclCallStackDump(interp); XOTclStackDump(interp);*/

        if (*proc == 0) {
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
        *cmd = MixinSearchProc(interp, obj, *method, cl, proc, cp, currentCmd);
        /*fprintf(stderr,"nextsearch: mixinsearch cmd %p, proc=%p\n",*cmd,*proc);*/
        if (*proc == 0) {
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
        /*fprintf(stderr, "no cmd, cl = %p %s\n",*cl, ObjStr((*cl)->object.cmdName));*/
    } else {
        *cl = 0;
    }

    if (*cmd) {
        *proc = Tcl_Command_objProc(*cmd);
        *cp   = Tcl_Command_objClientData(*cmd);
    }

    return;
}

static int
XOTclNextMethod(XOTclObject *obj, Tcl_Interp *interp, XOTclClass *givenCl,
		char *givenMethod, int objc, Tcl_Obj *CONST objv[],
		int useCallstackObjs) {
    XOTclCallStackContent *csc = CallStackGetTopFrame(interp);
    Tcl_ObjCmdProc *proc = 0;
    Tcl_Command cmd, currentCmd = NULL;
    ClientData cp = 0;
    int result = TCL_OK,
        frameType = XOTCL_CSC_TYPE_PLAIN,
        isMixinEntry = 0, isFilterEntry = 0,
        endOfFilterChain = 0;
    int nobjc; Tcl_Obj **nobjv;
    XOTclClass **cl = &givenCl;
    char **method = &givenMethod;

#if !defined(NDEBUG)
    if (useCallstackObjs) {
        Tcl_CallFrame *cf = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
        int found = 0;
        while (cf) {
            /*      fprintf(stderr, " ... compare fp = %p and cfp %p procFrame %p oc=%d\n",
                    cf, csc->currentFramePtr,
                    Tcl_Interp_framePtr(interp), Tcl_CallFrame_objc(Tcl_Interp_framePtr(interp))
                    );*/
            if (cf == csc->currentFramePtr) {
                found = 1;
                break;
            }
            cf = (Tcl_CallFrame *)((CallFrame *)cf)->callerPtr;
        }
        /*
          if (!found) {
          if (Tcl_Interp_varFramePtr(interp)) {
          fprintf(stderr,"found (csc->currentFramePtr %p)= %d cur level=%d\n",
          csc->currentFramePtr, found,
          Tcl_CallFrame_level(Tcl_Interp_varFramePtr(interp)));
          } else {
          fprintf(stderr,"no varFramePtr\n");
          }
          return TCL_OK;
          }
        */
    }
#endif
  
    /*fprintf(stderr,"Next givenMethod = %s, csc = %p, useCallstackObj %d, objc %d\n", 
      givenMethod, csc, useCallstackObjs, objc);*/

    /* if no args are given => use args from stack */
    if (objc < 2 && useCallstackObjs && csc->currentFramePtr) {
        nobjc = Tcl_CallFrame_objc(csc->currentFramePtr);
        nobjv = (Tcl_Obj **)Tcl_CallFrame_objv(csc->currentFramePtr);
    } else {
        nobjc = objc;
        nobjv = (Tcl_Obj **)objv;
    }

    /*
     * Search the next method & compute its method data
     */
    NextSearchMethod(obj, interp, csc, cl, method, &proc, &cmd, &cp,
                     &isMixinEntry, &isFilterEntry, &endOfFilterChain, &currentCmd);

  
    /*fprintf(stderr, "NextSearchMethod -- RETURN: method=%s eoffc=%d,",
     *method, endOfFilterChain); 

     if (obj)
     fprintf(stderr, " obj=%s,", ObjStr(obj->cmdName));
     if ((*cl))
     fprintf(stderr, " cl=%s,", (*cl)->nsPtr->fullName);
     fprintf(stderr, " mixin=%d, filter=%d, proc=%p\n",
     isMixinEntry, isFilterEntry, proc);
    */

    Tcl_ResetResult(interp); /* needed for bytecode support */

    if (proc != 0) {
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
    
        /*{int i; fprintf(stderr, "N\tCALL ");for(i=0; i<nobjc; i++) {fprintf(stderr,"%s ", ObjStr(nobjv[i]));} fprintf(stderr,"\n");}*/
  
        result = DoCallProcCheck(cp, (ClientData)obj, interp, nobjc, nobjv, cmd,
                                 obj, *cl, *method, frameType, 1/*fromNext*/, 0);

        csc->callType &= ~XOTCL_CSC_CALL_IS_NEXT;
 
        if (csc->frameType == XOTCL_CSC_TYPE_INACTIVE_FILTER)
            csc->frameType = XOTCL_CSC_TYPE_ACTIVE_FILTER;
        else if (csc->frameType == XOTCL_CSC_TYPE_INACTIVE_MIXIN)
            csc->frameType = XOTCL_CSC_TYPE_ACTIVE_MIXIN;
    } else if (result == TCL_OK && endOfFilterChain) {
        /*fprintf(stderr,"setting unknown to 1\n");*/
        RUNTIME_STATE(interp)->unknown = 1;
    }


    return result;
}

int
XOTclNextObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclCallStackContent *csc = CallStackGetTopFrame(interp);

    if (!csc->self)
        return XOTclVarErrMsg(interp, "next: can't find self", (char *) NULL);

    if (!csc->cmdPtr)
        return XOTclErrMsg(interp, "next: no executing proc", TCL_STATIC);

    if (!csc->destroyedCmd && !Tcl_Command_cmdEpoch(csc->cmdPtr)) {
        return XOTclNextMethod(csc->self, interp, csc->cl,
                               (char *)Tcl_GetCommandName(interp, csc->cmdPtr), 
                               objc, objv, 1);
    } else {
        /* also TCL_ERROR would be defensible, but next is as well silent on the "last" next*/
        return TCL_OK; 
    }
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

/* method for calling e.g.  $obj __next  */
static int
XOTclONextMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    XOTclCallStackContent *csc = CallStackGetTopFrame(interp);
    char *methodName;

    for (; csc >= cs->content; csc--) {
        if (csc->self == obj) break;
    }
    if (csc<cs->content)
        return XOTclVarErrMsg(interp, "__next: can't find object",
                              ObjStr(obj->cmdName), (char *) NULL);
    methodName = (char *)Tcl_GetCommandName(interp, csc->cmdPtr);
    /*fprintf(stderr,"******* next for proc %s\n", methodName);*/
    return XOTclNextMethod(obj, interp, csc->cl, methodName, objc-1, &objv[1], 0);
}

#if 0
/* method next for calling e.g.  $obj next  */
static int
XOTclONextMethod2(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    int result, nobjc;
    /*XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;*/
    XOTclCallStackContent *csc = CallStackGetTopFrame(interp);
    Tcl_Obj **nobjv;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");

    /* if no args are given => use args from stack */
    if (objc < 2) {
        nobjc = Tcl_CallFrame_objc(csc->currentFramePtr);
        nobjv = (Tcl_Obj **)Tcl_CallFrame_objv(csc->currentFramePtr);
    } else {
        nobjc = objc;
        nobjv = (Tcl_Obj **)objv;
    }
    {
        ALLOC_ON_STACK(Tcl_Obj *, nobjc + 1, ov);
        memcpy(ov+1, nobjv, sizeof(Tcl_Obj *)*nobjc);
        ov[0] = obj->cmdName;
        result = ObjDispatch(clientData, interp, nobjc+1, ov, 0);
        FREE_ON_STACK(ov);
    }
    /*fprintf(stderr,"******* next for proc %s\n", methodName);*/
    /*result = Tcl_EvalObjv(interp, objc, ov, 0);*/
    return result;
}
#endif

/*
 * "self" object command
 */

static int
FindSelfNext(Tcl_Interp *interp, XOTclObject *obj) {
    XOTclCallStackContent *csc = CallStackGetTopFrame(interp);
    Tcl_ObjCmdProc *proc = 0;
    Tcl_Command cmd, currentCmd = 0;
    ClientData cp = 0;
    int isMixinEntry = 0,
        isFilterEntry = 0,
        endOfFilterChain = 0;
    XOTclClass *cl = csc->cl;
    XOTclObject *o = csc->self;
    char *methodName;

    Tcl_ResetResult(interp);

    methodName = (char *) GetSelfProc(interp);
    if (!methodName)
        return TCL_OK;

    NextSearchMethod(o, interp, csc, &cl, &methodName, &proc, &cmd, &cp,
                     &isMixinEntry, &isFilterEntry, &endOfFilterChain, &currentCmd);

    if (cmd) {
        Tcl_SetObjResult(interp, getFullProcQualifier(interp, Tcl_GetCommandName(interp, cmd), 
                                                      o, cl, cmd));
    }
    return TCL_OK;
}

static Tcl_Obj *
computeLevelObj(Tcl_Interp *interp, CallStackLevel level) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    XOTclCallStackContent *csc;
    Tcl_Obj *resultObj;

    switch (level) {
    case CALLING_LEVEL: csc = XOTclCallStackFindLastInvocation(interp, 1); break;
    case ACTIVE_LEVEL:  csc = XOTclCallStackFindActiveFrame(interp, 1); break;
    default: csc = NULL;
    }

    if (cs->top->currentFramePtr == ((Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp))
        && csc && csc < cs->top && csc->currentFramePtr) {
        /* this was from an xotcl frame, return absolute frame number */
        char buffer[LONG_AS_STRING];
        int l;
        buffer[0] = '#';
        /* fprintf(stderr,"*** csc=%p\n", csc);*/
        XOTcl_ltoa(buffer+1,(long)Tcl_CallFrame_level(csc->currentFramePtr),&l);
        resultObj = Tcl_NewStringObj(buffer, l+1);
    } else {
        /* If not called from an xotcl frame, return 1 as default */
        resultObj = Tcl_NewIntObj(1);
    }
    /*XOTclStackDump(interp);XOTclCallStackDump(interp);*/

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
        csc = CallStackGetTopFrame(interp);
        if (csc->currentFramePtr) {
            CONST char *procName = Tcl_GetCommandName(interp, (Tcl_Command)csc->cmdPtr);
            Tcl_SetResult(interp, (char *)procName, TCL_VOLATILE);
        } else {
            char *procName = (char *)GetSelfProc(interp);
            if (procName) {
                Tcl_SetResult(interp, procName, TCL_VOLATILE);
            } else {
                rc = XOTclVarErrMsg(interp, "Can't find proc", (char *) NULL);
            }
        }
        break;
    } 
    case classIdx: { /* class subcommand */
        XOTclClass *cl = GetSelfClass(interp);
        Tcl_SetObjResult(interp, cl ? cl->object.cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
        break;
    } 
    case activelevelIdx: {
        Tcl_SetObjResult(interp, computeLevelObj(interp, ACTIVE_LEVEL));
        break;
    }
    case argsIdx: {
        int nobjc;
        Tcl_Obj **nobjv;
        csc = CallStackGetTopFrame(interp);
        nobjc = Tcl_CallFrame_objc(csc->currentFramePtr);
        nobjv = (Tcl_Obj **)Tcl_CallFrame_objv(csc->currentFramePtr);
        Tcl_SetObjResult(interp, Tcl_NewListObj(nobjc-1, nobjv+1));
        break;
    }
#if defined(ACTIVEMIXIN)
    case activemixinIdx: {
        XOTclObject *o = NULL;
        csc = CallStackGetTopFrame(interp);
        /*CmdListPrint(interp,"self a....\n", obj->mixinOrder);
          fprintf(stderr,"current cmdPtr = %p cl = %p, mo=%p %p\n", csc->cmdPtr, csc->cl,
          obj->mixinOrder,   RUNTIME_STATE(interp)->cmdPtr);*/
        if (RUNTIME_STATE(interp)->cmdPtr) {
            o = XOTclGetObjectFromCmdPtr(RUNTIME_STATE(interp)->cmdPtr);
        }
        Tcl_SetObjResult(interp, o ? o->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
        break;
    }
#endif
    case calledprocIdx:
    case calledmethodIdx: {
        if (!(csc = CallStackFindActiveFilter(interp)))
            rc = XOTclVarErrMsg(interp, "self ", ObjStr(option)," called from outside of a filter",
                                (char *) NULL);
        else 
            Tcl_SetObjResult(interp, csc->filterStackEntry->calledProc);
        break;

    }
    case calledclassIdx:
        Tcl_SetResult(interp, className(FindCalledClass(interp, obj)), TCL_VOLATILE);
        break;

    case callingprocIdx:
        csc = XOTclCallStackFindLastInvocation(interp, 1);
        Tcl_SetResult(interp, csc ? (char *)Tcl_GetCommandName(interp, csc->cmdPtr) : "",
                      TCL_VOLATILE);
        break;
    
    case callingclassIdx:
        csc = XOTclCallStackFindLastInvocation(interp, 1);
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
        csc = XOTclCallStackFindLastInvocation(interp, 1);
        Tcl_SetObjResult(interp, csc ? csc->self->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
        break;

    case filterregIdx:
        if (!(csc = CallStackFindActiveFilter(interp))) {
            rc = XOTclVarErrMsg(interp,
                                "self filterreg called from outside of a filter",
                                (char *) NULL);
        } else 
            Tcl_SetObjResult(interp, FilterFindReg(interp, obj, GetSelfProcCmdPtr(interp)));
        break;

    case isnextcallIdx: {
        XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
        csc = cs->top;
        csc--;
        Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                          (csc > cs->content &&
                           (csc->callType & XOTCL_CSC_CALL_IS_NEXT)));
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
  XOTclKObjCmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  if (objc < 2)
  return XOTclVarErrMsg(interp, "wrong # of args for K", (char *) NULL);

  Tcl_SetObjResult(interp, objv[1]);
  return TCL_OK;
  }
*/

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
            Tcl_SetObjResult(interp, obj->cmdName);
            rc = TCL_OK;
        } else {
            rc = XOTclVarErrMsg(interp, "self: no current object", (char *) NULL);
        }
    } else {
        rc = XOTclSelfSubCommand(interp, obj, objv[1]);
    }
    return rc;
}


/*
 * object creation & destruction
 */

static int
unsetInAllNamespaces(Tcl_Interp *in, Namespace *nsPtr, char *name) {
    int rc = 0;
    fprintf(stderr, "### unsetInAllNamespaces %s\n", name);
    if (nsPtr != NULL) {
        Tcl_HashSearch search;
        Tcl_HashEntry *entryPtr = Tcl_FirstHashEntry(&nsPtr->childTable, &search);
        Tcl_Var *varPtr;
        int rc = 0;
        
        varPtr = (Tcl_Var *) Tcl_FindNamespaceVar(in, name, (Tcl_Namespace *) nsPtr, 0);
        /*fprintf(stderr, "found %s in %s -> %p\n", name, nsPtr->fullName, varPtr);*/
        if (varPtr) {
            Tcl_DString dFullname, *dsPtr = &dFullname;
            Tcl_DStringInit(dsPtr);
            Tcl_DStringAppend(dsPtr, "unset ", -1);
            Tcl_DStringAppend(dsPtr, nsPtr->fullName, -1);
            Tcl_DStringAppend(dsPtr, "::", 2);
            Tcl_DStringAppend(dsPtr, name, -1);
            /*rc = Tcl_UnsetVar2(in, Tcl_DStringValue(dsPtr), NULL, TCL_LEAVE_ERR_MSG);*/
            rc = Tcl_Eval(in, Tcl_DStringValue(dsPtr));
            /* fprintf(stderr, "fqName = '%s' unset => %d %d\n", Tcl_DStringValue(dsPtr), rc, TCL_OK);*/
            if (rc == TCL_OK) {
                rc = 1;
            } else {
                Tcl_Obj *resultObj = Tcl_GetObjResult(in);
                fprintf(stderr, "   err = '%s'\n", ObjStr(resultObj));
            }
            Tcl_DStringFree(dsPtr);
        }
        
        while (entryPtr != NULL) {
            Namespace *childNsPtr = (Namespace *) Tcl_GetHashValue(entryPtr);
            /*fprintf(stderr, "child = %s\n", childNsPtr->fullName);*/
            entryPtr = Tcl_NextHashEntry(&search);
            rc |= unsetInAllNamespaces(in, childNsPtr, name);
        }
    }
    return rc;
}

static int
freeUnsetTraceVariable(Tcl_Interp *in, XOTclObject *obj) {
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

        rc = Tcl_UnsetVar2(in, obj->opt->volatileVarName, NULL, 0);
        if (rc != TCL_OK) {
            int rc = Tcl_UnsetVar2(in, obj->opt->volatileVarName, NULL, TCL_GLOBAL_ONLY);
            if (rc != TCL_OK) {
                Namespace *nsPtr = (Namespace *) Tcl_GetCurrentNamespace(in);
                if (unsetInAllNamespaces(in, nsPtr, obj->opt->volatileVarName) == 0) {
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
XOTclUnsetTrace(ClientData cd, Tcl_Interp *interp, CONST84 char *name, CONST84 char *name2, int flags)
{
    Tcl_Obj *obj = (Tcl_Obj *)cd;
    XOTclObject *o;
    char *result = NULL;
  
    /*fprintf(stderr,"XOTclUnsetTrace %s flags %x %x\n", name, flags, 
      flags & TCL_INTERP_DESTROYED);  */

    if ((flags & TCL_INTERP_DESTROYED) == 0) {
        if (XOTclObjConvertObject(interp, obj, &o) == TCL_OK) {
            Tcl_Obj *res = Tcl_GetObjResult(interp); /* save the result */
            INCR_REF_COUNT(res);

            /* clear variable, destroy is called from trace */
            if (o->opt && o->opt->volatileVarName) {
                o->opt->volatileVarName = NULL;
            }

            if (callMethod((ClientData)o, interp, XOTclGlobalObjects[XOTE_DESTROY], 2, 0, 0) != TCL_OK) {
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
 * mark an obj on the existing callstack, as not destroyed
 */
static void
UndestroyObj(Tcl_Interp *interp, XOTclObject *obj) {
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    XOTclCallStackContent *csc;

    /*
     * mark the object on the whole callstack as not destroyed
     */
    for (csc = &cs->content[1]; csc <= cs->top; csc++) {
        if (obj == csc->self && csc->destroyedCmd) {
            /*
             * The ref count was incremented, when csc->destroyedCmd
             * was set. We revert this first before forgetting the
             * destroyedCmd.
             */
            if (Tcl_Command_refCount(csc->destroyedCmd) > 1) {
                Tcl_Command_refCount(csc->destroyedCmd)--;
                MEM_COUNT_FREE("command refCount", csc->destroyedCmd);
            }
            csc->destroyedCmd  = 0;
        }
    }
    /*
     * mark obj->flags XOTCL_DESTROY_CALLED as NOT CALLED (0)
     */
    obj->flags &= ~XOTCL_DESTROY_CALLED;
}

/*
 * bring an object into a state, as after initialization
 */
static void
CleanupDestroyObject(Tcl_Interp *interp, XOTclObject *obj, int softrecreate) {
    XOTclClass *thecls, *theobj;

    thecls = RUNTIME_STATE(interp)->theClass;
    theobj = RUNTIME_STATE(interp)->theObject;
    /* remove the instance, but not for ::Class/::Object */
    if (obj != &(thecls->object) && obj != &(theobj->object)) {

        if (!softrecreate) {
            (void)RemoveInstance(obj, obj->cl);
        }
    }

    if (obj->nsPtr) {
        NSCleanupNamespace(interp, obj->nsPtr);
        NSDeleteChildren(interp, obj->nsPtr);
    }

    if (obj->varTable) {
        TclDeleteVars((Interp *)interp, obj->varTable);
        ckfree((char *)obj->varTable);
        /*  FREE(obj->varTable, obj->varTable);*/
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
             *  Remove this object from all mixinof lists and clear the mixin list
             */
            RemoveFromMixinofs(obj->id, opt->mixins);
  
            CmdListRemoveList(&opt->mixins, GuardDel);
            CmdListRemoveList(&opt->filters, GuardDel);

            FREE(XOTclObjectOpt, opt);
            opt = obj->opt = 0;
        }
    }

    if (obj->nonposArgsTable) {
        NonposArgsFreeTable(obj->nonposArgsTable);
        Tcl_DeleteHashTable(obj->nonposArgsTable);
        MEM_COUNT_FREE("Tcl_InitHashTable", obj->nonposArgsTable);
        ckfree((char *) obj->nonposArgsTable);
        MEM_COUNT_FREE("Tcl_HashTable", obj->nonposArgsTable);
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
        obj->nonposArgsTable = 0;
        obj->mixinOrder = 0;
        obj->filterOrder = 0;
        obj->flags = 0;
    }
    /*
      fprintf(stderr, "cleanupInitObject %s: %p cl = %p\n",
      obj->cmdName ? ObjStr(obj->cmdName) : "", obj, obj->cl);*/
}

/*
 * physical object destroy
 */
static void
PrimitiveODestroy(ClientData clientData) {
    XOTclObject *obj = (XOTclObject*)clientData;
    Tcl_Interp *interp;

    /*fprintf(stderr, "****** PrimitiveODestroy %p\n", obj);*/
    assert(obj && !(obj->flags & XOTCL_DESTROYED));

    /*
     * check and latch against recurrent calls with obj->teardown
     */
    PRINTOBJ("PrimitiveODestroy", obj);

    if (!obj || !obj->teardown) return;
    interp = obj->teardown;
    obj->teardown = 0;

    /*
     * Don't destroy, if the interpreter is destroyed already
     * e.g. TK calls Tcl_DeleteInterp directly, if the window is killed
     */
    if (Tcl_InterpDeleted(interp)) return;
    /*
     * call and latch user destroy with obj->id if we haven't
     */
    if (!(obj->flags & XOTCL_DESTROY_CALLED)) {
        callDestroyMethod(clientData, interp, obj, 0);
        obj->id = 0;
    }

#ifdef OBJDELETION_TRACE
    fprintf(stderr,"  physical delete of %p id=%p destroyCalled=%d '%s'\n",
            obj, obj->id, (obj->flags & XOTCL_DESTROY_CALLED), ObjStr(obj->cmdName));
#endif

    CleanupDestroyObject(interp, obj, 0);

    while (obj->mixinStack != NULL)
        MixinStackPop(obj);
    while (obj->filterStack != NULL)
        FilterStackPop(obj);

#if 0
    {
        /* Prevent that PrimitiveODestroy is called more than once.
           This code was used in earlier versions of XOTcl 
           but does not seem necessary any more. If it has to be used
           again in the future, don't use Tcl_GetCommandFromObj()
           in Tcl 8.4.* versions.
        */
        Tcl_Command cmd = Tcl_FindCommand(in, ObjStr(obj->cmdName), 0, 0);

        if (cmd != NULL)
            Tcl_Command_deleteProc(cmd) = 0;
    }
#endif

    if (obj->nsPtr) {
        /*fprintf(stderr,"primitive odestroy calls deletenamespace for obj %p\n", obj);*/
        XOTcl_DeleteNamespace(interp, obj->nsPtr);
        obj->nsPtr = 0;
    }

    /*fprintf(stderr, " +++ OBJ/CLS free: %s\n", ObjStr(obj->cmdName));*/

    obj->flags |= XOTCL_DESTROYED;
    objTrace("ODestroy", obj);
#if REFCOUNT_TRACE
    fprintf(stderr,"ODestroy %p flags %d rc %d destr %d dc %d\n",
	    obj, obj->flags,
	    (obj->flags & XOTCL_REFCOUNTED) != 0,
	    (obj->flags & XOTCL_DESTROYED) != 0,
	    (obj->flags & XOTCL_DESTROY_CALLED) != 0
	    );
#endif
#if REFCOUNTED
    if (!(obj->flags & XOTCL_REFCOUNTED)) {
        DECR_REF_COUNT(obj->cmdName);
    }
#else
    DECR_REF_COUNT(obj->cmdName);
#endif

    XOTclCleanupObject(obj);

#if !defined(NDEBUG)
    if (obj != (XOTclObject*)RUNTIME_STATE(interp)->theClass)
        checkAllInstances(interp, RUNTIME_STATE(interp)->theClass, 0);
#endif
}

static void
PrimitiveOInit(void *mem, Tcl_Interp *interp, char *name, XOTclClass *cl) {
    XOTclObject *obj = (XOTclObject*)mem;

#ifdef OBJDELETION_TRACE
    fprintf(stderr,"+++ PrimitiveOInit\n");
#endif

#ifdef XOTCLOBJ_TRACE
    fprintf(stderr, "OINIT %s = %p\n", name, obj);
#endif
    XOTclObjectRefCountIncr(obj);

    /* if the command of the obj was used before, we have to clean
     * up the callstack from set "destroyedCmd" flags
     */
    UndestroyObj(interp, obj);

    if (Tcl_FindNamespace(interp, name, NULL, 0)) {
        CleanupInitObject(interp, obj, cl,
                          NSGetFreshNamespace(interp, (ClientData)obj, name), 0);
    } else {
        CleanupInitObject(interp, obj, cl, NULL, 0);
    }
    /*obj->flags = XOTCL_MIXIN_ORDER_VALID | XOTCL_FILTER_ORDER_VALID;*/
    obj->mixinStack = 0;
    obj->filterStack = 0;
}

/*
 * Object creation: create object name (full name) and Tcl command
 */
static XOTclObject*
PrimitiveOCreate(Tcl_Interp *interp, char *name, XOTclClass *cl) {
    XOTclObject *obj = (XOTclObject*)ckalloc(sizeof(XOTclObject));
    unsigned length;

    /*fprintf(stderr, "CKALLOC Object %p %s\n", obj, name);*/
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
    if (!NSCheckForParent(interp, name, length)) {
        ckfree((char *) obj);
        return 0;
    }
    obj->id = Tcl_CreateObjCommand(interp, name, XOTclObjDispatch,
                                   (ClientData)obj, PrimitiveODestroy);

    PrimitiveOInit(obj, interp, name, cl);
#if 0
    /*defined(KEEP_TCL_CMD_TYPE)*/
    /*TclNewObj(obj->cmdName);*/
    obj->cmdName = Tcl_NewStringObj(name, length);
    TclSetCmdNameObj(interp, obj->cmdName, (Command*)obj->id);
    /*fprintf(stderr, "new command has name '%s'\n", ObjStr(obj->cmdName));*/
#else
    obj->cmdName = NewXOTclObjectObjName(obj, name, length);
#endif
    INCR_REF_COUNT(obj->cmdName);

    objTrace("PrimitiveOCreate", obj);

    return obj;
}

static XOTclClass *DefaultSuperClass(Tcl_Interp *interp, XOTclClass *cl, XOTclClass *topcl) {
    XOTclObject *obj = (XOTclObject*)cl;
    XOTclClass *defaultClass = topcl;
    if (obj->cl) {
        int result;
        /*fprintf(stderr, "mcl= %s\n", ObjStr(obj->cl->object.cmdName));*/
        result = setInstVar(interp, (XOTclObject *)obj->cl, 
                            XOTclGlobalObjects[XOTE_DEFAULTSUPERCLASS], NULL);
        if (result == TCL_OK) {
            Tcl_Obj *nameObj = Tcl_GetObjResult(interp);
            if (GetXOTclClassFromObj(interp, nameObj, &defaultClass, 0) != TCL_OK) {
                XOTclErrMsg(interp, "default superclass is not a class", TCL_STATIC);
            }
        } else {
            Tcl_Obj *bootstrap = Tcl_GetVar2Ex(interp, "::xotcl::bootstrap", NULL, 
                                               TCL_GLOBAL_ONLY);
            if (bootstrap) {
                Tcl_Obj *nameObj = Tcl_NewStringObj("::xotcl::Object", -1);
                INCR_REF_COUNT(nameObj);
                if (GetXOTclClassFromObj(interp, nameObj, &defaultClass, 0) != TCL_OK) {
                    XOTclErrMsg(interp, "default superclass is not a class", TCL_STATIC);
                }
                DECR_REF_COUNT(nameObj);
            } else {
                /*fprintf(stderr, "no default_superclass for cl %s found, returning %s\n",
                  ObjStr(cl->object.cmdName),
                  ObjStr(defaultClass->object.cmdName));*/
            }
        }
    } else {
        /* during bootstrapping, there might be no meta class defined yet */
        /*fprintf(stderr, "no meta class\n");*/
    }
    return defaultClass;
}

/*
 * Cleanup class: remove filters, mixins, assertions, instances ...
 * and remove class from class hierarchy
 */
static void
CleanupDestroyClass(Tcl_Interp *interp, XOTclClass *cl, int softrecreate) {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr;
    XOTclClass *theobj = RUNTIME_STATE(interp)->theObject;
    XOTclClassOpt *clopt = cl->opt;
    XOTclClass *defaultClass = NULL;

    if (clopt) {
        /*
         *  Remove this class from all instmixinofs and clear the instmixin list
         */
        
        RemoveFromInstmixinsofs(cl->object.id, clopt->instmixins);

        CmdListRemoveList(&clopt->instmixins, GuardDel);
        MixinInvalidateObjOrders(interp, cl);
    
        CmdListRemoveList(&clopt->instfilters, GuardDel);
        FilterInvalidateObjOrders(interp, cl);
        
        /*
         *  Remove this class from all mixin lists and clear the mixinofs list
         */
        
        RemoveFromMixins(cl->object.id, clopt->mixinofs);
        CmdListRemoveList(&clopt->mixinofs, GuardDel);

        /*
         *  Remove this class from all instmixin lists and clear the instmixinofs list
         */
        
        RemoveFromInstmixins(cl->object.id, clopt->instmixinofs);
        CmdListRemoveList(&clopt->instmixinofs, GuardDel);
        
        /* remove dependent filters of this class from all subclasses*/
        FilterRemoveDependentFilterCmds(cl, cl);
        AssertionRemoveStore(clopt->assertions);
#ifdef XOTCL_OBJECTDATA
        XOTclFreeObjectData(cl);
#endif
    }

    Tcl_ForgetImport(interp, cl->nsPtr, "*"); /* don't destroy namespace imported objects */
    NSCleanupNamespace(interp, cl->nsPtr);
    NSDeleteChildren(interp, cl->nsPtr);

    if (!softrecreate) {
        defaultClass = DefaultSuperClass(interp, cl, RUNTIME_STATE(interp)->theObject);
        /* reset all instances to the class ::xotcl::Object, that makes no sense
           for ::Object itself */
        if (cl != theobj) {
            hPtr = &cl->instances ? Tcl_FirstHashEntry(&cl->instances, &hSrch) : 0;
            for (; hPtr != 0; hPtr = Tcl_NextHashEntry(&hSrch)) {
                XOTclObject *inst = (XOTclObject*)Tcl_GetHashKey(&cl->instances, hPtr);
                if (inst && inst != (XOTclObject*)cl && inst->id) {
                    if (inst != &(theobj->object)) {
                        (void)RemoveInstance(inst, cl->object.cl);
                        AddInstance(inst, defaultClass);
                    }
                }
            }
        }
        Tcl_DeleteHashTable(&cl->instances);
        MEM_COUNT_FREE("Tcl_InitHashTable",&cl->instances);
    }

    if (cl->nonposArgsTable) {
        NonposArgsFreeTable(cl->nonposArgsTable);
        Tcl_DeleteHashTable(cl->nonposArgsTable);
        MEM_COUNT_FREE("Tcl_InitHashTable", cl->nonposArgsTable);
        ckfree((char *) cl->nonposArgsTable);
        MEM_COUNT_FREE("Tcl_HashTable", cl->nonposArgsTable);
    }

    if (cl->parameters) {
        DECR_REF_COUNT(cl->parameters);
    }

    if (clopt) {
        if (clopt->parameterClass) {
            DECR_REF_COUNT(clopt->parameterClass);
        }
        FREE(XOTclClassOpt, clopt);
        clopt = cl->opt = 0;
    }

    if (!softrecreate) {
        /*
         * flush all caches, unlink superclasses
         */
    
        FlushPrecedences(cl);
        while (cl->sub) {
            XOTclClass *subClass = cl->sub->cl;
            (void)RemoveSuper(subClass, cl);
            /* if there are no more super classes add the Object
             * class as superclasses
             * -> don't do that for Object itself!
             */
            if (subClass->super == 0 && cl != theobj)
                AddSuper(subClass, defaultClass);
        }
        while (cl->super) 
            (void)RemoveSuper(cl, cl->super->cl);
    }

}


/*
 * do class initialization & namespace creation
 */
static void
CleanupInitClass(Tcl_Interp *interp, XOTclClass *cl, Tcl_Namespace *namespacePtr,
		 int softrecreate) {
    XOTclObject *obj = (XOTclObject*)cl;
    XOTclClass *defaultSuperclass = RUNTIME_STATE(interp)->theObject;

    /* fprintf(stderr,"+++ CleanupInitClass\n"); */
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
    cl->super = 0;
    cl->sub = 0;
    /* Look for a configured default superclass */
    defaultSuperclass = DefaultSuperClass(interp, cl, RUNTIME_STATE(interp)->theObject);

    /*
      if (defaultSuperclass) {
      fprintf(stderr, "default superclass= %s\n", ObjStr(defaultSuperclass->object.cmdName));
      } else {
      fprintf(stderr, "empty super class\n");
      }*/

    AddSuper(cl, defaultSuperclass);
    cl->color = WHITE;
    cl->order = 0;
    cl->parameters = 0;

    if (!softrecreate) {
        Tcl_InitHashTable(&cl->instances, TCL_ONE_WORD_KEYS);
        MEM_COUNT_ALLOC("Tcl_InitHashTable",&cl->instances);
    }

    cl->opt = 0;
    cl->nonposArgsTable = 0;
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
    /*fprintf(stderr,"PrimitiveCDestroy %s flags %x\n", ObjStr(obj->cmdName), obj->flags);*/
    if (!(obj->flags & XOTCL_DESTROY_CALLED))
        /*fprintf(stderr,"PrimitiveCDestroy call destroy\n");*/
        callDestroyMethod(clientData, interp, obj, 0);

    obj->teardown = 0;

    CleanupDestroyClass(interp, cl, 0);

    /*
     * handoff the primitive teardown
     */

    saved = cl->nsPtr;
    obj->teardown = interp;

    /*
     * class object destroy + physical destroy
     */
    /*fprintf(stderr,"primitive cdestroy calls primitive odestroy\n");*/
    PrimitiveODestroy(clientData);

    /*fprintf(stderr,"primitive cdestroy calls deletenamespace for obj %p\n", cl);*/
    saved->clientData = 0;
    XOTcl_DeleteNamespace(interp, saved);

    return;
}

/*
 * class init
 */
static void
PrimitiveCInit(XOTclClass *cl, Tcl_Interp *interp, char *name) {
    TclCallFrame frame, *framePtr = &frame;
    Tcl_Namespace *ns;

    /*
     * ensure that namespace is newly created during CleanupInitClass
     * ie. kill it, if it exists already
     */
    if (Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, 
			  RUNTIME_STATE(interp)->XOTclClassesNS, 0)!= TCL_OK)
        return;
    ns = NSGetFreshNamespace(interp, (ClientData)cl, name);
    Tcl_PopCallFrame(interp);

    CleanupInitClass(interp, cl, ns, 0);
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
    if (!NSCheckForParent(interp, name, length)) {
        ckfree((char *) cl);
        return 0;
    }
    obj->id = Tcl_CreateObjCommand(interp, name, XOTclObjDispatch,
                                   (ClientData)cl, PrimitiveCDestroy);

    PrimitiveOInit(obj, interp, name, class);

    obj->cmdName = NewXOTclObjectObjName(obj, name, length);
    INCR_REF_COUNT(obj->cmdName);
    PrimitiveCInit(cl, interp, name+2);

    objTrace("PrimitiveCCreate", obj);
    return cl;
}

/* change XOTcl class conditionally; obj must not be NULL */

XOTCLINLINE static int
changeClass(Tcl_Interp *interp, XOTclObject *obj, XOTclClass *cl) {
    assert(obj);

    if (cl != obj->cl) {
        if (IsMetaClass(interp, cl) && !IsMetaClass(interp, obj->cl)) {
            return XOTclVarErrMsg(interp, "cannot change class of object ",
                                  ObjStr(obj->cmdName),
                                  " to metaclass ",
                                  ObjStr(cl->object.cmdName),(char *) NULL);
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
doCleanup(Tcl_Interp *interp, XOTclObject *newobj, XOTclObject *classobj,
	  int objc, Tcl_Obj *CONST objv[]) {
    int destroyed = 0, result;
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    XOTclCallStackContent *csc;
    /*
     * we check whether the object to be re-created is destroyed or not
     */
    for (csc = &cs->content[1]; csc <= cs->top; csc++) {
        if (newobj == csc->self && csc->destroyedCmd) {
            destroyed = 1; break;
        }
    }

    if (destroyed)
        UndestroyObj(interp, newobj);

    /*
     *  re-create, first ensure correct class for newobj
     */

    result = changeClass(interp, newobj, (XOTclClass*) classobj);

    if (result == TCL_OK) {
        /*
         * dispatch "cleanup"
         */
        result = callMethod((ClientData) newobj, interp, XOTclGlobalObjects[XOTE_CLEANUP], 2, 0, 0);
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
#ifdef NOT_USED
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
    if ((flags & TCL_GLOBAL_ONLY) != 0) {
        cxtNsPtr = Tcl_GetGlobalNamespace(interp);
    }
    else if (contextNsPtr != NULL) {
        cxtNsPtr = contextNsPtr;
    }
    else {
        cxtNsPtr = Tcl_GetCurrentNamespace(interp);
    }

    TclGetNamespaceForQualName(interp, name, (Namespace *) contextNsPtr,
                               flags, &nsPtr[0], &nsPtr[1], &cxtNsPtr, &simpleName);

    /*fprintf(stderr, "  ***Found %s, %s\n", nsPtr[0]->fullName, nsPtr[0]->fullName);*/

    /*
     * Look for the command in the command table of its namespace.
     * Be sure to check both possible search paths: from the specified
     * namespace context and from the global namespace.
     */

    cmd = NULL;
    for (search = 0;  (search < 2) && (cmd == NULL);  search++) {
        if ((nsPtr[search] != NULL) && (simpleName != NULL)) {
            cmdTable = Tcl_Namespace_cmdTable(nsPtr[search]);
            entryPtr = Tcl_FindHashEntry(cmdTable, simpleName);
            if (entryPtr != NULL) {
                cmd = (Tcl_Command) Tcl_GetHashValue(entryPtr);
            }
        }
    }
    if (cmd != NULL) {
        Tcl_ObjCmdProc *objProc = Tcl_Command_objProc(cmd);
        if (cxtNsPtr->deleteProc == NSNamespaceDeleteProc &&
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
            if ((nsPtr[0] != NULL) && (simpleName != NULL)) {
                cmdTable = Tcl_Namespace_cmdTable(nsPtr[0]);
                if ((entryPtr = Tcl_FindHashEntry(cmdTable, simpleName))) {
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

/*
 * object method implementations
 */

static int
XOTclODestroyMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 1) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], NULL);
    PRINTOBJ("XOTclODestroyMethod", obj);

    /*
     * call instdestroy for [self]
     */
    return XOTclCallMethodWithArgs((ClientData)obj->cl, interp,
                                   XOTclGlobalObjects[XOTE_INSTDESTROY], obj->cmdName,
                                   objc, objv+1, 0);
}

static int
XOTclOCleanupMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    XOTclClass  *cl  = XOTclObjectToClass(obj);
    char *fn;
    int softrecreate;
    Tcl_Obj *savedNameObj;

#if defined(OBJDELETION_TRACE)
    fprintf(stderr,"+++ XOTclOCleanupMethod\n");
#endif

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 1) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], NULL);
    PRINTOBJ("XOTclOCleanupMethod", obj);

    fn = ObjStr(obj->cmdName);
    savedNameObj = obj->cmdName;
    INCR_REF_COUNT(savedNameObj);

    /* save and pass around softrecreate*/
    softrecreate = obj->flags & XOTCL_RECREATE&& RUNTIME_STATE(interp)->doSoftrecreate;

    CleanupDestroyObject(interp, obj, softrecreate);
    CleanupInitObject(interp, obj, obj->cl, obj->nsPtr, softrecreate);

    if (cl) {
        CleanupDestroyClass(interp, cl, softrecreate);
        CleanupInitClass(interp, cl, cl->nsPtr, softrecreate);
    }

    DECR_REF_COUNT(savedNameObj);

    return TCL_OK;
}

static int
XOTclOIsClassMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    Tcl_Obj *className;
    XOTclObject *obj = (XOTclObject*)clientData, *o;
    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 1 || objc > 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], 
                                                       "?className?");
    className = (objc == 2) ? objv[1] : obj->cmdName;

    Tcl_SetIntObj(Tcl_GetObjResult(interp), 
                  (XOTclObjConvertObject(interp, className, &o) == TCL_OK
                   && XOTclObjectIsClass(o) ));
    return TCL_OK;
}

static int
XOTclOIsObjectMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData, *o;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "<objName>");

    if (XOTclObjConvertObject(interp, objv[1], &o) == TCL_OK) {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    } else {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
    }
    return TCL_OK;
}

static int
IsMetaClass(Tcl_Interp *interp, XOTclClass *cl) {
    /* check if cl is a meta-class by checking is Class is a superclass of cl*/
    XOTclClasses *pl, *checkList=0, *mixinClasses = 0, *mc;
    int hasMCM = 0;

    if (cl == RUNTIME_STATE(interp)->theClass)
        return 1;

    for (pl = ComputeOrder(cl, cl->order, Super); pl; pl = pl->nextPtr) {
        if (pl->cl == RUNTIME_STATE(interp)->theClass)
            return 1;
    }

    for (pl = ComputeOrder(cl, cl->order, Super); pl; pl = pl->nextPtr) {
        XOTclClassOpt *clopt = pl->cl->opt;
        if (clopt && clopt->instmixins) {
            MixinComputeOrderFullList(interp,
                                      &clopt->instmixins,
                                      &mixinClasses,
                                      &checkList, 0);
        }
    }

    for (mc=mixinClasses; mc; mc = mc->nextPtr) {
        /*fprintf(stderr,"- got %s\n", ObjStr(mc->cl->object.cmdName));*/
        if (isSubType(mc->cl, RUNTIME_STATE(interp)->theClass)) {
            hasMCM = 1;
            break;
        }
    }
    XOTclClassListFree(mixinClasses);
    XOTclClassListFree(checkList);
    /*fprintf(stderr,"has MC returns %d, mixinClasses = %p\n",
      hasMCM, mixinClasses);*/

    return hasMCM;
}

static int
XOTclOIsMetaClassMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData, *o;
    Tcl_Obj *className;
    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 1 || objc > 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], 
                                                       "?metaClassName?");

    className = (objc == 2) ? objv[1] : obj->cmdName;

    if (XOTclObjConvertObject(interp, className, &o) == TCL_OK
        && XOTclObjectIsClass(o)
        && IsMetaClass(interp, (XOTclClass*)o)) {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    } else {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);
    }
    return TCL_OK;
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
XOTclOIsTypeMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    XOTclClass *cl;
    int success = 0;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "<className>");
    if (obj->cl && GetXOTclClassFromObj(interp, objv[1], &cl, 0) == TCL_OK) {
        success = isSubType(obj->cl, cl);
    }
    Tcl_ResetResult(interp);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), success);
    return TCL_OK;
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
        success = (XOTclObjConvertObject(interp, objv[1], &obj) == TCL_OK 
                   && GetXOTclClassFromObj(interp, objv[3], &cl, 0) == TCL_OK
                   && isSubType(obj->cl, cl));
        break;

    case objectIdx: 
        if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "object <object>");
        success = (XOTclObjConvertObject(interp, objv[1], &obj) == TCL_OK);
        break;

    case classIdx: 
        if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "class <class>");
        success = (GetXOTclClassFromObj(interp, objv[1], &cl, 0) == TCL_OK);
        break;

    case metaclassIdx:
        if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "metaclass <class>");

        success = (XOTclObjConvertObject(interp, objv[1], &obj) == TCL_OK
                   && XOTclObjectIsClass(obj)
                   && IsMetaClass(interp, (XOTclClass*)obj));
        break;

    case mixinIdx:
        if (objc != 4) return XOTclObjErrArgCnt(interp, objv[0], NULL, "mixin <object> <class>");

        success = (XOTclObjConvertObject(interp, objv[1], &obj) == TCL_OK 
                   && GetXOTclClassFromObj(interp, objv[3], &cl, 0) == TCL_OK 
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

static int
XOTclOIsMixinMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    XOTclClass *cl;
    int success = 0;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "<className>");

    if (GetXOTclClassFromObj(interp, objv[1],&cl, 0) == TCL_OK) {
        success = hasMixin(interp, obj, cl);
    }
    Tcl_ResetResult(interp);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), success);
    return TCL_OK;
}

static int
XOTclOExistsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "var");

    Tcl_SetIntObj(Tcl_GetObjResult(interp), 
                  varExists(interp, obj, ObjStr(objv[1]), NULL, 1, 1));
    return TCL_OK;
}

static int
countModifiers(int objc, Tcl_Obj  *CONST objv[]) {
    int i, count = 0;
    char *to;
    for (i = 2; i < objc; i++) {
        to = ObjStr(objv[i]);
        if (to[0] == '-') {
            count++;
            /* '--' stops modifiers */
            if (to[1] == '-') break;
        }
    }
    return count;
}

static int
checkForModifier(Tcl_Obj  *CONST objv[], int numberModifiers, char *modifier) {
    int i;
    if (numberModifiers == 0) return 0;
    for (i = 2; i-2 < numberModifiers; i++) {
        char *ov = ObjStr(objv[i]);
        /* all start with a "-", so there must be a ov[1] */
        if (ov[1] == modifier[1] && !strcmp(ov, modifier))
            return 1;
    }
    return 0;
}

/************ info commands xxx ******/

static int
XOTclObjInfoArgsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    Tcl_Namespace *nsp;
  
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<object> <methodName>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    if (obj->nonposArgsTable) {
        XOTclNonposArgs *nonposArgs =  NonposArgsGet(obj->nonposArgsTable, ObjStr(objv[2]));
        if (nonposArgs && nonposArgs->ordinaryArgs) {
            return ListArgsFromOrdinaryArgs(interp, nonposArgs);
        }
    }

    nsp = obj->nsPtr;
    return nsp ? ListProcArgs(interp, Tcl_Namespace_cmdTable(nsp), ObjStr(objv[2])) : TCL_OK;
}

static int
XOTclObjInfoBodyMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    Tcl_Namespace *nsp;
  
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> <methodName>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");
  
    nsp = obj->nsPtr;
    return nsp ?  ListProcBody(interp, Tcl_Namespace_cmdTable(nsp), ObjStr(objv[2])) : TCL_OK;
}

static int
XOTclObjInfoClassMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;

    if (objc != 2) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object>"); 
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");
  
    Tcl_SetObjResult(interp, obj->cl->object.cmdName);
    return TCL_OK;
}

static int
XOTclObjInfoCommandsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    Tcl_Namespace *nsp;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> ?pattern?");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    nsp = obj->nsPtr;
    return ListKeys(interp, Tcl_Namespace_cmdTable(nsp), objc == 3 ? ObjStr(objv[2]) : NULL);
}

static int
XOTclObjInfoChildrenMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> ?pattern?");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    return ListChildren(interp, obj, objc == 3 ? ObjStr(objv[2]) : NULL, 0);
}

static int
XOTclObjInfoSlotObjectsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> ?pattern?");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    return ListSlotObjects(interp, obj, objc == 3 ? ObjStr(objv[2]) : NULL);
}

static int
XOTclObjInfoCheckMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;

    if (objc != 2) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    return AssertionListCheckOption(interp, obj);  
}

static int
XOTclObjInfoDefaultMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    Tcl_Namespace *nsp;

    if (objc != 5) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> <methodName> <arg> <var>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    if (obj->nonposArgsTable) {
        XOTclNonposArgs *nonposArgs = NonposArgsGet(obj->nonposArgsTable,  ObjStr(objv[2]));
        if (nonposArgs && nonposArgs->ordinaryArgs) {
            return ListDefaultFromOrdinaryArgs(interp, ObjStr(objv[2]), nonposArgs, ObjStr(objv[3]), objv[4]);
        }
    }
    nsp = obj->nsPtr;
    return nsp ? 
        ListProcDefault(interp, Tcl_Namespace_cmdTable(nsp), ObjStr(objv[2]), ObjStr(objv[3]), objv[4]) : 
        TCL_OK;
}

static int
XOTclObjInfoFilterMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    XOTclObjectOpt *opt;;
    int idx, nobjc, withGuards = 0, withOrder = 0;
    static CONST char *options[] = {"-guards", "-order", NULL};
    enum options {guardsIdx, orderIdx};

    for (idx = 2; idx < objc; idx++) {
        char *name;
        int index;

        name = Tcl_GetString(objv[idx]);
        if (name[0] != '-') {
            break;
        }
        if (Tcl_GetIndexFromObj(interp, objv[idx], options, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        switch ((enum options) index) {
        case guardsIdx: withGuards = 1; break;
        case orderIdx:  withOrder = 1;  break;
        }
    }
    nobjc = objc - idx;
  
    if (objc < 2 || nobjc > 1 || objc > 4)
        return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> ?-guards|-order? ?pattern?");

    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    if (withOrder) { 
        if (!(obj->flags & XOTCL_FILTER_ORDER_VALID))
            FilterComputeDefined(interp, obj);
        return FilterInfo(interp, obj->filterOrder, idx < objc ? ObjStr(objv[idx]) : NULL, withGuards, 1);
    }
    opt = obj->opt;
    return opt ? FilterInfo(interp, opt->filters, idx < objc ? ObjStr(objv[idx]) : NULL, withGuards, 0) : TCL_OK;
}

static int
XOTclObjInfoFilterguardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    XOTclObjectOpt *opt;
  
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> filter");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    opt = obj->opt;
    return opt ? GuardList(interp, opt->filters, ObjStr(objv[2])) : TCL_OK;
}

static int
XOTclObjInfoForwardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    Tcl_Namespace *nsp;
    int withDefinition = 0;
    char *name = NULL;

    if (objc < 2 || objc > 4) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> ?-definition? ?name?");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    if (objc > 2) {
        name = ObjStr(objv[2]);
        if (*name == '-' && !strcmp("-definition", name)) {
            withDefinition = 1;
            name = ObjStr(objv[3]);
        }
    }

    nsp = obj->nsPtr;
    return nsp ? forwardList(interp, Tcl_Namespace_cmdTable(nsp), name, withDefinition) : TCL_OK;
}

static int
XOTclObjInfoHasnamespaceMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    Tcl_Namespace *nsp;;

    if (objc != 2) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    nsp = obj->nsPtr;
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), nsp != NULL);  
    return TCL_OK;
}

static int
XOTclObjInfoInfoMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;

    if (objc != 2) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    return ListInfo(interp, GetXOTclClassFromObj(interp, obj->cmdName, NULL, 0) == TCL_OK);
}

static int
XOTclObjInfoInvarMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    XOTclObjectOpt *opt;

    if (objc != 2) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    opt = obj->opt;
    if (opt && opt->assertions) {
        Tcl_SetObjResult(interp, AssertionList(interp, opt->assertions->invariants));
    }
    return TCL_OK;
}

static int
XOTclObjInfoMixinMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    XOTclObjectOpt *opt;
    int idx, nobjc, withGuards = 0, withOrder = 0;
    static CONST char *options[] = {"-guards", "-order", NULL};
    enum options {guardsIdx, orderIdx};

    for (idx = 2; idx < objc; idx++) {
        char *name;
        int index;

        name = Tcl_GetString(objv[idx]);
        if (name[0] != '-') {
            break;
        }
        if (Tcl_GetIndexFromObj(interp, objv[idx], options, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        switch ((enum options) index) {
        case guardsIdx: withGuards = 1; break;
        case orderIdx:  withOrder = 1;  break;
        }
    }
    nobjc = objc-idx;
  
    if (objc < 2 || nobjc > 1 || objc > 4)
        return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> ?-guards|-order? ?pattern?");

    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    if (withOrder) {
        if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
            MixinComputeDefined(interp, obj);
        return MixinInfo(interp, obj->mixinOrder, idx < objc ? ObjStr(objv[idx]) : NULL, withGuards);
    }
  
    opt = obj->opt;
    return opt ? MixinInfo(interp, opt->mixins, idx < objc ? ObjStr(objv[idx]) : NULL, withGuards) : TCL_OK;
}

static int
XOTclObjInfoMixinguardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    XOTclObjectOpt *opt;

    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> mixin");    
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    opt = obj->opt;
    return opt ? GuardList(interp, opt->mixins, ObjStr(objv[2])) : TCL_OK;
}

static int
XOTclObjInfoMethodsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    int idx, nobj, noprocs = 0, nocmds = 0, nomixins = 0, inContext = 0;
    static CONST char *options[] = {"-noprocs", "-nocmds", "-nomixins", "-incontext", NULL};
    enum options {noprocsIdx, nocmdsIdx, nomixinsIdx, incontextIdx};

    for (idx = 2; idx < objc; idx++) {
        char *name;
        int index;

        name = Tcl_GetString(objv[idx]);
        if (name[0] != '-') {
            break;
        }
        if (Tcl_GetIndexFromObj(interp, objv[idx], options, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        switch ((enum options) index) {
        case noprocsIdx:   noprocs = 1; break;
        case nocmdsIdx:    nocmds = 1;  break;
        case nomixinsIdx:  nomixins = 1;  break;
        case incontextIdx: inContext = 1; break;
        }
    }
    nobj = objc-idx;
  
    if (nobj > 1)
        return XOTclObjErrArgCnt(interp, objv[0], NULL, 
                                 "<object> ?-noprocs? ?-nocmds? ?-nomixins? ?-incontext? ?pattern?");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    return ListMethods(interp, obj, nobj == 1 ? ObjStr(objv[idx]) : NULL, 
                       noprocs, nocmds, nomixins, inContext);
}

static int
XOTclObjInfoNonposargsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
  
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], objv[1], "<methodName>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    if (obj->nonposArgsTable) {
        XOTclNonposArgs *nonposArgs = NonposArgsGet(obj->nonposArgsTable, ObjStr(objv[2]));
        if (nonposArgs) {
            Tcl_SetObjResult(interp, NonposArgsFormat(interp, nonposArgs->nonposArgs));
        }
    }
    return TCL_OK;
}

static int
XOTclObjInfoProcsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    Tcl_Namespace *nsp;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> ?pattern?");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    nsp = obj->nsPtr;
    return nsp ? ListMethodKeys(interp, Tcl_Namespace_cmdTable(nsp), 
                                objc == 3 ? ObjStr(objv[2]) : NULL,
                                /*noProcs*/ 0, /*noCmds*/ 1, NULL, 0 ) : TCL_OK;
}

static int
XOTclObjInfoParentMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;

    if (objc != 2) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");
  
    return ListParent(interp, obj);
}

static int
XOTclObjInfoPreMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    XOTclObjectOpt *opt;

    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> <methodName>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    opt = obj->opt;
    if (opt) {
        XOTclProcAssertion *procs = AssertionFindProcs(opt->assertions, ObjStr(objv[2]));
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->pre));
    }
    return TCL_OK;
}

static int
XOTclObjInfoPostMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;
    XOTclObjectOpt *opt;

    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> <methodName>");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");

    opt = obj->opt;
    if (opt) {
        XOTclProcAssertion *procs = AssertionFindProcs(opt->assertions, ObjStr(objv[2]));
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->post));
    }
    return TCL_OK;
}

static int
XOTclObjInfoPrecedenceMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> ?pattern?");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");
  
    return ListPrecedence(interp, obj, objc == 3 ? ObjStr(objv[2]) : NULL);
}

static int
XOTclObjInfoVarsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<object> ?pattern?");  
    if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Object");
  
    return ListVars(interp, obj, objc == 3 ? ObjStr(objv[2]) : NULL);
}


static int
XOTclOInfoMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    Tcl_Namespace *nsp = obj->nsPtr;
    char *cmd, *pattern;
    int modifiers = 0;
    XOTclObjectOpt *opt;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 2)
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "<opt> ?args?");

    opt = obj->opt;
    cmd = ObjStr(objv[1]);
    pattern = (objc > 2) ? ObjStr(objv[2]) : 0;

    /*fprintf(stderr, "OInfo cmd=%s, obj=%s, nsp=%p\n", cmd, ObjStr(obj->cmdName), nsp);*/

    /*
     * check for "-" modifiers
     */
    if (pattern && *pattern == '-') {
        modifiers = countModifiers(objc, objv);
        pattern = (objc > 2+modifiers) ? ObjStr(objv[2+modifiers]) : 0;
    }

    switch (*cmd) {
    case 'a':
        if (isArgsString(cmd)) {
            if (objc != 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "args <proc>");
            if (obj->nonposArgsTable) {
                XOTclNonposArgs *nonposArgs =
                    NonposArgsGet(obj->nonposArgsTable, pattern);
                if (nonposArgs && nonposArgs->ordinaryArgs) {
                    return ListArgsFromOrdinaryArgs(interp, nonposArgs);
                }
            }
            if (nsp)
                return ListProcArgs(interp, Tcl_Namespace_cmdTable(nsp), pattern);
            else
                return TCL_OK;
        }
        break;

    case 'b':
        if (!strcmp(cmd, "body")) {
            if (objc != 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "body <proc>");
            if (nsp)
                return ListProcBody(interp, Tcl_Namespace_cmdTable(nsp), pattern);
            else
                return TCL_OK;
        }
        break;

    case 'c':
        if (isClassString(cmd)) {
            if (objc > 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "class ?class?");
            return ListClass(interp, obj, pattern, objc, objv);
        } else if (!strcmp(cmd, "commands")) {
            if (objc > 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "commands ?pat?");
            if (nsp)
                return ListKeys(interp, Tcl_Namespace_cmdTable(nsp), pattern);
            else
                return TCL_OK;
        } else if (!strcmp(cmd, "children")) {
            if (objc > 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "children ?pat?");
            return ListChildren(interp, obj, pattern, 0);
        } else if (!strcmp(cmd, "check")) {
            if (objc != 2 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "check");
            return AssertionListCheckOption(interp, obj);
        }
        break;

    case 'd':
        if (!strcmp(cmd, "default")) {
            if (objc != 5 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "default <proc> <arg> <var>");

            if (obj->nonposArgsTable) {
                XOTclNonposArgs *nonposArgs =
                    NonposArgsGet(obj->nonposArgsTable, pattern);
                if (nonposArgs && nonposArgs->ordinaryArgs) {
                    return ListDefaultFromOrdinaryArgs(interp, pattern,  nonposArgs,
                                                       ObjStr(objv[3]), objv[4]);
                }
            }
            if (nsp)
                return ListProcDefault(interp, Tcl_Namespace_cmdTable(nsp), pattern,
                                       ObjStr(objv[3]), objv[4]);
            else
                return TCL_OK;
        }
        break;

    case 'f':
        if (!strcmp(cmd, "filter")) {
            int withGuards = 0, withOrder = 0;
            if (objc-modifiers > 3)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], 
                                         "filter ?-guards? ?-order? ?pat?");
            if (modifiers > 0) {
                withGuards = checkForModifier(objv, modifiers, "-guards");
                withOrder = checkForModifier(objv, modifiers, "-order");

                if (withGuards == 0 && withOrder == 0)
                    return XOTclVarErrMsg(interp, "info filter: unknown modifier ",
                                          ObjStr(objv[2]), (char *) NULL);
                /*
                  if (withGuards && withOrder)
                  return XOTclVarErrMsg(interp, "info filter: cannot use -guards and -order together",
                  ObjStr(objv[2]), (char *) NULL);
                */
            }

            if (withOrder) {
                if (!(obj->flags & XOTCL_FILTER_ORDER_VALID))
                    FilterComputeDefined(interp, obj);
                return FilterInfo(interp, obj->filterOrder, pattern, withGuards, 1);
            }

            return opt ? FilterInfo(interp, opt->filters, pattern, withGuards, 0) : TCL_OK;

        } else if (!strcmp(cmd, "filterguard")) {
            if (objc != 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "filterguard filter");
            return opt ? GuardList(interp, opt->filters, pattern) : TCL_OK;
        } else if (!strcmp(cmd, "forward")) {
            int argc = objc-modifiers;
            int definition;
            if (argc < 2 || argc > 3)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], 
                                         "forward ?-definition? ?name?");
            definition = checkForModifier(objv, modifiers, "-definition");
            if (nsp)
                return forwardList(interp, Tcl_Namespace_cmdTable(nsp), pattern, definition);
            else
                return TCL_OK;
        }

        break;

    case 'h':
        if (!strcmp(cmd, "hasNamespace")) {
            Tcl_SetBooleanObj(Tcl_GetObjResult(interp), nsp != NULL);
            return TCL_OK;
        }
        break;

    case 'i':
        if (!strcmp(cmd, "invar")) {
            if (objc != 2 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "invar");
            if (opt && opt->assertions)
                Tcl_SetObjResult(interp, AssertionList(interp, opt->assertions->invariants));
            return TCL_OK;
        } else if (!strcmp(cmd, "info")) {
            if (objc > 2 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "info");
            return ListInfo(interp, GetXOTclClassFromObj(interp, obj->cmdName, NULL, 0) == TCL_OK);
        }
        break;

    case 'm':
        if (!strcmp(cmd, "mixin")) {
            int withOrder = 0, withGuards = 0;
            if (objc-modifiers > 3)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], 
                                         "mixin ?-guards? ?-order? ?class?");
            if (modifiers > 0) {
                withOrder = checkForModifier(objv, modifiers, "-order");
                withGuards = checkForModifier(objv, modifiers, "-guards");

                if (withOrder == 0 && withGuards == 0)
                    return XOTclVarErrMsg(interp, "info mixin: unknown modifier . ",
                                          ObjStr(objv[2]), (char *) NULL);
            }

            if (withOrder) {
                if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
                    MixinComputeDefined(interp, obj);
                return MixinInfo(interp, obj->mixinOrder, pattern, withGuards);
            }

            return opt ? MixinInfo(interp, opt->mixins, pattern, withGuards) : TCL_OK;

        } else if (!strcmp(cmd, "mixinguard")) {
            if (objc != 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "mixinguard mixin");

            return opt ? GuardList(interp, opt->mixins, pattern) : TCL_OK;

        } else if (!strcmp(cmd, "methods")) {
            int noprocs = 0, nocmds = 0, nomixins = 0, inContext = 0;
            if (objc-modifiers > 3)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], 
                                         "methods ?-noprocs? ?-nocmds? ?-nomixins? ?-incontext? ?pat?");
            if (modifiers > 0) {
                noprocs = checkForModifier(objv, modifiers, "-noprocs");
                nocmds = checkForModifier(objv, modifiers, "-nocmds");
                nomixins = checkForModifier(objv, modifiers, "-nomixins");
                inContext = checkForModifier(objv, modifiers, "-incontext");
            }
            return ListMethods(interp, obj, pattern, noprocs, nocmds, nomixins, inContext);
        }
#ifdef XOTCL_METADATA
        else  if (!strcmp(cmd, "metadata")) {
            if (objc > 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "metadata ?pat?");
            return ListKeys(interp, &obj->metaData, pattern);
        }
#endif
        break;

    case 'n':
        if (!strcmp(cmd, "nonposargs")) {
            if (objc != 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "nonposargs <proc>");
            if (obj->nonposArgsTable) {
                XOTclNonposArgs *nonposArgs =
                    NonposArgsGet(obj->nonposArgsTable, pattern);
                if (nonposArgs) {
                    Tcl_SetObjResult(interp, NonposArgsFormat(interp, nonposArgs->nonposArgs));
                }
            }
            return TCL_OK;
        }
        break;

    case 'p':
        if (!strcmp(cmd, "procs")) {
            if (objc > 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "procs ?pat?");
            if (nsp)
                return ListMethodKeys(interp, Tcl_Namespace_cmdTable(nsp), pattern,
                                      /*noProcs*/ 0, /*noCmds*/ 1, NULL, 0 );
            else
                return TCL_OK;
        } else if (!strcmp(cmd, "parent")) {
            if (objc > 2 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "parent");
            return ListParent(interp, obj);
        } else if (!strcmp(cmd, "pre")) {
            XOTclProcAssertion *procs;
            if (objc != 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "pre <proc>");
            if (opt) {
                procs = AssertionFindProcs(opt->assertions, ObjStr(objv[2]));
                if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->pre));
            }
            return TCL_OK;
        } else if (!strcmp(cmd, "post")) {
            XOTclProcAssertion *procs;
            if (objc != 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "post <proc>");
            if (opt) {
                procs = AssertionFindProcs(opt->assertions, ObjStr(objv[2]));
                if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->post));
            }
            return TCL_OK;
        } else if (!strcmp(cmd, "precedence")) {
            return ListPrecedence(interp, obj, pattern);
        }
        break;

    case 'v':
        if (!strcmp(cmd, "vars")) {
            if (objc > 3 || modifiers > 0)
                return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "vars ?pat?");
            return ListVars(interp, obj, pattern);
        }
        break;
    }
    return XOTclErrBadVal(interp, "info",
                          "an info option (use 'info info' to list all info options)", cmd);
}


static int
XOTclOProcMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj  *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    char *argStr, *bdyStr, *name;
    XOTclObjectOpt *opt;
    int incr = 0, result = TCL_OK;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 4 || objc > 7)
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], 
                                 "name ?non-positional-args? args body ?preAssertion postAssertion?");

    if (objc == 5 || objc == 7) {
        incr = 1;
    }

    argStr = ObjStr(objv[2 + incr]);
    bdyStr = ObjStr(objv[3 + incr]);
    name = ObjStr(objv[1 + incr]);

    if (*argStr == 0 && *bdyStr == 0) {
        opt = obj->opt;
        if (opt)
            AssertionRemoveProc(opt->assertions, name);
        if (obj->nsPtr)
            NSDeleteCmd(interp, obj->nsPtr, name);
    } else {
        XOTclAssertionStore *aStore = NULL;
        if (objc > 5) {
            opt = XOTclRequireObjectOpt(obj);
            if (!opt->assertions)
                opt->assertions = AssertionCreateStore();
            aStore = opt->assertions;
        }
        result = MakeProc(requireObjNamespace(interp, obj), aStore, &(obj->nonposArgsTable),
                          interp, objc, (Tcl_Obj **) objv, obj, 0);
    }

    /* could be a filter => recompute filter order */
    FilterComputeDefined(interp, obj);

    return result;
}

static int
XOTclONoinitMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj  *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 1) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], NULL);

    obj->flags |= XOTCL_INIT_CALLED;

    return TCL_OK;
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
    XOTclClass *cl = (XOTclClass*) class;
    int result;
    INCR_REF_COUNT(name);
    result = XOTclCallMethodWithArgs((ClientData)cl, interp,
                                     XOTclGlobalObjects[XOTE_CREATE], name, 1, 0, 0);
    DECR_REF_COUNT(name);
    return result;
}

extern int
XOTclCreate(Tcl_Interp *interp, XOTcl_Class *class, Tcl_Obj *name, ClientData data,
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
    result = createMethod(interp, (XOTclClass *)cl, data, objc+2, ov);

    FREE_ON_STACK(ov);
    DECR_REF_COUNT(name);

    return result;
}

int
XOTclDeleteObject(Tcl_Interp *interp, XOTcl_Object *obji) {
    XOTclObject *obj = (XOTclObject*) obji;
    return callMethod((ClientData)obj, interp, XOTclGlobalObjects[XOTE_DESTROY], 2, 0, 0);
}

int
XOTclDeleteClass(Tcl_Interp *interp, XOTcl_Class *cli) {
    XOTclClass *cl = (XOTclClass*) cli;
    return callMethod((ClientData)cl, interp, XOTclGlobalObjects[XOTE_DESTROY], 2, 0, 0);
}

extern int
XOTclUnsetInstVar2(XOTcl_Object *obji, Tcl_Interp *interp, char *name1, char *name2,
                   int flgs) {
    XOTclObject *obj = (XOTclObject*) obji;
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
    int new;
    Tcl_CallFrame *varFramePtr;
    TclVarHashTable *tablePtr;
    XOTcl_FrameDecls;

    int flgs = TCL_LEAVE_ERR_MSG |
        /* PARSE_PART1 needed for 8.0.5 */ TCL_PARSE_PART1;
#if 0
    /* why do we need to deal with vars with namepaces paths? */
    if (newName && strstr(varName, "::")) {
        /* the source variable name contains a namespace path. to locate it, we need a namespace */
        requireObjNamespace(interp, obj);
    }
    /*fprintf(stderr,"GetIntoScope obj=%s ns=%p newName=%s\n", ObjStr(obj->cmdName), obj->nsPtr, newName);*/
#endif

    XOTcl_PushFrame(interp, obj);
    if (obj->nsPtr) {
        flgs = flgs|TCL_NAMESPACE_ONLY;
    }

    otherPtr = XOTclObjLookupVar(interp, varName, (char *) NULL, flgs, "define",
                                 /*createPart1*/ 1, /*createPart2*/ 1, &arrayPtr);
    XOTcl_PopFrame(interp, obj);

    if (otherPtr == NULL) {
        return XOTclVarErrMsg(interp, "can't make instvar ", ObjStr(varName),
                              ": can't find variable on ", ObjStr(obj->cmdName), 
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
                                  " on ", ObjStr(obj->cmdName),
                                  ": variable cannot be an element in an array;",
                                  " use an alias or objeval.", (char *) NULL);
        }

        newName = varName;
    }
#if 0
    /* fprintf(stderr,"ERRORMSG=%s newName='%s' array %p, othervar %p\n",
       ObjStr(Tcl_GetObjResult(interp)), newName, arrayPtr, otherPtr);
    */
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
    if (varFramePtr != NULL && Tcl_CallFrame_isProcCallFrame(varFramePtr)) {
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
		  panic("new linkvar %s... When does this happen?", newNameString, NULL);*/
         
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
          ObjStr(newName), ObjStr(obj->cmdName), forwardCompatibleMode,
          varFlags(varPtr), 
          TclIsVarLink(varPtr), TclIsVarTraced(varPtr), TclIsVarUndefined(varPtr));
          }
	*/
    }
    return TCL_OK;
}

static int
XOTclOInstVarMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

extern int
XOTclInstVar(XOTcl_Object *obji, Tcl_Interp *interp, char *name, char *destName) {
    XOTclObject *obj = (XOTclObject*) obji;
    int result;
    Tcl_Obj *alias = 0;
    ALLOC_ON_STACK(Tcl_Obj *, 2, objv);

    objv[0] = XOTclGlobalObjects[XOTE_INSTVAR];
    objv[1] = Tcl_NewStringObj(name, -1);
    INCR_REF_COUNT(objv[1]);

    if (destName != 0) {
        alias = Tcl_NewStringObj(destName, -1);
        INCR_REF_COUNT(alias);
        Tcl_ListObjAppendElement(interp, objv[1], alias);
    }

    result = XOTclOInstVarMethod((ClientData) obj, interp, 2, objv);

    if (destName != 0) {
        DECR_REF_COUNT(alias);
    }
    DECR_REF_COUNT(objv[1]);
    FREE_ON_STACK(objv);
    return result;
}

extern void
XOTclRemovePMethod(Tcl_Interp *interp, XOTcl_Object *obji, char *nm) {
    XOTclObject *obj = (XOTclObject*) obji;
    if (obj->nsPtr)
        NSDeleteCmd(interp, obj->nsPtr, nm);
}

extern void
XOTclRemoveIMethod(Tcl_Interp *interp, XOTcl_Class *cli, char *nm) {
    XOTclClass *cl = (XOTclClass*) cli;
    NSDeleteCmd(interp, cl->nsPtr, nm);
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
XOTclOSetMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc > 3)  XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "var ?value?");
    return setInstVar(interp, obj, objv[1], objc == 3 ? objv[2] : NULL);
}

static int
XOTclSetterMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc > 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "?value?");
    return setInstVar(interp, obj, objv[0], objc == 2 ? objv[1] : NULL);
}


static int
XOTclOUpvarMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj  *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    Tcl_Obj *frameInfoObj = NULL;
    int i, result = TCL_ERROR;
    char *frameInfo;
    callFrameContext ctx = {0};

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], 
                                           "?level? otherVar localVar ?otherVar localVar ...?");

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

static int
XOTclOUplevelMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj  *CONST objv[]) {
    XOTclObject *obj = (XOTclObject *)clientData;
    int i, result = TCL_ERROR;
    char *frameInfo = NULL;
    Tcl_CallFrame *framePtr = NULL, *savedVarFramePtr;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 2) {
    uplevelSyntax:
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "?level? command ?arg ...?");
    }
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
    if (objc == 0) {
        goto uplevelSyntax;
    }

    if (!framePtr) {
        XOTclCallStackContent *csc = XOTclCallStackFindLastInvocation(interp, 1);
        if (csc)
            framePtr = csc->currentFramePtr;
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

static int
forwardArg(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
	   Tcl_Obj *o, forwardCmdClientData *tcd, Tcl_Obj **out,
	   Tcl_Obj **freeList, int *interpputarg, int *mapvalue) {
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
            pos = totalargs;
            remainder += 3;
        }
        if (element == remainder || abs(pos) > totalargs) {
            return XOTclVarErrMsg(interp, "forward: invalid index specified in argument ",
                                  ObjStr(o), (char *) NULL);
        }    if (!remainder || *remainder != ' ') {
            return XOTclVarErrMsg(interp, "forward: invaild syntax in '",  ObjStr(o),
                                  "' use: %@<pos> <cmd>",(char *) NULL);
        }

        element = ++remainder;
        if (pos<0) pos = totalargs + pos;
        /*fprintf(stderr,"remainder = '%s' pos = %ld\n", remainder, pos);*/
        *mapvalue = pos;
        element = remainder;
        c = *element;
    }
    /*fprintf(stderr,"c==%c element = '%s'\n", c, element);*/
    if (c == '%') {
        Tcl_Obj *list = 0, **listElements;
        int nrargs = objc-1, nrElements = 0;
        c = *++element;
        c1 = *(element+1);

        if (c == 's' && !strcmp(element,"self")) {
            *out = tcd->obj->cmdName;
        } else if (c == 'p' && !strcmp(element,"proc")) {
            *out = objv[0];
        } else if (c == '1' && (c1 == '\0' || c1 == ' ')) {
            /*fprintf(stderr, "   nrargs=%d, subcommands=%d inputarg=%d, objc=%d\n",
              nrargs, tcd->nr_subcommands, inputarg, objc);*/
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
                *interpputarg = 2;
            }
        } else if (c == 'a' && !strncmp(element,"argcl", 4)) {
            if (Tcl_ListObjIndex(interp, o, 1, &list) != TCL_OK) {
                return XOTclVarErrMsg(interp, "forward: %argclindex must by a valid list, given: '",
                                      ObjStr(o), "'", (char *) NULL);
            }
            if (Tcl_ListObjGetElements(interp, list, &nrElements, &listElements) != TCL_OK) {
                return XOTclVarErrMsg(interp, "forward: %argclindex contains invalid list '",
                                      ObjStr(list),"'", (char *) NULL);
            }
            if (nrargs >= nrElements) {
                return XOTclVarErrMsg(interp, "forward: not enough elements in specified list of ARGC argument ",
                                      ObjStr(o), (char *) NULL);
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
    XOTcl_FrameDecls;

    if (tcd->verbose) {
        Tcl_Obj *cmd = Tcl_NewListObj(objc, objv);
        fprintf(stderr,"calling %s\n", ObjStr(cmd));
        DECR_REF_COUNT(cmd);
    }
    if (tcd->objscope) {
        XOTcl_PushFrame(interp, tcd->obj);
    }
    if (tcd->objProc) {
        /*fprintf(stderr, "callForwarder: callobjproc\n");*/
        result = (tcd->objProc)(tcd->clientData, interp, objc, objv);
    } else if (tcd->cmdName->typePtr == &XOTclObjectType
               && XOTclObjConvertObject(interp, tcd->cmdName, (void*)&clientData) == TCL_OK) {
        /*fprintf(stderr, "callForwarder: XOTcl object %s, objc=%d\n", ObjStr(tcd->cmdName), objc);*/
        result = ObjDispatch(clientData, interp, objc, objv, 0);
    } else {
        /*fprintf(stderr, "callForwarder: no XOTcl object %s\n", ObjStr(tcd->cmdName));*/
        result = Tcl_EvalObjv(interp, objc, objv, 0);
    }

    if (tcd->objscope) {
        XOTcl_PopFrame(interp, tcd->obj);
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
    int result, j, inputarg=1, outputarg=0;
    if (!tcd || !tcd->obj) return XOTclObjErrType(interp, objv[0], "Object");

    /* it is a c-method; establish a value for the currentFramePtr */
    RUNTIME_STATE(interp)->cs.top->currentFramePtr =
        (Tcl_CallFrame *) Tcl_Interp_varFramePtr(interp);
  
    /*fprintf(stderr,"...setting currentFramePtr %p to %p (ForwardMethod)\n",
      RUNTIME_STATE(interp)->cs.top->currentFramePtr,
      (Tcl_CallFrame *) Tcl_Interp_varFramePtr(interp)); */
  
    if (tcd->passthrough) { /* two short cuts for simple cases */
        /* early binding, cmd* resolved, we have to care only for objscope */
        /*fprintf(stderr,"+++ forwardMethod passthrough \n");*/
        return callForwarder(tcd, interp, objc, objv);
    } else if (!tcd->args && *(ObjStr(tcd->cmdName)) != '%') { 
        /* we have ony to replace the method name with the given cmd name */
        ALLOC_ON_STACK(Tcl_Obj *, objc, ov);
        /*fprintf(stderr,"+++ forwardMethod must subst \n");*/
        memcpy(ov, objv, sizeof(Tcl_Obj *)*objc);
        ov[0] = tcd->cmdName;
        result = callForwarder(tcd, interp, objc, ov);
        FREE_ON_STACK(ov);
        return result;
    } else {
        Tcl_Obj **ov, *freeList=NULL;
        int totalargs = objc + tcd->nr_args + 3;
        ALLOC_ON_STACK(Tcl_Obj *, totalargs, OV);
        ALLOC_ON_STACK(int, totalargs, objvmap);
        /*fprintf(stderr,"+++ forwardMethod standard case \n");*/

        ov = &OV[1];
        if (tcd->needobjmap) {
            memset(objvmap, -1, sizeof(int)*totalargs);
        }
 
#if 0
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
        /*
          fprintf(stderr, "objc=%d, tcd->nr_subcommands=%d size=%d\n",
          objc, tcd->nr_subcommands, objc+ 2	    );*/

        if (objc-inputarg>0) {
            /*fprintf(stderr, "  copying remaining %d args starting at [%d]\n",
              objc-inputarg, outputarg);*/
            memcpy(ov+outputarg, objv+inputarg, sizeof(Tcl_Obj *)*(objc-inputarg));
        } else {
            /*fprintf(stderr, "  nothing to copy, objc=%d, inputarg=%d\n", objc, inputarg);*/
        }
        objc += outputarg - inputarg;

#if 0
        for(j=0; j<objc; j++) {
            /*fprintf(stderr, "  ov[%d]=%p, objc=%d\n", j, ov[j], objc);*/
            fprintf(stderr, " o[%d]=%s (%d),", j, ObjStr(ov[j]), objvmap[j]);
        }
        fprintf(stderr,"\n");
#endif

        if (tcd->needobjmap) 
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
                /* fprintf(stderr,"...setting at %d -> %s\n", pos, ObjStr(tmp)); */
                ov[pos] = tmp;
                objvmap[pos] = -1;
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
            fprintf(stderr, "  ov[%d]='%s' map=%d\n", j, ObjStr(ov[j]), objvmap[j]);
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



static int
XOTclOInstVarMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject  *obj   = (XOTclObject*)clientData;
    Tcl_Obj      **ov;
    int          i, oc, result = TCL_OK;
    callFrameContext ctx = {0};

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "?vars?");

    if (obj && (obj->filterStack || obj->mixinStack) ) {
        CallStackUseActiveFrames(interp, &ctx);
    }
    if (!Tcl_Interp_varFramePtr(interp)) {
        CallStackRestoreSavedFrames(interp, &ctx);
        return XOTclVarErrMsg(interp, "instvar used on ", ObjStr(obj->cmdName),
                              ", but callstack is not in procedure scope", 
                              (char *) NULL);
    }

    for (i=1; i<objc; i++) {
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
    CallStackRestoreSavedFrames(interp, &ctx);
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
XOTclOVwaitMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    int done, foundEvent;
    char *nameString;
    int flgs = TCL_TRACE_WRITES|TCL_TRACE_UNSETS;
    XOTcl_FrameDecls;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 2)
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "varname");

    nameString = ObjStr(objv[1]);

    /*
     * Make sure the var table exists and the varname is in there
     */
    if (NSRequireVariableOnObj(interp, obj, nameString, flgs) == 0)
        return XOTclVarErrMsg(interp, "Can't lookup (and create) variable ",
                              nameString, " on ", ObjStr(obj->cmdName), 
                              (char *) NULL);

    XOTcl_PushFrame(interp, obj);
    /*
     * much of this is copied from Tcl, since we must avoid
     * access with flag TCL_GLOBAL_ONLY ... doesn't work on
     * obj->varTable vars
     */
    if (Tcl_TraceVar(interp, nameString, flgs, (Tcl_VarTraceProc *)VwaitVarProc,
                     (ClientData) &done) != TCL_OK) {
        return TCL_ERROR;
    }
    done = 0;
    foundEvent = 1;
    while (!done && foundEvent) {
        foundEvent = Tcl_DoOneEvent(TCL_ALL_EVENTS);
    }
    Tcl_UntraceVar(interp, nameString, flgs, (Tcl_VarTraceProc *)VwaitVarProc,
                   (ClientData) &done);
    XOTcl_PopFrame(interp, obj);
    /*
     * Clear out the interpreter's result, since it may have been set
     * by event handlers.
     */
    Tcl_ResetResult(interp);

    if (!foundEvent) {
        return XOTclVarErrMsg(interp, "can't wait for variable '", nameString,
                              "':  would wait forever", (char *) NULL);
    }
    return TCL_OK;
}

static int
XOTclOInvariantsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    XOTclObjectOpt *opt;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 2)
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "<invariantList>");

    opt = XOTclRequireObjectOpt(obj);

    if (opt->assertions)
        TclObjListFreeList(opt->assertions->invariants);
    else
        opt->assertions = AssertionCreateStore();

    opt->assertions->invariants = AssertionNewList(interp, objv[1]);
    return TCL_OK;
}

static int
XOTclOAutonameMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    int instanceOpt = 0, resetOpt = 0;
    Tcl_Obj *autoname;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc == 3) {
        instanceOpt = (strcmp(ObjStr(objv[1]), "-instance") == 0);
        resetOpt = (strcmp(ObjStr(objv[1]), "-reset") == 0);
    }
    if ((objc < 2 || objc > 3) || (objc == 3 && !instanceOpt && !resetOpt))
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "?-instance|-reset? name");

    autoname = AutonameIncr(interp, objv[objc-1], obj, instanceOpt, resetOpt);
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

static int
XOTclOCheckMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    int ocArgs; Tcl_Obj **ovArgs;
    int i;
    XOTclObjectOpt *opt;

    /*fprintf(stderr,"checkmethod\n");*/
    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 2)
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], 
                                 "?all? ?pre? ?post? ?invar? ?instinvar?");

    opt = XOTclRequireObjectOpt(obj);
    opt->checkoptions = CHECK_NONE;

    if (Tcl_ListObjGetElements(interp, objv[1], &ocArgs, &ovArgs) == TCL_OK
        && ocArgs > 0) {
        for (i = 0; i < ocArgs; i++) {
            char *option = ObjStr(ovArgs[i]);
            if (option != 0) {
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
                              ObjStr(obj->cmdName), " ", ObjStr(objv[0]),
                              " ", ObjStr(objv[1]),
                              "', valid: all pre post invar instinvar", 
                              (char *) NULL);
    }

    Tcl_ResetResult(interp);
    return TCL_OK;
}

static int
XOTclConfigureCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int bool, opt, result = TCL_OK;
    static CONST char *opts[] = {
        "filter", "softrecreate",
        NULL
    };
    enum subCmdIdx {
        filterIdx, softrecreateIdx, 
    };

    if (objc < 2 || objc>3)
        return XOTclObjErrArgCnt(interp, objv[0], NULL,
                                 "filter|softrecreate ?on|off?");

    if (Tcl_GetIndexFromObj(interp, objv[1], opts, "option", 0, &opt) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 3) {
        result = Tcl_GetBooleanFromObj(interp, objv[2], &bool);
    }
    if (result == TCL_OK) {
        switch (opt) {
        case filterIdx:
            Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                              (RUNTIME_STATE(interp)->doFilters));
            if (objc == 3) 
                RUNTIME_STATE(interp)->doFilters = bool;
            break;
      
        case softrecreateIdx:
            Tcl_SetBooleanObj(Tcl_GetObjResult(interp),
                              (RUNTIME_STATE(interp)->doSoftrecreate));
            if (objc == 3) 
                RUNTIME_STATE(interp)->doSoftrecreate = bool;
            break;
        }
    }
    return result;
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
XOTclMethodPropertyCmd(ClientData clientData, Tcl_Interp *interp, 
                       int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = NULL;
    XOTclClass *cl = NULL;
    Tcl_Command cmd = NULL;
    char allocation, *methodName, *optionName;
    int protected = 0, i, opt;

    static CONST char *opts[] = {"protected", "public", "slotobj", NULL};
    enum subCmdIdx {protectedIdx, publicIdx, soltobjIdx};
  
    /* TODO introspection */

    if (objc < 4 || objc > 6) {
        return XOTclObjErrArgCnt(interp, objv[0], NULL,
                                 "<class>|<obj> <methodName> ?-per-object? <protected|public|slotobj> ?<arg>?");
    }
       
    GetXOTclClassFromObj(interp, objv[1], &cl, 0);
    if (!cl) {
        XOTclObjConvertObject(interp, objv[1], &obj);
        if (!obj) 
            return XOTclVarErrMsg(interp, ObjStr(objv[0]), 
                                  " should be called on Class|Object, not ",
                                  ObjStr(objv[1]), NULL);
        allocation = 'o';
    } else {
        obj = &cl->object;
        allocation = 'c';
    }

    methodName = ObjStr(objv[2]);
  
    for (i=3; i<5 && i < objc; i++) {
        optionName = ObjStr(objv[i]);
        if (*optionName != '-') break;
        if (!strcmp("-per-object", optionName)) {
            allocation = 'o';
            if (cl) obj = &(cl->object);
        } else {
            return XOTclErrBadVal(interp, "::xotcl::methodproperty", 
                                  "option -per-object", optionName);
        }
    }

    if (Tcl_GetIndexFromObj(interp, objv[i], opts, "methodproperty", 0, &opt) != TCL_OK) {
        return TCL_ERROR;
    }

    /*fprintf(stderr, "allocation for %s = %c\n", ObjStr(objv[1]), allocation);*/
        
    if (allocation == 'o') { /* xxx */
        if (obj->nsPtr) 
            cmd = FindMethod(methodName, obj->nsPtr);
        if (!cmd) {
            return XOTclVarErrMsg(interp, ObjStr(objv[0]), 
                                  " cannot lookup object method '",
                                  methodName, "' for object ", ObjStr(objv[1]),
                                  (char *) NULL);
        }
    } else {
        if (cl->nsPtr) 
            cmd = FindMethod(methodName, cl->nsPtr);
        if (!cmd) 
            return XOTclVarErrMsg(interp, ObjStr(objv[0]), " cannot lookup method '",
                                  methodName, "' from class  ", ObjStr(objv[1]),
                                  (char *) NULL);
    }
    
    if (opt == protectedIdx || opt == publicIdx) {
        protected = (opt == protectedIdx);
        
        if (protected) {
            Tcl_Command_flags(cmd) |= XOTCL_PROTECTED_METHOD; 
        } else {
            Tcl_Command_flags(cmd) &= XOTCL_PROTECTED_METHOD;
        }
    } else { /* slotobj */
        Tcl_HashTable **nonposArgsTable = allocation == 'o' ?  
            &(obj->nonposArgsTable) : 
            &(cl->nonposArgsTable);
        XOTclNonposArgs *nonposArgs;

        if (i + 2 != objc) {
            return XOTclObjErrArgCnt(interp, objv[0], NULL,
                                     "<class>|<obj> <methodName> ?-per-object? slotobj <slotobj>");
        }

        if (*nonposArgsTable == 0) {
            *nonposArgsTable = NonposArgsCreateTable();
            fprintf(stderr,"this can only happen if we define a slotobj for a class/object without nonposargs\n");
        }
        nonposArgs = NonposArgsGet(*nonposArgsTable, methodName);
        if (nonposArgs == NULL) {
            int nw;
            Tcl_HashEntry *hPtr = Tcl_CreateHashEntry(*nonposArgsTable, methodName, &nw);
            assert(nw);

            fprintf(stderr,"this can only happen if we define a slotobj for a method without nonpospargs\n slotobj = %s\n", ObjStr(objv[i+1]));

            MEM_COUNT_ALLOC("nonposArg", nonposArgs);
            nonposArgs = (XOTclNonposArgs*)ckalloc(sizeof(XOTclNonposArgs));
            nonposArgs->slotObj = NULL;
            nonposArgs->nonposArgs = NULL;
            nonposArgs->ordinaryArgs = NULL;
            Tcl_SetHashValue(hPtr, (ClientData)nonposArgs);

            /* TODO check:
               problem with nonposArgs->nonposArgs = NULL ?
               problem with nonposArgs->ordinaryArgs = NULL ?

               what happens if first method property and then method.
               what happens if method then property then new method?
            */
        } else {
            
            fprintf(stderr,"define slotobj for a method with nonpospargs\n slotobj = %s \n", ObjStr(objv[i+1]));
            if (nonposArgs->slotObj) {
                DECR_REF_COUNT(nonposArgs->slotObj);
            }
        }
        nonposArgs->slotObj = objv[i+1];
        INCR_REF_COUNT(nonposArgs->slotObj);
    }

    return TCL_OK;
}

static int
XOTclDispatchCmd(ClientData clientData, Tcl_Interp *interp, 
                 int objc, Tcl_Obj *CONST objv[]) {
    int result;
    char *method;
    XOTclObject *obj;
    register char *n;
    ClientData cp;
    /* xxx */

    if (objc < 3) {
        return XOTclObjErrArgCnt(interp, objv[0], NULL, "<obj> <methodName> ?args?");
    }
    XOTclObjConvertObject(interp, objv[2], &obj);
    if (!obj) 
        return XOTclObjErrType(interp, objv[2], "Class|Object");

    method = ObjStr(objv[1]);
    n = method + strlen(method);  

    /*search for last '::'*/
    while ((*n != ':' || *(n-1) != ':') && n-1 > method) {n--; }
    if (*n == ':' && n > method && *(n-1) == ':') {n--;}
  
    if ((n-method)>1 || *method == ':') {
        Tcl_DString parentNSName, *dsp = &parentNSName;
        Tcl_Namespace *ns;
        Tcl_Command cmd, importedCmd;
        char *parentName, *tail = n+2;
        DSTRING_INIT(dsp);

        if (n-method != 0) {
            Tcl_DStringAppend(dsp, method, (n-method));
            parentName = Tcl_DStringValue(dsp);
            ns = Tcl_FindNamespace(interp, parentName, (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY);
            DSTRING_FREE(dsp);
        } else {
            ns = Tcl_FindNamespace(interp, "::", (Tcl_Namespace *) NULL, TCL_GLOBAL_ONLY);
        }
        if (!ns) {
            return XOTclVarErrMsg(interp, "cannot lookup parent namespace '",
                                  method, "'", (char *) NULL);  
        }

        cmd = FindMethod(tail, ns);
        if (cmd && (importedCmd = TclGetOriginalCommand(cmd))) {
            cmd = importedCmd;
        }

        if (cmd == NULL) {
            return XOTclVarErrMsg(interp, "cannot lookup command '",
                                  tail, "'", (char *) NULL);
        }
    
        cp = Tcl_Command_objClientData(cmd);
        result = DoCallProcCheck(cp, (ClientData)obj, interp,
                                 objc-1, objv+1, cmd, obj, 
                                 NULL /*XOTclClass *cl*/, tail,
                                 XOTCL_CSC_TYPE_PLAIN, 0, 1);
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


static int
XOTclAliasCmd(ClientData clientData, Tcl_Interp *interp, 
              int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = NULL;
    XOTclClass *cl = NULL;
    Tcl_Command cmd = NULL, importedCmd, newCmd;
    Tcl_ObjCmdProc *objProc;
    char allocation, *methodName, *optionName;
    Tcl_CmdDeleteProc *dp = NULL;
    aliasCmdClientData *tcd = NULL;
    int objscope = 0, protected = 0, flags = 0, i;

    if (objc < 4 || objc > 6) {
        return XOTclObjErrArgCnt(interp, objv[0], NULL,
                                 "<class>|<obj> <methodName> ?-objscope? ?-per-object? <cmdName>");
    }
       
    GetXOTclClassFromObj(interp, objv[1], &cl, 0);
    if (!cl) {
        XOTclObjConvertObject(interp, objv[1], &obj);
        if (!obj) 
            return XOTclObjErrType(interp, objv[1], "Class|Object");
        allocation = 'o';
    } else {
        allocation = 'c';
    }

    methodName = ObjStr(objv[2]);
  
    for (i=3; i<6 && i < objc; i++) {
        optionName = ObjStr(objv[i]);
        if (*optionName != '-') break;
        if (!strcmp("-objscope", optionName)) {
            objscope = 1;
        } else if (!strcmp("-per-object", optionName)) {
            allocation = 'o';
        } else if (!strcmp("-protected", optionName)) {
            protected = 1;
        } else {
            return XOTclErrBadVal(interp, "::xotcl::alias", 
                                  "option -objscope or -per-object", optionName);
        }
    }

    cmd = Tcl_GetCommandFromObj(interp, objv[i]);
    if (cmd == NULL) 
        return XOTclVarErrMsg(interp, "cannot lookup command '",
                              ObjStr(objv[i]), "'", (char *) NULL);
  
    if ((importedCmd = TclGetOriginalCommand(cmd))) {
        cmd = importedCmd;
    }
    objProc = Tcl_Command_objProc(cmd); 

    if (objc>i+1) {
        return XOTclVarErrMsg(interp, "invalid argument '",
                              ObjStr(objv[i+1]), "'", (char *) NULL);
    }

    if (objscope) {
        tcd = NEW(aliasCmdClientData);
        tcd->cmdName = 0;
        tcd->obj = allocation == 'c' ? &cl->object : obj;
        tcd->objProc = objProc;
        tcd->clientData      = Tcl_Command_objClientData(cmd);
        objProc      = XOTclObjscopedMethod;
        dp = aliasCmdDeleteProc;
    }

    if (protected) {
        flags = XOTCL_PROTECTED_METHOD; 
    }

    if (allocation == 'c') {
        /*fprintf(stderr,"alias for %s allocation %c\n", ObjStr(cl->object.cmdName), allocation);*/
        newCmd = XOTclAddInstanceMethod(interp, (XOTcl_Class*)cl, methodName, 
                                        objProc, tcd, dp, flags);
    } else {
        /*fprintf(stderr,"alias for %s allocation %c\n", ObjStr(obj->cmdName), allocation);*/
        newCmd = XOTclAddObjectMethod(interp, (XOTcl_Object*)obj, methodName, 
                                      objProc, tcd, dp, flags);
    }

    return TCL_OK;
}



static int
XOTclSetInstvarCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = NULL;

    if (objc < 3 || objc > 4)
        return XOTclObjErrArgCnt(interp, objv[0], NULL, "obj var ?value?");
    /*fprintf(stderr,"setinstvar obj '%s' var '%s' %d\n", ObjStr(objv[1]), ObjStr(objv[2]), objc);*/
    XOTclObjConvertObject(interp, objv[1], &obj);
    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");

    return setInstVar(interp, obj , objv[2], objc == 4 ? objv[3] : NULL);
}


static int
XOTclRelationCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int oc; Tcl_Obj **ov;
    XOTclObject *obj = NULL, *nobj = NULL;
    XOTclClass *cl = NULL;
    XOTclObjectOpt *objopt = NULL;
    XOTclClassOpt *clopt = NULL, *nclopt = NULL;
    int i, opt;
    static CONST char *opts[] = {
        "mixin", "instmixin", "object-mixin", "class-mixin",
        "filter", "instfilter", "object-filter", "class-filter",
        "class", "superclass",
        NULL
    };
    enum subCmdIdx {
        mixinIdx, instmixinIdx, pomIdx, pcmIdx, 
        filterIdx, instfilterIdx, pofIdx, pcfIdx,
        classIdx, superclassIdx
    };
  
    if (objc < 3 || objc > 4)
        return XOTclObjErrArgCnt(interp, objv[0], NULL, "obj reltype values");

    if (Tcl_GetIndexFromObj(interp, objv[2], opts, "relation type", 0, &opt) != TCL_OK) {
        return TCL_ERROR;
    }

    switch (opt) {
    case pomIdx:
    case mixinIdx:
    case pofIdx:
    case filterIdx: 
        XOTclObjConvertObject(interp, objv[1], &obj);
        if (!obj) return XOTclObjErrType(interp, objv[1], "Object");
        if (objc == 3) {
            objopt = obj->opt;
            switch (opt) {
            case pomIdx:
            case mixinIdx: return objopt ? MixinInfo(interp, objopt->mixins, NULL, 1) : TCL_OK;
            case pofIdx:
            case filterIdx: return objopt ? FilterInfo(interp, objopt->filters, NULL, 1, 0) : TCL_OK;
            }
        } 
        if (Tcl_ListObjGetElements(interp, objv[3], &oc, &ov) != TCL_OK)
            return TCL_ERROR;
        objopt = XOTclRequireObjectOpt(obj);
        break;

    case pcmIdx:
    case instmixinIdx:
    case pcfIdx:
    case instfilterIdx: 
        GetXOTclClassFromObj(interp, objv[1], &cl, 0);
        if (!cl) return XOTclObjErrType(interp, objv[1], "Class");
    
        if (objc == 3) {
            clopt = cl->opt;
            switch (opt) {
            case pcmIdx:
            case instmixinIdx: return clopt ? MixinInfo(interp, clopt->instmixins, NULL, 1) : TCL_OK;
            case pcfIdx: 
            case instfilterIdx: return objopt ? FilterInfo(interp, clopt->instfilters, NULL, 1, 0) : TCL_OK;
            }
        } 
    
        if (Tcl_ListObjGetElements(interp, objv[3], &oc, &ov) != TCL_OK)
            return TCL_ERROR;
        clopt = XOTclRequireClassOpt(cl);
        break;

    case superclassIdx: 
        GetXOTclClassFromObj(interp, objv[1], &cl, 0);
        if (objc == 3) {
            return ListSuperclasses(interp, cl, NULL);
        }
        if (!cl) return XOTclObjErrType(interp, objv[1], "Class");
        if (Tcl_ListObjGetElements(interp, objv[3], &oc, &ov) != TCL_OK)
            return TCL_ERROR;
        return SuperclassAdd(interp, cl, oc, ov, objv[3], cl->object.cl);
    
    case classIdx: 
        XOTclObjConvertObject(interp, objv[1], &obj);
        if (objc == 3) {
            Tcl_SetObjResult(interp, obj->cl->object.cmdName);
            return TCL_OK;
        }
        if (!obj) return XOTclObjErrType(interp, objv[1], "Object");
        GetXOTclClassFromObj(interp, objv[3], &cl, obj->cl);
        if (!cl) return XOTclErrBadVal(interp, "class", "a class", ObjStr(objv[1]));
        return changeClass(interp, obj, cl);
    }

    switch (opt) {
    case pomIdx: 
    case mixinIdx: 

        if (objopt->mixins) {
            XOTclCmdList *cmdlist, *del;
            for (cmdlist = objopt->mixins; cmdlist; cmdlist = cmdlist->nextPtr) {
                cl = XOTclGetClassFromCmdPtr(cmdlist->cmdPtr);
                clopt = cl ? cl->opt : NULL;
                if (clopt) {
                    del = CmdListFindCmdInList(obj->id, clopt->mixinofs);
                    if (del) {
                        /* fprintf(stderr,"Removing object %s from mixinofs of class %s\n",
                           ObjStr(obj->cmdName), ObjStr(XOTclGetClassFromCmdPtr(cmdlist->cmdPtr)->object.cmdName)); */
                        del = CmdListRemoveFromList(&clopt->mixinofs, del);
                        CmdListDeleteCmdListEntry(del, GuardDel);
                    }
                }
            }
            CmdListRemoveList(&objopt->mixins, GuardDel);
        }
        
        obj->flags &= ~XOTCL_MIXIN_ORDER_VALID;
        /*
         * since mixin procs may be used as filters -> we have to invalidate
         */
        obj->flags &= ~XOTCL_FILTER_ORDER_VALID;

        /*
         * now add the specified mixins
         */
        for (i = 0; i < oc; i++) {
            Tcl_Obj *ocl = NULL;

            if (MixinAdd(interp, &objopt->mixins, ov[i], obj->cl->object.cl) != TCL_OK) {
                return TCL_ERROR;
            }
            /* fprintf(stderr,"Added to mixins of %s: %s\n", ObjStr(obj->cmdName), ObjStr(ov[i])); */
            Tcl_ListObjIndex(interp, ov[i], 0, &ocl);
            XOTclObjConvertObject(interp, ocl, &nobj);
            if (nobj) {
                /* fprintf(stderr,"Registering object %s to mixinofs of class %s\n",
                   ObjStr(obj->cmdName), ObjStr(nobj->cmdName)); */
                nclopt = XOTclRequireClassOpt((XOTclClass*) nobj);
                CmdListAdd(&nclopt->mixinofs, obj->id, NULL, /*noDuplicates*/ 1);
            } /* else fprintf(stderr,"Problem registering %s as a mixinof of %s\n",
                 ObjStr(ov[i]), ObjStr(cl->object.cmdName)); */
        }
    
        MixinComputeDefined(interp, obj);
        FilterComputeDefined(interp, obj);
        break;

    case pofIdx: 
    case filterIdx: 

        if (objopt->filters) CmdListRemoveList(&objopt->filters, GuardDel);
    
        obj->flags &= ~XOTCL_FILTER_ORDER_VALID;
        for (i = 0; i < oc; i ++) {
            if (FilterAdd(interp, &objopt->filters, ov[i], obj, 0) != TCL_OK)
                return TCL_ERROR;
        }
        /*FilterComputeDefined(interp, obj);*/
        break;

    case pcmIdx: 
    case instmixinIdx:

        if (clopt->instmixins) {
            RemoveFromInstmixinsofs(cl->object.id, clopt->instmixins);
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
               ObjStr(cl->object.cmdName), ObjStr(ov[i])); */
            
            Tcl_ListObjIndex(interp, ov[i], 0, &ocl);
            XOTclObjConvertObject(interp, ocl, &nobj);
            if (nobj) {
                /* fprintf(stderr,"Registering class %s to instmixinofs of class %s\n",
                   ObjStr(cl->object.cmdName), ObjStr(nobj->cmdName)); */
                nclopt = XOTclRequireClassOpt((XOTclClass*) nobj);
                CmdListAdd(&nclopt->instmixinofs, cl->object.id, NULL, /*noDuplicates*/ 1);
            } /* else fprintf(stderr,"Problem registering %s as a instmixinof of %s\n",
                 ObjStr(ov[i]), ObjStr(cl->object.cmdName)); */
        }
        break;

    case pcfIdx: 
    case instfilterIdx:

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


static int
XOTclOMixinGuardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    XOTclCmdList *h;
    XOTclObjectOpt *opt;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 3)
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "mixin guards");

    opt = obj->opt;
    if (opt && opt->mixins) {
        XOTclClass *mixinCl = XOTclpGetClass(interp, ObjStr(objv[1]));
        Tcl_Command mixinCmd = NULL;
        if (mixinCl) {
            mixinCmd = Tcl_GetCommandFromObj(interp, mixinCl->object.cmdName);
        }
        if (mixinCmd) {
            h = CmdListFindCmdInList(mixinCmd, opt->mixins);
            if (h) {
                if (h->clientData)
                    GuardDel((XOTclCmdList*) h);
                GuardAdd(interp, h, objv[2]);
                obj->flags &= ~XOTCL_MIXIN_ORDER_VALID;
                return TCL_OK;
            }
        }
    }

    return XOTclVarErrMsg(interp, "Mixinguard: can't find mixin ",
                          ObjStr(objv[1]), " on ", ObjStr(obj->cmdName), 
                          (char *) NULL);
}


static int
XOTclOFilterGuardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    XOTclCmdList *h;
    XOTclObjectOpt *opt;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 3)
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "filtername filterGuards");

    opt = obj->opt;
    if (opt && opt->filters) {
        h = CmdListFindNameInList(interp, ObjStr(objv[1]), opt->filters);
        if (h) {
            if (h->clientData)
                GuardDel((XOTclCmdList*) h);
            GuardAdd(interp, h, objv[2]);
            obj->flags &= ~XOTCL_FILTER_ORDER_VALID;
            return TCL_OK;
        }
    }

    return XOTclVarErrMsg(interp, "Filterguard: can't find filter ",
                          ObjStr(objv[1]), " on ", ObjStr(obj->cmdName), 
                          (char *) NULL);
}

/*
 *  Searches for filter on [self] and returns fully qualified name
 *  if it is not found it returns an empty string
 */
static int
XOTclOFilterSearchMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    char *methodName;
    XOTclCmdList *cmdList;
    XOTclClass *fcl;
    XOTclObject *fobj;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "<name>");
    Tcl_ResetResult(interp);

    if (!(obj->flags & XOTCL_FILTER_ORDER_VALID))
        FilterComputeDefined(interp, obj);
    if (!(obj->flags & XOTCL_FILTER_ORDER_DEFINED))
        return TCL_OK;

    methodName = ObjStr(objv[1]);
    cmdList = obj->filterOrder;

    while (cmdList) {
        CONST84 char *filterName = Tcl_GetCommandName(interp, cmdList->cmdPtr);
        if (filterName[0] == methodName[0] && !strcmp(filterName, methodName))
            break;
        cmdList = cmdList->nextPtr;
    }

    if (!cmdList)
        return TCL_OK;

    fcl = cmdList->clorobj;
    if (fcl && XOTclObjectIsClass(&fcl->object)) {
        fobj = 0;
    } else {
        fobj = (XOTclObject*)fcl;
        fcl = 0;
    }

    Tcl_SetObjResult(interp, 
                     getFullProcQualifier(interp, methodName, fobj, fcl, 
                                          cmdList->cmdPtr));
    return TCL_OK;
}

static int
XOTclOProcSearchMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    XOTclClass *pcl = NULL;
    Tcl_Command cmd = NULL;
    char *simpleName, *methodName;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "<name>");

    Tcl_ResetResult(interp);

    methodName = ObjStr(objv[1]);

    if (!(obj->flags & XOTCL_MIXIN_ORDER_VALID))
        MixinComputeDefined(interp, obj);

    if (obj->flags & XOTCL_MIXIN_ORDER_DEFINED_AND_VALID) {
        XOTclCmdList *mixinList = obj->mixinOrder;
        while (mixinList) {
            XOTclClass *mcl = XOTclpGetClass(interp, (char *)Tcl_GetCommandName(interp, mixinList->cmdPtr));
            if (mcl && (pcl = SearchCMethod(mcl, methodName, &cmd))) {
                break;
            }
            mixinList = mixinList->nextPtr;
        }
    }

    if (!cmd && obj->nsPtr) {
        cmd = FindMethod(methodName, obj->nsPtr);
    }
  
    if (!cmd && obj->cl)
        pcl = SearchCMethod(obj->cl, methodName, &cmd);

    if (cmd) {
        XOTclObject *pobj = pcl ? NULL : obj;
        simpleName = (char *)Tcl_GetCommandName(interp, cmd);
        Tcl_SetObjResult(interp, getFullProcQualifier(interp, simpleName, pobj, pcl, cmd));
    }
    return TCL_OK;
}

static int
XOTclORequireNamespaceMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc != 1) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], NULL);

    requireObjNamespace(interp, obj);
    return TCL_OK;
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
#if defined(PRE82)
        Tcl_Obj *tmp = Tcl_NewListObj(1, &obj);
        listType = tmp->typePtr;
        DECR_REF_COUNT(tmp);
#else
        static XOTclMutex initMutex = 0;
        XOTclMutexLock(&initMutex);
        if (listType == NULL) {
            listType = Tcl_GetObjType("list");
            /*fprintf(stderr, "fetching listType=%p\n", listType);*/
        }
        XOTclMutexUnlock(&initMutex);
#endif
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
      ObjStr(obj->cmdName), methodName, argc);*/

    if (isInitString(methodName))
        obj->flags |= XOTCL_INIT_CALLED;

    Tcl_ResetResult(interp);
    INCR_REF_COUNT(method);
    result = callMethod((ClientData)obj, interp, method, argc, argv, XOTCL_CM_NO_UNKNOWN);
    DECR_REF_COUNT(method);

    /*fprintf(stderr, "method  '%s' called args: %d o=%p, result=%d %d\n",
      methodName, argc+1, obj, result, TCL_ERROR);*/

    if (result != TCL_OK) {
        /* XXXX TODO not sure, if we really need this; see kristoffer lawson mail */
        Tcl_Obj *res =  Tcl_DuplicateObj(Tcl_GetObjResult(interp)); /* save the result */
        INCR_REF_COUNT(res);
        XOTclVarErrMsg(interp, ObjStr(res), " during '", ObjStr(obj->cmdName), " ",
                       methodName, "'", (char *) NULL);
        DECR_REF_COUNT(res);
    }

    return result;
}


static int
XOTclOSetvaluesMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    Tcl_Obj **argv, **nextArgv, *resultObj;
    int i, argc, nextArgc, normalArgs, result = TCL_OK, isdasharg = NO_DASH;
    char *methodName, *nextMethodName;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 1) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "?args?");

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
        case SKALAR_DASH:    /* argument is a skalar with a leading dash */
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
        case LIST_DASH:  /* argument is a list with a leading dash, grouping determined by list */
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
                return XOTclVarErrMsg(interp, ObjStr(obj->cmdName),
                                      " configure: unexpected argument '", 
                                      ObjStr(objv[i]),
                                      "' between parameters", (char *) NULL);
            }
        }
    }

    resultObj = Tcl_NewListObj(normalArgs, objv+1);
    /*fprintf(stderr,".... setvalues returns %s\n", ObjStr(resultObj));*/
    Tcl_SetObjResult(interp, resultObj);

    return result;
}

static int
XOTclOConfigureMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*)clientData;
    XOTclObjects *slotObjects, *so;
    int result;
    /* would be nice to do it here instead of setValue 
       XOTcl_FrameDecls;

       XOTcl_PushFrame(interp, obj); make instvars of obj accessible */

    /*
     * Search for default values on slots
     */
    slotObjects = computeSlotObjects(interp, obj, NULL);
    for (so = slotObjects; so; so = so->nextPtr) {
        result = setDefaultValue(interp, obj, so->obj);
        if (result != TCL_OK) { 
            goto configure_exit;
        }
    }

    /*
     * call configure methods (starting with '-')
     */
    result = callMethod((ClientData) obj, interp,
                        XOTclGlobalObjects[XOTE_SETVALUES], objc+1, objv+1, 0);
    if (result != TCL_OK) {
        goto configure_exit;
    }

    /*
     * Check, if we got the required values
     */
    for (so = slotObjects; so; so = so->nextPtr) {
        result = checkRequiredValue(interp, obj, so->obj);
        if (result != TCL_OK) { 
            goto configure_exit;
        }
    }

 configure_exit:
    /*XOTcl_PopFrame(interp, obj);*/

    if (slotObjects) 
        XOTclObjectListFree(slotObjects);
    return result;
}



/*
 * class method implementations
 */

static int
XOTclCInstDestroyMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);
    XOTclObject *delobj;
    int rc;

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc < 2)
        return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "<obj|cl>");

    if (XOTclObjConvertObject(interp, objv[1], &delobj) != TCL_OK)
        return XOTclVarErrMsg(interp, "Can't destroy object ",
                              ObjStr(objv[1]), " that does not exist.", 
                              (char *) NULL);
 
    /* fprintf(stderr,"instdestroy obj=%s, opt=%p\n", ObjStr(delobj->cmdName), delobj->opt);*/
    rc = freeUnsetTraceVariable(interp, delobj);
    if (rc != TCL_OK) {
        return rc;
    }

    /*
     * latch, and call delete command if not already in progress
     */
    delobj->flags |= XOTCL_DESTROY_CALLED;
    RUNTIME_STATE(interp)->callIsDestroy = 1;
    /*fprintf(stderr,"instDestroy %s : setting callIsDestroy = 1\n", ObjStr(objv[1]));*/
    if (RUNTIME_STATE(interp)->exitHandlerDestroyRound !=
        XOTCL_EXITHANDLER_ON_SOFT_DESTROY) {
        CallStackDestroyObject(interp, delobj);
    }

    return TCL_OK;
}


static Tcl_Namespace *
callingNameSpace(Tcl_Interp *interp) {
    Tcl_Namespace *ns = NULL;
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
    XOTclCallStackContent *top = cs->top;
    XOTclCallStackContent *csc = XOTclCallStackFindLastInvocation(interp, 0);

    /*fprintf(stderr," **** use last invocation csc = %p\n",  csc);*/
    if (csc && csc->currentFramePtr) {
        /* use the callspace from the last invocation */
        XOTclCallStackContent *called = csc<top? csc+1 : NULL;
        Tcl_CallFrame *f = called ?
            Tcl_CallFrame_callerPtr(called->currentFramePtr) : NULL;
        /*fprintf(stderr," ****  csc use frame= %p\n",  f);*/
        if (f) {
            ns = f->nsPtr;
        } else {
            Tcl_CallFrame *f = Tcl_CallFrame_callerPtr(csc->currentFramePtr);
            ns = Tcl_GetCurrentNamespace(interp);
            /* find last incovation outside ::xotcl (for things like relmgr) */
            while (ns == RUNTIME_STATE(interp)->XOTclNS) {
                if (f) {
                    ns = f->nsPtr;
                    f = Tcl_CallFrame_callerPtr(f);
                } else {
                    ns = Tcl_GetGlobalNamespace(interp);
                }
            }
            /*fprintf(stderr, "found ns %p '%s'\n", ns, ns?ns->fullName:"NULL");*/
        }
    }
    if (!ns) {
        /* calls on xotcl toplevel */
        XOTclCallStackContent *bot = cs->content + 1;
        /*fprintf(stderr, " **** bot=%p diff=%d\n", bot, top-bot);*/
        if (top - bot >= 0 && bot->currentFramePtr) {
            /* get calling tcl environment */
            Tcl_CallFrame *f = Tcl_CallFrame_callerPtr(bot->currentFramePtr);
            if (f) {
                ns = f->nsPtr;
                /*fprintf(stderr, "top=%p, bot=%p b->c=%p f=%p ns=%p\n",
                  top, bot, bot->currentFramePtr, f, ns);*/
                /*fprintf(stderr,"ns from calling tcl environment %p '%s'\n",
                  ns, ns?ns->fullName : "" );*/
            } else {
                /* fprintf(stderr, "nothing found, use ::\n"); */
                ns = Tcl_GetGlobalNamespace(interp);
            }
        }
    }

    /*XOTclCallStackDump(interp);*/
    /*XOTclStackDump(interp);*/

    /*fprintf(stderr,"callingNameSpace returns %p %s\n", ns, ns?ns->fullName:"");*/
    return ns;
}


static int
XOTclCAllocMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);
    XOTclClass *newcl;
    XOTclObject *newobj;
    int result;

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc < 2)
        return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "<obj|cl> ?args?");

#if 0
    fprintf(stderr, "type(%s)=%p %s %d\n",
            ObjStr(objv[1]), objv[1]->typePtr, objv[1]->typePtr?
            objv[1]->typePtr->name:"NULL",
            XOTclObjConvertObject(interp, objv[1], &newobj)
            );
    /*
     * if the lookup via GetObject for the object succeeds,
     * the object exists already,
     * and we do not overwrite it, but re-create it
     */
    if (XOTclObjConvertObject(interp, objv[1], &newobj) == TCL_OK) {
        fprintf(stderr, "lookup successful\n");
        result = doCleanup(interp, newobj, &cl->object, objc, objv);
    } else
#endif
        {
            /*
             * create a new object from scratch
             */
            char *objName = ObjStr(objv[1]);
            Tcl_Obj *tmpName = NULL;

            if (!NSCheckColons(objName, 0)) {
                return XOTclVarErrMsg(interp, "Cannot allocate object -- illegal name '",
                                      objName, "'", (char *) NULL);
            }

            if (!isAbsolutePath(objName)) {
                /*fprintf(stderr, "CallocMethod\n");*/
                tmpName = NameInNamespaceObj(interp, objName, callingNameSpace(interp));
                /*fprintf(stderr, "NoAbsoluteName for '%s' -> determined = '%s'\n",
                  objName, ObjStr(tmpName));*/
                objName = ObjStr(tmpName);

                /*fprintf(stderr," **** name is '%s'\n",  objName);*/
                INCR_REF_COUNT(tmpName);
            }

            if (IsMetaClass(interp, cl)) {
                /*
                 * if the base class is a meta-class, we create a class
                 */
                newcl = PrimitiveCCreate(interp, objName, cl);
                if (newcl == 0)
                    result = XOTclVarErrMsg(interp, "Class alloc failed for '", objName,
                                            "' (possibly parent namespace does not exist)", 
                                            (char *) NULL);
                else {
                    Tcl_SetObjResult(interp, newcl->object.cmdName);
                    result = TCL_OK;
                }
            } else {
                /*
                 * if the base class is an ordinary class, we create an object
                 */
                newobj = PrimitiveOCreate(interp, objName, cl);
                if (newobj == 0)
                    result = XOTclVarErrMsg(interp, "Object alloc failed for '", objName,
                                            "' (possibly parent namespace does not exist)", 
                                            (char *) NULL);
                else {
                    result = TCL_OK;
                    Tcl_SetObjResult(interp, newobj->cmdName);
                }
            }

            if (tmpName) {
                DECR_REF_COUNT(tmpName);
            }
	
        }

    return result;
}


static int
createMethod(Tcl_Interp *interp, XOTclClass *cl, ClientData data,
	     int objc, Tcl_Obj *CONST objv[]) { 
    XOTclObject *newobj = NULL;
    Tcl_Obj *nameObj, *tmpObj = NULL;
    int result;
    char *objName, *specifiedName;

    ALLOC_ON_STACK(Tcl_Obj *, objc, tov);

    memcpy(tov, objv, sizeof(Tcl_Obj *)*(objc));
    specifiedName = objName = ObjStr(objv[1]);
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
    newobj = XOTclpGetObject(interp, objName);

    /*fprintf(stderr,"+++ create objv[1] '%s', specifiedName '%s', newObj=%p\n",
      specifiedName, objName, newobj);*/

    /* don't allow an object to be recreated as a class */
    if (newobj && (!IsMetaClass(interp, cl) || IsMetaClass(interp, newobj->cl))) {
        /*fprintf(stderr, "%%%% recreate, call recreate method ... %s, objc=%d\n",
          ObjStr(tov[1]), objc+1);*/
        /* call recreate --> initialization */
        result = callMethod((ClientData) cl, interp,
                            XOTclGlobalObjects[XOTE_RECREATE], objc+1, tov+1, 0);
        if (result != TCL_OK)
            goto create_method_exit;

        Tcl_SetObjResult(interp, newobj->cmdName);
        nameObj = newobj->cmdName;
        objTrace("RECREATE", newobj);

    } else {

        /* fprintf(stderr, "alloc ... %s\n", ObjStr(tov[1]));*/
        result = callMethod((ClientData) cl, interp,
                            XOTclGlobalObjects[XOTE_ALLOC], objc+1, tov+1, 0);
        if (result != TCL_OK)
            goto create_method_exit;

        nameObj = Tcl_GetObjResult(interp);
        if (XOTclObjConvertObject(interp, nameObj, &newobj) != TCL_OK) {
            result = XOTclErrMsg(interp, "couldn't find result of alloc", TCL_STATIC);
            goto create_method_exit;
        }

        /*(void)RemoveInstance(newobj, newobj->cl);*/ /* TODO needed? remove? */
        AddInstance(newobj, cl);
        objTrace("CREATE", newobj);

        if (data) {
            XOTclObjectOpt *objopt = XOTclRequireObjectOpt(newobj);
            objopt->clientData = data;
        }

        /* in case, the object is destroyed during initialization, we incr refcount */
        INCR_REF_COUNT(nameObj);
        result = doObjInitialization(interp, newobj, objc, objv);
        DECR_REF_COUNT(nameObj);
    }
 create_method_exit:

    /* fprintf(stderr, "create -- end ... %s\n", ObjStr(tov[1]));*/
    if (tmpObj)  {DECR_REF_COUNT(tmpObj);}
    FREE_ON_STACK(tov);
    return result;
}


static int
XOTclCCreateMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc < 2)
        return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "<obj> ?args?");
    
    if (RUNTIME_STATE(interp)->exitHandlerDestroyRound != XOTCL_EXITHANDLER_OFF) {
        fprintf(stderr,"### Can't create object %s during shutdown\n", ObjStr(objv[1]));
        return TCL_ERROR;
        return TCL_OK; /* don't fail, if this happens during destroy, it might be canceled */
    }
    
    return createMethod(interp, cl, NULL, objc, objv);
}

static int
XOTclCNewMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass  *cl = XOTclObjectToClass(clientData);
    XOTclObject *child = NULL;
    Tcl_Obj *fullname;
    int result, offset = 1,
#if REFCOUNTED
        isrefcount = 0,
#endif
        i, prefixLength;
    Tcl_DString dFullname, *dsPtr = &dFullname;
    XOTclStringIncrStruct *iss = &RUNTIME_STATE(interp)->iss;

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");

    if (objc < 1)
        return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "?-childof obj? ?args?");

    for (i=1; i<objc; i++) {
        char *option = ObjStr(objv[i]);
        if (*option == '-' && strcmp(option,"-childof")==0 && i<objc-1) {
            offset += 2;
            if (XOTclObjConvertObject(interp, objv[i+1], &child) != TCL_OK) {
                return XOTclErrMsg(interp, "not a valid object specified as child", TCL_STATIC);
            }
#if REFCOUNTED
        } else if (strcmp(option,"-refcount")==0) {
            isrefcount = 1;
            offset += 1;
#endif
        } else
            break;
    }

    Tcl_DStringInit(dsPtr);
    if (child) {
        Tcl_DStringAppend(dsPtr, ObjStr(child->cmdName), -1);
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

    objc -= offset;
    {
        ALLOC_ON_STACK(Tcl_Obj *, objc+3, ov);

        ov[0] = objv[0];
        ov[1] = XOTclGlobalObjects[XOTE_CREATE];
        ov[2] = fullname;
        if (objc >= 1)
            memcpy(ov+3, objv+offset, sizeof(Tcl_Obj *)*objc);

        result = DoDispatch(clientData, interp, objc+3, ov, 0);
        FREE_ON_STACK(ov);
    }

#if REFCOUNTED
    if (result == TCL_OK) {
        if (isrefcount) {
            Tcl_Obj *obj = Tcl_GetObjResult(interp);
            XOTclObject *o = (XOTclObject*) obj->internalRep.otherValuePtr;
            o->flags |= XOTCL_REFCOUNTED;
            o->teardown = in;
            DECR_REF_COUNT(obj);
        }
    }
#endif

    DECR_REF_COUNT(fullname);
    Tcl_DStringFree(dsPtr);

    return result;
}


static int
XOTclCRecreateMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);
    XOTclObject *newobj;
    int result;

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc < 2)
        return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "<obj> ?args?");

    if (XOTclObjConvertObject(interp, objv[1], &newobj) != TCL_OK)
        return XOTclVarErrMsg(interp, "can't recreate not existing obj ",
                              ObjStr(objv[1]), (char *) NULL);

    INCR_REF_COUNT(objv[1]);
    newobj->flags |= XOTCL_RECREATE;

    result = doCleanup(interp, newobj, &cl->object, objc, objv);
    if (result == TCL_OK) {
        result = doObjInitialization(interp, newobj, objc, objv);
        if (result == TCL_OK)
            Tcl_SetObjResult(interp, objv[1]);
    }
    DECR_REF_COUNT(objv[1]);
    return result;
}




static int
XOTclClassInfoHeritageMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> ?pattern?");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    return ListHeritage(interp, cl, objc == 3 ? ObjStr(objv[2]) : NULL);
}

static int
XOTclClassInfoInstancesMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> ?pattern?");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");
  
    return ListObjPtrHashTable(interp, &cl->instances, objc == 3 ? ObjStr(objv[2]) : NULL);
}


static int
XOTclClassInfoInstargsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    Tcl_Namespace *nsp;

    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> <methodName>");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class"); 

    if (cl->nonposArgsTable) {
        XOTclNonposArgs *nonposArgs = NonposArgsGet(cl->nonposArgsTable, ObjStr(objv[2]));
        if (nonposArgs && nonposArgs->ordinaryArgs) {
            return ListArgsFromOrdinaryArgs(interp, nonposArgs);
        }
    }
    nsp = cl->nsPtr;
    return ListProcArgs(interp, Tcl_Namespace_cmdTable(nsp), ObjStr(objv[2]));
}

static int
XOTclClassInfoInstbodyMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    Tcl_Namespace *nsp;

    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> <methodName>");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class"); 

    nsp = cl->nsPtr;
    return ListProcBody(interp, Tcl_Namespace_cmdTable(nsp), ObjStr(objv[2]));
}

static int
XOTclClassInfoInstcommandsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    Tcl_Namespace *nsp;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> ?pattern?");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    nsp = cl->nsPtr;
    return ListKeys(interp, Tcl_Namespace_cmdTable(nsp), objc == 3 ? ObjStr(objv[2]) : NULL);
}

static int
XOTclClassInfoInstdefaultMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    Tcl_Namespace *nsp;

    if (objc != 5) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> <methodName> <arg> <var>");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    if (cl->nonposArgsTable) {
        XOTclNonposArgs *nonposArgs = NonposArgsGet(cl->nonposArgsTable,  ObjStr(objv[2]));
        if (nonposArgs && nonposArgs->ordinaryArgs) {
            return ListDefaultFromOrdinaryArgs(interp, ObjStr(objv[2]), nonposArgs, ObjStr(objv[3]), objv[4]);
        }
    }
    nsp = cl->nsPtr;
    return nsp ? 
        ListProcDefault(interp, Tcl_Namespace_cmdTable(nsp), ObjStr(objv[2]), ObjStr(objv[3]), objv[4]) : 
        TCL_OK;
}

static int
XOTclClassInfoInstfilterMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    XOTclClassOpt *opt;
    int idx, nobjc, withGuards = 0;
    static CONST char *options[] = {"-guards", NULL};
    enum options {guardsIdx};

    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    for (idx = 2; idx < objc; idx++) {
        char *name;
        int index;

        name = Tcl_GetString(objv[idx]);
        if (name[0] != '-') {
            break;
        }
        if (Tcl_GetIndexFromObj(interp, objv[idx], options, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        switch ((enum options) index) {
        case guardsIdx: withGuards = 1; break;
        }
    }
    nobjc = objc - idx;
  
    if (objc < 2 || nobjc > 1 || objc > 4)
        return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> ?-guards? ?pattern?");

    opt = cl->opt;
    return opt ? FilterInfo(interp, opt->instfilters, idx < objc ? ObjStr(objv[idx]) : NULL, withGuards, 0) : TCL_OK;
}

static int
XOTclClassInfoInstfilterguardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    XOTclClassOpt *opt;
  
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> filter");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    opt = cl->opt;
    return opt ? GuardList(interp, opt->instfilters, ObjStr(objv[2])) : TCL_OK;
}

static int
XOTclClassInfoInstforwardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    Tcl_Namespace *nsp;
    int withDefinition = 0;
    char *name = NULL;

    if (objc < 2 || objc > 4) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> ?-definition? ?name?");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    if (objc > 2) {
        name = ObjStr(objv[2]);
        if (*name == '-' && !strcmp("-definition", name)) {
            withDefinition = 1;
            name = ObjStr(objv[3]);
        }
    }

    nsp = cl->nsPtr;
    return forwardList(interp, Tcl_Namespace_cmdTable(cl->nsPtr), name, withDefinition);
}

static int
XOTclClassInfoInstinvarMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    XOTclClassOpt *opt;

    if (objc != 2) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class>");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    opt = cl->opt;
    if (opt && opt->assertions) {
        Tcl_SetObjResult(interp, AssertionList(interp, opt->assertions->invariants));
    }
  
    return TCL_OK;
}

static int
XOTclClassInfoInstmixinMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    XOTclClassOpt *opt;
    int idx, nobjc, withGuards = 0;
    static CONST char *options[] = {"-guards", NULL};
    enum options {guardsIdx};

    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    for (idx = 2; idx < objc; idx++) {
        char *name;
        int index;

        name = Tcl_GetString(objv[idx]);
        if (name[0] != '-') {
            break;
        }
        if (Tcl_GetIndexFromObj(interp, objv[idx], options, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        switch ((enum options) index) {
        case guardsIdx: withGuards = 1; break;
        }
    }
    nobjc = objc-idx;
  
    if (objc < 2 || nobjc > 1 || objc > 4)
        return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> ?-guards? ?pattern?");
  
    opt = cl->opt;
    return opt ? MixinInfo(interp, opt->instmixins, idx < objc ? ObjStr(objv[idx]) : NULL, withGuards) : TCL_OK;
}

static int
XOTclClassInfoInstmixinguardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    XOTclClassOpt *opt;
  
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> mixin");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    opt = cl->opt;
    return opt ? GuardList(interp, opt->instmixins, ObjStr(objv[2])) : TCL_OK;
}


static int
XOTclClassInfoInstnonposargsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
  
    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> <methodName>");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    if (cl->nonposArgsTable) {
        XOTclNonposArgs *nonposArgs = NonposArgsGet(cl->nonposArgsTable, ObjStr(objv[2]));
        if (nonposArgs) {
            Tcl_SetObjResult(interp, NonposArgsFormat(interp, nonposArgs->nonposArgs));
        }
    }
    return TCL_OK;
}

static int
XOTclClassInfoInstprocsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> ?pattern?");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    return ListMethodKeys(interp, Tcl_Namespace_cmdTable(cl->nsPtr), 
                          objc == 3 ? ObjStr(objv[2]) : NULL,
                          /*noProcs*/ 0, /*noCmds*/ 1, NULL, 0 );
}

static int
XOTclClassInfoInstpreMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    XOTclClassOpt *opt;

    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0], objv[1], "<methodName>");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    opt = cl->opt;
    if (opt) {
        XOTclProcAssertion *procs = AssertionFindProcs(opt->assertions, ObjStr(objv[2]));
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->pre));
    }
    return TCL_OK;
}

static int
XOTclClassInfoInstpostMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;
    XOTclClassOpt *opt;

    if (objc != 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> <methodName>");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    opt = cl->opt;
    if (opt) {
        XOTclProcAssertion *procs = AssertionFindProcs(opt->assertions, ObjStr(objv[2]));
        if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->post));
    }
    return TCL_OK;
}

static int
XOTclClassInfoParameterMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    Tcl_DString ds, *dsPtr = &ds;
    XOTclClass *cl;
    XOTclObject *o;

    if (objc != 2) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class>");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    DSTRING_INIT(dsPtr);
    Tcl_DStringAppend(dsPtr, className(cl), -1);
    Tcl_DStringAppend(dsPtr, "::slot", 6);
    o = XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
    if (o) {
        Tcl_Obj *varNameObj = Tcl_NewStringObj("__parameter",-1);
        Tcl_Obj *parameters = XOTcl_ObjGetVar2((XOTcl_Object*)o, 
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

static int
XOTclClassInfoSuperclassMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> ?pattern?");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    return ListSuperclasses(interp, cl, objc == 3 ? ObjStr(objv[2]) : NULL);
}

static int
XOTclClassInfoSubclassMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl;

    if (objc < 2 || objc > 3) return XOTclObjErrArgCnt(interp, objv[0],  NULL, "<class> ?pattern?");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    return ListSubclasses(interp, cl, objc == 3 ? ObjStr(objv[2]) : NULL);
}

static int
XOTclClassInfoSlotsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    Tcl_DString ds, *dsPtr = &ds;
    XOTclClass *cl;
    XOTclObject *o;
    int rc;

    if (objc < 2) return XOTclObjErrArgCnt(interp, objv[0], NULL, "<class>");  
    if (GetXOTclClassFromObj(interp, objv[1], &cl, 0) != TCL_OK)
        return XOTclObjErrType(interp, objv[1], "Class");

    DSTRING_INIT(dsPtr);
    Tcl_DStringAppend(dsPtr, className(cl), -1);
    Tcl_DStringAppend(dsPtr, "::slot", 6);
    o = XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
    if (o) {
        rc = ListChildren(interp, o, NULL, 0);
    } else {
        rc = TCL_OK;
    }
    DSTRING_FREE(dsPtr);
    return rc;
}

static int
XOTclCInfoMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj  *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);
    Tcl_Namespace *nsp;
    XOTclClassOpt *opt;
    char *pattern, *cmd;
    int modifiers = 0;

    if (objc < 2)
        return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "<opt> ?args?");

    if (cl) {
        nsp = cl->nsPtr;
        opt = cl->opt;

        cmd = ObjStr(objv[1]);
        pattern = (objc > 2) ? ObjStr(objv[2]) : 0;

        /*
         * check for "-" modifiers
         */
        if (pattern && *pattern == '-') {
            modifiers = countModifiers(objc, objv);
            pattern = (objc > 2+modifiers) ? ObjStr(objv[2+modifiers]) : 0;
        }

        switch (*cmd) {
        case 'c':
            if (!strcmp(cmd, "classchildren")) {
                if (objc > 3 || modifiers > 0)
                    return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "classchildren ?pat?");
                return ListChildren(interp, (XOTclObject*) cl, pattern, 1);
            } else if (!strcmp(cmd, "classparent")) {
                if (objc > 2 || modifiers > 0)
                    return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "classparent");
                return ListParent(interp, &cl->object);
            }
            break;

        case 'h':
            if (!strcmp(cmd, "heritage")) {
                if (objc > 3 || modifiers > 0)
                    return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "heritage ?pat?");
                return ListHeritage(interp, cl, pattern);
            }
            break;

        case 'i':
            if (cmd[1] == 'n' && cmd[2] == 's' && cmd[3] == 't') {
                char *cmdTail = cmd + 4;
                switch (*cmdTail) {
                case 'a':
                    if (!strcmp(cmdTail, "ances")) {
                        if (objc > 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "instances ?pat?");
                        return ListObjPtrHashTable(interp, &cl->instances, pattern);
                    } else if (!strcmp(cmdTail, "args")) {
                        if (objc != 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instargs <instproc>");
                        if (cl->nonposArgsTable) {
                            XOTclNonposArgs *nonposArgs =
                                NonposArgsGet(cl->nonposArgsTable, pattern);
                            if (nonposArgs && nonposArgs->ordinaryArgs) {
                                return ListArgsFromOrdinaryArgs(interp, nonposArgs);
                            }
                        }
                        return ListProcArgs(interp, Tcl_Namespace_cmdTable(nsp), pattern);
                    }
                    break;
	
                case 'b':
                    if (!strcmp(cmdTail, "body")) {
                        if (objc != 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instbody <instproc>");
                        return ListProcBody(interp, Tcl_Namespace_cmdTable(nsp), pattern);
                    }
                    break;

                case 'c':
                    if (!strcmp(cmdTail, "commands")) {
                        if (objc > 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instcommands ?pat?");
                        return ListKeys(interp, Tcl_Namespace_cmdTable(nsp), pattern);
                    }
                    break;
	
                case 'd':
                    if (!strcmp(cmdTail, "default")) {
                        if (objc != 5 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instdefault <instproc> <arg> <var>");
	
                        if (cl->nonposArgsTable) {
                            XOTclNonposArgs *nonposArgs =
                                NonposArgsGet(cl->nonposArgsTable, pattern);
                            if (nonposArgs && nonposArgs->ordinaryArgs) {
                                return ListDefaultFromOrdinaryArgs(interp, pattern, nonposArgs,
                                                                   ObjStr(objv[3]), objv[4]);
                            }
                        }
                        return ListProcDefault(interp, Tcl_Namespace_cmdTable(nsp), pattern,
                                               ObjStr(objv[3]), objv[4]);
                    }
                    break;
	
                case 'f':
                    if (!strcmp(cmdTail, "filter")) {
                        int withGuards = 0;
                        if (objc-modifiers > 3)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instfilter ?-guards? ?pat?");
                        if (modifiers > 0) {
                            withGuards = checkForModifier(objv, modifiers, "-guards");
                            if (withGuards == 0)
                                return XOTclVarErrMsg(interp, "info instfilter: unknown modifier ",
                                                      ObjStr(objv[2]), (char *) NULL);
                        }
                        return opt ? FilterInfo(interp, opt->instfilters, pattern, withGuards, 0) : TCL_OK;
	
                    } else if (!strcmp(cmdTail, "filterguard")) {
                        if (objc != 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instfilterguard filter");
                        return opt ? GuardList(interp, opt->instfilters, pattern) : TCL_OK;
                    } else if (!strcmp(cmdTail, "forward")) {
                        int argc = objc-modifiers;
                        int definition;
                        if (argc < 2 || argc > 3)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instforward ?-definition? ?name?");
                        definition = checkForModifier(objv, modifiers, "-definition");
                        if (nsp)
                            return forwardList(interp, Tcl_Namespace_cmdTable(nsp), pattern, definition);
                        else
                            return TCL_OK;
                    }
                    break;

                case 'i':
                    if (!strcmp(cmdTail, "invar")) {
                        XOTclAssertionStore *assertions = opt ? opt->assertions : 0;
                        if (objc != 2 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instinvar");
	
                        if (assertions && assertions->invariants)
                            Tcl_SetObjResult(interp, AssertionList(interp, assertions->invariants));
                        return TCL_OK;
                    }
                    break;
	
                case 'm':
                    if (!strcmp(cmdTail, "mixin")) {
                        int withGuards = 0;
	
                        if (objc-modifiers > 3 || modifiers > 1)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instmixin ?-guards? ?class?");
                        if (modifiers > 0) {
                            withGuards = checkForModifier(objv, modifiers, "-guards");
                            if (withGuards == 0)
                                return XOTclVarErrMsg(interp, "info instfilter: unknown modifier ",
                                                      ObjStr(objv[2]), (char *) NULL);
                        }
                        return opt ? MixinInfo(interp, opt->instmixins, pattern, withGuards) : TCL_OK;
	
                    } else if (!strcmp(cmdTail, "mixinof")) {
                        /* TODO: make a method out of me */
                        if (objc-modifiers > 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName,  objv[0],
                                                     "instmixinof ?class?");
                        return opt ? MixinOfInfo(interp, opt->instmixinofs, pattern) : TCL_OK;

                    } else if (!strcmp(cmdTail, "mixinguard")) {
                        if (objc != 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instmixinguard mixin");
                        return opt ? GuardList(interp, opt->instmixins, pattern) : TCL_OK;
                    }
                    break;
	
                case 'n':
                    if (!strcmp(cmdTail, "nonposargs")) {
                        if (objc != 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instnonposargs <instproc>");
                        if (cl->nonposArgsTable) {
                            XOTclNonposArgs *nonposArgs =
                                NonposArgsGet(cl->nonposArgsTable, pattern);
                            if (nonposArgs) {
                                Tcl_SetObjResult(interp, NonposArgsFormat(interp,
                                                                          nonposArgs->nonposArgs));
                            }
                        }
                        return TCL_OK;
                    }
                    break;
	
                case 'p':
                    if (!strcmp(cmdTail, "procs")) {
                        if (objc > 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], 
                                                     "instprocs ?pat?");
                        return ListMethodKeys(interp, Tcl_Namespace_cmdTable(nsp), pattern,
                                              /*noProcs*/ 0, /*noCmds*/ 1, NULL, 0);
                    } else if (!strcmp(cmdTail, "pre")) {
                        XOTclProcAssertion *procs;
                        if (objc != 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instpre <proc>");
                        if (opt && opt->assertions) {
                            procs = AssertionFindProcs(opt->assertions, ObjStr(objv[2]));
                            if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->pre));
                        }
                        return TCL_OK;
                    } else if (!strcmp(cmdTail, "post")) {
                        XOTclProcAssertion *procs;
                        if (objc != 3 || modifiers > 0)
                            return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                                     "instpost <proc>");
                        if (opt && opt->assertions) {
                            procs = AssertionFindProcs(opt->assertions, ObjStr(objv[2]));
                            if (procs) Tcl_SetObjResult(interp, AssertionList(interp, procs->post));
                        }
                        return TCL_OK;
                    }
                    break;
                }
            }
            break;

        case 'm':
            if (!strcmp(cmd, "mixinof")) {
                /* TODO: make a method out of me */
                if (objc-modifiers > 3 || modifiers > 0)
                    return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                             "mixinof ?object?");
                return opt ? MixinOfInfo(interp, opt->mixinofs, pattern) : TCL_OK;
            }
            break;

        case 'p':
            if (!strcmp(cmd, "parameter")) {

                Tcl_DString ds, *dsPtr = &ds;
                XOTclObject *o;
                DSTRING_INIT(dsPtr);
                Tcl_DStringAppend(dsPtr, className(cl), -1);
                Tcl_DStringAppend(dsPtr, "::slot", 6);
                o = XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
                if (o) {
                    Tcl_Obj *varNameObj = Tcl_NewStringObj("__parameter",-1);
                    Tcl_Obj *parameters = XOTcl_ObjGetVar2((XOTcl_Object*)o, 
                                                           interp, varNameObj, NULL,
                                                           TCL_LEAVE_ERR_MSG);
                    if (parameters) {
                        Tcl_SetObjResult(interp, parameters);
                    } else {
                        fprintf(stderr, "info parameters: No value for %s\n",
                                Tcl_DStringValue(dsPtr));
                        Tcl_SetObjResult(interp, XOTclGlobalObjects[XOTE_EMPTY]);
                    }
                    DECR_REF_COUNT(varNameObj);
                } else {
                    Tcl_SetObjResult(interp, XOTclGlobalObjects[XOTE_EMPTY]);
                }
                DSTRING_FREE(dsPtr);
#if 0
                if (cl->parameters) {
                    Tcl_SetObjResult(interp, cl->parameters);
                } else {
                    Tcl_SetObjResult(interp, XOTclGlobalObjects[XOTE_EMPTY]);
                }
#endif
                return TCL_OK;
            }
            break;

        case 's':
            if (!strcmp(cmd, "superclass")) {
                if (objc > 3 || modifiers > 0)
                    return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                             "superclass ?class?");
                return ListSuperclasses(interp, cl, pattern);
            } else if (!strcmp(cmd, "subclass")) {
                if (objc > 3 || modifiers > 0)
                    return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                             "subclass ?class?");
                return ListSubclasses(interp, cl, pattern);
            } else if (!strcmp(cmd, "slots")) {
                Tcl_DString ds, *dsPtr = &ds;
                XOTclObject *o;
                int rc;
                DSTRING_INIT(dsPtr);
                Tcl_DStringAppend(dsPtr, className(cl), -1);
                Tcl_DStringAppend(dsPtr, "::slot", 6);
                o = XOTclpGetObject(interp, Tcl_DStringValue(dsPtr));
                if (o) {
                    rc = ListChildren(interp, o, NULL, 0);
                } else {
                    rc = TCL_OK;
                }
                DSTRING_FREE(dsPtr);
                return rc;
            }
            break;
        }
    }

    return XOTclOInfoMethod(clientData, interp, objc, (Tcl_Obj **)objv);
}

static int
XOTclCInstParameterCmdMethod(ClientData clientData, Tcl_Interp *interp,
			     int objc, Tcl_Obj  *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc < 2) return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0], "name");
    XOTclAddInstanceMethod(interp, (XOTcl_Class*) cl, ObjStr(objv[1]),
                           (Tcl_ObjCmdProc*)XOTclSetterMethod, 0, 0, 0);
    return TCL_OK;
}

static int
XOTclCParameterCmdMethod(ClientData clientData, Tcl_Interp *interp,
			 int objc, Tcl_Obj  *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*) clientData;

    if (objc < 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "name");
    XOTclAddObjectMethod(interp, (XOTcl_Object*) obj, ObjStr(objv[1]),
                         (Tcl_ObjCmdProc*)XOTclSetterMethod, 0, 0, 0);
    return TCL_OK;
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
forwardProcessOptions(Tcl_Interp *interp, int objc, Tcl_Obj  *CONST objv[],
		      forwardCmdClientData **tcdp) {
    forwardCmdClientData *tcd;
    int i, rc = 0, earlybinding = 0;

    tcd = NEW(forwardCmdClientData);
    memset(tcd, 0, sizeof(forwardCmdClientData));

    for (i=2; i<objc; i++) {
        /*fprintf(stderr, "   processing '%s'\n", ObjStr(objv[i]));*/
        if (!strcmp(ObjStr(objv[i]),"-default")) {
            if (objc <= i+1) {rc = TCL_ERROR; break;}
            tcd->subcommands = objv[i+1];
            rc = Tcl_ListObjLength(interp, objv[i+1],&tcd->nr_subcommands);
            if (rc != TCL_OK) break;
            INCR_REF_COUNT(tcd->subcommands);
            i++;
        } else if (!strcmp(ObjStr(objv[i]),"-methodprefix")) {
            if (objc <= i+1) {rc = TCL_ERROR; break;}
            tcd->prefix = objv[i+1];
            INCR_REF_COUNT(tcd->prefix);
            i++;
        } else if (!strcmp(ObjStr(objv[i]),"-onerror")) {
            if (objc <= i+1) {rc = TCL_ERROR; break;}
            tcd->onerror = objv[i+1];
            INCR_REF_COUNT(tcd->onerror);
            i++;
        } else if (!strcmp(ObjStr(objv[i]),"-objscope")) {
            tcd->objscope = 1;
        } else if (!strcmp(ObjStr(objv[i]),"-earlybinding")) {
            earlybinding = 1;
        } else if (!strcmp(ObjStr(objv[i]),"-verbose")) {
            tcd->verbose = 1;
        } else {
            /* todo protected */
            break;
        }
    }

    tcd->needobjmap = 0;
    for (; i<objc; i++) {
        char *element = ObjStr(objv[i]);
        tcd->needobjmap |= (*element == '%' && *(element+1) == '@');

        if (tcd->cmdName == 0) {
            tcd->cmdName = objv[i];
        } else if (tcd->args == 0) {
            tcd->args = Tcl_NewListObj(1, &objv[i]);
            tcd->nr_args++;
            INCR_REF_COUNT(tcd->args);
        } else {
            Tcl_ListObjAppendElement(interp, tcd->args, objv[i]);
            tcd->nr_args++;
        }
    }

    if (!tcd->cmdName) {
        tcd->cmdName = objv[1];
    }

    if (tcd->objscope) {
        /* when we evaluating objscope, and define ...
           o forward append -objscope append
           a call to
           o append ...
           would lead to a recursive call; so we add the appropriate namespace
        */
        char *name = ObjStr(tcd->cmdName);
        if (!isAbsolutePath(name)) {
            tcd->cmdName = NameInNamespaceObj(interp, name, callingNameSpace(interp));
            /*fprintf(stderr,"name %s not absolute, therefore qualifying %s\n", name,
              ObjStr(tcd->cmdName));*/
        }
    }
    INCR_REF_COUNT(tcd->cmdName);

    if (earlybinding) {
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
    
    tcd->passthrough = !tcd->args && *(ObjStr(tcd->cmdName)) != '%' && tcd->objProc != NULL;

    /*fprintf(stderr, "forward args = %p, name = '%s'\n", tcd->args, ObjStr(tcd->cmdName));*/
    if (rc == TCL_OK) {
        *tcdp = tcd;
    } else {
        forwardCmdDeleteProc((ClientData)tcd);
    }
    return rc;
}


static int
XOTclCInstForwardMethod(ClientData clientData, Tcl_Interp *interp,
                        int objc, Tcl_Obj  *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);
    forwardCmdClientData *tcd;
    int rc;

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc < 2) goto forward_argc_error;
    rc = forwardProcessOptions(interp, objc, objv, &tcd);

    if (rc == TCL_OK) {
        tcd->obj = &cl->object;
        XOTclAddInstanceMethod(interp, (XOTcl_Class*) cl, NSTail(ObjStr(objv[1])),
                               (Tcl_ObjCmdProc*)XOTclForwardMethod,
                               (ClientData)tcd, forwardCmdDeleteProc, 0);
        return TCL_OK;
    } else {
    forward_argc_error:
        return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                 "method ?target? ?-default name? ?-objscope? ?-onerror cmd? ?-methodprefix string? ?args?");
    }
}

static int
XOTclOForwardMethod(ClientData clientData, Tcl_Interp *interp,
                    int objc, Tcl_Obj  *CONST objv[]) {
    XOTcl_Object *obj = (XOTcl_Object*) clientData;
    forwardCmdClientData *tcd;
    int rc;

    if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    if (objc < 2) goto forward_argc_error;

    rc = forwardProcessOptions(interp, objc, objv, &tcd);

    if (rc == TCL_OK) {
        tcd->obj = (XOTclObject*)obj;
        XOTclAddObjectMethod(interp, obj, NSTail(ObjStr(objv[1])),
                             (Tcl_ObjCmdProc*)XOTclForwardMethod,
                             (ClientData)tcd, forwardCmdDeleteProc, 0);
        return TCL_OK;
    } else {
    forward_argc_error:
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0],
                                 "method ?target? ?-default name? ?-objscope? ?-methodprefix string? ?args?");
    }
}


static int
XOTclOVolatileMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj  *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*) clientData;
    Tcl_Obj *o = obj->cmdName;
    int result = TCL_ERROR;
    char *fullName = ObjStr(o);
    char *vn;
    callFrameContext ctx = {0};

    if (objc != 1) 
        return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "");
    
    if (RUNTIME_STATE(interp)->exitHandlerDestroyRound != XOTCL_EXITHANDLER_OFF) {
        fprintf(stderr,"### Can't make objects volatile during shutdown\n");
        return XOTclVarErrMsg(interp, "Can't make objects volatile during shutdown\n", NULL);
    }

    CallStackUseActiveFrames(interp, &ctx);
    vn = NSTail(fullName);

    if (Tcl_SetVar2(interp, vn, NULL, fullName, 0) != NULL) {
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

static int
makeMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int clsns) {

    XOTclClass *cl = XOTclObjectToClass(clientData);
    char *argStr, *bdyStr, *name;
    XOTclClassOpt *opt;
    int incr = 0, result = TCL_OK;

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc < 4 || objc > 7)
        return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                 "name ?non-positional-args? args body ?preAssertion postAssertion?");

    if (objc == 5 || objc == 7) {
        incr = 1;
    }

    argStr = ObjStr(objv[2 + incr]);
    bdyStr = ObjStr(objv[3 + incr]);
    name = ObjStr(objv[1 + incr]);

    if ((cl == RUNTIME_STATE(interp)->theObject && isDestroyString(name)) ||
        (cl == RUNTIME_STATE(interp)->theClass && isInstDestroyString(name)) ||
        (cl == RUNTIME_STATE(interp)->theClass && isDeallocString(name)) ||
        (cl == RUNTIME_STATE(interp)->theClass && isAllocString(name)) ||
        (cl == RUNTIME_STATE(interp)->theClass && isCreateString(name)))
        return XOTclVarErrMsg(interp, className(cl), " method '", name, "' of ",
                              className(cl), " can not be overwritten. Derive a ",
                              "sub-class", (char *) NULL);

    if (*argStr == 0 && *bdyStr == 0) {
        int rc;
        opt = cl->opt;
        if (opt && opt->assertions)
            AssertionRemoveProc(opt->assertions, name);
        rc = NSDeleteCmd(interp, cl->nsPtr, name);
        if (rc < 0)
            return XOTclVarErrMsg(interp, className(cl), " cannot delete method '", name,
                                  "' of class ", className(cl), (char *) NULL);
    } else {
        XOTclAssertionStore *aStore = NULL;
        if (objc > 5) {
            opt = XOTclRequireClassOpt(cl);
            if (!opt->assertions)
                opt->assertions = AssertionCreateStore();
            aStore = opt->assertions;
        }
        result = MakeProc(cl->nsPtr, aStore, &(cl->nonposArgsTable),
                          interp, objc, (Tcl_Obj **) objv, &cl->object, clsns);
    }

    /* could be a filter or filter inheritance ... update filter orders */
    FilterInvalidateObjOrders(interp, cl);

    return result;
}


static int
XOTclCInstProcMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    return makeMethod(clientData, interp, objc, objv, 0);
}

static int
XOTclCInstProcMethodC(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    return makeMethod(clientData, interp, objc, objv, 1);
}

static int
XOTclCInstFilterGuardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);
    XOTclCmdList *h;
    XOTclClassOpt *opt;

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc != 3) return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                            "filtername filterGuard");

    opt = cl->opt;
    if (opt && opt->instfilters) {
        h = CmdListFindNameInList(interp, ObjStr(objv[1]), opt->instfilters);
        if (h) {
            if (h->clientData)
                GuardDel(h);
            GuardAdd(interp, h, objv[2]);
            FilterInvalidateObjOrders(interp, cl);
            return TCL_OK;
        }
    }

    return XOTclVarErrMsg(interp, "Instfilterguard: can't find filter ",
                          ObjStr(objv[1]), " on ", ObjStr(cl->object.cmdName),
                          (char *) NULL);
}


static int
XOTclCInstMixinGuardMethod(ClientData clientData, Tcl_Interp *interp, 
                           int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);
    XOTclCmdList *h;

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc != 3) return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                            "mixin guard");

    if (cl->opt && cl->opt->instmixins) {
        XOTclClass *mixinCl = XOTclpGetClass(interp, ObjStr(objv[1]));
        Tcl_Command mixinCmd = NULL;
        if (mixinCl) {
            mixinCmd = Tcl_GetCommandFromObj(interp, mixinCl->object.cmdName);
        }
        if (mixinCmd) {
            h = CmdListFindCmdInList(mixinCmd, cl->opt->instmixins);
            if (h) {
                if (h->clientData)
                    GuardDel((XOTclCmdList*) h);
                GuardAdd(interp, h, objv[2]);
                MixinInvalidateObjOrders(interp, cl);
                return TCL_OK;
            }
        }
    }

    return XOTclVarErrMsg(interp, "Instmixinguard: can't find mixin ",
                          ObjStr(objv[1]), " on ", ObjStr(cl->object.cmdName), 
                          (char *) NULL);
}

static int
XOTclCInvariantsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclClass *cl = XOTclObjectToClass(clientData);
    XOTclClassOpt *opt;

    if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
    if (objc != 2)
        return XOTclObjErrArgCnt(interp, cl->object.cmdName, objv[0],
                                 "<invariantList>");
    opt = XOTclRequireClassOpt(cl);

    if (opt->assertions)
        TclObjListFreeList(opt->assertions->invariants);
    else
        opt->assertions = AssertionCreateStore();

    opt->assertions->invariants = AssertionNewList(interp, objv[1]);
    return TCL_OK;
}

static int
XOTclCUnknownMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *obj = (XOTclObject*) clientData;
    char *self = ObjStr(obj->cmdName);
    int rc;

    if (objc < 2) return XOTclObjErrArgCnt(interp, obj->cmdName, objv[0], "message ?args .. args?");
    if (isCreateString(self))
        return XOTclVarErrMsg(interp, "error ", self, ": unable to dispatch '",
                              ObjStr(objv[1]), "'", (char *) NULL);

    rc = callMethod(clientData, interp, XOTclGlobalObjects[XOTE_CREATE], objc+1, objv+1, 0);
    return rc;
}

/*
 * New Tcl Commands
 */
static int
XOTcl_NSCopyCmds(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    Tcl_Command cmd;
    Tcl_Obj *newFullCmdName, *oldFullCmdName;
    char *newName, *oldName, *name;
    Tcl_Namespace *ns, *newNs;
    Tcl_HashTable *cmdTable;
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr;

    if (objc != 3)
        return XOTclObjErrArgCnt(interp, objv[0], NULL, "<fromNs> <toNs>");

    ns = ObjFindNamespace(interp, objv[1]);
    if (!ns)
        return TCL_OK;

    newNs = ObjFindNamespace(interp, objv[2]);
    if (!newNs)
        return XOTclVarErrMsg(interp, "CopyCmds: Destination namespace ",
                              ObjStr(objv[2]), " does not exist", (char *) NULL);
    /*
     * copy all procs & commands in the ns
     */
    cmdTable = Tcl_Namespace_cmdTable(ns);
    hPtr = Tcl_FirstHashEntry(cmdTable, &hSrch);

    /*fprintf(stderr,"copy cmdTable = %p, first=%p\n", cmdTable, hPtr);*/
    while (hPtr != NULL) {
        name = Tcl_GetHashKey(cmdTable, hPtr);

        /*
         * construct full cmd names
         */
        newFullCmdName = Tcl_NewStringObj(newNs->fullName,-1);
        oldFullCmdName = Tcl_NewStringObj(ns->fullName,-1);

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

        if (cmd != NULL) {
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
                Proc *procPtr = TclFindProc((Interp *)interp, oldName);
                Tcl_Obj *arglistObj;
                CompiledLocal *localPtr;

                /*
                 * Build a list containing the arguments of the proc
                 */

                arglistObj = Tcl_NewListObj(0, NULL);
                INCR_REF_COUNT(arglistObj);

                for (localPtr = procPtr->firstLocalPtr;  localPtr != NULL;
                     localPtr = localPtr->nextPtr) {

                    if (TclIsCompiledLocalArgument(localPtr)) {
                        Tcl_Obj *defVal, *defStringObj = Tcl_NewStringObj(localPtr->name, -1);
                        INCR_REF_COUNT(defStringObj);
                        /* check for default values */
                        if ((GetProcDefault(interp, cmdTable, name,
                                            localPtr->name, &defVal) == TCL_OK) &&
                            (defVal != 0)) {
                            Tcl_AppendStringsToObj(defStringObj, " ", ObjStr(defVal), 
                                                   (char *) NULL);
                        }
                        Tcl_ListObjAppendElement(interp, arglistObj, defStringObj);
                        DECR_REF_COUNT(defStringObj);
                    }
                }
	
                if (Tcl_Command_objProc(cmd) == RUNTIME_STATE(interp)->objInterpProc) {
                    Tcl_DString ds, *dsPtr = &ds;

                    if (isClassName(ns->fullName)) {
                        /* it started with ::xotcl::classes */
                        XOTclClass *cl = XOTclpGetClass(interp, NSCutXOTclClasses(ns->fullName));
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
                        Tcl_DStringAppendElement(dsPtr, NSCutXOTclClasses(newNs->fullName));
                        Tcl_DStringAppendElement(dsPtr, "instproc");
                        Tcl_DStringAppendElement(dsPtr, name);
                        Tcl_DStringAppendElement(dsPtr, ObjStr(arglistObj));
                        Tcl_DStringAppendElement(dsPtr,  StripBodyPrefix(ObjStr(procPtr->bodyPtr)));
                        if (procs) {
                            XOTclRequireClassOpt(cl);
                            AssertionAppendPrePost(interp, dsPtr, procs);
                        }
                        Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), Tcl_DStringLength(dsPtr), 0);
                        DSTRING_FREE(dsPtr);
                    } else {
                        XOTclObject *obj = XOTclpGetObject(interp, ns->fullName);
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
                        Tcl_DStringAppendElement(dsPtr, newNs->fullName);
                        Tcl_DStringAppendElement(dsPtr, "proc");
                        Tcl_DStringAppendElement(dsPtr, name);
                        Tcl_DStringAppendElement(dsPtr, ObjStr(arglistObj));
                        Tcl_DStringAppendElement(dsPtr, StripBodyPrefix(ObjStr(procPtr->bodyPtr)));
                        if (procs) {
                            XOTclRequireObjectOpt(obj);
                            AssertionAppendPrePost(interp, dsPtr, procs);
                        }
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
                    if (clientData == 0) {
                        /* if client data not null, we would have to copy
                           the client data; we don't know its size...., so rely
                           on introspection for copying */
                        Tcl_CreateObjCommand(interp, newName, objProc,
                                             Tcl_Command_objClientData(cmd), deleteProc);
                    }
                } else {
                    clientData = Tcl_Command_clientData(cmd);
                    if (clientData == 0) {
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
    Tcl_Namespace *ns, *newNs;
    Var *varPtr = NULL;
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr;
    TclVarHashTable *varTable;
    int rc = TCL_OK;
    XOTclObject *obj;
    char *destFullName;
    Tcl_Obj *destFullNameObj;
    TclCallFrame frame, *framePtr = &frame;
    Tcl_Obj *varNameObj = NULL;
    Tcl_Obj *nobjv[4];
    int nobjc;
    Tcl_Obj *setObj;

    if (objc != 3)
        return XOTclObjErrArgCnt(interp, objv[0], NULL, "<fromNs> <toNs>");

    ns =  ObjFindNamespace(interp, objv[1]);
    if (ns) {
        newNs = ObjFindNamespace(interp, objv[2]);
        if (!newNs)
            return XOTclVarErrMsg(interp, "CopyVars: Destination namespace ",
                                  ObjStr(objv[2]), " does not exist", (char *) NULL);

        obj = XOTclpGetObject(interp, ObjStr(objv[1]));
        destFullName = newNs->fullName;
	destFullNameObj = Tcl_NewStringObj(destFullName, -1);
	INCR_REF_COUNT(destFullNameObj);
	varTable = Tcl_Namespace_varTable(ns);
	Tcl_PushCallFrame(interp,(Tcl_CallFrame *)framePtr, newNs, 0);
    } else {
        XOTclObject *newObj;
        if (XOTclObjConvertObject(interp, objv[1], &obj) != TCL_OK) {
            return XOTclVarErrMsg(interp, "CopyVars: Origin object/namespace ",
                                  ObjStr(objv[1]), " does not exist", (char *) NULL);
        }
        if (XOTclObjConvertObject(interp, objv[2], &newObj) != TCL_OK) {
            return XOTclVarErrMsg(interp, "CopyVars: Destination object/namespace ",
                                  ObjStr(objv[2]), " does not exist", (char *) NULL);
        }
        varTable = obj->varTable;
        destFullNameObj = newObj->cmdName;
        destFullName = ObjStr(destFullNameObj);
    }

    setObj= Tcl_NewStringObj("set", 3);
    INCR_REF_COUNT(setObj);
    nobjc = 4;
    nobjv[0] = destFullNameObj;
    nobjv[1] = setObj;

    /* copy all vars in the ns */
    hPtr = varTable ? Tcl_FirstHashEntry(VarHashTable(varTable), &hSrch) : NULL;
    while (hPtr != NULL) {

        getVarAndNameFromHash(hPtr, &varPtr, &varNameObj);
	INCR_REF_COUNT(varNameObj);

        if (!TclIsVarUndefined(varPtr) && !TclIsVarLink(varPtr)) {
            if (TclIsVarScalar(varPtr)) {
                /* it may seem odd that we do not copy obj vars with the
                 * same SetVar2 as normal vars, but we want to dispatch it in order to
                 * be able to intercept the copying */

                if (obj) {
#if 1
                    /* can't rely on "set", if there are multiple object systems */
                    setInstVar(interp, obj, varNameObj, valueOfVar(Tcl_Obj, varPtr, objPtr));
#else
		    nobjv[2] = varNameObj;
		    nobjv[3] = valueOfVar(Tcl_Obj, varPtr, objPtr);
		    rc = Tcl_EvalObjv(interp, nobjc, nobjv, 0);
#endif
                } else {
                    Tcl_ObjSetVar2(interp, varNameObj, NULL, 
                                   valueOfVar(Tcl_Obj, varPtr, objPtr),
                                   TCL_NAMESPACE_ONLY);
                }
            } else {
                if (TclIsVarArray(varPtr)) {
		    /* //// HERE!! PRE85 Why not [array get/set] based? Let the core iterate */
		    TclVarHashTable *aTable = valueOfVar(TclVarHashTable, varPtr, tablePtr);
                    Tcl_HashSearch ahSrch;
		    Tcl_HashEntry *ahPtr = aTable ? Tcl_FirstHashEntry(VarHashTable(aTable), &ahSrch) : NULL;
	
                    for (; ahPtr != 0; ahPtr = Tcl_NextHashEntry(&ahSrch)) {
		        Tcl_Obj *eltNameObj;
			Var *eltVar;

			getVarAndNameFromHash(ahPtr, &eltVar, &eltNameObj);
			INCR_REF_COUNT(eltNameObj);

			if (TclIsVarScalar(eltVar)) {
			    if (obj) {
#if 1
			        XOTcl_ObjSetVar2((XOTcl_Object*)obj, interp, varNameObj, eltNameObj, valueOfVar(Tcl_Obj, eltVar, objPtr), 0);
#else
				Tcl_Obj *fullVarNameObj = Tcl_DuplicateObj(varNameObj);
 		
				INCR_REF_COUNT(fullVarNameObj);
				Tcl_AppendStringsToObj(fullVarNameObj, "(", 
						       ObjStr(eltNameObj), ")", NULL);
				nobjv[2] = fullVarNameObj;
				nobjv[3] = valueOfVar(Tcl_Obj, eltVar, objPtr);
 
				rc = Tcl_EvalObjv(interp, nobjc, nobjv, 0);
				DECR_REF_COUNT(fullVarNameObj);
#endif
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
    if (ns) {
        DECR_REF_COUNT(destFullNameObj);
	Tcl_PopCallFrame(interp);
    }
    DECR_REF_COUNT(setObj);
    return rc;
}

int
XOTclSelfDispatchCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    XOTclObject *self;
    int result;
    if (objc < 2) return XOTclObjErrArgCnt(interp, objv[0], NULL, "?-local? method ?args?");
    if ((self = GetSelfObj(interp))) {
        int i = 1;
        char *arg1 = ObjStr(objv[1]);
        if (*arg1 == '-' && !strcmp("-local", arg1)) {
            XOTclClass *cl = GetSelfClass(interp);
            Tcl_Command cmd;
            ClientData cp;
            char *method;
            if (objc < 3) return XOTclObjErrArgCnt(interp, objv[0], NULL, "?-local? method ?args?");
            method = ObjStr(objv[2]);
            i++;
            cmd = FindMethod(method, cl->nsPtr);
            if (cmd == 0)
                return XOTclVarErrMsg(interp, ObjStr(self->cmdName), 
                                      ": unable to dispatch local method '",
                                      method, "' in class ", ObjStr(cl->object.cmdName), 
                                      (char *) NULL);
            cp   = Tcl_Command_objClientData(cmd);
            /*fprintf(stderr, "method %s, cmd = %p objc=%d\n", method, cmd, objc);
              for (i=0; i<objc; i++) {fprintf(stderr,"%d: %s\n", i, ObjStr(objv[i]));} */
            result = DoCallProcCheck(cp, clientData, interp, objc-1, objv+1, cmd, self, cl,
                                     method, 0, 0 /* fromNext */, 0);
        } else {
            /*fprintf(stderr,"MY self=%p %s objc=%d, 1='%s'\n", self, ObjStr(self->cmdName), objc, ObjStr(objv[1]));
              {int i; fprintf(stderr, "MY\tCALL ");for(i=0; i<objc; i++) {fprintf(stderr,"%s ", ObjStr(objv[i]));} fprintf(stderr,"\n");}*/
            result = callMethod((ClientData)self, interp, objv[1], objc, objv+2, 0);
        }
    } else {
        result = XOTclVarErrMsg(interp, "Cannot resolve 'self', probably called outside the context of an XOTcl Object",
                                (char *) NULL);
    }
    return result;
}

int
XOTclInitProcNSCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *) Tcl_Interp_varFramePtr(interp);

    /*RUNTIME_STATE(interp)->varFramePtr = varFramePtr;*/
#if 0
    Tcl_Namespace *nsPtr = Tcl_Command_nsPtr(RUNTIME_STATE(interp)->cs.top->cmdPtr);
    fprintf(stderr,"initProcNS self=%s cmd=%p, '%s'\n",
            ObjStr(RUNTIME_STATE(interp)->cs.top->self->cmdName),
            nsPtr, nsPtr->fullName);
    fprintf(stderr,"\tsetting currentFramePtr in %p to %p in initProcNS\n",
            RUNTIME_STATE(interp)->cs.top->currentFramePtr, varFramePtr);
    XOTclCallStackDump(interp);
#endif

    if (RUNTIME_STATE(interp)->cs.top->currentFramePtr == 0) {
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

/*
 * Interpretation of Non-Positional Args
 */
int
isNonposArg(Tcl_Interp *interp, char  *argStr,
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

int
XOTclCheckBooleanArgs(ClientData clientData, Tcl_Interp *interp, int objc,
		      Tcl_Obj *CONST objv[]) {
    int result, bool;
    Tcl_Obj *boolean;

    if (objc == 2) {
        /* the variable is not yet defined (set), so we cannot check
           whether it is boolean or not */
        return TCL_OK;
    } else if (objc != 3) {
        return XOTclObjErrArgCnt(interp,  NULL, objv[0], "name ?value?");
    }

    boolean = Tcl_DuplicateObj(objv[2]);
    INCR_REF_COUNT(boolean);
    result = Tcl_GetBooleanFromObj(interp, boolean, &bool);
    DECR_REF_COUNT(boolean);
    /*
      if (result != TCL_OK)
      return XOTclVarErrMsg(interp,
      "non-positional argument: '", ObjStr(objv[1]), "' with value '",
      ObjStr(objv[2]), "' is not of type boolean",
      (char *) NULL);
    */
    Tcl_ResetResult(interp);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), (result == TCL_OK));
    return TCL_OK;
}

int
XOTclCheckRequiredArgs(ClientData clientData, Tcl_Interp *interp, int objc,
		       Tcl_Obj *CONST objv[]) {
    if (objc != 2 && objc != 3)
        return XOTclObjErrArgCnt(interp, NULL, objv[0], "<args> ?currentValue?");

    if (objc != 3)
        return XOTclVarErrMsg(interp,
                              "required arg '", ObjStr(objv[1]), "' missing",
                              (char *) NULL);
    Tcl_ResetResult(interp);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    return TCL_OK;
}

int
XOTclInterpretNonpositionalArgsCmd(ClientData clientData, Tcl_Interp *interp, int objc,
				   Tcl_Obj *CONST objv[]) {
    Tcl_Obj **npav, **checkv, **checkArgv, **argsv, **nonposArgsDefv,
        *invocation[4], **ordinaryArgsDefv, **defaultValueObjv, *list,
        *checkObj, *ordinaryArg;
    int npac, checkc, checkArgc, argsc, nonposArgsDefc,
        ordinaryArgsDefc, defaultValueObjc, argsDefined = 0,
        ordinaryArgsCounter = 0, i, j, result, ic;
    char  *lastDefArg = NULL, *arg, *argStr;
    int endOfNonposArgsReached = 0;
    Var *varPtr;

    XOTclClass *selfClass = GetSelfClass(interp);
    char *methodName = (char *) GetSelfProc(interp);
    Tcl_HashTable *nonposArgsTable;
    XOTclNonposArgs *nonposArgs;
    XOTclObject *selfObj;
    int r1, r2, r3, r4;

    if (objc != 2)
        return XOTclObjErrArgCnt(interp, objv[0], NULL, "?args?");

    if (selfClass) {
        nonposArgsTable = selfClass->nonposArgsTable;
    } else if ((selfObj = GetSelfObj(interp))) {
        nonposArgsTable = selfObj->nonposArgsTable;
    } else {
        return XOTclVarErrMsg(interp, "Non-positional args: can't find self/self class",
                              (char *) NULL);
    }

    nonposArgs = NonposArgsGet(nonposArgsTable, methodName);
    if (nonposArgs == 0) {
        return XOTclVarErrMsg(interp,
                              "Non-positional args: can't find hash entry for: ",
                              methodName,
                              (char *) NULL);
    }

    fprintf(stderr,"interpretnonposargs nonposargs = %s, ordinaryArgs = %s\n",
            ObjStr(nonposArgs->nonposArgs), ObjStr(nonposArgs->ordinaryArgs));

    r1 = Tcl_ListObjGetElements(interp, nonposArgs->nonposArgs,
                                &nonposArgsDefc, &nonposArgsDefv);
    r2 = Tcl_ListObjGetElements(interp, nonposArgs->ordinaryArgs,
                                &ordinaryArgsDefc, &ordinaryArgsDefv);
    r3 = Tcl_ListObjGetElements(interp, objv[1], &argsc, &argsv);
  
  
    if (r1 != TCL_OK || r2 != TCL_OK || r3 != TCL_OK) {
        return XOTclVarErrMsg(interp,
                              "Cannot split non-positional args list: ",
                              methodName,
                              (char *) NULL);
    }

    /* setting variables to default values */
    for (i=0; i < nonposArgsDefc; i++) {
        r1 = Tcl_ListObjGetElements(interp, nonposArgsDefv[i], &npac, &npav);
        if (r1 == TCL_OK) {
            if (npac == 3) {
                /* not sure, whether Tcl_ObjSetVar2 would be better or would
                   cause shimmering between list elements and vars (2 times);
                   however, Tcl_ObjSetVar2 seems to have a problem since 
                   a later eval does not see the variable...
                */
                Tcl_SetVar2Ex(interp, ObjStr(npav[0]), NULL, npav[2], 0);
            } else if (npac == 2 && !strcmp(ObjStr(npav[1]), "switch")) {
                /* default for switch is "off" */
                Tcl_SetVar2Ex(interp, ObjStr(npav[0]), NULL, Tcl_NewBooleanObj(0), 0);
            }
        }
    }

    if (ordinaryArgsDefc > 0) {
        lastDefArg = ObjStr(ordinaryArgsDefv[ordinaryArgsDefc-1]);
        if (isArgsString(lastDefArg)) {
            argsDefined = 1;
        }
    }

    /* setting specified variables */
    for (i=0; i < argsc; i++) {

        if (!endOfNonposArgsReached) {
            char *type;
            Tcl_Obj *var;
            argStr = ObjStr(argsv[i]);

            if (isDoubleDashString(argStr)) {
                endOfNonposArgsReached = 1;
                i++;
            }
            if (isNonposArg(interp, argStr, nonposArgsDefc, nonposArgsDefv, &var,&type)) {
                if (*type == 's' && !strcmp(type, "switch")) {
                    int bool;
		    Tcl_Obj *boolObj = Tcl_ObjGetVar2(interp, var, 0, 0);
		    if (Tcl_GetBooleanFromObj(interp, boolObj, &bool) != TCL_OK) {
                        return XOTclVarErrMsg(interp, "Non positional arg '", argStr,
                                              "': no boolean value", (char *) NULL);
		    }
		    Tcl_ObjSetVar2(interp, var, NULL, Tcl_NewBooleanObj(!bool), 0);
                } else {
                    i++;
                    if (i >= argsc)
                        return XOTclVarErrMsg(interp, "Non positional arg '",
                                              argStr, "' missing", (char *) NULL);
                    Tcl_ObjSetVar2(interp, var, NULL, argsv[i], 0);
                }
            } else {
                endOfNonposArgsReached = 1;
            }
        }
    
        if (endOfNonposArgsReached && i < argsc) {
            if (ordinaryArgsCounter >= ordinaryArgsDefc) {
                Tcl_Obj *tmp = NonposArgsFormat(interp, nonposArgs->nonposArgs);
                XOTclVarErrMsg(interp, "unknown argument '",
                               ObjStr(argsv[i]),
                               "' for method '", 
                               methodName,
                               "': valid arguments ",
                               ObjStr(tmp),
                               " ",
                               ObjStr(nonposArgs->ordinaryArgs),
                               (char *) NULL);
                DECR_REF_COUNT(tmp);
                return TCL_ERROR;
            }
            arg = ObjStr(ordinaryArgsDefv[ordinaryArgsCounter]);
            /* this is the last arg and 'args' is defined */
            if (argsDefined && ordinaryArgsCounter+1 == ordinaryArgsDefc) {
                list = Tcl_NewListObj(0, NULL);
                INCR_REF_COUNT(list);
                for(; i < argsc; i++) {
                    Tcl_ListObjAppendElement(interp, list, argsv[i]);
		}
                Tcl_ObjSetVar2(interp, ordinaryArgsDefv[ordinaryArgsCounter], NULL, list, 0);
                DECR_REF_COUNT(list);
            } else {
                /* break down this argument, if it has a default value,
                   use only the first part */
                ordinaryArg = ordinaryArgsDefv[ordinaryArgsCounter];
                r4 = Tcl_ListObjGetElements(interp, ordinaryArg,
                                            &defaultValueObjc, &defaultValueObjv);
                if (r4 == TCL_OK && defaultValueObjc == 2) {
                    ordinaryArg = defaultValueObjv[0];
                }
                Tcl_ObjSetVar2(interp, ordinaryArg, NULL, argsv[i], 0);
            }
            ordinaryArgsCounter++;
        }
    }

    /*fprintf(stderr,"... args defined %d argsc=%d  oa %d oad %d\n",
      argsDefined, argsc,
      ordinaryArgsCounter, ordinaryArgsDefc); */

    if ((!argsDefined && ordinaryArgsCounter != ordinaryArgsDefc) ||
        (argsDefined && ordinaryArgsCounter < ordinaryArgsDefc-1)) {

        /* we do not have enough arguments, maybe there are default arguments
           for the missing args */
        while (ordinaryArgsCounter != ordinaryArgsDefc) {
            if (argsDefined && ordinaryArgsCounter+1 == ordinaryArgsDefc) 
                break;
            r4 = Tcl_ListObjGetElements(interp, ordinaryArgsDefv[ordinaryArgsCounter],
                                        &defaultValueObjc, &defaultValueObjv);
            /*fprintf(stderr,"... try to get default for '%s', rc %d, objc %d\n",
	      ObjStr(ordinaryArgsDefv[ordinaryArgsCounter]), 
	      r4, defaultValueObjc);*/
            if (r4 == TCL_OK && defaultValueObjc == 2) {
	        Tcl_ObjSetVar2(interp, defaultValueObjv[0], NULL, defaultValueObjv[1], 0);
            } else {
                Tcl_Obj *tmp = NonposArgsFormat(interp, nonposArgs->nonposArgs);
                XOTclVarErrMsg(interp, "wrong # args for method '",
                               methodName, "': valid arguments ", ObjStr(tmp), " ",
                               ObjStr(nonposArgs->ordinaryArgs),
                               (char *) NULL);
                DECR_REF_COUNT(tmp);
                return TCL_ERROR;
            }
            ordinaryArgsCounter++;
        }
        if (argsDefined) {
            Tcl_SetVar2(interp, "args", NULL, "", 0);
        }
    } else if (argsDefined && ordinaryArgsCounter == ordinaryArgsDefc-1) {
        Tcl_SetVar2(interp, "args", NULL, "", 0);
    }

    if (!argsDefined) {
        Tcl_UnsetVar2(interp, "args", NULL, 0);
    }

    /* checking vars */
    for (i=0; i < nonposArgsDefc; i++) {
        r1 = Tcl_ListObjGetElements(interp, nonposArgsDefv[i], &npac, &npav);
        if (r1 == TCL_OK &&  npac > 1 && *(ObjStr(npav[1])) != '\0') {
            r1 = Tcl_ListObjGetElements(interp, npav[1], &checkc, &checkv);
            if (r1 == TCL_OK) {
                int checkResult = 0;
                checkObj = nonposArgs->slotObj == NULL ? 
                    XOTclGlobalObjects[XOTE_NON_POS_ARGS_OBJ] :
                    nonposArgs->slotObj;
                for (j=0; j < checkc; j++) {
                    r1 = Tcl_ListObjGetElements(interp, checkv[j], &checkArgc, &checkArgv);
                    if (r1 == TCL_OK && checkArgc > 1) {
                        if (isCheckObjString((ObjStr(checkArgv[0]))) && checkArgc == 2) {
                            checkObj = checkArgv[1];
                            continue;
                        }
                    }
                    invocation[0] = checkObj;
                    invocation[1] = checkv[j];
                    varPtr = TclVarTraceExists(interp, ObjStr(npav[0]));
                    invocation[2] = npav[0];
                    ic = 3;
                    if ((varPtr != NULL) && !TclIsVarUndefined(varPtr)) {
                        invocation[3] = Tcl_ObjGetVar2(interp, npav[0], 0, 0);
                        ic = 4;
                    }
                    result = Tcl_EvalObjv(interp, ic, invocation, 0);
                    /*
                      objPtr = Tcl_ConcatObj(ic, invocation);
                      fprintf(stderr,"eval on <%s>\n", ObjStr(objPtr));
                      result = Tcl_EvalObjEx(interp, objPtr, TCL_EVAL_DIRECT);
                    */
                    if (result == TCL_OK && ic == 4) {
                        result = Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp),&checkResult);
                    }
                    if (result != TCL_OK) {
                        return result;
                    }
                    if (!checkResult && ic == 4) {
                        return XOTclVarErrMsg(interp,
                                              "non-positional argument: '", ObjStr(invocation[2]), "' with value '",
                                              ObjStr(invocation[3]), "' is not of ", ObjStr(invocation[1]),
                                              (char *) NULL);
                    }
                }
            }
        }
    }
    return TCL_OK;
}


/* create a slave interp that calls XOTcl Init */
static int
XOTcl_InterpObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    Tcl_Interp *slave;
    char *subCmd;
    ALLOC_ON_STACK(Tcl_Obj *, objc, ov);

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
        /*fprintf(stderr,"checkallinstances %d cl=%p '%s'\n", lvl, cl, ObjStr(cl->object.cmdName));*/
        for (hPtr = Tcl_FirstHashEntry(&cl->instances, &search);  hPtr != NULL;
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
static void deleteProcsAndVars(Tcl_Interp *interp) {
    Tcl_Namespace *ns = Tcl_GetGlobalNamespace(interp);
    Tcl_HashTable *varTable = ns ? Tcl_Namespace_varTable(ns) : NULL;
    Tcl_HashTable *cmdTable = ns ? Tcl_Namespace_cmdTable(ns) : NULL;
    Tcl_HashSearch search;
    Var *varPtr;
    Tcl_Command cmd;
    register Tcl_HashEntry *entryPtr;
    char *varName;

    entryPtr = Tcl_FirstHashEntry(varTable, &search);
    while (entryPtr != NULL) {
        Tcl_Obj *nameObj;
	getVarAndNameFromHash(entryPtr, &varPtr, &nameObj);
	if (!TclIsVarUndefined(varPtr) || TclIsVarNamespaceVar(varPtr)) {
            /* fprintf(stderr, "unsetting var %s\n", ObjStr(nameObj));*/
            Tcl_UnsetVar2(interp, ObjStr(nameObj), (char *)NULL, TCL_GLOBAL_ONLY);
	}
	entryPtr = Tcl_NextHashEntry(&search);
    }
    entryPtr = Tcl_FirstHashEntry(cmdTable, &search);
    while (entryPtr) {
        cmd = (Tcl_Command)Tcl_GetHashValue(entryPtr);

        if (Tcl_Command_objProc(cmd) == RUNTIME_STATE(interp)->objInterpProc) {
            char *key = Tcl_GetHashKey(cmdTable, entryPtr);

            /*fprintf(stderr, "cmdname = %s cmd %p proc %p objProc %p %d\n",
	      key, cmd, Tcl_Command_proc(cmd), Tcl_Command_objProc(cmd),
	      Tcl_Command_proc(cmd)==RUNTIME_STATE(interp)->objInterpProc);*/
	
            Tcl_DeleteCommandFromToken(interp, cmd);
        }
        entryPtr = Tcl_NextHashEntry(&search);
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

static void freeAllXOTclObjectsAndClasses(Tcl_Interp *interp, Tcl_HashTable *commandTable) {
    Tcl_HashEntry *hPtr, *hDel;
    Tcl_HashSearch hSrch;
    XOTclObject *obj;
    XOTclClass  *thecls, *theobj, *cl;

    /* fprintf(stderr,"??? freeAllXOTclObjectsAndClasses in %p\n", interp); */

    thecls = RUNTIME_STATE(interp)->theClass;
    theobj = RUNTIME_STATE(interp)->theObject;
    /***** PHYSICAL DESTROY *****/
    RUNTIME_STATE(interp)->exitHandlerDestroyRound = XOTCL_EXITHANDLER_ON_PHYSICAL_DESTROY;
    while (1) {
        int deleted = 0;
        hPtr = Tcl_FirstHashEntry(commandTable, &hSrch);
        while (hPtr) {
            char *key = Tcl_GetHashKey(commandTable, hPtr);
            obj = XOTclpGetObject(interp, key);
            if (obj && !XOTclObjectIsClass(obj) && !ObjectHasChildren(interp, obj)) {
                /*fprintf(stderr,"  ... delete object %s %p, class=%s\n", key, obj, 
                  ObjStr(obj->cl->object.cmdName));*/
                freeUnsetTraceVariable(interp, obj);
                Tcl_DeleteCommandFromToken(interp, obj->id);
                hDel = hPtr;
                deleted++;
            } else {
                hDel = NULL;
            }
            hPtr = Tcl_NextHashEntry(&hSrch);
            if (hDel)
                Tcl_DeleteHashEntry(hDel);
        }
        /*fprintf(stderr, "deleted %d Objects\n", deleted);*/
        if (deleted>0)
            continue;


        hPtr = Tcl_FirstHashEntry(commandTable, &hSrch);
        while (hPtr) {
            char *key = Tcl_GetHashKey(commandTable, hPtr);
            cl = XOTclpGetClass(interp, key);
            /* fprintf(stderr,"cl key = %s %p\n", key, cl); */
            if (cl
                && !ObjectHasChildren(interp, (XOTclObject*)cl)
                && !ClassHasInstances(cl)
                && !ClassHasSubclasses(cl)
                && cl != RUNTIME_STATE(interp)->theClass
                && cl != RUNTIME_STATE(interp)->theObject
                ) {
                /* fprintf(stderr,"  ... delete class %s %p\n", key, cl); */
                freeUnsetTraceVariable(interp, &cl->object);
                Tcl_DeleteCommandFromToken(interp, cl->object.id);
                hDel = hPtr;
                deleted++;
            } else {
                hDel = NULL;
            }
            hPtr = Tcl_NextHashEntry(&hSrch);
            if (hDel)
                Tcl_DeleteHashEntry(hDel);
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
    RemoveSuper(thecls, theobj);
    RemoveInstance((XOTclObject*)thecls, thecls);
    RemoveInstance((XOTclObject*)theobj, thecls);

    Tcl_DeleteCommandFromToken(interp, theobj->object.id);
    RUNTIME_STATE(interp)->theObject = NULL;

    Tcl_DeleteCommandFromToken(interp, thecls->object.id);
    RUNTIME_STATE(interp)->theClass = NULL;

    XOTcl_DeleteNamespace(interp, RUNTIME_STATE(interp)->fakeNS);
    XOTcl_DeleteNamespace(interp, RUNTIME_STATE(interp)->XOTclClassesNS);
    XOTcl_DeleteNamespace(interp, RUNTIME_STATE(interp)->XOTclNS);

}
#endif /* DO_CLEANUP */


/*
 *  Exit Handler
 */
static void
ExitHandler(ClientData clientData) {
    Tcl_Interp *interp = (Tcl_Interp *) clientData;
    XOTclObject *obj;
    XOTclClass *cl;
    int result, flags, i;
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr;
    Tcl_HashTable objTable, *commandTable = &objTable;
    XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;

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
     * there is no such hook in Tcl as of Tcl8.3.2, that I know of.
     *
     * So, for the rest of procedure, assume the interp is alive !
     */

    /*fprintf(stderr,"+++ EXIT handler\n"); */
    flags = Tcl_Interp_flags(interp);
    Tcl_Interp_flags(interp) &= ~DELETED;
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
    /*
     * Pop any callstack entry that is still alive (e.g.
     * if "exit" is called and we were jumping out of the
     * callframe
     */
    while (cs->top > cs->content)
        CallStackPop(interp);

    while (1) {
        Tcl_CallFrame *f = Tcl_Interp_framePtr(interp);
        if (!f) break;
        if (Tcl_CallFrame_level(f) == 0) break;
        Tcl_PopCallFrame(interp);
    }

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
    getAllInstances(interp, commandTable, RUNTIME_STATE(interp)->theClass, 0);

    /***** SOFT DESTROY *****/
    RUNTIME_STATE(interp)->exitHandlerDestroyRound = XOTCL_EXITHANDLER_ON_SOFT_DESTROY;

    hPtr = Tcl_FirstHashEntry(commandTable, &hSrch);
    while (hPtr) {
        char *key = Tcl_GetHashKey(commandTable, hPtr);
        obj = XOTclpGetObject(interp, key);
        /*fprintf(stderr,"key = %s %p %d\n",
          key, obj, obj && !XOTclObjectIsClass(obj));*/
        if (obj && !XOTclObjectIsClass(obj)
            && !(obj->flags & XOTCL_DESTROY_CALLED))
            callDestroyMethod((ClientData)obj, interp, obj, XOTCL_CM_NO_UNKNOWN);
        hPtr = Tcl_NextHashEntry(&hSrch);
    }

    hPtr = Tcl_FirstHashEntry(commandTable, &hSrch);
    while (hPtr) {
        char *key = Tcl_GetHashKey(commandTable, hPtr);
        cl = XOTclpGetClass(interp, key);
        if (cl
            && !(cl->object.flags & XOTCL_DESTROY_CALLED)) {
            /*fprintf(stderr,"call destroy key = %s, cl = %p\n", key, cl);*/
            callDestroyMethod((ClientData)cl, interp, (XOTclObject*)cl, XOTCL_CM_NO_UNKNOWN);
        }
        hPtr = Tcl_NextHashEntry(&hSrch);
    }

#ifdef DO_CLEANUP
    freeAllXOTclObjectsAndClasses(interp, commandTable);
#endif

    /* must be before freeing of XOTclGlobalObjects */
    XOTclShadowTclCommands(interp, SHADOW_UNLOAD);
    /* free global objects */
    for (i = 0; i < nr_elements(XOTclGlobalStrings); i++) {
        DECR_REF_COUNT(XOTclGlobalObjects[i]);
    }
    XOTclStringIncrFree(&RUNTIME_STATE(interp)->iss);
    FREE(Tcl_Obj **, XOTclGlobalObjects);
    FREE(XOTclRuntimeState, RUNTIME_STATE(interp));

    MEM_COUNT_FREE("Tcl_InitHashTable", commandTable);
    Tcl_DeleteHashTable(commandTable);

#if defined(TCL_MEM_DEBUG)
    TclDumpMemoryInfo(stderr);
    Tcl_DumpActiveMemory("./xotclActiveMem");
    /* Tcl_GlobalEval(interp, "puts {checkmem to checkmemFile};
       checkmem checkmemFile"); */
#endif
    MEM_COUNT_DUMP();

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
#if !defined(PRE83)
    void XOTcl_ExitProc(ClientData clientData);
    Tcl_DeleteExitHandler(XOTcl_ExitProc, clientData);
#endif
    ExitHandler(clientData);
}
#endif

/*
 * Gets activated at application-exit
 */
void
XOTcl_ExitProc(ClientData clientData) {
    /*fprintf(stderr,"+++ XOTcl_ExitProc\n");*/
#if !defined(PRE83) && defined(TCL_THREADS)
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
#if !defined(PRE83) && defined(TCL_THREADS)
    Tcl_CreateThreadExitHandler(XOTcl_ThreadExitProc, clientData);
#endif
    Tcl_CreateExitHandler(XOTcl_ExitProc, clientData);
}

int
XOTclCreateObjectSystem(Tcl_Interp *interp, char *Object, char *Class) {
    XOTclClass *theobj = 0;
    XOTclClass *thecls = 0;  
  
    /* create Object and Class, and store them in the RUNTIME STATE */
    theobj = PrimitiveCCreate(interp, Object, 0);
    RUNTIME_STATE(interp)->theObject = theobj;
    if (!theobj) panic("Cannot create base Object class", 0);

    thecls = PrimitiveCCreate(interp, Class, 0);
    RUNTIME_STATE(interp)->theClass = thecls;
    if (!thecls) panic("Cannot create base Class", 0);

    /*theobj->parent = 0;
      thecls->parent = theobj;*/

    /*Tcl_AddInterpResolvers(interp, "XOTcl", XOTclResolveCmd, 0, 0);*/

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

        return XOTclErrMsg(interp, "Object/Class failed", TCL_STATIC);
    }

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

#ifndef PRE81
# ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
        return TCL_ERROR;
    }
# endif
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
            fprintf(stderr, "loading a version of xotcl compiled for 8.4 version into a 8.4 Tcl\n");
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
            fprintf(stderr, "loading a version of xotcl compiled for 8.4 version into a 8.5 Tcl\n");
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

    /* CallStack initialization */
    memset(RUNTIME_STATE(interp), 0, sizeof(XOTclRuntimeState));
    memset(RUNTIME_STATE(interp)->cs.content, 0, sizeof(XOTclCallStackContent));

    RUNTIME_STATE(interp)->cs.top = RUNTIME_STATE(interp)->cs.content;
    RUNTIME_STATE(interp)->doFilters = 1;
    RUNTIME_STATE(interp)->callDestroy = 1;

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
    RUNTIME_STATE(interp)->fakeNS =
        Tcl_CreateNamespace(interp, "::xotcl::fakeNS", (ClientData)NULL,
                            (Tcl_NamespaceDeleteProc *)NULL);
    MEM_COUNT_ALLOC("TclNamespace", RUNTIME_STATE(interp)->fakeNS);

    /* XOTclClasses in separate Namespace / Objects */
    RUNTIME_STATE(interp)->XOTclClassesNS =
        Tcl_CreateNamespace(interp, "::xotcl::classes",	(ClientData)NULL,
                            (Tcl_NamespaceDeleteProc *)NULL);
    MEM_COUNT_ALLOC("TclNamespace", RUNTIME_STATE(interp)->XOTclClassesNS);


    /* cache interpreters proc interpretation functions */
    RUNTIME_STATE(interp)->objInterpProc = TclGetObjInterpProc();
#if USE_INTERP_PROC
    RUNTIME_STATE(interp)->interpProc = TclGetInterpProc();
#endif
    RUNTIME_STATE(interp)->exitHandlerDestroyRound = XOTCL_EXITHANDLER_OFF;

    RegisterObjTypes();
    RegisterExitHandlers((ClientData)interp);

    XOTclStringIncrInit(&RUNTIME_STATE(interp)->iss);

    /* initialize global Tcl_Obj */
    XOTclGlobalObjects = NEW_ARRAY(Tcl_Obj *, nr_elements(XOTclGlobalStrings));

    for (i = 0; i < nr_elements(XOTclGlobalStrings); i++) {
        XOTclGlobalObjects[i] = Tcl_NewStringObj(XOTclGlobalStrings[i],-1);
        INCR_REF_COUNT(XOTclGlobalObjects[i]);
    }
#if defined(OO)
    Tcl_CreateNamespace(interp, "::oo", (ClientData)NULL, (Tcl_NamespaceDeleteProc*)NULL);
    XOTclCreateObjectSystem(interp, "::oo::object", "::oo::class");
#else 
    XOTclCreateObjectSystem(interp, "::xotcl::Object", "::xotcl::Class");
#endif

    {
        typedef struct methodDefinition {
            char *methodName;
            Tcl_ObjCmdProc *proc;
        } methodDefinition;

        char *namespace_names[] = {
            "::xotcl::cmd::Object",
            "::xotcl::cmd::Class", 
            "::xotcl::cmd::NonposArgs",
            "::xotcl::cmd::ObjectInfo",
            "::xotcl::cmd::ClassInfo"
        };

        methodDefinition definitions1[] = {
            {"autoname",         XOTclOAutonameMethod},
            {"check",            XOTclOCheckMethod},
            {"cleanup",          XOTclOCleanupMethod},
            {"configure",        XOTclOConfigureMethod},
            {"destroy",          XOTclODestroyMethod},
            {"exists",           XOTclOExistsMethod},
            {"filterguard",      XOTclOFilterGuardMethod},
            {"filtersearch",     XOTclOFilterSearchMethod},
            {"info",             XOTclOInfoMethod},
            {"instvar",          XOTclOInstVarMethod},
            {"invar",            XOTclOInvariantsMethod},
            {"isclass",          XOTclOIsClassMethod},
            {"ismetaclass",      XOTclOIsMetaClassMethod},
            {"isobject",         XOTclOIsObjectMethod},
            {"istype",           XOTclOIsTypeMethod},
            {"ismixin",          XOTclOIsMixinMethod},
#ifdef XOTCL_METADATA
            {"metadata",         XOTclOMetaDataMethod},
#endif
            {"mixinguard",       XOTclOMixinGuardMethod},
            {"__next",           XOTclONextMethod},
            /*    {"next",         XOTclONextMethod2},*/ 
            {"noinit",           XOTclONoinitMethod},
            {"parametercmd",     XOTclCParameterCmdMethod},
            {"proc",             XOTclOProcMethod},
            {"procsearch",       XOTclOProcSearchMethod},
            {"requireNamespace", XOTclORequireNamespaceMethod},
            {"set",              XOTclOSetMethod}, /***??**/
            {"setvalues",        XOTclOSetvaluesMethod},
            {"forward",          XOTclOForwardMethod},
            {"uplevel",          XOTclOUplevelMethod},
            {"upvar",            XOTclOUpvarMethod},
            {"volatile",         XOTclOVolatileMethod},
            {"vwait",            XOTclOVwaitMethod} 
        };
        methodDefinition definitions2[] = {
            {"alloc",            XOTclCAllocMethod},
            {"create",           XOTclCCreateMethod},
            {"dealloc",          XOTclCInstDestroyMethod},
            {"new",              XOTclCNewMethod},
            {"info",             XOTclCInfoMethod},
            {"instdestroy",      XOTclCInstDestroyMethod},
            {"instfilterguard",  XOTclCInstFilterGuardMethod},
            {"instinvar",        XOTclCInvariantsMethod},
            {"instmixinguard",   XOTclCInstMixinGuardMethod},
            {"instparametercmd", XOTclCInstParameterCmdMethod},
            {"instproc",         XOTclCInstProcMethod},
            {"classscopedinstproc", XOTclCInstProcMethodC},
            {"instforward",      XOTclCInstForwardMethod},
            {"recreate",         XOTclCRecreateMethod},
            {"unknown",          XOTclCUnknownMethod}
        };
        methodDefinition definitions3[] = {
            {"type=required", XOTclCheckRequiredArgs},
            {"type=switch",   XOTclCheckBooleanArgs},
            {"type=boolean",  XOTclCheckBooleanArgs}
        };

        methodDefinition definitions4[] = {
            {"args",         XOTclObjInfoArgsMethod},
            {"body",         XOTclObjInfoBodyMethod},
            {"class",        XOTclObjInfoClassMethod},
            {"commands",     XOTclObjInfoCommandsMethod},
            {"children",     XOTclObjInfoChildrenMethod},
            {"check",        XOTclObjInfoCheckMethod},
            {"default",      XOTclObjInfoDefaultMethod},
            {"filter",       XOTclObjInfoFilterMethod},
            {"filterguard",  XOTclObjInfoFilterguardMethod},
            {"forward",      XOTclObjInfoForwardMethod},
            {"hasNamespace", XOTclObjInfoHasnamespaceMethod},
            /*{"info",         XOTclObjInfoInfoMethod},*/
            {"invar",        XOTclObjInfoInvarMethod},
            {"methods",      XOTclObjInfoMethodsMethod},
            {"mixin",        XOTclObjInfoMixinMethod},
            {"mixinguard",   XOTclObjInfoMixinguardMethod},
            {"methods",      XOTclObjInfoMethodsMethod},
            {"nonposargs",   XOTclObjInfoNonposargsMethod},
            {"parent",       XOTclObjInfoParentMethod},
            {"post",         XOTclObjInfoPostMethod},
            {"pre",          XOTclObjInfoPreMethod},
            {"procs",        XOTclObjInfoProcsMethod},
            {"precedence",   XOTclObjInfoPrecedenceMethod},
            {"slotobjects",  XOTclObjInfoSlotObjectsMethod},
            {"vars",         XOTclObjInfoVarsMethod}
        };

        methodDefinition definitions5[] = {
            {"heritage",        XOTclClassInfoHeritageMethod},
            {"instances",       XOTclClassInfoInstancesMethod},
            {"instargs",        XOTclClassInfoInstargsMethod},
            {"instbody",        XOTclClassInfoInstbodyMethod},
            {"instcommands",    XOTclClassInfoInstcommandsMethod},
            {"instdefault",     XOTclClassInfoInstdefaultMethod},
            {"instfilter",      XOTclClassInfoInstfilterMethod},
            {"instfilterguard", XOTclClassInfoInstfilterguardMethod},
            {"instforward",     XOTclClassInfoInstforwardMethod},
            {"instinvar",       XOTclClassInfoInstinvarMethod},
            {"instmixin",       XOTclClassInfoInstmixinMethod},
            {"instmixinguard",  XOTclClassInfoInstmixinguardMethod},
            {"instprocs",       XOTclClassInfoInstprocsMethod},
            {"instnonposargs",  XOTclClassInfoInstnonposargsMethod},
            {"instpre",         XOTclClassInfoInstpreMethod},
            {"instpost",        XOTclClassInfoInstpostMethod},
            {"parameter",       XOTclClassInfoParameterMethod},
            {"subclass",        XOTclClassInfoSubclassMethod},
            {"superclass",      XOTclClassInfoSuperclassMethod},
            {"slots",           XOTclClassInfoSlotsMethod}
        };

        methodDefinition *definitions[] = {definitions1, definitions2, definitions3, 
                                           definitions4, definitions5};
        int nr_definitions[] = {nr_elements(definitions1), nr_elements(definitions2), 
                                nr_elements(definitions3), nr_elements(definitions4),
                                nr_elements(definitions5)};

        int namespacelength;
        Tcl_DString ds, *dsPtr = &ds;

        Tcl_CreateNamespace(interp, "::xotcl::cmd", 0, (Tcl_NamespaceDeleteProc*)NULL);

        DSTRING_INIT(dsPtr);
        for (i=0; i < nr_elements(namespace_names); i++) {
            int j;
            Tcl_DStringAppend(dsPtr, namespace_names[i], -1);
            /*fprintf(stderr,"namespace '%s'\n", namespace_names[i]);*/
            Tcl_CreateNamespace(interp, Tcl_DStringValue(dsPtr), 0, (Tcl_NamespaceDeleteProc*)NULL);
            Tcl_DStringAppend(dsPtr,"::", 2);
            namespacelength = Tcl_DStringLength(dsPtr);
            for (j = 0; j < nr_definitions[i]; j++) {
                Tcl_DStringAppend(dsPtr, definitions[i][j].methodName, -1);
                /*fprintf(stderr,"defining '%s' (%d of %d)\n", Tcl_DStringValue(dsPtr), j, nr_definitions[i]);*/
                Tcl_CreateObjCommand(interp, Tcl_DStringValue(dsPtr), definitions[i][j].proc, 0, 0);
                Tcl_DStringSetLength(dsPtr, namespacelength);
            }
            Tcl_DStringSetLength(dsPtr, 0);
        }

        DSTRING_FREE(dsPtr);
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
    instructions[INST_SELF_DISPATCH].cmdPtr = (Command *)
#endif
        Tcl_CreateObjCommand(interp, "::xotcl::my", XOTclSelfDispatchCmd, 0, 0);
#ifdef XOTCL_BYTECODE
    instructions[INST_NEXT].cmdPtr = (Command *)
#endif
        Tcl_CreateObjCommand(interp, "::xotcl::next", XOTclNextObjCmd, 0, 0);
#ifdef XOTCL_BYTECODE
    instructions[INST_SELF].cmdPtr = (Command *)
#endif
        Tcl_CreateObjCommand(interp, "::xotcl::self", XOTclGetSelfObjCmd, 0, 0);

    Tcl_CreateObjCommand(interp, "::xotcl::alias", XOTclAliasCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::dispatch", XOTclDispatchCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::methodproperty", XOTclMethodPropertyCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::configure", XOTclConfigureCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::deprecated", XOTcl_DeprecatedCmd, 0, 0);
#ifdef XOTCL_BYTECODE
    instructions[INST_INITPROC].cmdPtr = (Command *)
#endif
        Tcl_CreateObjCommand(interp, "::xotcl::initProcNS", XOTclInitProcNSCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::interpretNonpositionalArgs",
                         XOTclInterpretNonpositionalArgsCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::interp", XOTcl_InterpObjCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::namespace_copyvars", XOTcl_NSCopyVars, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::namespace_copycmds", XOTcl_NSCopyCmds, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::__qualify", XOTclQualifyObjCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::relation", XOTclRelationCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::setinstvar", XOTclSetInstvarCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::trace", XOTcl_TraceObjCmd, 0, 0);
    Tcl_CreateObjCommand(interp, "::xotcl::is", XOTclIsCmd, 0, 0);

    Tcl_Export(interp, RUNTIME_STATE(interp)->XOTclNS, "self", 0);
    Tcl_Export(interp, RUNTIME_STATE(interp)->XOTclNS, "next", 0);
    Tcl_Export(interp, RUNTIME_STATE(interp)->XOTclNS, "my", 0);

#ifdef XOTCL_BYTECODE
    XOTclBytecodeInit();
#endif

    /*
     * set runtime version information in Tcl variable
     */
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
        if (Tcl_GlobalEval(interp, cmd) != TCL_OK)
            return TCL_ERROR;
    }

#ifndef AOL_SERVER
    /* the AOL server uses a different package loading mechanism */
# ifdef COMPILE_XOTCL_STUBS
    Tcl_PkgProvideEx(interp, "XOTcl", PACKAGE_VERSION, (ClientData)&xotclStubs);
# else
    Tcl_PkgProvide(interp, "XOTcl", PACKAGE_VERSION);
# endif
#endif

#if !defined(TCL_THREADS) && !defined(PRE81)
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

