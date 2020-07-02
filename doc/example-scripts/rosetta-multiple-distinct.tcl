#
# == Rosetta example: Multiple distinct objects
#
#
# Create a sequence (array, list, whatever) consisting of "n"
# distinct, initialized items of the same type. The value of "n"
# should be determined at run time.
# 
# https://rosettacode.org/wiki/Multiple_distinct_objects
#

package req nx
package req nx::test

#
# The class +Foo+ defines and implements the item type. It can also
# be used to query its population of instances.
#

nx::Class create Foo
set n 100; # run time parameter

#
# Wrong: Only a single item (object) is created, its (command) name is
# replicated +n+ times.
#

? {llength [Foo info instances]} 0;

set theList [lrepeat $n [Foo new]]

? {llength [Foo info instances]} 1;
? {llength [lsort -unique $theList]} 1;

[lindex $theList 0] destroy

#
# Correct: +n+ items (objects) having distinct (command) names are
# created and stored in the list.
#

? {llength [Foo info instances]} 0;

set theList {}

for {set i 0} {$i<$n} {incr i} {
    lappend theList [Foo new]
}

? {llength [Foo info instances]} 100;
? {llength [lsort -unique $theList]} 100;
