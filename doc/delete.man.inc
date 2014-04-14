[comment {-*- tcl -*- manpage fragment for delete method, shared by nx::Object and nx::Class}]

[keywords "alias method"]
[keywords "forwarder method"]

[call [arg obj] [method delete] [const [vset SCOPE]] [const property] [arg propertyName]]
[call [arg obj] [method delete] [const [vset SCOPE]] [const variable] [arg variableName]]
[call [arg obj] [method delete] [const [vset SCOPE]] [const method] [arg methodName]]

Removes a property [arg propertyName], variable [arg variableName],
and method [arg methodNome], respectively, previously defined for the
scope of the [vset SCOPE]. This method is the equivalent to Tcl's [cmd rename] for
NX-specific constructs such as properties, object variables, and
methods.
[para]
[method "delete [const [vset SCOPE]] [const method]"] can be equally used for removing regular methods (see [method "[vset SCOPE] method"]), an [term "alias method"] (see [method "[vset SCOPE] alias"]), and a [term "forwarder method"] (see [method "[vset SCOPE] forward"]).
