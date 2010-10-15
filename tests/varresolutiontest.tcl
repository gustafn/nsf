#
# testing var resolution
#
package require nx; namespace import ::nx::*
::nx::configure defaultMethodCallProtection false
package require nx::test

Test parameter count 1

::nsf::alias ::nx::Object objeval -frame object ::eval 
::nsf::alias ::nx::Object array -frame object ::array
::nsf::alias ::nx::Object lappend -frame object ::lappend
::nsf::alias ::nx::Object incr -frame object ::incr
::nsf::alias ::nx::Object set -frame object ::set
::nsf::alias ::nx::Object unset -frame object ::unset

###########################################
# Basic tests for var resolution under 
# per-object namespaces ...
###########################################
Test case globals
set ::globalVar 1
Object create o 
o require namespace
? {o info vars} ""
? {info exists ::globalVar} 1
? {set ::globalVar} 1
? {o eval {info exists :globalVar}} 0
? {o array exists globalVar} 0
o array set globalVar {1 2}
? {o eval {info exists :globalVar}} 1
? {o info vars} globalVar
? {o array exists globalVar} 1
? {set ::globalVar} 1
? {o set globalVar(1)} 2

o destroy
unset ::globalVar

###########################################
# scopes
###########################################
Test case scopes

Object create o 
Object create o2 {set :i 1}
o objeval {
  # require an namespace within an objscoped frame; it is necessary to replace
  # vartables on the stack
  :require namespace
  global g
  ::nsf::importvar o2 i
  set x 1
  set :y 2
  set ::z 3
  set [current]::X 4
  set g 1
  set :a(:b) 1
  set :a(::c) 1
}
? {::nsf::importvar o2 j} \
    "importvar cannot import variable 'j' into method scope; not called from a method frame"

o method foo {} {::nsf::importvar [current] :a}
? {o foo} "variable name \":a\" must not contain namespace separator or colon prefix"

o method foo {} {::nsf::importvar [current] ::a}
? {o foo} "variable name \"::a\" must not contain namespace separator or colon prefix"

o method foo {} {::nsf::importvar [current] a(:b)}
? {o foo} "can't make instance variable a(:b) on ::o: Variable cannot be an element in an array; use e.g. an alias."

o method foo {} {::nsf::importvar [current] {a(:b) ab}}
? {o foo} ""

o method foo {} {::nsf::existsvar [current] ::a}
? {o foo} "variable name \"::a\" must not contain namespace separator or colon prefix"

o method foo {} {::nsf::existsvar [current] a(:b)}
? {o foo} 1

o method foo {} {::nsf::existsvar [current] a(::c)}
? {o foo} 1

set ::o::Y 5
? {info vars ::x} ""

? {info exists ::z} 1
? {set ::z} 3
? {lsort [o info vars]} {X Y a g i x y}
? {o eval {info exists :x}} 1
? {o eval {info exists :y}} 1
? {o eval {info exists :z}} 0
? {o eval {info exists :X}} 1
? {o eval {info exists :Y}} 1
? {o set y} 2
? {set ::g} 1 

o destroy
o2 destroy
unset ::z
unset ::g

# like the example above, but with the non-leaf initcmd

Object create o2 {set :i 1}
Object create o {
  :require namespace
  global g
  ::nsf::importvar o2 i
  set x 1
  set :y 2
  set ::z 3
  set [current]::X 4
  set g 1
}
set ::o::Y 5
? {info vars ::x} ""

? {info exists ::z} 1
? {set ::z} 3
? {lsort [o info vars]} {X Y y}
? {o eval {info exists :x}} 0
? {o eval {info exists :y}} 1
? {o eval {info exists :z}} 0
? {o eval {info exists :X}} 1
? {o eval {info exists :Y}} 1
? {o set y} 2
? {set ::g} 1 

o destroy
o2 destroy
unset ::z
unset ::g
foreach v {::x ::z ::g} {unset -nocomplain $v}

###########################################
# var exists tests
###########################################
Test case exists {
    set y 1 

    Object create o {set :x 1}
    o method foo {} {info exists :x}
    o method bar {} {info exists :y}
    ? {o eval {info exists :x}} 1
    ? {o eval {info exists :y}} 0
    ? {o eval {info exists x}} 0
    ? {o foo} 1
    ? {o bar} 0
    ? {::nx::var exists o x} 1
    ? {::nx::var exists o y} 0
    ? {::nx::var exists o :x} {variable name ":x" must not contain namespace separator or colon prefix}
    ? {::nx::var exists o :y} {variable name ":y" must not contain namespace separator or colon prefix}
    ? {::nx::var set o y 2} 2
    ? {::nx::var exists o y} 1
    ? {::nx::var set o :y 2} {variable name ":y" must not contain namespace separator or colon prefix}
}

###########################################
# mix & match namespace and object interfaces
###########################################
Test case namespaces

Object create o 
o require namespace
o set x 1
? {namespace eval ::o {set x}} 1
? {::o set x} 1
? {namespace eval ::o {set x 3}} 3
? {::o set x} 3
? {namespace eval ::o {info exists x}} 1
? {::o unset x} ""
? {::nsf::existsvar o x} 0
? {o eval {info exists :x}} 0
? {info vars ::x} ""
? {namespace eval ::o {info exists x}} 0
o lappend y 3
? {namespace eval ::o {llength y}} 1
? {namespace eval ::o {unset y}} ""
? {o eval {info exists :y}} 0
o destroy

###########################################
# array-specific tests
###########################################
Test case namespaces-array

Object create o 
o require namespace

? {o array exists a} 0
? {namespace eval ::o array exists a} 0
o array set a {1 2 3 4 5 6}
? {o array exists a} 1
? {namespace eval ::o array exists a} 1
? {namespace eval ::o array names a} [::o array names a]
? {namespace eval ::o array size a} [::o array size a]
? {o set a(1) 7} 7
? {namespace eval ::o array get a 1} {1 7}
? {namespace eval ::o set a(1) 2} 2
? {o array get a 1} {1 2}
? {::o unset a} ""
? {::o array unset a} ""
? {o array exists a} 0
? {namespace eval ::o array exists a} 0

o destroy

###########################################
# tests on namespace-qualified var names
###########################################
Test case namespaced-var-names
Object create o 
o require namespace
Object create o::oo 
o::oo require namespace

? {::o set ::x 1} 1
? {info exists ::x} [set ::x]
? {catch {unset ::x}} 0

? {::o set ::o::x 1} 1
? {o eval {info exists :x}} [::o set ::o::x]
? {namespace eval ::o unset x} ""
? {o eval {info exists x}} 0

# Note, relatively qualified var names (not prefixed with ::*) 
# are always resolved relative to the per-object namespace
? {catch {::o set o::x 1} msg} 1
? {::o set oo::x 1} 1
? {o::oo eval {info exists :x}} [::o set oo::x]
? {o unset oo::x} ""
? {o::oo eval {info exists :x}} 0

o destroy

###########################################
# tests on namespace-qualified on objects
# without namespaces
###########################################

# the tests below fail. We could consider
# to require namespaces on the fly in the future
#Object create o
#? {::o set ::o::x 1} 1
#? {o exists x} [::o set ::o::x]
#? {namespace eval ::o unset x} ""
#? {o exists x} 0

#? {::o set o::x 1} 1
#? {o exists x} [::o set o::x]
#? {namespace eval ::o unset x} ""
#? {o exists x} 0

#o destroy

###############################################
# tests for the compiled var resolver on Object
###############################################
Test case var-resolver-object
Object create o
o method foo {x} {set :y 2; return ${:x},${:y}}
o method bar {} {return ${:x},${:y}}
o set x 1
? {o foo 1} "1,2" "create var y and fetch var x"
? {o bar} "1,2" "fetch two instance variables"
? {o info vars} "x y" 
# recreate object, check var caching; 
# we have to recreate bar, so no problem
Object create o
o set x 1
o method bar {} {return ${:x},${:y}}
? {catch {o bar}} "1" "compiled var y should not exist"
o destroy

###############################################
# tests for the compiled var resolver on Class
###############################################
Test case var-resolver-class
Class create C -attributes {{x 1}}
C create c1
C method foo {x} {set :y 2; return ${:x},${:y}}
C method bar {} {return ${:x},${:y}}
? {c1 info vars} "x" 
? {c1 foo 1} "1,2" "create var y and fetch var x"
? {c1 bar} "1,2" "fetch two instance variables"
? {c1 info vars} "x y" 
# recreate object, check var caching; 
# we do not have to recreate bar, compiled var persists,
# change must be detected
C create c1
#puts stderr "after recreate"
? {catch {c1 bar}} "1" "compiled var y should not exist"
? {c1 info vars} "x" 
c1 destroy
C destroy



###############################################
# tests for the compiled var resolver with eval
###############################################
Test case compiled-var-resolver
Class create C -attributes {{x 1}}
C create c1
C method foo {x} {
  set :y 2; 
  eval "set :z 3"
  return ${:x},${:y},${:z}
}
? {c1 info vars} "x"
? {c1 foo 1} "1,2,3"
? {c1 info vars} "x y z"
C create c1
? {c1 info vars} "x"
C method foo {x} {
  set cmd set
  lappend cmd :y
  lappend cmd 100
  eval $cmd
  return $x,${:y}
}
C method bar {} {return [info exists :x],[info exists :y]}
C method bar2 {} {if {[info exists :x]} {set :x 1000}; return [info exists :x],[info exists :y]}
? {c1 foo 1} "1,100"
? {c1 bar} "1,1"
? {c1 bar2} "1,1"
c1 unset x
? {c1 bar2} "0,1"
c1 destroy
C destroy

###############################################
# tests with array
###############################################

Class create C
C create c1
C method foo {} {
  array set :a {a 1 b 2 c 3}
  set :z 100
}
? {c1 info vars} ""
c1 foo
? {lsort [c1 info vars]} {a z}

###############################################
# tests for the var resolver
###############################################
Test case var-resolver
Class create C
C method bar0 {} {return ${:x}}
C method bar1 {} {set a ${:x}; return [info exists :x],[info exists :y]}
C method bar2 {} {return [info exists :x],[info exists :y]}
C method foo {} {
  array set :a {a 1 b 2 c 3}
  set :z 100
}
C create c1
c1 set x 100
? {c1 bar0} 100 "single compiled local"
? {c1 bar1} 1,0 "lookup one compiled var and one non-existing"
? {c1 bar2} 1,0 "lookup one non compiled var and one non-existing"
C create c2
? {c2 bar2} 0,0 "lookup two one non-existing, first access to varTable"
c1 foo
? {lsort [c1 info vars]} "a x z" "array variable set via resolver"
? {lsort [c1 array names a]} "a b c" "array looks ok"

###############################################
# first tests for the cmd resolver
###############################################
Class create C
C method bar {args} {
  #puts stderr "[current] bar called with [list $args]"
  return $args
}
C forward test %self bar
C method foo {} {
  # this works
  lappend :r [:bar x 1]
  lappend :r [:test a b c]
  # these kind of works, but vars are nowhere....
  :set x 1
  :incr x 1
  :incr x 1
  return [lappend :r ${:x}]
}
C create c3
? {c3 foo} "{x 1} {a b c} 3"

###############################################
# refined tests for the var resolver under
# Tcl namespaces parallelling XOTcl objects 
# (! not declared through require namespace !)
# e.g., "info has namespace" reports 0 rather
# than 1 as under "require namespace"
###############################################

set ::w 1
array set ::tmpArray {key value}

Class create ::C
::nsf::alias ::C Set -frame object ::set
::nsf::alias ::C Unset -frame object ::unset

::C create ::c
namespace eval ::c {}
? {namespace exists ::c} 1
? {::nsf::isobject ::c} 1
? {::c info has namespace} 0

? {::c Set w 2; expr {[::c Set w] == $::w}} 0
? {::c Unset w; info exists ::w} 1
? {::c Set tmpArray(key) value2; expr {[::c Set tmpArray(key)] == $::tmpArray(key)}} 0
? {::c Unset tmpArray(key); info exists ::tmpArray(key)} 1

::c destroy
::C destroy
unset ::w
unset ::tmpArray

##################################################
# Testing aliases for eval with and without 
# -varscope flags and with a
# required namespace and without
##################################################
Test case eval-variants
::nsf::alias ::nx::Object objeval -frame object ::eval 
::nsf::alias ::nx::Object softeval -frame method ::eval 
::nsf::alias ::nx::Object softeval2 ::eval 

set G 1

Object create o {
  set xxx 1
  set :x 1
  ? {info exists G} 1
}
? {o eval {info exists :x}} 1
? {o eval {info exists :xxx}} 0

