/*
 * nsfFunPtrHashTable.c --
 *
 *      Provide a custom Tcl hashtable type, using function pointers
 *      as hash keys, and a slim wrapper around Tcl's hashtable
 *      API to manage them.
 *
 * Copyright (C) 2016-2017 Gustaf Neumann
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

/*
 * Definitions for HashType funPtr.
 *
 * Background: since it is not guaranteed that sizeof(function
 * pointer) == sizeof(data pointer) (or sizeof(function pointer) <=
 * sizeof(data pointer)), passing function pointers via data pointers
 * - which is what default Tcl hash types do - is potentially
 * dangerous. Therefore, and on top, it is not allowed under ISO
 * C. So, we define our own type that allows to hash on function
 * pointers safely.
 *
 */
static unsigned int FunPtrKey(Tcl_HashTable *tablePtr, VOID *keyPtr);
static int CompareFunPtrKeys(VOID *keyPtr, Tcl_HashEntry *hPtr);
static Tcl_HashEntry *AllocFunPtrEntry(Tcl_HashTable *tablePtr, VOID *keyPtr);


typedef struct funPtrEntry_t {
  Nsf_AnyFun *funPtr;
} funPtrEntry_t;

static Tcl_HashKeyType funPtrHashKeyType = {
  1,                 /* version*/
  0,                 /* flags */
  FunPtrKey,         /* hashKeyProc*/
  CompareFunPtrKeys, /* compareKeysProc */
  AllocFunPtrEntry,  /* allocEntryProc */
  NULL               /* freeEntryProc */
};

/*
 *----------------------------------------------------------------------
 *
 * FunPtrKey --
 *
 *	Computes an unsigned int hash value from a function pointer.
 *
 * Results:
 *	Returns the computed hash.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned int
FunPtrKey(
          Tcl_HashTable *UNUSED(tablePtr), /* Hash table. */
          VOID *keyPtr)		           /* Key from which to compute hash value. */
{
  funPtrEntry_t  *e = (funPtrEntry_t *)keyPtr;
  Nsf_AnyFun *value  = e->funPtr;

  /*
   * This is a very simple approach for obtaining a hash value. Maybe one
   * needs a more sophisticated approach with weird endians machines.
   */
  return PTR2UINT(value);

}

/*
 *----------------------------------------------------------------------
 *
 * CompareFunPtrKeys --
 *
 *	Compares two function pointer keys.
 *
 * Results:
 *	The return value is 0 if they are different and 1 if they are the
 *	same.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CompareFunPtrKeys(
                  VOID *keyPtr,		/* New key to compare. */
                  Tcl_HashEntry *hPtr)	/* Existing key to compare. */
{
  funPtrEntry_t  *e = (funPtrEntry_t *)keyPtr;
  Nsf_AnyFun *existingValue;

  memcpy(&existingValue, &hPtr->key.oneWordValue, sizeof(Nsf_AnyFun *));

  return e->funPtr == existingValue;
}

/*
 *----------------------------------------------------------------------
 *
 * AllocFunPtrEntry --
 *
 *	Allocate space for a Tcl_HashEntry containing the function pointer.
 *
 * Results:
 *	The return value is a pointer to the created entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Tcl_HashEntry *
AllocFunPtrEntry(
                 Tcl_HashTable *UNUSED(tablePtr), /* Hash table. */
                 VOID *keyPtr)		          /* Key to store in the hash table entry. */
{
  funPtrEntry_t   *e = (funPtrEntry_t *)keyPtr;
  Tcl_HashEntry   *hPtr;
  unsigned int     size;
  Nsf_AnyFun  *value = e->funPtr;

  size = sizeof(Tcl_HashEntry) + (sizeof(Nsf_AnyFun *)) - sizeof(hPtr->key);
  if (size < sizeof(Tcl_HashEntry)) {
    size = sizeof(Tcl_HashEntry);
  }
  hPtr = (Tcl_HashEntry *) ckalloc(size);

  memcpy(&hPtr->key.oneWordValue, &value, sizeof(Nsf_AnyFun *));

  hPtr->clientData = 0;

  return hPtr;
}


/*
 *----------------------------------------------------------------------
 * Nsf_InitFunPtrHashTable --
 *
 *    Initializes a hash table structure providing for function
 *    pointers as hash keys. It is a slim wrapper around
 *    Tcl_InitCustomHashTable().
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
Nsf_InitFunPtrHashTable(Tcl_HashTable *tablePtr) {

  nonnull_assert(tablePtr != NULL);
  
  Tcl_InitCustomHashTable(tablePtr, TCL_CUSTOM_PTR_KEYS, &funPtrHashKeyType);

}

/*
 *----------------------------------------------------------------------
 * Nsf_CreateFunPtrHashEntry --
 *
 *    Creates or finds an entry based on a given function-pointer
 *    key. It is a slim wrapper around Tcl_CreateHashEntry().
 *
 * Results:
 *    Returns a pointer to the matching entry.
 *
 * Side effects:
 *    A new entry may be stored in the hash table.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Nsf_CreateFunPtrHashEntry(Tcl_HashTable *tablePtr, Nsf_AnyFun *key, int *isNew) {
  Tcl_HashEntry *hPtr;
  funPtrEntry_t entry;
  
  nonnull_assert(tablePtr != NULL);
  nonnull_assert(key != NULL);

  entry.funPtr = key;
  hPtr = Tcl_CreateHashEntry(tablePtr, (const char *)&entry, isNew);
  return hPtr;
  
}

/*
 *----------------------------------------------------------------------
 * Nsf_FindFunPtrHashEntry --
 *
 *    Finds the entry with a matching function-pointer key in a given
 *    table. It is a slim wrapper around Tcl_FindHashEntry().
 *
 * Results:
 *    Returns a pointer to the matching entry, or NULL.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */

Tcl_HashEntry *
Nsf_FindFunPtrHashEntry(Tcl_HashTable *tablePtr, Nsf_AnyFun *key) {
  Tcl_HashEntry *hPtr;
  funPtrEntry_t entry;
  
  nonnull_assert(tablePtr != NULL);
  nonnull_assert(key != NULL);

  entry.funPtr = key;
  hPtr = Tcl_FindHashEntry(tablePtr, (const char *)&entry);
  return hPtr;
  
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */
