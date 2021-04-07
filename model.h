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
#ifndef MODEL_H_
#define MODEL_H_

#include "./color.h"

class Model {
public:
    static Model &instance();

    uint32_t Effect() const { return effect; };
    void SetEffect(uint32_t _effect) { effect = _effect; };
    uint32_t EffectCount() const { return 3; }

    auto BirdColor() const { return bird_color; }
    void SetBirdColor(auto _bird_color) { bird_color = _bird_color; }

    auto RingColor() const { return ring_color; }
    void SetRingColor(auto _ring_color) { ring_color = _ring_color; }

    float Brightness() const { return brightness; }
    void SetBrightness(float _brightness) { brightness = _brightness; }

private:
    uint32_t effect = 0;
    const color::rgba<uint8_t> bird_color{0x7f,0x7f,0x00};
    const color::rgba<uint8_t> ring_color{0x00, 0x17, 0x7F};
    float brightness = 0.1f;

    void init();
    bool initialized = false;
};

#endif /* MODEL_H_ */
