[comment {-*- tcl -*- manpage fragment for forward method, shared by nx::Object and nx::Class}]

[keywords "value checker"]
[keywords "forward method"]

[call [arg [vset CMD]] [opt "public | protected | private"] [const [vset SCOPE]] [method forward] [arg methodName] [opt [option "-default [arg value]"]] [opt [option "-methodprefix [arg prefixName]"]] [opt [option "-frame object"]] [opt [option "-onerror [arg cmdName]"]] [opt [option "-returns [arg valueChecker]"]] [opt [option "-verbose"]] [arg arg] [opt "[arg arg] ..."]]

Define a [term "forward method"] for the given [vset SCOPE]. The
definition of a [term "forward method"] registers a predefined, but
changeable list of forwarder arguments under the (forwarder) name [arg methodName]. Upon
calling the [term "forward method"], the forwarder arguments are
evaluated as a Tcl command call. That is, the first
argument [arg arg] is interpreted as a Tcl command (e.g., a Tcl [cmd proc] or
an object) and the remainder of the forwarder arguments as arguments
passed into this command. The actual method arguments to the
invocation of the [term "forward method"] itself are appended to the
list of forwarder arguments.

[para] 

As for a regular [method "[vset SCOPE] method"], [option "-returns"] allows
for setting a [term "value checker"] on the values returned by the
resulting Tcl command call. When passing [const object] to [option "-frame"], the
resulting Tcl command is evaluated in the context of the object
receiving the [term "forward method"] call. [option "-onerror"] sets a
Tcl command [arg cmdName] which is called as the handler for errors
caught during evaluating the resulting Tcl command.

[para]

The list of forwarder arguments [arg arg] can contain as its elements
a mix of literal values and placeholder values. Placeholders are
prefixed with a percent symbol (%) and substituted for concrete values
upon calling the [term "forward method"]. These placeholders allow for
constructing and for manipulating the list of forwarder arguments on
the fly:

[list_begin itemized]

[item] [const %method] becomes substituted for the name of the [term "forward method"], i.e. [arg methodName].

[item] [const %self] becomes substituted for the name of the object receiving the call of the [term "forward method"].

[item] [const %1] becomes substituted for the first method argument passed to the call of [term "forward method"].

[item] {[const %@][arg index] [arg value]} becomes substituted for the
specified [arg value] at position [arg index] in the
forwarder-arguments list, with [arg index] being either a positive
integer, a negative integer, or the literal value [const end] (such as
in Tcl's [cmd lindex]). Positive integers specify a list position
relative to the list head, negative integers give a position relative
to the list tail. Indexes for positioning placeholders in the definition of a
[term "forward method"] are evaluated from left to right and should be
used in ascending order.
[para]
Note that [arg value] can be a literal or any of the placeholders
(e.g., [const %method], [const %self]). An exception are position
prefixes themselves which are evaluated according to the [const %][arg cmdName]-rule 
(see below).

[item] {[const %argclindex] [arg list]} becomes substituted for the
[emph n]th element of the provided [arg list] , with [emph n]
corresponding to the number of method arguments passed to the [term "forward method"] call.

[item] [const %%] is substituted for a single literal percent (%) symbol.

[item] [const %][arg cmdName] is substituted for the value returned
from executing the Tcl command [arg cmdName]. To pass arguments to [arg cmdName], the placeholder should be wrapped into a Tcl [cmd list]: {[const %][arg cmdName] [opt "[arg arg] ..."]}.
[para]
Consider using fully-qualified Tcl command names for [arg cmdName] to
avoid possible name conflicts with the predefined placeholders, e.g.,
[const %self] vs. %[cmd ::nx::self].

[list_end]

[para]
[comment {
# @parameter	-default	Is used for default method names (only
# 				in connection with %1)
# @parameter 	-methodprefix 	Prepends the specified prefix to the second
# 				argument of the invocation.
}]

[para]
To inspect and to debug the substitutions performed by the above
placeholder values, setting the [term "switch"] [option "-verbose"]
will have the resulting Tcl command (i.e., after substitution) printed to
[const stdout] upon calling the [term "forward method"].

[comment {#
# @parameter 	method 		The name of the delegating or forwarder method
# @parameter 	-objframe	Causes the target to be evaluated in
# 				the scope of the object.
# @parameter 	-onerror	Register an error handler
# @parameter 	-returns	Provide an out parameter
# 				specification, used to check the
# 				return value of the forward dispatch.
# @parameter	-verbose 	Print the substituted command string to stderr
# 				before performing the command
# 				execution. For debugging purposes.
}]
