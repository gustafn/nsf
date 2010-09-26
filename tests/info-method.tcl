package req nx
::nx::configure defaultMethodProtection false
package require nx::test

Test case base {
  nx::Object create o {
    :alias set ::set
  }

  nx::Class create C {
    :method m {x} {return proc-[self proc]}
    :class-object method mpo {} {return instproc-[self proc]}
    :method m-with-assertions {} {return proc-[self proc]} -precondition 1 -postcondition 2
    
    :forward addOne expr 1 +
    :class-object forward add1 expr 1 +
    :class-object forward fpo ::o
    
    :setter s 
    :class-object setter spo
    
    :alias a ::set
    :class-object alias apo ::puts
  }
  C create c1
  
  ? {lsort [C info methods -callprotection all]} "a addOne m m-with-assertions s"
  #? {lsort [C info methods]} "a addOne s"
  foreach m [lsort [C info methods -callprotection all]] {
    ? [subst -nocommands {lsort [c1 info lookup methods $m]}] $m
  }
  ? {C info method definition a} "::C alias a ::set"
  ? {c1 info lookup method a} "::nsf::classes::C::a"
  ? {c1 info lookup method addOne} "::nsf::classes::C::addOne"
  ? {c1 info lookup method m} "::nsf::classes::C::m"
  ? {c1 info lookup method s} "::nsf::classes::C::s"
  c1 method foo {} {puts foo}
  ? {c1 info method definition foo} "::c1 method foo {} {puts foo}"
  ? {c1 info lookup method foo} "::c1::foo"
  
  ? {C info method handle m} "::nsf::classes::C::m"
  ? {C class-object info method handle mpo} "::C::mpo"
  
  ? {C info method definition m} {::C method m x {return proc-[self proc]}}
  ? {C info method def m} {::C method m x {return proc-[self proc]}}
  ? {C class-object info method definition mpo} {::C class-object method mpo {} {return instproc-[self proc]}}
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
  ? {C class-object info method definition add1} "::C class-object forward add1 expr 1 +"
  ? {C class-object info method definition fpo} "::C class-object forward fpo ::o"
  
  ? {C info method definition s} "::C setter s"
  ? {C class-object info method definition spo} "::C class-object setter spo"
  
  ? {C info method definition a} "::C alias a ::set"
  ? {C class-object info method definition apo} "::C class-object alias apo ::puts"
  
  ? {::nx::Object info lookup methods -source application} ""
  ? {::nx::Class info lookup methods -source application} ""

  set object_methods "__default_method_protection alias attribute class configure contains copy destroy eval filter forward info method mixin move protected public require setter volatile vwait"
  set class_methods "__default_method_protection alias alloc attribute attributes class class-object configure contains copy create dealloc destroy eval filter forward info method mixin move new protected public require setter superclass volatile vwait"

  ? {lsort [::nx::Object info lookup methods -source baseclasses]} $class_methods
  ? {lsort [::nx::Class info lookup methods -source baseclasses]} $class_methods
  ? {lsort [::nx::Object info lookup methods -source all]} $class_methods
  ? {lsort [::nx::Class info lookup methods -source all]} $class_methods
  ? {lsort [::nx::Object info lookup methods]} $class_methods
  ? {lsort [::nx::Class info lookup methods]} $class_methods
  ? {lsort [C info lookup methods -source application]} "add1 apo fpo mpo spo"
  ? {lsort [c1 info lookup methods -source application]} "a addOne foo m m-with-assertions s"
  ? {lsort [C info lookup methods -source baseclasses]} $class_methods
  ? {lsort [c1 info lookup methods -source baseclasses]} $object_methods

  ::nx::configure defaultMethodProtection true
  #
  # the subsequent tests assume defaultMethodProtection == true
  #
  ? {::nx::configure defaultMethodProtection} true
  
  ::nx::Class create MC -superclass ::nx::Class {
    :protected method bar1 args {;}
    :method bar2 args {;}
    :public method foo args {;}
    :public class-object method foo args {;}
  }

  ? {lsort [MC info methods -methodtype scripted -callprotection public]} "foo"
  ? {lsort [MC info methods -methodtype scripted -callprotection protected]} "bar1 bar2"
  ? {lsort [MC info methods -methodtype scripted -callprotection all]} "bar1 bar2 foo"
  

  ::nsf::methodproperty ::MC foo protected true
  ::nsf::methodproperty ::MC bar2 protected false
  
  ? {lsort [MC info methods -methodtype scripted -callprotection public]} "bar2"
  ? {lsort [MC info methods -methodtype scripted -callprotection protected]} "bar1 foo"
  ? {lsort [MC info methods -methodtype scripted -callprotection all]} "bar1 bar2 foo"
  ::nx::configure defaultMethodProtection false
}

