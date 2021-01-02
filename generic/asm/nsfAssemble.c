/*
 *      nsfAssemble.c --
 *
 *      Support for the experimental assemble feature. This file is
 *      only included in the source when NSF_ASSEMBLE is turned on.
 *
 * Copyright (C) 2011-2014 Gustaf Neumann
 */

//#define LABEL_THREADING

#if defined(LABEL_THREADING)
typedef void (* InstLabel)();
#endif

#define ASM_INFO_DECL  0x0001
#define ASM_INFO_PAIRS 0x0002
#define ASM_INFO_SKIP1 0x0004

#define ASM_SLOT_MUST_DECR  0x0001
#define ASM_SLOT_IS_INTEGER 0x0010

typedef struct AsmStatementInfo {
  int flags;
  CONST char **argTypes;
  int minArgs;
  int maxArgs;
  int cArgs;
} AsmStatementInfo;

typedef struct AsmInstruction {
  Tcl_ObjCmdProc *cmd;
  ClientData clientData;
  int argc;
  Tcl_Obj **argv;
#if defined(LABEL_THREADING)
  int labelIdx;
#endif
} AsmInstruction;

typedef struct AsmArgReference {
  int argNr;
  Tcl_Obj **objPtr;
} AsmArgReference;

#define NSF_ASM_NR_STATIC_SLOTS 30
typedef struct AsmCompiledProc {
  struct AsmInstruction *ip;  /* pointer to the next writable instruction */
  struct AsmInstruction *code;
  NsfObject *currentObject;
  int status;
  int nrLocals;
  Tcl_Obj **firstObj;      /* pointer to staticObjs */
  Tcl_Obj **locals;        /* pointer to staticObjs */
  Tcl_Obj **slots;         /* pointer to staticObjs */
  int slotFlags[NSF_ASM_NR_STATIC_SLOTS];       /* same size as allocated slots */
  Tcl_Obj *staticObjs[NSF_ASM_NR_STATIC_SLOTS]; // static objs for the time being TODO overflows/dynamic
  int nrAsmArgReferences;
  struct AsmArgReference argReferences[10]; // for the time being TODO overflows/dynamic
} AsmCompiledProc;

typedef struct AsmPatches {
  int targetAsmInstruction;
  int sourceAsmInstruction;
  int argvIndex;
} AsmPatches;

typedef struct AsmProcClientData {
  NsfObject *object;   /* common field of TclCmdClientData */
  AsmCompiledProc *proc;
  NsfParamDefs *paramDefs;
  int with_ad;
  int with_checkAlways;
} AsmProcClientData;

typedef struct AsmResolverInfo {
  Tcl_Command cmd;
  NsfObject *object;
  AsmCompiledProc *proc;
} AsmResolverInfo;

#if defined(LABEL_THREADING)
# define AsmInstructionNew(proc, instruction, argc) AsmInstructionNewLT(proc, IDX_ ## instruction, argc)
# define AsmInstructionSetCmd(inst, name) inst->labelIdx = IDX_ ## name
# define NsfAsmJump(index) ip = &proc->code[(index)]
# define NsfAsmJumpNext() ip++

AsmInstruction *AsmInstructionNewLT(AsmCompiledProc *proc, int labelIdx, int argc) {
  proc->ip->labelIdx = labelIdx;
  proc->ip->cmd = NULL;
  proc->ip->argc = argc;
  proc->ip->argv = proc->firstObj;
  proc->firstObj += argc;
  return proc->ip++;
}

#else
# define AsmInstructionNew(proc, instruction, argc) AsmInstructionNewCT(proc, (Tcl_ObjCmdProc*)instruction, argc)
# define AsmInstructionSetCmd(inst, name) inst->cmd = (Tcl_ObjCmdProc*)name
# define NsfAsmJump(index) proc->ip = &proc->code[(index)-1]
# define NsfAsmJumpNext()

AsmInstruction *AsmInstructionNewCT(AsmCompiledProc *proc, Tcl_ObjCmdProc *objProc, int argc) {
  proc->ip->cmd  = objProc;
  proc->ip->argc = argc;
  proc->ip->argv = proc->firstObj;
  proc->firstObj += argc;
  return proc->ip++;
}
#endif

void AsmLocalsAlloc(AsmCompiledProc *proc, int nrLocals) {
  proc->nrLocals = nrLocals;
  proc->firstObj += nrLocals;
}

void AsmArgSet(AsmCompiledProc *proc, int argNr, Tcl_Obj **addr) {
  AsmArgReference *arPtr = &proc->argReferences[proc->nrAsmArgReferences];
  arPtr->argNr = argNr;
  arPtr->objPtr = addr;
  proc->nrAsmArgReferences ++;
}

