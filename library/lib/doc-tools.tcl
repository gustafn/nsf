# @package nx::doc
# 
# Study for documentation classes for Next Scriptint
# 
# Compared to the "old" @ docmentation effort, this is a rather
# light-weight structure based on xotcl 2 (next) language
# features. The documentation classes build an (extensible) object
# structure which is used as a basis for some renderers. In general,
# the classes are defined in a way they can be used for
#
#  a) building documentation outside the source code artefacts, or
#
#  b) inside code artefacts (value added method definition commands
#     providing extra arguments for the documentation). The
#     documentation commands could reuse there names/arguments
#     etc. directly from the method definition by issuing these
#     commands inside the method definition methods.
#
# One could provide lint-like features to signal, whether the
# documentation is in sync with actually defined methods (when these
# are available).
#
# @require nx
# @version 0.1
 
package provide nx::doc 0.1
package require nx

namespace eval ::nx::doc {
  namespace import -force ::nx::*
  
  # @command ::nx::doc::@
  #
  # The helper proc "@" is a conveniant way for creating new
  # documentation objects with less syntactic overhead.
  #
  # @param class Request an instance of a particular entity class (e.g., ...)
  # @param name What is the entity name (e.g., nx::doc for a package)
  # @param args A vector of arbitrary arguments, provided to the entity when being constructed
  # @return The identifier of the newly created entity object
  
  # @subcommand ::nx::doc::@#foo
  #
  # This is the first subcommand foo of "@"
  # {{{
  # set do 1;
  # }}}
  #
  # @param -param1 do it
  # @param param2 do it a second time
  # @return Gives you a "foo" object

  # @subcommand ::nx::doc::@#bar
  #
  # This is the second subcommand bar of "@"
  #
  # @param -param1 do it
  # @param param2 do it a second time
  # @return Gives you a "bar" object

  proc @ {class name args} {$class new -name $name {*}$args}
 

  # @command ::nx::doc::sorted
  #
  # This proc is used to sort instances by values of a specified
  # attribute. {{{ set 
  # code 1; puts stderr $code; puts stderr [info script]; set l \{x\}
  # }}} Und nun gehen wir in eine zweite Zeile ... und fügen einen Link ein (e.g., {{@object ::nx::doc::@object}})
  #
  # ... um nach einem Zeilenbruch weiterzumachen
  # {{{
  #	\# Some comment
  #	set instances [list [Object new] [Object new]]
  #	::nx::doc::sorted $instances; set l {{{x}}}; # Some comment
  # {{{ }}} 
  #	set instances [list [Object new] [Object new]]
  #	::nx::doc::sorted $instances
  # }}}
  # Here it goes wider ...
    # {{{
  #	set instances [list [Object new] [Object new]]
  #	::nx::doc::sorted $instances
  # }}}
  #
  # @param instances Points to a list of entity instances to sort e.g. {{@object ::nx::doc::@object}}
  # @param sortedBy Indicates the attribte name whose values the sorting will be based on
  # @return A list of sorted documentation entity instances {{{instances of @object}}}
  proc sorted {instances sortedBy} {
    set order [list]
    foreach v $instances {lappend order [list $v [$v eval [list set :$sortedBy]]]}
    set result [list]
    foreach pair [lsort -index 1 $order] {lappend result [lindex $pair 0]}
    return $result
  }

  # @method ::nx::doc::ExceptionClass#behind?
  # 
  # This helper method can be used to decide whether a message
  # caught in error propagation qualifies as a valid exception
  # object.
  #
  # @param error_msg Stands for the intercepted string which assumingly represents an exception object identifier
  # @return 0 or 1
  Class create ExceptionClass -superclass Class {
    # A meta-class which defines common behaviour for exceptions
    # types, used to indicate particular events when processing
    # comment blocks.
 
    :method behind? {error_msg} {
      return [expr {[::nsf::is $error_msg object] && \
			[::nsf::is $error_msg type [current]]}]
    }
    
    # @method thrown_by?
    # 
    # This helper method realises a special-purpose catch variant to
    # safely evaluate scripts which are expected to produce exception
    # objects
    #
    # @return 1 iff an exception object is caught, 0 if the script did
    # not blow or it returned an error message not pointing to an
    # exception object
    :method thrown_by? {script} {
      if {[uplevel 1 [list ::catch $script msg]]} {
	return [:behind? [uplevel 1 [list set msg]]]
      }
      return 0
    }

  }

  ExceptionClass create Exception {
    # The base class for exception objects
    #
    # @param message An explanatory message meant for the developer
    :attribute message:required
    # @param stack_trace Contains the stack trace as saved at the time of throwing the exception object
    :attribute stack_trace 
    
    # @method throw
    #
    # The method makes sure that an Exception object is propagated
    # through the Tcl ::error mechanism, starting from the call site's
    # scope
    :method throw {} {
      if {![info exists :stack_trace] && [info exists ::errorInfo]} {
	:stack_trace $::errorInfo
      }
      #
      # uplevel: throw at the call site
      #
      uplevel 1 [list ::error [current]]
    }
  }
  
  ExceptionClass create StyleViolation -superclass Exception {
    # This exception indicates from within the parsing machinery that
    # a comment block was malformed (according to the rules layed out
    # by the statechart-like parsing specification.
  }
  ExceptionClass create InvalidTag -superclass Exception {
    # This exception is thrown upon situations that invalid tags are
    # used at various levels of entity/part nesting. This usually
    # hints at typos in tag labels or the misuse of tags in certain
    # contexts.
  }
  ExceptionClass create MissingPartofEntity -superclass Exception {
    # This exception occurs when parts are defined without providing
    # an owning (i.e., partof) entity. This might be caused by
    # failures in resolving this context.
  }


  Class create EntityClass -superclass Class {
    # A meta-class for named documenation entities. It sets some
    # shared properties (e.g., generation rules for tag names based on
    # entity class names, ...). Most importantly, it provides the
    # basic name-generating mechanisms for documentation entities
    # based on properties such as entity name, root namespace, etc.
    #
    # @param tag Defaults to the tag label to be used in comment tags. It may vary from the auto-generated default!
    # @param root_namespace You may choose your own root-level namespace hosting the namespace hierarchy of entity objects

    :attribute {tag {[string trimleft [string tolower [namespace tail [current]]] @]}}
    :attribute {root_namespace "::nx::doc::entities"}

    :attribute owned_part_attributes:object,type=::nx::doc::PartAttribute,multivalued {
      set :incremental 1
    }

    namespace eval ::nx::doc::entities {}

    :method get_fully_qualified_name {name} {
      if {![string match "::*" $name]} {
	error "You need to provide a fully-qualified (absolute) entity name for '$name'."
      }
      return $name
    }
    
    # @method id 
    #
    # A basic generator for the characteristic ideas, based on the
    # root_namespace, the tag label, and the fully qualified name of
    # the documented entity
    #
    # @param name The name of the documented entity
    # @return An identifier string, e.g., {{{ ::nx::doc::entities::object::ns1::Foo }}}
    # @see tag
    # @see root_namespace
    :method id {name} {
      set subns [string trimleft [namespace tail [current]] @]
      #return [:root_namespace]::${subns}::[string trimleft $name :]
      puts stderr "[current callingproc] -> [:root_namespace]::${subns}[:get_fully_qualified_name $name]"
      return "[:root_namespace]::${subns}[:get_fully_qualified_name $name]"
    }

    :method new {-name:required args} {
      # A refined frontend for object construction/resolution which
      # provides for generating an explicit name, according to the
      # rules specific to the entity type.
      #
      # @param name The of the documented entity
      # @return The identifier of the newly generated or resolved entity object
      set fq_name [:get_fully_qualified_name $name]
      :createOrConfigure [:id $name] -name $fq_name {*}$args
    }
    
    :method createOrConfigure {id args} {
      puts stderr "createOrConfigure id $id"
      # This method handles verifies whether an entity object based on
      # the given id exists. If so, it returns the resolved name. If
      # not, it provides for generating an object with the precomputed
      # id for the first time!
      #
      # @param id The identifier string generated beforehand
      # @return The identifier of the newly generated or resolved entity object
      # @see {{@method id}}
      namespace eval $id {}
      if {[::nsf::objectproperty $id object]} {
	$id configure {*}$args
      } else {
	:create $id {*}$args
      }
      return $id
    }

    # @method get_unqualified_name
    #
    # @param qualified_name The fully qualified name (i.e., including the root namespace)
    :method get_unqualified_name {qualified_name} {
      # TODO: danger, tcl-commands in comments
      # similar to \[namespace tail], but the "tail" might be an object with a namespace
      return [string trimleft [string map [list [:root_namespace] ""] $qualified_name] ":"]
    }
  }

  Class create PartClass -superclass EntityClass {
    :method id {partof_object scope name} {
      # ::Foo class foo
      set subns [string trimleft [namespace tail [current]] @]
      set partof_name [string trimleft $partof_object :]
      #      puts stderr "ID -> [join [list [:root_namespace] $subns $partof_name $scope $name] ::]"
      return [join [list [:root_namespace] $subns $partof_name $scope $name] ::]
    }
    :method new {	       
      -part_attribute 
      {-partof:substdefault {[[MissingPartofEntity new \
				   -message [subst {
				     Parts of type '[namespace tail [current]]'
				     require a partof entity to be set
				   }]] throw]}}
      -name 
      args
    } {
      puts stderr "+++ PART [current args]"
      :createOrConfigure [:id [:get_fully_qualified_name [$partof name]] [$part_attribute scope] $name] {*}[current args]
    }
  }
  
  # @object ::nx::doc::PartAttribute
  #
  # This special-purpose Attribute variant realises (1) a cumulative
  # value management and (2) support for distinguishing between
  # literal parts (e.g., @author, @see) and object parts (e.g.,
  # \@param).
  #
  # The cumulative value management adds the append() operation which
  # translates into an add(...,end) operation. PartAttribute slots
  # default to append() as their default setter operation. To draw a
  # line between object and literal parts, PartAttribute slots either
  # refer to a part_class (a subclass of Part) or they do not. If a
  # part_class is given, the values will be transformed accordingly
  # before being pushed into the internal storage.
  
  ::nx::MetaSlot create PartAttribute -superclass ::nx::Attribute {

    # @param part_class
    # 
    # The attribute slot refers to a concrete subclass of Part which
    # describes the parts being managed by the attribute slot.
    :attribute part_class:optional,class
    :attribute scope

    # :forward owning_entity_class {% [[:info parent] info parent] }
    :method init args {
      :defaultmethods [list get append]
      :multivalued true
      set :incremental true
      # TODO: setting a default value leads to erratic behaviour;
      # needs to be verified -> @author returns ""
      # :default ""
      if {![info exists :scope]} {
	set :scope class
	regexp -- {@(.*)-.*} [namespace tail [current]] _ :scope
      }
      next
      # :owning_entity_class owned_part_attributes add [current]
    }
    
    :method require_part {domain prop value} {
      if {[info exists :part_class]} {
	if {[::nsf::is $value object] && \
		[::nsf::is $value type ${:part_class}]} {
	  return $value
	}
	return  [${:part_class} new \
		     -name [lindex $value 0] \
		     -partof $domain \
		     -part_attribute [current] \
		     -@doc [lrange $value 1 end]]
      }
      return $value
    }
    :method append {domain prop value} {
      :add $domain $prop $value end
    }
    :method assign {domain prop value} {
      set parts [list]
      foreach v $value {
	lappend parts [:require_part $domain $prop $v]
      }
      next $domain $prop $parts
    }
    :method add {domain prop value {pos 0}} {
      set p [:require_part $domain $prop $value]
      if {![$domain eval [list info exists :$prop]] || $p ni [$domain $prop]} {
	next $domain $prop $p $pos
      }
      return $p
    }
    :method delete {domain prop value} {
      next $domain $prop [:require_part $prop $value]
    }
  }
  


  Class create Entity {
    #
    # Entity is the base class for the documentation classes
    #

    # @param name
    #
    # gives you the name (i.e., the Nx object identifier) of the documented entity
    :attribute name:required
    # every Entity must be created with a "@doc" value and can have
    # an optional initcmd 
    :method objectparameter args {next {@doc:optional __initcmd:initcmd,optional}}

    :attribute @doc:multivalued {set :incremental 1}
    :attribute @see -slotclass ::nx::doc::PartAttribute
    :attribute @properties -slotclass ::nx::doc::PartAttribute

    :method has_property {prop} {
       if {![info exists :@properties]} {return 0}
	expr {$prop in ${:@properties}}
    }

    :method owned_parts {} {
      set slots [:info slotobjects]
      set r [dict create]
      # puts stderr SLOTS=$slots
      foreach s $slots {
	if {![$s info is type ::nx::doc::PartAttribute] || ![$s eval {info exists :part_class}]} continue;
	set accessor [$s name]
	# puts stderr "PROCESSING ACCESSOR $accessor, [info exists :$accessor]"
	if {[info exists :$accessor]} {
	  dict set r $accessor [sorted [:$accessor] name]
	}
      }
      return $r
    }

    # @method _doc
    #
    # The method _doc can be use to obtain the value of the documentation
    # from another doc entity. This should avoid redundant documentation pieces.
    :method _doc {doc use what value} {
      if {$@doc ne ""} {return $doc}
      if {$use ne ""} {
	foreach thing {@command @object} {
	  set docobj [$thing id $use]
	  if {[::nsf::objectproperty $docobj object]} break
	}
	if {[::nsf::objectproperty $docobj object]} {
	  if {![$docobj eval [list info exists :$what]]} {error "no attribute $what in $docobj"}
	  set names [list]
	  foreach v [$docobj $what] {
	    if {[$v name] eq $value} {return [$v @doc]}
	    lappend names [$v name]
	  }
	  error "can't use $use, no $what with name $value in $docobj (available: $names)"
	} else {
	  error "can't use $use, no documentation object $docobj"
	}
      }
    }

    # @method text
    #
    # text is used to access the content of doc of an Entity, and
    # performs substitution on it.  The substitution is not essential,
    # but looks for now convenient.
    #
    :method text {-as_list:switch} {
      if {[info exists :@doc] && ${:@doc} ne ""} {
	set doc ${:@doc}
	set non_empty_elements [lsearch -all -not -exact $doc ""]
	set doc [lrange $doc [lindex $non_empty_elements 0] [lindex $non_empty_elements end]]
	if {$as_list} {
	  return $doc
	} else {
	  return [subst [join $doc " "]]
	}
      }
    }

    :method filename {} {
      return [[:info class] tag]_[string trimleft [string map {:: __} ${:name}] "_"]
    }
  }


