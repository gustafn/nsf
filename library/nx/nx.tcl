package provide nx 2.0
package require nsf

namespace eval ::nx {
  #
  # By setting the variable bootstrap, we can check later, whether we
  # are in bootstrapping mode
  #
  set bootstrap 1

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
  namespace eval ::nsf {}
  
  namespace import ::nsf::next ::nsf::current

  #
  # provide the standard command set for ::nx::Object
  #
  foreach cmd [info command ::nsf::methods::object::*] {
    set cmdName [namespace tail $cmd]
    if {$cmdName in [list "autoname" "exists" "filterguard" "instvar" "mixinguard" "requirenamespace"]} continue
    ::nsf::alias Object $cmdName $cmd 
  }
  
  # provide ::eval as method for ::nx::Object
  ::nsf::alias Object eval -nonleaf ::eval

  #
  # class methods
  #
  
  # provide the standard command set for Class
  foreach cmd [info command ::nsf::methods::class::*] {
    set cmdName [namespace tail $cmd]
    if {$cmdName in [list "filterguard" "mixinguard"]} continue
    ::nsf::alias Class $cmdName $cmd 
  }

  # set a few aliases as protected
  # "__next", if defined, should be added as well
  foreach cmd [list cleanup noinit residualargs uplevel upvar] {
    ::nsf::methodproperty Object $cmd protected 1
  }

  foreach cmd [list recreate] {
    ::nsf::methodproperty Class $cmd protected 1
  }

  # protect some methods against redefinition
  ::nsf::methodproperty Object destroy redefine-protected true
  ::nsf::methodproperty Class  alloc   redefine-protected true
  ::nsf::methodproperty Class  dealloc redefine-protected true
  ::nsf::methodproperty Class  create  redefine-protected true

  ::nsf::method Object __resolve_method_path {
    -create:switch 
    -per-object:switch 
    -verbose:switch 
    path
  } {
    # TODO: handle -create  (actually, its absence)
    #puts "resolve_method_path"
    set object [::nsf::current object]
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
	    if {![::nsf::isobject ${object}::slot]} {
	      ::nsf::methodproperty $object [Object create ${object}::slot] protected true
	      if {$verbose} {puts stderr "... create object ${object}::slot"}
	    }
	    set o [nx::EnsembleObject create ${object}::slot::__$w]
	    if {$verbose} {puts stderr "... create object $o"}
	    # We are on a class, and have to create an alias to be
	    # accessible for objects
	    ::nsf::alias $object $w $o
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
  ::nsf::methodproperty Object __resolve_method_path protected true

  ::nsf::method Object __default_method_protection args {return false}
  ::nsf::methodproperty Object __default_method_protection protected true


  # define method "method" for Class and Object

  ::nsf::method Class method {
    name arguments body -precondition -postcondition
  } {
    set conditions [list]
    if {[info exists precondition]}  {lappend conditions -precondition  $precondition}
    if {[info exists postcondition]} {lappend conditions -postcondition $postcondition}
    array set "" [:__resolve_method_path -create -verbose $name]
    #puts "class method $(object).$(methodName) [list $arguments] {...}"
    set r [::nsf::method $(object) $(methodName) $arguments $body {*}$conditions]
    if {$r ne ""} {
      # the method was not deleted
      ::nsf::methodproperty $(object) $r protected [::nsf::dispatch $(object) __default_method_protection]
      if {[info exists returns]} {::nsf::methodproperty $(object) $r returns $returns}
    }
    return $r
  }

  ::nsf::method Object method {
    name arguments body -precondition -postcondition -returns
  } {
    set conditions [list]
    if {[info exists precondition]}  {lappend conditions -precondition  $precondition}
    if {[info exists postcondition]} {lappend conditions -postcondition $postcondition}
    array set "" [:__resolve_method_path -create -per-object -verbose $name]
    #puts "object method $(object).$(methodName) [list $arguments] {...}"
    set r [::nsf::method $(object) -per-object $(methodName) $arguments $body {*}$conditions]
    if {$r ne ""} {
      # the method was not deleted
      ::nsf::methodproperty $(object) $r protected [::nsf::dispatch $(object) __default_method_protection]
      if {[info exists returns]} {::nsf::methodproperty $(object) $r returns $returns}
    }
    return $r
  }

  #
  # define method modifiers "class-object", and class level "unknown"
  #
  Class eval {

    # method-modifier for object specific methos
    :method class-object {what args} {
      if {$what in [list "alias" "attribute" "forward" "method" "setter"]} {
        return [::nsf::dispatch [::nsf::current object] ::nsf::classes::nx::Object::$what {*}$args]
      }
      if {$what in [list "info"]} {
        return [::nsf::dispatch [::nsf::current object] ::nx::Object::slot::__info [lindex $args 0] {*}[lrange $args 1 end]]
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
	  0 {return [::nsf::relation [::nsf::current object] object-$what]}
	  1 {return [::nsf::relation [::nsf::current object] object-$what {*}$args]}
	  default {return [::nx::Object::slot::$what [lindex $args 0] \
			       [::nsf::current object] object-$what \
			       {*}[lrange $args 1 end]]
	  }
	}
      }
      if {$what in [list "filterguard" "mixinguard"]} {
        return [::nsf::dispatch [::nsf::current object] ::nsf::methods::object::$what {*}$args]
      }
    }
    # define unknown handler for class
    :method unknown {m args} {
      error "Method '$m' unknown for [::nsf::current object].\
	Consider '[::nsf::current object] create $m $args' instead of '[::nsf::current object] $m $args'"
    }
    # protected is not yet defined
    ::nsf::methodproperty [::nsf::current object] unknown protected true
  }


