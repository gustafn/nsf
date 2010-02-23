
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
    int frameFlags = Tcl_CallFrame_isProcCallFrame(framePtr);
    XOTclCallStackContent *cscPtr = 
      (frameFlags & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) ?
      ((XOTclCallStackContent *)Tcl_CallFrame_clientData(framePtr)) : NULL;

    fprintf(stderr, "... var frame %p flags %.6x cd %p lvl %d ns %p %s ov %s %d",
            framePtr, frameFlags,
            Tcl_CallFrame_clientData(framePtr),
            Tcl_CallFrame_level(framePtr),
            Tcl_CallFrame_nsPtr(framePtr), Tcl_CallFrame_nsPtr(framePtr)->fullName,
            Tcl_CallFrame_objc(framePtr) ? ObjStr(Tcl_CallFrame_objv(framePtr)[0]) : "(null)",
            Tcl_CallFrame_objc(framePtr) ? Tcl_CallFrame_objc(framePtr) : -1);
    if (cscPtr) {
      fprintf(stderr, " frameType %d callType %d (%p %s)\n",
              cscPtr ? cscPtr->frameType : -1,
              cscPtr ? cscPtr->callType : -1,
              cscPtr ? cscPtr->self : NULL, 
              cscPtr ? objectName(cscPtr->self) : "");
    } else {
      fprintf(stderr, " no csc");
      if (frameFlags & FRAME_IS_XOTCL_OBJECT) {
        XOTclObject *object = (XOTclObject *)Tcl_CallFrame_clientData(framePtr);
        fprintf(stderr, " obj %p %s", object, objectName(object));
      }
      fprintf(stderr, "\n");
    }
  }
}

/* 
 * Push and pop operations.
 *
 * Note that it is possible that between push and pop
 * a object->nsPtr can be created (e.g. during a read trace)
 */
#define XOTcl_FrameDecls TclCallFrame frame, *framePtr = &frame
# ifndef PRE85
#  define XOTcl_PushFrameSetCd(framePtr, object) ((CallFrame *)framePtr)->clientData = (ClientData)(object)
# else
#  define XOTcl_PushFrameSetCd(framePtr, object)
# endif

static TclVarHashTable *VarHashTableCreate();

#define XOTcl_PushFrameObj(interp,object) XOTcl_PushFrameObj2(interp, object, framePtr)
#define XOTcl_PopFrameObj(interp) XOTcl_PopFrameObj2(interp, framePtr)

static void XOTcl_PushFrameObj2(Tcl_Interp *interp, XOTclObject *object, Tcl_CallFrame *framePtr) {
  /*fprintf(stderr,"PUSH OBJECT_FRAME (XOTcl_PushFrame) frame %p\n",framePtr);*/
  if (object->nsPtr) {
    /*fprintf(stderr,"XOTcl_PushFrame frame %p\n",framePtr);*/
    Tcl_PushCallFrame(interp, framePtr, object->nsPtr, 
                      0|FRAME_IS_XOTCL_OBJECT); 
  } else {
    /*fprintf(stderr,"XOTcl_PushFrame frame %p (with fakeProc)\n",framePtr);*/ 
    Tcl_PushCallFrame(interp, framePtr, Tcl_CallFrame_nsPtr(Tcl_Interp_varFramePtr(interp)), 
                      1|FRAME_IS_XOTCL_OBJECT);
    
    Tcl_CallFrame_procPtr(framePtr) = &RUNTIME_STATE(interp)->fakeProc;
    if (object->varTable == NULL) {
      object->varTable = VarHashTableCreate();
      /*fprintf(stderr, "+++ create varTable %p in PushFrameObj obj %p framePtr %p\n",  
        object->varTable, object, framePtr);*/
    }
    Tcl_CallFrame_varTablePtr(framePtr) = object->varTable;
  }
  XOTcl_PushFrameSetCd(framePtr, object);
}
static void XOTcl_PopFrameObj2(Tcl_Interp *interp, Tcl_CallFrame *framePtr) {
  Tcl_CallFrame_varTablePtr(framePtr) = 0;
  /*fprintf(stderr,"POP  OBJECT_FRAME (XOTcl_PopFrame) frame %p\n",framePtr);*/
  Tcl_PopCallFrame(interp);
}


