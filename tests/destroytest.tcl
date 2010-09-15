package require nx; namespace import ::nx::*
package require nx::test

Test parameter count 10

::nsf::alias ::nx::Object set -objscope ::set

Class create O -superclass Object {
  :method init {} {
    set ::ObjectDestroy 0
    set ::firstDestroy 0
  }
  :method destroy {} {
    incr ::ObjectDestroy
    #[:info class] dealloc [current]
    next
  }
}

#
# classical simple case
#
set case "simple destroy (1)"
Test case simple-destroy-1
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
C method foo {} {
  puts stderr "==== $::case [current]"
  :destroy
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  ? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBBB"
  ? {::nsf::isobject c1} 1 "$::case object still exists in proc"
  ? "set ::firstDestroy" 1 "firstDestroy called"
  ? "set ::ObjectDestroy" 1 "ObjectDestroy called"
}
C create c1
c1 foo
puts stderr ======[::nsf::isobject c1]
? {::nsf::isobject c1} 0 "$::case object deleted"
? "set ::firstDestroy" 1 "firstDestroy called"


#
# simple case, destroy does not propagate, c1 survives
#
set case "simple destroy (2), destroy blocks"
Test case simple-destroy-2
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy block"}
C method foo {} {
  puts stderr "==== $::case [current]"
  :destroy
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  ? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBBB"
  ? {::nsf::isobject c1} 1 "$::case object still exists in proc"
  ? "set ::firstDestroy" 1 "firstDestroy called"
  ? "set ::ObjectDestroy" 0 "ObjectDestroy called"
}
C create c1
c1 foo
puts stderr ======[::nsf::isobject c1]
? {::nsf::isobject c1} 1 "$::case object deleted"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 0 "ObjectDestroy called"

#
# simple object recreate
#
set case "recreate"
Test case recreate
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
C method foo {} {
  puts stderr "==== $::case [current]"
  [:info class] create [current]
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  ? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBBB"
  ? {::nsf::isobject c1} 1 "$::case object still exists in proc"
  ? "set ::firstDestroy" 0 "firstDestroy called"
  ? "set ::ObjectDestroy" 0 "ObjectDestroy called"
}
C create c1
c1 foo
puts stderr ======[::nsf::isobject c1]
? {::nsf::isobject c1} 1 "$::case object deleted"
? "set ::firstDestroy" 0 "firstDestroy called"

#
# cmd rename to empty, xotcl provides its own rename and calls destroy
# .. like simple case above
#
set case "cmd rename empty (1)"
Test case rename-empty-1
Object create o
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
C method foo {} {
  puts stderr "==== $::case [current]"
  rename [current] ""
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  ? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBB"
  ? {::nsf::isobject c1} 1 "$::case object still exists in proc"
  ? "set ::firstDestroy" 1 "firstDestroy called"
  ? "set ::ObjectDestroy" 1 "ObjectDestroy called"
}
C create c1
c1 foo
puts stderr ======[::nsf::isobject c1]
? {::nsf::isobject c1} 0 "$::case object still exists after proc"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 1 "ObjectDestroy called"

#
# cmd rename to empty, xotcl provides its own rename and calls
# destroy, but destroy does not propagate, c1 survives rename, since
# this is the situation like above, as long xotcl's rename is used.
#
set case "cmd rename empty (2)"
Test case rename-empty-2
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy block"}
C method foo {} {
  puts stderr "==== $::case [current]"
  rename [current] ""
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  ? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBB"
  ? {::nsf::isobject c1} 1 "$::case object still exists in proc"
  ? "set ::firstDestroy" 1 "firstDestroy called"
  ? "set ::ObjectDestroy" 0 "ObjectDestroy called"
}
C create c1
c1 foo
puts stderr ======[::nsf::isobject c1]
puts stderr ======[c1 set x]
? {::nsf::isobject c1} 1 "$::case object still exists after proc"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 0 "ObjectDestroy called"

#
# cmd rename other xotcl object to current, 
# xotcl's rename invokes a move 
#
set case "cmd rename object to current"
Test case rename-to-current
Object create o
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
C method foo {} {
  puts stderr "==== $::case [current]"
  rename o [current]
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  ? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBB"
  ? {::nsf::isobject c1} 1 "$::case object still exists in proc"
  ? "set ::firstDestroy" 0 "firstDestroy called"
  ? "set ::ObjectDestroy" 0 "ObjectDestroy called"
}
C create c1
c1 foo
puts stderr ======[::nsf::isobject c1]
? {::nsf::isobject c1} 1 "$::case object still exists after proc"
? "set ::firstDestroy" 0 "firstDestroy called"
? "set ::ObjectDestroy" 0 "ObjectDestroy called"

#
# cmd rename other proc to current object, 
# xotcl's rename invokes a move 
#
set case "cmd rename proc to current"
Test case rename-proc-to-current
proc o args {}
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
C method foo {} {
  puts stderr "==== $::case [current]"
  set x [catch {rename o [current]}]
  ? "set _ $x" 1 "$::case tcl refuses to rename into an existing command"
}
C create c1
c1 foo
? {::nsf::isobject c1} 1 "$::case object still exists after proc"
? "set ::firstDestroy" 0 "firstDestroy called"
? "set ::ObjectDestroy" 0 "ObjectDestroy called"


#
# namespace delete: tcl delays delete until the namespace is not
# active anymore. destroy is called after BBBB. Hypothesis: destroy is
# called only when we are lucky, since C might be destroyed before c1
# by the namespace delete
#

set case "delete parent namespace (1)"
Test case delete-parent-namespace
namespace eval ::test {
  Class create C -superclass O
  C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
  C method foo {} {
    puts stderr "==== $::case [current]"
    namespace delete ::test
    puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
    :set x 1
    #
    # If the following line is commented in, the namespace is deleted
    # here. Is there a bug with nsPtr->activationCount
    #
    #? "[current] set x" 1 "$::case can still access [current]"
    puts stderr "BBB"
    puts stderr "???? [current] exists [::nsf::isobject [current]]"
    ? "::nsf::isobject [current]" 0 ;# WHY?
    puts stderr "???? [current] exists [::nsf::isobject [current]]"
    ? "set ::firstDestroy" 0 "firstDestroy called"
    ? "set ::ObjectDestroy" 0 "$::case destroy not yet called"
  }
}
test::C create test::c1
test::c1 foo
puts stderr ======[::nsf::isobject test::c1]
? {::nsf::isobject test::c1} 0  "object still exists after proc"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 1 "destroy was called when poping stack frame"
? {::nsf::isobject ::test::C} 0  "class still exists after proc"
? {namespace exists ::test::C} 0  "namespace ::test::C still exists after proc"
? {namespace exists ::test} 1  "parent ::test namespace still exists after proc"
? {namespace exists ::xotcl::classes::test::C} 0  "namespace ::xotcl::classes::test::C still exists after proc"

#
# namespace delete: tcl delays delete until the namespace is not
# active anymore. destroy is called after BBBB, but does not
# propagate.  
#
set case "delete parent namespace (2)"
Test case delete-parent-namespace-2
namespace eval ::test {
  ? {namespace exists test::C} 0 "exists test::C"
  Class create C -superclass O
  C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy block"}
  C method foo {} {
    puts stderr "==== $::case [current]"
    namespace delete ::test
    puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
    :set x 1
    #
    # If the following line is commented in, the namespace is deleted
    # here. Is there a bug with nsPtr->activationCount
    #
    #? "[current] set x" 1 "$::case can still access [current]"
    puts stderr "BBBB"
    puts stderr "???? [current] exists [::nsf::isobject [current]]"
    ? "::nsf::isobject [current]" 0 "$::case object still exists in proc";# WHY?
    puts stderr "???? [current] exists [::nsf::isobject [current]]"
    ? "set ::firstDestroy" 0 "firstDestroy called"
    ? "set ::ObjectDestroy" 0  "ObjectDestroy called"; # NOT YET CALLED
  }
}
test::C create test::c1
test::c1 foo
puts stderr ======[::nsf::isobject test::c1]
? {::nsf::isobject test::c1} 0  "$::case object still exists after proc"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 0 "ObjectDestroy called"  ;# toplevel destroy was blocked

#
# controlled namespace delete: xotcl has its own namespace cleanup,
# topological order should be always ok. however, the object o::c1 is
# already deleted, while a method of it is excuted
#
set case "delete parent object (1)"
Test case delete-parent-object
Object create o
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
C method foo {} {
  puts stderr "==== $::case [current]"
  o destroy
  puts stderr "AAAA"
  # the following isobject call has a problem in Tcl_GetCommandFromObj(), 
  # which tries to access invalid memory
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  #? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBBB"
  ? {::nsf::isobject ::o::c1} 0 "$::case object still exists in proc"
  ? "set ::firstDestroy" 1 "firstDestroy called"
  ? "set ::ObjectDestroy" 1 "ObjectDestroy called"
}
C create o::c1
o::c1 foo

puts stderr ======[::nsf::isobject ::o::c1]
? {::nsf::isobject ::o::c1} 0 "$::case object o::c1 still exists after proc"
? {::nsf::isobject o} 0 "$::case object o still exists after proc"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 1 "ObjectDestroy called"

#
# controlled namespace delete: xotcl has its own namespace cleanup.
# destroy does not delegate, but still o::c1 does not survive, since o
# is deleted.
#
set case "delete parent object (2)"
Test case delete-parent-object-2
Object create o
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy block"}
C method foo {} {
  puts stderr "==== $::case [current]"
  o destroy
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  #? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBB"
  ? {::nsf::isobject ::o::c1} 0 "$::case object still exists in proc"
  ? "set ::firstDestroy" 1 "firstDestroy called"
  ? "set ::ObjectDestroy" 0 "ObjectDestroy called"
}
C create o::c1
o::c1 foo
puts stderr ======[::nsf::isobject ::o::c1]
? {::nsf::isobject ::o::c1} 0 "$::case object still exists after proc"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 0 "ObjectDestroy called"


#
# create an other cmd with the current object's name. 
# xotcl 1.6 crashed on this test
#
set case "redefined current object as proc"
Test case redefined-current-object-as-proc
Object create o
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
C method foo {} {
  puts stderr "==== $::case [current]"
  proc [current] {args} {puts HELLO}
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  #? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBB"
  ? "set ::firstDestroy" 1 "firstDestroy called"
  ? "set ::ObjectDestroy" 1 "ObjectDestroy called"
  ? {::nsf::isobject c1} 0 "$::case object still exists in proc"
}
C create c1
c1 foo
puts stderr ======[::nsf::isobject c1]
? {::nsf::isobject c1} 0 "$::case object still exists after proc"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 1 "ObjectDestroy called"



#
# delete the active class
#
set case "delete active class"
Test case delete-active-class
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
C method foo {} {
  puts stderr "==== $::case [current]"
  C destroy
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  #? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBB"
  #? [:info class] ::xotcl::Object "object reclassed"
  ? [:info class] ::C "object reclassed?"
  ? "set ::firstDestroy" 0 "firstDestroy called"
  ? "set ::ObjectDestroy" 0 "ObjectDestroy called"
  ? {::nsf::isobject c1} 1 "object still exists in proc"
  #? {::nsf::is class ::C} 0 "class still exists in proc"
  ? {::nsf::is class ::C} 1 "class still exists in proc"
}
C create c1
c1 foo
puts stderr ======[::nsf::isobject c1]
? {::nsf::isobject c1} 1 "object still exists after proc"
? [c1 info class] ::nx::Object "after proc: object reclassed?"
? "set ::firstDestroy" 0 "firstDestroy called"
? "set ::ObjectDestroy" 0 "ObjectDestroy called"

