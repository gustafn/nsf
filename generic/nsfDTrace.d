/*
 * nsfDTrace.d --
 *
 *	Next Scripting Framework DTrace provider.
 *
 * Copyright (c) 2011-2014 Gustaf Neumann
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

typedef struct Tcl_Obj Tcl_Obj;

/*
 * Next Scripting DTrace probes
 * 
 * Modeled in alignment with the Tcl DTrace probes
 */

provider nsf {
    /***************************** proc probes *****************************/
    /*
     *	nsf*:::method-entry probe
     *	    triggered immediately before method bytecode execution
     *		arg0: object name			(string)
     *		arg1: class/object name			(string)
     *		arg2: method name			(string)
     *		arg3: number of arguments		(int)
     *		arg4: array of proc argument objects	(Tcl_Obj**)
     */
    probe method__entry(char* object, char *class, char* method, int objc, Tcl_Obj **objv);
    /*
     *	nsf*:::proc-return probe
     *	    triggered immediately after proc bytecode execution
     *		arg0: object name			(string)
     *		arg1: class/object name			(string)
     *		arg2: method name			(string)
     *		arg3: return code			(int)
     */
    probe method__return(char *object, char *class, char* name, int code);
    /*
     *	tcl*:::proc-result probe
     *	    triggered after proc-return probe and result processing
     *		arg0: proc name				(string)
     *		arg1: return code			(int)
     *		arg2: proc result			(string)
     *		arg3: proc result object		(Tcl_Obj*)
     */

    /***************************** Object probes ******************************/
    /*
     *	nsf*:::object-alloc probe
     *	    triggered when an NSF object is allocated
     *		arg0: object 			(string)
     *		arg1: class 			(string)
     */
    probe object__alloc(char *object, char *class);
   /*
     *	nsf*:::object-free probe
     *	    triggered whean an NSF object is freeed
     *		arg0: object 			(string)
     *		arg1: class 			(string)
     */
    probe object__free(char *object, char *class);

    /***************************** nsf configure probe ******************************/
    /*
     *	nsf*:::configure-probe probe
     *	    triggered when the ::nsf::configure is called
     *		arg0-arg1: command arguments		(strings)
     */
    probe configure__probe(char *arg0, char *arg1);

};

/*
 * Tcl types and constants for use in DTrace scripts
 */

typedef struct Tcl_ObjType {
    char *name;
    void *freeIntRepProc;
    void *dupIntRepProc;
    void *updateStringProc;
    void *setFromAnyProc;
} Tcl_ObjType;

struct Tcl_Obj {
    int refCount;
    char *bytes;
    int length;
    Tcl_ObjType *typePtr;
    union {
	long longValue;
	double doubleValue;
	void *otherValuePtr;
	int64_t wideValue;
	struct {
	    void *ptr1;
	    void *ptr2;
	} twoPtrValue;
	struct {
	    void *ptr;
	    unsigned long value;
	} ptrAndLongRep;
    } internalRep;
};

enum return_codes {
    TCL_OK = 0,
    TCL_ERROR,
    TCL_RETURN,
    TCL_BREAK,
    TCL_CONTINUE
};

#pragma D attributes Evolving/Evolving/Common provider nsf provider
#pragma D attributes Private/Private/Common provider nsf module
#pragma D attributes Private/Private/Common provider nsf function
#pragma D attributes Evolving/Evolving/Common provider nsf name
#pragma D attributes Evolving/Evolving/Common provider nsf args

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
