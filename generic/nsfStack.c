
//static TclVarHashTable *VarHashTableCreate();

void TclShowStack(Tcl_Interp *interp) {
  Tcl_CallFrame *framePtr;

  fprintf(stderr, "TclShowStack framePtr %p varFramePtr %p\n",
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
    NsfCallStackContent *cscPtr = 
      (frameFlags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) ?
      ((NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr)) : NULL;

    fprintf(stderr, "... var frame %p flags %.6x cd %p lvl %d ns %p %s ov %s %d",
            framePtr, frameFlags,
            Tcl_CallFrame_clientData(framePtr),
            Tcl_CallFrame_level(framePtr),
            Tcl_CallFrame_nsPtr(framePtr), Tcl_CallFrame_nsPtr(framePtr)->fullName,
            Tcl_CallFrame_objc(framePtr) && 0 ? ObjStr(Tcl_CallFrame_objv(framePtr)[0]) : "(null)",
            Tcl_CallFrame_objc(framePtr) ? Tcl_CallFrame_objc(framePtr) : -1);
    if (cscPtr) {
      fprintf(stderr, " csc %p frameType %.4x callType %.4x (%p %s)\n",
	      cscPtr,
              cscPtr ? cscPtr->frameType : -1,
              cscPtr ? cscPtr->callType : -1,
              cscPtr ? cscPtr->self : NULL, 
              cscPtr ? objectName(cscPtr->self) : "");
    } else {
      fprintf(stderr, " no csc");
      if (frameFlags & FRAME_IS_NSF_OBJECT) {
        NsfObject *object = (NsfObject *)Tcl_CallFrame_clientData(framePtr);
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

static void Nsf_PushFrameObj(Tcl_Interp *interp, NsfObject *object, Tcl_CallFrame *framePtr) {
  /*fprintf(stderr,"PUSH OBJECT_FRAME (Nsf_PushFrameObj) frame %p\n",framePtr);*/
  if (object->nsPtr) {
    /*fprintf(stderr,"Nsf_PushFrame frame %p with object->nsPtr %p\n", framePtr, object->nsPtr);*/
    Tcl_PushCallFrame(interp, framePtr, object->nsPtr, 
                      0|FRAME_IS_NSF_OBJECT); 
  } else {
    /*fprintf(stderr,"Nsf_PushFrame frame %p (with fakeProc)\n",framePtr);*/
    Tcl_PushCallFrame(interp, framePtr, Tcl_CallFrame_nsPtr(Tcl_Interp_varFramePtr(interp)), 
                      1|FRAME_IS_NSF_OBJECT);
    
    Tcl_CallFrame_procPtr(framePtr) = &RUNTIME_STATE(interp)->fakeProc;
    if (object->varTable == NULL) {
      object->varTable = VarHashTableCreate();
      /*fprintf(stderr, "+++ create varTable %p in PushFrameObj obj %p framePtr %p\n",  
        object->varTable, object, framePtr);*/
    }
    Tcl_CallFrame_varTablePtr(framePtr) = object->varTable;
    /*fprintf(stderr,"+++ setting varTable %p in varFrame %p\n",object->varTable,framePtr);*/
  }
  Tcl_CallFrame_clientData(framePtr) = (ClientData)object;
}

static void Nsf_PopFrameObj(Tcl_Interp *interp, Tcl_CallFrame *framePtr) {
  /*fprintf(stderr,"POP  OBJECT_FRAME (Nsf_PopFrameObj) frame %p, vartable %p set to NULL, already %d\n", 
    framePtr, Tcl_CallFrame_varTablePtr(framePtr), Tcl_CallFrame_varTablePtr(framePtr) == NULL);*/
  Tcl_CallFrame_varTablePtr(framePtr) = NULL;
  Tcl_PopCallFrame(interp);
}

static void Nsf_PushFrameCsc(Tcl_Interp *interp, NsfCallStackContent *cscPtr, Tcl_CallFrame *framePtr) {
  CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
  /*fprintf(stderr,"PUSH CMETHOD_FRAME (Nsf_PushFrameCsc) frame %p cscPtr %p methodName %s\n",
    framePtr, cscPtr, Tcl_GetCommandName(interp,cscPtr->cmdPtr));*/

  Tcl_PushCallFrame(interp, framePtr, Tcl_CallFrame_nsPtr(varFramePtr), 
		    1|FRAME_IS_NSF_CMETHOD);
  Tcl_CallFrame_clientData(framePtr) = (ClientData)cscPtr;
  Tcl_CallFrame_procPtr(framePtr) = &RUNTIME_STATE(interp)->fakeProc;
}

static void Nsf_PopFrameCsc(Tcl_Interp *interp, Tcl_CallFrame *framePtr) {
  /*fprintf(stderr,"POP CMETHOD_FRAME (Nsf_PopFrameCsc) frame %p, varTable = %p\n",
    framePtr, Tcl_CallFrame_varTablePtr(framePtr));*/
  Tcl_PopCallFrame(interp);
}

/* 
 * stack query operations 
 */

static Tcl_CallFrame *
CallStackGetActiveProcFrame(Tcl_CallFrame *framePtr) {
  for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    register int flag = Tcl_CallFrame_isProcCallFrame(framePtr);

    if (flag & FRAME_IS_NSF_METHOD) {
      /* never return an inactive method frame */
      if (!(((NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr))->frameType 
            & NSF_CSC_TYPE_INACTIVE)) break;
    } else {
      if (flag & (FRAME_IS_NSF_CMETHOD|FRAME_IS_NSF_OBJECT)) continue;
      if (flag == 0 || flag & FRAME_IS_PROC) break;
    }
  }
  return framePtr;
}

static Tcl_CallFrame *
CallStackNextFrameOfType(Tcl_CallFrame *framePtr, int flags) {
  for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(framePtr) & flags)
      return framePtr;
  }
  return framePtr;
}

NSF_INLINE static NsfObject*
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
    if (flag & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
#if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... self returns %p %.6x %s\n", cscPtr->self, 
              cscPtr->self->flags, objectName(cscPtr->self));
#endif
      return cscPtr->self;
    } else if (flag & FRAME_IS_NSF_OBJECT) {
#if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... self returns %s\n",
              objectName(((NsfObject*)Tcl_CallFrame_clientData(varFramePtr))));