? {info exists ::xxx} 0
unset -nocomplain ::xxx

# eval does an objcope, all vars are instance variables; can access preexisting global vars
o objeval {
  set aaa 1
  set :a 1
  ? {info exists G} 1
}

? {o eval {info exists :a}} 1
? {o eval {info exists :aaa}} 1

? {info exists ::aaa} 0
unset -nocomplain ::aaa

# softeval (with -nonleaf) behaves like the initcmd and sets just
# instance variables via resolver.

o softeval {
  set bbb 1
  set :b 1
  ? {info exists G} 1
}
? {o eval {info exists :b}} 1
? {o eval {info exists :bbb}} 0

? {info vars ::bbb} ""
unset -nocomplain ::bbb

# softeval2 never sets instance variables
o softeval2 {
  set zzz 1
  set :z 1
  ? {info exists G} 1
}
? {o eval {info exists :z}} 0
? {o eval {info exists :zzz}} 0

? {info vars ::zzz} ::zzz
unset -nocomplain ::zzz

? {lsort [o info vars]} "a aaa b x"
o destroy

# now with an object namespace 
Object create o
o require namespace

# objeval does an objcope, all vars are instance variables
o objeval {
  set ccc 1
  set :c 1
}
? {o eval {info exists :c}} 1
? {o eval {info exists :ccc}} 1

# softeval behaves like the creation initcmd (just set dot vars)
o softeval {
  set ddd 1
  set :d 1
}
? {o eval {info exists :d}} 1
? {o eval {info exists :ddd}} 0

# softeval2 never sets variables
o softeval2 {
  set zzz 1
  set :z 1
}
? {o eval {info exists :z}} 0
? {o eval {info exists :zzz}} 0
? {lsort [o info vars]} "c ccc d"
o destroy

#################################################################
# The same as above, but with some global vars.  The global vars
# should not influence the behavior on instance variables
#################################################################
Test case with-global-vars
foreach var {.x x xxx :a a aaa :b b bbb :c c ccc :d d ddd :z z zzz} {set $var 1}

Object create o {
  set xxx 1
  set :x 1
}
? {o eval {info exists :x}} 1
? {o eval {info exists :xxx}} 0

# objeval does an objcope, all vars are instance variables
o objeval {
  set aaa 1
  set :a 1
}
? {o eval {info exists :a}} 1
? {o eval {info exists :aaa}} 1

# softeval should behave like the creation initcmd (just set dot vars)
o softeval {
  set bbb 1
  set :b 1
}
? {o eval {info exists :b}} 1
? {o eval {info exists :bbb}} 0

# softeval2 never sets instance variables
o softeval2 {
  set zzz 1
  set :z 1
}
? {o eval {info exists :z}} 0
? {o eval {info exists :zzz}} 0

? {lsort [o info vars]} "a aaa b x"
o destroy

# now with namespace 
Object create o
o require namespace

# eval does an objcope, all vars are instance variables
o objeval {
  set ccc 1
  set :c 1
}
? {o eval {info exists :c}} 1
? {o eval {info exists :ccc}} 1

# softeval2 should behave like the creation initcmd (just set dot vars)
o softeval {
  set ddd 1
  set :d 1
}
? {o eval {info exists :d}} 1
? {o eval {info exists :ddd}} 0

# softeval2 never sets variables
o softeval2 {
  set zzz 1
  set :z 1
}
? {o eval {info exists :z}} 0
? {o eval {info exists :zzz}} 0
? {lsort [o info vars]} "c ccc d"
o destroy

##################################################
# Test with proc scopes
##################################################
Test case proc-scopes
::nsf::alias ::nx::Object objscoped-eval -frame object ::eval 
::nsf::alias ::nx::Object nonleaf-eval -frame method ::eval 
::nsf::alias ::nx::Object plain-eval ::eval 

proc foo-via-initcmd {} {
  foreach v {x xxx} {unset -nocomplain ::$v}
  set p 1
  Object create o {
    set xxx 1
    set :x 1
    set ::result G=[info exists G],p=[info exists p]
  }
    return [o eval {info exists :x}]-[o eval {info exists :xxx}]-[info exists x]-[info exists xxx]-[info exists ::x]-[info exists ::xxx]-$::result
}

proc foo {type} {
  foreach v {x xxx} {unset -nocomplain ::$v}
  set p 1
  Object create o 
  o $type {
    set xxx 1
    set :x 1
    set ::result G=[info exists G],p=[info exists p]
  }
  return [o eval {info exists :x}]-[o eval {info exists :xxx}]-[info exists x]-[info exists xxx]-[info exists ::x]-[info exists ::xxx]-$::result
}

proc foo-tcl {what} {
  foreach v {x xxx} {unset -nocomplain ::$v}
  set p 1
  set body {
    set xxx 1
    set :x 1
    set ::result G=[info exists G],p=[info exists p]
  }
  switch $what {
    eval {eval $body}
    ns-eval {namespace eval [namespace current] $body}
  }
  return [o eval {info exists :x}]-[o eval {info exists :xxx}]-[info exists x]-[info exists xxx]-[info exists ::x]-[info exists ::xxx]-$::result
}

set G 1


 ? {foo-via-initcmd}    1-0-0-0-0-0-G=0,p=0
 ? {foo nonleaf-eval}   1-0-0-0-0-0-G=0,p=0
 ? {foo objscoped-eval} 1-1-0-0-0-0-G=0,p=0
 ? {foo plain-eval}     0-0-0-1-0-0-G=0,p=1
 ? {foo-tcl eval}       0-0-0-1-0-0-G=0,p=1
 ? {foo-tcl ns-eval}    0-0-0-0-0-1-G=1,p=0

##################################################
# dotCmd tests
##################################################
Test case dotcmd
set C 0
proc bar {} {incr ::C}
Class create Foo {
  :method init {} {set :c 0}
  :method callDot1 {} {:bar}
  :method callDot2 {} {:bar}
  :method callDot3 {} {:bar; ::bar; :bar}
  :method bar {} {incr :c}
}

Foo create f1
f1 callDot1
? {set ::C} 0
? {f1 eval {set :c}} 1

# call via callback
after 1 {f1 callDot2}
after 10 {set ::X 1}
vwait X

? {set ::C} 0
? {f1 eval {set :c}} 2

# call via callback, call :bar via .. from method
after 1 {f1 callDot3}
after 10 {set ::X 2}
vwait X

? {set ::C} 1
? {f1 eval {set :c}} 4


##################################################
# test for namespace resolver
##################################################
Test case nsresolver
namespace eval module {
  Class create C
  Class create M1
  Class create M2

  C mixin M1
  ? {::nsf::relation C class-mixin} "::module::M1"

  C mixin add M2
  ? {::nsf::relation C class-mixin} "::module::M2 ::module::M1"
}


##################################################
# test setting of instance variables for 
# objects with namespaces in and outside
# of an eval (one case uses compiler)
##################################################

Test case alias-dot-resolver-interp
# outside of eval scope (interpreted)
Class create V {
  set :Z 1
  set ZZZ 1
  :method bar {z} { return $z }
  :class-object method bar {z} { return $z }
  :create v {
    set zzz 2
    set :z 2
  }
}
? {lsort [V info vars]} {Z}
? {lsort [v info vars]} {z}

# dot-resolver/ dot-dispatcher used in aliased proc

Test case alias-dot-resolver {

  Class create V {
    set :Z 1
    set ZZZ 1
    :method bar {z} { return $z }
    :class-object method bar {z} { return $z }
    :create v {
      set :z 2
      set zzz 2
    }
  }
  ? {lsort [V info vars]} {Z}
  ? {lsort [v info vars]} {z}
}

#
# test [info vars] in eval method
#

Test case info-vars-in-eval {

  Object create o
  ? {o eval {
    set x 1
    expr {[info vars "x"] eq "x"}
  }} 1 
}

#
# test for former crash when variable is used in connection with
# prefixed variables
#
Test case tcl-variable-cmd {
  Object create o {
    :public method ? {varname} {info exists :$varname}
    :public method bar args {
      variable :a
      set a 3
      variable b
      set b 3
      variable c 1
      variable :d 1
      :info vars
    }
  }

  ? {o bar} d
  ? {o ? a} 0
  ? {o ? b} 0
  ? {o ? c} 0
  ? {o ? d} 1
  ? {lsort [o info vars]} d
  o eval {set :a 1}
  ? {o ? a} 1
  ? {lsort [o info vars]} "a d"
}
