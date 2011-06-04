/* 
 *  
 *  Next Scripting Framework
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *
 *  nsfObj.c --
 *  
 *  Tcl_Obj types provided by the Next Scripting Framework
 *  
 */

#include "nsfInt.h"

/*
 * Mixinreg type
 *
 * The mixin reg type is an Tcl_Obj type carrying a class and a guard
 * object. The string representation might have the form "/cls/" or "/cls/
 * -guard /expr/". When no guard expression is provided (first form), the
 * guard entry is NULL.
 */

typedef struct {
  NsfClass *mixin;
  Tcl_Obj *guardObj;
} MixinReg;

static Tcl_DupInternalRepProc	MixinregDupInteralRep;
static Tcl_FreeInternalRepProc	MixinregFreeInternalRep;
static Tcl_UpdateStringProc	MixinregUpdateString;
static Tcl_SetFromAnyProc       MixinregSetFromAny;

Tcl_ObjType NsfMixinregObjType = {
    "nsfMixinreg",			/* name */
    MixinregFreeInternalRep,		/* freeIntRepProc */
    MixinregDupInteralRep,		/* dupIntRepProc */
    MixinregUpdateString,		/* updateStringProc */
    MixinregSetFromAny			/* setFromAnyProc */
};

/* 
 * Dummy placeholder, should never be called.
 */
static void
MixinregUpdateString(Tcl_Obj *objPtr) {
  Tcl_Panic("MixinregUpdateString %s of type %s should not be called", "updateStringProc",
	    objPtr->typePtr->name);
}

/* 
 * Dummy placeholder, should never be called.
 */
static void
MixinregDupInteralRep(Tcl_Obj *srcPtr, Tcl_Obj *UNUSED(dupPtr)) {
  Tcl_Panic("MixinregDupInteralRep %s of type %s should not be called", "dupStringProc",
	    srcPtr->typePtr->name);
}

/* 
 * freeIntRepProc
 */
static void
MixinregFreeInternalRep(
    register Tcl_Obj *objPtr)	/* Mixinreg structure object with internal
				 * representation to free. */
{
  MixinReg *mixinRegPtr = (MixinReg *)objPtr->internalRep.twoPtrValue.ptr1;

  if (mixinRegPtr != NULL) {
    /*fprintf(stderr, "MixinregFreeInternalRep freeing mixinReg %p class %p guard %p\n",
      mixinRegPtr, mixinRegPtr->class, mixinRegPtr->guardObj);*/
    /*
     * Decrement refCounts
     */
    NsfObjectRefCountDecr(&(mixinRegPtr->mixin)->object);
    if (mixinRegPtr->guardObj) {DECR_REF_COUNT(mixinRegPtr->guardObj);}

    /*
     * ... and free structure
     */
    FREE(MixinReg, mixinRegPtr);
  }
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
  int oc; Tcl_Obj **ov;
  MixinReg *mixinRegPtr;

  if (Tcl_ListObjGetElements(interp, objPtr, &oc, &ov) == TCL_OK) {
    if (oc == 3 && !strcmp(ObjStr(ov[1]), NsfGlobalStrings[NSF_GUARD_OPTION])) {
      nameObj = ov[0];
      guardObj = ov[2];
      /*fprintf(stderr, "mixinadd name = '%s', guard = '%s'\n", ObjStr(name), ObjStr(guard));*/
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
  if (NsfGetClassFromObj(interp, nameObj, (Nsf_Class **)&mixin, 1) != TCL_OK) {
    return NsfObjErrType(interp, "mixin", nameObj, "a class as mixin", NULL);
  }
  
  /*
   * Conversion was ok.
   * Allocate structure ... 
   */
  mixinRegPtr = NEW(MixinReg);
  /*
   * ... and increment refCounts
   */
  NsfObjectRefCountIncr((&mixin->object));
  if (guardObj) {INCR_REF_COUNT(guardObj);}

  mixinRegPtr->mixin = mixin;
  mixinRegPtr->guardObj = guardObj;

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
    MixinReg *mixinRegPtr = obj->internalRep.twoPtrValue.ptr1;
    *guardObj = mixinRegPtr->guardObj;
    *clPtr = mixinRegPtr->mixin;
    return TCL_OK;
  }

  return TCL_ERROR;
}
/*
 * Mixinreg type end
 */
