package req nx
namespace import ::nx::*
::nx::configure defaultMethodProtection false
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

#
# testing ensemble objects with next
#
Test parameter count 1
Test case ensemble-next {

  nx::Class create FOO {
    # reduced ensemble
    :method foo args {lappend :v "FOO.foo//[nx::current method] ([nx::current args])"}

    # expanded ensemble
    :method "l1 l2 l3a" {x} {
      lappend :v "FOO.l1 l2 l3a//[nx::current method] ([nx::current args])"
    }
    :method "l1 l2 l3b" {x} {
      lappend :v "FOO.l1 l2 l3b//[nx::current method] ([nx::current args])"
    }
    # uplevel
    :method "bar x" {varname} {upvar $varname v; return [info exists v]}
    :method "baz" {} {
      set hugo 1
      return [:bar x hugo]
    }
  }
  nx::Class create M0 {
    :method "foo b x" {x} {
      lappend :v "M0.foo b x//[nx::current method] ([nx::current args])"
      nx::next
    }
    :method "foo b y" {x} {
      lappend :v "M0.foo b y//[nx::current method] ([nx::current args])"
      nx::next
    }
    :method "foo a" {x} {
      lappend :v "M0.foo a//[nx::current method] ([nx::current args])"
      nx::next
    }

    :method "l1 l2" {args} {
      lappend :v "l1 l2//[nx::current method] ([nx::current args])"
      nx::next
    }
  }

  nx::Class create M1 {
    :method "foo a" {x} {
      set :v [list "M1.foo a //[nx::current method] ([nx::current args])"]
      nx::next
    }
    :method "foo b x" {x} {
      set :v  [list "M1.foo b x //[nx::current method] ([nx::current args])"]
      nx::next
    }
    :method "foo b y" {x} {
      set :v  [list "M1.foo b y //[nx::current method] ([nx::current args])"]
      nx::next
    }

    :method "l1 l2 l3a" {x} {
      set :v  [list "M1.l1 l2 l3a//[nx::current method] ([nx::current args])"]
      nx::next
    }
    :method "l1 l2 l3b" {x} {
      set :v  [list "M1.l1 l2 l3b//[nx::current method] ([nx::current args])"]
      nx::next
    }
  }
  
  FOO mixin {M1 M0}
  FOO create f1
  
  #
  # The last list element shows handling of less deep ensembles
  # (longer arg list is passed)
  #
  ? {f1 foo a 1} "{M1.foo a //a (1)} {M0.foo a//a (1)} {FOO.foo//foo (a 1)}"
  ? {f1 foo b x 1} "{M1.foo b x //x (1)} {M0.foo b x//x (1)} {FOO.foo//foo (b x 1)}"
  ? {f1 foo b y 1} "{M1.foo b y //y (1)} {M0.foo b y//y (1)} {FOO.foo//foo (b y 1)}"
  #
  # The middle list element shows shrinking (less deep ensembles), the
  # last element shows expansion via mixin (deeper ensemble is reached
  # via next)
  #
  ? {f1 l1 l2 l3a 100} "{M1.l1 l2 l3a//l3a (100)} {l1 l2//l2 (l3a 100)} {FOO.l1 l2 l3a//l3a (100)}"
}

Test case ensemble-partial-next {
  nx::Class create M {
    :method "info has namespace" {} {
      next
      return sometimes
    }
    :method "info has something else" {} {
      return something
    }
    :method "info has something better" {} {
      next
      return better
    }
  }
  nx::Object mixin add M
  nx::Object create o1

  # call a submethod defined by a mixin, which does a next
  ? {o1 info has namespace} sometimes

  # call a submethod, which is not defined by the mixin
  ? {o1 info has type Object} 1
  ? {o1 info has type M} 0

  # call a submethod, which is nowhere defined
  ? {o1 info has typo M} \
      {unable to dispatch method <obj> info has typo; valid subcommands of has: namespace something}
  # call a submethod, which is only defined in the mixin
  ? {o1 info has something else} something

  # call a submethod, which is only defined in the mixin, and which
  # does a next (which should not complain)
  ? {o1 info has something better} better
}

#
# Check behavior of upvars in ensemble methods
#
Test case ensemble-upvar {

  nx::Class create FOO {
    :method "bar0 x" {varname} {upvar $varname v; return [info exists v]}
    :method "baz0" {} {
      set hugo 1
      return [:bar0 x hugo]
    }
    :method "bar1 x" {varname} {:upvar $varname v; return [info exists v]}
    :method "baz1" {} {
      set hugo 1
      return [:bar1 x hugo]
    }
    :create f1
  }
 
  ? {f1 baz0} 0
  ? {f1 baz1} 1
}

#
# Check behavior of next with arguments within an ensemble
#
Test case ensemble-next-with-args {
  nx::Object create o {
    :method foo {x}          {return $x}
    :method "e1 sm" {x}      {return $x}
    :method "e2 sm1 sm2" {x} {return $x}
    :method "e2 e2 e2" {x}   {return $x}
    :method "e1 e1 e1" args  {return $args}
  }
  nx::Class create M {
    :method foo {}          {next 1}
    :method "e1 sm" {}      {next 2}
    :method "e2 sm1 sm2" {} {next 3}
    :method "e2 e2 e2" {}   {next 4}
    :method "e1 e1 e1" args {next {e1 e1 e1}}
  }
  o mixin add M

  # case without ensemble
  ? {o foo} 1

  # ensemble depth 1, 1 arg
  ? {o e1 sm} 2

  # ensemble depth 2, 1 arg
  ? {o e2 sm1 sm2} 3

  # ensemble depth 2, 1 arg, same tcl-objs
  ? {o e2 e2 e2} 4

  # ensemble depth 2, multiple args, same tcl-objs
  ? {o e1 e1 e1} {e1 e1 e1}
}
