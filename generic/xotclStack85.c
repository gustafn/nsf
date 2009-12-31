/* TODO final touch: unify names (check tcl naming convention), make functions here static */

#if defined(TCL85STACK)

void tcl85showStack(Tcl_Interp *interp) {
  Tcl_CallFrame *framePtr;
  fprintf(stderr, "tcl85showStack framePtr %p varFramePtr %p\n",
          Tcl_Interp_framePtr(interp), Tcl_Interp_varFramePtr(interp));
  /* framePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
    for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    fprintf(stderr, "... frame %p flags %.6x cd %p objv[0] %s\n",
            framePtr, Tcl_CallFrame_isProcCallFrame(framePtr),
            Tcl_CallFrame_clientData(framePtr),
            Tcl_CallFrame_objc(framePtr) ? ObjStr(Tcl_CallFrame_objv(framePtr)[0]) : "(null)");
            }*/
  framePtr = (Tcl_CallFrame *)Tcl_Interp_framePtr(interp);
  for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    XOTclCallStackContent *csc = Tcl_CallFrame_isProcCallFrame(framePtr)
      & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD) ?
      ((XOTclCallStackContent *)Tcl_CallFrame_clientData(framePtr)) : NULL;

    fprintf(stderr, "... var frame %p flags %.6x cd %.8x lvl %d frameType %d ns %p %s, %p %s %s\n",
            framePtr, Tcl_CallFrame_isProcCallFrame(framePtr),
            (int)Tcl_CallFrame_clientData(framePtr),
            Tcl_CallFrame_level(framePtr),
            csc ? csc->frameType : -1,
            Tcl_CallFrame_nsPtr(framePtr), Tcl_CallFrame_nsPtr(framePtr)->fullName,
            csc ? csc->self : NULL, csc ? objectName(csc->self) : "",
            Tcl_CallFrame_objc(framePtr) ? ObjStr(Tcl_CallFrame_objv(framePtr)[0]) : "(null)");
  }
}

static Tcl_CallFrame *
nonXotclObjectProcFrame(Tcl_CallFrame *framePtr) {
  for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    register int flag = Tcl_CallFrame_isProcCallFrame(framePtr);
    if (flag & FRAME_IS_XOTCL_METHOD) {
      /* never return an inactive method frame */
      if (!(((XOTclCallStackContent *)Tcl_CallFrame_clientData(framePtr))->frameType & XOTCL_CSC_TYPE_INACTIVE)) break;
    } else {
      if (flag & FRAME_IS_XOTCL_OBJECT) continue;
      /*if ((flag & (FRAME_IS_XOTCL_OBJECT|FRAME_IS_XOTCL_CMETHOD)) == 0) break;*/
      if (flag == 0 || flag & FRAME_IS_PROC) break;
    }
  }
  return framePtr;
}

static Tcl_CallFrame *
nextFrameOfType(Tcl_CallFrame *framePtr, int flags) {
  for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(framePtr) & flags)
      return framePtr;
  }
  return framePtr;
}

XOTCLINLINE static XOTclObject*
GetSelfObj(Tcl_Interp *interp) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  /*fprintf(stderr, "GetSelfObj interp has frame %p and varframe %p\n",
    Tcl_Interp_framePtr(interp),Tcl_Interp_varFramePtr(interp));*/
  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    register int flag = Tcl_CallFrame_isProcCallFrame(varFramePtr);
#if defined(TCL85STACK_TRACE)
    fprintf(stderr, "GetSelfObj check frame %p flags %.6x cd %p objv[0] %s\n",
            varFramePtr, Tcl_CallFrame_isProcCallFrame(varFramePtr),
            Tcl_CallFrame_clientData(varFramePtr),
            Tcl_CallFrame_objc(varFramePtr) ? ObjStr(Tcl_CallFrame_objv(varFramePtr)[0]) : "(null)");
#endif
    if (flag & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
#if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... self returns %p %.6x %s\n",csc->self, 
              csc->self->flags, objectName(csc->self));
#endif
      return csc->self;
    } else if (flag & FRAME_IS_XOTCL_OBJECT) {
#if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... self returns %s\n",
              objectName(((XOTclObject*)Tcl_CallFrame_clientData(varFramePtr))));
#endif
      return (XOTclObject *)Tcl_CallFrame_clientData(varFramePtr);
    }
  }
  return NULL;
}

static XOTclCallStackContent*
CallStackGetFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
# if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... check frame %p flags %.6x cd %p objv[0] %s\n",
              varFramePtr, Tcl_CallFrame_isProcCallFrame(varFramePtr),
              Tcl_CallFrame_clientData(varFramePtr),
              Tcl_CallFrame_objc(varFramePtr) ? ObjStr(Tcl_CallFrame_objv(varFramePtr)[0]) : "(null)");
