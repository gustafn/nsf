namespace eval ::nsf {

  #
  # get frequenly used primitiva into the ::nsf namespace
  #
  namespace export next current my is relation interp

  #
  # error handler for info
  #
  proc ::nsf::infoError msg {
    #puts stderr "INFO ERROR: <$msg>\n$::errorInfo"
    regsub -all " <object>" $msg "" msg
    regsub -all " <class>" $msg "" msg
    regsub {\"} $msg "\"info " msg
    error $msg ""
  }
 
  #
  # exit handlers
  #
  proc ::nsf::unsetExitHandler {} {
    proc ::nsf::__exitHandler {} {
      # clients should append exit handlers to this proc body
    }
  }
  proc ::nsf::setExitHandler {newbody} {::proc ::nsf::__exitHandler {} $newbody}
  proc ::nsf::getExitHandler {} {::info body ::nsf::__exitHandler}
  # initialize exit handler
  ::nsf::unsetExitHandler

  #
  # determine platform aware temp directory
  #
  proc tmpdir {} {
    foreach e [list TMPDIR TEMP TMP] {
      if {[info exists ::env($e)] \
              && [file isdirectory $::env($e)] \
              && [file writable $::env($e)]} {
        return $::env($e)
      }
    }
    if {$::tcl_platform(platform) eq "windows"} {
      foreach d [list "C:\\TEMP" "C:\\TMP" "\\TEMP" "\\TMP"] {
        if {[file isdirectory $d] && [file writable $d]} {
          return $d
        }
      }
    }
    return /tmp
  }  

  namespace export tmpdir 

  # if HOME is not set, and ~ is resolved, Tcl chokes on that
  if {![info exists ::env(HOME)]} {set ::env(HOME) /root}

}
