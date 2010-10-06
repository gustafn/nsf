# nx.decls --
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
    int NsfCreateObject(Tcl_Interp *interp, Tcl_Obj *name, struct Nsf_Class *cl)
}
declare 6 generic {
    int NsfDeleteObject(Tcl_Interp *interp, struct Nsf_Object *obj)
}
declare 7 generic {
    int NsfRemoveObjectMethod(Tcl_Interp *interp,struct Nsf_Object *obj, CONST char *nm)
}
declare 8 generic {
    int NsfRemoveClassMethod(Tcl_Interp *interp, struct Nsf_Class *cl, CONST char *nm)
}
declare 9 generic {
    Tcl_Obj *NsfOSetInstVar(struct Nsf_Object *obj, Tcl_Interp *interp,
	       Tcl_Obj *name, Tcl_Obj *value, int flgs)
}
declare 10 generic {
    Tcl_Obj *NsfOGetInstVar(struct Nsf_Object *obj, Tcl_Interp *interp,
	       Tcl_Obj *name, int flgs)
}
declare 11 generic {
    Tcl_Obj *Nsf_ObjSetVar2(struct Nsf_Object *obj,
                 Tcl_Interp *interp,Tcl_Obj *name1,Tcl_Obj *name2,
		 Tcl_Obj *value,int flgs)
}
declare 12 generic {
    Tcl_Obj *Nsf_ObjGetVar2(struct Nsf_Object *obj, 
                 Tcl_Interp *interp,Tcl_Obj *name1,Tcl_Obj *name2,
		 int flgs)
}
declare 13 generic {
    int NsfUnsetInstVar2(struct Nsf_Object *obj, Tcl_Interp *interp, 
                           CONST char *name1, CONST char *name2, 
		           int flgs)
}
declare 14 generic {
    int NsfErrMsg(Tcl_Interp *interp, char *msg, Tcl_FreeProc *type)
}
declare 15 generic {
    int NsfVarErrMsg(Tcl_Interp  *interp, ...)
}
declare 16 generic {
    int NsfErrInProc (Tcl_Interp *interp, Tcl_Obj *objName,
		Tcl_Obj *clName, CONST char *procName)
}
declare 17 generic {
   int NsfErrBadVal_(Tcl_Interp *interp, char *expected, char *value)
}
declare 18 generic {
    int NsfObjErrType(Tcl_Interp *interp, Tcl_Obj *nm, char *wt, char *parameterName)
}
declare 19 generic {
    void NsfStackDump (Tcl_Interp *interp)
}
declare 20 generic {
    void NsfSetObjClientData(Nsf_Object *obj, ClientData data)
}
declare 21 generic {
    ClientData NsfGetObjClientData(Nsf_Object *obj)
}
declare 22 generic {
    void NsfSetClassClientData(Nsf_Class *cl, ClientData data)
}
declare 23 generic {
    ClientData NsfGetClassClientData(Nsf_Class *cl)
}
declare 24 generic {
    void NsfRequireObjNamespace(Tcl_Interp *interp, Nsf_Object *obj)
}
declare 25 generic {
    int NsfErrBadVal(Tcl_Interp *interp, char *context, char *expected, CONST char *value)
}
declare 26 generic {
        int NsfNextObjCmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
}
declare 27 generic {
        int  NsfCallMethodWithArgs(ClientData cd, Tcl_Interp *interp, 
                Tcl_Obj *method, Tcl_Obj *arg,
       	         int objc, Tcl_Obj *CONST objv[], int flags)
}
declare 28 generic {
    int NsfObjErrArgCnt(Tcl_Interp *interp, Tcl_Obj *cmdName, Tcl_Obj *methodName, 
                                     char *arglist)
}
declare 29 generic {
    int NsfAddObjectMethod(Tcl_Interp *interp, struct Nsf_Object *obj, 
                         CONST char *nm, Tcl_ObjCmdProc *proc,
	                 ClientData cd, Tcl_CmdDeleteProc *dp, int flags)
}
declare 30 generic {
   int NsfAddClassMethod(Tcl_Interp *interp, struct Nsf_Class *cl, 
                         CONST char *nm, Tcl_ObjCmdProc *proc,
	                 ClientData cd, Tcl_CmdDeleteProc *dp, int flags)
}
declare 31 generic {
    int NsfCreate(Tcl_Interp *in, Nsf_Class *class, Tcl_Obj *name, ClientData data,
            int objc, Tcl_Obj *CONST objv[])
}
