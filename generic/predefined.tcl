namespace eval ::nsf {
  #
  # get frequenly used primitiva into the ::nsf namespace
  #
  # Symbols reused in the next scripting language
  
  
  namespace export next current 
  
  # Symbols reused in XOTcl
  
  namespace export alias configure finalize interp is my relation

  #
  # support for method provide and method require
  #

  proc ::nsf::provide_method {require_name definition {script ""}} {
    set ::nsf::methodIndex($require_name) [list definition $definition script $script]
  }

  proc ::nsf::require_method {object name {per_object 0}} {
    set key ::nsf::methodIndex($name)
    if {[info exists $key]} {
      array set "" [set $key]
      if {$(script) ne ""} {
	eval $(script)
      }
      if {$per_object} {
	set cmd [linsert $(definition) 1 -per-object]
	eval [linsert $cmd 1 $object]
      } else {
        eval [linsert $(definition) 1 $object]
      }
    } else {
      error "cannot require method $name for $object, method unknown"
    }
  }

  #
  # ::nsf::mixin
  #
  # provide a similar interface as for ::nsf::method, ::nsf::alias, ...
  #

  proc ::nsf::mixin {object args} {
    if {[lindex $args 0] eq "-per-object"} {
      set rel "object-mixin"
      set args [lrange $args 1 end]
    } else {
      set rel "mixin"
    }
    set oldSetting [::nsf::relation $object $rel]
    # use uplevel to avoid namespace surprises
    uplevel [list ::nsf::relation $object $rel [linsert $oldSetting end $args]]
  }

  #
  # provide some popular methods for "method require"
  #
  ::nsf::provide_method autoname {::nsf::alias autoname ::nsf::cmd::Object::autoname}
  ::nsf::provide_method exists   {::nsf::alias  exists ::nsf::cmd::Object::exists}

  #
  # error handler for info
  #
  proc ::nsf::infoError msg {
    #puts stderr "INFO ERROR: <$msg>\n$::errorInfo"
    regsub -all " <object>" $msg "" msg
    regsub -all " <class>" $msg "" msg
    regsub {\"} $msg "\"info " msg
    error $msg ""
  }
 
  #
  # exit handlers
  #
  proc ::nsf::unsetExitHandler {} {
    proc ::nsf::__exitHandler {} {
      # clients should append exit handlers to this proc body
    }
  }
  proc ::nsf::setExitHandler {newbody} {::proc ::nsf::__exitHandler {} $newbody}
  proc ::nsf::getExitHandler {} {::info body ::nsf::__exitHandler}
  # initialize exit handler
  ::nsf::unsetExitHandler

  #
  # determine platform aware temp directory
  #
  
  proc tmpdir {} {
    foreach e [list TMPDIR TEMP TMP] {
      if {[info exists ::env($e)] \
              && [file isdirectory $::env($e)] \
              && [file writable $::env($e)]} {
        return $::env($e)
      }
    }
    if {$::tcl_platform(platform) eq "windows"} {
      foreach d [list "C:\\TEMP" "C:\\TMP" "\\TEMP" "\\TMP"] {
        if {[file isdirectory $d] && [file writable $d]} {
          return $d
        }
      }
    }
    return /tmp
  }  

  namespace export tmpdir 

  # if HOME is not set, and ~ is resolved, Tcl chokes on that
  if {![info exists ::env(HOME)]} {set ::env(HOME) /root}

}
