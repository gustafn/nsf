package require XOTcl 2.0
package provide nx::serializer 1.0

# For the time being, we require classical XOTcl.

# TODO: separate into two packages (i.e. make one XOTcl specific
# serializer package, and (a) load this package on a load of this
# package (when ::xotcl::Object is defined), and (b) load it from
# "xotcl1.tcl", when the serializer is alreaded loaded.

namespace eval ::nx::serializer {
  namespace eval ::xotcl {} ;# just to make mk_pkgIndex happy
  namespace import ::xotcl::* ;# just needed for the time being for @
  namespace import -force ::nx::*

  @ @File {
    description {
      This package provides the class Serializer, which can be used to
      generate a snapshot of the current state of the workspace
      in the form of XOTcl source code.
    }
    authors {
      Gustaf Neumann, Gustaf.Neumann@wu-wien.ac.at
    }
  }
  
  @ Serializer proc all {
		 ?-ignoreVarsRE&nbsp;RE? 
		 "provide regular expression; matching vars are ignored"
		 ?-ignore&nbsp;obj1&nbsp;obj2&nbsp;...? 
		 "provide a list of objects to be omitted"} {
    Description {
      Serialize all objects and classes that are currently 
      defined (except the specified omissions and the current
	       Serializer object). 
      <p>Examples:<@br>
      <@pre class='code'>Serializer all -ignoreVarsRE {::b$}</@pre>
      Do not serialize any instance variable named b (of any object).<p>
      <@pre class='code'>Serializer all -ignoreVarsRE {^::o1::.*text.*$|^::o2::x$}</@pre>
      Do not serialize any variable of c1 whose name contains 
      the string "text" and do not serialze the variable x of o2.<p>
      <@pre class='code'>Serializer all -ignore obj1 obj2 ... </@pre>
      do not serizalze the specified objects
    }
    return "script"
  }
  
  @ Serializer proc deepSerialize {
		   ?-ignoreVarsRE&nbsp;RE? 
		   "provide regular expression; matching vars are ignored"
		   ?-ignore&nbsp;obj1&nbsp;obj2&nbsp;...? 
		   "provide a list of objects to be omitted"
		   ?-map&nbsp;list? "translate object names in serialized code"
		   objs "Objects to be serialized"
				 } {
    Description {
      Serialize object with all child objects (deep operation) 
      except the specified omissions. For the description of 
      <@tt>ignore</@tt> and <@tt>igonoreVarsRE</@tt> see 
      <@tt>Serizalizer all</@tt>. <@tt>map</@tt> can be used
      in addition to provide pairs of old-string and new-string
      (like in the tcl command <@tt>string map</@tt>). This option
      can be used to regenerate the serialized object under a different
      object or under an different name, or to translate relative
      object names in the serialized code.<p>
      
      Examples:  
      <@pre class='code'>Serializer deepSerialize -map {::a::b ::x::y} ::a::b::c</@pre>
      Serialize the object <@tt>c</@tt> which is a child of <@tt>a::b</@tt>; 
      the object will be reinitialized as object <@tt>::x::y::c</@tt>,
      all references <@tt>::a::b</@tt> will be replaced by <@tt>::x::y</@tt>.<p>
      
      <@pre class='code'>Serializer deepSerialize -map {::a::b [self]} ::a::b::c</@pre>
      The serizalized object can be reinstantiated under some current object,
      under which the script is evaluated.<p>
      
      <@pre class='code'>Serializer deepSerialize -map {::a::b::c ${var} ::a::b::c}</@pre>
      The serizalized object will be reinstantiated under a name specified
      by the variable <@tt>var<@tt> in the recreation context.
    }
    return "script"
  }
  
