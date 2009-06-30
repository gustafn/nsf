
typedef struct {
  char *methodName;
  Tcl_ObjCmdProc *proc;
  CONST interfaceDefinition ifd;
} methodDefinition2;

static int parse2(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
                  int idx, parseContext *pc);

static int getMatchObject3(Tcl_Interp *interp, Tcl_Obj *patternObj,  parseContext *pc,
                           XOTclObject **matchObject, char **pattern);
  
static int XOTclCheckBooleanArgsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCheckRequiredArgsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCAllocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCCreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCDeallocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCInstFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCInstForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCInstMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCInstParametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCInstProcMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCInstProcMethodCStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCInvariantsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCNewMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCRecreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCUnknownMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
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
static int XOTclObjInfoInvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoNonposargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoParametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoParentMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoPostMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoPreMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoPrecedenceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoProcsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoSlotObjectsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoVarsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOAutonameMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOCheckMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOCleanupMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOConfigureMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclODestroyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOExistsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOFilterSearchMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOInstVarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOInvariantsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOIsClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOIsMetaClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOIsMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOIsObjectMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOIsTypeMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclONextMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclONoinitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOParametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOProcMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOProcSearchMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclORequireNamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOSetMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOSetvaluesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOUplevelMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOUpvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOVolatileMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOVwaitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

