/*  
 *  nsfObjectData.c --
 *  
 *      Nsf Object Data, needs NSF_OBJECTDATA to be compiled in.  When
 *      specified, it can be use to equip every object from C with an
 *      additional payload.
 *  
 *  Copyright (C) 1999-2011 Gustaf Neumann
 *  Copyright (C) 1999-2007 Uwe Zdun
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

#include "nsfInt.h"

#ifdef NSF_OBJECTDATA
extern void
NsfFreeObjectData(NsfClass* cl) {
  if (cl->opt && cl->opt->objectdata) {
    Tcl_DeleteHashTable(cl->opt->objectdata);
    ckfree((char*)cl->opt->objectdata);
    cl->opt->objectdata = 0; 
  }
}
extern void
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

extern int
NsfGetObjectData(NsfObject* obj, NsfClass* cl, ClientData* data) {
  Tcl_HashEntry *hPtr;
  if (!cl->opt || !cl->opt->objectdata) 
    return 0;
  hPtr = Tcl_FindHashEntry(cl->opt->objectdata, (char*)obj);
  if (data) *data = hPtr ? Tcl_GetHashValue(hPtr) : 0;
  return hPtr != 0;
}

extern int
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
 * End:
 */
