package require nx
package require nsf::mongo

#
# This sample script shows some basic interactions from the nsf mongo
# interface with gridFS. It connects to mongo, opens a GridFS named
# "myfs" and inserts a file into the file systems.  Run the script
# with the current directory of nsfmongo, such it can find the README
# file.
#
# After running the script, one can use the following command to
# inspect the content in the GridFS via the mongo shell
#
#    $ mongo
#    > use myfs
#    > show collections
#    > db.fs.files.find()
#
# or via the mongofiles interface:
#
#    $ mongofiles -d myfs list
#

#
# First, as usual, open the connection to the mongo db
#
set mongoConn [::mongo::connect]

#
# Open a GridFS in the mongo datbase "myfs" and use the usual prefix
# "fs", such GridFS names the collections "fs.chunks" and "fs.files".
#
set gridFS [::mongo::gridfs::open $mongoConn myfs fs]

#
# gridfs::remove_file removes all files with the specified name
# multiple store operations create "revisions" with different uploadDates
::mongo::gridfs::remove_file $gridFS README

#
# ::mongo::gridfs::store_file $gridFS $inputFileName $storeFileName text/plain
# Note that the input file name can be "-" for reading from stdin.
#
# The current version of gridfs_store_file() is quite unfriendly,
# since it assumes that the file exists, and aborts otherwise. So, we
# perform the existence test here. 
#
# Store a known file:
#
set fn README
if {[file readable $fn]} {
  set r [::mongo::gridfs::store_file $gridFS $fn $fn text/plain]
  puts stderr "::mongo::gridfs::store_file returned $r"
} else {
  puts stderr "no such file: $fn"
}

#
# Open a grid file, get some of its properties, and read it in chunks
# of 500 bytes, and close it finally.
#
set f [mongo::gridfile::open $gridFS README]
puts stderr "\nOpened grid file '$f'"
puts stderr "Metadata: [mongo::gridfile::get_metadata $f]"
puts stderr "ContentLength: [mongo::gridfile::get_contentlength $f]"
puts stderr "ContentType: [mongo::gridfile::get_contenttype $f]"
while {1} {
  set chunk [mongo::gridfile::read $f 500]
  puts stderr "read chunk-len [string length $chunk] content [string range $chunk 0 10]..."
  if {[string length $chunk] < 500} {
    break
  }
}
mongo::gridfile::close $f

#
# Access the files stored in the gridfs via plain query interface
#
puts "\nAll Files:\n[join [::mongo::query $mongoConn myfs.fs.files {}] \n]\n"

#
# Get the file named README from the gridfs via plain query interface
#
set atts [lindex [::mongo::query $mongoConn myfs.fs.files \
		      [list \$query object {filename string README}] \
		      -limit 1] 0]
puts "Attributes of file README:\n$atts\n"

#
# Extract the oid from the bson attributes
#
foreach {name type value} $atts {
  if {$name eq "_id"} {
    set oid $value
    break
  }
}

#
# Add a dc:creator to the bson attributes ...
#
lappend atts metadata object {dc:creator string "Gustaf Neumann"}
# .. and update the entry in the gridfs
::mongo::update $mongoConn myfs.fs.files [list _id oid $oid] $atts

#
# Now we can use the gridfs interface to obtain the additional
# metadata as well
#
set f [mongo::gridfile::open $gridFS README]
puts stderr "Metadata: [mongo::gridfile::get_metadata $f]"
mongo::gridfile::close $f

#
# close everything
#
::mongo::gridfs::close $gridFS
::mongo::close $mongoConn
