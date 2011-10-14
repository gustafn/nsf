/* -*- D -*-
 *
 * Quantize time between method-entry and method-returns for calls on ::nx::Object
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
nsf*:::method-entry /self->tracing && copyinstr(arg1) == "::nx::Object"/ {
  self->start = timestamp;
}

nsf*:::method-return /self->tracing && copyinstr(arg1) == "::nx::Object" && self->start/ {
  @[copyinstr(arg1), copyinstr(arg2)] = quantize(timestamp - self->start);
  self->start = 0;
}
