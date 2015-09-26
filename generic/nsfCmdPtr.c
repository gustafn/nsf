/*
 * nsfCmdPtr.c --
 *
 *      Conversion from CmdPtr to Class / Object
 *
 * Copyright (C) 2014-2015 Gustaf Neumann
 *
 * Vienna University of Economics and Business
 * Institute of Information Systems and New Media
 * A-1020, Welthandelsplatz 1
 * Vienna, Austria
 *
 * This work is licensed under the MIT License http://www.opensource.org/licenses/MIT
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

 */

static NSF_INLINE NsfObject*NsfGetObjectFromCmdPtr(Tcl_Command cmd) nonnull(1);
static NSF_INLINE NsfClass*NsfGetClassFromCmdPtr(Tcl_Command cmd) nonnull(1);
static NSF_INLINE ClientData NsfGetClientDataFromCmdPtr(Tcl_Command cmd) nonnull(1);

static NSF_INLINE ClientData
NsfGetClientDataFromCmdPtr(Tcl_Command cmd) {
  
  assert(cmd != NULL);
  
  /*fprintf(stderr, "objProc=%p %p\n", Tcl_Command_objProc(cmd),NsfObjDispatch);*/
  if (likely(Tcl_Command_objProc(cmd) == NsfObjDispatch)) {
    return Tcl_Command_objClientData(cmd);
  } else {
    cmd = TclGetOriginalCommand(cmd);
    if (likely(cmd != NULL) && unlikely(Tcl_Command_objProc(cmd) == NsfObjDispatch)) {
      /*fprintf(stderr, "???? got cmd right in 2nd round\n");*/
      return Tcl_Command_objClientData(cmd);
    }
    return NULL;
  }
}

static NSF_INLINE NsfClass*
NsfGetClassFromCmdPtr(Tcl_Command cmd) {
  ClientData cd = NsfGetClientDataFromCmdPtr(cmd);

  assert(cmd != NULL);

  /*fprintf(stderr, "cd=%p\n",cd);*/
  if (likely(cd != NULL)) {
    return NsfObjectToClass(cd);
  } else {
    return NULL;
  }
}

static NSF_INLINE NsfObject*
NsfGetObjectFromCmdPtr(Tcl_Command cmd) {

  assert(cmd != NULL);

  return (NsfObject*) NsfGetClientDataFromCmdPtr(cmd);
}
