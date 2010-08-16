package require nx
package require nx::test
namespace import ::nx::*

Test case dummy {
  puts current=[::namespace current]
  set o [Object create o]
  puts o=$o

  ? {::nsf::objectproperty ::o object} 1
}
? {::nsf::objectproperty ::o object} 0 
#exit

#######################################################
# parametercheck
#######################################################
Test parameter count 10000
Test case parametercheck {

  Object create o1
  Class create C -parameter {a {b:boolean} {c 1}}
  C create c1 
  Class create M
  c1 mixin M

  ? {::nsf::parametercheck object o1} 1
  ? {::nsf::parametercheck integer 1} 1

  ? {::nsf::objectproperty o1 object} 1
  ? {::nsf::objectproperty c1 type C} 1

  ? {::nsf::is c1 object -type C} 1
  ? {::nsf::is c1 object -hasmixin M -type C} 1
  ? {::nsf::is c1 object -hasmixin M1 -type C} 0
  ? {::nsf::is c1 object -hasmixin M -type C0} 0
  ? {::nsf::is o1 object} 1
  ? {::nsf::is 1 integer} 1
  ? {::nsf::is c1 type C} 1
  ? {::nsf::is o type C} 0
  ? {::nsf::is o object -type C} 0
  ? {::nsf::is o object -hasmixin C} 0
#exit
  ? {::nsf::parametercheck class o1} {expected class but got "o1" for parameter value}
  ? {::nsf::parametercheck -nocomplain class o1} 0
  ? {::nsf::parametercheck class Test} 1
  ? {::nsf::parametercheck object,multivalued [list o1 Test]} 1

  ? {::nsf::parametercheck integer,multivalued [list 1 2 3]} 1
  ? {::nsf::parametercheck integer,multivalued [list 1 2 3 a]} \
      {invalid value in "1 2 3 a": expected integer but got "a" for parameter value}
  ? {::nsf::parametercheck object,type=::C c1} 1
  ? {::nsf::parametercheck object,type=::C o} \
      {expected object but got "o" for parameter value} \
      "object, but different type"
  ? {::nsf::parametercheck object,type=::C c} \
      {expected object but got "c" for parameter value} \
      "no object"
  ? {::nsf::parametercheck object,type=::nx::Object c1} 1 "general type"
  
  # do not allow "currently unknown" user defined types in parametercheck
  ? {::nsf::parametercheck in1 aaa} {invalid value constraints "in1"}
  
  ? {::nsf::parametercheck lower c} 1 "lower case char"
  ? {::nsf::parametercheck lower abc} 1 "lower case chars"
  ? {::nsf::parametercheck lower Abc} {expected lower but got "Abc" for parameter value} 
  ? {string is lower abc} 1 "tcl command 'string is lower'"
  
  ? {::nsf::parametercheck {i:integer 1} 2} {invalid value constraints "i:integer 1"}
}

#######################################################
# parametercheck
#######################################################
Test parameter count 10000
Test case parametercheck {

  Object create ::paramManager {
    :method type=sex {name value} {
      return "agamous"
    }
  }
  
  ? {::nsf::parametercheck sex,slot=::paramManager female} "1"
}
#######################################################
# cononical feature table
#######################################################
#
# parameter options
#   required
#   optional
#   multivalued
#   noarg
#   arg=
#   substdefault:  if no value given, subst on default (result is substituted value); 
#                  susbt cmd can use variable resolvers,
#                  works for scripted/c-methods and obj-parm, 
#                  autmatically set by "$slot toParameterSyntax" if default contains "[" ... "]".
#
#   initcmd:       evaluate body in an xotcl nonleaf frame, called via configure 
#                  (example: last arg on create)
#   method         call specified method in an xotcl nonleaf frame, called via configure;
#                  specified value is the first argument unless "noarg" is used  
#                  (example: -noinit).
#
# parameter type   multivalued  required  noarg  type=   arg=  parametercheck  methodParm  objectParm
#  initcmd            NO         YES        NO    NO      NO      NO       NO/POSSIBLE    YES
#  method             NO         YES        YES   NO      YES     NO       NO/POSSIBLE    YES
#
#  relation           NO         YES        NO    NO      YES     NO          NO          YES
#  stringtype         YES        YES        NO    NO      NO      YES         YES         YES
#
#  switch             NO         NO         NO    NO      NO      NO          YES         YES
#  integer            YES        YES        NO    NO      NO      YES         YES         YES    
#  boolean            YES        YES        NO    NO      NO      YES         YES         YES
#  object             YES        YES        NO    YES     NO      YES         YES         YES
#  class              YES        YES        NO    YES     NO      YES         YES         YES
#
#  userdefined        YES        YES        NO    NO      YES     YES         YES         YES
#
# tclObj + converterArg (alnum..xdigit)            Attribute ... -type alnum
# object + converterArg (some class, e.g. ::C)     Attribute ... -type ::C    Attribute -type object -arg ::C
# class  + converterArg (some metaclass, e.g. ::M)                            Attribute -type class -arg ::M
#
#
#::xotcl::Slot {
#     {name "[namespace tail [::xotcl::self]]"}
#     {methodname}
#     {domain "[lindex [regexp -inline {^(.*)::slot::[^:]+$} [::xotcl::self]] 1]"}
#     {defaultmethods {get assign}}
#     {manager "[::xotcl::self]"}
#     {multivalued false}
#     {per-object false}
#     {required false}
#     default
#     type
# } -- No instances
#
# ::xotcl::RelationSlot -superclass ::xotcl::Slot {
#     {multivalued true}
#     {type relation}
#     {elementtype ::nx::Class}
# } -- sample instances: class superclass, mixin filter
#
# ::nx::Attribute -superclass ::xotcl::Slot {
#     {value_check once}
#     initcmd
#     valuecmd
#     valuechangedcmd
#     arg
# } -- typical object parameters
#
# MethodParameterSlot -parameter {type required multivalued noarg arg}
# -- typical method parameters


