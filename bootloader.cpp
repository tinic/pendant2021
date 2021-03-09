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
#include "./bootloader.h"
#include "./emfat.h"
#include "./main.h"

#include "M480.h"

#ifdef BOOTLOADER

static emfat_t emfat;

#define CMA_TIME EMFAT_ENCODE_CMA_TIME(9,7,2020,12,0,0)
#define CMA { CMA_TIME, CMA_TIME, CMA_TIME }

static void firmware_read_proc(uint8_t *data, int size, uint32_t offset, size_t userdata) 
{
    (void)data;
    (void)size;
    (void)offset;
    (void)userdata;
}

static void firmware_write_proc(const uint8_t *data, int size, uint32_t offset, size_t userdata) 
{
    (void)data;
    (void)size;
    (void)offset;
    (void)userdata;
}

static emfat_entry_t emfat_entries[] =
{
    // name           dir    lvl offset  size             max_size        user  time  read                write
    { "",             true,  0,  0,      0,               0,              0,    CMA,  NULL,               NULL,                { 0, 0, 0, 0, NULL, NULL, NULL } }, // root
    { "firmware.bin", false, 1,  0,      FIRMWARE_SIZE,   FIRMWARE_SIZE,  0,    CMA,  firmware_read_proc, firmware_write_proc, { 0, 0, 0, 0, NULL, NULL, NULL } }, // firmware.bin
    { NULL,           false, 0,  0,      0,               0,              0,    CMA,    NULL,               NULL,                { 0, 0, 0, 0, NULL, NULL, NULL } }
};

Bootloader &Bootloader::instance() {
    static Bootloader bootloader;
    if (!bootloader.initialized) {
        bootloader.initialized = true;
        bootloader.init();
    }
    return bootloader;
}

void Bootloader::init() {
}

void Bootloader::Run() {

    emfat_init(&emfat, "duckpond", emfat_entries);

    while (1) {
        __WFI();
    }
}

void bootloader_entry(void) {
    Bootloader::instance().Run();
}

#endif  // #ifdef BOOTLOADER
