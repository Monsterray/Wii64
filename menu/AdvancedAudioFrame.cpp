// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Wii64 - AdvancedAudioFrame.cpp
 *
 * Wii64 homepage: http://www.emulatemii.com
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/
#include "MenuContext.h"
#include "AdvancedAudioFrame.h"
#include "SettingsFrame.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/CursorManager.h"
#include "../libgui/FocusManager.h"
#include "../main/wii64config.h"
#include <stdio.h>

static menu::Button *buttons[5];
static menu::TextBox *texts[8];
static char values[5][32];
static char *valueText[5] = { values[0], values[1], values[2], values[3], values[4] };
static char labels[8][80] = {
    "Audio Processing", "N64 synthesis resampler", "Output resampler",
    "Mixer precision", "Latency profile", "Audio synchronization",
    "Hi-Fi and Preserve Pitch: experimental CPU processing",
    "Low/Balanced cap queued PCM; excess audio can be dropped"
};
static char *labelText[8] = {
    labels[0], labels[1], labels[2], labels[3], labels[4], labels[5], labels[6], labels[7]
};
static char *settings[5] = {
    &audioQuality, &audioOutputResampler, &audioMixerPrecision, &audioLatency, &audioSync
};
static const char *names[5][3] = {
    { "Accurate", "Fast", "Hi-Fi (cubic)" },
    { "Wii DSP", "Hi-Fi (sinc)", NULL },
    { "Accurate", "Hi-Fi", NULL },
    { "Low", "Balanced", "Stable (legacy)" },
    { "Native Rate", "Follow Speed", "Preserve Pitch" }
};
static const int counts[5] = { 3, 2, 2, 3, 3 };

static void refresh(void)
{
    for (int i = 0; i < 5; i++) {
        int value = *settings[i];
        if (value < 0 || value >= counts[i]) value = 0;
        snprintf(values[i], sizeof(values[i]), "%s", names[i][value]);
    }
}
static void cycle(int row)
{
    *settings[row] = (*settings[row] + 1) % counts[row];
    refresh();
}
static void synthesis(void) { cycle(0); }
static void output(void) { cycle(1); }
static void mixer(void) { cycle(2); }
static void latency(void) { cycle(3); }
static void sync(void) { cycle(4); }
extern MenuContext *pMenuContext;
static void back(void)
{
    pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS, SettingsFrame::SUBMENU_AUDIO);
}

AdvancedAudioFrame::AdvancedAudioFrame()
{
    ButtonFunc functions[5] = { synthesis, output, mixer, latency, sync };
    for (int i = 0; i < 5; i++) {
        float y = 125 + 50 * i;
        buttons[i] = new menu::Button(BTN_A_NRM, &valueText[i], 330, y, 265, 42);
        buttons[i]->setFontSize(0.85f);
        buttons[i]->setClicked(functions[i]);
        buttons[i]->setReturn(back);
        buttons[i]->setActive(true);
        add(buttons[i]);
        menu::Cursor::getInstance().addComponent(this, buttons[i], 330, 595, y, y + 42);
    }
    for (int i = 0; i < 5; i++) {
        buttons[i]->setNextFocus(menu::Focus::DIRECTION_UP, buttons[(i + 4) % 5]);
        buttons[i]->setNextFocus(menu::Focus::DIRECTION_DOWN, buttons[(i + 1) % 5]);
    }
    for (int i = 0; i < 8; i++) {
        float x = i >= 1 && i <= 5 ? 40 : 320;
        float y = i == 0 ? 95 : i <= 5 ? 146 + (i - 1) * 50 : 395 + (i - 6) * 25;
        texts[i] = new menu::TextBox(&labelText[i], x, y, i == 0 ? 1.0f : 0.75f,
                                    !(i >= 1 && i <= 5));
        add(texts[i]);
    }
    refresh();
    setDefaultFocus(buttons[0]);
    setBackFunc(back);
    setEnabled(true);
}

AdvancedAudioFrame::~AdvancedAudioFrame()
{
    for (int i = 0; i < 8; i++) delete texts[i];
    for (int i = 0; i < 5; i++) {
        menu::Cursor::getInstance().removeComponent(this, buttons[i]);
        delete buttons[i];
    }
}

void AdvancedAudioFrame::activateSubmenu(int submenu)
{
    (void)submenu;
    refresh();
}
