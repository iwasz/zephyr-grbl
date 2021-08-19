/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "hw_timer.h"
#include "mcu-peripherals.h"
#include "stepper-drivers.h"
#include <disk/disk_access.h>
#include <ff.h>
#include <fs/fs.h>
#include <logging/log.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include <drivers/spi.h>

extern "C" int grblMain ();

/****************************************************************************/

LOG_MODULE_REGISTER (main);

static int lsdir (const char *path);

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
        .type = FS_FATFS,
        .fs_data = &fat_fs,
};

/*
 *  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
 *  in ffconf.h
 */
static const char *disk_mount_pt = "/SD:";

/****************************************************************************/

void switchPressed (const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
        if (dev == leftSwitch && pins == BIT (SWITCH_LEFT_PIN)) {
                gpio_pin_set (dirX, MOTORX_DIR_PIN, false);
                LOG_DBG ("L\n");
        }
        else if (dev == rightSwitch && pins == BIT (SWITCH_RIGHT_PIN)) {
                gpio_pin_set (dirX, MOTORX_DIR_PIN, true);
                LOG_DBG ("R\n");
        }
        else if (dev == topSwitch && pins == BIT (SWITCH_TOP_PIN)) {
                gpio_pin_set (dirY, MOTORY_DIR_PIN, false);
                LOG_DBG ("T\n");
        }
        else if (dev == bottomSwitch && pins == BIT (SWITCH_BOTTOM_PIN)) {
                gpio_pin_set (dirY, MOTORY_DIR_PIN, true);
                LOG_DBG ("B\n");
        }
}

/****************************************************************************/

static int lsdir (const char *path)
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

#define PERIOD_USEC 500U
// #define NUM_STEPS 50U
// #define STEP_USEC (PERIOD_USEC / NUM_STEPS)
// #define SLEEP_MSEC 25U

void timer_update_callback ()
{
        static bool stepState = true;
        gpio_pin_set (stepX, MOTORX_STEP_PIN, (int)stepState);
        gpio_pin_set (stepY, MOTORY_STEP_PIN, (int)stepState);
        stepState = !stepState;
}

// extern "C" int grblMain ();

const struct device *dev;

// static void user_entry (void *p1, void *p2, void *p3) { hw_timer_print (dev); }

void main (void)
{
        // printk ("Hello World from the app!\n");

        // dev = device_get_binding ("CUSTOM_DRIVER");

        // __ASSERT (dev, "Failed to get device binding");

        // printk ("device is %p, name is %s\n", dev, dev->name);

        // k_object_access_grant (dev, k_current_get ());
        // k_thread_user_mode_enter (user_entry, NULL, NULL, NULL);

        // return;

        /*--------------------------------------------------------------------------*/

        mcu_peripherals_init ();
        drv::init ();

#if 0
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
                LOG_INF ("Memory Size(MB) %u\n", (uint32_t) (memory_size_mb >> 20));
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
#endif

        /*--------------------------------------------------------------------------*/

        static struct gpio_callback buttonCbDataLeft;
        gpio_init_callback (&buttonCbDataLeft, switchPressed, BIT (SWITCH_LEFT_PIN));
        gpio_add_callback (leftSwitch, &buttonCbDataLeft);

        static struct gpio_callback buttonCbDataRight;
        gpio_init_callback (&buttonCbDataRight, switchPressed, BIT (SWITCH_RIGHT_PIN));
        gpio_add_callback (rightSwitch, &buttonCbDataRight);

        static struct gpio_callback buttonCbDataTop;
        gpio_init_callback (&buttonCbDataTop, switchPressed, BIT (SWITCH_TOP_PIN));
        gpio_add_callback (topSwitch, &buttonCbDataTop);

        static struct gpio_callback buttonCbDataBottom;
        gpio_init_callback (&buttonCbDataBottom, switchPressed, BIT (SWITCH_BOTTOM_PIN));
        gpio_add_callback (bottomSwitch, &buttonCbDataBottom);

        static struct gpio_callback motor1StallCbData;
        // gpio_init_callback (&motor1StallCbData, switchPressed, BIT (MOTORX_STALL_PIN));
        // gpio_add_callback (motor1Stall, &motor1StallCbData);

        static struct gpio_callback motor2StallCbData;
        // gpio_init_callback (&motor2StallCbData, switchPressed, BIT (MOTORY_STALL_PIN));
        // gpio_add_callback (motor2Stall, &motor2StallCbData);

        /*--------------------------------------------------------------------------*/

        // gpio_pin_set (dirX, MOTOR1_DIR_PIN, false);
        // gpio_pin_set (dirY, MOTOR2_DIR_PIN, false);

        /*--------------------------------------------------------------------------*/

        // pwm_set_update_callback (pwm, timer_update_callback);

        // int ret = pwm_pin_set_usec (pwm, PWM_CHANNEL, PERIOD_USEC, 0, PWM_FLAGS);

        // if (ret) {
        //         LOG_ERR ("Error %d: failed to set pulse width\n", ret);
        //         return;
        // }

        grblMain ();

        // bool stepState{};

        // while (1) {
        //         // #ifdef MOTOR1
        //         //                 gpio_pin_set (stepX, MOTOR1_STEP_PIN, (int)stepState);
        //         // #endif

        //         // #ifdef MOTOR2
        //         //                 gpio_pin_set (stepY, MOTOR2_STEP_PIN, (int)stepState);
        //         // #endif
        //         //                 stepState = !stepState;
        //         k_usleep (100);
        // }
}
