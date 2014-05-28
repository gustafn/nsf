[comment {-*- tcl -*- 
  manpage fragment for "info *" subset shared by nx::Object and nx::Class
}]	 

[keywords "method filter"]

[call [arg obj] [method "info [vset SCOPE] filter"] [arg option] [opt "[arg arg] ..."]]

[list_begin definitions]


[def "[arg obj] [method "info [vset SCOPE] filter methods"] [opt [option -guards]] [opt [arg pattern]]"]


If [arg pattern] is omitted, returns all filter names which are
defined by [arg obj]. By turning on the [term switch] [option -guards],
the corresponding guard expressions, if any, are also
reported along with each filter as a three-element list: [arg filterName] 
-guard [arg guardExpr]. The returned filters can be limited to those whose names
match [arg patterns] (see [cmd "string match"]).

[def "[arg obj] [method "info [vset SCOPE] filter guard"] [arg name]"]

Returns the guard expression set on the [term "filter"] [arg name]
defined for [arg obj]. If none is available, an empty string will
be returned.

[list_end]

[call [arg obj] [method {info object method}] ...]
[para] Description of [method {info object method args}] ...

[call [arg obj] [method {info object method body}] ...]
[para] Description of [method {info object method body}] ...

[call [arg obj] [method {info object method definition}] ...]
[para] Description of [method {info object method definition}] ...

[call [arg obj] [method {info object method definitionhandle}] ...]
[para] Description of [method {info object method definitionhandle}] ...

[call [arg obj] [method {info object method exists}] ...]
[para] Description of [method {info object method exists}] ...

[call [arg obj] [method {info object method handle}] ...]
[para] Description of [method {info object method handle}] ...

[call [arg obj] [method {info object method origin}] ...]
[para] Description of [method {info object method origin}] ...

[call [arg obj] [method {info object method parameters}] ...]
[para] Description of [method {info object method parameters}] ...

[call [arg obj] [method {info object method registrationhandle}] ...]
[para] Description of [method {info object method registrationhandle}] ...

[call [arg obj] [method {info object method returns}] ...]
[para] Description of [method {info object method returns}] ...

[call [arg obj] [method {info object method submethods}] ...]
[para] Description of [method {info object method submethods}] ...

[call [arg obj] [method {info object method syntax}] ...]
[para] Description of [method {info object method syntax}] ...

[call [arg obj] [method {info object method type}] ...]
[para] Description of [method {info object method type}] ...

[call [arg obj] [method "info [vset SCOPE] methods"] [opt "[option -callprotection] [arg level]"] [opt "[option -type] [arg methodType]"] [opt [option -path]] [opt [arg namePattern]]]

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

[call [arg obj] [method "info [vset SCOPE] mixin"] [arg option] [opt "[arg arg] ..."]]

[list_begin definitions]

[def "[arg obj] [method {info object mixin classes}] [opt [option -guards]] [opt [arg pattern]]"]

If [arg pattern] is omitted, returns the object names of the [term "mixin class"]es which
extend [arg obj] directly. By turning on the [term switch] [option -guards],
the corresponding guard expressions, if any, are also
reported along with each mixin as a three-element list: [arg className] 
-guard [arg guardExpr]. The returned [term "mixin class"]es can be limited to those whose names
match [arg patterns] (see [cmd "string match"]).

[def "[arg obj] [method {info object mixin guard}] [arg name]"]

For the [term "mixin class"] [arg name] which extends
[arg obj] directly, returns the corresponding guard expression, if
any. If none is available, an empty string will be returned.

[list_end]


[call [arg obj] [method "info [vset SCOPE] slots"] [opt "[option -type] [arg className]"] [arg pattern]]

If [arg pattern] is not specified, returns the object names of all [term "slot object"]s defined by [arg obj]. The returned [term "slot object"]s can be limited according to any or a
combination of the following criteria: First, [term "slot object"]s
can be filtered based on their command names matching [arg namePattern] (see [cmd "string
match"]). Second, [option "-type"] allows to select
[term "slot object"]s which are instantiated from a subclass [arg className] of [cmd nx::Slot] (default: [cmd nx::Slot]).

[call [arg obj] [method "info [vset SCOPE] variables"] [arg pattern]]

If [arg pattern] is omitted, returns the object names of all [term "slot object"]s provided
by [arg obj] which are responsible for managing properties and variables of [arg obj]. Otherwise,
only [term "slot object"]s whose names match [arg pattern] are
returned.

[para]

This is equivalent to calling: [arg obj] [method "info [vset SCOPE] slots"] -type ::nx::VariableSlot [opt [arg namePattern]].

[para]

To extract details of each [term "slot object"], use the [method info]
submethods available for each [term "slot object"].

