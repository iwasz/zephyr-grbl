#include <TMC2130Stepper.h>
#include <device.h>
#include <devicetree.h>
#include <disk/disk_access.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include <drivers/spi.h>
#include <ff.h>
#include <fs/fs.h>
#include <logging/log.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#define MOTOR1_DIR_NODE DT_PATH (motor1_pins, dir)
#define MOTOR1_DIR_LABEL DT_GPIO_LABEL (MOTOR1_DIR_NODE, gpios)
#define MOTOR1_DIR_PIN DT_GPIO_PIN (MOTOR1_DIR_NODE, gpios)
#define MOTOR1_DIR_FLAGS DT_GPIO_FLAGS (MOTOR1_DIR_NODE, gpios)

#define MOTOR1_STEP_NODE DT_PATH (motor1_pins, step)
#define MOTOR1_STEP_LABEL DT_GPIO_LABEL (MOTOR1_STEP_NODE, gpios)
#define MOTOR1_STEP_PIN DT_GPIO_PIN (MOTOR1_STEP_NODE, gpios)
#define MOTOR1_STEP_FLAGS DT_GPIO_FLAGS (MOTOR1_STEP_NODE, gpios)

#define MOTOR1_ENABLE_NODE DT_PATH (motor1_pins, enable)
#define MOTOR1_ENABLE_LABEL DT_GPIO_LABEL (MOTOR1_ENABLE_NODE, gpios)
#define MOTOR1_ENABLE_PIN DT_GPIO_PIN (MOTOR1_ENABLE_NODE, gpios)
#define MOTOR1_ENABLE_FLAGS DT_GPIO_FLAGS (MOTOR1_ENABLE_NODE, gpios)

#define MOTOR1_NSS_NODE DT_PATH (motor1_pins, nss)
#define MOTOR1_NSS_LABEL DT_GPIO_LABEL (MOTOR1_NSS_NODE, gpios)
#define MOTOR1_NSS_PIN DT_GPIO_PIN (MOTOR1_NSS_NODE, gpios)
#define MOTOR1_NSS_FLAGS DT_GPIO_FLAGS (MOTOR1_NSS_NODE, gpios)

/*--------------------------------------------------------------------------*/

#define MOTOR2_DIR_NODE DT_PATH (motor2_pins, dir)
#define MOTOR2_DIR_LABEL DT_GPIO_LABEL (MOTOR2_DIR_NODE, gpios)
#define MOTOR2_DIR_PIN DT_GPIO_PIN (MOTOR2_DIR_NODE, gpios)
#define MOTOR2_DIR_FLAGS DT_GPIO_FLAGS (MOTOR2_DIR_NODE, gpios)

#define MOTOR2_STEP_NODE DT_PATH (motor2_pins, step)
#define MOTOR2_STEP_LABEL DT_GPIO_LABEL (MOTOR2_STEP_NODE, gpios)
#define MOTOR2_STEP_PIN DT_GPIO_PIN (MOTOR2_STEP_NODE, gpios)
#define MOTOR2_STEP_FLAGS DT_GPIO_FLAGS (MOTOR2_STEP_NODE, gpios)

#define MOTOR2_ENABLE_NODE DT_PATH (motor2_pins, enable)
#define MOTOR2_ENABLE_LABEL DT_GPIO_LABEL (MOTOR2_ENABLE_NODE, gpios)
#define MOTOR2_ENABLE_PIN DT_GPIO_PIN (MOTOR2_ENABLE_NODE, gpios)
#define MOTOR2_ENABLE_FLAGS DT_GPIO_FLAGS (MOTOR2_ENABLE_NODE, gpios)

#define MOTOR2_NSS_NODE DT_PATH (motor2_pins, nss)
#define MOTOR2_NSS_LABEL DT_GPIO_LABEL (MOTOR2_NSS_NODE, gpios)
#define MOTOR2_NSS_PIN DT_GPIO_PIN (MOTOR2_NSS_NODE, gpios)
#define MOTOR2_NSS_FLAGS DT_GPIO_FLAGS (MOTOR2_NSS_NODE, gpios)

/*--------------------------------------------------------------------------*/

