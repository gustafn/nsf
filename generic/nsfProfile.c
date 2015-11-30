/*
 * nsfProfile.c --
 *
 *      Provides profiling information about Next Scripting Framework internals.
 *      For turning on profiling, NSF_PROFILE must be configured.
 *
 * Copyright (C) 2010-2014 Gustaf Neumann
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
static void NsfProfileFillTable(Tcl_HashTable *table, const char *keyStr, double totalMicroSec) nonnull(1) nonnull(2);

static void
NsfProfileFillTable(Tcl_HashTable *table, const char *keyStr, double totalMicroSec) {
  NsfProfileData *value;
  Tcl_HashEntry *hPtr;
  int isNew;

  nonnull_assert(table != NULL);
  nonnull_assert(keyStr != NULL);

  hPtr = Tcl_CreateHashEntry(table, keyStr, &isNew);
  if (isNew != 0) {
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
 * Nsf_ProfileFilterObjCmd --
 *
 *    Stub command to include C-level commands in profile traces.
 *
 * Results:
 *    Tcl result code
 *
 * Side effects:
 *    Perform tracing
 *
 *----------------------------------------------------------------------
 */
static int
Nsf_ProfileFilterObjCmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  int result;
  NsfShadowTclCommandInfo *ti;
  struct timeval start;
  const char *fullMethodName, *label;
  Tcl_DString ds;

  assert(cd);

  fullMethodName = ObjStr(objv[0]);
  ti = (NsfShadowTclCommandInfo *)cd;

  if (ti->nrArgs == 0 || objc < 2) {
    label = fullMethodName;
  } else {
    int i, nrArgs = objc;

    if (nrArgs > ti->nrArgs) {
      nrArgs = ti->nrArgs;
    }

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, fullMethodName, -1);
    for (i = 1; i<=nrArgs; i++) {
      Tcl_DStringAppend(&ds, " ", 1);
      Tcl_DStringAppend(&ds, ObjStr(objv[i]), -1);
    }
    label = ds.string;
  }

  NsfProfileTraceCallAppend(interp, label);

  gettimeofday(&start, NULL);
  result = Tcl_NRCallObjProc(interp, ti->proc, ti->clientData, objc, objv);
  NsfProfileRecordProcData(interp, label, start.tv_sec, start.tv_usec);

  if (label != fullMethodName) {
    Tcl_DStringFree(&ds);
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 * GetPair --
 *
 *    Split a Tcl_Obj into a nameObj and an integer value, if possible
 *
 * Results:
 *    Tcl result
 *
 * Side effects:
 *    Produce warnings for error cases, when "verbose" is on.
 *
 *----------------------------------------------------------------------
 */
static int
GetPair(Tcl_Interp *interp, Tcl_Obj *objPtr, int verbose, Tcl_Obj **nameObjPtr, int *nrArgsPtr)
  nonnull(1) nonnull(2) nonnull(4) nonnull(5);

static int
GetPair(Tcl_Interp *interp, Tcl_Obj *objPtr, int verbose, Tcl_Obj **nameObjPtr, int *nrArgsPtr) {
  int result = TCL_OK, oc;
  Tcl_Obj **ov;

  if (Tcl_ListObjGetElements(interp, objPtr, &oc, &ov) != TCL_OK) {
    if (verbose) {
      NsfLog(interp, NSF_LOG_WARN, "nsfprofile: invalid list element '%s'", ObjStr(objPtr));
      result = TCL_ERROR;
    }
  } else {
    if (oc == 1) {
      *nameObjPtr = ov[0];
    } else if (oc == 2) {
      if (Tcl_GetIntFromObj(interp, ov[1], nrArgsPtr) == TCL_OK) {
        *nameObjPtr = ov[0];
      } else {
        if (verbose) {
          NsfLog(interp, NSF_LOG_WARN, "nsfprofile: second element of '%s' must be an integer", ObjStr(objPtr));
          result = TCL_ERROR;
        }
      }
    } else {
      if (verbose) {
        NsfLog(interp, NSF_LOG_WARN, "nsfprofile: list element '%s' not a valid pair", ObjStr(objPtr));
        result = TCL_ERROR;
      }
    }
  }

  return result;
}

/*
 *----------------------------------------------------------------------
 * NsfProfileTrace --
 *
 *    Function callable via tcl to control trace behavior.
 *
 * Results:
 *    OK
 *
 * Side effects:
 *    update RUNTIME_STATE(interp)->doTrace
 *    and    profilePtr->verbose
 *    and    profilePtr->inmemory
 *
 *----------------------------------------------------------------------
 */
int
NsfProfileTrace(Tcl_Interp *interp, int withEnable, int withVerbose, int withDontsave, Tcl_Obj *builtinObjs) {
  NsfRuntimeState *rst;
  NsfProfile      *profilePtr;
  int              oldProfileState, oc;
  Tcl_Obj        **ov;

  nonnull_assert(interp != NULL);

  rst = RUNTIME_STATE(interp);
  profilePtr = &rst->profile;

  oldProfileState = rst->doTrace;
  rst->doTrace = withEnable;

  /*
   * Turn automically profiling on&off, when trace is turned on/off
   */
  if (withEnable == 1) {
    if (rst->doProfile == 1) {
      NsfLog(interp, NSF_LOG_WARN, "nsfprofile: tracing is already active");
    } else {
      /*
       * Activate profile trace.
       */
      if (builtinObjs != NULL) {
        /*
         * A list of cammands was provided
         */
        if (Tcl_ListObjGetElements(interp, builtinObjs, &oc, &ov) != TCL_OK) {
          NsfLog(interp, NSF_LOG_WARN, "nsfprofile: argument '%s' is not a list of commands", ObjStr(builtinObjs));
        } else {
          NsfShadowTclCommandInfo *ti = NEW_ARRAY(NsfShadowTclCommandInfo, oc);
          int i;

          for (i = 0; i < oc; i++) {
            int nrArgs = 0;
            Tcl_Obj *nameObj = NULL;

            if (GetPair(interp, ov[i], 1, &nameObj, &nrArgs) == TCL_OK) {
              ti[i].nrArgs = nrArgs;
              if (NsfReplaceCommand(interp, nameObj, Nsf_ProfileFilterObjCmd, &ti[i], &ti[i]) != TCL_OK) {
                NsfLog(interp, NSF_LOG_WARN, "nsfprofile: list element '%s' is not a command", ObjStr(nameObj));
              }
            }
          }
          INCR_REF_COUNT(builtinObjs);
          profilePtr->shadowedObjs = builtinObjs;
          profilePtr->shadowedTi = ti;
        }
      }
    }
  } else {
    /*
     * Deactivate profile trace.
     */
    if (profilePtr->shadowedObjs != NULL) {

      if (Tcl_ListObjGetElements(interp, profilePtr->shadowedObjs, &oc, &ov) != TCL_OK) {
        NsfLog(interp, NSF_LOG_WARN, "nsfprofile: shadowed objects are apparently not a list");
      } else {
        int i;

        for (i = 0; i < oc; i++) {
          int nrArgs = 0;
          Tcl_Obj *nameObj = NULL;

          if (GetPair(interp, ov[i], 0, &nameObj, &nrArgs) == TCL_OK) {
            NsfReplaceCommandCleanup(interp, nameObj, &profilePtr->shadowedTi[i]);
          }
        }
      }
      INCR_REF_COUNT(profilePtr->shadowedObjs);

      FREE(NsfShadowTclCommandInfo*, profilePtr->shadowedTi);
      profilePtr->shadowedTi = NULL;
      profilePtr->shadowedObjs = NULL;
      /*fprintf(stderr, "freed profile information\n");*/
    }

  }

  rst->doProfile = withEnable;

  profilePtr->verbose = withVerbose;
  profilePtr->inmemory = (withDontsave == 1) ? 0 : 1;
  Tcl_SetObjResult(interp, Tcl_NewBooleanObj(oldProfileState));

  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 * ReportLine --
 *
 *    Report a profile line via NsfLog(). Since NsfLog() uses a Tcl function,
 *    ReportLine has to turn off profiling to avoid recursive profile
 *    invocation. It is as well necessary to save the interp result.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    logging
 *
 *----------------------------------------------------------------------
 */
static void ReportLine(Tcl_Interp *interp,  NsfRuntimeState *rst, const char *line)
  nonnull(1) nonnull(2) nonnull(3);

static void
ReportLine(Tcl_Interp *interp,  NsfRuntimeState *rst, const char *line) {
  Tcl_Obj *savedResultObj;

  rst->doProfile = 0;

  savedResultObj = Tcl_GetObjResult(interp);
  INCR_REF_COUNT(savedResultObj);

  NsfLog(interp, NSF_LOG_NOTICE, "%s", line);

  Tcl_SetObjResult(interp, savedResultObj);
  DECR_REF_COUNT(savedResultObj);

  rst->doProfile = 1;
}

/*
 *----------------------------------------------------------------------
 * NsfProfileTraceCallAppend, NsfProfileTraceExitAppend --
 *
 *    Low level function to add entries to the trace dstring when functions ar
 *    called or exited.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    update profilePtr->depth and profilePtr->traceDs
 *
 *----------------------------------------------------------------------
 */
void
NsfProfileTraceCallAppend(Tcl_Interp *interp, const char *label) {
  NsfRuntimeState *rst = RUNTIME_STATE(interp);
  NsfProfile *profilePtr = &rst->profile;
  Tcl_DString ds;

  profilePtr->depth ++;

  Tcl_DStringInit(&ds);
  Nsf_DStringPrintf(&ds, "call(%d): %s", profilePtr->depth, label);
  if (profilePtr->verbose) {
    ReportLine(interp, rst, ds.string);
  }
  if (profilePtr->inmemory) {
    Tcl_DStringAppend(&ds, "\n", 1);
    Tcl_DStringAppend(&profilePtr->traceDs, ds.string, ds.length);
  }
  Tcl_DStringFree(&ds);
}

void
NsfProfileTraceExitAppend(Tcl_Interp *interp, const char *label, double duration) {
  NsfRuntimeState *rst = RUNTIME_STATE(interp);
  NsfProfile *profilePtr = &rst->profile;
  Tcl_DString ds;

  Tcl_DStringInit(&ds);
  Nsf_DStringPrintf(&ds, "exit(%d): %s %.0f", profilePtr->depth, label, duration);
  if (profilePtr->verbose) {
    ReportLine(interp, rst, ds.string);
  }
  if (profilePtr->inmemory) {
    Tcl_DStringAppend(&ds, "\n", 1);
    Tcl_DStringAppend(&profilePtr->traceDs, ds.string, ds.length);
  }
  Tcl_DStringFree(&ds);

  profilePtr->depth --;
}


/*
 *----------------------------------------------------------------------
 * NsfProfileObjectLabel, NsfProfileMethodLabel --
 *
 *    Produce a string label for an object or method using in profiling.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    Initializes and fills the passed DString,
 *
 *----------------------------------------------------------------------
 */
static void NsfProfileMethodLabel(Tcl_DString *dsPtr, NsfObject *obj, NsfClass *cl, const char *methodName)
  nonnull(1) nonnull(2) nonnull(4);

void
NsfProfileObjectLabel(Tcl_DString *dsPtr, NsfObject *obj, NsfClass *cl, const char *methodName) {

  nonnull_assert(dsPtr != NULL);
  nonnull_assert(obj != NULL);
  nonnull_assert(methodName != NULL);

  Tcl_DStringAppend(dsPtr, ObjectName(obj), -1);
  Tcl_DStringAppend(dsPtr, " ", 1);
  Tcl_DStringAppend(dsPtr, ClassName(obj->cl), -1);
}

static void
NsfProfileMethodLabel(Tcl_DString *dsPtr, NsfObject *obj, NsfClass *cl, const char *methodName) {

  nonnull_assert(dsPtr != NULL);
  nonnull_assert(obj != NULL);
  nonnull_assert(methodName != NULL);

  if (cl != NULL) {
    Tcl_DStringAppend(dsPtr, ObjStr(cl->object.cmdName), -1);
    Tcl_DStringAppend(dsPtr, " ", 1);
  }
  Tcl_DStringAppendElement(dsPtr, methodName);
}

/*
 *----------------------------------------------------------------------
 * NsfProfileTraceCall, NsfProfileTraceExit --
 *
 *    Add entries to the trace dstring when methods/procs are called or
 *    exited.  This function builds the labels for invocation strings in the
 *    same way as for profiling and calls the lower level function, which does
 *    the recording.
 *
 * Results:
 *    None
 *
 * Side effects:
 *    update profilePtr->depth and profilePtr->traceDs
 *
 *----------------------------------------------------------------------
 */

void
NsfProfileTraceCall(Tcl_Interp *interp, NsfObject *object, NsfClass *cl, const char *methodName) {
  NsfRuntimeState *rst = RUNTIME_STATE(interp);

  if (rst->doTrace) {
    Tcl_DString ds, traceLabel;

    Tcl_DStringInit(&ds);
    NsfProfileObjectLabel(&ds, object, cl, methodName);

    Tcl_DStringInit(&traceLabel);
    Tcl_DStringAppendElement(&traceLabel, Tcl_DStringValue(&ds));
    Tcl_DStringAppend(&traceLabel, " ", 1);

    Tcl_DStringTrunc(&ds, 0);
    NsfProfileMethodLabel(&ds, object, cl, methodName);
    Tcl_DStringAppendElement(&traceLabel, Tcl_DStringValue(&ds));

    NsfProfileTraceCallAppend(interp, Tcl_DStringValue(&traceLabel));
    Tcl_DStringFree(&traceLabel);
    Tcl_DStringFree(&ds);
  }
}

void
NsfProfileTraceExit(Tcl_Interp *interp, NsfObject *object, NsfClass *cl, const char *methodName,
                    struct timeval *callTime) {
  NsfRuntimeState *rst = RUNTIME_STATE(interp);

  if (rst->doTrace) {
    Tcl_DString ds, traceLabel;
    double totalMicroSec;
    struct timeval trt;

    gettimeofday(&trt, NULL);
    totalMicroSec = (trt.tv_sec - callTime->tv_sec) * 1000000 + (trt.tv_usec - callTime->tv_usec);

    Tcl_DStringInit(&ds);
    NsfProfileObjectLabel(&ds, object, cl, methodName);

    Tcl_DStringInit(&traceLabel);
    Tcl_DStringAppendElement(&traceLabel, Tcl_DStringValue(&ds));
    Tcl_DStringAppend(&traceLabel, " ", 1);

    Tcl_DStringTrunc(&ds, 0);
    NsfProfileMethodLabel(&ds, object, cl, methodName);
    Tcl_DStringAppendElement(&traceLabel, Tcl_DStringValue(&ds));

    NsfProfileTraceExitAppend(interp, Tcl_DStringValue(&traceLabel), totalMicroSec);
    Tcl_DStringFree(&traceLabel);
    Tcl_DStringFree(&ds);
  }
}

/*
 *----------------------------------------------------------------------
 * NsfProfileRecordMethodData --
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
NsfProfileRecordMethodData(Tcl_Interp *interp, NsfCallStackContent *cscPtr) {
  NsfRuntimeState *rst = RUNTIME_STATE(interp);
  double totalMicroSec;
  NsfObject *obj = cscPtr->self;
  NsfClass *cl = cscPtr->cl;
  Tcl_DString methodKey, objectKey, methodInfo;
  NsfProfile *profilePtr = &rst->profile;
  struct timeval trt;

  nonnull_assert(interp != NULL);
  nonnull_assert(cscPtr != NULL);

  gettimeofday(&trt, NULL);

  totalMicroSec = (trt.tv_sec - cscPtr->startSec) * 1000000 + (trt.tv_usec - cscPtr->startUsec);
  profilePtr->overallTime += totalMicroSec;

  if (obj->teardown == 0 || !obj->id) {
    return;
  }

  Tcl_DStringInit(&objectKey);
  NsfProfileObjectLabel(&objectKey, obj, NULL, cscPtr->methodName);

  Tcl_DStringInit(&methodInfo);
  Tcl_DStringInit(&methodKey);
  NsfProfileMethodLabel(&methodInfo, obj, cl, cscPtr->methodName);

  if (rst->doTrace) {
    Tcl_DString traceKey;

    Tcl_DStringInit(&traceKey);
    Tcl_DStringAppendElement(&traceKey, Tcl_DStringValue(&objectKey));
    Tcl_DStringAppend(&traceKey, " ", 1);
    Tcl_DStringAppendElement(&traceKey, Tcl_DStringValue(&methodInfo));
    NsfProfileTraceExitAppend(interp, Tcl_DStringValue(&traceKey), totalMicroSec);
    Tcl_DStringFree(&traceKey);
  }

  /*
   * Append method to object key as needed by statisitics (but not by trace)
   */
  Tcl_DStringAppendElement(&objectKey, cscPtr->methodName);

  /*
   * Build method key, containing actual method info and caller method info.
   */
  Tcl_DStringInit(&methodKey);
  Tcl_DStringAppend(&methodKey, "{", 1);
  Tcl_DStringAppend(&methodKey, Tcl_DStringValue(&methodInfo), Tcl_DStringLength(&methodInfo));
  Tcl_DStringAppend(&methodKey, "}", 1);

  {
    NsfCallStackContent *cscPtrTop = NsfCallStackGetTopFrame(interp, NULL);
    if (cscPtrTop != NULL) {
      Tcl_DStringAppend(&methodKey, " {", 2);
      NsfProfileMethodLabel(&methodKey, cscPtrTop->self, cscPtrTop->cl, cscPtrTop->methodName);
      Tcl_DStringAppend(&methodKey, "}", 1);
    } else {
      Tcl_DStringAppend(&methodKey, " {}", 3);
    }
  }

  NsfProfileFillTable(&profilePtr->objectData, Tcl_DStringValue(&objectKey), totalMicroSec);
  NsfProfileFillTable(&profilePtr->methodData, Tcl_DStringValue(&methodKey), totalMicroSec);
  Tcl_DStringFree(&objectKey);
  Tcl_DStringFree(&methodKey);
  Tcl_DStringFree(&methodInfo);
}



/*
 *----------------------------------------------------------------------
 * NsfProfileRecordProcData --
 *
 *    This function is invoked, when a call of a nsf::proc. It records
 *    time spent and count per nsf::proc.
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
NsfProfileRecordProcData(Tcl_Interp *interp, const char *methodName, long startSec, long startUsec) {
  NsfRuntimeState *rst = RUNTIME_STATE(interp);
  NsfProfile *profilePtr = &rst->profile;
  double totalMicroSec;
  struct timeval trt;

  nonnull_assert(interp != NULL);
  nonnull_assert(methodName != NULL);

  gettimeofday(&trt, NULL);

  totalMicroSec = (trt.tv_sec - startSec) * 1000000 + (trt.tv_usec - startUsec);
  profilePtr->overallTime += totalMicroSec;

  if (rst->doTrace) {
    NsfProfileTraceExitAppend(interp, methodName, totalMicroSec);
  }

  NsfProfileFillTable(&profilePtr->procData, methodName, totalMicroSec);
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


static void NsfProfileClearTable(Tcl_HashTable *table) nonnull(1);

static void
NsfProfileClearTable(Tcl_HashTable *table) {
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  nonnull_assert(table != NULL);

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

  nonnull_assert(interp != NULL);

  NsfProfileClearTable(&profilePtr->objectData);
  NsfProfileClearTable(&profilePtr->methodData);
  NsfProfileClearTable(&profilePtr->procData);

  gettimeofday(&trt, NULL);
  profilePtr->startSec = trt.tv_sec;
  profilePtr->startUSec = trt.tv_usec;
  profilePtr->overallTime = 0;
  profilePtr->depth = 0;

  Tcl_DStringTrunc(&profilePtr->traceDs, 0);
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
static Tcl_Obj* NsfProfileGetTable(Tcl_Interp *interp, Tcl_HashTable *table) nonnull(1) nonnull(2);

static Tcl_Obj*
NsfProfileGetTable(Tcl_Interp *interp, Tcl_HashTable *table) {
  Tcl_Obj *list = Tcl_NewListObj(0, NULL);
  Tcl_HashSearch hSrch;
  Tcl_HashEntry *hPtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(table != NULL);

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

  nonnull_assert(interp != NULL);

  gettimeofday(&trt, NULL);
  totalMicroSec = (trt.tv_sec - profilePtr->startSec) * 1000000 + (trt.tv_usec - profilePtr->startUSec);

  Tcl_ListObjAppendElement(interp, list, Tcl_NewIntObj(totalMicroSec));
  Tcl_ListObjAppendElement(interp, list, Tcl_NewIntObj(profilePtr->overallTime));
  Tcl_ListObjAppendElement(interp, list, NsfProfileGetTable(interp, &profilePtr->objectData));
  Tcl_ListObjAppendElement(interp, list, NsfProfileGetTable(interp, &profilePtr->methodData));
  Tcl_ListObjAppendElement(interp, list, NsfProfileGetTable(interp, &profilePtr->procData));
  Tcl_ListObjAppendElement(interp, list, Tcl_NewStringObj(profilePtr->traceDs.string, profilePtr->traceDs.length));

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

  nonnull_assert(interp != NULL);

  Tcl_InitHashTable(&profilePtr->objectData, TCL_STRING_KEYS);
  Tcl_InitHashTable(&profilePtr->methodData, TCL_STRING_KEYS);
  Tcl_InitHashTable(&profilePtr->procData, TCL_STRING_KEYS);

  gettimeofday(&trt, NULL);
  profilePtr->startSec = trt.tv_sec;
  profilePtr->startUSec = trt.tv_usec;
  profilePtr->overallTime = 0;
  profilePtr->depth = 0;
  Tcl_DStringInit(&profilePtr->traceDs);
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

  nonnull_assert(interp != NULL);

  NsfProfileClearData(interp);
  Tcl_DeleteHashTable(&profilePtr->objectData);
  Tcl_DeleteHashTable(&profilePtr->methodData);
  Tcl_DeleteHashTable(&profilePtr->procData);
  Tcl_DStringFree(&profilePtr->traceDs);
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
