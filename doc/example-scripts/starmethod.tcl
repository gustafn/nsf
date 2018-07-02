#
# = Star Methods
#
# Design study for implementing methods which applies to instances of
# instances meta-classes. This study implements in addition to the
# regular "method" a new construct called "*method" which has the
# mentioned transitive property. The same behavior can be achieved in
# many ways. In this study, we define a special class (the method
# container class for *methods) which is kept in the precedence path
# of instances. This way, it can be defined freely with other
# extension mechanisms such as mixins, traits or filters.
#
package req nx::test

nx::Class eval {
    #
    # Define a *method, which is a method that applies for instances of
    # the instances of a meta-class.
    # - *methods are only defineable on meta-classes
    # - *methods are applicable on the instances of the instances of the
    #   meta-class
    # - If one defines a *method "bar" on a meta-class "MClass", and a
    #   class "C" as an instance of "MClass", and "c1" is an instance of
    #   "C", then "bar" is applicable for "c1".
    
    #
    # The "*method" has the same signature as regular methods, and can
    # be used in combination with the modifiers
    # public/protected/private as usual.
    #
    :public method *method {name arguments:parameter,0..* -returns body -precondition -postcondition} {
	#
	# Allow the definition only on meta-classes
	#
	if {![nsf::is metaclass [self]]} {
	    error "[self] is not a meta-class"
	}
	#
	# Do we have the class for keeping the *methods already?
	#
	set starClass [nx::Class create [self]::*]
	
	if {![nsf::object::exists $starClass]} {
	    #
	    # If not, create the *method container class and provide
	    # it as a default in the superclass hierarchy. This
	    # happens by modifying the property "-superclasses" which
	    # is used on every class to specify the class hierarchy.
	    #
	    :property [list superclasses $starClass] {
		#
		# Define a slot-specific method for keeping the
		# *method container class in the hierarchy.
		#
		:public object method appendToRelations { class property value } {
		    set sc [nsf::relation::get $class $property]
		    if {$sc eq "::nx::Object"} {
			nsf::relation::set $class $property $value
		    } else {
			nsf::relation::set $class $property [concat $sc $value]
		    }
		}
		
		#
		# Whenever the "-superclasses" relation is called,
		# make sure, we keep the *method container class in
		# the hierarchy.
		#
		:public object method value=set { class property value } {
		    :appendToRelations $class superclass $value
		}
	    }
	    
	    #
	    # Update class hierarchies of the previously created instances
	    # of the meta-class.
	    #
	    foreach class [:info instances] {
		set slot [$class info lookup slots superclasses]
		$slot appendToRelations $class superclass $starClass
	    }
	}

	#
	# Define the *method as regular method in the star method
	# container class.
	#
	[self]::* method $name $arguments \
	    {*}[expr {[info exists returns] ? [list -returns $returns] : ""}] \
	    $body \
	    {*}[expr {[info exists precondition]  ? [list -precondition $precondition] : ""}] \
	    {*}[expr {[info exists postcondition] ? [list -postcondition $postcondition] : ""}]
    }
}
set ::nsf::methodDefiningMethod(*method) 1


#
# == Some base test cases:
#
# Define a meta-class MClass with a method "foo" and to star methods
# named "foo" and "bar".
#
nx::Class create MClass -superclass nx::Class {
    :public method foo {} {return MClass-[next]}
    :public *method foo {} {return *-[next]}
    :public *method bar {} {return *-[next]}
}

#
# Define a class based on MClass and define here as well a method
# "foo" to show the next-path in combination with the *methods.
#
MClass create C {
    :public method foo {} {return C-[next]}
}

? {C info superclasses} "::MClass::*"

#
# Finally create an instance with the method foo as well.
#
C create c1 {
    :public object method foo {} {return c1-[next]}
}

#
# The result of "foo" reflects the execution order: object before
# classes (including the *method container).
#

? {c1 info precedence} "::C ::MClass::* ::nx::Object"
? {c1 foo} "c1-C-*-"
? {c1 bar} "*-"


#
# Define a Class D as a specialization of C
#
MClass create D -superclass C {
    :public method foo {} {return D-[next]}
    :create d1
}

? {d1 info precedence} "::D ::C ::MClass::* ::nx::Object"
? {d1 foo} "D-C-*-"

#
# Dynamically add *method "baz". 
#
? {d1 baz} "::d1: unable to dispatch method 'baz'"
MClass eval {
    :public *method baz {} {return baz*-[next]}
}
? {d1 baz} "baz*-"

#
# Test adding of *methods at a time, when the meta-class has already
# instances.
#
# Create a meta-class without a *method
nx::Class create MClass2 -superclass nx::Class
MClass2 create X {:create x1}
? {x1 info precedence} "::X ::nx::Object"

# Now add a *method
MClass2 eval {
    :public *method baz {} {return baz*-[next]}
}

# Adding the *method alters the superclass order of already created
# instances of the meta-class
? {x1 info precedence} "::X ::MClass2::* ::nx::Object"
? {x1 baz} "baz*-"


#
# Finally, there is a simple application example for ActiveRecord
# pattern. All instances of the application classes (such as
# "Product") should have a method "save" (together with other methods
# now shown here). First define the ActiveRecord class (as a
# meta-class).
#
Class create ActiveRecord -superclass nx::Class {
    :property table_name

    :method init {} {
	if {![info exists :table_name]} {
	    set :table_name [string tolower [namespace tail [self]]s]
	}
    }
    :public *method save {} {
	puts "save [self] into table [[:info class] cget -table_name]"
    }
}

#
# Define the application class "Product" with an instance
#
ActiveRecord create Product
Product create p1
p1 save

# The last command prints out: "save ::p1 into table products"


