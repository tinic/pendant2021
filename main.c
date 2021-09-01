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
#include "version.h"

#include <stdio.h>

void delay_us(int usec) {
    asm volatile (
        "1:\n\t"
#define NOP6 "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
#define NOP24 NOP6 NOP6 NOP6 NOP6
#define NOP96 NOP24 NOP24 NOP24 NOP24
        NOP96
        "subs %[usec], #1\n\t"
        "bge 1b\n\t"
    : : [usec] "r" (usec) : );
}

#ifdef BOOTLOADER
extern void bootloader_entry(void);
#else  // #ifdef BOOTLOADER
extern void pendant_entry(void);
#endif  // #ifdef BOOTLOADER

static void SYS_Init(void)
{
    SYS_UnlockReg();

    // Set up pins

    // I2C0
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC1MFP_Msk | SYS_GPC_MFPL_PC0MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC1MFP_I2C0_SCL | SYS_GPC_MFPL_PC0MFP_I2C0_SDA);

    // ICE
    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF1MFP_Msk | SYS_GPF_MFPL_PF0MFP_Msk);
    SYS->GPF_MFPL |= (SYS_GPF_MFPL_PF1MFP_ICE_CLK | SYS_GPF_MFPL_PF0MFP_ICE_DAT);

    // GPIO
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB15MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB15MFP_GPIO);

    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB5MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB0MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB5MFP_GPIO | SYS_GPB_MFPL_PB1MFP_GPIO | SYS_GPB_MFPL_PB0MFP_GPIO);

    SYS->GPF_MFPL &= ~(SYS_GPF_MFPL_PF5MFP_Msk | SYS_GPF_MFPL_PF4MFP_Msk | SYS_GPF_MFPL_PF3MFP_Msk | SYS_GPF_MFPL_PF2MFP_Msk);
    SYS->GPF_MFPL |= (SYS_GPF_MFPL_PF5MFP_GPIO | SYS_GPF_MFPL_PF4MFP_GPIO | SYS_GPF_MFPL_PF3MFP_GPIO | SYS_GPF_MFPL_PF2MFP_GPIO);

    // SDA
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC1MFP_Msk | SYS_GPC_MFPL_PC0MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC1MFP_I2C0_SCL | SYS_GPC_MFPL_PC0MFP_I2C0_SDA);

    // QSPI0
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA3MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk | SYS_GPA_MFPL_PA1MFP_Msk | SYS_GPA_MFPL_PA0MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA3MFP_QSPI0_SS | SYS_GPA_MFPL_PA2MFP_QSPI0_CLK | SYS_GPA_MFPL_PA1MFP_QSPI0_MISO0 | SYS_GPA_MFPL_PA0MFP_QSPI0_MOSI0);

    // SPI0
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_SPI0_MOSI);

    // SPI1
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB4MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB4MFP_SPI1_MOSI);

    // UART1
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB3MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB3MFP_UART1_TXD);

    // USB
    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA15MFP_Msk | SYS_GPA_MFPH_PA14MFP_Msk | SYS_GPA_MFPH_PA13MFP_Msk | SYS_GPA_MFPH_PA12MFP_Msk);
    SYS->GPA_MFPH |= (SYS_GPA_MFPH_PA15MFP_USB_OTG_ID | SYS_GPA_MFPH_PA14MFP_USB_D_P | SYS_GPA_MFPH_PA13MFP_USB_D_N | SYS_GPA_MFPH_PA12MFP_USB_VBUS);

    // Init clocks
    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_HIRC48MEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRC48MSTB_Msk);

    CLK_DisablePLL();

    CLK_SetCoreClock(96000000UL);

    CLK->PCLKDIV = CLK_PCLKDIV_APB0DIV_DIV1 | CLK_PCLKDIV_APB1DIV_DIV1; // 96Mhz

    CLK_EnableModuleClock(TMR0_MODULE);
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_LIRC, MODULE_NoMsk); // 10Khz

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_LIRC, MODULE_NoMsk); // 10Khz

    CLK_EnableModuleClock(UART1_MODULE);
    CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HIRC, CLK_CLKDIV0_UART1(12)); // 1Mhz
   
    CLK_EnableModuleClock(SPI0_MODULE);
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_HIRC, MODULE_NoMsk); // 12Mhz

    CLK_EnableModuleClock(SPI1_MODULE);
    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_HIRC, MODULE_NoMsk); // 12Mhz

    CLK_EnableModuleClock(FMCIDLE_MODULE); // HIRC, 12Mhz
    CLK_EnableModuleClock(ISP_MODULE); // HIRC, 12Mhz

    CLK_EnableModuleClock(I2C0_MODULE); // PCLK0, 12Mhz
    CLK_EnableModuleClock(PDMA_MODULE); // HCLK, 96Mhz

    CLK_EnableModuleClock(QSPI0_MODULE);
    CLK_SetModuleClock(QSPI0_MODULE, CLK_CLKSEL2_QSPI0SEL_PLL, MODULE_NoMsk); // 96Mhz

    CLK_EnableModuleClock(USBD_MODULE);
    CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_RC48M, MODULE_NoMsk); // 48Mhz

    // Enable USB FS phy
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk;

    SystemCoreClockUpdate();

    SYS_LockReg();
}

