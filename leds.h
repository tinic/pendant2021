/*
Copyright 2020 Tinic Uro

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
#ifndef LEDS_H_
#define LEDS_H_

#include "./color.h"

#include <numbers>
#include <cmath>

#define USE_SPI_DMA 1
//#define USE_PWM_DMA 1
#define USE_PWM 1

extern "C" {
    void EPWM0P1_IRQHandler(void);
    void EPWM1P1_IRQHandler(void);
};

class Leds {
public:
    static constexpr size_t sidesN = 2;
    static constexpr size_t circleLedsN = 32;
    static constexpr size_t birdLedsN = 8;
    static constexpr size_t ledsN = ( circleLedsN + birdLedsN ) * sidesN;

    static Leds &instance();

    void apply() { transfer(); }

    static struct Map {
        
        consteval Map() : map() {
            float i = - float(std::numbers::pi) * 0.5f;
            float j = 0;
            for (size_t c = 0; c < circleLedsN; c++) {
                map[c].x =  cosf(i);
                map[c].y = -sinf(i);
                map[c].z = j;
                map[c].w = i;
                i += 2.0f * float(std::numbers::pi) / float(circleLedsN);
                j += 1.0f / float(circleLedsN);
            }
            map[circleLedsN + 0] = vector::float4(  0.0f,  12.0f, 0.0f, 0.0f ) * (1.0f / 25.0f);
            map[circleLedsN + 1] = vector::float4(-11.0f,   5.0f, 0.0f, 0.0f ) * (1.0f / 25.0f);
            map[circleLedsN + 2] = vector::float4( -7.0f,   0.0f, 0.0f, 0.0f ) * (1.0f / 25.0f);
            map[circleLedsN + 3] = vector::float4(  0.0f,   0.0f, 0.0f, 0.0f ) * (1.0f / 25.0f);
            map[circleLedsN + 4] = vector::float4(  7.0f,   0.0f, 0.0f, 0.0f ) * (1.0f / 25.0f);
            map[circleLedsN + 5] = vector::float4( 11.0f,   5.0f, 0.0f, 0.0f ) * (1.0f / 25.0f);
            map[circleLedsN + 6] = vector::float4(  0.0f,  -8.0f, 0.0f, 0.0f ) * (1.0f / 25.0f);
            map[circleLedsN + 7] = vector::float4(  0.0f,- 16.0f, 0.0f, 0.0f ) * (1.0f / 25.0f);
            for (size_t c = circleLedsN; c < (circleLedsN + birdLedsN); c++) {
                map[c].w = sqrtf(map[c].x * map[c].x + map[c].y * map[c].y);
            }
        }

        vector::float4 get(size_t index) const {
            index %= circleLedsN + birdLedsN;
            return map[index];
        }

        vector::float4 getCircle(size_t index) const {
            index %= circleLedsN;
            return map[index];
        }

        vector::float4 getBird(size_t index) const {
            index %= birdLedsN;
            return map[circleLedsN + index];
        }

    private:
        vector::float4 map[circleLedsN + birdLedsN];
    } map;

    void black() {
        for (size_t c = 0; c < ledsN; c++) {
            set(c, color::srgb8({0x00,0x00,0x00}, 1.0f));
        }
    }

    void white() {
        for (size_t c = 0; c < ledsN; c++) {
            set(c, color::srgb8({0xFF,0xFF,0xFF}, 1.0f));
        }
    }

    void half() {
        for (size_t c = 0; c < ledsN; c++) {
            set(c, color::srgb8({0x1F,0x1F,0x1F}, 1.0f));
        }
    }

    void set(size_t index, const vector::float4 &c) {
        get(index) = c;
    }

    vector::float4 &get(size_t index) {
        index %= 2 * ( circleLedsN + birdLedsN );
        if (index >= 2 * circleLedsN + birdLedsN) {
            return birdLeds[1][index - (2 * circleLedsN + birdLedsN)];
        } else if (index >= 2 * circleLedsN) {
            return birdLeds[0][index - (2 * circleLedsN)];
        } else if (index >= circleLedsN) {
            return circleLeds[1][index - circleLedsN];
        } else {
            return circleLeds[0][index];
        }
    }

    void setCircle(size_t side, size_t index, const vector::float4 &c) {
        getCircle(side, index) = c;
    }

    vector::float4 &getCircle(size_t side, size_t index) {
        side %= sidesN;
        index %= circleLedsN;
        return circleLeds[side][index];
    }

    void setBird(size_t side, size_t index, const vector::float4 &c) {
        getBird(side, index) = c;
    }

    vector::float4 &getBird(size_t side, size_t index) {
        side %= sidesN;
        index %= birdLedsN;
        return birdLeds[side][index];
    }

    auto getCircle() { return circleLeds; }
    auto getBird() { return birdLeds; };
    void setCircle(auto _circleLeds) { circleLeds = _circleLeds; }
    void setBird(auto _birdLeds) { birdLeds = _birdLeds; }

    void forceStop();

private:
    friend void EPWM0P1_IRQHandler(void);
    friend void EPWM1P1_IRQHandler(void);

    static const struct lut_table {
        consteval lut_table() {
            for (uint32_t c = 0; c < 256; c++) {
                table[c] = 0x88888888 |
                        (((c >>  4) | (c <<  6) | (c << 16) | (c << 26)) & 0x04040404)|
                        (((c >>  1) | (c <<  9) | (c << 19) | (c << 29)) & 0x40404040);
            }
        }

        uint32_t operator[](size_t index) const {
            return table[index];
        }

    private:
        uint32_t table[256];
    } lut;

    std::array<std::array<vector::float4, circleLedsN>, sidesN> circleLeds;
    std::array<std::array<vector::float4, birdLedsN>, sidesN> birdLeds;

    static constexpr size_t bitsPerComponent = 16;
    static constexpr size_t bitsPerLed = bitsPerComponent * 3;

#ifdef USE_PWM
    static constexpr size_t extraBirdPadding = bitsPerLed * 2; // Need padding for PWM
    std::array<std::array<uint8_t, birdLedsN * bitsPerLed + extraBirdPadding>, sidesN> birdsLedsDMABuf __attribute__ ((aligned (16)));
#else  // #ifdef USE_PWM
    std::array<std::array<uint8_t, (birdLedsN * bitsPerLed) / 2>, sidesN> birdsLedsDMABuf __attribute__ ((aligned (16)));
#endif  // #ifdef USE_PWM
    std::array<std::array<uint8_t, (circleLedsN * bitsPerLed) / 2>, sidesN> circleLedsDMABuf __attribute__ ((aligned (16)));

    void transfer();
    void prepare();

    void init();
    bool initialized = false;

};

#endif /* LEDS_H_ */
