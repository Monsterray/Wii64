/**
 * Wii64 - controller-Classic.c
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2010 sepp256
 * 
 * Classic controller input module
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                sepp256@gmail.com
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


#include <string.h>
#include <malloc.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include "controller.h"
#include "n64_analog.h"
#include "../gui/DEBUG.h"

/* A stick as calibrated fractions (n64_analog.h). This used to go through wiiuse's
   angle/magnitude, divide the magnitude by 0.667 and cut a 10% dead zone per axis: full
   deflection arrived at two thirds of the throw, the last third did nothing, and small
   movements snapped to the axes. */
#define JS_FRAC_X(j) stick_frac((j)->pos.x, (j)->min.x, (j)->center.x, (j)->max.x)
#define JS_FRAC_Y(j) stick_frac((j)->pos.y, (j)->min.y, (j)->center.y, (j)->max.y)

static void js_stick(joystick_t* j, signed char* x, signed char* y)
{
	cal_stick(j->pos.x, j->pos.y, j->min.x, j->min.y, j->center.x, j->center.y,
	          j->max.x, j->max.y, x, y);
}

enum {
	L_STICK_AS_ANALOG = 1, R_STICK_AS_ANALOG = 2,
};

enum {
	L_STICK_L = 0x01 << 16,
	L_STICK_R = 0x02 << 16,
	L_STICK_U = 0x04 << 16,
	L_STICK_D = 0x08 << 16,
	R_STICK_L = 0x10 << 16,
	R_STICK_R = 0x20 << 16,
	R_STICK_U = 0x40 << 16,
	R_STICK_D = 0x80 << 16,
};

static button_t buttons[] = {
	{  0, ~0,                         "None" },
	{  1, CLASSIC_CTRL_BUTTON_UP,     "D-Up" },
	{  2, CLASSIC_CTRL_BUTTON_LEFT,   "D-Left" },
	{  3, CLASSIC_CTRL_BUTTON_RIGHT,  "D-Right" },
	{  4, CLASSIC_CTRL_BUTTON_DOWN,   "D-Down" },
	{  5, CLASSIC_CTRL_BUTTON_FULL_L, "L" },
	{  6, CLASSIC_CTRL_BUTTON_FULL_R, "R" },
	{  7, CLASSIC_CTRL_BUTTON_ZL,     "Left Z" },
	{  8, CLASSIC_CTRL_BUTTON_ZR,     "Right Z" },
	{  9, CLASSIC_CTRL_BUTTON_A,      "A" },
	{ 10, CLASSIC_CTRL_BUTTON_B,      "B" },
	{ 11, CLASSIC_CTRL_BUTTON_X,      "X" },
	{ 12, CLASSIC_CTRL_BUTTON_Y,      "Y" },
	{ 13, CLASSIC_CTRL_BUTTON_PLUS,   "+" },
	{ 14, CLASSIC_CTRL_BUTTON_MINUS,  "-" },
	{ 15, CLASSIC_CTRL_BUTTON_HOME,   "Home" },
	{ 16, R_STICK_U,                  "RS-Up" },
	{ 17, R_STICK_L,                  "RS-Left" },
	{ 18, R_STICK_R,                  "RS-Right" },
	{ 19, R_STICK_D,                  "RS-Down" },
	{ 20, L_STICK_U,                  "LS-Up" },
	{ 21, L_STICK_L,                  "LS-Left" },
	{ 22, L_STICK_R,                  "LS-Right" },
	{ 23, L_STICK_D,                  "LS-Down" },
};

static button_t analog_sources[] = {
	{ 0, L_STICK_AS_ANALOG,  "Left Stick" },
	{ 1, R_STICK_AS_ANALOG,  "Right Stick" },
	{ 2, BUTTON_AS_ANALOG,   "D-Pad" },
};

static button_t menu_combos[] = {
	{ 0, CLASSIC_CTRL_BUTTON_X|CLASSIC_CTRL_BUTTON_Y, "X+Y" },
	{ 1, CLASSIC_CTRL_BUTTON_ZL|CLASSIC_CTRL_BUTTON_ZR, "ZL+ZR" },
	{ 2, CLASSIC_CTRL_BUTTON_HOME, "Home" },
};

static unsigned int getButtons(classic_ctrl_t* controller)
{
	unsigned int b = (unsigned short)controller->btns;
	float stickX    = JS_FRAC_X(&controller->ljs);
	float stickY    = JS_FRAC_Y(&controller->ljs);
	float substickX = JS_FRAC_X(&controller->rjs);
	float substickY = JS_FRAC_Y(&controller->rjs);

	if(stickX    < -0.5f) b |= L_STICK_L;
	if(stickX    >  0.5f) b |= L_STICK_R;
	if(stickY    >  0.5f) b |= L_STICK_U;
	if(stickY    < -0.5f) b |= L_STICK_D;

	if(substickX < -0.5f) b |= R_STICK_L;
	if(substickX >  0.5f) b |= R_STICK_R;
	if(substickY >  0.5f) b |= R_STICK_U;
	if(substickY < -0.5f) b |= R_STICK_D;

	return b;
}

static int available(int Control) {
	// WPADData is already refreshed by _GetKeys' WPAD_ScanPads/WPAD_Data
	// right before this is called, so reading the expansion type out of it
	// is free -- WPAD_Probe() here was a second IOS/IPC round trip per
	// controller per input poll.
	WPADData* wpad = WPAD_Data(Control);
	int err = wpad->err;
	u32 expType = wpad->exp.type;
	if(err == WPAD_ERR_NONE &&
	   expType == WPAD_EXP_CLASSIC){
		controller_Classic.available[Control] = 1;
		return 1;
	} else {
		controller_Classic.available[Control] = 0;
		if(err == WPAD_ERR_NONE &&
		   expType == WPAD_EXP_NUNCHUK){
			controller_WiimoteNunchuk.available[Control] = 1;
		}
		else if (err == WPAD_ERR_NONE &&
		   expType == WPAD_EXP_NONE){
			controller_Wiimote.available[Control] = 1;
		}
		return 0;
	}
}

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	if(wpadNeedScan){ WPAD_ScanPads(); wpadNeedScan = 0; }
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected classic controller
	if(!available(Control))
		return 0;

	unsigned int b = getButtons(&wpad->exp.classic);
	inline int isHeld(button_tp button){
		return (b & button->mask) == button->mask;
	}
	
	c->R_DPAD       = isHeld(config->DR);
	c->L_DPAD       = isHeld(config->DL);
	c->D_DPAD       = isHeld(config->DD);
	c->U_DPAD       = isHeld(config->DU);
	
	c->START_BUTTON = isHeld(config->START);
	c->B_BUTTON     = isHeld(config->B);
	c->A_BUTTON     = isHeld(config->A);

	c->Z_TRIG       = isHeld(config->Z);
	c->R_TRIG       = isHeld(config->R);
	c->L_TRIG       = isHeld(config->L);

	c->R_CBUTTON    = isHeld(config->CR);
	c->L_CBUTTON    = isHeld(config->CL);
	c->D_CBUTTON    = isHeld(config->CD);
	c->U_CBUTTON    = isHeld(config->CU);

	signed char x = 0, y = 0;
	if(config->analog->mask == L_STICK_AS_ANALOG)
		js_stick(&wpad->exp.classic.ljs, &x, &y);
	else if(config->analog->mask == R_STICK_AS_ANALOG)
		js_stick(&wpad->exp.classic.rjs, &x, &y);
	else if(config->analog->mask == BUTTON_AS_ANALOG)
		button_stick(!!(b & CLASSIC_CTRL_BUTTON_RIGHT) - !!(b & CLASSIC_CTRL_BUTTON_LEFT),
		             !!(b & CLASSIC_CTRL_BUTTON_UP) - !!(b & CLASSIC_CTRL_BUTTON_DOWN), &x, &y);
	c->X_AXIS = x;
	c->Y_AXIS = config->invertedY ? -y : y;

	//DEBUG_print(txtbuffer,DBG_RSPINFO1+Control);

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static void pause(int Control){
	WPAD_Rumble(Control, 0);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	WPAD_Rumble(Control, rumble ? 1 : 0);
}

static void configure(int Control, controller_config_t* config){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// v is the N64 port (0-3); WPAD_LED_1..4 are 0x01,0x02,0x04,0x08, so
	// player N+1 lights LED N+1 only, matching the System Menu/most games'
	// convention rather than a binary player-count encoding.
	WPAD_ControlLed(p, 1 << v);
}

static void refreshAvailable(void);

controller_t controller_Classic =
	{ 'C',
	  _GetKeys,
	  configure,
	  assign,
	  pause,
	  resume,
	  rumble,
	  refreshAvailable,
	  {0, 0, 0, 0},
	  sizeof(buttons)/sizeof(buttons[0]),
	  buttons,
	  sizeof(analog_sources)/sizeof(analog_sources[0]),
	  analog_sources,
	  sizeof(menu_combos)/sizeof(menu_combos[0]),
	  menu_combos,
	  { .DU        = &buttons[1],  // D-Pad Up
	    .DL        = &buttons[2],  // D-Pad Left
	    .DR        = &buttons[3],  // D-Pad Right
	    .DD        = &buttons[4],  // D-Pad Down
	    .Z         = &buttons[5],  // Left Z
	    .L         = &buttons[8],  // Left Trigger
	    .R         = &buttons[6],  // Right Trigger
	    .A         = &buttons[9],  // A
	    .B         = &buttons[10], // B
	    .START     = &buttons[13], // +
	    .CU        = &buttons[16], // Right Stick Up
	    .CL        = &buttons[17], // Right Stick Left
	    .CR        = &buttons[18], // Right Stick Right
	    .CD        = &buttons[19], // Right Stick Down
	    .analog    = &analog_sources[0],
	    .exit      = &menu_combos[2],
	    .invertedY = 0,
	  },
	  {{0}}, {{0}}
	 };

static void refreshAvailable(void){

	int i, err;
	u32 expType;
	WPAD_ScanPads();
	for(i=0; i<4; ++i){
		err = WPAD_Probe(i, &expType);
		if(err == WPAD_ERR_NONE &&
		   expType == WPAD_EXP_CLASSIC){
			controller_Classic.available[i] = 1;
			WPAD_SetDataFormat(i, WPAD_DATA_EXPANSION);
		} else
			controller_Classic.available[i] = 0;
	}
}
