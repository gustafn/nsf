/*
 * Macros to abstract access to Tcl internals where possible.
 * This file is part of the Next Scripting Framework
 *
 * Copyright (C) 2010-2014 Gustaf Neumann
 *
 * Vienna University of Economics and Business
 * Institute of Information Systems and New Media
 * A-1020, Welthandelsplatz 1
 * Vienna, Austria
 *
 * This work is licensed under the MIT License https://www.opensource.org/licenses/MIT
 *
 * Copyright:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
#define Tcl_Interp_numLevels(interp)       ((Interp *)(interp))->numLevels
#define Tcl_Interp_framePtr(interp)        ((Tcl_CallFrame *)((Interp *)(interp))->framePtr)
#define Tcl_Interp_varFramePtr(interp)     (((Interp *)(interp))->varFramePtr)
#define Tcl_Interp_cmdFramePtr(interp)     (((Interp *)(interp))->cmdFramePtr)
#define Tcl_Interp_globalNsPtr(interp)     ((Tcl_Namespace *)((Interp *)(interp))->globalNsPtr)
#define Tcl_Interp_flags(interp)           ((Interp *)(interp))->flags
#define Tcl_Interp_threadId(interp)        ((Interp *)(interp))->threadId

#define Tcl_CallFrame_callerPtr(cf)       ((Tcl_CallFrame*)((CallFrame *)(cf))->callerPtr)
#define Tcl_CallFrame_procPtr(cf)         ((CallFrame *)(cf))->procPtr
#define Tcl_CallFrame_varTablePtr(cf)     ((CallFrame *)(cf))->varTablePtr
#define Tcl_CallFrame_level(cf)           ((CallFrame *)(cf))->level
#define Tcl_CallFrame_isProcCallFrame(cf) ((CallFrame *)(cf))->isProcCallFrame
#define Tcl_CallFrame_compiledLocals(cf)  ((CallFrame *)(cf))->compiledLocals
#define Tcl_CallFrame_numCompiledLocals(cf)  ((CallFrame *)(cf))->numCompiledLocals
#define Tcl_CallFrame_callerVarPtr(cf)    ((Tcl_CallFrame*)((CallFrame *)(cf))->callerVarPtr)
#define Tcl_CallFrame_objc(cf)            ((CallFrame *)(cf))->objc
#define Tcl_CallFrame_objv(cf)            ((CallFrame *)(cf))->objv
#define Tcl_CallFrame_clientData(cf)      ((CallFrame *)(cf))->clientData
#define Tcl_CallFrame_nsPtr(cf)           ((Tcl_Namespace *)((CallFrame *)(cf))->nsPtr)

#define Tcl_Namespace_cmdTablePtr(nsPtr) &((Namespace *)(nsPtr))->cmdTable
#define Tcl_Namespace_varTablePtr(nsPtr) &((Namespace *)(nsPtr))->varTable
#define Tcl_Namespace_childTablePtr(nsPtr) &((Namespace *)(nsPtr))->childTable
#define Tcl_Namespace_activationCount(nsPtr) ((Namespace *)(nsPtr))->activationCount
#define Tcl_Namespace_deleteProc(nsPtr)  ((Namespace *)(nsPtr))->deleteProc
#define Tcl_Namespace_parentPtr(nsPtr)   ((Namespace *)(nsPtr))->parentPtr
#define Tcl_Namespace_commandPathLength(nsPtr) ((Namespace *)(nsPtr))->commandPathLength
#define Tcl_Namespace_commandPathArray(nsPtr)  ((Namespace *)(nsPtr))->commandPathArray
#define Tcl_Namespace_refCount(nsPtr)  ((Namespace *)(nsPtr))->refCount
#define Tcl_Namespace_flags(nsPtr)  ((Namespace *)(nsPtr))->flags


#define Tcl_Command_refCount(cmd)      ((Command *)(cmd))->refCount
#define Tcl_Command_cmdEpoch(cmd)      ((Command *)(cmd))->cmdEpoch
#define Tcl_Command_flags(cmd)         ((Command *)(cmd))->flags
/* the following items could be obtained from 
   Tcl_GetCommandInfoFromToken(cmd, infoPtr) */
#define Tcl_Command_nsPtr(cmd)         ((Tcl_Namespace*)(((Command *)(cmd))->nsPtr))
#define Tcl_Command_objProc(cmd)       ((Command *)(cmd))->objProc
#if defined(NRE)
# define Tcl_Command_nreProc(cmd)       ((Command *)(cmd))->nreProc
#endif
#define Tcl_Command_objClientData(cmd) ((Command *)(cmd))->objClientData
#define Tcl_Command_proc(cmd)          ((Command *)(cmd))->proc
#define Tcl_Command_clientData(cmd)    ((Command *)(cmd))->clientData
#define Tcl_Command_deleteProc(cmd)    ((Command *)(cmd))->deleteProc
#define Tcl_Command_deleteData(cmd)    ((Command *)(cmd))->deleteData

/* 
 * The offsetof() macro is an ANSI C library feature.
 * Workaround for platforms missing offsetof(), e.g. VC++ 6.0 
 */
#ifndef offsetof
# define offsetof(type, field) ((size_t) ((char *) &((type *) 0)->field))
#endif

/*
 * Var Reform Compatibility support.
 *
 *   Definitions for accessing Tcl variable structures after varreform
 *   in Tcl 8.5.
 */

#define TclIsCompiledLocalArgument(compiledLocalPtr)  ((compiledLocalPtr)->flags & VAR_ARGUMENT)
#define TclIsCompiledLocalTemporary(compiledLocalPtr) ((compiledLocalPtr)->flags & VAR_TEMPORARY)

#define TclVarHashGetValue(hPtr)	((Var *) ((char *)(hPtr) - offsetof(VarInHash, entry)))
#define TclVarHashGetKey(varPtr)	(((VarInHash *)(varPtr))->entry.key.objPtr)
#define TclVarHashTablePtr(varTablePtr)		&(varTablePtr)->table
#define TclVarValue(type, varPtr, field)	(type *)(varPtr)->value.field

#if !defined(Tcl_HashSize)
# define Tcl_HashSize(tablePtr) ((tablePtr)->numEntries)
#endif




