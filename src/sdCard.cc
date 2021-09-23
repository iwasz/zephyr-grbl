/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "sdCard.h"
#include "grbl/protocol.h"
#include "grbl/serial.h"
#include <climits>
#include <cstdlib>
#include <disk/disk_access.h>
#include <etl/string.h>
#include <ff.h>
#include <fs/fs.h>
#include <logging/log.h>

/*
 * https://github.com/gnea/grbl/issues/822 - good summary of the protocol.
 *
 * To sum things up:
 * - 3 types of requests : g-code blocks, control commands and real-time commands
 * - 2? types of resonses:
 *   - push commands (async)
 *   - responses to the above requests
 * The so called "streaming protocol" (called this way by the op) is the request-response
 * flow involving the c-command blocks and control commands.
 */

LOG_MODULE_REGISTER (sdCard);

namespace {

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
static const char *disk_mount_pt = "/SD:";

void grblResponseThread (void *, void *, void *);
void grblRequestThread (void *, void *, void *);

constexpr int SD_CARD_STACK_SIZE = 1024;
constexpr int SD_CARD_PRIORITY = 13; // The same as the main thread! Time slicing is ON!

K_THREAD_DEFINE (sdrx, SD_CARD_STACK_SIZE, grblResponseThread, NULL, NULL, NULL, SD_CARD_PRIORITY, 0, 0);
// K_THREAD_DEFINE (sdtx, SD_CARD_STACK_SIZE, grblRequestThread, NULL, NULL, NULL, SD_CARD_PRIORITY, 0, 0);

// To make things easier.
static_assert (LINE_BUFFER_SIZE % 4 == 0);

/**
 *
 */
void grblResponseThread (void *, void *, void *)
{
        while (true) {
                k_sleep (K_MSEC (10));

                string rsp;
                rsp.resize (rsp.max_size ());
                auto ret = serial_get_tx_line (reinterpret_cast<uint32_t *> (rsp.data ()), rsp.size () / 4);

                if (ret < 0) {
                        printk ("rsp buffer error");
                        continue;
                }

                rsp.resize (ret);

                if (!rsp.empty ()) {
                        printk ("<%s", rsp.c_str ());
                }
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
} // namespace

namespace sd {

/**
 *
 */
void command (const char *gCommand)
{
        if (!serial_is_initialized ()) {
                LOG_WRN ("Serial subsystem is required for SD card to operate.");
                return;
        }

        serial_buffer_append (gCommand);
}

/**
 * Waits until gets some.
 */
// string response ()
// {
//         // while (true) {
//         string rsp;
//         rsp.resize (rsp.max_size ());
//         auto ret = serial_get_tx_line (reinterpret_cast<uint32_t *> (rsp.data ()), rsp.size () / 4);

//         if (ret < 0) {
//                 return {};
//         }

//         if (ret == 0) {
//                 // k_sleep (K_MSEC (10));
//                 // continue;
//                 return {};
//         }

//         rsp.resize (ret);
//         return rsp;
//         // }
// }

/**
 *
 */
void executeLine (const char *line)
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
        printk (">%s", line);
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

/**
 *
 */
void init ()
{
        /* raw disk i/o */
        do {
                static const char *disk_pdrv = "SD";
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
int lsdir (const char *path)
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