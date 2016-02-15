/*
 * nsfCmdDefinitions.c --
 *
 *      Provide API for registering method definitions
 *      and obtaining this data for introspection
 *
 * Copyright (C) 2014-2016 Gustaf Neumann
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

static Tcl_HashTable cmdDefinitonHashTable, *cmdDefinitonHashTablePtr = &cmdDefinitonHashTable;
static int cmdDefinitonRefCount = 0;
static NsfMutex cmdDefinitonMutex = 0;

static int Register(Tcl_Interp *interp, Nsf_methodDefinition *methodDefinition);


/*
 * Defintions for HashType cmdPtr.
 *
 * Background: since it is not guaranteed that sizeof(function pointer) ==
 * sizeof(data pointer) (or sizeof(function pointer) <= sizeof(data pointer)),
 * passing function pointers via data pointers - which is the default tcl hash
 * types do - is dangerous. So we define our own type that allows to hash on
 * function pointers safely.
 *
 */
static unsigned int CmdPtrKey(Tcl_HashTable *tablePtr, VOID *keyPtr);
static int CompareCmdPtrKeys(VOID *keyPtr, Tcl_HashEntry *hPtr);
static Tcl_HashEntry *AllocCmdPtrEntry(Tcl_HashTable *tablePtr, VOID *keyPtr);

typedef struct cmdPtrEntry_t {
  Tcl_ObjCmdProc *proc;
} cmdPtrEntry_t;

static Tcl_HashKeyType cmdPtrHashKeyType = {
  1,                 /* version*/
  0,                 /* flags */
  CmdPtrKey,         /* hashKeyProc*/
  CompareCmdPtrKeys, /* compareKeysProc */
  AllocCmdPtrEntry,  /* allocEntryProc */
  NULL,              /* freeEntryProc */
};

/*
 *----------------------------------------------------------------------
 *
 * CmdPtrKey --
 *
 *	Compute a unsigned int hash value from a function pointer.
 *
 * Results:
 *	Hash value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned int
CmdPtrKey(
          Tcl_HashTable *tablePtr, /* Hash table. */
          VOID *keyPtr)		   /* Key from which to compute hash value. */
{
  cmdPtrEntry_t  *cmdEntryPtr = (cmdPtrEntry_t *)keyPtr;
  Tcl_ObjCmdProc *value  = cmdEntryPtr->proc;

  //fprintf(stderr, "=== hash from %p = %u // 0x%x\n", (void *)value, PTR2UINT(value), PTR2UINT(value));
  /*
   * This is a very simple approach for obtaining a hash value. Maybe one
   * needs a more sophisticated approach with wierd endians machines.
   */
  return PTR2UINT(value);

  /*
    as a reference: tcl's string hash functions

    register unsigned int result;
    register int c;
    result = 0;

    for (c=*string++ ; c ; c=*string++) {
    result += (result<<3) + c;
    }
    return result;
  */
}

/*
 *----------------------------------------------------------------------
 *
 * CompareCmdPtrKeys --
 *
 *	Compares two cmd ptr keys.
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
CompareCmdPtrKeys(
                  VOID *keyPtr,		/* New key to compare. */
                  Tcl_HashEntry *hPtr)	/* Existing key to compare. */
{
  cmdPtrEntry_t  *cmdEntryPtr = (cmdPtrEntry_t *)keyPtr;
  Tcl_ObjCmdProc *existingValue;

  memcpy(&existingValue, &hPtr->key.oneWordValue, sizeof(Tcl_ObjCmdProc *));

  //fprintf(stderr, "=== compare new %p existing %p\n", (void *)cmdPtr->proc, (void *)existingValue);

  return cmdEntryPtr->proc == existingValue;
}

/*
 *----------------------------------------------------------------------
 *
 * AllocCmdPtrEntry --
 *
 *	Allocate space for a Tcl_HashEntry containing the cmd ptr
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
AllocCmdPtrEntry(
                 Tcl_HashTable *tablePtr,   /* Hash table. */
                 VOID *keyPtr)		    /* Key to store in the hash table entry. */
{
  cmdPtrEntry_t   *cmdEntryPtr = (cmdPtrEntry_t *)keyPtr;
  Tcl_HashEntry   *hPtr;
  unsigned int     size;
  Tcl_ObjCmdProc  *value = cmdEntryPtr->proc;

  size = sizeof(Tcl_HashEntry) + (sizeof(Tcl_ObjCmdProc *)) - sizeof(hPtr->key);
  if (size < sizeof(Tcl_HashEntry)) {
    size = sizeof(Tcl_HashEntry);
  }
  //fprintf(stderr, "=== alloc entry %p\n", (void *)value);
  hPtr = (Tcl_HashEntry *) ckalloc(size);

  //fprintf(stderr, "=== trying to copy %ld bytes from %p to %p\n", sizeof(Tcl_ObjCmdProc *), (void *)value, &hPtr->key.oneWordValue);
  memcpy(&hPtr->key.oneWordValue, &value, sizeof(Tcl_ObjCmdProc *));

  hPtr->clientData = 0;

  return hPtr;
}

/*
 *----------------------------------------------------------------------
 * Nsf_CmdDefinitionInit --
 *
 *    Initialize cmd definition structures
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
Nsf_CmdDefinitionInit(Tcl_Interp *interp) {

  nonnull_assert(interp != NULL);

  NsfMutexLock(&cmdDefinitonMutex);

  if (cmdDefinitonRefCount == 0) {
    // Tcl_InitHashTable(cmdDefinitonHashTablePtr, TCL_ONE_WORD_KEYS);
    Tcl_InitCustomHashTable(cmdDefinitonHashTablePtr, TCL_CUSTOM_PTR_KEYS, &cmdPtrHashKeyType);
  }
  cmdDefinitonRefCount++;

  NsfMutexUnlock(&cmdDefinitonMutex);
}


/*
 *----------------------------------------------------------------------
 * Nsf_CmdDefinitionRegister --
 *
 *    Register an array of cmd definitons
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
Nsf_CmdDefinitionRegister(Tcl_Interp *interp, Nsf_methodDefinition *definitionRecords) {
  Nsf_methodDefinition *ePtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(definitionRecords != NULL);

  for (ePtr = definitionRecords; ePtr->methodName; ePtr++) {
    Register(interp, ePtr);
  }

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * Nsf_CmdDefinitionGet --
 *
 *    Obtain the definiton for a registered proc
 *
 * Results:
 *    method definition or NULL
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
Nsf_methodDefinition *
Nsf_CmdDefinitionGet(Tcl_ObjCmdProc *proc) {
  Tcl_HashEntry *hPtr;
  cmdPtrEntry_t cmdEntry;

  nonnull_assert(proc != NULL);

  //fprintf(stderr, "=== Lookup proc %p\n", proc);
  cmdEntry.proc = proc;

  NsfMutexLock(&cmdDefinitonMutex);
  hPtr = Tcl_FindHashEntry(cmdDefinitonHashTablePtr, (const char *)&cmdEntry);
  NsfMutexUnlock(&cmdDefinitonMutex);

  if (hPtr != NULL) {
    return Tcl_GetHashValue(hPtr);
  }

  return NULL;
}

/*
 *----------------------------------------------------------------------
 * Register --
 *
 *    Register a method Definition
 *
 * Results:
 *    Tcl result code.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
static int Register(Tcl_Interp *interp, Nsf_methodDefinition *methodDefinition) nonnull(1) nonnull(2);

static int
Register(Tcl_Interp *interp, Nsf_methodDefinition *methodDefinition) {
  Tcl_HashEntry *hPtr;
  int isNew;
  cmdPtrEntry_t cmdEntry;

  nonnull_assert(interp != NULL);
  nonnull_assert(methodDefinition != NULL);

  //fprintf(stderr, "=== Register proc %p with name %s\n", methodDefinition->proc, methodDefinition->methodName);
  cmdEntry.proc = methodDefinition->proc;
  NsfMutexLock(&cmdDefinitonMutex);
  hPtr = Tcl_CreateHashEntry(cmdDefinitonHashTablePtr, (const char *)&cmdEntry, &isNew);
  NsfMutexUnlock(&cmdDefinitonMutex);

  if (isNew != 0) {
    Tcl_SetHashValue(hPtr, methodDefinition);
    return TCL_OK;
  } else {
    return NsfPrintError(interp, "proc %s is already registered", methodDefinition->methodName);
  }
}


/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */
