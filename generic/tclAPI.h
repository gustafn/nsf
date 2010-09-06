
static int convertToInfomethodsubcmd(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"args", "body", "definition", "handle", "parameter", "parametersyntax", "type", "precondition", "postcondition", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "infomethodsubcmd", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum InfomethodsubcmdIdx {InfomethodsubcmdNULL, InfomethodsubcmdArgsIdx, InfomethodsubcmdBodyIdx, InfomethodsubcmdDefinitionIdx, InfomethodsubcmdHandleIdx, InfomethodsubcmdParameterIdx, InfomethodsubcmdParametersyntaxIdx, InfomethodsubcmdTypeIdx, InfomethodsubcmdPreconditionIdx, InfomethodsubcmdPostconditionIdx};
  
static int convertToMethodtype(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"all", "scripted", "builtin", "alias", "forwarder", "object", "setter", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-methodtype", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum MethodtypeIdx {MethodtypeNULL, MethodtypeAllIdx, MethodtypeScriptedIdx, MethodtypeBuiltinIdx, MethodtypeAliasIdx, MethodtypeForwarderIdx, MethodtypeObjectIdx, MethodtypeSetterIdx};
  
static int convertToCallprotection(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"all", "protected", "public", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-callprotection", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum CallprotectionIdx {CallprotectionNULL, CallprotectionAllIdx, CallprotectionProtectedIdx, CallprotectionPublicIdx};
  
static int convertToScope(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"all", "class", "object", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-scope", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum ScopeIdx {ScopeNULL, ScopeAllIdx, ScopeClassIdx, ScopeObjectIdx};
  
static int convertToAssertionsubcmd(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"check", "object-invar", "class-invar", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "assertionsubcmd", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum AssertionsubcmdIdx {AssertionsubcmdNULL, AssertionsubcmdCheckIdx, AssertionsubcmdObject_invarIdx, AssertionsubcmdClass_invarIdx};
  
static int convertToConfigureoption(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"filter", "softrecreate", "objectsystems", "keepinitcmd", "checkresults", "checkarguments", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "configureoption", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum ConfigureoptionIdx {ConfigureoptionNULL, ConfigureoptionFilterIdx, ConfigureoptionSoftrecreateIdx, ConfigureoptionObjectsystemsIdx, ConfigureoptionKeepinitcmdIdx, ConfigureoptionCheckresultsIdx, ConfigureoptionCheckargumentsIdx};
  
static int convertToCurrentoption(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"proc", "method", "object", "class", "activelevel", "args", "activemixin", "calledproc", "calledmethod", "calledclass", "callingproc", "callingmethod", "callingclass", "callinglevel", "callingobject", "filterreg", "isnextcall", "next", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "currentoption", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum CurrentoptionIdx {CurrentoptionNULL, CurrentoptionProcIdx, CurrentoptionMethodIdx, CurrentoptionObjectIdx, CurrentoptionClassIdx, CurrentoptionActivelevelIdx, CurrentoptionArgsIdx, CurrentoptionActivemixinIdx, CurrentoptionCalledprocIdx, CurrentoptionCalledmethodIdx, CurrentoptionCalledclassIdx, CurrentoptionCallingprocIdx, CurrentoptionCallingmethodIdx, CurrentoptionCallingclassIdx, CurrentoptionCallinglevelIdx, CurrentoptionCallingobjectIdx, CurrentoptionFilterregIdx, CurrentoptionIsnextcallIdx, CurrentoptionNextIdx};
  
static int convertToMethodproperty(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"class-only", "protected", "redefine-protected", "returns", "slotobj", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "methodproperty", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum MethodpropertyIdx {MethodpropertyNULL, MethodpropertyClass_onlyIdx, MethodpropertyProtectedIdx, MethodpropertyRedefine_protectedIdx, MethodpropertyReturnsIdx, MethodpropertySlotobjIdx};
  
static int convertToRelationtype(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"object-mixin", "class-mixin", "object-filter", "class-filter", "class", "superclass", "rootclass", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "relationtype", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum RelationtypeIdx {RelationtypeNULL, RelationtypeObject_mixinIdx, RelationtypeClass_mixinIdx, RelationtypeObject_filterIdx, RelationtypeClass_filterIdx, RelationtypeClassIdx, RelationtypeSuperclassIdx, RelationtypeRootclassIdx};
  
static int convertToInfocallablesubcmd(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"filter", "method", "methods", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "infocallablesubcmd", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum InfocallablesubcmdIdx {InfocallablesubcmdNULL, InfocallablesubcmdFilterIdx, InfocallablesubcmdMethodIdx, InfocallablesubcmdMethodsIdx};
  
static int convertToObjectkind(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"class", "baseclass", "metaclass", NULL};
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "objectkind", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum ObjectkindIdx {ObjectkindNULL, ObjectkindClassIdx, ObjectkindBaseclassIdx, ObjectkindMetaclassIdx};
  

typedef struct {
  CONST char *methodName;
  Tcl_ObjCmdProc *proc;
  int nrParameters;
  NsfParam paramDefs[12];
} methodDefinition;

static int ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], 
                         NsfObject *obj, Tcl_Obj *procName,
                         NsfParam CONST *paramPtr, int nrParameters, int doCheck,
			 parseContext *pc);

static int getMatchObject(Tcl_Interp *interp, Tcl_Obj *patternObj, Tcl_Obj *origObj,
			  NsfObject **matchObject, CONST char **pattern);

/* just to define the symbol */
static methodDefinition method_definitions[];
  
static CONST char *method_command_namespace_names[] = {
  "::nsf::cmd::ObjectInfo",
  "::nsf::cmd::Object",
  "::nsf::cmd::ParameterType",
  "::nsf::cmd::ClassInfo",
  "::nsf::cmd::Class"
};
static int NsfCAllocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCCreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCDeallocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCNewMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCRecreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoFiltermethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoHeritageMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoInstancesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoMixinOfMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoMixinclassesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoSlotsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoSubclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoSuperclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfAliasCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfAssertionCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfColonCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfConfigureCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCreateObjectSystemCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCurrentCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfDeprecatedCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfDispatchCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfExistsVarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfFinalizeObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfForwardCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfImportvarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfInterpObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfInvalidateObjectParameterCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfIsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfIsObjectCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMethodCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMethodPropertyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfNSCopyCmdsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfNSCopyVarsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfQualifyObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfRelationCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfSetVarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfSetterCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOAutonameMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOCleanupMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOConfigureMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfODestroyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOExistsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOInstVarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfONoinitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfORequireNamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOResidualargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOUplevelMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOUpvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOVolatileMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOVwaitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoCallableMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoFiltermethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoHasMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoHasTypeMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoIsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoMixinclassesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoParentMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoPrecedenceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoSlotObjectsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoVarsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

static int NsfCAllocMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *name);
static int NsfCCreateMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *name, int objc, Tcl_Obj *CONST objv[]);
static int NsfCDeallocMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *object);
static int NsfCFilterGuardMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *filter, Tcl_Obj *guard);
static int NsfCMixinGuardMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *mixin, Tcl_Obj *guard);
static int NsfCNewMethod(Tcl_Interp *interp, NsfClass *cl, NsfObject *withChildof, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfCRecreateMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *name, int objc, Tcl_Obj *CONST objv[]);
static int NsfClassInfoFilterguardMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *filter);
static int NsfClassInfoFiltermethodsMethod(Tcl_Interp *interp, NsfClass *cl, int withGuards, CONST char *pattern);
static int NsfClassInfoForwardMethod(Tcl_Interp *interp, NsfClass *cl, int withDefinition, CONST char *name);
static int NsfClassInfoHeritageMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *pattern);
static int NsfClassInfoInstancesMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, CONST char *patternString, NsfObject *patternObj);
static int NsfClassInfoMethodMethod(Tcl_Interp *interp, NsfClass *cl, int infomethodsubcmd, CONST char *name);
static int NsfClassInfoMethodsMethod(Tcl_Interp *interp, NsfClass *cl, int withMethodtype, int withCallprotection, int withNomixins, int withIncontext, CONST char *pattern);
static int NsfClassInfoMixinOfMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, int withScope, CONST char *patternString, NsfObject *patternObj);
static int NsfClassInfoMixinclassesMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, int withGuards, CONST char *patternString, NsfObject *patternObj);
static int NsfClassInfoMixinguardMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *mixin);
static int NsfClassInfoSlotsMethod(Tcl_Interp *interp, NsfClass *cl);
static int NsfClassInfoSubclassMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, CONST char *patternString, NsfObject *patternObj);
static int NsfClassInfoSuperclassMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, Tcl_Obj *pattern);
static int NsfAliasCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, CONST char *methodName, int withNonleaf, int withObjscope, Tcl_Obj *cmdName);
static int NsfAssertionCmd(Tcl_Interp *interp, NsfObject *object, int assertionsubcmd, Tcl_Obj *arg);
static int NsfColonCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int NsfConfigureCmd(Tcl_Interp *interp, int configureoption, Tcl_Obj *value);
static int NsfCreateObjectSystemCmd(Tcl_Interp *interp, Tcl_Obj *rootClass, Tcl_Obj *rootMetaClass, Tcl_Obj *systemMethods);
static int NsfCurrentCmd(Tcl_Interp *interp, int currentoption);
static int NsfDeprecatedCmd(Tcl_Interp *interp, CONST char *what, CONST char *oldCmd, CONST char *newCmd);
static int NsfDispatchCmd(Tcl_Interp *interp, NsfObject *object, int withObjscope, Tcl_Obj *command, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfExistsVarCmd(Tcl_Interp *interp, NsfObject *object, CONST char *var);
static int NsfFinalizeObjCmd(Tcl_Interp *interp);
static int NsfForwardCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, Tcl_Obj *method, Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix, int withObjscope, Tcl_Obj *withOnerror, int withVerbose, Tcl_Obj *target, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfImportvarCmd(Tcl_Interp *interp, NsfObject *object, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfInterpObjCmd(Tcl_Interp *interp, CONST char *name, int objc, Tcl_Obj *CONST objv[]);
static int NsfInvalidateObjectParameterCmd(Tcl_Interp *interp, NsfClass *class);
static int NsfIsCmd(Tcl_Interp *interp, int withComplain, Tcl_Obj *constraint, Tcl_Obj *value);
static int NsfIsObjectCmd(Tcl_Interp *interp, Tcl_Obj *object);
static int NsfMethodCmd(Tcl_Interp *interp, NsfObject *object, int withInner_namespace, int withPer_object, int withPublic, Tcl_Obj *name, Tcl_Obj *args, Tcl_Obj *body, Tcl_Obj *withPrecondition, Tcl_Obj *withPostcondition);
static int NsfMethodPropertyCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, Tcl_Obj *methodName, int methodproperty, Tcl_Obj *value);
static int NsfMyCmd(Tcl_Interp *interp, int withLocal, Tcl_Obj *method, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfNSCopyCmds(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs);
static int NsfNSCopyVars(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs);
static int NsfQualifyObjCmd(Tcl_Interp *interp, Tcl_Obj *name);
static int NsfRelationCmd(Tcl_Interp *interp, NsfObject *object, int relationtype, Tcl_Obj *value);
static int NsfSetVarCmd(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *variable, Tcl_Obj *value);
static int NsfSetterCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, Tcl_Obj *parameter);
static int NsfOAutonameMethod(Tcl_Interp *interp, NsfObject *obj, int withInstance, int withReset, Tcl_Obj *name);
static int NsfOCleanupMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfOConfigureMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfODestroyMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfOExistsMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *var);
static int NsfOFilterGuardMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *filter, Tcl_Obj *guard);
static int NsfOInstVarMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfOMixinGuardMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *mixin, Tcl_Obj *guard);
static int NsfONoinitMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfORequireNamespaceMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfOResidualargsMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfOUplevelMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfOUpvarMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfOVolatileMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfOVwaitMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *varname);
static int NsfObjInfoCallableMethod(Tcl_Interp *interp, NsfObject *obj, int infocallablesubcmd, int withMethodtype, int withCallprotection, int withApplication, int withNomixins, int withIncontext, CONST char *pattern);
static int NsfObjInfoChildrenMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *pattern);
static int NsfObjInfoClassMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfObjInfoFilterguardMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *filter);
static int NsfObjInfoFiltermethodsMethod(Tcl_Interp *interp, NsfObject *obj, int withGuards, int withOrder, CONST char *pattern);
static int NsfObjInfoForwardMethod(Tcl_Interp *interp, NsfObject *obj, int withDefinition, CONST char *name);
static int NsfObjInfoHasMixinMethod(Tcl_Interp *interp, NsfObject *obj, NsfClass *class);
static int NsfObjInfoHasTypeMethod(Tcl_Interp *interp, NsfObject *obj, NsfClass *class);
static int NsfObjInfoHasnamespaceMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfObjInfoIsMethod(Tcl_Interp *interp, NsfObject *obj, int objectkind);
static int NsfObjInfoMethodMethod(Tcl_Interp *interp, NsfObject *obj, int infomethodsubcmd, CONST char *name);
static int NsfObjInfoMethodsMethod(Tcl_Interp *interp, NsfObject *obj, int withMethodtype, int withCallprotection, int withNomixins, int withIncontext, CONST char *pattern);
static int NsfObjInfoMixinclassesMethod(Tcl_Interp *interp, NsfObject *obj, int withGuards, int withOrder, CONST char *patternString, NsfObject *patternObj);
static int NsfObjInfoMixinguardMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *mixin);
static int NsfObjInfoParentMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfObjInfoPrecedenceMethod(Tcl_Interp *interp, NsfObject *obj, int withIntrinsic, CONST char *pattern);
static int NsfObjInfoSlotObjectsMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *pattern);
static int NsfObjInfoVarsMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *pattern);

enum {
 NsfCAllocMethodIdx,
 NsfCCreateMethodIdx,
 NsfCDeallocMethodIdx,
 NsfCFilterGuardMethodIdx,
 NsfCMixinGuardMethodIdx,
 NsfCNewMethodIdx,
 NsfCRecreateMethodIdx,
 NsfClassInfoFilterguardMethodIdx,
 NsfClassInfoFiltermethodsMethodIdx,
 NsfClassInfoForwardMethodIdx,
 NsfClassInfoHeritageMethodIdx,
 NsfClassInfoInstancesMethodIdx,
 NsfClassInfoMethodMethodIdx,
 NsfClassInfoMethodsMethodIdx,
 NsfClassInfoMixinOfMethodIdx,
 NsfClassInfoMixinclassesMethodIdx,
 NsfClassInfoMixinguardMethodIdx,
 NsfClassInfoSlotsMethodIdx,
 NsfClassInfoSubclassMethodIdx,
 NsfClassInfoSuperclassMethodIdx,
 NsfAliasCmdIdx,
 NsfAssertionCmdIdx,
 NsfColonCmdIdx,
 NsfConfigureCmdIdx,
 NsfCreateObjectSystemCmdIdx,
 NsfCurrentCmdIdx,
 NsfDeprecatedCmdIdx,
 NsfDispatchCmdIdx,
 NsfExistsVarCmdIdx,
 NsfFinalizeObjCmdIdx,
 NsfForwardCmdIdx,
 NsfImportvarCmdIdx,
 NsfInterpObjCmdIdx,
 NsfInvalidateObjectParameterCmdIdx,
 NsfIsCmdIdx,
 NsfIsObjectCmdIdx,
 NsfMethodCmdIdx,
 NsfMethodPropertyCmdIdx,
 NsfMyCmdIdx,
 NsfNSCopyCmdsIdx,
 NsfNSCopyVarsIdx,
 NsfQualifyObjCmdIdx,
 NsfRelationCmdIdx,
 NsfSetVarCmdIdx,
 NsfSetterCmdIdx,
 NsfOAutonameMethodIdx,
 NsfOCleanupMethodIdx,
 NsfOConfigureMethodIdx,
 NsfODestroyMethodIdx,
 NsfOExistsMethodIdx,
 NsfOFilterGuardMethodIdx,
 NsfOInstVarMethodIdx,
 NsfOMixinGuardMethodIdx,
 NsfONoinitMethodIdx,
 NsfORequireNamespaceMethodIdx,
 NsfOResidualargsMethodIdx,
 NsfOUplevelMethodIdx,
 NsfOUpvarMethodIdx,
 NsfOVolatileMethodIdx,
 NsfOVwaitMethodIdx,
 NsfObjInfoCallableMethodIdx,
 NsfObjInfoChildrenMethodIdx,
 NsfObjInfoClassMethodIdx,
 NsfObjInfoFilterguardMethodIdx,
 NsfObjInfoFiltermethodsMethodIdx,
 NsfObjInfoForwardMethodIdx,
 NsfObjInfoHasMixinMethodIdx,
 NsfObjInfoHasTypeMethodIdx,
 NsfObjInfoHasnamespaceMethodIdx,
 NsfObjInfoIsMethodIdx,
 NsfObjInfoMethodMethodIdx,
 NsfObjInfoMethodsMethodIdx,
 NsfObjInfoMixinclassesMethodIdx,
 NsfObjInfoMixinguardMethodIdx,
 NsfObjInfoParentMethodIdx,
 NsfObjInfoPrecedenceMethodIdx,
 NsfObjInfoSlotObjectsMethodIdx,
 NsfObjInfoVarsMethodIdx
} NsfMethods;


static int
NsfCAllocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCAllocMethodIdx].paramDefs, 
                     method_definitions[NsfCAllocMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfCAllocMethod(interp, cl, name);

  }
}

static int
NsfCCreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCCreateMethodIdx].paramDefs, 
                     method_definitions[NsfCCreateMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *name = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfCCreateMethod(interp, cl, name, objc, objv);

  }
}

static int
NsfCDeallocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCDeallocMethodIdx].paramDefs, 
                     method_definitions[NsfCDeallocMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *object = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfCDeallocMethod(interp, cl, object);

  }
}

static int
NsfCFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCFilterGuardMethodIdx].paramDefs, 
                     method_definitions[NsfCFilterGuardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *filter = (CONST char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfCFilterGuardMethod(interp, cl, filter, guard);

  }
}

static int
NsfCMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCMixinGuardMethodIdx].paramDefs, 
                     method_definitions[NsfCMixinGuardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *mixin = (CONST char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfCMixinGuardMethod(interp, cl, mixin, guard);

  }
}

static int
NsfCNewMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCNewMethodIdx].paramDefs, 
                     method_definitions[NsfCNewMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *withChildof = (NsfObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfCNewMethod(interp, cl, withChildof, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfCRecreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCRecreateMethodIdx].paramDefs, 
                     method_definitions[NsfCRecreateMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfCRecreateMethod(interp, cl, name, objc, objv);

  }
}

static int
NsfClassInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoFilterguardMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoFilterguardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *filter = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfClassInfoFilterguardMethod(interp, cl, filter);

  }
}

static int
NsfClassInfoFiltermethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoFiltermethodsMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoFiltermethodsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withGuards = (int )PTR2INT(pc.clientData[0]);
    CONST char *pattern = (CONST char *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfClassInfoFiltermethodsMethod(interp, cl, withGuards, pattern);

  }
}

static int
NsfClassInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoForwardMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoForwardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withDefinition = (int )PTR2INT(pc.clientData[0]);
    CONST char *name = (CONST char *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfClassInfoForwardMethod(interp, cl, withDefinition, name);

  }
}

static int
NsfClassInfoHeritageMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoHeritageMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoHeritageMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *pattern = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfClassInfoHeritageMethod(interp, cl, pattern);

  }
}

static int
NsfClassInfoInstancesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoInstancesMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoInstancesMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withClosure = (int )PTR2INT(pc.clientData[0]);
    CONST char *patternString = NULL;
    NsfObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[1];
    int returnCode;

    if (getMatchObject(interp, pattern, objc>1 ? objv[1] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = NsfClassInfoInstancesMethod(interp, cl, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfClassInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoMethodMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoMethodMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int infomethodsubcmd = (int )PTR2INT(pc.clientData[0]);
    CONST char *name = (CONST char *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfClassInfoMethodMethod(interp, cl, infomethodsubcmd, name);

  }
}

static int
NsfClassInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoMethodsMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoMethodsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withMethodtype = (int )PTR2INT(pc.clientData[0]);
    int withCallprotection = (int )PTR2INT(pc.clientData[1]);
    int withNomixins = (int )PTR2INT(pc.clientData[2]);
    int withIncontext = (int )PTR2INT(pc.clientData[3]);
    CONST char *pattern = (CONST char *)pc.clientData[4];

    parseContextRelease(&pc);
    return NsfClassInfoMethodsMethod(interp, cl, withMethodtype, withCallprotection, withNomixins, withIncontext, pattern);

  }
}

static int
NsfClassInfoMixinOfMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoMixinOfMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoMixinOfMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withClosure = (int )PTR2INT(pc.clientData[0]);
    int withScope = (int )PTR2INT(pc.clientData[1]);
    CONST char *patternString = NULL;
    NsfObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[2];
    int returnCode;

    if (getMatchObject(interp, pattern, objc>2 ? objv[2] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = NsfClassInfoMixinOfMethod(interp, cl, withClosure, withScope, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfClassInfoMixinclassesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoMixinclassesMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoMixinclassesMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withClosure = (int )PTR2INT(pc.clientData[0]);
    int withGuards = (int )PTR2INT(pc.clientData[1]);
    CONST char *patternString = NULL;
    NsfObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[2];
    int returnCode;

    if (getMatchObject(interp, pattern, objc>2 ? objv[2] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = NsfClassInfoMixinclassesMethod(interp, cl, withClosure, withGuards, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfClassInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoMixinguardMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoMixinguardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *mixin = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfClassInfoMixinguardMethod(interp, cl, mixin);

  }
}

static int
NsfClassInfoSlotsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoSlotsMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoSlotsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfClassInfoSlotsMethod(interp, cl);

  }
}

static int
NsfClassInfoSubclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoSubclassMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoSubclassMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withClosure = (int )PTR2INT(pc.clientData[0]);
    CONST char *patternString = NULL;
    NsfObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[1];
    int returnCode;

    if (getMatchObject(interp, pattern, objc>1 ? objv[1] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = NsfClassInfoSubclassMethod(interp, cl, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfClassInfoSuperclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, objv[0], "Class", "");
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoSuperclassMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoSuperclassMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withClosure = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfClassInfoSuperclassMethod(interp, cl, withClosure, pattern);

  }
}

static int
NsfAliasCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfAliasCmdIdx].paramDefs, 
                     method_definitions[NsfAliasCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int withPer_object = (int )PTR2INT(pc.clientData[1]);
    CONST char *methodName = (CONST char *)pc.clientData[2];
    int withNonleaf = (int )PTR2INT(pc.clientData[3]);
    int withObjscope = (int )PTR2INT(pc.clientData[4]);
    Tcl_Obj *cmdName = (Tcl_Obj *)pc.clientData[5];

    parseContextRelease(&pc);
    return NsfAliasCmd(interp, object, withPer_object, methodName, withNonleaf, withObjscope, cmdName);

  }
}

static int
NsfAssertionCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfAssertionCmdIdx].paramDefs, 
                     method_definitions[NsfAssertionCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int assertionsubcmd = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *arg = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfAssertionCmd(interp, object, assertionsubcmd, arg);

  }
}

static int
NsfColonCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {

    

    return NsfColonCmd(interp, objc, objv);

}

static int
NsfConfigureCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfConfigureCmdIdx].paramDefs, 
                     method_definitions[NsfConfigureCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int configureoption = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfConfigureCmd(interp, configureoption, value);

  }
}

static int
NsfCreateObjectSystemCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfCreateObjectSystemCmdIdx].paramDefs, 
                     method_definitions[NsfCreateObjectSystemCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *rootClass = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *rootMetaClass = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *systemMethods = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfCreateObjectSystemCmd(interp, rootClass, rootMetaClass, systemMethods);

  }
}

static int
NsfCurrentCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfCurrentCmdIdx].paramDefs, 
                     method_definitions[NsfCurrentCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int currentoption = (int )PTR2INT(pc.clientData[0]);

    parseContextRelease(&pc);
    return NsfCurrentCmd(interp, currentoption);

  }
}

static int
NsfDeprecatedCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfDeprecatedCmdIdx].paramDefs, 
                     method_definitions[NsfDeprecatedCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *what = (CONST char *)pc.clientData[0];
    CONST char *oldCmd = (CONST char *)pc.clientData[1];
    CONST char *newCmd = (CONST char *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfDeprecatedCmd(interp, what, oldCmd, newCmd);

  }
}

static int
NsfDispatchCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfDispatchCmdIdx].paramDefs, 
                     method_definitions[NsfDispatchCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int withObjscope = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *command = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfDispatchCmd(interp, object, withObjscope, command, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfExistsVarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfExistsVarCmdIdx].paramDefs, 
                     method_definitions[NsfExistsVarCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    CONST char *var = (CONST char *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfExistsVarCmd(interp, object, var);

  }
}

static int
NsfFinalizeObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfFinalizeObjCmdIdx].paramDefs, 
                     method_definitions[NsfFinalizeObjCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfFinalizeObjCmd(interp);

  }
}

static int
NsfForwardCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfForwardCmdIdx].paramDefs, 
                     method_definitions[NsfForwardCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
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
    return NsfForwardCmd(interp, object, withPer_object, method, withDefault, withEarlybinding, withMethodprefix, withObjscope, withOnerror, withVerbose, target, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfImportvarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfImportvarCmdIdx].paramDefs, 
                     method_definitions[NsfImportvarCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfImportvarCmd(interp, object, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfInterpObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfInterpObjCmdIdx].paramDefs, 
                     method_definitions[NsfInterpObjCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *name = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfInterpObjCmd(interp, name, objc, objv);

  }
}

static int
NsfInvalidateObjectParameterCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfInvalidateObjectParameterCmdIdx].paramDefs, 
                     method_definitions[NsfInvalidateObjectParameterCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfClass *class = (NsfClass *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfInvalidateObjectParameterCmd(interp, class);

  }
}

static int
NsfIsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfIsCmdIdx].paramDefs, 
                     method_definitions[NsfIsCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withComplain = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *constraint = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfIsCmd(interp, withComplain, constraint, value);

  }
}

static int
NsfIsObjectCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfIsObjectCmdIdx].paramDefs, 
                     method_definitions[NsfIsObjectCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *object = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfIsObjectCmd(interp, object);

  }
}

static int
NsfMethodCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMethodCmdIdx].paramDefs, 
                     method_definitions[NsfMethodCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int withInner_namespace = (int )PTR2INT(pc.clientData[1]);
    int withPer_object = (int )PTR2INT(pc.clientData[2]);
    int withPublic = (int )PTR2INT(pc.clientData[3]);
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[4];
    Tcl_Obj *args = (Tcl_Obj *)pc.clientData[5];
    Tcl_Obj *body = (Tcl_Obj *)pc.clientData[6];
    Tcl_Obj *withPrecondition = (Tcl_Obj *)pc.clientData[7];
    Tcl_Obj *withPostcondition = (Tcl_Obj *)pc.clientData[8];

    parseContextRelease(&pc);
    return NsfMethodCmd(interp, object, withInner_namespace, withPer_object, withPublic, name, args, body, withPrecondition, withPostcondition);

  }
}

static int
NsfMethodPropertyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMethodPropertyCmdIdx].paramDefs, 
                     method_definitions[NsfMethodPropertyCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int withPer_object = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *methodName = (Tcl_Obj *)pc.clientData[2];
    int methodproperty = (int )PTR2INT(pc.clientData[3]);
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[4];

    parseContextRelease(&pc);
    return NsfMethodPropertyCmd(interp, object, withPer_object, methodName, methodproperty, value);

  }
}

static int
NsfMyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMyCmdIdx].paramDefs, 
                     method_definitions[NsfMyCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withLocal = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *method = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfMyCmd(interp, withLocal, method, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfNSCopyCmdsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfNSCopyCmdsIdx].paramDefs, 
                     method_definitions[NsfNSCopyCmdsIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *fromNs = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *toNs = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfNSCopyCmds(interp, fromNs, toNs);

  }
}

static int
NsfNSCopyVarsStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfNSCopyVarsIdx].paramDefs, 
                     method_definitions[NsfNSCopyVarsIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *fromNs = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *toNs = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfNSCopyVars(interp, fromNs, toNs);

  }
}

static int
NsfQualifyObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfQualifyObjCmdIdx].paramDefs, 
                     method_definitions[NsfQualifyObjCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfQualifyObjCmd(interp, name);

  }
}

static int
NsfRelationCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfRelationCmdIdx].paramDefs, 
                     method_definitions[NsfRelationCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int relationtype = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfRelationCmd(interp, object, relationtype, value);

  }
}

static int
NsfSetVarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfSetVarCmdIdx].paramDefs, 
                     method_definitions[NsfSetVarCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    Tcl_Obj *variable = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfSetVarCmd(interp, object, variable, value);

  }
}

static int
NsfSetterCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfSetterCmdIdx].paramDefs, 
                     method_definitions[NsfSetterCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int withPer_object = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *parameter = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfSetterCmd(interp, object, withPer_object, parameter);

  }
}

static int
NsfOAutonameMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOAutonameMethodIdx].paramDefs, 
                     method_definitions[NsfOAutonameMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withInstance = (int )PTR2INT(pc.clientData[0]);
    int withReset = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfOAutonameMethod(interp, obj, withInstance, withReset, name);

  }
}

static int
NsfOCleanupMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOCleanupMethodIdx].paramDefs, 
                     method_definitions[NsfOCleanupMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfOCleanupMethod(interp, obj);

  }
}

static int
NsfOConfigureMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
    

    return NsfOConfigureMethod(interp, obj, objc, objv);

}

static int
NsfODestroyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfODestroyMethodIdx].paramDefs, 
                     method_definitions[NsfODestroyMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfODestroyMethod(interp, obj);

  }
}

static int
NsfOExistsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOExistsMethodIdx].paramDefs, 
                     method_definitions[NsfOExistsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *var = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfOExistsMethod(interp, obj, var);

  }
}

static int
NsfOFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOFilterGuardMethodIdx].paramDefs, 
                     method_definitions[NsfOFilterGuardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *filter = (CONST char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfOFilterGuardMethod(interp, obj, filter, guard);

  }
}

static int
NsfOInstVarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
    

    return NsfOInstVarMethod(interp, obj, objc, objv);

}

static int
NsfOMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOMixinGuardMethodIdx].paramDefs, 
                     method_definitions[NsfOMixinGuardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *mixin = (CONST char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfOMixinGuardMethod(interp, obj, mixin, guard);

  }
}

static int
NsfONoinitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfONoinitMethodIdx].paramDefs, 
                     method_definitions[NsfONoinitMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfONoinitMethod(interp, obj);

  }
}

