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
#include <stdio.h>
#include <string.h>

const char* wii64Dir(void); // main/main_gc-menu2.cpp

/* Two buttons for each controller port. The first cycles the pak (a port holds one):
   Controller Pak, Rumble Pak, Transfer Pak, Bio Sensor, None. With a Transfer Pak, the
   second steps through the Game Boy cartridges (.gb, .gbc) in wii64/gb. */
static void cyclePak(int port);
static void nextCartridge(int port);
static void Func_Controller1Pak() { cyclePak(0); }
static void Func_Controller2Pak() { cyclePak(1); }
static void Func_Controller3Pak() { cyclePak(2); }
static void Func_Controller4Pak() { cyclePak(3); }
static void Func_Controller1Cart() { nextCartridge(0); }
static void Func_Controller2Cart() { nextCartridge(1); }
static void Func_Controller3Cart() { nextCartridge(2); }
static void Func_Controller4Cart() { nextCartridge(3); }
void Func_ReturnFromConfigurePaksFrame();

#define NUM_FRAME_BUTTONS 8
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
static const char* PakLabels[5] = { "Controller Pak", "Rumble Pak", "None", "Bio Sensor", "Transfer Pak" };
/* The cartridge buttons' text: the file name, shortened */
static char cartLabel[4][24];
static char* cartText[4] = { cartLabel[0], cartLabel[1], cartLabel[2], cartLabel[3] };
static char* noText = (char*)"";

struct ButtonInfo
{
	menu::Button	*button;
	int				buttonStyle;
	char**			buttonString;
	float			x;
	float			y;
	float			width;
	float			height;
	float			fontSize;
	ButtonFunc		clickedFunc;
	ButtonFunc		returnFunc;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonStyle	buttonString	x		y		width	height	font	clickFunc				returnFunc
	{	NULL,	BTN_A_NRM,	&noText,		215.0,	100.0,	205.0,	50.0,	0.9,	Func_Controller1Pak,	Func_ReturnFromConfigurePaksFrame }, // Controller 1: pak
	{	NULL,	BTN_A_NRM,	&noText,		215.0,	170.0,	205.0,	50.0,	0.9,	Func_Controller2Pak,	Func_ReturnFromConfigurePaksFrame }, // Controller 2: pak
	{	NULL,	BTN_A_NRM,	&noText,		215.0,	240.0,	205.0,	50.0,	0.9,	Func_Controller3Pak,	Func_ReturnFromConfigurePaksFrame }, // Controller 3: pak
	{	NULL,	BTN_A_NRM,	&noText,		215.0,	310.0,	205.0,	50.0,	0.9,	Func_Controller4Pak,	Func_ReturnFromConfigurePaksFrame }, // Controller 4: pak
	{	NULL,	BTN_A_NRM,	&cartText[0],	430.0,	100.0,	200.0,	50.0,	0.75,	Func_Controller1Cart,	Func_ReturnFromConfigurePaksFrame }, // Controller 1: cartridge
	{	NULL,	BTN_A_NRM,	&cartText[1],	430.0,	170.0,	200.0,	50.0,	0.75,	Func_Controller2Cart,	Func_ReturnFromConfigurePaksFrame }, // Controller 2: cartridge
	{	NULL,	BTN_A_NRM,	&cartText[2],	430.0,	240.0,	200.0,	50.0,	0.75,	Func_Controller3Cart,	Func_ReturnFromConfigurePaksFrame }, // Controller 3: cartridge
	{	NULL,	BTN_A_NRM,	&cartText[3],	430.0,	310.0,	200.0,	50.0,	0.75,	Func_Controller4Cart,	Func_ReturnFromConfigurePaksFrame }, // Controller 4: cartridge
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
	{	NULL,	FRAME_STRINGS[0],	115.0,	125.0,	 1.0,	true }, // Controller 1
	{	NULL,	FRAME_STRINGS[1],	115.0,	195.0,	 1.0,	true }, // Controller 2
	{	NULL,	FRAME_STRINGS[2],	115.0,	265.0,	 1.0,	true }, // Controller 3
	{	NULL,	FRAME_STRINGS[3],	115.0,	335.0,	 1.0,	true }, // Controller 4
	{	NULL,	FRAME_STRINGS[4],	317.0,	125.0,	 1.0,	true }, // Unavailable
	{	NULL,	FRAME_STRINGS[4],	317.0,	195.0,	 1.0,	true }, // Unavailable
	{	NULL,	FRAME_STRINGS[4],	317.0,	265.0,	 1.0,	true }, // Unavailable
	{	NULL,	FRAME_STRINGS[4],	317.0,	335.0,	 1.0,	true }, // Unavailable
};

static menu::Frame* thisFrame;

ConfigurePaksFrame::ConfigurePaksFrame()
{
	thisFrame = this;
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button = new menu::Button(FRAME_BUTTONS[i].buttonStyle, FRAME_BUTTONS[i].buttonString,
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y,
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);
		FRAME_BUTTONS[i].button->setFontSize(FRAME_BUTTONS[i].fontSize);
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

/* The cartridge button's text: the file name without folder and extension */
static void setCartLabel(int port)
{
	const char *path = transferPakRom[port];
	if (!path[0]) { snprintf(cartLabel[port], sizeof(cartLabel[port]), "No cartridge"); return; }
	const char *slash = strrchr(path, '/');
	const char *name = slash ? slash + 1 : path;
	const char *dot = strrchr(name, '.');
	int len = dot ? (int)(dot - name) : (int)strlen(name);
	if (len > 17) snprintf(cartLabel[port], sizeof(cartLabel[port]), "%.15s..", name);
	else snprintf(cartLabel[port], sizeof(cartLabel[port]), "%.*s", len, name);
}

void ConfigurePaksFrame::activateSubmenu(int submenu)
{
	Component* defaultFocus = this;
	int rows[4], count = 0;

	// Buttons for each port with a controller, "Unavailable" for the others
	for (int i = 0; i < 4; i++)
	{
		bool on = Controls[i].Present;
		bool cart = on && pakMode[i] == PAKMODE_TRANSFERPAK;
		FRAME_BUTTONS[i].button->setVisible(on);
		FRAME_BUTTONS[i].button->setActive(on);
		FRAME_BUTTONS[i+4].button->setVisible(cart);
		FRAME_BUTTONS[i+4].button->setActive(cart);
		FRAME_TEXTBOXES[i+4].textBox->setVisible(!on);
		if (!on) continue;
		FRAME_BUTTONS[i].button->setText((char**)&PakLabels[(int)pakMode[i] % 5]);
		setCartLabel(i);
		rows[count++] = i;
	}
	// Up and down go round the rows that are there; right and left between a row's buttons
	for (int k = 0; k < count; k++)
	{
		int i = rows[k];
		menu::Button *up = count > 1 ? FRAME_BUTTONS[rows[(k + count - 1) % count]].button : NULL;
		menu::Button *down = count > 1 ? FRAME_BUTTONS[rows[(k + 1) % count]].button : NULL;
		menu::Button *pak = FRAME_BUTTONS[i].button, *cart = FRAME_BUTTONS[i+4].button;
		pak->setNextFocus(menu::Focus::DIRECTION_UP, up);
		pak->setNextFocus(menu::Focus::DIRECTION_DOWN, down);
		pak->setNextFocus(menu::Focus::DIRECTION_RIGHT, pakMode[i] == PAKMODE_TRANSFERPAK ? cart : NULL);
		cart->setNextFocus(menu::Focus::DIRECTION_LEFT, pak);
		cart->setNextFocus(menu::Focus::DIRECTION_UP, up);
		cart->setNextFocus(menu::Focus::DIRECTION_DOWN, down);
	}
	if (count) defaultFocus = FRAME_BUTTONS[rows[0]].button;
	setDefaultFocus(defaultFocus);
}

/* Put the port's cartridge in (or take it out), and say why when it does not load. */
static void insertCartridge(int port)
{
	const char *why = transferpak_insert(port);
	if (!why) return;
	char msg[160];
	snprintf(msg, sizeof(msg), "Controller %d Transfer Pak:\n%s", port + 1, why);
	menu::MessageBox::getInstance().setMessage(msg);
}

static void cyclePak(int port)
{
	// Controller Pak -> Rumble Pak -> Transfer Pak -> Bio Sensor -> None -> Controller Pak
	// (indexed in PAKMODE_* order)
	static const char next[5] = { PAKMODE_RUMBLEPAK, PAKMODE_TRANSFERPAK, PAKMODE_MEMPAK,
	                              PAKMODE_NONE, PAKMODE_BIOSENSOR };
	bool transfer = pakMode[port] == PAKMODE_TRANSFERPAK;
	pakMode[port] = next[pakMode[port] % 5];
	Controls[port].Plugin = pak_plugin(pakMode[port]);
	if (transfer || pakMode[port] == PAKMODE_TRANSFERPAK) insertCartridge(port);
	thisFrame->activateSubmenu(ConfigurePaksFrame::SUBMENU_NONE);
}

static void nextCartridge(int port)
{
	char dir[32], path[256];
	snprintf(dir, sizeof(dir), "%sgb", wii64Dir());
	if (!tpak_next_rom(dir, transferPakRom[port], path, sizeof(path)))
	{
		char msg[96];
		snprintf(msg, sizeof(msg), "Put Game Boy cartridges (.gb, .gbc)\nin %s", dir);
		menu::MessageBox::getInstance().setMessage(msg);
		return;
	}
	snprintf(transferPakRom[port], sizeof(transferPakRom[port]), "%s", path);
	setCartLabel(port);
	insertCartridge(port);
}

extern MenuContext *pMenuContext;

void Func_ReturnFromConfigurePaksFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SETTINGS,SettingsFrame::SUBMENU_INPUT);
}
