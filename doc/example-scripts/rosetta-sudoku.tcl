#
# == Rosetta Example: Sudoku
#
# Solve a partially filled-in 9x9 Sudoku grid and display the result
# in a human-readable format.  For detailed description of this
# example, see https://rosettacode.org/wiki/Sudoku_Solver
#
# This implementation is based on https://wiki.tcl-lang.org/19934 

package require nx

#
# The class +Sudoku+ implements the basic interface to a sudoku 9x9
# board to load/dump data and to set/access cells, rows, columns and
# regions.
nx::Class create Sudoku {
    
    :variable board

    # Setup an array from 0..9 to ease iterations over the cells of
    # lines and columns.
    for {set i 0} {$i < 9} {incr i} {lappend positions $i}
    :variable positions $positions
 
    :public method load {data} {
	#
	# Load a 9x9 partially solved sudoku. The unsolved cells are
	# represented by a@ symbols.
	#
	set error "data must be a 9-element list, each element also being a\
		list of 9 numbers from 1 to 9 or blank or an @ symbol."
	if {[llength $data] != 9} {
	    error $error
	}
	foreach y ${:positions} {
	    set row [lindex $data $y]
	    if {[llength $row] != 9} {
		error $error
	    }
	    foreach x ${:positions} {
		set cell [lindex $row $x]
		if {![regexp {^[@1-9]?$} $cell]} {
		    error $cell-$error
		}
		if {$cell eq "@"} {set cell ""}
		:set $x $y $cell
	    }
	}
    }

    :public method dump {-pretty-print:switch} {
	#
	# Output the current state of the sudoku either as list or in
	# a pretty-print style.
	#
	set rows [lmap y ${:positions} {:getRow 0 $y}]
	if {${pretty-print}} {
	    set result +-----+-----+-----+\n
	    foreach line $rows postline {0 0 1 0 0 1 0 0 1} {
		append result |[lrange $line 0 2]|[lrange $line 3 5]|[lrange $line 6 8]|\n
		if {$postline} {
		    append result +-----+-----+-----+\n
		}
	    }
	    return $result
	} else {
	    return $rows
	}
    }
	
    :method log {msg} {
	#puts "log: $msg"
    }
 
    :method set {x y value:integer,0..1} {
	#
	# Set cell at position x,y to the given value or empty.
	#
	if {$value<1 || $value>9} {
	    set :board($x,$y) {}
	} else {
	    set :board($x,$y) $value
	}
    }
    :method get {x y} {
	#
	# Get value of cell at position x, y.
	#
	return [set :board($x,$y)]
    }
 
    :method getRow {x y} {
	#
	# Return a row at constant position y.
	#
	return [lmap x ${:positions} {:get $x $y}]
    }
    :method getCol {x y} {
	#
	# Return a column at constant position x.
	#
	return [lmap y ${:positions} {:get $x $y}]
    }

    :method getRegion {x y} {
	#
	# Return a 3x3 region
	#
	set xR [expr {($x/3)*3}]
	set yR [expr {($y/3)*3}]
	set regn {}
	for {set x $xR} {$x < $xR+3} {incr x} {
	    for {set y $yR} {$y < $yR+3} {incr y} {
		lappend regn [:get $x $y]
	    }
	}
	return $regn
    }
}
 
# The class +SudokuSolver+ inherits from +Sudoku+, and adds the
# ability to solve a given Sudoku game. The method 'solve' applies all
# rules for each unsolved cell until it finds a safe solution.
 
nx::Class create SudokuSolver -superclass Sudoku {

    :public method validchoices {x y} {
	set v [:get $x $y]
	if {$v ne {}} {
	    return $v
	}
	
	set row [:getRow $x $y]
	set col [:getCol $x $y]
	set regn [:getRegion $x $y]
	set eliminate [list {*}$row {*}$col {*}$regn]
	set eliminate [lsearch -all -inline -not $eliminate {}]
	set eliminate [lsort -unique $eliminate]
 
	set choices {}
	for {set c 1} {$c < 10} {incr c} {
	    if {$c ni $eliminate} {
		lappend choices $c
	    }
	}
	if {[llength $choices]==0} {
	    error "No choices left for square $x,$y"
	}
	return $choices
    }
    
    :method completion {} {
	#
	# Return the number of already solved items.
	#
	return [expr {81-[llength [lsearch -all -inline [join [:dump]] {}]]}]
    }
    
    :public method solve {} {
	#
	# Try to solve the sudoku by applying the provided rules.
	#
	while {1} {
	    set begin [:completion]
	    foreach y ${:positions} {
		foreach x ${:positions} {
		    if {[:get $x $y] eq ""} {
			foreach rule [Rule info instances] {
			    set c [$rule solve [self] $x $y]
			    if {$c} {
				:set $x $y $c
				:log "[$rule info class] solved [self] at $x,$y for $c"
				break
			    }
			}
		    }
		}
	    }
	    set end [:completion]
	    if {$end == 81} {
		:log "Finished solving!"
		break
	    } elseif {$begin == $end} {
		:log "A round finished without solving any squares, giving up."
		break
	    }
	}
    }
}
 
# The class rule provides "solve" as public interface for all rule
# objects. The rule objects apply their logic to the values
# passed in and return either '0' or a number to allocate to the
# requested square.
nx::Class create Rule {
    
    :public method solve {hSudoku:object,type=::SudokuSolver x y} {
	:Solve $hSudoku $x $y [$hSudoku validchoices $x $y]
    }
 
    # Get all the allocated numbers for each square in the row, column, and
    # region containing $x,$y. If there is only one unallocated number among all
    # three groups, it must be allocated at $x,$y
    :create ruleOnlyChoice {
	:object method Solve {hSudoku x y choices} {
	    if {[llength $choices] == 1} {
		return $choices 
	    } else {
		return 0
	    }
	}
    }

    # Test each column to determine if $choice is an invalid choice for all other
    # columns in row $X. If it is, it must only go in square $x,$y.
    :create RuleColumnChoice {
	:object method Solve {hSudoku x y choices} {
	    foreach choice $choices {
		set failed 0
		for {set x2 0} {$x2 < 9} {incr x2} {
		    if {$x2 != $x && $choice in [$hSudoku validchoices $x2 $y]} {
			set failed 1
			break
		    }
		}
		if {!$failed} {return $choice}
	    }
	    return 0
	}
    }
 
    # Test each row to determine if $choice is an invalid choice for all other
    # rows in column $y. If it is, it must only go in square $x,$y.
    :create RuleRowChoice {
	:object method Solve {hSudoku x y choices} {
	    foreach choice $choices {
		set failed 0
		for {set y2 0} {$y2 < 9} {incr y2} {
		    if {$y2 != $y && $choice in [$hSudoku validchoices $x $y2]} {
			set failed 1
			break
		    }
		}
		if {!$failed} {return $choice}
	    }
	    return 0
	}
    }
 
    # Test each square in the region occupied by $x,$y to determine if $choice is
    # an invalid choice for all other squares in that region. If it is, it must
    # only go in square $x,$y.
    :create RuleRegionChoice {
	:object method Solve {hSudoku x y choices} {
	    foreach choice $choices {
		set failed 0
		set regnX [expr {($x/3)*3}]
		set regnY [expr {($y/3)*3}]
		for {set y2 $regnY} {$y2 < $regnY+3} {incr y2} {
		    for {set x2 $regnX} {$x2 < $regnX+3} {incr x2} {
			if {
			    ($x2!=$x || $y2!=$y)
			    && $choice in [$hSudoku validchoices $x2 $y2]
			} then {
			    set failed 1
			    break
			}
		    }
		}
		if {!$failed} {return $choice}
	    }
	    return 0
	}
    }
}

SudokuSolver create sudoku {

    :load {
	{3 9 4    @ @ 2    6 7 @}
	{@ @ @    3 @ @    4 @ @}
	{5 @ @    6 9 @    @ 2 @}
	
	{@ 4 5    @ @ @    9 @ @}
	{6 @ @    @ @ @    @ @ 7}
	{@ @ 7    @ @ @    5 8 @}
	
	{@ 1 @    @ 6 7    @ @ 8}
	{@ @ 9    @ @ 8    @ @ @}
	{@ 2 6    4 @ @    7 3 5}
    }
    :solve
    
    puts [:dump -pretty-print]
}

# The dump method outputs the solved Sudoku:
#
#  +-----+-----+-----+
#  |3 9 4|8 5 2|6 7 1|
#  |2 6 8|3 7 1|4 5 9|
#  |5 7 1|6 9 4|8 2 3|
#  +-----+-----+-----+
#  |1 4 5|7 8 3|9 6 2|
#  |6 8 2|9 4 5|3 1 7|
#  |9 3 7|1 2 6|5 8 4|
#  +-----+-----+-----+
#  |4 1 3|5 6 7|2 9 8|
#  |7 5 9|2 3 8|1 4 6|
#  |8 2 6|4 1 9|7 3 5|
#  +-----+-----+-----+

