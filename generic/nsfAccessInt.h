#define Tcl_Interp_numLevels(interp)       ((Interp *)interp)->numLevels
#define Tcl_Interp_framePtr(interp)        ((Tcl_CallFrame *)((Interp *)interp)->framePtr)
#define Tcl_Interp_varFramePtr(interp)     (((Interp *)interp)->varFramePtr)
#define Tcl_Interp_cmdFramePtr(interp)     (((Interp *)interp)->cmdFramePtr)
#define Tcl_Interp_globalNsPtr(interp)     ((Tcl_Namespace *)((Interp *)interp)->globalNsPtr)
#define Tcl_Interp_flags(interp)           ((Interp *)interp)->flags
#define Tcl_Interp_threadId(interp)        ((Interp *)interp)->threadId

#define Tcl_CallFrame_callerPtr(cf)       ((Tcl_CallFrame*)((CallFrame *)cf)->callerPtr)
#define Tcl_CallFrame_procPtr(cf)         ((CallFrame *)cf)->procPtr
#define Tcl_CallFrame_varTablePtr(cf)     ((CallFrame *)cf)->varTablePtr
#define Tcl_CallFrame_level(cf)           ((CallFrame *)cf)->level
#define Tcl_CallFrame_isProcCallFrame(cf) ((CallFrame *)cf)->isProcCallFrame
#define Tcl_CallFrame_compiledLocals(cf)  ((CallFrame *)cf)->compiledLocals
#define Tcl_CallFrame_numCompiledLocals(cf)  ((CallFrame *)cf)->numCompiledLocals
#define Tcl_CallFrame_callerVarPtr(cf)    ((Tcl_CallFrame*)((CallFrame *)cf)->callerVarPtr)
#define Tcl_CallFrame_objc(cf)            ((CallFrame *)cf)->objc
#define Tcl_CallFrame_objv(cf)            ((CallFrame *)cf)->objv
#define Tcl_CallFrame_clientData(cf)      ((CallFrame *)cf)->clientData
#define Tcl_CallFrame_nsPtr(cf)           ((Tcl_Namespace *)((CallFrame *)cf)->nsPtr)

#define Tcl_Namespace_cmdTablePtr(nsPtr) &((Namespace *)nsPtr)->cmdTable
#define Tcl_Namespace_varTablePtr(nsPtr) &((Namespace *)nsPtr)->varTable
#define Tcl_Namespace_childTablePtr(nsPtr) &((Namespace *)nsPtr)->childTable
#define Tcl_Namespace_activationCount(nsPtr) ((Namespace *)nsPtr)->activationCount
#define Tcl_Namespace_deleteProc(nsPtr)  ((Namespace *)nsPtr)->deleteProc
#define Tcl_Namespace_parentPtr(nsPtr)   ((Namespace *)nsPtr)->parentPtr
#define Tcl_Namespace_commandPathLength(nsPtr) ((Namespace *)nsPtr)->commandPathLength
#define Tcl_Namespace_commandPathArray(nsPtr)  ((Namespace *)nsPtr)->commandPathArray


#define Tcl_Command_refCount(cmd)      ((Command *)cmd)->refCount
#define Tcl_Command_cmdEpoch(cmd)      ((Command *)cmd)->cmdEpoch
#define Tcl_Command_flags(cmd)         ((Command *)cmd)->flags
/* the following items could be obtained from 
   Tcl_GetCommandInfoFromToken(cmd, infoPtr) */
#define Tcl_Command_nsPtr(cmd)         ((Tcl_Namespace*)(((Command *)cmd)->nsPtr))
#define Tcl_Command_objProc(cmd)       ((Command *)cmd)->objProc
#define Tcl_Command_objClientData(cmd) ((Command *)cmd)->objClientData
#define Tcl_Command_proc(cmd)          ((Command *)cmd)->proc
#define Tcl_Command_clientData(cmd)    ((Command *)cmd)->clientData
#define Tcl_Command_deleteProc(cmd)    ((Command *)cmd)->deleteProc
#define Tcl_Command_deleteData(cmd)    ((Command *)cmd)->deleteData

/*
 * Conversion from CmdPtr to Class / Object
 */

static NSF_INLINE ClientData
NsfGetClientDataFromCmdPtr(Tcl_Command cmd) {
  assert(cmd);
  /*fprintf(stderr, "objProc=%p %p\n",Tcl_Command_objProc(cmd),NsfObjDispatch);*/
  if (Tcl_Command_objProc(cmd) == NsfObjDispatch /* && !Tcl_Command_cmdEpoch(cmd)*/)
    return Tcl_Command_objClientData(cmd);
  else {
    cmd = TclGetOriginalCommand(cmd);
    if (cmd && Tcl_Command_objProc(cmd) == NsfObjDispatch) {
      /*fprintf(stderr, "???? got cmd right in 2nd round\n");*/
      return Tcl_Command_objClientData(cmd);
    }
    return NULL;
  }
}

static NSF_INLINE NsfClass*
NsfGetClassFromCmdPtr(Tcl_Command cmd) {
  ClientData cd = NsfGetClientDataFromCmdPtr(cmd);
  /*fprintf(stderr, "cd=%p\n",cd);*/
  if (cd) 
    return NsfObjectToClass(cd);
  else
    return 0;
}

static NSF_INLINE NsfObject*
NsfGetObjectFromCmdPtr(Tcl_Command cmd) {
  return (NsfObject*) NsfGetClientDataFromCmdPtr(cmd);
}


