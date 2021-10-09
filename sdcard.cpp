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
#include "./main.h"
#include "./msc.h"
#include "./stm32wl.h"
#include "./timeline.h"
#include "./version.h"

#include "M480.h"
#include "diskio.h"
#include "ff.h"

#include <functional>

enum {
    CMD0 = 0x40 + 0, // GO_IDLE_STATE
    CMD1 = 0x40 + 1, // SEND_OP_COND (MMC)
    ACMD13 = 0xC0 + 13, // SD_STATUS (SDC)
    ACMD41 = 0xC0 + 41, // SEND_OP_COND (SDC)
    CMD8 = 0x40 + 8, // SEND_IF_COND
    CMD9 = 0x40 + 9, // SEND_CSD
    CMD10 = 0x40 + 10, // SEND_CID
    CMD16 = 0x40 + 16, // SET_BLOCKLEN
    CMD17 = 0x40 + 17, // READ_SINGLE_BLOCK
    CMD24 = 0x40 + 24, // WRITE_SINGLE_BLOCK
    CMD42 = 0x40 + 42, // LOCK_UNLOCK
    CMD55 = 0x40 + 55, // APP_CMD
    CMD58 = 0x40 + 58, // READ_OCR
    CMD59 = 0x40 + 59, // CRC_ON_OFF
    CMD
};

enum {
    markerBootable = 0xB007AB1E,

    markerTesting = 0x7E577E57,
    markerRelease = 0x5ADD1ED0,

    markerBootloader = 0xB00710AD,
    markerBootloaded = 0x1ED54B0B,
};

static constexpr uint32_t swap(uint32_t v) {
    return ((v >> 24) & 0xFF) | ((v & 0xFF) << 24) | ((v >> 8) & 0xFF00) | ((v & 0xFF00) << 8);
}

extern "C" const uint32_t __attribute__((used, section(".metadata"))) metadata[] = {
    swap(markerBootable),
#if defined(TESTING)
    swap(markerTesting),
#else
    swap(markerRelease),
#endif // defined(TESTING)
#if defined(BOOTLOADER)
    swap(markerBootloader),
#else // #if defined(BOOTLOADER)
    swap(markerBootloaded),
#endif // #if defined(BOOTLOADER)
    GIT_REV_COUNT_INT,
    0, 0, 0, 0
};

extern "C" DWORD get_fattime(void) {
    return STM32WL::instance().DateTime();
}

static constexpr uint8_t DEV_MMC = 0;

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) {
        return STA_NOINIT;
    }

    return 0;
}

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) {
        return STA_NOINIT;
    }

    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0) {
        return RES_PARERR;
    }

    SDCard::instance().readBlock(sector, buff, count);
    return RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0) {
        return RES_PARERR;
    }

    SDCard::instance().writeBlock(sector, buff, count);
    return RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv != 0) {
        return RES_PARERR;
    }
    switch (cmd) {
    case CTRL_SYNC: {
        printf("CTRL_SYNC\n");
        return RES_OK;
    } break;
    }
    return RES_PARERR;
}

#define TRIM_INIT (SYS_BASE + 0x10C)

SDCard& SDCard::instance() {
    static SDCard sdcard;
    if (!sdcard.initialized) {
        sdcard.initialized = true;
        sdcard.init();
    }
    return sdcard;
}

