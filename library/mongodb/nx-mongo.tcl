#
# Object orientend mapping between MongoDB and nx. 
#
# Gustaf Neumann              fecit, April 2011
#
package require nx
package require nsf::mongo
package provide nx::mongo 0.2

# todo: how to handle multiple connections; currently we have a single, global connection
# todo: make embedded spec nicer
# todo: handle time stamps
# todo: handle remove for non-multivalued embedded objects
# idea: handle names of nx objects (e.g. attribute like __name)
# idea: handle classes von nx objects (e.g. attribute like __class)
# idea: combine incremental slot operations with e.g. add -> $push, remove -> $pull

namespace eval ::nx::mongo {

  ::nx::Object create ::nx::mongo::db {
    :public method connect {args} {set :mongoConn [::mongo::connect {*}$args]}
    :public method count   {args} {::mongo::count  ${:mongoConn} {*}$args}
    :public method index   {args} {::mongo::index  ${:mongoConn} {*}$args}
    :public method insert  {args} {::mongo::insert ${:mongoConn} {*}$args}
    :public method remove  {args} {::mongo::remove ${:mongoConn} {*}$args}
    :public method query   {args} {::mongo::query  ${:mongoConn} {*}$args}
    :public method update  {args} {::mongo::update ${:mongoConn} {*}$args}
  }
  
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
	  switch -glob ${:type} {
	    "boolean" -
	    "integer" {set :mongotype ${:type}}
	    "embedded" {set :mongotype object}
	  }
	  #"::*" {set :mongotype object}
	}
      }
      #puts stderr "mongo type of ${:name} is ${:mongotype} [info exists :type]"
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
	foreach {pos type v} $value {lappend result [:bson decode $type $v]}
	return $result
      } elseif {$bsontype eq "object"} {
	#puts stderr "*** we have an object '$value', [:serialize]"
	if {${:type} eq "embedded" && [info exists :arg]} {
	  set value [${:arg} bson create $value]
	  #puts stderr "*** ${:arg} bson create ==> $value"
	} else {
	  error "don't know how to decode object with value '$value'; [:serialize]"
	}
      }
      return $value
    }
    
    :method "bson encodeValue" {value} {
      if {${:mongotype} eq "object"} {
	return [list ${:mongotype} [$value bson encode]]
      } else {
	return [list ${:mongotype} $value]
      }
    }

    :method "bson encode" {value} {
      if {[:isMultivalued]} {
	set c -1
	set array [list]
	foreach v $value {lappend array [incr c] {*}[:bson encodeValue $v]}
	return [list array $array]
      } else {
	return [:bson encodeValue $value]
      }
    }

    :public method remove {object value} {
      if {[:isMultivalued]} {
	set values [::nsf::var::set $object ${:name}]
	set p [lsearch $values $value]
	if {$p < 0} {
	  error "$value not included in $object.$value ($values)"
	}
	set newValues [lreplace $values $p $p]
	::nsf::var::set $object ${:name} $newValues
      } else {
	error "remove just implemented for multivalued slots"
      }
    }

    #
    # Type converter for handling embedded objects. Makes sure to
    # track "embedded in" relationship
    #
    :public method type=embedded {name value arg} {
      set s [:uplevel self]
      #puts stderr "check $name '$value' arg='$arg' s=$s"
      if {[::nsf::isobject $value] && [::nsf::is class $arg] && [$value info has type $arg]} {
	::nsf::var::set $value __embedded_in [list $s $name]
	::nsf::var::set $s __contains($value) 1
      } else {
	error "value '$value' for attribute $name is not of type $arg"
      }
    }
  }
  
  ::nx::Class create ::nx::mongo::Class -superclass nx::Class {
    
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
      #puts "Query: $result"
      return $result
    }
    
    :method "bson parameter" {tuple} {
      set objParams [list]
      foreach {att type value} $tuple {
	set slot [:get slot $att]
	#puts stderr "att $att type $type value $value => '$slot'"
	lappend objParams -$att [$slot bson decode $type $value]
      }
      return $objParams
    }
    
    :public method "bson create" {{-name ""} tuple} {
      if {$name ne ""} {
	return [:create $name {*}[:bson parameter $tuple]]
      } else {
	return [:new {*}[:bson parameter $tuple]]
      }
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
      #::mongo::index $::mongoConn ${:document} [list $att int $type]
      db index ${:document} [list $att int $type]
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
    # The method "count" is similar to find, but returns just the
    # number of tuples for the query.
    #
    :public method count {{-cond ""}} {
      return [::nx::mongo::db count ${:document} $cond]
    }
    
    #
    # The query interface consists currently of "find first" (returning
    # a single instance) and "find all" (returning a list of instances).
    #
    :public method "find first" {
				 {-instance ""}
				 {-cond ""}
				 {-orderby ""} 
			       } {
      set tuple [lindex [::nx::mongo::db query ${:document} \
			     [:bson query -cond $cond -orderby $orderby] \
			     -limit 1] 0]
      #puts "find first fetched: $tuple"
      if {$instance ne ""} {set instance [:uplevel [list ::nsf::qualify $instance]]}
      return [:bson create -name $instance $tuple]
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
      set fetched [::nx::mongo::db query ${:document} \
		       [:bson query -cond $cond -orderby $orderby] \
		       {*}$opts]
      puts "[join $fetched \n]"
      foreach tuple $fetched {
	lappend result [:bson create $tuple]
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
  ::nx::Class create ::nx::mongo::Object {
    
    #
    # _id is the special attribute maintained by mongoDB
    #
    :attribute _id -class ::nx::mongo::Attribute {
      set :mongotype oid
    }

    #
    # Encode all object data in bson notation
    #
    :method "bson encode" {} {
      set bson [list]
      set cls [:info class]
      foreach var [:info vars] {
	set slot [$cls get slot $var]
	if {$slot ne ""} {
	  lappend bson $var {*}[$slot bson encode [set :$var]]
	}
      }
      return $bson
    }

    :method "bson pp_array" {{-indent 0} list} {
      set result [list]
      foreach {name type value} $list {
	switch $type {
	  object { lappend result "\{ [:bson pp -indent $indent $value] \}" }
	  array { lappend result "\[ [:bson pp_array -indent $indent $value] \]" }
	  default { lappend result [list $value]}
	}
      }
      return [join $result ", "]
    }

    :method "bson pp" {{-indent 0} list} {
      set result [list]
      set nextIndent [expr {$indent + 2}]
      foreach {name type value} $list {
	set prefix "\n[string repeat { } $indent]$name: "
	switch $type {
	  object { lappend result "$prefix\{ [:bson pp -indent $nextIndent $value] \}" }
	  array { lappend result "$prefix\[ [:bson pp_array -indent $nextIndent $value] \]" }
	  default { lappend result $prefix[list $value]}
	}
      }
      return [join $result ", "]
    }

    #
    # embedded_in denotes that the object is embedded in another
    # object with a reference to the attribute
    #
    # :public method embedded_in {object attribute} {
    #   set :__embedded_in [list $object $attribute]
    #   $object $attribute add [self] end
    # }

    #
    # destroy a mapped object from memory
    #
    :public method destroy {} {
      if {[array exists :__contains]} {
	# destroy embedded object
	foreach o [array names :__contains] {
	  puts "[self] contains $o -> destroy"
	  $o destroy
	}
      }
      if {[info exists :__embedded_in]} {
	lassign ${:__embedded_in} parent att
	::nsf::var::unset $parent __contains([self])
      }
      next
    }

    #
    # delete the current object from the db
    #
    :public method delete {} {
      puts stderr "[self] delete"
      if {[info exists :__embedded_in]} {
	puts "[self] is embedded in ${:__embedded_in}"
	lassign ${:__embedded_in} parent att
	set slot [[$parent info class] get slot $att]
	$slot remove $parent [self]
	#puts stderr [:serialize]
	puts stderr "[self] must save parent $parent in db"
	:destroy
      } else {
	puts "delete a non-embedded entry"
	if {[info exists :_id]} {
	  set document [[:info class] document]
	  ::nx::mongo::db remove $document [list _id oid ${:_id}]
	} else {
	  error "[self]: object does not contain an _id; it can't be delete from the mongo db."
	}
      }
    }
    
    #
    # Save the current object. When we have an _id, perform an update,
    # otherwise perform an insert
    #
    :public method save {} {
      set document [[:info class] document]
      if {$document eq ""} {
	# We could perform the delegation probably automatically, but
	# for now we provide an error
	error "No document specified for [:info class]. In case this is an embedded object, save the embedding one."
      } else {
	set bson [:bson encode]
	if {[info exists :_id]} {
	  puts stderr "we have to update [:bson pp -indent 4 $bson]"
	  ::nx::mongo::db update $document [list _id oid ${:_id}] $bson
	} else {
	  puts stderr "we have to insert [:bson pp -indent 4 $bson]"
	  set r [::nx::mongo::db insert $document $bson]
	  set :_id [lindex $r 2]
	}
      }
    }
  }
  
}