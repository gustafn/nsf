# Assumes Tcl 8.6 (couroutine support)
if {[catch {package req Tcl 8.6}]} return

#
# == Rosetta example: Tokenize a string with escaping
#
#
# Write a class which allows for splitting a string at each non-escaped
# occurrence of a separator character.
# 
# See https://rosettacode.org/wiki/Tokenize_a_string_with_escaping
#

package req nx
package req nx::test

nx::Class create Tokenizer {
    :property s:required
    :method init {} {
	:require namespace
        set coro [coroutine [current]::nextCoro [current] iter ${:s}]
        :public object forward next $coro
    }
    :public method iter {s} {
        yield [info coroutine]
        for {set i 0} {$i < [string length $s]} {incr i} {
            yield [string index $s $i]
        }
        return -code break
    }
    :public object method tokenize {{-sep |} {-escape ^} s} {
	set t [[current] new -s $s]
	set part ""
	set parts [list]
	while {1} {
	    set c [$t next]
	    if {$c eq $escape} {
		append part [$t next]
	    } elseif {$c eq $sep} {
		lappend parts $part
		set part ""
	    } else {
		append part $c
	    }
	}
	lappend parts $part
	return $parts	
    }
}

# Run some tests incl. the escape character:

? {Tokenizer tokenize -sep | -escape ^ ^|} {|}
? {Tokenizer tokenize -sep | -escape ^ ^|^|} {||}
? {Tokenizer tokenize -sep | -escape ^ ^^^|} {^|}
? {Tokenizer tokenize -sep | -escape ^ |} {{} {}}

# Test for the output required by the Rosetta example:
? {Tokenizer tokenize -sep | -escape ^ one^|uno||three^^^^|four^^^|^cuatro|} {one|uno {} three^^ four^|cuatro {}}
