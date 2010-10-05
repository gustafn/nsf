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

  proc find_asset_path {{subdir library/lib/doc-assets}} {
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


  Class create MixinLayer -superclass Class {
    :attribute {prefix ""}
    :public method apply {} {
      foreach mixin [:info children -type [current class]::Mixin] {
	set base "${:prefix}::[namespace tail $mixin]"
	puts "TRYING mixin $mixin base $base"
	if {[::nsf::isobject $base]} {
	  set scope [expr {[$mixin scope] eq "object" && [$base info is class]?"class-object":""}]
	  puts stderr "APPLYING $base {*}$scope mixin add $mixin"
	  $base {*}$scope mixin add $mixin
	}
    }
    }
    
    Class create [current]::Mixin -superclass Class {
      :attribute {scope class}
      :method init args {
	:public method foo {} {
	  puts stderr "[current class]->[current method]";
	  next
	}
      }
    }
  }

  Class create Tag -superclass Class {
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

    namespace eval ::nx::doc::entities {}

    :public class-object method normalise {tagpath names} {
      # 1) verify balancedness of 
      if {[llength $tagpath] != [llength $names]} {
	return [list 1 "Imbalanced tag line spec: '$tagpath' vs. '$names'"]
      }
      
      # 2) expand shortcuts (i.e., nested lists into additional tag
      # path elements) and flatten the tagpath list.
      set expanded [list]
      
      foreach n $names {
	lappend expanded {*}[lrepeat [llength $n] [lindex $tagpath [lsearch -exact $names $n]]]
      }

      return [list 0 [list $expanded [concat {*}$names]]]

    }
    
    :public class-object method find {
	-strict:switch 
	-all:switch 
	tagpath 
	names 
	{entity ""}} {

      if {[llength $tagpath] != [llength $names]} {
	return [list 1 "Imbalanced tag line spec: '$tagpath' vs. '$names'"]
      }

      # make sure that expansion has been applied (not allowing sub-lists in names!)

      if {[concat {*}$names] ne $names} {
	return [list 1 "Names list contains sub-lists. Not expanded?"]
      }

      set last_axis [expr {$entity ne ""?[$entity info class]:""}]
      set last_name [expr {$entity ne ""?[$entity name]:""}]
      set entity_path [list]
      foreach axis $tagpath value $names {
	if {$entity eq ""} {
	  if {[QualifierTag info instances @$axis] eq "" && [Tag info instances @$axis] eq ""} {
	    return [list 1 "The entity type '@$axis' is not available."]
	  }
	  set entity [@$axis id $value]
	} else {
	  if {$strict && ![::nsf::isobject $entity]} {
	    return [list 1 "The tag path '$tagpath' -> '$names' points to a non-existing documentation entity: '@$last_axis' -> '$last_name'"]
	  }
      if {$all} {lappend entity_path $entity [$entity name]}
	  set entity [$entity origin]
	  if {[$entity info lookup methods -source application @$axis] eq ""} {
	    return [list 1 "The tag '$axis' is not supported for the entity type '[namespace tail [$entity info class]]'"]
	  }
	  #puts stderr "$entity @$axis id $value"
	  set entity [$entity @$axis id $value]
	  set last_axis $axis
	  set last_name $value
	}
      }

      if {$strict && $entity ne "" && ![::nsf::isobject $entity]} {
	return [list 1 "The tag path '$tagpath' -> '$names' points to a non-existing documentation entity: '@$last_axis' -> '$last_name'"]
      }
      if {$all} {lappend entity_path $entity [$entity name]}


      return [list 0 [expr {$all?$entity_path:$entity}]]
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

    :public method id {
      -partof_name
      {-scope ""} 
      name
    } {
      set subns [string trimleft [namespace tail [current]] @]
      if {[info exists partof_name]} {
	set partof_name [string trimleft $partof_name :]
	return [join [list [:root_namespace] $subns $partof_name {*}$scope $name] ::]
      } else {
	return "[:root_namespace]::${subns}$name"
      }
    }

    :public method new {
      -part_attribute
      -partof:object,type=::nx::doc::Entity
      -name:required 
      args
    } {
      # A refined frontend for object construction/resolution which
      # provides for generating an explicit name, according to the
      # rules specific to the entity type.
      #
      # @param name The of the documented entity
      # @return The identifier of the newly generated or resolved entity object
      # set fq_name [:get_fully_qualified_name $name]
      set ingredients [list]
      if {[info exists partof]} {
	lappend ingredients -partof_name [$partof name]
	lappend ingredients -scope [expr {[info exists part_attribute]?[$part_attribute scope]:""}]
      }
      lappend ingredients $name
      :createOrConfigure [:id {*}$ingredients] -name $name {*}$args
    }
    
    :method createOrConfigure {id args} {
      # puts stderr "createOrConfigure id $id"
      # This method handles verifies whether an entity object based on
      # the given id exists. If so, it returns the resolved name. If
      # not, it provides for generating an object with the precomputed
      # id for the first time!
      #
      # @param id The identifier string generated beforehand
      # @return The identifier of the newly generated or resolved entity object
      # @see {{@method id}}
      namespace eval $id {}
      if {[::nsf::isobject $id]} {
	$id configure {*}$args
	# return $id
      } else {
	:create $id {*}$args
      }
      return $id
    }

    # @method get_unqualified_name
    #
    # @param qualified_name The fully qualified name (i.e., including the root namespace)
    :public method get_unqualified_name {qualified_name} {
      # TODO: danger, tcl-commands in comments
      # similar to \[namespace tail], but the "tail" might be an object with a namespace
      return [string trimleft [string map [list [:root_namespace] ""] $qualified_name] ":"]
    }
    :public method get_tail_name {qualified_name} {
      return [string trimleft [string map [list ${:tag} ""] [:get_unqualified_name $qualified_name]]  ":"]
    }
  }

  Class create QualifierTag -superclass Tag {
    :method get_fully_qualified_name {name} {
      if {![string match "::*" $name]} {
	error "You need to provide a fully-qualified (absolute) entity name for '$name'."
      }
      return $name
    }

    :public method id {
      -partof_name
      {-scope ""} 
      name
    } {
      if {[info exists partof_name]} {
	#puts stderr "QUALIFIER=[join [list $partof_name $name] ::]"
	#next [join [list $partof_name $name] ::]
	next
      } else {
	set n [:get_fully_qualified_name $name]
#	puts stderr FINALNAME=$n
	next $n
      }
    }

    :public method new {
      -part_attribute
      -partof:object,type=::nx::doc::Entity
      -name:required 
      args
    } {
      set id_name $name
      if {[info exists partof]} {
	#set name [join [list [$partof name] $name] ::]
	set id_name ::[join [list [[$partof info class] get_tail_name $partof] $name] ::]
      } else {
	set name [:get_fully_qualified_name $name]
      }
      :createOrConfigure [:id $id_name] \
	  {*}[expr {[info exists part_attribute]?"-part_attribute $part_attribute":""}] \
	  {*}[expr {[info exists partof]?"-partof $partof":""}] \
	  -name $name {*}$args
    }
  }

  Class create PartTag -superclass Tag {
    :public method id {partof_name scope name} {
      next [list -partof_name $partof_name -scope $scope $name]
    }

    :public method new {	       
      -part_attribute:required
      -partof:object,type=::nx::doc::Entity
      -name 
      args
    } {
      :createOrConfigure [:id [$partof name] [$part_attribute scope] $name] {*}[current args]
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

    :attribute {pretty_name {[string totitle [string trimleft [namespace tail [current]] @]]}}
    :attribute {pretty_plural {[string totitle [string trimleft [namespace tail [current]] @]]}}

    # :forward owning_entity_class {% [[:info parent] info parent] }
    :method init args {
      :defaultmethods [list get append]
      :multivalued true
      set :incremental true
      # TODO: setting a default value leads to erratic behaviour;
      # needs to be verified -> @author returns ""
      # :default ""
      if {![info exists :scope]} {
	set :scope ""
	regexp -- {@(.*)-.*} [namespace tail [current]] _ :scope
      }
      next
    }
    
    :public method id {domain prop value} {
      #puts stderr "PARTATTRIBUTE-ID: [current args]"
      if {![info exists :part_class]} {
	error "Requested id generation from a simple part attribute!"
      }
      return [${:part_class} id [$domain name] ${:scope} $value]
    }

    :method require_part {domain prop value} {
      if {[info exists :part_class]} {
	if {[::nsf::is object $value] && \
		[$value info has type ${:part_class}]} {
	  return $value
	}
	# puts stderr "NEWWWWWW ${:part_class} new \
	# 	     -name [lindex $value 0] \
	# 	     -partof $domain \
	# 	     -part_attribute [current] \
	# 	     -@doc [lrange $value 1 end]"
	return  [${:part_class} new \
		     -name [lindex $value 0] \
		     -partof $domain \
		     -part_attribute [current] \
		     -@doc [lrange $value 1 end]]
      }
      return $value
    }
    :public method append {domain prop value} {
      :add $domain $prop $value end
    }
    :public method assign {domain prop value} {
      set parts [list]
      foreach v $value {
	lappend parts [:require_part $domain $prop $v]
      }
      next [list $domain $prop $parts]
    }
    :public method add {domain prop value {pos 0}} {
      set p [:require_part $domain $prop $value]
      if {![$domain eval [list info exists :$prop]] || $p ni [$domain $prop]} {
	next [list $domain $prop $p $pos]
      }
      return $p
    }
    :public method delete {domain prop value} {
      next [list $domain $prop [:require_part $prop $value]]
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
    :method objectparameter args {
      next [list [list @doc:optional __initcmd:initcmd,optional]]
    }

    :attribute partof:object,type=::nx::doc::StructuredEntity
    :attribute part_attribute:object,type=::nx::doc::PartAttribute
 
    :public method get_upward_path {
      -relative:switch 
      {-attribute {set :name}}
      {-type ::nx::doc::Entity}
    } {
      set path [list]
      if {!$relative} {
	lappend path [list [current] [:eval $attribute]]
      }
      #puts stderr ARGS=[current args]-[info exists :partof]
      #puts stderr HELP=$path
      
      if {[info exists :partof] && [${:partof} info has type $type]} {
	#puts stderr "CHECK ${:partof} info has type $type -> [${:partof} info has type $type]"
	
	set path [concat [${:partof} [current method] -attribute $attribute -type $type] $path]
      }
      #puts stderr PATHRETURN=$path
      return [concat {*}$path]
    }

    :attribute @doc:multivalued {set :incremental 1}
    :attribute @see -slotclass ::nx::doc::PartAttribute
    :attribute @properties -slotclass ::nx::doc::PartAttribute

    :attribute @use {
      :public method assign {domain prop value} {
	# @command nx
	#
	# @use ::nsf::command
	# @use {Object foo}
	# @use command {Object foo}
	lassign $value pathspec pathnames
	if {$pathnames eq ""} {
	  set pathnames $pathspec
	  # puts stderr PATH=[$domain get_upward_path \
	  # 				    -attribute {[:info class] tag}]
	  # puts stderr "dict create {*}[$domain get_upward_path \
	  # 				    -attribute {[:info class] tag}]"
	  set pathspec [dict create {*}[$domain get_upward_path \
					    -attribute {[:info class] tag}]]
	  set pathspec [dict values $pathspec]
	} else {
	  set pathspec [split $pathspec .]
	}
	#puts stderr "PATHSPEC $pathspec PATHNAMES $pathnames"
	lassign [::nx::doc::Tag normalise $pathspec $pathnames] err res
	if {$err} {
	  error "Invalid @use values provided: $res"
	}
	
	lassign $res pathspec pathnames
	
	lassign [::nx::doc::Tag find $pathspec $pathnames] err res
	if {$err} {
	  error "Generating an entity handle failed: $res"
	}
	#puts stderr "next $domain $prop $res"
	next [list $domain $prop $res]
      }
      
    }

    :public method origin {} {
      if {[info exists :@use]} {
	#puts stderr ORIGIN(${:@use})=isobj-[::nsf::isobject ${:@use}]
	if {![::nsf::isobject ${:@use}] || ![${:@use} info has type [:info class]]} {
	  error "Referring to a non-existing doc entity or a doc entity of a different type."
	}
	return [${:@use} origin]
      }
      return [current]
    }

    :method has_property {prop} {
       if {![info exists :@properties]} {return 0}
	expr {$prop in ${:@properties}}
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
	  if {[::nsf::isobject $docobj]} break
	}
	if {[::nsf::isobject $docobj]} {
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

    :public method as_list {} {
      if {[info exists :@doc] && ${:@doc} ne ""} {
	#puts stderr DOC=${:@doc}
	set non_empty_elements [lsearch -all -not -exact ${:@doc} ""]
	return [lrange ${:@doc} [lindex $non_empty_elements 0] [lindex $non_empty_elements end]]
      }
    }

    :public method as_text {} {
      set doc [list]
      set lines [:as_list]
      foreach l $lines {
	lappend doc [string trimleft $l]
      }
      return [subst [join $doc " "]]
    }
  }

  Tag create @glossary -superclass Entity {
    :attribute @pretty_name
    :attribute @pretty_plural
    :attribute @acronym
  }


  Class create StructuredEntity -superclass Entity {
    :public method owned_parts {} {
      set slots [:info lookup slots]
      set r [dict create]
#      puts stderr SLOTS=$slots
      foreach s $slots {
	if {![$s info has type ::nx::doc::PartAttribute] || ![$s eval {info exists :part_class}]} continue;
	set accessor [$s name]
#	puts stderr "PROCESSING ACCESSOR $accessor, [info exists :$accessor]"
	if {[info exists :$accessor]} {
	  dict set r $s [sorted [:$accessor] name]
	}
      }
      return $r
    }
  }


  Class create ContainerEntity -superclass StructuredEntity {
    
    Class create [current]::Resolvable {
      :class-object attribute container:object,type=[:info parent]
      :method get_fully_qualified_name {name} {
	set container [[current class] container]
	if {![string match "::*" $name]} {
#	  puts -nonewline stderr "--- EXPANDING name $name"
	  set name [$container @namespace]::$name 
#	  puts stderr " to name $name"
	}
	next $name
      }
    }

    Class create [current]::Containable {
      # TODO: check the interaction of required, per-object attribute and ::nsf::assertion
      #:object attribute container:object,type=[:info parent],required
      :class-object attribute container:object,type=[:info parent]
      :method create args {
	#
	# Note: preserve the container currently set at this callstack
	# level.  [next] will cause the container to change if another
	# container entity is initialised in the following!
	#
	if {[[current class] eval {info exists :container}]} {
	  set container [[current class] container]
	  set obj [next]
	  if {![$obj eval {info exists :partof}]} {
	    $container register $obj
	  }
	  return $obj
	} else {
	  next
	}
      }
    }
    # Note: The default "" corresponds to the top-level namespace "::"!
    :attribute {@namespace ""}

    :attribute @class -slotclass ::nx::doc::PartAttribute {
      :pretty_name "Class"
      :pretty_plural "Classes"
      set :part_class ::nx::doc::@class
    }
    :attribute @object -slotclass ::nx::doc::PartAttribute {
      :pretty_name "Object"
      :pretty_plural "Objects"
      set :part_class ::nx::doc::@object
    }
   
    :attribute @command -slotclass ::nx::doc::PartAttribute {
      :pretty_name "Command"
      :pretty_plural "Commands"
      set :part_class ::nx::doc::@command
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
      QualifierTag mixin add [current class]::Resolvable
      [current class]::Resolvable container [current]
      QualifierTag mixin add [current class]::Containable
      @package class-object mixin add [current class]::Containable
      [current class]::Containable container [current]
    }

    :public method register {containable:object,type=::nx::doc::Entity} {
      set tag [[$containable info class] tag]
      if {[:info lookup methods -source application "@$tag"] ne ""} {
	:@$tag $containable
      }
    }
  }

  Tag create @project -superclass ContainerEntity {
    :attribute url
    :attribute license
    :attribute creationdate
    :attribute {version ""}

    :attribute @package -slotclass ::nx::doc::PartAttribute {
      set :part_class ::nx::doc::@package
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

  Tag create @package -superclass ContainerEntity {
    :attribute @require -slotclass ::nx::doc::PartAttribute
    :attribute @version -slotclass ::nx::doc::PartAttribute
  }

  QualifierTag create @command -superclass StructuredEntity {
    :attribute @parameter -slotclass ::nx::doc::PartAttribute {
      set :part_class ::nx::doc::@param
    }
    :attribute @return -slotclass ::nx::doc::PartAttribute {
      :method require_part {domain prop value} {
	set value [expr {![string match ":*" $value] ? "__out__: $value": "__out__$value"}]
	next [list $domain $prop $value]
	#next $domain $prop "__out__ $value"
      }
      set :part_class ::nx::doc::@param
    }

    :forward @sub-command %self @command
    :attribute @command -slotclass ::nx::doc::PartAttribute {
      :pretty_name "Subcommand"
      :pretty_plural "Subcommands"
      :public method id {domain prop value} { 
	# TODO: [${:part_class}] resolves to the attribute slot
	# object, not the global @command object. is this intended, in
	# line with the intended semantics?
	return [${:part_class} [current method] -partof_name [$domain name] -scope ${:scope} $value]
      }
      set :part_class ::nx::doc::@command
    }
    :public method parameters {} {
      set params [list]
      if {[info exists :@parameter]} {
	foreach p [:@parameter] {
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
  
  QualifierTag create @object \
      -superclass StructuredEntity \
      -mixin ContainerEntity::Containable {
	:attribute @author -slotclass ::nx::doc::PartAttribute 

	:forward @object %self @child-object
	:attribute @child-object -slotclass ::nx::doc::PartAttribute {
	  set :part_class ::nx::doc::@object
	  :public method id {domain prop value} {
#	    puts stderr "CHILD-OBJECT: [current args]"
	    # if {![info exists :part_class]} {
	    #   error "Requested id generation from a simple part attribute!"
	    # }
	    return [${:part_class} id [join [list [$domain name] $value] ::]]
#	    return [${:part_class} id -partof_name [$domain name] -scope ${:scope} $value]
	  }

	}

	:forward @class %self @child-class
	:attribute @child-class -slotclass ::nx::doc::PartAttribute {
	  set :part_class ::nx::doc::@class
	  :public method id {domain prop value} {
	    #puts stderr "CHILD-CLASS: [current args]"
	    # if {![info exists :part_class]} {
	    #   error "Requested id generation from a simple part attribute!"
	    # }
	    return [${:part_class} id [join [list [$domain name] $value] ::]]
	    #return [${:part_class} id -partof_name [$domain name] -scope ${:scope} $value]
	  }
	}

	:forward @method %self @object-method
	:attribute @class-object-method -slotclass ::nx::doc::PartAttribute {
	  set :part_class ::nx::doc::@method
	}

	:forward @attribute %self @class-object-attribute
	#:forward @param %self @object-param
	:attribute @class-object-attribute -slotclass ::nx::doc::PartAttribute {
	  set :part_class ::nx::doc::@param
	}

	:method undocumented {} {
	  # TODO: for object methods and class methods
	  if {![::nsf::isobject ${:name}]} {return ""}
	  foreach m [${:name} info methods -callprotection all] {set available_method($m) 1}
	  set methods ${:@method}
	  if {[info exists :@param]} {set methods [concat ${:@method} ${:@param}]}
	  foreach m $methods {
	    set mn [namespace tail $m]
	    if {[info exists available_method($mn)]} {unset available_method($mn)}
	  }
	  return [lsort [array names available_method]]
	}
      }

  QualifierTag create @class \
      -superclass @object {
	:attribute @superclass -slotclass ::nx::doc::PartAttribute
	
	:forward @attribute %self @class-attribute
	:attribute @class-attribute -slotclass ::nx::doc::PartAttribute {
	  :pretty_name "Per-class attribute"
	  :pretty_plural "Per-class attributes"
	  set :part_class ::nx::doc::@param
	}
	
	:forward @method %self @class-method
	:attribute @class-method -slotclass ::nx::doc::PartAttribute {
	  :pretty_name "Per-class method"
	  :pretty_plural "Per-class methods"
	  set :part_class ::nx::doc::@method
	  :method require_part {domain prop value} {
	    # TODO: verify whether these scoping checks are sufficient
	    # and/or generalisable: For instance, is the scope
	    # requested (from the part_attribute) applicable to the
	    # partof object, which is the object behind [$domain name]?
	    if {[info exists :scope] && 
		![::nsf::is ${:scope} [$domain name]]} {
	      error "The entity '[$domain name]' does not qualify as '${:scope}'"
	    }
	    next
	  }
	}
	
	:method inherited {member} {
	  if {[${:name} info is class]} {
	    set inherited [dict create]
	    foreach c [lreverse [${:name} info heritage]] {
	      set entity [[::nsf::current class] id $c]
	      if {![::nsf::is object $entity]} continue
	      if {[$entity eval [list info exists :${member}]]} {
		dict set inherited $entity [$entity $member]
	      }
	    }
	    return $inherited
	  }
	}
      }


  Class create PartEntity -superclass Entity {
    :attribute partof:object,type=::nx::doc::StructuredEntity,required
    :attribute part_attribute:object,type=::nx::doc::PartAttribute,required
  }
 

  # @object ::nx::doc::@method
  #
  # "@method" is a named entity, which is part of some other
  # docEntity (a class or an object). We might be able to use the
  # "use" parameter for registered aliases to be able to refer to the 
  # documentation of the original method.
  #
  PartTag create @method \
      -superclass StructuredEntity {
	:attribute {@modifier public} -slotclass ::nx::doc::PartAttribute
	:attribute @parameter -slotclass ::nx::doc::PartAttribute {
	  set :part_class ::nx::doc::@param
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
	    next [list $domain $prop $value]
	  }
	  set :part_class ::nx::doc::@param
	}

	:public class-object method new {	       
	  -part_attribute:required
	  -partof:object,type=::nx::doc::Entity
	  -name 
	  args
	} {
	  # 1) Are we in a sub-method?
	  if {[$partof info has type [current]]} {
	    :createOrConfigure [:id [:get_tail_name $partof] "" $name] {*}[current args]
	  } else {
	    next
	  }
	}
	


	:forward @class-method %self @method
	:forward @class-object-method %self @method
	:forward @sub-method %self @method
	:attribute @method -slotclass ::nx::doc::PartAttribute {
	  set :part_class ::nx::doc::@method
	  :public method id {domain prop name} {
	    # TODO: ${:part_class} resolves to the local slot
	    # [current], rather than ::nx::doc::@method. Why?
	    if {[$domain info has type ::nx::doc::@method]} {
	      set id [::nx::doc::@method id [::nx::doc::@method get_tail_name $domain] "" $name]
	      return $id
	    } else {
	      return [::nx::doc::@method id [$domain name] ${:scope} $name]
	    }
	  }
	  
	  # :method require_part {domain prop value} {
	  #   set partof [$domain partof]
	  #   next $partof $prop [join [list [[$domain part_attribute] scope] [$domain name] $value] ::]
	  # }
	}

	:public method parameters {} {
	  set params [list]
	  if {[info exists :@parameter]} {
	    foreach p [:@parameter] {
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
	    if {[::nsf::isobject $object]} {
	      if {[$object info methods -callprotection all ${:name}] ne ""} {
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
		  # TODO: requesting the param spec of an ensemble
		  # object (info) does not work right now? How to deal
		  # with it?
		  if {($object eq "::nx::Object" || $object eq "::nx::Class") && ${:name} eq "info"} {
		    set actualParams ""
		    set syntax ""
		  } else {
		    set actualParams [$object info method parameter ${:name}]
		    set syntax [$object info method parametersyntax ${:name}]
		  }
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
	    return [concat $params <br>$comment]
	    } 
	    return $params
	}
	
	:public method get_sub_methods {} {
	  if {[info exists :@method]} {
	    set leaves [list]
	    foreach m ${:@method} {
	      if {![$m eval {info exists :@method}]} {
		lappend leaves $m
	      } else {
		lappend leaves {*}[$m get_sub_methods]
	      }
	    }
#	    puts stderr LEAVES=$leaves
	    #puts stderr [::nx::doc::entities::method::nx::Object::class::info::has @method]
	    return $leaves
	  }
	}

	:public method get_combined {what} {
	  set result [list]
	  if {[info exists :partof] && [${:partof} info has type [current class]]} {
	    lappend result {*}[${:partof} get_combined $what] [:$what]
	  }
	  return $result
	}

      }; # @method
  
  # PartTag create @subcommand -superclass {Part @command}
  #  PartTag create @subcommand -superclass {Part @command}

  # @object ::nx::doc::@param
  #
  # The entity type "@param" represents the documentation unit
  # for several parameter types, e.g., object, method, and
  # command parameters.
  #
  PartTag create @param \
      -superclass PartEntity {
	:attribute spec
	:attribute default
	  

	:public class-object method id {partof_name scope name} {
	  next [list [:get_unqualified_name ${partof_name}] $scope $name]
	}
	
	# :class-object method id {partof_name name} {
	#   # The method contains the parameter-specific name production rules.
	#   #
	#   # @param partof Refers to the entity object which contains this part 
	#   # @param name Stores the name of the documented parameter
	#   # @modifier protected

	#   set partof_fragment [:get_unqualified_name ${partof_name}]
	#   return [:root_namespace]::${:tag}::${partof_fragment}::${name}
	# }
	
	# @class-object-method new
	#
	# The per-object method refinement indirects entity creation
	# to feed the necessary ingredients to the name generator
	#
	# @param -part_attribute 
	# @param -partof
	# @param -name
	# @param args
	:public class-object method new {
		-part_attribute 
		-partof:required
		-name 
		args
	      } {
	  
	  lassign $name name def
	  set spec ""
	  regexp {^(.*):(.*)$} $name _ name spec
	  :createOrConfigure [:id $partof [$part_attribute scope] $name] \
	      -spec $spec \
	      -name $name \
	      -partof $partof \
	      {*}[expr {$def ne "" ? "-default $def" : ""}] \
	      -part_attribute $part_attribute {*}$args
	  
	}
      }

  #
  # Provide two interp-wide aliases for @param. This is mere syntactic
  # sugar!
  #
  interp alias {} ::nx::doc::@attribute {} ::nx::doc::@param
  interp alias {} ::nx::doc::@parameter {} ::nx::doc::@param

  namespace export CommentBlockParser @command @object @class @package \
      @project @method @attribute @parameter @
}



namespace eval ::nx::doc {

  Class create Renderer {
    :public method run {} {
      :render=[namespace tail [:info class]]
    }
  }

  Class create TemplateData {  

    :public method write_data {content path} {
      set fh [open $path w]
      puts $fh $content
      catch {close $fh}
    }


    :public method read_tmpl {path} {
      if {[file pathtype $path] ne "absolute"} {
	set assetdir [find_asset_path]
	set tmpl [file join $assetdir $path]
      } else {
	set tmpl [file normalize $path]
      }
      
      if {![[current class] eval [list info exists :templates($tmpl)]]} {
	if {![file exists $tmpl] || ![file isfile $tmpl]} {
	  error "The template file '$path' was not found."
	}
	set fh [open $tmpl r]
	[current class] eval [list set :templates($tmpl) [read $fh]]
	catch {close $fh}
      }
      
      return [[current class] eval [list set :templates($tmpl)]]
    }

    # This mixin class realises a rudimentary templating language to
    # be used in nx::doc templates. It realises language expressions
    # to verify the existence of variables and simple loop constructs
    :public method render {
      {-initscript ""}
      template 
      {entity:substdefault "[current]"}
    } {
      # Here, we assume the -nonleaf mode being active for {{{[eval]}}}.
      set tmplscript [list subst [:read_tmpl $template]]
      $entity eval [subst -nocommands {
	$initscript
	$tmplscript
      }]
      # $entity eval [list subst $template]
    }
    
    
    #
    # some instructions for a dwarfish, embedded templating language
    #
    :method !let {var value} {
      # uplevel 1 [list ::set $var [expr {[info exists value]?$value:""}]]
      uplevel 1 [list ::set $var $value]
      return
    }

    :method !get {-sortedby varname} {
      if {[info exists sortedby]} { 
	uplevel 1 [list sorted [[:origin] eval [list ::set :$varname]] $sortedby]
      } else {
	uplevel 1 [list [:origin] eval [list ::set :$varname] ]
      }
    }

    :method for {var list body} { 
      set rendered ""
      ::foreach $var $list {
	uplevel 1 [list ::set $var [set $var]]
	#uplevel 1 [list ::lassign [set $var] {*}$var]
	append rendered [uplevel 1 [list subst $body]]
      }
      return $rendered
    }

    :method ?objvar {obj varname args} {
     # set args [lassign $args then_script]
    #  append script "\[::set $varname \[$obj eval {set :$varname; puts stderr >>>>\[set :$varname\]}\]\]\n" $then_script
      uplevel 1 [list :? -ops [list [::nsf::current method] -] \
		     "\[$obj eval {info exists :$varname}\]" {*}$args]
    }

    :method ?var {varname args} {
      set cmd [expr {[string match ":*" $varname]?"\[[:origin] eval {info exists $varname}\]":"\[info exists $varname\]"}]
      uplevel 1 [list :? -ops [list [::nsf::current method] -] \
		     $cmd {*}$args]
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
      uplevel 1 [list subst [:read_tmpl $template]]
    }

    :method code args {
      error "Subclass responsibility: You must provide a method definition of '[current method]' in a proper subclass"
    }

    :method link args {
      error "Subclass responsibility: You must provide a method definition of '[current method]' in a proper subclass"
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

    :method as_list {} {
	set preprocessed [list]
	set is_code_block 0
	foreach line [next] {
	  if {[regsub -- {^\s*(\{\{\{)\s*$} $line "\[:code -inline false \{" line] || \
		  (${is_code_block} && [regsub -- {^\s*(\}\}\})\s*$} $line "\}\]" line])} {
	    set is_code_block [expr {!$is_code_block}]
	    append line \n
	  } elseif {${is_code_block}} {
	    # set line [:map $line unescape]
	    append line \n
	  } else {
	    # set line [:map $line sub]
	    # set line [:map $line unescape]
	    set line [string trimleft $line]
	    if {$line eq {}} {
	      set line "\n\n"
	    }
	  } 
	  lappend preprocessed $line
	}
      return $preprocessed
    }

    :public method as_text {} {
      set preprocessed [join [:as_list] " "]
      set preprocessed [:map $preprocessed sub]
      set preprocessed [:map $preprocessed unescape]
      return [subst $preprocessed]
    }

  }

    #
    # A default TemplateData implementation, targeting the derived YUI
    # Doc templates.
    # 

  MixinLayer create NxDocRenderer -superclass Renderer -prefix ::nx::doc {
    :public method run {{-tmpl entity.html.tmpl} {-outdir /tmp}} {

      #
      # Note: For now, this method is called upon a @project
      # instance. This may change during continued refactoring.
      #

      # 1) apply the mixin layer
      [current class] apply

      # 2) proceed by rendering the project's parts (package, class,
      # object, and command entities)
      set ext [lindex [split [file tail $tmpl] .] end-1]
      set top_level_entities [:owned_parts]
      dict for {feature instances} $top_level_entities {
	if {[$feature name] eq "@package"} {
	  foreach {entity_type pkg_entities} [$feature owned_parts] {
	    dict lappend top_level_entities $entity_type {*}$pkg_entities
	  }
	}
      }

      set init [subst {
	set project [current object]
	set project_entities \[list $top_level_entities\]
      }]
      set project_path [file join $outdir [string trimleft ${:name} :]]
      if {![catch {file mkdir $project_path} msg]} {
	#	  puts stderr [list file copy -force -- [$renderer find_asset_path] $project_path/assets]
	set assets [lsearch -all -inline -glob -not [glob -directory [find_asset_path] *] *.tmpl]
	set target $project_path/assets
	file mkdir $target
	file copy -force -- {*}$assets $target
	
	set values [join [dict values $top_level_entities]]
	#	puts stderr "VALUES=$values"
	foreach e $values {
	  #puts stderr "PROCESSING=$e render -initscript $init $tmpl"
	  set content [$e render -initscript $init $tmpl]
	  :write_data $content [file join $project_path "[$e filename].$ext"]
	  puts stderr "$e written to [file join $project_path [$e filename].$ext]"
	}

	set index [:render -initscript $init $tmpl]
	# puts stderr "we have [llength $entities] documentation entities ($entities)"
	:write_data $index [file join $project_path "index.$ext"]


      }
            
      # 3) TODO: revoke the application of the mixin layer (for the sake of
      # some sort of bounded quantification) 
    }

    #
    # The actual refinements delivered by the mixin layer
    #

    MixinLayer::Mixin create [current]::Entity -superclass TemplateData {
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
	#
	# TODO: Should I wrap up delegating calls to the originator
	# entity behind a unified interface (a gatekeeper?)
	#
	set features [[:origin] owned_parts]
	dict for {feature instances} $features {
	  foreach inst $instances {
	    # TODO: @modifier support is specific to the parts of
	    # @object instances. Untangle!
	    set access [expr {[$inst eval {info exists :@modifier}]?[$inst @modifier]:""}]
	    set host ${:name}
	    set name [$inst name]
	    set url  "[:filename].html#[string trimleft [$feature name] @]_[$inst name]"
	    set type [$feature name]
	    lappend entries [subst $entry]
	  }
	}
	return "\[[join $entries ,\n]\]"
      }
       
      :method code {{-inline true} script} {
	return [expr {$inline?"<code>$script</code>":"<pre>$script</pre>"}]
      }
      
      :method link {tag names} {
	#puts stderr "RESOLVING tag $tag names $names"
	set tagpath [split [string trimleft $tag @] .]
	lassign [::nx::doc::Tag normalise $tagpath $names] err res
	if {$err} {
	  #puts stderr RES=$res
	  return "<a href=\"#\">?</a>";
	}
	lassign [::nx::doc::Tag find -all -strict {*}$res] err path
	if {$err || $path eq ""} {
	  #puts stderr "FAILED res $path (err-$err-id-[expr {$path eq ""}])"
	  return "<a href=\"#\">?</a>";
	}

	set path [dict create {*}$path]
	set entities [dict keys $path]
	set id [lindex $entities end]
	return [$id render_link $tag [current] $path]
      }

      :public method render_link {tag source path} {
	#puts stderr PATH=$path
	set id [current]
	set pathnames [dict values $path]
	set entities [dict keys $path]
	set top_entity [lindex $entities 0]
	# puts stderr RESOLPATH([$id info class])=$path
	set pof ""
	if {$top_entity ne $id} {
	  set pof "[$top_entity name]#"
	  set pathnames [lrange $pathnames 1 end]
	  set entities [lrange $entities 1 end]
	}
	return "<a href=\"[$id href $top_entity]\">$pof[join $pathnames .]</a>"
      }
      
      :public method as_text {} {
	set text [expr {[:origin] ne [current]?[[:origin] as_text]:[next]}]
	return [string map {"\n\n" "<br/><br/>"} $text]
      }

      :public method href {-local:switch top_entity:optional} {
	set path [dict create {*}[:get_upward_path -attribute {set :name}]]
	set originator_top_entity [lindex [dict keys $path] 0]
	if {![info exists top_entity] || [dict size $path] == 1} {
	  set top_entity $originator_top_entity
	}
	dict unset path $originator_top_entity
	set fragment_path [list]
	#puts stderr FRAGMENTPATH=$path
	dict for {entity name} $path {
	  lappend fragment_path [$entity filename]
	} 
	set fragments [join $fragment_path _]
	if {$local} { 
	  return $fragments
	} else {
	  set href "[$top_entity filename].html#$fragments"
	  #puts stderr HREF=$href
	  return $href
	}
      }

      :public method filename {} {
	if {[info exists :partof]} {
	  return [string trimleft [${:part_attribute} name] @]_${:name}
	} else {
	  return [[:info class] tag]_[string trimleft [string map {:: __} ${:name}] "_"]
	}
      }
    }; # NxDocTemplating::Entity

    MixinLayer::Mixin create [current]::@glossary -superclass [current]::Entity {
     

      :public method render_link {tag source path} {
	if {![info exists :refs]} {
	  set :refs [dict create]
	}
	dict incr :refs $source
	# TODO: provide the project context here and render the
	# glossary location accordingly, rather than hard-code "index.html".
	return "<a href=\"index.html#${:name}\" title=\"${:@pretty_name}\" class=\"gloss\">[string tolower ${:@pretty_name}]</a>"
      }

      #
      # TODO: this should go into the appropriate template
      #
      :public method render_refs {} {
	if {[info exists :refs]} {
	  dict for {entity count} ${:refs} {
	  }
	}
      }
      
    }
    
  }; # NxDocTemplating
  
  #
  # Provide a simple HTML renderer. For now, we make our life simple
  # by defining for the different supported docEntities different methods.
  #
  # We could think about a java-doc style renderer...
  #
  
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
      if {[info exists :@class-object-method]} {
	set methods [sorted [:@class-object-method] name]
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
    :public method process {{-noeval false} thing args} {
      # 1) in-situ processing: a class object
      if {[::nsf::isobject $thing]} {
	if {[$thing eval {info exists :__initcmd}]} {
	  
          :analyze_initcmd [expr {[::nsf::is class $thing]?"@class":"@object"}] $thing [$thing eval {set :__initcmd}]
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
	set script ""
	if {[file readable $thing]} {
	  # a) process the target file
	  set fh [open $thing r]
	  if {[catch {append script [read $fh]} msg]} {
	    catch {close $fh}
	    :log "error reading the file '$thing', i.e.: '$msg'"
	  }
	  catch {close $fh}
	}
	# b) verify the existence of an *.nxd companion file
	set rootname [file rootname $thing]
	set companion $rootname.nxd
	if {[file isfile $companion] && [file readable $companion]} {
	  set fh [open $companion r]
	  if {[catch {append script "\n\n" [read $fh]} msg]} {
	    catch {close $fh}
	    :log "error reading the file '$thing', i.e.: '$msg'"
	  }
	  catch {close $fh}
	}
	
	if {$script eq ""} {
	  :log "script empty, probaly file '$thing' is not readable"
	}

	doc analyze -noeval $noeval $script {*}$args
	puts stderr FILE=$thing--[file readable $thing]-COMPANION=$companion--[file readable $companion]-ANALYZED-[string length $script]bytes
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
    
    :public method analyze {{-noeval false} script {additions ""}} {
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
	set cbp [::nx::doc::CommentBlockParser process $block]
	# TODO: How to handle contingent (recoverable) conditions here?
	# if {[catch {::nx::doc::CommentBlockParser process $block} msg]} {
	#   if {![InvalidTag behind? $msg] && ![StyleViolation behind? $msg] && ![MissingPartofEntity behind? $msg]} {
	#     if {[Exception behind? $msg]} {
	#       ::return -code error -errorinfo $::errorInfo "[$msg info class]->[$msg message]"
	#       # error [$msg info class]->[$msg message]
	#     }
	#     ::return -code error -errorinfo $::errorInfo $msg
	#   }
	# }
      }
      # 3) process the recorded object additions, i.e., the stored
      # initcmds and method bodies.
      foreach addition $additions {
	# TODO: for now, we skip over pure Tcl commands and procs
	if {![::nsf::is object $addition]} continue;
	set kind [expr {[::nsf::is class $addition]?"@class":"@object"}]
	#puts stderr "ADDITION :process [namespace origin $addition]"
	if {[$addition eval {info exists :__initcmd}]} {
	  :analyze_initcmd $kind $addition [$addition eval {set :__initcmd}]
	}

	# TODO: Note, the CommentBlockParser should operate on the
	# level of a single block, not entire initcmd and method body
	# scripts. The process=@object ressembles some ::nx::doc
	# methods, so relocated and call the parser from within.
	set entity [@ $kind $addition]
	#puts stderr ":process=$kind $entity"
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

    :public method analyze_line {line} {
      set regex {^[\s#]*#+(.*)$}
      if {[regexp -- $regex $line --> comment]} {
	return [list 1 [string trimright $comment]]
      } else {
	return [list 0 $line]
      }
    }
    
    :public method comment_blocks {script} {
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
	   
    :public method analyze_initcmd {{-parsing_level 1} docKind name initcmd} {
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
	set cbp [CommentBlockParser process -parsing_level $parsing_level -partof_entity $id {*}$arguments]
	
#	if {[catch {CommentBlockParser process -partof_entity $id {*}$arguments} msg]} {
#	  lappend failed_blocks $line_offset
#	}
      }
      
    }; # analyze_initcmd method
    
    # TODO: how can I obtain some reuse here when later @class is
    # distinguished from @object (dispatch along the inheritance
    # hierarchy?)
    :public method process=@class {entity} {
      set name [$entity name]

      # attributes
      foreach slot [$name info slots] {
	if {[$slot eval {info exists :__initcmd}]} {
	  set blocks [:comment_blocks [$slot eval {set :__initcmd}]]
	  foreach {line_offset block} $blocks {
	    if {$line_offset > 1} break;	      
	    set scope [expr {[$slot per-object]?"class-object":"class"}]
	    set id [$entity @${scope}-attribute [$slot name]]
	    CommentBlockParser process \
		-parsing_level 2 \
		-partof_entity $entity \
		-initial_section description \
		-entity $id \
		$block
	  }

          # :analyze_initcmd -parsing_level 2 @class $name [$name eval {set :__initcmd}]
	}
      }

      foreach methodName [$name info methods \
			      -methodtype scripted \
			      -callprotection all] {
	# TODO: should the comment_blocks parser be relocated?
	set blocks [:comment_blocks [${name} info method \
					    body $methodName]]
	foreach {line_offset block} $blocks {
	  if {$line_offset > 1} break;	      
	  set id [$entity @class-method $methodName]
	  CommentBlockParser process \
	      -parsing_level 2 \
	      -partof_entity $entity \
	      -initial_section description \
	      -entity $id \
	      $block
	}
      }
      
      :process=@object $entity class-object
      
    }
    
    :public method process=@object {entity {scope ""}} {
      set name [$entity name]
      
      # methods
      foreach methodName [${name} {*}$scope info methods\
			      -methodtype scripted \
			      -callprotection all] {
	
	set blocks [:comment_blocks [${name} {*}$scope info method \
					    body $methodName]]
	foreach {line_offset block} $blocks {
	  if {$line_offset > 1} break;
	  set id [$entity @class-object-method $methodName]
	  CommentBlockParser :process \
	      -parsing_level 2 \
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
       
    :method write {content path} {
      set fh [open $path w]
      puts $fh $content
      catch {close $fh}
    }

    :public method doc {
      {-renderer ::nx::doc::HtmlRenderer}
      -project:object,type=::nx::doc::@project
      args
    } {
      $project mixin add $renderer
      $project run {*}$args
      $project mixin delete $renderer
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

    :attribute {parsing_level:integer 0}

    :attribute {message ""}
    :attribute {status:in "COMPLETED"} {

      set :incremental 1
      
      set :statuscodes {
	COMPLETED
	INVALIDTAG
	MISSINGPARTOF
	STYLEVIOLATION
	LEVELMISMATCH
      }
      
      :public method type=in {name value} {
	if {$value ni ${:statuscodes}} {
	  error "Invalid statuscode '$code'."
	}
	return $value
      }
      
      :public method ? [list obj var value:in,slot=[current object]] {
	return [expr {[:get $obj $var] eq $value}]
      }

      :public method is {obj var value} {
	return [expr {$value in ${:statuscodes}}]
      }
    }

    :attribute processed_section  {
      set :incremental 1
      :public method assign {domain prop value} {
	set current_entity [$domain current_entity]
	set scope [expr {[$current_entity info is class]?"class-object mixin":"mixin"}]
	#	puts stderr "Switching: [$current_entity {*}$scope] --> target $value"
	if {[$domain eval [list info exists :$prop]] && [:get $domain $prop] in [$current_entity {*}$scope]} {
	  $current_entity {*}$scope delete [:get $domain $prop]
	}
	$current_entity {*}$scope add [next [list $domain $prop $value]]
      }
    }
    :attribute current_entity:object
    
    :public class-object method process {
			      {-partof_entity ""}
			      {-initial_section context}
			      {-parsing_level 0}
			      -entity
			      block
			    } {
	
	if {![info exists entity]} {
	  set entity [Entity]
	}
      
	set parser_obj [:new -current_entity $entity -parsing_level $parsing_level]
	$parser_obj [current proc] \
	    -partof_entity $partof_entity \
	    -initial_section $initial_section \
	    $block
	return $parser_obj
      }
    
    :forward has_next expr {${:idx} < [llength ${:comment_block}]}
    :method dequeue {} {
      set r [lindex ${:comment_block} ${:idx}]
      incr :idx
      return $r
    }
    :forward rewind incr :idx -1
    :forward fastforward set :idx {% llength ${:comment_block}}

    :public method cancel {statuscode {msg ""}} {
      :fastforward
      :status $statuscode
      :message $msg
      uplevel 1 [list ::return -code error $statuscode]
    }
    #
    # everything below assumes that the current class is an active mixin
    # on an instance of an Entity subclass!
    #

    :public method process {
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
#      set failure ""
      
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
	 # puts stderr "PROCESS ${:current_entity} event=process $line"
	  ${:current_entity} event=process $line
	} failure]} {
	  if {![:status is $failure]} {
	    ::return -code error -errorinfo $::errorInfo
	  }
	} 
      }
      if {!$is_first_iteration} {
	${:current_entity} on_exit $line
      }

      # ISSUE: In case of some sub-method definitions (namely "info
      # mixin"), the sub-method entity object for "mixin" replaces the
      # forward handlers of the mixin relation slot. So, any slot-like
      # interactions such as delete() won't work anymore. We need to
      # bypass it by using ::nsf::relation, for the time being. This
      # is a clear con of the explicit naming of entity objects (or at
      # least the current scheme)!

      # if {[${:processed_section} info mixinof -scope object ${:current_entity}] ne ""} {
      # 	${:current_entity} {*}$scope mixin delete ${:processed_section}
      # }

      set scope [expr {[${:current_entity} info is class]?"class-object":""}]
      set mixins [${:current_entity} {*}$scope info mixin classes]
      if {${:processed_section} in $mixins} {
	set idx [lsearch -exact $mixins ${:processed_section}]
	set mixins [lreplace $mixins $idx $idx]
	::nsf::relation ${:current_entity} object-mixin $mixins
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
	${:block_parser} cancel STYLEVIOLATION $msg
	# [StyleViolation new -message $msg] throw
      }
      return [list $tgt_line_type $transitions(${src_line_type}->${tgt_line_type})]
    }

    # the actual events to be signalled to and sensed within the
    # super-states and sub-states

    :public method event=process {line} {
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

   
    # realise the sub-state (a variant of METHOD-FOR-STATES) and their
    # specific event handling
    :method parse@tag {line} {
      set line [split [string trimleft $line]]
      set tag [lindex $line 0]
      if {[:info lookup methods -source application $tag] eq ""} {
	set msg "The tag '$tag' is not supported for the entity type '[namespace tail [:info class]]"
	${:block_parser} cancel INVALIDTAG $msg
      }
#      puts stderr ":$tag [lrange $line 1 end]"
      :$tag [lrange $line 1 end]
    }

    :method parse@text {line} {
#      puts stderr "ADDLINE([current]) :@doc add $line end"
      :@doc add $line end
    }
    :method parse@space {line} {;}
    
    #
    # so far, we only need enter and exit handlers at the level of the
    # superstates: context, description, part
    #
    :public method on_enter {line} {;}
    :public method on_exit {line} {;}
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
	      if {[::nsf::is object $partof_entity]} {
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
	    if {[$partof_entity info lookup methods -source application $tag] eq ""} {
	      ${:block_parser} cancel INVALIDTAG "The tag '$tag' is not supported for the entity type
		'[namespace tail [$partof_entity info class]]'"
	      # [InvalidTag new -message [subst {
	      # 	The tag '$tag' is not supported for the entity type
	      # 	'[namespace tail [$partof_entity info class]]'
	      # }]] throw	      
	    }
	    # puts stderr "$partof_entity $tag $nq_name {*}$args"
	    set current_entity [$partof_entity $tag $nq_name {*}$args]
	    
	  } else {
	    #
	    # TODO: @object-method raises some issues (at least when
	    # processed without a resolved context = its partof entity).
	    # It is not an entity type, because it merely is a "scoped"
	    # @method. It won't resolve then as a proper instance of
	    # Tag, hence we observe an InvalidTag exception. For
	    # now, we just ignore and bypass this issue by allowing
	    # InvalidTag exceptions in analyze()
	    #
	    set qualified_tag [namespace qualifiers [current]]::$tag
	    ${:block_parser} cancel INVALIDTAG "The entity type '$tag' is not available"
	    # if {[Tag info instances -closure $qualified_tag] eq ""} {
	    #   [InvalidTag new -message [subst {
	    # 	The entity type '$tag' is not available
	    #   }]] throw 
	    # }
	    # puts stderr "$tag new -name $nq_name {*}$args"
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

	:method parse@tag {line} {
	  set args [lassign $line axes names]
	  set entity ${:partof_entity}
	  set axes [split [string trimleft $axes @] .]

	  # 1) get the parsing level from the comment block parser
	  set start_idx [lindex [lsearch -all -not -exact $axes ""] 0]
	  
	  set pl [${:block_parser} parsing_level]
	  if {$pl != $start_idx} {
	    ${:block_parser} cancel LEVELMISMATCH "Parsing level mismatch: Tag is meant for level '$start_idx', we are at '$pl'."
	  }
	  
	  # 2) stash away a number of empty axes according to the parsing level 
	  set axes [lrange $axes $pl end]
	  
	  lassign [::nx::doc::Tag normalise $axes $names] err res
	  if {$err} {
	     ${:block_parser} cancel STYLEVIOLATION $res
	  }

	  lassign $res tagpath names
	  
	  set leaf(axis) [lindex $tagpath end]
	  set tagpath [lrange $tagpath 0 end-1]
	  set leaf(name) [lindex $names end]
	  set names [lrange $names 0 end-1]
	  
	  lassign [::nx::doc::Tag find -strict $tagpath $names $entity] err res
	  if {$err} {
	    ${:block_parser} cancel INVALIDTAG $res
	  }
	  
	  set entity $res
	  
	  if {$entity eq ""} {
	    if {[QualifierTag info instances @$leaf(axis)] eq "" && [Tag info instances @$leaf(axis)] eq ""} {
	      ${:block_parser} cancel INVALIDTAG "The entity type '@$leaf(axis)' is not available."
	    }
	    set entity [@$leaf(axis) new -name $leaf(name) {*}$args]
	  } else {
	    if {[$entity info lookup methods -source application @$leaf(axis)] eq ""} {
okup())
	      ${:block_parser} cancel INVALIDTAG "The tag '$leaf(axis)' is not supported for the entity type '[namespace tail [$entity info class]]'"
	    }
	    set entity [$entity @$leaf(axis) [list $leaf(name) {*}$args]]
	  }
	  
	  ${:block_parser} current_entity $entity
	  ${:block_parser} processed_section [current class]
	  $entity current_comment_line_type ${:current_comment_line_type}
	  $entity block_parser ${:block_parser}
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
	text->space	parse
	space->text	parse
	space->space	parse
	space->tag	next
    } {
      
      :public method on_enter {line} {
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
	:public method on_enter {line} {
#	  puts stderr "ENTERING part $line, current section [${:block_parser} processed_section]"
	  unset -nocomplain :current_part
	  next
	}
	:method parse@tag {line} {
#	  puts stderr "PART parse@tag [current]"
	  set r [next]
#	  puts stderr GOT=$r
	  if {[::nsf::isobject $r] && [$r info has type ::nx::doc::Entity]} {
	    set :current_part $r
	  }
	  return $r
	}
	:method parse@text {line} {
#	  puts stderr "PART parse@text [current]"
	  if {[info exists :current_part]} {
#	    puts stderr "${:current_part} @doc add $line end"
	    ${:current_part} @doc add $line end
	  } else {
	    :event=next $line
	  }
	}
	# :method parse@space {line} {;}
      }
}

# puts stderr "Doc Tools loaded: [info command ::nx::doc::*]"