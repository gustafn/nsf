/*
 *  nsfEnumerationType.c --
 *
 *      Provide API for registering enumeration types
 *      and obtaining their domain.
 *
 *  Copyright (C) 2014 Gustaf Neumann
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

#include "nsfInt.h"

static Tcl_HashTable enumerationHashTable, *enumerationHashTablePtr = &enumerationHashTable;
static int enumerationTypeRefCount = 0;
static NsfMutex enumerationMutex = 0;

static int Register(Tcl_Interp *interp, CONST char* domain, Nsf_TypeConverter *converter) nonnull(1) nonnull(3);
/*
 *----------------------------------------------------------------------
 * Nsf_EnumerationTypeInit --
 *
 *    Initialize enumeration type converters
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
Nsf_EnumerationTypeInit(Tcl_Interp *interp) {

  assert(interp);

  NsfMutexLock(&enumerationMutex);

  if (enumerationTypeRefCount == 0) {
    Tcl_InitHashTable(enumerationHashTablePtr, TCL_STRING_KEYS);
  }
  enumerationTypeRefCount++;

  NsfMutexUnlock(&enumerationMutex);
}

/*
 *----------------------------------------------------------------------
 * Nsf_EnumerationTypeRegister --
 *
 *    Register an array of enumeration types
 *
 * Results:
 *    TCL_OK
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
int
Nsf_EnumerationTypeRegister(Tcl_Interp *interp, Nsf_EnumeratorConverterEntry *typeRecords) {
  Nsf_EnumeratorConverterEntry *ePtr;

  assert(interp);
  assert(typeRecords);

  for (ePtr = typeRecords; ePtr->converter; ePtr++) {
    int result = Register(interp, ePtr->domain, ePtr->converter);
    if (unlikely(result != TCL_OK)) {
      return result;
    }
  }

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * Nsf_EnumerationTypeGetDomain --
 *
 *    Obtain the domain from an enumeration type converter
 *
 * Results:
 *    domain as a string or NULL, if not successful
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
CONST char *
Nsf_EnumerationTypeGetDomain(Nsf_TypeConverter *converter) {
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch hSrch;
  CONST char* domain = NULL;

  assert(converter);

  NsfMutexLock(&enumerationMutex);

  for (hPtr = Tcl_FirstHashEntry(enumerationHashTablePtr, &hSrch); hPtr != NULL;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    void *ptr = Tcl_GetHashValue(hPtr);

    if (ptr == converter) {
      domain = Tcl_GetHashKey(enumerationHashTablePtr, hPtr);
      break;
    }
  }
  NsfMutexUnlock(&enumerationMutex);

  return domain;
}

/*
 *----------------------------------------------------------------------
 * Register --
 *
 *    Register a enumeration type converter and its domain.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

static int
Register(Tcl_Interp *interp, CONST char* domain, Nsf_TypeConverter *converter) {
  Tcl_HashEntry *hPtr;
  int isNew;

  assert(interp);
  assert(converter);

  NsfMutexLock(&enumerationMutex);
  hPtr = Tcl_CreateHashEntry(enumerationHashTablePtr, domain, &isNew);
  NsfMutexUnlock(&enumerationMutex);

  if (isNew) {
    Tcl_SetHashValue(hPtr, converter);
  } else {
    /*
     * In general, it would make sense to return an error here, but for
     * multiple interps (e.g. slave interps) the register happens per
     * interp. So, not even a warning seems here appropriate
     */
    /*return NsfPrintError(interp, "type converter %s is already registered", domain);
      NsfLog(interp, NSF_LOG_WARN, "type converter %s is already registered", domain);
    */
  }
  return TCL_OK;
}


/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */
