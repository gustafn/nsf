#
# This example is a straight translation of the OTcl Tutorial
# http://www.isi.edu/nsnam/otcl/doc/tutorial.html to NX. It serves as
# a very short intro to the basic elements of scripting with NX and
# provides a comparison study to OTcl.
#

package req nx
package req nx::test
nx::test configure -count 1

# Suppose we need to work with many bagels in our application. We
# might start by creating a Bagel class.

? {nx::Class create Bagel} ::Bagel

# We can now create bagels and keep track of them using the info
# method.

? {Bagel create abagel} ::abagel

? {abagel info class} ::Bagel

? {Bagel info instances} ::abagel

# Of course, bagels don't do much yet. They should remember whether
# they've been toasted. We can create and access an instance variable
# by defining an property for the class. All instance variables are
# per default public in the sense of C++.
? {Bagel property {toasted 0}} ""

# Since abagel was created before the definition of the property we
# have to set the default value for it using the setter method. Again,
# the info method helps us keep track of things.

? {abagel cget -toasted} 0

? {abagel info vars} toasted

# But we really want them to begin in an untoasted state to start
# with.

? {Bagel create bagel2} ::bagel2

? {bagel2 info vars} toasted

? {bagel2 cget -toasted} 0

#
# Our bagels now remember whether they've been toasted. Let is
# recreate the first one.

? {lsort [Bagel info instances]} {::abagel ::bagel2}

? {::abagel destroy} ""

? {Bagel info instances} ::bagel2

? {Bagel create abagel} ::abagel

# Now we're ready to add a method to bagels so that we can toast
# them. Methods have an argument list and body like regular
# Tcl procs. Here's the toast method.

? {Bagel public method toast {} {
      if {[incr :toasted] > 1} then { 
        error "something's burning!"
      }
}} "::nsf::classes::Bagel::toast"

# The defined methods can be queried with info. We see as well the
# setter method for the variable toasted.

? {Bagel info methods} {toast}

# Aside from setting the toasted variable, the body of the toast
# method demonstrates how to access instance variables by using a
# leading colon in the name. 

# We invoke the toast method on bagels in the same way we use the
# info and destroy methods that were provided by the system. That
# is, there is no distinction between user and system methods.

? {abagel toast} ""
? {abagel toast} "something's burning!"

# Now we can add spreads to the bagels and start tasting them. If we
# have bagels that aren't topped, as well as bagels that are, we may
# want to make toppable bagels a separate class. Let explore
# inheritance with these two classes, starting by making a new class
# SpreadableBagel that inherits from Bagel. A SpreadableBagel has an
# property toppings which might have multiple values. Initially,
# toppings are empty.

? {nx::Class create SpreadableBagel -superclass Bagel {
    :property -incremental {toppings:0..n ""}
}} ::SpreadableBagel

? {SpreadableBagel info superclass} ::Bagel

? {SpreadableBagel info heritage} {::Bagel ::nx::Object}

# Let's add a taste method to bagels, splitting its functionality
# between the two classes and combining it with next.

? {Bagel public method taste {} {
    if {${:toasted} == 0} then {
    return raw!
  } elseif {${:toasted} == 1} then {
    return toasty
  } else {
    return burnt!
  }
}} "::nsf::classes::Bagel::taste"

? {SpreadableBagel public method taste {} {
    set t [next]
    foreach i ${:toppings} {
       lappend t $i
    }
    return $t
}} "::nsf::classes::SpreadableBagel::taste"

? {SpreadableBagel create abagel} ::abagel

? {abagel toast} ""
? {abagel toppings add jam} jam

? {abagel taste} "toasty jam"

# Of course, along come sesame, onion, poppy, and a host of other
# bagels, requiring us to expand our scheme. We could keep track of
# flavor with an instance variable, but this may not be
# appropriate. Flavor is an innate property of the bagels, and one
# that can affect other behavior - you wouldn't put jam on an onion
# bagel, would you? Instead of making a class hierarchy, let's use
# multiple inheritance to make the flavor classes mixins that add a
# their taste independent trait to bagels or whatever other food they
# are mixed with.

? {nx::Class create Sesame {
    :public method taste {} {concat [next] "sesame"}
}} ::Sesame

? {nx::Class create Onion {
    :public method taste {} {concat [next] "onion"}
}} "::Onion"

? {nx::Class create Poppy {
    :public method taste {} {concat [next] "poppy"}
}} "::Poppy"

# Well, they don't appear to do much, but the use of next allows them
# to be freely mixed.

? {nx::Class create SesameOnionBagel -superclass SpreadableBagel -mixin {Sesame Onion}} ::SesameOnionBagel

? {SesameOnionBagel create abagel -toppings butter} "::abagel"

? {abagel taste} "raw! butter onion sesame"

# For multiple inheritance, the system determines a linear inheritance
# ordering that respects all of the local superclass orderings. You
# can examine this ordering with an info option. next follows this
# ordering when it combines behavior.

? {SesameOnionBagel info heritage} {::Sesame ::Onion ::SpreadableBagel ::Bagel ::nx::Object}

? {abagel info precedence} {::Sesame ::Onion ::SesameOnionBagel ::SpreadableBagel ::Bagel ::nx::Object}

# We can also combine our mixins with other classes, classes that need
# have nothing to do with bagels, leading to a family of chips.

? {nx::Class create Chips {
    :public method taste {} {return "crunchy"}
}} "::Chips"

? {nx::Class create OnionChips -superclass Chips -mixin Onion} ::OnionChips

? {OnionChips create abag} ::abag

? {abag taste} "crunchy onion"

