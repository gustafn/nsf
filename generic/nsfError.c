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
Tcl_Obj *NsfParamDefsSyntax(INTERP_DECL Nsf_Param CONST *paramPtr);

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

extern void
NsfDStringPrintf(Tcl_DString *dsPtr, CONST char *fmt, va_list apSrc) {
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
 * NsfDStringArgv --
 *
 *      Appends argument vector to an initialized Tcl_DString.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
extern void
NsfDStringArgv(Tcl_DString *dsPtr, int objc, Tcl_Obj *CONST objv[]) {
  int i;
  for (i=0; i<objc; i++) {
    Tcl_DStringAppendElement(dsPtr, ObjStr(objv[i]));
  }
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
extern int
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

/*
 *----------------------------------------------------------------------
 *
 * NsfErrInProc --
 *
 *      Produce a general error message when an error occurs in a scripted nsf
 *      method.
 *
 * Results:
 *      TCL_ERROR
 *
 * Side effects:
 *      Sets the result message.
 *
 *----------------------------------------------------------------------
 */
extern int
NsfErrInProc(Tcl_Interp *interp, Tcl_Obj *objName,
               Tcl_Obj *clName, CONST char *procName) {
  Tcl_DString errMsg;
  char *cName, *space;
  
  Tcl_DStringInit(&errMsg);
  Tcl_DStringAppend(&errMsg, "\n    ", -1);
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
  Tcl_DStringFree(&errMsg);
  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfObjWrongArgs --
 *
 *      Produce a general error message when a nsf method is called with an
 *      invalid argument list (wrong number of arguments).
 *
 * Results:
 *      TCL_ERROR
 *
 * Side effects:
 *      Sets the result message.
 *
 *----------------------------------------------------------------------
 */
extern int
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


/*
 *----------------------------------------------------------------------
 *
 * NsfArgumentError --
 *
 *      Produce a wrong number of argument error based on a parameter definition
 *
 * Results:
 *      TCL_ERROR
 *
 * Side effects:
 *      Sets the result message.
 *
 *----------------------------------------------------------------------
 */
extern int
NsfArgumentError(Tcl_Interp *interp, CONST char *errorMsg, Nsf_Param CONST *paramPtr,
              Tcl_Obj *cmdNameObj, Tcl_Obj *methodObj) {
  Tcl_Obj *argStringObj = NsfParamDefsSyntax(INTERP paramPtr);

  NsfObjWrongArgs(interp, errorMsg, cmdNameObj, methodObj, ObjStr(argStringObj));
  DECR_REF_COUNT2("paramDefsObj", argStringObj);

  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfNoDispatchObjectError --
 *
 *      Produce a error message when method was not dispatched on an object
 *
 * Results:
 *      TCL_ERROR
 *
 * Side effects:
 *      Sets the result message.
 *
 *----------------------------------------------------------------------
 */
extern int
NsfDispatchClientDataError(Tcl_Interp *interp, ClientData clientData, 
			   CONST char *what, CONST char *methodName) {
  if (clientData) {
    return NsfPrintError(interp, "Method %s not dispatched on valid %s",
			 methodName, what);  
  } else {
    return NsfNoCurrentObjectError(interp, methodName);
  }
}

/*
 *----------------------------------------------------------------------
 *
 * NsfNoDispatchObjectError --
 *
 *      Produce a error message when method was not dispatched on an object
 *
 * Results:
 *      TCL_ERROR
 *
 * Side effects:
 *      Sets the result message.
 *
 *----------------------------------------------------------------------
 */
extern int
NsfNoCurrentObjectError(Tcl_Interp *interp, CONST char *what) {
  return NsfPrintError(interp, "No current object; %s called outside the context of a Next Scripting method",
		       what ? what : "command");
}

/*
 *----------------------------------------------------------------------
 *
 * NsfObjErrType --
 *
 *      Produce a general error message when a nsf method is called with an
 *      invalid value for some argument.
 *
 * Results:
 *      TCL_ERROR
 *
 * Side effects:
 *      Sets the result message.
 *
 *----------------------------------------------------------------------
 */
extern int
NsfObjErrType(Tcl_Interp *interp, 
	      CONST char *context, 
	      Tcl_Obj *value, 
	      CONST char *type, 
	      Nsf_Param CONST *paramPtr) 
{
  int named = (paramPtr && (paramPtr->flags & NSF_ARG_UNNAMED) == 0);
  int returnValue = !named && paramPtr && (paramPtr->flags & NSF_ARG_IS_RETURNVALUE);

  /*fprintf(stderr, "NsfObjErrType param %p named %d\n", paramPtr, named);*/

  Tcl_ResetResult(interp);
  if (context) {
    Tcl_AppendResult(interp, context, ": ",  (char *) NULL);
  }
  Tcl_AppendResult(interp,"expected ", type, " but got \"",  ObjStr(value), "\"", (char *) NULL);
  if (named) {
    Tcl_AppendResult(interp," for parameter \"", paramPtr->name, "\"", (char *) NULL);
  } else if (returnValue) {
    Tcl_AppendResult(interp," as return value", (char *) NULL);
  }
  return TCL_ERROR;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
