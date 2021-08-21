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
    void SetEffect(uint32_t _effect) { effect = _effect % EffectCount(); dirty = true; };
    uint32_t EffectCount() const { return 3; }

    auto BirdColor() const { return bird_color; }
    void SetBirdColor(auto _bird_color) { bird_color = _bird_color; dirty = true; }

    auto RingColor() const { return ring_color; }
    void SetRingColor(auto _ring_color) { ring_color = _ring_color;  dirty = true; }

    float Brightness() const { return brightness; }
    void SetBrightness(float _brightness) { brightness = _brightness;  dirty = true; }

    size_t Switch1Count() const { return switch1Count; }
    void IncSwitch1Count() { switch1Count++; dirty = true; }

    size_t Switch2Count() const { return switch2Count; }
    void IncSwitch2Count() { switch2Count++; dirty = true; }

    size_t Switch3Count() const { return switch3Count; }
    void IncSwitch3Count() { switch3Count++; dirty = true; }

    size_t BootCount() const { return bootCount; }
    void IncBootCount() { bootCount++; dirty = true; }

    size_t IntCount() const { return intCount; }
    void SetIntCount(uint16_t count) { if ( intCount != count) { intCount = count; dirty = true; } }

    size_t DselCount() const { return dselCount; }
    void IncDselCount() { dselCount++; dirty = true; }

    void load();
    void save();

private:
    static bool dirty;
    static bool initialized;
    static constexpr uint32_t dataAddress = 0x3F000; // Last 4KB page

    void init();

    uint32_t version = 0;

    uint32_t effect = 0;
    const color::rgba<uint8_t> bird_color{0x7f, 0x7f, 0x00};
    const color::rgba<uint8_t> ring_color{0x00, 0x17, 0x7F};
    float brightness = 0.1f;

    size_t switch1Count = 0;
    size_t switch2Count = 0;
    size_t switch3Count = 0;

    size_t bootCount = 0;
    size_t intCount = 0;
    size_t dselCount = 0;
};

#endif /* MODEL_H_ */
