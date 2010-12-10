/*
 * nsfsdbm.c
 *
 * based on Tclndbm 0.5 by John Ellson (ellson@lucent.com)
 */

#include <stdio.h>
#include <tcl.h>
#include "sdbm.h"
#include <fcntl.h>
#include <nsf.h>

#if (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION<1)
# define TclObjStr(obj) Tcl_GetStringFromObj(obj, ((int*)NULL))
#else
# define TclObjStr(obj) Tcl_GetString(obj)
#endif

/*
 * a database ..
 */

typedef struct db_s {
  int mode;
  DBM *db;
} db_t ;

static int
NsfSdbmOpenMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  int mode;
  db_t *db;
  Nsf_Object* obj = (Nsf_Object *) cd;
/*
  int i;
  fprintf(stderr, "Method=NsfSdbmOpenMethod\n");
  for (i=0; i< objc; i++)
    fprintf(stderr, "   objv[%d]=%s\n",i,TclObjStr(objv[i]));
*/
  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 2)
    return NsfObjErrArgCnt(in, obj->cmdName, "open filename");

    /*
     * check mode string if given
     *
    mode = O_RDONLY ;
    if (argc == 3) {

        if (strcmp(argv[2],"r")==0)
            mode = O_RDONLY ;
        else if (strcmp(argv[2],"rw")==0)
            mode = O_RDWR | O_SYNC ;
        else if (strcmp(argv[2],"rwc")==0)
            mode = O_CREAT | O_RDWR | O_SYNC ;
        else if (strcmp(argv[2],"rwn")==0)
            mode = O_CREAT | O_EXCL | O_RDWR | O_SYNC ;
        else {
            sprintf(buf, BAD_MODE, argv[0], argv[2]);
            Tcl_AppendResult (interp,buf,(char *)0);
            return (TCL_ERROR);
        }
    }
   */
  /* Storage interface at the moment assumes mode=rwc */
#ifdef O_SYNC
  mode = O_CREAT | O_RDWR | O_SYNC;
#else
  mode = O_CREAT | O_RDWR;
#endif

  /* name not in hashtab - create new db */
  if (NsfGetObjClientData(obj))
    return NsfPrintError(in, "Called open on '%s', but open database was not closed before", 
			 TclObjStr(obj->cmdName);

  db = (db_t*) ckalloc (sizeof(db_t));

  /*
   * create new name and malloc space for it
   * malloc extra space for name
  db->name = (char *) malloc (strlen(buf)+1) ;
    if (!db->name) {
        perror ("malloc for name in db_open");
        exit (-1);
        }
    strcpy(db->name,buf);
  */

  db->mode = mode;
  db->db = sdbm_open(TclObjStr(objv[1]), mode, 0644);

  if (!db->db) {
        /*
         * error occurred
         * free previously allocated memory
         */
    /*ckfree ((char*) db->name);*/
    ckfree ((char*) db);
    db = (db_t*) NULL ;

    return NsfPrintError(in, "Open on '%s' failed with '%s'", 
			 TclObjStr(obj->cmdName), TclObjStr(objv[1]));
  } else {
    /*
     * success
     */
    NsfSetObjClientData(obj, (ClientData) db);
    return TCL_OK;
  }
}

static int
NsfSdbmCloseMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  db_t *db;
  Nsf_Object* obj = (Nsf_Object *) cd;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 1)
    return NsfObjErrArgCnt(in, obj->cmdName, "close");

  db = (db_t*) NsfGetObjClientData(obj);
  if (!db) {
    return NsfPrintError(in, "Called close on '%s', but database was not opened yet", 
			 TclObjStr(obj->cmdName));
  }
  sdbm_close (db->db);

  /*ckfree((char*)db->name);*/
  ckfree ((char*)db);
  NsfSetObjClientData(obj, 0);

  return TCL_OK;
}

static int
NsfSdbmNamesMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  Tcl_Obj *list;
  db_t *db;
  Tcl_DString result;
  datum key;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 1)
    return NsfObjErrArgCnt(in, obj->cmdName, "names");

  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfPrintError(in, "Called names on '%s', but database was not opened yet", 
			 TclObjStr(obj->cmdName));
  Tcl_DStringInit(&result);

  key = sdbm_firstkey(db->db);
  if (!key.dptr) {
    /* empty db */
    return TCL_OK ;
  }

  /*
   * copy key to result and go to next key
   */
  list = Tcl_NewListObj(0, NULL);
  do {
    Tcl_ListObjAppendElement(in,list,Tcl_NewStringObj(key.dptr,(int)(key.dsize-1)));
      key = sdbm_nextkey(db->db);
  } while (key.dptr);
  Tcl_SetObjResult(in, list);

  return TCL_OK;
}

static int
NsfSdbmSetMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  db_t *db;
  datum key, content;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc <2 || objc > 3)
    return NsfObjErrArgCnt(in, obj->cmdName, "set key ?value?");

  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfPrintError(in, "Called set on '%s', but database was not opened yet", 
			 TclObjStr(obj->cmdName));

  key.dptr = TclObjStr(objv[1]);
  key.dsize = objv[1]->length + 1;

  if (objc == 2) {
      /* get value */
      content = sdbm_fetch(db->db,key);
      if (content.dptr) {
	  /* found */
	Tcl_Obj *r = Tcl_NewStringObj(content.dptr, (int)(content.dsize-1));
	  Tcl_SetObjResult(in, r);
      } else {
	  /* key not found */
	  return NsfPrintError(in, "no such variable '%s'", key.dptr);
      }
  } else {
      /* set value */
      if (db->mode == O_RDONLY) {
	  return NsfPrintError(in, "Trying to set '%s', but database is in read mode", 
			       TclObjStr(obj->cmdName));
      }
      content.dptr = TclObjStr(objv[2]);
      content.dsize = objv[2]->length + 1;
      if (sdbm_store(db->db, key, content, SDBM_REPLACE) == 0) {
	  /*fprintf(stderr,"setting %s to '%s'\n",key.dptr,content.dptr);*/
	  Tcl_SetObjResult(in, objv[2]);
      } else {
	return NsfPrintError(in, "set of variable '%s' failed", TclObjStr(obj->cmdName));
      }
  }
  return TCL_OK;
}

static int
NsfSdbmExistsMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  db_t *db;
  datum key, content;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 2)
    return NsfObjErrArgCnt(in, obj->cmdName, "exists variable");

  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
      return NsfPrintError(in, "Called exists on '%s', but database was not opened yet", 
			   TclObjStr(obj->cmdName));

  key.dptr = TclObjStr(objv[1]);
  key.dsize = objv[1]->length + 1;

  content = sdbm_fetch(db->db,key);
  Tcl_SetIntObj(Tcl_GetObjResult(in), content.dptr != NULL);

  return TCL_OK;
}



static int
NsfSdbmUnsetMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  db_t *db;
  datum key;
  int ret;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 2)
    return NsfObjErrArgCnt(in, obj->cmdName, "unset key");

  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfPrintError(in, "Called unset on '%s', but database was not opened yet",
			 TclObjStr(obj->cmdName));
  /* check for read mode */
  if (db->mode == O_RDONLY) {
    return NsfPrintError(in, "Called unset on '%s', but database is in read mode", 
			 TclObjStr(obj->cmdName));
  }

  key.dptr = TclObjStr(objv[1]);
  key.dsize = objv[1]->length + 1;

  ret = sdbm_delete(db->db, key);

  if (ret == 0) {
    return TCL_OK;
  } else {
    return NsfPrintError(in, "Tried to unset '%s', but key does not exist", TclObjStr(objv[1]));
  }
}

/*
 * ndbm_firstkey
 */

