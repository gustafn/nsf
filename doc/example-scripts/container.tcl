#
# This example is a small design study to implement container classes
# with different features, namely a +SimpleContainer+, an
# +OrderedContainer+ and a +SortedContainer+. First of all, we require NX:
#

package req nx
package req nx::test
nx::test configure -count 1

# == Simple Container
#
# The first container class presented here is called
# +SimpleContainer+, which manages its contained items. As all
# container classes presented here, the items are created as child
# objects embedded in the container. If the container is deleted, all
# items are deleted as well.  The items, which will be put into the
# container, should be instances of a certain class. We define here
# for this purpose an arbitrary class +C+:
nx::Class create C 

# The class +SimpleContainer+ keeps and manages items added to it.
# Every instance of this class might have different item classes. We
# might provide a prefix for naming the items, otherwise the default
# is +member+.
#
nx::Class create SimpleContainer {
  :property {memberClass ::MyItem}
  :property {prefix member}

  # Require the method "autoname" for generating nice names
  :require method autoname

  # The method new is responsible for creating a child of the current
  # container.
  :public method new {args} {
    set item [${:memberClass} create [:]::[:autoname ${:prefix}] {*}$args]
    return $item
  }
}

# Create and instance of the class +SimpleContainer+ ...
? {SimpleContainer create container1 -memberClass ::C} ::container1

# and add a few items:
? {container1 new} "::container1::member1"

? {container1 new} "::container1::member2"

? {container1 new} "::container1::member3"

# The elements of the container can be obtained via +info children+:
? {container1 info children} "::container1::member1 ::container1::member2 ::container1::member3"

# == Ordered Container
#
# In the example with +SimpleContainer+, the order of the results of
# +info children+ just happens to be in the order of the added items,
# but in general, this order is not guaranteed, but depends on the
# population of the hash tables. In the next step, we extend the
# example above by preserving the order of the elements.

# The class +OrderedContainer+ is similar to +SimpleContainer+, but
# keeps a list of items that were added to the container. The item
# list is managed in a property +items+ which is defined as
# +incremental+ to make use of the +add+ and +delete+ methods provided
# by the slots.
#
nx::Class create OrderedContainer -superclass SimpleContainer {
  :property -incremental {items:0..n {}}

  :public method new {args} {
    set item [${:memberClass} create [:]::[:autoname ${:prefix}] {*}$args]
    :items add $item end
    return $item
  }

  # Since we keep the list of items, we have to maintain it in case
  # items are deleted.
  :public method delete {item:object} {
    :items delete $item
    $item destroy
  }

}

# Create an instance of +OrderedContainer+ ...
? {OrderedContainer create container2 -memberClass ::C} "::container2"

# and add a few items:
? {container2 new} "::container2::member1"

? {container2 new} "::container2::member2"

? {container2 new} "::container2::member3"

# The elements of the container are obtained via the method +items+.
? {container2 items} "::container2::member1 ::container2::member2 ::container2::member3"

# When we delete an item in the container ...
? {container2 delete ::container2::member2} ""

# the item is as well removed from the +items+ list.
? {container2 items} "::container2::member1 ::container2::member3"

# == Sorted Container
#
# In the next step, we define a +SortedContainer+, that keeps
# additionally a sorted list for iterating through the items without
# needing to sort the items when needed.  The implementation maintains
# an additional sorted list. The implementation of the SortedContainer
# depends on "lsearch -bisect" which requires Tcl 8.6. Therefore, if
# we have no Tcl 8.6, just return here.
if {[info command yield] eq ""} return

# For sorting, we require the item class to have a key, that can be
# freely specified. We use there the property +name+ of Class +D+:

nx::Class create D {
  :property name:required
}

nx::Class create SortedContainer -superclass OrderedContainer {
  
  # In order to keep the index consisting of just the objects and to
  # ease sorting, we maintain two list, one list of values and one
  # list of objects. We assume for the time being, that the keys are
  # not changing.

  :variable values {}
  :variable index {}
  :property key

  :public method index {} { return ${:index}}

  :public method new {args} {
    set item [${:memberClass} create [:]::[:autoname ${:prefix}] {*}$args]
    if {[info exists :key]} {
      set value [$item cget -${:key}]
      set pos [lsearch -bisect ${:values} $value]
      set :values [linsert ${:values} [expr {$pos + 1}] $value]
      set :index  [linsert ${:index}  [expr {$pos + 1}] $item]
    }
    lappend :items $item
    return $item
  }

  # Since we keep the list of items, we have to maintain it in case
  # items are deleted.
  :public method delete {item:object} {
    set pos [lsearch ${:index} $item]
    if {$pos == -1} {error "item $item not found in container; items: ${:index}"}
    set :values [lreplace ${:values} $pos $pos]
    set :index  [lreplace ${:index}  $pos $pos]
    next
  }
}

# Create a container for class +D+ with key +name+:
SortedContainer create container3 -memberClass ::D -key name

# Add a few items
? {container3 new -name victor} "::container3::member1"

? {container3 new -name stefan} "::container3::member2"

? {container3 new -name gustaf} "::container3::member3"

# The method +items+ returns the items in the order of insertion (as before):
? {container3 items} "::container3::member1 ::container3::member2 ::container3::member3"

# The method +index+ returns the items in sorting order (sorted by the +name+ member):
? {container3 index} "::container3::member3 ::container3::member2 ::container3::member1"

# Now we delete an item:
? {container3 delete ::container3::member2} ""

# The item is as well removed from the result lists
? {container3 items} "::container3::member1 ::container3::member3"

? {container3 index} "::container3::member3 ::container3::member1"