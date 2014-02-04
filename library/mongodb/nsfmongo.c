/*
 * Experimental Interface to MongoDB based on nsf (Next Scripting
 * Framework)
 *
 * The example shows how to use the source code generator from nsf to
 * generate a c interface.

 * This implementation provides a low level interface based on tagged
 * elements to force / preserve the datatypes of mongodb.
 *
 * This code serves as well as an example how to use the source code
 * generator of nsf to provide a C interface for optional nsf
 * packages.
 *
 * -gustaf neumann    March 27, 2011
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bson.h"
#include "mongo.h"
#include <gridfs.h>

#include <tcl.h>
#include <assert.h>
#include <nsf.h>

/*
 * Define the counters to generate nice symbols for pointer converter
 */
static int gridfsCount = 0;
static int gridfileCount = 0;
static int mongoCount = 0;
static int cursorCount = 0;

/***********************************************************************
 * The following definitions should not be here, but they are included
 * to get compilation going for the time being.
 ***********************************************************************/
typedef void *NsfObject;

#define PARSE_CONTEXT_PREALLOC 20
typedef struct {
  ClientData *clientData;
  int status;
  Tcl_Obj **objv;
  Tcl_Obj **full_objv;
  int *flags;
  ClientData clientData_static[PARSE_CONTEXT_PREALLOC];
  Tcl_Obj *objv_static[PARSE_CONTEXT_PREALLOC+1];
  int flags_static[PARSE_CONTEXT_PREALLOC+1];
  int lastobjc;
  int objc;
  int varArgs;
  NsfObject *object;
} ParseContext;

#define NSF_ARG_REQUIRED	0x000001

#define nr_elements(arr)  ((int) (sizeof(arr) / sizeof(arr[0])))
#define ObjStr(obj) (obj)->bytes ? (obj)->bytes : Tcl_GetString(obj)
#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif


#if defined(HAVE_STDINT_H)
# define HAVE_INTPTR_T
# define HAVE_UINTPTR_T
#endif

#if !defined(INT2PTR) && !defined(PTR2INT)
#   if defined(HAVE_INTPTR_T) || defined(intptr_t)
#	define INT2PTR(p) ((void *)(intptr_t)(p))
#	define PTR2INT(p) ((int)(intptr_t)(p))
#   else
#	define INT2PTR(p) ((void *)(p))
#	define PTR2INT(p) ((int)(p))
#   endif
#endif
#if !defined(UINT2PTR) && !defined(PTR2UINT)
#   if defined(HAVE_UINTPTR_T) || defined(uintptr_t)
#	define UINT2PTR(p) ((void *)(uintptr_t)(p))
#	define PTR2UINT(p) ((unsigned int)(uintptr_t)(p))
#   else
#	define UINT2PTR(p) ((void *)(p))
#	define PTR2UINT(p) ((unsigned int)(p))
#   endif
#endif


static int ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[],
                         NsfObject *obj, Tcl_Obj *procName,
                         Nsf_Param CONST *paramPtr, int nrParameters, int serial,
			 int doCheck, ParseContext *pc) {
  return Nsf_ArgumentParse(interp, objc, objv, (Nsf_Object *)obj,
			   procName, paramPtr, nrParameters, serial,
			   doCheck, (Nsf_ParseContext *)pc);
}

/***********************************************************************
 * Include the generated mongo db api.
 ***********************************************************************/

#include "mongoAPI.h"

/***********************************************************************
 * Helper functions
 ***********************************************************************/

/*
 *----------------------------------------------------------------------
 *
 * BsonToList --
 *
 *      Convert a bson structure to a tagged list. Each value field is
 *      preceded by a tag denoting its bson type.
 *
 * Results:
 *      Tagged list.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
Tcl_Obj *
BsonToList(Tcl_Interp *interp, const char *data , int depth) {
  bson_iterator i;
  const char *tag;
  char oidhex[25];
  Tcl_Obj *resultObj, *elemObj;

  bson_iterator_from_buffer( &i , data );
  resultObj = Tcl_NewListObj(0, NULL);

  while ( bson_iterator_next( &i ) ){
    bson_type t = bson_iterator_type( &i );
    const char *key;

    if ( t == 0 )
      break;
    key = bson_iterator_key( &i );
    /*fprintf(stderr, "BsonToList: key %s t %d string %d\n", key, t, bson_string);*/

    switch ( t ){
    case BSON_INT:    tag = "integer"; elemObj = Tcl_NewIntObj(bson_iterator_int( &i )); break;
    case BSON_LONG:   tag = "long";    elemObj = Tcl_NewLongObj(bson_iterator_long( &i )); break;
    case BSON_DATE:   tag = "date";    elemObj = Tcl_NewLongObj(bson_iterator_date( &i )); break;
    case BSON_DOUBLE: tag = "double";  elemObj = Tcl_NewDoubleObj(bson_iterator_double( &i )); break;
    case BSON_BOOL:   tag = "boolean"; elemObj = Tcl_NewBooleanObj(bson_iterator_bool( &i )); break;
    case BSON_REGEX:  tag = "regex";   elemObj = Tcl_NewStringObj(bson_iterator_regex( &i ), -1); break;
    case BSON_STRING: tag = "string";  elemObj = Tcl_NewStringObj(bson_iterator_string( &i ), -1); break;
    case BSON_MINKEY: tag = "minkey";  elemObj = Tcl_NewStringObj("null", 4); break;
    case BSON_MAXKEY: tag = "maxkey";  elemObj = Tcl_NewStringObj("null", 4); break;
    case BSON_NULL:   tag = "null";    elemObj = Tcl_NewStringObj("null", 4); break;
    case BSON_OID: {
      tag = "oid";
      bson_oid_to_string(bson_iterator_oid(&i), oidhex);
      elemObj = Tcl_NewStringObj(oidhex, -1);
      break;
    }
    case BSON_TIMESTAMP: {
      bson_timestamp_t ts;
      tag = "timestamp";
      ts = bson_iterator_timestamp( &i );
      elemObj = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewIntObj(ts.t));
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewIntObj(ts.i));
      break;
    }
    case BSON_OBJECT:
    case BSON_ARRAY:
      tag = t == BSON_OBJECT ? "object" : "array";
      elemObj = BsonToList(interp, bson_iterator_value( &i ) , depth + 1 );
      break;
    default:
      tag = "unknown";
      elemObj = Tcl_NewStringObj("", 0);
      NsfLog(interp, NSF_LOG_WARN, "BsonToList: unknown type %d", t);
    }

    Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj(key, -1));
    Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj(tag, -1));
    Tcl_ListObjAppendElement(interp, resultObj, elemObj);
  }

  return resultObj;
}

/*
 *----------------------------------------------------------------------
 *
 * BsonTagToType --
 *
 *      Convert a bson tag string to a bson_type. For the time being
 *      we compare as little as possible characters. In the future we
 *      might want to cache the bson tag in the Tcl_obj, maybe we can
 *      use Tcl_GetIndexFromObj();
 *
 * Results:
 *      bson_type.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
bson_type
BsonTagToType(Tcl_Interp *interp, char *tag) {
  char firstChar = *tag;

  switch (firstChar) {
  case 'a': /* array */   return BSON_ARRAY;
  case 'b': /* bool */    return BSON_BOOL;
  case 'd':
    if (*(tag + 1) == 'a') /* date   */ return BSON_DATE;
    if (*(tag + 1) == 'o') /* double */ return BSON_DOUBLE;
  case 'i': /* integer */ return BSON_INT;
  case 'l': /* long */    return BSON_LONG;
  case 'm':
    if  (*(tag + 1) == 'i') /* minkey */ return BSON_MINKEY;
    if  (*(tag + 1) == 'a') /* maxkey */ return BSON_MAXKEY;
    break;
  case 'n': /* null */    return BSON_NULL;
  case 'o':
    if  (*(tag + 1) == 'i') /* oid */ return BSON_OID;
    if  (*(tag + 1) == 'b') /* object */ return BSON_OBJECT;
    break;
  case 'r': /* regex */   return BSON_REGEX;
  case 's': /* string */  return BSON_STRING;
  case 't': /* timestamp */ return BSON_TIMESTAMP;
  }

  NsfLog(interp, NSF_LOG_WARN, "BsonTagToType: Treat unknown tag '%s' as string", tag);
  return BSON_STRING;
}

/*
 *----------------------------------------------------------------------
 *
 * BsonAppend --
 *
 *      append a tagged element to a bson buffer.
 *
 * Results:
 *      Tcl result code.
 *
 * Side effects:
 *      Value appended to bson buffer.
 *
 *----------------------------------------------------------------------
 */
static int
BsonAppend(Tcl_Interp *interp, bson *bbPtr, char *name, char *tag, Tcl_Obj *value) {
  int result = TCL_OK;
  bson_type t = BsonTagToType(interp, tag);

  /*fprintf(stderr, "BsonAppend: add name %s tag %s value '%s'\n", name, tag, ObjStr(value));*/

  switch ( t ){
  case BSON_STRING:
    bson_append_string(bbPtr, name, ObjStr(value));
    break;
  case BSON_INT: {
    int v;
    result = Tcl_GetIntFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_int(bbPtr, name, v);
    break;
  }
  case BSON_DOUBLE: {
    double v;
    result = Tcl_GetDoubleFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_double(bbPtr, name, v);
    break;
  }
  case BSON_BOOL: {
    int v;
    result = Tcl_GetBooleanFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_bool(bbPtr, name, v);
    break;
  }
  case BSON_LONG: {
    long v;
    result = Tcl_GetLongFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_long(bbPtr, name, v);
    break;
  }
  case BSON_MAXKEY:
    bson_append_maxkey(bbPtr, name);
    break;
  case BSON_MINKEY:
    bson_append_minkey(bbPtr, name);
    break;
  case BSON_NULL: {
    bson_append_null(bbPtr, name);
    break;
  }
  case BSON_OID: {
    bson_oid_t v;
    bson_oid_from_string(&v, ObjStr(value));
    bson_append_oid(bbPtr, name, &v);
    break;
  }
  case BSON_REGEX: {
    char *opts = ""; /* TODO: how to handle regex opts? */
    bson_append_regex(bbPtr, name, ObjStr(value), opts);
    break;
  }
  case BSON_DATE: {
    long v;
    result = Tcl_GetLongFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_date(bbPtr, name, v);
    break;
  }
  case BSON_TIMESTAMP: {
    bson_timestamp_t v;
    Tcl_Obj **objv;
    int objc = 0;
    result = Tcl_ListObjGetElements(interp, value, &objc, &objv);
    if (result != TCL_OK || objc != 2) {
      return NsfPrintError(interp, "invalid timestamp: %s", ObjStr(value));
    }
    result = Tcl_GetIntFromObj(interp, objv[0], &v.t);
    if (result == TCL_OK) {
      result = Tcl_GetIntFromObj(interp, objv[1], &v.i);
    }
    if (result != TCL_OK) break;
    bson_append_timestamp(bbPtr, name, &v);
    break;
  }
  case BSON_OBJECT:
  case BSON_ARRAY: {
    int i, objc;
    Tcl_Obj **objv;

    result = Tcl_ListObjGetElements(interp, value, &objc, &objv);
    if (result != TCL_OK || objc % 3 != 0) {
      return NsfPrintError(interp, "invalid %s value contain multiple of 3 elements %s", tag, ObjStr(value));
    }

    if (t == BSON_OBJECT) {
      bson_append_start_object(bbPtr, name);
    } else {
      bson_append_start_array(bbPtr, name);
    }
    for (i = 0; i< objc; i += 3) {
      /*fprintf(stderr, "value %s, i %d, [0]: %s, [1]: %s, [2]: %s\n", ObjStr(value), i,
	ObjStr(objv[i]),  ObjStr(objv[i+1]), ObjStr(objv[i+2]));*/
      result = BsonAppend(interp, bbPtr, ObjStr(objv[i]),  ObjStr(objv[i+1]), objv[i+2]);
      if (result != TCL_OK) break;
    }

    /*
     * finish_object works for arrays and objects
     */
    bson_append_finish_object(bbPtr);
    break;
  }

  case BSON_BINDATA:
  case BSON_DBREF:
  case BSON_CODE:
  case BSON_SYMBOL:
  case BSON_CODEWSCOPE:
    return NsfPrintError(interp, "tag %s not handled yet", tag);
    break;

  case BSON_UNDEFINED:
  case BSON_EOO:
    break;

    /* no default here, to get the warning to the compilation log for the time being */
  }
  return result;
}

/*
 *----------------------------------------------------------------------
 *
 * BsonAppendObjv --
 *
 *      Append all elements of objv to an uninitialized bson buffer.
 *
 * Results:
 *      Tcl result code.
 *
 * Side effects:
 *      Value appended to bson buffer.
 *
 *----------------------------------------------------------------------
 */
static int
BsonAppendObjv(Tcl_Interp *interp, bson *bPtr, int objc, Tcl_Obj **objv) {
  int i;

  bson_init(bPtr);
  for (i = 0; i < objc; i += 3) {
    char *name = ObjStr(objv[i]);
    char *tag = ObjStr(objv[i+1]);
    Tcl_Obj *value = objv[i+2];
    /*fprintf(stderr, "adding pair '%s' (%s) '%s'\n", name, tag, ObjStr(value));*/
    BsonAppend(interp, bPtr, name, tag, value);
  }
  bson_finish(bPtr);

  return TCL_OK;
}

static char *
ErrorMsg(int status) {

    switch (status) {
    case MONGO_CONN_NO_SOCKET:    return "Could not create socket";
    case MONGO_CONN_FAIL:         return "An error occured while calling connect()";
    case MONGO_CONN_ADDR_FAIL:    return "An error occured while calling getaddrinfo()";
    case MONGO_CONN_NOT_MASTER:   return "Connected to a non-master node (read-only)";
    case MONGO_CONN_BAD_SET_NAME: return "Given replica set name doesn't match this replica set";
    case MONGO_CONN_NO_PRIMARY:   return "Can't find primary in replica set";

    case MONGO_IO_ERROR:          return "An error occurred while reading or writing on the socket";
    case MONGO_READ_SIZE_ERROR:   return "The response is not the expected length";
    case MONGO_COMMAND_FAILED:    return "The command returned with 'ok' value of 0";
    case MONGO_BSON_INVALID:      return "BSON not valid for the specified op";
    case MONGO_BSON_NOT_FINISHED: return "BSON object has not been finished";

    default: return "Unknown error (maybe mongodb server not running)";
    }
}

/***********************************************************************
 * Define the api functions
 ***********************************************************************/

/*
cmd close NsfMongoClose {
  {-argName "conn" -required 1 -type mongo -withObj 1}
}
*/
static int
NsfMongoClose(Tcl_Interp *interp, mongo *connPtr, Tcl_Obj *connObj) {

  if (connPtr) {
    mongo_destroy(connPtr);
    Nsf_PointerDelete(ObjStr(connObj), connPtr, 1);
  }
  return TCL_OK;
}


