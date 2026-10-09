/**
 * Wii64 - gb_cart.c
 *
 * The Game Boy cartridge of the Transfer Pak; see gb_cart.h.
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
**/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gb_cart.h"

/* Header 0x147: the cartridge types that work. */
static const struct { uint8_t type, mbc, flags; } TYPES[] = {
	{ 0x00, GB_MBC_NONE, 0 },
	{ 0x08, GB_MBC_NONE, GB_RAM },
	{ 0x09, GB_MBC_NONE, GB_RAM | GB_BATTERY },
	{ 0x01, GB_MBC1, 0 },
	{ 0x02, GB_MBC1, GB_RAM },
	{ 0x03, GB_MBC1, GB_RAM | GB_BATTERY },
	{ 0x05, GB_MBC2, 0 },
	{ 0x06, GB_MBC2, GB_BATTERY },
	{ 0x0F, GB_MBC3, GB_BATTERY | GB_RTC },
	{ 0x10, GB_MBC3, GB_RAM | GB_BATTERY | GB_RTC },
	{ 0x11, GB_MBC3, 0 },
	{ 0x12, GB_MBC3, GB_RAM },
	{ 0x13, GB_MBC3, GB_RAM | GB_BATTERY },
	{ 0x19, GB_MBC5, 0 },
	{ 0x1A, GB_MBC5, GB_RAM },
	{ 0x1B, GB_MBC5, GB_RAM | GB_BATTERY },
	{ 0x1C, GB_MBC5, GB_RUMBLE },
	{ 0x1D, GB_MBC5, GB_RAM | GB_RUMBLE },
	{ 0x1E, GB_MBC5, GB_RAM | GB_BATTERY | GB_RUMBLE },
};
/* Real types for a later phase: MMM01, MBC6, MBC7, Pocket Camera, TAMA5, HuC3, HuC1. */
static const uint8_t LATER[] = { 0x0B, 0x0C, 0x0D, 0x20, 0x22, 0xFC, 0xFD, 0xFE, 0xFF };
/* Header 0x149 */
static const uint32_t RAM_SIZES[] = { 0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000 };
/* Bits that exist in the clock registers */
static const uint8_t RTC_MASK[GB_RTC_REGS] = { 0x3F, 0x3F, 0x1F, 0xFF, 0xC1 };

static int64_t clock_time(void) { return (int64_t)time(NULL); }

int gb_cart_init(struct gb_cart *c, const uint8_t *rom, uint32_t rom_size)
{
	memset(c, 0, sizeof(*c));
	if (rom_size < 0x8000) return GB_CART_TOO_SMALL;
	uint8_t type = rom[0x147];
	unsigned i;
	for (i = 0; i < sizeof(TYPES) / sizeof(TYPES[0]) && TYPES[i].type != type; i++);
	if (i == sizeof(TYPES) / sizeof(TYPES[0]))
		return memchr(LATER, type, sizeof(LATER)) ? GB_CART_UNSUPPORTED : GB_CART_UNKNOWN;

	c->rom = rom;
	c->rom_size = rom_size;
	c->type = type;
	c->mbc = TYPES[i].mbc;
	c->flags = TYPES[i].flags;
	for (i = 0; i < 16 && rom[0x134 + i] >= 0x20 && rom[0x134 + i] < 0x7F; i++)
		c->title[i] = rom[0x134 + i];

	if (c->mbc == GB_MBC2) c->ram_size = 512; /* 512 x 4 bits, one byte each */
	else if (c->flags & GB_RAM && rom[0x149] < 6) c->ram_size = RAM_SIZES[rom[0x149]];
	/* MBC30 (Japanese Pokemon Crystal): 64 KB RAM or more than 2 MB ROM */
	if (c->mbc == GB_MBC3 && (c->ram_size > 0x8000 || rom_size > 0x200000)) c->flags |= GB_MBC30;
	/* MBC1 multicart: 1 MB, and a second Nintendo logo at the start of game 2 (bank 0x10) */
	if (c->mbc == GB_MBC1 && rom_size == 0x100000 && !memcmp(rom + 0x104, rom + 0x40104, 48))
		c->flags |= GB_MBC1M;
	if (c->ram_size) {
		if (!(c->ram = malloc(c->ram_size))) return GB_CART_NO_MEMORY;
		memset(c->ram, 0xFF, c->ram_size);
	}
	c->rom_bank = c->bank_lo = 1;
	c->clock = clock_time;
	c->rtc_time = c->clock();
	return GB_CART_OK;
}

void gb_cart_free(struct gb_cart *c)
{
	free(c->ram);
	c->ram = NULL;
}

/* The ROM bank at 0x4000-0x7FFF */
static uint32_t rom_bank(const struct gb_cart *c)
{
	switch (c->mbc) {
	case GB_MBC_NONE: return 1;
	case GB_MBC1: return c->flags & GB_MBC1M ? (c->bank_hi << 4) | (c->bank_lo & 0x0F)
	                                         : (c->bank_hi << 5) | c->bank_lo;
	default: return c->rom_bank;
	}
}

