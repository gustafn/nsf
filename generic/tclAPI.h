
static int convertToConfigureoption(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  static CONST char *opts[] = {"filter", "softrecreate", NULL};
  return Tcl_GetIndexFromObj(interp, objPtr, opts, "configureoption", 0, (int *)clientData);
}
enum configureoptionIdx {configureoptionFilterIdx, configureoptionSoftrecreateIdx};
  
static int convertToMethodproperty(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  static CONST char *opts[] = {"protected", "public", "slotobj", NULL};
  return Tcl_GetIndexFromObj(interp, objPtr, opts, "methodproperty", 0, (int *)clientData);
}
enum methodpropertyIdx {methodpropertyProtectedIdx, methodpropertyPublicIdx, methodpropertySlotobjIdx};
  
static int convertToRelationtype(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  static CONST char *opts[] = {"mixin", "instmixin", "object-mixin", "class-mixin", "filter", "instfilter", "object-filter", "class_filter", "class", "superclass", "rootclass", NULL};
  return Tcl_GetIndexFromObj(interp, objPtr, opts, "relationtype", 0, (int *)clientData);
}
enum relationtypeIdx {relationtypeMixinIdx, relationtypeInstmixinIdx, relationtypeObject_mixinIdx, relationtypeClass_mixinIdx, relationtypeFilterIdx, relationtypeInstfilterIdx, relationtypeObject_filterIdx, relationtypeClass_filterIdx, relationtypeClassIdx, relationtypeSuperclassIdx, relationtypeRootclassIdx};
  

typedef struct {
  char *methodName;
  Tcl_ObjCmdProc *proc;
  /*CONST interfaceDefinition ifd;*/
  int ifdSize;
  argDefinition ifd[10];
} methodDefinition;

static int parseObjv(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], Tcl_Obj *procName,
		     argDefinition CONST *ifdPtr, int ifdSize, parseContext *pc);

static int getMatchObject(Tcl_Interp *interp, Tcl_Obj *patternObj, Tcl_Obj *origObj,
			  XOTclObject **matchObject, char **pattern);

/* just to define the symbol */
static methodDefinition method_definitions[];
  
char *method_command_namespace_names[] = {
  "::xotcl::cmd::ObjectInfo",
  "::xotcl::cmd::Object",
  "::xotcl::cmd::ClassInfo",
  "::xotcl::cmd::NonposArgs",
  "::xotcl::cmd::Class"
};
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
static int XOTclAliasCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclConfigureCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclMethodPropertyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclMyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclRelationCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclSetInstvarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

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
static int XOTclClassInfoSuperclassMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, Tcl_Obj *pattern);
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
static int XOTclAliasCmd(Tcl_Interp *interp, XOTclObject *object, char *methodName, int withObjscope, int withPer_object, int withProtected, Tcl_Obj *cmdName);
static int XOTclConfigureCmd(Tcl_Interp *interp, int configureoption, Tcl_Obj *value);
static int XOTclMethodPropertyCmd(Tcl_Interp *interp, XOTclObject *object, char *methodName, int withPer_object, int methodproperty, Tcl_Obj *value);
static int XOTclMyCmd(Tcl_Interp *interp, int withLocal, Tcl_Obj *method, int nobjc, Tcl_Obj *CONST nobjv[]);
static int XOTclRelationCmd(Tcl_Interp *interp, XOTclObject *object, int relationtype, Tcl_Obj *value);
static int XOTclSetInstvarCmd(Tcl_Interp *interp, XOTclObject *object, Tcl_Obj *variable, Tcl_Obj *value);

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
 XOTclOVwaitMethodIdx,
 XOTclAliasCmdIdx,
 XOTclConfigureCmdIdx,
 XOTclMethodPropertyCmdIdx,
 XOTclMyCmdIdx,
 XOTclRelationCmdIdx,
 XOTclSetInstvarCmdIdx
} XOTclMethods;


static int
XOTclCheckBooleanArgsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCheckBooleanArgsIdx].ifd, 
		method_definitions[XOTclCheckBooleanArgsIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *name = (char *)pc.clientData[0];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclCheckBooleanArgs(interp, name, value);

  }
}
  
static int
XOTclCheckRequiredArgsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCheckRequiredArgsIdx].ifd, 
		method_definitions[XOTclCheckRequiredArgsIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *name = (char *)pc.clientData[0];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclCheckRequiredArgs(interp, name, value);

  }
}
  
static int
XOTclCAllocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCAllocMethodIdx].ifd, 
		method_definitions[XOTclCAllocMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *name = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCAllocMethod(interp, cl, name);

  }
}
  
static int
XOTclCCreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCCreateMethodIdx].ifd, 
		method_definitions[XOTclCCreateMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *name = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCCreateMethod(interp, cl, name, objc, objv);

  }
}
  
static int
XOTclCDeallocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCDeallocMethodIdx].ifd, 
		method_definitions[XOTclCDeallocMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *object = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCDeallocMethod(interp, cl, object);

  }
}
  
