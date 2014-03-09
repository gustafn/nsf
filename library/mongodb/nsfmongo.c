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
#include "mongoc.h"

#include <tcl.h>
#include <assert.h>
#include <nsf.h>

#define USE_CLIENT_POOL 1

/*
 * Define the counters to generate nice symbols for pointer converter
 */
static int gridfileCount = 0;
static int gridfsCount = 0;
static int mongoClientCount = 0;
static int mongoCollectionCount = 0;
static int mongoCursorCount = 0;

#if defined(USE_CLIENT_POOL)
static NsfMutex poolMutex = 0;
static mongoc_client_pool_t *mongoClientPool = NULL;
static int mongoClientPoolRefCount = 0;
static mongoc_uri_t *mongoUri = NULL;
#endif

typedef enum {
  NSF_BSON_ARRAY,
  NSF_BSON_BOOL,
  NSF_BSON_INT32,
  NSF_BSON_INT64,
  NSF_BSON_DATE_TIME,
  NSF_BSON_DOCUMENT,
  NSF_BSON_DOUBLE,
  NSF_BSON_MINKEY,
  NSF_BSON_MAXKEY,
  NSF_BSON_NULL,
  NSF_BSON_OID,
  NSF_BSON_REGEX,
  NSF_BSON_STRING,
  NSF_BSON_TIMESTAMP,
  NSF_BSON_UNKNOWN
} nsfMongoTypes;

