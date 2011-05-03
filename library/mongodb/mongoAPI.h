

/* just to define the symbol */
static Nsf_methodDefinition method_definitions[];
  
static CONST char *method_command_namespace_names[] = {
  "::nsf::methods::object::info",
  "::nsf::methods::object",
  "::mongo",
  "::nsf::cmd::ParameterType",
  "::nsf::methods::class::info",
  "::nsf::methods::class"
};
static int NsfMongoCloseStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoConnectStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoCountStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoIndexStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoInsertStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoQueryStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoRemoveStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);
static int NsfMongoUpdateStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv []);

static int NsfMongoClose(Tcl_Interp *interp, Tcl_Obj *conn);
static int NsfMongoConnect(Tcl_Interp *interp, CONST char *withReplica_set, Tcl_Obj *withServer);
static int NsfMongoCount(Tcl_Interp *interp, Tcl_Obj *conn, CONST char *namespace, Tcl_Obj *query);
static int NsfMongoIndex(Tcl_Interp *interp, Tcl_Obj *conn, CONST char *namespace, Tcl_Obj *attributes, int withDropdups, int withUnique);
static int NsfMongoInsert(Tcl_Interp *interp, Tcl_Obj *conn, CONST char *namespace, Tcl_Obj *values);
static int NsfMongoQuery(Tcl_Interp *interp, Tcl_Obj *conn, CONST char *namespace, Tcl_Obj *query, Tcl_Obj *withAtts, int withLimit, int withSkip);
static int NsfMongoRemove(Tcl_Interp *interp, Tcl_Obj *conn, CONST char *namespace, Tcl_Obj *condition);
static int NsfMongoUpdate(Tcl_Interp *interp, Tcl_Obj *conn, CONST char *namespace, Tcl_Obj *cond, Tcl_Obj *values, int withUpsert, int withAll);

enum {
 NsfMongoCloseIdx,
 NsfMongoConnectIdx,
 NsfMongoCountIdx,
 NsfMongoIndexIdx,
 NsfMongoInsertIdx,
 NsfMongoQueryIdx,
 NsfMongoRemoveIdx,
 NsfMongoUpdateIdx
} NsfMethods;


static int
NsfMongoCloseStub(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
  (void)clientData;

    

      if (objc != 2) {
	return NsfArgumentError(interp, "wrong # of arguments:", 
			     method_definitions[NsfMongoCloseIdx].paramDefs,
			     NULL, objv[0]); 
      }
    
    return NsfMongoClose(interp, objc == 2 ? objv[1] : NULL);

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

    assert(pc.status == 0);
    return NsfMongoConnect(interp, withReplica_set, withServer);

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
    Tcl_Obj *conn = (Tcl_Obj *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *query = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfMongoCount(interp, conn, namespace, query);

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
    Tcl_Obj *conn = (Tcl_Obj *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *attributes = (Tcl_Obj *)pc.clientData[2];
    int withDropdups = (int )PTR2INT(pc.clientData[3]);
    int withUnique = (int )PTR2INT(pc.clientData[4]);

    assert(pc.status == 0);
    return NsfMongoIndex(interp, conn, namespace, attributes, withDropdups, withUnique);

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
    Tcl_Obj *conn = (Tcl_Obj *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *values = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfMongoInsert(interp, conn, namespace, values);

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
    Tcl_Obj *conn = (Tcl_Obj *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *query = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj *withAtts = (Tcl_Obj *)pc.clientData[3];
    int withLimit = (int )PTR2INT(pc.clientData[4]);
    int withSkip = (int )PTR2INT(pc.clientData[5]);

    assert(pc.status == 0);
    return NsfMongoQuery(interp, conn, namespace, query, withAtts, withLimit, withSkip);

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
    Tcl_Obj *conn = (Tcl_Obj *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *condition = (Tcl_Obj *)pc.clientData[2];

    assert(pc.status == 0);
    return NsfMongoRemove(interp, conn, namespace, condition);

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
    Tcl_Obj *conn = (Tcl_Obj *)pc.clientData[0];
    CONST char *namespace = (CONST char *)pc.clientData[1];
    Tcl_Obj *cond = (Tcl_Obj *)pc.clientData[2];
    Tcl_Obj *values = (Tcl_Obj *)pc.clientData[3];
    int withUpsert = (int )PTR2INT(pc.clientData[4]);
    int withAll = (int )PTR2INT(pc.clientData[5]);

    assert(pc.status == 0);
    return NsfMongoUpdate(interp, conn, namespace, cond, values, withUpsert, withAll);

  }
}

static Nsf_methodDefinition method_definitions[] = {
{"::mongo::close", NsfMongoCloseStub, 1, {
  {"conn", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::connect", NsfMongoConnectStub, 2, {
  {"-replica-set", 0, 1, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-server", 0, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::count", NsfMongoCountStub, 3, {
  {"conn", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"query", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::index", NsfMongoIndexStub, 5, {
  {"conn", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"attributes", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-dropdups", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-unique", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::insert", NsfMongoInsertStub, 3, {
  {"conn", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"values", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::query", NsfMongoQueryStub, 6, {
  {"conn", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"query", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-atts", 0, 1, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-limit", 0, 1, Nsf_ConvertToInteger, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-skip", 0, 1, Nsf_ConvertToInteger, NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::remove", NsfMongoRemoveStub, 3, {
  {"conn", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"condition", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},
{"::mongo::update", NsfMongoUpdateStub, 6, {
  {"conn", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"namespace", NSF_ARG_REQUIRED, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"cond", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"values", NSF_ARG_REQUIRED, 0, Nsf_ConvertToTclobj, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-upsert", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL},
  {"-all", 0, 0, Nsf_ConvertToString, NULL,NULL,NULL,NULL,NULL,NULL,NULL}}
},{NULL}
};

