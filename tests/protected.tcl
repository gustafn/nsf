package require nx
package require nx::test
namespace import ::nx::*

Test parameter count 1

Class create C {
  :alias SET ::set
  :method foo {} {return [current method]}
  :method bar {} {return [current method]}
  :method bar-foo {} {
    c1 foo
  }
  :method bar-SET {} {
    c1 SET x 1
  }
}

C create c1
C create c2

? {c1 SET x 1} {1}
? {c1 foo} {foo}
? {c1 bar-SET} {1}
? {c1 bar-foo} {foo}

::nsf::methodproperty C SET protected true
? {catch {c1 SET x 1} errorMsg; set errorMsg} {::c1: unable to dispatch method 'SET'}
? {::nsf::dispatch c1 SET x 2} {2} "dispatch of protected methods works"
? {c1 foo} {foo}
? {c1 bar} {bar}
? {c1 bar-SET} {1}
? {c1 bar-foo} {foo}
? {catch {c2 bar-SET} errorMsg; set errorMsg} {::c1: unable to dispatch method 'SET'}
? {c2 bar-foo} {foo}

::nsf::methodproperty C foo protected true
? {catch {c1 SET x 1} errorMsg; set errorMsg} {::c1: unable to dispatch method 'SET'}
? {::nsf::dispatch c1 SET x 2} {2} "dispatch of protected methods works"
? {c1 bar} {bar} "other method work"
? {catch {c1 foo} errorMsg; set errorMsg} {::c1: unable to dispatch method 'foo'}
? {c1 bar-SET} {1} "internal call of protected C implementend method"
? {c1 bar-foo} {foo} "internal call of protected Tcl implemented method"
? {catch {c2 bar-SET} errorMsg; set errorMsg} {::c1: unable to dispatch method 'SET'}
? {catch {c2 bar-foo} errorMsg; set errorMsg} {::c1: unable to dispatch method 'foo'}

# unset protected
? {::nsf::methodproperty C SET protected} 1
::nsf::methodproperty C SET protected false
? {::nsf::methodproperty C SET protected} 0
? {::nsf::methodproperty C foo protected} 1
::nsf::methodproperty C foo protected false
? {::nsf::methodproperty C foo protected} 0

? {c1 SET x 3} 3
? {::nsf::dispatch c1 SET x 2} {2} 
? {c1 foo} {foo}
? {c1 bar} {bar}
? {c1 bar-SET} {1}
? {c1 bar-foo} {foo}
? {c2 bar-SET} 1
? {c2 bar-foo} {foo}

# define a protected method
C protected method foo {} {return [current method]}
? {::nsf::methodproperty C SET protected} 0
? {c1 SET x 3} 3
? {::nsf::dispatch c1 SET x 4} {4} 
? {catch {c1 foo} errorMsg; set errorMsg} {::c1: unable to dispatch method 'foo'}
? {c1 bar} {bar}
? {c1 bar-SET} {1}
? {c1 bar-foo} foo
? {c2 bar-SET} 1
? {catch {c2 bar-foo} errorMsg; set errorMsg} {::c1: unable to dispatch method 'foo'}

? {::nsf::methodproperty C SET redefine-protected true} 1
? {catch {C method SET {a b c} {...}}  errorMsg; set errorMsg} \
    {Method 'SET' of ::C can not be overwritten. Derive e.g. a sub-class!}
? {::nsf::methodproperty C foo redefine-protected true} 1
? {catch {C method foo {a b c} {...}}  errorMsg; set errorMsg} \
    {Method 'foo' of ::C can not be overwritten. Derive e.g. a sub-class!}
# check a predefined protection
? {catch {::nx::Class method dealloc {a b c} {...}}  errorMsg; set errorMsg} \
    {Method 'dealloc' of ::nx::Class can not be overwritten. Derive e.g. a sub-class!}
# try to redefined via alias
? {catch {::nsf::alias Class dealloc ::set}  errorMsg; set errorMsg} \
    {Method 'dealloc' of ::nx::Class can not be overwritten. Derive e.g. a sub-class!}
# try to redefine via forward
? {catch {C forward SET ::set}  errorMsg; set errorMsg} \
    {Method 'SET' of ::C can not be overwritten. Derive e.g. a sub-class!}
# try to redefine via setter
? {catch {C setter SET}  errorMsg; set errorMsg} \
    {Method 'SET' of ::C can not be overwritten. Derive e.g. a sub-class!}

# overwrite-protect object specific method
Object create o
o method foo {} {return 13}
::nsf::methodproperty o foo redefine-protected true
? {catch {o method foo {} {return 14}} errorMsg; set errorMsg} \
    {Method 'foo' of ::o can not be overwritten. Derive e.g. a sub-class!}

