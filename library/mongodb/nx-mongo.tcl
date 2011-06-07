#
# Object orientend mapping between MongoDB and nx. 
#
# Gustaf Neumann              fecit, April 2011
#
package require nx
package require nsf::mongo
package provide nx::mongo 0.2

# todo: how to handle multiple connections; currently we have a single, global connection
# todo: all references are currently auto-fetched. make this optional
# todo: If "emebds" or "references" are used, the object must be of 
#       the specified classes, no subclasses allowed
# todo: extend the query language syntax, e.g. regexp, ...
# todo: handle remove for non-multivalued embedded objects
# idea: handle names of nx objects (e.g. attribute like __name)
# idea: handle classes von nx objects (e.g. attribute like __class)
# idea: combine incremental slot operations with e.g. add -> $push, remove -> $pull
# todo: make "embedded", "reference" spec even nicer?

namespace eval ::nx::mongo {

  ::nx::Object create ::nx::mongo::db {
    :attribute db
    :public method connect {{-db test} args} {
      set :db $db
      set :mongoConn [::mongo::connect {*}$args]
    }
    :public method count   {args} {::mongo::count  ${:mongoConn} {*}$args}
    :public method index   {args} {::mongo::index  ${:mongoConn} {*}$args}
    :public method insert  {args} {::mongo::insert ${:mongoConn} {*}$args}
    :public method remove  {args} {::mongo::remove ${:mongoConn} {*}$args}
    :public method query   {args} {::mongo::query  ${:mongoConn} {*}$args}
    :public method update  {args} {::mongo::update ${:mongoConn} {*}$args}
  }
  
