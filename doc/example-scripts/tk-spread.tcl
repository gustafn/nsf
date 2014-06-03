# A small Spreadsheet implementation, originally developed by Richard
# Suchenwirth in plain Tcl (see http://wiki.tcl.tk/1287). The
# spreadsheet was rewritten in an object oriented manner as a design
# study in NX by Gustaf Neumann in May 2011.
#
# image::tk-spread.png[]
#
package require Tk
package require nx::trait

 ##############################################################################
 # Class SpreadSheet
 #
 # The SpreadSheet computes simply totals for rows and columns.
 ##############################################################################
 nx::Class create SpreadSheet {
   #
   # The following attributes can be used for configuring the
   # spreadsheet.
   #
   :property {rows:integer 3} 
   :property {cols:integer 2} 
   :property {width:integer 8}
   
   #
   # If no widget is provided, use the name of the object as widget
   # name.
   #
   :property {widget ".[namespace tail [self]]"}

   #
   # Use the nx callback trait 
   #
   :require trait nx::traits::callback
   
   #
   # The method "cell" hides the internal respresentation and sets a
   # cell to a value.
   #
   :method cell {pair value} {
     set :data($pair) $value
   }
   
   #
   # The constructor builds the SpreadSheet matrix via multiple text
   # entry fields.
   #
   :method init {} {
     set :last ${:rows},${:cols}  ;# keep grand total field
     trace var [:bindvar data] w [:callback redo]
     frame ${:widget}
     for {set y 0} {$y <= ${:rows}} {incr y} {
       set row [list]
       for {set x 0} {$x <= ${:cols}} {incr x} {
	 set e [entry ${:widget}.$y,$x -width ${:width} \
		    -textvar [:bindvar data($y,$x)] -just right]
	 if {$x==${:cols} || $y==${:rows}} {
	   $e config -state disabled -background grey -relief flat
	 }
	 lappend row $e
       }
       grid {*}$row -sticky news
     }
     $e config -relief solid
   }
   
   # 
   # The method "redo" is triggered via the updates in the cells
   #
   :public method redo {varname el op} {
     if {$el ne ${:last}} {
       lassign [split $el ,] y x
       if {$x ne ""} {
	 :sum $y,* $y,${:cols}
	 :sum *,$x ${:rows},$x
       } ;# otherwise 'el' was not a cell index
     }   ;# prevent endless recalculation of grand total
   }
   
   #
   # The method "sum" adds the values matched by pattern (typically a
   # row or column) and sets finally the target column with the total
   #
   :method sum {pat target} {
     set sum 0
     set total "" ;# default if no addition succeeds
     foreach {i value} [array get :data $pat] {
       if {$i != $target} {
	 if {[string is double -strict $value]} {
	   set total [set sum [expr {$sum + $value}]]
	 }
       }
     }
     :cell $target $total
   }
 }

# Build spreadsheet "x"
SpreadSheet create x {
   # populate with some values
   :cell 0,0 Spread1 
   :cell 1,0 47
   :cell 2,1 11
 }

# Build spreadsheet "y"
SpreadSheet create y -rows 4 -cols 4 {
   :cell 0,0 Spread2
   :cell 1,0 12
   :cell 2,2 22
 }

# Pack the spreadsheets into one pane
pack [x cget -widget] [y cget -widget] -fill both

