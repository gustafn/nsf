#
# The "Business Insider" Data Model
#
# { title: 'Too Big to Fail', 
#   author: 'John S.',
#   ts: Date('05-Nov-09 10:33'),
#   [ comments: [ {author: 'Ian White',
#                  comment: 'Great Article!' },
#                 {author: 'Joe Smith',
#                  comment: 'But how fast is it?',
#                  replies: [ {author: Jane Smith',
#                              comment: 'scalable?' }]
#                 }
#   ],
#   tags: ['finance', 'economy']
# }

#
# This is an example how to use the nx mongo mapping.
#
# Gustaf Neumann              fecit, April 2011
#
package require nx::mongo
package require nx::serializer
package require nx::test


# TODO: 
#   - make embedded spec nicer
#   - handle fetch of embedded
#   - handle count() like find()


# Establish connection to the database
::nx::mongo::db connect

# Make sure, we start always from scratch
nx::mongo::db remove tutorial.bi {}

#
# Create the application classes
#
nx::mongo::Class create Comment {
  #:document "tutorial.bi"

  :attribute author:required
  :attribute comment:required 
  :attribute replies:embedded,arg=::Comment,0..n {set :incremental 1}
}

nx::mongo::Class create Posting {
  :document "tutorial.bi"
  :index ts
  :attribute title:required
  :attribute author:required
  :attribute ts:required
  :attribute comments:embedded,arg=::Comment,0..n {set :incremental 1}
  :attribute tags:0..n {set :incremental 1}
}
#puts stderr [Posting serialize]

set p [Posting new -title "Too Big to Fail" -author "John S." -ts "05-Nov-09 10:33" -tags {finance economy} \
	   -comments [list \
			  [Comment new -author "Ian White" -comment "Great Article!"] \
			  [Comment new -author "Joe Smith" -comment "But how fast is it?" \
			       -replies [list [Comment new -author "Jane Smith" -comment "scalable?"]]] \
			 ]]
#
# When we save the item, the embedded objects (the comments and
# replies) are saved together with the entry.
#
puts stderr ====
$p save
puts stderr ====

# After saving the item, the main object contains an _id, such that a
# subsequent save operation does not create an additional item. For
# our little experiment here, we like to save multiple copies to see
# the results of our changes, and we remove the _id manually
$p eval {unset :_id}

# Now we want to remove e.g. the second comment (with the embedded
# replies). First get the corresponding object $c ...
set c [lindex [$p comments] 1]
# ... and delete it
$c delete

# The delete operation on an embedded object removes it from the
# object lists, but the change is not automatically persisted, since
# there might be multiple changes in a complex document. Therefore we
# have to perform an save operation of the containing document.
$p save

# Now, we have two postings in the database, the first with the two
# comments, the second one with just a single comment.  
? {nx::mongo::db count tutorial.bi {}} 2

# Again, we want to continue with our test and remove the fresh _id as
# well.
$p eval {unset :_id}

# Add an additional comment at the end of the list of the comments....
$p comments add [Comment new -author "Gustaf N" -comment "This sounds pretty cool"] end
# ... and add another tag ...
$p tags add nx
# ... and save it
$p save
? {nx::mongo::db count tutorial.bi {}} 3

puts stderr ====EXIT