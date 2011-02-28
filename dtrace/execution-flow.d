/* -*- D -*-
 * 
 * Execution flow trace without arguments
 *
 * Activate tracing between 
 *    ::nsf::configure dtrace on
 * and
 *    ::nsf::configure dtrace off
 *
 */

nsf*:::configure-probe /!self->tracing && copyinstr(arg0) == "dtrace" / {		       
  self->tracing = (arg1 && copyinstr(arg1) == "on") ? 1 : 0;
}

nsf*:::configure-probe /self->tracing && copyinstr(arg0) == "dtrace" / {
  self->tracing = (arg1 && copyinstr(arg1) == "off") ? 0 : 1;
}

/*
 * Output object, class, method and number of arguments upon method
 * invocation.
 */

nsf*:::method-entry /self->tracing/ {
  printf("%s %s.%s (%d)", 
	 copyinstr(arg0), copyinstr(arg1), copyinstr(arg2), arg3);
}

/*
 * Output object, class, method and return code upon method return.
 */
nsf*:::method-return /self->tracing/ {
  printf("%s %s.%s -> %d", copyinstr(arg0), copyinstr(arg1), copyinstr(arg2), arg3);
}
