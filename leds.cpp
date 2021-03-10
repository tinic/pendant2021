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
#include "M480.h"

#include "./leds.h"
#include "./color.h"

#define SPI0_MASTER_TX_DMA_CH   0
#define SPI1_MASTER_TX_DMA_CH   1
#define USPI0_MASTER_TX_DMA_CH  2 

Leds &Leds::instance() {
    static Leds leds;
    if (!leds.initialized) {
        leds.initialized = true;
        leds.init();
    }
    return leds;
}

void Leds::init() {
    SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 32, 12000000);
    SPI_Open(SPI1, SPI_MASTER, SPI_MODE_0, 32, 12000000);
    USPI_Open(USPI0, USPI_MASTER, USPI_MODE_0, 16, 12000000);

    PDMA_Open(PDMA,
        (1UL << SPI0_MASTER_TX_DMA_CH)||
        (1UL << SPI1_MASTER_TX_DMA_CH)||
        (1UL << USPI0_MASTER_TX_DMA_CH));

    PDMA_SetTransferCnt(PDMA, SPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_32, circleLedsDMABuf[0].size());
    PDMA_SetTransferAddr(PDMA, SPI0_MASTER_TX_DMA_CH, (uint32_t)circleLedsDMABuf[0].data(), PDMA_SAR_INC, (uint32_t)&SPI0->TX, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, SPI0_MASTER_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);
    PDMA_SetBurstType(PDMA, SPI0_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    PDMA->DSCT[SPI0_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    PDMA_SetTransferCnt(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_WIDTH_32, circleLedsDMABuf[1].size());
    PDMA_SetTransferAddr(PDMA, SPI1_MASTER_TX_DMA_CH, (uint32_t)circleLedsDMABuf[1].data(), PDMA_SAR_INC, (uint32_t)&SPI1->TX, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);
    PDMA_SetBurstType(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    PDMA->DSCT[SPI1_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    PDMA_SetTransferCnt(PDMA, USPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_16, birdsLedsDMABuf.size());    
    PDMA_SetTransferAddr(PDMA, USPI0_MASTER_TX_DMA_CH, (uint32_t)birdsLedsDMABuf.data(), PDMA_SAR_INC, (uint32_t)&USPI0->TXDAT, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, USPI0_MASTER_TX_DMA_CH, PDMA_USCI0_TX, FALSE, 0);    
    PDMA_SetBurstType(PDMA, USPI0_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);   
    PDMA->DSCT[USPI0_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;
}

void Leds::prepare() {
    static color::convert converter;

    auto convert_to_one_wire = [] (uint8_t *p, uint16_t v) {
        for (uint32_t c = 0; c < 16; c++) {
            if ( ((1<<(15-c)) & v) != 0 ) {
                *p++ = 0b11111000;
            } else {
                *p++ = 0b10000000;
            }
        }
        return p;
    };

    uint8_t *ptr0 = circleLedsDMABuf[0].data();
    uint8_t *ptr1 = circleLedsDMABuf[1].data();
    for (size_t c = 0; c < circleLedsN; c++) {
        color::rgba<uint16_t> pixel0(color::rgba<uint16_t>(converter.CIELUV2sRGB(circleLeds[0][c])).fix_for_ws2816());
        color::rgba<uint16_t> pixel1(color::rgba<uint16_t>(converter.CIELUV2sRGB(circleLeds[1][c])).fix_for_ws2816());

        ptr0 = convert_to_one_wire(ptr0, pixel0.g);
        ptr0 = convert_to_one_wire(ptr0, pixel0.r);
        ptr0 = convert_to_one_wire(ptr0, pixel0.b);
        ptr1 = convert_to_one_wire(ptr1, pixel1.g);
        ptr1 = convert_to_one_wire(ptr1, pixel1.r);
        ptr1 = convert_to_one_wire(ptr1, pixel1.b);
    }

    auto convert_two_to_one_wire = [] (uint16_t *p, uint16_t v0, uint16_t v1) {
        for (uint32_t c = 0; c < 16; c++) {
            uint16_t b0 = 0;
            if ( ((1<<(15-c)) & v0) != 0 ) {
                b0 = 0b0101010101000000;
            } else {
                b0 = 0b0100000000000000;
            }
            uint16_t b1 = 0;
            if ( ((1<<(15-c)) & v1) != 0 ) {
                b1 = 0b1010101010000000;
            } else {
                b1 = 0b1000000000000000;
            }
            *p++ = b0 | b1;
        }
        return p;
    };

    uint16_t *ptr2 = reinterpret_cast<uint16_t *>(birdsLedsDMABuf.data());
    for (size_t c = 0; c < birdLedsN; c++) {
        color::rgba<uint16_t> pixel2(color::rgba<uint16_t>(converter.CIELUV2sRGB(birdLeds[0][c])).fix_for_ws2816());
        color::rgba<uint16_t> pixel3(color::rgba<uint16_t>(converter.CIELUV2sRGB(birdLeds[1][c])).fix_for_ws2816());

        ptr2 = convert_two_to_one_wire(ptr2, pixel2.g, pixel3.g);
        ptr2 = convert_two_to_one_wire(ptr2, pixel2.r, pixel3.r);
        ptr2 = convert_two_to_one_wire(ptr2, pixel2.b, pixel3.b);
    }
}

void Leds::transfer() {

    prepare();

    PDMA_SetTransferCnt(PDMA,SPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_32, circleLedsDMABuf[0].size() / sizeof(uint32_t));
    PDMA_SetTransferMode(PDMA,SPI0_MASTER_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);
    SPI_TRIGGER_TX_PDMA(SPI0);

    PDMA_SetTransferCnt(PDMA,SPI1_MASTER_TX_DMA_CH, PDMA_WIDTH_32, circleLedsDMABuf[1].size() / sizeof(uint32_t));
    PDMA_SetTransferMode(PDMA,SPI1_MASTER_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);
    SPI_TRIGGER_TX_PDMA(SPI1);

    PDMA_SetTransferCnt(PDMA,USPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_16, birdsLedsDMABuf.size() / sizeof(uint16_t));    
    PDMA_SetTransferMode(PDMA,USPI0_MASTER_TX_DMA_CH, PDMA_USCI0_TX, FALSE, 0);    
    USPI_TRIGGER_RX_PDMA(USPI0);

}