Test case subobj {
  ::nx::Object create o {
    ::nx::Object create [::nx::self]::sub {
      :method foo {} {;}
    }
    :alias subal ::o::sub
  }
  ? {o info methods} "sub subal"
  ? {o info method type sub} "object"
  ? {o info method definition sub} "::nx::Object create ::o::sub"
  ? {o info method type subal} "alias"
}

Test case callable {
    # define the same method for Object and Class
    ::nx::Object method bar {} {return Object.bar}
    ::nx::Class method bar {} {return Class.bar}

    ::nx::Object create o
    ? {o info lookup method bar} "::nsf::classes::nx::Object::bar"
    ? {o info lookup methods bar} bar
    ? {o bar} Object.bar

    o mixin ::nx::Class
    ? {o info lookup method bar} "::nsf::classes::nx::Class::bar"
    ? {o info lookup methods bar} bar
    ? {o info lookup methods superclass} ""
    ? {o info lookup method superclass} ""
    ? {o bar} Class.bar

    ? {o method foo {} {return o.foo}} "::o::foo"
    ? {o alias is ::nsf::is} "::o::is"
    ? {o setter x} "::o::x"
    ? {lsort [o info methods]} "foo is x"

    ? {o attribute A} ::o::A
    ? {o forward fwd ::set} ::o::fwd
    ? {lsort [o info methods]} "A foo fwd is x"

    o method f args ::nx::next
    ? {o info lookup methods superclass} ""
    ? {o info lookup methods filter} "filter"
    ? {o info lookup method filter} "::nsf::classes::nx::Object::filter"
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

    ? {Foo class-object method f args ::nx::next} "::Foo::f"
    ? {Foo class-object method f2 args ::nx::next} "::Foo::f2"
    ? {Foo class-object filter {f f2}} ""
    ? {Foo class-object info filter methods} "f f2"
    ? {Foo class-object filter guard f {2 == 2}} ""
    ? {Foo class-object info filter guard f} "2 == 2"
    ? {Foo class-object info filter methods -guards f} "{f -guard {2 == 2}}"
    ? {Foo class-object info filter methods -guards f2} "f2"
    ? {Foo class-object info filter methods -guards} "{f -guard {2 == 2}} f2"
    ? {Foo class-object filter {}} ""
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

    Foo class-object mixin add ::nx::Class
    Foo class-object mixin add Fly
    ? {Foo class-object info mixin classes} "::Fly ::nx::Class"
    ? {Foo class-object mixin guard ::Fly {1}} ""
    ? {Foo class-object info mixin classes -guards} "{::Fly -guard 1} ::nx::Class"
    ? {Foo class-object info mixin classes -guards Fly} "{::Fly -guard 1}"
    Foo class-object mixin delete ::Fly
    ? {Foo class-object info mixin classes} "::nx::Class"

    ? {Foo info lookup methods superclass} "superclass"
    ? {Foo info lookup method superclass} "::nsf::classes::nx::Class::superclass"
    
    ? {o mixin ""} ""
}

Test case slots {

  nx::Class create C {
    :attribute a
    :attribute {b 1}
  }
  
  nx::Class create D -superclass C {
    :attribute {b 2}
    :attribute c
    :class-object attribute a2
    :method "sub foo" args {;}
  }
  
  D create d1
  ? {D info lookup slots} "::nx::Class::slot::object-mixin ::nx::Class::slot::mixin ::nx::Class::slot::superclass ::nx::Class::slot::object-filter ::nx::Class::slot::filter ::nx::Object::slot::class"
  ? {D info slots} "::D::slot::b ::D::slot::a2 ::D::slot::c"
  ? {::nx::Object info method parameter info} ""
}


Test case info-submethod {

  nx::Object create o {
    :method "foo a" {} {return a}
    :method "foo b" {x:int y:upper} {return b}
  }
  nx::Object create o2
  
  nx::Class create C {
    :method "bar a" {} {return a}
    :method "bar b" {x:int y:upper} {return b}
    :method "bar baz x" {x:int y:upper} {return x}
    :method "bar baz y" {x:int y:upper} {return y}
    :class-object method "foo x" {z:int} {return z}
    :class-object method "foo y" {z:int} {return z}
  }

  # query definition on submethod
  ? {o info method definition "foo b"}  {::o method {foo b} {x:int y:upper} {return b}}

  # query definition on submethod with handle
  ? {o info method definition "::o::foo b"}  {::o method {foo b} {x:int y:upper} {return b}}

  # query definition on submethod with handle
  ? {o info method definition "::o::foo b"}  {::o method {foo b} {x:int y:upper} {return b}}

  # query definition on submethod with handle called on different object
  ? {o2 info method definition "::o::foo b"}  {::o method {foo b} {x:int y:upper} {return b}}

  # query definition on handle of ensemble object called on different object
  ? {o2 info method definition "::o::foo::b"} {::o::foo method b {x:int y:upper} {return b}}

  # query definition on submethod with handle called on class
  ? {o2 info method definition "::o::foo b"}  {::o method {foo b} {x:int y:upper} {return b}}

  # query definition on handle of ensemble object called on class
  ? {o2 info method definition "::o::foo::b"} {::o::foo method b {x:int y:upper} {return b}}

  # query definition on submethod of class
  ? {::nx::Object info method definition "info lookup methods"} \
      {::nx::Object alias {info lookup methods} ::nsf::methods::object::info::lookupmethods}

  # query definition on submethod of class with handle
  ? {o info method definition "::nsf::classes::nx::Object::info lookup methods"} \
      {::nx::Object alias {info lookup methods} ::nsf::methods::object::info::lookupmethods}

  # query definition on handle of ensemble object of class
  ? {o info method definition "::nx::Object::slot::__info::lookup::methods"} \
      {::nx::Object::slot::__info::lookup alias methods ::nsf::methods::object::info::lookupmethods}

  ? {lsort [o info method submethods dummy]} ""
  ? {lsort [o info method submethods foo]} "a b"
  ? {lsort [o info method submethods "foo a"]} ""

  ? {lsort [C info method submethods "bar"]} "a b baz"
  ? {lsort [C info method submethods "bar a"]} ""
  ? {lsort [C info method submethods "bar baz"]} "x y"
  ? {lsort [C info method submethods "bar baz y"]} ""

  ? {lsort [C class-object info method submethods "foo"]} "x y"
  ? {lsort [C class-object info method submethods "foo x"]} ""

  ? {C info method handle "bar"} {::nsf::classes::C::bar}
  ? {C info method handle "bar a"} {::nsf::classes::C::bar a}
  ? {C info method handle "bar baz y"} {::nsf::classes::C::bar baz y}

  ? {C info method definition "bar b"} {::C method {bar b} {x:int y:upper} {return b}}
  ? {C info method definition "::nsf::classes::C::bar b"}  {::C method {bar b} {x:int y:upper} {return b}}
  ? {o2 info method definition "::nsf::classes::C::bar b"} {::C method {bar b} {x:int y:upper} {return b}}

  ? {C class-object info method handle "foo"}   {::C::foo}
  ? {C class-object info method handle "foo x"} {::C::foo x}

  ? {C class-object info method definition "::C::foo x"} {::C class-object method {foo x} z:int {return z}}
  ? {C info method definition "::C::foo x"}        {::C class-object method {foo x} z:int {return z}}
  ? {o2 info method definition "::C::foo x"}       {::C class-object method {foo x} z:int {return z}}
  
  ? {C info method definition "bar baz y"} \
      {::C method {bar baz y} {x:int y:upper} {return y}}
  ? {C info method definition "::nsf::classes::C::bar baz y"} \
      {::C method {bar baz y} {x:int y:upper} {return y}}

  ? {nx::Object info method parameter "info lookup methods"} \
      "-methodtype -callprotection -source -nomixins -incontext pattern:optional"
  ? {o info method parameter "foo b"} "x:int y:upper"

  ? {nx::Object info method parameter ::nx::Object::slot::__info::lookup::methods} \
      "-methodtype -callprotection -source -nomixins -incontext pattern:optional"
  ? {o info method parameter "::o::foo::b"} "x:int y:upper"

  ? {nx::Object info method handle "info"} "::nsf::classes::nx::Object::info"
  ? {nx::Object info method handle "info lookup methods"} \
      "::nsf::classes::nx::Object::info lookup methods"

  ? {nx::Object info method handle "::nsf::classes::nx::Object::info lookup methods"} \
      "::nsf::classes::nx::Object::info lookup methods"

  ? {o info method handle "foo b"} "::o::foo b"
}
