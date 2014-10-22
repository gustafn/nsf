[comment {-*- tcl -*- manpage fragment for forward method, shared by nx::Object and nx::Class}]

[keywords "value checker"]
[keywords "forward method"]
[keywords "debugging level"]

[call [arg [vset CMD]] [opt "public | protected | private"] [const [vset MODIFIER]] [method forward] [arg methodName] [opt "[option -prefix] [arg prefixName]"] [opt "[option -frame] [arg object]"] [opt "[option -returns] [arg valueChecker]"] [opt [option -verbose]] [opt [arg target]] [opt "[arg arg] ..."]]

Define a [term "forward method"] for the given [vset SCOPE]. The
definition of a [term "forward method"] registers a predefined, but
changeable list of forwarder arguments under the (forwarder) name [arg methodName]. Upon
calling the [term "forward method"], the forwarder
arguments are evaluated as a Tcl command call. That is, if present, [arg target]
is interpreted as a Tcl command (e.g., a Tcl [cmd proc] or an object)
and the remainder of the forwarder arguments [arg arg] as arguments passed into
this command. The actual method arguments to the invocation of the
[term "forward method"] itself are appended to the list of forwarder
arguments.

If [arg target] is omitted, the value of [arg methodName] is
implicitly set and used as [arg target]. This way, when providing a
fully-qualified Tcl command name as [arg methodName] without [arg target], the
unqualified [arg methodName] ([cmd "namespace tail"]) is used as the
forwarder name; while the fully-qualified one serves as the [arg target].


[para] 

As for a regular [method "[vset MODIFIER] method"], [option "-returns"] allows
for setting a [term "value checker"] on the values returned by the
resulting Tcl command call. When passing to [option "-frame"] [const object], the
resulting Tcl command is evaluated in the context of the object
receiving the [term "forward method"] call. This way, variable names
used in the resulting execution of a command become resolved as
object variables.

[para]

The list of forwarder arguments [arg arg] can contain as its elements
a mix of literal values and placeholders. Placeholders are prefixed
with a percent symbol (%) and substituted for concrete values upon
calling the [term "forward method"]. These placeholders allow for
constructing and for manipulating the arguments to be passed into the
resulting command call on the fly:

[list_begin itemized]

[item] [const %method] becomes substituted for the name of the [term "forward method"], i.e. [arg methodName].

[item] [const %self] becomes substituted for the name of the object receiving the call of the [term "forward method"].

[item] [const %1] becomes substituted for the first method argument passed to the call of [term "forward method"]. This requires, in turn, that [emph "at least"] one argument is passed along with the method call.
[para]
Alternatively, [const %1] accepts an optional argument [arg defaults]: {[const %1] [arg defaults]}. 
[arg defaults] must be a valid Tcl list of two elements. For the first
element, [const %1] is substituted when there is no first method
argument which can be consumed by [const %1]. The second element is
inserted upon availability of a first method argument with the
consumed argument being appended right after the second list
element. This placeholder is typically used to define a pair of
getter/setter methods.

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
(e.g., [const %method], [const %self]). Position prefixes are
exempted, they are evaluated as [const %][arg cmdName]-placeholders in this context.

[item] {[const %argclindex] [arg list]} becomes substituted for the
[emph n]th element of the provided [arg list] , with [emph n]
corresponding to the number of method arguments passed to the [term "forward method"] call.

[item] [const %%] is substituted for a single, literal percent symbol (%).

[item] [const %][arg cmdName] is substituted for the value returned
from executing the Tcl command [arg cmdName]. To pass arguments to [arg cmdName], the placeholder should be wrapped into a Tcl [cmd list]: {[const %][arg cmdName] [opt "[arg arg] ..."]}.
[para]
Consider using fully-qualified Tcl command names for [arg cmdName] to
avoid possible name conflicts with the predefined placeholders, e.g.,
[const %self] vs. %[cmd ::nx::self].

[list_end]

[para] 

To disambiguate the names of subcommands or methods, which potentially
become called by a [term "forward method"], a prefix [arg prefixName]
can be set using [option "-prefix"]. This prefix is prepended
automatically to the argument following [arg target] (i.e., a second
argument), if present. If missing, [option "-prefix"] has no
effect on the [term "forward method"] call. 

[para]
To inspect and to debug the conversions performed by the above
placeholders, setting the [term "switch"] [option "-verbose"]
will have the command list to be executed (i.e., after substitution)
printed using [cmd ::nsf::log] ([term "debugging level"]: [const notice]) upon
calling the [term "forward method"].