#######################################################
# objectparameter
#######################################################
Test parameter count 10
Test case objectparameter {

  Class create C -parameter {a {b:boolean} {c 1}}
  C create c1
 
  ? {C eval {:objectparameter}} \
      "-object-mixin:relation,slot=::nx::Class::slot::object-mixin -mixin:relation,arg=class-mixin,slot=::nx::Class::slot::mixin -superclass:relation,slot=::nx::Class::slot::superclass -object-filter:relation,slot=::nx::Class::slot::object-filter -filter:relation,arg=class-filter,slot=::nx::Class::slot::filter -class:relation,slot=::nx::Object::slot::class -parameter:method,optional -noinit:method,optional,noarg -volatile:method,optional,noarg __initcmd:initcmd,optional"




  ? {c1 eval {:objectparameter}} \
      "-a:slot=::C::slot::a -b:boolean,slot=::C::slot::b {-c:slot=::C::slot::c 1} -mixin:relation,arg=object-mixin,slot=::nx::Object::slot::mixin -filter:relation,slot=::nx::Object::slot::filter -class:relation,slot=::nx::Object::slot::class -noinit:method,optional,noarg -volatile:method,optional,noarg __initcmd:initcmd,optional"
}

#######################################################
# reclass to Object, no need to do anything on caching
#######################################################
Test case reclass {

  Class create C -parameter {a {b:boolean} {c 1}}
  C create c1

  c1 class Object
  ? {c1 eval :objectparameter} \
      "-mixin:relation,arg=object-mixin,slot=::nx::Object::slot::mixin -filter:relation,slot=::nx::Object::slot::filter -class:relation,slot=::nx::Object::slot::class -noinit:method,optional,noarg -volatile:method,optional,noarg __initcmd:initcmd,optional"
  
  Class create D -superclass C -parameter {d:required}
  D create d1 -d 100
  
  ? {d1 eval :objectparameter} \
      "-d:required,slot=::D::slot::d -a:slot=::C::slot::a -b:boolean,slot=::C::slot::b {-c:slot=::C::slot::c 1} -mixin:relation,arg=object-mixin,slot=::nx::Object::slot::mixin -filter:relation,slot=::nx::Object::slot::filter -class:relation,slot=::nx::Object::slot::class -noinit:method,optional,noarg -volatile:method,optional,noarg __initcmd:initcmd,optional"
}

