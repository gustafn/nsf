#
# == Rosetta example: Multiple distinct objects
#
#
# Create a sequence (array, list, whatever) consisting of n distinct,
# initialized items of the same type. n should be determined at
# runtime.
# 
# https://rosettacode.org/wiki/Multiple_distinct_objects
#

package req nx
package req nx::test

#
# The class +Klass+ defines and implements the item type. It can also
# be used to query its population of instances.
#

nx::Class create Klass
set n 100; # runtime parameter

#
# Wrong: Only a single item (object) is created, its (command) name is replicated +n+ times.
#

? {llength [Klass info instances]} 0;

set theList [lrepeat $n [Klass new]]

? {llength [Klass info instances]} 1;
? {llength [lsort -unique $theList]} 1;

[lindex $theList 0] destroy

#
# Correct: +n+ items (objects) having distinct (command) names are
# created and stored in the list.
#

? {llength [Klass info instances]} 0;

set theList {}

for {set i 0} {$i<$n} {incr i} {
    lappend theList [Klass new]
}

? {llength [Klass info instances]} $n;
? {llength [lsort -unique $theList]} $n;