static int XOTclCheckBooleanArgs(Tcl_Interp *interp, char *name, Tcl_Obj *value);
static int XOTclCheckRequiredArgs(Tcl_Interp *interp, char *name, Tcl_Obj *value);
static int XOTclCAllocMethod(Tcl_Interp *interp, XOTclClass *cl, char *name);
static int XOTclCCreateMethod(Tcl_Interp *interp, XOTclClass *cl, char *name, int objc, Tcl_Obj *CONST objv[]);
static int XOTclCDeallocMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *object);
static int XOTclCInstFilterGuardMethod(Tcl_Interp *interp, XOTclClass *cl, char *filter, Tcl_Obj *guard);
static int XOTclCInstForwardMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *method, Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix, int withObjscope, Tcl_Obj *withOnerror, int withVerbose, Tcl_Obj *target, int nobjc, Tcl_Obj *CONST nobjv[]);
static int XOTclCInstMixinGuardMethod(Tcl_Interp *interp, XOTclClass *cl, char *mixin, Tcl_Obj *guard);
static int XOTclCInstParametercmdMethod(Tcl_Interp *interp, XOTclClass *cl, char *name);
static int XOTclCInstProcMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *name, Tcl_Obj *args, Tcl_Obj *body, Tcl_Obj *precondition, Tcl_Obj *postcondition);
static int XOTclCInstProcMethodC(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *name, Tcl_Obj *args, Tcl_Obj *body, Tcl_Obj *precondition, Tcl_Obj *postcondition);
static int XOTclCInvariantsMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *invariantlist);
static int XOTclCNewMethod(Tcl_Interp *interp, XOTclClass *cl, XOTclObject *withChildof, int nobjc, Tcl_Obj *CONST nobjv[]);
static int XOTclCRecreateMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *name, int objc, Tcl_Obj *CONST objv[]);
static int XOTclCUnknownMethod(Tcl_Interp *interp, XOTclClass *cl, char *name, int objc, Tcl_Obj *CONST objv[]);
static int XOTclClassInfoHeritageMethod(Tcl_Interp *interp, XOTclClass *class, char *pattern);
static int XOTclClassInfoInstancesMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoInstargsMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName);
static int XOTclClassInfoInstbodyMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName);
static int XOTclClassInfoInstcommandsMethod(Tcl_Interp *interp, XOTclClass *class, char *pattern);
static int XOTclClassInfoInstdefaultMethod(Tcl_Interp *interp, XOTclClass *class, char *methodName, char *arg, Tcl_Obj *var);
static int XOTclClassInfoInstfilterMethod(Tcl_Interp *interp, XOTclClass *class, int withGuards, char *pattern);
static int XOTclClassInfoInstfilterguardMethod(Tcl_Interp *interp, XOTclClass *class, char *filter);
static int XOTclClassInfoInstforwardMethod(Tcl_Interp *interp, XOTclClass *class, int withDefinition, char *pattern);
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
static int XOTclObjInfoForwardMethod(Tcl_Interp *interp, XOTclObject *object, int withDefinition, char *pattern);
static int XOTclObjInfoHasnamespaceMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclObjInfoInvarMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclObjInfoMethodsMethod(Tcl_Interp *interp, XOTclObject *object, int withNoprocs, int withNocmds, int withNomixins, int withIncontext, char *pattern);
static int XOTclObjInfoMixinMethod(Tcl_Interp *interp, XOTclObject *object, int withGuards, int withOrder, char *patternString, XOTclObject *patternObj);
static int XOTclObjInfoMixinguardMethod(Tcl_Interp *interp, XOTclObject *object, char *mixin);
static int XOTclObjInfoNonposargsMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName);
static int XOTclObjInfoParametercmdMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern);
static int XOTclObjInfoParentMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclObjInfoPostMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName);
static int XOTclObjInfoPreMethod(Tcl_Interp *interp, XOTclObject *object, char *methodName);
static int XOTclObjInfoPrecedenceMethod(Tcl_Interp *interp, XOTclObject *object, int withIntrinsic, char *pattern);
static int XOTclObjInfoProcsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern);
static int XOTclObjInfoSlotObjectsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern);
static int XOTclObjInfoVarsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern);
static int XOTclOAutonameMethod(Tcl_Interp *interp, XOTclObject *obj, int withInstance, int withReset, Tcl_Obj *name);
static int XOTclOCheckMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *flag);
static int XOTclOCleanupMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclOConfigureMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclODestroyMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclOExistsMethod(Tcl_Interp *interp, XOTclObject *obj, char *var);
static int XOTclOFilterGuardMethod(Tcl_Interp *interp, XOTclObject *obj, char *filter, Tcl_Obj *guard);
static int XOTclOFilterSearchMethod(Tcl_Interp *interp, XOTclObject *obj, char *filter);
static int XOTclOForwardMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *method, Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix, int withObjscope, Tcl_Obj *withOnerror, int withVerbose, Tcl_Obj *target, int nobjc, Tcl_Obj *CONST nobjv[]);
static int XOTclOInstVarMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclOInvariantsMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *invariantlist);
static int XOTclOIsClassMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *class);
static int XOTclOIsMetaClassMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *metaclass);
static int XOTclOIsMixinMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *class);
static int XOTclOIsObjectMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *object);
static int XOTclOIsTypeMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *class);
static int XOTclOMixinGuardMethod(Tcl_Interp *interp, XOTclObject *obj, char *mixin, Tcl_Obj *guard);
static int XOTclONextMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclONoinitMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclOParametercmdMethod(Tcl_Interp *interp, XOTclObject *obj, char *name);
static int XOTclOProcMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *name, Tcl_Obj *args, Tcl_Obj *body, Tcl_Obj *precondition, Tcl_Obj *postcondition);
static int XOTclOProcSearchMethod(Tcl_Interp *interp, XOTclObject *obj, char *name);
static int XOTclORequireNamespaceMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclOSetMethod(Tcl_Interp *interp, XOTclObject *obj, Tcl_Obj *var, Tcl_Obj *value);
static int XOTclOSetvaluesMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclOUplevelMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclOUpvarMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclOVolatileMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclOVwaitMethod(Tcl_Interp *interp, XOTclObject *obj, char *varname);