static char *
NsfMongoGlobalStrings[] = {
  "array",
  "boolean",
  "int32",
  "int64",
  "datetime",
  "document",
  "double",
  "minkey",
  "maxkey",
  "null",
  "oid",
  "regex",
  "string",
  "timestamp",
  "unknown",
  NULL
};
static Tcl_Obj **NsfMongoGlobalObjs = NULL;

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
 *      Convert a bson_t structure to a tagged list. Each value field is
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
BsonToList(Tcl_Interp *interp, const bson_t *data , int depth) {
  bson_iter_t i;
  char oidhex[25];
  Tcl_Obj *resultObj, *elemObj;

  bson_iter_init( &i , data );
  resultObj = Tcl_NewListObj(0, NULL);

  while ( bson_iter_next( &i ) ){
    bson_type_t t = bson_iter_type( &i );
    nsfMongoTypes tag;
    const char *key;

    if ( t == 0 )
      break;
    key = bson_iter_key( &i );
    /*fprintf(stderr, "BsonToList: key %s t %d string %d\n", key, t, bson_string);*/

    switch ( t ){
    case BSON_TYPE_INT32:     tag = NSF_BSON_INT32;     elemObj = Tcl_NewIntObj(bson_iter_int32( &i )); break;
    case BSON_TYPE_INT64:     tag = NSF_BSON_INT64;     elemObj = Tcl_NewLongObj(bson_iter_int64( &i )); break;
    case BSON_TYPE_DATE_TIME: tag = NSF_BSON_DATE_TIME; elemObj = Tcl_NewLongObj(bson_iter_date_time( &i )); break;
    case BSON_TYPE_DOUBLE:    tag = NSF_BSON_DOUBLE;    elemObj = Tcl_NewDoubleObj(bson_iter_double( &i )); break;
    case BSON_TYPE_BOOL:      tag = NSF_BSON_BOOL;      elemObj = Tcl_NewBooleanObj(bson_iter_bool( &i )); break;
    case BSON_TYPE_REGEX:  {
      const char *options = NULL, *regex = NULL;
      tag = NSF_BSON_REGEX;
      regex = bson_iter_regex( &i, &options );
      elemObj = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewStringObj(regex, -1));
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewStringObj(options, -1));
      break;
    }
    case BSON_TYPE_UTF8: {
      uint32_t utf8_len;
      const char *string = bson_iter_utf8( &i, &utf8_len);
      /*fprintf(stderr, "append UTF8: <%s> %d\n", string, utf8_len);*/
      tag = NSF_BSON_STRING; elemObj = Tcl_NewStringObj(string, utf8_len); 
      break;
    }
    case BSON_TYPE_MINKEY: tag = NSF_BSON_MINKEY; elemObj = Tcl_NewStringObj("null", 4); break;
    case BSON_TYPE_MAXKEY: tag = NSF_BSON_MAXKEY; elemObj = Tcl_NewStringObj("null", 4); break;
    case BSON_TYPE_NULL:   tag = NSF_BSON_NULL;   elemObj = Tcl_NewStringObj("null", 4); break;
    case BSON_TYPE_OID: {
      tag = NSF_BSON_OID;
      bson_oid_to_string(bson_iter_oid(&i), oidhex);
      elemObj = Tcl_NewStringObj(oidhex, -1);
      break;
    }
    case BSON_TYPE_TIMESTAMP: {
      uint32_t timestamp, increment;
      tag = NSF_BSON_TIMESTAMP;
      bson_iter_timestamp( &i, &timestamp, &increment );
      elemObj = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewIntObj(timestamp));
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewIntObj(increment));
      break;
    }
    case BSON_TYPE_DOCUMENT: {
      const uint8_t *docbuf = NULL;
      uint32_t doclen = 0;
      bson_t b;
      tag = NSF_BSON_DOCUMENT;
      bson_iter_document (&i, &doclen, &docbuf);
      bson_init_static(&b, docbuf, doclen);
      elemObj = BsonToList(interp, &b , depth + 1 );
      break;
    }
    case BSON_TYPE_ARRAY: {
      const uint8_t *docbuf = NULL;
      uint32_t doclen = 0;
      bson_t b;
      tag = NSF_BSON_ARRAY;
      bson_iter_array(&i, &doclen, &docbuf);
      bson_init_static (&b, docbuf, doclen);
      elemObj = BsonToList(interp, &b , depth + 1 );
      break;
    }
    default:
      tag = NSF_BSON_UNKNOWN;
      elemObj = Tcl_NewStringObj("", 0);
      NsfLog(interp, NSF_LOG_WARN, "BsonToList: unknown type %d", t);
    }

    Tcl_ListObjAppendElement(interp, resultObj, Tcl_NewStringObj(key, -1));
    Tcl_ListObjAppendElement(interp, resultObj, NsfMongoGlobalObjs[tag]);
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
bson_type_t
BsonTagToType(Tcl_Interp *interp, char *tag) {
  char firstChar = *tag;

  switch (firstChar) {
  case 'a': /* array */   return BSON_TYPE_ARRAY;
  case 'b': /* bool */    return BSON_TYPE_BOOL;
  case 'd':
    if (*(tag + 1) == 'a') /* date   */ return BSON_TYPE_DATE_TIME;
    if (*(tag + 1) == 'o' && *(tag + 2) == 'c') /* document */ return BSON_TYPE_DOCUMENT;
    if (*(tag + 1) == 'o' && *(tag + 2) == 'u') /* double   */ return BSON_TYPE_DOUBLE;
    break;
  case 'i': /* int32|64 */ 
    if (*(tag + 1) == 'n' && *(tag + 2) == 't' && *(tag + 3) == '3') return BSON_TYPE_INT32;
    if (*(tag + 1) == 'n' && *(tag + 2) == 't' && *(tag + 3) == '6') return BSON_TYPE_INT64;
    if (*(tag + 1) == 'n' && *(tag + 2) == 't') return BSON_TYPE_INT32;
    break;
  case 'm':
    if  (*(tag + 1) == 'i') /* minkey */ return BSON_TYPE_MINKEY;
    if  (*(tag + 1) == 'a') /* maxkey */ return BSON_TYPE_MAXKEY;
    break;
  case 'n': /* null */    return BSON_TYPE_NULL;
  case 'o':
    if  (*(tag + 1) == 'i') /* oid */ return BSON_TYPE_OID;
    break;
  case 'r': /* regex */   return BSON_TYPE_REGEX;
  case 's': /* string */  return BSON_TYPE_UTF8;
  case 't': /* timestamp */ return BSON_TYPE_TIMESTAMP;
  }

  NsfLog(interp, NSF_LOG_WARN, "BsonTagToType: Treat unknown tag '%s' as string", tag);
  return BSON_TYPE_UTF8;
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
BsonAppend(Tcl_Interp *interp, bson_t *bbPtr, char *name, char *tag, Tcl_Obj *value) {
  int result = TCL_OK;
  bson_type_t t = BsonTagToType(interp, tag);
  int keyLength = strlen(name);

  /*fprintf(stderr, "BsonAppend: add name %s tag %s value '%s'\n", name, tag, ObjStr(value));*/

  switch ( t ){
  case BSON_TYPE_UTF8: {
    const char* string = ObjStr(value);
    bson_append_utf8(bbPtr, name, keyLength, string, strlen(string));
    break;
  }
  case BSON_TYPE_INT32: {
    int32_t v;
    result = Tcl_GetIntFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_int32(bbPtr, name, keyLength, v);
    break;
  }
  case BSON_TYPE_DOUBLE: {
    double v;
    result = Tcl_GetDoubleFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_double(bbPtr, name, keyLength, v);
    break;
  }
  case BSON_TYPE_BOOL: {
    int v;
    result = Tcl_GetBooleanFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_bool(bbPtr, name, keyLength, v);
    break;
  }
  case BSON_TYPE_INT64: {
    long v;
    result = Tcl_GetLongFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_int64(bbPtr, name, keyLength, v);
    break;
  }
  case BSON_TYPE_MAXKEY:
    bson_append_maxkey(bbPtr, name, keyLength);
    break;
  case BSON_TYPE_MINKEY:
    bson_append_minkey(bbPtr, name, keyLength);
    break;
  case BSON_TYPE_NULL: {
    bson_append_null(bbPtr, name, keyLength);
    break;
  }
  case BSON_TYPE_OID: {
    bson_oid_t v;
    bson_oid_init_from_string(&v, ObjStr(value));
    bson_append_oid(bbPtr, name, keyLength, &v);
    break;
  }
  case BSON_TYPE_REGEX: {
    int objc = 0;
    Tcl_Obj **objv;

    result = Tcl_ListObjGetElements(interp, value, &objc, &objv);
    if (result != TCL_OK || objc != 2) {
      return NsfPrintError(interp, "invalid regexp representation: %s", ObjStr(value));
    }
    bson_append_regex(bbPtr, name, keyLength, ObjStr(objv[0]), ObjStr(objv[1]));
    break;
  }
  case BSON_TYPE_DATE_TIME: {
    long v;
    result = Tcl_GetLongFromObj(interp, value, &v);
    if (result != TCL_OK) break;
    bson_append_date_time(bbPtr, name, keyLength, v);
    break;
  }
  case BSON_TYPE_TIMESTAMP: {
    int timestamp, increment, objc = 0;
    Tcl_Obj **objv;

    result = Tcl_ListObjGetElements(interp, value, &objc, &objv);
    if (result != TCL_OK || objc != 2) {
      return NsfPrintError(interp, "invalid timestamp: %s", ObjStr(value));
    }
    result = Tcl_GetIntFromObj(interp, objv[0], &timestamp);
    if (result == TCL_OK) {
      result = Tcl_GetIntFromObj(interp, objv[1], &increment);
    }
    if (result != TCL_OK) break;
    bson_append_timestamp(bbPtr, name, keyLength, timestamp, increment);
    break;
  }
  case BSON_TYPE_DOCUMENT:
  case BSON_TYPE_ARRAY: {
    int       i, objc;
    Tcl_Obj **objv;
    bson_t    child, *childPtr = &child;

    result = Tcl_ListObjGetElements(interp, value, &objc, &objv);
    if (result != TCL_OK || objc % 3 != 0) {
      return NsfPrintError(interp, "invalid %s value contain multiple of 3 elements %s", tag, ObjStr(value));
    }

    if (t == BSON_TYPE_DOCUMENT) {
      bson_append_document_begin(bbPtr, name, keyLength, childPtr);
    } else {
      bson_append_array_begin(bbPtr, name, keyLength, childPtr);
    }
    for (i = 0; i< objc; i += 3) {
      /*fprintf(stderr, "value %s, i %d, [0]: %s, [1]: %s, [2]: %s\n", ObjStr(value), i,
	ObjStr(objv[i]),  ObjStr(objv[i+1]), ObjStr(objv[i+2]));*/
      result = BsonAppend(interp, childPtr, ObjStr(objv[i]),  ObjStr(objv[i+1]), objv[i+2]);
      if (result != TCL_OK) break;
    }

    if (t == BSON_TYPE_DOCUMENT) {
      bson_append_document_end(bbPtr, childPtr);
    } else {
      bson_append_array_end(bbPtr, childPtr);
    }
    break;
  }

  case BSON_TYPE_BINARY:
  case BSON_TYPE_DBPOINTER:
  case BSON_TYPE_CODE:
  case BSON_TYPE_SYMBOL:
  case BSON_TYPE_CODEWSCOPE:
    return NsfPrintError(interp, "tag %s not handled yet", tag);
    break;

  case BSON_TYPE_UNDEFINED:
  case BSON_TYPE_EOD:
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
BsonAppendObjv(Tcl_Interp *interp, bson_t *bPtr, int objc, Tcl_Obj **objv) {
  int i;

  bson_init(bPtr);
  for (i = 0; i < objc; i += 3) {
    char *name = ObjStr(objv[i]);
    char *tag = ObjStr(objv[i+1]);
    Tcl_Obj *value = objv[i+2];
    /*fprintf(stderr, "adding pair '%s' (%s) '%s'\n", name, tag, ObjStr(value));*/
    BsonAppend(interp, bPtr, name, tag, value);
  }

  return TCL_OK;
}


/***********************************************************************
 * Define the api functions
 ***********************************************************************/

/*
cmd close NsfMongoClose {
  {-argName "conn" -required 1 -type mongoc_client_t -withObj 1}
}
*/
static int
NsfMongoClose(Tcl_Interp *interp, mongoc_client_t *clientPtr, Tcl_Obj *clientObj) {

  if (clientPtr) {
#if defined(USE_CLIENT_POOL)
    mongoc_client_pool_push(mongoClientPool, clientPtr);
#else
    mongoc_client_destroy(clientPtr);
#endif
    Nsf_PointerDelete(ObjStr(clientObj), clientPtr, 0);
  }
  return TCL_OK;
}


/*
cmd connect NsfMongoConnect {
  {-argName "-uri" -required 1 -nrargs 1}
}
*/
static int
NsfMongoConnect(Tcl_Interp *interp, CONST char *uri) {
  char channelName[80];
  mongoc_client_t *clientPtr;

  if (uri == NULL) {
    uri = "mongodb://127.0.0.1:27017/";
  }

#if defined(USE_CLIENT_POOL)
  NsfMutexLock(&poolMutex);

  if (mongoClientPool == NULL) {
    mongoUri = mongoc_uri_new(uri);
    NsfLog(interp, NSF_LOG_NOTICE, "nsf::mongo::connect: creating pool with uri %s", uri);
    mongoClientPool = mongoc_client_pool_new(mongoUri);
  }

  NsfMutexUnlock(&poolMutex);
  clientPtr = mongoc_client_pool_pop(mongoClientPool);
#else
  clientPtr = mongoc_client_new(uri);
#endif

  if (clientPtr == NULL) {
      return NsfPrintError(interp, "failed to parse Mongo URI");
  }

  /*
   * Make an entry in the symbol table and return entry name it as
   * result.
   */
  if (Nsf_PointerAdd(interp, channelName, "mongoc_client_t", clientPtr) != TCL_OK) {
    mongoc_client_destroy(clientPtr);
    return TCL_ERROR;
  }

  Tcl_SetObjResult(interp, Tcl_NewStringObj(channelName, -1));

  return TCL_OK;
}

/*
cmd run NsfMongoRunCmd {
  {-argName "-nocomplain" -required 0 -nrargs 0}
  {-argName "conn" -required 1 -type mongoc_client_t}
  {-argName "db" -required 1}
  {-argName "cmd" -required 1 -type tclobj}
}
*/
static int
NsfMongoRunCmd(Tcl_Interp *interp, int withNocomplain, mongoc_client_t *clientPtr, 
	       CONST char *db, Tcl_Obj *cmdObj) {
  int result, objc;
  Tcl_Obj **objv;
  bson_t cmd[1], out[1];
  mongoc_read_prefs_t *readPrefsPtr = NULL; /* TODO: not used */
  bson_error_t bsonError;

  result = Tcl_ListObjGetElements(interp, cmdObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(cmdObj));
  }
  BsonAppendObjv(interp, cmd, objc, objv);

  /*mongo_clear_errors( connPtr );*/
  result = mongoc_client_command_simple( clientPtr, db, cmd, readPrefsPtr, out, &bsonError);
  bson_destroy( cmd );

  if (withNocomplain == 0 && result == 0) {
    return NsfPrintError(interp, "mongo::run: command '%s' returned error: %s", ObjStr(cmdObj), bsonError.message);
  }

  Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
  return TCL_OK;
}

/*
cmd collection::open NsfCollectionOpen {
  {-argName "conn" -required 1 -type mongoc_client_t}
  {-argName "dbname" -required 1}
  {-argName "collectionname" -required 1}
}
*/
int
NsfCollectionOpen(Tcl_Interp *interp, 
		  mongoc_client_t *clientPtr, 
		  const char *dbName,
		  const char *collectionName) {
  int   result = TCL_ERROR;
  mongoc_collection_t *collectionPtr;

  collectionPtr = mongoc_client_get_collection(clientPtr, dbName, collectionName);
  if (collectionPtr != NULL) {
    char buffer[80];

    if (Nsf_PointerAdd(interp, buffer, "mongoc_collection_t", collectionPtr) == TCL_OK) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
      result = TCL_OK;
    } else {
      mongoc_collection_destroy(collectionPtr);
      result = TCL_ERROR;
    }
  }
  
  if (collectionPtr == NULL) {
    result = NsfPrintError(interp, 
			   "collection::open: could not open collection: %s.%s", 
			   dbName, collectionName);
  }

  return result;
}

/*
cmd collection::close NsfCollectionClose {
  {-argName "collection" -required 1 -type mongoc_collection_t -withObj 1}
}
*/
static int
NsfCollectionClose(Tcl_Interp *interp, mongoc_collection_t *collectionPtr, Tcl_Obj *clientObj) {
  if (collectionPtr) {
    mongoc_collection_destroy(collectionPtr);
    Nsf_PointerDelete(ObjStr(clientObj), collectionPtr, 0);
  }
  return TCL_OK;
}

/*
cmd collection::count NsfMongoCollectionCount {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "query" -required 1 -type tclobj}
}
*/
static int
NsfMongoCollectionCount(Tcl_Interp *interp, 
			mongoc_collection_t *collectionPtr, 
			Tcl_Obj *queryObj) {
  int objc, result;
  Tcl_Obj **objv;
  int count;
  bson_t query[1];
  bson_error_t bsonError;

  result = Tcl_ListObjGetElements(interp, queryObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(queryObj));
  }

  BsonAppendObjv(interp, query, objc, objv);

  if (collectionPtr != NULL) {
    count = mongoc_collection_count(collectionPtr, 
				    0 /* query flags */, query,
				    0 /*skip */, 0 /*limit */,
				    NULL /* read preferences */,
				    &bsonError);
    fprintf(stderr, "count returns %d \n", count);
    if (count == -1) {
      bson_destroy( query );
      return NsfPrintError(interp, "mongo::collection::count: error: %s", bsonError.message);
    }

  } else {
    count = 0;
  }

  bson_destroy( query );

  Tcl_SetObjResult(interp, Tcl_NewIntObj(count));

  return TCL_OK;
}

