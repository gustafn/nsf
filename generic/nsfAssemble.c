#define ASM_INFO_DECL  0x0001
#define ASM_INFO_PAIRS 0x0002
#define ASM_INFO_SKIP1 0x0004

typedef struct AsmInstructionInfo {
  int flags;
  CONST char **argTypes;
  int minArgs;
  int maxArgs;
  int cArgs;
} AsmInstructionInfo;

typedef struct AsmInstruction {
  Tcl_ObjCmdProc *cmd;
  ClientData clientData;
  int argc;
  Tcl_Obj **argv;
} AsmInstruction;

typedef struct AsmArgReference {
  int argNr;
  Tcl_Obj **objPtr;
} AsmArgReference;

typedef struct AsmCompiledProc {
  struct AsmInstruction *ip;  /* pointer to the next writable instruction */
  struct AsmInstruction *code;
  NsfObject *currentObject;
  int status;
  int nrLocals;
  Tcl_Obj **firstObj;      /* pointer to staticObjs */
  Tcl_Obj **locals;        /* pointer to staticObjs */
  Tcl_Obj **slots;         /* pointer to staticObjs */
  Tcl_Obj *staticObjs[30]; // 30 static objs for the time being TODO overflows/dynamic
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
  AsmCompiledProc *asmProc;
  NsfParamDefs *paramDefs;
  int with_ad;
} AsmProcClientData;

typedef struct AsmResolverInfo {
  Tcl_Command cmd;
  NsfObject *object;
  AsmCompiledProc *asmProc;
} AsmResolverInfo;

AsmInstruction *AsmInstructionNew(AsmCompiledProc *proc, Tcl_ObjCmdProc* objProc, int argc) {
  proc->ip->cmd  = objProc;
  proc->ip->argc = argc;
  proc->ip->argv = proc->firstObj;
  proc->firstObj += argc;
  return proc->ip++;
}

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
  for (i=0; i<ip->argc; i++) {fprintf(stderr, "%s ", Tcl_GetString(ip->argv[i]));}
  fprintf(stderr, "\n");
}

static int
AsmExecute(ClientData cd, Tcl_Interp *interp, AsmCompiledProc *proc, int argc, Tcl_Obj *CONST argv[]) {
  //AsmInstruction *ip;
  int i, result;

#if 0
  Var *compiledLocals;

  compiledLocals = ((Interp *) interp)->varFramePtr->compiledLocals;
  if (compiledLocals) {
    fprintf(stderr, "compiledLocals = %p\n", compiledLocals);
  }
#endif

  /* 
   * Place a copy of the actual argument into locals.
   */
  for (i=1; i < argc; i++) {
    proc->locals[i-1] = argv[i];
  }
  /*
   * Update all references to compiled arguments.
   */
  for (i=0; i < proc->nrAsmArgReferences; i++) {
    AsmArgReference *arPtr = &proc->argReferences[i];
    *(arPtr->objPtr) = proc->locals[arPtr->argNr];
  }

  /*
   * Set the instruction pointer to the begin of the code.
   */
  proc->ip = proc->code;
  //fprintf(stderr, "ip %p\n", proc->ip);

  while (*proc->ip->cmd) {
    //fprintf(stderr, "will execute instruction ip %p cmd %p %p/%d\n", ip, ip->cmd, ip->argv[0], ip->argc);
    //if (ip->cmd == tclFormat) {AsmInstructionPrint(ip);}
    //if (ip->cmd == (Tcl_ObjCmdProc*)tclDispatch) {AsmInstructionPrint(ip);}
    result = (*proc->ip->cmd)(proc->ip->clientData, interp, proc->ip->argc, proc->ip->argv);
    /*fprintf(stderr, "%s returned <%s> (%d)\n", 
	    Tcl_GetString(ip->argv[0]), 
	    Tcl_GetString(Tcl_GetObjResult(interp)), result);*/
    if (unlikely(result != TCL_OK)) break;
    proc->ip++;
    //fprintf(stderr, "ip %p\n", proc->ip);
  }

  return result;
}


/*
 *----------------------------------------------------------------------
 * asmStoreResult, asmSetResult --
 *
 *    Define helper instructions for moving around runtime date
 *    between the arguments. TODO: currently matching DecrRefCount
 *    from asmStoreResult is missing.
 *
 *----------------------------------------------------------------------
 */

#if defined(USE_OWN_SET)
int asmSet(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  Tcl_Obj *resultObj;
  assert(objc == 3);
  resultObj = Tcl_ObjSetVar2(interp, objv[1], NULL, objv[2], 0);
  Tcl_SetObjResult(interp, resultObj);
  fprintf(stderr, "set %s %s => returns <%s>\n", 
	  Tcl_GetString(objv[1]),
	  Tcl_GetString(objv[2]),
	  Tcl_GetString(resultObj));
  return TCL_OK;
}
#endif

