# 
# C-Code generator to generate stubs to handle all objv-parsing from
# an simple interface definition language. This guarantees consistent
# handling of input argument types, consistent error messages in case
# of failures and eases documentation.
#
# Gustaf Neumann,                                fecit in June 2009
#

set ::converter ""
set ::objCmdProc "(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv \[\]);"

proc convertername {type argname} {
  #return [string totitle [string map [list | _] $type]]
  return [string totitle $argname]
}

proc createconverter {type argname} {
  set name [convertername $type $argname]
  if {[info exists ::created($name)]} {
    return ""
  }
  set domain [split $type |]
  set opts "static CONST char *opts\[\] = {\"[join $domain {", "}]\", NULL};"
  set enums [list]
  foreach d $domain {lappend enums $argname[string totitle [string map [list - _] $d]]Idx}
  subst {
static int convertTo${name}(Tcl_Interp *interp, Tcl_Obj *objPtr, ClientData *clientData) {
  $opts
  return Tcl_GetIndexFromObj(interp, objPtr, opts, "$argname", 0, (int *)clientData);
}
enum ${argname}Idx {[join $enums {, }]};
  }
}

proc genifd {argDefinitions} {
  set l [list]
  foreach argDefinition $argDefinitions {
    array set "" $argDefinition
    switch $(-type) {
      "" {set type NULL}
      default {set type $(-type)}
    }
    switch -glob $type {
      "NULL"       {set converter String}
      "boolean"    {set converter Boolean}
      "class"      {set converter Class}
      "object"     {set converter Object}
      "tclobj"     {set converter Tclobj}
      "args"       {set converter Nothing}
      "allargs"    {set converter Nothing}
      "objpattern" {set converter Objpattern}
      *|* {  
        set converter [convertername $type $(-argName)]
        append ::converter [createconverter $type $(-argName)]
        set (-argName) $type
      }
    }
    lappend l "{\"$(-argName)\", $(-required), $(-nrargs), convertTo$converter}"
  }
  join $l ",\n  "
}

proc gencall {fn argDefinitions clientData cDefsVar ifDefVar arglistVar preVar postVar introVar} {
  upvar $cDefsVar cDefs $ifDefVar ifDef $arglistVar arglist $preVar pre $postVar post \
      $introVar intro 
  set c [list]
  set i 0
  set pre ""; set post ""
  set intro "  parseContext pc;\n"

  switch $clientData {
    class {
      set a  [list cl]
      set if [list "XOTclClass *cl"]
      append intro \
          "  XOTclClass *cl =  XOTclObjectToClass(clientData);" \n \
          {  if (!cl) return XOTclObjErrType(interp, objv[0], "Class");}
    }
    object {
      set a  [list obj]
      set if [list "XOTclObject *obj"]
      append intro \
          "  XOTclObject *obj =  (XOTclObject *)clientData;" \n \
          {  if (!obj) return XOTclObjErrType(interp, objv[0], "Object");}
    }
    ""    {
      set a [list]
      set if [list]
      array set cd {arglist "" ifDefs ""}
    }
  }
  foreach argDefinition $argDefinitions {
    array set "" $argDefinition
    set ifSet 0
    set cVar 1
    set (-argName) [string map [list - _] $(-argName)]
    if {[regexp {^_(.*)$} $(-argName) _ switchName]} {
      set varName with[string totitle $switchName]
      set calledArg $varName
      set type "int "
      if {$(-nrargs) == 1} {
        switch $(-type) {
          ""           {set type "char *"}
          "class"      {set type "XOTclClass *"}
          "object"     {set type "XOTclObject *"}
          "tclobj"     {set type "Tcl_Obj *"}
          default      {error "type '$(-type)' not allowed for parameter"}
        }
      }
    } else {
      set varName $(-argName)
      set calledArg $varName
      switch -glob $(-type) {
        ""           {set type "char *"}
        "boolean"    {set type "int "}
        "class"      {set type "XOTclClass *"}
        "object"     {set type "XOTclObject *"}
        "tclobj"     {set type "Tcl_Obj *"}
        "args"       {
          set type "int "
          set calledArg "objc-pc.lastobjc, objv+pc.lastobjc"
          lappend if "int nobjc" "Tcl_Obj *CONST nobjv\[\]"
          set ifSet 1
          set cVar 0
        }
        "allargs"       {
          set type "int "
          set calledArg "objc, objv"
          lappend if "int objc" "Tcl_Obj *CONST objv\[\]"
          set ifSet 1
          set cVar 0
        }
        "objpattern" {
          set type "Tcl_Obj *"
          lappend c "char *${varName}String = NULL;" "XOTclObject *${varName}Obj = NULL;"
          set calledArg "${varName}String, ${varName}Obj"
          lappend if "char *${varName}String" "XOTclObject *${varName}Obj"
          set ifSet 1
          append pre [subst -nocommands {
    if (getMatchObject(interp, ${varName},  objv[$i], &${varName}Obj, &${varName}String) == -1) {
      if (${varName}) {
        DECR_REF_COUNT(${varName});
      }
      return TCL_OK;
    }
          }]
	  append post [subst -nocommands {
    if (${varName}) {
      DECR_REF_COUNT(${varName});
    }
         }]
	  # end of obj pattern
        }
        *|* {set type "int "}
        default  {
          error "type '$(-type)' not allowed for argument"
        }
      }
    }
    if {!$ifSet} {lappend if "$type$varName"}
    if {$cVar} {lappend c [subst -nocommands {$type$varName = ($type)pc.clientData[$i];}]}
    lappend a $calledArg
    incr i
  }
  set ifDef   [join $if ", "]
  set cDefs   [join $c "\n    "]
  set arglist [join $a ", "]
}