#######################################################
# Add mixin
#######################################################
Test case objparam-mixins {

  Class create C -parameter {a {b:boolean} {c 1}}
  Class create D -superclass C -parameter {d:required}
  D create d1 -d 100

  Class create M -parameter {m1 m2 b}
  Class create M2 -parameter {b2}
  D mixin M
  ? {d1 eval :objectparameter} \
      "-b:slot=::M::slot::b -m1:slot=::M::slot::m1 -m2:slot=::M::slot::m2 -d:required,slot=::D::slot::d -a:slot=::C::slot::a {-c:slot=::C::slot::c 1} -mixin:relation,arg=object-mixin,slot=::nx::Object::slot::mixin -filter:relation,slot=::nx::Object::slot::filter -class:relation,slot=::nx::Object::slot::class -noinit:method,optional,noarg -volatile:method,optional,noarg __initcmd:initcmd,optional" \
    "mixin added"
  M mixin M2
  ? {d1 eval :objectparameter} \
      "-b2:slot=::M2::slot::b2 -b:slot=::M::slot::b -m1:slot=::M::slot::m1 -m2:slot=::M::slot::m2 -d:required,slot=::D::slot::d -a:slot=::C::slot::a {-c:slot=::C::slot::c 1} -mixin:relation,arg=object-mixin,slot=::nx::Object::slot::mixin -filter:relation,slot=::nx::Object::slot::filter -class:relation,slot=::nx::Object::slot::class -noinit:method,optional,noarg -volatile:method,optional,noarg __initcmd:initcmd,optional" \
    "transitive mixin added"
  D mixin ""
  #we should have again the old interface

  ? {d1 eval :objectparameter} \
      "-d:required,slot=::D::slot::d -a:slot=::C::slot::a -b:boolean,slot=::C::slot::b {-c:slot=::C::slot::c 1} -mixin:relation,arg=object-mixin,slot=::nx::Object::slot::mixin -filter:relation,slot=::nx::Object::slot::filter -class:relation,slot=::nx::Object::slot::class -noinit:method,optional,noarg -volatile:method,optional,noarg __initcmd:initcmd,optional"

  C mixin M
  ? {d1 eval :objectparameter} \
      "-b2:slot=::M2::slot::b2 -b:slot=::M::slot::b -m1:slot=::M::slot::m1 -m2:slot=::M::slot::m2 -d:required,slot=::D::slot::d -a:slot=::C::slot::a {-c:slot=::C::slot::c 1} -mixin:relation,arg=object-mixin,slot=::nx::Object::slot::mixin -filter:relation,slot=::nx::Object::slot::filter -class:relation,slot=::nx::Object::slot::class -noinit:method,optional,noarg -volatile:method,optional,noarg __initcmd:initcmd,optional" \
    "mixin added"
  C mixin ""
  #we should have again the old interface
  
  
  ? {d1 eval :objectparameter} \
      "-d:required,slot=::D::slot::d -a:slot=::C::slot::a -b:boolean,slot=::C::slot::b {-c:slot=::C::slot::c 1} -mixin:relation,arg=object-mixin,slot=::nx::Object::slot::mixin -filter:relation,slot=::nx::Object::slot::filter -class:relation,slot=::nx::Object::slot::class -noinit:method,optional,noarg -volatile:method,optional,noarg __initcmd:initcmd,optional" 
}

#######################################################
# test passed arguments
#######################################################

Test case passed-arguments {

  Class create C -parameter {a {b:boolean} {c 1}}
  Class create D -superclass C -parameter {d:required}

  ? {catch {D create d1 -d 123}} 0 "create d1 with required argument given"
  ? {catch {D create d1}} 1 "create d1 without required argument given"
  #puts stderr current=[namespace current]
  
  ? {D create d1} "::d1 configure: required argument 'd' is missing" "check error msg"
  
  ? {D create d2 -d x -b a} \
      {expected boolean value but got "a" for parameter -b} \
      "create d2 without required argument given"

  D create d1 -d 1
  D method foo {-b:boolean -r:required,int {-x:int aaa} {-object:object} {-class:class}} {
    #if {[info exists x]} {puts stderr x=$x}
  }
  
  ? {d1 foo} \
      "::d1 foo: required argument 'r' is missing" \
      "call method without a required argument"
  
  ? {d1 foo -r a} \
      {expected integer but got "a" for parameter -r} \
      "required argument is not integer"
  
  ? {d1 foo -r 1} \
      {expected integer but got "aaa" for parameter -x} \
      "default value is not of type integer"
  
  ? {d1 foo -r 1 -x 1 -object d1} \
      "" \
      "pass object"
  
  ? {d1 foo -r 1 -x 1 -object d11} \
      {expected object but got "d11" for parameter -object} \
      "pass non-existing object"
  
  ? {d1 foo -r 1 -x 1 -class D} \
      "" \
      "pass class"
  
  ? {d1 foo -r 1 -x 1 -class d1} \
      {expected class but got "d1" for parameter -class} \
      "pass object instead of class"
  
  ? {d1 foo -r 1 -x 1 -class D11} \
      {expected class but got "D11" for parameter -class} \
      "pass non-existing class"
  
  ? {D method foo {a:relation} {}} \
      {Parameter option 'relation' not allowed} \
      "don't allow relation option as method parameter"
  
  ? {D method foo {a:double} {return $a}} \
      {::nsf::classes::D::foo} \
      "allow 'string is XXXX' for argument checking"
  ? {d1 foo 1} 1 "check int as double"
  ? {d1 foo 1.1} 1.1 "check double as double"
  ? {d1 foo 1.1a} {expected double but got "1.1a" for parameter a} "check non-double as double"
  ? {D info method parameter foo} a:double
}