int asmStoreResult(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *target[]) {
  target[0] = Tcl_GetObjResult(interp);
  Tcl_IncrRefCount(target[0]);
  return TCL_OK;
}

int asmSetResult(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmCompiledProc *asmProc = clientData;
  int indexValue = PTR2INT(objv[0]);

  Tcl_SetObjResult(interp, asmProc->slots[indexValue]);
  //fprintf(stderr, "asmSetResult index %d => '%s'\n", indexValue, ObjStr(asmProc->slots[indexValue]));
  return TCL_OK;
}

int asmNoop(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  return TCL_OK;
}

int asmDispatch(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  //int i;
  //fprintf(stderr, "dispatch (%d): \n", objc);
  //for (i=0; i<objc; i++) {fprintf(stderr, "%s ", Tcl_GetString(objv[i]));}
  //fprintf(stderr, "\n");
  return Tcl_EvalObjv(interp, objc, objv, 0);
}

// both unknown
int asmMethodDelegateDispatch00(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  NsfObject *object;
  Tcl_Command cmd;
  int result;
  /*
    int i;
    fprintf(stderr, "asmMethodDispatch (%d): \n", objc);
    for (i=0; i<objc; i++) {fprintf(stderr, "[%d] %s ", i, Tcl_GetString(objv[i]));}
    fprintf(stderr, "\n");
  */
  result = GetObjectFromObj(interp, objv[0], &object);
  if (likely(clientData != NULL)) {
    cmd = clientData;
  } else {
    cmd = Tcl_GetCommandFromObj(interp, objv[1]);
  }
  //fprintf(stderr, "cmd %p object %p\n", cmd, object);
  return MethodDispatch(object, interp, objc-1, objv+1, cmd, object, NULL,
			ObjStr(objv[1]), 0, 0);
}

// both known 
int asmMethodDelegateDispatch11(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmResolverInfo *resInfo = clientData;
  return MethodDispatch(resInfo->object, interp, objc-1, objv+1, 
			resInfo->cmd, resInfo->object, NULL,
			ObjStr(objv[1]), 0, 0);
}

int asmMethodSelfDispatch(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmResolverInfo *resInfo = clientData;
  Tcl_Command cmd = resInfo->cmd ? resInfo->cmd : Tcl_GetCommandFromObj(interp, objv[0]);

  return MethodDispatch(resInfo->asmProc->currentObject, interp, objc, objv, 
			cmd, resInfo->asmProc->currentObject, NULL,
			ObjStr(objv[0]), 0, 0);
}

int asmMethodSelfCmdDispatch(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmResolverInfo *resInfo = clientData;
  assert(resInfo->cmd != NULL);
  return Tcl_NRCallObjProc(interp, Tcl_Command_objProc(resInfo->cmd), resInfo->asmProc->currentObject, objc, objv);
}

int asmMethodSelf(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmCompiledProc *asmProc = clientData;
  Tcl_SetObjResult(interp, asmProc->currentObject->cmdName);
  return TCL_OK;
}

int asmJump(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmCompiledProc *asmProc = clientData;
  int instructionIndex;

  instructionIndex = PTR2INT(objv[0]);
  //fprintf(stderr, "asmJump oc %d instructionIndex %d\n", objc, instructionIndex);
  asmProc->ip =  &asmProc->code[instructionIndex] - 1;

  return TCL_OK;
}

int asmJumpTrue(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmCompiledProc *asmProc = clientData;
  int instructionIndex;
  
  if (asmProc->status) {
    instructionIndex = PTR2INT(objv[0]);
    //fprintf(stderr, "asmJump oc %d instructionIndex %d\n", objc, instructionIndex);
    asmProc->ip =  &asmProc->code[instructionIndex] - 1;
  }

  return TCL_OK;
}

int asmLeScalar(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmCompiledProc *asmProc = clientData;
  int valueIndex1, valueIndex2, value1, value2;
  //fprintf(stderr, "asmLeScalar oc %d op1 %p op2 %p\n", objc, objv[0],  objv[1]);

  valueIndex1 = PTR2INT(objv[0]);
  valueIndex2 = PTR2INT(objv[1]);

  //fprintf(stderr, "asmLeScalar oc %d index1 %d index2 %d\n", objc, valueIndex1,  valueIndex2);

  // for the time being, we compare two int values
  Tcl_GetIntFromObj(interp, asmProc->slots[valueIndex1], &value1);
  Tcl_GetIntFromObj(interp, asmProc->slots[valueIndex2], &value2);
  //fprintf(stderr, "asmLeScalar oc %d op1 %d op2 %d => %d\n", objc, value1, value2, value1 <= value2);

  asmProc->status = value1 <= value2;

  return TCL_OK;
}

