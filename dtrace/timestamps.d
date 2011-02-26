nsf*:::method-entry
{
  self->start = timestamp;
}

nsf*:::method-return
/self->start/
{
  @[copyinstr(arg0), copyinstr(arg1), copyinstr(arg2)] = avg(timestamp - self->start);
  self->start = 0;
}