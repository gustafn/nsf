# NX supports "open class definitions", object specific behavior
# and mixin classes (among other things) to achieve dynamic behavior
# extensions. The so-called per-object mixins are actually an
# implementation of the decorator pattern.

package req nx
package req nx::test

# Here is the original example: a method from a derived class
# extends the behavior of the baseclass; the primitive "next"
# calls other same-named methods. In the example below, the
# baseclass method "speak" is called at the beginning of the
# derived-class method. The primitive "next" can be placed at
# arbitrary places, or it can be omitted when the baseclass method
# should not be called.

nx::Class create BaseClass {
  :public method speak {} {
    puts "Hello from BC."
  }
}

nx::Class create DerivedClass -superclass BaseClass {
  :public method speak {} {
    next
    puts "Hello from DC."
  }
}

DerivedClass create o1
o1 speak

# The output is:
# ----
#    Hello from BC.
#    Hello from DC.
# ----

# There are many ways to extend the behavior NX classes at
# run time. The easiest thing is to add methods dynamically to
# classes. E.g. we can extend the BaseClass with the method unknown,
# which is called whenever an unknown method is called.

BaseClass method unknown {m args} {
  puts "What? $m? I don't understand."
}
o1 sing

# The output is:
# ----
#     What? sing? I don't understand.
# ----
#
# Often, you do not want to extend the class, but to
# modify the behavior of a single object. In NX, an object
# can have individual methods:

o1 public object method sing {} {
  puts "Ok, here it goes: Lala Lala!"
}
o1 sing

# The output is:
# ----
#     Ok, here it goes: Lala Lala!
# ----
#
# In many situations, it is desired to add/remove a set
# of methods dynamically to objects or classes. The mechanisms
# above allow this, but they are rather cumbersome and do
# support  a systematic behavior engineering. One can add
# so-called "mixin classes" to objects and/or classes. For
# example, we can define a class M for a more verbose methods:

nx::Class create M {
  :public method sing {} {
    puts -nonewline "[self] sings: "
    next
  }
  :method unknown args {
    puts -nonewline "[self] is confused: "
    next
  }
}

# The behavior of M can be mixed into the behavior of o1 through
# per object mixins ...
o1 object mixins set M

# ... and we call the methods again:
o1 sing
o1 read

# The output is:
# ----
#    ::o1 sings: Ok, here it goes: Lala Lala!
#    ::o1 is confused: What? read? I don't understand.
# ----
#
# We can remove the new behavior easily by unregistering the
# mixin class ...

o1 object mixins set ""; # implicit: overwrite using empty string
o1 object mixins clear; # explicit

# ... and we call the methods again:
o1 sing
o1 read

# The output is:
# ----
#    Ok, here it goes: Lala Lala!
#    What? read? I don't understand.
# ----
#
# Mixin classes can be used to extend the behavior of classes
# as well.

BaseClass mixins set M

o1 sing
o1 read

DerivedClass create o2
o2 read

# The output is:
# ----
#    ::o1 sings: Ok, here it goes: Lala Lala!
#    ::o1 is confused: What? read? I don't understand.
#    ::o2 is confused: What? read? I don't understand.
# ----
