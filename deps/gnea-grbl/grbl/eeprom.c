/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "eeprom.h"
#include "grbl.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

LOG_MODULE_REGISTER(eeprom);

/* These EEPROM bits have different names on different devices. */
#ifndef EEPE
		#define EEPE  EEWE  //!< EEPROM program/write enable.
		#define EEMPE EEMWE //!< EEPROM master program/write enable.
#endif

/* These two are unfortunately not defined in the device include files. */
#define EEPM1 5 //!< EEPROM Programming Mode Bit 1.
#define EEPM0 4 //!< EEPROM Programming Mode Bit 0.

/* Define to reduce code size. */
#define EEPROM_IGNORE_SELFPROG //!< Remove SPM flag polling.

static struct nvs_fs fs;

#define STORAGE_NODE DT_NODE_BY_FIXED_PARTITION_LABEL(storage)
#define FLASH_NODE DT_MTD_FROM_FIXED_PARTITION(STORAGE_NODE)
#define ADDRESS_ID 1
#define RBT_CNT_ID 3

void init_nvs ()
{
  return;
        const struct device *flash_dev;
		int rc;

        /* define the nvs file system by settings with:
         *      sector_size equal to the pagesize,
         *      3 sectors
         *      starting at FLASH_AREA_OFFSET(storage)
         */
        flash_dev = DEVICE_DT_GET(FLASH_NODE);

        if (!device_is_ready(flash_dev)) {
                LOG_ERR("Flash device %s is not ready", flash_dev->name);
                return;
        }

        fs.offset = FLASH_AREA_OFFSET(storage);
		struct flash_pages_info info = {0,};

        if ((rc = flash_get_page_info_by_offs(flash_dev, fs.offset, &info))) {
                LOG_ERR("Unable to get page info");
                return;
        }

        fs.sector_size = info.size;
        fs.sector_count = 2U;

        if ((rc = nvs_init(&fs, flash_dev->name))) {
                LOG_ERR("Flash Init failed");
                return;
        }

		//       /* RBT_CNT_ID is used to store the reboot counter, lets see
        //  * if we can read it from flash
        //  */
		//          uint32_t reboot_counter = 0U, reboot_counter_his;

        // rc = nvs_read(&fs, RBT_CNT_ID, &reboot_counter, sizeof(reboot_counter));
        // if (rc > 0) { /* item was found, show it */
        //         printk("Id: %d, Reboot_counter: %d\n",
        //                 RBT_CNT_ID, reboot_counter);
        // } else   {/* item was not found, add it */
        //         printk("No Reboot counter found, adding it at id %d\n",
        //                RBT_CNT_ID);
        //         (void)nvs_write(&fs, RBT_CNT_ID, &reboot_counter,
        //                   sizeof(reboot_counter));
        // }

		// ++reboot_counter;
		// nvs_write(&fs, RBT_CNT_ID, &reboot_counter, sizeof(reboot_counter));




        // /* ADDRESS_ID is used to store an address, lets see if we can
        //  * read it from flash, since we don't know the size read the
        //  * maximum possible
        //  */
		// char buf[256];
        // rc = nvs_read(&fs, ADDRESS_ID, &buf, sizeof(buf));
        // if (rc > 0) { /* item was found, show it */
        //         printk("Id: %d, Address: %s\n", ADDRESS_ID, buf);
        // } else   {/* item was not found, add it */
        //         strcpy(buf, "192.168.1.1");
        //         printk("No address found, adding %s at id %d", buf,
        //                ADDRESS_ID);
        //         (void)nvs_write(&fs, ADDRESS_ID, &buf, strlen(buf)+1);
        // }

		// snprintf (buf, sizeof (buf), "192.168.1.%d-192.168.1.%d-192.168.1.%d-192.168.1.%d", reboot_counter, reboot_counter, reboot_counter, reboot_counter);
        // nvs_write(&fs, ADDRESS_ID, &buf, strlen(buf)+1);

}

/*! \brief  Read byte from EEPROM.
 *
 *  This function reads one byte from a given EEPROM address.
 *
 *  \note  The CPU is halted for 4 clock cycles during EEPROM read.
 *
 *  \param  addr  EEPROM address to read from.
 *  \return  The byte read from the EEPROM address.
 */