  Class create ContainerEntity -superclass Entity {
    
    Class create [current]::Resolvable {
      :object attribute container:object,type=[:info parent]
      :method get_fully_qualified_name {name} {
	set container [[current class] container]
	if {![string match "::*" $name]} {
	  # puts -nonewline stderr "--- EXPANDING name $name"
	  set name [$container namespace]::$name 
	  # puts stderr " to name $name"
	}
	next $name
      }
    }

    Class create [current]::Containable {
      # TODO: check the interaction of required, per-object attribute and ::nsf::assertion
      #:object attribute container:object,type=[:info parent],required
      :object attribute container:object,type=[:info parent]
      :method init args {
	#
	# Note: preserve the container currently set at this callstack
	# level.  [next] will cause the container to change if another
	# container entity is initialised in the following!
	#
	if {[[current class] eval {info exists :container}]} {
	  set container [[current class] container]
	  next
	  puts stderr "--- entity [current] starts living, register with $container"
	  $container register [current]
	} else {
	  next
	}
      }
    }
    # Note: The default "" corresponds to the top-level namespace "::"!
    :attribute {namespace ""}

    :attribute @class -slotclass ::nx::doc::PartAttribute {
      set :part_class @class
    }
    :attribute @object -slotclass ::nx::doc::PartAttribute {
      set :part_class @object
    }
    :attribute @command -slotclass ::nx::doc::PartAttribute {
      set :part_class @command
    }

    # :attribute @class:object,type=::nx::doc::@class,multivalued {
    #   set :incremental 1
    # }

    # :attribute @object:object,type=::nx::doc::@object,multivalued {
    #   set :incremental 1
    # }

    # :attribute @command:object,type=::nx::doc::@command,multivalued {
    #   set :incremental 1
    # }

    :method init {} {
      next
      puts stderr "APPLYING Resolvable container [current]"
      EntityClass mixin add [current class]::Resolvable
      [current class]::Resolvable container [current]
      puts stderr "APPLYING Containable container [current]"
      # Entity mixin add [current class]::Containable
      [current class]::Containable container [current]
    }

    :method register {containable:object,type=::nx::doc::Entity} {
      set tag [[$containable info class] tag]
      if {[:info callable methods -application "@$tag"] ne ""} {
	puts stderr "REGISTERING: tag $tag containable $containable on [current]"
	:@$tag $containable
      }
    }
  }

  EntityClass create @project -superclass ContainerEntity {
    :attribute url
    :attribute license
    :attribute creationdate
    :attribute {version ""}

    :attribute @package -slotclass ::nx::doc::PartAttribute {
      set :part_class @package
    }
  }

  #
  # Now, define some kinds of documentation entities. The toplevel
  # docEntities are named objects in the ::nx::doc::entities namespace
  # to ease access to it.
  #
  # For now, we define here the following toplevel docEntities:
  #
  # - @package 
  # - @command
  # - @object
  # - ...
  #
  # These can contain multiple parts.
  #  - @method
  #  - @param
  #  - ...
  #

  EntityClass create @package -superclass ContainerEntity -mixin ContainerEntity::Containable {
    :attribute @require -slotclass ::nx::doc::PartAttribute
    :attribute @version -slotclass ::nx::doc::PartAttribute
  }

  EntityClass create @command -superclass Entity -mixin ContainerEntity::Containable {
    :attribute @param -slotclass ::nx::doc::PartAttribute {
      set :part_class @param
    }
    :attribute @return -slotclass ::nx::doc::PartAttribute {
      :method require_part {domain prop value} {
	set value [expr {![string match ":*" $value] ? "__out__: $value": "__out__$value"}]
	next $domain $prop $value
	#next $domain $prop "__out__ $value"
      }
      set :part_class @param
    }
    :attribute @subcommand -slotclass ::nx::doc::PartAttribute {
      set :part_class @subcommand
    }
    :method parameters {} {
      set params [list]
      if {[info exists :@param]} {
	foreach p [:@param] {
	  set value [$p name]
	    if {[$p eval {info exists :default}] || [$p name] eq "args" } {
	    set value "?[$p name]?"
	  }
	  lappend params $value
	}
      }
      return $params
    }
  }
  
  EntityClass create @object \
      -superclass Entity \
      -mixin ContainerEntity::Containable {
	:attribute @author -slotclass ::nx::doc::PartAttribute 

	:forward @method %self @object-method
	:attribute @object-method -slotclass ::nx::doc::PartAttribute {
	  set :part_class @method
	}

	:forward @param %self @object-param
	:attribute @object-param -slotclass ::nx::doc::PartAttribute {
	  set :part_class @param
	}

	:method undocumented {} {
	  # TODO: for object methods and class methods
	  if {![::nsf::objectproperty ${:name} object]} {return ""}
	  foreach m [${:name} info methods] {set available_method($m) 1}
	  set methods ${:@method}
	  if {[info exists :@param]} {set methods [concat ${:@method} ${:@param}]}
	  foreach m $methods {
	    set mn [namespace tail $m]
	    if {[info exists available_method($mn)]} {unset available_method($mn)}
	  }
	  return [lsort [array names available_method]]
	}
      }

  EntityClass create @class \
      -superclass @object \
      -mixin ContainerEntity::Containable {
	:attribute @superclass -slotclass ::nx::doc::PartAttribute
	
	:forward @param %self @class-param
	:attribute @class-param -slotclass ::nx::doc::PartAttribute {
	  set :part_class @param
	}
	
	:forward @method %self @class-method
	:attribute @class-method -slotclass ::nx::doc::PartAttribute {
	  set :part_class @method
	  :method require_part {domain prop value} {
	    # TODO: verify whether these scoping checks are sufficient
	    # and/or generalisable: For instance, is the scope
	    # requested (from the part_attribute) applicable to the
	    # partof object, which is the object behind [$domain name]?
	    if {[info exists :scope] && 
		![::nsf::objectproperty [$domain name] ${:scope}]} {
	      error "The object '[$domain name]' does not qualify as '[$part_attribute scope]'"
	    }
	    next
	  }
	}
	
	:method inherited {member} {
	  if {[${:name} info is class]} {
	    set inherited [dict create]
	    foreach c [lreverse [${:name} info heritage]] {
	      set entity [[::nsf::current class] id $c]
	      if {![::nsf::is $entity object]} continue
	      if {[$entity eval [list info exists :${member}]]} {
		dict set inherited $entity [$entity $member]
	      }
	    }
	    return $inherited
	  }
	}
      }
  
  
  # @object ::nx::doc::Part
  #
  # A Part is a part of a documentation entity, defined by a
  # separate object. Every Part is associated to another
  # documentation entity and is identified by a name.
  #
  Class create Part -superclass Entity {
    
    #:method objectparameter args {next {doc -use}}
    :attribute partof:required
    :attribute use
    :attribute part_attribute
  }

