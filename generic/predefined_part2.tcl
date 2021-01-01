# -*- Tcl -*-
#
# Define a basic set of predefined Tcl commands and definitions for
# the Next Scripting Framework. This file will be transformed by
# mk_predefined.tcl into "predefined.h", which in included in nsf.c.
#
#   Copyright (C) 2009-2021 Gustaf Neumann
#   Copyright (C) 2010 Stefan Sobernig
#
# The predefined code has to be split into 2 parts due to a string
# literal limitation in ISO C99, that requires compilers to support
# only strings up to 4095 bytes.
#
# This is part 2.
#

namespace eval ::nsf {
  #
  # parameter support
  #
  namespace eval ::nsf::parameter {}
  proc ::nsf::parameter::filter {defs pattern} {
    set result {}
    foreach def $defs {
      if {[string match $pattern [::nsf::parameter::info name $def]]} {
	lappend result $def
      }
    }
    return $result
  }

  set ::nsf::parameter::syntax(::nsf::xotclnext) "?--noArgs? ?/arg .../?"
  set ::nsf::parameter::syntax(::nsf::__unset_unknown_args) ""
  set ::nsf::parameter::syntax(::nsf::exithandler) "?get?|?set /cmds/?|?unset?"

  #
  # Provide the build-time configuration settings via namespaced
  # variables, for backward compatibility.
  #

  if {[info commands ::nsf::pkgconfig] ne ""} {
    
    foreach c {version commit patchLevel} {
      set ::nsf::$c [::nsf::pkgconfig get $c]
    }

    foreach c {development memcount memtrace profile dtrace assertions} {
      set ::nsf::config($c) [::nsf::pkgconfig get $c]
    }
    
    unset -nocomplain c
    
  }
}

#
# Local variables:
#    mode: tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:
