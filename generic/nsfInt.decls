# nsfInt.decls --
#
#	This file contains the declarations for all unsupported
#	functions that are exported by the Tcl library.  This file
#	is used to generate the itclIntDecls.h and itclIntStub.c
#	files
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# See the file "tcl-license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 

library nsf

# Define the unsupported generic interfaces.

interface nsfInt


#
# Functions used within the package, but not considered "public"
#

#declare 0 generic {

#}
#declare 1 generic {

#}
# declare 2 generic {
#     int NsfErrInProc (Tcl_Interp *in, Tcl_Obj* objName,
# 		Tcl_Obj* clName, char* procName)
# }
# declare 3 generic {
#     int NsfObjErrArgCnt(Tcl_Interp *in, Tcl_Obj *cmdname, char *arglist)
# }
# declare 4 generic {
#     int NsfErrBadVal(Tcl_Interp *in, char *expected, char *value)
# }
# declare 5 generic {
#     int NsfObjErrType(Tcl_Interp *in, Tcl_Obj *nm, char* wt)
# }
# declare 6 generic {
#     void NsfStackTrace (Tcl_Interp* in)
# }
# declare 7 generic {
#     void NsfCallStackTrace (Tcl_Interp* in)
# }
#declare 8 generic {
#    void NsfFilterTrace (Tcl_Interp* in)
#}
#declare 9 generic {
#    int NsfIsType(NsfObject* obj, NsfClass* type)
#}
#declare 10 generic {
#    void NsfRemoveClasses(NsfClasses* sl)
#}
# declare 11 generic {
#     NsfClasses** NsfAddClass(NsfClasses** cList, NsfClass* cl, ClientData cd)
# }
# declare 12 generic {

# }
# declare 13 generic {
#     NsfClasses* NsfComputeDependents(register NsfClass* cl)
# }
# declare 14 generic {
#     void NsfDeprecatedMsg(char* oldCmd, char* newCmd)
# }
# declare 15 generic {
#     void NsfSetObjClientData(NsfObject* obj, ClientData data)
# }
# declare 16 generic {
#     ClientData NsfGetObjClientData(NsfObject* obj)
# }
# declare 17 generic {
#     void NsfSetClassClientData(NsfClass* cl, ClientData data)
# }
# declare 18 generic {
#     ClientData NsfGetClassClientData(NsfClass* cl)
# }
# declare 19 generic {
#     void NsfRequireObjectOpt(NsfObject* obj)
# }
# declare 20 generic {
#     void NsfRequireClassOpt(NsfClass* cl)
# }
# declare 21 generic {
#     void NsfRequireObjNamespace(Tcl_Interp* in, NsfObject* obj)
# }
# declare 22 generic {
    
# }
# declare 23 generic {
    
# }
# declare 24 generic {
    
# }
# declare 25 generic {
    
# }
# declare 26 generic {
    
# }
# declare 27 generic {
    
# }
# declare 28 generic {
    
# }
# declare 29 generic {
    
# }
# declare 30 generic {
    
# }
