/* n64_analog_test.c - exhaustive check of Wii64's analog-stick maths.
 *
 *     clang -O2 -o n64_analog_test tests/n64_analog_test.c && ./n64_analog_test
 *
 * gc_input/n64_analog.h is plain C with no libogc in it, and the drivers call its
 * gc_stick/cal_stick/drc_stick/button_stick and nothing else, so this is the code the
 * Wii runs. It covers the maths; diag.cfg's padsweep= + scripts/padtest.py cover the live
 * path from the driver to the bytes the game reads, which this cannot see.
 *
 * For every raw position each driver can produce, it asserts:
 *   - rest reads exactly (0, 0);
 *   - full cardinal travel reads exactly 82 (the N64 gate) and a diagonal corner (70, 70);
 *   - along each axis the curve never goes backwards and no run of raw steps does nothing
 *     longer than the source's resolution forces;
 *   - every output lies inside the N64 gate and keeps the sign of its input;
 *   - a d-pad standing in for the stick reaches the gate's cardinals and corners.
 */
#include <math.h>
#include <stdio.h>

#include "../gc_input/n64_analog.h"

typedef void (*stick_fn)(int rx, int ry, signed char *x, signed char *y);

/* A driver's raw domain: rest, and the raw value at full travel on each side of each axis
 * (the calibrated extremes), plus the widest raw value the hardware can report at all. */
typedef struct {
	const char *name;
	stick_fn f;
	int rest, lo_full, hi_full, lo_raw, hi_raw;
	float dead;
} driver_t;

static int failures;

#define FAIL(...) do { printf("  FAIL "); printf(__VA_ARGS__); printf("\n"); failures++; } while (0)

static void gc_main(int rx, int ry, signed char *x, signed char *y)   { gc_stick(rx, ry, GC_MAIN_FULL, x, y); }
static void gc_c(int rx, int ry, signed char *x, signed char *y)      { gc_stick(rx, ry, GC_CSTICK_FULL, x, y); }
/* Calibration blocks as Dolphin builds them for its model of the real controllers:
 * centre 0x80, gate radius 0x61 (Classic) / 0x60 (Nunchuk), at 8 bits, shifted down to
 * the stick's own precision -- the same >>2 / >>3 wiiuse's classic.c applies. */
static void cc_left(int px, int py, signed char *x, signed char *y)   { cal_stick(px, py, 0x1f >> 2, 0x1f >> 2, 32, 32, 0xe1 >> 2, 0xe1 >> 2, x, y); }
static void cc_right(int px, int py, signed char *x, signed char *y)  { cal_stick(px, py, 0x1f >> 3, 0x1f >> 3, 16, 16, 0xe1 >> 3, 0xe1 >> 3, x, y); }
static void nunchuk(int px, int py, signed char *x, signed char *y)   { cal_stick(px, py, 0x20, 0x20, 0x80, 0x80, 0xe0, 0xe0, x, y); }
static void drc(int rx, int ry, signed char *x, signed char *y)       { drc_stick(rx, ry, x, y); }

static void axis_curve(const driver_t *d)
{
	signed char x, y, prev;
	int i, plateau = 0, worst = 0, worst_at = 0;
	int steps = d->hi_full - d->rest;
	/* A source with fewer raw steps than the N64 has values must skip some outputs, one
	 * with more must repeat some; only a run longer than that is a defect. A dead zone
	 * spans both sides of rest. */
	int allowed = (int)ceilf((float)steps / N64_GATE_R0) + 1 + (int)ceilf(2.0f * steps * d->dead);

	d->f(d->rest, d->rest, &x, &y);
	if (x || y) FAIL("%s: rest reads (%d, %d)", d->name, x, y);
	d->f(d->hi_full, d->rest, &x, &y);
	if (x != 82 || y) FAIL("%s: full right reads (%d, %d), not (82, 0)", d->name, x, y);
	d->f(d->lo_full, d->rest, &x, &y);
	if (x != -82 || y) FAIL("%s: full left reads (%d, %d), not (-82, 0)", d->name, x, y);
	d->f(d->rest, d->hi_full, &x, &y);
	if (x || y != 82) FAIL("%s: full up reads (%d, %d), not (0, 82)", d->name, x, y);
	d->f(d->rest, d->lo_full, &x, &y);
	if (x || y != -82) FAIL("%s: full down reads (%d, %d), not (0, -82)", d->name, x, y);

	d->f(d->lo_raw, d->rest, &prev, &y);
	for (i = d->lo_raw + 1; i <= d->hi_raw; i++) {
		d->f(i, d->rest, &x, &y);
		if (x < prev) { FAIL("%s: curve goes backwards at raw %d (%d then %d)", d->name, i, prev, x); break; }
		if (i > d->lo_full && i <= d->hi_full) {
			plateau = (x == prev) ? plateau + 1 : 0;
			if (plateau > worst) { worst = plateau; worst_at = i; }
		}
		prev = x;
	}
	if (worst >= allowed)
		FAIL("%s: %d raw steps around %d change nothing (allowed %d)", d->name, worst + 1, worst_at, allowed);
	printf("  %-16s raw %4d..%-4d full %4d..%-4d  longest flat %d/%d\n",
		d->name, d->lo_raw, d->hi_raw, d->lo_full, d->hi_full, worst + 1, allowed);
}

static void whole_square(const driver_t *d)
{
	signed char x, y, px;
	int rx, ry, i, bad = 0;

	/* The source gate's corners: full travel at 45 degrees, at the nearest raw step. A
	 * coarse stick (the Classic's 5-bit right stick) cannot land on 45 degrees exactly, so
	 * the expected value is the gate's corner scaled by how far that step really is. */
	for (i = 0; i < 2; i++) {
		int s = i ? -1 : 1, steps = i ? d->rest - d->lo_full : d->hi_full - d->rest;
		int raw = (int)lroundf(steps * 0.70710678f);
		float want = 70.0f * fminf(1.0f, (float)raw / steps / 0.70710678f);
		d->f(d->rest + s * raw, d->rest + s * raw, &x, &y);
		if (fabsf(s * x - want) > 1.5f || fabsf(s * y - want) > 1.5f)
			FAIL("%s: corner at raw %+d reads (%d, %d), want about %.1f", d->name, s * raw, x, y, s * want);
	}

	for (ry = d->lo_raw; ry <= d->hi_raw && bad < 3; ry++) {
		d->f(d->lo_raw, ry, &px, &y);
		for (rx = d->lo_raw; rx <= d->hi_raw && bad < 3; rx++) {
			d->f(rx, ry, &x, &y);
			float m = hypotf(x, y);
			float a = atan2f(fminf(fabsf((float)x), fabsf((float)y)), fmaxf(fabsf((float)x), fabsf((float)y)));
			if (m > n64_gate_radius(a) + 0.75f) { FAIL("%s: (%d, %d) -> (%d, %d) is outside the N64 gate", d->name, rx, ry, x, y); bad++; }
			if ((rx > d->rest && x < 0) || (rx < d->rest && x > 0) || (ry > d->rest && y < 0) || (ry < d->rest && y > 0)) {
				FAIL("%s: (%d, %d) -> (%d, %d) changes sign", d->name, rx, ry, x, y); bad++;
			}
			if (x < px) { FAIL("%s: row %d goes backwards at raw x %d", d->name, ry, rx); bad++; }
			px = x;
		}
	}
}

int main(void)
{
	static const driver_t drivers[] = {
		{ "GC main stick", gc_main,  0, -101, 101, -128, 127, 0.0f },
		{ "GC C-stick",    gc_c,     0,  -92,  92, -128, 127, 0.0f },
		{ "Classic left",  cc_left,  32,   7,  56,    0,  63, 0.0f },
		{ "Classic right", cc_right, 16,   3,  28,    0,  31, 0.0f },
		{ "Nunchuk",       nunchuk, 128,  32, 224,    0, 255, 0.0f },
		{ "Wii U GamePad", drc,      0,  -75,  75, -100, 100, DRC_DEADZONE },
	};
	signed char x, y;
	unsigned i;

	printf("axis curves\n");
	for (i = 0; i < sizeof drivers / sizeof drivers[0]; i++)
		axis_curve(&drivers[i]);

	printf("whole raw square: inside the gate, signs kept, rows monotonic, corners at (70, 70)\n");
	for (i = 0; i < sizeof drivers / sizeof drivers[0]; i++)
		whole_square(&drivers[i]);

	printf("d-pad as stick\n");
	button_stick(1, 0, &x, &y);   if (x != 82 || y) FAIL("right reads (%d, %d)", x, y);
	button_stick(0, -1, &x, &y);  if (x || y != -82) FAIL("down reads (%d, %d)", x, y);
	button_stick(1, 1, &x, &y);   if (x != 70 || y != 70) FAIL("up-right reads (%d, %d)", x, y);
	button_stick(-1, -1, &x, &y); if (x != -70 || y != -70) FAIL("down-left reads (%d, %d)", x, y);
	button_stick(0, 0, &x, &y);   if (x || y) FAIL("none reads (%d, %d)", x, y);

	printf(failures ? "\n%d failure(s)\n" : "\nall checks passed\n", failures);
	return failures ? 1 : 0;
}
