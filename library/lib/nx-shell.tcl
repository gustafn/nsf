package require nx

package provide nx::shell 1.0

nx::Object create ::nx::shell {
  :public object method run {argc argv} {
    uplevel #0 {namespace import -force ::nx::*}
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
          set script [list catch $line result opts]
          uplevel #0 [list if "\[$script\]" {
            puts [dict get $opts -errorinfo]
            unset opts;
          } else {
            puts $result
            unset result
          }]
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
