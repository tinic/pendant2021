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
#include "./i2cmanager.h"

#include "M480.h"

extern "C" {

typedef void (*I2C_FUNC)(uint32_t u32Status);
static volatile I2C_FUNC s_I2C0HandlerFn = NULL;

void I2C0_IRQHandler(void)
{
    uint32_t u32Status = I2C_GET_STATUS(I2C0);
    if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
        I2C_ClearTimeoutFlag(I2C0);
    } else {
        if (s_I2C0HandlerFn != NULL) {
            s_I2C0HandlerFn(u32Status);
        }
    }
}

}

bool I2CManager::deviceReady(uint8_t u8SlaveAddr) {

    I2C_START(I2C0);                                                    /* Send START */

    uint8_t ctrl = 0u;
    bool done = false;
    bool error = true;

    while(!done)
    {
        I2C_WAIT_READY(I2C0) {}

        uint32_t status = I2C_GET_STATUS(I2C0);
        switch(status) {
        case 0x08u:
            I2C_SET_DATA(I2C0, (uint8_t)((u8SlaveAddr << 1u) | 0x01u)); /* Write SLA+R to Register I2CDAT */
            ctrl = I2C_CTL_SI;                                          /* Clear SI */
            break;
        case 0x40u:                                                     /* Slave Address ACK */
            ctrl = I2C_CTL_SI;                                          /* Clear SI */
            break;
        case 0x58u:                                                     /* Receive Data */
            ctrl = I2C_CTL_STO_SI;                                      /* Clear SI and STOP */
            error = false;
            done = true;
            break;
        default:                                                        /* Unknow status */
            ctrl = I2C_CTL_STO_SI;                                      /* Clear SI and STOP */
            done = true;
            break;
        }

        I2C_SET_CONTROL_REG(I2C0, ctrl);                                /* Write controlbit to I2C_CTL register */
    }

    return !error;
}

I2CManager &I2CManager::instance() {
    static I2CManager i2c;
    if (!i2c.initialized) {
        i2c.initialized = true;
        i2c.init();
    }
    return i2c;
}

void I2CManager::probe() {
    for (size_t c = 0; c <  127; c++) {
        if (deviceReady(c)) {
            printf("I2C device 0x%02x ready.\n", c);
        }
    }
}

uint8_t I2CManager::getReg8(uint8_t slaveAddr, uint8_t reg) {
    return I2C_ReadByteOneReg(I2C0, slaveAddr, reg);
}

void I2CManager::setReg8(uint8_t slaveAddr, uint8_t reg, uint8_t dat) {
    I2C_WriteByteOneReg(I2C0, slaveAddr, reg, dat);
}

void I2CManager::setReg8Bits(uint8_t slaveAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(slaveAddr, reg);
    value |= mask;
    setReg8(slaveAddr, reg, value);
}

void I2CManager::clearReg8Bits(uint8_t slaveAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(slaveAddr, reg);
    value &= ~mask;
    setReg8(slaveAddr, reg, value);
}

void I2CManager::init() {
    NVIC_SetPriority(I2C0_IRQn, 3);

    I2C_Open(I2C0, 100000);

    probe();
}
