# A small Ludo/Mensch ärgere Dich nicht/Pachisie game, originally
# developed by Richard Suchenwirth in plain Tcl (see
# http://wiki.tcl.tk/956). The game was rewritten as a design study in
# NX by Gustaf Neumann in July 2013. 
#
# Major changes:
#
# - object-oriented design (no global variables) 
#
# - knowledge about the paths of the figures
#
# - animated moves
#
# - knowledge about the basic rules (e.g. need 6 to move out of the
#   nest, have to move figures from starting position)
#
# - throw opponents out
#
# - sanity checks
#
# - user feedback
#
# image::tk-ludo.png[width=400]
#
# Short Instructions
#
# - The active player (marked with the button) has to dice (click on
#   the die, or press somewhere on the board "d").
#
# - If all figures are in the nest (start position), the player needs
#   to dice a 6. The player is allowed to try three times, then the
#   player is done (press "done" button, or type "n") and the turn
#   moves to the next player.
#
# - When a player got 6 eyes, he can move out of the nest. This is
#   done by clicking on the figure the player wants to move.
#
# - After dicing 6, the player can dice again and move the player on
#   the field (always by clicking on the figure).
#
# == Implementation
#
package require Tk
package require nx::trait

#
# Define an application specific converter "expr" that passes the
# scalar result of the expression. Since the converter is defined on
# nx::Slot, it is applicable to all method and configure arguments.
#
::nx::Slot method type=expr {name value} {return [expr $value]}

#
# Class Figure
#

nx::Class create Figure {
    :property canvas:required
    :property x:double
    :property y:double
    :property size:double
    :property position:integer
    :property color 
    :property no:integer
    :property board:object,required
    :variable tag ""

    :require trait nx::trait::callback

    :method init {} {
	#
	# Draw figure and define interactions
	#
	set d [expr {${:size}/6.}]
	set s [expr {${:size}/1.5}]
	set y [expr {${:y}-$d*2.5}]
	set :tag ${:color}${:no}
	set id [${:canvas} create arc [expr {${:x}-$s}] [expr {${:y}-$s}] \
		    [expr {${:x}+$s}] [expr {${:y}+$s}] -outline grey \
		    -start 250 -extent 40 -fill ${:color} \
		    -tags [list mv ${:tag}]]
	${:canvas} create oval \
	    [expr {${:x}-$d}] [expr {${:y}-$d}] \
	    [expr {${:x}+$d}] [expr {${:y}+$d}] \
	    -fill ${:color} -outline grey -tags [list mv ${:tag}]
	#${:board} figure set $id [self]
	${:canvas} bind ${:tag} <B1-ButtonRelease> [:callback go]
    }

    :public method go {} {
	#
	# Start moving the figure if the draw is permitted.
	# The board knows the die and the rules.
	#
	if {![${:board} moveFigure [self]]} {
	    # stay at old position
	    :gotoNr ${:position}
	}
    }

    :public method gotoNr {nr {-path ""} {-afterCmd ""}} {
	#
	# Move figure to the numbered position. If a path is given it
	# moves stepwise from position to position.
	#
	set oldPos ${:position}
	set :position $nr
	if {$path eq ""} {set path $nr}
	return [:move {*}[${:board} getPointCenter $oldPos] $path \
		    -afterCmd $afterCmd]
    }

    :protected method move {x0 y0 path:integer,1..n {-afterCmd ""}} {
	#
	# Move figure from old position (x0 y0) stepwise along the
	# path using animation. At the end of the move, 'afterCmd' is
	# issued.
	#
	set t 0
	foreach pos $path {
	    lassign [${:board} getPointCenter $pos] x y
	    set stepx [expr {($x-$x0)/50.0}]
	    set stepy [expr {($y-$y0)/50.0}]
	    for {set i 0} {$i < 50} {incr i} {
		after [incr t 8] ${:canvas} move ${:tag} $stepx $stepy
	    }
	    lassign [list $x $y] x0 y0
	    incr t 100
	}
	after $t ${:canvas} raise ${:tag}
	after $t $afterCmd
	set :x $x; set :y $y
    }
    
    :public object method lookup {position} {
	#
	# Return the figure at the provided position.  This function
	# could be made faster, but is efficient enough as it is.
	#
	foreach f [Figure info instances] {
	    if {[$f cget -position] == $position} {
		return $f
	    }
	}
	return ""
    }
}

#
# Helper functions for the die
#

proc random:select L {lindex $L [expr int(rand()*[llength $L].)]}
proc lexpr {term L} {
    # map an expr term to each element \$i of a list
    set res [list]
    foreach i $L {lappend res [eval expr $term]}
    set res
}

#
# Class Die
#
nx::Class create Die {
    :property canvas:required
    :property x:double
    :property y:double
    :property {size:double 25}
    :property {fg gold}
    :property {bg red}
    :property {eyes 0}

    :require trait nx::trait::callback

    :method set {n} {
	#
	# Set the eyes of the die.
	#
	${:canvas} itemconfig ${:grouptag} -fill ${:bg} -outline ${:bg}
	foreach i [lindex [list \
	       {} {d5} [random:select {{d3 d7} {d1 d9}}] \
	       [random:select {{d1 d5 d9} {d3 d5 d7}}] \
	       {d1 d3 d7 d9} {d1 d3 d5 d7 d9} \
	       [random:select {{d1 d3 d4 d6 d7 d9} {d1 d2 d3 d7 d8 d9}}] \
	      ] $n] {
            ${:canvas} itemconfig ${:id}$i -fill ${:fg} -outline ${:fg}
	}
	set :eyes $n
    }

    :public method invalidate {} {
	#
	# Invalidate the eyes to avoid double uses of the eyes.
	#
	set :eyes 0
    }

    :public method roll {} {
	#
	# Roll the dice and animate rolling
	#
	# wiggle: amount, pick one of eight wiggle directions
	set dwig [expr {${:size}/5}]
	for {set i 10} {$i<100} {incr i 10} {
	    :set [expr {int(rand() * 6) + 1}]
	    set wig [random:select {0,1 0,-1 1,0 -1,0 1,1 -1,1 1,-1 -1,-1}]
	    set wig [lexpr \$i*$dwig [split $wig ,]]
	    ${:canvas} move group${:id} {*}$wig
	    update
	    set wig [lexpr \$i*-1 $wig] ;# wiggle back
	    ${:canvas} move group${:id} {*}$wig
	    after $i
	}
    }

    :method init {} {
	#
	# initialize the widgets with tags interactions
	#
	set x [expr {${:x} - ${:size}/2.0}]
	set y [expr {${:y} - ${:size}/2.0}]
	set :id [${:canvas} create rect $x $y \
		     [expr {$x+${:size}}] [expr {$y+${:size}}] \
		     -fill ${:bg} -tags mvg]
	set :grouptag group${:id}
	${:canvas} addtag ${:grouptag} withtag ${:id}
	set ex [expr {$x+${:size}/10.}]
	set ey [expr {$y+${:size}/10.}]
	set d  [expr {${:size}/5.}];# dot diameter
	set dotno 1 ;# dot counter
	foreach y [list $ey [expr {$ey+$d*1.5}] [expr {$ey+$d*3}]] {
	    foreach x [list $ex [expr {$ex+$d*1.5}] [expr {$ex+$d*3}]] {
		${:canvas} create oval $x $y [expr {$x+$d}] [expr {$y+$d}] \
		    -fill ${:bg} -outline ${:bg} \
		    -tags [list mvg ${:grouptag} ${:id}d$dotno]
		incr dotno
	    }
	}
	:set [expr {int(rand()*6)+1}]
	:invalidate
	#
	# To dice, let people click on the die, or press <d> on the
	# board
	#
	${:canvas} bind mvg <1> [:callback roll]
	bind . <d> [:callback roll]
    }
}

#
# Class Board
#
nx::Class create Board {
    :property canvas:required
    :property {size:integer 25}
    :property {bg LightBlue1}
    :property {fg white}
    :property {colors:1..n {red green yellow blue}}

    :require trait nx::trait::callback

    :method lookup {var idx} {
	#
	# Convenience lookup function for arbitrary instance
	# variables.
	#
	set key "${var}($idx)"
	if {[info exists $key]} {return [set $key]}
	return ""
    }
    
    :public method getPointCenter {nr} {:lookup :pointCenter $nr}
    :public method getPointId {nr}     {:lookup :pointId $nr}

    :method line {
	x0:expr,convert y0:expr,convert x1:expr,convert y1:expr,convert 
	{-width 1} {-arrow none}
    } {
	#
	# Convenience function for line drawing, evaluates passed
	# expressions.
	#
	${:canvas} create line $x0 $y0 $x1 $y1 -width $width -arrow $arrow
    }
    
    :method point {x:expr,convert y:expr,convert d {-number:switch false} -fill} {
	#
	# Draw a point (a position on the game board) and keep its
	# basic data in instance variables. We could as well turn the
	# positions into objects.
	#
	if {![info exists fill]} {set fill ${:fg}}
	incr :pointCounter
	set id [${:canvas} create oval \
		    [expr {$x-$d/2.}] [expr {$y-$d/2.}] \
		    [expr {$x+$d/2.}] [expr {$y+$d/2.}] \
		    -fill $fill -tags [list point] -outline brown -width 2]
	#${:canvas} create text $x $y -text ${:pointCounter} -fill grey
	set :pointNr($id) ${:pointCounter}
	set :pointCenter(${:pointCounter}) [list $x $y]
	set :pointId(${:pointCounter}) $id
	return ${:pointCounter}
    }

    :method fpoint {x:expr,convert y:expr,convert psize fsize color no} {
	#
	# Draw a point with a figure, note the position in the board
	# in the figure
	#
	set nr [:point $x $y $psize -fill $color]
	Figure new -board [self] -canvas ${:canvas} \
	    -x $x -y [expr {$y-$fsize/2.0}] \
	    -size $fsize -color $color -no $no -position $nr
	return $nr
    }

    :method pnest {x:expr,convert y:expr,convert d colorNr xf yf} {
	#
	# Draw the nest with the figures in it
	#
	set fsize [expr {$d/0.75}]
	set color [lindex ${:colors} $colorNr]
	lappend :nest($colorNr) [:fpoint $x-$d $y-$d $d $fsize $color 0]
	lappend :nest($colorNr) [:fpoint $x-$d $y+$d $d $fsize $color 1]
	lappend :nest($colorNr) [:fpoint $x+$d $y-$d $d $fsize $color 2]
	lappend :nest($colorNr) [:fpoint $x+$d $y+$d $d $fsize $color 3]
	set :buttonPos($colorNr) [list [expr $x+($xf*$d)] [expr $y+($yf*$d)]]
    }

    :method pline {
	x0:expr,convert y0:expr,convert 
	x1:expr,convert y1:expr,convert d {-width 1} {-arrow none}
    } {
	#
	# Draw a path of the play-field with points (potential player
	# positions) on it.
	#
	set id [${:canvas} create line $x0 $y0 $x1 $y1 \
		    -width $width -arrow $arrow -fill brown]
	if {$x0 eq $x1} {
	    # vertical
	    set f [expr {$y1<$y0 ? -1.25 : 1.25}]
	    for {set i 0} {$i < int(abs($y1-$y0)/($d*1.25))} {incr i} {
		:point $x0 $y0+$i*$d*$f $d
	    }
	} else {
	    # horizontal
	    set f [expr {$x1<$x0 ? -1.25 : 1.25}]
	    for {set i 0} {$i < int(abs($x1-$x0)/($d*1.25))} {incr i} {
		:point $x0+$i*$d*$f $y0 $d -number
	    }
	}
	${:canvas} lower $id
    }

    :method draw {m} {
	#
	# Draw board and create figures
	#
	set d ${:size}
	set u [expr {$d * 1.25}]
	#
	# Major positions: p0 .. p1 ..m.. p2 .. p3
	#
	set p0 [expr {$u-$d/2.0}]
	set p1 [expr {$m-$u}]
	set p2 [expr {$m+$u}]
	set p3 [expr {2*$m-$u+$d/2}]

	:pline $p0 $p1 $p1 $p1 $d -width 4
	:pline $p1 $p1 $p1 $p0 $d -width 4
	:pline $p1 $p0 $p2 $p0 $d -width 4 ;# horizonal short line
	:pline $p2 $p0 $p2 $p1 $d -width 4
	:pline $p2 $p1 $p3 $p1 $d -width 4
	:pline $p3 $p1 $p3 $p2 $d -width 4 ;# vertical short line
	:pline $p3 $p2 $p2 $p2 $d -width 4
	:pline $p2 $p2 $p2 $p3 $d -width 4
	:pline $p2 $p3 $p1 $p3 $d -width 4 ;# horizonal short line
	:pline $p1 $p3 $p1 $p2 $d -width 4
	:pline $p1 $p2 $p0 $p2 $d -width 4
	:pline $p0 $p2 $p0 $p1 $d -width 4 ;# vertical short line
	:line $m+5*$d  $m+2*$d  $m+6*$d  $m+2*$d -arrow first
	:line $m-2*$d  $m+5*$d  $m-2*$d  $m+6*$d -arrow first
	:line $m-5*$d  $m-2*$d  $m-6*$d  $m-2*$d -arrow first
	:line $m+2*$d  $m-5*$d  $m+2*$d  $m-6*$d -arrow first

	set d2 [expr {$d*0.75}]
	set d15 $d2*2
	set o [expr {$u*5}]
	:pnest $m+$o-$d $m-$o+$d $d2 0 -1  3
	:pnest $m+$o-$d $m+$o-$d $d2 1 -1 -2.5 
	:pnest $d15     $m+$o-$d $d2 2  1 -2.5 
	:pnest $d15     $m-$o+$d $d2 3  1  3
	for {set i 0; set y [expr $d*2]} {$i<4} {incr i;set y [expr {$y+$d}]} {
	    lappend p(0) [:point $m      $y      $d2 -fill [lindex ${:colors} 0]]
	    lappend p(1) [:point $m*2-$y $m      $d2 -fill [lindex ${:colors} 1]]
	    lappend p(2) [:point $m      $m*2-$y $d2 -fill [lindex ${:colors} 2]]
	    lappend p(3) [:point $y      $m      $d2 -fill [lindex ${:colors} 3]]
	}
	#
	# Setup the path per player and color the starting points
	#
	for {set i 1} {$i < 41} {incr i} {lappend path $i}	
	foreach c {0 1 2 3} pos {11 21 31 1} o {11 21 31 1} {
	    ${:canvas} itemconfig [:getPointId $pos] -fill [lindex ${:colors} $c]
	    set :path($c) [concat [lrange $path $o-1 end] [lrange $path 0 $o-2] $p($c)] 
	}
    }

    :public method msg {text} {
	#
	# Report a message to the user.
	#
	${:canvas} itemconfig ${:msgId} -text $text
	return 0
    }

    :public method wannaGo {obj pos {-path ""}} {
	#
	# We know that we can move the figure in principle.  We have
	# to check, whether the target position is free. If the target
	# is occupied by our own player, we give up, otherwise we
	# through the opponent out.
	#
	if {$pos eq ""} {return [:msg "beyond path"]}
	set other [Figure lookup $pos]
	set afterCmd ""
	if {$other ne ""} {
	    if {[$obj cget -color] eq [$other cget -color]} {
		# On player can't have two figure at the same place.
		return [:msg "My player is already at pos $pos"]
	    } else {
		# Opponent is at the target position. Find a free
		# position in the opponents nest and though her out.
		set opponent [$other cget -color]
		foreach p [set :nest([lsearch ${:colors} $opponent])] {
		    if {[Figure lookup $p] eq ""} {
			set afterCmd [list $other gotoNr $p]
			break
		    }
		}
	    }
	}
	:msg "[$obj cget -color]-[$obj cget -no] went to $pos"
	$obj gotoNr $pos -path $path -afterCmd $afterCmd
	${:die} invalidate
    }

    :public method moveFigure {obj} {
	#
	# Move the provided figure by the diced eyes according to the
	# rules. First we check, if we are allowed to move this
	# figure, which might be in the nest or on the run.
	#
	set currentColor [lindex ${:colors} ${:player}]
	if {[$obj cget -color] ne $currentColor} {
	    return [:msg "figure is not from the current player"]
	}
	set eyes [${:die} cget -eyes]
	if {$eyes == 0} {
	    return [:msg "Must dice first"]
	}
	set position [$obj cget -position]
	if {$position in [set :nest(${:player})]} {
	    # Figure is in the nest, just accept eyes == 6
	    if {$eyes == 6} {
		:wannaGo $obj [lindex [set :path(${:player})] 0]
	    } else {
		return [:msg "Need 6 to move this figure"]
	    }
	} else {
	    #
	    # Check, if we have still figures in the nest
	    #
	    set inNest ""
	    foreach p [set :nest(${:player})] {
		set inNest [Figure lookup $p]
		if {$inNest ne ""} break
	    }
	    #
	    # Check, if the actual figure is at the start position.
	    #
	    set startPos [lindex [set :path(${:player})] 0]
	    set atStart [Figure lookup $startPos]
	    if {$eyes == 6} {
		if {$inNest ne ""} {
		    # Move a figure out from the nest, if we can
		    if {$atStart ne ""} {
			if {[$atStart cget -color] eq $currentColor} {
			    set path [set :path(${:player})]
			    set current [lsearch $path $position]
			    set targetPos [expr {$current + [${:die} cget -eyes]}]
			    :wannaGo $obj [lindex $path $targetPos] \
				-path [lrange $path $current+1 $targetPos]
			    return 1
			}
		    }
		    return [:msg "You have to move the figures from your nest first"]
		}
	    }
	    if {$atStart ne "" && $inNest ne "" && $obj ne $atStart} {
		return [:msg "You have to move the figures from the start first"]
	    }
	    set path [set :path(${:player})]
	    set current [lsearch $path $position]
	    set targetPos [expr {$current + [${:die} cget -eyes]}]
	    :wannaGo $obj [lindex $path $targetPos] \
		-path [lrange $path $current+1 $targetPos]
	}
	return 1
    }

    :public method nextPlayer {} {
	#
	# Switch to the next player. 
	#
	set :player [expr {(${:player}+1) % 4}]
	${:canvas} coords ${:buttonWindow} {*}[set :buttonPos(${:player})]
    }

    :method init {} {
	set hw [expr {14 * ${:size}}]
	set center [expr {$hw / 2}]
	canvas ${:canvas} -bg ${:bg} -height $hw -width $hw
	:draw $center
	set :die [Die new -canvas ${:canvas} -x $center -y $center -size ${:size}]
	set :msgId [${:canvas} create text [expr {${:size}*4}] 10 -text ""]
	#
	# Player management (signal which player is next, etc.)
	#
	set :player 2
	button .b1 -text "Done" -command [:callback nextPlayer]
	set :buttonWindow [.p create window 22 14 -window .b1]
	:nextPlayer
	bind . <n> [:callback nextPlayer]
    }
}

#
# Finally, create the board and pack it
#

Board new -canvas .p -bg beige -size 40
pack .p



