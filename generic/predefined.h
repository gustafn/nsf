static char cmd[] = 
"namespace eval ::nsf {\n"
"namespace export next current self\n"
"namespace export alias configure finalize interp is my relation\n"
"proc ::nsf::provide_method {require_name definition {script \"\"}} {\n"
"set ::nsf::methodIndex($require_name) [list definition $definition script $script]}\n"
"proc ::nsf::require_method {object name {per_object 0}} {\n"
"set key ::nsf::methodIndex($name)\n"
"if {[info exists $key]} {\n"
"array set \"\" [set $key]\n"
"if {$(script) ne \"\"} {\n"
"eval $(script)}\n"
"if {$per_object} {\n"
"set cmd [linsert $(definition) 1 -per-object]\n"
"eval [linsert $cmd 1 $object]} else {\n"
"eval [linsert $(definition) 1 $object]}} else {\n"
"error \"cannot require method $name for $object, method unknown\"}}\n"
"set ::nsf::parametersyntax(::nsf::mixin) \"object ?-per-object? classes\"\n"
"proc ::nsf::mixin {object args} {\n"
"if {[lindex $args 0] eq \"-per-object\"} {\n"
"set rel \"object-mixin\"\n"
"set args [lrange $args 1 end]} else {\n"
"set rel \"class-mixin\"}\n"
"puts stderr LL=[llength $args]-$args\n"
"if {[lindex $args 0] ne \"\"} {\n"
"set oldSetting [::nsf::relation $object $rel]\n"
"uplevel [list ::nsf::relation $object $rel [linsert $oldSetting 0 $args]]} else {\n"
"uplevel [list ::nsf::relation $object $rel \"\"]}}\n"
"::nsf::provide_method autoname {::nsf::alias autoname ::nsf::methods::object::autoname}\n"
"::nsf::provide_method exists   {::nsf::alias  exists ::nsf::methods::object::exists}\n"
"proc ::nsf::exithandler {args} {\n"
"lassign $args op value\n"
"switch $op {\n"
"set {::proc ::nsf::__exithandler {} $value}\n"
"get {::info body ::nsf::__exithandler}\n"
"unset {proc ::nsf::__exithandler args {;}}\n"
"default {error \"syntax: ::nsf::exithandler $::nsf::parametersyntax(::nsf::exithandler)\"}}}\n"
"::nsf::exithandler unset\n"
"if {[info command ::ns_log] ne \"\"} {\n"
"proc ::nsf::log {level msg} {\n"
"ns_log $level \"nsf: $msg\"}} else {\n"
"proc ::nsf::log {level msg} {\n"
"puts stderr \"$level: $msg\"}}\n"
"proc tmpdir {} {\n"
"foreach e [list TMPDIR TEMP TMP] {\n"
"if {[info exists ::env($e)] \\\n"
"&& [file isdirectory $::env($e)] \\\n"
"&& [file writable $::env($e)]} {\n"
"return $::env($e)}}\n"
"if {$::tcl_platform(platform) eq \"windows\"} {\n"
"foreach d [list \"C:\\\\TEMP\" \"C:\\\\TMP\" \"\\\\TEMP\" \"\\\\TMP\"] {\n"
"if {[file isdirectory $d] && [file writable $d]} {\n"
"return $d}}}\n"
"return /tmp}\n"
"namespace export tmpdir\n"
"if {![info exists ::env(HOME)]} {set ::env(HOME) /root}\n"
"set ::nsf::parametersyntax(::nsf::xotclnext) \"?--noArgs? ?arg ...?\"\n"
"set ::nsf::parametersyntax(::nsf::__unset_unknown_args) \"\"\n"
"set ::nsf::parametersyntax(::nsf::exithandler) \"?get?|?set cmds?|?unset?\"}\n"
"";

