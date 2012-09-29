############################################################
# nx.tcl -
#
#      Implementation of the NX object system, based
#      on the Next Scripting Framework
#
# Copyright (C) 2010-2012 Gustaf Neumann
# Copyright (C) 2010-2012 Stefan Sobernig
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
package require nsf
package provide nx 2.0b4

namespace eval ::nx {
  namespace eval ::nsf {}          ;# make pkg-indexer happy
  namespace eval ::nsf::object {}  ;# make pkg-indexer happy
  namespace eval ::nx::internal {} ;# make pkg-indexer happy
  namespace eval ::nx::traits {}   ;# make pkg-indexer happy

  #
  # By setting the variable bootstrap, we can check later, whether we
  # are in bootstrapping mode
  #
  set ::nsf::bootstrap ::nx

  #
  # First create the ::nx object system. The internally called methods,
  # which are not defined by in this script, must have method handles
  # included. The methods "create", "configure", "destroy", "move" and
  # "__objectparameter" are defined in this script (either scripted, or
  # via alias).
  #
  ::nsf::objectsystem::create ::nx::Object ::nx::Class {
    -class.alloc {__alloc ::nsf::methods::class::alloc}
    -class.create create
    -class.dealloc {__dealloc ::nsf::methods::class::dealloc}
    -class.objectparameter __objectparameter
    -class.recreate {__recreate ::nsf::methods::class::recreate}
    -object.configure __configure
    -object.defaultmethod {defaultmethod ::nsf::methods::object::defaultmethod}
    -object.destroy destroy
    -object.init {init ::nsf::methods::object::init}
    -object.move move
    -object.unknown unknown
  }

  #
  # get frequently used primitiva from the next scripting framework
  #
  namespace export next current self configure finalize interp is relation
  namespace import ::nsf::next ::nsf::current ::nsf::self ::nsf::dispatch

  #
  # provide the standard command set for ::nx::Object
  #
  ::nsf::method::alias Object volatile  ::nsf::methods::object::volatile
#  ::nsf::method::alias Object configure ::nsf::methods::object::configure
  ::nsf::method::alias Object upvar     ::nsf::methods::object::upvar
  ::nsf::method::alias Object destroy   ::nsf::methods::object::destroy
  ::nsf::method::alias Object uplevel   ::nsf::methods::object::uplevel

  #
  # provide ::eval as method for ::nx::Object
  #
  ::nsf::method::alias Object eval -frame method ::eval

  ######################################################################
  # Default Methods (referenced via createobjectsystem)
  ######################################################################

  namespace eval ::nsf::methods {}         ;# make pkg-indexer happy
  namespace eval ::nsf::methods::object {} ;# make pkg-indexer happy

  # Actually, we do not need an unknown handler, but if someone
  # defines his own unknown handler we define it automatically
  proc ::nsf::methods::object::unknown {m args} {
    error "[::nsf::self]: unable to dispatch method '$m'"
  }

  # The default constructor
  proc ::nsf::methods::object::init args {}

  # This method can be called on invocations of the object without a
  # specified method.
  proc ::nsf::methods::object::defaultmethod {} {::nsf::self}

  ######################################################################
  # Class methods
  ######################################################################

  # provide the standard command set for Class
  ::nsf::method::alias Class create ::nsf::methods::class::create
  ::nsf::method::alias Class new ::nsf::methods::class::new

  # set a few aliases as protected
  # "__next", if defined, should be added as well
  foreach cmd [list uplevel upvar] {
    ::nsf::method::property Object $cmd call-protected 1
  }
  unset cmd

  # protect some methods against redefinition
  ::nsf::method::property Object destroy redefine-protected true
  ::nsf::method::property Class  create  redefine-protected true

  #
  # Use method::provide for base methods in case they are overloaded
  # with scripted counterparts
  ::nsf::method::provide __alloc     {::nsf::method::alias __alloc     ::nsf::methods::class::alloc}
  ::nsf::method::provide __dealloc   {::nsf::method::alias __dealloc   ::nsf::methods::class::dealloc}
  ::nsf::method::provide __recreate  {::nsf::method::alias __recreate  ::nsf::methods::class::recreate}
  ::nsf::method::provide __configure {::nsf::method::alias __configure ::nsf::methods::object::configure}
  ::nsf::method::provide unknown     {::nsf::method::alias unknown     ::nsf::methods::object::unknown}

  #
  # The method __resolve_method_path resolves a space separated path
  # of a method name and creates from the path the necessary ensemble
  # objects when needed.
  #
  ::nsf::method::create Object __resolve_method_path {
    -per-object:switch
    -verbose:switch
    path
  } {
    set object [::nsf::self]
    set methodName $path
    set regObject ""
    if {[string first " " $path] > -1} {
      set methodName [lindex $path end]
      set regObject $object

      foreach w [lrange $path 0 end-1] {
	set scope [expr {[::nsf::is class $object] && !${per-object} ? "class" : "object"}]
	if {[::nsf::is class $object] && !${per-object}} {
	  set scope class
	  set ensembleName [::nx::slotObj ${object} __$w]
	} else {
	  set scope object
	  set ensembleName ${object}::$w
	} 
	#puts stderr "NX check $scope $object info methods $path @ <$w> cmd=[info command $w] obj?[nsf::object::exists $ensembleName] "
	if {![nsf::object::exists $ensembleName]} {
 	  #
	  # Create dispatch/ensemble object and accessor method (if wanted)
	  #
	  set o [nx::EnsembleObject create $ensembleName]
	  if {$scope eq "class"} {
	    if {$verbose} {puts stderr "... create object $o"}
	    # We are on a class, and have to create an alias to be
	    # accessible for objects
	    ::nsf::method::alias $object $w $o
	    if {$verbose} {puts stderr "... create alias $object $w $o"}
	  } else {
	    if {$verbose} {puts stderr "... create object $o"}
	  }
	  set object $o
	} else {
	  #
	  # The accessor method exists already, check, if it is
	  # appropriate for extending.
	  #
	  set type [::nsf::directdispatch $object ::nsf::methods::${scope}::info::method type $w]
	  set definition [::nsf::directdispatch $object ::nsf::methods::${scope}::info::method definition $w]
	  if {$scope eq "class"} {
	    if {$type eq ""} {
	      # In case of a copy operation, the ensemble object might
	      # exist, but the alias might be missing.
	      ::nsf::method::alias $object $w $ensembleName
	      set object $ensembleName
	    } else {
	      if {$type ne "alias"} {error "can't append to $type"}
	      if {$definition eq ""} {error "definition must not be empty"}
	      set object [lindex $definition end]
	    }
	  } else {
	    if {$type ne "object"} {error "can't append to $type"}
	    if {[llength $definition] != 3} {error "unexpected definition '$definition'"}
	    append object ::$w
	  }
	}
      }
      #puts stderr "... final object $object method $methodName"
    }
    return [list object $object methodName $methodName regObject $regObject]
  }

  ::nsf::method::property Object __resolve_method_path call-protected true

  ######################################################################
  # Define default method and property protection
  ######################################################################
  ::nsf::method::create Object __default_method_call_protection args {return false}
  ::nsf::method::create Object __default_accessor args {return public}

  ::nsf::method::property Object __default_method_call_protection call-protected true
  ::nsf::method::property Object __default_accessor call-protected true

  ######################################################################
  # Define method "method" for Class and Object
  ######################################################################

  ::nsf::method::create Class method {
    name arguments:parameter,0..* -returns body -precondition -postcondition
  } {
    set conditions [list]
    if {[info exists precondition]}  {lappend conditions -precondition  $precondition}
    if {[info exists postcondition]} {lappend conditions -postcondition $postcondition}
    array set "" [:__resolve_method_path $name]
    #puts "class method $(object).$(methodName) [list $arguments] {...}"
    set r [::nsf::method::create $(object) \
	       {*}[expr {$(regObject) ne "" ? "-reg-object [list $(regObject)]" : ""}] \
	       $(methodName) $arguments $body {*}$conditions]
    if {$r ne ""} {
      # the method was not deleted
      ::nsf::method::property $(object) $r call-protected \
	  [::nsf::dispatch $(object) __default_method_call_protection]
      if {[info exists returns]} {::nsf::method::property $(object) $r returns $returns}
    }
    return $r
  }

  ::nsf::method::create Object method {
    name arguments:parameter,0..* -returns body -precondition -postcondition
  } {
    set conditions [list]
    if {[info exists precondition]}  {lappend conditions -precondition  $precondition}
    if {[info exists postcondition]} {lappend conditions -postcondition $postcondition}
    array set "" [:__resolve_method_path -per-object $name]
    # puts "object method $(object).$(methodName) [list $arguments] {...}"
    set r [::nsf::method::create $(object) \
	       {*}[expr {$(regObject) ne "" ? "-reg-object [list $(regObject)]" : ""}] \
	       -per-object \
	       $(methodName) $arguments $body {*}$conditions]
    if {$r ne ""} {
      # the method was not deleted
      ::nsf::method::property $(object) $r call-protected \
	  [::nsf::dispatch $(object) __default_method_call_protection]
      if {[info exists returns]} {::nsf::method::property $(object) $r returns $returns}
    }
    return $r
  }

  ######################################################################
  # Define method "unknown"
  ######################################################################

  Class eval {

    # define unknown handler for class
    :method unknown {methodName args} {
      error "method '$methodName' unknown for [::nsf::self];\
	consider '[::nsf::self] create $methodName $args' instead of '[::nsf::self] $methodName $args'"
    }
    # protected is not yet defined
    ::nsf::method::property [::nsf::self] unknown call-protected true
  }

  ######################################################################
  # Remember names of method defining methods
  ######################################################################

  # Well, class is not a method defining method either, but a modifier
  array set ::nsf::methodDefiningMethod {
    method 1 alias 1 forward 1 class 1
    ::nsf::classes::nx::Class::method 1 ::nsf::classes::nx::Object::method 1
    ::nsf::classes::nx::Class::alias 1 ::nsf::classes::nx::Object::alias 1
    ::nsf::classes::nx::Class::forward 1 ::nsf::classes::nx::Object::forward 1
  }

  ######################################################################
  # Provide method modifiers for ::nx::Object
  ######################################################################
  Object eval {

    # method modifier "public"
    :method public {args} {
      if {![info exists ::nsf::methodDefiningMethod([lindex $args 0])]} {
	error "'[lindex $args 0]' is not a method defining method"
      } elseif {[lindex $args 0] eq "class" && ![info exists ::nsf::methodDefiningMethod([lindex $args 1])]} {
	error "'[lindex $args 1]' is not a method defining method"
      }
      set r [: -system {*}$args]
      if {$r ne ""} {::nsf::method::property [self] $r call-protected false}

      return $r
    }

    # method modifier "protected"
    :method protected {args} {
      if {![info exists ::nsf::methodDefiningMethod([lindex $args 0])]} {
	error "'[lindex $args 0]' is not a method defining method"
      } elseif {[lindex $args 0] eq "class" && ![info exists ::nsf::methodDefiningMethod([lindex $args 1])]} {
	error "'[lindex $args 1]' is not a method defining method"
      }
      set r [: -system {*}$args]
      if {$r ne ""} {::nsf::method::property [self] $r call-protected true}
      return $r
    }

    # method modifier "private"
    :method private {args} {
      if {![info exists ::nsf::methodDefiningMethod([lindex $args 0])]} {
	error "'[lindex $args 0]' is not a method defining method"
      } elseif {[lindex $args 0] eq "class" && ![info exists ::nsf::methodDefiningMethod([lindex $args 1])]} {
	error "'[lindex $args 1]' is not a method defining method"
      }
      set r [: -system {*}$args]
      if {$r ne ""} {::nsf::method::property [self] $r call-private true}
      return $r
    }
  }

  # Provide a placeholder for objectparameter during the bootup
  # process. The real definition is based on slots, which are not
  # available at this point.

  Object protected method __objectparameter {} {;}

  ######################################################################
  # Define forward methods
  ######################################################################
  #
  # We could do this simply as
  #
  #   ::nsf::method::forward Object forward ::nsf::method::forward %self -per-object
  #   ::nsf::method::forward Class forward ::nsf::method::forward %self
  #
  # but then, we would loose the option to use compound names
  #

  Object public method forward {
     method
     -default -methodprefix -objframe:switch -onerror -returns -verbose:switch
     target:optional args
   } {
    array set "" [:__resolve_method_path -per-object $method]
    set arguments [lrange [::nsf::current args] 1 end]
    if {[info exists returns]} {
      # search for "-returns" in the arguments before $args ...
      set p [lsearch -exact [lrange $arguments 0 [expr {[llength $arguments]-[llength $args]}]] -returns]
      # ... and remove it if found
      if {$p > -1} {set arguments [lreplace $arguments $p $p+1]}
    }
    set r [::nsf::method::forward $(object) -per-object $(methodName) {*}$arguments]
    ::nsf::method::property $(object) -per-object $r call-protected \
	[::nsf::dispatch $(object) __default_method_call_protection]
    if {[info exists returns]} {::nsf::method::property $(object) $r returns $returns}
    return $r
  }

  Class public method forward {
     method
     -default -methodprefix -objframe:switch -onerror -returns -verbose:switch
     target:optional args
   } {
    array set "" [:__resolve_method_path $method]
    set arguments [lrange [::nsf::current args] 1 end]
    if {[info exists returns]} {
      # search for "-returns" in the arguments before $args ...
      set p [lsearch -exact [lrange $arguments 0 [expr {[llength $arguments]-[llength $args]}]] -returns]
      # ... and remove it if found
      if {$p > -1} {set arguments [lreplace $arguments $p $p+1]}
    }
    set r [::nsf::method::forward $(object) $(methodName) {*}$arguments]
    ::nsf::method::property $(object) $r call-protected \
	[::nsf::dispatch $(object) __default_method_call_protection]
    if {[info exists returns]} {::nsf::method::property $(object) $r returns $returns}
    return $r
  }

  ######################################################################
  # Provide method "alias"
  #
  # -frame object|method make only sense for c-defined cmds,
  ######################################################################

  Object public method alias {methodName -returns {-frame default} cmd} {
    array set "" [:__resolve_method_path -per-object $methodName]
    #puts "object alias $(object).$(methodName) $cmd"
    set r [::nsf::method::alias $(object) -per-object $(methodName) \
	       -frame $frame $cmd]
    ::nsf::method::property $(object) -per-object $r call-protected \
	[::nsf::dispatch $(object) __default_method_call_protection]
    if {[info exists returns]} {::nsf::method::property $(object) $r returns $returns}
    return $r
  }

  Class public method alias {methodName -returns {-frame default} cmd} {
    array set "" [:__resolve_method_path $methodName]
    #puts "class alias $(object).$(methodName) $cmd"
    set r [::nsf::method::alias $(object) $(methodName) -frame $frame $cmd]
    ::nsf::method::property $(object) $r call-protected \
	[::nsf::dispatch $(object) __default_method_call_protection]
    if {[info exists returns]} {::nsf::method::property $(object) $r returns $returns}
    return $r
  }

  ######################################################################
  # Basic definitions for slots
  ######################################################################
  #
  # The function isSlotContainer tests, whether the provided object is
  # a slot container based on the method property slotcontainer, used
  # internally by the serializer.
  #
  proc ::nx::isSlotContainer {object} {
    return [::nsf::object::property $object slotcontainer]
  }

  #
  # The function nx::internal::setSlotContainerProperties set the method
  # properties for slot containers
  #
  proc ::nx::internal::setSlotContainerProperties {baseObject containerName} {
    set slotContainer ${baseObject}::$containerName
    $slotContainer ::nsf::methods::object::requirenamespace
    ::nsf::method::property $baseObject -per-object $containerName call-protected true
    ::nsf::method::property $baseObject -per-object $containerName redefine-protected true
    #puts stderr "::nsf::method::property $baseObject -per-object $containerName slotcontainer true"
    #::nsf::method::property $baseObject -per-object $containerName slotcontainer true
    ::nsf::object::property $slotContainer slotcontainer true
  }

  #
  # The function nx::slotObj ensures that the slot container for the
  # provided baseObject exists. It returns either the name of the
  # slotContainer (when no slot name was provided) or the fully
  # qualified name of the slot object.
  #
  ::nsf::proc ::nx::slotObj {{-container slot} baseObject name:optional} {
    # Create slot container object if needed
    set slotContainer ${baseObject}::$container
    if {![::nsf::object::exists $slotContainer]} {
      ::nx::Object ::nsf::methods::class::alloc $slotContainer
      ::nx::internal::setSlotContainerProperties $baseObject $container
      if {$container eq "per-object-slot"} {
	::nsf::object::property $baseObject hasperobjectslots true
      }
    }
    if {[info exists name]} {
      return ${slotContainer}::$name
    }
    return ${slotContainer}
  }

  ######################################################################
  # Allocate system slot containers
  ######################################################################
  ::nx::slotObj ::nx::Class
  ::nx::slotObj ::nx::Object


  ######################################################################
  # Define the EnsembleObject with its base methods
  ######################################################################

  Class create ::nx::EnsembleObject
  ::nx::EnsembleObject eval {
    #
    # The EnsembleObjects are called typically with a "self" bound to
    # the object, on which they are registered as methods. This way,
    # only method registered on the object are resolved (ensemble
    # methods). Only for the methods "unknown" and "defaultmethod",
    # self is actually the ensemble object. These methods are
    # maintenance methods. We have to be careful here ...
    #
    # a) not to interfere between "maintenance methods" and "ensemble
    #    methods" within the maintenance methods. This is achieved
    #    via explicit dispatch commands in the maintenance methods.
    #
    # b) not to overload "maintenance methods" with "ensemble
    #    methods".  This is achieved via the object-method-only policy
    #    (we cannot call "subcmd <subcmdName>" when "subcmdName" is a
    #    method on EnsembleObject) and via a skip object-methods flag
    #    in nsf when calling e.g. "unknown" (such that a subcmd
    #    "unknown" does not interfere with the method "unknown").
    #
    :protected method init {} {
      ::nsf::object::property [self] keepcallerself true
      ::nsf::object::property [self] perobjectdispatch true
    }
    :protected method unknown {callInfo args} {
      set path [lrange $callInfo 1 end-1]; # set path [current methodpath]
      set m [lindex $callInfo end]
      set obj [lindex $callInfo 0]
      #puts stderr "### [list $obj ::nsf::methods::object::info::lookupmethods -path \"$path *\"]"
      if {[catch {set valid [$obj ::nsf::methods::object::info::lookupmethods -path "$path *"]} errorMsg]} {
	set valid ""
	puts stderr "+++ UNKNOWN raises error $errorMsg"
      }
      set ref "\"$m\" of $obj $path"
      error "unable to dispatch sub-method $ref; valid are: [join [lsort $valid] {, }]"
    }

    :protected method defaultmethod {} {
      if {[catch {set obj [uplevel ::nsf::current]}]} {
	error "ensemble dispatch called outside of method context"
      }
      set path [::nsf::current methodpath]
      set l [string length $path]
      set submethods [$obj ::nsf::methods::object::info::lookupmethods -path "$path *"]
      foreach sm $submethods {set results([lindex [string range $sm $l+1 end] 0]) 1}
      error "valid submethods of $obj $path: [lsort [array names results]]"
    }

    # end of EnsembleObject
  }
  ######################################################################
  # Now we are able to use ensemble methods in the definition of NX
  ######################################################################

  #
  # Method for deletion of properties, variables and plain methods
  #
  Object public method "delete property" {name} {
    # call explicitly the per-object variant of "info::slotobjects"
    set slot [: ::nsf::methods::object::info::slotobjects -type ::nx::Slot $name]
    if {$slot eq ""} {error "[self]: cannot delete object specific property '$name'"}
    $slot destroy
    nsf::var::unset -nocomplain [self] $name
  }
  Object public method "delete variable" {name} {
    # First remove the instance variable and complain, if it does
    # not exist.
    if {[nsf::var::exists [self] $name]} {
      nsf::var::unset [self] $name
    } else {
      error "[self]: object does not have an instance variable '$name'"
    }
    # call explicitly the per-object variant of "info::slotobejcts"
    set slot [: ::nsf::methods::object::info::slotobjects -type ::nx::Slot $name]

    if {$slot ne ""} {
      # it is not a slot-less variable
      $slot destroy
    }
  }
  Object public method "delete method" {name} {
    ::nsf::method::delete [self] -per-object $name
  }

  Class public method "delete property" {name} {
    set slot [:info slot objects $name]
    if {$slot eq ""} {error "[self]: cannot delete property '$name'"}
    $slot destroy
  }
  Class public alias "delete variable" ::nx::Class::slot::__delete::property
  Class public method "delete method" {name} {
    ::nsf::method::delete [self] $name
  }

  #
  # provide aliases for "class delete"
  #
  ::nx::Class eval {
    :alias "class delete property" ::nx::Object::slot::__delete::property
    :alias "class delete variable" ::nx::Object::slot::__delete::variable
    :alias "class delete method" ::nx::Object::slot::__delete::method
  }

  ######################################################################
  # Provide method "require"
  ######################################################################
  Object eval {
    :method "require namespace" {} {
      ::nsf::directdispatch [::nsf::self] ::nsf::methods::object::requirenamespace
    }
    #
    # method require, base cases
    #
    :method "require method" {methodName} {
      return [::nsf::method::require [::nsf::self] $methodName 0]
    }
    :method "require class method" {methodName} {
      ::nsf::method::require [::nsf::self] $methodName 1
      return [:info lookup method $methodName]
    }
    #
    # method require, public explicitly
    #
    :method "require public method" {methodName} {
      set result [:require method $methodName]
      ::nsf::method::property [self] $result call-protected false
      return $result
    }
    :method "require public class method" {methodName} {
      set result [:require class method $methodName]
      ::nsf::method::property [self] $result call-protected false
      return $result
    }
    #
    # method require, protected explicitly
    #
    :method "require protected method" {methodName} {
      set result [:require method $methodName]
      ::nsf::method::property [self] $result call-protected true
      return $result
    }
    :method "require protected class method" {methodName} {
      set result [:require class method $methodName]
      ::nsf::method::property [self] $result call-protected true
      return $result
    }

    #
    # method require, private explicitly
    #
    :method "require private method" {methodName} {
      set result [:require method $methodName]
      ::nsf::method::property [self] $result call-private true
      return $result
    }
    :method "require private class method" {methodName} {
      set result [:require class method $methodName]
      ::nsf::method::property [self] $result call-private true
      return $result
    }
  }

  ######################################################################
  # Provide Tk-style methods for configure and cget
  ######################################################################
  Object eval {
    :public alias cget ::nsf::methods::object::cget

    :protected alias __configure ::nsf::methods::object::configure    
    :public method configure {args} {
      if {[llength $args] == 0} {
	: ::nsf::methods::object::info::objectparameter syntax
      } else {
	: __configure {*}$args
	return
      }
    }
  }

  ######################################################################
  # Info definition
  ######################################################################

  # we have to use "eval", since objectParameters are not defined yet

  Object eval {
    :alias "info lookup filter"  ::nsf::methods::object::info::lookupfilter
    :alias "info lookup method"  ::nsf::methods::object::info::lookupmethod
    :alias "info lookup methods" ::nsf::methods::object::info::lookupmethods
    :method "info lookup slots" {{-type:class ::nx::Slot} -source pattern:optional} {
      set cmd [list ::nsf::methods::object::info::lookupslots -type $type]
      if {[info exists source]} {lappend cmd -source $source}
      if {[info exists pattern]} {lappend cmd $pattern}
      return [: {*}$cmd]
    }
    :method "info lookup parameter definitions" {pattern:optional} {
      set cmd [list ::nsf::methods::object::info::objectparameter definitions]
      if {[info exists pattern]} {lappend cmd $pattern}
      return [: {*}$cmd]
    }
    :method "info lookup parameter names" {pattern:optional} {
      set cmd [list ::nsf::methods::object::info::objectparameter names]
      if {[info exists pattern]} {lappend cmd $pattern}
      return [: {*}$cmd]
    }
    :method "info lookup parameter list" {pattern:optional} {
      set cmd [list ::nsf::methods::object::info::objectparameter list]
      if {[info exists pattern]} {lappend cmd $pattern}
      return [: {*}$cmd]
    }
    :method "info lookup parameter syntax" {pattern:optional} {
      set cmd [list ::nsf::methods::object::info::objectparameter syntax]
      if {[info exists pattern]} {lappend cmd $pattern}
      return [: {*}$cmd]
    }
    :alias "info children"         ::nsf::methods::object::info::children
    :alias "info class"            ::nsf::methods::object::info::class
    :alias "info filter guard"     ::nsf::methods::object::info::filterguard
    :alias "info filter methods"   ::nsf::methods::object::info::filtermethods
    :alias "info has mixin"        ::nsf::methods::object::info::hasmixin
    :alias "info has namespace"    ::nsf::methods::object::info::hasnamespace
    :alias "info has type"         ::nsf::methods::object::info::hastype
    :alias "info is"               ::nsf::methods::object::info::is
    :alias "info methods"          ::nsf::methods::object::info::methods
    :alias "info mixin guard"      ::nsf::methods::object::info::mixinguard
    :alias "info mixin classes"    ::nsf::methods::object::info::mixinclasses
    :alias "info name"             ::nsf::methods::object::info::name
    :alias "info parent"           ::nsf::methods::object::info::parent
    :alias "info precedence"       ::nsf::methods::object::info::precedence
    :method "info slot definition" {{-type:class ::nx::Slot} pattern:optional} {
      set result {}
      foreach slot [: ::nsf::methods::object::info::slotobjects -type $type {*}[current args]] {
	lappend result [$slot getPropertyDefinition]
      }
      return $result
    }
    :method "info slot names" {{-type:class ::nx::Slot} pattern:optional} {
      set result {}
      foreach slot [: ::nsf::methods::object::info::slotobjects -type $type {*}[current args]] {
	lappend result [$slot name]
      }
      return $result
    }
    :method "info slot objects" {{-type:class ::nx::Slot} pattern:optional} {
      return [: ::nsf::methods::object::info::slotobjects -type $type {*}[current args]]
    }
    # "info properties" is a short form of "info slot definition"
    :alias "info properties"     ::nx::Object::slot::__info::slot::definition
    :alias "info vars"           ::nsf::methods::object::info::vars
  }

  ######################################################################
  # Create the ensemble object for "info" here manually to prevent the
  # replicated definitions from Object.info in Class.info.
  # Potentially, some names are overwritten later by Class.info. Note,
  # that the automatically created name of the ensemble object has to
  # be the same as defined above.
  ######################################################################

  EnsembleObject create ::nx::Class::slot::__info
  Class alias info ::nx::Class::slot::__info

  #
  # The following test is just for the redefinition case, after a
  # "package forget". We clear "info method" for ::nx::Object to avoid
  # confusions in the copy loop below, which uses method "method".
  #
  if {[::nsf::directdispatch ::nx::Object::slot::__info ::nsf::methods::object::info::methods "method"] ne ""} {
    Object method "info method" {} {}
  }

  #
  # Copy all info methods except the sub-objects to
  # ::nx::Class::slot::__info
  #
  nsf::object::property ::nx::Class::slot::__info keepcallerself false
  foreach m [::nsf::directdispatch ::nx::Object::slot::__info ::nsf::methods::object::info::methods] {
    if {[::nsf::directdispatch ::nx::Object::slot::__info ::nsf::methods::object::info::method type $m] eq "object"} continue
    set definition [::nsf::directdispatch ::nx::Object::slot::__info ::nsf::methods::object::info::method definition $m]
    ::nx::Class::slot::__info {*}[lrange $definition 1 end]
    unset definition
  }
  nsf::object::property ::nx::Class::slot::__info keepcallerself true

  Class eval {
    :alias "info lookup"         ::nx::Object::slot::__info::lookup
    :alias "info filter guard"   ::nsf::methods::class::info::filterguard
    :alias "info filter methods" ::nsf::methods::class::info::filtermethods
    :alias "info has"            ::nx::Object::slot::__info::has
    :alias "info heritage"       ::nsf::methods::class::info::heritage
    :alias "info instances"      ::nsf::methods::class::info::instances
    :alias "info methods"        ::nsf::methods::class::info::methods
    :alias "info mixin guard"    ::nsf::methods::class::info::mixinguard
    :alias "info mixin classes"  ::nsf::methods::class::info::mixinclasses
    :alias "info mixinof"        ::nsf::methods::class::info::mixinof
    :method "info parameter definitions" {pattern:optional} {
      set cmd [list ::nsf::methods::class::info::slotobjects -closure -type ::nx::Slot]
      if {[info exists pattern]} {lappend cmd $pattern}
      return [::nsf::parameter::specs -configure [: {*}$cmd]]
    }
    :method "info parameter list" {{pattern:optional ""}} {
      set defs [:info parameter definitions {*}$pattern]
      set result ""
      foreach def $defs {lappend result [::nsf::parameter::get list $def]}
      return $result
    }
    :method "info parameter names" {{pattern:optional ""}} {
      set defs [:info parameter definitions {*}$pattern]
      set result ""
      foreach def $defs {lappend result [::nsf::parameter::get name $def]}
      return $result
    }
    :method "info parameter syntax" {{pattern:optional ""}} {
      set defs [:info parameter definitions {*}$pattern]
      set result ""
      foreach def $defs {lappend result [::nsf::parameter::get syntax $def]}
      return [join $result " "]
    }
    :method "info slot objects" {{-type ::nx::Slot} -closure:switch -source:optional pattern:optional} {
      set cmd [list ::nsf::methods::class::info::slotobjects -type $type]
      if {[info exists source]} {lappend cmd -source $source}
      if {$closure} {lappend cmd -closure}
      if {[info exists pattern]} {lappend cmd $pattern}
      return [: {*}$cmd]
    }
    :method "info slot definition" {{-type ::nx::Slot} -closure:switch -source:optional pattern:optional} {
      set result {}
      foreach slot [: ::nsf::methods::class::info::slotobjects -type $type {*}[current args]] {
	lappend result [$slot getPropertyDefinition]
      }
      return $result
    }
    :method "info slot names" {{-type ::nx::Slot} -closure:switch -source:optional pattern:optional} {
      set result {}
      foreach slot [: ::nsf::methods::class::info::slotobjects -type $type {*}[current args]] {
	lappend result [$slot name]
      }
      return $result
    }
    # "info properties" is a short form of "info slot definition"
    :alias "info properties"     ::nx::Class::slot::__info::slot::definition
    :alias "info subclass"       ::nsf::methods::class::info::subclass
    :alias "info superclass"     ::nsf::methods::class::info::superclass
  }

  ######################################################################
  # Define "info info" and "info unknown"
  ######################################################################

  proc ::nx::internal::infoOptions {obj} {
    #puts stderr "INFO INFO $obj -> '[::nsf::directdispatch $obj ::nsf::methods::object::info::methods -methodtype all]'"
    set methods [list]
    foreach name [::nsf::directdispatch $obj ::nsf::methods::object::info::methods] {
      if {$name eq "unknown"} continue
      lappend methods $name
    }
    return "valid options are: [join [lsort $methods] {, }]"
  }

  Object protected method "info unknown" {method obj:object args} {
    error "[::nsf::self] unknown info option \"$method\"; [$obj info info]"
  }

  Object method "info info" {} {::nx::internal::infoOptions ::nx::Object::slot::__info}
  Class  method "info info" {} {::nx::internal::infoOptions ::nx::Class::slot::__info}

  # finally register method "method" (otherwise, we cannot use "method" above)
  Object alias "info method" ::nsf::methods::object::info::method
  Class  alias "info method" ::nsf::methods::class::info::method

  ######################################################################
  # Definition of "abstract method foo ...."
  #
  # Deactivated for now. If we like to revive this method, it should
  # be integrated with the method modifiers and the method "class"
  #
  # Object method abstract {methtype -per-object:switch methName arglist} {
  #   if {$methtype ne "method"} {
  #     error "invalid method type '$methtype', must be 'method'"
  #   }
  #   set body "
  #     if {!\[::nsf::current isnextcall\]} {
  #       error \"abstract method $methName $arglist called\"
  #     } else {::nsf::next}
  #   "
  #   if {${per-object}} {
  #     :method -per-object $methName $arglist $body
  #   }  else {
  #     :method $methName $arglist $body
  #   }
  # }

  #
  # Provide basic "class ...." functionality. The aliases require the
  # RHS to be defined.
  #

  ::nx::Class eval {

    :alias "class alias" ::nsf::classes::nx::Object::alias
    :alias "class forward" ::nsf::classes::nx::Object::forward
    :alias "class method" ::nsf::classes::nx::Object::method
    :alias "class info" ::nx::Object::slot::__info

    :method "class filter" args {
      set what filter
      switch [llength $args] {
	0 {return [::nsf::relation [::nsf::self] object-$what]}
	1 {return [::nsf::relation [::nsf::self] object-$what {*}$args]}
	default {return [::nx::Object::slot::$what [lindex $args 0] \
			     [::nsf::self] object-$what \
			     {*}[lrange $args 1 end]]
	}
      }
    }
    :method "class mixin" args {
      set what mixin
      switch [llength $args] {
	0 {return [::nsf::relation [::nsf::self] object-$what]}
	1 {return [::nsf::relation [::nsf::self] object-$what {*}$args]}
	default {return [::nx::Object::slot::$what [lindex $args 0] \
			     [::nsf::self] object-$what \
			     {*}[lrange $args 1 end]]
	}
      }
    }
    :alias "class filterguard" ::nsf::methods::object::filterguard
    :alias "class mixinguard" ::nsf::methods::object::mixinguard
  }

  ######################################################################
  # MetaSlot definitions
  #
  # The MetaSlots are used later to create SlotClasses
  ######################################################################
  #
  # We are in bootstrap code; we cannot use slots/parameter to define
  # slots, so the code is a little low level. After the definition of
  # the slots, we can use slot-based code such as "-parameter" or
  # "objectparameter".
  #
  Class create ::nx::MetaSlot
  ::nsf::relation MetaSlot superclass Class

  MetaSlot class method requireClass {required:class old:class,0..1} {
    #
    # Combine two classes and return the more specialized one
    #
    if {$old eq "" || $old eq $required} {return $required}
    if {[$required info superclass -closure $old] ne ""} {
      #puts stderr "required $required has $old as superclass => specializing"
      return $required
    } elseif {[$required info subclass -closure $old] ne ""} {
      #puts stderr "required $required is more general than $old => keep $old"
      return $old
    } else {
      error "required class $required not compatible with $old"
    }
  }

  MetaSlot public class method parseParameterSpec {
    {-class ""}
    {-defaultopts ""}
    spec
    default:optional
  } {
    array set opt $defaultopts
    set opts ""
    set colonPos [string first : $spec]
    if {$colonPos == -1} {
      set name $spec
      set parameterOptions ""
    } else {
      set parameterOptions [string range $spec [expr {$colonPos+1}] end]
      set name [string range $spec 0 [expr {$colonPos -1}]]
      foreach property [split $parameterOptions ,] {
        if {$property in [list "required" "convert" "substdefault" "noarg" "noleadingdash"]} {
	  if {$property eq "convert" } {set class [:requireClass ::nx::VariableSlot $class]}
          lappend opts -$property 1
        } elseif {$property eq "noconfig"} {
	  set opt(-config) 0 ;# TODO
        } elseif {$property eq "incremental"} {
	  error "parameter option incremental must not be used; use non-positional argument -incremental instead"
        } elseif {[string match type=* $property]} {
	  set class [:requireClass ::nx::VariableSlot $class]
          set type [string range $property 5 end]
          if {![string match ::* $type]} {set type ::$type}
        } elseif {[string match arg=* $property]} {
          set argument [string range $property 4 end]
          lappend opts -arg $argument
        } elseif {[string match method=* $property]} {
          lappend opts -methodname [string range $property 7 end]
        } elseif {$property eq "optional"} {
	  lappend opts -required 0
        } elseif {$property in [list "alias" "forward" "initcmd"]} {
	  set class [:requireClass ::nx::ObjectParameterSlot $class]
	  lappend opts -disposition $property
	  set class [:requireClass ::nx::ObjectParameterSlot $class]
        } elseif {[regexp {([01])[.][.]([1n*])} $property _ minOccurance maxOccurance]} {
	  lappend opts -multiplicity $property
        } else {
          set type $property
        }
      }
    }

    if {[info exists type]} {
      #if {$type eq "switch"} {error "switch is not allowed as type for object parameter $name"}
      lappend opts -type $type
    }
    lappend opts {*}[array get opt]
    #puts stderr "[self] *** parseParameterSpec [list $name $parameterOptions $class $opts]"
    return [list $name $parameterOptions $class $opts]
  }

  MetaSlot public class method createFromParameterSpec {
    target
    -per-object:switch
    {-class ""}
    {-initblock ""}
    {-private:switch}
    {-incremental:switch}
    {-defaultopts ""}
    spec
    default:optional
  } {

    lassign [:parseParameterSpec -class $class -defaultopts $defaultopts $spec] \
	name parameterOptions class opts

    lappend opts -incremental $incremental
    if {[info exists default]} {
      lappend opts -default $default
    }
    if {${per-object}} {
      lappend opts -per-object true
      set scope object
      set container per-object-slot
    } else {
      set scope class
      set container slot
    }

    if {$private} {
      regsub -all :  __$target _ prefix 
      lappend opts -settername $name -name __private($target,$name)
      set slotname ${prefix}.$name
    } else {
      set slotname $name
    }

    if {$class eq ""} {
      set class ::nx::VariableSlot
    } else {
      #puts stderr "*** Class for '$target $name' is $class // [$class info heritage]"
    }

    #puts stderr "*** [list $class create [::nx::slotObj -container $container $target $slotname] {*}$opts $initblock]"
    set r [$class create [::nx::slotObj -container $container $target $slotname] {*}$opts $initblock]
    #puts stderr "*** returned $r"
    return $r
  }

}
namespace eval ::nx {

  ######################################################################
  # Slot definitions
  ######################################################################

  MetaSlot create ::nx::Slot

  MetaSlot create ::nx::ObjectParameterSlot
  ::nsf::relation ObjectParameterSlot superclass Slot

  MetaSlot create ::nx::MethodParameterSlot
  ::nsf::relation MethodParameterSlot superclass Slot

  # Create a slot instance for dispatching method parameter specific
  # value checkers
  MethodParameterSlot create ::nx::methodParameterSlot

  # Define a temporary, low level interface for defining slot
  # values. Normally, this is done via slot objects, which are defined
  # later. The proc is removed later in this script.

  proc createBootstrapVariableSlots {class definitions} {
    foreach att $definitions {
      if {[llength $att]>1} {foreach {att default} $att break}
      set slotObj [::nx::slotObj $class $att]
      #puts stderr "::nx::BootStrapVariableSlot create $slotObj"
      ::nx::BootStrapVariableSlot create $slotObj
      if {[info exists default]} {
        #puts stderr "::nsf::var::set $slotObj default $default"
        ::nsf::var::set $slotObj default $default
        unset default
      }
      #
      # register the standard setter
      #
      ::nsf::method::setter $class $att
      #
      # set for every bootstrap property slot the position 0
      #
      ::nsf::var::set $slotObj position 0
      ::nsf::var::set $slotObj config 1
    }

    #puts stderr "Bootstrap-slot for $class calls parameter:invalidate::classcache"
    ::nsf::parameter:invalidate::classcache $class
  }

  ObjectParameterSlot public method namedParameterSpec {prefix name options} {
    #
    # Build a pos/nonpos parameter specification from name and option list
    #
    if {[llength $options]>0} {
      return $prefix${name}:[join $options ,]
    } else {
      return $prefix${name}
    }
  }

  ######################################################################
  # Define slots for slots
  ######################################################################
  #
  # We would like to have property slots during bootstrap to
  # configure the slots itself (e.g. a relation slot object). This is
  # however a chicken/egg problem, so we use a very simple class for
  # defining slots for slots, called BootStrapVariableSlot.
  #
  MetaSlot create ::nx::BootStrapVariableSlot
  ::nsf::relation BootStrapVariableSlot superclass ObjectParameterSlot

  BootStrapVariableSlot public method getParameterSpec {} {
    #
    # Bootstrap version of getParameter spec. Just bare essentials.
    #
    if {[info exists :parameterSpec]} {
      return ${:parameterSpec}
    }
    set name [namespace tail [self]]
    set prefix [expr {[info exists :positional] && ${:positional} ? "" : "-"}]
    set options [list]
    if {[info exists :default]} {
      if {[string match {*\[*\]*} ${:default}]} {
        append options substdefault
      }
      set :parameterSpec [list [list [:namedParameterSpec $prefix $name $options]] ${:default}]
    } else {
      set :parameterSpec [list [:namedParameterSpec $prefix $name $options]]
    }
    return ${:parameterSpec}
  }

  BootStrapVariableSlot protected method init {args} {
    #
    # Empty constructor; do nothing, intentionally without "next"
    #
  }

  ######################################################################
  # configure nx::Slot
  ######################################################################
  createBootstrapVariableSlots ::nx::Slot {
  }

  ######################################################################
  # configure nx::ObjectParameterSlot
  ######################################################################

  createBootstrapVariableSlots ::nx::ObjectParameterSlot {
    {name "[namespace tail [::nsf::self]]"}
    {domain "[lindex [regexp -inline {^(.*)::(per-object-slot|slot)::[^:]+$} [::nsf::self]] 1]"}
    {manager "[::nsf::self]"}
    {per-object false}
    {methodname}
    {forwardername}
    {defaultmethods {get assign}}
    {accessor public}
    {incremental:boolean false}
    {config true}
    {noarg}
    {noleadingdash}
    {disposition alias}
    {required false}
    {default}
    {initcmd}
    {substdefault false}
    {position 0}
    {positional}
    {elementtype}
    {multiplicity 1..1}
  }

  # TODO: check, if substdefault/default could work with e.g. alias; otherwise, move substdefault down
  #
  # Default unknown handler for all slots
  #
  ObjectParameterSlot protected method unknown {method args} {
    #
    # Report just application specific methods not starting with "__"
    #
    set methods [list]
    foreach m [::nsf::directdispatch [::nsf::self] \
		   ::nsf::methods::object::info::lookupmethods -source application] {
      if {[string match __* $m]} continue
      lappend methods $m
    }
    error "method '$method' unknown for slot [::nsf::self]; valid are: {[lsort $methods]}"
  }

  ObjectParameterSlot protected method init {args} {
    #
    # Provide a default depending on :name for :methodname.  When slot
    # objects are created, invalidate the object parameters to reflect
    # the changes
    #
    if {${:incremental} && [:info class] eq [current class]} {
      error "flag incremental must not be used for this slot type"
    }
    if {![info exists :methodname]} {
      set :methodname ${:name}
    }
    if {${:per-object}} {
      ::nsf::parameter:invalidate::objectcache ${:domain}
    } else {
      ::nsf::parameter:invalidate::classcache ${:domain}
    }

    #
    # plain object parameter have currently no setter/forwarder
    #
  }

  ObjectParameterSlot public method destroy {} {
    if {[info exists :domain] && ${:domain} ne ""} {
      #
      # When slot objects are destroyed, flush the parameter cache and
      # delete the accessors
      #
      #puts stderr "*** slot destroy of [self], domain ${:domain} per-object ${:per-object}"
      
      if {${:per-object}} {
	::nsf::parameter:invalidate::objectcache ${:domain}
	if {[${:domain} ::nsf::methods::object::info::method exists ${:name}]} {
	  ::nsf::method::delete ${:domain} -per-object ${:name}
	}
      } else {
	::nsf::parameter:invalidate::classcache ${:domain}
	if {[${:domain} ::nsf::methods::class::info::method exists ${:name}]} {
	  ::nsf::method::delete ${:domain} ${:name}
	}
      }
    }
    ::nsf::next
  }

  ObjectParameterSlot protected method makeForwarder {} {
    #
    # Build forwarder from the source object class ($domain) to the slot
    # to delegate read and update operations
    #
    # intended to be called on RelationSlot or VariableSlot
    #
    if {![info exists :forwardername]} {
      set :forwardername ${:methodname}
    }
    #puts stderr "makeforwarder --> '${:forwardername}'"
    if {[info exists :settername]} {
      set name ${:settername}
    } else {
      set name ${:name}
    }
    ::nsf::method::forward ${:domain} \
	{*}[expr {${:per-object} ? "-per-object" : ""}] \
	$name \
	${:manager} \
	[list %1 [${:manager} defaultmethods]] %self \
	${:forwardername}
  }

  ObjectParameterSlot protected method getParameterOptions {
    {-withMultiplicity 0}
    {-forObjectParameter 0}
  } {
    #
    # Obtain a list of parameter options from slot object
    #
    set options [list]
    if {[info exists :elementtype] && ${:elementtype} ne {}} {
      lappend options ${:elementtype}
      #puts stderr "+++ [self] added elementtype ${:elementtype}"
    }
    lappend options ${:disposition}
    if {${:name} ne ${:methodname}} {lappend options method=${:methodname}}
    if {${:required}} {
      lappend options required
    } elseif {[info exists :positional] && ${:positional}} {
      lappend options optional
    }
    if {$forObjectParameter} {
      if {[:info lookup method assign] ni {"" "::nsf::classes::nx::RelationSlot::assign"}} {
	# In case the "assign" method was provided on the slot, ask nsf to call it directly
	lappend options slot=[::nsf::self] slotassign
      } elseif {[:info lookup method get] ni {"" "::nsf::classes::nx::RelationSlot::get"}} {
	# In case the "get" method was provided on the slot, ask nsf to call it directly
	lappend options slot=[::nsf::self]
      }
      if {[info exists :substdefault] && ${:substdefault}} {
	lappend options substdefault
      }
    }
    if {[info exists :noarg] && ${:noarg}} {lappend options noarg}
    if {[info exists :noleadingdash] && ${:noleadingdash}} {lappend options noleadingdash}
    if {$withMultiplicity && [info exists :multiplicity] && ${:multiplicity} ne "1..1"} {
      #puts stderr "### [self] added multiplicity ${:multiplicity}"
      lappend options ${:multiplicity}
    }
    return $options
  }

  ObjectParameterSlot public method getParameterSpec {} {
    #
    # Get a full object parmeter specification from slot object
    #
    if {[info exists :parameterSpec]} {
    } else {
      set prefix [expr {[info exists :positional] && ${:positional} ? "" : "-"}]
      set options [:getParameterOptions -withMultiplicity true -forObjectParameter true]

      if {[info exists :initcmd]} {
	lappend options initcmd
	if {[info exists :default]} {
	  # append initcmd "[::nsf::self] setCheckedInstVar -nocomplain \[::nsf::self\] [list ${:default}]\n"
	  append initcmd "::nsf::var::set \[::nsf::self\] ${:name} [list ${:default}]\n"
	}
	append initcmd ${:initcmd}
	set :parameterSpec [list [:namedParameterSpec $prefix ${:name} $options] $initcmd]

      } elseif {[info exists :default]} {
	# deactivated for now: || [string first {$} ${:default}] > -1
	if {[string match {*\[*\]*} ${:default}]} {
	  lappend options substdefault
	}
	set :parameterSpec [list [:namedParameterSpec $prefix ${:name} $options] ${:default}]
      } else {
	set :parameterSpec [list [:namedParameterSpec $prefix ${:name} $options]]
      }
    }
    return ${:parameterSpec}
  }

  ObjectParameterSlot public method getPropertyDefinition {} {
    set options [:getParameterOptions -withMultiplicity true]
    if {[info exists :positional]} {lappend options positional}
    if {!${:config}} {lappend options noconfig}
    if {[info exists :default]} {
      return [list [:namedParameterSpec "" ${:name} $options] ${:default}]
    } else {
      return [list [:namedParameterSpec "" ${:name} $options]]
    }
  }

  ######################################################################
  # We have no working objectparameter yet, since it requires a
  # minimal slot infrastructure to build object parameters from
  # slots. The above definitions should be sufficient as a basis for
  # object parameters. We provide the definition here before we refine
  # the slot definitions.
  #
  # Invalidate previously defined object parameter (built with the
  # empty objectparameter definition.
  #
  ::nsf::parameter:invalidate::classcache MetaSlot

  ######################################################################
  # Define objectparameter method
  ######################################################################

  Object protected method __objectparameter {} {
    set slotObjects [nsf::directdispatch [self] ::nsf::methods::object::info::lookupslots -type ::nx::Slot]
    return [::nsf::parameter::specs $slotObjects]
  }
}

namespace eval ::nx {

  ######################################################################
  #  class nx::RelationSlot
  ######################################################################

  MetaSlot create ::nx::RelationSlot
  ::nsf::relation RelationSlot superclass ObjectParameterSlot

  createBootstrapVariableSlots ::nx::RelationSlot {
    {accessor public}
    {multiplicity 0..n}
  }

  RelationSlot protected method init {} {
    ::nsf::next
    if {${:accessor} ne ""} {
      :makeForwarder
    }
  }

  #
  # create methods for slot operations assign/get/add/delete
  #
  ::nsf::method::alias RelationSlot assign ::nsf::relation
  ::nsf::method::alias RelationSlot get ::nsf::relation

  RelationSlot protected method delete_value {obj prop old value} {
    #
    # Helper method for the delete operation, deleting a value from a
    # relation slot list.
    #
    if {[string first * $value] > -1 || [string first \[ $value] > -1} {
      #
      # Value contains globbing meta characters.
      #
      if {[info exists :elementtype] && ${:elementtype} eq "mixinreg"
	  && ![string match ::* $value]} {
	#
        # Prefix glob pattern with ::, since all object names have
        # leading "::"
	#
        set value ::$value
      }
      return [lsearch -all -not -glob -inline $old $value]
    } elseif {[info exists :elementtype] && ${:elementtype} eq "mixinreg"} {
      #
      # Value contains no globbing meta characters, and elementtype could be
      # fully qualified
      #
      if {[string first :: $value] == -1} {
	#
	# Obtain a fully qualified name.
	#
        if {![::nsf::object::exists $value]} {
          error "$value does not appear to be an object"
        }
        set value [::nsf::directdispatch $value -frame method ::nsf::self]
      }
    }
    set p [lsearch -exact $old $value]
    if {$p > -1} {
      return [lreplace $old $p $p]
    } else {
      #
      # In the resulting list might be guards. If so, do another round
      # of checking to test the first list element.
      #
      set new [list]
      set found 0
      foreach v $old {
	if {[llength $v]>1 && $value eq [lindex $v 0]} {
	  set found 1
	  continue
	}
	lappend new $v
      }
      if {!$found} {error "$value is not a $prop of $obj (valid are: $old)"}
      return $new
    }
  }

  RelationSlot public method get {obj prop} {
    ::nsf::relation $obj $prop
  }

  RelationSlot public method add {obj prop value {pos 0}} {
    set oldSetting [::nsf::relation $obj $prop]
    #puts stderr [list ::nsf::relation $obj $prop [linsert $oldSetting $pos $value]]
    #
    # Use uplevel to avoid namespace surprises
    #
    uplevel [list ::nsf::relation $obj $prop [linsert $oldSetting $pos $value]]
  }

  RelationSlot public method delete {-nocomplain:switch obj prop value} {
    uplevel [list ::nsf::relation $obj $prop \
		 [:delete_value $obj $prop [::nsf::relation $obj $prop] $value]]
  }

  ######################################################################
  # Register system slots
  ######################################################################

  #
  # Most system slots are RelationSlots
  #
  ::nx::RelationSlot create ::nx::Object::slot::mixin \
      -forwardername object-mixin -elementtype mixinreg
  ::nx::RelationSlot create ::nx::Object::slot::filter \
      -forwardername object-filter -elementtype filterreg

  ::nx::RelationSlot create ::nx::Class::slot::mixin \
      -forwardername class-mixin -elementtype mixinreg
  ::nx::RelationSlot create ::nx::Class::slot::filter \
      -forwardername class-filter -elementtype filterreg

  #
  # Create two convenience object parameters to allow configuration
  # of per-object mixins and filters for classes.
  #
  ::nx::ObjectParameterSlot create ::nx::Class::slot::object-mixin \
      -methodname "::nsf::classes::nx::Object::mixin" -elementtype mixinreg
  ::nx::ObjectParameterSlot create ::nx::Class::slot::object-filter \
      -methodname "::nsf::classes::nx::Object::filter" -elementtype filterreg

  #
  # Create object parameter slots for "noninit" and "volatile"
  #
  ::nx::ObjectParameterSlot create ::nx::Object::slot::noinit \
      -methodname ::nsf::methods::object::noinit -noarg true
  ::nx::ObjectParameterSlot create ::nx::Object::slot::volatile -noarg true
  ::nsf::method::create ::nx::Object::slot::volatile assign {object var value} {$object volatile}
  ::nsf::method::create ::nx::Object::slot::volatile get {object var} {
    ::nsf::object::property $object volatile
  }

  #
  # Define "class" as a ObjectParameterSlot defined as alias
  #
  ::nx::ObjectParameterSlot create ::nx::Object::slot::class \
      -methodname "::nsf::methods::object::class" -elementtype class

  #
  # Define "superclass" as a ObjectParameterSlot defined as alias
  #
  ::nx::ObjectParameterSlot create ::nx::Class::slot::superclass \
      -methodname "::nsf::methods::class::superclass" \
      -elementtype class \
      -multiplicity 1..n \
      -default ::nx::Object

  #
  # Define the initcmd as a positional ObjectParameterSlot
  #
  ::nx::ObjectParameterSlot create ::nx::Object::slot::__initcmd \
      -disposition initcmd \
      -noleadingdash true \
      -positional true \
      -position 2

  #
  # Make sure the invalidate all ObjectParameterSlots
  #
  ::nsf::parameter:invalidate::classcache ::nx::ObjectParameterSlot

  #
  # Define method "guard" for mixin- and filter-slots of Object and Class
  #
  ::nx::Object::slot::filter method guard {obj prop filter guard:optional} {
    if {[info exists guard]} {
      ::nsf::directdispatch $obj ::nsf::methods::object::filterguard $filter $guard
    } else {
      $obj info filter guard $filter
    }
  }
  ::nx::Class::slot::filter method guard {obj prop filter guard:optional} {
    if {[info exists guard]} {
      ::nsf::directdispatch $obj ::nsf::methods::class::filterguard $filter $guard
    } else {
      $obj info filter guard $filter
    }
  }
  ::nx::Object::slot::mixin method guard {obj prop mixin guard:optional} {
    if {[info exists guard]} {
      ::nsf::directdispatch $obj ::nsf::methods::object::mixinguard $mixin $guard
    } else {
      $obj info mixin guard $mixin
    }
  }
  ::nx::Class::slot::mixin method guard {obj prop filter guard:optional} {
    if {[info exists guard]} {
      ::nsf::directdispatch $obj ::nsf::methods::class::mixinguard $filter $guard
    } else {
      $obj info mixin guard $filter
    }
  }
  #::nsf::method::alias ::nx::Class::slot::object-filter guard ::nx::Object::slot::filter::guard

  #
  # With a special purpose eval, we could avoid the need for
  # reconfigure for slot changes via eval (two cases in the regression
  # test). However, most of the eval uses are from various reading
  # purposes, so maybe this is an overkill.
  #
  #::nx::ObjectParameterSlot public method eval {cmd} {
  #  set r [next]
  #  #puts stderr "eval on slot [self] $cmd -> $r"
  #  :reconfigure
  #  return $r
  #}


  ######################################################################
  # Variable slots
  ######################################################################
  ::nsf::parameter:invalidate::classcache MetaSlot

  MetaSlot create ::nx::VariableSlot -superclass ::nx::ObjectParameterSlot

  createBootstrapVariableSlots ::nx::VariableSlot {
    {arg}
    {convert false}
    {incremental:boolean false}
    {multiplicity 1..1}
    {accessor public}
    {type}
    {settername}
    valuecmd
    defaultcmd
    valuechangedcmd
  }

  ::nx::VariableSlot public method setCheckedInstVar {-nocomplain:switch object value} {
    
    if {[::nsf::var::exists $object ${:name}] && !$nocomplain} {
      error "object $object has already an instance variable named '${:name}'"
    }
    set options [:getParameterOptions -withMultiplicity true]
    if {[llength $options]} {
      ::nsf::is -complain [join $options ,] $value
    }
    
    set traces [::nsf::directdispatch $object -frame object ::trace info variable ${:name}]
    foreach trace $traces { 
      lassign $trace ops cmdPrefix
      ::nsf::directdispatch $object -frame object ::trace remove variable ${:name} $ops $cmdPrefix
      append restore "[list ::nsf::directdispatch $object -frame object ::trace add variable ${:name} $ops $cmdPrefix]\n"
    }
    ::nsf::var::set $object ${:name} ${:default}
    if {[info exists restore]} { {*}$restore }
  }

  ::nx::VariableSlot protected method getParameterOptions {
    {-withMultiplicity 0}
    {-forObjectParameter 0}
  } {
    set options ""
    set slotObject ""
    if {[info exists :type]} {
      set type ${:type}
      if {$type eq "switch" && !$forObjectParameter} {set type boolean}
      if {$type eq "initcmd"} {
	lappend options initcmd
      } elseif {[string match ::* $type]} {
	lappend options [expr {[::nsf::is metaclass $type] ? "class" : "object"}] type=$type
      } else {
	lappend options $type
	if {$type ni [list "" \
			     "boolean" "integer" "object" "class" \
			     "metaclass" "baseclass" "parameter" \
			     "alnum" "alpha" "ascii" "control" "digit" "double" \
			     "false" "graph" "lower" "print" "punct" "space" "true" \
			     "wideinteger" "wordchar" "xdigit" ]} {
	  lappend options slot=[::nsf::self] 
	}
      }
    } elseif {[:info lookup method assign] ne "::nsf::classes::nx::VariableSlot::assign"} {
      # In case the "assign" method was provided on the slot, ask nsf to call it directly
      lappend options slot=[::nsf::self] slotassign
    } elseif {[:info lookup method get] ne "::nsf::classes::nx::VariableSlot::get"} {
      # In case the "get" method was provided on the slot, ask nsf to call it directly
      lappend options slot=[::nsf::self]
    }
    if {[:info lookup method initialize] ne "" && $forObjectParameter} {
      if {"slot=[::nsf::self]" ni $options} {lappend options slot=[::nsf::self]}
      lappend options slotinitialize
    }
    if {[info exists :arg]} {lappend options arg=${:arg}}
    if {${:required}} {
      lappend options required
    } elseif {[info exists :positional] && ${:positional}} {
      lappend options optional
    }
    if {${:convert}} {lappend options convert}
    if {$withMultiplicity && [info exists :multiplicity] && ${:multiplicity} ne "1..1"} {
      lappend options ${:multiplicity}
    }
    if {$forObjectParameter} {
      if {[info exists :substdefault] && ${:substdefault}} {
	lappend options substdefault
      }
      if {[info exists :config] && !${:config}} {
	lappend options noconfig
      }
    }
    #puts stderr "*** getParameterOptions [self] returns '$options'"
    return $options
  }

  ::nx::VariableSlot protected method isMultivalued {} {
    return [string match {*..[n*]} ${:multiplicity}]
  }

  ::nx::VariableSlot protected method needsForwarder {} {
    #
    # We just forward, when
    #   * "assign" and "add" are still untouched, or
    #   * or incremental is specified
    #
    if {[:info lookup method assign] ne "::nsf::classes::nx::VariableSlot::assign"} {return 1}
    if {[:info lookup method add] ne "::nsf::classes::nx::VariableSlot::add"} {return 1}
    if {[:info lookup method get] ne "::nsf::classes::nx::VariableSlot::get"} {return 1}
    if {[info exists :settername]} {return 1}
    if {!${:incremental}} {return 0}
    #if {![:isMultivalued]} {return 0}
    #puts stderr "[self] ismultivalued"
    return 1
  }

  ::nx::VariableSlot public method makeAccessor {} {
    
    if {${:accessor} eq "none"} {
      #puts stderr "Do not register forwarder ${:domain} ${:name}"
      return 0
    }

    if {[:needsForwarder]} {
      set handle [:makeForwarder]
      :makeIncrementalOperations
    } else {
      set handle [:makeSetter]
    }

    if {${:accessor} eq "protected"} {
      ::nsf::method::property ${:domain} {*}[expr {${:per-object} ? "-per-object" : ""}] \
	  $handle call-protected true
      set :config 0
    } elseif {${:accessor} eq "private"} {
      ::nsf::method::property ${:domain} {*}[expr {${:per-object} ? "-per-object" : ""}] \
	  $handle call-private true
      set :config 0
    } elseif {${:accessor} ne "public"} {
      error "accessor value '${:accessor}' invalid; might be one of public|protected|private or none"
    }
    return 1
  }

  ::nx::VariableSlot public method reconfigure {} {
    #puts stderr "*** Should we reconfigure [self]???"
    unset -nocomplain :parameterSpec
    :makeAccessor
    if {${:per-object} && [info exists :default]} {
      :setCheckedInstVar -nocomplain=[info exists :nocomplain] ${:domain} ${:default}
    }
    if {[::nsf::is class ${:domain}]} {
      ::nsf::parameter:invalidate::classcache ${:domain}
    }
  }

  ::nx::VariableSlot protected method init {} {
    #puts "VariableSlot [self] ${:incremental} && ${:accessor} && ${:multiplicity} incremental ${:incremental}"
    if {${:incremental}} {
      if {${:accessor} eq "none"} { set :accessor "public" }
      if {![:isMultivalued]} { set :multiplicity "0..n" }
    }
    next
    :makeAccessor
    :handleTraces
  }

  ::nx::VariableSlot protected method makeSetter {} {
    set options [:getParameterOptions -withMultiplicity true]
    set setterParam ${:name}
    if {[llength $options]>0} {append setterParam :[join $options ,]}
    ::nsf::method::setter ${:domain} {*}[expr {${:per-object} ? "-per-object" : ""}] $setterParam
  }

  ::nx::VariableSlot protected method makeIncrementalOperations {} {
    set options_single [:getParameterOptions]
    #if {[llength $options_single] == 0} {}
    if {![info exists :type]} {
      # No need to make per-slot methods; the general rules on
      # nx::VariableSlot are sufficient
      return
    }
    #puts "makeIncrementalOperations -- $options_single // [:info vars]"
    #if {[info exists :type]} {puts ".... type ${:type}"}
    set options [:getParameterOptions -withMultiplicity true]
    lappend options slot=[::nsf::self]
    set body {::nsf::var::set $obj $var $value}

    # We need the following rule e.g. for private properties, where
    # the setting of the property is handled via slot.
    if {[:info lookup method assign] eq "::nsf::classes::nx::VariableSlot::assign"} {
      #puts stderr ":public method assign [list obj var [:namedParameterSpec {} value $options]] $body"
      :public method assign [list obj var [:namedParameterSpec {} value $options]] $body
    }
    if {[:isMultivalued] && [:info lookup method add] eq "::nsf::classes::nx::VariableSlot::add"} {
      lappend options_single slot=[::nsf::self]
      #puts stderr ":public method add [list obj prop [:namedParameterSpec {} value $options_single] {pos 0}] {::nsf::next}"
      :public method add [list obj prop [:namedParameterSpec {} value $options_single] {pos 0}] {::nsf::next}
    } else {
      # TODO should we deactivate add/delete?
    }
  }

  ######################################################################
  # Handle variable traces
  ######################################################################

  ::nx::VariableSlot protected method handleTraces {} {
    # essentially like before
    set __initcmd ""
    set trace {::nsf::directdispatch [::nsf::self] -frame object ::trace}
    # There might be already default values registered on the
    # class. If so, defaultcmd is ignored.
    if {[info exists :default]} {
      if {[info exists :defaultcmd]} {error "defaultcmd can't be used together with default value"}
      if {[info exists :valuecmd]} {error "valuecmd can't be used together with default value"}
    } elseif [info exists :defaultcmd] {
      if {[info exists :valuecmd]} {error "valuecmd can't be used together with defaultcmd"}
      append __initcmd "$trace add variable [list ${:name}] read \
	\[list [::nsf::self] __default_from_cmd \[::nsf::self\] [list [set :defaultcmd]]\]\n"
    } elseif [info exists :valuecmd] {
      append __initcmd "$trace add variable [list ${:name}] read \
	\[list [::nsf::self] __value_from_cmd \[::nsf::self\] [list [set :valuecmd]]\]"
    }
    if {[info exists :valuechangedcmd]} {
      append __initcmd "$trace add variable [list ${:name}] write \
	\[list [::nsf::self] __value_changed_cmd \[::nsf::self\] [list [set :valuechangedcmd]]\]"
    }
    if {$__initcmd ne ""} {
      if {${:per-object}} {
	${:domain} eval $__initcmd
      }
      set :initcmd $__initcmd
    }
  }

  #
  # Implementation of methods called by the traces
  #
  ::nx::VariableSlot method __default_from_cmd {obj cmd var sub op} {
    #puts "GETVAR [::nsf::current method] obj=$obj cmd=$cmd, var=$var, op=$op"
    ::nsf::directdispatch $obj -frame object \
	::trace remove variable $var $op [list [::nsf::self] [::nsf::current method] $obj $cmd]
    ::nsf::var::set $obj $var [$obj eval $cmd]
  }
  ::nx::VariableSlot method __value_from_cmd {obj cmd var sub op} {
    #puts "GETVAR [::nsf::current method] obj=$obj cmd=$cmd, var=$var, op=$op"
    ::nsf::var::set $obj $var [$obj eval $cmd]
  }
  ::nx::VariableSlot method __value_changed_cmd {obj cmd var sub op} {
    # puts stderr "**************************"
    # puts "valuechanged obj=$obj cmd=$cmd, var=$var, op=$op, ...\n$obj exists $var -> [::nsf::var::set $obj $var]"
    eval $cmd
  }

  ######################################################################
  # Implementation of (incremental) forwarder operations for
  # VariableSlots:
  #  - assign
  #  - get
  #  - add
  #  - delete
  ######################################################################

  ::nsf::method::alias ::nx::VariableSlot get ::nsf::var::set
  ::nsf::method::alias ::nx::VariableSlot assign ::nsf::var::set

  ::nx::VariableSlot public method add {obj prop value {pos 0}} {
    if {![:isMultivalued]} {
      #puts stderr "... vars [[self] info vars] // [[self] eval {set :multiplicity}]"
      error "property $prop of [set :domain] ist not multivalued"
    }
    if {[::nsf::var::exists $obj $prop]} {
      ::nsf::var::set $obj $prop [linsert [::nsf::var::set $obj $prop] $pos $value]
    } else {
      ::nsf::var::set $obj $prop [list $value]
    }
  }

  ::nx::VariableSlot public method delete {-nocomplain:switch obj prop value} {
    set old [::nsf::var::set $obj $prop]
    set p [lsearch -glob $old $value]
    if {$p>-1} {::nsf::var::set $obj $prop [lreplace $old $p $p]} else {
      error "$value is not a $prop of $obj (valid are: $old)"
    }
  }


  ######################################################################
  # Define method "property" for convenience
  ######################################################################

  nx::Object method variable {
     {-accessor "none"}
     {-incremental:switch}
     {-class ""}
     {-config:switch}
     {-initblock ""}
     {-nocomplain:switch}
     spec:parameter
     defaultValue:optional
   } {
    #
    # This method creates sometimes a slot, sometimes not
    # (optimization).  We need a slot currently in the following
    # situations:
    #  - when accessors are needed
    #    (serializer uses slot object to create accessors)
    #  - when initblock is non empty
    #

    #puts stderr "Object variable $spec accessor $accessor nocomplain $nocomplain incremental $incremental"

    # get name and list of parameter options
    lassign [::nx::MetaSlot parseParameterSpec -class $class $spec] \
	name parameterOptions class options
    array set opts $options

    # TODO: do we need config as parameter property?
    if {[info exists opts(-config)]} {
      set config $opts(-config)
    }

    #if {$initblock eq "" && $accessor eq "none" && !$incremental} 
    if {$initblock eq "" && !$config && $accessor eq "none" && !$incremental} {
      #
      # we can build a slot-less variable
      #
      #puts "... slotless variable $spec"

      set isSwitch [regsub {\mswitch\M} $parameterOptions boolean parameterOptions]
      if {[info exists defaultValue]} {
	if {[info exists :$name] && !$nocomplain} {
	  error "object [self] has already an instance variable named '$name'"
	}
	if {$parameterOptions ne ""} {
	  #puts stderr "*** ::nsf::is $parameterOptions $defaultValue // opts=$options"
	  # we rely here that the nsf::is error message expresses the implementation limits
	  set noptions {}
	  foreach o [split $parameterOptions ,] {
	    if {$o ne "noconfig"} {lappend noptions $o}
	  }
	  set parameterOptions [join $noptions ,]
	  ::nsf::is -complain $parameterOptions $defaultValue
	} else {
	  set name $spec
	}
	set :$name $defaultValue
      } elseif {$isSwitch} {
	set :$name 0
      } else {
	error "variable definition for '$name' (without value and accessor) is useless"
      }
      return
    }

    #puts "... slot variable $spec"
    #
    # create variable via a slot object
    #
    set slot [::nx::MetaSlot createFromParameterSpec [self] \
		  -per-object \
		  -class $class \
		  -initblock $initblock \
		  -incremental=$incremental \
		  -private=[expr {$accessor eq "private"}] \
		  -defaultopts [list -accessor $accessor] \
		  $spec \
		  {*}[expr {[info exists defaultValue] ? [list $defaultValue] : ""}]]

    if {$nocomplain} {$slot eval {set :nocomplain 1}}
    if {!$config} {$slot eval {set :config false}}
    if {[info exists defaultValue]} {$slot setCheckedInstVar -nocomplain=$nocomplain [self] $defaultValue}

    if {[$slot eval {info exists :settername}]} {
      set name [$slot settername]
    } else {
      set name [$slot name]
    }

    return [::nsf::directdispatch [self] ::nsf::methods::object::info::method registrationhandle $name]
  }

  Object method property {
    {-accessor ""}
    {-incremental:switch}
    {-class ""}
    {-nocomplain:switch}
     spec:parameter
    {initblock ""}
  } {

    if {${accessor} eq ""} {
      set accessor [::nsf::dispatch [self] __default_accessor]
      #puts stderr "OBJECT got default accessor ${accessor}"
    }

    set r [[self] ::nsf::classes::nx::Object::variable \
	       -accessor $accessor \
	       -incremental=$incremental \
	       -class $class \
	       -initblock $initblock \
	       -config=true \
	       -nocomplain=$nocomplain \
	       {*}$spec]
    return $r
  }

  nx::Class method variable {
    {-accessor "none"}
    {-incremental:switch}
    {-class ""}
    {-config:switch}
    {-initblock ""}
    spec:parameter
    defaultValue:optional
  } {
    set slot [::nx::MetaSlot createFromParameterSpec [::nsf::self] \
		  -class $class \
		  -initblock $initblock \
		  -incremental=$incremental \
		  -private=[expr {$accessor eq "private"}] \
		  -defaultopts [list -accessor $accessor -config $config] \
		  $spec \
		  {*}[expr {[info exists defaultValue] ? [list $defaultValue] : ""}]]
    if {[$slot eval {info exists :settername}]} {
      set name [$slot settername]
    } else {
      set name [$slot name]
    }
    #puts stderr handle=[::nsf::directdispatch [self] ::nsf::methods::class::info::method registrationhandle $name]
    return [::nsf::directdispatch [self] ::nsf::methods::class::info::method registrationhandle $name]
  }

  nx::Class method property {
    {-accessor ""}
    {-incremental:switch}
    {-class ""}
    spec:parameter
    {initblock ""}
  } {

    if {${accessor} eq ""} {
      set accessor [::nsf::dispatch [self] __default_accessor]
      #puts stderr "CLASS got default accessor ${accessor}"
    }
    set r [[self] ::nsf::classes::nx::Class::variable \
	       -accessor $accessor \
	       -incremental=$incremental \
	       -class $class \
	       -config=true \
	       -initblock $initblock \
	       {*}$spec]
    return $r
  }

  #
  # provide aliases for "class property" and "class variable"
  #
  ::nx::Class eval {
    :alias "class property" ::nsf::classes::nx::Object::property
    :alias "class variable" ::nsf::classes::nx::Object::variable
  }


  ######################################################################
  # Define method "properties" for convenience to define multiple
  # properties based on a list of parameter specifications.
  ######################################################################

  proc ::nx::internal::addProperties {arglist} {
    foreach arg $arglist {:property $arg}
  }
  ::nx::ObjectParameterSlot create ::nx::Object::slot::properties \
      -methodname "::nx::internal::addProperties"

  ######################################################################
  # Minimal definition of a value checker that permits every value
  # without warnings. The primary purpose of this value checker is to
  # provide a means to specify that the value can have every possible
  # content and not to produce a warning when it might look like a
  # non-positional parameter.
  ######################################################################
  ::nx::Slot method type=any {name value} {
  }

  ######################################################################
  # Now the slots are defined; now we can defines the Objects or
  # classes with parameters more easily than above.
  ######################################################################

  # remove helper proc
  rename ::nx::createBootstrapVariableSlots ""

  ######################################################################
  # Define a scoped "new" method, which is similar to plain new, but
  # uses the current namespace by default as root of the object name.
  ######################################################################

  Class create ::nx::NsScopedNew {
    :public method new {-childof args} {
      if {![info exists childof]} {
	#
	# Obtain the namespace from plain uplevel to honor the
	# namespace provided by apply
	#
	set childof [uplevel {namespace current}]
      }
      #
      # Use the uplevel method to assure that "... new -volatile ..."
      # has the right scope
      #
      :uplevel [list [self] ::nsf::methods::class::new -childof $childof {*}$args]
    }
  }

  ######################################################################
  # The method 'contains' changes the namespace in which objects with
  # relative names are created.  Therefore, 'contains' provides a
  # friendly notation for creating nested object
  # structures. Optionally, creating new objects in the specified
  # scope can be turned off.
  ######################################################################

  Object public method contains {
    {-withnew:boolean true}
    -object
    {-class:class ::nx::Object}
    cmds
  } {
    if {![info exists object]} {set object [::nsf::self]}
    if {![::nsf::object::exists $object]} {$class create $object}
    # This method is reused in XOTcl which has e.g. no "require";
    # therefore use nsf primitiva.
    ::nsf::directdispatch $object ::nsf::methods::object::requirenamespace

    if {$withnew} {
      #
      # When $withnew is requested we replace the default new method
      # with a version using the current namespace as root. Earlier
      # implementations used a mixin on nx::Class and xotcl::Class,
      # but frequent mixin operations on the most general meta-class
      # are expensive when there are many classes defined
      # (e.g. several ten thousands), since the mixin operation
      # invalidates the mixins for all instances of the meta-class
      # (i.e. for all classes)
      #
      set infoMethod "::nsf::methods::class::info::method"
      set plainNew   "::nsf::methods::class::new"
      set mappedNew  [::nx::NsScopedNew $infoMethod definitionhandle new]

      set nxMapNew [expr {[::nx::Class $infoMethod origin new] eq $plainNew}]
      if {$nxMapNew} {::nsf::method::alias ::nx::Class new $mappedNew}

      if {[::nsf::is class ::xotcl::Class]} {
	set xotclMapNew [expr {[::xotcl::Class $infoMethod origin new] eq $plainNew}]
	if {$xotclMapNew} {::nsf::method::alias ::xotcl::Class new $mappedNew }
      }
      #
      # Evaluate the command under catch to ensure reverse mapping
      # of "new"
      #
      set errorOccured [catch [list ::apply [list {} $cmds $object]] errorMsg]
      #
      # Remove the mapped "new" method, if it was added above
      #
      if {$nxMapNew} {::nsf::method::alias ::nx::Class new $plainNew}
      if {[::nsf::is class ::xotcl::Class]} {
	if {$xotclMapNew} {::nsf::method::alias ::xotcl::Class new $plainNew}
      }
      if {$errorOccured} {error $errorMsg}

    } else {
      ::apply [list {} $cmds $object]
    }
  }

  ######################################################################
  # copy/move implementation
  ######################################################################

  Class create ::nx::CopyHandler {

    :property {targetList ""}
    :property {dest ""}
    :property objLength

    :method makeTargetList {t} {
      lappend :targetList $t
      #puts stderr "COPY makeTargetList $t targetList '${:targetList}'"
      # if it is an object without namespace, it is a leaf
      if {[::nsf::object::exists $t]} {
	if {[::nsf::directdispatch $t ::nsf::methods::object::info::hasnamespace]} {
	  # make target list from all children
	  set children [$t info children]
        } else {
	  # ok, no namespace -> no more children
	  return
        }
      }
      # now append all namespaces that are in the obj, but that
      # are not objects
      foreach c [namespace children $t] {
        if {![::nsf::object::exists $c]} {
          lappend children [namespace children $t]
        }
      }

      # a namespace or an obj with namespace may have children
      # itself
      foreach c $children {
        :makeTargetList $c
      }
    }

    :method copyNSVarsAndCmds {orig dest} {
      ::nsf::nscopyvars $orig $dest
      ::nsf::nscopycmds $orig $dest
    }

    # construct destination obj name from old qualified ns name
    :method getDest {origin} {
      if {${:dest} eq ""} {
	return ""
      } else {
	set tail [string range $origin [set :objLength] end]
	return ::[string trimleft [set :dest]$tail :]
      }
    }

    :method copyTargets {} {
      #puts stderr "COPY will copy targetList = [set :targetList]"
      set objs {}
      foreach origin [set :targetList] {
        set dest [:getDest $origin]
        if {[::nsf::object::exists $origin]} {
	  if {$dest eq ""} {
	    set obj [[$origin info class] new -noinit]
	    set dest [set :dest $obj]
	  } else {
	    set obj [[$origin info class] create $dest -noinit]
	  }
          # copy class information
          if {[::nsf::is class $origin]} {
	    # obj is a class, copy class specific information
            ::nsf::relation $obj superclass [$origin info superclass]
            ::nsf::method::assertion $obj class-invar [::nsf::method::assertion $origin class-invar]
	    ::nsf::relation $obj class-filter [::nsf::relation $origin class-filter]
	    ::nsf::relation $obj class-mixin [::nsf::relation $origin class-mixin]
	    :copyNSVarsAndCmds ::nsf::classes$origin ::nsf::classes$dest
	  }
	  # copy object -> might be a class obj
	  ::nsf::object::property $obj keepcallerself [::nsf::object::property $origin keepcallerself]
	  ::nsf::object::property $obj perobjectdispatch [::nsf::object::property $origin perobjectdispatch]
	  ::nsf::object::property $obj hasperobjectslots [::nsf::object::property $origin hasperobjectslots]
	  ::nsf::method::assertion $obj check [::nsf::method::assertion $origin check]
	  ::nsf::method::assertion $obj object-invar [::nsf::method::assertion $origin object-invar]
	  ::nsf::relation $obj object-filter [::nsf::relation $origin object-filter]
	  ::nsf::relation $obj object-mixin [::nsf::relation $origin object-mixin]
            # reused in XOTcl, no "require" there, so use nsf primitiva
	  if {[::nsf::directdispatch $origin ::nsf::methods::object::info::hasnamespace]} {
	    ::nsf::directdispatch $obj ::nsf::methods::object::requirenamespace
	  }
	} else {
	  namespace eval $dest {}
	}
	lappend objs $obj
	:copyNSVarsAndCmds $origin $dest
	foreach i [$origin ::nsf::methods::object::info::forward] {
	  ::nsf::method::forward $dest -per-object $i \
	      {*}[$origin ::nsf::methods::object::info::forward -definition $i]

	}
	if {[::nsf::is class $origin]} {
	  foreach i [$origin ::nsf::methods::class::info::forward] {
	    ::nsf::method::forward $dest $i \
		{*}[$origin ::nsf::methods::class::info::forward -definition $i]
	  }
	}

	#
	# Check, if $origin is a slot container. If yes, set the slot
	# container properties on $dest
	#
	if {[::nsf::object::property $origin slotcontainer]} {
	  ::nx::internal::setSlotContainerProperties \
	      [$dest ::nsf::methods::object::info::parent] [namespace tail $origin]
	}

	#
	# transfer the traces
	#
	foreach var [$origin info vars] {
	  set cmds [::nsf::directdispatch $origin -frame object ::trace info variable $var]
	  if {$cmds ne ""} {
	    foreach cmd $cmds {
	      foreach {op def} $cmd break
	      #$origin trace remove variable $var $op $def
	      set domain [lindex $def 0]
	      if {$domain eq $origin} {
		set def [concat $dest [lrange $def 1 end]]
	      }
	      if {[::nsf::object::exists $domain] && [$domain info has type ::nx::Slot]} {
		# slot traces are handled already by the slot mechanism
		continue
	      }
	      $dest trace add variable $var $op $def
	    }
	  }
	}
	#puts stderr "====="
      }

      #
      # alter 'domain' and 'manager' in slot objects
      #
      foreach origin [set :targetList] {
	set dest [:getDest $origin]
	set slots [list]
	#
	# get class specific slots
	#
	if {[::nsf::is class $origin]} {
	  set slots [$origin ::nsf::methods::class::info::slotobjects -type ::nx::Slot]
	}
	#
	# append object specific slots
	#
	foreach slot [$origin ::nsf::methods::object::info::slotobjects -type ::nx::Slot] {
	  lappend slots $slot
	}
	#puts stderr "replacing domain and manager from <$origin> to <$dest> in slots <$slots>"
	foreach oldslot $slots {
	  set container [expr {[$oldslot per-object] ? "per-object-slot" : "slot"}]
	  set newslot [::nx::slotObj -container $container $dest [namespace tail $oldslot]]
	  if {[$oldslot domain] eq $origin}   {$newslot domain $dest}
	  if {[$oldslot manager] eq $oldslot} {$newslot manager $newslot}
	  $newslot eval :init
	}
      }
      return [lindex $objs 0]
    }

    :public method copy {obj {dest ""}} {
      #puts stderr "[::nsf::self] copy <$obj> <$dest>"
      set :objLength [string length $obj]
      set :dest $dest
      :makeTargetList $obj
      :copyTargets
    }

  }

  Object public method copy {{newName ""}} {
    if {[string compare [string trimleft $newName :] [string trimleft [::nsf::self] :]]} {
      [CopyHandler new -volatile] copy [::nsf::self] $newName
    }
  }

  Object public method move {newName} {
    if {[string trimleft $newName :] ne [string trimleft [::nsf::self] :]} {
      if {$newName ne ""} {
        :copy $newName
      }
      ### let all subclasses get the copied class as superclass
      if {[::nsf::is class [::nsf::self]] && $newName ne ""} {
        foreach subclass [:info subclass] {
          set scl [$subclass info superclass]
          if {[set index [lsearch -exact $scl [::nsf::self]]] != -1} {
            set scl [lreplace $scl $index $index $newName]
	    ::nsf::relation $subclass superclass $scl
          }
        }	
      }
      :destroy
    }
  }


  ######################################################################
  # Methods of meta-classes are methods intended for classes. Make
  # sure, these methods are only applied on classes.
  ######################################################################

  foreach m [Class info methods] {
    ::nsf::method::property Class $m class-only true
  }
  if {[info exists m]} {unset m}

  ######################################################################
  # some utilities
  ######################################################################
  #
  # Provide mechanisms to configure nx
  #
  ::nx::Object create ::nx::configure {
    #
    # Set the default method protection for nx methods. This
    # protection level is used per default for all method definitions
    # of scripted methods, aliases and forwarders without explicit
    # protection specified.
    #
    :method defaultMethodCallProtection {value:boolean,optional} {
      if {[info exists value]} {
	::nsf::method::create Object __default_method_call_protection args [list return $value]
	::nsf::method::property Object  __default_method_call_protection call-protected true
      }
      return [::nsf::dispatch [::nx::self] __default_method_call_protection]
    }

    #
    # Set the default method accessor handling nx properties. The configured
    # value is used for creating accessors for properties in nx.
    #
    :method defaultAccessor {value:optional} {
      if {[info exists value]} {
	if {$value ni {"public" "protected" "private" "none"}} {
	  error {defaultAccessor must be "public", "protected", "private" or "none"}
	}
	::nsf::method::create Object __default_accessor args [list return $value]
	::nsf::method::property Object __default_accessor call-protected true
      }
      return [::nsf::dispatch [::nx::self] __default_accessor]
    }
  }
  #
  # Make the default protected methods
  #
  ::nx::configure defaultMethodCallProtection true
  ::nx::configure defaultAccessor public

  #
  # Provide an ensemble-like interface to the ::nsf primitiva to
  # access variables. Note that aliasing in the next scripting
  # framework is faster than namespace-ensembles.
  #
  Object create ::nx::var {
    :public alias exists ::nsf::var::exists
    :public alias import ::nsf::var::import
    :public alias set ::nsf::var::set
  }

  #interp alias {} ::nx::self {} ::nsf::self

  set value "?classes?|?add class?|?delete class?"
  set ::nsf::parametersyntax(::nsf::classes::nx::Object::mixin) $value
  set ::nsf::parametersyntax(::nsf::classes::nx::Class::mixin) $value
  set ::nsf::parametersyntax(::nsf::classes::nx::Class::superclass) $value
  set ::nsf::parametersyntax(::nsf::classes::nx::Object::class) "?className?"
  set value "?filters?|?add filter?|?delete filter?"
  set ::nsf::parametersyntax(::nsf::classes::nx::Object::filter) $value
  set ::nsf::parametersyntax(::nsf::classes::nx::Class::filter) $value
  set ::nsf::parametersyntax(::nsf::classes::nx::Object::eval) "arg ?arg ...?"
  unset value

  ::nsf::configure debug 1
}


namespace eval ::nx {

  ######################################################################
  # Define exported Tcl commands
  ######################################################################

  # export the main commands of ::nx
  namespace export Object Class next self current

  set ::nx::confdir ~/.nx
  set ::nx::logdir $::nx::confdir/log

  unset ::nsf::bootstrap
}

#
# When debug is not deactivated, tell the developer, what happened
#
if {[::nsf::configure debug] > 1} {
  foreach ns {::nsf ::nx} {
    puts "vars of $ns: [info vars ${ns}::*]"
    puts stderr "$ns exports: [namespace eval $ns {lsort [namespace export]}]"
  }
  puts stderr "======= nx loaded"
}
