#!/usr/bin/env tclsh
#
# Script to beautify scripted documentations.
#
# The script receives a nx script as an argument and
# outputs to the same directory a asciidoc scipt.
#
# Gustaf Neumann,   Dez 2010

package req nx

nx::Object create output {
  set :state ""
  set :text ""
  :public object method line {kind string} {
    if {${:state} ne $kind} {
      if {${:state} ne ""} {:flush}
      set :state $kind
      set :text ""
    }
    append :text $string \n
  }
  :public object method flush {} {
    set trimmed [string trim ${:text} \n]
    if {$trimmed ne ""} {
      :${:state} $trimmed
    }
  }
  :public object method postprocess {block} {
    set result ""
    set cmd ""
    foreach l [split $block \n] {
      append cmd $l \n
      if {[info complete $cmd]} then {
	set w0 ""
	regexp {^\s*(\S+)\s*} $cmd _ w0
	#set w0 [lindex $cmd 0]
	if { ($w0 eq "?" && [llength $cmd] == 3) || 
	     ($w0 eq "!" && [llength $cmd] == 2) } {
	  append result "% [lindex $cmd 1]\n"
	  set cmdresult [lindex $cmd 2]
	  if {$cmdresult ne "" && ![string match ::nsf::__* $cmdresult]} {append result $cmdresult \n}
	} else {
	  append result $cmd
	}
	set cmd ""
      }
    }
    return [string trimright $result \n]
  }
  :public object method prog {block} {
    puts $::out {[source,tcl]}
    puts $::out --------------------------------------------------
    puts $::out [:postprocess $block]
    puts $::out --------------------------------------------------\n
  }
  :public object method doc {block} {
    #puts $::out "=====doc\n$block\n====="
    puts $::out $block\n
  }
}

#puts stderr "find -L $opt(-path) -type f -name '$opt(-name)'"
foreach file $argv {
  set F [open $file]; set content [read $F]; close $F
  set outfn [file rootname $file].txt
  set out [open $outfn w]

  set title "Listing of $file"
  regexp {^# = (.+?)\n(.*)$} $content _ title content

  foreach ignorePattern {
    "package req nx::test"
    "package require nx::test"
    "nx::Test parameter count 1"
    "proc ! args.*?"
  } {
    regsub "$ignorePattern\s?\n" $content "" content
  }

  puts $::out "= $title\n"
  foreach line [split $content \n] {
    if {[regexp {^# ?(.*)$} $line _ comment]} {
      output line doc $comment
    } else {
      output line prog $line
    }
  }
  output flush
}