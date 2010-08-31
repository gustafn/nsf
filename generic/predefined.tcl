namespace eval ::nsf {
  #
  # get frequenly used primitiva into the ::nsf namespace
  #
  # Symbols reused in the next scripting language
  
  # @command assertion
  #
  # @param object:object
  # @param assertionsubcmd:required
  # @param arg

  # @command existsvar
  #
  # @param object:object
  # @param var

  # @command methodproperty
  # 
  # @param object:object
  # @param -per-object:switch
  # @param methodName
  # @param methodproperty Accepts one of: {{{protected}}},
  # {{{redefine-protected}}}, {{{returns}}}, {{{slotobj}}}
  # @param value
  
  # @command setter
  #
  # @param object:object
  # @param -per-object:switch
  # @param parameter

  # @command createobjectsystem
  #
  # @param rootClass
  # @param rootMetaClass
  # @param systemMethods:optional

  # @command dispatch
  #
  # @param object:object
  # @param -objscope
  # @param command
  # @param args

  # @command deprecated
  #
  # @param what
  # @param oldCmd
  # @param newCmd:optional

  # @command objectproperty
  #
  # @param object:object
  # @param objectkind Accepts one of: {{{type}}}, {{{object}}},
  # {{{class}}}, {{{baseclass}}}, {{{metaclass}}}, {{{hasmixin}}}
  # @param value:optional

  # @command importvar
  #
  # @param object:object
  # @param args

  # @command parametercheck
  #
  # @param -nocomplain
  # @param param
  # @param value:optional

  # @command forward
  #
  # @param object:object
  # @param -per-object:switch
  # @param method
  # @param -default
  # @param -earlybinding:switch
  # @param -methodprefix
  # @param -objscope:switch
  # @param -onerror
  # @param -verbose:switch
  # @param target
  # @param args
  
  # @command setvar
  #
  # @param object:object
  # @param variable
  # @param value
  
  # @command method
  #
  # @param object:object
  # @param -inner-namespace
  # @param -per-object
  # @param -public
  # @param name
  # @param args
  # @param body
  # @param -percondition
  # @param -postcondition

  # @command next
  #
  # Invokes the shadowed (i.e, same-named) method which is next along
  # the precedence path and returns the results of this invocation. If
  # {{{next}}} is called without arguments, the arguments of the current
  # method (i.e., the arguments as present at the current callframe) are
  # passed through to the shadowed method. If next is invoked with the
  # flag --noArgs, the shadowed method is called without the active
  # callframe arguments. If other arguments are specified for {{{next}}}
  # explicitly, these will be passed instead.
  # 
  # @param --noArgs:optional Deactivates the forward-passing of the current callframe's arguments
  # @param args Explicitly declared arguments to pass to shadowed methods


  # @command current
  #
  # An introspective command which allows you to explore the "Next"
  # callstack from within the scope of a method (or procif bound to an
  # object via {{{alias}}}). {{{current}}} computes callstack related
  # information. If executed without specifying a subcommand,
  # i.e. {{{[current]}}}, returns the name of the object, which is
  # currently in execution. If called from outside a proc, it returns
  # the error message "No current object". 
  # 
  # It comes with a variety of subcommands to query different bits of
  # callstack information. See below.
  #
  # @subcommand class Returns the name of the class holding the
  # currently executing per-class method, if and only if called from
  # within a per-class method. Note, that this method-owning class may
  # be different to the class of the current object. If called from
  # within a per-object method, it returns an empty string.
  #
  # @subcommand proc Returns the name of the currently executing method.
  #
  # @subcommand callingclass Returns the name of the class which is
  # calling into the executing method.
  #
  # @subcommand callingobject Returns the name of the object which is
  # calling into the executing method.
  #
  # @subcommand calledclass Returns the name of the class that holds
  # the originally (and now shadowed) target method (applicable in
  # mixin classes and filters).
  #
  # @subcommand calledproc Returns the name of the target method
  # (applicable in a filter only).
  #
  # @subcommand isnextcall Returns 1 if the executing method was
  # invoked via {{@command ::nx::next}}, 0 otherwise.
  #
  # @subcommand next Returns the name of the method next on the
  # precedence path as a string.
  #
  # @subcommand filterreg In a method serving as active filter,
  # returns the name of the object (class) on which the method is
  # registered as a filter.
  #
  # @subcommand callinglevel Returns the "original" callstack level
  # calling into the executing method. Intermediary {{{next}}} calls
  # are ignored in this computation. The level is returned in a form
  # so that it can be used as first argument in {{@method ::nx::Object
  # class uplevel}} or {{@method ::nx::Object class upvar}}.
  #
  # @subcommand activelevel Returns the actual callstack level calling
  # into the executing method. The active might correspond the
  # {{{callinglevel}}}, but this is not necessarily the case. The
  # {{{activelevel}}} counts {{@command ::nx::next}} call. The level
  # is returned in a form so that it can be used as first argument in
  # {{@method ::nx::Object class uplevel}} or {{@method ::nx::Object
  # class upvar}}.
  
  namespace export next current 
  # Symbols reused in XOTcl
  
  # @command configure
  # 
  # A top-level configuration facility which allows you modify
  # properties of the "Next" object system for the scope of an entire
  # {{{interp}}}.
  
  # @command.subcommand {configure filter}
  #
  # Allows turning on or off filters globally for the current
  # interpreter. By default, the filter state is turned off. This
  # function returns the old filter state. This filterless {{{interp}}}
  # state is needed for the serializer which should introspect and stream the
  # objects and classes without being affected by active filter.
  #
  # @param toggle Accepts either "on" or "off"
  # @return The current filter activation state
  
  # @command.subcommand {configure softrecreate}
  # 
  # Allows controlling the scheme applied when recreating an object or a
  # class. By default, it is set to {{{off}}}. This means that the
  # object/class is destroyed and all relations
  # (e.g. subclass/superclass) to other objects/classes are revoked as
  # well. If softrecreate is set to {{{on}}}, the object is re-set, but not
  # destroyed, the relations are
  # kept. 
  # 
  # A "soft" recreation is important for e.g. reloading a file with
  # class definitions (e.g. when used in OpenACS with file watching and
  # reloading). With softrecreate set, it is not necessary to recreate
  # dependent subclasses etc. Consider the example of a class hierarchy
  # {{{A <- B <- C}}}. Without {{{softrecreate}}} set, a reload of
  # {{{B}}} means first a destroy of B, leading to {{{A <- C}}}, and
  # instances of {{{B}}} are re-classed to {{@object
  # ::nx::Object}}. When softrecreate is set, the class hierarchy
  # remains untouched.
  #
  # @param toggle Accepts either "on" or "off"
  # @return The current toggle value
  
  
  # @command.subcommand {configure objectsystems}
  # 
  # A mere introspection subcommand. It gives you the top level of the
  # current object system, i.e., the ruling root class and root
  # meta-class. For "Next":
  #
  # {{{
  #	configure objectsystems; # returns "::nx::Object ::nx::Class"
  # }}}
  #
  # @return The active pair of root class and root meta-class
  
  # @command.subcommand {configure keepinitcmd}
  #
  # Usually, initcmd scripts are discarded by the {{{interp}}} once
  # having been evaluated (in contrast to {{{proc}}} and {{{method}}}
  # bodies). If you need them preserved for later introspection and
  # processing (as in the "Next" documentation system), set this option
  # to {{{true}}}. Then, the initcmd scripts are retained as a
  # particular object variable ({{{__initcmd}}}) of classes and
  # objects. It defaults to {{{false}}}.
  #
  # @param value:boolean Either {{{true}}} or {{{false}}}
  # @return The current setting
  
  # @command alias
  #
  # @param object:object The target object which becomes the owner of
  # the aliased command (method, object or command).
  #
  # @param -per-object:switch If the target object is a class, one can
  # specify the binding scope (i.e., per-object or per-class) of the
  # aliased command
  #
  # @param methodName The name of the alias. 
  # @param -nonleaf:switch ...
  # @param -objscope:switch ...
  # @param cmdName The alias source as a command handle (as returned by ...)

  # @command finalize

  # @command interp 
  #
  # @param name
  # @param args

  # @command is 
  #
  # @param value
  # @param constraint
  # @param -hasmixin
  # @param -type
  # @param arg
  
  # @command my
  #
  # @param -local
  # @param method
  # @param args

  # @command relation
  #
  # @param object
  # @param relationtype
  # @param value

  namespace export alias configure finalize interp is my relation

  #
  # support for method provide and method require
  #

  # @command provide_method
  #
  # @param require_name
  # @param definition
  # @param script:optional

  proc ::nsf::provide_method {require_name definition {script ""}} {
    set ::nsf::methodIndex($require_name) [list definition $definition script $script]
  }

  # @command require_method
  # 
  # @param object
  # @param name
  # @param per_object
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
  # nsf::mixin
  #
  # provide a similar interface as for ::nsf::method, ::nsf::alias, ...
  #

  # @command mixin
  # 
  # @param object
  # @param args
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
  
  # @command tmpdir
  #
  # @return The platform-specific path name to the system-wide temporary directory

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
