#
# == Design study to show the differences between decorator mixin classes and Ruby's mixin modules
#
# This example shows that the dynamic class structure of NX (and
# XOTcl) is able to support _Ruby style mixins_ (called modules) and
# _decorator style mixins_ (named after the design pattern Decorator)
# in the same script.

package req nx::test
nx::test configure -count 1

#
# One important difference between mixin classes in NX and Ruby's
# mixins is the precedence order. While in NX, mixins are decorators
# (the mixins have higher precedence than the intrinsic classes,
# therefore a mixin class can overload the methods of the current
# class and its subclasses), the mixins of Ruby have a lower
# precedence (they extend the _base behavior_; although Ruby's modules
# are not full classes, they are folded into the _intrinsic class
# hierarchy_). Therefore, a Ruby style mixin can be refined by the
# class, into which it is mixed in (or by a subclass). Decorator style
# mixins modify the behavior of a full intrinsic class tree, while
# Ruby style mixins are compositional units for a single class.
#
# To show the differences, we define the method +module+, which
# behaves somewhat similar to Ruby's +module+ command. This method
# adds the provided class to the precedence order after the current
# class. The easiest way to achieve this is via multiple inheritance
# (i.e. via the +superclass+ relationship).
#
package req nx

nx::Class eval {
  :protected method module {name:class} {
    nsf::relation::set [self] superclass [concat $name [:info superclasses]]
  }
}

#
# For illustration of the behavior of +module+ we define a class
# +Enumerable+ somewhat inspired by the Ruby module with the same
# name. We define here just the methods +map+, +each+, +count+, and
# +count_if+.
#
nx::Class create Enumerable {
  :property members:0..n

  # The method 'each' applies the provided block on every element of
  # 'members'
  :public method each {var block} {
    foreach member ${:members} {
      uplevel [list set $var $member]
      uplevel $block
    }
  }

  # The method 'map' applies the provided block on every element of
  # 'members' and returns a list, where every element is the mapped
  # result of the source.
  :public method map {var block} {
    set result [list]
    :each $var {
      uplevel [list set $var [set $var]]
      lappend result [uplevel $block]
    }
    return $result
  } 
  
  # The method 'count' returns the number of elements.
  :public method count {} {
    return [llength ${:members}]
  }
  
  # The method 'count_if' returns the number of elements for which
  # the provided expression is true.
  :public method count_if {var expr} {
    set result 0
    :each $var {
      incr result [expr $expr]
    }
    return $result
  }
}

# After having defined the class +Enumerable+, we define a class
# +Group+ using +Enumerable+ as a Ruby style mixin. This makes
# essentially +Group+ a subclass of +Enumerable+, but with the only
# difference that +Group+ might have other superclasses as well.
nx::Class create Group {
  #
  # Include the "module" Enumerable
  #
  :module Enumerable
}

# Define now a group +g1+ with the three provided members.
? {Group create g1 -members {mini trix trax}} ::g1

# Since the definition of +Group+ includes the module +Enumerable+,
# this class is listed in the precedence order after the class +Group+:

? {g1 info precedence} "::Group ::Enumerable ::nx::Object"

# Certainly, we can call the methods of +Enumerable+ as usual:
? {g1 count} 3

? {g1 map x {list pre-$x-post}} "pre-mini-post pre-trix-post pre-trax-post"

? {g1 count_if x {[string match tr*x $x] > 0}} 2

#
# To show the difference between a +module+ and a +decorator mixin+ we
# define a class named +Mix+ with the single method +count+, which
# wraps the result of the underlying +count+ method between the
# +alpha+ and +omega+.

nx::Class create Mix {
  :public method count {} {
    return [list alpha [next] omega]
  }
}

# When the mixin class is added to +g1+, it is added to the front of
# the precedence list. A decorator is able to modify the behavior of
# all of the methods of the class, where it is mixed into.

? {g1 object mixins set Mix} "::Mix"

? {g1 info precedence} "::Mix ::Group ::Enumerable ::nx::Object"

? {g1 count} {alpha 3 omega}

# For the time being, remove the mixin class again.
? {g1 object mixins set ""} ""
? {g1 info precedence} "::Group ::Enumerable ::nx::Object"

#
# An important difference between NX/XOTcl style mixins (decorators)
# and Ruby style modules is that the decorator will have always a
# higher precedence than the intrinsic classes, while the +module+ is
# folded into the precedence path.
#

# Define a class +ATeam+ that uses +Enumerable+ in the style of a Ruby
# module. The class might refine some of the provided methods. We
# refined the method +each+, which is used as well by the other
# methods. In general, by defining +each+ one can define very
# different kind of enumerators (for lists, databases, etc.).
#
# Since +Enumerable+ is a module, the definition of +each+ in the
# class +ATeam+ has a higher precedence than the definition in the
# class +Enumerable+. If +Enumerable+ would be a decorator style mixin
# class, it would not e possible to refine the definition in the class
# +ATeam+, but maybe via another mixin class.
#
nx::Class create ATeam {
  #
  # Include the "module" Enumerable
  #
  :module Enumerable

  #
  # Overload "each" 
  #
  :public method each {var block} {
    foreach member ${:members} {
      uplevel [list set $var $member-[string length $member]]
      uplevel $block
    }
  }

  #
  # Use "map", which uses the "each" method defined in this class.
  #
  :public method foo {} {
    return [:map x {string totitle $x}]
  }
}

#
# Define now a team +t1+ with the three provided members. 
#
? {ATeam create t1 -members {arthur bill chuck}} ::t1

# As above, the precedence of +ATeam+ is higher than the precedence of
# +Enumerable+. Therefore, the object +t1+ uses the method +each+ specialized in
# class +ATeam+:
? {t1 info precedence} "::ATeam ::Enumerable ::nx::Object"

? {t1 foo} "Arthur-6 Bill-4 Chuck-5"

#
# The class +ATeam+ can be specialized further by a class +SpecialForce+:
#
nx::Class create SpecialForce -superclass ATeam {
  # ...
}

# Define a special force +s1+ with the four provided members.

? {SpecialForce create s1 -members {Donald Micky Daniel Gustav}} ::s1

# As above, the precedence of +Enumerable+ is lower then the
# precedence of +ATeam+ and +Enumerable+. Therefore, +ATeam+ can refine
# the behavior of +Enumerable+, the class +SpecialForce+ can refine
# the behavior of +ATeam+.
? {s1 info precedence} "::SpecialForce ::ATeam ::Enumerable ::nx::Object"

? {s1 foo} "Donald-6 Micky-5 Daniel-6 Gustav-6"

# Let us look again on decorator style mixin classes. If we add a
# per-class mixin to +ATeam+, the mixin class has highest precedence,
# and decorates the instances of +ATeam+ as well the instances of its
# specializations (like e.g. +SpecialForce+).

? {ATeam mixins set Mix} "::Mix"

? {s1 info precedence} "::Mix ::SpecialForce ::ATeam ::Enumerable ::nx::Object"

? {s1 count} {alpha 4 omega}

# This example showed that NX/XOTcl dynamic class structure is able to
# support Ruby-style mixins, and decorator style mixins in the same script.
