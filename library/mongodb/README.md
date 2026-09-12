# Interface between MongoDB and the Next Scripting Framework #

This is a client interface to MongoDB based on NSF (Next Scripting
Framework).

This implementation provides a low-level interface based on tagged
elements to force / preserve the datatypes of MongoDB when converting
into and from Tcl.

This code also serves as an example of how to use the source-code generator
of NSF to generate a C-level programming interface.

Gustaf Neumann    March 2011
Stefan Sobernig   May 2019, September 2026


## Ingredients: ##

  https://github.com/mongodb/mongo
  https://github.com/mongodb/mongo-c-driver

The current version of the NSF mongo binding is 2.5.0 and was tested
with:

- Tcl 8.6.18 and 9.0.4
- MongoDB 8.0.28 (released in July 2026)
- mongo-c-driver 1.30.8 (released April 2026)
- libbson 1.30.8 (released April 2026)

Important: The tested MongoDB C client library is the latest
maintenance release of the 1.* mainline which has reached its
end-of-life. The 2.* mainline has not been integrated or
tested. However, the tested version is compatible with MongoDB
versions 4 through 8.

Follow these steps to get MongoDB up and running
and to compile the MongoDB driver for NSF/NX.


## Obtain MongoDB and Mongo-C-Driver: ##

- Compile or obtain MongoDB (the database).

- Compile or obtain the mongo-c-driver (client interface)

````
      cd /usr/local/src
      wget https://github.com/mongodb/mongo-c-driver/releases/download/1.30.8/mongo-c-driver-1.30.8.tar.gz
      tar zxvf mongo-c-driver-1.30.8.tar.gz
      rm -rf mongo-c-driver
      ln -sf mongo-c-driver-1.30.8 mongo-c-driver
      cd mongo-c-driver
      cmake .
      make
      sudo make install
````

  If you experience errors during autogen on Debian, you might have to
      apt-get install libtool

  If configure complains about not finding bson, you might have to do
      export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

## Compiling the MongoDB NSF Binding: ##

Assume the following installation directories

  - Tcl:                            `/your/path/to/your/tclConfig.sh`
  - mongo-c-driver (sources):       `/your/path/to/mongo-c-driver/`
  - mongo-c-driver (installation):  `/usr/local/lib`
  - mongo-c-driver (headers):       `/usr/local/include`
  
Configure the MongoDB NSF interface via the following command in the
directory `/your/path/to/nsf-sources/library/mongodb/`.

You will have to adjust the paths. First, run configure:

````
   ./configure --enable-threads \
               --with-tcl=/your/path/to/your/tclConfig.sh \
			   --with-nsf=/your/path/to/nsf-sources/\
			   --with-mongoc=/usr/local/include/libmongoc-1.0,/usr/local/lib/\
			   --with-bson=/usr/local/include/libbson-1.0/,/usr/local/lib/
````

Then, execute

````
  make install
````

Important: Under macOS, the Tcl build harness currently does not
support rpath. The MongoDB C driver libraries, however, are, by
default, rpath-enabled:

Option 1: Inject the linker flags (this will create the `LC_RPATH` field in the nsfmongo library)

````
  make  LDFLAGS_DEFAULT="${LDFLAGS_DEFAULT} -Wl,-rpath,/usr/local/lib"
````

Option 2: Set the environment variable `DYLD_FALLBACK_LIBRARY_PATH`

````
  DYLD_FALLBACK_LIBRARY_PATH=/usr/local/lib ./tclsh
````

In order to run the NSF sample script, perform the following steps

* First, start the MongoDB server (e.g. `mongod`)
* Go to your `/your/path/to/nsf-sources/`
* Run:

````
    ./nxsh library/mongodb/tests/nsf-mongo.test
````

The script tests the low-level interface (nsf::mongo) and the
high-level one (nx::mongo),  each exercising a few insert, query
and delete statements.

After running this script, you should check the content using the
MongoDB shell:

````
	% mongosh
   Current Mongosh Log ID:	6aa51a1d8e1302fce1eb7a3f
   Connecting to:		mongodb://127.0.0.1:27017/?directConnection=true&serverSelectionTimeoutMS=2000&appName=mongosh+2.9.2
   Using MongoDB:		8.0.28
   Using Mongosh:		2.9.2

    > use tutorial
    switched to db tutorial
    >  db.persons.find();
   { "_id" : ObjectId("530c6e4649686ad16e261f81"), "name" : "Gustaf", "projects" : "nsf", "age" : 53 }
   { "_id" : ObjectId("530c6e4649686ad16e261f82"), "name" : "Stefan", "projects" : "nsf" }
   { "_id" : ObjectId("530c6e4649686ad16e261f83"), "name" : "Victor", "a" : [  "x",  "y" ], "age" : 31 }
   { "_id" : ObjectId("530c6e4649686ad16e261f84"), "name" : "Joe", "projects" : "abc", "age" : 23, "classes" : [  DBRef("courses", ObjectId("100000000000000000000000")) ] }
   { "_id" : ObjectId("530c6e4649686ad16e261f85"), "name" : "Franz", "info" : { "x" : 203, "y" : 102 }, "age" : 29, "projects" : "gtat" }
   { "_id" : ObjectId("530c6e4649686ad16e261f86"), "name" : "Selim", "ts" : Timestamp(1302945037, 1), "d" : ISODate("2011-04-16T09:53:39.279Z") }
   > quit()
````

## Testing the object-oriented mapping between NX and MongoDB: ##

Test the basic mapping and the OO query methods:

````
    ./nxsh library/mongodb/tests/nx-mongo.test
````

Show the classical Business Informer example in NX:

````
    ./nxsh library/mongodb/tests/nx-bi.test
````

Further test scripts for reference handling, serialization and
MongoDB GridFS.

````
    ./nxsh library/mongodb/tests/nx-reference-one.test
    ./nxsh library/mongodb/tests/nx-reference-many.test
    ./nxsh library/mongodb/tests/nx-rep.test
    ./nxsh library/mongodb/tests/nx-serialize.test
    ./nxsh library/mongodb/tests/nsf-gridfs.test
````
