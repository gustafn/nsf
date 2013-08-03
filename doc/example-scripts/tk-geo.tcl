# Drawing geometric figures - the result of airplane travel. 
#
# The example script shows the use of canvas and geometric figues
# (regular, convex polygons) with different number of edges based on
# trigonometric functions.
#
# -gustaf neumann    (Aug 2, 2013)
#
# image::tk-geo1.png[width=400]
# image::tk-geo2.png[width=400]
#

package require Tk
package require nx

#
# Class Canvas is a simple convenience wrapper for the tk canvas,
# which packs itself.
#
nx::Class create Canvas {
  :property {canvas .canvas}
  :property {bg beige}
  :property {height 500}
  :property {width 500}

  :method init {} {
    canvas ${:canvas} -bg ${:bg} -height ${:height} -width ${:width}
    pack ${:canvas}
  }
}

#
# Class Area provides a center point (x, y) and a radius
#
nx::Class create Area {
  :property {canvas .canvas}
  :property {x 250}
  :property {y 250}
  :property {radius 200}

  :variable pi [expr {acos(-1)}]

  :method degree {d} {
    #
    # return a coordinate pair on a circle around the center point with
    # :radius at the provided degrees (0..360)
    #
    set x  [expr {$d*${:pi}/180.0 - ${:pi}/2.0}]
    set x0 [expr {cos($x)*${:radius}+${:x}}]
    set y0 [expr {sin($x)*${:radius}+${:y}}]
    list $x0 $y0
  }

  :method n-tangle {n} {
    #
    # Draw a regular n-tangle (e.g. when n==3, a triangle) inscribed to
    # a circle with radius :radius
    #
    for {set i 0} {$i < $n} {incr i} {
      set p($i) [:degree [expr {$i*360/$n}]]
    }
    lassign $p(0) x0 y0
    for {set i 1} {$i < $n} {incr i} {
      lassign $p($i) x1 y1
      ${:canvas} create line $x0 $y0 $x1 $y1
      lassign $p($i) x0 y0
    }
    lassign $p(0) x1 y1
    ${:canvas} create line $x0 $y0 $x1 $y1
  }
}

#
# Class Inscribe draws multiple n-tangles with the came center point.
#
nx::Class create Inscribe -superclass Area {
  :property {count 4}
  :property {edges 3}
  :method init {} {
    for {set i 0} {$i < ${:count}} {incr i} {
      ${:canvas} create oval \
	  [expr {${:x}-${:radius}}] [expr {${:y}-${:radius}}] \
	  [expr {${:x}+${:radius}}] [expr {${:y}+${:radius}}]
      :n-tangle ${:edges}
      set :radius [expr {${:radius}/2.0}]
    }
  }
}

#
# Class Hull creates an n-tangle with :density hull lines between
# neighboring edges
#
nx::Class create Hull -superclass Area {
  :property {edges 3}
  :property {density 10}

  :method n-tangle {n} {
    for {set i 0} {$i < $n} {incr i} {
      set p($i) [:degree [expr {$i*360/$n}]]
    }
    lassign $p(0) x0 y0
    for {set i 1} {$i < $n} {incr i} {
      lassign $p($i) x1 y1
      set line($i) [list $x0 $y0 $x1 $y1]
      ${:canvas} create line $x0 $y0 $x1 $y1
      lassign $p($i) x0 y0
    }
    lassign $p(0) x1 y1
    ${:canvas} create line $x0 $y0 $x1 $y1
    set line(0) [list $x0 $y0 $x1 $y1]
    set line($n) [list $x0 $y0 $x1 $y1]

    for {set i 0} {$i < $n} {incr i} {
      lassign $line($i) x0 y0 x1 y1
      lassign $line([expr {$i+1}]) x2 y2 x3 y3
      set dx1 [expr {($x0 - $x1)*1.0/${:density}}]
      set dy1 [expr {($y0 - $y1)*1.0/${:density}}]
      set dx2 [expr {($x2 - $x3)*1.0/${:density}}]
      set dy2 [expr {($y2 - $y3)*1.0/${:density}}]
      for {set j 1} {$j < ${:density}} {incr j} {
	${:canvas} create line [expr {$x0-$dx1*$j}] [expr {$y0-$dy1*$j}] \
	    [expr {$x2-$dx2*$j}] [expr {$y2-$dy2*$j}]
      }
    }
  }
    
  :method init {} {
    :n-tangle ${:edges}
  }
}


# Draw either one larger figure with inner figures
# or a series of smaller figures next to each other.

set multiple 0

if {$multiple} {
  # Draw a series of figures next to each other
  set c [::Canvas new -width 650 -height 750 -bg white]
  ::Inscribe new -canvas [$c cget -canvas] -x 100 -y 100 -radius 80 -count 7
  ::Inscribe new -canvas [$c cget -canvas] -x 300 -y 100 -radius 80 -count 7 -edges 4
  ::Inscribe new -canvas [$c cget -canvas] -x 500 -y 100 -radius 80 -count 7 -edges 5
  ::Hull new -canvas [$c cget -canvas] -x 100 -y 300 -radius 80 -edges 3 -density 10
  ::Hull new -canvas [$c cget -canvas] -x 300 -y 300 -radius 80 -edges 4 -density 10
  ::Hull new -canvas [$c cget -canvas] -x 500 -y 300 -radius 80 -edges 5 -density 10
  ::Hull new -canvas [$c cget -canvas] -x 300 -y 600 -radius 200 -edges 3 -density 40
} else {
  # Draw a several series of figures with the same center
  set c [::Canvas new -width 650 -height 650 -bg white]
  ::Hull new -canvas [$c cget -canvas] -x 300 -y 320 -radius 300 -edges 5 -density 40
  ::Hull new -canvas [$c cget -canvas] -x 300 -y 320 -radius 150 -edges 4 -density 20
  ::Hull new -canvas [$c cget -canvas] -x 300 -y 320 -radius 75 -edges 3 -density 10
  ::Hull new -canvas [$c cget -canvas] -x 300 -y 320 -radius 30 -edges 5 -density 5
}


