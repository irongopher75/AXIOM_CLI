#include "doctest.h"
#include "ui/ui.hpp"
#include <string>

TEST_CASE("column_width handles basic ASCII") {
    CHECK(Axiom::UI::column_width("hello") == 5);
    CHECK(Axiom::UI::column_width("axiom") == 5);
}

TEST_CASE("column_width handles UTF-8 characters") {
    CHECK(Axiom::UI::column_width("hello✓") == 6); 
    CHECK(Axiom::UI::column_width("█") == 1);
}

TEST_CASE("column_width ignores ANSI escape codes") {
    CHECK(Axiom::UI::column_width("\033[31mhello\033[0m") == 5);
    CHECK(Axiom::UI::column_width(Axiom::UI::fg(Axiom::UI::EMERALD) + "test" + Axiom::UI::RESET) == 4);
}

TEST_CASE("column_width handles malformed UTF-8 gracefully without crashing") {
    // A string with a trailing incomplete UTF-8 sequence (e.g. 0xC0)
    std::string malformed = "hello\xC0";
    // Width should be 5 because the malformed byte is skipped in the fallback
    CHECK(Axiom::UI::column_width(malformed) == 5);
}
