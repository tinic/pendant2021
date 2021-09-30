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
#include "./ui.h"
#include "./timeline.h"
#include "./sdd1306.h"
#include "./model.h"
#include "./stm32wl.h"

#include <stdio.h>

UI &UI::instance() {
    static UI ui;
    if (!ui.initialized) {
        ui.initialized = true;
        ui.init();
    }
    return ui;
}

void UI::init() {
    static Timeline::Display mainUI;
    if (!Timeline::instance().Scheduled(mainUI)) {
        mainUI.time = Timeline::SystemTime();
        mainUI.duration = std::numeric_limits<double>::infinity();

        mainUI.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
            SDD1306::instance().ClearChar();
            char str[32];
            sprintf(str,"B:      |");
            SDD1306::instance().PlaceUTF8String(0,0,str);
            sprintf(str,"D:%fs", Timeline::SystemTime());
            SDD1306::instance().PlaceUTF8String(0,1,str);
            sprintf(str,"T:%6.1fC", double(STM32WL::instance().Temperature()));
            SDD1306::instance().PlaceUTF8String(0,2,str);
            sprintf(str,"H:%6.1f%%", double(STM32WL::instance().Humidity())*100.0);
            SDD1306::instance().PlaceUTF8String(0,3,str);
            sprintf(str,"V:%6.1fV", double(STM32WL::instance().BatteryVoltage()));
            SDD1306::instance().PlaceUTF8String(0,4,str);
        };
        mainUI.commitFunc = [=](Timeline::Span &) {
            SDD1306::instance().Display();
        };
        mainUI.switch1Func = [=](Timeline::Span &, bool up) {
            if (up) { 
                printf("SW1\n");
                Model::instance().SetEffect((Model::instance().Effect() + 1) % Model::instance().EffectCount());
            }
        };
        mainUI.switch2Func = [=](Timeline::Span &, bool up) {
            if (up) { 
                printf("SW2\n");
            }
        };
        mainUI.switch3Func = [=](Timeline::Span &, bool up) {
            if (up) { 
                printf("SW3\n");
            }
        };
        Timeline::instance().Add(mainUI);
    }

    static Timeline::Display bootScreen;
    bootScreen.time = Timeline::SystemTime();
    bootScreen.duration = 1.0; // timeout

    static Timeline::Display moveOut;
    moveOut.time = bootScreen.time + bootScreen.duration;
    moveOut.duration = 0.25; // timeout

    static Timeline::Display moveIn;
    moveIn.time = moveOut.time + moveOut.duration;
    moveIn.duration = 0.25; // timeout

    bootScreen.startFunc = [=](Timeline::Span &) {
        SDD1306::instance().ClearChar();
        SDD1306::instance().ClearAttr();
        SDD1306::instance().SetBootScreen(true, 100);
        SDD1306::instance().Display();
    };
    bootScreen.calcFunc = [=](Timeline::Span &span, Timeline::Span &) {
        double now = Timeline::SystemTime();
        double delta = ( (span.time + span.duration) - now ) / span.duration;
        SDD1306::instance().SetBootScreen(true, static_cast<int32_t>(100.0f * Cubic::easeIn(static_cast<float>(delta), 0.0f, 1.0f, 1.0f)));
        SDD1306::instance().Display();
    };
    bootScreen.doneFunc = [=](Timeline::Span &span) {
        SDD1306::instance().SetBootScreen(true, 0);
        SDD1306::instance().Display();
        Timeline::instance().Add(moveOut);
        Timeline::instance().Remove(span);
    };

    moveOut.startFunc = [=](Timeline::Span &) {
    };

    moveOut.calcFunc = [=](Timeline::Span &span, Timeline::Span &) {
        double now = Timeline::SystemTime();
        double delta = ( (span.time + span.duration) - now ) / span.duration;
        SDD1306::instance().SetVerticalShift(-static_cast<int8_t>(16.0f * (1.0f - Cubic::easeOut(static_cast<float>(delta), 0.0f, 1.0f, 1.0f))));
        SDD1306::instance().Display();
    };
    moveOut.doneFunc = [=](Timeline::Span &span) {
        SDD1306::instance().SetVerticalShift(0);
        SDD1306::instance().SetBootScreen(false, 0);
        SDD1306::instance().SetCenterFlip(48);
        SDD1306::instance().Display();
        Timeline::instance().Add(moveIn);
        Timeline::instance().Remove(span);
    };

    moveIn.startFunc = [=](Timeline::Span &) {
        SDD1306::instance().SetVerticalShift(0);
        SDD1306::instance().SetBootScreen(false, 0);
        SDD1306::instance().Invalidate();
        SDD1306::instance().Display();
    };
    moveIn.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
        below.Calc();
        double now = Timeline::SystemTime();
        double delta = ( (span.time + span.duration) - now ) / span.duration;
        SDD1306::instance().SetCenterFlip(static_cast<int8_t>(48.0 * (delta)));
        SDD1306::instance().Display();
    };
    moveIn.doneFunc = [=](Timeline::Span &span) {
        SDD1306::instance().SetCenterFlip(0);
        SDD1306::instance().Display();
        Timeline::instance().Remove(span);
    };

    Timeline::instance().Add(bootScreen);
}