#define XOTcl_PushFrameCsc(interp,cscPtr) XOTcl_PushFrameCsc2(interp, cscPtr, framePtr)
#define XOTcl_PopFrameCsc(interp) XOTcl_PopFrameCsc2(interp, framePtr)

static void XOTcl_PushFrameCsc2(Tcl_Interp *interp, XOTclCallStackContent *cscPtr, Tcl_CallFrame *framePtr) {
  CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);

  /*fprintf(stderr,"PUSH CMETHOD_FRAME (XOTcl_PushFrame) frame %p object->nsPtr %p interp ns %p\n",
          framePtr,object->nsPtr, 
          Tcl_CallFrame_nsPtr(varFramePtr));*/

  Tcl_PushCallFrame(interp, framePtr, Tcl_CallFrame_nsPtr(varFramePtr), 0|FRAME_IS_XOTCL_CMETHOD);
  XOTcl_PushFrameSetCd(framePtr, cscPtr);
}

static void XOTcl_PopFrameCsc2(Tcl_Interp *interp, Tcl_CallFrame *framePtr) {
  Tcl_PopCallFrame(interp);
}

/* 
 * query operations.
 *
 */

static Tcl_CallFrame *
nonXotclObjectProcFrame(Tcl_CallFrame *framePtr) {
  for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    register int flag = Tcl_CallFrame_isProcCallFrame(framePtr);

    if (flag & FRAME_IS_XOTCL_METHOD) {
      /* never return an inactive method frame */
      if (!(((XOTclCallStackContent *)Tcl_CallFrame_clientData(framePtr))->frameType 
            & XOTCL_CSC_TYPE_INACTIVE)) break;
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
      XOTclCallStackContent *cscPtr = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
#if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... self returns %p %.6x %s\n", cscPtr->self, 
              cscPtr->self->flags, objectName(cscPtr->self));
#endif
      return cscPtr->self;
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

static XOTclCallStackContent *
XOTclCallStackFindLastInvocation(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  int lvl = Tcl_CallFrame_level(varFramePtr);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *cscPtr = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if ((cscPtr->callType & XOTCL_CSC_CALL_IS_NEXT) || (cscPtr->frameType & XOTCL_CSC_TYPE_INACTIVE)) {
        continue;
      }
      if (offset) {
        offset--;
      } else {
        if (Tcl_CallFrame_level(varFramePtr) < lvl) {
          if (framePtrPtr) *framePtrPtr = varFramePtr;
          return cscPtr;
        }
      }
    }
  }
  if (framePtrPtr) *framePtrPtr = NULL;
  return NULL;
}

static XOTclCallStackContent *
XOTclCallStackFindActiveFrame(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  /* skip #offset frames */
  for (; offset>0 && varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr), offset--);

  /* search for first active frame and set tcl frame pointers */
  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *cscPtr = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (!(cscPtr->frameType & XOTCL_CSC_TYPE_INACTIVE)) {
        /* we found the highest active frame */
        if (framePtrPtr) *framePtrPtr = varFramePtr;
        return cscPtr;
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
      XOTclCallStackContent *cscPtr = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (cscPtr->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
        return cscPtr;
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
FilterActiveOnObj(Tcl_Interp *interp, XOTclObject *object, Tcl_Command cmd) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *cscPtr = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (cmd == cscPtr->cmdPtr && object == cscPtr->self &&
          cscPtr->frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER) {
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
      XOTclCallStackContent *cscPtr = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (cscPtr->cmdPtr == cmd) {
        cscPtr->cmdPtr = NULL;
      }
    }
  }
}

