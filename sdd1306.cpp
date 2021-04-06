/*
Copyright 2019 Tinic Uro

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
#include "./sdd1306.h"
#include "./i2cmanager.h"

#include "M480.h"

#include <memory.h>

static constexpr
#include "font.h"

static void delay_us(int usec) {
    
    /* TIMER0 clock from LIRC */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & (~CLK_CLKSEL1_TMR0SEL_Msk)) | CLK_CLKSEL1_TMR0SEL_LIRC;
    CLK->APBCLK0 |= CLK_APBCLK0_TMR0CKEN_Msk;
    TIMER0->CTL = 0;
    TIMER0->INTSTS = (TIMER_INTSTS_TIF_Msk | TIMER_INTSTS_TWKF_Msk);   /* write 1 to clear for safety */
    TIMER0->CMP = usec / 100;
    TIMER0->CTL = (11 << TIMER_CTL_PSC_Pos) | TIMER_ONESHOT_MODE | TIMER_CTL_CNTEN_Msk;

    while (!TIMER0->INTSTS);
}

static const uint8_t rev_bits[] =
{
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

static SDD1306 sdd1306;

SDD1306::SDD1306() {
    memset(text_buffer_cache, 0, sizeof(text_buffer_cache));
    memset(text_buffer_screen, 0, sizeof(text_buffer_screen));
    memset(text_attr_cache, 0, sizeof(text_attr_cache));
    memset(text_attr_screen, 0, sizeof(text_attr_screen));
}
    
SDD1306 &SDD1306::instance() {
    if (!sdd1306.initialized) {
        sdd1306.initialized = true;
        sdd1306.Init();
    }
    return sdd1306;
}


void SDD1306::Clear() {
    memset(text_buffer_cache, 0, sizeof(text_buffer_cache));
    memset(text_buffer_screen, 0, sizeof(text_buffer_screen));
    memset(text_attr_cache, 0, sizeof(text_attr_cache));
    memset(text_attr_screen, 0, sizeof(text_attr_screen));
    center_flip_screen = 0;
    center_flip_cache = 0;
}
    
void SDD1306::ClearAttr() {
    memset(text_attr_cache, 0, sizeof(text_attr_cache));
}
    
void SDD1306::DisplayBootScreen() {
    for (uint32_t c=0; c<text_x_size*text_y_size; c++) {
        text_buffer_cache[c] = static_cast<uint16_t>(0x80 + c);
    }
}
    
void SDD1306::SetCenterFlip(int8_t progression) {
    center_flip_cache = progression;
}

void SDD1306::PlaceUTF8String(uint32_t x, uint32_t y, const char *s) {
    if (y>=text_y_size || x>=text_x_size) return;
	
	const uint8_t *str = reinterpret_cast<const uint8_t *>(s);

	static uint8_t trailingBytesForUTF8[256] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
	};

	static const uint32_t offsetsFromUTF8[6] = {
		0x00000000UL, 0x00003080UL, 0x000E2080UL,
		0x03C82080UL, 0xFA082080UL, 0x82082080UL
	};
	
    for (uint32_t c=0;;c++) {
    	if (*str == 0) {
    		return;
    	}
    	if ((x + c) >= text_x_size) {
    		return;
    	}
    	uint8_t nb  = trailingBytesForUTF8[static_cast<uint8_t>(*str)];
        uint32_t ch = 0;
        switch (nb) {
        case 5: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 4: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 3: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 2: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 1: ch += static_cast<uint32_t>(*str++); ch <<= 6;
        // fall through
        case 0: ch += static_cast<uint32_t>(*str++);
        // fall through
        }
        ch -= offsetsFromUTF8[nb];
        if ((ch < 0x20) || (ch > 0xFFFF)) {
            text_buffer_cache[y*text_x_size+x+c] = 0;
        } else {
            text_buffer_cache[y*text_x_size+x+c] = static_cast<uint16_t>(ch) - 0x20;
        }
    }
}

