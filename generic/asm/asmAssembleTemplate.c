enum asmStatementIndex {
  asmObjProcIdx,
  $STATEMENT_INDICES
};

static CONST char *asmStatementNames[] = {
  "cmd",
  $STATEMENT_NAMES,
  NULL
};

enum asmStatementArgTypeIndex {
  asmStatementArgTypeArgIdx,
  asmStatementArgTypeArgvIdx,
  asmStatementArgTypeInstructionIdx,
  asmStatementArgTypeIntIdx,
  asmStatementArgTypeObjIdx,
  asmStatementArgTypeResultIdx,
  asmStatementArgTypeSlotIdx,
  asmStatementArgTypeVarIdx
};

static CONST char *asmStatementArgType[] = {
  "arg",
  "argv",
  "instruction",
  "int",
  "obj",
  "result",
  "slot",
  "var",
  NULL};

static CONST char *asmStatementCmdType[]         = {"arg", "obj", "result", "var", NULL};
static CONST char *asmStatementInstructionType[] = {"instruction", NULL};
static CONST char *asmStatementIntType[]         = {"int", NULL};
static CONST char *asmStatementObjType[]         = {"obj", NULL};
static CONST char *asmStatementSlotObjArgType[]  = {"slot", "obj", "arg", NULL};
static CONST char *asmStatementSlotType[]        = {"slot", NULL};
static CONST char *asmStatementSlotIntType[]     = {"slot", "int", NULL};
static CONST char *asmStatementStoreType[]       = {"instruction", "argv", NULL};

static AsmStatementInfo asmStatementInfo[] = {
  /* asmObjProcIdx, */
  {ASM_INFO_PAIRS|ASM_INFO_SKIP1, NULL, 2, TCL_INDEX_NONE, NR_PAIRS1},
  $STATEMENT_INFO
};


/*
 *----------------------------------------------------------------------
 * AsmAssemble --
 *
 *      The assmbler, takes an assembly script in the form of a nested
 *      list and emits the internal representation for the execution
 *      engine.
 *
 *----------------------------------------------------------------------
 */

