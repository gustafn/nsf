/*
 * nsfCmdPtr.c --
 *
 *      Conversion from CmdPtr to Class / Object
 *
 * Copyright (C) 2014-2018 Gustaf Neumann
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
 */

/*
 * nsfInt is just needed for NSF_INLINE
 */
#include <nsfInt.h>

static NSF_INLINE NsfObject* NsfGetObjectFromCmdPtr(const Tcl_Command cmd) nonnull(1);
static NSF_INLINE ClientData NsfGetClientDataFromCmdPtr(const Tcl_Command cmd) nonnull(1);
#ifdef NSF_C
static NSF_INLINE NsfClass*  NsfGetClassFromCmdPtr(const Tcl_Command cmd) nonnull(1);
#endif

static NSF_INLINE ClientData
NsfGetClientDataFromCmdPtr(const Tcl_Command cmd) {
  ClientData result;

  nonnull_assert(cmd != NULL);

  /*  
   * When we activate NS_TCL_HAVE_TIP629 (TIP supporting > 2^31
   * elements in object vectors), Tcl defines different objProcs
   * (Tcl_ObjCmdProc and Tcl_ObjCmdProc2) where the usage of these
   * depends on a cmdWrapperProc. Unfortunately, the resolving of
   * these are performed via CmdWrapperInfo, which is not exported. We
   * have to think how to resolve these to make this working as with
   * prior Tcl versions.
   *
  (lldb) p *(Command*)cmd
  (Command) $1 = {
    hPtr = 0x00000001048175d0
    nsPtr = 0x00000001018b8410
    refCount = 2
    cmdEpoch = 0
    compileProc = 0x0000000000000000
    objProc = 0x00000001005e5bc0 (libtcl9.0.dylib`cmdWrapperProc at tclBasic.c:2663)
    objClientData = 0x000000010105d5d0
    proc = 0x00000001005e1e24 (libtcl9.0.dylib`TclInvokeObjectCommand at tclBasic.c:2991)
    clientData = 0x0000000101057910
    deleteProc = 0x00000001005e5c2c (libtcl9.0.dylib`cmdWrapperDeleteProc at tclBasic.c:2671)
    deleteData = 0x000000010105d5d0
    flags = 0
    importRefPtr = NULL
    tracePtr = NULL
    nreProc = 0x00000001005eb1c0 (libtcl9.0.dylib`cmdWrapperNreProc at tclBasic.c:8559)
    } 
    */
  
  /*fprintf(stderr, "NsfGetClientDataFromCmdPtr objProc=%p %p\n",
    (void*)TCL_COMMAND_OBJPROC(cmd), (void*)NsfObjDispatch);*/
  if (likely((TCL_OBJCMDPROC_T*)Tcl_Command_objProc(cmd) == NsfObjDispatch)) {
    result = Tcl_Command_objClientData(cmd);

  } else {
    Tcl_Command cmd1 = TclGetOriginalCommand(cmd);
    
    if (likely(cmd1 != NULL) && unlikely((TCL_OBJCMDPROC_T*)Tcl_Command_objProc(cmd1) == NsfObjDispatch)) {
      result = Tcl_Command_objClientData(cmd1);
    } else {
      result = NULL;
    }
  }
  return result;
}

#ifdef NSF_C
static NSF_INLINE NsfClass*
NsfGetClassFromCmdPtr(const Tcl_Command cmd) {
  ClientData  cd;
  NsfClass   *result;

  nonnull_assert(cmd != NULL);

  cd = NsfGetClientDataFromCmdPtr(cmd);
  /*fprintf(stderr, "cd=%p\n", cd);*/
  if (likely(cd != NULL)) {
    result = NsfObjectToClass(cd);
  } else {
    result = NULL;
  }
  return result;
}
#endif

static NSF_INLINE NsfObject*
NsfGetObjectFromCmdPtr(const Tcl_Command cmd) {

  nonnull_assert(cmd != NULL);

  return (NsfObject*) NsfGetClientDataFromCmdPtr(cmd);
}
