
$GENERATED_INSTRUCTIONS;

/*
 *----------------------------------------------------------------------
 * AsmExecute --
 *
 *    Define the execution engine for the code
 *
 *----------------------------------------------------------------------
 */
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


