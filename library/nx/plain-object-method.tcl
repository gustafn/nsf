package provide nx::plain-object-method 1.0

namespace eval ::nx {

  nx::Object eval {

    :public method method {      
      name arguments:parameter,0..* -returns body -precondition -postcondition
    } {
      ::nsf::log warn "LEGACY CMD: [self] [current method] [current args]"
      :public object [current method] {*}[current args]
    }

    :public method alias args {
      ::nsf::log warn "LEGACY CMD: [self] [current method] [current args]"
      :public object [current method] {*}$args
    }
    :public method forward args {
      ::nsf::log warn "LEGACY CMD: [self] [current method] [current args]"
      :public object [current method] {*}$args
    }

    :public method filter args {
      ::nsf::log warn "LEGACY CMD: [self] [current method] [current args]"
      :object [current method] {*}$args
    }

    :public method mixin args {
      ::nsf::log warn "LEGACY CMD: [self] [current method] [current args]"
      :object [current method] {*}$args
    }

    :public method property args {
      ::nsf::log warn "LEGACY CMD: [self] [current method] [current args]"
      :object [current method] {*}$args
    }
    :public method variable args {
      ::nsf::log warn "LEGACY CMD: [self] [current method] [current args]"
      :object [current method] {*}$args
    }

    :public alias "info method"           ::nsf::methods::object::info::method
    :public alias "info methods"          ::nsf::methods::object::info::methods
    :public alias "info filter guard"     ::nsf::methods::object::info::filterguard
    :public alias "info filter methods"   ::nsf::methods::object::info::filtermethods
    :public alias "info mixin guard"      ::nsf::methods::object::info::mixinguard
    :public alias "info mixin classes"    ::nsf::methods::object::info::mixinclasses

    :public method "info slots" args {
      ::nsf::log warn "LEGACY CMD: [self] [current method] [current args]"
      :object [current method] {*}$args
    }

  }


  Object eval {
    #
    # method require, base cases
    #
    :method "require method" {methodName} {
      ::nsf::method::require [::nsf::self] $methodName 1
      return [:info lookup method $methodName]
    }
    #
    # method require, public explicitly
    #
    :method "require public method" {methodName} {
      set result [:require object method $methodName]
      ::nsf::method::property [self] $result call-protected false
      return $result
    }
    #
    # method require, protected explicitly
    #
    :method "require protected method" {methodName} {
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