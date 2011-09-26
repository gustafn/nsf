# Implementation study based on 
#
#   S. Ducasse, O. Nierstrasz, N. Schärli, R. Wuyts, A. Black:
#   Traits: A Mechanism for Fine-grained Reuse,
#   ACM transactions on Programming Language Systems, Vol 28, No 2, March 2006
#
# Example in Fig 12: ReadStream and Trait TReadStream
#
# In this example, traits are used to extend classes and other traits.

package require nx::test
package require nx::trait

#
# Create a simple trait called +TReadStream+ which provides the
# interface to a stream. In contrary to a composite trait, a simple
# trait does not inherit from another trait.
#
nx::Trait create TReadStream {
  #
  # Define the methods provided by this trait:
  #
  :public method atStart {} {expr {[:position] == [:minPosition]}}
  :public method atEnd {} {expr {[:position] == [:maxPosition]}}
  :public method setToStart {} {set :position [:minPosition]}
  :public method setToEnd {} {set :position [:maxPosition]}
  :public method maxPosition {} {llength ${:collection}}
  :public method on {collection} {set :collection $collection; :setToStart}
  :public method next {} {
    if {[:atEnd]} {return ""} else {
      set r [lindex ${:collection} ${:position}]
      :nextPosition
      return $r
    }
  }
  :public method minPosition {} {return 0}
  :public method nextPosition {} {incr :position 1}
  
  # This trait requires a method "position" and a variable
  # "collection" from the base class. The definition is incomplete in
  # these regards.

  :requiredMethods position
  :requiredVariables collection
}

# Define the class +ReadStream+ with properties +position+ and
# +collection+ that uses the trait. The method +useTrait+ checks the
# requirements of the trait and imports the methods into +ReadStream+.

nx::Class create ReadStream {
  :property {collection ""}
  :property {position 0}
  :useTrait TReadStream
}

# Create an instance of the class +ReadStream+:
ReadStream create r1 -collection {a b c d e}

# Test the behavior of the composed class:
? {r1 atStart} 1
? {r1 atEnd} 0
? {r1 next} a
? {r1 next} b
