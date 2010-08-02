package require next; namespace import -force ::nx::*
package require next::test

Test parameter count 10
Test case alias-preliminaries { 
  
  # The system methods of Object are either alias or forwarders
  ? {lsort [::nx::ObjectParameterSlot info methods -methodtype alias]} {assign get}
  ? {::nx::ObjectParameterSlot info method definition get} "::nx::ObjectParameterSlot alias get ::nx::core::setvar"

  # define an alias and retrieve its definition
  set cmd "::nx::Object alias -objscope set ::set"
  eval $cmd
  ? {Object info method definition set} $cmd
  
}

Test case alias-simple { 
  # define an alias and retrieve its definition
  Class create Base {
    :method foo {{-x 1}} {return $x}
  }

  Class create Foo
  ::nx::core::alias ::Foo foo ::nx::core::classes::Base::foo
  
  ? {Foo info method definition foo} "::Foo alias foo ::nx::core::classes::Base::foo"
  
  Foo create f1
  ? {f1 foo} 1
  ? {f1 foo -x 2} 2
  ? {Foo info methods -methodtype alias} "foo"
  
  ? {Base info methods -methodtype scripted} {foo}
  ? {Foo info methods -methodtype scripted} {}
  ? {Foo info methods -methodtype alias} {foo}
  Base method foo {} {}
  ? {Foo info methods -methodtype alias} ""
  ? {Base info methods -methodtype scripted} {}
  ? {Foo info methods -methodtype scripted} {}
  ? {Foo info method definition foo} ""
  

  Base method foo {{-x 1}} {return $x}
  ::nx::core::alias ::Foo foo ::nx::core::classes::Base::foo
  
  ? {Base info methods -methodtype scripted} {foo} "defined again"
  ? {Foo info methods -methodtype alias} {foo} "aliased again"
  Foo method foo {} {}
  ? {Base info methods -methodtype scripted} {foo} "still defined"
  ? {Foo info methods -methodtype alias} {} "removed"
}

Test case alias-chaining {
  #
  # chaining aliases
  #
  
  Class create T
  Class create S
  T create t
  S create s
  
  
  T method foo args { return [self class]->[self proc] }
  ::nx::core::alias T FOO ::nx::core::classes::T::foo 
  
  ? {t foo} ::T->foo
  ? {t FOO} ::T->foo
  
  ? {lsort [T info methods]} {FOO foo}
  T method foo {} {}
  ? {lsort [T info methods]} {} "alias is deleted"
  
  # puts stderr "double indirection"
  T method foo args { return [self class]->[self proc] }
  ::nx::core::alias T FOO ::nx::core::classes::T::foo 
  ::nx::core::alias S BAR ::nx::core::classes::T::FOO
  
  ? {T info methods -methodtype alias} "FOO"
  ? {T info method definition FOO} "::T alias FOO ::nx::core::classes::T::foo"
  ? {lsort [T info methods]} {FOO foo}
  ? {S info methods} {BAR}
  T method FOO {} {}
  ? {T info methods} {foo}
  ? {S info methods} {BAR}
  ? {s BAR} ::S->foo
  ? {t foo} ::T->foo
  ? {S info method definition BAR} "::S alias BAR ::nx::core::classes::T::FOO"
  
  
  T method foo {} {}
  ? {T info methods} {}
  ? {S info methods} {}
  
  T method foo args { return [self class]->[self proc] }
  ::nx::core::alias T FOO ::nx::core::classes::T::foo 
  ::nx::core::alias S BAR ::nx::core::classes::T::FOO
  
  ? {lsort [T info methods]} {FOO foo}
  ? {S info methods} {BAR}
  T method foo {} {}
  ? {S info methods} {}
  ? {T info methods} {}
  
  T method foo args { return [self class]->[self proc] }
  T object method bar args { return [self class]->[self proc] }
  ::nx::core::alias T -per-object FOO ::nx::core::classes::T::foo 
  ::nx::core::alias T -per-object BAR ::T::FOO 
  ::nx::core::alias T -per-object ZAP ::T::BAR 
  ? {T info methods} {foo}
  ? {lsort [T object info methods -methodtype alias]} {BAR FOO ZAP}
  ? {lsort [T object info methods]} {BAR FOO ZAP bar}
  ? {t foo} ::T->foo
  ? {T object info method definition ZAP} {::T object alias ZAP ::T::BAR}
  
  ? {T FOO} ->foo
  ? {T BAR} ->foo
  ? {T ZAP} ->foo
  ? {T bar} ->bar
  T object method FOO {} {}
  ? {T info methods} {foo}
  ? {lsort [T object info methods]} {BAR ZAP bar}
  ? {T BAR} ->foo
  ? {T ZAP} ->foo
  rename ::T::BAR ""
  ? {T info methods} {foo}
  ? {lsort [T object info methods]} {ZAP bar}
  #? {T BAR} ""; #  now calling the proc defined above, alias chain seems intact
  ? {T ZAP} ->foo; # is ok, still pointing to 'foo'
  #T object method BAR {} {}
  ? {T info methods} {foo}
  ? {lsort [T object info methods]} {ZAP bar}
  ? {T ZAP} ->foo
  T method foo {} {}
  ? {T info methods} {}
  ? {lsort [T object info methods]} {bar}
}

