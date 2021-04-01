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
#include "./input.h"
#include "./bq25895.h"

#include "M480.h"

extern "C" {

void GPF_IRQHandler(void)
{
    if(GPIO_GET_INT_FLAG(PF, BIT2))
    {
        Input::instance().switch1Down = PF2 ? true : false;
        GPIO_CLR_INT_FLAG(PF, BIT2);
        printf("(SW1) PF.2 IRQ occurred.\n");
    }

    if(GPIO_GET_INT_FLAG(PF, BIT4))
    {
        BQ25895::instance().UpdateState();
        GPIO_CLR_INT_FLAG(PF, BIT4);
    }

    if(GPIO_GET_INT_FLAG(PF, BIT5))
    {
        GPIO_CLR_INT_FLAG(PF, BIT5);
        printf("(DSEL) PF.5 IRQ occurred.\n");
    }
}

void GPB_IRQHandler(void)
{
    if(GPIO_GET_INT_FLAG(PB, BIT5))
    {
        Input::instance().switch2Down = PB5 ? true : false;
        GPIO_CLR_INT_FLAG(PB, BIT5);
        printf("(SW2) PB.5 IRQ occurred.\n");
    }

    if(GPIO_GET_INT_FLAG(PB, BIT15))
    {
        Input::instance().switch2Down = PB15 ? true : false;
        GPIO_CLR_INT_FLAG(PB, BIT15);
        printf("(SW3) PB.15 IRQ occurred.\n");
    }
}

}

Input &Input::instance() {
    static Input input;
    if (!input.initialized) {
        input.initialized = true;
        input.init();
    }
    return input;
}

void Input::init() {
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_64);

    // BQ_INT
    GPIO_SetMode(PF, BIT4, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PF, BIT4, GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PF, 4, GPIO_INT_BOTH_EDGE);

    // DSEL
    GPIO_SetMode(PF, BIT5, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PF, BIT5, GPIO_PUSEL_PULL_UP);
    GPIO_EnableInt(PF, 5, GPIO_INT_BOTH_EDGE);

    // SW1
    GPIO_SetMode(PF, BIT2, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PF, BIT2, GPIO_PUSEL_PULL_UP);
    GPIO_ENABLE_DEBOUNCE(PF, BIT2);
    GPIO_CLR_INT_FLAG(PF, BIT2);
    GPIO_EnableInt(PF, 2, GPIO_INT_BOTH_EDGE);

    // SW2
    GPIO_SetMode(PB, BIT5, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PB, BIT5, GPIO_PUSEL_PULL_UP);
    GPIO_ENABLE_DEBOUNCE(PB, BIT5);
    GPIO_CLR_INT_FLAG(PB, BIT5);
    GPIO_EnableInt(PB, 5, GPIO_INT_BOTH_EDGE);

    // SW3
    GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PB, BIT15, GPIO_PUSEL_PULL_UP);
    GPIO_ENABLE_DEBOUNCE(PB, BIT15);
    GPIO_CLR_INT_FLAG(PB, BIT15);
    GPIO_EnableInt(PB, 15, GPIO_INT_BOTH_EDGE);

    NVIC_EnableIRQ(GPB_IRQn);
    NVIC_EnableIRQ(GPF_IRQn);
}
