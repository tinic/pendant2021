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

#include "./main.h"
#include "./leds.h"
#include "./color.h"
#include "./model.h"

#include <memory.h>

#define SPI0_MASTER_TX_DMA_CH   0
#define SPI1_MASTER_TX_DMA_CH   1
#define EPWM1_TX_DMA_CH         2
#define EPWM0_TX_DMA_CH         3

extern "C" 
{

#ifndef USE_PWM_DMA
static uint8_t *pwm0Buf = 0;
static uint8_t *pwm0BufEnd = 0;

__attribute__ ((optimize("Os"), flatten))
void EPWM0P1_IRQHandler(void) {
    static volatile uint32_t *cmr = &EPWM0->CMPDAT[3];
    static volatile uint32_t *intsts0 = &EPWM0->INTSTS0;
    static volatile uint32_t *cnten0 = &EPWM0->CNTEN;
    static volatile uint32_t *cnten1 = &EPWM1->CNTEN;
    static volatile uint32_t *gpb_mfpl = &SYS->GPB_MFPL;
    // Set new interval within fewest cycles possible in interrupt
    *cmr = *pwm0Buf;
    // Clear zero interrupt flag
    *intsts0 = ((1UL << EPWM_INTEN0_ZIEN0_Pos) << 3);
    // Now check if we are at the end
    if (++pwm0Buf >= pwm0BufEnd) {
        // Stop Top
        *cnten0 &= ~EPWM_CH_3_MASK;
        // Pull line down by setting back to GPIO
        *gpb_mfpl = (*gpb_mfpl & ~(SYS_GPB_MFPL_PB2MFP_Msk)) | (SYS_GPB_MFPL_PB2MFP_GPIO);
        // Enable Bottom
        *cnten1 |= EPWM_CH_2_MASK;
    }
}

static uint8_t *pwm1Buf = 0;
static uint8_t *pwm1BufEnd = 0;

__attribute__ ((optimize("Os"), flatten))
void EPWM1P1_IRQHandler(void) {
    static volatile uint32_t *cmr = &EPWM1->CMPDAT[2];
    static volatile uint32_t *intsts0 = &EPWM1->INTSTS0;
    static volatile uint32_t *cnten = &EPWM1->CNTEN;
    static volatile uint32_t *gpb_mfpl = &SYS->GPB_MFPL;
    // Set new interval within fewest cycles possible in interrupt
    *cmr = *pwm1Buf;
    // Clear zero interrupt flag
    *intsts0 = ((1UL << EPWM_INTEN0_ZIEN0_Pos) << 2);
    // Now check if we are at the end
    if (++pwm1Buf >= pwm1BufEnd) {
        // Stop Bottom
        *cnten &= ~EPWM_CH_2_MASK;
        // Pull line down by setting back to GPIO
        *gpb_mfpl = (*gpb_mfpl & ~(SYS_GPB_MFPH_PB13MFP_Msk)) | (SYS_GPB_MFPH_PB13MFP_GPIO);
    }
}
#endif  // #ifndef USE_DMA

}

Leds &Leds::instance() {
    static Leds leds;
    if (!leds.initialized) {
        leds.initialized = true;
        leds.init();
    }
    return leds;
}

struct Leds::Map Leds::map;

const Leds::lut_table Leds::lut;

void Leds::init() {

    // LED_ON
    GPIO_SetMode(PF, BIT3, GPIO_MODE_OUTPUT);

    half();

    // Turn Mosfet on
    PF3 = 0;

    SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 8, 4000000);
    SPI_Open(SPI1, SPI_MASTER, SPI_MODE_0, 8, 4000000);

    GPIO_SetMode(PB, BIT2, GPIO_MODE_OUTPUT);
    PB2 = 0;

    GPIO_SetMode(PB, BIT13, GPIO_MODE_OUTPUT);
    PB13 = 0;

#ifdef USE_PWM

    // TOP_LED_BIRD
    CLK_SetModuleClock(EPWM0_MODULE, CLK_CLKSEL2_EPWM0SEL_PLL, 0); // 96Mhz
    CLK_EnableModuleClock(EPWM0_MODULE);

    // BOTTOM_LED_BIRD
    CLK_SetModuleClock(EPWM1_MODULE, CLK_CLKSEL2_EPWM1SEL_PLL, 0); // 96Mhz
    CLK_EnableModuleClock(EPWM1_MODULE);

#endif // #ifdef USE_PWM

#ifdef USE_SPI_DMA

    PDMA_Open(PDMA,(1UL << SPI0_MASTER_TX_DMA_CH)|(1UL << SPI1_MASTER_TX_DMA_CH));

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

#endif  // #ifdef USE_SPI_DMA

#ifdef USE_PWM_DMA

    PDMA_Open(PDMA,(1UL << EPWM0_TX_DMA_CH)|(1UL << EPWM1_TX_DMA_CH));

    PDMA_SetTransferCnt(PDMA, EPWM0_TX_DMA_CH, PDMA_WIDTH_8, birdsLedsDMABuf[0].size());
    PDMA_SetTransferAddr(PDMA, EPWM0_TX_DMA_CH, (uint32_t)birdsLedsDMABuf[0].data(), PDMA_SAR_INC, (uint32_t)&EPWM0->CMPDAT[3], PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, EPWM0_TX_DMA_CH, PDMA_EPWM0_CH3_TX, FALSE, 0);
    PDMA_SetBurstType(PDMA, EPWM0_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    PDMA->DSCT[EPWM0_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    PDMA_SetTransferCnt(PDMA, EPWM1_TX_DMA_CH, PDMA_WIDTH_8, birdsLedsDMABuf[1].size());
    PDMA_SetTransferAddr(PDMA, EPWM1_TX_DMA_CH, (uint32_t)birdsLedsDMABuf[1].data(), PDMA_SAR_INC, (uint32_t)&EPWM1->CMPDAT[2], PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA, EPWM1_TX_DMA_CH, PDMA_EPWM1_CH2_TX, FALSE, 0);
    PDMA_SetBurstType(PDMA, EPWM1_TX_DMA_CH, PDMA_REQ_SINGLE, 0);
    PDMA->DSCT[EPWM1_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

#endif // #ifdef USE_PWM_DMA

}

void Leds::prepare() {
    static color::convert converter;

    float brightness = Model::instance().Brightness();

    uint32_t *ptr0 = reinterpret_cast<uint32_t *>(circleLedsDMABuf[0].data());
    uint32_t *ptr1 = reinterpret_cast<uint32_t *>(circleLedsDMABuf[1].data());
    for (size_t c = 0; c < circleLedsN; c++) {
        color::rgba<uint16_t> pixel0(color::rgba<uint16_t>(converter.CIELUV2sRGB(circleLeds[0][c]*brightness)).fix_for_ws2816());
        color::rgba<uint16_t> pixel1(color::rgba<uint16_t>(converter.CIELUV2sRGB(circleLeds[1][c]*brightness)).fix_for_ws2816());

        auto convert_to_one_wire_spi = [] (uint32_t *p, uint16_t v) {
            *p++ = lut[(v>>8)&0xFF];
            *p++ = lut[(v>>0)&0xFF];
            return p;
        };

        ptr0 = convert_to_one_wire_spi(ptr0, pixel0.g);
        ptr0 = convert_to_one_wire_spi(ptr0, pixel0.r);
        ptr0 = convert_to_one_wire_spi(ptr0, pixel0.b);
        ptr1 = convert_to_one_wire_spi(ptr1, pixel1.g);
        ptr1 = convert_to_one_wire_spi(ptr1, pixel1.r);
        ptr1 = convert_to_one_wire_spi(ptr1, pixel1.b);

    }

#ifdef USE_PWM
    uint8_t *ptr2 = birdsLedsDMABuf[0].data();
    uint8_t *ptr3 = birdsLedsDMABuf[1].data();
#else  // #ifdef USE_PWM
    uint32_t *ptr2 = reinterpret_cast<uint32_t *>(birdsLedsDMABuf[0].data());
    uint32_t *ptr3 = reinterpret_cast<uint32_t *>(birdsLedsDMABuf[1].data());
#endif  // #ifdef USE_PWM
    for (size_t c = 0; c < birdLedsN; c++) {
        color::rgba<uint16_t> pixel0(color::rgba<uint16_t>(converter.CIELUV2sRGB(birdLeds[0][c]*brightness)).fix_for_ws2816());
        color::rgba<uint16_t> pixel1(color::rgba<uint16_t>(converter.CIELUV2sRGB(birdLeds[1][c]*brightness)).fix_for_ws2816());

#ifdef USE_PWM
        auto convert_to_one_wire_pwm = [] (uint8_t *p, uint16_t v) {
            for (uint32_t b = 0; b < 16; b++) {
                if ( ((1<<(15-b)) & v) != 0 ) {
                    *p++ = 0x40;
                } else {
                    *p++ = 0x20;
                }
            }
            return p;
        };

        ptr2 = convert_to_one_wire_pwm(ptr2, pixel0.g);
        ptr2 = convert_to_one_wire_pwm(ptr2, pixel0.r);
        ptr2 = convert_to_one_wire_pwm(ptr2, pixel0.b);
        ptr3 = convert_to_one_wire_pwm(ptr3, pixel1.g);
        ptr3 = convert_to_one_wire_pwm(ptr3, pixel1.r);
        ptr3 = convert_to_one_wire_pwm(ptr3, pixel1.b);
#else  // #ifdef USE_PWM
        auto convert_to_one_wire_spi = [] (uint32_t *p, uint16_t v) {
            *p++ = lut[(v>>8)&0xFF];
            *p++ = lut[(v>>0)&0xFF];
            return p;
        };

        ptr2 = convert_to_one_wire_spi(ptr2, pixel0.g);
        ptr2 = convert_to_one_wire_spi(ptr2, pixel0.r);
        ptr2 = convert_to_one_wire_spi(ptr2, pixel0.b);
        ptr3 = convert_to_one_wire_spi(ptr3, pixel1.g);
        ptr3 = convert_to_one_wire_spi(ptr3, pixel1.r);
        ptr3 = convert_to_one_wire_spi(ptr3, pixel1.b);
#endif  // #ifdef USE_PWM
    }

#ifdef USE_PWM
    for (size_t c = 0; c < extraBirdPadding; c++) {
        *ptr2++ = 0;
        *ptr3++ = 0;
    }
#endif  // #ifdef USE_PWM
}

void Leds::forceStop() {
    EPWM_ForceStop(EPWM0, EPWM_CH_3_MASK);
    EPWM_ForceStop(EPWM1, EPWM_CH_2_MASK);
}

__attribute__ ((hot, optimize("Os"), flatten))
void Leds::transfer() {
    prepare();

#ifdef USE_SPI_DMA

    PDMA_SetTransferCnt(PDMA,SPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_8, circleLedsDMABuf[0].size());
    PDMA_SetTransferMode(PDMA,SPI0_MASTER_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);
    SPI_TRIGGER_TX_PDMA(SPI0);

    PDMA_SetTransferCnt(PDMA,SPI1_MASTER_TX_DMA_CH, PDMA_WIDTH_8, circleLedsDMABuf[1].size());
    PDMA_SetTransferMode(PDMA,SPI1_MASTER_TX_DMA_CH, PDMA_SPI1_TX, FALSE, 0);
    SPI_TRIGGER_TX_PDMA(SPI1);

#else  // #ifdef USE_DMA

    for(size_t c = 0; c < circleLedsDMABuf[0].size(); c++) {
        while(SPI_GET_TX_FIFO_FULL_FLAG(SPI0) == 1) {}
        SPI_WRITE_TX(SPI0, circleLedsDMABuf[0].data()[c]);
    }

    for(size_t c = 0; c < circleLedsDMABuf[1].size(); c++) {
        while(SPI_GET_TX_FIFO_FULL_FLAG(SPI1) == 1) {}
        SPI_WRITE_TX(SPI1, circleLedsDMABuf[1].data()[c]);
    }

#endif  // #ifdef USE_DMA

#ifdef USE_PWM
    // TOP_LED_BIRD
    PB2 = 0;

    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB2MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB2MFP_EPWM0_CH3);

    EPWM_ConfigOutputChannel(EPWM0, 3, 800000, 0);
    EPWM_SET_PRESCALER(EPWM0, 3, 0);
    EPWM_EnableOutput(EPWM0, EPWM_CH_3_MASK);

#ifndef USE_PWM_DMA
    EPWM_EnableZeroInt(EPWM0, 3);
    NVIC_SetPriority(EPWM0P1_IRQn, 0);
    NVIC_EnableIRQ(EPWM0P1_IRQn);

    pwm0Buf = birdsLedsDMABuf[0].data();
    pwm0BufEnd = pwm0Buf + birdsLedsDMABuf[0].size();

    EPWM_SET_CMR(EPWM0, 3, *pwm0Buf++);
#else  // #ifndef USE_PWM_DMA
    EPWM_SET_CMR(EPWM0, 3, 0x0);
#endif  // #ifndef USE_DMA

    EPWM_SET_CNR(EPWM0, 3, 0x100);

    // BOTTOM_LED_BIRD
    PB13 = 0;

    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB13MFP_EPWM1_CH2);

    EPWM_ConfigOutputChannel(EPWM1, 2, 800000, 0);
    EPWM_SET_PRESCALER(EPWM1, 2, 0);
    EPWM_EnableOutput(EPWM1, EPWM_CH_2_MASK);

#ifndef USE_PWM_DMA
    EPWM_EnableZeroInt(EPWM1, 2);
    NVIC_SetPriority(EPWM1P1_IRQn, 0);
    NVIC_EnableIRQ(EPWM1P1_IRQn);

    pwm1Buf = birdsLedsDMABuf[1].data();
    pwm1BufEnd = pwm1Buf + birdsLedsDMABuf[1].size();

    EPWM_SET_CMR(EPWM1, 2, *pwm1Buf++);
#else  // #ifndef USE_PWM_DMA
    EPWM_SET_CMR(EPWM1, 2, 0x0);
#endif  // #ifdef USE_DMA

    EPWM_SET_CNR(EPWM1, 2, 0x100);

#ifdef USE_PWM_DMA
    EPWM0->PDMACTL = EPWM_PDMACTL_CHEN2_3_Msk | EPWM_PDMACTL_CHSEL2_3_Msk;
    EPWM1->PDMACTL = EPWM_PDMACTL_CHEN2_3_Msk;

    PDMA_SetTransferCnt(PDMA,EPWM0_TX_DMA_CH, PDMA_WIDTH_8, birdsLedsDMABuf[0].size());
    PDMA_SetTransferMode(PDMA,EPWM0_TX_DMA_CH, PDMA_EPWM0_CH3_TX, FALSE, 0);

    PDMA_SetTransferCnt(PDMA,EPWM1_TX_DMA_CH, PDMA_WIDTH_8, birdsLedsDMABuf[1].size());
    PDMA_SetTransferMode(PDMA,EPWM1_TX_DMA_CH, PDMA_EPWM0_CH2_TX, FALSE, 0);

    // Note: Nothing happens. I assume the chip does not support DMA transfer like STM32s/NXPs.
    EPWM_Start(EPWM0, EPWM_CH_3_MASK);
    EPWM_Start(EPWM1, EPWM_CH_2_MASK);
#else  // #ifdef USE_DMA
    // Start Top
    EPWM_Start(EPWM0, EPWM_CH_3_MASK);
#endif  // #ifdef USE_DMA

#else  // #ifdef USE_PWM

#define DELAY() \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::); \
    asm volatile ("nop"::);

    __disable_irq();
    PB2 = 0;
    PB13 = 0;
    for (size_t c = 0; c < birdsLedsDMABuf[0].size(); c++) {
        DELAY();
        PB2 = (birdsLedsDMABuf[0][c] >> 7) & 1;
        PB13 = (birdsLedsDMABuf[1][c] >> 7) & 1;
        DELAY();
        PB2 = (birdsLedsDMABuf[0][c] >> 6) & 1;
        PB13 = (birdsLedsDMABuf[1][c] >> 6) & 1;
        DELAY();
        PB2 = (birdsLedsDMABuf[0][c] >> 5) & 1;
        PB13 = (birdsLedsDMABuf[1][c] >> 5) & 1;
        DELAY();
        PB2 = (birdsLedsDMABuf[0][c] >> 4) & 1;
        PB13 = (birdsLedsDMABuf[1][c] >> 4) & 1;
        DELAY();
        PB2 = (birdsLedsDMABuf[0][c] >> 3) & 1;
        PB13 = (birdsLedsDMABuf[1][c] >> 3) & 1;
        DELAY();
        PB2 = (birdsLedsDMABuf[0][c] >> 2) & 1;
        PB13 = (birdsLedsDMABuf[1][c] >> 2) & 1;
        DELAY();
        PB2 = (birdsLedsDMABuf[0][c] >> 1) & 1;
        PB13 = (birdsLedsDMABuf[1][c] >> 1) & 1;
        DELAY();
        PB2 = (birdsLedsDMABuf[0][c] >> 0) & 1;
        PB13 = (birdsLedsDMABuf[1][c] >> 0) & 1;
    }
    PB2 = 0;
    PB13 = 0;
    __enable_irq();
#endif // #ifdef USE_PWM
}
