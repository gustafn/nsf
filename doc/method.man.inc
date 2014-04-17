[comment {-*- tcl -*- manpage fragment for method method, shared by nx::Object and nx::Class}]

[call [arg [vset CMD]] [opt "public | protected | private"] [const [vset SCOPE]] [method method] [arg name] [arg parameters] [opt [option "-checkalways"]] [opt [option "-returns [arg valueChecker]"]] [arg body]]

[keywords "value checker"]
[keywords "colon-prefix notation"]
[keywords "self call"]
[keywords "alias method"]

Defines a scripted method [arg methodName] for the scope of the [vset SCOPE]. The
method becomes part of the [vset SCOPE]'s signature interface. Besides
a [arg methodName], the method definition specifies
the method [arg parameters] and a method [arg body].

[para]

[arg parameters] accepts a Tcl [cmd list] containing an arbitrary
number non-positional and positional parameter definitions. Each parameter
definition comprises a parameter name, a parameter-specific [term "value checker"], and
parameter options.

[para]

The [arg body] contains the method implementation as a script block. In this body script, the
colon-prefix notation is available to denote an object variable and
a [term "self call"]. In addition, the context of the object receiving the
method call (i.e., the message) can be accessed (e.g., using [cmd nx::self]) and
the call stack can be introspected (e.g., using [cmd nx::current]).

[para]

Optionally, [option "-returns"] allows for setting a [term "value checker"] on
values returned by the method implementation. By setting
the [term "switch"] [option "-checkalways"], value checking on
arguments and return value is guaranteed to be performed, even if
value checking is temporarily disabled; see [cmd nx::configure]).

[para]

A method closely resembles a Tcl [cmd proc], but it differs in some
important aspects: First, a method can define non-positional
parameters and value checkers on arguments. Second, the script
implementing the method body can contain object-specific notation and
commands (see above). Third, method calls [emph cannot] be intercepted
using Tcl [cmd trace]. Note that an existing Tcl [cmd proc] can be registered as
an [term "alias method"] (see [method "[vset SCOPE] alias"]).

[comment {TODO: refer to nsf::proc?}]