  # @object ::nx::doc::@method
  #
  # "@method" is a named entity, which is part of some other
  # docEntity (a class or an object). We might be able to use the
  # "use" parameter for registered aliases to be able to refer to the 
  # documentation of the original method.
  #
  PartClass create @method \
      -superclass Part {
	:attribute {@modifier public} -slotclass ::nx::doc::PartAttribute
	:attribute @param -slotclass ::nx::doc::PartAttribute {
	  set :part_class @param
	}
	:attribute @return -slotclass ::nx::doc::PartAttribute {
	  
	  #
	  # TODO: @return spec fragments should be nameless,
	  # conceptually. They represent "out" parameters with each
	  # @method being allowed to have one only. For now, we fix
	  # this by injecting a dummy name "__out__" which should not
	  # be displayed. I shall fix this later and refactor it to a
	  # shared place between @method and @command.
	  #
	  :method require_part {domain prop value} {
	    set value [expr {![string match ":*" $value] ? "__out__: $value": "__out__$value"}]
	    next $domain $prop $value
	  }
	  set :part_class @param
	}
	:method parameters {} {
	  set params [list]
	  if {[info exists :@param]} {
	    foreach p [:@param] {
	      set value [$p name]
	      if {[$p eval {info exists :default}] || [$p name] eq "args" } {
		set value "?[$p name]?"
	      }
	      lappend params $value
	    }
	  }
	  if {1} {
	    # documentaion quality check: is documentation in sync with implementation?
	    # TODO: make me conditional, MARKUP should be in templates
	    set object [${:partof} name] 
	    if {[::nsf::objectproperty $object object]} {
	      if {[$object info methods ${:name}] ne ""} {
		set actualParams ""
		if {[$object info method type ${:name}] eq "forward"} {
		  set cmd ""
		  foreach w [lrange [$object info method definition ${:name}] 2 end] {
		    if {[string match ::* $w]} {
		      set cmd $w
		      break
		    }
		  }
		    if {$cmd ne "" && [string match ::nsf::* $cmd]} {
		    # TODO: we assume here, the cmd is a primitive
		    # command and we intend only to handle cases from
		    # predefined or xotcl2. Make sure this is working
		    # reasonable for other cases, such as forwards to 
		    # other objects, as well
		    if {![catch {set actualParams [::nx::Object info method parameter $cmd]}]} {
		      # drop usual object
		      set actualParams [lrange $actualParams 1 end]
		      # drop per object ; TODO: always?
                      if {[lindex $actualParams 0] eq "-per-object"} {
			set actualParams [lrange $actualParams 1 end]
			set syntax [lrange [::nx::Object info method parametersyntax $cmd] 2 end]
		      } else {
			set syntax [lrange [::nx::Object info method parametersyntax $cmd] 1 end]
		      }
		    }
		  }
		  set comment "<span style='color: orange'>Defined as a forwarder, can't check</span>"
		  #set handle ::nsf::signature($object-class-${:name})
		  #if {[info exists $handle]} {append comment <br>[set $handle]}
		} else {
		  set actualParams [$object info method parameter ${:name}]
		  set syntax [$object info method parametersyntax ${:name}]
		}
		if {$actualParams eq $params} {
		  set comment "<span style='color: green'>Perfect match</span>"
		} else {
		  set comment "<span style='color: red'>actual parameter: $actualParams</span>"
		}
		append comment "<br>Syntax: <i>obj</i> <strong>${:name}</strong> $syntax"
	      } else {
		set comment "<span style='color: red'>Method '${:name}' not defined on $object</span>"
	      }
	    } else {
	      set comment "<span style='color: red'>cannot check object, probably not instantiated</span>"
	    }
	    #puts stderr "XXXX [current] ${:name} is part of ${:partof} // [${:partof} name]"
	    return [concat $params <br>$comment]
	    } 
	    return $params
	}
	:method process {
	  {-initial_section:optional "context"} 
	  comment_block
	} {
	  next \
	      -initial_section $initial_section \
	      -entity [current] $comment_block
	}

      }; # @method
  
  PartClass create @subcommand -superclass {Part @command}

  # @object ::nx::doc::@param
  #
  # The entity type "@param" represents the documentation unit
  # for several parameter types, e.g., object, method, and
  # command parameters.
  #
  # @superclass ::nx::doc::entities::object::nx::doc::Part
  PartClass create @param \
      -superclass Part {
	:attribute spec
	:attribute default
	  
	:object method id {partof name} {
	  # The method contains the parameter-specific name production rules.
	  #
	  # @param partof Refers to the entity object which contains this part 
	  # @param name Stores the name of the documented parameter
	  # @modifier protected

	  set partof_fragment [:get_unqualified_name ${partof}]
	  return [:root_namespace]::${:tag}::${partof_fragment}::${name}
	}
	
	# @object-method new
	#
	# The per-object method refinement indirects entity creation
	# to feed the necessary ingredients to the name generator
	#
	# @param -part_attribute 
	# @param -partof
	# @param -name
	# @param args
	:object method new {
		-part_attribute 
		{-partof:substdefault {[[MissingPartofEntity new \
					     -message [subst {
			Parts of type '[namespace tail [current]]'
			require a partof entity to be set
					     }]] throw]}}
		-name 
		args
	      } {
	  
	  lassign $name name def
	  set spec ""
	  regexp {^(.*):(.*)$} $name _ name spec
	  :createOrConfigure [:id $partof $name] \
	      -spec $spec \
	      -name $name \
	      -partof $partof \
	      {*}[expr {$def ne "" ? "-default $def" : ""}] \
	      -part_attribute $part_attribute {*}$args
	  
	}
      }

  namespace export CommentBlockParser @command @object @class @package @project @method \
      @param @ Exception StyleViolation InvalidTag \
      MissingPartofEntity ExceptionClass
}



namespace eval ::nx::doc {

