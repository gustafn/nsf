#
# This is an introductory example how to use the nx mongo mapping for
# referencing some object. We use here an example of an Posting having
# a single (possible compound) user as originator. All example work
# the same way as well with with multivalued attributes.
#
# Gustaf Neumann              fecit, May 2011
#
package require nx::mongo

# Establish connection to the database
::nx::mongo::db connect -db "tutorial"

# Make sure, we start always from scratch
nx::mongo::db remove tutorial.users {}
nx::mongo::db remove tutorial.posts {}

######################################################################
# The first approach to implement references simply as an property.
# This is just feasible in cases, where the user has just a name and
# not more attributes.
#
nx::mongo::Class create Post {
  :property title
  :property user
}

Post insert -title "Hello trivial World" -user smith

# Retrieve the entry from the database:
set p [Post find first -cond {title = "Hello trivial World"}]
puts stderr "Name of user: [$p user]\n"

######################################################################
# The second approach to implement references to other objects via an
# property pointing to the object id of an other object. This is the
# classical datbase approach. When the object is fetched, the
# application developer has to care about fetching/dereferencing the
# referenced object.
#
nx::mongo::Class create User {
  :property name
}
nx::mongo::Class create Post {
  :property title
  :property user_id
}

# The method "insert" returns the object id of the newly created
# object. We can use this value as a reference in the Post.
set uid [User insert -name Smith]
Post insert -title "Hello simple World" -user_id $uid

# Retrieve the entry from the database:
set p [Post find first -cond {title = "Hello simple World"}]
set u [User find first -cond [list _id = [$p user_id]]]
puts stderr "Name of user: [$u name]\n"

######################################################################
# The third approach is to embed the object in the referencing
# document. This might be feasible when the values of the embedded
# objects seldomly change, When the containing object (the posting) is
# loaded, the appropriate object structure is created automatically.
#
nx::mongo::Class create User {
  :property name
}
nx::mongo::Class create Post {
  :property title
  :property user:embedded,type=::User
}

Post insert -title "Hello embedded World" -user [User new -name Smith]

# Retrieve the entry from the database:
set p [Post find first -cond {title = "Hello embedded World"}]
puts stderr "Name of user: [[$p user] name]\n"

######################################################################
# The fourth approach is to use mongo db-refs for referencing.  This
# is similar to approach two, but provides support for dereferencing
# and maintaining the reference lists.
#
nx::mongo::Class create User {
  :property name
}
nx::mongo::Class create Post {
  :property title
  :property user:reference,type=::User
}

if {0} {
  #
  # Currently, the mongo c-driver does not allow to add DBRefs, since
  # it refuses to accept field names with leading '$'. So we skip this
  # version for the time being.
  #
  Post insert -title "Hello referenced World" -user [User new -name Smith]

  # Retrieve the entry from the database:
  set p [Post find first -cond {title = "Hello referenced World"}]
  
  puts stderr "Name of user: [[$p user] name]\n"
  
  puts stderr "Content of the collection groups:"
  Post show
}
######################################################################
# Output
######################################################################

# Content of the collection groups:
# {
#     _id: 4daae48056b77f0e00000000, 
#     title: {Hello trivial World}, 
#     user: smith
# }, {
#     _id: 4daae48056b77f0e00000002, 
#     title: {Hello simple World}, 
#     user_id: 4daae48056b77f0e00000001
# }, {
#     _id: 4daae48056b77f0e00000003, 
#     title: {Hello embedded World}, 
#     user: { 
#       name: Smith }
# }, {
#     _id: 4daae48056b77f0e00000005, 
#     title: {Hello referenced World}, 
#     user: { 
#       $ref: users, 
#       $id: 4daae48056b77f0e00000004, 
#       $db: tutorial }
# }
