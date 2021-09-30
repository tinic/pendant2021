/*
Copyright 2020 Tinic Uro

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
#ifndef SDCARD_H_
#define SDCARD_H_

#include "./version.h"

#include <stdint.h>
#include <tuple>

#include "ff.h"
#include "diskio.h"

class SDCard {
public:
    static SDCard &instance();

    void process();

    bool inserted() const;
    uint32_t blocks() const;

    void readBlock(uint32_t blockAddr, uint8_t *buffer, int32_t blockLen);
    void writeBlock(uint32_t blockAddr, const uint8_t *buffer, int32_t blockLen);

    bool readFromDataFile(uint8_t *outBuf, size_t offset, size_t size);
    bool dataFilePresent() const { return datafile_present; }

    bool newFirmwareAvailable() const {  
        return firmware_bootloaded && 
               firmware_release && 
               firmware_revision > GIT_REV_COUNT_INT; 
    }

private:
    struct
    {
        /** Manufacturer ID. */
        unsigned int    mid:            8;
        /** OEM/Application ID. Composed of 2 characters. */
        char            oid[2];
        /** Product name. Composed of 5 characters. */
        char            pnm[5];
        /** n part of the product revision number, which is in the form of n.m */
        unsigned int    prv_n:            4;
        /** m part of the product revision number, which is in the form of n.m */
        unsigned int    prv_m:            4;
        /** Bits [31:24] of the 4 byte product serial number. */
        unsigned int    psn_high:        8;
        /** Bits [23:16] of the 4 byte product serial number. */
        unsigned int    psn_mid_high:    8;
        /** Bits [15:8] of the 4 byte product serial number. */
        unsigned int    psn_mid_low:    8;
        /** Bits [7:0] of the 4 byte product serial number. */
        unsigned int    psn_low:        8;
        /** Year manufactured. */
        unsigned int    mdt_year:        8;
        /** Month manufactured. */
        unsigned int    mdt_month:        4;
        /* Bitfield padding */
        unsigned int:                    4;
        /** Checksum for the CID. */
        unsigned int    crc:            7;
        /* Bitfield padding */
        unsigned int:                    1;
    } sd_spi_cid;

    typedef struct sd_spi_csd_v1
    {
        /** Low bits of c_size. Used to compute the card's size. */
        unsigned int    c_size_low:        8;
        /** High bits of c_size. Used to compute the card's size. */
        unsigned int    c_size_high:    4;
        /** Max read current for minimal supply voltage. */
        unsigned int    vdd_r_curr_min:    3;
        /* Bitfield padding */
        unsigned int:                    1;
        /** Max read current for max supply voltage. */
        unsigned int    vdd_r_curr_max:    3;
        /** Max write current for minimal supply voltage. */
        unsigned int    vdd_w_curr_min:    3;
        /* Bitfield padding */
        unsigned int:                    2;
        /** Max write current for max supply voltage. */
        unsigned int    vdd_w_curr_max:    3;
        /** Multiplier used to compute the card's size. */
        unsigned int     c_size_mult:    3;
        /* Bitfield padding */
        unsigned int:                    2;
    } sd_spi_csd_v1_t;

    typedef struct sd_spi_csd_v2
    {
        /** Low bits of c_size. Used to compute the card's size. */
        unsigned int    c_size_low:        8;
        /** Mid bits of c_size. Used to compute the card's size. */
        unsigned int    c_size_mid:        8;
        /** High bits of c_size. Used to compute the card's size. */
        unsigned int    c_size_high:    6;
        /* Bitfield padding */
        unsigned int:                    2;
    } sd_spi_csd_v2_t;

    typedef union sd_spi_csd_v_info
    {
        sd_spi_csd_v1_t v1;
        sd_spi_csd_v2_t v2;
    } sd_spi_csd_v_info_t;

    struct
    {
        /** Union of the CSD version specific info. */
        sd_spi_csd_v_info_t        cvsi;
        /** CSD version. */
        unsigned int             csd_structure:        2;
        /** The file format of the card. */
        unsigned int            file_format:        2;
        /* Bitfield padding */
        unsigned int:                                4;
        /** Asynchronous part of data access time. Table of the corresponding
            values are found in the SD specifications. */
        unsigned int            taac:                8;
        /** Worst case for the clock dependent factor of the data access time.*/
        unsigned int            nsac:                8;
        /** Max data transfer speed for a single data line. */
        unsigned int            tran_speed:            8;
            /** Low bits of the ccc; the command class of the card. */
        unsigned int            ccc_low:            8;
        /** High bits of the ccc; the command class of the card. */
        unsigned int            ccc_high:            4;
        /** Max block length. */
        unsigned int            max_read_bl_len:    4;
        /** Defines if the card can read partial blocks. */
        unsigned int            read_bl_partial:    1;
        /** Defines if a data block can be spanned over a block boundary when
            written. */
        unsigned int            write_bl_misalign:    1;
        /** Defines if a data can be read over block boundaries. */
        unsigned int            read_bl_misalign:    1;
        /** Defines if the configurable driver stage is implemented in the card. */
        unsigned int            dsr_imp:            1;
        /** Defines the unit size of data being erased. */
        unsigned int            erase_bl_en:        1;
        /** Ratio of read to write time. */
        unsigned int            r2w_factor:            3;
        /** The size of an erasable sector. */
        unsigned int            erase_sector_size:    7;
        /** Defines if write group protection is possible. */
        unsigned int            wp_grp_enable:        1;
        /** The size of a write protected group. */
        unsigned int            wp_grp_size:        7;
        /** Defines if the card can write partial blocks. */
        unsigned int            write_bl_partial:    1;
        /** The max write block length. */
        unsigned int            write_bl_len:        4;
        /** The file format group of the card. */
        unsigned int            file_format_grp:    1;
        /** Defines if the cards content is original or is a copy. */
        unsigned int            copy:                1;
        /** Determines if the card is permanently write protected or not. */
        unsigned int            perm_write_protect:    1;
        /** Determines if the card is temporarily write protected or not. */
        unsigned int            tmp_write_protect:    1;
        /** Checksum for the CSD. */
        unsigned int            crc:                7;
        /* Bitfield padding */
        unsigned int:                                1;
    } sd_spi_csd;

    FATFS FatFs;
    bool mounted = false;

    uint32_t cardType = 0;
    uint32_t totalBlocks = 0;
    uint32_t u32TrimInit = 0;

    bool datafile_present = false;

    bool firmware_release = false;
    bool firmware_bootloaded = false;

    uint32_t firmware_revision = 0;
    
    bool detectCardType();
    bool setSectorSize();
    bool readCID();
    bool readCSD();

    void findFirmware();
    void findDataFile();

    friend DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);
    friend DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count );

    bool goIdle();
    bool waitReady();

    std::tuple<bool, uint8_t> SendCmd(uint8_t cmd, uint32_t data);
    bool readBytes(uint8_t *buffer, size_t len);

    uint8_t QSPIReadByte();
    void QSPIWriteByte(uint8_t byte);

    void init();
    bool initialized = false;
};

#endif /* SDCARD_H_ */
