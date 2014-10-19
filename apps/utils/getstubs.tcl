#
# Copy stubs from the appropriate tcl version directory.  Since the
# configured tclsh "knows" its version, one can use this to fetch the
# appropriate stub files after configuration.
#
puts "Using stubs for Tcl [set tcl_version]"
set generic [lindex $argv 0]
file copy -force {*}[glob $generic/stubs[set tcl_version]/*.*] $generic
