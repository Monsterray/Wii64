/**
 * Wii64 - transfer_pak.c
 *
 * The N64 Transfer Pak; see transfer_pak.h.
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transfer_pak.h"

/* The cartridge address of a pak address in 0xC000-0xFFFF */
static uint16_t cart_address(const struct transfer_pak *t, uint16_t address)
{
	return (address & 0x3FFF) + t->bank * 0x4000;
}

static int cart_on(const struct transfer_pak *t)
{
	return t->powered && t->access && t->rom;
}

void tpak_read(struct transfer_pak *t, uint16_t address, uint8_t *data)
{
	uint8_t value = 0;
	switch (address >> 12) {
	case 0x8:
		value = t->powered ? TPAK_POWER_ON : 0x00;
		break;
	case 0xB:
		value = (cart_on(t) ? TPAK_ACCESS : 0) | (t->reset & 3) << 2 |
		        (t->rom ? 0 : TPAK_PULLED) | (t->powered ? TPAK_POWERED : 0);
		/* The reset bits step on status reads exactly as in mupen64plus, which the
		   games are tested with: 3 to 2 after an access write, else 2 to 1 to 0. */
		if (t->written && t->reset == 3) t->reset = 2;
		else if (!t->written && (t->reset == 2 || t->reset == 1)) t->reset--;
		break;
	case 0xC: case 0xD: case 0xE: case 0xF:
		if (cart_on(t)) {
			uint16_t a = cart_address(t, address);
			for (int i = 0; i < 32; i++) data[i] = gb_cart_read(&t->cart, a + i);
			return;
		}
		break;
	}
	memset(data, value, 32);
}

void tpak_write(struct transfer_pak *t, uint16_t address, const uint8_t *data)
{
	uint8_t value = data[31];
	switch (address >> 12) {
	case 0x8:
		if (value == TPAK_POWER_ON) {
			if (!t->powered) { t->bank = 3; t->access = t->written = 0; t->reset = 0; }
			t->powered = 1;
		}
		else if (value == TPAK_POWER_OFF)
			t->powered = 0;
		break;
	case 0xA:
		if (t->powered) t->bank = value > 3 ? 0 : value;
		break;
	case 0xB:
		if (t->powered) { t->access = value & 1; t->written = 1; t->reset = 3; }
		break;
	case 0xC: case 0xD: case 0xE: case 0xF:
		if (cart_on(t)) {
			uint16_t a = cart_address(t, address);
			for (int i = 0; i < 32; i++) gb_cart_write(&t->cart, a + i, data[i]);
		}
		break;
	}
}

/* Read a whole file into a new buffer; *size gets its size. */
static uint8_t *read_file(const char *path, long *size)
{
	FILE *f = fopen(path, "rb");
	if (!f) return NULL;
	uint8_t *buf = NULL;
	if (!fseek(f, 0, SEEK_END) && (*size = ftell(f)) > 0 && !fseek(f, 0, SEEK_SET) &&
	    (buf = malloc(*size)) && fread(buf, 1, *size, f) != (size_t)*size) {
		free(buf);
		buf = NULL;
	}
	fclose(f);
	return buf;
}

int tpak_insert(struct transfer_pak *t, const char *rom_path)
{
	tpak_eject(t);
	long size;
	uint8_t *rom = read_file(rom_path, &size);
	if (!rom) return -1;
	int err = gb_cart_init(&t->cart, rom, (uint32_t)size);
	if (err) { free(rom); return err; }
	t->rom = rom;

	/* the save: the ROM's path with .sav for its extension */
	snprintf(t->sav, sizeof(t->sav), "%s", rom_path);
	char *dot = strrchr(t->sav, '.'), *slash = strrchr(t->sav, '/');
	if (!dot || (slash && dot < slash)) dot = t->sav + strlen(t->sav);
	snprintf(dot, sizeof(t->sav) - (dot - t->sav), ".sav");
	long sav_size;
	uint8_t *sav = read_file(t->sav, &sav_size);
	if (sav) { gb_cart_load(&t->cart, sav, (size_t)sav_size); free(sav); }

	/* a new cartridge: the pak starts off, as when it is plugged in */
	t->powered = t->access = t->bank = t->written = 0;
	t->reset = 3;
	return GB_CART_OK;
}

int tpak_save(struct transfer_pak *t)
{
	if (!t->rom || !(t->cart.flags & GB_BATTERY) || !t->cart.ram_dirty) return 0;
	size_t size = gb_cart_save_size(&t->cart);
	uint8_t *buf = malloc(size);
	if (!buf) return -1;
	gb_cart_save(&t->cart, buf, size);

	/* write a new file, then put it in place of the old one */
	char tmp[sizeof(t->sav) + 4];
	snprintf(tmp, sizeof(tmp), "%s.tmp", t->sav);
	FILE *f = fopen(tmp, "wb");
	int ok = f && fwrite(buf, 1, size, f) == size;
	if (f && fclose(f)) ok = 0;
	free(buf);
	if (!ok) { remove(tmp); t->cart.ram_dirty = 1; return -1; }
	remove(t->sav); /* libfat does not rename over a file */
	if (rename(tmp, t->sav)) { t->cart.ram_dirty = 1; return -1; } /* the save stays in .tmp */
	return 1;
}

void tpak_eject(struct transfer_pak *t)
{
	gb_cart_free(&t->cart);
	free(t->rom);
	t->rom = NULL;
	t->sav[0] = 0;
	t->powered = t->access = t->bank = t->written = t->reset = 0;
}
