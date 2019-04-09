
# == Rosetta Example: Polymorphism
# For details see https://rosettacode.org/wiki/Polymorphism
#
package req nx
package req nx::test

nx::Class create Point {

  :property x:double
  :property y:double

  :public method print {} {
    return "Point(${:x},${:y})"
  }
}

nx::Class create Circle -superclass Point {

  :property radius:double

  :public method print {} {
    return "Circle(${:x},${:y},${:radius})"
  }
}

# === Demonstrating the behavior in a shell: 

# Create a point and get the print string:
? {set p [Point new -x 1.0 -y 2.0]} "::nsf::__#0"
? {$p print} "Point(1.0,2.0)"

# Get the x coordinate of this point:
? {$p cget -x} "1.0"

# Create a circle:
? {set c [Circle new -x 3.0 -y 4.0 -radius 5.0]} "::nsf::__#1"
# Copy the circle
? {set d [$c copy]} "::nsf::__#3"

# Change the radius of the copied circle:
? {$d configure -radius 1.5} ""

# Print the two circles:
? {$c print} "Circle(3.0,4.0,5.0)"

? {$d print} "Circle(3.0,4.0,1.5)"
