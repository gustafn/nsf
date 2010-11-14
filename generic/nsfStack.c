
#ifdef CHECK_ACTIVATION_COUNTS
static NsfClasses * NsfClassListUnlink(NsfClasses **firstPtrPtr, void *key);

/*
 *----------------------------------------------------------------------
 * CscListAdd --
 *
 *    Add csc entry to the list of unstack entries
 *
 * Results:
 *    none
 *
 * Side effects:
 *    list element added
 *
 *----------------------------------------------------------------------
 */
static void
CscListAdd(Tcl_Interp *interp, NsfCallStackContent *cscPtr) {
  NsfClassListAdd(&RUNTIME_STATE(interp)->cscList, (NsfClass *)cscPtr, NULL);
}

/*
 *----------------------------------------------------------------------
 * CscListRemove --
 *
 *    Remove csc entry from the list of unstack entries
 *
 * Results:
 *    true on success or 0
 *
 * Side effects:
 *    list element potentially removed and freed
 *
 *----------------------------------------------------------------------
 */
static int
CscListRemove(Tcl_Interp *interp, NsfCallStackContent *cscPtr) {
  NsfClasses *entryPtr;

  entryPtr = NsfClassListUnlink(&RUNTIME_STATE(interp)->cscList, cscPtr);
  if (entryPtr) {
    FREE(NsfClasses, entryPtr);
  }
  return (entryPtr != NULL);
}
#endif

/*
 *----------------------------------------------------------------------
 * NsfShowStack --
 *
 *    Print the contents of the callstack to stderr. This function is
 *    for debugging purposes only.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Output on stderr.
 *
 *----------------------------------------------------------------------
 */
void NsfShowStack(Tcl_Interp *interp) {
  Tcl_CallFrame *framePtr;

  fprintf(stderr, "NsfShowStack framePtr %p varFramePtr %p\n",
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
      fprintf(stderr, " csc %p frameType %.4x callType %.4x (%s.%p %s)\n",
	      cscPtr,
              cscPtr ? cscPtr->frameType : -1,
              cscPtr ? cscPtr->flags : -1,
              cscPtr ? objectName(cscPtr->self) : "",
              cscPtr ? cscPtr->cmdPtr : NULL,
              cscPtr ? Tcl_GetCommandName(interp, cscPtr->cmdPtr) : ""
	      );
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

/*
 *----------------------------------------------------------------------
 * Nsf_PushFrameObj, Nsf_PopFrameObj --
 *
 *    Push or pop a frame with a callstack content as a OBJECT
 *    frame.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static void Nsf_PushFrameObj(Tcl_Interp *interp, NsfObject *object, CallFrame *framePtr) {
  /*fprintf(stderr,"PUSH OBJECT_FRAME (Nsf_PushFrameObj) frame %p\n",framePtr);*/
  if (object->nsPtr) {
    Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, object->nsPtr,
                      0|FRAME_IS_NSF_OBJECT);
  } else {
    /* The object has no nsPtr, so we diguise as a proc, using fakeProc */
    Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, Tcl_CallFrame_nsPtr(Tcl_Interp_varFramePtr(interp)),
                      FRAME_IS_PROC|FRAME_IS_NSF_OBJECT);

    Tcl_CallFrame_procPtr(framePtr) = &RUNTIME_STATE(interp)->fakeProc;
    if (object->varTablePtr == NULL) {
      object->varTablePtr = VarHashTableCreate();
    }
    Tcl_CallFrame_varTablePtr(framePtr) = object->varTablePtr;
  }
  Tcl_CallFrame_clientData(framePtr) = (ClientData)object;
}

static void Nsf_PopFrameObj(Tcl_Interp *interp, CallFrame *framePtr) {

  /*fprintf(stderr,"POP  OBJECT_FRAME (Nsf_PopFrameObj) frame %p, vartable %p set to NULL, already %d\n",
    framePtr, Tcl_CallFrame_varTablePtr(framePtr), Tcl_CallFrame_varTablePtr(framePtr) == NULL);*/

  Tcl_CallFrame_varTablePtr(framePtr) = NULL; 
  Tcl_PopCallFrame(interp);
}

/*
 *----------------------------------------------------------------------
 * Nsf_PushFrameCsc, Nsf_PopFrameCsc --
 *
 *    Push or pop a frame with a callstack content as a CMETHOD
 *    frame.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static void 
Nsf_PushFrameCsc(Tcl_Interp *interp, NsfCallStackContent *cscPtr, CallFrame *framePtr) {
  CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
  /*fprintf(stderr,"PUSH CMETHOD_FRAME (Nsf_PushFrameCsc) frame %p cscPtr %p methodName %s\n",
    framePtr, cscPtr, Tcl_GetCommandName(interp,cscPtr->cmdPtr));*/

  Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, Tcl_CallFrame_nsPtr(varFramePtr),
		    FRAME_IS_PROC|FRAME_IS_NSF_CMETHOD);
  Tcl_CallFrame_clientData(framePtr) = (ClientData)cscPtr;
  Tcl_CallFrame_procPtr(framePtr) = &RUNTIME_STATE(interp)->fakeProc;
}

static void 
Nsf_PopFrameCsc(Tcl_Interp *interp, CallFrame *framePtr) {
  /*fprintf(stderr,"POP CMETHOD_FRAME (Nsf_PopFrameCsc) frame %p, varTablePtr = %p\n",
    framePtr, Tcl_CallFrame_varTablePtr(framePtr));*/
  Tcl_PopCallFrame(interp);
}

/*
 * stack query operations
 */

/*
 *----------------------------------------------------------------------
 * CallStackGetActiveProcFrame --
 *
 *    Return the Tcl call frame of the last scripted method.
 *
 * Results:
 *    Tcl_CallFrame
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_CallFrame *
CallStackGetActiveProcFrame(Tcl_CallFrame *framePtr) {
  for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    register int flag = Tcl_CallFrame_isProcCallFrame(framePtr);

    if (flag & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      /* never return an inactive method frame */
      if (!(((NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr))->frameType
            & NSF_CSC_TYPE_INACTIVE)) break;
    } else {
      if (flag & (FRAME_IS_NSF_OBJECT)) continue;
      if (flag == 0 || (flag & FRAME_IS_PROC)) break;
    }
  }

  return framePtr;
}

/*
 *----------------------------------------------------------------------
 * CallStackNextFrameOfType --
 *
 *    Return the next frame with a specified type from the call stack.
 *    The type is specified by a bit mask passed as flags.
 *
 * Results:
 *    Tcl_CallFrame
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_CallFrame *
CallStackNextFrameOfType(Tcl_CallFrame *framePtr, int flags) {
  for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(framePtr) & flags)
      return framePtr;
  }
  return framePtr;
}

/*
 *----------------------------------------------------------------------
 * GetSelfObj --
 *
 *    Return the currently active object from a method or object frame.
 *
 * Results:
 *    NsfObject * or NULL.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

#define SKIP_LEVELS 1
#define SKIP_LAMBDA 1

#if defined(SKIP_LAMBDA)
# if !defined(SKIP_LEVELS)
#  define SKIP_LEVELS 1
# endif
#endif

NSF_INLINE static NsfObject*
GetSelfObj(Tcl_Interp *interp) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  /*fprintf(stderr, "GetSelfObj interp has frame %p and varframe %p\n",
    Tcl_Interp_framePtr(interp),Tcl_Interp_varFramePtr(interp));*/

  for (; varFramePtr; varFramePtr =
	 
#if defined(SKIP_LEVELS)
	 Tcl_CallFrame_callerPtr(varFramePtr)
#else
	 NULL
#endif
       ) {
    register int flags = Tcl_CallFrame_isProcCallFrame(varFramePtr);

    if (flags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      return cscPtr->self;

    } else if (flags & FRAME_IS_NSF_OBJECT) {

      return (NsfObject *)Tcl_CallFrame_clientData(varFramePtr);
    }
#if defined(SKIP_LAMBDA)
    if (flags & FRAME_IS_LAMBDA) {
      continue;
    }
    break;
#endif
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * CallStackGetTopFrame --
 *
 *    Return the topmost invocation of a (scripted or nonleaf) method
 *
 * Results:
 *    Call stack content or NULL.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static NsfCallStackContent*
CallStackGetTopFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {

      if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
        if (framePtrPtr) *framePtrPtr = varFramePtr;
        return (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      }
  }
  if (framePtrPtr) *framePtrPtr = NULL;
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * NsfCallStackFindLastInvocation --
 *
 *    Find last invocation of a (scripted or nonleaf) method with a
 *    specified offset.
 *
 * Results:
 *    Call stack content or NULL.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static NsfCallStackContent *
NsfCallStackFindLastInvocation(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  int lvl = Tcl_CallFrame_level(varFramePtr);

  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if ((cscPtr->flags & (NSF_CSC_CALL_IS_NEXT|NSF_CSC_CALL_IS_ENSEMBLE|NSF_CSC_CALL_IS_TRANSPARENT))
	  || (cscPtr->frameType & NSF_CSC_TYPE_INACTIVE)) {
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

/*
 *----------------------------------------------------------------------
 * NsfCallStackFindActiveFrame --
 *
 *    Search for the first active frame on the callstack.
 *
 * Results:
 *    Call stack content or NULL.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static NsfCallStackContent *
NsfCallStackFindActiveFrame(Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  /* skip #offset frames */
  for (; offset>0 && varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr), offset--);

  /* search for first active frame and set tcl frame pointers */
  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & /*FRAME_IS_NSF_METHOD*/ (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
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

/*
 *----------------------------------------------------------------------
 * CallStackUseActiveFrame --
 *
 *    Activate the varFrame of the first active non-object frame and
 *    save the previously active frames in the call frame context.
 *    These stored frames are typically reactivated by
 *    CallStackRestoreSavedFrames().
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    The varFramePtr of the interp is potentially updated
 *
 *----------------------------------------------------------------------
 */

static void
CallStackUseActiveFrame(Tcl_Interp *interp, callFrameContext *ctx) {
  Tcl_CallFrame *framePtr, *inFramePtr;

  inFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  /* Get the first active non object frame */
  framePtr = CallStackGetActiveProcFrame(inFramePtr);

  /*fprintf(stderr,"... use frameptr %p \n", framePtr);*/

  if (inFramePtr == framePtr) {
    /* call frame pointers are fine */
    ctx->frameSaved = 0;
  } else {
    ctx->varFramePtr = inFramePtr;
    /*fprintf(stderr, "CallStackUseActiveFrame stores %p\n",framePtr);*/
    Tcl_Interp_varFramePtr(interp) = (CallFrame *)framePtr;
    ctx->frameSaved = 1;
  }
}

/*
 *----------------------------------------------------------------------
 * CallStackRestoreSavedFrames --
 *
 *    Restore the previously saved frames from the speficied call
 *    frame context. These frames are typically saved by
 *    CallStackUseActiveFrame().
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    The varFramePtr of the interp is potentially updated
 *
 *----------------------------------------------------------------------
 */

static void
CallStackRestoreSavedFrames(Tcl_Interp *interp, callFrameContext *ctx) {
  if (ctx->frameSaved) {
    /*fprintf(stderr, "CallStackRestoreSavedFrames drops %p restores %p\n",
      Tcl_Interp_varFramePtr(interp), ctx->varFramePtr);*/
    Tcl_Interp_varFramePtr(interp) = (CallFrame *)ctx->varFramePtr;
  }
}

/*
 *----------------------------------------------------------------------
 * CallStackFindActiveFilter --
 *
 *    Return the callstack content of the currently active filter
 *
 * Results:
 *    Callstack content or NULL, if no filter is active
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
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
    if ((cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE) == 0) break;
  }
  if (framePtrPtr) {
    *framePtrPtr = varFramePtr;
  }

  return cscPtr;
}

/*
 *----------------------------------------------------------------------
 * CallStackMethodPath --
 *
 *    Return the method path of the current ensemble.
 *
 * Results:
 *    Tcl_Obj containing the method path
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static Tcl_Obj*
CallStackMethodPath(Tcl_Interp *interp, Tcl_CallFrame *framePtr, Tcl_Obj *methodPathObj) {
  int elements;
  Tcl_Obj *resultObj;

  assert(framePtr);
  /* 
   *  Append all ensemble names to the specified list obj 
   */
  for (framePtr = Tcl_CallFrame_callerPtr(framePtr), elements = 1;
       Tcl_CallFrame_isProcCallFrame(framePtr) & (FRAME_IS_NSF_CMETHOD|FRAME_IS_NSF_METHOD);
       framePtr = Tcl_CallFrame_callerPtr(framePtr), elements ++) {
    NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr);
    assert(cscPtr);
    Tcl_ListObjAppendElement(interp, methodPathObj, 
			     Tcl_NewStringObj(Tcl_GetCommandName(interp, cscPtr->cmdPtr), -1));
    if ((cscPtr->flags & NSF_CSC_TYPE_ENSEMBLE) == 0) break;
  }
  /* 
   *  The resulting list has reveresed order. If there are multiple
   *  arguments, reverse the list to obtain the right order.
   */

  if (elements > 1) {
    int oc, i;
    Tcl_Obj **ov;
    
    Tcl_ListObjGetElements(interp, methodPathObj, &oc, &ov);
    resultObj = Tcl_NewListObj(0, NULL);

    for (i = elements-1; i >= 0; i--) {
      Tcl_ListObjAppendElement(interp, resultObj, ov[i]);
    }
    DECR_REF_COUNT(methodPathObj);

  } else {
    resultObj = methodPathObj;
  }

  return resultObj;
}

/*
 *----------------------------------------------------------------------
 * CallStackMethodPath --
 *
 *    Check, if there is an active filter on "obj" using the specified
 *    cmd.
 *
 * Results:
 *    0 or 1
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
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

/*
 *----------------------------------------------------------------------
 * CallStackReplaceVarTableReferences --
 *
 *    Replace all references to the old var table (arg 1) by
 *    references to a new var table (arg 2) on the callstack.
 *    This function is e.g. used by require namespace.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Updated stack.
 *
 *----------------------------------------------------------------------
 */
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

/*
 *----------------------------------------------------------------------
 * CallStackClearCmdReferences --
 *
 *    Clear all references to the specified cmd in the callstack
 *    contents.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Updated stack.
 *
 *----------------------------------------------------------------------
 */
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

/*
 *----------------------------------------------------------------------
 * CallStackPopAll --
 *
 *    Unwind the stack and pop all callstack entries that are still
 *    alive (e.g.  if "exit" is called and we were jumping out of the
 *    callframe).
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Updated stack.
 *
 *----------------------------------------------------------------------
 */

static void CallStackPopAll(Tcl_Interp *interp) {

  if (RUNTIME_STATE(interp)->debugLevel > 1) {
    NsfShowStack(interp);
  }

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
      cscPtr->flags &= ~NSF_CSC_CALL_IS_NRE;
#endif
      CscFinish(interp, cscPtr, "popall");
    } else if (frameFlags & FRAME_IS_NSF_OBJECT) {
      Tcl_CallFrame_varTablePtr(framePtr) = NULL;
    }

    /* pop the Tcl frame */
    Tcl_PopCallFrame(interp);
  }

#if defined(CHECK_ACTIVATION_COUNTS)
  { int count = 0;
    NsfClasses *unstackedEntries;

    for (unstackedEntries = RUNTIME_STATE(interp)->cscList; 
	 unstackedEntries; 
	 unstackedEntries = unstackedEntries->nextPtr, count ++) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)unstackedEntries->cl;
      CscListRemove(interp, cscPtr);
      CscFinish(interp, cscPtr, "unwind");
    }
    
    if (count>0 && RUNTIME_STATE(interp)->debugLevel > 0) {
      fprintf(stderr, "+++ unwind removed %d unstacked csc entries\n", count);
    }
  }
#endif

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
    cscPtr->flags = NSF_CSC_CALL_IS_NRE;
  } else {
    cscPtr->flags = 0;
  }
#else
  cscPtr->flags = 0;
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
	Tcl_Command cmd, int frameType, int flags) {

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

    cscPtr->flags |= NSF_CSC_OBJECT_ACTIVATED;

    /*
     * track object activations
     */
    object->activationCount ++;
    /*fprintf(stderr, "CscInit %s method %s activationCount ++ (%s) --> %d\n",
	    msg, cmd ? Tcl_GetCommandName(object->teardown,cmd) : "UNK", 
	    objectName(object),  object->activationCount);*/
    /*
     * track class activations
     */
    if (cl && cmd) {
      Namespace *nsPtr = ((Command *)cmd)->nsPtr;
      cl->object.activationCount ++;
      /*fprintf(stderr, "CscInit %s %s activationCount %d cmd %s cmd ns %p (%s, refCount %d ++) "
	      "obj ns %p parent %p\n",
	      msg, className(cl), cl->object.activationCount,
	      Tcl_GetCommandName(object->teardown, cmd),
	      nsPtr, nsPtr->fullName, nsPtr->refCount,
	      cl->object.nsPtr,cl->object.nsPtr ? ((Namespace*)cl->object.nsPtr)->parentPtr : NULL);*/

      /* incremement the namespace ptr in case tcl tries to delete this namespace
	 during the invocation */
      nsPtr->refCount ++;
    }

  }
  cscPtr->flags        |= flags & NSF_CSC_COPY_FLAGS;
  cscPtr->self          = object;
  cscPtr->cl            = cl;
  cscPtr->cmdPtr        = cmd;
  cscPtr->objv          = NULL;
  cscPtr->filterStackEntry = object->filterStack;
  cscPtr->frameType     = frameType;


  /*fprintf(stderr, "CscInit %p (%s) object %p %s flags %.6x cmdPtr %p\n",cscPtr, msg,
    object, objectName(object), cscPtr->flags, cscPtr->cmdPtr);*/
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
CscFinish_(Tcl_Interp *interp, NsfCallStackContent *cscPtr) {
  int allowDestroy = RUNTIME_STATE(interp)->exitHandlerDestroyRound !=
    NSF_EXITHANDLER_ON_SOFT_DESTROY;
  NsfObject *object;
  int flags;

  assert(cscPtr);
  assert(cscPtr->self);

  object = cscPtr->self;
  flags = cscPtr->flags;

  /*fprintf(stderr, "CscFinish %p object %p %s flags %.6x cmdPtr %p\n",cscPtr,
    object, objectName(object), flags, cscPtr->cmdPtr);*/

  /*
   *  We cannot rely on the existence of cscPtr->cmdPtr (like in
   *  initialize), since the cmd might have been deleted during the
   *  activation.
   */
  if ((flags & NSF_CSC_OBJECT_ACTIVATED)) {
    /*
     * Tracking activations of objects
     */
    object->activationCount --;

    /*fprintf(stderr, "... activationCount -- (%s) --> %d\n",objectName(object),
      object->activationCount);*/

    /*fprintf(stderr, "decr activationCount for %s to %d object->flags %.6x dc %.6x succ %.6x\n",
	    objectName(cscPtr->self), cscPtr->self->activationCount, object->flags,
	    object->flags & NSF_DESTROY_CALLED,
	    object->flags & NSF_DESTROY_CALLED_SUCCESS
	    );*/
    assert(object->activationCount > -1);

    /*
    if (((object->flags & NSF_DESTROY_CALLED_SUCCESS)>0) !=
	((object->flags & NSF_DESTROY_CALLED)>0)) {
      fprintf(stderr, "*** flags differ for obj %p\n", object);
    }
    */

    if (object->activationCount < 1 && object->flags & NSF_DESTROY_CALLED_SUCCESS && allowDestroy) {
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

      /*fprintf(stderr, "CscFinish check ac %d flags destroy %.6x success %.6x\n",
	      object->activationCount,
	      object->flags & NSF_DESTROY_CALLED,
	      object->flags & NSF_DESTROY_CALLED_SUCCESS);*/

#if 0
      // TODO remove block
      if (((object->flags & NSF_DESTROY_CALLED_SUCCESS)>0) !=
	  ((object->flags & NSF_DESTROY_CALLED)>0)) {
	fprintf(stderr, "*** flags differ for class %p\n", object);
      }
#endif
      if (object->activationCount < 1 && object->flags & NSF_DESTROY_CALLED_SUCCESS && allowDestroy) {
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
  if ((cscPtr->flags & NSF_CSC_CALL_IS_NRE)) {
    NsfTclStackFree(interp, cscPtr, "CscFinish");
  }
#endif
  /*fprintf(stderr, "CscFinish done\n");*/
}


static Tcl_CallFrame *
BeginOfCallChain(Tcl_Interp *interp, NsfObject *object) {
  Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp), 
    *prevFramePtr = varFramePtr;

  fprintf(stderr, "BeginOfCallChain obj %s\n", objectName(object));
  if (object) {
    for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
      register int flags = Tcl_CallFrame_isProcCallFrame(varFramePtr);
      
      if (flags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
	NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
	if (cscPtr->self == object) {
	  prevFramePtr = varFramePtr;
	  continue;
	}
      } else if (flags & (FRAME_IS_NSF_OBJECT|FRAME_IS_LAMBDA)) {
	continue;
      }
      break;
    }
  }
  fprintf(stderr, "BeginOfCallChain returns %p\n", prevFramePtr);
  return prevFramePtr;
}
