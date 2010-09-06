/*
 * nsfgdbm.c
 *
 * based on Tclndbm 0.5 by John Ellson (ellson@lucent.com)
 */

#include <stdio.h>
#include <tcl.h>
#include <gdbm.h>
#include <fcntl.h>
#include <stdlib.h>
#include <nsf.h>

#if (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION<1)
# define TclObjStr(obj) Tcl_GetStringFromObj(obj, ((int*)NULL))
#else
# define TclObjStr(obj) Tcl_GetString(obj)
#endif

typedef struct db_s {
  datum* lastSearchKey;
  GDBM_FILE db;
} db_t;

void
gdbmFatalFunc(char* message) {
  fprintf(stderr, "GDBM FATAL:\n%s\n", message);
  exit(1);
}

static int
NsfGdbmOpenMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  db_t *db;
  Nsf_Object* obj = (Nsf_Object*) cd;
  int flags, block_size, mode;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 2)
    return NsfObjErrArgCnt(in, obj->cmdName, "open filename");

  /* name not in hashtab - create new db */
  if (NsfGetObjClientData(obj))
    return NsfVarErrMsg(in, "Called open on '", TclObjStr(obj->cmdName),
			  "', but open database was not closed before.", 0);

  db = (db_t*) ckalloc (sizeof(db_t));
  db->lastSearchKey = NULL;

  flags = GDBM_WRCREAT;
  block_size = 0;
  mode = 0644;
  
  db->db = gdbm_open(TclObjStr(objv[1]), block_size, flags, mode, gdbmFatalFunc);

  if (db->db == NULL) {  
    ckfree ((char*) db);
    db = (db_t*) NULL ;
    return NsfVarErrMsg(in, "Open on '", TclObjStr(obj->cmdName),
			  "' failed with '", TclObjStr(objv[1]),"': ", 
			  gdbm_strerror(gdbm_errno), 0);
  }
  /* 
   * success
   */
  NsfSetObjClientData(obj, (ClientData) db);
  return TCL_OK;
}

static int
NsfGdbmCloseMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  db_t *db;
  Nsf_Object* obj = (Nsf_Object *) cd;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 1)
    return NsfObjErrArgCnt(in, obj->cmdName, "close");
    
  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfVarErrMsg(in, "Called close on '", TclObjStr(obj->cmdName),
			  "', but database was not opened yet.", 0);
  gdbm_close(db->db);
  ckfree ((char*)db);
  NsfSetObjClientData(obj, 0);
 
  return TCL_OK;
}

static int
NsfGdbmNamesMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  Tcl_Obj *list;
  db_t *db;
  Tcl_DString result;
  datum del, key;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 1)
    return NsfObjErrArgCnt(in, obj->cmdName, "names");
  
  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfVarErrMsg(in, "Called names on '", TclObjStr(obj->cmdName),
			  "', but database was not opened yet.", 0);
  Tcl_DStringInit(&result);
  
  key = gdbm_firstkey(db->db);
  if (!key.dptr) {
    /* empty db */
    return TCL_OK ;
  }
  
  /* 
   * copy key to result and go to next key
   */
  list = Tcl_NewListObj(0, NULL);
  do {
      Tcl_ListObjAppendElement(in,list,Tcl_NewStringObj(key.dptr,key.dsize-1));
      del.dptr = key.dptr;
      key = gdbm_nextkey(db->db, key);
      free(del.dptr);
  } while (key.dptr);
  Tcl_SetObjResult(in, list);

  return TCL_OK;
}

static int
NsfGdbmSetMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *)cd;
  db_t *db;
  datum key, content;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc <2 || objc > 3)
    return NsfObjErrArgCnt(in, obj->cmdName, "set key ?value?");

  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfVarErrMsg(in, "Called set on '", TclObjStr(obj->cmdName),
			  "', but database was not opened yet.", 0);

  key.dptr = TclObjStr(objv[1]);
  key.dsize = objv[1]->length + 1;

  if (objc == 2) {
      /* get value */
      content = gdbm_fetch(db->db, key);
      if (content.dptr) {
	  /* found */
	  Tcl_Obj *r = Tcl_NewStringObj(content.dptr, content.dsize-1);
	  Tcl_SetObjResult(in, r);
	  free(content.dptr);
      } else {
	  /* key not found */
	  return NsfVarErrMsg(in, "no such variable '", key.dptr,
				"'", 0);
      }
  } else {
      /* set value */
      content.dptr = TclObjStr(objv[2]);
      content.dsize = objv[2]->length + 1;
      if (gdbm_store(db->db, key, content, GDBM_REPLACE) == 0) {
	  /*fprintf(stderr,"setting %s to '%s'\n",key.dptr,content.dptr);*/
	  Tcl_SetObjResult(in, objv[2]);
      } else {
	  return NsfVarErrMsg(in, "set of variable '", TclObjStr(obj->cmdName),
				"' failed.", 0);
      }
  }    
  return TCL_OK;
}

static int
NsfGdbmExistsMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  db_t *db;
  datum key;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 2)
    return NsfObjErrArgCnt(in, obj->cmdName, "exists variable");

  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfVarErrMsg(in, "Called exists on '", TclObjStr(obj->cmdName),
			  "', but database was not opened yet.", 0);

  key.dptr = TclObjStr(objv[1]);
  key.dsize = objv[1]->length + 1;

  if (gdbm_exists(db->db, key))
    Tcl_SetIntObj(Tcl_GetObjResult(in), 1);
  else
    Tcl_SetIntObj(Tcl_GetObjResult(in), 0);

  return TCL_OK;
}

static int
NsfGdbmUnsetMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  db_t *db;
  datum key;
  int ret;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 2)
    return NsfObjErrArgCnt(in, obj->cmdName, "unset key");
  
  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfVarErrMsg(in, "Called unset on '", TclObjStr(obj->cmdName),
			  "', but database was not opened yet.", 0);

  key.dptr = TclObjStr(objv[1]);
  key.dsize = objv[1]->length + 1;

  ret = gdbm_delete(db->db, key);

  if (ret == 0) {
    return TCL_OK;
  } else {
    return NsfVarErrMsg(in, "Tried to unset '", TclObjStr(objv[1]), 
			  "' but key does not exist.", 0);
  }
}

static int 
NsfGdbmFirstKeyMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  db_t *db;
  datum key;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 1)
    return NsfObjErrArgCnt(in, obj->cmdName, "firstkey");
  
  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfVarErrMsg(in, "Called unset on '", TclObjStr(obj->cmdName),
			  "', but database was not opened yet.", 0);

  if (db->lastSearchKey != 0) {
    ckfree((char*) db->lastSearchKey->dptr);
    ckfree((char*) db->lastSearchKey);
    db->lastSearchKey = 0;
  }

  key = gdbm_firstkey(db->db);
  if (!key.dptr) {
    /*
     * empty db
     */
    return TCL_OK;
  }
  
  Tcl_AppendResult (in, key.dptr, (char*)0);
  
  db->lastSearchKey = (datum*) ckalloc(sizeof(datum));
  db->lastSearchKey->dptr = key.dptr;
  db->lastSearchKey->dsize = key.dsize;

  return TCL_OK;
}

static int 
NsfGdbmNextKeyMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  db_t *db;
  datum  newkey;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 1)
    return NsfObjErrArgCnt(in, obj->cmdName, "nextkey");
  
  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfVarErrMsg(in, "Called unset on '", TclObjStr(obj->cmdName),
			  "', but database was not opened yet.", 0);
  if (db->lastSearchKey == 0)
    return NsfVarErrMsg(in, 
			  "nextkey invoked, but no search was started on '", 
			  TclObjStr(obj->cmdName), "'", 0);

  newkey = gdbm_nextkey(db->db, *db->lastSearchKey);

  if (!newkey.dptr) {
    /*
     * end of search
     */
    if (db->lastSearchKey != 0) {
      free((char*) db->lastSearchKey->dptr);
      ckfree((char*) db->lastSearchKey);
      db->lastSearchKey = 0;
    }
    return TCL_OK ;
  }

  Tcl_AppendResult (in, newkey.dptr, (char*)0);
  if (db->lastSearchKey != 0) {
    free((char*) db->lastSearchKey->dptr);
  }
  db->lastSearchKey->dptr = newkey.dptr;
  db->lastSearchKey->dsize = newkey.dsize;
  return TCL_OK;
}


/*
 * Xotclgdbm_Init
 * register commands, init data structures
 */

extern int 
Xotclgdbm_Init(Tcl_Interp * in) {
  Nsf_Class* cl;
  int result;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(in, TCL_VERSION, 0) == NULL) {
        return TCL_ERROR;
    }
# ifdef USE_NSF_STUBS
    if (Nsf_InitStubs(in, "1.1", 0) == NULL) {
        return TCL_ERROR;
    }
# endif
#else
    if (Tcl_PkgRequire(in, "Tcl", TCL_VERSION, 0) == NULL) {
        return TCL_ERROR;
    }
#endif
    Tcl_PkgProvide(in, "nsf::store::gdbm", PACKAGE_VERSION);

#ifdef PACKAGE_REQUIRE_XOTL_FROM_SLAVE_INTERP_WORKS_NOW
    if (Tcl_PkgRequire(in, "XOTcl", XOTCLVERSION, 0) == NULL) {
        return TCL_ERROR;
    }
#endif
    if (Tcl_PkgRequire(in, "nsf::store", 0, 0) == NULL) {
        return TCL_ERROR;
    }
    result = Tcl_VarEval (in, 
			  "::nsf::Class Storage=Gdbm -superclass Storage",
			  (char *) NULL);
    if (result != TCL_OK)
      return result;

    cl = NsfGetClass(in, "Storage=Gdbm");
    NsfAddClassMethod(in, cl, "open", NsfGdbmOpenMethod, 0, 0);
    NsfAddClassMethod(in, cl, "close", NsfGdbmCloseMethod, 0, 0);
    NsfAddClassMethod(in, cl, "set", NsfGdbmSetMethod, 0, 0);
    NsfAddClassMethod(in, cl, "exists", NsfGdbmExistsMethod, 0, 0);
    NsfAddClassMethod(in, cl, "names", NsfGdbmNamesMethod, 0, 0);
    NsfAddClassMethod(in, cl, "unset", NsfGdbmUnsetMethod, 0, 0);
    NsfAddClassMethod(in, cl, "firstkey", NsfGdbmFirstKeyMethod, 0, 0);
    NsfAddClassMethod(in, cl, "nextkey", NsfGdbmNextKeyMethod, 0, 0);

    Tcl_SetIntObj(Tcl_GetObjResult(in), 1);
    return TCL_OK;
}

extern int
Xotclgdbm_SafeInit(interp)
    Tcl_Interp *interp;
{
    return Xotclgdbm_Init(interp);
}
