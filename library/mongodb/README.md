
# Interface between mongoDB and the Next Scripting Framework #

This is an interface between MongoDB based on NSF (Next Scripting
Framework)

This implementation provides a low level interface based on tagged elements
to force / preserve the datatypes of MongoDB when converting into Tcl.

This code serves as well as an example how to use the source code generator
of NSF.  The example shows how to use the source code generator from NSF to
generate a C interface.

-gustaf neumann    March 27, 2011


## Ingredients: ##

  https://github.com/mongodb/mongo  
  https://github.com/mongodb/mongo-c-driver  

The current version of the NSF mongo binding is 2.2 and was
tested with
- Tcl 8.5, 8.6 and 8.7
- MongoDB v3.6.5 (released May 29, 2018)
- mongodb-c-driver 1.11.0 (released June 23, 2018)
- libbson 1.11.0 (released June 23, 2018)

Follow the following steps to get MongoDB up and running
and to compile the MongoDB driver for NX.


## Obtain Mongodb and Mongo-C-Driver: ##

- Compile or obtain mongodb (the database).

- Compile or obtain the mongo-c-driver (client interface)

````
      cd /usr/local/src
      wget https://github.com/mongodb/mongo-c-driver/releases/download/1.11.0/mongo-c-driver-1.11.0.tar.gz
      tar zxvf  mongo-c-driver-1.11.0.tar.gz
      rm -rf mongo-c-driver
      ln -sf mongo-c-driver-1.11.0 mongo-c-driver
      cd mongo-c-driver
      cmake .
      make
      sudo make install
````

  Alternatively, one can get the newest version from git

````
      cd /usr/local/src
      git clone https://github.com/mongodb/mongo-c-driver
      cd mongo-c-driver
      cmake .
      make
      sudo make install
````

  If you experience errors during autogen on Debian, you might have to
      apt-get install libtool

  If configure complains about not finding bson, you might have to do
      export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

  Note: Version 1.5.1 of the c-driver leads on macOS to a crash on
  exit, when configured SASL support (from mac ports) and the flag
  "--disable-automatic-init-and-cleanup" is missing.


## Compiling the Mongo NSF Binding: ##

Assume the following installation directories

  - Tcl:             /usr/local/ns/lib/
  - mongo-c-driver: /usr/local/src/mongo-c-driver/

configure the mongodb NSF interface via the following
command in the directory nsf*/library/mongodb/
You will probably have to adjust the paths.

````
   ./configure --with-tcl=/usr/local/ns/lib/ --prefix=/usr/local/ns --with-nsf=../../ \
               --with-mongoc=/usr/local/include/libmongoc-1.0/,/usr/local/lib/ \
               --with-bson=/usr/local/include/libbson-1.0,/usr/local/lib/ \
	       --enable-threads --enable-symbols
````

In order to run the sample script,
  * first start the mongodb (e.g. mongod)

  * go to your NSF source directory

  * make sure, the c-driver libraries are on the library path
    (assuming the c-driver was installed in /usr/local/lib)

````
    export DYLD_LIBRARY_PATH=/usr/local/lib:`pwd`
````

  * run

````
    ./nxsh library/mongodb/tests/nsf-mongo.test
````

    The script is using the low level interface (nsf::mongo) and has a
    few insert, query and delete statements, some of these are
    commented out.

 * run

````
    ./nxsh library/mongodb/example-nx-mongo.tcl
````

    This example script is using the higher-level object-oriented
    interface for NX (nx::mongo).

    After running this script, you should could
    check the content in MongoDB:

````
    % mongo
    MongoDB shell version: v3.4.0
    connecting to: test
    > use tutorial
    switched to db tutorial
    >  db.persons.find();
   { "_id" : ObjectId("530c6e4649686ad16e261f81"), "name" : "Gustaf", "projects" : "nsf", "age" : 53 }
   { "_id" : ObjectId("530c6e4649686ad16e261f82"), "name" : "Stefan", "projects" : "nsf" }
   { "_id" : ObjectId("530c6e4649686ad16e261f83"), "name" : "Victor", "a" : [  "x",  "y" ], "age" : 31 }
   { "_id" : ObjectId("530c6e4649686ad16e261f84"), "name" : "Joe", "projects" : "abc", "age" : 23, "classes" : [  DBRef("courses", ObjectId("100000000000000000000000")) ] }
   { "_id" : ObjectId("530c6e4649686ad16e261f85"), "name" : "Franz", "info" : { "x" : 203, "y" : 102 }, "age" : 29, "projects" : "gtat" }
   { "_id" : ObjectId("530c6e4649686ad16e261f86"), "name" : "Selim", "ts" : Timestamp(1302945037, 1), "d" : ISODate("2011-04-16T09:53:39.279Z") }
   >
````

## Further sample-scripts: ##

````
    ./nxsh library/mongodb/tests/nx-bi.test
    ./nxsh library/mongodb/tests/nx-reference-one.test
    ./nxsh library/mongodb/tests/nx-reference-many.test
    ./nxsh library/mongodb/tests/nx-rep.test
    ./nxsh library/mongodb/tests/nx-serialize.test
    ./nxsh library/mongodb/tests/nsf-gridfs.test
````