void AsmInstructionPrint(AsmInstruction *ip) {
  int i;
  fprintf(stderr, "(%d) ", ip->argc);
  for (i=0; i<ip->argc; i++) {fprintf(stderr, "%s ", ObjStr(ip->argv[i]));}
  fprintf(stderr, "\n");
}


/*
 *----------------------------------------------------------------------
 * AsmExecute, AsmAssemble --
 *
 *      Define the execution engine for the code
 *
 *----------------------------------------------------------------------
 */
#define NR_PAIRS -1
#define NR_PAIRS1 -2

/*
 * Prototypes needed for the execution engines included below
 */
static int
AsmInstructionArgvCheck(Tcl_Interp *interp, int from, int to, CONST char **argType,
			int nrObjs, int nrStatements,
			Tcl_Obj **wordOv, Tcl_Obj *lineObj);
static void
AsmInstructionArgvSet(Tcl_Interp *interp, int from, int to, int currentArg,
	       AsmInstruction *inst, AsmCompiledProc *asmProc,
	       Tcl_Obj **wordOv, int verbose);

#if defined(LABEL_THREADING)
# include "asm/nsfAsmExecuteLabelThreading.c"
#else
# include "asm/nsfAsmExecuteCallThreading.c"
#endif

#include "asm/nsfAsmAssemble.c"


/*
 *----------------------------------------------------------------------
 * AsmInstructionArgvCheck --
 *
 *      Check the argument types of an assemble statement.
 *
 *----------------------------------------------------------------------
 */
static int
AsmInstructionArgvCheck(Tcl_Interp *interp, int from, int to, CONST char **argType,
			int nrSlots, int nrStatements, Tcl_Obj **wordOv, Tcl_Obj *lineObj) {
  int j;

  for (j = from; j < to; j += 2) {
    int argIndex, typesIndex, intValue, result;

    //fprintf(stderr, "check arg type %s\n", ObjStr(wordOv[j]));
    result = Tcl_GetIndexFromObj(interp, wordOv[j], asmStatementArgType,
				 "asm statement arg type", 0, &typesIndex);
    if (result != TCL_OK) {
      return NsfPrintError(interp,
			   "Asm: unknown arg type %s, line '%s'",
			   ObjStr(wordOv[j]), ObjStr(lineObj));
    }

    result = Tcl_GetIndexFromObj(interp, wordOv[j], argType, "asm internal arg type", 0, &argIndex);
    if (result != TCL_OK) {
      return NsfPrintError(interp,
			   "Asm: instruction argument has invalid type: '%s', line %s\n",
			   ObjStr(wordOv[j]), ObjStr(lineObj));
    }
    //fprintf(stderr, "check arg value %s\n", ObjStr(wordOv[j+1]));
    if (Tcl_GetIntFromObj(interp, wordOv[j+1], &intValue) != TCL_OK
	|| intValue < 0) {
      return NsfPrintError(interp,
			   "Asm: instruction argument of type %s must have numeric index >= 0,"
			   " got '%s', line '%s'",
			   ObjStr(wordOv[j]), ObjStr(wordOv[j+1]), ObjStr(lineObj));
    }

    if ((
	 typesIndex == asmStatementArgTypeObjIdx ||
	 typesIndex == asmStatementArgTypeSlotIdx
	 )
	&& intValue > nrSlots) {
      return NsfPrintError(interp,
			   "Asm: instruction argument value must be less than %d,"
			   " got '%s', line '%s'",
			   nrSlots, ObjStr(wordOv[j+1]), ObjStr(lineObj));
    }
    /* we assume, that every declaration results in exactly one slot */
    if ((typesIndex == asmStatementArgTypeInstructionIdx)
	&& intValue > (nrStatements - nrSlots)) {
      return NsfPrintError(interp,
			   "Asm: instruction argument value must be less than %d,"
			   " got '%s', line '%s'",
			   nrStatements - nrSlots, ObjStr(wordOv[j+1]), ObjStr(lineObj));
    }
  }

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * AsmInstructionArgvSet --
 *
 *      Set argument to be passed to an instruction of the assemble
 *      code.
 *
 *----------------------------------------------------------------------
 */
static void
AsmInstructionArgvSet(Tcl_Interp *interp, int from, int to, int currentArg,
	       AsmInstruction *inst, AsmCompiledProc *asmProc,
	       Tcl_Obj **wordOv, int verbose) {
  int j;

  for (j = from; j < to; j += 2, currentArg++) {
    int argIndex, intValue;
	
    Tcl_GetIndexFromObj(interp, wordOv[j], asmStatementArgType, "asm cmd arg type", 0, &argIndex);
    Tcl_GetIntFromObj(interp, wordOv[j+1], &intValue);

    if (verbose != 0) {
      fprintf(stderr, "AsmInstructionArgvSet (type %d) arg[%d] := %s[%s]\n",
	      argIndex, currentArg, ObjStr(wordOv[j]), ObjStr(wordOv[j+1]));
    }
	
    switch (argIndex) {
    case asmStatementArgTypeObjIdx:
      inst->argv[currentArg] = asmProc->slots[intValue];
      break;
	
    case asmStatementArgTypeArgIdx:
      AsmArgSet(asmProc, intValue, &inst->argv[currentArg]);
      break;
	
    case asmStatementArgTypeResultIdx:
      inst->argv[currentArg] = NULL;
      break;

    case asmStatementArgTypeSlotIdx:
    case asmStatementArgTypeInstructionIdx:
    case asmStatementArgTypeIntIdx:
      inst->argv[currentArg] = INT2PTR(intValue);
      break;

    case asmStatementArgTypeVarIdx:
      fprintf(stderr, ".... var set [%d] = %s\n", currentArg, ObjStr(wordOv[j+1]));
      inst->argv[currentArg] = wordOv[j+1];
      Tcl_IncrRefCount(inst->argv[currentArg]); // TODO: DECR missing
      break;

    }
    /*fprintf(stderr, "[%d] inst %p name %s arg[%d] %s\n", currentAsmInstruction,
      inst, ObjStr(inst->argv[0]), currentArg,
      inst->argv[currentArg] ? ObjStr(inst->argv[currentArg]) : "NULL");*/
  }
}



/*
 *----------------------------------------------------------------------
 * NsfAsmProcDeleteProc --
 *
 *      Tcl_CmdDeleteProc for NsfAsmProcDeleteProc. Is called, whenever a
 *      NsfAsmProcDeleteProc is deleted and frees the associated client data.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Frees client-data
 *
 *----------------------------------------------------------------------
 */
static void
NsfAsmProcDeleteProc(ClientData clientData) {
  AsmProcClientData *cd = clientData;

  /*fprintf(stderr, "NsfAsmProcDeleteProc received %p\n", clientData);*/

  fprintf(stderr, "NsfAsmProcDeleteProc: TODO free asmProc\n");
  if (cd->paramDefs != 0) {
    /* tcd->paramDefs is freed by NsfProcDeleteProc() */
    fprintf(stderr, "NsfAsmProcDeleteProc: TODO free paramDefs\n");
  }
  FREE(AsmProcClientData, cd);
}

/*
 *----------------------------------------------------------------------
 * NsfAsmProc --
 *
 *      Tcl_ObjCmdProc implementing Asm procs. This function processes
 *      the argument list in accordance with the parameter definitions
 *      and calls in case of success the asm proc body.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
extern int
NsfAsmProc(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  AsmProcClientData *cd = clientData;
  int result;

  assert(cd != NULL);
  assert(cd->proc);
  //fprintf(stderr, "NsfAsmProcStub %s is called, tcd %p object %p\n", ObjStr(objv[0]), cd, cd->object);

  if (likely(cd->paramDefs && cd->paramDefs->paramsPtr)) {
    ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

    fprintf(stderr, "not implemented yet\n");
#if 0
    {
      ParseContext *pcPtr;
      pcPtr = (ParseContext *) NsfTclStackAlloc(interp, sizeof(ParseContext), "parse context");
      /*
       * We have to substitute the first element of objv with the name
       * of the function to be called. Since objv is immutable, we have
       * to copy the full argument vector and replace the element on
       * position [0]
       */
      memcpy(tov, objv, sizeof(Tcl_Obj *)*(objc));
      //tov[0] = tcd->procName;

      /* If the argument parsing is ok, the body will be called */
      result = ProcessMethodArguments(pcPtr, interp, NULL, 0,
				      cd->paramDefs, objv[0],
				      objc, tov);

      if (likely(result == TCL_OK)) {
	result = InvokeShadowedProc(interp, cd->procName, cd->cmd, pcPtr);
      } else {
	/*Tcl_Obj *resultObj = Tcl_GetObjResult(interp);
	  fprintf(stderr, "NsfProcStub: incorrect arguments (%s)\n", ObjStr(resultObj));*/
	ParseContextRelease(pcPtr);
	NsfTclStackFree(interp, pcPtr, "release parse context");
      }
    }
