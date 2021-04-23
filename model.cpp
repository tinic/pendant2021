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
#include "./model.h"
#include "./leds.h"

#include "NuMicro.h"

#include <memory.h>

const uint32_t currentVersion = 0x1ED50002;

bool Model::dirty = false;
bool Model::initialized = false;

Model &Model::instance() {
    static Model model;
    if (!model.initialized) {
        model.initialized = true;
        model.init();
    }
    return model;
}

void Model::init() {
    load();
}

void Model::load() {
    SYS_UnlockReg();

    FMC_Open();

    uint32_t au32Config[2];
    memset(au32Config, 0, sizeof(au32Config));
    FMC_ReadConfig(au32Config, 2);

    if (currentVersion == FMC_Read(dataAddress) ) {
        uint32_t *self = reinterpret_cast<uint32_t *>(this);
        for (size_t c = 0; c < sizeof(Model); c += sizeof(uint32_t)) {
            *self++ = FMC_Read(dataAddress+c);
        }
    } else {
        dirty = true;
    }

    FMC_Close();

    SYS_LockReg();
}

void Model::save() {
    if (!dirty) {
        return;
    }

    printf("Saving model.\r\n");

    Leds::instance().forceStop();

    dirty = false;

    version = currentVersion;

    SYS_UnlockReg();

    FMC_Open();

    uint32_t au32Config[2];
    memset(au32Config, 0, sizeof(au32Config));
    FMC_ReadConfig(au32Config, 2);

    if (!(((au32Config[0] & 1) == 0) && 
           (au32Config[1] == dataAddress))) {

        au32Config[0] &= ~1;
        au32Config[1]  = dataAddress;

        FMC_WriteConfig(au32Config, 2);

        SYS_ResetChip();

        for(;;) { }
    }

    memset(au32Config, 0, sizeof(au32Config));
    FMC_ReadConfig(au32Config, 2);

    FMC_ENABLE_AP_UPDATE();

    __disable_irq();
    FMC_Erase(dataAddress);
    uint32_t *self = reinterpret_cast<uint32_t *>(this);
    for (size_t c = 0; c < sizeof(Model); c += sizeof(uint32_t)) {
        FMC_Write(dataAddress+c, *self++);
    }
    __enable_irq();

    FMC_DISABLE_AP_UPDATE();

    FMC_Close();

    SYS_LockReg();

}
