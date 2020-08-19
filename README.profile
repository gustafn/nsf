
Profiling with NSF
==================

NSF offers the following built-in options for profiling,
which apply to methods (all instance and per class methods
of XOTcl, NX, ...) and to nsf::procs.

Preliminaries:
--------------

- make sure nsf is compiled with "--enable-profile"

- if you are not sure, check the file config.log
  in the source directory, where nsf was compiled.
  You find a line looking like
  
      $ ./configure ....

  near to the top of the file. If the configure line does
  not contain "--enable-profile", run the configure command
  with "--enable-profile" added, then
      make clean; make; sudo make install

- to check, if the version you are using is complied properly
  check the global Tcl array ::nsf::config, e.g. with 

     $ nxsh
     % parray ::nsf::config 


Profiling:
----------

   From nsf profiling supports 
   1) detailed profiling about a full run, or
   2) selective profiling 


Dtrace:
-------
   Alternatively dtrace [2] can be used to obtain 
   profiling information from nsf (and Tcl as well).... 
   but this depends on the operating system support
   (Solaris heritage).

=========================================================================================================

#
# Sample script and helper procs to show, how profiling/tracing works
# in NSF with the not widely advertised profiling support based on
#
#     ::nsf::__profile
#
# Gustaf Neumann
#
package require nx

#
# Helper function for in-memory profiling
#

proc mini-profile {cmd} {
    if {[nsf::pkgconfig get profile]} {
	try {
	    nsf::__profile_clear
	    nsf::__profile_trace -enable true
	    uplevel $cmd
	} on error {errorMsg} {
	    error $errorMsg
	} finally {
	    nsf::__profile_trace -enable false
	}
	#
	# The collected profile data contains:
	#
	#  totalMicroSec: measured time since profile start
	#
	#  overallTime:  aggregated time from traces
	#
	#  objectData	 list of lists, where every element contains
	#                {object class method} aggregated_time number_of_calls
	#
	#  methodData    list of lists, where every element contains
	#                {method class} {callerMethod callerClass} aggregated_time number_of_calls
	#
	#  procData      list of lists, where every element contains
	#                {proc aggregated_time number_of_calls}
	#
	#  trace         new-line separated entries containing call, exit, and time
	#
	lassign [::nsf::__profile_get] totalMicroSec overallTime objectData methodData procData trace
	
	# Output profile data... or do something else with it...
	#
	puts "Profile Data:\n==================="
	foreach v {totalMicroSec overallTime objectData methodData procData trace} {
	    set break [expr {$v eq "trace" ? "\n" : "\t"} ]
	    puts "$v$break[set $v]"
	}
	puts "==================="
    } else {
	puts stderr "NSF is not compiled with profiling support"
	uplevel $cmd
    }
}

nsf::proc bar {x:integer} {
    return $x
}
nx::Class create C {
    #
    # Naive version of factorial
    #
    :public method fact {n} {
	if {$n < 2} {
	    return 1
	} else {
	    return [expr {$n * [:fact [expr {$n-1}]]}]
	}
    }
    :public method foo {n} {
	bar 1
	return [:fact $n]
    }
}

C create c1

#######################################################
# Plain run
#######################################################
puts "factorial of 6 = [c1 foo 6]"


#######################################################
# Run the same command with profiling,
# collect the profile data in memory
#######################################################
mini-profile {c1 foo 6}

puts "\nTrace profiling information to output"
puts "==================="
nsf::__profile_trace -enable true -dontsave true -verbose 1
c1 foo 6
nsf::__profile_trace -enable false
puts "==================="


#######################################################
# Define custom logger to handle e.g. trace output
#######################################################
proc ::nsf::log {level msg} {
    puts stderr "MY-logger: $level: $msg"
}

puts "\nTrace profiling information to output (with extra logger)"
puts "==================="
nsf::__profile_trace -enable true -dontsave true -verbose 1
c1 foo 6
nsf::__profile_trace -enable false
puts "==================="


#######################################################
# Selective profiling and tracing
# Define custom debug commands
#######################################################
proc ::nsf::debug::call {level objectInfo methodInfo arglist} {
    nsf::log Debug "MY call($level) - $objectInfo $methodInfo $arglist"
}
proc ::nsf::debug::exit {level objectInfo methodInfo result usec} {
    nsf::log Debug "MY exit($level) - $objectInfo $methodInfo $usec usec -> $result"
}
#
# Select methods to be traced (could be also done with the
# flag "-debug" in the source
#
::nsf::method::property C fact debug true
::nsf::method::property C ::bar debug true

puts "\nSelective profiling (with extra logger, debug, and custom debug messages)"
puts "==================="
c1 foo 6
puts "==================="
