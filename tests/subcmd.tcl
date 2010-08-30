package req nx
namespace import ::nx::*
package require nx::test

Test case submethods {
  #Object method unknown {} {}
  Object create o1   
  ? {o1 foo} "::o1: unable to dispatch method 'foo'"
  
  #
  # test subcmd "tricky" names
  # - names called on ensemble objects from C (defaultmethod, unknown)
  # - names equal to helper methods of the ensemble object
  #
  Object create o {
    :method "string length"  x {return [current method]}
    :method "string tolower" x {return [current method]}
    :method "string info" x {return [current method]}
    :method "foo a x" {} {return [current method]}
    :method "foo a y" {} {return [current method]}
    :method "foo a subcmdName" {} {return [current method]}
    :method "foo a defaultmethod" {} {return [current method]}
    :method "foo a unknown" args {return [current method]}
    :method "foo b" {} {return [current method]}
  }
  Class create Foo {
    :method "bar m1" {a:integer -flag} {;}
    :method "bar m2" {x:integer -y:boolean} {;}
    :method "baz a m1" {x:integer -y:boolean} {return m1}
    :method "baz a m2" {x:integer -y:boolean} {;}
    :method "baz b" {} {;}
  }
  
  ? {o string length 1} length
  ? {o string tolower 2} tolower
  ? {o string toupper 2} \
      {unable to dispatch method ::o string toupper; valid subcommands of string: info length tolower}
  
  ? {o foo a x} "x"
  ? {o foo a y} "y"
  ? {o foo a z} {unable to dispatch method ::o foo a z; valid subcommands of a: defaultmethod subcmdName unknown x y}
  
  ? {o info method type string} object
  # the following is a problem, when string has subcmd "info"
  #? {o::string info class} ::nx::EnsembleObject
  
  ? {o string length aaa} "length"
  ? {o string info class} "info"
  ? {o string hugo} \
      {unable to dispatch method ::o string hugo; valid subcommands of string: info length tolower}
  
  Foo create f1
  ? {f1 baz a m1 10} m1
  ? {f1 baz a m3 10} {unable to dispatch method <obj> baz a m3; valid subcommands of a: m1 m2}
}

Test parameter count 1 
Test case defaultmethod {
  Object create o {
    :method "string length"  x {return [current method]}
    :method "string tolower" x {return [current method]}
    :method "string info" x {return [current method]}
    :method "foo a x" {} {return [current method]}
    :method "foo a y" {} {return [current method]}
    :method "foo a subcmdName" {} {return [current method]}
    :method "foo a defaultmethod" {} {return [current method]}
    :method "foo a unknown" args {return [current method]}
    :method "foo b" {} {return [current method]}
  }
  Class create Foo {
    :method "bar m1" {a:integer -flag} {;}
    :method "bar m2" {x:integer -y:boolean} {;}
    :method "baz a m1" {x:integer -y:boolean} {return m1}
    :method "baz a m2" {x:integer -y:boolean} {;}
    :method "baz b" {} {;}
    :create f1
  }

  ? {o string} "info length tolower"
  ? {o foo} "a b"
  
  ? {f1 bar} "m1 m2"
  ? {f1 baz} "a b"
  ? {f1 baz a} "m1 m2"
}