# testing mixinof
package require nx; namespace import ::nx::*
package require nx::test

###########################################
# testing simple per object mixins
###########################################
Class create A
Object create o -mixin A
? {o mixin} ::A
? {o info mixin} ::A
? {A info mixinof} ::o

o destroy
? {A info mixinof} ""

A destroy

###########################################
# testing transitive per object mixins
###########################################

Class create B 
Class create C -superclass B

Class create M 
B mixin M

Object create o -mixin C
Object create o1 -mixin B
? {C info mixinof} ::o
? {lsort [B info mixinof -closure]} "::o ::o1"
? {lsort [B info mixinof -closure ::o1]} "::o1"
? {lsort [B info mixinof -closure ::o*]} "::o ::o1"
? {lsort [C info mixinof -closure ::o*]} "::o"
# A class is mixed into a per-object mixin class
? {lsort [M info mixinof -closure ::o*]} "::o ::o1"
? {lsort [M info mixinof -scope object]} ""

M destroy
B destroy
C destroy
::o destroy
::o1 destroy

###########################################
# testing per object mixins with redefinition
###########################################
Class create M {:method foo args {puts x;next}}
Object create o -mixin M 

? {o info mixin} ::M
? {o info precedence} "::M ::nx::Object"
? {o info callable -which foo} "::M method foo args {puts x;next}"

Class create M {:method foo args next}
? {o info mixin} ::M
? {o info precedence} "::M ::nx::Object"
? {o info callable -which foo} "::M method foo args next"

M destroy
? {o info mixin} ""
? {o info precedence} "::nx::Object"
? {o info callable -which foo} ""

o destroy

###########################################
# testing simple per class mixins
###########################################
Test case pcm
Class create A
Class create B -mixin A
Class create C -superclass B
C create c1

? {B mixin} ::A
? {B info mixin} ::A
? {A info mixinof} ::B
? {c1 info precedence} "::A ::C ::B ::nx::Object"

B destroy
? {A info mixinof} ""
? {c1 info precedence} "::C ::nx::Object"

A destroy
C destroy 
c1 destroy

###########################################
# testing simple per class mixins with guards
###########################################
Test case pcm2
Class create M1
Class create M2
Class create X
Class create A -mixin {M1 M2 X}
A mixinguard M1 "test"
Class create B -superclass A
? {A info mixin M2} ::M2
? {A info mixin M*} "::M1 ::M2"
? {A info mixin -guards} "{::M1 -guard test} ::M2 ::X"
? {B info mixin} ""
? {B info mixin -closure} "::M1 ::M2 ::X"
? {B info mixin -closure M2} ::M2
? {B info mixin -closure M*} "::M1 ::M2"
? {B info mixin -closure -guards} "{::M1 -guard test} ::M2 ::X"
? {B info mixin -closure -guards M1} "{::M1 -guard test}"
? {B info mixin -closure -guards M*} "{::M1 -guard test} ::M2"
A destroy
B destroy
X destroy
M1 destroy
M2 destroy

###########################################
# testing transitive per class mixins
###########################################
Test case trans-pcm1
Class create A
Class create B -mixin A
Class create C -superclass B
A mixin [Class create M]

A create a1
B create b1
C create c1

? {B mixin} ::A
? {B info mixin} ::A
? {A info mixinof -scope class} ::B
? {a1 info precedence} "::M ::A ::nx::Object"
? {b1 info precedence} "::M ::A ::B ::nx::Object"
? {c1 info precedence} "::M ::A ::C ::B ::nx::Object"

? {M info mixinof -scope class} "::A"
# since M is an instmixin of A and A is a instmixin of B,
# M is a instmixin of B as well, and of its subclasses
? {M info mixinof -scope class -closure} "::A ::B ::C"
? {A info mixinof -scope class} "::B"
? {A info mixinof -scope class -closure} "::B ::C"
? {B info mixinof -scope class} ""
? {B info mixinof -scope class -closure} ""

# and now destroy mixin classes
M destroy
? {a1 info precedence} "::A ::nx::Object"
? {b1 info precedence} "::A ::B ::nx::Object"
? {c1 info precedence} "::A ::C ::B ::nx::Object"

B destroy
? {A info mixinof -scope class} ""
? {c1 info precedence} "::C ::nx::Object"

foreach o {A C a1 b1 c1} { $o destroy }


###########################################
# testing transitive per class mixins with subclasses
###########################################
Test case trans-pcm2
Class create X
Class create D
Class create C -superclass D
Class create A -mixin C
Class create B -superclass A
B create b1

? {C info mixinof -scope class -closure} "::A ::B"
? {D info mixinof -scope class -closure} ""
? {A info mixinof -scope class -closure} ""
? {B info mixinof -scope class -closure} ""
? {X info mixinof -scope class -closure} ""
D mixin X
? {C info mixinof -scope class -closure} "::A ::B"
? {D info mixinof -scope class -closure} ""
? {A info mixinof -scope class -closure} ""
? {B info mixinof -scope class -closure} ""
? {X info mixinof -scope class -closure} "::D ::C ::A ::B"
? {b1 info precedence} "::C ::X ::D ::B ::A ::nx::Object"
B create b2
? {b2 info precedence} "::C ::X ::D ::B ::A ::nx::Object"

foreach o {X D C A B b1 b2} {$o destroy}

###########################################
# testing transitive per class mixins with subclasses
###########################################
Test case trans-pcm3
Class create A3 -superclass [Class create A2 -superclass [Class create A1]]
Class create B3 -superclass [Class create B2 -superclass [Class create B1 -superclass [Class create B0]]]
Class create C3 -superclass [Class create C2 -superclass [Class create C1]]

A2 mixin B2
B1 mixin C2

? {A1 info mixinof -scope class -closure} ""
? {A2 info mixinof -scope class -closure} ""
? {A3 info mixinof -scope class -closure} ""

? {B0 info mixinof -scope class -closure} ""
? {B1 info mixinof -scope class -closure} ""
? {B2 info mixinof -scope class -closure} "::A2 ::A3"
? {B3 info mixinof -scope class -closure} ""

? {C1 info mixinof -scope class -closure} ""
? {C2 info mixinof -scope class -closure} "::B1 ::B2 ::B3 ::A2 ::A3"
? {C3 info mixinof -scope class -closure} ""

foreach o {A1 A2 A3 B0 B1 B2 B3 C1 C2 C3} {$o destroy}

###########################################
# testing transitive per class mixins with destroy
###########################################
Test case pcm-trans-destroy-A
Class create A -mixin [Class create M]
Class create B -mixin A
Class create C -superclass B

A create a1
B create b1
C create c1

? {B mixin} ::A
? {B info mixin} ::A
? {A info mixinof -scope class} ::B
? {a1 info precedence} "::M ::A ::nx::Object"
? {b1 info precedence} "::M ::A ::B ::nx::Object"
? {c1 info precedence} "::M ::A ::C ::B ::nx::Object"

# and now destroy A
A destroy
? {a1 info precedence} "::nx::Object"
? {b1 info precedence} "::B ::nx::Object"
? {c1 info precedence} "::C ::B ::nx::Object"

? {M info mixinof} ""
? {M info mixinof -closure} ""

B destroy
? {M info mixinof -scope class} ""
? {c1 info precedence} "::C ::nx::Object"

foreach o {M C a1 b1 c1} { $o destroy }

###########################################
# testing transitive per class mixins with destroy
###########################################
Test case pcm-trans-destroy-B
Class create A -mixin [Class create M]
Class create B -mixin A
Class create C -superclass B

A create a1
B create b1
C create c1

? {B mixin} ::A
? {B info mixin} ::A
? {A info mixinof -scope class} ::B
? {a1 info precedence} "::M ::A ::nx::Object"
? {b1 info precedence} "::M ::A ::B ::nx::Object"
? {c1 info precedence} "::M ::A ::C ::B ::nx::Object"

B destroy
? {a1 info precedence} "::M ::A ::nx::Object"
? {b1 info precedence} "::nx::Object"
? {c1 info precedence} "::C ::nx::Object"

? {M info mixinof -scope class} "::A"
? {M info mixinof -scope class -closure} "::A"
? {A info mixinof -scope class} ""

foreach o {M C a1 b1 c1} {
  $o destroy
}

###########################################
# testing simple per class mixins with redefinition
###########################################
Test case pcm-redefine
Class create A
Class create B -mixin A
Class create C -superclass B
C create c1

? {B mixin} ::A
? {B info mixin} ::A
? {A info mixinof -scope class} ::B
? {c1 info precedence} "::A ::C ::B ::nx::Object"
? {B info heritage} "::nx::Object"
? {C info heritage} "::B ::nx::Object"

Class create B -mixin A

? {B info heritage} "::nx::Object"
? {C info heritage} "::nx::Object"
? {B mixin} ::A
? {B info mixin} ::A
? {A info mixinof} ::B
? {c1 info precedence} "::C ::nx::Object"

B destroy
? {A info mixinof} ""
? {c1 info precedence} "::C ::nx::Object"

A destroy
C destroy 
c1 destroy


###########################################
# testing simple per class mixins with 
# redefinition and softrecreate
###########################################
Test case pcm-redefine-soft
::nsf::configure softrecreate true
Class create A
Class create B -mixin A
Class create C -superclass B
C create c1

? {B mixin} ::A
? {B info mixin} ::A
? {A info mixinof -scope class} ::B
? {c1 info precedence} "::A ::C ::B ::nx::Object"
? {C info heritage} "::B ::nx::Object"
? {B info heritage} "::nx::Object"

Class create B -mixin A
? {C info heritage} "::B ::nx::Object"
? {B info heritage} "::nx::Object"
? {B info mixin} ::A
? {A info mixinof -scope class} ::B
? {c1 info precedence} "::A ::C ::B ::nx::Object"

B destroy
? {A info mixinof -scope class} ""
? {c1 info precedence} "::C ::nx::Object"

A destroy
C destroy 
c1 destroy


###########################################
# test of recreate with same superclass, 
# with softrecreate off
###########################################
Test case precedence
::nsf::configure softrecreate false
Class create O
Class create A -superclass O
Class create B -superclass A
B create b1
A create a1
O create o1
? {A info superclass} "::O"
? {B info heritage} "::A ::O ::nx::Object"
? {list [A info subclass]   [B info subclass]   [O info subclass]}   "::B {} ::A"
? {list [A info superclass] [B info superclass] [O info superclass]} "::O ::A ::nx::Object"
? {list [a1 info class]     [b1 info class]     [o1 info class]}     "::A ::B ::O"
? {o1 info precedence} "::O ::nx::Object"
? {a1 info precedence} "::A ::O ::nx::Object"
? {b1 info precedence} "::B ::A ::O ::nx::Object"
# we recreate the class new, with the same superclass
Class create A -superclass O
? {A info superclass} "::O"
? {B info heritage} "::nx::Object"
? {list [A info subclass]   [B info subclass]   [O info subclass]}   "{} {} ::A"
? {list [A info superclass] [B info superclass] [O info superclass]} "::O ::nx::Object ::nx::Object"
? {list [a1 info class]     [b1 info class]     [o1 info class]}     "::nx::Object ::B ::O"
? {o1 info precedence} "::O ::nx::Object"
? {a1 info precedence} "::nx::Object"
? {b1 info precedence} "::B ::nx::Object"
foreach o {A O B a1 b1 o1} {$o destroy} 

###########################################
# test of recreate with different superclass 
# with softrecreate on
###########################################
Test case alternate-precedence
::nsf::configure softrecreate false
Class create O
Class create A -superclass O
Class create B -superclass A
B create b1
A create a1
O create o1
? {A info superclass} "::O"
? {B info heritage}   "::A ::O ::nx::Object"
? {list [A info subclass]   [B info subclass]   [O info subclass]}   "::B {} ::A"
? {list [A info superclass] [B info superclass] [O info superclass]} "::O ::A ::nx::Object"
? {list [a1 info class]     [b1 info class]     [o1 info class]}     "::A ::B ::O"
? {o1 info precedence} "::O ::nx::Object"
? {a1 info precedence} "::A ::O ::nx::Object"
? {b1 info precedence} "::B ::A ::O ::nx::Object"
# we recreate the class new, with a different superclass
Class create A
? {A info superclass} "::nx::Object"
? {B info heritage}   "::nx::Object"
? {list [A info subclass]   [B info subclass]   [O info subclass]}   "{} {} {}"
? {list [A info superclass] [B info superclass] [O info superclass]} "::nx::Object ::nx::Object ::nx::Object"
? {list [a1 info class]     [b1 info class]     [o1 info class]}     "::nx::Object ::B ::O"
? {o1 info precedence} "::O ::nx::Object"
? {a1 info precedence} "::nx::Object"
? {b1 info precedence} "::B ::nx::Object"
foreach o {A O B a1 b1 o1} {$o destroy} 


###########################################
# test of recreate with same superclass, 
# with softrecreate on
###########################################
Test case recreate-precedence
::nsf::configure softrecreate true
Class create O
Class create A -superclass O
Class create B -superclass A
B create b1
A create a1
O create o1
? {A info superclass} "::O"
? {B info heritage} "::A ::O ::nx::Object"
? {list [A info subclass]   [B info subclass]   [O info subclass]}   "::B {} ::A"
? {list [A info superclass] [B info superclass] [O info superclass]} "::O ::A ::nx::Object"
? {list [a1 info class]     [b1 info class]     [o1 info class]}     "::A ::B ::O"
? {o1 info precedence} "::O ::nx::Object"
? {a1 info precedence} "::A ::O ::nx::Object"
? {b1 info precedence} "::B ::A ::O ::nx::Object"
# we recreate the class new, with the same superclass
Class create A -superclass O
? {A info superclass} "::O"
? {B info heritage} "::A ::O ::nx::Object"
? {list [A info subclass]   [B info subclass]   [O info subclass]}   "::B {} ::A"
? {list [A info superclass] [B info superclass] [O info superclass]} "::O ::A ::nx::Object"
? {list [a1 info class]     [b1 info class]     [o1 info class]}     "::A ::B ::O"
? {o1 info precedence} "::O ::nx::Object"
? {a1 info precedence} "::A ::O ::nx::Object"
? {b1 info precedence} "::B ::A ::O ::nx::Object"
foreach o {A O B a1 b1 o1} {$o destroy} 

###########################################
# test of recreate with different superclass 
# with softrecreate on
###########################################
Test case recreate-alternate-precedence
::nsf::configure softrecreate true
Class create O
Class create A -superclass O
Class create B -superclass A
B create b1
A create a1
O create o1
? {B info heritage} "::A ::O ::nx::Object"
? {list [A info subclass]   [B info subclass]   [O info subclass]}   "::B {} ::A"
? {list [A info superclass] [B info superclass] [O info superclass]} "::O ::A ::nx::Object"
? {list [a1 info class]     [b1 info class]     [o1 info class]}     "::A ::B ::O"
? {o1 info precedence} "::O ::nx::Object"
? {a1 info precedence} "::A ::O ::nx::Object"
? {b1 info precedence} "::B ::A ::O ::nx::Object"
# we recreate the class new, with a different superclass
Class create A
? {A info superclass} "::nx::Object"
? {B info heritage} "::A ::nx::Object"
? {B info heritage} "::A ::nx::Object"
? {list [A info subclass]   [B info subclass]   [O info subclass]}   "::B {} {}"
? {list [A info superclass] [B info superclass] [O info superclass]} "::nx::Object ::A ::nx::Object"
? {list [a1 info class]     [b1 info class]     [o1 info class]}     "::A ::B ::O"
? {o1 info precedence} "::O ::nx::Object"
? {a1 info precedence} "::A ::nx::Object"
? {b1 info precedence} "::B ::A ::nx::Object"
foreach o {A O B a1 b1 o1} {$o destroy} 

#foreach o [::nx::test::Test info instances] {$o destroy}
#::nx::test::Test destroy
#puts [lsort [::nx::Object allinstances]]

namespace import -force ::nx::*
###########################################
# testing simple per object mixins
###########################################
Test case nx-mixinof {
  Class create M
  Class create A
  Class create C
  C create c1 -mixin A
  C create c2
  Class create C2 -mixin A
  C2 create c22

  ? {c1 mixin} ::A
  ? {c1 info mixin} ::A
  ? {lsort [A info mixinof]} "::C2 ::c1"
  ? {M info mixinof} ""
  C mixin M
  #? {M info mixinof -scope object} "::c1 ::c2"
  ? {M info mixinof -scope object} ""
  ? {M info mixinof -scope class} "::C"
  ? {M info mixinof -scope all} "::C"
  ? {M info mixinof} "::C"

  ? {lsort [A info mixinof]} "::C2 ::c1"
  ? {A info mixinof -scope object} "::c1"
  ? {A info mixinof -scope class} "::C2"
  
  c1 destroy
  ? {A info mixinof} "::C2"
  ? {M info mixinof} "::C"
  C destroy
  ? {M info mixinof} ""
}


