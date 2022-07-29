# w/o tls
# linux: https://kitcreator.rkeene.org/kits/840dec4286102c869d85bae3b0dcd32565e7bf12/tclkit
# macos: https://kitcreator.rkeene.org/kits/6967b89da1f6af7b12cdc82819f3bdb13a661242/tclkit

# w/ tls
# linux: http://kitcreator.rkeene.org/kits/c8fe6fba3323b12b924b4a0716609abbaa00822c/tclkit
# macos: http://kitcreator.rkeene.org/kits/31eaf9ae17e769609700b41d1d3c9abeda27510d/tclkit
# win: http://kitcreator.rkeene.org/kits/32c6369ff6ef02a685b75854237635d4a3d56611/tclkit.exe

package require http
package require tar
package require platform

proc ::build {HOMEDIR BUILDDIR TCLTAG {TOOLCHAIN autoconf-tea}} {

  puts TCLVER=[package req Tcl],[info loaded]
  
  set tarball "tcl.tar.gz"
  set INSTALLDIR [file join $HOMEDIR install]

  cd $HOMEDIR

  set URL https://core.tcl-lang.org/tcl/tarball/$tarball?uuid=$TCLTAG
  
  if {![catch {package require tls}]} {
    
    http::register https 443 [list ::tls::socket -tls1 1]

    set fh [open $tarball wb+]
    try {
      
      ::http::geturl $URL \
          -binary true \
          -channel $fh
      close $fh
      # seek $fh 0
      # zlib push gunzip $fh
      # ::tar::untar $fh -chan

      exec >@stdout 2>@stderr bash -lc "7z x $tarball -so | 7z x -aoa -si -ttar -o tcl"
      
    } on error {e opts} {
      file delete -force tcl
      return -options $opts $e
    } finally {
      catch {close $fh}
      file delete -force $tarball
    }


    
  } else {

    # fall back to using curl
    exec >@stdout 2>@stderr bash -lc "curl -L -k -o $tarball $URL"

    # set fh [open $tarball rb]
    try {
      # zlib push gunzip $fh
      # ::tar::untar $fh -chan
      exec >@stdout 2>@stderr bash -lc "7z x $tarball -so | 7z x -aoa -si -ttar -o tcl"
      
    } finally {
      # close $fh
      file delete -force $tarball
    }
  }

  # exec tar -xzf tcl.tar.gz
  # https://stackoverflow.com/questions/22333745/how-does-tcl-exec-work-exactly

  set dir [expr {[string match "win*" [platform::generic]]?"win":"unix"}]
  
  set tclRoot [file normalize tcl]
  set tclDir [file join $tclRoot $dir]

  # puts pwd([pwd])=[glob *]
  # puts tclDir($tclDir)=[glob $tclDir/*]
  
  set buildDir [pwd]
  
  cd $tclDir

  puts ENV=$::env(PATH)
  puts ENV=$::env(HOME)
  exec >@stdout 2>@stderr bash -lc "echo \$PATH"
  exec >@stdout 2>@stderr bash -lc "cd && pwd"
  exec >@stdout 2>@stderr bash -lc "pwd"
  exec >@stdout 2>@stderr bash -lc "cd && ls -la"
  
  switch -exact -- $TOOLCHAIN {
    autoconf-tea {
      set opts [list --libdir=$tclDir --enable-64bit]
      
      exec >@stdout 2>@stderr bash -lc "./configure $opts"
      exec >@stdout 2>@stderr bash -lc "make"
      
      cd $BUILDDIR
      # puts BUILDDIR=$BUILDDIR,PWD=[pwd],INSTALLDIR=$INSTALLDIR
      # exec >@stdout 2>@stderr bash -lc "./configure --with-tcl=$tclDir"
      exec >@stdout 2>@stderr bash -lc "./configure --prefix=$INSTALLDIR --exec-prefix=$INSTALLDIR --with-tcl=$tclDir"
      try {
        exec >@stdout 2>@stderr bash -lc "make test"
      } trap CHILDSTATUS {- opts} {
        lassign [dict get $opts -errorcode] -> pid code
        # when make encountered a build error, we expect to see an
        # error code of 2. Any other, non-make error code will be
        # ignored for the time being; assuming the test suite
        # completed.
        if {$code == 2} {exit 1}
        puts stderr "WARNING: make failed with unexpected error code: $opts"
      }

      exec >@stdout 2>@stderr bash -lc "make install"
    }
    nmake-tea {
      exec >@stdout 2>@stderr nmake -nologo -f makefile.vc TCLDIR=$tclRoot release
      
      cd [file join $BUILDDIR win]
      
      exec >@stdout 2>@stderr nmake -nologo -f makefile.vc TCLDIR=$tclRoot all
      exec >@stdout 2>@stderr nmake -nologo -f makefile.vc TCLDIR=$tclRoot test
      exec >@stdout 2>@stderr nmake -nologo -f makefile.vc TCLDIR=$tclRoot install INSTALLDIR=$INSTALLDIR
    }
    default {
      throw [list BUILD UNSUPPORTED $TOOLCHAIN] \
          "Unsupported toolchain: '$TOOLCHAIN'."
    }
  }
}

# puts ===$::argv
::build {*}$::argv


# Local variables:
#    mode: tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:










