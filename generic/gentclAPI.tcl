
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
    switch $(-type) {
      "" {set type NULL}
      default {set type "\"$(-type)\""}
    }
    lappend l "{\"$(-argName)\", $(-required), $(-nrArgs), $type}"
  }
  join $l ",\n  "
}

proc gencall {argDefinitions cDefsVar ifDefVar arglistVar preVar postVar} {
  upvar $cDefsVar cDefs $ifDefVar ifDef $arglistVar arglist $preVar pre $postVar post
  set c [list]
  set a [list]
  set i 0
  set pre ""; set post ""
  foreach argDefinition $argDefinitions {
    array set "" $argDefinition
    set ifSet 0
    if {[regexp {^-(.*)$} $(-argName) _ switchName]} {
      set varName with[string totitle $switchName]
      set calledArg $varName
      set type "int "
    } else {
      set varName $(-argName)
      set calledArg $varName
      switch $(-type) {
        ""           {set type "char *"}
        "class"      {set type "XOTclClass *"}
        "object"     {set type "XOTclObject *"}
        "tclobj"     {set type "Tcl_Obj *"}
        "objpattern" {
          set type "Tcl_Obj *"
          lappend c "char *${varName}String = NULL;" "XOTclObject *${varName}Obj = NULL;"
          set calledArg "${varName}String, ${varName}Obj"
          lappend if "char *${varName}String" "XOTclObject *${varName}Obj"
          set ifSet 1
          append pre [subst -nocommands {
    if (getMatchObject3(interp, ${varName},  &pc, &${varName}Obj, &${varName}String) == -1) {
      return TCL_OK;
    }
          }]
#          append post [subst -nocommands {
#    if (${varName}Obj) {
#      Tcl_SetObjResult(interp, returnCode ? ${varName}Obj->cmdName : XOTclGlobalObjects[XOTE_EMPTY]);
#    }
#          }]
        }
      }
    }
    if {!$ifSet} {lappend if "$type$varName"}
    lappend c [subst -nocommands {$type $varName = ($type)pc.clientData[$i];}]
    lappend a $calledArg
    incr i
  }
  set ifDef [join $if ", "]
  set cDefs [join $c "\n    "]
  set arglist [join $a ", "]
}


proc genifds {} {
  set stubDecls ""
  set decls ""
  set enums [list]
  set ifds [list]
  foreach key [lsort [array names ::definitions]] {
    array set d $::definitions($key)
    append stubDecls "static int $d(stub)$::objCmdProc\n"
    lappend enums $d(idx)
    lappend ifds "{\"$d(methodName)\", $d(stub), {\n  [genifd $d(argDefintions)]}\n}"

    gencall $d(argDefintions) cDefs ifDef arglist pre post
    append decls "static int $d(implementation)(Tcl_Interp *interp, $ifDef);\n"
    if {$post ne ""} {
      append cDefs "\n    int returnCode;"
      set call "returnCode = $d(implementation)(interp, $arglist);"
      set post [string trimright $post]
      append post "\n    return TCL_OK;"
    } else {
      set call "return $d(implementation)(interp, $arglist);"
    }
    append fns [subst -nocommands {
static int
$d(stub)(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
  if (parse2(clientData, interp, objc, objv, $d(idx), &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    $cDefs
$pre
    $call
$post
  }
}
  }]
  }

  puts {
typedef struct {
  char *methodName;
  Tcl_ObjCmdProc *proc;
  interfaceDefinition ifd;
} methodDefinition2;

static int parse2(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
                  int idx, parseContext *pc);
static int getMatchObject3(Tcl_Interp *interp, Tcl_Obj *patternObj,  parseContext *pc,
                           XOTclObject **matchObject, char **pattern);
  }
  puts $stubDecls
  puts $decls
  set enumString [join $enums ",\n "]
  puts "enum {\n $enumString\n} XOTclMethods;\n"
  puts $fns
  set definitionString [join $ifds ",\n"]
  puts "static methodDefinition2 methodDefinitons\[\] = \{\n$definitionString\n\};\n"
}

proc methodDefinition {methodName methodType implementation argDefinitions} {
  set d(methodName) $methodName
  set d(implementation) $implementation
  set d(stub) ${implementation}Stub
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

proc infoObjectMethod {methodName implementation argDefinitions} {
  methodDefinition $methodName infoObjectMethod $implementation $argDefinitions
}

#
# info object methods
#
infoObjectMethod args XOTclObjInfoArgsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod body XOTclObjInfoBodyMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod check XOTclObjInfoCheckMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod children XOTclObjInfoChildrenMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
infoObjectMethod class XOTclObjInfoClassMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod commands XOTclObjInfoCommandsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
infoObjectMethod default XOTclObjInfoDefaultMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
  {-argName "arg" -required 1}
  {-argName "var" -required 1 -type tclobj}
}
infoObjectMethod filter XOTclObjInfoFilterMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-order"}
  {-argName "-guards"}
  {-argName "pattern"}
}
infoObjectMethod filterguard XOTclObjInfoFilterguardMethod {
  {-argName "object" -required 1 -type object}
  {-argName "filter" -required 1}
}
infoObjectMethod forward XOTclObjInfoForwardMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-definition"}
  {-argName "methodName" -required 1}
}
infoObjectMethod hasnamespace XOTclObjInfoHasnamespaceMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod invar XOTclObjInfoInvarMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod methods XOTclObjInfoMethodsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-noprocs"}
  {-argName "-nocmds"}
  {-argName "-nomixins"}
  {-argName "-incontext"}
  {-argName "pattern"}
}
infoObjectMethod mixin XOTclObjInfoMixinMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-guards"}
  {-argName "-order"}
  {-argName "pattern" -type objpattern}
}
infoObjectMethod mixinguard XOTclObjInfoMixinguardMethod {
  {-argName "object" -required 1 -type object}
  {-argName "mixin"  -required 1}
}
infoObjectMethod nonposargs XOTclObjInfoNonposargsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod parent XOTclObjInfoParentMethod {
  {-argName "object" -required 1 -type object}
}
infoObjectMethod parametercmd XOTclObjInfoParametercmdMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern"}
}
infoObjectMethod post XOTclObjInfoPostMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod pre XOTclObjInfoPreMethod {
  {-argName "object" -required 1 -type object}
  {-argName "methodName" -required 1}
}
infoObjectMethod precedence XOTclObjInfoPrecedenceMethod {
  {-argName "object" -required 1 -type object}
  {-argName "-intrinsic"}
  {-argName "pattern" -required 0}
}
infoObjectMethod procs XOTclObjInfoProcsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
infoObjectMethod slotobjects XOTclObjInfoSlotObjectsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}
infoObjectMethod vars XOTclObjInfoVarsMethod {
  {-argName "object" -required 1 -type object}
  {-argName "pattern" -required 0}
}


#
# info class methods
#
infoClassMethod instances XOTclClassInfoHeritageMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "pattern"}
}

infoClassMethod instances XOTclClassInfoInstancesMethod {
  {-argName "class"   -required 1 -type class}
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
  {-argName "class"   -required 1 -type class}
  {-argName "pattern"}
}

infoClassMethod instdefault XOTclClassInfoInstdefaultMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "methodName" -required 1}
  {-argName "arg" -required 1}
  {-argName "var" -required 1 -type tclobj}
}

infoClassMethod instfilter XOTclClassInfoInstfilterMethod {
  {-argName "class"   -required 1 -type class}
  {-argName "-guards"}
  {-argName "pattern"}
}

infoClassMethod instfilterguard XOTclClassInfoInstfilterguardMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "filter" -required 1}
}

infoClassMethod instforward XOTclClassInfoInstforwardMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-definition"}
  {-argName "methodName" -required 1}
}

infoClassMethod instinvar XOTclClassInfoInstinvarMethod {
  {-argName "class"  -required 1 -type class}
}

infoClassMethod instmixin XOTclClassInfoInstmixinMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "-guards"}
  {-argName "pattern" -type objpattern}
}

infoClassMethod instmixinguard XOTclClassInfoInstmixinguardMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "mixin" -required 1}
}

infoClassMethod instmixinof XOTclClassInfoInstmixinofMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}

infoClassMethod instnonposargs XOTclClassInfoInstnonposargsMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "methodName" -required 1}
}

infoClassMethod instparametercmd XOTclClassInfoInstparametercmdMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "pattern"}
}

infoClassMethod instpost XOTclClassInfoInstpostMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "methodName" -required 1}
}

infoClassMethod instpre XOTclClassInfoInstpreMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "methodName" -required 1}
}

infoClassMethod instprocs XOTclClassInfoInstprocsMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "pattern"}
}

infoClassMethod mixinof XOTclClassInfoMixinofMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}

infoClassMethod parameter XOTclClassInfoParameterMethod {
  {-argName "class"  -required 1 -type class}
}

infoClassMethod slots XOTclClassInfoSlotsMethod {
  {-argName "class"  -required 1 -type class}
}

infoClassMethod subclass XOTclClassInfoSubclassMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern" -type objpattern}
}

infoClassMethod superclass XOTclClassInfoSuperclassMethod {
  {-argName "class"  -required 1 -type class}
  {-argName "-closure"}
  {-argName "pattern"}
}







genifds