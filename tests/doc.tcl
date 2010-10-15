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

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? COMPLETED] 1

  set block {
    {}
  }

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? COMPLETED] 0
  ? [list $cbp status ? STYLEVIOLATION] 1
  
  #
  # For now, a valid comment block must start with a non-space line
  # (i.e., a tag or text line, depending on the section: context
  # vs. description)
  #
  
  set block {
    {}
    {@command ::cc}
  }
  
  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 1

  set block {
    {command ::cc}
    {}
  }

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 1

  set block {
    {@command ::cc}
    {some description}
  }
 
  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 1
 
 set block {
   {@command ::cc} 
   {}
   {}
   {}
   {@see ::o}
 }
  
  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 0
  ? [list $cbp status ? COMPLETED] 1

 set block {
   {@command ::cc}
   {}
   {some description}
   {some description2}
   {}
   {}
  }

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 0

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
  
    set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 0

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

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 1
  

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

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 1

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

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 1

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

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 1 
  
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

    set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 0

  set block {
    {@object ::cc}
    {}
    {add a line of description}
    {a second line of description}
    {}
    {@see SomeOtherEntity2}
    {@xyz SomeOtherEntity2}
  }

    set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? INVALIDTAG] 1

  set block {
    {@class ::cc}
    {}
    {add a line of description}
    {a second line of description}
    {}
    {@see SomeOtherEntity2}
    {@xyz SomeOtherEntity2}
  }

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? INVALIDTAG] 1

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

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? COMPLETED] 1
  set entity [$cbp current_entity]
  ? [list ::nsf::is object $entity] 1
  ? [list $entity info has type ::nx::doc::@object] 1
  ? [list $entity @author] "stefan.sobernig@wu.ac.at gustaf.neumann@wu-wien.ac.at";
  ? [list $entity as_text] "some more text and another line for the description";
  
  set block {
    {@command ::c} 
    {}
    {some text on the command}
    {}
    {@see ::o}
  }

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? COMPLETED] 1
  set entity [$cbp current_entity]

  ? [list ::nsf::is object $entity] 1
  ? [list $entity info has type ::nx::doc::@command] 1
  ? [list $entity as_text] "some text on the command";
  ? [list $entity @see] "::o";

  set block {
    {@class ::C} 
    {}
    {some text on the class entity}
    {}
    {@class-attribute attr1 Here! we check whether we can get a valid description block}
    {for text spanning multiple lines}
  }

  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? COMPLETED] 1
  set entity [$cbp current_entity]

  ? [list ::nsf::is object $entity] 1
  ? [list $entity info has type ::nx::doc::@class] 1
  ? [list $entity as_text] "some text on the class entity";
  ? [list llength [$entity @attribute]] 1
  ? [list [$entity @attribute] info has type ::nx::doc::@param] 1
  ? [list [$entity @attribute] as_text] "Here! we check whether we can get a valid description block for text spanning multiple lines"

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
      
      # @.attribute attr1 
      #
      # This attribute 1 is wonderful
      #
      # @see ::nx::Attribute
      # @see ::nx::MetaSlot
      :attribute attr1
      :attribute attr2
      :attribute attr3

      # @.method foo
      #
      # This describes the foo method
      #
      # @parameter a Provides a first value
      # @parameter b Provides a second value
      :method foo {a b} {;}
    }
  }

  eval $script
  doc process ::Foo
  set entity [@class id ::Foo]
  ? [list ::nsf::is object $entity] 1
  ? [list $entity info has type ::nx::doc::@class] 1
  ? [list $entity as_text] "The class Foo defines the behaviour for all Foo objects";
  ? [list $entity @author] "gustaf.neumann@wu-wien.ac.at ssoberni@wu.ac.at"
  # TODO: Fix the [@param id] programming scheme to allow (a) for
  # entities to be passed and the (b) documented structures
  set entity [@attribute id [@class id ::Foo] class attr1]
  ? [list ::nsf::is object $entity] 1
  ? [list $entity info has type ::nx::doc::@attribute] 1
  ? [list $entity @see] "::nx::Attribute ::nx::MetaSlot";

  set entity [@method id ::Foo class foo]
  ? [list [@class id ::Foo] @method] $entity
  ? [list ::nsf::is object $entity] 1
  ? [list $entity info has type ::nx::doc::@method] 1
  ? [list $entity as_text] "This describes the foo method";
  
  foreach p [$entity @parameter] expected {
    "Provides a first value"
    "Provides a second value"
  } {
    ? [list expr [list [$p as_text] eq $expected]] 1;
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

    # @class.attribute {::Bar attr1}
    #
    # This attribute 1 is wonderful
    #
    # @see ::nx::Attribute
    # @see ::nx::MetaSlot

    # @class.class-method {::Bar foo}
    #
    #
    # This describes the foo method
    #
    # @parameter a Provides a first value
    # @parameter b Provides a second value

    # @class.class-object-method {::Bar foo}
    #
    # This describes the per-object foo method
    #
    # @parameter a Provides a first value
    # @parameter b Provides a second value

    namespace eval ::ns1 {
      ::nx::Object create ooo
    }
    Class create Bar {

      :attribute attr1
      :attribute attr2
      :attribute attr3

      # @.method foo
      #
      # This describes the foo method in the initcmd
      #
      # @parameter a Provides a first value
      # @parameter b Provides a second value

      :method foo {a b} {
	# This describes the foo method in the method body
	#
	# @parameter a Provides a first value (refined)

      }
      
      :class-object method foo {a b c} {
	# This describes the per-object foo method in the method body
	#
	# @parameter b Provides a second value (refined)
      	# @parameter c Provides a third value (first time)

      }

    }
  }

  set i [doc process $script]

  set entity [@class id ::Bar]
  ? [list $i eval [list ::nsf::is object $entity]] 1
  ? [list $i eval [list $entity info has type ::nx::doc::@class]] 1
  ? [list $i eval [list $entity as_text]] "The class Bar defines the behaviour for all Bar objects";
  ? [list $i eval [list $entity @author]] "gustaf.neumann@wu-wien.ac.at ssoberni@wu.ac.at"

  # TODO: Fix the [@param id] programming scheme to allow (a) for
  # entities to be passed and the (b) documented structures
  set entity [@attribute id [@class id ::Bar] class attr1]
  ? [list $i eval [list ::nsf::is object $entity]] 1
  ? [list $i eval [list $entity info has type ::nx::doc::@attribute]] 1
  ? [list $i eval [list $entity @see]] "::nx::Attribute ::nx::MetaSlot";

  set entity [@method id ::Bar class foo]
  ? [list $i eval [list [@class id ::Bar] @method]] $entity
  ? [list $i eval [list ::nsf::is object $entity]] 1
  ? [list $i eval [list $entity info has type ::nx::doc::@method]] 1
  ? [list $i eval [list $entity as_text]] "This describes the foo method in the method body";

  foreach p [$i eval [list $entity @parameter]] expected {
    "Provides a first value (refined)"
    "Provides a second value"
  } {
    ? [list expr [list [$i eval [list $p as_text]] eq $expected]] 1;
  }


  set entity [@method id ::Bar class-object foo]
  ? [list $i eval [list [@class id ::Bar] @class-object-method]] $entity
  ? [list $i eval [list ::nsf::is object $entity]] 1
  ? [list $i eval [list $entity info has type ::nx::doc::@method]] 1
  ? [list $i eval [list $entity as_text]] "This describes the per-object foo method in the method body";

  foreach p [$i eval [list $entity @parameter]] expected {
    "Provides a first value"
    "Provides a second value (refined)"
    "Provides a third value (first time)"
  } {
    ? [list expr [list [$i eval [list $p as_text]] eq $expected]] 1;
  }

 
  interp delete $i


  #
  # Some tests on structured/navigatable tag notations
  #

  # adding support for parsing levels
  
  # -- @class.object.object {::D o1 o2}
  set block {
    {@..object o2 We have a tag notation sensitive to the parsing level}
  }
  
  set entity [[@ @class ::D] @object o1]
  set cbp [CommentBlockParser process -parsing_level 1 -partof_entity $entity $block]
  ? [list $cbp status ? LEVELMISMATCH] 1
  set cbp [CommentBlockParser process -parsing_level 2 -partof_entity $entity $block]
  ? [list $cbp status ? COMPLETED] 1
  set entity [$cbp current_entity]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@object] 1
  ? [list $entity as_text] "We have a tag notation sensitive to the parsing level"

  set block {
    {@..object {o2 o3} We still look for balanced specs}
  }

  set entity [[@ @class ::D] @object o1]
  set cbp [CommentBlockParser process -parsing_level 2 -partof_entity $entity $block]
  ? [list $cbp status ? STYLEVIOLATION] 1

  # This fails because we do not allow uninitialised/non-existing
  # entity objects (@object o) along the resolution path ...
  set block {
    {@class.object.attribute {::C o attr1} We have an invalid specification}
  }
  
  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? INVALIDTAG] 1
#  ? [list $cbp message] "The tag 'object' is not supported for the entity type '@class'"

  set block {
    {@class.method.attribute attr1 We have an imbalanced specification (the names are underspecified!)}
  }
  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 1
  ? [list $cbp message] "Imbalanced tag line spec: 'class method attribute' vs. 'attr1'"

  # For now, we do not verify and use a fixed scope of permissive tag
  # names. So, punctuation errors or typos are most probably reported
  # as imbalanced specs. In the mid-term run, this should rather
  # become an INVALIDTAG condition.
  set block {
    {@cla.ss.method.parameter {::C foo p1} We mistyped a tag fragment}
  }
  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? STYLEVIOLATION] 1
  ? [list $cbp message] "Imbalanced tag line spec: 'cla ss method parameter' vs. '::C foo p1'"

  set block {
    {@cla,ss.method.parameter {::C foo p1} We mistyped a tag fragment}
  }
  set cbp [CommentBlockParser process $block]
  ? [list $cbp status ? INVALIDTAG] 1
  ? [list $cbp message] "The entity type '@cla,ss' is not available."

  set script {
    # @class ::C
    #
    # The global description of ::C
    #
    # @attribute attr1 Here we can only provide a description block for object parameters
    
    # @class.attribute {::C attr1} Here, we could also write '@class.class-attribute \{::C attr1\}', @attribute is a mere forwarder! In the context section, only one-liners are allowed!

    # @class.object.attribute {::C foo p1} A short description is ...
    #
    # .. is overruled by a long one ...

    # If addressing to a nested object, one strategy would be to use
    # @object and provide the object identifier (which reflects the
    # nesting, e.g. ::C::foo). However, we cannot distinguish between
    # namespace qualifiers denoting an object, class or owning
    # namespace!
    #
    # ISSUE: If specifying an axis ".object", we would have to define
    # a part attribute @object on @class and @object. However, @object
    # would be ambiguous now: It could be called in a freestanding
    # (absolute) manner AND in a contextualised manner (in an initcmd
    # script). In the latter case, it would fail because we would have
    # to provide a FQ'ed name (which defeats the purpose of a nested =
    # contextualised notation).
    # 
    # SO: for now, we introduce a part attribute child-object (and
    # child-class?) to discrimate between the two situations ...
    #
    # TODO: How to register this so created @object entity as nested
    # object with the doc entity represented the parent object?
    
    Class create C {
      # This is the initcmd-level description of ::C which overwrites the
      # global description (see above)
      
      # @.attribute attr1
      #
      # This is equivalent to writing "@class-attribute attr1"
      :attribute attr1 {
	# This description does not apply to the object parameter
	# "attr1" owned by the ::C class, rather it is a description
	# of the attribute slot object! How should we deal with this
	# situation? Should this level overwrite the top-level and
	# initcmd-level descriptions?
      }
      
      # @.class-object-attribute attr2 Carries a short desc only
      :class-object attribute attr2
      
      # @.method foo
      #
      # @parameter p1
      set fooHandle [:method foo {p1} {
	# Here goes some method-body-level description
	#
	# @parameter p1 The most specific level!
	return [current method]-$p1-[current]
      }]
           
      # @.class-object-method.parameter {bar p1}
      #
      # This extended form allows to describe a method parameter with all
      # its structural features!
      set barHandle [:class-object method bar {p1} {
	return [current method]-$p1-[current]
      }]

      # @.object foo 'foo' needs to be defined before referencing any of its parts!

      # @.object.attribute {foo p1}
      #
      # The first element in the name list is resolved into a fully
      # qualified (absolute) entity, based on the object owning the
      # initcmd!
      Object create [current]::foo {
	# Adding a line for the first time (not processed in the initcmd phase!)
	
	# @..attribute p1
	# 
	# This is equivalent to stating "@class-object-attribute p1"
	:attribute p1
      }
      
      # @.class Foo X
      #
      # By providing a fully-qualified identifier ("::Foo") you leave the
      # context of the initcmd-owning object, i.e. you would NOT refer to 
      # a nested class object named "Foo" anymore!
      
      # @.class.attribute {Foo p1}
      #
      # This is equivalent to stating "@child-class.class-attribute {Foo p1}"
      
      # @.class.class-object-attribute {Foo p2} Y
      Class create [current]::Foo {

	# @..attribute p1
	# 
	#
	# This is equivalent to stating "@class-attribute p1"; or
	# '@class.object.attribute {::C Foo p1}' from the top-level.
	:attribute p1
	
	# @..class-object-attribute p2
	:class-object attribute p2
      }
      
      
      # @.class-object-method.sub-method {sub foo}
      #
      # ISSUE: Should submethods be navigatable through "method" (i.e.,
      # "@method.method.method ...") or "submethod" (i.e.,
      # "@method.submethod.submethod ...")? ISSUE: Should it be sub* with
      # "-" (to correspond to "@class-object-method", "@class-method")? Also, we
      # could allow both (@sub-method is the attribute name, @method is a
      # forwarder in the context of an owning @method object!)
      # 
      # @parameter p1 Some words on p1
      :class-object alias "sub foo" $fooHandle
      
      # @.method sub
      #
      # The desc of the ensemble object 'sub'
      #
      # @sub-method bar Only description available here ...

      # ISSUE: Should the helper object "sub" be documentable in its own
      # right?  This would be feasible with the dotted notation from
      # within and outside the initcmd script block, e.g. "@object sub" or
      # "@class.object {::C sub}"
      #
      # ISSUE: Is it correct to say the sub appears as per-object method
      # and so do its submethods? Or is it misleading to document it that
      # way? Having an "@class-object-submethod" would not make much sense to
      # me?!
      :alias "sub bar" $barHandle 

      # @.class-object-method sub A brief desc

      # @.class-object-method {"sub foo2"}
      #
      # could allow both (@sub-method is the attribute name, @method is a
      # forwarder in the context of an owning @method object!)
      # 
      # @parameter p1 Some words on p1
      # @see anotherentity
      # @author ss@thinkersfoot.net
      :class-object alias "sub foo2" $fooHandle
    }
  }

  #
  # 1) process the top-level comments (PARSING LEVEL 0)
  #

  doc analyze -noeval true $script

  # --testing-- "@class ::C"
  set entity [@class id ::C]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@class] 1
  ? [list $entity as_text] "The global description of ::C";
  # --testing-- "@class.attribute {::C attr1}"
  set entity [@attribute id $entity class attr1]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@attribute] 1
  ? [list $entity as_text] "Here, we could also write '@class.class-attribute {::C attr1}', @attribute is a mere forwarder! In the context section, only one-liners are allowed!"

  # --testing-- "@class.object.attribute {::C foo p1} A short description is ..."
  # set entity [@attribute id $entity class attr1]
  # set entity [@object id -partof_name ::C -scope child foo]
  # ? [list ::nsf::isobject $entity] 1
  # ? [list $entity info has type ::nx::doc::@object] 1
  # ? [list $entity as_text] ""
  # set entity [@attribute id $entity object p1]
  # ? [list ::nsf::isobject $entity] 1
  # ? [list $entity info has type ::nx::doc::@attribute] 1
  # ? [list $entity as_text] ".. is overruled by a long one ..."

  set entity [@object id ::C::foo]
  ? [list ::nsf::isobject $entity] 0
  set entity [@attribute id $entity class-object p1]
  ? [list ::nsf::isobject $entity] 0
  #  ? [list $entity info has type ::nx::doc::@attribute] 1
  #  ? [list $entity as_text] ".. is overruled by a long one ..."

  # --testing-- @class-object-attribute attr2 (its non-existance)
  set entity [@attribute id [@class id ::C] class-object attr2]
  ? [list ::nsf::isobject $entity] 0
  # --testing-- @child-class Foo (its non-existance)
  set entity [@class id ::C::Foo]
  ? [list ::nsf::isobject $entity] 0
  # --testing -- @method foo (its non-existance)
  set entity [@method id ::C class foo]
  ? [list ::nsf::isobject $entity] 0
  # --testing-- @class-object-method.parameter {bar p1} (its non-existance)
  set entity [@parameter id [@method id ::C class-object bar] "" p1]
  ? [list ::nsf::isobject $entity] 0
  # --testing-- @child-object.attribute {foo p1} (its non-existance)
  set cl [@class id ::C::Foo]
  ? [list ::nsf::isobject $entity] 0
  set entity [@attribute id $cl class p1]
  ? [list ::nsf::isobject $entity] 0
  set entity [@attribute id $cl class-object p2]
  ? [list ::nsf::isobject $entity] 0

  #
  # 2) process the initcmd comments (PARSING LEVEL 1)
  #
  
  eval $script

  doc analyze_initcmd @class ::C [::C eval {set :__initcmd}]

  # a) existing, but modified ...

  set entity [@class id ::C]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@class] 1
  ? [list $entity as_text] "This is the initcmd-level description of ::C which overwrites the global description (see above)"
    
  set entity [@attribute id $entity class attr1]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@attribute] 1
  ? [list $entity as_text] {This is equivalent to writing "@class-attribute attr1"}


  set entity [@object id ::C::foo]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@object] 1
  ? [list $entity as_text] "'foo' needs to be defined before referencing any of its parts!"; # still empty!
  set entity [@attribute id $entity class-object p1]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@attribute] 1
  ? [list $entity as_text] "The first element in the name list is resolved into a fully qualified (absolute) entity, based on the object owning the initcmd!"

  # b) newly added ...

  # --testing-- @class-object-attribute attr2
  set entity [@attribute id [@class id ::C] class-object attr2]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@attribute] 1
  ? [list $entity as_text] "Carries a short desc only";

  # --testing-- @child-class Foo
  # TODO: provide a check against fully-qualified names in part specifications
  set entity [@class id ::C::Foo]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@class] 1
  ? [list $entity as_text] {By providing a fully-qualified identifier ("::Foo") you leave the context of the initcmd-owning object, i.e. you would NOT refer to a nested class object named "Foo" anymore!}

  set entity [@attribute id [@class id ::C] class p1]
  ? [list ::nsf::isobject $entity] 0; # should be 0 at this stage!

  # --testing -- @method foo
  set entity [@method id ::C class foo]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] ""
  # --testing-- @class-object-method.parameter {bar p1} (its non-existance) It
  # still cannot exist as a documented entity, as the class-object method
  # has not been initialised before!
  set entity [@parameter id [@method id ::C class-object bar] "" p1]
  ? [list ::nsf::isobject $entity] 0
  # --testing-- @child-class.attribute {foo p1} (its non-existance)
  # --testing-- @child-class.object-attribute {foo p2} (its non-existance)
  set cl [@class id ::C::Foo]
  ? [list ::nsf::isobject $cl] 1
  set entity [@attribute id $cl class p1]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] {This is equivalent to stating "@child-class.class-attribute {Foo p1}"}
  set entity [@attribute id $cl class-object p2]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] "Y"

  set entity [@method id ::C class sub]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] "The desc of the ensemble object 'sub'"

  set entity [@method id ::C class sub::bar]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] "Only description available here ..."

  set entity [@method id ::C class-object sub]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] "A brief desc"

  set entity [@method id ::C class-object sub::foo2]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@method] 1
  ? [list $entity as_text] "could allow both (@sub-method is the attribute name, @method is a forwarder in the context of an owning @method object!)"
  ? [list $entity @see] "anotherentity"
  # TODO: @author not supported for @method (fine so?)
  # ? [list $entity @author] "ss@thinkersfoot"
  set entity [@parameter id $entity "" p1]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] "Some words on p1"

  #
  # 3a) process the attribute initcmds and method bodies (PARSING LEVEL 2)!
  #  

  doc process=@class [@class id ::C]
  
  # methods ...

  set entity [@method id ::C class foo]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] "Here goes some method-body-level description"
  set entity [@parameter id [@method id ::C class foo] "" p1]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] "The most specific level!"

  # attributes ...

  # attr1 
  set entity [@attribute id [@class id ::C] class attr1]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@attribute] 1
  ? [list $entity as_text] {This description does not apply to the object parameter "attr1" owned by the ::C class, rather it is a description of the attribute slot object! How should we deal with this situation? Should this level overwrite the top-level and initcmd-level descriptions?}

  #
  # 3b) nested objects/ classes (PARSING LEVEL 2)!
  #  

  doc analyze_initcmd -parsing_level 2 @object ::C::foo [::C::foo eval {set :__initcmd}]
  doc process=@object [@object id ::C::foo]

  set entity [@object id ::C::foo]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@object] 1
  ? [list $entity as_text] "Adding a line for the first time (not processed in the initcmd phase!)"; # still empty!
  set entity [@attribute id $entity class-object p1]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity info has type ::nx::doc::@attribute] 1
  ? [list $entity as_text] {This is equivalent to stating "@class-object-attribute p1"}

  doc analyze_initcmd -parsing_level 2 @class ::C::Foo [::C::Foo eval {set :__initcmd}]
  doc process=@class [@class id ::C::Foo]

  set cl [@class id ::C::Foo]
  ? [list ::nsf::isobject $cl] 1
  set entity [@attribute id $cl class p1]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] {This is equivalent to stating "@class-attribute p1"; or '@class.object.attribute {::C Foo p1}' from the top-level.}
  set entity [@attribute id $cl class-object p2]
  ? [list ::nsf::isobject $entity] 1
  ? [list $entity as_text] ""

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
  # ? [list $i eval [list ::nsf::is object [@package id nx::doc]]] 1
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
		     -name NextScriptingFramework \
		     -url http://www.next-scripting.org/ \
		     -version 1.0.0a \
		     -@namespace "::nsf"]

    doc process -noeval true generic/nsf.tcl

    ::nx::doc::make doc \
    	-renderer ::nx::doc::NxDocRenderer \
    	-project $project \
    	-outdir [::nsf::tmpdir]

    #puts stderr NSF=[info commands ::nx::doc::entities::command::nsf::*]

    puts stderr TIMING=[time {
    set project [::nx::doc::@project new \
    		     -name NextScriptingLanguage \
    		     -url http://www.next-scripting.org/ \
    		     -version 1.0.0a \
    		     -@namespace "::nx"]
      # ISSUE: If calling '-namespace "::nx"' instead of '-@namespace
      # "::nx"', we get an irritating failure. VERIFY!
    doc process -noeval true library/nx/nx.tcl
    ::nx::doc::make doc \
    	-renderer ::nx::doc::NxDocRenderer \
    	-project $project \
    	-outdir [::nsf::tmpdir]
    } 1]
  }

  interp delete $i
  
  set _ {
    # 2) XOTcl2 documentation project
    doc process -noeval true library/xotcl/xotcl.tcl
    ::nx::doc::make doc \
	-renderer ::nx::doc::NxDocTemplateData \
	-outdir [::nsf::tmpdir] \
	-project {name XOTcl2 url http://www.xotcl.org/ version 2.0.0a}
    
    # 3) NSL documentation project
    doc process -noeval true library/nx/nx.tcl
    ::nx::doc::make doc \
	-renderer ::nx::doc::NxDocTemplateData \
	-outdir [::nsf::tmpdir] \
	-project {name NextScriptingLanguage url http://www.next-scripting.org/ version 1.0.0a}
    
    # 4) Next Scripting Libraries
    # doc process -noeval true ...
    # ::nx::doc::make doc \
    # 	-renderer ::nx::doc::NxDocTemplateData \
    # 	-outdir [::nsf::tmpdir] \
    # 	-project {name NextScriptingLibraries url http://www.next-scripting.org/ version 1.0.0a}
  }

}


# # # # # # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # # # # # #
# # # # # # # # # # # # # # # # # # # #

# 1) Test case scoping rules -> in Object->eval()

Test case issues? {

  # TODO: where to locate the @ comments (in predefined.xotcl, in
  # gentclAPI.decls)? how to deal with ::nsf::* vs. ::nx::*
  
  # TODO: which values are returned from Object->configure() and
  # passed to init()? how to document residualargs()?

  # TODO: Object->cleanup() said: "Resets an object or class into an
  # initial state, as after construction." If by construction it means
  # after create(), then cleanup() is missing a configure() call to
  # set defaults, etc! 
  # ?? cleanup does not set defaults; depending on "softrecreate", it 
  # deletes instances, childobjects, procs, instprocs, ...

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
  # parameter up..... but this calculation requires as well the interactions
  # with nonpos arguments, which might be values for positional arguments
  # as well.... not, sure, it is worth to invest much time here.

  # TODO: how is upvar affected by the ":"-prefixing? -> AVOID_RESOLVERS ...

  # this is a tcl question, maybe version dependent.
  


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

  # ISSUE: Object->info->parameter() still needed to retrieve
  # objectparameters?

  # TODO: decide how to deal with @package and @project names (don't
  # need namespace delimiters!)

}

# if {$log} {
#   ::nx::doc::CommentState mixin delete ::nx::doc::CommentState::Log
#   ::nx::doc::CommentLine mixin delete ::nx::doc::CommentLine::Log
#   ::nx::doc::CommentSection mixin delete ::nx::doc::CommentSection::Log
# }