/*
cmd connect NsfMongoConnect {
  {-argName "-replica-set" -required 0 -nrargs 1}
  {-argName "-server" -required 0 -nrargs 1 -type tclobj}
  {-argName "-timeout" -required 0 -nrargs 1 -type int32}
}
*/
static int
NsfMongoConnect(Tcl_Interp *interp, CONST char *replicaSet, Tcl_Obj *server, int withTimeout) {
  char channelName[80], *buffer = NULL;
  mongo_host_port host_port;
  int status, objc = 0;
  mongo *connPtr;
  Tcl_Obj **objv;

  if (server) {
    int result = Tcl_ListObjGetElements(interp, server, &objc, &objv);
    if (result != TCL_OK) {
      return NsfPrintError(interp, "The provided servers are not a well-formed list");
    }
  }

  connPtr = (mongo *)ckalloc(sizeof(mongo));

  if (objc == 0) {
    /*
     * No -server argument or an empty list was provided; use the
     * mongo default values.
     */
    status = mongo_client( connPtr, "127.0.0.1", 27017 );

  } else if (objc == 1 && replicaSet == NULL) {
    /*
     * A single element was provided to -server, we have no replica
     * set specified.
     */
    mongo_parse_host(ObjStr(objv[0]), &host_port);
    status = mongo_client( connPtr, host_port.host, host_port.port );
    if (buffer) {ckfree(buffer);}

  } else if (replicaSet) {
    /*
     * A list of 1 or more server was provided together with a replica
     * set.
     */
    int i;

    mongo_replset_init( connPtr, replicaSet );

    for (i = 0; i < objc; i++) {
      mongo_parse_host(ObjStr(objv[i]), &host_port);
      mongo_replset_add_seed(connPtr, host_port.host, host_port.port );
      if (buffer) {ckfree(buffer);}
    }

    status = mongo_replset_connect( connPtr );

  } else {
    ckfree((char *)connPtr);
    return NsfPrintError(interp, "A list of servers was provided, but not name for the replica set");
  }

  /*
   * Process the status from either mongo_connect() or
   * mongo_replset_connect().
   */
  if (status != MONGO_OK) {
    ckfree((char *)connPtr);
    return NsfPrintError(interp, ErrorMsg(status));
  }

  if (withTimeout > 0) {
    /*
     * setting connection timeout - measured in  milliseconds
     */
    if (mongo_set_op_timeout(connPtr, withTimeout) != MONGO_OK) {
      ckfree((char *)connPtr);
      return NsfPrintError(interp, "setting connection timeout failed");
    }
  }

  /*
   * Make an entry in the symbol table and return entry name it as
   * result.
   */
  Nsf_PointerAdd(interp, channelName, "mongo", connPtr);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(channelName, -1));

  return TCL_OK;
}

/*
cmd run NsfMongoRunCmd {
  {-argName "-nocomplain" -required 0 -nrargs 0}
  {-argName "conn" -required 1 -type mongo}
  {-argName "db" -required 1}
  {-argName "cmd" -required 1 -type tclobj}
}
*/
static int
NsfMongoRunCmd(Tcl_Interp *interp, int withNocomplain, mongo *connPtr, CONST char *db, Tcl_Obj *cmdObj) {
  int result, objc;
  Tcl_Obj **objv;
  bson cmd[1], out[1];

  result = Tcl_ListObjGetElements(interp, cmdObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(cmdObj));
  }
  BsonAppendObjv(interp, cmd, objc, objv);

  mongo_clear_errors( connPtr );
  result = mongo_run_command( connPtr, db, cmd, out );
  bson_destroy( cmd );

  if (withNocomplain == 0 && result != MONGO_OK) {
    fprintf(stderr, "run result %d\n", result);
    return NsfPrintError(interp, "mongo::run: command '%s' returned an unknown error", ObjStr(cmdObj));
  }

  Tcl_SetObjResult(interp, Tcl_NewIntObj(result == MONGO_OK));
  return TCL_OK;
}


/*
cmd query NsfMongoCount {
  {-argName "conn" -required 1 -type mongo}
  {-argName "namespace" -required 1}
  {-argName "query" -required 1 -type tclobj}
}
*/
static int
NsfMongoCount(Tcl_Interp *interp, mongo *connPtr, CONST char *namespace, Tcl_Obj *queryObj) {
  int objc, result;
  Tcl_Obj **objv;
  char *db, *collection;
  int count, length;
  bson query[1];

  result = Tcl_ListObjGetElements(interp, queryObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(queryObj));
  }

  BsonAppendObjv(interp, query, objc, objv);

  length = strlen(namespace)+1;
  db = ckalloc(length);
  memcpy(db, namespace, length);
  collection = strchr(db, '.');

  if (collection != NULL) {
    /* successful */
    *collection = '\0';
    collection ++;
    count = mongo_count(connPtr, db, collection, query);
  } else {
    count = 0;
  }

  bson_destroy( query );
  ckfree(db);

  Tcl_SetObjResult(interp, Tcl_NewIntObj(count));

  return TCL_OK;
}


