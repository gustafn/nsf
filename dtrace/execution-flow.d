nsf*:::method-entry
{
   printf("%s %s.%s\n", copyinstr(arg0), copyinstr(arg1), copyinstr(arg2));
}

nsf*:::method-return
{
   printf("%s %s.%s\n", copyinstr(arg0), copyinstr(arg1), copyinstr(arg2));
}