#endif
    /*fprintf(stderr, "NsfProcStub free on stack %p\n", tov);*/
    FREE_ON_STACK(Tcl_Obj *, tov);

  } else {
    int requiredArgs = cd->proc->slots - cd->proc->locals;

    //fprintf(stderr, "no compiled parameters\n");
    if (unlikely(requiredArgs != objc-1)) {
      return NsfPrintError(interp, "wrong # of arguments");
    }
    cd->proc->currentObject = cd->object;
    result = AsmExecute(NULL, interp, cd->proc, objc, objv);

  }

  return result;
}




static int
NsfAsmProcAddParam(Tcl_Interp *interp, NsfParsedParam *parsedParamPtr,
		   Tcl_Obj *nameObj, Tcl_Obj *bodyObj, int with_ad) {
  fprintf(stderr, "NsfAsmProcAddParam not implemented yet\n");
  //CONST char *procName = ObjStr(nameObj);

  return TCL_OK;
}

static int
NsfAsmProcAddArgs(Tcl_Interp *interp, Tcl_Obj *argumentsObj,
		  Tcl_Obj *nameObj, Tcl_Obj *bodyObj,
		  int with_ad, int with_checkAlways) {
  int argc, result;
  Tcl_Obj **argv;
  AsmCompiledProc *asmProc;
  AsmProcClientData *cd;
  CONST char *procName = ObjStr(nameObj);

  if (unlikely(Tcl_ListObjGetElements(interp, argumentsObj, &argc, &argv) != TCL_OK)) {
    return NsfPrintError(interp, "argument list invalid '%s'", ObjStr(argumentsObj));
  }

  result = AsmAssemble(NULL, interp, nameObj, argc, bodyObj, &asmProc);
  if (unlikely(result != TCL_OK)) {
    return result;
  }

  cd = NEW(AsmProcClientData);
  cd->object = NULL;
  cd->proc = asmProc;
  cd->paramDefs = NULL;
  cd->with_ad = with_ad;
  cd->with_checkAlways = (with_checkAlways != 0) ? NSF_ARGPARSE_CHECK : 0;

  Tcl_CreateObjCommand(interp, procName, NsfAsmProc,
		       cd, NsfAsmProcDeleteProc);
  return TCL_OK;
}


/*
cmd method::asmcreate NsfAsmMethodCreateCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-checkalways" -required 0 -nrargs 0 -type switch}
  {-argName "-inner-namespace" -nrargs 0}
  {-argName "-per-object"  -nrargs 0}
  {-argName "-reg-object" -required 0 -nrargs 1 -type object}
  {-argName "name" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
}
*/

static int
NsfAsmMethodCreateCmd(Tcl_Interp *interp, NsfObject *defObject,
		      int withInner_namespace, int withPer_object, NsfObject *regObject,
		      Tcl_Obj *nameObj, Tcl_Obj *argumentsObj, Tcl_Obj *bodyObj) {
  int argc, result;
  Tcl_Obj **argv;
  AsmCompiledProc *asmProc;
  AsmProcClientData *cd;
  NsfClass *cl =
    (withPer_object || ! NsfObjectIsClass(defObject)) ?
    NULL : (NsfClass *)defObject;

  // not handled:
  //  * withInner_namespace,
  //  * regObject,
  //  * pre and post-conditions
  //  * withCheckAlways ? NSF_ARGPARSE_CHECK : 0

  if (cl == 0) {
    RequireObjNamespace(interp, defObject);
  }

  if (unlikely(Tcl_ListObjGetElements(interp, argumentsObj, &argc, &argv) != TCL_OK)) {
    return NsfPrintError(interp, "argument list invalid '%s'", ObjStr(argumentsObj));
  }

  result = AsmAssemble(NULL, interp, nameObj, argc, bodyObj, &asmProc);
  if (unlikely(result != TCL_OK)) {
    return result;
  }

  cd = NEW(AsmProcClientData);
  cd->proc = asmProc;
  cd->paramDefs = NULL;
  cd->with_ad = 0;

  if (cl == NULL) {
    result = NsfAddObjectMethod(interp, (Nsf_Object *)defObject, ObjStr(nameObj),
				(Tcl_ObjCmdProc *)NsfAsmProc,
				cd, NsfAsmProcDeleteProc, 0);
  } else {
    result = NsfAddClassMethod(interp, (Nsf_Class *)cl, ObjStr(nameObj),
			       (Tcl_ObjCmdProc *)NsfAsmProc,
			       cd, NsfAsmProcDeleteProc, 0);
  }

  return result;
}

