[comment {-*- tcl -*- manpage fragment for property method, shared by nx::Object and nx::Class}]

[keywords "property"]
[keywords "slot object"]
[keywords "call protection"]
[keywords "public"]
[keywords "protected"]
[keywords "private"]
[keywords "switch"]
[keywords "value checker"]
[keywords "multiplicity"]

[comment {
  At the time of writing, the only material difference between
  property and variable is a different -configurable default.
}]

Defines a [term property] for the scope of the [vset SCOPE]. By
default, a property defines a configuration option for the [vset SCOPE] to
be accessed and modified using [method cget] and [method configure],
respectively (see also [option -configurable] below). Therefore,
assuming default settings, every [term property] is managed by a [term "slot object"].

[para] 

If [option "-accessor"] is set explicitly, a property will provide for a pair of
getter and setter methods, automatically. By default, no getter and
setter methods are created. The option value passed along [option "-accessor"] sets
the level of [term "call protection"] for the getter and setter
methods: [term public], [term protected], or [term private].

[para]

By setting [option -configurable] to [const true] (the default), the property can be
accessed and modified through [method cget] and [method configure],
respectively. If [const false], the interface based on [method cget] and
[method configure] will not become available. In this case, and provided that
[option -accessor] is set, the property can be accessed and modified via
the getter/setter methods. Alternatively, the object variable, which
is represented by the property, can always be accessed and modified
directly, e.g., using [method eval]. By default, [option -configurable] is
[const true].

[para]

Provided a [term "slot object"] managing the [term property] is to be
created, a custom class [arg className] from which this [term "slot object"] is
to be instantiated can be set using [option -class]. The
default value is [cmd ::nx::VariableSlot].

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

[para]

Turning on the [term switch] [option -incremental] provides a refined
setter interface to the value managed by the [term property]. First,
setting [option -incremental] implies requesting [option -accessor]
(set to [emph public] by default, if not specified
explicitly). Second, the managed value will be considered a valid Tcl
list. A [term multiplicity] of [const 0..*] is set by default, if not
specified explicitly as part of [arg spec] (see above). Third, to
manage this list value element-wise ([emph incrementally]), two
additional operations become available:

[list_begin definitions]
[def "[arg obj] [arg propertyName] [method add] [arg element] [opt [arg index]]"]
Adding [arg element] to the managed list value, at the list position given by [arg index] (by default: 0).
[def "[arg obj] [arg propertyName] [method delete] [arg element] [opt [arg index]]"]
Removing the first occurrence of [arg value] from the managed list value, starting from list position [arg index] (by default: [const end]).
[list_end]

In summary, a [term "slot object"] for the [term property] is provided under any of the following conditions:
[list_begin itemized]
[item] [option -configurable] equals [const true] (by default).
[item] [option -accessor] is one of [const public], [const protected], or [const private].
[item] [option -incremental] is turned on.
[item] [arg initBlock] is not an empty string.
[list_end]