static int
NsfORequireNamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfORequireNamespaceMethodIdx].paramDefs, 
                     method_definitions[NsfORequireNamespaceMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfORequireNamespaceMethod(interp, obj);

  }
}

static int
NsfOResidualargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
    

    return NsfOResidualargsMethod(interp, obj, objc, objv);

}

static int
NsfOUplevelMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
    

    return NsfOUplevelMethod(interp, obj, objc, objv);

}

static int
NsfOUpvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
    

    return NsfOUpvarMethod(interp, obj, objc, objv);

}

static int
NsfOVolatileMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOVolatileMethodIdx].paramDefs, 
                     method_definitions[NsfOVolatileMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfOVolatileMethod(interp, obj);

  }
}

static int
NsfOVwaitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOVwaitMethodIdx].paramDefs, 
                     method_definitions[NsfOVwaitMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *varname = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfOVwaitMethod(interp, obj, varname);

  }
}

static int
NsfObjInfoCallableMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoCallableMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoCallableMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int infocallablesubcmd = (int )PTR2INT(pc.clientData[0]);
    int withMethodtype = (int )PTR2INT(pc.clientData[1]);
    int withCallprotection = (int )PTR2INT(pc.clientData[2]);
    int withApplication = (int )PTR2INT(pc.clientData[3]);
    int withNomixins = (int )PTR2INT(pc.clientData[4]);
    int withIncontext = (int )PTR2INT(pc.clientData[5]);
    CONST char *pattern = (CONST char *)pc.clientData[6];

    parseContextRelease(&pc);
    return NsfObjInfoCallableMethod(interp, obj, infocallablesubcmd, withMethodtype, withCallprotection, withApplication, withNomixins, withIncontext, pattern);

  }
}

static int
NsfObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoChildrenMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoChildrenMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *pattern = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfObjInfoChildrenMethod(interp, obj, pattern);

  }
}

static int
NsfObjInfoClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoClassMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoClassMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfObjInfoClassMethod(interp, obj);

  }
}

static int
NsfObjInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoFilterguardMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoFilterguardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *filter = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfObjInfoFilterguardMethod(interp, obj, filter);

  }
}

static int
NsfObjInfoFiltermethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoFiltermethodsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoFiltermethodsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withGuards = (int )PTR2INT(pc.clientData[0]);
    int withOrder = (int )PTR2INT(pc.clientData[1]);
    CONST char *pattern = (CONST char *)pc.clientData[2];

    parseContextRelease(&pc);
    return NsfObjInfoFiltermethodsMethod(interp, obj, withGuards, withOrder, pattern);

  }
}

static int
NsfObjInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoForwardMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoForwardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withDefinition = (int )PTR2INT(pc.clientData[0]);
    CONST char *name = (CONST char *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfObjInfoForwardMethod(interp, obj, withDefinition, name);

  }
}

static int
NsfObjInfoHasMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoHasMixinMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoHasMixinMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfClass *class = (NsfClass *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfObjInfoHasMixinMethod(interp, obj, class);

  }
}

static int
NsfObjInfoHasTypeMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoHasTypeMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoHasTypeMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfClass *class = (NsfClass *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfObjInfoHasTypeMethod(interp, obj, class);

  }
}

static int
NsfObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoHasnamespaceMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoHasnamespaceMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfObjInfoHasnamespaceMethod(interp, obj);

  }
}

static int
NsfObjInfoIsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoIsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoIsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int objectkind = (int )PTR2INT(pc.clientData[0]);

    parseContextRelease(&pc);
    return NsfObjInfoIsMethod(interp, obj, objectkind);

  }
}

static int
NsfObjInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoMethodMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoMethodMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int infomethodsubcmd = (int )PTR2INT(pc.clientData[0]);
    CONST char *name = (CONST char *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfObjInfoMethodMethod(interp, obj, infomethodsubcmd, name);

  }
}

static int
NsfObjInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoMethodsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoMethodsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withMethodtype = (int )PTR2INT(pc.clientData[0]);
    int withCallprotection = (int )PTR2INT(pc.clientData[1]);
    int withNomixins = (int )PTR2INT(pc.clientData[2]);
    int withIncontext = (int )PTR2INT(pc.clientData[3]);
    CONST char *pattern = (CONST char *)pc.clientData[4];

    parseContextRelease(&pc);
    return NsfObjInfoMethodsMethod(interp, obj, withMethodtype, withCallprotection, withNomixins, withIncontext, pattern);

  }
}

static int
NsfObjInfoMixinclassesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoMixinclassesMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoMixinclassesMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withGuards = (int )PTR2INT(pc.clientData[0]);
    int withOrder = (int )PTR2INT(pc.clientData[1]);
    CONST char *patternString = NULL;
    NsfObject *patternObj = NULL;
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[2];
    int returnCode;

    if (getMatchObject(interp, pattern, objc>2 ? objv[2] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    parseContextRelease(&pc);
    returnCode = NsfObjInfoMixinclassesMethod(interp, obj, withGuards, withOrder, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfObjInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoMixinguardMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoMixinguardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *mixin = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfObjInfoMixinguardMethod(interp, obj, mixin);

  }
}

static int
NsfObjInfoParentMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoParentMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoParentMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    

    parseContextRelease(&pc);
    return NsfObjInfoParentMethod(interp, obj);

  }
}

static int
NsfObjInfoPrecedenceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoPrecedenceMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoPrecedenceMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withIntrinsic = (int )PTR2INT(pc.clientData[0]);
    CONST char *pattern = (CONST char *)pc.clientData[1];

    parseContextRelease(&pc);
    return NsfObjInfoPrecedenceMethod(interp, obj, withIntrinsic, pattern);

  }
}

static int
NsfObjInfoSlotObjectsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoSlotObjectsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoSlotObjectsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *pattern = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfObjInfoSlotObjectsMethod(interp, obj, pattern);

  }
}

static int
NsfObjInfoVarsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, objv[0], "Object", "");
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoVarsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoVarsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *pattern = (CONST char *)pc.clientData[0];

    parseContextRelease(&pc);
    return NsfObjInfoVarsMethod(interp, obj, pattern);

  }
}

