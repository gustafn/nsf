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
#
# or via the mongofiles interface:
#
#    $ mongofiles -d myfs list
#

set mongoConn [::mongo::connect]

#
# Open a GridFS in the mongo datbase "myfs" and use the usual prefix
# "fs", such GridFS names the collections "fs.chunks" and "fs.files".
#
set gridFS [::mongo::gridfs::open $mongoConn myfs fs]

#
# The current version of gridfs_store_file() is quite unfriendly,
# since it assumes that the file exists, and aborts otherwise. So, we
# perform the existance test here. 
#
# Note that the input file name can be "-" for reading from stdin.
#
# Store a known file:
#
set fn README
if {[file readable $fn]} {
  set r [::mongo::gridfs::store_file $gridFS $fn $fn text/plain]
  puts stderr r=$r
} else {
  puts stderr "no such file: $fn"
}

# unknown file
set fn unknown-file
if {[file readable $fn]} {
  set r [::mongo::gridfs::store_file $gridFS $fn $fn text/plain]
  puts stderr r=$r
} else {
  puts stderr "no such file: $fn"
}

set f [mongo::gridfile::open $gridFS README]
puts stderr "opened file $f"
puts stderr meta=[mongo::gridfile::get_metadata $f]
puts stderr contentlength=[mongo::gridfile::get_contentlength $f]
puts stderr contenttype=[mongo::gridfile::get_contenttype $f]
while {1} {
  set chunk [mongo::gridfile::read $f 500]
  puts stderr "read chunk-len [string length $chunk] content [string range $chunk 0 10]..."
  if {[string length $chunk] < 500} break;
}
mongo::gridfile::close $f
puts stderr OK

#
# close everything
#
::mongo::gridfs::close $gridFS
::mongo::close $mongoConn
