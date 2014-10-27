/*
 * aolstub.c --
 *
 * 	This file provides the stubs needed for the AOLserver, to load NSF.
 *  	Please note that at least AOLserver 4.* or NaviServer 4.99.4 or newer
 *  	are required.  might have to have to apply a small patch to the
 *  	AOLserver as well (available from www.xotcl.org) in order to get it
 *  	working.
 *
 * Copyright (C) 2006-2013 Zoran Vasiljevic (a)
 * Copyright (C) 2006-2014 Gustaf Neumann (b)
 *
 * (a) Archiware Inc.
 *
 * (b) Vienna University of Economics and Business
 *     Institute of Information Systems and New Media
 *     A-1020, Welthandelsplatz 1
 *     Vienna, Austria
 *
 * This work is licensed under the MIT License http://www.opensource.org/licenses/MIT
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
#ifdef AOL_SERVER


#include "nsf.h"
#include <ns.h>

NS_EXPORT int Ns_ModuleVersion = 1;

#if NS_MAJOR_VERSION>=4
# define AOL4
#endif

/*
 *----------------------------------------------------------------------------
 *
 * NsNsf_Init --
 *
 *    Loads the package for the first time, i.e. in the startup thread.
 *
 * Results:
 *    Standard Tcl result
 *
 * Side effects:
 *    Package initialized. Tcl commands created.
 *
 *----------------------------------------------------------------------------
 */


static int
NsNsf_Init (Tcl_Interp *interp, void *context)
{
 static int firsttime = 1;
 int ret;

 ret = Nsf_Init(interp);

 if (firsttime) {
   if (ret != TCL_OK) {
     Ns_Log(Warning, "can't load module %s: %s", (char *)context,
	    Tcl_GetStringResult(interp));
   } else {
     Ns_Log(Notice, "%s module version %s", (char*)context, NSF_PATCHLEVELL);
     /*
      * Import the Nsf namespace only for the shell after
      * predefined is through
      */
     Tcl_Import(interp, Tcl_GetGlobalNamespace(interp), "nsf::*", 0);
   }
   firsttime = 0;
 }

 return ret;
}

/*
 *----------------------------------------------------------------------------
 *
 * NsNsf_Init1 --
 *
 *    Loads the package in each thread-interpreter.
 *    This is needed since Nsf Class/Object commands are not copied
 *    from the startup thread to the connection (or any other) thread.
 *    during AOLserver initialization and/or thread creation times.
 *
 *    Why ?
 *
 *    Simply because these two commands declare a delete callback which is
 *    unsafe to call in any other thread but in the one which created them.
 *
 *    To understand this, you may need to get yourself acquainted with the
 *    mechanics of the AOLserver, more precisely, with the way Tcl interps
 *    are initialized (dive into nsd/tclinit.c in AOLserver distro).
 *
 *    So, we made sure (by patching the AOLserver code) that no commands with
 *    delete callbacks declared, are ever copied from the startup thread.
 *    Additionaly, we also made sure that AOLserver properly invokes any
 *    AtCreate callbacks. So, instead of activating those callbacks *after*
 *    running the Tcl-initialization script (which is the standard behaviour)
 *    we activate them *before*. So we may get a chance to configure the
 *    interpreter correctly for any commands within the init script.
 *
 *    Proper Nsf usage would be to declare all resources (classes, objects)
 *    at server initialization time and let AOLserver machinery to copy them
 *    (or re-create them, better yet) in each new thread.
 *    Resources created within a thread are automatically garbage-collected
 *    on thread-exit time, so don't create any Nsf resources there.
 *    Create them in the startup thread and they will automatically be copied
 *    for you.
 *    Look in <serverroot>/modules/tcl/nsf for a simple example.
 *
 * Results:
 *    Standard Tcl result.
 *
 * Side effects:
 *    Tcl commands created.
 *
 *----------------------------------------------------------------------------
 */

static int
NsNsf_Init1 (Tcl_Interp *interp, void *notUsed)
{
  int result;

#ifndef AOL4
  result = Nsf_Init(interp);
#else
  result = TCL_OK;
#endif

  /*
   * Import the Nsf namespace only for the shell after
   * predefined is through
   */
  Tcl_Import(interp, Tcl_GetGlobalNamespace(interp), "nsf::*", 1);

  return result;
}

/*
 *----------------------------------------------------------------------------
 *
 * Ns_ModuleInit --
 *
 *    Called by the AOLserver when loading shared object file.
 *
 * Results:
 *    Standard AOLserver result
 *
 * Side effects:
 *    Many. Depends on the package.
 *
 *----------------------------------------------------------------------------
 */

int Ns_ModuleInit(char *hServer, char *hModule) nonnull(1) nonnull(2);

int
Ns_ModuleInit(char *hServer, char *hModule) {
  int ret;

  assert(hServer);
  assert(hModule);

  /*Ns_Log(Notice, "+++ ModuleInit","INIT");*/
  ret = Ns_TclInitInterps(hServer, NsNsf_Init, (void*)hModule);

  if (ret == TCL_OK) {
    /*
     * See discussion for NsNsf_Init1 procedure.
     * Note that you need to patch AOLserver for this to work!
     * The patch basically forbids copying of C-level commands with
     * declared delete callbacks. It also runs all AtCreate callbacks
     * BEFORE AOLserver runs the Tcl script for initializing new interps.
     * These callbacks are then responsible for setting up the stage
     * for correct (Nsf) extension startup (including copying any
     * Nsf resources (classes, objects) created in the startup thread.
     */
    Ns_TclRegisterAtCreate((Ns_TclInterpInitProc *)NsNsf_Init1, NULL);
  }

  return ret == TCL_OK ? NS_OK : NS_ERROR;
}
#endif

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */
