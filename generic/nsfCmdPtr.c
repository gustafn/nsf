/*
 * Conversion from CmdPtr to Class / Object
 */

static NSF_INLINE NsfObject*NsfGetObjectFromCmdPtr(Tcl_Command cmd) nonnull(1);
static NSF_INLINE NsfClass*NsfGetClassFromCmdPtr(Tcl_Command cmd) nonnull(1);
static NSF_INLINE ClientData NsfGetClientDataFromCmdPtr(Tcl_Command cmd) nonnull(1);

static NSF_INLINE ClientData
NsfGetClientDataFromCmdPtr(Tcl_Command cmd) {
  assert(cmd != NULL);
  /*fprintf(stderr, "objProc=%p %p\n", Tcl_Command_objProc(cmd),NsfObjDispatch);*/
  if (likely(Tcl_Command_objProc(cmd) == NsfObjDispatch))
    return Tcl_Command_objClientData(cmd);
  else {
    cmd = TclGetOriginalCommand(cmd);
    if (likely(cmd != NULL) && unlikely(Tcl_Command_objProc(cmd) == NsfObjDispatch)) {
      /*fprintf(stderr, "???? got cmd right in 2nd round\n");*/
      return Tcl_Command_objClientData(cmd);
    }
    return NULL;
  }
}

static NSF_INLINE NsfClass*
NsfGetClassFromCmdPtr(Tcl_Command cmd) {
  ClientData cd = NsfGetClientDataFromCmdPtr(cmd);
  assert(cmd != NULL);
  /*fprintf(stderr, "cd=%p\n",cd);*/
  if (likely(cd != NULL)) {
    return NsfObjectToClass(cd);
  } else {
    return NULL;
  }
}

static NSF_INLINE NsfObject*
NsfGetObjectFromCmdPtr(Tcl_Command cmd) {
  assert(cmd != NULL);
  return (NsfObject*) NsfGetClientDataFromCmdPtr(cmd);
}