static void SYS_DeInit(void)
{
    CLK_DisableModuleClock(USBD_MODULE);
    CLK_DisableModuleClock(EPWM1_MODULE);
    CLK_DisableModuleClock(EPWM0_MODULE);
    CLK_DisableModuleClock(TRNG_MODULE);
    CLK_DisableModuleClock(PDMA_MODULE);
    CLK_DisableModuleClock(I2C0_MODULE);
    CLK_DisableModuleClock(ISP_MODULE);
    CLK_DisableModuleClock(FMCIDLE_MODULE);
    CLK_DisableModuleClock(SPI1_MODULE);
    CLK_DisableModuleClock(SPI0_MODULE);
    CLK_DisableModuleClock(UART1_MODULE);
    CLK_DisableModuleClock(TMR0_MODULE);
    CLK_DisableModuleClock(TMR1_MODULE);
}

int main(void)
{
    SYS_Init();

    UART_Open(UART1, 115200);

    SYS_UnlockReg();
    FMC_Open();

    printf("================================================================================\n");
    printf("Pendant 2021 "
#if defined(TESTING)
#if defined(BOOTLOADER)
        "Bootloader Testing"
#else  // #if defined(BOOTLOADER)
        "Main Testing"
#endif  // #if defined(BOOTLOADER)
#elif defined(BOOTLOADER)
        "Bootloader"
#else  // #if defined(BOOTLOADER)
        "Main"
#endif  // #if defined(BOOTLOADER)
        "\nPID(0x%08x) UID(0x%08x%08x%08x)\n"
        "Build %s\n",
        (unsigned int)SYS_ReadPDID(),
        (unsigned int)FMC_ReadUID(0),
        (unsigned int)FMC_ReadUID(1),
        (unsigned int)FMC_ReadUID(2),
        "r" GIT_REV_COUNT " (" GIT_SHORT_SHA ") " GIT_COMMIT_DATE);
    printf("--------------------------------------------------------------------------------\n");

    FMC_Close();
    SYS_LockReg();

#if defined(BOOTLOADER)

    FMC_Open();

    if (PF2 != 0) {        
        SYS_DeInit();
        
        FMC_SetVectorPageAddr(FIRMWARE_ADDR + FIRMWARE_START);

        SYS_ResetChip();

        for (;;) { }
    }

    bootloader_entry();

#else  // #if defined(BOOTLOADER)

    pendant_entry();

#endif  // #if defined(BOOTLOADER)

    for (;;) {
    }

    SYS_DeInit();

    return 0;
}
