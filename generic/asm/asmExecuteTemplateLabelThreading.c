
enum instructionIdx { 
  IDX_objProc,
  $INSTRUCTION_INDICES,
  IDX_NULL
};

/*
 *----------------------------------------------------------------------
 * AsmExecute --
 *
 *    Define the execution engine for the code
 *
 *----------------------------------------------------------------------
 */
int AsmExecute(ClientData cd, Tcl_Interp *interp, AsmCompiledProc *proc, int argc, Tcl_Obj *CONST argv[]) {
  int i, result = TCL_OK;
  AsmInstruction *ip; 

  static void *instructionLabel[] = { 
    &&INST_objProc,
    $INSTRUCTION_LABELS,
    &&INST_NULL
  };


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
  ip = proc->code; 
  proc->status = 0;  
 
  //fprintf(stderr, "AsmExecute jumps to %p\n", ip);

  goto *instructionLabel[ip->labelIdx];

 INST_NULL:
  return result;

 EXEC_RESULT_CODE_HANDLER:
  if (likely(result == TCL_OK)) {
    ip++;
    goto *instructionLabel[ip->labelIdx];
  } else {
    return result;
  }

 INST_objProc:
  result = (*ip->cmd)(ip->clientData, interp, ip->argc, ip->argv);
  goto EXEC_RESULT_CODE_HANDLER;
  
  $GENERATED_INSTRUCTIONS
}