  Object eval {

    # method modifier "public"
    :method public {args} {
      set p [lsearch -regexp $args {^(method|alias|attribute|forward|setter)$}]
      if {$p == -1} {error "$args is not a method defining method"}
      set r [{*}:$args]
      ::nsf::methodproperty [::nsf::current object] $r protected false
      return $r
    }

    # method modifier "protected"
    :method protected {args} {
      set p [lsearch -regexp $args {^(method|alias|attribute|forward|setter)$}]
      if {$p == -1} {error "$args is not a method defining command"}
      set r [{*}:$args]
      ::nsf::methodproperty [::nsf::current object] $r [::nsf::current method] true
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
    #   error "[::nsf::current object]: unable to dispatch method '$m'"
    # }
    
    # "init" must exist on Object. per default it is empty.
    :protected method init args {}

    # this method is called on calls to object without a specified method
    :protected method defaultmethod {} {::nsf::current object}

    # provide a placeholder for the bootup process. The real definition
    # is based on slots, which are not available at this point.
    :protected method objectparameter {} {;}
  }

  # define forward methods

  ::nsf::forward Object forward ::nsf::forward %self -per-object
  #set ::nsf::signature(::nx::Object-method-forward) {(methodName) obj forward name ?-default default? ?-earlybinding? ?-methodprefix name? ?-objscope? ?-onerror proc? ?-verbose? target ?args?}

  ::nsf::forward Class forward ::nsf::forward %self

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
  # -nonleaf and -objscope make only sense for c-defined cmds,
  # -objscope implies -nonleaf
  #
  Object public method alias {-nonleaf:switch -objscope:switch methodName cmd} {
    array set "" [:__resolve_method_path -per-object -create -verbose $methodName]
    #puts "object alias $(object).$(methodName) $cmd"
    ::nsf::alias $(object) -per-object $(methodName) \
        {*}[expr {${objscope} ? "-objscope" : ""}] \
        {*}[expr {${nonleaf} ? "-nonleaf" : ""}] \
        $cmd
  }

  Class public method alias {-nonleaf:switch -objscope:switch methodName cmd} {
    array set "" [:__resolve_method_path -create -verbose $methodName]
    #puts "class alias $(object).$(methodName) $cmd"
    ::nsf::alias $(object) $(methodName) \
        {*}[expr {${objscope} ? "-objscope" : ""}] \
        {*}[expr {${nonleaf} ? "-nonleaf" : ""}] \
        $cmd
  }

  # Add setter methods. 
  #  
  Object public method setter {methodName} {
    ::nsf::setter [::nsf::current object] -per-object $methodName
  }
  Class public method setter {methodName} {
    ::nsf::setter [::nsf::current object] $methodName
  }

  # Add method "require"
  #
  Object method require {what args} {
    switch -- $what {
      class-object {
	set what [lindex $args 0]
	if {$what eq "method"} {
	  ::nsf::require_method [::nsf::current object] [lindex $args 1] 1
	}
      }
      method {
	::nsf::require_method [::nsf::current object] [lindex $args 0] 0
      }
      namespace {
	::nsf::dispatch [::nsf::current object] ::nsf::methods::object::requirenamespace
      }
    }
  }

  proc ::nx::slotObj {baseObject {name ""}} {
    # Create slot parent object if needed
    set slotParent ${baseObject}::slot
    if {![::nsf::isobject $slotParent]} {
      ::nx::Object alloc $slotParent
      ::nsf::methodproperty ${baseObject} -per-object slot protected true
    }
    if {$name eq ""} {
      return ${slotParent}
    }
    return ${slotParent}::$name
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
    # maintenance methods. We have to be careful ...
    #
    # a) not to interfere between "maintenance methods" and "ensemble
    #    methods" within the maintenance methods. This is achieved
    #    via explicit dispatch commands in the maintenance methods.
    #
    # b) not to overload "maintenance methods" with "ensemble
    #    methods".  This is achieved via the object-method-only policy
    #    (we cannot call "subcmd <subcmdName>" when "subcmdName" is a
    #    method on EnsembleObject) and via a skip object-methods flag
    #    in nsf when calling e.g. "unknwown" (such that a subcmd
    #    "unknown" does not interfere with the method "unknown").
    #
    :method subcmdName {} {
      #
      # Compute the name of a subcmd and the object, on which it is
      # registed, give an Ensemble object.
      #
      set self [::nsf::current object]
      set parent [::nsf::dispatch $self ::nsf::methods::object::info::parent]
      set grandparent [::nsf::dispatch $parent ::nsf::methods::object::info::parent]
      set tail [namespace tail $parent]
      if {$tail eq "slot" && [::nsf::is class $grandparent]} {
	set aliases [::nsf::dispatch $grandparent ::nsf::methods::class::info::methods -methodtype alias]
	foreach alias $aliases {
	  set def [::nsf::dispatch $grandparent ::nsf::methods::class::info::method definition $alias]
	  if {[lindex $def end] eq $self} {
	    return [list name [lindex $def 2] regobj <obj>]
	  }
	}
      } 
      return [list name [namespace tail $self] regobj $parent]
    }
  
    :method methodPath {} {
      #
      # Compute the composite path of a given ensemble object,
      # containing its parent ensemble objects.
      #
      set o [::nsf::current object]
      array set "" [$o ::nsf::classes::nx::EnsembleObject::subcmdName]
      set path $(name)
      while {1} {
	set o [::nsf::dispatch $o ::nsf::methods::object::info::parent]
	if {![::nsf::dispatch $o ::nsf::methods::object::info::hastype ::nx::EnsembleObject]} break
	array set "" [$o ::nsf::classes::nx::EnsembleObject::subcmdName]
	set path "$(name) $path"
      }
      return [list regobj $(regobj) path $path]
    }
    
    :method subMethods {} {
      #
      # Compute pairs of method names and ensemble (sub)objects
      # contained in the current object.
      #
      set result [list]
      set self [::nsf::current object]
      set methods [lsort [::nsf::dispatch $self ::nsf::methods::object::info::methods]]
      array set "" [$self ::nsf::classes::nx::EnsembleObject::subcmdName]
      foreach m $methods {
	set type [::nsf::dispatch $self ::nsf::methods::object::info::method type $m]
	if {$type eq "object"} {
	  foreach {obj submethod} \
	      [::nsf::dispatch ${self}::$m ::nsf::classes::nx::EnsembleObject::subMethods] {
		lappend result $obj $submethod
	      }
	} else {
	  lappend result $self $m
	}
      }
      return $result
    }
    
    #
    # The methods "unknown" and "defaultmethod" are called internally
    #
    :method unknown {m args} {
      set self [::nsf::current object]
      #puts stderr "UNKNOWN [self] $args"
      array set "" [$self ::nsf::classes::nx::EnsembleObject::methodPath]
      set subcmds [lsort [::nsf::dispatch $self ::nsf::methods::object::info::methods]]
      error "unable to dispatch method $(regobj) $(path) $m;\
	valid subcommands of [namespace tail $self]: $subcmds"
    }
    
    :method defaultmethod {} {
      #puts uplevel-method=[uplevel {nx::current method}]-[uplevel nx::self]
      set self [current object]
      set methods [lsort [::nsf::dispatch $self ::nsf::methods::object::info::methods]]
      array set "" [$self ::nsf::classes::nx::EnsembleObject::subcmdName]
      set pairs [$self ::nsf::classes::nx::EnsembleObject::subMethods] 
      foreach {obj m} $pairs {
	array set "" [$obj ::nsf::classes::nx::EnsembleObject::methodPath]
	set cmd [::nsf::dispatch $obj ::nsf::methods::object::info::method parametersyntax $m]
	puts stderr "$(regobj) $(path) $m $cmd"
      }
      return $methods
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
      ::nsf::dispatch [::nsf::current object] \
	  ::nsf::methods::object::info::lookupslots -type ::nx::Slot
    }
    :alias "info children"         ::nsf::methods::object::info::children
    :alias "info class"            ::nsf::methods::object::info::class
    :alias "info filter guard"     ::nsf::methods::object::info::filterguard
    :alias "info filter methods"   ::nsf::methods::object::info::filtermethods
    :alias "info forward"          ::nsf::methods::object::info::forward
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
      set slotContainer [::nsf::current object]::slot
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
  # copy all methods except the subobjects to ::nx::Class::slot::__info
  #
  foreach m [::nsf::dispatch ::nx::Object::slot::__info ::nsf::methods::object::info::methods] {
    if {[::nsf::dispatch ::nx::Object::slot::__info ::nsf::methods::object::info::method type $m] eq "object"} continue
    set definition [::nsf::dispatch ::nx::Object::slot::__info ::nsf::methods::object::info::method definition $m]
    ::nx::Class::slot::__info {*}[lrange $definition 1 end]
  }

  Class eval {
    :alias "info lookup"         ::nx::Object::slot::__info::lookup
    :alias "info filter guard"   ::nsf::methods::class::info::filterguard
    :alias "info filter methods" ::nsf::methods::class::info::filtermethods
    :alias "info forward"        ::nsf::methods::class::info::forward
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

  Object method "info unknown" {method obj args} {
    error "[::nsf::current object] unknown info option \"$method\"; [$obj info info]"
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

  MetaSlot public method createFromParameterSyntax {
    target -per-object:switch 
    {-initblock ""} 
    value default:optional
  } {
    set opts [list]
    set colonPos [string first : $value]
    if {$colonPos == -1} {
      set name $value
    } else {
      set properties [string range $value [expr {$colonPos+1}] end]
      set name [string range $value 0 [expr {$colonPos -1}]]
      foreach property [split $properties ,] {
        if {$property in [list "required" "multivalued" "allowempty" "convert" "nosetter"]} {
          lappend opts -$property 1
        } elseif {[string match type=* $property]} {
          set type [string range $property 5 end]
          if {![string match ::* $type]} {set type ::$type}
        } elseif {[string match arg=* $property]} {
          set argument [string range $property 4 end]
          lappend opts -arg $argument
        } else {
          set type $property
        }
      }
    }
    if {[info exists type]} {
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

    :create [::nx::slotObj $target $name] {*}$opts $initblock
    return [::nsf::dispatch $target ::nsf::methods::${scope}::info::method handle $name]
  }

}
namespace eval ::nx {

  MetaSlot create ::nx::Slot

  MetaSlot create ::nx::ObjectParameterSlot
  ::nsf::relation ObjectParameterSlot superclass Slot
  
  MetaSlot create ::nx::MethodParameterSlot
  ::nsf::relation MethodParameterSlot superclass Slot

  # create an object for dispatching
  MethodParameterSlot create ::nx::methodParameterSlot
  
  # use low level interface for defining slot values. Normally, this is
  # done via slot objects, which are defined later.

  proc createBootstrapAttributeSlots {class definitions} {
    foreach att $definitions {
      if {[llength $att]>1} {foreach {att default} $att break}
      set slotObj [::nx::slotObj $class $att] 
      ::nx::ObjectParameterSlot create $slotObj
      if {[info exists default]} {
        ::nsf::setvar $slotObj default $default
        unset default
      }
      ::nsf::setter $class $att
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
          if {![::nsf::existsvar $i $att]} {
            if {[string match {*\[*\]*} $default]} {
              set value [::nsf::dispatch $i -objscope ::eval subst $default]
            } else {
	      set value $default
	    }
            ::nsf::setvar $i $att $value
          }
        }
        unset default
      }
    }

    #puts stderr "Bootstrapslot for $class calls invalidateobjectparameter"
    ::nsf::invalidateobjectparameter $class
  }

  ############################################
  # Define slots for slots
  ############################################

  createBootstrapAttributeSlots ::nx::Slot {
    {name}
    {multivalued false}
    {required false}
    default
    type
  }

  createBootstrapAttributeSlots ::nx::ObjectParameterSlot {
    {name "[namespace tail [::nsf::current object]]"}
    {methodname}
    {domain "[lindex [regexp -inline {^(.*)::slot::[^:]+$} [::nsf::current object]] 1]"}
    {defaultmethods {get assign}}
    {manager "[::nsf::current object]"}
    {per-object false}
    {nosetter}
  }

  # maybe add the following slots at some later time here
  #   initcmd
  #   valuecmd
  #   valuechangedcmd
  
  ::nsf::alias ObjectParameterSlot get ::nsf::setvar
  ::nsf::alias ObjectParameterSlot assign ::nsf::setvar
  
  ObjectParameterSlot public method add {obj prop value {pos 0}} {
    if {![set :multivalued]} {
      error "Property $prop of [set :domain]->$obj ist not multivalued"
    }
    if {[::nsf::existsvar $obj $prop]} {
      ::nsf::setvar $obj $prop [linsert [::nsf::setvar $obj $prop] $pos $value]
    } else {
      ::nsf::setvar $obj $prop [list $value]
    }
  }

  ObjectParameterSlot public method delete {-nocomplain:switch obj prop value} {
    set old [::nsf::setvar $obj $prop]
    set p [lsearch -glob $old $value]
    if {$p>-1} {::nsf::setvar $obj $prop [lreplace $old $p $p]} else {
      error "$value is not a $prop of $obj (valid are: $old)"
    }
  }
  
  ObjectParameterSlot method unknown {method args} {
    set methods [list]
    foreach m [::nsf::dispatch [::nsf::current object] ::nsf::methods::object::info::lookupmethods] {
      if {[::nsf::dispatch Object ::nsf::methods::object::info::lookupmethods $m] ne ""} continue
      if {[string match __* $m]} continue
      lappend methods $m
    }
    error "Method '$method' unknown for slot [::nsf::current object]; valid are: {[lsort $methods]}"
  }
  
  ObjectParameterSlot public method destroy {} {
    if {${:domain} ne "" && [::nsf::is class ${:domain}]} {
      ::nsf::invalidateobjectparameter ${:domain}
    }
    ::nsf::next
  }
  
  ObjectParameterSlot protected method init {args} {
    if {${:domain} eq ""} {
      set :domain [::nsf::current callingobject]
    }
    if {${:domain} ne ""} {
      if {![info exists :methodname]} {
        set :methodname ${:name}
      }
      if {[::nsf::is class ${:domain}]} {
	::nsf::invalidateobjectparameter ${:domain}
      } 
      if {${:per-object} && [info exists :default] } {
        ::nsf::setvar ${:domain} ${:name} ${:default}
      }
      if {[info exists :nosetter]} {
	#puts stderr "Do not register forwarder ${:domain} ${:name}" 
        return
      }
      #puts stderr "Slot [::nsf::current object] init, forwarder on ${:domain}"
      ::nsf::forward ${:domain} ${:name} \
          ${:manager} \
          [list %1 [${:manager} defaultmethods]] %self \
          ${:methodname}
    }
  }

  #################################################################
  # We have no working objectparameter yet, since it requires a
  # minimal slot infrastructure to build object parameters from
  # slots. The above definitions should be sufficient. We provide the
  # definition here before we refine the slot definitions.
  # 
  # Invalidate previously defined object parameter.

  ::nsf::invalidateobjectparameter MetaSlot

  # Provide the a slot based mechanism for building an object
  # configuration interface from slot definitions

  ObjectParameterSlot public method toParameterSyntax {{name:substdefault ${:name}}} {
    set objparamdefinition $name
    set methodparamdefinition ""
    set objopts [list]
    set methodopts [list]
    set type ""
    if {[info exists :required] && ${:required}} {
      lappend objopts required
      lappend methodopts required
    }
    if {[info exists :type]} {
      if {[string match ::* ${:type}]} {
	set type [expr {[::nsf::is metaclass ${:type}] ? "class" : "object"}]
        lappend objopts type=${:type}
        lappend methodopts type=${:type}
      } else {
        set type ${:type}
      }
    }
    # TODO: remove multivalued check on relations by handling multivalued
    # not in relation, but in the converters
    if {[info exists :multivalued] && ${:multivalued}} {
      if {!([info exists :type] && ${:type} eq "relation")} {
        lappend objopts multivalued
      } else {
        #puts stderr "ignore multivalued for $name in relation"
      }
    }
    if {[info exists :arg]} {
      set prefix [expr {$type eq "object" || $type eq "class" ? "type" : "arg"}]
      lappend objopts $prefix=${:arg}
      lappend methodopts $prefix=${:arg}
    }
    foreach att {convert allowempty} {
      if {[info exists :$att]} {
	lappend objopts $att
	lappend methodopts $att
      }
    }
    if {[info exists :default]} {
      set arg ${:default}
      # deactivated for now: || [string first {$} $arg] > -1
      if {[string match {*\[*\]*} $arg] 
          && $type ne "substdefault"} {
        lappend objopts substdefault
      }
    } elseif {[info exists :initcmd]} {
      set arg ${:initcmd}
      lappend objopts initcmd
    }
    if {[info exists :methodname]} {
      if {${:methodname} ne ${:name}} {
        lappend objopts arg=${:methodname}
        lappend methodopts arg=${:methodname}
        #puts stderr "..... setting arg for methodname: [::nsf::current object] has arg arg=${:methodname}"
      }
    }
    if {$type ne ""} {
      set objopts [linsert $objopts 0 $type]
      # Never add "substdefault" to methodopts, since these are for
      # provided values, not for defaults.
      if {$type ne "substdefault"} {set methodopts [linsert $methodopts 0 $type]}
    }
    lappend objopts slot=[::nsf::current object]

    if {[llength $objopts] > 0} {
      append objparamdefinition :[join $objopts ,]
    }
    if {[llength $methodopts] > 0} {
      set methodparamdefinition [join $methodopts ,]
    }
    if {[info exists arg]} {
      lappend objparamdefinition $arg
    }
    #puts stderr "[::nsf::current method] ${name} returns [list oparam $objparamdefinition mparam $methodparamdefinition]"
    return [list oparam $objparamdefinition mparam $methodparamdefinition]
  }

  proc ::nsf::parametersfromslots {obj} {
    set parameterdefinitions [list]
    foreach slot [::nsf::dispatch $obj ::nsf::methods::object::info::lookupslots -type ::nx::Slot] {
      # Skip some slots for xotcl; 
      # TODO: maybe different parametersfromslots for xotcl?
      if {[::nsf::is class ::xotcl::Object] 
	  && [::nsf::dispatch $obj ::nsf::methods::object::info::hastype ::xotcl::Object] && 
          ([$slot name] eq "mixin" || [$slot name] eq "filter")
	} continue
      array set "" [$slot toParameterSyntax]
      lappend parameterdefinitions -$(oparam)
    }
    return $parameterdefinitions
  }

  Object protected method objectparameter {{lastparameter __initcmd:initcmd,optional}} {
    #puts stderr "... objectparameter [::nsf::current object]"
    set parameterdefinitions [::nsf::parametersfromslots [::nsf::current object]]
    if {[::nsf::is class [::nsf::current object]]} {
      lappend parameterdefinitions -attributes:method,optional
    }
    lappend parameterdefinitions \
        -noinit:method,optional,noarg \
        -volatile:method,optional,noarg \
        {*}$lastparameter
    #puts stderr "*** parameter definition for [::nsf::current object]: $parameterdefinitions"
    return $parameterdefinitions
  }

}
namespace eval ::nx {
  ############################################
  #  RelationSlot
  ############################################
  MetaSlot create ::nx::RelationSlot
  createBootstrapAttributeSlots ::nx::RelationSlot {
    {multivalued true}
    {type relation}
    {elementtype ::nx::Class}
  }

  ::nsf::relation RelationSlot superclass ObjectParameterSlot
  ::nsf::alias RelationSlot assign ::nsf::relation


  RelationSlot protected method init {} {
    if {${:type} ne "relation"} {
      error "RelationSlot requires type == \"relation\""
    }
    ::nsf::next
  }

  RelationSlot protected method delete_value {obj prop old value} {
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
        set value [::nsf::dispatch $value -objscope ::nsf::current object]
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
    if {![set :multivalued]} {
      error "Property $prop of ${:domain}->$obj ist not multivalued"
    }
    set oldSetting [::nsf::relation $obj $prop]
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

    ::nx::RelationSlot create ${os}::Class::slot::superclass
    ::nsf::alias              ${os}::Class::slot::superclass assign ::nsf::relation

    ::nx::RelationSlot create ${os}::Object::slot::class -multivalued false
    ::nsf::alias              ${os}::Object::slot::class assign ::nsf::relation

    ::nx::RelationSlot create ${os}::Object::slot::mixin \
	-methodname object-mixin    

    ::nx::RelationSlot create ${os}::Object::slot::filter -elementtype "" \
	-methodname object-filter


    ::nx::RelationSlot create ${os}::Class::slot::mixin -methodname class-mixin
    
    ::nx::RelationSlot create ${os}::Class::slot::filter -elementtype "" \
        -methodname class-filter

    # Create two conveniance slots to allow configuration of 
    # object-slots for classes via object-mixin
    ::nx::RelationSlot create ${os}::Class::slot::object-mixin -nosetter 1
    ::nx::RelationSlot create ${os}::Class::slot::object-filter -elementtype "" -nosetter 1

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
    #::nsf::alias ::nx::Class::slot::object-filter guard ${os}::Object::slot::filter::guard
  }

  register_system_slots ::nx
  proc ::nx::register_system_slots {} {}


  ############################################
  # Attribute slots
  ############################################
  ::nsf::invalidateobjectparameter MetaSlot
  
  MetaSlot create ::nx::Attribute -superclass ObjectParameterSlot

  createBootstrapAttributeSlots ::nx::Attribute {
    {value_check once}
    incremental
    initcmd
    valuecmd
    valuechangedcmd
    arg
    allowempty
    convert
  }

  Attribute method __default_from_cmd {obj cmd var sub op} {
    #puts "GETVAR [::nsf::current method] obj=$obj cmd=$cmd, var=$var, op=$op"
    $obj trace remove variable $var $op [list [::nsf::current object] [::nsf::current method] $obj $cmd]
    ::nsf::setvar $obj $var [$obj eval $cmd]
  }
  Attribute method __value_from_cmd {obj cmd var sub op} {
    #puts "GETVAR [::nsf::current method] obj=$obj cmd=$cmd, var=$var, op=$op"
    ::nsf::setvar $obj $var [$obj eval $cmd]
  }
  Attribute method __value_changed_cmd {obj cmd var sub op} {
    # puts stderr "**************************"
    # puts "valuechanged obj=$obj cmd=$cmd, var=$var, op=$op, ...\n$obj exists $var -> [::nsf::setvar $obj $var]"
    eval $cmd
  }
  Attribute protected method init {} {
    ::nsf::next ;# do first ordinary slot initialization
    # there might be already default values registered on the class
    set __initcmd ""
    if {[info exists :default]} {
    } elseif [info exists :initcmd] {
      append __initcmd ":trace add variable [list ${:name}] read \
	\[list [::nsf::current object] __default_from_cmd \[::nsf::current object\] [list [set :initcmd]]\]\n"
    } elseif [info exists :valuecmd] {
      append __initcmd ":trace add variable [list ${:name}] read \
	\[list [::nsf::current object] __value_from_cmd \[::nsf::current object\] [list [set :valuecmd]]\]"
    }
    array set "" [:toParameterSyntax ${:name}]

    #puts stderr "Attribute.init valueParam for [::nsf::current object] is $(mparam)"
    if {$(mparam) ne ""} {
      if {[info exists :multivalued] && ${:multivalued}} {
        #puts stderr "adding assign [list obj var value:$(mparam),multivalued] // for [::nsf::current object] with $(mparam)"

	# set variable body to minimize problems with spacing, since
	# the body is literally compared by the slot optimizer.
	set body {::nsf::setvar $obj $var $value}
        :public method assign [list obj var value:$(mparam),multivalued,slot=[::nsf::current object]] \
	    $body

        #puts stderr "adding add method for [::nsf::current object] with value:$(mparam)"
        :public method add [list obj prop value:$(mparam),slot=[::nsf::current object] {pos 0}] {
          ::nsf::next
        }
      } else {
        #puts stderr "SV adding assign [list obj var value:$(mparam)] // for [::nsf::current object] with $(mparam)"
	set body {::nsf::setvar $obj $var $value}
        :public method assign [list obj var value:$(mparam),slot=[::nsf::current object]] $body
      }
    }
    if {[info exists :valuechangedcmd]} {
      append __initcmd ":trace add variable [list ${:name}] write \
	\[list [::nsf::current object] __value_changed_cmd \[::nsf::current object\] [list [set :valuechangedcmd]]\]"
    }
    if {$__initcmd ne ""} {
      set :initcmd $__initcmd
    }
  }

  # mixin class for optimizing slots
  Class create ::nx::Attribute::Optimizer {

    :method method args  {set r [::nsf::next]; :optimize; return $r}
    :method forward args {set r [::nsf::next]; :optimize; return $r}
    :protected method init args {set r [::nsf::next]; :optimize; return $r}

    :public method optimize {} {
      #puts stderr OPTIMIZER-[info exists :incremental]
      if {![info exists :methodname]} {return}
      set object [expr {${:per-object} ? {object} : {}}]
      if {${:per-object}} {
        set perObject -per-object
        set infokind object
      } else {
        set perObject ""
        set infokind class
      }
      if {[::nsf::dispatch ${:domain} ::nsf::methods::${infokind}::info::method handle ${:name}] ne ""} {
        #puts stderr "OPTIMIZER RESETTING ${:domain} slot ${:name}"
        ::nsf::forward ${:domain} {*}$perObject ${:name} \
            ${:manager} \
            [list %1 [${:manager} defaultmethods]] %self \
            ${:methodname}
      }
      #puts "*** stderr OPTIMIZER incremental [info exists :incremental] def '[set :defaultmethods]' nosetter [info exists :nosetter]"
      if {[info exists :incremental] && ${:incremental}} return
      if {[info exists :nosetter]} return
      if {[set :defaultmethods] ne {get assign}} return

      #
      # Check, if the definition of "assign" and "get" are still the
      # defaults. If this is not the case, we cannot replace them with
      # the plain setters.
      # 
      set assignInfo [:info method definition [:info lookup method assign]]
      #puts stderr "OPTIMIZER assign=$assignInfo//[lindex $assignInfo end]//[:info precedence]"
      if {$assignInfo ne "::nx::ObjectParameterSlot alias assign ::nsf::setvar" &&
          [lindex $assignInfo end] ne {::nsf::setvar $obj $var $value} } return
      #if {$assignInfo ne "::nx::ObjectParameterSlot alias assign ::nsf::setvar"} return

      set getInfo [:info method definition [:info lookup method get]]
      if {$getInfo ne "::nx::ObjectParameterSlot alias get ::nsf::setvar"} return

      array set "" [:toParameterSyntax ${:name}]
      if {$(mparam) ne ""} {
        set setterParam [lindex $(oparam) 0]
        #puts stderr "setterParam=$setterParam, op=$(oparam)"
      } else {
        set setterParam ${:name}
      }
      ::nsf::setter ${:domain} {*}$perObject $setterParam
      #puts stderr "::nsf::setter ${:domain} {*}$perObject $setterParam"
    }
  }
  # register the optimizer per default
  Attribute mixin add Attribute::Optimizer

  ############################################
  # Define method "attribute" for convenience
  ############################################
  Class method attribute {spec {-slotclass ::nx::Attribute} {initblock ""}} {
    $slotclass createFromParameterSyntax [::nsf::current object] -initblock $initblock {*}$spec
  }

  Object method attribute {spec {-slotclass ::nx::Attribute} {initblock ""}} {
    $slotclass createFromParameterSyntax [::nsf::current object] -per-object -initblock $initblock {*}$spec
  }

  ############################################
  # Define method "parameter" for backward 
  # compatibility and convenience
  ############################################
  Class public method attributes arglist {
  
    foreach arg $arglist {
      Attribute createFromParameterSyntax [::nsf::current object] {*}$arg
    }
    set slot [::nx::slotObj [::nsf::current object]]
    ::nsf::setvar $slot __parameter $arglist
  }

  Class method "info attributes" {} {
    set slot [::nx::slotObj [::nsf::current object]]
    if {[::nsf::existsvar $slot __parameter]} {
      return [::nsf::setvar $slot __parameter]
    }
    return ""
  }

  ##################################################################
  # now the slots are defined; now we can defines the Objects or 
  # classes with parameters more easily than above.
  ##################################################################

  # remove helper proc
  proc createBootstrapAttributeSlots {} {}

  ##################################################################
  # create user-level converter/checker based on ::nsf primitves
  ##################################################################

  Slot method type=baseclass {name value} {
    # note, that we cannot use "nsf::is baseclass ..." here, since nsf::is call this converter
    if {![::nsf::isobject $value] || ![::nsf::dispatch $value ::nsf::methods::object::info::is baseclass]} {
      error "expected baseclass but got \"$value\" for parameter $name"
    }
    return $value
  }

  Slot method type=metaclass {name value} {
    if {![::nsf::isobject $value] || ![::nsf::dispatch $value ::nsf::methods::object::info::is metaclass]} {
      error "expected metaclass but got \"$value\" for parameter $name"
    }
    return $value
  }

  ##################################################################
  # Create a mixin class to overload method "new" such it does not
  # allocate new objects in ::nx::*, but in the specified object
  # (without syntactic overhead).
  ##################################################################

  Class create ::nx::ScopedNew -superclass Class {
  
    :attribute {withclass ::nx::Object}
    :attribute container

    :protected method init {} {
       :public method new {-childof args} {
	 ::nsf::importvar [::nsf::current class] {container object} withclass
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
    {-class ::nx::Object}
    cmds
  } {
    if {![info exists object]} {set object [::nsf::current object]}
    if {![::nsf::isobject $object]} {$class create $object}
    # reused in XOTcl, no "require" there, so use nsf primitiva
    ::nsf::dispatch $object ::nsf::methods::object::requirenamespace    
    if {$withnew} {
      set m [ScopedNew new -volatile \
		 -container $object -withclass $class]
      Class mixin add $m end
      # TODO: the following is not pretty; however, contains might
      # build xotcl and next objects.
      if {[::nsf::is class ::xotcl::Class]} {::xotcl::Class instmixin add $m end}
      namespace eval $object $cmds
      Class mixin delete $m
      if {[::nsf::is class ::xotcl::Class]} {::xotcl::Class instmixin delete $m}
    } else {
      namespace eval $object $cmds
    }
  }

  # TODO: This is the slots method.... remove it for now.
  #
  #Class forward slots %self contains \
  #    -object {%::nsf::dispatch [::nsf::current object] -objscope ::subst [::nsf::current object]::slot}

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
            ::nsf::assertion $cl class-invar [::nsf::assertion $origin class-invar]
	    ::nsf::relation $cl class-filter [::nsf::relation $origin class-filter]
	    ::nsf::relation $cl class-mixin [::nsf::relation $origin class-mixin]
	    :copyNSVarsAndCmds ::nsf::classes$origin ::nsf::classes$dest
	  } else {
	    # create obj
	    set obj [[$origin info class] create $dest -noinit]
          }
	  # copy object -> may be a class obj
	  ::nsf::assertion $obj check [::nsf::assertion $origin check]
	  ::nsf::assertion $obj object-invar [::nsf::assertion $origin object-invar]
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
	  ::nsf::forward $dest -per-object $i {*}[$origin ::nsf::methods::object::info::forward -definition $i]

	}
	if {[::nsf::is class $origin]} {
	  foreach i [$origin ::nsf::methods::class::info::forward] {
	    ::nsf::forward $dest $i {*}[$origin ::nsf::methods::class::info::forward -definition $i]
	  }
	}
	set traces [list]
	foreach var [$origin info vars] {
	  set cmds [::nsf::dispatch $origin -objscope ::trace info variable $var]
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
      # alter 'domain' and 'manager' in slot objects for classes
      foreach origin [set :targetList] {
	if {[::nsf::is class $origin]} {
	  set dest [:getDest $origin]
	  foreach oldslot [$origin info slots] {
	    set newslot [::nx::slotObj $dest [namespace tail $oldslot]] 
	    if {[$oldslot domain] eq $origin}   {$newslot domain $cl}
	    if {[$oldslot manager] eq $oldslot} {$newslot manager $newslot}
	  }
	}
      }
    }
    
    :public method copy {obj dest} {
      #puts stderr "[::nsf::current object] copy <$obj> <$dest>"
      set :objLength [string length $obj]
      set :dest $dest
      :makeTargetList $obj
      :copyTargets
    }

  }

  Object public method copy newName {
    if {[string compare [string trimleft $newName :] [string trimleft [::nsf::current object] :]]} {
	[CopyHandler new -volatile] copy [::nsf::current object] $newName
    }
  }

  Object public method move newName {
    if {[string trimleft $newName :] ne [string trimleft [::nsf::current object] :]} {
      if {$newName ne ""} {
        :copy $newName
      }
      ### let all subclasses get the copied class as superclass
      if {[::nsf::is class [::nsf::current object]] && $newName ne ""} {
        foreach subclass [:info subclass] {
          set scl [$subclass info superclass]
          if {[set index [lsearch -exact $scl [::nsf::current object]]] != -1} {
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

  #puts stderr Class-methods=[lsort [Class info methods]]
  foreach m [Class info methods] {
    ::nsf::methodproperty Class $m class-only true
  }

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
    # without explicit protection specified.
    #
    :method defaultMethodProtection {value:boolean,optional} {
      if {[info exists value]} {
	::nsf::method Object __default_method_protection args [list return $value]
      }
      return [::nsf::dispatch [::nx::current object] __default_method_protection]
    }
  }
  #
  # Make the default protected methods
  #
  ::nx::configure defaultMethodProtection true

  #
  # Provide an ensemble-like interface to the ::nsf primitiva to
  # access variables. Note that aliasing in the next scripting
  # framework is faster than namespace-ensembles.
  #
  Object create ::nx::var {
    :alias exists ::nsf::existsvar 
    :alias import ::nsf::importvar
    :alias set ::nsf::setvar
  }

  interp alias {} ::nx::self {} ::nsf::current object
}

#######################################################################
# define, what should be exported 
namespace eval ::nx {

  # export the main commands of ::nx
  namespace export Object Class next self current

  set ::nx::confdir ~/.nx
  set ::nx::logdir $::nx::confdir/log
  
  unset bootstrap
}
puts stderr =======NX-done

