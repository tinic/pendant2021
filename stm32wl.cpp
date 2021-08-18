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
#include "./timeline.h"

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

    // Get zero register
    I2CManager::instance().getReg8(i2c_addr, 0);

    for (size_t c = 0; c < sizeof(i2cRegs.fields.devEUI); c++) {
         i2cRegs.fields.devEUI[c] = I2CManager::instance().getReg8(i2c_addr,c+offsetof(I2CRegs,fields.devEUI));
    }
    for (size_t c = 0; c < sizeof(i2cRegs.fields.joinEUI); c++) {
         i2cRegs.fields.joinEUI[c] = I2CManager::instance().getReg8(i2c_addr,c+offsetof(I2CRegs,fields.joinEUI));
    }
    for (size_t c = 0; c < sizeof(i2cRegs.fields.appKey); c++) {
         i2cRegs.fields.appKey[c] = I2CManager::instance().getReg8(i2c_addr,c+offsetof(I2CRegs,fields.appKey));
    }

    auto set8u = [](auto offset, auto value) {
        static_assert(std::is_same<decltype(offset), size_t>::value, "offset must be size_t");
        static_assert(std::is_same<decltype(value), uint8_t>::value, "value must be uint8_t");
        I2CManager::instance().setReg8(i2c_addr,offset,value);
    };

    auto set16u = [](auto offset, auto value) {
        static_assert(std::is_same<decltype(offset), size_t>::value, "offset must be size_t");
        static_assert(std::is_same<decltype(value), uint16_t>::value, "value must be uint16_t");
        I2CManager::instance().setReg8(i2c_addr,offset+0,(value>>0)&0xFF);
        I2CManager::instance().setReg8(i2c_addr,offset+1,(value>>8)&0xFF);
    };

    // controller
    set8u(offsetof(I2CRegs,fields.effectN),i2cRegs.fields.effectN = Model::instance().Effect());
    set8u(offsetof(I2CRegs,fields.brightness),i2cRegs.fields.brightness = uint8_t(Model::instance().Brightness() * 255.0f));

    set16u(offsetof(I2CRegs,fields.systemTime), i2cRegs.fields.systemTime = uint16_t(Timeline::SystemTime()));

    Model::instance().RingColor().write_rgba_bytes(&i2cRegs.fields.ring_color[0]);
    Model::instance().BirdColor().write_rgba_bytes(&i2cRegs.fields.bird_color[0]);
    for (size_t c = 0; c < 4; c++) {
        set8u(offsetof(I2CRegs,fields.ring_color)+c,i2cRegs.fields.ring_color[c]);
        set8u(offsetof(I2CRegs,fields.bird_color)+c,i2cRegs.fields.bird_color[c]);
    }

    set16u(offsetof(I2CRegs,fields.switch1Count), i2cRegs.fields.switch1Count = uint16_t(Model::instance().Switch1Count()));
    set16u(offsetof(I2CRegs,fields.switch2Count), i2cRegs.fields.switch2Count = uint16_t(Model::instance().Switch2Count()));
    set16u(offsetof(I2CRegs,fields.switch3Count), i2cRegs.fields.switch3Count = uint16_t(Model::instance().Switch3Count()));
    set16u(offsetof(I2CRegs,fields.bootCount), i2cRegs.fields.bootCount = uint16_t(Model::instance().BootCount()));

    // peripheral
    set16u(offsetof(I2CRegs,fields.intCount), i2cRegs.fields.intCount = uint16_t(Model::instance().IntCount()));
    set16u(offsetof(I2CRegs,fields.dselCount), i2cRegs.fields.dselCount = uint16_t(Model::instance().DselCount()));

}

void STM32WL::init() {
    if (!devicePresent) return;
    update();
    printf("STM32WL DevEUI: ");
    for(size_t c = 0; c < 8; c++) {
        printf("%02x ",i2cRegs.fields.devEUI[c]);
    }
    printf("\r\nSTM32WL JoinEUI: ");
    for(size_t c = 0; c < 8; c++) {
        printf("%02x ",i2cRegs.fields.joinEUI[c]);
    }
    printf("\r\nSTM32WL AppKey: ");
    for(size_t c = 0; c < 16; c++) {
        printf("%02x ",i2cRegs.fields.appKey[c]);
    }
    printf("\r\n");
}
