#if !defined(TCL85STACK)

XOTCLINLINE static XOTclObject*
GetSelfObj(Tcl_Interp *interp) {
  return CallStackGetFrame(interp)->self;
}

static XOTclCallStackContent*
CallStackGetFrame(Tcl_Interp *interp) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  register XOTclCallStackContent *top = cs->top;
  Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  
  /*fprintf(stderr, "framePtr %p != varFramePtr %p && top->currentFramePtr %p => %d\n", 
          Tcl_Interp_framePtr(interp), varFramePtr, top->currentFramePtr,
          (Tcl_Interp_framePtr(interp) != varFramePtr && top->currentFramePtr)
          );*/

  if (Tcl_Interp_framePtr(interp) != varFramePtr && top->currentFramePtr) {
    XOTclCallStackContent *bot = cs->content + 1;

    /* we are in a uplevel */
    while (varFramePtr != top->currentFramePtr && top>bot) {
      top--;
    }
  }

  return top;
}

static void 
CallStackClearCmdReferences(Tcl_Interp *interp, Tcl_Command cmd) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  XOTclCallStackContent *csc = cs->top;

  for (; csc > cs->content; csc--) {
    if (csc->cmdPtr == cmd) {
      csc->cmdPtr = NULL;
    }
  }
}
#endif /* TCL85STACK */