int asmSetScalar(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmCompiledProc *asmProc = clientData;
  int indexValue;

  indexValue = PTR2INT(objv[0]);
  //fprintf(stderr, "asmSetScalar oc %d var %d = %s\n", objc, indexValue, ObjStr(objv[1]));  

  asmProc->slots[indexValue] = objv[1];
  return TCL_OK;
}

int asmSetScalarResult(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmCompiledProc *asmProc = clientData;
  int indexValue = PTR2INT(objv[0]);
  //fprintf(stderr, "asmSetScalar oc %d var %d = %s\n", objc, indexValue, ObjStr(objv[1]));  

  asmProc->slots[indexValue] =  Tcl_GetObjResult(interp);
  return TCL_OK;
}

int asmIncrScalar(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *objv[]) {
  AsmCompiledProc *asmProc = clientData;
  int indexValue1, indexValue2, intValue, incrValue;
  Tcl_Obj *intObj, *incrObj;

  indexValue1 = PTR2INT(objv[0]);
  indexValue2 = PTR2INT(objv[1]);

  //fprintf(stderr, "asmIncrScalar oc %d var[%d] incr var[%d]\n", objc, indexValue1, indexValue2);

  intObj = asmProc->slots[indexValue1];
  incrObj = asmProc->slots[indexValue2];

  Tcl_GetIntFromObj(interp, intObj, &intValue);
  Tcl_GetIntFromObj(interp, incrObj, &incrValue);

  //fprintf(stderr, "asmIncrScalar %d + %d = %d\n", intValue, incrValue, intValue + incrValue);
  
  Tcl_InvalidateStringRep(intObj);
  intObj->internalRep.longValue = (long)(intValue + incrValue);

  Tcl_SetObjResult(interp, intObj);

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * tcl5_assemble --
 *
 *    Compile body from assembly text to a struct AsmCompiledProc
 *    containing the instructions and data.
 *
 *----------------------------------------------------------------------
 */

enum asmCmdArgIndex      {asmCmdArgArgIdx, asmCmdArgIntIdx, asmCmdArgObjIdx, asmCmdArgResultIdx, asmCmdArgVarIdx};
static CONST char        *asmCmdArgTypes[] = {"arg", "int", "obj", "result", "var", NULL};

enum asmAddrIndex        {asmAddrCodeIdx, asmAddrArgvIdx};
static CONST char        *asmAddrTypes[] = {"code", "argv", NULL};

static int
AsmCheckArgTypes(Tcl_Interp *interp, int from, int to, CONST char **argTypes, 
		 int nrObjs, Tcl_Obj **wordOv, Tcl_Obj *lineObj) {
  int j, result;

  for (j = from; j < to; j += 2) {
    int argIndex, intValue;
    //fprintf(stderr, "check arg type %s\n", Tcl_GetString(wordOv[j]));
    result = Tcl_GetIndexFromObj(interp, wordOv[j], argTypes, "asm internal arg type", 0, &argIndex);
    if (result != TCL_OK) {
      return NsfPrintError(interp, 
			   "Asm: instruction argument has unknown type: '%s', line %s\n", 
			   Tcl_GetString(wordOv[j]), Tcl_GetString(lineObj));
    }
    //fprintf(stderr, "check arg value %s\n", Tcl_GetString(wordOv[j+1]));
    if (Tcl_GetIntFromObj(interp, wordOv[j+1], &intValue) != TCL_OK 
	|| intValue < 0 
	|| (argTypes == asmCmdArgTypes && argIndex == asmCmdArgObjIdx && intValue > nrObjs)) {
      return NsfPrintError(interp, 
			   "Asm: instruction argument of type %s must have numeric index >= 0 (less than max),"
			   " got '%s', line '%s'", 
			   ObjStr(wordOv[j]), ObjStr(wordOv[j+1]), ObjStr(lineObj));
    }
  }
  return TCL_OK;
}

static void 
AsmSetArgValue(Tcl_Interp *interp, int from, int to, int currentArg,
	       AsmInstruction *inst, AsmCompiledProc *asmProc,
	       Tcl_Obj **wordOv, int verbose) {
  int j;

  for (j = from; j < to; j += 2, currentArg++) {
    int argIndex, intValue;
	  
    Tcl_GetIndexFromObj(interp, wordOv[j], asmCmdArgTypes, "asm cmd arg type", 0, &argIndex);
    Tcl_GetIntFromObj(interp, wordOv[j+1], &intValue);

    if (verbose) {
      fprintf(stderr, "AsmSetArgValue (type %d) arg[%d] := %s[%s]\n", 
	      argIndex, currentArg, ObjStr(wordOv[j]), ObjStr(wordOv[j+1]));
    }
	  
    switch (argIndex) {
    case asmCmdArgObjIdx: 
      inst->argv[currentArg] = asmProc->slots[intValue];
      break;
	    
    case asmCmdArgArgIdx:
      AsmArgSet(asmProc, intValue, &inst->argv[currentArg]);
      break;
	    
    case asmCmdArgResultIdx: 
      inst->argv[currentArg] = NULL;
      break;

    case asmCmdArgIntIdx:
      inst->argv[currentArg] = INT2PTR(intValue);
      break;

    case asmCmdArgVarIdx:
      fprintf(stderr, ".... var set [%d] = %s\n", currentArg, ObjStr(wordOv[j+1]));
      inst->argv[currentArg] = wordOv[j+1];
      Tcl_IncrRefCount(inst->argv[currentArg]); // TODO: DECR missing
      break;

    }
    /*fprintf(stderr, "[%d] inst %p name %s arg[%d] %s\n", currentAsmInstruction,
      inst, Tcl_GetString(inst->argv[0]), currentArg, 
      inst->argv[currentArg] ? Tcl_GetString(inst->argv[currentArg]) : "NULL");*/
  }
}


static int 
AsmAssemble(ClientData cd, Tcl_Interp *interp, Tcl_Obj *nameObj, 
	      int nrArgs, Tcl_Obj *asmObj, AsmCompiledProc **retAsmProc) {
  AsmPatches patchArray[100], *patches = &patchArray[0], *patchPtr; // TODO: make me dynamic
  Tcl_Command cmd;
  AsmCompiledProc *asmProc;
  AsmInstruction *inst;
  int i, result, nrAsmInstructions, nrObjs, totalArgvArgs;
  int oc, currentAsmInstruction, currentSlot;
  Tcl_Obj **ov;
  CONST char *procName;

  enum asmAsmInstructionIndex {
    asmObjIdx, 
    asmVarIdx,
    asmNoopIdx,
    asmCmdIdx, 
    asmEvalIdx, 
    asmMethodDelegateDispatchIdx, 
    asmMethodSelfDispatchIdx, 
    asmSelfIdx, 
    asmJumpIdx, 
    asmJumpTrueIdx, 
    asmLeScalarIdx, 
    asmSetScalarIdx, 
    asmSetScalarResultIdx, 
    asmIncrScalarIdx,
    asmResultIdx, 
    asmStoreIdx
  };
  static CONST char *asmInstructionNames[] = {
    "obj", 
    "var", 
    "noop",
    "cmd", 
    "eval", 
    "methodDelegateDispatch", 
    "methodSelfDispatch", 
    "self",
    "jump", 
    "jumpTrue", 
    "leScalar", 
    "setScalar", 
    "setScalarResult", 
    "incrScalar", 
    "result", 
    "store",
    NULL
  };

#define NR_PAIRS -1
#define NR_PAIRS1 -2

  static AsmInstructionInfo asmInstructionInfo[] = {
    /* asmObjIdx, */
    {ASM_INFO_DECL, NULL, 2, 2, 0},
    /* asmVarIdx, */
    {ASM_INFO_DECL|ASM_INFO_PAIRS, asmCmdArgTypes, 3, 3, 0},   // should force just "obj" arg1
    /* asmNoopIdx, */
    {0, NULL, 1, 1, 0},
    /* asmCmdIdx, */
    {ASM_INFO_PAIRS|ASM_INFO_SKIP1, NULL, 2, -1, NR_PAIRS1},
    /* asmEvalIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 3, -1, NR_PAIRS},
    /* asmMethodDelegateDispatchIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 5, -1, NR_PAIRS},
    /* asmMethodSelfDispatchIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 3, -1, NR_PAIRS},
    /* asmSelfIdx, */
    {0, NULL, 1, 1, 0},
    /* asmJumpIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 3, 3, 1},  // should force arg1 "int", label
    /* asmJumpTrueIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 3, 3, 1},  // should force arg1 "int", label
    /* asmLeScalarIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 5, 5, 2},
    /* asmSetScalarIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 5, 5, 2},  // should force arg1 & arg2 "int" 
    /* asmSetScalarResultIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 3, 3, 2},  // should force arg1 "int" 
    /* asmIncrScalarIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 5, 5, 2},  // should force arg1 "int" 
    /* asmResultIdx, */
    {ASM_INFO_PAIRS, asmCmdArgTypes, 3, 3, 1},  // should force arg1 "int" 
    /* asmStoreIdx */
    {ASM_INFO_PAIRS, asmAddrTypes, 5, 5, 0}
  };

    /*
    {obj a}
    {var obj 0}
    {cmd ::format obj 0 obj 3} 
    {eval obj 0 obj 1 obj 2}
    {store code 4 argv 3}
    {methodDelegateDispatch obj 0 obj 1 obj 2}
    {methodSelfDispatch obj 0 obj 1}
    {self}
    {jump int 2}
    {jumpTrue int 6}
    {leScalar int 5 int 4}
    {setScalar int 2 arg 0}
    {setScalarResult int 2}
    {incrScalar int 2 obj 1}
    {store code 4 argv 2}
    {result obj 0}
    */

  assert(nameObj);
  procName = ObjStr(nameObj);
  
  if (Tcl_ListObjGetElements(interp, asmObj, &oc, &ov) != TCL_OK) {
    return NsfPrintError(interp, "Asm code is not a valid list");
  }

  /*
   * First Iteration: check wellformedness, determine sizes
   */
  nrAsmInstructions = 0;
  nrObjs = 0;
  totalArgvArgs = 0;

  for (i = 0; i < oc; i++) {
    int index, offset, wordOc;
    Tcl_Obj *lineObj = ov[i], **wordOv;
    
    if (Tcl_ListObjGetElements(interp, lineObj, &wordOc, &wordOv) != TCL_OK) {
      return NsfPrintError(interp, 
			   "Asm: line is not a well-formed asm instruction: %s", 
			   ObjStr(lineObj));
    }

    result = Tcl_GetIndexFromObj(interp, wordOv[0], asmInstructionNames, "asm instruction", 0, &index);
    if (result != TCL_OK) {
      return NsfPrintError(interp, 
			   "Asm: line is not a valid asm instruction: word %s, line %s", 
			   ObjStr(wordOv[0]), ObjStr(lineObj));
    }

    offset = (asmInstructionInfo[index].flags & ASM_INFO_SKIP1) ? 2 : 1;

    if ((asmInstructionInfo[index].flags & ASM_INFO_PAIRS) && (wordOc-offset) % 2 == 1) {
      return NsfPrintError(interp, "Asm: argument list of cmd must contain pairs: %s", 
			   ObjStr(lineObj));
    }

    if (asmInstructionInfo[index].minArgs > -1 
	&& wordOc < asmInstructionInfo[index].minArgs) {
      return NsfPrintError(interp, "Asm: statement must contain at least %d words: %s", 
			   asmInstructionInfo[index].minArgs, ObjStr(lineObj));
    }

    if (asmInstructionInfo[index].maxArgs > -1
	&& wordOc > asmInstructionInfo[index].maxArgs) {
      return NsfPrintError(interp, "Asm: statement must contain at most %d words: %s", 
			   asmInstructionInfo[index].maxArgs, ObjStr(lineObj));
    }

    if (asmInstructionInfo[index].argTypes) {
      result = AsmCheckArgTypes(interp, offset, wordOc, asmInstructionInfo[index].argTypes, nrObjs, wordOv, lineObj);
      if (unlikely(result != TCL_OK)) {return result;}
    }

    if ((asmInstructionInfo[index].flags & ASM_INFO_DECL) == 0) {
      int cArgs = asmInstructionInfo[index].cArgs;
      if (cArgs == NR_PAIRS) {
	cArgs = (wordOc-offset) / 2;
      } else if (cArgs == NR_PAIRS1) {
	cArgs = 1 + (wordOc-offset) / 2;
      }
      //fprintf(stderr, "instruction %s need argvargs %d\n", ObjStr(lineObj), cArgs);
      totalArgvArgs += cArgs;

      nrAsmInstructions++;
    } else {
      /* currently obj and var from the same pool, must change... */
      nrObjs ++;
    }

    switch (index) {
    case asmObjIdx:
    case asmVarIdx:
      break;

    case asmCmdIdx:
      /* {cmd ::set slot 0 slot 2} */
      cmd = Tcl_GetCommandFromObj(interp, wordOv[1]);
      if (cmd == NULL) {
	return NsfPrintError(interp, 
			     "Asm: cmd is not a valid tcl command: %s\n", 
			     Tcl_GetString( wordOv[1]));
      }
      break;

    case asmNoopIdx: /* fall through */
    case asmEvalIdx:
    case asmMethodDelegateDispatchIdx:   
    case asmMethodSelfDispatchIdx:   
    case asmSelfIdx: 
    case asmJumpIdx:    
    case asmJumpTrueIdx:    
    case asmLeScalarIdx:    
    case asmSetScalarIdx:    
    case asmSetScalarResultIdx:
    case asmIncrScalarIdx:    
    case asmStoreIdx:    
    case asmResultIdx:   
      break;
    }
    
  }
  /* END instruction */
  nrAsmInstructions ++;
  fprintf(stderr, "%s: nrAsmInstructions %d nrObjs %d nrArgs %d argvArgs %d => data %d\n", 
	  procName, nrAsmInstructions, nrObjs, nrArgs, totalArgvArgs, 
	  nrObjs + nrArgs + totalArgvArgs );

  /*
   * Allocate structures
   */

  asmProc = (AsmCompiledProc *)ckalloc(sizeof(AsmCompiledProc));
  asmProc->code = (AsmInstruction *)ckalloc(sizeof(AsmInstruction) * nrAsmInstructions);

  asmProc->ip = asmProc->code;  /* points to the first writable instructon */
  asmProc->firstObj = asmProc->staticObjs;  /* point to the first free obj */
  asmProc->locals = asmProc->staticObjs;    /* locals is just an alias     */
  asmProc->nrLocals = 0;
  asmProc->nrAsmArgReferences = 0;
  asmProc->slots = asmProc->locals + nrArgs;
  //fprintf(stderr, "args = %ld\n", asmProc->slots - asmProc->locals);

  AsmLocalsAlloc(asmProc, nrArgs + nrObjs);

  /*
   * Second Iteration: produce code
   */
  currentSlot = 0;
  currentAsmInstruction = 0;

  for (i = 0; i < oc; i++) {
    int index, offset, cArgs, wordOc, codeIndex, argvIndex, j;
    Tcl_Obj *lineObj = ov[i], **wordOv;
    
    Tcl_ListObjGetElements(interp, lineObj, &wordOc, &wordOv);
    Tcl_GetIndexFromObj(interp, wordOv[0], asmInstructionNames, "asm instruction", 0, &index);

    offset = (asmInstructionInfo[index].flags & ASM_INFO_SKIP1) ? 2 : 1;

    cArgs = asmInstructionInfo[index].cArgs;
    if (cArgs == NR_PAIRS) {
      cArgs = (wordOc-offset) / 2;
    } else if (cArgs == NR_PAIRS1) {
      cArgs = 1 + (wordOc-offset) / 2;
    }
    
    switch (index) {
    case asmObjIdx:
      /* {obj a} */
      asmProc->slots[currentSlot] = wordOv[1];
      Tcl_IncrRefCount(asmProc->slots[currentSlot]);
      currentSlot ++;
      /* a line like the following will be needed when freeing this asmProc */
      //for (i=0; i < nrObjs; i++) { Tcl_DecrRefCount(asmProc->slots[i]); }
      break;

    case asmVarIdx:
      /* {var obj 0} */
      // for now, we just allocate a tcl-obj and ignore the name
      asmProc->slots[currentSlot] = NULL;
      currentSlot ++;
      break;

    case asmNoopIdx:
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc*)asmNoop, 0);
      break;

    case asmCmdIdx:
      /* {cmd ::set slot 0 slot 2} */
      cmd = Tcl_GetCommandFromObj(interp, wordOv[1]);
      inst = AsmInstructionNew(asmProc, ((Command *)cmd)->objProc, cArgs);
      inst->clientData = ((Command *)cmd)->objClientData;
      /* use the assembly word as cmd name; should be ok when we keep assembly around */
      inst->argv[0] = wordOv[1];
      /*fprintf(stderr, "[%d] %s/%d\n", currentAsmInstruction, Tcl_GetString(wordOv[1]), 1+((wordOc-offset)/2));*/

      AsmSetArgValue(interp, offset, wordOc, 1, inst, asmProc, wordOv, 0);
      break;

    case asmMethodDelegateDispatchIdx:
      /* {methodDelegateDispatch obj 0 obj 1 obj 2} */
      { Tcl_Command cmd = NULL;
	NsfObject *object = NULL;
	AsmResolverInfo *resInfo;

	inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc*)asmMethodDelegateDispatch00, cArgs);
	AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 0);
	if (strncmp(ObjStr(inst->argv[1]), "::nsf::methods::", 16) == 0) {
	  cmd = Tcl_GetCommandFromObj(interp, inst->argv[1]);
	  fprintf(stderr, "%s: asmMethod cmd '%s' => %p\n", procName, ObjStr(inst->argv[1]), cmd);
	}
	if (strncmp(ObjStr(inst->argv[0]), "::nx::", 6) == 0) {
	  GetObjectFromObj(interp, inst->argv[0], &object);
	  fprintf(stderr, "%s: asmMethod object '%s' => %p\n", procName, ObjStr(inst->argv[0]), object);
	}
	if (cmd && object) {
	  // experimental: bind obj and method
	  resInfo = NEW(AsmResolverInfo); // TODO: LEAK
	  resInfo->cmd = cmd;
	  resInfo->object = object;
	  inst->clientData = resInfo;
	  inst->cmd = (Tcl_ObjCmdProc*)asmMethodDelegateDispatch11;
	} else if (cmd) {
	  inst->clientData = cmd;
	} else {	  
	  inst->clientData = NULL;
	}
      }
      break;
      
    case asmMethodSelfDispatchIdx:
      /* {methodSelfDispatch obj 0 obj 1 obj 2} */
      { Tcl_Command cmd = NULL;	  
	AsmResolverInfo *resInfo;
	  
	inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc*)asmMethodSelfDispatch, cArgs);
	AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 0);
	if (strncmp(ObjStr(inst->argv[0]), "::nsf::methods::", 16) == 0) {
	  cmd = Tcl_GetCommandFromObj(interp, inst->argv[0]);
	  if (cmd) {
	    fprintf(stderr, "%s: asmMethodSelfCmdDispatch cmd '%s' => %p\n", procName, ObjStr(inst->argv[0]), cmd);
	    inst->cmd = (Tcl_ObjCmdProc*)asmMethodSelfCmdDispatch;
	  }
	} else {
	  fprintf(stderr, "%s: asmMethodSelfDispatch cmd '%s'\n", procName, ObjStr(inst->argv[0]));
	}
	resInfo = NEW(AsmResolverInfo); // TODO: LEAK
	resInfo->cmd = cmd;
	resInfo->asmProc = asmProc;
	inst->clientData = resInfo;
      }
      break;

    case asmResultIdx:
      /* {result int 0} */
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc*)asmSetResult, 1);
      AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 0);
      inst->clientData = asmProc;
      break;
      
    case asmStoreIdx:
      /* {store code 4 argv 1} */
      codeIndex = -1;
      argvIndex = -1;
      for (j = offset; j < wordOc; j += 2) {
	int argIndex, intValue;
	Tcl_GetIndexFromObj(interp, wordOv[j], asmAddrTypes, "asm internal arg type", 0, &argIndex);
	Tcl_GetIntFromObj(interp, wordOv[j+1], &intValue);
	switch (argIndex) {
	case asmAddrCodeIdx: codeIndex = intValue; break;
	case asmAddrArgvIdx: argvIndex = intValue; break;
	}
      }
      // TODO: CHECK codeIndex, argvIndex (>0, reasonable values)
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc*)asmStoreResult, 0);
      //fprintf(stderr, "%p setting instruction %d => %d %d\n", patches, currentAsmInstruction, codeIndex, argvIndex);
      patches->targetAsmInstruction = currentAsmInstruction;
      patches->sourceAsmInstruction = codeIndex;
      patches->argvIndex = argvIndex;
      patches++;
      break;
      
    case asmEvalIdx:
      /* {eval slot 1 slot 2} */
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc *)asmDispatch, cArgs);
      //fprintf(stderr, "[%d] %s/%d\n", currentAsmInstruction, Tcl_GetString(wordOv[1]), ((wordOc-offset)/2));
      AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 0);
      break;

    case asmJumpIdx:    
      /* {jump int 2} */
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc *)asmJump, cArgs);
      AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 0);
      inst->clientData = asmProc;
      break;

    case asmJumpTrueIdx:   
      /* {jumpTrue int 6 */
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc *)asmJumpTrue, cArgs);
      AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 0);
      inst->clientData = asmProc;
      break;

    case asmLeScalarIdx:
      /* {leScalar int 5 int 4} */
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc *)asmLeScalar, cArgs);
      AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 1);
      inst->clientData = asmProc;
      break;

    case asmSetScalarIdx:
      /* {setScalar int 2 int 0} */
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc *)asmSetScalar, cArgs);
      AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 0);
      inst->clientData = asmProc;
      break;

    case asmSetScalarResultIdx:
      /* {setScalarResult int 2} */
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc *)asmSetScalarResult, cArgs);
      AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 0);
      inst->clientData = asmProc;
      break;

    case asmIncrScalarIdx:
      /* {incrScalar int 2 int 0} */
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc *)asmIncrScalar, cArgs);
      AsmSetArgValue(interp, offset, wordOc, 0, inst, asmProc, wordOv, 0);
      inst->clientData = asmProc;
      break;
      
    case asmSelfIdx:
      /* {self} */
      inst = AsmInstructionNew(asmProc, (Tcl_ObjCmdProc *)asmMethodSelf, 0);
      inst->clientData = asmProc;
      break;
    }

    if ((asmInstructionInfo[index].flags & ASM_INFO_DECL) == 0) {
      currentAsmInstruction ++;
    }
  }

  /*
   * add END instruction
   */
  inst = AsmInstructionNew(asmProc, NULL, 0);
  
  /*
   * All addresses are determined, apply the argv patches triggered
   * from above.
   */

  for (patchPtr = &patchArray[0]; patchPtr < patches; patchPtr++) {
    fprintf(stderr, "wanna patch code[%d]->argv = code[%d]->argv[%d]\n",
	    patchPtr->targetAsmInstruction, patchPtr->sourceAsmInstruction, patchPtr->argvIndex);
    /* set the argument vector of code[1] to the address of code[4]->argv[1] */
    (&asmProc->code[patchPtr->targetAsmInstruction])->argv = 
      &(&asmProc->code[patchPtr->sourceAsmInstruction])->argv[patchPtr->argvIndex];
  }

  *retAsmProc = asmProc;

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * NsfAsmProcDeleteProc --
 *
 *    Tcl_CmdDeleteProc for NsfAsmProcDeleteProc. Is called, whenever a
 *    NsfAsmProcDeleteProc is deleted and frees the associated client data.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Frees client-data
 *
 *----------------------------------------------------------------------
 */
