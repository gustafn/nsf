/* 
 *  
 *  Extended Object Tcl (XOTcl)
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *
 *  nsfShadow.c --
 *  
 *  Shadowing (overloading) and accessing global tcl obj commands
 *  
 */

#include "nsfInt.h"
#include "nsfAccessInt.h"

static int
NsfReplaceCommandCleanup(Tcl_Interp *interp, NsfGlobalNames name) {
  Tcl_Command cmd;
  int result = TCL_OK;
  NsfShadowTclCommandInfo *ti = &RUNTIME_STATE(interp)->tclCommands[name-NSF_EXPR];

  /*fprintf(stderr," cleanup for %s  ti=%p in %p\n", NsfGlobalStrings[name], ti, interp);*/
  cmd = Tcl_GetCommandFromObj(interp, NsfGlobalObjs[name]);
  if (cmd != NULL) {
    Tcl_Command_objProc(cmd) = ti->proc;
    ti->proc = NULL;
  } else {
    result = TCL_ERROR;
  }
  
  return result;
}

static void
NsfReplaceCommandCheck(Tcl_Interp *interp, NsfGlobalNames name, Tcl_ObjCmdProc *proc) {
  Tcl_Command cmd;
  NsfShadowTclCommandInfo *ti = &RUNTIME_STATE(interp)->tclCommands[name-NSF_EXPR];
  cmd = Tcl_GetCommandFromObj(interp, NsfGlobalObjs[name]);
  
  if (cmd != NULL && ti->proc && Tcl_Command_objProc(cmd) != proc) {
    /*
    fprintf(stderr, "we have to do something about %s %p %p\n",
	    NsfGlobalStrings[name], Tcl_Command_objProc(cmd), proc);
    */
    ti->proc = Tcl_Command_objProc(cmd);
    ti->clientData = Tcl_Command_objClientData(cmd);
    Tcl_Command_objProc(cmd) = proc;
  }
}

static int
NsfReplaceCommand(Tcl_Interp *interp, NsfGlobalNames name,
		    Tcl_ObjCmdProc *nsfReplacementProc, int pass) {
  Tcl_Command cmd;
  NsfShadowTclCommandInfo *ti = &RUNTIME_STATE(interp)->tclCommands[name-NSF_EXPR];
  int result = TCL_OK;

  /*fprintf(stderr,"NsfReplaceCommand %d\n",name);*/
  cmd = Tcl_GetCommandFromObj(interp, NsfGlobalObjs[name]);
  
  if (cmd == NULL) {
    result = TCL_ERROR;
  } else {
    Tcl_ObjCmdProc *objProc = Tcl_Command_objProc(cmd);
    if (nsfReplacementProc != objProc) {
      if (pass == 0) { /* setting values on first pass (must be locked here) */
	ti->proc = objProc;
	ti->clientData = Tcl_Command_objClientData(cmd);
      } else if (ti->proc != objProc) {
	/*fprintf(stderr, "we have to refetch command for %s\n",NsfGlobalStrings[name]);*/
	ti->proc = objProc;
	ti->clientData = Tcl_Command_objClientData(cmd);
      }
      if (nsfReplacementProc) {
	Tcl_Command_objProc(cmd) = nsfReplacementProc;
	/*Tcl_CreateObjCommand(interp, NsfGlobalStrings[name], nsfReplacementProc, 0, 0);*/
      }
    }
  }
  return result;
}

