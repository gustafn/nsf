[comment {-*- tcl -*- manpage fragment for forward method, shared by nx::Object and nx::Class}]

[keywords "call protection"]

[call [arg [vset CMD]] [opt "public | protected | private"] [method require] [const [vset SCOPE]] [method method] [arg methodName]]

Attempts to register a method definition made available using [cmd ::nsf::method::provide] under
the name [arg methodName] with the object [arg obj] . The registered
method is subjected to default [term "call protection"] ([const protected]), if
not set explicitly.
