[comment {-*- tcl -*- manpage fragment for filter method, shared by nx::Object and nx::Class}]

[keywords "method filter"]

[call [arg obj] [const [vset SCOPE]] [method filter] [arg submethod] [opt "[arg arg] ..."]]

Accesses and modifies the list of methods which are registered as
[term "filter"]s with [arg obj] using a specific setter or getter 
[arg submethod]:

[list_begin definitions]
[def "[arg obj] [const [vset SCOPE]] [method {filter set}] [arg filterSpecList]"]

[arg filterSpecList] takes a list of [term "filter"] specs, with each spec being itself a
one-element or two-elements list: [arg methodName] ?[arg guardExpr]?. [arg methodName] identifies
an existing method of [arg obj] which becomes
registered as a filter. If having two elements, the second
element [arg guardExpr] will be stored as a guard expression of the
[term "filter"]. This guard expression must be a valid Tcl expression
(see [cmd expr]). [arg expr] is evaluated when [arg obj] receives a message to determine whether the
filter should intercept the message. Guard expressions
allow for realizing context-dependent or conditional filter
composition.

[def "[arg obj] [const [vset SCOPE]] [method {filter get}]"]
Returns the current list of methods registered as [term "filter"]s.

[def "[arg obj] [const [vset SCOPE]] [method {filter add}] [arg spec] [opt [arg index]]"]
Inserts a single [term "filter"] into the current list of [term "filter"]s of [arg obj]. Using [arg index], a position in the existing list of [term "filter"]s for inserting the new [term "filter"] can be set. If
omitted, [arg index] defaults to the list head (0).

[def "[arg obj] [const [vset SCOPE]] [method {filter delete}] [option -nocomplain] [arg specPattern]"]
Removes a single [term "filter"] from the current list of [term "filter"]s of
[arg obj] whose spec matches [arg specPattern]. [arg specPattern] can
contain special matching chars (see [cmd "string match"]). [const [vset SCOPE]] [method "filter delete"] will
throw an error if there is no matching [term "filter"], unless
[option -nocomplain] is set.

[def "[arg obj] [const [vset SCOPE]] [method {filter clear}]"]
Removes all [term "filter"]s from [arg obj]. This is equivalent
to passing an empty list for [arg filterSpecList] to [const [vset SCOPE]] [method {filter set}].

[def "[arg obj] [const [vset SCOPE]] [method {filter guard}] [arg methodName] [arg expr]"]

Registers a guard expression [arg expr] with a filter [arg methodName]
in a separate step, once the filter was set or added using [const [vset SCOPE]] [method {filter set}] or
[const [vset SCOPE]] [method {filter add}]. [arg expr] must be a valid Tcl expression (see
[cmd expr]). An empty string for [arg expr] will clear the currently registered
guard expression for filter [arg methodName].

[list_end]
Every [arg methodName] in a [arg spec] must resolve to an existing method in
the scope of the [vset SCOPE]. To
access and to manipulate the list of [term "filter"]s of [arg obj], 
[method cget]|[method configure] [option -[vset SCOPE]-filter] can also be used.
