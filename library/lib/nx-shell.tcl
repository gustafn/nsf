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
      :evalScript ${:line}
      unset :line
      if {![info exists :forever]} {
        :prompt
      }
    }
  }

  :protected object method evalScript {-exit:switch script} {
    set script [list catch [string trim $script] [current]::result [current]::opts]
    set r [uplevel #0 $script]
    if {$r} {
      puts stderr [dict get ${:opts} -errorinfo]
      if {$exit} {
        exit 1
      } else {
        unset :opts;
      }
    } else {
      if {${:result} ne ""} {
        puts stdout ${:result}
      }
      unset :result
    }
  }

  :protected object method prompt {{chan stdout}} {
    puts -nonewline $chan "% "
    flush $chan
  }

  :public object method run {argc argv} {
    if {[catch {
      if {$argc == 0} {
        # interactive mode
        :prompt stdout
        fconfigure stdin -blocking 0 -buffering line
        fileevent stdin readable [list [current] onRead]
        vwait :forever
        fileevent stdin readable {}
        exit
      } else {
        # non-interactive modes
        :nonInteract {*}$argv
      }
    } msg]} {
      puts stderr "[current] failed unexpectedly with '$msg'"
      exit 2
    }
  }

  :protected object method nonInteract {-c:switch args} {
    if {$c} {
      # non-interactive mode: arg command xor stdin
      if {[llength $args]} {
        # arg command plus argc/argv
        set args [lassign $args script]
        set ::argv $args
        set ::argc [llength $args]
      } else {
        # stdin
        set ::argv ""
        set ::argc 0
        set script [gets stdin]
      }
      :evalScript -exit $script
    } else {
      # non-interactive mode: script
      set ::argv [lassign $args argv0]
      incr ::argc -1
      if {[catch {uplevel #0 [list source $argv0]} msg]} {
        puts stderr $msg
        exit 1
      }
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
