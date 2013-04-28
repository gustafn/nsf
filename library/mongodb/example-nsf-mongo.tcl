package require nx
package require nsf::mongo

#nsf::configure debug 2
#
# One might query these values from the mongo shell via:
#
#    mongo
#    > use tutorial
#    > db.persons.find();
#

#set mongoConn [::mongo::connect -server 127.0.0.1:27017]
set mongoConn [::mongo::connect]
puts "Connection: $mongoConn"

if {1} {
  ::mongo::remove $mongoConn tutorial.persons {}
  # Drop old potenially old collection and 
  # recreate it as a capped collection
  ::mongo::run $mongoConn tutorial {drop string persons}
  puts "\nCreate a capped collection: [::mongo::run $mongoConn tutorial {
    create string persons
    capped bool 1 
    size int 100000
  }]"

  puts "\nInserting a few tuples"
  if {[catch {
    ::mongo::insert $mongoConn tutorial.persons [list name string Joe projects string abc age int 23 \
						     classes array {0 object {$ref string courses $id oid 1}}]
  }]} {puts "!!! cannot insert dbref;\
	most likely, the c-driver does not support yet insertion of \$ref, \$id and \$db fields"}
  ::mongo::insert $mongoConn tutorial.persons [list name string Gustaf projects string nsf age int 53]
  ::mongo::insert $mongoConn tutorial.persons [list name string Stefan projects string nsf]
  ::mongo::insert $mongoConn tutorial.persons [list name string Franz info object {x int 203 y int 102} age int 29 projects string gtat]
  ::mongo::insert $mongoConn tutorial.persons [list name string Victor a array {0 string "x" 1 string "y"} age int 31]
  ::mongo::insert $mongoConn tutorial.persons [list name string Selim ts timestamp {1302945037 1} d date 1302947619279]

  puts stderr "\nCreate an index on name (ascending)"
  ::mongo::index $mongoConn tutorial.persons [list name int 1]
}

puts stderr "\nFull content"
puts [join [::mongo::query $mongoConn tutorial.persons {}] \n]

puts stderr "\nProject members of nsf sorted by name"
puts  [join [::mongo::query $mongoConn tutorial.persons [list \$query object {projects string nsf} \$orderby object {name int 1}]] \n]

puts stderr "\nAge > 30 (all atts)"
puts  [join [::mongo::query $mongoConn tutorial.persons [list \$query object {age object {$gt int 30}}]] \n]

puts stderr "\nAge > 30 (only atts name and age)"
puts  [join [::mongo::query $mongoConn tutorial.persons [list \$query object {age object {$gt int 30}}] \
		 -atts {name int 1 age int 1}] \n]

puts stderr "\nCount Age > 30"
puts  [::mongo::count $mongoConn tutorial.persons {age object {$gt int 30}}]

puts stderr "\nAge > 30 (all atts, via cursor interface)"
set cursor [::mongo::cursor::find $mongoConn tutorial.persons [list \$query object {age object {$gt int 30}}]]
puts "Cursor: $cursor"
puts NEXT=[::mongo::cursor::next $cursor]
puts NEXT=[::mongo::cursor::next $cursor]
puts NEXT=[::mongo::cursor::next $cursor]
::mongo::cursor::close $cursor

puts stderr "\nAge > 30 (all atts, via cursor interface, tailable)"
set cursor [::mongo::cursor::find $mongoConn tutorial.persons [list \$query object {age object {$gt int 30}}] -tailable]
puts "Cursor: $cursor"
if {$cursor ne ""} {
  while {1} {
    set result [::mongo::cursor::next $cursor]
    if {$result eq ""} break
    puts NEXT=$result
  }
  ::mongo::cursor::close $cursor
}

puts stderr "\nempty result (via cursor interface)"
set cursor [::mongo::cursor::find $mongoConn tutorial.persons [list \$query object {age object {$gt int 300}}]]
puts "Cursor: $cursor"
if {$cursor ne ""} {
  while {1} {
    set result [::mongo::cursor::next $cursor]
    if {$result eq ""} break
    puts NEXT=$result
  }
  ::mongo::cursor::close $cursor
}

puts stderr "\nArray 'a' contains 'x'"
puts  [join [::mongo::query $mongoConn tutorial.persons [list \$query object {a string "x"}]] \n]

puts stderr "\nEmbedded object has some value (info.y > 100)"
puts [join [::mongo::query $mongoConn tutorial.persons [list \$query object {info.y object {$gt int 100}}]] \n]

puts stderr "\nProjects in {nsf gtat}"
puts [join [::mongo::query $mongoConn tutorial.persons [list \$query object {projects object {$in array {0 string nsf 1 string gtat}}}]] \n]

if {0} {
  puts stderr "\nDelete members of project abc"
  ::mongo::remove $mongoConn tutorial.persons [list projects string abc]
}

if {0} {
  puts stderr "\nDelete all from tutorial.persons"
  ::mongo::remove $mongoConn tutorial.persons {}
}

::mongo::close $mongoConn
