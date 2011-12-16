package require nx

######################################################################
# The code engine
######################################################################

nsf::proc generate {threadingType:class} {
  Instruction mixin add ${threadingType}::Instruction
  set suffix [string trimleft ${threadingType} :]
  set dirName [file dirname [info script]]
  set fn $dirName/asmExecuteTemplate$suffix.c
  set f [open $fn]; set template [read $f]; close $f
  set instructions [lsort [Instruction info instances]]
  set labels {}
  set indices {}
  foreach instruction $instructions {
    append GENERATED_INSTRUCTIONS [$instruction generate] \n
    lappend labels &&[$instruction labelName]
    lappend indices IDX_[$instruction cName]
  }
  Instruction mixin delete ${threadingType}::Instruction
  set INSTRUCTION_LABELS [join $labels ",\n    "]
  set INSTRUCTION_INDICES [join $indices ",\n  "]

  set statementIndex {}
  set statementNames {}
  foreach s [lsort [Statement info instances -closure]] {
    if {[$s maxArgs] == 0} {
      puts stderr "ignore statement $s"
      continue
    }
    lappend statementIndex [$s cName]Idx
    lappend statementNames \"[$s name]\"
    set flags 0
    if {[$s info has type ::Declaration]} {
      lappend flags ASM_INFO_DECL
    }
    if {[$s mustContainPairs]} {
      lappend flags ASM_INFO_PAIRS
    }
    lappend statementInfo \
	"/* [$s cName] */\n  {[join $flags |], [$s argTypes], [$s minArgs], [$s maxArgs], [$s cArgs]}"
  }
  set STATEMENT_INDICES [join $statementIndex ",\n  "]
  set STATEMENT_NAMES [join $statementNames ",\n  "]
  set STATEMENT_INFO [join $statementInfo ",\n  "]

  #puts stderr statementIndex=$statementIndex
  #puts stderr statementNames=$statementNames
  
  set f [open $dirName/nsfAsmExecute$suffix.c w]
  puts $f [subst -nocommand -nobackslash $template]
  close $f
}

######################################################################
# Basic Class for Instructions and Declarations
######################################################################
nx::Class create Statement {
  :property {name "[namespace tail [self]]"}
  :property {mustContainPairs true}
  :property {argTypes NULL}
  :property {minArgs 0}
  :property {maxArgs 0}
  :property {cArgs 0}

  :public method cName {} {
    # prepend asm and capitalize first character
    return asm[string toupper [string range ${:name} 0 0]][string range ${:name} 1 end]
  }
}

######################################################################
# Basic Class for Instructions and Declarations
######################################################################
nx::Class create Declaration -superclass Statement {
}

######################################################################
# Basic Class for defining Instructions independent of the code
# generator (label threading, call threading)
######################################################################

nx::Class create Instruction -superclass Statement {
  :property {execCode ""}

  :property {isJump false}
  :property {returnsResult false}

  :method "code get" {} {
    return ${:cCode}
  }

  :method "code append" {value} {
    append :cCode $value
  }

  :method "code mustAssign" {value} {
    if {![regexp "\\m${value}\\M\\s*=" ${:cCode}]} {
      error "code does not assign variable '$value': ${:cCode}"
    }
  }
}

######################################################################
# Code Generator for Label Threading
######################################################################

nx::Class create LabelThreading {
  nx::Class create [self]::Instruction {
    #
    # This Class is designed as a mixin class for Instruction
    #
    :public method labelName {} {
      return INST_[:cName]
    }
    :method nextInstruction {} {
      if {[:isJump]} {
	:code mustAssign ip
	:code append "\n  goto *instructionLabel\[ip->labelIdx];\n"
      } else {
	:code append "\n  ip++;\n  goto *instructionLabel\[ip->labelIdx];\n"
      }
    }
    :public method "code generate" {} {
      :code append ${:execCode}
      if {[:returnsResult]} {
	:code mustAssign result
	:code append "  goto EXEC_RESULT_CODE_HANDLER;\n"
      }
    }
    
    :public method generate {} {
      :code append [:labelName]:\n
      :code generate
      :nextInstruction
      return [:code get]
    }
  }
}

