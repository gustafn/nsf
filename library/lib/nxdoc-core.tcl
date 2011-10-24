# @package nx::doc
# 
# The NXDoc infrastructure is built upon a representational model of
# NSF/NX code units; e.g., packages, commands, objects, and
# classes. This package declares the essential entities for
# representing NSF/NX programs, in terms of a special-purpose NSF/NX
# program. In addition, some utilities for implementing front- and
# backends for NXDoc are provided.
# 
# @author stefan.sobernig@wu.ac.at
# @require nx
# @version 1.0
# @namespace ::nx::doc
 
package provide nx::doc 1.0
namespace eval ::nx::doc {}

package require nx

namespace eval ::nx::doc {
  namespace import -force ::nx::*
  
  # @command ::nx::doc::@
  #
  # The helper proc "@" is a conveniant mean for creating new
  # documentation objects with minimal syntactic overhead.
  #
  # @parameter class Request an instance of a particular entity class (e.g., ...)
  # @parameter name What is the entity name (e.g., nx::doc for a package)
  # @parameter args A vector of arbitrary arguments, provided to the
  # entity when being constructed
  # @return The identifier of the newly created entity object
  
  proc @ {class name args} {$class new -name $name {*}$args}
 

  # @command ::nx::doc::sorted
  #
  # This utility proc is used to sort entities by values of a
  # specified attribute.
  #
  # @parameter instances 	Points to a list of entity instances
  # 				to sort e.g. <<@class ::nx::doc::@object>>
  # @parameter sortedBy 	Indicates the attribte name whose
  # 				values the sorting will be based on
  # @return 			A list of sorted documentation entity
  # 				instances <<@class ::nx::doc::@object>>
  proc sorted {instances sortedBy} {
    set order [list]
    foreach v $instances {lappend order [list $v [$v eval [list set :$sortedBy]]]}
    set result [list]
    foreach pair [lsort -index 1 $order] {lappend result [lindex $pair 0]}
    return $result
  }

  proc filtered {instances filteredBy} {
    set filtered [list]
    foreach v $instances { 
      if {[$v eval [list expr $filteredBy]]} {
	lappend filtered $v
      }
    }
    return $filtered
  }


  proc sortByValue {d} {
    set haystack [list]
    dict for {key value} $d {
      lappend haystack [list $key $value]
    }
    return [dict create {*}[concat {*}[lsort -integer -index 1 -decreasing $haystack]]]
  }
    
  proc findAssetPath {{subdir library/lib/nxdoc-assets}} {
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
    :property {prefix ""}
    :public method init {} {
      set :active_mixins [dict create]
      next
    }
    :public method apply {} {
      if {${:active_mixins} ne ""} {
	puts stderr "Warning: mixin layer has not been revoked!"
	set :active_mixins [dict create]
      }
      foreach mixin [:info children -type [current class]::Mixin] {
	set base "${:prefix}::[namespace tail $mixin]"
	if {[::nsf::object::exists $base]} {
	  set scope [expr {[$mixin scope] eq "object" && \
			       [$base info is class]?"class":""}]
	  dict lappend :active_mixins $base $mixin
	  $base {*}$scope mixin add $mixin
	}
      }
    }

    :public method revoke {} {
      dict for {base mixins} ${:active_mixins} {
	foreach m $mixins {
	  set scope [expr {[$m scope] eq "object" && \
			       [$base info is class]?"class":""}]
	  $base {*}$scope mixin delete $m
	}
      }
      set :active_mixins [dict create]
    }
    
    Class create [current]::Mixin -superclass Class {
      :property {scope class}
    }
  }

  Class create Tag -superclass Class {
    # A meta-class for named documenation entities. It sets some
    # shared properties (e.g., generation rules for tag names based on
    # entity class names, ...). Most importantly, it provides the
    # basic name-generating mechanisms for documentation entities
    # based on properties such as entity name, root namespace, etc.
    #
    # @param tag Defaults to the tag label to be used in comment
    # tags. It may vary from the auto-generated default!
    # @param root_namespace You may choose your own root-level
    # namespace hosting the namespace hierarchy of entity objects

    :property {tag {[string trimleft [string tolower [namespace tail [current]]] @]}}
    :property {root_namespace "::nx::doc::entities"}

    namespace eval ::nx::doc::entities {}

    :public class method normalise {tagpath names} {
      # 1) verify balancedness of path spec elements
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
    
    :public class method find {
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
	  set cmd [info command @$axis]
	  #
	  # TODO interp-aliasing objects under different command names
	  # is currently not transparent to some ::nsf::* helpers,
	  # such as ::nsf::object::exists. Do we need to tackle this?
	  #
	  if {$cmd ne ""} {
	    set cmd [namespace origin $cmd]
	    set target [interp alias {} $cmd]
	    if {$target ne ""} {
	      set cmd $target
	    }
	  }
	  if {$cmd eq "" || ![::nsf::object::exists $cmd] || ![$cmd info has type Tag]} {
	    return [list 1 "The entity type '@$axis' is not available."]
	  }
	  set entity [@$axis id $value]
	} else {
	  if {$strict && ![::nsf::object::exists $entity]} {
	    return [list 1 "The tag path '$tagpath' -> '$names' points to a non-existing documentation entity: '@$last_axis' -> '$last_name'"]
	  }
      if {$all} {lappend entity_path $entity [$entity name]}
	  set entity [$entity origin]
	  if {[$entity info lookup methods -source application @$axis] eq ""} {
	    return [list 1 "The tag '$axis' is not supported for the entity type '[namespace tail [$entity info class]]'"]
	  }
	  set entity [$entity @$axis id $value]
	  set last_axis $axis
	  set last_name $value
	}
      }

      if {$strict && $entity ne "" && ![::nsf::object::exists $entity]} {
	return [list 1 "The tag path '$tagpath' -> '$names' points to a non-existing documentation entity: '@$last_axis' -> '$last_name'"]
      }
      if {$all} {lappend entity_path $entity [$entity name]}


      return [list 0 [expr {$all?$entity_path:$entity}]]
    }
    
