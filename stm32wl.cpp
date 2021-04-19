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
#include "./bq25895.h"
#include "./ens210.h"
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
    for (size_t c = 0; c < sizeof(i2cRegs.devEUI); c++) {
         i2cRegs.devEUI[c] = I2CManager::instance().getReg8(i2c_addr,c+offsetof(I2CRegs,devEUI));
    }
    for (size_t c = 0; c < sizeof(i2cRegs.joinEUI); c++) {
         i2cRegs.joinEUI[c] = I2CManager::instance().getReg8(i2c_addr,c+offsetof(I2CRegs,joinEUI));
    }
    for (size_t c = 0; c < sizeof(i2cRegs.appKey); c++) {
         i2cRegs.appKey[c] = I2CManager::instance().getReg8(i2c_addr,c+offsetof(I2CRegs,appKey));
    }
    i2cRegs.effectN = Model::instance().Effect();
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,effectN),i2cRegs.effectN);

    i2cRegs.brightness = uint8_t(Model::instance().Brightness() * 255.0f);
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,brightness),i2cRegs.brightness);

    i2cRegs.systemTime = uint16_t(Timeline::SystemTime());
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,systemTime)+0,((i2cRegs.systemTime)>>0)&0xFF);
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,systemTime)+1,((i2cRegs.systemTime)>>8)&0xFF);

    auto f2u8 = [](float v, float min, float max) {
        return static_cast<uint8_t>(std::clamp( ( (v - min) / (max - min) ) * 255.0f, 0.0f, 255.0f));;
    };

    i2cRegs.batteryVoltage = f2u8(BQ25895::instance().BatteryVoltage(), 2.7f, 4.2f);
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,batteryVoltage),i2cRegs.batteryVoltage);

    i2cRegs.systemVoltage = f2u8(BQ25895::instance().SystemVoltage(), 2.7f, 4.2f);
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,systemVoltage),i2cRegs.systemVoltage);

    i2cRegs.vbusVoltage = f2u8(BQ25895::instance().VBUSVoltage(), 0.0f, 5.5f);
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,vbusVoltage),i2cRegs.vbusVoltage);

    i2cRegs.chargeCurrent = f2u8(BQ25895::instance().ChargeCurrent(), 0.0f, 1000.0f);
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,chargeCurrent),i2cRegs.chargeCurrent);

    i2cRegs.temperature = f2u8(ENS210::instance().Temperature(), 0.0f, 50.0f);
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,temperature),i2cRegs.temperature);

    i2cRegs.humidity = f2u8(ENS210::instance().Humidity(), 0.0f, 1.0f);
    I2CManager::instance().setReg8(i2c_addr,offsetof(I2CRegs,humidity),i2cRegs.humidity);
}

void STM32WL::init() {
    if (!devicePresent) return;
    update();
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