static int
XOTclCInstFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCInstFilterGuardMethodIdx].ifd, 
		method_definitions[XOTclCInstFilterGuardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *filter = (char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclCInstFilterGuardMethod(interp, cl, filter, guard);

  }
}
  
static int
XOTclCInstForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCInstForwardMethodIdx].ifd, 
		method_definitions[XOTclCInstForwardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *method = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *withDefault = (Tcl_Obj *)pc.clientData[1];
    int withEarlybinding = (int )pc.clientData[2];
    Tcl_Obj *withMethodprefix = (Tcl_Obj *)pc.clientData[3];
    int withObjscope = (int )pc.clientData[4];
    Tcl_Obj *withOnerror = (Tcl_Obj *)pc.clientData[5];
    int withVerbose = (int )pc.clientData[6];
    Tcl_Obj *target = (Tcl_Obj *)pc.clientData[7];

    parseContextRelease(&pc);
    return XOTclCInstForwardMethod(interp, cl, method, withDefault, withEarlybinding, withMethodprefix, withObjscope, withOnerror, withVerbose, target, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}
  
static int
XOTclCInstMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCInstMixinGuardMethodIdx].ifd, 
		method_definitions[XOTclCInstMixinGuardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *mixin = (char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclCInstMixinGuardMethod(interp, cl, mixin, guard);

  }
}
  
static int
XOTclCInstParametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCInstParametercmdMethodIdx].ifd, 
		method_definitions[XOTclCInstParametercmdMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *name = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCInstParametercmdMethod(interp, cl, name);

  }
}
  
static int
XOTclCInstProcMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCInstProcMethodIdx].ifd, 
		method_definitions[XOTclCInstProcMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *args = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *body = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj *precondition = (Tcl_Obj *)pc.clientData[3];
    Tcl_Obj *postcondition = (Tcl_Obj *)pc.clientData[4];

    parseContextRelease(&pc);
    return XOTclCInstProcMethod(interp, cl, name, args, body, precondition, postcondition);

  }
}
  
static int
XOTclCInstProcMethodCStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCInstProcMethodCIdx].ifd, 
		method_definitions[XOTclCInstProcMethodCIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *args = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *body = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj *precondition = (Tcl_Obj *)pc.clientData[3];
    Tcl_Obj *postcondition = (Tcl_Obj *)pc.clientData[4];

    parseContextRelease(&pc);
    return XOTclCInstProcMethodC(interp, cl, name, args, body, precondition, postcondition);

  }
}
  
static int
XOTclCInvariantsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCInvariantsMethodIdx].ifd, 
		method_definitions[XOTclCInvariantsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *invariantlist = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCInvariantsMethod(interp, cl, invariantlist);

  }
}
  
static int
XOTclCNewMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCNewMethodIdx].ifd, 
		method_definitions[XOTclCNewMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *withChildof = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCNewMethod(interp, cl, withChildof, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}
  
static int
XOTclCRecreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCRecreateMethodIdx].ifd, 
		method_definitions[XOTclCRecreateMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCRecreateMethod(interp, cl, name, objc, objv);

  }
}
  
static int
XOTclCUnknownMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclCUnknownMethodIdx].ifd, 
		method_definitions[XOTclCUnknownMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *name = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCUnknownMethod(interp, cl, name, objc, objv);

  }
}
  
static int
XOTclClassInfoHeritageMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoHeritageMethodIdx].ifd, 
		method_definitions[XOTclClassInfoHeritageMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoHeritageMethod(interp, class, pattern);

  }
}
  
static int
XOTclClassInfoInstancesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstancesMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstancesMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )pc.clientData[1];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[2];
    int returnCode;

    if (getMatchObject(interp, pattern,  objv[2], &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = XOTclClassInfoInstancesMethod(interp, class, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}
  
static int
XOTclClassInfoInstargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstargsMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstargsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstargsMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstbodyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstbodyMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstbodyMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstbodyMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstcommandsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstcommandsMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstcommandsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstcommandsMethod(interp, class, pattern);

  }
}
  
static int
XOTclClassInfoInstdefaultMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstdefaultMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstdefaultMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];
    char *arg = (char *)pc.clientData[2];
    Tcl_Obj *var = (Tcl_Obj *)pc.clientData[3];

    parseContextRelease(&pc);
    return XOTclClassInfoInstdefaultMethod(interp, class, methodName, arg, var);

  }
}
  
static int
XOTclClassInfoInstfilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstfilterMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstfilterMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withGuards = (int )pc.clientData[1];
    char *pattern = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclClassInfoInstfilterMethod(interp, class, withGuards, pattern);

  }
}
  
static int
XOTclClassInfoInstfilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstfilterguardMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstfilterguardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *filter = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstfilterguardMethod(interp, class, filter);

  }
}
  
