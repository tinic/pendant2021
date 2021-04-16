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
#include "./stm32wl.h"

#include "M480.h"

#include "./i2cmanager.h"

bool STM32WL::devicePresent = false;

STM32WL &STM32WL::instance() {
    static STM32WL stm32wl;
    if (!stm32wl.initialized) {
        stm32wl.initialized = true;
        stm32wl.init();
    }
    return stm32wl;
}

void STM32WL::update() {
    if (!devicePresent) return;
}

void STM32WL::init() {
    if (!devicePresent) return;
    for (size_t c = 0; c < sizeof(devEUI); c++) {
         devEUI[c] = I2CManager::instance().getReg8(i2c_addr,c);
    }
    for (size_t c = 0; c < sizeof(appKey); c++) {
         appKey[c] = I2CManager::instance().getReg8(i2c_addr,c+8);
    }
    printf("STM32WL DevEUI: ");
    for(size_t c = 0; c < 8; c++) {
        printf("%02x ",devEUI[c]);
    }
    printf("\r\nSTM32WL AppKey: ");
    for(size_t c = 0; c < 16; c++) {
        printf("%02x ",appKey[c]);
    }
    printf("\r\n");
}
