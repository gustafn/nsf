/*
 * nsfObjectData.c --
 *
 *      NSF object data assumes NSF_OBJECTDATA to be compiled in.  When
 *      specified, it can be used to equip every object with an
 *      additional payload from C.
 *
 * Copyright (C) 1999-2014 Gustaf Neumann (a, b)
 * Copyright (C) 1999-2007 Uwe Zdun (a, b)
 * 
 * (a) University of Essen
 *     Specification of Software Systems
 *     Altendorferstrasse 97-101
 *     D-45143 Essen, Germany
 *
 * (b) Vienna University of Economics and Business
 *     Institute of Information Systems and New Media
 *     A-1020, Welthandelsplatz 1
 *     Vienna, Austria
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
 *
 */

#include "nsfInt.h"

#ifdef NSF_OBJECTDATA
EXTERN void
NsfFreeObjectData(NsfClass* cl) {
  if (cl->opt && cl->opt->objectdata) {
    Tcl_DeleteHashTable(cl->opt->objectdata);
    ckfree((char*)cl->opt->objectdata);
    cl->opt->objectdata = 0;
  }
}
EXTERN void
NsfSetObjectData(NsfObject* obj, NsfClass* cl, ClientData data) {
  Tcl_HashEntry *hPtr;
  int nw;

  NsfRequireClassOpt(cl);

  if (!cl->opt->objectdata) {
    cl->opt->objectdata = (Tcl_HashTable*)ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(cl->opt->objectdata, TCL_ONE_WORD_KEYS);
  }
  hPtr = Tcl_CreateHashEntry(cl->opt->objectdata, (char*)obj, &nw);
  Tcl_SetHashValue(hPtr, data);
}

EXTERN int
NsfGetObjectData(NsfObject* obj, NsfClass* cl, ClientData* data) {
  Tcl_HashEntry *hPtr;
  if (!cl->opt || !cl->opt->objectdata)
    return 0;
  hPtr = Tcl_FindHashEntry(cl->opt->objectdata, (char*)obj);
  if (data) *data = hPtr ? Tcl_GetHashValue(hPtr) : 0;
  return hPtr != 0;
}

EXTERN int
NsfUnsetObjectData(NsfObject* obj, NsfClass* cl) {
  Tcl_HashEntry *hPtr;

  if (!cl->opt || !cl->opt->objectdata)
    return 0;
  hPtr = Tcl_FindHashEntry(cl->opt->objectdata, (char*)obj);
  if (hPtr) Tcl_DeleteHashEntry(hPtr);
  return hPtr != 0;
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
