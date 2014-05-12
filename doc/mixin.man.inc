[comment {-*- tcl -*- manpage fragment for mixin method, shared by nx::Object and nx::Class}]

[keywords "mixin class"]
[keywords linearisation]

[call [arg obj] [const [vset SCOPE]] [method mixin] [arg mixinSpecList]]

Sets one or several classes as [term "mixin class"]es of the
[vset SCOPE] [arg obj]. [method "[vset SCOPE] mixin"] accepts a list of [term "mixin class"] specs,
with each spec being itself a list: [arg className]
?-guard [arg guardExpr]?. If having one element, the element will be considered the [arg className]
of the [term "mixin class"]. If having three elements, the third
element [arg guardExpr] is stored as a guard expression of the [term "mixin class"]. This guard expression is evaluated using [cmd expr] at runtime when [arg obj] receives a message to determine if the mixin is
to be considered during method dispatch or not. Guard expressions allow for realizing context-dependent or conditional mixin composition.  
[para]
A [term "mixin class"] whose spec is featured earlier in [arg mixinSpecList] takes precedence in the [term "linearisation"] over a [term "mixin class"] whose spec is listed later. The computed, overall [term linearisation] of [arg obj] guarantees to maintain this local order of [term "mixin class"]es.
[para]
If [arg mixinSpecList] represents an empty list, any existing [term "mixin class"] is removed from [arg obj]. An element arity of a spec other than the ones documented above is reported an error.
[para]
A [term "mixin class"] spec can also be registered or unregistered in
an incremental manner, that is, one by one:
[list_begin definitions]
[def "[arg obj] [const [vset SCOPE]] [method {mixin add}] [arg spec] [opt [arg index]]"]
Inserts a single [term "mixin class"] into the current list of [term "mixin class"]es of [arg obj]. Using [arg index], a position in the existing list of [term "mixin class"]es for inserting the new [term "mixin class"] can be set. If
omitted, [arg index] defaults to the list head (0). Therefore, by default, any added [term "mixin class"] takes precedence over previously added classes in the overall linearisation of [arg obj].
[def "[arg obj] [const [vset SCOPE]] [method {mixin delete}] [option -nocomplain] [arg specPattern]"]
Removes a [term "mixin class"] from a current list of [term "mixin class"]es of [arg obj] whose spec matches [arg specPattern]. [arg specPattern] can contain special matching chars (see [cmd "string match"]). [const [vset SCOPE]] [method "mixin delete"] will throw an error if there is no matching [term "mixin class"], unless [option -nocomplain] is set.
[list_end]
At the time of setting the mixin relation, that is, calling [const [vset SCOPE]] [method mixin], every
[arg className] as part of a spec must be an existing instance of [cmd nx::Class]. To
access and to manipulate the list of [term "mixin class"]es of [arg obj], 
[method cget]|[method configure] [option -[vset SCOPE]-mixin] can also be used.

