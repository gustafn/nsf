/*
 * nsfError.c --
 *
 *      Error reporting functions for the Next Scripting Framework.
 *
 * Copyright (C) 1999-2018 Gustaf Neumann (a, b)
 * Copyright (C) 1999-2007 Uwe Zdun (a, b)
 * Copyright (C) 2011-2016 Stefan Sobernig (b)
 *
 * (a) University of Essen
 *      Specification of Software Systems
 *      Altendorferstrasse 97-101
 *      D-45143 Essen, Germany
 *
 * (b) Vienna University of Economics and Business
 *      Institute of Information Systems and New Media
 *      A-1020, Welthandelsplatz 1
 *      Vienna, Austria
 *
 * This work is licensed under the MIT License https://www.opensource.org/licenses/MIT
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
Tcl_Obj *NsfParamDefsSyntax(
    Tcl_Interp *interp, Nsf_Param const *paramsPtr,
    NsfObject *contextObject, const char *pattern
) nonnull(1) nonnull(2) returns_nonnull;


/*
 *----------------------------------------------------------------------
 *
 * NsfDStringVPrintf --
 *
 *      Appends a formatted value to a Tcl_DString. This function
 *      continues until having allocated sufficient memory.
 *
 *      Note: The current implementation assumes C99 compliant
 *      implementations of vs*printf() for all runtimes other than
 *      MSVC. For MSVC, the pre-C99 vs*printf() implementations are
 *      explicitly set by Tcl internals (see tclInt.h). For
 *      MinGW/MinGW-w64, __USE_MINGW_ANSI_STDIO must be set (see
 *      nsfInt.h).
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
NsfDStringVPrintf(Tcl_DString *dsPtr, const char *fmt, va_list argPtr) {
  int        result;
  TCL_SIZE_T avail, offset;
  va_list    argPtrCopy;
  bool       failure;

  /*
   * Tcl_DStringLength returns the current length *without* the
   * terminating character (NTC).
   */
  offset = Tcl_DStringLength(dsPtr);

#if defined(_MSC_VER)
  /*
   * Pre-C99: currently free storage, excluding NTC
   */
  avail = dsPtr->spaceAvl - offset - 1;
#else
  /*
   * C99: currently free storage, including NTC
   */
  avail = dsPtr->spaceAvl - offset;
#endif

  /*
   * 1) Copy va_list so that the caller's copy is untouched.
   * 2) Run vsnprintf() eagerly.
   */
  va_copy(argPtrCopy, argPtr);
  result = vsnprintf(dsPtr->string + offset, (size_t)avail, fmt, argPtrCopy);
  va_end(argPtrCopy);

#if defined(_MSC_VER)
  /*
   *  vs*printf() in pre-C99 runtimes (MSVC up to VS13, VS15 and newer
   *  in backward-compat mode) return -1 upon overflowing the buffer.
   *
   *  Note: Tcl via tclInt.h precludes the use of pre-C99 mode even in
   *  VS15 and newer (vsnprintf points to backwards compatible, pre-C99
   *  _vsnprintf).
   */
  failure = (result == -1);
#else
  /*
   *  vs*printf() in C99 compliant runtimes (GCC, CLANG, MSVC in VS15
   *  and newer, MinGW/MinGW-w64 with __USE_MINGW_ANSI_STDIO) returns
   *  the number of chars to be written if the buffer would be
   *  sufficiently large (excluding NTC, the terminating null
   *  character). A return value of -1 signals an encoding error.
   */
  assert(result > -1); /* no encoding error */
  failure = (result >= (int)avail);
#endif

  if (likely(! failure)) {
    /*
     * vsnprintf() copied all content, adjust the Tcl_DString length.
     */
    Tcl_DStringSetLength(dsPtr, offset + (TCL_SIZE_T)result);

  } else {
    TCL_SIZE_T addedStringLength;
    /*
     * vsnprintf() could not copy all content, content was truncated.
     * Determine the required length (for MSVC), adjust the Tcl_DString
     * size, and copy again.
     */

#if defined(_MSC_VER)
    va_copy(argPtrCopy, argPtr);
    addedStringLength = _vscprintf(fmt, argPtrCopy);
    va_end(argPtrCopy);
#else
    addedStringLength = (TCL_SIZE_T)result;
#endif

    Tcl_DStringSetLength(dsPtr, offset + addedStringLength);

#if defined(_MSC_VER)
    /*
     * Pre-C99: currently free storage, excluding NTC
     */
    avail = dsPtr->spaceAvl - offset - 1;
#else
    /*
     * C99: currently free storage, including NTC
     */
    avail = dsPtr->spaceAvl - offset;
#endif

    va_copy(argPtrCopy, argPtr);
    result = vsnprintf(dsPtr->string + offset, (size_t)avail, fmt, argPtrCopy);
    va_end(argPtrCopy);

#if defined(_MSC_VER)
    failure = (result == -1);
#else
    failure = (result == -1 || (TCL_SIZE_T)result >= avail);
#endif

#if defined(NDEBUG)
    if (unlikely(failure)) {
      Tcl_Panic("writing string-formatting output to a dynamic Tcl string failed");
    }
#endif
    assert(! failure);
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
    va_list ap;

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
NsfDStringArgv(
    Tcl_DString *dsPtr,
    TCL_OBJC_T objc, Tcl_Obj *const objv[]
) {
  nonnull_assert(dsPtr != NULL);
  nonnull_assert(objv != NULL);

  if (objc > 0) {
    TCL_OBJC_T i;

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
 *      Produce a formatted error message with a printf-like semantics.
 *
 * Results:
 *      Returns always TCL_ERROR.
 *
 * Side effects:
 *      Leaves an error message in the interpreter's result object.
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
 *      Produce a general error message when an error occurs in a
 *      scripted NSF method.
 *
 * Results:
 *      Returns always TCL_ERROR.
 *
 * Side effects:
 *      Leaves an error message in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */
int
NsfErrInProc(
    Tcl_Interp *interp,
    Tcl_Obj *objName,
    Tcl_Obj *clName,
    const char *procName
) {
  Tcl_DString errMsg;
  const char *cName, *space;

  Tcl_DStringInit(&errMsg);
  Tcl_DStringAppend(&errMsg, "\n    ", TCL_INDEX_NONE);
  if (clName != NULL) {
    cName = ObjStr(clName);
    space = " ";
  } else {
    cName = "";
    space = "";
  }
  Tcl_DStringAppend(&errMsg, ObjStr(objName), TCL_INDEX_NONE);
  Tcl_DStringAppend(&errMsg, space, TCL_INDEX_NONE);
  Tcl_DStringAppend(&errMsg, cName, TCL_INDEX_NONE);
  Tcl_DStringAppend(&errMsg, "->", 2);
  Tcl_DStringAppend(&errMsg, procName, TCL_INDEX_NONE);
  Tcl_AddErrorInfo (interp, Tcl_DStringValue(&errMsg));
  Tcl_DStringFree(&errMsg);
  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * NsfObjWrongArgs --
 *
 *      Produce a general error message when an NSF method is called with
 *      an invalid argument list (wrong number of arguments).
 *
 * Results:
 *      Returns always TCL_ERROR.
 *
 * Side effects:
 *      Leaves an error message in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */
int
NsfObjWrongArgs(
    Tcl_Interp *interp,
    const char *msg, Tcl_Obj *cmdNameObj,
    Tcl_Obj *methodPathObj, const char *arglist
) {
  bool        need_space = NSF_FALSE;
  Tcl_DString ds;

  nonnull_assert(interp != NULL);
  nonnull_assert(msg != NULL);

  Tcl_DStringInit(&ds);

  Nsf_DStringPrintf(&ds, "%s should be \"", msg);
  if (cmdNameObj != NULL) {
    Tcl_DStringAppend(&ds, ObjStr(cmdNameObj), TCL_INDEX_NONE);
    need_space = NSF_TRUE;
  }

  if (methodPathObj != NULL) {
    if (need_space) {
      Tcl_DStringAppend(&ds, " ", 1);
    }

    INCR_REF_COUNT(methodPathObj);
    Tcl_DStringAppend(&ds, ObjStr(methodPathObj), TCL_INDEX_NONE);
    DECR_REF_COUNT(methodPathObj);

    need_space = NSF_TRUE;
  }
  if (arglist != NULL) {
    if (need_space) {
      Tcl_DStringAppend(&ds, " ", 1);
    }
    Tcl_DStringAppend(&ds, arglist, TCL_INDEX_NONE);
  }
  Tcl_DStringAppend(&ds, "\"", 1);

  Tcl_SetObjResult(interp, Tcl_NewStringObj(ds.string, ds.length));
  Tcl_DStringFree(&ds);

  return TCL_ERROR;
}


/*
 *----------------------------------------------------------------------
 *
 * NsfArgumentError --
 *
 *      Produce a wrong-number-of-arguments error based on a parameter
 *      definition.
 *
 * Results:
 *      Returns always TCL_ERROR.
 *
 * Side effects:
 *      Leaves an error message in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */
int
NsfArgumentError(
    Tcl_Interp *interp,
    const char *errorMsg, Nsf_Param const *paramPtr,
    Tcl_Obj *cmdNameObj, Tcl_Obj *methodPathObj
) {
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
 *      Produce an error message on an unexpected argument (most likely,
 *      too many arguments)
 *
 * Results:
 *      Returns always TCL_ERROR.
 *
 * Side effects:
 *      Leaves an error message in the interpreter's result object.
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
  Nsf_DStringPrintf(dsPtr, "invalid argument '%s', maybe too many arguments;", argumentString);
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
 *      Produce an error message on an invalid non-positional argument.
 *
 * Results:
 *      Returns always TCL_ERROR.
 *
 * Side effects:
 *      Leaves an error message in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */
int
NsfUnexpectedNonposArgumentError(
    Tcl_Interp *interp,
    const char *argumentString,
    Nsf_Object *object,
    Nsf_Param const *currentParamPtr,
    Nsf_Param const *paramPtr,
    Tcl_Obj *methodPathObj
) {
  Tcl_DString ds, *dsPtr = &ds;
  const Nsf_Param *pPtr;

  nonnull_assert(interp != NULL);
  nonnull_assert(argumentString != NULL);
  nonnull_assert(currentParamPtr != NULL);
  nonnull_assert(paramPtr != NULL);
  nonnull_assert(methodPathObj != NULL);

  DSTRING_INIT(dsPtr);
  Nsf_DStringPrintf(dsPtr, "invalid non-positional argument '%s', valid are: ", argumentString);
  for (pPtr = currentParamPtr; (pPtr->name != NULL) && (*pPtr->name == '-'); pPtr ++) {
    if (pPtr->flags & NSF_ARG_NOCONFIG) {
      continue;
    }
    Tcl_DStringAppend(dsPtr, pPtr->name, TCL_INDEX_NONE);
    Tcl_DStringAppend(dsPtr, ", ", TCL_INDEX_NONE);
  }
  Tcl_DStringSetLength(dsPtr, Tcl_DStringLength(dsPtr) - 2);
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
 *      Produce an error message when a method was not dispatched on an
 *      object.
 *
 * Results:
 *      Returns always TCL_ERROR.
 *
 * Side effects:
 *      Leaves an error message in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */
int
NsfDispatchClientDataError(
    Tcl_Interp *interp, ClientData clientData,
    const char *what, const char *methodName
) {

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
 *      Produce an error message when a method/command was called
 *      outside the context of an object or a method. The passed in
 *      methodName is NULL when e.g. "self" is called outside of an NSF
 *      context.
 *
 * Results:
 *      Returns always TCL_ERROR.
 *
 * Side effects:
 *      Leaves an error message in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */
int
NsfNoCurrentObjectError(Tcl_Interp *interp, const char *methodName) {

  nonnull_assert(interp != NULL);

  return NsfPrintError(interp, "no current object; %s called outside the context of a Next Scripting method",
                       (methodName != NULL) ? methodName : "command");
}

/*
 *----------------------------------------------------------------------
 *
 * NsfObjErrType --
 *
 *      Produce a general error message when an NSF method is called with
 *      an invalid value for some argument.
 *
 * Results:
 *      Returns always TCL_ERROR.
 *
 * Side effects:
 *      Leaves an error message in the interpreter's result object.
 *
 *----------------------------------------------------------------------
 */
int
NsfObjErrType(
    Tcl_Interp *interp,
    const char *context,
    Tcl_Obj *value,
    const char *type,
    Nsf_Param const *NsfObjErrType
) {
  bool        isNamed     = (NsfObjErrType && (NsfObjErrType->flags & NSF_ARG_UNNAMED) == 0);
  int         returnValue = !isNamed && NsfObjErrType && (NsfObjErrType->flags & NSF_ARG_IS_RETURNVALUE);
  TCL_SIZE_T  errMsgLen;
  const char *prevErrMsg  = Tcl_GetStringFromObj(Tcl_GetObjResult(interp), &errMsgLen);
  Tcl_DString ds;

  Tcl_DStringInit(&ds);
  if (errMsgLen > 0) {
    Tcl_DStringAppend(&ds, prevErrMsg, errMsgLen);
    Tcl_DStringAppend(&ds, " 2nd error: ", TCL_INDEX_NONE);
  }

  if (context != NULL) {
    Tcl_DStringAppend(&ds, context, TCL_INDEX_NONE);
    Tcl_DStringAppend(&ds, ": ", 2);
  }

  Nsf_DStringPrintf(&ds, "expected %s but got \"%s\"", type, ObjStr(value));
  if (isNamed) {
    Nsf_DStringPrintf(&ds, " for parameter \"%s\"", NsfObjErrType->name);
  } else if (returnValue != 0) {
    Tcl_DStringAppend(&ds, " as return value", TCL_INDEX_NONE);
  }

  Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_DStringValue(&ds), Tcl_DStringLength(&ds)));
  Tcl_DStringFree(&ds);

  return TCL_ERROR;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 72
 * indent-tabs-mode: nil
 * eval: (c-guess)
 * End:
 */
