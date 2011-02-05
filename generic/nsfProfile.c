/* -*- Mode: c++ -*-
 *
 *  Next Scripting Framework 
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *
 *  nsfProfile.c --
 *  
 *  Profiling information printout for Next Scripting Framework 
 *
 *  For profiling infos NSF_PROFILE (nsf.h) flag must be activated
 *  
 */

#include "nsfInt.h"

#if defined(NSF_PROFILE)

typedef struct NsfProfileData {
  long microSec;
  long count;
} NsfProfileData;

/*
 *----------------------------------------------------------------------
 * NsfProfileFillTable --
 *
 *    Insert or Update a keyed entry with provided microseconds and
 *    update the counts for this entry.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    Updated or created profile data entry
 *
 *----------------------------------------------------------------------
 */
static void
NsfProfileFillTable(Tcl_HashTable *table, char *keyStr, double totalMicroSec) {
  NsfProfileData *value;
  Tcl_HashEntry *hPtr;
  int isNew;

  hPtr = Tcl_CreateHashEntry(table, keyStr, &isNew);
  if (isNew) {
    value = (NsfProfileData *)ckalloc(sizeof(NsfProfileData));
    value->microSec = 0;
    value->count = 0;
    Tcl_SetHashValue(hPtr, (ClientData) value);
  } else {
    value = (NsfProfileData *)Tcl_GetHashValue (hPtr);
  }
  value->microSec += totalMicroSec;
  value->count ++;
}

/*
 *----------------------------------------------------------------------
 * NsfProfileEvaluateData --
 *
 *    This function is invoked, when a call of a method ends. It
 *    records profiling information based on the provided call stack
 *    content and the caller. In particular, it records the time spent
 *    in an object (identified with an objectKey) and the time spent
 *    in the method (using methodKey).
 *
 * Results:
 *    None
 *
 * Side effects:
 *    Updated or created profile data entries
 *
 *----------------------------------------------------------------------
 */
void
NsfProfileEvaluateData(Tcl_Interp *interp, NsfCallStackContent *cscPtr) {
  double totalMicroSec;
  NsfObject *obj = cscPtr->self;
  NsfClass *cl = cscPtr->cl;
  long int startSec = cscPtr->startSec;
  long int startUsec = cscPtr->startUsec;
  CONST char *methodName = cscPtr->methodName;
  Tcl_DString methodKey, objectKey;
  NsfProfile *profile = &RUNTIME_STATE(interp)->profile;
  struct timeval trt;

  gettimeofday(&trt, NULL);

  totalMicroSec = (trt.tv_sec - startSec) * 1000000 + (trt.tv_usec - startUsec);
  profile->overallTime += totalMicroSec;

  if (obj->teardown == 0 || !obj->id) {
    return;
  }

  Tcl_DStringInit(&objectKey);
  Tcl_DStringAppend(&objectKey, ObjectName(obj), -1);
  Tcl_DStringAppend(&objectKey, " (", 1);
  Tcl_DStringAppend(&objectKey, ClassName(obj->cl), -1);
  Tcl_DStringAppend(&objectKey, " )", 1);

  Tcl_DStringInit(&methodKey);
  Tcl_DStringAppend(&methodKey, cl ? ObjStr(cl->object.cmdName) : ObjStr(obj->cmdName), -1);
  Tcl_DStringAppend(&methodKey, " ", 1);
  Tcl_DStringAppend(&methodKey, methodName, -1);
  if (cl) {
    Tcl_DStringAppend(&methodKey, " method", 7);
  } else {
    Tcl_DStringAppend(&methodKey, " object-method", 14);
  }

  {
    NsfCallStackContent *cscPtrTop = NsfCallStackGetTopFrame(interp, NULL);
    if (cscPtrTop) {
      NsfClass *cl = cscPtrTop->cl;
      NsfObject *obj = cscPtrTop->self;
      CONST char *methodName = cscPtrTop->methodName;
      
      Tcl_DStringAppend(&methodKey, " ", 1);
      Tcl_DStringAppend(&methodKey, cl ? ObjStr(cl->object.cmdName) : ObjStr(obj->cmdName), -1);
      Tcl_DStringAppend(&methodKey, " ", 1);
      Tcl_DStringAppend(&methodKey, methodName, -1);
      if (cl) {
	Tcl_DStringAppend(&methodKey, " method", 7);
      } else {
	Tcl_DStringAppend(&methodKey, " object-method", 14);
      }
    } else {
      Tcl_DStringAppend(&methodKey, " - - -", 6);
    }
  }

  NsfProfileFillTable(&profile->objectData, Tcl_DStringValue(&objectKey), totalMicroSec);
  NsfProfileFillTable(&profile->methodData, Tcl_DStringValue(&methodKey), totalMicroSec);
  Tcl_DStringFree(&objectKey);
  Tcl_DStringFree(&methodKey);
}

/*
 *----------------------------------------------------------------------
 * NsfProfileClearTable --
 *
 *    Clear all data in a profile table.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    freed profile information.
 *
 *----------------------------------------------------------------------
 */

