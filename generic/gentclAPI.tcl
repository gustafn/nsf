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
  return [string totitle [string trimleft $argname -]]
}

proc createconverter {type argname} {
  set name [convertername $type $argname]
  if {[info exists ::created($name)]} {
    return ""
  }
  set ::created($name) 1
  set domain [split $type |]
  set opts "static CONST char *opts\[\] = {\"[join $domain {", "}]\", NULL};"
  set enums [list ${name}NULL]
  foreach d $domain {lappend enums $name[string totitle [string map [list - _] $d]]Idx}
  subst {
static int convertTo${name}(Tcl_Interp *interp, Tcl_Obj *objPtr, XOTclParam CONST *pPtr, 
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  $opts
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "$argname", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
enum ${name}Idx {[join $enums {, }]};
  }
}

proc genifd {parameterDefinitions} {
  set l [list]
  foreach parameterDefinition $parameterDefinitions {
    array set "" $parameterDefinition
    switch $(-type) {
      "" {set type NULL}
      default {set type $(-type)}
    }
    set argName $(-argName)
    switch -glob $type {
      "NULL"       {set converter String}
      "boolean"    {set converter Boolean}
      "switch"     {set converter Boolean}
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
    # this does not work, since initializer element is not constant.
#     if {[info exists (-default)]} {
#       puts stderr "default of $argName = '$(-default)'"
#       set default ", Tcl_NewStringObj(\"$(-default)\",-1)"
#     } else {
#       set default ""
#     }
    lappend l "{\"$argName\", $(-required), $(-nrargs), convertTo$converter}"
  }
  join $l ",\n  "
}

proc gencall {fn parameterDefinitions clientData cDefsVar ifDefVar arglistVar preVar postVar introVar} {
  upvar $cDefsVar cDefs $ifDefVar ifDef $arglistVar arglist $preVar pre $postVar post \
      $introVar intro 
  set c [list]
  set i 0
  set pre ""; set post ""
  set intro ""

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
  foreach parameterDefinition $parameterDefinitions {
    array set "" $parameterDefinition
    set ifSet 0
    set cVar 1
    set (-argName) [string map [list - _] $(-argName)]
    if {[regexp {^_(.*)$} $(-argName) _ switchName]} {
      set varName with[string totitle $switchName]
      set calledArg $varName
      set type "int "
      if {$(-nrargs) == 1} {
        switch -glob $(-type) {
          ""           {set type "char *"}
          "class"      {set type "XOTclClass *"}
          "object"     {set type "XOTclObject *"}
          "tclobj"     {set type "Tcl_Obj *"}
          "*|*"        {set type "int "}
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
    if {$cVar} {
      if {$type eq "int "} {
        lappend c [subst -nocommands {$type$varName = ($type)PTR2INT(pc.clientData[$i]);}]
      } else {
        lappend c [subst -nocommands {$type$varName = ($type)pc.clientData[$i];}]
      }
    }
    lappend a $calledArg
    incr i
  }
  set ifDef   [join $if ", "]
  set cDefs   [join $c "\n    "]
  set arglist [join $a ", "]
}

proc genStub {stub intro obj idx cDefs pre call post} {
  return [subst -nocommands {
static int
${stub}(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  parseContext pc;
$intro
  if (ArgumentParse(interp, objc, objv, $obj, objv[0], 
                     method_definitions[$idx].paramDefs, 
                     method_definitions[$idx].nrParameters, 
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
}]}

proc genSimpleStub {stub intro idx cDefs pre call post} {
  return [subst -nocommands {
static int
${stub}(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
$intro
    $cDefs
$pre
    $call
$post
}
}]}

proc implArgList {implementation prefix arglist} {
  if {$arglist ne ""} {
    return "${implementation}(${prefix}interp, $arglist)"
  }
  return "${implementation}(${prefix}interp)"
}

proc genstubs {} {
  set stubDecls ""
  set decls ""
  set enums [list]
  set ifds [list]
  foreach key [lsort [array names ::definitions]] {
    array set d $::definitions($key)
    lappend enums $d(idx)
    set nrArgs [llength $d(parameterDefinitions)]
    set stubDecl "static int $d(stub)$::objCmdProc\n"
    set ifd "{\"$::ns($d(methodType))::$d(methodName)\", $d(stub), $nrArgs, {\n  [genifd $d(parameterDefinitions)]}\n}"
    
    gencall $d(stub) $d(parameterDefinitions) $d(clientData) cDefs ifDef arglist pre post intro 
    append decls "static int [implArgList $d(implementation) {Tcl_Interp *} $ifDef];\n"
    if {$post ne ""} {
      append cDefs "\n    int returnCode;"
      set call "returnCode = [implArgList $d(implementation) {} $arglist];"
      set post [string trimright $post]
      append post "\n    return returnCode;"
    } else {
      set call "return [implArgList $d(implementation) {} $arglist];"
    }
    
    #if {$nrArgs == 1} { puts stderr "$d(stub) => '$arglist'" }
    if {$nrArgs == 1 && $arglist eq "objc, objv"} {
      # TODO we would not need to generate a stub at all.... 
      #set ifd "{\"$::ns($d(methodType))::$d(methodName)\", $d(implementation), $nrArgs, {\n  [genifd $d(parameterDefinitions)]}\n}"
      #set stubDecl "static int $d(implementation)$::objCmdProc\n"
      append fns [genSimpleStub $d(stub) $intro $d(idx) $cDefs $pre $call $post]
    } elseif {$nrArgs == 1 && $arglist eq "obj, objc, objv"} {
      # no need to call objv parser
      #puts stderr "$d(stub) => '$arglist'"
      append fns [genSimpleStub $d(stub) $intro $d(idx) $cDefs $pre $call $post]
    } else {
      switch $d(methodType) {
        objectMethod {set obj "obj"}
        classMethod {set obj "(XOTclObject *) cl"}
        default {set obj "NULL"}
      }
      append fns [genStub $d(stub) $intro $obj $d(idx) $cDefs $pre $call $post]
    }
    lappend ifds $ifd
    append stubDecls $stubDecl
  }
  
  puts $::converter
  puts {
typedef struct {
  char *methodName;
  Tcl_ObjCmdProc *proc;
  int nrParameters;
  XOTclParam paramDefs[12];
} methodDefinition;

static int ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], 
                         XOTclObject *obj, Tcl_Obj *procName,
                         XOTclParam CONST *paramPtr, int nrParameters, parseContext *pc);

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
  puts "static char *method_command_namespace_names\[\] = {\n  $namespaceString\n};"
  puts $stubDecls
  puts $decls
  set enumString [join $enums ",\n "]
  puts "enum {\n $enumString\n} XOTclMethods;\n"
  puts $fns
  set definitionString [join $ifds ",\n"]
puts "static methodDefinition method_definitions\[\] = \{\n$definitionString,\{NULL\}\n\};\n"
}

proc methodDefinition {methodName methodType implementation parameterDefinitions} {
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
  foreach parameterDefinition $parameterDefinitions {
    array set "" {-required 0 -nrargs 0 -type ""}
    array set "" $parameterDefinition
    lappend completed [array get ""]
  }
  set d(parameterDefinitions) $completed
  set ::definitions($d(methodType)-$d(implementation)-$d(methodName)) [array get d]
}

proc infoClassMethod {methodName implementation parameterDefinitions} {
  methodDefinition $methodName infoClassMethod $implementation $parameterDefinitions
}
proc infoObjectMethod {methodName implementation parameterDefinitions} {
  methodDefinition $methodName infoObjectMethod $implementation $parameterDefinitions
}
proc checkMethod {methodName implementation parameterDefinitions} {
  methodDefinition type=$methodName checkMethod $implementation $parameterDefinitions
}
proc classMethod {methodName implementation parameterDefinitions} {
  methodDefinition $methodName classMethod $implementation $parameterDefinitions
}
proc objectMethod {methodName implementation parameterDefinitions} {
  methodDefinition $methodName objectMethod $implementation $parameterDefinitions
}
proc xotclCmd {methodName implementation parameterDefinitions} {
  methodDefinition $methodName xotclCmd $implementation $parameterDefinitions
}

source [file dirname [info script]]/gentclAPI.decls

genstubs
puts stderr "[array size ::definitions] parsing stubs generated"
