package require nx
package require nx::trait
package provide nx::callback 1.0

nx::Trait create nx::TCallback {
  #
  # A small support trait to ease syntactically the reference to
  # instance variables and the registration of callbacks.
  #
  :method bindvar {name} {
    :require namespace
    return [nx::self]::$name
  }
  :method callback {name args} {
    return [list [nx::self] $name {*}$args]
  }
}