set auto_path [concat . $auto_path]

#########################################################################
#
# Implementation of the Tcl Core Object Oriented Language (TclCOOL)
# based on the Next Scripting Framework
#
#########################################################################
#
# This script consists of three major parts:
#
#   1. The definition of TclCOOL (Tcl Core Object Oriented Language)
#      based on the Next Scripting Framework (::nsf). TclCOOL is
#      simple but powerful object language.
#
#   2. Sample TclCOOL program
#

# In a first step, we load nsf

package require nsf

# Now we have the following commands and methods defined in the ::nsf
# namespace (among more functionality not needed here):
#
# Two un-exported commands for OO-language designer
#   ::nsf::alias
#   ::nsf::objectsystem::create
#   ::nsf::forward
#   ::nsf::method
#   ::nsf::relation
#
# Three exported commands to be used by in the languages
#   ::nsf::my
#   ::nsf::current
#   ::nsf::next
#
# An unregistered (unattached) set of methods that can be used for
# objects.  We use here just:
#    instvar
#    lookupmethods
#
# An unattached method can be attached to an object or class by the
# ::nsf::alias
#
#    ::nsf::method::alias class|obj ?-per-object? methodName cmdName
#
# The command registers a command ("cmdName") under a certain name
# ("methodName") to an object or class (1st argument) to make the
# command available as a method.


######################################################################
#
# 1. TclCOOL language definition based on the Next Scripting Framework
#
######################################################################

namespace eval tcl-cool {

  # In a first step, we create two base classes of TclCOOL,
  # namely "object" and "class" in the current namespace:

  ::nsf::objectsystem::create object class

  # We have now the two base classes defined as Tcl commands. Now we
  # can define methods for these newly defined classes
  # (tcl-cool::object and tcl-cool::class).
  #
  # We define as well [self] as a synonym of "nsf::current object"
  #
  interp alias {} ::tcl-cool::self {} ::nsf::current object

  # We define 2 methods for "class" (actually "::tcl-cool::class)
  # based on the method set for classes
  #
  #   - "method"  is a means to define the methods, which are provided
  #               by the class to the instances of the class
  #   - "forward" is a forwarder for instances of the class
  #
  # These methods are defined by the means of ::nsf::forward

  ::nsf::method::forward class method  ::nsf::method::create %self
  ::nsf::method::forward class forward ::nsf::method::forward %self

  # We could have defined the methods "method" and "forward" as well
  # by the means of ::nsf::method, such as
  #
  #   ::nsf::method::create class method {methodName arguments body} {
  #      return [::nsf::method::create [self] $methodName $arguments $body]
  #   }
  #   ::nsf::method::create class forward {methodName args} {
  #     return [::nsf::method::forward [self] $methodName {*}$args]
  #   }
  #
  # Sometimes using method is better to be selective on the arguments
  # and the provided switches.

  # Next, we define 3 methods for "object" (actually "::tcl-cool::object)
  # based on the method set for objects
  #
  #   - "variable" is a means to import instance variables into
  #                the current scope ("instvar" in XOTcl)
  #   - "forward" is a method for delegating calls to different objects
  #   - "methods" is an introspection method for showing the methods of an object
  #
  ::nsf::method::alias   object variable ::nsf::methods::object::instvar
  ::nsf::method::forward object forward  ::nsf::method::forward %self -per-object
  ::nsf::method::alias   object methods  ::nsf::methods::object::info::lookupmethods

  #
  # The method "create" defines, what happens, when a class or object
  # is created. First the object is allocated, then the constructor is called.
  #
  class method create {name args} {
    set obj [::nsf::dispatch [self] ::nsf::methods::class::alloc $name]
    $obj init {*}$args
    return $obj
  }

  # provide primitive commands; we use these from nsf
  namespace import ::nsf::next ::nsf::my

  # a small helper proc for processing the body of the constructor
  proc pop vn {upvar $vn v; set r [lindex $v 0]; set v [lreplace $v 0 0]; return $r}

  # When we create classes without specifying a superclass, we have to
  # choose a default class. This is achieved by setting an instance
  # variable in the meta-class "class" with the name
  # "__default_superclass" to the newly defined object. In XOTcl, this
  # is integrated with the slot objects that provide a uniform
  # interface. Slot objects would need more support from other
  # commands in TclCOOL, so they are left out for the time being....
  #
  # The following method is the constructor for classes.  It sets the
  # default superclass and provides easy means for specifying methods
  # and superclasses during initialization

  class method init {spec} {
    my variable __default_superclass
    set __default_superclass [namespace current]::object
    while {[llength $spec]} {
      set m [pop spec]
      switch $m {
	method     {my method [pop spec] [pop spec] [pop spec]}
	superclass {my superclass [pop spec]}
	default    {error "unknown argument '$m'"}
      }
    }
  }

  # Call the constructor on "class" to set the default superclass as
  # well for this class, and define a convenience routine for defining
  # superclasses
  class init {
    method superclass {sc} {
      ::nsf::relation [self] superclass $sc
    }
  }

  # Finally, we provide a few methods for all objects in TclCOOL:
  #  - "unknown": provide an error message, when unknown methods are called
  #  - "filter": convenience routine to set filters though
  object method unknown {m args} {error "[self]: unknown method '$m' called"}

  # Provide users a convenient way to register/deregister per-object
  # filters and mixins

  object forward filter ::nsf::relation %self object-filter
  object forward mixin  ::nsf::relation %self object-mixin

  # finally, export a few commands
  namespace export object class my self next
}



######################################################################
#
# 3. Sample TclCOOL program
#
######################################################################
namespace import tcl-cool::*

class create dog {
  method init {} {
    tail create [self]::tail
    my forward wag [self]::tail wag
    my forward rise [self]::tail rise
  }
  method bark {} {
    puts "[self] Bark, bark, bark."
  }
}

# we can extend the class incrementally
dog method chase {thing} {
  puts "Chase $thing!"
}

class create tail {
  method init {} {
    my variable length
    set length 5
  }
  method wag {} {
    return Joy
  }
  method rise {} {
    return Attention
  }
}

dog create fido
puts "wag means [fido wag]"
fido chase tweedy!

# The output is:
# wag means Joy
# Chase tweedy!!



puts "\n============ filter ================"

#
# define a filter method....
#
object method tracer args {
  puts "* call [self] [::nsf::current calledmethod] [::nsf::current args]"
  set r [next]
  puts "* exit [self] [::nsf::current calledmethod], returns '$r'"
  return $r
}

#
# ... and register the filter on the object:
#
fido filter tracer

#
# invoke the methods again
#
puts "wag means [fido wag]"
fido chase tweedy!


# The output is:
# > ============ filter ================
# > * call ::fido wag
# > * exit ::fido wag, returns 'Joy'
# > wag means Joy
# > * call ::fido chase tweedy!
# > Chase tweedy!!
# > * exit ::fido chase, returns ''
# > * call ::fido filter {}
# > * exit ::fido filter, returns ''

#
# remove the filter
#
fido filter ""

puts "\n============ mixin class ================"

#
# define a class, which should be mixed in into instances of dogs
#

class create lazydog {
  method wag {} {
    puts "... well, if i have to...."
    next
  }
  method chase thing {
    puts "... [self] does not like to chase $thing"
  }
}

#
# ... and register the filter on the object:
#
fido mixin lazydog

#
# invoke the methods again
#
puts "wag means [fido wag]"
fido chase tweedy!


# The output is:
# > ============ mixin class ================
# > ... well, if i have to....
# > wag means Joy
# > ... ::fido does not like to chase tweedy!


#
# remove the mixin class again
#
fido mixin ""

puts "\n============ subclass ================"

class create terrier {
  superclass dog
  method chase thing {
    puts "[self]: Yippy, I'll get that wicked $thing!"
  }
}

terrier create frido
frido chase tweedy

# The output is:
# > ============ subclass ================
# >Yippy, I'll get that wicked tweedy!!


puts "\nApplication specific methods of fido: [fido methods -source application]"
puts "System specific methods of fido: [fido methods -source baseclasses]"
puts "All methods of fido: [fido methods]\n"
foreach cmd {{fido wag} {fido rise}} {  puts "$cmd [time {eval $cmd} 10000]" }


