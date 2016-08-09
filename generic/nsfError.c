/*
 * nsfError.c --
 *
 *      Error reporting functions for the Next Scripting Framework.
 *
 * Copyright (C) 1999-2015 Gustaf Neumann (a, b)
 * Copyright (C) 1999-2007 Uwe Zdun (a, b)
 * Copyright (C) 2011-2016 Stefan Sobernig (b)
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
 *
 */

#include "nsfInt.h"

/* function prototypes */
Tcl_Obj *NsfParamDefsSyntax(Tcl_Interp *interp, Nsf_Param const *paramsPtr,
			    NsfObject *contextObject, const char *pattern)
  nonnull(1) nonnull(2) returns_nonnull;


/*
 *----------------------------------------------------------------------
 *
 * NsfDStringVPrintf --
 *
 *      Appends a formatted value to a Tcl_DString. This function
 *      continues until having allocated sufficient memory.
 *
 *      Note: The current implementation assumes C99 compliant implementations
 *      of vs*printf() for all runtimes other than MSVC. For MSVC, the pre-C99
 *      vs*printf() implementations are explicitly set by Tcl internals (see
 *      tclInt.h). For MinGW/MinGW-w64, __USE_MINGW_ANSI_STDIO must be set
 *      (see nsfInt.h).
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
NsfDStringVPrintf(Tcl_DString *dsPtr, const char *fmt, va_list vargs) {
  int      result, failure, offset, avail;
  va_list  vargsCopy;

  /* Calculate the DString's anatomy */
  offset = Tcl_DStringLength(dsPtr); 	/* current length *without* null
                                           terminating character (NTC) */

#if defined(_MSC_VER)
  avail = dsPtr->spaceAvl - offset - 1; /* Pre-C99: currently free storage, excluding NTC */
#else
  avail = dsPtr->spaceAvl - offset; /* C99: currently free storage, including NTC */
#endif
  
  /*
   * 1) Copy va_list so that the caller's copy is untouched.
   * 2) Run vsnprintf() eagerly.
   */
  va_copy(vargsCopy, vargs);
  result = vsnprintf(dsPtr->string + offset, avail, fmt, vargsCopy);
  va_end(vargsCopy);

#if defined(_MSC_VER)
  /* 
     vs*printf() in pre-C99 runtimes (MSVC up to VS13, VS15 and newer in
     backward-compat mode) return -1 upon overflowing the buffer.

     Note: Tcl via tclInt.h precludes the use of pre-C99 mode even in VS15 and
     newer (vsnprintf points to backward-compat, pre-C99 _vsnprintf).
   */
  failure = (result == -1);
#else
  /* 
     vs*printf() in C99 compliant runtimes (GCC, CLANG, MSVC in VS15 and
     newer, MinGW/MinGW-w64 with __USE_MINGW_ANSI_STDIO) returns the number of
     chars to be written if the buffer would be sufficiently large (excluding
     NTC, the terminating null character). A return value of -1 signals an
     encoding error.
  */
  assert(result > -1); /* no encoding error */
  failure = (result >= avail);
#endif

  if (likely(failure == 0)) {
    /*
     * vsnprintf() copied all content, adjust the DString length.
     */
    Tcl_DStringSetLength(dsPtr, offset + result);

  } else {
    int addedStringLength;
    /*
     * vsnprintf() could not copy all content, content was truncated.
     * Determine the required length (for MSVC), adjust the DString size, and
     * copy again.
     */

#if defined(_MSC_VER)
    va_copy(vargsCopy, vargs);
    addedStringLength = _vscprintf(fmt, vargsCopy);
    va_end(vargsCopy);
#else
    addedStringLength = result;
#endif

    Tcl_DStringSetLength(dsPtr, offset + addedStringLength);

#if defined(_MSC_VER)
    avail = dsPtr->spaceAvl - offset - 1; /* Pre-C99: currently free storage, excluding NTC */
#else
    avail = dsPtr->spaceAvl - offset; /* C99: currently free storage, including NTC */
#endif
    
    va_copy(vargsCopy, vargs);
    result = vsnprintf(dsPtr->string + offset, avail, fmt, vargsCopy);
    va_end(vargsCopy);
    
#if defined(_MSC_VER)
    failure = (result == -1);
#else
    failure = (result == -1 || result >= avail);
#endif

#if defined(NDEBUG)
    if (unlikely(failure != 0)) {
      Tcl_Panic("writing string-formatting output to a dynamic Tcl string failed");
    }
#endif
    assert(failure == 0);
  }
}

