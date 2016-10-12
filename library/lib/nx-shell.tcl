package require nx

package provide nx::shell 1.1

nx::Object create ::nx::shell2 {

  :public object method onRead {{chan stdin}} {
    append :line [read $chan]
    if {[eof $chan]} {
        set :forever 0
        fileevent $chan readable {}
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
    set script [list interp invokehidden {} catch [string trim $script] [current]::result [current]::opts]
    set r [uplevel #0 $script]
    if {$r == 1} {
      puts stderr [dict get ${:opts} -errorinfo]
      if {$exit} {
        :onExit -shell 1
      } else {
        unset :opts;
      }
    } elseif {$r == 2 && [info exists :statusCode]} {
      set sc ${:statusCode}
      unset :statusCode
      if {$exit} {
        :onExit -shell $sc
      } else {
        set :forever $sc
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
    :exitOn
    if {$argc == 0} {
        # interactive mode
        :prompt stdout
        fconfigure stdin -blocking 0 -buffering line
        fileevent stdin readable [list [current] onRead]
        vwait :forever
        fileevent stdin readable {}
        :onExit -shell ${:forever}
      } else {
        # non-interactive modes
        :nonInteract {*}$argv
      }
    :exitOff
    return 0
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
      if {[catch {uplevel #0 [list source $argv0]} msg opts]} {
        puts [dict get $opts -errorinfo]
        exit 1
      }
    }
  }

  :public object method onExit {-shell:switch {statusCode 0}} {
    if {$shell} {
      :exitOff
      # outer (shell) exit
      return -code ok -level [info level] $statusCode
    } else {
      # inner (script) exit
      set :statusCode $statusCode
      return -code return -level [info level]
    }
  }
  
  :public object method onCatch {args} {
    set r [uplevel 1 [list interp invokehidden {} catch {*}$args]]
    if {$r == 2 && [info exists :statusCode]} {
      return -code return
    }
    return $r
  }

  # 8.6 only
  if {[info commands ::try] ne ""} {
    :public object method onHandler {script} {
      if {[info exists :statusCode]} {
        return -code return -level 2
      }
      uplevel 1 $script
    }

    :public object method onTry {script args} {
      set l [llength $args]
      for {set i 0; set j 1} {$i < $l} {incr i; set j [expr {$i+1}]} {
        # watch out for the finally handler
        if {$i == [expr {$l-2}] && [lindex $args $i] eq "finally"} {
          set finallyScript [lindex $args $j]
          lset args $j [list [current] onHandler $finallyScript]
          break
        }
        # watch out for on-return handlers
        if {$i < [expr {$l-3}] && [lindex $args $i] eq "on" && [lindex $args $j] in {return 2}} {
          # imputate a wrapped return script
          set idx [expr {$i+3}]
          set returnScript [lindex $args $idx]
          lset args $idx [list [current] onHandler $returnScript]
          incr i 3
        }
      }
      uplevel 1 [list interp invokehidden {} try $script {*}$args]
    }
  }
  
  :public object method exitOn {} {
    if {[info commands ::_exit] eq ""} {
      #
      # exit is already aliased/hidden by nx::test
      # 
      rename ::exit ::_exit
      proc ::exit {{exitCode 0}} "[current] onExit \$exitCode"
      interp hide {} catch;
      interp alias {} ::catch {} [current] onCatch
    }
    if {[info commands ::try] ne ""} {
      # 8.6 only
      interp hide {} try;
      interp alias {} ::try {} [current] onTry
    }
  }
  
  :public object method exitOff {} {
    if {[info commands ::_exit] ne ""} {
      rename ::exit ""
      rename ::_exit ::exit
      interp alias {} ::catch {}
      interp expose {} catch;
      if {[interp alias {} ::try] ne ""} {
        # 8.6 only
        interp alias {} ::try {}
        interp expose {} try;
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

package provide nx::shell 1.1

# Local variables:
#    mode: tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:
