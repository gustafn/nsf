#
# == Rosetta Example: Singleton 
# For details see http://rosettacode.org/wiki/Singleton
#
# === A Singleton Class
package req nx
package req nx::test

nx::Class create Singleton {
  #
  # We overload the system method "create". In the modified method we
  # save the created instance in the instance variable named
  # "instance"
  #
  :variable instance:object

  :public class method create {args} {
    return [expr {[info exists :instance] ? ${:instance} : [set :instance [next]]}]
  }
}

# === Demonstrating the behavior in a shell: 
#
# Calling  +Singleton new+ multiple times returns always the same object:
? {expr {[Singleton new] eq [Singleton new]}} 1

#
# === A Singleton Meta-class
#
# Alternatively, we can follow a more generic approach and define a
# metaclass which allows to define several application classes as
# singletons. The metaclass has the most general metaclass +nx::Class+
# as superclass. In contrary to the example obove, the +create+ method
# is not defined as a class method, but it will be inherited to its
# instances (to the application classes).
# 
nx::Class create Singleton -superclass nx::Class {
  #
  # We overload the system method "create". In the modified method we
  # save the created instance in the instance variable named
  # "instance"
  #
  :variable instance:object

  :public method create {args} {
    return [expr {[info exists :instance] ? ${:instance} : [set :instance [next]]}]
  }
}

# Create an application class named +Counter+ as a singleton:

? {Singleton create Counter} ::Counter

# Calling  +Counter new+ multiple times returns always the same object:
? {expr {[Counter new] eq [Counter new]}} 1