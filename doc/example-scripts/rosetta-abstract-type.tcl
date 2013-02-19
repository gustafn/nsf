#
# == Rosetta Example: Abstract type 
#
# Define a class without instances and without implemented methods.
# For detailed description of this example 
# see http://rosettacode.org/wiki/Abstract_type
#
package req nx
package req nx::test

#
# Define a class AbstractQueue
nx::Class create AbstractQueue {
  
  :public method enqueue {item} {error "not implemented"}
  :public method dequeue {} {error "not implemented"}
 
  :public class method create {args} {
    error "Cannot instantiate abstract class [self]"
  }
}

#
# Define a concrete queue (named ListQueue) based 
# on the Abstract Queue
nx::Class create ListQueue -superclass AbstractQueue {
  :variable list {}
  :public method enqueue {item} {
    lappend :list $item
  }
  :public method dequeue {} {
    set item [lindex ${:list} 0]
    set :list [lrange ${:list} 1 end]
    return $item
  }
}

# === Demonstrating the behavior in a shell: 
#
# Trying to create an instance of the AbstraceQueue returns an error message:
? {AbstractQueue new} {Cannot instantiate abstract class ::AbstractQueue}

# Create an instance of the concrete queue:
? {set q [ListQueue new]} "::nsf::__#1"

# Enqueue and dequeue items
? {$q enqueue 100} 100
? {$q enqueue 101} "100 101"
? {$q dequeue} 100