#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogc/system.h>

#include "Autoboot.h"
#include "Autotest.h"

static unsigned int target_vis;
static unsigned int vis;
static char result_path[192];

static void write_state(const char* state)
{
	FILE* file = fopen("sd:/wii64/autotest/state.txt", "w");
	if (file) {
		fputs(state, file);
		fclose(file);
	}
}

extern "C" void autotest_load(void)
{
	char line[256];
	char rom_path[192] = "";
	FILE* control = fopen("sd:/wii64/autotest/current.ini", "r");

	if (!control) {
		write_state("control=missing\n");
		return;
	}
	write_state("control=found\n");
	while (fgets(line, sizeof(line), control)) {
		char* value = strchr(line, '=');
		if (!value)
			continue;
		*value++ = '\0';
		value[strcspn(value, "\r\n")] = '\0';
		if (!strcmp(line, "rom"))
			strncpy(rom_path, value, sizeof(rom_path) - 1);
		else if (!strcmp(line, "vis"))
			target_vis = strtoul(value, NULL, 10);
		else if (!strcmp(line, "result"))
			strncpy(result_path, value, sizeof(result_path) - 1);
	}
	fclose(control);
	rom_path[sizeof(rom_path) - 1] = '\0';
	result_path[sizeof(result_path) - 1] = '\0';

	if (!target_vis || !result_path[0]) {
		write_state("control=invalid\n");
		return;
	}
	Autoboot::setPath(rom_path);
	if (!Autoboot::hasPath()) {
		target_vis = 0;
		write_state("rom=invalid\n");
	}
}

extern "C" void autotest_load_result(int result)
{
	write_state(result ? "rom=load-failed\n" : "rom=loaded\n");
}

extern "C" void autotest_tick(void)
{
	if (!target_vis || ++vis != target_vis)
		return;

	write_state("status=pass\n");
	FILE* result = fopen(result_path, "w");
	if (result) {
		fprintf(result, "status=pass\nvis=%u\n", vis);
		fclose(result);
	}
	target_vis = 0;
	SYS_ResetSystem(SYS_POWEROFF, 0, FALSE);
}
