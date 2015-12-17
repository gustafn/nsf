# -*- Tcl -*-
#
# nsfAPI.decls --
#
# 	Public functions and Tcl commands offered by the Next
# 	Scripting Framework (NSF) library. This script is sourced by
# 	the C-code generator gentclAPI.tcl in the same directory.
#
# Copyright (C) 2009-2014 Gustaf Neumann
# 
# Vienna University of Economics and Business
# Institute of Information Systems and New Media
# A-1020, Welthandelsplatz 1
# Vienna, Austria
# 
# This work is licensed under the MIT License http://www.opensource.org/licenses/MIT
#
# Copyright:
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

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
cmd __db_show_obj NsfDebugShowObj {
  {-argName "obj"    -required 1 -type tclobj}
}
cmd __profile_clear NsfProfileClearDataStub {} 
cmd __profile_get NsfProfileGetDataStub {}
cmd __profile_get NsfProfileGetDataStub {}
cmd __profile_trace NsfProfileTraceStub {
  {-argName "-enable" -required 1 -nrargs 1 -type boolean}
  {-argName "-verbose" -required 0 -nrargs 1 -type boolean}
  {-argName "-dontsave" -required 0 -nrargs 1 -type boolean}
  {-argName "-builtins" -required 0 -nrargs 1 -type tclobj}
}

cmd __unset_unknown_args NsfUnsetUnknownArgsCmd {}

cmd "asm::proc" NsfAsmProcCmd {
  {-argName "-ad" -required 0  -nrargs 0 -type switch}
  {-argName "-checkalways" -required 0  -nrargs 0 -type switch}
  {-argName "procName" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
}

cmd configure NsfConfigureCmd {
  {-argName "option" -required 1 -typeName "configureoption" -type "debug|dtrace|filter|profile|trace|softrecreate|objectsystems|keepcmds|checkresults|checkarguments"}
  {-argName "value" -required 0 -type tclobj}
} {-nxdoc 1}
cmd colon NsfColonCmd {
  {-argName "args" -type allargs}
}
cmd "directdispatch" NsfDirectDispatchCmd {
  {-argName "object"     -required 1 -type object}
  {-argName "-frame"     -required 0 -type "method|object|default" -default "default"}
  {-argName "command"    -required 1 -type tclobj}
  {-argName "args"       -type args}
}
cmd "dispatch" NsfDispatchCmd {
  {-argName "object"     -required 1 -type object}
  {-argName "-intrinsic" -required 0 -nrargs 0 -type switch}
  {-argName "-system"    -required 0 -nrargs 0 -type switch}
  {-argName "command"    -required 1 -type tclobj}
  {-argName "args"       -type args}
} {-nxdoc 1}
cmd finalize NsfFinalizeCmd {
  {-argName "-keepvars" -required 0 -nrargs 0 -type switch}
} {-nxdoc 1}
cmd interp NsfInterpObjCmd {
  {-argName "name" -required 1}
  {-argName "args" -type allargs}
} {-nxdoc 1}

cmd is NsfIsCmd {
  {-argName "-complain"  -nrargs 0 -type switch}
  {-argName "-configure" -nrargs 0 -type switch}
  {-argName "-name" -required 0}
  {-argName "constraint" -required 1 -type tclobj}
  {-argName "value" -required 1 -type tclobj}
} {-nxdoc 1}


cmd parameter::info NsfParameterInfoCmd {
  {-argName "subcmd"   -typeName "parametersubcmd" -type "default|list|name|syntax|type" -required 1}
  {-argName "spec"     -required 1 -type tclobj}
  {-argName "varname"  -required 0 -type tclobj}
}

cmd parameter::cache::classinvalidate NsfParameterCacheClassInvalidateCmd {
  {-argName "class" -required 1 -type class}
}

cmd parameter::cache::objectinvalidate NsfParameterCacheObjectInvalidateCmd {
  {-argName "object" -required 1 -type object}
}

cmd parameter::specs NsfParameterSpecsCmd {
  {-argName "-configure"  -nrargs 0 -required 0 -type switch}
  {-argName "-nonposargs" -nrargs 0 -required 0 -type switch}
  {-argName "slotobjs"    -required 1 -type tclobj}
}

#
# cmd cmds (maybe more later)
#

cmd "cmd::info" NsfCmdInfoCmd {
  {-argName "subcmd" -required 1 -typeName "methodgetcmd" -type "args|body|definition|exists|registrationhandle|definitionhandle|origin|parameter|syntax|type|precondition|postcondition|submethods|returns"}
  {-argName "-context" -required 0 -type object}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "pattern" -required 0}
} {-nxdoc 1}

#
# method cmds
#
cmd "method::alias" NsfMethodAliasCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -required 0 -nrargs 0 -type switch}
  {-argName "methodName" -required 1}
  {-argName "-frame" -required 0 -type "method|object|default" -default "default"}
  {-argName "-protection" -required 0 -type "call-protected|redefine-protected|none" -default "none"}
  {-argName "cmdName" -required 1 -type tclobj}
} {-nxdoc 1}
cmd "method::assertion" NsfMethodAssertionCmd {
  {-argName "object" -required 1 -type object}
  {-argName "subcmd" -required 1 -typeName "assertionsubcmd" -type "check|object-invar|class-invar"}
  {-argName "arg" -required 0 -type tclobj}
} {-nxdoc 1}
cmd "method::asmcreate" NsfAsmMethodCreateCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-checkalways" -required 0 -nrargs 0 -type switch}
  {-argName "-inner-namespace" -nrargs 0 -type switch}
  {-argName "-per-object" -required 0 -nrargs 0 -type switch}
  {-argName "-reg-object" -required 0 -nrargs 1 -type object}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
}
cmd "method::create" NsfMethodCreateCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-checkalways" -required 0 -nrargs 0 -type switch}
  {-argName "-inner-namespace" -nrargs 0 -type switch}
  {-argName "-per-object" -required 0 -nrargs 0 -type switch}
  {-argName "-reg-object" -required 0 -type object}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "arguments" -required 1 -type tclobj}
  {-argName "body" -required 1 -type tclobj}
  {-argName "-precondition"  -type tclobj}
  {-argName "-postcondition" -type tclobj}
} {-nxdoc 1}
cmd "method::delete" NsfMethodDeleteCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -required 0 -nrargs 0 -type switch}
  {-argName "methodName" -required 1 -type tclobj}
} {-nxdoc 1}


cmd "method::forward" NsfMethodForwardCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -required 0 -nrargs 0 -type switch}
  {-argName "method" -required 1 -type tclobj}
  {-argName "-default" -type tclobj}
  {-argName "-earlybinding" -nrargs 0 -type switch}
  {-argName "-onerror" -type tclobj}
  {-argName "-prefix" -type tclobj}
  {-argName "-frame" -nrargs 1 -type "object|method|default" -default default}
  {-argName "-verbose" -nrargs 0 -type switch}
  {-argName "target" -type tclobj}
  {-argName "args" -type args}
} {-nxdoc 1}

cmd "method::forward::property" NsfForwardPropertyCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -required 0 -nrargs 0 -type switch}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "forwardProperty" -required 1 -type "prefix|target|verbose"}
  {-argName "value" -type tclobj}
}


cmd "method::property" NsfMethodPropertyCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -required 0 -nrargs 0 -type switch}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "methodProperty" -required 1 -type "class-only|call-private|call-protected|debug|deprecated|redefine-protected|returns"}
  {-argName "value" -type tclobj}
} {-nxdoc 1}

cmd "method::registered" NsfMethodRegisteredCmd {
  {-argName "handle" -required 1 -type tclobj}
} {-nxdoc 1}

cmd "method::setter" NsfMethodSetterCmd {
  {-argName "object" -required 1 -type object}
  {-argName "-per-object" -required 0 -nrargs 0 -type switch}
  {-argName "parameter" -required 1 -type tclobj}
} {-nxdoc 1}

#
# object cmds
#
cmd "object::alloc" NsfObjectAllocCmd {
  {-argName "class" -required 1 -type class}
  {-argName "name" -required 1 -type tclobj}
  {-argName "initcmd" -required 0 -type tclobj}
}
cmd "object::exists" NsfObjectExistsCmd {
  {-argName "value" -required 1 -type tclobj}
} {-nxdoc 1}
cmd "object::property" NsfObjectPropertyCmd {
  {-argName "objectName" -required 1 -type object}
  {-argName "objectProperty" -type "initialized|class|rootmetaclass|rootclass|volatile|slotcontainer|hasperobjectslots|keepcallerself|perobjectdispatch" -required 1}
  {-argName "value" -required 0 -type tclobj}
} {-nxdoc 1}
cmd "object::qualify" NsfObjectQualifyCmd {
  {-argName "objectName" -required 1 -type tclobj}
} {-nxdoc 1}

#
# objectsystem cmds
#
cmd "objectsystem::create" NsfObjectSystemCreateCmd {
  {-argName "rootClass" -required 1 -type tclobj}
  {-argName "rootMetaClass" -required 1 -type tclobj}
  {-argName "systemMethods" -required 0 -type tclobj}
} {-nxdoc 1}

cmd my NsfMyCmd {
  {-argName "-intrinsic" -nrargs 0 -type switch}
  {-argName "-local"     -nrargs 0 -type switch}
  {-argName "-system"    -nrargs 0 -type switch}
  {-argName "methodName" -required 1 -type tclobj}
  {-argName "args"       -type args}
} {-nxdoc 1}
cmd next NsfNextCmd {
  {-argName "arguments" -required 0 -type tclobj}
} {-nxdoc 1}
cmd nscopyvars NsfNSCopyVarsCmd {
  {-argName "fromNs" -required 1 -type tclobj}
  {-argName "toNs" -required 1 -type tclobj}
}

cmd proc NsfProcCmd {
  {-argName "-ad"          -required 0 -nrargs 0 -type switch}
  {-argName "-checkalways" -required 0 -nrargs 0 -type switch}
  {-argName "-debug"       -required 0 -nrargs 0 -type switch}
  {-argName "-deprecated"  -required 0 -nrargs 0 -type switch}
  {-argName "procName"     -required 1 -type tclobj}
  {-argName "arguments"    -required 1 -type tclobj}
  {-argName "body"         -required 1 -type tclobj}
} {-nxdoc 1}

cmd relation::get NsfRelationGetCmd {
  {-argName "object"  -required 1 -type object}
  {-argName "type" -required 1 -typeName "relationtype" -type "object-mixin|class-mixin|object-filter|class-filter|class|superclass|rootclass"}
} {-nxdoc 1}

cmd relation::set NsfRelationSetCmd {
  {-argName "object"  -required 1 -type object}
  {-argName "type" -required 1 -typeName "relationtype" -type "object-mixin|class-mixin|object-filter|class-filter|class|superclass|rootclass"}
  {-argName "value" -required 0 -type tclobj}
} {-nxdoc 1}


cmd current NsfCurrentCmd {
  {-argName "option" -required 0 -typeName "currentoption" -type "proc|method|methodpath|object|class|activelevel|args|activemixin|calledproc|calledmethod|calledclass|callingproc|callingmethod|callingclass|callinglevel|callingobject|filterreg|isnextcall|nextmethod"}
} {-nxdoc 1}
cmd self NsfSelfCmd {
} {-nxdoc 1}

#
# var cmds
#
cmd "var::exists" NsfVarExistsCmd {
  {-argName "-array" -required 0 -nrargs 0 -type switch}
  {-argName "object" -required 1 -type object}
  {-argName "varName" -required 1}
} {-nxdoc 1}
cmd "var::get" NsfVarGetCmd {
  {-argName "-array" -required 0 -nrargs 0 -type switch}
  {-argName "object" -required 1 -type object}
  {-argName "varName" -required 1 -type tclobj}
} {-nxdoc 1}

cmd "var::import" NsfVarImportCmd {
  {-argName "object" -required 1 -type object}
  {-argName "args" -type args}
} {-nxdoc 1}
cmd "var::set" NsfVarSetCmd {
  {-argName "-array" -required 0 -nrargs 0 -type switch}
  {-argName "object" -required 1 -type object}
  {-argName "varName" -required 1 -type tclobj}
  {-argName "value" -required 0 -type tclobj}
} {-nxdoc 1}
cmd "var::unset" NsfVarUnsetCmd {
  {-argName "-nocomplain" -required 0 -nrargs 0 -type switch}
  {-argName "object" -required 1 -type object}
  {-argName "varName" -required 1 -type tclobj}
} {-nxdoc 1}

#
# object methods
#
objectMethod autoname NsfOAutonameMethod {
  {-argName "-instance" -nrargs 0 -type switch}
  {-argName "-reset"  -nrargs 0 -type switch}
  {-argName "name" -required 1 -type tclobj}
}

objectMethod class NsfOClassMethod {
  {-argName "class" -required 0 -type tclobj}
}

objectMethod cleanup NsfOCleanupMethod {
}

objectMethod cget NsfOCgetMethod {
  {-argName "name" -type tclobj -required 1}
}

objectMethod configure NsfOConfigureMethod {
  {-argName "args" -type virtualobjectargs}
} {-objv0 1}

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
  {-argName "objectName" -required 1 -type tclobj}
  {-argName "args" -type virtualclassargs}
}

classMethod dealloc NsfCDeallocMethod {
  {-argName "object" -required 1 -type tclobj}
}

classMethod filterguard NsfCFilterGuardMethod {
  {-argName "filter" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}

classMethod getCachedParameters NsfCGetCachendParametersMethod {
}

classMethod mixinguard NsfCMixinGuardMethod {
  {-argName "mixin" -required 1 -type tclobj}
  {-argName "guard" -required 1 -type tclobj}
}

classMethod new NsfCNewMethod {
  {-argName "-childof" -required 0 -type tclobj}
  {-argName "args" -required 0 -type virtualclassargs}
}

classMethod recreate NsfCRecreateMethod {
  {-argName "objectName" -required 1 -type tclobj}
  {-argName "args" -type virtualclassargs}
}

classMethod superclass NsfCSuperclassMethod {
  {-argName "superclasses" -required 0 -type tclobj}
}

#
# info object methods
#
objectInfoMethod baseclass NsfObjInfoBaseclassMethod {
}
objectInfoMethod children NsfObjInfoChildrenMethod {
  {-argName "-type" -required 0 -type class}
  {-argName "pattern" -required 0}
}
objectInfoMethod class NsfObjInfoClassMethod {
}
objectInfoMethod filterguard NsfObjInfoFilterguardMethod {
  {-argName "filter" -required 1}
}
objectInfoMethod filters NsfObjInfoFiltersMethod {
  {-argName "-guards" -nrargs 0 -type switch}
  {-argName "pattern"}
}
objectInfoMethod forward NsfObjInfoForwardMethod {
  {-argName "-definition" -nrargs 0 -type switch}
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
objectInfoMethod lookupfilter NsfObjInfoLookupFilterMethod {
  {-argName "filter" -required 1}
}
objectInfoMethod lookupfilters NsfObjInfoLookupFiltersMethod {
  {-argName "-guards" -nrargs 0 -type switch}
  {-argName "pattern"}
}
objectInfoMethod lookupmethod NsfObjInfoLookupMethodMethod {
  {-argName "name" -required 1 -type tclobj}
}
objectInfoMethod lookupmethods NsfObjInfoLookupMethodsMethod {
  {-argName "-callprotection" -type "all|public|protected|private" -default all}
  {-argName "-incontext" -nrargs 0 -type switch}
  {-argName "-type" -typeName "methodtype" -type "all|scripted|builtin|alias|forwarder|object|setter|nsfproc"}
  {-argName "-nomixins" -nrargs 0 -type switch}
  {-argName "-path" -nrargs 0 -type switch}
  {-argName "-source" -type "all|application|system" -default all}
  {-argName "pattern" -required 0}
}
objectInfoMethod lookupmixins NsfObjInfoLookupMixinsMethod {
  {-argName "-guards" -nrargs 0 -type switch}
  {-argName "pattern" -type objpattern}
}
objectInfoMethod lookupslots NsfObjInfoLookupSlotsMethod {
  {-argName "-source" -type "all|application|system" -default all}
  {-argName "-type" -required 0 -type class}
  {-argName "pattern" -required 0}
}
objectInfoMethod method NsfObjInfoMethodMethod {
  {-argName "subcmd" -required 1 -typeName "infomethodsubcmd" -type "args|body|definition|exists|registrationhandle|definitionhandle|origin|parameter|syntax|type|precondition|postcondition|submethods|returns"}
  {-argName "name" -required 1 -type tclobj}
}
objectInfoMethod methods NsfObjInfoMethodsMethod {
  {-argName "-callprotection" -type "all|public|protected|private" -default all}
  {-argName "-type"  -typeName "methodtype" -type "all|scripted|builtin|alias|forwarder|object|setter|nsfproc"}
  {-argName "-path" -nrargs 0 -type switch}
  {-argName "pattern" -required 0}
}

objectInfoMethod mixins NsfObjInfoMixinsMethod {
  {-argName "-guards" -nrargs 0 -type switch}
  {-argName "pattern" -type objpattern}
}
objectInfoMethod mixinguard NsfObjInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
objectInfoMethod name NsfObjInfoNameMethod {
}
objectInfoMethod parent NsfObjInfoParentMethod {
}
objectInfoMethod objectparameter NsfObjInfoObjectparameterMethod {
  {-argName "subcmd" -typeName "infoobjectparametersubcmd" -type "definitions|list|names|syntax" -required 1}
  {-argName "pattern" -required 0}
}
objectInfoMethod precedence NsfObjInfoPrecedenceMethod {
  {-argName "-intrinsic" -nrargs 0 -type switch}
  {-argName "pattern" -required 0}
}
objectInfoMethod slotobjects NsfObjInfoSlotobjectsMethod {
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
classInfoMethod filters NsfClassInfoFiltersMethod {
  {-argName "-guards" -nrargs 0 -type switch}
  {-argName "pattern"}
}
classInfoMethod forward NsfClassInfoForwardMethod {
  {-argName "-definition" -nrargs 0 -type switch}
  {-argName "name"}
}
classInfoMethod heritage NsfClassInfoHeritageMethod {
  {-argName "pattern"}
}
classInfoMethod instances NsfClassInfoInstancesMethod {
  {-argName "-closure" -nrargs 0 -type switch}
  {-argName "pattern" -type objpattern}
}

classInfoMethod method NsfClassInfoMethodMethod {
  {-argName "subcmd" -required 1 -typeName "infomethodsubcmd" -type "args|body|definition|exists|registrationhandle|definitionhandle|origin|parameter|syntax|type|precondition|postcondition|submethods|returns"}
  {-argName "name" -required 1 -type tclobj}
}
classInfoMethod methods NsfClassInfoMethodsMethod {
  {-argName "-callprotection" -type "all|public|protected|private" -default all}
  {-argName "-closure" -nrargs 0 -type switch}
  {-argName "-type" -nrargs 1 -typeName "methodtype" -type "all|scripted|builtin|alias|forwarder|object|setter|nsfproc"}
  {-argName "-path" -nrargs 0 -type switch}
  {-argName "-source" -nrargs 1 -type "all|application|system" -default all}
  {-argName "pattern"}
}
classInfoMethod mixins NsfClassInfoMixinsMethod {
  {-argName "-closure" -nrargs 0 -type switch}
  {-argName "-guards" -nrargs 0 -type switch}
  {-argName "-heritage" -nrargs 0 -type switch}
  {-argName "pattern" -type objpattern}
}
classInfoMethod mixinguard NsfClassInfoMixinguardMethod {
  {-argName "mixin"  -required 1}
}
classInfoMethod mixinof  NsfClassInfoMixinOfMethod {
  {-argName "-closure" -nrargs 0 -type switch}
  {-argName "-scope" -required 0 -type "all|class|object"}
  {-argName "pattern" -type objpattern}
}
classInfoMethod slotobjects NsfClassInfoSlotobjectsMethod {
  {-argName "-closure" -nrargs 0 -type switch}
  {-argName "-source" -type "all|application|system" -default all}
  {-argName "-type" -required 0 -type class}
  {-argName "pattern" -required 0}
 }
classInfoMethod subclass NsfClassInfoSubclassMethod {
  {-argName "-closure" -nrargs 0 -type switch}
  {-argName "-dependent" -nrargs 0 -type switch}
  {-argName "pattern" -type objpattern -flags NSF_ARG_NODASHALNUM}
}
classInfoMethod superclass NsfClassInfoSuperclassMethod {
  {-argName "-closure" -nrargs 0 -type switch}
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

#
# Local variables:
#    mode: tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:
