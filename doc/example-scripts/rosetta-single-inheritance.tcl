#
# == Rosetta example: Inheritance/Single
#
# Show a tree of types which inherit from each other. The top of the
# tree should be a class called Animal. The second level should have
# Dog and Cat. Under Dog should be Lab and Collie.
# 
# https://rosettacode.org/wiki/Inheritance/Single
#

package req nx
package req nx::test

nx::Class create Animal
nx::Class create Dog -superclasses Animal
nx::Class create Cat -superclasses Animal
nx::Class create Collie -superclasses Dog
nx::Class create Lab -superclasses Dog

# Show the resulting class search order:
? {Lab info superclasses -closure} {::Dog ::Animal ::nx::Object}
? {[Collie new] info precedence} {::Collie ::Dog ::Animal ::nx::Object}

