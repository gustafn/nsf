Summary: Object Oriented Extension for Tcl
Name: xotcl
Version: 1.3.5
Release: 0
Copyright: open source
Group: Development/Languages
Source:  http://www.xotcl.org/xotcl-1.3.5.tar.gz
URL: http://www.xotcl.org
Packager: Gustaf.Neumann@wu-wien.ac.at
Distribution: RedHat 8.0
Requires: tcl
Prefix: /usr

%description
XOTcl is an object-oriented scripting language based on MIT's OTcl.
This packages provides a pre-packaged tcl-shell (xotclsh) and tk-shell
(xowish) together with the Tcl-extension (libxotcl.so) which can be
loaded to any Tcl-application. Furthermore it includes several
xotcl-based packages for e.g. HTTP client and server, XML, RDF,
persistent object store, mobile code system, etc. For more details
consult http://www.xotcl.org

%prep
%setup -q -n xotcl-1.3.5


%build
./configure --with-tcl=/usr/lib --with-all --prefix=/usr/local/aolserver40r8/ --exec-prefix=/usr/local/aolserver40r8/
#make CFLAGS_DEFAULT='-O3 -mcpu=i686 -Wall -fomit-frame-pointer'
make CFLAGS_DEFAULT='-O3 -Wall -fomit-frame-pointer'

%install
make install

%files
%define _unpackaged_files_terminate_build      0
%undefine       __check_files

%doc doc 
/usr/local/aolserver40r8//lib/xotcl1.3.5
/usr/local/aolserver40r8//bin/xotclsh
/usr/local/aolserver40r8//bin/xowish
/usr/local/aolserver40r8//include/xotclDecls.h
/usr/local/aolserver40r8//include/xotcl.h
/usr/local/aolserver40r8//include/xotclIntDecls.h
/usr/local/aolserver40r8//include/xotclInt.h
