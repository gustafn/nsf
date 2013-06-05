/*  
 *  nsfError.c --
 *  
 *      Tcl_Obj types provided by the Next Scripting Framework.
 *  
 *  Copyright (C) 1999-2013 Gustaf Neumann
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
	    objPtr, mcPtr, mcPtr->methodEpoch, 
	    objPtr->typePtr ? objPtr->typePtr->name : "none");
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
    int flags		  		/* flags */
		)
{
  NsfMethodContext *mcPtr;

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "... NsfMethodObjSet %p %s context %p methodEpoch %d "
	  "cmd %p cl %p %s old obj type <%s> flags %.6x\n",
	  objPtr, ObjStr(objPtr), context, methodEpoch, cmd, cl, cl ? ClassName(cl) : "obj",
	  objPtr->typePtr ? objPtr->typePtr->name : "none", flags);
#endif
  /*
   * Free or reuse the old interal representation and store own
   * structure as internal representation.
   */
  if (objPtr->typePtr != objectType) {
#if defined(METHOD_OBJECT_TRACE)
    fprintf(stderr, "... NsfMethodObjSet frees old int rep %s\n",
	    objPtr->typePtr ? objPtr->typePtr->name : "none");
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

  assert(mcPtr);

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
    if (flagPtr->payload) {DECR_REF_COUNT2("flagPtr->payload", flagPtr->payload);}

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
    Nsf_Param CONST *baseParamPtr,	/* the full parameter block */
    int serial,				/* interface serial */
    Nsf_Param CONST *paramPtr,  	/* a single parameter */
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

    /* fprintf(stderr, "NsfFlagObjSet %p reuses interal rep, serial (%d/%d)\n",
       objPtr, flagPtr->serial, serial);*/

    if (flagPtr->payload) {DECR_REF_COUNT2("flagPtr->payload", flagPtr->payload);}
  }

  assert(flagPtr);

  /*
   * add values to the structure
   */
  flagPtr->signature = baseParamPtr;
  flagPtr->serial = serial;
  flagPtr->paramPtr = paramPtr;
  flagPtr->payload = payload;
  if (payload) {INCR_REF_COUNT2("flagPtr->payload", flagPtr->payload);}
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

  assert(mixinRegPtr);
  /*fprintf(stderr, "MixinregFreeInternalRep freeing mixinReg %p class %p guard %p\n",
    mixinRegPtr, mixinRegPtr->class, mixinRegPtr->guardObj);*/
  /*
   * Decrement refCounts
   */
  NsfObjectRefCountDecr(&(mixinRegPtr->mixin)->object);
  if (mixinRegPtr->guardObj) {DECR_REF_COUNT2("mixinRegPtr->guardObj", mixinRegPtr->guardObj);}
  
  /*
   * ... and free structure
   */
  FREE(Mixinreg, mixinRegPtr);
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

  assert(srcPtr);

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
  if (srcPtr->guardObj) {INCR_REF_COUNT2("mixinRegPtr->guardObj", srcPtr->guardObj);}

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
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr)	/* The object to convert. */
{
  NsfClass *mixin = NULL;
  Tcl_Obj *guardObj = NULL, *nameObj;
  Mixinreg *mixinRegPtr;
  int oc; Tcl_Obj **ov;

  if (Tcl_ListObjGetElements(interp, objPtr, &oc, &ov) == TCL_OK) {
    if (oc == 3 && !strcmp(ObjStr(ov[1]), NsfGlobalStrings[NSF_GUARD_OPTION])) {
      nameObj = ov[0];
      guardObj = ov[2];
      /* fprintf(stderr, "mixinadd name = '%s', guard = '%s'\n", ObjStr(nameObj), ObjStr(guardObj));*/
    } else if (oc == 1) {
      nameObj = ov[0];
    } else {
      return TCL_ERROR;
    }
  } else {
    return TCL_ERROR;
  }

  /* 
   * Try to resolve unknowns
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
  if (guardObj) {INCR_REF_COUNT2("mixinRegPtr->guardObj", guardObj);}

  /*fprintf(stderr, "MixinregSetFromAny alloc mixinReg %p class %p guard %p\n",
    mixinRegPtr, mixinRegPtr->mixin, mixinRegPtr->guardObj);*/

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
NsfMixinregGet(Tcl_Obj *obj, NsfClass **clPtr, Tcl_Obj **guardObj) {
  if (obj->typePtr == &NsfMixinregObjType) {
    Mixinreg *mixinRegPtr = obj->internalRep.twoPtrValue.ptr1;
    *guardObj = mixinRegPtr->guardObj;
    *clPtr = mixinRegPtr->mixin;
    return TCL_OK;
  }

  return TCL_ERROR;
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

  assert(filterregPtr);

  /*fprintf(stderr, "FilterregFreeInternalRep freeing filterreg %p class %p guard %p\n",
    filterregPtr, filterregPtr->class, filterregPtr->guardObj);*/

  /*
   * Decrement refCounts
   */
  DECR_REF_COUNT2("filterregPtr->filterObj", filterregPtr->filterObj);
  if (filterregPtr->guardObj) {DECR_REF_COUNT2("filterregPtr->guardObj", filterregPtr->guardObj);}

  /*
   * ... and free structure
   */
  FREE(Filterreg, filterregPtr);
}

/* 
 * dupIntRepProc
 */
static void
FilterregDupInternalRep(
    Tcl_Obj *srcObjPtr,
    Tcl_Obj *dstObjPtr)
{
  register Filterreg *srcPtr = (Filterreg *)srcObjPtr->internalRep.twoPtrValue.ptr1, *dstPtr;

  assert(srcPtr);

#if defined(METHOD_OBJECT_TRACE)
  fprintf(stderr, "FilterregDupInternalRep src %p dst %p\n", srcObjPtr, dstObjPtr);
#endif

  dstPtr = NEW(Filterreg);
  memcpy(dstPtr, srcPtr, sizeof(Filterreg));

  /* 
   * increment refcounts 
   */
  INCR_REF_COUNT2("filterregPtr->filterObj", srcPtr->filterObj);
  if (srcPtr->guardObj) {INCR_REF_COUNT2("FilterregPtr->guardObj", srcPtr->guardObj);}

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
    if (oc == 3 && !strcmp(ObjStr(ov[1]), NsfGlobalStrings[NSF_GUARD_OPTION])) {
      filterObj = ov[0];
      guardObj = ov[2];
      /*fprintf(stderr, "filteradd name = '%s', guard = '%s'\n", ObjStr(name), ObjStr(guard));*/
    } else if (oc == 1) {
      filterObj = ov[0];
    } else {
      return TCL_ERROR;
    }
  } else {
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
  if (guardObj) {INCR_REF_COUNT2("filterregPtr->guardObj", guardObj);}

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
NsfFilterregGet(Tcl_Obj *obj, Tcl_Obj **filterObj, Tcl_Obj **guardObj) {
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
