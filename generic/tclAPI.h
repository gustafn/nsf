
typedef struct {
  char *methodName;
  Tcl_ObjCmdProc *proc;
  interfaceDefinition ifd;
} methodDefinition2;

static int parse2(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
                  int idx, parseContext *pc);
static int getMatchObject3(Tcl_Interp *interp, Tcl_Obj *patternObj,  parseContext *pc,
                           XOTclObject **matchObject, char **pattern);
  
static int XOTclClassInfoHeritageMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstancesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstbodyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstcommandsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstdefaultMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstfilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstfilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstforwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstinvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstmixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstmixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstmixinofMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

static int XOTclClassInfoHeritageMethod(Tcl_Interp *interp, XOTclClass * class, char * pattern);
static int XOTclClassInfoInstancesMethod(Tcl_Interp *interp, XOTclClass * class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoInstargsMethod(Tcl_Interp *interp, XOTclClass * class, char * methodName);
static int XOTclClassInfoInstbodyMethod(Tcl_Interp *interp, XOTclClass * class, char * methodName);
static int XOTclClassInfoInstcommandsMethod(Tcl_Interp *interp, XOTclClass * class, char * pattern);
static int XOTclClassInfoInstdefaultMethod(Tcl_Interp *interp, XOTclClass * class, char * methodName, char * arg, Tcl_Obj * var);
static int XOTclClassInfoInstfilterMethod(Tcl_Interp *interp, XOTclClass * class, int withGuards, char * pattern);
static int XOTclClassInfoInstfilterguardMethod(Tcl_Interp *interp, XOTclClass * class, char * filter);
static int XOTclClassInfoInstforwardMethod(Tcl_Interp *interp, XOTclClass * class, int withDefinition, char * methodName);
static int XOTclClassInfoInstinvarMethod(Tcl_Interp *interp, XOTclClass * class);
static int XOTclClassInfoInstmixinMethod(Tcl_Interp *interp, XOTclClass * class, int withClosure, int withGuards, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoInstmixinguardMethod(Tcl_Interp *interp, XOTclClass * class, char * mixin);
static int XOTclClassInfoInstmixinofMethod(Tcl_Interp *interp, XOTclClass * class, int withClosure, char *patternString, XOTclObject *patternObj);

enum {
 XOTclClassInfoHeritageMethodIdx,
 XOTclClassInfoInstancesMethodIdx,
 XOTclClassInfoInstargsMethodIdx,
 XOTclClassInfoInstbodyMethodIdx,
 XOTclClassInfoInstcommandsMethodIdx,
 XOTclClassInfoInstdefaultMethodIdx,
 XOTclClassInfoInstfilterMethodIdx,
 XOTclClassInfoInstfilterguardMethodIdx,
 XOTclClassInfoInstforwardMethodIdx,
 XOTclClassInfoInstinvarMethodIdx,
 XOTclClassInfoInstmixinMethodIdx,
 XOTclClassInfoInstmixinguardMethodIdx,
 XOTclClassInfoInstmixinofMethodIdx
} XOTclMethods;


static int
XOTclClassInfoHeritageMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoHeritageMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclClassInfoHeritageMethod(interp, class, pattern);

  }
}
  
static int
XOTclClassInfoInstancesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstancesMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int)pc.clientData[1];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj * pattern = (Tcl_Obj *)pc.clientData[2];

    if (getMatchObject3(interp, pattern,  &pc, &patternObj, &patternString) == -1) {
      return TCL_OK;
    }
          
    return XOTclClassInfoInstancesMethod(interp, class, withClosure, patternString, patternObj);

  }
}
  
static int
XOTclClassInfoInstargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstargsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclClassInfoInstargsMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstbodyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstbodyMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclClassInfoInstbodyMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstcommandsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstcommandsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclClassInfoInstcommandsMethod(interp, class, pattern);

  }
}
  
static int
XOTclClassInfoInstdefaultMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstdefaultMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];
    char * arg = (char *)pc.clientData[2];
    Tcl_Obj * var = (Tcl_Obj *)pc.clientData[3];

    return XOTclClassInfoInstdefaultMethod(interp, class, methodName, arg, var);

  }
}
  
static int
XOTclClassInfoInstfilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstfilterMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    int withGuards = (int)pc.clientData[1];
    char * pattern = (char *)pc.clientData[2];

    return XOTclClassInfoInstfilterMethod(interp, class, withGuards, pattern);

  }
}
  
static int
XOTclClassInfoInstfilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstfilterguardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * filter = (char *)pc.clientData[1];

    return XOTclClassInfoInstfilterguardMethod(interp, class, filter);

  }
}
  
static int
XOTclClassInfoInstforwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstforwardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    int withDefinition = (int)pc.clientData[1];
    char * methodName = (char *)pc.clientData[2];

    return XOTclClassInfoInstforwardMethod(interp, class, withDefinition, methodName);

  }
}
  
static int
XOTclClassInfoInstinvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstinvarMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];

    return XOTclClassInfoInstinvarMethod(interp, class);

  }
}
  
static int
XOTclClassInfoInstmixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstmixinMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int)pc.clientData[1];
    int withGuards = (int)pc.clientData[2];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj * pattern = (Tcl_Obj *)pc.clientData[3];

    if (getMatchObject3(interp, pattern,  &pc, &patternObj, &patternString) == -1) {
      return TCL_OK;
    }
          
    return XOTclClassInfoInstmixinMethod(interp, class, withClosure, withGuards, patternString, patternObj);

  }
}
  
static int
XOTclClassInfoInstmixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstmixinguardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * mixin = (char *)pc.clientData[1];

    return XOTclClassInfoInstmixinguardMethod(interp, class, mixin);

  }
}
  
static int
XOTclClassInfoInstmixinofMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstmixinofMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int)pc.clientData[1];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj * pattern = (Tcl_Obj *)pc.clientData[2];

    if (getMatchObject3(interp, pattern,  &pc, &patternObj, &patternString) == -1) {
      return TCL_OK;
    }
          
    return XOTclClassInfoInstmixinofMethod(interp, class, withClosure, patternString, patternObj);

  }
}
  
static methodDefinition2 methodDefinitons[] = {
{"instances", XOTclClassInfoHeritageMethodStub, {
  {"class", 1, 0, "class"},
  {"pattern", 0, 0, NULL}}
},
{"instances", XOTclClassInfoInstancesMethodStub, {
  {"class", 1, 0, "class"},
  {"-closure", 0, 0, NULL},
  {"pattern", 0, 0, "objpattern"}}
},
{"instargs", XOTclClassInfoInstargsMethodStub, {
  {"class", 1, 0, "class"},
  {"methodName", 1, 0, NULL}}
},
{"instbody", XOTclClassInfoInstbodyMethodStub, {
  {"class", 1, 0, "class"},
  {"methodName", 1, 0, NULL}}
},
{"instances", XOTclClassInfoInstcommandsMethodStub, {
  {"class", 1, 0, "class"},
  {"pattern", 0, 0, NULL}}
},
{"instdefault", XOTclClassInfoInstdefaultMethodStub, {
  {"class", 1, 0, "class"},
  {"methodName", 1, 0, NULL},
  {"arg", 1, 0, NULL},
  {"var", 1, 0, "tclobj"}}
},
{"instfilter", XOTclClassInfoInstfilterMethodStub, {
  {"class", 1, 0, "class"},
  {"-guards", 0, 0, NULL},
  {"pattern", 0, 0, NULL}}
},
{"instfilterguard", XOTclClassInfoInstfilterguardMethodStub, {
  {"class", 1, 0, "class"},
  {"filter", 1, 0, NULL}}
},
{"instforward", XOTclClassInfoInstforwardMethodStub, {
  {"class", 1, 0, "class"},
  {"-definition", 0, 0, NULL},
  {"methodName", 1, 0, NULL}}
},
{"instinvar", XOTclClassInfoInstinvarMethodStub, {
  {"class", 1, 0, "class"}}
},
{"instmixin", XOTclClassInfoInstmixinMethodStub, {
  {"class", 1, 0, "class"},
  {"-closure", 0, 0, NULL},
  {"-guards", 0, 0, NULL},
  {"pattern", 0, 0, "objpattern"}}
},
{"instmixinguard", XOTclClassInfoInstmixinguardMethodStub, {
  {"class", 1, 0, "class"},
  {"mixin", 1, 0, NULL}}
},
{"instmixinof", XOTclClassInfoInstmixinofMethodStub, {
  {"class", 1, 0, "class"},
  {"-closure", 0, 0, NULL},
  {"pattern", 0, 0, "objpattern"}}
}
};

