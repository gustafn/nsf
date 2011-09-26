
package provide nx::doc::dc 1.0
namespace eval ::nx::doc {}

package require nx::doc 1.0

namespace eval ::nx::doc {

  @project eval {

    :private method "frontend dc" {srcs cmds} {
      
      #
      # Action 7) Have the documentation dependencies processed
      # (documented, but stashed entities)
      #
      :readDeps $srcs $cmds
      
      return [:readSrcs $srcs $cmds]
    }


    nx::Class create [current]::DependencyEntity {
      :class property {instances:0..*,object,type=::nx::doc::Entity ""} {
	set :incremental 1
      }
      :public method init args {
	next
	#
	# stash the entity
	#
	:@stashed
	[current class] instances add [current]
      }
    }

    :public method readDeps {srcs cmds} -returns 0..*,object,type=::nx::doc::Entity {
      #
      # 1) Get dep sources/cmds
      #

      set dSrcs [dict filter $srcs script {k v} { dict with v {set dependency}}]
      set dCmds [dict filter $cmds script {k v} { dict with v {set dependency}}]
      
      #
      # 2) Forward to the frontend, while tracking the creation of doc
      # entities ...
      #
      Entity mixin add [current class]::DependencyEntity
      foreach companion [:getCompanions $dSrcs] {
	:readin $companion
      }      
      Entity mixin delete [current class]::DependencyEntity

      set inst [[current class]::DependencyEntity instances]

      #
      # cleanup
      #
      [current class]::DependencyEntity instances [list]

      return $inst
    }

        nx::Class create [current]::ProvidedEntity {
      :class property {instances:0..*,object,type=::nx::doc::Entity ""} {
	set :incremental 1
      }
      :public method init args {
	next
	[current class] instances add [current]
      }
    }


    :public method readSrcs {srcs cmds} {
      set aSrcs [dict filter $srcs script {k v} { dict with v {expr {!$dependency}} }]
      set aCmds [dict filter $cmds script {k v} { dict with v {expr {!$dependency}} }]

      Entity mixin add [current class]::ProvidedEntity
      #
      # 1) Process the doc sources
      #
      foreach companion [:getCompanions $aSrcs] {
	:readin $companion
      }

      if {0} {
	#
	# 2) FIXME: Process the cmds, for docstring occurrences
	#
	
	dict for {cmd info} $aCmds {
	  dict with info {
	    #
	    # TODO: for now, we assume objects beyond this point
	    # ... relax later!
	    #
	    if {$cmdtype ni [list @object @class]} continue;
	    if {[info exists docstring]} {
	      lassign [:readin \
			   -docstring \
			   -tag $cmdtype \
			   -name $cmd \
			   -parsing_level 1 \
			   $docstring] entity processed_entities
	      unset docstring
	    }
	    if {$entity ne ""} {
	      :process=$cmdtype [current] $entity
	    }
	  }
	}
      }
      #:frontend $frontend $aSrcs $aCmds
      Entity mixin delete [current class]::ProvidedEntity
      set inst [[current class]::ProvidedEntity instances]
      #
      # cleanup
      #
      [current class]::ProvidedEntity instances [list]
      return $inst
    }


    :protected method getCompanions {sourceScripts} {
      set scripts [list]
      array set sourcables [list]
      dict for {key info} $sourceScripts {
	dict with info {
	  if {[info exists script]} {
	    lappend scripts $script
	    unset script
	  }
	  if {[info exists path]} {
	    set rootname [file rootname $path]
	    set dir [file dirname $path]
	    set sourcables($rootname.nxd) ""
	    if {[info exists package]} {
	      set sourcables([file join $dir $package].nxd) ""
	      unset package
	    }
	    unset path
	  }
	}
      }
      
      foreach s [array names sourcables] {
	if {![file isfile $s] || ![file readable $s]} continue;
	set fh [open $s r]
	if {[catch {lappend scripts [read $fh]} msg]} {
	  catch {close $fh}
	  :log "error reading the file '$s', i.e.: '$msg'"
	}
	catch {close $fh}
      }
      return $scripts
    }




    :protected method process=package {project pkgs nsFilters:optional} {
      # / / / / / / / / / / / / / / / / / / / / / / / / / / / / /
      # VALIDATION
      set box [$project sandbox]
      $box permissive_pkgs $pkgs
      set 1pass ""
      foreach pkg $pkgs {
	if {[catch {package req $pkg} _]} {
	  error "Tcl package '$pkg' cannot be found."
	}
	append 1pass "package req $pkg\n"
      }

      #
      # a) 1-pass: requiring the packages first will provide
      # all dependencies (also those not to be documented).
      #
      $box do "::nx::doc::__trace_pkg; $1pass"
      #
      # b) 2-pass: [source] will re-evaluate the package scripts
      # (note, [load]-based extension packages are not covered by this!)
      #"
      if {[$box eval {info exists :source}]} {
	foreach {pkg src} [$box eval {set :source}] {
	  #
	  # TODO: 2-pass [source]s should not trigger transitive [source]s. we
	  # have flattened the relevant [source] hierarchy in the
	  # 1-pass.
	  #
	  append 2pass \
	      "::nx::doc::__cpackage push $pkg;\n" \
	      "source $src;\n" \
	      "::nx::doc::__cpackage pop;\n"
	}
	$box do "::nx::doc::__init; $2pass" 
      }

      #
      # Filter registered commands for includes/excludes 
      #
      # ISSUE: Filtering can apply to two different populations: a)
      # for validation, the registered commands; b) without
      # validation, the provided doc ones ... Should we stress (and
      # implement) the difference?!
      # 
      #
      if {[info exists nsFilters]} {
	$box registered_commands [$box get_registered_commands $nsFilters]
      }

      # VALIDATION
      # \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \
     
      # / / / / / / / / / / / / / / / / / / / / / / / / / / / / /
      # PROCESSOR
      foreach {attr part_class} [$project part_attributes] {
	$part_class class mixin add ::nx::doc::ContainerEntity::Containable
	$part_class container $project
      }

      set deps_entities [list]
      foreach dep [$box getCompanions [$box eval {set :deps}]] {
	lappend deps_entities {*}[:readin $dep]
      }
      foreach de $deps_entities {
	$de @stashed
      }

      set scripts [$box get_companions]
      set provided_entities [list]

      foreach script $scripts {
	lappend provided_entities {*}[:readin $script]; # -> TODO: rather dispatch to process=source()?!
      }
      return $provided_entities
      # PROCESSOR
      # \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \

    }

    :protected method process=source {project filepath} {;}

    :protected method process=eval {project scripts} {
      # / / / / / / / / / / / / / / / / / / / / / / / / / / / / /
      # VALIDATION

      set box [$project sandbox]
      #
      # 1a) 1pass ... TODO: should tracing be enabled in this scenario? ...
      #
      foreach script $scripts {
	$box do $script
      }

      #
      # 2) 2pass ... 
      # 
      $box do [list ::nx::doc::__init]

      foreach script $scripts {
	$box do $script
      }
      #
      # 3) documentation processing
      #

      # VALIDATION
      # \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \


      # / / / / / / / / / / / / / / / / / / / / / / / / / / / / /
      # PROCESSOR

      # 3a) top-level processing
      foreach script $scripts {
	:readin $script
      }
      

      # 3b) initcmds, proc bodies ...

      dict for {cmd info} [$box get_registered_commands] {
	dict with info {
	  #
	  # TODO: for now, we assume objects beyond this point
	  # ... relax later!
	  #
	  if {$cmdtype ni [list @object @class]} continue;
	  if {[info exists docstring]} {
	    lassign [:readin \
			    -docstring \
			    -tag $cmdtype \
			    -name $cmd \
			    -parsing_level 1 \
			    $docstring] entity processed_entities
	    unset docstring
	  } else {
	    set entity [@ $cmdtype $cmd]
	  }
	  :process=$cmdtype $project $entity
	}
      }

      # PROCESSOR
      # \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \

    }
        
    :public method readin {
	-docstring:switch 
	-tag
	-name
	-partof_entity:object,type=::nx::doc::StructuredEntity
	{-parsing_level:integer 0}
	script
      } {

      set blocks [:comment_blocks $script]
      set first_block 1
      set processed_entities [list]
      foreach {line_offset block} $blocks {
	array set arguments [list -initial_section context \
				 -parsing_level $parsing_level]

	if {$docstring} {
	  if {[info exists partof_entity]} {
	    set arguments(-partof_entity) $partof_entity
	  }
	  if {![info exists tag] || ![info exists name]} {
	    error "In docstring mode, provide the tag and the name of
	      a docstring-owning documentation entity object."
	  }
	  if {$first_block} {
	    #
	    # TODO: Note that the two "creation procedures" are not
	    # idempotent; the relative one overwrites description
	    # blocks of pre-exisiting entities, the freestanding @
	    # does not ... fix later when reviewing these parts of the
	    # program ...
	    # 
	      set docentity [expr {[info exists partof_entity]?\
				       [$partof_entity $tag $name]:[@ $tag $name]}]
	      if {$line_offset <= 1} {
		set arguments(-partof_entity) $docentity
		set arguments(-initial_section) description
		set arguments(-entity) $docentity
	      }
	    }
	}
	
	set args [array get arguments]
	lappend args $block
	#:apply
	::nx::doc::CommentBlockParser process {*}$args
	#lappend processed_entities {*}[:revoke]
	set first_block 0
      }
      #
      # FIXME /YYYY 
      #
      set processed_entities [list]
      if {$docstring && [info exists arguments(-partof_entity)]} {
	return [list $arguments(-partof_entity) $processed_entities]
      } else {
	return $processed_entities
      }
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
	  lappend comment_block $text}
	1,0	{
	  lappend comment_blocks $line_offset $comment_block; 
	  unset comment_block
	}
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
      if {[info exists comment_block]} {
	lappend comment_blocks $line_offset $comment_block
      }
      return $comment_blocks
    }
    
