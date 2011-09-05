package provide nx 2.0
package require nsf

namespace eval ::nx {
  namespace eval ::nsf {}; # make pkg indexer happy
  namespace eval ::nsf::object {}; # make pkg indexer happy
  namespace eval ::nx::internal {}; # make pkg indexer happy

  #
  # By setting the variable bootstrap, we can check later, whether we
  # are in bootstrapping mode
  #
  set ::nsf::bootstrap ::nx

  #
  # First create the ::nx object system. The interally called methods,
  # which are not defined by in this script, must have method handles
  # included. The methods "create", "configure", "destroy", "move" and
  # "objectparameter" are defined in this script (either scripted, or
  # aliases).
  #
  ::nsf::objectsystem::create ::nx::Object ::nx::Class {
    -class.alloc {alloc ::nsf::methods::class::alloc} 
    -class.create create
    -class.dealloc {dealloc ::nsf::methods::class::dealloc}
    -class.objectparameter objectparameter 
    -class.recreate {recreate ::nsf::methods::class::recreate}
    -object.configure configure
    -object.defaultmethod {defaultmethod ::nsf::methods::object::defaultmethod}
    -object.destroy destroy
    -object.init {init ::nsf::methods::object::init}
    -object.move move 
    -object.unknown unknown
  }

  #
  # get frequenly used primitiva from the next scripting framework 
  #
  namespace export next current self configure finalize interp is my relation
  namespace import ::nsf::next ::nsf::current ::nsf::self ::nsf::my ::nsf::dispatch

  #
  # provide the standard command set for ::nx::Object
  #
  ::nsf::method::alias Object volatile  ::nsf::methods::object::volatile 
  ::nsf::method::alias Object configure ::nsf::methods::object::configure 
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
  
  namespace eval ::nsf::methods {}; # make pkg indexer happy
  namespace eval ::nsf::methods::object {}; # make pkg indexer happy

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
  ::nsf::method::provide alloc     {::nsf::method::alias alloc     ::nsf::methods::class::alloc}
  ::nsf::method::provide dealloc   {::nsf::method::alias dealloc   ::nsf::methods::class::dealloc}
  ::nsf::method::provide recreate  {::nsf::method::alias recreate  ::nsf::methods::class::recreate}
  ::nsf::method::provide configure {::nsf::method::alias configure ::nsf::methods::object::configure}
  ::nsf::method::provide unknown   {::nsf::method::alias unknown   ::nsf::methods::object::unknown}

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
	#puts stderr "check $object info methods $path @ <$w>"
	# TODO: for debugging of submethods.test in tcl 8.6b2 
	#puts stderr ===nx118
	set scope [expr {[::nsf::is class $object] && !${per-object} ? "class" : "object"}] 
	#puts stderr ===nx119
	if {[::nsf::directdispatch $object ::nsf::methods::${scope}::info::methods $w] eq ""} {
 	  #
	  # Create dispatch/ensemble object and accessor method (if wanted)
	  #
	  if {$scope eq "class"} {
	    set o [nx::EnsembleObject create [::nx::slotObj ${object} __$w]]
	    if {$verbose} {puts stderr "... create object $o"}
	    # We are on a class, and have to create an alias to be
	    # accessible for objects
	    ::nsf::method::alias $object $w $o
	    if {$verbose} {puts stderr "... create alias $object $w $o"}
	  } else {
	    set o [EnsembleObject create ${object}::$w]
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
	    if {$type ne "alias"} {error "can't append to $type"}
	    if {$definition eq ""} {error "definition must not be empty"}
	    set object [lindex $definition end]
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
  ::nsf::method::create Object __default_property_call_protection args {return false}

  ::nsf::method::property Object __default_method_call_protection call-protected true
  ::nsf::method::property Object __default_property_call_protection call-protected true

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
    :method unknown {m args} {
      error "Method '$m' unknown for [::nsf::self].\
	Consider '[::nsf::self] create $m $args' instead of '[::nsf::self] $m $args'"
    }
    # protected is not yet defined
    ::nsf::method::property [::nsf::self] unknown call-protected true
  }

  ######################################################################
  # Remember names of method defining methods
  ######################################################################

  # Well, class is not a method defining method either, but a modifier
  array set ::nsf::methodDefiningMethod {method 1 alias 1 property 1 forward 1 class 1}

  ######################################################################
  # Provide method modifiers for ::nx::Object
  ######################################################################
  Object eval {

    # method modifier "public"
    :method public {args} {
      if {![info exists ::nsf::methodDefiningMethod([lindex $args 0])]} {
	error "'[lindex $args 0]' is not a method defining method"
      }
      set r [::nsf::my -system {*}$args]
      if {$r ne ""} {::nsf::method::property [self] $r call-protected false}
      return $r
    }

    # method modifier "protected"
    :method protected {args} {
      if {![info exists ::nsf::methodDefiningMethod([lindex $args 0])]} {
	error "'[lindex $args 0]' is not a method defining method"
      }
      set r [::nsf::my -system {*}$args]
      if {$r ne ""} {::nsf::method::property [self] $r call-protected true}
      return $r
    }

    # method modifier "private"
    :method private {args} {
      if {![info exists ::nsf::methodDefiningMethod([lindex $args 0])]} {
	error "'[lindex $args 0]' is not a method defining method"
      }
      set r [::nsf::my -system {*}$args]

      if {$r ne ""} {::nsf::method::property [self] $r call-private true}
      return $r
    }
  }

  # Provide a placeholder for objectparameter during the bootup
  # process. The real definition is based on slots, which are not
  # available at this point.
  Class protected method objectparameter {} {;}

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
  # Provide method "require"
  ######################################################################

  Object method require {what args} {
    switch -- $what {
      public -
      protected {
	set next [lindex $args 0]
	if {$next ni {"class" "method"}} {
	  error "public or procected must be followed by 'class' or 'method'"
	}
	set result [:require {*}$args]
	#puts stderr "[list :require {*}$args] => $result"
	::nsf::method::property [self] $result call-protected [expr {$what eq "protected"}]
	return $result
      }
      class {
	set what [lindex $args 0]
	if {$what ne "method"} {
	  error "'class' must be followed by 'method'"
	}
	set methodName [lindex $args 1]
	::nsf::method::require [::nsf::self] $methodName 1
	return [:info lookup method $methodName]
      }
      method {
	set methodName [lindex $args 0]
	return [::nsf::method::require [::nsf::self] $methodName 0]
      }
      namespace {
	::nsf::directdispatch [::nsf::self] ::nsf::methods::object::requirenamespace
      }
    }
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
    set container [namespace tail $object]
    if {[::nsf::object::exists $object] && $container in {slot per-object-slot}} {
      set parent [$object ::nsf::methods::object::info::parent]
      return [expr {[::nsf::object::exists $parent] 
		    && [::nsf::method::property $parent -per-object $container slotcontainer]}]
    }
    return 0
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
    ::nsf::method::property $baseObject -per-object $containerName slotcontainer true
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
      error "Unable to dispatch sub-method $ref; valid are: [join [lsort $valid] {, }]"
    }
    
    :protected method defaultmethod {} {
      set obj [uplevel {::nsf::current}]
      set path [::nsf::current methodpath]
      set l [string length $path]
      set submethods [$obj ::nsf::methods::object::info::lookupmethods -path "$path *"]
      foreach sm $submethods {set results([lindex [string range $sm $l+1 end] 0]) 1}
      error "Valid submethods of $obj $path: [lsort [array names results]]"
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
    set slot [::nsf::my ::nsf::methods::object::info::slotobjects $name]
    if {$slot eq ""} {error "[self]: cannot delete object specific property '$name'"}
    $slot destroy
    nsf::var::unset -nocomplain [self] $name
  }
  Object public method "delete variable" {name} {
    # First remove the instanstance variable and complain, if it does
    # not exist.
    if {[nsf::var::exists [self] $name]} {
      nsf::var::unset [self] $name
    } else {
      error "[self]: object does not have an instance variable '$name'"
    }
    # call explicitly the per-object variant of "info::slotobejcts"
    set slot [::nsf::my ::nsf::methods::object::info::slotobjects $name]

    if {$slot ne ""} {
      # it is not a slot-less variable
      $slot destroy
    }
  }
  Object public method "delete method" {name} {
    array set "" [:__resolve_method_path -per-object $name]
    ::nsf::method::delete $(object) -per-object $(methodName)
  }

  Class public method "delete property" {name} {
    set slot [:info slot objects $name]
    if {$slot eq ""} {error "[self]: cannot delete property '$name'"}
    $slot destroy
  }
  Class public alias "delete variable" ::nx::Class::slot::__delete::property
  Class public method "delete method" {name} {
    array set "" [:__resolve_method_path $name]
    ::nsf::method::delete $(object) $(methodName)
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
  # Info definition
  ######################################################################

  # we have to use "eval", since objectParameters are not defined yet
  
  Object eval {
    :alias "info lookup filter"  ::nsf::methods::object::info::lookupfilter
    :alias "info lookup method"  ::nsf::methods::object::info::lookupmethod
    :alias "info lookup methods" ::nsf::methods::object::info::lookupmethods
    :method "info lookup slots" {{-type ::nx::Slot} -source pattern:optional} {
      set cmd [list ::nsf::methods::object::info::lookupslots -type $type]
      if {[info exists source]} {lappend cmd -source $source}
      if {[info exists pattern]} {lappend cmd $pattern}
      return [::nsf::my {*}$cmd]
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
    :alias "info parent"           ::nsf::methods::object::info::parent
    :alias "info precedence"       ::nsf::methods::object::info::precedence
    :method "info slot definition" {{-type ::nx::Slot} -closure:switch -source:optional pattern:optional} {
      set result {}
      foreach slot [::nsf::my ::nsf::methods::object::info::slotobjects {*}[current args]] {
	lappend result [$slot getPropertyDefinition]
      }
      return $result
    }
    :method "info slot names" {{-type ::nx::Slot} -closure:switch -source:optional pattern:optional} {
      set result {}
      foreach slot [::nsf::my ::nsf::methods::object::info::slotobjects {*}[current args]] {
	lappend result [$slot name]
      }
      return $result
    }
    :method "info slot objects" {{-type ::nx::Slot} pattern:optional} {
      return [::nsf::my ::nsf::methods::object::info::slotobjects {*}[current args]]
    }
    # "info properties" is a short form of "info slot definition"
    :alias "info properties"     ::nx::Object::slot::__info::slot::definition
    :alias "info vars"           ::nsf::methods::object::info::vars
  }

  ######################################################################
  # Create the ensemble object for "info" here manually to prevent the
  # replicated definitions from Object.info in Class.info.
  # Potentially, some names are overwritten later by Class.info. Note,
  # that the automatically created name of the sensemble object has to
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
  # Copy all info methods except the subobjects to
  # ::nx::Class::slot::__info
  #
  foreach m [::nsf::directdispatch ::nx::Object::slot::__info ::nsf::methods::object::info::methods] {
    if {[::nsf::directdispatch ::nx::Object::slot::__info ::nsf::methods::object::info::method type $m] eq "object"} continue
    set definition [::nsf::directdispatch ::nx::Object::slot::__info ::nsf::methods::object::info::method definition $m]
    ::nx::Class::slot::__info {*}[lrange $definition 1 end]
    unset definition
  }

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
    :method "info parameter definition" {name:optional} {
      if {[info exists name]} {
	return [::nsf::my ::nsf::methods::class::info::objectparameter parameter $name]
      }
      return [:objectparameter]
    }
    :method "info parameter list" {name:optional} {
      set cmd [list ::nsf::methods::class::info::objectparameter list]
      if {[info exists name]} {lappend cmd $name}
      return [::nsf::my {*}$cmd]
    }
    :method "info parameter names" {name:optional} {
      set cmd [list ::nsf::methods::class::info::objectparameter name]
      if {[info exists name]} {lappend cmd $name}
      return [::nsf::my {*}$cmd]
    }
    :method "info parameter syntax" {name:optional} {
      set cmd [list ::nsf::methods::class::info::objectparameter parametersyntax]
      if {[info exists name]} {lappend cmd $name}
      return [::nsf::my {*}$cmd]
    }
    :method "info slot objects" {{-type ::nx::Slot} -closure:switch -source:optional pattern:optional} {
      set cmd [list ::nsf::methods::class::info::slotobjects -type $type]
      if {[info exists source]} {lappend cmd -source $source}
      if {$closure} {lappend cmd -closure}
      if {[info exists pattern]} {lappend cmd $pattern}
      return [::nsf::my {*}$cmd]
    }
    :method "info slot definition" {{-type ::nx::Slot} -closure:switch -source:optional pattern:optional} {
      set result {}
      foreach slot [::nsf::my ::nsf::methods::class::info::slotobjects {*}[current args]] {
	lappend result [$slot getPropertyDefinition]
      }
      return $result
    }
    :method "info slot names" {{-type ::nx::Slot} -closure:switch -source:optional pattern:optional} {
      set result {}
      foreach slot [::nsf::my ::nsf::methods::class::info::slotobjects {*}[current args]] {
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

  Object protected method "info unknown" {method obj args} {
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
  # Object method abstract {methtype -per-object:switch methname arglist} {
  #   if {$methtype ne "method"} {
  #     error "invalid method type '$methtype', must be 'method'"
  #   }
  #   set body "
  #     if {!\[::nsf::current isnextcall\]} {
  #       error \"Abstract method $methname $arglist called\"
  #     } else {::nsf::next}
  #   "
  #   if {${per-object}} {
  #     :method -per-object $methname $arglist $body
  #   }  else {
  #     :method $methname $arglist $body
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
  # slots, so the code is a little low level. After the defintion of
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
        if {$property in [list "required" "convert" "substdefault" "noarg"]} {
	  if {$property eq "convert" } {set class [:requireClass ::nx::VariableSlot $class]}
          lappend opts -$property 1
        } elseif {$property eq "noaccessor"} {
	  set opt(-accessor) 0
        } elseif {$property eq "noconfig"} {
	  set opt(-config) 0
        } elseif {$property eq "incremental"} {
	  set opt(-accessor) 1
	  lappend opts -incremental 1
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
    {-defaultopts ""}
    spec
    default:optional
  } {

    lassign [:parseParameterSpec -class $class -defaultopts $defaultopts $spec] \
	name parameterOptions class opts
    
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

    if {$class eq ""} {
      set class ::nx::VariableSlot
    } else {
      #puts stderr "*** Class for '$value' is $class"
    }

    #puts stderr "*** [list $class create [::nx::slotObj -container $container $target $name] {*}$opts $initblock]"
    $class create [::nx::slotObj -container $container $target $name] {*}$opts $initblock
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
    
    #puts stderr "Bootstrapslot for $class calls invalidateobjectparameter"
    ::nsf::invalidateobjectparameter $class
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
    set name [namespace tail [self]]
    set prefix [expr {[info exists :positional] && ${:positional} ? "" : "-"}]
    set options [list]
    if {[info exists :default]} {
      if {[string match {*\[*\]*} ${:default}]} {
        append options substdefault
      }
      return [list [list [:namedParameterSpec $prefix $name $options]] ${:default}]
    }
    return [list [:namedParameterSpec $prefix $name $options]]
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
    {accessor false}
    {config true}
    {noarg}
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
    error "Method '$method' unknown for slot [::nsf::self]; valid are: {[lsort $methods]}"
  }

  ObjectParameterSlot protected method init {args} {
    #
    # Provide a default depending on :name for :methodname.  When slot
    # objects are created, invalidate the object parameters to reflect
    # the changes
    #
    if {![info exists :methodname]} {
      set :methodname ${:name}
    }
    if {[::nsf::is class ${:domain}]} {
      ::nsf::invalidateobjectparameter ${:domain}
    }
    #
    # plain object parameter have currently no setter/forwarder
    #
  }

  ObjectParameterSlot public method destroy {} {
    #
    # When slot objects are destroyed, invalidate the object
    # parameters to reflect the changes
    #
    if {[info exists :domain] && ${:domain} ne ""} {
      if {[::nsf::is class ${:domain}]} {
	::nsf::invalidateobjectparameter ${:domain}
      }
      #puts stderr "*** slot destroy of [self], domain ${:domain} per-object ${:per-object}"
      #
      # delete the accessor
      #
      if {${:per-object}} {
	if {[${:domain} ::nsf::methods::object::info::method exists ${:name}]} {
	  ::nsf::method::delete ${:domain} -per-object ${:name}
	}
      } elseif {[${:domain} ::nsf::methods::class::info::method exists ${:name}]} {
	::nsf::method::delete ${:domain} ${:name}
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
    ::nsf::method::forward ${:domain} \
	{*}[expr {${:per-object} ? "-per-object" : ""}] \
	${:name} \
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
      if {[info exists :substdefault] && ${:substdefault}} {
	lappend options substdefault
      }
    }
    if {[info exists :noarg] && ${:noarg}} {lappend options noarg}
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
	set :parameterSpec [list [:namedParameterSpec $prefix ${:name} $options] ${:initcmd}]
	
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
    if {!${:accessor}} {lappend options noaccessor}
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
  ::nsf::invalidateobjectparameter MetaSlot

  ######################################################################
  # Define objectparameter method
  ######################################################################

  Class protected method objectparameter {} {
    # 
    # Collect the object parameter slots in per-position lists to
    # ensure partial ordering and avoid sorting.
    #
    foreach slot [nsf::directdispatch [self] ::nsf::methods::class::info::slotobjects -closure -type ::nx::Slot] {
      lappend defs([$slot position]) [$slot getParameterSpec]
    }
    #
    # Fold the per-position lists into a common list
    # parameterdefinitions, which is the result.
    #
    set parameterdefinitions [list]
    foreach p [lsort [array names defs]] {
      lappend parameterdefinitions {*}$defs($p)
    }
    #puts stderr "*** parameter definition for [::nsf::self]: $parameterdefinitions"
    return $parameterdefinitions
  }
}

namespace eval ::nx {

  ######################################################################
  #  class nx::RelationSlot
  ######################################################################

  MetaSlot create ::nx::RelationSlot
  ::nsf::relation RelationSlot superclass ObjectParameterSlot

  createBootstrapVariableSlots ::nx::RelationSlot {
    {accessor true}
    {multiplicity 1..n}
  }

  RelationSlot protected method init {} {
    ::nsf::next
    if {${:accessor}} {
      :makeForwarder
    }
  }

  #
  # create methods for slot operations assign/get/add/delete
  #
  ::nsf::method::alias RelationSlot assign ::nsf::relation

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

  # Define a temporary, low level interface for defining system slots.
  # The proc is removed later in this script.

  proc register_system_slots {os} {
    #
    # Most system slots are RelationSlots
    #
    ::nx::RelationSlot create ${os}::Object::slot::mixin \
	-forwardername object-mixin -elementtype mixinreg
    ::nx::RelationSlot create ${os}::Object::slot::filter \
	-forwardername object-filter -elementtype filterreg

    ::nx::RelationSlot create ${os}::Class::slot::mixin \
	-forwardername class-mixin -elementtype mixinreg
    ::nx::RelationSlot create ${os}::Class::slot::filter \
        -forwardername class-filter -elementtype filterreg

    #
    # Create two convenience object parameters to allow configuration
    # of per-object mixins and filters for classes.
    #
    ::nx::ObjectParameterSlot create ${os}::Class::slot::object-mixin \
	-methodname "::nsf::classes::nx::Object::mixin" -elementtype mixinreg
    ::nx::ObjectParameterSlot create ${os}::Class::slot::object-filter \
	-methodname "::nsf::classes::nx::Object::filter" -elementtype filterreg

    #
    # Create object parameter slots for "noninit" and "volatile"
    #
    ::nx::ObjectParameterSlot create ${os}::Object::slot::noinit \
	-methodname ::nsf::methods::object::noinit -noarg true
    ::nx::ObjectParameterSlot create ${os}::Object::slot::volatile -noarg true

    #
    # Define "class" as a ObjectParameterSlot defined as alias
    #
    ::nx::ObjectParameterSlot create ${os}::Object::slot::class \
	-methodname "::nsf::methods::object::class" -elementtype class

    #
    # Define "superclass" as a ObjectParameterSlot defined as alias
    #
    ::nx::ObjectParameterSlot create ${os}::Class::slot::superclass \
	-methodname "::nsf::methods::class::superclass" \
	-elementtype class \
	-multiplicity 1..n \
	-default ${os}::Object

    #
    # Define the initcmd as a positional ObjectParameterSlot
    #
#    ::nx::ObjectParameterSlot create ${os}::Object::slot::__init \
#	-disposition alias \
#	-methodname "init" \
#	-noarg true \
#	-positional true \
#	-position 1

    #
    # Define the initcmd as a positional ObjectParameterSlot
    #
    ::nx::ObjectParameterSlot create ${os}::Object::slot::__initcmd \
	-disposition initcmd \
	-positional true \
	-position 2

    #
    # Make sure the invalidate all ObjectParameterSlots
    #
    ::nsf::invalidateobjectparameter ${os}::ObjectParameterSlot

    #
    # Define method "guard" for mixin- and filter-slots of Object and Class
    #
    ${os}::Object::slot::filter method guard {obj prop filter guard:optional} {
      if {[info exists guard]} {
	::nsf::directdispatch $obj ::nsf::methods::object::filterguard $filter $guard
      } else {
	$obj info filter guard $filter 
      }
    }
    ${os}::Class::slot::filter method guard {obj prop filter guard:optional} {
      if {[info exists guard]} {
	::nsf::directdispatch $obj ::nsf::methods::class::filterguard $filter $guard
      } else {
	$obj info filter guard $filter 
      }
    }
    ${os}::Object::slot::mixin method guard {obj prop mixin guard:optional} {
      if {[info exists guard]} {
	::nsf::directdispatch $obj ::nsf::methods::object::mixinguard $mixin $guard
      } else {
	$obj info mixin guard $mixin
      }
    }
    ${os}::Class::slot::mixin method guard {obj prop filter guard:optional} {
      if {[info exists guard]} {
	::nsf::directdispatch $obj ::nsf::methods::class::mixinguard $filter $guard
      } else {
	$obj info mixin guard $filter 
      }
    }
    #::nsf::method::alias ::nx::Class::slot::object-filter guard ${os}::Object::slot::filter::guard
  }


  register_system_slots ::nx

  # remove helper proc
  rename register_system_slots ""
  
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
  ::nsf::invalidateobjectparameter MetaSlot
  
  MetaSlot create ::nx::VariableSlot -superclass ::nx::ObjectParameterSlot

  createBootstrapVariableSlots ::nx::VariableSlot {
    {arg}
    {convert false}
    {incremental}
    {multiplicity 1..1}
    {accessor true}
    {type}

    valuecmd
    defaultcmd
    valuechangedcmd
  }

  ::nx::VariableSlot public method setCheckedInstVar {-nocomplain:switch value} {
    if {[::nsf::var::exists ${:domain} ${:name}] && !$nocomplain} {
      error "Object ${:domain} has already an instance variable named '${:name}'"
    }
    set options [:getParameterOptions -withMultiplicity true]
    if {[llength $options] > 0} {
      ::nsf::is -complain [join $options ,] $value
    }
    ::nsf::var::set ${:domain} ${:name} ${:default}
  }

  ::nx::VariableSlot protected method getParameterOptions {
    {-withMultiplicity 0} 
    {-forObjectParameter 0}
  } {
    set options ""
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
    if {![info exists :incremental]} {return 0}
    #if {![:isMultivalued]} {return 0}
    #puts stderr "[self] ismultivalued"
    return 1
  }

  ::nx::VariableSlot public method makeAccessor {} {

    if {!${:accessor}} {
      #puts stderr "Do not register forwarder ${:domain} ${:name}" 
      return 0
    }   
    if {[:needsForwarder]} {
      set handle [:makeForwarder]
      :makeIncrementalOperations
    } else {
      set handle [:makeSetter]
    }
    ::nsf::method::property ${:domain} \
	{*}[expr {${:per-object} ? "-per-object" : ""}] \
	$handle call-protected \
	[::nsf::dispatch ${:domain} __default_property_call_protection]
    return 1
  }

  ::nx::VariableSlot public method reconfigure {} {
    #puts stderr "*** Should we reconfigure [self]???"
    unset -nocomplain :parameterSpec
    :makeAccessor
    if {${:per-object} && [info exists :default]} {
      :setCheckedInstVar -nocomplain=[info exists :nocomplain] ${:default}
    }
    if {[::nsf::is class ${:domain}]} {
      ::nsf::invalidateobjectparameter ${:domain}
    }
  }

  ::nx::VariableSlot protected method init {} {
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
    if {[llength $options_single] == 0} {
      # No need to make per-slot methods; the general rules on
      # nx::VariableSlot are sufficient
      return
    }
    set options [:getParameterOptions -withMultiplicity true]
    lappend options slot=[::nsf::self]
    set body {::nsf::var::set $obj $var $value}
    
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
      error "Property $prop of [set :domain] ist not multivalued"
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
     {-accessor:switch}
     {-class ""} 
     {-initblock ""} 
     {-nocomplain:switch}
     spec:parameter
     value:optional
   } {
    #
    # This method creates sometimes a slot, sometimes not
    # (optimization).  We need a slot currently in the following
    # situations:
    #  - when accessors are needed 
    #    (serializer uses slot object to create accessors)
    #  - when initblock is non empty
    #

    #puts stderr "Object variable $spec accessor $accessor nocomplain $nocomplain"

    # get name and list of parameter options
    lassign [::nx::MetaSlot parseParameterSpec -class $class $spec] \
	name parameterOptions class options
    array set opts $options

    if {[info exists opts(-incremental)]} {
      # the usage of "-incremental" implies "-accessor"
      set accessor true
    }

    if {$initblock eq "" && !$accessor} {
      #
      # we can build a slot-less variable
      #
      set isSwitch [regsub {\mswitch\M} $parameterOptions boolean parameterOptions]
      if {[info exists value]} {
	if {[info exists :$name] && !$nocomplain} {
	  error "Object [self] has already an instance variable named '$name'"
	}
	if {$parameterOptions ne ""} {
	  #puts stderr "*** ::nsf::is $parameterOptions $value // opts=$options"
	  # we rely here that the nsf::is error message expresses the implementation limits
	  set noptions {}
	  foreach o [split $parameterOptions ,] {
	    if {$o ne "noconfig"} {lappend noptions $o}
	  }
	  set parameterOptions [join $noptions ,]
	  ::nsf::is -complain $parameterOptions $value
	} else {
	  set name $spec
	}
	set :$name $value
      } elseif {$isSwitch} {
	set :$name 0
      } else {
	error "Variable definition for '$name' (without value and accessor) is useless"
      }
      return
    }
    #
    # create variable via a slot object
    #
    set slot [::nx::MetaSlot createFromParameterSpec [self] \
		  -per-object \
		  -class $class \
		  -initblock $initblock \
		  -defaultopts [list -accessor $accessor -config false] \
		  $spec \
		  {*}[expr {[info exists value] ? [list $value] : ""}]]

    if {$nocomplain} {$slot eval {set :nocomplain 1}}
    if {[info exists value]} {$slot setCheckedInstVar -nocomplain=$nocomplain $value}
    return [::nsf::directdispatch [self] ::nsf::methods::object::info::method handle [$slot name]]
  }

  Object method property {
    {-class ""} 
    -nocomplain:switch 
     spec:parameter
    {initblock ""}
  } {
    set r [[self] ::nsf::classes::nx::Object::variable \
	       -accessor=true \
	       -class $class \
	       -initblock $initblock \
	       -nocomplain=$nocomplain \
	       {*}$spec]
    return $r
  }

  nx::Class method variable {
     {-accessor:switch}
     {-class ""}
     {-config:switch}
     {-initblock ""} 
     spec:parameter
     default:optional
   } {
    set slot [::nx::MetaSlot createFromParameterSpec [::nsf::self] \
		  -class $class \
		  -initblock $initblock \
		  -defaultopts [list -accessor $accessor -config $config] \
		  $spec \
		  {*}[expr {[info exists default] ? [list $default] : ""}]]
    return [::nsf::directdispatch [self] ::nsf::methods::class::info::method handle [$slot name]]
  }
  
  nx::Class method property {
    {-class ""}
    spec:parameter
    {initblock ""}
  } {
    set r [[self] ::nsf::classes::nx::Class::variable \
	       -accessor=true \
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
  ::nx::ObjectParameterSlot create ::nx::Class::slot::properties \
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
  # Create a mixin class to overload method "new" such it does not
  # allocate new objects in ::nx::*, but in the specified object
  # (without syntactic overhead).
  ######################################################################

  Class create ::nx::ScopedNew -superclass ::nx::Class {
  
    :property {withclass ::nx::Object}
    :property container

    :protected method init {} {
       :public method new {-childof args} {
	 ::nsf::var::import [::nsf::current class] {container object} withclass
	 if {![::nsf::object::exists $object]} {
	   $withclass create $object
	 }
	 ::nsf::next [list -childof $object {*}$args]
       }
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
    # reused in XOTcl, no "require" there, so use nsf primitiva
    ::nsf::directdispatch $object ::nsf::methods::object::requirenamespace    
    if {$withnew} {
      set m [ScopedNew new -container $object -withclass $class]
      $m volatile
      Class mixin add $m end
      # TODO: the following is not pretty; however, contains might
      # build xotcl and next objects.
      if {[::nsf::is class ::xotcl::Class]} {::xotcl::Class instmixin add $m end}
      #namespace eval $object $cmds
      #::nsf::directdispatch [self] -frame method ::apply [list {} $cmds $object]
      ::apply [list {} $cmds $object]
      Class mixin delete $m
      if {[::nsf::is class ::xotcl::Class]} {::xotcl::Class instmixin delete $m}
    } else {
      #namespace eval $object $cmds
      #::nsf::directdispatch [self] -frame method ::apply [list {} $cmds $object]
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
            $obj configure -superclass [$origin info superclass]
            ::nsf::method::assertion $obj class-invar [::nsf::method::assertion $origin class-invar]
	    ::nsf::relation $obj class-filter [::nsf::relation $origin class-filter]
	    ::nsf::relation $obj class-mixin [::nsf::relation $origin class-mixin]
	    :copyNSVarsAndCmds ::nsf::classes$origin ::nsf::classes$dest
	  }
	  # copy object -> might be a class obj
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
	# Check, if $origin is a slot container. If yes, set the same
	# properties on $dest
	#
	set base [$origin ::nsf::methods::object::info::parent]
	set container [namespace tail $origin]
	if {[::nsf::object::exists $base] 
	    && [::nsf::method::property $base -per-object $container slotcontainer]
	  } {
	  ::nx::internal::setSlotContainerProperties [$dest ::nsf::methods::object::info::parent] $container
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
	  set slots [$origin ::nsf::methods::class::info::slotobjects]
	}
	#
	# append object specific slots
	#
	foreach slot [$origin ::nsf::methods::object::info::slotobjects] {
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
	    $subclass configure -superclass $scl
          }
        }	
      }
      :destroy
    }
  }


  ######################################################################
  # Methods of metaclasses are methods intended for classes. Make
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
    # Set the default method protection for nx methods. This
    # protection level is used per default for definitions of
    # properties and setters
    #
    :method defaultPropertyCallProtection {value:boolean,optional} {
      if {[info exists value]} {
	::nsf::method::create Object __default_property_call_protection args [list return $value]
	::nsf::method::property Object  __default_property_call_protection call-protected true
      }
      return [::nsf::dispatch [::nx::self] __default_property_call_protection]
    }
  }
  #
  # Make the default protected methods
  #
  ::nx::configure defaultMethodCallProtection true
  ::nx::configure defaultPropertyCallProtection false

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