enum {
 XOTclCheckBooleanArgsIdx,
 XOTclCheckRequiredArgsIdx,
 XOTclCAllocMethodIdx,
 XOTclCCreateMethodIdx,
 XOTclCDeallocMethodIdx,
 XOTclCInstFilterGuardMethodIdx,
 XOTclCInstForwardMethodIdx,
 XOTclCInstMixinGuardMethodIdx,
 XOTclCInstParametercmdMethodIdx,
 XOTclCInstProcMethodIdx,
 XOTclCInstProcMethodCIdx,
 XOTclCInvariantsMethodIdx,
 XOTclCNewMethodIdx,
 XOTclCRecreateMethodIdx,
 XOTclCUnknownMethodIdx,
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
 XOTclObjInfoHasnamespaceMethodIdx,
 XOTclObjInfoInvarMethodIdx,
 XOTclObjInfoMethodsMethodIdx,
 XOTclObjInfoMixinMethodIdx,
 XOTclObjInfoMixinguardMethodIdx,
 XOTclObjInfoNonposargsMethodIdx,
 XOTclObjInfoParametercmdMethodIdx,
 XOTclObjInfoParentMethodIdx,
 XOTclObjInfoPostMethodIdx,
 XOTclObjInfoPreMethodIdx,
 XOTclObjInfoPrecedenceMethodIdx,
 XOTclObjInfoProcsMethodIdx,
 XOTclObjInfoSlotObjectsMethodIdx,
 XOTclObjInfoVarsMethodIdx,
 XOTclOAutonameMethodIdx,
 XOTclOCheckMethodIdx,
 XOTclOCleanupMethodIdx,
 XOTclOConfigureMethodIdx,
 XOTclODestroyMethodIdx,
 XOTclOExistsMethodIdx,
 XOTclOFilterGuardMethodIdx,
 XOTclOFilterSearchMethodIdx,
 XOTclOForwardMethodIdx,
 XOTclOInstVarMethodIdx,
 XOTclOInvariantsMethodIdx,
 XOTclOIsClassMethodIdx,
 XOTclOIsMetaClassMethodIdx,
 XOTclOIsMixinMethodIdx,
 XOTclOIsObjectMethodIdx,
 XOTclOIsTypeMethodIdx,
 XOTclOMixinGuardMethodIdx,
 XOTclONextMethodIdx,
 XOTclONoinitMethodIdx,
 XOTclOParametercmdMethodIdx,
 XOTclOProcMethodIdx,
 XOTclOProcSearchMethodIdx,
 XOTclORequireNamespaceMethodIdx,
 XOTclOSetMethodIdx,
 XOTclOSetvaluesMethodIdx,
 XOTclOUplevelMethodIdx,
 XOTclOUpvarMethodIdx,
 XOTclOVolatileMethodIdx,
 XOTclOVwaitMethodIdx
} XOTclMethods;


static int
XOTclCheckBooleanArgsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclCheckBooleanArgsIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * name = (char *)pc.clientData[0];
    Tcl_Obj * value = (Tcl_Obj *)pc.clientData[1];

    return XOTclCheckBooleanArgs(interp, name, value);

  }
}
  
static int
XOTclCheckRequiredArgsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclCheckRequiredArgsIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * name = (char *)pc.clientData[0];
    Tcl_Obj * value = (Tcl_Obj *)pc.clientData[1];

    return XOTclCheckRequiredArgs(interp, name, value);

  }
}
  
static int
XOTclCAllocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCAllocMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * name = (char *)pc.clientData[0];

    return XOTclCAllocMethod(interp, cl, name);

  }
}
  
static int
XOTclCCreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCCreateMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * name = (char *)pc.clientData[0];

    return XOTclCCreateMethod(interp, cl, name, objc, objv);

  }
}
  
static int
XOTclCDeallocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCDeallocMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * object = (Tcl_Obj *)pc.clientData[0];

    return XOTclCDeallocMethod(interp, cl, object);

  }
}
  
static int
XOTclCInstFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCInstFilterGuardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * filter = (char *)pc.clientData[0];
    Tcl_Obj * guard = (Tcl_Obj *)pc.clientData[1];

    return XOTclCInstFilterGuardMethod(interp, cl, filter, guard);

  }
}
  
static int
XOTclCInstForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCInstForwardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * method = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj * withDefault = (Tcl_Obj *)pc.clientData[1];
    int  withEarlybinding = (int )pc.clientData[2];
    Tcl_Obj * withMethodprefix = (Tcl_Obj *)pc.clientData[3];
    int  withObjscope = (int )pc.clientData[4];
    Tcl_Obj * withOnerror = (Tcl_Obj *)pc.clientData[5];
    int  withVerbose = (int )pc.clientData[6];
    Tcl_Obj * target = (Tcl_Obj *)pc.clientData[7];

    return XOTclCInstForwardMethod(interp, cl, method, withDefault, withEarlybinding, withMethodprefix, withObjscope, withOnerror, withVerbose, target, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}
  
static int
XOTclCInstMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCInstMixinGuardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * mixin = (char *)pc.clientData[0];
    Tcl_Obj * guard = (Tcl_Obj *)pc.clientData[1];

    return XOTclCInstMixinGuardMethod(interp, cl, mixin, guard);

  }
}
  
