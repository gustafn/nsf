namespace eval ::nsf {
  #
  # get frequenly used primitiva into the ::nsf namespace
  #
  # Symbols reused in the next scripting language
  
  
  namespace export next current self
  
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

  set ::nsf::parametersyntax(::nsf::mixin) "object ?-per-object? classes"

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
  ::nsf::provide_method autoname {::nsf::alias autoname ::nsf::methods::object::autoname}
  ::nsf::provide_method exists   {::nsf::alias  exists ::nsf::methods::object::exists}

  #
  # exit handlers
  #
  proc ::nsf::exithandler {args} {
    lassign $args op value
    switch $op {
      set {::proc ::nsf::__exithandler {} $value}
      get {::info body ::nsf::__exithandler}
      unset {proc ::nsf::__exithandler args {;}}
      default {error "syntax: ::nsf::exithandler $::nsf::parametersyntax(::nsf::exithandler)"}
    }
  }
  # initialize exit handler
  ::nsf::exithandler unset

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

  set ::nsf::parametersyntax(::nsf::xotclnext) "?--noArgs? ?arg ...?"
  set ::nsf::parametersyntax(::nsf::__unset_unknown_args) ""
  set ::nsf::parametersyntax(::nsf::exithandler) "?get?|?set cmds?|?unset?"

}
