/*  
 *  nsfError.c --
 *  
 *      Error reporting functions for the Next Scripting Framework.
 *  
 *  Copyright (C) 1999-2011 Gustaf Neumann
 *  Copyright (C) 1999-2007 Uwe Zdun
 *  Copyright (C) 2011 Stefan Sobernig
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

/* function prototypes */
Tcl_Obj *NsfParamDefsSyntax(Nsf_Param CONST *paramPtr);

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
  if (objc > 0) {
    Tcl_DStringAppendElement(dsPtr, NsfMethodName(objv[0]));
    for (i = 1; i < objc; i++) {
      Tcl_DStringAppendElement(dsPtr, ObjStr(objv[i]));
    }
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

    Tcl_Obj *resultObj = NsfMethodNamePath(interp, methodName);
    INCR_REF_COUNT(resultObj);
    Tcl_AppendResult(interp, ObjStr(resultObj), (char *) NULL);
    DECR_REF_COUNT(resultObj);

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
  Tcl_Obj *argStringObj = NsfParamDefsSyntax(paramPtr);

  NsfObjWrongArgs(interp, errorMsg, cmdNameObj, methodObj, ObjStr(argStringObj));
  DECR_REF_COUNT2("paramDefsObj", argStringObj);

  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfUnexpectedArgumentError --
 *
 *      Produce an error message about an unexpected argument (most likely,
 *      too many arguments)
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
NsfUnexpectedArgumentError(Tcl_Interp *interp, CONST char *argumentString, 
			   Nsf_Object *object, Nsf_Param CONST *paramPtr, Tcl_Obj *procNameObj) {
  Tcl_DString ds, *dsPtr = &ds;
  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, "Invalid argument '", -1);
  Tcl_DStringAppend(dsPtr, argumentString, -1);
  Tcl_DStringAppend(dsPtr, "', maybe too many arguments;", -1);
  NsfArgumentError(interp, Tcl_DStringValue(dsPtr), paramPtr,
		   object ? object->cmdName : NULL,
		   procNameObj);
  DSTRING_FREE(dsPtr);
  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfUnexpectedNonposArgumentError --
 *
 *      Produce a unexpecte argument number (most likely, too many arguments)
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
NsfUnexpectedNonposArgumentError(Tcl_Interp *interp, 
				 CONST char *argumentString, 
				 Nsf_Object *object, 
				 Nsf_Param CONST *currentParamPtr,
				 Nsf_Param CONST *paramPtr,
				 Tcl_Obj *procNameObj) {
  Tcl_DString ds, *dsPtr = &ds;
  Nsf_Param CONST *pPtr;

  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, "Invalid non-positional argument '", -1);
  Tcl_DStringAppend(dsPtr, argumentString, -1);
  Tcl_DStringAppend(dsPtr, "', valid are : ", -1);
  for (pPtr = currentParamPtr; pPtr->name && *pPtr->name == '-'; pPtr ++) {
    if (pPtr->flags & NSF_ARG_NOCONFIG) {
      continue;
    }
    Tcl_DStringAppend(dsPtr, pPtr->name, -1);
    Tcl_DStringAppend(dsPtr, ", ", -1);
  }
  Tcl_DStringTrunc(dsPtr, Tcl_DStringLength(dsPtr) - 2);
  Tcl_DStringAppend(dsPtr, ";\n", 2);
  
  NsfArgumentError(interp, Tcl_DStringValue(dsPtr), paramPtr,
		   object ? object->cmdName : NULL,
		   procNameObj);
  DSTRING_FREE(dsPtr);
  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfDispatchClientDataError --
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
 * NsfNoCurrentObjectError --
 *
 *      Produce a error message when method was called outside the context of
 *      a method
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