static int
XOTclCInstParametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCInstParametercmdMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * name = (char *)pc.clientData[0];

    return XOTclCInstParametercmdMethod(interp, cl, name);

  }
}
  
static int
XOTclCInstProcMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCInstProcMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * name = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj * args = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj * body = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj * precondition = (Tcl_Obj *)pc.clientData[3];
    Tcl_Obj * postcondition = (Tcl_Obj *)pc.clientData[4];

    return XOTclCInstProcMethod(interp, cl, name, args, body, precondition, postcondition);

  }
}
  
static int
XOTclCInstProcMethodCStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCInstProcMethodCIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * name = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj * args = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj * body = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj * precondition = (Tcl_Obj *)pc.clientData[3];
    Tcl_Obj * postcondition = (Tcl_Obj *)pc.clientData[4];

    return XOTclCInstProcMethodC(interp, cl, name, args, body, precondition, postcondition);

  }
}
  
static int
XOTclCInvariantsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCInvariantsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * invariantlist = (Tcl_Obj *)pc.clientData[0];

    return XOTclCInvariantsMethod(interp, cl, invariantlist);

  }
}
  
static int
XOTclCNewMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCNewMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * withChildof = (XOTclObject *)pc.clientData[0];

    return XOTclCNewMethod(interp, cl, withChildof, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}
  
static int
XOTclCRecreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCRecreateMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * name = (Tcl_Obj *)pc.clientData[0];

    return XOTclCRecreateMethod(interp, cl, name, objc, objv);

  }
}
  
static int
XOTclCUnknownMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parse2(interp, objc, objv, XOTclCUnknownMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * name = (char *)pc.clientData[0];

    return XOTclCUnknownMethod(interp, cl, name, objc, objv);

  }
}
  
static int
XOTclClassInfoHeritageMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclClassInfoHeritageMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstancesMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstargsMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstbodyMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstcommandsMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstdefaultMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstfilterMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstfilterguardMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstforwardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];
    int  withDefinition = (int )pc.clientData[1];
    char * pattern = (char *)pc.clientData[2];

    return XOTclClassInfoInstforwardMethod(interp, class, withDefinition, pattern);

  }
}
  
static int
XOTclClassInfoInstinvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclClassInfoInstinvarMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];

    return XOTclClassInfoInstinvarMethod(interp, class);

  }
}
  
static int
XOTclClassInfoInstmixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclClassInfoInstmixinMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstmixinguardMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstmixinofMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstnonposargsMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstparametercmdMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstpostMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstpreMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoInstprocsMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoMixinofMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoParameterMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];

    return XOTclClassInfoParameterMethod(interp, class);

  }
}
  
static int
XOTclClassInfoSlotsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclClassInfoSlotsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass * class = (XOTclClass *)pc.clientData[0];

    return XOTclClassInfoSlotsMethod(interp, class);

  }
}
  
static int
XOTclClassInfoSubclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclClassInfoSubclassMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclClassInfoSuperclassMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclObjInfoArgsMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclObjInfoBodyMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclObjInfoCheckMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];

    return XOTclObjInfoCheckMethod(interp, object);

  }
}
  
static int
XOTclObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoChildrenMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclObjInfoClassMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];

    return XOTclObjInfoClassMethod(interp, object);

  }
}
  
static int
XOTclObjInfoCommandsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoCommandsMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclObjInfoDefaultMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclObjInfoFilterMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclObjInfoFilterguardMethodIdx, &pc) != TCL_OK) {
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

  if (parse2(interp, objc, objv, XOTclObjInfoForwardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    int  withDefinition = (int )pc.clientData[1];
    char * pattern = (char *)pc.clientData[2];

    return XOTclObjInfoForwardMethod(interp, object, withDefinition, pattern);

  }
}
  
static int
XOTclObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoHasnamespaceMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];

    return XOTclObjInfoHasnamespaceMethod(interp, object);

  }
}
  
static int
XOTclObjInfoInvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoInvarMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];

    return XOTclObjInfoInvarMethod(interp, object);

  }
}
  
static int
XOTclObjInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoMethodsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    int  withNoprocs = (int )pc.clientData[1];
    int  withNocmds = (int )pc.clientData[2];
    int  withNomixins = (int )pc.clientData[3];
    int  withIncontext = (int )pc.clientData[4];
    char * pattern = (char *)pc.clientData[5];

    return XOTclObjInfoMethodsMethod(interp, object, withNoprocs, withNocmds, withNomixins, withIncontext, pattern);

  }
}
  
static int
XOTclObjInfoMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoMixinMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    int  withGuards = (int )pc.clientData[1];
    int  withOrder = (int )pc.clientData[2];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj * pattern = (Tcl_Obj *)pc.clientData[3];

    if (getMatchObject3(interp, pattern,  &pc, &patternObj, &patternString) == -1) {
      return TCL_OK;
    }
          
    return XOTclObjInfoMixinMethod(interp, object, withGuards, withOrder, patternString, patternObj);

  }
}
  
static int
XOTclObjInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoMixinguardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * mixin = (char *)pc.clientData[1];

    return XOTclObjInfoMixinguardMethod(interp, object, mixin);

  }
}
  
static int
XOTclObjInfoNonposargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoNonposargsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclObjInfoNonposargsMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoParametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoParametercmdMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclObjInfoParametercmdMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoParentMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoParentMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];

    return XOTclObjInfoParentMethod(interp, object);

  }
}
  
static int
XOTclObjInfoPostMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoPostMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclObjInfoPostMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoPreMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoPreMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * methodName = (char *)pc.clientData[1];

    return XOTclObjInfoPreMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoPrecedenceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoPrecedenceMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    int  withIntrinsic = (int )pc.clientData[1];
    char * pattern = (char *)pc.clientData[2];

    return XOTclObjInfoPrecedenceMethod(interp, object, withIntrinsic, pattern);

  }
}
  
static int
XOTclObjInfoProcsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoProcsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclObjInfoProcsMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoSlotObjectsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoSlotObjectsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclObjInfoSlotObjectsMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoVarsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parse2(interp, objc, objv, XOTclObjInfoVarsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject * object = (XOTclObject *)pc.clientData[0];
    char * pattern = (char *)pc.clientData[1];

    return XOTclObjInfoVarsMethod(interp, object, pattern);

  }
}
  
static int
XOTclOAutonameMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOAutonameMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int  withInstance = (int )pc.clientData[0];
    int  withReset = (int )pc.clientData[1];
    Tcl_Obj * name = (Tcl_Obj *)pc.clientData[2];

    return XOTclOAutonameMethod(interp, obj, withInstance, withReset, name);

  }
}
  
static int
XOTclOCheckMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOCheckMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * flag = (Tcl_Obj *)pc.clientData[0];

    return XOTclOCheckMethod(interp, obj, flag);

  }
}
  
static int
XOTclOCleanupMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOCleanupMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclOCleanupMethod(interp, obj);

  }
}
  
static int
XOTclOConfigureMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOConfigureMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclOConfigureMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclODestroyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclODestroyMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclODestroyMethod(interp, obj);

  }
}
  
static int
XOTclOExistsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOExistsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * var = (char *)pc.clientData[0];

    return XOTclOExistsMethod(interp, obj, var);

  }
}
  
static int
XOTclOFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOFilterGuardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * filter = (char *)pc.clientData[0];
    Tcl_Obj * guard = (Tcl_Obj *)pc.clientData[1];

    return XOTclOFilterGuardMethod(interp, obj, filter, guard);

  }
}
  
static int
XOTclOFilterSearchMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOFilterSearchMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * filter = (char *)pc.clientData[0];

    return XOTclOFilterSearchMethod(interp, obj, filter);

  }
}
  
static int
XOTclOForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOForwardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * method = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj * withDefault = (Tcl_Obj *)pc.clientData[1];
    int  withEarlybinding = (int )pc.clientData[2];
    Tcl_Obj * withMethodprefix = (Tcl_Obj *)pc.clientData[3];
    int  withObjscope = (int )pc.clientData[4];
    Tcl_Obj * withOnerror = (Tcl_Obj *)pc.clientData[5];
    int  withVerbose = (int )pc.clientData[6];
    Tcl_Obj * target = (Tcl_Obj *)pc.clientData[7];

    return XOTclOForwardMethod(interp, obj, method, withDefault, withEarlybinding, withMethodprefix, withObjscope, withOnerror, withVerbose, target, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}
  
static int
XOTclOInstVarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOInstVarMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclOInstVarMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclOInvariantsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOInvariantsMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * invariantlist = (Tcl_Obj *)pc.clientData[0];

    return XOTclOInvariantsMethod(interp, obj, invariantlist);

  }
}
  
static int
XOTclOIsClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOIsClassMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * class = (Tcl_Obj *)pc.clientData[0];

    return XOTclOIsClassMethod(interp, obj, class);

  }
}
  
static int
XOTclOIsMetaClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOIsMetaClassMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * metaclass = (Tcl_Obj *)pc.clientData[0];

    return XOTclOIsMetaClassMethod(interp, obj, metaclass);

  }
}
  
static int
XOTclOIsMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOIsMixinMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * class = (Tcl_Obj *)pc.clientData[0];

    return XOTclOIsMixinMethod(interp, obj, class);

  }
}
  
static int
XOTclOIsObjectMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOIsObjectMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * object = (Tcl_Obj *)pc.clientData[0];

    return XOTclOIsObjectMethod(interp, obj, object);

  }
}
  
static int
XOTclOIsTypeMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOIsTypeMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * class = (Tcl_Obj *)pc.clientData[0];

    return XOTclOIsTypeMethod(interp, obj, class);

  }
}
  
static int
XOTclOMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOMixinGuardMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * mixin = (char *)pc.clientData[0];
    Tcl_Obj * guard = (Tcl_Obj *)pc.clientData[1];

    return XOTclOMixinGuardMethod(interp, obj, mixin, guard);

  }
}
  
static int
XOTclONextMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclONextMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclONextMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclONoinitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclONoinitMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclONoinitMethod(interp, obj);

  }
}
  
static int
XOTclOParametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOParametercmdMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * name = (char *)pc.clientData[0];

    return XOTclOParametercmdMethod(interp, obj, name);

  }
}
  
static int
XOTclOProcMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOProcMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * name = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj * args = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj * body = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj * precondition = (Tcl_Obj *)pc.clientData[3];
    Tcl_Obj * postcondition = (Tcl_Obj *)pc.clientData[4];

    return XOTclOProcMethod(interp, obj, name, args, body, precondition, postcondition);

  }
}
  
static int
XOTclOProcSearchMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOProcSearchMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * name = (char *)pc.clientData[0];

    return XOTclOProcSearchMethod(interp, obj, name);

  }
}
  
static int
XOTclORequireNamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclORequireNamespaceMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclORequireNamespaceMethod(interp, obj);

  }
}
  
static int
XOTclOSetMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOSetMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj * var = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj * value = (Tcl_Obj *)pc.clientData[1];

    return XOTclOSetMethod(interp, obj, var, value);

  }
}
  
static int
XOTclOSetvaluesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOSetvaluesMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclOSetvaluesMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclOUplevelMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOUplevelMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclOUplevelMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclOUpvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOUpvarMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclOUpvarMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclOVolatileMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOVolatileMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    return XOTclOVolatileMethod(interp, obj);

  }
}
  
static int
XOTclOVwaitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parse2(interp, objc, objv, XOTclOVwaitMethodIdx, &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char * varname = (char *)pc.clientData[0];

    return XOTclOVwaitMethod(interp, obj, varname);

  }
}
  
static methodDefinition2 methodDefinitons[] = {
{"type=boolean", XOTclCheckBooleanArgsStub, {
  {"name", 1, 0, NULL},
  {"value", 0, 0, "tclobj"}}
},
{"type=required", XOTclCheckRequiredArgsStub, {
  {"name", 1, 0, NULL},
  {"value", 0, 0, "tclobj"}}
},
{"alloc", XOTclCAllocMethodStub, {
  {"name", 1, 0, NULL}}
},
{"create", XOTclCCreateMethodStub, {
  {"name", 1, 0, NULL},
  {"args", 0, 0, "allargs"}}
},
{"dealloc", XOTclCDeallocMethodStub, {
  {"object", 1, 0, "tclobj"}}
},
{"instfilterguard", XOTclCInstFilterGuardMethodStub, {
  {"filter", 1, 0, NULL},
  {"guard", 1, 0, "tclobj"}}
},
{"instforward", XOTclCInstForwardMethodStub, {
  {"method", 1, 0, "tclobj"},
  {"-default", 0, 1, "tclobj"},
  {"-earlybinding", 0, 0, NULL},
  {"-methodprefix", 0, 1, "tclobj"},
  {"-objscope", 0, 0, NULL},
  {"-onerror", 0, 1, "tclobj"},
  {"-verbose", 0, 0, NULL},
  {"target", 0, 0, "tclobj"},
  {"args", 0, 0, "args"}}
},
{"instmixinguard", XOTclCInstMixinGuardMethodStub, {
  {"mixin", 1, 0, NULL},
  {"guard", 1, 0, "tclobj"}}
},
{"instparametercmd", XOTclCInstParametercmdMethodStub, {
  {"name", 1, 0, NULL}}
},
{"instproc", XOTclCInstProcMethodStub, {
  {"name", 1, 0, "tclobj"},
  {"args", 1, 0, "tclobj"},
  {"body", 1, 0, "tclobj"},
  {"precondition", 0, 0, "tclobj"},
  {"postcondition", 0, 0, "tclobj"}}
},
{"classscopedinstproc", XOTclCInstProcMethodCStub, {
  {"name", 1, 0, "tclobj"},
  {"args", 1, 0, "tclobj"},
  {"body", 1, 0, "tclobj"},
  {"precondition", 0, 0, "tclobj"},
  {"postcondition", 0, 0, "tclobj"}}
},
{"instinvar", XOTclCInvariantsMethodStub, {
  {"invariantlist", 1, 0, "tclobj"}}
},
{"new", XOTclCNewMethodStub, {
  {"-childof", 0, 1, "object"},
  {"args", 0, 0, "args"}}
},
{"recreate", XOTclCRecreateMethodStub, {
  {"name", 1, 0, "tclobj"},
  {"args", 0, 0, "allargs"}}
},
{"unknown", XOTclCUnknownMethodStub, {
  {"name", 1, 0, NULL},
  {"args", 0, 0, "allargs"}}
},
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
  {"pattern", 0, 0, NULL}}
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
  {"pattern", 0, 0, NULL}}
},
{"hasnamespace", XOTclObjInfoHasnamespaceMethodStub, {
  {"object", 1, 0, "object"}}
},
{"invar", XOTclObjInfoInvarMethodStub, {
  {"object", 1, 0, "object"}}
},
{"methods", XOTclObjInfoMethodsMethodStub, {
  {"object", 1, 0, "object"},
  {"-noprocs", 0, 0, NULL},
  {"-nocmds", 0, 0, NULL},
  {"-nomixins", 0, 0, NULL},
  {"-incontext", 0, 0, NULL},
  {"pattern", 0, 0, NULL}}
},
{"mixin", XOTclObjInfoMixinMethodStub, {
  {"object", 1, 0, "object"},
  {"-guards", 0, 0, NULL},
  {"-order", 0, 0, NULL},
  {"pattern", 0, 0, "objpattern"}}
},
{"mixinguard", XOTclObjInfoMixinguardMethodStub, {
  {"object", 1, 0, "object"},
  {"mixin", 1, 0, NULL}}
},
{"nonposargs", XOTclObjInfoNonposargsMethodStub, {
  {"object", 1, 0, "object"},
  {"methodName", 1, 0, NULL}}
},
{"parametercmd", XOTclObjInfoParametercmdMethodStub, {
  {"object", 1, 0, "object"},
  {"pattern", 0, 0, NULL}}
},
{"parent", XOTclObjInfoParentMethodStub, {
  {"object", 1, 0, "object"}}
},
{"post", XOTclObjInfoPostMethodStub, {
  {"object", 1, 0, "object"},
  {"methodName", 1, 0, NULL}}
},
{"pre", XOTclObjInfoPreMethodStub, {
  {"object", 1, 0, "object"},
  {"methodName", 1, 0, NULL}}
},
{"precedence", XOTclObjInfoPrecedenceMethodStub, {
  {"object", 1, 0, "object"},
  {"-intrinsic", 0, 0, NULL},
  {"pattern", 0, 0, NULL}}
},
{"procs", XOTclObjInfoProcsMethodStub, {
  {"object", 1, 0, "object"},
  {"pattern", 0, 0, NULL}}
},
{"slotobjects", XOTclObjInfoSlotObjectsMethodStub, {
  {"object", 1, 0, "object"},
  {"pattern", 0, 0, NULL}}
},
{"vars", XOTclObjInfoVarsMethodStub, {
  {"object", 1, 0, "object"},
  {"pattern", 0, 0, NULL}}
},
{"autoname", XOTclOAutonameMethodStub, {
  {"-instance", 0, 0, NULL},
  {"-reset", 0, 0, NULL},
  {"name", 1, 0, "tclobj"}}
},
{"check", XOTclOCheckMethodStub, {
  {"flag", 1, 0, "tclobj"}}
},
{"cleanup", XOTclOCleanupMethodStub, {
  }
},
{"configure", XOTclOConfigureMethodStub, {
  {"args", 0, 0, "allargs"}}
},
{"destroy", XOTclODestroyMethodStub, {
  }
},
{"exists", XOTclOExistsMethodStub, {
  {"var", 1, 0, NULL}}
},
{"filterguard", XOTclOFilterGuardMethodStub, {
  {"filter", 1, 0, NULL},
  {"guard", 1, 0, "tclobj"}}
},
{"filtersearch", XOTclOFilterSearchMethodStub, {
  {"filter", 1, 0, NULL}}
},
{"forward", XOTclOForwardMethodStub, {
  {"method", 1, 0, "tclobj"},
  {"-default", 0, 1, "tclobj"},
  {"-earlybinding", 0, 0, NULL},
  {"-methodprefix", 0, 1, "tclobj"},
  {"-objscope", 0, 0, NULL},
  {"-onerror", 0, 1, "tclobj"},
  {"-verbose", 0, 0, NULL},
  {"target", 0, 0, "tclobj"},
  {"args", 0, 0, "args"}}
},
{"instvar", XOTclOInstVarMethodStub, {
  {"args", 0, 0, "allargs"}}
},
{"invar", XOTclOInvariantsMethodStub, {
  {"invariantlist", 1, 0, "tclobj"}}
},
{"isclass", XOTclOIsClassMethodStub, {
  {"class", 0, 0, "tclobj"}}
},
{"ismetaclass", XOTclOIsMetaClassMethodStub, {
  {"metaclass", 0, 0, "tclobj"}}
},
{"ismixin", XOTclOIsMixinMethodStub, {
  {"class", 1, 0, "tclobj"}}
},
{"isobject", XOTclOIsObjectMethodStub, {
  {"object", 1, 0, "tclobj"}}
},
{"istype", XOTclOIsTypeMethodStub, {
  {"class", 1, 0, "tclobj"}}
},
{"mixinguard", XOTclOMixinGuardMethodStub, {
  {"mixin", 1, 0, NULL},
  {"guard", 1, 0, "tclobj"}}
},
{"__next", XOTclONextMethodStub, {
  {"args", 0, 0, "allargs"}}
},
{"noinit", XOTclONoinitMethodStub, {
  }
},
{"parametercmd", XOTclOParametercmdMethodStub, {
  {"name", 1, 0, NULL}}
},
{"proc", XOTclOProcMethodStub, {
  {"name", 1, 0, "tclobj"},
  {"args", 1, 0, "tclobj"},
  {"body", 1, 0, "tclobj"},
  {"precondition", 0, 0, "tclobj"},
  {"postcondition", 0, 0, "tclobj"}}
},
{"procsearch", XOTclOProcSearchMethodStub, {
  {"name", 1, 0, NULL}}
},
{"requireNamespace", XOTclORequireNamespaceMethodStub, {
  }
},
{"set", XOTclOSetMethodStub, {
  {"var", 1, 0, "tclobj"},
  {"value", 0, 0, "tclobj"}}
},
{"setvalues", XOTclOSetvaluesMethodStub, {
  {"args", 0, 0, "allargs"}}
},
{"uplevel", XOTclOUplevelMethodStub, {
  {"args", 0, 0, "allargs"}}
},
{"upvar", XOTclOUpvarMethodStub, {
  {"args", 0, 0, "allargs"}}
},
{"volatile", XOTclOVolatileMethodStub, {
  }
},
{"vwait", XOTclOVwaitMethodStub, {
  {"varname", 1, 0, NULL}}
}
};

