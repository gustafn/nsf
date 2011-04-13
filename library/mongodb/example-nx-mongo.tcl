#
# This is an example how to use the nx mongo mapping.
#
# Gustaf Neumann              fecit, April 2011
#
package require nx::mongo

# Establish connection to the database
::nx::mongo::db connect

# Make sure, we start always from scratch
nx::mongo::db remove tutorial.persons {}

#
# Create the application class "Person"
#
nx::mongo::Class create Person {
  :document "tutorial.persons"
  :index name

  :attribute name:required
  :attribute projects:0..n {set :incremental 1}
  :attribute age:integer
}

#
# Insert a tuple to the database via creating an object, saving and
# destroying it:
#
set p [Person new -name Gustaf -projects nsf -age 53]
$p save; $p destroy

#
# The insert operation of above can be achieved with less typing via
# the conveniance method "insert":
#
Person insert -name Stefan -projects nsf
Person insert -name Joe -projects abc -age 23
Person insert -name Franz -projects {gtat annobackend abc} -age 29

#
# Lookup a single Person, create an instance of the object ...
#
set p [Person find first -cond {name = Gustaf}]
#puts [$p serialize]
#
# ... change the age, add an project, and save it.
#
$p age 55
$p projects add xowiki
$p save; $p destroy

#
# Lookup a single Person and create a named object
#
Person find first -instance p2 -cond {name = Gustaf}
#puts [p2 serialize]

#
# Test a few queries based on a user-friendly query language
#
puts "\nProject members of nsf:"
foreach p [Person find all -cond {projects = nsf}] {
  puts "\t$p:\t[$p name]"
}

puts "\nAll Persons sorted by name (ascending):"
foreach p [Person find all -orderby name] {
  puts "\t$p:\t[$p name]"
}

puts "\nMembers of Projects != 'abc' nsf sorted by name desc and age:"
foreach p [Person find all -cond {projects != "abc"} -orderby {{name desc} age}] {
  puts "\t$p:\t[$p name]"
}

puts "\nFind persons age > 30:"
foreach p [Person find all -cond {age > 30}] {
  puts "\t$p:\t[$p name]"
}

#
# Define a special find method for "Person" named "oldies"
#
Person public class method "find oldies" {} {
  return [:find all -cond {age > 30}]
}

#
# Use the special find method
#
puts "\nFind oldies:"
foreach p [Person find oldies] {
  puts "\t$p:\t[$p name]"
}

