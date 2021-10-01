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
#include "./timeline.h"
#include "./model.h"
#include "./seed.h"

#include "M480.h"

#include <limits>
#include <array>
#include <algorithm>

extern "C" {

static uint32_t systemSeconds = 0;

void TMR0_IRQHandler(void)
{
    if(TIMER_GetIntFlag(TIMER0)) {
        TIMER_ClearIntFlag(TIMER0);
        systemSeconds++;
    }
}

static bool effectReady = false;

void TMR1_IRQHandler(void)
{
    if(TIMER_GetIntFlag(TIMER1)) {
        TIMER_ClearIntFlag(TIMER1);
        effectReady = true;
    }
}

}

float Quad::easeIn (float t,float b , float c, float d) {
    t /= d;
    return c*t*t+b;
}
float Quad::easeOut(float t,float b , float c, float d) {
    t /= d;
    return -c*t*(t-2.0f)+b;
}

float Quad::easeInOut(float t,float b , float c, float d) {
    t /= d/2.0f;
    if (t < 1.0f) {
        return ((c/2.0f)*t*t)+b;
    } else {
        t -= 1.0f;
        return -c/2.0f*(((t-2)*t)-1.0f)+b;
    }
}

float Cubic::easeIn (float t, float b, float c, float d) {
    t /= d;
    return c*t*t*t+b;
}

float Cubic::easeOut(float t, float b, float c, float d) {
    t = t/d-1.0f;
    return c*(t*t*t+1.0f)+b;
}

float Cubic::easeInOut(float t, float b, float c, float d) {
    t /= d/2.0f;
    if (t < 1.0f) {
        return c/2.0f*t*t*t+b;
    } else {
        t -= 2.0f;
        return c/2.0f*(t*t*t+2.0f)+b;    
    }
}

Timeline &Timeline::instance() {
    static Timeline timeline;
    if (!timeline.initialized) {
        timeline.initialized = true;
        timeline.init();
    }
    return timeline;
}

bool Timeline::Scheduled(Timeline::Span &span) {
    for (Span *i = head; i ; i = i->next) {
        if ( i == &span ) {
            return true;
        }
    }
    return false;
}

void Timeline::Add(Timeline::Span &span) {
    for (Span *i = head; i ; i = i->next) {
        if ( i == &span ) {
            return;
        }
    }

    span.next = head;
    head = &span;
}

void Timeline::Remove(Timeline::Span &span) {
    Span *p = 0;
    for (Span *i = head; i ; i = i->next) {
        if ( i == &span ) {
            if (p) {
                p->next = i->next;
            } else {
                head = i->next;
            }
            i->next = 0;
            i->Done();
            return;
        }
        p = i;
    }
}

void Timeline::Process(Span::Type type) {
    static std::array<Span *, 64> collected;
    size_t collected_num = 0;
    double now = SystemTime();
    Span *p = 0;
    for (Span *i = head; i ; i = i->next) {
        if (i->type == type) {
            if ((i->time) <= now && !i->active) {
                i->active = true;
                i->Start();
            }
            switch (type) {
                case Span::Display: {
                    Display *display = static_cast<Display *>(i);
                    if (display->duration != std::numeric_limits<double>::infinity() && ((display->time + display->duration) < now)) {
                        if (p) {
                            p->next = display->next;
                        } else {
                            head = display->next;
                        }
                        if (collected_num < collected.size()) {
                            collected[collected_num++] = display;
                        }
                    }
                } break;
                case Span::Effect: {
                    Effect *effect = static_cast<Effect *>(i);
                    if (effect->duration != std::numeric_limits<double>::infinity() && ((effect->time + effect->duration) < now)) {
                        if (p) {
                            p->next = effect->next;
                        } else {
                            head = effect->next;
                        }
                        if (collected_num < collected.size()) {
                            collected[collected_num++] = effect;
                        }
                    }
                } break;
                case Span::Interval: {
                    Interval *interval = static_cast<Interval *>(i);
                    if (interval->duration != std::numeric_limits<double>::infinity() && ((interval->time + interval->duration) < now)) {
                        // Reschedule
                        if (interval->intervalFuzz != 0.0) {
                            std::uniform_real_distribution<> dis(interval->interval, interval->interval + interval->intervalFuzz);
                            interval->time += dis(gen);
                        } else {
                            interval->time += interval->interval;
                        }
                        interval->active = false;
                        interval->Done();
                    }
                } break;
                case Span::None: {
                } break;
            }
        }
        p = i;
    }
    switch (type) {
        case Span::Display:
        case Span::Effect: {
            for (size_t c = 0; c < collected_num; c++) {
                collected[c]->active = false;
                collected[c]->next = 0;
                collected[c]->Done();
            }
        } break;
        case Span::Interval: {
        } break;
        case Span::None: {
        } break;
    }
}

