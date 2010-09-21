package require nx
package require nx::test

# just 8.6 or similar
if {[info command yield] eq ""} return

Test case generator {
  # ===================================
  # nx coro
  # ===================================
  nx::Object create numbers {
    :public method ++ {} {  
      puts stderr BEFORE-yield-[nx::self]
      #::nsf::yieldcheck
      yield
      puts stderr AFTER-yield-[nx::self]
      set i 0
      while 1 {
	yield $i
	incr i 2
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
