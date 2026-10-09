/* Host test of gc_memory/transfer_pak.c:
   cc -std=c11 -Igc_memory tests/transfer_pak_test.c gc_memory/transfer_pak.c gc_memory/gb_cart.c
   Run with a scratch folder: ./transfer_pak_test /tmp/dir */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transfer_pak.h"

static uint8_t buf[32];

static void fill(uint8_t v) { memset(buf, v, 32); }
static void put(struct transfer_pak *t, uint16_t a, uint8_t v) { fill(v); tpak_write(t, a, buf); }
static uint8_t get(struct transfer_pak *t, uint16_t a) { tpak_read(t, a, buf); return buf[0]; }
static int all(uint8_t v) { for (int i = 0; i < 32; i++) if (buf[i] != v) return 0; return 1; }

/* A Game Boy address through the pak: bank register, then 0xC000 + offset */
static void gb_put(struct transfer_pak *t, uint16_t gb, uint8_t v)
{
	put(t, TPAK_BANK, gb >> 14);
	put(t, TPAK_CART + (gb & 0x3FFF), v);
}
static uint8_t gb_get(struct transfer_pak *t, uint16_t gb)
{
	put(t, TPAK_BANK, gb >> 14);
	return get(t, TPAK_CART + (gb & 0x3FFF));
}

static void write_file(const char *path, const void *data, size_t size)
{
	FILE *f = fopen(path, "wb");
	assert(f && fwrite(data, 1, size, f) == size && !fclose(f));
}

int main(int argc, char **argv)
{
	const char *dir = argc > 1 ? argv[1] : ".";
	char rom_path[400], sav_path[400], path[512];
	snprintf(rom_path, sizeof(rom_path), "%s/Pokemon.Crystal.gbc", dir);
	snprintf(sav_path, sizeof(sav_path), "%s/Pokemon.Crystal.sav", dir);

	/* MBC3 + RAM + battery + clock, 128 KB; every bank starts with its number at 0x100 */
	static uint8_t rom[0x20000];
	for (int b = 0; b < 8; b++) rom[b * 0x4000 + 0x100] = b;
	memcpy(rom + 0x134, "PM_CRYSTAL", 10);
	rom[0x147] = 0x10;
	rom[0x149] = 3;
	write_file(rom_path, rom, sizeof(rom));
	static uint8_t sav[0x8000];
	sav[0] = 0x42;
	write_file(sav_path, sav, sizeof(sav)); /* a save with no clock footer */

	static struct transfer_pak t; /* zero, as the static array in pif.c */

	/* No cartridge: off, and the status says pulled */
	assert(get(&t, TPAK_POWER) == 0x00 && all(0x00));
	assert(get(&t, TPAK_STATUS) == TPAK_PULLED);

	/* Errors */
	snprintf(path, sizeof(path), "%s/missing.gb", dir);
	assert(tpak_insert(&t, path) == -1 && !t.rom);
	rom[0x147] = 0xFC; /* Pocket Camera: later phase */
	snprintf(path, sizeof(path), "%s/camera.gb", dir);
	write_file(path, rom, 0x8000);
	assert(tpak_insert(&t, path) == GB_CART_UNSUPPORTED && !t.rom);
	rom[0x147] = 0x10;

	/* Insert: the pak starts off, the .sav (same name) is loaded */
	assert(tpak_insert(&t, rom_path) == GB_CART_OK);
	assert(!strcmp(t.sav, sav_path) && !strcmp(t.cart.title, "PM_CRYSTAL"));
	assert(t.cart.ram[0] == 0x42);

	/* Writes while off do nothing */
	put(&t, TPAK_BANK, 2);
	assert(t.bank == 0);
	assert(get(&t, TPAK_CART) == 0x00);

	/* The libdragon / osGbpak start: power on, read it back, access on, status */
	put(&t, TPAK_POWER, TPAK_POWER_ON);
	assert(get(&t, TPAK_POWER) == TPAK_POWER_ON && all(TPAK_POWER_ON));
	assert(t.bank == 3);
	assert(get(&t, TPAK_STATUS) == TPAK_POWERED);   /* access off, no reset bits */
	assert(get(&t, TPAK_CART) == 0x00);             /* no access: no cartridge data */
	put(&t, TPAK_STATUS, 1);
	assert(get(&t, TPAK_STATUS) == (TPAK_POWERED | TPAK_ACCESS | TPAK_BOOTING | TPAK_RESET));
	assert(get(&t, TPAK_STATUS) == (TPAK_POWERED | TPAK_ACCESS | TPAK_RESET));
	assert(get(&t, TPAK_STATUS) == (TPAK_POWERED | TPAK_ACCESS | TPAK_RESET));

	/* The cartridge header through bank 0, 32 bytes at a time */
	put(&t, TPAK_BANK, 0);
	tpak_read(&t, TPAK_CART + 0x120, buf);
	assert(!memcmp(buf + 0x14, "PM_CRYSTAL", 10));
	assert(get(&t, TPAK_CART + 0x140) == 0x00 && buf[7] == 0x10); /* type at 0x147 */
	/* bank values above 3 select 0 */
	put(&t, TPAK_BANK, 7);
	assert(t.bank == 0);

	/* MBC3 ROM bank 5 at Game Boy 0x4000 */
	gb_put(&t, 0x2000, 5);
	assert(gb_get(&t, 0x4100) == 5);

	/* RAM: enable, bank 0, read the save, write, then the save file */
	assert(gb_get(&t, 0xA000) == 0xFF);              /* RAM still off */
	gb_put(&t, 0x0000, 0x0A);
	gb_put(&t, 0x4000, 0x00);
	assert(gb_get(&t, 0xA000) == 0x42);
	put(&t, TPAK_BANK, 2);
	for (int i = 0; i < 32; i++) buf[i] = i;
	tpak_write(&t, TPAK_CART + 0x2020, buf);         /* Game Boy 0xA020-0xA03F */
	assert(t.cart.ram[0x20] == 0 && t.cart.ram[0x3F] == 31);

	/* Clock: 32 bytes of 0 then 32 of 1 to 0x6000 latch it; register 0x08 */
	gb_put(&t, 0x6000, 0x00);
	gb_put(&t, 0x6000, 0x01);
	gb_put(&t, 0x4000, 0x08);
	assert(gb_get(&t, 0xA000) < 60);

	/* Save: written once, then nothing until the RAM changes again */
	assert(tpak_save(&t) == 1 && tpak_save(&t) == 0);
	FILE *f = fopen(sav_path, "rb");
	assert(f);
	fseek(f, 0, SEEK_END);
	assert(ftell(f) == 0x8000 + 48);                  /* now with the clock footer */
	fclose(f);
	snprintf(path, sizeof(path), "%s.tmp", sav_path);
	assert(!fopen(path, "rb"));

	/* Access off: no cartridge data, no ACCESS bit; power off: probe reads 0 */
	put(&t, TPAK_STATUS, 0);
	assert(!(get(&t, TPAK_STATUS) & TPAK_ACCESS));
	assert(gb_get(&t, 0x0100) == 0x00);
	put(&t, TPAK_POWER, TPAK_POWER_OFF);
	assert(get(&t, TPAK_POWER) == 0x00);
	assert(!(get(&t, TPAK_STATUS) & TPAK_POWERED));

	/* A second insert reads the save written above */
	static struct transfer_pak u;
	assert(tpak_insert(&u, rom_path) == GB_CART_OK);
	assert(u.cart.ram[0] == 0x42 && u.cart.ram[0x3F] == 31);

	/* .sav name for a path with no extension */
	snprintf(path, sizeof(path), "%s/norom", dir);
	write_file(path, rom, sizeof(rom));
	assert(tpak_insert(&u, path) == GB_CART_OK);
	snprintf(path, sizeof(path), "%s/norom.sav", dir);
	assert(!strcmp(u.sav, path));

	tpak_eject(&t);
	tpak_eject(&u);
	assert(get(&t, TPAK_STATUS) == TPAK_PULLED);

	puts("transfer pak: power, status, banks, cartridge ROM/RAM/clock, .sav load and save: ok");
	return 0;
}
