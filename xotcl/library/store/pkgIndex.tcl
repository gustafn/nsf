# Tcl package index file, version 1.1
# This file is generated by the "pkg_mkIndex -direct" command
# and sourced either when an application starts up or
# by a "package unknown" script.  It invokes the
# "package ifneeded" command to set up package-related
# information so that packages will be loaded automatically
# in response to "package require" commands.  When this
# script is sourced, the variable $dir must contain the
# full path name of this file's directory.

package ifneeded xotcl::store 0.84 [list source [file join $dir Storage.xotcl]]
package ifneeded xotcl::store::jufgdbm 0.81 [list source [file join $dir JufGdbmStorage.xotcl]]
package ifneeded xotcl::store::mem 0.84 [list source [file join $dir MemStorage.xotcl]]
package ifneeded xotcl::store::multi 0.9 [list source [file join $dir MultiStorage.xotcl]]
package ifneeded xotcl::store::persistence 0.8 [list source [file join $dir Persistence.xotcl]]
package ifneeded xotcl::store::tclgdbm 0.84 [list source [file join $dir TclGdbmStorage.xotcl]]
package ifneeded xotcl::store::textfile 0.84 [list source [file join $dir TextFileStorage.xotcl]]
set __store_dir__ $dir
foreach index [glob -nocomplain [file join $dir * pkgIndex.tcl]] {
  set dir [file dirname $index]
  #puts subdir=$dir,index=$index
  source $index
}
set dir $__store_dir__
unset __store_dir__


