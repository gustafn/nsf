/*
 * nsfStubLib.c --
 *
 *      Stub object that will be statically linked into extensions of the Next
 *      Scripting Framework.
 *
 * Copyright (c) 1998 Paul Duffin
 * Copyright (c) 2001-2013 Gustaf Neumann
 * Copyright (c) 2001-2007 Uwe Zdun
 *
 * See the file "tcl-license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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
MODULE_SCOPE const NsfStubs *nsfStubsPtr;
MODULE_SCOPE const NsfIntStubs *nsfIntStubsPtr;
#endif
CONST86 NsfStubs *nsfStubsPtr = NULL;
CONST86 NsfIntStubs *nsfIntStubsPtr = NULL;


/*
 *----------------------------------------------------------------------
 *
 * Nsf_InitStubs --
 *
 *      Tries to initialise the stub table pointers and ensures that
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

CONST char *
Nsf_InitStubs(Tcl_Interp *interp, CONST char *version, int exact) {
    CONST char *actualVersion;
    const char *packageName = "nsf";
    ClientData clientData = NULL;

    actualVersion = Tcl_PkgRequireEx(interp, "nsf", version, exact,
				     &clientData);

    /*fprintf(stderr, "Nsf_InitStubs required nsf version %s exact %d -> %s %p\n",
      version, exact, actualVersion, clientData);*/

    if (clientData == NULL) {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp, "Error loading package ", packageName,
		       ": package not present or incomplete", NULL);
        return NULL;
    } else {
      CONST86 NsfStubs * const stubsPtr = clientData;
      CONST86 NsfIntStubs * const intStubsPtr = stubsPtr->hooks ?
        stubsPtr->hooks->nsfIntStubs : NULL;

      if (actualVersion == NULL) {
        return NULL;
      }

      if (!stubsPtr || !intStubsPtr) {
        static char *errMsg = "missing stub table pointer";

        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "Error loading package", packageName,
                         ": (requested version '", version, "', loaded version '",
                         actualVersion, "'): ", errMsg, NULL);
        return NULL;
      }

      nsfStubsPtr = stubsPtr;
      nsfIntStubsPtr = intStubsPtr;

      return actualVersion;
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
