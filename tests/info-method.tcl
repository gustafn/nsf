package req nx
package require nx::test

Test case base {
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
}

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
    ? {o info callable methods superclass} ""
    ? {o info callable method superclass} ""
    ? {o bar} Class.bar

    ? {o method foo {} {return o.foo}} "::o::foo"
    ? {o alias is ::nsf::objectproperty} "::o::is"
    ? {o setter x} "::o::x"
    ? {lsort [o info methods]} "foo is x"

    ? {o attribute A} ::o::A
    ? {o forward fwd ::set} ::o::fwd
    ? {lsort [o info methods]} "A foo fwd is x"

    o method f args ::nx::next
    ? {o info callable methods superclass} ""
    ? {o info callable methods filter} "filter"
    ? {o info callable method filter} "::nsf::classes::nx::Object::filter"
    ? {o filter f} ""
    ? {o filter guard f { 1 == 1 }} ""
    ? {o info filter guard f} " 1 == 1 "
    ? {o filter guard f} " 1 == 1 "
    o filter ""

    nx::Class create Foo
    ? {Foo method f args ::nx::next} "::nsf::classes::Foo::f"
    ? {Foo method f2 args ::nx::next} "::nsf::classes::Foo::f2"
    ? {Foo filter {f f2}} ""
    ? {Foo info filter methods} "f f2"
    ? {Foo filter guard f {2 == 2}} ""
    ? {Foo info filter guard f} "2 == 2"
    ? {Foo info filter methods -guards f} "{f -guard {2 == 2}}"
    ? {Foo info filter methods -guards f2} "f2"
    ? {Foo info filter methods -guards} "{f -guard {2 == 2}} f2"
    ? {Foo filter {}} ""

    ? {Foo object method f args ::nx::next} "::Foo::f"
    ? {Foo object method f2 args ::nx::next} "::Foo::f2"
    ? {Foo object filter {f f2}} ""
    ? {Foo object info filter methods} "f f2"
    ? {Foo object filter guard f {2 == 2}} ""
    ? {Foo object info filter guard f} "2 == 2"
    ? {Foo object info filter methods -guards f} "{f -guard {2 == 2}}"
    ? {Foo object info filter methods -guards f2} "f2"
    ? {Foo object info filter methods -guards} "{f -guard {2 == 2}} f2"
    ? {Foo object filter {}} ""
    Foo destroy 

    nx::Class create Fly
    o mixin add Fly
    ? {o info mixin classes} "::Fly ::nx::Class"
    ? {o mixin guard ::Fly {1}} ""
    ? {o info mixin classes -guards} "{::Fly -guard 1} ::nx::Class"
    ? {o info mixin classes -guards Fly} "{::Fly -guard 1}"
    o mixin delete ::Fly
    ? {o info mixin classes} "::nx::Class"

    nx::Class create Foo
    Foo mixin add ::nx::Class
    Foo mixin add Fly
    ? {Foo info mixin classes} "::Fly ::nx::Class"
    ? {Foo mixin guard ::Fly {1}} ""
    ? {Foo info mixin classes -guards} "{::Fly -guard 1} ::nx::Class"
    ? {Foo info mixin classes -guards Fly} "{::Fly -guard 1}"
    Foo mixin delete ::Fly
    ? {Foo info mixin classes} "::nx::Class"

    Foo object mixin add ::nx::Class
    Foo object mixin add Fly
    ? {Foo object info mixin classes} "::Fly ::nx::Class"
    ? {Foo object mixin guard ::Fly {1}} ""
    ? {Foo object info mixin classes -guards} "{::Fly -guard 1} ::nx::Class"
    ? {Foo object info mixin classes -guards Fly} "{::Fly -guard 1}"
    Foo object mixin delete ::Fly
    ? {Foo object info mixin classes} "::nx::Class"

    ? {Foo info callable methods superclass} "superclass"
    ? {Foo info callable method superclass} "::nsf::classes::nx::Class::superclass"
    
    ? {o mixin ""} ""
}
