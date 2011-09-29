# Tcl package index file, version 1.1
# This file is generated by the "pkg_mkIndex -direct -load nsf -load nx" command
# and sourced either when an application starts up or
# by a "package unknown" script.  It invokes the
# "package ifneeded" command to set up package-related
# information so that packages will be loaded automatically
# in response to "package require" commands.  When this
# script is sourced, the variable $dir must contain the
# full path name of this file's directory.

package ifneeded nx::callback 1.0 [list source [file join $dir nx-callback.tcl]]
package ifneeded nx::doc 1.0 [list source [file join $dir nxdoc-core.tcl]]
package ifneeded nx::doc::dc 1.0 [list source [file join $dir nxdoc-dc.tcl]]
package ifneeded nx::doc::html 1.0 [list source [file join $dir nxdoc-html.tcl]]
package ifneeded nx::doc::xodoc 1.0 [list source [file join $dir nxdoc-xodoc.tcl]]
package ifneeded nx::doc::xowiki 1.0 [list source [file join $dir nxdoc-xowiki.tcl]]
package ifneeded nx::pp 1.0 [list source [file join $dir pp.tcl]]
package ifneeded nx::test 1.0 [list source [file join $dir test.tcl]]
package ifneeded nx::trait 0.1 [list source [file join $dir nx-traits.tcl]]
package ifneeded nx::zip 1.1 [list source [file join $dir nx-zip.tcl]]
