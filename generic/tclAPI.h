
typedef struct {
  char *methodName;
  Tcl_ObjCmdProc *proc;
  interfaceDefinition ifd;
} methodDefinition2;
  
static int XOTclClassInfoHeritageMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstancesMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstargsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstbodyMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstcommandsMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstdefaultMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstfilterMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstfilterguardMethod(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

enum {
 XOTclClassInfoHeritageMethodIdx,
 XOTclClassInfoInstancesMethodIdx,
 XOTclClassInfoInstargsMethodIdx,
 XOTclClassInfoInstbodyMethodIdx,
 XOTclClassInfoInstcommandsMethodIdx,
 XOTclClassInfoInstdefaultMethodIdx,
 XOTclClassInfoInstfilterMethodIdx,
 XOTclClassInfoInstfilterguardMethodIdx
} XOTclMethods;

static methodDefinition2 methodDefinitons[] = {
{"instances", XOTclClassInfoHeritageMethod, {
  {"class", 1, 0, "class"},
  {"pattern", 0, 0, NULL}}
},
{"instances", XOTclClassInfoInstancesMethod, {
  {"class", 1, 0, "class"},
  {"-closure", 0, 0, NULL},
  {"pattern", 0, 0, "objpattern"}}
},
{"instargs", XOTclClassInfoInstargsMethod, {
  {"class", 1, 0, "class"},
  {"methodName", 1, 0, NULL}}
},
{"instbody", XOTclClassInfoInstbodyMethod, {
  {"class", 1, 0, "class"},
  {"methodName", 1, 0, NULL}}
},
{"instances", XOTclClassInfoInstcommandsMethod, {
  {"class", 1, 0, "class"},
  {"pattern", 0, 0, NULL}}
},
{"instdefault", XOTclClassInfoInstdefaultMethod, {
  {"class", 1, 0, "class"},
  {"methodName", 1, 0, NULL},
  {"arg", 1, 0, NULL},
  {"var", 1, 0, NULL}}
},
{"instfilter", XOTclClassInfoInstfilterMethod, {
  {"class", 1, 0, "class"},
  {"-guards", 0, 0, NULL},
  {"pattern", 0, 0, NULL}}
},
{"instfilterguard", XOTclClassInfoInstfilterguardMethod, {
  {"class", 1, 0, "class"},
  {"filter", 1, 0, NULL}}
}
};

