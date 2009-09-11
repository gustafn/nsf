static char cmd[] = 
"# $Id: predefined.xotcl,v 1.12 2006/10/04 20:40:23 neumann Exp $\n"
"namespace eval ::xotcl {\n"
"namespace eval ::oo {}\n"
"::xotcl::createobjectsystem ::oo::object ::oo::class\n"
"if {[info command ::oo::object] ne \"\"} {\n"
"::xotcl::alias ::oo::object destroy ::xotcl::cmd::Object::destroy\n"
"::xotcl::alias ::oo::class  dealloc ::xotcl::cmd::Class::dealloc\n"
"::xotcl::createobjectsystem ::xotcl::Object ::xotcl::Class}\n"
"set bootstrap 1\n"
"foreach cmd [info command ::xotcl::cmd::Object::*] {\n"
"::xotcl::alias ::xotcl::Object [namespace tail $cmd] $cmd}\n"
"foreach cmd {array append eval incr lappend    subst unset trace} {\n"
"::xotcl::alias ::xotcl::Object $cmd -objscope ::$cmd}\n"
"foreach cmd [info command ::xotcl::cmd::Class::*] {\n"
"::xotcl::alias ::xotcl::Class [namespace tail $cmd] $cmd}\n"
"::xotcl::Object instproc init args {}\n"
"::xotcl::Object instproc objectparameter {} {;}\n"
"::xotcl::Class create ::xotcl::ParameterType\n"
"foreach cmd [info command ::xotcl::cmd::ParameterType::*] {\n"
"::xotcl::alias ::xotcl::ParameterType [namespace tail $cmd] $cmd}\n"
"::xotcl::alias ::xotcl::ParameterType type=switch ::xotcl::cmd::ParameterType::type=boolean\n"
"::xotcl::ParameterType create ::xotcl::parameterType\n"
"::xotcl::Object create ::xotcl::objectInfo\n"
"::xotcl::Object create ::xotcl::classInfo\n"
"foreach cmd [info command ::xotcl::cmd::ObjectInfo::*] {\n"
"::xotcl::alias ::xotcl::objectInfo [namespace tail $cmd] $cmd\n"
"::xotcl::alias ::xotcl::classInfo [namespace tail $cmd] $cmd}\n"
"foreach cmd [info command ::xotcl::cmd::ClassInfo::*] {\n"
"::xotcl::alias ::xotcl::classInfo [namespace tail $cmd] $cmd}\n"
"unset cmd\n"
"::xotcl::alias ::xotcl::objectInfo is ::xotcl::is\n"
"::xotcl::alias ::xotcl::classInfo is ::xotcl::is\n"
"::xotcl::alias ::xotcl::classInfo classparent ::xotcl::cmd::ObjectInfo::parent\n"
"::xotcl::alias ::xotcl::classInfo classchildren ::xotcl::cmd::ObjectInfo::children\n"
"::xotcl::Object instforward info -onerror ::xotcl::infoError ::xotcl::objectInfo %1 {%@2 %self}\n"
"::xotcl::Class instforward info -onerror ::xotcl::infoError ::xotcl::classInfo %1 {%@2 %self}\n"
"proc ::xotcl::infoError msg {\n"
"regsub -all \" <object>\" $msg \"\" msg\n"
"regsub -all \" <class>\" $msg \"\" msg\n"
"regsub {\\\"} $msg \"\\\"info \" msg\n"
"error $msg \"\"}\n"
"::xotcl::objectInfo proc info {obj} {\n"
"set methods [list]\n"
"foreach m [::info commands ::xotcl::objectInfo::*] {\n"
"set name [namespace tail $m]\n"
"if {$name eq \"unknown\"} continue\n"
"lappend methods $name}\n"
"return \"valid options are: [join [lsort $methods] {, }]\"}\n"
"::xotcl::objectInfo proc unknown {method args} {\n"
"error \"unknown info option \\\"$method\\\"; [my info info]\"}\n"
"::xotcl::classInfo proc info {cl} {\n"
"set methods [list]\n"
"foreach m [::info commands ::xotcl::classInfo::*] {\n"
"set name [namespace tail $m]\n"
"if {$name eq \"unknown\"} continue\n"
"lappend methods $name}\n"
"return \"valid options are: [join [lsort $methods] {, }]\"}\n"
"::xotcl::classInfo proc unknown {method args} {\n"
"error \"unknown info option \\\"$method\\\"; [my info info]\"}\n"
"#     info instargs\n"
"#     istype??\n"
"proc ::xotcl::info_args {inst o method} {\n"
"set result [list]\n"
"foreach \\\n"
"argName [::xotcl::classInfo ${inst}params $o $method -varNames] \\\n"
"flag    [::xotcl::classInfo ${inst}params $o $method] {\n"
"if {[string match -* $flag]} continue\n"
"lappend result $argName}\n"
"return $result}\n"
"proc ::xotcl::info_nonposargs {inst o method} {\n"
"set result [list]\n"
"foreach flag [::xotcl::classInfo ${inst}params $o $method] {\n"
"if {![string match -* $flag]} continue\n"
"lappend result $flag}\n"
"return $result}\n"
"proc ::xotcl::info_default {inst o method arg varName} {\n"
"foreach \\\n"
"argName [::xotcl::classInfo ${inst}params $o $method -varNames] \\\n"
"flag    [::xotcl::classInfo ${inst}params $o $method] {\n"
"if {$argName eq $arg} {\n"
"upvar 3 $varName default\n"
"if {[llength $flag] == 2} {\n"
"set default [lindex $flag 1]\n"
"return 1}\n"
"set default \"\"\n"
"return 0}}\n"
"error \"procedure \\\"$method\\\" doesn't have an argument \\\"$varName\\\"\"}\n"
"::xotcl::classInfo  proc instargs {o method} {::xotcl::info_args inst $o $method}\n"
"::xotcl::classInfo  proc args     {o method} {::xotcl::info_args \"\" $o $method}\n"
"::xotcl::objectInfo proc args     {o method} {::xotcl::info_args \"\" $o $method}\n"
"::xotcl::classInfo  proc instnonposargs {o method} {::xotcl::info_nonposargs inst $o $method}\n"
"::xotcl::classInfo  proc nonposargs     {o method} {::xotcl::info_nonposargs \"\" $o $method}\n"
"::xotcl::objectInfo proc nonposargs     {o method} {::xotcl::info_nonposargs \"\" $o $method}\n"
"::xotcl::classInfo  proc instdefault {o method arg var} {::xotcl::info_default inst $o $method $arg $var}\n"
"::xotcl::classInfo  proc default     {o method arg var} {::xotcl::info_default \"\" $o $method $arg $var}\n"
"::xotcl::objectInfo proc default     {o method arg var} {::xotcl::info_default \"\" $o $method $arg $var}\n"
"::xotcl::Object create ::xotcl::@\n"
"::xotcl::@ proc unknown args {}\n"
"proc ::xotcl::myproc {args} {linsert $args 0 [::xotcl::self]}\n"
"proc ::xotcl::myvar  {var}  {::xotcl::my requireNamespace; return [::xotcl::self]::$var}\n"
"namespace export Object Class @ myproc myvar Attribute\n"
"::xotcl::Class create ::xotcl::MetaSlot\n"
"::xotcl::relation ::xotcl::MetaSlot superclass ::xotcl::Class\n"
"::xotcl::MetaSlot instproc new args {\n"
"set slotobject [::xotcl::self callingobject]::slot\n"
"if {![::xotcl::is $slotobject object]} {::xotcl::Object create $slotobject}\n"
"eval next -childof $slotobject $args}\n"
"::xotcl::MetaSlot create ::xotcl::Slot\n"
"::xotcl::MetaSlot invalidateobjectparameter\n"
"::xotcl::Object instproc objectparameter {} {\n"
"set parameterdefinitions [list]\n"
"set slots [::xotcl::objectInfo slotobjects [self]]\n"
"foreach slot $slots {\n"
"set parameterdefinition \"-[namespace tail $slot]\"\n"
"set opts [list]\n"
"if {[$slot exists required] && [$slot required]} {\n"
"lappend opts required}\n"
"if {[$slot exists type]} {\n"
"lappend opts [$slot type]}\n"
"if {[$slot exists default]} {\n"
"set arg [$slot set default]\n"
"if {[string match {*\\[*\\]*} $arg]} {\n"
"lappend opts substdefault}} elseif {[$slot exists initcmd]} {\n"
"set arg [$slot set initcmd]\n"
"lappend opts initcmd}\n"
"if {[llength $opts] > 0} {\n"
"append parameterdefinition :[join $opts ,]}\n"
"if {[info exists arg]} {\n"
"lappend parameterdefinition $arg\n"
"unset arg}\n"
"lappend parameterdefinitions $parameterdefinition}\n"
"lappend parameterdefinitions args\n"
"return $parameterdefinitions}\n"
"proc createBootstrapAttributeSlots {class definitions} {\n"
"if {![::xotcl::is ${class}::slot object]} {\n"
"::xotcl::Object create ${class}::slot}\n"
"foreach att $definitions {\n"
"if {[llength $att]>1} {foreach {att default} $att break}\n"
"::xotcl::Slot create ${class}::slot::$att\n"
"if {[info exists default]} {\n"
"::xotcl::setinstvar ${class}::slot::$att default $default\n"
"unset default}\n"
"$class instparametercmd $att}\n"
"foreach att $definitions {\n"
"if {[llength $att]>1} {foreach {att default} $att break}\n"
"if {[info exists default]} {\n"
"foreach i [$class info instances] {\n"
"if {![$i exists $att]} {\n"
"if {[string match {*[*]*} $default]} {set default [$i eval subst $default]}\n"
"::xotcl::setinstvar $i $att $default}}\n"
"unset default}}\n"
"$class invalidateobjectparameter}\n"
"createBootstrapAttributeSlots ::xotcl::Class {\n"
"{__default_superclass ::xotcl::Object}\n"
"{__default_metaclass ::xotcl::Class}}\n"
"createBootstrapAttributeSlots ::xotcl::Slot {\n"
"{name \"[namespace tail [::xotcl::self]]\"}\n"
"{domain \"[lindex [regexp -inline {^(.*)::slot::[^:]+$} [::xotcl::self]] 1]\"}\n"
"{defaultmethods {get assign}}\n"
"{manager \"[::xotcl::self]\"}\n"
"{multivalued false}\n"
"{per-object false}\n"
"{required false}\n"
"default\n"
"type}\n"
"::xotcl::alias ::xotcl::Slot get ::xotcl::setinstvar\n"
"::xotcl::alias ::xotcl::Slot assign ::xotcl::setinstvar\n"
"::xotcl::Slot instproc add {obj prop value {pos 0}} {\n"
"if {![::xotcl::my multivalued]} {\n"
"error \"Property $prop of [::xotcl::my domain]->$obj ist not multivalued\"}\n"
"if {[$obj exists $prop]} {\n"
"$obj set $prop [linsert [$obj set $prop] $pos $value]} else {\n"
"$obj set $prop [list $value]}}\n"
"::xotcl::Slot instproc delete {-nocomplain:switch obj prop value} {\n"
"set old [$obj set $prop]\n"
"set p [lsearch -glob $old $value]\n"
"if {$p>-1} {$obj set $prop [lreplace $old $p $p]} else {\n"
"error \"$value is not a $prop of $obj (valid are: $old)\"}}\n"
"::xotcl::Slot instproc unknown {method args} {\n"
"set methods [list]\n"
"foreach m [::xotcl::my info methods] {\n"
"if {[::xotcl::Object info methods $m] ne \"\"} continue\n"
"if {[string match __* $m]} continue\n"
"lappend methods $m}\n"
"error \"Method '$method' unknown for slot [::xotcl::self]; valid are: {[lsort $methods]]}\"}\n"
"::xotcl::Slot instproc destroy {} {\n"
"::xotcl::instvar domain\n"
"if {$domain ne \"\"} {\n"
"$domain invalidateobjectparameter}\n"
"next}\n"
"::xotcl::Slot instproc init {} {\n"
"::xotcl::instvar name domain manager per-object\n"
"set forwarder [expr {${per-object} ? \"forward\" : \"instforward\"}]\n"
"if {$domain eq \"\"} {\n"
"set domain [::xotcl::self callingobject]} else {\n"
"$domain invalidateobjectparameter}\n"
"$domain $forwarder $name -default [$manager defaultmethods] $manager %1 %self %proc}\n"
"::xotcl::MetaSlot create ::xotcl::InfoSlot\n"
"createBootstrapAttributeSlots ::xotcl::InfoSlot {\n"
"{multivalued true}\n"
"{elementtype ::xotcl::Class}}\n"
"::xotcl::relation ::xotcl::InfoSlot superclass ::xotcl::Slot\n"
"::xotcl::InfoSlot instproc get {obj prop} {$obj info $prop}\n"
"::xotcl::InfoSlot instproc add {obj prop value {pos 0}} {\n"
"if {![::xotcl::my multivalued]} {\n"
"error \"Property $prop of [::xotcl::my domain]->$obj ist not multivalued\"}\n"
"$obj $prop [linsert [$obj info $prop] $pos $value]}\n"
"::xotcl::InfoSlot instproc delete {-nocomplain:switch obj prop value} {\n"
"set old [$obj info $prop]\n"
"if {[string first * $value] > -1 || [string first \\[ $value] > -1} {\n"
"if {[my elementtype] ne \"\" && ![string match ::* $value]} {\n"
"set value ::$value}\n"
"return [$obj $prop [lsearch -all -not -glob -inline $old $value]]} elseif {[my elementtype] ne \"\"} {\n"
"if {[string first :: $value] == -1} {\n"
"if {![my isobject $value]} {\n"
"error \"$value does not appear to be an object\"}\n"
"set value [$value self]}\n"
"if {![$value isclass [my elementtype]]} {\n"
"error \"$value does not appear to be of type [my elementtype]\"}}\n"
"set p [lsearch -exact $old $value]\n"
"if {$p > -1} {\n"
"$obj $prop [lreplace $old $p $p]} else {\n"
"error \"$value is not a $prop of $obj (valid are: $old)\"}}\n"
"::xotcl::MetaSlot alloc ::xotcl::InterceptorSlot\n"
"::xotcl::relation ::xotcl::InterceptorSlot superclass ::xotcl::InfoSlot\n"
"::xotcl::alias ::xotcl::InterceptorSlot set ::xotcl::relation ;# for backwards compatibility\n"
"::xotcl::alias ::xotcl::InterceptorSlot assign ::xotcl::relation\n"
"::xotcl::InterceptorSlot instproc add {obj prop value {pos 0}} {\n"
"if {![::xotcl::my multivalued]} {\n"
"error \"Property $prop of [::xotcl::my domain]->$obj ist not multivalued\"}\n"
"$obj $prop [linsert [$obj info $prop -guards] $pos $value]}\n"
"namespace eval ::xotcl::Object::slot {}\n"
"::xotcl::Object alloc ::xotcl::Class::slot\n"
"::xotcl::Object alloc ::xotcl::Object::slot\n"
"::xotcl::InfoSlot create ::xotcl::Class::slot::superclass -type relation\n"
"::xotcl::alias ::xotcl::Class::slot::superclass assign ::xotcl::relation\n"
"::xotcl::InfoSlot create ::xotcl::Object::slot::class -type relation\n"
"::xotcl::alias ::xotcl::Object::slot::class assign ::xotcl::relation\n"
"::xotcl::InterceptorSlot create ::xotcl::Object::slot::mixin \\\n"
"-type relation\n"
"::xotcl::InterceptorSlot create ::xotcl::Object::slot::filter \\\n"
"-elementtype \"\" -type relation\n"
"::xotcl::InterceptorSlot create ::xotcl::Class::slot::instmixin \\\n"
"-type relation\n"
"::xotcl::InterceptorSlot create ::xotcl::Class::slot::instfilter \\\n"
"-elementtype \"\" \\\n"
"-type relation\n"
"::xotcl::MetaSlot create ::xotcl::Attribute -superclass ::xotcl::Slot\n"
"createBootstrapAttributeSlots ::xotcl::Attribute {\n"
"{value_check once}\n"
"initcmd\n"
"valuecmd\n"
"valuechangedcmd}\n"
"::xotcl::Attribute instproc __default_from_cmd {obj cmd var sub op} {\n"
"$obj trace remove variable $var $op [list [::xotcl::self] [::xotcl::self proc] $obj $cmd]\n"
"$obj set $var [$obj eval $cmd]}\n"
"::xotcl::Attribute instproc __value_from_cmd {obj cmd var sub op} {\n"
"$obj set $var [$obj eval $cmd]}\n"
"::xotcl::Attribute instproc __value_changed_cmd {obj cmd var sub op} {\n"
"eval $cmd}\n"
"::xotcl::Attribute instproc check_single_value {\n"
"{-keep_old_value:boolean true}\n"
"value predicate type obj var} {\n"
"if {![expr $predicate]} {\n"
"if {[$obj exists __oldvalue($var)]} {\n"
"$obj set $var [$obj set __oldvalue($var)]} else {\n"
"$obj unset -nocomplain $var}\n"
"error \"'$value' is not of type $type\"}\n"
"if {$keep_old_value} {$obj set __oldvalue($var) $value}}\n"
"::xotcl::Attribute instproc check_multiple_values {values predicate type obj var} {\n"
"foreach value $values {\n"
"::xotcl::my check_single_value -keep_old_value false $value $predicate $type $obj $var}\n"
"$obj set __oldvalue($var) $value}\n"
"::xotcl::Attribute instproc mk_type_checker {} {\n"
"set __initcmd \"\"\n"
"if {[::xotcl::my exists type]} {\n"
"::xotcl::my instvar type name\n"
"if {[::xotcl::Object isclass $type]} {\n"
"set predicate [subst -nocommands {\n"
"[::xotcl::Object isobject \\$value] && [\\$value istype $type]}]} elseif {[llength $type]>1} {\n"
"set predicate \"\\[$type \\$value\\]\"} else {\n"
"set predicate \"\\[[self] type=$type $name \\$value\\]\"}\n"
"::xotcl::my append valuechangedcmd [subst {\n"
"::xotcl::my [expr {[::xotcl::my multivalued] ?\n"
"\"check_multiple_values\" : \"check_single_value\"}] \\[\\$obj set $name\\] \\\n"
"{$predicate} [list $type] \\$obj $name}]\n"
"append __initcmd [subst -nocommands {\n"
"if {[::xotcl::my exists $name]} {::xotcl::my set __oldvalue($name) [::xotcl::my set $name]}\\n}]}\n"
"return $__initcmd}\n"
"::xotcl::Attribute instproc init {} {\n"
"::xotcl::my instvar domain name\n"
"next ;# do first ordinary slot initialization\n"
"set __initcmd \"\"\n"
"if {[::xotcl::my exists default]} {} elseif [::xotcl::my exists initcmd] {\n"
"append __initcmd \"::xotcl::my trace add variable [list $name] read \\\n"
"\\[list [::xotcl::self] __default_from_cmd \\[::xotcl::self\\] [list [::xotcl::my initcmd]]\\]\\n\"} elseif [::xotcl::my exists valuecmd] {\n"
"append __initcmd \"::xotcl::my trace add variable [list $name] read \\\n"
"\\[list [::xotcl::self] __value_from_cmd \\[::xotcl::self\\] [list [::xotcl::my valuecmd]]\\]\"}\n"
"if {[::xotcl::my exists valuechangedcmd]} {\n"
"append __initcmd \"::xotcl::my trace add variable [list $name] write \\\n"
"\\[list [::xotcl::self] __value_changed_cmd \\[::xotcl::self\\] [list [::xotcl::my valuechangedcmd]]\\]\"}\n"
"if {$__initcmd ne \"\"} {\n"
"my set initcmd $__initcmd}}\n"
"::xotcl::Class create ::xotcl::Slot::Nocheck \\\n"
"-instproc check_single_value args {;} -instproc check_multiple_values args {;} \\\n"
"-instproc mk_type_checker args {return \"\"}\n"
"::xotcl::Class create ::xotcl::Slot::Optimizer \\\n"
"-instproc proc args    {::xotcl::next; ::xotcl::my optimize} \\\n"
"-instproc forward args {::xotcl::next; ::xotcl::my optimize} \\\n"
"-instproc init args    {::xotcl::next; ::xotcl::my optimize} \\\n"
"-instproc optimize {} {\n"
"if {[::xotcl::my multivalued]} return\n"
"if {[::xotcl::my defaultmethods] ne {get assign}} return\n"
"if {[::xotcl::my procsearch assign] ne \"::xotcl::Slot instcmd assign\"} return\n"
"if {[::xotcl::my procsearch get]    ne \"::xotcl::Slot instcmd get\"} return\n"
"set forwarder [expr {[::xotcl::my per-object] ? \"parametercmd\":\"instparametercmd\"}]\n"
"[::xotcl::my domain] $forwarder [::xotcl::my name]}\n"
"::xotcl::Attribute instmixin add ::xotcl::Slot::Optimizer\n"
"::xotcl::Class create ::xotcl::ScopedNew -superclass ::xotcl::Class\n"
"createBootstrapAttributeSlots ::xotcl::ScopedNew {\n"
"{withclass ::xotcl::Object}\n"
"inobject}\n"
"::xotcl::ScopedNew instproc init {} {\n"
"::xotcl::my instproc new {-childof args} {\n"
"[::xotcl::self class] instvar {inobject object} withclass\n"
"if {![::xotcl::my isobject $object]} {\n"
"$withclass create $object}\n"
"eval ::xotcl::next -childof $object $args}}\n"
"::xotcl::Object instproc contains {\n"
"{-withnew:boolean true}\n"
"-object\n"
"{-class ::xotcl::Object}\n"
"cmds} {\n"
"if {![info exists object]} {set object [::xotcl::self]}\n"
"if {![::xotcl::my isobject $object]} {$class create $object}\n"
"$object requireNamespace\n"
"if {$withnew} {\n"
"set m [::xotcl::ScopedNew new \\\n"
"-inobject $object -withclass $class -volatile]\n"
"::xotcl::Class instmixin add $m end\n"
"namespace eval $object $cmds\n"
"::xotcl::Class instmixin delete $m} else {\n"
"namespace eval $object $cmds}}\n"
"::xotcl::Class instforward slots %self contains \\\n"
"-object {%::xotcl::my subst [::xotcl::self]::slot}\n"
"::xotcl::Class instproc parameter arglist {\n"
"if {![::xotcl::is [::xotcl::self]::slot object]} {\n"
"::xotcl::Object create [::xotcl::self]::slot}\n"
"foreach arg $arglist {\n"
"set l [llength $arg]\n"
"set name [lindex $arg 0]\n"
"if {[string first : $name] > -1} {\n"
"foreach {name type} [split $name :] break\n"
"if {$type eq \"required\"} {\n"
"set required 1\n"
"unset type}}\n"
"set cmd [list ::xotcl::Attribute create [::xotcl::self]::slot::$name]\n"
"if {[info exists type]} {\n"
"lappend cmd -type $type\n"
"unset type}\n"
"if {[info exists required]} {\n"
"lappend cmd -required 1\n"
"unset required}\n"
"if {$l == 1} {\n"
"eval $cmd} elseif {$l == 2} {\n"
"lappend cmd [list -default [lindex $arg 1]]\n"
"eval $cmd} elseif {$l == 3 && [lindex $arg 1] eq \"-default\"} {\n"
"lappend cmd [list -default [lindex $arg 2]]\n"
"eval $cmd} else {\n"
"set paramstring [string range $arg [expr {[string length $name]+1}] end]\n"
"if {[string match {[$\\[]*} $paramstring]} {\n"
"lappend cmd [list -default $paramstring]\n"
"eval $cmd\n"
"continue}\n"
"set po ::xotcl::Class::Parameter\n"
"puts stderr \"deprecated parameter usage '$arg'; use '-slots {Attribute ...}' instead\"\n"
"set cl [::xotcl::self]\n"
"$po set name $name\n"
"$po set cl [::xotcl::self]\n"
"::eval $po configure [lrange $arg 1 end]\n"
"if {[$po exists extra] || [$po exists setter] ||\n"
"[$po exists getter] || [$po exists access]} {\n"
"$po instvar extra setter getter access defaultParam\n"
"if {![info exists extra]} {set extra \"\"}\n"
"if {![info exists defaultParam]} {set defaultParam \"\"}\n"
"if {![info exists setter]} {set setter set}\n"
"if {![info exists getter]} {set getter set}\n"
"if {![info exists access]} {set access ::xotcl::my}\n"
"$cl instproc $name args \"\n"
"if {\\[llength \\$args] == 0} {\n"
"return \\[$access $getter $extra $name\\]} else {\n"
"return \\[eval $access $setter $extra $name \\$args $defaultParam \\]}\"\n"
"foreach instvar {extra defaultParam setter getter access} {\n"
"$po unset -nocomplain $instvar}} else {\n"
"::xotcl::my instparametercmd $name}}}\n"
"[::xotcl::self]::slot set __parameter $arglist}\n"
"::xotcl::Object instproc self {} {::xotcl::self}\n"
"::xotcl::Object instproc defaultmethod {} {\n"
"return [::xotcl::self]}\n"
"::xotcl::Object instproc hasclass cl {\n"
"if {[::xotcl::my ismixin $cl]} {return 1}\n"
"::xotcl::my istype $cl}\n"
"::xotcl::Class instproc allinstances {} {\n"
"return [::xotcl::my info instances -closure]}\n"
"::xotcl::Object proc unsetExitHandler {} {\n"
"::xotcl::Object proc __exitHandler {} {\n"
";}}\n"
"::xotcl::Object unsetExitHandler\n"
"::xotcl::Object proc setExitHandler {newbody} {\n"
"::xotcl::Object proc __exitHandler {} $newbody}\n"
"::xotcl::Object proc getExitHandler {} {\n"
"::xotcl::Object info body __exitHandler}\n"
"proc ::xotcl::__exitHandler {} {\n"
"::xotcl::Object __exitHandler}\n"
"::xotcl::Object instproc abstract {methtype methname arglist} {\n"
"if {$methtype ne \"proc\" && $methtype ne \"instproc\" && $methtype ne \"method\"} {\n"
"error \"invalid method type '$methtype', \\\n"
"must be either 'proc', 'instproc' or 'method'.\"}\n"
"::xotcl::my $methtype $methname $arglist \"\n"
"if {!\\[::xotcl::self isnextcall\\]} {\n"
"error \\\"Abstract method $methname $arglist called\\\"} else {::xotcl::next}\n"
"\"}\n"
"::xotcl::Class create ::xotcl::Object::CopyHandler -parameter {\n"
"{targetList \"\"}\n"
"{dest \"\"}\n"
"objLength}\n"
"::xotcl::Object::CopyHandler instproc makeTargetList t {\n"
"::xotcl::my lappend targetList $t\n"
"if {[::xotcl::my isobject $t]} {\n"
"if {[$t info hasnamespace]} {\n"
"set children [$t info children]} else {\n"
"return}}\n"
"foreach c [namespace children $t] {\n"
"if {![::xotcl::my isobject $c]} {\n"
"lappend children [namespace children $t]}}\n"
"foreach c $children {\n"
"::xotcl::my makeTargetList $c}}\n"
"::xotcl::Object::CopyHandler instproc copyNSVarsAndCmds {orig dest} {\n"
"::xotcl::namespace_copyvars $orig $dest\n"
"::xotcl::namespace_copycmds $orig $dest}\n"
"::xotcl::Object::CopyHandler instproc getDest origin {\n"
"set tail [string range $origin [::xotcl::my set objLength] end]\n"
"return ::[string trimleft [::xotcl::my set dest]$tail :]}\n"
"::xotcl::Object::CopyHandler instproc copyTargets {} {\n"
"foreach origin [::xotcl::my set targetList] {\n"
"set dest [::xotcl::my getDest $origin]\n"
"if {[::xotcl::my isobject $origin]} {\n"
"if {[::xotcl::my isclass $origin]} {\n"
"set cl [[$origin info class] create $dest -noinit]\n"
"set obj $cl\n"
"$cl superclass [$origin info superclass]\n"
"$cl instinvar [$origin info instinvar]\n"
"$cl instfilter [$origin info instfilter -guards]\n"
"$cl instmixin [$origin info instmixin]\n"
"my copyNSVarsAndCmds ::xotcl::classes$origin ::xotcl::classes$dest} else {\n"
"set obj [[$origin info class] create $dest -noinit]}\n"
"$obj invar [$origin info invar]\n"
"$obj check [$origin info check]\n"
"$obj mixin [$origin info mixin]\n"
"$obj filter [$origin info filter -guards]\n"
"if {[$origin info hasnamespace]} {\n"
"$obj requireNamespace}} else {\n"
"namespace eval $dest {}}\n"
"::xotcl::my copyNSVarsAndCmds $origin $dest\n"
"foreach i [$origin info forward] {\n"
"eval [concat $dest forward $i [$origin info forward -definition $i]]}\n"
"if {[::xotcl::my isclass $origin]} {\n"
"foreach i [$origin info instforward] {\n"
"eval [concat $dest instforward $i [$origin info instforward -definition $i]]}}\n"
"set traces [list]\n"
"foreach var [$origin info vars] {\n"
"set cmds [$origin trace info variable $var]\n"
"if {$cmds ne \"\"} {\n"
"foreach cmd $cmds {\n"
"foreach {op def} $cmd break\n"
"if {[lindex $def 0] eq $origin} {\n"
"set def [concat $dest [lrange $def 1 end]]}\n"
"$dest trace add variable $var $op $def}}}}\n"
"foreach origin [::xotcl::my set targetList] {\n"
"if {[::xotcl::my isclass $origin]} {\n"
"set dest [::xotcl::my getDest $origin]\n"
"foreach oldslot [$origin info slots] {\n"
"set newslot ${dest}::slot::[namespace tail $oldslot]\n"
"if {[$oldslot domain] eq $origin}   {$newslot domain $cl}\n"
"if {[$oldslot manager] eq $oldslot} {$newslot manager $newslot}}}}}\n"
"::xotcl::Object::CopyHandler instproc copy {obj dest} {\n"
"::xotcl::my set objLength [string length $obj]\n"
"::xotcl::my set dest $dest\n"
"::xotcl::my makeTargetList $obj\n"
"::xotcl::my copyTargets}\n"
"::xotcl::Object instproc copy newName {\n"
"if {[string compare [string trimleft $newName :] [string trimleft [::xotcl::self] :]]} {\n"
"[[::xotcl::self class]::CopyHandler new -volatile] copy [::xotcl::self] $newName}}\n"
"::xotcl::Object instproc move newName {\n"
"if {[string trimleft $newName :] ne [string trimleft [::xotcl::self] :]} {\n"
"if {$newName ne \"\"} {\n"
"::xotcl::my copy $newName}\n"
"if {[::xotcl::my isclass [::xotcl::self]] && $newName ne \"\"} {\n"
"foreach subclass [::xotcl::my info subclass] {\n"
"set scl [$subclass info superclass]\n"
"if {[set index [lsearch -exact $scl [::xotcl::self]]] != -1} {\n"
"set scl [lreplace $scl $index $index $newName]\n"
"$subclass superclass $scl}}	}\n"
"::xotcl::my destroy}}\n"
"::xotcl::Object create ::xotcl::config\n"
"::xotcl::config proc load {obj file} {\n"
"source $file\n"
"foreach i [array names ::auto_index [list $obj *proc *]] {\n"
"set type [lindex $i 1]\n"
"set meth [lindex $i 2]\n"
"if {[$obj info ${type}s $meth] == {}} {\n"
"$obj $type $meth auto $::auto_index($i)}}}\n"
"::xotcl::config proc mkindex {meta dir args} {\n"
"set sp {[ 	]+}\n"
"set st {^[ 	]*}\n"
"set wd {([^ 	;]+)}\n"
"foreach creator $meta {\n"
"::lappend cp $st$creator${sp}create$sp$wd\n"
"::lappend ap $st$creator$sp$wd}\n"
"foreach method {proc instproc} {\n"
"::lappend mp $st$wd${sp}($method)$sp$wd}\n"
"foreach cl [concat ::xotcl::Class [::xotcl::Class info heritage]] {\n"
"eval ::lappend meths [$cl info instcommands]}\n"
"set old [pwd]\n"
"cd $dir\n"
"::append idx \"# Tcl autoload index file, version 2.0\\n\"\n"
"::append idx \"# xotcl additions generated with \"\n"
"::append idx \"\\\"::xotcl::config::mkindex [list $meta] [list $dir] $args\\\"\\n\"\n"
"set oc 0\n"
"set mc 0\n"
"foreach file [eval glob -nocomplain -- $args] {\n"
"if {[catch {set f [open $file]} msg]} then {\n"
"catch {close $f}\n"
"cd $old\n"
"error $msg}\n"
"while {[gets $f line] >= 0} {\n"
"foreach c $cp {\n"
"if {[regexp $c $line x obj]==1 &&\n"
"[string index $obj 0]!={$}} then {\n"
"::incr oc\n"
"::append idx \"set auto_index($obj) \"\n"
"::append idx \"\\\"::xotcl::config::load $obj \\$dir/$file\\\"\\n\"}}\n"
"foreach a $ap {\n"
"if {[regexp $a $line x obj]==1 &&\n"
"[string index $obj 0]!={$} &&\n"
"[lsearch -exact $meths $obj]==-1} {\n"
"::incr oc\n"
"::append idx \"set auto_index($obj) \"\n"
"::append idx \"\\\"::xotcl::config::load $obj \\$dir/$file\\\"\\n\"}}\n"
"foreach m $mp {\n"
"if {[regexp $m $line x obj ty pr]==1 &&\n"
"[string index $obj 0]!={$} &&\n"
"[string index $pr 0]!={$}} then {\n"
"::incr mc\n"
"::append idx \"set \\{auto_index($obj \"\n"
"::append idx \"$ty $pr)\\} \\\"source \\$dir/$file\\\"\\n\"}}}\n"
"close $f}\n"
"set t [open tclIndex a+]\n"
"puts $t $idx nonewline\n"
"close $t\n"
"cd $old\n"
"return \"$oc objects, $mc methods\"}\n"
"::xotcl::Object instproc extractConfigureArg {al name {cutTheArg 0}} {\n"
"set value \"\"\n"
"upvar $al argList\n"
"set largs [llength $argList]\n"
"for {set i 0} {$i < $largs} {incr i} {\n"
"if {[lindex $argList $i] == $name && $i + 1 < $largs} {\n"
"set startIndex $i\n"
"set endIndex [expr {$i + 1}]\n"
"while {$endIndex < $largs &&\n"
"[string first - [lindex $argList $endIndex]] != 0} {\n"
"lappend value [lindex $argList $endIndex]\n"
"incr endIndex}}}\n"
"if {[info exists startIndex] && $cutTheArg != 0} {\n"
"set argList [lreplace $argList $startIndex [expr {$endIndex - 1}]]}\n"
"return $value}\n"
"::xotcl::Object create ::xotcl::rcs\n"
"::xotcl::rcs proc date string {\n"
"lreplace [lreplace $string 0 0] end end}\n"
"::xotcl::rcs proc version string {\n"
"lindex $string 2}\n"
"if {![info exists ::env(HOME)]} {set ::env(HOME) /root}\n"
"set ::xotcl::confdir ~/.xotcl\n"
"set ::xotcl::logdir $::xotcl::confdir/log\n"
"::xotcl::Class proc __unknown name {}\n"
"::xotcl::Class instproc uses list {\n"
"foreach package $list {\n"
"::xotcl::package import -into [::xotcl::self] $package\n"
"puts stderr \"*** using ${package}::* in [::xotcl::self]\"}}\n"
"::xotcl::Class create ::xotcl::package -superclass ::xotcl::Class -parameter {\n"
"provide\n"
"{version 1.0}\n"
"{autoexport {}}\n"
"{export {}}}\n"
"::xotcl::package proc create {name args} {\n"
"set nq [namespace qualifiers $name]\n"
"if {$nq ne \"\" && ![namespace exists $nq]} {Object create $nq}\n"
"next}\n"
"::xotcl::package proc extend {name args} {\n"
"my require $name\n"
"eval $name configure $args}\n"
"::xotcl::package instproc contains script {\n"
"if {[my exists provide]} {\n"
"package provide [my provide] [my version]} else {\n"
"package provide [::xotcl::self] [::xotcl::my version]}\n"
"namespace eval [::xotcl::self] {namespace import ::xotcl::*}\n"
"namespace eval [::xotcl::self] $script\n"
"foreach e [my export] {\n"
"set nq [namespace qualifiers $e]\n"
"if {$nq ne \"\"} {\n"
"namespace eval [::xotcl::self]::$nq [list namespace export [namespace tail $e]]} else {\n"
"namespace eval [::xotcl::self] [list namespace export $e]}}\n"
"foreach e [my autoexport] {\n"
"namespace eval :: [list namespace import [::xotcl::self]::$e]}}\n"
"::xotcl::package configure \\\n"
"-set component . \\\n"
"-set verbose 0 \\\n"
"-set packagecmd ::package\n"
"::xotcl::package proc unknown args {\n"
"eval [my set packagecmd] $args}\n"
"::xotcl::package proc verbose value {\n"
"my set verbose $value}\n"
"::xotcl::package proc present args {\n"
"if {$::tcl_version<8.3} {\n"
"my instvar loaded\n"
"switch -exact -- [lindex $args 0] {\n"
"-exact  {set pkg [lindex $args 1]}\n"
"default {set pkg [lindex $args 0]}}\n"
"if {[info exists loaded($pkg)]} {\n"
"return $loaded($pkg)} else {\n"
"error \"not found\"}} else {\n"
"eval [my set packagecmd] present $args}}\n"
"::xotcl::package proc import {{-into ::} pkg} {\n"
"my require $pkg\n"
"namespace eval $into [subst -nocommands {\n"
"namespace import ${pkg}::*}]\n"
"foreach e [$pkg export] {\n"
"set nq [namespace qualifiers $e]\n"
"if {$nq ne \"\"} {\n"
"namespace eval $into$nq [list namespace import ${pkg}::$e]}}}\n"
"::xotcl::package proc require args {\n"
"::xotcl::my instvar component verbose uses loaded\n"
"set prevComponent $component\n"
"if {[catch {set v [eval package present $args]} msg]} {\n"
"switch -exact -- [lindex $args 0] {\n"
"-exact  {set pkg [lindex $args 1]}\n"
"default {set pkg [lindex $args 0]}}\n"
"set component $pkg\n"
"lappend uses($prevComponent) $component\n"
"set v [uplevel \\#1 [my set packagecmd] require $args]\n"
"if {$v ne \"\" && $verbose} {\n"
"set path [lindex [::package ifneeded $pkg $v] 1]\n"
"puts \"... $pkg $v loaded from '$path'\"\n"
"set loaded($pkg) $v   ;# loaded stuff needed for Tcl 8.0}}\n"
"set component $prevComponent\n"
"return $v}\n"
"::xotcl::Object instproc method {name arguments body} {\n"
"my proc name $arguments $body				}\n"
"::xotcl::Class instproc method {-per-object:switch name arguments body} {\n"
"if {${per-object}} {\n"
"my proc $name $arguments $body} else {\n"
"my instproc $name $arguments $body}}\n"
"proc ::xotcl::tmpdir {} {\n"
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
"unset bootstrap}";

