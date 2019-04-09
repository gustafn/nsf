#
# == Rosetta Example: Respond to an unknown method call 
# For details see https://rosettacode.org/wiki/Respond_to_an_unknown_method_call
#
package req nx
package req nx::test

#
# Define a class Example modelled after the 
# Python version of Rosetta:
#

nx::Class create Example {
  
  :public method foo {} {return "This is foo."}
  :public method bar {} {return "This is bar."}

  :method unknown {method args} {
    set result "Tried to handle unknown method '$method'."
    if {[llength $args] > 0} {
      append result " It had arguments '$args'."
    }
    return $result
  }
}

# === Demonstrating the behavior in a shell: 
#
# Create an instance of the class Example:
? {set e [Example new]} "::nsf::__#0"

? {$e foo} "This is foo."

? {$e bar} "This is bar."

? {$e grill} "Tried to handle unknown method 'grill'."

? {$e ding dong} "Tried to handle unknown method 'ding'. It had arguments 'dong'."
