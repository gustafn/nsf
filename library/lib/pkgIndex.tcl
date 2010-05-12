# Tcl package index file, version 1.1
# This file is generated by the "pkg_mkIndex -direct" command
# and sourced either when an application starts up or
# by a "package unknown" script.  It invokes the
# "package ifneeded" command to set up package-related
# information so that packages will be loaded automatically
# in response to "package require" commands.  When this
# script is sourced, the variable $dir must contain the
# full path name of this file's directory.

package ifneeded XOTcl 2.0 [list source [file join $dir xotcl1.xotcl]]
package ifneeded next::doc 0.1 [list source [file join $dir doc-tools.xotcl]]
package ifneeded xotcl::htmllib 0.1 [list source [file join $dir htmllib.xotcl]]
package ifneeded xotcl::metadataAnalyzer 0.84 [list source [file join $dir metadataAnalyzer.xotcl]]
package ifneeded xotcl::mixinStrategy 0.9 [list source [file join $dir mixinStrategy.xotcl]]
package ifneeded xotcl::package 0.91 [list source [file join $dir package.xotcl]]
package ifneeded xotcl::script 0.9 [list source [file join $dir Script.xotcl]]
package ifneeded xotcl::staticMetadataAnalyzer 0.84 [list source [file join $dir staticMetadata.xotcl]]
package ifneeded xotcl::test 2.0 [list source [file join $dir test.xotcl]]
package ifneeded xotcl::trace 0.91 [list source [file join $dir trace.xotcl]]
package ifneeded xotcl::upvar-compat 1.0 [list source [file join $dir upvarcompat.xotcl]]
package ifneeded xotcl::wafecompat 0.9 [list source [file join $dir wafecompat.tcl]]
package ifneeded xotcl::xodoc 0.84 [list source [file join $dir xodoc.xotcl]]
package ifneeded xotcl::package 0.91 [list source [file join $dir package.xotcl]]
