#
# == Rosetta example: Add a variable to a class instance at runtime
#
# Demonstrate how to dynamically add variables to an object (a class instance) at runtime.

# https://rosettacode.org/wiki/Add_a_variable_to_a_class_instance_at_runtime#Tcl
#

package req nx
package req nx::test

#
# The class +Empty+ does not provide any structural or behaviora
# features on behalf of future instances, they will remain empty.
#

nx::Class create Empty

#
# Provide one instance of +Empty+ to add an object variable to ...
#

Empty create ::e

# Is +e+ truly empty?

? {::e info vars} ""

#
# NX offers different types of object-variable managers: properties,
# variable slots, and plain Tcl per-object variables. Below, we
# showcase variable slots (although the others can be added
# dynamically alike).
#

# a) Declare a variable slot +foo+; +-accessor+ will provide
# getter/setter methods for this one object +e+ automatically.

::e object variable -accessor public foo

# b) Define a value for the variable slot +foo+
? {::e foo set 1} 1

# c) Is there a Tcl variable managed by the variable slot?
? {::e info vars} "foo"

# d) Retrieve +foo+'s value
? {::e foo get} 1

#
# A second instance of +Empty+ has no such capability: +foo+
#

Empty create ::f

# a) Is there any Tcl variable, one named +foo+? No ...
? {::f info vars} ""
# b) Are there getter/setter methods for a +foo+? No ...
? {::f foo set} "::f: unable to dispatch method 'foo'"
# c) Is there a variable slot +foo+? No ...
? {::f info object variables foo} ""

#
# In NX, once dynamically added, a variable slot can also be dynamically removed again.
#

::e delete object variable foo

# a) Is the variable slot +foo+ gone? Yes ...
? {::e info object variables foo} ""

# b) Is the Tcl variable gone? Yes ...
? {::e info vars} {}

# c) Are the getter/setter methods gone? Yes ...
? {::e foo get} "::e: unable to dispatch method 'foo'"
