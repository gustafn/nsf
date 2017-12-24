/*
 * nsfPointer.c --
 *
 *      Provide API for accessing mallocated data via Tcl.
 *      This is used e.g. via the MongoDB interface.
 *
 * Copyright (C) 2011-2014 Gustaf Neumann
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
 */

#include "nsfInt.h"

static Tcl_HashTable pointerHashTable, *pointerHashTablePtr = &pointerHashTable;
static int pointerTableRefCount = 0;
static NsfMutex pointerMutex = 0;

/*
 *----------------------------------------------------------------------
 *
 * Nsf_PointerAdd --
 *
 *      Add an entry to our locally maintained hash table and set its
 *      value to the provided valuePtr. The keys are generated based on
 *      the passed type and the counter obtained from the type
 *      registration.
 *
 * Results:
 *      Tcl result code
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Nsf_PointerAdd(Tcl_Interp *interp, char *buffer, const char *typeName, void *valuePtr) {
  int *counterPtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(buffer != NULL);
  nonnull_assert(typeName != NULL);
  nonnull_assert(valuePtr != NULL);

  counterPtr = Nsf_PointerTypeLookup(interp, typeName);
  if (counterPtr != NULL) {
    Tcl_DString ds, *dsPtr = &ds;
    Tcl_HashEntry *hPtr;
    int isNew;

    Tcl_DStringInit(dsPtr);
    Tcl_DStringAppend(dsPtr, typeName, -1);
    Tcl_DStringAppend(dsPtr, ":%d", 3);
    NsfMutexLock(&pointerMutex);
    sprintf(buffer, Tcl_DStringValue(dsPtr), (*counterPtr)++);
    hPtr = Tcl_CreateHashEntry(pointerHashTablePtr, buffer, &isNew);
    NsfMutexUnlock(&pointerMutex);
    Tcl_SetHashValue(hPtr, valuePtr);
    /*fprintf(stderr, "Nsf_PointerAdd key '%s' prefix '%s' => %p value %p\n", buffer, typeName, hPtr, valuePtr);*/

    Tcl_DStringFree(dsPtr);
  } else {
    return NsfPrintError(interp, "no type converter for %s registered", typeName);
  }
  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * Nsf_PointerGet --
 *
 *      Get an entry to our locally maintained hash table and make sure
 *      that the prefix matches (this ensures that the right type of
 *      entry is obtained). If the prefix does not match, or there is no
 *      such entry in the table, the function returns NULL.
 *
 * Results:
 *      valuePtr or NULL.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static void * Nsf_PointerGet(char *key, const char *prefix) nonnull(1) nonnull(2);