#
# delete active object nested in class
#
set case "delete active object nested in class"
Test case delete-active-object-nested-in-class
Class create C -superclass O
C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
C method foo {} {
  puts stderr "==== $::case [current]"
  C destroy
  puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
  :set x 1
  #? "[current] set x" 1 "$::case can still access [current]"
  puts stderr "BBB"
  #? "set ::firstDestroy" 0 "firstDestroy called"
  ? "set ::firstDestroy" 1 "firstDestroy called"
  #? "set ::ObjectDestroy" 0 "ObjectDestroy called"
  ? "set ::ObjectDestroy" 1 "ObjectDestroy called"
  ? [:info class] ::C "object reclassed"
  #? [:info class] ::xotcl::Object "object reclassed"
  ? {::nsf::isobject ::C::c1} 1 "object still exists in proc"
  ? {::nsf::is class ::C} 1 "class still exists in proc"
}
C create ::C::c1
C::c1 foo
#puts stderr ======[::nsf::isobject ::C::c1]
? {::nsf::isobject ::C::c1} 0 "object still exists after proc"
? {::nsf::is class ::C} 0 "class still exists after proc"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 1 "ObjectDestroy called"

#
Test case nesting-destroy {
  Object create x
  Object create x::y
  x destroy
  ? {::nsf::isobject x} 0 "parent object gone"
  ? {::nsf::isobject x::y} 0 "child object gone"
}

Test case deleting-aliased-object {
  Object create o
  Object create o2
  ::nsf::alias o a o2
  ? {o a} ::o2 "call object via alias"
  ? {o info method type a} alias
  ## the ensemble-object needs per-object methods
  o2 method info args {:info {*}$args}
  o2 method set args {:set {*}$args}
  ? {o a info vars} "" "call info on aliased object"
  ? {o set x 10} 10   "set variable on object"
  ? {o info vars} x   "query vars"
  ? {o a info vars} x  "query vars via alias"
  ? {o a set x} 10     "set var via alias"
  o2 destroy
  ? {o a info vars} "Trying to dispatch deleted object via method 'a'" "1st call on deleted object"
  ? {o a info vars} "::o: unable to dispatch method 'a'" "2nd call on deleted object"
}

set case "deleting object with alias to object"
Test case deleting-object-with-alias-to-object
Object create o
Object create o3
::nsf::alias o x o3
o destroy
? {::nsf::isobject o} 0 "parent object gone"
? {::nsf::isobject o3} 1 "aliased object still here"
o3 destroy
? {::nsf::isobject o3} 0 "aliased object destroyed"

set case "create an alias, and delete cmd via aggregation"
Test case create-alias-delete-via-aggregation
Object create o
Object create o3
::nsf::alias o x o3
o::x destroy
? {::nsf::isobject o3} 0 "aliased object destroyed"
o destroy

#
# create an alias, and recreate obj
#
Test case create-alias-and-recreate-obj {
  Object create o
  Object create o3
  o alias x o3
  Object create o3
  o3 method set args {:set {*}$args}
  o set a 13
  ? {o x set a} 13 "aliased object works after recreate"
}

#
# create an alias on the class level, double aliasing, delete aliased
# object
#
Test case create-alias-on-class-delete-aliased-obj {
  Class create C
  Object create o
  Object create o3
  o alias a o3
  C alias b o

  o3 method set args {:set {*}$args}
  o method set args {:set {*}$args}

  C create c1
  ? {c1 b set B 2} 2 "call 1st level"
  ? {c1 b a set A 3} 3 "call 2nd level"
  
  ? {c1 set B} 2 "call 1st level ok"
  ? {c1 set A} 3 "call 2nd level ok"
  o destroy
  ? {c1 b} "Trying to dispatch deleted object via method 'b'" "call via alias to deleted object"
}

#
# create an alias on the class level, double aliasing, destroy class
#
Test case create-alias-on-class-destroy-class {
  Class create C
  Object create o
  Object create o3
  o alias a o3
  C alias b o
  C create c1
  C destroy
  ? {::nsf::isobject o} 1 "object o still here"
  ? {::nsf::isobject o3} 1 "object o3 still here"
}

#
# test cases where preexisting namespaces are re-used
#

Test case module {
  # create a namespace with an object/class in it
  namespace eval ::module { Object create foo }
  
  # reuse the namespace for a class/object
  Class create ::module

  ? {::nsf::is class ::module} 1

  # delete the object/class ... and namespace
  ::module destroy

  ? {::nsf::is class ::module} 0
}

Test case namespace-import {

  namespace eval ::module {
    Class create Foo {
      :create foo
    }
    namespace export Foo foo
  }
  Class create ::module {
    :create mod1
  }
  ? {::nsf::is class ::module::Foo} 1
  ? {::nsf::is class ::module::foo} 0
  ? {::nsf::isobject ::module::foo} 1
  ? {::nsf::is class ::module} 1

  Object create ::o { :require namespace }
  namespace eval ::o {namespace import ::module::*}

  ? {::nsf::is class ::o::Foo} 1
  ? {::nsf::isobject ::o::foo} 1

  # do not destroy namespace imported objects/classes
  ::o destroy

  ? {::nsf::is class ::o::Foo} 0
  ? {::nsf::isobject ::o::foo} 0

  ? {::nsf::is class ::module::Foo} 1
  ? {::nsf::isobject ::module::foo} 1

  ::module destroy
}

# to avoid CallDirectly, we could activate this line
::nx::Class create M {:method dealloc args {next}}
Test case delete-parent-namespace-dealloc
namespace eval ::test {
  Class create C -superclass O
  C method destroy {} {incr ::firstDestroy; puts stderr "  *** [current] destroy"; next}
  C method foo {} {
    puts stderr "==== $::case [current]"
    namespace delete ::test
    puts stderr "AAAA [current] exists [::nsf::isobject [current]]"
    :set x 1
    #
    # If the following line is commented in, the namespace is deleted
    # here. Is there a bug with nsPtr->activationCount
    #
    #? "[current] set x" 1 "$::case can still access [current]"
    puts stderr "BBB"
    puts stderr "???? [current] exists [::nsf::isobject [current]]"
    ? "::nsf::isobject [current]" 0 ;# WHY?
    puts stderr "???? [current] exists [::nsf::isobject [current]]"
    ? "set ::firstDestroy" 0 "firstDestroy called"
    ? "set ::ObjectDestroy" 0 "$::case destroy not yet called"
  }
}
test::C create test::c1
test::c1 foo
puts stderr ======[::nsf::isobject test::c1]
? {::nsf::isobject test::c1} 0  "object still exists after proc"
? "set ::firstDestroy" 1 "firstDestroy called"
? "set ::ObjectDestroy" 1 "destroy was called when poping stack frame"
? {::nsf::isobject ::test::C} 0  "class still exists after proc"
? {namespace exists ::test::C} 0  "namespace ::test::C still exists after proc"
? {namespace exists ::test} 1  "parent ::test namespace still exists after proc"
? {namespace exists ::xotcl::classes::test::C} 0  "namespace ::xotcl::classes::test::C still exists after proc"

puts stderr "==== EXIT ===="
exit

TODO:
fix crashes in regression test: DONE, 
     -> well we can't call traceprocs on the object being destroyed; maybe call CleanupDestroyObject() ealier
  move destroy logic to activationCount DONE
  simplify logic (remove callIsDestroy, callstate XOTCL_CSC_CALL_IS_DESTROY, destroyedCmd on stack content) DONE
  remove CallStackMarkDestroyed(), CallStackMarkUndestroyed() DONE
  remove traces of rst->callIsDestroy DONE
  revive tclStack (without 85) DONE
  check state changes DONE
  delete active class; maybe C destroy, c1 destroy (or C::c1 + C destroy) DONE
  add recreate logic test case DONE

more generic */
XOTCLINLINE static Tcl_ObjType *
GetCmdNameType(Tcl_ObjType *cmdType) {

  MATRIX