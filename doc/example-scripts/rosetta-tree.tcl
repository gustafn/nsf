#
# == Rosetta example:https://rosettacode.org/wiki/Tree_traversal
#
#
# Implement a binary tree structure, with each node carrying an
# integer as a node label, and four traversal strategies: pre-order,
# in-order, postorder, and levelorder traversals.
# 
# https://rosettacode.org/wiki/Tree_traversal
#

package req nx
package req nx::test

#
# The class +Tree+ implements the basic binary composite structure (left, right).
#

nx::Class create Tree {
    :property -accessor public value:required
    :property -accessor public left:object,type=[current]
    :property -accessor public right:object,type=[current]

    :public method traverse {order} {
	set list {}
	:$order v {
	    lappend list $v
	}
	return $list
    }
 
    # Traversal methods
    :public method preOrder {varName script {level 0}} {
	upvar [incr level] $varName var
	set var ${:value}
	uplevel $level $script
	if {[info exists :left]} {${:left} preOrder $varName $script $level}
	if {[info exists :right]} {${:right} preOrder $varName $script $level}
    }
    
    :public method inOrder {varName script {level 0}} {
	upvar [incr level] $varName var
	if {[info exists :left]} {${:left} inOrder $varName $script $level}
	set var ${:value}
	uplevel $level $script
	if {[info exists :right]} {${:right} inOrder $varName $script $level}
    }
    :public method postOrder {varName script {level 0}} {
	upvar [incr level] $varName var
	if {[info exists :left]} {${:left} postOrder $varName $script $level}
	if {[info exists :right]} {${:right} postOrder $varName $script $level}
	set var ${:value}
	uplevel $level $script
    }
    :public method levelOrder {varName script} {
	upvar 1 $varName var
	set nodes [list [current]]
	while {[llength $nodes] > 0} {
	    set nodes [lassign $nodes n]
	    set var [$n value get]
	    uplevel 1 $script
	    if {[$n eval {info exists :left}]} {lappend nodes [$n left get]}
	    if {[$n eval {info exists :right}]} {lappend nodes [$n right get]}
	}
    }
}

#
# This is a factory method to build up the object tree recursively
# from a nested Tcl list. Note that we create left and right childs by
# nesting them in their parent, this provides for a cascading cleanup
# of an entire tree (there is no need for an explicit cascading of
# +destroy+ methods down the composite).
#

Tree public object method newFromList {-parent l} {
    lassign $l value left right
    set n [:new {*}[expr {[info exists parent]?[list -childof $parent]:""}] -value $value]
    set props [list]
    if {$left ne ""} {lappend props -left [:newFromList -parent $n $left]}
    if {$right ne ""} {lappend props -right [:newFromList -parent $n $right]}
    $n configure {*}$props
    return $n
}

# Run the required tests:

set t [Tree newFromList {1 {2 {4 7} 5} {3 {6 8 9}}}]
? {$t traverse preOrder} {1 2 4 7 5 3 6 8 9}
? {$t traverse inOrder} {7 4 2 5 1 8 6 9 3}
? {$t traverse postOrder}  {7 4 5 2 8 9 6 3 1}
? {$t traverse levelOrder} {1 2 3 4 5 6 7 8 9}