#define SWITCH_LEFT_NODE DT_PATH (edge_switch_pins, left)
#define SWITCH_LEFT_LABEL DT_GPIO_LABEL (SWITCH_LEFT_NODE, gpios)
#define SWITCH_LEFT_PIN DT_GPIO_PIN (SWITCH_LEFT_NODE, gpios)
#define SWITCH_LEFT_FLAGS DT_GPIO_FLAGS (SWITCH_LEFT_NODE, gpios)

#define SWITCH_RIGHT_NODE DT_PATH (edge_switch_pins, right)
#define SWITCH_RIGHT_LABEL DT_GPIO_LABEL (SWITCH_RIGHT_NODE, gpios)
#define SWITCH_RIGHT_PIN DT_GPIO_PIN (SWITCH_RIGHT_NODE, gpios)
#define SWITCH_RIGHT_FLAGS DT_GPIO_FLAGS (SWITCH_RIGHT_NODE, gpios)

#define SWITCH_TOP_NODE DT_PATH (edge_switch_pins, top)
#define SWITCH_TOP_LABEL DT_GPIO_LABEL (SWITCH_TOP_NODE, gpios)
#define SWITCH_TOP_PIN DT_GPIO_PIN (SWITCH_TOP_NODE, gpios)
#define SWITCH_TOP_FLAGS DT_GPIO_FLAGS (SWITCH_TOP_NODE, gpios)

#define SWITCH_BOTTOM_NODE DT_PATH (edge_switch_pins, bottom)
#define SWITCH_BOTTOM_LABEL DT_GPIO_LABEL (SWITCH_BOTTOM_NODE, gpios)
#define SWITCH_BOTTOM_PIN DT_GPIO_PIN (SWITCH_BOTTOM_NODE, gpios)
#define SWITCH_BOTTOM_FLAGS DT_GPIO_FLAGS (SWITCH_BOTTOM_NODE, gpios)

#define MOTOR1_STALL_NODE DT_PATH (edge_switch_pins, motor1_stall)
#define MOTOR1_STALL_LABEL DT_GPIO_LABEL (MOTOR1_STALL_NODE, gpios)
#define MOTOR1_STALL_PIN DT_GPIO_PIN (MOTOR1_STALL_NODE, gpios)
#define MOTOR1_STALL_FLAGS DT_GPIO_FLAGS (MOTOR1_STALL_NODE, gpios)

#define MOTOR2_STALL_NODE DT_PATH (edge_switch_pins, motor2_stall)
#define MOTOR2_STALL_LABEL DT_GPIO_LABEL (MOTOR2_STALL_NODE, gpios)
#define MOTOR2_STALL_PIN DT_GPIO_PIN (MOTOR2_STALL_NODE, gpios)
#define MOTOR2_STALL_FLAGS DT_GPIO_FLAGS (MOTOR2_STALL_NODE, gpios)

/*--------------------------------------------------------------------------*/

#define PWM_LED0_NODE DT_ALIAS (pwm_led0)

#if DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)
#define PWM_LABEL DT_PWMS_LABEL (PWM_LED0_NODE)
#define PWM_CHANNEL DT_PWMS_CHANNEL (PWM_LED0_NODE)
#define PWM_FLAGS DT_PWMS_FLAGS (PWM_LED0_NODE)
#endif

/****************************************************************************/

const struct device *leftSwitch{};
const struct device *rightSwitch{};
const struct device *topSwitch{};
const struct device *bottomSwitch{};
const struct device *motor1Stall{};
const struct device *motor2Stall{};

