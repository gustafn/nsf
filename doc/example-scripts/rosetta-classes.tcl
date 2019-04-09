
# == Rosetta Example: Classes
# For details see https://rosettacode.org/wiki/Classes
#
package req nx
package req nx::test

nx::Class create summation {
  :method init {} {set :v 0}
  :public method add {x} {incr :v $x}
  :public method value {} {return ${:v}}
  :public method destroy {} {puts "ended with value [:value]"; next}
}

# === Demonstrating the behavior in a shell: 

? {set sum [summation new]} "::nsf::__#0"
? {$sum value} 0
? {$sum add 1} 1
? {$sum add 2} 3
? {$sum add 3} 6
? {$sum add 4} 10
? {$sum value} 10

# During the destroy of the object, +ended with value 10+ is printed
? {$sum destroy} ""
