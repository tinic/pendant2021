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

class Leds {
public:
    static constexpr size_t circleLedsN = 32;
    static constexpr size_t birdLedsN = 8;

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
        }

        vector::float4 get(size_t index) const {
            index %= circleLedsN + birdLedsN;
            return map[index];
        }

    private:
        vector::float4 map[circleLedsN + birdLedsN];
    } map;

private:
    std::array<std::array<vector::float4, circleLedsN>, 2> circleLeds;
    std::array<std::array<vector::float4, birdLedsN>, 2> birdLeds;

    static constexpr size_t bitsPerComponent = 8;
    static constexpr size_t bytesPerColor = ( 48 * bitsPerComponent ) / 8;

    std::array<std::array<uint8_t, circleLedsN * bytesPerColor>, 2> circleLedsDMABuf;
    std::array<std::array<uint8_t, birdLedsN * bytesPerColor>, 2> birdsLedsDMABuf;

    void transfer();
    void prepare();

    void init();
    bool initialized = false;
};

#endif /* LEDS_H_ */
