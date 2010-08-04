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

? {::nx::core::objectproperty Object object} 1
? {::nx::core::objectproperty Object class} 1
? {::nx::core::objectproperty Object metaclass} 0
? {Object info superclass} ""
? {Object info class} ::nx::Class

? {::nx::core::objectproperty Class object} 1
? {::nx::core::objectproperty Class class} 1
? {::nx::core::objectproperty Class metaclass} 1
? {Class info superclass} ::nx::Object
? {Class info class} ::nx::Class


Object create o
? {::nx::core::objectproperty Object object} 1
? {::nx::core::objectproperty o class} 0
? {::nx::core::objectproperty o metaclass} 0
? {o info class} ::nx::Object
? {Object info instances o} ::o
? {Object info instances ::o} ::o

Class create C0
? {::nx::core::objectproperty C0 class} 1
? {::nx::core::objectproperty C0 metaclass} 0
? {C0 info superclass} ::nx::Object
? {C0 info class} ::nx::Class
#? {lsort [Class info vars]} "__default_metaclass __default_superclass"

Class create M -superclass ::nx::Class
? {::nx::core::objectproperty M object} 1
? {::nx::core::objectproperty M class} 1
? {::nx::core::objectproperty M metaclass} 1
? {M info superclass} ::nx::Class
? {M info class} ::nx::Class

M create C
? {::nx::core::objectproperty C object} 1
? {::nx::core::objectproperty C class} 1
? {::nx::core::objectproperty C metaclass} 0
? {C info superclass} ::nx::Object
? {C info class} ::M

C create c1
? {::nx::core::objectproperty c1 object} 1
? {::nx::core::objectproperty c1 class} 0
? {::nx::core::objectproperty c1 metaclass} 0
? {c1 info class} ::C

Class create M2 -superclass M
? {::nx::core::objectproperty M2 object} 1
? {::nx::core::objectproperty M2 class} 1
? {::nx::core::objectproperty M2 metaclass} 1
? {M2 info superclass} ::M
? {M2 info class} ::nx::Class

M2 create m2
? {m2 info superclass} ::nx::Object
? {m2 info class} ::M2

# destroy meta-class M, reclass meta-class instances to the base
# meta-class and set subclass of M to the root meta-class
M destroy
? {::nx::core::objectproperty C object} 1
? {::nx::core::objectproperty C class} 1
? {::nx::core::objectproperty C metaclass} 0
? {C info superclass} ::nx::Object
? {C info class} ::nx::Class

? {::nx::core::objectproperty M2 metaclass} 1
? {M2 info superclass} ::nx::Class
? {m2 info superclass} ::nx::Object
? {m2 info class} ::M2


# destroy class M, reclass class instances to the base class
C destroy
? {::nx::core::objectproperty c1 objec} 1
? {::nx::core::objectproperty c1 class} 0
? {::nx::core::objectproperty c1 metaclass} 0
? {c1 info class} ::nx::Object

# basic parameter tests

Class create C -parameter {{x 1} {y 2}}
? {::nx::core::objectproperty C object} 1
? {::nx::core::objectproperty C::slot object} 1
? {C info children} ::C::slot


C copy X
? {::nx::core::objectproperty X object} 1
? {X info vars} ""
? {C info vars} ""
? {::nx::core::objectproperty X::slot object} 1

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
::nx::core::dispatch o eval set :x 1
? {o info vars} x "simple dispatch has set variable x"
? {::nx::var set o x} 1 "simple dispatch has set variable x to 1"
? {::nx::core::dispatch o foo} "goo" "simple dispatch with one arg works"
? {::nx::core::dispatch o bar 1} "goo-1" "simple dispatch with two args works"
o destroy

# dispatch with colon names
Object create o {set :x 1}
::nx::core::dispatch ::o ::incr x
? {o eval {set :x}} 1 "cmd dispatch without -objscope did not modify the instance variable"
::nx::core::dispatch ::o -objscope ::incr x
? {o eval {set :x}} 2 "cmd dispatch -objscope modifies the instance variable"
? {catch {::nx::core::dispatch ::o -objscope ::xxx x}} 1 "cmd dispatch with unknown command"
o destroy

puts stderr ===MINI-OBJECTSYSTEM
# test object system
# create a minimal object system without internally dipatched methods
::nx::core::createobjectsystem ::object ::class

? {::nx::core::objectproperty ::object object} 1
? {::nx::core::objectproperty ::object class} 1
? {::nx::core::objectproperty ::object metaclass} 0
? {::nx::core::relation ::object class} ::class
? {::nx::core::relation ::object superclass} ""

? {::nx::core::objectproperty ::class object} 1
? {::nx::core::objectproperty ::class class} 1
? {::nx::core::objectproperty ::class metaclass} 1
? {::nx::core::relation ::class class} ::class
? {::nx::core::relation ::class superclass} ::object

# define non-standard methos to create/destroy objects and classes
::nx::core::alias ::class  + ::nx::core::cmd::Class::create
::nx::core::alias ::object - ::nx::core::cmd::Object::destroy

# create a class named C
::class + C

? {::nx::core::objectproperty ::C object} 1
? {::nx::core::objectproperty ::C class} 1
? {::nx::core::objectproperty ::C metaclass} 0
? {::nx::core::relation ::C class} ::class
? {::nx::core::relation ::C superclass} ::object

# create an instance of C
C + c1

? {::nx::core::objectproperty ::c1 object} 1
? {::nx::core::objectproperty ::c1 class} 0
? {::nx::core::objectproperty ::c1 metaclass} 0
? {::nx::core::relation ::c1 class} ::C

# destroy instance and class
c1 -

? {::nx::core::objectproperty ::c1 object} 0
? {::nx::core::objectproperty ::C class} 1

C -

? {::nx::core::objectproperty ::C class} 0

::nx::Class create ::C

? {catch {::nx::core::objectproperty ::C type ::UNKNOWN}} 1
? {catch {::C info is type ::xyz::Bar}} 1
? {catch {::nx::core::objectproperty ::CCCC type ::nx::Object}} 1

::C destroy

puts stderr ===EXIT
