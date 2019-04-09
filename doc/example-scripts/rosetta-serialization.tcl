
# == Rosetta Example: Object serialization
# For details see https://rosettacode.org/wiki/Object_serialization
#
package req nx
package req nx::test
proc ! args {uplevel {*}$args}
package req nx::serializer

nx::Class create Being {
  :property {alive:boolean true}
}

nx::Class create Animal -superclass Being {
  :property name
  :public method print {} {
    puts "i am ${:name} alive ${:alive}"
  }
}

# === Demonstrating the behavior in a shell: 
# Create a few animals
? {Animal new -name "Fido"} "::nsf::__#0"
? {Animal new -name "Lupo"} "::nsf::__#1"
? {Animal new -name "Kiki" -alive false} "::nsf::__#2"

# Print the created animals
? {foreach i [Animal info instances] { $i print }} ""

# The loop prints: +
#    +i am Kiki alive false+ +
#    +i am Lupo alive true+ +
#    +i am Fido alive true+ 
#
# Serialize the animals to a file
! {set fpath [::nsf::tmpdir]/dump}
! {set f [open $fpath w]}
? {foreach i [Animal info instances] { puts $f [$i serialize] }} ""
? {close $f} ""

# Destroy all animal instances:
? {foreach i [Animal info instances] { $i destroy }} ""
? {puts ===========} ""

# Print the existing animals (will print nothing)
? {foreach i [Animal info instances] { $i print }} ""
? {puts ===========} ""

# Load the animals again ...
? {source $fpath} ""

# and print it. The print output is the same as above
? {foreach i [Animal info instances] { $i print }} ""
