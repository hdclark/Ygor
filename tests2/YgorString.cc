
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