static int
NsfSdbmFirstKeyMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  db_t *db;
  datum key;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 1)
    return NsfObjErrArgCnt(in, obj->cmdName, "firstkey");

  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfPrintError(in, "Called unset on '%s', but database was not opened yet",  
			 TclObjStr(obj->cmdName));


  key = sdbm_firstkey(db->db);
  if (!key.dptr) {
    /*
     * empty db
     */
    return TCL_OK;
  }

  Tcl_AppendResult (in, key.dptr, (char*)0);
  return TCL_OK;
}

static int
NsfSdbmNextKeyMethod(ClientData cd, Tcl_Interp* in, int objc, Tcl_Obj* CONST objv[]) {
  Nsf_Object* obj = (Nsf_Object *) cd;
  db_t *db;
  datum  newkey;

  if (!obj) return NsfObjErrType(in, obj->cmdName, "Object", "");
  if (objc != 1)
    return NsfObjErrArgCnt(in, obj->cmdName, "nextkey");

  db = (db_t*) NsfGetObjClientData(obj);
  if (!db)
    return NsfPrintError(in, "Called unset on '%s', but database was not opened yet", 
			 TclObjStr(obj->cmdName));

  newkey = sdbm_nextkey(db->db);

  if (!newkey.dptr) {
    /*
     * empty db
     */
    return TCL_OK ;
  }

  Tcl_AppendResult (in, newkey.dptr, (char*)0);
  return TCL_OK ;
}

/*
 * Xotclsdbm_Init
 * register commands, init data structures
 */

/* this should be done via the stubs ... for the time being
   simply export */
#ifdef VISUAL_CC
DLLEXPORT extern int Xotclsdbm_Init(Tcl_Interp * in);
#endif

extern int
Xotclsdbm_Init(Tcl_Interp * in) {
  Nsf_Class* cl;
  int result;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(in, "8.1", 0) == NULL) {
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
    Tcl_PkgProvide(in, "nsf::store::sdbm", PACKAGE_VERSION);

#ifdef PACKAGE_REQUIRE_NSF_FROM_SLAVE_INTERP_WORKS_NOW
    if (Tcl_PkgRequire(in, "XOTcl", XOTCLVERSION, 0) == NULL) {
        return TCL_ERROR;
    }
#endif
    if (Tcl_PkgRequire(in, "nsf::store", 0, 0) == NULL) {
        return TCL_ERROR;
    }
    result = Tcl_VarEval (in, "::nsf::Class create Storage=Sdbm -superclass Storage",
			  (char *) NULL);
    if (result != TCL_OK)
      return result;
    /*{
      Tcl_Obj *res = Tcl_GetObjResult(in);
      fprintf(stderr,"res='%s'\n", TclObjStr(res));
      cl = NsfGetClass(in, "Storage=Sdbm");
      fprintf(stderr,"cl=%p\n",cl);
      }*/

    cl = NsfGetClass(in, "Storage=Sdbm");
    if (!cl) {
      return TCL_ERROR;
    }

    NsfAddClassMethod(in, cl, "open", NsfSdbmOpenMethod, 0, 0);
    NsfAddClassMethod(in, cl, "close", NsfSdbmCloseMethod, 0, 0);
    NsfAddClassMethod(in, cl, "set", NsfSdbmSetMethod, 0, 0);
    NsfAddClassMethod(in, cl, "exists", NsfSdbmExistsMethod, 0, 0);
    NsfAddClassMethod(in, cl, "names", NsfSdbmNamesMethod, 0, 0);
    NsfAddClassMethod(in, cl, "unset", NsfSdbmUnsetMethod, 0, 0);
    NsfAddClassMethod(in, cl, "firstkey", NsfSdbmFirstKeyMethod, 0, 0);
    NsfAddClassMethod(in, cl, "nextkey", NsfSdbmNextKeyMethod, 0, 0);

    Tcl_SetIntObj(Tcl_GetObjResult(in), 1);
    return TCL_OK;
}

extern int
Xotclsdbm_SafeInit(interp)
    Tcl_Interp *interp;
{
    return Xotclsdbm_Init(interp);
}
