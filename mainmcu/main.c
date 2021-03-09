#include "main.h"

#include "M480.h"

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

    for (;;) {
    }

    return 0;
}
