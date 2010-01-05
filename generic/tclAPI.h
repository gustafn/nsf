
static int convertToInfomethodsubcmd(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, ClientData *clientData) {
  int index, result;
  static CONST char *opts[] = {"args", "body", "definition", "name", "parameter", "type", "precondition", "postcondition", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "infomethodsubcmd", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  return result;
}
enum InfomethodsubcmdIdx {InfomethodsubcmdNULL, InfomethodsubcmdArgsIdx, InfomethodsubcmdBodyIdx, InfomethodsubcmdDefinitionIdx, InfomethodsubcmdNameIdx, InfomethodsubcmdParameterIdx, InfomethodsubcmdTypeIdx, InfomethodsubcmdPreconditionIdx, InfomethodsubcmdPostconditionIdx};
  
static int convertToMethodtype(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, ClientData *clientData) {
  int index, result;
  static CONST char *opts[] = {"all", "scripted", "builtin", "alias", "forwarder", "object", "setter", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-methodtype", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  return result;
}
enum MethodtypeIdx {MethodtypeNULL, MethodtypeAllIdx, MethodtypeScriptedIdx, MethodtypeBuiltinIdx, MethodtypeAliasIdx, MethodtypeForwarderIdx, MethodtypeObjectIdx, MethodtypeSetterIdx};
  
static int convertToCallprotection(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, ClientData *clientData) {
  int index, result;
  static CONST char *opts[] = {"all", "protected", "public", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-callprotection", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  return result;
}
enum CallprotectionIdx {CallprotectionNULL, CallprotectionAllIdx, CallprotectionProtectedIdx, CallprotectionPublicIdx};
  
static int convertToAssertionsubcmd(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, ClientData *clientData) {
  int index, result;
  static CONST char *opts[] = {"check", "object-invar", "class-invar", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "assertionsubcmd", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  return result;
}
enum AssertionsubcmdIdx {AssertionsubcmdNULL, AssertionsubcmdCheckIdx, AssertionsubcmdObject_invarIdx, AssertionsubcmdClass_invarIdx};
  
static int convertToConfigureoption(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, ClientData *clientData) {
  int index, result;
  static CONST char *opts[] = {"filter", "softrecreate", "cacheinterface", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "configureoption", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  return result;
}
enum ConfigureoptionIdx {ConfigureoptionNULL, ConfigureoptionFilterIdx, ConfigureoptionSoftrecreateIdx, ConfigureoptionCacheinterfaceIdx};
  
static int convertToSelfoption(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, ClientData *clientData) {
  int index, result;
  static CONST char *opts[] = {"proc", "class", "activelevel", "args", "activemixin", "calledproc", "calledmethod", "calledclass", "callingproc", "callingclass", "callinglevel", "callingobject", "filterreg", "isnextcall", "next", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "selfoption", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  return result;
}
enum SelfoptionIdx {SelfoptionNULL, SelfoptionProcIdx, SelfoptionClassIdx, SelfoptionActivelevelIdx, SelfoptionArgsIdx, SelfoptionActivemixinIdx, SelfoptionCalledprocIdx, SelfoptionCalledmethodIdx, SelfoptionCalledclassIdx, SelfoptionCallingprocIdx, SelfoptionCallingclassIdx, SelfoptionCallinglevelIdx, SelfoptionCallingobjectIdx, SelfoptionFilterregIdx, SelfoptionIsnextcallIdx, SelfoptionNextIdx};
  
static int convertToObjectkind(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, ClientData *clientData) {
  int index, result;
  static CONST char *opts[] = {"type", "object", "class", "metaclass", "mixin", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "objectkind", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  return result;
}
enum ObjectkindIdx {ObjectkindNULL, ObjectkindTypeIdx, ObjectkindObjectIdx, ObjectkindClassIdx, ObjectkindMetaclassIdx, ObjectkindMixinIdx};
  
static int convertToMethodproperty(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, ClientData *clientData) {
  int index, result;
  static CONST char *opts[] = {"protected", "redefine-protected", "slotobj", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "methodproperty", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  return result;
}
enum MethodpropertyIdx {MethodpropertyNULL, MethodpropertyProtectedIdx, MethodpropertyRedefine_protectedIdx, MethodpropertySlotobjIdx};
  
static int convertToRelationtype(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, ClientData *clientData) {
  int index, result;
  static CONST char *opts[] = {"object-mixin", "class-mixin", "object-filter", "class-filter", "class", "superclass", "rootclass", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "relationtype", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  return result;
}
enum RelationtypeIdx {RelationtypeNULL, RelationtypeObject_mixinIdx, RelationtypeClass_mixinIdx, RelationtypeObject_filterIdx, RelationtypeClass_filterIdx, RelationtypeClassIdx, RelationtypeSuperclassIdx, RelationtypeRootclassIdx};
  

typedef struct {
  char *methodName;
  Tcl_ObjCmdProc *proc;
  int nrParameters;
  XOTclParam paramDefs[12];
} methodDefinition;

static int ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], 
                         XOTclObject *obj, Tcl_Obj *procName,
                         XOTclParam CONST *paramPtr, int nrParameters, parseContext *pc);

static int getMatchObject(Tcl_Interp *interp, Tcl_Obj *patternObj, Tcl_Obj *origObj,
			  XOTclObject **matchObject, char **pattern);

/* just to define the symbol */
static methodDefinition method_definitions[];
  
static char *method_command_namespace_names[] = {
  "::xotcl::cmd::ObjectInfo",
  "::xotcl::cmd::Object",
  "::xotcl::cmd::ClassInfo",
  "::xotcl::cmd::ParameterType",
  "::xotcl::cmd::Class"
};
static int XOTclCheckBooleanArgsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCheckRequiredArgsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCAllocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCCreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCDeallocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCInvalidateObjectParameterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCNewMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCRecreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoClassMixinOfMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoFilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoHeritageMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoInstancesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoObjectMixinOfMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoParameterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoSlotsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoSubclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclClassInfoSuperclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoCallableMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoFilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoParentMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoPrecedenceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoSlotObjectsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclObjInfoVarsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOAutonameMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOCleanupMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOConfigureMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclODestroyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOExistsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOFilterSearchMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOInstVarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclONextMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclONoinitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclORequireNamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOResidualargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOUplevelMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOUpvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOVolatileMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclOVwaitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclAliasCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclAssertionCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclConfigureCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclCreateObjectSystemCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclDeprecatedCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclDispatchCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclDotCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclExistsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclFinalizeObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclForwardCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclGetSelfObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclImportvarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclInterpObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclIsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclMethodCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclMethodPropertyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclMyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclNSCopyCmdsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclNSCopyVarsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclQualifyObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclRelationCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclSetInstvarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int XOTclSetterCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

static int XOTclCheckBooleanArgs(Tcl_Interp *interp, char *name, Tcl_Obj *value);
static int XOTclCheckRequiredArgs(Tcl_Interp *interp, char *name, Tcl_Obj *value);
static int XOTclCAllocMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *name);
static int XOTclCCreateMethod(Tcl_Interp *interp, XOTclClass *cl, char *name, int objc, Tcl_Obj *CONST objv[]);
static int XOTclCDeallocMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *object);
static int XOTclCFilterGuardMethod(Tcl_Interp *interp, XOTclClass *cl, char *filter, Tcl_Obj *guard);
static int XOTclCInvalidateObjectParameterMethod(Tcl_Interp *interp, XOTclClass *cl);
static int XOTclCMixinGuardMethod(Tcl_Interp *interp, XOTclClass *cl, char *mixin, Tcl_Obj *guard);
static int XOTclCNewMethod(Tcl_Interp *interp, XOTclClass *cl, XOTclObject *withChildof, int nobjc, Tcl_Obj *CONST nobjv[]);
static int XOTclCRecreateMethod(Tcl_Interp *interp, XOTclClass *cl, Tcl_Obj *name, int objc, Tcl_Obj *CONST objv[]);
static int XOTclClassInfoClassMixinOfMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoFilterMethod(Tcl_Interp *interp, XOTclClass *class, int withGuards, char *pattern);
static int XOTclClassInfoFilterguardMethod(Tcl_Interp *interp, XOTclClass *class, char *filter);
static int XOTclClassInfoForwardMethod(Tcl_Interp *interp, XOTclClass *class, int withDefinition, char *name);
static int XOTclClassInfoHeritageMethod(Tcl_Interp *interp, XOTclClass *class, char *pattern);
static int XOTclClassInfoInstancesMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoMethodMethod(Tcl_Interp *interp, XOTclClass *class, int infomethodsubcmd, char *name);
static int XOTclClassInfoMethodsMethod(Tcl_Interp *interp, XOTclClass *object, int withMethodtype, int withCallprotection, int withNomixins, int withIncontext, char *pattern);
static int XOTclClassInfoMixinMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, int withGuards, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoMixinguardMethod(Tcl_Interp *interp, XOTclClass *class, char *mixin);
static int XOTclClassInfoObjectMixinOfMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoParameterMethod(Tcl_Interp *interp, XOTclClass *class);
static int XOTclClassInfoSlotsMethod(Tcl_Interp *interp, XOTclClass *class);
static int XOTclClassInfoSubclassMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, char *patternString, XOTclObject *patternObj);
static int XOTclClassInfoSuperclassMethod(Tcl_Interp *interp, XOTclClass *class, int withClosure, Tcl_Obj *pattern);
static int XOTclObjInfoCallableMethod(Tcl_Interp *interp, XOTclObject *object, int withWhich, int withMethodtype, int withCallprotection, int withApplication, int withNomixins, int withIncontext, char *pattern);
static int XOTclObjInfoChildrenMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern);
static int XOTclObjInfoClassMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclObjInfoFilterMethod(Tcl_Interp *interp, XOTclObject *object, int withOrder, int withGuards, char *pattern);
static int XOTclObjInfoFilterguardMethod(Tcl_Interp *interp, XOTclObject *object, char *filter);
static int XOTclObjInfoForwardMethod(Tcl_Interp *interp, XOTclObject *object, int withDefinition, char *name);
static int XOTclObjInfoHasnamespaceMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclObjInfoMethodMethod(Tcl_Interp *interp, XOTclObject *object, int infomethodsubcmd, char *name);
static int XOTclObjInfoMethodsMethod(Tcl_Interp *interp, XOTclObject *object, int withMethodtype, int withCallprotection, int withNomixins, int withIncontext, char *pattern);
static int XOTclObjInfoMixinMethod(Tcl_Interp *interp, XOTclObject *object, int withGuards, int withOrder, char *patternString, XOTclObject *patternObj);
static int XOTclObjInfoMixinguardMethod(Tcl_Interp *interp, XOTclObject *object, char *mixin);
static int XOTclObjInfoParentMethod(Tcl_Interp *interp, XOTclObject *object);
static int XOTclObjInfoPrecedenceMethod(Tcl_Interp *interp, XOTclObject *object, int withIntrinsic, char *pattern);
static int XOTclObjInfoSlotObjectsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern);
static int XOTclObjInfoVarsMethod(Tcl_Interp *interp, XOTclObject *object, char *pattern);
static int XOTclOAutonameMethod(Tcl_Interp *interp, XOTclObject *obj, int withInstance, int withReset, Tcl_Obj *name);
static int XOTclOCleanupMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclOConfigureMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclODestroyMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclOExistsMethod(Tcl_Interp *interp, XOTclObject *obj, char *var);
static int XOTclOFilterGuardMethod(Tcl_Interp *interp, XOTclObject *obj, char *filter, Tcl_Obj *guard);
static int XOTclOFilterSearchMethod(Tcl_Interp *interp, XOTclObject *obj, char *filter);
static int XOTclOInstVarMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclOMixinGuardMethod(Tcl_Interp *interp, XOTclObject *obj, char *mixin, Tcl_Obj *guard);
static int XOTclONextMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclONoinitMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclORequireNamespaceMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclOResidualargsMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclOUplevelMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclOUpvarMethod(Tcl_Interp *interp, XOTclObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int XOTclOVolatileMethod(Tcl_Interp *interp, XOTclObject *obj);
static int XOTclOVwaitMethod(Tcl_Interp *interp, XOTclObject *obj, char *varname);
static int XOTclAliasCmd(Tcl_Interp *interp, XOTclObject *object, int withPer_object, char *methodName, int withNonleaf, int withObjscope, Tcl_Obj *cmdName);
static int XOTclAssertionCmd(Tcl_Interp *interp, XOTclObject *object, int assertionsubcmd, Tcl_Obj *arg);
static int XOTclConfigureCmd(Tcl_Interp *interp, int configureoption, Tcl_Obj *value);
static int XOTclCreateObjectSystemCmd(Tcl_Interp *interp, Tcl_Obj *rootClass, Tcl_Obj *rootMetaClass);
static int XOTclDeprecatedCmd(Tcl_Interp *interp, char *what, char *oldCmd, char *newCmd);
static int XOTclDispatchCmd(Tcl_Interp *interp, XOTclObject *object, int withObjscope, Tcl_Obj *command, int nobjc, Tcl_Obj *CONST nobjv[]);
static int XOTclDotCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int XOTclExistsCmd(Tcl_Interp *interp, XOTclObject *object, char *var);
static int XOTclFinalizeObjCmd(Tcl_Interp *interp);
static int XOTclForwardCmd(Tcl_Interp *interp, XOTclObject *object, int withPer_object, Tcl_Obj *method, Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix, int withObjscope, Tcl_Obj *withOnerror, int withVerbose, Tcl_Obj *target, int nobjc, Tcl_Obj *CONST nobjv[]);
static int XOTclGetSelfObjCmd(Tcl_Interp *interp, int selfoption);
static int XOTclImportvarCmd(Tcl_Interp *interp, XOTclObject *object, int nobjc, Tcl_Obj *CONST nobjv[]);
static int XOTclInterpObjCmd(Tcl_Interp *interp, char *name, int objc, Tcl_Obj *CONST objv[]);
static int XOTclIsCmd(Tcl_Interp *interp, Tcl_Obj *object, int objectkind, Tcl_Obj *value);
static int XOTclMethodCmd(Tcl_Interp *interp, XOTclObject *object, int withInner_namespace, int withPer_object, int withPublic, Tcl_Obj *name, Tcl_Obj *args, Tcl_Obj *body, Tcl_Obj *withPrecondition, Tcl_Obj *withPostcondition);
static int XOTclMethodPropertyCmd(Tcl_Interp *interp, XOTclObject *object, int withPer_object, Tcl_Obj *methodName, int methodproperty, Tcl_Obj *value);
static int XOTclMyCmd(Tcl_Interp *interp, int withLocal, Tcl_Obj *method, int nobjc, Tcl_Obj *CONST nobjv[]);
static int XOTclNSCopyCmds(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs);
static int XOTclNSCopyVars(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs);
static int XOTclQualifyObjCmd(Tcl_Interp *interp, Tcl_Obj *name);
static int XOTclRelationCmd(Tcl_Interp *interp, XOTclObject *object, int relationtype, Tcl_Obj *value);
static int XOTclSetInstvarCmd(Tcl_Interp *interp, XOTclObject *object, Tcl_Obj *variable, Tcl_Obj *value);
static int XOTclSetterCmd(Tcl_Interp *interp, XOTclObject *object, int withPer_object, char *methodName);

enum {
 XOTclCheckBooleanArgsIdx,
 XOTclCheckRequiredArgsIdx,
 XOTclCAllocMethodIdx,
 XOTclCCreateMethodIdx,
 XOTclCDeallocMethodIdx,
 XOTclCFilterGuardMethodIdx,
 XOTclCInvalidateObjectParameterMethodIdx,
 XOTclCMixinGuardMethodIdx,
 XOTclCNewMethodIdx,
 XOTclCRecreateMethodIdx,
 XOTclClassInfoClassMixinOfMethodIdx,
 XOTclClassInfoFilterMethodIdx,
 XOTclClassInfoFilterguardMethodIdx,
 XOTclClassInfoForwardMethodIdx,
 XOTclClassInfoHeritageMethodIdx,
 XOTclClassInfoInstancesMethodIdx,
 XOTclClassInfoMethodMethodIdx,
 XOTclClassInfoMethodsMethodIdx,
 XOTclClassInfoMixinMethodIdx,
 XOTclClassInfoMixinguardMethodIdx,
 XOTclClassInfoObjectMixinOfMethodIdx,
 XOTclClassInfoParameterMethodIdx,
 XOTclClassInfoSlotsMethodIdx,
 XOTclClassInfoSubclassMethodIdx,
 XOTclClassInfoSuperclassMethodIdx,
 XOTclObjInfoCallableMethodIdx,
 XOTclObjInfoChildrenMethodIdx,
 XOTclObjInfoClassMethodIdx,
 XOTclObjInfoFilterMethodIdx,
 XOTclObjInfoFilterguardMethodIdx,
 XOTclObjInfoForwardMethodIdx,
 XOTclObjInfoHasnamespaceMethodIdx,
 XOTclObjInfoMethodMethodIdx,
 XOTclObjInfoMethodsMethodIdx,
 XOTclObjInfoMixinMethodIdx,
 XOTclObjInfoMixinguardMethodIdx,
 XOTclObjInfoParentMethodIdx,
 XOTclObjInfoPrecedenceMethodIdx,
 XOTclObjInfoSlotObjectsMethodIdx,
 XOTclObjInfoVarsMethodIdx,
 XOTclOAutonameMethodIdx,
 XOTclOCleanupMethodIdx,
 XOTclOConfigureMethodIdx,
 XOTclODestroyMethodIdx,
 XOTclOExistsMethodIdx,
 XOTclOFilterGuardMethodIdx,
 XOTclOFilterSearchMethodIdx,
 XOTclOInstVarMethodIdx,
 XOTclOMixinGuardMethodIdx,
 XOTclONextMethodIdx,
 XOTclONoinitMethodIdx,
 XOTclORequireNamespaceMethodIdx,
 XOTclOResidualargsMethodIdx,
 XOTclOUplevelMethodIdx,
 XOTclOUpvarMethodIdx,
 XOTclOVolatileMethodIdx,
 XOTclOVwaitMethodIdx,
 XOTclAliasCmdIdx,
 XOTclAssertionCmdIdx,
 XOTclConfigureCmdIdx,
 XOTclCreateObjectSystemCmdIdx,
 XOTclDeprecatedCmdIdx,
 XOTclDispatchCmdIdx,
 XOTclDotCmdIdx,
 XOTclExistsCmdIdx,
 XOTclFinalizeObjCmdIdx,
 XOTclForwardCmdIdx,
 XOTclGetSelfObjCmdIdx,
 XOTclImportvarCmdIdx,
 XOTclInterpObjCmdIdx,
 XOTclIsCmdIdx,
 XOTclMethodCmdIdx,
 XOTclMethodPropertyCmdIdx,
 XOTclMyCmdIdx,
 XOTclNSCopyCmdsIdx,
 XOTclNSCopyVarsIdx,
 XOTclQualifyObjCmdIdx,
 XOTclRelationCmdIdx,
 XOTclSetInstvarCmdIdx,
 XOTclSetterCmdIdx
} XOTclMethods;


static int
XOTclCheckBooleanArgsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclCheckBooleanArgsIdx].paramDefs, 
                     method_definitions[XOTclCheckBooleanArgsIdx].nrParameters, 
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclCheckRequiredArgsIdx].paramDefs, 
                     method_definitions[XOTclCheckRequiredArgsIdx].nrParameters, 
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
  if (ArgumentParse(interp, objc, objv, (XOTclObject *) cl, objv[0], 
                     method_definitions[XOTclCAllocMethodIdx].paramDefs, 
                     method_definitions[XOTclCAllocMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCAllocMethod(interp, cl, name);

  }
}

static int
XOTclCCreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (ArgumentParse(interp, objc, objv, (XOTclObject *) cl, objv[0], 
                     method_definitions[XOTclCCreateMethodIdx].paramDefs, 
                     method_definitions[XOTclCCreateMethodIdx].nrParameters, 
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
  if (ArgumentParse(interp, objc, objv, (XOTclObject *) cl, objv[0], 
                     method_definitions[XOTclCDeallocMethodIdx].paramDefs, 
                     method_definitions[XOTclCDeallocMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *object = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCDeallocMethod(interp, cl, object);

  }
}

static int
XOTclCFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (ArgumentParse(interp, objc, objv, (XOTclObject *) cl, objv[0], 
                     method_definitions[XOTclCFilterGuardMethodIdx].paramDefs, 
                     method_definitions[XOTclCFilterGuardMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *filter = (char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclCFilterGuardMethod(interp, cl, filter, guard);

  }
}

static int
XOTclCInvalidateObjectParameterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (ArgumentParse(interp, objc, objv, (XOTclObject *) cl, objv[0], 
                     method_definitions[XOTclCInvalidateObjectParameterMethodIdx].paramDefs, 
                     method_definitions[XOTclCInvalidateObjectParameterMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclCInvalidateObjectParameterMethod(interp, cl);

  }
}

static int
XOTclCMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (ArgumentParse(interp, objc, objv, (XOTclObject *) cl, objv[0], 
                     method_definitions[XOTclCMixinGuardMethodIdx].paramDefs, 
                     method_definitions[XOTclCMixinGuardMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *mixin = (char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclCMixinGuardMethod(interp, cl, mixin, guard);

  }
}

static int
XOTclCNewMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclClass *cl =  XOTclObjectToClass(clientData);
  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");
  if (ArgumentParse(interp, objc, objv, (XOTclObject *) cl, objv[0], 
                     method_definitions[XOTclCNewMethodIdx].paramDefs, 
                     method_definitions[XOTclCNewMethodIdx].nrParameters, 
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
  if (ArgumentParse(interp, objc, objv, (XOTclObject *) cl, objv[0], 
                     method_definitions[XOTclCRecreateMethodIdx].paramDefs, 
                     method_definitions[XOTclCRecreateMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclCRecreateMethod(interp, cl, name, objc, objv);

  }
}

static int
XOTclClassInfoClassMixinOfMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoClassMixinOfMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoClassMixinOfMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )PTR2INT(pc.clientData[1]);
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
    returnCode = XOTclClassInfoClassMixinOfMethod(interp, class, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
XOTclClassInfoFilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoFilterMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoFilterMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withGuards = (int )PTR2INT(pc.clientData[1]);
    char *pattern = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclClassInfoFilterMethod(interp, class, withGuards, pattern);

  }
}

static int
XOTclClassInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoFilterguardMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoFilterguardMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *filter = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoFilterguardMethod(interp, class, filter);

  }
}

static int
XOTclClassInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoForwardMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoForwardMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withDefinition = (int )PTR2INT(pc.clientData[1]);
    char *name = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclClassInfoForwardMethod(interp, class, withDefinition, name);

  }
}

static int
XOTclClassInfoHeritageMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoHeritageMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoHeritageMethodIdx].nrParameters, 
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoInstancesMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoInstancesMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )PTR2INT(pc.clientData[1]);
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
XOTclClassInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoMethodMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoMethodMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int infomethodsubcmd = (int )PTR2INT(pc.clientData[1]);
    char *name = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclClassInfoMethodMethod(interp, class, infomethodsubcmd, name);

  }
}

static int
XOTclClassInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoMethodsMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoMethodsMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *object = (XOTclClass *)pc.clientData[0];
    int withMethodtype = (int )PTR2INT(pc.clientData[1]);
    int withCallprotection = (int )PTR2INT(pc.clientData[2]);
    int withNomixins = (int )PTR2INT(pc.clientData[3]);
    int withIncontext = (int )PTR2INT(pc.clientData[4]);
    char *pattern = (char *)pc.clientData[5];

    parseContextRelease(&pc);
    return XOTclClassInfoMethodsMethod(interp, object, withMethodtype, withCallprotection, withNomixins, withIncontext, pattern);

  }
}

static int
XOTclClassInfoMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoMixinMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoMixinMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )PTR2INT(pc.clientData[1]);
    int withGuards = (int )PTR2INT(pc.clientData[2]);
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
    returnCode = XOTclClassInfoMixinMethod(interp, class, withClosure, withGuards, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
XOTclClassInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoMixinguardMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoMixinguardMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    char *mixin = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclClassInfoMixinguardMethod(interp, class, mixin);

  }
}

static int
XOTclClassInfoObjectMixinOfMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoObjectMixinOfMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoObjectMixinOfMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )PTR2INT(pc.clientData[1]);
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
    returnCode = XOTclClassInfoObjectMixinOfMethod(interp, class, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
XOTclClassInfoParameterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoParameterMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoParameterMethodIdx].nrParameters, 
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoSlotsMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoSlotsMethodIdx].nrParameters, 
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoSubclassMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoSubclassMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )PTR2INT(pc.clientData[1]);
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclClassInfoSuperclassMethodIdx].paramDefs, 
                     method_definitions[XOTclClassInfoSuperclassMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclClass *class = (XOTclClass *)pc.clientData[0];
    int withClosure = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclClassInfoSuperclassMethod(interp, class, withClosure, pattern);

  }
}

static int
XOTclObjInfoCallableMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoCallableMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoCallableMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withWhich = (int )PTR2INT(pc.clientData[1]);
    int withMethodtype = (int )PTR2INT(pc.clientData[2]);
    int withCallprotection = (int )PTR2INT(pc.clientData[3]);
    int withApplication = (int )PTR2INT(pc.clientData[4]);
    int withNomixins = (int )PTR2INT(pc.clientData[5]);
    int withIncontext = (int )PTR2INT(pc.clientData[6]);
    char *pattern = (char *)pc.clientData[7];

    parseContextRelease(&pc);
    return XOTclObjInfoCallableMethod(interp, object, withWhich, withMethodtype, withCallprotection, withApplication, withNomixins, withIncontext, pattern);

  }
}

static int
XOTclObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoChildrenMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoChildrenMethodIdx].nrParameters, 
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoClassMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoClassMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclObjInfoClassMethod(interp, object);

  }
}

static int
XOTclObjInfoFilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoFilterMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoFilterMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withOrder = (int )PTR2INT(pc.clientData[1]);
    int withGuards = (int )PTR2INT(pc.clientData[2]);
    char *pattern = (char *)pc.clientData[3];

    parseContextRelease(&pc);
    return XOTclObjInfoFilterMethod(interp, object, withOrder, withGuards, pattern);

  }
}

static int
XOTclObjInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoFilterguardMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoFilterguardMethodIdx].nrParameters, 
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoForwardMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoForwardMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withDefinition = (int )PTR2INT(pc.clientData[1]);
    char *name = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclObjInfoForwardMethod(interp, object, withDefinition, name);

  }
}

static int
XOTclObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoHasnamespaceMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoHasnamespaceMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclObjInfoHasnamespaceMethod(interp, object);

  }
}

static int
XOTclObjInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoMethodMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoMethodMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int infomethodsubcmd = (int )PTR2INT(pc.clientData[1]);
    char *name = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclObjInfoMethodMethod(interp, object, infomethodsubcmd, name);

  }
}

static int
XOTclObjInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoMethodsMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoMethodsMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withMethodtype = (int )PTR2INT(pc.clientData[1]);
    int withCallprotection = (int )PTR2INT(pc.clientData[2]);
    int withNomixins = (int )PTR2INT(pc.clientData[3]);
    int withIncontext = (int )PTR2INT(pc.clientData[4]);
    char *pattern = (char *)pc.clientData[5];

    parseContextRelease(&pc);
    return XOTclObjInfoMethodsMethod(interp, object, withMethodtype, withCallprotection, withNomixins, withIncontext, pattern);

  }
}

static int
XOTclObjInfoMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoMixinMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoMixinMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withGuards = (int )PTR2INT(pc.clientData[1]);
    int withOrder = (int )PTR2INT(pc.clientData[2]);
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoMixinguardMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoMixinguardMethodIdx].nrParameters, 
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
XOTclObjInfoParentMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoParentMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoParentMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclObjInfoParentMethod(interp, object);

  }
}

static int
XOTclObjInfoPrecedenceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoPrecedenceMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoPrecedenceMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withIntrinsic = (int )PTR2INT(pc.clientData[1]);
    char *pattern = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclObjInfoPrecedenceMethod(interp, object, withIntrinsic, pattern);

  }
}

static int
XOTclObjInfoSlotObjectsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoSlotObjectsMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoSlotObjectsMethodIdx].nrParameters, 
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclObjInfoVarsMethodIdx].paramDefs, 
                     method_definitions[XOTclObjInfoVarsMethodIdx].nrParameters, 
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
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclOAutonameMethodIdx].paramDefs, 
                     method_definitions[XOTclOAutonameMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withInstance = (int )PTR2INT(pc.clientData[0]);
    int withReset = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclOAutonameMethod(interp, obj, withInstance, withReset, name);

  }
}

static int
XOTclOCleanupMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclOCleanupMethodIdx].paramDefs, 
                     method_definitions[XOTclOCleanupMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclOCleanupMethod(interp, obj);

  }
}

static int
XOTclOConfigureMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    

    return XOTclOConfigureMethod(interp, obj, objc, objv);

}

static int
XOTclODestroyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclODestroyMethodIdx].paramDefs, 
                     method_definitions[XOTclODestroyMethodIdx].nrParameters, 
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
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclOExistsMethodIdx].paramDefs, 
                     method_definitions[XOTclOExistsMethodIdx].nrParameters, 
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
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclOFilterGuardMethodIdx].paramDefs, 
                     method_definitions[XOTclOFilterGuardMethodIdx].nrParameters, 
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
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclOFilterSearchMethodIdx].paramDefs, 
                     method_definitions[XOTclOFilterSearchMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *filter = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclOFilterSearchMethod(interp, obj, filter);

  }
}

static int
XOTclOInstVarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    

    return XOTclOInstVarMethod(interp, obj, objc, objv);

}

static int
XOTclOMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclOMixinGuardMethodIdx].paramDefs, 
                     method_definitions[XOTclOMixinGuardMethodIdx].nrParameters, 
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
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    

    return XOTclONextMethod(interp, obj, objc, objv);

}

static int
XOTclONoinitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclONoinitMethodIdx].paramDefs, 
                     method_definitions[XOTclONoinitMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclONoinitMethod(interp, obj);

  }
}

static int
XOTclORequireNamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclORequireNamespaceMethodIdx].paramDefs, 
                     method_definitions[XOTclORequireNamespaceMethodIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclORequireNamespaceMethod(interp, obj);

  }
}

static int
XOTclOResidualargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    

    return XOTclOResidualargsMethod(interp, obj, objc, objv);

}

static int
XOTclOUplevelMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    

    return XOTclOUplevelMethod(interp, obj, objc, objv);

}

static int
XOTclOUpvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
    

    return XOTclOUpvarMethod(interp, obj, objc, objv);

}

static int
XOTclOVolatileMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  XOTclObject *obj =  (XOTclObject *)clientData;
  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclOVolatileMethodIdx].paramDefs, 
                     method_definitions[XOTclOVolatileMethodIdx].nrParameters, 
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
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[XOTclOVwaitMethodIdx].paramDefs, 
                     method_definitions[XOTclOVwaitMethodIdx].nrParameters, 
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

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclAliasCmdIdx].paramDefs, 
                     method_definitions[XOTclAliasCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withPer_object = (int )PTR2INT(pc.clientData[1]);
    char *methodName = (char *)pc.clientData[2];
    int withNonleaf = (int )PTR2INT(pc.clientData[3]);
    int withObjscope = (int )PTR2INT(pc.clientData[4]);
    Tcl_Obj *cmdName = (Tcl_Obj *)pc.clientData[5];

    parseContextRelease(&pc);
    return XOTclAliasCmd(interp, object, withPer_object, methodName, withNonleaf, withObjscope, cmdName);

  }
}

static int
XOTclAssertionCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclAssertionCmdIdx].paramDefs, 
                     method_definitions[XOTclAssertionCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int assertionsubcmd = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *arg = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclAssertionCmd(interp, object, assertionsubcmd, arg);

  }
}

static int
XOTclConfigureCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclConfigureCmdIdx].paramDefs, 
                     method_definitions[XOTclConfigureCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int configureoption = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclConfigureCmd(interp, configureoption, value);

  }
}

static int
XOTclCreateObjectSystemCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclCreateObjectSystemCmdIdx].paramDefs, 
                     method_definitions[XOTclCreateObjectSystemCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *rootClass = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *rootMetaClass = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclCreateObjectSystemCmd(interp, rootClass, rootMetaClass);

  }
}

static int
XOTclDeprecatedCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclDeprecatedCmdIdx].paramDefs, 
                     method_definitions[XOTclDeprecatedCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *what = (char *)pc.clientData[0];
    char *oldCmd = (char *)pc.clientData[1];
    char *newCmd = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclDeprecatedCmd(interp, what, oldCmd, newCmd);

  }
}

static int
XOTclDispatchCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclDispatchCmdIdx].paramDefs, 
                     method_definitions[XOTclDispatchCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withObjscope = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *command = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclDispatchCmd(interp, object, withObjscope, command, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
XOTclDotCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {

    

    return XOTclDotCmd(interp, objc, objv);

}

static int
XOTclExistsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclExistsCmdIdx].paramDefs, 
                     method_definitions[XOTclExistsCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    char *var = (char *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclExistsCmd(interp, object, var);

  }
}

static int
XOTclFinalizeObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclFinalizeObjCmdIdx].paramDefs, 
                     method_definitions[XOTclFinalizeObjCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return XOTclFinalizeObjCmd(interp);

  }
}

static int
XOTclForwardCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclForwardCmdIdx].paramDefs, 
                     method_definitions[XOTclForwardCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withPer_object = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *method = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj *withDefault = (Tcl_Obj *)pc.clientData[3];
    int withEarlybinding = (int )PTR2INT(pc.clientData[4]);
    Tcl_Obj *withMethodprefix = (Tcl_Obj *)pc.clientData[5];
    int withObjscope = (int )PTR2INT(pc.clientData[6]);
    Tcl_Obj *withOnerror = (Tcl_Obj *)pc.clientData[7];
    int withVerbose = (int )PTR2INT(pc.clientData[8]);
    Tcl_Obj *target = (Tcl_Obj *)pc.clientData[9];

    parseContextRelease(&pc);
    return XOTclForwardCmd(interp, object, withPer_object, method, withDefault, withEarlybinding, withMethodprefix, withObjscope, withOnerror, withVerbose, target, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
XOTclGetSelfObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclGetSelfObjCmdIdx].paramDefs, 
                     method_definitions[XOTclGetSelfObjCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int selfoption = (int )PTR2INT(pc.clientData[0]);

    parseContextRelease(&pc);
    return XOTclGetSelfObjCmd(interp, selfoption);

  }
}

static int
XOTclImportvarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclImportvarCmdIdx].paramDefs, 
                     method_definitions[XOTclImportvarCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclImportvarCmd(interp, object, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
XOTclInterpObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclInterpObjCmdIdx].paramDefs, 
                     method_definitions[XOTclInterpObjCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    char *name = (char *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclInterpObjCmd(interp, name, objc, objv);

  }
}

static int
XOTclIsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclIsCmdIdx].paramDefs, 
                     method_definitions[XOTclIsCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *object = (Tcl_Obj *)pc.clientData[0];
    int objectkind = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclIsCmd(interp, object, objectkind, value);

  }
}

static int
XOTclMethodCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclMethodCmdIdx].paramDefs, 
                     method_definitions[XOTclMethodCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withInner_namespace = (int )PTR2INT(pc.clientData[1]);
    int withPer_object = (int )PTR2INT(pc.clientData[2]);
    int withPublic = (int )PTR2INT(pc.clientData[3]);
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[4];
    Tcl_Obj *args = (Tcl_Obj *)pc.clientData[5];
    Tcl_Obj *body = (Tcl_Obj *)pc.clientData[6];
    Tcl_Obj *withPrecondition = (Tcl_Obj *)pc.clientData[7];
    Tcl_Obj *withPostcondition = (Tcl_Obj *)pc.clientData[8];

    parseContextRelease(&pc);
    return XOTclMethodCmd(interp, object, withInner_namespace, withPer_object, withPublic, name, args, body, withPrecondition, withPostcondition);

  }
}

static int
XOTclMethodPropertyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclMethodPropertyCmdIdx].paramDefs, 
                     method_definitions[XOTclMethodPropertyCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withPer_object = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *methodName = (Tcl_Obj *)pc.clientData[2];
    int methodproperty = (int )PTR2INT(pc.clientData[3]);
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[4];

    parseContextRelease(&pc);
    return XOTclMethodPropertyCmd(interp, object, withPer_object, methodName, methodproperty, value);

  }
}

static int
XOTclMyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclMyCmdIdx].paramDefs, 
                     method_definitions[XOTclMyCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withLocal = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *method = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclMyCmd(interp, withLocal, method, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
XOTclNSCopyCmdsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclNSCopyCmdsIdx].paramDefs, 
                     method_definitions[XOTclNSCopyCmdsIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *fromNs = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *toNs = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclNSCopyCmds(interp, fromNs, toNs);

  }
}

static int
XOTclNSCopyVarsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclNSCopyVarsIdx].paramDefs, 
                     method_definitions[XOTclNSCopyVarsIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *fromNs = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *toNs = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return XOTclNSCopyVars(interp, fromNs, toNs);

  }
}

static int
XOTclQualifyObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclQualifyObjCmdIdx].paramDefs, 
                     method_definitions[XOTclQualifyObjCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return XOTclQualifyObjCmd(interp, name);

  }
}

static int
XOTclRelationCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclRelationCmdIdx].paramDefs, 
                     method_definitions[XOTclRelationCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int relationtype = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclRelationCmd(interp, object, relationtype, value);

  }
}

static int
XOTclSetInstvarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclSetInstvarCmdIdx].paramDefs, 
                     method_definitions[XOTclSetInstvarCmdIdx].nrParameters, 
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

static int
XOTclSetterCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[XOTclSetterCmdIdx].paramDefs, 
                     method_definitions[XOTclSetterCmdIdx].nrParameters, 
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    XOTclObject *object = (XOTclObject *)pc.clientData[0];
    int withPer_object = (int )PTR2INT(pc.clientData[1]);
    char *methodName = (char *)pc.clientData[2];

    parseContextRelease(&pc);
    return XOTclSetterCmd(interp, object, withPer_object, methodName);

  }
}

static methodDefinition method_definitions[] = {
{"::xotcl::cmd::ParameterType::type=boolean", XOTclCheckBooleanArgsStub, 2, {
  {"name", 1, 0, convertToString},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::ParameterType::type=required", XOTclCheckRequiredArgsStub, 2, {
  {"name", 1, 0, convertToString},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::alloc", XOTclCAllocMethodStub, 1, {
  {"name", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::create", XOTclCCreateMethodStub, 2, {
  {"name", 1, 0, convertToString},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Class::dealloc", XOTclCDeallocMethodStub, 1, {
  {"object", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::filterguard", XOTclCFilterGuardMethodStub, 2, {
  {"filter", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::__invalidateobjectparameter", XOTclCInvalidateObjectParameterMethodStub, 0, {
  }
},
{"::xotcl::cmd::Class::mixinguard", XOTclCMixinGuardMethodStub, 2, {
  {"mixin", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::xotcl::cmd::Class::new", XOTclCNewMethodStub, 2, {
  {"-childof", 0, 1, convertToObject},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::Class::recreate", XOTclCRecreateMethodStub, 2, {
  {"name", 1, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::cmd::ClassInfo::class-mixin-of", XOTclClassInfoClassMixinOfMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-closure", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::xotcl::cmd::ClassInfo::filter", XOTclClassInfoFilterMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-guards", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::filterguard", XOTclClassInfoFilterguardMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"filter", 1, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::forward", XOTclClassInfoForwardMethodStub, 3, {
  {"class", 1, 0, convertToClass},
  {"-definition", 0, 0, convertToString},
  {"name", 0, 0, convertToString}}
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
{"::xotcl::cmd::ClassInfo::method", XOTclClassInfoMethodMethodStub, 3, {
  {"class", 0, 0, convertToClass},
  {"infomethodsubcmd", 0, 0, convertToInfomethodsubcmd},
  {"name", 0, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::methods", XOTclClassInfoMethodsMethodStub, 6, {
  {"object", 0, 0, convertToClass},
  {"-methodtype", 0, 1, convertToMethodtype},
  {"-callprotection", 0, 1, convertToCallprotection},
  {"-nomixins", 0, 0, convertToString},
  {"-incontext", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::mixin", XOTclClassInfoMixinMethodStub, 4, {
  {"class", 1, 0, convertToClass},
  {"-closure", 0, 0, convertToString},
  {"-guards", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::xotcl::cmd::ClassInfo::mixinguard", XOTclClassInfoMixinguardMethodStub, 2, {
  {"class", 1, 0, convertToClass},
  {"mixin", 1, 0, convertToString}}
},
{"::xotcl::cmd::ClassInfo::object-mixin-of", XOTclClassInfoObjectMixinOfMethodStub, 3, {
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
{"::xotcl::cmd::ObjectInfo::callable", XOTclObjInfoCallableMethodStub, 8, {
  {"object", 0, 0, convertToObject},
  {"-which", 0, 0, convertToString},
  {"-methodtype", 0, 1, convertToMethodtype},
  {"-callprotection", 0, 1, convertToCallprotection},
  {"-application", 0, 0, convertToString},
  {"-nomixins", 0, 0, convertToString},
  {"-incontext", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::children", XOTclObjInfoChildrenMethodStub, 2, {
  {"object", 1, 0, convertToObject},
  {"pattern", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::class", XOTclObjInfoClassMethodStub, 1, {
  {"object", 1, 0, convertToObject}}
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
  {"name", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::hasnamespace", XOTclObjInfoHasnamespaceMethodStub, 1, {
  {"object", 1, 0, convertToObject}}
},
{"::xotcl::cmd::ObjectInfo::method", XOTclObjInfoMethodMethodStub, 3, {
  {"object", 0, 0, convertToObject},
  {"infomethodsubcmd", 0, 0, convertToInfomethodsubcmd},
  {"name", 0, 0, convertToString}}
},
{"::xotcl::cmd::ObjectInfo::methods", XOTclObjInfoMethodsMethodStub, 6, {
  {"object", 0, 0, convertToObject},
  {"-methodtype", 0, 1, convertToMethodtype},
  {"-callprotection", 0, 1, convertToCallprotection},
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
{"::xotcl::cmd::ObjectInfo::parent", XOTclObjInfoParentMethodStub, 1, {
  {"object", 1, 0, convertToObject}}
},
{"::xotcl::cmd::ObjectInfo::precedence", XOTclObjInfoPrecedenceMethodStub, 3, {
  {"object", 1, 0, convertToObject},
  {"-intrinsic", 0, 0, convertToString},
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
{"::xotcl::cmd::Object::instvar", XOTclOInstVarMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
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
{"::xotcl::cmd::Object::requireNamespace", XOTclORequireNamespaceMethodStub, 0, {
  }
},
{"::xotcl::cmd::Object::residualargs", XOTclOResidualargsMethodStub, 1, {
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
  {"object", 0, 0, convertToObject},
  {"-per-object", 0, 0, convertToString},
  {"methodName", 0, 0, convertToString},
  {"-nonleaf", 0, 0, convertToString},
  {"-objscope", 0, 0, convertToString},
  {"cmdName", 1, 0, convertToTclobj}}
},
{"::xotcl::assertion", XOTclAssertionCmdStub, 3, {
  {"object", 0, 0, convertToObject},
  {"assertionsubcmd", 1, 0, convertToAssertionsubcmd},
  {"arg", 0, 0, convertToTclobj}}
},
{"::xotcl::configure", XOTclConfigureCmdStub, 2, {
  {"configureoption", 1, 0, convertToConfigureoption},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::createobjectsystem", XOTclCreateObjectSystemCmdStub, 2, {
  {"rootClass", 1, 0, convertToTclobj},
  {"rootMetaClass", 1, 0, convertToTclobj}}
},
{"::xotcl::deprecated", XOTclDeprecatedCmdStub, 3, {
  {"what", 1, 0, convertToString},
  {"oldCmd", 1, 0, convertToString},
  {"newCmd", 0, 0, convertToString}}
},
{"::xotcl::dispatch", XOTclDispatchCmdStub, 4, {
  {"object", 1, 0, convertToObject},
  {"-objscope", 0, 0, convertToString},
  {"command", 1, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::dot", XOTclDotCmdStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::exists", XOTclExistsCmdStub, 2, {
  {"object", 1, 0, convertToObject},
  {"var", 1, 0, convertToString}}
},
{"::xotcl::finalize", XOTclFinalizeObjCmdStub, 0, {
  }
},
{"::xotcl::forward", XOTclForwardCmdStub, 11, {
  {"object", 1, 0, convertToObject},
  {"-per-object", 0, 0, convertToString},
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
{"::xotcl::self", XOTclGetSelfObjCmdStub, 1, {
  {"selfoption", 0, 0, convertToSelfoption}}
},
{"::xotcl::importvar", XOTclImportvarCmdStub, 2, {
  {"object", 0, 0, convertToObject},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::interp", XOTclInterpObjCmdStub, 2, {
  {"name", 0, 0, convertToString},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::is", XOTclIsCmdStub, 3, {
  {"object", 1, 0, convertToTclobj},
  {"objectkind", 0, 0, convertToObjectkind},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::method", XOTclMethodCmdStub, 9, {
  {"object", 1, 0, convertToObject},
  {"-inner-namespace", 0, 0, convertToString},
  {"-per-object", 0, 0, convertToString},
  {"-public", 0, 0, convertToString},
  {"name", 1, 0, convertToTclobj},
  {"args", 1, 0, convertToTclobj},
  {"body", 1, 0, convertToTclobj},
  {"-precondition", 0, 1, convertToTclobj},
  {"-postcondition", 0, 1, convertToTclobj}}
},
{"::xotcl::methodproperty", XOTclMethodPropertyCmdStub, 5, {
  {"object", 1, 0, convertToObject},
  {"-per-object", 0, 0, convertToString},
  {"methodName", 1, 0, convertToTclobj},
  {"methodproperty", 1, 0, convertToMethodproperty},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::my", XOTclMyCmdStub, 3, {
  {"-local", 0, 0, convertToString},
  {"method", 1, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::xotcl::namespace_copycmds", XOTclNSCopyCmdsStub, 2, {
  {"fromNs", 1, 0, convertToTclobj},
  {"toNs", 1, 0, convertToTclobj}}
},
{"::xotcl::namespace_copyvars", XOTclNSCopyVarsStub, 2, {
  {"fromNs", 1, 0, convertToTclobj},
  {"toNs", 1, 0, convertToTclobj}}
},
{"::xotcl::__qualify", XOTclQualifyObjCmdStub, 1, {
  {"name", 1, 0, convertToTclobj}}
},
{"::xotcl::relation", XOTclRelationCmdStub, 3, {
  {"object", 0, 0, convertToObject},
  {"relationtype", 1, 0, convertToRelationtype},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::setinstvar", XOTclSetInstvarCmdStub, 3, {
  {"object", 1, 0, convertToObject},
  {"variable", 1, 0, convertToTclobj},
  {"value", 0, 0, convertToTclobj}}
},
{"::xotcl::setter", XOTclSetterCmdStub, 3, {
  {"object", 1, 0, convertToObject},
  {"-per-object", 0, 0, convertToString},
  {"methodName", 1, 0, convertToString}}
}
};