static void
NsfAsmProcDeleteProc(ClientData clientData) {
  AsmProcClientData *cd = clientData;

  /*fprintf(stderr, "NsfAsmProcDeleteProc received %p\n", clientData);*/
  
  fprintf(stderr, "NsfAsmProcDeleteProc: TODO free asmProc\n");
  if (cd->paramDefs) {
    /* tcd->paramDefs is freed by NsfProcDeleteProc() */
    fprintf(stderr, "NsfAsmProcDeleteProc: TODO free paramDefs\n");
  }
  FREE(AsmProcClientData, cd);
}

/*
 *----------------------------------------------------------------------
 * NsfAsmProc --
 *
 *    Tcl_ObjCmdProc implementing Asm procs. This function processes
 *    the argument list in accordance with the parameter definitions
 *    and calls in case of success the asm proc body.
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
NsfAsmProc(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  AsmProcClientData *cd = clientData;
  int result;

  assert(cd);
  assert(cd->asmProc);
  //fprintf(stderr, "NsfAsmProcStub %s is called, tcd %p object %p\n", ObjStr(objv[0]), cd, cd->object);

  if (likely(cd->paramDefs && cd->paramDefs->paramsPtr)) {
    ParseContext *pcPtr = (ParseContext *) NsfTclStackAlloc(interp, sizeof(ParseContext), "parse context");
    ALLOC_ON_STACK(Tcl_Obj*, objc, tov);

    fprintf(stderr, "not implemented yet\n");
    pcPtr = (ParseContext *)tov; // dummy operation to keep compiler quiet
#if 0
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
#endif
    /*fprintf(stderr, "NsfProcStub free on stack %p\n", tov);*/
    FREE_ON_STACK(Tcl_Obj *, tov);

  } else {
    int requiredArgs = cd->asmProc->slots - cd->asmProc->locals;

    //fprintf(stderr, "no compiled parameters\n");
    if (unlikely(requiredArgs != objc-1)) {
      return NsfPrintError(interp, "wrong # of arguments");
    }
    cd->asmProc->currentObject = cd->object;
    result = AsmExecute(NULL, interp, cd->asmProc, objc, objv);
  
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
		  Tcl_Obj *nameObj, Tcl_Obj *bodyObj, int with_ad) {
  int argc, result;
  Tcl_Obj **argv;
  AsmCompiledProc *asmProc;
  AsmProcClientData *cd;
  Tcl_Command cmd;
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
  cd->asmProc = asmProc;
  cd->paramDefs = NULL;
  cd->with_ad = with_ad;

  cmd = Tcl_CreateObjCommand(interp, procName, NsfAsmProc,
			     cd, NsfAsmProcDeleteProc);
  return TCL_OK;
}


/*
cmd method::asmcreate NsfAsmMethodCreateCmd {
  {-argName "object" -required 1 -type object}
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

  // not handled:  withInner_namespace, regObject, no pre and post-conditions

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
  cd->asmProc = asmProc;
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
