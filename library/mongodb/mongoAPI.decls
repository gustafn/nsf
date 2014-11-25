# -*- Tcl -*-
#
# API declarations for the nsf mongo interface
#

# namespaces for types of methods
array set ns {
  cmd  "::mongo"
}
array set ptrConverter {
  mongoc_client_t 1
  mongoc_collection_t 1
  mongoc_cursor_t 1
  mongoc_gridfs_file_t 1
  mongoc_gridfs_t 1
}

cmd json NsfMongoJson {
  {-argName "list" -required 1 -type tclobj}
}

cmd close NsfMongoClose {
  {-argName "conn" -required 1 -type mongoc_client_t -withObj 1}
}
cmd connect NsfMongoConnect {
  {-argName "-uri" -required 0 -nrargs 1}
}
cmd run NsfMongoRunCmd {
  {-argName "-nocomplain" -required 0 -nrargs 0}
  {-argName "conn" -required 1 -type mongoc_client_t}
  {-argName "db" -required 1}
  {-argName "cmd" -required 1 -type tclobj}
}
cmd status NsfMongoStatus {
  {-argName "conn" -required 1 -type mongoc_client_t -withObj 1}
}


#
# collection
#
cmd "collection::close" NsfCollectionClose {
  {-argName "collection" -required 1 -type mongoc_collection_t -withObj 1}
}
cmd "collection::count" NsfMongoCollectionCount {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "query" -required 1 -type tclobj}
}
cmd "collection::delete" NsfMongoCollectionDelete {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "condition" -required 1 -type tclobj}
}
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
cmd "collection::insert" NsfMongoCollectionInsert {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "values" -required 1 -type tclobj}
}
cmd collection::open NsfCollectionOpen {
  {-argName "conn" -required 1 -type mongoc_client_t}
  {-argName "dbname" -required 1}
  {-argName "collectionname" -required 1}
}
cmd "collection::query" NsfMongoCollectionQuery {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "query" -required 1 -type tclobj}
  {-argName "-atts" -required 0 -nrargs 1 -type tclobj}
  {-argName "-limit" -required 0 -type int32}
  {-argName "-skip" -required 0 -type int32}
}
cmd "collection::stats" NsfMongoCollectionStats {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "-options" -required 0 -type tclobj}
}
cmd "collection::update" NsfMongoCollectionUpdate {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "cond" -required 1 -type tclobj}
  {-argName "values" -required 1 -type tclobj}
  {-argName "-upsert" -required 0 -nrargs 0}
  {-argName "-all" -required 0 -nrargs 0}
}

#
# Cursor
#
cmd cursor::aggregate NsfMongoCursorAggregate {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "pipeline" -required 1 -type tclobj}
  {-argName "options" -required 1 -type tclobj}
  {-argName "-tailable" -required 0 -nrargs 0}
  {-argName "-awaitdata" -required 0 -nrargs 0}
}
cmd cursor::find NsfMongoCursorFind {
  {-argName "collection" -required 1 -type mongoc_collection_t}
  {-argName "query" -required 1 -type tclobj}
  {-argName "-atts" -required 0 -nrargs 1 -type tclobj}
  {-argName "-limit" -required 0 -type int32}
  {-argName "-skip" -required 0 -type int32}
  {-argName "-tailable" -required 0 -nrargs 0}
  {-argName "-awaitdata" -required 0 -nrargs 0}
}
cmd cursor::next NsfMongoCursorNext {
  {-argName "cursor" -required 1 -type mongoc_cursor_t}
}
cmd cursor::close NsfMongoCursorClose {
  {-argName "cursor" -required 1 -type mongoc_cursor_t -withObj 1}
}

#
# GridFS
#
cmd gridfs::close NsfMongoGridFSClose {
  {-argName "gfs" -required 1 -type mongoc_gridfs_t -withObj 1}
}

cmd gridfs::open NsfMongoGridFSOpen {
  {-argName "conn" -required 1 -type mongoc_client_t}
  {-argName "dbname" -required 1}
  {-argName "prefix" -required 1}
}


#
# GridFile commands operating on GridFS
#

cmd gridfile::create NsfMongoGridFileCreate {
  {-argName "-source" -required 1 -typeName "gridfilesource" -type "file|string"}
  {-argName "gfs" -required 1 -type mongoc_gridfs_t}
  {-argName "value" -required 1}
  {-argName "name" -required 1}
  {-argName "contenttype" -required 1}
  {-argName "-metadata" -type tclobj}
}

cmd "gridfile::delete" NsfMongoGridFileDelete {
  {-argName "gfs" -required 1 -type mongoc_gridfs_t}
  {-argName "query" -required 1 -type tclobj}
}
cmd "gridfile::open" NsfMongoGridFileOpen {
  {-argName "gfs" -required 1 -type mongoc_gridfs_t}
  {-argName "query" -required 1 -type tclobj}
}


#
# GridFile
#

cmd "gridfile::close" NsfMongoGridFileClose {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t -withObj 1}
}
cmd "gridfile::get_contentlength" NsfMongoGridFileGetContentlength {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
}
cmd "gridfile::get_contenttype" NsfMongoGridFileGetContentType {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
}
cmd "gridfile::get_metadata" NsfMongoGridFileGetMetaData {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
}
cmd "gridfile::read" NsfMongoGridFileRead {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
  {-argName "size" -required 1 -type int32}
}
cmd "gridfile::seek" NsfMongoGridFileSeek {
  {-argName "gridfile" -required 1 -type mongoc_gridfs_file_t}
  {-argName "offset" -required 1 -type int32}
}


#
# Local variables:
#    mode: tcl
#    tcl-indent-level: 2
#    indent-tabs-mode: nil
# End:
