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
#include "./timeline.h"
#include "./model.h"

#include "M480.h"

extern "C" {

void GPF_IRQHandler(void)
{
    if(GPIO_GET_INT_FLAG(PF, BIT2))
    {
        Timeline::instance().TopDisplay().ProcessSwitch1(PF2 ? true : false);
        if (PF2) Model::instance().IncSwitch1Count();
        GPIO_CLR_INT_FLAG(PF, BIT2);
    }

    if(GPIO_GET_INT_FLAG(PF, BIT5))
    {
        if (PF5) Model::instance().IncDselCount();
        printf("(DSEL) PF.5 IRQ occurred.\n");
        GPIO_CLR_INT_FLAG(PF, BIT5);
    }
}

void GPB_IRQHandler(void)
{
    if(GPIO_GET_INT_FLAG(PB, BIT5))
    {
        Timeline::instance().TopDisplay().ProcessSwitch2(PB5 ? true : false);
        if (PB5) Model::instance().IncSwitch2Count();
        GPIO_CLR_INT_FLAG(PB, BIT5);
    }

    if(GPIO_GET_INT_FLAG(PB, BIT15))
    {
        Timeline::instance().TopDisplay().ProcessSwitch3(PB15 ? true : false);
        if (PB15) Model::instance().IncSwitch3Count();
        GPIO_CLR_INT_FLAG(PB, BIT15);
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
    GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_256);

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
    NVIC_SetPriority(GPB_IRQn, 5);
    
    NVIC_EnableIRQ(GPF_IRQn);
    NVIC_SetPriority(GPF_IRQn, 5);
}