void SDD1306::Invert() {
    for (uint32_t c=0; c<text_x_size*text_y_size; c++) {
        text_attr_cache[c] ^= 1;
    }
}
    
void SDD1306::SetAttr(uint32_t x, uint32_t y, uint8_t attr) {
    if (y>=text_y_size || x>=text_x_size) return;
    text_attr_cache[y*text_x_size+x] = attr;
}

void SDD1306::SetBootScreen(bool on, int32_t xpos) {
	display_boot_screen = on;
	boot_screen_offset = xpos;
}
    
void SDD1306::Display() {

    bool display_center_flip = false;
    if (center_flip_cache || center_flip_screen) {
        center_flip_screen = center_flip_cache;
        display_center_flip = true;
    }
    for (uint32_t y=0; y<text_y_size; y++) {
        if (display_boot_screen) {

            WriteCommand(0xB0+0);
			WriteCommand(0x00);
			WriteCommand(0x10);

            uint8_t buf[97];
            buf[0] = 0x40;
                
            // write first line
            for (int32_t x = 0; x < (text_x_size*8); x++) {
                int32_t rx = (boot_screen_offset + x ) % (27 * 8) ;
                int32_t cx = rx >> 3;
                buf[x+1] = font_pbm[0x0EA8 + cx * 8 + (rx & 0x07)];
            }

            I2CManager::instance().write(i2caddr,  buf, 0x61);

            WriteCommand(0xB0+1);
			WriteCommand(0x00);
			WriteCommand(0x10);

            // write second line
            for (int32_t x = 0; x < (text_x_size*8); x++) {
                int32_t rx = (boot_screen_offset + x ) % (27 * 8) ;
                int32_t cx = rx >> 3;
                buf[x+1] = font_pbm[0x16A8 + cx * 8 + (rx & 0x07)];
            }
            I2CManager::instance().write(i2caddr, buf, 0x61);
        } else {
            for (uint32_t x=0; x<text_x_size; x++) {
                if (text_buffer_cache[y*text_x_size+x] != text_buffer_screen[y*text_x_size+x] ||
                    text_attr_cache[y*text_x_size+x] != text_attr_screen[y*text_x_size+x]) {
                    text_buffer_screen[y*text_x_size+x] = text_buffer_cache[y*text_x_size+x];
                    text_attr_screen[y*text_x_size+x] = text_attr_cache[y*text_x_size+x];
                    if (!display_center_flip) {
                        DisplayChar(x,y,text_buffer_screen[y*text_x_size+x],text_attr_screen[y*text_x_size+x]);
                    }
                }
            }
        }
    }
    if (display_center_flip) {
        DisplayCenterFlip();
    }

}
    
void SDD1306::SetVerticalShift(int8_t val) {
	vertical_shift = static_cast<int32_t>(val);
    WriteCommand(0xD3);
    if (val < 0) {
        val = 64+val;
        WriteCommand(val&0x3F);
    } else {
        WriteCommand(val&0x3F);
    }
}

void SDD1306::DisplayOn() {
    WriteCommand(0xAF);
}

void SDD1306::DisplayOff() {
    WriteCommand(0xAE);
}

void SDD1306::Init() {
    // Reset OLED screen
    PB1 = true; // OLED_CS
    PB0 = true; // OLED_RESET
    delay_us(1000);
    PB0 = false;
    delay_us(10000);
    PB0 = true;
    delay_us(20000);

    uint8_t status = 0x8;
    uint8_t value = 0x0;

    I2CManager::instance().write(i2caddr,  &status, 1);
    I2CManager::instance().read(i2caddr,  &value, 1);

    if (value == 0) {
        devicePresent = true;
    } else {
        return;
    }

    static uint8_t startup_sequence[] = {
        0xAE,           // Display off
        0xD5, 0x80,     // Set Display Clock Divide Ratio
        0xA8, 0x0F,     // Set Multiplex Ratio
        0xD3, 0x00,     // Set Display Offset
        0x8D, 0x14,     // Enable Charge Pump
        0x40,           // Set Display RAM start
        0xA6,           // Set to normal display (0xA7 == inverse)
        0xA4,           // Force Display From RAM On
        0xA1,           // Set Segment Re-map
        0xC8,           // Set COM Output Scan Direction (flipped)
        0xDA, 0x02,     // Set Pins configuration
        0x81, 0x80,     // Set Contrast (0x00-0xFF)
        0xD9, 0x10,     // Set Pre-Charge period
        0xDB, 0x40,     // Adjust Vcomm regulator output
        0xAF            // Display on
    };

    for (size_t c = 0; c < sizeof(startup_sequence); c++) {
        WriteCommand(startup_sequence[c]);
    }
}

