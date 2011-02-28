/* 
 * Measure time btween method-entry and method-returns
 *
 * Display execution flow between 
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

nsf*:::method-entry /self->tracing/ {
  self->start = timestamp;
}

nsf*:::method-return /self->tracing && self->start/ {
  @[copyinstr(arg0), copyinstr(arg1), copyinstr(arg2)] = avg(timestamp - self->start);
  self->start = 0;
}

END {
  printa("\n%-35s %-35s %-40s = %@d", @);
}
