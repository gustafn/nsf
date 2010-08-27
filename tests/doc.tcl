package require nx
package require nx::test
package require nx::doc

namespace import -force ::nx::*
namespace import -force ::nx::doc::*


Test parameter count 1

#
# some helper
#

proc lcompare {a b} {
  foreach x $a y $b {
    if {$a ne $b} {
      return -1; break;
    }
  }
  return 1
}

# Class create ::nx::doc::CommentState::Log {
#   :method on_enter {line} {
#     puts -nonewline stderr "ENTER -> [namespace tail [:info class]]#[namespace tail [self]]"
#     next
#   }
#   :method on_exit {line} { 
#     next
#     puts -nonewline stderr "EXIT -> [namespace tail [:info class]]#[namespace tail [self]]"
#   }
# }

# Class create ::nx::doc::CommentLine::Log {
#   :method on_enter {line} {
#     puts -nonewline stderr "\t"; next; puts stderr " -> LINE = ${:processed_line}"
#   }
#   :method on_exit {line} {
#     puts -nonewline stderr "\t"; next; puts stderr " -> LINE = ${:processed_line}"
#   }
# }

# Class create ::nx::doc::CommentSection::Log {
#   :method on_enter {line} {
#     next; puts -nonewline stderr "\n"
#   }
#   :method on_exit {line} {
#     next; puts -nonewline stderr "\n";
#   }
# }

# set log false

# if {$log} {
#   ::nx::doc::CommentState mixin add ::nx::doc::CommentState::Log
#   ::nx::doc::CommentLine mixin add ::nx::doc::CommentLine::Log
#   ::nx::doc::CommentSection mixin add ::nx::doc::CommentSection::Log
# }

# --


Test case scanning {

  set lines {
    "# @package o"			1
    "#@package o"			1
    "bla"				0
    "# @object o"			1
    "# 1 2 3"				1
    "#"					1
    "#    "				1
    "   #   "				1
    "\t#\t \t"				1
    "#  345"				1
    "# @tag1 part1"			1
    "bla; # no comment"			0
    ""					0
    "\t\t"				0
    "### # # # # @object o # ####"	1
    "# # # # # 345"			1
    "# # # @tag1 part1"			1
    "bla; # # # # # no comment"		0
    "    "				0

  }
  
  foreach {::line ::result} $lines {
    ? {foreach {is_comment text} [doc analyze_line $::line] break; set is_comment} $::result "doc analyze_line '$::line'"
  }

  set script {
    # @package o
    # 1 2 3
    bla
    bla
    # @object o
    # 1 2 3
    #
    #  345 
    # @tag1 part1 
    # @tag2 part2
    bla; # no comment
    bla
    bla
    bla
    
    
    ### # # # # @object o # ####
    #    1 2 3
    #
    # # # # # 345 
    # # # @tag1 part1 
    # @tag2 part2
    bla; # # # # # no comment
  }

  set blocks {1 {{ @package o} { 1 2 3}} 5 {{ @object o} { 1 2 3} {} {  345} { @tag1 part1} { @tag2 part2}} 17 {{ @object o # ####} {    1 2 3} {} { 345} { @tag1 part1} { @tag2 part2}}}

  ? [list ::lcompare [doc comment_blocks $script] $blocks] 1
}

Test case parsing {
  #
  # TODO: Add tests for doc-parsing state machine.
  #
  set block {
    {@command ::cc}
  }

  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 0

  set block {
    {}
  }
  CommentBlockParser process $block
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 1

  #
  # For now, a valid comment block must start with a non-space line
  # (i.e., a tag or text line, depending on the section: context
  # vs. description)
  #
  
  set block {
    {}
    {@command ::cc}
  }
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 1

 set block {
   {command ::cc}
   {}
  }
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 1

  set block {
    {@command ::cc}
    {some description}
  }
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 1

 set block {
   {@command ::cc} 
   {}
   {}
   {}
   {@see ::o}
 }
  
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 0

 set block {
   {@command ::cc}
   {}
   {some description}
   {some description2}
   {}
   {}
  }
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 0

  # Note: We do allow description blocks with intermediate space
  # lines, for now.
  set block {
    {@command ::cc}
    {}
    {some description}
    {some description2}
    {}
    {an erroreneous description line, for now}
  }
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 0

  #
  # TODO: Do not enforce space line between the context and immediate
  # part block (when description is skipped)?
  # 
  # OR: For absolutely qualifying parts (e.g., outside of an initcmd block),
  # do we need sequences of _two_ (or more) tag lines, e.g.
  # 
  # --
  # @object Foo
  # @param attr1
  # --
  #
  # THEN, we can only discriminate between the context and an
  # immediate part section by requiring a space line!
  #
  # Alternatively, we can use the @see like syntax for qualifying:
  # @param ::Foo#attr1 (I have a preference for this option).
  set block {
    {@command ::cc}
    {@see someOtherEntity}
  }
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 1

  #
  # TODO: Disallow space lines between parts? Check back with Javadoc spec.
  #
  set block {
    {@command ::cc}
    {}
    {@see SomeOtherEntity}
    {add a line of description}
    {}
    {}
    {@see SomeOtherEntity2}
    {}
  }
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 0

  #
  # TODO: Should we enforce a mandatory space line between description and part block?
  #
  set block {
    {@command ::cc}
    {}
    {add a line of description}
    {a second line of description}
    {a third line of description}
    {@see entity3}
    {@see SomeOtherEntity2}
  }
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 1

  set block {
    {@command ::cc}
    {}
    {add a line of description}
    {a second line of description}
    {a third line of description}
    {}
    {@see SomeOtherEntity2}
    {}
    {}
    {an erroreneous description line, for now}
  }
  
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 1
  
  set block {
    {@command ::cc}
    {}
    {add a line of description}
    {a second line of description}
    {}
    {a third line of description}
    {}
    {@see SomeOtherEntity2}
  }
  ? [list StyleViolation thrown_by? [list CommentBlockParser process $block]] 0

  set block {
    {@object ::cc}
    {}
    {add a line of description}
    {a second line of description}
    {}
    {@see SomeOtherEntity2}
    {@xyz SomeOtherEntity2}
  }
  ? [list InvalidTag thrown_by? [list CommentBlockParser process $block]] 1

  set block {
    {@class ::cc}
    {}
    {add a line of description}
    {a second line of description}
    {}
    {@see SomeOtherEntity2}
    {@xyz SomeOtherEntity2}
  }
  ? [list InvalidTag thrown_by? [list CommentBlockParser process $block]] 1

  #
  # testing the doc object construction
  #
  set block {
    {@object ::o} 
    {}
    {some more text}
    {and another line for the description}
    {}
    {@author stefan.sobernig@wu.ac.at}
    {@author gustaf.neumann@wu-wien.ac.at}
  }
  set entity [CommentBlockParser process $block]
  ? [list ::nsf::is $entity object] 1
  ? [list $entity info is type ::nx::doc::@object] 1
  ? [list $entity @author] "stefan.sobernig@wu.ac.at gustaf.neumann@wu-wien.ac.at";
  ? [list $entity text] "some more text and another line for the description";

  set block {
    {@command ::c} 
    {}
    {some text on the command}
    {}
    {@see ::o}
  }
  set entity [CommentBlockParser process $block]
  puts stderr ENTITY=$entity
  ? [list ::nsf::is $entity object] 1
  ? [list $entity info is type ::nx::doc::@command] 1
  ? [list $entity text] "some text on the command";
  ? [list $entity @see] "::o";

  set block {
    {@class ::C} 
    {}
    {some text on the class entity}
    {}
    {@class-param attr1 Here, we check whether we can get a valid description block}
    {for text spanning multiple lines}
  }
  set entity [CommentBlockParser process $block]
  ? [list ::nsf::is $entity object] 1
  ? [list $entity info is type ::nx::doc::@class] 1
  ? [list $entity text] "some text on the command";
  ? [list llength [$entity @param]] 1
  ? [list [$entity @param] info is type ::nx::doc::@param]
  ? [list [$entity @param] @doc] ""
exit

  #
  # basic test for in-situ documentation (initcmd block)
  # 
  #

  set script {
    Class create Foo {
      # The class Foo defines the behaviour for all Foo objects
      #
      # @author gustaf.neumann@wu-wien.ac.at
      # @author ssoberni@wu.ac.at
      
      # @param attr1 
      #
      # This attribute 1 is wonderful
      #
      # @see ::nx::Attribute
      # @see ::nx::MetaSlot
      :attribute attr1
      :attribute attr2
      :attribute attr3

      # @method foo
      #
      # This describes the foo method
      #
      # @param a Provides a first value
      # @param b Provides a second value
      :method foo {a b} {;}
    }
  }

  eval $script
  doc process ::Foo
  set entity [@class id ::Foo]
  ? [list ::nsf::is $entity object] 1
  ? [list $entity info is type ::nx::doc::@class] 1
  ? [list $entity text] "The class Foo defines the behaviour for all Foo objects";
  ? [list $entity @author] "gustaf.neumann@wu-wien.ac.at ssoberni@wu.ac.at"
  # TODO: Fix the [@param id] programming scheme to allow (a) for
  # entities to be passed and the (b) documented structures
  #set entity [@param id ::Foo class attr1]
  set entity [@param id $entity attr1]
  ? [list ::nsf::is $entity object] 1
  ? [list $entity info is type ::nx::doc::@param] 1
  ? [list $entity @see] "::nx::Attribute ::nx::MetaSlot";

  set entity [@method id ::Foo class foo]
  ? [list [@class id ::Foo] @method] $entity
  ? [list ::nsf::is $entity object] 1
  ? [list $entity info is type ::nx::doc::@method] 1
  ? [list $entity text] "This describes the foo method";
  
  foreach p [$entity @param] expected {
    "Provides a first value"
    "Provides a second value"
  } {
    ? [list expr [list [$p text] eq $expected]] 1;
  }


  # TODO: how to realise scanning and parsing for mixed ex- and
  # in-situ documentation? That is, how to differentiate between
  # absolutely and relatively qualified comment blocks in line-based
  # scanning phase (or later)?

  set script {
    namespace import -force ::nx::*
    # @class ::Bar
    # 
    # The class Bar defines the behaviour for all Bar objects
    #
    # @author gustaf.neumann@wu-wien.ac.at
    # @author ssoberni@wu.ac.at

    # @param ::Bar#attr1 
    #
    # This attribute 1 is wonderful
    #
    # @see ::nx::Attribute
    # @see ::nx::MetaSlot

    # @method ::Bar#foo
    #
    # This describes the foo method
    #
    # @param a Provides a first value
    # @param b Provides a second value

    # @object-method ::Bar#foo
    #
    # This describes the per-object foo method
    #
    # @param a Provides a first value
    # @param b Provides a second value

    namespace eval ::ns1 {
      ::nx::Object create ooo
    }
    Class create Bar {

      :attribute attr1
      :attribute attr2
      :attribute attr3

      # @method foo
      #
      # This describes the foo method in the initcmd
      #
      # @param a Provides a first value
      # @param b Provides a second value

      :method foo {a b} {
	# This describes the foo method in the method body
	#
	# @param a Provides a first value (refined)

      }
      
      :object method foo {a b c} {
	# This describes the per-object foo method in the method body
	#
	# @param b Provides a second value (refined)
      	# @param c Provides a third value (first time)

      }

    }
  }

  set i [doc process $script]

  set entity [@class id ::Bar]
  ? [list $i eval [list ::nsf::is $entity object]] 1
  ? [list $i eval [list $entity info is type ::nx::doc::@class]] 1
  ? [list $i eval [list $entity text]] "The class Bar defines the behaviour for all Bar objects";
  ? [list $i eval [list $entity @author]] "gustaf.neumann@wu-wien.ac.at ssoberni@wu.ac.at"

  # TODO: Fix the [@param id] programming scheme to allow (a) for
  # entities to be passed and the (b) documented structures
  #set entity [@param id ::Bar class attr1]
  set entity [@param id $entity attr1]
  ? [list $i eval [list ::nsf::is $entity object]] 1
  ? [list $i eval [list $entity info is type ::nx::doc::@param]] 1
  ? [list $i eval [list $entity @see]] "::nx::Attribute ::nx::MetaSlot";

  set entity [@method id ::Bar class foo]
  ? [list $i eval [list [@class id ::Bar] @method]] $entity
  ? [list $i eval [list ::nsf::is $entity object]] 1
  ? [list $i eval [list $entity info is type ::nx::doc::@method]] 1
  ? [list $i eval [list $entity text]] "This describes the foo method in the method body";

  foreach p [$i eval [list $entity @param]] expected {
    "Provides a first value (refined)"
    "Provides a second value"
  } {
    ? [list expr [list [$i eval [list $p text]] eq $expected]] 1;
  }
  set entity [@method id ::Bar object foo]
  ? [list $i eval [list [@class id ::Bar] @object-method]] $entity
  ? [list $i eval [list ::nsf::is $entity object]] 1
  ? [list $i eval [list $entity info is type ::nx::doc::@method]] 1
  ? [list $i eval [list $entity text]] "This describes the per-object foo method in the method body";

  foreach p [$i eval [list $entity @param]] expected {
    "Provides a first value"
    "Provides a second value (refined)"
    "Provides a third value (first time)"
  } {
    ? [list expr [list [$i eval [list $p text]] eq $expected]] 1;
  }
 
  interp delete $i
  puts stderr =================================================
  #
  # self documentation
  #
  # if {[catch {set i [doc process nx::doc]} msg]} {
  #   puts stderr ERRORINFO=$::errorInfo
  #   if {[Exception behind? $msg]} {
  #     puts stderr [$msg info class]->[$msg message]
  #   } else {
  #     error $msg
  #   }
  # }
  # ? [list $i eval [list ::nsf::is [@package id nx::doc] object]] 1
  # puts stderr [$i eval [list [@package id nx::doc] text]]
  # puts stderr [$i eval [list [@package id nx::doc] @require]]
  # set path [file join /tmp nextdoc]
  # if {[file exists $path]} {
  #   file delete -force $path
  # }
  # $i eval [list ::nx::doc::make doc \
  # 	       -renderer ::nx::doc::TemplateData \
  # 	       -outdir /tmp \
  # 	       -project {name nextdoc url http://www.next-scripting.org/ version 0.1d}]
  # interp delete $i

  #
  # core documentation
  #
  foreach path [list [file join [::nsf::tmpdir] NextScriptingFramework] \
		    [file join [::nsf::tmpdir] NextScriptingLanguage]] {
    if {[file exists $path]} {
      file delete -force $path
    }
  }

  set i [interp create]
  $i eval {
    package req nx::doc
    namespace import ::nx::*
    namespace import ::nx::doc::*
    
    # 1) NSF documentation project
    
    set project [::nx::doc::@project new \
		     -name ::NextScriptingFramework \
		     -url http://www.next-scripting.org/ \
		     -version 1.0.0a \
		     -namespace "::nsf"]

    doc process -noeval true generic/predefined.tcl

    ::nx::doc::make doc \
	-renderer ::nx::doc::TemplateData \
	-outdir [::nsf::tmpdir] \
	-project $project

    set project [::nx::doc::@project new \
		     -name ::NextScriptingLanguage \
		     -url http://www.next-scripting.org/ \
		     -version 1.0.0a \
		     -namespace "::nx"]
    doc process -noeval true library/nx/nx.tcl
    ::nx::doc::make doc \
	-renderer ::nx::doc::TemplateData \
	-outdir [::nsf::tmpdir] \
	-project $project
  }

  interp delete $i
  
  set _ {
    # 2) XOTcl2 documentation project
    doc process -noeval true library/xotcl/xotcl.tcl
    ::nx::doc::make doc \
	-renderer ::nx::doc::TemplateData \
	-outdir [::nsf::tmpdir] \
	-project {name XOTcl2 url http://www.xotcl.org/ version 2.0.0a}
    
    # 3) NSL documentation project
    doc process -noeval true library/nx/nx.tcl
    ::nx::doc::make doc \
	-renderer ::nx::doc::TemplateData \
	-outdir [::nsf::tmpdir] \
	-project {name NextScriptingLanguage url http://www.next-scripting.org/ version 1.0.0a}
    
    # 4) Next Scripting Libraries
    # doc process -noeval true ...
    # ::nx::doc::make doc \
    # 	-renderer ::nx::doc::TemplateData \
    # 	-outdir [::nsf::tmpdir] \
    # 	-project {name NextScriptingLibraries url http://www.next-scripting.org/ version 1.0.0a}
  }

}


# # # # # # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # # # # # #

# 1) Test case scoping rules -> in Object->eval()
# Why does [info] intropsection not work as expected in eval()?

Test case issues? {

  # TODO: is [autoname -instance] really needed?
  #       is autoname needed in Next Scripting?

  # TODO: why is XOTclNextObjCmd/::nsf::next not in gentclAPI.decls?
  #       why should it be there? there are pros and cons, and very little benefit, or?

  # TODO: where to locate the @ comments (in predefined.xotcl, in
  # gentclAPI.decls)? how to deal with ::nsf::* vs. ::nx::*
  
  # TODO: which values are returned from Object->configure() and
  # passed to init()? how to document residualargs()?

  # TODO: Object->cleanup() said: "Resets an object or class into an
  # initial state, as after construction." If by construction it means
  # after create(), then cleanup() is missing a configure() call to
  # set defaults, etc! 
  # ?? cleanup does not set defaults; depending on "softrecreate", it 
  # deletes instances, childobjects, procs, instprocs, ....

  # TODO: exists and bestandteil von info() oder selbstständig?
  # ausserdem: erlauben von :-präfix?!

  # we have discussed this already

  # TODO: should we keep a instvar variant (i support this!)

  # what means "keep". next scripting should be mininmal, 
  # "instvar" is not needed and error-prone. We have now
  # "::nx::var import" and ::nsf::importvar
  # (of you want, similar to variable or global).

  # TODO: verify the use of filtersearch()? should it return a method
  # handle and the filter name? how to deal with it when refactoring
  # procsearch()?

  # ?? what does it return? What is the issue?

  # TODO: mixinguard doc is missing in old doc

  # mixinguard is described in the tutorial, it should have been documented
  # in the langref as well

  # TODO: what is Object->__next() for?

  # See the following script:
  #

# Object instproc defaultmethod {} {puts "[self proc]"; return [self]}
# Class A
# A instproc defaultmethod {} {puts "[self proc]"; [::xotcl::my info parent] __next}
# Class D -instproc t {} {puts "my stuff"}
# D create d1
# puts [d1 t]
# ### we create a subobject named t, which shadows effectively D->t
# A create d1::t
# puts ===
# # when we call "d1 t", we effectively call "d1::t", which calls "default method".
# # the defaultmethod should do a next on object d1.
# puts [d1 t]
# puts ===EXIT

  # but seems - at least in this usecase broken. Deactivated
  # in source for now.

  # TODO: what to do with hasNamespace()? [Object info is namespace]?

  # what is wrong with ":info hashNamespace"?

  # TODO: why is XOTclOUplevelMethodStub/XOTclOUplevelMethod defined
  # with "args" while it logically uses the stipulated parameter
  # signature (level ...). is this because of the first pos, optional
  # parameter? ... same goes for upvar() ...

  # the logic is a tribute to the argument logic in Tcl, which complex.
  #   uplevel ?level? arg ?arg ...?
  # It is a combination between an optional first argument and
  # and an args logic. 
  #
  # Most likely, it could be partly solved with a logic for optional
  # first arguments (if the number of actual arguments is 
  # higher than the minimal number of arguments, one could fill optional
  # paramter up..... but this calculation requires as well the interactions
  # with nonpos arguments, which might be values for positional arguments
  # as well.... not, sure, it is worth to invest much time here.

  # TODO: is Object->uplevel still needed with an integrated cs management?

  # yes, this is completely unrelated with the kind of callstack implemtation.
  # the methods upvar and uplevel are interceptor transparent, which means
  # that an uplevel will work for a method the same way, when a mixin or filter
  # are registered.

  # TODO: how is upvar affected by the ":"-prefixing? -> AVOID_RESOLVERS ...

  # this is a tcl question, maybe version dependent.
  
  # TODO: do all member-creating operations return valid, canonical handles!

  # what are member-creating operations? if you mean "method-creating methods"
  # they should (in next scripting) (i.e. necessary for e.g. method modifiers).

  # TODO: the objectsystems subcommand of ::nsf::configure does
  # not really fit in there because it does not allow for configuring
  # anything. it is a mere introspection-only command. relocate (can
  # we extend standard [info] somehow, i.e., [info objectsystems]

  # what means "configuring anything"? 
  # maybe you are refering to "configure objectsystems" which is parked there.
  # there would be an option to change the internally called methods via
  # configure as well, but i think, one is asking for troubles by allowing 
  # this.
  # extending info is possible via the shadowcommands, but the tct
  # does not like it.
  #
  # ad configure: we could fold as well methodproperty and
  # objectproperty into configure since these allow as well setting
  # and querying....
  #
  #   configure method METHODHANDLE public
  #   configure object OBJECT metaclass
  #
  # but there, the object property is just for quering.
  # Another option is define and "info"
  #
  #   ::nsf::info object OBJECT metaclass
  #   ::nsf::info objectsystems 
  #
  # but if we would fold these into tcl-info, conflicts with
  # tcl will arise.

  # TODO: extend [info level] & [info frame]!
  #
  # Why and what exactly?
  # If we would do it the tcloo-way, it would be very expensive.
  # whe have "info frame" implemnted with a less expensive approach since March 1

  # TODO: there is still --noArgs on [next], which does not correspond
  # to single-dashed flags used elsewhere. Why?
  #
  # (a) backward compatibility and (b) do you have suggestions?

  # TODO: renaming of self to current?
  #
  # what do you mean by "renaming"? both commands were available 
  # since a while. Maybe we should not import "self" into next scripting.
  #
  # DONE (self is not imported anymore, all occurrences in next tests are changed)
  # Not sure, we should keep since, since it will be a problem in many scripts
  # (e.g. in all slots, since slots are always next objects; maybe some advanced
  # OpenACS users will be hit).
  #
  
  # TODO: is [self callingclass] == [[self callingobject] info class]?
  #
  # no

  # TODO: "# @subcommand next Returns the name of the method next on
  # the precedence path as a string" shouldn't these kinds of
  # introspective commands return method handles (in the sense of
  # alias)? Retrieving the name from a handle is the more specific
  # operation (less generic). ... same for "filterreg"
  #
  # this is most likely "self next" and "self filterreg",
  # but applies as well for .e.g "info filter ... -order ..."
  # there are already changes to xotcl (see migration guide).
  # since the handle works now as well for "info method", 
  # this could be effectively done, but it requires 
  # backward compatibility.
  #
  # DONE
}

# if {$log} {
#   ::nx::doc::CommentState mixin delete ::nx::doc::CommentState::Log
#   ::nx::doc::CommentLine mixin delete ::nx::doc::CommentLine::Log
#   ::nx::doc::CommentSection mixin delete ::nx::doc::CommentSection::Log
# }
