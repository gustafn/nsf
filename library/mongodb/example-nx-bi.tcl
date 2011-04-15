# An example for the usage of the nx mongo object mapping based on the
# the real world 
#
#      "Business Insider" Data Model 
#
# (see e.g.
# http://www.slideshare.net/mongodb/nosql-the-shift-to-a-nonrelational-world).
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
#
#
# Gustaf Neumann              fecit, May 2011
#
package require nx::mongo
package require nx::serializer
package require nx::test

# Establish connection to the database
::nx::mongo::db connect

# Make sure, we start always from scratch, so remove everything from
# the collection.
nx::mongo::db remove tutorial.bi {}

#
# Create the application classes based on the "Business Insider" data
# model. Note that instances of the class "Comment" can be embedded in
# a posting (attribute "comments") as well as in an comment itself
# (attribute "replies"). All comments are in this example multivalued
# and incremental (i.e. one can use slot methods "... add ..." and
# "... delete ..." to add values to the attributes).
#
nx::mongo::Class create Comment {
  # This class does not have a :document" spec, since it is embedded.

  :attribute author:required
  :attribute comment:required 
  :attribute replies:embedded,arg=::Comment,0..n {set :incremental 1}
}

nx::mongo::Class create Posting {
  :document "tutorial.bi"
  :index tags
  :attribute title:required
  :attribute author:required
  :attribute ts:required
  :attribute comments:embedded,arg=::Comment,0..n {set :incremental 1}
  :attribute tags:0..n {set :incremental 1}
}

#
# Build a composite Posting instance based on the example above.
#
set p [Posting new -title "Too Big to Fail" -author "John S." \
	   -ts "05-Nov-09 10:33" -tags {finance economy} \
	   -comments [list \
			  [Comment new -author "Ian White" -comment "Great Article!"] \
			  [Comment new -author "Joe Smith" -comment "But how fast is it?" \
			       -replies [list [Comment new -author "Jane Smith" -comment "scalable?"]]] \
			 ]]
#
# When we save the item, the embedded objects (the comments and
# replies) are saved together with the posting in a compound document.
#
$p save

# After saving the item, the main object contains an _id, such that a
# subsequent save operations do not create an additional entries in
# the database. For our little experiment here, we like to save
# multiple copies to see the results of our changes. Therefore we
# remove the _id manually:
$p eval {unset :_id}

# We have two comments for the posting $p
? [list llength [$p comments]] 2

# Now we want to remove e.g. the second comment (with the embedded
# replies). First get this comment object $c ...
set c [lindex [$p comments] 1]

# ... and delete it
$c delete

# The delete operation destroy the embedded object and removes the
# referece to it in the comments attribute.
? [list llength [$p comments]] 1

# The delete operation does not automatically persist the change,
# since there might be multiple changes in a complex
# document. Therefore we have to perform an save operation of the
# containing document.
$p save

# Now, we have two postings in the database, the first with the two
# comments, the second one with just a single comment.
? {Posting count} 2

# Again, we want to continue with our test and remove the fresh _id as
# well.
$p eval {unset :_id}

# We add an additional comment at the end of the list of the comments
# with the incremental operations (the slot is incremental) ...
$p comments add [Comment new -author "Gustaf N" -comment "This sounds pretty cool"] end

# ... and we add another tag ...
$p tags add nx

# ... and save everything
$p save

# We have now three entries in the database collection.
? {Posting count} 3

# Now fetch the first entry with the tag "nx"
set q [Posting find first -cond {tags = nx}]

# The fetched entry should have the two comments:
? [list llength [$q comments]] 2

# We add jet another tag and save it
$q tags add nsf
$q save

# We still have three entries in the database
? {Posting count} 3

puts stderr [$q bson pp [$q eval {:bson encode}]]

puts stderr ====EXIT