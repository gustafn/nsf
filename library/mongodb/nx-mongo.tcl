#
# Object orientend mapping between MongoDB and nx. 
#
# Gustaf Neumann              fecit, April 2011
#
package require nx
package require nsf::mongo
package provide nx::mongo 0.2

# todo: how to handle multiple connections; currently we have a single, global connection
# todo: handle embedded bson objects
# todo: handle named nx objects (e.g. attribute _oid?}

#
# nx::mongo::Attribute is a specialized attribute slot
#
::nx::MetaSlot create ::nx::mongo::Attribute -superclass ::nx::Attribute {
  :attribute mongotype

  :protected method init {} {
    #
    # If the mongotype was not provided, set it to a value derived
    # from "type". Not all types are mappable easily to mongo types.
    #
    if {![info exists :mongotype]} {
      set :mongotype string
      if {[info exists :type]} {
	switch ${:type} {
	  "boolean" -
	  "integer" {set :mongotype ${:type}}
	}
      }
    }
    next
  }

  #
  # The methods "bson encode|decode" perform the low level type
  # mapping. For now, this handles just the array notation.
  #
  :method "bson decode" {bsontype value} {
    #puts stderr "bson decode of ${:name} /$bsontype/ '$value'"
    if {$bsontype eq "array"} {
      if {![:isMultivalued]} {
	# We got an array, but the slot is not multivalued. Maybe
	# generating an error is too harsh, but for the mapping back,
	# we check for multivalued as well.
	error "Attribute ${:name} should be multivalued, but it is not"
      }
      set result [list]
      foreach {pos type v} $value {lappend result $v}
      return $result
    }
    return $value
  }

  :method "bson encode" {value} {
    if {[:isMultivalued]} {
      set c -1
      set array [list]
      foreach v $value {lappend array [incr c] ${:mongotype} $v}
      return [list array $array]
    } else {
      return [list ${:mongotype} $value]
    }
  }
}

nx::Class create nx::mongo::Class -superclass nx::Class {
  
  #
  # Every mongo class can be configured with a document, from which
  # its instance data is queried.
  #
  :attribute document

  #
  # Provide helper methods to access from an external specifier
  # (attribute name or operator name) internal representations
  # (eg. mongo type, or mongo operator).
  #

  :method "get slot" {att} {
    set classes [concat [self] [:info mixin classes] [:info heritage]]
    foreach cls $classes {
      set slot [$cls info slot $att]
      if {$slot ne ""} {
	return $slot
      }
    }
  }

  :public method "get relop" {op} {
    array set "" {< $lt > $gt <= $lte >= $gte != $ne}
    return $($op)
  }

  #
  # For interaction with bson structures, we provide on the class
  # level "bson query" (a small dsl for a more convenient syntax in
  # bson queries) and "bson parameter" which translates from a bson
  # structure (tuple) into a dashed parameter list used in object
  # creation.
  #

  :method "bson query" {{-cond ""} {-orderby ""}} {
    set bson [list]
    foreach {att op value} $cond {
      set slot [:get slot $att]
      switch $op {
	"=" {lappend bson $att [$slot mongotype] $value}
	">" - "<" - "<=" - ">=" - "!="  {
	  lappend bson $att object [list [:get relop $op] [$slot mongotype] $value]
	}
	default {error "unknown operator $op"}
      }
    }
    set result [list \$query object $bson]
    if {[llength $orderby] > 0} {
      set bson [list]
      foreach attspec $orderby {
	lassign $attspec att direction
	lappend bson $att int [expr {$direction eq "desc" ? -1 : 1}]
      }
      lappend result \$orderby object $bson
    }
    puts "Query: $result"
    return $result
  }

  :method "bson parameter" {tuple} {
    set objParams [list]
    foreach {att type value} $tuple {
      set slot [:get slot $att]
      lappend objParams -$att [$slot bson decode $type $value]
    }
    return $objParams
  }

  #
  # Overload method attribute to provide "::nx::mongo::Attribute" as a
  # default slot class
  #
  :public method attribute {spec {-class ::nx::mongo::Attribute} {initblock ""}} {
    next [list $spec -class $class $initblock]
  }

  #
  # index method
  #
  :public method index {att {-type 1}} {
    # todo: 2d index will need a different type
    ::mongo::index $::mongoConn ${:document} [list $att int $type]
  }

  #
  # A convenience method for inserting a fresh tuple
  #
  :public method insert {args} {
    set p [:new {*}$args]
    $p save
    $p destroy
  }

  #
  # The query interface consists currently of "find first" (returning
  # a single instance) and "find all" (returning a list of instances).
  #
  :public method "find first" {
     -instance 
     {-cond ""}
     {-orderby ""} 
   } {
    set fetched [::mongo::query $::mongoConn ${:document} \
		     [:bson query -cond $cond -orderby $orderby] \
		     -limit 1]
    puts "[join $fetched \n]"
    foreach tuple $fetched {
      if {[info exists instance]} {
	set o [:uplevel [list [self] create $instance {*}[:bson parameter $tuple]]]
	return $o
      } else {
	return [:uplevel [list [self] new {*}[:bson parameter $tuple]]]
      }
    }
  }

  :public method "find all" {
     {-cond ""} 
     {-orderby ""} 
     {-limit} 
     {-skip} 
   } {
    set result [list]
    set opts [list]
    if {[info exists limit]} {lappend opts -limit $limit}
    if {[info exists skip]} {lappend opts -skip $skip}
    set fetched [::mongo::query $::mongoConn ${:document} \
		     [:bson query -cond $cond -orderby $orderby] \
		     {*}$opts]
    puts "[join $fetched \n]"
    foreach tuple $fetched {
      lappend result [:uplevel [list [self] new {*}[:bson parameter $tuple]]]
    }
    return $result
  }

  #
  # When a mongo::Class is created, mixin the mongo::Object to make
  # "save" etc. available
  #
  
  :method init {} {
    :mixin add ::nx::mongo::Object
  }

  # :public method create args {
  #   puts stderr CREATE-[self]-$args
  #   set o [next]
  #   $o mixin add ::nx::mongo::Object
  #   puts stderr CREATED-$o-[$o info mixin]
  #   return $o
  # }

}

#
# The class mongo::Object provides methods for mongo objects (such as
# "save")
#
nx::Class create nx::mongo::Object {

  #
  # _id is the special attribute maintained by mongoDB
  #
  :attribute _id -class ::nx::mongo::Attribute {
    set :mongotype oid
  }

  #
  # Save the current object. When we have an _id, perform an update,
  # otherwise perform an insert
  #
  :public method save {} {
    set bson [list]
    set cls [:info class]
    foreach var [:info vars] {
      set slot [$cls get slot $var]
      lappend bson $var {*}[$slot bson encode [set :$var]]
    }
    if {[info exists :_id]} {
      #puts stderr "we have to update $bson"
      ::mongo::update $::mongoConn [$cls document] [list _id oid ${:_id}] $bson
    } else {
      #puts stderr "we have to insert $bson"
      ::mongo::insert $::mongoConn [$cls document] $bson
    }
  }

}

