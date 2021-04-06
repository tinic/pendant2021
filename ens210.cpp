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
#include "./ens210.h"

#include "M480.h"

#include "./i2cmanager.h"
#include "./timeline.h"

static void delay_us(int usec) {
    double t = Timeline::instance().SystemTime();
    while ( ( Timeline::instance().SystemTime() - t ) > (double(usec) * (1.0 / 1000000.0) ) ) { }
}

bool ENS210::devicePresent = false;

ENS210 &ENS210::instance() {
    static ENS210 ens210;
    if (!ens210.initialized) {
        ens210.initialized = true;
        ens210.init();
    }
    return ens210;
}

void ENS210::reset() {
    if (!devicePresent) return;
    static uint8_t th_reset[] = { 0x10, 0x80 };
    I2CManager::instance().write(i2c_addr, th_reset, 2);
    delay_us(2000);
    static uint8_t th_normal[] = { 0x10, 0x00 };
    I2CManager::instance().write(i2c_addr, th_normal, 2);
    delay_us(2000);
}

void ENS210::measure() {
    if (!devicePresent) return;
    static uint8_t th_start_single[] = { 0x21, 0x00, 0x03 };
    I2CManager::instance().write(i2c_addr, (uint8_t *)&th_start_single, sizeof(th_start_single));
}

void ENS210::wait() {
    if (!devicePresent) return;
    static uint8_t th_sens_stat = 0x24;
    static uint8_t th_stat = 0;
    do {
        I2CManager::instance().write(i2c_addr, (uint8_t *)&th_sens_stat, sizeof(th_sens_stat));
        I2CManager::instance().read(i2c_addr, (uint8_t *)&th_stat, sizeof(th_stat));
        delay_us(2000);
    } while (th_stat);
}

void ENS210::read() {
    if (!devicePresent) return;
    static uint8_t th_read = 0x30;
    static uint8_t th_data[6] = { 0, 0, 0, 0, 0, 0 };
    I2CManager::instance().write(i2c_addr, (uint8_t *)&th_read, sizeof(th_read));
    I2CManager::instance().read(i2c_addr, (uint8_t *)&th_data, sizeof(th_data));

    uint32_t t_val = (th_data[2]<<16) + (th_data[1]<<8) + (th_data[0]<<0);
    uint32_t h_val = (th_data[5]<<16) + (th_data[4]<<8) + (th_data[3]<<0);

    uint32_t t_data = (t_val>>0 ) & 0xffff;
    float TinK = (float)t_data / 64;
    float TinC = TinK - 273.15f;
    temperature = TinC;

    uint32_t h_data = (h_val>>0 ) & 0xffff;
    float H = (float)h_data/51200;
    humidity = H;
}

void ENS210::update() {
    if (!devicePresent) return;
    read();
    measure();
}

void ENS210::init() {
    if (!devicePresent) return;
    reset();
    measure();
    wait();
    read();
}