    # TODO: how can I obtain some reuse here when later @class is
    # distinguished from @object (dispatch along the inheritance
    # hierarchy?)

    :public method process=@command {project entity} {;}

    :public method process=@class {project entity} {
      set name [$entity name]
      set box [$project sandbox]
      # attributes
      foreach slot [$box do [list $name info slot objects]] {
	if {[$box do [list $slot eval {info exists :__initcmd}]]} {
	  #
	  # TODO: Here, we eagerly create doc entities, is this an issue?
	  # Should we mark them for removal if not further processed?
	  # This might be contradicting to the requirement of
	  # identifying documented/undocumented program structures.
	  #
	  # There are two alternatives:
	  # -> use a freestanding identity generator (preferred!)
	  # -> mark the entity for deletion
	  #
	  # set id [$entity @${scope}-attribute [$box do [list $slot name]]]
	 
	  set scope [expr {[$box do [list $slot per-object]]?"class-object":"class"}]
	  :readin \
	      -partof_entity $entity \
	      -docstring \
	      -tag @${scope}-property \
	      -name [$box do [list $slot name]] \
	      -parsing_level 2 \
	      [$box do [list $slot eval {set :__initcmd}]]

	}
      }

      foreach methodName [$box do [list $name info methods \
				       -methodtype scripted \
				       -callprotection public]] {
	:readin \
	    -partof_entity $entity \
	    -docstring \
	    -tag @class-method \
	    -name $methodName \
	    -parsing_level 2  \
	    [$box do [list ${name} info method body $methodName]]
      }
      
      :process=@object $project $entity class
      
    }
    
