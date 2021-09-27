/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "grblState.h"
#include "Machine.h"
#include "grbl/protocol.h"
#include "grbl/serial.h"
#include <climits>
#include <cstdlib>
#include <ctre.hpp>
#include <etl/string.h>
#include <gsl/gsl>
#include <logging/log.h>

/*
 * https://github.com/gnea/grbl/issues/822 - good summary of the protocol.
 * https://github.com/wnelis/Grbl-driver-python/blob/master/GrblDriver.py - Python code written by the guy from the above thread.
 *
 * To sum things up:
 * - 3 types of requests : g-code blocks, control commands and real-time commands
 * - 2? types of resonses:
 *   - push commands (async)
 *   - responses to the above requests
 * The so called "streaming protocol" (called this way by the op) is the request-response
 * flow involving the c-command blocks and control commands.
 *
 * empty string, g-code or system command ($x) -> response:
 * ok | error:num
 * ^X : welcome
 * !, ? and ~ are treated somewhat differently, but I can't see how.
 *
 * Push messages:
 *   gdWelcomeMsg,		        # Soft reset completed message
 *   '^ALARM:',				# Alarm message
 *   '^\[MSG:.*\]$',			# Unsolicited feedback message
 *   '^>.*:(?:ok|error:.*)$',		# Start up message ($N)
 *   '^$'				# Empty line
 *
 * "Verbose" messages:
 * <Idle|MPos:0.000,3.000,0.000|F:0|WCO:0.000,0.000,0.000>
 * <Idle|MPos:0.000,3.000,0.000|F:0>
 */

LOG_MODULE_REGISTER (grbl);

using string = etl::string<LINE_BUFFER_SIZE>;

void grblResponseThread (void *, void *, void *);
void grblRequestThread (void *, void *, void *);

constexpr int SD_CARD_STACK_SIZE = 32768;
constexpr int SD_CARD_PRIORITY = 13; // The same as the main thread! Time slicing is ON!

K_THREAD_DEFINE (sdrx, SD_CARD_STACK_SIZE, grblResponseThread, NULL, NULL, NULL, SD_CARD_PRIORITY, 0, 0);
K_MUTEX_DEFINE (machineMutex);

// To make things easier.
static_assert (LINE_BUFFER_SIZE % 4 == 0);

/*
Actions
- Wait for the system to get ready.
  - For instance chceck if peripherals are initialized or something.
- Reset the GRBL. It can be in unknown state. UGS sequence is (I think) this : ^X, $$, $G
- $X : Unlock
- $H : Home
- $N probably those 2 lines that got stored in the EEPROM ?
- $$ : current settings
- $x=val - store a setting?
- $G : supported comands? Noo, too few.
- $# some state. Don't know the meaning.
- $SLP : sleep (^X to wake up)
- $ : help
- $I version
- $J : ?
$C : ?




TODO move to a separate file (differentiate to sd and grbl)
TODO change namespace name to grbl
*/

namespace grbl {
void executeLine (gsl::czstring line);
void executeCommand (char command);

using namespace ls;

enum class Error { none, one, two };
enum class Alarm { none, limit, two };
enum class StateName { idle, alarm, hold };

/**
 * Tracks the state of the GRBL. We probably could reach out to the GRBL itself since it
 * is runninng alongside in another thread, but I feel that communicating wit it the normal
 * way is the right thing to do. Everythong stays separated.
 */
struct ReceivedState {
        Error error{};
        Alarm alarm{};
        StateName stateName{};
};

ReceivedState receivedState;

struct RequestedState {
        JogDirection jogDirection{};
};

RequestedState requestedState;

/**
 * State machine conditions.
 */

/// Gcode and status response:
constexpr auto gCodeResponse = [] (auto const &s) { return ctre::match<"^(?:ok|error:(\\d*))(\r\n)?$"> (s); };
constexpr auto ok = [] (auto const &s) { return s == "ok\r\n"; };
/// Push messages:
// constexpr auto welcomeMessage = [] (auto const &s) { return ctre::match<"^Grbl.+\\](\r\n)?$"> (s); };
// constexpr auto welcomeMessage = [] (auto const &s) { return ctre::match<"^Grbl.+\\](\r\n)?$"> ("Grbl 1.1h ['$' for help]"); };
constexpr auto welcomeMessage = [] (auto const &s) { return s == "Grbl 1.1h ['$' for help]\r\n"; };
// auto welcomeMessage = [] { return [] (auto const &s) { return ctre::match<"^Grbl.+\\](\r\n)?$"> (s); }; };
constexpr auto settingMessage = [] (auto const &s) { return ctre::match<"^\\$\\d+=.*(\r\n)?"> (s); };
constexpr auto msgMessage = [] (auto const &s) { return ctre::match<"^\\[MSG:.*\\](\r\n)?$"> (s); };
constexpr auto lockedMessage = [] (auto const &s) { return ctre::match<"^\\[MSG:'\\$H'\\|'\\$X' to unlock\\](\r\n)?$"> (s); };
constexpr auto unlockedMessage = [] (auto const &s) { return ctre::match<"^\\[MSG:Caution: Unlocked\\](\r\n)?$"> (s); };
constexpr auto alarmMessage = [] (auto const &s) { return ctre::match<"^ALARM:(\\d*)(\r\n)?$"> (s); };
constexpr auto errorMessage = [] (auto const &s) { return ctre::match<"^error:(\\d*)(\r\n)?$"> (s); };
/// Verbose (responses to the "?")
constexpr auto verboseMessage = [] (auto const &s) { return ctre::match<"^<([a-zA-Z]*)\\|MPos:(.*),(.*),(.*)\\|F:(\\d*).*(\r\n)?$"> (s); };
constexpr auto idleVerbose = [] (auto const &s) { return ctre::match<"^<Idle.*(\r\n)?$"> (s); };
constexpr auto jogVerbose = [] (auto const &s) { return ctre::match<"^<Jog.*(\r\n)?$"> (s); };
constexpr auto alarmVerbose = [] (auto const &s) { return ctre::match<"^<Alarm.*(\r\n)?$"> (s); };

constexpr char CMD_RESET = 0x18;
constexpr char CMD_STATUS_REPORT = '?';
constexpr char CMD_CYCLE_START = '~';
constexpr char CMD_FEED_HOLD = '!';

/**
 * Tracks GRBL's state. A minimal subset of what the UGS can do.
 */
auto grblMachine
        = machine (state ("init"_ST, entry ([] {
                                  printk ("# init\r\n");
                                  executeCommand (CMD_RESET);
                          }),
                          transition ("ready"_ST, welcomeMessage)),

                   state ("ready"_ST, entry ([] () { printk ("# ready\r\n"); }),
                          transition ("locked"_ST, lockedMessage),                           // GRBL locked itself. Immediately unlock.
                          transition ("jog"_ST, [] (string const &s) { return s == "jog"; }) // User action jog along Y in the positive dir.
                          ),

                   state ("locked"_ST, entry ([] (auto) { // Automatic unlocking state.
                                  printk ("# locked\r\n");
                                  executeLine ("$X\r\n"); // Send command to the GRBL.
                          }),
                          transition ("ready"_ST, unlockedMessage)), // Transition back to ready upon receiving a response.

                   state ("jog"_ST, entry ([rs = requestedState.jogDirection] (auto) {
                                  printk ("# jog\r\n");

                                  switch (rs) {
                                  case JogDirection::xPositive:
                                          grbl::executeLine ("$J=G21G91Y10F500\r\n");
                                          break;

                                  case JogDirection::xNegative:
                                          grbl::executeLine ("$J=G21G91Y-10F500\r\n");
                                          break;

                                  case JogDirection::yPositive:
                                          grbl::executeLine ("$J=G21G91Y10F500\r\n");
                                          break;

                                  case JogDirection::yNegative:
                                          grbl::executeLine ("$J=G21G91Y-10F500\r\n");
                                          break;

                                  default:
                                          break;
                                  }
                          }),
                          transition ("waitidle"_ST, ok)),

                   state ("waitidle"_ST, entry ([] {
                                  printk ("# wi\r\n");
                                  executeCommand (CMD_STATUS_REPORT);
                                  k_sleep (K_MSEC (250));
                          }),
                          transition ("ready"_ST, idleVerbose), transition ("alarm"_ST, alarmVerbose),
                          transition ("waitidle"_ST, [] (auto) { return true; })),

                   state ("alarm"_ST, entry ([] { printk ("# ALARM"); }), transition ("ready"_ST, [] (string const &s) { return s == ""; })),
                   state ("waitrsp"_ST, entry ([] () {}), transition ("ready"_ST, [] (string const &s) { return s == ""; })));

/**
 *
 */
void jog (JogDirection dir)
{
        k_mutex_lock (&machineMutex, K_FOREVER);
        requestedState.jogDirection = dir;
        grblMachine.run (string ("jog"));
        k_mutex_unlock (&machineMutex);
}

/**
 * Get and return next response if there is some in the ring buffer.
 */
string response ()
{
        string rsp;
        rsp.resize (rsp.max_size ());
        auto ret = serial_get_tx_line (reinterpret_cast<uint32_t *> (rsp.data ()), rsp.size () / 4);

        if (ret < 0) {
                printk ("rsp buffer error");
                return {};
        }

        rsp.resize (ret);
        return rsp;
}

} // namespace grbl

/**
 *
 */
void grblResponseThread (void *, void *, void *)
{
        while (true) {
                k_sleep (K_MSEC (10));

                string rsp = grbl::response ();

                if (!rsp.empty ()) {
                        printk ("< %s", rsp.c_str ()); // Formatted synchronously, so no worries aboyt rsp.
                }

                k_mutex_lock (&machineMutex, K_FOREVER);
                grbl::grblMachine.run (rsp);
                k_mutex_unlock (&machineMutex);
        }
}

/**
 *
 */
void grblRequestThread (void *, void *, void *)
{
        while (true) {
                grbl::executeLine ("G0 X-0.5 Y0.\r\n");
                k_sleep (K_SECONDS (3));
                grbl::executeLine ("G0 X0. Y0.\r\n");
                k_sleep (K_SECONDS (3));
        }
}

namespace grbl {

/**
 *
 */
void executeLine (gsl::czstring line)
{
        if (!serial_is_initialized ()) {
                LOG_WRN ("Serial subsystem is required for SD card to operate.");
                return;
        }

        serial_disable_irqs ();
        printk ("> %s", line); // Formatted synchronously, so no worries aboyt rsp.
        serial_buffer_append (line);
        serial_enable_irqs ();
}

/**
 *
 */
void executeCommand (char command)
{
        if (!serial_is_initialized ()) {
                LOG_WRN ("Serial subsystem is required for SD card to operate.");
                return;
        }

        serial_disable_irqs ();
        serial_check_real_time_command (command);
        serial_enable_irqs ();
}

} // namespace grbl