# endif
      if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
        if (framePtrPtr) *framePtrPtr = varFramePtr;
        return (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      }
  }
  if (framePtrPtr) *framePtrPtr = NULL;
  return NULL;
}

XOTCLINLINE static XOTclCallStackContent*
CallStackGetTopFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
  return CallStackGetFrame(interp, framePtrPtr);
}

XOTclCallStackContent *
XOTclCallStackFindLastInvocation(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  int lvl = Tcl_CallFrame_level(varFramePtr);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if ((csc->callType & XOTCL_CSC_CALL_IS_NEXT) || (csc->frameType & XOTCL_CSC_TYPE_INACTIVE)) {
        continue;
      }
      if (offset) {
        offset--;
      } else {
        if (Tcl_CallFrame_level(varFramePtr) < lvl) {
          if (framePtrPtr) *framePtrPtr = varFramePtr;
          return csc;
        }
      }
    }
  }
  if (framePtrPtr) *framePtrPtr = NULL;
  return NULL;
}

XOTclCallStackContent *
XOTclCallStackFindActiveFrame(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  /* skip #offset frames */
  for (; offset>0 && varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr), offset--);

  /* search for first active frame and set tcl frame pointers */
  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (!(csc->frameType & XOTCL_CSC_TYPE_INACTIVE)) {
        /* we found the highest active frame */
        if (framePtrPtr) *framePtrPtr = varFramePtr;
        return csc;
      }
    }
  }
  /* we could not find an active frame; called from toplevel? */
  if (framePtrPtr) *framePtrPtr = NULL;
  return NULL;
}

static void
CallStackUseActiveFrames(Tcl_Interp *interp, callFrameContext *ctx) {
  Tcl_CallFrame *inFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp),
    *varFramePtr, *activeFramePtr, *framePtr;

  XOTclCallStackFindActiveFrame(interp, 0, &activeFramePtr);
  /*tcl85showStack(interp);*/
# if defined(TCL85STACK_TRACE)
  tcl85showStack(interp);
# endif
  /* Get the first active non object frame (or object frame with proc */
  varFramePtr = nonXotclObjectProcFrame(inFramePtr);

  /*fprintf(stderr,"CallStackUseActiveFrames inframe %p varFrame %p activeFrame %p lvl %d\n",
    inFramePtr,varFramePtr,activeFramePtr, Tcl_CallFrame_level(inFramePtr));*/

  if (activeFramePtr == varFramePtr || activeFramePtr == inFramePtr) {
    /* top frame is a active frame */
    framePtr = varFramePtr;

  } else if (activeFramePtr == NULL) {
    /* There is no XOTcl callframe active; use the caller of inframe */
    /*fprintf(stderr,"activeFramePtr == NULL\n");*/

    if ((Tcl_CallFrame_isProcCallFrame(inFramePtr) & FRAME_IS_XOTCL_METHOD) == 0) {
      framePtr = varFramePtr;
    } else {
      framePtr = Tcl_CallFrame_callerPtr(inFramePtr);
    }

  } else {
    /* The active framePtr is an entry deeper in the stack. When XOTcl
       is interleaved with Tcl, we return the Tcl frame */

    /*fprintf(stderr,"active == deeper, use Tcl frame\n"); */
    for (framePtr = varFramePtr; framePtr && framePtr != activeFramePtr;
         framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
      if ((Tcl_CallFrame_isProcCallFrame(framePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) == 0) {
        break;
      }
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
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
        return csc;
      }
    }
  }
  /* for some reasons, we could not find invocation (topLevel, destroy) */
  return NULL;
}

/*
 * check, if there is an active filters on "obj" using cmd
 */
XOTCLINLINE static int
FilterActiveOnObj(Tcl_Interp *interp, XOTclObject *obj, Tcl_Command cmd) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (cmd == csc->cmdPtr && obj == csc->self &&
          csc->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
        return 1;
      }
    }
  }
  return 0;
}

static void
CallStackClearCmdReferences(Tcl_Interp *interp, Tcl_Command cmd) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (csc->cmdPtr == cmd) {
        csc->cmdPtr = NULL;
      }
    }
  }
}

static XOTclCallStackContent*
CallStackGetObjectFrame(Tcl_Interp *interp, XOTclObject *obj) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *csc = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (csc->self == obj) {
        return csc;
      }
    }
  }
  return NULL;
}

/*
 * Pop any callstack entry that is still alive (e.g.
 * if "exit" is called and we were jumping out of the
 * callframe
 */
static void CallStackPopAll(Tcl_Interp *interp) {

  while (1) {
    Tcl_CallFrame *framePtr = Tcl_Interp_framePtr(interp);
    if (!framePtr) break;
    if (Tcl_CallFrame_level(framePtr) == 0) break;

    if (Tcl_CallFrame_isProcCallFrame(framePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      /* free the call stack content; we need this just for decr activation count */
      XOTclCallStackContent *csc = ((XOTclCallStackContent *)Tcl_CallFrame_clientData(framePtr));
      CallStackPop(interp, csc);
    }
    /* pop the Tcl frame */
    Tcl_PopCallFrame(interp);
  }
}

XOTCLINLINE static void
CallStackPush(XOTclCallStackContent *csc, XOTclObject *obj, XOTclClass *cl, Tcl_Command cmd, int frameType) {
  obj->activationCount ++;
#if 1
  if (cl) {
    Namespace *nsPtr = ((Command *)cmd)->nsPtr;
    cl->object.activationCount ++;
    /*fprintf(stderr, "... %s cmd %s cmd ns %p (%s) obj ns %p parent %p\n", 
            className(cl), 
            Tcl_GetCommandName(obj->teardown, cmd),
            ((Command *)cmd)->nsPtr, ((Command *)cmd)->nsPtr->fullName,
            cl->object.nsPtr,cl->object.nsPtr ? ((Namespace*)cl->object.nsPtr)->parentPtr : NULL);*/
    
    /* incremement the namespace ptr in case tcl tries to delete this namespace 
       during the invocation */
    nsPtr->refCount ++;
  }
#endif
  /*fprintf(stderr, "incr activationCount for %s to %d\n", objectName(obj), obj->activationCount);*/
  csc->self          = obj;
  csc->cl            = cl;
  csc->cmdPtr        = cmd;
  csc->frameType     = frameType;
  csc->callType      = 0;
  csc->filterStackEntry = frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER ? obj->filterStack : NULL;

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "PUSH csc %p type %d obj %s, self=%p cmd=%p (%s) id=%p (%s) obj refcount %d name refcount %d\n",
          csc, frameType, objectName(obj), obj,
          cmd, (char *) Tcl_GetCommandName(obj->teardown, cmd),
          obj->id, obj->id ? Tcl_GetCommandName(obj->teardown, obj->id) : "(deleted)",
          obj->id ? Tcl_Command_refCount(obj->id) : -100, obj->cmdName->refCount
          );
#endif
}

XOTCLINLINE static void
CallStackPop(Tcl_Interp *interp, XOTclCallStackContent *csc) {
  XOTclObject *obj = csc->self;

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "POP  csc=%p, obj %s method %s (%d)\n", csc, objectName(obj),
          Tcl_GetCommandName(interp, csc->cmdPtr));
#endif
  obj->activationCount --;
  
  /*fprintf(stderr, "decr activationCount for %s to %d\n", objectName(csc->self), 
    csc->self->activationCount);*/

  if (obj->activationCount < 1 && obj->flags & XOTCL_DESTROY_CALLED) {
    CallStackDoDestroy(interp, obj);
  }
#if 1
  if (csc->cl) {
    Namespace *nsPtr = csc->cmdPtr ? ((Command *)(csc->cmdPtr))->nsPtr : NULL;

    obj = &csc->cl->object;
    obj->activationCount --;
    /*  fprintf(stderr, "CallStackPop cl=%p %s (%d) flags %.6x cl ns=%p cmd %p cmd ns %p\n",
            obj, objectName(obj), obj->activationCount, obj->flags, csc->cl->nsPtr, 
            csc->cmdPtr, ((Command *)csc->cmdPtr)->nsPtr); */

    /*fprintf(stderr, "dealloc called %d\n",rst->deallocCalled);*/

    /*fprintf(stderr, "CallStackPop check ac %d flags %.6x\n",
      obj->activationCount, obj->flags & XOTCL_DESTROY_CALLED);*/

    if (obj->activationCount < 1 && obj->flags & XOTCL_DESTROY_CALLED) {
      /* fprintf(stderr, "CallStackPop calls CallStackDoDestroy %p\n",obj);*/
      CallStackDoDestroy(interp, obj);
    }

    if (nsPtr) {
      nsPtr->refCount--;
      /*fprintf(stderr, "CallStackPop parent %s activationCount %d flags %.4x refCount %d\n", 
        nsPtr->fullName, nsPtr->activationCount, nsPtr->flags, nsPtr->refCount);*/
    
      if ((nsPtr->refCount == 0) && (nsPtr->flags & NS_DEAD)) {
        /* the namspace refcound has reached 0, we have to free
           it. unfortunately, NamespaceFree() is not exported */
        fprintf(stderr, "HAVE TO FREE %p\n",nsPtr);
        /*NamespaceFree(nsPtr);*/
        ckfree(nsPtr->fullName);
        ckfree(nsPtr->name);
        ckfree((char*)nsPtr);
      }
    }

    /*fprintf(stderr, "CallStackPop done\n");*/
  }
#endif
}
#endif /* TCL85STACK */


