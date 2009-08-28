# -*- Tcl -*-
#
# This script is sourced by the C-code generator gentclAPI.tcl in the
# same directory

# namespaces for types of methods
array set ns {
  xotclCmd    "::xotcl"
  objectMethod "::xotcl::cmd::Object"
  classMethod  "::xotcl::cmd::Class"
  checkMethod  "::xotcl::cmd::NonposArgs"
  infoClassMethod  "::xotcl::cmd::ClassInfo"
  infoObjectMethod  "::xotcl::cmd::ObjectInfo"
}

#
# XOTcl commands
#
xotclCmd alias XOTclAliasCmd {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
  {-argName "-objscope"}
  {-argName "-per-object"}
  {-argName "-protected"}
  {-argName "cmdName" -required 1 -type tclobj}
}
xotclCmd configure XOTclConfigureCmd {
  {-argName "configureoption" -required 1 -type "filter|softrecreate|cacheinterface"}
  {-argName "value" -required 0 -type tclobj}
}
xotclCmd createobjectsystem XOTclCreateObjectSystemCmd {
  {-argName "rootClass" -required 1}
  {-argName "rootMetaClass" -required 1}
}
xotclCmd deprecated XOTclDeprecatedCmd {
  {-argName "oldCmd" -required 1}
  {-argName "newCmd" -required 0}
}
xotclCmd finalize XOTclFinalizeObjCmd {
}
xotclCmd instvar XOTclInstvarCmd {
  {-argName "args" -type allargs}
}
xotclCmd methodproperty XOTclMethodPropertyCmd {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
  {-argName "-per-object"}
  {-argName "methodproperty" -required 1 -type "protected|public|slotobj"}
  {-argName "value" -type tclobj}
}
xotclCmd my XOTclMyCmd {
  {-argName "-local"}
  {-argName "method" -required 1 -type tclobj}
  {-argName "args" -type args}
}
xotclCmd relation XOTclRelationCmd {
  {-argName "object" -required 1 -type object}
  {-argName "relationtype" -required 1 -type "mixin|instmixin|object-mixin|class-mixin|filter|instfilter|object-filter|class_filter|class|superclass|rootclass"}
  {-argName "value" -required 0 -type tclobj}
}
xotclCmd setinstvar XOTclSetInstvarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "variable" -required 1 -type tclobj}
  {-argName "value" -required 0 -type tclobj}
}
#
# object methods
#
objectMethod autoname XOTclOAutonameMethod {
  {-argName "-instance"}
  {-argName "-reset"}
  {-argName "name" -required 1 -type tclobj}
}
objectMethod check XOTclOCheckMethod {
  {-argName "flag" -required 1 -type tclobj}
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
objectMethod filtersearch XOTclOFilterSearchMethod {
  {-argName "filter" -required 1}
}
objectMethod forward XOTclOForwardMethod {
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
objectMethod instvar XOTclOInstVarMethod {
  {-argName "args" -type allargs}
}
objectMethod invar XOTclOInvariantsMethod {
  {-argName "invariantlist" -required 1 -type tclobj}
}
objectMethod isclass XOTclOIsClassMethod {
  {-argName "class" -type tclobj}
}
objectMethod ismetaclass XOTclOIsMetaClassMethod {
  {-argName "metaclass" -type tclobj}
}
objectMethod ismixin XOTclOIsMixinMethod {
  {-argName "class" -required 1 -type tclobj}
}
objectMethod isobject XOTclOIsObjectMethod {
  {-argName "object"  -required 1 -type tclobj}
}
objectMethod istype XOTclOIsTypeMethod {
  {-argName "class" -required 1 -type tclobj}
}
objectMethod mixinguard XOTclOMixinGuardMethod {
  {-argName "mixin" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}
objectMethod __next XOTclONextMethod {
  {-argName "args" -type allargs}
}
objectMethod noinit XOTclONoinitMethod {
}
objectMethod parametercmd XOTclOParametercmdMethod {
  {-argName "name" -required 1}
}
objectMethod proc XOTclOProcMethod {
  {-argName "name" -required 1 -type tclobj}
  {-argName "args" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
  {-argName "precondition"  -type tclobj}
  {-argName "postcondition" -type tclobj}
}
objectMethod procsearch XOTclOProcSearchMethod {
  {-argName "name" -required 1}
}
objectMethod requireNamespace XOTclORequireNamespaceMethod {
}
# "set" needed?
objectMethod set XOTclOSetMethod {
  {-argName "var" -required 1 -type tclobj}
  {-argName "value" -type tclobj}
}
objectMethod setvalues XOTclOSetvaluesMethod {
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
# class methods
#
classMethod alloc XOTclCAllocMethod {
  {-argName "name" -required 1}
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
classMethod instfilterguard XOTclCInstFilterGuardMethod {
  {-argName "filter" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}
classMethod instinvar XOTclCInvariantsMethod {
  {-argName "invariantlist" -required 1 -type tclobj}
}
classMethod instmixinguard XOTclCInstMixinGuardMethod {
  {-argName "mixin" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}
classMethod instparametercmd XOTclCInstParametercmdMethod {
  {-argName "name" -required 1}
}
classMethod instproc XOTclCInstProcMethod {
  {-argName "name" -required 1 -type tclobj}
  {-argName "args" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
  {-argName "precondition"  -type tclobj}
  {-argName "postcondition" -type tclobj}
}
classMethod classscopedinstproc XOTclCInstProcMethodC {
  {-argName "name" -required 1 -type tclobj}
  {-argName "args" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
  {-argName "precondition"  -type tclobj}
  {-argName "postcondition" -type tclobj}
}
classMethod instforward XOTclCInstForwardMethod {
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
# todo -protected for XOTclCInstForwardMethod
classMethod recreate XOTclCRecreateMethod {
  {-argName "name" -required 1 -type tclobj}
  {-argName "args" -type allargs}
}
classMethod unknown XOTclCUnknownMethod {
  {-argName "name" -required 1}
  {-argName "args" -type allargs}
}

#
# check methods
#
checkMethod required XOTclCheckRequiredArgs {
  {-argName "name" -required 1}
  {-argName "value" -required 0 -type tclobj}
}
checkMethod boolean XOTclCheckBooleanArgs {
  {-argName "name" -required 1}
  {-argName "value" -required 0 -type tclobj}
}

#
# info object methods
#
infoObjectMethod args XOTclObjInfoArgsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod body XOTclObjInfoBodyMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod check XOTclObjInfoCheckMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod children XOTclObjInfoChildrenMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
infoObjectMethod class XOTclObjInfoClassMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod commands XOTclObjInfoCommandsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
infoObjectMethod default XOTclObjInfoDefaultMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
  {-argName "arg" -required 1}
  {-argName "var" -required 1 -type tclobj}
}
infoObjectMethod filter XOTclObjInfoFilterMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-order"}
  {-argName "-guards"}
  {-argName "pattern"}
}
infoObjectMethod filterguard XOTclObjInfoFilterguardMethod {
  {-argName "object" -required 1 -type object}
  {-argName "filter" -required 1}
}
infoObjectMethod forward XOTclObjInfoForwardMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-definition"}
  {-argName "pattern"}
}
infoObjectMethod hasnamespace XOTclObjInfoHasnamespaceMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod invar XOTclObjInfoInvarMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod methods XOTclObjInfoMethodsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-noprocs"}
  {-argName "-nocmds"}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern"}
}
infoObjectMethod mixin XOTclObjInfoMixinMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern" -type objpattern}
}
infoObjectMethod mixinguard XOTclObjInfoMixinguardMethod {
  {-argName "object" -required 1 -type object}
  {-argName "mixin"  -required 1}
}
infoObjectMethod nonposargs XOTclObjInfoNonposargsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod parent XOTclObjInfoParentMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod parametercmd XOTclObjInfoParametercmdMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern"}
}
infoObjectMethod post XOTclObjInfoPostMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod pre XOTclObjInfoPreMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod precedence XOTclObjInfoPrecedenceMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-intrinsic"}
  {-argName "pattern" -required 0}
}
infoObjectMethod procs XOTclObjInfoProcsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
infoObjectMethod slotobjects XOTclObjInfoSlotObjectsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
infoObjectMethod vars XOTclObjInfoVarsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}


#
# info class methods
#
infoClassMethod heritage XOTclClassInfoHeritageMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "pattern"}
}
infoClassMethod instances XOTclClassInfoInstancesMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
infoClassMethod instargs XOTclClassInfoInstargsMethod {
  {-argName "class" -required 1 -type class}
  {-argName "methodName" -required 1}
}
infoClassMethod instbody XOTclClassInfoInstbodyMethod {
  {-argName "class" -required 1 -type class}
  {-argName "methodName" -required 1}
}
infoClassMethod instcommands XOTclClassInfoInstcommandsMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "pattern"}
}
infoClassMethod instdefault XOTclClassInfoInstdefaultMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "methodName" -required 1}
  {-argName "arg" -required 1}
  {-argName "var" -required 1 -type tclobj}
}
infoClassMethod instfilter XOTclClassInfoInstfilterMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "-guards"}
  {-argName "pattern"}
}
infoClassMethod instfilterguard XOTclClassInfoInstfilterguardMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "filter" -required 1}
}
infoClassMethod instforward XOTclClassInfoInstforwardMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-definition"}
  {-argName "pattern"}
}
infoClassMethod instinvar XOTclClassInfoInstinvarMethod {
  {-argName "class"  -required 1 -type class}
}
infoClassMethod instmixin XOTclClassInfoInstmixinMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "-guards"}
  {-argName "pattern" -type objpattern}
}
infoClassMethod instmixinguard XOTclClassInfoInstmixinguardMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "mixin" -required 1}
}
infoClassMethod instmixinof XOTclClassInfoInstmixinofMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
infoClassMethod instnonposargs XOTclClassInfoInstnonposargsMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "methodName" -required 1}
}
infoClassMethod instparametercmd XOTclClassInfoInstparametercmdMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "pattern"}
}
infoClassMethod instpost XOTclClassInfoInstpostMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "methodName" -required 1}
}
infoClassMethod instpre XOTclClassInfoInstpreMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "methodName" -required 1}
}
infoClassMethod instprocs XOTclClassInfoInstprocsMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "pattern"}
}
infoClassMethod mixinof XOTclClassInfoMixinofMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
infoClassMethod parameter XOTclClassInfoParameterMethod {
  {-argName "class"  -required 1 -type class}
}
infoClassMethod slots XOTclClassInfoSlotsMethod {
  {-argName "class"  -required 1 -type class}
}
infoClassMethod subclass XOTclClassInfoSubclassMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
infoClassMethod superclass XOTclClassInfoSuperclassMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type tclobj}
}

