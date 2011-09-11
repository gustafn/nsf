/* -*- Mode: c++ -*-
 *  
 *  Next Scripting Framework 
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *
 *  nsfDebug.c --
 *  
 *  Debugging facilities for the Next Scripting Framework 
 *  
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
void
NsfReportVars(Tcl_Interp *interp) {
  Tcl_SetVar(interp, "::nsf::version", NSF_VERSION, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::patchLevel", NSF_PATCHLEVEL, TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::config(development)", 
#ifdef NSF_DEVELOPMENT
	     "1",
#else
	     "0",
#endif
	     TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::config(memcount)", 
#ifdef NSF_MEM_COUNT
	     "1",
#else
	     "0",
#endif
	     TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::config(memtrace)", 
#ifdef NSF_MEM_TRACE
	     "1",
#else
	     "0",
#endif
	     TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::config(profile)", 
#ifdef NSF_PROFILE
	     "1",
#else
	     "0",
#endif
	     TCL_GLOBAL_ONLY);
  Tcl_SetVar(interp, "::nsf::config(dtrace)", 
#ifdef NSF_DTRACE
	     "1",
#else
	     "0",
#endif
	     TCL_GLOBAL_ONLY);

  Tcl_SetVar(interp, "::nsf::config(cflags)", NSF_CFLAGS , TCL_GLOBAL_ONLY);
}

/*
 *----------------------------------------------------------------------
 * NsfStackDump --
 *
 *    Write the current callstack with various debugging infos to stderr. This
 *    function is primarily for debugging proposes of the C implmenentation of
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

void
NsfStackDump(Tcl_Interp *interp) {
  Interp *iPtr = (Interp *)interp;
  CallFrame *f = iPtr->framePtr, *v = iPtr->varFramePtr;
  Tcl_Obj *varCmdObj;

  NsfNewObj(varCmdObj);
  fprintf (stderr, "     TCL STACK:\n");
  if (f == 0) fprintf(stderr, "- ");
  while (f) {
    Tcl_Obj *cmdObj;
    NsfNewObj(cmdObj);
    fprintf(stderr, "\tFrame=%p ", f);
    if (f && f->isProcCallFrame && f->procPtr && f->procPtr->cmdPtr) {
      fprintf(stderr,"caller %p ", Tcl_CallFrame_callerPtr(f));
      fprintf(stderr,"callerV %p ", Tcl_CallFrame_callerVarPtr(f));
      Tcl_GetCommandFullName(interp, (Tcl_Command)f->procPtr->cmdPtr, cmdObj);
      fprintf(stderr, "%s (%p) lvl=%d\n", ObjStr(cmdObj), f->procPtr->cmdPtr, f->level);
      DECR_REF_COUNT(cmdObj);
    } else {
        if (f && f->varTablePtr) {
            fprintf(stderr, "var_table = %p ", f->varTablePtr);
        }
        fprintf(stderr, "- \n");
    }

    f = f->callerPtr;
  }

  fprintf (stderr, "     VARFRAME:\n");
  fprintf(stderr, "\tFrame=%p ", v);
  if (v) {
      fprintf(stderr, "caller %p var_table %p ", v->callerPtr, v->varTablePtr);
      /*      if (v->varTablePtr) 
              panic(0, "testing");*/
  }
  if (v && v->isProcCallFrame && v->procPtr && v->procPtr->cmdPtr) {
    Tcl_GetCommandFullName(interp, (Tcl_Command)  v->procPtr->cmdPtr, varCmdObj);
    if (varCmdObj) {
      fprintf(stderr, " %s (%d)\n", ObjStr(varCmdObj), v->level);
    }
  } else fprintf(stderr, "- \n");
  DECR_REF_COUNT(varCmdObj);
}

/*
 *----------------------------------------------------------------------
 * NsfPrintObjv --
 *
 *    Print the provided argument vector to stderr. This function is primarily
 *    for debugging proposes of the C implmenentation of nsf.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    Debugging output
 *
 *----------------------------------------------------------------------
 */

void 
NsfPrintObjv(char *string, int objc, Tcl_Obj *CONST objv[]) {
  int j; 
  fprintf(stderr, "%s", string);
  for (j = 0; j < objc; j++) {
    /*fprintf(stderr, "  objv[%d]=%s, ", j, objv[j] ? ObjStr(objv[j]) : "NULL");*/
    fprintf(stderr, "  objv[%d]=%s %p, ", j, objv[j] ? ObjStr(objv[j]) : "NULL", objv[j]);
  }
  fprintf(stderr, "\n");
}

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
static Tcl_HashTable * 
NsfMemCountGetTable(int **initialized) {
  static Tcl_ThreadDataKey memCountTableKey;
  static Tcl_ThreadDataKey memCountFlagKey;
  Tcl_HashTable *tablePtr;
  
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
void 
NsfMemCountAlloc(char *id, void *p) {
  int new, *tableInitialized;
  NsfMemCounter *entry;
  Tcl_HashTable *tablePtr = NsfMemCountGetTable(&tableInitialized);
  Tcl_HashEntry *hPtr;

  if (!*tableInitialized) {
    fprintf(stderr, "+++ alloc %s %p\n", id, p);
    return;
  }

  hPtr = Tcl_CreateHashEntry(tablePtr, id, &new);
#ifdef NSF_MEM_TRACE
  fprintf(stderr, "+++ alloc %s %p\n", id, p);
#endif
  if (new) {
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

void
NsfMemCountFree(char *id, void *p) {
  NsfMemCounter *entry;
  int *tableInitialized;
  Tcl_HashTable *tablePtr = NsfMemCountGetTable(&tableInitialized);
  Tcl_HashEntry *hPtr;

  if (!*tableInitialized) {
    fprintf(stderr, "+++ free %s %p\n", id, p);
    return;
  }
#ifdef NSF_MEM_TRACE
  fprintf(stderr, "+++ free %s %p\n", id, p);
#endif

  hPtr = Tcl_FindHashEntry(tablePtr, id);
  if (!hPtr) {
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
NsfMemCountInit() {
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
NsfMemCountRelease() {
  int *tableInitialized;
  Tcl_HashTable *tablePtr = NsfMemCountGetTable(&tableInitialized);
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;
  int count = 0;

#ifdef NSF_MEM_TRACE
  fprintf(stderr, "+++ release count %d\n", *tableInitialized);
#endif

  if (!*tableInitialized) {
    fprintf(stderr, "+++ release called on uninitialized/free hash table\n");
    return;
  } 

  if (*tableInitialized == 1) {
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
 * fill-column: 78
 * End:
 */