static int
XOTclClassInfoInstforwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstforwardMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstforwardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withDefinition = (int )pc.clientData[1];
    char *pattern = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclClassInfoInstforwardMethod(interp, class, withDefinition, pattern);

  }
}
  
static int
XOTclClassInfoInstinvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstinvarMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstinvarMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclClassInfoInstinvarMethod(interp, class);

  }
}
  
static int
XOTclClassInfoInstmixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstmixinMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstmixinMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )pc.clientData[1];
    int withGuards = (int )pc.clientData[2];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[3];
    int returnCode;

    if (getMatchObject(interp, pattern,  objv[3], &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = XOTclClassInfoInstmixinMethod(interp, class, withClosure, withGuards, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}
  
static int
XOTclClassInfoInstmixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstmixinguardMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstmixinguardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *mixin = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstmixinguardMethod(interp, class, mixin);

  }
}
  
static int
XOTclClassInfoInstmixinofMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstmixinofMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstmixinofMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )pc.clientData[1];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[2];
    int returnCode;

    if (getMatchObject(interp, pattern,  objv[2], &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = XOTclClassInfoInstmixinofMethod(interp, class, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}
  
static int
XOTclClassInfoInstnonposargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstnonposargsMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstnonposargsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstnonposargsMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstparametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstparametercmdMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstparametercmdMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstparametercmdMethod(interp, class, pattern);

  }
}
  
static int
XOTclClassInfoInstpostMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstpostMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstpostMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstpostMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstpreMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstpreMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstpreMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstpreMethod(interp, class, methodName);

  }
}
  
static int
XOTclClassInfoInstprocsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoInstprocsMethodIdx].ifd, 
		method_definitions[XOTclClassInfoInstprocsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoInstprocsMethod(interp, class, pattern);

  }
}
  
static int
XOTclClassInfoMixinofMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoMixinofMethodIdx].ifd, 
		method_definitions[XOTclClassInfoMixinofMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )pc.clientData[1];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[2];
    int returnCode;

    if (getMatchObject(interp, pattern,  objv[2], &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = XOTclClassInfoMixinofMethod(interp, class, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}
  
static int
XOTclClassInfoParameterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoParameterMethodIdx].ifd, 
		method_definitions[XOTclClassInfoParameterMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclClassInfoParameterMethod(interp, class);

  }
}
  
static int
XOTclClassInfoSlotsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoSlotsMethodIdx].ifd, 
		method_definitions[XOTclClassInfoSlotsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclClassInfoSlotsMethod(interp, class);

  }
}
  
static int
XOTclClassInfoSubclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoSubclassMethodIdx].ifd, 
		method_definitions[XOTclClassInfoSubclassMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )pc.clientData[1];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[2];
    int returnCode;

    if (getMatchObject(interp, pattern,  objv[2], &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = XOTclClassInfoSubclassMethod(interp, class, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}
  
static int
XOTclClassInfoSuperclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclClassInfoSuperclassMethodIdx].ifd, 
		method_definitions[XOTclClassInfoSuperclassMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )pc.clientData[1];
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclClassInfoSuperclassMethod(interp, class, withClosure, pattern);

  }
}
  
static int
XOTclObjInfoArgsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoArgsMethodIdx].ifd, 
		method_definitions[XOTclObjInfoArgsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoArgsMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoBodyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoBodyMethodIdx].ifd, 
		method_definitions[XOTclObjInfoBodyMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoBodyMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoCheckMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoCheckMethodIdx].ifd, 
		method_definitions[XOTclObjInfoCheckMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclObjInfoCheckMethod(interp, object);

  }
}
  
static int
XOTclObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoChildrenMethodIdx].ifd, 
		method_definitions[XOTclObjInfoChildrenMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoChildrenMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoClassMethodIdx].ifd, 
		method_definitions[XOTclObjInfoClassMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclObjInfoClassMethod(interp, object);

  }
}
  
static int
XOTclObjInfoCommandsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoCommandsMethodIdx].ifd, 
		method_definitions[XOTclObjInfoCommandsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoCommandsMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoDefaultMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoDefaultMethodIdx].ifd, 
		method_definitions[XOTclObjInfoDefaultMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];
    char *arg = (char *)pc.clientData[2];
    Tcl_Obj *var = (Tcl_Obj *)pc.clientData[3];

    parseContextRelease(&pc);
    return XOTclObjInfoDefaultMethod(interp, object, methodName, arg, var);

  }
}
  
static int
XOTclObjInfoFilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoFilterMethodIdx].ifd, 
		method_definitions[XOTclObjInfoFilterMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withOrder = (int )pc.clientData[1];
    int withGuards = (int )pc.clientData[2];
    char *pattern = (char *)pc.clientData[3];

    parseContextRelease(&pc);
    return XOTclObjInfoFilterMethod(interp, object, withOrder, withGuards, pattern);

  }
}
  
static int
XOTclObjInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoFilterguardMethodIdx].ifd, 
		method_definitions[XOTclObjInfoFilterguardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *filter = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoFilterguardMethod(interp, object, filter);

  }
}
  
static int
XOTclObjInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoForwardMethodIdx].ifd, 
		method_definitions[XOTclObjInfoForwardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withDefinition = (int )pc.clientData[1];
    char *pattern = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclObjInfoForwardMethod(interp, object, withDefinition, pattern);

  }
}
  
static int
XOTclObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoHasnamespaceMethodIdx].ifd, 
		method_definitions[XOTclObjInfoHasnamespaceMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclObjInfoHasnamespaceMethod(interp, object);

  }
}
  
static int
XOTclObjInfoInvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoInvarMethodIdx].ifd, 
		method_definitions[XOTclObjInfoInvarMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclObjInfoInvarMethod(interp, object);

  }
}
  
static int
XOTclObjInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoMethodsMethodIdx].ifd, 
		method_definitions[XOTclObjInfoMethodsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withNoprocs = (int )pc.clientData[1];
    int withNocmds = (int )pc.clientData[2];
    int withNomixins = (int )pc.clientData[3];
    int withIncontext = (int )pc.clientData[4];
    char *pattern = (char *)pc.clientData[5];

    parseContextRelease(&pc);
    return XOTclObjInfoMethodsMethod(interp, object, withNoprocs, withNocmds, withNomixins, withIncontext, pattern);

  }
}
  
static int
XOTclObjInfoMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoMixinMethodIdx].ifd, 
		method_definitions[XOTclObjInfoMixinMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withGuards = (int )pc.clientData[1];
    int withOrder = (int )pc.clientData[2];
    char *patternString = NULL;
    XOTclObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[3];
    int returnCode;

    if (getMatchObject(interp, pattern,  objv[3], &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = XOTclObjInfoMixinMethod(interp, object, withGuards, withOrder, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}
  
static int
XOTclObjInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoMixinguardMethodIdx].ifd, 
		method_definitions[XOTclObjInfoMixinguardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *mixin = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoMixinguardMethod(interp, object, mixin);

  }
}
  
static int
XOTclObjInfoNonposargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoNonposargsMethodIdx].ifd, 
		method_definitions[XOTclObjInfoNonposargsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoNonposargsMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoParametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoParametercmdMethodIdx].ifd, 
		method_definitions[XOTclObjInfoParametercmdMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoParametercmdMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoParentMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoParentMethodIdx].ifd, 
		method_definitions[XOTclObjInfoParentMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclObjInfoParentMethod(interp, object);

  }
}
  
static int
XOTclObjInfoPostMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoPostMethodIdx].ifd, 
		method_definitions[XOTclObjInfoPostMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoPostMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoPreMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoPreMethodIdx].ifd, 
		method_definitions[XOTclObjInfoPreMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoPreMethod(interp, object, methodName);

  }
}
  
static int
XOTclObjInfoPrecedenceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoPrecedenceMethodIdx].ifd, 
		method_definitions[XOTclObjInfoPrecedenceMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withIntrinsic = (int )pc.clientData[1];
    char *pattern = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclObjInfoPrecedenceMethod(interp, object, withIntrinsic, pattern);

  }
}
  
static int
XOTclObjInfoProcsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoProcsMethodIdx].ifd, 
		method_definitions[XOTclObjInfoProcsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoProcsMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoSlotObjectsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoSlotObjectsMethodIdx].ifd, 
		method_definitions[XOTclObjInfoSlotObjectsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoSlotObjectsMethod(interp, object, pattern);

  }
}
  
static int
XOTclObjInfoVarsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclObjInfoVarsMethodIdx].ifd, 
		method_definitions[XOTclObjInfoVarsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *pattern = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclObjInfoVarsMethod(interp, object, pattern);

  }
}
  
static int
XOTclOAutonameMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOAutonameMethodIdx].ifd, 
		method_definitions[XOTclOAutonameMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withInstance = (int )pc.clientData[0];
    int withReset = (int )pc.clientData[1];
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclOAutonameMethod(interp, obj, withInstance, withReset, name);

  }
}
  
static int
XOTclOCheckMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOCheckMethodIdx].ifd, 
		method_definitions[XOTclOCheckMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *flag = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOCheckMethod(interp, obj, flag);

  }
}
  
static int
XOTclOCleanupMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOCleanupMethodIdx].ifd, 
		method_definitions[XOTclOCleanupMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclOCleanupMethod(interp, obj);

  }
}
  
static int
XOTclOConfigureMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOConfigureMethodIdx].ifd, 
		method_definitions[XOTclOConfigureMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclOConfigureMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclODestroyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclODestroyMethodIdx].ifd, 
		method_definitions[XOTclODestroyMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclODestroyMethod(interp, obj);

  }
}
  
static int
XOTclOExistsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOExistsMethodIdx].ifd, 
		method_definitions[XOTclOExistsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *var = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOExistsMethod(interp, obj, var);

  }
}
  
