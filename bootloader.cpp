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
#include "./main.h"
#include "./leds.h"
#include "./i2cmanager.h"
#include "./sdcard.h"

#include "M480.h"

extern "C" {
#include "./msc.h"
}

#ifdef BOOTLOADER

Bootloader &Bootloader::instance() {
    static Bootloader bootloader;
    if (!bootloader.initialized) {
        bootloader.initialized = true;
        bootloader.init();
    }
    return bootloader;
}

void Bootloader::init() {
    Leds::instance();
    I2CManager::instance();
    SDCard::instance();
}

void Bootloader::Run() {
    for(;;) {
        __WFI();
    }
}

void bootloader_entry(void) {
    Bootloader::instance().Run();
}

#endif  // #ifdef BOOTLOADER
