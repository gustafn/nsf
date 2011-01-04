
typedef struct {
  CONST char *methodName;
  Tcl_ObjCmdProc *proc;
  int nrParameters;
  NsfParam paramDefs[12];
} methodDefinition;
   

enum InfomethodsubcmdIdx {InfomethodsubcmdNULL, InfomethodsubcmdArgsIdx, InfomethodsubcmdBodyIdx, InfomethodsubcmdDefinitionIdx, InfomethodsubcmdHandleIdx, InfomethodsubcmdParameterIdx, InfomethodsubcmdParametersyntaxIdx, InfomethodsubcmdTypeIdx, InfomethodsubcmdPreconditionIdx, InfomethodsubcmdPostconditionIdx, InfomethodsubcmdSubmethodsIdx};

static int ConvertToInfomethodsubcmd(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"args", "body", "definition", "handle", "parameter", "parametersyntax", "type", "precondition", "postcondition", "submethods", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "infomethodsubcmd", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum CallprotectionIdx {CallprotectionNULL, CallprotectionAllIdx, CallprotectionProtectedIdx, CallprotectionPublicIdx};

static int ConvertToCallprotection(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"all", "protected", "public", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-callprotection", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum MethodtypeIdx {MethodtypeNULL, MethodtypeAllIdx, MethodtypeScriptedIdx, MethodtypeBuiltinIdx, MethodtypeAliasIdx, MethodtypeForwarderIdx, MethodtypeObjectIdx, MethodtypeSetterIdx, MethodtypeNsfprocIdx};

static int ConvertToMethodtype(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"all", "scripted", "builtin", "alias", "forwarder", "object", "setter", "nsfproc", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-methodtype", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum ScopeIdx {ScopeNULL, ScopeAllIdx, ScopeClassIdx, ScopeObjectIdx};

static int ConvertToScope(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"all", "class", "object", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-scope", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum FrameIdx {FrameNULL, FrameMethodIdx, FrameObjectIdx, FrameDefaultIdx};

static int ConvertToFrame(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"method", "object", "default", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-frame", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum AssertionsubcmdIdx {AssertionsubcmdNULL, AssertionsubcmdCheckIdx, AssertionsubcmdObject_invarIdx, AssertionsubcmdClass_invarIdx};

static int ConvertToAssertionsubcmd(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"check", "object-invar", "class-invar", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "assertionsubcmd", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum ConfigureoptionIdx {ConfigureoptionNULL, ConfigureoptionDebugIdx, ConfigureoptionFilterIdx, ConfigureoptionSoftrecreateIdx, ConfigureoptionObjectsystemsIdx, ConfigureoptionKeepinitcmdIdx, ConfigureoptionCheckresultsIdx, ConfigureoptionCheckargumentsIdx};

static int ConvertToConfigureoption(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"debug", "filter", "softrecreate", "objectsystems", "keepinitcmd", "checkresults", "checkarguments", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "configureoption", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum CurrentoptionIdx {CurrentoptionNULL, CurrentoptionProcIdx, CurrentoptionMethodIdx, CurrentoptionMethodpathIdx, CurrentoptionObjectIdx, CurrentoptionClassIdx, CurrentoptionActivelevelIdx, CurrentoptionArgsIdx, CurrentoptionActivemixinIdx, CurrentoptionCalledprocIdx, CurrentoptionCalledmethodIdx, CurrentoptionCalledclassIdx, CurrentoptionCallingprocIdx, CurrentoptionCallingmethodIdx, CurrentoptionCallingclassIdx, CurrentoptionCallinglevelIdx, CurrentoptionCallingobjectIdx, CurrentoptionFilterregIdx, CurrentoptionIsnextcallIdx, CurrentoptionNextIdx};

static int ConvertToCurrentoption(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"proc", "method", "methodpath", "object", "class", "activelevel", "args", "activemixin", "calledproc", "calledmethod", "calledclass", "callingproc", "callingmethod", "callingclass", "callinglevel", "callingobject", "filterreg", "isnextcall", "next", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "currentoption", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum MethodpropertyIdx {MethodpropertyNULL, MethodpropertyClass_onlyIdx, MethodpropertyCall_protectedIdx, MethodpropertyRedefine_protectedIdx, MethodpropertyReturnsIdx, MethodpropertySlotcontainerIdx, MethodpropertySlotobjIdx};

static int ConvertToMethodproperty(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"class-only", "call-protected", "redefine-protected", "returns", "slotcontainer", "slotobj", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "methodproperty", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum RelationtypeIdx {RelationtypeNULL, RelationtypeObject_mixinIdx, RelationtypeClass_mixinIdx, RelationtypeObject_filterIdx, RelationtypeClass_filterIdx, RelationtypeClassIdx, RelationtypeSuperclassIdx, RelationtypeRootclassIdx};

static int ConvertToRelationtype(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"object-mixin", "class-mixin", "object-filter", "class-filter", "class", "superclass", "rootclass", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "relationtype", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum ObjectkindIdx {ObjectkindNULL, ObjectkindClassIdx, ObjectkindBaseclassIdx, ObjectkindMetaclassIdx};

static int ConvertToObjectkind(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"class", "baseclass", "metaclass", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "objectkind", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  
enum SourceIdx {SourceNULL, SourceAllIdx, SourceApplicationIdx, SourceBaseclassesIdx};

static int ConvertToSource(Tcl_Interp *interp, Tcl_Obj *objPtr, NsfParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  static CONST char *opts[] = {"all", "application", "baseclasses", NULL};
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "-source", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  

static enumeratorConverterEntry enumeratorConverterEntries[] = {
  {ConvertToScope, "all|class|object"},
  {ConvertToInfomethodsubcmd, "args|body|definition|handle|parameter|parametersyntax|type|precondition|postcondition|submethods"},
  {ConvertToCallprotection, "all|protected|public"},
  {ConvertToMethodtype, "all|scripted|builtin|alias|forwarder|object|setter|nsfproc"},
  {ConvertToFrame, "method|object|default"},
  {ConvertToCurrentoption, "proc|method|methodpath|object|class|activelevel|args|activemixin|calledproc|calledmethod|calledclass|callingproc|callingmethod|callingclass|callinglevel|callingobject|filterreg|isnextcall|next"},
  {ConvertToObjectkind, "class|baseclass|metaclass"},
  {ConvertToMethodproperty, "class-only|call-protected|redefine-protected|returns|slotcontainer|slotobj"},
  {ConvertToRelationtype, "object-mixin|class-mixin|object-filter|class-filter|class|superclass|rootclass"},
  {ConvertToSource, "all|application|baseclasses"},
  {ConvertToConfigureoption, "debug|filter|softrecreate|objectsystems|keepinitcmd|checkresults|checkarguments"},
  {ConvertToAssertionsubcmd, "check|object-invar|class-invar"},
  {NULL, NULL}
};
   

static int ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], 
                         NsfObject *obj, Tcl_Obj *procName,
                         NsfParam CONST *paramPtr, int nrParameters, int doCheck,
			 ParseContext *pc);

static int GetMatchObject(Tcl_Interp *interp, Tcl_Obj *patternObj, Tcl_Obj *origObj,
			  NsfObject **matchObject, CONST char **pattern);

/* just to define the symbol */
static methodDefinition method_definitions[];
  
static CONST char *method_command_namespace_names[] = {
  "::nsf::methods::object::info",
  "::nsf::methods::object",
  "::nsf::cmd::ParameterType",
  "::nsf::methods::class::info",
  "::nsf::methods::class"
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
static int NsfClassInfoSubclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfClassInfoSuperclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfAliasCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfAssertionCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfColonCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfConfigureCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCreateObjectSystemCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfCurrentCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfDebugRunAssertionsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
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
static int NsfNSCopyCmdsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfNSCopyVarsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfNextCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfProcCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfQualifyObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfRelationCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfSelfCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfSetVarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfSetterCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfShowStackCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOAutonameMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOCleanupMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOConfigureMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfODestroyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOExistsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOInstvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfONoinitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfORequireNamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOResidualargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOUplevelMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOUpvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOVolatileMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfOVwaitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoFiltermethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoHasMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoHasTypeMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoIsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoLookupFilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoLookupMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoLookupMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoLookupSlotsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoMixinclassesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoParentMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoPrecedenceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfObjInfoVarsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

static int NsfCAllocMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *objectName);
static int NsfCCreateMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *objectName, int objc, Tcl_Obj *CONST objv[]);
static int NsfCDeallocMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *object);
static int NsfCFilterGuardMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *filter, Tcl_Obj *guard);
static int NsfCMixinGuardMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *mixin, Tcl_Obj *guard);
static int NsfCNewMethod(Tcl_Interp *interp, NsfClass *cl, NsfObject *withChildof, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfCRecreateMethod(Tcl_Interp *interp, NsfClass *cl, Tcl_Obj *objectName, int objc, Tcl_Obj *CONST objv[]);
static int NsfClassInfoFilterguardMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *filter);
static int NsfClassInfoFiltermethodsMethod(Tcl_Interp *interp, NsfClass *cl, int withGuards, CONST char *pattern);
static int NsfClassInfoForwardMethod(Tcl_Interp *interp, NsfClass *cl, int withDefinition, CONST char *name);
static int NsfClassInfoHeritageMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *pattern);
static int NsfClassInfoInstancesMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, CONST char *patternString, NsfObject *patternObj);
static int NsfClassInfoMethodMethod(Tcl_Interp *interp, NsfClass *cl, int infomethodsubcmd, Tcl_Obj *name);
static int NsfClassInfoMethodsMethod(Tcl_Interp *interp, NsfClass *cl, int withCallprotection, int withIncontext, int withMethodtype, int withNomixins, int withPath, CONST char *pattern);
static int NsfClassInfoMixinOfMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, int withScope, CONST char *patternString, NsfObject *patternObj);
static int NsfClassInfoMixinclassesMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, int withGuards, CONST char *patternString, NsfObject *patternObj);
static int NsfClassInfoMixinguardMethod(Tcl_Interp *interp, NsfClass *cl, CONST char *mixin);
static int NsfClassInfoSubclassMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, CONST char *patternString, NsfObject *patternObj);
static int NsfClassInfoSuperclassMethod(Tcl_Interp *interp, NsfClass *cl, int withClosure, Tcl_Obj *pattern);
static int NsfAliasCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, CONST char *methodName, int withFrame, Tcl_Obj *cmdName);
static int NsfAssertionCmd(Tcl_Interp *interp, NsfObject *object, int assertionsubcmd, Tcl_Obj *arg);
static int NsfColonCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int NsfConfigureCmd(Tcl_Interp *interp, int configureoption, Tcl_Obj *value);
static int NsfCreateObjectSystemCmd(Tcl_Interp *interp, Tcl_Obj *rootClass, Tcl_Obj *rootMetaClass, Tcl_Obj *systemMethods);
static int NsfCurrentCmd(Tcl_Interp *interp, int currentoption);
static int NsfDebugRunAssertionsCmd(Tcl_Interp *interp);
static int NsfDispatchCmd(Tcl_Interp *interp, NsfObject *object, int withFrame, Tcl_Obj *command, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfExistsVarCmd(Tcl_Interp *interp, NsfObject *object, CONST char *varName);
static int NsfFinalizeObjCmd(Tcl_Interp *interp);
static int NsfForwardCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, Tcl_Obj *method, Tcl_Obj *withDefault, int withEarlybinding, Tcl_Obj *withMethodprefix, int withObjframe, Tcl_Obj *withOnerror, int withVerbose, Tcl_Obj *target, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfImportvarCmd(Tcl_Interp *interp, NsfObject *object, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfInterpObjCmd(Tcl_Interp *interp, CONST char *name, int objc, Tcl_Obj *CONST objv[]);
static int NsfInvalidateObjectParameterCmd(Tcl_Interp *interp, NsfClass *class);
static int NsfIsCmd(Tcl_Interp *interp, int withComplain, Tcl_Obj *constraint, Tcl_Obj *value);
static int NsfIsObjectCmd(Tcl_Interp *interp, Tcl_Obj *value);
static int NsfMethodCmd(Tcl_Interp *interp, NsfObject *object, int withInner_namespace, int withPer_object, Tcl_Obj *methodName, Tcl_Obj *arguments, Tcl_Obj *body, Tcl_Obj *withPrecondition, Tcl_Obj *withPostcondition);
static int NsfMethodPropertyCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, Tcl_Obj *methodName, int methodproperty, Tcl_Obj *value);
static int NsfMyCmd(Tcl_Interp *interp, int withLocal, Tcl_Obj *methodName, int nobjc, Tcl_Obj *CONST nobjv[]);
static int NsfNSCopyCmdsCmd(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs);
static int NsfNSCopyVarsCmd(Tcl_Interp *interp, Tcl_Obj *fromNs, Tcl_Obj *toNs);
static int NsfNextCmd(Tcl_Interp *interp, Tcl_Obj *arguments);
static int NsfProcCmd(Tcl_Interp *interp, int withAd, Tcl_Obj *procName, Tcl_Obj *arguments, Tcl_Obj *body);
static int NsfQualifyObjCmd(Tcl_Interp *interp, Tcl_Obj *objectName);
static int NsfRelationCmd(Tcl_Interp *interp, NsfObject *object, int relationtype, Tcl_Obj *value);
static int NsfSelfCmd(Tcl_Interp *interp);
static int NsfSetVarCmd(Tcl_Interp *interp, NsfObject *object, Tcl_Obj *varName, Tcl_Obj *value);
static int NsfSetterCmd(Tcl_Interp *interp, NsfObject *object, int withPer_object, Tcl_Obj *parameter);
static int NsfShowStackCmd(Tcl_Interp *interp);
static int NsfOAutonameMethod(Tcl_Interp *interp, NsfObject *obj, int withInstance, int withReset, Tcl_Obj *name);
static int NsfOCleanupMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfOConfigureMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfODestroyMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfOExistsMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *varName);
static int NsfOFilterGuardMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *filter, Tcl_Obj *guard);
static int NsfOInstvarMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfOMixinGuardMethod(Tcl_Interp *interp, NsfObject *obj, Tcl_Obj *mixin, Tcl_Obj *guard);
static int NsfONoinitMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfORequireNamespaceMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfOResidualargsMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfOUplevelMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfOUpvarMethod(Tcl_Interp *interp, NsfObject *obj, int objc, Tcl_Obj *CONST objv[]);
static int NsfOVolatileMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfOVwaitMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *varName);
static int NsfObjInfoChildrenMethod(Tcl_Interp *interp, NsfObject *obj, NsfClass *withType, CONST char *pattern);
static int NsfObjInfoClassMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfObjInfoFilterguardMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *fileName);
static int NsfObjInfoFiltermethodsMethod(Tcl_Interp *interp, NsfObject *obj, int withGuards, int withOrder, CONST char *pattern);
static int NsfObjInfoForwardMethod(Tcl_Interp *interp, NsfObject *obj, int withDefinition, CONST char *name);
static int NsfObjInfoHasMixinMethod(Tcl_Interp *interp, NsfObject *obj, NsfClass *class);
static int NsfObjInfoHasTypeMethod(Tcl_Interp *interp, NsfObject *obj, NsfClass *class);
static int NsfObjInfoHasnamespaceMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfObjInfoIsMethod(Tcl_Interp *interp, NsfObject *obj, int objectkind);
static int NsfObjInfoLookupFilterMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *filter);
static int NsfObjInfoLookupMethodMethod(Tcl_Interp *interp, NsfObject *obj, Tcl_Obj *name);
static int NsfObjInfoLookupMethodsMethod(Tcl_Interp *interp, NsfObject *obj, int withCallprotection, int withIncontext, int withMethodtype, int withNomixins, int withPath, int withSource, CONST char *pattern);
static int NsfObjInfoLookupSlotsMethod(Tcl_Interp *interp, NsfObject *obj, NsfClass *withType);
static int NsfObjInfoMethodMethod(Tcl_Interp *interp, NsfObject *obj, int infomethodsubcmd, Tcl_Obj *name);
static int NsfObjInfoMethodsMethod(Tcl_Interp *interp, NsfObject *obj, int withCallprotection, int withIncontext, int withMethodtype, int withNomixins, int withPath, CONST char *pattern);
static int NsfObjInfoMixinclassesMethod(Tcl_Interp *interp, NsfObject *obj, int withGuards, int withOrder, CONST char *patternString, NsfObject *patternObj);
static int NsfObjInfoMixinguardMethod(Tcl_Interp *interp, NsfObject *obj, CONST char *mixin);
static int NsfObjInfoParentMethod(Tcl_Interp *interp, NsfObject *obj);
static int NsfObjInfoPrecedenceMethod(Tcl_Interp *interp, NsfObject *obj, int withIntrinsic, CONST char *pattern);
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
 NsfClassInfoSubclassMethodIdx,
 NsfClassInfoSuperclassMethodIdx,
 NsfAliasCmdIdx,
 NsfAssertionCmdIdx,
 NsfColonCmdIdx,
 NsfConfigureCmdIdx,
 NsfCreateObjectSystemCmdIdx,
 NsfCurrentCmdIdx,
 NsfDebugRunAssertionsCmdIdx,
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
 NsfNSCopyCmdsCmdIdx,
 NsfNSCopyVarsCmdIdx,
 NsfNextCmdIdx,
 NsfProcCmdIdx,
 NsfQualifyObjCmdIdx,
 NsfRelationCmdIdx,
 NsfSelfCmdIdx,
 NsfSetVarCmdIdx,
 NsfSetterCmdIdx,
 NsfShowStackCmdIdx,
 NsfOAutonameMethodIdx,
 NsfOCleanupMethodIdx,
 NsfOConfigureMethodIdx,
 NsfODestroyMethodIdx,
 NsfOExistsMethodIdx,
 NsfOFilterGuardMethodIdx,
 NsfOInstvarMethodIdx,
 NsfOMixinGuardMethodIdx,
 NsfONoinitMethodIdx,
 NsfORequireNamespaceMethodIdx,
 NsfOResidualargsMethodIdx,
 NsfOUplevelMethodIdx,
 NsfOUpvarMethodIdx,
 NsfOVolatileMethodIdx,
 NsfOVwaitMethodIdx,
 NsfObjInfoChildrenMethodIdx,
 NsfObjInfoClassMethodIdx,
 NsfObjInfoFilterguardMethodIdx,
 NsfObjInfoFiltermethodsMethodIdx,
 NsfObjInfoForwardMethodIdx,
 NsfObjInfoHasMixinMethodIdx,
 NsfObjInfoHasTypeMethodIdx,
 NsfObjInfoHasnamespaceMethodIdx,
 NsfObjInfoIsMethodIdx,
 NsfObjInfoLookupFilterMethodIdx,
 NsfObjInfoLookupMethodMethodIdx,
 NsfObjInfoLookupMethodsMethodIdx,
 NsfObjInfoLookupSlotsMethodIdx,
 NsfObjInfoMethodMethodIdx,
 NsfObjInfoMethodsMethodIdx,
 NsfObjInfoMixinclassesMethodIdx,
 NsfObjInfoMixinguardMethodIdx,
 NsfObjInfoParentMethodIdx,
 NsfObjInfoPrecedenceMethodIdx,
 NsfObjInfoVarsMethodIdx
} NsfMethods;


static int
NsfCAllocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
    

      if (objc != 2) {
	return ArgumentError(interp, "wrong # of arguments:", 
			     method_definitions[NsfCAllocMethodIdx].paramDefs,
			     NULL, objv[0]); 
      }
    
    return NsfCAllocMethod(interp, cl, objc == 2 ? objv[1] : NULL);

}

static int
NsfCCreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCCreateMethodIdx].paramDefs, 
                     method_definitions[NsfCCreateMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *objectName = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfCCreateMethod(interp, cl, objectName, objc, objv);

  }
}

static int
NsfCDeallocMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
    

      if (objc != 2) {
	return ArgumentError(interp, "wrong # of arguments:", 
			     method_definitions[NsfCDeallocMethodIdx].paramDefs,
			     NULL, objv[0]); 
      }
    
    return NsfCDeallocMethod(interp, cl, objc == 2 ? objv[1] : NULL);

}

static int
NsfCFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCFilterGuardMethodIdx].paramDefs, 
                     method_definitions[NsfCFilterGuardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *filter = (CONST char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfCFilterGuardMethod(interp, cl, filter, guard);

  }
}

static int
NsfCMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCMixinGuardMethodIdx].paramDefs, 
                     method_definitions[NsfCMixinGuardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *mixin = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfCMixinGuardMethod(interp, cl, mixin, guard);

  }
}

static int
NsfCNewMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCNewMethodIdx].paramDefs, 
                     method_definitions[NsfCNewMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *withChildof = (NsfObject *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfCNewMethod(interp, cl, withChildof, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfCRecreateMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfCRecreateMethodIdx].paramDefs, 
                     method_definitions[NsfCRecreateMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *objectName = (Tcl_Obj *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfCRecreateMethod(interp, cl, objectName, objc, objv);

  }
}

static int
NsfClassInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoFilterguardMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoFilterguardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *filter = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfClassInfoFilterguardMethod(interp, cl, filter);

  }
}

static int
NsfClassInfoFiltermethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoFiltermethodsMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoFiltermethodsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withGuards = (int )PTR2INT(pc.clientData[0]);
    CONST char *pattern = (CONST char *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfClassInfoFiltermethodsMethod(interp, cl, withGuards, pattern);

  }
}

static int
NsfClassInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoForwardMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoForwardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withDefinition = (int )PTR2INT(pc.clientData[0]);
    CONST char *name = (CONST char *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfClassInfoForwardMethod(interp, cl, withDefinition, name);

  }
}

static int
NsfClassInfoHeritageMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoHeritageMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoHeritageMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *pattern = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfClassInfoHeritageMethod(interp, cl, pattern);

  }
}

static int
NsfClassInfoInstancesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
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

    if (GetMatchObject(interp, pattern, objc>1 ? objv[1] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    assert(pc.status == 0);
    returnCode = NsfClassInfoInstancesMethod(interp, cl, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfClassInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoMethodMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoMethodMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int infomethodsubcmd = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfClassInfoMethodMethod(interp, cl, infomethodsubcmd, name);

  }
}

static int
NsfClassInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoMethodsMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoMethodsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withCallprotection = (int )PTR2INT(pc.clientData[0]);
    int withIncontext = (int )PTR2INT(pc.clientData[1]);
    int withMethodtype = (int )PTR2INT(pc.clientData[2]);
    int withNomixins = (int )PTR2INT(pc.clientData[3]);
    int withPath = (int )PTR2INT(pc.clientData[4]);
    CONST char *pattern = (CONST char *)pc.clientData[5];

    assert(pc.status == 0);
    return NsfClassInfoMethodsMethod(interp, cl, withCallprotection, withIncontext, withMethodtype, withNomixins, withPath, pattern);

  }
}

static int
NsfClassInfoMixinOfMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
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

    if (GetMatchObject(interp, pattern, objc>2 ? objv[2] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    assert(pc.status == 0);
    returnCode = NsfClassInfoMixinOfMethod(interp, cl, withClosure, withScope, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfClassInfoMixinclassesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
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

    if (GetMatchObject(interp, pattern, objc>2 ? objv[2] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    assert(pc.status == 0);
    returnCode = NsfClassInfoMixinclassesMethod(interp, cl, withClosure, withGuards, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfClassInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoMixinguardMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoMixinguardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *mixin = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfClassInfoMixinguardMethod(interp, cl, mixin);

  }
}

static int
NsfClassInfoSubclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
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

    if (GetMatchObject(interp, pattern, objc>1 ? objv[1] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    assert(pc.status == 0);
    returnCode = NsfClassInfoSubclassMethod(interp, cl, withClosure, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfClassInfoSuperclassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfClass *cl =  NsfObjectToClass(clientData);
  if (!cl) return NsfObjErrType(interp, NULL, objv[0], "Class", NULL);
  if (ArgumentParse(interp, objc, objv, (NsfObject *) cl, objv[0], 
                     method_definitions[NsfClassInfoSuperclassMethodIdx].paramDefs, 
                     method_definitions[NsfClassInfoSuperclassMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withClosure = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *pattern = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfClassInfoSuperclassMethod(interp, cl, withClosure, pattern);

  }
}

static int
NsfAliasCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfAliasCmdIdx].paramDefs, 
                     method_definitions[NsfAliasCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int withPer_object = (int )PTR2INT(pc.clientData[1]);
    CONST char *methodName = (CONST char *)pc.clientData[2];
    int withFrame = (int )PTR2INT(pc.clientData[3]);
    Tcl_Obj *cmdName = (Tcl_Obj *)pc.clientData[4];

    assert(pc.status == 0);
    return NsfAliasCmd(interp, object, withPer_object, methodName, withFrame, cmdName);

  }
}

static int
NsfAssertionCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfAssertionCmdIdx].paramDefs, 
                     method_definitions[NsfAssertionCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int assertionsubcmd = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *arg = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfAssertionCmd(interp, object, assertionsubcmd, arg);

  }
}

static int
NsfColonCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  (void)clientData;

    

    return NsfColonCmd(interp, objc, objv);

}

static int
NsfConfigureCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfConfigureCmdIdx].paramDefs, 
                     method_definitions[NsfConfigureCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int configureoption = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfConfigureCmd(interp, configureoption, value);

  }
}

static int
NsfCreateObjectSystemCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfCreateObjectSystemCmdIdx].paramDefs, 
                     method_definitions[NsfCreateObjectSystemCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *rootClass = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *rootMetaClass = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *systemMethods = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfCreateObjectSystemCmd(interp, rootClass, rootMetaClass, systemMethods);

  }
}

static int
NsfCurrentCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfCurrentCmdIdx].paramDefs, 
                     method_definitions[NsfCurrentCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int currentoption = (int )PTR2INT(pc.clientData[0]);

    assert(pc.status == 0);
    return NsfCurrentCmd(interp, currentoption);

  }
}

static int
NsfDebugRunAssertionsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  (void)clientData;

    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfDebugRunAssertionsCmdIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfDebugRunAssertionsCmd(interp);

}

static int
NsfDispatchCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfDispatchCmdIdx].paramDefs, 
                     method_definitions[NsfDispatchCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int withFrame = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *command = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfDispatchCmd(interp, object, withFrame, command, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfExistsVarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfExistsVarCmdIdx].paramDefs, 
                     method_definitions[NsfExistsVarCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    CONST char *varName = (CONST char *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfExistsVarCmd(interp, object, varName);

  }
}

static int
NsfFinalizeObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  (void)clientData;

    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfFinalizeObjCmdIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfFinalizeObjCmd(interp);

}

static int
NsfForwardCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

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
    int withObjframe = (int )PTR2INT(pc.clientData[6]);
    Tcl_Obj *withOnerror = (Tcl_Obj *)pc.clientData[7];
    int withVerbose = (int )PTR2INT(pc.clientData[8]);
    Tcl_Obj *target = (Tcl_Obj *)pc.clientData[9];

    assert(pc.status == 0);
    return NsfForwardCmd(interp, object, withPer_object, method, withDefault, withEarlybinding, withMethodprefix, withObjframe, withOnerror, withVerbose, target, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfImportvarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfImportvarCmdIdx].paramDefs, 
                     method_definitions[NsfImportvarCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfImportvarCmd(interp, object, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfInterpObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfInterpObjCmdIdx].paramDefs, 
                     method_definitions[NsfInterpObjCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *name = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfInterpObjCmd(interp, name, objc, objv);

  }
}

static int
NsfInvalidateObjectParameterCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfInvalidateObjectParameterCmdIdx].paramDefs, 
                     method_definitions[NsfInvalidateObjectParameterCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfClass *class = (NsfClass *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfInvalidateObjectParameterCmd(interp, class);

  }
}

static int
NsfIsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfIsCmdIdx].paramDefs, 
                     method_definitions[NsfIsCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withComplain = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *constraint = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfIsCmd(interp, withComplain, constraint, value);

  }
}

static int
NsfIsObjectCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  (void)clientData;

    

      if (objc != 2) {
	return ArgumentError(interp, "wrong # of arguments:", 
			     method_definitions[NsfIsObjectCmdIdx].paramDefs,
			     NULL, objv[0]); 
      }
    
    return NsfIsObjectCmd(interp, objc == 2 ? objv[1] : NULL);

}

static int
NsfMethodCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMethodCmdIdx].paramDefs, 
                     method_definitions[NsfMethodCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int withInner_namespace = (int )PTR2INT(pc.clientData[1]);
    int withPer_object = (int )PTR2INT(pc.clientData[2]);
    Tcl_Obj *methodName = (Tcl_Obj *)pc.clientData[3];
    Tcl_Obj *arguments = (Tcl_Obj *)pc.clientData[4];
    Tcl_Obj *body = (Tcl_Obj *)pc.clientData[5];
    Tcl_Obj *withPrecondition = (Tcl_Obj *)pc.clientData[6];
    Tcl_Obj *withPostcondition = (Tcl_Obj *)pc.clientData[7];

    assert(pc.status == 0);
    return NsfMethodCmd(interp, object, withInner_namespace, withPer_object, methodName, arguments, body, withPrecondition, withPostcondition);

  }
}

static int
NsfMethodPropertyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

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

    assert(pc.status == 0);
    return NsfMethodPropertyCmd(interp, object, withPer_object, methodName, methodproperty, value);

  }
}

static int
NsfMyCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMyCmdIdx].paramDefs, 
                     method_definitions[NsfMyCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withLocal = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *methodName = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfMyCmd(interp, withLocal, methodName, objc-pc.lastobjc, objv+pc.lastobjc);

  }
}

static int
NsfNSCopyCmdsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfNSCopyCmdsCmdIdx].paramDefs, 
                     method_definitions[NsfNSCopyCmdsCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *fromNs = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *toNs = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfNSCopyCmdsCmd(interp, fromNs, toNs);

  }
}

static int
NsfNSCopyVarsCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfNSCopyVarsCmdIdx].paramDefs, 
                     method_definitions[NsfNSCopyVarsCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *fromNs = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *toNs = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfNSCopyVarsCmd(interp, fromNs, toNs);

  }
}

static int
NsfNextCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  (void)clientData;

    

      if (objc < 1 || objc > 2) {
	return ArgumentError(interp, "wrong # of arguments:", 
			     method_definitions[NsfNextCmdIdx].paramDefs,
			     NULL, objv[0]); 
      }
    
    return NsfNextCmd(interp, objc == 2 ? objv[1] : NULL);

}

static int
NsfProcCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfProcCmdIdx].paramDefs, 
                     method_definitions[NsfProcCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withAd = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *procName = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *arguments = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj *body = (Tcl_Obj *)pc.clientData[3];

    assert(pc.status == 0);
    return NsfProcCmd(interp, withAd, procName, arguments, body);

  }
}

static int
NsfQualifyObjCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  (void)clientData;

    

      if (objc != 2) {
	return ArgumentError(interp, "wrong # of arguments:", 
			     method_definitions[NsfQualifyObjCmdIdx].paramDefs,
			     NULL, objv[0]); 
      }
    
    return NsfQualifyObjCmd(interp, objc == 2 ? objv[1] : NULL);

}

static int
NsfRelationCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfRelationCmdIdx].paramDefs, 
                     method_definitions[NsfRelationCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int relationtype = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfRelationCmd(interp, object, relationtype, value);

  }
}

static int
NsfSelfCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  (void)clientData;

    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfSelfCmdIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfSelfCmd(interp);

}

static int
NsfSetVarCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfSetVarCmdIdx].paramDefs, 
                     method_definitions[NsfSetVarCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    Tcl_Obj *varName = (Tcl_Obj *)pc.clientData[1];
    Tcl_Obj *value = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfSetVarCmd(interp, object, varName, value);

  }
}

static int
NsfSetterCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfSetterCmdIdx].paramDefs, 
                     method_definitions[NsfSetterCmdIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfObject *object = (NsfObject *)pc.clientData[0];
    int withPer_object = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *parameter = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfSetterCmd(interp, object, withPer_object, parameter);

  }
}

static int
NsfShowStackCmdStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  (void)clientData;

    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfShowStackCmdIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfShowStackCmd(interp);

}

static int
NsfOAutonameMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOAutonameMethodIdx].paramDefs, 
                     method_definitions[NsfOAutonameMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withInstance = (int )PTR2INT(pc.clientData[0]);
    int withReset = (int )PTR2INT(pc.clientData[1]);
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfOAutonameMethod(interp, obj, withInstance, withReset, name);

  }
}

static int
NsfOCleanupMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfOCleanupMethodIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfOCleanupMethod(interp, obj);

}

static int
NsfOConfigureMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

    return NsfOConfigureMethod(interp, obj, objc, objv);

}

static int
NsfODestroyMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfODestroyMethodIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfODestroyMethod(interp, obj);

}

static int
NsfOExistsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOExistsMethodIdx].paramDefs, 
                     method_definitions[NsfOExistsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *varName = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfOExistsMethod(interp, obj, varName);

  }
}

static int
NsfOFilterGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOFilterGuardMethodIdx].paramDefs, 
                     method_definitions[NsfOFilterGuardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *filter = (CONST char *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfOFilterGuardMethod(interp, obj, filter, guard);

  }
}

static int
NsfOInstvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

    return NsfOInstvarMethod(interp, obj, objc, objv);

}

static int
NsfOMixinGuardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOMixinGuardMethodIdx].paramDefs, 
                     method_definitions[NsfOMixinGuardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    Tcl_Obj *mixin = (Tcl_Obj *)pc.clientData[0];
    Tcl_Obj *guard = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfOMixinGuardMethod(interp, obj, mixin, guard);

  }
}

static int
NsfONoinitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfONoinitMethodIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfONoinitMethod(interp, obj);

}

static int
NsfORequireNamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfORequireNamespaceMethodIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfORequireNamespaceMethod(interp, obj);

}

static int
NsfOResidualargsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

    return NsfOResidualargsMethod(interp, obj, objc, objv);

}

static int
NsfOUplevelMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

    return NsfOUplevelMethod(interp, obj, objc, objv);

}

static int
NsfOUpvarMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

    return NsfOUpvarMethod(interp, obj, objc, objv);

}

static int
NsfOVolatileMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfOVolatileMethodIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfOVolatileMethod(interp, obj);

}

static int
NsfOVwaitMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfOVwaitMethodIdx].paramDefs, 
                     method_definitions[NsfOVwaitMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *varName = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfOVwaitMethod(interp, obj, varName);

  }
}

static int
NsfObjInfoChildrenMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoChildrenMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoChildrenMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfClass *withType = (NsfClass *)pc.clientData[0];
    CONST char *pattern = (CONST char *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfObjInfoChildrenMethod(interp, obj, withType, pattern);

  }
}

static int
NsfObjInfoClassMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfObjInfoClassMethodIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfObjInfoClassMethod(interp, obj);

}

static int
NsfObjInfoFilterguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoFilterguardMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoFilterguardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *fileName = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfObjInfoFilterguardMethod(interp, obj, fileName);

  }
}

static int
NsfObjInfoFiltermethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoFiltermethodsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoFiltermethodsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withGuards = (int )PTR2INT(pc.clientData[0]);
    int withOrder = (int )PTR2INT(pc.clientData[1]);
    CONST char *pattern = (CONST char *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfObjInfoFiltermethodsMethod(interp, obj, withGuards, withOrder, pattern);

  }
}

static int
NsfObjInfoForwardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoForwardMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoForwardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withDefinition = (int )PTR2INT(pc.clientData[0]);
    CONST char *name = (CONST char *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfObjInfoForwardMethod(interp, obj, withDefinition, name);

  }
}

static int
NsfObjInfoHasMixinMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoHasMixinMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoHasMixinMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfClass *class = (NsfClass *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfObjInfoHasMixinMethod(interp, obj, class);

  }
}

static int
NsfObjInfoHasTypeMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoHasTypeMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoHasTypeMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfClass *class = (NsfClass *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfObjInfoHasTypeMethod(interp, obj, class);

  }
}

static int
NsfObjInfoHasnamespaceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfObjInfoHasnamespaceMethodIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfObjInfoHasnamespaceMethod(interp, obj);

}

static int
NsfObjInfoIsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoIsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoIsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int objectkind = (int )PTR2INT(pc.clientData[0]);

    assert(pc.status == 0);
    return NsfObjInfoIsMethod(interp, obj, objectkind);

  }
}

static int
NsfObjInfoLookupFilterMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoLookupFilterMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoLookupFilterMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *filter = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfObjInfoLookupFilterMethod(interp, obj, filter);

  }
}

static int
NsfObjInfoLookupMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

      if (objc != 2) {
	return ArgumentError(interp, "wrong # of arguments:", 
			     method_definitions[NsfObjInfoLookupMethodMethodIdx].paramDefs,
			     NULL, objv[0]); 
      }
    
    return NsfObjInfoLookupMethodMethod(interp, obj, objc == 2 ? objv[1] : NULL);

}

static int
NsfObjInfoLookupMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoLookupMethodsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoLookupMethodsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withCallprotection = (int )PTR2INT(pc.clientData[0]);
    int withIncontext = (int )PTR2INT(pc.clientData[1]);
    int withMethodtype = (int )PTR2INT(pc.clientData[2]);
    int withNomixins = (int )PTR2INT(pc.clientData[3]);
    int withPath = (int )PTR2INT(pc.clientData[4]);
    int withSource = (int )PTR2INT(pc.clientData[5]);
    CONST char *pattern = (CONST char *)pc.clientData[6];

    assert(pc.status == 0);
    return NsfObjInfoLookupMethodsMethod(interp, obj, withCallprotection, withIncontext, withMethodtype, withNomixins, withPath, withSource, pattern);

  }
}

static int
NsfObjInfoLookupSlotsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoLookupSlotsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoLookupSlotsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    NsfClass *withType = (NsfClass *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfObjInfoLookupSlotsMethod(interp, obj, withType);

  }
}

static int
NsfObjInfoMethodMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoMethodMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoMethodMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int infomethodsubcmd = (int )PTR2INT(pc.clientData[0]);
    Tcl_Obj *name = (Tcl_Obj *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfObjInfoMethodMethod(interp, obj, infomethodsubcmd, name);

  }
}

static int
NsfObjInfoMethodsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoMethodsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoMethodsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withCallprotection = (int )PTR2INT(pc.clientData[0]);
    int withIncontext = (int )PTR2INT(pc.clientData[1]);
    int withMethodtype = (int )PTR2INT(pc.clientData[2]);
    int withNomixins = (int )PTR2INT(pc.clientData[3]);
    int withPath = (int )PTR2INT(pc.clientData[4]);
    CONST char *pattern = (CONST char *)pc.clientData[5];

    assert(pc.status == 0);
    return NsfObjInfoMethodsMethod(interp, obj, withCallprotection, withIncontext, withMethodtype, withNomixins, withPath, pattern);

  }
}

static int
NsfObjInfoMixinclassesMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
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

    if (GetMatchObject(interp, pattern, objc>2 ? objv[2] : NULL, &patternObj, &patternString) == -1) {
      if (pattern) {
        DECR_REF_COUNT(pattern);
      }
      return TCL_OK;
    }
          
    assert(pc.status == 0);
    returnCode = NsfObjInfoMixinclassesMethod(interp, obj, withGuards, withOrder, patternString, patternObj);

    if (pattern) {
      DECR_REF_COUNT(pattern);
    }
    return returnCode;
  }
}

static int
NsfObjInfoMixinguardMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoMixinguardMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoMixinguardMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *mixin = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfObjInfoMixinguardMethod(interp, obj, mixin);

  }
}

static int
NsfObjInfoParentMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
    

      if (objc != 1) {
	return ArgumentError(interp, "too many arguments:", 
			     method_definitions[NsfObjInfoParentMethodIdx].paramDefs,
			     NULL, objv[0]); 
      } 
    
    return NsfObjInfoParentMethod(interp, obj);

}

static int
NsfObjInfoPrecedenceMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoPrecedenceMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoPrecedenceMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    int withIntrinsic = (int )PTR2INT(pc.clientData[0]);
    CONST char *pattern = (CONST char *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfObjInfoPrecedenceMethod(interp, obj, withIntrinsic, pattern);

  }
}

