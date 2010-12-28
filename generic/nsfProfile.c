/* -*- Mode: c++ -*-
 *
 *  Extended Object Tcl (XOTcl)
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *
 *  nsfProfile.c --
 *  
 *  Profiling information printout for XOTcl
 *
 *  For profiling infos NSF_PROFILE (nsf.h) flag must be activated
 *  
 */

#include "nsfInt.h"

#if defined(NSF_PROFILE)
void
NsfProfileFillTable(Tcl_HashTable* table, Tcl_DString* key,
		 double totalMicroSec) {
  Tcl_HashEntry* hPtr;
  char* keyStr = Tcl_DStringValue(key);
  long int* value;

  hPtr = Tcl_FindHashEntry(table, keyStr);
  if (!hPtr) {
    int nw;
    hPtr = Tcl_CreateHashEntry(table, keyStr, &nw);
    if (!nw)
      return;
    value = (long int*) ckalloc (sizeof(long int));
    *value = 0;
    Tcl_SetHashValue(hPtr, (ClientData) value);
  } else
    value = (long int*) Tcl_GetHashValue (hPtr);

  *value += totalMicroSec;


  /* {
    long int* d = (long int*) Tcl_GetHashValue (hPtr);
    fprintf(stderr, "Entered %s ... %ld\n", Tcl_GetHashKey(table, hPtr), *d);
    }*/

}

void
NsfProfileEvaluateData(Tcl_Interp* interp, long int startSec, long int startUsec,
		    NsfObject* obj, NsfClass *cl, char *methodName) {
  double totalMicroSec;
  struct timeval trt;
  Tcl_DString objectKey, methodKey;

  NsfProfile* profile = &RUNTIME_STATE(interp)->profile;

  gettimeofday(&trt, NULL);

  totalMicroSec = (trt.tv_sec - startSec) * 1000000 +
    (trt.tv_usec - startUsec);

  profile->overallTime += totalMicroSec;

  if (obj->teardown == 0 || !obj->id || obj->destroyCalled) {
    return;
  }
  Tcl_DStringInit(&objectKey);
  Tcl_DStringAppend(&objectKey, ObjStr(obj->cmdName), -1);

  Tcl_DStringInit(&methodKey);
  Tcl_DStringAppend(&methodKey, cl ? ObjStr(cl->object.cmdName) : ObjStr(obj->cmdName), -1);
  Tcl_DStringAppend(&methodKey, "->", 2);
  Tcl_DStringAppend(&methodKey, methodName, -1);
  if (cl)
    Tcl_DStringAppend(&methodKey, " (instproc)", 11);
  else
    Tcl_DStringAppend(&methodKey, " (proc)", 7);

  NsfProfileFillTable(&profile->objectData, &objectKey, totalMicroSec);
  NsfProfileFillTable(&profile->methodData, &methodKey, totalMicroSec);
  Tcl_DStringFree(&objectKey);
  Tcl_StringFree(&methodKey);
}

void
NsfProfilePrintTable(Tcl_HashTable* table) {
  Tcl_HashEntry* topValueHPtr;
  long int* topValue;

  do {
    Tcl_HashSearch hSrch;
    Tcl_HashEntry* hPtr = table ?
      Tcl_FirstHashEntry(table, &hSrch) : 0;
    char* topKey = 0;

    topValueHPtr = 0;
    topValue = 0;

    for (; hPtr != 0; hPtr = Tcl_NextHashEntry(&hSrch)) {
      long int *val = (long int*) Tcl_GetHashValue(hPtr);
      if (val && (!topValue || (topValue && *val >= *topValue))) {
	topValue = val;
	topValueHPtr = hPtr;
	topKey =  Tcl_GetHashKey(table, hPtr);
      }
    }

    if (topValueHPtr) {
      fprintf(stderr, "  %15ld   %s\n", *topValue, topKey);
      ckfree((char*) topValue);
      Tcl_DeleteHashEntry(topValueHPtr);
    }
  } while (topValueHPtr);
}

void
NsfProfilePrintData(Tcl_Interp *interp) {
  NsfProfile* profile = &RUNTIME_STATE(interp)->profile;

  fprintf(stderr, "------------------------------------------------------------------\n");
  fprintf(stderr, "\nXOTcl Profile Information\n\n");
  fprintf(stderr, "------------------------------------------------------------------\n");
  fprintf(stderr, "Overall Elapsed Time              %ld\n",
	  profile->overallTime);
  fprintf(stderr, "------------------------------------------------------------------\n");
  fprintf(stderr, "     MICROSECONDS   OBJECT-NAME\n");
  NsfProfilePrintTable(&profile->objectData);
  fprintf(stderr, "------------------------------------------------------------------\n");
  fprintf(stderr, "     MICROSECONDS   (CL/OBJ)->METHOD-NAME\n");
  NsfProfilePrintTable(&profile->methodData);
  fprintf(stderr, "------------------------------------------------------------------\n");
}

void 
NsfProfileInit(Tcl_Interp *interp) {
  RUNTIME_STATE(interp)->profile.overallTime = 0;
  Tcl_InitHashTable(&RUNTIME_STATE(interp)->profile.objectData,
		    TCL_STRING_KEYS);
  Tcl_InitHashTable(&RUNTIME_STATE(interp)->profile.methodData,
		    TCL_STRING_KEYS);
}

#endif