static methodDefinition method_definitions[] = {
{"::nsf::cmd::Class::alloc", NsfCAllocMethodStub, 1, {
  {"name", 1, 0, convertToTclobj}}
},
{"::nsf::cmd::Class::create", NsfCCreateMethodStub, 2, {
  {"name", 1, 0, convertToString},
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::cmd::Class::dealloc", NsfCDeallocMethodStub, 1, {
  {"object", 1, 0, convertToTclobj}}
},
{"::nsf::cmd::Class::filterguard", NsfCFilterGuardMethodStub, 2, {
  {"filter", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::nsf::cmd::Class::mixinguard", NsfCMixinGuardMethodStub, 2, {
  {"mixin", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::nsf::cmd::Class::new", NsfCNewMethodStub, 2, {
  {"-childof", 0, 1, convertToObject},
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::cmd::Class::recreate", NsfCRecreateMethodStub, 2, {
  {"name", 1, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::cmd::ClassInfo::filterguard", NsfClassInfoFilterguardMethodStub, 1, {
  {"filter", 1, 0, convertToString}}
},
{"::nsf::cmd::ClassInfo::filtermethods", NsfClassInfoFiltermethodsMethodStub, 2, {
  {"-guards", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::nsf::cmd::ClassInfo::forward", NsfClassInfoForwardMethodStub, 2, {
  {"-definition", 0, 0, convertToString},
  {"name", 0, 0, convertToString}}
},
{"::nsf::cmd::ClassInfo::heritage", NsfClassInfoHeritageMethodStub, 1, {
  {"pattern", 0, 0, convertToString}}
},
{"::nsf::cmd::ClassInfo::instances", NsfClassInfoInstancesMethodStub, 2, {
  {"-closure", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::nsf::cmd::ClassInfo::method", NsfClassInfoMethodMethodStub, 2, {
  {"infomethodsubcmd", 0, 0, convertToInfomethodsubcmd},
  {"name", 0, 0, convertToString}}
},
{"::nsf::cmd::ClassInfo::methods", NsfClassInfoMethodsMethodStub, 5, {
  {"-methodtype", 0, 1, convertToMethodtype},
  {"-callprotection", 0, 1, convertToCallprotection},
  {"-nomixins", 0, 0, convertToString},
  {"-incontext", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::nsf::cmd::ClassInfo::mixinof", NsfClassInfoMixinOfMethodStub, 3, {
  {"-closure", 0, 0, convertToString},
  {"-scope", 0, 1, convertToScope},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::nsf::cmd::ClassInfo::mixinclasses", NsfClassInfoMixinclassesMethodStub, 3, {
  {"-closure", 0, 0, convertToString},
  {"-guards", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::nsf::cmd::ClassInfo::mixinguard", NsfClassInfoMixinguardMethodStub, 1, {
  {"mixin", 1, 0, convertToString}}
},
{"::nsf::cmd::ClassInfo::slots", NsfClassInfoSlotsMethodStub, 0, {
  }
},
{"::nsf::cmd::ClassInfo::subclass", NsfClassInfoSubclassMethodStub, 2, {
  {"-closure", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::nsf::cmd::ClassInfo::superclass", NsfClassInfoSuperclassMethodStub, 2, {
  {"-closure", 0, 0, convertToString},
  {"pattern", 0, 0, convertToTclobj}}
},
{"::nsf::alias", NsfAliasCmdStub, 6, {
  {"object", 0, 0, convertToObject},
  {"-per-object", 0, 0, convertToString},
  {"methodName", 0, 0, convertToString},
  {"-nonleaf", 0, 0, convertToString},
  {"-objscope", 0, 0, convertToString},
  {"cmdName", 1, 0, convertToTclobj}}
},
{"::nsf::assertion", NsfAssertionCmdStub, 3, {
  {"object", 0, 0, convertToObject},
  {"assertionsubcmd", 1, 0, convertToAssertionsubcmd},
  {"arg", 0, 0, convertToTclobj}}
},
{"::nsf::colon", NsfColonCmdStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::configure", NsfConfigureCmdStub, 2, {
  {"configureoption", 1, 0, convertToConfigureoption},
  {"value", 0, 0, convertToTclobj}}
},
{"::nsf::createobjectsystem", NsfCreateObjectSystemCmdStub, 3, {
  {"rootClass", 1, 0, convertToTclobj},
  {"rootMetaClass", 1, 0, convertToTclobj},
  {"systemMethods", 0, 0, convertToTclobj}}
},
{"::nsf::current", NsfCurrentCmdStub, 1, {
  {"currentoption", 0, 0, convertToCurrentoption}}
},
{"::nsf::deprecated", NsfDeprecatedCmdStub, 3, {
  {"what", 1, 0, convertToString},
  {"oldCmd", 1, 0, convertToString},
  {"newCmd", 0, 0, convertToString}}
},
{"::nsf::dispatch", NsfDispatchCmdStub, 4, {
  {"object", 1, 0, convertToObject},
  {"-objscope", 0, 0, convertToString},
  {"command", 1, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::existsvar", NsfExistsVarCmdStub, 2, {
  {"object", 1, 0, convertToObject},
  {"var", 1, 0, convertToString}}
},
{"::nsf::finalize", NsfFinalizeObjCmdStub, 0, {
  }
},
{"::nsf::forward", NsfForwardCmdStub, 11, {
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
{"::nsf::importvar", NsfImportvarCmdStub, 2, {
  {"object", 0, 0, convertToObject},
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::interp", NsfInterpObjCmdStub, 2, {
  {"name", 0, 0, convertToString},
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::invalidateobjectparameter", NsfInvalidateObjectParameterCmdStub, 1, {
  {"class", 0, 0, convertToClass}}
},
{"::nsf::is", NsfIsCmdStub, 3, {
  {"-complain", 0, 0, convertToString},
  {"constraint", 1, 0, convertToTclobj},
  {"value", 1, 0, convertToTclobj}}
},
{"::nsf::isobject", NsfIsObjectCmdStub, 1, {
  {"object", 1, 0, convertToTclobj}}
},
{"::nsf::method", NsfMethodCmdStub, 9, {
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
{"::nsf::methodproperty", NsfMethodPropertyCmdStub, 5, {
  {"object", 1, 0, convertToObject},
  {"-per-object", 0, 0, convertToString},
  {"methodName", 1, 0, convertToTclobj},
  {"methodproperty", 1, 0, convertToMethodproperty},
  {"value", 0, 0, convertToTclobj}}
},
{"::nsf::my", NsfMyCmdStub, 3, {
  {"-local", 0, 0, convertToString},
  {"method", 1, 0, convertToTclobj},
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::namespace_copycmds", NsfNSCopyCmdsStub, 2, {
  {"fromNs", 1, 0, convertToTclobj},
  {"toNs", 1, 0, convertToTclobj}}
},
{"::nsf::namespace_copyvars", NsfNSCopyVarsStub, 2, {
  {"fromNs", 1, 0, convertToTclobj},
  {"toNs", 1, 0, convertToTclobj}}
},
{"::nsf::__qualify", NsfQualifyObjCmdStub, 1, {
  {"name", 1, 0, convertToTclobj}}
},
{"::nsf::relation", NsfRelationCmdStub, 3, {
  {"object", 0, 0, convertToObject},
  {"relationtype", 1, 0, convertToRelationtype},
  {"value", 0, 0, convertToTclobj}}
},
{"::nsf::setvar", NsfSetVarCmdStub, 3, {
  {"object", 1, 0, convertToObject},
  {"variable", 1, 0, convertToTclobj},
  {"value", 0, 0, convertToTclobj}}
},
{"::nsf::setter", NsfSetterCmdStub, 3, {
  {"object", 1, 0, convertToObject},
  {"-per-object", 0, 0, convertToString},
  {"parameter", 0, 0, convertToTclobj}}
},
{"::nsf::cmd::Object::autoname", NsfOAutonameMethodStub, 3, {
  {"-instance", 0, 0, convertToString},
  {"-reset", 0, 0, convertToString},
  {"name", 1, 0, convertToTclobj}}
},
{"::nsf::cmd::Object::cleanup", NsfOCleanupMethodStub, 0, {
  }
},
{"::nsf::cmd::Object::configure", NsfOConfigureMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::cmd::Object::destroy", NsfODestroyMethodStub, 0, {
  }
},
{"::nsf::cmd::Object::exists", NsfOExistsMethodStub, 1, {
  {"var", 1, 0, convertToString}}
},
{"::nsf::cmd::Object::filterguard", NsfOFilterGuardMethodStub, 2, {
  {"filter", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::nsf::cmd::Object::instvar", NsfOInstVarMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::cmd::Object::mixinguard", NsfOMixinGuardMethodStub, 2, {
  {"mixin", 1, 0, convertToString},
  {"guard", 1, 0, convertToTclobj}}
},
{"::nsf::cmd::Object::noinit", NsfONoinitMethodStub, 0, {
  }
},
{"::nsf::cmd::Object::requireNamespace", NsfORequireNamespaceMethodStub, 0, {
  }
},
{"::nsf::cmd::Object::residualargs", NsfOResidualargsMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::cmd::Object::uplevel", NsfOUplevelMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::cmd::Object::upvar", NsfOUpvarMethodStub, 1, {
  {"args", 0, 0, convertToNothing}}
},
{"::nsf::cmd::Object::volatile", NsfOVolatileMethodStub, 0, {
  }
},
{"::nsf::cmd::Object::vwait", NsfOVwaitMethodStub, 1, {
  {"varname", 1, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::callable", NsfObjInfoCallableMethodStub, 7, {
  {"infocallablesubcmd", 1, 1, convertToInfocallablesubcmd},
  {"-methodtype", 0, 1, convertToMethodtype},
  {"-callprotection", 0, 1, convertToCallprotection},
  {"-application", 0, 0, convertToString},
  {"-nomixins", 0, 0, convertToString},
  {"-incontext", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::children", NsfObjInfoChildrenMethodStub, 1, {
  {"pattern", 0, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::class", NsfObjInfoClassMethodStub, 0, {
  }
},
{"::nsf::cmd::ObjectInfo::filterguard", NsfObjInfoFilterguardMethodStub, 1, {
  {"filter", 1, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::filtermethods", NsfObjInfoFiltermethodsMethodStub, 3, {
  {"-guards", 0, 0, convertToString},
  {"-order", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::forward", NsfObjInfoForwardMethodStub, 2, {
  {"-definition", 0, 0, convertToString},
  {"name", 0, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::hasmixin", NsfObjInfoHasMixinMethodStub, 1, {
  {"class", 0, 0, convertToClass}}
},
{"::nsf::cmd::ObjectInfo::hastype", NsfObjInfoHasTypeMethodStub, 1, {
  {"class", 0, 0, convertToClass}}
},
{"::nsf::cmd::ObjectInfo::hasnamespace", NsfObjInfoHasnamespaceMethodStub, 0, {
  }
},
{"::nsf::cmd::ObjectInfo::is", NsfObjInfoIsMethodStub, 1, {
  {"objectkind", 0, 0, convertToObjectkind}}
},
{"::nsf::cmd::ObjectInfo::method", NsfObjInfoMethodMethodStub, 2, {
  {"infomethodsubcmd", 0, 0, convertToInfomethodsubcmd},
  {"name", 0, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::methods", NsfObjInfoMethodsMethodStub, 5, {
  {"-methodtype", 0, 1, convertToMethodtype},
  {"-callprotection", 0, 1, convertToCallprotection},
  {"-nomixins", 0, 0, convertToString},
  {"-incontext", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::mixinclasses", NsfObjInfoMixinclassesMethodStub, 3, {
  {"-guards", 0, 0, convertToString},
  {"-order", 0, 0, convertToString},
  {"pattern", 0, 0, convertToObjpattern}}
},
{"::nsf::cmd::ObjectInfo::mixinguard", NsfObjInfoMixinguardMethodStub, 1, {
  {"mixin", 1, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::parent", NsfObjInfoParentMethodStub, 0, {
  }
},
{"::nsf::cmd::ObjectInfo::precedence", NsfObjInfoPrecedenceMethodStub, 2, {
  {"-intrinsic", 0, 0, convertToString},
  {"pattern", 0, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::slotobjects", NsfObjInfoSlotObjectsMethodStub, 1, {
  {"pattern", 0, 0, convertToString}}
},
{"::nsf::cmd::ObjectInfo::vars", NsfObjInfoVarsMethodStub, 1, {
  {"pattern", 0, 0, convertToString}}
},{NULL}
};