#######################################################
# non required positional arguments
#######################################################
Test case non-reg-args {

  Class create D 
  D create d1
  
  D method foo {a b:optional c:optional} {
    return "[info exists a]-[info exists b]-[info exists c]"
  }
  ? {d1 foo 1 2} "1-1-0" "omit optional argument"
  ? {d1 foo 1} "1-0-0" "omit optional arguments"
  
  # non required positional arguments and args
  D method foo {a b:optional c:optional args} {
    return "[info exists a]-[info exists b]-[info exists c]-[info exists args]"
  }
  ? {d1 foo 1 2} "1-1-0-1" "omit optional argument"
  ? {d1 foo 1} "1-0-0-1" "omit optional arguments"
}

#######################################################
# multivalued arguments
#######################################################
Test case multivalued {

  Class create D 
  D create d1
  Object create o

  D method foo {m:integer,multivalued} {
    return $m
  }
  ? {d1 foo ""} "" "emtpy list"
  ? {d1 foo 1} "1" "single value"
  ? {d1 foo {1 2}} "1 2" "multiple values"
  ? {d1 foo {1 a 2}} \
      {invalid value in "1 a 2": expected integer but got "a" for parameter m} \
      "multiple values with wrong value"
  
  D method foo {m:object,multivalued} {
    return $m
  }
  ? {d1 foo ""} "" "emtpy list"
  ? {d1 foo o} "o" "single value"
  ? {d1 foo {o d1 x}} \
      {invalid value in "o d1 x": expected object but got "x" for parameter m} \
      "multiple values"
  
  Class create Foo -parameter {
    {ints:integer,multivalued}
  }
  ? {Foo create foo -ints {1 2}} "::foo"
  ? {Foo create foo -ints {1 a 2}} {invalid value in "1 a 2": expected integer but got "a" for parameter -ints}
  
  # make slot incremental
  Foo slot ints eval {
    set :incremental 1
    :optimize
  }
  Foo create foo -ints {1 2}
  ? {foo ints add 0} "0 1 2"
  ? {foo ints add a} {expected integer but got "a" for parameter value}
}

#######################################################
# subst default tests
#######################################################
Test case subst-default {
  
  Class create D {
    :attribute {c 1}
    :attribute {d 2}

    :create d1

    :method bar {
      {-s:substdefault "[current]"} 
      {-literal "[current]"} 
      {-c:substdefault "[my c]"} 
      {-d:integer,substdefault "$d"}
    } {
      return $s-$literal-$c-$d
    }
  }
  
  ? {d1 bar -c 1} {::d1-[current]-1-2} "substdefault in method parameter"
  
  Class create Bar -superclass D -parameter {
    {s "[current]"} 
    {literal "\\[current\\]"} 
    {c "[:info class]"} 
    {d "literal $d"}
    {switch:switch}
  }
  Bar create bar1
  #puts stderr [bar1 objectparameter]
  
  ? {subst {[bar1 s]-[bar1 literal]-[bar1 c]-[bar1 d]-[bar1 switch]}} \
      {::bar1-[current]-::Bar-literal $d-0} \
      "substdefault and switch in object parameter 1"
  
  Bar create bar2 -switch
  ? {subst {[bar2 s]-[bar2 literal]-[bar2 c]-[bar2 d]-[bar2 switch]}} \
      {::bar2-[current]-::Bar-literal $d-1} \
      "substdefault and switch in object parameter 2"
  
  # Observations:
  #  1) syntax for "-parameter" and method parameter is quite different.
  #     it would be nice to be able to specify the objparameters in
  #     the same syntax as the method parameters. 
  #
  # 1a) Especially specifying "-" in front of a -parameter or not might
  #     be confusing.
  #
  # 1b) Positional args for obj parameter and arguments for init
  #     might be confusing as well. Should we forget about 
  #     passing arguments to init?
  #
  #  2) substdefault for '$' in -parameter defaults does not make much sense.
  #     deactivated for now; otherwise we would need "\\"
  
  D method bar {
		{-s:substdefault "[current]"} 
		{-literal "[current]"} 
		{-c:substdefault "[my c]"} 
		{-d:integer,substdefault "$d"}
		{-switch:switch}
		{-optflag}
		x
		y:integer
		{z 1}
	      } {
    return $s-$literal-$c-$d
  }
  
  ? {D info method args bar} {s literal c d switch optflag x y z} "all args" 
  ? {D info method parameter bar} \
      {{-s:substdefault "[current]"} {-literal "[current]"} {-c:substdefault "[my c]"} {-d:integer,substdefault "$d"} -switch:switch -optflag x y:integer {z 1}} \
      "query method parameter" 
  
  D method foo {a b {-c 1} {-d} x {-end 100}} {
    set result [list]
    foreach v [[current class] info method args [current method]] {
      lappend result $v [info exists $v]
    }
    return $result
  }
  ? {d1 foo 1 2 3} \
      "a 1 b 1 c 1 d 0 x 1 end 1" \
      "parse multiple groups of nonpos args"
  
  D method foo {a b c {end 100}} {
    set result [list]
    foreach v [[current class] info method args [current method]] {
      lappend result $v [info exists $v]
    }
    return $result
  }
  ? {d1 foo 1 2 3} \
      "a 1 b 1 c 1 end 1" \
      "query arguments with default, no paramdefs needed"

  #######################################################
  # Query method parameter
  #######################################################

  ? {D info method parameter foo} \
      "a b c {end 100}" \
      "query instparams with default, no paramdefs needed"

  ? {Class info method parameter method} \
      "name arguments body -precondition -postcondition" \
      "query instparams for scripted method 'method'"
  
  ? {catch {Object info method parameter forward}} \
      "1" \
      "query parameter for C-defined method 'forward'"
  
  ? {Object info method parameter autoname} \
      "-instance -reset name" \
      "query parameter for C-defined method 'autoname'"
  
  # TODO: how to query the params/instparams of info subcommands?
  #? {::xotcl::objectInfo info params params} \
  #    "xxx" \
  #    "query instparams for info method 'params' method"
}