static int
XOTclOFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOFilterGuardMethodIdx].ifd, 
		method_definitions[XOTclOFilterGuardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *filter = (char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclOFilterGuardMethod(interp, obj, filter, guard);

  }
}
  
static int
XOTclOFilterSearchMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOFilterSearchMethodIdx].ifd, 
		method_definitions[XOTclOFilterSearchMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *filter = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOFilterSearchMethod(interp, obj, filter);

  }
}
  
static int
XOTclOForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOForwardMethodIdx].ifd, 
		method_definitions[XOTclOForwardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *method = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *withDefault = (Tcl_Obj *)pc.clientData[1];
    int withEarlybinding = (int )pc.clientData[2];
    Tcl_Obj *withMethodprefix = (Tcl_Obj *)pc.clientData[3];
    int withObjscope = (int )pc.clientData[4];
    Tcl_Obj *withOnerror = (Tcl_Obj *)pc.clientData[5];
    int withVerbose = (int )pc.clientData[6];
    Tcl_Obj *target = (Tcl_Obj *)pc.clientData[7];

    parseContextRelease(&pc);
    return XOTclOForwardMethod(interp, obj, method, withDefault, withEarlybinding, withMethodprefix, withObjscope, withOnerror, withVerbose, target, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}
  
static int
XOTclOInstVarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOInstVarMethodIdx].ifd, 
		method_definitions[XOTclOInstVarMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclOInstVarMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclOInvariantsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOInvariantsMethodIdx].ifd, 
		method_definitions[XOTclOInvariantsMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *invariantlist = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOInvariantsMethod(interp, obj, invariantlist);

  }
}
  
static int
XOTclOIsClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOIsClassMethodIdx].ifd, 
		method_definitions[XOTclOIsClassMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *class = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOIsClassMethod(interp, obj, class);

  }
}
  
static int
XOTclOIsMetaClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOIsMetaClassMethodIdx].ifd, 
		method_definitions[XOTclOIsMetaClassMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *metaclass = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOIsMetaClassMethod(interp, obj, metaclass);

  }
}
  
static int
XOTclOIsMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOIsMixinMethodIdx].ifd, 
		method_definitions[XOTclOIsMixinMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *class = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOIsMixinMethod(interp, obj, class);

  }
}
  
static int
XOTclOIsObjectMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOIsObjectMethodIdx].ifd, 
		method_definitions[XOTclOIsObjectMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *object = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOIsObjectMethod(interp, obj, object);

  }
}
  
static int
XOTclOIsTypeMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOIsTypeMethodIdx].ifd, 
		method_definitions[XOTclOIsTypeMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *class = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOIsTypeMethod(interp, obj, class);

  }
}
  
static int
XOTclOMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOMixinGuardMethodIdx].ifd, 
		method_definitions[XOTclOMixinGuardMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *mixin = (char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclOMixinGuardMethod(interp, obj, mixin, guard);

  }
}
  
static int
XOTclONextMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclONextMethodIdx].ifd, 
		method_definitions[XOTclONextMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclONextMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclONoinitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclONoinitMethodIdx].ifd, 
		method_definitions[XOTclONoinitMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclONoinitMethod(interp, obj);

  }
}
  
static int
XOTclOParametercmdMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOParametercmdMethodIdx].ifd, 
		method_definitions[XOTclOParametercmdMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *name = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOParametercmdMethod(interp, obj, name);

  }
}
  
static int
XOTclOProcMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOProcMethodIdx].ifd, 
		method_definitions[XOTclOProcMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *args = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *body = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj *precondition = (Tcl_Obj *)pc.clientData[3];
    Tcl_Obj *postcondition = (Tcl_Obj *)pc.clientData[4];

    parseContextRelease(&pc);
    return XOTclOProcMethod(interp, obj, name, args, body, precondition, postcondition);

  }
}
  
static int
XOTclOProcSearchMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOProcSearchMethodIdx].ifd, 
		method_definitions[XOTclOProcSearchMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *name = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOProcSearchMethod(interp, obj, name);

  }
}
  
static int
XOTclORequireNamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclORequireNamespaceMethodIdx].ifd, 
		method_definitions[XOTclORequireNamespaceMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclORequireNamespaceMethod(interp, obj);

  }
}
  
static int
XOTclOSetMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOSetMethodIdx].ifd, 
		method_definitions[XOTclOSetMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *var = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclOSetMethod(interp, obj, var, value);

  }
}
  
static int
XOTclOSetvaluesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOSetvaluesMethodIdx].ifd, 
		method_definitions[XOTclOSetvaluesMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclOSetvaluesMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclOUplevelMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOUplevelMethodIdx].ifd, 
		method_definitions[XOTclOUplevelMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclOUplevelMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclOUpvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOUpvarMethodIdx].ifd, 
		method_definitions[XOTclOUpvarMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclOUpvarMethod(interp, obj, objc, objv);

  }
}
  
static int
XOTclOVolatileMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOVolatileMethodIdx].ifd, 
		method_definitions[XOTclOVolatileMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclOVolatileMethod(interp, obj);

  }
}
  
static int
XOTclOVwaitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclOVwaitMethodIdx].ifd, 
		method_definitions[XOTclOVwaitMethodIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *varname = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOVwaitMethod(interp, obj, varname);

  }
}
  
static int
XOTclAliasCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclAliasCmdIdx].ifd, 
		method_definitions[XOTclAliasCmdIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];
    int withObjscope = (int )pc.clientData[2];
    int withPer_object = (int )pc.clientData[3];
    int withProtected = (int )pc.clientData[4];
    Tcl_Obj *cmdName = (Tcl_Obj *)pc.clientData[5];

    parseContextRelease(&pc);
    return XOTclAliasCmd(interp, object, methodName, withObjscope, withPer_object, withProtected, cmdName);

  }
}
  
static int
XOTclConfigureCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclConfigureCmdIdx].ifd, 
		method_definitions[XOTclConfigureCmdIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int configureoption = (int )pc.clientData[0];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclConfigureCmd(interp, configureoption, value);

  }
}
  
static int
XOTclMethodPropertyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclMethodPropertyCmdIdx].ifd, 
		method_definitions[XOTclMethodPropertyCmdIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *methodName = (char *)pc.clientData[1];
    int withPer_object = (int )pc.clientData[2];
    int methodproperty = (int )pc.clientData[3];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[4];

    parseContextRelease(&pc);
    return XOTclMethodPropertyCmd(interp, object, methodName, withPer_object, methodproperty, value);

  }
}
  
static int
XOTclMyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclMyCmdIdx].ifd, 
		method_definitions[XOTclMyCmdIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withLocal = (int )pc.clientData[0];
    Tcl_Obj *method = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclMyCmd(interp, withLocal, method, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}
  
static int
XOTclRelationCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclRelationCmdIdx].ifd, 
		method_definitions[XOTclRelationCmdIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int relationtype = (int )pc.clientData[1];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclRelationCmd(interp, object, relationtype, value);

  }
}
  
static int
XOTclSetInstvarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[XOTclSetInstvarCmdIdx].ifd, 
		method_definitions[XOTclSetInstvarCmdIdx].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    Tcl_Obj *variable = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclSetInstvarCmd(interp, object, variable, value);

  }
}
  
