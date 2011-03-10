package provide nx 2.0
package require nsf

namespace eval ::nx {
  #
  # By setting the variable bootstrap, we can check later, whether we
  # are in bootstrapping mode
  #
  set ::nsf::bootstrap ::nx

  #
  # First create the ::nx object system. 
  #
  ::nsf::createobjectsystem ::nx::Object ::nx::Class {
    -class.alloc alloc 
    -class.create create
    -class.dealloc dealloc
    -class.recreate recreate 
    -class.requireobject __unknown
    -object.configure configure
    -object.defaultmethod defaultmethod 
    -object.destroy destroy
    -object.init init
    -object.move move 
    -object.objectparameter objectparameter 
    -object.residualargs residualargs
    -object.unknown unknown
  }

  #
  # get frequenly used primitiva from the next scripting framework 
  #
  namespace eval ::nsf {}; # make pkg indexer happy
  namespace import ::nsf::next ::nsf::current ::nsf::self

  #
  # provide the standard command set for ::nx::Object
  #
  foreach cmd [info command ::nsf::methods::object::*] {
    set cmdName [namespace tail $cmd]
    if {$cmdName in [list "autoname" "cleanup" "configure" "exists" \
			 "filterguard" "instvar" "mixinguard" \
			 "noinit" "requirenamespace"]} continue
    ::nsf::method::alias Object $cmdName $cmd 
  }
  
  # provide ::eval as method for ::nx::Object
  ::nsf::method::alias Object eval -frame method ::eval

  #
  # class methods
  #
  
  # provide the standard command set for Class
  foreach cmd [info command ::nsf::methods::class::*] {
    set cmdName [namespace tail $cmd]
    if {$cmdName in [list "filterguard" "mixinguard" "alloc" "dealloc" "recreate"]} continue
    # set tgt [Class ::nsf::methods::class::info::methods -methodtype alias -callprotection all $cmdName]
    #if {$tgt ne "" && [::nsf::method::property Class $cmdName redefine-protected]} {
    #  ::nsf::method::property Class $cmdName redefine-protected false
    #}
    ::nsf::method::alias Class $cmdName $cmd 
    unset cmdName
  }

  # set a few aliases as protected
  # "__next", if defined, should be added as well
  foreach cmd [list residualargs uplevel upvar] {
    ::nsf::method::property Object $cmd call-protected 1
  }

  unset cmd

  # protect some methods against redefinition
  ::nsf::method::property Object destroy redefine-protected true
  #::nsf::method::property Class  alloc   redefine-protected true
  #::nsf::method::property Class  dealloc redefine-protected true
  ::nsf::method::property Class  create  redefine-protected true

  ::nsf::method::provide alloc     {::nsf::method::alias alloc     ::nsf::methods::class::alloc}
  ::nsf::method::provide dealloc   {::nsf::method::alias dealloc   ::nsf::methods::class::dealloc}
  ::nsf::method::provide recreate  {::nsf::method::alias recreate  ::nsf::methods::class::recreate}
  ::nsf::method::provide configure {::nsf::method::alias configure ::nsf::methods::object::configure}

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
    if {[string first " " $path]} {
      set methodName [lindex $path end]
      foreach w [lrange $path 0 end-1] {
	#puts stderr "check $object info methods $path @ <$w>"
	set scope [expr {[::nsf::is class $object] && !${per-object} ? "class" : "object"}] 
	if {[::nsf::dispatch $object ::nsf::methods::${scope}::info::methods $w] eq ""} {
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
	  set type [::nsf::dispatch $object ::nsf::methods::${scope}::info::method type $w]
	  set definition [::nsf::dispatch $object ::nsf::methods::${scope}::info::method definition $w]
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
    return [list object $object methodName $methodName]
  }

  ::nsf::method::property Object __resolve_method_path call-protected true

  ::nsf::method::create Object __default_method_call_protection args {return false}
  ::nsf::method::create Object __default_attribute_call_protection args {return false}

  ::nsf::method::property Object __default_method_call_protection call-protected true
  ::nsf::method::property Object __default_attribute_call_protection call-protected true


  # define method "method" for Class and Object

  ::nsf::method::create Class method {
    name arguments:parameter,0..* -returns body -precondition -postcondition
  } {
    set conditions [list]
    if {[info exists precondition]}  {lappend conditions -precondition  $precondition}
    if {[info exists postcondition]} {lappend conditions -postcondition $postcondition}
    array set "" [:__resolve_method_path $name]
    #puts "class method $(object).$(methodName) [list $arguments] {...}"
    set r [::nsf::method::create $(object) $(methodName) $arguments $body {*}$conditions]
    if {$r ne ""} {
      # the method was not deleted
      ::nsf::method::property $(object) $r call-protected [::nsf::dispatch $(object) __default_method_call_protection]
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
    set r [::nsf::method::create $(object) -per-object $(methodName) $arguments $body {*}$conditions]
    if {$r ne ""} {
      # the method was not deleted
      ::nsf::method::property $(object) $r call-protected [::nsf::dispatch $(object) __default_method_call_protection]
      if {[info exists returns]} {::nsf::method::property $(object) $r returns $returns}
    }
    return $r
  }

  #
  # define method modifiers "class-object", and class level "unknown"
  #
  Class eval {

    # method-modifier for object specific methos
    :method class-object {what args} {
      if {$what in [list "alias" "attribute" "forward" "method"]} {
        return [::nsf::dispatch [::nsf::self] ::nsf::classes::nx::Object::$what {*}$args]
      }
      if {$what in [list "info"]} {
        return [::nsf::dispatch [::nsf::self] ::nx::Object::slot::__info \
		    [lindex $args 0] {*}[lrange $args 1 end]]
      }
      if {$what in [list "filter" "mixin"]} {
	# 
	# It would be much easier, to do a 
	#
	#    return [:object-$what {*}$args]
	#
	# here. However, since we removed "object-mixin" and friends
	# from the registered methods, we have to emulate the work of
	# the forwarder.
	#
	switch [llength $args] {
	  0 {return [::nsf::relation [::nsf::self] object-$what]}
	  1 {return [::nsf::relation [::nsf::self] object-$what {*}$args]}
	  default {return [::nx::Object::slot::$what [lindex $args 0] \
			       [::nsf::self] object-$what \
			       {*}[lrange $args 1 end]]
	  }
	}
      }
      if {$what in [list "filterguard" "mixinguard"]} {
        return [::nsf::dispatch [::nsf::self] ::nsf::methods::object::$what {*}$args]
      }
      error "'$what' not allowed to be modified by 'class-object'"
    }
    # define unknown handler for class
    :method unknown {m args} {
      error "Method '$m' unknown for [::nsf::self].\
	Consider '[::nsf::self] create $m $args' instead of '[::nsf::self] $m $args'"
    }
    # protected is not yet defined
    ::nsf::method::property [::nsf::self] unknown call-protected true
  }

  # Well, class-object is not a method defining method either, but a modifier
  array set ::nsf::methodDefiningMethod {method 1 alias 1 attribute 1 forward 1 class-object 1}

  Object eval {

    # method modifier "public"
    :method public {args} {
      if {![info exists ::nsf::methodDefiningMethod([lindex $args 0])]} {
	error "'[lindex $args 0]' is not a method defining method"
      }
      set r [::nsf::dispatch [::nsf::current object] {*}$args]
      if {$r ne ""} {::nsf::method::property [::nsf::self] $r call-protected false}
      return $r
    }

    # method modifier "protected"
    :method protected {args} {
      if {![info exists ::nsf::methodDefiningMethod([lindex $args 0])]} {
	error "'[lindex $args 0]' is not a method defining method"
      }
      set r [{*}:$args]
      if {$r ne ""} {::nsf::method::property [::nsf::self] $r call-protected true}
      return $r
    }
  }

  Object eval {

    # Default unknown-handler for Object
    #
    # Actually, we do not need this unknown handler, but we could
    # define it as follows:
    #
    # :protected method unknown {m args} {
    #   error "[::nsf::self]: unable to dispatch method '$m'"
    # }
    
    # "init" must exist on Object. per default it is empty.
    :protected method init args {}

    # this method is called on calls to object without a specified method
    :protected method defaultmethod {} {::nsf::self}

    # provide a placeholder for the bootup process. The real definition
    # is based on slots, which are not available at this point.
    :protected method objectparameter {} {;}
  }

  #
  # Define forward methods
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

  #
  # The method __unknown is called in cases, where we try to resolve
  # an unkown class. one could define a custom resolver with this name
  # to load the class on the fly. After the call to __unknown, XOTcl
  # tries to resolve the class again. This meachnism is used e.g. by
  # the ::ttrace mechanism for partial loading by Zoran.
  #

  Class protected class-object method __unknown {name} {}

  # Add alias methods. cmdName for a method can be added via
  #   [... info method handle <methodName>]
  #
  # -frame object|method make only sense for c-defined cmds,
  #
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

  # Add method "require"
  #
  Object method require {what args} {
    switch -- $what {
      class-object {
	set what [lindex $args 0]
	if {$what eq "method"} {
	  ::nsf::method::require [::nsf::self] [lindex $args 1] 1
	}
      }
      method {
	::nsf::method::require [::nsf::self] [lindex $args 0] 0
      }
      namespace {
	::nsf::dispatch [::nsf::self] ::nsf::methods::object::requirenamespace
      }
    }
  }

  #
  # isSlotContainer tests, whether the provided object is a slot
  # container based on the methodproperty slotcontainer, used
  # internally by nsf.
  #
  proc ::nx::isSlotContainer {object} {
    if {[::nsf::isobject $object] && [namespace tail $object] eq "slot"} {
      set parent [$object ::nsf::methods::object::info::parent]
      return [expr {[::nsf::isobject $parent] 
		    && [::nsf::method::property $parent -per-object slot slotcontainer]}]
    }
    return 0
  }

  proc ::nx::slotObj {baseObject {name ""}} {
    # Create slot container object if needed
    set slotContainer ${baseObject}::slot
    if {![::nsf::isobject $slotContainer]} {
      ::nx::Object ::nsf::methods::class::alloc $slotContainer
      ::nsf::method::property ${baseObject} -per-object slot call-protected true
      ::nsf::method::property ${baseObject} -per-object slot redefine-protected true
      ::nsf::method::property ${baseObject} -per-object slot slotcontainer true
      $slotContainer ::nsf::methods::object::requirenamespace
    }
    if {$name eq ""} {
      return ${slotContainer}
    }
    return ${slotContainer}::$name
  }

  # allocate system slot parents
  ::nx::slotObj ::nx::Class
  ::nx::slotObj ::nx::Object

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
    :protected method unknown {obj m args} {
      set path [current methodpath]
      #puts stderr "+++ UNKNOWN obj $obj '$m' $args // path '[current methodpath]'"
      if {[catch {set valid [$obj ::nsf::methods::object::info::lookupmethods -path "$path *"]} errorMsg]} {
	set valid ""
	puts stderr "+++ UNKNOWN raises error $errorMsg"
      }
      set ref "\"[lindex $args 0]\" of $obj $path"
      error "Unable to dispatch sub-method $ref; valid are: [join [lsort $valid] {, }]"
    }
    
    :protected method defaultmethod {} {
      set obj [uplevel {self}]
      set path [current methodpath]
      set l [string length $path]
      set submethods [$obj ::nsf::methods::object::info::lookupmethods -path "$path *"]
      foreach sm $submethods {set results([lindex [string range $sm $l+1 end] 0]) 1}
      error "Valid submethods of $obj $path: [lsort [array names results]]"
    }

    # end of EnsembleObject
  }
  

  ########################
  # Info definition
  ########################

  # we have to use "eval", since objectParameters are not defined yet
  
  Object eval {
    :alias "info lookup filter"  ::nsf::methods::object::info::lookupfilter
    :alias "info lookup method"  ::nsf::methods::object::info::lookupmethod
    :alias "info lookup methods" ::nsf::methods::object::info::lookupmethods
    :method "info lookup slots" {} {
      ::nsf::dispatch [::nsf::self] \
	  ::nsf::methods::object::info::lookupslots -type ::nx::Slot
    }
    :alias "info children"         ::nsf::methods::object::info::children
    :alias "info class"            ::nsf::methods::object::info::class
    :alias "info filter guard"     ::nsf::methods::object::info::filterguard
    :alias "info filter methods"   ::nsf::methods::object::info::filtermethods
    #:alias "info forward"          ::nsf::methods::object::info::forward
    :alias "info has mixin"        ::nsf::methods::object::info::hasmixin
    :alias "info has namespace"    ::nsf::methods::object::info::hasnamespace
    :alias "info has type"         ::nsf::methods::object::info::hastype
    :alias "info is"               ::nsf::methods::object::info::is
    :alias "info methods"          ::nsf::methods::object::info::methods
    :alias "info mixin guard"      ::nsf::methods::object::info::mixinguard
    :alias "info mixin classes"    ::nsf::methods::object::info::mixinclasses
    :alias "info parent"           ::nsf::methods::object::info::parent
    :alias "info precedence"       ::nsf::methods::object::info::precedence
    :method "info slots" {} {
      set slotContainer [::nsf::self]::slot
      if {[::nsf::isobject $slotContainer]} {
	::nsf::dispatch $slotContainer ::nsf::methods::object::info::children -type ::nx::Slot
      }
    }
    :alias "info vars"           ::nsf::methods::object::info::vars
  }

  # Create the ensemble object here to prepare for copy of the above
  # definitions from Object.info to Class.info. Potentially, some
  # names are overwritten later by Class.info. Note, that the
  # automatically created name of the sensemble object has to be the
  # same as defined above.
  
  EnsembleObject create ::nx::Class::slot::__info
  Class alias info ::nx::Class::slot::__info

  #
  # The following test is just for the redefinition case, after a
  # "package forget". We clear "info method" for ::nx::Object to avoid
  # confusions in the copy loop below, which uses method "method".
  #
  if {[::nsf::dispatch ::nx::Object::slot::__info ::nsf::methods::object::info::methods "method"] ne ""} {
    Object method "info method" {} {}
  }

  #
  # Copy all methods except the subobjects to ::nx::Class::slot::__info
  #
  foreach m [::nsf::dispatch ::nx::Object::slot::__info ::nsf::methods::object::info::methods] {
    if {[::nsf::dispatch ::nx::Object::slot::__info ::nsf::methods::object::info::method type $m] eq "object"} continue
    set definition [::nsf::dispatch ::nx::Object::slot::__info ::nsf::methods::object::info::method definition $m]
    ::nx::Class::slot::__info {*}[lrange $definition 1 end]
    unset definition
  }

  Class eval {
    :alias "info lookup"         ::nx::Object::slot::__info::lookup
    :alias "info filter guard"   ::nsf::methods::class::info::filterguard
    :alias "info filter methods" ::nsf::methods::class::info::filtermethods
    #:alias "info forward"        ::nsf::methods::class::info::forward
    :alias "info has"            ::nx::Object::slot::__info::has
    :alias "info heritage"       ::nsf::methods::class::info::heritage
    :alias "info instances"      ::nsf::methods::class::info::instances
    :alias "info methods"        ::nsf::methods::class::info::methods
    :alias "info mixin guard"    ::nsf::methods::class::info::mixinguard
    :alias "info mixin classes"  ::nsf::methods::class::info::mixinclasses
    :alias "info mixinof"        ::nsf::methods::class::info::mixinof
    :alias "info subclass"       ::nsf::methods::class::info::subclass
    :alias "info superclass"     ::nsf::methods::class::info::superclass
  }

  #
  # Define "info info" and unknown
  #
  proc ::nx::infoOptions {obj} {
    #puts stderr "INFO INFO $obj -> '[::nsf::dispatch $obj ::nsf::methods::object::info::methods -methodtype all]'"
    set methods [list]
    foreach name [::nsf::dispatch $obj ::nsf::methods::object::info::methods] {
      if {$name eq "unknown"} continue
      lappend methods $name
    }
    return "valid options are: [join [lsort $methods] {, }]"
  }

  Object protected method "info unknown" {method obj args} {
    error "[::nsf::self] unknown info option \"$method\"; [$obj info info]"
  }

  Object method "info info" {} {::nx::infoOptions ::nx::Object::slot::__info}
  Class  method "info info" {} {::nx::infoOptions ::nx::Class::slot::__info}
  
  # finally register method "method" (otherwise, we cannot use "method" above)
  Object alias "info method" ::nsf::methods::object::info::method
  Class  alias "info method" ::nsf::methods::class::info::method

  #
  # Definition of "abstract method foo ...."
  #
  # Deactivated for now. If we like to revive this method, it should
  # be integrated with the method modifiers and the method "class-object"
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


  ########################################
  # Slot definitions
  ########################################

  #
  # We are in bootstrap code; we cannot use slots/parameter to define
  # slots, so the code is a little low level. After the defintion of
  # the slots, we can use slot-based code such as "-parameter" or
  # "objectparameter".
  #
  Class create ::nx::MetaSlot
  ::nsf::relation MetaSlot superclass Class
  
  MetaSlot class-object method requireClass {required:class old:class,0..1} {
    #
    # Combine two classes and return the more specialized one
    #
    if {$old eq "" || $old eq $required} {return $required}
    if {[$required info superclass -closure $old] ne ""} {
      puts stderr "required $required has $old as superclass => specializing"
      return $required
    } elseif {[$required info subclass -closure $old] ne ""} {
      puts stderr "required $required is more general than $old => keep $old"
      return $old
    } else {
      error "required class $required not compatible with $old"
    }
  }

  MetaSlot public class-object method createFromParameterSpec {
    target 
    -per-object:switch 
    {-class ""}
    {-initblock ""} 
    value
    default:optional
  } {
    set opts [list]
    set colonPos [string first : $value]
    if {$colonPos == -1} {
      set name $value
    } else {
      set properties [string range $value [expr {$colonPos+1}] end]
      set name [string range $value 0 [expr {$colonPos -1}]]
      set useArgFor arg
      foreach property [split $properties ,] {
        if {$property in [list "required" "convert" "nosetter" "substdefault" "noarg"]} {
	  if {$property in "convert" } {
	    set class [:requireClass ::nx::Attribute $class]
	  }
          lappend opts -$property 1
        } elseif {[string match type=* $property]} {
	  set class [:requireClass ::nx::Attribute $class]
          set type [string range $property 5 end]
          if {![string match ::* $type]} {set type ::$type}
        } elseif {[string match arg=* $property]} {
          set argument [string range $property 4 end]
          lappend opts -$useArgFor $argument
        } elseif {$property eq "optional"} {
	  lappend opts -required 0
        } elseif {$property in [list "alias" "forward"]} {
	  set class [:requireClass ::nx::ObjectParameterSlot $class]
	  lappend opts -disposition $property
	  set class [:requireClass ::nx::ObjectParameterSlot $class]
	  set useArgFor methodname
        } elseif {[regexp {([01])[.][.]([1n*])} $property _ minOccurance maxOccurance]} {
	  lappend opts -multiplicity $property
        } else {
          set type $property
        }
      }
    }

    if {[info exists type]} {
      if {$type eq "switch"} {error "switch is not allowed as type for object parameter $name"}
      lappend opts -type $type
    }

    if {[info exists default]} {
      lappend opts -default $default
    }
    if {${per-object}} {
      lappend opts -per-object true
      set scope object
    } else {
      set scope class
    }

    if {$class eq ""} {
      set class ::nx::Attribute
    } else {
      #puts stderr "*** Class for '$value' is $class"
    }
    #puts stderr "*** $class create [::nx::slotObj $target $name] {*}$opts $initblock"
    $class create [::nx::slotObj $target $name] {*}$opts $initblock
    return [::nsf::dispatch $target ::nsf::methods::${scope}::info::method handle $name]
  }

}
namespace eval ::nx {

  MetaSlot create ::nx::Slot

  MetaSlot create ::nx::ObjectParameterSlot
  ::nsf::relation ObjectParameterSlot superclass Slot
  
  MetaSlot create ::nx::MethodParameterSlot
  ::nsf::relation MethodParameterSlot superclass Slot

  # Create an object for dispatching method parameter specific value
  # checkers
  MethodParameterSlot create ::nx::methodParameterSlot
  
  # use low level interface for defining slot values. Normally, this is
  # done via slot objects, which are defined later.

  proc createBootstrapAttributeSlots {class definitions} {
    foreach att $definitions {
      if {[llength $att]>1} {foreach {att default} $att break}
      set slotObj [::nx::slotObj $class $att] 
      #puts stderr "::nx::BootStrapAttributeSlot create $slotObj"
      ::nx::BootStrapAttributeSlot create $slotObj
      if {[info exists default]} {
        ::nsf::var::set $slotObj default $default
        unset default
      }
      ::nsf::method::setter $class $att
    }
    
    #
    # Perform a second round to set default values for already defined
    # slot objects.
    #
    foreach att $definitions {
      if {[llength $att]>1} {foreach {att default} $att break}
      if {[info exists default]} {

        # checking subclasses is not required during bootstrap
        foreach i [::nsf::dispatch $class ::nsf::methods::class::info::instances] {
          if {![::nsf::var::exists $i $att]} {
            if {[string match {*\[*\]*} $default]} {
              set value [::nsf::dispatch $i -frame object ::eval subst $default]
            } else {
	      set value $default
	    }
            ::nsf::var::set $i $att $value
            #puts stderr "::nsf::var::set $i $att $value (second round)"
          }
        }
        unset default
      }
    }

    #puts stderr "Bootstrapslot for $class calls invalidateobjectparameter"
    ::nsf::invalidateobjectparameter $class
  }

  ObjectParameterSlot public method namedParameterSpec {{-prefix -} name options} {
    #
    # Build a pos/nonpos parameter specification from name and option list
    #
    if {[llength $options]>0} {
      return $prefix${name}:[join $options ,]
    } else {
      return $prefix${name}
    }
  }

  ############################################
  # Define slots for slots
  ############################################

  #
  # We would like to have attribute slots during bootstrap to
  # configure the slots itself (e.g. a relation slot object). This is
  # however a chicken/egg problem, so we use a very simple class for
  # defining slots for slots, called BootStrapAttributeSlot.
  #
  MetaSlot create ::nx::BootStrapAttributeSlot
  ::nsf::relation BootStrapAttributeSlot superclass ObjectParameterSlot

  BootStrapAttributeSlot public method getParameterSpec {} {
    # 
    # Bootstrap version of getParameter spec. Just bare essentials.
    #
    set options [list]
    if {[info exists :default]} {
      if {[string match {*\[*\]*} ${:default}]} {
        append options substdefault
      }
      return [list [list [:namedParameterSpec [namespace tail [self]] $options]] ${:default}]
    }
    return [list [:namedParameterSpec [namespace tail [self]] $options]]
  }

  BootStrapAttributeSlot protected method init {args} {
    #
    # Empty constructor; do nothing, intentionally without "next"
    #
  }

  #####################################
  # configure nx::Slot
  #####################################
  createBootstrapAttributeSlots ::nx::Slot {
  }

  #####################################
  # configure nx::ObjectParameterSlot
  #####################################

  createBootstrapAttributeSlots ::nx::ObjectParameterSlot {
    {name "[namespace tail [::nsf::self]]"}
    {domain "[lindex [regexp -inline {^(.*)::slot::[^:]+$} [::nsf::self]] 1]"}
    {manager "[::nsf::self]"}
    {per-object false}
    {methodname}
    {forwardername}
    {defaultmethods {get assign}}
    {nosetter true}
    {noarg}
    {disposition alias}
    {required false}
    {default}
    {initcmd}
    {substdefault false}
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
    foreach m [::nsf::dispatch [::nsf::self] ::nsf::methods::object::info::lookupmethods -source application] {
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
    if {[info exists :domain] && ${:domain} ne "" && [::nsf::is class ${:domain}]} {
      ::nsf::invalidateobjectparameter ${:domain}
    }
    ::nsf::next
  }

  ObjectParameterSlot protected method makeForwarder {} {
    #
    # Build forwarder from the source object class ($domain) to the slot
    # to delegate read and update operations
    #
    # intended to be called on RelationSlot or AttributeSlot
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
  
  ObjectParameterSlot protected method getParameterOptions {{-withMultiplicity 0} {-withSubstdefault 0}} {
    #
    # Obtain a list of parameter options from slot object
    #
    # "-withMultiplicity" is here a dummy parameter, since we have no
    # multiplicty at the level of the ObjectParameter. We want to have
    # the same interface as on Attribute.
    set options ${:disposition}
    if {${:name} ne ${:methodname}} {lappend options arg=${:methodname}}
    if {${:required}} {lappend options required}
    if {$withSubstdefault && [info exists :substdefault] && ${:substdefault}} {
      lappend options substdefault
    }
    if {[info exists :noarg] && ${:noarg}} {lappend options noarg}
    return $options
  }
  
  ObjectParameterSlot public method getParameterSpec {} {
    #
    # Get a full object parmeter specification from slot object
    #
    if {![info exists :parameterSpec]} {
      set options [:getParameterOptions -withMultiplicity true -withSubstdefault true]
      if {[info exists :initcmd]} {
	lappend options initcmd
	set :parameterSpec [list [:namedParameterSpec ${:name} $options] ${:initcmd}]
	
      } elseif {[info exists :default]} {
	# deactivated for now: || [string first {$} ${:default}] > -1
	if {[string match {*\[*\]*} ${:default}]} {
	  lappend options substdefault
	}
	set :parameterSpec  [list [:namedParameterSpec ${:name} $options] ${:default}]
      } else {
	set :parameterSpec  [list [:namedParameterSpec ${:name} $options]]
      }
    }
    return ${:parameterSpec}
  }

  #################################################################
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

  Object protected method objectparameter {{lastparameter __initcmd:initcmd,optional}} {
    #puts stderr "... objectparameter for [::nsf::self]"
    set parameterdefinitions [list]
    foreach slot [nsf::dispatch [self] ::nsf::methods::object::info::lookupslots -type ::nx::Slot] {
      lappend parameterdefinitions [$slot getParameterSpec]
    }
    lappend parameterdefinitions {*}$lastparameter
    #puts stderr "*** parameter definition for [::nsf::self]: $parameterdefinitions"
    return $parameterdefinitions
  }
}

namespace eval ::nx {
  ############################################
  #  class nx::RelationSlot
  ############################################
  MetaSlot create ::nx::RelationSlot
  ::nsf::relation RelationSlot superclass ObjectParameterSlot

  createBootstrapAttributeSlots ::nx::RelationSlot {
    {elementtype ::nx::Class}
    {nosetter false}
  }

  RelationSlot protected method init {} {
    ::nsf::next
    if {!${:nosetter}} {
      :makeForwarder
    }
  }

  #
  # create methods for slot operations assign/get/add/delete
  #
  ::nsf::method::alias RelationSlot assign ::nsf::relation

  RelationSlot protected method delete_value {obj prop old value} {
    #
    # helper method for the delete operation
    #
    if {[string first * $value] > -1 || [string first \[ $value] > -1} {
      # value contains globbing meta characters
      if {${:elementtype} ne "" && ![string match ::* $value]} {
        # prefix glob pattern with ::, since all object names have leading ::
        set value ::$value
      }
      return [lsearch -all -not -glob -inline $old $value]
    } elseif {${:elementtype} ne ""} {
      # value contains no globbing meta characters, but elementtype is given
      if {[string first :: $value] == -1} {
	# get fully qualified name
        if {![::nsf::isobject $value]} {
          error "$value does not appear to be an object"
        }
        set value [::nsf::dispatch $value -frame method ::nsf::self]
      }
      if {![::nsf::is class ${:elementtype}]} {
        error "$value does not appear to be of type ${:elementtype}"
      }
    }
    set p [lsearch -exact $old $value]
    if {$p > -1} {
      return [lreplace $old $p $p]
    } else {
      # In the resulting list might be guards. If so, do another round
      # of checking to test the first list element.
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
    # use uplevel to avoid namespace surprises
    uplevel [list ::nsf::relation $obj $prop [linsert $oldSetting $pos $value]]
  }
  
  RelationSlot public method delete {-nocomplain:switch obj prop value} {
    uplevel [list ::nsf::relation $obj $prop [:delete_value $obj $prop [::nsf::relation $obj $prop] $value]]
  }
 
  ############################################
  # Register system slots
  ############################################
  proc register_system_slots {os} {

    # method "class" is a plain forwarder to relation (no slot)
    ::nsf::method::forward ${os}::Object class ::nsf::relation %self class

    # all other relation cmds are defined as slots

    ::nx::RelationSlot create ${os}::Class::slot::superclass \
	-default ${os}::Object

    ::nx::RelationSlot create ${os}::Object::slot::mixin \
	-forwardername object-mixin
    ::nx::RelationSlot create ${os}::Object::slot::filter -elementtype "" \
	-forwardername object-filter

    ::nx::RelationSlot create ${os}::Class::slot::mixin \
	-forwardername class-mixin
    ::nx::RelationSlot create ${os}::Class::slot::filter -elementtype "" \
        -forwardername class-filter

    #
    # Create two convenience object parameters to allow configuration
    # of per-object mixins and filters for classes.
    #
    ::nx::ObjectParameterSlot create ${os}::Class::slot::object-mixin \
	-methodname "::nsf::classes::nx::Object::mixin"
    ::nx::ObjectParameterSlot create ${os}::Class::slot::object-filter \
	-methodname "::nsf::classes::nx::Object::filter"

    ::nx::ObjectParameterSlot create ${os}::Class::slot::attributes
    ::nx::ObjectParameterSlot create ${os}::Object::slot::noinit \
	-methodname ::nsf::methods::object::noinit -noarg true
    ::nx::ObjectParameterSlot create ${os}::Object::slot::volatile -noarg true

    #
    # Define method "guard" for mixin- and filter-slots of Object and Class
    #
    ${os}::Object::slot::filter method guard {obj prop filter guard:optional} {
      if {[info exists guard]} {
	::nsf::dispatch $obj ::nsf::methods::object::filterguard $filter $guard
      } else {
	$obj info filter guard $filter 
      }
    }
    ${os}::Class::slot::filter method guard {obj prop filter guard:optional} {
      if {[info exists guard]} {
	::nsf::dispatch $obj ::nsf::methods::class::filterguard $filter $guard
      } else {
	$obj info filter guard $filter 
      }
    }
    ${os}::Object::slot::mixin method guard {obj prop filter guard:optional} {
      if {[info exists guard]} {
	::nsf::dispatch $obj ::nsf::methods::object::mixinguard $filter $guard
      } else {
	$obj info mixin guard $filter 
      }
    }
    ${os}::Class::slot::mixin method guard {obj prop filter guard:optional} {
      if {[info exists guard]} {
	::nsf::dispatch $obj ::nsf::methods::class::mixinguard $filter $guard
      } else {
	$obj info mixin guard $filter 
      }
    }
    #::nsf::method::alias ::nx::Class::slot::object-filter guard ${os}::Object::slot::filter::guard
  }

  register_system_slots ::nx
  proc ::nx::register_system_slots {} {}


  ############################################
  # Attribute slots
  ############################################
  ::nsf::invalidateobjectparameter MetaSlot
  
  MetaSlot create ::nx::Attribute -superclass ::nx::ObjectParameterSlot

  createBootstrapAttributeSlots ::nx::Attribute {
    {arg}
    {convert false}
    {incremental}
    {multiplicity 1..1}
    {nosetter false}
    {type}

    valuecmd
    defaultcmd
    valuechangedcmd
  }

  ::nx::Attribute protected method checkInstVar {} {
    if {${:per-object} && [info exists :default] } {
      if {![::nsf::var::exists ${:domain} ${:name}]} {
	::nsf::var::set ${:domain} ${:name} ${:default}
      }
    }
  }

  ::nx::Attribute protected method getParameterOptions {{-withMultiplicity 0} {-withSubstdefault 0}} {
    set options ""
    if {[info exists :type]} {
      if {[string match ::* ${:type}]} {
	lappend options [expr {[::nsf::is metaclass ${:type}] ? "class" : "object"}] type=${:type}
      } else {
	lappend options ${:type}
	if {${:type} ni [list "" "boolean" "integer" "object" "class" \
			     "metaclass" "baseclass" "parameter" \
			     "alnum" "alpha" "ascii" "control" "digit" "double" \
			     "false" "graph" "lower" "print" "punct" "space" "true" \
			     "wideinteger" "wordchar" "xdigit" ]} {
	  lappend options slot=[::nsf::self]
	}
      }
    }
    if {${:required}} {lappend options required}
    if {${:convert}} {lappend options convert}
    if {$withMultiplicity && [info exists :multiplicity] && ${:multiplicity} ne "1..1"} {
      lappend options ${:multiplicity}
    }
    if {$withSubstdefault && [info exists :substdefault] && ${:substdefault}} {
      lappend options substdefault
    }
    #puts stderr "*** getParameterOptions [self] return $options"
    return $options
  }

  ::nx::Attribute protected method isMultivalued {} {
    return [string match {*..[n*]} ${:multiplicity}]
  }

  ::nx::Attribute protected method needsForwarder {} {
    #
    # We just forward, when 
    #   * "assign" and "add" are still untouched, or
    #   * or incremental is specified
    # 
    if {[:info lookup method assign] ne "::nsf::classes::nx::Attribute::assign"} {return 1}
    if {[:info lookup method add] ne "::nsf::classes::nx::Attribute::add"} {return 1}
    if {[:info lookup method get] ne "::nsf::classes::nx::Attribute::get"} {return 1}
    if {![info exists :incremental]} {return 0}
    #if {![:isMultivalued]} {return 0}
    #puts stderr "[self] ismultivalued"
    return 1
  }

  ::nx::Attribute protected method makeAccessor {} {
    if {${:nosetter}} {
      #puts stderr "Do not register forwarder ${:domain} ${:name}" 
      return
    }   
    if {[:needsForwarder]} {
      :makeForwarder
      :makeIncrementalOperations
    } else {
      :makeSetter
    }
  }

  ::nx::Attribute public method reconfigure {} {
    #puts stderr "*** Should we reconfigure [self]???"
    unset -nocomplain :parameterSpec
    :makeAccessor
    ::nsf::invalidateobjectparameter ${:domain}
  }

  ::nx::Attribute protected method init {} {
    next
    :checkInstVar
    :makeAccessor
    :handleTraces
  }

  ::nx::Attribute protected method makeSetter {} {
    set options [:getParameterOptions -withMultiplicity true]
    set setterParam ${:name}
    if {[llength $options]>0} {append setterParam :[join $options ,]}
    #puts stderr [list ::nsf::method::setter ${:domain} {*}[expr {${:per-object} ? "-per-object" : ""}] $setterParam]
    ::nsf::method::setter ${:domain} {*}[expr {${:per-object} ? "-per-object" : ""}] $setterParam
  }

  ::nx::Attribute protected method makeIncrementalOperations {} {
    set options_single [:getParameterOptions]
    if {[llength $options_single] == 0} {
      # No need to make per-slot methods; the general rules on
      # nx::Attribute are sufficient
      return
    }
    set options [:getParameterOptions -withMultiplicity true]
    lappend options slot=[::nsf::self]
    set body {::nsf::var::set $obj $var $value}
    
    if {[:info lookup method assign] eq "::nsf::classes::nx::Attribute::assign"} {
      puts stderr ":public method assign [list obj var [:namedParameterSpec -prefix {} value $options]] $body"
      :public method assign [list obj var [:namedParameterSpec -prefix {} value $options]] $body
    }
    if {[:isMultivalued] && [:info lookup method add] eq "::nsf::classes::nx::Attribute::add"} {
      lappend options_single slot=[::nsf::self]
      puts stderr ":public method add [list obj prop [:namedParameterSpec -prefix {} value $options_single] {pos 0}] {::nsf::next}"
      :public method add [list obj prop [:namedParameterSpec -prefix {} value $options_single] {pos 0}] {::nsf::next}
    } else {
      # TODO should we deactivate add/delete?
    }
  }

  ::nx::Attribute protected method handleTraces {} {
    # essentially like before
    set __initcmd ""
    set trace {::nsf::dispatch [::nsf::self] -frame object ::trace}
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
  # implementation of forwarder operations: assign get add delete
  #
  ::nsf::method::alias Attribute get ::nsf::var::set
  ::nsf::method::alias Attribute assign ::nsf::var::set

  Attribute public method add {obj prop value {pos 0}} {
    if {![:isMultivalued]} {
      puts stderr "... vars [[self] info vars] // [[self] eval {set :multiplicity}]"
      error "Property $prop of [set :domain] ist not multivalued"
    }
    if {[::nsf::var::exists $obj $prop]} {
      ::nsf::var::set $obj $prop [linsert [::nsf::var::set $obj $prop] $pos $value]
    } else {
      ::nsf::var::set $obj $prop [list $value]
    }
  }

  Attribute public method delete {-nocomplain:switch obj prop value} {
    set old [::nsf::var::set $obj $prop]
    set p [lsearch -glob $old $value]
    if {$p>-1} {::nsf::var::set $obj $prop [lreplace $old $p $p]} else {
      error "$value is not a $prop of $obj (valid are: $old)"
    }
  }
  
  #
  # implementation of trace commands
  #
  Attribute method __default_from_cmd {obj cmd var sub op} {
    #puts "GETVAR [::nsf::current method] obj=$obj cmd=$cmd, var=$var, op=$op"
    ::nsf::dispatch $obj -frame object \
	::trace remove variable $var $op [list [::nsf::self] [::nsf::current method] $obj $cmd]
    ::nsf::var::set $obj $var [$obj eval $cmd]
  }
  Attribute method __value_from_cmd {obj cmd var sub op} {
    #puts "GETVAR [::nsf::current method] obj=$obj cmd=$cmd, var=$var, op=$op"
    ::nsf::var::set $obj $var [$obj eval $cmd]
  }
  Attribute method __value_changed_cmd {obj cmd var sub op} {
    # puts stderr "**************************"
    # puts "valuechanged obj=$obj cmd=$cmd, var=$var, op=$op, ...\n$obj exists $var -> [::nsf::var::set $obj $var]"
    eval $cmd
  }

  ##################################################################
  # Define method "attribute" for convenience
  ##################################################################
  Class method attribute {spec {-class ""} {initblock ""}} {
    set r [::nx::MetaSlot createFromParameterSpec [::nsf::self] \
	       -class $class -initblock $initblock {*}$spec]
    if {$r ne ""} {
      set o [::nsf::self]
      ::nsf::method::property $o $r call-protected \
	  [::nsf::dispatch $o __default_attribute_call_protection]
      return $r
    }
  }

  Object method attribute {spec {-class ""} {initblock ""}} {
    set r [::nx::MetaSlot createFromParameterSpec [::nsf::self] \
	       -class $class -per-object -initblock $initblock {*}$spec]
    if {$r ne ""} {
      set o [::nsf::self]
      ::nsf::method::property $o -per-object $r call-protected \
	  [::nsf::dispatch $o __default_attribute_call_protection]
    }
    return $r    
  }

  ##################################################################
  # Define method "attributes" for convenience to define multiple
  # attributes based on a list of parameter specifications.
  ##################################################################
  Class public method attributes arglist {
  
    foreach arg $arglist {
      ::nx::MetaSlot createFromParameterSpec [::nsf::self] {*}$arg
    }
    set slot [::nx::slotObj [::nsf::self]]
    ::nsf::var::set $slot __parameter $arglist
  }

  Class method "info attributes" {} {
    set slot [::nx::slotObj [::nsf::self]]
    if {[::nsf::var::exists $slot __parameter]} {
      return [::nsf::var::set $slot __parameter]
    }
    return ""
  }
  
  ##################################################################
  # Minimal definition of a value checker that permits every value
  # without warnings. The primary purpose of this value checker is to
  # provide a means to specify that the value can have every possible
  # content and not to produce a warning when it might look like a
  # non-positional parameter.
  ##################################################################
  ::nx::Slot method type=any {name value} {
  }

  ##################################################################
  # Now the slots are defined; now we can defines the Objects or 
  # classes with parameters more easily than above.
  ##################################################################

  # remove helper proc
  proc createBootstrapAttributeSlots {} {}

  ##################################################################
  # Create a mixin class to overload method "new" such it does not
  # allocate new objects in ::nx::*, but in the specified object
  # (without syntactic overhead).
  ##################################################################

  Class create ::nx::ScopedNew -superclass ::nx::Class {
  
    :attribute {withclass ::nx::Object}
    :attribute container

    :protected method init {} {
       :public method new {-childof args} {
	 ::nsf::var::import [::nsf::current class] {container object} withclass
	 if {![::nsf::isobject $object]} {
	   $withclass create $object
	 }
	 ::nsf::next [list -childof $object {*}$args]
       }
    }
  }

  ##################################################################
  # The method 'contains' changes the namespace in which objects with
  # relative names are created.  Therefore, 'contains' provides a
  # friendly notation for creating nested object
  # structures. Optionally, creating new objects in the specified
  # scope can be turned off.
  ##################################################################

  Object public method contains {
    {-withnew:boolean true}
    -object
    {-class:class ::nx::Object}
    cmds
  } {
    if {![info exists object]} {set object [::nsf::self]}
    if {![::nsf::isobject $object]} {$class create $object}
    # reused in XOTcl, no "require" there, so use nsf primitiva
    ::nsf::dispatch $object ::nsf::methods::object::requirenamespace    
    if {$withnew} {
      set m [ScopedNew new -container $object -withclass $class]
      $m volatile
      Class mixin add $m end
      # TODO: the following is not pretty; however, contains might
      # build xotcl and next objects.
      if {[::nsf::is class ::xotcl::Class]} {::xotcl::Class instmixin add $m end}
      #namespace eval $object $cmds
      #::nsf::dispatch [self] -frame method ::apply [list {} $cmds $object]
      ::apply [list {} $cmds $object]
      Class mixin delete $m
      if {[::nsf::is class ::xotcl::Class]} {::xotcl::Class instmixin delete $m}
    } else {
      #namespace eval $object $cmds
      #::nsf::dispatch [self] -frame method ::apply [list {} $cmds $object]
      ::apply [list {} $cmds $object]
    }
  }

  ##################################################################
  # copy/move implementation
  ##################################################################

  Class create ::nx::CopyHandler {

    :attribute {targetList ""}
    :attribute {dest ""}
    :attribute objLength

    :method makeTargetList {t} {
      lappend :targetList $t
      #puts stderr "COPY makeTargetList $t target= ${:targetList}"
      # if it is an object without namespace, it is a leaf
      if {[::nsf::isobject $t]} {
	if {[::nsf::dispatch $t ::nsf::methods::object::info::hasnamespace]} {
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
        if {![::nsf::isobject $c]} {
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
    :method getDest origin {
      set tail [string range $origin [set :objLength] end]
      return ::[string trimleft [set :dest]$tail :]
    }
  
    :method copyTargets {} {
      #puts stderr "COPY will copy targetList = [set :targetList]"
      foreach origin [set :targetList] {
        set dest [:getDest $origin]
        if {[::nsf::isobject $origin]} {
          # copy class information
          if {[::nsf::is class $origin]} {
            set cl [[$origin info class] create $dest -noinit]
            # class object
            set obj $cl
            $cl superclass [$origin info superclass]
            ::nsf::method::assertion $cl class-invar [::nsf::method::assertion $origin class-invar]
	    ::nsf::relation $cl class-filter [::nsf::relation $origin class-filter]
	    ::nsf::relation $cl class-mixin [::nsf::relation $origin class-mixin]
	    :copyNSVarsAndCmds ::nsf::classes$origin ::nsf::classes$dest
	  } else {
	    # create obj
	    set obj [[$origin info class] create $dest -noinit]
          }
	  # copy object -> may be a class obj
	  ::nsf::method::assertion $obj check [::nsf::method::assertion $origin check]
	  ::nsf::method::assertion $obj object-invar [::nsf::method::assertion $origin object-invar]
	  ::nsf::relation $obj object-filter [::nsf::relation $origin object-filter]
	  ::nsf::relation $obj object-mixin [::nsf::relation $origin object-mixin]
            # reused in XOTcl, no "require" there, so use nsf primitiva
	  if {[::nsf::dispatch $origin ::nsf::methods::object::info::hasnamespace]} {
	    ::nsf::dispatch $obj ::nsf::methods::object::requirenamespace
	  }
	} else {
	  namespace eval $dest {}
	}
	:copyNSVarsAndCmds $origin $dest
	foreach i [$origin ::nsf::methods::object::info::forward] {
	  ::nsf::method::forward $dest -per-object $i {*}[$origin ::nsf::methods::object::info::forward -definition $i]

	}
	if {[::nsf::is class $origin]} {
	  foreach i [$origin ::nsf::methods::class::info::forward] {
	    ::nsf::method::forward $dest $i {*}[$origin ::nsf::methods::class::info::forward -definition $i]
	  }
	}
	set traces [list]
	foreach var [$origin info vars] {
	  set cmds [::nsf::dispatch $origin -frame object ::trace info variable $var]
	  if {$cmds ne ""} {
	    foreach cmd $cmds {
	      foreach {op def} $cmd break
	      #$origin trace remove variable $var $op $def
	      set domain [lindex $def 0]
	      if {$domain eq $origin} {
		set def [concat $dest [lrange $def 1 end]]
	      }
	      if {[::nsf::isobject $domain] && [$domain info has type ::nx::Slot]} {
		# slot traces are handled already by the slot mechanism
		continue
	      }
	      $dest trace add variable $var $op $def
	    }
	  }
	}
	#puts stderr "====="
      }
      # alter 'domain' and 'manager' in slot objects
      foreach origin [set :targetList] {
	set dest [:getDest $origin]
	foreach oldslot [$origin info slots] {
	  set newslot [::nx::slotObj $dest [namespace tail $oldslot]]
	  if {[$oldslot domain] eq $origin}   {$newslot domain $dest}
	  if {[$oldslot manager] eq $oldslot} {$newslot manager $newslot}
	  $newslot eval :init
	}
      }
    }
    
    :public method copy {obj dest} {
      #puts stderr "[::nsf::self] copy <$obj> <$dest>"
      set :objLength [string length $obj]
      set :dest $dest
      :makeTargetList $obj
      :copyTargets
    }

  }

  Object public method copy newName {
    if {[string compare [string trimleft $newName :] [string trimleft [::nsf::self] :]]} {
      [CopyHandler new -volatile] copy [::nsf::self] $newName
    }
  }

  Object public method move newName {
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
	    $subclass superclass $scl
          }
        }	
      }
      :destroy
    }
  }


  #######################################################
  # Methods of metaclasses are methods intended for 
  # classes. Make sure, these methods are only applied 
  # on classes.
  #######################################################

  foreach m [Class info methods] {
    ::nsf::method::property Class $m class-only true
  }
  unset m

  #######################################################
  # some utilities
  #######################################################

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
    # attributes and setters
    #
    :method defaultAttributeCallProtection {value:boolean,optional} {
      if {[info exists value]} {
	::nsf::method::create Object __default_attribute_call_protection args [list return $value]
	::nsf::method::property Object  __default_attribute_call_protection call-protected true
      }
      return [::nsf::dispatch [::nx::self] __default_attribute_call_protection]
    }
  }
  #
  # Make the default protected methods
  #
  ::nx::configure defaultMethodCallProtection true
  ::nx::configure defaultAttributeCallProtection false

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

#######################################################################
# define, what should be exported 
namespace eval ::nx {

  # export the main commands of ::nx
  namespace export Object Class next self current

  set ::nx::confdir ~/.nx
  set ::nx::logdir $::nx::confdir/log
  
  unset ::nsf::bootstrap
}
if {[::nsf::configure debug] > 1} {
  foreach ns {::nsf ::nx} {
    puts "vars of $ns: [info vars ${ns}::*]"
    puts stderr "$ns exports: [namespace eval $ns {lsort [namespace export]}]"
  }
  puts stderr "======= nx loaded"
}
