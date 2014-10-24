[comment {-*- tcl -*- manpage fragment for delete method, shared by nx::Object and nx::Class}]

[keywords "alias method"]
[keywords "forwarder method"]

[call [arg [vset CMD]] [method [string trim "delete [vset MODIFIER]"]] [arg feature] [arg arg]]

This method serves as the equivalent to Tcl's [cmd rename] for
removing structural (properties, variables) and behavioral features
(methods) of the [vset SCOPE]:

[def "[arg [vset CMD]] [method "delete [vset MODIFIER] property"] [arg propertyName]"]
[def "[arg [vset CMD]] [method "delete [vset MODIFIER] variable"] [arg variableName]"]
[def "[arg [vset CMD]] [method "delete [vset MODIFIER] method"] [arg methodName]"]

Removes a property [arg propertyName], variable [arg variableName],
and method [arg methodName], respectively, previously defined for the
scope of the [vset SCOPE]. 
[para]
[method "delete [vset MODIFIER] method"] can be equally used for removing regular methods (see [method "[vset MODIFIER] method"]), an [term "alias method"] (see [method "[vset MODIFIER] alias"]), and a [term "forwarder method"] (see [method "[vset MODIFIER] forward"]).