unsigned char eeprom_get_char( unsigned int addr )
{
	/*
	do {} while( EECR & (1<<EEPE) ); // Wait for completion of previous write.
	EEAR = addr; // Set EEPROM address register.
	EECR = (1<<EERE); // Start EEPROM read operation.
	return EEDR; // Return the byte read from EEPROM.
	*/
	return 0;
}

/*! \brief  Write byte to EEPROM.
 *
 *  This function writes one byte to a given EEPROM address.
 *  The differences between the existing byte and the new value is used
 *  to select the most efficient EEPROM programming mode.
 *
 *  \note  The CPU is halted for 2 clock cycles during EEPROM programming.
 *
 *  \note  When this function returns, the new EEPROM value is not available
 *         until the EEPROM programming time has passed. The EEPE bit in EECR
 *         should be polled to check whether the programming is finished.
 *
 *  \note  The EEPROM_GetChar() function checks the EEPE bit automatically.
 *
 *  \param  addr  EEPROM address to write to.
 *  \param  new_value  New EEPROM value.
 */
void eeprom_put_char( unsigned int addr, unsigned char new_value )
{
	/*
	char old_value; // Old EEPROM value.
	char diff_mask; // Difference mask, i.e. old value XOR new value.

	cli(); // Ensure atomic operation for the write operation.

	do {} while( EECR & (1<<EEPE) ); // Wait for completion of previous write.
	#ifndef EEPROM_IGNORE_SELFPROG
	do {} while( SPMCSR & (1<<SELFPRGEN) ); // Wait for completion of SPM.
	#endif

	EEAR = addr; // Set EEPROM address register.
	EECR = (1<<EERE); // Start EEPROM read operation.
	old_value = EEDR; // Get old EEPROM value.
	diff_mask = old_value ^ new_value; // Get bit differences.

	// Check if any bits are changed to '1' in the new value.
	if( diff_mask & new_value ) {
		// Now we know that _some_ bits need to be erased to '1'.

		// Check if any bits in the new value are '0'.
		if( new_value != 0xff ) {
			// Now we know that some bits need to be programmed to '0' also.

			EEDR = new_value; // Set EEPROM data register.
			EECR = (1<<EEMPE) | // Set Master Write Enable bit...
			       (0<<EEPM1) | (0<<EEPM0); // ...and Erase+Write mode.
			EECR |= (1<<EEPE);  // Start Erase+Write operation.
		} else {
			// Now we know that all bits should be erased.

			EECR = (1<<EEMPE) | // Set Master Write Enable bit...
			       (1<<EEPM0);  // ...and Erase-only mode.
			EECR |= (1<<EEPE);  // Start Erase-only operation.
		}
	} else {
		// Now we know that _no_ bits need to be erased to '1'.

		// Check if any bits are changed from '1' in the old value.
		if( diff_mask ) {
			// Now we know that _some_ bits need to the programmed to '0'.

			EEDR = new_value;   // Set EEPROM data register.
			EECR = (1<<EEMPE) | // Set Master Write Enable bit...
			       (1<<EEPM1);  // ...and Write-only mode.
			EECR |= (1<<EEPE);  // Start Write-only operation.
		}
	}

	sei(); // Restore interrupt flag state.
	*/
}

// Extensions added as part of Grbl


void memcpy_to_eeprom_with_checksum(unsigned int destination, char *source, unsigned int size) {
	/*
  unsigned char checksum = 0;
  for(; size > 0; size--) {
    checksum = (checksum << 1) || (checksum >> 7);
    checksum += *source;
    eeprom_put_char(destination++, *(source++));
  }
  eeprom_put_char(destination, checksum);
  */
}

int memcpy_from_eeprom_with_checksum(char *destination, unsigned int source, unsigned int size) {
  unsigned char checksum = 0;
  /*
  for(; size > 0; size--) {
    data = eeprom_get_char(source++);
    checksum = (checksum << 1) || (checksum >> 7);
    checksum += data;
    *(destination++) = data;
  }
  */
  return(checksum == eeprom_get_char(source));
}

// end of file
