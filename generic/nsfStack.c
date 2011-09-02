
#ifdef CHECK_ACTIVATION_COUNTS
static NsfClasses * NsfClassListUnlink(NsfClasses **firstPtrPtr, void *key);

/*
 *----------------------------------------------------------------------
 * CscListAdd --
 *
 *    Add an entry to the list of unstacked CSC entries.
 *
 * Results:
 *    none
 *
 * Side effects:
 *    List element added
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
 *    Removes an entry from the list of unstacked CSC entries.
 *
 * Results:
 *    true on success or 0
 *
 * Side effects: 
 *    
 *    List element potentially removed and freed. If a list turns
 *    empty, the interp's state is updated.
 *
 *----------------------------------------------------------------------
 */
static int
CscListRemove(Tcl_Interp *interp, NsfCallStackContent *cscPtr, NsfClasses **cscListPtr) {
  NsfClasses *entryPtr, **cscList = &RUNTIME_STATE(interp)->cscList;
  entryPtr = NsfClassListUnlink(cscList, cscPtr);
  if (entryPtr) {
    FREE(NsfClasses, entryPtr);
  }
  if (cscListPtr != NULL) {
    *cscListPtr = *cscList;
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
  fprintf(stderr, "...         varFrame  flags       clientData lvl               ns\n");
  for (; framePtr; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    int frameFlags = Tcl_CallFrame_isProcCallFrame(framePtr);
    NsfCallStackContent *cscPtr =
      (frameFlags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) ?
      ((NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr)) : NULL;

    fprintf(stderr, "... %16p %.6x %16p %3d %16p %s ov %s %d",
            framePtr, frameFlags,
            Tcl_CallFrame_clientData(framePtr),
            Tcl_CallFrame_level(framePtr),
            Tcl_CallFrame_nsPtr(framePtr), Tcl_CallFrame_nsPtr(framePtr)->fullName,
            Tcl_CallFrame_objc(framePtr) && 0 ? ObjStr(Tcl_CallFrame_objv(framePtr)[0]) : "(null)",
            Tcl_CallFrame_objc(framePtr) ? Tcl_CallFrame_objc(framePtr) : -1);
    if (cscPtr) {
      fprintf(stderr, " csc %p frameType %.4x flags %.6x (%s.%p %s)\n",
	      cscPtr,
              cscPtr ? cscPtr->frameType : -1,
              cscPtr ? cscPtr->flags : -1,
              cscPtr ? ObjectName(cscPtr->self) : "",
              cscPtr ? cscPtr->cmdPtr : NULL,
              cscPtr ? Tcl_GetCommandName(interp, cscPtr->cmdPtr) : ""
	      );
    } else {
      fprintf(stderr, " no csc");
      if (frameFlags & FRAME_IS_NSF_OBJECT) {
        NsfObject *object = (NsfObject *)Tcl_CallFrame_clientData(framePtr);
        fprintf(stderr, " obj %p %s", object, ObjectName(object));
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
 *    Push or pop a frame with a callstack content as an OBJECT
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
  /*fprintf(stderr,"PUSH OBJECT_FRAME (Nsf_PushFrameObj) frame %p\n", framePtr);*/
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

NSF_INLINE static void 
Nsf_PushFrameCsc(Tcl_Interp *interp, NsfCallStackContent *cscPtr, CallFrame *framePtr) {
  CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);
  /*fprintf(stderr,"PUSH CMETHOD_FRAME (Nsf_PushFrameCsc) frame %p cscPtr %p methodName %s\n",
    framePtr, cscPtr, Tcl_GetCommandName(interp, cscPtr->cmdPtr));*/

  Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, Tcl_CallFrame_nsPtr(varFramePtr),
		    FRAME_IS_PROC|FRAME_IS_NSF_CMETHOD);
  Tcl_CallFrame_clientData(framePtr) = (ClientData)cscPtr;
  Tcl_CallFrame_procPtr(framePtr) = &RUNTIME_STATE(interp)->fakeProc;
}

NSF_INLINE static void 
Nsf_PopFrameCsc(Tcl_Interp *interp, CallFrame *UNUSED(framePtr)) {
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
    Tcl_Interp_framePtr(interp), Tcl_Interp_varFramePtr(interp));*/

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
 * CallStackGetTopFrame, CallStackGetTopFrame0, NsfCallStackGetTopFrame --
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

NSF_INLINE static NsfCallStackContent*
CallStackGetTopFrame0(Tcl_Interp *interp) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  for (; varFramePtr; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
      if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
        return (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      }
  }
  return NULL;
}

#if defined(NSF_PROFILE)
NsfCallStackContent*
NsfCallStackGetTopFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
  return CallStackGetTopFrame(interp, framePtrPtr);
}
#endif

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
    if (Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
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
    /*fprintf(stderr, "CallStackUseActiveFrame stores %p\n", framePtr);*/
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
  for (/* Skipping the starting frame, assumingly a "leaf" frame in an ensemle dispatch */
       varFramePtr = Tcl_CallFrame_callerPtr(framePtr);
       Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_NSF_CMETHOD; 
       varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
    assert(cscPtr);
    
    /*fprintf(stderr,"	--- frame %p cmdPtr %p NSF_CSC_TYPE_ENSEMBLE %d NSF_CSC_CALL_IS_ENSEMBLE %d \
			 NSF_CSC_TYPE_INACTIVE %d\n",
	    varFramePtr,
	    cscPtr->cmdPtr,
	    (cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) != 0,
	    (cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE) != 0,
	    (cscPtr->frameType & NSF_CSC_TYPE_INACTIVE) != 0);*/
    /*
     * The "root" frame in a callstack branch resulting from an ensemble
     * dispatch is not typed as an NSF_CSC_TYPE_ENSEMBLE frame, the call type
     * /is/ NSF_CSC_CALL_IS_ENSEMBLE.
     */
    if ((cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) == 0 && 
	(cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE)) break;
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
 *    Return the method path of the current ensemble in a Tcl_Obj with
 *    refCount 0.
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
CallStackMethodPath(Tcl_Interp *interp, Tcl_CallFrame *framePtr) {
  int elements;
  Tcl_Obj *resultObj;
  Tcl_Obj *methodPathObj = Tcl_NewListObj(0, NULL);

  assert(framePtr);
  /* 
   * Append all ensemble names to the specified list obj 
   */

  for (/* Skipping the starting frame, assumingly a "leaf" frame in an ensemle dispatch */ 
       framePtr = Tcl_CallFrame_callerPtr(framePtr), elements = 0;
       Tcl_CallFrame_isProcCallFrame(framePtr) & (FRAME_IS_NSF_CMETHOD|FRAME_IS_NSF_METHOD);
       framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    
    NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr);
    assert(cscPtr);
    
    /*fprintf(stderr,	"--- frame %p cmdPtr %p cmd %s NSF_CSC_TYPE_ENSEMBLE %d \
			NSF_CSC_CALL_IS_ENSEMBLE %d NSF_CSC_TYPE_INACTIVE %d\n",
	    framePtr,
	    cscPtr->cmdPtr,
	    Tcl_GetCommandName(interp, cscPtr->cmdPtr),
	    (cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) != 0,
	    (cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE) != 0,
	    (cscPtr->frameType & NSF_CSC_TYPE_INACTIVE) != 0);*/

    /* 
     * The "ensemble" call type, we find applied to all intermediate and leaf
     * ensemble frames. By filtering according to the ensemble call type, we
     * effectively omit leaf ensemble and non-ensemble frames from being
     * reported.
     */
    if ((cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE) == 0) break;

    /* Do not record any INACTIVE frames in the method path */
    if ((cscPtr->frameType & NSF_CSC_TYPE_INACTIVE)) continue;

    Tcl_ListObjAppendElement(interp, methodPathObj, 
			     Tcl_NewStringObj(Tcl_GetCommandName(interp, cscPtr->cmdPtr), -1));
    elements++;
    
    /*
     * The "root" frame in a callstack branch resulting from an ensemble
     * dispatch is not typed as an NSF_CSC_TYPE_ENSEMBLE frame, the call type
     * /is/ NSF_CSC_CALL_IS_ENSEMBLE (as checked above).
     */
    
    if ((cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) == 0) break;
    
  }
  /* 
   *  The resulting list has reveresed order. If there are multiple
   *  arguments, reverse the list to obtain the right order.
   */
  if (elements > 1) {
    int oc, i;
    Tcl_Obj **ov;
    
    INCR_REF_COUNT(methodPathObj);
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
 * FilterActiveOnObj --
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
      CscFinish(interp, cscPtr, TCL_OK, "popall");
    } else if (frameFlags & FRAME_IS_NSF_OBJECT) {
      Tcl_CallFrame_varTablePtr(framePtr) = NULL;
    }

    /* pop the Tcl frame */
    Tcl_PopCallFrame(interp);
  }

#if defined(CHECK_ACTIVATION_COUNTS)
  { int count = 0;
    NsfClasses *unstackedEntries = RUNTIME_STATE(interp)->cscList, *nextCscPtr = unstackedEntries;

    while (nextCscPtr) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)nextCscPtr->cl;
      CscListRemove(interp, cscPtr, &unstackedEntries);
      CscFinish(interp, cscPtr, TCL_OK, "unwind");
      
      count ++;
      nextCscPtr = unstackedEntries ? unstackedEntries->nextPtr : NULL; 
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
  (void)interp;
  (void)cmd;
  cscPtr->flags = 0;
#endif

  /*fprintf(stderr, "CscAlloc allocated %p\n", cscPtr);*/
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
CscInit_(/*@notnull@*/ NsfCallStackContent *cscPtr, NsfObject *object, NsfClass *cl,
	Tcl_Command cmd, int frameType, int flags) {
#if defined(NSF_PROFILE)
  struct timeval trt;
#endif

  assert(cscPtr);
  
#if defined(NSF_PROFILE)
  gettimeofday(&trt, NULL);
  
  cscPtr->startUsec = trt.tv_usec;
  cscPtr->startSec = trt.tv_sec;
#endif

  /*
   *  When cmd is provided, the call is not an unknown, the method
   *  will be executed and the object will be stacked. In these
   *  cases, we maintain an activation count. 
   */
  if (cmd) {
    /*
     * Track object activations
     */
    object->activationCount ++;
    MEM_COUNT_ALLOC("object.activationCount",object);
    /*fprintf(stderr, "CscInit %p method %s activationCount ++ (%s) --> %d (cl %p)\n",
	    cscPtr, cmd ? Tcl_GetCommandName(object->teardown, cmd) : "UNK", 
	    ObjectName(object),  object->activationCount, cl);*/
    /*
     * Track class activations
     */
    if (cl) {
      /*
       * handle class activation count
       */
      cl->object.activationCount ++;
      MEM_COUNT_ALLOC("class.activationCount", cl);
      /* 
       * Incremement the namespace ptr in case Tcl tries to delete
       * this namespace during the invocation
       */
      NSNamespacePreserve(Tcl_Command_nsPtr(cmd));
      /*fprintf(stderr, "NSNamespacePreserve %p\n", nsPtr);*/
    }

    NsfCommandPreserve(cmd);
  }
  cscPtr->flags        |= flags & NSF_CSC_COPY_FLAGS;
  cscPtr->self          = object;
  cscPtr->cl            = cl;
  cscPtr->cmdPtr        = cmd;
  cscPtr->objv          = NULL;
  cscPtr->filterStackEntry = object->filterStack;
  cscPtr->frameType     = frameType;

  /*fprintf(stderr, "CscInit %p (%s) object %p %s flags %.6x cmdPtr %p\n", cscPtr, msg,
    object, ObjectName(object), cscPtr->flags, cscPtr->cmdPtr);*/
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

  assert(cscPtr);
  assert(cscPtr->self);

#if defined(NSF_PROFILE)
  if (RUNTIME_STATE(interp)->doProfile) {
    NsfProfileRecordMethodData(interp, cscPtr);
  }
#endif

  object = cscPtr->self;

  /*fprintf(stderr, "CscFinish %p object %p %s flags %.6x cmdPtr %p\n", cscPtr,
    object, ObjectName(object), cscPtr->flags, cscPtr->cmdPtr); */

  /*
   *  In the cases, where an cmd was provided, we tracked in init the
   *  activations. Release these activations now. Notem, that
   *  cscPtr->cmdPtr might have been epoched, but it is still
   *  available, since we used NsfCommandPreserve() in CscInit().
   */
  if (cscPtr->cmdPtr) {
    /*
     * Track object activations
     */
    object->activationCount --;
    MEM_COUNT_FREE("object.activationCount", object);

    /*fprintf(stderr, "CscFinish decr activationCount for %s to %d object->flags %.6x dc %.6x succ %.6x\n",
	    ObjectName(cscPtr->self), cscPtr->self->activationCount, object->flags,
	    object->flags & NSF_DESTROY_CALLED,
	    object->flags & NSF_DESTROY_CALLED_SUCCESS
	    );*/

    assert(object->activationCount > -1);

    if (object->activationCount < 1 && (object->flags & NSF_DESTROY_CALLED) && allowDestroy) {
      /*fprintf(stderr, "CscFinish calls destroy object %p\n", object);*/
      CallStackDoDestroy(interp, object);
    }

    /*
     * Track class activations
     */
    if (cscPtr->cl) {
      NsfObject *clObject = &cscPtr->cl->object;
      clObject->activationCount --;
      MEM_COUNT_FREE("class.activationCount", clObject);
      
      /*fprintf(stderr, "CscFinish class %p %s check ac %d flags destroy %.6x success %.6x\n",
	      clObject, ObjectName(clObject),
	      clObject->activationCount,
	      clObject->flags & NSF_DESTROY_CALLED,
	      clObject->flags & NSF_DESTROY_CALLED_SUCCESS);*/

      if (clObject->activationCount < 1 && clObject->flags & NSF_DESTROY_CALLED && allowDestroy) {
	/* fprintf(stderr, "CscFinish calls destroy class %p\n", clObject);*/
	CallStackDoDestroy(interp, clObject);
      }
      /*
       * Release the Namespace
       */
      NSNamespaceRelease(Tcl_Command_nsPtr(cscPtr->cmdPtr));
    }
    /*
     * Release the Command
     */
    NsfCommandRelease(cscPtr->cmdPtr);
  }

#if defined(NRE)
  if ((cscPtr->flags & NSF_CSC_CALL_IS_NRE)) {
    NsfTclStackFree(interp, cscPtr, "CscFinish");
  }
#endif
  /*fprintf(stderr, "CscFinish done\n");*/
}

/*
 *----------------------------------------------------------------------
 * BeginOfCallChain --
 *
 *    Experimental function to track the begin of a call chain.
 *    Currently not used.
 *
 * Results:
 *    Callframe ptr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
#if 0
static Tcl_CallFrame *
BeginOfCallChain(Tcl_Interp *interp, NsfObject *object) {
  Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp), 
    *prevFramePtr = varFramePtr;

  if (object) {
    fprintf(stderr, "BeginOfCallChain obj %s\n", ObjectName(object));
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
#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
