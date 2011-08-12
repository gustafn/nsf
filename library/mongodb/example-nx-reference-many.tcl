#
# This is an introductory example how to use the nx mongo mapping for
# referencing some objects. We use here an example of a Group having
# a (possible compound) users as members.
#
# Gustaf Neumann              fecit, May 2011
#
package require nx::mongo

# Establish connection to the database
::nx::mongo::db connect -db "tutorial"

# Make sure, we start always from scratch
nx::mongo::db remove tutorial.groups {}
nx::mongo::db remove tutorial.members {}

######################################################################
# The first approach to implement references simply as multivalued
# attributes. This is just feasible in cases, where the user has just
# a name and not more attributes.
#
nx::mongo::Class create Group {
  :property name
  :property members:0..n
}

Group insert -name "grp1" -members {gustaf stefan}

# Retrieve the entry from the database:
set g [Group find first -cond {name = "grp1"}]
puts stderr "Members of group: [$g members]\n"

######################################################################
# The second approach to implement references to other objects via an
# property pointing to the object ids of other objects. This is
# similar to the classical datbase approach. When the object is
# fetched, the application developer has to care about
# fetching/dereferencing the referenced objects.
#
nx::mongo::Class create Member {
  :property name
}
nx::mongo::Class create Group {
  :property name
  :property members:0..n
}

set id1 [Member insert -name gustaf] 
set id2 [Member insert -name stefan]
Group insert -name "grp2" -members [list $id1 $id2]

# Retrieve the entry from the database:
set g [Group find first -cond {name = "grp2"}]
set members [list]
foreach m [Member find all -cond [list _id in [$g members]]] {
  lappend members $m
}
puts stderr "Members of group [$g name]:"
foreach m $members {puts stderr "\t[$m name]"}
puts stderr ""

######################################################################
# The third approach is to embed the objects in the referencing
# document. This might be feasible when the values of the embedded
# objects seldomly change, When the containing object (the posting) is
# loaded, the appropriate object structure is created automatically.
#
nx::mongo::Class create Member {
  :property name
}
nx::mongo::Class create Group {
  :property name
  :property members:embedded,type=::Member,0..n
}

Group insert -name "grp3" \
    -members [list \
		  [Member new -name gustaf] \
		  [Member new -name stefan]]

# Retrieve the entry from the database:
set g [Group find first -cond {name = "grp3"}]

puts stderr "Members of group [$g name]:"
foreach m [$g members] {puts stderr "\t[$m name]"}
puts stderr ""


######################################################################
# The fourth approach is to use mongo db-refs for referencing.  This
# is similar to approach two, but provides support for dereferencing
# and maintaining the reference lists.
#
nx::mongo::Class create Member {
  :property name
}
nx::mongo::Class create Group {
  :property name
  :property members:reference,type=::Member,0..n
}

Group insert -name "grp4" \
    -members [list \
		  [Member new -name gustaf] \
		  [Member new -name stefan]]

# Retrieve the entry from the database:
set g [Group find first -cond {name = "grp4"}]

puts stderr "Members of group [$g name]:"
foreach m [$g members] {puts stderr "\t[$m name]"}
puts stderr ""

puts stderr "Content of collection groups:"
Group show

######################################################################
# Output
######################################################################
# Content of collection groups:
# {
#     _id: 4daae3e492b5570e00000000, 
#     name: grp1, 
#     members: [ gustaf, stefan ]
# }, {
#     _id: 4daae3e492b5570e00000003, 
#     name: grp2, 
#     members: [ 4daae3e492b5570e00000001, 4daae3e492b5570e00000002 ]
# }, {
#     _id: 4daae3e492b5570e00000004, 
#     name: grp3, 
#     members: [ { 
#       name: gustaf }, { 
#       name: stefan } ]
# }, {
#     _id: 4daae3e492b5570e00000007, 
#     name: grp4, 
#     members: [ { 
#       $ref: members, 
#       $id: 4daae3e492b5570e00000005, 
#       $db: tutorial }, { 
#       $ref: members, 
#       $id: 4daae3e492b5570e00000006, 
#       $db: tutorial } ]
# }


