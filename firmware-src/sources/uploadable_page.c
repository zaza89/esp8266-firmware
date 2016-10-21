/*
 * rest.c
 *
 * Copyright 2016 DeviceHive
 *
 * Author: Nikolay Khabarov
 *
 */

#include <c_types.h>
#include <spi_flash.h>
#include <irom.h>
#include <osapi.h>
#include <os_type.h>
#include <mem.h>
#include "uploadable_page.h"
#include "dhdebug.h"

/** Uploadable web page maximum length and ROM addresses. */
#define UPLOADABLE_PAGE_START_SECTOR 0x68
#define UPLOADABLE_PAGE_END_SECTOR 0x78
#define UPLOADABLE_PAGE_MAX_SIZE ((UPLOADABLE_PAGE_END_SECTOR - UPLOADABLE_PAGE_START_SECTOR) * SPI_FLASH_SEC_SIZE)
#define UPLOADABLE_PAGE_TIMEOUT_MS 60000
#define SPI_FLASH_BASE_ADDRESS 0x40200000

LOCAL unsigned int mPageLength = 0;
LOCAL unsigned int mFlashingSector = 0;
LOCAL os_timer_t mFlashingTimer;

LOCAL void ICACHE_FLASH_ATTR flash_timeout(void *arg) {
	uploadable_page_finish();
	dhdebug("Flashing procedure isn't finished correctly");
}

LOCAL void ICACHE_FLASH_ATTR reset_timer() {
	os_timer_disarm(&mFlashingTimer);
	os_timer_setfn(&mFlashingTimer, (os_timer_func_t *)flash_timeout, NULL);
	os_timer_arm(&mFlashingTimer, 60000, 0);
}

const char *ICACHE_FLASH_ATTR uploadable_page_get(unsigned int *len) {
	RO_DATA char flashing[] = "<html><body><h1>Flashing is in process...</h1></body></html>";
	const void *data = (void *)(SPI_FLASH_BASE_ADDRESS + UPLOADABLE_PAGE_START_SECTOR * SPI_FLASH_SEC_SIZE);
	if(mFlashingSector) {
		*len = sizeof(flashing) - 1;
		return flashing;
	}
	if(mPageLength == 0) {
		const uint32_t *dwdata = (const uint32_t*)data;
		unsigned int i;
		for(i = 0; i < UPLOADABLE_PAGE_MAX_SIZE; i += sizeof(uint32_t)) {
			uint32_t b = dwdata[i / sizeof(uint32_t)];
			if((b & 0x000000FF) == 0) {
				break;
			} else if((b & 0x0000FF00) == 0) {
				i++;
				break;
			} else if((b & 0x00FF0000) == 0) {
				i += 2;
				break;
			} else if((b & 0xFF000000) == 0) {
				i += 3;
				break;
			}
		}
		mPageLength = i;
	}
	*len = mPageLength;
	return (const char *)data;
}

int ICACHE_FLASH_ATTR uploadable_page_delete() {
	// set first byte to zero
	SpiFlashOpResult res;
	uint8_t *data = (uint8_t *)os_malloc(SPI_FLASH_SEC_SIZE);
	res = spi_flash_read(UPLOADABLE_PAGE_START_SECTOR * SPI_FLASH_SEC_SIZE, (uint32 *)data, SPI_FLASH_SEC_SIZE);
	if (res == SPI_FLASH_RESULT_OK) {
		data[0] = 0;
		// no need to erase data, since here writes just zero byte
		res = spi_flash_write(UPLOADABLE_PAGE_START_SECTOR * SPI_FLASH_SEC_SIZE, (uint32 *)data, SPI_FLASH_SEC_SIZE);
	}
	os_free(data);
	return (res == SPI_FLASH_RESULT_OK) ? 1 : 0;
}

int ICACHE_FLASH_ATTR uploadable_page_begin() {
	mFlashingSector = UPLOADABLE_PAGE_START_SECTOR;
	reset_timer();
}

int ICACHE_FLASH_ATTR uploadable_page_put(const char *data, unsigned int data_len) {
	if(mFlashingSector == 0)
		return 0;
	reset_timer();
	// TODO everything
	return 0;
}

int ICACHE_FLASH_ATTR uploadable_page_finish() {
	os_timer_disarm(&mFlashingTimer);
	mFlashingSector = 0;
	// TODO writing tail
	return 1;
}
