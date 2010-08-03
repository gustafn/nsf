### inEachDir changes now to each directory
### install clears tgarget directory before installing
### Object file added (for better -n processing)
lappend auto_path ..

package require nx
namespace import -force ::nx::*

###
Object create make {
  #
  # shared lib add files for pkgIndex.tcl
  #
  :method mkIndex {name} {
    #puts stderr "+++ mkIndex in [pwd]"
    set fls {}
    foreach f [glob -nocomplain *tcl] {
      if {![file isdirectory $f]} {
        set F [open $f]; set c [read $F]; close $F
        if {[string match "*package provide*" $c]} { lappend fls $f }
      }
    }

    set so [glob -nocomplain *[info sharedlibextension]]
    set version $::nx::core::version
    # loading libnext into nextsh might cause problems on some systems
    foreach lib [list libnext$version[info sharedlibextension] \
                     next$version.dll] {
      set p [lsearch -exact $so $lib]
      if {$p != -1} {
        set so [lreplace $so $p $p]
        #puts stderr "new so=<$so>"
      }
    }
    #puts stderr "[pwd]: call so=<$so>"
    set fls [concat $fls $so]
    
    if {$fls ne ""} {
      if {[file exists pkgIndex.tcl]} {
        file delete -force pkgIndex.tcl
      }
      #puts stderr "callinglevel <[self callinglevel]> $fls"
      #puts stderr "[pwd]:\n\tcall eval pkg_mkIndex -verbose -direct . $fls"
      if {[catch {pkg_mkIndex -verbose -direct . {*}$fls} errs]} {
        puts stderr "!!! $errs"
      }
      #puts stderr "[pwd] done"
    }
    
    foreach addFile [glob -nocomplain *.add] {
      if {[file exists $addFile]} {
        puts stderr "Appending $addFile to pkgIndex.tcl in [pwd]"
        set OUT [file open pkgIndex.tcl a]
        set IN [file open $addFile]
        puts -nonewline $OUT [read $IN]
        close $IN; close $OUT
      }
    }
    #puts stderr "+++ mkIndex name=$name, pwd=[pwd] DONE"
  }

  :method inEachDir {path cmd} {
    #puts stderr "[pwd] inEachDir $path  [file isdirectory $path]"
    if { [file isdirectory $path] 
         && ![string match *CVS $path]
         && ![string match *SCCS $path]
         && ![string match *Attic $path]
         && ![string match *dbm* $path]
       } {
      set olddir [pwd]
      cd $path
      make {*}$cmd $path
      set files [glob -nocomplain *]
      cd $olddir
      foreach p $files { :inEachDir $path/$p $cmd }
      #puts stderr "+++ change back to $olddir"
    }
  }

  :method in {path cmd} {
    if {[file isdirectory $path] && ![string match *CVS $path]} {
      set olddir [pwd]
      cd $path
      make {*}$cmd $path
      cd $olddir
    }
  }
}

### tcl file-command
rename file tcl_file
Object create file {
  :requireNamespace

  array set :destructive {
    atime 0       attributes 0  copy 1       delete 1      dirname 0
    executable 0  exists 0      extension 0  isdirectory 0 isfile 0
    join 0        lstat 0       mkdir 1      mtime 0       nativename 0
    owned 0       pathtype 0    readable 0   readlink 0    rename 1
    rootname 0    size 0        split 0      stat 0        tail 0
    type 0        volumes 0     writable 0
  }

  foreach subcmd [array names :destructive] {
    :method $subcmd args {
      #puts stderr " [pwd] call: '::tcl_file [self proc] $args'"
      ::tcl_file [self proc] {*}$args
    }
  }
}

rename open file::open
proc open {f {mode r}} { file open $f $mode }


### minus n option
Class create make::-n
foreach f [file info methods] {
  if {$f eq "unknown" || $f eq "next" || $f eq "self"} continue
  if {![file exists destructive($f)] || [file eval [list set :destructive($f)]]} {
    #puts stderr destruct=$f
    make::-n method $f args {
	puts "--- [pwd]:\t[self proc] $args"
    }
  } else {
    #puts stderr nondestruct=$f
    make::-n method $f args {
      set r [next]
      #puts "??? [self proc] $args -> {$r}"
      return $r
    }
  }
}

### command line parameters
if {![info exists argv] || $argv eq ""} {set argv -all}
if {$argv eq "-n"} {set argv "-n -all"}

Class create Script {
  :object method create args {
    lappend args {*}$::argv
    set s [next]
    set method [list]
    foreach arg [lrange $args 1 end] {
      switch -glob -- $arg {
        "-all" {$s all}
        "-n" {$s n}
        "-*" {set method [string range $arg 1 end]}
        default {$s $method $arg}
      }
    }
  }

  :method unknown args {
    puts stderr "$::argv0: Unknown option ´-$args´ provided"
  }

  :method n {} {file mixin make::-n}

  :method all {} {make inEachDir . mkIndex}

  :method dir {dirName} {cd $dirName}

  :method target {path} {make eval [list set :target $path]}

  :create main
}

#puts stderr "+++ make.tcl finished."
#if {[set ::tcl_platform(platform)] eq "windows"} {
#  exit
#}
