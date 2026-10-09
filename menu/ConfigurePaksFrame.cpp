/**
 * Wii64 - ConfigurePaksFrame.cpp
 * Copyright (C) 2009, 2010 sepp256
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
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
#include "ConfigurePaksFrame.h"
#include "../libgui/Button.h"
#include "../libgui/TextBox.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"

#include "../main/wii64config.h"

extern "C" {
#include "../main/plugin.h"
#include "../main/rom.h"
#include "../gc_memory/pif.h"
}

/* One button for each controller port. A port holds one pak, so A cycles it:
   Controller Pak, Rumble Pak, Bio Sensor, None. */
static void cyclePak(int port);
static void Func_Controller1Pak() { cyclePak(0); }
static void Func_Controller2Pak() { cyclePak(1); }
static void Func_Controller3Pak() { cyclePak(2); }
static void Func_Controller4Pak() { cyclePak(3); }
void Func_ReturnFromConfigurePaksFrame();

#define NUM_FRAME_BUTTONS 4
#define FRAME_BUTTONS configurePaksFrameButtons
#define FRAME_STRINGS configurePaksFrameStrings
#define NUM_FRAME_TEXTBOXES 8
#define FRAME_TEXTBOXES configurePaksFrameTextBoxes

static char FRAME_STRINGS[5][13] =
	{ "Controller 1",
	  "Controller 2",
	  "Controller 3",
	  "Controller 4",
	  "Unavailable"
};

/* In PAKMODE_* order */
static const char* PakLabels[4] = { "Controller Pak", "Rumble Pak", "None", "Bio Sensor" };

struct ButtonInfo
{
	menu::Button	*button;
	int				buttonStyle;
	char*			buttonString;
	float			x;
	float			y;
	float			width;
	float			height;
	ButtonFunc		clickedFunc;
	ButtonFunc		returnFunc;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle	buttonString		x		y		width	height	clickFunc				returnFunc
	{	NULL,	BTN_A_NRM,	(char*)"None",		330.0,	100.0,	230.0,	50.0,	Func_Controller1Pak,	Func_ReturnFromConfigurePaksFrame }, // Controller 1
	{	NULL,	BTN_A_NRM,	(char*)"None",		330.0,	170.0,	230.0,	50.0,	Func_Controller2Pak,	Func_ReturnFromConfigurePaksFrame }, // Controller 2
	{	NULL,	BTN_A_NRM,	(char*)"None",		330.0,	240.0,	230.0,	50.0,	Func_Controller3Pak,	Func_ReturnFromConfigurePaksFrame }, // Controller 3
	{	NULL,	BTN_A_NRM,	(char*)"None",		330.0,	310.0,	230.0,	50.0,	Func_Controller4Pak,	Func_ReturnFromConfigurePaksFrame }, // Controller 4
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
	{	NULL,	FRAME_STRINGS[0],	180.0,	125.0,	 1.0,	true }, // Controller 1
	{	NULL,	FRAME_STRINGS[1],	180.0,	195.0,	 1.0,	true }, // Controller 2
	{	NULL,	FRAME_STRINGS[2],	180.0,	265.0,	 1.0,	true }, // Controller 3
	{	NULL,	FRAME_STRINGS[3],	180.0,	335.0,	 1.0,	true }, // Controller 4
	{	NULL,	FRAME_STRINGS[4],	445.0,	125.0,	 1.0,	true }, // Unavailable
	{	NULL,	FRAME_STRINGS[4],	445.0,	195.0,	 1.0,	true }, // Unavailable
	{	NULL,	FRAME_STRINGS[4],	445.0,	265.0,	 1.0,	true }, // Unavailable
	{	NULL,	FRAME_STRINGS[4],	445.0,	335.0,	 1.0,	true }, // Unavailable
};

ConfigurePaksFrame::ConfigurePaksFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button = new menu::Button(FRAME_BUTTONS[i].buttonStyle, &FRAME_BUTTONS[i].buttonString,
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y,
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);
		FRAME_BUTTONS[i].button->setActive(true);
		FRAME_BUTTONS[i].button->setClicked(FRAME_BUTTONS[i].clickedFunc);
		FRAME_BUTTONS[i].button->setReturn(FRAME_BUTTONS[i].returnFunc);
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
	setBackFunc(Func_ReturnFromConfigurePaksFrame);
	setEnabled(true);
	activateSubmenu(SUBMENU_NONE);
}

ConfigurePaksFrame::~ConfigurePaksFrame()
{
	for (int i = 0; i < NUM_FRAME_TEXTBOXES; i++)
		delete FRAME_TEXTBOXES[i].textBox;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
}

void ConfigurePaksFrame::activateSubmenu(int submenu)
{
	Component* defaultFocus = this;
	menu::Button* present[4];
	int count = 0;

	// A button for each port with a controller, "Unavailable" for the others
	for (int i = 0; i < 4; i++)
	{
		bool on = Controls[i].Present;
		FRAME_BUTTONS[i].button->setVisible(on);
		FRAME_BUTTONS[i].button->setActive(on);
		FRAME_TEXTBOXES[i+4].textBox->setVisible(!on);
		if (!on) continue;
		FRAME_BUTTONS[i].button->setText((char**)&PakLabels[(int)pakMode[i] & 3]);
		present[count++] = FRAME_BUTTONS[i].button;
	}
	// Up and down go round the buttons that are there
	for (int i = 0; i < count; i++)
	{
		present[i]->setNextFocus(menu::Focus::DIRECTION_UP, count > 1 ? present[(i + count - 1) % count] : NULL);
		present[i]->setNextFocus(menu::Focus::DIRECTION_DOWN, count > 1 ? present[(i + 1) % count] : NULL);
	}
	if (count) defaultFocus = present[0];
	setDefaultFocus(defaultFocus);
}

static void cyclePak(int port)
{
	// Controller Pak -> Rumble Pak -> Bio Sensor -> None -> Controller Pak (PAKMODE_* order)
	static const char next[4] = { PAKMODE_RUMBLEPAK, PAKMODE_BIOSENSOR, PAKMODE_MEMPAK, PAKMODE_NONE };
	pakMode[port] = next[pakMode[port] & 3];
	Controls[port].Plugin = pak_plugin(pakMode[port]);
	FRAME_BUTTONS[port].button->setText((char**)&PakLabels[(int)pakMode[port]]);
}

extern MenuContext *pMenuContext;

void Func_ReturnFromConfigurePaksFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_INPUT);
}
