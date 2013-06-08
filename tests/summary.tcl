# -*- Tcl -*-
#
# This script computes the regression test summary displayed at the
# end of the regression test. It aggreates the content of the test log
# provided via arg "-testlog".

array set opt $::argv

if {[info exists opt(-testlog)]} {
  set f [open $opt(-testlog)]; set content [read $f]; close $f
  lassign {0 0 0 0 0.0} tests success failures files ms
  foreach l [split $content \n] {
    array set "" $l
    if {[info exists (tests)]} {
      incr tests $(tests)
      incr failures $(failure)
      incr success $(success)
      incr files 1
      set ms [expr {$ms + $(ms)}]
    }
  }

  puts "\nRegression Test Summary:"
  puts "\tEnvironment: Tcl $tcl_patchLevel, OS $tcl_platform(os) $tcl_platform(osVersion)\
	machine $tcl_platform(machine) threaded [info exists tcl_platform(threaded)]."
  puts "\tNSF performed $tests tests in $files files, success $success, failures $failures in [expr {$ms / 1000.0}] seconds"
  if {$failures == 0} {
    puts "\tCongratulations, NSF [package require nsf], NX [package require nx], and\
	XOTcl [package require XOTcl 2] work fine in your environment."
  }
  puts ""
}

