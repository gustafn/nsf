# -*- Tcl -*-
#
# This script is sourced by the C-code generator gentclAPI.tcl in the
# same directory. It is also used by the nx::doc toolkit to generate
# the authorative language reference documents.

# @package next
#
# "Next" is a compact and expressive object-oriented language
# extension for Tcl. The object system model is highly influenced by
# CLOS. This package provides the basic object system for the Next
# language. It defines the basic language entities {{@object ::nx::Object}} and
# {{@object ::nx::Class}}, as well as essential language primitives
# (e.g., {{@command ::nx::next}} and {{@command ::nx::self}}).
#
# @require Tcl
# @version 1.0.0a

# namespaces for types of methods
array set ns {
  xotclCmd    "::nsf"
  objectMethod "::nsf::cmd::Object"
  classMethod  "::nsf::cmd::Class"
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

# @command ::nsf::configure
# 
# A top-level configuration facility which allows you modify
# properties of the "Next" object system for the scope of an entire
# {{{interp}}}.

# @subcommand ::nsf::configure#filter
#
# Allows turning on or off filters globally for the current
# interpreter. By default, the filter state is turned off. This
# function returns the old filter state. This filterless {{{interp}}}
# state is needed for the serializer which should introspect and stream the
# objects and classes without being affected by active filter.
#
# @param toggle Accepts either "on" or "off"
# @return The current filter activation state

# @subcommand ::nsf::configure#softrecreate
# 
# Allows controlling the scheme applied when recreating an object or a
# class. By default, it is set to {{{off}}}. This means that the
# object/class is destroyed and all relations
# (e.g. subclass/superclass) to other objects/classes are revoked as
# well. If softrecreate is set to {{{on}}}, the object is re-set, but not
# destroyed, the relations are
# kept. 
# 
# A "soft" recreation is important for e.g. reloading a file with
# class definitions (e.g. when used in OpenACS with file watching and
# reloading). With softrecreate set, it is not necessary to recreate
# dependent subclasses etc. Consider the example of a class hierarchy
# {{{A <- B <- C}}}. Without {{{softrecreate}}} set, a reload of
# {{{B}}} means first a destroy of B, leading to {{{A <- C}}}, and
# instances of {{{B}}} are re-classed to {{@object
# ::nx::Object}}. When softrecreate is set, the class hierarchy
# remains untouched.
#
# @param toggle Accepts either "on" or "off"
# @return The current toggle value


# @subcommand ::nsf::configure#objectsystems
# 
# A mere introspection subcommand. It gives you the top level of the
# current object system, i.e., the ruling root class and root
# meta-class. For "Next":
#
# {{{
#	::nsf::configure objectsystems; # returns "::nx::Object ::nx::Class"
# }}}
#
# @return The active pair of root class and root meta-class

# @subcommand ::nsf::configure#keepinitcmd
#
# Usually, initcmd scripts are discarded by the {{{interp}}} once
# having been evaluated (in contrast to {{{proc}}} and {{{method}}}
# bodies). If you need them preserved for later introspection and
# processing (as in the "Next" documentation system), set this option
# to {{{true}}}. Then, the initcmd scripts are retained as a
# particular object variable ({{{__initcmd}}}) of classes and
# objects. It defaults to {{{false}}}.
#
# @param value:boolean Either {{{true}}} or {{{false}}}
# @return The current setting
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

# @command ::nx::next
#
# Invokes the shadowed (i.e, same-named) method which is next along
# the precedence path and returns the results of this invocation. If
# {{{next}}} is called without arguments, the arguments of the current
# method (i.e., the arguments as present at the current callframe) are
# passed through to the shadowed method. If next is invoked with the
# flag --noArgs, the shadowed method is called without the active
# callframe arguments. If other arguments are specified for {{{next}}}
# explicitly, these will be passed instead.
# 
# @param --noArgs:optional Deactivates the forward-passing of the current callframe's arguments
# @param args Explicitly declared arguments to pass to shadowed methods

# TODO: shouldn't next be defined here?

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
  {-argName "value" -required 0 -type tclobj}
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

# @command ::nx::current
#
# An introspective command which allows you to explore the "Next"
# callstack from within the scope of a method (or procif bound to an
# object via {{{alias}}}). {{{current}}} computes callstack related
# information. If executed without specifying a subcommand,
# i.e. {{{[current]}}}, returns the name of the object, which is
# currently in execution. If called from outside a proc, it returns
# the error message "No current object". 
# 
# It comes with a variety of subcommands to query different bits of
# callstack information. See below.
#
# @subcommand class Returns the name of the class holding the currently executing per-class method, if and only if called from within a per-class method. Note, that this method-owning class may be different to the class of the current object. If called from within a per-object method, it returns an empty string.
# @subcommand proc Returns the name of the currently executing method.
# @subcommand callingclass Returns the name of the class which is calling into the executing method.
# @subcommand callingobject Returns the name of the object which is calling into the executing method.
# @subcommand calledclass Returns the name of the class that holds the originally (and now shadowed) target method (applicable in mixin classes and filters).
# @subcommand calledproc Returns the name of the target method (applicable in a filter only).
# @subcommand isnextcall Returns 1 if the executing method was invoked via {{@command ::nx::next}}, 0 otherwise.
# @subcommand next Returns the name of the method next on the precedence path as a string.
# @subcommand filterreg In a method serving as active filter, returns the name of the object (class) on which the method is registered as a filter.
# @subcommand callinglevel Returns the "original" callstack level calling into the executing method. Intermediary {{{next}}} calls are ignored in this computation. The level is returned in a form so that it can be used as first argument in {{@method ::nx::Object class uplevel}} or {{@method ::nx::Object class upvar}}.
# @subcommand activelevel Returns the actual callstack level calling into the executing method. The active might correspond the {{{callinglevel}}}, but this is not necessarily the case. The {{{activelevel}}} counts {{@command ::nx::next}} call. The level is returned in a form so that it can be used as first argument in {{@method ::nx::Object class uplevel}} or {{@method ::nx::Object class upvar}}.

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

#
# object methods
#

# @object ::nx::Object
#
# "Next" programs are constructed out of objects. This class
# describes common structural and behavioural features for all "Next"
# objects. It is the root object-class in the "Next" object system.

# @method ::nx::Object#autoname
#
# Provides a facility for auto-generating object identifiers. It is
# constructed from a seeding string which is appended a numeric
# index. This numeric index is incremented upon each call to
# {{{autoname}}}.
# {{{
#       set obj [Object new]
#       $obj autoname a; # yields "a1"
#       $obj autoname -instance B; # yields "b1"
#       $obj autoname a; # yields "a2"
#       $obj autoname b; # yields "b1"
#       $obj autoname -reset a; # ""
#       $obj autoname -reset -instance B; # ""
#       $obj autoname -instance a; # yields "a1", and NOT "a3"!
#       $obj autoname -instance B; # yields "b1"
#       $obj autoname b; # yields "b2"
# }}}
# The seeding string may also contain {{{[format]}}} expressions (see ...):
# {{{
#       $obj autoname a%06d; # gives you "a000001", ...
# }}}
#
# @param -instance Have the generated name start with a lower letter (though the seed string has a major first letter)
# @param -reset Reset the object-internal counter for a given seed string
# @param name The seeding string which is used as a base for name generation
# @return The generated name string
objectMethod autoname XOTclOAutonameMethod {
  {-argName "-instance"}
  {-argName "-reset"}
  {-argName "name" -required 1 -type tclobj}
}

# @method ::nx::Object#cleanup
#
# TODO: this is a method not used in the Next Scripting Langauge. This
# mehtod is just called via recreate, so everything necessary can be
# performed there as well. However, it is available for backward
# compatibility available in XOTcl 2.0
#
# Resets an object or class to its initial state, as after object
# allocation (see {{@method ::nx::Class class alloc}}). This method
# participates in recreating objects, i.e, it is called during the
# recreation process by {{@method ::nx::Class class recreate}}.
# Depending on the recreation scheme applied (see {{@command
# ::nsf::configure}}, object variables are deleted, per-object
# namespaces are cleared, and the object's relationsships (e.g., mixin
# relations) are reset.
# 
# @properties interally-called
objectMethod cleanup XOTclOCleanupMethod {
}

# @method ::nx::Object#configure
#
# This method participates in the object creation process. It is
# automatically invoked after having produced a new object by
# {{@method ::nx::Class class create}}.
# Upon its invocation, the variable argument vector {{{args}}}
# contains a list of parameters and parameter values passed in
# from the call site of object creation. They are matched against
# an object parameter definition. This definition, and so the
# actual method parameter definition of this method, is assembled
# from configuration values of the classes along the precedence
# order (see also {{@method ::nx::Object class objectparameter}}).
# The method {{{configure}}} can be called at arbitrary times to
# "re-set" an object.
#
# @properties interally-called
# @param args The variable argument vector stores the object parameters and their values
objectMethod configure XOTclOConfigureMethod {
  {-argName "args" -type allargs}
}

# @method ::nx::Object#destroy
#
# The standard destructor for an object. The method {{@method ::nx::Object class destroy}}
# triggers the physical destruction of the object. The method {{{destroy}}} can be refined
# by subclasses or mixin classes to add additional (class specific) destruction behavior.
# Note that in most cases, the class specific {{{destroy}}} methods should call 
# {{@command ::nx::next}} to trigger physical destruction.
# {{{
#     nx::Class create Foo {
#        :method destroy {} {
#            puts "destroying [self]"
#            next
#        }
#     }
#     Foo create f1
#     f1 destroy
# }}}
# Technical details: The method {{@method ::nx::Object class destroy}}
# delegates the actual destruction to {{@method ::nx::Class class dealloc}} 
# which clears the memory object storage. Essentially, the behaviour could be 
# scripted as:
# {{{
#       Object method destroy {} {
#               [:info class] dealloc [self]
#       }
# }}}
# Note, however, that {{{destroy}}} is protected against
# application-level redefinition. You must refine it in a subclass
# or mixin class. 
#
objectMethod destroy XOTclODestroyMethod {
}

# @method ::nx::Object#exists 
#
# A helper method for checking whether the variable {{{var}}} is
# defined on the object and assigned a value. You may use a variable
# name with or without prefix, both will resolve to the object scope:
# {{{
#       $obj eval {
#          set :foo 1
#          set bar 2
#       }
#
#       $obj exists foo; # returns 1
#       $obj exists :foo; # returns 1
#       $obj exists bar; # returns 0
#       $obj exists :bar; # returns 0
# }}}
#
# @param var The name of the variable to verify
# @return :boolean 1 if the variable exists, 0 otherwise
objectMethod exists XOTclOExistsMethod {
  {-argName "var" -required 1}
}

# @method ::nx::Object#filterguard
#
# Adds conditions to guard invocations of a filter. The
# filter will only execute, if the guards evaluate to true. Otherwise,
# the guarded filter is ignored. If no guards are given,
# always execute the filter.
#
# @param filter Handle to identify and address a filter once registered
# @param guard A list of guard expressions
objectMethod filterguard XOTclOFilterGuardMethod {
  {-argName "filter" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}

# @method ::nx::Object#filtersearch
#
# 
#
# @param filter Handle to identify and address a filter once registered
# @param guard A list of guard expressions
# @return A string which describes a fully qualified method handle
#objectMethod filtersearch XOTclOFilterSearchMethod {
#  {-argName "filter" -required 1}
#}

# @method ::nx::Object#instvar
#
# @param args
objectMethod instvar XOTclOInstVarMethod {
  {-argName "args" -type allargs}
}

# @method ::nx::Object#mixinguard
#
# @param mixin
# @param guard
objectMethod mixinguard XOTclOMixinGuardMethod {
  {-argName "mixin" -required 1}
  {-argName "guard" -required 1 -type tclobj}
}

# objectMethod __next XOTclONextMethod {
#  {-argName "args" -type allargs}
#}

# @method ::nx::Object#mixinguard
#
# @param mixin
# @param guard
objectMethod noinit XOTclONoinitMethod {
}

# @method ::nx::Object#requireNamespace
#
# This method allows you to request the creation of a namespace for
# the given object, a per-object namespace. The namespace is then used
# to store instance variables, methods and nested objects. Per-object
# namespaces are needed for using and binding object variables to
# non-object scopes in Tcl and Tk. For instance, you may use an
# per-object namespace to have object variables accessible Tk widgets
# and Tk callbacks. To verify whether a per-object namespace is
# available for an object, see ...
#
# Beware that there is a difference between per-object namespaces and
# Tcl namespaces which shadow an existing object (i.e., carry the same
# name):
# {{{
# 	Object create Foo
# 	Foo requireNamespace
# 	namespace exists Foo; # returns 1
# 	Foo info hasnamespace; # returns 1
#
# 	Object create Bar
# 	namespace eval ::Bar {}
# 	namespace exists Bar; # returns 1
# 	Bar info hasnamespace; # returns 0
# }}}
objectMethod requireNamespace XOTclORequireNamespaceMethod {
}

# @method ::nx::Object#residualargs
#
# @properties interally-called
# @param args
objectMethod residualargs XOTclOResidualargsMethod {
  {-argName "args" -type allargs}
}

# @method ::nx::Object#uplevel
#
# This helper allows you to evaluate a script in the context of
# another callstack level (i.e., callstack frame).
#
# @param level:optional The starting callstack level (defaults to the value of {{{[current callinglevel]}}})
# @param script:list The script to be evaluated in the targeted callstack level
objectMethod uplevel XOTclOUplevelMethod {
  {-argName "args" -type allargs}
}

# @method ::nx::Object#upvar
#
# This helper allows you to bind a local variable to a variable
# residing at a different callstack level (frame).
#
# @param level:optional The starting callstack level (defaults to the value of {{{[current callinglevel]}}})
# @param sourceVar A variable which should be linked to a ...
# @param targetVar ... which is a local variable in a method scope
# @see ...
objectMethod upvar XOTclOUpvarMethod {
  {-argName "args" -type allargs}
}

# @method ::nx::Object#volatile
#
# By calling on this method, the object is bound in its lifetime to
# the one of call site (e.g., the given Tcl proc or method scope):
# {{{
# 	proc foo {} {
#   		info vars; # shows ""
#   		set x [Object create Bar -volatile]
#   		info vars; # shows "x Bar"
# 	}
# }}}
# Behind the scenes, {{{volatile}}} registers a C-level variable trace
# ({{{Tcl_TraceVar()}}}) on the hiddenly created local variable (e.g.,
# {{{Bar}}}), firing upon unset events and deleting the referenced
# object ({{{Bar}}}). That is, once the callframe context of {{{foo}}}
# is left, the local variable {{{Bar}}} is unset and so the bound
# object is destroyed.
objectMethod volatile XOTclOVolatileMethod {
}

# @method ::nx::Object#vwait
#
# A method variant of the Tcl {{{vwait}}} command. You can use it to
# have the {{{interp}}} enter an event loop until the specified
# variable {{{varname}}} is set on the object.
#
# @param varname The name of the signalling object variable.
objectMethod vwait XOTclOVwaitMethod {
  {-argName "varname" -required 1}
}

# temporary 
#  TODO: remove me
objectMethod vars XOTclOVarsMethod {
  {-argName "pattern" -required 0}
}



#
# class methods
#

# @object ::nx::Class
#
# A class defines a family of object types which own a common set of
# attributes (see {{@object ::nx::Attribute}}) and methods. Classes
# are organised according to their similarities and differences in
# classification hierarchies. This object represents the root
# meta-class in the "Next" object system.
#
# @superclass ::nx::doc::entities::object::nx::Object

# @method ::nx::Class#alloc
#
# Creates a bare object or class which is not
# fully initialized. {{{alloc}}} is used by {{@method ::nx::Class class create}} to
# request a memory object storage. In subsequent steps,
# {{{create}}} invokes {{{configure}}} and {{{init}}} to further
# set up the object. Only in rare situations, you may consider
# bypassing the overall {{{create}}} mechanism by just allocating
# uninitialized objects using {{{alloc}}}.
#
# @properties interally-called
# @param name The object identifier assigned to the object storage to be allocated.
# @return The name of the allocated, uninitialized object
classMethod alloc XOTclCAllocMethod {
  {-argName "name" -required 1 -type tclobj}
}

# @method ::nx::Class#create
#
# Provides for creating application-level classes and objects. If
# the method receiver is a meta-class, a class will be
# created. Otherwise, {{{create}}} yields an object. {{{create}}}
# is responsible a multi-phase object creation scheme. This
# creation scheme involves three major steps:
# {{{
# [Object create anObject]               (1)
#                       ---------------.   .--------------.
#       -------------->|Class->create()|-->|Class->alloc()|
#                      `---------------'   `--------------'
#                                |  |  (2) .-------------------.
#                                |  .----->|Object->configure()|
#                                |         `-------------------'
#                                |   (3)   .------.
#                                .........>|init()|
#                                          `------'
# }}}
# (1) A call to {{@method ::nx::Class class alloc}} to create a raw,
# uninitalized object.
#
# (2) The newly allocated object receives a method call upon
# {{@method ::nx::Object class configure}}. This will establish the
# object's initial state, by applying object parameter values
# provided at object creation time and default values defined at
# object definition time.
#
# (3) Finally, {{{create}}} emits a call to the initialization
# method {{{init}}} (i.e., the actual "constructor"), if
# available. An {{{init}}} method can be defined by a class on
# behalf of its objects, to lay out class-specific initialisation
# behaviour. Alternatively, each single object may define an
# {{{init}}} method on its own.
#
# By overloading the method in a meta-class, you can refine or
# replace this default object creation scheme (e.g., for applying
# application-specific naming schemes).
#
# For creating an object or a class, you must name {{{create}}}
# explicitly, i.e.:
# {{{
#		::nx::Object create anObject
#		::nx::Class create AClass
#		::nx::Class AnotherClass; # This fails: "Method 'AnotherClass' unknown for ::nx::Class."
# }}}
# 
# @param name The designated identifier on the class or the object to be created.
# @param args Arguments to be passed down to the object creation procedure used to initialize the object.
# @return The name of the created, fully initialized object.
classMethod create XOTclCCreateMethod {
  {-argName "name" -required 1}
  {-argName "args" -type allargs}
}

# @method ::nx::Class#dealloc
#
# Marks objects for physical deletion in memory. Beware the fact
# that calling {{{dealloc}}} does not necessarily cause the object
# to be deleted immediately. Depending on the lifecycle of the the
# object's environment (e.g., the {{{interp}}}, the containing
# namespace) and on call references down the callstack, the actual
# memory freeing operation may occur time-shifted (that is,
# later). While {{{dealloc}}} itself cannot be redefined for
# {{{::nx::Class}}}, you may consider refining it in a subclass or
# mixin class for customizing the destruction process.
#
# @properties interally-called
# @param object The name of the object to be scheduled for deletion.
classMethod dealloc XOTclCDeallocMethod {
  {-argName "object" -required 1 -type tclobj}
}

# @method ::nx::Class#new
#
# A convenience method to create auto-named objects and classes. It is
# a front-end to {{@method ::nx::Class class create}}. For instance:
# {{{
#	set obj [Object new]
#	set cls [Class new]
# }}}
#
# This will provide object identifiers of the form
# e.g. {{{::nsf::__#0}}}. In contrast to {{@method ::nx::Object class autoname}},
# the uniqueness of auto-generated identifiers is guaranteed for the
# scope of the {{{interp}}}.
#
# @param -childof If provided, the new object is created as a child of the specified object.
# @param args The variable arguments passed down to {{@method ::nx::Class class create}}.
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

# @method ::nx::Class#recreate
#
# This method is called upon recreating an object. Recreation is the
# scheme for resolving object naming conflicts in the dynamic and
# scripted programming environment of "Next": An object or class is
# created while an object or class with an identical object identifier
# already exists. The method {{{recreate}}} performs standard object
# initialization, per default, after re-setting the state and
# relationships of the object under recreation. This re-set is
# achieved by invoking {{@method ::nx::Object class cleanup}}.
# {{{
#	Object create Bar
#	\# ...
#	Object create Bar; # calls Object->recreate(::Bar, ...) + ::Bar->cleanup()
# }}}
# By refining {{{recreate}}} in an application-level subclass or mixin
# class, you can intercept the recreation process. In the pre-part the
# refined {{{recreate}}} method, the recreated object has its old
# state, after calling {{@command ::nx::next}} it is cleaned up.
#
# If the name conflict occurs between an existing class and a newly
# created object (or vice versa), {{{recreate}}} is not
# performed. Rather, a sequence of {{@method ::nx::Object class destroy}}
# and {{@method ::nx::Class class create}} is triggered:
# {{{
#	Object create Bar
#	\# ...
#	Class create Bar; # calls Bar->destroy() + Class->create(::Bar, ...)
# }}}
#
# @properties interally-called
# @param name The name (identifier) of the object under recreation
# @param args Arbitrary vector of arguments
# @return The name of the recreated object
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

#
# info object methods
#
infoObjectMethod callable XOTclObjInfoCallableMethod {
  {-argName "object" -type object}
  {-argName "infocallablesubcmd" -nrargs 1 -type "filter|method|methods" -required 1}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default all}
  {-argName "-application"}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern" -required 0}
}
infoObjectMethod children XOTclObjInfoChildrenMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
infoObjectMethod class XOTclObjInfoClassMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod filter XOTclObjInfoFilterMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-guard"}
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern"}
}
infoObjectMethod forward XOTclObjInfoForwardMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-definition"}
  {-argName "name"}
}
infoObjectMethod hasnamespace XOTclObjInfoHasnamespaceMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod method XOTclObjInfoMethodMethod {
  {-argName "object" -type object}
  {-argName "infomethodsubcmd" -type "args|body|definition|handle|parameter|parametersyntax|type|precondition|postcondition"}
  {-argName "name"}
}
infoObjectMethod methods XOTclObjInfoMethodsMethod {
  {-argName "object" -type object}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern"}
}
infoObjectMethod mixin XOTclObjInfoMixinMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-guard"}
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern" -type objpattern}
}
infoObjectMethod parent XOTclObjInfoParentMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod precedence XOTclObjInfoPrecedenceMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-intrinsic"}
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
infoClassMethod filter XOTclClassInfoFilterMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "-guard"}
  {-argName "-guards"}
  {-argName "pattern"}
}
infoClassMethod forward XOTclClassInfoForwardMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-definition"}
  {-argName "name"}
}
infoClassMethod method XOTclClassInfoMethodMethod {
  {-argName "class" -type class}
  {-argName "infomethodsubcmd" -type "args|body|definition|handle|parameter|parametersyntax|type|precondition|postcondition"}
  {-argName "name"}
}
infoClassMethod methods XOTclClassInfoMethodsMethod {
  {-argName "class" -type class}
  {-argName "-methodtype" -nrargs 1 -type "all|scripted|builtin|alias|forwarder|object|setter"}
  {-argName "-callprotection" -nrargs 1 -type "all|protected|public" -default public}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern"}
}
infoClassMethod mixin XOTclClassInfoMixinMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "-guard"}
  {-argName "-guards"}
  {-argName "pattern" -type objpattern}
}
infoClassMethod mixinof  XOTclClassInfoMixinOfMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "-scope" -required 0 -nrargs 1 -type "all|class|object"}
  {-argName "pattern" -type objpattern}
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

