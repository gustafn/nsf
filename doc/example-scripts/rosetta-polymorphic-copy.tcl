#
# == Rosetta Example: Polymorphic copy
# For details see http://rosettacode.org/wiki/Polymorphic_copy
#
package req nx
package req nx::test

nx::Class create T {
  :public method name {} {return T}
}
nx::Class create S -superclass T {
  :public method name {} {return S}
}

# === Demonstrating the behavior in a shell: 
#
# +o1+ and +o2+ are instances of +T+ and +S+. +$o1 name+ returns the same value as its copy, same for +$o2+

? {set o1 [T new]} "::nsf::__#0"
? {set o2 [S new]} "::nsf::__#1"

? {$o1 name} "T"
? {$o2 name} "S"

? {[$o1 copy] name} "T"
? {[$o2 copy] name} "S"