    #
    # TODO: how to resolve to the current project's context. For now,
    # we pass a parameter value, revisit this decision once we decide
    # on a location for this behaviour.
    #
    :public method process=@object {project entity {scope ""}} {
      set name [$entity name]
      set box [$project sandbox]
      # methods

      foreach methodName [$box do [list ${name} {*}$scope info methods\
				       -methodtype scripted \
				       -callprotection public]] {
	
	set tag [join [list {*}[expr {$scope eq "class"?"class-object":""}] method] -]
	# set id [$entity @$tag $methodName]
	:readin \
	    -partof_entity $entity \
	    -docstring \
	    -tag @$tag \
	    -name $methodName \
	    -parsing_level 2  \
	    [$box do [list ${name} {*}$scope info method body $methodName]]
      }
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

    :property {parsing_level:integer 0}

    :property {message ""}
    :property {status:in "COMPLETED"} {

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

    :property processed_section  {
      :public method assign {domain prop value} {
	set current_entity [$domain current_entity]
	set scope [expr {[$current_entity info is class]?"class":""}]
	if {[$domain eval [list info exists :$prop]] && [:get $domain $prop] in [$current_entity {*}$scope info mixin classes]} {
	  $current_entity {*}$scope mixin delete [:get $domain $prop]
	}
	$current_entity {*}$scope mixin add [next [list $domain $prop $value]]
      }
    }
    :property current_entity:object
    
    :public class method process {
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
    
    :public forward has_next expr {${:idx} < [llength ${:comment_block}]}
    :public method dequeue {} {
      set r [lindex ${:comment_block} ${:idx}]
      incr :idx
      return $r
    }
    :public forward rewind incr :idx -1
    :public forward fastforward set :idx {% llength ${:comment_block}}

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
      # property slots defined in mixin classes; so do it manually
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

      set scope [expr {[${:current_entity} info is class]?"class":""}]
      set mixins [${:current_entity} {*}$scope info mixin classes]
      if {${:processed_section} in $mixins} {
	set idx [lsearch -exact $mixins ${:processed_section}]
	set mixins [lreplace $mixins $idx $idx]
	::nsf::relation ${:current_entity} object-mixin $mixins
      }
      
    }; # CommentBlockParser->process()
    
  }
  
  Class create CommentBlockParsingState -superclass Class {
    
    :property next_comment_section
    :property comment_line_transitions:required
    
  }
  
  Class create CommentSection {

    :property block_parser:object,type=::nx::doc::CommentBlockParser
    :property {current_comment_line_type ""}

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

    :public forward event=parse %self {% subst {parse@${:current_comment_line_type}}} 
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
    # set :lineproc {{tag args} {return [concat {*}$args]}}
    # set :lineproc {{tag args} {puts stderr LINE=[list $tag {*}$args]; return [list $tag {*}$args]}}
    set :lineproc {{tag args} {return [list $tag [expr {$args eq ""?$args:[list $args]}]]}}
    :method parse@tag {line} {
      lassign [apply [[current class] eval {set :lineproc}] {*}$line] tag line
      #set line [lassign [apply [[current class] eval {set :lineproc}] {*}$line] tag]
      if {[:info lookup methods -source application $tag] eq ""} {
	set msg "The tag '$tag' is not supported for the entity type '[namespace tail [:info class]]"
	${:block_parser} cancel INVALIDTAG $msg
      }
      #:$tag [lrange $line 1 end]
      #:$tag {*}[expr {$line eq ""?$line:[list $line]}]
      #:$tag $line
      :$tag {*}$line
    }

    :method parse@text {line} {
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

	set :lineproc {{tag name args} {return [list $tag $name $args]}}
	:method parse@tag {line} {
	  lassign [apply [[current class] eval {set :lineproc}] {*}$line] axes names args
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
	    set cmd [info commands @$leaf(axis)]
	    
	    # TODO interp-aliasing objects under different command names
	    # is currently not transparent to some ::nsf::* helpers,
	    # such as ::nsf::object::exists. Should this be changed?
	    #
	    if {$cmd ne ""} {
	      set cmd [namespace origin $cmd]
	      set target [interp alias {} $cmd]
	      if {$target ne ""} {
		set cmd $target
	      }
	    }
	    
	    if {$cmd eq "" || ![::nsf::object::exists $cmd] || \
		    ![$cmd info has type Tag]} {
	      
	      ${:block_parser} cancel INVALIDTAG "The entity type '@$leaf(axis)' is not available."
	    }

	    # VERIFY! Still an issue? TODO: @object-method raises some
	    # issues (at least when processed without a resolved
	    # context = its partof entity).  It is not an entity type,
	    # because it merely is a "scoped" @method. It won't
	    # resolve then as a proper instance of Tag, hence we
	    # observe an InvalidTag exception. For now, we just ignore
	    # and bypass this issue by allowing InvalidTag exceptions
	    # in analyze()

	    set entity [@$leaf(axis) new -name $leaf(name) {*}$args]
	  } else {
	    if {[$entity info lookup methods -source application @$leaf(axis)] eq ""} {
	      ${:block_parser} cancel INVALIDTAG \
		  "The tag '$leaf(axis)' is not supported for the entity type '[namespace tail [$entity info class]]'"
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
	  unset -nocomplain :current_part
	  next
	}
	:method parse@tag {line} {
	  set r [next]
	  if {[::nsf::object::exists $r] && [$r info has type ::nx::doc::Entity]} {
	    set :current_part $r
	  }
	  return $r
	}
	:method parse@text {line} {
	  if {[info exists :current_part]} {
	    ${:current_part} @doc add $line end
	  } else {
	    :event=next $line
	  }
	}
	# :method parse@space {line} {;}
      }

  namespace export CommentBlockParser
}