void SDCard::process() {

    if (((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x1)) {
        /* Start USB trim if it is not enabled. */
        if ((SYS->HIRCTCTL & SYS_HIRCTCTL_FREQSEL_Msk) != 1) {
            if (USBD->INTSTS & USBD_INTSTS_SOFIF_Msk) {
                /* Clear SOF */
                USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;

                /* Re-enable crystal-less */
                SYS->HIRCTCTL = 0x1;
                SYS->HIRCTCTL |= SYS_HIRCTCTL_REFCKSEL_Msk;
            }
        }

        /* Disable USB Trim when error */
        if (SYS->HIRCTISTS & (SYS_HIRCTISTS_CLKERRIF_Msk | SYS_HIRCTISTS_TFAILIF_Msk)) {
            /* Init TRIM */
            M32(TRIM_INIT) = u32TrimInit;

            /* Disable crystal-less */
            SYS->HIRCTCTL = 0;

            /* Clear error flags */
            SYS->HIRCTISTS = SYS_HIRCTISTS_CLKERRIF_Msk | SYS_HIRCTISTS_TFAILIF_Msk;

            /* Clear SOF */
            USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;
        }
    }

    MSC_ProcessCmd();
}

void SDCard::readBlock(uint32_t blockAddr, uint8_t* buffer, int32_t blockLen) {
    for (; blockLen > 0;) {
        SendCmd(CMD17, (cardType & CT_BLOCK) == 0 ? (blockAddr * 512) : blockAddr);
        readBytes(buffer, 512);
        blockLen--;
        blockAddr++;
    }
    QSPI_SET_SS_HIGH(QSPI0);
}

void SDCard::writeBlock(uint32_t blockAddr, const uint8_t* buffer, int32_t blockLen) {
#if 0
    for (; blockLen > 0 ;) {
        SendCmd(CMD24, (cardType & CT_BLOCK) == 0 ? (blockAddr * 512) : blockAddr);
        for(size_t c = 0; c < 512; c++) {
            writeByte(*buffer++);    
        }
        blockLen--;
        blockAddr++;
    }
    QSPI_SET_SS_HIGH(QSPI0);
#endif // #if 0
}

bool SDCard::readBytes(uint8_t* buf, size_t len) {
    uint64_t start_time = Timeline::FastSystemTime();
    uint64_t max_timout = Timeline::FastSystemTimeCmp() * 10;
    for (; QSPIReadByte() != 0xFE;) {
        if ((Timeline::FastSystemTime() - start_time) > max_timout) {
            printf("SDCard::readBytes: timeout %d len %d!\n", int((Timeline::FastSystemTime() - start_time)), int(len));
            return false;
        }
    }

    QSPI_SET_DATA_WIDTH(QSPI0, 32);
    uint32_t* buf32 = reinterpret_cast<uint32_t*>(buf);
    for (size_t c = 0; c < len; c += 4) {
        QSPI_WRITE_TX(QSPI0, 0xFFFFFFFF);
        while (QSPI_IS_BUSY(QSPI0))
            ;
        *buf32++ = __builtin_bswap32(QSPI_READ_RX(QSPI0));
    }
    QSPI_SET_DATA_WIDTH(QSPI0, 8);
    QSPIReadByte(); // crc
    QSPIReadByte(); // crc
    return true;
}

uint8_t SDCard::QSPIReadByte() {
    QSPI_WRITE_TX(QSPI0, 0xFF);
    while (QSPI_IS_BUSY(QSPI0))
        ;
    return QSPI_READ_RX(QSPI0);
}

void SDCard::QSPIWriteByte(uint8_t byte) {
    QSPI_WRITE_TX(QSPI0, byte);
    while (QSPI_IS_BUSY(QSPI0))
        ;
    QSPI_READ_RX(QSPI0);
}

bool SDCard::waitReady() {
    uint64_t start_time = Timeline::FastSystemTime();
    uint64_t max_timout = Timeline::FastSystemTimeCmp() * 10;
    for (; QSPIReadByte() != 0xFF;) {
        if ((Timeline::FastSystemTime() - start_time) > max_timout) {
            printf("SDCard::waitReady: timeout %d!\n", int((Timeline::FastSystemTime() - start_time)));
            return false;
        }
    }
    return false;
}

std::tuple<bool, uint8_t> SDCard::SendCmd(uint8_t cmd, uint32_t data) {

    if (cmd & 0x08) {
        SendCmd(CMD55, 0);
        cmd &= ~0x80;
    }

    QSPI_SET_SS_HIGH(QSPI0);
    QSPIReadByte();
    QSPI_SET_SS_LOW(QSPI0);
    QSPIReadByte();
    waitReady();

    QSPIWriteByte(cmd);
    QSPIWriteByte((data >> 24) & 0xFF);
    QSPIWriteByte((data >> 16) & 0xFF);
    QSPIWriteByte((data >> 8) & 0xFF);
    QSPIWriteByte((data >> 0) & 0xFF);
    uint8_t ncr = 0x01;
    if (cmd == CMD0)
        ncr = 0x95;
    if (cmd == CMD8)
        ncr = 0x87;
    QSPIWriteByte(ncr);

    uint8_t res = 0;
    for (int32_t c = 0; c < 10; c++) {
        res = QSPIReadByte();
        if ((res & 0x80) == 0) {
            return { true, res };
        }
    }

    return { false, 0 };
}

bool SDCard::readCID() {

    auto [success, result] = SendCmd(CMD10, 0);
    if (!success || result != 0) {
        return false;
    }

    uint8_t cid[16];
    readBytes(cid, 16);
    uint8_t* cidPtr = cid;

    sd_spi_cid.mid = *cidPtr++;

    uint8_t i;
    for (i = 0; i < 2; i++) {
        sd_spi_cid.oid[i] = *cidPtr++;
    }

    for (i = 0; i < 5; i++) {
        sd_spi_cid.pnm[i] = *cidPtr++;
    }

    i = *cidPtr++;
    sd_spi_cid.prv_n = i >> 4;
    sd_spi_cid.prv_m = i;

    sd_spi_cid.psn_high = *cidPtr++;
    sd_spi_cid.psn_mid_high = *cidPtr++;
    sd_spi_cid.psn_mid_low = *cidPtr++;
    sd_spi_cid.psn_low = *cidPtr++;

    i = *cidPtr++;
    sd_spi_cid.mdt_year = (i << 4) & 0xF0;
    i = *cidPtr++;
    sd_spi_cid.mdt_year |= i >> 4;
    sd_spi_cid.mdt_month = i;
    sd_spi_cid.crc = *cidPtr++ >> 1;

    printf("SDCard: Read CID!\n");

    return true;
}

bool SDCard::readCSD() {

    auto [success, result] = SendCmd(CMD9, 0);
    if (!success || result != 0) {
        return false;
    }

    uint8_t csd[16];
    readBytes(csd, 16);
    uint8_t* csdPtr = csd;

    uint8_t b = *csdPtr++;
    sd_spi_csd.csd_structure = b >> 6;
    b = *csdPtr++;
    sd_spi_csd.taac = b;
    b = *csdPtr++;
    sd_spi_csd.nsac = b;
    b = *csdPtr++;
    sd_spi_csd.tran_speed = b;
    b = *csdPtr++;
    sd_spi_csd.ccc_high = b >> 4;
    sd_spi_csd.ccc_low |= b << 4;
    b = *csdPtr++;
    sd_spi_csd.ccc_low |= b >> 4;
    sd_spi_csd.max_read_bl_len = b;
    b = *csdPtr++;
    sd_spi_csd.read_bl_partial = b >> 7;
    sd_spi_csd.write_bl_misalign = b >> 6;
    sd_spi_csd.read_bl_misalign = b >> 5;
    sd_spi_csd.dsr_imp = b >> 4;

    if (sd_spi_csd.csd_structure == 0) {
        sd_spi_csd.cvsi.v1.c_size_high = (b << 2) & 0x0C;
        b = *csdPtr++;
        sd_spi_csd.cvsi.v1.c_size_high |= b >> 6;
        sd_spi_csd.cvsi.v1.c_size_low = b << 2;
        b = *csdPtr++;
        sd_spi_csd.cvsi.v1.c_size_low |= b >> 6;
        sd_spi_csd.cvsi.v1.vdd_r_curr_min = b >> 3;
        sd_spi_csd.cvsi.v1.vdd_r_curr_max = b;
        b = *csdPtr++;
        sd_spi_csd.cvsi.v1.vdd_w_curr_min = b >> 5;
        sd_spi_csd.cvsi.v1.vdd_w_curr_max = b >> 2;
        sd_spi_csd.cvsi.v1.c_size_mult = (b << 1) & 0x06;
        b = *csdPtr++;
        sd_spi_csd.cvsi.v1.c_size_mult |= b >> 7;
    } else {
        b = *csdPtr++;
        sd_spi_csd.cvsi.v2.c_size_high = b;
        b = *csdPtr++;
        sd_spi_csd.cvsi.v2.c_size_mid = b;
        b = *csdPtr++;
        sd_spi_csd.cvsi.v2.c_size_low = b;
        b = *csdPtr++;
    }

    sd_spi_csd.erase_bl_en = b >> 6;
    sd_spi_csd.erase_sector_size = (b << 1) & 0x7E;
    b = *csdPtr++;
    sd_spi_csd.erase_sector_size |= b >> 7;
    sd_spi_csd.wp_grp_size = b << 1;
    b = *csdPtr++;
    sd_spi_csd.wp_grp_enable = b >> 7;
    sd_spi_csd.r2w_factor = b >> 2;
    sd_spi_csd.write_bl_len = (b << 2) & 0x0C;
    b = *csdPtr++;
    sd_spi_csd.write_bl_len |= b >> 6;
    sd_spi_csd.write_bl_partial = b >> 5;
    b = *csdPtr++;
    sd_spi_csd.file_format_grp = b >> 7;
    sd_spi_csd.copy = b >> 6;
    sd_spi_csd.perm_write_protect = b >> 5;
    sd_spi_csd.tmp_write_protect = b >> 4;
    sd_spi_csd.file_format = b >> 2;
    b = *csdPtr++;
    sd_spi_csd.crc = b >> 1;

    printf("SDCard: Read CSD!\n");

    totalBlocks = 0;
    if (sd_spi_csd.csd_structure == 0) {
        totalBlocks = (uint32_t)((sd_spi_csd.cvsi.v1.c_size_high << 8 | sd_spi_csd.cvsi.v1.c_size_low) + 1) * (1 << (sd_spi_csd.cvsi.v1.c_size_mult + 2));
        totalBlocks *= (uint32_t)(1 << sd_spi_csd.max_read_bl_len) / 512;
    } else {
        totalBlocks = (uint32_t)(sd_spi_csd.cvsi.v2.c_size_high << 16 | sd_spi_csd.cvsi.v2.c_size_mid << 8 | sd_spi_csd.cvsi.v2.c_size_low) + 1;
        totalBlocks <<= 10;
    }

    printf("SDCard: Media is %.2fGB in size!\n", (double(uint64_t(totalBlocks) * 512)) / (1024.0 * 1024.0 * 1024.0));

    return true;
}

bool SDCard::inserted() const {
    return true;
}

uint32_t SDCard::blocks() const {
    return totalBlocks;
}

bool SDCard::goIdle() {
    uint8_t res = 0;
    for (int32_t c = 0; c < 10; c++) {
        res = QSPIReadByte();
        if ((res & 0x80) == 0) {
            break;
        }
    }

    uint64_t start_time = Timeline::FastSystemTime();
    uint64_t max_timout = Timeline::FastSystemTimeCmp() * 10;
    for (; std::get<1>(SendCmd(CMD0, 0)) != 1;) {
        if ((Timeline::FastSystemTime() - start_time) > max_timout) {
            printf("SDCard::goIdle: timeout %d!\n", int((Timeline::FastSystemTime() - start_time)));
            return false;
        }
    }
    return true;
}

bool SDCard::detectCardType() {

    if (!goIdle()) {
        return false;
    }

    bool SDv2 = false;
    auto [success, result] = SendCmd(CMD8, 0x1AA);
    if (success) {
        if ((result & 0x04) == 0) {
            QSPIReadByte();
            QSPIReadByte();
            if (QSPIReadByte() == 0x01 && QSPIReadByte() == 0xAA) {
                SDv2 = true;
            }
            printf("SDCard: SD Card detected!\n");
        } else {
            printf("SDCard: MMC Card detected! (Unsupported)\n");
        }
        if (SDv2) {
            uint64_t start_time = Timeline::FastSystemTime();
            uint64_t max_timout = Timeline::FastSystemTimeCmp() * 10;
            for (; std::get<1>(SendCmd(ACMD41, 0x40000000)) != 0;) {
                if ((Timeline::FastSystemTime() - start_time) > max_timout) {
                    printf("SDCard::detectCardType: timeout %d!\n", int((Timeline::FastSystemTime() - start_time)));
                    return false;
                }
            }
            if ((std::get<1>(SendCmd(CMD58, 0)) & 0x40) != 0) {
                return false;
            }
            if ((QSPIReadByte() & 0x40) != 0) {
                printf("SDCard: SD Card is high capacity!\n");
                cardType = CT_SDC2 | CT_BLOCK;
            } else {
                printf("SDCard: SD Card is low capacity!\n");
                cardType = CT_SDC2;
            }
            QSPIReadByte();
            QSPIReadByte();
            QSPIReadByte();
        } else {
            // Do we care?
            return false;
        }
    }

    printf("SDCard: Card type determined!\n");
    return true;
}

bool SDCard::setSectorSize() {

    auto [success, result] = SendCmd(CMD16, 512);
    if (!success || result != 0) {
        return false;
    }
    return true;
}

bool SDCard::readFromDataFile(uint8_t* outBuf, size_t offset, size_t size) {
    if (!datafile_present) {
        return false;
    }

    FIL Fil;
    if (f_open(&Fil, "data.bin", FA_READ | FA_OPEN_EXISTING) == FR_OK) {
        f_lseek(&Fil, offset);
        UINT readLen = size;
        f_read(&Fil, outBuf, readLen, &readLen);
        f_close(&Fil);
        return readLen == size;
    }

    return false;
}

void SDCard::findDataFile() {
    if (!mounted) {
        return;
    }

    FIL Fil;
    if (f_open(&Fil, "data.bin", FA_READ | FA_OPEN_EXISTING) == FR_OK) {
        printf("SDCard: Found data.bin!\n");
        datafile_present = true;
        f_close(&Fil);
    }
}

void SDCard::findFirmware() {
    if (!mounted) {
        return;
    }

    const char* filename = "firmware.bin";

    FILINFO FilInfo = {};
    f_stat(filename, &FilInfo);
    if (FilInfo.fsize < 32768) {
        return;
    }

    FIL Fil = {};
    if (f_open(&Fil, filename, FA_READ | FA_OPEN_EXISTING) != FR_OK) {
        return;
    }

    printf("SDCard: Found firmware.bin!\n");

    const size_t markerSize = 32;

    f_lseek(&Fil, FilInfo.fsize - markerSize);

    BYTE buffer[markerSize];
    UINT readLen = markerSize;
    f_read(&Fil, buffer, readLen, &readLen);
    if (readLen != markerSize) {
        return;
    }

    auto matchMarker = [=](uint32_t pos, uint32_t marker) {
        // Big-endian
        return buffer[pos * sizeof(uint32_t) + 0] == ((marker >> 24) & 0xFF) && buffer[pos * sizeof(uint32_t) + 1] == ((marker >> 16) & 0xFF) && buffer[pos * sizeof(uint32_t) + 2] == ((marker >> 8) & 0xFF) && buffer[pos * sizeof(uint32_t) + 3] == ((marker >> 0) & 0xFF);
    };

    auto readUint32 = [=](uint32_t pos) {
        return (uint32_t(buffer[pos * sizeof(uint32_t) + 0]) << 0) | (uint32_t(buffer[pos * sizeof(uint32_t) + 1]) << 8) | (uint32_t(buffer[pos * sizeof(uint32_t) + 2]) << 16) | (uint32_t(buffer[pos * sizeof(uint32_t) + 3]) << 24);
    };

    if (!matchMarker(0, markerBootable)) {
        return;
    }

    printf("SDCard: Found valid firmware sentinel!\n");
    if (!matchMarker(1, markerTesting) && !matchMarker(1, markerRelease)) {
        return;
    }

    firmware_release = matchMarker(1, markerRelease);
    printf("SDCard: Firmware type %d\n", firmware_release ? 1 : 0);
    if (!matchMarker(2, markerBootloader) && !matchMarker(2, markerBootloaded)) {
        return;
    }

    firmware_bootloaded = matchMarker(2, markerBootloaded);
    printf("SDCard: Firmware variant %d\n", firmware_bootloaded ? 1 : 0);

    firmware_revision = readUint32(3);
    printf("SDCard: Firmware rev %d\n", int(firmware_revision));

    f_close(&Fil);
}

void SDCard::init() {
    QSPI0->SSCTL = QSPI_SS_ACTIVE_LOW;
    QSPI0->CTL = QSPI_MASTER | (8 << QSPI_CTL_DWIDTH_Pos) | (QSPI_MODE_0) | QSPI_CTL_QSPIEN_Msk;
    QSPI0->CLKDIV = 0U;

    QSPI_SetFIFO(QSPI0, 7, 7);

    if (!detectCardType()) {
        return;
    }

    if (!setSectorSize()) {
        return;
    }

    if (!readCID()) {
        return;
    }

    if (!readCSD()) {
        return;
    }

    if (f_mount(&FatFs, "", 0) == FR_OK) {
        printf("SDCard: mounted!\n");
        mounted = true;
        findFirmware();
#ifndef BOOTLOADER
        findDataFile();
#endif // #ifndef BOOTLOADER
    }

    return;

#ifndef BOOTLOADER
    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);
    USBD_SetConfigCallback(MSC_SetConfig);

    MSC_Init();
    USBD_Start();

    if (((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x1)) {
        /* Start USB trim */
        USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;
        while ((USBD->INTSTS & USBD_INTSTS_SOFIF_Msk) == 0)
            ;
        SYS->HIRCTCTL = 0x1;
        SYS->HIRCTCTL |= SYS_HIRCTCTL_REFCKSEL_Msk;
        /* Backup default trim */
        u32TrimInit = M32(TRIM_INIT);
    }

    NVIC_SetPriority(USBD_IRQn, 4);
    NVIC_EnableIRQ(USBD_IRQn);
#endif // #ifndef BOOTLOADER
}
