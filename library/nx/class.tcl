package provide nx::class 1.0

namespace eval ::nsf {
  array set ::nsf::methodDefiningMethod {
    class 1
  }
}

namespace eval ::nx {
  nx::Class eval {
    :public alias "class method" ::nx::Object::slot::__object::method

    :public alias "class alias" ::nx::Object::slot::__object::alias
    :public alias "class forward" ::nx::Object::slot::__object::forward
    #:public method "class forward" args {
    #  puts stderr "CLASS CMD: [self] [current method] [current args]"
    #  :public object forward {*}$args
    #}

    :public alias "class info" ::nx::Object::slot::__info

    :public method "class filter" args {
      set what filter
      switch [llength $args] {
	0 {return [::nsf::relation [::nsf::self] object-$what]}
	1 {return [::nsf::relation [::nsf::self] object-$what {*}$args]}
	default {return [::nx::Object::slot::$what [lindex $args 0] \
			     [::nsf::self] object-$what \
			     {*}[lrange $args 1 end]]
	}
      }
    }
    :public method "class mixin" args {
      set what mixin
      switch [llength $args] {
	0 {return [::nsf::relation [::nsf::self] object-$what]}
	1 {return [::nsf::relation [::nsf::self] object-$what {*}$args]}
	default {return [::nx::Object::slot::$what [lindex $args 0] \
			     [::nsf::self] object-$what \
			     {*}[lrange $args 1 end]]
	}
      }
    }
    :public alias "class filterguard" ::nsf::methods::object::filterguard
    :public alias "class mixinguard" ::nsf::methods::object::mixinguard
    
  }

  #
  # provide aliases for "class property" and "class variable"
  #
  ::nx::Class eval {
    :alias "class property" ::nsf::classes::nx::Object::property
    :alias "class variable" ::nsf::classes::nx::Object::variable
  }

  #
  # provide aliases for "class delete"
  #
  ::nx::Class eval {
    :alias "class delete property" ::nx::Object::slot::__delete::property
    :alias "class delete variable" ::nx::Object::slot::__delete::variable
    :alias "class delete method" ::nx::Object::slot::__delete::method
  }

  #
  # info redirector
  #
  ::nx::Class eval {
    :alias "class info" ::nx::Object::slot::__info
  }
  ######################################################################
  # Provide method "require"
  ######################################################################
  Object eval {
    #
    # method require, base cases
    #
    :method "require class method" {methodName} {
      ::nsf::method::require [::nsf::self] $methodName 1
      return [:info lookup method $methodName]
    }
    #
    # method require, public explicitly
    #
    :method "require public class method" {methodName} {
      set result [:require class method $methodName]
      ::nsf::method::property [self] $result call-protected false
      return $result
    }
    #
    # method require, protected explicitly
    #
    :method "require protected class method" {methodName} {
      set result [:require class method $methodName]
      ::nsf::method::property [self] $result call-protected true
      return $result
    }
    #
    # method require, private explicitly
    #
    :method "require private class method" {methodName} {
      set result [:require class method $methodName]
      ::nsf::method::property [self] $result call-private true
      return $result
    }
  }
}