/*
cmd "collection::delete" NsfMongoCollectionDelete {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "condition" -required 1 -type tclobj}
}
*/
static int
NsfMongoCollectionDelete(Tcl_Interp *interp, 
			mongoc_collection_t *collectionPtr,
			 Tcl_Obj *conditionObj) {
  int objc, result, status;
  Tcl_Obj **objv;
  bson_t query[1];
  bson_error_t bsonError;
  mongoc_delete_flags_t deleteFlags = 0; /* TODO: not handled */
  /* MONGOC_DELETE_SINGLE_REMOVE = 1 << 0,**/
  const mongoc_write_concern_t *writeConcern = NULL; /* TODO: not handled yet */

  result = Tcl_ListObjGetElements(interp, conditionObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(conditionObj));
  }

  BsonAppendObjv(interp, query, objc, objv);
  status = mongoc_collection_delete(collectionPtr, deleteFlags, query, writeConcern, &bsonError);

  if (status == 0) {
    result = NsfPrintError(interp, "mongo::collection::delete: error: %s", bsonError.message);
  }
  bson_destroy(query);
  return result;
}

/*
cmd "collection::index" NsfMongoCollectionIndex {
  {-argName "collection" -required 1 -type mongoc_collection_t}
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
NsfMongoCollectionIndex(Tcl_Interp *interp,
			mongoc_collection_t *collectionPtr,
			Tcl_Obj *attributesObj,
			CONST char *withName,
			int withBackground,
			int withDropdups,
			int withSparse,
			int withTtl,
			int withUnique) {
  int success = 0;
  int objc, result;
  Tcl_Obj **objv;
  bson_t keys[1];
  bson_error_t bsonError;
  mongoc_index_opt_t options;

  result = Tcl_ListObjGetElements(interp, attributesObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(attributesObj));
  }

  BsonAppendObjv(interp, keys, objc, objv);

  mongoc_index_opt_init(&options);

  if (withBackground) {options.background = 1;}
  if (withDropdups)   {options.drop_dups = 1;}
  if (withSparse)     {options.sparse = 1;}
  if (withUnique)     {options.unique = 1;}
  if (withTtl)        {options.expire_after_seconds = withTtl;}
  if (withName)       {options.name = withName;}
  /* TODO: not handled: is_initialized, v, weights, default_language, laguage_override, padding */

  success = mongoc_collection_ensure_index(collectionPtr, keys, &options, &bsonError);

  bson_destroy(keys);

  Tcl_SetObjResult(interp, Tcl_NewBooleanObj(success));
  return TCL_OK;
}


