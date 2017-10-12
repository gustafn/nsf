/*
 * nsfStack.c --
 *
 *      Stack handling functions of the Next Scripting Framework.
 *
 * Copyright (C) 2010-2016 Gustaf Neumann
 * Copyright (C) 2011-2017 Stefan Sobernig
 *
 * Vienna University of Economics and Business
 * Institute of Information Systems and New Media
 * A-1020, Welthandelsplatz 1
 * Vienna, Austria
 *
 * This work is licensed under the MIT License http://www.opensource.org/licenses/MIT
 *
 * Copyright:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifdef CHECK_ACTIVATION_COUNTS
static NsfClasses * NsfClassListUnlink(NsfClasses **firstPtrPtr, const void *key);

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
static int CscListRemove(const Tcl_Interp *interp, const NsfCallStackContent *cscPtr, NsfClasses **cscListPtr)
  nonnull(1) nonnull(2);
static void CscListAdd(const Tcl_Interp *interp, const NsfCallStackContent *cscPtr) nonnull(1) nonnull(2);

static void
CscListAdd(const Tcl_Interp *interp, const NsfCallStackContent *cscPtr) {

  nonnull_assert(interp != NULL);
  nonnull_assert(cscPtr != NULL);

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
CscListRemove(const Tcl_Interp *interp, const NsfCallStackContent *cscPtr, NsfClasses **cscListPtr) {
  NsfClasses *entryPtr, **cscList = &RUNTIME_STATE(interp)->cscList;

  nonnull_assert(interp != NULL);
  nonnull_assert(cscPtr != NULL);

  entryPtr = NsfClassListUnlink(cscList, cscPtr);
  if (entryPtr != NULL) {
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
 *    Print the contents of the call-stack to stderr. This function is
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

  nonnull_assert(interp != NULL);

  fprintf(stderr, "NsfShowStack framePtr %p varFramePtr %p\n",
          (void *)Tcl_Interp_framePtr(interp), (void *)Tcl_Interp_varFramePtr(interp));
  /* framePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
    for (; framePtr != NULL; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    fprintf(stderr, "... frame %p flags %.6x cd %p objv[0] %s\n",
            framePtr, Tcl_CallFrame_isProcCallFrame(framePtr),
            Tcl_CallFrame_clientData(framePtr),
            Tcl_CallFrame_objc(framePtr) ? ObjStr(Tcl_CallFrame_objv(framePtr)[0]) : "(null)");
            }*/
  framePtr = (Tcl_CallFrame *)Tcl_Interp_framePtr(interp);
  fprintf(stderr, "...         varFrame  flags       clientData lvl               ns\n");
  for (; framePtr != NULL; framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    unsigned int frameFlags = (unsigned int)Tcl_CallFrame_isProcCallFrame(framePtr);
    NsfCallStackContent *cscPtr =
      (frameFlags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) ?
      ((NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr)) : NULL;

    fprintf(stderr, "... %16p %.6x %16p %3d %16p %s ov %s %d",
            (void *)framePtr, frameFlags,
            Tcl_CallFrame_clientData(framePtr),
            Tcl_CallFrame_level(framePtr),
            (void *)Tcl_CallFrame_nsPtr(framePtr), Tcl_CallFrame_nsPtr(framePtr)->fullName,
            Tcl_CallFrame_objc(framePtr) > 0 ? ObjStr(Tcl_CallFrame_objv(framePtr)[0]) : "(null)",
            Tcl_CallFrame_objc(framePtr) > 0 ? Tcl_CallFrame_objc(framePtr) : -1);
    if (cscPtr != NULL) {
      fprintf(stderr, " csc %p frameType %.4x flags %.6x (%s.%p %s)\n",
              (void *)cscPtr,
              cscPtr->frameType,
              cscPtr->flags,
              ObjectName(cscPtr->self),
              (void *)cscPtr->cmdPtr,
              Tcl_GetCommandName(interp, cscPtr->cmdPtr));
    } else {
      fprintf(stderr, " no csc");
      if (frameFlags & FRAME_IS_NSF_OBJECT) {
        NsfObject *object = (NsfObject *)Tcl_CallFrame_clientData(framePtr);
        fprintf(stderr, " obj %p %s", (void *)object, ObjectName(object));
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
 *    Push or pop a frame with a call-stack content as an OBJECT
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
static void Nsf_PushFrameObj(Tcl_Interp *interp, NsfObject *object, const CallFrame *framePtr) nonnull(1) nonnull(2) nonnull(3);
static void Nsf_PopFrameObj(Tcl_Interp *interp, CallFrame *framePtr) nonnull(1) nonnull(2);

static void Nsf_PushFrameObj(Tcl_Interp *interp, NsfObject *object, const CallFrame *framePtr) {

  nonnull_assert(interp != NULL);
  nonnull_assert(object != NULL);
  nonnull_assert(framePtr != NULL);

  /*fprintf(stderr,"PUSH OBJECT_FRAME (Nsf_PushFrameObj) frame %p\n", framePtr);*/
  if (object->nsPtr != NULL) {
    Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, object->nsPtr,
                      0|FRAME_IS_NSF_OBJECT);
  } else {
    /* The object has no nsPtr, so we disguise as a proc, using fakeProc */
    Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, Tcl_CallFrame_nsPtr(Tcl_Interp_varFramePtr(interp)),
                      FRAME_IS_PROC|FRAME_IS_NSF_OBJECT);

    Tcl_CallFrame_procPtr(framePtr) = &RUNTIME_STATE(interp)->fakeProc;
    if (unlikely(object->varTablePtr == NULL)) {
      object->varTablePtr = VarHashTableCreate();
    }
    Tcl_CallFrame_varTablePtr(framePtr) = object->varTablePtr;
  }
  Tcl_CallFrame_clientData(framePtr) = (ClientData)object;
}


static void Nsf_PopFrameObj(Tcl_Interp *interp, CallFrame *framePtr) {

  nonnull_assert(interp != NULL);
  nonnull_assert(framePtr != NULL);

  /*fprintf(stderr,"POP  OBJECT_FRAME (Nsf_PopFrameObj) frame %p, varTable %p set to NULL, already %d\n",
    framePtr, Tcl_CallFrame_varTablePtr(framePtr), Tcl_CallFrame_varTablePtr(framePtr) == NULL);*/

  Tcl_CallFrame_varTablePtr(framePtr) = NULL;
  Tcl_PopCallFrame(interp);
}

/*
 *----------------------------------------------------------------------
 * Nsf_PushFrameCsc, Nsf_PopFrameCsc --
 *
 *    Push or pop a frame with a call-stack content as a CMETHOD
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
NSF_INLINE static void Nsf_PushFrameCsc(Tcl_Interp *interp, const NsfCallStackContent *cscPtr, CallFrame *framePtr)
  nonnull(1) nonnull(2) nonnull(3);
static void Nsf_PopFrameCsc(Tcl_Interp *interp, CallFrame *UNUSED(framePtr)) nonnull(1);

NSF_INLINE static void
Nsf_PushFrameCsc(Tcl_Interp *interp, const NsfCallStackContent *cscPtr, CallFrame *framePtr) {
  CallFrame *varFramePtr = Tcl_Interp_varFramePtr(interp);

  nonnull_assert(interp != NULL);
  nonnull_assert(cscPtr != NULL);
  nonnull_assert(framePtr != NULL);

  /*fprintf(stderr,"PUSH CMETHOD_FRAME (Nsf_PushFrameCsc) frame %p cscPtr %p methodName %s\n",
    framePtr, cscPtr, Tcl_GetCommandName(interp, cscPtr->cmdPtr));*/

  Tcl_PushCallFrame(interp, (Tcl_CallFrame *)framePtr, Tcl_CallFrame_nsPtr(varFramePtr),
                    FRAME_IS_PROC|FRAME_IS_NSF_CMETHOD);
  Tcl_CallFrame_clientData(framePtr) = (ClientData)cscPtr;
  Tcl_CallFrame_procPtr(framePtr) = &RUNTIME_STATE(interp)->fakeProc;
}

NSF_INLINE static void
Nsf_PopFrameCsc(Tcl_Interp *interp, CallFrame *UNUSED(framePtr)) {
  nonnull_assert(interp != NULL);

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
static Tcl_CallFrame * CallStackGetActiveProcFrame(Tcl_CallFrame *framePtr) nonnull(1);

static Tcl_CallFrame *
CallStackGetActiveProcFrame(Tcl_CallFrame *framePtr) {

  nonnull_assert(framePtr != NULL);

  do {
    register unsigned int flag = (unsigned int)Tcl_CallFrame_isProcCallFrame(framePtr);

    if ((flag & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0) {
      /* never return an inactive method frame */
      if (likely(!(((NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr))->frameType
                   & NSF_CSC_TYPE_INACTIVE))) {
        break;
      }
    } else {
      if (likely((flag & (FRAME_IS_NSF_OBJECT)) == 0u)) {
        if (flag == 0 || (flag & FRAME_IS_PROC) != 0) {
          break;
        }
      }
    }
    framePtr = Tcl_CallFrame_callerPtr(framePtr);
  } while (framePtr != NULL);

  return framePtr;
}

/*
 *----------------------------------------------------------------------
 * GetSelfObj, GetSelfObj2 --
 *
 *    Return the corresponding object from a method or from an object
 *    frame. GetSelfObj defaults to the top-most callframe, GetSelfObj2 allows
 *    one to set another frame.
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

#define GetSelfObj(interp) \
  GetSelfObj2((interp), (Tcl_CallFrame *)Tcl_Interp_varFramePtr((interp)))

#if 0
NSF_INLINE static NsfObject* GetSelfObj(const Tcl_Interp *interp) nonnull(1);

NSF_INLINE static NsfObject*
GetSelfObj(const Tcl_Interp *interp) {
  register Tcl_CallFrame *varFramePtr;

  nonnull_assert(interp != NULL);

  /*fprintf(stderr, "GetSelfObj interp has frame %p and var-frame %p\n",
    Tcl_Interp_framePtr(interp), Tcl_Interp_varFramePtr(interp));*/

  for (varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
       varFramePtr != NULL;
       varFramePtr =
#if defined(SKIP_LEVELS)
                        Tcl_CallFrame_callerPtr(varFramePtr)
#else
                        NULL
#endif
                        ) {
    register unsigned int flags;

    flags = (unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr);
    if (likely((flags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0u)) {
      return ((NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr))->self;

    } else if ((flags & FRAME_IS_NSF_OBJECT) != 0u) {
      return (NsfObject *)Tcl_CallFrame_clientData(varFramePtr);
      
    }
#if defined(SKIP_LAMBDA)
    if ((flags & FRAME_IS_LAMBDA) != 0u) {
      continue;
    }
    break;
#endif
  }
  return NULL;
}
#endif

NSF_INLINE static NsfObject* GetSelfObj2(const Tcl_Interp *interp, Tcl_CallFrame *framePtr) nonnull(1) nonnull(2);

NSF_INLINE static NsfObject*
GetSelfObj2(const Tcl_Interp *interp, Tcl_CallFrame *framePtr) {
  register Tcl_CallFrame *varFramePtr;

  nonnull_assert(interp != NULL);

  /*fprintf(stderr, "GetSelfObj interp has frame %p and var-frame %p\n",
    Tcl_Interp_framePtr(interp), Tcl_Interp_varFramePtr(interp));*/

  for (varFramePtr = framePtr;
       varFramePtr != NULL;
       varFramePtr =
#if defined(SKIP_LEVELS)
                        Tcl_CallFrame_callerPtr(varFramePtr)
#else
                        NULL
#endif
                        ) {
    register unsigned int flags;

    flags = (unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr);
    if (likely((flags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0u)) {
      return ((NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr))->self;

    } else if ((flags & FRAME_IS_NSF_OBJECT) != 0u) {
      return (NsfObject *)Tcl_CallFrame_clientData(varFramePtr);
      
    }
#if defined(SKIP_LAMBDA)
    if ((flags & FRAME_IS_LAMBDA) != 0u) {
      continue;
    }
    break;
#endif
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * CallStackGetTclFrame --
 *
 *    Return the Tcl_Callframe a (scripted or non-leaf) method starting with
 *    the specified or topmost frame; if skip is a positive number the
 *    specified number of Tcl frames are skipped.
 *
 * Results:
 *    Tcl_CallFrame or NULL.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static  Tcl_CallFrame* CallStackGetTclFrame(const Tcl_Interp *interp,
                                            Tcl_CallFrame *startFramePtr,
                                            int skip) nonnull(1);

static Tcl_CallFrame* CallStackGetTclFrame(const Tcl_Interp *interp,
                                           Tcl_CallFrame *varFramePtr,
                                           int skip) {
  nonnull_assert(interp != NULL);
  assert(skip >= 0);

  /* NsfShowStack(interp); */

  if (varFramePtr == NULL) {
    varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  }

  while(skip-- && varFramePtr != NULL) {
    varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr);
  }

  for (; varFramePtr != NULL; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (((unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0u) {
      return varFramePtr;
    }
  }

  return NULL;
}

/*
 *----------------------------------------------------------------------
 * CallStackGetTopFrame, CallStackGetTopFrame0, NsfCallStackGetTopFrame --
 *
 *    Return the NsfCallStackContent* of the topmost invocation of a (scripted
 *    or non-leaf) method. If framePtrPtr is provided, it is used to return the
 *    Tcl frame as well.
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
CallStackGetTopFrame(const Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) nonnull(1);

static NsfCallStackContent*
CallStackGetTopFrame(const Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr;

  nonnull_assert(interp != NULL);

  for (varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
       varFramePtr != NULL;
       varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {

    if (((unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0u) {
        if (framePtrPtr != NULL) {
          *framePtrPtr = varFramePtr;
        }
        return (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      }
  }
  if (framePtrPtr != NULL) {
    *framePtrPtr = NULL;
  }
  return NULL;
}

NSF_INLINE static NsfCallStackContent* CallStackGetTopFrame0(const Tcl_Interp *interp) nonnull(1);

NSF_INLINE static NsfCallStackContent*
CallStackGetTopFrame0(const Tcl_Interp *interp) {
  register Tcl_CallFrame *varFramePtr;
  NsfCallStackContent    *result = NULL;

  nonnull_assert(interp != NULL);

  for (varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
       varFramePtr != NULL;
       varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (likely(((unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0u)) {
      result = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      break;
    }
  }
  return result;
}

#if defined(NSF_PROFILE)
NsfCallStackContent* NsfCallStackGetTopFrame(const Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) nonnull(1);

NsfCallStackContent*
NsfCallStackGetTopFrame(const Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr) {
  return CallStackGetTopFrame(interp, framePtrPtr);
}
#endif

/*
 *----------------------------------------------------------------------
 * NsfCallStackFindLastInvocation --
 *
 *    Find last invocation of a (scripted or non-leaf) method with a
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
static NsfCallStackContent *NsfCallStackFindLastInvocation(const Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr)
  nonnull(1);

static NsfCallStackContent *
NsfCallStackFindLastInvocation(const Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
  int lvl = Tcl_CallFrame_level(varFramePtr);

  nonnull_assert(interp != NULL);

  for (; likely(varFramePtr != NULL); varFramePtr = Tcl_CallFrame_callerVarPtr(varFramePtr)) {

    if (((unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0u) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);

      /*
       * A NSF method frame.
       */
      if ((cscPtr->flags & (NSF_CSC_CALL_IS_NEXT|NSF_CSC_CALL_IS_ENSEMBLE))
          || (cscPtr->frameType & NSF_CSC_TYPE_INACTIVE)) {
        continue;
      }

      if (offset != 0) {
        offset--;
      } else if (Tcl_CallFrame_level(varFramePtr) < lvl) {
        if (framePtrPtr != NULL) {
          *framePtrPtr = varFramePtr;
        }
        return cscPtr;
      }
    } else if (Tcl_CallFrame_isProcCallFrame(varFramePtr)) {

      /*
       * A Tcl proc frame.
       */
      if (offset != 0) {
        offset--;
      } else if (Tcl_CallFrame_level(varFramePtr) < lvl) {
        if (framePtrPtr != NULL) {
          *framePtrPtr = varFramePtr;
        }
        return NULL;
      }
    }
  }

  if (framePtrPtr != NULL) {
    *framePtrPtr = NULL;
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * NsfCallStackFindActiveFrame --
 *
 *    Search for the first active frame on the call-stack.
 *
 * Results:
 *    Call stack content or NULL.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static NsfCallStackContent *NsfCallStackFindActiveFrame(const Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) nonnull(1);

static NsfCallStackContent *
NsfCallStackFindActiveFrame(const Tcl_Interp *interp, int offset, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr;

  nonnull_assert(interp != NULL);

  /* skip #offset frames */
  for (varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);
       (offset > 0) && (varFramePtr != NULL);
       varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr), offset--);

  /* search for first active frame and set Tcl frame pointers */
  for (; varFramePtr != NULL; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (((unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0u) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
      if (!(cscPtr->frameType & NSF_CSC_TYPE_INACTIVE)) {
        /* we found the highest active frame */
        if (framePtrPtr != NULL) {
          *framePtrPtr = varFramePtr;
        }
        return cscPtr;
      }
    }
  }
  /* we could not find an active frame; called from toplevel? */
  if (framePtrPtr != NULL) {
    *framePtrPtr = NULL;
  }
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
static void CallStackUseActiveFrame(const Tcl_Interp *interp, callFrameContext *ctx) nonnull(1) nonnull(2);

static void
CallStackUseActiveFrame(const Tcl_Interp *interp, callFrameContext *ctx) {
  Tcl_CallFrame *framePtr, *inFramePtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(ctx != NULL);

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
 *    Restore the previously saved frames from the specified call
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
static void CallStackRestoreSavedFrames(Tcl_Interp *interp, callFrameContext *ctx) nonnull(1) nonnull(2);

static void
CallStackRestoreSavedFrames(Tcl_Interp *interp, callFrameContext *ctx) {

  nonnull_assert(interp != NULL);
  nonnull_assert(ctx != NULL);

  if (ctx->frameSaved != 0) {
    /*fprintf(stderr, "CallStackRestoreSavedFrames drops %p restores %p\n",
      Tcl_Interp_varFramePtr(interp), ctx->varFramePtr);*/
    Tcl_Interp_varFramePtr(interp) = (CallFrame *)ctx->varFramePtr;
  }
}

/*
 *----------------------------------------------------------------------
 * CallStackFindActiveFilter --
 *
 *    Return the call-stack content of the currently active filter
 *
 * Results:
 *    Call-stack content or NULL, if no filter is active
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static NsfCallStackContent * CallStackFindActiveFilter(const Tcl_Interp *interp) nonnull(1);

static NsfCallStackContent *
CallStackFindActiveFilter(const Tcl_Interp *interp) {
  register Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  nonnull_assert(interp != NULL);

  for (; varFramePtr != NULL; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (((unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0u) {
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
 *    Return the call-stack content and the optionally the stack frame
 *    of the last ensemble invocation.
 *
 * Results:
 *    Call-stack content
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static NsfCallStackContent * CallStackFindEnsembleCsc(const Tcl_CallFrame *framePtr, Tcl_CallFrame **framePtrPtr) nonnull(1) nonnull(2);

static NsfCallStackContent *
CallStackFindEnsembleCsc(const Tcl_CallFrame *framePtr, Tcl_CallFrame **framePtrPtr) {
  register Tcl_CallFrame *varFramePtr;
  NsfCallStackContent *cscPtr = NULL;

  nonnull_assert(framePtr != NULL);
  nonnull_assert(framePtrPtr != NULL);

  for (/* Skipping the starting frame, assuming a "leaf" frame in an ensemble dispatch */
       varFramePtr = Tcl_CallFrame_callerPtr(framePtr);
       ((unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr) & FRAME_IS_NSF_CMETHOD) != 0u;
       varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
    assert(cscPtr != NULL);

    /*fprintf(stderr,"	--- frame %p cmdPtr %p NSF_CSC_TYPE_ENSEMBLE %d NSF_CSC_CALL_IS_ENSEMBLE %d \
                         NSF_CSC_TYPE_INACTIVE %d\n",
            varFramePtr,
            cscPtr->cmdPtr,
            (cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) != 0,
            (cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE) != 0,
            (cscPtr->frameType & NSF_CSC_TYPE_INACTIVE) != 0);*/
    /*
     * The "root" frame in a call-stack branch resulting from an ensemble
     * dispatch is not typed as an NSF_CSC_TYPE_ENSEMBLE frame, the call type
     * /is/ NSF_CSC_CALL_IS_ENSEMBLE.
     */
    if ((cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) == 0u &&
        (cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE)) {
      break;
    }
  }

  *framePtrPtr = varFramePtr;

  return cscPtr;
}

/*
 *----------------------------------------------------------------------
 * CallStackNextFrameOfType --
 *
 *    Return the next frame on the call stack being of a specified type. The
 *    type is specified by a bitmask passed as flags.
 *
 * Results:
 *    Tcl_CallFrame
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_CallFrame * CallStackNextFrameOfType(Tcl_CallFrame *framePtr, unsigned int flags) nonnull(1);

static Tcl_CallFrame *
CallStackNextFrameOfType(Tcl_CallFrame *framePtr, unsigned int flags) {

  nonnull_assert(framePtr != NULL);

  do {
    NsfCallStackContent *cscPtr = Tcl_CallFrame_clientData(framePtr);

    if (cscPtr != NULL && (cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) != 0u) {
      (void)CallStackFindEnsembleCsc(framePtr, &framePtr);
    }

    if (((unsigned int)Tcl_CallFrame_isProcCallFrame(framePtr) & flags) != 0u) {
      /*
       * framePtr has already the return value.
       */
      break;
    }

    framePtr = Tcl_CallFrame_callerPtr(framePtr);

  } while (framePtr != NULL);

  return framePtr;
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
static Tcl_Obj* CallStackMethodPath(Tcl_Interp *interp, Tcl_CallFrame *framePtr)
  nonnull(1) nonnull(2);

static Tcl_Obj*
CallStackMethodPath(Tcl_Interp *interp, Tcl_CallFrame *framePtr) {
  int      elements;
  Tcl_Obj *resultObj, *methodPathObj = Tcl_NewListObj(0, NULL);

  nonnull_assert(interp != NULL);
  nonnull_assert(framePtr != NULL);

  /*
   * Append all ensemble names to the specified list obj
   */
  for (elements = 0;
       ((unsigned int)Tcl_CallFrame_isProcCallFrame(framePtr) & (FRAME_IS_NSF_CMETHOD|FRAME_IS_NSF_METHOD)) != 0u;
       framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    const NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(framePtr);

    assert(cscPtr != NULL);

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
    if ((cscPtr->flags & NSF_CSC_CALL_IS_ENSEMBLE) == 0u) {
      break;
    }
    /*
     * The call-stack might contain consecutive calls of ensemble entry calls
     * chained via next. We can detect consecutive calls via the elements
     * count.
     */
    if (elements == 0 && (cscPtr->flags & NSF_CM_ENSEMBLE_UNKNOWN) && (cscPtr->flags & NSF_CSC_CALL_IS_NEXT)) {
      break;
    }

    Tcl_ListObjAppendElement(interp, methodPathObj,
                             Tcl_NewStringObj(Tcl_GetCommandName(interp, cscPtr->cmdPtr), -1));
    elements++;

    /*
     * The "root" frame in a call-stack branch resulting from an ensemble
     * dispatch is not typed as an NSF_CSC_TYPE_ENSEMBLE frame, the call type
     * /is/ NSF_CSC_CALL_IS_ENSEMBLE (as checked above).
     */

    if ((cscPtr->frameType & NSF_CSC_TYPE_ENSEMBLE) == 0u) {
      break;
    }

  }
  /*
   *  The resulting list has reversed order. If there are multiple
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

  /*fprintf(stderr, "--- CallStackMethodPath returns %s\n", ObjStr(resultObj));*/

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
NSF_INLINE static int FilterActiveOnObj(const Tcl_Interp *interp, const NsfObject *object, Tcl_Command cmd) nonnull(1) nonnull(2);

NSF_INLINE static int
FilterActiveOnObj(const Tcl_Interp *interp, const NsfObject *object, Tcl_Command cmd) {
  register const Tcl_CallFrame *varFramePtr = (const Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp);

  nonnull_assert(interp != NULL);
  nonnull_assert(object != NULL);

  for (; varFramePtr != NULL; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
    if (((unsigned int)Tcl_CallFrame_isProcCallFrame(varFramePtr) & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) != 0u) {
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
 *    references to a new var table (arg 2) on the call-stack.
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
static void CallStackReplaceVarTableReferences(const Tcl_Interp *interp, TclVarHashTable *oldVarTablePtr, TclVarHashTable *newVarTablePtr) nonnull(1) nonnull(2) nonnull(3);

static void
CallStackReplaceVarTableReferences(const Tcl_Interp *interp, TclVarHashTable *oldVarTablePtr, TclVarHashTable *newVarTablePtr) {
  Tcl_CallFrame *framePtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(oldVarTablePtr != NULL);
  nonnull_assert(newVarTablePtr != NULL);

  for (framePtr = (Tcl_CallFrame *)Tcl_Interp_framePtr(interp);
       framePtr != NULL;
       framePtr = Tcl_CallFrame_callerPtr(framePtr)) {
    unsigned int frameFlags = (unsigned int)Tcl_CallFrame_isProcCallFrame(framePtr);

    if ((frameFlags & FRAME_IS_NSF_OBJECT) == 0u) {
      continue;
    }
    if (!(Tcl_CallFrame_varTablePtr(framePtr) == oldVarTablePtr)) {
      continue;
    }

    /*fprintf(stderr, "+++ makeObjNamespace replacing varTable %p with %p in frame %p\n",
      oldVarTablePtr, newVarTablePtr, framePtr);*/
    Tcl_CallFrame_varTablePtr(framePtr) = newVarTablePtr;
  }
}

/*
 *----------------------------------------------------------------------
 * CallStackPopAll --
 *
 *    Unwind the stack and pop all call-stack entries that are still
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

  nonnull_assert(interp != NULL);

  if (RUNTIME_STATE(interp)->logSeverity == NSF_LOG_DEBUG) {
    NsfShowStack(interp);
  }

  while (1) {
    const Tcl_CallFrame *framePtr = Tcl_Interp_framePtr(interp);
    unsigned int frameFlags;

    if (framePtr == NULL || (Tcl_CallFrame_level(framePtr) == 0)) {
      break;
    }

    frameFlags = (unsigned int)Tcl_CallFrame_isProcCallFrame(framePtr);
    /*fprintf(stderr, "--- popping %p frame-flags %.6x\n", framePtr, frameFlags);*/

    if ((frameFlags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD))) {
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

    while (nextCscPtr != NULL) {
      NsfCallStackContent *cscPtr = (NsfCallStackContent *)nextCscPtr->cl;
      CscListRemove(interp, cscPtr, &unstackedEntries);
      CscFinish(interp, cscPtr, TCL_OK, "unwind");

      count ++;
      nextCscPtr = (unstackedEntries != NULL) ? unstackedEntries->nextPtr : NULL;
    }

    if (count>0 && RUNTIME_STATE(interp)->logSeverity > 0) {
      fprintf(stderr, "+++ unwind removed %d unstacked csc entries\n", count);
    }
  }
#endif

}

/*
 *----------------------------------------------------------------------
 * CscAlloc --
 *
 *    Allocate the CSC structure either from the stack or via
 *    StackAlloc (the latter is recorded in the callType). The Alloc
 *    operation requires a CscFinish operation later.
 *
 * Results:
 *    A valid, semi-initialized cscPtr.
 *
 * Side effects:
 *    Memory allocation
 *
 *----------------------------------------------------------------------
 */
#if defined(NRE)
static NsfCallStackContent * CscAlloc(Tcl_Interp *interp, NsfCallStackContent *cscPtr, Tcl_Command cmd)
  nonnull(1);
#else
static NsfCallStackContent * CscAlloc(Tcl_Interp *interp, NsfCallStackContent *cscPtr, Tcl_Command cmd)
  nonnull(2);
#endif

static NsfCallStackContent *
CscAlloc(Tcl_Interp *interp, NsfCallStackContent *cscPtr, Tcl_Command cmd) {
#if defined(NRE)
  Tcl_ObjCmdProc *proc = (cmd != NULL) ? Tcl_Command_objProc(cmd) : NULL;

  if (proc == TclObjInterpProc) {
    cscPtr = (NsfCallStackContent *) NsfTclStackAlloc(interp, sizeof(NsfCallStackContent), "csc");
    cscPtr->flags = NSF_CSC_CALL_IS_NRE;
  } else {
    cscPtr->flags = 0;
  }
#else
  nonnull_assert(cscPtr != NULL);
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
NSF_INLINE static void CscInit_(/*@notnull@*/ NsfCallStackContent *cscPtr, NsfObject *object, NsfClass *cl,
        Tcl_Command cmd, unsigned short frameType, unsigned int flags) nonnull(1) nonnull(2);

NSF_INLINE static void
CscInit_(/*@notnull@*/ NsfCallStackContent *cscPtr, NsfObject *object, NsfClass *cl,
        Tcl_Command cmd, unsigned short frameType, unsigned int flags) {
#if defined(NSF_PROFILE)
  struct Tcl_Time trt;
#endif

  nonnull_assert(cscPtr != NULL);
  nonnull_assert(object != NULL);

#if defined(NSF_PROFILE)
  Tcl_GetTime(&trt);

  cscPtr->startUsec = trt.usec;
  cscPtr->startSec = trt.sec;
#endif

  /*
   *  When cmd is provided, the call is not unknown, the method
   *  will be executed and the object will be stacked. In these
   *  cases, we maintain an activation count.
   */
  if (likely(cmd != NULL)) {
    /*
     * Track object activations
     */
    object->activationCount ++;
    MEM_COUNT_ALLOC("object.activationCount",object);
    /*fprintf(stderr, "CscInit %p method %s activationCount ++ (%s) --> %d (cl %p)\n",
            cscPtr, (cmd != NULL) ? Tcl_GetCommandName(object->teardown, cmd) : "UNK",
            ObjectName(object),  object->activationCount, cl);*/
    /*
     * Track class activations
     */
    if (cl != NULL) {
      /*
       * handle class activation count
       */
      cl->object.activationCount ++;
      MEM_COUNT_ALLOC("class.activationCount", cl);
      /*
       * Increment the namespace ptr in case Tcl tries to delete
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
NSF_INLINE static void CscFinish_(Tcl_Interp *interp, NsfCallStackContent *cscPtr) nonnull(1) nonnull(2);

NSF_INLINE static void
CscFinish_(Tcl_Interp *interp, NsfCallStackContent *cscPtr) {
  NsfObject *object;

  nonnull_assert(interp != NULL);
  nonnull_assert(cscPtr != NULL);
  assert(cscPtr->self != NULL);

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
  if (likely(cscPtr->cmdPtr != NULL)) {
    int allowDestroy = RUNTIME_STATE(interp)->exitHandlerDestroyRound ==
      NSF_EXITHANDLER_OFF;

    if ((Tcl_Command_flags(cscPtr->cmdPtr) & NSF_CMD_DEBUG_METHOD) != 0) {
#if defined(NSF_PROFILE) || defined(NSF_DTRACE)
      NsfProfileDebugExit(interp, cscPtr->self, cscPtr->cl, cscPtr->methodName,
                          cscPtr->startSec, cscPtr->startUsec);
#else
      NsfProfileDebugExit(interp, cscPtr->self, cscPtr->cl,
                          Tcl_GetCommandName(interp, cscPtr->cmdPtr), 0, 0);
#endif
    }

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
    if (unlikely(cscPtr->cl != NULL)) {
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
static Tcl_CallFrame * BeginOfCallChain(const Tcl_Interp *interp, NsfObject *object) nonnull(1);

static Tcl_CallFrame *
BeginOfCallChain(const Tcl_Interp *interp, NsfObject *object) {
  Tcl_CallFrame *varFramePtr = (Tcl_CallFrame *)Tcl_Interp_varFramePtr(interp),
    *prevFramePtr = varFramePtr;

  nonnull_assert(interp != NULL);

  if (object != NULL) {
    fprintf(stderr, "BeginOfCallChain obj %s\n", ObjectName(object));
    for (; varFramePtr != NULL; varFramePtr = Tcl_CallFrame_callerPtr(varFramePtr)) {
      register unsigned int flags = Tcl_CallFrame_isProcCallFrame(varFramePtr);

      if (flags & (FRAME_IS_NSF_METHOD|FRAME_IS_NSF_CMETHOD)) {
        const NsfCallStackContent *cscPtr = (NsfCallStackContent *)Tcl_CallFrame_clientData(varFramePtr);
        if (cscPtr->self == object) {
          prevFramePtr = varFramePtr;
          continue;
        }
      } else if ((flags & (FRAME_IS_NSF_OBJECT|FRAME_IS_LAMBDA)) != 0u) {
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
 * indent-tabs-mode: nil
 * End:
 */
