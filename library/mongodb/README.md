

# Interface between MongoDB and the Next Scripting Framework #

This is an interface between MongoDB based on NSF (Next Scripting
Framework)

This implementation provides a low-level interface based on tagged elements
to force / preserve the datatypes of MongoDB when converting into Tcl.

This code serves as well as an example how to use the source code generator
of NSF.  The example shows how to use the source code generator from NSF to
generate a C interface.

-gustaf neumann    March 27, 2011
-stefan sobernig     May 6,      2019


## Ingredients: ##

  https://github.com/mongodb/mongo
  https://github.com/mongodb/mongo-c-driver

The current version of the NSF mongo binding is 2.2 and was
tested with
- Tcl 8.5, 8.6 and 8.7
- MongoDB 4.0.9 (released April 16, 2019)
- mongodb-c-driver 1.14.0 (released February 22, 2019)
- libbson 1.14.0 (released February 22, 2019)

Follow the following steps to get MongoDB up and running
and to compile the MongoDB driver for NX.


## Obtain MongoDB and Mongo-C-Driver: ##

- Compile or obtain MongoDB (the database).

- Compile or obtain the mongo-c-driver (client interface)

````
      cd /usr/local/src
      wget https://github.com/mongodb/mongo-c-driver/releases/download/1.12.0/mongo-c-driver-1.12.0.tar.gz
      tar zxvf  mongo-c-driver-1.12.0.tar.gz
      rm -rf mongo-c-driver
      ln -sf mongo-c-driver-1.12.0 mongo-c-driver
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


## Compiling the MongoDB NSF Binding: ##

Assume the following installation directories

  - Tcl:             /usr/local/ns/lib/
  - mongo-c-driver: /usr/local/src/mongo-c-driver/

Configure the MongoDB NSF interface via the following
command in the directory nsf*/library/mongodb/

You will probably have to adjust the paths.

````
   ./configure --with-tcl=/usr/local/ns/lib/ --prefix=/usr/local/ns --with-nsf=../../ \
               --with-mongoc=/usr/local/include/libmongoc-1.0/,/usr/local/lib/ \
               --with-bson=/usr/local/include/libbson-1.0,/usr/local/lib/ \
	       --enable-threads --enable-symbols
````

In order to run the NSF sample script, perform the following steps

	* first start the mongodb (e.g. mongod)
	* go to your NSF source directory
	* make sure, the c-driver libraries are on the library path
      (assuming the c-driver was installed in /usr/local/lib)

Linux:

````
    export LD_LIBRARY_PATH=/usr/local/lib:`pwd`
````


macOS:

````
    export DYLD_LIBRARY_PATH=/usr/local/lib:`pwd`
````

  * run

````
    ./nxsh library/mongodb/tests/nsf-mongo.test
````

    The script tests the low-level interface (nsf::mongo) and the
    high-level one (nx::mongo), with each exercising a
    few insert, query and delete statements.

    After running this script, you should check the content using the
    MongoDB shell:

````
    % mongo
    MongoDB shell version: v4.0.9
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
   > quit()
````

## Testing the object oriented mapping between NX and MongoDB: ##

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