Test case alias-per-object {

  Class create T {
    :object method bar args { return [self class]->[self proc] }
    :create t
  }
  proc ::foo args { return [self class]->[self proc] }

  #
  # per-object methods as per-object aliases
  #
  T object method m1 args { return [self class]->[self proc] }
  ::nx::core::alias T -per-object M1 ::T::m1 
  ::nx::core::alias T -per-object M11 ::T::M1 
  ? {lsort [T object info methods]} {M1 M11 bar m1}
  ? {T m1} ->m1
  ? {T M1} ->m1
  ? {T M11} ->m1
  T object method M1 {} {}
  ? {lsort [T object info methods]} {M11 bar m1}
  ? {T m1} ->m1
  ? {T M11} ->m1
  T object method m1 {} {}
  ? {lsort [T object info methods]} {bar}
  
  #
  # a proc as alias
  #
  
  proc foo args { return [self class]->[self proc] }
  ::nx::core::alias T FOO1 ::foo 
  ::nx::core::alias T -per-object FOO2 ::foo
  #
  # ! per-object alias referenced as per-class alias !
  #
  ::nx::core::alias T BAR ::T::FOO2
  ? {lsort [T object info methods]} {FOO2 bar}
  ? {lsort [T info methods]} {BAR FOO1}
  ? {T FOO2} ->foo
  ? {t FOO1} ::T->foo
  ? {t BAR} ::T->foo
  #
  # delete proc
  #
  rename ::foo ""
  ? {lsort [T object info methods]} {bar}
  ? {lsort [T info methods]} {}
}


# namespaced procs + namespace deletion
Test case alias-namespaced {
  Class create T {
    :object method bar args { return [self class]->[self proc] }
    :create t
  }
  
  namespace eval ::ns1 {
    proc foo args { return [self class]->[self proc] }
    proc bar args { return [uplevel 2 {set _}] }
    proc bar2 args { upvar 2 _ __; return $__}
  }
  
  ::nx::core::alias T FOO ::ns1::foo
  ::nx::core::alias T BAR ::ns1::bar
  ::nx::core::alias T BAR2 ::ns1::bar2
  ? {lsort [T info methods]} {BAR BAR2 FOO}
  set ::_ GOTYA
  ? {t FOO} ::T->foo
  ? {t BAR} GOTYA
  ? {t BAR2} GOTYA
  namespace delete ::ns1
  ? {info procs ::ns1::*} {}
  ? {lsort [T info methods]} {}
  
  # per-object namespaces
  
  Class create U
  U create u
  ? {namespace exists ::U} 0
  U object method zap args { return [self class]->[self proc] }
  ::nx::core::alias ::U -per-object ZAP ::U::zap 
  U requireNamespace
  ? {namespace exists ::U} 1
  
  U object method bar args { return [self class]->[self proc] }
  ::nx::core::alias U -per-object BAR ::U::bar
  ? {lsort [U object info methods]} {BAR ZAP bar zap}
  ? {U BAR} ->bar
  ? {U ZAP} ->zap
  namespace delete ::U
  ? {namespace exists ::U} 0
  ? {lsort [U object info methods]} {}
  ? {U info callable BAR} ""
  ? {U info callable ZAP} ""
  
  ::U destroy
}

# dot-resolver/ dot-dispatcher used in aliased proc

Test case alias-dot-resolver {

  Class create V {
    set :z 1
    :method bar {z} { return $z }
    :object method bar {z} { return $z }
    :create v {
      set :z 2
    }
  }
  ? {lsort [V info vars]} {z}

  ? {lsort [V info vars]} {z}
  ? {lsort [v info vars]} {z}

  proc ::foo args { return [:bar ${:z}]-[set :z]-[:bar [set :z]] }

  ::nx::core::alias V FOO1 ::foo 
  ::nx::core::alias V -per-object FOO2 ::foo

  ? {lsort [V object info methods]} {FOO2 bar}
  ? {lsort [V info methods]} {FOO1 bar}

  ? {V FOO2} 1-1-1
  ? {v FOO1} 2-2-2
  V method FOO1 {} {}
  ? {lsort [V info methods]} {bar}
  rename ::foo ""
  ? {lsort [V object info methods]} {bar}
}

