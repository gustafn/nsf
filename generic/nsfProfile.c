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

static void
NsfProfilePrintTable(Tcl_HashTable *table) {
  Tcl_HashEntry *topValueHPtr;

  assert(table);

  do {
    NsfProfileData *topValue;
    Tcl_HashSearch hSrch;
    Tcl_HashEntry *hPtr;
    char *topKey = NULL;

    topValueHPtr = NULL;
    topValue = NULL;

    for (hPtr = Tcl_FirstHashEntry(table, &hSrch); hPtr; 
	 hPtr = Tcl_NextHashEntry(&hSrch)) {
      NsfProfileData *value = (NsfProfileData *) Tcl_GetHashValue(hPtr);
      if (value && 
	  (!topValue || (topValue && value->microSec >= topValue->microSec))) {
	topValue = value;
	topValueHPtr = hPtr;
	topKey = Tcl_GetHashKey(table, hPtr);
      }
    }

    if (topValueHPtr) {
      fprintf(stderr, "  %15ld %10ld   %s\n", topValue->microSec, topValue->count, topKey);
      ckfree((char *) topValue);
      Tcl_DeleteHashEntry(topValueHPtr);
    }
  } while (topValueHPtr);
}

void
NsfProfilePrintData(Tcl_Interp *interp) {
  NsfProfile *profilePtr = &RUNTIME_STATE(interp)->profile;
  struct timeval trt;

  fprintf(stderr, "------------------------------------------------------------------\n");
  fprintf(stderr, "\nXOTcl Profile Information\n\n");
  fprintf(stderr, "------------------------------------------------------------------\n");
  fprintf(stderr, "Overall Elapsed Time              %ld\n", profilePtr->overallTime);
  fprintf(stderr, "------------------------------------------------------------------\n");
  fprintf(stderr, "     MICROSECONDS      COUNT   OBJECT-NAME\n");
  NsfProfilePrintTable(&profilePtr->objectData);
  fprintf(stderr, "------------------------------------------------------------------\n");
  fprintf(stderr, "     MICROSECONDS      COUNT   (CL/OBJ)->METHOD-NAME\n");
  NsfProfilePrintTable(&profilePtr->methodData);
  fprintf(stderr, "------------------------------------------------------------------\n");

  gettimeofday(&trt, NULL);
  profilePtr->startSec = trt.tv_sec;
  profilePtr->startUSec = trt.tv_usec;
  profilePtr->overallTime = 0;
}

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

void 
NsfProfileInit(Tcl_Interp *interp) {
  NsfProfile *profilePtr = &RUNTIME_STATE(interp)->profile;
  struct timeval trt;

  Tcl_InitHashTable(&RUNTIME_STATE(interp)->profile.objectData,
		    TCL_STRING_KEYS);
  Tcl_InitHashTable(&RUNTIME_STATE(interp)->profile.methodData,
		    TCL_STRING_KEYS);

  gettimeofday(&trt, NULL);
  profilePtr->startSec = trt.tv_sec;
  profilePtr->startUSec = trt.tv_usec;
  profilePtr->overallTime = 0;
}

#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
