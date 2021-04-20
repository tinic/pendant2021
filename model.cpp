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

#include "NuMicro.h"

const uint32_t currentVersion = 0x1ED50001;

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
    FMC_Open();

    if (currentVersion == FMC_Read(dataAddress) ) {
        uint32_t *self = reinterpret_cast<uint32_t *>(this);
        for (size_t c = 0; c < sizeof(this); c += sizeof(uint32_t)) {
            *self++ = FMC_Read(dataAddress+c);
        }
    } else {
        dirty = true;
    }

    FMC_Close();
}

void Model::save() {
    if (!dirty) {
        return;
    }

    FMC_Open();

    dirty = false;

    FMC_ENABLE_AP_UPDATE();

    uint32_t au32Config[2];
    FMC_ReadConfig(au32Config, 2);

    if (!((!(au32Config[0] & 0x1)) && 
            (au32Config[1] == dataAddress))) {

        SYS_UnlockReg();

        FMC_ENABLE_CFG_UPDATE();

        au32Config[0] &= ~0x1;
        au32Config[1] = dataAddress;

        FMC_WriteConfig(au32Config, 2);

        SYS_ResetCPU();

        FMC_DISABLE_CFG_UPDATE();

        SYS_LockReg();

        return;
    }

    FMC_Erase(dataAddress);
    FMC_WriteMultiple(dataAddress, reinterpret_cast<uint32_t *>(this), sizeof(this));

    FMC_DISABLE_AP_UPDATE();

    FMC_Close();
}
