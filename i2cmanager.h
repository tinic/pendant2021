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
#ifndef I2CMANAGER_H_
#define I2CMANAGER_H_

#include "./color.h"

class I2CManager {
public:
    static I2CManager &instance();

    void prepareBatchWrite();
    void queueBatchWrite(uint8_t slaveAddr, uint8_t data[], size_t len);
    void performBatchWrite();

    bool inBatchWrite() const { return u8Xfering && (u8Err == 0u) && qBufPtr; }

    void write(uint8_t slaveAddr, uint8_t data[], size_t len);
    uint8_t read(uint8_t slaveAddr, uint8_t data[], size_t len);

    void setReg8(uint8_t slaveAddr, uint8_t reg, uint8_t dat);
    uint8_t getReg8(uint8_t slaveAddr, uint8_t reg);

    void setReg8Bits(uint8_t slaveAddr, uint8_t reg, uint8_t mask);
    void clearReg8Bits(uint8_t slaveAddr, uint8_t reg, uint8_t mask);

    bool error() const { return u8Err ? true : false; }

private:
    static void batchWriteIRQHandler(void);
    static void writeIRQHandler(void);
    static void readIRQHandler(void);
    static void setReg8IRQHandler(void);
    static void getReg8IRQHandler(void);

    void batchWriteIRQ();
    void writeIRQ();
    void readIRQ();
    void setReg8IRQ();
    void getReg8IRQ();

    bool deviceReady(uint8_t u8SlaveAddr);
    void probe();
    void init();
    void waitForFinish();

    bool initialized = false;

    uint8_t u8SlaveAddr = 0u;
    uint8_t u8Xfering = 0u;
    uint8_t u8Err = 0u;
    uint8_t u8Ctrl = 0u;
    uint8_t u8DataAddr = 0u;
    uint8_t u8RData = 0u;
    uint8_t u8WData = 0u;
    uint32_t u32txLen = 0u;
    uint32_t u32rxLen = 0u;
    uint32_t u32wLen = 0u;
    uint32_t u32rLen = 0u;
    uint8_t txBuf[256];
    uint8_t rxBuf[256];
    uint8_t qBufSeq[2048];
    uint8_t *qBufPtr = 0;
    uint8_t *qBufEnd = 0;

};

#endif /* I2CMANAGER_H_ */