const struct device *dir1{};
const struct device *step1{};
const struct device *dir2{};
const struct device *step2{};

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
        // printk ("Switch pressed at %" PRIu32 ", switch : %u\n", k_cycle_get_32 (), pins);

        if (dev == leftSwitch && pins == BIT (SWITCH_LEFT_PIN)) {
                gpio_pin_set (dir1, MOTOR1_DIR_PIN, false);
                printk ("L\n");
        }
        else if (dev == rightSwitch && pins == BIT (SWITCH_RIGHT_PIN)) {
                gpio_pin_set (dir1, MOTOR1_DIR_PIN, true);
                printk ("R\n");
        }
        else if (dev == topSwitch && pins == BIT (SWITCH_TOP_PIN)) {
                gpio_pin_set (dir2, MOTOR2_DIR_PIN, false);
                printk ("T\n");
        }
        else if (dev == bottomSwitch && pins == BIT (SWITCH_BOTTOM_PIN)) {
                gpio_pin_set (dir2, MOTOR2_DIR_PIN, true);
                printk ("B\n");
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
                printk ("Error opening dir %s [%d]\n", path, res);
                return res;
        }

        printk ("\nListing dir %s ...\n", path);
        for (;;) {
                /* Verify fs_readdir() */
                res = fs_readdir (&dirp, &entry);

                /* entry.name[0] == 0 means end-of-dir */
                if (res || entry.name[0] == 0) {
                        break;
                }

                if (entry.type == FS_DIR_ENTRY_DIR) {
                        printk ("[DIR ] %s\n", entry.name);
                }
                else {
                        printk ("[FILE] %s (size = %zu)\n", entry.name, entry.size);
                }
        }

        /* Verify fs_closedir() */
        fs_closedir (&dirp);

        return res;
}

#define MOTOR1 1
#define MOTOR2 1

#define PERIOD_USEC 100U
// #define NUM_STEPS 50U
// #define STEP_USEC (PERIOD_USEC / NUM_STEPS)
// #define SLEEP_MSEC 25U

void timer_update_callback ()
{
        static bool stepState = true;

#ifdef MOTOR1
        gpio_pin_set (step1, MOTOR1_STEP_PIN, (int)stepState);
#endif

#ifdef MOTOR2
        gpio_pin_set (step2, MOTOR2_STEP_PIN, (int)stepState);
#endif
        stepState = !stepState;
}

// extern "C" int grblMain ();

void main (void)
{

        // grblMain ();

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
                printk ("Sector size %u\n", block_size);

                memory_size_mb = (uint64_t)block_count * block_size;
                printk ("Memory Size(MB) %u\n", (uint32_t) (memory_size_mb >> 20));
        } while (0);

        mp.mnt_point = disk_mount_pt;

        int res = fs_mount (&mp);

        if (res == FR_OK) {
                printk ("Disk mounted.\n");
                lsdir (disk_mount_pt);
        }
        else {
                printk ("Error mounting disk.\n");
        }

        /*--------------------------------------------------------------------------*/

        int ret{};
        const struct device *spi = device_get_binding (DT_LABEL (DT_NODELABEL (spi1)));

        if (!spi) {
                printk ("Could not find SPI driver\n");
                return;
        }

#ifdef MOTOR1
        dir1 = device_get_binding (MOTOR1_DIR_LABEL);

        if (dir1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (dir1, MOTOR1_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_DIR_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        step1 = device_get_binding (MOTOR1_STEP_LABEL);

        if (step1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (step1, MOTOR1_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *enable1 = device_get_binding (MOTOR1_ENABLE_LABEL);

        if (enable1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (enable1, MOTOR1_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *nss1 = device_get_binding (MOTOR1_NSS_LABEL);

        if (nss1 == NULL) {
                return;
        }

        ret = gpio_pin_configure (nss1, MOTOR1_NSS_PIN, GPIO_OUTPUT_ACTIVE | MOTOR1_NSS_FLAGS);

        if (ret < 0) {
                return;
        }

        gpio_pin_set (enable1, MOTOR1_ENABLE_PIN, false);

        const struct spi_cs_control spiCs1 = {.gpio_dev = nss1, .delay = 0, .gpio_pin = MOTOR1_NSS_PIN, .gpio_dt_flags = GPIO_ACTIVE_LOW};
        struct spi_config spiCfg1 = {0};

        spiCfg1.operation = SPI_WORD_SET (8) | SPI_MODE_CPOL | SPI_MODE_CPHA;
        spiCfg1.frequency = 500000U;
        spiCfg1.cs = &spiCs1;

        TMC2130Stepper driver1 (spi, &spiCfg1);

        // driver1.rms_current (600);
        // driver1.stealthChop (1);
        // driver1.microsteps (8);
        driver1.dedge (true);
        driver1.push ();
        driver1.toff (3);
        driver1.tbl (1);
        driver1.chm (true);
        driver1.hysteresis_start (4);
        driver1.hysteresis_end (-2);
        driver1.rms_current (800); // mA
        driver1.microsteps (8);

        driver1.sgt (10);
        driver1.sfilt (false);

        driver1.diag1_stall (true);
        driver1.diag1_active_high (true);

        driver1.coolstep_min_speed (0xFFFFF); // 20bit max
        driver1.THIGH (0);
        driver1.semin (5);
        driver1.semax (2);
        driver1.sedn (0b01);
        driver1.sg_stall_value (0);

        gpio_pin_set (enable1, MOTOR1_ENABLE_PIN, true);
#endif

        /****************************************************************************/

#ifdef MOTOR2
        dir2 = device_get_binding (MOTOR2_DIR_LABEL);

        if (dir2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (dir2, MOTOR2_DIR_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_DIR_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        step2 = device_get_binding (MOTOR2_STEP_LABEL);

        if (step2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (step2, MOTOR2_STEP_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_STEP_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *enable2 = device_get_binding (MOTOR2_ENABLE_LABEL);

        if (enable2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (enable2, MOTOR2_ENABLE_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_ENABLE_FLAGS);

        if (ret < 0) {
                return;
        }

        /*--------------------------------------------------------------------------*/

        const struct device *nss2 = device_get_binding (MOTOR2_NSS_LABEL);

        if (nss2 == NULL) {
                return;
        }

        ret = gpio_pin_configure (nss2, MOTOR2_NSS_PIN, GPIO_OUTPUT_ACTIVE | MOTOR2_NSS_FLAGS);

        if (ret < 0) {
                return;
        }

        gpio_pin_set (enable2, MOTOR2_ENABLE_PIN, false);

        const struct spi_cs_control spiCs2 = {.gpio_dev = nss2, .delay = 0, .gpio_pin = MOTOR2_NSS_PIN, .gpio_dt_flags = GPIO_ACTIVE_LOW};
        struct spi_config spiCfg2 = {0};

        spiCfg2.operation = SPI_WORD_SET (8) | SPI_MODE_CPOL | SPI_MODE_CPHA;
        spiCfg2.frequency = 500000U;
        spiCfg2.cs = &spiCs2;

        TMC2130Stepper driver2 (spi, &spiCfg2);

        // driver2.rms_current (600);
        // driver2.stealthChop (1);
        // driver2.microsteps (8);
        driver2.dedge (true);
        driver2.push ();
        driver2.toff (3);
        driver2.tbl (1);
        driver2.chm (true);
        driver2.hysteresis_start (4);
        driver2.hysteresis_end (-2);
        driver2.rms_current (800); // mA
        driver2.microsteps (8);

        driver2.diag1_stall (1);
        driver2.diag1_active_high (1);
        driver2.coolstep_min_speed (0xFFFFF); // 20bit max
        driver2.THIGH (0);
        driver2.semin (5);
        driver2.semax (2);
        driver2.sedn (0b01);
        driver2.sg_stall_value (0);

        gpio_pin_set (enable2, MOTOR2_ENABLE_PIN, true);
#endif

        /*--------------------------------------------------------------------------*/

        leftSwitch = device_get_binding (SWITCH_LEFT_LABEL);

        if (leftSwitch == NULL) {
                return;
        }

        ret = gpio_pin_configure (leftSwitch, SWITCH_LEFT_PIN, GPIO_INPUT | SWITCH_LEFT_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (leftSwitch, SWITCH_LEFT_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        static struct gpio_callback buttonCbDataLeft;
        gpio_init_callback (&buttonCbDataLeft, switchPressed, BIT (SWITCH_LEFT_PIN));
        gpio_add_callback (leftSwitch, &buttonCbDataLeft);

        /*--------------------------------------------------------------------------*/

        rightSwitch = device_get_binding (SWITCH_RIGHT_LABEL);

        if (rightSwitch == NULL) {
                return;
        }

        ret = gpio_pin_configure (rightSwitch, SWITCH_RIGHT_PIN, GPIO_INPUT | SWITCH_RIGHT_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (rightSwitch, SWITCH_RIGHT_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        static struct gpio_callback buttonCbDataRight;
        gpio_init_callback (&buttonCbDataRight, switchPressed, BIT (SWITCH_RIGHT_PIN));
        gpio_add_callback (rightSwitch, &buttonCbDataRight);

        /*--------------------------------------------------------------------------*/

        topSwitch = device_get_binding (SWITCH_TOP_LABEL);

        if (topSwitch == NULL) {
                return;
        }

        ret = gpio_pin_configure (topSwitch, SWITCH_TOP_PIN, GPIO_INPUT | SWITCH_TOP_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (topSwitch, SWITCH_TOP_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        static struct gpio_callback buttonCbDataTop;
        gpio_init_callback (&buttonCbDataTop, switchPressed, BIT (SWITCH_TOP_PIN));
        gpio_add_callback (topSwitch, &buttonCbDataTop);

        /*--------------------------------------------------------------------------*/

        bottomSwitch = device_get_binding (SWITCH_BOTTOM_LABEL);

        if (bottomSwitch == NULL) {
                return;
        }

        ret = gpio_pin_configure (bottomSwitch, SWITCH_BOTTOM_PIN, GPIO_INPUT | SWITCH_BOTTOM_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (bottomSwitch, SWITCH_BOTTOM_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        static struct gpio_callback buttonCbDataBottom;
        gpio_init_callback (&buttonCbDataBottom, switchPressed, BIT (SWITCH_BOTTOM_PIN));
        gpio_add_callback (bottomSwitch, &buttonCbDataBottom);

        /*--------------------------------------------------------------------------*/

        motor1Stall = device_get_binding (MOTOR1_STALL_LABEL);

        if (motor1Stall == NULL) {
                return;
        }

        ret = gpio_pin_configure (motor1Stall, MOTOR1_STALL_PIN, GPIO_INPUT | MOTOR1_STALL_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (motor1Stall, MOTOR1_STALL_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        static struct gpio_callback motor1StallCbData;
        gpio_init_callback (&motor1StallCbData, switchPressed, BIT (MOTOR1_STALL_PIN));
        gpio_add_callback (motor1Stall, &motor1StallCbData);

        /*--------------------------------------------------------------------------*/

        motor2Stall = device_get_binding (MOTOR2_STALL_LABEL);

        if (motor2Stall == NULL) {
                return;
        }

        ret = gpio_pin_configure (motor2Stall, MOTOR2_STALL_PIN, GPIO_INPUT | MOTOR2_STALL_FLAGS);

        if (ret < 0) {
                return;
        }

        ret = gpio_pin_interrupt_configure (motor2Stall, MOTOR2_STALL_PIN, GPIO_INT_EDGE_TO_ACTIVE);

        if (ret != 0) {
                printk ("Error %d: failed to configure interrupt\n", ret);
                return;
        }

        static struct gpio_callback motor2StallCbData;
        gpio_init_callback (&motor2StallCbData, switchPressed, BIT (MOTOR2_STALL_PIN));
        gpio_add_callback (motor2Stall, &motor2StallCbData);

        /*--------------------------------------------------------------------------*/

        gpio_pin_set (dir1, MOTOR1_DIR_PIN, false);
        gpio_pin_set (dir2, MOTOR2_DIR_PIN, false);

        /*--------------------------------------------------------------------------*/

        const struct device *pwm;
        uint32_t pulse_width = 0U;

        pwm = device_get_binding (PWM_LABEL);

        if (!pwm) {
                printk ("Error: didn't find %s device\n", PWM_LABEL);
                return;
        }

        pwm_set_update_callback (pwm, timer_update_callback);

        ret = pwm_pin_set_usec (pwm, PWM_CHANNEL, PERIOD_USEC, pulse_width, PWM_FLAGS);

        if (ret) {
                printk ("Error %d: failed to set pulse width\n", ret);
                return;
        }

        bool stepState{};

        while (1) {
                // #ifdef MOTOR1
                //                 gpio_pin_set (step1, MOTOR1_STEP_PIN, (int)stepState);
                // #endif

                // #ifdef MOTOR2
                //                 gpio_pin_set (step2, MOTOR2_STEP_PIN, (int)stepState);
                // #endif
                //                 stepState = !stepState;
                k_usleep (100);
        }
}