static methodDefinition method_definitions[] = {
{"::xotcl::cmd::NonposArgs::type=boolean", XOTclCheckBooleanArgsStub, 2, {
  {"name", 1, 0, convertToString},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::NonposArgs::type=required", XOTclCheckRequiredArgsStub, 2, {
  {"name", 1, 0, convertToString},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::alloc", XOTclCAllocMethodStub, 1, {
  {"name", 1, 0, convertToString}}
},
{"::xotcl::cmd::Class::create", XOTclCCreateMethodStub, 2, {
  {"name", 1, 0, convertToString},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Class::dealloc", XOTclCDeallocMethodStub, 1, {
  {"object", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::instfilterguard", XOTclCInstFilterGuardMethodStub, 2, {
  {"filter", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::instforward", XOTclCInstForwardMethodStub, 9, {
  {"method", 1, 0, convertToTclobj},
  {"-default", 0, 1, convertToTclobj},
  {"-earlybinding", 0, 0, convertToString},
  {"-methodprefix", 0, 1, convertToTclobj},
  {"-objscope", 0, 0, convertToString},
  {"-onerror", 0, 1, convertToTclobj},
  {"-verbose", 0, 0, convertToString},
  {"target", 0, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Class::instmixinguard", XOTclCInstMixinGuardMethodStub, 2, {
  {"mixin", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::instparametercmd", XOTclCInstParametercmdMethodStub, 1, {
  {"name", 1, 0, convertToString}}
},
{"::xotcl::cmd::Class::instproc", XOTclCInstProcMethodStub, 5, {
  {"name", 1, 0, convertToTclobj},
  {"args", 1, 0, convertToTclobj},
  {"body", 1, 0, convertToTclobj},
  {"precondition", 0, 0, convertToTclobj},
  {"postcondition", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::classscopedinstproc", XOTclCInstProcMethodCStub, 5, {
  {"name", 1, 0, convertToTclobj},
  {"args", 1, 0, convertToTclobj},
  {"body", 1, 0, convertToTclobj},
  {"precondition", 0, 0, convertToTclobj},
  {"postcondition", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::instinvar", XOTclCInvariantsMethodStub, 1, {
  {"invariantlist", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::new", XOTclCNewMethodStub, 2, {
  {"-childof", 0, 1, convertToObject},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Class::recreate", XOTclCRecreateMethodStub, 2, {
  {"name", 1, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Class::unknown", XOTclCUnknownMethodStub, 2, {
  {"name", 1, 0, convertToString},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::ClassInfo::heritage", XOTclClassInfoHeritageMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instances", XOTclClassInfoInstancesMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-closure", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::xotcl::cmd::ClassInfo::instargs", XOTclClassInfoInstargsMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instbody", XOTclClassInfoInstbodyMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instcommands", XOTclClassInfoInstcommandsMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instdefault", XOTclClassInfoInstdefaultMethodStub, 4, {
  {"class", 1, 0, convertToClass},
  {"methodName", 1, 0, convertToString},
  {"arg", 1, 0, convertToString},
  {"var", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::ClassInfo::instfilter", XOTclClassInfoInstfilterMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-guards", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instfilterguard", XOTclClassInfoInstfilterguardMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"filter", 1, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instforward", XOTclClassInfoInstforwardMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-definition", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instinvar", XOTclClassInfoInstinvarMethodStub, 1, {
  {"class", 1, 0, convertToClass}}
},
{"::xotcl::cmd::ClassInfo::instmixin", XOTclClassInfoInstmixinMethodStub, 4, {
  {"class", 1, 0, convertToClass},
  {"-closure", 0, 0, convertToString},
  {"-guards", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::xotcl::cmd::ClassInfo::instmixinguard", XOTclClassInfoInstmixinguardMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"mixin", 1, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instmixinof", XOTclClassInfoInstmixinofMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-closure", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::xotcl::cmd::ClassInfo::instnonposargs", XOTclClassInfoInstnonposargsMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instparametercmd", XOTclClassInfoInstparametercmdMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instpost", XOTclClassInfoInstpostMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instpre", XOTclClassInfoInstpreMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::instprocs", XOTclClassInfoInstprocsMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::mixinof", XOTclClassInfoMixinofMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-closure", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::xotcl::cmd::ClassInfo::parameter", XOTclClassInfoParameterMethodStub, 1, {
  {"class", 1, 0, convertToClass}}
},
{"::xotcl::cmd::ClassInfo::slots", XOTclClassInfoSlotsMethodStub, 1, {
  {"class", 1, 0, convertToClass}}
},
{"::xotcl::cmd::ClassInfo::subclass", XOTclClassInfoSubclassMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-closure", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::xotcl::cmd::ClassInfo::superclass", XOTclClassInfoSuperclassMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-closure", 0, 0, convertToString},
  {"pattern", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::ObjectInfo::args", XOTclObjInfoArgsMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::body", XOTclObjInfoBodyMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::check", XOTclObjInfoCheckMethodStub, 1, {
  {"object", 1, 0, convertToObject}}
},
{"::xotcl::cmd::ObjectInfo::children", XOTclObjInfoChildrenMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::class", XOTclObjInfoClassMethodStub, 1, {
  {"object", 1, 0, convertToObject}}
},
{"::xotcl::cmd::ObjectInfo::commands", XOTclObjInfoCommandsMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::default", XOTclObjInfoDefaultMethodStub, 4, {
  {"object", 1, 0, convertToObject},
  {"methodName", 1, 0, convertToString},
  {"arg", 1, 0, convertToString},
  {"var", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::ObjectInfo::filter", XOTclObjInfoFilterMethodStub, 4, {
  {"object", 1, 0, convertToObject},
  {"-order", 0, 0, convertToString},
  {"-guards", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::filterguard", XOTclObjInfoFilterguardMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"filter", 1, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::forward", XOTclObjInfoForwardMethodStub, 3, {
  {"object", 1, 0, convertToObject},
  {"-definition", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::hasnamespace", XOTclObjInfoHasnamespaceMethodStub, 1, {
  {"object", 1, 0, convertToObject}}
},
{"::xotcl::cmd::ObjectInfo::invar", XOTclObjInfoInvarMethodStub, 1, {
  {"object", 1, 0, convertToObject}}
},
{"::xotcl::cmd::ObjectInfo::methods", XOTclObjInfoMethodsMethodStub, 6, {
  {"object", 1, 0, convertToObject},
  {"-noprocs", 0, 0, convertToString},
  {"-nocmds", 0, 0, convertToString},
  {"-nomixins", 0, 0, convertToString},
  {"-incontext", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::mixin", XOTclObjInfoMixinMethodStub, 4, {
  {"object", 1, 0, convertToObject},
  {"-guards", 0, 0, convertToString},
  {"-order", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::xotcl::cmd::ObjectInfo::mixinguard", XOTclObjInfoMixinguardMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"mixin", 1, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::nonposargs", XOTclObjInfoNonposargsMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::parametercmd", XOTclObjInfoParametercmdMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::parent", XOTclObjInfoParentMethodStub, 1, {
  {"object", 1, 0, convertToObject}}
},
{"::xotcl::cmd::ObjectInfo::post", XOTclObjInfoPostMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::pre", XOTclObjInfoPreMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"methodName", 1, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::precedence", XOTclObjInfoPrecedenceMethodStub, 3, {
  {"object", 1, 0, convertToObject},
  {"-intrinsic", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::procs", XOTclObjInfoProcsMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::slotobjects", XOTclObjInfoSlotObjectsMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::vars", XOTclObjInfoVarsMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::Object::autoname", XOTclOAutonameMethodStub, 3, {
  {"-instance", 0, 0, convertToString},
  {"-reset", 0, 0, convertToString},
  {"name", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::check", XOTclOCheckMethodStub, 1, {
  {"flag", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::cleanup", XOTclOCleanupMethodStub, 0, {
  }
},
{"::xotcl::cmd::Object::configure", XOTclOConfigureMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Object::destroy", XOTclODestroyMethodStub, 0, {
  }
},
{"::xotcl::cmd::Object::exists", XOTclOExistsMethodStub, 1, {
  {"var", 1, 0, convertToString}}
},
{"::xotcl::cmd::Object::filterguard", XOTclOFilterGuardMethodStub, 2, {
  {"filter", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::filtersearch", XOTclOFilterSearchMethodStub, 1, {
  {"filter", 1, 0, convertToString}}
},
{"::xotcl::cmd::Object::forward", XOTclOForwardMethodStub, 9, {
  {"method", 1, 0, convertToTclobj},
  {"-default", 0, 1, convertToTclobj},
  {"-earlybinding", 0, 0, convertToString},
  {"-methodprefix", 0, 1, convertToTclobj},
  {"-objscope", 0, 0, convertToString},
  {"-onerror", 0, 1, convertToTclobj},
  {"-verbose", 0, 0, convertToString},
  {"target", 0, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Object::instvar", XOTclOInstVarMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Object::invar", XOTclOInvariantsMethodStub, 1, {
  {"invariantlist", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::isclass", XOTclOIsClassMethodStub, 1, {
  {"class", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::ismetaclass", XOTclOIsMetaClassMethodStub, 1, {
  {"metaclass", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::ismixin", XOTclOIsMixinMethodStub, 1, {
  {"class", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::isobject", XOTclOIsObjectMethodStub, 1, {
  {"object", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::istype", XOTclOIsTypeMethodStub, 1, {
  {"class", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::mixinguard", XOTclOMixinGuardMethodStub, 2, {
  {"mixin", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::__next", XOTclONextMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Object::noinit", XOTclONoinitMethodStub, 0, {
  }
},
{"::xotcl::cmd::Object::parametercmd", XOTclOParametercmdMethodStub, 1, {
  {"name", 1, 0, convertToString}}
},
{"::xotcl::cmd::Object::proc", XOTclOProcMethodStub, 5, {
  {"name", 1, 0, convertToTclobj},
  {"args", 1, 0, convertToTclobj},
  {"body", 1, 0, convertToTclobj},
  {"precondition", 0, 0, convertToTclobj},
  {"postcondition", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::procsearch", XOTclOProcSearchMethodStub, 1, {
  {"name", 1, 0, convertToString}}
},
{"::xotcl::cmd::Object::requireNamespace", XOTclORequireNamespaceMethodStub, 0, {
  }
},
{"::xotcl::cmd::Object::set", XOTclOSetMethodStub, 2, {
  {"var", 1, 0, convertToTclobj},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::Object::setvalues", XOTclOSetvaluesMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Object::uplevel", XOTclOUplevelMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Object::upvar", XOTclOUpvarMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Object::volatile", XOTclOVolatileMethodStub, 0, {
  }
},
{"::xotcl::cmd::Object::vwait", XOTclOVwaitMethodStub, 1, {
  {"varname", 1, 0, convertToString}}
},
{"::xotcl::alias", XOTclAliasCmdStub, 6, {
  {"object", 1, 0, convertToObject},
  {"methodName", 1, 0, convertToString},
  {"-objscope", 0, 0, convertToString},
  {"-per-object", 0, 0, convertToString},
  {"-protected", 0, 0, convertToString},
  {"cmdName", 1, 0, convertToTclobj}}
},
{"::xotcl::configure", XOTclConfigureCmdStub, 2, {
  {"filter|softrecreate", 1, 0, convertToConfigureoption},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::methodproperty", XOTclMethodPropertyCmdStub, 5, {
  {"object", 1, 0, convertToObject},
  {"methodName", 1, 0, convertToString},
  {"-per-object", 0, 0, convertToString},
  {"protected|public|slotobj", 1, 0, convertToMethodproperty},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::my", XOTclMyCmdStub, 3, {
  {"-local", 0, 0, convertToString},
  {"method", 1, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::relation", XOTclRelationCmdStub, 3, {
  {"object", 1, 0, convertToObject},
  {"mixin|instmixin|object-mixin|class-mixin|filter|instfilter|object-filter|class_filter|class|superclass|rootclass", 1, 0, convertToRelationtype},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::setinstvar", XOTclSetInstvarCmdStub, 3, {
  {"object", 1, 0, convertToObject},
  {"variable", 1, 0, convertToTclobj},
  {"value", 0, 0, convertToTclobj}}
}
};