  @ Serializer proc methodSerialize {
		     object "object or class"
		     method "name of method"
		     prefix "either empty or 'inst' (latter for instprocs)"
				   } {
    Description {
      Serialize the specified method. In order to serialize 
      an instproc, <@tt>prefix</@tt> should be 'inst'; to serialze
      procs, it should be empty.<p> 
      
      Examples:
      <@pre class='code'>Serializer methodSerialize Serializer deepSerialize ""</@pre>
      This command serializes the proc <@tt>deepSerialize</@tt> 
      of the Class <@tt>Serializer</@tt>.<p>
      
      <@pre class='code'>Serializer methodSerialize Serializer serialize inst</@pre>
      This command serializes the instproc <@tt>serialize</@tt> 
      of the Class <@tt>Serializer</@tt>.<p>
    }
    return {Script, which can be used to recreate the specified method}
  }
  @ Serializer proc exportMethods {
	list "list of methods of the form 'object proc|instproc methodname'" 
      } {
    Description {
      This method can be used to specify methods that should be
      exported in every <@tt>Serializer all<@/tt>. The rationale
      behind this is that the serializer does not serialize objects
      from the namespaces of the basic object systems, which are 
      used for the object system internals and volatile objects. 

      TODO
      It is however often useful to define
      methods on ::xotcl::Class or ::xotcl::Objects, which should
      be exported. One can export procs, instprocs, forward and instforward<p>
      Example:
      <@pre class='code'>      Serializer exportMethods {
	::xotcl::Object instproc __split_arguments
	::xotcl::Object instproc __make_doc
	::xotcl::Object instproc ad_proc
	::xotcl::Class  instproc ad_instproc
	::xotcl::Object forward  expr
      }<@/pre>
    }
  }
  
  
  @ Serializer instproc serialize {entity "Object or Class"} {
    Description {
      Serialize the specified object or class.
    }
    return {Object or Class with all currently defined methods, 
      variables, invariants, filters and mixins}
  }

  ###########################################################################
  # Serializer Class, independent from Object System
  ###########################################################################

  Class create Serializer -parameter {ignoreVarsRE} {
    
    :method ignore args {
      # Ignore the objects passed via args.
      # :skip is used for filtering only in the topological sort.
      foreach element $args { 
        foreach o [Serializer allChildren $element] {
          set :skip($o) 1
        }
      }
    }
    
    :method init {} {
      # Never serialize the (volatile) serializer object
      :ignore [self]
    }

    :method warn msg {
      if {[info command ns_log] ne ""} {
        ns_log Notice $msg
      } else {
        puts stderr "!!! $msg"
      }
    }

    :method addPostCmd {cmd} {
      if {$cmd ne ""} {append :post_cmds $cmd "\n"}
    }
    
    :method setObjectSystemSerializer {o serializer} {
      #puts stderr "set :serializer($o) $serializer"
      set :serializer($o) $serializer
    }

    :method isExportedObject {o} {
      # Check, whether o is exported. For exported objects.
      # we export the object tree.
      set oo $o
      while {1} {
        if {[::nx::core::existsvar [self class] exportObjects($o)]} {
          return 1
        }
        # we do this for object trees without object-less namespaces
        if {![::nx::core::objectproperty $o object]} {
          return 0
        }
        set o [$o info parent]
      }
    }
    
    :method topoSort {set all} {
      if {[array exists :s]} {array unset :s}
      if {[array exists :level]} {array unset :level}

      # TODO generalize?
      set ns_excluded(::ns) 1
      foreach c $set {
	set ns [namespace qualifiers $c]
        if {!$all &&
            [info exists ns_excluded($ns)] && 
            ![:isExportedObject $c]} continue
        if {[info exists :skip($c)]} continue
        set :s($c) 1
      }
      set stratum 0
      while {1} {
        set set [array names :s]
        if {[llength $set] == 0} break
        incr stratum
        # :warn "$stratum set=$set"
        set :level($stratum) {}
        foreach c $set {
          set oss [set :serializer($c)]
          if {[$oss needsNothing $c [self]]} {
            lappend :level($stratum) $c
          }
        }
        if {[set :level($stratum)] eq ""} {
          set :level($stratum) $set
          :warn "Cyclic dependency in $set"
        }
        foreach i [set :level($stratum)] {unset :s($i)}
      }
    }

    :method needsOneOf list {
      foreach e $list {if {[info exists :s($e)]} {return 1}}
      return 0
    }
    
    :method serialize-objects {list all} {
      set :post_cmds ""

      # register for introspection purposes "trace" under a different
      # name for every object system
      foreach oss [ObjectSystemSerializer info instances] {
        $oss registerTrace 1
      }

      :topoSort $list $all
      #foreach i [lsort [array names :level]] { :warn "$i: [set :level($i)]"}
      set result ""
      foreach l [lsort -integer [array names :level]] {
        foreach i [set :level($l)] {
          #.warn "serialize $i"
          #append result "# Stratum $l\n"
          set oss [set :serializer($i)]
          append result [$oss serialize $i [self]] \n
        }
      }
      foreach e $list {
        set namespace($e) 1
        set namespace([namespace qualifiers $e]) 1
      }
      # remove "trace" from all object systems
      foreach oss [ObjectSystemSerializer info instances] {
        $oss registerTrace 0
      }
      
      # Handling of variable traces: traces might require a 
      # different topological sort, which is hard to handle.
      # Similar as with filters, we deactivate the variable
      # traces during initialization. This happens by
      # (1) replacing the next's trace method by a no-op
      # (2) collecting variable traces through collect-var-traces
      # (3) re-activating the traces after variable initialization
      
      set exports ""
      set pre_cmds ""
      
      # delete ::xotcl from the namespace list, if it exists...
      #catch {unset namespace(::xotcl)}
      catch {unset namespace(::ns)}
      foreach ns [array name namespace] {
        if {![namespace exists $ns]} continue
        if {![::nx::core::objectproperty $ns object]} {
          append pre_cmds "namespace eval $ns {}\n"
        } elseif {$ns ne [namespace origin $ns] } {
          append pre_cmds "namespace eval $ns {}\n"
        }
        set exp [namespace eval $ns {namespace export}]
        if {$exp ne ""} {
          append exports "namespace eval $ns {namespace export $exp}" \n
        }
      }
      return $pre_cmds$result${:post_cmds}$exports
    }

    :method deepSerialize o {
      # assumes $o to be fully qualified
      set instances [Serializer allChildren $o] 
      foreach oss [ObjectSystemSerializer info instances] {
        $oss registerSerializer [self] $instances
      }
      :serialize-objects $instances 1
    }

    ###############################
    # class object specfic methods
    ###############################

    :object method allChildren o {
      # return o and all its children fully qualified
      set set [::nx::core::dispatch $o -objscope ::nx::core::current]
      foreach c [$o info children] {
        lappend set {*}[:allChildren $c]
      }
      return $set
    }

    :object method exportMethods list {
      foreach {o p m} $list {set :exportMethods([list $o $p $m]) 1}
    }

    :object method exportObjects list {
      foreach o $list {set :exportObjects($o) 1}
    }

    :object method exportedMethods {} {array names :exportMethods}
    :object method exportedObjects {} {array names :exportObjects}

    :object method resetPattern {} {array unset :ignorePattern}
    :object method addPattern {p} {set :ignorePattern($p) 1}
    
    :object method checkExportedMethods {} {
      foreach k [array names :exportMethods] {
        foreach {o p m} $k break
        set ok 0
        foreach p [array names :ignorePattern] {
          if {[string match $p $o]} {
            set ok 1; break
          }
        }
        if {!$ok} {
          error "method export is only for classes in\
		[join [array names :ignorePattern] {, }] not for $o"
        }
      }
    }

    :object method checkExportedObject {} {
      foreach o [array names :exportObjects] {
        if {![::nx::core::objectproperty $o object]} {
          puts stderr "Serializer exportObject: ignore non-existing object $o"
          unset :exportObjects($o)
        } else {
          # add all child objects
          foreach o [:allChildren $element] {
            set :exportObjects($o) 1
          }
        }
      }
    }

    :object method all {-ignoreVarsRE -ignore} {

      # don't filter anything during serialization
      set filterstate [::nx::core::configure filter off]
      set s [:new -childof [self] -volatile]
      if {[info exists ignoreVarsRE]} {$s ignoreVarsRE $ignoreVarsRE}
      if {[info exists ignore]} {$s ignore $ignore}

      set r [subst {
        set ::xotcl::__filterstate \[::nx::core::configure filter off\]
        #::nx::Slot mixin add ::nx::Slot::Nocheck
        ::nx::core::configure softrecreate [::nx::core::configure softrecreate]
        ::nx::core::setExitHandler [list [::nx::core::getExitHandler]]
      }]\n
      :resetPattern
      set instances [list]
      foreach oss [ObjectSystemSerializer info instances] {
        append r [$oss serialize-all-start $s]
        lappend instances {*}[$oss instances $s]
      }

      # provide error messages for invalid exports
      :checkExportedMethods

      # export the objects and classes
      #$s warn "export objects = [array names :exportObjects]"
      #$s warn "export objects = [array names :exportMethods]"
      
      append r [$s serialize-objects $instances 0] 

      foreach oss [ObjectSystemSerializer info instances] {
        append r [$oss serialize-all-end $s]
      }

      append r {
        #::nx::Slot mixin delete ::nx::Slot::Nocheck
        ::nx::core::configure filter $::xotcl::__filterstate
        unset ::xotcl::__filterstate
      }
      ::nx::core::configure filter $filterstate
      return $r
    }

    :object method methodSerialize {object method prefix} {
      set s [:new -childof [self] -volatile]
      concat $object [$s method-serialize $object $method $prefix]
    }

    :object method deepSerialize {-ignoreVarsRE -ignore -map args} {
      :resetPattern
      set s [:new -childof [self] -volatile]
      if {[info exists ignoreVarsRE]} {$s ignoreVarsRE $ignoreVarsRE}
      if {[info exists ignore]} {$s ignore $ignore}
      
      foreach o $args {
        append r [$s deepSerialize [$o]]
      }
      if {[info exists map]} {return [string map $map $r]}
      return $r
    }

    # include Serializer in the serialized code
    :exportObjects [self]
    
  }

  
  ###########################################################################
  # Object System specific serializer
  ###########################################################################

  Class create ObjectSystemSerializer {
    
    :method init {} {
      # Include object system serializers and the meta-class in "Serializer all"
      Serializer exportObjects [self class]
      Serializer exportObjects [self]
    }

    #
    # Methods to be executed at the begin and end of serialize all
    #
    :method serialize-all-start {s} {
      :getExported
      return [:serializeExportedMethods $s]
    }

    :method serialize-all-end {s} {
      set cmd ""
      foreach o [list ${:rootClass} ${:rootMetaClass}] {
        append cmd \
            [:frameWorkCmd ::nx::core::relation $o object-mixin] \
            [:frameWorkCmd ::nx::core::relation $o class-mixin] \
            [:frameWorkCmd ::nx::core::assertion $o object-invar] \
            [:frameWorkCmd ::nx::core::assertion $o class-invar]
      }
      return $cmd
    }
    
    :method registerTrace {on} {
      if {$on} {
        ::nx::core::alias ${:rootClass}  __trace__ -objscope ::trace
      } else {
        ::nx::core::method ${:rootClass} __trace__ {} {}
      }
    }

    #
    # Handle association between objects and responsible serializers
    #
    :method registerSerializer {s instances} {
      # Communicate responsibility to serializer object $s
      foreach i $instances {
        if {![::nx::core::objectproperty $i type ${:rootClass}]} continue
        $s setObjectSystemSerializer $i [self]
      }
    }

    :method instances {s} {
      # Compute all instances, for which we are responsible and
      # notify serializer object $s
      set instances [list]
      foreach i [${:rootClass} info instances -closure] {
	if {[:matchesIgnorePattern $i] && ![$s isExportedObject $i]} {
          continue
        }
        $s setObjectSystemSerializer $i [self]
        lappend instances $i
      }
      #$s warn "[self] handled instances: $instances"
      return $instances
    }

    :method getExported {} {
      #
      # get exported objects and methods from main Serializer for
      # which this object specific serializer is responsible
      #
      foreach k [Serializer exportedMethods] {
        foreach {o p m} $k break
	if {![::nx::core::objectproperty $o object]} {
	  puts stderr "Warning: $o is not an object"
	} elseif {[::nx::core::objectproperty $o type ${:rootClass}]} {set :exportMethods($k) 1}
      }
      foreach o [Serializer exportedObjects] {
	if {![::nx::core::objectproperty $o object]} {
	  puts stderr "Warning: $o is not an object"
	} elseif {[::nx::core::objectproperty $o type ${:rootClass}]} {set :exportObjects($o) 1}
      }
      foreach p [array names :ignorePattern] {Serializer addPattern $p}
    }
    

    ###############################
    # general method serialization
    ###############################    

    :method classify {o} {
      if {[::nx::core::objectproperty $o type ${:rootMetaClass}]} \
          {return Class} {return Object}
    }

    :method collectVars o {
      set setcmd [list]
      foreach v [lsort [$o info vars]] {
        if {![info exists :ignoreVarsRE] || ![regexp [set :ignoreVarsRE] ${o}::$v]} {
          if {[$o eval [list ::array exists :$v]]} {
            lappend setcmd [list array set :$v [$o eval [list array get :$v]]]
          } else {
            lappend setcmd [list set :$v [::nx::core::setvar $o $v]]
          }
        }
      }
      return $setcmd
    }

    :method frameWorkCmd {cmd o relation -unless} {
      set v [$cmd $o $relation]
      if {$v eq ""} {return ""}
      if {[info exists unless] && $v eq $unless} {return ""}
      return [list $cmd $o $relation $v]\n
    }

    :method serializeExportedMethods {s} {
      set r ""
      foreach k [array names :exportMethods] {
        foreach {o p m} $k break
        if {![:methodExists $o $p $m]} {
          $s warn "Method does not exist: $o $p $m"
          continue
        }
        append methods($o) [:serializeExportedMethod $o $p $m]\n
      }
      foreach o [array names methods] {set ($o) 1}
      foreach o [list ${:rootClass} ${:rootMetaClass}] {
        if {[info exists ($o)]} {unset ($o)}
      }
      foreach o [concat ${:rootClass} ${:rootMetaClass} [array names ""]] {
        if {![info exists methods($o)]} continue
        append r \n $methods($o)
      }
      #puts stderr "[self] ... exportedMethods <$r\n>"
      return "$r\n"
    }

    ###############################
    # general object serialization
    ###############################

    :method serialize {objectOrClass s} {
      :[:classify $objectOrClass]-serialize $objectOrClass $s
    }

    :method matchesIgnorePattern {o} {
      foreach p [array names :ignorePattern] {
        if {[string match $p $o]} {return 1}
      }
      return 0
    }

    :method collect-var-traces {o s} {
      foreach v [$o info vars] {
        set t [$o __trace__ info variable $v]
        if {$t ne ""} {
          foreach ops $t { 
            foreach {op cmd} $ops break
            # save traces in post_cmds
            $s addPostCmd [list $o trace add variable $v $op $cmd]

            # remove trace from object
            $o trace remove variable $v $op $cmd
          }
        }
      }
    }

    ###############################
    # general dependency handling 
    ###############################

    :method needsNothing {x s} {
      return [:[:classify $x]-needsNothing $x $s]
    }

    :method Class-needsNothing {x s} {
      if {![:Object-needsNothing $x $s]} {return 0}
      set scs [$x info superclass]
      if {[$s needsOneOf $scs]} {return 0}
      if {[$s needsOneOf [::nx::core::relation $x class-mixin]]} {return 0}
      foreach sc $scs {if {[$s needsOneOf [$sc info slots]]} {return 0}}
      return 1
    }

    :method Object-needsNothing {x s} {
      set p [$x info parent]
      if {$p ne "::"  && [$s needsOneOf $p]} {return 0}
      if {[$s needsOneOf [$x info class]]}  {return 0}
      if {[$s needsOneOf [[$x info class] info slots]]}  {return 0}
      return 1
    }
    
  }

  ###########################################################################
  # next specific serializer
  ###########################################################################

  ObjectSystemSerializer create Serializer2 {
    
    set :rootClass ::nx::Object
    set :rootMetaClass ::nx::Class
    array set :ignorePattern [list "::nx::*" 1 "::xotcl::*" 1]

    :method serialize-all-start {s} {
      if {[info command ::Object] ne "" && [namespace origin ::Object] eq "::nx::Object"} {
        set intro "package require nx; namespace import -force ::nx::*"
      } else {
        set intro ""
      }
      return "$intro\n[next]"
    }

    ###############################
    # next method serialization
    ###############################

    :method methodExists {object kind name} {
      expr {[$object info method type $name] != ""}
    }

    :method serializeExportedMethod {object kind name} {
      # todo: object modifier is missing
      return [:method-serialize $object $name ""]
    }

    :method method-serialize {o m modifier} {
      if {![::nx::core::objectproperty $o class]} {set modifier ""}
      return [$o {*}$modifier info method definition $m]
    }

    ###############################
    # next object serialization
    ###############################

    :method Object-serialize {o s} {
      :collect-var-traces $o $s
      append cmd [list [$o info class] create \
                      [::nx::core::dispatch $o -objscope ::xotcl::self]]

      append cmd " -noinit\n"
      foreach i [lsort [::nx::core::cmd::ObjectInfo::methods $o]] {
        append cmd [:method-serialize $o $i "object"] "\n"
      }
      append cmd \
          [list $o eval [join [:collectVars $o] "\n   "]]\n \
          [:frameWorkCmd ::nx::core::relation $o object-mixin] \
          [:frameWorkCmd ::nx::core::assertion $o object-invar]

      if {[::nx::core::objectproperty $o type ::nx::Slot]} {
        # Slots needs to be initialized to ensure
        # __invalidateobjectparameter to be called
        append cmd [list $o eval :init] \n
      }

      $s addPostCmd [:frameWorkCmd ::nx::core::relation $o object-filter]
      return $cmd
    }

    ###############################
    # next class serialization
    ###############################
    
    :method Class-serialize {o s} {

      set cmd [:Object-serialize $o $s]
      foreach i [lsort [::nx::core::cmd::ClassInfo::methods $o]] {
        append cmd [:method-serialize $o $i ""] "\n"
      }
      append cmd \
          [:frameWorkCmd ::nx::core::relation $o superclass -unless ${:rootClass}] \
          [:frameWorkCmd ::nx::core::relation $o class-mixin] \
          [:frameWorkCmd ::nx::core::assertion $o class-invar]
      
      $s addPostCmd [:frameWorkCmd ::nx::core::relation $o class-filter]
      return $cmd\n
    }

    # register serialize a global method
    ::nx::Object method serialize {} {
      ::Serializer deepSerialize [self]
    }
    
  }



  ###########################################################################
  # XOTcl specific serializer
  ###########################################################################

  ObjectSystemSerializer create Serializer1 {
    
    set :rootClass ::xotcl::Object
    set :rootMetaClass ::xotcl::Class
    #array set :ignorePattern [list "::xotcl::*" 1]
    array set :ignorePattern [list "::nx::core::*" 1 "::xotcl::*" 1]


    :method serialize-all-start {s} {
      set intro "package require XOTcl 2.0"
      if {[info command ::Object] ne "" && [namespace origin ::Object] eq "::xotcl::Object"} {
        append intro "\nnamespace import -force ::xotcl::*"
      }
      return "$intro\n::xotcl::Object instproc trace args {}\n[next]"
    }

    :method serialize-all-end {s} {
      return "[next]\n::nx::core::alias ::xotcl::Object trace -objscope ::trace\n"
    }


    ###############################
    # XOTcl method serialization
    ###############################

    :method methodExists {object kind name} {
      switch $kind {
        proc - instproc {
          return [expr {[$object info ${kind}s $name] ne ""}]
        }
        forward - instforward {
          return [expr {[$object info ${kind} $name] ne ""}]
        }
      }
    }

    :method serializeExportedMethod {object kind name} {
      set code ""
      switch $kind {
        proc - instproc {
          if {[$object info ${kind}s $name] ne ""} {
            set prefix [expr {$kind eq "proc" ? "" : "inst"}] 
            set code [:method-serialize $object $name $prefix]\n
          }
        }
        forward - instforward {
          if {[$object info $kind $name] ne ""} {
            set code [concat [list $object] $kind $name [$object info $kind -definition $name]]\n
          }
        }
      }
      return $code
    }

    :method method-serialize {o m prefix} {
      set arglist [list]
      foreach v [$o info ${prefix}args $m] {
        if {[$o info ${prefix}default $m $v x]} {
          lappend arglist [list $v $x] } {lappend arglist $v}
      }
      lappend r $o ${prefix}proc $m \
          [concat [$o info ${prefix}nonposargs $m] $arglist] \
          [$o info ${prefix}body $m]
      foreach p {pre post} {
        if {[$o info ${prefix}$p $m] ne ""} {lappend r [$o info ${prefix}$p $m]}
      }
      return $r
    }

    ###############################
    # XOTcl object serialization
    ###############################

    :method Object-serialize {o s} {
      :collect-var-traces $o $s
      append cmd [list [$o info class] create [::nx::core::dispatch $o -objscope ::xotcl::self]]
      # slots needs to be initialized when optimized, since
      # parametercmds are not serialized
      append cmd " -noinit\n"
      foreach i [::nx::core::cmd::ObjectInfo::methods $o -methodtype scripted] {
        append cmd [:method-serialize $o $i ""] "\n"
      }
      foreach i [::nx::core::cmd::ObjectInfo::methods $o -methodtype forward] {
        append cmd [concat [list $o] forward $i [$o info forward -definition $i]] "\n"
      }
      foreach i [::nx::core::cmd::ObjectInfo::methods $o -methodtype setter] {
        append cmd [list $o parametercmd $i] "\n"
      }
      append cmd \
          [list $o eval [join [:collectVars $o] "\n   "]] \n \
          [:frameWorkCmd ::nx::core::relation $o object-mixin] \
          [:frameWorkCmd ::nx::core::assertion $o object-invar]

      $s addPostCmd [:frameWorkCmd ::nx::core::relation $o object-filter]

      return $cmd
    }

    ###############################
    # XOTcl class serialization
    ###############################
    
    :method Class-serialize {o s} {
      set cmd [:Object-serialize $o $s]
      foreach i [$o info instprocs] {
        append cmd [:method-serialize $o $i inst] "\n"
      }
      foreach i [$o info instforward] {
        append cmd [concat [list $o] instforward $i [$o info instforward -definition $i]] "\n"
      }
      foreach i [$o info instparametercmd] {
        append cmd [list $o instparametercmd $i] "\n"
      }
      # provide limited support for exporting aliases for XOTcl objects
      foreach i [::nx::core::cmd::ClassInfo::methods $o -methodtype alias] {
        set xotcl2Def [::nx::core::cmd::ClassInfo::method $o definition $i]
        set objscope   [lindex $xotcl2Def end-2]
        set methodName [lindex $xotcl2Def end-1]
        set cmdName    [lindex $xotcl2Def end]
        if {$objscope ne "-objscope"} {set objscope ""}
        append cmd [list ::nx::core::alias $o $methodName {*}$objscope $cmdName]\n
      }
      append cmd \
          [:frameWorkCmd ::nx::core::relation $o superclass -unless ${:rootClass}] \
          [:frameWorkCmd ::nx::core::relation $o class-mixin] \
          [:frameWorkCmd ::nx::core::assertion $o class-invar]

      $s addPostCmd [:frameWorkCmd ::nx::core::relation $o class-filter]
      return $cmd
    }

    # register serialize a global method for XOTcl
    ::xotcl::Object instproc serialize {} {
      ::Serializer deepSerialize [self]
    }

    
    # include this method in the serialized code
    #Serializer exportMethods {
    #  ::xotcl::Object instproc contains
    #}
  }


  namespace export Serializer
  namespace eval :: "namespace import -force [namespace current]::*"
}
