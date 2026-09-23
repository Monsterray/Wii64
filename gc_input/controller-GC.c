/**
 * Wii64 - controller-GC.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 sepp256
 * 
 * Gamecube controller input module
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogc/pad.h>
#include "controller.h"
#include "n64_analog.h"
#include "../main/perf_prof.h"

enum {
	ANALOG_AS_ANALOG = 1, C_STICK_AS_ANALOG = 2,
};

enum {
	ANALOG_L  = 0x01 << 16,
	ANALOG_R  = 0x02 << 16,
	ANALOG_U  = 0x04 << 16,
	ANALOG_D  = 0x08 << 16,
	C_STICK_L = 0x10 << 16,
	C_STICK_R = 0x20 << 16,
	C_STICK_U = 0x40 << 16,
	C_STICK_D = 0x80 << 16,
};

static button_t buttons[] = {
	{  0, ~0,                "None" },
	{  1, PAD_BUTTON_UP,    "D-Up" },
	{  2, PAD_BUTTON_LEFT,  "D-Left" },
	{  3, PAD_BUTTON_RIGHT, "D-Right" },
	{  4, PAD_BUTTON_DOWN,  "D-Down" },
	{  5, PAD_TRIGGER_Z,    "Z" },
	{  6, PAD_TRIGGER_L,    "L" },
	{  7, PAD_TRIGGER_R,    "R" },
	{  8, PAD_BUTTON_A,     "A" },
	{  9, PAD_BUTTON_B,     "B" },
	{ 10, PAD_BUTTON_X,     "X" },
	{ 11, PAD_BUTTON_Y,     "Y" },
	{ 12, PAD_BUTTON_START, "Start" },
	{ 13, C_STICK_U,        "C-Up" },
	{ 14, C_STICK_L,        "C-Left" },
	{ 15, C_STICK_R,        "C-Right" },
	{ 16, C_STICK_D,        "C-Down" },
	{ 17, ANALOG_U,         "A-Up" },
	{ 18, ANALOG_L,         "A-Left" },
	{ 19, ANALOG_R,         "A-Right" },
	{ 20, ANALOG_D,         "A-Down" },
};

static button_t analog_sources[] = {
	{ 0, ANALOG_AS_ANALOG,  "Analog Stick" },
	{ 1, C_STICK_AS_ANALOG, "C-Stick" },
	{ 2, BUTTON_AS_ANALOG,  "D-Pad" },
};

static button_t menu_combos[] = {
	{ 0, PAD_BUTTON_X|PAD_BUTTON_Y, "X+Y" },
	{ 1, PAD_BUTTON_START|PAD_BUTTON_X, "Start+X" },
};

u32 gc_connected;

/* One reading of a pad: everything the driver uses comes from here, so a stand-in
   (padsweep below) replaces the whole reading at the one point a real pad is read. */
typedef struct { u32 btns; s8 sx, sy, cx, cy; } gc_raw_t;

/* diag.cfg padsweep=<vi>[,<hold>] (main_gc-menu2.cpp): from guest VI padsweep_vi of each
   game on, port 1 reads this generated sweep instead of the pad, one step per
   padsweep_hold VIs -- two by default (a 30 fps game polls every other VI); use 4 for a
   game that drops below 30 fps, or it misses steps. Main stick X end to end, main stick Y, C-stick X, C-stick
   Y (every raw value), the main stick's rim (one degree per step, along the octagonal
   gate a real pad has), then each button alone. Repeats, so a sweep started before a
   game finished loading still catches a whole pass. */
unsigned int padsweep_vi, padsweep_hold = 2;
extern unsigned int diag_vi_count;

static const u32 sweep_buttons[] = {
	PAD_BUTTON_A, PAD_BUTTON_B, PAD_BUTTON_X, PAD_BUTTON_Y, PAD_TRIGGER_Z, PAD_TRIGGER_L,
	PAD_TRIGGER_R, PAD_BUTTON_START, PAD_BUTTON_UP, PAD_BUTTON_DOWN, PAD_BUTTON_LEFT,
	PAD_BUTTON_RIGHT,
};
#define SWEEP_AXIS 256
#define SWEEP_RIM  360
#define SWEEP_BTN  16	// held for half of it, released for the other half

static void padsweep(gc_raw_t* r)
{
	unsigned int nb = sizeof(sweep_buttons) / sizeof(sweep_buttons[0]);
	unsigned int step = (diag_vi_count - padsweep_vi) / padsweep_hold;
	step %= 4 * SWEEP_AXIS + SWEEP_RIM + nb * SWEEP_BTN;
	memset(r, 0, sizeof(*r));
	if (step < 4 * SWEEP_AXIS) {
		s8 v = (s8)((int)(step % SWEEP_AXIS) - 128);
		switch (step / SWEEP_AXIS) {
			case 0: r->sx = v; break;
			case 1: r->sy = v; break;
			case 2: r->cx = v; break;
			case 3: r->cy = v; break;
		}
		return;
	}
	step -= 4 * SWEEP_AXIS;
	if (step < SWEEP_RIM) {
		float a = step * (3.14159265f / 180.0f);
		float ca = fabsf(cosf(a)), sa = fabsf(sinf(a));
		float rim = GC_MAIN_FULL * src_gate_radius(atan2f(fminf(ca, sa), fmaxf(ca, sa)));
		r->sx = (s8)lroundf(rim * cosf(a));
		r->sy = (s8)lroundf(rim * sinf(a));
		return;
	}
	step -= SWEEP_RIM;
	if (step % SWEEP_BTN < SWEEP_BTN / 2)
		r->btns = sweep_buttons[step / SWEEP_BTN];
}

/* chain=<vis>,input=<name> (main_gc-menu2.cpp): port 1 replays sd:/wii64/input/<name>.txt
   instead of the pad -- a recording scripts/dtm2input.py made from a Dolphin movie. Each
   line is "<guest VI> <PAD_BUTTON_* mask, hex> <sx> <sy> <cx> <cy>" and holds until the
   next line; before the first line the pad is at rest. Keyed on the game's own VIs, not
   host frames, so a replay stays in step on a Wii that runs slower than Dolphin did. */
typedef struct { unsigned int vi; gc_raw_t r; } padrec_t;
static padrec_t* replay_recs;
static unsigned int replay_n;

/* Load a replay for the next game, or clear it (path NULL). Returns the record count. */
unsigned int padreplay_load(const char* path)
{
	free(replay_recs);
	replay_recs = NULL;
	replay_n = 0;
	FILE* f = path ? fopen(path, "r") : NULL;
	if (!f) return 0;
	unsigned int cap = 0, vi, b;
	int sx, sy, cx, cy;
	char line[80];
	while (fgets(line, sizeof(line), f)) {
		if (sscanf(line, "%u %x %d %d %d %d", &vi, &b, &sx, &sy, &cx, &cy) != 6) continue;
		if (replay_n == cap) {
			padrec_t* p = realloc(replay_recs, (cap = cap ? cap * 2 : 256) * sizeof(*p));
			if (!p) break;
			replay_recs = p;
		}
		replay_recs[replay_n++] = (padrec_t){ vi, { b, sx, sy, cx, cy } };
	}
	fclose(f);
	return replay_n;
}

static void padreplay(gc_raw_t* r)
{
	static unsigned int i; // records are in VI order: walk forward, restart for a new game
	if (i >= replay_n || replay_recs[i].vi > diag_vi_count) i = 0;
	while (i + 1 < replay_n && replay_recs[i + 1].vi <= diag_vi_count) i++;
	if (replay_recs[i].vi <= diag_vi_count) *r = replay_recs[i].r;
	else memset(r, 0, sizeof(*r));
}

static int sweeping(int Control)
{
	return Control == 0 && (replay_n || (padsweep_vi && diag_vi_count >= padsweep_vi));
}

static void gc_read(int Control, gc_raw_t* r)
{
	if (sweeping(Control)) {
		if (replay_n) padreplay(r);
		else padsweep(r);
	} else {
		r->btns = PAD_ButtonsHeld(Control);
		r->sx = PAD_StickX(Control);
		r->sy = PAD_StickY(Control);
		r->cx = PAD_SubStickX(Control);
		r->cy = PAD_SubStickY(Control);
	}
	if (Control == 0)
		perfProf_padRaw(r->sx, r->sy, r->cx, r->cy, r->btns);
}

static unsigned int getButtons(const gc_raw_t* r)
{
	unsigned int b = r->btns;

	if(r->sx < -48) b |= ANALOG_L;
	if(r->sx >  48) b |= ANALOG_R;
	if(r->sy >  48) b |= ANALOG_U;
	if(r->sy < -48) b |= ANALOG_D;

	if(r->cx < -48) b |= C_STICK_L;
	if(r->cx >  48) b |= C_STICK_R;
	if(r->cy >  48) b |= C_STICK_U;
	if(r->cy < -48) b |= C_STICK_D;

	return b;
}

static int _GetKeys(int Control, BUTTONS * Keys, controller_config_t* config)
{
	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	controller_GC.available[Control] = (gc_connected & (1<<Control)) || sweeping(Control);
	if (!controller_GC.available[Control]) return 0;

	gc_raw_t r;
	gc_read(Control, &r);
	unsigned int b = getButtons(&r);
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
	if(config->analog->mask == ANALOG_AS_ANALOG)
		gc_stick(r.sx, r.sy, GC_MAIN_FULL, &x, &y);
	else if(config->analog->mask == C_STICK_AS_ANALOG)
		gc_stick(r.cx, r.cy, GC_CSTICK_FULL, &x, &y);
	else if(config->analog->mask == BUTTON_AS_ANALOG)
		button_stick(!!(b & PAD_BUTTON_RIGHT) - !!(b & PAD_BUTTON_LEFT),
		             !!(b & PAD_BUTTON_UP) - !!(b & PAD_BUTTON_DOWN), &x, &y);
	c->X_AXIS = x;
	c->Y_AXIS = config->invertedY ? -y : y;

	// Return whether the exit button(s) are pressed
	return isHeld(config->exit);
}

static void pause(int Control){
	PAD_ControlMotor(Control, PAD_MOTOR_STOP);
}

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	PAD_ControlMotor(Control, rumble ? PAD_MOTOR_RUMBLE : PAD_MOTOR_STOP);
}

static void configure(int Control, controller_config_t* config){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// Nothing to do here
}

static void refreshAvailable(void);

controller_t controller_GC =
	{ 'G',
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
	    .Z         = &buttons[6],  // Z
	    .L         = &buttons[5],  // Left Trigger
	    .R         = &buttons[7],  // Right Trigger
	    .A         = &buttons[8],  // A
	    .B         = &buttons[9],  // B
	    .START     = &buttons[12], // Start
	    .CU        = &buttons[13], // C-Stick Up
	    .CL        = &buttons[14], // C-Stick Left
	    .CR        = &buttons[15], // C-Stick Right
	    .CD        = &buttons[16], // C-Stick Down
	    .analog    = &analog_sources[0],
	    .exit      = &menu_combos[0],
	    .invertedY = 0,
	  },
	  {{0}}, {{0}}
	 };

static void refreshAvailable(void){

	if(padNeedScan){ gc_connected = PAD_ScanPads(); padNeedScan = 0; }

	int i;
	for(i=0; i<4; ++i)
		controller_GC.available[i] = (gc_connected & (1<<i));
	if(padsweep_vi || replay_n) // a sweep or replay stands in for a pad on port 1, plugged in or not
		controller_GC.available[0] = 1;
}