static void *
Nsf_PointerGet(char *key, const char *prefix) {
  void *valuePtr = NULL;

  nonnull_assert(key != NULL);
  nonnull_assert(prefix != NULL);

  /* make sure to return the right type of hash entry */
  if (strncmp(prefix, key, strlen(prefix)) == 0) {
    Tcl_HashEntry *hPtr;

    NsfMutexLock(&pointerMutex);
    hPtr = Tcl_CreateHashEntry(pointerHashTablePtr, key, NULL);

    if (hPtr != NULL) {
      valuePtr = Tcl_GetHashValue(hPtr);
    }
    NsfMutexUnlock(&pointerMutex);
  }
  return valuePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Nsf_PointerGetHptr --
 *
 *      Find for a pointer the associated key for a valuePtr (reverse
 *      lookup). The current implementation is quite slow in case there
 *      are a high number of pointer values registered (which should not
 *      be the case for the current usage patterns).  It could certainly
 *      be improved by a second hash table. The function should be run
 *      under a callers mutex.
 *
 * Results:
 *      key or NULL.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static Tcl_HashEntry * Nsf_PointerGetHptr(void *valuePtr) nonnull(1);

static Tcl_HashEntry *
Nsf_PointerGetHptr(void *valuePtr) {
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch hSrch;

  nonnull_assert(valuePtr != NULL);

  for (hPtr = Tcl_FirstHashEntry(pointerHashTablePtr, &hSrch);
       hPtr != NULL;
       hPtr = Tcl_NextHashEntry(&hSrch)) {
    void *ptr = Tcl_GetHashValue(hPtr);
    if (ptr == valuePtr) {
      return hPtr;
    }
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Nsf_PointerDelete --
 *
 *      Delete an hash entry from the locally maintained hash table and
 *      free the associated memory, if the hash entry is
 *      found. Normally, the key should be provided. If the key is not
 *      available, we perform a reverse lookup from the hash table.
 *
 * Results:
 *      valuePtr or NULL.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
Nsf_PointerDelete(const char *key, void *valuePtr, int free) {
  Tcl_HashEntry *hPtr;
  int result;

  nonnull_assert(valuePtr != NULL);

  NsfMutexLock(&pointerMutex);
  hPtr = (key != NULL)
    ? Tcl_CreateHashEntry(pointerHashTablePtr, key, NULL)
    : Nsf_PointerGetHptr(valuePtr);
  if (hPtr != NULL) {
    if (free != 0) {
      ckfree((char *)valuePtr);
    }
    Tcl_DeleteHashEntry(hPtr);
    result = TCL_OK;
  } else {
    result = TCL_ERROR;
  }

  NsfMutexUnlock(&pointerMutex);
  return result;
}


/*
 *----------------------------------------------------------------------
 * Nsf_ConvertToPointer --
 *
 *    Nsf_TypeConverter setting the client data (passed to C functions)
 *    to the valuePtr of the opaque structure. This nsf type converter
 *    checks the passed value via the internally maintained pointer hash
 *    table.
 *
 * Results:
 *    Tcl result code, *clientData and **outObjPtr
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

int Nsf_ConvertToPointer(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			 ClientData *clientData, Tcl_Obj **outObjPtr)
  nonnull(1) nonnull(2) nonnull(3) nonnull(4) nonnull(5);

int
Nsf_ConvertToPointer(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
		     ClientData *clientData, Tcl_Obj **outObjPtr) {
  void *valuePtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(objPtr != NULL);
  nonnull_assert(pPtr != NULL);
  nonnull_assert(clientData != NULL);
  nonnull_assert(outObjPtr != NULL);

  *outObjPtr = objPtr;
  valuePtr = Nsf_PointerGet(ObjStr(objPtr), pPtr->type);
  if (valuePtr != NULL) {
    *clientData = valuePtr;
    return TCL_OK;
  }
  return NsfObjErrType(interp, NULL, objPtr, pPtr->type, (Nsf_Param *)pPtr);
}

/*
 *----------------------------------------------------------------------
 * Nsf_PointerTypeRegister --
 *
 *    Register a pointer type which is identified by the type string
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
Nsf_PointerTypeRegister(Tcl_Interp *interp, const char* typeName, int *counterPtr) {
  Tcl_HashEntry *hPtr;
  int isNew;

  nonnull_assert(interp != NULL);
  nonnull_assert(typeName != NULL);
  nonnull_assert(counterPtr != NULL);

  NsfMutexLock(&pointerMutex);

  hPtr = Tcl_CreateHashEntry(pointerHashTablePtr, typeName, &isNew);

  NsfMutexUnlock(&pointerMutex);

  if (isNew != 0) {
    Tcl_SetHashValue(hPtr, counterPtr);
    return TCL_OK;
  } else {
    return NsfPrintError(interp, "type converter %s is already registered", typeName);
  }
}

/*
 *----------------------------------------------------------------------
 * Nsf_PointerTypeLookup --
 *
 *    Lookup of type name. If the type name is registered, return the
 *    converter or NULL otherwise.
 *
 * Results:
 *    TypeConverter on success or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void *
Nsf_PointerTypeLookup(Tcl_Interp *interp, const char* typeName) {
  const Tcl_HashEntry *hPtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(typeName != NULL);

  NsfMutexLock(&pointerMutex);
  hPtr = Tcl_CreateHashEntry(pointerHashTablePtr, typeName, NULL);
  NsfMutexUnlock(&pointerMutex);

  if (hPtr != NULL) {
    return Tcl_GetHashValue(hPtr);
  }
  return NULL;
}

/*
 *----------------------------------------------------------------------
 * Nsf_PointerInit --
 *
 *    Initialize the Pointer converter
 *
 * Results:
 *    void
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
Nsf_PointerInit(Tcl_Interp *interp) {

  nonnull_assert(interp != NULL);

  NsfMutexLock(&pointerMutex);

  if (pointerTableRefCount == 0) {
    Tcl_InitHashTable(pointerHashTablePtr, TCL_STRING_KEYS);
  }
  pointerTableRefCount++;

  /* fprintf(stderr, "Nsf_PointerInit pointerTableRefCount == %d\n", pointerTableRefCount);*/

  NsfMutexUnlock(&pointerMutex);

}

/*
 *----------------------------------------------------------------------
 * Nsf_PointerExit --
 *
 *    Exit handler for the Pointer converter
 *
 * Results:
 *    void
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

void
Nsf_PointerExit(Tcl_Interp *interp) {

  nonnull_assert(interp != NULL);

  NsfMutexLock(&pointerMutex);
  if (--pointerTableRefCount == 0) {

    if (RUNTIME_STATE(interp)->logSeverity == NSF_LOG_DEBUG) {
      Tcl_HashSearch hSrch;
      const Tcl_HashEntry *hPtr;

      for (hPtr = Tcl_FirstHashEntry(pointerHashTablePtr, &hSrch);
           hPtr != NULL;
	   hPtr = Tcl_NextHashEntry(&hSrch)) {
	const char *key      = Tcl_GetHashKey(pointerHashTablePtr, hPtr);
	const void *valuePtr = Tcl_GetHashValue(hPtr);

	/*
	 * We can't use NsfLog here any more, since the Tcl procs are
	 * already deleted.
	 */

	fprintf(stderr, "Nsf_PointerExit: we have still an entry %s with value %p\n", key, valuePtr);
      }
    }

    Tcl_DeleteHashTable(pointerHashTablePtr);
  }
  /*fprintf(stderr, "Nsf_PointerExit pointerTableRefCount == %d\n", pointerTableRefCount);*/

  NsfMutexUnlock(&pointerMutex);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */
