#
# Object orientend mapping between MongoDB and nx. 
#
# Gustaf Neumann              fecit, April 2011
#
package require nx
package require nsf::mongo
package provide nx::mongo 2.1

# todo: how to handle multiple connections; currently we have a single, global connection
# todo: all references are currently auto-fetched. make this optional
# todo: If "embeds" or "references" are used, the object must be of
#       the specified classes, no subclasses allowed
# todo: extend the query language syntax, e.g. regexp, ...
# todo: handle remove for non-multivalued embedded objects
# idea: handle names of nx objects (e.g. property like __name)
# idea: handle classes von nx objects (e.g. property like __class)
# idea: combine incremental slot operations with e.g. add -> $push, remove -> $pull
# todo: make "embedded", "reference" spec even nicer?

namespace eval ::nx::mongo {

  set ::nx::mongo::log 0

  ::nx::Object create ::nx::mongo::db {
    :object property db
    :object property mongoConn
    :object property gridFsName

    :public object method connect {{-db test} args} {
      if {[info exists :db]} {
        if {${:db} eq $db} {
          # reuse existing connection
          return ${:mongoConn}
        }
        ::mongo::close ${:mongoConn}
      }
      set :db $db
      set :mongoConn [::mongo::connect {*}$args]
    }

    :public object method close {} {
      if {[info exists :gridFs]} {
        ::nsf::log notice "nx::mongo: auto close gridfs"
        :gridfs close
      }
      foreach {ns coll} [array get :collection] {
        ::nsf::log notice "nx::mongo: auto close collection $ns $coll"
        ::mongo::collection::close $coll
      }
      ::mongo::close ${:mongoConn}
      unset :db :mongoConn
    }

    :public object method destroy {}     {
      if {[info exists :db]} {
        ::nsf::log notice "nx::mongo: auto close connection to database '${:db}'"
        ::mongo::close ${:mongoConn}
      }
    }
    :public object method collection {ns} {
      set key :collection($ns)
      if {[info exists $key]} {return [set $key]}
      if {[regexp {^([^.]+)[.](.+)$} $ns _ db coll]} {
        return [set $key [mongo::collection::open ${:mongoConn} $db $coll]]
      }
      return -code error "invalid mongo namespace '$ns'"
    }

    :public object method count  {ns args} {::mongo::collection::count  [:collection $ns] {*}$args}
    :public object method index  {ns args} {::mongo::collection::index  [:collection $ns] {*}$args}
    :public object method insert {ns args} {::mongo::collection::insert [:collection $ns] {*}$args}
    :public object method delete {ns args} {::mongo::collection::delete [:collection $ns] {*}$args}
    :public object method query  {ns args} {::mongo::collection::query  [:collection $ns] {*}$args}
    :public object method update {ns args} {::mongo::collection::update [:collection $ns] {*}$args}
    :public object method "drop collection" {name} {
      ::mongo::run -nocomplain ${:mongoConn} ${:db} [list drop string $name]
    }
    :public object method "drop database" {} {
      ::mongo::run -nocomplain ${:mongoConn} ${:db} [list dropDatabase integer 1]
    }
    :public object method "reset error" {} {
      ::mongo::run -nocomplain ${:mongoConn} ${:db} [list reseterror integer 1]
    }
    :public object method is_oid  {string} {expr {[string length $string] == 24}}

    #
    # GridFS
    # 
    :object property gridFs

    :public object method "gridfs open" {{name fs}} {
      if {[info exists :gridFsName]} {
        if {${:gridFsName} eq $name} {return ${:gridFs}}
        :gridfs close
      }
      set :gridFsName $name
      set :gridFs [::mongo::gridfs::open ${:mongoConn} ${:db} $name]
    }

    :public object method "gridfs close" {} {
      ::mongo::gridfs::close ${:gridFs}
      unset :gridFs :gridFsName
    }

    :public object method "gridfs create" {{-source file} value name {mime text/plain} {-metadata}} {
      ::mongo::gridfile::create -source $source ${:gridFs} $value $name $mime \
          {*}[expr {[info exists metadata] ? [list -metadata $metadata] : {}}]
    }

    :public object method "gridfs list" {{-all:switch false} query} {
      set coll [:collection ${:db}.${:gridFsName}.files]
      if {!$all} {
        set info [::mongo::collection::query $coll $query -limit 1]
        return [lindex $info 0]
      } else {
        set info [::mongo::collection::query $coll {}]
        return $info
      }
    }

    :public object method "gridfs update" {id bson} {
      ::mongo::collection::update [:collection ${:db}.${:gridFsName}.files] \
          [list _id oid $id] $bson
    }

    :public object method "file content" {query} {
      set f [mongo::gridfile::open ${:gridFs} $query]
      set content ""
      while {1} {
        append content [set chunk [mongo::gridfile::read $f 4096]]
        if {[string length $chunk] < 4096} {
          break
        }
      }
      mongo::gridfile::close $f
      return $content
    }
    
    :public object method "gridfs set attribute" {query attribute value} {
      set info [::nx::mongo::db gridfs list $query]
      if {$info eq ""} {return -code error "no such file <$query> stored in gridfs"}
      foreach {att type v} $info { dict set d $att $v }
      if {[dict exists $d $attribute] && [dict get $d $attribute] eq $value} {
        # right value, nothing to do
        return
      } elseif {[dict exists $d $attribute]} {
        # wrong value replace it
        set bson {}
        foreach {att type v} $info { 
          if {$att eq $attribute} {
            lappend bson $att $type $value
          } else {
            lappend bson $att $type $v
          }
        }
      } else {
        #no such value, add it
        lappend bson {*}$info $attribute string $value
      }
      nx::mongo::db gridfs update [dict get $d _id] $bson
    }

    :public object method "gridfs unset attribute" {query attribute} {
      set info [::nx::mongo::db gridfs list $query]
      if {$info eq ""} {return -code error "no such file <$query> stored in gridfs"}
      foreach {att type v} $info { dict set d $att $v }
      if {[dict exists $d $attribute]} {
        # delete the attribute
        nx::mongo::db gridfs update [dict get $d _id] [list \$unset document [list $attribute string ""]]
      } else {
        # does not exist, nothing to do
      }
    }

    :public object method "gridfs map" {query url} {
      # map always the newest entry
      set fullQuery [list \$query document $query \$orderby document {uploadDate integer -1}]
      ::nx::mongo::db gridfs set attribute $fullQuery url $url
    }
    :public object method "gridfs mapped" {url} {
      set info [::mongo::collection::query [:collection ${:db}.${:gridFsName}.files] \
                    [list \$query document [list url string $url]] \
                    -limit 1]
      return [lindex $info 0]
    }


  }
  
  #######################################################################
  # nx::mongo::Attribute is a specialized property slot
  #
  ::nx::MetaSlot create ::nx::mongo::Attribute -superclass ::nx::VariableSlot {
    :property mongotype
    :property rep
    
    #
    # manage logging of mongo concerns
    #
    :public method log {msg} {
      if {$::nx::mongo::log} {
	nsf::log notice "mongo-attribute: $msg"
      }
    }

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

	  return -code error "Attribute ${:name} should be multivalued, but it is not"
	}
	set result [list]
	foreach {pos type v} $value {lappend result [:bson decode $type $v]}
	return $result
      } elseif {$bsontype eq "document"} {
	#puts stderr "*** we have an document '$value', [:serialize]"
	if {${:type} eq "embedded" && [info exists :arg]} {
	  #puts stderr "*** we have an embed class = ${:arg}"
	  set value [${:arg} bson create $value]
	  #puts stderr "*** ${:arg} bson create ==> $value"
	} elseif {${:type} eq "reference" && [info exists :arg]} {
	  #puts stderr "*** we have a reference, class = ${:arg}"
	  # TODO we assume auto_deref
	  set value [:bson deref ${:arg} $value]
	  #puts stderr "*** bson deref ${:arg} ==> $value"
	} else {
	  return -code error "don't know how to decode document with value '$value'; [:serialize]"
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
	return -code error "value to be dereferenced does not contain dbref id: $value"
      }
      if {[info exists (db)]} {
	if {$(db) ne [$class cget -mongo_db]} {
	  return -code error "$(db) is different to [$class cget -mongo_db]"
	}
      }
      if {[info exists (ref)]} {
	if {$(ref) ne [$class cget -mongo_collection]} {
	  return -code error "$(ref) is different to [$class cget -mongo_collection]"
	}
      }
      return [$class find first -cond [list _id = $(id)]]
    }

    :method "bson encodeValue" {value} {
      if {${:mongotype} eq "embedded_object"} {
        #puts "embedded_object <$value>"
	return [list document [$value bson encode]]
      } elseif {${:mongotype} eq "referenced_object"} {
	if {![::nsf::var::exists $value _id]} {
	  :log "autosave $value to obtain an object_id"
	  $value save
	}
	set _id [$value cget -_id]
	set cls [$value info class]
	return [list document [list \
				 {$ref} string [$cls cget -mongo_collection] \
				 {$id} oid $_id \
				 {$db} string [$cls cget -mongo_db]]]
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
	  return -code error "$value not included in $object.$value ($values)"
	}
	set newValues [lreplace $values $p $p]
	::nsf::var::set $object ${:name} $newValues
      } else {
	return -code error "remove just implemented for multivalued slots"
      }
    }

    #
    # Type converter for handling embedded objects. Makes sure to
    # track "embedded in" relationship
    #
    :public method type=embedded {name value arg} {
      # Determine the calling object of the type converter, which
      # might be object itself or a variable slot managing the object.
      set s [:uplevel self]
      #puts stderr "XXXX check $name '$value' arg='$arg' s=$s // [:uplevel 1 self]"
      # The calling objects have the the mongo::Object mixed in.
      if {![$s info has mixin ::nx::mongo::Object]} {
        # If this is not the case, we might be in a variable slot,
        # where we cannot trust the incoming name and we have to
        # obtain the object from one level higher.
        if {[$s info has type ::nx::VariableSlot]} {
          set name [$s cget -name]
          set s [:uplevel 2 self]
          if {![$s info has mixin ::nx::mongo::Object]} {set s ""}
        } else {
          # no slot object, some strange constellation
          set s ""
        }
        if {$s eq ""} {
          return -code error "$name '$value' is not embedded in object of type ::nx::mongo::Object"
        }
      }

      if {[::nsf::object::exists $value] && [::nsf::is class $arg] && [$value info has type $arg]} {
	::nsf::var::set $value __embedded_in [list $s $name]
	::nsf::var::set $s __contains($value) 1
      } else {
	return -code error "value '$value' for property $name is not of type $arg"
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
	return -code error "value '$value' for property $name is not of type $arg"
      }
    }
    #
    # Type converter for datetime handling (scan date strings into
    # input values into integers in the form mongo expects it)
    #
    :public method type=datetime {name value} {
      # puts stderr "...  [clock format [clock scan $value -format {%B %d, %Y}] -format {%B %d, %Y}]"
      # MongoDB stores time in ms
      if {[info exists :scanformat]} {return [expr {[clock scan $value -format ${:scanformat}] * 1000}]}
      return [expr {[clock scan $value] * 1000}]
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
    :property mongo_ns
    :property mongo_db
    :property mongo_collection
    
    #
    # Provide helper methods to access from an external specifier
    # (property name or operator name) internal representations
    # (eg. mongo type, or mongo operator).
    #
    :method "get slot" {att} {
      set classes [concat [self] [:info mixins] [:info heritage]]
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
    # bson queries), "bson opts" (options like e.g.  ordering), "bson
    # atts (a simplifed property selection) and "bson parameter" which
    # translates from a bson structure (tuple) into a dashed parameter
    # list used in object creation.
    #

    :method "bson cond" {cond} {
      #puts "bson cond $cond"
      set bson [list]
      foreach {att op value} $cond {
	set slot [:get slot $att]
	if {$slot eq ""} {return -code error "could not obtain slot for <$att $op $value>"}
	switch $op {
	  "=" {lappend bson $att [$slot cget -mongotype] $value}
	  ">" - "<" - "<=" - ">=" - "!="  {
	    lappend bson $att document [list [:get relop $op] [$slot cget -mongotype] $value]
	  }
	  "in" - "all" {
	    lappend bson $att document [list [:get relop $op] {*}[$slot bson encode -array $value]]
	  }
          "~" {
            # value should be a two-element list contain pattern and options
	    lappend bson $att document [list {$regex} regex $value]
	  }

	  default {return -code error "unknown operator $op"}
	}
      }
      #puts "bson cond <$cond> => $bson"
      return $bson
    }
    
    :method "bson opts" {{-orderby ""} {-atts ""} -limit:integer -skip:integer} {
      set result ""
      if {$atts ne ""} {
        lappend result projection document [:bson atts $atts]
      }
      if {[info exists limit]} {
        lappend result limit int64 $limit
      }
      if {[info exists skip]} {
        lappend result skip int64 $skip
      }
      if {$orderby ne ""} {
        lappend result sort document [:bson orderby $orderby]
      }
      return $result
    }

    :method "bson orderby" {orderby} {
      set bson [list]
      foreach attspec $orderby {
        lassign $attspec att direction
        lappend bson $att int [expr {$direction eq "desc" ? -1 : 1}]
      }
      return $bson
    }
    
    :method "bson atts" {atts} {
      set result {}
      foreach {att value} $atts {
	if {![string is integer -strict $value]} {
	  return -code error "$atts: $value should be integer"
	}
	lappend result $att int $value
      }
      return $result
    }

    :method "bson parameter" {tuple} {
      #
      # Translate bson tuple into a parameter values pairs suited as
      # configure parameters
      #
      #puts "bson parameter: <$tuple>"
      set objParams [list]
      foreach {att type value} $tuple {
	set slot [:get slot $att]
	if {$slot eq ""} {return -code error "could not obtain slot for <$att $type $value>"}
	lappend objParams -$att [$slot bson decode $type $value]
      }
      #puts "bson parameter <$tuple> => $objParams"
      return $objParams
    }

    :method "bson setvalues" {tuple} {
      #
      # Translate bson tuple into a cmd to set instance values, which
      # can be evaluated in the context of an object.
      #
      #puts "bson setvalues: <$tuple>"
      set cmd ""
      foreach {att type value} $tuple {
	set slot [:get slot $att]
	if {$slot eq ""} {return -code error "could not obtain slot for <$att $type $value>"}
        if {[nx::var exists $slot rep] && [nx::var set $slot rep] ne ""} {
          set script [:bson rep decode [nx::var set $slot rep] $slot $att $type $value]
          append cmd $script\n
        } else {
          append cmd "set [list :$att] [list [$slot bson decode $type $value]]\n"
        }
      }
      #puts "bson parameter <$tuple> => $objParams"
      return $cmd
    }

    :public method "bson create" {{-name ""} tuple} {
      set o [::nsf::object::alloc [self] $name [:bson setvalues $tuple]]
      $o eval :init
      return $o
    }

    :method "bson pp_array" {{-indent 0} list} {
      set result [list]
      foreach {name type value} $list {
	switch $type {
	  document { lappend result "\{ [:bson pp -indent $indent $value] \}" }
	  array    { lappend result "\[ [:bson pp_array -indent $indent $value] \]" }
	  default  { lappend result [list $value]}
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
	  document { lappend result "$prefix\{ [:bson pp -indent $nextIndent $value] \}" }
	  array    { lappend result "$prefix\[ [:bson pp_array -indent $nextIndent $value] \]" }
	  default  { lappend result $prefix[list $value]}
	}
      }
      return [join $result ", "]
    }

    #
    # Overload method property to provide "::nx::mongo::Attribute" as a
    # default slot class
    #
    :public method property {
                             {-accessor ""}
                             {-class ::nx::mongo::Attribute}
                             {-configurable:boolean true}
                             {-incremental:switch}
                             {-rep ""}
                             spec:parameter
                             {initblock ""}
                           } {
      regsub -all {,type=::} $spec {,arg=::} spec
      set result [next [list -accessor $accessor -class $class \
                            -configurable $configurable -incremental=$incremental \
                            $spec $initblock]]
      lassign [::nx::MetaSlot parseParameterSpec $spec] name 
      [:info slots $name] configure -rep $rep
      return $result
    }

    :public method variable {
                             {-accessor "none"}
                             {-class ::nx::mongo::Attribute}
                             {-configurable:boolean false}
                             {-incremental:switch}
                             {-initblock ""}
                             {-rep ""}
                             spec:parameter
                             defaultValue:optional
                           } {
      regsub -all {,type=::} $spec {,arg=::} spec
      set result [next [list -accessor $accessor -class $class \
                            -configurable $configurable -incremental=$incremental \
                            -initblock $initblock $spec \
                            {*}[expr {[info exists defaultValue] ? [list $defaultValue] : ""}]]]
      lassign [::nx::MetaSlot parseParameterSpec $spec] name 
      [:info slots $name] configure -rep $rep
      return $result
    }

    :public method pretty_variables {} {
      set vars {}
      foreach p [lmap handle [lsort [:info variables]] {::nx::Object info variable parameter $handle}] {
        if {[regexp {^([^:]+):(.*)$} $p _ name options]} {
          set resultOptions {}
          set opts [split $options ,]
          if {[lindex $opts 0] eq "embedded"} {
            set resultOpts {}
            foreach opt $opts {
              switch -glob $opt {
                slot=* {continue}
                arg=*  {lappend resultOpts type=[string range $opt 4 end]}
                default {lappend resultOpts $opt}
              }
            }
            lappend vars $name:[join $resultOpts ,]
            continue
          }
        }
        lappend vars $p
      }
      return $vars
    }
    
    #
    # index method
    #
    :public method index {att {-type 1} args} {
      if {![info exists :mongo_ns]} {:mongo_setup}
      # todo: 2nd index will need a different type
      # todo: multi-property indices
      db index ${:mongo_ns} [list $att int $type] {*}$args
    }
    
    #
    # A convenience method for inserting a fresh tuple
    #
    :public method insert {args} {
      set p [:new {*}$args]
      $p save
      set _id [$p cget -_id]
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
			     [:bson cond $cond] \
                             -opts [:bson opts -atts $atts -limit 1 -orderby $orderby] \
                            ] 0]
      if {$tuple eq ""} {
        return ""
      }
      if {$instance ne ""} {set instance [:uplevel [list ::nsf::object::qualify $instance]]}
      return [:bson create -name $instance $tuple]
    }
    
    :public method "find all" {
			       {-atts ""}
			       {-cond ""} 
			       {-orderby ""} 
			       {-limit:integer} 
			       {-skip:integer} 
			     } {
      set result [list]
      set opts [list]
      if {[info exists limit]} {lappend opts -limit $limit}
      if {[info exists skip]} {lappend opts -skip $skip}
      set fetched [::nx::mongo::db query ${:mongo_ns} \
		       [:bson cond $cond] \
                       -opts [:bson opts -orderby $orderby -atts $atts {*}$opts] ]

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
			 {-puts:boolean 1} 
                       } {
      set opts [list]
      if {[info exists limit]} {lappend opts -limit $limit}
      if {[info exists skip]} {lappend opts -skip $skip}
      set fetched [::nx::mongo::db query ${:mongo_ns} \
		       [:bson cond $cond] \
                       -opts [:bson opts -orderby $orderby -atts $atts {*}$opts] ]
      set tuples [list]
      foreach tuple $fetched {
	lappend tuples "\{[:bson pp -indent 4 $tuple]\n\}"
      }
      if {$puts} {puts [join $tuples ", "]}
      return $tuples
    }
    
    :method mongo_setup {} {
      #
      # setup mongo_collection, mongo_db and mongo_ns
      #
      if {[info exists :mongo_ns]} {
	#puts stderr "given mongo_ns ${:mongo_ns}"
	if {![regexp {^([^.]+)[.](.*)$} ${:mongo_ns} :mongo_db :mongo_collection]} {
	  return -code error "${:mongo_ns} does not contain a dot."
	}
      } else {
	if {![info exists :mongo_collection]} {
	  set :mongo_collection [string tolower [namespace tail [self]]]s
	}
	if {![info exists :mongo_db]} {
	  set :mongo_db [::nx::mongo::db cget -db]
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
      :mixins add ::nx::mongo::Object
      :mongo_setup
    }

    #
    :public method "bson rep encode array" {slot obj name} {
      return [$slot bson encode -array [$obj eval [list array get :$name]]]
    }
    :public method "bson rep decode array" {slot name bsontype value} {
      set result [list]
      foreach {pos type v} $value {lappend result [$slot bson decode $type $v]}
      return [list array set :$name $result]
    }
  }

  #
  # Allow special representations in MongoDB for instance variables.
  # The methods
  #
  #   bson rep encode <CODEC> ....
  #   bson rep decode <CODEC> ....
  #
  # allow for creating tailored methods to obtain + encode instance
  # variables and for decode an setting these. The codecs
  # (coder/decoder) are extensible on the application level by
  # defining ensemble methods with the name of the codec as last part.
  
  ::nx::mongo::Class eval {
    #
    # rep codec "array"
    #
    :public method "bson rep encode array" {slot obj name} {
      set body {}
      set c 0
      foreach {k v} [$obj eval [list array get :$name]] {
        lappend body [incr c] document [list k string $k v string $v]
      }
      return [list array $body]
    }
    :public method "bson rep decode array" {slot name bsontype value} {
      set av ""
      foreach {pos type entry} $value {
        lappend av [lindex $entry 2] [lindex $entry 5] 
      }
      return "array set :$name [list $av]"
    }

    #
    # rep codec "dict"
    #
    :public method "bson rep encode dict" {slot obj name} {
      set body {}
      dict for {k v} [$obj eval [list set :$name]] {
        lappend body $k string $v 
      }
      return [list document $body]
    }
    :public method "bson rep decode dict" {slot name bsontype value} {
      set result ""
      foreach {k type v} $value {
        lappend result $k $v
      }
      return "set :$name \[dict create $result\]"
    }
  }
  
  #######################################################################
  # The class mongo::Object provides methods for mongo objects (such as
  # "save")
  #
  ::nx::Class create ::nx::mongo::Object {
    
    #
    # manage logging of mongo concerns
    #
    :public method log {msg} {
      if {$::nx::mongo::log} {
	nsf::log notice "mongo: $msg"
      }
    }

    #
    # _id is the special property maintained by mongoDB
    #
    :property -accessor public -class ::nx::mongo::Attribute _id  {
      set :mongotype oid
    }

    #
    # Encode all object data in bson notation
    #
    :method "bson encode" {{-ignore ""}} {
      set bson [list]
      set cls [:info class]
      foreach var [:info vars] {
        if {$var in $ignore} continue
	set slot [$cls get slot $var]
	if {$slot ne ""} {
          if {[nx::var exists $slot rep] && [nx::var set $slot rep] ne ""} {
            lappend bson $var {*}[$cls bson rep encode [nx::var set $slot rep] $slot [self] $var]
          } else {
            lappend bson $var {*}[$slot bson encode [set :$var]]
          }
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
	  :log "[self] contains $o -> destroy"
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
      if {[info exists :__embedded_in]} {
        #
	# When an embedded object is deleted, it is removed for the
	# reference list. The containing object is not automatically
	# saved for the time being. We could consider an automatic
	# save or mongo-$pull update operation.
        #
	#puts "[self] is embedded in ${:__embedded_in}"
	lassign ${:__embedded_in} parent att
	set slot [[$parent info class] get slot $att]
	if {$slot eq ""} {return -code error "could not obtain slot for <[$parent info class] $att>"}
	$slot remove $parent [self]
	#puts stderr [:serialize]
	:log "[self] must save parent $parent in db"
	:destroy
      } elseif {[info exists :__referenced_in]} {
        #
	# When a referenced is deleted, we do for now essentially the
	# same as for embedded objects. However, the same object might
	# be referenced by several objects.
        #
	#puts "[self] is referenced in ${:__referenced_in}"
	foreach reference ${:__referenced_in} {
	  lassign $reference parent att
	  set slot [[$parent info class] get slot $att]
	  if {$slot eq ""} {return -code error "could not obtain slot for <[$parent info class] $att>"}
	  $slot remove $parent [self]
	  :log "[self] must save parent $parent in db"
	}
	:destroy
      } else {
	#puts "delete a non-embedded entry"
	if {[info exists :_id]} {
	  set mongo_ns [[:info class] cget -mongo_ns]
	  ::nx::mongo::db delete $mongo_ns [list _id oid ${:_id}]
	} else {
	  return -code error "[self]: object does not contain an _id; it can't be delete from the mongo db."
	}
      }
    }
    
    #
    # Save the current object. When we have an _id, perform an update,
    # otherwise perform an insert
    #
    :public method save {} {
      set mongo_ns [[:info class] cget -mongo_ns]
      if {$mongo_ns eq ""} {
	# We could perform the delegation probably automatically, but
	# for now we provide an error
	return -code error "No mongo_ns specified for [:info class]. In case this is an embedded object, save the embedding one."
      } else {
	set bson [:bson encode]
	if {[info exists :_id]} {
	  :log "we have to update [[:info class] bson pp -indent 4 $bson]"
	  ::nx::mongo::db update $mongo_ns [list _id oid ${:_id}] $bson
	  set :_id
	} else {
	  :log "we have to insert [[:info class] bson pp -indent 4 $bson]"
	  set r [::nx::mongo::db insert $mongo_ns $bson]
	  set :_id [lindex $r 2]
	}
      }
    }
  }
  
}

#
# Local variables:
#    mode: Tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:
