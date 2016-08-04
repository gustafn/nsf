/*
 * nsfEnumerationType.c --
 *
 *      Provide an API for registering enumeration types
 *      and obtaining their domains.
 *
 * Copyright (C) 2014 Gustaf Neumann
 * Copyright (C) 2016 Stefan Sobernig
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

static Tcl_HashTable enumerationHashTable, *enumerationHashTablePtr = &enumerationHashTable;
static int enumerationTypeRefCount = 0;
static NsfMutex enumerationMutex = 0;

static int Register(Tcl_Interp *interp, const char *domain, Nsf_TypeConverter *converter) nonnull(1) nonnull(3);
/*
 *----------------------------------------------------------------------
 * Nsf_EnumerationTypeInit --
 *
 *    Initialize a hash table to keep the enumeration-type converters.
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

  nonnull_assert(interp != NULL);

  NsfMutexLock(&enumerationMutex);

  if (enumerationTypeRefCount == 0) {
    Nsf_InitFunPtrHashTable(enumerationHashTablePtr);
  }
  enumerationTypeRefCount++;

  NsfMutexUnlock(&enumerationMutex);
}

/*
 *----------------------------------------------------------------------
 * Nsf_EnumerationTypeRelease --
 *
 *    Release and, eventually, delete the hash table for enumeration-type
 *    converters.
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
Nsf_EnumerationTypeRelease(Tcl_Interp *interp) {

  nonnull_assert(interp != NULL);

  NsfMutexLock(&enumerationMutex);

  if (--enumerationTypeRefCount < 1) {
    Tcl_DeleteHashTable(enumerationHashTablePtr);
  }

  NsfMutexUnlock(&enumerationMutex);
}


/*
 *----------------------------------------------------------------------
 * Nsf_EnumerationTypeRegister --
 *
 *    Registers an array of enumeration types upon NSF initialization.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
int
Nsf_EnumerationTypeRegister(Tcl_Interp *interp, Nsf_EnumeratorConverterEntry *typeRecords) {
  Nsf_EnumeratorConverterEntry *ePtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(typeRecords != NULL);

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
 *    Obtain the domain (i.e., the permitted enumeration literals) for a given
 *    enumeration-type converter.
 *
 * Results: 
 *    Domain of permitted enumeration literals as a string or NULL, if
 *    not successful.
 *
 * Side effects:
 *    Sets a mutex lock.
 *
 *----------------------------------------------------------------------
 */
const char *
Nsf_EnumerationTypeGetDomain(Nsf_TypeConverter *converter) {
  Tcl_HashEntry *hPtr;
  /* Tcl_HashSearch hSrch; */
  const char* domain = NULL;

  nonnull_assert(converter != NULL);

  NsfMutexLock(&enumerationMutex);
  hPtr = Nsf_FindFunPtrHashEntry(enumerationHashTablePtr, (Nsf_AnyFun *)converter);
  NsfMutexUnlock(&enumerationMutex);

  if (hPtr != NULL) {
    domain = Tcl_GetHashValue(hPtr);
  }
  
  return domain;
}

/*
 *----------------------------------------------------------------------
 * Register --
 *
 *    Register a enumeration-type converter and its domain.
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    Sets a mutex lock.
 *
 *----------------------------------------------------------------------
 */

static int
Register(Tcl_Interp *interp, const char *domain, Nsf_TypeConverter *converter) {
  Tcl_HashEntry *hPtr;
  int isNew;

  nonnull_assert(interp != NULL);
  nonnull_assert(converter != NULL);

  NsfMutexLock(&enumerationMutex);
  hPtr = Nsf_CreateFunPtrHashEntry(enumerationHashTablePtr, (Nsf_AnyFun *)converter, &isNew);
  NsfMutexUnlock(&enumerationMutex);

  if (isNew != 0) {
    Tcl_SetHashValue(hPtr, domain);
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
