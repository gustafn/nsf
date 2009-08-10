#if defined(TCL85STACK)

XOTCLINLINE static XOTclObject*
GetSelfObj(Tcl_Interp *interp) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  
  /*fprintf(stderr, "GetSelfObj interp has frame %p and varframe %p\n",
    Tcl_Interp_framePtr(interp),Tcl_Interp_varFramePtr(interp));*/
  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
#if defined(TCL85STACK_TRACE)
    fprintf(stderr, "GetSelfObj check frame %p flags %.6x cd %p objv[0] %s\n",
            varFramePtr, Tcl_CallFrame_isProcCallFrame(varFramePtr), 
            Tcl_CallFrame_clientData(varFramePtr),
            Tcl_CallFrame_objc(varFramePtr) ? ObjStr(Tcl_CallFrame_objv(varFramePtr)[0]) : "(null)");
#endif
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_XOTCL_OBJECT) {
#if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... self returns %s\n",
              objectName(((XOTclObject*)Tcl_CallFrame_clientData(varFramePtr))));
#endif
      return (XOTclObject *)Tcl_CallFrame_clientData(varFramePtr);
    }
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_XOTCL_METHOD) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
#if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... self returns %s\n",objectName(csc->self));
#endif
      return csc->self;
    }
  }
  return NULL;
}


static XOTclCallStackContent*
CallStackGetFrame(Tcl_Interp *interp) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
# if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... check frame %p flags %.6x cd %p objv[0] %s\n",
              varFramePtr, Tcl_CallFrame_isProcCallFrame(varFramePtr), 
              Tcl_CallFrame_clientData(varFramePtr),
              Tcl_CallFrame_objc(varFramePtr) ? ObjStr(Tcl_CallFrame_objv(varFramePtr)[0]) : "(null)");
# endif
      if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_XOTCL_METHOD) {
        return (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      }
  }
  return NULL;
}

static void 
CallStackClearCmdReferences(Tcl_Interp *interp, Tcl_Command cmd) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_XOTCL_METHOD) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (csc->cmdPtr == cmd) {
        csc->cmdPtr = NULL;
      }
    }
  }
}

/* 
   TODO: we have a small divergence in the test "filterGuards" due to
   different lifetime of stack entries, so we keep for reference and
   for potential mor digging the following function, which can be used
   in xotcl.c in CallStackDestroyObject() like

     int marked = CallStackMarkDestroyed(interp, obj);
     int mm2 = CallStackMarkDestroyed84dummy(interp, obj);

     fprintf(stderr, "84 => %d marked, 85 => %d marked, ok = %d\n",marked, m2, marked == m2);
     if (marked != m2) {
        tcl85showStack(interp);
     }
*/
static int
CallStackMarkDestroyed84dummy(Tcl_Interp *interp, XOTclObject *obj) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  XOTclCallStackContent *csc;
  int countSelfs = 0;
  Tcl_Command oid = obj->id;

  for (csc = &cs->content[1]; csc <= cs->top; csc++) {
    if (csc->self == obj) {
      /*csc->destroyedCmd = oid;
        csc->callType |= XOTCL_CSC_CALL_IS_DESTROY;*/
      fprintf(stderr,"84 setting destroy on csc %p for obj %p\n", csc, obj);
      if (oid) {
        /*Tcl_Command_refCount(csc->destroyedCmd)++;*/
        MEM_COUNT_ALLOC("command refCount", csc->destroyedCmd);
      }
      countSelfs++;
    }
  }
  return countSelfs;
}

static int
CallStackMarkDestroyed(Tcl_Interp *interp, XOTclObject *obj) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  int marked = 0;
  Tcl_Command oid = obj->id;

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_XOTCL_METHOD) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (csc->self == obj) {
        csc->destroyedCmd = oid;
        csc->callType |= XOTCL_CSC_CALL_IS_DESTROY;
        /*fprintf(stderr,"setting destroy on csc %p for obj %p\n", csc, obj);*/
        if (csc->destroyedCmd) {
          Tcl_Command_refCount(csc->destroyedCmd)++;
          MEM_COUNT_ALLOC("command refCount", csc->destroyedCmd);
        }
        marked++;
      }
    }
  }
  return marked;
}

#endif /* TCL85STACK */


