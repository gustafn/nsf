# -*- Tcl -*-
#
# API declarations for the nsf mongo interface
#

# namespaces for types of methods
array set ns {
  cmd  "::mongo"
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
  {-argName "-atts" -required 0 -nrargs 1 -type tclobj}
  {-argName "-limit" -required 0 -nrargs 1 -type int32}
  {-argName "-skip" -required 0 -nrargs 1 -type int32}
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

#
# GridFS
#
cmd gridfs::open NsfMongoGridFSOpen {
  {-argName "conn" -required 1 -type tclobj}
  {-argName "dbname" -required 1}
  {-argName "prefix" -required 1}
}

cmd gridfs::store_file NsfMongoGridFSStoreFile {
  {-argName "gfs" -required 1 -type tclobj}
  {-argName "filename" -required 1}
  {-argName "remotename" -required 1}
  {-argName "contenttype" -required 1}
}

cmd gridfs::remove_file NsfMongoGridFSRemoveFile {
  {-argName "gfs" -required 1 -type tclobj}
  {-argName "filename" -required 1}
}

cmd gridfs::close NsfMongoGridFSClose {
  {-argName "gfs" -required 1 -type tclobj}
}

#
# GridFile
#

cmd gridfile::close NsfMongoGridFileClose {
  {-argName "gridfile" -required 1 -type tclobj}
}

cmd gridfile::get_contentlength NsfMongoGridFileGetContentlength {
  {-argName "gridfile" -required 1 -type tclobj}
}
cmd gridfile::get_contenttype NsfMongoGridFileGetContentType {
  {-argName "gridfile" -required 1 -type tclobj}
}
cmd gridfile::get_metadata NsfMongoGridFileGetMetaData {
  {-argName "gridfile" -required 1 -type tclobj}
}
cmd gridfile::open NsfMongoGridFileOpen {
  {-argName "gfs" -required 1 -type tclobj}
  {-argName "filename" -required 1}
}
cmd gridfile::read NsfMongoGridFileRead {
  {-argName "gridfile" -required 1 -type tclobj}
  {-argName "size" -required 1 -type int32}
}
