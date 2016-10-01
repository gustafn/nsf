#
# == Rosetta example: Inheritance/Multiple
#
# Write two classes (or interfaces) Camera and MobilePhone, then write
# a class CameraPhone which is both a Camera and a MobilePhone.
# 
# https://rosettacode.org/wiki/Inheritance/Multiple
#

package req nx
package req nx::test

#
# NX offers class-based and mixin-based multiple inheritance. The
# search order of features (methods, properties) along the class
# hierarchy is computed using a scheme equivalent with L3
# linearization.
#

#
# a) Class-based multiple inheritance
#
nx::Class create Camera
nx::Class create MobilePhone

nx::Class create CameraPhone -superclasses {Camera MobilePhone}

# Show the resulting class search order:
? {CameraPhone info superclasses -closure} {::Camera ::MobilePhone ::nx::Object}
? {[CameraPhone new] info precedence} {::CameraPhone ::Camera ::MobilePhone ::nx::Object}

# b) Mixin-based multiple inheritance

nx::Class create CameraPhone -mixins {Camera MobilePhone}
? {CameraPhone info mixins} {::Camera ::MobilePhone}

# Show the resulting class search order:
? {[CameraPhone new] info precedence} {::Camera ::MobilePhone ::CameraPhone ::nx::Object}
