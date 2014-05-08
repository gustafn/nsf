/*
 *  nsfCmdDefinitions.c --
 *
 *      Provide API for registering method definitions
 *      and obtaining this data for introspection
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

static Tcl_HashTable cmdDefinitonHashTable, *cmdDefinitonHashTablePtr = &cmdDefinitonHashTable;
static int cmdDefinitonRefCount = 0;
static NsfMutex cmdDefinitonMutex = 0;

static int Register(Tcl_Interp *interp, Nsf_methodDefinition *methodDefinition);

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

  assert(interp);

  NsfMutexLock(&cmdDefinitonMutex);
  
  if (cmdDefinitonRefCount == 0) {
    Tcl_InitHashTable(cmdDefinitonHashTablePtr, TCL_ONE_WORD_KEYS);
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

  assert(interp);
  assert(definitionRecords);

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

  assert(proc);

  NsfMutexLock(&cmdDefinitonMutex);
  hPtr = Tcl_FindHashEntry(cmdDefinitonHashTablePtr, (char *)proc); 
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

  assert(interp);
  assert(methodDefinition);

  NsfMutexLock(&cmdDefinitonMutex);
  hPtr = Tcl_CreateHashEntry(cmdDefinitonHashTablePtr, (char *)methodDefinition->proc, &isNew);
  NsfMutexUnlock(&cmdDefinitonMutex);

  if (isNew) {
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
 * fill-column: 72
 * End:
 */