static int
Nsf_RenameObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  /* this call the Tcl_RenameObjCmd, but it ensures before that
     the renamed obj, functions, etc. are not part of XOTcl */
  Tcl_Command cmd;

  /* wrong # args => normal Tcl ErrMsg*/
  if (objc != 3) {
    return NsfCallCommand(interp, NSF_RENAME, objc, objv);
  }

  /* if an obj/cl should be renamed => call the XOTcl move method */
  cmd = Tcl_FindCommand(interp, ObjStr(objv[1]), (Tcl_Namespace *)NULL,0);
  if (cmd) {
    NsfObject *object = NsfGetObjectFromCmdPtr(cmd);
    Tcl_Obj *methodObj = object ? NsfMethodObj(interp, object, NSF_o_move_idx) : NULL;
    if (object && methodObj) {
      return NsfCallMethodWithArgs((ClientData)object, interp,
                                     methodObj, objv[2], 1, 0, NSF_CSC_IMMEDIATE);
    }
  }

  /* Actually rename the cmd using Tcl's rename*/
  return NsfCallCommand(interp, NSF_RENAME, objc, objv);
}

static int
Nsf_InfoFrameObjCmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  int result;

  result = NsfCallCommand(interp, NSF_INFO_FRAME, objc, objv);

  if (result == TCL_OK && objc == 2) {
    int level, topLevel, frameFlags;
    CONST char *frameType;
    CmdFrame *framePtr = Tcl_Interp_cmdFramePtr(interp);
    CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
    Tcl_Obj *resultObj = Tcl_GetObjResult(interp);

    /* level must be ok, otherwise we would not have a TCL_OK */
    Tcl_GetIntFromObj(interp, objv[1], &level);

    /* todo: coroutine level messing is missing */
    topLevel = (framePtr == NULL) ? 0 :  framePtr->level;

    if (level > 0) {
      level -= topLevel;
    }

    while (++level <= 0 && varFramePtr) {
      framePtr = framePtr->nextPtr;
      varFramePtr = varFramePtr->callerPtr;
    }

    frameFlags = varFramePtr ? Tcl_CallFrame_isProcCallFrame(varFramePtr) : 0;
    /*fprintf(stderr, " ... frame %p varFramePtr %p frameFlags %.6x\n", framePtr, varFramePtr, frameFlags);
      Tcl85showStack(interp);*/
    if (frameFlags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      NsfCallStackContent *cscPtr = 
        ((NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr));
      Tcl_Obj *listObj, **ov;
      int oc, i;

      listObj = Tcl_NewListObj(0, NULL);
      /* remove "proc" element from list, if provided */
      Tcl_ListObjGetElements(interp, resultObj, &oc, &ov);
      for (i=0; i<oc; i += 2) {
	if (!strcmp(ObjStr(ov[i]), "proc")) {
	  continue;
	}
	Tcl_ListObjAppendElement(interp, listObj, ov[i]);
	Tcl_ListObjAppendElement(interp, listObj, ov[i+1]);
      }

      Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("object",6));
      Tcl_ListObjAppendElement(interp, listObj, cscPtr->self->cmdName);
      Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("class",5));
      Tcl_ListObjAppendElement(interp, listObj, cscPtr->cl 
			       ? cscPtr->cl->object.cmdName 
			       : NsfGlobalObjs[NSF_EMPTY]);
      Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("method",6));
      Tcl_ListObjAppendElement(interp, listObj, cscPtr->cmdPtr 
			       ? Tcl_NewStringObj(Tcl_GetCommandName(interp, cscPtr->cmdPtr), -1)
			       : NsfGlobalObjs[NSF_EMPTY]);
      Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj("frametype",9));
      if (cscPtr->frameType == NSF_CSC_TYPE_PLAIN) {
        frameType = "intrinsic";
      } else if (cscPtr->frameType & NSF_CSC_TYPE_ACTIVE_MIXIN) {
        frameType = "mixin";
      } else if (cscPtr->frameType & NSF_CSC_TYPE_ACTIVE_FILTER) {
        frameType = "filter";
      } else if (cscPtr->frameType & NSF_CSC_TYPE_GUARD) {
        frameType = "guard";
      } else {
        frameType = "unknown";
      }
      Tcl_ListObjAppendElement(interp, listObj, Tcl_NewStringObj(frameType,-1));
      Tcl_SetObjResult(interp, listObj);
    } else if (frameFlags & (FRAME_IS_NSF_OBJECT)) {
      NsfObject *object = (NsfObject *)Tcl_CallFrame_clientData(varFramePtr);
      //Tcl_Obj *listObj = Tcl_NewListObj(0, NULL);

      Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("object",6));
      Tcl_ListObjAppendElement(interp, resultObj, object->cmdName);
      Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("frameType",9));
      Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj("object",6));
      Tcl_SetObjResult(interp, resultObj);
    }
  }

  return result;
}

