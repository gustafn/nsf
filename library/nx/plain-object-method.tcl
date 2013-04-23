package provide nx::plain-object-method 1.0

namespace eval ::nx {

  nx::Object eval {

    :public method method {      
      name arguments:parameter,0..* -returns body -precondition -postcondition
    } {
      puts stderr "LEGACY CMD: [self] [current method] [current args]"
      :public object method {*}[current args]
    }

    :public method alias args {
      puts stderr "LEGACY CMD: [self] [current method] [current args]"
      :public object alias {*}$args
    }
    :public method forward args {
      puts stderr "LEGACY CMD: [self] [current method] [current args]"
      :public object forward {*}$args
    }

    :public method mixin args {
      puts stderr "LEGACY CMD: [self] [current method] [current args]"
      :object mixin {*}$args
    }

    :public method filter args {
      puts stderr "LEGACY CMD: [self] [current method] [current args]"
      :object filter {*}$args
    }

    :public alias "info method"           ::nsf::methods::object::info::method
    :public alias "info methods"          ::nsf::methods::object::info::methods
    :public alias "info filter guard"     ::nsf::methods::object::info::filterguard
    :public alias "info filter methods"   ::nsf::methods::object::info::filtermethods
    :public alias "info mixin guard"      ::nsf::methods::object::info::mixinguard
    :public alias "info mixin classes"    ::nsf::methods::object::info::mixinclasses

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