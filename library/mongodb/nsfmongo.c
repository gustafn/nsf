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

#include <tcl.h>
#include <assert.h>
#include <nsf.h>

static Tcl_HashTable mongoConnsHashTable, *mongoConnsHashTablePtr = &mongoConnsHashTable;
static NsfMutex mongoMutex = 0;
static int mongoConns = 0;

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
                         Nsf_Param *paramPtr, int nrParameters, int doCheck,
                         ParseContext *pc) {
  return Nsf_ArgumentParse(interp, objc, objv, (Nsf_Object *)obj, 
			   procName, paramPtr, nrParameters, 
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
 * MongoGetConn --
 *
 *      Obtain a mongo connection from the hash table key returned via
 *      NsfMongoConnect.
 *
 * Results:
 *      mongo connection or NULL if not found/invalid.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
mongo_connection *
MongoGetConn(Tcl_Obj *connObj) {
  mongo_connection *connPtr = NULL;
  Tcl_HashEntry *hPtr;
  
  NsfMutexLock(&mongoMutex);
  hPtr = Tcl_CreateHashEntry(mongoConnsHashTablePtr, ObjStr(connObj), NULL);

  if (hPtr) {
    connPtr = Tcl_GetHashValue(hPtr);
  }
  NsfMutexUnlock(&mongoMutex);

  return connPtr;
}

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
  const char *key, *tag;
  bson_timestamp_t ts;
  char oidhex[25];
  Tcl_Obj *resultObj, *elemObj;

  bson_iterator_init( &i , data );
  resultObj = Tcl_NewListObj(0, NULL);

  while ( bson_iterator_next( &i ) ){
    bson_type t = bson_iterator_type( &i );

    if ( t == 0 )
      break;
    key = bson_iterator_key( &i );
    /*fprintf(stderr, "key %s t %d string %d\n", key, t, bson_string);*/

    switch ( t ){
    case bson_int:    tag = "integer"; elemObj = Tcl_NewIntObj(bson_iterator_int( &i )); break;
    case bson_long:   tag = "long";    elemObj = Tcl_NewIntObj(bson_iterator_long( &i )); break;
    case bson_double: tag = "double";  elemObj = Tcl_NewDoubleObj(bson_iterator_double( &i )); break;
    case bson_bool:   tag = "boolean"; elemObj = Tcl_NewBooleanObj(bson_iterator_bool( &i )); break;
    case bson_regex:  tag = "regex";   elemObj = Tcl_NewStringObj(bson_iterator_regex( &i ), -1); break;
    case bson_string: tag = "string";  elemObj = Tcl_NewStringObj(bson_iterator_string( &i ), -1); break;
    case bson_null:   tag = "null";    elemObj = Tcl_NewStringObj("null", 4); break;
    case bson_oid: {
      tag = "oid";
      bson_oid_to_string(bson_iterator_oid(&i), oidhex); 
      elemObj = Tcl_NewStringObj(oidhex, -1);
      break;
    }
    case bson_timestamp:
      tag = "timestamp";
      ts = bson_iterator_timestamp( &i );
      elemObj = Tcl_NewListObj(0, NULL);
      Tcl_AppendObjToObj(elemObj, Tcl_NewIntObj(ts.i));
      Tcl_AppendObjToObj(elemObj, Tcl_NewIntObj(ts.t));
      break;
    case bson_object: 
    case bson_array:
      tag = t == bson_object ? "object" : "array";
      elemObj = BsonToList(interp, bson_iterator_value( &i ) , depth + 1 );
      break;
    default:
      tag = "unknown";
      elemObj = Tcl_NewStringObj("", 0);
      fprintf( stderr , "unknown type : %d\n" , t );
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
  case 'a': /* array */   return bson_array;
  case 'b': /* bool */    return bson_bool;
  case 'd': /* double */  return bson_double;
  case 'i': /* integer */ return bson_int;
  case 'l': /* long */    return bson_long;
  case 'n': /* null */    return bson_null;
  case 'o': 
    if  (*(tag + 1) == 'i') /* oid */ return bson_oid;
    if  (*(tag + 1) == 'b') /* object */ return bson_object;
    break;
  case 'r': /* regex */   return bson_regex;
  case 's': /* string */  return bson_string;
  case 't': /* timestamp */ return bson_timestamp;
  }

  NsfLog(interp, NSF_LOG_WARN, "Treat unknown tag '%s' as string", tag);
  return bson_string;
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
BsonAppend(Tcl_Interp *interp, bson_buffer *bbPtr, char *name, char *tag, Tcl_Obj *value) {
  int result = TCL_OK;
  bson_type t = BsonTagToType(interp, tag);

  /*fprintf(stderr, "add name %s tag %s value '%s'\n", name, tag, ObjStr(value));*/

  switch ( t ){
  case bson_string: 
    bson_append_string(bbPtr, name, ObjStr(value)); 
    break;
  case bson_int: {
    int v;
    result = Tcl_GetIntFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_int(bbPtr, name, v);
    break;
  }
  case bson_double: {
    double v;
    result = Tcl_GetDoubleFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_double(bbPtr, name, v);
    break;
  }
  case bson_bool: {
    int v;
    result = Tcl_GetBooleanFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_bool(bbPtr, name, v);
    break;
  }
  case bson_long: {
    long v;
    result = Tcl_GetLongFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_long(bbPtr, name, v);
    break;
  }
  case bson_null: {
    bson_append_null(bbPtr, name);
    break;
  }
  case bson_oid: {
    bson_oid_t v;
    bson_oid_from_string(&v, ObjStr(value));
    bson_append_oid(bbPtr, name, &v);
    break;
  }
  case bson_regex: {
    char *opts = ""; /* TODO: how to handle regex opts? */
    bson_append_regex(bbPtr, name, ObjStr(value), opts);
    break;
  }
  case bson_timestamp: {
    bson_timestamp_t v;
    Tcl_Obj **objv;
    int objc = 0;
    result = Tcl_ListObjGetElements(interp, value, &objc, &objv);
    if (result != TCL_OK || objc != 2) {
      return NsfPrintError(interp, "invalid timestamp: %s", ObjStr(value));
    }
    result = Tcl_GetIntFromObj(interp, objv[0], &v.i);
    if (result == TCL_OK) {
      result = Tcl_GetIntFromObj(interp, objv[1], &v.t);
    }
    if (result != TCL_OK) break;
    bson_append_timestamp(bbPtr, name, &v);
    break;
  }
  case bson_object: 
  case bson_array: {
    int i, objc;
    Tcl_Obj **objv;

    bson_buffer *(*bsonStartFn)( bson_buffer *b, const char *name);
    bsonStartFn = (t == bson_object) ? bson_append_start_object : bson_append_start_array;

    result = Tcl_ListObjGetElements(interp, value, &objc, &objv);
    if (result != TCL_OK || objc % 3 != 0) {
      return NsfPrintError(interp, "invalid %s value contain multiple of 3 elements %s", tag, ObjStr(value));
    }

    (*bsonStartFn)( bbPtr, name);

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
  bson_buffer buf[1];

  bson_buffer_init(buf);
  for (i = 0; i < objc; i += 3) {
    char *name = ObjStr(objv[i]);
    char *tag = ObjStr(objv[i+1]);
    Tcl_Obj *value = objv[i+2];
    /*fprintf(stderr, "adding pair '%s' (%s) '%s'\n", name, tag, ObjStr(value));*/
    BsonAppend(interp, buf, name, tag, value);
  }

  bson_from_buffer(bPtr, buf);
  
  return TCL_OK;
}

/***********************************************************************
 * Define the api functions
 ***********************************************************************/

/*
cmd close NsfMongoClose {
  {-argName "conn" -required 1 -type tclobj}
}
*/
static int 
NsfMongoClose(Tcl_Interp *interp, Tcl_Obj *connObj) {
  mongo_connection *connPtr = MongoGetConn(connObj);

  if (connPtr) {
    Tcl_HashEntry *hPtr;

    mongo_destroy(connPtr);
    ckfree((char *)connPtr);

    NsfMutexLock(&mongoMutex);
    hPtr = Tcl_CreateHashEntry(mongoConnsHashTablePtr, ObjStr(connObj), NULL);
    Tcl_DeleteHashEntry(hPtr);
    NsfMutexUnlock(&mongoMutex);
  }
  return TCL_OK;
}

/*
cmd connect NsfMongoConnect {
  {-argName "-host" -required 0 -nrargs 1}
  {-argName "-port" -required 0 -nrargs 1 -type int}
}
*/
static int 
NsfMongoConnect(Tcl_Interp *interp, CONST char *host, int port) {
  Tcl_HashEntry *hPtr;
  char channelName[80];
  int isNew;
  mongo_connection *connPtr;
  mongo_connection_options opts[1];
  mongo_conn_return status;

  strcpy(opts->host , host ? host : "127.0.0.1");
  opts->port = port != 0 ? port : 27017;
  connPtr = (mongo_connection *)ckalloc(sizeof(mongo_connection));

  status = mongo_connect( connPtr, opts );
  if (status != mongo_conn_success) {
    char *errorMsg;

    ckfree((char*)connPtr);

    switch (status) {
    case mongo_conn_bad_arg:    errorMsg = "bad arguments"; break;
    case mongo_conn_no_socket:  errorMsg = "no socket"; break;
    case mongo_conn_fail:       errorMsg = "connection failed"; break;
    case mongo_conn_not_master: errorMsg = "not master"; break;
    default: errorMsg = "unknown Error"; break;
    }
    return NsfPrintError(interp, errorMsg);
  }

  NsfMutexLock(&mongoMutex);
  sprintf(channelName, "mongo_conn%d", mongoConns++);  
  hPtr = Tcl_CreateHashEntry(mongoConnsHashTablePtr, channelName, &isNew);
  NsfMutexUnlock(&mongoMutex);
  Tcl_SetHashValue(hPtr, connPtr);

  Tcl_SetObjResult(interp, Tcl_NewStringObj(channelName, -1));
  return TCL_OK;
}

/*
cmd index NsfMongoIndex {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "attributes" -required 1 -type tclobj}
  {-argName "-dropdups" -required 0 -nrargs 0}
  {-argName "-unique" -required 0 -nrargs 0}
}
*/
static int 
NsfMongoIndex(Tcl_Interp *interp, Tcl_Obj *connObj, CONST char *namespace, Tcl_Obj *attributesObj, 
	      int withDropdups, int withUnique) {
  mongo_connection *connPtr = MongoGetConn(connObj);
  bson_bool_t success;
  int objc, result, options = 0;
  Tcl_Obj **objv;
  bson keys[1], out[1];

  if (connPtr == NULL)  {
    return NsfObjErrType(interp, "", connObj, "connection", NULL);
  }
  result = Tcl_ListObjGetElements(interp, attributesObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(attributesObj));
  }
  BsonAppendObjv(interp, keys, objc, objv);
  if (withDropdups) {options |= MONGO_INDEX_DROP_DUPS;}
  if (withUnique) {options |= MONGO_INDEX_UNIQUE;}

  success = mongo_create_index(connPtr, namespace, keys, options, out);
  bson_destroy(keys);
  /* TODO: examples in mongo-client do not touch out; do we have to do
     something about it? */

  Tcl_SetObjResult(interp, Tcl_NewBooleanObj(success));
  return TCL_OK;
}



/*
cmd insert NsfMongoInsert {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "values" -required 1 -type tclobj}
}
*/
static int NsfMongoInsert(Tcl_Interp *interp, Tcl_Obj *connObj, CONST char *namespace, Tcl_Obj *valuesObj) {
  mongo_connection *connPtr = MongoGetConn(connObj);
  int i, objc, result;
  Tcl_Obj **objv;
  bson_buffer buf[1];
  bson b[1];

  if (connPtr == NULL)  {
    return NsfObjErrType(interp, "", connObj, "connection", NULL);
  }

  result = Tcl_ListObjGetElements(interp, valuesObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(valuesObj));
  }
    
  bson_buffer_init(buf);
  bson_append_new_oid(buf, "_id");

  for (i = 0; i < objc; i += 3) {
    char *name = ObjStr(objv[i]);
    char *tag = ObjStr(objv[i+1]);
    Tcl_Obj *value = objv[i+2];
    /*fprintf(stderr, "adding pair '%s' (%s) '%s'\n", name, tag, ObjStr(value));*/
    BsonAppend(interp, buf, name, tag, value);
  }

  bson_from_buffer( b, buf );
  mongo_insert(connPtr, namespace, b);
  bson_destroy(b);

  return TCL_OK;
}

/*
cmd query NsfMongoQuery {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "query" -required 1 -type tclobj}
  {-argName "-limit" -required 0 -type int}
  {-argName "-skip" -required 0 -type int}
}
*/
static int 
NsfMongoQuery(Tcl_Interp *interp, Tcl_Obj *connObj, CONST char *namespace, Tcl_Obj *queryObj, 
	      int withLimit, int withSkip) {
  int objc, result;
  Tcl_Obj **objv, *resultObj;
  mongo_connection *connPtr = MongoGetConn(connObj);
  mongo_cursor *cursor;
  bson query[1];
  bson empty[1];

  if (connPtr == NULL)  {
    return NsfObjErrType(interp, "", connObj, "connection", NULL);
  }

  result = Tcl_ListObjGetElements(interp, queryObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(queryObj));
  }

  BsonAppendObjv(interp, query, objc, objv);
  bson_empty( empty );
  resultObj = Tcl_NewListObj(0, NULL);

  /* 
   *  The la񓩌st field of mongo_find is options, semantics are described here
   *  http://www.mongodb.org/display/DOCS/Mongo+Wire+Protocol#MongoWireProtocol-OPQUERY
   */
  cursor = mongo_find( connPtr, namespace, query, NULL, withLimit, withSkip, 0 );
  while( mongo_cursor_next( cursor ) ) {
    Tcl_ListObjAppendElement(interp, resultObj, BsonToList(interp, (&cursor->current)->data, 0));
  }

  mongo_cursor_destroy( cursor );        
  bson_destroy( query );
  bson_destroy( empty );

  Tcl_SetObjResult(interp, resultObj);

  return TCL_OK;
}

/*
cmd remove NsfMongoRemove {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "condition" -required 1 -type tclobj}
}
*/
static int 
NsfMongoRemove(Tcl_Interp *interp, Tcl_Obj *connObj, CONST char *namespace, Tcl_Obj *conditionObj) {
  int objc, result;
  Tcl_Obj **objv;
  mongo_connection *connPtr = MongoGetConn(connObj);
  bson query[1];

  if (connPtr == NULL)  {
    return NsfObjErrType(interp, "", connObj, "connection", NULL);
  }

  result = Tcl_ListObjGetElements(interp, conditionObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(conditionObj));
  }
 
  BsonAppendObjv(interp, query, objc, objv);
  mongo_remove(connPtr, namespace, query);

  bson_destroy(query);
  return TCL_OK;
}

/*
cmd insert NsfMongoUpdate {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "cond" -required 1 -type tclobj}
  {-argName "values" -required 1 -type tclobj}
  {-argName "-upsert" -required 0 -nrargs 0}
  {-argName "-all" -required 0 -nrargs 0}
}
*/
static int 
NsfMongoUpdate(Tcl_Interp *interp, Tcl_Obj *connObj, CONST char *namespace, 
	       Tcl_Obj *conditionObj, Tcl_Obj *valuesObj, int withUpsert, int withAll) {
  int objc, result, options = 0;
  Tcl_Obj **objv;
  mongo_connection *connPtr = MongoGetConn(connObj);
  bson cond[1], values[1];

  if (connPtr == NULL)  {
    return NsfObjErrType(interp, "", connObj, "connection", NULL);
  }

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
  mongo_update(connPtr, namespace, cond, values, options);
  
  return TCL_OK;
}


/***********************************************************************
 * Finally, provide the necessary Tcl package interface.
 ***********************************************************************/

void 
Nsfmongo_Exit(ClientData clientData) {
  fprintf(stderr, "Nsfmongo Exit\n");
}

extern int 
Nsfmongo_Init(Tcl_Interp * interp) {
  int i;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
        return TCL_ERROR;
    }

# ifdef USE_NSF_STUBS
    if (Nsf_InitStubs(interp, "1.1", 0) == NULL) {
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
    if (Tcl_PkgRequire(interp, "nsf", XOTCLVERSION, 0) == NULL) {
        return TCL_ERROR;
    }
#endif

    Tcl_CreateExitHandler(Nsfmongo_Exit, interp);
    NsfMutexLock(&mongoMutex);
    Tcl_InitHashTable(mongoConnsHashTablePtr, TCL_STRING_KEYS);
    NsfMutexUnlock(&mongoMutex);

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