namespace eval ::asm {
  ######################################################################
  # Declarations
  ######################################################################

  # {obj a}
  Declaration create obj \
      -mustContainPairs false \
      -minArgs 2 -maxArgs 2

  # {var obj 0} 
  # should force arg to "obj"
  # obj is intended to be the varname, but currently ignored
  Declaration create var \
      -minArgs 3 -maxArgs 3 -argTypes asmCmdArgTypes



  ######################################################################
  # Instructions
  ######################################################################

  # {noop} 
  Instruction create noop \
      -mustContainPairs false \
      -minArgs 1 -maxArgs 1

  #  {eval obj 0 obj 1 obj 2}
  Instruction create dispatch \
      -name "eval" \
      -minArgs 3 -maxArgs -1 -cArgs NR_PAIRS -argTypes asmCmdArgTypes \
      -returnsResult true \
      -execCode {
	result = Tcl_EvalObjv(interp, ip->argc, ip->argv, 0);
      }

  # {methodDelegateDispatch obj 0 obj 1 obj 2} 
  Instruction create methodDelegateDispatch \
      -name "methodDelegateDispatch" \
      -minArgs 5 -maxArgs -1 -cArgs NR_PAIRS -argTypes asmCmdArgTypes \
      -returnsResult true \
      -execCode {
	// obj and method are unresolved
	result = GetObjectFromObj(interp, ip->argv[0], &object);
	if (likely(ip->clientData != NULL)) {
	  cmd = clientData;
	} else {
	  cmd = Tcl_GetCommandFromObj(interp, ip->argv[1]);
	}
	//fprintf(stderr, "cmd %p object %p\n", cmd, object);
	result = MethodDispatch(object, interp, ip->argc-1, ip->argv+1, cmd, object, NULL,
				ObjStr(ip->argv[1]), 0, 0);    
      }
  
  # methodDelegateDispatch11 is an optimized variant of
  # methodDelegateDispatch, emitted alternatively by the assembler for
  # the above instruction.
  Instruction create methodDelegateDispatch11 \
      -returnsResult true \
      -execCode {
	// obj and method are resolved
	{
	  AsmResolverInfo *resInfo = ip->clientData;
	  result = MethodDispatch(resInfo->object, interp, ip->argc-1, ip->argv+1, 
				  resInfo->cmd, resInfo->object, NULL,
				  ObjStr(ip->argv[1]), 0, 0);
	}
      }


  # {methodSelfDispatch obj 0 obj 1 obj 2} 

  Instruction create methodSelfDispatch \
      -minArgs 3 -maxArgs -1 -cArgs NR_PAIRS -argTypes asmCmdArgTypes \
      -returnsResult true \
      -execCode {
	{
	  AsmResolverInfo *resInfo = ip->clientData;
	  Tcl_Command cmd = resInfo->cmd ? resInfo->cmd : Tcl_GetCommandFromObj(interp, ip->argv[0]);
	  
	  result = MethodDispatch(resInfo->asmProc->currentObject, interp, 
				  ip->argc, ip->argv, 
				  cmd, resInfo->asmProc->currentObject, NULL,
				  ObjStr(ip->argv[0]), 0, 0);
	}
      }

  # methodSelfCmdDispatch is an optimized variant of
  # methodSelfDispatch, emitted alternatively by the assembler for the
  # above instruction.
  Instruction create methodSelfCmdDispatch \
      -returnsResult true \
      -execCode {
	{
	  AsmResolverInfo *resInfo = ip->clientData;
	  assert(resInfo->cmd != NULL);
	  result = Tcl_NRCallObjProc(interp, Tcl_Command_objProc(resInfo->cmd), resInfo->asmProc->currentObject, 
				     ip->argc, ip->argv);
	}
      }

  # {self} 
  # TODO: rename instruction to self ? why "method"
  Instruction create methodSelf \
      -minArgs 1 -maxArgs 1 \
      -execCode {
	Tcl_SetObjResult(interp, proc->currentObject->cmdName);
      }


  # {jump int 2}
  # TODO: should force arg1 "int", maybe define later jump labels in asm source
  Instruction create jump \
      -minArgs 3 -maxArgs 3 -cArgs 1 -argTypes asmCmdArgTypes \
      -isJump true \
      -execCode {
	//fprintf(stderr, "asmJump oc %d instructionIndex %d\n", ip->argc, PTR2INT(ip->argv[0]));
	ip = &proc->code[PTR2INT(ip->argv[0])];
      }
  
  # {jumpTrue int 6}
  # TODO: should force arg1 "int", maybe define later jump labels in asm source
  Instruction create jumpTrue \
      -minArgs 3 -maxArgs 3 -cArgs 1 -argTypes asmCmdArgTypes \
      -isJump true \
      -execCode {
	if (proc->status) {
	  //fprintf(stderr, "asmJumpTrue jump oc %d instructionIndex %d\n", ip->argc, PTR2INT(ip->argv[0]));
	  ip = &proc->code[PTR2INT(ip->argv[0])];
	} else {
	  //fprintf(stderr, "asmJumpTrue fall through\n");
	  ip++;
	}
      }
  
  # {leScalar int 4 int 7}
  # TODO: should force arg1 & arg2 "int" 
  Instruction create leScalar \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmCmdArgTypes \
      -execCode {
	{
	  int value1, value2;
	  Tcl_Obj *obj;
	  //fprintf(stderr, "asmLeScalar oc %d op1 %p op2 %p\n", ip->argc, ip->argv[0], ip->argv[1]);
	  
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
      }


  # {copyScalar int 6 obj 2}  
  # TODO: rename copyObj
  # TODO: should force arg1 "int"

  Instruction create copyScalar \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmCmdArgTypes \
      -execCode {
	indexValue = PTR2INT(ip->argv[0]);
	//fprintf(stderr, "asmCopyScalar var[%d] = %s\n", indexValue, ObjStr(ip->argv[1]));  
	if (proc->slots[indexValue]) {
	  Tcl_DecrRefCount(proc->slots[indexValue]);
	}
	proc->slots[indexValue] = Tcl_DuplicateObj(ip->argv[1]); 
	Tcl_IncrRefCount(proc->slots[indexValue]); // TODO: Leak? .. Clear all these vars when freeing the proc, or stack
      }


  # {setScalar int 2 arg 0}
  # TODO: should force arg1 "int"
  Instruction create setScalar \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmCmdArgTypes \
      -execCode {
	indexValue = PTR2INT(ip->argv[0]);
	//fprintf(stderr, "asmSetScalar var[%d] = %s\n", indexValue, ObjStr(ip->argv[1]));  
	proc->slots[indexValue] = ip->argv[1];
      }

  # {setScalarResult int 5}
  # TODO: should force arg1 "int"
  Instruction create setScalarResult \
      -minArgs 3 -maxArgs 3 -cArgs 2 -argTypes asmCmdArgTypes \
      -execCode {
	indexValue = PTR2INT(ip->argv[0]);
	//fprintf(stderr, "asmSetScalar var[%d] = %s\n", indexValue, ObjStr(ip->argv[1]));  
	proc->slots[indexValue] = Tcl_GetObjResult(interp);
      }
  
  # {setResult int 6}
  # TODO: should force arg1 "int"
  Instruction create setResult \
      -minArgs 3 -maxArgs 3 -cArgs 1 -argTypes asmCmdArgTypes \
      -execCode {
	indexValue = PTR2INT(ip->argv[0]);
	Tcl_SetObjResult(interp, proc->slots[indexValue]);
	//fprintf(stderr, "asmSetResult index %d => '%s'\n", indexValue, ObjStr(proc->slots[indexValue]));
      }


  # {store code 4 argv 2}
  Instruction create storeResult \
      -minArgs 5 -maxArgs 5 -cArgs 0 -argTypes asmAddrTypes \
      -execCode {
	ip->argv[0] = Tcl_GetObjResult(interp);
	Tcl_IncrRefCount(ip->argv[0]);
      }

  # {incrScalar int 6 int 7} 
  # TODO: should force arg1&2 "int"
  Instruction create incrScalar \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmCmdArgTypes \
      -execCode {
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
	  
	  Tcl_SetObjResult(interp, intObj);
	}
      }    

}

######################################################################
# generate the code
######################################################################
  
generate ::LabelThreading