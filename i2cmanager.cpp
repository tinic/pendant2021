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
#include "./stm32wl.h"
#include "./sdd1306.h"
#include "./timeline.h"

#include "M480.h"

#include <memory.h>

#define I2C0_PDMA_TX_CH     2
#define I2C0_PDMA_RX_CH     3

extern "C" {

static void nullIRQHandler(void) {
    I2C_GET_STATUS(I2C0);
    if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
        I2C_ClearTimeoutFlag(I2C0);
    }
}

typedef void (*I2C_FUNC)(void);
static I2C_FUNC s_I2C0HandlerFn = nullIRQHandler;

void I2C0_IRQHandler(void)
{
    s_I2C0HandlerFn();
}

}

I2CManager &I2CManager::instance() {
    static I2CManager i2c;
    if (!i2c.initialized) {
        i2c.initialized = true;
        i2c.init();
    }
    return i2c;
}

void I2CManager::reprobeCritial() {
    if (!SDD1306::devicePresent) {
        SDD1306::devicePresent = deviceReady(SDD1306::i2c_addr);
        printf("SDD1306 is ready on reprobe.\n");
    }
    if (!STM32WL::devicePresent) {
        STM32WL::devicePresent = deviceReady(STM32WL::i2c_addr);
        printf("STM32WL is ready on reprobe.\n");
    }
}

void I2CManager::probe() {
    SDD1306::InitPins();
    for (size_t c = 0; c <  127; c++) {
        if (deviceReady(c)) {
            switch(c) {
                case SDD1306::i2c_addr: {
                    SDD1306::devicePresent = true;
                    printf("SDD1306 is ready.\n");
                } break;
                case STM32WL::i2c_addr: {
                    STM32WL::devicePresent = true;
                    printf("STM32WL is ready.\n");
                } break;
            default: {
                    printf("Unknown I2C device 0x%02x is ready.\n", c);
                } break;
            }
        }
    }
}

bool I2CManager::deviceReady(uint8_t _u8PeripheralAddr) {
    return false;
}

void I2CManager::write(uint8_t _u8PeripheralAddr, uint8_t data[], size_t _u32wLen) {
    s_I2C0HandlerFn = writeIRQHandler;
}

void I2CManager::writeIRQHandler(void) {
    I2CManager::instance().writeIRQ();
}

void I2CManager::writeIRQ() {
}

uint8_t I2CManager::read(uint8_t _u8PeripheralAddr, uint8_t rdata[], size_t _u32rLen) {
    s_I2C0HandlerFn = readIRQHandler;
    return 0;
}

void I2CManager::readIRQHandler(void) {
    I2CManager::instance().readIRQ();
}

void I2CManager::readIRQ() {
}

uint8_t I2CManager::getReg8(uint8_t _u8PeripheralAddr, uint8_t _u8DataAddr) {
    return 0;
}

void I2CManager::setReg8(uint8_t _u8PeripheralAddr, uint8_t _u8DataAddr, uint8_t _u8WData) {
}

void I2CManager::init() {
    GPIO_SetMode(PA, BIT4, GPIO_MODE_OPEN_DRAIN);
    GPIO_SetMode(PA, BIT5, GPIO_MODE_OPEN_DRAIN);
    PA4 = 1;
    PA5 = 1;

    PDMA_Open(PDMA, 1 << I2C0_PDMA_TX_CH);
    PDMA_Open(PDMA, 1 << I2C0_PDMA_RX_CH);
    PDMA_SetTransferMode(PDMA, I2C0_PDMA_TX_CH, PDMA_I2C0_TX, 0, 0);
    PDMA_SetTransferMode(PDMA, I2C0_PDMA_RX_CH, PDMA_I2C0_RX, 0, 0);
    PDMA_SetBurstType(PDMA, I2C0_PDMA_TX_CH, PDMA_REQ_SINGLE, 0);
    PDMA_SetBurstType(PDMA, I2C0_PDMA_RX_CH, PDMA_REQ_SINGLE, 0);
    PDMA_EnableInt(PDMA, I2C0_PDMA_TX_CH, 0);
    PDMA_EnableInt(PDMA, I2C0_PDMA_RX_CH, 0);
    NVIC_SetPriority(PDMA_IRQn, 3);
    NVIC_EnableIRQ(PDMA_IRQn);

    I2C_Open(I2C0, 400000);

    uint32_t STCTL = 0;
    uint32_t HTCTL = 2;

    I2C0->TMCTL = ( ( STCTL << I2C_TMCTL_STCTL_Pos) & I2C_TMCTL_STCTL_Msk ) |
                  ( ( HTCTL << I2C_TMCTL_HTCTL_Pos) & I2C_TMCTL_HTCTL_Msk );

    I2C_EnableInt(I2C0);
    NVIC_SetPriority(I2C0_IRQn, 3);
    NVIC_EnableIRQ(I2C0_IRQn);

    probe();
}