  #######################################################################
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
	    "embedded" {set :mongotype embedded_object}
	    "reference" {set :mongotype referenced_object}
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
	  #puts stderr "*** we have an embed class = ${:arg}"
	  set value [${:arg} bson create $value]
	  #puts stderr "*** ${:arg} bson create ==> $value"
	} elseif {${:type} eq "reference" && [info exists :arg]} {
	  #puts stderr "*** we have a reference, class = ${:arg}"
	  # TODO we assume auto_deref
	  set value [:bson deref ${:arg} $value]
	  puts stderr "*** bson deref ${:arg} ==> $value"
	} else {
	  error "don't know how to decode object with value '$value'; [:serialize]"
	}
      }
      return $value
    }

    :method "bson deref" {class value} {
      #puts stderr "*** bson deref $class '$value'"
      foreach {name type v} $value {
	if {[string match {$*} $name]} {set ([string range $name 1 end]) $v}
      }
      if {![info exists (id)]} {
	error "value to be dereferenced does not contain dbref id: $value"
      }
      if {[info exists (db)]} {
	if {$(db) ne [$class mongo_db]} {error "$(db) is different to [$class mongo_db]"}
      }
      if {[info exists (ref)]} {
	if {$(ref) ne [$class mongo_collection]} {error "$(ref) is different to [$class mongo_collection]"}
      }
      return [$class find first -cond [list _id = $(id)]]
    }

    :method "bson encodeValue" {value} {
      if {${:mongotype} eq "embedded_object"} {
	return [list object [$value bson encode]]
      } elseif {${:mongotype} eq "referenced_object"} {
	if {![::nsf::var::exists $value _id]} {
	  puts stderr "autosave $value to obtain an object_id"
	  $value save
	}
	set _id [$value _id]
	set cls [$value info class]
	return [list object [list \
				 {$ref} string [$cls mongo_collection] \
				 {$id} oid $_id \
				 {$db} string [$cls mongo_db]]]
      } else {
	return [list ${:mongotype} $value]
      }
    }

    :method "bson encodeArray" {value} {
      set c -1
      set array [list]
      foreach v $value {lappend array [incr c] {*}[:bson encodeValue $v]}
      return [list array $array]
    }

    :public method "bson encode" {-array:switch value} {
      if {[:isMultivalued] || $array} {
	return [:bson encodeArray $value]
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
      if {[::nsf::object::exists $value] && [::nsf::is class $arg] && [$value info has type $arg]} {
	::nsf::var::set $value __embedded_in [list $s $name]
	::nsf::var::set $s __contains($value) 1
      } else {
	error "value '$value' for attribute $name is not of type $arg"
      }
    }
    #
    # Type converter for handling embedded objects. Makes sure to
    # track "embedded in" relationship
    #
    :public method type=reference {name value arg} {
      set s [:uplevel self]
      #puts stderr "check $name '$value' arg='$arg' s=$s"
      if {[::nsf::object::exists $value] && [::nsf::is class $arg] && [$value info has type $arg]} {
	set ref [list $s $name]
	if {[::nsf::var::exists $value __referenced_in]} {
	  set refs [::nsf::var::set $value __referenced_in]
	  if {[lsearch $refs $ref] == -1} {lappend refs $ref}
	} else {
	  set refs [list $ref]
	}
	::nsf::var::set $value __referenced_in $refs
      } else {
	error "value '$value' for attribute $name is not of type $arg"
      }
    }
  }
  

  #######################################################################
  # The class mongo::Class provides methods for mongo classes (such as
  # "find", "insert", ...)
  #
  ::nx::Class create ::nx::mongo::Class -superclass nx::Class {
    
    #
    # Every mongo class can be configured with a mongo_ns, from which
    # its instance data is queried.
    #
    :attribute mongo_ns
    :attribute mongo_db
    :attribute mongo_collection
    
    #
    # Provide helper methods to access from an external specifier
    # (attribute name or operator name) internal representations
    # (eg. mongo type, or mongo operator).
    #
    
    :method "get slot" {att} {
      set classes [concat [self] [:info mixin classes] [:info heritage]]
      foreach cls $classes {
	set slot [$cls info slots $att]
	if {$slot ne ""} {
	  return $slot
	}
      }
    }
    
    :public method "get relop" {op} {
      array set "" {< $lt > $gt <= $lte >= $gte != $ne in $in all $all}
      return $($op)
    }
    
    #
    # For interaction with bson structures, we provide on the class
    # level "bson cond" (a small dsl for a more convenient syntax in
    # bson queries), "bson query" (combining conditions with
    # ordering), "bson atts (a simplifed attribute selection) and
    # "bson parameter" which translates from a bson structure (tuple)
    # into a dashed parameter list used in object creation.
    #

    :method "bson cond" {cond} {
      #puts "bson cond $cond"
      set bson [list]
      foreach {att op value} $cond {
	set slot [:get slot $att]
	switch $op {
	  "=" {lappend bson $att [$slot mongotype] $value}
	  ">" - "<" - "<=" - ">=" - "!="  {
	    lappend bson $att object [list [:get relop $op] [$slot mongotype] $value]
	  }
	  "in" - "all" {
	    lappend bson $att object [list [:get relop $op] {*}[$slot bson encode -array $value]]
	  }
	  default {error "unknown operator $op"}
	}
      }
      #puts "bson cond <$cond> => $bson"
      return $bson
    }
    
    :method "bson query" {{-cond ""} {-orderby ""}} {
      #puts "bson query -cond <$cond> -orderby <$orderby>"
      set bson [:bson cond $cond]
      set result [list \$query object $bson]

      if {[llength $orderby] > 0} {
	set bson [list]
	foreach attspec $orderby {
	  lassign $attspec att direction
	  lappend bson $att int [expr {$direction eq "desc" ? -1 : 1}]
	}
	lappend result \$orderby object $bson
      }
      #puts "bson query -cond <$cond> -orderby <$orderby> => $result"
      return $result
    }

    :method "bson atts" {atts} {
      set result {}
      foreach {att value} $atts {
	if {![string is integer -strict $value]} {
	  error "$atts: $value should be integer"
	}
	lappend result $att int $value
      }
      return $result
    }

    :method "bson parameter" {tuple} {
      #puts "bson parameter: <$tuple>"
      set objParams [list]
      foreach {att type value} $tuple {
	set slot [:get slot $att]
	#puts stderr "att $att type $type value $value => '$slot'"
	lappend objParams -$att [$slot bson decode $type $value]
      }
      #puts "bson parameter <$tuple> => $objParams"
      return $objParams
    }
    
    :public method "bson create" {{-name ""} tuple} {
      if {$name ne ""} {
	return [:create $name {*}[:bson parameter $tuple]]
      } else {
	return [:new {*}[:bson parameter $tuple]]
      }
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

    :public method "bson pp" {{-indent 0} list} {
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
    # Overload method attribute to provide "::nx::mongo::Attribute" as a
    # default slot class
    #
    :public method attribute {spec {-class ::nx::mongo::Attribute} {initblock ""}} {
      regsub -all {,type=} $spec {,arg=} spec
      next [list $spec -class $class $initblock]
    }
    
    #
    # index method
    #
    :public method index {att {-type 1} args} {
      if {![info exists :mongo_ns]} {:mongo_setup}
      # todo: 2nd index will need a different type
      # todo: multi-attribute indices
      db index ${:mongo_ns} [list $att int $type] {*}$args
    }
    
    #
    # A convenience method for inserting a fresh tuple
    #
    :public method insert {args} {
      set p [:new {*}$args]
      $p save
      set _id [$p _id]
      $p destroy
      return $_id
    }

    #
    # The method "count" is similar to find, but returns just the
    # number of tuples for the query.
    #
    :public method count {{-cond ""}} {
      return [::nx::mongo::db count ${:mongo_ns} [:bson cond $cond]]
    }
    
    #
    # The query interface consists currently of "find first" (returning
    # a single instance) and "find all" (returning a list of instances).
    #
    :public method "find first" {
				 {-instance ""}
				 {-atts ""}
				 {-cond ""}
				 {-orderby ""} 
			       } {
      set tuple [lindex [::nx::mongo::db query ${:mongo_ns} \
			     [:bson query -cond $cond -orderby $orderby] \
			     -atts [:bson atts $atts] \
			     -limit 1] 0]
      #puts "find first fetched: $tuple"
      if {$instance ne ""} {set instance [:uplevel [list ::nsf::object::qualify $instance]]}
      return [:bson create -name $instance $tuple]
    }
    
    :public method "find all" {
			       {-atts ""}
			       {-cond ""} 
			       {-orderby ""} 
			       {-limit} 
			       {-skip} 
			     } {
      set result [list]
      set opts [list]
      if {[info exists limit]} {lappend opts -limit $limit}
      if {[info exists skip]} {lappend opts -skip $skip}
      set fetched [::nx::mongo::db query ${:mongo_ns} \
		       [:bson query -cond $cond -orderby $orderby] \
		       -atts [:bson atts $atts] \
		       {*}$opts]
      puts "[join $fetched \n]"
      foreach tuple $fetched {
	lappend result [:bson create $tuple]
      }
      return $result
    }

    :public method show {
			 {-atts ""}
			 {-cond ""} 
			 {-orderby ""} 
			 {-limit} 
			 {-skip} 
			 } {
      set result [list]
      set opts [list]
      if {[info exists limit]} {lappend opts -limit $limit}
      if {[info exists skip]} {lappend opts -skip $skip}
      set fetched [::nx::mongo::db query ${:mongo_ns} \
		       [:bson query -cond $cond -orderby $orderby] \
		       -atts [:bson atts $atts] \
		       {*}$opts]
      set tuples [list]
      foreach tuple $fetched {
	lappend tuples "\{[:bson pp -indent 4 $tuple]\n\}"
      }
      puts [join $tuples ", "]
    }
    
    :method mongo_setup {} {
      #
      # setup mongo_collection, mongo_db and mongo_ns
      #
      if {[info exists :mongo_ns]} {
	#puts stderr "given mongo_ns ${:mongo_ns}"
	if {![regexp {^([^.]+)[.](.*)$} ${:mongo_ns} :mongo_db :mongo_collection]} {
	  error "${:mongo_ns} does not contain a dot."
	}
      } else {
	if {![info exists :mongo_collection]} {
	  set :mongo_collection [string tolower [namespace tail [self]]]s
	}
	if {![info exists :mongo_db]} {
	  set :mongo_db [::nx::mongo::db db]
	}
	set :mongo_ns ${:mongo_db}.${:mongo_collection}
	#puts stderr "mongo_ns is set to ${:mongo_ns}"
      }
    }

    #
    # When a mongo::Class is created, mixin the mongo::Object to make
    # "save" etc. available
    #
    
    :method init {} {
      :mixin add ::nx::mongo::Object
      :mongo_setup
    }
  }
  
  #######################################################################
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
	# When an embedded object is deleted, it is removed for the
	# reference list. The containing object is not automatically
	# saved for the time being. We could consider an automatic
	# save or mongo-$pull update operation.
	puts "[self] is embedded in ${:__embedded_in}"
	lassign ${:__embedded_in} parent att
	set slot [[$parent info class] get slot $att]
	$slot remove $parent [self]
	#puts stderr [:serialize]
	puts stderr "[self] must save parent $parent in db"
	:destroy
      } elseif {[info exists :__referenced_in]} {
	# When a referenced is deleted, we do for now essentially the
	# same as for embedded objects. However, the same object might
	# be referenced by several objects.
	puts "[self] is referenced in ${:__referenced_in}"
	foreach reference ${:__referenced_in} {
	  lassign $reference parent att
	  set slot [[$parent info class] get slot $att]
	  $slot remove $parent [self]
	  puts stderr "[self] must save parent $parent in db"
	}
	:destroy
      } else {
	puts "delete a non-embedded entry"
	if {[info exists :_id]} {
	  set mongo_ns [[:info class] mongo_ns]
	  ::nx::mongo::db remove $mongo_ns [list _id oid ${:_id}]
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
      set mongo_ns [[:info class] mongo_ns]
      if {$mongo_ns eq ""} {
	# We could perform the delegation probably automatically, but
	# for now we provide an error
	error "No mongo_ns specified for [:info class]. In case this is an embedded object, save the embedding one."
      } else {
	set bson [:bson encode]
	if {[info exists :_id]} {
	  puts stderr "we have to update [[:info class] bson pp -indent 4 $bson]"
	  ::nx::mongo::db update $mongo_ns [list _id oid ${:_id}] $bson
	} else {
	  puts stderr "we have to insert [[:info class] bson pp -indent 4 $bson]"
	  set r [::nx::mongo::db insert $mongo_ns $bson]
	  set :_id [lindex $r 2]
	}
      }
    }
  }
  
}
