/**
 * Wii64 - settings.c
 *
 * The settings.ini engine; see settings.h. No libogc: tests/settings_test.c runs it.
**/

#include <stdlib.h>
#include <string.h>
#include "settings.h"
#include "config_parse.h"

/* While a game's file is in effect: which rows it set, and their global values. */
static int gameValue[SETTINGS_MAX];
static char* gameText[SETTINGS_MAX]; /* SETTING_TEXT rows: a copy of the global text */
static unsigned char gameSet[SETTINGS_MAX];

static int get(const struct setting* s)
{
	return s->type == SETTING_CHAR ? *(signed char*)s->value : *(int*)s->value;
}

static void put(const struct setting* s, int v)
{
	if (s->type == SETTING_CHAR) *(char*)s->value = (char)v;
	else *(int*)s->value = v;
}

void settings_defaults(const struct setting* t, int n)
{
	for (int i = 0; i < n; i++) {
		if (t[i].type == SETTING_TEXT) ((char*)t[i].value)[0] = 0;
		else if (t[i].type != SETTING_SECTION) put(&t[i], t[i].def);
	}
}

/* The row a "key = value" line sets, or -1. *value is the value's text. */
static int find(const struct setting* t, int n, char* line, int gameOnly, char** value)
{
	char* key = config_line(line);
	char* v = key ? config_split(key) : 0;
	if (!v) return -1;
	v[strcspn(v, "\r\n")] = 0;
	*value = v;
	for (int i = 0; i < n; i++)
		if (t[i].type != SETTING_SECTION && !strcmp(t[i].key, key))
			return !gameOnly || (t[i].flags & SETTING_GAME) ? i : -1;
	return -1;
}

static int apply(const struct setting* s, char* v)
{
	if (s->type == SETTING_TEXT) {
		if (*v != '"') return 0;
		snprintf((char*)s->value, s->max, "%s", config_unquote(v));
		return 1;
	}
	char* end;
	long x = strtol(v, &end, 10);
	if (end == v || x < s->min || x > s->max) return 0;
	put(s, (int)x);
	return 1;
}

int settings_line(const struct setting* t, int n, char* line, int gameOnly)
{
	char* v;
	int i = find(t, n, line, gameOnly, &v);
	return i >= 0 && apply(&t[i], v);
}

void settings_read(const struct setting* t, int n, FILE* f)
{
	char line[256];
	while (fgets(line, sizeof(line), f))
		settings_line(t, n, line, 0);
}

void settings_write(const struct setting* t, int n, FILE* f)
{
	fprintf(f, "; Wii64 settings. One \"key = value\" on each line; the comment above a key\n"
	           "; tells its values. Settings > Save Settings writes this file again.\n"
	           "; A game can have its own file, settings/<game code>.ini, with the keys marked\n"
	           "; \"per game\"; Current ROM shows the game code.\n");
	for (int i = 0; i < n; i++) {
		const struct setting* s = &t[i];
		if (s->type == SETTING_SECTION) {
			fprintf(f, "\n[%s]\n; %s\n", s->key, s->note);
			continue;
		}
		fprintf(f, "\n; %s%s\n", s->note, s->flags & SETTING_GAME ? " (per game)" : "");
		/* the global value, also while a game's file is in effect */
		if (s->type == SETTING_TEXT)
			fprintf(f, "%s = \"%s\"\n", s->key, i < SETTINGS_MAX && gameText[i] ? gameText[i] : (const char*)s->value);
		else
			fprintf(f, "%s = %d\n", s->key, i < SETTINGS_MAX && gameSet[i] ? gameValue[i] : get(s));
	}
}

void settings_game_begin(const struct setting* t, int n, FILE* f)
{
	char line[256], *v;
	settings_game_end(t, n);
	while (fgets(line, sizeof(line), f)) {
		int i = find(t, n, line, 1, &v);
		if (i < 0 || i >= SETTINGS_MAX) continue;
		int text = t[i].type == SETTING_TEXT;
		int global = text ? 0 : get(&t[i]);
		char* saved = NULL;
		if (text && !gameSet[i]) { /* keep the global text to put back */
			if (!(saved = malloc(t[i].max))) continue;
			memcpy(saved, t[i].value, t[i].max);
		}
		if (apply(&t[i], v) && !gameSet[i]) {
			gameValue[i] = global;
			gameText[i] = saved;
			saved = NULL;
			gameSet[i] = 1;
		}
		free(saved);
	}
}

int settings_game_count(void)
{
	int count = 0;
	for (int i = 0; i < SETTINGS_MAX; i++) count += gameSet[i];
	return count;
}

void settings_game_end(const struct setting* t, int n)
{
	for (int i = 0; i < n && i < SETTINGS_MAX; i++) {
		if (gameText[i]) {
			memcpy(t[i].value, gameText[i], t[i].max);
			free(gameText[i]);
			gameText[i] = NULL;
		}
		else if (gameSet[i]) put(&t[i], gameValue[i]);
		gameSet[i] = 0;
	}
}
