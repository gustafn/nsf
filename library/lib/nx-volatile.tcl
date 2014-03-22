#
# Package to add configure-parameter "-volatile"
#
package require nx
package provide nx::volatile 1.0

namespace eval ::nx {

  ::nx::ObjectParameterSlot create ::nx::Object::slot::volatile -noarg true

  ::nsf::method::create ::nx::Object::slot::volatile assign {object var value} {
    $object ::nsf::methods::object::volatile
  }
  ::nsf::method::create ::nx::Object::slot::volatile get {object var} {
    ::nsf::object::property $object volatile
  }

  # this loop should not be required.
  foreach c [::nx::Object info subclass -closure] {
    ::nsf::parameter:invalidate::classcache $c
  }
  unset -nocomplain c

}

#
# Local variables:
#    mode: tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:
