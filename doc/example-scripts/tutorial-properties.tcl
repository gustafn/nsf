# NX supports the concept of properties, which can be used to
# configure instance variables of a class (or an object) at creation
# or run time and to add standardized accessor methods.

package req nx
package req nx::test

# In a first step, we create a class named +Foo+ with three
# properties, +x+, +y+ and +z+.

? {nx::Class create Foo {
    :property x
    :property -accessor public y
    :property -incremental z:0..n
}} ::Foo


# By defining something as a property, it will be possible 
# (1) to specify a value for the same-named instance variable at creation time,
# (2) to modify the instance variable via +configure+, and
# (3) to access the value of this instance variable from the via the +cget+ method.

? {Foo create f1 -x 1} ::f1

? {f1 cget -x} 1
? {f1 configure -x 2} ""
? {f1 cget -x} 2

# When a property is defined with an accessor (see property +y+), then
# an accessor method is added with the specified protection level. The
# accessor method has the same name as the property. All accessor
# method can be used to +set+ and +get+ the value of the instance
# variable.

? {f1 y set 3} 3
? {f1 y get} 3

# Certainly, the methods +configure+ and +cget+ can be used also for
# these properties as in the example with property +x+.

? {f1 cget -y} 3

# You might wonder, why these accessor methods are needed, when the
# same behavior can be achieved via +cget+ and +configure+. The main
# difference is that the accessor methods are tailorable (the behavior
# can be modified) and extensible (more subcommands like +set+ and
# +get+ can be defined). The details, how this is done is not covered
# in this chapter. However, when a property is defined as
# +incremental+ the sub-methods are extended.

# When a property is defined as +incremental+, a public accessor and
# multi-valued are assumed. By specifying +incremental+ the
# sub-commands +add+ and +delete+ are provided to the accessor, which
# allows one to add or delete values to a multi-valued property
# incrementally (see property +z+). The term incrementally means here
# that one can e.g. add a value to the list without the need to +get+
# the values of the list in a first step, to +lappend+ the value, and
# to +set+ the variable to the resulting list. The +add+ sub-command
# is similar to +push+ in MongoDB or +LPUSH+ in redis.

? {f1 z add 1} 1
? {f1 z add 2} {2 1}
? {f1 z add 3 end} {2 1 3}

? {f1 z get} {2 1 3}

# The sub-command +delete+ can be used to delete a certain value from
# the list of value.

? {f1 z delete 2} {1 3}
