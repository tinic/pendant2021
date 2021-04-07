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
#include "./sdd1306.h"

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
    ENS210::instance();
}

__attribute__ ((optimize("Os")))
void Pendant::DemoPattern() {
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
}

void Pendant::Run() {


    while (1) {

        __WFI();

        if (Timeline::instance().CheckEffectReadyAndClear()) {
            Timeline::instance().ProcessEffect();
        }

        if (Timeline::instance().CheckDisplayReadyAndClear()) {
            Timeline::instance().ProcessDisplay();
        }

        if (Timeline::instance().CheckBackgroundReadyAndClear()) {
            ENS210::instance().update();
            BQ25895::instance().UpdateState();
        }

#if 0
            static double last_printf = 0;
            if ( (Timeline::instance().SystemTime() - last_printf ) > (1.0/10.0)) {     
                Timeline::ProcessDisplay();
                last_printf = Timeline::instance().SystemTime();      
                SDD1306::instance().ClearChar();
                char str[32];
                sprintf(str,"B:%6.1f%%", (delta_busy/(delta_idle+delta_busy))*100.0);
                SDD1306::instance().PlaceUTF8String(0,0,str);
                sprintf(str,"D:%6.1fs", Timeline::instance().SystemTime());
                SDD1306::instance().PlaceUTF8String(0,1,str);
                sprintf(str,"T:%6.1fC", double(ENS210::instance().Temperature()));
                SDD1306::instance().PlaceUTF8String(0,2,str);
                sprintf(str,"H:%6.1f%%", double(ENS210::instance().Humidity())*100.0);
                SDD1306::instance().PlaceUTF8String(0,3,str);
                sprintf(str,"V:%6.1fV", double(BQ25895::instance().BatteryVoltage()));
                SDD1306::instance().PlaceUTF8String(0,4,str);

                SDD1306::instance().Display();
            }
            if ( (Timeline::instance().SystemTime() - last_printf ) > (1.0/10.0)) {     
            }
        }
#endif  // #if 0

    }
}

void pendant_entry(void) {
    Pendant::instance().Run();
}

#endif  // #ifndef BOOTLOADER