/* A ROM byte. The bank wraps at the next power of two of the ROM size, as the
   unconnected address lines do; a ROM that is not a power of two reads 0xFF past its end. */
static uint8_t rom_byte(const struct gb_cart *c, uint32_t bank, uint16_t offset)
{
	uint32_t banks = 1;
	while (banks * 0x4000 < c->rom_size) banks <<= 1;
	uint32_t a = (bank & (banks - 1)) * 0x4000 + (offset & 0x3FFF);
	return a < c->rom_size ? c->rom[a] : 0xFF;
}

/* The RAM byte for bank and address, or NULL; RAM sizes are powers of two, so the
   mask also mirrors a 2 KB RAM through 0xA000-0xBFFF. */
static uint8_t *ram_byte(struct gb_cart *c, uint32_t bank, uint16_t address)
{
	if (!c->ram_size) return NULL;
	return &c->ram[(bank * 0x2000 + (address & 0x1FFF)) & (c->ram_size - 1)];
}

/* Advance the clock by secs seconds. Registers out of range (a game can write 62
   seconds) count up to their bit limit and wrap to 0 with no carry. */
static void rtc_add(struct gb_cart *c, uint64_t secs)
{
	uint8_t *r = c->rtc;
	while (secs) {
		if (r[GB_RTC_S] < 60 && r[GB_RTC_M] < 60 && r[GB_RTC_H] < 24) {
			uint64_t t = (((uint64_t)(r[GB_RTC_DL] | (r[GB_RTC_DH] & 1) << 8) * 24 + r[GB_RTC_H]) * 60
			              + r[GB_RTC_M]) * 60 + r[GB_RTC_S] + secs;
			r[GB_RTC_S] = t % 60; t /= 60;
			r[GB_RTC_M] = t % 60; t /= 60;
			r[GB_RTC_H] = t % 24; t /= 24;
			if (t > 511) r[GB_RTC_DH] |= GB_RTC_CARRY;
			t &= 511;
			r[GB_RTC_DL] = t & 0xFF;
			r[GB_RTC_DH] = (r[GB_RTC_DH] & ~1) | (uint8_t)(t >> 8);
			return;
		}
		secs--;
		if ((r[GB_RTC_S] = (r[GB_RTC_S] + 1) & 0x3F) != 60) continue;
		r[GB_RTC_S] = 0;
		if ((r[GB_RTC_M] = (r[GB_RTC_M] + 1) & 0x3F) != 60) continue;
		r[GB_RTC_M] = 0;
		if ((r[GB_RTC_H] = (r[GB_RTC_H] + 1) & 0x1F) != 24) continue;
		r[GB_RTC_H] = 0;
		if (++r[GB_RTC_DL]) continue;
		if (r[GB_RTC_DH] & 1) r[GB_RTC_DH] = (r[GB_RTC_DH] & ~1) | GB_RTC_CARRY;
		else r[GB_RTC_DH] |= 1;
	}
}

/* Bring the live registers to the clock's time; a halted clock does not count. */
static void rtc_update(struct gb_cart *c)
{
	int64_t now = c->clock();
	if (!(c->rtc[GB_RTC_DH] & GB_RTC_HALT) && now > c->rtc_time)
		rtc_add(c, (uint64_t)(now - c->rtc_time));
	c->rtc_time = now;
}

static int rtc_selected(const struct gb_cart *c)
{
	return c->mbc == GB_MBC3 && c->ram_bank >= 0x08 && c->ram_bank <= 0x0C;
}

uint8_t gb_cart_read(struct gb_cart *c, uint16_t address)
{
	if (address < 0x4000) /* MBC1 mode 1 also banks this area */
		return rom_byte(c, c->mbc == GB_MBC1 && c->mode ? c->bank_hi << (c->flags & GB_MBC1M ? 4 : 5) : 0, address);
	if (address < 0x8000)
		return rom_byte(c, rom_bank(c), address);
	if (address < 0xA000 || address >= 0xC000 || (c->mbc != GB_MBC_NONE && !c->ram_enable))
		return 0xFF;

	uint8_t *p;
	switch (c->mbc) {
	case GB_MBC1: p = ram_byte(c, c->mode ? c->bank_hi : 0, address); break;
	case GB_MBC2: return 0xF0 | c->ram[address & 0x1FF];
	case GB_MBC3:
		if (rtc_selected(c))
			return c->flags & GB_RTC ? c->rtc_latched[c->ram_bank - 8] : 0xFF;
		p = ram_byte(c, c->ram_bank, address);
		break;
	case GB_MBC5: p = ram_byte(c, c->ram_bank, address); break;
	default: p = ram_byte(c, 0, address); break;
	}
	return p ? *p : 0xFF;
}

void gb_cart_write(struct gb_cart *c, uint16_t address, uint8_t value)
{
	if (address < 0x8000) {
		switch (c->mbc) {
		case GB_MBC1:
			if (address < 0x2000) c->ram_enable = (value & 0x0F) == 0x0A;
			else if (address < 0x4000) c->bank_lo = (value & 0x1F) ? value & 0x1F : 1;
			else if (address < 0x6000) c->bank_hi = value & 3;
			else c->mode = value & 1;
			break;
		case GB_MBC2: /* address bit 8 chooses: RAM enable (clear) or ROM bank (set) */
			if (address >= 0x4000) break;
			if (address & 0x100) c->rom_bank = (value & 0x0F) ? value & 0x0F : 1;
			else c->ram_enable = (value & 0x0F) == 0x0A;
			break;
		case GB_MBC3:
			if (address < 0x2000) c->ram_enable = (value & 0x0F) == 0x0A;
			else if (address < 0x4000) {
				c->rom_bank = value & (c->flags & GB_MBC30 ? 0xFF : 0x7F);
				if (!c->rom_bank) c->rom_bank = 1;
			}
			else if (address < 0x6000) c->ram_bank = value;
			else { /* 0x00 then 0x01 latches the clock */
				if (c->flags & GB_RTC && c->latch == 0 && value == 1) {
					rtc_update(c);
					memcpy(c->rtc_latched, c->rtc, GB_RTC_REGS);
				}
				c->latch = value;
			}
			break;
		case GB_MBC5:
			if (address < 0x2000) c->ram_enable = value == 0x0A;
			else if (address < 0x3000) c->rom_bank = (c->rom_bank & 0x100) | value;
			else if (address < 0x4000) c->rom_bank = (c->rom_bank & 0xFF) | (value & 1) << 8;
			else if (address < 0x6000) {
				if (c->flags & GB_RUMBLE) { c->rumble = value >> 3 & 1; c->ram_bank = value & 7; }
				else c->ram_bank = value & 0x0F;
			}
			break;
		}
		return;
	}
	if (address < 0xA000 || address >= 0xC000 || (c->mbc != GB_MBC_NONE && !c->ram_enable))
		return;

	uint8_t *p;
	switch (c->mbc) {
	case GB_MBC1: p = ram_byte(c, c->mode ? c->bank_hi : 0, address); break;
	case GB_MBC2: c->ram[address & 0x1FF] = value & 0x0F; c->ram_dirty = 1; return;
	case GB_MBC3:
		if (rtc_selected(c)) { /* the live counter; a seconds write restarts the second */
			if (!(c->flags & GB_RTC)) return;
			int r = c->ram_bank - 8;
			rtc_update(c);
			c->rtc[r] = value & RTC_MASK[r];
			c->ram_dirty = 1;
			return;
		}
		p = ram_byte(c, c->ram_bank, address);
		break;
	case GB_MBC5: p = ram_byte(c, c->ram_bank, address); break;
	default: p = ram_byte(c, 0, address); break;
	}
	if (p) { *p = value; c->ram_dirty = 1; }
}

size_t gb_cart_save_size(const struct gb_cart *c)
{
	return c->ram_size + (c->flags & GB_RTC ? 48 : 0);
}

static void put32(uint8_t *p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static uint32_t get32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

size_t gb_cart_save(struct gb_cart *c, uint8_t *out, size_t size)
{
	size_t n = gb_cart_save_size(c);
	if (size < n) return 0;
	if (c->ram_size) memcpy(out, c->ram, c->ram_size);
	if (c->flags & GB_RTC) {
		uint8_t *f = out + c->ram_size;
		rtc_update(c);
		for (int i = 0; i < GB_RTC_REGS; i++) {
			put32(f + 4 * i, c->rtc[i]);
			put32(f + 20 + 4 * i, c->rtc_latched[i]);
		}
		put32(f + 40, (uint32_t)c->rtc_time);
		put32(f + 44, (uint32_t)((uint64_t)c->rtc_time >> 32));
	}
	c->ram_dirty = 0;
	return n;
}

void gb_cart_load(struct gb_cart *c, const uint8_t *sav, size_t size)
{
	size_t n = size < c->ram_size ? size : c->ram_size;
	if (n) memcpy(c->ram, sav, n);
	if (c->mbc == GB_MBC2)
		for (size_t i = 0; i < n; i++) c->ram[i] &= 0x0F;
	size_t footer = size - n;
	if (c->flags & GB_RTC && size >= c->ram_size && (footer == 48 || footer == 44)) {
		const uint8_t *f = sav + c->ram_size;
		for (int i = 0; i < GB_RTC_REGS; i++) {
			c->rtc[i] = get32(f + 4 * i) & RTC_MASK[i];
			c->rtc_latched[i] = get32(f + 20 + 4 * i) & RTC_MASK[i];
		}
		c->rtc_time = footer == 48 ? (int64_t)(get32(f + 40) | (uint64_t)get32(f + 44) << 32)
		                           : (int64_t)get32(f + 40);
		rtc_update(c); /* the clock ran while the cartridge was out */
	}
	c->ram_dirty = 0;
}
