/* 
 * nsfStubInit.c --
 *
 *	This file contains the initializers for the stub vectors of the Next
 *	Scripting Framework.
 *
 * Copyright (C) 1999-2014 Gustaf Neumann (a, b)
 *
 * (a) University of Essen
 *      Specification of Software Systems
 *      Altendorferstrasse 97-101
 *      D-45143 Essen, Germany
 *
 * (b) Vienna University of Economics and Business
 *      Institute of Information Systems and New Media
 *      A-1020, Welthandelsplatz 1
 *      Vienna, Austria
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

#include "nsfInt.h"

/*
 * Remove macros that will interfere with the definitions below.
 */

/*
 * WARNING: The contents of this file is automatically generated by the
 * tools/genStubs.tcl script. Any modifications to the function declarations
 * below should be made in the generic/tcl.decls script.
 */

#if defined(PRE86)
EXTERN NsfStubs nsfStubs;
# else
MODULE_SCOPE const NsfStubs nsfStubs;
#endif




/* !BEGIN!: Do not edit below this line. */

NsfIntStubs nsfIntStubs = {
    TCL_STUB_MAGIC,
    NULL,
};

static NsfStubHooks nsfStubHooks = {
    &nsfIntStubs
};

NsfStubs nsfStubs = {
    TCL_STUB_MAGIC,
    &nsfStubHooks,
    Nsf_Init, /* 0 */
    NULL, /* 1 */
    NsfIsClass, /* 2 */
    NsfGetObject, /* 3 */
    NsfGetClass, /* 4 */
    NsfDeleteObject, /* 5 */
    NsfRemoveObjectMethod, /* 6 */
    NsfRemoveClassMethod, /* 7 */
    Nsf_ObjSetVar2, /* 8 */
    Nsf_ObjGetVar2, /* 9 */
    Nsf_UnsetVar2, /* 10 */
    NsfDStringVPrintf, /* 11 */
    NsfPrintError, /* 12 */
    NsfErrInProc, /* 13 */
    NsfObjErrType, /* 14 */
    NsfStackDump, /* 15 */
    NsfSetObjClientData, /* 16 */
    NsfGetObjClientData, /* 17 */
    NsfSetClassClientData, /* 18 */
    NsfGetClassClientData, /* 19 */
    NsfRequireObjNamespace, /* 20 */
    NsfCallMethodWithArgs, /* 21 */
    NsfAddObjectMethod, /* 22 */
    NsfAddClassMethod, /* 23 */
    NsfCreate, /* 24 */
    Nsf_ArgumentParse, /* 25 */
    NsfLog, /* 26 */
    Nsf_PointerAdd, /* 27 */
    Nsf_PointerDelete, /* 28 */
    Nsf_PointerTypeRegister, /* 29 */
    Nsf_ConvertToBoolean, /* 30 */
    Nsf_ConvertToClass, /* 31 */
    Nsf_ConvertToInt32, /* 32 */
    Nsf_ConvertToInteger, /* 33 */
    Nsf_ConvertToObject, /* 34 */
    Nsf_ConvertToPointer, /* 35 */
    Nsf_ConvertToString, /* 36 */
    Nsf_ConvertToTclobj, /* 37 */
    Nsf_EnumerationTypeRegister, /* 38 */
    Nsf_CmdDefinitionRegister, /* 39 */
    NsfArgumentError, /* 40 */
    Nsf_DStringPrintf, /* 41 */
};

/* !END!: Do not edit above this line. */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * End:
 */
