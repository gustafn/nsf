[comment {-*- tcl -*- manpage fragment for variable method, shared by nx::Object and nx::Class}]

[keywords "variable"]

[comment {
  At the time of writing, the only material difference between
  property and variable is a different -configurable default.
}]

Defines a [term variable] for the scope of the [vset SCOPE].

[para] 

If [option "-accessor"] is set explicitly, a [term variable] will provide for a pair of
getter and setter methods, automatically. By default, no getter and
setter methods are created. The option value passed along [option "-accessor"] sets
the level of [term "call protection"] for the getter and setter
methods: [term public], [term protected], or [term private].

[para]

By setting [option -configurable] to [const true], the [term variable] can be
accessed and modified through [method cget] and [method configure],
respectively. If [const false] (the default), the interface based on [method cget] and
[method configure] will not become available. In this case, and provided that
[option -accessor] is set, the [term variable] can be accessed and modified via
the getter/setter methods. Alternatively, the underlying Tcl variable, which
is represented by the [term variable], can always be accessed and modified
directly, e.g., using [method eval]. By default, [option -configurable] is
[const false].

[para]

Provided a [term "slot object"] managing the [term variaböe] is to be
created, a custom class [arg className] from which this [term "slot object"] is
to be instantiated can be set using [option -class]. The
default value is [cmd ::nx::VariableSlot].

[para]

The [arg spec] provides the [term variable] specification in form of [arg variableName][opt ":[emph typeSpec]"]. The [arg variableName] sets
the name of the [term variable] and the corresponding Tcl variable(s). It
is, optionally, equipped with a [arg typeSpec] following a colon
delimiter which specifies a [term "value checker"] for the values
managed by the [term variable]. Optionally, a [emph defaultValue] can be defined.

[para]

Using [option -initblock], an optional Tcl [arg script] can be defined which becomes passed into
the initialization procedure (see [method configure]) of the [term variable]'s [term "slot object"]. See
also [sectref description "[arg initBlock] for [method create] and [method new]"].

[para]

Turning on the [term switch] [option -incremental] provides a refined
setter interface to the value managed by the [term variable]. First,
setting [option -incremental] implies requesting [option -accessor]
([emph public] by default, if not specified
explicitly). Second, the managed value will be considered a valid Tcl
list. A [term multiplicity] of [const 0..*] is set by default, if not
specified explicitly as part of [arg spec] (see above). Third, to
manage this list value element-wise ([emph incrementally]), two
additional operations become available:

[list_begin definitions]
[def "[arg obj] [arg variableName] [method add] [arg element] [opt [arg index]]"]
Adding [arg element] to the managed list value, at the list position given by [arg index] (by default: 0).
[def "[arg obj] [arg variableName] [method delete] [arg element] [opt [arg index]]"]
Removing the first occurrence of [arg value] from the managed list value, starting from list position [arg index] (by default: [const end]).
[list_end]

In summary, a [term "slot object"] for the [term variable] is provided under any of the following conditions:
[list_begin itemized]
[item] [option -configurable] equals [const true].
[item] [option -accessor] is one of [const public], [const protected], or [const private].
[item] [option -incremental] is turned on.
[item] [option -initblock] is not assigned an empty string.
[list_end]



[comment {
  # @class.method {Object variable}
  #	 
  # Defines a per-object variable. Per default, no getter and setter
  # methods for this object variable are created, unless requested
  # explicitly. A defaul value can be specified.
  #
  # @parameter 	-accessor:switch	If provided, a pair of getter
  # 		and setter methods, named after the variable, are
  # 		created on the fly.
  # @parameter 	-class Allows for specifying a class for the
  # 		managing <<@gls slotobject>> other than the default
  # 		slot class, i.e., <<@class VariableSlot>>. Note that a
  # 		slot object is not necessarily created by
  # 		'''variable''', only if needed (i.e., accessors are
  # 		requested, an init script is provided)
  # @parameter 	-initblock An optional Tcl script which is evaluated
  #            	for the scope of the variable-managing <<@gls
  #            	slotobject>> during its initialization.
  # @parameter 	-nocomplain:switch 	If this flag is provided, an
  # 		existing object variable by the property name will not
  # 		be overwritten. Instead, an error exception is thrown.
  # @parameter 	spec 	The variable specification is a single element
  #            	which specifies the variable name, optionally followed by
  #            	parameter types after a colon delimiter. 
  # @parameter	defaultValue	If provided, sets the default value for this
  # 				object variable.
}]
