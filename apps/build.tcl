# linux: http://kitcreator.rkeene.org/kits/840dec4286102c869d85bae3b0dcd32565e7bf12/tclkit
# osx: http://kitcreator.rkeene.org/kits/6967b89da1f6af7b12cdc82819f3bdb13a661242/tclkit

package require http
package require tar

proc ::build {HOMEDIR TCLTAG args} {
  set tarball "tcl.tar.gz"

  cd $HOMEDIR
  
  set fh [open $tarball wb+]
  try {

    ::http::geturl http://core.tcl.tk/tcl/tarball/$tarball?uuid=$TCLTAG \
        -binary true \
        -channel $fh

    seek $fh 0
    zlib push gunzip $fh
    ::tar::untar $fh -chan
    
  } on error {e opts} {
    file delete -force tcl
    return -options $opts $e
  } finally {
    close $fh
    file delete $tarball
  }
  
  # exec tar -xzf tcl.tar.gz
  # https://stackoverflow.com/questions/22333745/how-does-tcl-exec-work-exactly
  
  set tclDir [file normalize [file join tcl unix]]
  set buildDir [pwd]
  
  cd $tclDir
  exec >@stdout 2>@stderr [file join [pwd] configure] --libdir=$tclDir
  exec >@stdout 2>@stderr make
  
  set tclSh [file join $tclDir tclsh]
  
  cd $::env(TRAVIS_BUILD_DIR)
  exec >@stdout 2>@stderr [file join [pwd] configure] --with-tcl=$tclDir
  exec >@stdout 2>@stderr make test
}

puts ===$::argv
::build {*}$::argv


# Local variables:
#    mode: tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:










