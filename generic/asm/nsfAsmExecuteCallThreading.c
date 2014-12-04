
static int asmEval(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  int result;

	result = Tcl_EvalObjv(interp, argc, argv, 0);
        return result;
}

static int asmDuplicateObj(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	{
	  int indexValue = PTR2INT(argv[0]);
	  //fprintf(stderr, "duplicateObj var[%d] = %s\n", indexValue, ObjStr(argv[1]));  
	  if (proc->slots[indexValue]) {
	    Tcl_DecrRefCount(proc->slots[indexValue]);
	  }
	  proc->slots[indexValue] = Tcl_DuplicateObj(argv[1]); 
	  Tcl_IncrRefCount(proc->slots[indexValue]); 
	  proc->slotFlags[indexValue] |= ASM_SLOT_MUST_DECR;
	}
        return TCL_OK;
}

static int asmIncrInt(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	{
	  int intValue, incrValue;
	  //fprintf(stderr, "incrInt var[%d] incr var[%d]\n", PTR2INT(argv[0]), PTR2INT(argv[1]));
	  intValue  = PTR2INT(proc->slots[PTR2INT(argv[0])]);
	  incrValue = PTR2INT(proc->slots[PTR2INT(argv[1])]);
	  //fprintf(stderr, ".... intValue %d incr Value %d\n", intValue, incrValue);
	  
	  proc->slots[PTR2INT(argv[0])] = INT2PTR(intValue + incrValue);
	  //fprintf(stderr, ".... [%d] => %d\n", PTR2INT(argv[0]), intValue + incrValue);
	}
        return TCL_OK;
}

static int asmIncrObj(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	{
	  int intValue, incrValue;
	  Tcl_Obj *intObj, *incrObj;
	  
	  //fprintf(stderr, "asmIncrScalar var[%d] incr var[%d], ", PTR2INT(argv[0]), PTR2INT(argv[1]));
	  
	  intObj = proc->slots[PTR2INT(argv[0])];
	  incrObj = proc->slots[PTR2INT(argv[1])];
	  
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
	  
	  //fprintf(stderr, "updated %p var[%d] %p\n",  intObj, PTR2INT(argv[0]), proc->slots[PTR2INT(argv[0])]);
	  
	  //Tcl_SetObjResult(interp, intObj);
	}
        return TCL_OK;
}

static int asmJump(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	//fprintf(stderr, "asmJump oc %d instructionIndex %d\n", argc, PTR2INT(argv[0]));
	NsfAsmJump(PTR2INT(argv[0]));
        return TCL_OK;
}

static int asmJumpTrue(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	if (proc->status != 0) {
	  //fprintf(stderr, "asmJumpTrue jump oc %d instructionIndex %d\n", argc, PTR2INT(argv[0]));
	  NsfAsmJump(PTR2INT(argv[0]));
	} else {
	  //fprintf(stderr, "asmJumpTrue fall through\n");
	  NsfAsmJumpNext();
	}
        return TCL_OK;
}

static int asmLeInt(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	{
	  int value1, value2;
	  value1 = PTR2INT(proc->slots[PTR2INT(argv[0])]);
	  value2 = PTR2INT(proc->slots[PTR2INT(argv[1])]);
	  proc->status = value1 <= value2;
	}
        return TCL_OK;
}

static int asmLeIntObj(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	{
	  int value1, value2;
	  Tcl_Obj *obj;
	  //fprintf(stderr, "leIntObj oc %d op1 %p op2 %p\n", argc, argv[0], argv[1]);
	  
	  // for the time being, we compare two int values
	  obj = proc->slots[PTR2INT(argv[0])];
	  if (likely(obj->typePtr == Nsf_OT_intType)) {
	    value1 = obj->internalRep.longValue;
	  } else {
	    Tcl_GetIntFromObj(interp, obj, &value1);
	  }
	  obj = proc->slots[PTR2INT(argv[1])];
	  if (likely(obj->typePtr == Nsf_OT_intType)) {
	    value2 = obj->internalRep.longValue;
	  } else {
	    Tcl_GetIntFromObj(interp, obj, &value2);
	  }
	  //fprintf(stderr, "asmLeScalar oc %d op1 %d op2 %d => %d\n", argc, value1, value2, value1 <= value2);
	  
	  proc->status = value1 <= value2;
	}
        return TCL_OK;
}

static int asmMethodDelegateDispatch(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  int result;

	{ Tcl_Command cmd = NULL;
	  NsfObject *object;

	  // obj and method are unresolved
	  result = GetObjectFromObj(interp, argv[0], &object);
	  if (likely(clientData != NULL)) {
	    cmd = clientData;
	  } else {
	    cmd = Tcl_GetCommandFromObj(interp, argv[1]);
	  }
	  //fprintf(stderr, "cmd %p object %p\n", cmd, object);
	  result = MethodDispatch(object, interp, argc-1, argv+1, cmd, object, NULL,
				  ObjStr(argv[1]), 0, 0);    
	}
        return result;
}

static int asmMethodDelegateDispatch11(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  int result;

	// obj and method are resolved
	{
	  AsmResolverInfo *resInfo = clientData;
	  result = MethodDispatch(resInfo->object, interp, argc-1, argv+1, 
				  resInfo->cmd, resInfo->object, NULL,
				  ObjStr(argv[1]), 0, 0);
	}
        return result;
}

static int asmMethodSelfCmdDispatch(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  int result;

	{
	  AsmResolverInfo *resInfo = clientData;
	  assert(resInfo->cmd != NULL);
	  result = Tcl_NRCallObjProc(interp, Tcl_Command_objProc(resInfo->cmd), resInfo->proc->currentObject, 
				     argc, argv);
	}
        return result;
}

static int asmMethodSelfDispatch(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  int result;

	{
	  AsmResolverInfo *resInfo = clientData;
	  Tcl_Command cmd = (resInfo->cmd != NULL) ? resInfo->cmd : Tcl_GetCommandFromObj(interp, argv[0]);
	  
	  result = MethodDispatch(resInfo->proc->currentObject, interp, 
				  argc, argv, 
				  cmd, resInfo->proc->currentObject, NULL,
				  ObjStr(argv[0]), 0, 0);
	}
        return result;
}

static int asmNoop(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  return TCL_OK;
}

static int asmSelf(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	Tcl_SetObjResult(interp, proc->currentObject->cmdName);
        return TCL_OK;
}

static int asmSetInt(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	proc->slots[PTR2INT(argv[0])] = argv[1];
        return TCL_OK;
}

static int asmSetObj(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	//fprintf(stderr, "setObj var[%d] = %s\n", PTR2INT(argv[0]), ObjStr(argv[1]));  
	proc->slots[PTR2INT(argv[0])] = argv[1];
        return TCL_OK;
}

static int asmSetObjToResult(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	//fprintf(stderr, "setObjToResult var[%d] = %s\n", PTR2INT(argv[0]), ObjStr(argv[1]));  
	proc->slots[PTR2INT(argv[0])] = Tcl_GetObjResult(interp);
        return TCL_OK;
}

static int asmSetResult(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	Tcl_SetObjResult(interp, proc->slots[PTR2INT(argv[0])]);
        return TCL_OK;
}

static int asmSetResultInt(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {
  AsmCompiledProc *proc = clientData;

	Tcl_SetObjResult(interp, Tcl_NewIntObj(PTR2INT(proc->slots[PTR2INT(argv[0])])));
        return TCL_OK;
}

static int asmStoreResult(ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv[]) {

	argv[0] = Tcl_GetObjResult(interp);
	Tcl_IncrRefCount(argv[0]);
        return TCL_OK;
}

;

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
  if (compiledLocals != NULL) {
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


