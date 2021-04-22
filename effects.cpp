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
#include "./color.h"
#include "./fastmath.h"

#include <random>
#include <array>
#include <limits>
#include <math.h>

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

void Effects::standard_bird() {
    static float walker = 0.0f;
    walker += 0.01f;
    if (walker >= 1.0f) walker = 0.0f;
    auto calc = [](const std::function<vector::float4 (const vector::float4 &pos)> &func) {
        Leds &leds(Leds::instance());
        for (size_t c = 0; c < Leds::birdLedsN; c++) {
            auto pos = Leds::instance().map.getBird(c);
            auto col = func(pos) * walker;
            leds.setBird(0,c,col);
            leds.setBird(1,c,col);
        }
    };
    calc([=](const vector::float4 &pos) {
        return color::srgb8(Model::instance().BirdColor()) + color::srgb8({0xff,0xff,0xff}) * fast_pow(1.0f - pos.w, 8.0f) * 0.25f;
    });
}

void Effects::color_walker() {
    standard_bird();

    Leds &leds(Leds::instance());

    double now = Timeline::SystemTime();

    const double speed = 1.0;

    float rgb_walk = (       static_cast<float>(frac(now * (1.0 / 5.0) * speed)));
    float val_walk = (1.0f - static_cast<float>(frac(now               * speed)));

    vector::float4 col(gradient_rainbow.repeat(rgb_walk));
    for (size_t c = 0; c < Leds::circleLedsN; c++) {
        float mod_walk = val_walk + (1.0f - (c * ( 1.0f / static_cast<float>(Leds::circleLedsN) ) ) );
        if (mod_walk > 1.0f) {
            mod_walk -= 1.0f;
        }
        float v = fast_pow(std::min(1.0f, mod_walk), 2.0f);;
        leds.setCircle(0, c, col * v);
        leds.setCircle(1, Leds::circleLedsN-1-c, col * v);
    }

}

void Effects::light_walker() {
    standard_bird();

    Leds &leds(Leds::instance());

    double now = Timeline::SystemTime();

    const double speed = 1.0;

    float rgb_walk = (       static_cast<float>(frac(now * (1.0 / 5.0) * speed)));
    float val_walk = (1.0f - static_cast<float>(frac(now               * speed)));

    vector::float4 hsv(rgb_walk, 1.0f, 1.0f);
    for (size_t c = 0; c < Leds::circleLedsN; c++) {
        float mod_walk = val_walk + (1.0f - (c * ( 1.0f / static_cast<float>(Leds::circleLedsN) ) ) );
        if (mod_walk > 1.0f) {
            mod_walk -= 1.0f;
        }
        hsv.y = 1.0f - fast_pow(std::min(1.0f, mod_walk), 6.0f);
        hsv.z = fast_pow(std::min(1.0f, mod_walk), 6.0f);
        leds.setCircle(0, c, color::hsv(hsv));
        leds.setCircle(1, Leds::circleLedsN-1-c, color::hsv(hsv));
    }

}

template<const std::size_t n> static void band_mapper(std::array<float, n> &stops, float stt, float end) {

    for (; stt < 0 ;) {
        stt += 1.0f;
        end += 1.0f;
    }

    for (; stt > 1.0f ;) {
        stt -= 1.0f;
        end -= 1.0f;
    }

    stt = fmodf(stt, 2.0f) + 0.5f;
    end = fmodf(end, 2.0f) + 0.5f;

    float stop_step = static_cast<float>(stops.size());
    float stop_stepi = 1.0f / static_cast<float>(stops.size());

    float stop_stt = -stop_stepi * 0.5f + 0.5f;
    float stop_end = +stop_stepi * 0.5f + 0.5f;

    for (size_t c = 0; c < stops.size(); c++) {
        stops[c] = 0.0f;
    }

    for (size_t c = 0; c < stops.size() * 2; c++) {
        if (stt <= stop_stt && end >= stop_end) {
            if ( ( stop_stt - stt ) < stop_stepi) {
                stops[c % stops.size()] = ( stop_stt - stt ) * stop_step;
            } else if ( ( end - stop_end ) < stop_stepi) {
                stops[c % stops.size()] = ( end - stop_end ) * stop_step;
            } else {
                stops[c % stops.size()] = 1.0f;
            }
        }
        stop_end += stop_stepi;
        stop_stt += stop_stepi;
    }
}

