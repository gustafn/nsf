/* -*- Mode: c++ -*-
 *  
 *  Extended Object Tcl (XOTcl)
 *
 *  Copyright (C) 1999-2010 Gustaf Neumann, Uwe Zdun
 *
 *
 *  nsfError.c --
 *  
 *  error return functions for XOTcl
 *  
 */

#include "nsfInt.h"


/*
 *----------------------------------------------------------------------
 *
 * NsfDStringPrintf --
 *
 *      Appends to a Tcl_DString a formatted value. This function
 *      iterates until it has sufficiently memory allocated.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
NsfDStringPrintf(Tcl_DString *dsPtr, CONST char *fmt, va_list apSrc)
{
  int      result, avail = dsPtr->spaceAvl, offset = dsPtr->length;
  va_list  ap;
  
  va_copy(ap, apSrc);
  result = vsnprintf(Tcl_DStringValue(dsPtr) + offset, avail, fmt, ap);
  va_end(ap);

  while (result >= avail) {
    
    Tcl_DStringSetLength(dsPtr, avail + 4096);
    avail = dsPtr->spaceAvl;
    /* fprintf(stderr, "NsfDStringPrintf must iterate, new avail %d\n", avail);*/

    va_copy(ap, apSrc);
    result = vsnprintf(Tcl_DStringValue(dsPtr) + offset, avail, fmt, ap);
    va_end(ap);
  }
  Tcl_DStringSetLength(dsPtr, result);
}

/*
 *----------------------------------------------------------------------
 *
 * NsfPrintError --
 *
 *      Produce a formatted error message with a printf like semantics
 *
 * Results:
 *      TCL_ERROR
 *
 * Side effects:
 *      Sets the result message.
 *
 *----------------------------------------------------------------------
 */

int
NsfPrintError(Tcl_Interp *interp, CONST char *fmt, ...) {
  va_list ap;
  Tcl_DString ds;

  Tcl_DStringInit(&ds);

  va_start(ap, fmt);
  NsfDStringPrintf(&ds, fmt, ap);
  va_end(ap);

  Tcl_SetResult(interp, Tcl_DStringValue(&ds), TCL_VOLATILE);
  Tcl_DStringFree(&ds);

  return TCL_ERROR;
}

int
NsfErrInProc(Tcl_Interp *interp, Tcl_Obj *objName,
               Tcl_Obj *clName, CONST char *procName) {
  Tcl_DString errMsg;
  char *cName, *space;
  ALLOC_DSTRING(&errMsg, "\n    ");
  if (clName) {
    cName = ObjStr(clName);
    space = " ";
  } else {
    cName = "";
    space ="";
  }
  Tcl_DStringAppend(&errMsg, ObjStr(objName),-1);
  Tcl_DStringAppend(&errMsg, space, -1);
  Tcl_DStringAppend(&errMsg, cName, -1);
  Tcl_DStringAppend(&errMsg, "->", 2);
  Tcl_DStringAppend(&errMsg, procName, -1);
  Tcl_AddErrorInfo (interp, Tcl_DStringValue(&errMsg));
  DSTRING_FREE(&errMsg);
  return TCL_ERROR;
}

int
NsfObjWrongArgs(Tcl_Interp *interp, CONST char *msg, Tcl_Obj *cmdName, 
		Tcl_Obj *methodName, char *arglist) {
  int need_space = 0;
  Tcl_ResetResult(interp);
  Tcl_AppendResult(interp, msg, " should be \"", (char *) NULL);
  if (cmdName) {
    Tcl_AppendResult(interp, ObjStr(cmdName), (char *) NULL);
    need_space = 1;
  }
  if (methodName) {
    if (need_space) Tcl_AppendResult(interp, " ", (char *) NULL);
    Tcl_AppendResult(interp, ObjStr(methodName), (char *) NULL);
    need_space = 1;
  }
  if (arglist != NULL) {
    if (need_space) Tcl_AppendResult(interp, " ", (char *) NULL);
    Tcl_AppendResult(interp, arglist, (char *) NULL);
  }
  Tcl_AppendResult(interp, "\"", (char *) NULL);
  return TCL_ERROR;
}

int
NsfErrBadVal(Tcl_Interp *interp, char *context, char *expected, CONST char *value) {
  Tcl_ResetResult(interp);
  Tcl_AppendResult(interp, context, ": expected ", expected, " but got '", 
		   value, "'", (char *) NULL);
  return TCL_ERROR;
}

extern int
NsfObjErrType(Tcl_Interp *interp, Tcl_Obj *value, CONST char *type, Nsf_Param CONST *pPtr) {
  NsfParam *paramPtr = (NsfParam *)pPtr;
  int named = (paramPtr && (paramPtr->flags & NSF_ARG_UNNAMED) == 0);
  int returnValue = !named && (paramPtr->flags & NSF_ARG_IS_RETURNVALUE);

  /*fprintf(stderr, "NsfObjErrType param %p named %d\n", paramPtr, named);*/

  Tcl_ResetResult(interp);
  Tcl_AppendResult(interp,"expected ", type, " but got \"",  ObjStr(value), "\"", (char *) NULL);
  if (named) {
    Tcl_AppendResult(interp," for parameter \"", paramPtr->name, "\"", (char *) NULL);
  } else if (returnValue) {
    Tcl_AppendResult(interp," as return value", (char *) NULL);
  }
  return TCL_ERROR;
}
