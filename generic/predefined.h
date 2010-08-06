static char cmd[] = 
"namespace eval ::nx {\n"
"set bootstrap 1\n"
"::nx::core::createobjectsystem ::nx::Object ::nx::Class {\n"
"-class.alloc alloc\n"
"-class.create create\n"
"-class.dealloc dealloc\n"
"-class.recreate recreate\n"
"-class.requireobject __unknown\n"
"-object.configure configure\n"
"-object.defaultmethod defaultmethod\n"
"-object.destroy destroy\n"
"-object.init init\n"
"-object.move move\n"
"-object.objectparameter objectparameter\n"
"-object.residualargs residualargs\n"
"-object.unknown unknown}\n"
"namespace eval ::nx::core {\n"
"namespace export next self \\\n"
"my is relation interp}\n"
"namespace import ::nx::core::next ::nx::core::self\n"
"foreach cmd [info command ::nx::core::cmd::Object::*] {\n"
"set cmdName [namespace tail $cmd]\n"
"if {$cmdName in [list \"exists\" \"instvar\"]} continue\n"
"::nx::core::alias Object $cmdName $cmd}\n"
"::nx::core::alias Object eval -nonleaf ::eval\n"
"foreach cmd [info command ::nx::core::cmd::Class::*] {\n"
"set cmdName [namespace tail $cmd]\n"
"::nx::core::alias Class $cmdName $cmd}\n"
"foreach cmd [list __next cleanup  noinit residualargs uplevel upvar] {\n"
"::nx::core::methodproperty Object $cmd protected 1}\n"
"foreach cmd [list recreate] {\n"
"::nx::core::methodproperty Class $cmd protected 1}\n"
"::nx::core::methodproperty Object destroy redefine-protected true\n"
"::nx::core::methodproperty Class  alloc   redefine-protected true\n"
"::nx::core::methodproperty Class  dealloc redefine-protected true\n"
"::nx::core::methodproperty Class  create  redefine-protected true\n"
"::nx::core::method Class method {\n"
"name arguments body -precondition -postcondition} {\n"
"set conditions [list]\n"
"if {[info exists precondition]}  {lappend conditions -precondition  $precondition}\n"
"if {[info exists postcondition]} {lappend conditions -postcondition $postcondition}\n"
"::nx::core::method [::nx::core::current object] $name $arguments $body {*}$conditions}\n"
"::nx::core::method Object method {\n"
"name arguments body -precondition -postcondition} {\n"
"set conditions [list]\n"
"if {[info exists precondition]}  {lappend conditions -precondition  $precondition}\n"
"if {[info exists postcondition]} {lappend conditions -postcondition $postcondition}\n"
"::nx::core::method [::nx::core::current object] -per-object $name $arguments $body {*}$conditions}\n"
"Class eval {\n"
":method object {what args} {\n"
"if {$what in [list \"alias\" \"attribute\" \"forward\" \"method\" \"setter\"]} {\n"
"return [::nx::core::dispatch [::nx::core::current object] ::nx::core::classes::nx::Object::$what {*}$args]}\n"
"if {$what in [list \"info\"]} {\n"
"return [::nx::objectInfo [lindex $args 0] [::nx::core::current object] {*}[lrange $args 1 end]]}\n"
"if {$what in [list \"filter\" \"mixin\"]} {\n"
"return [:object-$what {*}$args]}\n"
"if {$what in [list \"filterguard\" \"mixinguard\"]} {\n"
"return [::nx::core::dispatch [::nx::core::current object] ::nx::core::cmd::Object::$what {*}$args]}}\n"
":method unknown {m args} {\n"
"error \"Method '$m' unknown for [::nx::core::current object].\\\n"
"Consider '[::nx::core::current object] create $m $args' instead of '[::nx::core::current object] $m $args'\"}\n"
"::nx::core::methodproperty [::nx::core::current object] unknown protected 1}\n"
"Object eval {\n"
":method public {args} {\n"
"set p [lsearch -regexp $args {^(method|alias|attribute|forward|setter)$}]\n"
"if {$p == -1} {error \"$args is not a method defining method\"}\n"
"set r [{*}:$args]\n"
"::nx::core::methodproperty [::nx::core::current object] $r protected false\n"
"return $r}\n"
":method protected {args} {\n"
"set p [lsearch -regexp $args {^(method|alias|attribute|forward|setter)$}]\n"
"if {$p == -1} {error \"$args is not a method defining command\"}\n"
"set r [{*}:$args]\n"
"::nx::core::methodproperty [::nx::core::current object] $r [::nx::core::current method] true\n"
"return $r}\n"
":protected method unknown {m args} {\n"
"if {![::nx::core::current isnext]} {\n"
"error \"[::nx::core::current object]: unable to dispatch method '$m'\"}}\n"
":protected method init args {}\n"
":protected method defaultmethod {} {::nx::core::current object}\n"
":protected method objectparameter {} {;}}\n"
"::nx::core::forward Object forward ::nx::core::forward %self -per-object\n"
"set ::nx::core::signature(::nx::Object-method-forward) {(methodName) obj forward name ?-default default? ?-earlybinding? ?-methodprefix name? ?-objscope? ?-onerror proc? ?-verbose? target ?args?}\n"
"::nx::core::forward Class forward ::nx::core::forward %self\n"
"Class protected object method __unknown {name} {}\n"
"Object public method alias {-nonleaf:switch -objscope:switch methodName cmd} {\n"
"::nx::core::alias [::nx::core::current object] -per-object $methodName \\\n"
"{*}[expr {${objscope} ? \"-objscope\" : \"\"}] \\\n"
"{*}[expr {${nonleaf} ? \"-nonleaf\" : \"\"}] \\\n"
"$cmd}\n"
"Class public method alias {-nonleaf:switch -objscope:switch methodName cmd} {\n"
"::nx::core::alias [::nx::core::current object] $methodName \\\n"
"{*}[expr {${objscope} ? \"-objscope\" : \"\"}] \\\n"
"{*}[expr {${nonleaf} ? \"-nonleaf\" : \"\"}] \\\n"
"$cmd}\n"
"Object public method setter {methodName} {\n"
"::nx::core::setter [::nx::core::current object] -per-object $methodName}\n"
"Class public method setter {methodName} {\n"
"::nx::core::setter [::nx::core::current object] $methodName}\n"
"Object create ::nx::objectInfo\n"
"Object create ::nx::classInfo\n"
"objectInfo eval {\n"
":alias is ::nx::core::objectproperty\n"
":public method info {obj} {\n"
"set methods [list]\n"
"foreach name [::nx::core::cmd::ObjectInfo::methods [::nx::core::current object]] {\n"
"if {$name eq \"unknown\"} continue\n"
"lappend methods $name}\n"
"return \"valid options are: [join [lsort $methods] {, }]\"}\n"
":method unknown {method obj args} {\n"
"error \"[::nx::core::current object] unknown info option \\\"$method\\\"; [$obj info info]\"}}\n"
"classInfo eval {\n"
":alias is ::nx::core::objectproperty\n"
":alias classparent ::nx::core::cmd::ObjectInfo::parent\n"
":alias classchildren ::nx::core::cmd::ObjectInfo::children\n"
":alias info [::nx::core::cmd::ObjectInfo::method objectInfo name info]\n"
":alias unknown [::nx::core::cmd::ObjectInfo::method objectInfo name info]}\n"
"foreach cmd [info command ::nx::core::cmd::ObjectInfo::*] {\n"
"::nx::core::alias ::nx::objectInfo [namespace tail $cmd] $cmd\n"
"::nx::core::alias ::nx::classInfo [namespace tail $cmd] $cmd}\n"
"foreach cmd [info command ::nx::core::cmd::ClassInfo::*] {\n"
"set cmdName [namespace tail $cmd]\n"
"if {$cmdName in [list \"object-mixin-of\" \"class-mixin-of\"]} continue\n"
"::nx::core::alias ::nx::classInfo $cmdName $cmd}\n"
"unset cmd\n"
"Object forward info -onerror ::nx::core::infoError ::nx::objectInfo %1 {%@2 %self}\n"
"Class forward  info -onerror ::nx::core::infoError ::nx::classInfo %1 {%@2 %self}\n"
"proc ::nx::core::infoError msg {\n"
"regsub -all \" <object>\" $msg \"\" msg\n"
"regsub -all \" <class>\" $msg \"\" msg\n"
"regsub {\\\"} $msg \"\\\"info \" msg\n"
"error $msg \"\"}\n"
"Object method abstract {methtype -per-object:switch methname arglist} {\n"
"if {$methtype ne \"method\"} {\n"
"error \"invalid method type '$methtype', must be 'method'\"}\n"
"set body \"\n"
"if {!\\[::nx::core::current isnextcall\\]} {\n"
"error \\\"Abstract method $methname $arglist called\\\"} else {::nx::core::next}\n"
"\"\n"
"if {${per-object}} {\n"
":method -per-object $methname $arglist $body}  else {\n"
":method $methname $arglist $body}}\n"
"proc ::nx::core::unsetExitHandler {} {\n"
"proc ::nx::core::__exitHandler {} {}}\n"
"proc ::nx::core::setExitHandler {newbody} {::proc ::nx::core::__exitHandler {} $newbody}\n"
"proc ::nx::core::getExitHandler {} {::info body ::nx::core::__exitHandler}\n"
"::nx::core::unsetExitHandler\n"
"namespace export Object Class next self}\n"
"namespace eval ::nx {\n"
"::nx::Class create ::nx::MetaSlot\n"
"::nx::core::relation ::nx::MetaSlot superclass ::nx::Class\n"
"::nx::MetaSlot public method slotName {name baseObject} {\n"
"set slotParent ${baseObject}::slot\n"
"if {![::nx::core::objectproperty ${slotParent} object]} {\n"
"::nx::Object create ${slotParent}}\n"
"return ${slotParent}::$name}\n"
"::nx::MetaSlot method createFromParameterSyntax {\n"
"target -per-object:switch\n"
"{-initblock \"\"}\n"
"value default:optional} {\n"
"set opts [list]\n"
"set colonPos [string first : $value]\n"
"if {$colonPos == -1} {\n"
"set name $value} else {\n"
"set properties [string range $value [expr {$colonPos+1}] end]\n"
"set name [string range $value 0 [expr {$colonPos -1}]]\n"
"foreach property [split $properties ,] {\n"
"if {$property eq \"required\"} {\n"
"lappend opts -required 1} elseif {$property eq \"multivalued\"} {\n"
"lappend opts -multivalued 1} elseif {[string match type=* $property]} {\n"
"set type [string range $property 5 end]\n"
"if {![string match ::* $type]} {set type ::$type}} elseif {[string match arg=* $property]} {\n"
"set argument [string range $property 4 end]\n"
"lappend opts -arg $argument} else {\n"
"set type $property}}}\n"
"if {[info exists type]} {\n"
"lappend opts -type $type}\n"
"if {[info exists default]} {\n"
"lappend opts -default $default}\n"
"if {${per-object}} {\n"
"lappend opts -per-object true\n"
"set info ObjectInfo} else {\n"
"set info ClassInfo}\n"
":create [:slotName $name $target] {*}$opts $initblock\n"
"return [::nx::core::cmd::${info}::method $target name $name]}\n"
"::nx::MetaSlot create ::nx::Slot\n"
"::nx::MetaSlot create ::nx::ObjectParameterSlot\n"
"::nx::core::relation ::nx::ObjectParameterSlot superclass ::nx::Slot\n"
"::nx::MetaSlot create ::nx::MethodParameterSlot\n"
"::nx::core::relation ::nx::MethodParameterSlot superclass ::nx::Slot\n"
"::nx::MethodParameterSlot create ::nx::methodParameterSlot\n"
"proc createBootstrapAttributeSlots {class definitions} {\n"
"foreach att $definitions {\n"
"if {[llength $att]>1} {foreach {att default} $att break}\n"
"set slotObj [::nx::ObjectParameterSlot slotName $att $class]\n"
"::nx::ObjectParameterSlot create $slotObj\n"
"if {[info exists default]} {\n"
"::nx::core::setvar $slotObj default $default\n"
"unset default}\n"
"::nx::core::setter $class $att}\n"
"foreach att $definitions {\n"
"if {[llength $att]>1} {foreach {att default} $att break}\n"
"if {[info exists default]} {\n"
"foreach i [::nx::core::cmd::ClassInfo::instances $class] {\n"
"if {![::nx::core::existsvar $i $att]} {\n"
"if {[string match {*\\[*\\]*} $default]} {\n"
"set value [::nx::core::dispatch $i -objscope ::eval subst $default]} else {\n"
"set value $default}\n"
"::nx::core::setvar $i $att $value}}\n"
"unset default}}\n"
"$class __invalidateobjectparameter}\n"
"createBootstrapAttributeSlots ::nx::Slot {\n"
"{name}\n"
"{multivalued false}\n"
"{required false}\n"
"default\n"
"type}\n"
"createBootstrapAttributeSlots ::nx::ObjectParameterSlot {\n"
"{name \"[namespace tail [::nx::core::current object]]\"}\n"
"{methodname}\n"
"{domain \"[lindex [regexp -inline {^(.*)::slot::[^:]+$} [::nx::core::current object]] 1]\"}\n"
"{defaultmethods {get assign}}\n"
"{manager \"[::nx::core::current object]\"}\n"
"{per-object false}}\n"
"::nx::core::alias ::nx::ObjectParameterSlot get ::nx::core::setvar\n"
"::nx::core::alias ::nx::ObjectParameterSlot assign ::nx::core::setvar\n"
"::nx::ObjectParameterSlot public method add {obj prop value {pos 0}} {\n"
"if {![set :multivalued]} {\n"
"error \"Property $prop of [set :domain]->$obj ist not multivalued\"}\n"
"if {[::nx::core::existsvar $obj $prop]} {\n"
"::nx::core::setvar $obj $prop [linsert [::nx::core::setvar $obj $prop] $pos $value]} else {\n"
"::nx::core::setvar $obj $prop [list $value]}}\n"
"::nx::ObjectParameterSlot public method delete {-nocomplain:switch obj prop value} {\n"
"set old [::nx::core::setvar $obj $prop]\n"
"set p [lsearch -glob $old $value]\n"
"if {$p>-1} {::nx::core::setvar $obj $prop [lreplace $old $p $p]} else {\n"
"error \"$value is not a $prop of $obj (valid are: $old)\"}}\n"
"::nx::ObjectParameterSlot method unknown {method args} {\n"
"set methods [list]\n"
"foreach m [:info callable] {\n"
"if {[::nx::Object info callable $m] ne \"\"} continue\n"
"if {[string match __* $m]} continue\n"
"lappend methods $m}\n"
"error \"Method '$method' unknown for slot [::nx::core::current object]; valid are: {[lsort $methods]}\"}\n"
"::nx::ObjectParameterSlot public method destroy {} {\n"
"if {${:domain} ne \"\" && [::nx::core::objectproperty ${:domain} class]} {\n"
"${:domain} __invalidateobjectparameter}\n"
"::nx::core::next}\n"
"::nx::ObjectParameterSlot protected method init {args} {\n"
"if {${:domain} eq \"\"} {\n"
"set :domain [::nx::core::current callingobject]}\n"
"if {${:domain} ne \"\"} {\n"
"if {![info exists :methodname]} {\n"
"set :methodname ${:name}}\n"
"if {[::nx::core::objectproperty ${:domain} class]} {\n"
"${:domain} __invalidateobjectparameter}\n"
"if {${:per-object} && [info exists :default] } {\n"
"::nx::core::setvar ${:domain} ${:name} ${:default}}\n"
"set cl [expr {${:per-object} ? \"Object\" : \"Class\"}]\n"
"::nx::core::forward ${:domain} ${:name} \\\n"
"${:manager} \\\n"
"[list %1 [${:manager} defaultmethods]] %self \\\n"
"${:methodname}}}\n"
"::nx::MetaSlot __invalidateobjectparameter\n"
"::nx::ObjectParameterSlot method toParameterSyntax {{name:substdefault ${:name}}} {\n"
"set objparamdefinition $name\n"
"set methodparamdefinition \"\"\n"
"set objopts [list]\n"
"set methodopts [list]\n"
"set type \"\"\n"
"if {[info exists :required] && ${:required}} {\n"
"lappend objopts required\n"
"lappend methodopts required}\n"
"if {[info exists :type]} {\n"
"if {[string match ::* ${:type}]} {\n"
"set type [expr {[::nx::core::objectproperty ${:type} metaclass] ? \"class\" : \"object\"}]\n"
"lappend objopts type=${:type}\n"
"lappend methodopts type=${:type}} else {\n"
"set type ${:type}}}\n"
"if {[info exists :multivalued] && ${:multivalued}} {\n"
"if {!([info exists :type] && ${:type} eq \"relation\")} {\n"
"lappend objopts multivalued} else {}}\n"
"if {[info exists :arg]} {\n"
"set prefix [expr {$type eq \"object\" || $type eq \"class\" ? \"type\" : \"arg\"}]\n"
"lappend objopts $prefix=${:arg}\n"
"lappend methodopts $prefix=${:arg}}\n"
"if {[info exists :default]} {\n"
"set arg ${:default}\n"
"if {[string match {*\\[*\\]*} $arg]\n"
"&& $type ne \"substdefault\"} {\n"
"lappend objopts substdefault}} elseif {[info exists :initcmd]} {\n"
"set arg ${:initcmd}\n"
"lappend objopts initcmd}\n"
"if {[info exists :methodname]} {\n"
"if {${:methodname} ne ${:name}} {\n"
"lappend objopts arg=${:methodname}\n"
"lappend methodopts arg=${:methodname}}}\n"
"if {$type ne \"\"} {\n"
"set objopts [linsert $objopts 0 $type]\n"
"if {$type ne \"substdefault\"} {set methodopts [linsert $methodopts 0 $type]}}\n"
"lappend objopts slot=[::nx::core::current object]\n"
"if {[llength $objopts] > 0} {\n"
"append objparamdefinition :[join $objopts ,]}\n"
"if {[llength $methodopts] > 0} {\n"
"set methodparamdefinition [join $methodopts ,]}\n"
"if {[info exists arg]} {\n"
"lappend objparamdefinition $arg}\n"
"return [list oparam $objparamdefinition mparam $methodparamdefinition]}\n"
"proc ::nx::core::parametersFromSlots {obj} {\n"
"set parameterdefinitions [list]\n"
"foreach slot [::nx::objectInfo slotobjects $obj] {\n"
"if {[::nx::core::objectproperty ::xotcl::Object class]\n"
"&& [::nx::core::objectproperty $obj type ::xotcl::Object] &&\n"
"([$slot name] eq \"mixin\" || [$slot name] eq \"filter\")} continue\n"
"array set \"\" [$slot toParameterSyntax]\n"
"lappend parameterdefinitions -$(oparam)}\n"
"return $parameterdefinitions}\n"
"::nx::Object protected method objectparameter {{lastparameter __initcmd:initcmd,optional}} {\n"
"set parameterdefinitions [::nx::core::parametersFromSlots [::nx::core::current object]]\n"
"if {[::nx::core::objectproperty [::nx::core::current object] class]} {\n"
"lappend parameterdefinitions -parameter:method,optional}\n"
"lappend parameterdefinitions \\\n"
"-noinit:method,optional,noarg \\\n"
"-volatile:method,optional,noarg \\\n"
"{*}$lastparameter\n"
"return $parameterdefinitions}\n"
"::nx::MetaSlot create ::nx::RelationSlot\n"
"createBootstrapAttributeSlots ::nx::RelationSlot {\n"
"{multivalued true}\n"
"{type relation}\n"
"{elementtype ::nx::Class}}\n"
"::nx::core::relation ::nx::RelationSlot superclass ::nx::ObjectParameterSlot\n"
"::nx::core::alias ::nx::RelationSlot assign ::nx::core::relation\n"
"::nx::RelationSlot protected method init {} {\n"
"if {${:type} ne \"relation\"} {\n"
"error \"RelationSlot requires type == \\\"relation\\\"\"}\n"
"::nx::core::next}\n"
"::nx::RelationSlot protected method delete_value {obj prop old value} {\n"
"if {[string first * $value] > -1 || [string first \\[ $value] > -1} {\n"
"if {${:elementtype} ne \"\" && ![string match ::* $value]} {\n"
"set value ::$value}\n"
"return [lsearch -all -not -glob -inline $old $value]} elseif {${:elementtype} ne \"\"} {\n"
"if {[string first :: $value] == -1} {\n"
"if {![::nx::core::objectproperty $value object]} {\n"
"error \"$value does not appear to be an object\"}\n"
"set value [::nx::core::dispatch $value -objscope ::nx::core::current object]}\n"
"if {![::nx::core::objectproperty ${:elementtype} class]} {\n"
"error \"$value does not appear to be of type ${:elementtype}\"}}\n"
"set p [lsearch -exact $old $value]\n"
"if {$p > -1} {\n"
"return [lreplace $old $p $p]} else {\n"
"error \"$value is not a $prop of $obj (valid are: $old)\"}}\n"
"::nx::RelationSlot public method delete {-nocomplain:switch obj prop value} {\n"
"$obj $prop [:delete_value $obj $prop [$obj info $prop] $value]}\n"
"::nx::RelationSlot public method get {obj prop} {\n"
"::nx::core::relation $obj $prop}\n"
"::nx::RelationSlot public method add {obj prop value {pos 0}} {\n"
"if {![set :multivalued]} {\n"
"error \"Property $prop of ${:domain}->$obj ist not multivalued\"}\n"
"set oldSetting [::nx::core::relation $obj $prop]\n"
"uplevel [list ::nx::core::relation $obj $prop [linsert $oldSetting $pos $value]]}\n"
"::nx::RelationSlot public method delete {-nocomplain:switch obj prop value} {\n"
"uplevel [list ::nx::core::relation $obj $prop [:delete_value $obj $prop [::nx::core::relation $obj $prop] $value]]}\n"
"proc ::nx::core::register_system_slots {os} {\n"
"${os}::Object alloc ${os}::Class::slot\n"
"${os}::Object alloc ${os}::Object::slot\n"
"::nx::RelationSlot create ${os}::Class::slot::superclass\n"
"::nx::core::alias         ${os}::Class::slot::superclass assign ::nx::core::relation\n"
"::nx::RelationSlot create ${os}::Object::slot::class -multivalued false\n"
"::nx::core::alias         ${os}::Object::slot::class assign ::nx::core::relation\n"
"::nx::RelationSlot create ${os}::Object::slot::mixin -methodname object-mixin\n"
"::nx::RelationSlot create ${os}::Object::slot::filter -elementtype \"\"\n"
"::nx::RelationSlot create ${os}::Class::slot::mixin -methodname class-mixin\n"
"::nx::RelationSlot create ${os}::Class::slot::filter -elementtype \"\" \\\n"
"-methodname class-filter\n"
"::nx::RelationSlot create ${os}::Class::slot::object-mixin\n"
"::nx::RelationSlot create ${os}::Class::slot::object-filter -elementtype \"\"}\n"
"::nx::core::register_system_slots ::nx\n"
"proc ::nx::core::register_system_slots {} {}\n"
"::nx::MetaSlot __invalidateobjectparameter\n"
"::nx::MetaSlot create ::nx::Attribute -superclass ::nx::ObjectParameterSlot\n"
"createBootstrapAttributeSlots ::nx::Attribute {\n"
"{value_check once}\n"
"incremental\n"
"initcmd\n"
"valuecmd\n"
"valuechangedcmd\n"
"arg}\n"
"::nx::Attribute method __default_from_cmd {obj cmd var sub op} {\n"
"$obj trace remove variable $var $op [list [::nx::core::current object] [::nx::core::current method] $obj $cmd]\n"
"::nx::core::setvar $obj $var [$obj eval $cmd]}\n"
"::nx::Attribute method __value_from_cmd {obj cmd var sub op} {\n"
"::nx::core::setvar $obj $var [$obj eval $cmd]}\n"
"::nx::Attribute method __value_changed_cmd {obj cmd var sub op} {\n"
"eval $cmd}\n"
"::nx::Attribute protected method init {} {\n"
"::nx::core::next ;# do first ordinary slot initialization\n"
"set __initcmd \"\"\n"
"if {[info exists :default]} {} elseif [info exists :initcmd] {\n"
"append __initcmd \":trace add variable [list ${:name}] read \\\n"
"\\[list [::nx::core::current object] __default_from_cmd \\[::nx::core::current object\\] [list [set :initcmd]]\\]\\n\"} elseif [info exists :valuecmd] {\n"
"append __initcmd \":trace add variable [list ${:name}] read \\\n"
"\\[list [::nx::core::current object] __value_from_cmd \\[::nx::core::current object\\] [list [set :valuecmd]]\\]\"}\n"
"array set \"\" [:toParameterSyntax ${:name}]\n"
"if {$(mparam) ne \"\"} {\n"
"if {[info exists :multivalued] && ${:multivalued}} {\n"
":method assign [list obj var value:$(mparam),multivalued,slot=[::nx::core::current object]] {\n"
"::nx::core::setvar $obj $var $value}\n"
":method add [list obj prop value:$(mparam),slot=[::nx::core::current object] {pos 0}] {\n"
"::nx::core::next}} else {\n"
":method assign [list obj var value:$(mparam),slot=[::nx::core::current object]] {\n"
"::nx::core::setvar $obj $var $value}}}\n"
"if {[info exists :valuechangedcmd]} {\n"
"append __initcmd \":trace add variable [list ${:name}] write \\\n"
"\\[list [::nx::core::current object] __value_changed_cmd \\[::nx::core::current object\\] [list [set :valuechangedcmd]]\\]\"}\n"
"if {$__initcmd ne \"\"} {\n"
"set :initcmd $__initcmd}}\n"
"::nx::Class create ::nx::Attribute::Optimizer {\n"
":method method args  {::nx::core::next; :optimize}\n"
":method forward args {::nx::core::next; :optimize}\n"
":protected method init args {::nx::core::next; :optimize}\n"
":public method optimize {} {\n"
"if {![info exists :methodname]} {return}\n"
"set object [expr {${:per-object} ? {object} : {}}]\n"
"if {${:per-object}} {\n"
"set perObject -per-object\n"
"set infokind Object} else {\n"
"set perObject \"\"\n"
"set infokind Class}\n"
"if {[::nx::core::cmd::${infokind}Info::method ${:domain} name ${:name}] ne \"\"} {\n"
"::nx::core::forward ${:domain} {*}$perObject ${:name} \\\n"
"${:manager} \\\n"
"[list %1 [${:manager} defaultmethods]] %self \\\n"
"${:methodname}}\n"
"if {[info exists :incremental] && ${:incremental}} return\n"
"if {[set :defaultmethods] ne {get assign}} return\n"
"set assignInfo [:info callable -which assign]\n"
"if {$assignInfo ne \"::nx::ObjectParameterSlot alias assign ::nx::core::setvar\" &&\n"
"[lindex $assignInfo {end 0}] ne \"::nx::core::setvar\" } return\n"
"if {[:info callable -which get] ne \"::nx::ObjectParameterSlot alias get ::nx::core::setvar\"} return\n"
"array set \"\" [:toParameterSyntax ${:name}]\n"
"if {$(mparam) ne \"\"} {\n"
"set setterParam [lindex $(oparam) 0]} else {\n"
"set setterParam ${:name}}\n"
"::nx::core::setter ${:domain} {*}$perObject $setterParam}}\n"
"::nx::Attribute mixin add ::nx::Attribute::Optimizer\n"
"::nx::Class method attribute {spec {-slotclass ::nx::Attribute} {initblock \"\"}} {\n"
"$slotclass createFromParameterSyntax [::nx::core::current object] -initblock $initblock {*}$spec}\n"
"::nx::Object method attribute {spec {-slotclass ::nx::Attribute} {initblock \"\"}} {\n"
"$slotclass createFromParameterSyntax [::nx::core::current object] -per-object -initblock $initblock {*}$spec}\n"
"::nx::Class public method parameter arglist {\n"
"foreach arg $arglist {\n"
"::nx::Attribute createFromParameterSyntax [::nx::core::current object] {*}$arg}\n"
"set slot [::nx::core::current object]::slot\n"
"if {![::nx::core::objectproperty $slot object]} {::nx::Object create $slot}\n"
"::nx::core::setvar $slot __parameter $arglist}\n"
"::nx::core::method ::nx::classInfo parameter {class} {\n"
"set slot ${class}::slot\n"
"if {![::nx::core::objectproperty $slot object]} {::nx::Object create $slot}\n"
"if {[::nx::core::existsvar $slot __parameter]} {\n"
"return [::nx::core::setvar $slot __parameter]}\n"
"return \"\"}\n"
"proc createBootstrapAttributeSlots {} {}\n"
"::nx::Slot method type=hasmixin {name value arg} {\n"
"if {![::nx::core::objectproperty $value hasmixin $arg]} {\n"
"error \"expected object with mixin $arg but got \\\"$value\\\" for parameter $name\"}\n"
"return $value}\n"
"::nx::Slot method type=baseclass {name value} {\n"
"if {![::nx::core::objectproperty $value baseclass]} {\n"
"error \"expected baseclass but got \\\"$value\\\" for parameter $name\"}\n"
"return $value}\n"
"::nx::Slot method type=metaclass {name value} {\n"
"if {![::nx::core::objectproperty $value metaclass]} {\n"
"error \"expected metaclass but got \\\"$value\\\" for parameter $name\"}\n"
"return $value}}\n"
"::nx::Class create ::nx::ScopedNew -superclass ::nx::Class {\n"
":attribute {withclass ::nx::Object}\n"
":attribute container\n"
":protected method init {} {\n"
":public method new {-childof args} {\n"
"::nx::core::importvar [::nx::core::current class] {container object} withclass\n"
"if {![::nx::core::objectproperty $object object]} {\n"
"$withclass create $object}\n"
"eval ::nx::core::next -childof $object $args}}}\n"
"::nx::Object public method contains {\n"
"{-withnew:boolean true}\n"
"-object\n"
"{-class ::nx::Object}\n"
"cmds} {\n"
"if {![info exists object]} {set object [::nx::core::current object]}\n"
"if {![::nx::core::objectproperty $object object]} {$class create $object}\n"
"$object requireNamespace\n"
"if {$withnew} {\n"
"set m [::nx::ScopedNew new -volatile \\\n"
"-container $object -withclass $class]\n"
"::nx::Class mixin add $m end\n"
"if {[::nx::core::objectproperty ::xotcl::Class class]} {::xotcl::Class instmixin add $m end}\n"
"namespace eval $object $cmds\n"
"::nx::Class mixin delete $m\n"
"if {[::nx::core::objectproperty ::xotcl::Class class]} {::xotcl::Class instmixin delete $m}} else {\n"
"namespace eval $object $cmds}}\n"
"::nx::Class forward slots %self contains \\\n"
"-object {%::nx::core::dispatch [::nx::core::current object] -objscope ::subst [::nx::core::current object]::slot}\n"
"::nx::Class create ::nx::CopyHandler {\n"
":attribute {targetList \"\"}\n"
":attribute {dest \"\"}\n"
":attribute objLength\n"
":method makeTargetList {t} {\n"
"lappend :targetList $t\n"
"if {[::nx::core::objectproperty $t object]} {\n"
"if {[$t info hasnamespace]} {\n"
"set children [$t info children]} else {\n"
"return}}\n"
"foreach c [namespace children $t] {\n"
"if {![::nx::core::objectproperty $c object]} {\n"
"lappend children [namespace children $t]}}\n"
"foreach c $children {\n"
":makeTargetList $c}}\n"
":method copyNSVarsAndCmds {orig dest} {\n"
"::nx::core::namespace_copyvars $orig $dest\n"
"::nx::core::namespace_copycmds $orig $dest}\n"
":method getDest origin {\n"
"set tail [string range $origin [set :objLength] end]\n"
"return ::[string trimleft [set :dest]$tail :]}\n"
":method copyTargets {} {\n"
"foreach origin [set :targetList] {\n"
"set dest [:getDest $origin]\n"
"if {[::nx::core::objectproperty $origin object]} {\n"
"if {[::nx::core::objectproperty $origin class]} {\n"
"set cl [[$origin info class] create $dest -noinit]\n"
"set obj $cl\n"
"$cl superclass [$origin info superclass]\n"
"::nx::core::assertion $cl class-invar [::nx::core::assertion $origin class-invar]\n"
"::nx::core::relation $cl class-filter [::nx::core::relation $origin class-filter]\n"
"::nx::core::relation $cl class-mixin [::nx::core::relation $origin class-mixin]\n"
":copyNSVarsAndCmds ::nx::core::classes$origin ::nx::core::classes$dest} else {\n"
"set obj [[$origin info class] create $dest -noinit]}\n"
"::nx::core::assertion $obj check [::nx::core::assertion $origin check]\n"
"::nx::core::assertion $obj object-invar [::nx::core::assertion $origin object-invar]\n"
"::nx::core::relation $obj object-filter [::nx::core::relation $origin object-filter]\n"
"::nx::core::relation $obj object-mixin [::nx::core::relation $origin object-mixin]\n"
"if {[$origin info hasnamespace]} {\n"
"$obj requireNamespace}} else {\n"
"namespace eval $dest {}}\n"
":copyNSVarsAndCmds $origin $dest\n"
"foreach i [::nx::core::cmd::ObjectInfo::forward $origin] {\n"
"eval [concat ::nx::core::forward $dest -per-object $i [::nx::core::cmd::ObjectInfo::forward $origin -definition $i]]}\n"
"if {[::nx::core::objectproperty $origin class]} {\n"
"foreach i [::nx::core::cmd::ClassInfo::forward $origin] {\n"
"eval [concat ::nx::core::forward $dest $i [::nx::core::cmd::ClassInfo::forward $origin -definition $i]]}}\n"
"set traces [list]\n"
"foreach var [$origin info vars] {\n"
"set cmds [::nx::core::dispatch $origin -objscope ::trace info variable $var]\n"
"if {$cmds ne \"\"} {\n"
"foreach cmd $cmds {\n"
"foreach {op def} $cmd break\n"
"if {[lindex $def 0] eq $origin} {\n"
"set def [concat $dest [lrange $def 1 end]]}\n"
"$dest trace add variable $var $op $def}}}}\n"
"foreach origin [set :targetList] {\n"
"if {[::nx::core::objectproperty $origin class]} {\n"
"set dest [:getDest $origin]\n"
"foreach oldslot [$origin info slots] {\n"
"set newslot [::nx::Slot slotName [namespace tail $oldslot] $dest]\n"
"if {[$oldslot domain] eq $origin}   {$newslot domain $cl}\n"
"if {[$oldslot manager] eq $oldslot} {$newslot manager $newslot}}}}}\n"
":public method copy {obj dest} {\n"
"set :objLength [string length $obj]\n"
"set :dest $dest\n"
":makeTargetList $obj\n"
":copyTargets}}\n"
"::nx::Object public method copy newName {\n"
"if {[string compare [string trimleft $newName :] [string trimleft [::nx::core::current object] :]]} {\n"
"[::nx::CopyHandler new -volatile] copy [::nx::core::current object] $newName}}\n"
"::nx::Object public method move newName {\n"
"if {[string trimleft $newName :] ne [string trimleft [::nx::core::current object] :]} {\n"
"if {$newName ne \"\"} {\n"
":copy $newName}\n"
"if {[::nx::core::objectproperty [::nx::core::current object] class] && $newName ne \"\"} {\n"
"foreach subclass [:info subclass] {\n"
"set scl [$subclass info superclass]\n"
"if {[set index [lsearch -exact $scl [::nx::core::current object]]] != -1} {\n"
"set scl [lreplace $scl $index $index $newName]\n"
"$subclass superclass $scl}}	}\n"
":destroy}}\n"
"namespace eval ::nx {\n"
"Object create ::nx::var {\n"
":alias exists ::nx::core::existsvar\n"
":alias import ::nx::core::importvar\n"
":alias set ::nx::core::setvar}}\n"
"namespace eval ::nx::core {\n"
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
"namespace export tmpdir}\n"
"namespace eval ::nx {\n"
"namespace export Attribute current\n"
"if {![info exists ::env(HOME)]} {set ::env(HOME) /root}\n"
"set ::nx::confdir ~/.xotcl\n"
"set ::nx::logdir $::nx::confdir/log\n"
"unset bootstrap}\n"
"";

