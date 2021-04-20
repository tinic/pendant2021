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
#include "./pendant.h"
#include "./color.h"
#include "./leds.h"
#include "./i2cmanager.h"
#include "./timeline.h"
#include "./sdcard.h"
#include "./input.h"
#include "./bq25895.h"
#include "./ens210.h"
#include "./stm32wl.h"
#include "./sdd1306.h"
#include "./effects.h"
#include "./ui.h"
#include "./model.h"

#include "M480.h"

#ifndef BOOTLOADER

Pendant &Pendant::instance() {
    static Pendant pendant;
    if (!pendant.initialized) {
        pendant.initialized = true;
        pendant.init();
    }
    return pendant;
}

void Pendant::init() { 
    Model::instance();
    Leds::instance();
    I2CManager::instance();
    Timeline::instance();
    Input::instance();
    SDD1306::instance();
    BQ25895::instance();
    ENS210::instance();
    Effects::instance();
    UI::instance();
    SDCard::instance();
    SDD1306::instance().DisplayOff();
    STM32WL::instance();
}

void Pendant::Run() {
    Model::instance().IncBootCount();
    while (1) {
        __WFI();
        if (Timeline::instance().CheckBackgroundReadyAndClear()) {
            BQ25895::instance().UpdateState();
            ENS210::instance().update();
            STM32WL::instance().update();
        }
        if (Timeline::instance().CheckEffectReadyAndClear()) {
            Timeline::instance().ProcessEffect();
            if (Timeline::instance().TopEffect().Valid()) {
                Timeline::instance().TopEffect().Calc();
                Timeline::instance().TopEffect().Commit();
            }
        }
        if (SDD1306::instance().IsDisplayOn() && 
            Timeline::instance().CheckDisplayReadyAndClear()) {
            Timeline::instance().ProcessDisplay();
            if (Timeline::instance().TopDisplay().Valid()) {
                Timeline::instance().TopDisplay().Calc();
                Timeline::instance().TopDisplay().Commit();
            }
        }
    }
}

void pendant_entry(void) {
    Pendant::instance().Run();
}

#endif  // #ifndef BOOTLOADER
