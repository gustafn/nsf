/*
 *  nsfDebug.c --
 *
 *      Debugging facilities for the Next Scripting Framework.
 *
 *  Copyright (C) 1999-2014 Gustaf Neumann (a, b)
 *  Copyright (C) 1999-2007 Uwe Zdun (a, b)
 *
 * (a) University of Essen
 *     Specification of Software Systems
 *     Altendorferstrasse 97-101
 *     D-45143 Essen, Germany
 *
 * (b) Vienna University of Economics and Business
 *     Institute of Information Systems and New Media
 *     A-1020, Welthandelsplatz 1
 *     Vienna, Austria
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
#include "nsfAccessInt.h"


/*
 *----------------------------------------------------------------------
 * NsfReportVars --
 *
 *    Report version numbers and configure options as tcl variables.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Setting Tcl variables
 *
 *----------------------------------------------------------------------
 */

#define NsfConfigStr(x) (NsfConfigEnabled(x) ? "1" : "0")

void
NsfReportVars(Tcl_Interp *interp) {

  nonnull_assert(interp != NULL);

  Tcl_SetVar(interp, "::nsf::version", NSF_VERSION, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::patchLevel", NSF_PATCHLEVEL, TCL_GLOBAL_ONLY);

  Tcl_SetVar(interp, "::nsf::config(development)",
             NsfConfigStr(DEVELOPMENT),
             TCL_GLOBAL_ONLY);


  Tcl_SetVar(interp, "::nsf::config(memcount)",
             NsfConfigStr(MEM_COUNT),
             TCL_GLOBAL_ONLY);

  Tcl_SetVar(interp, "::nsf::config(memtrace)",
             NsfConfigStr(MEM_TRACE),
             TCL_GLOBAL_ONLY);

  Tcl_SetVar(interp, "::nsf::config(profile)",
             NsfConfigStr(PROFILE),
             TCL_GLOBAL_ONLY);

  Tcl_SetVar(interp, "::nsf::config(dtrace)",
             NsfConfigStr(DTRACE),
             TCL_GLOBAL_ONLY);

  Tcl_SetVar(interp, "::nsf::config(assertions)",
             NsfConfigStr(WITH_ASSERTIONS),
             TCL_GLOBAL_ONLY);
}

/*
 *----------------------------------------------------------------------
 * NsfStackDump --
 *
 *    Write the current callstack with various debugging infos to stderr. This
 *    function is primarily for debugging proposes of the C implementation of
 *    nsf.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Debugging output
 *
 *----------------------------------------------------------------------
 */

void NsfStackDump(Tcl_Interp *interp) nonnull(1);

void
NsfStackDump(Tcl_Interp *interp) {
  Interp     *iPtr;
  CallFrame  *f, *v;
  Tcl_Obj    *varCmdObj;

  nonnull_assert(interp != NULL);

  iPtr = (Interp *)interp;
  f = iPtr->framePtr;
  v = iPtr->varFramePtr;
  varCmdObj = Tcl_NewObj();
  fprintf (stderr, "     TCL STACK:\n");
  if (f == NULL) {
    fprintf(stderr, "- ");
  }
  while (f != NULL) {
    Tcl_Obj *cmdObj = Tcl_NewObj();

    fprintf(stderr, "\tFrame=%p ", (void *)f);
    if ((f != NULL)
        && (f->isProcCallFrame != 0)
        && (f->procPtr != NULL)
        && (f->procPtr->cmdPtr != NULL)
        ) {
      fprintf(stderr,"caller %p ", (void *)Tcl_CallFrame_callerPtr(f));
      fprintf(stderr,"callerV %p ", (void *)Tcl_CallFrame_callerVarPtr(f));
      Tcl_GetCommandFullName(interp, (Tcl_Command)f->procPtr->cmdPtr, cmdObj);
      fprintf(stderr, "%s (%p) lvl=%d\n", ObjStr(cmdObj), (void *)f->procPtr->cmdPtr, f->level);
    } else {
        if (f != NULL && f->varTablePtr != NULL) {
            fprintf(stderr, "var_table = %p ", (void *)f->varTablePtr);
        }
        fprintf(stderr, "- \n");
    }
    DECR_REF_COUNT(cmdObj);
    f = f->callerPtr;
  }

  fprintf (stderr, "     VARFRAME:\n");
  fprintf(stderr, "\tFrame=%p ", (void *)v);
  if (v != NULL) {
      fprintf(stderr, "caller %p var_table %p ", (void *)v->callerPtr, (void *)v->varTablePtr);
      /*      if (v->varTablePtr != NULL)
              panic(0, "testing");*/
  }
  if (v != NULL && v->isProcCallFrame && v->procPtr && v->procPtr->cmdPtr) {
    Tcl_GetCommandFullName(interp, (Tcl_Command)  v->procPtr->cmdPtr, varCmdObj);
    fprintf(stderr, " %s (%d)\n", ObjStr(varCmdObj), v->level);
  } else {
    fprintf(stderr, "- \n");
  }
  DECR_REF_COUNT(varCmdObj);
}

#ifdef NSF_PRINT_OBJV
/*
 *----------------------------------------------------------------------
 * NsfPrintObjv --
 *
 *    Print the provided argument vector to stderr. This function is primarily
 *    for debugging proposes of the C implementation of nsf.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Debugging output
 *
 *----------------------------------------------------------------------
 */

void NsfPrintObjv(char *string, int objc, Tcl_Obj *CONST objv[]) nonnull(1) nonnull(3);

void
NsfPrintObjv(char *string, int objc, Tcl_Obj *CONST objv[]) {
  int j;

  nonnull_assert(string != NULL);
  nonnull_assert(objv != NULL);

  fprintf(stderr, "%s", string);
  for (j = 0; j < objc; j++) {
    /*fprintf(stderr, "  objv[%d]=%s, ", j, objv[j] ? ObjStr(objv[j]) : "NULL");*/
    fprintf(stderr, "  objv[%d]=%s %p, ", j, objv[j] ? ObjStr(objv[j]) : "NULL", (void *)objv[j]);
  }
  fprintf(stderr, "\n");
}
#endif

#ifdef NSF_MEM_COUNT
/*
 *----------------------------------------------------------------------
 * NsfMemCountGetTable --
 *
 *    Obtain the hash table structure
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Updateing Hash table
 *
 *----------------------------------------------------------------------
 */
static Tcl_HashTable *  NsfMemCountGetTable(int **initialized) nonnull(1);

static Tcl_HashTable *
NsfMemCountGetTable(int **initialized) {
  static Tcl_ThreadDataKey memCountTableKey;
  static Tcl_ThreadDataKey memCountFlagKey;
  Tcl_HashTable *tablePtr;

  nonnull_assert(initialized != NULL);

  tablePtr = (Tcl_HashTable *)Tcl_GetThreadData(&memCountTableKey, sizeof(Tcl_HashTable));
  *initialized = (int *)Tcl_GetThreadData(&memCountFlagKey, sizeof(int));

  return tablePtr;
}

/*
 *----------------------------------------------------------------------
 * NsfMemCountAlloc --
 *
 *    Bookkeeping function for memory und refcount debugging. This function
 *    records the allocation of memory resources. The accompanying function is
 *    NsfMemCountFree().
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Updateing Hash table
 *
 *----------------------------------------------------------------------
 */

void  NsfMemCountAlloc(char *id, void *p) nonnull(1);

void
NsfMemCountAlloc(char *id, void *p) {
  int new, *tableInitialized;
  NsfMemCounter *entry;
  Tcl_HashTable *tablePtr = NsfMemCountGetTable(&tableInitialized);
  Tcl_HashEntry *hPtr;

  nonnull_assert(id != NULL);

  if (!*tableInitialized) {
    fprintf(stderr, "+++ alloc %s %p\n", id, p);
    return;
  }

  hPtr = Tcl_CreateHashEntry(tablePtr, id, &new);
#ifdef NSF_MEM_TRACE
  fprintf(stderr, "+++ alloc %s %p\n", id, p);
#endif
  if (new != 0) {
    entry = (NsfMemCounter*)ckalloc(sizeof(NsfMemCounter));
    entry->count = 1;
    entry->peak = 1;
    Tcl_SetHashValue(hPtr, entry);
  } else {
    entry = (NsfMemCounter*) Tcl_GetHashValue(hPtr);
    entry->count++;
    if (entry->count > entry->peak) {
      entry->peak = entry->count;
    }
  }
}

/*
 *----------------------------------------------------------------------
 * NsfMemCountFree --
 *
 *    Bookkeeping function for memory und refcount debugging. This function
 *    records the deallocation of memory resources. The accompanying function
 *    is NsfMemCountAlloc().
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Updateing Hash table
 *
 *----------------------------------------------------------------------
 */

void NsfMemCountFree(char *id, void *p) nonnull(1);

void
NsfMemCountFree(char *id, void *p) {
  NsfMemCounter *entry;
  int *tableInitialized;
  Tcl_HashTable *tablePtr = NsfMemCountGetTable(&tableInitialized);
  Tcl_HashEntry *hPtr;

  nonnull_assert(id != NULL);

  if (!*tableInitialized) {
    fprintf(stderr, "+++ free %s %p !tableInitialized !\n", id, p);
    return;
  }
#ifdef NSF_MEM_TRACE
  fprintf(stderr, "+++ free %s %p\n", id, p);
#endif

  hPtr = Tcl_FindHashEntry(tablePtr, id);
  if (hPtr == NULL) {
    fprintf(stderr, "******** MEM COUNT ALERT: Trying to free %p <%s>, "
            "but was not allocated\n", p, id);
    return;
  }
  entry = (NsfMemCounter *)Tcl_GetHashValue(hPtr);
  entry->count--;
}

/*
 *----------------------------------------------------------------------
 * NsfMemCountInit --
 *
 *    Initialize book-keeping for memory und refcount debugging. The
 *    bookkeeping is realized via a per-interp hash table.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Initializes a hash table
 *
 *----------------------------------------------------------------------
 */
void
NsfMemCountInit(void) {
  int *tableInitialized;
  Tcl_HashTable *tablePtr = NsfMemCountGetTable(&tableInitialized);

  if (!*tableInitialized) {
    Tcl_InitHashTable(tablePtr, TCL_STRING_KEYS);
  }
  (*tableInitialized) ++;
}

/*
 *----------------------------------------------------------------------
 * NsfMemCountRelease --
 *
 *    Terminate book-keeping for memory und refcount debugging. This function
 *    prints the resulting book-information to stderr, in case of paired
 *    allocs/frees and incr-ref-counts and dec-ref-counts, the Overall count
 *    should be 0.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Deletes the book-keeping hash table, outputs to stderr
 *
 *----------------------------------------------------------------------
 */
void
NsfMemCountRelease(void) {
  int *tableInitialized;
  Tcl_HashTable *tablePtr = NsfMemCountGetTable(&tableInitialized);

#ifdef NSF_MEM_TRACE
  fprintf(stderr, "+++ release count %d\n", *tableInitialized);
#endif

  if (!*tableInitialized) {
    fprintf(stderr, "+++ release called on uninitialized/free hash table\n");
    return;
  }

  if (*tableInitialized == 1) {
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    int count = 0;

    fprintf(stderr, "******** NSF MEM Count *********\n*  count peak\n");

    for (hPtr = Tcl_FirstHashEntry(tablePtr, &search);  hPtr != NULL;
         hPtr = Tcl_NextHashEntry(&search)) {
      char *id = Tcl_GetHashKey(tablePtr, hPtr);
      NsfMemCounter *entry = (NsfMemCounter*)  Tcl_GetHashValue(hPtr);
      count += entry->count;
      fprintf(stderr, "* %4d %6d %s\n", entry->count, entry->peak, id);
      ckfree ((char*) entry);
    }

    Tcl_DeleteHashTable(tablePtr);

    fprintf(stderr, "******** Count Overall = %d\n", count);
  }

  (*tableInitialized) --;
}

#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * fill-column: 78
 * End:
 */
