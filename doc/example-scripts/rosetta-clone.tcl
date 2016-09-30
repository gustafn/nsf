#
# == Rosetta example: Polymorphic copy
#
# Let a polymorphic object contain an instance of some specific type S
# derived from a type T. The type T is known. The type S is possibly
# unknown until run time. The objective is to create an exact copy of
# such polymorphic object.

# https://rosettacode.org/wiki/Polymorphic_copy#Ruby
#

package req nx
package req nx::test

#
# nx::Object provides a method +copy+ which creates a deep copy of any
# source object (hence, polymorphic in the sense of this task), i.e.,
# it contains all structural and behavioral features of the source and
# preserves its signature type.
#

nx::Class create T {
    :property -accessor public {label "T"}
}
nx::Class create S -superclasses T {
    :property -accessor public {label "S"}
}

set client [nx::Object new {
    :public object method duplicate {src} {
	# this is the polymorphic call site
	return [$src copy]
    }
}]

set t [T new]
? {$t label get} "T" 	    
set s [S new]
? {$s label get} "S"

#
# Provide two copies, using +copy+ underneath
#
set t2 [$client duplicate $t]
set s2 [$client duplicate $s]

#
# Are the copies truly independent objects (identities)? Yes ...
#

? {expr {$t2 ne $t}} 1
? {expr {$s2 ne $s}} 1

#
# Are the copies offsprings of the source types/classes? Yes ...
#

? {$t info class} "::T"
? {$t2 info class} "::T"

? {$s info class} "::S"
? {$s2 info class} "::S"

#
# Do the copies operate exactly like their source objects? Yes ...
#

? {$t label get} "T"
? {$t2 label get} "T"

? {$s label get} "S"
? {$s2 label get} "S"

