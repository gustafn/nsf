namespace eval ::nsf {
  #
  # get frequenly used primitiva into the ::nsf namespace
  #
  # Symbols reused in the next scripting language
  
  namespace export \
      next current self configure finalize interp is my relation dispatch
 
  namespace eval ::nsf::method::create {namespace export alias}

  #
  # support for method provide and method require
  #

  proc ::nsf::method::provide {require_name definition {script ""}} {
    set ::nsf::methodIndex($require_name) [list definition $definition script $script]
  }

  proc ::nsf::method::require {object name {per_object 0}} {
    #
    # On a method require, the optional script is evaluated and the
    # "definition" gets inserted
    # - on posiiton 1 the actual object
    # - on posiiton 2 optionally "-per-object"
    #
    # The definition cmd must return the method handle.
    #
    set key ::nsf::methodIndex($name)
    if {[info exists $key]} {
      array set "" [set $key]
      if {$(script) ne ""} {
	eval $(script)
      }
      if {$per_object} {
	set cmd [linsert $(definition) 1 -per-object]
	return [eval [linsert $cmd 1 $object]]
      } else {
        return [eval [linsert $(definition) 1 $object]]
      }
    } else {
      error "cannot require method $name for $object, method unknown"
    }
  }

  #
  # The following helper proc is used e.g. in OpenACS to pair
  # introspection with nsf::procs.
  #
  ::proc strip_proc_name {name} {
    if {[string match ::nsf::procs::* $name]} {
      return [string range $name 12 end]
    } elseif {[string match nsf::procs::* $name]} {
      return [string range $name 12 end]
    } else {
      return $name
    }
  }

  #
  # ::nsf::mixin 
  #
  # Provide a similar interface as for ::nsf::method::create, ::nsf::method::alias,
  # etc..  Semantically, ::nsf::mixin behaves like a "mixin add", but
  # can be used as well for deleting the mixin list (empty last
  # argument).
  #

  ::nsf::proc ::nsf::mixin {object -per-object:switch classes} {
    set rel [expr {${per-object} ? "object-mixin" : "class-mixin"}]
    if {[lindex $classes 0] ne ""} {
      set oldSetting [::nsf::relation $object $rel]
      # use uplevel to avoid namespace surprises
      uplevel [list ::nsf::relation $object $rel [linsert $oldSetting 0 $classes]]
    } else {
      uplevel [list ::nsf::relation $object $rel ""]
    }
  }

  #
  # provide some popular methods for "method require"
  #
  ::nsf::method::provide autoname {::nsf::method::alias autoname ::nsf::methods::object::autoname}
  ::nsf::method::provide exists   {::nsf::method::alias exists   ::nsf::methods::object::exists}

  ######################################################################
  # unknown handler for objects and classes
  #
  proc ::nsf::object::unknown {name} {
    foreach {key handler} [array get ::nsf::object::unknown] {
      set result [uplevel [list {*}$handler $name]]
      if {$result ne ""} {
	return $result
      }
    }
    return ""
  }
  namespace eval ::nsf::object::unknown {
    proc add {key handler} {set ::nsf::object::unknown($key) $handler}
    proc get {key}         {return $::nsf::object::unknown($key)}
    proc delete {key}      {array unset ::nsf::object::unknown($key)}
    proc keys {}           {array names ::nsf::object::unknown}
  }

  # Example unknown handler:
  # ::nsf::object::unknown::add xotcl {::xotcl::Class __unknown}


  ######################################################################
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
  # logger
  #
  if {[info command ::ns_log] ne ""} {
    proc ::nsf::log {level msg} {
      ns_log $level "nsf: $msg"
    }
  } else {
    proc ::nsf::log {level msg} {
      puts stderr "$level: $msg"
    }
  }

  #
  # deprecated command
  #
  proc ::nsf::deprecated {what oldCmd newCmd} {
    set msg "**\n** The $what $oldcmd is deprecated."
    if {$newCmd ne ""} {append msg " use $newCmd instead."}
    append msg "\n**\n"
    nsf::log Warning $msg
  }

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
