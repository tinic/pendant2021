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
#include "./bq25895.h"

#include "./i2cmanager.h"
#include "./input.h"

#include "M480.h"

BQ25895  &BQ25895 ::instance() {
    static BQ25895 bq25895;
    if (!bq25895.initialized) {
        bq25895.initialized = true;
        bq25895.init();
    }
    return bq25895;
}

void BQ25895::DisableWatchdog() {
    I2CManager::instance().clearReg8Bits(bq25895Addr, 0x07, (1 << 4));
    I2CManager::instance().clearReg8Bits(bq25895Addr, 0x07, (1 << 5));
}
     
void BQ25895::DisableOTG() {
    I2CManager::instance().clearReg8Bits(bq25895Addr, 0x03, (1 << 5));
}

float BQ25895::ReadBatteryVoltage() {
    uint8_t reg = I2CManager::instance().getReg8(bq25895Addr,0x0E) & 0x7F;
    return 2.304f + ( static_cast<float>(reg) * 2.540f ) * ( 1.0f / 127.0f);
}

float BQ25895::ReadSystemVoltage() {
    uint8_t reg = I2CManager::instance().getReg8(bq25895Addr,0x0F) & 0x7F;
    return 2.304f + ( static_cast<float>(reg) * 2.540f ) * ( 1.0f / 127.0f);
}

float BQ25895::ReadVBUSVoltage() {
    uint8_t reg = I2CManager::instance().getReg8(bq25895Addr,0x11) & 0x7F;
    return 2.6f + ( static_cast<float>(reg) * 12.7f ) * ( 1.0f / 127.0f);
}

float BQ25895::ReadChargeCurrent() {
    uint8_t reg = I2CManager::instance().getReg8(bq25895Addr,0x12) & 0x7F;
    return ( static_cast<float>(reg) * 6350.0f ) * ( 1.0f / 127.0f);
}

void BQ25895::UpdateState() {
    status = I2CManager::instance().getReg8(bq25895Addr, 0x0B);
    faultState = I2CManager::instance().getReg8(bq25895Addr, 0x0C);
    batteryVoltage = ReadBatteryVoltage();
    systemVoltage = ReadSystemVoltage();
    vbusVoltage = ReadVBUSVoltage();
    chargeCurrent = ReadChargeCurrent();
    OneShotADC();
}

void BQ25895::SetBoostVoltage (uint32_t voltageMV) {
    uint8_t reg =I2CManager::instance().getReg8(bq25895Addr, 0x06);
    if ((voltageMV >= 4550) && (voltageMV <= 5510)) {
        uint32_t codedValue = voltageMV;
        codedValue = (codedValue - 4550) / 64;
        if ((voltageMV - 4550) % 64 != 0) {
            codedValue++;
        }
        reg &= ~(0x0f << 4);
        reg |= static_cast<uint8_t>((codedValue & 0x0f) << 4);
        I2CManager::instance().setReg8(bq25895Addr, 0x0A, reg);
    }
}

uint32_t BQ25895::GetBoostVoltage() {
    uint8_t reg = I2CManager::instance().getReg8(bq25895Addr, 0x0A);
    reg = (reg >> 4) & 0x0f;
    return 4550 + (static_cast<uint32_t>(reg)) * 64;
}

void BQ25895::SetInputCurrent(uint32_t currentMA) {
    if (currentMA >= 50 && currentMA <= 3250) {
        uint32_t codedValue = currentMA;
        codedValue = ((codedValue) / 50) - 1;
        codedValue |= (1 << 6);
        I2CManager::instance().setReg8(bq25895Addr, 0x00, static_cast<uint8_t>(codedValue));
    }
    if (currentMA == 0) {
        uint32_t codedValue = 0;
        codedValue |= (1 << 6);
        I2CManager::instance().setReg8(bq25895Addr,0x00, static_cast<uint8_t>(codedValue));
    }
}
     
bool BQ25895::ADCActive() {
    return ( I2CManager::instance().getReg8(bq25895Addr, 0x02) & (1 << 7) ) ? true : false;
}

void BQ25895::OneShotADC() {
    I2CManager::instance().clearReg8Bits(bq25895Addr, 0x02, (1 << 6));
    I2CManager::instance().setReg8Bits(bq25895Addr, 0x02, (1 << 7));
}

uint32_t BQ25895::GetInputCurrent () {
    return ((I2CManager::instance().getReg8(bq25895Addr, 0x00) & (0x3F)) * 50);
}

void BQ25895::stats() {
    printf("Battery Voltage: %g\n", double(BatteryVoltage()));
    printf("System Voltage: %g\n", double(SystemVoltage()));
    printf("VBUSVoltage Voltage: %g\n", double(VBUSVoltage()));
    printf("Charge Current: %g\n", double(ChargeCurrent()));
}

void BQ25895::init() {
    DisableWatchdog();
    DisableOTG();
    OneShotADC();
    SetBoostVoltage(4550);
    SetInputCurrent(500);
    UpdateState();
    stats();
} 