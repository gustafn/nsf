package prefer latest
package require nx::test


nx::test case alias-redefine-method1 {
    #
    # redefine an object method by an alias pointing to an alias
    #
    proc ::foo args {;}
    
    nx::Object create o
    ? {::o public object method BAR {} {;}} ::o::BAR
    ? {::nsf::method::alias ::o bar ::foo} ::o::bar

    ? {info commands ::o::bar} ::o::bar              "::o::bar exists"
    ? {info commands ::o::BAR} ::o::BAR              "a command ::o::BAR exists"
    ? {::nsf::method::alias o BAR ::o::bar} ::o::BAR "redefine an object method with an alias (pointing to an alias) 87a2"
}

nx::test case alias-redefine-method2 {
    #
    # redefine an object method by an alias pointing to an object method
    #
    proc ::foo args {;}
    
    nx::Object create o
    ? {::o public object method BAR {} {;}} ::o::BAR
    ? {::o public object method FOO {} {;}} ::o::FOO

    ? {info commands ::o::FOO} ::o::FOO              "a command ::o::FOO exists"
    ? {info commands ::o::BAR} ::o::BAR              "a command ::o::BAR exists"
    ? {::nsf::method::alias o BAR ::o::FOO} ::o::BAR "redefine an object method with an alias (pointing to a method) 87a2"
}


nx::test case alias-double-alias-proc {

    proc ::foo args {;}
    nx::Object create o

    ? {info commands ::o::FOO} ""                    "a command ::o::FOO' does not exist"
    ? {info commands ::o::BAR} ""                    "a command ::o::BAR does not exist"
    ? {::nsf::method::alias o FOO ::foo}    ::o::FOO "define an object alias based on existing ::foo"
    ? {::nsf::method::alias o BAR ::o::FOO} ::o::BAR "define an object alias based on alias based on existing ::o::FOO"
}

nx::test case alias-double-alias-define {
    #
    # same as alias-double-reference-proc, but method instead of proc as target of o::FOO
    #
    proc ::foo args {;}

    nx::Object create o
    ? {::nsf::method::alias ::o bar ::foo} ::o::bar

    ? {info commands ::o::bar} ::o::bar              "::o::bar exists"
    ? {info commands ::o::FOO} ""                    "a command ::o::FOO' does not exists"
    ? {info commands ::o::BAR} ""                    "a command ::o::BAR does not exist"
    ? {::nsf::method::alias o FOO ::o::bar} ::o::FOO "define an object alias based on existing ::o::bar"
    ? {::nsf::method::alias o BAR ::o::FOO} ::o::BAR "define an object alias based on alias based on existing (?) ::o::bar"
}


nx::test case alias-double-alias-redefine {
    #
    # same as alias-double-reference-define, but redefined instead of new definition
    #
    proc ::foo args {;}
    
    nx::Object create o
    ? {::nsf::method::alias ::o FOO ::foo} ::o::FOO
    ? {::nsf::method::alias ::o bar ::foo} ::o::bar

    ? {info commands ::o::bar} ::o::bar              "::o::bar exists"
    ? {info commands ::o::FOO} ::o::FOO              "a command ::o::FOO' exists"
    ? {info commands ::o::BAR} ""                    "a command ::o::BAR does not exist"
    ? {::nsf::method::alias o FOO ::o::bar} ::o::FOO "redefine an object alias based on existing ::o::bar"
    ? {::nsf::method::alias o BAR ::o::FOO} ::o::BAR "define an object alias based on alias based on existing ::o::FOO"
}

nx::test case alias-double-alias-redefine0 {
    #
    # same as alias-double-reference-define, but redefined second cmd instead of new definition
    #
    proc ::foo args {;}
    
    nx::Object create o
    ? {::o public object method BAR {} {;}} ::o::BAR
    ? {::nsf::method::alias ::o bar ::foo} ::o::bar

    ? {info commands ::o::bar} ::o::bar              "::o::bar exists"
    ? {info commands ::o::FOO} ""                    "a command ::o::FOO' does not exist"
    ? {info commands ::o::BAR} ::o::BAR              "a command ::o::BAR exists"
    ? {::nsf::method::alias o FOO ::foo} ::o::FOO    "define an object alias based on existing ::foo"
    ? {::nsf::method::alias o BAR ::o::FOO} ::o::BAR "redefine an object alias based on alias based on existing ::o::FOO 87a2"
}

nx::test case alias-double-alias-redefine1 {
    #
    # same as alias-double-reference-define, but redefined second cmd instead of new definition
    #
    proc ::foo args {;}
    
    nx::Object create o
    ? {::o public object method BAR {} {;}} ::o::BAR
    ? {::nsf::method::alias ::o bar ::foo} ::o::bar

    ? {info commands ::o::bar} ::o::bar              "::o::bar exists"
    ? {info commands ::o::FOO} ""                    "a command ::o::FOO' does not exist"
    ? {info commands ::o::BAR} ::o::BAR              "a command ::o::BAR exists"
    ? {::nsf::method::alias o FOO ::o::bar} ::o::FOO "define an object alias based on existing ::o::bar"
    ? {::nsf::method::alias o BAR ::o::FOO} ::o::BAR "redefine an object alias based on alias based on existing ::o::FOO 87a2"
}

nx::test case alias-double-alias-redefine2 {
    #
    # same as alias-double-reference-define, but redefined twice instead of new definition
    #
    proc ::foo args {;}
    
    nx::Object create o
    ? {::nsf::method::alias ::o FOO ::foo} ::o::FOO
    ? {::o public object method BAR {} {;}} ::o::BAR
    ? {::nsf::method::alias ::o bar ::foo} ::o::bar

    ? {info commands ::o::bar} ::o::bar              "::o::bar exists"
    ? {info commands ::o::FOO} ::o::FOO              "a command ::o::FOO' exists"
    ? {info commands ::o::BAR} ::o::BAR              "a command ::o::BAR exists"
    ? {::nsf::method::alias o FOO ::o::bar} ::o::FOO "redefine an object alias based on existing ::o::bar"
    ? {::nsf::method::alias o BAR ::o::FOO} ::o::BAR "redefine an object alias based on alias based on existing ::o::FOO 87a2"
}



nx::test case alias-double-alias-object-method-redefine {

    proc ::foo args {;}
    
    nx::Object create o
    ? {::nsf::method::alias ::o FOO ::foo} ::o::FOO
    ? {::o public object method bar {} {;}} ::o::bar

    ? {info commands ::o::bar} ::o::bar              "handle ::o::bar exists"
    ? {info commands ::o::FOO} ::o::FOO              "a command ::o::FOO' exists"
    ? {info commands ::o::BAR} ""                    "a command ::o::BAR does not exist"
    ? {::nsf::method::alias o FOO ::o::bar} ::o::FOO "redefine an object alias based on existing (?) ::o::bar"
    ? {::nsf::method::alias o BAR ::o::FOO} ::o::BAR "define an object alias based on alias based on existing ::o::FOO"
    ? {info exists ::nsf::alias(::o,FOO,1)} 1
    ? {info exists ::nsf::alias(::o,BAR,1)} 1
    
    o public object method bar {} {}
    ? {info exists ::nsf::alias(::o,FOO,1)} 1
    ? {info exists ::nsf::alias(::o,BAR,1)} 1
}


nx::test case alias-double-alias-object-method-redefine2 {

    proc ::foo args {;}
    
    nx::Object create o
    ? {::nsf::method::alias ::o FOO ::foo} ::o::FOO
    ? {::o public object method BAR {} {;}} ::o::BAR
    ? {::o public object method bar {} {;}} ::o::bar

    ? {info commands ::o::bar} ::o::bar              "handle ::o::bar exists"
    ? {info commands ::o::FOO} ::o::FOO              "a command ::o::FOO' exists"
    ? {info commands ::o::BAR} ::o::BAR              "a command ::o::BAR does not exist"
    ? {::nsf::method::alias o FOO ::o::bar} ::o::FOO "redefine an object alias based on existing (?) ::o::bar"
    ? {::nsf::method::alias o BAR ::o::FOO} ::o::BAR "redefine an object alias based on alias based on existing ::o::FOO 87a2"
}