  Class create TemplateData {
    # This mixin class realises a rudimentary templating language to
    # be used in nx::doc templates. It realises language expressions
    # to verify the existence of variables and simple loop constructs
    :method render {
      {-initscript ""}
      template 
      {entity:substdefault "[current]"}
    } {
      # Here, we assume the -nonleaf mode being active for {{{[eval]}}}.
      set tmplscript [list subst [[::nsf::current class] read_tmpl $template]]
      $entity eval [subst -nocommands {
	$initscript
	$tmplscript
      }]
      # $entity eval [list subst $template]
    }
    
    
    #
    # some instructions for a dwarfish, embedded templating language
    #
    :method let {var value} {
      uplevel 1 [list ::set $var [expr {[info exists value]?$value:""}]]
      return
    }
    :method for {var list body} { 
      set rendered ""
      ::foreach $var $list {
	uplevel 1 [list ::set $var [set $var]]
	append rendered [uplevel 1 [list subst $body]]
      }
      return $rendered
    }
    :method ?var {varname args} {
      uplevel 1 [list :? -ops [list [::nsf::current method] -] \
		     "\[info exists $varname\]" {*}$args]
    } 
    :method ? {
      {-ops {? -}}
      expr 
      then
      next:optional 
      args
    } {
      if {[info exists next] && $next ni $ops} {
	return -code error "Invalid control operator '$next', we expect one of $ops"
      }
      if {[uplevel 1 [list expr $expr]]} {
	return [uplevel 1 [list subst $then]]
      } elseif {[info exists next]} {
	if {$next eq "-"} {
	  set args [lassign $args next_then]
	  if {$next_then eq ""} {
	    return -code error "A then script is missing for '-'"
	  }
	  if {$args ne ""} {
	    return -code error "Too many arguments: $args"
	  }
	  return [uplevel 1 [list subst $next_then]]
	}
	return [:$next {*}$args]
      }
    }
    
    :method include {template} {
      uplevel 1 [list subst [[::nsf::current class] read_tmpl $template]]
    }

    #
    # TODO: This should turn into a hook, the output
    # specificities should move in a refinement of TemplateData, e.g.,
    # DefaultHtmlTemplateData or the like.
    #
    :method fit {str max {placeholder "..."}} {
      if {[llength [split $str ""]] < $max} {
	return $str;
      }
      set redux [llength [split $placeholder ""]]
      set margin [expr {($max-$redux)/2}]
      return "[string range $str 0 [expr {$margin-1}]]$placeholder[string range $str end-[expr {$margin+1}] end]"
    }

    :method list_structural_features {} {
      set entry {{"access": "$access", "host": "$host", "name": "$name", "url": "$url", "type": "$type"}}
      set entries [list]
      if {[:info is type ::nx::doc::@package]} {
	set features [list @object @command]
	foreach feature $features {
	  set instances [sorted [$feature info instances] name]
	  foreach inst $instances {
	    set access ""
	    set host [:name]
	    set name [$inst name]
	    set url  "[$inst filename].html"
	    set type [$feature tag]
	    lappend entries [subst $entry]
	  }
	}
      } elseif {[:info is type ::nx::doc::@object]} {
	# TODO: fix support for @object-method!
	set features [list @method @param]
	foreach feature $features {
	  if {[info exists :$feature]} {
	    set instances [sorted [:$feature] name]
	    foreach inst $instances {
	      set access [expr {[info exists :@modifier]?[:@modifier]:""}]
	      set host [:name]
	      set name [$inst name]
	      set url  "[:filename].html#[$feature tag]_[$inst name]"
	      set type [$feature tag]
	      lappend entries [subst $entry]
	    }
	  }
	}
      } elseif {[:info is type ::nx::doc::@command]} {
	set features @subcommand
	foreach feature $features {
	  if {[info exists :$feature]} {
	    set instances [sorted [set :$feature] name]
	    foreach inst $instances {
	      set access ""
	      set host ${:name}
	      set name [$inst name]
	      set url  "[:filename].html#[$feature tag]_[$inst name]"
	      set type [$feature tag]
	      lappend entries [subst $entry]
	    }
	  }
	}
      }
      return "\[[join $entries ,\n]\]"
    }
  
    :method code {{-inline true} script} {
      return [expr {$inline?"<code>$script</code>":"<pre>$script</pre>"}]
    }

    :method link {entity_type args} {
      set id [$entity_type id {*}$args]
      if {![::nsf::is $id object]} return;
      set pof ""
      if {[$id info is type ::nx::doc::Part]} {
	set pof "[[$id partof] name]#"
	set filename [[$id partof] filename]
      } else {
	set filename [$id filename]
      }
      return "<a href=\"$filename.html#[$entity_type tag]_[$id name]\">$pof[$id name]</a>"
    }

    :method text {} {
      # Provide \n replacements for empty lines according to the
      # rendering frontend (e.g., in HTML -> <br/>) ...
      if {[info exists :@doc]} {
	set doc [next -as_list]
	foreach idx [lsearch -all -exact $doc ""] {
	  lset doc $idx "<br/><br/>"
	}
	return [subst [join $doc " "]]
      }
    }


    
    #
    #
    #
 
    :object method find_asset_path {{-subdir library/lib/doc-assets}} {
      # This helper tries to identify the file system path of the
      # asset ressources.
      #
      # @param -subdir Denotes the name of the sub-directory to look for
      foreach dir $::auto_path {
	set assets [file normalize [file join $dir $subdir]]
	if {[file exists $assets]} {
	  return $assets
	}
      }
    }

    :object method read_tmpl {path} {
      if {[file pathtype $path] ne "absolute"} {
	set assetdir [:find_asset_path]
	set tmpl [file join $assetdir $path]
      } else {
	set tmpl [file normalize $path]
      }
      if {![file exists $tmpl] || ![file isfile $tmpl]} {
	error "The template file '$path' was not found."
      }
      set fh [open $tmpl r]
      set content [read $fh]
      catch {close $fh}
      return $content
    }

  }
  
  #
  # Provide a simple HTML renderer. For now, we make our life simple
  # by defining for the different supported docEntities different methods.
  #
  # We could think about a java-doc style renderer...
  #
  
  Class create Renderer {
    :method render {} {
      :render=[namespace tail [:info class]]
    }
  }

  Class create HtmlRenderer -superclass Renderer {
    # render command pieces in the text
    :method tt {text} {return <@TT>$text</@TT>}


    :method render=@package {} {
      puts "<LI>[:tt ${:name}] <br>\n[:text]"
      set req [:@require]
      if {$req ne ""} {
	puts "   <UL>"
	foreach r $req {puts "    <LI>$r</LI>"}
	puts "   </UL>"
      }
      puts "</LI>\n"

    }

    #
    # render xotcl commands
    #
    :method render=@command {} {
      puts "<LI>[:tt ${:name}] <br>\n[:text]"
      # set variants [sorted [:variants] name]
      # if {$variants ne ""} {
      # 	puts "   <UL>"
      # 	foreach v $variants {puts "    <LI>[$v text]"}
      # 	puts "   </UL>"
      # }
      set params [:@param]
      if {$params ne ""} {
	puts "   <UL>"
	foreach v $params {puts "    <LI>[$v tt [$v name]] [$v text]"}
	puts "   </UL>"
      }
      puts "</LI>\n"
    }

    #
    # render next classes
    #
    :method render=@object {} {
      puts "<LI>[:tt ${:name}] <br>\n[:text]"
      if {[info exists :@method]} {
	set methods [sorted [:@method] name]
	if {$methods ne ""} {
	  puts "<br>Methods of ${:name}:\n   <UL>"
	  foreach m $methods {$v render}
	  puts "   </UL>"
	}
      }
      if {[info exists :@object-method]} {
	set methods [sorted [:@object-method] name]
	if {$methods ne ""} {
	  puts "<br>Object methods of ${:name}:\n   <UL>"
	  foreach m $methods {$v render}
	  puts "   </UL>"
	}
      }
      puts "</LI>\n"
    }

    #
    # render next methods
    #
    :method render=@method {} {
      puts "<LI>[:tt [:signature]] <br>\n[:text]"
      set params [:@param]
      if {$params ne ""} {
	puts "   <UL>"
	foreach v $params {puts "    <LI>[$v tt [$v name]] [$v text]"}
	puts "   </UL>"
      }
      if {${:returns} ne ""} {
	puts "   Returns: ${:@return}"
      }
      puts "\n"
    }

  }

}

