/*
 * Interface between MongoDB based on NSF (Next Scripting
 * Framework)
 *
 * This implementation provides a low-level interface based on tagged elements
 * to force / preserve the datatypes of MongoDB when converting into Tcl.
 *
 * This code serves as well as an example how to use the source code generator
 * of NSF.  The example shows how to use the source code generator from NSF to
 * generate a C interface.
 *
 * -gustaf neumann    March 27, 2011
 *
 * Copyright (C) 2011-2018 Gustaf Neumann
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
 * Size of handle as used by Nsf_PointerAdd()
 */
#define POINTER_HANDLE_SIZE 80u

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
  NSF_BSON_BINARY,
  NSF_BSON_BOOL,
  NSF_BSON_INT32,
  NSF_BSON_INT64,
  NSF_BSON_DATE_TIME,
  NSF_BSON_DECIMAL128,
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

static const char *
NsfMongoGlobalStrings[] = {
  "array",
  "binary",
  "boolean",
  "int32",
  "int64",
  "datetime",
  "decimal128",
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

static Tcl_Obj *BsonToList(Tcl_Interp *interp, const bson_t *data , int depth);
static bson_type_t BsonTagToType(Tcl_Interp *interp, Tcl_Obj *tagObj);

extern Tcl_PackageInitProc Nsfmongo_SafeInit;
extern Tcl_PackageInitProc Nsfmongo_Init;
static Tcl_ExitProc Nsfmongo_Exit;
static Tcl_ExitProc Nsfmongo_ThreadExit;

Nsf_TypeConverter Nsf_ConvertTo_Boolean;
Nsf_TypeConverter Nsf_ConvertTo_Class;
Nsf_TypeConverter Nsf_ConvertTo_Int32;
Nsf_TypeConverter Nsf_ConvertTo_Integer;
Nsf_TypeConverter Nsf_ConvertTo_Object;
Nsf_TypeConverter Nsf_ConvertTo_Pointer;
Nsf_TypeConverter Nsf_ConvertTo_String;
Nsf_TypeConverter Nsf_ConvertTo_Tclobj;

/***********************************************************************
 * The following definitions should not be here, but they are included
 * to get compilation going for the time being.
 ***********************************************************************/
typedef void *NsfObject;

#define PARSE_CONTEXT_PREALLOC 20
typedef struct {
  ClientData   *clientData;   /* 4 members pointer to the actual parse context data */
  Tcl_Obj     **objv;
  Tcl_Obj     **full_objv;    /* contains method as well */
  unsigned int *flags;
  ClientData    clientData_static[PARSE_CONTEXT_PREALLOC]; /* 3 members preallocated parse context data */
  Tcl_Obj      *objv_static[PARSE_CONTEXT_PREALLOC+1];
  unsigned int  flags_static[PARSE_CONTEXT_PREALLOC+1];
  unsigned int  status;
  int           lastObjc;     /* points to the first "unprocessed" argument */
  int           objc;
  NsfObject    *object;
  int           varArgs;      /* does the parameter end with some kind of "args" */
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
#  if defined(HAVE_INTPTR_T) || defined(intptr_t)
#    define INT2PTR(p) ((void *)(intptr_t)(p))
#    define PTR2INT(p) ((int)(intptr_t)(p))
#  else
#    define INT2PTR(p) ((void *)(p))
#    define PTR2INT(p) ((int)(p))
#  endif
#endif
#if !defined(UINT2PTR) && !defined(PTR2UINT)
#  if defined(HAVE_UINTPTR_T) || defined(uintptr_t)
#    define UINT2PTR(p) ((void *)(uintptr_t)(p))
#    define PTR2UINT(p) ((unsigned int)(uintptr_t)(p))
#  else
#    define UINT2PTR(p) ((void *)(p))
#    define PTR2UINT(p) ((unsigned int)(p))
#  endif
#endif


static int ArgumentParse(Tcl_Interp *interp, int objc, Tcl_Obj *const objv[],
                         NsfObject *obj, Tcl_Obj *procName,
                         Nsf_Param const *paramPtr, int nrParameters, int serial,
                         unsigned int processFlags, ParseContext *pc) {
  return Nsf_ArgumentParse(interp, objc, objv, (Nsf_Object *)obj,
                           procName, paramPtr, nrParameters, serial,
                           processFlags, (Nsf_ParseContext *)pc);
}

/***********************************************************************
 * Include the generated mongo db API.
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
BsonToList(Tcl_Interp *interp, const bson_t *data , int depth)
{
  bson_iter_t i;
  char        oidhex[25];
  Tcl_Obj    *resultObj, *elemObj;

  bson_iter_init( &i , data );
  resultObj = Tcl_NewListObj(0, NULL);

  while ( bson_iter_next( &i ) ){
    bson_type_t   t = bson_iter_type( &i );
    nsfMongoTypes tag;
    const char   *key;

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
      const char *options = NULL, *regex;

      tag = NSF_BSON_REGEX;
      regex = bson_iter_regex( &i, &options );
      elemObj = Tcl_NewListObj(0, NULL);
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewStringObj(regex, -1));
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewStringObj(options, -1));
      break;
    }
    case BSON_TYPE_UTF8: {
      uint32_t    utf8_len;
      const char *string = bson_iter_utf8( &i, &utf8_len);

      /*fprintf(stderr, "append UTF8: <%s> %d\n", string, utf8_len);*/
      tag = NSF_BSON_STRING; elemObj = Tcl_NewStringObj(string, (int)utf8_len);
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
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewLongObj((long)timestamp));
      Tcl_ListObjAppendElement(interp, elemObj, Tcl_NewLongObj((long)increment));
      break;
    }
    case BSON_TYPE_DOCUMENT: {
      const uint8_t *docbuf = NULL;
      uint32_t       doclen = 0;
      bson_t         b;

      tag = NSF_BSON_DOCUMENT;
      bson_iter_document(&i, &doclen, &docbuf);
      bson_init_static(&b, docbuf, doclen);
      elemObj = BsonToList(interp, &b , depth + 1 );
      break;
    }
    case BSON_TYPE_ARRAY: {
      const uint8_t *docbuf = NULL;
      uint32_t       doclen = 0;
      bson_t         b;

      tag = NSF_BSON_ARRAY;
      bson_iter_array(&i, &doclen, &docbuf);
      bson_init_static(&b, docbuf, doclen);
      elemObj = BsonToList(interp, &b , depth + 1 );
      break;
    }
    case BSON_TYPE_DECIMAL128: {
      bson_decimal128_t decimal128;
      char              string[BSON_DECIMAL128_STRING];

      tag = NSF_BSON_DECIMAL128;
      bson_iter_decimal128( &i, &decimal128);
      bson_decimal128_to_string(&decimal128, string);
      elemObj = Tcl_NewStringObj(string, -1);

      break;
    }
    case BSON_TYPE_BINARY: {
      uint32_t       length;
      const uint8_t *bytes;

      tag = NSF_BSON_BINARY;
      bson_iter_binary( &i, NULL /* subtype_t */, &length, &bytes);
      elemObj = Tcl_NewByteArrayObj(bytes, (int)length);
      break;
    }
    case BSON_TYPE_CODE:       NSF_FALL_THROUGH; /* fall through */
    case BSON_TYPE_CODEWSCOPE: NSF_FALL_THROUGH; /* fall through */
    case BSON_TYPE_DBPOINTER:  NSF_FALL_THROUGH; /* fall through */
    case BSON_TYPE_EOD:        NSF_FALL_THROUGH; /* fall through */
    case BSON_TYPE_SYMBOL:     NSF_FALL_THROUGH; /* fall through */
    case BSON_TYPE_UNDEFINED:  NSF_FALL_THROUGH; /* fall through */
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
BsonTagToType(Tcl_Interp *interp, Tcl_Obj *tagObj)
{
  const char *tag = ObjStr(tagObj);
  char        firstChar = *tag;

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
 *      A standard Tcl result.
 *
 * Side effects:
 *      Value appended to bson buffer.
 *
 *----------------------------------------------------------------------
 */
static int
BsonAppend(Tcl_Interp *interp, bson_t *bbPtr, Tcl_Obj *nameObj, Tcl_Obj *tagObj, Tcl_Obj *value)
{
  int         result = TCL_OK;
  bson_type_t t = BsonTagToType(interp, tagObj);
  int         keyLength;
  const char *name = Tcl_GetStringFromObj(nameObj, &keyLength);

  /*fprintf(stderr, "BsonAppend: add name %s tag %s value '%s'\n", name, tag, ObjStr(value));*/

  switch ( t ){
  case BSON_TYPE_UTF8: {
    int         stringLength;
    const char* string = Tcl_GetStringFromObj(value, &stringLength);

    bson_append_utf8(bbPtr, name, keyLength, string, stringLength);
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
    int       objc = 0;
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
    int       timestamp = 0, increment = 0, objc = 0;
    Tcl_Obj **objv;

    result = Tcl_ListObjGetElements(interp, value, &objc, &objv);
    if (result != TCL_OK || objc != 2) {
      return NsfPrintError(interp, "invalid timestamp: %s", ObjStr(value));
    } else {
      result = Tcl_GetIntFromObj(interp, objv[0], &timestamp);
      if (result == TCL_OK) {
        result = Tcl_GetIntFromObj(interp, objv[1], &increment);
      }
      if (result == TCL_OK) {
        bson_append_timestamp(bbPtr, name, keyLength, (uint32_t)timestamp, (uint32_t)increment);
      }
    }
    break;
  }
  case BSON_TYPE_DOCUMENT:
  case BSON_TYPE_ARRAY: {
    int       i, objc;
    Tcl_Obj **objv;
    bson_t    child, *childPtr = &child;

    result = Tcl_ListObjGetElements(interp, value, &objc, &objv);
    if (result != TCL_OK || ((objc % 3) != 0)) {
      return NsfPrintError(interp, "invalid %s value contain multiple of 3 elements %s",
                           ObjStr(tagObj), ObjStr(value));
    }

    if (t == BSON_TYPE_DOCUMENT) {
      bson_append_document_begin(bbPtr, name, keyLength, childPtr);
    } else {
      bson_append_array_begin(bbPtr, name, keyLength, childPtr);
    }
    for (i = 0; i< objc; i += 3) {
      /*fprintf(stderr, "value %s, i %d, [0]: %s, [1]: %s, [2]: %s\n", ObjStr(value), i,
        ObjStr(objv[i]),  ObjStr(objv[i+1]), ObjStr(objv[i+2]));*/
      result = BsonAppend(interp, childPtr, objv[i], objv[i+1], objv[i+2]);
      if (result != TCL_OK) break;
    }

    if (t == BSON_TYPE_DOCUMENT) {
      bson_append_document_end(bbPtr, childPtr);
    } else {
      bson_append_array_end(bbPtr, childPtr);
    }
    break;
  }
  case BSON_TYPE_DECIMAL128: {
    bson_decimal128_t decimal128;

    bson_decimal128_from_string(ObjStr(value), &decimal128);
    bson_append_decimal128(bbPtr, name, keyLength, &decimal128);
    break;
  }
  case BSON_TYPE_BINARY: {
    int            length;
    const uint8_t *data = Tcl_GetByteArrayFromObj(value, &length);
    bson_append_binary(bbPtr, name, keyLength, 0x00 /*bson_subtype_t*/,
                       data, (uint32_t)length);
    break;
  }
  case BSON_TYPE_DBPOINTER:
  case BSON_TYPE_CODE:
  case BSON_TYPE_SYMBOL:
  case BSON_TYPE_CODEWSCOPE:
    return NsfPrintError(interp, "tag %s not handled yet", ObjStr(tagObj));
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
 *      A standard Tcl result.
 *
 * Side effects:
 *      Value appended to bson buffer.
 *
 *----------------------------------------------------------------------
 */
static int
BsonAppendObjv(Tcl_Interp *interp, bson_t *bPtr, int objc, Tcl_Obj **objv)
{
  int i, result = TCL_OK;

  bson_init(bPtr);
  for (i = 0; i < objc; i += 3) {
    /*fprintf(stderr, "adding pair '%s' (%s) '%s'\n",
      ObjStr(objv[i]), ObjStr(objv[i+1]), ObjStr(objv[i+2]));*/
    result = BsonAppend(interp, bPtr, objv[i], objv[i+1], objv[i+2]);
    if (result != TCL_OK) {
      break;
    }
  }
  return result;
}


/***********************************************************************
 * Define the API functions
 ***********************************************************************/
/*
  cmd json::generate NsfMongoJsonGenerate {
  {-argName "list" -required 1 -type tclobj}
  }
*/
static int
NsfMongoJsonGenerate(Tcl_Interp *interp, Tcl_Obj *listObj)
{
  bson_t    list, *listPtr = &list;
  size_t    length;
  int       result, objc;
  Tcl_Obj **objv;

  result = Tcl_ListObjGetElements(interp, listObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(listObj));
  }

  result = BsonAppendObjv(interp, listPtr, objc, objv);
  if (result == TCL_OK) {
    char     *jsonString;

    jsonString = bson_as_json(listPtr, &length);
    if (jsonString != NULL) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(jsonString, (int)length));
      bson_free(jsonString);
    } else {
      result = NsfPrintError(interp, "invalid bson string: %s", ObjStr(listObj));
    }

    bson_destroy( listPtr );
  }

  return result;
}
/*
  cmd json::parse NsfMongoJsonParse {
  {-argName "json" -required 1 -type tclobj}
  }
*/
static int
NsfMongoJsonParse(Tcl_Interp *interp, Tcl_Obj *jsonObj)
{
  bson_t       bson, *bsonPtr = &bson;
  const char  *jsonString;
  int          result, jsonLength;
  bson_error_t bsonError;

  jsonString = Tcl_GetStringFromObj(jsonObj, &jsonLength);

  if (bson_init_from_json (bsonPtr, jsonString,jsonLength, &bsonError) == true) {
    Tcl_SetObjResult(interp, BsonToList(interp, bsonPtr, 0));
    bson_destroy( bsonPtr );
    result = TCL_OK;
  } else {
    result = NsfPrintError(interp, "mongo::json::parse: error: %s", bsonError.message);
  }

  return result;
}

/*
  cmd close NsfMongoClose {
  {-argName "conn" -required 1 -type mongoc_client_t -withObj 1}
  }
*/
static int
NsfMongoClose(Tcl_Interp *UNUSED(interp), mongoc_client_t *connPtr, Tcl_Obj *connObj)
{
#if defined(USE_CLIENT_POOL)
  mongoc_client_pool_push(mongoClientPool, connPtr);
#else
  mongoc_client_destroy(connPtr);
#endif
  Nsf_PointerDelete(ObjStr(connObj), connPtr, 0);

  return TCL_OK;
}

/*
  cmd connect NsfMongoConnect {
  {-argName "-uri" -required 0 -nrargs 1}
  }
*/
static int
NsfMongoConnect(Tcl_Interp *interp, const char *uri)
{
  char             channelName[POINTER_HANDLE_SIZE];
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
  if (Nsf_PointerAdd(interp, channelName, sizeof(channelName), "mongoc_client_t", clientPtr) != TCL_OK) {
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
               const char *db, Tcl_Obj *cmdObj)
{
  bson_t               cmd, *cmdPtr = &cmd, reply, *replyPtr = &reply;
  mongoc_read_prefs_t *readPrefsPtr = NULL; /* TODO: not used */
  bson_error_t         bsonError;
  int                  result, objc;
  Tcl_Obj            **objv;

  result = Tcl_ListObjGetElements(interp, cmdObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(cmdObj));
  }
  BsonAppendObjv(interp, cmdPtr, objc, objv);

  /*mongo_clear_errors( connPtr );*/
  result = mongoc_client_command_simple( clientPtr, db, cmdPtr, readPrefsPtr, replyPtr, &bsonError);
  bson_destroy( cmdPtr );

  if (withNocomplain == 0 && result == 0) {
    return NsfPrintError(interp, "mongo::run: command '%s' returned error: %s",
                         ObjStr(cmdObj), bsonError.message);
  }

  Tcl_SetObjResult(interp, BsonToList(interp, replyPtr, 0));
  bson_destroy(replyPtr);

  return TCL_OK;
}

/*
  cmd status NsfMongoStatus {
  {-argName "conn" -required 1 -type mongoc_client_t -withObj 1}
  }
*/
static int
NsfMongoStatus(Tcl_Interp *interp, mongoc_client_t *clientPtr, Tcl_Obj *UNUSED(clientObj))
{
  mongoc_read_prefs_t *readPrefs = NULL; /* TODO: not handled */
  bson_t               reply, *replyPtr = &reply;
  bson_error_t         bsonError;
  int                  result = TCL_OK;
  bson_t               cmd = BSON_INITIALIZER;
  bool                 ret;

  BSON_APPEND_INT32(&cmd, "serverStatus", 1);
  ret = mongoc_client_command_simple(clientPtr, "admin", &cmd, readPrefs, replyPtr, &bsonError);
  bson_destroy(&cmd);

  if (likely(ret != 0)) {
    Tcl_SetObjResult(interp, BsonToList(interp, replyPtr, 0));
  } else {
    result = NsfPrintError(interp, "mongo::status: error: %s", bsonError.message);
  }

  bson_destroy(replyPtr);
  return result;
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
                  const char *collectionName)
{
  int                  result = TCL_ERROR;
  mongoc_collection_t *collectionPtr;

  collectionPtr = mongoc_client_get_collection(clientPtr, dbName, collectionName);
  if (collectionPtr != NULL) {
    char buffer[POINTER_HANDLE_SIZE];

    if (Nsf_PointerAdd(interp, buffer, sizeof(buffer), "mongoc_collection_t", collectionPtr) == TCL_OK) {
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
NsfCollectionClose(Tcl_Interp *UNUSED(interp), mongoc_collection_t *collectionPtr, Tcl_Obj *clientObj)
{
  mongoc_collection_destroy(collectionPtr);
  Nsf_PointerDelete(ObjStr(clientObj), collectionPtr, 0);

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
                        Tcl_Obj *queryObj)
{
  int          objc, result;
  int64_t      count;
  Tcl_Obj    **objv;
  bson_t       query, *queryPtr = &query;
  bson_error_t bsonError;
  /*bson_t* opts = BCON_NEW("skip", BCON_INT64(5));*/

  result = Tcl_ListObjGetElements(interp, queryObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(queryObj));
  }

  BsonAppendObjv(interp, queryPtr, objc, objv);

  count = mongoc_collection_count_documents(collectionPtr,
                                            queryPtr,
                                            NULL /* opts */,
                                            NULL /* read preferences */,
                                            NULL /* replyPtr */,
                                            &bsonError);
  if (count == -1) {
    bson_destroy( queryPtr );
    return NsfPrintError(interp, "mongo::collection::count: error: %s", bsonError.message);
  }

  bson_destroy( queryPtr );
  Tcl_SetObjResult(interp, Tcl_NewWideIntObj((Tcl_WideInt)count));

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
                         Tcl_Obj *conditionObj)
{
  int                           objc, result, success;
  Tcl_Obj                     **objv;
  bson_t                        query, *queryPtr = &query;
  bson_error_t                  bsonError;
  mongoc_remove_flags_t         removeFlags = 0; /* TODO: not handled */
  /* MONGOC_DELETE_SINGLE_REMOVE = 1 << 0,**/
  const mongoc_write_concern_t *writeConcern = NULL; /* TODO: not handled yet */

  result = Tcl_ListObjGetElements(interp, conditionObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(conditionObj));
  }

  BsonAppendObjv(interp, queryPtr, objc, objv);
  success = mongoc_collection_remove(collectionPtr, removeFlags, queryPtr, writeConcern, &bsonError);

  if (success == 0) {
    result = NsfPrintError(interp, "mongo::collection::delete: error: %s", bsonError.message);
  }
  bson_destroy(queryPtr);
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
                        const char *withName,
                        int withBackground,
                        int withDropdups,
                        int withSparse,
                        int withTtl,
                        int withUnique)
{
  int                objc, result, success = 0;
  Tcl_Obj          **objv;
  bson_t             keys, *keysPtr = &keys;
  bson_error_t       bsonError;
  mongoc_index_opt_t options;
  bson_t            *create_indexes;
  char              *index_name;
  const char        *collection_name;

  result = Tcl_ListObjGetElements(interp, attributesObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(attributesObj));
  }

  BsonAppendObjv(interp, keysPtr, objc, objv);

  index_name = mongoc_collection_keys_to_index_string(keysPtr);
  collection_name = mongoc_collection_get_name(collectionPtr);

  create_indexes = BCON_NEW("createIndexes",
                            BCON_UTF8(collection_name),
                            "indexes",
                            "[",
                            "{",
                            "key",
                            BCON_DOCUMENT(keysPtr),
                            "name",
                            BCON_UTF8(index_name),
                            "}",
                            "]");
  mongoc_index_opt_init(&options);

  if (withBackground != 0) {options.background = 1;}
  if (withDropdups != 0)   {options.drop_dups = 1;}
  if (withSparse != 0)     {options.sparse = 1;}
  if (withUnique != 0)     {options.unique = 1;}
  if (withTtl != 0)        {options.expire_after_seconds = withTtl;}
  if (withName != 0)       {options.name = withName;}
  /* TODO: not handled: is_initialized, v, weights, default_language, language_override, padding */

  success = mongoc_collection_write_command_with_opts(
                                                    collectionPtr,
                                                    create_indexes,
                                                    NULL /* opts */,
                                                    NULL /*&reply*/,
                                                    &bsonError);
  bson_destroy(keysPtr);
  bson_free(index_name);
  bson_destroy(create_indexes);

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
                                    Tcl_Obj *valuesObj)
{
  int                   i, objc, result, success;
  Tcl_Obj             **objv;
  bson_t                bson, *bsonPtr = &bson;
  bson_oid_t            oid;
  bson_error_t          bsonError;
  mongoc_insert_flags_t insertFlags = MONGOC_INSERT_NO_VALIDATE; /* otherwise, we can't insert a DBRef */

  /* TODO: insertFlags not handled:
     MONGOC_INSERT_NONE              = 0,
     MONGOC_INSERT_CONTINUE_ON_ERROR = 1 << 0,
     MONGOC_INSERT_NO_VALIDATE       = 1 << 31,
  */
  const mongoc_write_concern_t *writeConcern = NULL; /* TODO: not handled yet */

  result = Tcl_ListObjGetElements(interp, valuesObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(valuesObj));
  }

  bson_init(bsonPtr);
  bson_oid_init(&oid, NULL);
  bson_append_oid(bsonPtr, "_id", 3, &oid);

  for (i = 0; i < objc; i += 3) {
    /*fprintf(stderr, "adding pair '%s' (%s) '%s'\n", ObjStr(name), ObjStr(tag), ObjStr(value));*/
    BsonAppend(interp, bsonPtr, objv[i], objv[i+1], objv[i+2]);
  }

  success = mongoc_collection_insert(collectionPtr, insertFlags, bsonPtr, writeConcern, &bsonError);

  if (success == 0) {
    result = NsfPrintError(interp, "mongo::collection::insert: error: %s", bsonError.message);
  } else {
    Tcl_SetObjResult(interp, BsonToList(interp, bsonPtr, 0));
  }

  bson_destroy(bsonPtr);

  return result;
}

/*
  cmd collection::query NsfMongoCollectionQuery {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "filter" -required 1 -type tclobj}
  {-argName "-opts" -required 0 -nrargs 1 -type tclobj}
  }
*/
static int
NsfMongoCollectionQuery(Tcl_Interp *interp,
                        mongoc_collection_t *collectionPtr,
                        Tcl_Obj *filterObj, Tcl_Obj *withOptsObj)
{
  int                  objc1, objc2 = 0, result;
  Tcl_Obj            **objv1, **objv2 = NULL, *resultObj;
  mongoc_cursor_t     *cursor;
  bson_t               filter, *const filterPtr = &filter;
  bson_t               opts,   *const optsPtr   = &opts;
  const bson_t        *nextPtr;
  mongoc_read_prefs_t *readPrefsPtr = NULL; /* TODO: not handled */

  /*fprintf(stderr, "NsfMongoQuery: namespace %s withLimit %d withSkip %d\n",
    namespace, withLimit, withSkip);*/

  result = Tcl_ListObjGetElements(interp, filterObj, &objc1, &objv1);
  if (result != TCL_OK || ((objc1 % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(filterObj));
  }
  if (withOptsObj != NULL) {
    result = Tcl_ListObjGetElements(interp, withOptsObj, &objc2, &objv2);
    if (result != TCL_OK || ((objc2 % 3) != 0)) {
      return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(withOptsObj));
    }
  } else {
    objc2 = 0;
  }

  BsonAppendObjv(interp, filterPtr, objc1, objv1);
  BsonAppendObjv(interp, optsPtr,   objc2, objv2);

  resultObj = Tcl_NewListObj(0, NULL);

  cursor = mongoc_collection_find_with_opts( collectionPtr,
                                             filterPtr,
                                             optsPtr,
                                             readPrefsPtr);

  while( mongoc_cursor_next( cursor, &nextPtr ) == 1 ) {
    Tcl_ListObjAppendElement(interp, resultObj, BsonToList(interp, nextPtr, 0));
  }

  mongoc_cursor_destroy( cursor );
  bson_destroy( filterPtr );
  bson_destroy( optsPtr );

  Tcl_SetObjResult(interp, resultObj);

  return TCL_OK;
}

/*
  cmd "collection::stats" NsfMongoCollectionStats {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "-options" -required 0 -type tclobj}
  }
*/
static int
NsfMongoCollectionStats(Tcl_Interp *interp,
                        mongoc_collection_t *collectionPtr,
                        Tcl_Obj *optionsObj)
{
  int          objc = 0, success, result;
  Tcl_Obj    **objv = NULL;
  bson_t       options, *optionsPtr = NULL;
  bson_t       stats, *statsPtr = &stats;
  bson_t       cmd = BSON_INITIALIZER;
  bson_iter_t  iter;
  bson_error_t bsonError;

  if (optionsObj != NULL) {
    result = Tcl_ListObjGetElements(interp, optionsObj, &objc, &objv);

    if (result != TCL_OK || ((objc % 3) != 0)) {
      return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(optionsObj));
    }
    optionsPtr = &options;
    BsonAppendObjv(interp, optionsPtr, objc, objv);
  }

  if (optionsPtr != NULL && bson_iter_init_find(&iter, optionsPtr, "scale")
      && !BSON_ITER_HOLDS_INT32 (&iter)) {
    bson_set_error(&bsonError,
                    MONGOC_ERROR_BSON,
                    MONGOC_ERROR_BSON_INVALID,
                    "'scale' must be an int32 value.");
    success = 0;
  } else {

    BSON_APPEND_UTF8(&cmd, "collStats", mongoc_collection_get_name(collectionPtr));

    if (optionsPtr != NULL) {
      bson_concat(&cmd, optionsPtr);
    }
    success = mongoc_collection_command_simple(collectionPtr,
                                               &cmd,
                                               mongoc_collection_get_read_prefs(collectionPtr),
                                               statsPtr,
                                               &bsonError);
    bson_destroy(&cmd);
  }

  if (optionsPtr != NULL) {
    bson_destroy(optionsPtr);
  }

  if (success != 0) {
    Tcl_SetObjResult(interp, BsonToList(interp, statsPtr, 0));
    bson_destroy(statsPtr);
    result = TCL_OK;
  } else {
    result = NsfPrintError(interp, "mongo::collection::stats: error: %s", bsonError.message);
  }
  return result;
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
  mongoc_update_flags_t         updateFlags =  MONGOC_UPDATE_NO_VALIDATE; /* for dbrefs */
  bson_error_t                  bsonError;
  bson_t                        cond, *condPtr = &cond, values, *valuesPtr = &values;
  int                           objc, result, success;
  Tcl_Obj                     **objv;

  result = Tcl_ListObjGetElements(interp, conditionObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(conditionObj));
  }

  BsonAppendObjv(interp, condPtr, objc, objv);

  result = Tcl_ListObjGetElements(interp, valuesObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    bson_destroy(condPtr);
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(valuesObj));
  }

  BsonAppendObjv(interp, valuesPtr, objc, objv);

  if (withUpsert != 0) {updateFlags |= MONGOC_UPDATE_UPSERT;}
  if (withAll != 0)    {updateFlags |= MONGOC_UPDATE_MULTI_UPDATE;}

  success = mongoc_collection_update(collectionPtr, updateFlags, condPtr, valuesPtr, writeConcern, &bsonError);

  if (success == 0) {
    result = NsfPrintError(interp, "mongo::collection::delete: error: %s", bsonError.message);
  }

  return result;
}

/***********************************************************************
 * Cursor interface
 ***********************************************************************/
/*
  cmd cursor::aggregate NsfMongoCursorAggregate {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "pipeline" -required 1 -type tclobj}
  {-argName "options" -required 1 -type tclobj}
  {-argName "-tailable" -required 0 -nrargs 0}
  {-argName "-awaitdata" -required 0 -nrargs 0}
  }
*/
static int
NsfMongoCursorAggregate(Tcl_Interp *interp,
                        mongoc_collection_t *collectionPtr,
                        Tcl_Obj *pipelineObj,
                        Tcl_Obj *optionsObj,
                        int withTailable,
                        int withAwaitdata)
{
  int                  objc1, objc2, result;
  mongoc_query_flags_t queryFlags = 0;
  Tcl_Obj            **objv1, **objv2 = NULL;
  mongoc_cursor_t     *cursor;
  bson_t               pipeline, *pipelinePtr = &pipeline;
  bson_t               options,  *optionsPtr  = &options;
  mongoc_read_prefs_t *readPrefsPtr = NULL; /* TODO: not used */

  result = Tcl_ListObjGetElements(interp, pipelineObj, &objc1, &objv1);
  if (result != TCL_OK || ((objc1 % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(pipelineObj));
  }
  result = Tcl_ListObjGetElements(interp, optionsObj, &objc2, &objv2);
  if (result != TCL_OK || ((objc2 % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(optionsObj));
  }

  BsonAppendObjv(interp, pipelinePtr, objc1, objv1);
  BsonAppendObjv(interp, optionsPtr,  objc2, objv2);

  /*
   *  The last field of mongo_find is options, semantics are described here
   *  https://www.mongodb.org/display/DOCS/Mongo+Wire+Protocol#MongoWireProtocol-OPQUERY
   */
  if (withTailable != 0) {
    queryFlags |= MONGOC_QUERY_TAILABLE_CURSOR;
  }
  if (withAwaitdata != 0) {
    queryFlags |= MONGOC_QUERY_AWAIT_DATA;
  }
  /* TODO: query flags:
     MONGOC_QUERY_SLAVE_OK          = 1 << 2,
     MONGOC_QUERY_OPLOG_REPLAY      = 1 << 3,
     MONGOC_QUERY_NO_CURSOR_TIMEOUT = 1 << 4,
     MONGOC_QUERY_EXHAUST           = 1 << 6,
     MONGOC_QUERY_PARTIAL           = 1 << 7,
  */
  cursor = mongoc_collection_aggregate(collectionPtr, queryFlags,
                                       pipelinePtr, optionsPtr,
                                       readPrefsPtr);
  if (cursor != NULL) {
    char buffer[POINTER_HANDLE_SIZE];

    if (Nsf_PointerAdd(interp, buffer, sizeof(buffer), "mongoc_cursor_t", cursor) == TCL_OK) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
    } else {
      mongoc_cursor_destroy( cursor );
      result = TCL_ERROR;
    }
  } else {
    Tcl_ResetResult(interp);
  }

  bson_destroy( pipelinePtr );
  bson_destroy( optionsPtr );

  return result;
}

/*
  cmd cursor::find NsfMongoCursorFind {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "filter" -required 1 -type tclobj}
  {-argName "-opts" -required 0 -nrargs 1 -type tclobj}
  }
*/
static int
NsfMongoCursorFind(Tcl_Interp *interp,
                   mongoc_collection_t *collectionPtr,
                   Tcl_Obj *filterObj,
                   Tcl_Obj *withOptsObj)
{
  int                  objc1, objc2 = 0, result;
  Tcl_Obj            **objv1, **objv2 = NULL;
  mongoc_cursor_t     *cursor;
  bson_t               filter, *filterPtr = &filter;
  bson_t               opts,   *optsPtr  = &opts;
  mongoc_read_prefs_t *readPrefsPtr = NULL; /* TODO: not used */

  /*fprintf(stderr, "NsfMongoQuery: namespace %s withLimit %d withSkip %d\n",
    namespace, withLimit, withSkip);*/

  result = Tcl_ListObjGetElements(interp, filterObj, &objc1, &objv1);
  if (result != TCL_OK || ((objc1 % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(filterObj));
  }
  if (withOptsObj != NULL) {
    result = Tcl_ListObjGetElements(interp, withOptsObj, &objc2, &objv2);
    if (result != TCL_OK || ((objc2 % 3) != 0)) {
      return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(withOptsObj));
    }
  }

  BsonAppendObjv(interp, filterPtr, objc1, objv1);
  BsonAppendObjv(interp, optsPtr,   objc2, objv2);

  cursor = mongoc_collection_find_with_opts( collectionPtr,
                                             filterPtr,
                                             optsPtr,
                                             readPrefsPtr);

  if (cursor != NULL) {
    char buffer[POINTER_HANDLE_SIZE];
    if (Nsf_PointerAdd(interp, buffer, sizeof(buffer), "mongoc_cursor_t", cursor) == TCL_OK) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
    } else {
      mongoc_cursor_destroy( cursor );
      result = TCL_ERROR;
    }
  } else {
    Tcl_ResetResult(interp);
  }

  bson_destroy( filterPtr );
  bson_destroy( optsPtr );

  return result;
}

/*
  cmd cursor::next NsfMongoCursorNext {
  {-argName "cursor" -required 1 -type mongoc_cursor_t}
  }
*/
static int
NsfMongoCursorNext(Tcl_Interp *interp, mongoc_cursor_t *cursor)
{
  int           result;
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
NsfMongoCursorClose(Tcl_Interp *UNUSED(interp), mongoc_cursor_t *cursor, Tcl_Obj *cursorObj)
{
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
NsfMongoGridFSClose(Tcl_Interp *UNUSED(interp), mongoc_gridfs_t *gridfsPtr, Tcl_Obj *gridfsObj)
{
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
                   const char *dbname, const char *prefix)
{
  char             buffer[POINTER_HANDLE_SIZE];
  int              result = TCL_OK;
  bson_error_t     bsonError;
  mongoc_gridfs_t *gfsPtr;

  gfsPtr = mongoc_client_get_gridfs(clientPtr, dbname, prefix, &bsonError);

  if (gfsPtr == NULL) {
    result = NsfPrintError(interp, "mongo::gridfs::open: error: %s", bsonError.message);
  }

  if (Nsf_PointerAdd(interp, buffer, sizeof(buffer), "mongoc_gridfs_t", gfsPtr) == TCL_OK) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
  } else {
    mongoc_gridfs_destroy(gfsPtr);
    result = TCL_ERROR;
  }

  return result;
}

/***********************************************************************
 * GridFile interface operating on GridFS
 ***********************************************************************/

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
NsfMongoGridFileCreate(Tcl_Interp *interp,
                       GridfilesourceIdx_t withSource,
                       mongoc_gridfs_t *gridfsPtr,
                       const char *value,
                       const char *name,
                       const char *contenttype,
                       Tcl_Obj *withMetadata
                       )
{
  int                      result = TCL_OK;
  mongoc_gridfs_file_opt_t fileOpts ;
  mongoc_gridfs_file_t    *gridFile;
  bson_t                   bsonMetaData, *bsonMetaDataPtr = &bsonMetaData;

  memset(&fileOpts, 0, sizeof(fileOpts));

  if (withSource == GridfilesourceNULL) {
    withSource = GridfilesourceFileIdx;
  }

  if (withMetadata != NULL) {
    Tcl_Obj **objv;
    int objc;

    result = Tcl_ListObjGetElements(interp, withMetadata, &objc, &objv);
    if (result != TCL_OK || ((objc % 3) != 0)) {
      return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(withMetadata));
    }
    BsonAppendObjv(interp, bsonMetaDataPtr, objc, objv);
    fileOpts.metadata = bsonMetaDataPtr;
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
    mongoc_iovec_t iov = { buf, 0 };
    int fd = open(value, O_RDONLY);

    if (fd < 1) {
      mongoc_gridfs_file_destroy(gridFile);
      return NsfPrintError(interp, "nsf::gridfile::create: cannot open file '%s' for reading", value);
    }

    for (;;) {
      ssize_t n = read(fd, iov.iov_base, MONGOC_GRIDFS_READ_CHUNK);

      if (n > 0) {
        iov.iov_len = (size_t)n;
        n = mongoc_gridfs_file_writev(gridFile, &iov, 1, 0);
        if ((size_t)n != iov.iov_len) {
          NsfLog(interp, NSF_LOG_WARN, "mongodb: write of %d bytes returned %d", iov.iov_len, n);
        }
      } else if (n == 0) {
        break;
      } else {
        result = TCL_ERROR;
        break;
      }
    }
    close(fd);
  } else {
    mongoc_iovec_t iov = { (char *)value, strlen(value) };
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
                       Tcl_Obj *queryObj)
{
  bson_t               query, *queryPtr = &query;
  mongoc_cursor_t     *files;
  const bson_t        *nextPtr;
  bson_iter_t          it;
  Tcl_Obj            **objv;
  int                  objc, result;
  mongoc_read_prefs_t *readPrefsPtr = NULL; /* TODO: not handled */

  result = Tcl_ListObjGetElements(interp, queryObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(queryObj));
  }

  BsonAppendObjv(interp, queryPtr, objc, objv);
  files = mongoc_collection_find_with_opts( mongoc_gridfs_get_files(gridfsPtr),
                                            queryPtr, NULL, readPrefsPtr);
  bson_destroy(queryPtr);

  /*
   * Files should be a valid cursor even if the file doesn't exist.
   */
  if ( files == NULL ) {
    return NsfPrintError(interp, "gridfs::remove_file: invalid cursor for files");
  }

  /*
   * Remove each file and it's chunks from files named filename.
   */
  while (mongoc_cursor_next(files, &nextPtr)) {
    bson_t       bson, *bsonPtr = &bson;
    bson_error_t bsonError;
    bson_oid_t   id;

    bson_iter_init_find(&it, nextPtr, "_id");
    id = *bson_iter_oid(&it);

    /*
     * Remove the file with the specified id.
     */
    bson_init(bsonPtr);
    bson_append_oid(bsonPtr, "_id", 3, &id);
    mongoc_collection_remove(mongoc_gridfs_get_files(gridfsPtr), 0, bsonPtr, NULL, &bsonError);
    bson_destroy(bsonPtr);

    /*
     * Remove all chunks from the file with the specified id.
     */
    bson_init(bsonPtr);
    bson_append_oid(bsonPtr, "files_id", 8, &id);
    mongoc_collection_remove(mongoc_gridfs_get_chunks(gridfsPtr), 0, bsonPtr, NULL, &bsonError);
    bson_destroy(bsonPtr);
  }

  mongoc_cursor_destroy(files);
  return TCL_OK;
}

/*
  cmd gridfile::open NsfMongoGridFileOpen {
  {-argName "gfs" -required 1 -type mongoc_gridfs_t}
  {-argName "filter" -required 1 -type tclobj}
  }
*/
static int
NsfMongoGridFileOpen(Tcl_Interp *interp,
                     mongoc_gridfs_t *gridfsPtr,
                     Tcl_Obj *filterObj)
{
  mongoc_gridfs_file_t* gridFilePtr;
  bson_error_t          bsonError;
  bson_t                filter, *filterPtr = &filter;
  int                   result, objc;
  Tcl_Obj             **objv;

  /*fprintf(stderr, "NsfMongoFilter: namespace %s withLimit %d withSkip %d\n",
    namespace, withLimit, withSkip);*/

  result = Tcl_ListObjGetElements(interp, filterObj, &objc, &objv);
  if (result != TCL_OK || ((objc % 3) != 0)) {
    return NsfPrintError(interp, "%s: must contain a multiple of 3 elements", ObjStr(filterObj));
  }

  BsonAppendObjv(interp, filterPtr, objc, objv);

  gridFilePtr = mongoc_gridfs_find_one_with_opts(gridfsPtr, filterPtr, NULL, &bsonError);

  if (gridFilePtr != NULL) {
    char buffer[POINTER_HANDLE_SIZE];

    if (Nsf_PointerAdd(interp, buffer, sizeof(buffer), "mongoc_gridfs_file_t", gridFilePtr) == TCL_OK) {
      Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
    } else {
      mongoc_gridfs_file_destroy(gridFilePtr);
      result = TCL_ERROR;
    }
  } else {
    Tcl_ResetResult(interp);
  }

  bson_destroy(filterPtr);
  return result;
}


/***********************************************************************
 * GridFile interface
 *
 * Currently offsets and sizes are limited to 32-bit integers, we should
 * relax this later.
 ***********************************************************************/

/*
  cmd gridfile::close NsfMongoGridFileClose {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t -withObj 1}
  }
*/
static int
NsfMongoGridFileClose(Tcl_Interp *UNUSED(interp), mongoc_gridfs_file_t* gridFilePtr, Tcl_Obj *gridFileObj)
{
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
NsfMongoGridFileGetContentlength(Tcl_Interp *interp, mongoc_gridfs_file_t* gridFilePtr)
{
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
NsfMongoGridFileGetContentType(Tcl_Interp *interp, mongoc_gridfs_file_t* gridFilePtr)
{
  Tcl_SetObjResult(interp, Tcl_NewStringObj(mongoc_gridfs_file_get_content_type(gridFilePtr), -1));

  return TCL_OK;
}



/*
  cmd gridfile::get_metadata NsfMongoGridFileGetMetaData {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
  }
*/
static int
NsfMongoGridFileGetMetaData(Tcl_Interp *interp, mongoc_gridfs_file_t* gridFilePtr)
{
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
NsfMongoGridFileRead(Tcl_Interp *interp, mongoc_gridfs_file_t *gridFilePtr, int size)
{
  ssize_t      readSize;
  Tcl_Obj     *resultObj = Tcl_NewByteArrayObj(NULL, size);
  mongoc_iovec_t iov = { NULL, (size_t)size };

  assert(size > 0);

  iov.iov_base = Tcl_SetByteArrayLength(resultObj, size);

  readSize = mongoc_gridfs_file_readv(gridFilePtr, &iov, 1,
                                      0 /* min_bytes */,
                                      0 /* timeout_msec */);
  /*fprintf(stderr, "NsfMongoGridFileRead want %d got %d\n", size, readSize);*/
  Tcl_SetByteArrayLength(resultObj, (int)readSize);
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
NsfMongoGridFileSeek(Tcl_Interp *UNUSED(interp), mongoc_gridfs_file_t *gridFilePtr, int offset)
{
  int result;

  /*
   * TODO: whence SEEK_SET, SEEK_CUR or SEEK_END; implementation of SEEK_END looks incorrect
   */
  result = mongoc_gridfs_file_seek(gridFilePtr, offset, SEEK_SET);

  return result < 0 ? TCL_ERROR : TCL_OK;
}

/***********************************************************************
 * Finally, provide the necessary Tcl package interface.
 ***********************************************************************/

static void
Nsfmongo_ThreadExit(ClientData UNUSED(clientData))
{
  /*
   * The exit might happen at a time, when Tcl is already shut down.
   * We can't reliably call NsfLog.
   */

  fprintf(stderr, "+++ Nsfmongo_ThreadExit\n");

#if defined(USE_CLIENT_POOL)
  NsfMutexLock(&poolMutex);
  mongoClientPoolRefCount --;
  if (mongoClientPool != NULL) {
    /*fprintf(stderr, "========= Nsfmongo_ThreadExit mongoClientPoolRefCount %d\n", mongoClientPoolRefCount);*/
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

static void
Nsfmongo_Exit(ClientData clientData)
{
  /*
   * The exit might happen at a time, when Tcl is already shut down.
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

  /*
   * Release the state of mongo-c-driver explicitly.
   */
  mongoc_cleanup();
}


extern int
Nsfmongo_Init(Tcl_Interp * interp)
{
  int             i;
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
   * Register global mongo Tcl_Objs once.
   */
  NsfMutexLock(&initMutex);
  if (NsfMongoGlobalObjs == NULL) {
    NsfMongoGlobalObjs = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*)*nr_elements(NsfMongoGlobalStrings));

    for (i = 0; i < nr_elements(NsfMongoGlobalStrings); i++) {
      NsfMongoGlobalObjs[i] = Tcl_NewStringObj(NsfMongoGlobalStrings[i], -1);
      Tcl_IncrRefCount(NsfMongoGlobalObjs[i]);
    }

    /*
     * Initializing state of mongo-c-driver explicitly.
     */
    mongoc_init();
  }
  NsfMutexUnlock(&initMutex);

  Nsf_EnumerationTypeRegister(interp, enumeratorConverterEntries);
  Nsf_CmdDefinitionRegister(interp, method_definitions);

  /*
   * Register the pointer converter.
   */
  Nsf_PointerTypeRegister(interp, "mongoc_client_t",      &mongoClientCount);
  Nsf_PointerTypeRegister(interp, "mongoc_collection_t",  &mongoCollectionCount);
  Nsf_PointerTypeRegister(interp, "mongoc_cursor_t",      &mongoCursorCount);
  Nsf_PointerTypeRegister(interp, "mongoc_gridfs_file_t", &gridfileCount);
  Nsf_PointerTypeRegister(interp, "mongoc_gridfs_t",      &gridfsCount);

  for (i=0; i < nr_elements(method_command_namespace_names); i++) {
    Tcl_CreateNamespace(interp, method_command_namespace_names[i], 0, (Tcl_NamespaceDeleteProc *)NULL);
  }

  /*
   * Create all method commands (will use the namespaces above)
   */
  for (i = 0; i < nr_elements(method_definitions)-1; i++) {
    Tcl_CreateObjCommand(interp, method_definitions[i].methodName, method_definitions[i].proc, 0, 0);
  }

  Tcl_SetIntObj(Tcl_GetObjResult(interp), 1);

  return TCL_OK;
}

extern int
Nsfmongo_SafeInit( Tcl_Interp *interp) {
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
