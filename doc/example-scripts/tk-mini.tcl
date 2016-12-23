#
# Tiny Tk example scriped based on NX.
#
# image::tk-mini.png[]
#

package require Tk
package require nx::trait

nx::Class create MyClass {
  #
  # A sample application class that creates a text entry field bound
  # to an instance variable. When the provided button is pressed, the
  # content of the variable is placed into an additional output label.

  #
  # The callback trait imports methods "callback" and "bindvar":
  #
  :require trait nx::trait::callback

  :public method button-pressed {} {
    # When this method is invoked, the content of the ".label" widget
    # is updated with the content of the instance variable "myvar".
    .label configure -text ${:myvar}
  }

  :method init {} {
    wm geometry . -500+500
    pack [label .title -text "Type something and press the start button ..."]
    pack [entry .text -textvariable [:bindvar myvar]]
    pack [label .label]
    pack [button .button -text start -command [:callback button-pressed]]
  }
}
  
MyClass new 
  
