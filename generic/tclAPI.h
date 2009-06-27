
enum {
  /* Object method definitions (1) */
  XOTclOAutonameMethodIdx,
  XOTclOCheckMethodIdx,
  XOTclOCleanupMethodIdx,
  XOTclOConfigureMethodIdx,
  XOTclODestroyMethodIdx,
  XOTclOExistsMethodIdx,
  XOTclOFilterGuardMethodIdx,
  XOTclOFilterSearchMethodIdx,
  XOTclOInstVarMethodIdx,
  XOTclOInvariantsMethodIdx,
  XOTclOIsClassMethodIdx,
  XOTclOIsMetaClassMethodIdx,
  XOTclOIsObjectMethodIdx,
  XOTclOIsTypeMethodIdx,
  XOTclOIsMixinMethodIdx,
  XOTclOMixinGuardMethodIdx,
  XOTclONextMethodIdx,
  XOTclONoinitMethodIdx,
  XOTclCParameterCmdMethodIdx,
  XOTclOProcMethodIdx,
  XOTclOProcSearchMethodIdx,
  XOTclORequireNamespaceMethodIdx,
  XOTclOSetMethodIdx, /***??**/
  XOTclOSetvaluesMethodIdx,
  XOTclOForwardMethodIdx,
  XOTclOUplevelMethodIdx,
  XOTclOUpvarMethodIdx,
  XOTclOVolatileMethodIdx,
  XOTclOVwaitMethodIdx,

  /* Class method definitions (2) */
  XOTclCAllocMethodIdx,
  XOTclCCreateMethodIdx,
  XOTclCDeallocMethodIdx,
  XOTclCNewMethodIdx,
  XOTclCInstFilterGuardMethodIdx,
  XOTclCInvariantsMethodIdx,
  XOTclCInstMixinGuardMethodIdx,
  XOTclCInstParameterCmdMethodIdx,
  XOTclCInstProcMethodIdx,
  XOTclCInstProcMethodCIdx,
  XOTclCInstForwardMethodIdx,
  XOTclCRecreateMethodIdx,
  XOTclCUnknownMethodIdx,

  /* Check method definitions (3) */
  XOTclCheckRequiredArgsIdx,
  /*XOTclCheckBooleanArgsIdx,  for boolean and switch, we use the same checker */
  XOTclCheckBooleanArgsIdx,

  /* Object info definitions (4) */
  XOTclObjInfoArgsMethodIdx,
  XOTclObjInfoBodyMethodIdx,
  XOTclObjInfoClassMethodIdx,
  XOTclObjInfoCommandsMethodIdx,
  XOTclObjInfoChildrenMethodIdx,
  XOTclObjInfoCheckMethodIdx,
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
  XOTclObjInfoParentMethodIdx,
  XOTclObjInfoParametercmdMethodIdx,
  XOTclObjInfoPostMethodIdx,
  XOTclObjInfoPreMethodIdx,
  XOTclObjInfoProcsMethodIdx,
  XOTclObjInfoPrecedenceMethodIdx,
  XOTclObjInfoSlotObjectsMethodIdx,
  XOTclObjInfoVarsMethodIdx,
       
  /* Class info definitions (5) */ 
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
  XOTclClassInfoInstprocsMethodIdx,
  XOTclClassInfoInstnonposargsMethodIdx,
  XOTclClassInfoInstparametercmdMethodIdx,
  XOTclClassInfoInstpreMethodIdx,
  XOTclClassInfoInstpostMethodIdx,
  XOTclClassInfoMixinofMethodIdx,
  XOTclClassInfoParameterMethodIdx,
  XOTclClassInfoSubclassMethodIdx,
  XOTclClassInfoSuperclassMethodIdx,
  XOTclClassInfoSlotsMethodIdx,

  methodIdxEND
} XOTclMethods;

static interfaceDefinition methodDefinitions[methodIdxEND];

interfaceDefinition xxx = {
  {"class",   1,0, "class"},
  {"-closure"},
  {"pattern", 0,0, "objpattern"}
};

methodDefinitions[XOTclClassInfoInstancesMethodIdx] = xxx;
/*
methodDefinitions[XOTclClassInfoInstancesMethodIdx] = {
  {"class",   1,0, "class"},
  {"-closure"},
  {"pattern", 0,0, "objpattern"}
};
*/
