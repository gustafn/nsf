# Implementation study based on 
#
# S. Ducasse, O. Nierstrasz, N. Schärli, R. Wuyts, A. Black:
# Traits: A Mechanism for Fine-grained Reuse,
# ACM transactions on Programming Language Systems, Vol 28, No 2, March 2006
#
# Fig 13: TReadStream and TWriteStream as composite traits
#
# In this example, traits are used to extend classes and other traits
# (called then _composite traits_).
#
package req nx::test
package req nx::trait

#
# Create a Trait called +TPositionableStream+
#
nx::Trait create TPositionableStream {
  #
  # Define the methods provided by this trait:
  #  
  :public method atStart {} {expr {[:position] == [:minPosition]}}
  :public method atEnd {} {expr {[:position] == [:maxPosition]}}
  :public method setToStart {} {set :position [:minPosition]}
  :public method setToEnd {} {set :position [:maxPosition]}
  :public method maxPosition {} {llength ${:collection}}
  :public method minPosition {} {return 0}
  :public method nextPosition {} {incr :position 1}

  # The trait requires a method "position" and a variable "collection"
  # from the base class or other traits. The definition is incomplete
  # in these regards

  :requiredMethods position
  :requiredVariables collection
}

#
# Create a composite trait called +TReadStream+ based on the trait
# +TPositionableStream+:
#
nx::Trait create TReadStream {
  #
  # Methods provided by this trait:
  #  
  :public method on {collection} {set :collection $collection; :setToStart}
  :public method next {} {
    if {[:atEnd]} {return ""} else {
      set r [lindex ${:collection} ${:position}]
      :nextPosition
      return $r
    }
  }
  
  # This trait requires these methods:
  :requiredMethods {setToStart atEnd nextPosition}

  # Use the trait "TPositionableStream"
  :useTrait TPositionableStream
}

#
# Create a composite trait called +TWriteStream+ based on the trait
# +TPositionableStream+:
#
nx::Trait create TWriteStream {
  #
  # Methods provided by this trait:
  #  
  :public method on {collection} {set :collection $collection; :setToEnd}
  :public method nextPut {element} {
    lappend :collection $element
    :nextPosition    
    return ""
  }

  # This trait requires these methods:
  :requiredMethods {setToEnd nextPosition}

  # Use the trait "TPositionableStream"
  :useTrait TPositionableStream
}

# Define a class +ReadStream+ with properties +position+ and
# +collection+ that uses the composite trait +TReadStream+:
nx::Class create ReadStream {
  :property {collection ""}
  :property {position 0}
  :useTrait TReadStream
}

# Create an instance of +ReadStream+:
ReadStream create r1 -collection {a b c d e}

# Test the behavior of the composed class:
? {r1 atStart} 1
? {r1 atEnd} 0
? {r1 next} a
? {r1 next} b
