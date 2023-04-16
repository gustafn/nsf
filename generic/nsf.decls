# -*- Tcl -*- 
#
# nx.decls --
#
#	This file contains the declarations for all supported public
#	functions that are exported by the Next Scripting Framework
#	(NSF) library via stubs tables.  This file is used to
#	generate nsfDecls.h.
#	
# Copyright (C) 1999-2014 Gustaf Neumann (a, b)
# Copyright (C) 1999-2007 Uwe Zdun (a, b)
# 
# (a) University of Essen
#     Specification of Software Systems
#     Altendorferstrasse 97-101
#     D-45143 Essen, Germany
# 
# (b) Vienna University of Economics and Business
#     Institute of Information Systems and New Media
#     A-1020, Welthandelsplatz 1
#     Vienna, Austria
# 
# This work is licensed under the MIT License https://www.opensource.org/licenses/MIT
#
# Copyright:
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# 

library nsf

interface nsf
hooks {nsfInt}

# Declare each of the functions in the public Tcl interface.  Note that
# the an index should never be reused for a different function in order
# to preserve backwards compatibility.

declare 0 generic {
    int Nsf_Init(Tcl_Interp *interp)
}
# 1 is reserved for safe init
#declare 1 generic {
#    int Nsf_SafeInit(Tcl_Interp *interp)
#}
declare 2 generic {
  struct Nsf_Class *NsfIsClass(Tcl_Interp *interp, ClientData cd)
}
declare 3 generic {
  struct Nsf_Object *NsfGetObject(Tcl_Interp *interp, const char *name)
}
declare 4 generic {
  struct Nsf_Class *NsfGetClass(Tcl_Interp *interp, const char *name)
}
declare 5 generic {
  int NsfDeleteObject(Tcl_Interp *interp, struct Nsf_Object *object)
}
declare 6 generic {
    int NsfRemoveObjectMethod(Tcl_Interp *interp, struct Nsf_Object *object,
			      const char *methodName)
}
declare 7 generic {
    int NsfRemoveClassMethod(Tcl_Interp *interp, struct Nsf_Class *class,
			     const char *methodName)
}
declare 8 generic {
  Tcl_Obj *Nsf_ObjSetVar2(struct Nsf_Object *object,
			  Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
			  Tcl_Obj *valueObj, unsigned int flags)
}
declare 9 generic {
  Tcl_Obj *Nsf_ObjGetVar2(struct Nsf_Object *object, 
			  Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
			  unsigned int flags)
}
declare 10 generic {
  int Nsf_UnsetVar2(struct Nsf_Object *object, Tcl_Interp *interp, 
		    const char *name1, const char *name2, unsigned int flags)
}
declare 11 generic {
  void NsfDStringVPrintf(Tcl_DString *dsPtr, const char *fmt, va_list argPtr)
}
declare 12 generic {
  int NsfPrintError(Tcl_Interp *interp, const char *fmt, ...)
}
declare 13 generic {
  int NsfErrInProc (Tcl_Interp *interp, Tcl_Obj *objName,
		    Tcl_Obj *clName, const char *procName)
}
declare 14 generic {
  int NsfObjErrType(Tcl_Interp *interp, const char *context, 
		    Tcl_Obj *value, const char *type,
		    Nsf_Param const *NsfObjErrType)
}
declare 15 generic {
  void NsfStackDump (Tcl_Interp *interp)
}
declare 16 generic {
  void NsfSetObjClientData(Tcl_Interp *interp, Nsf_Object *object, ClientData data)
}
declare 17 generic {
  ClientData NsfGetObjClientData(Tcl_Interp *interp, Nsf_Object *object)
}
declare 18 generic {
  void NsfSetClassClientData(Tcl_Interp *interp, Nsf_Class *cl, ClientData data)
}
declare 19 generic {
  ClientData NsfGetClassClientData(Tcl_Interp *interp, Nsf_Class *cl)
}
declare 20 generic {
  void NsfRequireObjNamespace(Tcl_Interp *interp, Nsf_Object *object)
}
declare 21 generic {
  int  NsfCallMethodWithArgs(Tcl_Interp *interp, Nsf_Object *object,
			     Tcl_Obj *methodObj, Tcl_Obj *arg1,
			     TCL_OBJC_T givenObjc, Tcl_Obj *const objv[],
			     unsigned int flags)
}
declare 22 generic {
  int NsfAddObjectMethod(Tcl_Interp *interp, struct Nsf_Object *object, 
                         const char *methodName, TCL_OBJCMDPROC_T *proc,
	                 ClientData clientData, Tcl_CmdDeleteProc *dp,
			 unsigned int flags)
}
declare 23 generic {
  int NsfAddClassMethod(Tcl_Interp *interp, struct Nsf_Class *class, 
			const char *methodName, TCL_OBJCMDPROC_T *proc,
			ClientData clientData, Tcl_CmdDeleteProc *dp,
			unsigned int flags)
}
declare 24 generic {
  int NsfCreate(Tcl_Interp *interp, Nsf_Class *class, Tcl_Obj *nameObj, 
		TCL_OBJC_T objc, Tcl_Obj *const objv[])
}
declare 25 generic {
  int Nsf_ArgumentParse(Tcl_Interp *interp, TCL_OBJC_T objc, Tcl_Obj *const objv[],
		  Nsf_Object *object, Tcl_Obj *procNameObj,
		  Nsf_Param const *paramPtr, TCL_OBJC_T nrParams, int serial,
		  unsigned int processFlags, Nsf_ParseContext *pcPtr)
}
declare 26 generic {
  void NsfLog(Tcl_Interp *interp, int requiredLevel, const char *fmt, ...)
}

declare 27 generic {
  int Nsf_PointerAdd(Tcl_Interp *interp, char *buffer, size_t size, const char *typeName, void *valuePtr)
}

declare 28 generic {
  int Nsf_PointerDelete(const char *key, void *valuePtr, int free)
}
declare 29 generic {
  int Nsf_PointerTypeRegister(Tcl_Interp *interp, const char* typeName, int *counterPtr)
}

declare 30 generic {
  int Nsf_ConvertToBoolean(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			   ClientData *clientData, Tcl_Obj **outObjPtr)
}
declare 31 generic {
  int Nsf_ConvertToClass(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			   ClientData *clientData, Tcl_Obj **outObjPtr)
}
declare 32 generic {
  int Nsf_ConvertToInt32(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			   ClientData *clientData, Tcl_Obj **outObjPtr)
}
declare 33 generic {
  int Nsf_ConvertToInteger(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			   ClientData *clientData, Tcl_Obj **outObjPtr)
}
declare 34 generic {
  int Nsf_ConvertToObject(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			 ClientData *clientData, Tcl_Obj **outObjPtr)
}
declare 35 generic {
  int Nsf_ConvertToPointer(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr)
}
declare 36 generic {
  int Nsf_ConvertToString(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr)
}
declare 37 generic {
  int Nsf_ConvertToTclobj(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr)
}
declare 38 generic {
  int Nsf_EnumerationTypeRegister(Tcl_Interp *interp, Nsf_EnumeratorConverterEntry *typeRecords)
}
declare 39 generic {
  int Nsf_CmdDefinitionRegister(Tcl_Interp *interp, Nsf_methodDefinition *definitionRecords)
}
declare 40 generic {
  int NsfArgumentError(Tcl_Interp *interp, const char *errorMsg, Nsf_Param const *paramPtr,
		       Tcl_Obj *cmdNameObj, Tcl_Obj *methodPathObj)
}
declare 41 generic {
  void Nsf_DStringPrintf(Tcl_DString *dsPtr, const char *fmt, ...)
}
