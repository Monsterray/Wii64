/* Host test of main/settings.c: cc -std=c11 -Imain tests/settings_test.c main/settings.c */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "settings.h"

static char limit, audio, pak;
static int core;
static char dir[32];

static const struct setting T[] = {
	{ "General", SETTING_SECTION, 0, 0, 0, 0, 0, "The emulator." },
	{ "LimitVIs", SETTING_CHAR, SETTING_GAME, &limit, 0, 2, 1, "0 = off, 1 = VI, 2 = frame." },
	{ "Core", SETTING_INT, SETTING_GAME, &core, 0, 2, 1, "0, 1 or 2." },
	{ "Audio", SETTING_CHAR, 0, &audio, 0, 1, 1, "0 = off, 1 = on." },
	{ "Pak1", SETTING_CHAR, SETTING_GAME, &pak, 0, 1, 0, "0 = Memory Pak, 1 = Rumble Pak." },
	{ "RomDir", SETTING_TEXT, 0, dir, 0, sizeof(dir), 0, "A folder." },
};
#define N ((int)(sizeof(T) / sizeof(T[0])))

static void lines(const char* text, int game)
{
	FILE* f = tmpfile();
	fputs(text, f);
	rewind(f);
	if (game) settings_game_begin(T, N, f); else settings_read(T, N, f);
	fclose(f);
}

int main(void)
{
	settings_defaults(T, N);
	assert(limit == 1 && core == 1 && audio == 1 && pak == 0 && dir[0] == 0);

	/* settings.cfg as Wii64 1.6 wrote it, and hand-edited forms */
	lines("LimitVIs = 0\nCore = 2\n", 0);
	assert(limit == 0 && core == 2);
	lines("\n[General]\n; a comment\n# another\n  LimitVIs:2\nAudio=0\nNoSuchKey = 1\n", 0);
	assert(limit == 2 && audio == 0);
	lines("LimitVIs = 7\nCore = -1\nAudio = x\nLimitVIs\nPak1 = \n", 0); /* out of range or no value */
	assert(limit == 2 && core == 2 && audio == 0 && pak == 0);
	lines("RomDir = \"sd:/ROMS/N64\"\nRomDir = sd:/unquoted\n", 0);
	assert(!strcmp(dir, "sd:/ROMS/N64"));
	lines("RomDir = \"sd:/no end quote\n", 0);
	assert(!strcmp(dir, "sd:/no end quote"));

	/* write, then read into fresh defaults: the same values */
	FILE* f = tmpfile();
	settings_write(T, N, f);
	rewind(f);
	settings_defaults(T, N);
	settings_read(T, N, f);
	fclose(f);
	assert(limit == 2 && core == 2 && audio == 0 && !strcmp(dir, "sd:/no end quote"));

	/* a game's file: only per-game keys, and the global values come back */
	lines("LimitVIs = 0\nAudio = 1\nPak1 = 1\nPak1 = 0\nPak1 = 1\n", 1);
	assert(limit == 0 && audio == 0 && pak == 1);
	f = tmpfile(); /* Save Settings while the game runs writes the global values */
	settings_write(T, N, f);
	rewind(f);
	char text[4096] = "";
	size_t got = fread(text, 1, sizeof(text) - 1, f);
	fclose(f);
	assert(got > 0);
	assert(strstr(text, "LimitVIs = 2\n") && strstr(text, "Pak1 = 0\n") && strstr(text, "[General]\n; The emulator.\n"));
	assert(strstr(text, "; 0 = off, 1 = VI, 2 = frame. (per game)\nLimitVIs"));
	settings_game_end(T, N);
	assert(limit == 2 && pak == 0 && audio == 0);
	lines("Core = 0\n", 1); /* the next game's file starts from the global values */
	lines("LimitVIs = 1\n", 1);
	assert(core == 2 && limit == 1);
	settings_game_end(T, N);
	assert(core == 2 && limit == 2);

	puts("settings: defaults, cfg/ini lines, ranges, text, round trip, per-game apply and restore: ok");
	return 0;
}
