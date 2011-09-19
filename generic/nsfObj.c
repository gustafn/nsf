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

static Tcl_FreeInternalRepProc	FlagFreeInternalRep;

Tcl_ObjType NsfFlagObjType = {
    "nsfFlag",			/* name */
    FlagFreeInternalRep,	/* freeIntRepProc */
    NULL,			/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};

/* 
 * freeIntRepProc
 */
static void
FlagFreeInternalRep(
    register Tcl_Obj *objPtr)	/* Tcl_Obj structure object with internal
				 * representation to free. */
{
  NsfFlag *flagPtr = (NsfFlag *)objPtr->internalRep.twoPtrValue.ptr1;

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
    objPtr->internalRep.twoPtrValue.ptr1 = NULL; // TODO: needed?
  }
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
  if (likely(objPtr->typePtr != &NsfFlagObjType)) {
    TclFreeIntRep(objPtr);
    flagPtr = NEW(NsfFlag);
    objPtr->internalRep.twoPtrValue.ptr1 = (void *)flagPtr;
    objPtr->internalRep.twoPtrValue.ptr2 = NULL;
    objPtr->typePtr = &NsfFlagObjType;
  } else {
    flagPtr = (NsfFlag *)objPtr->internalRep.twoPtrValue.ptr1;

    //fprintf(stderr, "NsfFlagObjSet %p reuses interal rep, serial (%d/%d)\n",
    //	    objPtr, flagPtr->serial, serial);

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
} MixinReg;

static Tcl_FreeInternalRepProc	MixinregFreeInternalRep;
static Tcl_SetFromAnyProc       MixinregSetFromAny;

Tcl_ObjType NsfMixinregObjType = {
    "nsfMixinreg",			/* name */
    MixinregFreeInternalRep,		/* freeIntRepProc */
    NULL,				/* dupIntRepProc */
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
  MixinReg *mixinRegPtr = (MixinReg *)objPtr->internalRep.twoPtrValue.ptr1;

  if (mixinRegPtr != NULL) {

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
  MixinReg *mixinRegPtr;
  int oc; Tcl_Obj **ov;

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
  if (NsfGetClassFromObj(interp, nameObj, &mixin, 1) != TCL_OK) {
    return NsfObjErrType(interp, "mixin", nameObj, "a class as mixin", NULL);
  }
  
  /*
   * Conversion was ok.
   * Allocate structure ... 
   */
  mixinRegPtr = NEW(MixinReg);
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
    MixinReg *mixinRegPtr = obj->internalRep.twoPtrValue.ptr1;
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
static Tcl_SetFromAnyProc       FilterregSetFromAny;

Tcl_ObjType NsfFilterregObjType = {
    "nsfFilterreg",			/* name */
    FilterregFreeInternalRep,		/* freeIntRepProc */
    NULL,				/* dupIntRepProc */
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

  if (filterregPtr != NULL) {

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
      /*fprintf(stderr, "mixinadd name = '%s', guard = '%s'\n", ObjStr(name), ObjStr(guard));*/
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
    filterregPtr, filterregPtr->mixin, filterregPtr->guardObj);*/

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
