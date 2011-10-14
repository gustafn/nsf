# == Rosetta Example: Multiple distinct objects
# For details see http://rosettacode.org/wiki/Multiple_distinct_objects
#
package req nx
package req nx::test

# Set the number of objects that should be created
? {set n 100} 100

# Create a sequence as a list with +n+ instances of the class +nx::Object+
? {set sequence {}} ""
? {for {set i 0} {$i < $n} {incr i} {
    lappend sequence [nx::Object new]
}
} ""


