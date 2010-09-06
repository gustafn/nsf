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
#declare 3 generic {
#
#}
declare 4 generic {
    struct Nsf_Object *NsfGetObject(Tcl_Interp *interp, CONST char *name)
}
declare 5 generic {
    struct Nsf_Class *NsfGetClass(Tcl_Interp *interp, CONST char *name)
}
declare 6 generic {
    int NsfCreateObject(Tcl_Interp *interp, Tcl_Obj *name, struct Nsf_Class *cl)
}
#declare 7 generic {
#
#}
#declare 8 generic {
#    int NsfCreateClass(Tcl_Interp *interp, Tcl_Obj *name, struct Nsf_Class *cl)
#}
declare 9 generic {
    int NsfDeleteObject(Tcl_Interp *interp, struct Nsf_Object *obj)
}
#declare 10 generic {
#    int NsfDeleteClass(Tcl_Interp *interp, struct Nsf_Class *cl)
#}
#declare 11 generic {
#    int NsfAddObjectMethod(Tcl_Interp *interp, struct Nsf_Object *obj, 
#                         CONST char* nm, Tcl_ObjCmdProc* proc,
#	                 ClientData cd, Tcl_CmdDeleteProc *dp)
#}
#declare 12 generic {
#    int NsfAddClassMethod(Tcl_Interp *interp, struct Nsf_Class *cl, 
#                         CONST char* nm, Tcl_ObjCmdProc* proc,
#	                 ClientData cd, Tcl_CmdDeleteProc *dp)
#}
declare 13 generic {
    int NsfRemoveObjectMethod(Tcl_Interp *interp,struct Nsf_Object *obj, CONST char *nm)
}
declare 14 generic {
    int NsfRemoveClassMethod(Tcl_Interp *interp, struct Nsf_Class *cl, CONST char *nm)
}
declare 15 generic {
    Tcl_Obj *NsfOSetInstVar(struct Nsf_Object *obj, Tcl_Interp *interp,
	       Tcl_Obj *name, Tcl_Obj *value, int flgs)
}
declare 16 generic {
    Tcl_Obj *NsfOGetInstVar(struct Nsf_Object *obj, Tcl_Interp *interp,
	       Tcl_Obj *name, int flgs)
}
#declare 17 generic {
#    int	NsfInstVar(struct Nsf_Object *obj, Tcl_Interp *interp,
#	     char *name, char *destName)
#}
#declare 18 generic {
#
#}
declare 19 generic {
    Tcl_Obj *Nsf_ObjSetVar2(struct Nsf_Object *obj,
                 Tcl_Interp *interp,Tcl_Obj *name1,Tcl_Obj *name2,
		 Tcl_Obj *value,int flgs)
}
declare 20 generic {
    Tcl_Obj *Nsf_ObjGetVar2(struct Nsf_Object *obj, 
                 Tcl_Interp *interp,Tcl_Obj *name1,Tcl_Obj *name2,
		 int flgs)
}
declare 21 generic {
    int NsfUnsetInstVar2(struct Nsf_Object *obj, Tcl_Interp *interp, 
                           CONST char *name1, CONST char *name2, 
		           int flgs)
}
#declare 22 generic {
#    int Nsf_TraceObjCmd(ClientData cd, Tcl_Interp *interp,
#                          int objc, Tcl_Obj *CONST objv[])
#}
declare 23 generic {
    int NsfErrMsg(Tcl_Interp *interp, char *msg, Tcl_FreeProc *type)
}
declare 24 generic {
    int NsfVarErrMsg(Tcl_Interp  *interp, ...)
}
declare 25 generic {
    int NsfErrInProc (Tcl_Interp *interp, Tcl_Obj *objName,
		Tcl_Obj *clName, CONST char *procName)
}
#declare 26 generic {
#
#}
declare 27 generic {
   int NsfErrBadVal_(Tcl_Interp *interp, char *expected, char *value)
}
declare 28 generic {
    int NsfObjErrType(Tcl_Interp *interp, Tcl_Obj *nm, char *wt, char *parameterName)
}
declare 29 generic {
    void NsfStackDump (Tcl_Interp *interp)
}
#declare 30 generic {
#    void NsfCallStackDump (Tcl_Interp *interp)
#}
#declare 31 generic {
#    void NsfDeprecatedMsg(char *oldCmd, char *newCmd)
#}
declare 32 generic {
    void NsfSetObjClientData(Nsf_Object *obj, ClientData data)
}
declare 33 generic {
    ClientData NsfGetObjClientData(Nsf_Object *obj)
}
declare 34 generic {
    void NsfSetClassClientData(Nsf_Class *cl, ClientData data)
}
declare 35 generic {
    ClientData NsfGetClassClientData(Nsf_Class *cl)
}
declare 36 generic {
    void NsfRequireObjNamespace(Tcl_Interp *interp, Nsf_Object *obj)
}
declare 37 generic {
    int NsfErrBadVal(Tcl_Interp *interp, char *context, char *expected, CONST char *value)
}
declare 38 generic {
        int NsfNextObjCmd(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
}
declare 39 generic {
        int  NsfCallMethodWithArgs(ClientData cd, Tcl_Interp *interp, 
                Tcl_Obj *method, Tcl_Obj *arg,
       	         int objc, Tcl_Obj *CONST objv[], int flags)
}
declare 40 generic {
    int NsfObjErrArgCnt(Tcl_Interp *interp, Tcl_Obj *cmdName, Tcl_Obj *methodName, 
                                     char *arglist)
}
declare 41 generic {
    int NsfAddObjectMethod(Tcl_Interp *interp, struct Nsf_Object *obj, 
                         CONST char *nm, Tcl_ObjCmdProc *proc,
	                 ClientData cd, Tcl_CmdDeleteProc *dp, int flags)
}
declare 42 generic {
   int NsfAddClassMethod(Tcl_Interp *interp, struct Nsf_Class *cl, 
                         CONST char *nm, Tcl_ObjCmdProc *proc,
	                 ClientData cd, Tcl_CmdDeleteProc *dp, int flags)
}
declare 43 generic {
    int NsfCreate(Tcl_Interp *in, Nsf_Class *class, Tcl_Obj *name, ClientData data,
            int objc, Tcl_Obj *CONST objv[])
}
