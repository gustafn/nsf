# xotcl.decls --
#
#	This file contains the declarations for all supported public
#	functions that are exported by the XOTcl library via the stubs table.
#	This file is used to generate the xotclDecls.h, xotclPlatDecls.h,
#	xotclStub.c, and xotclPlatStub.c files.
#	
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# See the file "tcl-license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 

library xotcl

interface xotcl
hooks {xotclInt}

# Declare each of the functions in the public Tcl interface.  Note that
# the an index should never be reused for a different function in order
# to preserve backwards compatibility.

declare 0 generic {
    int Next_Init(Tcl_Interp *interp)
}
# 1 is reserved for safe init
#declare 1 generic {
#    int Next_SafeInit(Tcl_Interp *interp)
#}
declare 2 generic {
    struct XOTcl_Class *XOTclIsClass(Tcl_Interp *interp, ClientData cd)
}
#declare 3 generic {
#
#}
declare 4 generic {
    struct XOTcl_Object *XOTclGetObject(Tcl_Interp *interp, CONST char *name)
}
declare 5 generic {
    struct XOTcl_Class *XOTclGetClass(Tcl_Interp *interp, CONST char *name)
}
declare 6 generic {
    int XOTclCreateObject(Tcl_Interp *interp, Tcl_Obj *name, struct XOTcl_Class *cl)
}
#declare 7 generic {
#
#}
#declare 8 generic {
#    int XOTclCreateClass(Tcl_Interp *interp, Tcl_Obj *name, struct XOTcl_Class *cl)
#}
declare 9 generic {
    int XOTclDeleteObject(Tcl_Interp *interp, struct XOTcl_Object *obj)
}
#declare 10 generic {
#    int XOTclDeleteClass(Tcl_Interp *interp, struct XOTcl_Class *cl)
#}
#declare 11 generic {
#    int XOTclAddObjectMethod(Tcl_Interp *interp, struct XOTcl_Object *obj, 
#                         CONST char* nm, Tcl_ObjCmdProc* proc,
#	                 ClientData cd, Tcl_CmdDeleteProc *dp)
#}
#declare 12 generic {
#    int XOTclAddClassMethod(Tcl_Interp *interp, struct XOTcl_Class *cl, 
#                         CONST char* nm, Tcl_ObjCmdProc* proc,
#	                 ClientData cd, Tcl_CmdDeleteProc *dp)
#}
declare 13 generic {
    int XOTclRemoveObjectMethod(Tcl_Interp *interp,struct XOTcl_Object *obj, CONST char *nm)
}
declare 14 generic {
    int XOTclRemoveClassMethod(Tcl_Interp *interp, struct XOTcl_Class *cl, CONST char *nm)
}
declare 15 generic {
    Tcl_Obj *XOTclOSetInstVar(struct XOTcl_Object *obj, Tcl_Interp *interp,
	       Tcl_Obj *name, Tcl_Obj *value, int flgs)
}
declare 16 generic {
    Tcl_Obj *XOTclOGetInstVar(struct XOTcl_Object *obj, Tcl_Interp *interp,
	       Tcl_Obj *name, int flgs)
}
#declare 17 generic {
#    int	XOTclInstVar(struct XOTcl_Object *obj, Tcl_Interp *interp,
#	     char *name, char *destName)
#}
#declare 18 generic {
#
#}
declare 19 generic {
    Tcl_Obj *XOTcl_ObjSetVar2(struct XOTcl_Object *obj,
                 Tcl_Interp *interp,Tcl_Obj *name1,Tcl_Obj *name2,
		 Tcl_Obj *value,int flgs)
}
declare 20 generic {
    Tcl_Obj *XOTcl_ObjGetVar2(struct XOTcl_Object *obj, 
                 Tcl_Interp *interp,Tcl_Obj *name1,Tcl_Obj *name2,
		 int flgs)
}
declare 21 generic {
    int XOTclUnsetInstVar2(struct XOTcl_Object *obj, Tcl_Interp *interp, 
                           CONST char *name1, CONST char *name2, 
		           int flgs)
}
#declare 22 generic {
#    int XOTcl_TraceObjCmd(ClientData cd, Tcl_Interp *interp,
#                          int objc, Tcl_Obj *CONST objv[])
#}
declare 23 generic {
    int XOTclErrMsg(Tcl_Interp *interp, char *msg, Tcl_FreeProc *type)
}
declare 24 generic {
    int XOTclVarErrMsg(Tcl_Interp  *interp, ...)
}
declare 25 generic {
    int XOTclErrInProc (Tcl_Interp *interp, Tcl_Obj *objName,
		Tcl_Obj *clName, CONST char *procName)
}
#declare 26 generic {
#
#}
declare 27 generic {
   int XOTclErrBadVal_(Tcl_Interp *interp, char *expected, char *value)
}
declare 28 generic {
    int XOTclObjErrType(Tcl_Interp *interp, Tcl_Obj *nm, char *wt, char *parameterName)
}
declare 29 generic {
    void XOTclStackDump (Tcl_Interp *interp)
}
#declare 30 generic {
#    void XOTclCallStackDump (Tcl_Interp *interp)
#}
#declare 31 generic {
#    void XOTclDeprecatedMsg(char *oldCmd, char *newCmd)
#}
declare 32 generic {
    void XOTclSetObjClientData(XOTcl_Object *obj, ClientData data)
}
declare 33 generic {
    ClientData XOTclGetObjClientData(XOTcl_Object *obj)
}
declare 34 generic {
    void XOTclSetClassClientData(XOTcl_Class *cl, ClientData data)
}
declare 35 generic {
    ClientData XOTclGetClassClientData(XOTcl_Class *cl)
}
declare 36 generic {
    void XOTclRequireObjNamespace(Tcl_Interp *interp, XOTcl_Object *obj)
}
declare 37 generic {
    int XOTclErrBadVal(Tcl_Interp *interp, char *context, char *expected, CONST char *value)
}
declare 38 generic {
        int XOTclNextObjCmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
}
declare 39 generic {
        int  XOTclCallMethodWithArgs(ClientData cd, Tcl_Interp *interp, 
                Tcl_Obj *method, Tcl_Obj *arg,
       	         int objc, Tcl_Obj *CONST objv[], int flags)
}
declare 40 generic {
    int XOTclObjErrArgCnt(Tcl_Interp *interp, Tcl_Obj *cmdName, Tcl_Obj *methodName, 
                                     char *arglist)
}
declare 41 generic {
    int XOTclAddObjectMethod(Tcl_Interp *interp, struct XOTcl_Object *obj, 
                         CONST char *nm, Tcl_ObjCmdProc *proc,
	                 ClientData cd, Tcl_CmdDeleteProc *dp, int flags)
}
declare 42 generic {
   int XOTclAddClassMethod(Tcl_Interp *interp, struct XOTcl_Class *cl, 
                         CONST char *nm, Tcl_ObjCmdProc *proc,
	                 ClientData cd, Tcl_CmdDeleteProc *dp, int flags)
}
declare 43 generic {
    int XOTclCreate(Tcl_Interp *in, XOTcl_Class *class, Tcl_Obj *name, ClientData data,
            int objc, Tcl_Obj *CONST objv[])
}
