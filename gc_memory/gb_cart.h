/**
 * Wii64 - gb_cart.h
 *
 * A Game Boy / Game Boy Color cartridge for the Transfer Pak: ROM, mapper (MBC),
 * battery RAM and the MBC3 clock. No Game Boy CPU: the N64 game reads the cartridge.
 * Behaviour from the Pan Docs (gbdev.io/pandocs) and SameBoy, Gambatte and mGBA.
 * See doc/transfer-pak-plan.md. No libogc: tests/gb_cart_test.c runs it on a PC.
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
**/

#ifndef GB_CART_H
#define GB_CART_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum gb_mbc { GB_MBC_NONE, GB_MBC1, GB_MBC2, GB_MBC3, GB_MBC5 };

/* gb_cart.flags */
#define GB_RAM     0x01
#define GB_BATTERY 0x02
#define GB_RTC     0x04
#define GB_RUMBLE  0x08
#define GB_MBC1M   0x10 /* MBC1 multicart: 4-bit ROM bank register */
#define GB_MBC30   0x20 /* MBC3 with 8 RAM banks and 8-bit ROM bank (Japanese Crystal) */

/* gb_cart_init() results */
enum gb_cart_error {
	GB_CART_OK = 0,
	GB_CART_TOO_SMALL,   /* smaller than 32 KB */
	GB_CART_UNKNOWN,     /* header 0x147 is no cartridge type */
	GB_CART_UNSUPPORTED, /* a real type that is not done yet (MBC6, MBC7, HuC1, ...) */
	GB_CART_NO_MEMORY,
	GB_CART_READ_ERROR   /* the bank hook could not read the header */
};

/* A 16 KB ROM bank, or NULL when it cannot be read. The cartridge reads its ROM only
   through this, so a large ROM can stay in its file (transfer_pak.c). */
typedef const uint8_t *(*gb_bank_fn)(void *ctx, uint32_t bank);

/* MBC3 clock registers: seconds, minutes, hours, day low, day high (bit 0 day 8,
   bit 6 halt, bit 7 day carry). */
enum { GB_RTC_S, GB_RTC_M, GB_RTC_H, GB_RTC_DL, GB_RTC_DH, GB_RTC_REGS };
#define GB_RTC_HALT  0x40
#define GB_RTC_CARRY 0x80

struct gb_cart {
	gb_bank_fn bank;         /* the ROM, 16 KB at a time */
	void *bank_ctx;
	uint32_t rom_size;
	uint8_t *ram;            /* gb_cart_init() allocates it */
	uint32_t ram_size;
	uint8_t type;            /* header 0x147 */
	int mbc, flags;
	char title[17];

	/* mapper registers */
	uint8_t ram_enable, mode, bank_lo, bank_hi, ram_bank, latch;
	uint16_t rom_bank;

	/* MBC3 clock: live and latched registers; rtc_time is the clock's time (seconds)
	   that the live registers show. clock() gives the time: time() by default. */
	uint8_t rtc[GB_RTC_REGS], rtc_latched[GB_RTC_REGS];
	int64_t rtc_time;
	int64_t (*clock)(void);

	int ram_dirty;           /* RAM or clock changed since the last gb_cart_save() */
	int rumble;              /* MBC5 rumble motor */
};

/* Parse the header and set up the mapper. rom stays the caller's. */
int gb_cart_init(struct gb_cart *cart, const uint8_t *rom, uint32_t rom_size);
/* The same, with the ROM read a bank at a time through bank(ctx, n). */
int gb_cart_init_banked(struct gb_cart *cart, uint32_t rom_size, gb_bank_fn bank, void *ctx);
void gb_cart_free(struct gb_cart *cart);

/* The cartridge bus, 0x0000-0xFFFF: ROM 0x0000-0x7FFF, RAM or clock 0xA000-0xBFFF.
   Nothing else answers (0xFF). */
uint8_t gb_cart_read(struct gb_cart *cart, uint16_t address);
void gb_cart_write(struct gb_cart *cart, uint16_t address, uint8_t value);

/* The .sav file of SameBoy, mGBA, BGB and VBA-M: the RAM, then for a clock cartridge
   48 bytes: 5 live and 5 latched registers (32-bit little endian each), then the
   64-bit UNIX time of the save. A 44-byte footer (32-bit time) also loads. */
size_t gb_cart_save_size(const struct gb_cart *cart);
size_t gb_cart_save(struct gb_cart *cart, uint8_t *out, size_t size);
void gb_cart_load(struct gb_cart *cart, const uint8_t *sav, size_t size);

#ifdef __cplusplus
}
#endif

#endif
