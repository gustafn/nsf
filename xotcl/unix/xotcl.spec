Summary: Object Oriented Extension for Tcl
Name: xotcl
Version: 1.5.4
Release: 0
License: bsd style open source
Group: Development/Languages
Source:  http://www.xotcl.org/xotcl-1.5.4.tar.gz
URL: http://www.xotcl.org
Packager: Gustaf.Neumann@wu-wien.ac.at
Distribution: Fedorea Core 3
Requires: tcl
Prefix: /usr
BuildRoot: %{_tmppath}/%{name}-root
BuildRequires: autoconf

%description
XOTcl is an object-oriented scripting language based on MIT's OTcl.
This packages provides a pre-packaged tcl-shell (xotclsh) and tk-shell
(xowish) together with the Tcl-extension (libxotcl.so) which can be
loaded to any Tcl-application. Furthermore it includes several
xotcl-based packages for e.g. HTTP client and server, XML, RDF,
persistent object store, mobile code system, etc. For more details
consult http://www.xotcl.org

%prep
%setup -q -n xotcl-1.5.4


%build
autoconf
./configure --with-tcl=/usr/lib --with-all --prefix=/usr/local --exec-prefix=/usr/local
#make CFLAGS_DEFAULT='-O3 -mcpu=i686 -Wall -fomit-frame-pointer'
make CFLAGS_DEFAULT='-O3 -Wall -fomit-frame-pointer'

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
rm -f $RPM_BUILD_ROOT/usr/lib/libxotcl1.5.4.so
ln -s xotcl1.5.4/libxotcl1.5.4.so $RPM_BUILD_ROOT/usr/lib

%files
%defattr(-, root, root)
%doc doc
%_bindir/xotclsh
%_bindir/xowish
%_prefix/lib/xotcl1.5.4
%_prefix/lib/libxotcl1.5.4.so
%_prefix/lib/xotclexpat*
%_prefix/lib/xotclgdbm*
%_prefix/lib/xotclsdbm*
%_prefix/lib/xotclConfig.sh
%_includedir/xotcl.h
%_includedir/xotclInt.h
%_includedir/xotclDecls.h
%_includedir/xotclIntDecls.h