static XOTclCallStackContent*
CallStackGetObjectFrame(Tcl_Interp *interp, XOTclObject *object) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_XOTCL_METHOD|FRAME_IS_XOTCL_CMETHOD)) {
      XOTclCallStackContent *cscPtr = (XOTclCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (cscPtr->self == object) {
        return cscPtr;
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
      XOTclCallStackContent *cscPtr = ((XOTclCallStackContent *)Tcl_CallFrame_clientData(framePtr));
      CallStackPop(interp, cscPtr);
    }
    /* pop the Tcl frame */
    Tcl_PopCallFrame(interp);
  }
}

XOTCLINLINE static void
CallStackPush(XOTclCallStackContent *cscPtr, XOTclObject *object, XOTclClass *cl, Tcl_Command cmd, int frameType) {
  object->activationCount ++;
#if 1
  if (cl) {
    Namespace *nsPtr = ((Command *)cmd)->nsPtr;
    cl->object.activationCount ++;
    /*fprintf(stderr, "... %s cmd %s cmd ns %p (%s) obj ns %p parent %p\n", 
            className(cl), 
            Tcl_GetCommandName(object->teardown, cmd),
            ((Command *)cmd)->nsPtr, ((Command *)cmd)->nsPtr->fullName,
            cl->object.nsPtr,cl->object.nsPtr ? ((Namespace*)cl->object.nsPtr)->parentPtr : NULL);*/
    
    /* incremement the namespace ptr in case tcl tries to delete this namespace 
       during the invocation */
    nsPtr->refCount ++;
  }
#endif
  /* fprintf(stderr, "incr activationCount for %s to %d\n", objectName(object), object->activationCount); */
  cscPtr->self          = object;
  cscPtr->cl            = cl;
  cscPtr->cmdPtr        = cmd;
  cscPtr->frameType     = frameType;
  cscPtr->callType      = 0;
  cscPtr->filterStackEntry = frameType == XOTCL_CSC_TYPE_ACTIVE_FILTER ? object->filterStack : NULL;
  cscPtr->objv          = NULL;

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "PUSH csc %p type %d obj %s, self=%p cmd=%p (%s) id=%p (%s) obj refcount %d name refcount %d\n",
          cscPtr, frameType, objectName(object), object,
          cmd, (char *) Tcl_GetCommandName(object->teardown, cmd),
          object->id, object->id ? Tcl_GetCommandName(object->teardown, object->id) : "(deleted)",
          object->id ? Tcl_Command_refCount(object->id) : -100, object->cmdName->refCount
          );
#endif
}

XOTCLINLINE static void
CallStackPop(Tcl_Interp *interp, XOTclCallStackContent *cscPtr) {
  XOTclObject *object = cscPtr->self;

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "POP  csc=%p, obj %s method %s (%d)\n", cscPtr, objectName(object),
          Tcl_GetCommandName(interp, cscPtr->cmdPtr));
#endif
  object->activationCount --;
  
  /* fprintf(stderr, "decr activationCount for %s to %d cscPtr->cl %p\n", objectName(cscPtr->self), 
     csc->self->activationCount, cscPtr->cl);*/

  if (object->activationCount < 1 && object->flags & XOTCL_DESTROY_CALLED) {
    CallStackDoDestroy(interp, object);
  }
#if 1
  if (cscPtr->cl) {
    Namespace *nsPtr = cscPtr->cmdPtr ? ((Command *)(cscPtr->cmdPtr))->nsPtr : NULL;

    object = &cscPtr->cl->object;
    object->activationCount --;
    /*  fprintf(stderr, "CallStackPop cl=%p %s (%d) flags %.6x cl ns=%p cmd %p cmd ns %p\n",
            object, objectName(object), object->activationCount, object->flags, cscPtr->cl->nsPtr, 
            cscPtr->cmdPtr, ((Command *)cscPtr->cmdPtr)->nsPtr); */

    /*fprintf(stderr, "CallStackPop check ac %d flags %.6x\n",
      object->activationCount, object->flags & XOTCL_DESTROY_CALLED);*/

    if (object->activationCount < 1 && object->flags & XOTCL_DESTROY_CALLED) {
      /* fprintf(stderr, "CallStackPop calls CallStackDoDestroy %p\n", object);*/
      CallStackDoDestroy(interp, object);
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


