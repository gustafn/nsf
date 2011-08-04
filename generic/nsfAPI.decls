# -*- Tcl -*-
#
# This script is sourced by the C-code generator gentclAPI.tcl in the
# same directory. It is also used by the nx::doc toolkit to generate
# the authorative language reference documents.

# namespaces for types of methods
array set ns {
  cmd              "::nsf"
  objectMethod     "::nsf::methods::object"
  objectInfoMethod "::nsf::methods::object::info"
  classMethod      "::nsf::methods::class"
  classInfoMethod  "::nsf::methods::class::info"
}

#
# Next Scripting commands
#
cmd __db_compile_epoch NsfDebugCompileEpoch {}
cmd __db_run_assertions NsfDebugRunAssertionsCmd {}
cmd __db_show_stack NsfShowStackCmd {}
cmd __profile_clear NsfProfileClearDataStub {}
cmd __profile_get NsfProfileGetDataStub {}
cmd __unset_unknown_args NsfUnsetUnknownArgsCmd {}

cmd configure NsfConfigureCmd {
  {-argName "configureoption" -required 1 -type "debug|dtrace|filter|profile|softrecreate|objectsystems|keepinitcmd|checkresults|checkarguments"}
  {-argName "value" -required 0 -type tclobj}
}
cmd colon NsfColonCmd {
  {-argName "args" -type allargs}
}
cmd finalize NsfFinalizeObjCmd {
}
cmd interp NsfInterpObjCmd {
  {-argName "name" -required 1}
  {-argName "args" -type allargs}
}
cmd invalidateobjectparameter NsfInvalidateObjectParameterCmd {
  {-argName "class" -required 1 -type class}
}
cmd is NsfIsCmd {
  {-argName "-complain"  -nrargs 0}
  {-argName "constraint" -required 1 -type tclobj}
  {-argName "value" -required 1 -type tclobj}
}

#
# method cmds
#
cmd "method::alias" NsfMethodAliasCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -required 0 -nrargs 0}
  {-argName "methodName" -required 1}
  {-argName "-frame" -required 0 -type "method|object|default" -default "default"}
  {-argName "cmdName" -required 1 -type tclobj}
}
cmd "method::assertion" NsfMethodAssertionCmd {
  {-argName "object" -required 1 -type object}
  {-argName "assertionsubcmd" -required 1 -type "check|object-invar|class-invar"}
  {-argName "arg" -required 0 -type tclobj}
}
cmd "method::create" NsfMethodCreateCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-inner-namespace" -nrargs 0}
  {-argName "-per-object" -nrargs 0}
  {-argName "-reg-object" -required 0 -type object}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
  {-argName "-precondition"  -type tclobj}
  {-argName "-postcondition" -type tclobj}
}
cmd "method::delete" NsfMethodDeleteCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -nrargs 0}
  {-argName "methodName" -required 1 -type tclobj}
}
cmd "method::forward" NsfMethodForwardCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -nrargs 0}
  {-argName "method" -required 1 -type tclobj}
  {-argName "-default" -type tclobj}
  {-argName "-earlybinding" -nrargs 0}
  {-argName "-methodprefix" -type tclobj}
  {-argName "-objframe" -nrargs 0}
  {-argName "-onerror" -type tclobj}
  {-argName "-verbose" -nrargs 0}
  {-argName "target" -type tclobj}
  {-argName "args" -type args}
}
cmd "method::property" NsfMethodPropertyCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -nrargs 0}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "methodproperty" -required 1 -type "class-only|call-protected|redefine-protected|returns|slotcontainer|slotobj"}
  {-argName "value" -type tclobj}
}
cmd "method::registered" NsfMethodRegisteredCmd {
  {-argName "handle" -required 1 -type tclobj}
}
cmd "method::setter" NsfMethodSetterCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -nrargs 0}
  {-argName "parameter" -required 1 -type tclobj}
}

#
# object cmds
#
cmd "object::dispatch" NsfObjectDispatchCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-frame" -required 0 -type "method|object|default" -default "default"}
  {-argName "command" -required 1 -type tclobj}
  {-argName "args"  -type args}
}
cmd "object::exists" NsfObjectExistsCmd {
  {-argName "value" -required 1 -type tclobj}
}
cmd "object::property" NsfObjectPropertyCmd {
  {-argName "objectName" -required 1 -type object}
  {-argName "objectproperty" -type "initialized|class|rootmetaclass|rootclass|slotcontainer" -required 1}
}
cmd "object::qualify" NsfObjectQualifyCmd {
  {-argName "objectName" -required 1 -type tclobj}
}

#
# objectsystem cmds
#
cmd "objectsystem::create" NsfObjectSystemCreateCmd {
  {-argName "rootClass" -required 1 -type tclobj}
  {-argName "rootMetaClass" -required 1 -type tclobj}
  {-argName "systemMethods" -required 0 -type tclobj}
}

cmd my NsfMyCmd {
  {-argName "-local" -nrargs 0}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "args" -type args}
}
cmd next NsfNextCmd {
  {-argName "arguments" -required 0 -type tclobj}
}
cmd nscopycmds NsfNSCopyCmdsCmd {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}
cmd nscopyvars NsfNSCopyVarsCmd {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}

cmd proc NsfProcCmd {
  {-argName "-ad" -required 0  -nrargs 0}
  {-argName "procName" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
}
cmd relation NsfRelationCmd {
  {-argName "object"  -required 1 -type object}
  {-argName "relationtype" -required 1 -type "object-mixin|class-mixin|object-filter|class-filter|class|superclass|rootclass"}
  {-argName "value" -required 0 -type tclobj}
}

cmd current NsfCurrentCmd {
  {-argName "currentoption" -required 0 -type "proc|method|methodpath|object|class|activelevel|args|activemixin|calledproc|calledmethod|calledclass|callingproc|callingmethod|callingclass|callinglevel|callingobject|filterreg|isnextcall|next"}
}
cmd self NsfSelfCmd {
}

#
# var cmds
#
cmd "var::exists" NsfVarExistsCmd {
  {-argName "-array" -required 0 -nrargs 0}
  {-argName "object" -required 1 -type object}
  {-argName "varName" -required 1}
}
cmd "var::import" NsfVarImportCmd {
  {-argName "object" -required 1 -type object}
  {-argName "args" -type args}
}
cmd "var::set" NsfVarSetCmd {
  {-argName "-array" -required 0 -nrargs 0}
  {-argName "object" -required 1 -type object}
  {-argName "varName" -required 1 -type tclobj}
  {-argName "value" -required 0 -type tclobj}
}
cmd "var::unset" NsfVarUnsetCmd {
  {-argName "-nocomplain" -required 0 -nrargs 0}
  {-argName "object" -required 1 -type object}
  {-argName "varName" -required 1 -type tclobj}
}

#
# object methods
#
objectMethod autoname NsfOAutonameMethod {
  {-argName "-instance" -nrargs 0}
  {-argName "-reset"  -nrargs 0}
  {-argName "name" -required 1 -type tclobj}
}

objectMethod class NsfOClassMethod {
  {-argName "class" -required 0 -type tclobj}
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
  {-argName "-childof" -type object}
  {-argName "args" -required 0 -type args}
}

classMethod recreate NsfCRecreateMethod {
  {-argName "objectName" -required 1 -type tclobj}
  {-argName "args" -type allargs}
}

classMethod superclass NsfCSuperclassMethod {
  {-argName "superclasses" -required 0 -type tclobj}
}

#
# info object methods
#
objectInfoMethod children NsfObjInfoChildrenMethod {
  {-argName "-type" -required 0 -type class}
  {-argName "pattern" -required 0}
}
objectInfoMethod class NsfObjInfoClassMethod {
}
objectInfoMethod filterguard NsfObjInfoFilterguardMethod {
  {-argName "fileName" -required 1}
}
objectInfoMethod filtermethods NsfObjInfoFiltermethodsMethod {
  {-argName "-guards" -nrargs 0}
  {-argName "-order" -nrargs 0}
  {-argName "pattern"}
}
objectInfoMethod forward NsfObjInfoForwardMethod {
  {-argName "-definition" -nrargs 0}
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
  {-argName "-callprotection" -type "all|protected|public" -default all}
  {-argName "-incontext" -nrargs 0}
  {-argName "-methodtype" -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-nomixins" -nrargs 0}
  {-argName "-path" -nrargs 0}
  {-argName "-source" -type "all|application|baseclasses"}
  {-argName "pattern" -required 0}
}
objectInfoMethod lookupslots NsfObjInfoLookupSlotsMethod {
  {-argName "-source" -type "all|application|baseclasses" -default all}
  {-argName "-type" -required 0 -type class}
  {-argName "pattern" -required 0}
}
objectInfoMethod method NsfObjInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|definition|exists|handle|parameter|parametersyntax|type|precondition|postcondition|submethods"}
  {-argName "name" -required 1 -type tclobj}
}
objectInfoMethod methods NsfObjInfoMethodsMethod {
  {-argName "-callprotection" -type "all|protected|public" -default public}
  {-argName "-methodtype" -type "all|scripted|builtin|alias|forwarder|object|setter|nsfproc"}
  {-argName "-path" -nrargs 0}
  {-argName "pattern" -required 0}
}

objectInfoMethod mixinclasses NsfObjInfoMixinclassesMethod {
  {-argName "-guards" -nrargs 0}
  {-argName "-heritage" -nrargs 0}
  {-argName "pattern" -type objpattern}
}
objectInfoMethod mixinguard NsfObjInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
objectInfoMethod parent NsfObjInfoParentMethod {
}
objectInfoMethod precedence NsfObjInfoPrecedenceMethod {
  {-argName "-intrinsic" -nrargs 0}
  {-argName "pattern" -required 0}
}
objectInfoMethod slots NsfObjInfoSlotsMethod {
  {-argName "-type" -required 0 -type class}
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
  {-argName "-guards" -nrargs 0}
  {-argName "pattern"}
}
classInfoMethod forward NsfClassInfoForwardMethod {
  {-argName "-definition" -nrargs 0}
  {-argName "name"}
}
classInfoMethod heritage NsfClassInfoHeritageMethod {
  {-argName "pattern"}
}
classInfoMethod instances NsfClassInfoInstancesMethod {
  {-argName "-closure" -nrargs 0}
  {-argName "pattern" -type objpattern}
}

classInfoMethod method NsfClassInfoMethodMethod {
  {-argName "infomethodsubcmd" -type "args|body|definition|exists|handle|parameter|parametersyntax|type|precondition|postcondition|submethods"}
  {-argName "name" -required 1 -type tclobj}
}
classInfoMethod methods NsfClassInfoMethodsMethod {
  {-argName "-callprotection" -type "all|protected|public" -default public}
  {-argName "-methodtype" -type "all|scripted|builtin|alias|forwarder|object|setter|nsfproc"}
  {-argName "-path" -nrargs 0}
  {-argName "pattern"}
}
classInfoMethod mixinclasses NsfClassInfoMixinclassesMethod {
  {-argName "-closure" -nrargs 0}
  {-argName "-guards" -nrargs 0}
  {-argName "-heritage" -nrargs 0}
  {-argName "pattern" -type objpattern}
}
classInfoMethod mixinguard NsfClassInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
classInfoMethod mixinof  NsfClassInfoMixinOfMethod {
  {-argName "-closure" -nrargs 0}
  {-argName "-scope" -required 0 -type "all|class|object"}
  {-argName "pattern" -type objpattern}
}
classInfoMethod objectparameter NsfClassInfoObjectparameterMethod {
  {-argName "infoobjectparametersubcmd" -type "list|name|parameter|parametersyntax" -required 1}
  {-argName "pattern" -required 0}
}
classInfoMethod slots NsfClassInfoSlotsMethod {
  {-argName "-closure" -nrargs 0}
  {-argName "-source" -type "all|application|baseclasses" -default all}
  {-argName "-type" -required 0 -type class}
  {-argName "pattern" -required 0}
 }
classInfoMethod subclass NsfClassInfoSubclassMethod {
  {-argName "-closure" -nrargs 0}
  {-argName "pattern" -type objpattern}
}
classInfoMethod superclass NsfClassInfoSuperclassMethod {
  {-argName "-closure" -nrargs 0}
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
