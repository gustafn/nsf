package require nx

::nsf::configure dtrace on

nx::Object create o {
  :public method foo {x y} {
    [self] ::incr x 1
    return [expr {$x + $y}]
  }
}
o foo 1 2

::nsf::configure dtrace off