#
# post processor for initcmds and method bodies
#
namespace eval ::nx {
  namespace import -force ::nx::doc::*
  ::nx::Object create doc  {

    :method log {msg} {
      puts stderr "[current]->[uplevel 1 [list ::nsf::current method]]: $msg"
    }

    # @method process
    #
    # There is a major distinction: Is the entity the comment block is
    # referring to given *extrinsically* (to the comment block) or
    # *intrinsically* (as a starting tag).
    # 
    # a. extrinsic: 'thing' is a valid class or object name
    # b. intrinsic: 'thing' is a arbitrary string block describing 
    # a script.
    # 
    :method process {{-noeval false} thing args} {
      # 1) in-situ processing: a class object
      if {[::nsf::objectproperty $thing object]} {
	if {[$thing eval {info exists :__initcmd}]} {
	  
          :analyze_initcmd [expr {[::nsf::objectproperty $thing class]?"@class":"@object"}] $thing [$thing eval {set :__initcmd}]
        }
      } elseif {![catch {package present $thing} msg]} {
	# For tcl packages, we assume that the package is sourceable
	# in the current interpreter.
	set i [interp create]
	set cmd [subst -nocommands {
	  package req nx::doc
	  namespace import -force ::nx::*; 
	  ::nx::Class create SourcingTracker {
	    :method create args {
	      set obj [next];
	      #[::nsf::current class] eval {
	      # if {![info exists :scripts([info script])]} {
	      #dict create :scripts
	      #dict set :scripts [info script] objects 
	      #	}
	      #}
	      #puts stderr "dict lappend :scripts([info script]) objects [current]"
	      [::nsf::current class] eval [list dict set :scripts [info script] objects \$obj _]
	      return \$obj
	    }
	  }
	  ::nx::Object mixin add SourcingTracker
	  package forget $thing
	  package req $thing
	  ::nx::Object mixin delete SourcingTracker
	  #puts stderr sourced_scripts=[SourcingTracker eval {dict keys \${:scripts}}]
	  dict for {script entities} [SourcingTracker eval {set :scripts}] {
	    doc process \$script \$entities
	  }
	  
	}]
	interp eval $i $cmd
	return $i
      } elseif {[file isfile $thing]} {
	# 3) alien script file
	if {[file readable $thing]} {
	  set fh [open $thing r]
	  if {[catch {set script [read $fh]} msg]} {
	    catch {close $fh}
	    :log "error reading the file '$thing', i.e.: '$msg'"
	  }
	  close $fh
	  doc analyze -noeval $noeval $script {*}$args
	  puts stderr SCRIPT=$thing--[file readable $thing]-ANALYZED-[string length $script]bytes
	  #doc process -noeval $noeval $script {*}$args
	} else {
	  :log "file '$thing' not readable"
	}
      } else {
	# 4) we assume a string block, e.g., to be fed into eval
	set i [interp create]
	set cmd [subst {
	  package req nx::doc
	  namespace import -force ::nx::doc::*
	  doc analyze -noeval $noeval [list $thing]
	}]
	interp eval $i $cmd
	#interp delete $i
	return $i
      }
    }

    :method analyze {{-noeval false} script {additions ""}} {
      # NOTE: This method is to be executed in a child/ slave
      # interpreter.
      if {!$noeval} {
	uplevel #0 [list namespace import -force ::nx::doc::*]
	set pre_commands [:list_commands]
	uplevel #0 [list eval $script]
	set post_commands [:list_commands]
	if {$additions eq ""} {
	  set additions [dict keys [dict remove [dict create {*}"[join $post_commands " _ "] _"] {*}$pre_commands]]
	} else {
	  set additions [dict keys [dict get $additions objects]]
	}
	# puts stderr ADDITIONS=$additions
      }
      set blocks [:comment_blocks $script]
      # :log "blocks: '$blocks'"
      # 1) eval the script in a dedicated interp; provide for
      # recording script-specific object additions.
      # set failed_blocks [list]
      foreach {line_offset block} $blocks {
	# 2) process the comment blocks, however, fail gracefully here
	# (most blocks, especially in initcmd and method blocks, are
	# not qualified, so they are set to fail. however, record the
	# failing ones for the time being
	if {[catch {::nx::doc::CommentBlockParser process $block} msg]} {
	  if {![InvalidTag behind? $msg] && ![StyleViolation behind? $msg] && ![MissingPartofEntity behind? $msg]} {
	    if {[Exception behind? $msg]} {
	      error [$msg info class]->[$msg message]
	    }
	    error $msg
	  }
	}
      }
      # 3) process the recorded object additions, i.e., the stored
      # initcmds and method bodies.
      foreach addition $additions {
	# TODO: for now, we skip over pure Tcl commands and procs
	if {![::nsf::is $addition object]} continue;
	set kind [expr {[::nsf::is $addition class]?"@class":"@object"}]
	#puts stderr "ADDITION :process [namespace origin $addition]"
	if {[$addition eval {info exists :__initcmd}]} {
	  :analyze_initcmd $kind $addition [$addition eval {set :__initcmd}]
	}

	# TODO: Note, the CommentBlockParser should operate on the
	# level of a single block, not entire initcmd and method body
	# scripts. The process=@object ressembles some ::nx::doc
	# methods, so relocated and call the parser from within.
	set entity [@ $kind $addition]
	:process=$kind $entity
      }
    }

    :method list_commands {{parent ""}} {
      set cmds [info commands ${parent}::*]
      foreach nsp [namespace children $parent] {
	lappend cmds {*}[:list_commands ${nsp}]
      }
      return $cmds
    }

    :method analyze_line {line} {
      set regex {^[\s#]*#+(.*)$}
      if {[regexp -- $regex $line --> comment]} {
	return [list 1 [string trimright $comment]]
      } else {
	return [list 0 $line]
      }
    }
    
    :method comment_blocks {script} {
      set lines [split $script \n]
      set comment_blocks [list]
      set was_comment 0
      
      set spec {
	0,1	{
	  set line_offset $line_counter; 
	  set comment_block [list]; 
	  # Note, we use [split] here to avoid stumbling over
	  # uncommented script blocks which contain pairs of curly
	  # braces which appear scattered over several physical lines
	  # of code. This avoids "unmatched open brace" failures when
	  # feeding each physical line to a list command (later, in
	  # the parsing machinery)
	  lappend comment_block $text}
	1,0	{lappend comment_blocks $line_offset $comment_block}
	1,1	{lappend comment_block $text}
	0,0	{}
      }
      array set do $spec 
      set line_counter -1
      foreach line $lines {
	incr line_counter
	# foreach {is_comment text} [:analyze_line $line] break;
	lassign [:analyze_line $line] is_comment text;
	eval $do($was_comment,$is_comment)
	set was_comment $is_comment
      }
      return $comment_blocks
    }
	   
    :method analyze_initcmd {docKind name initcmd} {
      set first_block 1
      set failed_blocks [list]
      foreach {line_offset block} [:comment_blocks $initcmd] {
	set arguments [list]
	if {$first_block} {
	  set id [@ $docKind $name]
	  #
	  # Note: To distinguish between intial comments blocks
	  # in initcmds and method bodies which refer to the
	  # surrounding entity (e.g., the object or the method)
	  # we use the line_offset recorded by the
	  # comment_blocks() scanner. Later, we plan to use the
	  # line_offset to compute line pointers for error
	  # messages. Also, we can use the line offsets of each
	  # comment block to identify faulty comment blocks.
	  #
	  # A acceptance level of <= 1 means that a script
	  # block must contain the first line of this
	  # special-purpose comment block either in the very
	  # first or second script line.
	  #
	  if {$line_offset <= 1} {
	    lappend arguments -initial_section description
	    lappend arguments -entity $id
	  }
	  set first_block 0
	} else {
	  set initial_section context
	}
	lappend arguments $block
	# TODO: Filter for StyleViolations as >the only< valid case
	# for a continuation. Report other issues immediately. What
	# about InvalidTag?!
	# TODO: Passing $id as partof_entity appears unnecessary,
	# clean up the logic in CommentBlockParser->process()!!!
	#puts stderr "==== CommentBlockParser process -partof_entity $id {*}$arguments"
	if {[catch {CommentBlockParser process -partof_entity $id {*}$arguments} msg]} {
	  lappend failed_blocks $line_offset
	}
      }
      
    }; # analyze_initcmd method
    
    # TODO: how can I obtain some reuse here when later @class is
    # distinguished from @object (dispatch along the inheritance
    # hierarchy?)
    :method process=@class {entity} {
      set name [$entity name]
      foreach methodName [${name} info methods -methodtype scripted] {
	# TODO: should the comment_blocks parser relocated?
	set blocks [:comment_blocks [${name} info method \
					    body $methodName]]
	foreach {line_offset block} $blocks {
	  if {$line_offset > 1} break;	      
	  set id [$entity @class-method $methodName]
	  CommentBlockParser process \
	      -partof_entity $entity \
	      -initial_section description \
	      -entity $id \
	      $block
	}
	:process=@object $entity object
      }
    }
    
    :method process=@object {entity {scope ""}} {
      set name [$entity name]
      
      foreach methodName [${name} {*}$scope info methods\
			      -methodtype scripted] {
	
	set blocks [:comment_blocks [${name} {*}$scope info method \
					    body $methodName]]
	foreach {line_offset block} $blocks {
	  if {$line_offset > 1} break;
	  set id [$entity @object-method $methodName]
	  CommentBlockParser :process \
	      -partof_entity $name \
	      -initial_section description \
	      -entity $id \
	      $block
	}
      }
    }
    
    # activate the recoding of initcmds
    ::nsf::configure keepinitcmd true
    
  }
}
	  

#
# toplevel interface
#   ::nx::doc::make all
#   ::nx::doc::make doc
#
namespace eval ::nx::doc {

  Object create make  {
    
    :method all {{-verbose:switch} {-class ::nx::Class}} {
      foreach c [$class info instances -closure] {
	if {$verbose} {puts "postprocess $c"}
	::nx::doc::postprocessor process $c
      }
    }
    
    :method doc {
      {-renderer ::nx::doc::HtmlRenderer}
      {-outdir /tmp/}
    } {
      
      # register the HTML renderer for all docEntities.
      
      Entity mixin add $renderer

      puts "<h2>Tcl packages</h2>\n<UL>"
      foreach pkg [sorted [@package info instances] name] {
	$pkg render
      }

      
      puts "<h2>Primitive Next framework commands</h2>\n<UL>"
      foreach cmd [sorted [@command info instances] name] {
	$cmd render
      }
      puts "</UL>\n\n"
      
      puts "<h2>Next objects</h2>\n<UL>"
      foreach cmd [sorted [@object info instances] name] {
	$cmd render
      }
      puts "</UL>\n\n"
      
      Entity mixin delete $renderer
    }
   
    :method write {content path} {
      set fh [open $path w]
      puts $fh $content
      catch {close $fh}
    }

    :method doc {
      {-renderer ::nx::doc::HtmlRenderer}
      {-outdir /tmp/}
      {-tmpl entity.html.tmpl}
      {-project:object,type=::nx::doc::@project}
    } {
      Entity mixin add $renderer
      # GN: why the manual hack instead of "file extension"?  
      # SS: Because I use the name of the template file to denote the
      # file extension of the output file which is not *.tmpl, but
      # rather: *.html.tmpl -> *.html. [file extension] just returns
      # the trailing extension.
      set ext [lindex [split [file tail $tmpl] .] end-1]
      set top_level_entities [$project owned_parts]
      if {[dict exists $top_level_entities @package]} {
	foreach p [dict get $top_level_entities @package] {
	  foreach {entity_type pkg_entities} [$p owned_parts] {
	    dict lappend top_level_entities $entity_type {*}$pkg_entities
	  }
	}
      }
      puts stderr TOP_LEVEL_ENTITIES=$top_level_entities
      # set entities [concat [sorted [@package info instances] name] \
      # 			[sorted [@command info instances] name] \
      # 			[sorted [@object info instances] name]]
      set init [subst -nocommands {
	set project $project
	array set "" [list $top_level_entities]
      }]
      set project_path [file join $outdir [string trimleft [$project name] :]]
      if {![catch {file mkdir $project_path} msg]} {
	#	  puts stderr [list file copy -force -- [$renderer find_asset_path] $project_path/assets]
	set assets [lsearch -all -inline -glob -not [glob -directory [$renderer find_asset_path] *] *.tmpl]
	set target $project_path/assets
	file mkdir $target
	file copy -force -- {*}$assets $target
	set index [$project render -initscript $init $tmpl]
	# puts stderr "we have [llength $entities] documentation entities ($entities)"
	:write $index [file join $project_path "index.$ext"]
	set values [join [dict values $top_level_entities]]
	puts stderr "VALUES=$values"
	foreach e $values {
	  puts stderr "PROCESSING=$e render -initscript $init $tmpl"
	  set content [$e render -initscript $init $tmpl]
	  :write $content [file join $project_path "[$e filename].$ext"]
	  puts stderr "$e written to [file join $project_path [$e filename].$ext]"
	}
      }
            
      Entity mixin delete $renderer
    }
  }
  
  #
  # This is a mixin class which adds comment block parsing
  # capabilities to documentation entities (Entity, ...), once
  # identified.
  #
  # It acts as the event source external to the modal parser (i.e.,
  # the parsed entity). Expressing a modal behavioural design itself
  # (around the line queue of a comment block), it produces certain
  # events which are then signalled to the parsed entity.
  #
  Class create CommentBlockParser {

    :attribute processed_section  {
      set :incremental 1
      :method assign {domain prop value} {
	set current_entity [$domain current_entity]
	set scope [expr {[$current_entity info is class]?"object mixin":"mixin"}]
	if {[$domain eval [list info exists :$prop]] && [:get $domain $prop] in [$current_entity {*}$scope]} {
	  $current_entity {*}$scope delete [:get $domain $prop]
	}
	$current_entity {*}$scope add [next $domain $prop $value]
      }
    }
    :attribute current_entity:object

    :object method process {
	{-partof_entity ""}
	{-initial_section context}
	-entity
	block
      } {
      
      if {![info exists entity]} {
	set entity [Entity]
      }
      
      set parser_obj [:new -current_entity $entity -volatile]
      $parser_obj [current proc] \
	  -partof_entity $partof_entity \
	  -initial_section $initial_section \
	  $block
      return [$parser_obj current_entity]
    }
    
    :forward has_next expr {${:idx} < [llength ${:comment_block}]}
    :method dequeue {} {
      set r [lindex ${:comment_block} ${:idx}]
      incr :idx
      return $r
    }
    :forward rewind incr :idx -1
#    :forward fastforward set :idx {% expr {[llength ${:comment_block}] - 1} }
    :forward fastforward set :idx {% llength ${:comment_block}}


    #
    # everything below assumes that the current class is an active mixin
    # on an instance of an Entity subclass!
    #

    :method process {
      {-partof_entity ""}
      {-initial_section context}
      block
    } {

      set :comment_block $block
      set :idx 0

      :processed_section [$initial_section]

      # TODO: currently, default values are not initialised for
      # attribute slots defined in mixin classes; so do it manually
      # for the time being.
      ${:current_entity} current_comment_line_type ""

      ${:current_entity} block_parser [current]      
      ${:current_entity} eval [list set :partof_entity $partof_entity]
      
      set is_first_iteration 1
      set failure ""
      
      #
      # Note: Within the while-loop, two object variables constantly
      # change (as "wanted" side-effects): processed_section: reflects
      # the currently processed comment section; see event=next()
      # current_entity: reflects the currently documentation entity
      # (once resolved); see context->event=parse@tag()
      #
      while {[:has_next]} {
	set line [:dequeue]	
	if {$is_first_iteration} {
	  ${:current_entity} on_enter $line
	  set is_first_iteration 0
	}

	if {[catch {
	  set actions [${:current_entity} event=process $line]
	} failure]} {
	  puts stderr ERRORINFO=$::errorInfo
	  :fastforward
	} 
      }
      if {!$is_first_iteration} {
	${:current_entity} on_exit $line
      }
      
      if {$failure ne ""} {
	error $failure
      }
            
    }; # CommentBlockParser->process()
    
  }
  
  Class create CommentBlockParsingState -superclass Class {
    
    :attribute next_comment_section
    :attribute comment_line_transitions:required
    
  }
  
  Class create CommentSection {

    :attribute block_parser:object,type=::nx::doc::CommentBlockParser
    :attribute {current_comment_line_type ""}

    set :line_types {
      tag {regexp -- {^\s*@[^[:space:]@]+} $line}
      text {regexp -- {^\s*([^[:space:]@]+|@[[:space:]@]+)} $line}
      space {expr {$line eq {}}}
    }

    :method get_transition {src_line_type tgt_line} {
      set section [${:block_parser} processed_section]
      array set transitions [$section comment_line_transitions]
      # expected outcome
      # 1. new state -> becomes current_comment_line
      # 2. actions to be triggered from the transition
      
      foreach {line_type expression} [[current class] eval {set :line_types}] {
	set line $tgt_line
	if {[eval $expression]} {
	  set tgt_line_type $line_type
	  break
	}
      }

      if {![info exists tgt_line_type]} {
	error "Could not resolve the type of line '$line'"
      }

      if {![info exists transitions(${src_line_type}->${tgt_line_type})]} {
	set msg "Style violation in a [namespace tail [:info class]] section:\n"
	if {$src_line_type eq ""} {
	  append msg "Invalid first line ('${tgt_line_type}')"
	} else {
	  append msg "A ${src_line_type} line is followed by a ${tgt_line_type} line"
	}
	[StyleViolation new -message $msg] throw
      }
      
      return [list $tgt_line_type $transitions(${src_line_type}->${tgt_line_type})]
    }

    # the actual events to be signalled to and sensed within the
    # super-states and sub-states

    :method event=process {line} {
      lassign [:get_transition ${:current_comment_line_type} $line] \
	  :current_comment_line_type actions
      foreach action $actions {
	:event=$action $line
      }
    }

    :forward event=parse %self {% subst {parse@${:current_comment_line_type}}} 
    :method event=next {line} {
      set next_section [[${:block_parser} processed_section] next_comment_section]
      :on_exit $line
      
      ${:block_parser} rewind
      :current_comment_line_type ""
    
      ${:block_parser} processed_section [$next_section]
      :on_enter $line
    }


    set :markup_map(sub) { 
      "{{{" "\[:code \{" 
      "}}}" "\}\]"
      "{{" "\[:link " 
      "}}" "\]" 
    }
    
    set :markup_map(unescape) {
      "\\{" "{"
      "\\}" "}"
      "\\#" "#"
    }
    
    :method map {line set} {
      set line [string map [[::nsf::current class] eval [list set :markup_map($set)]] $line]
    }
    
    # realise the sub-state (a variant of METHOD-FOR-STATES) and their
    # specific event handling
    :method parse@tag {line} {
      set line [:map $line sub]
      set line [:map $line unescape]
      set line [split [string trimleft $line]]
      set tag [lindex $line 0]
      if {[:info callable methods -application $tag] eq ""} {
	[InvalidTag new -message [subst {
	  The tag '$tag' is not supported for the entity type
	  '[namespace tail [:info class]]'
	}]] throw
      }
      :$tag [lrange $line 1 end]
    }

    :method parse@text {line} {

      if {![info exists :is_code_block]} {
	set :is_code_block 0
      }

      if {[regsub -- {^\s*(\{\{\{)\s*$} $line "\[:code -inline false \{" line] || \
	      (${:is_code_block} && [regsub -- {^\s*(\}\}\})\s*$} $line "\}\]" line])} {
	set :is_code_block [expr {!${:is_code_block}}]
	append line \n
      } elseif {${:is_code_block}} {
	set line [:map $line unescape]
	append line \n
      } else {
	set line [:map $line sub]
	set line [:map $line unescape]
	set line [string trimleft $line]
      }
      #puts stderr "ADDLINE :@doc add $line end"
      :@doc add $line end
    }
    :method parse@space {line} {;}
    
    #
    # so far, we only need enter and exit handlers at the level of the
    # superstates: context, description, part
    #
    :method on_enter {line} {;}
    :method on_exit {line} {;}
  }

  # NOTE: add these transitions for supporting multiple text lines for
  # the context element
  #	tag->text	parse
  #	text->text	parse
  #	text->space	""
  
  
  CommentBlockParsingState create context -superclass CommentSection \
      -next_comment_section description \
      -comment_line_transitions {
	->tag		parse
	tag->space	""
	space->space	""
	space->text	next
	space->tag	next
      } {
	
	:method resolve_partof_entity {tag name} {
	  # a) unqualified: attr1
	  # b) qualified: Bar#attr1
	  if {[regexp -- {([^\s#]*)#([^\s#]*)} $name _ qualifier nq_name]} {
	    # TODO: Currently, I only foresee @object and @command as
	    # possible qualifiers; however, this should be fixed asap, as
	    # soon as the variety of entities has been decided upon!
	    foreach entity_type {@class @command @object} {
	      set partof_entity [$entity_type id $qualifier]
	      # TODO: Also, we expect the qualifier to resolve against an
	      # already existing entity object? Is this intended?
	      if {[::nsf::is $partof_entity object]} {
		return [list $nq_name $partof_entity]
	      }
	    }
	    return [list $nq_name ${:partof_entity}]
	  } else {      
	    return [list $name ${:partof_entity}]
	  }
	}

	# realise the parse events specific to the substates of description
	:method parse@tag {line} {
	  #
	  # When hitting this parsing step, we have an unresolved
	  # entity. The context section specifies the entity to create
	  # or to resolve for further processing.
	  #
	  set line [split [string trimleft $line]]
	  set args [lassign $line tag name]
	  lassign [:resolve_partof_entity $tag $name] nq_name partof_entity
	  if {$partof_entity ne ""} {
	    if {[$partof_entity info callable methods -application $tag] eq ""} {
	      [InvalidTag new -message [subst {
		The tag '$tag' is not supported for the entity type
		'[namespace tail [$partof_entity info class]]'
	      }]] throw
	    }
	    #	  puts stderr "1. $partof_entity $tag $nq_name {*}$args"
	    set current_entity [$partof_entity $tag $nq_name {*}$args]
	    
	  } else {
	    #
	    # TODO: @object-method raises some issues (at least when
	    # processed without a resolved context = its partof entity).
	    # It is not an entity type, because it merely is a "scoped"
	    # @method. It won't resolve then as a proper instance of
	    # EntityClass, hence we observe an InvalidTag exception. For
	    # now, we just ignore and bypass this issue by allowing
	    # InvalidTag exceptions in analyze()
	    #
	    set qualified_tag [namespace qualifiers [current]]::$tag
	    if {[EntityClass info instances -closure $qualified_tag] eq ""} {
	      [InvalidTag new -message [subst {
		The entity type '$tag' is not available
	      }]] throw 
	    }
	    set current_entity [$tag new -name $nq_name {*}$args]
	  }
	  #
	  # make sure that the current_entity has parser capabilities
	  # and the relevant state of the previous entity before the
	  # context switch
	  # TODO: refactor later
	  ${:block_parser} current_entity $current_entity
	  ${:block_parser} processed_section [current class]
	  $current_entity current_comment_line_type ${:current_comment_line_type}
	  $current_entity block_parser ${:block_parser}
	}
	
	# :method parse@text {line} { next }
	# :method parse@space {line} { next }
	
      }
  
  CommentBlockParsingState create description -superclass CommentSection \
      -next_comment_section part \
      -comment_line_transitions {
	->text		parse
	->tag		next
	text->text	parse
	text->space	""
	space->text	parse
	space->space	parse
	space->tag	next
    } {
      
      :method on_enter {line} {
	unset -nocomplain :@doc
	next
      }

      # tag lines are not allowed in description blocks!
      # :method parse@tag {line} {;}
      :method parse@space {line} {
	:@doc add "" end
	next
      }
      
    }

  CommentBlockParsingState create part -superclass CommentSection  \
      -next_comment_section part \
      -comment_line_transitions {
	->tag		parse
	tag->text	parse
	text->text	parse
	text->tag	next
	text->space	""
	space->space	""
	tag->space	""
	space->tag	next
	tag->tag	next
      } {
	# realise the parse events specific to the substates of description
	# :method parse@tag {line} {;}
	# :method parse@text {line} {;}
	# :method parse@space {line} {;}
      }
}

puts stderr "Doc Tools loaded: [info command ::nx::doc::*]"