/*
 *----------------------------------------------------------------------
 * Nsf_DStringPrintf --
 *
 *      Append a sequence of values using a format string.
 *
 * Results:
 *      Pointer to the current string value.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
Nsf_DStringPrintf(Tcl_DString *dsPtr, const char *fmt, ...)
{
    va_list         ap;

    nonnull_assert(dsPtr != NULL);
    nonnull_assert(fmt != NULL);

    va_start(ap, fmt);
    NsfDStringVPrintf(dsPtr, fmt, ap);
    va_end(ap);
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
void
NsfDStringArgv(Tcl_DString *dsPtr, int objc, Tcl_Obj *CONST objv[]) {

  nonnull_assert(dsPtr != NULL);
  nonnull_assert(objv != NULL);

  if (objc > 0) {
    int i;
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
int
NsfPrintError(Tcl_Interp *interp, const char *fmt, ...) {
  va_list ap;
  Tcl_DString ds;

  Tcl_DStringInit(&ds);

  va_start(ap, fmt);
  NsfDStringVPrintf(&ds, fmt, ap);
  va_end(ap);

  Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_DStringValue(&ds), Tcl_DStringLength(&ds)));
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
int
NsfErrInProc(Tcl_Interp *interp, Tcl_Obj *objName,
               Tcl_Obj *clName, const char *procName) {
  Tcl_DString errMsg;
  char *cName, *space;

  Tcl_DStringInit(&errMsg);
  Tcl_DStringAppend(&errMsg, "\n    ", -1);
  if (clName != NULL) {
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
int
NsfObjWrongArgs(Tcl_Interp *interp, const char *msg, Tcl_Obj *cmdNameObj,
		Tcl_Obj *methodPathObj, char *arglist) {
  int need_space = 0;

  nonnull_assert(interp != NULL);
  nonnull_assert(msg != NULL);

  Tcl_ResetResult(interp);
  Tcl_AppendResult(interp, msg, " should be \"", (char *) NULL);
  if (cmdNameObj != NULL) {
    Tcl_AppendResult(interp, ObjStr(cmdNameObj), (char *) NULL);
    need_space = 1;
  }

  if (methodPathObj != NULL) {
    if (need_space != 0) {
      Tcl_AppendResult(interp, " ", (char *) NULL);
    }

    INCR_REF_COUNT(methodPathObj);
    Tcl_AppendResult(interp, ObjStr(methodPathObj), (char *) NULL);
    DECR_REF_COUNT(methodPathObj);

    need_space = 1;
  }
  if (arglist != NULL) {
    if (need_space != 0) {
      Tcl_AppendResult(interp, " ", (char *) NULL);
    }
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
int
NsfArgumentError(Tcl_Interp *interp, const char *errorMsg, Nsf_Param const *paramPtr,
              Tcl_Obj *cmdNameObj, Tcl_Obj *methodPathObj) {
  Tcl_Obj *argStringObj = NsfParamDefsSyntax(interp, paramPtr, NULL, NULL);

  nonnull_assert(interp != NULL);
  nonnull_assert(errorMsg != NULL);
  nonnull_assert(paramPtr != NULL);

  NsfObjWrongArgs(interp, errorMsg, cmdNameObj, methodPathObj, ObjStr(argStringObj));
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
int
NsfUnexpectedArgumentError(Tcl_Interp *interp, const char *argumentString,
			   Nsf_Object *object, Nsf_Param const *paramPtr,
			   Tcl_Obj *methodPathObj) {
  Tcl_DString ds, *dsPtr = &ds;

  nonnull_assert(interp != NULL);
  nonnull_assert(argumentString != NULL);
  nonnull_assert(paramPtr != NULL);
  nonnull_assert(methodPathObj != NULL);

  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, "invalid argument '", -1);
  Tcl_DStringAppend(dsPtr, argumentString, -1);
  Tcl_DStringAppend(dsPtr, "', maybe too many arguments;", -1);
  NsfArgumentError(interp, Tcl_DStringValue(dsPtr), paramPtr, (object != NULL) ? object->cmdName : NULL,
		   methodPathObj);
  DSTRING_FREE(dsPtr);
  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfUnexpectedNonposArgumentError --
 *
 *      Produce an error message about an invalid nonposistional argument.
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
NsfUnexpectedNonposArgumentError(Tcl_Interp *interp,
				 const char *argumentString,
				 Nsf_Object *object,
				 Nsf_Param const *currentParamPtr,
				 Nsf_Param const *paramPtr,
				 Tcl_Obj *methodPathObj) {
  Tcl_DString ds, *dsPtr = &ds;
  Nsf_Param const *pPtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(argumentString != NULL);
  nonnull_assert(currentParamPtr != NULL);
  nonnull_assert(paramPtr != NULL);
  nonnull_assert(methodPathObj != NULL);

  DSTRING_INIT(dsPtr);
  Tcl_DStringAppend(dsPtr, "invalid non-positional argument '", -1);
  Tcl_DStringAppend(dsPtr, argumentString, -1);
  Tcl_DStringAppend(dsPtr, "', valid are : ", -1);
  for (pPtr = currentParamPtr; (pPtr->name != NULL) && (*pPtr->name == '-'); pPtr ++) {
    if (pPtr->flags & NSF_ARG_NOCONFIG) {
      continue;
    }
    Tcl_DStringAppend(dsPtr, pPtr->name, -1);
    Tcl_DStringAppend(dsPtr, ", ", -1);
  }
  Tcl_DStringTrunc(dsPtr, Tcl_DStringLength(dsPtr) - 2);
  Tcl_DStringAppend(dsPtr, ";\n", 2);

  NsfArgumentError(interp, Tcl_DStringValue(dsPtr), paramPtr, (object != NULL) ? object->cmdName : NULL,
		   methodPathObj);
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
int
NsfDispatchClientDataError(Tcl_Interp *interp, ClientData clientData,
			   const char *what, const char *methodName) {

  nonnull_assert(interp != NULL);
  nonnull_assert(what != NULL);
  nonnull_assert(methodName != NULL);

  if (clientData != NULL) {
    return NsfPrintError(interp, "method %s not dispatched on valid %s",
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
int
NsfNoCurrentObjectError(Tcl_Interp *interp, const char *what) {

  nonnull_assert(interp != NULL);

  return NsfPrintError(interp, "no current object; %s called outside the context of a Next Scripting method",
                       (what != NULL) ? what : "command");
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
int
NsfObjErrType(Tcl_Interp *interp,
	      const char *context,
	      Tcl_Obj *value,
	      const char *type,
	      Nsf_Param const *paramPtr)
{
  int named = (paramPtr && (paramPtr->flags & NSF_ARG_UNNAMED) == 0);
  int returnValue = !named && paramPtr && (paramPtr->flags & NSF_ARG_IS_RETURNVALUE);
  char *prevErrMsg = ObjStr(Tcl_GetObjResult(interp));

  if (*prevErrMsg != '\0') {
    Tcl_AppendResult(interp, " 2nd error: ",  (char *) NULL);
  }

  /*Tcl_ResetResult(interp);*/
  if (context != NULL) {
    Tcl_AppendResult(interp, context, ": ",  (char *) NULL);
  }
  Tcl_AppendResult(interp,"expected ", type, " but got \"",  ObjStr(value), "\"", (char *) NULL);
  if (named != 0) {
    Tcl_AppendResult(interp," for parameter \"", paramPtr->name, "\"", (char *) NULL);
  } else if (returnValue != 0) {
    Tcl_AppendResult(interp," as return value", (char *) NULL);
  }
  return TCL_ERROR;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */
