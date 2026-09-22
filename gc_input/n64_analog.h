/**
 * Wii64 - n64_analog.h
 *
 * The whole stick path from a Wii/GameCube controller to what an N64 game reads, as pure
 * functions. Plain C with no libogc, so tests/n64_analog_test.c runs this exact code on the
 * host over every raw input each driver can produce.
 *
 * A driver turns its stick into per-axis fractions of full cardinal travel (-1..+1, 0 at
 * rest, +y up) and calls n64_stick(). That maps the source stick's own gate onto the N64
 * stick's gate: the rim of the source gate reaches the rim of the N64 gate in every
 * direction, and a position inside it moves linearly along its direction.
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

#ifndef N64_ANALOG_H
#define N64_ANALOG_H

#include <math.h>

/* The N64 stick's gate: an octagon, 82 at the cardinals and 98.4 at the diagonal corners
 * ((69.6, 69.6) per axis), straight edges between. From N64ModernRuntime's
 * convert_to_n64_range() (r0 = 82, alpha = 1.39414574), the measured OEM shape the N64
 * recompilation projects use. */
#define N64_GATE_R0    82.0f
#define N64_GATE_ALPHA 1.39414574f

/* Full cardinal deflection of the GameCube pad's sticks, in PAD_StickX() units. Dolphin's
 * model of a real controller (GCPadEmu.h MAIN_STICK_GATE_RADIUS, C_STICK_GATE_RADIUS
 * times the 127 radius of the reported range).
 * ponytail: fixed per stick type; a worn pad that falls short tops out slightly low. */
#define GC_MAIN_FULL   (0.7937125f * 127.0f)
#define GC_CSTICK_FULL (0.7221375f * 127.0f)

/* Full cardinal deflection of the Wii U GamePad's sticks, in WiiDRC_lStickX() units. */
#define DRC_STICK_FULL 75.0f
#define DRC_DEADZONE   0.078f

/* One axis as a fraction of its travel from calibration: each side of the centre is
 * scaled on its own, the convention of wiiuse's calc_joystick_state(). */
static inline float stick_frac(int pos, int min, int center, int max)
{
	if (pos >= center)
		return max > center ? (float)(pos - center) / (float)(max - center) : 0.0f;
	return center > min ? (float)(pos - center) / (float)(center - min) : 0.0f;
}

/* Radial dead zone, rescaled so the rest of the travel still reaches full: a direction is
 * never bent towards an axis the way a per-axis dead zone bends it. */
static inline void stick_deadzone(float *fx, float *fy, float dz)
{
	float m = hypotf(*fx, *fy);
	if (m <= dz) { *fx = *fy = 0.0f; return; }
	float s = (m - dz) / (1.0f - dz) / m;
	*fx *= s;
	*fy *= s;
}

/* Distance from the centre to each gate's rim, a being the angle from the nearest axis
 * (0..pi/4). Every Wii and GameCube stick has a regular octagonal gate with its corners at
 * full cardinal travel (Dolphin models all of them that way: OctagonAnalogStick). */
static inline float src_gate_radius(float a)
{
	return 0.92387953f / cosf(a - 0.39269908f);
}

static inline float n64_gate_radius(float a)
{
	return N64_GATE_R0 * sinf(N64_GATE_ALPHA) / sinf(3.14159265f - a - N64_GATE_ALPHA);
}

static inline signed char n64_round(float v)
{
	long r = lroundf(v);
	return (signed char)(r > 127 ? 127 : (r < -128 ? -128 : r));
}

static inline void n64_stick(float fx, float fy, signed char *x, signed char *y)
{
	float ax = fabsf(fx), ay = fabsf(fy);
	float m = hypotf(fx, fy);
	if (m == 0.0f) { *x = *y = 0; return; }
	float a = atan2f(ax < ay ? ax : ay, ax < ay ? ay : ax);
	float t = m / src_gate_radius(a);
	if (t > 1.0f) t = 1.0f;
	float r = t * n64_gate_radius(a) / m;
	*x = n64_round(fx * r);
	*y = n64_round(fy * r);
}

/* Each driver's whole conversion, from its raw reading. The drivers call these and nothing
 * else, so the host test covers exactly what runs on the Wii. */

/* GameCube pad: PAD_StickX()/PAD_StickY(), origin already subtracted, +y up. */
static inline void gc_stick(int rx, int ry, float full, signed char *x, signed char *y)
{
	n64_stick((float)rx / full, (float)ry / full, x, y);
}

/* Classic Controller and Nunchuk: raw position and the calibration wiiuse keeps with it
 * (joystick_t). No extra gain and no dead zone: calibrated full travel is the gate, the
 * way Dolphin builds the calibration block from its model of the real controller. */
static inline void cal_stick(int px, int py, int minx, int miny, int cx, int cy,
                             int maxx, int maxy, signed char *x, signed char *y)
{
	n64_stick(stick_frac(px, minx, cx, maxx), stick_frac(py, miny, cy, maxy), x, y);
}

/* Wii U GamePad: WiiDRC_lStickX()/Y(). Its sticks do not rest exactly on centre, so it
 * alone keeps a (radial) dead zone. */
static inline void drc_stick(int rx, int ry, signed char *x, signed char *y)
{
	float fx = (float)rx / DRC_STICK_FULL, fy = (float)ry / DRC_STICK_FULL;
	stick_deadzone(&fx, &fy, DRC_DEADZONE);
	n64_stick(fx, fy, x, y);
}

/* A d-pad or button standing in for the stick: -1, 0 or +1 per axis. */
static inline void button_stick(int dx, int dy, signed char *x, signed char *y)
{
	n64_stick((float)dx, (float)dy, x, y);
}

#endif
