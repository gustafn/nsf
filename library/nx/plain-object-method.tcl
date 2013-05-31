package provide nx::plain-object-method 1.0

namespace eval ::nx {

  #
  # Define a method to allow configuration for tracing of the
  # convenience methods. Use 
  #
  #    nx::configure plain-object-method-warning on|off
  #
  # for activation/deactivation of tracing
  #
  nx::configure public object method plain-object-method-warning {onoff:boolean,optional} {
    if {[info exists onoff]} {
      set :plain-object-method-warning $onoff
    } else {
      if {[info exists :plain-object-method-warning]} {
	if {${:plain-object-method-warning}} {
	  uplevel {::nsf::log warn "plain object method: [self] [current method] [current args]"}
	}
      }
    }
  }


  nx::Object eval {
    #
    # Definitions redirected to "object"
    #
    foreach m {alias filter forward method mixin property variable} {
      :public method $m {args} {
	nx::configure plain-object-method-warning
	:object [current method] {*}[current args]
      }
    }

    #
    # info subcmmands 
    #
    foreach m {method methods slots variables
      "filter guards" "filter methods"
      "mixin guards" "mixin classes"
    } {
      :public method "info $m" {args} [subst -nocommands {
	nx::configure plain-object-method-warning
	:info object $m {*}[current args]
      }]
    }

  }


  Object eval {
    #
    # method require, base cases
    #
    :method "require method" {methodName} {
      nx::configure plain-object-method-warning
      ::nsf::method::require [::nsf::self] $methodName 1
      return [:info lookup method $methodName]
    }
    #
    # method require, public explicitly
    #
    :method "require public method" {methodName} {
      nx::configure plain-object-method-warning
      set result [:require object method $methodName]
      ::nsf::method::property [self] $result call-protected false
      return $result
    }
    #
    # method require, protected explicitly
    #
    :method "require protected method" {methodName} {
      nx::configure plain-object-method-warning
      set result [:require object method $methodName]
      ::nsf::method::property [self] $result call-protected true
      return $result
    }

    #
    # method require, private explicitly
    #
    :method "require private method" {methodName} {
      set result [:require object method $methodName]
      ::nsf::method::property [self] $result call-private true
      return $result
    }
  }

}