#
# Tests for the ::nx::core::alias store, used for introspection for
# aliases. The alias store (an associative variable) is mostly
# necessary for for the direct aliases (e.g. aliases to C implemented
# tcl commands), for which we have no stubs at the place where the
# alias was registered.
#

#
# structure of the ::nx::core::alias store:
# <object>,<alias_name>,<per_object> -> <aliased_cmd>
#

Object create o
Class create C

o method bar args {;}

? {info vars ::nx::core::alias} ::nx::core::alias
? {array exists ::nx::core::alias} 1 

proc ::foo args {;}
::nx::core::alias ::o FOO ::foo
::nx::core::alias ::C FOO ::foo
? {info exists ::nx::core::alias(::o,FOO,1)} 1
? {info exists ::nx::core::alias(::C,FOO,0)} 1
? {array get ::nx::core::alias ::o,FOO,1} "::o,FOO,1 ::foo"
? {array get ::nx::core::alias ::C,FOO,0} "::C,FOO,0 ::foo"
? {o info method definition FOO} "::o alias FOO ::foo"
? {C info method definition FOO} "::C alias FOO ::foo"

::nx::core::alias o FOO ::o::bar
? {info exists ::nx::core::alias(::o,FOO,1)} 1
? {array get ::nx::core::alias ::o,FOO,1} "::o,FOO,1 ::o::bar"
? {o info method definition FOO} "::o alias FOO ::o::bar"

# AliasDelete in XOTclRemoveObjectMethod
o method FOO {} {}
? {info exists ::nx::core::alias(::o,FOO,1)} 0
? {array get ::nx::core::alias ::o,FOO,1} ""
? {o info method definition FOO} ""

# AliasDelete in XOTclRemoveClassMethod
C method FOO {} {}
? {info exists ::nx::core::alias(::C,FOO,0)} 0
? {array get ::nx::core::alias ::C,FOO,0} ""
? {C info method definition FOO} ""

::nx::core::alias ::o BAR ::foo
::nx::core::alias ::C BAR ::foo  

# AliasDelete in XOTclAddObjectMethod
? {info exists ::nx::core::alias(::o,BAR,1)} 1
::o method BAR {} {;}
? {info exists ::nx::core::alias(::o,BAR,1)} 0

# AliasDelete in XOTclAddInstanceMethod
? {info exists ::nx::core::alias(::C,BAR,0)} 1
::C method BAR {} {;}
? {info exists ::nx::core::alias(::C,BAR,0)} 0

# AliasDelete in aliasCmdDeleteProc
::nx::core::alias o FOO ::foo
? {info exists ::nx::core::alias(::o,FOO,1)} 1
rename ::foo ""
? {info exists ::nx::core::alias(::o,FOO,1)} 0

::nx::core::alias o FOO ::o::bar
::nx::core::alias o BAR ::o::FOO
? {info exists ::nx::core::alias(::o,FOO,1)} 1
? {info exists ::nx::core::alias(::o,BAR,1)} 1
o method bar {} {}
? {info exists ::nx::core::alias(::o,FOO,1)} 0
? {info exists ::nx::core::alias(::o,BAR,1)} 0

#
# pulling the rug out from the proc-alias deletion mechanism
#

proc ::foo args {;}
::nx::core::alias C FOO ::foo
? {info exists ::nx::core::alias(::C,FOO,0)} 1
unset ::nx::core::alias(::C,FOO,0)
? {info exists ::nx::core::alias(::C,FOO,0)} 0
? {C info method definition FOO} ""
? {C info methods -methodtype alias} FOO
rename ::foo ""
? {C info methods -methodtype alias} ""
? {info exists ::nx::core::alias(::C,FOO,0)} 0
? {C info method definition FOO} ""

#
# test renaming of Tcl proc (actually sensed by the alias, though not
# reflected by the alias definition store)
# a) is this acceptable?
# b) sync ::nx::core::alias upon "info method definition" calls? is this feasible,
# e.g. through rename traces?
#

C create c
proc ::foo args { return [self]->[self proc]}
? {info exists ::nx::core::alias(::C,FOO,0)} 0
::nx::core::alias C FOO ::foo
? {info exists ::nx::core::alias(::C,FOO,0)} 1
? {C info methods -methodtype alias} FOO
rename ::foo ::foo2
? {info exists ::nx::core::alias(::C,FOO,0)} 1
? {C info methods -methodtype alias} FOO
? {c FOO} ::c->foo2
? {C info method definition FOO} "::C alias FOO ::foo"; # should be ::foo2 (!)
