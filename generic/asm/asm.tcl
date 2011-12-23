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
# implementation in assembly, using tcl-objs for 
# "sum", "i" and the constants
nsf::asm::proc sum10.asm1 {} {
  {obj sum}
  {obj i}
  {obj 0}
  {obj 1}
  {obj 100}
  {obj 0}
  {var obj 0} 
  {var obj 1}              
  {duplicateObj slot 6 obj 2}  
  {duplicateObj slot 7 obj 5}  
  {leIntObj slot 4 slot 7}
  {jumpTrue instruction 7}
  {incrObj slot 6 slot 7} 
  {incrObj slot 7 slot 3} 
  {jump instruction 2}
  {setResult slot 6}
}
# implementation in assembly, using tcl-objs for 
# "sum", "i" and the constants
nsf::asm::proc sum10.asm2 {} {
  {obj sum}
  {obj i}
  {integer int 1}
  {integer int 100}
  {integer int 0}
  {integer int 0}
  {setInt slot 4 int 0}
  {setInt slot 5 int 0}
  {leInt slot 3 slot 5}
  {jumpTrue instruction 7}
  {incrInt slot 4 slot 5} 
  {incrInt slot 5 slot 2} 
  {jump instruction 2}
  {setResultInt slot 4}
}

? {sum10.tcl} "4950"
? {sum10.asm1} "4950"
? {sum10.asm2} "4950"

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
  {setObj slot 2 arg 0}
  {incrObj slot 2 slot 1}
  {setResult slot 2}
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
  {setObj slot 2 arg 0}
  {incrObj slot 2 slot 1}
  {setResult slot 2}
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
  {store instruction 4 argv 2}
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
  {setObj slot 3 obj 2}
  {setObj slot 4 arg 0}
  {incrObj slot 4 slot 2}
  {setObjToResult slot 5}
  {incrObj slot 3 slot 5}
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
  {store instruction 4 argv 1} 
  {cmd ::format obj 0 obj 3} 
  {store instruction 4 argv 3} 
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

