/*
 * nsfObj.c --
 *
 *      Tcl_Obj types provided by the Next Scripting Framework.
 *
 * Copyright (C) 1999-2017 Gustaf Neumann
 * Copyright (C) 2016 Stefan Sobernig
 *
 * Vienna University of Economics and Business
 * Institute of Information Systems and New Media
 * A-1020, Welthandelsplatz 1
 * Vienna, Austria
 *
 * This work is licensed under the MIT License https://www.opensource.org/licenses/MIT
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
 *
 */

#include "nsfInt.h"
#include "nsfAccessInt.h"

/*
 *----------------------------------------------------------------------
 *
 *      NsfMethodObjType Tcl_Obj type --
 *
 *      The NsfMethodObjType is a Tcl_Obj type carrying the result of
 *      a method lookup. We define two types (NsfInstanceMethodObjType
 *      and NsfObjectMethodObjType) sharing their implementation. The
 *      type setting function NsfMethodObjSet() receives the intended
 *      type.
 *
 *----------------------------------------------------------------------
 */

static Tcl_FreeInternalRepProc	MethodFreeInternalRep;
static Tcl_DupInternalRepProc MethodDupInternalRep;

Tcl_ObjType NsfInstanceMethodObjType = {
    "nsfInstanceMethod",	/* name */
    MethodFreeInternalRep,	/* freeIntRepProc */
    MethodDupInternalRep,	/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
#ifdef TCL_OBJTYPE_V0
   ,TCL_OBJTYPE_V0
#endif
};
Tcl_ObjType NsfObjectMethodObjType = {
    "nsfObjectMethod",		/* name */
    MethodFreeInternalRep,	/* freeIntRepProc */
    MethodDupInternalRep,	/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
#ifdef TCL_OBJTYPE_V0
   ,TCL_OBJTYPE_V0
#endif
};

/*
 *----------------------------------------------------------------------
 *
 * MethodFreeInternalRep --
 *
 *      Frees internal representation. Implementation of
 *      freeIntRepProc.
 *
 *----------------------------------------------------------------------
 */
static void
MethodFreeInternalRep(
    register Tcl_Obj *objPtr	/* Tcl_Obj structure object with internal
                                 * representation to free. */
) {
  NsfMethodContext *mcPtr = (NsfMethodContext *)objPtr->internalRep.twoPtrValue.ptr1;

  if (mcPtr != NULL) {
#if defined(METHOD_OBJECT_TRACE)
    fprintf(stderr, "MethodFreeInternalRep %p methodContext %p methodEpoch %d type <%s>\n",
            (void*)objPtr, (void*)mcPtr,
            mcPtr->methodEpoch, (objPtr->typePtr != NULL) ? objPtr->typePtr->name : "none");
#endif
    /*
     * ... and free structure
     */
    FREE(NsfMethodContext, mcPtr);
    objPtr->internalRep.twoPtrValue.ptr1 = NULL;
    objPtr->typePtr = NULL;
  }
}

/*
 *----------------------------------------------------------------------
 *
 * MethodDupInternalRep --
 *
 *      Duplicates internal representation. Implementation of
 *      dupIntRepProc.
 *
 *----------------------------------------------------------------------
 */
static void
MethodDupInternalRep(
    Tcl_Obj *srcObjPtr,
    Tcl_Obj *dstObjPtr
) {
  register NsfMethodContext *srcMcPtr = srcObjPtr->internalRep.twoPtrValue.ptr1, *dstMcPtr;

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "MethodDupInternalRep src %p dst %p\n", (void *)srcObjPtr, (void *)dstObjPtr);
#endif

  dstMcPtr = NEW(NsfMethodContext);
  /*fprintf(stderr, "MethodDupInternalRep allocated NsfMethodContext %p for %s\n", dstMcPtr, ObjStr(srcObjPtr));*/
  memcpy(dstMcPtr, srcMcPtr, sizeof(NsfMethodContext));

  dstObjPtr->typePtr = srcObjPtr->typePtr;
  dstObjPtr->internalRep.twoPtrValue.ptr1 = dstMcPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfMethodObjSet --
 *
 *      Converts the provided Tcl_Obj into the type of NsfMethodContext.
 *
 *----------------------------------------------------------------------
 */
