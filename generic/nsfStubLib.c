/*
 * nsfStubLib.c --
 *
 *      Stub object that will be statically linked into extensions of the Next
 *      Scripting Framework.
 *
 * Copyright (C) 1998 Paul Duffin
 * Copyright (C) 2001-2017 Gustaf Neumann (a)
 * Copyright (C) 2001-2007 Uwe Zdun (a)
 *
 * (a) Vienna University of Economics and Business
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
 * The original work by Paul Duffin was licensed as:
 *
 * "See the file "tcl-license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES."
 *
 * See also https://www.tcl-lang.org/software/tcltk/license.html.
 *
 */

/*
 * We need to ensure that we use the stub macros so that this file contains
 * no references to any of the stub functions.  This will make it possible
 * to build an extension that references Tcl_InitStubs but doesn't end up
 * including the rest of the stub functions.
 */

#ifndef USE_TCL_STUBS
# define USE_TCL_STUBS
#endif
#undef USE_TCL_STUB_PROCS

/*
 * This ensures that the Nsf_InitStubs has a prototype in
 * nsf.h and is not the macro that turns it into Tcl_PkgRequire
 */

#ifndef USE_NSF_STUBS
# define USE_NSF_STUBS
#endif

#include "nsfInt.h"

#if defined(PRE86)
extern NsfStubs *nsfStubsPtr;
#else
//MODULE_SCOPE const NsfStubs *nsfStubsPtr;
//MODULE_SCOPE const NsfIntStubs *nsfIntStubsPtr;
#endif
CONST86 NsfStubs *nsfStubsPtr = NULL;
CONST86 NsfIntStubs *nsfIntStubsPtr = NULL;


/*
 *----------------------------------------------------------------------
 *
 * Nsf_InitStubs --
 *
 *      Tries to initialize the stub table pointers and ensures that
 *      the correct version of nsf is loaded.
 *
 * Results:
 *      The actual version of nsf that satisfies the request, or
 *      NULL to indicate that an error occurred.
 *
 * Side effects:
 *      Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */

const char *
Nsf_InitStubs(Tcl_Interp *interp, const char *version, int exact) {
  const char *actualVersion, *packageName = "nsf";
  ClientData  clientData = NULL;

  actualVersion = Tcl_PkgRequireEx(interp, "nsf", version, exact,
                                   &clientData);
  if (clientData == NULL) {
    /*
     * When the "package require" for NSF fails, we can't use
     * any NSF calls, like e.g. NsfPrintError().
     */
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "Error loading ", packageName, " package: "
                     "package not present, incomplete or misconfigured. "
                     "Maybe NSF was not compiled with COMPILE_NSF_STUBS enabled?",
                     (char*) 0L);
    actualVersion = NULL;
  } else {
    CONST86 NsfStubs *stubsPtr;

    if (actualVersion != NULL) {

      stubsPtr = clientData;
      if (stubsPtr->hooks == NULL) {
        static const char *errMsg = "missing stubInt table pointer";

        NsfPrintError(interp, "Error loading package %s: "
                      "(requested version '%s', loaded version '%s'): %s",
                      packageName, version, actualVersion, errMsg);
        actualVersion = NULL;
      } else {
        CONST86 NsfIntStubs *intStubsPtr;

        intStubsPtr = stubsPtr->hooks->nsfIntStubs;

        nsfStubsPtr = stubsPtr;
        nsfIntStubsPtr = intStubsPtr;
      }

    }
  }
  return actualVersion;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */
