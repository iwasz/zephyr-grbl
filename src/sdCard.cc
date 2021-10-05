/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "sdCard.h"
#include <disk/disk_access.h>
#include <ff.h>
#include <fs/fs.h>
#include <logging/log.h>

LOG_MODULE_REGISTER (sdCard);

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
static const char *const disk_mount_pt = "/SD:";

namespace sd {

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
                printk ("Sector size %u\n", block_size);

                memory_size_mb = (uint64_t)block_count * block_size;
                printk ("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));
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
}

/**
 *
 */
int lsdir (gsl::czstring path)
{
        int res;
        struct fs_dir_t dirp;
        static struct fs_dirent entry;

        fs_dir_t_init (&dirp);

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

} // namespace sd