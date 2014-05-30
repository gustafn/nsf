[comment {-*- tcl -*- 
  manpage fragment for "info *" subset shared by nx::Object and nx::Class
}]	 

[keywords "method filter"]
[keywords "submethod"]
[keywords "parameter specification"]
[keywords "type specification"]
[keywords "method handle"]
[keywords "method path"]

[call [arg [vset CMD]] [method "info [vset MODIFIER] filter"] [arg option] [opt "[arg arg] ..."]]

[list_begin definitions]

[def "[arg [vset CMD]] [method "info [vset MODIFIER] filter methods"] [opt [option -guards]] [opt [arg pattern]]"]


If [arg pattern] is omitted, returns all filter names which are
defined by [arg obj]. By turning on the [term switch] [option -guards],
the corresponding guard expressions, if any, are also
reported along with each filter as a three-element list: [arg filterName] 
-guard [arg guardExpr]. The returned filters can be limited to those whose names
match [arg patterns] (see [cmd "string match"]).

[def "[arg [vset CMD]] [method "info [vset MODIFIER] filter guard"] [arg name]"]

Returns the guard expression set on the [term "filter"] [arg name]
defined for [arg obj]. If none is available, an empty string will
be returned.

[list_end]

[call [arg [vset CMD]] [method "info [vset MODIFIER] method"] [arg option] [arg methodName]]

This introspection [term "submethod"] provides access to the details
of [arg methodName] provided by [arg obj]. Permitted values for
[arg option] are:

[list_begin itemized]

[item] [const args] returns a list containing the parameter names of
[arg methodName], in order of the method-parameter specification.

[item] [const body] returns the body script of [arg methodName].

[item] [const definition] returns a canonical command list which allows for (re-)define [arg methodName].

[item] [const definitionhandle] returns the [term "method handle"] for a [term "submethod"] in a [term "method ensemble"] from the perspective of [arg obj] as method provider. [arg methodName] must contain a complete [term "method path"].

[item] [const exists] returns 1 if there is a [arg methodName] provided by [arg obj], returns 0 otherwise.

[item] [const handle] returns the [term "method handle"] for [arg methodName].

[item] [const origin] returns the aliased command if [arg methodName] is an [term "alias method"], or an empty string otherwise.

[item] [const parameters] returns the [term "parameter specification"] of [arg methodName] as
a list of parameter names and type specifications.

[item] [const registrationhandle] returns the [term "method handle"] for a [term "submethod"] in a [term "method ensemble"] from the perspective of the method caller. [arg methodName] must contain a complete [term "method path"].

[item] [const returns] gives the [term "type specification"] defined
for the return value of [arg methodName].

[item] [const submethods] returns the names of all [term "submethod"]s of [arg methodName], if [arg methodName] is a [term "method ensemble"]. Otherwise, an empty string is returned.

[item] [const syntax] returns the method parameters of [arg methodName] as a
concrete-syntax description to be used in human-understandable
messages (e.g., errors or warnings, documentation strings).

[item] [const type] returns whether [arg methodName] is a [emph scripted] method, an [emph alias] method, a [emph forwarder] method, or a [emph setter] method.

[comment {
  [item] [const object] 
  [item] [const nsfproc]
  [item] [const builtin] denotes methods provided a [term "baseclass"];

}]


[list_end]

[call [arg [vset CMD]] [method "info [vset MODIFIER] methods"] [opt "[option -callprotection] [arg level]"] [opt "[option -type] [arg methodType]"] [opt [option -path]] [opt [arg namePattern]]]

Returns the names of all methods defined by [arg obj]. Methods covered
include those defined using ,
[const [vset SCOPE]] [method alias], and [const [vset SCOPE]] [method forward]. The
returned methods can be limited to those whose names match [arg namePattern] (see
[cmd "string match"]).

[para]

By setting [option -callprotection], only methods of a certain [term "call protection"] [arg level] ([term "public"], [term "protected"], or [term "private"]) will be returned. Methods of a specific type can be requested using [option "-type"]. The recognized values for [arg methodType] are:
[list_begin itemized]
[item] [const scripted] denotes methods defined using [const [vset SCOPE]] [method method];
[item] [const alias] denotes [term "alias method"]s defined using [const [vset SCOPE]] [method alias];
[item] [const forwarder] denotes [term "forwarder method"]s defined using [const [vset SCOPE]] [method forward];
[item] [const setter] denotes methods defined using [cmd ::nsf::setter];
[item] [const all] returns methods of any type, without restrictions (also the default value);
[comment {
  [item] [const object] 
  [item] [const nsfproc]
  [item] [const builtin] denotes methods provided a [term "baseclass"];

}]
[list_end]

[call [arg [vset CMD]] [method "info [vset MODIFIER] mixin"] [arg option] [opt "[arg arg] ..."]]

[list_begin definitions]

[def "[arg [vset CMD]] [method "info [vset MODIFIER] mixin classes"] [opt [option -guards]] [opt [arg pattern]]"]

If [arg pattern] is omitted, returns the object names of the [term "mixin class"]es which
extend [arg obj] directly. By turning on the [term switch] [option -guards],
the corresponding guard expressions, if any, are also
reported along with each mixin as a three-element list: [arg className] 
-guard [arg guardExpr]. The returned [term "mixin class"]es can be limited to those whose names
match [arg patterns] (see [cmd "string match"]).

[def "[arg [vset CMD]] [method "info [vset MODIFIER] mixin guard"] [arg name]"]

For the [term "mixin class"] [arg name] which extends
[arg obj] directly, returns the corresponding guard expression, if
any. If none is available, an empty string will be returned.

[list_end]


[call [arg [vset CMD]] [method "info [vset MODIFIER] slots"] [opt "[option -type] [arg className]"] [arg pattern]]

If [arg pattern] is not specified, returns the object names of all [term "slot object"]s defined by [arg obj]. The returned [term "slot object"]s can be limited according to any or a
combination of the following criteria: First, [term "slot object"]s
can be filtered based on their command names matching [arg namePattern] (see [cmd "string
match"]). Second, [option "-type"] allows to select
[term "slot object"]s which are instantiated from a subclass [arg className] of [cmd nx::Slot] (default: [cmd nx::Slot]).

[call [arg [vset CMD]] [method "info [vset MODIFIER] variables"] [arg pattern]]

If [arg pattern] is omitted, returns the object names of all [term "slot object"]s provided
by [arg obj] which are responsible for managing properties and variables of [arg obj]. Otherwise,
only [term "slot object"]s whose names match [arg pattern] are
returned.

[para]

This is equivalent to calling: [arg obj] [method "info [vset MODIFIER] slots"] -type ::nx::VariableSlot [opt [arg namePattern]].

[para]

To extract details of each [term "slot object"], use the [method info]
submethods available for each [term "slot object"].

