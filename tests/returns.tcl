package require nx; # namespace import -force ::nx::*
package require nx::test

#
# The same tests are in this test suite, once with and once without
# checking 
#
# Make sure, checking is turned on
#
::nsf::configure checkresult true

Test parameter count 10000
Test case int-returns { 
    nx::Class create C {
	# scripted method without paramdefs
	:method bar-ok1 {a b} {return 1}
	:method bar-ok2 {a b} {return $a}
	# scripted method with paramdefs
	:method bar-nok {a b:integer} {return a}
	# alias to tcl-cmd (no param defs)
	:alias -objscope incr ::incr
	:alias -objscope lappend ::lappend
	:create c1
    }

    ::nsf::methodproperty C bar-ok1 returns integer
    ::nsf::methodproperty C bar-ok2 returns integer
    ::nsf::methodproperty C bar-nok returns integer
    ::nsf::methodproperty C incr returns integer
    ::nsf::methodproperty C lappend returns integer

    ? {c1 bar-ok1 1 2} 1
    ? {c1 bar-ok2 1 2} 1
    ? {c1 bar-nok 1 2} {expected integer but got "a" for parameter return-value}

    ? {c1 incr x} 1
    ? {c1 incr x} 10002

    ? {c1 lappend l e1} {expected integer but got "e1" for parameter return-value}

    # query the returns value
    ? {::nsf::methodproperty C lappend returns} integer

    # reset it to emtpy
    ? {::nsf::methodproperty C lappend returns ""} ""
    ? {::nsf::methodproperty C bar-ok1 returns ""} ""
    ? {::nsf::methodproperty C bar-ok2 returns ""} ""
    ? {::nsf::methodproperty C bar-nok returns ""} ""

    # no checking 
    ? {c1 bar-ok1 1 2} 1
    ? {c1 bar-ok2 1 2} 1
    ? {c1 bar-nok 1 2} a
    ? {c1 lappend l e2} "e1 e2"

    # query returns "", if there is no returns checking
    ? {::nsf::methodproperty C lappend returns} ""
    ? {::nsf::methodproperty ::nx::Object method returns} ""

}

Test parameter count 10
Test case app-specific-returns { 

    ::nx::methodParameterSlot method type=range {name value arg} {
	foreach {min max} [split $arg -] break
	if {$value < $min || $value > $max} {
	    error "Value '$value' of parameter $name not between $min and $max"
	}
	return $value
    }

    nx::Class create C {
	:method bar-ok1 {a b} {return 1}
	:method bar-ok2 {a b} {return $a}
	:method bar-nok {a b:integer} {return a}
	:alias -objscope incr ::incr
	:alias -objscope lappend ::lappend
	:create c1
    }

    ::nsf::methodproperty C bar-ok1 returns range,arg=1-3
    ::nsf::methodproperty C bar-ok2 returns range,arg=1-3
    ::nsf::methodproperty C bar-nok returns range,arg=1-3
    ::nsf::methodproperty C incr returns range,arg=1-30
    ::nsf::methodproperty C lappend returns range,arg=1-30

    ? {c1 bar-ok1 1 2} 1
    ? {c1 bar-ok2 1 2} 1
    ? {c1 bar-nok 1 2} {Value 'a' of parameter return-value not between 1 and 3}

    ? {c1 incr x} 1
    ? {c1 incr x} 12

    ? {c1 lappend l e1} {Value 'e1' of parameter return-value not between 1 and 30}
}

Test parameter count 1000
Test case converting-returns { 

    ::nx::methodParameterSlot method type=sex {name value args} {
        #puts stderr "[current] slot specific converter"
	switch -glob $value {
	    m* {return m}
	    f* {return f}
	    default {error "expected sex but got $value"}
	}
    }

    nx::Class create C {
	:method bar-ok1 {a b} {return male}
	:method bar-ok2 {a b} {return $a}
	:method bar-nok {a b:integer} {return $b}
	:alias -objscope set ::set
	:create c1
    }

    ::nsf::methodproperty C bar-ok1 returns sex
    ::nsf::methodproperty C bar-ok2 returns sex
    ::nsf::methodproperty C bar-nok returns sex
    ::nsf::methodproperty C set returns sex

    ? {c1 bar-ok1 1 2} male
    ? {c1 bar-ok2 female 2} female
    ? {c1 bar-nok 1 6} {expected sex but got 6}

    ? {c1 set x male} male
    ? {c1 eval {set :x}} male
    ? {c1 set x} male

    ? {c1 set x hugo} {expected sex but got hugo}

    ::nsf::methodproperty C bar-ok1 returns sex,convert
    ::nsf::methodproperty C bar-ok2 returns sex,convert
    ::nsf::methodproperty C bar-nok returns sex,convert
    ::nsf::methodproperty C set returns sex,convert

    ? {c1 bar-ok1 1 2} m
    ? {c1 bar-ok2 female 2} f
    ? {c1 bar-nok 1 6} {expected sex but got 6}

    ? {c1 set x male} m
    ? {c1 eval {set :x}} male
    ? {c1 set x} m

    ? {c1 set x hugo} {expected sex but got hugo}
}

#
# turn off result checking
#
::nsf::configure checkresults false

Test parameter count 1000
Test case int-returns-nocheck { 
    nx::Class create C {
	# scripted method without paramdefs
	:method bar-ok1 {a b} {return 1}
	:method bar-ok2 {a b} {return $a}
	# scripted method with paramdefs
	:method bar-nok {a b:integer} {return a}
	# alias to tcl-cmd (no param defs)
	:alias -objscope incr ::incr
	:alias -objscope lappend ::lappend
	:create c1
    }

    ::nsf::methodproperty C bar-ok1 returns integer
    ::nsf::methodproperty C bar-ok2 returns integer
    ::nsf::methodproperty C bar-nok returns integer
    ::nsf::methodproperty C incr returns integer
    ::nsf::methodproperty C lappend returns integer

    ? {c1 bar-ok1 1 2} 1
    ? {c1 bar-ok2 1 2} 1
    ? {c1 bar-nok 1 2} a

    ? {c1 incr x} 1
    ? {c1 incr x} 1002

    ? {c1 lappend l e1} e1

    # query the returns value
    ? {::nsf::methodproperty C lappend returns} integer

    # reset it to emtpy
    ? {::nsf::methodproperty C lappend returns ""} ""

    c1 eval {set :l e1}
    # no checking on lappend
    ? {c1 lappend l e2} "e1 e2"

    # query returns "", if there is no returns checking
    ? {::nsf::methodproperty C lappend returns} ""
    ? {::nsf::methodproperty ::nx::Object method returns} ""

}

Test parameter count 10
Test case app-specific-returns-nocheck { 

    ::nx::methodParameterSlot method type=range {name value arg} {
	foreach {min max} [split $arg -] break
	if {$value < $min || $value > $max} {
	    error "Value '$value' of parameter $name not between $min and $max"
	}
	return $value
    }

    nx::Class create C {
	:method bar-ok1 {a b} {return 1}
	:method bar-ok2 {a b} {return $a}
	:method bar-nok {a b:integer} {return a}
	:alias -objscope incr ::incr
	:alias -objscope lappend ::lappend
	:create c1
    }

    ::nsf::methodproperty C bar-ok1 returns range,arg=1-3
    ::nsf::methodproperty C bar-ok2 returns range,arg=1-3
    ::nsf::methodproperty C bar-nok returns range,arg=1-3
    ::nsf::methodproperty C incr returns range,arg=1-30
    ::nsf::methodproperty C lappend returns range,arg=1-30

    ? {c1 bar-ok1 1 2} 1
    ? {c1 bar-ok2 1 2} 1
    ? {c1 bar-nok 1 2} a

    ? {c1 incr x} 1
    ? {c1 incr x} 12

    ? {c1 lappend l e1} e1
}

Test parameter count 1000
Test case converting-returns-nocheck { 

    ::nx::methodParameterSlot method type=sex {name value args} {
        #puts stderr "[current] slot specific converter"
	switch -glob $value {
	    m* {return m}
	    f* {return f}
	    default {error "expected sex but got $value"}
	}
    }

    nx::Class create C {
	:method bar-ok1 {a b} {return male}
	:method bar-ok2 {a b} {return $a}
	:method bar-nok {a b:integer} {return $b}
	:alias -objscope set ::set
	:create c1
    }

    #
    # turn off checker
    #
    ::nsf::methodproperty C bar-ok1 returns sex
    ::nsf::methodproperty C bar-ok2 returns sex
    ::nsf::methodproperty C bar-nok returns sex
    ::nsf::methodproperty C set returns sex

    ? {c1 bar-ok1 1 2} male
    ? {c1 bar-ok2 female 2} female
    ? {c1 bar-nok 1 6} 6

    ? {c1 set x male} male
    ? {c1 eval {set :x}} male
    ? {c1 set x} male

    ? {c1 set x hugo} hugo

    #
    # don't turn off converter
    #
    ::nsf::methodproperty C bar-ok1 returns sex,convert
    ::nsf::methodproperty C bar-ok2 returns sex,convert
    ::nsf::methodproperty C bar-nok returns sex,convert
    ::nsf::methodproperty C set returns sex,convert

    ? {c1 bar-ok1 1 2} m
    ? {c1 bar-ok2 female 2} f
    ? {c1 bar-nok 1 6} {expected sex but got 6}

    ? {c1 set x male} m
    ? {c1 eval {set :x}} male
    ? {c1 set x} m

    ? {c1 set x hugo} {expected sex but got hugo}
}
