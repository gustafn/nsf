/*
 * nsfDTrace.d --
 *
 *	Next Scripting Framework DTrace provider.
 *
 * Copyright (c) 2011-2014 Gustaf Neumann
 * Copyright (c) 2018 Stefan Sobernig
 *
 * Vienna University of Economics and Business
 * Institute of Information Systems and New Media
 * A-1020, Welthandelsplatz 1
 * Vienna, Austria
 *
 * This work is licensed under the MIT License http://www.opensource.org/licenses/MIT
 *
 * Copyright:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

typedef struct Tcl_Obj Tcl_Obj;

/*
 * Next Scripting Framework (NSF) DTrace probes
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
     *	nsf*:::method-return probe
     *	    triggered immediately after proc bytecode execution
     *		arg0: object name			(string)
     *		arg1: class/object name			(string)
     *		arg2: method name			(string)
     *		arg3: return code			(int)
     */
    probe method__return(char *object, char *class, char* name, int code);

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
     *	    triggered whean an NSF object is freed
     *		arg0: object 			(string)
     *		arg1: class 			(string)
     */
    probe object__free(char *object, char *class);

    /***************************** NSF configure probe ******************************/
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
