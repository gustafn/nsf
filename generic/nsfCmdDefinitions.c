/*
 * nsfCmdDefinitions.c --
 *
 *      Provide an API for registering method definitions
 *      and obtaining these meta-data for introspection.
 *
 * Copyright (C) 2014-2016 Gustaf Neumann
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

static Tcl_HashTable cmdDefinitionHashTable, *cmdDefinitionHashTablePtr = &cmdDefinitionHashTable;
static int cmdDefinitionRefCount = 0;
static NsfMutex cmdDefinitionMutex = 0;

static int Register(Tcl_Interp *interp, Nsf_methodDefinition *methodDefinition);

/*
 *----------------------------------------------------------------------
 * Nsf_CmdDefinitionInit --
 *
 *    Initialize the hash-table structure for storing the method definitions.
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

  NsfMutexLock(&cmdDefinitionMutex);

  if (cmdDefinitionRefCount == 0) {
    Nsf_InitFunPtrHashTable(cmdDefinitionHashTablePtr);
  }
  cmdDefinitionRefCount++;

  NsfMutexUnlock(&cmdDefinitionMutex);
}

/*----------------------------------------------------------------------
* Nsf_EnumerationTypeRelease --
*
*    Release and, eventually, delete the hash table for method definitions.
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
Nsf_CmdDefinitionRelease(Tcl_Interp *interp) {
  
  nonnull_assert(interp != NULL);
  
  NsfMutexLock(&cmdDefinitionMutex);
  
  if (--cmdDefinitionRefCount < 1) {
    Tcl_DeleteHashTable(cmdDefinitionHashTablePtr);
  }
  
  NsfMutexUnlock(&cmdDefinitionMutex);
}



/*
 *----------------------------------------------------------------------
 * Nsf_CmdDefinitionRegister --
 *
 *    Register an array of cmd definitions
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
 *    Obtain the definition for a previously registered proc.
 *
 * Results:
 *    A pointer to a Method definition or NULL.
 *
 * Side effects:
 *    None.
 *
 *----------------------------------------------------------------------
 */
Nsf_methodDefinition *
Nsf_CmdDefinitionGet(Tcl_ObjCmdProc *proc) {
  Tcl_HashEntry *hPtr;

  nonnull_assert(proc != NULL);

  NsfMutexLock(&cmdDefinitionMutex);
  hPtr = Nsf_FindFunPtrHashEntry(cmdDefinitionHashTablePtr, (Nsf_AnyFun *)proc);
  NsfMutexUnlock(&cmdDefinitionMutex);

  if (hPtr != NULL) {
    return Tcl_GetHashValue(hPtr);
  }

  return NULL;
}

/*
 *----------------------------------------------------------------------
 * Register --
 *
 *    Register a method definition.
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

  nonnull_assert(interp != NULL);
  nonnull_assert(methodDefinition != NULL);

  NsfMutexLock(&cmdDefinitionMutex);
  hPtr = Nsf_CreateFunPtrHashEntry(cmdDefinitionHashTablePtr, (Nsf_AnyFun *)methodDefinition->proc, &isNew); 
  NsfMutexUnlock(&cmdDefinitionMutex);
  
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
