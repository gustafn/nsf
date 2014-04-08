[comment {-*- tcl -*- manpage fragment for property method}]

[keywords property]
[keywords "slot object"]
[keywords "call protection"]
[keywords "public"]
[keywords "protected"]
[keywords "private"]
[keywords "switch"]
[keywords "value checker"]

[call [arg obj] [const [vset SCOPE]] [method property] [opt "-accessor public|private|protected"] [opt "-configurable [arg trueFalse]"] [opt -incremental] [opt "-class [arg className]"] [opt -nocomplain] [arg spec] [opt [arg initBlock]]]

Defines a [term property] for the scope of the [vset SCOPE]. For every
property, a [term "slot object"] is created to manage the
corresponding object variable.
[para]
If [option "-accessor"] is set, a property will provide for a pair of getter
and setter methods, automatically. By default, no getter and setter
methods are created. The option value passed along [option "-accessor"]
sets the level of [term "call protection"] for the getter and setter
methods: [term public], [term protected], or [term private].

[para]

By setting [option -configurable] to [const true], the property can be
accessed and modified through [method cget] and [method configure],
respectively. If [const false], the interface based on [method cget] and
[method configure] will not become available. In this case, and provided that
[option -accessor] is set, the property can be accessed and modified via
the getter/setter methods. Alternatively, the object variable, which
is represented by the property, can always be accessed and modified
directly, e.g., using [method eval]. By default, [option -configurable] is
[const true].

[para]

A custom class for the slot object implementing the property, from
which this slot object is to be instantiated, can be provided using
[option -class]. The default value is [cmd ::nx::VariableSlot].

[para]

The [arg spec] provides the [term property] specification as a [cmd list] holding
at least one element or, maximum, two elements: [emph propertyName][opt ":[emph typeSpec]"] [opt [emph defaultValue]]. The [arg propertyName] sets
the name of the property and the corresponding object variable(s). It
is, optionally, equipped with a [emph typeSpec] following a colon
delimiter which specifies a [term "value checker"] for the values
which become assigned to the property. The second, optional element
sets a [emph defaultValue] for this property.

[para]

The last argument [arg initBlock] accepts an optional Tcl script which is passed into
the initialization procedure (see [method configure]) of the [term property]'s [term "slot object"]. See
also [sectref description "[arg initBlock] for [method create] and [method new]"].

[list_begin arguments]
[arg_def switch -incremental in] ...

[list_end]
