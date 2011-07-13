

/* just to define the symbol */
static Nsf_methodDefinition method_definitions[];
  
static CONST char *method_command_namespace_names[] = {
  "::mongo"
};
static int NsfMongoCloseStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoConnectStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoCountStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFSCloseStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFSOpenStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFSRemoveFileStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFSStoreFileStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFileCloseStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFileGetContentTypeStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFileGetContentlengthStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFileGetMetaDataStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFileOpenStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFileReadStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoGridFileSeekStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoIndexStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoInsertStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoQueryStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoRemoveStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoUpdateStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

static int NsfMongoClose(Tcl_Interp *interp, mongo_connection *connPtr, Tcl_Obj *connObj);
static int NsfMongoConnect(Tcl_Interp *interp, CONST char *withReplica_set, Tcl_Obj *withServer, int withTimeout);
static int NsfMongoCount(Tcl_Interp *interp, mongo_connection *connPtr, CONST char *namespace, Tcl_Obj *query);
static int NsfMongoGridFSClose(Tcl_Interp *interp, gridfs *gfsPtr, Tcl_Obj *gfsObj);
static int NsfMongoGridFSOpen(Tcl_Interp *interp, mongo_connection *connPtr, CONST char *dbname, CONST char *prefix);
static int NsfMongoGridFSRemoveFile(Tcl_Interp *interp, gridfs *gfsPtr, CONST char *filename);
static int NsfMongoGridFSStoreFile(Tcl_Interp *interp, gridfs *gfsPtr, CONST char *filename, CONST char *remotename, CONST char *contenttype);
static int NsfMongoGridFileClose(Tcl_Interp *interp, gridfile *filePtr, Tcl_Obj *fileObj);
static int NsfMongoGridFileGetContentType(Tcl_Interp *interp, gridfile *filePtr);
static int NsfMongoGridFileGetContentlength(Tcl_Interp *interp, gridfile *filePtr);
static int NsfMongoGridFileGetMetaData(Tcl_Interp *interp, gridfile *filePtr);
static int NsfMongoGridFileOpen(Tcl_Interp *interp, gridfs *fsPtr, CONST char *filename);
static int NsfMongoGridFileRead(Tcl_Interp *interp, gridfile *filePtr, int size);
static int NsfMongoGridFileSeek(Tcl_Interp *interp, gridfile *filePtr, int offset);
static int NsfMongoIndex(Tcl_Interp *interp, mongo_connection *connPtr, CONST char *namespace, Tcl_Obj *attributes, int withBackground, int withDropdups, int withSparse, int withUnique);
static int NsfMongoInsert(Tcl_Interp *interp, mongo_connection *connPtr, CONST char *namespace, Tcl_Obj *values);
static int NsfMongoQuery(Tcl_Interp *interp, mongo_connection *connPtr, CONST char *namespace, Tcl_Obj *query, Tcl_Obj *withAtts, int withLimit, int withSkip);
static int NsfMongoRemove(Tcl_Interp *interp, mongo_connection *connPtr, CONST char *namespace, Tcl_Obj *condition);
static int NsfMongoUpdate(Tcl_Interp *interp, mongo_connection *connPtr, CONST char *namespace, Tcl_Obj *cond, Tcl_Obj *values, int withUpsert, int withAll);

enum {
 NsfMongoCloseIdx,
 NsfMongoConnectIdx,
 NsfMongoCountIdx,
 NsfMongoGridFSCloseIdx,
 NsfMongoGridFSOpenIdx,
 NsfMongoGridFSRemoveFileIdx,
 NsfMongoGridFSStoreFileIdx,
 NsfMongoGridFileCloseIdx,
 NsfMongoGridFileGetContentTypeIdx,
 NsfMongoGridFileGetContentlengthIdx,
 NsfMongoGridFileGetMetaDataIdx,
 NsfMongoGridFileOpenIdx,
 NsfMongoGridFileReadIdx,
 NsfMongoGridFileSeekIdx,
 NsfMongoIndexIdx,
 NsfMongoInsertIdx,
 NsfMongoQueryIdx,
 NsfMongoRemoveIdx,
 NsfMongoUpdateIdx
} NsfMethods;


static int
NsfMongoCloseStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoCloseIdx].paramDefs, 
                     method_definitions[NsfMongoCloseIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    mongo_connection *connPtr = (mongo_connection *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfMongoClose(interp, connPtr,pc.objv[0]);

  }
}

static int
NsfMongoConnectStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoConnectIdx].paramDefs, 
                     method_definitions[NsfMongoConnectIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    CONST char *withReplica_set = (CONST char *)pc.clientData[0];
    Tcl_Obj *withServer = (Tcl_Obj *)pc.clientData[1];
    int withTimeout = (int )PTR2INT(pc.clientData[2]);

    assert(pc.status == 0);
    return NsfMongoConnect(interp, withReplica_set, withServer, withTimeout);

  }
}

static int
NsfMongoCountStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoCountIdx].paramDefs, 
                     method_definitions[NsfMongoCountIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    mongo_connection *connPtr = (mongo_connection *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *query = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfMongoCount(interp, connPtr, namespace, query);

  }
}

static int
NsfMongoGridFSCloseStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFSCloseIdx].paramDefs, 
                     method_definitions[NsfMongoGridFSCloseIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfs *gfsPtr = (gridfs *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfMongoGridFSClose(interp, gfsPtr,pc.objv[0]);

  }
}

static int
NsfMongoGridFSOpenStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFSOpenIdx].paramDefs, 
                     method_definitions[NsfMongoGridFSOpenIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    mongo_connection *connPtr = (mongo_connection *)pc.clientData[0];
    CONST char *dbname = (CONST char *)pc.clientData[1];
    CONST char *prefix = (CONST char *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfMongoGridFSOpen(interp, connPtr, dbname, prefix);

  }
}

static int
NsfMongoGridFSRemoveFileStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFSRemoveFileIdx].paramDefs, 
                     method_definitions[NsfMongoGridFSRemoveFileIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfs *gfsPtr = (gridfs *)pc.clientData[0];
    CONST char *filename = (CONST char *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfMongoGridFSRemoveFile(interp, gfsPtr, filename);

  }
}

static int
NsfMongoGridFSStoreFileStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFSStoreFileIdx].paramDefs, 
                     method_definitions[NsfMongoGridFSStoreFileIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfs *gfsPtr = (gridfs *)pc.clientData[0];
    CONST char *filename = (CONST char *)pc.clientData[1];
    CONST char *remotename = (CONST char *)pc.clientData[2];
    CONST char *contenttype = (CONST char *)pc.clientData[3];

    assert(pc.status == 0);
    return NsfMongoGridFSStoreFile(interp, gfsPtr, filename, remotename, contenttype);

  }
}

static int
NsfMongoGridFileCloseStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFileCloseIdx].paramDefs, 
                     method_definitions[NsfMongoGridFileCloseIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfile *filePtr = (gridfile *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfMongoGridFileClose(interp, filePtr,pc.objv[0]);

  }
}

static int
NsfMongoGridFileGetContentTypeStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFileGetContentTypeIdx].paramDefs, 
                     method_definitions[NsfMongoGridFileGetContentTypeIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfile *filePtr = (gridfile *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfMongoGridFileGetContentType(interp, filePtr);

  }
}

static int
NsfMongoGridFileGetContentlengthStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFileGetContentlengthIdx].paramDefs, 
                     method_definitions[NsfMongoGridFileGetContentlengthIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfile *filePtr = (gridfile *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfMongoGridFileGetContentlength(interp, filePtr);

  }
}

static int
NsfMongoGridFileGetMetaDataStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFileGetMetaDataIdx].paramDefs, 
                     method_definitions[NsfMongoGridFileGetMetaDataIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfile *filePtr = (gridfile *)pc.clientData[0];

    assert(pc.status == 0);
    return NsfMongoGridFileGetMetaData(interp, filePtr);

  }
}

static int
NsfMongoGridFileOpenStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFileOpenIdx].paramDefs, 
                     method_definitions[NsfMongoGridFileOpenIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfs *fsPtr = (gridfs *)pc.clientData[0];
    CONST char *filename = (CONST char *)pc.clientData[1];

    assert(pc.status == 0);
    return NsfMongoGridFileOpen(interp, fsPtr, filename);

  }
}

static int
NsfMongoGridFileReadStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFileReadIdx].paramDefs, 
                     method_definitions[NsfMongoGridFileReadIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfile *filePtr = (gridfile *)pc.clientData[0];
    int size = (int )PTR2INT(pc.clientData[1]);

    assert(pc.status == 0);
    return NsfMongoGridFileRead(interp, filePtr, size);

  }
}

static int
NsfMongoGridFileSeekStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoGridFileSeekIdx].paramDefs, 
                     method_definitions[NsfMongoGridFileSeekIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    gridfile *filePtr = (gridfile *)pc.clientData[0];
    int offset = (int )PTR2INT(pc.clientData[1]);

    assert(pc.status == 0);
    return NsfMongoGridFileSeek(interp, filePtr, offset);

  }
}

static int
NsfMongoIndexStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoIndexIdx].paramDefs, 
                     method_definitions[NsfMongoIndexIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    mongo_connection *connPtr = (mongo_connection *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *attributes = (Tcl_Obj *)pc.clientData[2];
    int withBackground = (int )PTR2INT(pc.clientData[3]);
    int withDropdups = (int )PTR2INT(pc.clientData[4]);
    int withSparse = (int )PTR2INT(pc.clientData[5]);
    int withUnique = (int )PTR2INT(pc.clientData[6]);

    assert(pc.status == 0);
    return NsfMongoIndex(interp, connPtr, namespace, attributes, withBackground, withDropdups, withSparse, withUnique);

  }
}

static int
NsfMongoInsertStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoInsertIdx].paramDefs, 
                     method_definitions[NsfMongoInsertIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    mongo_connection *connPtr = (mongo_connection *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *values = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfMongoInsert(interp, connPtr, namespace, values);

  }
}

static int
NsfMongoQueryStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoQueryIdx].paramDefs, 
                     method_definitions[NsfMongoQueryIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    mongo_connection *connPtr = (mongo_connection *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *query = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj *withAtts = (Tcl_Obj *)pc.clientData[3];
    int withLimit = (int )PTR2INT(pc.clientData[4]);
    int withSkip = (int )PTR2INT(pc.clientData[5]);

    assert(pc.status == 0);
    return NsfMongoQuery(interp, connPtr, namespace, query, withAtts, withLimit, withSkip);

  }
}

static int
NsfMongoRemoveStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoRemoveIdx].paramDefs, 
                     method_definitions[NsfMongoRemoveIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    mongo_connection *connPtr = (mongo_connection *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *condition = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfMongoRemove(interp, connPtr, namespace, condition);

  }
}

static int
NsfMongoUpdateStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  ParseContext pc;
  (void)clientData;

  if (ArgumentParse(interp, objc, objv, NULL, objv[0], 
                     method_definitions[NsfMongoUpdateIdx].paramDefs, 
                     method_definitions[NsfMongoUpdateIdx].nrParameters, 1,
                     &pc) != TCL_OK) {
    return TCL_ERROR;
  } else {
    mongo_connection *connPtr = (mongo_connection *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *cond = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj *values = (Tcl_Obj *)pc.clientData[3];
    int withUpsert = (int )PTR2INT(pc.clientData[4]);
    int withAll = (int )PTR2INT(pc.clientData[5]);

    assert(pc.status == 0);
    return NsfMongoUpdate(interp, connPtr, namespace, cond, values, withUpsert, withAll);

  }
}

static Nsf_methodDefinition method_definitions[] = {
{"::mongo::close", NsfMongoCloseStub, 1, {
  {"conn", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"mongo_connection",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::connect", NsfMongoConnectStub, 3, {
  {"-replica-set", 0, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-server", 0, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-timeout", 0, 1, Nsf_ConvertToInt32, NULL,NULL,"int32",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::count", NsfMongoCountStub, 3, {
  {"conn", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"mongo_connection",NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"query", NSF_ARG_REQUIRED, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfs::close", NsfMongoGridFSCloseStub, 1, {
  {"gfs", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfs",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfs::open", NsfMongoGridFSOpenStub, 3, {
  {"conn", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"mongo_connection",NULL,NULL,NULL,NULL,NULL},
  {"dbname", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"prefix", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfs::remove_file", NsfMongoGridFSRemoveFileStub, 2, {
  {"gfs", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfs",NULL,NULL,NULL,NULL,NULL},
  {"filename", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfs::store_file", NsfMongoGridFSStoreFileStub, 4, {
  {"gfs", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfs",NULL,NULL,NULL,NULL,NULL},
  {"filename", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"remotename", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"contenttype", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfile::close", NsfMongoGridFileCloseStub, 1, {
  {"file", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfile",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfile::get_contenttype", NsfMongoGridFileGetContentTypeStub, 1, {
  {"file", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfile",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfile::get_contentlength", NsfMongoGridFileGetContentlengthStub, 1, {
  {"file", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfile",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfile::get_metadata", NsfMongoGridFileGetMetaDataStub, 1, {
  {"file", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfile",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfile::open", NsfMongoGridFileOpenStub, 2, {
  {"fs", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfs",NULL,NULL,NULL,NULL,NULL},
  {"filename", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfile::read", NsfMongoGridFileReadStub, 2, {
  {"file", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfile",NULL,NULL,NULL,NULL,NULL},
  {"size", NSF_ARG_REQUIRED, 1, Nsf_ConvertToInt32, NULL,NULL,"int32",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::gridfile::seek", NsfMongoGridFileSeekStub, 2, {
  {"file", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"gridfile",NULL,NULL,NULL,NULL,NULL},
  {"offset", NSF_ARG_REQUIRED, 1, Nsf_ConvertToInt32, NULL,NULL,"int32",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::index", NsfMongoIndexStub, 7, {
  {"conn", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"mongo_connection",NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"attributes", NSF_ARG_REQUIRED, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-background", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-dropdups", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-sparse", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-unique", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::insert", NsfMongoInsertStub, 3, {
  {"conn", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"mongo_connection",NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"values", NSF_ARG_REQUIRED, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::query", NsfMongoQueryStub, 6, {
  {"conn", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"mongo_connection",NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"query", NSF_ARG_REQUIRED, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-atts", 0, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-limit", 0, 1, Nsf_ConvertToInt32, NULL,NULL,"int32",NULL,NULL,NULL,NULL,NULL},
  {"-skip", 0, 1, Nsf_ConvertToInt32, NULL,NULL,"int32",NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::remove", NsfMongoRemoveStub, 3, {
  {"conn", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"mongo_connection",NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"condition", NSF_ARG_REQUIRED, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::update", NsfMongoUpdateStub, 6, {
  {"conn", NSF_ARG_REQUIRED, 1, Nsf_ConvertToPointer, NULL,NULL,"mongo_connection",NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"cond", NSF_ARG_REQUIRED, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"values", NSF_ARG_REQUIRED, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-upsert", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-all", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},{NULL}
};

