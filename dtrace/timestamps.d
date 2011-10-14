/* -*- D -*-
 *
 * Measure time between method-entry and method-returns
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
 * Measure time differences
 */
nsf*:::method-entry /self->tracing/ {
  self->start = timestamp;
}

nsf*:::method-return /self->tracing && self->start/ {
  @[copyinstr(arg0), copyinstr(arg1), copyinstr(arg2)] = avg(timestamp - self->start);
  self->start = 0;
}

/*
 * Print aggregate with own format (less wide than the default)
 */
END {
  printa("\n%-35s %-35s %-40s = %@d", @);
}
