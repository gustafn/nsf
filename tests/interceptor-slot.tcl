package require nx
package require nx::test

namespace import ::nx::*

Class create M {
  :method mfoo {} {puts [self proc]}
}
Class create M2
Class create C 

? {C info callable method mixin} "::nsf::classes::nx::Class::mixin"
C mixin M
? {C info precedence} "::nx::Class ::nx::Object"
? {C mixin} "::M"
? {C info mixin} "::M"
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
? {C object info mixin} "::M"
::nsf::relation C object-mixin ""
? {C info precedence} "::nx::Class ::nx::Object"

#
# adding, removing per-object mixins for classes through slot
# "object-mixin"
#
C object-mixin M
? {C info precedence} "::M ::nx::Class ::nx::Object"
? {C object info mixin} "::M"
C object-mixin ""
? {C info precedence} "::nx::Class ::nx::Object"

#
# add and remove object mixin for classes via modifier "object" and
# "mixin"
#
C object mixin M
? {C info precedence} "::M ::nx::Class ::nx::Object"
? {C object info mixin} "::M"
C object mixin ""
? {C info precedence} "::nx::Class ::nx::Object"

#
#  add and remove object mixin for classes via object mixin add
#
C object mixin add M
? {C info precedence} "::M ::nx::Class ::nx::Object"
? {C object info mixin} "::M"
C object mixin ""
? {C info precedence} "::nx::Class ::nx::Object"

#
# adding per-object mixins for classes via "object mixin add M"
#
C object mixin add M 
? {C info precedence} "::M ::nx::Class ::nx::Object"
? {::nsf::relation C object-mixin} ::M
? {catch {C object mixin add UNKNOWN}} 1
? {::nsf::relation C object-mixin} "::M"
C object mixin ""
? {C info precedence} "::nx::Class ::nx::Object"

#
# adding per-object mixins for classes via "object mixin M"
#
C object mixin M 
? {C info precedence} "::M ::nx::Class ::nx::Object"

# forwarder with 0 arguments + flag
? {C object-mixin} "::M"

puts stderr "==================== XOTcl"
package require XOTcl
namespace import -force ::xotcl::*

Class create M1
Class create M11
M1 instproc mfoo {} {puts [self proc]}
Class create C1 
? {C1 procsearch mixin} "::xotcl::Object instforward mixin"
C1 mixin M1
? {C1 info precedence} "::M1 ::xotcl::Class ::xotcl::Object"
C1 create c11
? {c11 info precedence} "::C1 ::xotcl::Object"
C1 mixin add M11
? {C1 info precedence} "::M11 ::M1 ::xotcl::Class ::xotcl::Object"
puts stderr ===obj-create+add
Object o -mixin M1
puts stderr ====[o info class]-[o procsearch mixin]-[Object info instforward -definition mixin]
? {o info precedence} "::M1 ::xotcl::Object"
puts stderr ===class-create+add
Class O 
O mixin M1
? {O info precedence} "::M1 ::xotcl::Class ::xotcl::Object"
puts stderr ===class-create+add-via-parameter
Class O -mixin M1
puts stderr ====[O info class]
? {O info precedence} "::M1 ::xotcl::Class ::xotcl::Object"