/*
cmd "collection::insert" NsfMongoCollectionInsert {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "values" -required 1 -type tclobj}
}
*/
static int NsfMongoCollectionInsert(Tcl_Interp *interp, 
				    mongoc_collection_t *collectionPtr,
				    Tcl_Obj *valuesObj) {
  int i, objc, result;
  Tcl_Obj **objv;
  bson_t b[1];
  bson_oid_t oid;
  bson_error_t bsonError;
  mongoc_insert_flags_t insertFlags = MONGOC_INSERT_NO_VALIDATE; /* otherwise, we can't insert a DBRef */
  /* TODO: insertFlags not handled:
   MONGOC_INSERT_NONE              = 0,
   MONGOC_INSERT_CONTINUE_ON_ERROR = 1 << 0,
   MONGOC_INSERT_NO_VALIDATE       = 1 << 31,
  */
  const mongoc_write_concern_t *writeConcern = NULL; /* TODO: not handled yet */

  result = Tcl_ListObjGetElements(interp, valuesObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(valuesObj));
  }

  bson_init(b);
  bson_oid_init(&oid, NULL);
  bson_append_oid(b, "_id", 3, &oid);

  for (i = 0; i < objc; i += 3) {
    char *name = ObjStr(objv[i]);
    char *tag = ObjStr(objv[i+1]);
    Tcl_Obj *value = objv[i+2];
    /*fprintf(stderr, "adding pair '%s' (%s) '%s'\n", name, tag, ObjStr(value));*/
    BsonAppend(interp, b, name, tag, value);
  }

  result = mongoc_collection_insert(collectionPtr, insertFlags, b, writeConcern, &bsonError);

  if (result == 0) {
    bson_destroy(b);
    return NsfPrintError(interp, "mongo::collection::insert: error: %s", bsonError.message);
  } else {
    Tcl_SetObjResult(interp, BsonToList(interp, b, 0));
    result = TCL_OK;
    bson_destroy(b);
  }
  return result;
}