int
NsfMethodObjSet(
    Tcl_Interp       *UNUSED(interp),	/* Used for error reporting if not NULL. */
    register Tcl_Obj  *objPtr,          /* The object to convert. */
    const Tcl_ObjType *objectType,
    void              *context,		/* context (to avoid over-eager sharing) */
    unsigned int       methodEpoch,	/* methodEpoch */
    Tcl_Command        cmd,             /* the Tcl command behind the method */
    NsfClass          *cl,              /* the object/class where the method was defined */
    unsigned int       flags		/* flags */
) {
  NsfMethodContext *mcPtr;

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "... NsfMethodObjSet %p %s context %p methodEpoch %d "
          "cmd %p cl %p %s old obj type <%s> flags %.6x\n",
          (void *)objPtr, ObjStr(objPtr), context, methodEpoch,
          (void *)cmd, (void *)cl,
          (cl != NULL) ? ClassName(cl) : "obj",
          (objPtr->typePtr != NULL) ? objPtr->typePtr->name : "none", flags);
#endif
  /*
   * Free or reuse the old internal representation and store own
   * structure as internal representation.
   */
  if (objPtr->typePtr != objectType) {
#if defined(METHOD_OBJECT_TRACE)
    fprintf(stderr, "... NsfMethodObjSet frees old int rep %s\n", (objPtr->typePtr != NULL) ? objPtr->typePtr->name : "none");
#endif
    TclFreeInternalRep(objPtr);
    mcPtr = NEW(NsfMethodContext);
    /*fprintf(stderr, "NsfMethodObjSet allocated NsfMethodContext %p for %s\n", mcPtr, ObjStr(objPtr));*/
    objPtr->internalRep.twoPtrValue.ptr1 = (void *)mcPtr;
    objPtr->internalRep.twoPtrValue.ptr2 = NULL;
    objPtr->typePtr = objectType;
#if defined(METHOD_OBJECT_TRACE)
    fprintf(stderr, "alloc %p methodContext %p methodEpoch %d type <%s> %s refCount %d\n",
            (void *)objPtr, (void *)mcPtr,
            methodEpoch, objectType->name, ObjStr(objPtr), objPtr->refCount);
#endif
  } else {
    mcPtr = (NsfMethodContext *)objPtr->internalRep.twoPtrValue.ptr1;
#if defined(METHOD_OBJECT_TRACE)
    fprintf(stderr, "... NsfMethodObjSet %p reuses internal rep, serial (%d/%d) refCount %d\n",
            (void *)objPtr, mcPtr->methodEpoch, methodEpoch, objPtr->refCount);
#endif
  }

  assert(mcPtr != NULL);

  /*
   * add values to the structure
   */
  mcPtr->context = context;
  mcPtr->methodEpoch = methodEpoch;
  mcPtr->cmd = cmd;
  mcPtr->cl = cl;
  mcPtr->flags = flags;

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * The NsfFlagObjType is a Tcl_Obj type carrying the result of a flag
 * lookup.
 *
 *----------------------------------------------------------------------
 */

static Tcl_FreeInternalRepProc	FlagFreeInternalRep;
static Tcl_DupInternalRepProc   FlagDupInternalRep;

Tcl_ObjType NsfFlagObjType = {
    "nsfFlag",			/* name */
    FlagFreeInternalRep,	/* freeIntRepProc */
    FlagDupInternalRep,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
#ifdef TCL_OBJTYPE_V0
   ,TCL_OBJTYPE_V0
#endif
};

/*
 *----------------------------------------------------------------------
 *
 * FlagFreeInternalRep --
 *
 *      Frees internal representation. Implementation of
 *      freeIntRepProc.
 *
 *----------------------------------------------------------------------
 */
