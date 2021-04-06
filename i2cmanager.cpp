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

#include <memory.h>

//#define LOG_TIMEOUT 1

extern "C" {

static enum {
    I2C_None = 0,
    I2C_Write = 1,
    I2C_Read = 2,
    I2C_SetReg8 = 3,
    I2C_GetReg8 = 4,
    I2C_BatchWrite = 5
} I2C_CurrentOperation = I2C_None;

__attribute__ ((optimize("Os"), flatten))
void I2C0_IRQHandler(void)
{
    switch(I2C_CurrentOperation) {
        default:
        case I2C_None:{
            I2C_WAIT_READY(I2C0) { 
                if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
                    I2C_ClearTimeoutFlag(I2C0);
                }
            }
        } break;
        case I2C_Write:{
            I2CManager::instance().writeIRQ();
        } break;
        case I2C_Read:{
            I2CManager::instance().readIRQ();
        } break;
        case I2C_SetReg8:{
            I2CManager::instance().setReg8IRQ();
        } break;
        case I2C_GetReg8:{
            I2CManager::instance().getReg8IRQ();
        } break;
        case I2C_BatchWrite:{
            I2CManager::instance().batchWriteIRQ();
        } break;
    }
}

}

bool I2CManager::deviceReady(uint8_t _u8SlaveAddr) {

    I2C_DisableInt(I2C0);

retry:
    I2C_START(I2C0);                                                    /* Send START */

    uint8_t ctrl = 0u;
    bool done = false;
    bool error = true;

    while(!done)
    {
        I2C_WAIT_READY(I2C0) { 
            if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
                I2C_ClearTimeoutFlag(I2C0);
#ifdef LOG_TIMEOUT
                printf("deviceReady I2C timeout!\n");
#endif  // #ifdef LOG_TIMEOUT
                goto retry;
            }
        }
        uint32_t status = I2C_GET_STATUS(I2C0);
        switch(status) {
        case 0x08u:
            I2C_SET_DATA(I2C0, (uint8_t)((_u8SlaveAddr << 1u) | 0x01u)); /* Write SLA+R to Register I2CDAT */
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

    I2C_EnableInt(I2C0);

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

void I2CManager::batchClear() {

    if (qBufEnd) printf("Fuck0\n");

    while(u8Xfering && (u8Err == 0u)) { __WFI(); }

    qBufEnd = qBufSeq;
    qBufPtr = qBufSeq;
}

void I2CManager::queueBatchWrite(uint8_t slaveAddr, uint8_t data[], size_t len) {
    while(u8Xfering && (u8Err == 0u)) { __WFI(); }

    if (!qBufEnd || !qBufPtr) {
        return;
    }

    *qBufEnd++ = slaveAddr;
    *qBufEnd++ = len;
    memcpy(qBufEnd, data, len);
    qBufEnd += len;
}

void I2CManager::batchWrite() {
    while(u8Xfering && (u8Err == 0u)) { __WFI(); }

    if (!qBufEnd || !qBufPtr) {
        return;
    }

    u8Xfering = 1u;
    u8Err = 0u;
    u8Ctrl = 0u;
    u32txLen = 0u;
    
    u8SlaveAddr = *qBufPtr++;
    u32wLen = *qBufPtr++;

    I2C_CurrentOperation = I2C_BatchWrite;

    I2C_START(I2C0);

    // Do NOT wait for write
    //while(u8Xfering && (u8Err == 0u)) { __WFI(); }
}

__attribute__ ((optimize("Os"), flatten))
void I2CManager::batchWriteIRQ() {

    if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
        I2C_ClearTimeoutFlag(I2C0);
#ifdef LOG_TIMEOUT
        printf("batchWrite I2C timeout %d %d\n", int(u32wLen), int(qBufEnd-qBufPtr));
#endif  // #ifdef LOG_TIMEOUT
        u32txLen = 0;
        I2C_START(I2C0);                                         /* Send START */
        return;
    }

    switch(I2C_GET_STATUS(I2C0))
    {
    case 0x08u:
        I2C_SET_DATA(I2C0, (uint8_t)(u8SlaveAddr << 1u | 0x00u));   /* Write SLA+W to Register I2CDAT */
        u8Ctrl = I2C_CTL_SI;                                        /* Clear SI */
        break;
    case 0x18u:                                                     /* Slave Address ACK */
    case 0x28u:
        if(u32txLen < u32wLen) {
            I2C_SET_DATA(I2C0, qBufPtr[u32txLen++]);                /* Write Data to I2CDAT */
            u8Ctrl = I2C_CTL_SI_AA;                                 /* Clear SI */
        } else {
            u8Ctrl = I2C_CTL_STO_SI;                                /* Clear SI and send STOP */
            u32txLen = 0u;
            qBufPtr += u32wLen;
            u8SlaveAddr = *qBufPtr++;
            u32wLen = *qBufPtr++;
            if (qBufPtr < qBufEnd) {
                I2C_SET_CONTROL_REG(I2C0, u8Ctrl);                  /* Write controlbit to I2C_CTL register */
                I2C_START(I2C0);
                return;
            } else {
                u8Xfering = 0u;
                qBufEnd = 0;
                qBufPtr = 0;
            }
        }
        break;
    case 0x20u:                                                     /* Slave Address NACK */
    case 0x30u:                                                     /* Master transmit data NACK */
        u8Ctrl = I2C_CTL_STO_SI;                                    /* Clear SI and send STOP */
        u8Err = 1u;
        break;
    case 0x38u:                                                     /* Arbitration Lost */
    default:                                                        /* Unknow status */
        u8Ctrl = I2C_CTL_STO_SI;
        u8Err = 1u;
        break;
    }
    I2C_SET_CONTROL_REG(I2C0, u8Ctrl);                              /* Write controlbit to I2C_CTL register */
}

void I2CManager::write(uint8_t _u8SlaveAddr, uint8_t data[], size_t _u32wLen) {

    if (qBufEnd) printf("Fuck1\n");

    // Wait for pending write
    while(u8Xfering && (u8Err == 0u)) { __WFI(); }

    if (_u32wLen > sizeof(txBuf)) {
        return;
    }

    u8Xfering = 1u;
    u8Err = 0u;
    u8Ctrl = 0u;
    u32txLen = 0u;
    u32wLen = _u32wLen;
    u8SlaveAddr = _u8SlaveAddr;

    memcpy(txBuf, data, u32wLen);

    I2C_CurrentOperation = I2C_Write;

    I2C_START(I2C0);                                              /* Send START */

    // Do NOT wait for write
    //while(u8Xfering && (u8Err == 0u)) { __WFI(); }
}

__attribute__ ((optimize("Os"), flatten))
void I2CManager::writeIRQ() {

    if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
        I2C_ClearTimeoutFlag(I2C0);
#ifdef LOG_TIMEOUT
        printf("write I2C timeout %d\n", int(u32wLen));
#endif  // #ifdef LOG_TIMEOUT
        u32txLen = 0; // Start again
        I2C_START(I2C0);                                            /* Send START */
        return;
    }

    switch(I2C_GET_STATUS(I2C0))
    {
    case 0x08u:
        I2C_SET_DATA(I2C0, (uint8_t)(u8SlaveAddr << 1u | 0x00u));   /* Write SLA+W to Register I2CDAT */
        u8Ctrl = I2C_CTL_SI;                                        /* Clear SI */
        break;
    case 0x18u:                                                     /* Slave Address ACK */
    case 0x28u:
        if(u32txLen < u32wLen)
        {
            I2C_SET_DATA(I2C0, txBuf[u32txLen++]);                  /* Write Data to I2CDAT */
            u8Ctrl = I2C_CTL_SI_AA;                                 /* Clear SI */
        }
        else
        {
            u8Ctrl = I2C_CTL_STO_SI;                                /* Clear SI and send STOP */
            u8Xfering = 0u;
        }
        break;
    case 0x20u:                                                     /* Slave Address NACK */
    case 0x30u:                                                     /* Master transmit data NACK */
        u8Ctrl = I2C_CTL_STO_SI;                                    /* Clear SI and send STOP */
        u8Err = 1u;
        break;
    case 0x38u:                                                     /* Arbitration Lost */
    default:                                                        /* Unknow status */
        u8Ctrl = I2C_CTL_STO_SI;
        u8Err = 1u;
        break;
    }
    I2C_SET_CONTROL_REG(I2C0, u8Ctrl);                              /* Write controlbit to I2C_CTL register */
}

uint8_t I2CManager::read(uint8_t _u8SlaveAddr, uint8_t rdata[], size_t _u32rLen) {

    if (qBufEnd) printf("Fuck2\n");

    // Wait for pending write
    while(u8Xfering && (u8Err == 0u)) { __WFI(); }

    if (_u32rLen > sizeof(rxBuf)) {
        return 0;
    }

    u8Xfering = 1u;
    u8Err = 0u;
    u8Ctrl = 0u;
    u32rxLen = 0u;
    u32rLen = _u32rLen;
    u8SlaveAddr = _u8SlaveAddr;

    I2C_CurrentOperation = I2C_Read;

    I2C_START(I2C0);                                                /* Send START */

    // Wait for read
    while(u8Xfering && (u8Err == 0u)) { __WFI(); }

    memcpy(rdata, rxBuf, u32rxLen);

    return u32rxLen;                                                /* Return bytes length that have been received */
}

__attribute__ ((optimize("Os"), flatten))
void I2CManager::readIRQ() {

    if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
        I2C_ClearTimeoutFlag(I2C0);
#ifdef LOG_TIMEOUT
        printf("read I2C timeout!\n");
#endif  // #ifdef LOG_TIMEOUT
        I2C_START(I2C0);
        return;
    }

    switch(I2C_GET_STATUS(I2C0))
    {
    case 0x08u:
        I2C_SET_DATA(I2C0, (uint8_t)((u8SlaveAddr << 1u) | 0x01u));    /* Write SLA+R to Register I2CDAT */
        u8Ctrl = I2C_CTL_SI;                                    /* Clear SI */
        break;
    case 0x40u:                                                 /* Slave Address ACK */
        u8Ctrl = I2C_CTL_SI_AA;                                 /* Clear SI and set ACK */
        break;
    case 0x48u:                                                 /* Slave Address NACK */
        u8Ctrl = I2C_CTL_STO_SI;                                /* Clear SI and send STOP */
        u8Err = 1u;
        break;
    case 0x50u:
        rxBuf[u32rxLen++] = (unsigned char) I2C_GET_DATA(I2C0); /* Receive Data */
        if(u32rxLen < (u32rLen - 1u)) {
            u8Ctrl = I2C_CTL_SI_AA;                             /* Clear SI and set ACK */
        } else {
            u8Ctrl = I2C_CTL_SI;                                /* Clear SI */
        }
        break;
    case 0x58u:
        rxBuf[u32rxLen++] = (unsigned char) I2C_GET_DATA(I2C0); /* Receive Data */
        u8Ctrl = I2C_CTL_STO_SI;                                /* Clear SI and send STOP */
        u8Xfering = 0u;
        break;
    case 0x38u:                                                 /* Arbitration Lost */
    default:                                                    /* Unknow status */
        u8Ctrl = I2C_CTL_STO_SI;
        u8Err = 1u;
        break;
    }
    I2C_SET_CONTROL_REG(I2C0, u8Ctrl);                                 /* Write controlbit to I2C_CTL register */
}


uint8_t I2CManager::getReg8(uint8_t _u8SlaveAddr, uint8_t _u8DataAddr) {

    if (qBufEnd) printf("Fuck3\n");

    // Wait for pending write
    while(u8Xfering && (u8Err == 0u)) { __WFI(); }

    u8Xfering = 1u;
    u8Err = 0u;
    u8RData = 0u; 
    u8Ctrl = 0u;
    u8DataAddr = _u8DataAddr;
    u8SlaveAddr = _u8SlaveAddr;

    I2C_CurrentOperation = I2C_GetReg8;

    I2C_START(I2C0);                                                /* Send START */

    // Wait for read
    while(u8Xfering && (u8Err == 0u)) { __WFI(); }

    return u8RData;
}

__attribute__ ((optimize("Os"), flatten))
void I2CManager::getReg8IRQ() {

    if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
        I2C_ClearTimeoutFlag(I2C0);
#ifdef LOG_TIMEOUT
        printf("getReg8 I2C timeout!\n");
#endif  // #ifdef LOG_TIMEOUT
        I2C_START(I2C0);
        return;
    }

    switch(I2C_GET_STATUS(I2C0))
    {
    case 0x08u:
        I2C_SET_DATA(I2C0, (uint8_t)(u8SlaveAddr << 1u | 0x00u));      /* Write SLA+W to Register I2CDAT */
        u8Ctrl = I2C_CTL_SI;                             /* Clear SI */
        break;
    case 0x18u:                                             /* Slave Address ACK */
        I2C_SET_DATA(I2C0, u8DataAddr);                     /* Write Lo byte address of register */
        break;
    case 0x20u:                                             /* Slave Address NACK */
    case 0x30u:                                             /* Master transmit data NACK */
        u8Ctrl = I2C_CTL_STO_SI;                         /* Clear SI and send STOP */
        u8Err = 1u;
        break;
    case 0x28u:
        u8Ctrl = I2C_CTL_STA_SI;                         /* Send repeat START */
        break;
    case 0x10u:
        I2C_SET_DATA(I2C0, (uint8_t)((u8SlaveAddr << 1u) | 0x01u));    /* Write SLA+R to Register I2CDAT */
        u8Ctrl = I2C_CTL_SI;                               /* Clear SI */
        break;
    case 0x40u:                                             /* Slave Address ACK */
        u8Ctrl = I2C_CTL_SI;                             /* Clear SI */
        break;
    case 0x48u:                                             /* Slave Address NACK */
        u8Ctrl = I2C_CTL_STO_SI;                         /* Clear SI and send STOP */
        u8Err = 1u;
        break;
    case 0x58u:
        u8RData = (uint8_t) I2C_GET_DATA(I2C0);               /* Receive Data */
        u8Ctrl = I2C_CTL_STO_SI;                         /* Clear SI and send STOP */
        u8Xfering = 0u;
        break;
    case 0x38u:                                             /* Arbitration Lost */
    default:                                               /* Unknow status */
        u8Ctrl = I2C_CTL_STO_SI;
        u8Err = 1u;
        break;
    }
    I2C_SET_CONTROL_REG(I2C0, u8Ctrl);                          /* Write controlbit to I2C_CTL register */
}


void I2CManager::setReg8(uint8_t _u8SlaveAddr, uint8_t _u8DataAddr, uint8_t _u8WData) {

    if (qBufEnd) printf("Fuck4\n");

    // Wait for pending write
    while(u8Xfering && (u8Err == 0u)) { __WFI(); }

    u8Xfering = 1u;
    u8Err = 0u;
    u8Ctrl = 0u;
    u32txLen = 0u;
    u8DataAddr = _u8DataAddr;
    u8SlaveAddr = _u8SlaveAddr;
    u8WData = _u8WData; 

    I2C_CurrentOperation = I2C_SetReg8;

    I2C_START(I2C0);                                             /* Send START */

    // Do NOT wait for write
    //while(u8Xfering && (u8Err == 0u)) { __WFI(); }
}

__attribute__ ((optimize("Os"), flatten))
void I2CManager::setReg8IRQ() {

    if (I2C_GET_TIMEOUT_FLAG(I2C0)) {
        I2C_ClearTimeoutFlag(I2C0);
#ifdef LOG_TIMEOUT
        printf("setReg8 I2C timeout!\n");
#endif  // #ifdef LOG_TIMEOUT
        I2C_START(I2C0);
        return;
    }

    switch(I2C_GET_STATUS(I2C0))
    {
    case 0x08u:
        I2C_SET_DATA(I2C0, (uint8_t)(u8SlaveAddr << 1u | 0x00u));    /* Send Slave address with write bit */
        u8Ctrl = I2C_CTL_SI;                           /* Clear SI */
        break;
    case 0x18u:                                           /* Slave Address ACK */
        I2C_SET_DATA(I2C0, u8DataAddr);                   /* Write Lo byte address of register */
        break;
    case 0x20u:                                           /* Slave Address NACK */
    case 0x30u:                                           /* Master transmit data NACK */
        u8Ctrl = I2C_CTL_STO_SI;                       /* Clear SI and send STOP */
        u8Err = 1u;
        break;
    case 0x28u:
        if(u32txLen < 1u)
        {
            I2C_SET_DATA(I2C0, u8WData);
            u8Ctrl = I2C_CTL_SI;
            u32txLen++;
        }
        else
        {
            u8Ctrl = I2C_CTL_STO_SI;                   /* Clear SI and send STOP */
            u8Xfering = 0u;
        }
        break;
    case 0x38u:                                           /* Arbitration Lost */
    default:                                             /* Unknow status */
        u8Ctrl = I2C_CTL_STO_SI;
        u8Err = 1u;
        break;
    }
    I2C_SET_CONTROL_REG(I2C0, u8Ctrl);                        /* Write controlbit to I2C_CTL register */
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

    I2C_Open(I2C0, 400000);
    I2C_EnableTimeout(I2C0, 1);
    I2C_EnableInt(I2C0);

    NVIC_SetPriority(I2C0_IRQn, 3);
    NVIC_EnableIRQ(I2C0_IRQn);

    probe();
}
