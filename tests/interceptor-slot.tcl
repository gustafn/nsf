package require nx
package require nx::test

namespace import ::nx::*

Class create M {
  :method mfoo {} {puts [self proc]}
}
Class create M2
Class create C 

? {C info lookup method mixin} "::nsf::classes::nx::Class::mixin"
C mixin M
? {C info precedence} "::nx::Class ::nx::Object"
? {C mixin} "::M"
? {C info mixin classes} "::M"
C create c1
? {c1 info precedence} "::M ::C ::nx::Object"
C mixin add M2
? {c1 info precedence} "::M2 ::M ::C ::nx::Object"
C mixin delete M2
? {c1 info precedence} "::M ::C ::nx::Object"
C mixin delete M

# per-object mixins
? {c1 info precedence} "::C ::nx::Object"
c1 mixin add M
? {::nsf::relation c1 object-mixin} ::M
? {catch {c1 mixin UNKNOWN}} 1
? {::nsf::relation c1 object-mixin} "::M"

# add again the same mixin
c1 mixin add M
? {c1 info precedence} "::M ::C ::nx::Object"
c1 mixin add M2
? {c1 info precedence} "::M2 ::M ::C ::nx::Object"
c1 mixin delete M
? {c1 info precedence} "::M2 ::C ::nx::Object"
c1 mixin delete M2
? {c1 info precedence} "::C ::nx::Object"

#
# adding, removing per-object mixins for classes through relation
# "object-mixin"
#
::nsf::relation C object-mixin M
? {C info precedence} "::M ::nx::Class ::nx::Object"
? {C class-object info mixin classes} "::M"
::nsf::relation C object-mixin ""
? {C info precedence} "::nx::Class ::nx::Object"

#
# adding, removing per-object mixins for classes through slot
# "object-mixin"
#
# C object-mixin M
# ? {C info precedence} "::M ::nx::Class ::nx::Object"
# ? {C class-object info mixin classes} "::M"
# C object-mixin ""
# ? {C info precedence} "::nx::Class ::nx::Object"

#
# add and remove class-object mixin for classes via modifier "object" and
# "mixin"
#
C class-object mixin M
? {C info precedence} "::M ::nx::Class ::nx::Object"
? {C class-object info mixin classes} "::M"
C class-object mixin ""
? {C info precedence} "::nx::Class ::nx::Object"

#
#  add and remove class-object mixin for classes via class-object mixin add
#
C class-object mixin add M
? {C info precedence} "::M ::nx::Class ::nx::Object"
? {C class-object info mixin classes} "::M"
C class-object mixin ""
? {C info precedence} "::nx::Class ::nx::Object"

#
# adding per-object mixins for classes via "object mixin add M"
#
C class-object mixin add M 
? {C info precedence} "::M ::nx::Class ::nx::Object"
? {::nsf::relation C object-mixin} ::M
? {catch {C class-object mixin add UNKNOWN}} 1
? {::nsf::relation C object-mixin} "::M"
C class-object mixin ""
? {C info precedence} "::nx::Class ::nx::Object"

#
# adding per-object mixins for classes via "object mixin M"
#
C class-object mixin M 
? {C info precedence} "::M ::nx::Class ::nx::Object"

# forwarder with 0 arguments + flag
? {C class-object mixin} "::M"


Test case mixin-add {

  Class create M1 {
    :method mfoo {} {puts [current method]}
  }
  Class create M11
  Class create C1 

  ? {C1 info lookup method mixin} "::nsf::classes::nx::Class::mixin"
  C1 class-object mixin M1
  ? {C1 info precedence} "::M1 ::nx::Class ::nx::Object"
  C1 create c11
  ? {c11 info precedence} "::C1 ::nx::Object"
  C1 class-object mixin add M11
  ? {C1 info precedence} "::M11 ::M1 ::nx::Class ::nx::Object"
  Object create o -mixin M1
  ? {o info precedence} "::M1 ::nx::Object"

  Class create O 
  O class-object mixin M1
  ? {O info precedence} "::M1 ::nx::Class ::nx::Object"
  Class create O -object-mixin M1
  ? {O info precedence} "::M1 ::nx::Class ::nx::Object"
}

Test parameter count 3
Test case "filter-and-creation" {
  Class create Foo {
    :public method myfilter {args} {
      set i [::incr ::count]
      set s [self]
      set m [current calledmethod]
      #puts stderr "$i: $s.$m"
      #puts stderr "$i: procsearch before [$s procsearch info]"
      set r [next]
      #puts stderr "$i: $s.$m got ($r)"
      #puts stderr "$i: $s.$m procsearch after [$s info lookup method info]"
      return $r
    }
    # method for testing next to non-existing shadowed method
    :public method baz {} {next}
  }

  # define nx unknown handler in case it does not exist
  ::nx::Object protected method unknown {m args} {
    puts stderr XXXXX
    error "[::nsf::current object]: unable to dispatch method '$m'"
  }

  ? {Foo create ob} ::ob
  ? {ob bar} {::ob: unable to dispatch method 'bar'}
  ? {ob baz} {}

  Foo filter myfilter
  # create through filter
  ? {Foo create ob} ::ob
  # unknown through filter
puts stderr ======a
  ? {ob bar1} {::ob: unable to dispatch method 'bar1'}
puts stderr ======b
  ? {ob baz} {}

  # deactivate nx unknown handler in case it exists
  ::nx::Object method unknown {} {}

  # create through filter
  ? {Foo create ob2} ::ob2
  # unknown through filter
puts stderr ======c
  ? {ob2 bar2} {::ob2: unable to dispatch method 'bar2'}
puts stderr ======d
  ? {ob2 baz} {}

}
puts stderr ======EXIT