static void
FlagFreeInternalRep(
    Tcl_Obj *objPtr	/* Tcl_Obj structure object with internal
                         * representation to free. */
) {
  register NsfFlag *flagPtr = (NsfFlag *)objPtr->internalRep.twoPtrValue.ptr1;

  if (flagPtr != NULL) {

    /*fprintf(stderr, "FlagFreeInternalRep %p flagPtr %p serial (%d) payload %p\n",
      objPtr, flagPtr, flagPtr->serial, flagPtr->payload);*/

    /*
     * Decrement refCounts; same as in NsfFlagSet() in the reuse branch
     */
    if (flagPtr->payload != NULL) {
      DECR_REF_COUNT2("flagPtr->payload", flagPtr->payload);
    }

    /*
     * ... and free structure
     */
    FREE(NsfFlag, flagPtr);
    objPtr->internalRep.twoPtrValue.ptr1 = NULL;
  }
}

/*
 *----------------------------------------------------------------------
 *
 * FlagDupInternalRep --
 *
 *      Duplicates internal representation. Implementation of
 *      dupIntRepProc.
 *
 *----------------------------------------------------------------------
 */
static void
FlagDupInternalRep(
    Tcl_Obj *srcObjPtr,
    Tcl_Obj *dstObjPtr
) {
  register NsfFlag *srcPtr = (NsfFlag *)srcObjPtr->internalRep.twoPtrValue.ptr1, *dstPtr;

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "FlagDupInternalRepx src %p dst %p\n", (void *)srcObjPtr,(void *)dstObjPtr);
#endif

  dstPtr = NEW(NsfFlag);
  /*fprintf(stderr, "FlagDupInternalRep allocated NsfFlag %p for %s\n", dstPtr, ObjStr(srcObjPtr));*/
  memcpy(dstPtr, srcPtr, sizeof(NsfFlag));

  dstObjPtr->typePtr = srcObjPtr->typePtr;
  dstObjPtr->internalRep.twoPtrValue.ptr1 = dstPtr;
}

/*
 *----------------------------------------------------------------------
 *
 *      NsfFlagObjSet --
 *
 *      Convert the provided Tcl_Obj into the type of an NSF flag.
 *
 *----------------------------------------------------------------------
 */
