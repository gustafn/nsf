/*
 * nsfError.c --
 *
 *      Tcl_Obj types provided by the Next Scripting Framework.
 *
 * Copyright (C) 1999-2014 Gustaf Neumann
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
 *
 */

#include "nsfInt.h"
#include "nsfAccessInt.h"

/*
 *----------------------------------------------------------------------
 *
 *  NsfMethodObjType Tcl_Obj type --
 *
 *      The NsfMethodObjType is an Tcl_Obj type carrying the result of
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
  "nsfInstanceMethod",		/* name */
    MethodFreeInternalRep,	/* freeIntRepProc */
    MethodDupInternalRep,	/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};
Tcl_ObjType NsfObjectMethodObjType = {
    "nsfObjectMethod",		/* name */
    MethodFreeInternalRep,	/* freeIntRepProc */
    MethodDupInternalRep,	/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};

/*
 * freeIntRepProc
 */
static void
MethodFreeInternalRep(
    register Tcl_Obj *objPtr)	/* Tcl_Obj structure object with internal
				 * representation to free. */
{
  NsfMethodContext *mcPtr = (NsfMethodContext *)objPtr->internalRep.twoPtrValue.ptr1;

  if (mcPtr != NULL) {
#if defined(METHOD_OBJECT_TRACE)
    fprintf(stderr, "MethodFreeInternalRep %p methodContext %p methodEpoch %d type <%s>\n",
	    objPtr, mcPtr, mcPtr->methodEpoch, (objPtr->typePtr != NULL) ? objPtr->typePtr->name : "none");
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
 * dupIntRepProc
 */
static void
MethodDupInternalRep(
    Tcl_Obj *srcObjPtr,
    Tcl_Obj *dstObjPtr)
{
  register NsfMethodContext *srcMcPtr = srcObjPtr->internalRep.twoPtrValue.ptr1, *dstMcPtr;

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "MethodDupInternalRep src %p dst %p\n", srcObjPtr, dstObjPtr);
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
 *  NsfMethodObjSet --
 *
 *      Convert the provided Tcl_Obj into the type of NsfMethodContext.
 *
 *----------------------------------------------------------------------
 */
int
NsfMethodObjSet(
    Tcl_Interp *interp,			/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr,   	/* The object to convert. */
    Tcl_ObjType *objectType,
    void *context,			/* context (to avoid over-eager sharing) */
    int methodEpoch,			/* methodEpoch */
    Tcl_Command cmd,	  		/* the tclCommand behind the method */
    NsfClass *cl,	  		/* the object/class where the method was defined */
    unsigned int flags			/* flags */
		)
{
  NsfMethodContext *mcPtr;

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "... NsfMethodObjSet %p %s context %p methodEpoch %d "
	  "cmd %p cl %p %s old obj type <%s> flags %.6x\n",
	  objPtr, ObjStr(objPtr), context, methodEpoch, cmd, cl,
          (cl != NULL) ? ClassName(cl) : "obj",
          (objPtr->typePtr != NULL) ? objPtr->typePtr->name : "none", flags);
#endif
  /*
   * Free or reuse the old interal representation and store own
   * structure as internal representation.
   */
  if (objPtr->typePtr != objectType) {
#if defined(METHOD_OBJECT_TRACE)
    fprintf(stderr, "... NsfMethodObjSet frees old int rep %s\n", (objPtr->typePtr != NULL) ? objPtr->typePtr->name : "none");
#endif
    TclFreeIntRep(objPtr);
    mcPtr = NEW(NsfMethodContext);
    /*fprintf(stderr, "NsfMethodObjSet allocated NsfMethodContext %p for %s\n", mcPtr, ObjStr(objPtr));*/
    objPtr->internalRep.twoPtrValue.ptr1 = (void *)mcPtr;
    objPtr->internalRep.twoPtrValue.ptr2 = NULL;
    objPtr->typePtr = objectType;
#if defined(METHOD_OBJECT_TRACE)
    fprintf(stderr, "alloc %p methodContext %p methodEpoch %d type <%s> %s refCount %d\n",
	    objPtr, mcPtr, methodEpoch, objectType->name, ObjStr(objPtr), objPtr->refCount);
#endif
  } else {
    mcPtr = (NsfMethodContext *)objPtr->internalRep.twoPtrValue.ptr1;
#if defined(METHOD_OBJECT_TRACE)
    fprintf(stderr, "... NsfMethodObjSet %p reuses interal rep, serial (%d/%d) refCount %d\n",
	    objPtr, mcPtr->methodEpoch, methodEpoch, objPtr->refCount);
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
 *  NsfFlagObjType --
 *
 *      The NsfFlagObjType is an Tcl_Obj type carrying the result of a
 *      flag lookup.
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
};

/*
 * freeIntRepProc
 */
static void
FlagFreeInternalRep(
   Tcl_Obj *objPtr)	/* Tcl_Obj structure object with internal
			 * representation to free. */
{
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
 * dupIntRepProc
 */
static void
FlagDupInternalRep(
    Tcl_Obj *srcObjPtr,
    Tcl_Obj *dstObjPtr)
{
  register NsfFlag *srcPtr = (NsfFlag *)srcObjPtr->internalRep.twoPtrValue.ptr1, *dstPtr;

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "FlagDupInternalRepx src %p dst %p\n", srcObjPtr, dstObjPtr);
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
 *  NsfFlagObjSet --
 *
 *      Convert the provided Tcl_Obj into the type of an nsf flag.
 *
 *----------------------------------------------------------------------
 */
int
NsfFlagObjSet(
    Tcl_Interp *interp,			/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr,   	/* The object to convert. */
    Nsf_Param const *baseParamPtr,	/* the full parameter block */
    int serial,				/* interface serial */
    Nsf_Param const *paramPtr,  	/* a single parameter */
    Tcl_Obj *payload,  			/* payload */
    int flags  				/* detail infos */
	      )
{
  NsfFlag *flagPtr;

  /*fprintf(stderr, "NsfFlagObjSet %p %s signature %p (%d) param %p payload %p flags %.4x\n",
    objPtr, ObjStr(objPtr), baseParamPtr, serial, paramPtr, payload, flags);*/

  /*
   * Free or reuse the old interal representation and store own
   * structure as internal representation.
   */
  if (objPtr->typePtr != &NsfFlagObjType) {
    TclFreeIntRep(objPtr);
    flagPtr = NEW(NsfFlag);
    /*fprintf(stderr, "NsfFlagObjSet allocated NsfFlag %p for %s\n", flagPtr, ObjStr(objPtr));*/

    objPtr->internalRep.twoPtrValue.ptr1 = (void *)flagPtr;
    objPtr->internalRep.twoPtrValue.ptr2 = NULL;
    objPtr->typePtr = &NsfFlagObjType;
  } else {
    flagPtr = (NsfFlag *)objPtr->internalRep.twoPtrValue.ptr1;

    /*fprintf(stderr, "NsfFlagObjSet %p reuses interal rep, serial (%d/%d)\n",
      objPtr, flagPtr->serial, serial);*/

    if (flagPtr->payload != NULL) {DECR_REF_COUNT2("flagPtr->payload", flagPtr->payload);}
  }

  assert(flagPtr != NULL);

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
 *  Mixinreg Tcl_Obj type --
 *
 *      The mixin registration type is an Tcl_Obj type carrying a
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
};

/*
 * freeIntRepProc
 */
static void
MixinregFreeInternalRep(
    register Tcl_Obj *objPtr)	/* Mixinreg structure object with internal
				 * representation to free. */
{
  Mixinreg *mixinRegPtr = (Mixinreg *)objPtr->internalRep.twoPtrValue.ptr1;

  assert(mixinRegPtr != NULL);
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
 * dupIntRepProc
 */
static void
MixinregDupInternalRep(
    Tcl_Obj *srcObjPtr,
    Tcl_Obj *dstObjPtr)
{
  register Mixinreg *srcPtr = (Mixinreg *)srcObjPtr->internalRep.twoPtrValue.ptr1, *dstPtr;

  assert(srcPtr != NULL);

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "MixinregDupInternalRep src %p dst %p\n",
	  srcObjPtr, dstObjPtr);
#endif
  dstPtr = NEW(Mixinreg);
  memcpy(dstPtr, srcPtr, sizeof(Mixinreg));

  /*
   * increment refcounts
   */
  NsfObjectRefCountIncr(&(srcPtr->mixin)->object);
  if (srcPtr->guardObj != NULL) {INCR_REF_COUNT2("mixinRegPtr->guardObj", srcPtr->guardObj);}

  /*
   * update destination obj
   */
  dstObjPtr->typePtr = srcObjPtr->typePtr;
  dstObjPtr->internalRep.twoPtrValue.ptr1 = dstPtr;
}


/*
 * setFromAnyProc
 */
static int
MixinregSetFromAny(
    Tcl_Interp *interp,	/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr)	/* The object to convert. */
{
  NsfClass *mixin = NULL;
  Tcl_Obj *guardObj = NULL, *nameObj = NULL;
  Mixinreg *mixinRegPtr;
  int oc; Tcl_Obj **ov;

  if (Tcl_ListObjGetElements(interp, objPtr, &oc, &ov) == TCL_OK) {

    if (oc == 1) {
      nameObj = ov[0];

      /*} else if (oc == 2) {
      nameObj = ov[0];
      guardObj = ov[1];*/

    } else if (oc == 3 && !strcmp(ObjStr(ov[1]), NsfGlobalStrings[NSF_GUARD_OPTION])) {
      nameObj = ov[0];
      guardObj = ov[2];

    } else {
      nameObj = objPtr;
    }

  } else {
    return NsfObjErrType(interp, "mixin", nameObj, "a class as mixin", NULL);
  }

  /*
   * Syntax was ok. Try to lookup mixin classes:
   */
  if (NsfGetClassFromObj(interp, nameObj, &mixin, 1) != TCL_OK) {
    return NsfObjErrType(interp, "mixin", nameObj, "a class as mixin", NULL);
  }

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
   * Build list of tcl-objs per mixin class for invalidation.
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
   * Free the old interal representation and store own structure as internal
   * representation.
   */
  TclFreeIntRep(objPtr);
  objPtr->internalRep.twoPtrValue.ptr1 = (void *)mixinRegPtr;
  objPtr->internalRep.twoPtrValue.ptr2 = NULL;
  objPtr->typePtr = &NsfMixinregObjType;

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfMixinregGet --
 *
 *      Return the internal representation of a mixinreg obj type to
 *      keep internal rep local to this file.
 *
 * Results:
 *      Tcl result code, arg two and three on success.
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

int
NsfMixinregGet(Tcl_Interp *interp, Tcl_Obj *obj, NsfClass **clPtr, Tcl_Obj **guardObj) {

  assert(interp != NULL);
  assert(obj != NULL);
  assert(clPtr != NULL);
  assert(guardObj != NULL);

  if (obj->typePtr == &NsfMixinregObjType) {
    Mixinreg *mixinRegPtr = obj->internalRep.twoPtrValue.ptr1;

    /*
     * We got a mixin with an included cmd, but both might be already deleted.
     */
    if ((mixinRegPtr->mixin->object.flags & NSF_DELETED) != 0u
        || (Tcl_Command_flags(mixinRegPtr->mixin->object.id) & CMD_IS_DELETED) != 0u) {

      /*
       * The cmd is deleted. retry to refetch it.
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
    *clPtr = mixinRegPtr->mixin;

    return TCL_OK;
  }

  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfMixinregInvalidate --
 *
 *      MixinClasses keep a list of Tcl_Objs of type mixinReg.
 *      When a class is deleted, this call makes sure, that
 *      non-of these have th chance to point to a stale entry.
 *
 * Results:
 *      Tcl result code
 *
 * Side effects:
 *      Freeing internal rep of Tcl_Objs.
 *
 *----------------------------------------------------------------------
 */

int
NsfMixinregInvalidate(Tcl_Interp *interp, Tcl_Obj *obj) {
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
 *  Filterreg Tcl_Obj type --
 *
 *      The filter registration type is an Tcl_Obj type carrying a the
 *      name of a filter and a guard object. The string representation
 *      might have the form "/filter/" or "/filter/ -guard
 *      /expr/". When no guard expression is provided (first form),
 *      the guard entry is NULL. The primary purpose of this converter
 *      is to provide symmetry to mixinregs and to provide meaningful
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
};


/*
 * freeIntRepProc
 */
static void
FilterregFreeInternalRep(
    register Tcl_Obj *objPtr)	/* Filterreg structure object with internal
				 * representation to free. */
{
  Filterreg *filterregPtr = (Filterreg *)objPtr->internalRep.twoPtrValue.ptr1;

  assert(filterregPtr != NULL);

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
 * dupIntRepProc
 */
static void
FilterregDupInternalRep(
    Tcl_Obj *srcObjPtr,
    Tcl_Obj *dstObjPtr) {
  register Filterreg *srcPtr, *dstPtr;

  assert(srcObjPtr != NULL);
  assert(dstObjPtr != NULL);

  srcPtr = (Filterreg *)srcObjPtr->internalRep.twoPtrValue.ptr1;
  assert(srcPtr != NULL);
  
#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "FilterregDupInternalRep src %p dst %p\n", srcObjPtr, dstObjPtr);
#endif

  dstPtr = NEW(Filterreg);
  memcpy(dstPtr, srcPtr, sizeof(Filterreg));

  /*
   * increment refcounts
   */
  INCR_REF_COUNT2("filterregPtr->filterObj", srcPtr->filterObj);
  if (srcPtr->guardObj != NULL) {INCR_REF_COUNT2("FilterregPtr->guardObj", srcPtr->guardObj);}

  /*
   * update destination obj
   */
  dstObjPtr->typePtr = srcObjPtr->typePtr;
  dstObjPtr->internalRep.twoPtrValue.ptr1 = dstPtr;
}

/*
 * setFromAnyProc
 */
static int
FilterregSetFromAny(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr)	/* The object to convert. */
{
  Tcl_Obj *guardObj = NULL, *filterObj;
  Filterreg *filterregPtr;
  int oc; Tcl_Obj **ov;

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
   * Free the old interal representation and store own structure as internal
   * representation.
   */
  TclFreeIntRep(objPtr);
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
 *      Return the internal representation of a filterreg obj type to
 *      keep internal rep local to this file.
 *
 * Results:
 *      Tcl result code, arg two and three on success.
 *
 * Side effects:
 *      None
 *
 *----------------------------------------------------------------------
 */

int
NsfFilterregGet(Tcl_Interp *UNUSED(interp), Tcl_Obj *obj, Tcl_Obj **filterObj, Tcl_Obj **guardObj) {

  assert(obj != NULL);
  assert(filterObj != NULL);
  assert(guardObj != NULL);

  if (obj->typePtr == &NsfFilterregObjType) {
    Filterreg *filterregPtr = obj->internalRep.twoPtrValue.ptr1;
    *filterObj = filterregPtr->filterObj;
    *guardObj = filterregPtr->guardObj;
    return TCL_OK;
  }

  return TCL_ERROR;
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
 * End:
 */
