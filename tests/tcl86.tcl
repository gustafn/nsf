package require nx
package require nx::test

# just 8.6 or similar
if {[info command yield] eq ""} return

Test case coro-generator {
  # ===================================
  # nx coro
  # ===================================
  nx::Object create numbers {
    set :delta 2

    :public method ++ {} {  
      yield
      set i 0
      while 1 {
	yield $i
	incr i ${:delta}
      }
    }
  }
  
  coroutine nextNumber numbers ++
  set ::j 0
  for {set i 0} {$i < 10} {incr i} {
    incr ::j [nextNumber]
  }
  rename nextNumber {}

  ? {set ::j} 90
}

#
# apply
#
Test case apply {

  # Register apply as an alias
  ::nx::Object public alias apply ::apply

  ::nx::Object create o {
    # Set an object variable
    set :delta 100

    # Define a standard map function based on apply
    :public method map {lambda list} {
      set result {}
      foreach item $list {
	lappend result [:apply $lambda $item]
      }
      return $result
    }

  }

  # Two examples from the apply man page
  ? {o map {x {return [string length $x]:$x}} {a bb ccc dddd}} \
      "1:a 2:bb 3:ccc 4:dddd"
  ? {o map {x {expr {$x**2 + 3*$x - 2}}} {-4 -3 -2 -1 0 1 2 3 4}} \
      "2 -2 -4 -4 -2 2 8 16 26"

  ## Test case accessing object specific variable
  #? {o map {x {::nsf::__db_show_stack; return [expr {$x * ${:delta}}]}} {-4 -3 -2 -1 0 1 2 3 4}} \
  #    "-400 -300 -200 -100 0 100 200 300 400"

  # Test case accessing object specific variable
  ? {o map {x {expr {$x * ${:delta}}}} {-4 -3 -2 -1 0 1 2 3 4}} \
      "-400 -300 -200 -100 0 100 200 300 400"
}

