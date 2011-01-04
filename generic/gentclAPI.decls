# -*- Tcl -*-
#
# This script is sourced by the C-code generator gentclAPI.tcl in the
# same directory. It is also used by the nx::doc toolkit to generate
# the authorative language reference documents.

# namespaces for types of methods
array set ns {
  nsfCmd           "::nsf"
  objectMethod     "::nsf::methods::object"
  objectInfoMethod "::nsf::methods::object::info"
  classMethod      "::nsf::methods::class"
  classInfoMethod  "::nsf::methods::class::info"
  checkMethod      "::nsf::cmd::ParameterType"
}

#
# Next Scripting commands
#
nsfCmd __db_show_stack NsfShowStackCmd {
}
nsfCmd __db_run_assertions NsfDebugRunAssertionsCmd {
}
nsfCmd alias NsfAliasCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -required 0}
  {-argName "methodName" -required 1}
  {-argName "-frame" -required 0 -nrargs 1 -type "method|object|default" -default "default"}
  {-argName "cmdName" -required 1 -type tclobj}
}
nsfCmd assertion NsfAssertionCmd {
  {-argName "object" -required 1 -type object}
  {-argName "assertionsubcmd" -required 1 -nrargs 1 -type "check|object-invar|class-invar"}
  {-argName "arg" -required 0 -type tclobj}
}
nsfCmd configure NsfConfigureCmd {
  {-argName "configureoption" -required 1 -type "debug|filter|softrecreate|objectsystems|keepinitcmd|checkresults|checkarguments"}
  {-argName "value" -required 0 -type tclobj}
}
nsfCmd createobjectsystem NsfCreateObjectSystemCmd {
  {-argName "rootClass" -required 1 -type tclobj}
  {-argName "rootMetaClass" -required 1 -type tclobj}
  {-argName "systemMethods" -required 0 -type tclobj}
}
nsfCmd dispatch NsfDispatchCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-frame" -required 0 -nrargs 1 -type "method|object|default" -default "default"}
  {-argName "command" -required 1 -type tclobj}
  {-argName "args"  -type args}
}
nsfCmd colon NsfColonCmd {
  {-argName "args" -type allargs}
}
nsfCmd existsvar NsfExistsVarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "varName" -required 1}
}
nsfCmd finalize NsfFinalizeObjCmd {
}

nsfCmd forward NsfForwardCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "method" -required 1 -type tclobj}
  {-argName "-default" -nrargs 1 -type tclobj}
  {-argName "-earlybinding"}
  {-argName "-methodprefix" -nrargs 1 -type tclobj}
  {-argName "-objframe"}
  {-argName "-onerror" -nrargs 1 -type tclobj}
  {-argName "-verbose"}
  {-argName "target" -type tclobj}
  {-argName "args" -type args}
}
nsfCmd importvar NsfImportvarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "args" -type args}
}
nsfCmd interp NsfInterpObjCmd {
  {-argName "name" -required 1}
  {-argName "args" -type allargs}
}
nsfCmd invalidateobjectparameter NsfInvalidateObjectParameterCmd {
  {-argName "class" -required 1 -type class}
}
nsfCmd is NsfIsCmd {
  {-argName "-complain"}
  {-argName "constraint" -required 1 -type tclobj}
  {-argName "value" -required 1 -type tclobj}
}
nsfCmd isobject NsfIsObjectCmd {
  {-argName "value" -required 1 -type tclobj}
}
nsfCmd method NsfMethodCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-inner-namespace"}
  {-argName "-per-object"}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
  {-argName "-precondition"  -nrargs 1 -type tclobj}
  {-argName "-postcondition" -nrargs 1 -type tclobj}
}

nsfCmd proc NsfProcCmd {
  {-argName "-ad" -required 0}
  {-argName "procName" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
}

nsfCmd methodproperty NsfMethodPropertyCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "methodproperty" -required 1 -type "class-only|call-protected|redefine-protected|returns|slotcontainer|slotobj"}
  {-argName "value" -type tclobj}
}
nsfCmd my NsfMyCmd {
  {-argName "-local"}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "args" -type args}
}
nsfCmd next NsfNextCmd {
  {-argName "arguments" -required 0 -type tclobj}
}
nsfCmd nscopycmds NsfNSCopyCmdsCmd {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
nsfCmd nscopyvars NsfNSCopyVarsCmd {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
nsfCmd qualify NsfQualifyObjCmd {
  {-argName "objectName" -required 1 -type tclobj}
}
nsfCmd relation NsfRelationCmd {
  {-argName "object"  -required 1 -type object}
  {-argName "relationtype" -required 1 -type "object-mixin|class-mixin|object-filter|class-filter|class|superclass|rootclass"}
  {-argName "value" -required 0 -type tclobj}
}
nsfCmd current NsfCurrentCmd {
  {-argName "currentoption" -required 0 -type "proc|method|methodpath|object|class|activelevel|args|activemixin|calledproc|calledmethod|calledclass|callingproc|callingmethod|callingclass|callinglevel|callingobject|filterreg|isnextcall|next"}
}
nsfCmd self NsfSelfCmd {
}
nsfCmd setter NsfSetterCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object"}
  {-argName "parameter" -required 1 -type tclobj}
}
nsfCmd setvar NsfSetVarCmd {
  {-argName "object" -required 1 -type object}
  {-argName "varName" -required 1 -type tclobj}
  {-argName "value" -required 0 -type tclobj}
}


#
# object methods
#
objectMethod autoname NsfOAutonameMethod {
  {-argName "-instance"}
  {-argName "-reset"}
  {-argName "name" -required 1 -type tclobj}
}

objectMethod cleanup NsfOCleanupMethod {
}

objectMethod configure NsfOConfigureMethod {
  {-argName "args" -type allargs}
}

objectMethod destroy NsfODestroyMethod {
}

objectMethod exists NsfOExistsMethod {
  {-argName "varName" -required 1}
}

objectMethod filterguard NsfOFilterGuardMethod {
  {-argName "filter" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}

objectMethod instvar NsfOInstvarMethod {
  {-argName "args" -type allargs}
}

objectMethod mixinguard NsfOMixinGuardMethod {
  {-argName "mixin" -required 1 -type tclobj}
  {-argName "guard" -required 1 -type tclobj}
}

objectMethod noinit NsfONoinitMethod {
}

objectMethod requirenamespace NsfORequireNamespaceMethod {
}

objectMethod residualargs NsfOResidualargsMethod {
  {-argName "args" -type allargs}
}

objectMethod uplevel NsfOUplevelMethod {
  {-argName "args" -type allargs}
}

objectMethod upvar NsfOUpvarMethod {
  {-argName "args" -type allargs}
}

objectMethod volatile NsfOVolatileMethod {
}

objectMethod vwait NsfOVwaitMethod {
  {-argName "varName" -required 1}
}

#
# class methods
#

classMethod alloc NsfCAllocMethod {
  {-argName "objectName" -required 1 -type tclobj}
}

classMethod create NsfCCreateMethod {
  {-argName "objectName" -required 1}
  {-argName "args" -type allargs}
}

classMethod dealloc NsfCDeallocMethod {
  {-argName "object" -required 1 -type tclobj}
}

classMethod filterguard NsfCFilterGuardMethod {
  {-argName "filter" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}

classMethod mixinguard NsfCMixinGuardMethod {
  {-argName "mixin" -required 1 -type tclobj}
  {-argName "guard" -required 1 -type tclobj}
}

classMethod new NsfCNewMethod {
  {-argName "-childof" -type object -nrargs 1}
  {-argName "args" -required 0 -type args}
}

classMethod recreate NsfCRecreateMethod {
  {-argName "objectName" -required 1 -type tclobj}
  {-argName "args" -type allargs}
}

#
# info object methods
#
objectInfoMethod children NsfObjInfoChildrenMethod {
  {-argName "-type" -required 0 -nrargs 1 -type class}
  {-argName "pattern" -required 0}
}
objectInfoMethod class NsfObjInfoClassMethod {
}
objectInfoMethod filterguard NsfObjInfoFilterguardMethod {
  {-argName "fileName" -required 1}
}
objectInfoMethod filtermethods NsfObjInfoFiltermethodsMethod {
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern"}
}
objectInfoMethod forward NsfObjInfoForwardMethod {
  {-argName "-definition"}
  {-argName "name"}
}
objectInfoMethod hasmixin NsfObjInfoHasMixinMethod {
  {-argName "class" -required 1 -type class}
}
objectInfoMethod hasnamespace NsfObjInfoHasnamespaceMethod {
}
objectInfoMethod hastype NsfObjInfoHasTypeMethod {
  {-argName "class" -required 1 -type class}
}
objectInfoMethod is NsfObjInfoIsMethod {
  {-argName "objectkind" -required 1 -type "class|baseclass|metaclass"}
}
objectInfoMethod lookupfilter NsfObjInfoLookupFilterMethod {
  {-argName "filter" -required 1}
}
objectInfoMethod lookupmethod NsfObjInfoLookupMethodMethod {
  {-argName "name" -required 1 -type tclobj}
}
objectInfoMethod lookupmethods NsfObjInfoLookupMethodsMethod {
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default all}
  {-argName "-incontext"}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-nomixins"}
  {-argName "-path"}
  {-argName "-source" -nrargs 1 -type "all|application|baseclasses"}
  {-argName "pattern" -required 0}
}
objectInfoMethod lookupslots NsfObjInfoLookupSlotsMethod {
  {-argName "-type" -required 0 -nrargs 1 -type class}
}
objectInfoMethod method NsfObjInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|definition|handle|parameter|parametersyntax|type|precondition|postcondition|submethods"}
  {-argName "name" -required 1 -type tclobj}
}
objectInfoMethod methods NsfObjInfoMethodsMethod {
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-incontext"}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter|nsfproc"}
  {-argName "-nomixins"}
  {-argName "-path"}
  {-argName "pattern"}
}

objectInfoMethod mixinclasses NsfObjInfoMixinclassesMethod {
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern" -type objpattern}
}
objectInfoMethod mixinguard NsfObjInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
objectInfoMethod parent NsfObjInfoParentMethod {
}
objectInfoMethod precedence NsfObjInfoPrecedenceMethod {
  {-argName "-intrinsic"}
  {-argName "pattern" -required 0}
}
objectInfoMethod vars NsfObjInfoVarsMethod {
  {-argName "pattern" -required 0}
}

#
# info class methods
#
classInfoMethod filterguard NsfClassInfoFilterguardMethod {
  {-argName "filter" -required 1}
}
classInfoMethod filtermethods NsfClassInfoFiltermethodsMethod {
  {-argName "-guards"}
  {-argName "pattern"}
}
classInfoMethod forward NsfClassInfoForwardMethod {
  {-argName "-definition"}
  {-argName "name"}
}
classInfoMethod heritage NsfClassInfoHeritageMethod {
  {-argName "pattern"}
}
classInfoMethod instances NsfClassInfoInstancesMethod {
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}

classInfoMethod method NsfClassInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|definition|handle|parameter|parametersyntax|type|precondition|postcondition|submethods"}
  {-argName "name" -required 1 -type tclobj}
}
classInfoMethod methods NsfClassInfoMethodsMethod {
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-incontext"}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter|nsfproc"}
  {-argName "-nomixins"}
  {-argName "-path"}
  {-argName "pattern"}
}
classInfoMethod mixinclasses NsfClassInfoMixinclassesMethod {
  {-argName "-closure"}
  {-argName "-guards"}
  {-argName "pattern" -type objpattern}
}
classInfoMethod mixinguard NsfClassInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
classInfoMethod mixinof  NsfClassInfoMixinOfMethod {
  {-argName "-closure"}
  {-argName "-scope" -required 0 -nrargs 1 -type "all|class|object"}
  {-argName "pattern" -type objpattern}
}
classInfoMethod subclass NsfClassInfoSubclassMethod {
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}
classInfoMethod superclass NsfClassInfoSuperclassMethod {
  {-argName "-closure"}
  {-argName "pattern" -type tclobj}
}


#
# check methods
#
# checkMethod required NsfCheckRequiredArgs {
#   {-argName "name" -required 1}
#   {-argName "value" -required 0 -type tclobj}
# }
# checkMethod boolean NsfCheckBooleanArgs {
#   {-argName "name" -required 1}
#   {-argName "value" -required 0 -type tclobj}
# }
