/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <ctre.hpp>
#include <optional>
#include <string_view>

TEST_CASE ("Gcode response", "[regexps]")
{
        auto gCodeResponse = ctre::match<"^(?:ok|error:(\\d*))(\r\n)?$">;

        SECTION ("PLaying with ctre")
        {
                static_assert (gCodeResponse ("ok") == true);
                static_assert (gCodeResponse ("ok\r\n") == true);
                static_assert (gCodeResponse ("error:0") == true);
                static_assert (gCodeResponse ("error:8742360") == true);
                static_assert (gCodeResponse ("error:") == true);
                static_assert (gCodeResponse ("error") == false);

                REQUIRE (gCodeResponse ("ok") == true);
                REQUIRE (gCodeResponse ("error:1") == true);
        }

        SECTION ("Error with code")
        {
                auto m = gCodeResponse ("error:2");
                REQUIRE (m.get<1> () == true);
                REQUIRE (m.get<1> ().to_number () == 2);
        }

        SECTION ("OK response")
        {
                auto m = gCodeResponse ("ok");
                REQUIRE (m.get<1> () == false);
        }
}

TEST_CASE ("Push message", "[regexps]")
{
        auto welcomeMessage = ctre::match<"^Grbl.+\\](\r\n)?$">;
        static_assert (welcomeMessage ("Grbl 1.1h ['$' for help]"));
        static_assert (welcomeMessage ("Grbl 1.1h ['$' for help]\r\n"));
        static_assert (welcomeMessage ("$0=10") == false);

        auto settingMessage = ctre::match<"^\\$\\d+=.*(\r\n)?">;
        static_assert (settingMessage ("$0=10"));
        static_assert (settingMessage ("$25=1000.000\r\n"));

        auto msgMessage = ctre::match<"^\\[MSG:.*\\](\r\n)?$">;
        static_assert (msgMessage ("[MSG:'$H'|'$X' to unlock]"));
        static_assert (msgMessage ("[MSG:Reset to continue]"));
        static_assert (msgMessage ("[MSG:Caution: Unlocked]\r\n"));

        auto alarmMessage = ctre::match<"^ALARM:(\\d*)(\r\n)?$">;
        static_assert (alarmMessage ("ALARM:1"));
        static_assert (alarmMessage ("ALARM:1").get<1> ());
        REQUIRE (alarmMessage ("ALARM:6").get<1> ().to_number () == 6);

        // At startup, for instance error:7 means there are problems with EEPROM settings.
        auto errorMessage = ctre::match<"^error:(\\d*)(\r\n)?$">;
}

// <Idle|MPos:0.000,0.000,0.000|F:0|WCO:0.000,0.000,0.000>
// <Idle|MPos:0.000,0.000,0.000|F:0|Ov:100,100,100>
// <Idle|MPos:0.000,0.000,0.000|F:0>
TEST_CASE ("Verbose message", "[regexps]")
{
        auto verboseMessage = ctre::match<"^<([a-zA-Z]*)\\|MPos:(.*),(.*),(.*)\\|F:(\\d*).*(\r\n)?$">;

        static_assert (verboseMessage ("<Idle|MPos:0.000,0.000,0.000|F:0>"));
        static_assert (!verboseMessage ("<999|MPos:0.000,0.000,0.000|F:0>"));
        static_assert (!verboseMessage ("<999|MPos:0.000,0.000|F:0>"));
        static_assert (verboseMessage ("<Idle|MPos:0.000,0.000,0.000|F:0|WCO:0.000,0.000,0.000>"));

        auto [whole, status, x, y, z, f, crlf] = verboseMessage ("<Idle|MPos:0.000,0.000,0.000|F:0|WCO:0.000,0.000,0.000>");
        REQUIRE (status == "Idle");
}
