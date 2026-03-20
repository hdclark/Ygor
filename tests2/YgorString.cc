
#include <iostream>
#include <sstream>
#include <array>

#include <YgorMisc.h>
#include "YgorLog.h"
#include <YgorString.h>

#include "doctest/doctest.h"


TEST_CASE( "Expand_Macros" ){

    SUBCASE("Defaults, without brackets"){
        std::string text = "$THIS is a $TEST";
        std::map<std::string, std::string> replacements { { "THIS", "This" }, { "TEST", "test" } };
        std::string expected = "This is a test";

        auto out = ExpandMacros(text, replacements);
        REQUIRE(out == expected);
    }

    SUBCASE("Defaults, without brackets, partial expansion"){
        std::string text = "$THIS is a $TEST";
        std::map<std::string, std::string> replacements { { "THIS", "This" } };
        std::string expected = "This is a $TEST";

        auto out = ExpandMacros(text, replacements);
        REQUIRE(out == expected);
    }


    SUBCASE("Defaults, with brackets"){
        std::string text = "${THIS} is a ${TEST}";
        std::map<std::string, std::string> replacements { { "THIS", "This" }, { "TEST", "test" } };
        std::string expected = "This is a test";

        auto out = ExpandMacros(text, replacements);
        REQUIRE(out == expected);
    }

    SUBCASE("Defaults, brackets delineate replacements properly"){
        std::string text = "${SCRUNCHED}TOGETHER";
        std::map<std::string, std::string> replacements { { "SCRUNCHED", "not scrunched " } };
        std::string expected = "not scrunched TOGETHER";

        auto out = ExpandMacros(text, replacements);
        REQUIRE(out == expected);
    }

    SUBCASE("Defaults, with brackets, partial expansion"){
        std::string text = "${THIS} is a ${TEST}";
        std::map<std::string, std::string> replacements { { "THIS", "This" } };
        std::string expected = "This is a ${TEST}";

        auto out = ExpandMacros(text, replacements);
        REQUIRE(out == expected);
    }


    SUBCASE("Defaults, mixed with and without brackets"){
        std::string text = "$THIS is a ${TEST}";
        std::map<std::string, std::string> replacements { { "THIS", "This" }, { "TEST", "test" } };
        std::string expected = "This is a test";

        auto out = ExpandMacros(text, replacements);
        REQUIRE(out == expected);
    }


    SUBCASE("Custom indicator symbol"){
        std::string text = "@THIS is a @TEST";
        std::map<std::string, std::string> replacements { { "THIS", "This" }, { "TEST", "test" } };
        std::string expected = "This is a test";

        auto out = ExpandMacros(text, replacements, "@");
        REQUIRE(out == expected);
    }


    SUBCASE("Custom brackets []"){
        std::string text = "$[THIS] is a $[TEST]";
        std::map<std::string, std::string> replacements { { "THIS", "This" }, { "TEST", "test" } };
        std::string expected = "This is a test";
        std::string allowed_chars = "[:alnum:]";
        std::string allowed_brackets = "[]";

        auto out = ExpandMacros(text, replacements, "$", allowed_chars, allowed_brackets);
        REQUIRE(out == expected);
    }

    SUBCASE("Custom brackets ()"){
        std::string text = "$(THIS) is a $(TEST)";
        std::map<std::string, std::string> replacements { { "THIS", "This" }, { "TEST", "test" } };
        std::string expected = "This is a test";
        std::string allowed_chars = "[:alnum:]";
        std::string allowed_brackets = "()";

        auto out = ExpandMacros(text, replacements, "$", allowed_chars, allowed_brackets);
        REQUIRE(out == expected);
    }

    SUBCASE("Mixed custom brackets () and []"){
        std::string text = "$[THIS] is a $(TEST)";
        std::map<std::string, std::string> replacements { { "THIS", "This" }, { "TEST", "test" } };
        std::string expected = "This is a test";
        std::string allowed_chars = "[:alnum:]";
        std::string allowed_brackets = "()[]";

        auto out = ExpandMacros(text, replacements, "$", allowed_chars, allowed_brackets);
        REQUIRE(out == expected);
    }

    SUBCASE("Mixed custom brackets () and [] but only () replaced"){
        std::string text = "$[THIS] is a $(TEST)";
        std::map<std::string, std::string> replacements { { "THIS", "This" }, { "TEST", "test" } };
        std::string expected = "$[THIS] is a test";
        std::string allowed_chars = "[:alnum:]";
        std::string allowed_brackets = "()";

        auto out = ExpandMacros(text, replacements, "$", allowed_chars, allowed_brackets);
        REQUIRE(out == expected);
    }

    SUBCASE("Mixed custom brackets () and [] but only [] replaced"){
        std::string text = "$[THIS] is a $(TEST)";
        std::map<std::string, std::string> replacements { { "THIS", "This" }, { "TEST", "test" } };
        std::string expected = "This is a $(TEST)";
        std::string allowed_chars = "[:alnum:]";
        std::string allowed_brackets = "[]";

        auto out = ExpandMacros(text, replacements, "$", allowed_chars, allowed_brackets);
        REQUIRE(out == expected);
    }

}


TEST_CASE( "Get_Parent_Directory" ){

    SUBCASE("/some/file"){
        REQUIRE( Get_Parent_Directory("/some/file") == "/some/" );
    }
    SUBCASE("/some/path/"){
        REQUIRE( Get_Parent_Directory("/some/path/") == "/some/" );
    }
    SUBCASE("some/path/"){
        REQUIRE( Get_Parent_Directory("some/path/") == "some/" );
    }
    SUBCASE("some/file"){
        REQUIRE( Get_Parent_Directory("some/file") == "some/" );
    }
    SUBCASE("/some/"){
        REQUIRE( Get_Parent_Directory("/some/") == "/" );
    }
    SUBCASE("/"){
        REQUIRE( Get_Parent_Directory("/") == "" );
    }
    SUBCASE("bare file"){
        REQUIRE( Get_Parent_Directory("file") == "" );
    }
    SUBCASE("../file"){
        REQUIRE( Get_Parent_Directory("../file") == "../" );
    }
    SUBCASE("./file"){
        REQUIRE( Get_Parent_Directory("./file") == "./" );
    }
    SUBCASE("../../../"){
        REQUIRE( Get_Parent_Directory("../../../") == "../../" );
    }
    SUBCASE("./.././"){
        REQUIRE( Get_Parent_Directory("./.././") == "./../" );
    }
}


TEST_CASE( "Generate_Random_String_of_Length" ){

    SUBCASE("returns correct length"){
        const auto s = Generate_Random_String_of_Length(100);
        REQUIRE( s.size() == 100 );
    }

    SUBCASE("returns non-empty string"){
        const auto s = Generate_Random_String_of_Length(1);
        REQUIRE( !s.empty() );
    }

    SUBCASE("zero-length string"){
        const auto s = Generate_Random_String_of_Length(0);
        REQUIRE( s.empty() );
    }

    SUBCASE("different strings are generated"){
        // Generate several strings and verify they are not all identical.
        const size_t N = 20;
        std::string first = Generate_Random_String_of_Length(50);
        bool found_different = false;
        for(size_t i = 1; i < N; ++i){
            if(Generate_Random_String_of_Length(50) != first){
                found_different = true;
                break;
            }
        }
        REQUIRE( found_different );
    }
}