Timeline::Span &Timeline::Top(Span::Type type) const {
    static Timeline::Span empty;
    double time = SystemTime();
    for (Span *i = head; i ; i = i->next) {
        if ((i->type == type) &&
            (i->time <= time) &&
            ( (i->duration == std::numeric_limits<double>::infinity()) || ((i->time + i->duration) > time) ) ) {
            return *i;
        }
    }
    return empty;
}

Timeline::Span &Timeline::Below(Span *context, Span::Type type) const {
    static Timeline::Span empty;
    double time = SystemTime();
    for (Span *i = head; i ; i = i->next) {
        if (i == context) {
            continue;
        }
        if ((i->type == type) &&
            (i->time <= time) &&
            ( (i->duration == std::numeric_limits<double>::infinity()) || ((i->time + i->duration) > time) ) ) {
            return *i;
        }
    }
    return empty;
}

std::tuple<bool, float> Timeline::Effect::InAttackPeriod() const {
    double now = Timeline::SystemTime();
    if ( (now - time) < static_cast<double>(attack)) {
        return {true, static_cast<float>((now - time) * (1.0 / static_cast<double>(attack))) };
    }
    return {false, 0.0f};
}

std::tuple<bool, float> Timeline::Effect::InDecayPeriod() const {
    double now = Timeline::SystemTime();
    if (!std::get<0>(InAttackPeriod())) {
        if ( (now - time) < static_cast<double>(attack + decay)) {
            return {true, static_cast<float>((now - time) * (1.0 / static_cast<double>(decay))) };
        }
    }
    return {false, 0.0f};
}

std::tuple<bool, float> Timeline::Effect::InSustainPeriod() const {
    double now = Timeline::SystemTime();
    if (!std::get<0>(InDecayPeriod())) {
        double sustain = duration - attack - decay - release;
        if ( (now - time) < static_cast<double>(attack + decay + sustain)) {
            return {true, static_cast<float>((now - time) * (1.0 / static_cast<double>(sustain))) };
        }
    }
    return {false, 0.0f};
}

std::tuple<bool, float>  Timeline::Effect::InReleasePeriod() const {
    double now = Timeline::SystemTime();
    if (!std::get<0>(InSustainPeriod())) {
        double sustain = duration - attack - decay - release;
        if ( (now - time) < static_cast<double>(attack + decay + sustain + release)) {
            return { true, 1.0f - static_cast<float>(((time + duration) - now) * (1.0 / static_cast<double>(release))) };
        }
    }
    return {false, 0.0f};
}

void Timeline::ProcessEffect()
{
    return Process(Span::Effect);
}

void Timeline::ProcessDisplay()
{
    return Process(Span::Display);
}

void Timeline::ProcessInterval()
{
    return Process(Span::Interval);
}

Timeline::Effect &Timeline::TopEffect() const
{
    return static_cast<Effect&>(Top(Span::Effect));
}

Timeline::Display &Timeline::TopDisplay() const
{
    return static_cast<Display&>(Top(Span::Display));
}

Timeline::Interval &Timeline::TopInterval() const
{
    return static_cast<Interval&>(Top(Span::Interval));
}

double Timeline::SystemTime() {
    return double(systemSeconds) + (double(TIMER0->CNT) / double(TIMER0->CMP));
}

uint64_t Timeline::FastSystemTime() {
    return (uint64_t(systemSeconds) * uint64_t(TIMER0->CMP)) + uint64_t(TIMER0->CNT);
}

uint64_t Timeline::FastSystemTimeCmp() {
    return uint64_t(TIMER0->CMP);
}

static bool idleReady = false;
static bool backgroundReady = false;
static bool displayReady = false;

bool Timeline::CheckEffectReadyAndClear() {
    static size_t frameCount = 0;
    if (effectReady) {
        effectReady = false;
        idleReady = (frameCount % size_t(effectRate * idleRate)) == 0;
        backgroundReady = (frameCount % size_t(effectRate / backgroundRate)) == 0;
        displayReady = (frameCount % size_t(effectRate / displayRate)) == 0;
        frameCount ++;
        return true;
    }
    return false;
}

bool Timeline::CheckDisplayReadyAndClear() {
    if (displayReady) {
        displayReady = false;
        return true;
    }
    return false;
}

bool Timeline::CheckBackgroundReadyAndClear() {
    if (backgroundReady) {
        backgroundReady = false;
        return true;
    }
    return false;
}

bool Timeline::CheckIdleReadyAndClear() {
    if (idleReady) {
        idleReady = false;
        return true;
    }
    return false;
}

void Timeline::init() {
    // SystemTime timer
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 1);
    TIMER_EnableInt(TIMER0);
    NVIC_SetPriority(TMR0_IRQn, 1);
    NVIC_EnableIRQ(TMR0_IRQn);
    TIMER_Start(TIMER0);

    // Effect Frame rate timer
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, int32_t(effectRate));
    TIMER_EnableInt(TIMER1);
    NVIC_SetPriority(TMR1_IRQn, 2);
    NVIC_EnableIRQ(TMR1_IRQn);
    TIMER_Start(TIMER1);

    gen.seed(Seed::instance().seedU32());
}
