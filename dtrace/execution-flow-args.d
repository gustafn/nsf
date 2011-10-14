/* -*- D -*-
 *
 * Execution flow trace with arguments
 *
 * Activate tracing between 
 *    ::nsf::configure dtrace on
 * and
 *    ::nsf::configure dtrace off
 *
 * Since this D script accesses the C data structures it is sensitive
 * to the representation sizes of the data structures (e.g. pointers).
 * Make sure to call the script with the appropriate architecture flag
 * on Mac OS X; on SunOS, there is apparently a -32 or -64 flag.
 *
 * Example:
 *
 *   sudo dtrace -arch x86_64 -x bufsize=20m -F -s dtrace/execution-flow-args.d \
 *	-c "./nxsh dtrace/sample.tcl"
 * 
 * -gustaf neumann
 */

enum {maxstrlen = 50};

/*
 * Needed data structures to access the content of Tcl_Objs.
 */
typedef struct Tcl_Obj Tcl_Obj;

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

/*
 * Handling "nsf::configure dtrace on|off".
 */
nsf*:::configure-probe /!self->tracing && copyinstr(arg0) == "dtrace" / {		       
  self->tracing = (arg1 && copyinstr(arg1) == "on") ? 1 : 0;
}

nsf*:::configure-probe /self->tracing && copyinstr(arg0) == "dtrace" / {
  self->tracing = (arg1 && copyinstr(arg1) == "off") ? 0 : 1;
}

/*
 * Output object, class, method, number of arguments and first two
 * arguments upon method invocation.
 */
nsf*:::method-entry /self->tracing/ {
  
  this->objv = arg3 ? ((Tcl_Obj**)copyin((user_addr_t)((Tcl_Obj**)arg4),
					 sizeof(Tcl_Obj*) * arg3)) : NULL;
  
  this->i = 0;
  this->o = arg3 > this->i && *(this->objv + this->i) ?
    (Tcl_Obj*)copyin((user_addr_t)*(this->objv + this->i), sizeof(Tcl_Obj)) : NULL;
  this->s0 = this->o ? (this->o->bytes ? copyinstr((user_addr_t)this->o->bytes, maxstrlen) :
			lltostr(this->o->internalRep.longValue)) : "";
  
  this->i = 1;
  this->o = arg3 > this->i && *(this->objv + this->i) ?
    (Tcl_Obj*)copyin((user_addr_t)*(this->objv + this->i), sizeof(Tcl_Obj)) : NULL;
  this->s1 = this->o ? (this->o->bytes ? copyinstr((user_addr_t)this->o->bytes, maxstrlen) :
			lltostr(this->o->internalRep.longValue)) : "";
  
  printf("%s %s.%s (%d) %s %s", 
	 copyinstr(arg0), copyinstr(arg1), copyinstr(arg2), arg3, 
	 this->s0, this->s1);
}

/*
 * Output object, class, method and return code upon method return.
 */
nsf*:::method-return /self->tracing/ {
  printf("%s %s.%s -> %d", copyinstr(arg0), copyinstr(arg1), copyinstr(arg2), arg3);
}
