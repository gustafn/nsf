/*
 * nsfDTrace.d --
 *
 *	Next Scripting Framework DTrace provider.
 *
 * Copyright (c) 2011 Gustaf Neumann <neumann@wu-wien.ac.at>
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
     *		arg0: class/object name			(string)
     *		arg1: method name			(string)
     *		arg2: number of arguments		(int)
     *		arg3: array of proc argument objects	(Tcl_Obj**)
     */
    probe method__entry(char *class, char* method, int objc, Tcl_Obj **objv);
    /*
     *	nsf*:::proc-return probe
     *	    triggered immediately after proc bytecode execution
     *		arg0: class/object name			(string)
     *		arg1: method name			(string)
     *		arg2: return code			(int)
     */
    probe method__return(char *class, char* name, int code);
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
     *	nsf*:::object-create-start probe
     *	    triggered when an NSF object creation starts
     *		arg0: class 			(string)
     *		arg1: object 			(string)
     */
    probe object__create_start(char *class, char *object);
    /*
     *	nsf*:::object-create-end probe
     *	    triggered when an NSF object creation ends
     *		arg0: class 			(string)
     *		arg1: object 			(string)
     */
    probe object__create_end(char *class, char *object);
   /*
     *	nsf*:::object-destroy probe
     *	    triggered whean an NSF object is destroyed
     *		arg0: class 			(string)
     *		arg1: object 			(string)
     */
    probe object__destroy(char *class, char *object);

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