static void
NsfProfileClearTable(Tcl_HashTable *table) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  assert(table);
  for (hPtr = Tcl_FirstHashEntry(table, &hSrch); hPtr; 
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    NsfProfileData *value = (NsfProfileData *) Tcl_GetHashValue(hPtr);
    ckfree((char *) value);
    Tcl_DeleteHashEntry(hPtr);
  }
}

/*
 *----------------------------------------------------------------------
 * NsfProfileClearData --
 *
 *    Flush all data in all profile tables and reset the time
 *    counters.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    freed profile information.
 *
 *----------------------------------------------------------------------
 */
void
NsfProfileClearData(Tcl_Interp *interp) {
  NsfProfile *profilePtr = &RUNTIME_STATE(interp)->profile;
  struct timeval trt;

  NsfProfileClearTable(&profilePtr->objectData);
  NsfProfileClearTable(&profilePtr->methodData);
  
  gettimeofday(&trt, NULL);
  profilePtr->startSec = trt.tv_sec;
  profilePtr->startUSec = trt.tv_usec;
  profilePtr->overallTime = 0;
}

/*
 *----------------------------------------------------------------------
 * NsfProfileGetTable --
 *
 *    Return the profiling information for the specified profile table
 *    in form of a Tcl list.
 *
 * Results:
 *    Tcl List
 *
 * Side effects:
 *    Nne.
 *
 *----------------------------------------------------------------------
 */
static Tcl_Obj*
NsfProfileGetTable(Tcl_Interp *interp, Tcl_HashTable *table) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  assert(table);
  for (hPtr = Tcl_FirstHashEntry(table, &hSrch); hPtr; 
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    NsfProfileData *value = (NsfProfileData *) Tcl_GetHashValue(hPtr);
    char *key = Tcl_GetHashKey(table, hPtr);
    Tcl_Obj *subList = Tcl_NewListObj(0, NULL);

    Tcl_ListObjAppendElement(interp, subList, Tcl_NewStringObj(key, -1));
    Tcl_ListObjAppendElement(interp, subList, Tcl_NewIntObj(value->microSec));
    Tcl_ListObjAppendElement(interp, subList, Tcl_NewIntObj(value->count));
    Tcl_ListObjAppendElement(interp, list, subList);
  }
  return list;
}

/*
 *----------------------------------------------------------------------
 * NsfProfileGetData --
 *
 *    Return recorded profiling information. This function returns a
 *    list containing (a) the elapsed time since the last clear (or
 *    init), (b) the cumulative time, (c) the list with the per-object
 *    data and (d) the list with the method invocation data.
 *
 * Results:
 *    Tcl List
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
void
NsfProfileGetData(Tcl_Interp *interp) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  NsfProfile *profilePtr = &RUNTIME_STATE(interp)->profile;
  long totalMicroSec;
  struct timeval trt;

  gettimeofday(&trt, NULL);
  totalMicroSec = (trt.tv_sec - profilePtr->startSec) * 1000000 + (trt.tv_usec - profilePtr->startUSec);

  Tcl_ListObjAppendElement(interp, list, Tcl_NewIntObj(totalMicroSec));
  Tcl_ListObjAppendElement(interp, list, Tcl_NewIntObj(profilePtr->overallTime));
  Tcl_ListObjAppendElement(interp, list, NsfProfileGetTable(interp, &profilePtr->objectData));
  Tcl_ListObjAppendElement(interp, list, NsfProfileGetTable(interp, &profilePtr->methodData));

  Tcl_SetObjResult(interp, list);
}

/*
 *----------------------------------------------------------------------
 * NsfProfileInit --
 *
 *    Initialize the profiling information. This is a one-time only
 *    operation and initializes the hash table and the timing
 *    results. The inverse operation is NsfProfileFree()
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
void 
NsfProfileInit(Tcl_Interp *interp) {
  NsfProfile *profilePtr = &RUNTIME_STATE(interp)->profile;
  struct timeval trt;

  Tcl_InitHashTable(&profilePtr->objectData, TCL_STRING_KEYS);
  Tcl_InitHashTable(&profilePtr->methodData, TCL_STRING_KEYS);

  gettimeofday(&trt, NULL);
  profilePtr->startSec = trt.tv_sec;
  profilePtr->startUSec = trt.tv_usec;
  profilePtr->overallTime = 0;
}

/*
 *----------------------------------------------------------------------
 * NsfProfileFree --
 *
 *    Free all profiling information. This is a one-time only
 *    operation only. The inverse operation is NsfProfileInit().
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
void 
NsfProfileFree(Tcl_Interp *interp) {
  NsfProfile *profilePtr = &RUNTIME_STATE(interp)->profile;

  NsfProfileClearData(interp);
  Tcl_DeleteHashTable(&profilePtr->objectData);
  Tcl_DeleteHashTable(&profilePtr->methodData);
}
#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
