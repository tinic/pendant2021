/*
Copyright 2021 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "./sdcard.h"
#include "./emfat.h"
#include "./main.h"
#include "./timeline.h"

#include "M480.h"

extern "C" {
#include "./msc.h"
}

#include <functional>

enum {
    CMD0    = 0x40 + 0,  // GO_IDLE_STATE
    CMD1    = 0x40 + 1,  // SEND_OP_COND (MMC) 
    ACMD41  = 0xC0 + 41, // SEND_OP_COND (SDC)
    CMD8    = 0x40 + 8,  // SEND_IF_COND
    CMD9    = 0x40 + 9,  // SEND_CSD
    CMD10   = 0x40 + 10, // SEND_CID
    CMD16   = 0x40 + 16, // SET_BLOCKLEN
    CMD17   = 0x40 + 17, // READ_SINGLE_BLOCK
    CMD24   = 0x40 + 24, // WRITE_SINGLE_BLOCK
    CMD42   = 0x40 + 42, // LOCK_UNLOCK
    CMD55   = 0x40 + 55, // APP_CMD
    CMD58   = 0x40 + 58, // READ_OCR
    CMD59   = 0x40 + 59, // CRC_ON_OFF
    CMD
};

SDCard &SDCard::instance() {
    static SDCard sdcard;
    if (!sdcard.initialized) {
        sdcard.initialized = true;
        sdcard.init();
    }
    return sdcard;
}

void SDCard::process() {
}

uint8_t SDCard::readByte() {
    QSPI_WRITE_TX(QSPI0, 0xFF);
    while(QSPI_IS_BUSY(QSPI0));
    return QSPI_READ_RX(QSPI0);
}

void SDCard::writeByte(uint8_t byte) {
    QSPI_WRITE_TX(QSPI0, byte);
    while(QSPI_IS_BUSY(QSPI0));
    QSPI_READ_RX(QSPI0);
}

std::tuple<bool, uint8_t> SDCard::GoIdle() { 
    QSPI_SET_SS_LOW(QSPI0);
    readByte();

    writeByte(CMD0);
    writeByte(0); 
    writeByte(0);
    writeByte(0);
    writeByte(0);
    writeByte(0x95); // Ncr

    while(QSPI_IS_BUSY(QSPI0));

    uint8_t res = 0;
    for (int32_t c = 0; c < 8; c++)
    {
        res = readByte();
        if (res != 0xFF) {
            return {true, res};
        }
    }

    return {false, 0};
}

std::tuple<bool, uint8_t> SDCard::CheckVoltage() { 

    QSPI_SET_SS_LOW(QSPI0);
    readByte();

    writeByte(CMD8);
    writeByte(0);
    writeByte(0);
    writeByte(0x01);
    writeByte(0xAA);
    writeByte(0x87); // Ncr

    while(QSPI_IS_BUSY(QSPI0));

    uint8_t res = 0;
    for (int32_t c = 0; c < 8; c++)
    {
        res = readByte();
        if (res != 0xFF) {
            return {true, res};
        }
    }

    return {false, 0};
}

std::tuple<bool, uint8_t> SDCard::SendCmd(uint8_t cmd, uint32_t data) { 

    if (cmd & 0x08) {
        SendCmd(CMD55,0);
        cmd &= ~0x80;
    }

    QSPI_SET_SS_LOW(QSPI0);

    readByte();

    writeByte(cmd);
    writeByte((data>>24)&0xFF);
    writeByte((data>>16)&0xFF);
    writeByte((data>> 8)&0xFF);
    writeByte((data>> 0)&0xFF);
    writeByte(0xFF);

    while(QSPI_IS_BUSY(QSPI0));

    uint8_t res = 0;
    for (int32_t c = 0; c < 8; c++)
    {
        res = readByte();
        if (res != 0xFF) {
            return {true, res};
        }
    }

    return {false, 0};
}

bool SDCard::readCID() {

    auto [success, result] = SendCmd(CMD10, 0);
    if (!success || result != 0) {
        QSPI_SET_SS_HIGH(QSPI0);
        return false;
	}

    double start_time = Timeline::SystemTime();
    for ( ; readByte() != 0xFE ;) {
        if ( (Timeline::SystemTime() - start_time) > 0.5 ) {
            QSPI_SET_SS_HIGH(QSPI0);
            return false;
        }
    }

	sd_spi_cid.mid = readByte();

	uint8_t i;
	for (i = 0; i < 2; i++) {
		sd_spi_cid.oid[i] = readByte();
	}

	for (i = 0; i < 5; i++) {
		sd_spi_cid.pnm[i] = readByte();
	}

	i = readByte();
	sd_spi_cid.prv_n = i >> 4;
	sd_spi_cid.prv_m = i;

	sd_spi_cid.psn_high = readByte();
	sd_spi_cid.psn_mid_high = readByte();
	sd_spi_cid.psn_mid_low = readByte();
	sd_spi_cid.psn_low = readByte();

	i = readByte();
	sd_spi_cid.mdt_year = (i << 4) & 0xF0;
	i = readByte();
	sd_spi_cid.mdt_year |= i >> 4;
	sd_spi_cid.mdt_month = i;
	sd_spi_cid.crc = readByte() >> 1;

	readByte();
	readByte();

    printf("Read CID!\n");

    return true;
}

bool SDCard::readCSD() {
    auto [success, result] = SendCmd(CMD9, 0);
    if (!success || result != 0) {
        QSPI_SET_SS_HIGH(QSPI0);
        return false;
	}

    double start_time = Timeline::SystemTime();
    for ( ; readByte() != 0xFE ;) {
        if ( (Timeline::SystemTime() - start_time) > 0.5 ) {
            QSPI_SET_SS_HIGH(QSPI0);
            return false;
        }
    }

    uint8_t b = readByte();
	sd_spi_csd.csd_structure = b >> 6;
	b = readByte();
	sd_spi_csd.taac = b;
	b = readByte();
	sd_spi_csd.nsac = b;
	b = readByte();
	sd_spi_csd.tran_speed = b;
	b = readByte();
	sd_spi_csd.ccc_high = b >> 4;
	sd_spi_csd.ccc_low |= b << 4;
	b = readByte();
	sd_spi_csd.ccc_low |= b >> 4;
	sd_spi_csd.max_read_bl_len = b;
	b = readByte();
	sd_spi_csd.read_bl_partial = b >> 7;
	sd_spi_csd.write_bl_misalign = b >> 6;
	sd_spi_csd.read_bl_misalign = b >> 5;
	sd_spi_csd.dsr_imp = b >> 4;

	if (sd_spi_csd.csd_structure == 0) {
		sd_spi_csd.cvsi.v1.c_size_high = (b << 2) & 0x0C;
		b = readByte();
		sd_spi_csd.cvsi.v1.c_size_high |= b >> 6;
		sd_spi_csd.cvsi.v1.c_size_low = b << 2;
		b = readByte();
		sd_spi_csd.cvsi.v1.c_size_low |= b >> 6;
		sd_spi_csd.cvsi.v1.vdd_r_curr_min = b >> 3;
		sd_spi_csd.cvsi.v1.vdd_r_curr_max = b;
		b = readByte();
		sd_spi_csd.cvsi.v1.vdd_w_curr_min = b >> 5;
		sd_spi_csd.cvsi.v1.vdd_w_curr_max = b >> 2;
		sd_spi_csd.cvsi.v1.c_size_mult = (b << 1) & 0x06;
		b = readByte();
		sd_spi_csd.cvsi.v1.c_size_mult |= b >> 7;
	} else {
		b = readByte();
		sd_spi_csd.cvsi.v2.c_size_high = b;
		b = readByte();
		sd_spi_csd.cvsi.v2.c_size_mid = b;
		b = readByte();
		sd_spi_csd.cvsi.v2.c_size_low = b;
		b = readByte();
	}

	sd_spi_csd.erase_bl_en = b >> 6;
	sd_spi_csd.erase_sector_size = (b << 1) & 0x7E;
	b = readByte();
	sd_spi_csd.erase_sector_size |= b >> 7;
	sd_spi_csd.wp_grp_size = b << 1;
	b = readByte();
	sd_spi_csd.wp_grp_enable = b >> 7;
	sd_spi_csd.r2w_factor = b >> 2;
	sd_spi_csd.write_bl_len = (b << 2) & 0x0C;
	b = readByte();
	sd_spi_csd.write_bl_len |= b >> 6;
	sd_spi_csd.write_bl_partial = b >> 5;
	b = readByte();
	sd_spi_csd.file_format_grp = b >> 7;
	sd_spi_csd.copy = b >> 6;
	sd_spi_csd.perm_write_protect = b >> 5;
	sd_spi_csd.tmp_write_protect = b >> 4;
	sd_spi_csd.file_format = b >> 2;
	b = readByte();
	sd_spi_csd.crc = b >> 1;

	readByte();
	readByte();

    printf("Read CSD!\n");

    return true;
}

bool SDCard::readTotalBlocks() {
    auto [success, result] = SendCmd(CMD9, 0);
    if (!success || result != 0) {
        QSPI_SET_SS_HIGH(QSPI0);
        return false;
    }

    double start_time = Timeline::SystemTime();
    for ( ; readByte() != 0xFE ;) {
        if ( (Timeline::SystemTime() - start_time) > 0.5 ) {
            QSPI_SET_SS_HIGH(QSPI0);
            return false;
        }
    }

	uint8_t csd_structure = readByte() >> 6;

	for (size_t c = 0; c < 5; c++) {
		readByte();
	}

	uint8_t b = readByte();
    uint8_t max_read_bl_len = b & 0x0F;

	totalBlocks = 0;
	if (csd_structure == 0) {

		uint8_t c_size_high = (b << 2) & 0x0C;
		b = readByte();
		c_size_high |= b >> 6;
		uint8_t c_size_low = b << 2;
		b = readByte();
		c_size_low |= b >> 6;
		readByte();
		
		uint8_t c_size_mult;
		c_size_mult = (b << 1) & 0x06;
		b = readByte();
		c_size_mult |= b >> 7;

		totalBlocks = (uint32_t) ((c_size_high << 8 | c_size_low) + 1) * (1 << (c_size_mult + 2));
		totalBlocks *= (uint32_t) (1 << max_read_bl_len) / 512;

	} else {

		b = readByte();
		uint8_t c_size_high = b;
		b = readByte();
		uint8_t c_size_mid = b;
		b = readByte();
		uint8_t c_size_low = b;

		totalBlocks = (uint32_t) (c_size_high << 16 | c_size_mid << 8 | c_size_low) + 1;
		totalBlocks <<= 10;

	}

	for (size_t c = 0; c < 7; c++) {
		readByte();
	}

    printf("Media is %.2fGB in size!\n", (double(uint64_t(totalBlocks) * 512)) / (1024.0 * 1024.0 * 1024.0));
    return true;
}

bool SDCard::detectCardType() {

    for (size_t c = 0; c < 10; c++) {
        readByte();
    }

    {
        double start_time = Timeline::SystemTime();
        for (; std::get<1>(GoIdle()) != 1 ;) {
            if ( (Timeline::SystemTime() - start_time) > 0.5 ) {
                QSPI_SET_SS_HIGH(QSPI0);
                return false;
            }
        }
    }

    auto [success, result] = CheckVoltage();
    if (success) {
        if ( (result & 0x04 ) == 0) {
            readByte();
            readByte();
            if (readByte() == 0x01 &&
                readByte() == 0xAA) {
                isSdCard = true;
            }
            printf("SD Card detected!\n");
        } else {
            printf("MMC Card detected!\n");
        }
        if (isSdCard) {
            double start_time = Timeline::SystemTime();
            for (;std::get<1>(SendCmd(ACMD41, 0x40000000)) != 0 ;) {
                if ( (Timeline::SystemTime() - start_time) > 0.5 ) {
                    QSPI_SET_SS_HIGH(QSPI0);
                    return false;
                }
            }

            if ((std::get<1>(SendCmd(CMD58, 0)) & 0x40) != 0) {
                QSPI_SET_SS_HIGH(QSPI0);
                return false;
            }

            if ((readByte() & 0x40) != 0) {
                printf("SD Card is high capacity!\n");
                isSdHcCard = true;
            }
            readByte();
            readByte();
            readByte();
        } else {
            // Do we care?
        }
    }
    printf("Card type determined!\n");
    return true;
}

bool SDCard::setSectorSize() {
    auto [success, result] = SendCmd(CMD16, 512);
    if (!success || result != 0) {
        QSPI_SET_SS_HIGH(QSPI0);
        return false;
    }
    return true;
}

void SDCard::init() {

    QSPI_Open(QSPI0, QSPI_MASTER, QSPI_MODE_0, 8, 250000);

    QSPI_SET_SS_HIGH(QSPI0);

    if (!detectCardType()) {
        return;
    }

    if (!setSectorSize()) {
        return;
    }

    if (!readTotalBlocks()) {
        return;
    }

    if (!readCID()) {
        return;
    }

    if (!readCSD()) {
        return;
    }

    QSPI_SET_SS_HIGH(QSPI0);

/*  NVIC_SetPriority(USBD_IRQn, 4);
    NVIC_EnableIRQ(USBD_IRQn);

    USBD_SetConfigCallback(MSC_SetConfig);
    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);

    USBD_Start();*/
}
