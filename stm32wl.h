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
        };
    } i2cRegs;
};

#endif /* STM32WL_H_ */