static int
AsmAssemble(ClientData cd, Tcl_Interp *interp, Tcl_Obj *nameObj,
	      int nrArgs, Tcl_Obj *asmObj, AsmCompiledProc **retAsmProc) {
  AsmPatches patchArray[100], *patches = &patchArray[0], *patchPtr; // TODO: make me dynamic
  Tcl_Command cmd;
  AsmCompiledProc *proc;
  AsmInstruction *inst;
  int i, result, nrAsmInstructions, nrLocalObjs, totalArgvArgs;
  int oc, currentAsmInstruction, currentSlot;
  Tcl_Obj **ov;
  CONST char *procName;

  assert(nameObj != NULL);
  procName = ObjStr(nameObj);

  if (Tcl_ListObjGetElements(interp, asmObj, &oc, &ov) != TCL_OK) {
    return NsfPrintError(interp, "Asm code is not a valid list");
  }

  /*
   * First Iteration: check wellformedness, determine sizes
   */
  nrAsmInstructions = 0;
  nrLocalObjs = 0;
  totalArgvArgs = 0;

  for (i = 0; i < oc; i++) {
    int index, offset, wordOc;
    Tcl_Obj *lineObj = ov[i], **wordOv;

    if (Tcl_ListObjGetElements(interp, lineObj, &wordOc, &wordOv) != TCL_OK) {
      return NsfPrintError(interp,
			   "Asm: line is not a well-formed asm instruction: %s",
			   ObjStr(lineObj));
    }

    result = Tcl_GetIndexFromObj(interp, wordOv[0], asmStatementNames, "asm instruction", 0, &index);
    if (result != TCL_OK) {
      return NsfPrintError(interp,
			   "Asm: line is not a valid asm instruction: word %s, line %s",
			   ObjStr(wordOv[0]), ObjStr(lineObj));
    }

    offset = (asmStatementInfo[index].flags & ASM_INFO_SKIP1) ? 2 : 1;

    if ((asmStatementInfo[index].flags & ASM_INFO_PAIRS) && (wordOc-offset) % 2 == 1) {
      return NsfPrintError(interp, "Asm: argument list of cmd must contain pairs: %s",
			   ObjStr(lineObj));
    }

    if (asmStatementInfo[index].minArgs != TCL_INDEX_NONE
	&& wordOc < asmStatementInfo[index].minArgs) {
      return NsfPrintError(interp, "Asm: statement must contain at least %d words: %s",
			   asmStatementInfo[index].minArgs, ObjStr(lineObj));
    }

    if (asmStatementInfo[index].maxArgs != TCL_INDEX_NONE
	&& wordOc > asmStatementInfo[index].maxArgs) {
      return NsfPrintError(interp, "Asm: statement must contain at most %d words: %s",
			   asmStatementInfo[index].maxArgs, ObjStr(lineObj));
    }

    if (asmStatementInfo[index].argTypes) {
      result = AsmInstructionArgvCheck(interp, offset, wordOc,
				       asmStatementInfo[index].argTypes,
				       nrLocalObjs, oc, wordOv, lineObj);
      if (unlikely(result != TCL_OK)) {return result;}
    }

    if ((asmStatementInfo[index].flags & ASM_INFO_DECL) == 0) {
      int cArgs = asmStatementInfo[index].cArgs;
      /*
       * Determine the actual number of arguments passed to the
       * emitted instruction. This number might be determine by the
       * instruction type, or by the actual instruction being
       * processed (and later maybe for {*} etc.).
       */
      if (cArgs == NR_PAIRS) {
	cArgs = (wordOc-offset) / 2;
      } else if (cArgs == NR_PAIRS1) {
	cArgs = 1 + (wordOc-offset) / 2;
      }
      //fprintf(stderr, "instruction %s need argvargs %d\n", ObjStr(lineObj), cArgs);
      totalArgvArgs += cArgs;

      nrAsmInstructions++;
    } else {
      /* currently obj and var from the same pool, will change... */
      nrLocalObjs ++;
    }

    /*
     * optional, per-statement check operations
     */
    switch (index) {
    case asmObjProcIdx:
      /* {cmd ::set slot 0 slot 2} */
      cmd = Tcl_GetCommandFromObj(interp, wordOv[1]);
      if (cmd == NULL) {
	return NsfPrintError(interp,
			     "Asm: cmd is not a valid Tcl command: %s\n",
			     Tcl_GetString( wordOv[1]));
      }
      break;

   /* begin generated code */
   $ASSEMBLE_CHECK_CODE
   /* end generated code */

    default:
      break;
    }
  }

  nrAsmInstructions ++;
  fprintf(stderr, "%s: nrAsmInstructions %d nrLocalObjs %d nrArgs %d argvArgs %d => data %d\n",
	  procName, nrAsmInstructions, nrLocalObjs, nrArgs, totalArgvArgs,
	  nrLocalObjs + nrArgs + totalArgvArgs );

  /*
   * Allocate structures
   */

  proc = (AsmCompiledProc *)ckalloc(sizeof(AsmCompiledProc));
  proc->code = (AsmInstruction *)ckalloc(sizeof(AsmInstruction) * nrAsmInstructions);
  memset(proc->slotFlags, 0, sizeof(int) * NSF_ASM_NR_STATIC_SLOTS);

  proc->ip = proc->code;  /* points to the first writable instructon */
  proc->firstObj = proc->staticObjs;  /* point to the first free obj */
  proc->locals = proc->staticObjs;    /* locals is just an alias     */
  proc->nrAsmArgReferences = 0;
  proc->slots = proc->locals + nrArgs;
  //fprintf(stderr, "args = %ld\n", proc->slots - proc->locals);

  AsmLocalsAlloc(proc, nrArgs + nrLocalObjs);
  /* when freeing, we need something like
    for (i=0; i < nrArgs + nrLocalObjs; i++) {
      if (proc->slotFlags[i] & ASM_SLOT_MUST_DECR) {Tcl_DecrRefCount(proc->slots[i]); }
    }
  */

  /*
   * Second Iteration: emit code
   */
  currentSlot = 0;
  currentAsmInstruction = 0;

  for (i = 0; i < oc; i++) {
    int index, offset, cArgs, argc, codeIndex, argvIndex, j;
    Tcl_Obj *lineObj = ov[i], **argv;

    Tcl_ListObjGetElements(interp, lineObj, &argc, &argv);
    Tcl_GetIndexFromObj(interp, argv[0], asmStatementNames, "asm instruction", 0, &index);

    offset = (asmStatementInfo[index].flags & ASM_INFO_SKIP1) ? 2 : 1;

    cArgs = asmStatementInfo[index].cArgs;
    if (cArgs == NR_PAIRS) {
      cArgs = (argc-offset) / 2;
    } else if (cArgs == NR_PAIRS1) {
      cArgs = 1 + (argc-offset) / 2;
    }

    switch (index) {

    case asmObjProcIdx:
      /* {cmd ::set slot 0 slot 2} */
      cmd = Tcl_GetCommandFromObj(interp, argv[1]);
#if defined(LABEL_THREADING)
      inst = AsmInstructionNew(proc, objProc, cArgs);
      inst->cmd = ((Command *)cmd)->objProc;
#else
      inst = AsmInstructionNew(proc, ((Command *)cmd)->objProc, cArgs);
#endif
      inst->clientData = ((Command *)cmd)->objClientData;
      /* use the assembly word as cmd name; should be ok when we keep assembly around */
      inst->argv[0] = argv[1];
      /*fprintf(stderr, "[%d] %s/%d\n", currentAsmInstruction, Tcl_GetString(argv[1]), 1+((argc-offset)/2));*/

      AsmInstructionArgvSet(interp, offset, argc, 1, inst, proc, argv, 0);
      break;

      /* begin generated code */
$ASSEMBLE_EMIT_CODE
      /* end generated code */
  }

    if ((asmStatementInfo[index].flags & ASM_INFO_DECL) == 0) {
      currentAsmInstruction ++;
    }
  }

  /*
   * add END instruction
   */
  inst = AsmInstructionNew(proc, NULL, 0);

  /*
   * All addresses are determined, apply the argv patches triggered
   * from above.
   */

  for (patchPtr = &patchArray[0]; patchPtr < patches; patchPtr++) {
    fprintf(stderr, "want to patch code[%d]->argv = code[%d]->argv[%d]\n",
	    patchPtr->targetAsmInstruction, patchPtr->sourceAsmInstruction, patchPtr->argvIndex);
    /* set the argument vector of code[1] to the address of code[4]->argv[1] */
    (&proc->code[patchPtr->targetAsmInstruction])->argv =
      &(&proc->code[patchPtr->sourceAsmInstruction])->argv[patchPtr->argvIndex];
  }

  *retAsmProc = proc;

  return TCL_OK;
}
