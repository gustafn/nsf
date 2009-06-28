
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
static int XOTclClassInfoInstnonposargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstparametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstpostMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstpreMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstprocsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoMixinofMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoParameterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoSlotsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoSubclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoSuperclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoArgsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoBodyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoCheckMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoCommandsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoDefaultMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoFilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

static int XOTclClassInfoHeritageMethod(Tcl_Interp *interp, XOTclClass *class, char *pattern);
static int XOTclClassInfoInstancesMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoInstargsMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName);
static int XOTclClassInfoInstbodyMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName);
static int XOTclClassInfoInstcommandsMethod(Tcl_Interp *interp, XOTclClass *class, char *pattern);
static int XOTclClassInfoInstdefaultMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName, char *arg, Tcl_Obj *var);
static int XOTclClassInfoInstfilterMethod(Tcl_Interp *interp, XOTclClass *class, int withGuards, char *pattern);
static int XOTclClassInfoInstfilterguardMethod(Tcl_Interp *interp, XOTclClass *class, char *filter);
static int XOTclClassInfoInstforwardMethod(Tcl_Interp *interp, XOTclClass *class, int withDefinition, char *methodName);
static int XOTclClassInfoInstinvarMethod(Tcl_Interp *interp, XOTclClass *class);
static int XOTclClassInfoInstmixinMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, int withGuards, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoInstmixinguardMethod(Tcl_Interp *interp, XOTclClass *class, char *mixin);
static int XOTclClassInfoInstmixinofMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoInstnonposargsMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName);
static int XOTclClassInfoInstparametercmdMethod(Tcl_Interp *interp, XOTclClass *class, char *pattern);
static int XOTclClassInfoInstpostMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName);
static int XOTclClassInfoInstpreMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName);
static int XOTclClassInfoInstprocsMethod(Tcl_Interp *interp, XOTclClass *class, char *pattern);
static int XOTclClassInfoMixinofMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoParameterMethod(Tcl_Interp *interp, XOTclClass *class);
static int XOTclClassInfoSlotsMethod(Tcl_Interp *interp, XOTclClass *class);
static int XOTclClassInfoSubclassMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoSuperclassMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *pattern);
static int XOTclObjInfoArgsMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName);
static int XOTclObjInfoBodyMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName);
static int XOTclObjInfoCheckMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclObjInfoChildrenMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern);
static int XOTclObjInfoClassMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclObjInfoCommandsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern);
static int XOTclObjInfoDefaultMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName, char *arg, Tcl_Obj *var);
static int XOTclObjInfoFilterMethod(Tcl_Interp *interp, XOTclObject *object, int withOrder, int withGuards, char *pattern);
static int XOTclObjInfoFilterguardMethod(Tcl_Interp *interp, XOTclObject *object, char *filter);
static int XOTclObjInfoForwardMethod(Tcl_Interp *interp, XOTclObject *object, int withDefinition, char *methodName);
static int XOTclObjInfoHasnamespaceMethod(Tcl_Interp *interp, XOTclObject *object);

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
 XOTclClassInfoInstmixinofMethodIdx,
 XOTclClassInfoInstnonposargsMethodIdx,
 XOTclClassInfoInstparametercmdMethodIdx,
 XOTclClassInfoInstpostMethodIdx,
 XOTclClassInfoInstpreMethodIdx,
 XOTclClassInfoInstprocsMethodIdx,
 XOTclClassInfoMixinofMethodIdx,
 XOTclClassInfoParameterMethodIdx,
 XOTclClassInfoSlotsMethodIdx,
 XOTclClassInfoSubclassMethodIdx,
 XOTclClassInfoSuperclassMethodIdx,
 XOTclObjInfoArgsMethodIdx,
 XOTclObjInfoBodyMethodIdx,
 XOTclObjInfoCheckMethodIdx,
 XOTclObjInfoChildrenMethodIdx,
 XOTclObjInfoClassMethodIdx,
 XOTclObjInfoCommandsMethodIdx,
 XOTclObjInfoDefaultMethodIdx,
 XOTclObjInfoFilterMethodIdx,
 XOTclObjInfoFilterguardMethodIdx,
 XOTclObjInfoForwardMethodIdx,
 XOTclObjInfoHasnamespaceMethodIdx
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
    int  withClosure = (int )pc.clientData[1];
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
    int  withGuards = (int )pc.clientData[1];
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
    int  withDefinition = (int )pc.clientData[1];
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
    int  withClosure = (int )pc.clientData[1];
    int  withGuards = (int )pc.clientData[2];
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
    int  withClosure = (int )pc.clientData[1];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj * pattern = (Tcl_Obj *)pc.clientData[2];

    if (getMatchObject3(interp, pattern,  &pc, &patternObj, &patternString) == -1) {
      return TCL_OK;
    }
          
    return XOTclClassInfoInstmixinofMethod(interp, class, withClosure, patternString, patternObj);

  }
}
  
static int
XOTclClassInfoInstnonposargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstnonposargsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclClassInfoInstnonposargsMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstparametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstparametercmdMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclClassInfoInstparametercmdMethod(interp, class, pattern);

  }
}
  
static int
XOTclClassInfoInstpostMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstpostMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclClassInfoInstpostMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstpreMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstpreMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclClassInfoInstpreMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstprocsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoInstprocsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclClassInfoInstprocsMethod(interp, class, pattern);

  }
}
  
static int
XOTclClassInfoMixinofMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoMixinofMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    int  withClosure = (int )pc.clientData[1];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj * pattern = (Tcl_Obj *)pc.clientData[2];

    if (getMatchObject3(interp, pattern,  &pc, &patternObj, &patternString) == -1) {
      return TCL_OK;
    }
          
    return XOTclClassInfoMixinofMethod(interp, class, withClosure, patternString, patternObj);

  }
}
  
static int
XOTclClassInfoParameterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoParameterMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];

    return XOTclClassInfoParameterMethod(interp, class);

  }
}
  
static int
XOTclClassInfoSlotsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoSlotsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];

    return XOTclClassInfoSlotsMethod(interp, class);

  }
}
  
static int
XOTclClassInfoSubclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoSubclassMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    int  withClosure = (int )pc.clientData[1];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj * pattern = (Tcl_Obj *)pc.clientData[2];

    if (getMatchObject3(interp, pattern,  &pc, &patternObj, &patternString) == -1) {
      return TCL_OK;
    }
          
    return XOTclClassInfoSubclassMethod(interp, class, withClosure, patternString, patternObj);

  }
}
  
static int
XOTclClassInfoSuperclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclClassInfoSuperclassMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    int  withClosure = (int )pc.clientData[1];
    char * pattern = (char *)pc.clientData[2];

    return XOTclClassInfoSuperclassMethod(interp, class, withClosure, pattern);

  }
}
  
static int
XOTclObjInfoArgsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoArgsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclObjInfoArgsMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoBodyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoBodyMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclObjInfoBodyMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoCheckMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoCheckMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];

    return XOTclObjInfoCheckMethod(interp, object);

  }
}
  
static int
XOTclObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoChildrenMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclObjInfoChildrenMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoClassMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];

    return XOTclObjInfoClassMethod(interp, object);

  }
}
  
static int
XOTclObjInfoCommandsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoCommandsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclObjInfoCommandsMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoDefaultMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoDefaultMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];
    char * arg = (char *)pc.clientData[2];
    Tcl_Obj * var = (Tcl_Obj *)pc.clientData[3];

    return XOTclObjInfoDefaultMethod(interp, object, methodName, arg, var);

  }
}
  
static int
XOTclObjInfoFilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoFilterMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    int  withOrder = (int )pc.clientData[1];
    int  withGuards = (int )pc.clientData[2];
    char * pattern = (char *)pc.clientData[3];

    return XOTclObjInfoFilterMethod(interp, object, withOrder, withGuards, pattern);

  }
}
  
static int
XOTclObjInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoFilterguardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * filter = (char *)pc.clientData[1];

    return XOTclObjInfoFilterguardMethod(interp, object, filter);

  }
}
  
static int
XOTclObjInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoForwardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    int  withDefinition = (int )pc.clientData[1];
    char * methodName = (char *)pc.clientData[2];

    return XOTclObjInfoForwardMethod(interp, object, withDefinition, methodName);

  }
}
  
static int
XOTclObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, XOTclObjInfoHasnamespaceMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];

    return XOTclObjInfoHasnamespaceMethod(interp, object);

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
},
{"instnonposargs", XOTclClassInfoInstnonposargsMethodStub, {
  {"class", 1, 0, "class"},
  {"methodName", 1, 0, NULL}}
},
{"instparametercmd", XOTclClassInfoInstparametercmdMethodStub, {
  {"class", 1, 0, "class"},
  {"pattern", 0, 0, NULL}}
},
{"instpost", XOTclClassInfoInstpostMethodStub, {
  {"class", 1, 0, "class"},
  {"methodName", 1, 0, NULL}}
},
{"instpre", XOTclClassInfoInstpreMethodStub, {
  {"class", 1, 0, "class"},
  {"methodName", 1, 0, NULL}}
},
{"instprocs", XOTclClassInfoInstprocsMethodStub, {
  {"class", 1, 0, "class"},
  {"pattern", 0, 0, NULL}}
},
{"mixinof", XOTclClassInfoMixinofMethodStub, {
  {"class", 1, 0, "class"},
  {"-closure", 0, 0, NULL},
  {"pattern", 0, 0, "objpattern"}}
},
{"parameter", XOTclClassInfoParameterMethodStub, {
  {"class", 1, 0, "class"}}
},
{"slots", XOTclClassInfoSlotsMethodStub, {
  {"class", 1, 0, "class"}}
},
{"subclass", XOTclClassInfoSubclassMethodStub, {
  {"class", 1, 0, "class"},
  {"-closure", 0, 0, NULL},
  {"pattern", 0, 0, "objpattern"}}
},
{"superclass", XOTclClassInfoSuperclassMethodStub, {
  {"class", 1, 0, "class"},
  {"-closure", 0, 0, NULL},
  {"pattern", 0, 0, NULL}}
},
{"args", XOTclObjInfoArgsMethodStub, {
  {"object", 1, 0, "object"},
  {"methodName", 1, 0, NULL}}
},
{"body", XOTclObjInfoBodyMethodStub, {
  {"object", 1, 0, "object"},
  {"methodName", 1, 0, NULL}}
},
{"check", XOTclObjInfoCheckMethodStub, {
  {"object", 1, 0, "object"}}
},
{"children", XOTclObjInfoChildrenMethodStub, {
  {"object", 1, 0, "object"},
  {"pattern", 0, 0, NULL}}
},
{"class", XOTclObjInfoClassMethodStub, {
  {"object", 1, 0, "object"}}
},
{"commands", XOTclObjInfoCommandsMethodStub, {
  {"object", 1, 0, "object"},
  {"pattern", 0, 0, NULL}}
},
{"default", XOTclObjInfoDefaultMethodStub, {
  {"object", 1, 0, "object"},
  {"methodName", 1, 0, NULL},
  {"arg", 1, 0, NULL},
  {"var", 1, 0, "tclobj"}}
},
{"filter", XOTclObjInfoFilterMethodStub, {
  {"object", 1, 0, "object"},
  {"-order", 0, 0, NULL},
  {"-guards", 0, 0, NULL},
  {"pattern", 0, 0, NULL}}
},
{"filterguard", XOTclObjInfoFilterguardMethodStub, {
  {"object", 1, 0, "object"},
  {"filter", 1, 0, NULL}}
},
{"forward", XOTclObjInfoForwardMethodStub, {
  {"object", 1, 0, "object"},
  {"-definition", 0, 0, NULL},
  {"methodName", 1, 0, NULL}}
},
{"hasnamespace", XOTclObjInfoHasnamespaceMethodStub, {
  {"object", 1, 0, "object"}}
}
};

