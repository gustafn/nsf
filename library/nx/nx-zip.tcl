#
# Zip file generator - Create a Zip-file from a list of input file names
#
# This implementation is based on the zip file builder of Artur
# Trzewik (http://wiki.tcl.tk/15158), but was simplified, commented
# and extended, based on Trf and translated to NX 
#
# by Gustaf Neumann (June 2011)
#

package require nx
package require Trf
package provide nx::zip 1.0

namespace eval ::nx::zip {

  nx::Class create Archive {
    
    #
    # Add a file from the file system to the zip archive
    #
    :public method addFile {inputFileName outputFileName:optional} {
      # inputFileName - source file to archive
      # outputFileName - name of the file in the archive
      if {![file readable $inputFileName] || [file isdirectory $inputFileName]} {
	error "filename $inputFileName does not belong to a readable file"
      }
      if {![info exists outputFileName]} {set outputFileName $inputFileName}
      lappend :files file $inputFileName $outputFileName
    }

    #
    # Add a filecontent provided as string to the zip archive
    #
    :public method addString {string outputFileName} {
      # string - content to be added
      # outputFileName - name of the file in the archive
      lappend :files string $string $outputFileName
    }

    #
    # Write the added files to a zip file
    #
    :public method writeToZipFile {zipFileName} {
      set fout [open $zipFileName w]
      fconfigure $fout -encoding binary -translation binary
      :writeToStream $fout
      close $fout
    }

    #
    # return  the added files in aolserver/naviserver to the client
    #
    :public method ns_returnZipFile {zipFileName} {
      ns_write "HTTP/1.0 200 OK\r\nContent-type: application/zip\r\n"
      ns_write "Content-Disposition: attachment;filename=$zipFileName\r\n"
      ns_write "\r\n"
      set channel [ns_conn channel]
      fconfigure $channel -translation binary
      :writeToStream $channel
      # aolserver/naviserver closes the channel automatically
    }

    #
    # Write the added files to an already open stream
    #
    :public method writeToStream {outputStream} {
      set :outputStream $outputStream

      set descriptionList [list]
      foreach {type in fnOut} ${:files} {
	lappend descriptionList [:addToStream $type $in $fnOut]
      }

      set :cdOffset ${:written}
      foreach {type in fnOut} ${:files} desc $descriptionList {
	:writeCentralFileHeader $fnOut {*}$desc
      }
      set :cdLength [expr {${:written} - ${:cdOffset}}]

      # write header
      # EOCD 0X06054B50L scan 0X06054B50L %x s set s
      binary scan \x06\x05\x4B\x50 I EOCD
      :writeLong $EOCD
    
      # disk numbers
      :writeShort 0
      :writeShort 0
    
      # number of entries
      set filenum [expr {[llength ${:files}] / 3}]
      :writeShort $filenum
      :writeShort $filenum
    
      # length and location of CD
      :writeLong ${:cdLength}
      :writeLong ${:cdOffset}
    
      # zip file comment
      set comment ""

      # comment length
      :writeShort [string bytelength $comment]
      :writeString $comment
    }

    #
    # Constructor
    #
    :method init {} {
      set :files [list]
      set :cdLength 0
      set :cdOffset 0
      set :written 0
    }

    #
    # Output content file to the output stream
    #
    :method addToStream {type in fnOut} {
      set offset ${:written}
    
      if {$type eq "file"} {
	set fdata [open $in r]
	fconfigure $fdata -encoding binary -translation binary
	set data [read $fdata]
	close $fdata
	set mtime [file mtime $in]
      } else {
	set data $in
	set mtime [clock seconds]
      }

      binary scan \x04\x03\x4B\x50 I LFH_SIG
      :writeLong $LFH_SIG

      :writeShort 20
      # java implementation make 8
      # but tools (WinZip) leave it 0
      :writeShort 0
      :writeShort 8
    
      # last mod. time and date
      set dosTime [:toDosTime $mtime]
      :writeLong $dosTime

      set datacompressed [string range [::zip -mode compress $data] 2 end-4]    
      #set crc [::vfs::crc $data]
      set crc   [::crc-zlib $data]
      set csize [string length $datacompressed]
      set size  [string length $data]
      :writeString $crc
      :writeLong $csize
      :writeLong $size
    
      # file name length
      :writeShort [string length $fnOut]
    
      # extra field length
      set extra ""
      :writeShort [string length $extra]
    
      # file name
      :writeString $fnOut
      :writeString $extra
      :writeString $datacompressed
    
      return [list $offset $dosTime $crc $csize $size]
    }
  
    #
    # Convert the provided time stamp to dos time
    #
    :method toDosTime {time} {
      foreach {year month day hour minute second} \
	  [clock format $time -format "%Y %m %e %k %M %S"] {}

      set RE {^0([0-9]+)$}
      regexp $RE $year . year
      regexp $RE $month . month
      regexp $RE $day . day
      regexp $RE $hour . hour
      regexp $RE $minute . minute
      regexp $RE $second . second

      set value [expr {(($year - 1980) << 25) | ($month << 21) | 
		       ($day << 16) | ($hour << 11) | ($minute << 5) |
		       ($second >> 1)}]
      return $value
    }

    #
    # write header info about a content file
    #
    :method writeCentralFileHeader {fnOut offset dosTime crc size csize} {
    
      # CFH 0X02014B50L
      binary scan \x02\x01\x4B\x50 I CFH_SIG
      :writeLong $CFH_SIG
    
      if {$::tcl_platform(platform) eq "windows"} {
	# unix
	set pid 5
      } else {
	# windows
	set pid 11
      }
      :writeShort [expr { (($pid << 8) | 20) }]
      
      # version needed to extract
      # general purpose bit flag
      :writeShort 20
      :writeShort 0
    
      # compression method
      :writeShort 8
    
      # last mod. time and date
      :writeLong $dosTime
    
      # CRC
      # compressed length
      # uncompressed length
      :writeString $crc
      :writeLong $csize
      :writeLong $size
    
      set comment ""
      set extra ""
    
      # file name length
      :writeShort [string bytelength $fnOut]
      
      # extra field length
      :writeShort [string bytelength $extra]
    
      # file comment length
      :writeShort [string bytelength $comment]
    
      # disk number start
      :writeShort 0
    
      # internal file attributes
      :writeShort 0
    
      # external file attributes
      :writeLong 0
    
      # relative offset of LFH
      :writeLong $offset
    
      # file name
      :writeString $fnOut
    
      # extra field
      :writeString $extra
    
      # file comment
      :writeString $comment
    }

    #
    # Write the provided integer in binary form as a long value to the
    # output stream and increment byte counter
    #
    :method writeLong {long:integer} {
      puts -nonewline ${:outputStream} [binary format i $long]
      incr :written 4
    }

    #
    # Write the provided integer in binary form as a short value to
    # the output stream and increment byte counter
    #
    :method writeShort {short:integer} {
      puts -nonewline ${:outputStream} [binary format s $short]
      incr :written 2
    }

    #
    # Write the provided string to the output stream and increment
    # byte counter.
    #
    :method writeString {string} {
      puts -nonewline ${:outputStream} $string
      incr :written [string length $string]
    }
  }
}

if {0} {
  set z [::nx::zip::Archive new]
  $z addFile COMPILE 
  $z addFile COMPILE.win 
  $z addFile nsfUtil.o 
  $z addFile doc/nx.css
  $z addString "This is a file\nfrom a string\n"  README
  $z writeToZipFile /tmp/test.zip
  $z destroy
}
