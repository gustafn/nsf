[comment {-*- tcl -*- manpage fragment for mixin method, shared by nx::Object and nx::Class}]

[keywords "mixin class"]
[keywords linearisation]

[call [arg [vset CMD]] [method "[vset MODIFIER] mixins"] [arg submethod] [opt "[arg arg] ..."]]

Accesses and modifies the list of [term "mixin class"]es of 
[arg [vset CMD]] using a specific setter or getter [arg submethod]:

[list_begin definitions]

[def "[arg [vset CMD]] [const [vset MODIFIER]] [method {mixins add}] [arg spec] [opt [arg index]]"]
Inserts a single [term "mixin class"] into the current list of [term "mixin class"]es of [arg [vset CMD]]. Using [arg index], a position in the existing list of [term "mixin class"]es for inserting the new [term "mixin class"] can be set. If
omitted, [arg index] defaults to the list head (0). 
[comment {Therefore, by default, any added [term "mixin class"] takes precedence over previously added classes in the overall linearisation of [arg [vset CMD]].}]

[def "[arg [vset CMD]] [const [vset MODIFIER]] [method "mixins classes"] [opt [arg pattern]]"]

If [arg pattern] is omitted, returns the object names of the [term "mixin class"]es which
extend [arg [vset CMD]] directly. By specifying [arg pattern], the returned [term "mixin class"]es can
be limited to those whose names match [arg pattern] (see [cmd "string match"]).

[def "[arg [vset CMD]] [const [vset MODIFIER]] [method {mixins clear}]"]
Removes all [term "mixin class"]es from [arg [vset CMD]] and returns the list of removed [term "mixin class"]es. Clearing is equivalent to passing an empty list for [arg mixinSpecList] to 
[const [vset MODIFIER]] [method {mixins set}].

[def "[arg [vset CMD]] [const [vset MODIFIER]] [method {mixins delete}] [opt [option -nocomplain]] [arg specPattern]"]
Removes a [term "mixin class"] from a current list of [term "mixin class"]es of [arg [vset CMD]] whose spec matches [arg specPattern]. [arg specPattern] can contain special matching chars (see [cmd "string match"]). [const [vset SCOPE]] [method "mixins delete"] will throw an error if there is no matching [term "mixin class"], unless [option -nocomplain] is set.

[def "[arg [vset CMD]] [const [vset MODIFIER]] [method {mixins get}]"]
Returns the list of current [term "mixin specification"]s.

[def "[arg [vset CMD]] [const [vset MODIFIER]] [method {mixins guard}] [arg className] [opt [arg expr]]"]

If [arg expr] is specified, a guard expression [arg expr] is registered with the [term "mixin class"] [arg className]. This requires that the corresponding [term "mixin class"] [arg className] has been previously set using [const [vset SCOPE]] [method {mixins set}] or added using [const [vset MODIFIER]] [method {mixins add}]. [arg expr] must be a valid Tcl expression (see
[cmd expr]). An empty string for [arg expr] will clear the currently registered
guard expression for the [term "mixin class"] [arg className]. 

[para]

If [arg expr] is not specified, returns the active guard
expression. If none is available, an empty string will be returned.

[def "[arg [vset CMD]] [const [vset MODIFIER]] [method {mixins set}] [arg mixinSpecList]"] 

[arg mixinSpecList] represents a list of [term "mixin class"] specs, with each spec being itself either a one-element or a three-element list: [arg className] ?-guard [arg guardExpr]?. If
having one element, the element will be considered the [arg className]
of the [term "mixin class"]. If having three elements, the third
element [arg guardExpr] will be stored as a guard expression of the
[term "mixin class"]. This guard expression will be evaluated using
[cmd expr] when [arg [vset CMD]] receives a message to determine if the mixin
is to be considered during method dispatch or not. Guard expressions
allow for realizing context-dependent or conditional mixin
composition.

[list_end]
[comment {
  A [term "mixin class"] whose spec is featured earlier in [arg mixinSpecList] takes
  precedence in the [term "linearisation"] over a [term "mixin class"]
  whose spec is listed later. The computed,
  overall [term linearisation] of [arg [vset CMD]] guarantees to maintain
  this local order of [term "mixin class"]es.
}]
At the time of setting the mixin relation, that is, calling [const [vset MODIFIER]] [method mixins], every
[arg className] as part of a spec must be an existing instance of [cmd nx::Class]. To
access and to manipulate the list of [term "mixin class"]es of [arg [vset CMD]], 
[method cget]|[method configure] [option -[join [list {*}[vset MODIFIER] mixins] -]] can also be used.
