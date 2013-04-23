# Implementation study based on 
#
# S. Ducasse, O. Nierstrasz, N. Schärli, R. Wuyts, A. Black:
# Traits: A Mechanism for Fine-grained Reuse,
# ACM transactions on Programming Language Systems, Vol 28, No 2, March 2006
#
# Fig 13: tReadStream and tWriteStream as composite traits
#
# In this example, traits are used to extend classes and other traits
# (called then _composite traits_).
#
package req nx::test
package req nx::trait

#
# Create a Trait called +tPositionableStream+
#
nx::Trait create tPositionableStream {
  #
  # Define the methods provided by this trait:
  #  
  :public object method atStart {} {expr {[:position] == [:minPosition]}}
  :public object method atEnd {} {expr {[:position] == [:maxPosition]}}
  :public object method setToStart {} {set :position [:minPosition]}
  :public object method setToEnd {} {set :position [:maxPosition]}
  :public object method maxPosition {} {llength ${:collection}}
  :public object method minPosition {} {return 0}
  :public object method nextPosition {} {incr :position 1}

  # The trait requires a method "position" and a variable "collection"
  # from the base class or other traits. The definition is incomplete
  # in these regards

  :requiredMethods position
  :requiredVariables collection
}

#
# Create a composite trait called +tReadStream+ based on the trait
# +tPositionableStream+:
#
nx::Trait create tReadStream {
  #
  # Methods provided by this trait:
  #  
  :public object method on {collection} {set :collection $collection; :setToStart}
  :public object method next {} {
    if {[:atEnd]} {return ""} else {
      set r [lindex ${:collection} ${:position}]
      :nextPosition
      return $r
    }
  }
  
  # This trait requires these methods:
  :requiredMethods {setToStart atEnd nextPosition}

  # Require the trait "tPositionableStream"
  :require trait tPositionableStream
}

#
# Create a composite trait called +tWriteStream+ based on the trait
# +tPositionableStream+:
#
nx::Trait create tWriteStream {
  #
  # Methods provided by this trait:
  #  
  :public object method on {collection} {set :collection $collection; :setToEnd}
  :public object method nextPut {element} {
    lappend :collection $element
    :nextPosition    
    return ""
  }

  # This trait requires these methods:
  :requiredMethods {setToEnd nextPosition}

  # Require the trait "tPositionableStream"
  :require trait tPositionableStream
}

# Define a class +ReadStream+ with properties +position+ and
# +collection+ that uses the composite trait +tReadStream+:
nx::Class create ReadStream {
  :property {collection ""}
  :property {position 0}
  :require trait tReadStream
}

# Create an instance of +ReadStream+:
ReadStream create r1 -collection {a b c d e}

# Test the behavior of the composed class:
? {r1 atStart} 1
? {r1 atEnd} 0
? {r1 next} a
? {r1 next} b
