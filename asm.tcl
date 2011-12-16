package req nx::test
nx::Test parameter count 100000
#nx::Test parameter count 10

proc sum10.tcl {} {
  set sum 0
  for {set i 0} {$i < 100} {incr i} {
    incr sum $i
  }
  return $sum
}
nsf::asm::proc sum10.asm1 {} {
  {obj sum}
  {obj i}
  {obj 0}
  {obj 1}
  {obj 100}
  {obj 0}
  {var obj 0} 
  {var obj 1}              
  {copyScalar int 6 obj 2}  
  {copyScalar int 7 obj 5}  
  {leScalar int 4 int 7}
  {jumpTrue int 7}
  {incrScalar int 6 int 7} 
  {incrScalar int 7 int 3} 
  {jump int 2}
  {setResult int 6}
}

? {sum10.tcl} "4950"
? {sum10.asm1} "4950"

#exit

proc incr1.tcl {x} {
  incr x
}
# currently we have to set the local var of the argument
nsf::asm::proc incr1.asm1 {x} {
  {obj x}
  {obj 1}
  {cmd ::set obj 0 arg 0}
  {cmd ::incr obj 0 obj 1}
}
nsf::asm::proc incr1.asm2 {x} {
  {obj x}
  {obj 1}
  {var obj 0}
  {setScalar int 2 arg 0}
  {incrScalar int 2 int 1}
}
? {incr1.tcl 10} "11"
? {incr1.asm1 10} "11"
? {incr1.asm2 10} "11"

proc incr2.tcl {x} {
  set a $x
  incr a
}
nsf::asm::proc incr2.asm1 {x} {
  {obj a}
  {obj 1}
  {cmd ::set obj 0 arg 0}
  {cmd ::incr obj 0 obj 1}
}
nsf::asm::proc incr2.asm2 {x} {
  {obj a}
  {obj 1}
  {var obj 0}
  {setScalar int 2 arg 0}
  {incrScalar int 2 int 1}
}
? {incr2.tcl 13} "14"
? {incr2.asm1 13} "14"
? {incr2.asm2 13} "14"

proc foo.tcl {x} {
  set a 1
  set b $x
  incr a [incr b]
  return $a
}
nsf::asm::proc foo.asm1 {x} {
  {obj a}
  {obj b}
  {obj 1}
  {cmd ::set obj 0 obj 2}
  {cmd ::set obj 1 arg 0}
  {cmd ::incr obj 1}
  {store code 4 argv 2}
  {cmd ::incr obj 0 result 3}
  {cmd ::set obj 0}
}
nsf::asm::proc foo.asm2 {x} {
  {obj a}
  {obj b}
  {obj 1}
  {var obj 0}
  {var obj 1}
  {var obj 2}
  {setScalar int 3 obj 2}
  {setScalar int 4 arg 0}
  {incrScalar int 4 int 2}
  {setScalarResult int 5}
  {incrScalar int 3 int 5}
  {cmd ::set obj 0}
}
? {foo.tcl 100} "102"
? {foo.asm1 100} "102"
? {foo.asm2 100} "102"
#exit


proc bar.tcl {x} {concat [format %c 64] - [format %c 65] - $x}
nsf::asm::proc bar.asm {x} {
  {obj %c} 
  {obj -} 
  {obj 64} 
  {obj 65} 
  {cmd ::format obj 0 obj 2} 
  {store code 4 argv 1} 
  {cmd ::format obj 0 obj 3} 
  {store code 4 argv 3} 
  {cmd ::concat result 1 obj 1 result 3 obj 1 arg 0} 
}
#puts [bar.asm 123]
? {bar.tcl 123} "@ - A - 123"
? {bar.asm 123} "@ - A - 123"

proc create1.tcl {} {nx::Object create o1}
nsf::asm::proc create1.asm1 {} {
  {obj ::nx::Object}
  {obj create}
  {obj o1}
  {eval obj 0 obj 1 obj 2}
}
nsf::asm::proc create1.asm2 {} {
  {obj create}
  {obj o1}
  {cmd ::nx::Object obj 0 obj 1}
}
nsf::asm::proc create1.asm3 {} {
  {obj nx::Object}
  {obj ::nsf::methods::class::create}
  {obj o1}
  {methodDelegateDispatch obj 0 obj 1 obj 2}
}
nsf::asm::proc create1.asm4 {} {
  {obj ::nx::Object}
  {obj ::nsf::methods::class::create}
  {obj o1}
  {methodDelegateDispatch obj 0 obj 1 obj 2}
}

? {create1.tcl} "::o1"
? {create1.asm1} "::o1"
? {create1.asm2} "::o1"
? {create1.asm3} "::o1"
? {create1.asm4} "::o1"

proc create2.tcl {} {nx::Object create o1;o1 destroy;::nsf::object::exists o1}
nsf::asm::proc create2.asm1 {} {
  {obj create}
  {obj o1}
  {obj destroy}
  {cmd ::nx::Object obj 0 obj 1}
  {eval obj 1 obj 2}
  {cmd ::nsf::object::exists obj 1}
}
nsf::asm::proc create2.asm2 {} {
  {obj o1}
  {obj nx::Object}
  {obj ::nsf::methods::class::create}
  {obj ::nsf::methods::object::destroy}
  {methodDelegateDispatch obj 1 obj 2 obj 0}
  {methodDelegateDispatch obj 0 obj 3}
  {cmd ::nsf::object::exists obj 0}
}
nsf::asm::proc create2.asm3 {} {
  {obj o1}
  {obj ::nx::Object}
  {obj ::nsf::methods::class::create}
  {obj ::nsf::methods::object::destroy}
  {methodDelegateDispatch obj 1 obj 2 obj 0}
  {methodDelegateDispatch obj 0 obj 3}
  {cmd ::nsf::object::exists obj 0}
}
? {create2.tcl} 0
? {create2.asm1} 0
? {create2.asm2} 0
? {create2.asm3} 0

proc check_obj.tcl {} {::nsf::object::exists o1}
nsf::asm::proc check_obj.asm1 {} {
  {obj o1}
  {cmd ::nsf::object::exists obj 0}
}
nsf::asm::proc check_obj.asm2 {} {
  {obj o1}
  {obj ::nsf::object::exists}
  {eval obj 1 obj 0}
}
? {check_obj.tcl} 0
? {check_obj.asm1} 0
? {check_obj.asm2} 0

nx::Object create o {
  set :x 1
}
nsf::method::create o check_obj.tcl {} {::nsf::object::exists o1}
nsf::method::asmcreate o check_obj.asm1 {} {
  {obj o1}
  {cmd ::nsf::object::exists obj 0}
}
nsf::method::asmcreate o check_obj.asm2 {} {
  {obj o1}
  {obj ::nsf::object::exists}
  {eval obj 1 obj 0}
}
? {o check_obj.tcl} 0
? {o check_obj.asm1} 0
? {o check_obj.asm2} 0

# info exists is byte-compiled
nsf::method::create o check_var1.tcl {} {info exists :x}
nsf::method::asmcreate o check_var1.asm1 {} {
  {obj exists}
  {obj :x}
  {cmd ::info obj 0 obj 1}
}
? {o check_var1.tcl} 1
? {o check_var1.asm1} 1

# check for existence via method
nsf::method::create o check_var2.tcl {} {
  : ::nsf::methods::object::exists x
}
nsf::method::asmcreate o check_var2.asm1 {} {
  {obj :}
  {obj ::nsf::methods::object::exists}
  {obj x}
  {eval obj 0 obj 1 obj 2}
}
nsf::method::asmcreate o check_var2.asm2 {} {
  {obj ::o}
  {obj ::nsf::methods::object::exists}
  {obj x}
  {methodDelegateDispatch obj 0 obj 1 obj 2}
}
nsf::method::asmcreate o check_var2.asm3 {} {
  {obj nsf::methods::object::exists}
  {obj x}
  {methodSelfDispatch obj 0 obj 1}
}
nsf::method::asmcreate o check_var2.asm4 {} {
  {obj ::nsf::methods::object::exists}
  {obj x}
  {methodSelfDispatch obj 0 obj 1}
}
? {o check_var2.tcl} 1
? {o check_var2.asm1} 1
? {o check_var2.asm2} 1
? {o check_var2.asm3} 1
? {o check_var2.asm4} 1

#
# self
#
nsf::method::create o self.tcl {} {
  self
}
nsf::method::asmcreate o self.asm1 {} {
  {obj self}
  {eval obj 0}
}
nsf::method::asmcreate o self.asm2 {} {
  {cmd self}
}
nsf::method::asmcreate o self.asm3 {} {
  {self}
}

? {o self.tcl} ::o
? {o self.asm1} ::o
? {o self.asm2} ::o
? {o self.asm3} ::o

