#!/usr/bin/env tclsh
# -*- Tcl -*-
#
# C-Code generator to generate stubs to handle all objv-parsing from
# an simple interface definition language. This guarantees consistent
# handling of input argument types, consistent error messages in case
# of failures and eases documentation.
#
#   Copyright (C) 2009-2014 Gustaf Neumann
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

set ::converter ""
set ::objCmdProc "(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST* objv)"

proc convertername {type typename} {
  if {[info exists ::registeredConverter($type)]} {
    set name $::registeredConverter($type)
  } else {
    set name [string totitle [string trimleft $typename -]]
    set ::registeredConverter($type) $name
  }
  return $name
}

proc createconverter {type typename} {
  set name [convertername $type $typename]
  if {[info exists ::createdConverter($name)]} {
    return ""
  }
  set domain [split $type |]
  set opts "static const char *opts\[\] = {\"[join $domain {", "}]\", NULL};"
  set ::createdConverter($name) "ConvertTo${name}, \"$type\""
  set enums [list ${name}NULL]
  foreach d $domain {lappend enums $name[string totitle [string map [list - _] $d]]Idx}
  subst {
typedef enum {[join $enums {, }]} ${name}Idx_t;

static int ConvertTo${name}(Tcl_Interp *interp, Tcl_Obj *objPtr, Nsf_Param const *pPtr,
			    ClientData *clientData, Tcl_Obj **outObjPtr) {
  int index, result;
  $opts
  (void)pPtr;
  result = Tcl_GetIndexFromObj(interp, objPtr, opts, "$typename", 0, &index);
  *clientData = (ClientData) INT2PTR(index + 1);
  *outObjPtr = objPtr;
  return result;
}
  }
}

proc addFlags {flags_var new} {
  upvar $flags_var flags
  if {$flags eq "0"} {set flags $new} {append flags "|$new"}
}

proc genifd {parameterDefinitions} {
  #puts stderr $parameterDefinitions
  set l [list]
  foreach parameterDefinition $parameterDefinitions {
    array unset ""
    array set "" {-flags 0}
    array set "" $parameterDefinition
    switch $(-type) {
      "" {set type NULL}
      default {set type $(-type)}
    }
    set flags $(-flags)
    if {$(-required)} {addFlags flags "NSF_ARG_REQUIRED"}
    set argName $(-argName)
    switch -glob $type {
      "NULL"       {set converter String}
      "boolean"    {set converter Boolean}
      "switch"     {set converter Boolean}
      "int"        {set converter Integer}
      "int32"      {set converter Int32}
      "class"      {set converter Class}
      "object"     {set converter Object}
      "tclobj"     {set converter Tclobj}
      "args"       {set converter Nothing}
      "allargs"    -
      "virtualobjectargs" -
      "virtualclassargs" {set converter Nothing}
      "objpattern" {set converter Objpattern}
      *|* {
	if {![info exists (-typeName)]} {set (-typeName) $(-argName)}
        set converter [convertername $type $(-typeName)]
        append ::converter [createconverter $type $(-typeName)]
        addFlags flags "NSF_ARG_IS_ENUMERATION"
      }
      default {
	if {[info exists ::ptrConverter($type)]} {
	  set converter Pointer
	} else {
	  error "unknown type $type"
	}
      }
    }
    if {$converter in {Tclobj Integer Int32 Boolean String Class Object Pointer}} {
      set conv Nsf_ConvertTo_$converter
    } else {
      set conv ConvertTo$converter
    }
    switch -glob -- $(-type) {
      "*|*" -
      "tclobj" -
      "args" -
      "" {set typeString NULL}
      default {
	set typeString "\"$(-type)\""
      }
    }
    lappend l "{\"$argName\", $flags, $(-nrargs), $conv, NULL,NULL,$typeString,NULL,NULL,NULL,NULL,NULL}"
  }
  if {[llength $l] == 0} {
    return "{NULL, 0, 0, NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}"
  } else {
    return [join $l ",\n  "]
  }
}

proc varName {type name} {
  if {$type eq "tclobj" && $name ne "obj"} {
    set varName ${name}Obj
  } elseif {$type eq "object" && $name ne "object"} {
    regsub -all _object $name "" name
    set varName ${name}Object
  } elseif {$type eq "class" && $name ne "class"} {
    regsub -all _class $name "" name
    set varName ${name}Class
  } else {
    set varName $name
  }
  return $varName
}


proc gencall {methodName fn parameterDefinitions clientData
	      cDefsVar ifDefVar arglistVar preVar postVar introVar nnVar cleanupVar
	    } {
  upvar $cDefsVar cDefs $ifDefVar ifDef $arglistVar arglist $preVar pre $postVar post \
      $introVar intro $nnVar nn $cleanupVar cleanup
  set c [list]
  set i 0
  set pre ""; set post ""; set cleanup ""
  set intro ""

  switch $clientData {
    class {
      set a  [list class]
      set if [list "NsfClass *class"]
      set argNum 3
      append intro \
          "  NsfClass *class;" \n\n \
          "  assert(objc > 0);" \n\n \
          "  class = NsfObjectToClass(clientData);" \n \
          "  if (unlikely(class == NULL)) {" \n \
          "      return NsfDispatchClientDataError(interp, clientData, \"class\", ObjStr(objv\[0\]));" \n \
          "  }" \n
    }
    object {
      set a  [list object]
      set if [list "NsfObject *object"]
      set argNum 3
      append intro \
          "  NsfObject *object;" \n\n \
          "  assert(objc > 0);" \n\n \
          "  object = (NsfObject *)clientData;"
    }
    ""    {
      append intro "  (void)clientData;\n"
      set a [list]
      set if [list]
      set argNum 2
      array set cd {arglist "" ifDefs ""}
    }
  }
  foreach parameterDefinition $parameterDefinitions {
    array set "" $parameterDefinition
    set ifSet 0
    set cVar 1
    set (-argName) [string map [list - _] $(-argName)]
    if {[regexp {^_(.*)$} $(-argName) _ switchName]} {
      #
      # non positional args
      #
      set varName [varName $(-type) $switchName]
      if {$varName eq $switchName} {
        set varName with[string totitle $switchName]
      }
      set calledArg $varName
      set type "int "
      if {$(-nrargs) == 1} {
        switch -glob $(-type) {
          ""           {set type "const char *"}
          "class"      {set type "NsfClass *"}
          "object"     {set type "NsfObject *"}
          "tclobj"     {set type "Tcl_Obj *"}
          "int"        {set type "Tcl_Obj *"}
          "int32"      {set type "int "}
          "boolean"    {set type "int "}
          "*|*"        {
            if {![info exists (-typeName)]} {set (-typeName) $(-argName)}
            set type "[convertername $(-type) $(-typeName)]Idx_t "
            #puts stderr "nonpos: (-typeName) <$(-typeName)> (-type) <$(-type)>    ==> type=<$type>"
          }
          default      {error "type '$(-type)' not allowed for parameter"}
        }
      }
    } else {
      #
      # positionals
      #
      set varName [varName $(-type) $(-argName)]
      set calledArg $varName
      switch -glob $(-type) {
        ""           {set type "const char *"}
        "boolean"    {set type "int "}
        "int32"      {set type "int "}
        "class"      {set type "NsfClass *"}
        "object"     {set type "NsfObject *"}
        "tclobj"     {set type "Tcl_Obj *"}
        "virtualobjectargs" -
        "virtualclassargs" -
        "args"       {
          set type "int "
          set calledArg "objc - pc.lastObjc, objv + pc.lastObjc"
          lappend if "int trailingObjc" "Tcl_Obj *CONST* trailingObjv"
          set ifSet 1
          set cVar 0
        }
        "allargs" {
          set type "int "
          set calledArg "objc, objv"
          lappend if "int objc" "Tcl_Obj *CONST* objv"
          set ifSet 1
          set cVar 0
        }
        "objpattern" {
          set type "Tcl_Obj *"
          lappend c "const char *${varName}String = NULL;" "NsfObject *${varName}Object = NULL;"
          set calledArg "${varName}String, ${varName}Object"
          lappend if "const char *${varName}String" "NsfObject *${varName}Object"
          set ifSet 1
          append pre [subst -nocommands {
    if (GetMatchObject(interp, ${varName}, objc>$i ? objv[$i] : NULL, &${varName}Object, &${varName}String) == -1) {
      if (${varName}) {
        DECR_REF_COUNT2("patternObj", ${varName});
      }
      return TCL_OK;
    }
          }]
	  append post [subst -nocommands {
    if (${varName}) {
      DECR_REF_COUNT2("patternObj", ${varName});
    }
         }]
          set cleanup [subst -nocommands {$type$varName = ($type)pc.clientData[$i];}]
          append cleanup \n$post
	  # end of obj pattern
        }
        *|* {
          if {![info exists (-typeName)]} {set (-typeName) $(-argName)}
          set type "[convertername $(-type) $(-typeName)]Idx_t "
        }
        default  {
	  if {[info exists ::ptrConverter($(-type))]} {
	    set type "$(-type) *"
	    set varName "${varName}Ptr"
	    set calledArg $varName
	    if {$(-withObj)} {
	      append calledArg [subst -nocommands {,pc.objv[$i]}]
	      lappend if "$type$varName" "Tcl_Obj *$(-argName)Obj"
	      set ifSet 1
	    }
	  } else {
	    error "type '$(-type)' not allowed for argument"
	  }
        }
      }
    }

    if {[string match {*[*]*} $type] && $(-required)} {
      append nn " NSF_nonnull($argNum)"
    }
    if {!$ifSet} {lappend if "$type$varName"}
    if {$cVar} {
      if {$type eq "int " || [string match "*Idx " $type]} {
        lappend c [subst -nocommands {$type$varName = ($type)PTR2INT(pc.clientData[$i]);}]
      } else {
        lappend c [subst -nocommands {$type$varName = ($type)pc.clientData[$i];}]
      }
    }
    lappend a $calledArg
    incr i
    incr argNum
  }
  set ifDef   [join $if ", "]
  set cDefs   [join $c "\n    "]
  set arglist [join $a ", "]
}

proc genStub {stub intro obj idx cDefs pre call post cleanup} {
  # Tiny optimization for calls without parameters;
  # ParseContextExtendObjv() is just called for procs, so no need to
  # free non-static objvs. Actually, the api for c-methods does
  # not allow to generate structures which have to be freed.
  # we assert this in the code.
  if {$cDefs ne ""} {
    set releasePC "ParseContextRelease(&pc);"
    set releasePC "assert(pc.status == 0);"
  } else {
    set releasePC ""
  }
  return [subst -nocommands {
static int
${stub}(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST* objv) {
  ParseContext pc;
$intro
  if (likely(ArgumentParse(interp, objc, objv, $obj, objv[0],
                     method_definitions[$idx].paramDefs,
                     method_definitions[$idx].nrParameters, 0, NSF_ARGPARSE_BUILTIN,
                     &pc) == TCL_OK)) {
    $cDefs
$pre
    $releasePC
    $call
$post
  } else {
    $cleanup
    return TCL_ERROR;
  }
}
}]}

proc genSimpleStub {stub intro idx cDefs pre call post cleanup} {
  if {$cleanup ne ""} {error "$stub cleanup code '$cleanup' must be empty"}
  return [subst -nocommands {
static int
${stub}(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST* objv) {
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
    set nrParams [llength $d(parameterDefinitions)]
    set nn ""

    gencall $d(methodName) $d(stub) $d(parameterDefinitions) $d(clientData) \
	cDefs ifDef arglist pre post intro nn cleanup

    #
    # Check, if spec tells us to pass the original "objv[0]" as an
    # argument. For unstacked entries this is the only way to
    # determine the name, under which the cmd was called.
    #
    if {[dict get $d(options) -objv0]} {
      append ifDef ", Tcl_Obj *objv0"
      append arglist ", objv\[0\]"
    }

    if {[dict get $::definitions($key) clientData] ne ""} {
      set stubNN "NSF_nonnull(1) "
      set NN " NSF_nonnull(2)"
      regsub \n\n $intro "\n\n  NSF_nonnull_assert(clientData != NULL);\n" intro
    } else {
      set stubNN ""
      set NN ""
    }
    set stubDecl "static int $d(stub)$::objCmdProc\n  ${stubNN}NSF_nonnull(2) NSF_nonnull(4);\n"
    set ifd "{\"$d(ns)::$d(methodName)\", $d(stub), $nrParams, {\n  [genifd $d(parameterDefinitions)]}\n}"

    append decls "static int [implArgList $d(implementation) {Tcl_Interp *} $ifDef]\n  NSF_nonnull(1)${NN}${nn};\n"
    if {$post ne ""} {
      append cDefs "\n    int returnCode;"
      set call "returnCode = [implArgList $d(implementation) {} $arglist];"
      set post [string trimright $post]
      append post "\n    return returnCode;"
    } else {
      set call "return [implArgList $d(implementation) {} $arglist];"
    }

    #if {$nrParams == 1} { puts stderr "$d(stub) => '$arglist' cDefs=$cDefs ifd=$ifDef" }
    if {$nrParams == 1 && $arglist eq "objc, objv"} {
      # TODO we would not need to generate a stub at all....
      #set ifd "{\"$d(ns)::$d(methodName)\", $d(implementation), $nrParams, {\n  [genifd $d(parameterDefinitions)]}\n}"
      append fns [genSimpleStub $d(stub) $intro $d(idx) $cDefs $pre $call $post $cleanup]
    } elseif {$nrParams == 1 && $arglist eq "obj, objc, objv"} {
      # no need to call objv parser
      #puts stderr "$d(stub) => '$arglist'"
      append fns [genSimpleStub $d(stub) $intro $d(idx) $cDefs $pre $call $post $cleanup]
  } elseif {$nrParams == 0} {
    append pre [subst -nocommands {
      if (unlikely(objc != 1)) {
	return NsfArgumentError(interp, "too many arguments:",
			     method_definitions[$d(idx)].paramDefs,
			     NULL, objv[0]);
      }
    }]
    append fns [genSimpleStub $d(stub) $intro $d(idx) $cDefs $pre $call $post $cleanup]
  } elseif {$nrParams == 1 && [string match "Tcl_Obj *" $cDefs]} {

    array set defs [list -required 0]
    array set defs [lindex $d(parameterDefinitions) 0]

    if {$defs(-required)} {
      set op "objc != 2"
      set newArg {objv[1]}
    } else {
      set op "objc < 1 || objc > 2"
      set newArg {objc == 2 ? objv[1] : NULL}
    }
    append pre [subst -nocommands {
      if ($op) {
	return NsfArgumentError(interp, "wrong # of arguments:",
			     method_definitions[$d(idx)].paramDefs,
			     NULL, objv[0]);
      }
    }]

    if {[regexp {^(.*),(.*)$} $arglist _ arg1]} {
      set newArglist "$arg1, $newArg"
    } else {
      set newArglist $newArg
    }
    regsub ", $arglist\\)" $call ", $newArglist\)" call

    append fns [genSimpleStub $d(stub) $intro $d(idx) "" $pre $call $post $cleanup]
  } else {
      switch $d(methodType) {
        objectMethod {set obj "object"}
        classMethod {set obj "(NsfObject *) class"}
        default {set obj "NULL"}
      }
      append fns [genStub $d(stub) $intro $obj $d(idx) $cDefs $pre $call $post $cleanup]
    }
    lappend ifds $ifd
    append stubDecls $stubDecl
  }

  puts $::converter

  set entries [list]
  foreach c [array names ::createdConverter] {lappend entries "\{$::createdConverter($c)\}"}
  if {[llength $entries]>0} {
    puts [subst {
      static Nsf_EnumeratorConverterEntry enumeratorConverterEntries\[\] = {
  [join $entries ",\n  "],
  {NULL, NULL}
};
    }]
  }
  set nrIfds [expr {[llength $ifds]+1}]
  puts [subst -nocommands {
/* just to define the symbol */
static Nsf_methodDefinition method_definitions[$nrIfds];
  }]

  set namespaces [list]
  foreach {key value} [array get ::ns] {
     # no need to create the ::nsf namespace
     if {$value eq "::nsf"} continue
     lappend namespaces "\"$value\""
  }
  set namespaceString [join $namespaces ",\n  "]
  puts "static const char *method_command_namespace_names\[\] = {\n  $namespaceString\n};"
  puts $stubDecls
  puts $decls
  set enumString [join $enums ",\n "]
  puts "enum {\n $enumString\n} NsfMethods;\n"
  puts $fns
  set definitionString [join $ifds ",\n"]
  set terminator {NULL, NULL, 0, {{NULL, 0, 0, NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}}
  puts "static Nsf_methodDefinition method_definitions\[$nrIfds\] = \{\n$definitionString,\n{$terminator}\n\};\n"
}

proc methodDefinition {methodName methodType implementation parameterDefinitions options} {
  array set opts [list -ns $::ns($methodType) -nxdoc 0 -objv0 0]
  array set opts $options
  set d(methodName) $methodName
  set d(implementation) $implementation
  set d(stub) ${implementation}Stub
  set d(idx) ${implementation}Idx
  set d(methodType) $methodType
  set d(ns) $opts(-ns)
  set d(options) [array get opts]
  switch $methodType {
    classMethod  {set d(clientData) class}
    objectMethod {set d(clientData) object}
    default      {set d(clientData) ""}
  }
  set completed [list]
  foreach parameterDefinition $parameterDefinitions {
    array unset ""
    array set "" {-required 0 -nrargs 1 -type "" -withObj 0}
    array set "" $parameterDefinition
    lappend completed [array get ""]
  }
  set d(parameterDefinitions) $completed
  set ::definitions($d(methodType)-$d(implementation)-$d(methodName)) [array get d]
  # puts $::nxdocIndex [list set ::nxdoc::include($d(ns)::$d(methodName)) $opts(-nxdoc)]
}

proc checkMethod {methodName implementation parameterDefinitions {options ""}} {
  methodDefinition type=$methodName checkMethod $implementation $parameterDefinitions $options
}
proc classMethod {methodName implementation parameterDefinitions {options ""}} {
  methodDefinition $methodName classMethod $implementation $parameterDefinitions $options
}
proc objectMethod {methodName implementation parameterDefinitions {options ""}} {
  methodDefinition $methodName objectMethod $implementation $parameterDefinitions $options
}
proc objectInfoMethod {methodName implementation parameterDefinitions {options ""}} {
  lappend options -ns $::ns(objectInfoMethod)
  methodDefinition $methodName objectMethod $implementation $parameterDefinitions $options
}
proc classInfoMethod {methodName implementation parameterDefinitions {options ""}} {
  lappend options -ns $::ns(classInfoMethod)
  methodDefinition $methodName classMethod $implementation $parameterDefinitions $options
}
proc cmd {methodName implementation parameterDefinitions {options ""}} {
  methodDefinition $methodName cmd $implementation $parameterDefinitions $options
}

if {[llength $argv] == 1} {set decls $argv} {set decls generic/gentclAPI.decls}
# set ::nxdocIndex [open [file root $decls].nxdocindex w]
source $decls
# close $::nxdocIndex

puts {
/*
 * This source code file was generated by the C-code generator gentclAPI.tcl,
 * part of the Next Scripting Framework.
 */

#if defined(USE_NSF_STUBS)
int Nsf_ConvertTo_Boolean(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr) {
  return Nsf_ConvertToBoolean(interp, objPtr, pPtr, clientData, outObjPtr);
}
int Nsf_ConvertTo_Class(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr) {
  return Nsf_ConvertToClass(interp, objPtr, pPtr, clientData, outObjPtr);
}
int Nsf_ConvertTo_Int32(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr) {
  return Nsf_ConvertToInt32(interp, objPtr, pPtr, clientData, outObjPtr);
}
int Nsf_ConvertTo_Integer(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr) {
  return Nsf_ConvertToInteger(interp, objPtr, pPtr, clientData, outObjPtr);
}
int Nsf_ConvertTo_Object(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr) {
  return Nsf_ConvertToObject(interp, objPtr, pPtr, clientData, outObjPtr);
}
int Nsf_ConvertTo_Pointer(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr) {
  return Nsf_ConvertToPointer(interp, objPtr, pPtr, clientData, outObjPtr);
}
int Nsf_ConvertTo_String(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr) {
  return Nsf_ConvertToString(interp, objPtr, pPtr, clientData, outObjPtr);
}
int Nsf_ConvertTo_Tclobj(Tcl_Interp *interp, Tcl_Obj *objPtr,  Nsf_Param const *pPtr,
			  ClientData *clientData, Tcl_Obj **outObjPtr) {
  return Nsf_ConvertToTclobj(interp, objPtr, pPtr, clientData, outObjPtr);
}
#else
# define Nsf_ConvertTo_Boolean Nsf_ConvertToBoolean
# define Nsf_ConvertTo_Class Nsf_ConvertToClass
# define Nsf_ConvertTo_Int32 Nsf_ConvertToInt32
# define Nsf_ConvertTo_Integer Nsf_ConvertToInteger
# define Nsf_ConvertTo_Object Nsf_ConvertToObject
# define Nsf_ConvertTo_Pointer Nsf_ConvertToPointer
# define Nsf_ConvertTo_String Nsf_ConvertToString
# define Nsf_ConvertTo_Tclobj Nsf_ConvertToTclobj
#endif


#if !defined(likely)
# if defined(__GNUC__) && __GNUC__ > 2
/* Use gcc branch prediction hint to minimize cost of e.g. DTrace
 * ENABLED checks.
 */
#  define unlikely(x) (__builtin_expect((x), 0))
#  define likely(x) (__builtin_expect((x), 1))
# else
#  define unlikely(x) (x)
#  define likely(x) (x)
# endif
#endif

}
genstubs
puts stderr "[array size ::definitions] parsing stubs generated"

#
# Local variables:
#    mode: tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:
