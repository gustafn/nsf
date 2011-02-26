nsf*:::method-entry
/copyinstr(arg1) == "::nx::Object"/
{
  self->start = timestamp;
}

nsf*:::method-return
/copyinstr(arg1) == "::nx::Object" && self->start/
{
  @[copyinstr(arg1), copyinstr(arg2)] = quantize(timestamp - self->start);
  self->start = 0;
}