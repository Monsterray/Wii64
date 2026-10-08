/**
 * Wii64 - settings.h
 *
 * settings.ini: one table row per setting (main/main_gc-menu2.cpp SETTINGS[]) gives the
 * key, the variable, the range, the default and the comment written above the key. This
 * engine reads and writes any such table, so tests/settings_test.c runs it on the host.
 *
 * A file holds "key = value" lines, [section] lines and ';' or '#' comments. Unknown keys
 * are ignored (an older Wii64 reads a newer file), a missing key keeps its value, and a
 * value out of range is ignored. settings.cfg (before 1.7.0) has the same lines.
 *
 * A game can have its own file, settings/<game code>.ini, with only the keys it changes.
 * Only rows with SETTING_GAME are read from it; settings_game_end() puts the values back.
**/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum setting_type {
	SETTING_SECTION,  /* a [name] line; note is the comment under it */
	SETTING_CHAR,     /* value points to a char */
	SETTING_INT,      /* value points to an int-sized variable */
	SETTING_TEXT      /* value points to a char[max]; written in double quotes */
};

#define SETTING_GAME 1  /* a game's own settings file can set it */

struct setting {
	const char* key;
	unsigned char type, flags;
	void* value;
	int min, max;     /* SETTING_TEXT: max is the buffer size */
	int def;          /* SETTING_TEXT: the default is "" */
	const char* note; /* the comment written above the key */
};

#define SETTINGS_MAX 64 /* rows a table can have (game snapshot size) */

void settings_defaults(const struct setting* t, int n);
/* One line of a file or a launch argument ("key=value"). Returns 1 when a setting changed. */
int settings_line(const struct setting* t, int n, char* line, int gameOnly);
void settings_read(const struct setting* t, int n, FILE* f);
void settings_write(const struct setting* t, int n, FILE* f);
/* A game's file: keep the global values of the keys it sets, then set them. */
void settings_game_begin(const struct setting* t, int n, FILE* f);
/* Put back the values a game's file changed. */
void settings_game_end(const struct setting* t, int n);

#ifdef __cplusplus
}
#endif

#endif
