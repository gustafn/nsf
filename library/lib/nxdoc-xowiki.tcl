package provide nx::doc::xowiki 1.0
namespace eval ::nx::doc {}

package require nx::doc::html 1.0
package require nx::serializer
package require base64

namespace eval ::nx::doc {

  Renderer create xowiki -extends [html] {
    #
    # yuidoc refinements
    #
    #<a class="$cssclass" title="$title" href="[:href $top_entity]">$source_anchor</a>
    :addTemplate link yuidoc {
      [:! lassign [:getBase {*}[expr {[info exists top_entity]?$top_entity:""}]] base fragment_path]
      [:!let basename [expr {[$base info has type ::nx::doc::@glossary]?"en:glossary#[$base name]":"en:[$base filename]#[join $fragment_path _]"}]]
      \[\[$basename|$source_anchor\]\]
    }

    :class method render {project entity theme {tmplName ""}} {
      

      
      set top_level_entities [$project navigatable_parts]
      set init [subst {
	set project $project
	set project_entities \[list $top_level_entities\]
      }]
      $entity current_project $project
      $entity renderer [current]
      set content [$entity render -initscript $init -theme $theme body-chunked]
      set p [::xowiki::Page new -name en:[$entity filename] \
		 -title [$entity name] \
		 -text [list $content text/html]]
      return [$p serialize]
    }

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
      # TODO: assets (js, css, img must be wrapped as ::xowiki::Files)
      #
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
	$f eval [list set :__file_content [::base64::encode [:read -binary $assetPath]]]
	:write [$f serialize] $targetDir
      }
    }
  }; # xowiki renderer

  #
  # The necessary xowiki stubs
  #

  namespace eval ::xowiki {
    namespace import -force ::nx::*
    Class create Page { 
      :property {lang en}
      :property {description ""}
      :property {text ""}
      :property {nls_language en_US}
      :property {mime_type text/html}
      :property {title ""}
      :property name
      :property text 
    }
    Class create File -superclass Page
  }

}