#
# == Rosetta Example: Delegates
# For details see http://rosettacode.org/wiki/Delegates
#
package req nx
package req nx::test

nx::Class create Delegator {

  # The class Delegator has a property named "delegatee" which is an
  # object:

  :property delegatee:object

  # The method "operation" decides, whether it deletates the action to
  # another object, or it performs the action itself.

  :public method operation {} {
    if {[info exists :delegatee]} {
      ${:delegatee} operation
    } else {
      return "default implementatiton"
    }
  }
}

nx::Class create Delegatee {

  # The class "Delgatee" might receice invocations from the class
  # "Delegator"
  
  :public method operation {} {
    return "delegatee implementatiton"
  }
}

# === Demonstrating the behavior in a shell: 
#
# Create a +Delegator+, which has no +delegatee+ defined. Therefore
# delegator performs the action by itself, the default implementation.
#
? {set a [Delegator new]} "::nsf::__#0"
? {$a operation} "default implementatiton"

#
# Now, we set the +delegatee+; therefore, the delegatee will perform
# the action.
#
? {$a configure -delegatee [Delegatee new]} ""
? {$a operation} "delegatee implementatiton"