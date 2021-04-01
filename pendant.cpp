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

#include "M480.h"

#ifndef BOOTLOADER

static constexpr vector::float4 gradient_rainbow_data[] = {
    color::srgb8_stop({0xff,0x00,0x00}, 0.00f),
    color::srgb8_stop({0xff,0xff,0x00}, 0.16f),
    color::srgb8_stop({0x00,0xff,0x00}, 0.33f),
    color::srgb8_stop({0x00,0xff,0xff}, 0.50f),
    color::srgb8_stop({0x00,0x00,0xff}, 0.66f),
    color::srgb8_stop({0xff,0x00,0xff}, 0.83f),
    color::srgb8_stop({0xff,0x00,0x00}, 1.00f)};
static constexpr color::gradient gradient_rainbow(gradient_rainbow_data,7);

Pendant &Pendant::instance() {
    
    static Pendant pendant;
    if (!pendant.initialized) {
        pendant.initialized = true;
        pendant.init();
    }
    return pendant;
}

void Pendant::init() { 
    Leds::instance();
    I2CManager::instance();
    Timeline::instance();
    Input::instance();
    BQ25895::instance();
}

void Pendant::Run() {

    while (1) {
        __WFI();

        SDCard::instance().process();

        if (Timeline::instance().CheckFrameReadyAndClear()) {

#if 1
            static float rot = 0.0f;
            rot+=0.01f;
            for (size_t c = 0; c < Leds::instance().circleLedsN; c++) {
                Leds::instance().setCircle(0,c,gradient_rainbow.repeat(rot+float(c)/float(Leds::instance().circleLedsN)) * 0.1f);
                Leds::instance().setCircle(1,c,gradient_rainbow.repeat(rot+float(c)/float(Leds::instance().circleLedsN)) * 0.1f);
            }
            for (size_t c = 0; c < Leds::instance().birdLedsN; c++) {
                Leds::instance().setBird(0,c,gradient_rainbow.repeat(rot+float(c)/float(Leds::instance().circleLedsN)) * 0.1f);
                Leds::instance().setBird(1,c,gradient_rainbow.repeat(rot+float(c)/float(Leds::instance().circleLedsN)) * 0.1f);
            }
#endif  // #if 1

            printf("%f\r",float(Timeline::instance().SystemTime()));
            fflush(stdout);
            Leds::instance().apply();
        }
    }
}

void pendant_entry(void) {
    Pendant::instance().Run();
}

#endif  // #ifndef BOOTLOADER
