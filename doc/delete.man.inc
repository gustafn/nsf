[comment {-*- tcl -*- manpage fragment for delete method, shared by nx::Object and nx::Class}]

[keywords "alias method"]
[keywords "forwarder method"]

[call [arg [vset CMD]] [method delete] [const [vset MODIFIER]] [method property]|[method variable]|[method method] [arg arg]]

This method serves as the equivalent to Tcl's [cmd rename] for
removing structural (properties, variables) and behavioral features
(methods) of the [vset SCOPE]:

[def "[arg [vset CMD]] [method delete] [const [vset MODIFIER]] [const property] [arg propertyName]"]
[def "[arg [vset CMD]] [method delete] [const [vset MODIFIER]] [const variable] [arg variableName]"]
[def "[arg [vset CMD]] [method delete] [const [vset MODIFIER]] [const method] [arg methodName]"]

Removes a property [arg propertyName], variable [arg variableName],
and method [arg methodNome], respectively, previously defined for the
scope of the [vset SCOPE]. 
[para]
[method "delete [const [vset MODIFIER]] [const method]"] can be equally used for removing regular methods (see [method "[vset MODIFIER] method"]), an [term "alias method"] (see [method "[vset MODIFIER] alias"]), and a [term "forwarder method"] (see [method "[vset MODIFIER] forward"]).