void SDD1306::DisplayCenterFlip() {
    uint8_t buf[0x61];
    buf[0] = 0x40;
    for (uint32_t y=0; y<text_y_size; y++) {
        WriteCommand(static_cast<uint8_t>(0xB0+y));
        WriteCommand(0x00);
        WriteCommand(0x10);
        for (uint32_t x = 0; x < (text_x_size*8); x++) {
            if (center_flip_screen == (text_x_size*8/2)) {
                buf[x+1] = 0x00;
            } else {
                int32_t rx = ( ( ( static_cast<int32_t>(x) - (text_x_size*8/2) ) * (text_x_size*8/2) ) / static_cast<int32_t>((text_x_size*8/2) - center_flip_screen) ) + (text_x_size*8/2);
                if (rx < 0 || rx >= (text_x_size*8)) {
                    buf[x+1] = 0x00;
                } else {
                    uint8_t a = text_attr_screen[y*12+static_cast<uint32_t>(rx/8)];
                    uint8_t r = (a & 4) ? (7-(rx&7)) : (rx&7);
                    uint8_t v = font_pbm[text_buffer_screen[y*12+static_cast<uint32_t>(rx/8)]*8+r];
                    if (a & 1) {
                        v = ~v;
                    }
                    if (a & 2) {
                        v = rev_bits[v];
                    }
                    buf[x+1] = v;
                }
            }
        }
        I2CManager::instance().write(i2caddr,  buf, 0x61);
    }
}
    
void SDD1306::DisplayChar(uint32_t x, uint32_t y, uint16_t ch, uint8_t attr) {

    x = x * 8;

    WriteCommand(static_cast<uint8_t>(0xB0 + y));
    WriteCommand(static_cast<uint8_t>(0x0f&(x   )));
    WriteCommand(static_cast<uint8_t>(0x10|(x>>4)));
        
    uint8_t buf[9];
    buf[0] = 0x40;
    if ((attr & 4)) {
        if ((attr & 1)) {
            if ((attr & 2)) {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] = ~rev_bits[font_pbm[ch*8+7-c]];
                }
            } else {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] = ~font_pbm[ch*8+7-c];
                }
            }
        } else {
            if ((attr & 2)) {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] =  rev_bits[font_pbm[ch*8+7-c]];
                }
                } else {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] =  font_pbm[ch*8+7-c];
                }
            }
        }
    } else {
        if ((attr & 1)) {
            if ((attr & 2)) {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] = ~rev_bits[font_pbm[ch*8+c]];
                }
                } else {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] = ~font_pbm[ch*8+c];
                }
            }
        } else {
            if ((attr & 2)) {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] =  rev_bits[font_pbm[ch*8+c]];
                }
                } else {
                for (uint32_t c=0; c<8; c++) {
                    buf[c+1] =  font_pbm[ch*8+c];
                }
            }
        }
    }

    I2CManager::instance().write(i2caddr,  buf, 9);
}

void SDD1306::WriteCommand(uint8_t cmd_val) const {
    uint8_t cmd[2];
    cmd[0] = 0;
    cmd[1] = cmd_val;
    I2CManager::instance().write(i2caddr,  cmd, sizeof(cmd));
}