proc genstubs {} {
  set stubDecls ""
  set decls ""
  set enums [list]
  set ifds [list]
  foreach key [lsort [array names ::definitions]] {
    array set d $::definitions($key)
    append stubDecls "static int $d(stub)$::objCmdProc\n"
    lappend enums $d(idx)
    lappend ifds "{\"$::ns($d(methodType))::$d(methodName)\", $d(stub), [llength $d(argDefinitions)], {\n  [genifd $d(argDefinitions)]}\n}"

    gencall $d(stub) $d(argDefinitions) $d(clientData) cDefs ifDef arglist pre post intro 
    append decls "static int $d(implementation)(Tcl_Interp *interp, $ifDef);\n"
    if {$post ne ""} {
      append cDefs "\n    int returnCode;"
      set call "returnCode = $d(implementation)(interp, $arglist);"
      set post [string trimright $post]
      append post "\n    return returnCode;"
    } else {
      set call "return $d(implementation)(interp, $arglist);"
    }
    append fns [subst -nocommands {
static int
$d(stub)(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
$intro
  if (parseObjv(interp, objc, objv, objv[0], 
		method_definitions[$d(idx)].ifd, 
		method_definitions[$d(idx)].ifdSize, 
		&pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    $cDefs
$pre
    parseContextRelease(&pc);
    $call
$post
  }
}
  }]
  }

  puts $::converter
  puts {
typedef struct {
  char *methodName;
  Tcl_ObjCmdProc *proc;
  /*CONST interfaceDefinition ifd;*/
  int ifdSize;
  argDefinition ifd[10];
} methodDefinition;

static int parseObjv(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], Tcl_Obj *procName,
		     argDefinition CONST *ifdPtr, int ifdSize, parseContext *pc);

static int getMatchObject(Tcl_Interp *interp, Tcl_Obj *patternObj, Tcl_Obj *origObj,
			  XOTclObject **matchObject, char **pattern);

/* just to define the symbol */
static methodDefinition method_definitions[];
  }

  set namespaces [list]
  foreach {key value} [array get ::ns] { 
     # no need to create the ::xotcl namespace
     if {$value eq "::xotcl"} continue
     lappend namespaces "\"$value\"" 
  }
  set namespaceString [join $namespaces ",\n  "]
  puts "char *method_command_namespace_names\[\] = {\n  $namespaceString\n};"
  puts $stubDecls
  puts $decls
  set enumString [join $enums ",\n "]
  puts "enum {\n $enumString\n} XOTclMethods;\n"
  puts $fns
  set definitionString [join $ifds ",\n"]
  puts "static methodDefinition method_definitions\[\] = \{\n$definitionString\n\};\n"
}

proc methodDefinition {methodName methodType implementation argDefinitions} {
  set d(methodName) $methodName
  set d(implementation) $implementation
  set d(stub) ${implementation}Stub
  set d(idx) ${implementation}Idx
  set d(methodType) $methodType
  switch $methodType {
    classMethod  {set d(clientData) class}
    objectMethod {set d(clientData) object}
    default      {set d(clientData) ""}
  }
  set completed [list]
  foreach argDefinition $argDefinitions {
    array set "" {-required 0 -nrargs 0 -type ""}
    array set "" $argDefinition
    lappend completed [array get ""]
  }
  set d(argDefinitions) $completed
  set ::definitions($d(methodType)-$d(implementation)-$d(methodName)) [array get d]
}

proc infoClassMethod {methodName implementation argDefinitions} {
  methodDefinition $methodName infoClassMethod $implementation $argDefinitions
}
proc infoObjectMethod {methodName implementation argDefinitions} {
  methodDefinition $methodName infoObjectMethod $implementation $argDefinitions
}
proc checkMethod {methodName implementation argDefinitions} {
  methodDefinition type=$methodName checkMethod $implementation $argDefinitions
}
proc classMethod {methodName implementation argDefinitions} {
  methodDefinition $methodName classMethod $implementation $argDefinitions
}
proc objectMethod {methodName implementation argDefinitions} {
  methodDefinition $methodName objectMethod $implementation $argDefinitions
}
proc xotclCmd {methodName implementation argDefinitions} {
  methodDefinition $methodName xotclCmd $implementation $argDefinitions
}

source [file dirname [info script]]/gentclAPI.decls

genstubs
puts stderr "[array size ::definitions] parsing stubs generated"
