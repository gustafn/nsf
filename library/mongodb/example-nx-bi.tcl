#
# The Business Insider Data Model
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

# Establish connection to the database
::nx::mongo::db connect

# Make sure, we start always from scratch
nx::mongo::db remove tutorial.bi {}

#
# Create the application classes
#

# TODO: 
#   - make embedded spec nicer
#   - handle delete of mebedded obj
#   - 


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
  :attribute comments:embedded,arg=::Comment,0..n {
    #set :incremental 1
  }
  :attribute tags:0..n
}
#puts stderr [Posting serialize]

set p [Posting new -title "Too Big to Fail" -author "John S." -ts "05-Nov-09 10:33" -tags {finance economy} \
	   -comments [list \
			  [Comment new -author "Ian White" -comment "Great Article!"] \
			  [Comment new -author "Joe Smith" -comment "But how fast is it?" \
			       -replies [list [Comment new -author "Jane Smith" -comment "scalable?"]]] \
			 ]]

#puts [$p serialize]

puts stderr ====
$p save
puts stderr ====EXIT