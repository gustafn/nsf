package provide nx::doc::xodoc 1.0
namespace eval ::nx::doc::xodoc {}

package require nx::doc 1.0

namespace eval ::nx::doc::xodoc {

  namespace import -force ::nx::*
  namespace import -force ::nx::doc::*

  # xodoc		->	nxdoc
  # - - - - - - - - - - - - - - - -
  # MetadataToken	Entity
  # FileToken		@package 
  # PackageToken	@package
  # ConstraintToken	n/a
  # MethodToken		n/a
  # ProcToken		@method (scope = object)
  # InstprocToken	@method (scope = class)
  # ObjToken		@object
  # ClassToken		@class
  # MetaClassToken	n/a

  Class create MetadataToken {
    :object property -accessor public analyzer
    :public forward analyzer [current] %method
    :method as {partof:object,type=::nx::doc::StructuredEntity} \
        -returns object,type=::nx::doc::Entity {
	  error "Subclass responsibility"
        } 
    :public method emit {partof:object,type=::nx::doc::StructuredEntity} \
        -returns object,type=::nx::doc::Entity {
          set entity [:as $partof]
          set props [:get_properties]
          if {[dict exists $props description]} {
            $entity @doc [dict get $props description]
          }
          return $entity
        }
    :method get_properties {} {
      if {[info exists :properties]} {
	set props [dict create]
	foreach p ${:properties} {
	  if {[info exists :$p]} {
	    dict set props [string tolower $p] \
		[:format [set :$p]]
	  }
	}
	return $props
      }
    }
    :method format {value} {
      #
      # 1. replace @-prefixed tags etc.
      #
      set value [[:analyzer] replaceFormatTags $value]
      
      #
      # 2. escape Tcl evaluation chars in code listings
      #     
      set value [string map {
	"\\" "\\\\" 
	"{" "\\{" 
	"}" "\\}" 
	"\"" "\\\"" 
	"[" "\\[" 
	"]" "\\]" 
	"$" "\\$"
      } $value]
 
      #
      # 3. box the prop value in a list (this avoids unwanted
      # interactions with the line-by-line as_text post-processor)
      #
      return [list $value]
    }
  }
  
  Class create PackageToken -superclass MetadataToken
  Class create FileToken -superclass MetadataToken {
    :method as {partof:object,type=::nx::doc::StructuredEntity} \
        -returns object,type=::nx::doc::Entity {
          #
          # TODO: Where to retrieve the package name from?
          #
          return [@package new -name XOTcl]
        } 
    :public method emit {partof:object,type=::nx::doc::StructuredEntity} \
        -returns object,type=::nx::doc::Entity {
          set entity [next]
          set props [dict remove [:get_properties] description]
          dict for {prop value} $props {
            $entity @doc add "<h1>$prop</h1>[join $value]" end
          }
          $entity @namespace [[$entity current_project] @namespace]
          return $entity
        }
  }
  
  #
  # Note: For whatever reason, InstprocToken is provided but never
  # used, at least in XOTcl-langRef. while most probably due to a lack
  # of attention or a silent revocation of a design decision in xodoc,
  # it forces us into code replication for differentiating the
  # per-class and per-object scopes ... in xodoc, these scopes are
  # double-encoded, both in proper token subclassifications as well as
  # aggregation properties: procList, instprocList ... well, I will
  # have to live with it.
  #

  Class create MethodToken -superclass MetadataToken
  
  Class create ProcToken -superclass MethodToken {
    :method as {scope partof:object,type=::nx::doc::StructuredEntity} \
        -returns object,type=::nx::doc::Entity {
      return [$partof @${scope}-method [:name]]
    } 
    :public method emit {scope partof:object,type=::nx::doc::StructuredEntity} {
      set entity [:as $scope $partof]      
      set props [:get_properties]
      if {[dict exists $props description]} {
        $entity @doc [dict get $props description]
      }
      if {[dict exists $props return]} {
	$entity @return [dict get $props return]
      }
      return $entity
    }
  }
  
  Class create InstprocToken -superclass MethodToken
  
  Class create ObjToken -superclass MetadataToken {
    :method as {partof:object,type=::nx::doc::ContainerEntity} \
        -returns object,type=::nx::doc::Entity {
      return [@object new -name [:name]]
    } 
    :public method emit {entity:object,type=::nx::doc::Entity} \
        -returns object,type=::nx::doc::Entity {
          set entity [next]
          foreach p [:procList] {
            $p emit object $entity
          }
          return $entity
        }
  }
  
  Class create ClassToken -superclass ObjToken {
    :method as {partof:object,type=::nx::doc::ContainerEntity} \
        -returns object,type=::nx::doc::Entity {
      return [@class new -name [:name]]
    }
    :public method emit {entity:object,type=::nx::doc::Entity} \
        -returns object,type=::nx::doc::Entity {
          set entity [next]
          foreach iproc [:instprocList] {
            $iproc emit class $entity
          }
          return $entity
        }
  }
  
  Class create MetaClassToken -superclass ClassToken
  
  namespace export MetadataToken FileToken MethodToken ProcToken \
      InstprocToken ObjToken ClassToken MetaClassToken

  @project eval {
    :protected method "frontend xodoc" {srcs cmds} {

      set aSrcs [dict filter $srcs script {k v} { dict with v {expr {!$dependency}} }]

      #
      # Note: Expects the XOTcl2 utilities to be in place and
      # accessible by the [package req] mechanism, use e.g.:
      # export TCLLIBPATH=". ./library/xotcl/library/lib"
      #
      package req xotcl::xodoc
      namespace eval :: {namespace import -force ::xotcl::@}
      
      set docdb [XODoc new]
      ::@ set analyzerObj $docdb

      foreach m [namespace eval ::nx::doc::xodoc {namespace export}] {
	if {[::xotcl::Class info instances -closure ::xotcl::metadataAnalyzer::$m] ne ""} {
	  ::xotcl::metadataAnalyzer::$m instmixin add ::nx::doc::xodoc::$m
	}
      }
      
      dict for {s info} $aSrcs {
	dict with info {
	  if {![info exists script]} continue
	  $docdb analyzeFile $path
	  unset script

	  ::nx::doc::xodoc::MetadataToken eval [list set :analyzer $docdb]
	  set provided_entites [list]
	  #
	  # as we analyze file by file, there is only one FileToken to
	  # be molded into an @package
	  # 
	  set ft [::xotcl::metadataAnalyzer::FileToken allinstances]
	  if {[llength $ft] > 1} {
	    error "Too many xodoc file tokens processed. Expecting just one!"
	  }
	  
	  :@namespace "::xotcl"
	#  ::nx::doc::QualifierTag mixin add ::nx::doc::ContainerEntity::Resolvable
	#  ::nx::doc::ContainerEntity::Resolvable container $project
	  
	#  foreach {attr part_class} [$project part_attributes] {
	#    $part_class class mixin add ::nx::doc::ContainerEntity::Containable
	#    $part_class container $project
	#  }
	  
	  set partof [current]
	  if {$ft ne ""} {
	    set pkg [$ft emit [current]]
	    lappend provided_entities $pkg
	    set partof $pkg
	  }
	  
	  foreach token [::xotcl::metadataAnalyzer::ObjToken allinstances] {
	    lappend provided_entities [$token emit $partof]
	  }

	}
      } 
      return $provided_entities
    }
  }
}
