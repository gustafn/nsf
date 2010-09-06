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

int
NsfErrMsg(Tcl_Interp *interp, char *msg, Tcl_FreeProc* type) {
    Tcl_SetResult(interp, msg, type);
    return TCL_ERROR;
}

int
NsfVarErrMsg TCL_VARARGS_DEF (Tcl_Interp *, arg1) {
    va_list argList;
    char *string;
    Tcl_Interp *interp;

    interp = TCL_VARARGS_START(Tcl_Interp *, arg1, argList);
    Tcl_ResetResult(interp);
    while (1) {
      string = va_arg(argList, char *);
      if (string == NULL) {
        break;
      }
      Tcl_AppendResult(interp, string, (char *) NULL);
    }
    va_end(argList);
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
NsfObjErrArgCnt(Tcl_Interp *interp, Tcl_Obj *cmdName,  Tcl_Obj *methodName, char *arglist) {
  return NsfObjWrongArgs(interp, "wrong # args:", cmdName, methodName, arglist);
}

int
NsfErrBadVal(Tcl_Interp *interp, char *context, char *expected, CONST char *value) {
  Tcl_ResetResult(interp);
  Tcl_AppendResult(interp, context, ": expected ", expected, " but got '", 
		   value, "'", (char *) NULL);
  return TCL_ERROR;
}

int
NsfErrBadVal_(Tcl_Interp *interp, char *expected, char *value) {
  fprintf(stderr, "Deprecated call, recompile your program with nsf 1.5 or newer\n");
  Tcl_ResetResult(interp);
  Tcl_AppendResult(interp, ": expected ", expected, " but got '", 
		   value, "'", (char *) NULL);
  return TCL_ERROR;
}

extern int
NsfObjErrType(Tcl_Interp *interp, Tcl_Obj *value, char *type, char *parameterName) {
  Tcl_ResetResult(interp);
  Tcl_AppendResult(interp,"expected ", type, " but got \"",  ObjStr(value), "\"", 
                   parameterName ? " for parameter " : "",
                   parameterName ? parameterName : "",
                   (char *) NULL);
  return TCL_ERROR;
}
