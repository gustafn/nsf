
enum instructionIdx { 
  IDX_objProc,
  IDX_asmEval,
  IDX_asmDuplicateObj,
  IDX_asmIncrInt,
  IDX_asmIncrObj,
  IDX_asmJump,
  IDX_asmJumpTrue,
  IDX_asmLeInt,
  IDX_asmLeIntObj,
  IDX_asmMethodDelegateDispatch,
  IDX_asmMethodDelegateDispatch11,
  IDX_asmMethodSelfCmdDispatch,
  IDX_asmMethodSelfDispatch,
  IDX_asmNoop,
  IDX_asmSelf,
  IDX_asmSetInt,
  IDX_asmSetObj,
  IDX_asmSetObjToResult,
  IDX_asmSetResult,
  IDX_asmSetResultInt,
  IDX_asmStoreResult,
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
    &&INST_asmEval,
    &&INST_asmDuplicateObj,
    &&INST_asmIncrInt,
    &&INST_asmIncrObj,
    &&INST_asmJump,
    &&INST_asmJumpTrue,
    &&INST_asmLeInt,
    &&INST_asmLeIntObj,
    &&INST_asmMethodDelegateDispatch,
    &&INST_asmMethodDelegateDispatch11,
    &&INST_asmMethodSelfCmdDispatch,
    &&INST_asmMethodSelfDispatch,
    &&INST_asmNoop,
    &&INST_asmSelf,
    &&INST_asmSetInt,
    &&INST_asmSetObj,
    &&INST_asmSetObjToResult,
    &&INST_asmSetResult,
    &&INST_asmSetResultInt,
    &&INST_asmStoreResult,
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
  
  INST_asmEval:

	result = Tcl_EvalObjv(interp, ip->argc, ip->argv, 0);
        goto EXEC_RESULT_CODE_HANDLER;

  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmDuplicateObj:

	{
	  int indexValue = PTR2INT(ip->argv[0]);
	  //fprintf(stderr, "duplicateObj var[%d] = %s\n", indexValue, ObjStr(ip->argv[1]));  
	  if (proc->slots[indexValue]) {
	    Tcl_DecrRefCount(proc->slots[indexValue]);
	  }
	  proc->slots[indexValue] = Tcl_DuplicateObj(ip->argv[1]); 
	  Tcl_IncrRefCount(proc->slots[indexValue]); 
	  proc->slotFlags[indexValue] |= ASM_SLOT_MUST_DECR;
	}
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmIncrInt:

	{
	  int intValue, incrValue;
	  //fprintf(stderr, "incrInt var[%d] incr var[%d]\n", PTR2INT(ip->argv[0]), PTR2INT(ip->argv[1]));
	  intValue  = PTR2INT(proc->slots[PTR2INT(ip->argv[0])]);
	  incrValue = PTR2INT(proc->slots[PTR2INT(ip->argv[1])]);
	  //fprintf(stderr, ".... intValue %d incr Value %d\n", intValue, incrValue);
	  
	  proc->slots[PTR2INT(ip->argv[0])] = INT2PTR(intValue + incrValue);
	  //fprintf(stderr, ".... [%d] => %d\n", PTR2INT(ip->argv[0]), intValue + incrValue);
	}
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmIncrObj:

	{
	  int intValue, incrValue;
	  Tcl_Obj *intObj, *incrObj;
	  
	  //fprintf(stderr, "asmIncrScalar var[%d] incr var[%d], ", PTR2INT(ip->argv[0]), PTR2INT(ip->argv[1]));
	  
	  intObj = proc->slots[PTR2INT(ip->argv[0])];
	  incrObj = proc->slots[PTR2INT(ip->argv[1])];
	  
	  if (likely(intObj->typePtr == Nsf_OT_intType)) {
	    intValue = intObj->internalRep.longValue;
	  } else {
	    Tcl_GetIntFromObj(interp, intObj, &intValue);
	  }
	  
	  if (likely(incrObj->typePtr == Nsf_OT_intType)) {
	    incrValue = incrObj->internalRep.longValue;
	  } else {
	    Tcl_GetIntFromObj(interp, incrObj, &incrValue);
	  }
	  
	  //fprintf(stderr, "%d + %d = %d,", intValue, incrValue, intValue + incrValue);
	  
	  Tcl_InvalidateStringRep(intObj);
	  intObj->internalRep.longValue = (long)(intValue + incrValue);
	  
	  //fprintf(stderr, "updated %p var[%d] %p\n",  intObj, PTR2INT(ip->argv[0]), proc->slots[PTR2INT(ip->argv[0])]);
	  
	  //Tcl_SetObjResult(interp, intObj);
	}
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmJump:

	//fprintf(stderr, "asmJump oc %d instructionIndex %d\n", ip->argc, PTR2INT(ip->argv[0]));
	NsfAsmJump(PTR2INT(ip->argv[0]));
      
  goto *instructionLabel[ip->labelIdx];

INST_asmJumpTrue:

	if (proc->status) {
	  //fprintf(stderr, "asmJumpTrue jump oc %d instructionIndex %d\n", ip->argc, PTR2INT(ip->argv[0]));
	  NsfAsmJump(PTR2INT(ip->argv[0]));
	} else {
	  //fprintf(stderr, "asmJumpTrue fall through\n");
	  NsfAsmJumpNext();
	}
      
  goto *instructionLabel[ip->labelIdx];

INST_asmLeInt:

	{
	  int value1, value2;
	  value1 = PTR2INT(proc->slots[PTR2INT(ip->argv[0])]);
	  value2 = PTR2INT(proc->slots[PTR2INT(ip->argv[1])]);
	  proc->status = value1 <= value2;
	}
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmLeIntObj:

	{
	  int value1, value2;
	  Tcl_Obj *obj;
	  //fprintf(stderr, "leIntObj oc %d op1 %p op2 %p\n", ip->argc, ip->argv[0], ip->argv[1]);
	  
	  // for the time being, we compare two int values
	  obj = proc->slots[PTR2INT(ip->argv[0])];
	  if (likely(obj->typePtr == Nsf_OT_intType)) {
	    value1 = obj->internalRep.longValue;
	  } else {
	    Tcl_GetIntFromObj(interp, obj, &value1);
	  }
	  obj = proc->slots[PTR2INT(ip->argv[1])];
	  if (likely(obj->typePtr == Nsf_OT_intType)) {
	    value2 = obj->internalRep.longValue;
	  } else {
	    Tcl_GetIntFromObj(interp, obj, &value2);
	  }
	  //fprintf(stderr, "asmLeScalar oc %d op1 %d op2 %d => %d\n", ip->argc, value1, value2, value1 <= value2);
	  
	  proc->status = value1 <= value2;
	}
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmMethodDelegateDispatch:

	{ Tcl_Command cmd = NULL;
	  NsfObject *object;

	  // obj and method are unresolved
	  result = GetObjectFromObj(interp, ip->argv[0], &object);
	  if (likely(ip->clientData != NULL)) {
	    cmd = ip->clientData;
	  } else {
	    cmd = Tcl_GetCommandFromObj(interp, ip->argv[1]);
	  }
	  //fprintf(stderr, "cmd %p object %p\n", cmd, object);
	  result = MethodDispatch(object, interp, ip->argc-1, ip->argv+1, cmd, object, NULL,
				  ObjStr(ip->argv[1]), 0, 0);    
	}
        goto EXEC_RESULT_CODE_HANDLER;

  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmMethodDelegateDispatch11:

	// obj and method are resolved
	{
	  AsmResolverInfo *resInfo = ip->clientData;
	  result = MethodDispatch(resInfo->object, interp, ip->argc-1, ip->argv+1, 
				  resInfo->cmd, resInfo->object, NULL,
				  ObjStr(ip->argv[1]), 0, 0);
	}
        goto EXEC_RESULT_CODE_HANDLER;

  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmMethodSelfCmdDispatch:

	{
	  AsmResolverInfo *resInfo = ip->clientData;
	  assert(resInfo->cmd != NULL);
	  result = Tcl_NRCallObjProc(interp, Tcl_Command_objProc(resInfo->cmd), resInfo->proc->currentObject, 
				     ip->argc, ip->argv);
	}
        goto EXEC_RESULT_CODE_HANDLER;

  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmMethodSelfDispatch:

	{
	  AsmResolverInfo *resInfo = ip->clientData;
	  Tcl_Command cmd = resInfo->cmd ? resInfo->cmd : Tcl_GetCommandFromObj(interp, ip->argv[0]);
	  
	  result = MethodDispatch(resInfo->proc->currentObject, interp, 
				  ip->argc, ip->argv, 
				  cmd, resInfo->proc->currentObject, NULL,
				  ObjStr(ip->argv[0]), 0, 0);
	}
        goto EXEC_RESULT_CODE_HANDLER;

  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmNoop:

  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmSelf:

	Tcl_SetObjResult(interp, proc->currentObject->cmdName);
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmSetInt:

	proc->slots[PTR2INT(ip->argv[0])] = ip->argv[1];
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmSetObj:

	//fprintf(stderr, "setObj var[%d] = %s\n", PTR2INT(ip->argv[0]), ObjStr(ip->argv[1]));  
	proc->slots[PTR2INT(ip->argv[0])] = ip->argv[1];
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmSetObjToResult:

	//fprintf(stderr, "setObjToResult var[%d] = %s\n", PTR2INT(ip->argv[0]), ObjStr(ip->argv[1]));  
	proc->slots[PTR2INT(ip->argv[0])] = Tcl_GetObjResult(interp);
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmSetResult:

	Tcl_SetObjResult(interp, proc->slots[PTR2INT(ip->argv[0])]);
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmSetResultInt:

	Tcl_SetObjResult(interp, Tcl_NewIntObj(PTR2INT(proc->slots[PTR2INT(ip->argv[0])])));
      
  ip++;
  goto *instructionLabel[ip->labelIdx];

INST_asmStoreResult:

	ip->argv[0] = Tcl_GetObjResult(interp);
	Tcl_IncrRefCount(ip->argv[0]);
      
  ip++;
  goto *instructionLabel[ip->labelIdx];


}

