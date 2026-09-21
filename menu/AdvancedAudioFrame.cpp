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
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../main/wii64config.h"

void Func_AudioQualityFast();
void Func_AudioQualityHiFi();
void Func_ReturnFromAdvancedAudioFrame();

#define NUM_FRAME_BUTTONS 2
#define FRAME_BUTTONS advancedAudioFrameButtons
#define FRAME_STRINGS advancedAudioFrameStrings
#define NUM_FRAME_TEXTBOXES 3
#define FRAME_TEXTBOXES advancedAudioFrameTextBoxes

static char FRAME_STRINGS[5][50] =
	{ "Fast",
	  "Hi-Fi",
	  "Advanced Sound Settings",
	  "Fast: lower CPU cost, some audio quality loss",
	  "Hi-Fi: full quality (default)"
};

struct ButtonInfo
{
	menu::Button	*button;
	int				buttonStyle;
	char*			buttonString;
	float			x;
	float			y;
	float			width;
	float			height;
	int				focusUp;
	int				focusDown;
	int				focusLeft;
	int				focusRight;
	ButtonFunc		clickedFunc;
	ButtonFunc		returnFunc;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[0],	210.0,	180.0,	100.0,	50.0,	-1,	-1,	 1,	 1,	Func_AudioQualityFast,	Func_ReturnFromAdvancedAudioFrame }, // Fast
	{	NULL,	BTN_A_SEL,	FRAME_STRINGS[1],	325.0,	180.0,	100.0,	50.0,	-1,	-1,	 0,	 0,	Func_AudioQualityHiFi,	Func_ReturnFromAdvancedAudioFrame }, // Hi-Fi
};

struct TextBoxInfo
{
	menu::TextBox	*textBox;
	char*			textBoxString;
	float			x;
	float			y;
	float			scale;
	bool			centered;
} FRAME_TEXTBOXES[NUM_FRAME_TEXTBOXES] =
{ //	textBox	textBoxString		x		y		scale	centered
	{	NULL,	FRAME_STRINGS[2],	320.0,	110.0,	 1.0,	true }, // Title
	{	NULL,	FRAME_STRINGS[3],	320.0,	260.0,	 0.75,	true }, // Fast description
	{	NULL,	FRAME_STRINGS[4],	320.0,	290.0,	 0.75,	true }, // Hi-Fi description
};

AdvancedAudioFrame::AdvancedAudioFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(FRAME_BUTTONS[i].buttonStyle, &FRAME_BUTTONS[i].buttonString,
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y,
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);

	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		if (FRAME_BUTTONS[i].focusUp != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[i].focusUp].button);
		if (FRAME_BUTTONS[i].focusDown != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[i].focusDown].button);
		if (FRAME_BUTTONS[i].focusLeft != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_LEFT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusLeft].button);
		if (FRAME_BUTTONS[i].focusRight != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusRight].button);
		FRAME_BUTTONS[i].button->setActive(true);
		if (FRAME_BUTTONS[i].clickedFunc) FRAME_BUTTONS[i].button->setClicked(FRAME_BUTTONS[i].clickedFunc);
		if (FRAME_BUTTONS[i].returnFunc) FRAME_BUTTONS[i].button->setReturn(FRAME_BUTTONS[i].returnFunc);
		add(FRAME_BUTTONS[i].button);
		menu::Cursor::getInstance().addComponent(this, FRAME_BUTTONS[i].button, FRAME_BUTTONS[i].x,
												FRAME_BUTTONS[i].x+FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].y,
												FRAME_BUTTONS[i].y+FRAME_BUTTONS[i].height);
	}

	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
	{
		FRAME_TEXTBOXES[i].textBox = new menu::TextBox(&FRAME_TEXTBOXES[i].textBoxString,
										FRAME_TEXTBOXES[i].x, FRAME_TEXTBOXES[i].y,
										FRAME_TEXTBOXES[i].scale, FRAME_TEXTBOXES[i].centered);
		add(FRAME_TEXTBOXES[i].textBox);
	}

	setDefaultFocus(FRAME_BUTTONS[0].button);
	setBackFunc(Func_ReturnFromAdvancedAudioFrame);
	setEnabled(true);
}

AdvancedAudioFrame::~AdvancedAudioFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
}

void AdvancedAudioFrame::activateSubmenu(int submenu)
{
	FRAME_BUTTONS[0].button->setSelected(false);
	FRAME_BUTTONS[1].button->setSelected(false);
	if (audioQuality == AUDIOQUALITY_FAST)	FRAME_BUTTONS[0].button->setSelected(true);
	else									FRAME_BUTTONS[1].button->setSelected(true);
}

void Func_AudioQualityFast()
{
	FRAME_BUTTONS[0].button->setSelected(true);
	FRAME_BUTTONS[1].button->setSelected(false);
	audioQuality = AUDIOQUALITY_FAST;
}

void Func_AudioQualityHiFi()
{
	FRAME_BUTTONS[0].button->setSelected(false);
	FRAME_BUTTONS[1].button->setSelected(true);
	audioQuality = AUDIOQUALITY_HIFI;
}

extern MenuContext *pMenuContext;

void Func_ReturnFromAdvancedAudioFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS, SettingsFrame::SUBMENU_AUDIO);
}
