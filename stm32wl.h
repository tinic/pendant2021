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
#ifndef STM32WL_H_
#define STM32WL_H_

#include <stdint.h>


class STM32WL {
public:
    static STM32WL &instance();

    void update();

    float BatteryVoltage() const { return 2.304f + ( static_cast<float>(i2cRegs.fields.bq25895BatteryVoltage) * 2.540f ) * ( 1.0f / 127.0f); }
    float SystemVoltage() const { return 2.304f + ( static_cast<float>(i2cRegs.fields.bq25895SystemVoltage) * 2.540f ) * ( 1.0f / 127.0f); }
    float VBUSVoltage() const { return 2.6f + ( static_cast<float>(i2cRegs.fields.bq25895VbusVoltage) * 12.7f ) * ( 1.0f / 127.0f); }
    float ChargeCurrent() const { return ( static_cast<float>(i2cRegs.fields.bq25895ChargeCurrent) * 6350.0f ) * ( 1.0f / 127.0f); }
    float Temperature() const { return (float(i2cRegs.fields.ens210Tmp) / 64.f) - 273.15f; }
    float Humidity() const { return (float(i2cRegs.fields.ens210Hmd) / 51200.0f); }
    uint16_t SystemTime() const { return i2cRegs.fields.systemTime; }
    uint32_t DateTime() const { return i2cRegs.fields.rtcDateTime; }

private:
    friend class I2CManager;
    static constexpr uint8_t i2c_addr = 0x33;
    static bool devicePresent;

    void init();
    bool initialized = false;

    uint8_t i2cReg;
    union I2CRegs {
        uint8_t regs[256];
        struct  __attribute__ ((__packed__)) {
            // persistent, controller
            uint8_t effectN;
            uint8_t brightness;

            uint8_t ring_color[4];
            uint8_t bird_color[4];

            uint16_t switch1Count;
            uint16_t switch2Count;
            uint16_t switch3Count;
            uint16_t bootCount;
            uint16_t dselCount;

            // persistent, peripheral
            uint16_t intCount;

            // dynamic only, controller
            uint16_t systemTime;
            uint8_t status;

            // dynamic only, peripheral
            uint8_t devEUI[8];
            uint8_t joinEUI[8];
            uint8_t appKey[16];

            uint8_t bq25895Status;
            uint8_t bq25895FaulState;
            uint8_t bq25895BatteryVoltage;
            uint8_t bq25895SystemVoltage;
            uint8_t bq25895VbusVoltage;
            uint8_t bq25895ChargeCurrent;

            uint16_t ens210Tmp;
            uint16_t ens210Hmd;

            uint16_t lsm6dsmXG;
            uint16_t lsm6dsmYG;
            uint16_t lsm6dsmZG;
            uint16_t lsm6dsmXA;
            uint16_t lsm6dsmYA;
            uint16_t lsm6dsmZA;
            uint16_t lsm6dsmTmp;

            uint16_t mmc5633njlXG;
            uint16_t mmc5633njlYG;
            uint16_t mmc5633njlZG;
            uint8_t  mmc5633njlTmp;

            uint32_t rtcDateTime;
        } fields;
    } i2cRegs;
};

#endif /* STM32WL_H_ */
