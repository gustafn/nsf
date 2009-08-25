#if !defined(TCL85STACK)

XOTCLINLINE static XOTclCallStackContent *
CallStackPush(Tcl_Interp *interp, XOTclObject *obj, XOTclClass *cl, Tcl_Command cmd, int frameType) {
  XOTclCallStack *cs;
  register XOTclCallStackContent *csc;

  cs = &RUNTIME_STATE(interp)->cs;
  if (cs->top >= &cs->content[MAX_NESTING_DEPTH-1]) {
    Tcl_SetResult(interp, "too many nested calls to Tcl_EvalObj (infinite loop?)",
                  TCL_STATIC);
    return NULL;
  }
  obj->activationCount ++;
  csc = ++cs->top;
  csc->self          = obj;
  csc->cl            = cl;
  csc->cmdPtr        = cmd;
  csc->frameType     = frameType;
  csc->callType      = 0;
#if !defined(TCL85STACK)
  csc->currentFramePtr = NULL; /* this will be set by InitProcNSCmd */
#endif
  csc->filterStackEntry = frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER ? obj->filterStack : NULL;

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "PUSH csc %p type %d frame %p, obj %s, self=%p cmd=%p (%s) id=%p (%s)\n",
          csc, frameType, Tcl_Interp_framePtr(interp), objectName(obj), obj,
    cmd, (char *) Tcl_GetCommandName(interp, cmd),
          obj->id, Tcl_GetCommandName(interp, obj->id));
#endif

  MEM_COUNT_ALLOC("CallStack", NULL);
  return csc;
}

XOTCLINLINE static void
CallStackPop(Tcl_Interp *interp, XOTclCallStackContent *cscPtr) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  XOTclCallStackContent *csc = cs->top;
  XOTclObject *obj = csc->self;

  assert(cs->top > cs->content);

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "POP  csc=%p, frame %p\n", csc, Tcl_Interp_framePtr(interp));
#endif
  obj->activationCount --;

  if (obj->activationCount < 1 && obj->flags & XOTCL_DESTROY_CALLED) {
    CallStackDoDestroy(interp, obj);
  }

  cs->top--;
  MEM_COUNT_FREE("CallStack", NULL);
}

Tcl_CallFrame * nonXotclObjectProcFrame(Tcl_CallFrame *framePtr) {return framePtr;}

XOTCLINLINE static XOTclObject*
GetSelfObj(Tcl_Interp *interp) {
  return CallStackGetFrame(interp, NULL)->self;
}

static XOTclCallStackContent*
CallStackGetFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
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
  if (framePtrPtr) *framePtrPtr = top->currentFramePtr;
  return top;
}

XOTCLINLINE static XOTclCallStackContent*
CallStackGetTopFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  if (framePtrPtr) *framePtrPtr = cs->top->currentFramePtr;
  return cs->top;
}

XOTclCallStackContent *
XOTclCallStackFindLastInvocation(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
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
        if (framePtrPtr) *framePtrPtr = csc->currentFramePtr;
        return csc;
      }
      if (csc->currentFramePtr && Tcl_CallFrame_level(csc->currentFramePtr) < topLevel) {
        if (framePtrPtr) *framePtrPtr = csc->currentFramePtr;
        return csc;
      }
    }
  }
  /* for some reasons, we could not find invocation (topLevel, destroy) */
  /* fprintf(stderr, "csc %p could not find invocation\n",csc);*/
  if (framePtrPtr) *framePtrPtr = NULL;
  return NULL;
}

XOTclCallStackContent *
XOTclCallStackFindActiveFrame(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  register XOTclCallStackContent *csc;

  /* search for first active frame and set tcl frame pointers */
  for (csc=cs->top-offset; csc > cs->content; csc --) {
    if (!(csc->frameType & XOTCL_CSC_TYPE_INACTIVE)) {
      /* we found the highest active frame */
      if (framePtrPtr) *framePtrPtr = csc->currentFramePtr;
      return csc;
    }
  }
  /* we could not find an active frame; called from toplevel? */
  if (framePtrPtr) *framePtrPtr = NULL;
  return NULL;
}

static void
CallStackUseActiveFrames(Tcl_Interp *interp, callFrameContext *ctx) {
  XOTclCallStackContent *active, *top;
  Tcl_CallFrame *inFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp), 
    *varFramePtr, *activeFramePtr, *framePtr;

  active = XOTclCallStackFindActiveFrame(interp, 0, &activeFramePtr);
  top = CallStackGetTopFrame(interp, NULL);
  varFramePtr = inFramePtr;

  /*fprintf(stderr,"CallStackUseActiveFrames inframe %p varFrame %p activeFrame %p lvl %d\n",
    inFramePtr,varFramePtr,activeFramePtr, Tcl_CallFrame_level(inFramePtr));*/

  if (activeFramePtr == varFramePtr || active == top || Tcl_CallFrame_level(inFramePtr) == 0) {
    /* top frame is a active frame, or we could not find a calling frame */
    framePtr = varFramePtr;

  } else if (active == NULL) {
    /* There is no xotcl callframe active; use the caller of inframe */
    fprintf(stderr,"active == NULL\n");
    for (framePtr = inFramePtr; framePtr && Tcl_CallFrame_level(framePtr); framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
      if (framePtr != top->currentFramePtr)
        break;
    }
  } else {
    /* The active framePtr is an entry deeper in the stack. When XOTcl
       is interleaved with Tcl, we return the Tcl frame */

    /* fprintf(stderr,"active == deeper, use Tcl frame\n"); */
    if ((framePtr = (active+1)->currentFramePtr)) {
      framePtr = Tcl_CallFrame_callerPtr(framePtr);
    } else {
      framePtr = active->currentFramePtr;
    }
  }
  if (inFramePtr == framePtr) {
    /* call frame pointers are fine */
    /*fprintf(stderr, "... no need to save frames\n");*/
    ctx->framesSaved = 0;
  } else {
    ctx->varFramePtr = inFramePtr;
    Tcl_Interp_varFramePtr(interp) = (CallFrame *)framePtr;
    ctx->framesSaved = 1;
  }
}

static XOTclCallStackContent *
CallStackFindActiveFilter(Tcl_Interp *interp) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  register XOTclCallStackContent *csc;

  /* search for first active frame and set tcl frame pointers */
  for (csc=cs->top; csc > cs->content; csc --) {
    if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) return csc;
  }
  /* for some reasons, we could not find invocation (topLevel, destroy) */
  return NULL;
}

/*
 * check, if there is an active filters on "obj" using cmd
 */
XOTCLINLINE static int
FilterActiveOnObj(Tcl_Interp *interp, XOTclObject *obj, Tcl_Command cmd) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;
  XOTclCallStackContent *bot = cs->content;
  register XOTclCallStackContent *csc = cs->top;
  while (csc > bot) {
    /* only check the callstack entries for this object &&
       only check the callstack entries for the given cmd */
    if (obj == csc->self && cmd == csc->cmdPtr &&
        csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
      return 1;
    }
    csc--;
  }
  return 0;
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
  XOTclCallStackContent *csc = CallStackGetTopFrame(interp, NULL);

  for (; csc >= cs->content; csc--) {
    if (csc->self == obj) {
      return csc;
    }
  }
  return NULL;
}

/*
 * Pop any callstack entry that is still alive (e.g.
 * if "exit" is called and we were jumping out of the
 * callframe
 */
void CallStackPopAll(Tcl_Interp *interp) {
  XOTclCallStack *cs = &RUNTIME_STATE(interp)->cs;

  while (cs->top > cs->content)
    CallStackPop(interp, NULL);

  while (1) {
    Tcl_CallFrame *framePtr = Tcl_Interp_framePtr(interp);
    if (!framePtr) break;
    if (Tcl_CallFrame_level(framePtr) == 0) break;
    Tcl_PopCallFrame(interp);
  }
}
#endif /* NOT TCL85STACK */
