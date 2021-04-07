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
#include "./effects.h"
#include "./timeline.h"
#include "./leds.h"
#include "./model.h"

static constexpr vector::float4 gradient_rainbow_data[] = {
    color::srgb8_stop({0xff,0x00,0x00}, 0.00f),
    color::srgb8_stop({0xff,0xff,0x00}, 0.16f),
    color::srgb8_stop({0x00,0xff,0x00}, 0.33f),
    color::srgb8_stop({0x00,0xff,0xff}, 0.50f),
    color::srgb8_stop({0x00,0x00,0xff}, 0.66f),
    color::srgb8_stop({0xff,0x00,0xff}, 0.83f),
    color::srgb8_stop({0xff,0x00,0x00}, 1.00f)};
static constexpr color::gradient gradient_rainbow(gradient_rainbow_data,7);

Effects &Effects::instance() {
    static Effects effects;
    if (!effects.initialized) {
        effects.initialized = true;
        effects.init();
    }
    return effects;
}

void Effects::demo() {
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

void Effects::init() {
    static Timeline::Span mainEffect;

    static uint32_t current_effect = 0;
    static uint32_t previous_effect = 0;
    
    static double switch_time = 0;

    if (!Timeline::instance().Scheduled(mainEffect)) {
        mainEffect.type = Timeline::Span::Display;
        mainEffect.time = Timeline::instance().SystemTime();
        mainEffect.duration = std::numeric_limits<double>::infinity();
        mainEffect.calcFunc = [this](Timeline::Span &, Timeline::Span &) {

            if ( current_effect != Model::instance().Effect() ) {
                previous_effect = current_effect;
                current_effect = Model::instance().Effect();
                switch_time = Timeline::instance().SystemTime();
            }

            auto calc_effect = [this] (uint32_t effect) mutable {
                switch (effect) {
                    case 0:
                        demo();
                    break;
                }
            };

            double blend_duration = 0.5;
            double now = Timeline::instance().SystemTime();
            
            if ((now - switch_time) < blend_duration) {
                calc_effect(previous_effect);

                auto circleLedsPrev = Leds::instance().getCircle();
                auto birdsLedsPrev = Leds::instance().getBird();

                calc_effect(current_effect);

                auto circleLedsNext = Leds::instance().getCircle();
                auto birdsLedsNext = Leds::instance().getBird();

                float blend = static_cast<float>(now - switch_time) * (1.0f / static_cast<float>(blend_duration));

                for (size_t c = 0; c < circleLedsNext.size(); c++) {
                    for (size_t d = 0; d < circleLedsNext[c].size(); d++) {
                        circleLedsNext[c][d] = vector::float4::lerp(circleLedsPrev[c][d], circleLedsNext[c][d], blend);
                    }
                }

                Leds::instance().setCircle(circleLedsNext);

                for (size_t c = 0; c < birdsLedsNext.size(); c++) {
                    for (size_t d = 0; d < birdsLedsNext[c].size(); d++) {
                        birdsLedsNext[c][d] = vector::float4::lerp(birdsLedsPrev[c][d], birdsLedsNext[c][d], blend);
                    }
                }

                Leds::instance().setBird(birdsLedsNext);

            } else {
                calc_effect(current_effect);
            }

        };
        mainEffect.commitFunc = [this](Timeline::Span &) {
            Leds::instance().apply();
        };
        Timeline::instance().Add(mainEffect);
    }
}
