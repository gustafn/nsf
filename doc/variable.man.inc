[comment {-*- tcl -*- manpage fragment for variable method, shared by nx::Object and nx::Class}]

[keywords "variable"]

[comment {
  At the time of writing, the only material difference between
  property and variable is a different -configurable default.
}]

Defines a [term variable] for the scope of the [vset SCOPE]. The [arg spec] provides
the [term variable] specification: [arg variableName][opt "[const :][arg typeSpec]"]. The
[arg variableName] will be used to name the underlying Tcl variable
and the getter/setter methods, if requested (see [option "-accessor"]).
[arg spec] is optionally equipped with a [arg typeSpec] following a colon
delimiter which specifies a [term "value checker"] for the values
managed by the [term variable]. Optionally, a [emph defaultValue] can
be defined.

[para] 

If [option "-accessor"] is set explicitly, a [term variable] will provide for a pair of
getter and setter methods:

[list_begin definitions]
[def "[arg obj] [arg variableName] [method set] [arg varValue]"]
Sets [arg variableName] to [arg varValue].
[def "[arg obj] [arg variableName] [method get]"]
Returns the current value of [arg variableName].
[def "[arg obj] [arg variableName] [method unset]"]
Removes [arg variableName], if existing, underlying the property.
[list_end]

The option value passed along [option "-accessor"] sets the level of
[term "call protection"] for the getter and setter methods: [const public], 
[const protected], or [const private]. By default, no getter and setter
methods are created.

[para]

Turning on the [term switch] [option -incremental] provides a refined
setter interface to the value managed by the [term variable]. First,
setting [option -incremental] implies requesting [option -accessor]
([const public] by default, if not specified
explicitly). Second, the managed value will be considered a valid Tcl
list. A [term multiplicity] of [const 1..*] is set by default, if not
specified explicitly as part of [arg spec] (see above). Third, to
manage this list value element-wise ([emph incrementally]), two
additional setter operations become available:

[list_begin definitions]
[def "[arg obj] [arg variableName] [method add] [arg element] [opt [arg index]]"]
Adding [arg element] to the managed list value, at the list position given by [arg index] (by default: 0).
[def "[arg obj] [arg variableName] [method delete] [arg elementPattern]"]
Removing one or multiple elements from the managed list value which
match [arg elementPattern]. [arg elementPattern] can contain matching
characters (see [cmd "string match"]).
[list_end]

[para]

By setting [option -configurable] to [const true], the [term variable] can be
accessed and modified via [method cget] and [method configure],
respectively. If [const false] (the default), the interface based on [method cget] and
[method configure] will not become available. In this case, and provided that
[option -accessor] is set, the [term variable] can be accessed and modified via
the getter/setter methods. Alternatively, the underlying Tcl variable, which
is represented by the [term variable], can always be accessed and modified
directly, e.g., using [method eval]. By default, [option -configurable] is
[const false].

[para]

A [term variable] becomes implemented by a [term "slot object"] under any of the following conditions:
[list_begin itemized]
[item] [option -configurable] equals [const true].
[item] [option -accessor] is one of [const public], [const protected], or [const private].
[item] [option -incremental] is turned on.
[item] [option -initblock] is a non-empty string.
[list_end]

Provided a [term "slot object"] managing the [term variable] is to be
created, a custom class [arg className] from which this [term "slot object"] is
to be instantiated can be set using [option -class]. The
default value is [cmd ::nx::VariableSlot].

[para]

Using [option -initblock], an optional Tcl [arg script] can be defined which becomes passed into
the initialization procedure (see [method configure]) of the [term variable]'s [term "slot object"]. See
also [sectref description "[arg initBlock] for [method create] and [method new]"].
