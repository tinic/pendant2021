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
#include "./main.h"

#include "M480.h"

extern void pendant_entry(void);

void SYS_Init(void)
{
    SYS_UnlockReg();

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_HIRC48MEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRC48MSTB_Msk);

    CLK_SetCoreClock(96000000UL);

    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV8 | CLK_PCLKDIV_APB1DIV_DIV8;

    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1)); // 12Mhz
   
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_HIRC, MODULE_NoMsk); // 12Mhz
    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_HIRC, MODULE_NoMsk); // 12Mhz

    CLK_EnableModuleClock(I2C0_MODULE); // PCLK0, 12Mhz
    CLK_EnableModuleClock(PDMA_MODULE); // HCLK, 96Mhz
    CLK_EnableModuleClock(TRNG_MODULE); // PCLK1, 12 Mhz

    CLK_EnableModuleClock(USCI0_MODULE); 
    CLK_EnableModuleClock(USCI1_MODULE);

    SystemCoreClockUpdate();

    // SW1
    GPIO_SetMode(PF, BIT2, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PF, BIT2, GPIO_PUSEL_PULL_UP);
    // SW2
    GPIO_SetMode(PB, BIT5, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PB, BIT5, GPIO_PUSEL_PULL_UP);
    // SW3
    GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PB, BIT15, GPIO_PUSEL_PULL_UP);
    // BQ_INT
    GPIO_SetMode(PF, BIT4, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PF, BIT4, GPIO_PUSEL_PULL_UP);
    // DSEL
    GPIO_SetMode(PF, BIT5, GPIO_MODE_OUTPUT);
    // LED_ON
    GPIO_SetMode(PF, BIT3, GPIO_MODE_OUTPUT);
    // OLED_CS
    GPIO_SetMode(PB, BIT1, GPIO_MODE_OUTPUT);
    // OLED_RESET
    GPIO_SetMode(PB, BIT9, GPIO_MODE_OUTPUT);

    // TODO: USB

    SYS_LockReg();
}

int main(void)
{
    SYS_Init();

    pendant_entry();

    for (;;) {
    }

    return 0;
}
