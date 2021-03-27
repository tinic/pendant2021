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

#define USE_DMA 1

#define SPI0_MASTER_TX_DMA_CH   0
#define SPI1_MASTER_TX_DMA_CH   1
#define USPI0_MASTER_TX_DMA_CH  2 
#define USPI1_MASTER_TX_DMA_CH  3

Leds &Leds::instance() {
    static Leds leds;
    if (!leds.initialized) {
        leds.initialized = true;
        leds.init();
    }
    return leds;
}

void Leds::init() {
    white();

    PF3 = 0;

    SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 8, 8000000);
    SPI_Open(SPI1, SPI_MASTER, SPI_MODE_0, 8, 8000000);
    USPI_Open(USPI0, USPI_MASTER, USPI_MODE_0, 8, 8000000);
    USPI_Open(USPI1, USPI_MASTER, USPI_MODE_0, 8, 8000000);

#ifdef USE_DMA

    PDMA_Open(PDMA,
        (1UL << SPI0_MASTER_TX_DMA_CH)|
        (1UL << SPI1_MASTER_TX_DMA_CH)|
        (1UL << USPI0_MASTER_TX_DMA_CH)|
        (1UL << USPI1_MASTER_TX_DMA_CH));

    PDMA_SetTransferCnt(PDMA, SPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_8, circleLedsDMABuf[0].size());
    PDMA_SetTransferAddr(PDMA, SPI0_MASTER_TX_DMA_CH, (uint32_t)circleLedsDMABuf[0].data(), PDMA_SAR_INC, (uint32_t)&SPI0->TX, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, SPI0_MASTER_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);
    PDMA_SetBurstType(PDMA, SPI0_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    PDMA->DSCT[SPI0_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    PDMA_SetTransferCnt(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_WIDTH_8, circleLedsDMABuf[1].size());
    PDMA_SetTransferAddr(PDMA, SPI1_MASTER_TX_DMA_CH, (uint32_t)circleLedsDMABuf[1].data(), PDMA_SAR_INC, (uint32_t)&SPI1->TX, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);
    PDMA_SetBurstType(PDMA, SPI1_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    PDMA->DSCT[SPI1_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    PDMA_SetTransferCnt(PDMA, USPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_8, birdsLedsDMABuf[0].size());    
    PDMA_SetTransferAddr(PDMA, USPI0_MASTER_TX_DMA_CH, (uint32_t)birdsLedsDMABuf[0].data(), PDMA_SAR_INC, (uint32_t)&USPI0->TXDAT, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, USPI0_MASTER_TX_DMA_CH, PDMA_USCI0_TX, FALSE, 0);    
    PDMA_SetBurstType(PDMA, USPI0_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);   
    PDMA->DSCT[USPI0_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    PDMA_SetTransferCnt(PDMA, USPI1_MASTER_TX_DMA_CH, PDMA_WIDTH_8, birdsLedsDMABuf[1].size());    
    PDMA_SetTransferAddr(PDMA, USPI1_MASTER_TX_DMA_CH, (uint32_t)birdsLedsDMABuf[1].data(), PDMA_SAR_INC, (uint32_t)&USPI1->TXDAT, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, USPI1_MASTER_TX_DMA_CH, PDMA_USCI1_TX, FALSE, 0);    
    PDMA_SetBurstType(PDMA, USPI1_MASTER_TX_DMA_CH, PDMA_REQ_SINGLE, 0);   
    PDMA->DSCT[USPI1_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

#endif  // #ifdef USE_DMA

}

void Leds::prepare() {
    static color::convert converter;

    auto convert_to_one_wire = [] (uint8_t *p, uint16_t v) {
        for (uint32_t c = 0; c < 16; c++) {
            if ( ((1<<(15-c)) & v) != 0 ) {
                *p++ = 0b11110000;
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

    uint8_t *ptr2 = birdsLedsDMABuf[0].data();
    uint8_t *ptr3 = birdsLedsDMABuf[1].data();
    for (size_t c = 0; c < birdLedsN; c++) {
        color::rgba<uint16_t> pixel0(color::rgba<uint16_t>(converter.CIELUV2sRGB(birdLeds[0][c])).fix_for_ws2816());
        color::rgba<uint16_t> pixel1(color::rgba<uint16_t>(converter.CIELUV2sRGB(birdLeds[1][c])).fix_for_ws2816());

        ptr2 = convert_to_one_wire(ptr2, pixel0.g);
        ptr2 = convert_to_one_wire(ptr2, pixel0.r);
        ptr2 = convert_to_one_wire(ptr2, pixel0.b);
        ptr3 = convert_to_one_wire(ptr3, pixel1.g);
        ptr3 = convert_to_one_wire(ptr3, pixel1.r);
        ptr3 = convert_to_one_wire(ptr3, pixel1.b);
    }
}

__attribute__ ((hot, optimize("O3"), flatten))
void Leds::transfer() {
    prepare();

#ifdef USE_DMA

    PDMA_SetTransferCnt(PDMA,SPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_8, circleLedsDMABuf[0].size() + 256);
    PDMA_SetTransferMode(PDMA,SPI0_MASTER_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);
    SPI_TRIGGER_TX_PDMA(SPI0);

    PDMA_SetTransferCnt(PDMA,SPI1_MASTER_TX_DMA_CH, PDMA_WIDTH_8, circleLedsDMABuf[1].size() + 256);
    PDMA_SetTransferMode(PDMA,SPI1_MASTER_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);
    SPI_TRIGGER_TX_PDMA(SPI1);

    PDMA_SetTransferCnt(PDMA,USPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_8, birdsLedsDMABuf[0].size() + 256);    
    PDMA_SetTransferMode(PDMA,USPI0_MASTER_TX_DMA_CH, PDMA_USCI0_TX, FALSE, 0);    
    USPI_TRIGGER_RX_PDMA(USPI0);

    PDMA_SetTransferCnt(PDMA,USPI1_MASTER_TX_DMA_CH, PDMA_WIDTH_8, birdsLedsDMABuf[1].size() + 256);    
    PDMA_SetTransferMode(PDMA,USPI1_MASTER_TX_DMA_CH, PDMA_USCI1_TX, FALSE, 0);    
    USPI_TRIGGER_RX_PDMA(USPI1);

#else  // #ifdef USE_DMA

    for(size_t c = 0; c < circleLedsDMABuf[0].size() + 256; c++) {
        while(SPI_GET_TX_FIFO_FULL_FLAG(SPI0) == 1) {}
        SPI_WRITE_TX(SPI0, circleLedsDMABuf[0].data()[c]);
    }

    for(size_t c = 0; c < circleLedsDMABuf[0].size() + 256; c++) {
        while(SPI_GET_TX_FIFO_FULL_FLAG(SPI1) == 1) {}
        SPI_WRITE_TX(SPI1, circleLedsDMABuf[1].data()[c]);
    }

    for(size_t c = 0; c < birdsLedsDMABuf[0].size() + 256; c++) {
        while(USPI_GET_TX_FULL_FLAG(USPI0) == 1) {}
        USPI_WRITE_TX(USPI0, birdsLedsDMABuf[0].data()[c]);
    }

    for(size_t c = 0; c < birdsLedsDMABuf[0].size() + 256; c++) {
        while(USPI_GET_TX_FULL_FLAG(USPI1) == 1) {}
        USPI_WRITE_TX(USPI1, birdsLedsDMABuf[1].data()[c]);
    }

#endif  // #ifdef USE_DMA
}
