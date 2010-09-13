package require nx; # namespace import -force ::nx::*
package require nx::test

Test parameter count 10
Test case method-require { 

    #
    # A few method-provides
    #
    # Some provides could be in e.g. nx.tcl, some could be loaded via
    # package require. We could as well think about an auto-indexer
    # producing these....
    #

    nsf::provide_method append   {::nsf::alias  append -objscope ::append}
    nsf::provide_method lappend  {::nsf::alias  lappend -objscope ::lappend}
    nsf::provide_method set      {::nsf::alias  set -objscope ::set}
    nsf::provide_method tcl::set {::nsf::alias  set -objscope ::set}
    nsf::provide_method exists   {::nsf::alias  exists ::nsf::cmd::Object::exists}
    nsf::provide_method foo      {::nsf::method foo {x y} {return x=$x,y=$y}}
    nsf::provide_method x        {::nsf::mixin ::MIX} {
	# here could be as well a package require, etc.
	::nx::Class create ::MIX {:method x {} {return x}}
    }

    #
    # Lets try it out:
    #

    nx::Class create C {
	:require method set
	:require method exists
	
	# required names can be different from registered names; if there
	# are multiple set methods, we could point to the right one
	:require method tcl::set
	
	# object methods:
	:require class-object method lappend
	
	# a scripted method
	:require class-object method foo
	
	:require class-object method x
	
	# looks as well ok:
	:require namespace
    }
    
    C create c1
    ? {c1 set x 100} 100
    ? {c1 exists x} 1
    ? {C lappend some_list e1 e2} "e1 e2"
    ? {C foo 1 2} x=1,y=2
    ? {C x} x
}


