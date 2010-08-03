package requite nx

nx::Class create stack {
  :method init {} {set :things {}}
  :public method push {thing} {
    set :things [linsert ${:things} 0 $thing]
    return $thing
  }
  :public method pop {} {
    set top [lindex ${:things} 0]
    set :things [lrange ${:things} 1 end]
    return $top
  }
}

stack create s1
puts [s1 push 1]
puts [s1 push 2]
puts [s1 pop]
puts [s1 pop]
puts [s1 pop]