/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "sdCard.h"
#include "Machine.h"
#include "grbl/protocol.h"
#include "grbl/serial.h"
#include <climits>
#include <cstdlib>
#include <ctre.hpp>
#include <disk/disk_access.h>
#include <etl/string.h>
#include <ff.h>
#include <fs/fs.h>
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

LOG_MODULE_REGISTER (sdCard);

static FATFS fat_fs;

/* mounting info */
static struct fs_mount_t mp = {
        .type = FS_FATFS,
        .fs_data = &fat_fs,
};

using string = etl::string<LINE_BUFFER_SIZE>;

/*
 *  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
 *  in ffconf.h
 */
static const char *const disk_mount_pt = "/SD:";

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

namespace sd {
void executeLine (gsl::czstring line);
void executeCommand (char command);

using namespace ls;

enum class Error { none, one, two };
enum class Alarm { limit, two };
enum class StateName { idle, alarm, hold };

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
                          transition ("locked"_ST, lockedMessage),                             // GRBL locked itself. Immediately unlock.
                          transition ("jogYP"_ST, [] (string const &s) { return s == "yp"; }), // User action jog along Y in the positive dir.
                          transition ("jogYN"_ST, [] (string const &s) { return s == "yn"; })  // User action jog along Y in the positive dir.
                          ),

                   state ("locked"_ST, entry ([] (auto) { // Automatic unlocking state.
                                  printk ("# locked\r\n");
                                  executeLine ("$X\r\n"); // Send command to the GRBL.
                          }),
                          transition ("ready"_ST, unlockedMessage)), // Transition back to ready upon receiving a response.

                   state ("jogYP"_ST, entry ([] (auto) {
                                  printk ("# yp\r\n");
                                  sd::executeLine ("$J=G21G91Y10F500\r\n");
                          }),
                          transition ("waitidle"_ST, ok)),

                   state ("jogYN"_ST, entry ([] (auto) {
                                  printk ("# yn\r\n");
                                  sd::executeLine ("$J=G21G91Y-10F500\r\n");
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

void jogYP ()
{
        k_mutex_lock (&machineMutex, K_FOREVER);
        grblMachine.run (string ("yp"));
        k_mutex_unlock (&machineMutex);
}

void jogYN ()
{
        k_mutex_lock (&machineMutex, K_FOREVER);
        grblMachine.run (string ("yn"));
        k_mutex_unlock (&machineMutex);
}

/**
 * Tracks the state of the GRBL. We probably could reach out to the GRBL itself since it
 * is runninng alongside in another thread, but I feel that communicating wit it the normal
 * way is the right thing to do. Everythong stays separated.
 */
class State {
public:
private:
        Error error;
        Alarm alarm;
        StateName stateName;
};

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

} // namespace sd

/**
 *
 */
void grblResponseThread (void *, void *, void *)
{
        while (true) {
                k_sleep (K_MSEC (10));

                string rsp = sd::response ();

                if (!rsp.empty ()) {
                        printk ("< %s", rsp.c_str ()); // Formatted synchronously, so no worries aboyt rsp.
                }

                k_mutex_lock (&machineMutex, K_FOREVER);
                sd::grblMachine.run (rsp);
                k_mutex_unlock (&machineMutex);
        }
}

/**
 *
 */
void grblRequestThread (void *, void *, void *)
{
        while (true) {
                sd::executeLine ("G0 X-0.5 Y0.\r\n");
                k_sleep (K_SECONDS (3));
                sd::executeLine ("G0 X0. Y0.\r\n");
                k_sleep (K_SECONDS (3));
        }
}

namespace sd {

/**
 *
 */
void command (gsl::czstring gCommand)
{
        if (!serial_is_initialized ()) { // TODO remove
                LOG_WRN ("Serial subsystem is required for SD card to operate.");
                return;
        }

        printk ("> %s", gCommand); // Formatted synchronously, so no worries aboyt rsp.
        serial_buffer_append (gCommand);
}

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
        // serial_reset_read_buffer ();
        // serial_reset_transmit_buffer ();

        /*
         * Now we are sure the ring buffers are empty. UART IRQs are off, and the ring
         * buffers has been cleared. The only source that can now insert new data into
         * thise buffer is the SD card API.
         */
        command (line);
        // printk (" %s", line);
        // // return;

        // /*
        //  * Possible responses:
        //  * - ok\r\n
        //  * - error:dec\r\n
        //  */

        // static string rsp /* = response () */; // Static for logging which runs in different thread.

        // // Responses are sent tgrough the GRBL uart port.

        // // if (rsp.find ("error:") == 0 && rsp.size () > 6) {
        // //         // LOG_WRN ("GRBL eror:%d", atoi (rsp.c_str () + 6));
        // //         // LOG_WRN ("GRBL eror:%d", atoi (rsp.c_str () + 6));
        // // }
        // // else if (!rsp.empty () && rsp != "ok\r\n") {
        // //         LOG_WRN ("Unknown GRBL resp");
        // // }

        // Make sure the file has been executed entirely.
        // serial_reset_read_buffer ();
        // serial_reset_transmit_buffer ();
        serial_enable_irqs ();
}

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

/**
 *
 */
void init ()
{
        /* raw disk i/o */
        do {
                static gsl::czstring disk_pdrv = "SD";
                uint64_t memory_size_mb;
                uint32_t block_count;
                uint32_t block_size;

                if (disk_access_init (disk_pdrv) != 0) {
                        LOG_ERR ("Storage init ERROR!");
                        break;
                }

                if (disk_access_ioctl (disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
                        LOG_ERR ("Unable to get sector count");
                        break;
                }
                LOG_INF ("Block count %u", block_count);

                if (disk_access_ioctl (disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
                        LOG_ERR ("Unable to get sector size");
                        break;
                }
                LOG_INF ("Sector size %u\n", block_size);

                memory_size_mb = (uint64_t)block_count * block_size;
                LOG_INF ("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));
        } while (0);

        mp.mnt_point = disk_mount_pt;

        int res = fs_mount (&mp);

        if (res == FR_OK) {
                LOG_INF ("Disk mounted.\n");
                lsdir (disk_mount_pt);
        }
        else {
                LOG_ERR ("Error mounting disk.\n");
        }
}

/**
 *
 */
int lsdir (gsl::czstring path)
{
        int res;
        struct fs_dir_t dirp;
        static struct fs_dirent entry;

        /* Verify fs_opendir() */
        res = fs_opendir (&dirp, path);
        if (res) {
                LOG_ERR ("Error opening dir %s [%d]\n", path, res);
                return res;
        }

        LOG_INF ("\nListing dir %s ...\n", path);

        for (;;) {
                /* Verify fs_readdir() */
                res = fs_readdir (&dirp, &entry);

                /* entry.name[0] == 0 means end-of-dir */
                if (res || entry.name[0] == 0) {
                        break;
                }

                if (entry.type == FS_DIR_ENTRY_DIR) {
                        LOG_INF ("[DIR ] %s\n", entry.name);
                }
                else {
                        LOG_INF ("[FILE] %s (size = %zu)\n", entry.name, entry.size);
                }
        }

        /* Verify fs_closedir() */
        fs_closedir (&dirp);

        return res;
}

} // namespace sd