void Effects::rgb_band() {

    standard_bird();

    static float rgb_band_r_walk = 0.0f;
    static float rgb_band_g_walk = 0.0f;
    static float rgb_band_b_walk = 0.0f;

    static float rgb_band_r_walk_step = 1.0f;
    static float rgb_band_g_walk_step = 1.0f;
    static float rgb_band_b_walk_step = 1.0f;

    static std::mt19937 gen;
    static std::uniform_real_distribution<float> disf(+0.001f, +0.005f);
    static std::uniform_int_distribution<int32_t> disi(0, 1);

    std::array<float, Leds::circleLedsN> band_r;
    std::array<float, Leds::circleLedsN> band_g;
    std::array<float, Leds::circleLedsN> band_b;

    if (fabsf(rgb_band_r_walk) >= 2.0f) {
        while (rgb_band_r_walk >= +1.0f) { rgb_band_r_walk -= 1.0f; }
        while (rgb_band_r_walk <= -1.0f) { rgb_band_r_walk += 1.0f; }
        rgb_band_r_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
    }

    if (fabsf(rgb_band_g_walk) >= 2.0f) {
        while (rgb_band_g_walk >= +1.0f) { rgb_band_g_walk -= 1.0f; }
        while (rgb_band_g_walk <= -1.0f) { rgb_band_g_walk += 1.0f; }
        rgb_band_g_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
    }

    if (fabsf(rgb_band_b_walk) >= 2.0f) {
        while (rgb_band_b_walk >= +1.0f) { rgb_band_b_walk -= 1.0f; }
        while (rgb_band_b_walk <= -1.0f) { rgb_band_b_walk += 1.0f; }
        rgb_band_b_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
    }

    band_mapper(band_r, rgb_band_r_walk, rgb_band_r_walk + (1.0f / 3.0f));
    band_mapper(band_g, rgb_band_g_walk, rgb_band_g_walk + (1.0f / 3.0f));
    band_mapper(band_b, rgb_band_b_walk, rgb_band_b_walk + (1.0f / 3.0f));

    Leds &leds(Leds::instance());
    for (size_t c = 0; c < Leds::circleLedsN; c++) {
        auto out = color::srgb({band_r[c], band_g[c], band_b[c]});
        leds.setCircle(0, c, out);
        leds.setCircle(1, Leds::circleLedsN-1-c, out);
    }

    rgb_band_r_walk -= rgb_band_r_walk_step;
    rgb_band_g_walk += rgb_band_g_walk_step;
    rgb_band_b_walk += rgb_band_b_walk_step;
}

void Effects::brilliance() {
    standard_bird();

/*    float now = static_cast<float>(Timeline::SystemTime());

    static float next = -1.0f;
    static float dir = 0.0f;
    
    if ((next - now) < 0.0f || next < 0.0f) {
        next = now + random.get(2.0f, 20.0f);
        dir = random.get(0.0f, 3.141f * 2.0f);
    }

    static colors::gradient bw;
    static colors::rgb8 col;
    if (bw.check_init() || col != Model::instance().RingColor()) {
        col = Model::instance().RingColor();
        const geom::float4 bwg[] = {
            geom::float4(Model::instance().RingColor().hex(), 0.00f),
            geom::float4(Model::instance().RingColor().hex(), 0.14f),
            geom::float4(0xffffff, 0.21f),
            geom::float4(Model::instance().RingColor().hex(), 0.28f),
            geom::float4(Model::instance().RingColor().hex(), 1.00f)};
        bw.init(bwg,5);
    }

    calc_outer([=](geom::float4 pos) {
        pos = pos.rotate2d(dir);
        pos *= 0.50f;
        pos += (next - now) * 8.0f;
        pos *= 0.05f;
        return bw.clamp(pos.x);
    });*/
}

void Effects::init() {

    random.set_seed(0xBEEFEA78);

    static Timeline::Span mainEffect;

    static uint32_t current_effect = 0;
    static uint32_t previous_effect = 0;
    
    static double switch_time = 0;

    if (!Timeline::instance().Scheduled(mainEffect)) {
        mainEffect.type = Timeline::Span::Effect;
        mainEffect.time = Timeline::SystemTime();
        mainEffect.duration = std::numeric_limits<double>::infinity();
        mainEffect.calcFunc = [this](Timeline::Span &, Timeline::Span &) {

            if ( current_effect != Model::instance().Effect() ) {
                previous_effect = current_effect;
                current_effect = Model::instance().Effect();
                switch_time = Timeline::SystemTime();
            }

            auto calc_effect = [this] (uint32_t effect) {
                switch (effect) {
                    case 0:
                        rgb_band();
                    break;
                    case 1:
                        light_walker();
                    break;
                    case 2:
                        color_walker();
                    break;
                }
            };

            double blend_duration = 0.5;
            double now = Timeline::SystemTime();
            
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
