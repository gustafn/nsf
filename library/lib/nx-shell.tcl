package require nx

package provide nx::shell 1.0

nx::Object create ::nx::shell2 {

  :public object method onRead {{chan stdin}} {
    append :line [read $chan]
    if {[eof $chan]} {
        fileevent $chan readable {}
        set :forever eof
    }
    if {${:line} eq "\n"} {
      unset :line
      :prompt
      return
    }
    if {[info complete ${:line}]} {
      set script [list catch [string trim ${:line}] [current]::result [current]::opts]
      set r [uplevel #0 $script]
      if {$r} {
        puts stdout [dict get ${:opts} -errorinfo]
        unset :opts;
      } else {
        puts stdout ${:result}
        unset :result
      }
      unset :line
      if {![info exists :forever]} {
        :prompt
      }
    }
  }

  :public object method prompt {{chan stdout}} {
    puts -nonewline $chan "% "
    flush $chan
  }

  :public object method run {argc argv} {
    if {$argc == 0} {
      :prompt stdout
      fconfigure stdin -blocking 0 -buffering line
      fileevent stdin readable [list [current] onRead]
      vwait :forever
      fileevent stdin readable {}
      exit
    } else {
      set ::argv [lassign $argv argv0]
      incr ::argc -1
      uplevel #0 [list source $argv0]
    }
  }
}

nx::Object create ::nx::shell {
  :public object method run {argc argv} {
    if {$argc == 0} {
      set prefix ""
      set line ""
      while {1} {
        update
        if {$line eq ""} {
          puts -nonewline "% "
          flush stdout
        }
        append line [gets stdin]
        if {[info complete $line]} {
          set script [list catch $line [current]::result [current]::opts]
          set r [uplevel #0 $script]
          if {$r} {
            puts [dict get ${:opts} -errorinfo]
            unset :opts;
          } else {
            puts ${:result}
            unset :result
          }          
          set line ""
          continue
        }
        append line \n 
      }
    } else {
      set ::argv [lassign $argv argv0]
      incr ::argc -1
      uplevel #0 [list source $argv0]
    }
  }
}

package provide nx::shell 1.0

# Local variables:
#    mode: tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:
