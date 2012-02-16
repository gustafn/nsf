package provide nx::doc::xowiki 1.0
namespace eval ::nx::doc {}

package require nx::doc::html 1.0
package require nx::serializer


#
# The necessary xowiki stubs
#

namespace eval ::xowiki {
  # namespace import -force ::nx::*
  nx::Class create Page { 
    :property {lang en}
    :property {description ""}
    :property {text ""}
    :property {nls_language en_US}
    :property {mime_type text/html}
    :property {title ""}
    :property name
    :property text 
    :property page_order
    #
    # For representing a folder structure
    #
    :property parent_id
    :property item_id
  }
  
  nx::Class create File -superclass Page
  nx::Class create PageTemplate -superclass Page
  nx::Class create PlainPage -superclass Page {
    :property {mime_type text/plain}
  }
  nx::Class create Object -superclass PlainPage


  nx::Class create Form -superclass PageTemplate {
    :property {form ""}
    :property {form_constraints ""}
  }
  
  nx::Class create FormPage -superclass Page { 
    :property page_template:integer
    :property instance_attributes
  }


  #
  # A "folder" prototype form
  # 

  Form create folder.form \
      -form {
	{<form>@extra_menu_entries@ @index@ @ParameterPages@ </form>} text/html
      } \
      -form_constraints {
	extra_menu_entries:menuentries _nls_language:omit _name:required
      } \
      -item_id "1000" \
      -name en:folder.form {
	set :__export_reason "implicit_page_template"
      }

  #
  # A variant of the "toc" includelet, adapted for nxdoc pages ...
  #

  Object create includelet.nxdoc-toc \
      -name en:nxdoc-toc \
      -text {
	my initialize -parameter {
	  {-submenu ""}
	}	
	
	my proc page_number {page_order remove_levels} {
	  return ""
	}

	my proc href {book_mode name} {
	  set page [my info parent]
	  if {![$page exists __including_page]} return
	  $page instvar package_id __including_page
	  if {$book_mode} {
	    set href [$package_id url]#$name
	  } else {
	    set href [$package_id pretty_link -parent_id [$__including_page parent_id] $name]
	  }
	  return $href
	}


	my proc content {} {
	  my get_parameters
	  set page [my info parent]
	  if {![$page exists __including_page]} return
	  $page instvar __including_page
	  set r [$__including_page include [list toc  \
						-style none  \
						-decoration plain  \
						-book_mode 0  \
						-expand_all 1  \
						-ajax 0]]
	  if {[info command ::__xowiki__MenuBar] ne "" && [llength $submenu]} {
	    #
	    # TODO: Right now, there is no way of providing a
	    # multi-level subtree as the additional_asub_menu helper
	    # is not applied recursively ...
	    #
	    # set items [::xo::OrderedComposite new -destroy_on_cleanup]
	    # foreach {feat sections} $submenu {
	    #   set pages [::xo::OrderedComposite new -destroy_on_cleanup]
	    #   foreach {name url} $sections {
	    # 	set o [xotcl::Object new]
	    # 	$o set name $name
	    # 	$o set title $name
	    # 	$o set page_order 0
	    # 	$pages add $o
	    #   }
	    #   $pages set name $feat
	    #   $pages set title $feat
	    #   $pages set page_order 0
	    #   $items add $pages
	    # }
	    set items [::xo::OrderedComposite new -destroy_on_cleanup]
	    foreach {feat sections} $submenu {
	      foreach {name url} $sections {
		set o [xotcl::Object new]
		$o set name $url
		$o set title $name
		$o set page_order 0
		$items add $o
	      }
	    }
	    my set book_mode 1
	    
	    ::__xowiki__MenuBar additional_sub_menu -kind folder -pages $items -owner [self]
	  }
	  return $r
	}
      }
  
  namespace export Page FormPage folder.form
}

namespace eval ::nx::doc {
  #
  # In this backend, we aim at generating the following xowiki structure:
  #
  # `- nx		<-- Folder+Page (for all packages)
  #	`- ::nx::C  	<-- Folder+Page (for all objects, classes, commands)
  #		`- ...  <-- auto append (from DOM) (e.g., methods, parameters ...)
  #		`- ...	
  #	`- ... 			
  #	`- ...
  # `- ::D
  #
  
