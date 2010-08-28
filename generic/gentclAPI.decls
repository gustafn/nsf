# -*- Tcl -*-
#
# This script is sourced by the C-code generator gentclAPI.tcl in the
# same directory. It is also used by the nx::doc toolkit to generate
# the authorative language reference documents.

# namespaces for types of methods
array set ns {
  xotclCmd    "::nsf"
  objectMethod "::nsf::cmd::Object"
  objectInfoMethod "::nsf::cmd::ObjectInfo2"
  classMethod  "::nsf::cmd::Class"
  classInfoMethod "::nsf::cmd::ClassInfo2"
  checkMethod  "::nsf::cmd::ParameterType"
  infoClassMethod  "::nsf::cmd::ClassInfo"
  infoObjectMethod  "::nsf::cmd::ObjectInfo"
}

#
# XOTcl commands
#
xotclCmd alias XOTclAliasCmd {
  {-argName "object" -type object}
  {-argName "-per-object"}
  {-argName "methodName"}
  {-argName "-nonleaf"}
  {-argName "-objscope"}
  {-argName "cmdName" -required 1 -type tclobj}
}
xotclCmd assertion XOTclAssertionCmd {
  {-argName "object" -type object}
  {-argName "assertionsubcmd" -required 1 -type "check|object-invar|class-invar"}
  {-argName "arg" -required 0 -type tclobj}
}

xotclCmd configure XOTclConfigureCmd {
  {-argName "configureoption" -required 1 -type "filter|softrecreate|objectsystems|keepinitcmd"}
  {-argName "value" -required 0 -type tclobj}
}
xotclCmd createobjectsystem XOTclCreateObjectSystemCmd {
  {-argName "rootClass" -required 1 -type tclobj}
  {-argName "rootMetaClass" -required 1 -type tclobj}
  {-argName "systemMethods" -required 0 -type tclobj}
}
xotclCmd deprecated XOTclDeprecatedCmd {
  {-argName "what" -required 1}
  {-argName "oldCmd" -required 1}
  {-argName "newCmd" -required 0}
}
xotclCmd dispatch XOTclDispatchCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-objscope"}
  {-argName "command" -required 1 -type tclobj}
  {-argName "args"  -type args}
}
xotclCmd colon XOTclColonCmd {
  {-argName "args" -type allargs}
}
xotclCmd existsvar XOTclExistsVarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "var" -required 1}
}
xotclCmd finalize XOTclFinalizeObjCmd {
}

xotclCmd forward XOTclForwardCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "method" -required 1 -type tclobj}
  {-argName "-default" -nrargs 1 -type tclobj}
  {-argName "-earlybinding"}
  {-argName "-methodprefix" -nrargs 1 -type tclobj}
  {-argName "-objscope"}
  {-argName "-onerror" -nrargs 1 -type tclobj}
  {-argName "-verbose"}
  {-argName "target" -type tclobj}
  {-argName "args" -type args}
}
xotclCmd importvar XOTclImportvarCmd {
  {-argName "object" -type object}
  {-argName "args" -type args}
}
xotclCmd interp XOTclInterpObjCmd {
  {-argName "name"}
  {-argName "args" -type allargs}
}
xotclCmd invalidateobjectparameter XOTclInvalidateObjectParameterCmd {
  {-argName "class" -type class}
}
xotclCmd is XOTclIsCmd {
  {-argName "value" -required 1 -type tclobj}
  {-argName "constraint" -required 1 -type tclobj}
  {-argName "-hasmixin" -required 0 -nrargs 1 -type tclobj}
  {-argName "-type" -required 0 -nrargs 1 -type tclobj}
  {-argName "arg" -required 0 -type tclobj}
}
xotclCmd method XOTclMethodCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-inner-namespace"}
  {-argName "-per-object"}
  {-argName "-public"}
  {-argName "name" -required 1 -type tclobj}
  {-argName "args" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
  {-argName "-precondition"  -nrargs 1 -type tclobj}
  {-argName "-postcondition" -nrargs 1 -type tclobj}
}
xotclCmd methodproperty XOTclMethodPropertyCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "methodproperty" -required 1 -type "class-only|protected|redefine-protected|returns|slotobj"}
  {-argName "value" -type tclobj}
}
xotclCmd my XOTclMyCmd {
  {-argName "-local"}
  {-argName "method" -required 1 -type tclobj}
  {-argName "args" -type args}
}

xotclCmd namespace_copycmds XOTclNSCopyCmds {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
xotclCmd namespace_copyvars XOTclNSCopyVars {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
xotclCmd objectproperty XOTclObjectpropertyCmd {
  {-argName "object" -required 1 -type tclobj}
  {-argName "objectkind" -type "type|object|class|baseclass|metaclass|hasmixin"}
  {-argName "value" -required 0 -type tclobj}
}
xotclCmd parametercheck XOTclParametercheckCmd {
  {-argName "-nocomplain"}
  {-argName "param" -type tclobj}
  {-argName "value" -required 1 -type tclobj}
}
xotclCmd __qualify XOTclQualifyObjCmd {
  {-argName "name" -required 1 -type tclobj}
}
xotclCmd relation XOTclRelationCmd {
  {-argName "object" -type object}
  {-argName "relationtype" -required 1 -type "object-mixin|class-mixin|object-filter|class-filter|class|superclass|rootclass"}
  {-argName "value" -required 0 -type tclobj}
}
xotclCmd current XOTclCurrentCmd {
  {-argName "currentoption" -required 0 -type "proc|method|object|class|activelevel|args|activemixin|calledproc|calledmethod|calledclass|callingproc|callingmethod|callingclass|callinglevel|callingobject|filterreg|isnextcall|next"}
}

xotclCmd setvar XOTclSetVarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "variable" -required 1 -type tclobj}
  {-argName "value" -required 0 -type tclobj}
}
xotclCmd setter XOTclSetterCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "parameter" -type tclobj}
}

objectMethod autoname XOTclOAutonameMethod {
  {-argName "-instance"}
  {-argName "-reset"}
  {-argName "name" -required 1 -type tclobj}
}

objectMethod cleanup XOTclOCleanupMethod {
}

objectMethod configure XOTclOConfigureMethod {
  {-argName "args" -type allargs}
}

objectMethod destroy XOTclODestroyMethod {
}

objectMethod exists XOTclOExistsMethod {
  {-argName "var" -required 1}
}

objectMethod filterguard XOTclOFilterGuardMethod {
  {-argName "filter" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}

objectMethod instvar XOTclOInstVarMethod {
  {-argName "args" -type allargs}
}

objectMethod mixinguard XOTclOMixinGuardMethod {
  {-argName "mixin" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}

# objectMethod __next XOTclONextMethod {
#  {-argName "args" -type allargs}
#}

objectMethod noinit XOTclONoinitMethod {
}

objectMethod requireNamespace XOTclORequireNamespaceMethod {
}

objectMethod residualargs XOTclOResidualargsMethod {
  {-argName "args" -type allargs}
}

objectMethod uplevel XOTclOUplevelMethod {
  {-argName "args" -type allargs}
}

objectMethod upvar XOTclOUpvarMethod {
  {-argName "args" -type allargs}
}

objectMethod volatile XOTclOVolatileMethod {
}

objectMethod vwait XOTclOVwaitMethod {
  {-argName "varname" -required 1}
}

#
# info object methods
#
objectInfoMethod callable XOTclObjInfoCallableMethod {
  {-argName "infocallablesubcmd" -nrargs 1 -type "filter|method|methods" -required 1}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default all}
  {-argName "-application"}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern" -required 0}
}
objectInfoMethod children XOTclObjInfoChildrenMethod {
  {-argName "pattern" -required 0}
}
objectInfoMethod class XOTclObjInfoClassMethod {
}
objectInfoMethod filtermethods XOTclObjInfoFiltermethodsMethod {
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern"}
}
objectInfoMethod filterguard XOTclObjInfoFilterguardMethod {
  {-argName "filter" -required 1}
}
objectInfoMethod forward XOTclObjInfoForwardMethod {
  {-argName "-definition"}
  {-argName "name"}
}
objectInfoMethod hasnamespace XOTclObjInfoHasnamespaceMethod {
}
objectInfoMethod method XOTclObjInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|definition|handle|parameter|parametersyntax|type|precondition|postcondition"}
  {-argName "name"}
}
objectInfoMethod methods XOTclObjInfoMethodsMethod {
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern"}
}

objectInfoMethod mixinclasses XOTclObjInfoMixinclassesMethod {
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern" -type objpattern}
}
objectInfoMethod mixinguard XOTclObjInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
objectInfoMethod parent XOTclObjInfoParentMethod {
}
objectInfoMethod precedence XOTclObjInfoPrecedenceMethod {
  {-argName "-intrinsic"}
  {-argName "pattern" -required 0}
}
objectInfoMethod slotobjects XOTclObjInfoSlotObjectsMethod {
  {-argName "pattern" -required 0}
}
objectInfoMethod vars XOTclObjInfoVarsMethod {
  {-argName "pattern" -required 0}
}

#
# info class methods
#
classInfoMethod filtermethods XOTclClassInfoFiltermethodsMethod {
  {-argName "-guards"}
  {-argName "pattern"}
}
classInfoMethod filterguard XOTclClassInfoFilterguardMethod {
  {-argName "filter" -required 1}
}
classInfoMethod forward XOTclClassInfoForwardMethod {
  {-argName "-definition"}
  {-argName "name"}
}
classInfoMethod heritage XOTclClassInfoHeritageMethod {
  {-argName "pattern"}
}
classInfoMethod instances XOTclClassInfoInstancesMethod {
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}

classInfoMethod method XOTclClassInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|definition|handle|parameter|parametersyntax|type|precondition|postcondition"}
  {-argName "name"}
}
classInfoMethod methods XOTclClassInfoMethodsMethod {
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern"}
}
classInfoMethod mixinclasses XOTclClassInfoMixinclassesMethod {
  {-argName "-closure"}
  {-argName "-guards"}
  {-argName "pattern" -type objpattern}
}
classInfoMethod mixinguard XOTclClassInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
classInfoMethod mixinof  XOTclClassInfoMixinOfMethod {
  {-argName "-closure"}
  {-argName "-scope" -required 0 -nrargs 1 -type "all|class|object"}
  {-argName "pattern" -type objpattern}
}
classInfoMethod slots XOTclClassInfoSlotsMethod {
}
classInfoMethod subclass XOTclClassInfoSubclassMethod {
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
classInfoMethod superclass XOTclClassInfoSuperclassMethod {
  {-argName "-closure"}
  {-argName "pattern" -type tclobj}
}

#
# class methods
#

classMethod alloc XOTclCAllocMethod {
  {-argName "name" -required 1 -type tclobj}
}

classMethod create XOTclCCreateMethod {
  {-argName "name" -required 1}
  {-argName "args" -type allargs}
}

classMethod dealloc XOTclCDeallocMethod {
  {-argName "object" -required 1 -type tclobj}
}

classMethod new XOTclCNewMethod {
  {-argName "-childof" -type object -nrargs 1}
  {-argName "args" -required 0 -type args}
}
classMethod filterguard XOTclCFilterGuardMethod {
  {-argName "filter" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}
classMethod mixinguard XOTclCMixinGuardMethod {
  {-argName "mixin" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}

classMethod recreate XOTclCRecreateMethod {
  {-argName "name" -required 1 -type tclobj}
  {-argName "args" -type allargs}
}
#
# check methods
#
# checkMethod required XOTclCheckRequiredArgs {
#   {-argName "name" -required 1}
#   {-argName "value" -required 0 -type tclobj}
# }
# checkMethod boolean XOTclCheckBooleanArgs {
#   {-argName "name" -required 1}
#   {-argName "value" -required 0 -type tclobj}
# }