/*
cmd index NsfMongoIndex {
  {-argName "conn" -required 1 -type mongo}
  {-argName "namespace" -required 1}
  {-argName "attributes" -required 1 -type tclobj}
  {-argName "-name" -required 0 -nrargs 1}
  {-argName "-background" -required 0 -nrargs 0}
  {-argName "-dropdups" -required 0 -nrargs 0}
  {-argName "-sparse" -required 0 -nrargs 0}
  {-argName "-ttl" -required 0 -nrargs 1 -type int32}
  {-argName "-unique" -required 0 -nrargs 0}
}
*/


static int
NsfMongoIndex(Tcl_Interp *interp,
	      mongo *connPtr,
	      CONST char *namespace,
	      Tcl_Obj *attributesObj,
	      CONST char *withName,
	      int withBackground,
	      int withDropdups,
	      int withSparse,
	      int withTtl,
	      int withUnique) {
  bson_bool_t success;
  int objc, result, options = 0;
  Tcl_Obj **objv;
  bson keys[1], out[1];

  result = Tcl_ListObjGetElements(interp, attributesObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(attributesObj));
  }
  BsonAppendObjv(interp, keys, objc, objv);
  if (withBackground) {options |= MONGO_INDEX_BACKGROUND;}
  if (withDropdups)   {options |= MONGO_INDEX_DROP_DUPS;}
  if (withSparse)     {options |= MONGO_INDEX_SPARSE;}
  if (withUnique)     {options |= MONGO_INDEX_UNIQUE;}

  success = mongo_create_index(connPtr, namespace, keys, withName, options, withTtl, out);
  bson_destroy(keys);
  /* TODO: examples in mongo-client do not touch out; do we have to do
     something about it? */

  Tcl_SetObjResult(interp, Tcl_NewBooleanObj(success == MONGO_OK));
  return TCL_OK;
}



/*
cmd insert NsfMongoInsert {
  {-argName "conn" -required 1 -type mongo}
  {-argName "namespace" -required 1}
  {-argName "values" -required 1 -type tclobj}
}
*/
static int NsfMongoInsert(Tcl_Interp *interp, mongo *connPtr, CONST char *namespace, Tcl_Obj *valuesObj) {
  int i, objc, result;
  Tcl_Obj **objv;
  bson b[1];

  result = Tcl_ListObjGetElements(interp, valuesObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(valuesObj));
  }

  //bson_init(buf);
  bson_init(b);
  bson_append_new_oid(b, "_id");

  for (i = 0; i < objc; i += 3) {
    char *name = ObjStr(objv[i]);
    char *tag = ObjStr(objv[i+1]);
    Tcl_Obj *value = objv[i+2];
    /*fprintf(stderr, "adding pair '%s' (%s) '%s'\n", name, tag, ObjStr(value));*/
    BsonAppend(interp, b, name, tag, value);
  }

  bson_finish(b);
  /* for the time being, no write_concern (last arg of mongo_insert()) */
  result = mongo_insert(connPtr, namespace, b, NULL);

  if (result == MONGO_ERROR) {
    result = NsfPrintError(interp, ErrorMsg(connPtr->err));
  } else {
    Tcl_Obj *resultObj = BsonToList(interp, b->data, 0);
    Tcl_SetObjResult(interp, resultObj);
    result = TCL_OK;
  }
  bson_destroy(b);
  return result;
}

/*
cmd query NsfMongoQuery {
  {-argName "conn" -required 1 -type mongo}
  {-argName "namespace" -required 1}
  {-argName "query" -required 1 -type tclobj}
  {-argName "-atts" -required 0 -nrargs 1 -type tclobj}
  {-argName "-limit" -required 0 -type int32}
  {-argName "-skip" -required 0 -type int32}
}
*/
static int
NsfMongoQuery(Tcl_Interp *interp, mongo *connPtr, CONST char *namespace,
	      Tcl_Obj *queryObj, Tcl_Obj *withAttsObj,
	      int withLimit, int withSkip) {
  int objc1, objc2, result;
  Tcl_Obj **objv1, **objv2, *resultObj;
  mongo_cursor *cursor;
  bson query[1];
  bson atts[1];

  /*fprintf(stderr, "NsfMongoQuery: namespace %s withLimit %d withSkip %d\n",
    namespace, withLimit, withSkip);*/

  result = Tcl_ListObjGetElements(interp, queryObj, &objc1, &objv1);
  if (result != TCL_OK || (objc1 % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(queryObj));
  }
  if (withAttsObj) {
    result = Tcl_ListObjGetElements(interp, withAttsObj, &objc2, &objv2);
    if (result != TCL_OK || (objc2 % 3 != 0)) {
      return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(withAttsObj));
    }
  } else {
    objc2 = 0;
  }

  /* fprintf(stderr, "query # %d, atts # %d\n", objc1, objc2); */
  BsonAppendObjv(interp, query, objc1, objv1);
  BsonAppendObjv(interp, atts, objc2, objv2);

  resultObj = Tcl_NewListObj(0, NULL);

  /*
   *  The last field of mongo_find is options, semantics are described here
   *  http://www.mongodb.org/display/DOCS/Mongo+Wire+Protocol#MongoWireProtocol-OPQUERY
   */
  cursor = mongo_find( connPtr, namespace, query, atts, withLimit, withSkip, 0 );
  while( mongo_cursor_next( cursor ) == MONGO_OK ) {
    Tcl_ListObjAppendElement(interp, resultObj, BsonToList(interp, (&cursor->current)->data, 0));
  }

  mongo_cursor_destroy( cursor );
  bson_destroy( query );
  bson_destroy( atts );

  Tcl_SetObjResult(interp, resultObj);

  return TCL_OK;
}

/*
cmd remove NsfMongoRemove {
  {-argName "conn" -required 1 -type mongo}
  {-argName "namespace" -required 1}
  {-argName "condition" -required 1 -type tclobj}
}
*/
static int
NsfMongoRemove(Tcl_Interp *interp, mongo *connPtr, CONST char *namespace, Tcl_Obj *conditionObj) {
  int objc, result, status;
  Tcl_Obj **objv;
  bson query[1];

  result = Tcl_ListObjGetElements(interp, conditionObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(conditionObj));
  }

  BsonAppendObjv(interp, query, objc, objv);
  /* for the time being, no write_concern (last arg of mongo_remove()) */
  status = mongo_remove(connPtr, namespace, query, NULL);

  Tcl_SetObjResult(interp, Tcl_NewIntObj(status == MONGO_OK));

  bson_destroy(query);
  return TCL_OK;
}

/*
cmd insert NsfMongoUpdate {
  {-argName "conn" -required 1 -type mongo}
  {-argName "namespace" -required 1}
  {-argName "cond" -required 1 -type tclobj}
  {-argName "values" -required 1 -type tclobj}
  {-argName "-upsert" -required 0 -nrargs 0}
  {-argName "-all" -required 0 -nrargs 0}
}
*/
static int
NsfMongoUpdate(Tcl_Interp *interp, mongo *connPtr, CONST char *namespace,
	       Tcl_Obj *conditionObj, Tcl_Obj *valuesObj, int withUpsert, int withAll) {
  int objc, result, mongorc, options = 0;
  Tcl_Obj **objv;
  bson cond[1], values[1];

  result = Tcl_ListObjGetElements(interp, conditionObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(conditionObj));
  }

  BsonAppendObjv(interp, cond, objc, objv);

  result = Tcl_ListObjGetElements(interp, valuesObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    bson_destroy(cond);
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(valuesObj));
  }

  BsonAppendObjv(interp, values, objc, objv);

  if (withUpsert) {options |= 1;}
  if (withAll) {options |= 2;}

  /* for the time being, no write_concern (last arg of mongo_update()) */
  mongorc = mongo_update(connPtr, namespace, cond, values, options, NULL);

  Tcl_SetObjResult(interp, Tcl_NewBooleanObj(mongorc == MONGO_OK));

  return TCL_OK;
}

/***********************************************************************
 * Cursor interface
 ***********************************************************************/
/*
cmd cursor::find NsfMongoCursorFind {
  {-argName "conn" -required 1 -type mongo}
  {-argName "namespace" -required 1}
  {-argName "query" -required 1 -type tclobj}
  {-argName "-atts" -required 0 -nrargs 1 -type tclobj}
  {-argName "-limit" -required 0 -type int32}
  {-argName "-skip" -required 0 -type int32}
  {-argName "-tailable" -required 0 -nrargs 0}
  {-argName "-awaitdata" -required 0 -nrargs 0}
}
*/
static int
NsfMongoCursorFind(Tcl_Interp *interp, mongo *connPtr, CONST char *namespace,
	      Tcl_Obj *queryObj, Tcl_Obj *withAttsObj,
		   int withLimit, int withSkip,
		   int withTailable, int withAwaitdata) {
  int objc1, objc2, result, options = 0;
  Tcl_Obj **objv1, **objv2;
  mongo_cursor *cursor;
  bson query[1];
  bson atts[1];

  /*fprintf(stderr, "NsfMongoQuery: namespace %s withLimit %d withSkip %d\n",
    namespace, withLimit, withSkip);*/

  result = Tcl_ListObjGetElements(interp, queryObj, &objc1, &objv1);
  if (result != TCL_OK || (objc1 % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(queryObj));
  }
  if (withAttsObj) {
    result = Tcl_ListObjGetElements(interp, withAttsObj, &objc2, &objv2);
    if (result != TCL_OK || (objc2 % 3 != 0)) {
      return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(withAttsObj));
    }
  } else {
    objc2 = 0;
  }

  BsonAppendObjv(interp, query, objc1, objv1);
  BsonAppendObjv(interp, atts, objc2, objv2);

  /*
   *  The last field of mongo_find is options, semantics are described here
   *  http://www.mongodb.org/display/DOCS/Mongo+Wire+Protocol#MongoWireProtocol-OPQUERY
   */
  if (withTailable) {
    options |= MONGO_TAILABLE;
  }
  if (withAwaitdata) {
    options |= MONGO_AWAIT_DATA;
  }
  cursor = mongo_find( connPtr, namespace, query, atts, withLimit, withSkip, options);

  if (cursor) {
    char buffer[80];
    Nsf_PointerAdd(interp, buffer, "mongo_cursor", cursor);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
  } else {
    Tcl_ResetResult(interp);
  }

  bson_destroy( query );
  bson_destroy( atts );

  return TCL_OK;
}

/*
cmd cursor::next NsfMongoCursorNext {
  {-argName "cursor" -required 1 -type mongo_cursor}
}
*/
static int
NsfMongoCursorNext(Tcl_Interp *interp, mongo_cursor *cursor) {
  int result;

  result = mongo_cursor_next( cursor );
  if (result == MONGO_OK) {
    Tcl_SetObjResult(interp, BsonToList(interp, (&cursor->current)->data, 0));
  }
  return TCL_OK;
}

/*
cmd cursor::close NsfMongoCursorClose {
  {-argName "cursor" -required 1 -type mongo_cursor -withObj 1}
}
*/
static int
NsfMongoCursorClose(Tcl_Interp *interp, mongo_cursor *cursor, Tcl_Obj *cursorObj) {

  mongo_cursor_destroy( cursor );
  Nsf_PointerDelete(ObjStr(cursorObj), cursor, 0);

  return TCL_OK;
}



/***********************************************************************
 * GridFS interface
 ***********************************************************************/
/*
cmd gridfs::open NsfMongoGridFSOpen {
  {-argName "conn" -required 1 -type mongo}
  {-argName "dbname" -required 1}
  {-argName "prefix" -required 1}
}
*/

static int
NsfMongoGridFSOpen(Tcl_Interp *interp, mongo *connPtr,
		   CONST char *dbname, CONST char *prefix) {
  char buffer[80];
  gridfs *gfsPtr;

  gfsPtr = (gridfs *)ckalloc(sizeof(gridfs));
  gridfs_init(connPtr, dbname, prefix, gfsPtr);

  Nsf_PointerAdd(interp, buffer, "gridfs", gfsPtr);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));

  return TCL_OK;
}


/*
cmd gridfs::remove_file NsfMongoGridFSRemoveFile {
  {-argName "gfs" -required 1 -type tclobj}
  {-argName "filename" -required 1}
}
*/
static int
NsfMongoGridFSRemoveFile(Tcl_Interp *interp, gridfs *gridfsPtr,
			CONST char *filename) {
  int status;

  /* the current interfaces does not return a status ! */
  status = gridfs_remove_filename(gridfsPtr, filename);

  Tcl_SetObjResult(interp, Tcl_NewIntObj(status == MONGO_OK));

  return TCL_OK;
}

/*
cmd gridfs::store_file NsfMongoGridFSStoreFile {
  {-argName "gfs" -required 1 -type gridfs}
  {-argName "filename" -required 1}
  {-argName "remotename" -required 1}
  {-argName "contenttype" -required 1}
}
*/
static int
NsfMongoGridFSStoreFile(Tcl_Interp *interp, gridfs *gridfsPtr,
			CONST char *filename, CONST char *remotename,
			CONST char *contenttype) {
  int flags = 0;  // TODO: add/handle flags
  int result = gridfs_store_file(gridfsPtr, filename, remotename, contenttype, flags);

  /* currently, we do not get the bson structure;
     Tcl_SetObjResult(interp, BsonToList(interp, b.data, 0));*/

  Tcl_SetObjResult(interp, Tcl_NewIntObj(result == MONGO_OK));

  return TCL_OK;
}

/*
cmd gridfs::close NsfMongoGridFSClose {
  {-argName "gfs" -required 1 -type gridfs -withObj 1}
}
*/
static int
NsfMongoGridFSClose(Tcl_Interp *interp, gridfs *gridfsPtr, Tcl_Obj *gridfsObj) {

  gridfs_destroy(gridfsPtr);
  Nsf_PointerDelete(ObjStr(gridfsObj), gridfsPtr, 1);

  return TCL_OK;
}

/***********************************************************************
 * GridFile interface
 *
 * Currently offsets and sizes are limited to 32bit integers, we should
 * relax this later.
 ***********************************************************************/

/*
cmd gridfile::close NsfMongoGridFileClose {
  {-argName "file" -required 1 -type gridfile -withObj 1}
}
*/
static int
NsfMongoGridFileClose(Tcl_Interp *interp, gridfile* gridFilePtr, Tcl_Obj *gridFileObj) {

  gridfile_destroy(gridFilePtr);
  Nsf_PointerDelete(ObjStr(gridFileObj), gridFilePtr, 1);

  return TCL_OK;
}

/*
cmd gridfile::get_contentlength NsfMongoGridFileGetContentlength {
  {-argName "gridfile" -required 1 -type gridfile}
}
*/
static int
NsfMongoGridFileGetContentlength(Tcl_Interp *interp, gridfile* gridFilePtr) {
  gridfs_offset len;

  len = gridfile_get_contentlength(gridFilePtr);
  Tcl_SetObjResult(interp, Tcl_NewLongObj(len));

  return TCL_OK;
}

