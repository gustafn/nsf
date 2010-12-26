# -*- Tcl -*- nx.decls --
#
#	This file contains the declarations for all supported public
#	functions that are exported by the Next scripting library via the stubs table.
#	This file is used to generate the nxDecls.h, nxPlatDecls.h,
#	nxStub.c, and nxPlatStub.c files.
#	
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# See the file "tcl-license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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
  struct Nsf_Object *NsfGetObject(Tcl_Interp *interp, CONST char *name)
}
declare 4 generic {
  struct Nsf_Class *NsfGetClass(Tcl_Interp *interp, CONST char *name)
}
declare 5 generic {
  int NsfDeleteObject(Tcl_Interp *interp, struct Nsf_Object *obj)
}
declare 6 generic {
  int NsfRemoveObjectMethod(Tcl_Interp *interp, struct Nsf_Object *obj, CONST char *nm)
}
declare 7 generic {
  int NsfRemoveClassMethod(Tcl_Interp *interp, struct Nsf_Class *cl, CONST char *nm)
}
declare 8 generic {
  Tcl_Obj *Nsf_ObjSetVar2(struct Nsf_Object *obj,
			  Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
			  Tcl_Obj *value, int flgs)
}
declare 9 generic {
  Tcl_Obj *Nsf_ObjGetVar2(struct Nsf_Object *obj, 
			  Tcl_Interp *interp, Tcl_Obj *name1, Tcl_Obj *name2,
			  int flgs)
}
declare 10 generic {
  int Nsf_UnsetVar2(struct Nsf_Object *obj, Tcl_Interp *interp, 
		    CONST char *name1, CONST char *name2, int flgs)
}
declare 11 generic {
  void NsfDStringPrintf(Tcl_DString *dsPtr, CONST char *fmt, va_list apSrc)
}
declare 12 generic {
  int NsfPrintError(Tcl_Interp *interp, CONST char *fmt, ...)
}
declare 13 generic {
  int NsfErrInProc (Tcl_Interp *interp, Tcl_Obj *objName,
		    Tcl_Obj *clName, CONST char *procName)
}
declare 14 generic {
  int NsfObjErrType(Tcl_Interp *interp, CONST char *context, 
		    Tcl_Obj *value, CONST char *type, Nsf_Param CONST *pPtr)
}
declare 15 generic {
  void NsfStackDump (Tcl_Interp *interp)
}
declare 16 generic {
  void NsfSetObjClientData(Nsf_Object *obj, ClientData data)
}
declare 17 generic {
  ClientData NsfGetObjClientData(Nsf_Object *obj)
}
declare 18 generic {
  void NsfSetClassClientData(Nsf_Class *cl, ClientData data)
}
declare 19 generic {
  ClientData NsfGetClassClientData(Nsf_Class *cl)
}
declare 20 generic {
  void NsfRequireObjNamespace(Tcl_Interp *interp, Nsf_Object *obj)
}
declare 21 generic {
  int  NsfCallMethodWithArgs(ClientData cd, Tcl_Interp *interp, 
			     Tcl_Obj *method, Tcl_Obj *arg,
			     int objc, Tcl_Obj *CONST objv[], int flags)
}
declare 22 generic {
  int NsfAddObjectMethod(Tcl_Interp *interp, struct Nsf_Object *obj, 
                         CONST char *nm, Tcl_ObjCmdProc *proc,
	                 ClientData cd, Tcl_CmdDeleteProc *dp, int flags)
}
declare 23 generic {
  int NsfAddClassMethod(Tcl_Interp *interp, struct Nsf_Class *cl, 
			CONST char *nm, Tcl_ObjCmdProc *proc,
			ClientData cd, Tcl_CmdDeleteProc *dp, int flags)
}
declare 24 generic {
  int NsfCreate(Tcl_Interp *in, Nsf_Class *class, Tcl_Obj *name, 
		int objc, Tcl_Obj *CONST objv[])
}
