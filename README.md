# Next Scripting Framework (NSF) & Next Scripting Language (NX) #

NX is a highly flexible, [Tcl](https://www.tcl-lang.org/)-based,
object-oriented scripting language. It is a descendant of
[XOTcl](https://xotcl.org/) and was designed based on 10 years of
experience with XOTcl in projects containing several hundred thousand
lines of code. While XOTcl was the first language designed to provide
support for design patterns and to offer a highly dynamic programming
environment, the Next Scripting Framework (NSF) and NX add these
features to support language-oriented programming.

For details and more background, consult:

  https://next-scripting.org/
  
For reporting issues and requests for enhancements, please submit a
ticket to:

  https://sourceforge.net/p/next-scripting/tickets/

## Getting started (source distribution)  ##

1. Install Tcl in version 8.5 or higher:

  https://www.tcl-lang.org/software/tcltk/
  
  Important: Make sure to install Tcl's internal headers!

2. Build (as an example for `autoconf` builds on Unix and similar):

````
% ./configure
...
%  make
...
%  sudo make install
...
````

3. Run:

````
% nxsh
% package req nsf
2.5.0
% package req nx
2.5.0
% package req XOTcl
2.5.0
````

4. Work through the tutorial: 

  https://next-scripting.org/xowiki/docs/nx/tutorial/index1
  
## Other installation options ##

Visit:

  https://next-scripting.org/xowiki/getstart
