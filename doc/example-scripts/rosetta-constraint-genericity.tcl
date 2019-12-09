#
# == Rosetta Example: Constrained genericity 
# For details see https://rosettacode.org/wiki/Constrained_genericity
#
package req nx
package req nx::test

#
# Define the two classes +Eatable+ and +Fish+. +Eatable+ is a class
# for all eatable things, a +Fish+ is a subclass ant therefore
# eatable.
#
nx::Class create Eatable
nx::Class create Fish -superclass Eatable {
  :property name
}

#
# A +FoodBax+ may only contain eatable items. Therefore, with we define
# +items+ as a property of type +Eatable" which has a multiplicity of
# +0..n+ (might contain 0 to n eatable items). Furthermore, we define
# items as +incremental+, such we can add / remove items with +item
# add+ or +item remove+.
#
nx::Class create FoodBox {
  :property -incremental item:object,type=::Eatable
  :public method print {} {
    set string "Foodbox contains:\n"
    foreach i ${:item} {append string "   [$i cget -name]\n"}
    return $string
  }
}

# === Demonstrating the behavior in a shell: 
# Create two fishes, Wanda and Nemo:
? {set f1 [Fish new -name "Wanda"]} "::nsf::__#0"
? {set f2 [Fish new -name "Nemo"]} "::nsf::__#1"

# Create a Foodbox and add the two fishes:
? {set fb [FoodBox new]} "::nsf::__#2"
? {$fb item add $f1} "::nsf::__#0"
? {$fb item add $f2} "::nsf::__#1 ::nsf::__#0"

# Return the print string of the contents:
? {$fb print} {Foodbox contains:
   Nemo
   Wanda
}