/*
cmd gridfile::get_contenttype NsfMongoGridFileGetContentType {
  {-argName "gridfile" -required 1 -type gridfile}
}
*/
static int
NsfMongoGridFileGetContentType(Tcl_Interp *interp, gridfile* gridFilePtr) {
  CONST char *contentType;

  contentType = gridfile_get_contenttype(gridFilePtr);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(contentType, -1));

  return TCL_OK;
}

/*
cmd gridfile::get_metadata NsfMongoGridFileGetMetaData {
  {-argName "gridfile" -required 1 -type tclgridfile* gridFilePtrobj}
}
*/
static int
NsfMongoGridFileGetMetaData(Tcl_Interp *interp, gridfile* gridFilePtr) {
  bson b;
  bson_bool_t copyData = 0; // TODO: what does this

  gridfile_get_metadata(gridFilePtr, &b, copyData);
  Tcl_SetObjResult(interp, BsonToList(interp, b.data, 0));

  return TCL_OK;
}

/*
cmd gridfile::open NsfMongoGridFileOpen {
  {-argName "gfs" -required 1 -type gridfs}
  {-argName "filename" -required 1}
}
*/
static int
NsfMongoGridFileOpen(Tcl_Interp *interp, gridfs *gridfsPtr, CONST char *filename) {
  gridfile* gridFilePtr;
  int result;

  gridFilePtr = (gridfile *)ckalloc(sizeof(gridfile));
  result = gridfs_find_filename(gridfsPtr, filename, gridFilePtr);

  if (result == MONGO_OK) {
    char buffer[80];
    Nsf_PointerAdd(interp, buffer, "gridfile", gridFilePtr);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
  } else {
    ckfree((char *)gridFilePtr);
    Tcl_ResetResult(interp);
  }

  return TCL_OK;
}

/*
cmd gridfile::read NsfMongoGridFileRead {
  {-argName "gridfile" -required 1 -type gridfile}
  {-argName "size" -required 1 -type int}
}
*/
static int
NsfMongoGridFileRead(Tcl_Interp *interp, gridfile *gridFilePtr, int size) {
  int readSize;
  char *buffer;

  buffer = ckalloc(size);
  readSize = gridfile_read_buffer(gridFilePtr, buffer, size);
  Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, readSize));
  ckfree(buffer);

  return TCL_OK;
}

/*
cmd "gridfile::seek" NsfMongoGridFileSeek {
  {-argName "file" -required 1 -type gridfile}
  {-argName "offset" -required 1 -type int32}
}
*/
static int
NsfMongoGridFileSeek(Tcl_Interp *interp, gridfile *gridFilePtr, int offset) {
  int pos;

  pos = gridfile_seek(gridFilePtr, offset);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(pos));

  return TCL_OK;
}

/***********************************************************************
 * Finally, provide the necessary Tcl package interface.
 ***********************************************************************/

void
Nsfmongo_Exit(ClientData clientData) {
  /*
   * The exit might happen at a time, when tcl is already shut down.
   * We can't reliably call NsfLog.
   *
   *   Tcl_Interp *interp = (Tcl_Interp *)clientData;
   *   NsfLog(interp,NSF_LOG_NOTICE, "Nsfmongo Exit");
   */
}

extern int
Nsfmongo_Init(Tcl_Interp * interp) {
  int i;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
        return TCL_ERROR;
    }

# ifdef USE_NSF_STUBS
    if (Nsf_InitStubs(interp, "2.0", 0) == NULL) {
        return TCL_ERROR;
    }
# endif

#else
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 0) == NULL) {
        return TCL_ERROR;
    }
#endif
    Tcl_PkgProvide(interp, "nsf::mongo", PACKAGE_VERSION);

#ifdef PACKAGE_REQUIRE_FROM_SLAVE_INTERP_WORKS_NOW
    if (Tcl_PkgRequire(interp, "nsf", PACKAGE_VERSION, 0) == NULL) {
        return TCL_ERROR;
    }
#endif

    Tcl_CreateExitHandler(Nsfmongo_Exit, interp);

    /*
     * register the pointer converter
     */
    Nsf_PointerTypeRegister(interp, "gridfs",       &gridfsCount);
    Nsf_PointerTypeRegister(interp, "gridfile",     &gridfileCount);
    Nsf_PointerTypeRegister(interp, "mongo",        &mongoCount);
    Nsf_PointerTypeRegister(interp, "mongo_cursor", &cursorCount);

    /* create all method commands (will use the namespaces above) */
    for (i=0; i < nr_elements(method_definitions)-1; i++) {
      Tcl_CreateObjCommand(interp, method_definitions[i].methodName, method_definitions[i].proc, 0, 0);
    }

    Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);
    return TCL_OK;
}

extern int
Nsfmongo_SafeInit(interp)
    Tcl_Interp *interp;
{
    return Nsfmongo_Init(interp);
}
