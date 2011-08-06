/* -*- Mode: c++ -*-
 *  
 *  Extended Object Tcl (XOTcl)
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *
 *  nsfTrace.c --
 *  
 *  Tracing facilities for XOTcl
 *  
 */

#include "nsfInt.h"
#include "nsfAccessInt.h"

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
Tcl_HashTable nsfMemCount;


void 
NsfMemCountAlloc(Tcl_Interp *interp, char *id, void *p) {
  int new;
  NsfMemCounter *entry;
  Tcl_HashTable *tablePtr = &RUNTIME_STATE(interp)->memCountTable;
  //Tcl_HashTable *tablePtr = &nsfMemCount;
  Tcl_HashEntry *hPtr;

  assert(interp);
  hPtr = Tcl_CreateHashEntry(tablePtr, id, &new);
#ifdef NSF_MEM_TRACE
  fprintf(stderr, "+++ alloc %s %p\n", id, p);
#endif
  /*fprintf(stderr,"+++alloc '%s'\n", id);*/
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

void
NsfMemCountFree(Tcl_Interp *interp, char *id, void *p) {
  NsfMemCounter *entry;
  //Tcl_HashTable *tablePtr = &nsfMemCount;
  Tcl_HashTable *tablePtr = &RUNTIME_STATE(interp)->memCountTable;
  Tcl_HashEntry *hPtr;
#ifdef NSF_MEM_TRACE
  fprintf(stderr, "+++ free %s %p\n", id, p);
#endif

  assert(interp);
  hPtr = Tcl_FindHashEntry(tablePtr, id);
  if (!hPtr) {
    fprintf(stderr, "******** MEM COUNT ALERT: Trying to free %p <%s>, "
	    "but was not allocated\n", p, id);
    return;
  }
  entry = (NsfMemCounter *)Tcl_GetHashValue(hPtr);
  entry->count--;
}

void
NsfMemCountInit(Tcl_Interp *interp) {
  Tcl_HashTable *tablePtr = &RUNTIME_STATE(interp)->memCountTable;

  Tcl_InitHashTable(tablePtr, TCL_STRING_KEYS);
#if 0
  extern Tcl_HashTable nsfMemCount;
  Tcl_InitHashTable(&nsfMemCount, TCL_STRING_KEYS);
#endif
}

void
NsfMemCountRelease(Tcl_Interp *interp) {
  Tcl_HashTable *tablePtr = &RUNTIME_STATE(interp)->memCountTable;
  Tcl_HashSearch search;
  Tcl_HashEntry *hPtr;
  int count = 0;

#if 0
  nsfMemCountInterpCounter--;
  if (nsfMemCountInterpCounter != 0) {
    return;
  }
#endif

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

#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