int
NsfFlagObjSet(
    Tcl_Interp *UNUSED(interp),		/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr,           /* The object to convert. */
    Nsf_Param const *baseParamPtr,	/* the full parameter block */
    int serial,				/* interface serial */
    Nsf_Param const *paramPtr,          /* a single parameter */
    Tcl_Obj *payload,                   /* payload */
    unsigned int flags                  /* detail infos */
) {
  NsfFlag *flagPtr;

  /*fprintf(stderr, "NsfFlagObjSet %p %s signature %p (%d) param %p payload %p flags %.4x\n",
    objPtr, ObjStr(objPtr), baseParamPtr, serial, paramPtr, payload, flags);*/

  /*
   * Free or reuse the old internal representation and store own
   * structure as internal representation.
   */
  if (objPtr->typePtr != &NsfFlagObjType) {
    TclFreeInternalRep(objPtr);
    flagPtr = NEW(NsfFlag);
    assert(flagPtr != NULL);
    /*fprintf(stderr, "NsfFlagObjSet allocated NsfFlag %p for %s\n", flagPtr, ObjStr(objPtr));*/

    objPtr->internalRep.twoPtrValue.ptr1 = (void *)flagPtr;
    objPtr->internalRep.twoPtrValue.ptr2 = NULL;
    objPtr->typePtr = &NsfFlagObjType;

  } else {
    flagPtr = (NsfFlag *)objPtr->internalRep.twoPtrValue.ptr1;

    assert(flagPtr != NULL);

    /*fprintf(stderr, "NsfFlagObjSet %p reuses internal rep, serial (%d/%d)\n",
      objPtr, flagPtr->serial, serial);*/

      if (flagPtr->payload != NULL) {
      DECR_REF_COUNT2("flagPtr->payload", flagPtr->payload);
    }
  }

  /*
   * add values to the structure
   */
  flagPtr->signature = baseParamPtr;
  flagPtr->serial = serial;
  flagPtr->paramPtr = paramPtr;
  flagPtr->payload = payload;
  if (payload != NULL) {INCR_REF_COUNT2("flagPtr->payload", flagPtr->payload);}
  flagPtr->flags = flags;

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 *      Mixinreg Tcl_Obj type --
 *
 *      The mixin registration type is a Tcl_Obj type carrying a
 *      class and a guard object. The string representation might have
 *      the form "/cls/" or "/cls/ -guard /expr/". When no guard
 *      expression is provided (first form), the guard entry is NULL.
 *
 *----------------------------------------------------------------------
 */

typedef struct {
  NsfClass *mixin;
  Tcl_Obj *guardObj;
} Mixinreg;

static Tcl_FreeInternalRepProc	MixinregFreeInternalRep;
static Tcl_SetFromAnyProc       MixinregSetFromAny;
static Tcl_DupInternalRepProc   MixinregDupInternalRep;

Tcl_ObjType NsfMixinregObjType = {
    "nsfMixinreg",			/* name */
    MixinregFreeInternalRep,		/* freeIntRepProc */
    MixinregDupInternalRep,		/* dupIntRepProc */
    NULL,				/* updateStringProc */
    MixinregSetFromAny			/* setFromAnyProc */
#ifdef TCL_OBJTYPE_V0
   ,TCL_OBJTYPE_V0
#endif
};

/*
 *----------------------------------------------------------------------
 *
 * MixinregFreeInternalRep --
 *
 *      Frees internal representation. Implementation of
 *      freeIntRepProc.
 *
 *----------------------------------------------------------------------
 */
static void
MixinregFreeInternalRep(
    register Tcl_Obj *objPtr	/* Mixinreg structure object with internal
                                 * representation to free. */
) {
  Mixinreg *mixinRegPtr = (Mixinreg *)objPtr->internalRep.twoPtrValue.ptr1;

  nonnull_assert(mixinRegPtr != NULL);
  /*fprintf(stderr, "MixinregFreeInternalRep freeing mixinReg %p class %p guard %p refcount before decr %d\n",
    mixinRegPtr, mixinRegPtr->mixin, mixinRegPtr->guardObj, (&(mixinRegPtr->mixin)->object)->refCount);*/

  /*
   * Decrement refCounts
   */
  NsfCleanupObject(&(mixinRegPtr->mixin)->object, "MixinregFreeInternalRep");

  if (mixinRegPtr->guardObj != NULL) {DECR_REF_COUNT2("mixinRegPtr->guardObj", mixinRegPtr->guardObj);}

  /*
   * ... and free structure
   */
  FREE(Mixinreg, mixinRegPtr);
  objPtr->internalRep.twoPtrValue.ptr1 = NULL;
  objPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * MixinregDupInternalRep --
 *
 *      Duplicates internal representation. Implementation of
 *      dupIntRepProc.
 *
 *----------------------------------------------------------------------
 */
static void
MixinregDupInternalRep(
    Tcl_Obj *srcObjPtr,
    Tcl_Obj *dstObjPtr
) {
  register Mixinreg *srcPtr = (Mixinreg *)srcObjPtr->internalRep.twoPtrValue.ptr1, *dstPtr;

  nonnull_assert(srcPtr != NULL);

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "MixinregDupInternalRep src %p dst %p\n",
          (void *)srcObjPtr, (void *)dstObjPtr);
#endif
  dstPtr = NEW(Mixinreg);
  memcpy(dstPtr, srcPtr, sizeof(Mixinreg));

  /*
   * Increment refcounts.
   */
  NsfObjectRefCountIncr(&(srcPtr->mixin)->object);
  if (srcPtr->guardObj != NULL) {
    INCR_REF_COUNT2("mixinRegPtr->guardObj", srcPtr->guardObj);
  }

  /*
   * Update destination obj.
   */
  dstObjPtr->typePtr = srcObjPtr->typePtr;
  dstObjPtr->internalRep.twoPtrValue.ptr1 = dstPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * MixinregSetFromAny --
 *
 *      Sets the type and internal representation when converting to
 *      this object type. Implementation of setFromAnyProc.
 *
 *----------------------------------------------------------------------
 */
static int
MixinregSetFromAny(
    Tcl_Interp *interp,	/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr	/* The object to convert. */
) {
  NsfClass  *mixin = NULL;
  TCL_OBJC_T        oc, result;
  Tcl_Obj  **ov;

  result = Tcl_ListObjGetElements(interp, objPtr, &oc, &ov);
  if (likely(result == TCL_OK)) {
    Tcl_Obj *guardObj, *nameObj;

    /*
     *  objPtr holds a valid Tcl list
     */
    if (oc == 1) {
      nameObj = ov[0];
      guardObj = NULL;

    } else if (oc == 3 && !strcmp(ObjStr(ov[1]), NsfGlobalStrings[NSF_GUARD_OPTION])) {
      nameObj = ov[0];
      guardObj = ov[2];

    } else {
      nameObj = objPtr;
      guardObj = NULL;
    }

    /*
     * Syntax was ok. Try to lookup mixin classes:
     */
    if (NsfGetClassFromObj(interp, nameObj, &mixin, 1) != TCL_OK) {
      result = NsfObjErrType(interp, "mixin", nameObj, "a class as mixin", NULL);
    } else {
      Mixinreg *mixinRegPtr;

      assert(mixin != NULL);

      /*
       * Conversion was ok.
       * Allocate structure ...
       */
      mixinRegPtr = NEW(Mixinreg);
      mixinRegPtr->mixin = mixin;
      mixinRegPtr->guardObj = guardObj;

      /*
       * ... and increment refCounts
       */
      NsfObjectRefCountIncr((&mixin->object));
      if (guardObj != NULL) {INCR_REF_COUNT2("mixinRegPtr->guardObj", guardObj);}

      /*
       * Build list of Tcl_Objs per mixin class for invalidation.
       */
      { NsfClassOpt *clOpt = NsfRequireClassOpt(mixin);
        if (clOpt->mixinRegObjs == NULL) {
          clOpt->mixinRegObjs = Tcl_NewListObj(1, &objPtr);
          INCR_REF_COUNT2("mixinRegObjs", clOpt->mixinRegObjs);
        } else {
          Tcl_ListObjAppendElement(interp, clOpt->mixinRegObjs, objPtr);
        }
      }

      /*fprintf(stderr, "MixinregSetFromAny alloc mixinReg %p class %p guard %p object->refCount %d\n",
        mixinRegPtr, mixinRegPtr->mixin, mixinRegPtr->guardObj, ((&mixin->object)->refCount));*/

      /*
       * Free the old internal representation and store own structure as internal
       * representation.
       */
      TclFreeInternalRep(objPtr);
      objPtr->internalRep.twoPtrValue.ptr1 = (void *)mixinRegPtr;
      objPtr->internalRep.twoPtrValue.ptr2 = NULL;
      objPtr->typePtr = &NsfMixinregObjType;
    }
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfMixinregGet --
 *
 *      Return the internal representation of a Mixinreg Tcl_Obj to
 *      keep it local to this file.
 *
 * Results:
 *      Tcl result code, arg two and three on success.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
NsfMixinregGet(
    Tcl_Interp *interp,
    Tcl_Obj    *obj,
    NsfClass  **classPtr,
    Tcl_Obj   **guardObj
) {

  nonnull_assert(interp != NULL);
  nonnull_assert(obj != NULL);
  nonnull_assert(classPtr != NULL);
  nonnull_assert(guardObj != NULL);

  if (obj->typePtr == &NsfMixinregObjType) {
    Mixinreg *mixinRegPtr = obj->internalRep.twoPtrValue.ptr1;

    /*
     * We got a mixin with an included cmd, but both might have been deleted already.
     */
    if ((mixinRegPtr->mixin->object.flags & NSF_DELETED) != 0u
        || TclIsCommandDeleted(mixinRegPtr->mixin->object.id)) {

      /*
       * The cmd is deleted. Try to refetch it.
       */
      /*fprintf(stderr, "### we have to refetch internal rep of obj %p refCount %d\n",
        obj, obj->refCount);*/

      if (MixinregSetFromAny(interp, obj) == TCL_OK) {
        mixinRegPtr = obj->internalRep.twoPtrValue.ptr1;
      } else {
        return TCL_ERROR;
      }
    }

    *guardObj = mixinRegPtr->guardObj;
    *classPtr = mixinRegPtr->mixin;

    return TCL_OK;
  }

  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfMixinregInvalidate --
 *
 *      MixinClasses keep a list of Tcl_Objs of type Mixinreg.
 *      When a class is deleted, this call makes sure that
 *      non-of these have the chance to point to a stale entry.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      Freeing internal rep of Tcl_Objs.
 *
 *----------------------------------------------------------------------
 */

int
NsfMixinregInvalidate(
    Tcl_Interp *interp,
    Tcl_Obj *obj
) {
  int i, result, oc = 0;
  Tcl_Obj **objv;

  result = Tcl_ListObjGetElements(interp, obj, &oc, &objv);

  for (i= 0; i < oc; i++) {
    if (objv[i]->typePtr == &NsfMixinregObjType) {
      MixinregFreeInternalRep((objv[i]));
    }
  }

  return result;
}


/*
 *----------------------------------------------------------------------
 *
 *      Filterreg Tcl_Obj type --
 *
 *      The filter registration type is a Tcl_Obj type carrying the
 *      name of a filter and a guard object. The string representation
 *      might have the form "/filter/" or "/filter/ -guard
 *      /expr/". When no guard expression is provided (first form),
 *      the guard entry is NULL. The primary purpose of this converter
 *      is to provide symmetry to Mixinregs and to provide meaningful
 *      type names for introspection.
 *
 *----------------------------------------------------------------------
 */

typedef struct {
  Tcl_Obj *filterObj;
  Tcl_Obj *guardObj;
} Filterreg;

static Tcl_FreeInternalRepProc	FilterregFreeInternalRep;
static Tcl_DupInternalRepProc   FilterregDupInternalRep;
static Tcl_SetFromAnyProc       FilterregSetFromAny;

Tcl_ObjType NsfFilterregObjType = {
    "nsfFilterreg",			/* name */
    FilterregFreeInternalRep,		/* freeIntRepProc */
    FilterregDupInternalRep,		/* dupIntRepProc */
    NULL,				/* updateStringProc */
    FilterregSetFromAny			/* setFromAnyProc */
#ifdef TCL_OBJTYPE_V0
   ,TCL_OBJTYPE_V0
#endif
};

/*
 *----------------------------------------------------------------------
 *
 * FilterregFreeInternalRep --
 *
 *      Frees internal representation. Implementation of
 *      freeIntRepProc.
 *
 *----------------------------------------------------------------------
 */
static void
FilterregFreeInternalRep(
    register Tcl_Obj *objPtr	/* Filterreg structure object with internal
                                 * representation to free. */
) {
  Filterreg *filterregPtr = (Filterreg *)objPtr->internalRep.twoPtrValue.ptr1;

  nonnull_assert(filterregPtr != NULL);

  /*fprintf(stderr, "FilterregFreeInternalRep freeing filterreg %p class %p guard %p\n",
    filterregPtr, filterregPtr->class, filterregPtr->guardObj);*/

  /*
   * Decrement refCounts
   */
  DECR_REF_COUNT2("filterregPtr->filterObj", filterregPtr->filterObj);
  if (filterregPtr->guardObj != NULL) {DECR_REF_COUNT2("filterregPtr->guardObj", filterregPtr->guardObj);}

  /*
   * ... and free structure
   */
  FREE(Filterreg, filterregPtr);
  objPtr->internalRep.twoPtrValue.ptr1 = NULL;
  objPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * FilterregDupInternalRep --
 *
 *      Duplicates internal representation. Implementation of
 *      dupIntRepProc.
 *
 *----------------------------------------------------------------------
 */
static void
FilterregDupInternalRep(
    Tcl_Obj *srcObjPtr,
    Tcl_Obj *dstObjPtr
) {
  register Filterreg *srcPtr, *dstPtr;

  assert(srcObjPtr != NULL);
  assert(dstObjPtr != NULL);

  srcPtr = (Filterreg *)srcObjPtr->internalRep.twoPtrValue.ptr1;
  assert(srcPtr != NULL);

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "FilterregDupInternalRep src %p dst %p\n", (void *)srcObjPtr, (void *)dstObjPtr);
#endif

  dstPtr = NEW(Filterreg);
  memcpy(dstPtr, srcPtr, sizeof(Filterreg));

  /*
   * Increment refcounts
   */
  INCR_REF_COUNT2("filterregPtr->filterObj", srcPtr->filterObj);
  if (srcPtr->guardObj != NULL) {INCR_REF_COUNT2("FilterregPtr->guardObj", srcPtr->guardObj);}

  /*
   * Update destination Tcl_Obj.
   */
  dstObjPtr->typePtr = srcObjPtr->typePtr;
  dstObjPtr->internalRep.twoPtrValue.ptr1 = dstPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FilterregSetFromAny --
 *
 *      Sets the type and internal representation when converting to
 *      this object type. Implementation of setFromAnyProc.
 *
 *----------------------------------------------------------------------
 */
static int
FilterregSetFromAny(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr	/* The object to convert. */
) {
  Tcl_Obj *guardObj = NULL, *filterObj;
  Filterreg *filterregPtr;
  TCL_OBJC_T oc; Tcl_Obj **ov;

  if (Tcl_ListObjGetElements(interp, objPtr, &oc, &ov) == TCL_OK) {
    if (oc == 1) {
      filterObj = ov[0];

      /*    } else if (oc == 2) {
      filterObj = ov[0];
      guardObj = ov[1];*/

    } else if (oc == 3 && !strcmp(ObjStr(ov[1]), NsfGlobalStrings[NSF_GUARD_OPTION])) {
      filterObj = ov[0];
      guardObj = ov[2];

    } else {
      return TCL_ERROR;
    }
  }  else {
    return TCL_ERROR;
  }

  /*
   * Conversion was ok.
   * Allocate structure ...
   */
  filterregPtr = NEW(Filterreg);

  filterregPtr->filterObj = filterObj;
  filterregPtr->guardObj = guardObj;

  /*
   * ... and increment refCounts
   */
  INCR_REF_COUNT2("filterregPtr->filterObj", filterObj);
  if (guardObj != NULL) {INCR_REF_COUNT2("filterregPtr->guardObj", guardObj);}

  /*fprintf(stderr, "FilterregSetFromAny alloc filterreg %p class %p guard %p\n",
    filterregPtr, filterregPtr->filterObj, filterregPtr->guardObj);*/

  /*
   * Free the old internal representation and store own structure as internal
   * representation.
   */
  TclFreeInternalRep(objPtr);
  objPtr->internalRep.twoPtrValue.ptr1 = (void *)filterregPtr;
  objPtr->internalRep.twoPtrValue.ptr2 = NULL;
  objPtr->typePtr = &NsfFilterregObjType;

  return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * NsfFilterregGet --
 *
 *      Return the internal representation of a Filterreg Tcl_Obj to
 *      keep it local to this file.
 *
 * Results:
 *      Tcl result code, arg two and three on success.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

int
NsfFilterregGet(
    Tcl_Interp *UNUSED(interp),
    Tcl_Obj *obj,
    Tcl_Obj **filterObj,
    Tcl_Obj **guardObj
) {
  int result;

  nonnull_assert(obj != NULL);
  nonnull_assert(filterObj != NULL);
  nonnull_assert(guardObj != NULL);

  if (obj->typePtr == &NsfFilterregObjType) {
    Filterreg *filterregPtr = obj->internalRep.twoPtrValue.ptr1;
    *filterObj = filterregPtr->filterObj;
    *guardObj = filterregPtr->guardObj;
    result = TCL_OK;
  } else {
    result = TCL_ERROR;
  }

  return result;
}
/*
 * Filterreg type end
 */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * eval: (c-guess)
 * End:
 */
