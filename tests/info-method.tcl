package req nx
package require nx::test

nx::Object create o {
  :alias set ::set
}

nx::Class create C {
  :method m {x} {return proc-[self proc]}
  :object method mpo {} {return instproc-[self proc]}
  :method m-with-assertions {} {return proc-[self proc]} -precondition 1 -postcondition 2

  :forward addOne expr 1 +
  :object forward add1 expr 1 +
  :object forward fpo ::o

  :setter s 
  :object setter spo

  :alias a ::set
  :object alias apo ::puts
}
C create c1

? {lsort [C info methods -callprotection all]} "a addOne m m-with-assertions s"
#? {lsort [C info methods]} "a addOne s"
foreach m [lsort [C info methods -callprotection all]] {
  ? [subst -nocommands {lsort [c1 info callable methods $m]}] $m
}
? {C info method definition a} "::C alias a ::set"
? {c1 info callable method a} "::nsf::classes::C::a"
? {c1 info callable method addOne} "::nsf::classes::C::addOne"
? {c1 info callable method m} "::nsf::classes::C::m"
? {c1 info callable method s} "::nsf::classes::C::s"
c1 method foo {} {puts foo}
? {c1 info method definition foo} "::c1 method foo {} {puts foo}"
? {c1 info callable method foo} "::c1::foo"

? {C info method handle m} "::nsf::classes::C::m"
? {C object info method handle mpo} "::C::mpo"

? {C info method definition m} {::C method m x {return proc-[self proc]}}
? {C info method def m} {::C method m x {return proc-[self proc]}}
? {C object info method definition mpo} {::C object method mpo {} {return instproc-[self proc]}}
? {C info method definition m-with-assertions} \
    {::C method m-with-assertions {} {return proc-[self proc]} -precondition 1 -postcondition 2}
? {C info method parameter m} {x}
? {nx::Class info method parameter method} \
    {name arguments body -precondition -postcondition}
? {nx::Object info method parameter alias} \
    {-nonleaf:switch -objscope:switch methodName cmd}
# raises currently an error
? {catch {C info method parameter a}} 1
  
? {C info method definition addOne} "::C forward addOne expr 1 +"
? {C object info method definition add1} "::C object forward add1 expr 1 +"
? {C object info method definition fpo} "::C object forward fpo ::o"

? {C info method definition s} "::C setter s"
? {C object info method definition spo} "::C object setter spo"

? {C info method definition a} "::C alias a ::set"
? {C object info method definition apo} "::C object alias apo ::puts"


? {::nx::Object info callable methods -application} ""
? {::nx::Class info callable methods -application} ""
? {lsort [C info callable methods -application]} "add1 apo fpo mpo spo"
? {lsort [c1 info callable methods -application]} "a addOne foo m m-with-assertions s"

Test case callable {
    # define the same method for Object and Class
    ::nx::Object method bar {} {return Object.bar}
    ::nx::Class method bar {} {return Class.bar}

    ::nx::Object create o
    ? {o info callable method bar} "::nsf::classes::nx::Object::bar"
    ? {o info callable methods bar} bar
    ? {o bar} Object.bar

    o mixin ::nx::Class
    ? {o info callable method bar} "::nsf::classes::nx::Class::bar"
    ? {o info callable methods bar} bar
    ? {o bar} Class.bar

    ? {o method foo {} {return o.foo}} "::o::foo"
    ? {o info methods} "foo"
    ? {o mixin ""} ""
}
