# Tcl package index file, version 1.1
# This file is generated by the "pkg_mkIndex -direct" command
# and sourced either when an application starts up or
# by a "package unknown" script.  It invokes the
# "package ifneeded" command to set up package-related
# information so that packages will be loaded automatically
# in response to "package require" commands.  When this
# script is sourced, the variable $dir must contain the
# full path name of this file's directory.

package ifneeded xotcl::scriptCreation::recoveryPoint 0.8 [list source [file join $dir RecoveryPoint.xotcl]]
package ifneeded xotcl::scriptCreation::scriptCreator 0.8 [list source [file join $dir ScriptCreator.xotcl]]
package ifneeded xotcl::serializer 0.9 [list source [file join $dir Serializer.xotcl]]