#endif
      return (NsfObject *)Tcl_CallFrame_clientData(varFramePtr);
    }
  }
  return NULL;
}

static NsfCallStackContent*
CallStackGetFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
# if defined(TCL85STACK_TRACE)
      fprintf(stderr, "... check frame %p flags %.6x cd %p objv[0] %s\n",
              varFramePtr, Tcl_CallFrame_isProcCallFrame(varFramePtr),
              Tcl_CallFrame_clientData(varFramePtr),
              Tcl_CallFrame_objc(varFramePtr) ? ObjStr(Tcl_CallFrame_objv(varFramePtr)[0]) : "(null)");
# endif
      if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
        if (framePtrPtr) *framePtrPtr = varFramePtr;
        return (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      }
  }
  if (framePtrPtr) *framePtrPtr = NULL;
  return NULL;
}

NSF_INLINE static NsfCallStackContent*
CallStackGetTopFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
  return CallStackGetFrame(interp, framePtrPtr);
}

/* find last invocation of a scripted method */
static NsfCallStackContent *
NsfCallStackFindLastInvocation(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  int lvl = Tcl_CallFrame_level(varFramePtr);
  
  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_NSF_METHOD) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if ((cscPtr->callType & NSF_CSC_CALL_IS_NEXT) || (cscPtr->frameType & NSF_CSC_TYPE_INACTIVE)) {
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

static NsfCallStackContent *
NsfCallStackFindActiveFrame(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  /* skip #offset frames */
  for (; offset>0 && varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr), offset--);

  /* search for first active frame and set tcl frame pointers */
  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_NSF_METHOD/*(FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)*/) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (!(cscPtr->frameType & NSF_CSC_TYPE_INACTIVE)) {
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
  Tcl_CallFrame 
    *inFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp),
    *framePtr;

  /*NsfCallStackFindActiveFrame(interp, 0, &activeFramePtr);*/
# if defined(TCL85STACK_TRACE)
  TclShowStack(interp);
# endif
  /* Get the first active non object frame */
  framePtr = CallStackGetActiveProcFrame(inFramePtr);

  /*fprintf(stderr,"... use frameptr %p \n", framePtr);*/

  if (inFramePtr == framePtr) {
    /* call frame pointers are fine */
    ctx->framesSaved = 0;
  } else {
    ctx->varFramePtr = inFramePtr;
    /*fprintf(stderr, "CallStackUseActiveFrames stores %p\n",framePtr);*/
    Tcl_Interp_varFramePtr(interp) = (CallFrame *)framePtr;
    ctx->framesSaved = 1;
  }
}

static void
CallStackRestoreSavedFrames(Tcl_Interp *interp, callFrameContext *ctx) {
  if (ctx->framesSaved) {
    /*fprintf(stderr, "CallStackRestoreSavedFrames drops %p restores %p\n",
      Tcl_Interp_varFramePtr(interp), ctx->varFramePtr);*/
    Tcl_Interp_varFramePtr(interp) = (CallFrame *)ctx->varFramePtr;
  }
}

static NsfCallStackContent *
CallStackFindActiveFilter(Tcl_Interp *interp) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER) {
        return cscPtr;
      }
    }
  }
  /* for some reasons, we could not find invocation (topLevel, destroy) */
  return NULL;
}


/*
 *----------------------------------------------------------------------
 * CallStackFindEnsembleCsc --
 *
 *    Return the callstack content and the optionally the stack frame
 *    of the last ensemble invocation.
 *
 * Results:
 *    callstack content
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static NsfCallStackContent *
CallStackFindEnsembleCsc(Tcl_CallFrame *framePtr, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr;
  NsfCallStackContent *cscPtr = NULL;

  assert(framePtr);
  for (varFramePtr = Tcl_CallFrame_callerPtr(framePtr);
       Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_NSF_CMETHOD; 
       varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
    assert(cscPtr);
    /*
     * The test for CALL_IS_ENSEMBLE is just a saftey belt
     */ 
    if ((cscPtr->callType & NSF_CSC_CALL_IS_ENSEMBLE) == 0) break;
  }
  if (framePtrPtr) {
    *framePtrPtr = varFramePtr;
  }
  
  return cscPtr;
}

/*
 * check, if there is an active filters on "obj" using cmd
 */
NSF_INLINE static int
FilterActiveOnObj(Tcl_Interp *interp, NsfObject *object, Tcl_Command cmd) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (cmd == cscPtr->cmdPtr && object == cscPtr->self &&
          cscPtr->frameType == NSF_CSC_TYPE_ACTIVE_FILTER) {
        return 1;
      }
    }
  }
  return 0;
}

static void
CallStackReplaceVarTableReferences(Tcl_Interp *interp, TclVarHashTable *oldVarTablePtr, TclVarHashTable *newVarTablePtr) {   
  Tcl_CallFrame *framePtr;

  for (framePtr = (Tcl_CallFrame *)Tcl_Interp_framePtr(interp); framePtr; 
       framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    int frameFlags = Tcl_CallFrame_isProcCallFrame(framePtr);
          
    if (!(frameFlags & FRAME_IS_NSF_OBJECT)) continue;
    if (!(Tcl_CallFrame_varTablePtr(framePtr) == oldVarTablePtr)) continue;
    
    /*fprintf(stderr, "+++ makeObjNamespace replacing vartable %p with %p in frame %p\n", 
      oldVarTablePtr, newVarTablePtr, framePtr);*/
    Tcl_CallFrame_varTablePtr(framePtr) = newVarTablePtr;
  }
}

static void
CallStackClearCmdReferences(Tcl_Interp *interp, Tcl_Command cmd) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (cscPtr->cmdPtr == cmd) {
        cscPtr->cmdPtr = NULL;
      }
    }
  }
}


#if 0
/* just used by NsfONextMethod() */
static NsfCallStackContent*
CallStackGetObjectFrame(Tcl_Interp *interp, NsfObject *object) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (cscPtr->self == object) {
        return cscPtr;
      }
    }
  }
  return NULL;
}
#endif

/*
 * Pop any callstack entry that is still alive (e.g.
 * if "exit" is called and we were jumping out of the
 * callframe
 */
static void CallStackPopAll(Tcl_Interp *interp) {
  TclShowStack(interp);

  while (1) {
    Tcl_CallFrame *framePtr = Tcl_Interp_framePtr(interp);
    int frameFlags; 

    if (!framePtr) break;
    if (Tcl_CallFrame_level(framePtr) == 0) break;

    frameFlags = Tcl_CallFrame_isProcCallFrame(framePtr);
    /*fprintf(stderr, "--- popping %p frameflags %.6x\n", framePtr, frameFlags);*/

    if (frameFlags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      /* free the call stack content; we need this just for decr activation count */
      NsfCallStackContent *cscPtr = ((NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr));

#if defined(NRE)
      /* Mask out IS_NRE, since Tcl_PopCallFrame takes care about TclStackFree */
      cscPtr->callType &= ~NSF_CSC_CALL_IS_NRE;
#endif
      CscFinish(interp, cscPtr, "popall");
    } else if (frameFlags & FRAME_IS_NSF_OBJECT) {
      Tcl_CallFrame_varTablePtr(framePtr) = NULL;
    }

    /* pop the Tcl frame */
    Tcl_PopCallFrame(interp);
  }
}

/*
 *----------------------------------------------------------------------
 * CscAlloc --
 *
 *    Allocate the csc structure either from the stack or via
 *    StackAlloc (the latter is recorded in the callType). The Alloc
 *    operation requires a CscFinish operation later.
 *
 * Results:
 *    A valid, semiinitialized cscPtr.
 *
 * Side effects:
 *    Memory allocation
 *
 *----------------------------------------------------------------------
 */
static NsfCallStackContent *
CscAlloc(Tcl_Interp *interp, NsfCallStackContent *cscPtr, Tcl_Command cmd) {
#if defined(NRE)
  Tcl_ObjCmdProc *proc = cmd ? Tcl_Command_objProc(cmd) : NULL;

  if (proc == TclObjInterpProc) {
    cscPtr = (NsfCallStackContent *) NsfTclStackAlloc(interp, sizeof(NsfCallStackContent), "csc");
    cscPtr->callType = NSF_CSC_CALL_IS_NRE;
  } else {
    cscPtr->callType = 0;
  }
#else
  cscPtr->callType = 0;
#endif

  /*fprintf(stderr, "CscAlloc allocated %p\n",cscPtr);*/
  return cscPtr;
}

/*
 *----------------------------------------------------------------------
 * CscInit --
 *
 *    Initialize call stack content and track activation counts
 *    of involved objects and classes
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Initialized Csc, updated activation counts
 *
 *----------------------------------------------------------------------
 */

NSF_INLINE static void
CscInit(/*@notnull@*/ NsfCallStackContent *cscPtr, NsfObject *object, NsfClass *cl,
	Tcl_Command cmd, int frameType, int flags, char *msg) {

  assert(cscPtr);

  if (cmd) {
    /* 
     *  When cmd is provided, the call is not an unknown, the method
     *  will be executed and the object will be stacked. In these
     *  cases, we maintain an activation count. The fact that the
     *  activation cound was incremented for this frame is noted via
     *  NSF_CSC_OBJECT_ACTIVATED; callType is initialized in
     *  CscAlloc()
     */

    cscPtr->callType |= NSF_CSC_OBJECT_ACTIVATED;

    // TODO
    /*
     * Some csc's are never stacked. We flag this case by setting self
     * to NULL. This cscPtr should never appear on the stack.
     */
    if (Tcl_Command_objClientData(cmd) == NULL && !(Tcl_Command_flags(cmd) & NSF_CMD_NONLEAF_METHOD)) {
      /*fprintf(stderr, "+++ no CscInit needed\n");*/
      //cscPtr->self = NULL;
      //return;
    }
    
    /*
     * track object activations
     */ 
    object->activationCount ++;
    //fprintf(stderr, "activationCount ++ (%s) --> %d\n",objectName(object),  object->activationCount);
    /*
     * track class activations
     */ 
    if (cl && cmd) {
      Namespace *nsPtr = ((Command *)cmd)->nsPtr;
      cl->object.activationCount ++;
      /*fprintf(stderr, "... %s cmd %s cmd ns %p (%s, refCount %d ++) obj ns %p parent %p\n", 
	className(cl), 
	Tcl_GetCommandName(object->teardown, cmd),
	nsPtr, nsPtr->fullName, nsPtr->refCount,
	cl->object.nsPtr,cl->object.nsPtr ? ((Namespace*)cl->object.nsPtr)->parentPtr : NULL);*/
      
      /* incremement the namespace ptr in case tcl tries to delete this namespace 
	 during the invocation */
      nsPtr->refCount ++;
    }
    
  }
  cscPtr->callType     |= flags & NSF_CSC_COPY_FLAGS;
  cscPtr->self          = object;
  cscPtr->cl            = cl;
  cscPtr->cmdPtr        = cmd;
  cscPtr->frameType     = frameType;
  cscPtr->filterStackEntry = (frameType == NSF_CSC_TYPE_ACTIVE_FILTER) ? object->filterStack : NULL;
  cscPtr->objv          = NULL;
  
#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "PUSH csc %p type %d obj %s, self=%p cmd=%p (%s) id=%p (%s) obj refcount %d name refcount %d\n",
          cscPtr, frameType, objectName(object), object,
          cmd, (char *) Tcl_GetCommandName(object->teardown, cmd),
          object->id, object->id ? Tcl_GetCommandName(object->teardown, object->id) : "(deleted)",
          object->id ? Tcl_Command_refCount(object->id) : -100, object->cmdName->refCount
          );
#endif
  /*fprintf(stderr, "CscInit %p (%s) object %p %s flags %.6x cmdPtr %p\n",cscPtr, msg,
    object, objectName(object), cscPtr->callType, cscPtr->cmdPtr);*/
}

