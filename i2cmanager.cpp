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

extern "C" {
    void I2C0_IRQHandler(void) {
        I2CManager::instance().I2C0_IRQHandler();
    }
    void PDMA_IRQHandler(void) {
        I2CManager::instance().PDMA_IRQHandler();
    }
}

bool I2CManager::deviceReady(uint8_t _u8PeripheralAddr) {
    if ( I2C_WriteByte(I2C0, _u8PeripheralAddr, 0) == 0 ) {
        return true;
    }
    return false;
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
        if (SDD1306::devicePresent) printf("SDD1306 is ready on reprobe.\n");
    }
    if (!STM32WL::devicePresent) {
        STM32WL::devicePresent = deviceReady(STM32WL::i2c_addr);
        if (STM32WL::devicePresent) printf("STM32WL is ready on reprobe.\n");
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

void I2CManager::prepareBatchWrite() {
    qBufEnd = qBufSeq;
    qBufPtr = qBufSeq;
}

void I2CManager::queueBatchWrite(uint8_t peripheralAddr, uint8_t data[], size_t len) {

    if (!qBufEnd || !qBufPtr) {
        printf("Not in batch mode!\n");
        return;
    }

    if (size_t((qBufEnd + len + 2) - qBufSeq) > sizeof(qBufSeq)) {
        printf("I2C batch buffer too small (wanted at least %d bytes)!\n", ((qBufEnd + len + 2) - qBufSeq));
        return;
    }

    *qBufEnd++ = len;
    *qBufEnd++ = peripheralAddr << 1;
    memcpy(qBufEnd, data, len);
    qBufEnd += len;
}

void I2CManager::I2C0_IRQHandler(void) {
    if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
        I2C_ClearTimeoutFlag(I2C0);
    } else {
        uint32_t u32Status = I2C_GET_STATUS(I2C0);
        if(u32Status == 0x08) {        /* START has been transmitted */
        } else if(u32Status == 0x10) { /* Repeat START has been transmitted */
        } else if(u32Status == 0x18) { /* SLA+W has been transmitted and ACK has been received */
        } else if(u32Status == 0x20) { /* SLA+W has been transmitted and NACK has been received */
            I2C_STOP(I2C0);
            I2C_START(I2C0);
        } else if(u32Status == 0x28) { /* DATA has been transmitted and ACK has been received */
        } else {
        }
    }
}

void I2CManager::PDMA_IRQHandler(void) {
    uint32_t u32Status = PDMA->TDSTS;
    if(u32Status & (0x1 << I2C0_PDMA_TX_CH)) {
        PDMA->TDSTS = 0x1 << I2C0_PDMA_TX_CH;
        pdmaDone = true;
    }
    if(u32Status & (0x1 << 2)) {
        PDMA->TDSTS = 0x1 << 2;
        printf("2!!!\n");
    }
    if(u32Status & (0x1 << 3)) {
        PDMA->TDSTS = 0x1 << 3;
        printf("2!!!\n");
    }
}

void I2CManager::performBatchWrite() {

    if (!qBufEnd || !qBufPtr) {
        printf("Not in batch mode!\n");
        return;
    }

    I2C_EnableInt(I2C0);
    I2C_ENABLE_TX_PDMA(I2C0);
    I2C_DISABLE_RX_PDMA(I2C0);

    for (; qBufPtr < qBufEnd;) {
        pdmaDone = false;
        uint8_t u32wLen = *qBufPtr++;
        PDMA_SetTransferMode(PDMA, I2C0_PDMA_TX_CH, PDMA_I2C0_TX, 0, 0);
        PDMA_SetTransferCnt(PDMA, I2C0_PDMA_TX_CH, PDMA_WIDTH_8, u32wLen + 1);
        PDMA_SetTransferAddr(PDMA, I2C0_PDMA_TX_CH, ((uint32_t) (&qBufPtr[0])), PDMA_SAR_INC, (uint32_t)(&(I2C0->DAT)), PDMA_DAR_FIX);
        I2C_START(I2C0);
        for (; !pdmaDone ;) { __WFI(); }
        for (; (I2C0->STATUS1 & I2C_STATUS1_ONBUSY_Msk) != 0 ;) { }
        qBufPtr += u32wLen + 1;
    }
    
    I2C_DISABLE_TX_PDMA(I2C0);
    I2C_DISABLE_RX_PDMA(I2C0);
    I2C_DisableInt(I2C0);
}


void I2CManager::write(uint8_t _u8PeripheralAddr, uint8_t data[], size_t _u32wLen) {
    I2C_WriteMultiBytes(I2C0, _u8PeripheralAddr, data, _u32wLen);
}

uint8_t I2CManager::read(uint8_t _u8PeripheralAddr, uint8_t rdata[], size_t _u32rLen) {
    return I2C_ReadMultiBytes(I2C0, _u8PeripheralAddr, rdata, _u32rLen);
}

uint8_t I2CManager::getReg8(uint8_t _u8PeripheralAddr, uint8_t _u8DataAddr) {
    return I2C_ReadByteOneReg(I2C0, _u8PeripheralAddr, _u8DataAddr);
}

void I2CManager::setReg8(uint8_t _u8PeripheralAddr, uint8_t _u8DataAddr, uint8_t _u8WData) {
    I2C_WriteByteOneReg(I2C0, _u8PeripheralAddr, _u8DataAddr, _u8WData);
}

void I2CManager::setReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(peripheralAddr, reg);
    value |= mask;
    setReg8(peripheralAddr, reg, value);
}

void I2CManager::clearReg8Bits(uint8_t peripheralAddr, uint8_t reg, uint8_t mask) {
    uint8_t value = getReg8(peripheralAddr, reg);
    value &= ~mask;
    setReg8(peripheralAddr, reg, value);
}

void I2CManager::init() {

    GPIO_SetMode(PA, BIT4, GPIO_MODE_OPEN_DRAIN);
    GPIO_SetMode(PA, BIT5, GPIO_MODE_OPEN_DRAIN);

    PA4 = 1;
    PA5 = 1;

    PA->SMTEN |= GPIO_SMTEN_SMTEN4_Msk | GPIO_SMTEN_SMTEN5_Msk;

    PDMA_Open(PDMA, 1 << I2C0_PDMA_TX_CH);

    PDMA_EnableInt(PDMA, I2C0_PDMA_TX_CH, 0);
    PDMA_SetBurstType(PDMA, I2C0_PDMA_TX_CH, PDMA_REQ_SINGLE, 0);

    I2C_Open(I2C0, 600000);

    uint32_t STCTL = 0;
    uint32_t HTCTL = 2;

    I2C0->TMCTL = ( ( STCTL << I2C_TMCTL_STCTL_Pos) & I2C_TMCTL_STCTL_Msk ) |
                  ( ( HTCTL << I2C_TMCTL_HTCTL_Pos) & I2C_TMCTL_HTCTL_Msk );


    NVIC_SetPriority(PDMA_IRQn, 3);
    NVIC_EnableIRQ(PDMA_IRQn);

    NVIC_SetPriority(I2C0_IRQn, 3);
    NVIC_EnableIRQ(I2C0_IRQn);

    probe();
}
