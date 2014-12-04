package require nx
######################################################################
# The code engine
######################################################################

nsf::proc generate {threadingType:class} {
  set suffix [string trimleft ${threadingType} :]
  set dirName [file dirname [info script]]

  foreach {var value} [${threadingType} generate] {
    set $var $value
  }
 
  set template [readFile $dirName/asmExecuteTemplate$suffix.c]
  writeFile $dirName/nsfAsmExecute$suffix.c [subst -nocommand -nobackslash $template]
  
  set template [readFile $dirName/asmAssembleTemplate.c]
  writeFile $dirName/nsfAsmAssemble.c [subst -nocommand -nobackslash $template]
}

nsf::proc readFile {fn} {set f [open $fn]; set content [read $f]; close $f; return $content}
nsf::proc writeFile {fn content} {
  puts stderr "writing $fn"
  set f [open $fn w]; puts -nonewline $f $content; close $f
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

  :property {asmCheckCode ""}
  :property {asmEmitCode ""}

  :public method cName {} {
    # prepend asm and capitalize first character
    return asm[string toupper [string range ${:name} 0 0]][string range ${:name} 1 end]
  }
  :public method getAsmEmitCode {} {
    return ${:asmEmitCode}
  }

  :public class method "generate assembler" {} {
    set statementIndex {}
    set statementNames {}
    set (ASSEMBLE_EMIT_CODE) ""
    foreach s [lsort [Statement info instances -closure]] {
      if {[$s maxArgs] == 0} {
	puts stderr "ignore statement $s"
	continue
      }
      lappend statementIndex [$s cName]Idx
      lappend statementNames \"[$s name]\"
      
      set emitCode [$s getAsmEmitCode]
      if {$emitCode ne ""} {
	append (ASSEMBLE_EMIT_CODE) "   case [$s cName]Idx:\n$emitCode\n      break;\n\n"
      }
      
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
    array set {} [list \
	STATEMENT_INDICES [join $statementIndex ",\n  "] \
	STATEMENT_NAMES [join $statementNames ",\n  "] \
	STATEMENT_INFO [join $statementInfo ",\n  "] \
	ASSEMBLE_CHECK_CODE ""]

    return [array get {}]
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

  # The property "execNeedsProc" is just needed for call threading,
  # where we have to pass proc via inst->clientData
  :property {execNeedsProc false}

  :public method getAsmEmitCode {} {
    #
    # For every instruction, the C-code allocates an instruction record
    #
    append . \
	"\n\tinst = AsmInstructionNew(proc, [:cName], cArgs);" \
	"\n\tif (cArgs > 0) {AsmInstructionArgvSet(interp, offset, argc, 0, inst, proc, argv, 0);}" \
	[:asmEmitCode]
  }

  :method "code clear" {} {
    set :cCode ""
  }

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

  :method "code mustContain" {value} {
    if {![regexp ${value} ${:cCode}]} {
      error "code does not contain '$value': ${:cCode}"
    }
  }
}

######################################################################
# Code Generator for Label Threading
######################################################################

nx::Class create LabelThreading {

  :public class method generate {} {
    Instruction mixin add [self]::Instruction
    set instructions [lsort [Instruction info instances]]
    set labels {}
    set indices {}
    foreach instruction $instructions {
      append (GENERATED_INSTRUCTIONS) [$instruction generate] \n
      lappend labels &&[$instruction labelName]
      lappend indices IDX_[$instruction cName]
    }

    array set {} [list \
	INSTRUCTION_LABELS [join $labels ",\n    "] \
	INSTRUCTION_INDICES [join $indices ",\n  "] \
        {*}[Statement generate assembler]]

    Instruction mixin delete [self]::Instruction
    return [array get {}]
  }

  nx::Class create [self]::Instruction {
    #
    # This Class is designed as a mixin class for Instruction
    #
    :public method labelName {} {
      return INST_[:cName]
    }
    :method nextInstruction {} {
      if {[:isJump]} {
	:code mustContain NsfAsmJump
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
      :code clear
      :code append [:labelName]:\n
      :code generate
      :nextInstruction
      return [:code get]
    }
  }
}

######################################################################
# Code Generator for Call Threading
######################################################################

nx::Class create CallThreading {

  :public class method generate {} {
    Instruction mixin add [self]::Instruction
    Statement   mixin add [self]::Statement

    foreach instruction [lsort [Instruction info instances]] {
      append (GENERATED_INSTRUCTIONS) [$instruction generate] \n
    }

    array set {} [Statement generate assembler]

    Instruction mixin delete [self]::Instruction
    Statement   mixin delete [self]::Statement

    return [array get {}]
  }

  nx::Class create [self]::Statement {

    :public method asmEmitCode {} {
      set asmEmitCode ${:asmEmitCode}
      if {[:execNeedsProc]} {
	append asmEmitCode "\n\tinst->clientData = proc;\n"
      }
      return $asmEmitCode
    }
  }

  nx::Class create [self]::Instruction {
    #
    # This Class is designed as a mixin class for Instruction
    #

    :public method "code generate" {} {
      set code ${:execCode}
      regsub -all {\mip->argv\M} $code argv code
      regsub -all {\mip->argc\M} $code argc code
      regsub -all {\mip->clientData\M} $code clientData code

      if {[:isJump]} {
	regsub -all {\mip\s*= } $code "proc->ip = " code
	regsub -all {\mip\s*[+][+]} $code "proc->ip++" code
      }

      if {[:returnsResult]} {
	:code append "  int result;\n"
	:code append $code
	:code mustAssign result
	:code append "  return result;\n"
      } else {
	:code append $code
	:code append "  return TCL_OK;\n"
      }
    }
    
    :public method generate {} {
      :code clear
      :code append \
	  "static int [:cName](ClientData clientData, Tcl_Interp *interp, int argc, Tcl_Obj *argv\[]) \{\n"
      if {[:execNeedsProc]} {
	:code append "  AsmCompiledProc *proc = clientData;\n"
      }
      :code generate
      :code append "\}\n"
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
      -minArgs 2 -maxArgs 2 \
      -asmEmitCode {
	proc->slots[currentSlot] = argv[1];
	Tcl_IncrRefCount(proc->slots[currentSlot]);
	proc->slotFlags[currentSlot] |= ASM_SLOT_MUST_DECR;
	currentSlot ++;
      }

  # {var obj 0} 
  # obj is intended to be the varname, but currently ignored
  Declaration create var \
      -minArgs 3 -maxArgs 3 -argTypes asmStatementObjType \
      -asmEmitCode {
	proc->slots[currentSlot] = NULL;
	currentSlot ++;
      }

  # {integer int 0} 
  Declaration create integer \
      -minArgs 3 -maxArgs 3 -argTypes asmStatementIntType \
      -asmEmitCode {
	{
	  int intValue;
	  Tcl_GetIntFromObj(interp, argv[2], &intValue);
	  proc->slots[currentSlot] = INT2PTR(intValue);
	  //fprintf(stderr, "setting slots [%d] = %d\n", currentSlot, intValue);
	  proc->slotFlags[currentSlot] |= ASM_SLOT_IS_INTEGER;
	  currentSlot ++;
	}
      }


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
      -minArgs 3 -maxArgs -1 -cArgs NR_PAIRS -argTypes asmStatementCmdType \
      -returnsResult true \
      -execCode {
	result = Tcl_EvalObjv(interp, ip->argc, ip->argv, 0);
      }

  # {methodDelegateDispatch obj 0 obj 1 obj 2} 
  Instruction create methodDelegateDispatch \
      -name "methodDelegateDispatch" \
      -minArgs 5 -maxArgs -1 -cArgs NR_PAIRS -argTypes asmStatementCmdType \
      -asmEmitCode {
	{ Tcl_Command cmd = NULL;
	  NsfObject *object = NULL;
	  AsmResolverInfo *resInfo;
	  
	  if (strncmp(ObjStr(inst->argv[1]), "::nsf::methods::", 16) == 0) {
	    cmd = Tcl_GetCommandFromObj(interp, inst->argv[1]);
	    //fprintf(stderr, "%s: asmMethod cmd '%s' => %p\n", procName, ObjStr(inst->argv[1]), cmd);
	  }
	  if (strncmp(ObjStr(inst->argv[0]), "::nx::", 6) == 0) {
	    GetObjectFromObj(interp, inst->argv[0], &object);
	    //fprintf(stderr, "%s: asmMethod object '%s' => %p\n", procName, ObjStr(inst->argv[0]), object);
	  }
	  if (cmd && object) {
	    // experimental: bind obj and method
	    resInfo = NEW(AsmResolverInfo); // TODO: LEAK
	    resInfo->cmd = cmd;
	    resInfo->object = object;
	    inst->clientData = resInfo;
	    AsmInstructionSetCmd(inst, asmMethodDelegateDispatch11);
	  } else if (cmd != NULL) {
	    inst->clientData = cmd;
	  } else {	  
	    inst->clientData = NULL;
	  }
	}
      } \
      -returnsResult true \
      -execCode {
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
      -minArgs 3 -maxArgs -1 -cArgs NR_PAIRS -argTypes asmStatementCmdType \
      -asmEmitCode {
	{ Tcl_Command cmd = NULL;	  
	  AsmResolverInfo *resInfo;
	  
	  if (strncmp(ObjStr(inst->argv[0]), "::nsf::methods::", 16) == 0) {
	    cmd = Tcl_GetCommandFromObj(interp, inst->argv[0]);
	    if (cmd != NULL) {
	      //fprintf(stderr, "%s: asmMethodSelfCmdDispatch cmd '%s' => %p\n", procName, ObjStr(inst->argv[0]), cmd);
	      AsmInstructionSetCmd(inst, asmMethodSelfCmdDispatch);
	    }
	  } else {
	    //fprintf(stderr, "%s: asmMethodSelfDispatch cmd '%s'\n", procName, ObjStr(inst->argv[0]));
	  }
	  resInfo = NEW(AsmResolverInfo); // TODO: LEAK
	  resInfo->cmd = cmd;
	  resInfo->proc = proc;
	  inst->clientData = resInfo;
	}
      } \
      -returnsResult true \
      -execCode {
	{
	  AsmResolverInfo *resInfo = ip->clientData;
	  Tcl_Command cmd = (resInfo->cmd != NULL) ? resInfo->cmd : Tcl_GetCommandFromObj(interp, ip->argv[0]);
	  
	  result = MethodDispatch(resInfo->proc->currentObject, interp, 
				  ip->argc, ip->argv, 
				  cmd, resInfo->proc->currentObject, NULL,
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
	  result = Tcl_NRCallObjProc(interp, Tcl_Command_objProc(resInfo->cmd), resInfo->proc->currentObject, 
				     ip->argc, ip->argv);
	}
      }

  # {self} 

  Instruction create self \
      -minArgs 1 -maxArgs 1 \
      -execNeedsProc true \
      -execCode {
	Tcl_SetObjResult(interp, proc->currentObject->cmdName);
      }


  # {jump instruction 2}
  # TODO: maybe define later jump labels in asm source
  Instruction create jump \
      -minArgs 3 -maxArgs 3 -cArgs 1 -argTypes asmStatementInstructionType \
      -execNeedsProc true \
      -isJump true \
      -execCode {
	//fprintf(stderr, "asmJump oc %d instructionIndex %d\n", ip->argc, PTR2INT(ip->argv[0]));
	NsfAsmJump(PTR2INT(ip->argv[0]));
      }
  
  # {jumpTrue instruction 6}
  # TODO: maybe define later jump labels in asm source
  Instruction create jumpTrue \
      -minArgs 3 -maxArgs 3 -cArgs 1 -argTypes asmStatementInstructionType \
      -execNeedsProc true \
      -isJump true \
      -execCode {
	if (proc->status != 0) {
	  //fprintf(stderr, "asmJumpTrue jump oc %d instructionIndex %d\n", ip->argc, PTR2INT(ip->argv[0]));
	  NsfAsmJump(PTR2INT(ip->argv[0]));
	} else {
	  //fprintf(stderr, "asmJumpTrue fall through\n");
	  NsfAsmJumpNext();
	}
      }
  
  # {leIntObj slot 4 slot 7}

  Instruction create leIntObj \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmStatementSlotType \
      -execNeedsProc true \
      -execCode {
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
      }

  # {leInt slot 4 slot 7}

  Instruction create leInt \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmStatementSlotType \
      -execNeedsProc true \
      -execCode {
	{
	  int value1, value2;
	  value1 = PTR2INT(proc->slots[PTR2INT(ip->argv[0])]);
	  value2 = PTR2INT(proc->slots[PTR2INT(ip->argv[1])]);
	  proc->status = value1 <= value2;
	}
      }


  # {duplicateObj slot 6 obj 2}  
  # TODO: should force first arg "slot"
  Instruction create duplicateObj \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmStatementSlotObjArgType \
      -execNeedsProc true \
      -execCode {
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
      }


  # {setObj slot 2 arg 0}
  # TODO: should force first arg "slot"
  Instruction create setObj \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmStatementSlotObjArgType \
      -execNeedsProc true \
      -execCode {
	//fprintf(stderr, "setObj var[%d] = %s\n", PTR2INT(ip->argv[0]), ObjStr(ip->argv[1]));  
	proc->slots[PTR2INT(ip->argv[0])] = ip->argv[1];
      }

  # {setInt slot 6 int 0}
  # TODO: should force first arg "slot"
  Instruction create setInt \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmStatementSlotIntType \
      -execNeedsProc true \
      -execCode {
	proc->slots[PTR2INT(ip->argv[0])] = ip->argv[1];
      }

  # {setObjToResult slot 5}
  Instruction create setObjToResult \
      -minArgs 3 -maxArgs 3 -cArgs 2 -argTypes asmStatementSlotType \
      -execNeedsProc true \
      -execCode {
	//fprintf(stderr, "setObjToResult var[%d] = %s\n", PTR2INT(ip->argv[0]), ObjStr(ip->argv[1]));  
	proc->slots[PTR2INT(ip->argv[0])] = Tcl_GetObjResult(interp);
      }
  
  # {setResult slot 6}
  Instruction create setResult \
      -minArgs 3 -maxArgs 3 -cArgs 1 -argTypes asmStatementSlotType \
      -execNeedsProc true \
      -execCode {
	Tcl_SetObjResult(interp, proc->slots[PTR2INT(ip->argv[0])]);
      }

  # {setResultInt slot 6}
  Instruction create setResultInt \
      -minArgs 3 -maxArgs 3 -cArgs 1 -argTypes asmStatementSlotType \
      -execNeedsProc true \
      -execCode {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(PTR2INT(proc->slots[PTR2INT(ip->argv[0])])));
      }

  # {store code 4 argv 2}
  Instruction create storeResult \
      -minArgs 5 -maxArgs 5 -cArgs 0 -argTypes asmStatementStoreType \
      -asmEmitCode {
	codeIndex = -1;
	argvIndex = -1;
	for (j = offset; j < argc; j += 2) {
	    int argIndex, intValue;
	    Tcl_GetIndexFromObj(interp, argv[j], asmStatementArgType, "asm internal arg type", 0, &argIndex);
	    Tcl_GetIntFromObj(interp, argv[j+1], &intValue);
	    switch (argIndex) {
	      case asmStatementArgTypeInstructionIdx: codeIndex = intValue; break;
	      case asmStatementArgTypeArgvIdx: argvIndex = intValue; break;
	    }
	  }
	// TODO: CHECK codeIndex, argvIndex (>0, reasonable values)
	//fprintf(stderr, "%p setting instruction %d => %d %d\n", patches, currentAsmInstruction, codeIndex, argvIndex);
	patches->targetAsmInstruction = currentAsmInstruction;
	patches->sourceAsmInstruction = codeIndex;
	patches->argvIndex = argvIndex;
	patches++;
      } -execCode {
	ip->argv[0] = Tcl_GetObjResult(interp);
	Tcl_IncrRefCount(ip->argv[0]);
      }

  # {incrObj slot 6 slot 7} 
  Instruction create incrObj \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmStatementSlotType \
      -execNeedsProc true \
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
	  
	  //Tcl_SetObjResult(interp, intObj);
	}
      } 

  # {incrInt slot 6 slot 7} 
  Instruction create incrInt \
      -minArgs 5 -maxArgs 5 -cArgs 2 -argTypes asmStatementSlotType \
      -execNeedsProc true \
      -execCode {
	{
	  int intValue, incrValue;
	  //fprintf(stderr, "incrInt var[%d] incr var[%d]\n", PTR2INT(ip->argv[0]), PTR2INT(ip->argv[1]));
	  intValue  = PTR2INT(proc->slots[PTR2INT(ip->argv[0])]);
	  incrValue = PTR2INT(proc->slots[PTR2INT(ip->argv[1])]);
	  //fprintf(stderr, ".... intValue %d incr Value %d\n", intValue, incrValue);
	  
	  proc->slots[PTR2INT(ip->argv[0])] = INT2PTR(intValue + incrValue);
	  //fprintf(stderr, ".... [%d] => %d\n", PTR2INT(ip->argv[0]), intValue + incrValue);
	}
      } 

}

######################################################################
# generate the code
######################################################################
  
generate ::LabelThreading
generate ::CallThreading
