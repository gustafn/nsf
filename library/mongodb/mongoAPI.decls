# namespaces for types of methods
array set ns {
  cmd              "::mongo"
  objectMethod     "::nsf::methods::object"
  objectInfoMethod "::nsf::methods::object::info"
  classMethod      "::nsf::methods::class"
  classInfoMethod  "::nsf::methods::class::info"
  checkMethod      "::nsf::cmd::ParameterType"
}

cmd close NsfMongoClose {
  {-argName "conn" -required 1 -type tclobj}
}

cmd connect NsfMongoConnect {
  {-argName "-replica-set" -required 0 -nrargs 1}
  {-argName "-server" -required 0 -nrargs 1 -type tclobj}
}

cmd count NsfMongoCount {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "query" -required 1 -type tclobj}
}

cmd index NsfMongoIndex {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "attributes" -required 1 -type tclobj}
  {-argName "-dropdups" -required 0 -nrargs 0}
  {-argName "-unique" -required 0 -nrargs 0}
}

cmd insert NsfMongoInsert {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "values" -required 1 -type tclobj}
}

cmd query NsfMongoQuery {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "query" -required 1 -type tclobj}
  {-argName "-limit" -required 0 -nrargs 1 -type int}
  {-argName "-skip" -required 0 -nrargs 1 -type int}
}

cmd remove NsfMongoRemove {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "condition" -required 1 -type tclobj}
}

cmd update NsfMongoUpdate {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "namespace" -required 1}
  {-argName "cond" -required 1 -type tclobj}
  {-argName "values" -required 1 -type tclobj}
  {-argName "-upsert" -required 0 -nrargs 0}
  {-argName "-all" -required 0 -nrargs 0}
}

