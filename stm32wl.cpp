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
#include "./model.h"

#include "M480.h"

#include <cstddef>

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
    i2cRegs.effectN = Model::instance().Effect();
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,effectN),i2cRegs.effectN);
    i2cRegs.brightness = uint8_t(Model::instance().Brightness() * 255.0f);
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,brightness),i2cRegs.brightness);
    printf("STM32WL::update\r\n");
}

void STM32WL::init() {
    if (!devicePresent) return;
    for (size_t c = 0; c < sizeof(i2cRegs.devEUI); c++) {
         i2cRegs.devEUI[c] = I2CManager::instance().getReg8(i2c_addr,c+offsetof(I2CRegs,devEUI));
    }
    for (size_t c = 0; c < sizeof(i2cRegs.appKey); c++) {
         i2cRegs.joinEUI[c] = I2CManager::instance().getReg8(i2c_addr,c+offsetof(I2CRegs,joinEUI));
    }
    for (size_t c = 0; c < sizeof(i2cRegs.appKey); c++) {
         i2cRegs.appKey[c] = I2CManager::instance().getReg8(i2c_addr,c+offsetof(I2CRegs,appKey));
    }
    printf("STM32WL DevEUI: ");
    for(size_t c = 0; c < 8; c++) {
        printf("%02x ",i2cRegs.devEUI[c]);
    }
    printf("\r\nSTM32WL JoinEUI: ");
    for(size_t c = 0; c < 8; c++) {
        printf("%02x ",i2cRegs.joinEUI[c]);
    }
    printf("\r\nSTM32WL AppKey: ");
    for(size_t c = 0; c < 16; c++) {
        printf("%02x ",i2cRegs.appKey[c]);
    }
    printf("\r\n");
}