/*
 * Obtain the names of the tcl commands
 * not available through the stub interface and overload some global commands
 */
int
NsfShadowTclCommands(Tcl_Interp *interp, NsfShadowOperations load) {
  int rc = TCL_OK;
  if (load == SHADOW_LOAD) {
    int initialized = (RUNTIME_STATE(interp)->tclCommands != NULL);
    assert(initialized == 0);
    RUNTIME_STATE(interp)->tclCommands = 
      NEW_ARRAY(NsfShadowTclCommandInfo, NSF_SUBST - NSF_EXPR + 1);

    /*fprintf(stderr, "+++ load tcl commands %d %d\n", load, initialized);*/

#ifdef USE_TCL_STUBS
    /* no commands are overloaded, these are only used for calling 
       e.g. Tcl_ExprObjCmd(), Tcl_IncrObjCmd() and Tcl_SubstObjCmd(), 
       which are not available in though the stub table */
    rc |= NsfReplaceCommand(interp, NSF_EXPR,       NULL, initialized);
    rc |= NsfReplaceCommand(interp, NSF_SUBST,      NULL, initialized);
#endif
    rc |= NsfReplaceCommand(interp, NSF_FORMAT,     NULL, initialized);
    rc |= NsfReplaceCommand(interp, NSF_INTERP,     NULL, initialized);
    rc |= NsfReplaceCommand(interp, NSF_IS,         NULL, initialized);

    /* for the following commands, we have to add our own semantics */
    rc |= NsfReplaceCommand(interp, NSF_INFO_FRAME, Nsf_InfoFrameObjCmd, initialized);
    rc |= NsfReplaceCommand(interp, NSF_RENAME,     Nsf_RenameObjCmd, initialized);
  } else if (load == SHADOW_REFETCH) {
    NsfReplaceCommandCheck(interp, NSF_INFO_FRAME, Nsf_InfoFrameObjCmd);
    NsfReplaceCommandCheck(interp, NSF_RENAME,     Nsf_RenameObjCmd);
  } else {
    NsfReplaceCommandCleanup(interp, NSF_INFO_FRAME);
    NsfReplaceCommandCleanup(interp, NSF_RENAME);

    FREE(NsfShadowTclCommandInfo*, RUNTIME_STATE(interp)->tclCommands);
    RUNTIME_STATE(interp)->tclCommands = NULL;
  }
  return rc;
}

/*
 * call a Tcl command with given objv's ... replace objv[0]
 * with the given command name
 */
int NsfCallCommand(Tcl_Interp *interp, NsfGlobalNames name,
	    int objc, Tcl_Obj *CONST objv[]) {
  int result;
  NsfShadowTclCommandInfo *ti = &RUNTIME_STATE(interp)->tclCommands[name-NSF_EXPR];
  ALLOC_ON_STACK(Tcl_Obj*,objc, ov);
  /*
   {int i;
    fprintf(stderr,"calling %s (%p %p) in %p, objc=%d ",
	    NsfGlobalStrings[name],ti,ti->proc, interp, objc);
            for(i=0;i<objc;i++){fprintf(stderr, "'%s' ", ObjStr(objv[i]));}
    fprintf(stderr,"\n");
  } 
  */
  ov[0] = NsfGlobalObjs[name];
  if (objc > 1)
    memcpy(ov+1, objv+1, sizeof(Tcl_Obj *)*(objc-1));
  result = Tcl_NRCallObjProc(interp, ti->proc, ti->clientData, objc, objv);
  FREE_ON_STACK(Tcl_Obj *, ov);
  return result;
}
