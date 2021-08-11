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

    static float u82f(uint8_t v, float min, float max) {
        return ((float(v) * ( 1.0f / 255.0f ) * (max - min) ) + min);
    }

    float BatteryVoltage() const { return u82f(i2cRegs.batteryVoltage, 2.7f, 4.2f); }
    float SystemVoltage() const { return u82f(i2cRegs.systemVoltage, 2.7f, 4.2f); }
    float VBUSVoltage() const { return u82f(i2cRegs.vbusVoltage, 0.0f, 5.5f); }
    float ChargeCurrent() const { return u82f(i2cRegs.chargeCurrent, 0.0f, 1000.0f); }
    float Temperature() const { return u82f(i2cRegs.temperature, 0.0f, 50.0f); }
    float Humidity() const { return u82f(i2cRegs.humidity, 0.0f, 1.0f); }

private:
    friend class I2CManager;
    static constexpr uint8_t i2c_addr = 0x33;
    static bool devicePresent;

    void init();
    bool initialized = false;

    union I2CRegs {
        uint8_t regs[256];
        struct  __attribute__ ((__packed__)) {
            uint8_t devEUI[8];
            uint8_t joinEUI[8];
            uint8_t appKey[16];

            uint16_t systemTime;
            uint8_t status;
            uint8_t effectN;
            uint8_t brightness;
            uint8_t batteryVoltage;
            uint8_t systemVoltage;
            uint8_t vbusVoltage;
            uint8_t chargeCurrent;
            uint8_t temperature;
            uint8_t humidity;

            uint8_t ring_color[4];
            uint8_t bird_color[4];

            uint16_t switch1Count;
            uint16_t switch2Count;
            uint16_t switch3Count;
            uint16_t bootCount;
            uint16_t intCount;
            uint16_t dselCount;
        };
    } i2cRegs;
};

#endif /* STM32WL_H_ */
