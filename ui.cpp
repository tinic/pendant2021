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
#include "./ens210.h"
#include "./bq25895.h"

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
    static Timeline::Span mainUI;
    if (!Timeline::instance().Scheduled(mainUI)) {
        mainUI.type = Timeline::Span::Display;
        mainUI.time = Timeline::instance().SystemTime();
        mainUI.duration = std::numeric_limits<double>::infinity();
        mainUI.calcFunc = [=](Timeline::Span &, Timeline::Span &) {
            SDD1306::instance().ClearChar();
            char str[32];
            sprintf(str,"B:%6.1f%%", 0.0);
            SDD1306::instance().PlaceUTF8String(0,0,str);
            sprintf(str,"D:%6.1fs", Timeline::instance().SystemTime());
            SDD1306::instance().PlaceUTF8String(0,1,str);
            sprintf(str,"T:%6.1fC", double(ENS210::instance().Temperature()));
            SDD1306::instance().PlaceUTF8String(0,2,str);
            sprintf(str,"H:%6.1f%%", double(ENS210::instance().Humidity())*100.0);
            SDD1306::instance().PlaceUTF8String(0,3,str);
            sprintf(str,"V:%6.1fV", double(BQ25895::instance().BatteryVoltage()));
            SDD1306::instance().PlaceUTF8String(0,4,str);
        };
        mainUI.commitFunc = [=](Timeline::Span &) {
            SDD1306::instance().Display();
        };
        mainUI.switch1Func = [=](Timeline::Span &, bool) {
        };
        mainUI.switch2Func = [=](Timeline::Span &, bool) {
        };
        mainUI.switch3Func = [=](Timeline::Span &, bool) {
        };
        Timeline::instance().Add(mainUI);
    }
}