/*
 *----------------------------------------------------------------------
 * CscFinish --
 *
 *    Counterpart of CscInit(). Decrement activation counts
 *    and delete objects/classes if necessary.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    potentially deletes objects, classes or namespaces.
 *
 *----------------------------------------------------------------------
 */
NSF_INLINE static void
CscFinish(Tcl_Interp *interp, NsfCallStackContent *cscPtr, char *msg) {
  int allowDestroy = RUNTIME_STATE(interp)->exitHandlerDestroyRound !=
    NSF_EXITHANDLER_ON_SOFT_DESTROY;
  NsfObject *object;
  int flags;

  assert(cscPtr);
  assert(cscPtr->self);

  object = cscPtr->self;
  flags = cscPtr->callType;

  /*fprintf(stderr, "CscFinish %p (%s) object %p %s flags %.6x cmdPtr %p\n",cscPtr, msg, 
    object, objectName(object), flags, cscPtr->cmdPtr);*/

#if defined(TCL85STACK_TRACE)
  fprintf(stderr, "POP  csc=%p, obj %s method %s (%s)\n", cscPtr, objectName(object),
          Tcl_GetCommandName(interp, cscPtr->cmdPtr), msg);
#endif
  /* 
   *  We cannot rely on the existence of cscPtr->cmdPtr (like in
   *  initialize), since the cmd might have been deleted during the
   *  activation.
   */
  if ((flags & NSF_CSC_OBJECT_ACTIVATED)) {
    /* 
       tracking activations of objects
    */
    object->activationCount --;
    
    /*fprintf(stderr, "... activationCount -- (%s) --> %d\n",objectName(object),  object->activationCount);*/
    
    /*fprintf(stderr, "decr activationCount for %s to %d cscPtr->cl %p\n", objectName(cscPtr->self), 
      cscPtr->self->activationCount, cscPtr->cl);*/
    assert(object->activationCount > -1);
    if (object->activationCount < 1 && object->flags & NSF_DESTROY_CALLED && allowDestroy) {
      CallStackDoDestroy(interp, object);
    }
#if defined(OBJDELETION_TRACE)
    else if (!allowDestroy) {
      fprintf(stderr,"checkFree %p %s\n",object, objectName(object));
    }
#endif
    
    /* 
       tracking activations of classes 
    */
    if (cscPtr->cl) {
      Namespace *nsPtr = cscPtr->cmdPtr ? ((Command *)(cscPtr->cmdPtr))->nsPtr : NULL;
      
      object = &cscPtr->cl->object;
      object->activationCount --;
      /*fprintf(stderr, "CscFinish cl=%p %s (%d) flags %.6x cl ns=%p cmd %p cmd ns %p\n",
	      object, objectName(object), object->activationCount, object->flags, cscPtr->cl->nsPtr, 
	      cscPtr->cmdPtr, ((Command *)cscPtr->cmdPtr)->nsPtr); */
      
      /*fprintf(stderr, "CscFinish check ac %d flags %.6x\n",
	object->activationCount, object->flags & NSF_DESTROY_CALLED);*/
      
      if (object->activationCount < 1 && object->flags & NSF_DESTROY_CALLED && allowDestroy) {
	CallStackDoDestroy(interp, object);
      } 
#if defined(OBJDELETION_TRACE)
      else if (!allowDestroy) {
	fprintf(stderr,"checkFree %p %s\n",object, objectName(object));
      }
#endif
      // TODO do we have a leak now?
      if (0 && nsPtr) {
	nsPtr->refCount--;
	/*fprintf(stderr, "CscFinish parent %s activationCount %d flags %.4x refCount %d\n", 
	  nsPtr->fullName, nsPtr->activationCount, nsPtr->flags, nsPtr->refCount);*/
	
	if ((nsPtr->refCount == 0) && (nsPtr->flags & NS_DEAD)) {
	  /* the namespace refcount has reached 0, we have to free
	     it. unfortunately, NamespaceFree() is not exported */
	  /* TODO: remove me finally */
	  fprintf(stderr, "HAVE TO FREE %p\n",nsPtr);
	  /*NamespaceFree(nsPtr);*/
	  ckfree(nsPtr->fullName);
	  ckfree(nsPtr->name);
	  ckfree((char*)nsPtr);
	}
      }
      
    }
  }

#if defined(NRE)
  if ((cscPtr->callType & NSF_CSC_CALL_IS_NRE)) {
    NsfTclStackFree(interp, cscPtr, msg);
  }
#endif
  /*fprintf(stderr, "CscFinish done\n");*/
}


