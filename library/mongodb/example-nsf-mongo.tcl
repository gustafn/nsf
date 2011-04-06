package require nx
package require nsf::mongo

#
# One might query these values from the mongo shell via:
#
#    mongo
#    > use tutorial
#    > db.persons.find();
#

#set mongoConn [::mongo::connect -port 27017]
set mongoConn [::mongo::connect]

if {1} {
  ::mongo::remove $mongoConn tutorial.persons {}

  puts stderr "\nInserting a few tuples"
  ::mongo::insert $mongoConn tutorial.persons [list name string Joe projects string abc age int 23]
  ::mongo::insert $mongoConn tutorial.persons [list name string Gustaf projects string nsf age int 53]
  ::mongo::insert $mongoConn tutorial.persons [list name string Stefan projects string nsf]
  ::mongo::insert $mongoConn tutorial.persons [list name string Franz info object {x int 203 y int 102} age int 29]
  ::mongo::insert $mongoConn tutorial.persons [list name string Victor a array {0 string "x" 1 string "y"} age int 31]

  puts stderr "\nCreate an index on name (ascending)"
  ::mongo::index $mongoConn tutorial.persons [list name int 1]
}

puts stderr "\nFull content"
puts [join [::mongo::query $mongoConn tutorial.persons {}] \n]

puts stderr "\nProject members of nsf sorted by name"
puts  [join [::mongo::query $mongoConn tutorial.persons [list \$query object {projects string nsf} \$orderby object {name int 1}]] \n]

puts stderr "\nAge > 30"
puts  [join [::mongo::query $mongoConn tutorial.persons [list \$query object {age object {$gt int 30}}]] \n]

puts stderr "\nArray 'a' contains 'x'"
puts  [join [::mongo::query $mongoConn tutorial.persons [list \$query object {a string "x"}]] \n]

puts stderr "\nEmbedded object has some value (info.y > 100)"
puts  [join [::mongo::query $mongoConn tutorial.persons [list \$query object {info.y object {$gt int 100}}]] \n]

if {0} {
  puts stderr "\nDelete members of project abc"
  ::mongo::remove $mongoConn tutorial.persons [list projects string abc]
}

if {0} {
  puts stderr "\nDelete all from tutorial.persons"
  ::mongo::remove $mongoConn tutorial.persons {}
}

::mongo::close $mongoConn
