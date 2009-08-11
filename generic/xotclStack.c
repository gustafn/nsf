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

XOTCLINLINE static XOTclCallStackContent*
CallStackGetTopFrame(Tcl_Interp *interp) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  return cs->top;
}

XOTclCallStackContent *
XOTclCallStackFindLastInvocation(Tcl_Interp *interp, int offset) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  register XOTclCallStackContent *csc = cs->top;
  int topLevel = csc->currentFramePtr ? Tcl_CallFrame_level(csc->currentFramePtr) : 0;
  int deeper = offset;

  /* skip through toplevel inactive filters, do this offset times */
  for (csc=cs->top; csc > cs->content; csc--) {
    /* fprintf(stderr, "csc %p callType = %x, frameType = %x, offset=%d\n",
       csc,csc->callType,csc->frameType,offset); */
    if ((csc->callType & XOTCL_CSC_CALL_IS_NEXT) ||
        (csc->frameType & XOTCL_CSC_TYPE_INACTIVE))
      continue;
    if (offset)
      offset--;
    else {
      /* fprintf(stderr, "csc %p offset ok, deeper=%d\n",csc,deeper); */
      if (!deeper || cs->top->callType & XOTCL_CSC_CALL_IS_GUARD) {
        return csc;
      }
      if (csc->currentFramePtr && Tcl_CallFrame_level(csc->currentFramePtr) < topLevel) {
        return csc;
      }
    }
  }
  /* for some reasons, we could not find invocation (topLevel, destroy) */
  /* fprintf(stderr, "csc %p could not find invocation\n",csc);*/
  return NULL;
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

static XOTclCallStackContent* 
CallStackGetObjectFrame(Tcl_Interp *interp, XOTclObject *obj) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  XOTclCallStackContent *csc = CallStackGetTopFrame(interp);

  for (; csc >= cs->content; csc--) {
    if (csc->self == obj) {
      return csc;
    }
  }
  return NULL;
}

static int
CallStackMarkDestroyed(Tcl_Interp *interp, XOTclObject *obj) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  XOTclCallStackContent *csc;
  int countSelfs = 0;
  Tcl_Command oid = obj->id;

  for (csc = &cs->content[1]; csc <= cs->top; csc++) {
    if (csc->self == obj) {
      csc->destroyedCmd = oid;
      csc->callType |= XOTCL_CSC_CALL_IS_DESTROY;
      /*fprintf(stderr,"setting destroy on csc %p for obj %p\n", csc, obj);*/
      if (csc->destroyedCmd) {
        Tcl_Command_refCount(csc->destroyedCmd)++;
        MEM_COUNT_ALLOC("command refCount", csc->destroyedCmd);
      }
      countSelfs++;
    }
  }
  return countSelfs;
}
#endif /* NOT TCL85STACK */