    # @class.method {Tag id}
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
	set name [string trimleft $name :]
	return "[:root_namespace]::${subns}::$name"
      }
    }

    :public method new {
      -part_attribute
      -partof:object,type=::nx::doc::Entity
      -name:any,required 
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
      namespace eval $id {}
      if {[::nsf::object::exists $id]} {
	$id configure {*}$args
      } else {
	set id [:create $id {*}$args]
      }
      return $id
    }

    # @class.method {Tag get_unqualified_name}
    #
    # @param qualified_name The fully qualified name (i.e., including the root namespace)
    :public method get_unqualified_name {qualified_name} {
      # TODO: danger, tcl-commands in comments
      # similar to \[namespace tail], but the "tail" might be an object with a namespace
      return [string trimleft [string map [list [:root_namespace] ""] $qualified_name] ":"]
    }
    :public method get_tail_name {qualified_name} {
      #return [string trimleft [string map [list ${:tag} ""] [:get_unqualified_name $qualified_name]]  ":"]
      return [join [lrange [concat {*}[split [:get_unqualified_name $qualified_name] "::"]] 1 end] "::"]
    }

    # / / / / / / / / / / / / / / / / / / / / / / / /
    # Manage chains of responsible container entities
    #
    # TODO: We don't need the stack-like dispensing of containers,
    # make it a simple one-element store

    :public class property containers:0..*,object,type=::nx::doc::ContainerEntity {
      set :incremental 1
    }

    :public method "containers empty" {} -returns boolean {
      return [[current class] eval {expr {![info exists :containers] || ![llength ${:containers}]}}]
    }

    :public method "containers peek" {} {
      if {![:containers empty]} {
	return [lindex [[current class] containers] end]
      }
    }

    :public method "containers push" {container:object,type=::nx::doc::ContainerEntity} {
      set prev [:containers peek]
      if {$prev ne ""} {
	$container previous $prev 
      }
      [current class] containers add $container end
    }

    :public method "containers reset" {{v ""}} {
      [current class] containers $v
    }

  }

  Class create QualifierTag -superclass Tag {
    :method get_fully_qualified_name {name} {
      if {![string match "::*" $name]} {
	set container [:containers peek]
	set ns [$container getAuthoritativeNS]
	set name ${ns}::$name 
      }
      return $name
    }

    :public method id {
      -partof_name
      {-scope ""} 
      name
    } {
      if {[info exists partof_name]} {
	next
      } else {
	set n [:get_fully_qualified_name $name]
	next $n
      }
    }

    :public method new {
      -part_attribute
      -partof:object,type=::nx::doc::Entity
      -name:any,required
      args
    } {
      set id_name $name
      if {[info exists partof]} {
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
      next [list -partof_name $partof_name -scope $scope -- $name]
    }

    :public method new {	       
      -part_attribute:required
      -partof:object,type=::nx::doc::Entity
      -name:any,required
      args
    } {
      :createOrConfigure [:id [$partof name] [$part_attribute scope] $name] {*}[current args]
    }
  }
  
  # @class ::nx::doc::PartAttribute
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

  nx::MetaSlot create PartAttribute -superclass ::nx::VariableSlot {
    # @.parameter part_class
    # 
    # The property refers to a concrete subclass of Part which
    # describes the parts being managed by the property.
    :property part_class:optional,class {
      :public method assign {domain prop value} {
	set owningClass [[$domain info parent] info parent]
	if {"::nx::doc::ContainerEntity" in [concat $owningClass [$owningClass info heritage]]} {
	  $value class mixin add ::nx::doc::ContainerEntity::Containable
	}
	next
      }
    }
    :property scope

    :property {pretty_name {[string totitle [string trimleft [namespace tail [current]] @]]}}
    :property {pretty_plural {[string totitle [string trimleft [namespace tail [current]] @]]}}

    # :forward owning_entity_class {% [[:info parent] info parent] }
    :method init args {
      :defaultmethods [list get append]
      :multiplicity 0..n 
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
	error "Requested id generation from a simple part property!"
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
		     [lrange $value 1 end]]
	#-@doc [lrange $value 1 end]]
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
      next [list $domain $prop [:require_part $domain $prop $value]]
    }
  }
  
  ::nx::MetaSlot create SwitchAttribute -superclass ::nx::VariableSlot {
    :public method init args {
      set :defaultmethods [list get get]
      next
    }
    :public method get {obj prop} {
      set def [expr {[info exists :default]?${:default}:0}]
      if {[$obj eval [list set :$prop]] == $def} {
	return [::nsf::var::set $obj $prop [expr {!$def}]]
      }
      return [next]
    }
  }

  #
  # Sketch the entire hierarchy of documentation entities
  # supported. Entity behaviour is defined further below
  #

  Class create Entity
  Class create StructuredEntity -superclass Entity
  Class create ContainerEntity -superclass StructuredEntity  
  Class create PartEntity -superclass Entity

  Tag create @glossary -superclass Entity
  Tag create @project -superclass ContainerEntity
  Tag create @package -superclass ContainerEntity
  QualifierTag create @command -superclass StructuredEntity  
  QualifierTag create @object -superclass StructuredEntity
  QualifierTag create @class -superclass @object

  PartTag create @method -superclass StructuredEntity
  PartTag create @param -superclass PartEntity



  Entity eval {
    #
    # Entity is the base class for the documentation classes
    #

    # @.parameter name
    #
    # gives you the name (i.e., the Nx object identifier) of the documented entity
    :property name:any,required

    :class property current_project:object,type=::nx::doc::@project,0..1
    :public forward current_project [current] %method

    :property partof:object,type=::nx::doc::StructuredEntity
    :property part_attribute:object,type=::nx::doc::PartAttribute


    :public method get_fqn_command_name {} {
      return ${:name}
    }

    #
    # TODO: the pdata/pinfo/validate combo only makes sense for
    # entities which reflect Tcl program structures -> refactor into a
    # dedicated PEntity class or the like
    #
    
    :property pdata

    :public method "pinfo get" {{-default ?} args} {
      if {![info exists :pdata] || ![dict exists ${:pdata} {*}$args]} {
	return $default;
      }
      dict get ${:pdata} {*}$args
    } 
    
    :public method "pinfo exists" args {
      if {![info exists :pdata]} {return 0}
      dict exists ${:pdata} {*}$args
    }
    
    :public method "pinfo lappend" args {
      if {![info exists :pdata]} return;
      dict lappend :pdata {*}$args
    }

    :public method "pinfo set" args {
      if {![info exists :pdata]} return;
      dict set :pdata {*}$args
    }

    
    :public method "pinfo propagate" args {
      set path [dict create {*}[:get_upward_path \
				-attribute {set :name}]]
      foreach p [lreverse [dict keys $path]] {
	#
	# For now, we disallow upstream propagation if the receiving
	# entity is missing ... as this would be pointless ...
	#
	if {[$p pinfo get -default "extra" status] eq "missing"} break;
	$p pinfo set {*}$args
      }
    }
    
    :public method get_upward_path {
      -relative:switch 
      {-attribute {set :name}}
      {-type ::nx::doc::Entity}
    } {
      set path [list]
      if {!$relative} {
	lappend path [list [current] [:eval $attribute]]
      }
      
      if {[info exists :partof] && [${:partof} info has type $type]} {	
	set path [concat [${:partof} [current method] -attribute $attribute -type $type] $path]
      }
      return [concat {*}$path]
    }

    :property @doc:0..* {
      set :incremental 1
      set :positional true
      set :position 1
    }

    :property -class ::nx::doc::PartAttribute @see

    :property -class ::nx::doc::SwitchAttribute @deprecated:boolean {
      set :default 0
    }
    :property -class ::nx::doc::SwitchAttribute @stashed:boolean {
      set :default 0
    }
    :property -class ::nx::doc::SwitchAttribute @c-implemented:boolean {
      set :default 0
    }

    :public method @property {props} {
      foreach prop $props {
	:@$prop
      }
    }

    :property @use {
      :public method assign {domain prop value} {
	lassign $value pathspec pathnames
	if {$pathnames eq ""} {
	  set pathnames $pathspec
	  set pathspec [dict create {*}[$domain get_upward_path \
					    -attribute {[:info class] tag}]]
	  set pathspec [dict values $pathspec]
	} else {
	  set pathspec [split $pathspec .]
	}
	lassign [::nx::doc::Tag normalise $pathspec $pathnames] err res
	if {$err} {
	  error "Invalid @use values provided: $res"
	}
	
	lassign $res pathspec pathnames
	lassign [::nx::doc::Tag find $pathspec $pathnames] err res
	if {$err} {
	  error "Generating an entity handle failed: $res"
	}
	next [list $domain $prop $res]
      }
      
    }

    :public method origin {} {
      if {[info exists :@use]} {
	if {![::nsf::object::exists ${:@use}] || ![${:@use} info has type [:info class]]} {
	  return -code error "Referring to a non-existing doc entity or a doc entity of a different type."
	}
	return [${:@use} origin]
      }
      return [current]
    }

    :public method as_list {} {
      if {[info exists :@doc] && ${:@doc} ne ""} {
	set non_empty_elements [lsearch -all -not -exact ${:@doc} ""]
	return [lrange ${:@doc} [lindex $non_empty_elements 0] [lindex $non_empty_elements end]]
      }
    }

    # @.method as_text
    #
    # text is used to access the content of doc of an Entity, and
    # performs substitution on it.  The substitution is not essential,
    # but looks for now convenient.

    :public method as_text {} {
      set doc [list]
      set lines [:as_list]
      foreach l $lines {
	lappend doc [string trimleft $l]
      }
      return [subst [join $doc " "]]
    }

    :public method error {msg} {
      return -code error "[current].[uplevel 1 [list ::nsf::current method]](): $msg"
    }

  }


  # @class @glossary
  @glossary eval {
    :property @pretty_name
    :property @pretty_plural
    :property @acronym
  }


  StructuredEntity eval {

    :public method part_attributes {} {
      set slots [:info lookup slots]
      set attrs [list]
      foreach s $slots {
	if {![$s info has type ::nx::doc::PartAttribute] || ![$s eval {info exists :part_class}]} continue;
	lappend attrs $s [$s part_class]
      }
      return $attrs
    }

    :public method owned_parts {
	{-class:object "::nx::Object"}
	-where
      } {
      set __owned_parts [dict create]
      foreach {s cls} [:part_attributes] {
	#
	# Note: For the time being, we skip over the bottom-most level of
	# entities, i.e. those which are not structured entities
	# themselves.
	#
	if {[info exists class] && \
		[[$s part_class] info superclass -closure $class] eq ""} continue;
	set accessor [$s name]
	if {[info exists :$accessor]} {
	  set items [sorted [:$accessor] name]
	  if {[info exists where]} {
	    set l [list]
	    foreach i $items {
	      if {[$i eval [list expr $where]]} {
		lappend l $i
	      }
	    }
	    set items $l
	  }
	  if {$items ne ""} {
	    dict set __owned_parts $s $items
	  }
	}
      }
      return $__owned_parts
    }

  }


  ContainerEntity eval {

    Class create [current]::Containable {
      :method create args {
    	#
    	# Note: preserve the container currently set at this callstack
    	# level.  [next] might cause another container to be pushed on
    	# top.
    	#
	set cont [:containers peek] 
	set obj [next]
	if {![$obj eval {info exists :partof}] && $cont ne ""} {
	  $cont register $obj
	}
	return $obj
      }
    }

    # Note: The default "" corresponds to the top-level namespace "::"!
    :property {@namespace ""}
    
    :property -class ::nx::doc::PartAttribute @class {
      :pretty_name "Class"
      :pretty_plural "Classes"
      :part_class ::nx::doc::@class
    }

    :property -class ::nx::doc::PartAttribute @object {
      :pretty_name "Object"
      :pretty_plural "Objects"
      :part_class ::nx::doc::@object
    }
   
    :property -class ::nx::doc::PartAttribute @command {
      :pretty_name "Command"
      :pretty_plural "Commands"
      :part_class ::nx::doc::@command
    }

    :public method register {containable:object,type=::nx::doc::Entity} {
      set tag [[$containable info class] tag]
      if {[:info lookup methods -source application "@$tag"] ne ""} {
	:@$tag $containable
      } elseif {[info exists :previous]} {
	${:previous} register $containable
      }
    }

    :property previous:object,type=[current]

    :public method announceAsContainer {tag:object,type=::nx::doc::Tag} {
      $tag containers push [current]
    }

    :public method getAuthoritativeNS {} {
      if {${:@namespace} eq "" && [info exists :previous]} {
	return ${:previous} [current method]
      } else {
	return ${:@namespace}; # defaults to top-level/global NS
      }
    }

    :protected method init args {
      next
      :announceAsContainer [:info class]
    }
    
  }

  @project eval {

    # / / / / / / / / / / / / / / / / / /
    # Doc entity interface
    # / / / / / / / / / / / / / / / / / /

    :public property sandbox:object,type=::nx::doc::Sandbox

    :property url
    :property license
    :property creationdate
    :property {version ""}
    
    :property {is_validated 0} 
    :property depends:0..*,object,type=[current]
    
    :property -class ::nx::doc::PartAttribute @glossary {
      :part_class ::nx::doc::@glossary
      :public method get {domain prop} {
	set l [next]
	if {[$domain eval {info exists :depends}]} {
	  foreach d [$domain depends] {
	    lappend l {*}[$d $prop]
	  }
	}
	return [lsort -unique $l]
      }
    }

    :property -class ::nx::doc::PartAttribute @package {
      :pretty_name "Package"
      :pretty_plural "Packages"
      :part_class ::nx::doc::@package
    }

    # / / / / / / / / / / / / / / / / / /
    # Frontend interface
    # / / / / / / / / / / / / / / / / / /

    :private method "frontend unknown" {m args} {
      :error "The NXDoc frontend '$m' is not available."
    }

    :public method read {frontend srcs cmds} -returns 0..*,object,type=::nx::doc::Entity {
      :frontend $frontend $srcs $cmds
    }

    :public class method newFromSources {
	{-frontend dc} 
	{-sandboxed:boolean 1}
	-include
	-exclude
	sources
	args
      } {

      #
      # Action 1)  Object creation
      #
      set newPrj [:new {*}$args]

      #
      # Action 2)  Initialise a sandbox
      #      
      set sandbox [$newPrj sandbox [Sandbox new -interp [expr {$sandboxed?[interp create]:""}]]]
          
      #
      # Action 3) Extract documentation sources (1pass)
      #    
      $sandbox do [$newPrj get1PassScript $sources]
      set sourceScripts [$sandbox getDocumentationScripts]
      
      #
      # Action 4) Determine command population through introspection (2pass)
      #
      $sandbox do [$newPrj get2PassScript $sourceScripts]


      #
      # Action 5) Applying command filters and obtain the workspace in
      # terms of commands ...
      #
      if {[info exists include] && [info exists exclude]} {
	$newPrj error "Inclusion and exclusion constraints are mutually exclusive!"
      }

      set nsFilters [list]
      if {[info exists include] && $include ne ""} {
	set nsFilters [list $include]
      }
      if {[info exists exclude] && $exclude ne ""} {
	set nsFilters [list -not $exclude]
      }

      set commandsFound [$sandbox getCommandsFound {*}$nsFilters]

      #
      # Action 6) Load the requested frontend extension
      #
      package req nx::doc::$frontend
      
      #
      # Action 7) Have the intended documentation entities processed
      # (documented, and meant to be visible)
      #
      # $newPrj readSrcs $frontend $sourceScripts $commandsFound
      $newPrj read $frontend $sourceScripts $commandsFound

      return $newPrj
    }

    :public method get1PassScript {sources} {
      set 1pass "::nx::doc::__trace_pkg\n"
      dict for {srcType items} $sources {
	if {![llength $items]} continue;
	switch -exact -- $srcType {
	  package {
	    foreach i $items {
	      append 1pass "package require $i\n" 
	    }
	  }
	  source {
	    foreach i $items {
	      append 1pass "source $i\n" 
	    }
	  }
	  eval {
	    error "Not implemented!"
	    # foreach i $items {
	    #   append 1pass "info script X-EVAL\n"
	    #   append 1pass "$i\n"
	    # }
	    # set srcType source
	    # set items X-EVAL
	  }
	  default {
	    error "Unsupported documentation source type '$srcType'"
	  }
	}
	${:sandbox} permissive lappend $srcType $items
      }
      return $1pass
    }

    :public method get2PassScript {sourceScripts} {
      set 2pass "::nx::doc::__init\n"
      dict for {id info} $sourceScripts {
	set block "%s"
	dict with info {
	  # Available vars:
	  #
	  # package
	  # path
	  # script
	  # dependency
	  #
	  if {$dependency || ![info exists script]} continue;

	  if {[info exists package]} {
	    set fragment "
	      ::nx::doc::__cpackage push $package;
	      %s 
	      ::nx::doc::__cpackage pop;
	    "
	    set block [format $block $fragment]
	    unset package
	  }

	  if {[info exists path]} {
	    set block [format $block "info script $path;\n%s"]
	    unset path
	  }
	}
	append 2pass [format $block $script]
	unset script
      }
      return $2pass
    }

    # / / / / / / / / / / / / / / / / / /
    # Backend interface
    # / / / / / / / / / / / / / / / / / /

    :public method write {{-format html} args} {
      package req nx::doc::$format
      $format run -project [current] {*}$args
    }


    # / / / / / / / / / / / / / / / / / /
    # Lifecycling
    # / / / / / / / / / / / / / / / / / /

    :public method destroy {} {
      #
      # TODO: Using the auto-cleanup feature in [Test case ...] does
      # not respect explicit destroy along object relations. Turn the
      # test environment more passive by checking for the existance
      # before calling destroy!
      #
      if {[info exists :sandbox] && [::nsf::object::exists ${:sandbox}]} {
	${:sandbox} destroy
      }
      :current_project ""
      next
    }

    :method init {} {
      #
      # TODO: the way we provide the project as a context object to
      # all entities is not easily restricted. Review later (e.g.,
      # relocate into the Validator) ...
      #
      [current class] containers reset
      :current_project [current]; # side effect: sets a per-class-object variable on Entity!
      next
    }

  }

  
  # TODO: decide how to deal with @package and @project names (don't
  # need namespace delimiters!)

  @package eval {
    :property -class ::nx::doc::PartAttribute @require
    :property @version
    :property -class ::nx::doc::PartAttribute @author 
    
  }

  @command eval {
    :property -class ::nx::doc::PartAttribute @parameter {
      :part_class ::nx::doc::@param
    }
    :property -class ::nx::doc::PartAttribute @return {
      :method require_part {domain prop value} {
	set value [expr {![string match ":*" $value] ? "__out__: $value": "__out__$value"}]
	next [list $domain $prop $value]
      }
      :part_class ::nx::doc::@param
    }

    :public forward @sub-command %self @command

    :property -class ::nx::doc::PartAttribute @command {
      :pretty_name "Subcommand"
      :pretty_plural "Subcommands"
      :public method id {domain prop value} { 
	return [${:part_class} [current method] \
		    -partof_name [$domain name] \
		    -scope ${:scope} -- $value]
      }
      :part_class ::nx::doc::@command
    }
  }
  
  @object eval {
    
    :public forward @object %self @child-object
    
    :property -class ::nx::doc::PartAttribute @child-object {
      :part_class ::nx::doc::@object
      :public method id {domain prop value} {
	return [${:part_class} id [join [list [$domain name] $value] ::]]
      }
      
    }
    
    :public forward @class %self @child-class
    
    :property -class ::nx::doc::PartAttribute @child-class {
      :part_class ::nx::doc::@class
      :public method id {domain prop value} {
	return [${:part_class} id [join [list [$domain name] $value] ::]]
      }
    }

	:public forward @method %self @object-method

	:property -class ::nx::doc::PartAttribute @object-method {
	  :pretty_name "Object method"
	  :pretty_plural "Object methods"
	  :part_class ::nx::doc::@method
	}

	:public forward @property %self @object-property
	#:forward @param %self @object-param

	:property -class ::nx::doc::PartAttribute @object-property {
	  :part_class ::nx::doc::@param
	}

	:method undocumented {} {
	  # TODO: for object methods and class methods
	  if {![::nsf::object::exists ${:name}]} {return ""}
	  foreach m [${:name} info methods -callprotection public] {set available_method($m) 1}
	  set methods ${:@method}
	  if {[info exists :@param]} {set methods [concat ${:@method} ${:@param}]}
	  foreach m $methods {
	    set mn [namespace tail $m]
	    if {[info exists available_method($mn)]} {unset available_method($mn)}
	  }
	  return [lsort [array names available_method]]
	}
      }

  @class eval {

	:property -class ::nx::doc::PartAttribute @superclass
	
	:public forward @property %self @class-property

	:property -class ::nx::doc::PartAttribute @class-property {
	  :pretty_name "Per-class attribute"
	  :pretty_plural "Per-class attributes"
	  :part_class ::nx::doc::@param
	}
	
	:public forward @class-object-method %self @object-method
	:public forward @class-object-property %self @object-property

	:public forward @hook %self @class-hook

	:property -class ::nx::doc::PartAttribute @class-hook {
	  :pretty_name "Hook method"
	  :pretty_plural "Hook methods"
	  :part_class ::nx::doc::@method
	}

	:public forward @method %self @class-method

	:property -class ::nx::doc::PartAttribute @class-method {
	  :pretty_name "Provided method"
	  :pretty_plural "Provided methods"
	  :part_class ::nx::doc::@method
	  :method require_part {domain prop value} {
	    # TODO: verify whether these scoping checks are sufficient
	    # and/or generalisable: For instance, is the scope
	    # requested (from the part_attribute) applicable to the
	    # partof object, which is the object behind [$domain name]?
	    
	    # TODO: disable for the moment ... how to rewrite to fit
	    # the sandboxed environment?
	    #	    if {[info exists :scope] && 
	    #		![::nsf::is ${:scope} [$domain name]]} {
	    #	      error "The entity '[$domain name]' does not qualify as '${:scope}'"
	    #	    }
	    next
	  }
	}
      }
  

  PartEntity eval {
    :property partof:object,type=::nx::doc::StructuredEntity,required
    :property part_attribute:object,type=::nx::doc::PartAttribute,required
  }
 

  # @class ::nx::doc::@method
  #
  # "@method" is a named entity, which is part of some other
  # docEntity (a class or an object). We might be able to use the
  # "use" parameter for registered aliases to be able to refer to the 
  # documentation of the original method.
  #
  @method eval {

	:property -class ::nx::doc::SwitchAttribute @syshook:boolean {
	  set :default 0
	}
	:property -class ::nx::doc::PartAttribute @parameter {
	  :part_class ::nx::doc::@param
	}
	:property -class ::nx::doc::PartAttribute @return {
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
	  :part_class ::nx::doc::@param
	}

	:public class method new {	       
	  -part_attribute:required
	  -partof:object,type=::nx::doc::Entity
	  -name:any,required
	  args
	} {
	  # 1) Are we in a sub-method?
	  if {[$partof info has type [current]]} {
	    :createOrConfigure [:id [:get_tail_name $partof] "" $name] {*}[current args]
	  } else {
	    next
	  }
	}
	


	:public forward @class-method %self @method
	:public forward @class-object-method %self @method
	:public forward @sub-method %self @method

	:property -class ::nx::doc::PartAttribute @method {
	  :part_class ::nx::doc::@method
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
	}

	:public method get_fqn_command_name {} {
	  set scope [expr {[${:part_attribute} scope] eq "class"?"classes":"objects"}]
	  return ::nsf::${scope}::[string trimleft [[:partof] name] :]::${:name}
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
	    return $leaves
	  }
	}

	:public method get_combined {what} {
	  set result [list]
	  if {[info exists :partof] && [${:partof} info has type [current class]]} {
	    set result [${:partof} get_combined $what]
	  }
	  return [lappend result [:$what]]
	}

	:public method get_owning_object {} {
	  return [[:get_owning_partof] name]
	}

	:public method get_owning_partof {} {
	  if {[${:partof} info has type [current class]]} {
	    return [${:partof} [current method]]
	  } else {
	    return ${:partof}
	  }
	}



      }; # @method
  
  # @class ::nx::doc::@param
  #
  # The entity type "@param" represents the documentation unit
  # for several parameter types, e.g., object, method, and
  # command parameters.
  #
  @param eval {
	:property -class ::nx::doc::PartAttribute @spec
	:property default

	:public class method id {partof_name scope name} {
	  next [list [:get_unqualified_name ${partof_name}] $scope $name]
	}
		
	# @class-object-method new
	#
	# The per-object method refinement indirects entity creation
	# to feed the necessary ingredients to the name generator
	#
	# @param -part_attribute 
	# @param -partof
	# @param -name
	# @param args

	:public class method new {
		-part_attribute 
		-partof:required
		-name:any,required
		args
	      } {
	  lassign $name name def
	  set spec ""
	  regexp {^(.*):(.*)$} $name _ name spec
	  :createOrConfigure [:id $partof [$part_attribute scope] $name] \
	      -@spec $spec \
	      -name $name \
	      -partof $partof \
	      {*}[expr {$def ne "" ? "-default $def" : ""}] \
	      -part_attribute $part_attribute {*}$args
	  
	}

	:public method get_fqn_command_name {} {
	  #
	  # TODO: For now, we only handle class properties
	  # (*::slot::*), per-object properties are excluded
	  # (*::per-object-slot::*). However, as per-object ones do
	  # not make it into the object parameters spec, this is fine
	  # for now ... review later.
	  #
	  if {[${:partof} info has type ::nx::doc::@object]} {
	    return "[${:partof} name]::slot::${:name}"
	  } else {
	    next
	  }
	}
      }

  #
  # Provide two interp-wide aliases for @param. This is mere syntactic
  # sugar!
  #
  interp alias {} ::nx::doc::@property {} ::nx::doc::@param
  interp alias {} ::nx::doc::@parameter {} ::nx::doc::@param

  #
  # Providing interp-wide aliases for @glossary. For most processing
  # steps, this is syntactic sugar, however, the aliases cause
  # different rendering behaviour for glossary references and entries.
  #

  interp alias {} ::nx::doc::@gls {} ::nx::doc::@glossary
  interp alias {} ::nx::doc::@Gls {} ::nx::doc::@glossary
  interp alias {} ::nx::doc::@glspl {} ::nx::doc::@glossary
  interp alias {} ::nx::doc::@Glspl {} ::nx::doc::@glossary
  interp alias {} ::nx::doc::@acr {} ::nx::doc::@glossary
  interp alias {} ::nx::doc::@acrfirst {} ::nx::doc::@glossary  

  namespace export CommentBlockParser @command @object @class @package \
      @project @method @property @parameter @ MixinLayer
}



namespace eval ::nx::doc {

  Class create TemplateData {
    
    :class property renderer
    :public forward renderer [current] %method

    :public forward rendered [current] %method
    :class method "rendered push" {e:object,type=::nx::doc::Entity} {
      if {![info exists :__rendered_entity]} {
	set :__rendered_entity [list]
      }
      set :__rendered_entity [concat $e {*}${:__rendered_entity}]
    }

    :class method "rendered pop" {} {
      set :__rendered_entity [lassign ${:__rendered_entity} e]
      return $e
    }

    :class method "rendered top" {} {
      return [lindex ${:__rendered_entity} 0]
    }

    :public method render_start {} {;}
    :public method render_end {} {;}
    
    # This mixin class realises a rudimentary templating language to
    # be used in nx::doc templates. It realises language expressions
    # to verify the existence of variables and simple loop constructs
    :public method render {
      {-initscript ""}
      -theme
      {name:substdefault "[::namespace tail [:info class]]"}
    } {
      :rendered push [current]
      # Here, we assume the -nonleaf mode being active for {{{[eval]}}}.
      # set tmplscript [list subst [:read_tmpl $template]]
      set tmplscript [list subst [[:renderer] getTemplate $name \
				      {*}[expr {[info exists theme]?$theme:""}]]]
      #
      # TODO: This looks awkward, however, till all requirements are
      # figured out (as for the origin mechanism) we so keep track
      # of the actual rendered entity ... review later ...
      #
      :render_start
      set content [:eval [subst -nocommands {
	$initscript
	$tmplscript
      }]]
      :render_end
      :rendered pop
      return [string trim $content \n]
    }
    
    
    #
    # some instructions for a dwarfish, embedded templating language
    #
    :method !let {var value} {
      # uplevel 1 [list ::set $var [expr {[info exists value]?$value:""}]]
      uplevel 1 [list ::set $var $value]
      return
    }
    
    :method ! {cmd args} {
      uplevel 1 [list ::$cmd {*}$args]
      return
    }
    
    :public method !get {-sortedby -with -where varname} {
      set origin [:origin]
      if {![$origin eval [list info exists :$varname]]} return
      if {[info exists sortedby]} { 
	set r [uplevel 1 [list ::nx::doc::sorted [$origin eval [list ::set :$varname]] $sortedby]]
      } else {
	set r [uplevel 1 [list $origin eval [list ::set :$varname] ]]
      }
      
      #
      # TODO: For now, we cannot distinguish between entities stashed
      # for the role as mere dependencies and those which are stashed
      # explicitly! Probably, we must introduce a decent dep state for
      # entities ... Again, review later ...
      # set where_clause "!\${:@stashed}"
      set where_clause "1"
      if {[info exists where]} {
	append where_clause "&& $where"
      }
      set l [list]
      foreach item $r {
	if {![::nsf::object::exists $item] || ![$item info has type ::nx::doc::Entity]} {
	  lappend l $item
	} else {
	  if {[[$item origin] eval [list expr $where_clause]]} {
	    lappend l $item
	  }
	}
      }
      set r $l  
      
      if {[info exists with]} {
	set l [list]
	foreach item $r {
	  lappend l [[$item origin] eval [list set :$with]] $item
	}
	set r $l
      }
      
      return $r
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
      uplevel 1 [list :? -ops [list [::nsf::current method] -] \
		     "\[$obj eval {info exists :$varname}\]" {*}$args]
    }

    :public method ?var {varname args} {
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
	return [uplevel 1 [list [current] $next {*}$args]]
      }
    }
    
    :method include {
      -theme
      {name:substdefault "[::namespace tail [:info class]]"}
    } {
      uplevel 1 [list subst [[:renderer] getTemplate $name \
				 {*}[expr {[info exists theme]?$theme:""}]]]
    }
    
    :method listing {{-inline true} script} {
      set iscript [join [list [list set inline $inline] [list set script $script]] \n]
      :render -initscript $iscript [current method]
    }
    
    :method link args {
      error "Subclass responsibility: You must provide a method definition of '[current method]' in a proper subclass"
    }
    
    set :markup_map(sub) { 
      "'''" "\[:listing \{" 
      "'''" "\}\]"
      "<<" "\[:link " 
      ">>" "\]" 
    }
    
    set :markup_map(unescape) {
      "\\{" "{"
      "\\}" "}"
      "\\#" "#"
      "\\<" "<"
      "\\>" ">"
      "\\'" "'"
    }
    
    :method unescape {line} {
      set line [string map [[::nsf::current class] eval [list set :markup_map(unescape)]] $line]
    }

    :method map {line} {
      regsub -all -- {('''([^']+?)''')} $line {[:listing {\2}]} line
      regsub -all -- {(<<([^<]+?)>>)} $line {[:link \2]} line
      return $line
    }

    :method as_list {} {
	set preprocessed [list]
	set is_code_block 0
	foreach line [next] {
	  if {(!${is_code_block} && [regsub -- {^\s*(''')\s*$} $line "\[:listing -inline false \{" line]) || \
		  (${is_code_block} && [regsub -- {^\s*(''')\s*$} $line "\}\]" line])} {
	    set is_code_block [expr {!$is_code_block}]
	    append line \n
	  } elseif {${is_code_block}} {
	    append line \n
	  } else {
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
      set preprocessed [:map $preprocessed]
      set preprocessed [:unescape $preprocessed]

      # TODO: For now, we take a passive approach: Some docstrings
      # might fail because they contain substitution characters
      # ($,[]); see nx.tcl. The same goes for legacy xodoc docstrings,
      # and their code listing (see langRef.xotcl). Catching
      # evaluations errors here makes it unnecessary to
      # escape/unescape evaluation chars; at the same time, we can't
      # distinguish errors on unintended and intended evaluations.
      # ...

      if {[catch {set preprocessed [subst $preprocessed]} msg]} {
	puts stderr SELF=[current]
	puts stderr MSG=$msg
	puts stderr IN->$preprocessed
	puts stderr errorInfo->$::errorInfo
      }
      return $preprocessed
    }

  }

  #
  # NXDoc backend infrastructure:
  # A Renderer base class ...
  #
  Class create Renderer -superclass MixinLayer {
    
    :property {extension "[namespace tail [current]]"}
    :property extends:object,type=[current]
    
    #
    # mixin-layer management
    #
    
    :method init args {
      set :prefix "::nx::doc"
      next
    }
    
    :public method apply {} {
      if {[info exists :extends]} {
	${:extends} [current method]
      }
      next
    }
    
    :public method revoke {} {
      next
      if {[info exists :extends]} {
	${:extends} [current method]
      }
    }
    
    #
    # template management
    #
    
    :property current_theme
    :protected property {templates {[dict create]}}
    
    :public method addTemplate {name theme body} {
      dict set :templates $theme $name $body
      return $body
    }
    :public method deleteTemplate {name theme} {
      dict remove ${:templates} $theme $name
    }
    :public method getTemplate {
	name 
	{theme:substdefault "${:current_theme}"}
      } {
      if {[dict exists ${:templates} $theme $name]} {
	return [dict get ${:templates} $theme $name]
      } else {
	#
	# 1) if available, read-in template file lazily
	#
	set templateName $name.${:extension}.$theme
	set body [:readAsset $templateName]
	if {$body ne ""} {
	  return [:addTemplate $name $theme $body]
	}
	#
	# 2) resolve the template along the "extends" chain
	#
	if {[info exists :extends]} {
	  return [${:extends} [current method] $name $theme]
	}
	#
	# 3) if ending up here, report a missing template!
	#
	error "The template '$templateName' requested for \
	        renderer '[namespace tail [current]]' is \
		not available."
      }
    }
				
    :method readAsset {assetName} {
      set assetDir [findAssetPath]
      set assetPath [file join $assetDir $assetName]
      return [:read $assetPath]
    }

    :method read {-binary:switch path} {
      if {[file exists $path] && [file isfile $path]} {
	set fh [open $path r]
	if {$binary} {
	  fconfigure $fh -encoding binary -translation binary
	}
	set body [read $fh]
	catch {close $fh}
	return $body
      }
    }

    #
    # rendering
    # 

    :method write {content path} {
      set fh [open $path a]
      puts $fh $content
      catch {close $fh}
    }
    :method remove {{-nocomplain:switch} path} {
      if {![file exists $path] && !$nocomplain} {
	error "Path does not exists: '$path'."
      }
      file delete -force $path
    }

    :method installAssets {project theme targetDir} {
      error "Not implemented. Instance responsibility!"
    }
    
    :method "layout many-to-1" {
      project 
      theme
      {-outdir [::nsf::tmpdir]}
    } {
      set fn [file join $outdir "[$project name].${:extension}"]
      :remove -nocomplain $fn
      
      set values [concat {*}[dict values [$project navigatable_parts]]]
      lappend values $project
      
      set output [list]
      foreach e $values {
	lappend output [:render $project $e $theme]
      }
      :write [join $output \n\n] $fn
      :installAssets $project $theme $fn
      puts stderr "$e written to $fn"
    }
    
    :method "layout many-to-many" {
      project 
      theme
      {-outdir [::nsf::tmpdir]}
    } {
      set ext ${:extension}
      
      #
      # 1) provide a per-project output directory
      #
      set project_path [file join $outdir [string trimleft [$project name] :]]
      :remove -nocomplain $project_path
      
      if {![catch {file mkdir $project_path} msg]} {
	#
	# 2) place the theme-specifc assets into the project directory
	#
	set target $project_path/assets
	:installAssets $project $theme $target

	#
	# 3) Set-up the list of entities to be processed. Note that in
	# this layout, the @project entity is processed along with all
	# the other entities, but *last*.
	#
	set values [concat {*}[dict values [$project navigatable_parts]]]
	lappend values $project

	foreach e $values {
	  #
	  # TODO: For now, in templates we (silently) assume that we act
	  # upon structured entities only ...
	  #
	  set content [:render $project $e $theme @project]
	  :write $content [file join $project_path "[$e filename].$ext"]
	  puts stderr "$e written to [file join $project_path [$e filename].$ext]"
	}
      }
    }

    :method "layout 1-to-1" {
      project
      theme
      {-outdir "[::nsf::tmpdir]"}
    } {      
      set ext ${:extension}
      set fn [file join $outdir "[$project name].$ext"]
      
      :remove -nocomplain $fn
      set content [:render $project $project $theme]
      :installAssets $project $theme $outdir
      :write $content $fn
      puts stderr "$project written to $fn"
    }

    :public method run {
	-project 
	{-layout many-to-many} 
	{-theme yuidoc} 
	args
      } {
      :apply
      :current_theme $theme
      :layout $layout $project $theme {*}$args
      :revoke
    }
    
    :method render {project entity theme {tmplName ""}} {
      error "Not implemented. Instance responsibility!"
    }
  }
}  

#
# sandboxing
#

namespace eval ::nx::doc {
  namespace import -force ::nx::*
  Class create Sandbox {

    :public class method type=in {name value arg} {
      if {$value ni [split $arg |]} {
	error "The value '$value' provided for parameter $name not permissible."
      }
      return $value
    }

    :public class method type=fqn {name value} {
      if {[string first "::" $value] != 0} {
	error "The value '$value' must be a fully-qualified Tcl name."
      }
      return $value
    }

    :public class method type=fpathtype {name value arg} {
      #
      # Note: We might receive empty strings in case of [eval]s!
      #
      set pt [file pathtype $value]
      if {$value ne "" && $pt ne $arg} {
	error "The filepath '$value' must be $arg, rather than $pt."
      }
      return $value
    }

    :public class method type=nonempty {name value} {
      if {$value eq ""} {
	return \
	    -code error \
	    "An empty value is not allowed for parameter '$name'."
      }
      return $value
    }

    :protected property {current_packages ""}

    :public method "permissive lappend" {type value} {
      set d [lindex [current methodpath] 0]
      dict [current method] :$d $type {*}$value
    }

    :public method "permissive get" {type} {
      if {![info exists :permissive]} {
	set :permissive [dict create]
      }
      dict [current method] ${:permissive} $type
    }

    :public method getDocumentationScripts {} {
      if {[info exists :dSources]} {
	return ${:dSources}
      }
    }

    #
    # some callbacks invoked from within the sandbox interp
    #
    :public method "cpackage pop" {} {
      set :current_packages [lrange ${:current_packages} 0 end-1]
    }
    :public method "cpackage push" {p} {
      lappend :current_packages $p
    }
    :public method "cpackage top" {} {
      if {[info exists :current_packages]} {
	return [lindex ${:current_packages} end]
      }
    }

    :public method at_source {filePath} {
      set cpackage [:cpackage top]
      set fh [open $filePath r]
      set script [read $fh] 
      catch {close $fh}

      set info [dict create]
      set key ""
      if {$cpackage ne ""} {
	set key "$cpackage."
	dict set info package $cpackage
	dict set info dependency [expr {$cpackage ni [:permissive get package]}]
      } else {
	# TODO: dict set info dependency [expr {$filePath ni [:permissive get source]}]
	dict set info dependency 1
      }
      dict set info path $filePath
      dict set info script $script
      
      dict set :dSources $key$filePath $info
    }

    :public method at_load {filepath} {
      set cpackage [:cpackage top]
      set key ""
      set info [dict create]
      dict set info path $filepath
      if {$cpackage ne ""} {
	set key "$cpackage."
	dict set info package $cpackage
	dict set info dependency [expr {$cpackage ni [:permissive get package]}]
      } else {
	# TODO: dict set info dependency [expr {$filePath ni [:permissive get source]}]
	dict set info dependency 1
      }
      dict set :dSources $key$filepath $info
  }


    :public method at_register_package {pkg_name version} {
      dict set :registered_packages $pkg_name version $version
    }

    :public method at_register_command [list \
	name:fqn,slot=[current] \
	->cmdtype:in,arg=@object|@class|@command|@method,slot=[current] \
   	->ownerPath:0..*,fqn,slot=[current] \
        ->source:fpathtype,arg=absolute,slot=[current] \
	{->nsexported:boolean 0} \
	{->nsimported:boolean 0} \
        ->docstring:optional,slot=[current] \
	->bundle
     ] {
      set storable_vars [info vars >*]
      foreach svar $storable_vars {
	dict set :registered_commands $name [string trimleft $svar >] [set $svar]
      }
      
      # peek the currently processed package (if any)
      set cpackage [:cpackage top]
      if {$cpackage ne ""} {
	dict set :registered_commands $name package $cpackage
	dict set :registered_commands $name dependency \
	    [expr {$cpackage ni [:permissive get package]}]
      } else {
	# FIXME dict set :registered_commands $name dependency \
	#    [expr {$source ni [:permissive get source]}]
	dict set :registered_commands $name dependency 1
      }
    }

    :public method at_deregister_command [list name:fqn,slot=[current]] {
      dict unset :registered_commands $name
    }

    :public method init args {
      :do {
	#
	# TODO: refactor the proxy handling ...
	#
	interp hide "" proc
	interp hide "" namespace
	interp hide "" source
	interp hide "" load
	interp hide "" package
	interp hide "" auto_import

	interp invokehidden "" proc ::proc args {
	  uplevel [list interp invokehidden "" proc {*}$args]
	}

	proc ::namespace args {
	  # set ns [uplevel [list interp invokehidden "" namespace current]]
	  uplevel [list interp invokehidden "" namespace {*}$args]
	  # interp invokehidden {} -namespace $ns namespace {*}$args
	}

	proc ::source args {
	  uplevel [list interp invokehidden "" source {*}$args]
	}

	proc ::load args {
	  # set ns [uplevel [list interp invokehidden "" namespace current]]
	  # interp invokehidden "" -namespace $ns load {*}$args
	  uplevel [list interp invokehidden "" load {*}$args]

	}

	proc ::package args {
	  # set ns [uplevel [list interp invokehidden "" namespace current]]
	  # interp invokehidden "" -namespace $ns package {*}$args
	  uplevel [list interp invokehidden "" package {*}$args]
	}

	proc ::auto_import args {
	  # set ns [uplevel [list interp invokehidden "" namespace current]]
	  # interp invokehidden "" -namespace $ns auto_import {*}$args
	  uplevel [list interp invokehidden "" auto_import {*}$args]
	}
	
	namespace eval ::nx::doc {
	  
	  proc is_exported {name} {
	    #
	    # ! ISSUE: The built-in [namespace] command is hidden in our
	    # ! sandbox interp when [is_exported] is used during a
	    # ! 2pass!!!!
	    #
	    set calling_ns [uplevel [list interp invokehidden "" namespace current]]
	    set ns [interp invokehidden "" namespace current]::_?_
	    interp invokehidden "" namespace eval $ns \
		[list interp invokehidden "" namespace import -force $name]
	    set is [expr {[info commands ${ns}::[interp invokehidden "" namespace tail $name]] ne ""}]
	    interp invokehidden "" namespace delete $ns
	    return $is
	  }

	  ::interp invokehidden "" proc ::nx::doc::paraminfo {
		value {default ""}
	      } {
	    set colon [string first : $value]
		set spec ""
		if {$colon == -1} {
		  set name $value
		} else {
		  set spec [string range $value [expr {$colon+1}] end]
		  set name [string range $value 0 [expr {$colon -1}]]
		}
		return [list $name [list $spec $default]]
	      }

	  proc __trace_pkg {} {

	    #    ::interp hide "" source
	    ::proc ::source {path} {
	      set ns [uplevel [list namespace current]]
	      if {[file tail $path] ne "pkgIndex.tcl"} {
		::nx::doc::__at_source [file normalize $path]
	      }
	      uplevel [list interp invokehidden "" source $path]
	    }
	    
	    proc list_commands {{parent ""}} {
	      set ns [dict create]
	      #
	      # Note: We trigger a [namespace import] for the
	      # currently processed namespace before requesting the
	      # command list in order to have the auto_load feature
	      # initialise commands otherwise found too late,
	      # i.e. after having computed the [info
	      # commands] snapshot!
	      #

	      set cmds [info commands ${parent}::*]

	      set exported [list]
	      foreach cmd $cmds {
		dict set ns ::[string trimleft $parent :] $cmd [is_exported $cmd]
	      }

	      foreach nsp [namespace children ${parent}::] {
		set ns [dict merge $ns [list_commands ${nsp}]]
	      }
	      return $ns
	    }


	    ::proc ::load args {

	      set ns [uplevel [list namespace current]]

	      # 
	      # pre-state
	      #
	      set pre_loaded [lreverse [concat {*}[info loaded ""]]]
	      set pre [::nx::doc::list_commands]
	      set pre_commands [dict create {*}[concat {*}[dict values $pre]]]
	      set pre_namespaces [dict keys $pre]

	      interp invokehidden "" -namespace $ns load {*}$args
	      
	      #
	      # post-state
	      #
	      set post_loaded [lreverse [concat {*}[info loaded ""]]]
	      set post [::nx::doc::list_commands]
	      set post_commands [dict create {*}[concat {*}[dict values $post]]]
	      set post_namespaces [dict keys $post]
	      
	      #
	      # deltas
	      #
	      set delta_commands [dict remove $post_commands {*}[dict keys $pre_commands]]

	      set delta_namespaces [dict keys [dict remove [dict create {*}"[join $post_namespaces " _ "] _"] {*}$pre_namespaces]]

	      set delta_pkg [dict remove \
				 [dict create {*}$post_loaded] \
				 [dict keys [dict create {*}$pre_loaded]]]
	      
	      lassign $delta_pkg pkg_name filepath
	      set filepath [file normalize $filepath]

	      ::nx::doc::__at_load $filepath

	      # TODO: Temporary hack to reflect that we provide for a
	      # helper objsys to retrieve command parameter specs and
	      # parametersyntax prints.
	      if {[info commands ::nsf::objectsystem::create] ne "" && \
		      [::nsf::configure objectsystem] eq ""} {
		namespace eval ::nx::doc::_%& {}
		set rootclass ::nx::doc::_%&::obj
		set rootmclass ::nx::doc::_%&::cls
		::nsf::objectsystem::create $rootclass $rootmclass
	      } else {
		lassign {*}[::nsf::configure objectsystem] rootclass rootmclass
	      }

	      foreach {cmd isexported} $delta_commands {
		set bundle [dict create]
		set infoMethod ::nsf::methods::object::info::method
		if {[::nsf::object::exists $cmd]} {
		  #
		  # TODO: Only classes are provided with parametersyntax
		  # info. Is this sufficient?!
		  #
		  if {[::nsf::is class $cmd]} {

		    dict set bundle parametersyntax [::nsf::dispatch $cmd \
			::nsf::methods::class::info::objectparameter \
			parametersyntax]
		    #
		    # TODO: Are the parameters needed for objects???
		    #
		    # dict set bundle parameter [::nsf::dispatch $cmd \
		    # 	::nsf::methods::class::info::objectparameter \
		    # 	parameter]
		  }
		} else {
		  if {![catch {set syntax [::nsf::dispatch $rootclass $infoMethod \
			 parametersyntax $cmd]} _]} {
		    dict set bundle parametersyntax $syntax
		  }
		  
		  if {![catch {set pa [::nsf::dispatch $rootclass $infoMethod \
					parameter $cmd]} _]} {
		    foreach pspec $pa {
		      dict set bundle parameter {*}[::nx::doc::paraminfo {*}$pspec]
		    }
		  }
		}

		::nx::doc::__at_register_command $cmd \
		    ->cmdtype @command \
		    ->source $filepath \
		    ->nsexported $isexported \
		    ->bundle $bundle
	      }
	    }

	    ::proc ::package {subcmd args} {
	      set ns [uplevel [list namespace current]]
	      set was_registered 0
	      switch -glob -- $subcmd {
		ifneeded {
		  lassign $args pkg_name version script
		  append wrapped_script "::nx::doc::__cpackage push $pkg_name;\n" $script "\n::nx::doc::__cpackage pop;"
		  set args [list $pkg_name $version $wrapped_script]
		  ::nx::doc::__at_register_package $pkg_name $version
		}
	      } 
	      interp invokehidden "" -namespace $ns package $subcmd {*}$args
	    }

	    #
	    # Note that we have to wrap up Tcl's auto_import due to
	    # our practise of [namespace import]'ing application
	    # namespaces to verify whether commands are actually
	    # exported; see list_commands. Currently, we escape to a
	    # generic package called TCL_LIBRARY to filter out
	    # commands lazily acquired through the auto_load
	    # mechanism, triggered by the [namespace import]
	    # probing.
	    #
	    #::interp hide "" auto_import
	    ::proc ::auto_import {pattern} {
	      set ns [uplevel [list namespace current]]
	      ::nx::doc::__cpackage push TCL_LIBRARY;
	      interp invokehidden "" -namespace $ns auto_import $pattern
	      ::nx::doc::__cpackage pop;
	    }
	  }
	  proc __init {} {
	    # 1) provide for tracing NSF objects
	    if {[info commands ::nsf::configure] ne "" && \
		    [::nsf::configure objectsystem] ne ""} {
	      set objsys [lindex [::nsf::configure objectsystem] 0]
	      set m [lassign $objsys rootclass rootmclass]
	      #
	      # TODO: Temporary hack to reflect that we provide for a
	      # helper objsys to retrieve command parameter specs and
	      # parametersyntax prints.
	      # 
	      if {$rootclass ne "::nx::doc::_%&::obj"} {
		
		::nsf::configure keepinitcmd true;
	      
	      array set sysmeths [concat {*}$m]
	      set ::nx::doc::rootns [namespace qualifier $rootmclass]
	      $rootmclass $sysmeths(-class.create) ${::nx::doc::rootns}::__Tracer
	      ::nsf::method::create ${::nx::doc::rootns}::__Tracer \
		  $sysmeths(-class.create) {name args} {
		    set obj [::nsf::next];
		    set bundle [dict create]
		    if {[info commands "::nx::Class"] ne ""} {
		      if {[::nsf::dispatch $obj ::nsf::methods::object::info::hastype ::nx::Slot]} {
			dict set bundle objtype slot
			dict set bundle incremental [expr {[::nsf::dispatch $obj ::nsf::methods::object::info::hastype ::nx::RelationSlot] || ([::nsf::dispatch $obj ::nsf::methods::object::info::hastype ::nx::VariableSlot] && [::nsf::var::exists $obj incremental] && [::nsf::var::set $obj incremental])}]
		      }
		      if {[::nsf::dispatch $obj ::nsf::methods::object::info::hastype ::nx::EnsembleObject]} {
			dict set bundle objtype ensemble
		      }
		      dict set bundle ismetaclass [::nsf::is metaclass $obj]
		    }
	      	    set cmdtype [expr {[::nsf::is class $obj]?"@class":"@object"}]
	      	    ::nx::doc::__at_register_command $obj \
	      		->cmdtype $cmdtype \
	      		->source [file normalize [info script]] \
	      		->nsexported [::nx::doc::is_exported $obj] \
			->bundle $bundle \
	      		{*}[expr {[::nsf::var::exists $obj __initcmd] && [::nsf::var::set $obj __initcmd] ne ""?[list ->docstring [::nsf::var::set $obj __initcmd]]:[list]}]
	      	    return $obj
	      	  }
	      
	      if {[info commands "::nx::Object"] ne ""} {
		$rootmclass $sysmeths(-class.create) ${::nx::doc::rootns}::__ObjTracer
		::nsf::method::create ${::nx::doc::rootns}::__ObjTracer \
		    __resolve_method_path {
		      -per-object:switch 
		      -verbose:switch 
		      path
		    } {
		      array set "" [::nsf::next]
		      set l [llength $path]
		      if {$l > 1} {
			set target $(object)
			set objects [list]
			for {set j 1} {$j < [expr {$l-1}]} {incr j} {
			  set target [namespace qualifiers $target]
			  lappend objects $target
			}
			lappend objects [::nsf::current object]
			set first 1
			foreach leg [lrange $path 0 end-1] obj [lreverse $objects] {
			  if {$first} {
			    set scope [expr {${per-object}?"object":"class"}]
			    set first 0
			  } else {
			    set scope object
			  }
			 
			  set handle [::nsf::dispatch $obj \
					  ::nsf::methods::${scope}::info::method \
					  handle $leg]
			  if {![::nsf::var::exists [::nsf::current class] handles] || ![[::nsf::current class] eval [concat dict exists \${:handles} $handle]]} {
			    dict set bundle handle $handle
			    dict set bundle handleinfo [::nx::doc::handleinfo $handle]
			    dict set bundle type [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::${scope}::info::method type $handle]
			    if {![catch {set pa [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::${scope}::info::method parameter $handle]} _]} {
			      foreach pspec $pa {
				dict set bundle parameter {*}[::nx::doc::paraminfo {*}$pspec]
			      }
			    }
			    if {![catch {set psyn [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::${scope}::info::method parametersyntax $handle]} _]} {
			      dict set bundle parametersyntax $psyn
			    }
			    ::nx::doc::__at_register_command $handle \
				->cmdtype @method \
				->ownerPath $obj \
				->source [file normalize [info script]] \
				->bundle $bundle
			    [::nsf::current class] eval [list dict set :handles $handle _]			    
			  }
			}
		      }
		      
		      return [array get ""]
		    }
		::nsf::mixin $rootclass ${::nx::doc::rootns}::__ObjTracer
		#::nsf::relation $rootclass class-mixin ${::nx::doc::rootns}::__ObjTracer
	      }

		::nsf::mixin $rootmclass ${::nx::doc::rootns}::__Tracer

	      }
	      ::interp invokehidden "" proc ::nx::doc::handleinfo {handle} {
		set definition [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method definition $handle]
		if {$definition ne ""} {
		  set obj [lindex $definition 0]
		  set modifier [lindex $definition 2]
		  if {[lindex $definition 1] eq "create"} {
		    set obj $modifier 
		    set scope ""
		    set name ""
		  } elseif {$modifier eq "class"} {
		    set scope $modifier
		    set name [lindex $definition 4]
		  } else {
		    set scope ""
		    set name [lindex $definition 3]
		  }
		}
		if {$scope eq ""} {
		  set is_method 0
		  set obj [concat {*}[split [string trimleft $obj :] "::"]]
		  foreach label $obj {
		    if {$label eq "slot"} {set is_method 1; continue;}
		    if {$is_method} {
		      lappend method_name [string trimleft $label _]
		    } else {
		      lappend obj_name $label
		    }
		  }
		  set name [lappend method_name {*}$name]
		  set obj ::[join $obj_name "::"]
		}
		return [list $obj $scope $name]
	      }
	     

	      rename ::nsf::method::create ::nsf::_%&method 
	      ::interp invokehidden "" proc ::nsf::method::create {
		object
		args
	      } {
		set object [uplevel [list namespace which $object]]
		set handle [uplevel [list ::nsf::_%&method $object {*}$args]]
		if {$handle ne ""} {
		  set bundle [dict create]
		  dict set bundle handle $handle
		  dict set bundle handleinfo [::nx::doc::handleinfo $handle]
		  foreach pspec [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method parameter $handle] {
		    dict set bundle parameter {*}[::nx::doc::paraminfo {*}$pspec]
		  }
		  dict set bundle parametersyntax [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method parametersyntax $handle]
		  dict set bundle type [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method type $handle]
		  dict set bundle returns [::nsf::method::property ${::nx::doc::rootns}::__Tracer $handle returns]
		  ::nx::doc::__at_register_command $handle \
		      ->cmdtype @method \
		      ->ownerPath $object \
		      ->source [file normalize [info script]] \
		      ->bundle $bundle
		} 
		return $handle
	      }

	      rename ::nsf::method::alias ::nsf::_%&alias 
	      ::interp invokehidden "" proc ::nsf::method::alias {
		object args
	      } {
		set object [uplevel [list namespace which $object]]
		# set object [uplevel [list $object]]; crashes
		set handle [uplevel [list ::nsf::_%&alias $object {*}$args]]
		if {$handle ne ""} {
		  dict set bundle handle $handle
		  dict set bundle handleinfo [::nx::doc::handleinfo $handle]
		  dict set bundle returns [::nsf::method::property ${::nx::doc::rootns}::__Tracer $handle returns]
		  dict set bundle type [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method type $handle]
		  if {![catch {set pa [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method parameter $handle]} _]} {
		    foreach pspec $pa {
		      dict set bundle parameter {*}[::nx::doc::paraminfo {*}$pspec]
		    }
		  }
		  if {![catch {set psyn [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method parametersyntax $handle]} _]} {
		    dict set bundle parametersyntax $psyn
		  }

		  ::nx::doc::__at_register_command $handle \
		      ->cmdtype @method \
		      ->ownerPath $object \
		      ->source [file normalize [info script]] \
		      ->bundle $bundle
		} 
		return $handle
	      }

	      rename ::nsf::method::forward ::nsf::_%&forward 
	      ::interp invokehidden "" proc ::nsf::method::forward {
		object args
	      } {
		set object [uplevel [list namespace which $object]]
	      	set handle [uplevel [list ::nsf::_%&forward $object {*}$args]]
	      	if {$handle ne ""} {
	      	  dict set bundle handle $handle
		  dict set bundle handleinfo [::nx::doc::handleinfo $handle]
	      	  dict set bundle type [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method type $handle]
		  if {![catch {set psyn [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method parametersyntax $handle]} _]} {
		    dict set bundle parametersyntax $psyn
		  }

	      	  ::nx::doc::__at_register_command $handle \
	      	      ->cmdtype @method \
		      ->ownerPath $object \
	      	      ->source [file normalize [info script]] \
	      	      ->bundle $bundle
	      	} 
		return $handle
	      }

	      rename ::nsf::method::setter ::nsf::_%&setter
	      ::interp invokehidden "" proc ::nsf::method::setter {
	      	object args
	      } {
		set object [uplevel [list namespace which $object]]
	      	set handle [uplevel [list ::nsf::_%&setter $object {*}$args]]
	      	if {$handle ne ""} {
	      	  dict set bundle handle $handle
		  dict set bundle handleinfo [::nx::doc::handleinfo $handle]
	      	  dict set bundle type [::nsf::dispatch ${::nx::doc::rootns}::__Tracer ::nsf::methods::object::info::method type $handle]
		  
		  ::nx::doc::__at_register_command $handle \
	      	      ->cmdtype @method \
		      ->ownerPath $object \
	      	      ->source [file normalize [info script]] \
		      ->bundle $bundle
	      	}
		return $handle
	      }

	    rename ::nsf::objectsystem::create ::nsf::_%&createobjectsystem 
	      ::interp invokehidden "" proc ::nsf::objectsystem::create {
		rootclass 
		rootmclass 
		args
	      } {
		uplevel [list ::nsf::_%&createobjectsystem $rootclass $rootmclass {*}$args]
		
		foreach r [list $rootclass $rootmclass] {
		  dict set bundle ismetaclass [::nsf::is metaclass $r]
		  ::nx::doc::__at_register_command $r \
		      ->cmdtype @class \
		      ->source [file normalize [info script]] \
		      ->nsexported [::nx::doc::is_exported $r] \
		      {*}[expr {[::nsf::var::exists $r __initcmd] && [::nsf::var::set $obj __initcmd] ne ""?[list ->docstring [::nsf::var::set $r __initcmd]]:[list]}] \
		      ->bundle $bundle
		}
	      }

	    }
	    # 2a) provide for tracing Tcl procs declared at "sourcing time" -> [proc]
	    #::interp hide "" proc
	    ::interp invokehidden "" proc ::proc {name arguments body} {
	      set ns [uplevel [list namespace current]]
	      interp invokehidden "" -namespace $ns proc $name $arguments $body
	      set fqn $name
	      if {[string first "::" $name] != 0} {
		set fqn [string trimright $ns :]::$name
	      }
	      if {$arguments eq "" && $body eq ""} {
		::nx::doc::__at_deregister_command $fqn
	      } else {
		::nx::doc::__at_register_command $fqn \
		    ->cmdtype @command \
		    ->source [file normalize [info script]] \
		    ->nsexported [::nx::doc::is_exported $fqn] \
		    ->docstring $body
	      }

	    }
	    
	    if {[info commands ::nsf::proc] ne ""} {
	      rename ::nsf::proc ::nsf::_%&proc
	      ::interp invokehidden "" proc ::nsf::proc {name arguments body} {
		set ns [uplevel [list namespace current]]
		uplevel [list ::nsf::_%&proc $name $arguments $body]	      
		set fqn $name
		if {[string first "::" $name] != 0} {
		  set fqn [string trimright $ns :]::$name
		}
		if {$arguments eq "" && $body eq ""} {
		  ::nx::doc::__at_deregister_command $fqn
		} else {
		  ::nx::doc::__at_register_command $fqn \
		      ->cmdtype @command \
		      ->source [file normalize [info script]] \
		      ->nsexported [::nx::doc::is_exported $fqn] \
		    ->docstring $body
		}
	      }
	    }
	      
	    # 3) provide for tracing commands namespace-imported at "sourcing time"
	    #::interp hide "" namespace
	    ::interp invokehidden "" proc ::namespace {subcmd args} {
	      set ns [uplevel [list interp invokehidden "" namespace current]]
	      switch -glob -- $subcmd {
		imp* {
		  foreach pattern $args {
		    if {[string match "-*" $pattern]} continue;
		    foreach cmd [info commands $pattern] {
		      if {![::nx::doc::is_exported $cmd]} continue;
		      set type @command
		      if {[info commands ::nsf::object::exists] ne "" &&\
			      [::nsf::object::exists $cmd]} {
			set type [expr {[::nsf::is class $cmd]?"@class":"@object"}]
		      }
		      set imported_name [string trimright $ns :]::[namespace tail $cmd]
		      ::nx::doc::__at_register_command $imported_name \
			  ->cmdtype $type \
			  ->source [file normalize [info script]] \
			  ->nsexported [::nx::doc::is_exported $imported_name] \
			  ->nsimported 1
		    }
		  }
		}
	      } 
	      interp invokehidden "" -namespace $ns namespace $subcmd {*}$args
	    }
	  }
	}
      }
      ::interp alias ${:interp} ::nx::doc::__at_register_command \
	  "" [current] at_register_command
      ::interp alias ${:interp} ::nx::doc::__at_deregister_command \
	  "" [current] at_deregister_command
      ::interp alias ${:interp} ::nx::doc::__cpackage \
	  "" [current] cpackage
      ::interp alias ${:interp} ::nx::doc::__at_register_package \
	  "" [current] at_register_package
      ::interp alias ${:interp} ::nx::doc::__at_source \
	  "" [current] at_source
      ::interp alias ${:interp} ::nx::doc::__at_load \
	  "" [current] at_load

      next
    }
    :protected property {interp ""}; # the default empty string points to the current interp

    :public property registered_commands

    :public method get_companions {} {
      set companions [dict create]
      dict for {cmd props} ${:registered_commands} {
	dict with props {
	  # $source, $package
	  dict set companions $source $package
	}
      }
      return [:getCompanions $companions]
    }

    :public method getCommandsFound {
	-exported:boolean
        -imported:boolean
	-types
	-not:switch
	nspatterns:optional
      } {

      if {![info exists :registered_commands]} return;
      if {[info exists nspatterns]} {
	set opts [join $nspatterns |]
	set nspatterns "^($opts)\$"
      }
      dict filter ${:registered_commands} script {cmd props} {
	dict with props {
	  #
	  # TODO: Depending on the include/exclude semantics intended,
	  # we could filter for the entire ownership path ... However,
	  # this would require changing the index file usage. For now,
	  # we cannot include an object and exclude a single method
	  # ... a feature currently needed. An option is to switch to
	  # exclude semantics ... Review later, when the release dust
	  # settles ...
	  #
	  #if {[info exists ownerPath]} {
	  #  lappend cmd {*}$ownerPath
	  #}
	  expr {[expr {[info exists nspatterns]?[expr {[expr {[lsearch -regexp $cmd $nspatterns] > -1}] != $not}]:1}] && \
		    [expr {[info exists exported]?[expr {$nsexported == $exported}]:1}] && \
		    [expr {[info exists imported]?[expr {$nsimported == $imported}]:1}] && \
			   [expr {[info exists types]?[expr {$cmdtype in $types}]:1}]}
	}
      }
  }

    :public method do {script} {
      ::interp eval ${:interp} $script
    } 

    :public method destroy {} {
      if {${:interp} ne ""} {
	if {[interp exists ${:interp}]} {
	  interp delete ${:interp}
	}
      } else {
	#
	# TODO: complete the coverage of the cleanup ...
	#
	:do {
	  if {[info commands ::nsf::configure] ne ""} {
	    ::nsf::configure keepinitcmd false;
	    array set sysmeths [concat {*}[lassign {*}[::nsf::configure objectsystem] rootclass rootmclass]]
	    # TODO: some cleanup is only needed if __init has been called
	    # (which is not always the case). refactor the code
	    # accordingly.
	    set ::nx::doc::rootns [namespace qualifier $rootmclass]
	    if {[::nsf::object::exists ${::nx::doc::rootns}::__Tracer]} {
	      ${::nx::doc::rootns}::__Tracer $sysmeths(-object.destroy)
	      ::nsf::relation $rootmclass class-mixin {}
	    }
	    if {[info commands ::nsf::_%&createobjectsystem] ne ""} {
	      rename ::nsf::_%&createobjectsystem ::nsf::objectsystem::create
	    }
	    unset ::nx::doc::rootns
	  }
	  rename ::proc ""
	  interp expose "" proc
	  rename ::namespace ""
	  interp expose "" namespace
	  rename ::source ""
	  interp expose "" source
	  rename ::load ""	  
	  interp expose "" load
	  rename ::package ""
	  interp expose "" package
	  rename ::auto_import ""
	  interp expose "" auto_import

	  proc ::nx::doc::__at_register_command {} {}
	  proc ::nx::doc::__at_deregister_command {} {}
	  proc ::nx::doc::__at_register_package {} {}
	  proc ::nx::doc::__at_deregister_package {} {}
	}
      }
      next
    }
  }
  namespace export Sandbox
}




#
# Validator
#
namespace eval ::nx::doc {
  
  MixinLayer create @project::Validator -prefix ::nx::doc {

    namespace eval [[current] info parent] {
          namespace import -force ::nx::doc::*
    }

    namespace eval ::nx::doc::MixinLayer {
	namespace export Mixin
      }    
    
    namespace import -force ::nx::doc::MixinLayer::*

    :public method read {frontend srcs cmds} {
      set box ${:sandbox}
      [current class] apply
      set provided_entities [next]

      #
      # trigger the validation (top-down)
      #
      :validate
      # --

      [current class] revoke
      [:info class] containers reset [current]
      #
      # TODO: is_validated to later to become a derived/computed
      # property ... for now, we just need to escape from setting
      # validation-related info in non-validated projects!
      #
      :is_validated 1; # is_validated = 1
      
      set present_entities [::nx::doc::filtered $provided_entities {[[:origin] eval {info exists :pdata}]}]
      
      # / / / / / / / / / / / / / / / / / / / / / 
      # Handling "missing" documentation entities
      #
      # 1) Add support for "generated" packages and their validation.
      #
      
      set pkgMap [dict create]
      if {[$box eval {info exists :registered_packages}] && [$box permissive get package] ne ""} {
	set generatedPackages [$box eval {set :registered_packages}]
	set providedPackages [@package info instances]
	set presentPackages [::nx::doc::filtered $providedPackages {[[:origin] eval {info exists :pdata}]}]
	foreach presPkgName $presentPackages {
	  set generatedPackages [dict remove $generatedPackages [$presPkgName name]]
	  dict set pkgMap [$presPkgName name] $presPkgName
	}
	  set permissivePkgs [$box permissive get package]
	  dict for {genPkgName info} $generatedPackages {
	    if {$genPkgName in $permissivePkgs} {
	      dict with info {
		set pkgObj [@package new -name $genPkgName -@version $version]
		$pkgObj pdata [list status missing]
	      }
	      dict set pkgMap $genPkgName $pkgObj
	    }
	  }
	}
      set generated_commands  [dict filter $cmds script {k v} { 
	dict with v { expr {$cmdtype in {@object @class @command} && !$dependency } } 
      }]
      
      set generated_commands [dict merge $generated_commands [dict filter $cmds script {k v} { 
	dict with v { expr {$cmdtype eq "@method" && !$dependency} } 
      }]]
      
      
	set map [dict create]
	foreach pe $present_entities {
	  if {[$pe pinfo exists bundle handle]} {
	    set fqn [$pe pinfo get bundle handle]
	  } else {
	    set fqn [$pe get_fqn_command_name]
	  }
	  dict unset generated_commands $fqn
	  dict set map $fqn $pe
	}

      #
      # 2.) Generate entities for undocumented ("missing") program entities
      #
      dict for {cmd info} $generated_commands {
	dict with info {
	  if {$cmdtype ni [list @command @object @class @method]} continue;
	  if {[info exists package] && [dict exists $pkgMap $package]} {
	    set pkgObj [dict get $pkgMap $package]
	    [:info class] containers push $pkgObj
	    unset package
	  }
	  
	    if {$cmdtype eq "@object" && [string match *::slot::* $cmd]} {
	      if {[dict exists $info bundle objtype] && [dict get $info bundle objtype] eq "ensemble"} continue;
	      set name [namespace tail $cmd]
	      set scope ""
	      set obj [namespace qualifiers [namespace qualifiers $cmd]]
	      if {![dict exists $map $obj]} continue;
	      set partof_entity [dict get $map $obj]
	      set entity [$partof_entity @[join [list {*}${scope} property] -] $name]
	    } elseif {$cmdtype eq "@method"} {
	      lassign [dict get $bundle handleinfo] obj scope name
	      # ! we assume the partof entity is present or has been generated
	      if {![dict exists $map $obj]} continue;
	      set partof_entity [dict get $map $obj]
	      if {![$partof_entity info has type ::nx::doc::@object]} continue;
	      set owning_entity $partof_entity 
	      foreach subm $name {
		set en [$partof_entity @[join [list {*}${scope} method] -] id $subm]
		if {$en ni $provided_entities} {
		  set partof_entity [$partof_entity @[join [list {*}${scope} method] -] $subm]
		  $partof_entity pinfo propagate status mismatch
		} else {
		  set partof_entity $en
		}
	      }
	      set entity $partof_entity
	      if {[dict exists $info bundle parameter]} {
		dict for {pname paraminfo} [dict get $info bundle parameter] {
		  lassign $paraminfo spec default
		  set paramid [@parameter id $entity "" $pname]
		  set ppdata [list bundle [list spec $spec default $default]]
		  if {$paramid ni $provided_entities} {
		    set paramid [$entity @parameter $pname]
		    lappend ppdata status missing
		  }
		  $paramid pdata $ppdata
		  $paramid pinfo propagate status mismatch
		}
	      }
	    } else {
	      set entity [@ $cmdtype $cmd]
	    }
	    
	    $entity pdata [lappend info status missing]
	    dict set map [$entity get_fqn_command_name] $entity
	  }
      }
    }
    
    #
    # The actual Validator mixin layer. The mixins trace the creation
    # process of provided entities (and assign corresponding pdata, if
    # present) and provide validation hooks for the various entity
    # types.
    #

    Mixin create [current]::Entity {
      :public method init args {
	next
	set prj [:current_project]
	if {$prj ne ""} {
	  set box [$prj sandbox]	  
	  set cmdname [:get_fqn_command_name]
	  if {[$box eval {info exists :registered_commands}] && \
		  [$box eval [concat dict exists \${:registered_commands} $cmdname]]} {
	    
	    :pdata [$box eval [concat dict get \${:registered_commands} $cmdname]]
	  }
	}
      }

      :public method validate {} {
	#
	# TODO: At some validate() spots, we still assume that the
	# missing entities are processed by the validator (e.g., to
	# mark container entities as a mismatch if a child entity is
	# missing etc.) ... This needs to be relocated ...
	#
	if {[info exists :pdata] && \
		[:pinfo get -default complete status] ne "missing"} {
	  if {[[:origin] as_list] eq ""} {
	    :pinfo propagate status mismatch
	    :pinfo lappend validation "Provide a short, summarising description!"
	  }
	}
	next
      }

    }

    Mixin create [current]::StructuredEntity -superclass [current]::Entity {
      :public method validate {} {
	next
	dict for {s entities} [:owned_parts -where "!\${:@stashed}"] {
	  foreach e $entities {
	    if {![$e eval {info exists :@use}]} {
	      $e [current method]
	    }
	  }
	} 
      }
    }


    Mixin create [current]::@command -superclass [current]::StructuredEntity {
      :public method validate {} {
	if {[info exists :pdata] && \
		[:pinfo get -default complete status] ne "missing"} {
	  
	  if {![info exists :@command]} {
	    set params [list]
	    set param_names [list]
	    if {[info exists :@parameter]} {
	      foreach p [:@parameter] {
		set value [$p name]
		lappend param_names $value
		if {[$p eval {info exists :default}] || $value eq "args" } {
		  set value "?$value?"
		}
		lappend params $value
	      }
	    }
	    
	    set ps [:pinfo get -default "" bundle parameter]
	    dict for {actualparam paraminfo} $ps {
	      if {$actualparam ni $param_names} {
		set p [:@parameter $actualparam]
		$p pdata [lappend paraminfo status missing]
	      }
	    }
	  } 

	  if {![:pinfo exists bundle parametersyntax]} {
	    :pinfo set bundle parametersyntax $params
	  }
	  
	  # TODO (Review!): [next] will cause the missing parameter
	  # created to be validated and will have the appropriate
	  # status propagated upstream!
	  next
	}
      }
    }

    Mixin create [current]::@object -superclass [current]::StructuredEntity
    
    Mixin create [current]::@class -superclass [current]::@object {
      :public method validate {} {
	next
	#
	# TODO: Certain metadata could also be valid in "missing"
	# state, e.g., paramtersyntax? Re-arrange later ...
	#
	if {[info exists :pdata] &&
	    [:pinfo get -default complete status] ne "missing"} {
	  #
	  # Note: Some metadata on classes cannot be retrieved from
	  # within the tracers, as they might not be set local to the
	  # class definition. Hence, we gather them at this point.
	  #
	  set prj [:current_project]
	  set box [$prj sandbox]
	  set statement [list ::nsf::dispatch ${:name} \
			     ::nsf::methods::class::info::objectparameter \
			     parametersyntax]
	  :pinfo set bundle parametersyntax [$box eval $statement]
	}
      }
    }

    Mixin create [current]::ContainerEntity -superclass [current]::StructuredEntity
    
    Mixin create [current]::@package -superclass [current]::ContainerEntity {
      :public method init args {
	next
	set prj [:current_project]
	if {$prj ne ""} {
	  set box [$prj sandbox]
	  if {[$box eval [concat dict exists \${:registered_packages} ${:name}]]} {
	    :pdata [$box eval [concat dict get \${:registered_packages} ${:name}]]
	  }
	}
      }

      :public method validate {} {
	if {[info exists :pdata] && \
		[:pinfo get -default complete status] ne "missing"} {
	  set orig [:origin]
	  if {[$orig eval {info exists :@version}]} {
	    set docVersion [$orig @version]
	    set actualVersion [:pinfo get version]
	    if {$docVersion ne $actualVersion} {
	      :pinfo propagate status mismatch
	      :pinfo lappend validation "Package version mismatch: $docVersion vs. $actualVersion"
	    }
	  }
	  next
	}
      }
    }

    Mixin create [current]::@method -superclass [current]::Entity {
      :method init args {
	next
	set scope [expr {[${:part_attribute} scope] eq "class"?"class":"object"}]
	set obj [:get_owning_object]
	set method_name [:get_combined name]
	set prj [:current_project]
	if {$prj ne ""} {
	  set box [$prj sandbox]	  
	  set script "if {\[::nsf::object::exists $obj\]} {array set \"\" \[$obj eval {:__resolve_method_path \"$method_name\"}\]; ::nsf::dispatch \$(object) ::nsf::methods::${scope}::info::method registrationhandle \$(methodName)}"
	  set cmdname [$box do $script]
	  if {$cmdname ne "" && [$box eval [concat dict exists \${:registered_commands} $cmdname]]} {
	    :pdata [$box eval [concat dict get \${:registered_commands} $cmdname]]
	  }
	}
	
      }

      :public method validate {} {
	set partof [:get_owning_partof]
	if {[info exists :pdata] &&
	    [:pinfo get -default complete status] ne "missing"} {
	  #
	  # Note: Some information on methods cannot be retrieved from
	  # within the tracers as they might not be set local to the
	  # method definition. Hence, we gather them at this point. I
	  # will review whether there is a more appropriate way of
	  # dealing with this issue ...
	  #
	  set prj [:current_project]
	  set box [$prj sandbox]
	  set obj [$partof name]
	  
	  if {[:pinfo exists bundle handle]} {
	    set handle [:pinfo get bundle handle]
	    :pinfo set bundle redefine-protected [$box eval [list ::nsf::method::property $obj $handle redefine-protected]]
	    :pinfo set bundle call-protected [$box eval [list ::nsf::method::property $obj $handle call-protected]]
	  }
	  
	  set params [list]
	  set param_names [list]
	  if {[info exists :@parameter]} {
	    foreach p [:@parameter] {
	      set value [$p name]
	      lappend param_names $value
	      if {[$p eval {info exists :default}] || $value eq "args" } {
		set value "?$value?"
	      }
	      lappend params $value
	    }
	  }
	  
	  dict for {actualparam paraminfo} [:pinfo get -default "" bundle parameter] {
	    if {$actualparam ni $param_names} {
	      set p [:@parameter $actualparam]
	      $p pdata [lappend paraminfo status missing]
	    }
	  }
	  
	  if {![:pinfo exists bundle parametersyntax]} {
	    :pinfo set bundle parametersyntax $params
	  }
	  
	  # Note: [next] will cause the missing parameter created to
	  # be validated and will have the appropriate status
	  # upstream!
	  next
	} else {
	  $partof pinfo propagate status mismatch
	}
      }
    }

    Mixin create [current]::@param -superclass [current]::Entity {
      :public method init args {
	if {[${:partof} info has type ::nx::doc::@method] || \
		[${:partof} info has type ::nx::doc::@command]} {
	  if {${:name} eq "__out__"} {
	    if {[${:partof} pinfo exists bundle returns]} {
	      :pdata [list bundle [list spec [${:partof} pinfo get bundle returns]]]
	    }
	  }
	  if {[${:partof} pinfo exists bundle parameter ${:name}]} {
	    lassign [${:partof} pinfo get bundle parameter ${:name}] spec default
	    :pdata [list bundle [list spec $spec default $default]]
	  }
	} else {
	  next
	}
      }

      :public method validate {} {
	#
	# TODO: For now, we escape from @param validaton on command
	# parameters. There is no equivalent to [info parameter]
	# available, so we would need to cook a substitute based on
	# the parametersyntax. Review later ...
	#
	if {${:name} eq "__out__" && \
		[${:partof} info has type ::nx::doc::@command]} return;

	#
	# Here, we escape from any parameter verification for
	# parameters on forwards & alias, as there is no basis for
	# comparison!
	#
	if {[${:partof} info has type ::nx::doc::@method] && \
		[${:partof} pinfo get bundle type] in [list forward alias]} {
	  dict set :pdata status ""
	  return;
	}

	if {[info exists :pdata] && \
		[:pinfo get -default complete status] ne "missing"} {
	  
	  # valid for both object and method parameters
	  set pspec [:pinfo get -default "" bundle spec]
	  if {[info exists :spec] && \
		  ${:spec} ne $pspec} {
	    :pinfo propagate status mismatch 
	    :pinfo lappend validation "Specification mismatch. Expected: \
						'${:spec}' Got: '$pspec'."
	  }
	  next
	} else {
	  ${:partof} pinfo propagate status mismatch 
	}
      }
    }
  }
}
	  
namespace eval ::nx::doc {

  ::nsf::proc mkIndex {{-documentAll:switch 0} {-indexfiles:0..* ""} {-outdir "[pwd]"} args} {

    if {![llength $args]} {
      set args *.tcl
    }

    set scripts [list]
    foreach file [glob -- {*}$args] {
      set file [file normalize $file]
      if {[file readable $file]} {
	lappend scripts $file
      }
    }

    if {![llength $scripts]} return;

    set sbox [Sandbox new -interp [interp create]]
    # 1pass
    append scriptBlock "source " [join $scripts "; source "]
    $sbox do [list package req nsf]
    $sbox do $scriptBlock
    # 2pass
    $sbox do [list ::nx::doc::__init]
    $sbox do $scriptBlock
    set cmds [dict keys [$sbox getCommandsFound -types {@command @object @class @method}]]
    
    append index "# NXDoc index file, version [package require nx::doc]\n"
    append index "# This file was generated by the \"::nx::doc::mkIndex\" command\n"
    append index "# and is optionally sourced by nxdoc to filter the command population\n"
    append index "# to be documented.  Typically each line is a command that\n"
    append index "# sets an element in the ::nxdoc::include array, where the\n"
    append index "# element name is the name of a command and the value indicates whether\n"
    append index "# the command is to be documented (1) or not (0).\n"
    append index \n

    if {[llength $indexfiles]} {
      append index "# Source external (e.g., auto-generated) index files\n"
    }

    foreach idx $indexfiles {
      append index {source [file join [file dirname [info script]] } $idx {]} "\n"
    }

    foreach cmd $cmds {
      append index "set ::nxdoc::include($cmd) $documentAll\n"
    }

    set fid [open [file join [file normalize $outdir] nxdocIndex.tcl] w]
    puts -nonewline $fid $index
    close $fid
  }  
}