static int
NsfObjInfoVarsMethodStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  NsfObject *obj =  (NsfObject *)clientData;
  if (!obj) return NsfObjErrType(interp, NULL, objv[0], "Object", NULL);
  if (ArgumentParse(interp, objc, objv, obj, objv[0], 
                     method_definitions[NsfObjInfoVarsMethodIdx].paramDefs, 
                     method_definitions[NsfObjInfoVarsMethodIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *pattern = (CONST char *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfObjInfoVarsMethod(interp, obj, pattern);

  }
}

static methodDefinition method_definitions[] = {
{"::nsf::methods::class::alloc", NsfCAllocMethodStub, 1, {
  {"objectName", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::class::create", NsfCCreateMethodStub, 2, {
  {"objectName", NSF_ARG_REQUIRED, 0, ConvertToString},
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::methods::class::dealloc", NsfCDeallocMethodStub, 1, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::class::filterguard", NsfCFilterGuardMethodStub, 2, {
  {"filter", NSF_ARG_REQUIRED, 0, ConvertToString},
  {"guard", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::class::mixinguard", NsfCMixinGuardMethodStub, 2, {
  {"mixin", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"guard", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::class::new", NsfCNewMethodStub, 2, {
  {"-childof", 0, 1, ConvertToObject},
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::methods::class::recreate", NsfCRecreateMethodStub, 2, {
  {"objectName", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::methods::class::info::filterguard", NsfClassInfoFilterguardMethodStub, 1, {
  {"filter", NSF_ARG_REQUIRED, 0, ConvertToString}}
},
{"::nsf::methods::class::info::filtermethods", NsfClassInfoFiltermethodsMethodStub, 2, {
  {"-guards", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToString}}
},
{"::nsf::methods::class::info::forward", NsfClassInfoForwardMethodStub, 2, {
  {"-definition", 0, 0, ConvertToString},
  {"name", 0, 0, ConvertToString}}
},
{"::nsf::methods::class::info::heritage", NsfClassInfoHeritageMethodStub, 1, {
  {"pattern", 0, 0, ConvertToString}}
},
{"::nsf::methods::class::info::instances", NsfClassInfoInstancesMethodStub, 2, {
  {"-closure", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToObjpattern}}
},
{"::nsf::methods::class::info::method", NsfClassInfoMethodMethodStub, 2, {
  {"infomethodsubcmd", 0|NSF_ARG_IS_ENUMERATION, 0, ConvertToInfomethodsubcmd},
  {"name", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::class::info::methods", NsfClassInfoMethodsMethodStub, 6, {
  {"-callprotection", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToCallprotection},
  {"-incontext", 0, 0, ConvertToString},
  {"-methodtype", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToMethodtype},
  {"-nomixins", 0, 0, ConvertToString},
  {"-path", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToString}}
},
{"::nsf::methods::class::info::mixinof", NsfClassInfoMixinOfMethodStub, 3, {
  {"-closure", 0, 0, ConvertToString},
  {"-scope", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToScope},
  {"pattern", 0, 0, ConvertToObjpattern}}
},
{"::nsf::methods::class::info::mixinclasses", NsfClassInfoMixinclassesMethodStub, 3, {
  {"-closure", 0, 0, ConvertToString},
  {"-guards", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToObjpattern}}
},
{"::nsf::methods::class::info::mixinguard", NsfClassInfoMixinguardMethodStub, 1, {
  {"mixin", NSF_ARG_REQUIRED, 0, ConvertToString}}
},
{"::nsf::methods::class::info::subclass", NsfClassInfoSubclassMethodStub, 2, {
  {"-closure", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToObjpattern}}
},
{"::nsf::methods::class::info::superclass", NsfClassInfoSuperclassMethodStub, 2, {
  {"-closure", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToTclobj}}
},
{"::nsf::alias", NsfAliasCmdStub, 5, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"-per-object", 0, 0, ConvertToString},
  {"methodName", NSF_ARG_REQUIRED, 0, ConvertToString},
  {"-frame", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToFrame},
  {"cmdName", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::assertion", NsfAssertionCmdStub, 3, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"assertionsubcmd", NSF_ARG_REQUIRED|NSF_ARG_IS_ENUMERATION, 1, ConvertToAssertionsubcmd},
  {"arg", 0, 0, ConvertToTclobj}}
},
{"::nsf::colon", NsfColonCmdStub, 1, {
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::configure", NsfConfigureCmdStub, 2, {
  {"configureoption", NSF_ARG_REQUIRED|NSF_ARG_IS_ENUMERATION, 0, ConvertToConfigureoption},
  {"value", 0, 0, ConvertToTclobj}}
},
{"::nsf::createobjectsystem", NsfCreateObjectSystemCmdStub, 3, {
  {"rootClass", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"rootMetaClass", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"systemMethods", 0, 0, ConvertToTclobj}}
},
{"::nsf::current", NsfCurrentCmdStub, 1, {
  {"currentoption", 0|NSF_ARG_IS_ENUMERATION, 0, ConvertToCurrentoption}}
},
{"::nsf::__db_run_assertions", NsfDebugRunAssertionsCmdStub, 0, {
  }
},
{"::nsf::dispatch", NsfDispatchCmdStub, 4, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"-frame", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToFrame},
  {"command", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::existsvar", NsfExistsVarCmdStub, 2, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"varName", NSF_ARG_REQUIRED, 0, ConvertToString}}
},
{"::nsf::finalize", NsfFinalizeObjCmdStub, 0, {
  }
},
{"::nsf::forward", NsfForwardCmdStub, 11, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"-per-object", 0, 0, ConvertToString},
  {"method", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"-default", 0, 1, ConvertToTclobj},
  {"-earlybinding", 0, 0, ConvertToString},
  {"-methodprefix", 0, 1, ConvertToTclobj},
  {"-objframe", 0, 0, ConvertToString},
  {"-onerror", 0, 1, ConvertToTclobj},
  {"-verbose", 0, 0, ConvertToString},
  {"target", 0, 0, ConvertToTclobj},
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::importvar", NsfImportvarCmdStub, 2, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::interp", NsfInterpObjCmdStub, 2, {
  {"name", NSF_ARG_REQUIRED, 0, ConvertToString},
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::invalidateobjectparameter", NsfInvalidateObjectParameterCmdStub, 1, {
  {"class", NSF_ARG_REQUIRED, 0, ConvertToClass}}
},
{"::nsf::is", NsfIsCmdStub, 3, {
  {"-complain", 0, 0, ConvertToString},
  {"constraint", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"value", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::isobject", NsfIsObjectCmdStub, 1, {
  {"value", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::method", NsfMethodCmdStub, 8, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"-inner-namespace", 0, 0, ConvertToString},
  {"-per-object", 0, 0, ConvertToString},
  {"methodName", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"arguments", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"body", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"-precondition", 0, 1, ConvertToTclobj},
  {"-postcondition", 0, 1, ConvertToTclobj}}
},
{"::nsf::methodproperty", NsfMethodPropertyCmdStub, 5, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"-per-object", 0, 0, ConvertToString},
  {"methodName", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"methodproperty", NSF_ARG_REQUIRED|NSF_ARG_IS_ENUMERATION, 0, ConvertToMethodproperty},
  {"value", 0, 0, ConvertToTclobj}}
},
{"::nsf::my", NsfMyCmdStub, 3, {
  {"-local", 0, 0, ConvertToString},
  {"methodName", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::nscopycmds", NsfNSCopyCmdsCmdStub, 2, {
  {"fromNs", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"toNs", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::nscopyvars", NsfNSCopyVarsCmdStub, 2, {
  {"fromNs", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"toNs", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::next", NsfNextCmdStub, 1, {
  {"arguments", 0, 0, ConvertToTclobj}}
},
{"::nsf::proc", NsfProcCmdStub, 4, {
  {"-ad", 0, 0, ConvertToString},
  {"procName", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"arguments", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"body", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::qualify", NsfQualifyObjCmdStub, 1, {
  {"objectName", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::relation", NsfRelationCmdStub, 3, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"relationtype", NSF_ARG_REQUIRED|NSF_ARG_IS_ENUMERATION, 0, ConvertToRelationtype},
  {"value", 0, 0, ConvertToTclobj}}
},
{"::nsf::self", NsfSelfCmdStub, 0, {
  }
},
{"::nsf::setvar", NsfSetVarCmdStub, 3, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"varName", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"value", 0, 0, ConvertToTclobj}}
},
{"::nsf::setter", NsfSetterCmdStub, 3, {
  {"object", NSF_ARG_REQUIRED, 0, ConvertToObject},
  {"-per-object", 0, 0, ConvertToString},
  {"parameter", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::__db_show_stack", NsfShowStackCmdStub, 0, {
  }
},
{"::nsf::methods::object::autoname", NsfOAutonameMethodStub, 3, {
  {"-instance", 0, 0, ConvertToString},
  {"-reset", 0, 0, ConvertToString},
  {"name", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::object::cleanup", NsfOCleanupMethodStub, 0, {
  }
},
{"::nsf::methods::object::configure", NsfOConfigureMethodStub, 1, {
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::methods::object::destroy", NsfODestroyMethodStub, 0, {
  }
},
{"::nsf::methods::object::exists", NsfOExistsMethodStub, 1, {
  {"varName", NSF_ARG_REQUIRED, 0, ConvertToString}}
},
{"::nsf::methods::object::filterguard", NsfOFilterGuardMethodStub, 2, {
  {"filter", NSF_ARG_REQUIRED, 0, ConvertToString},
  {"guard", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::object::instvar", NsfOInstvarMethodStub, 1, {
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::methods::object::mixinguard", NsfOMixinGuardMethodStub, 2, {
  {"mixin", NSF_ARG_REQUIRED, 0, ConvertToTclobj},
  {"guard", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::object::noinit", NsfONoinitMethodStub, 0, {
  }
},
{"::nsf::methods::object::requirenamespace", NsfORequireNamespaceMethodStub, 0, {
  }
},
{"::nsf::methods::object::residualargs", NsfOResidualargsMethodStub, 1, {
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::methods::object::uplevel", NsfOUplevelMethodStub, 1, {
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::methods::object::upvar", NsfOUpvarMethodStub, 1, {
  {"args", 0, 0, ConvertToNothing}}
},
{"::nsf::methods::object::volatile", NsfOVolatileMethodStub, 0, {
  }
},
{"::nsf::methods::object::vwait", NsfOVwaitMethodStub, 1, {
  {"varName", NSF_ARG_REQUIRED, 0, ConvertToString}}
},
{"::nsf::methods::object::info::children", NsfObjInfoChildrenMethodStub, 2, {
  {"-type", 0, 1, ConvertToClass},
  {"pattern", 0, 0, ConvertToString}}
},
{"::nsf::methods::object::info::class", NsfObjInfoClassMethodStub, 0, {
  }
},
{"::nsf::methods::object::info::filterguard", NsfObjInfoFilterguardMethodStub, 1, {
  {"fileName", NSF_ARG_REQUIRED, 0, ConvertToString}}
},
{"::nsf::methods::object::info::filtermethods", NsfObjInfoFiltermethodsMethodStub, 3, {
  {"-guards", 0, 0, ConvertToString},
  {"-order", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToString}}
},
{"::nsf::methods::object::info::forward", NsfObjInfoForwardMethodStub, 2, {
  {"-definition", 0, 0, ConvertToString},
  {"name", 0, 0, ConvertToString}}
},
{"::nsf::methods::object::info::hasmixin", NsfObjInfoHasMixinMethodStub, 1, {
  {"class", NSF_ARG_REQUIRED, 0, ConvertToClass}}
},
{"::nsf::methods::object::info::hastype", NsfObjInfoHasTypeMethodStub, 1, {
  {"class", NSF_ARG_REQUIRED, 0, ConvertToClass}}
},
{"::nsf::methods::object::info::hasnamespace", NsfObjInfoHasnamespaceMethodStub, 0, {
  }
},
{"::nsf::methods::object::info::is", NsfObjInfoIsMethodStub, 1, {
  {"objectkind", NSF_ARG_REQUIRED|NSF_ARG_IS_ENUMERATION, 0, ConvertToObjectkind}}
},
{"::nsf::methods::object::info::lookupfilter", NsfObjInfoLookupFilterMethodStub, 1, {
  {"filter", NSF_ARG_REQUIRED, 0, ConvertToString}}
},
{"::nsf::methods::object::info::lookupmethod", NsfObjInfoLookupMethodMethodStub, 1, {
  {"name", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::object::info::lookupmethods", NsfObjInfoLookupMethodsMethodStub, 7, {
  {"-callprotection", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToCallprotection},
  {"-incontext", 0, 0, ConvertToString},
  {"-methodtype", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToMethodtype},
  {"-nomixins", 0, 0, ConvertToString},
  {"-path", 0, 0, ConvertToString},
  {"-source", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToSource},
  {"pattern", 0, 0, ConvertToString}}
},
{"::nsf::methods::object::info::lookupslots", NsfObjInfoLookupSlotsMethodStub, 1, {
  {"-type", 0, 1, ConvertToClass}}
},
{"::nsf::methods::object::info::method", NsfObjInfoMethodMethodStub, 2, {
  {"infomethodsubcmd", 0|NSF_ARG_IS_ENUMERATION, 0, ConvertToInfomethodsubcmd},
  {"name", NSF_ARG_REQUIRED, 0, ConvertToTclobj}}
},
{"::nsf::methods::object::info::methods", NsfObjInfoMethodsMethodStub, 6, {
  {"-callprotection", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToCallprotection},
  {"-incontext", 0, 0, ConvertToString},
  {"-methodtype", 0|NSF_ARG_IS_ENUMERATION, 1, ConvertToMethodtype},
  {"-nomixins", 0, 0, ConvertToString},
  {"-path", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToString}}
},
{"::nsf::methods::object::info::mixinclasses", NsfObjInfoMixinclassesMethodStub, 3, {
  {"-guards", 0, 0, ConvertToString},
  {"-order", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToObjpattern}}
},
{"::nsf::methods::object::info::mixinguard", NsfObjInfoMixinguardMethodStub, 1, {
  {"mixin", NSF_ARG_REQUIRED, 0, ConvertToString}}
},
{"::nsf::methods::object::info::parent", NsfObjInfoParentMethodStub, 0, {
  }
},
{"::nsf::methods::object::info::precedence", NsfObjInfoPrecedenceMethodStub, 2, {
  {"-intrinsic", 0, 0, ConvertToString},
  {"pattern", 0, 0, ConvertToString}}
},
{"::nsf::methods::object::info::vars", NsfObjInfoVarsMethodStub, 1, {
  {"pattern", 0, 0, ConvertToString}}
},{NULL}
};

