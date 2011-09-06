package provide nx::doc::html 1.0
namespace eval ::nx::doc {}

package require nx::doc 1.0

namespace eval ::nx::doc {

  Renderer create html {
    
    :method render {project entity theme {tmplName ""}} {
      set top_level_entities [$project navigatable_parts]
      set init [subst {
	set project $project
	set project_entities \[list $top_level_entities\]
      }]
      $entity current_project $project
      $entity renderer [current]
      $entity render -initscript $init -theme $theme {*}$tmplName
    }

    :method installAssets {project theme targetDir} {
      set assets [glob -directory [file join [::nx::doc::find_asset_path] $theme] *]
      file mkdir $targetDir
      if {$assets eq ""} return;
      file copy -force -- {*}$assets $targetDir
    }

    #
    # The actual refinements delivered by the mixin layer
    #

    MixinLayer::Mixin create [current]::Entity -superclass TemplateData {
      #
      # TODO: Would it be useful to allow property slots to describe
      # a per-class-object state, while the accessor/mutator methods
      # are defined on the per-class level. It feels like the class
      # instance variables in Smalltalk ...
      #
      # TODO: Why is call protection barfing when the protected target
      # is called from within a public forward. This should qualify as
      # a valid call site (from "within" the same object!), shouldn't it?
      # :protected class property current_project:object,type=::nx::doc::@project
      # :class property current_project:object,type=::nx::doc::@project
      # :public forward current_project [current] %method

      # :public forward print_name %current name

      :public method statustoken {} {
	set token ""
	set obj [:origin]
	set prj [:current_project]
	if {[$prj is_validated]} {
	  if {[$obj eval {info exists :pdata}]} {
	    set token [$obj pinfo get -default "" status]
	  } else {
	    set token "extra"
	  }
	}
	return $token
      }

      :public method statusmark {} {
	set token [:statustoken]
	set status_mark "<span title=\"$token\" class=\"status $token\">&nbsp;</span>"
      }
      :public method print_name {-status:switch} {
	set status_mark [expr {$status?[:statusmark]:""}]
	return "${:name}$status_mark"
      }

      :method fit {str max {placeholder "..."}} {
	if {[llength [split $str ""]] < $max} {
	  return $str;
	}
	set redux [llength [split $placeholder ""]]
	set margin [expr {($max-$redux)/2}]
	return "[string range $str 0 [expr {$margin-1}]]$placeholder[string range $str end-[expr {$margin+1}] end]"
      }
      
      :public method as_dict {partof feature} {
	set hash [dict create]
	dict set hash access ""
	dict set hash host [$partof name]
	dict set hash name [:print_name]
	#	dict set hash url "[$partof filename].html#[string trimleft [$feature name] @]_${:name}"
	dict set hash url "[:href $partof]"
	dict set hash type [$feature pretty_name]
	return $hash
      }

      :method as_array_of_hashes {} {
	set features [:navigatable_parts]
	set js_array [list]
	dict for {feature instances} $features {
	  foreach inst $instances {
	    set d [$inst as_dict [current] $feature]
	    set js_hash {{"access": "$access", "host": "$host", "name": "$name", "url": "$url", "type": "$type"}}
	    dict with d {
	      lappend js_array [subst $js_hash]
	    }
	  }
	}
	return "\[[join $js_array ,\n]\]"
      }
      
      :public method navigatable_parts {} {
	#
	# TODO: Should I wrap up delegating calls to the originator
	# entity behind a unified interface (a gatekeeper?)
	#
	return [[:origin] owned_parts \
		    -where "!\${:@stashed}" \
		    -class ::nx::doc::StructuredEntity]
      }
      
      :method listing {{-inline true} script} {
	set listing $script
	if {!$inline} {
	  set listing [string trimright [nx::pp render [string trimright $script " \r\n"]] "\n"]
	}
	next [list -inline $inline $listing]
      }
      
      :method link=tclcmd {cmd} {
	#
	# TODO: allow the parametrization of the reference URL at the
	# project level ...
	#
	return "<a href=\"http://www.tcl.tk/man/tcl8.5/TclCmd/${cmd}.htm\"><code>$cmd</code></a>"
      }

      :method link {tag value} {
	set unresolvable "<a href=\"#\">?</a>"
	if {[string first @ $tag] != 0} {
	  set m [current method]=$tag
	  if {[:info lookup methods \
		   -source application \
		   -callprotection public $m] eq ""} {
	    return $unresolvable
	  }
	  return [:$m $value]
	} else {
	  set names $value
	  set tagpath [split [string trimleft $tag @] .]
	  lassign [::nx::doc::Tag normalise $tagpath $names] err res
	  if {$err} {
	    # puts stderr RES=$res
	    return $unresolvable;
	  }
	  lassign [::nx::doc::Tag find -all -strict {*}$res] err path
	  if {$err || $path eq ""} {
	    # puts stderr "FAILED res $path (err-$err-id-[expr {$path eq ""}])"
	    return $unresolvable;
	  }
	  
	  set path [dict create {*}$path]
	  set entities [dict keys $path]
	  set id [lindex $entities end]
	  return [$id render_link $tag [:rendered top] $path]
	}
      }
      
      :public method make_link {source} {
	set path [dict create {*}[:get_upward_path -attribute {set :name}]]
	set tag [[:info class] tag]
	return [:render_link $tag $source $path]
      }

      :public method render_link {tag source path} {
	set id [current]
	set pathnames [dict values $path]
	set entities [dict keys $path]
	set top_entity [lindex $entities 0]
	set pof ""
	if {$top_entity ne $id} {
	  set pof "[$top_entity name]#"
	  set pathnames [lrange $pathnames 1 end]
	  set entities [lrange $entities 1 end]
	}
	#return "<a href=\"[$id href $top_entity]\">$pof[join $pathnames .]</a>"
	# GN TODO: Maybe a nicer "title" property via method title?
	#return "<a class='nsfdoc-link' title='$pof[join $pathnames .]' \
	    #	href='[$id href $top_entity]'>[join $pathnames { }]</a>"
	set iscript [join [list [list set title $pof[join $pathnames .]] \
			       [list set source_anchor [join $pathnames { }]] \
			       [list set top_entity $top_entity]] \n]
	:render -initscript $iscript link
      }
      
      :public method as_text {} {
	set text [expr {[:origin] ne [current]?[[:origin] as_text]:[next]}]
	return [string map {"\n\n" "<br/><br/>"} $text]
      }

      :method getBase {top_entity:optional} {
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
	return [list $top_entity $fragment_path]
      }

      :public method href {-local:switch top_entity:optional} {
	lassign [:getBase {*}[expr {[info exists top_entity]?$top_entity:""}]] base fragment_path
	set fragments [join $fragment_path _]
	if {$local} { 
	  return $fragments
	} else {
	  set href "[$base filename].html#$fragments"
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

      :public method as_tag_id {} {
	set tagclass [:info class]
	set tail [$tagclass get_tail_name [current]]
	set tname [string trimleft [string map {:: _} $tail] "_"]
	return [$tagclass tag]__$tname
      }


    }; # NxDocTemplating::Entity

    MixinLayer::Mixin create [current]::@project -superclass [current]::Entity {
      :public method filename {} {
	return "index"
      }
      :public method navigatable_parts {} {
	#
	# TODO: Should I wrap up delegating calls to the originator
	# entity behind a unified interface (a gatekeeper?)
	#
	set top_level_entities [next]
	dict for {feature instances} $top_level_entities {
	  if {[$feature name] eq "@package"} {
	    foreach pkg $instances {
	      dict for {pkg_feature pkg_feature_instances} [$pkg navigatable_parts] {
		dict lappend top_level_entities $pkg_feature \
		    {*}$pkg_feature_instances
	      }
	    }
	  }
	}
	return $top_level_entities
      }
      
    }
    
    MixinLayer::Mixin create [current]::@glossary -superclass [current]::Entity {

      :public method print_name {} {
	return [expr {[info exists :@acronym]?${:@acronym}:${:@pretty_name}}]
      }

      array set :tags {
	@gls		{
	  set print_name [string tolower ${:@pretty_name} 0 0]
	  set title ${:@pretty_name}
	}
	@Gls		{
	  set print_name [string toupper ${:@pretty_name} 0 0]
	  set title ${:@pretty_name}
	}
	@glspl		{
	  set print_name [string tolower ${:@pretty_plural} 0 0]
	  set title ${:@pretty_plural}
	}
	@Glspl 		{
	  set print_name [string toupper ${:@pretty_plural} 0 0]
	  set title ${:@pretty_plural}
	}
	@acr		{
	  set acronym(short) 1
	}
	@acrfirst	{
	  set acronym(long) 1
	}

      }
      
      :public method href {-local:switch top_entity:optional} {
	set fragments "#${:name}"
	if {$local} { 
	  return $fragments
	} else {
	  return "[[:current_project] filename].html$fragments"
	}

      }

      :public method render_link {tag source path} {
	# tag-specific rendering
	set acronym(long) 0
	set acronym(short) 0
	set print_name ${:@pretty_name}
	set title ${:@pretty_name}
	if {[[current class] eval [list info exists :tags($tag)]]} {
	  eval [[current class] eval [list set :tags($tag)]]
	}
	if {[info exists :@acronym]} {
	  #
	  # First occurrance of an acronym entry!
	  #
	  if {!$acronym(short) && ($acronym(long) || ![info exists :refs] || \
				       ![dict exists ${:refs} [:current_project] $source])} {
	    set print_name "$print_name (${:@acronym})"
	  } else {
	    set title $print_name
	    set print_name ${:@acronym}
	    set anchor "<a href=\"[:href]\" title=\"$title\" class='nsfdoc-gloss'>$print_name</a>"
	    # TODO: Re-provide the <abbrv> environment
	    #set res "<abbr title=\"$title\">$anchor</abbr>" 
	  }
	}

	# record for reverse references
	if {![info exists :refs]} {
	  set :refs [dict create]
	}
	dict update :refs [:current_project] prj {
	  dict incr prj $source
	}

	set iscript [join [list [list set title $title] \
			       [list set source_anchor $print_name] \
			       [list set top_entity [current]] \
			       [list set cssclass nsfdoc-gloss]] \n]
	set res [:render -initscript $iscript link]
	return $res
      }
    }; # NxDocRenderer::@glossary

    MixinLayer::Mixin create [current]::@class -superclass [current]::Entity {
      :method inherited {member} {
	set inherited [dict create]
	set prj [:current_project]
	if {![$prj eval {info exists :sandbox}]} return;
	set box [$prj sandbox]
	set exp "expr {\[::nsf::is class ${:name}\]?\[lreverse \[${:name} info heritage\]\]:\"\"}"
	set ipath [$box do $exp]
	foreach c [concat $ipath ${:name}] {
	  set entity [[:info class] id $c]
	  if {![::nsf::is object $entity]} continue; 
	  set origin [$entity origin]
	  if {$origin ni [concat {*}[dict values [$prj navigatable_parts]]]} continue;
	  
	  if {[$origin eval [list info exists :${member}]]} {
	    dict set inherited $entity [$entity !get \
					    -sortedby name \
					    -with name $member]
	    if {[info exists previous_entity]} {
	      dict set inherited $previous_entity \
		  [dict remove [dict get $inherited $previous_entity] \
		       {*}[dict keys [dict get $inherited $entity]]]
	    }
	  }
	  set previous_entity $entity
	}
	dict unset inherited [current]
	return $inherited
      }
    }


    MixinLayer::Mixin create [current]::@method -superclass [current]::Entity {
      :public method as_dict {partof feature} {
	set hash [next]
	dict set hash access [expr {[:pinfo get -default 0 bundle call-protected]?"protected":""}]
	return $hash
      }
    }; # html::@method

  }; # html renderer
}