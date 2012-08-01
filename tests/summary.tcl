
array set opt $::argv
if {[info exists opt(-testlog)]} {
  set f [open $opt(-testlog)]; set content [read $f]; close $f
  lassign {0 0 0 0} tests success failures files
  foreach l [split $content \n] {
    array set "" $l
    if {[info exists (tests)]} {
      incr tests $(tests)
      incr failures $(failure)
      incr success $(success)
      incr files 1
    }
  }
  puts "\nRegression Test Summary:"
  puts "\tEnvironment: Tcl $tcl_patchLevel, OS $tcl_platform(os) $tcl_platform(osVersion)\
	machine $tcl_platform(machine) threaded $tcl_platform(threaded)."
  puts "\tNSF performed $tests tests in $files files, success $success, failures $failures."
  if {$failures == 0} {
    puts "\tCongratulations, NSF [package require nsf], NX [package require nx], and\
	XOTcl [package require XOTcl 2] work fine in your environment."
  }
  puts ""
}

