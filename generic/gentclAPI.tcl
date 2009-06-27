



# objectMethod
# classMethod
# infoObjectMethod
# infoClassMethod
# checkMethod

set objCmdProc "(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv \[\]);"
proc genifd {argDefinitions} {
  set l [list]
  foreach argDefinition $argDefinitions {
    array set "" $argDefinition
    if {$(-type) eq ""} {set type NULL} {set type "\"$(-type)\""}
    lappend l "{\"$(-argName)\", $(-required), $(-nrArgs), $type}"
  }
  join $l ",\n  "
}

proc genc {} {
  set decls ""
  set enums [list]
  set ifds [list]
  foreach key [lsort [array names ::definitions]] {
    array set d $::definitions($key)
    append decls "static int $d(implementation)$::objCmdProc\n"
    lappend enums $d(idx)
    lappend ifds "{\"$d(methodName)\", $d(implementation), {\n  [genifd $d(argDefintions)]}\n}"
  }
  puts {
typedef struct {
  char *methodName;
  Tcl_ObjCmdProc *proc;
  interfaceDefinition ifd;
} methodDefinition2;
  }
  puts $decls
  set enumString [join $enums ",\n "]
  puts "enum {\n $enumString\n} XOTclMethods;\n"
  set definitionString [join $ifds ",\n"]
  puts "static methodDefinition2 methodDefinitons\[\] = \{\n$definitionString\n\};\n"
}

proc methodDefinition {methodName methodType implementation argDefinitions} {
  set d(methodName) $methodName
  set d(implementation) $implementation
  set d(methodType) $methodType
  set d(idx) ${implementation}Idx
  set completed [list]
  foreach argDefinition $argDefinitions {
    array set "" {-required 0 -nrArgs 0 -type ""}
    array set "" $argDefinition
    lappend completed [array get ""]
  }
  set d(argDefintions) $completed
  set ::definitions($d(methodType)-$d(implementation)-$d(methodName)) [array get d]
}

proc infoClassMethod {methodName implementation argDefinitions} {
  methodDefinition $methodName infoClassMethod $implementation $argDefinitions
}

infoClassMethod instances XOTclClassInfoHeritageMethod {
  {-argName "class"   -required 1 -nrargs 0 -type class}
  {-argName "pattern"}
}

infoClassMethod instances XOTclClassInfoInstancesMethod {
  {-argName "class"   -required 1 -nrargs 0 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}

infoClassMethod instargs XOTclClassInfoInstargsMethod {
  {-argName "class" -required 1 -type class}
  {-argName "methodName" -required 1}
}

infoClassMethod instbody XOTclClassInfoInstbodyMethod {
  {-argName "class" -required 1 -type class}
  {-argName "methodName" -required 1}
}

infoClassMethod instances XOTclClassInfoInstcommandsMethod {
  {-argName "class"   -required 1 -nrargs 0 -type class}
  {-argName "pattern"}
}

infoClassMethod instdefault XOTclClassInfoInstdefaultMethod {
  {-argName "class"   -required 1 -nrargs 0 -type class}
  {-argName "methodName" -required 1}
  {-argName "arg" -required 1}
  {-argName "var" -required 1}
}

infoClassMethod instfilter XOTclClassInfoInstfilterMethod {
  {-argName "class"   -required 1 -nrargs 0 -type class}
  {-argName "-guards"}
  {-argName "pattern"}
}

infoClassMethod instfilterguard XOTclClassInfoInstfilterguardMethod {
  {-argName "class"  -required 1 -nrargs 0 -type class}
  {-argName "filter" -required 1}
}



genc