  namespace import -force ::xowiki::Page ::xowiki::FormPage \
      ::xowiki::folder.form

  nx::Class create EntityFolder -superclass ::xowiki::FormPage {
    :property indexPage:object,type=[namespace current]::Page
    :public method init {} {
      set :item_id [expr {[folder.form item_id] + [llength [[current class] info instances]]}]
      set :page_template [folder.form item_id]
      set :instance_attributes {
	index en:index 
	extra_menu_entries {} 
	ParameterPages {}
      }
      next
    }

    :public class method mkFolder {entity} -returns object,type=[current] {
      return [:new -name en:[$entity filename] -title [$entity print_name]]
    }
    
    :public method mkFolder {partEntity} -returns object,type=[current] {
      set f [[current class] mkFolder $partEntity]
      $f parent_id ${:item_id}]
      $partEntity folder $f
      return $f
    }

    :public method mkIndex {entity indexContent contentType} {
      set pid [expr {[folder.form item_id] - [llength [[current class] info instances]]}]
      set :indexPage [Page new \
			  -name "en:index" \
			  -item_id  $pid \
			  -parent_id ${:item_id}]
      ${:indexPage} title [$entity name]
      ${:indexPage} text [list $indexContent text/html]
    }

    :public method serialize {} {
      set map [list [current class] [[: -system info class] info superclass]]
      set ignore [join [[current class] info slot names] |]
      append script [::Serializer deepSerialize -map $map -ignoreVarsRE $ignore [current]]
      if {[info exists :indexPage]} {
	append script \n [${:indexPage} serialize]
      }
      return $script
    }
  }


  Renderer create xowiki -extends [html] {

    MixinLayer::Mixin create [current]::Entity {

      :public method pathLevels {} {
	if {![info exists :levels]} {
	  lassign [:getEntityPath] basePath
	  set :levels [expr {[llength $basePath] + 1}]
	}
	return [join [lrepeat ${:levels} ".."] "/"]
      }


      :method asSubmenu {} {
	set features [:navigatable_parts]
	set subm [list]
	dict for {feature instances} $features {
	  set tmp [list]
	  foreach inst $instances {
	    set d [$inst as_dict [current] $feature]
	    dict with d {
	      lappend tmp $name [$inst href -local]
	    }
	  }
	  lappend subm [$feature pretty_plural] $tmp
	}
	return $subm
      }

      :method getEntityPath {} {
	set path [dict create {*}[:get_upward_path -withContainers true -attribute {set :name}]]
	set prj [:current_project]
	set topLevelEntities [concat {*}[dict values [$prj navigatable_parts]]]
	set entities [dict keys $path]
	
	# 
	# We default to the project entity as base entity ...
	#
	set baseEntity $prj
	foreach e [lreverse $entities] {
	  if {$e in $topLevelEntities} {
	    set baseEntity $e; break
	  }
	}
	# The "path" contains ...
	# 1. ... the path to the base entity (project, package, ...)
	# 2. ... the base entity (i.e., the entity which turns into a
	# self-contained render unit; a markup page, an xowiki page
	# etc.)
	# 3. ... the currently rendered entity. If the currently
	# rendered and the base entity are not identical, then the
	# rendered one turns into a subordinate path (e.g., anchor
	# fragments) of the base entitiy.
	set baseIdx [lsearch -exact $entities $baseEntity]
	set basePath [lrange $entities 0 [expr {$baseIdx - 1}]]
	set baseSubs [lrange $entities [expr {$baseIdx + 1}] end]
	return [list $basePath $baseEntity $baseSubs]
      }

      #
      # TODO: The HREF rendering needs to reflect the folder/page
      # structure ...
      #
      :public method href {-local:switch {topEntity:substdefault [current]}} {
	lassign [:getEntityPath] basePath baseEntity baseSubs
	set fragments [list]
	foreach sub $baseSubs {
	  lappend fragments [$sub filename]
	}
	set fragments [join $fragments _]
	if {$local} { 
	  return $fragments
	} else {
	  set folderPath [list] 
	  lappend basePath $baseEntity
	  foreach pe $basePath {
	    set prefix [expr {[$pe eval {info exists :part_attribute}] && \
				  [[$pe partof] info has type ::nx::doc::ContainerEntity]?[[$pe part_attribute] pretty_plural]:""}]
	    lappend folderPath {*}$prefix [$pe filename]
	  }
	  # TODO: is the "index" file really necessary, the folder
	  # default index should actually suffice. Check later ...
	  lappend folderPath "index"
	  set href [join [concat [join $folderPath "/"] $fragments] "#"]
	  #
	  # TODO: It would be more elegant to render a variant of
	  # [[..]] blocks which simply return the href ... instead of
	  # a complete <a> pair. Can we find a more xowiki'sh way?
	  #
	  # puts stderr HREF="[[:current_project] url]$href"
	  return [[:current_project] url]$href
	}
      }
      
      :public method filename {} {
	set n [string trimleft [string map {:: __} ${:name}] "_"]
	if {[info exists :partof]} {
	  return [string trimleft [${:part_attribute} name] @]_$n
	} else {
	  return [[: -system info class] tag]_$n
	}
      }
    }
   
    MixinLayer::Mixin create [current]::StructuredEntity \
	-superclass [current]::Entity {
	  :property folder:object,type=::nx::doc::EntityFolder

	  :public method requireFolder {} {
	    if {![info exists :folder]} {
	      set folderBuilder [expr {[info exists :partof]?\
					   [${:partof} requireFolder]:"::nx::doc::EntityFolder"}]
	      set :folder [$folderBuilder mkFolder [current]]
	    }
	    return ${:folder}
	  }
	}
    
    MixinLayer::Mixin create [current]::ContainerEntity \
	-superclass [current]::StructuredEntity {
	  :method asSubmenu {} {;}
	}
    
    :addTemplate link yuidoc {
      [:!let basename [expr {[: -system info has type ::nx::doc::@glossary]?\
				 "[$srcEntity pathLevels]/en:glossary#[:name]":"[:href]"}]]
      \[\[$basename|$source_anchor\]\]
    }


    :class method render {
	project 
	entity:object,type=::nx::doc::StructuredEntity 
	theme 
	{tmplName ""}} {
      set top_level_entities [$project navigatable_parts]
      set init [subst {
	set project $project
	set project_entities \[list $top_level_entities\]
      }]
      $entity current_project $project
      $entity renderer [current]
      set content [$entity render -initscript $init -theme $theme body-chunked]
      set folder [$entity requireFolder]
      $folder mkIndex $entity $content "text/html"
      return [$folder serialize]
    }
    
    if {[namespace ensemble exists ::binary] && \
	    "encode" in [dict keys [namespace ensemble configure ::binary -map]]} {
      set fwdTarget [list ::binary encode base64]
    } else {
      package require base64
      set fwdTarget [list ::base64::encode]
    }

    :class forward asBase64 {*}$fwdTarget

    :class method installAssets {project theme targetDir} {
      #
      # render and append single glossary page to the output
      #
      
      set top_level_entities [$project navigatable_parts]
      set init [subst {
	set project $project
	set project_entities \[list $top_level_entities\]
	set include glossary
      }]

      set c [$project render \
		 -initscript $init \
		 -theme $theme \
		 body-chunked]
      set p [::xowiki::Page new \
		 -name en:glossary \
		 -title Glossary \
		 -text [list $c text/html]]
      :write [$p serialize] $targetDir

      #
      # Dump the "folder" form prototype
      #

      :write [::xowiki::folder.form serialize] $targetDir
      :write [::xowiki::includelet.nxdoc-toc serialize] $targetDir

      set assets [glob -directory [file join [findAssetPath] $theme] *]

      array set mime {
	js	application/x-javascript
	css	text/css
	png	image/png
	gif	image/gif
	jpg	image/jpg
      }
      foreach assetPath $assets {
	set filename [file tail $assetPath]
	set f [::xowiki::File new \
		   -name file:$filename \
		   -title $filename \
		   -mime_type $mime([string trim [file extension $assetPath] "."])]
	$f eval [list set :__file_content [:asBase64 [:read -binary $assetPath]]]
	:write [$f serialize] $targetDir
      }
    }
  }; # xowiki renderer

}

#
# ./apps/utils/nxdoc  -doctitle nx -docurl "http://next-scripting.org/" -docversion 2.0b2 -outdir ./doc -format xowiki -layout many-to-1 -indexfiles library/nx/nxdocIndex.tcl -- "package:nx"
#
#4