/*
cmd collection::query NsfMongoCollectionQuery {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "query" -required 1 -type tclobj}
  {-argName "-atts" -required 0 -nrargs 1 -type tclobj}
  {-argName "-limit" -required 0 -type int32}
  {-argName "-skip" -required 0 -type int32}
}
*/
static int
NsfMongoCollectionQuery(Tcl_Interp *interp, 
			mongoc_collection_t *collectionPtr,
			Tcl_Obj *queryObj, Tcl_Obj *withAttsObj,
			int withLimit, int withSkip) {
  int objc1, objc2, result;
  Tcl_Obj **objv1, **objv2, *resultObj;
  mongoc_cursor_t *cursor;
  bson_t query[1];
  bson_t atts[1];
  const bson_t *nextPtr;
  mongoc_query_flags_t queryFlags = 0; /* TODO: not handled */
  mongoc_read_prefs_t *readPrefs = NULL; /* TODO: not handled */

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
  BsonAppendObjv(interp, atts,  objc2, objv2);

  resultObj = Tcl_NewListObj(0, NULL);

  /*
   *  The last field of mongo_find is options, semantics are described here
   *  http://www.mongodb.org/display/DOCS/Mongo+Wire+Protocol#MongoWireProtocol-OPQUERY
   */
  cursor = mongoc_collection_find( collectionPtr, queryFlags, 
				   withSkip, withLimit, 0 /* batch_size */,
				   query, atts, readPrefs);

  while( mongoc_cursor_next( cursor, &nextPtr ) == 1 ) {
    Tcl_ListObjAppendElement(interp, resultObj, BsonToList(interp, nextPtr, 0));
  }

  mongoc_cursor_destroy( cursor );
  bson_destroy( query );
  bson_destroy( atts );

  Tcl_SetObjResult(interp, resultObj);

  return TCL_OK;
}


/*
cmd "collection::update" NsfMongoCollectionUpdate {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "cond" -required 1 -type tclobj}
  {-argName "values" -required 1 -type tclobj}
  {-argName "-upsert" -required 0 -nrargs 0}
  {-argName "-all" -required 0 -nrargs 0}
}
*/
static int
NsfMongoCollectionUpdate(Tcl_Interp *interp, 
			 mongoc_collection_t *collectionPtr,
			 Tcl_Obj *conditionObj, Tcl_Obj *valuesObj, 
			 int withUpsert, int withAll) {

  const mongoc_write_concern_t *writeConcern = NULL; /* TODO: not handled yet */
  mongoc_update_flags_t updateFlags =  MONGOC_UPDATE_NO_VALIDATE; /* for dbrefs */
  bson_error_t bsonError;
  bson_t cond[1], values[1];
  int objc, result, success;
  Tcl_Obj **objv;

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

  if (withUpsert) {updateFlags |= MONGOC_UPDATE_UPSERT;}
  if (withAll)    {updateFlags |= MONGOC_UPDATE_MULTI_UPDATE;}

  success = mongoc_collection_update(collectionPtr, updateFlags, cond, values, writeConcern, &bsonError);

  if (success == 0) {
    result = NsfPrintError(interp, "mongo::collection::delete: error: %s", bsonError.message);
  }

  return result;
}

/***********************************************************************
 * Cursor interface
 ***********************************************************************/
/*
cmd cursor::find NsfMongoCursorFind {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "query" -required 1 -type tclobj}
  {-argName "-atts" -required 0 -nrargs 1 -type tclobj}
  {-argName "-limit" -required 0 -type int32}
  {-argName "-skip" -required 0 -type int32}
  {-argName "-tailable" -required 0 -nrargs 0}
  {-argName "-awaitdata" -required 0 -nrargs 0}
}
*/
static int
NsfMongoCursorFind(Tcl_Interp *interp, 
		   mongoc_collection_t *collectionPtr,
		   Tcl_Obj *queryObj, Tcl_Obj *withAttsObj,
		   int withLimit, int withSkip,
		   int withTailable, int withAwaitdata) {
  int objc1, objc2, result;
  mongoc_query_flags_t queryFlags = 0;
  Tcl_Obj **objv1, **objv2;
  mongoc_cursor_t *cursor;
  bson_t query[1];
  bson_t atts[1];
  mongoc_read_prefs_t *readPrefsPtr = NULL; /* TODO: not used */

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
    queryFlags |= MONGOC_QUERY_TAILABLE_CURSOR;
  }
  if (withAwaitdata) {
    queryFlags |= MONGOC_QUERY_AWAIT_DATA;
  }
  /* TODO: query flags:
   MONGOC_QUERY_SLAVE_OK          = 1 << 2,
   MONGOC_QUERY_OPLOG_REPLAY      = 1 << 3,
   MONGOC_QUERY_NO_CURSOR_TIMEOUT = 1 << 4,
   MONGOC_QUERY_EXHAUST           = 1 << 6,
   MONGOC_QUERY_PARTIAL           = 1 << 7,
  */
  cursor = mongoc_collection_find(collectionPtr, queryFlags, 
				  withSkip, withLimit, 0 /*TODO missing batch_size*/,
				  query, atts, readPrefsPtr);
  if (cursor) {
    char buffer[80];
    if (Nsf_PointerAdd(interp, buffer, "mongoc_cursor_t", cursor) == TCL_OK) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
    } else {
      mongoc_cursor_destroy( cursor );
      result = TCL_ERROR;
    }
  } else {
    Tcl_ResetResult(interp);
  }

  bson_destroy( query );
  bson_destroy( atts );

  return result;
}

/*
cmd cursor::next NsfMongoCursorNext {
  {-argName "cursor" -required 1 -type mongoc_cursor_t}
}
*/
static int
NsfMongoCursorNext(Tcl_Interp *interp, mongoc_cursor_t *cursor) {
  int result;
  const bson_t *nextPtr;

  result = mongoc_cursor_next( cursor, &nextPtr );
  if (result == 1) {
    Tcl_SetObjResult(interp, BsonToList(interp, nextPtr, 0));
  }
  return TCL_OK;
}

/*
cmd cursor::close NsfMongoCursorClose {
  {-argName "cursor" -required 1 -type mongoc_cursor_t -withObj 1}
}
*/
static int
NsfMongoCursorClose(Tcl_Interp *interp, mongoc_cursor_t *cursor, Tcl_Obj *cursorObj) {

  mongoc_cursor_destroy( cursor );
  Nsf_PointerDelete(ObjStr(cursorObj), cursor, 0);

  return TCL_OK;
}



/***********************************************************************
 * GridFS interface
 ***********************************************************************/

/*
cmd gridfs::close NsfMongoGridFSClose {
  {-argName "gfs" -required 1 -type mongoc_gridfs_t -withObj 1}
}
*/
static int
NsfMongoGridFSClose(Tcl_Interp *interp, mongoc_gridfs_t *gridfsPtr, Tcl_Obj *gridfsObj) {

  mongoc_gridfs_destroy(gridfsPtr);
  Nsf_PointerDelete(ObjStr(gridfsObj), gridfsPtr, 0);

  return TCL_OK;
}

/*
cmd gridfs::open NsfMongoGridFSOpen {
  {-argName "conn" -required 1 -type mongoc_client_t}
  {-argName "dbname" -required 1}
  {-argName "prefix" -required 1}
}
*/

static int
NsfMongoGridFSOpen(Tcl_Interp *interp, mongoc_client_t *clientPtr,
		   CONST char *dbname, CONST char *prefix) {
  char buffer[80];
  int result = TCL_OK;
  bson_error_t bsonError;
  mongoc_gridfs_t *gfsPtr;

  gfsPtr = mongoc_client_get_gridfs(clientPtr, dbname, prefix, &bsonError);

  if (gfsPtr == NULL) {
    result = NsfPrintError(interp, "mongo::gridfs::open: error: %s", bsonError.message);
  }

  if (Nsf_PointerAdd(interp, buffer, "mongoc_gridfs_t", gfsPtr) == TCL_OK) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
  } else {
    mongoc_gridfs_destroy(gfsPtr);
    result = TCL_ERROR;
  }

  return result;
}

/***********************************************************************
 * GridFile interface operating on GridFS
 *
 * Currently we need a few private gridfs functions since the new
 * c-driver has less functionality than the old one.
 ***********************************************************************/
#define MONGO_HAS_NO_GRIDFS_COLLECTION_ACCESSOR 1

#if defined(MONGO_HAS_NO_GRIDFS_COLLECTION_ACCESSOR)
#define MONGOC_INSIDE 1
#include "mongoc-gridfs-private.h"
#undef MONGOC_INSIDE

mongoc_collection_t *
mongoc_gridfs_get_files (mongoc_gridfs_t * gridfs) 
{
    return gridfs->files;
}

mongoc_collection_t *
mongoc_gridfs_get_chunks (mongoc_gridfs_t * gridfs) 
{
    return gridfs->chunks;
}
#endif



#define MONGOC_GRIDFS_READ_CHUNK 4096*4


/*
cmd gridfile::create NsfMongoGridFileCreate {
  {-argName "-source" -required 1 -typeName "gridfilesource" -type "file|string"}
  {-argName "gfs" -required 1 -type mongoc_gridfs_t}
  {-argName "value" -required 1}
  {-argName "name" -required 1}
  {-argName "contenttype" -required 1}
  {-argName "-metadata" -required 0 -nrags 1 -type tclobj}
}
*/
static int
NsfMongoGridFileCreate(Tcl_Interp *interp, int withSource,
		       mongoc_gridfs_t *gridfsPtr,
		       CONST char *value, CONST char *name,
		       CONST char *contenttype,
		       Tcl_Obj *withMetadata
		       ) {
  int result = TCL_OK;
  mongoc_gridfs_file_opt_t fileOpts = {NULL};
  mongoc_gridfs_file_t *gridFile;
  bson_t bsonMetaData[1];

  if (withSource == GridfilesourceNULL) {
    withSource = GridfilesourceFileIdx;
  }

  if (withMetadata != NULL) {
    Tcl_Obj **objv;
    int objc;

    result = Tcl_ListObjGetElements(interp, withMetadata, &objc, &objv);
    if (result != TCL_OK || (objc % 3 != 0)) {
      return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(withMetadata));
    }
    BsonAppendObjv(interp, bsonMetaData, objc, objv);
    fileOpts.metadata = bsonMetaData;
  }

  fileOpts.filename = name;
  fileOpts.content_type = contenttype;
  /*
   const char   *md5;
   const bson_t *aliases;
   uint32_t chunk_size;
  */
  gridFile = mongoc_gridfs_create_file(gridfsPtr, &fileOpts);

  if (withSource == GridfilesourceFileIdx) {
    uint8_t buf[MONGOC_GRIDFS_READ_CHUNK];
    struct iovec iov = { buf, 0 };
    int fd = open(value, O_RDONLY);
    
    if (fd < 1) {
      mongoc_gridfs_file_destroy(gridFile);
      return NsfPrintError(interp, "nsf::gridfile::create: cannot open file '%s' for reading", value);
    }

    for (;; ) {
      int n = read(fd, iov.iov_base, MONGOC_GRIDFS_READ_CHUNK);
      if (n > 0) {
	iov.iov_len = n;
	n = mongoc_gridfs_file_writev(gridFile, &iov, 1, 0);
      } else if (n == 0) {
	break;
      } else {
	result = TCL_ERROR;
	break;
      }
    }
    close(fd);
  } else {
    struct iovec iov = { (char *)value, strlen(value) };
    mongoc_gridfs_file_writev(gridFile, &iov, 1, 0);
  }
  if (result == TCL_OK) {
    mongoc_gridfs_file_save(gridFile);
  }

  mongoc_gridfs_file_destroy(gridFile);

  Tcl_SetObjResult(interp, Tcl_NewIntObj(result == TCL_OK));

  return result;
}


/*
cmd "gridfile::delete" NsfMongoGridFileDelete {
  {-argName "gfs" -required 1 -type mongoc_gridfs_t}
  {-argName "query" -required 1 -type tclobj}
}
*/
static int
NsfMongoGridFileDelete(Tcl_Interp *interp, 
			 mongoc_gridfs_t *gridfsPtr,
			 Tcl_Obj *queryObj) {
  bson_t query[1];
  mongoc_cursor_t *files;
  bson_iter_t it[1];
  bson_oid_t id;
  bson_t b[1];
  const bson_t *nextPtr;
  bson_error_t bsonError;
  Tcl_Obj **objv;
  int objc, result;

  result = Tcl_ListObjGetElements(interp, queryObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(queryObj));
  }

  BsonAppendObjv(interp, query, objc, objv);
  files = mongoc_collection_find( mongoc_gridfs_get_files(gridfsPtr), 0, 
				   0, 0, 0 /* batch_size */,
				   query, NULL, NULL);
  bson_destroy(query);

  /* files should be a valid cursor even if the file doesn't exist */
  if ( files == NULL ) {
    return NsfPrintError(interp, "gridfs::remove_file: invalid cursor for files");
  }

  /* Remove each file and it's chunks from files named filename */
  while (mongoc_cursor_next(files, &nextPtr)) {
    bson_iter_init_find(it, nextPtr, "_id");
    id = *bson_iter_oid(it);

    /* Remove the file with the specified id */
    bson_init(b);
    bson_append_oid(b, "_id", 3, &id);
    mongoc_collection_delete(mongoc_gridfs_get_files(gridfsPtr), 0, b, NULL, &bsonError);
    bson_destroy(b);

    /* Remove all chunks from the file with the specified id */
    bson_init(b);
    bson_append_oid(b, "files_id", 8, &id);
    mongoc_collection_delete(mongoc_gridfs_get_chunks(gridfsPtr), 0, b, NULL, &bsonError);
    bson_destroy(b);
  }

  mongoc_cursor_destroy(files);
  return TCL_OK;
}

/*
cmd gridfile::open NsfMongoGridFileOpen {
  {-argName "gfs" -required 1 -type mongoc_gridfs_t}
  {-argName "query" -required 1 -type tclobj}
}
*/
static int
NsfMongoGridFileOpen(Tcl_Interp *interp, 
		     mongoc_gridfs_t *gridfsPtr, 
		     Tcl_Obj *queryObj) {
  mongoc_gridfs_file_t* gridFilePtr;
  bson_error_t bsonError;
  int result, objc;
  bson_t query[1];
  Tcl_Obj **objv;

  /*fprintf(stderr, "NsfMongoQuery: namespace %s withLimit %d withSkip %d\n",
    namespace, withLimit, withSkip);*/

  result = Tcl_ListObjGetElements(interp, queryObj, &objc, &objv);
  if (result != TCL_OK || (objc % 3 != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(queryObj));
  }

  BsonAppendObjv(interp, query, objc, objv);

  gridFilePtr = mongoc_gridfs_find_one(gridfsPtr, query, &bsonError);

  if (gridFilePtr != NULL) {
    char buffer[80];

    if (Nsf_PointerAdd(interp, buffer, "mongoc_gridfs_file_t", gridFilePtr) == TCL_OK) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
    } else {
      mongoc_gridfs_file_destroy(gridFilePtr);
      result = TCL_ERROR;
    }
  } else {
    Tcl_ResetResult(interp);
  }

  bson_destroy(query);
  return result;
}


/***********************************************************************
 * GridFile interface
 *
 * Currently offsets and sizes are limited to 32bit integers, we should
 * relax this later.
 ***********************************************************************/

/*
cmd gridfile::close NsfMongoGridFileClose {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t -withObj 1}
}
*/
static int
NsfMongoGridFileClose(Tcl_Interp *interp, mongoc_gridfs_file_t* gridFilePtr, Tcl_Obj *gridFileObj) {

  mongoc_gridfs_file_destroy(gridFilePtr);
  Nsf_PointerDelete(ObjStr(gridFileObj), gridFilePtr, 0);

  return TCL_OK;
}

/*
cmd gridfile::get_contentlength NsfMongoGridFileGetContentlength {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
}
*/
static int
NsfMongoGridFileGetContentlength(Tcl_Interp *interp, mongoc_gridfs_file_t* gridFilePtr) {
  int64_t len;

  len = mongoc_gridfs_file_get_length(gridFilePtr);
  Tcl_SetObjResult(interp, Tcl_NewLongObj(len));

  return TCL_OK;
}

/*
cmd gridfile::get_contenttype NsfMongoGridFileGetContentType {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
}
*/
static int
NsfMongoGridFileGetContentType(Tcl_Interp *interp, mongoc_gridfs_file_t* gridFilePtr) {

  Tcl_SetObjResult(interp, Tcl_NewStringObj(mongoc_gridfs_file_get_content_type(gridFilePtr), -1));

  return TCL_OK;
}



/*
cmd gridfile::get_metadata NsfMongoGridFileGetMetaData {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
}
*/
static int
NsfMongoGridFileGetMetaData(Tcl_Interp *interp, mongoc_gridfs_file_t* gridFilePtr) {
  const bson_t *metaDataPtr = mongoc_gridfs_file_get_metadata(gridFilePtr);

  if (metaDataPtr != NULL) {
    Tcl_SetObjResult(interp, BsonToList(interp, metaDataPtr, 0));
  }
  return TCL_OK;
}

/*
cmd gridfile::read NsfMongoGridFileRead {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
  {-argName "size" -required 1 -type int}
}
*/
static int
NsfMongoGridFileRead(Tcl_Interp *interp, mongoc_gridfs_file_t *gridFilePtr, int size) {
  int readSize;
  Tcl_Obj *resultObj = Tcl_NewByteArrayObj(NULL, size);
  struct iovec iov = { NULL, size };

  iov.iov_base = Tcl_SetByteArrayLength(resultObj, size);

  readSize = mongoc_gridfs_file_readv(gridFilePtr, &iov, 1,
				      0 /* min_bytes */,
				      0 /* timeout_msec */);
  /*fprintf(stderr, "NsfMongoGridFileRead want %d got %d\n", size, readSize);*/
  Tcl_SetByteArrayLength(resultObj, readSize);
  Tcl_SetObjResult(interp, resultObj);

  return TCL_OK;
}

/*
cmd "gridfile::seek" NsfMongoGridFileSeek {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
  {-argName "offset" -required 1 -type int32}
}
*/
static int
NsfMongoGridFileSeek(Tcl_Interp *interp, mongoc_gridfs_file_t *gridFilePtr, int offset) {
  int result;

  /* TODO: whence SEEK_SET, SEEK_CUR or SEEK_END; implementation of SEEK_END looks incorrect */
  result = mongoc_gridfs_file_seek(gridFilePtr, offset, SEEK_SET);

  return result < 0 ? TCL_ERROR : TCL_OK;
}

/***********************************************************************
 * Finally, provide the necessary Tcl package interface.
 ***********************************************************************/

void
Nsfmongo_ThreadExit(ClientData clientData) {
  /*
   * The exit might happen at a time, when tcl is already shut down.
   * We can't reliably call NsfLog.
   */
  fprintf(stderr, "+++ Nsfmongo_ThreadExit\n");
#if defined(USE_CLIENT_POOL)
  NsfMutexLock(&poolMutex);
  mongoClientPoolRefCount --;
  if (mongoClientPool != NULL) {
    fprintf(stderr, "========= Nsfmongo_ThreadExit mongoClientPoolRefCount %d\n", mongoClientPoolRefCount);
    if (mongoClientPoolRefCount < 1) {
      mongoc_client_pool_destroy(mongoClientPool);
      mongoClientPool = NULL;
      mongoc_uri_destroy(mongoUri);
      mongoUri = NULL;
    }
  }
  NsfMutexUnlock(&poolMutex);
#endif
}

void
Nsfmongo_Exit(ClientData clientData) {
  /*
   * The exit might happen at a time, when tcl is already shut down.
   * We can't reliably call NsfLog.
   *
   *   Tcl_Interp *interp = (Tcl_Interp *)clientData;
   *   NsfLog(interp,NSF_LOG_NOTICE, "Nsfmongo Exit");
   */
  fprintf(stderr, "+++ Nsfmongo_Exit\n");
#if defined(TCL_THREADS)
  Tcl_DeleteThreadExitHandler(Nsfmongo_ThreadExit, clientData);
#endif
  Tcl_Release(clientData);
}



extern int
Nsfmongo_Init(Tcl_Interp * interp) {
  int i;
  static NsfMutex initMutex = 0;

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

  Tcl_Preserve(interp);
#if defined(TCL_THREADS)
  Tcl_CreateThreadExitHandler(Nsfmongo_ThreadExit, interp);
#endif
  Tcl_CreateExitHandler(Nsfmongo_Exit, interp);

#if defined(USE_CLIENT_POOL)
  NsfMutexLock(&poolMutex);
  mongoClientPoolRefCount ++;
  NsfMutexUnlock(&poolMutex);
#endif

  /*
   * Register global mongo tcl_objs
   */
  NsfMutexLock(&initMutex);
  if (NsfMongoGlobalObjs == NULL) {
    NsfMongoGlobalObjs = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*)*nr_elements(NsfMongoGlobalStrings));
    for (i = 0; i < nr_elements(NsfMongoGlobalStrings); i++) {
      NsfMongoGlobalObjs[i] = Tcl_NewStringObj(NsfMongoGlobalStrings[i], -1);
      Tcl_IncrRefCount(NsfMongoGlobalObjs[i]);
    }
  }
  NsfMutexUnlock(&initMutex);

  /*
   * register the pointer converter
   */
  Nsf_PointerTypeRegister(interp, "mongoc_client_t",      &mongoClientCount);
  Nsf_PointerTypeRegister(interp, "mongoc_collection_t",  &mongoCollectionCount);
  Nsf_PointerTypeRegister(interp, "mongoc_cursor_t",      &mongoCursorCount);
  Nsf_PointerTypeRegister(interp, "mongoc_gridfs_file_t", &gridfileCount);
  Nsf_PointerTypeRegister(interp, "mongoc_gridfs_t",      &gridfsCount);

  for (i=0; i < nr_elements(method_command_namespace_names); i++) {
    Tcl_CreateNamespace(interp, method_command_namespace_names[i], 0, (Tcl_NamespaceDeleteProc *)NULL);
  }

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

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * fill-column: 78
 * indent-tabs-mode: nil
 * End:
 */

