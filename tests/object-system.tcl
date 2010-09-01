package require nx
namespace import nx::*
#
# Basic tests of the object system, should not require Class Test,
# since even class Test might not work at that time.
#
proc ? {cmd expected {msg ""}} {
   #puts "??? $cmd"                 
   set r [uplevel $cmd]
   if {$msg eq ""} {set msg $cmd}
   if {$r ne $expected} {
     puts stderr "ERROR $msg returned '$r' ne '$expected'"
     error "FAILED $msg returned '$r' ne '$expected'"
   } else {
     puts stderr "OK $msg"
   }
}

? {::nsf::isobject Object} 1
? {::nsf::objectproperty class Object} 1
? {::nsf::objectproperty metaclass Object} 0
? {Object info superclass} ""
? {Object info class} ::nx::Class

? {::nsf::isobject Class} 1
? {::nsf::objectproperty class Class} 1
? {::nsf::objectproperty metaclass Class} 1
? {Class info superclass} ::nx::Object
? {Class info class} ::nx::Class


Object create o
? {::nsf::isobject Object} 1
? {::nsf::objectproperty class o} 0
? {::nsf::objectproperty metaclass o} 0
? {o info class} ::nx::Object
? {Object info instances o} ::o
? {Object info instances ::o} ::o

Class create C0
? {::nsf::objectproperty class C0} 1
? {::nsf::objectproperty metaclass C0} 0
? {C0 info superclass} ::nx::Object
? {C0 info class} ::nx::Class
#? {lsort [Class info vars]} "__default_metaclass __default_superclass"

Class create M -superclass ::nx::Class
? {::nsf::isobject M} 1
? {::nsf::objectproperty class M} 1
? {::nsf::objectproperty metaclass M} 1
? {M info superclass} ::nx::Class
? {M info class} ::nx::Class

M create C
? {::nsf::isobject C} 1
? {::nsf::objectproperty class C} 1
? {::nsf::objectproperty metaclass C} 0
? {C info superclass} ::nx::Object
? {C info class} ::M

C create c1
? {::nsf::isobject c1} 1
? {::nsf::objectproperty class c1} 0
? {::nsf::objectproperty metaclass c1} 0
? {c1 info class} ::C

Class create M2 -superclass M
? {::nsf::isobject M2} 1
? {::nsf::objectproperty class M2} 1
? {::nsf::objectproperty metaclass M2} 1
? {M2 info superclass} ::M
? {M2 info class} ::nx::Class

M2 create m2
? {m2 info superclass} ::nx::Object
? {m2 info class} ::M2

# destroy meta-class M, reclass meta-class instances to the base
# meta-class and set subclass of M to the root meta-class
M destroy
? {::nsf::isobject C} 1
? {::nsf::objectproperty class C} 1
? {::nsf::objectproperty metaclass C} 0
? {C info superclass} ::nx::Object
? {C info class} ::nx::Class

? {::nsf::objectproperty metaclass M2} 1
? {M2 info superclass} ::nx::Class
? {m2 info superclass} ::nx::Object
? {m2 info class} ::M2


# destroy class M, reclass class instances to the base class
C destroy
? {::nsf::isobject c1} 1
? {::nsf::objectproperty object c1} 1
? {::nsf::objectproperty class c1} 0
? {::nsf::objectproperty metaclass c1} 0
? {c1 info class} ::nx::Object

# basic parameter tests

Class create C -parameter {{x 1} {y 2}}
? {::nsf::isobject C} 1
? {::nsf::isobject C::slot} 1
? {C info children} ::C::slot


C copy X
? {::nsf::isobject X} 1
? {X info vars} ""
? {C info vars} ""
? {::nsf::isobject X::slot} 1

#? {C::slot info vars} __parameter
? {C info parameter} {{x 1} {y 2}}

#? {X::slot info vars} __parameter
? {X info parameter} {{x 1} {y 2}}

#
# tests for the dispatch command
 
Object create o
o method foo {} {return goo}
o method bar {x} {return goo-$x}

# dispatch without colon names
::nsf::dispatch o eval set :x 1
? {o info vars} x "simple dispatch has set variable x"
? {::nx::var set o x} 1 "simple dispatch has set variable x to 1"
? {::nsf::dispatch o foo} "goo" "simple dispatch with one arg works"
? {::nsf::dispatch o bar 1} "goo-1" "simple dispatch with two args works"
o destroy

# dispatch with colon names
Object create o {set :x 1}
::nsf::dispatch ::o ::incr x
? {o eval {set :x}} 1 "cmd dispatch without -objscope did not modify the instance variable"
::nsf::dispatch ::o -objscope ::incr x
? {o eval {set :x}} 2 "cmd dispatch -objscope modifies the instance variable"
? {catch {::nsf::dispatch ::o -objscope ::xxx x}} 1 "cmd dispatch with unknown command"
o destroy

puts stderr ===MINI-OBJECTSYSTEM
# test object system
# create a minimal object system without internally dipatched methods
::nsf::createobjectsystem ::object ::class

? {::nsf::isobject ::object} 1
? {::nsf::objectproperty class ::object} 1
? {::nsf::objectproperty metaclass ::object} 0
? {::nsf::relation ::object class} ::class
? {::nsf::relation ::object superclass} ""

? {::nsf::isobject ::class} 1
? {::nsf::objectproperty class ::class} 1
? {::nsf::objectproperty metaclass ::class} 1
? {::nsf::relation ::class class} ::class
? {::nsf::relation ::class superclass} ::object

# define non-standard methos to create/destroy objects and classes
::nsf::alias ::class  + ::nsf::cmd::Class::create
::nsf::alias ::object - ::nsf::cmd::Object::destroy

# create a class named C
::class + C

? {::nsf::isobject ::C} 1
? {::nsf::objectproperty class ::C} 1
? {::nsf::objectproperty metaclass ::C} 0
? {::nsf::relation ::C class} ::class
? {::nsf::relation ::C superclass} ::object

# create an instance of C
C + c1

? {::nsf::isobject ::c1} 1
? {::nsf::objectproperty class ::c1} 0
? {::nsf::objectproperty metaclass ::c1} 0
? {::nsf::relation ::c1 class} ::C

# destroy instance and class
c1 -

? {::nsf::isobject ::c1} 0
? {::nsf::objectproperty class ::C} 1

C -

? {::nsf::objectproperty class ::C} 0

::nx::Class create ::C

? {catch {::C info has type ::UNKNOWN}} 1
? {catch {::C info has type ::xyz::Bar}} 1
#? {catch {::nsf::objectproperty type ::CCCC ::nx::Object}} 1

::C destroy

puts stderr ===EXIT