#######################################################
# user defined parameter types
#######################################################
Test case user-types {

  Class create D -parameter d
  D create d1

  # create a userdefined type
  ::nx::methodParameterSlot method type=mytype {name value args} {
    if {$value < 1 || $value > 3} {
      error "Value '$value' of parameter $name is not between 1 and 3"
    }
  }


  D method foo {a:mytype} {
    puts stderr a=$a
  }
  d1 foo 1

  ? {d1 foo 10} \
      "Value '10' of parameter a is not between 1 and 3" \
      "value not between 1 and 3"
  
  D method foo {a:unknowntype} {
    puts stderr a=$a
  }
  
  ? {d1 foo 10} \
      "::nx::methodParameterSlot: unable to dispatch method 'type=unknowntype'" \
      "missing type checker"
  
  # create a userdefined type with a simple argument
  ::nx::methodParameterSlot method type=in {name value arg} {
    if {$value ni [split $arg |]} {
      error "Value '$value' of parameter $name not in permissible values $arg"
    }
    return $value
  }
  
  D method foo {a:in,arg=a|b|c} {
    return a=$a
  }
  
  ? {d1 foo a} "a=a"
  ? {d1 foo 10} \
      "Value '10' of parameter a not in permissible values a|b|c" \
      "invalid value"
  
  D method foo {a:in,arg=a|b|c b:in,arg=good|bad {-c:in,arg=a|b a}} {
    return a=$a,b=$b,c=$c
  }
  
  ? {d1 foo a good -c b} "a=a,b=good,c=b"
  ? {d1 foo a good} "a=a,b=good,c=a"
  ? {d1 foo b "very good"} \
      "Value 'very good' of parameter b not in permissible values good|bad" \
      "invalid value (not included)"
  
  ::nx::methodParameterSlot method type=range {name value arg} {
    foreach {min max} [split $arg -] break
    if {$value < $min || $value > $max} {
      error "Value '$value' of parameter $name not between $min and $max"
    }
    return $value
  }
  
  D method foo {a:range,arg=1-3 {-b:range,arg=2-6 3} c:range,arg=5-10} {
    return a=$a,b=$b,c=$c
  }
  
  ? {d1 foo 2 -b 4 9} "a=2,b=4,c=9"
  ? {d1 foo 2 10} "a=2,b=3,c=10"
  ? {d1 foo 2 11} \
      "Value '11' of parameter c not between 5 and 10" \
      "invalid value"
  
  # define type twice
  ? {D method foo {a:int,range,arg=1-3} {return a=$a}} \
      "Refuse to redefine parameter converter to use type=range" \
      "invalid value"
  
  #
  # handling of arg with spaces/arg as list
  #
  ::nx::methodParameterSlot method type=list {name value arg} {
    #puts $value/$arg
    return $value
  }
  
  # handling spaces in "arg" is not not particular nice
  D method foo {{"-a:list,arg=2 6" 3} {"b:list,arg=5 10"}} {
    return a=$a,b=$b
  }
  ? {d1 foo -a 2 10} "a=2,b=10"
  
}
#######################################################
# testing object types in method parameters
#######################################################
Test case mp-object-types {

  Class create C
  Class create D -superclass C -parameter d

  Class create M
  D create d1 -d 1
  C create c1 -mixin M
  Object create o
  
  D method foo-base     {x:baseclass} {return $x}
  D method foo-class    {x:class} {return $x}
  D method foo-object   {x:object} {return $x}
  D method foo-meta     {x:metaclass} {return $x}
  D method foo-hasmixin {x:hasmixin,arg=::M} {return $x}
  D method foo-type     {x:object,type=::C} {return $x}
  
  ? {D info method parameter foo-base} "x:baseclass"
  ? {D info method parameter foo-hasmixin} "x:hasmixin,arg=::M"
  ? {D info method parameter foo-type} "x:object,type=::C"
  
  ? {d1 foo-base ::nx::Object} "::nx::Object"
  ? {d1 foo-base C} \
      {expected baseclass but got "C" for parameter x} \
      "not a base class"
  
  ? {d1 foo-class D} "D"
  ? {d1 foo-class xxx} \
      {expected class but got "xxx" for parameter x} \
      "not a class"
  ? {d1 foo-class o} \
      {expected class but got "o" for parameter x} \
      "not a class"
  
  ? {d1 foo-meta ::nx::Class} "::nx::Class"
  ? {d1 foo-meta ::nx::Object} \
      {expected metaclass but got "::nx::Object" for parameter x} \
      "not a base class"

  ? {d1 foo-hasmixin c1} "c1"
  ? {d1 foo-hasmixin o} \
      {expected object with mixin ::M but got "o" for parameter x} \
      "does not have mixin M"
  
  ? {d1 foo-object o} "o"
  ? {d1 foo-object xxx} \
      {expected object but got "xxx" for parameter x} \
      "not an object"
  
  ? {d1 foo-type d1} "d1"
  ? {d1 foo-type c1} "c1"
  ? {d1 foo-type o} \
      {expected object of type ::C but got "o" for parameter x} \
      "o not of type ::C"
}

#######################################################
# substdefault
#######################################################
Test case substdefault {

  Class create S -parameter {{x 1} {y b} {z {1 2 3}}}
  S create s1 {
    :method foo {{y:substdefault ${:x}}} {
      return $y
    }
    :method bar {{y:integer,substdefault ${:x}}} {
      return $y
    }
    :method baz {{x:integer,substdefault ${:y}}} {
      return $x
    }
    :method boz {{x:integer,multivalued,substdefault ${:z}}} {
      return $x
    }
  }
  ? {s1 foo} 1
  ? {s1 foo 2} 2
  
  ? {S method foo {a:substdefault} {return 1}} \
      {parameter option substdefault specified for parameter "a" without default value}
  
  ? {s1 bar} 1
  ? {s1 bar 3} 3
  ? {s1 bar a} {expected integer but got "a" for parameter y}
  
  ? {s1 baz} {expected integer but got "b" for parameter x}
  ? {s1 baz 20} 20
  s1 y 100
  ? {s1 baz} 100
  ? {s1 baz 101} 101
  
  ? {s1 boz} {1 2 3}
  s1 z {1 x 100}
  ? {s1 boz} {invalid value in "1 x 100": expected integer but got "x" for parameter x}
  ? {s1 boz {100 200}} {100 200}
  
  set ::aaa 100
  ? {s1 method foo {{a:substdefault $::aaa}} {return $a}} ::s1::foo
  ? {s1 foo} 100
  unset ::aaa
  ? {s1 foo} {can't read "::aaa": no such variable}
  
  ? {s1 method foo {{a:substdefault $aaa}} {return $a}} ::s1::foo
  ? {s1 foo} {can't read "aaa": no such variable}
  
  ? {s1 method foo {{a:substdefault [current]}} {return $a}} ::s1::foo
  ? {s1 foo} ::s1
}

#######################################################
# testing substdefault for object parameters
#######################################################
Test case substdefault-objparam {

  Class create Bar {

    # simple, implicit substdefault
    :attribute {s0 "[current]"}

    # explicit substdefault
    :attribute {s1:substdefault "[current]"}
    
    # unneeded double substdefault
    :attribute {s2:substdefault,substdefault "[current]"}

    # substdefault with incremental
    :attribute {s3:substdefault "[current]"} {
      # Bypassing the Optimizer helps after applying the patch (solving step 1)
      set :incremental 1
    }
  }

  Bar create ::b
  ? {b s0} "::b"
  ? {b s1} "::b"
  ? {b s2} "::b"
  ? {b s3} "::b"
}

#######################################################
# testing object types in object parameters
#######################################################
Test case op-object-types {

  Class create C
  Class create D -superclass C -parameter d

  Class create MC -superclass Class
  MC create MC1
  Class create M
  D create d1 -d 1
  C create c1 -mixin M
  Object create o
  
  Class create ParamTest -parameter {
    o:object
    c:class
    c1:class,type=::MC
    d:object,type=::C
    d1:object,type=C
    m:metaclass
    mix:hasmixin,arg=M
    b:baseclass
    u:upper
    us:upper,multivalued
    {x:object,multivalued {o}}
  }
  
  # TODO: we have no good interface for querying the slot notation for parameters
  proc ::parameterFromSlot {class objectparameter} {
    set slot ${class}::slot::$objectparameter
    array set "" [$slot toParameterSyntax $objectparameter]
    return $(oparam)
  }
  
  ? {::parameterFromSlot ParamTest o} "o:object,slot=::ParamTest::slot::o"
  ? {::parameterFromSlot ParamTest c} "c:class,slot=::ParamTest::slot::c"
  ? {::parameterFromSlot ParamTest c1} "c1:class,type=::MC,slot=::ParamTest::slot::c1"
  ? {::parameterFromSlot ParamTest d} "d:object,type=::C,slot=::ParamTest::slot::d"
  ? {::parameterFromSlot ParamTest d1} "d1:object,type=::C,slot=::ParamTest::slot::d1"
  ? {::parameterFromSlot ParamTest mix} "mix:hasmixin,arg=M,slot=::ParamTest::slot::mix"
  ? {::parameterFromSlot ParamTest x} "x:object,multivalued,slot=::ParamTest::slot::x o"
  ? {::parameterFromSlot ParamTest u} "u:upper,slot=::ParamTest::slot::u"
  ? {::parameterFromSlot ParamTest us} "us:upper,multivalued,slot=::ParamTest::slot::us"
  
  ? {ParamTest create p -o o} ::p
  ? {ParamTest create p -o xxx} \
      {expected object but got "xxx" for parameter -o} \
      "not an object"
  
  ? {ParamTest create p -c C} ::p "class"
  ? {ParamTest create p -c o} \
      {expected class but got "o" for parameter -c} \
      "not a class"
  
  ? {ParamTest create p -c1 MC1} ::p "instance of meta-class MC"
  ? {ParamTest create p -c1 C} \
      {expected class of type ::MC but got "C" for parameter -c1} \
      "not an instance of meta-class MC"
  
  ? {ParamTest create p -d d1} ::p
  ? {ParamTest create p -d1 d1} ::p
  ? {ParamTest create p -d c1} ::p
  ? {ParamTest create p -d o} \
      {expected object of type ::C but got "o" for parameter -d} \
      "o not of type ::C"
  
  ? {ParamTest create p -mix c1} ::p
  ? {ParamTest create p -mix o} \
      {expected object with mixin M but got "o" for parameter mix} \
      "does not have mixin M"
  
  ? {ParamTest create p -u A} ::p
  ? {ParamTest create p -u c1} {expected upper but got "c1" for parameter -u}
  ? {ParamTest create p -us {A B c}} \
      {invalid value in "A B c": expected upper but got "c" for parameter -us}
  ParamTest slot us eval {
    set :incremental 1
    :optimize
  }
  ? {ParamTest create p -us {A B}} ::p
  ? {p us add C end} "A B C"

  ? {p o o} \
      "o" \
      "value is an object"

  ? {p o xxx} \
      {expected object but got "xxx" for parameter o} \
      "value is not an object"

  #ParamTest slots {
  #  ::nx::Attribute create os -type object -multivalued true
  #}
  ParamTest eval {
      :attribute os {
	  :type object 
	  :multivalued true
      }
  }
  
  ? {p os o} \
      "o" \
      "value is a list of objects (1 element)"
  ? {p os {o c1 d1}} \
      "o c1 d1" \
      "value is a list of objects (multiple elements)"
  
  ? {p os {o xxx d1}} \
      {invalid value in "o xxx d1": expected object but got "xxx" for parameter os} \
      "list with invalid object"
}

#######################################################
# application specific multivalued converter
#######################################################
Test case multivalued-app-converter {

  ::nx::methodParameterSlot method type=sex {name value args} {
    #puts stderr "[current] slot specific converter"
    switch -glob $value {
      m* {return m}
      f* {return f}
      default {error "expected sex but got $value"}
    }
  }
  Class create C {
    :method foo {s:sex,multivalued} {return $s}
  }
  C create c1
  ? {c1 foo {male female mann frau}} "m f m f"
  

  Object create tmpObj
  tmpObj method type=mType {name value arg:optional} {
    if {$value} {
      error "expected false but got $value"
    }
    # Note that this converter does NOT return a value; it converts all
    # values into emtpy strings.
  }
  
  ? {::nsf::parametercheck mType,slot=::tmpObj,multivalued {1 0}} \
      {invalid value in "1 0": expected false but got 1} \
      "fail on first value"
  ? {::nsf::parametercheck mType,slot=::tmpObj,multivalued {0 0 0}} 1 "all pass"
  ? {::nsf::parametercheck mType,slot=::tmpObj,multivalued {0 1}} \
      {invalid value in "0 1": expected false but got 1} \
      "fail o last value"
}
#######################################################
# application specific multivalued converter
#######################################################
Test case shadowing-app-converter {

  Object create mySlot {
    :method type=integer {name value arg:optional} {
      return [expr {$value + 1}]
    }
  }
  Object create o {
    :method foo {x:integer,slot=::mySlot} {
      return $x
    }
  }
  
  ? {::nsf::parametercheck integer,slot=::mySlot 1} 1 
  ? {o foo 3} 4
}


#######################################################
# allow empty values
#######################################################
Test case allow-empty {

  Object create o1
  Object create o2
  Object create o3

  Object create o {
    :method foo {x:integer,allowempty y:integer os:object,multivalued,allowempty} {
      return $x
    }
  }
  
  ? {o foo 1 2 {o1 o2}} 1 "all values specified"
  ? {o foo "" 2 {o1 o2}} "" "first is empty"
  ? {o foo 1 "" {o1 o2}} {expected integer but got "" for parameter y} "second is empty"
  ? {o foo 1 2 {}} 1 "empty list, does not require allowempty"
  ? {o foo 1 2 {o1 "" o2}} 1 "list contains empty value"

  ? {o info method parameter foo} "x:integer,allowempty y:integer os:object,multivalued,allowempty"

  o method foo {x:integer,allowempty y:integer os:object,multivalued} {return $x}
  ? {o foo 1 2 {o1 "" o2}} {invalid value in "o1 "" o2": expected object but got "" for parameter os} \
      "list contains empty value"

}
#######################################################
# slot specific converter
#######################################################
Test case slot-specfic-converter {

  Class create Person {
    :attribute sex {
      :type "sex"
      :method type=sex {name value} {
	#puts stderr "[self] slot specific converter"
	switch -glob $value {
	  m* {return m}
	  f* {return f}
	  default {error "expected sex but got $value"}
	}
      }
    }
  }

  Person create p1 -sex male
  ? {p1 sex} m
  Person method foo {s:sex,slot=::Person::slot::sex} {return $s}
  ? {p1 foo male} m
  ? {p1 sex male} m
}

#######################################################
# test for setters with parameters 
#######################################################
Test case setters {
  Object create o
  Class create C
  
  ? {::nsf::setter o a} "::o::a"
  ? {::nsf::setter C c} "::nsf::classes::C::c"
  ? {o info method definition a} "::o setter a"
  ? {o info method parameter a} "a"
  ? {o info method args a} "a"
  ? {C info method definition c} "::C setter c"
  ? {o a 1} "1"
  
  ? {::nsf::setter o a:integer} "::o::a"
  ? {::nsf::setter o ints:integer,multivalued} "::o::ints"
  ? {::nsf::setter o o:object} "::o::o"
  
  ? {o info method name ints} "::o::ints"
  ? {o info method definition ints} "::o setter ints:integer,multivalued"
  ? {o info method parameter ints} "ints:integer,multivalued"
  ? {o info method args ints} "ints"
  
  ? {o info method name o} "::o::o"
  ? {o info method definition o} "::o setter o:object"
  ? {o info method parameter o} "o:object"
  ? {o info method args o} "o"

  ? {o a 2} 2
  ? {o a hugo} {expected integer but got "hugo" for parameter a}

  ? {o ints {10 100 1000}} {10 100 1000}
  ? {o ints hugo} {invalid value in "hugo": expected integer but got "hugo" for parameter ints}
  ? {o o o} o
  ? {::nsf::setter o {d default}} {parameter "d" is not allowed to have default "default"}
  ? {::nsf::setter o -x} {method name "-x" must not start with a dash}
}

#######################################################
# test for slot-optimizer
#######################################################
Test parameter count 1000
Test case slot-optimizer {

  Class create C -parameter {a b:integer c:integer,multivalued}
  
  C create c1 
  ? {c1 a 1} 1
  ? {c1 b 1} 1
  ? {c1 c 1} 1

  # before: 1st case: setter, 2&3: forward
  #slot-optimizer.001:	  1.50 mms, c1 a 1
  #slot-optimizer.002:	  3.30 mms, c1 b 1
  #slot-optimizer.003:	  3.40 mms, c1 c 1
  #
  # after: 1st, 2nd, 3rd case: setter 
  #slot-optimizer.001:	  1.50 mms, c1 a 1
  #slot-optimizer.002:	  1.50 mms, c1 b 1
  #slot-optimizer.003:	  1.60 mms, c1 c 1
}
## TODO regression test for type checking, parameter options (initcmd,
## substdefault, combinations with defaults, ...), etc.

puts stderr =====END
