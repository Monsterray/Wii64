/* Host test of gc_memory/gb_cart.c:
   cc -std=c11 -Igc_memory tests/gb_cart_test.c gc_memory/gb_cart.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gb_cart.h"

static int64_t now;
static int64_t fake_clock(void) { return now; }

/* A ROM whose every 16 KB bank starts with its bank number (low, high). */
static uint8_t *make_rom(uint32_t size, uint8_t type, uint8_t ram_code)
{
	uint8_t *rom = calloc(1, size);
	for (uint32_t b = 0; b < size / 0x4000; b++) {
		rom[b * 0x4000 + 0x100] = b & 0xFF; /* past the header area of bank 0 */
		rom[b * 0x4000 + 0x101] = b >> 8;
	}
	memcpy(rom + 0x134, "TESTCART", 8);
	rom[0x147] = type;
	rom[0x149] = ram_code;
	return rom;
}

static unsigned bank_at(struct gb_cart *c, uint16_t base)
{
	return gb_cart_read(c, base + 0x100) | gb_cart_read(c, base + 0x101) << 8;
}

static struct gb_cart cart(uint8_t *rom, uint32_t size)
{
	struct gb_cart c;
	assert(gb_cart_init(&c, rom, size) == GB_CART_OK);
	c.clock = fake_clock;
	c.rtc_time = now;
	return c;
}

static void ram_ab(struct gb_cart *c, uint8_t a, uint8_t b)
{
	gb_cart_write(c, 0xA000, a);
	gb_cart_write(c, 0xBFFF, b);
}

int main(void)
{
	struct gb_cart c;
	uint8_t *rom, sav[0x20100];

	/* Header: errors, title, sizes */
	rom = make_rom(0x8000, 0x00, 0);
	assert(gb_cart_init(&c, rom, 0x4000) == GB_CART_TOO_SMALL);
	rom[0x147] = 0x04;
	assert(gb_cart_init(&c, rom, 0x8000) == GB_CART_UNKNOWN);
	rom[0x147] = 0xFC; /* Pocket Camera: a later phase */
	assert(gb_cart_init(&c, rom, 0x8000) == GB_CART_UNSUPPORTED);
	rom[0x147] = 0x00;
	c = cart(rom, 0x8000);
	assert(!strcmp(c.title, "TESTCART") && c.mbc == GB_MBC_NONE && c.ram_size == 0);
	assert(bank_at(&c, 0x0000) == 0 && bank_at(&c, 0x4000) == 1);
	assert(gb_cart_read(&c, 0xA000) == 0xFF && gb_cart_read(&c, 0xC000) == 0xFF);
	gb_cart_free(&c);
	free(rom);

	/* MBC1: 2 MB, 32 KB RAM */
	rom = make_rom(0x200000, 0x03, 3);
	c = cart(rom, 0x200000);
	assert(c.mbc == GB_MBC1 && c.ram_size == 0x8000 && !(c.flags & GB_MBC1M));
	gb_cart_write(&c, 0x2000, 0x00);
	assert(bank_at(&c, 0x4000) == 1);          /* bank 0 selects 1 */
	gb_cart_write(&c, 0x2000, 0x1F);
	assert(bank_at(&c, 0x4000) == 0x1F);
	gb_cart_write(&c, 0x2000, 0x20);           /* only 5 bits: 0x20 is 0, so 1 */
	assert(bank_at(&c, 0x4000) == 1);
	gb_cart_write(&c, 0x4000, 1);
	gb_cart_write(&c, 0x2000, 0x00);
	assert(bank_at(&c, 0x4000) == 0x21);       /* 0x20 cannot be selected */
	assert(bank_at(&c, 0x0000) == 0);
	gb_cart_write(&c, 0x6000, 1);              /* mode 1: 0x0000-0x3FFF banks too */
	assert(bank_at(&c, 0x0000) == 0x20);
	assert(gb_cart_read(&c, 0xA000) == 0xFF);  /* RAM off */
	ram_ab(&c, 1, 2);
	assert(!c.ram_dirty);
	gb_cart_write(&c, 0x0000, 0x1A);           /* low nibble A turns RAM on */
	ram_ab(&c, 0x11, 0x22);                    /* RAM bank 1 in mode 1 */
	assert(c.ram_dirty && c.ram[0x2000] == 0x11 && c.ram[0x3FFF] == 0x22);
	gb_cart_write(&c, 0x6000, 0);              /* mode 0: RAM bank 0 */
	assert(gb_cart_read(&c, 0xA000) == 0xFF);
	gb_cart_write(&c, 0x0000, 0x00);
	assert(gb_cart_read(&c, 0xA000) == 0xFF);
	gb_cart_free(&c);
	free(rom);

	/* MBC1 multicart: 1 MB with a second logo at bank 0x10 */
	rom = make_rom(0x100000, 0x01, 0);
	memset(rom + 0x104, 0xCE, 48);
	memset(rom + 0x40104, 0xCE, 48);
	c = cart(rom, 0x100000);
	assert(c.flags & GB_MBC1M);
	gb_cart_write(&c, 0x4000, 1);
	gb_cart_write(&c, 0x2000, 0x13);           /* 4 bits of the low register */
	assert(bank_at(&c, 0x4000) == 0x13);
	gb_cart_write(&c, 0x6000, 1);
	assert(bank_at(&c, 0x0000) == 0x10);
	gb_cart_free(&c);
	free(rom);

	/* MBC2: 256 KB, 512 x 4-bit RAM */
	rom = make_rom(0x40000, 0x06, 0);
	c = cart(rom, 0x40000);
	assert(c.mbc == GB_MBC2 && c.ram_size == 512);
	gb_cart_write(&c, 0x2100, 0x05);           /* address bit 8 set: ROM bank */
	assert(bank_at(&c, 0x4000) == 5);
	gb_cart_write(&c, 0x2100, 0x00);
	assert(bank_at(&c, 0x4000) == 1);
	gb_cart_write(&c, 0x0100, 0x0A);           /* bit 8 set: not RAM enable */
	assert(gb_cart_read(&c, 0xA000) == 0xFF);
	gb_cart_write(&c, 0x0000, 0x0A);
	gb_cart_write(&c, 0xA001, 0x5C);           /* 4 bits kept, upper bits read 1 */
	assert(gb_cart_read(&c, 0xA001) == 0xFC && gb_cart_read(&c, 0xA201) == 0xFC); /* mirror */
	gb_cart_free(&c);
	free(rom);

	/* MBC3 with clock: 2 MB, 32 KB RAM */
	now = 1000000;
	rom = make_rom(0x200000, 0x10, 3);
	c = cart(rom, 0x200000);
	assert(c.mbc == GB_MBC3 && c.flags & GB_RTC && !(c.flags & GB_MBC30));
	gb_cart_write(&c, 0x2000, 0x00);
	assert(bank_at(&c, 0x4000) == 1);
	gb_cart_write(&c, 0x2000, 0xFF);           /* 7 bits */
	assert(bank_at(&c, 0x4000) == 0x7F);
	gb_cart_write(&c, 0x0000, 0x0A);
	gb_cart_write(&c, 0x4000, 0x03);
	ram_ab(&c, 0x33, 0x44);
	assert(c.ram[0x6000] == 0x33 && c.ram[0x7FFF] == 0x44);
	/* clock: 1 day, 1 hour, 1 minute, 1 second later */
	now += 86400 + 3661;
	gb_cart_write(&c, 0x4000, 0x08);
	assert(gb_cart_read(&c, 0xA000) == 0);     /* not latched yet */
	gb_cart_write(&c, 0x6000, 0x00);
	gb_cart_write(&c, 0x6000, 0x01);
	assert(gb_cart_read(&c, 0xA000) == 1);
	gb_cart_write(&c, 0x4000, 0x09); assert(gb_cart_read(&c, 0xA000) == 1);
	gb_cart_write(&c, 0x4000, 0x0A); assert(gb_cart_read(&c, 0xA000) == 1);
	gb_cart_write(&c, 0x4000, 0x0B); assert(gb_cart_read(&c, 0xA000) == 1);
	now += 10;                                 /* the latch holds */
	gb_cart_write(&c, 0x4000, 0x08); assert(gb_cart_read(&c, 0xA000) == 1);
	gb_cart_write(&c, 0x6000, 0x01);           /* 1 to 1 does not latch */
	assert(gb_cart_read(&c, 0xA000) == 1);
	gb_cart_write(&c, 0x6000, 0x00);
	gb_cart_write(&c, 0x6000, 0x01);
	assert(gb_cart_read(&c, 0xA000) == 11);
	/* halt stops it */
	gb_cart_write(&c, 0x4000, 0x0C);
	gb_cart_write(&c, 0xA000, GB_RTC_HALT);
	now += 500;
	gb_cart_write(&c, 0x6000, 0x00); gb_cart_write(&c, 0x6000, 0x01);
	gb_cart_write(&c, 0x4000, 0x08);
	assert(gb_cart_read(&c, 0xA000) == 11);
	/* out-of-range seconds wrap at 64 with no carry; day 511 carries */
	gb_cart_write(&c, 0xA000, 62);
	gb_cart_write(&c, 0x4000, 0x09); gb_cart_write(&c, 0xA000, 59);
	gb_cart_write(&c, 0x4000, 0x0A); gb_cart_write(&c, 0xA000, 23);
	gb_cart_write(&c, 0x4000, 0x0B); gb_cart_write(&c, 0xA000, 0xFF);
	gb_cart_write(&c, 0x4000, 0x0C); gb_cart_write(&c, 0xA000, 0x01); /* day 511, running */
	now += 2;
	gb_cart_write(&c, 0x6000, 0x00); gb_cart_write(&c, 0x6000, 0x01);
	assert(c.rtc_latched[GB_RTC_S] == 0 && c.rtc_latched[GB_RTC_M] == 59);
	now += 60 * 60;                            /* 59:00 + 1 h: past 23:59:59 of day 511 */
	gb_cart_write(&c, 0x6000, 0x00); gb_cart_write(&c, 0x6000, 0x01);
	assert(c.rtc_latched[GB_RTC_DH] == GB_RTC_CARRY && c.rtc_latched[GB_RTC_DL] == 0);
	assert(c.rtc_latched[GB_RTC_H] == 0 && c.rtc_latched[GB_RTC_M] == 59);
	/* .sav with the 48-byte footer; loaded 100 s later the clock has run on */
	assert(gb_cart_save_size(&c) == 0x8000 + 48);
	assert(gb_cart_save(&c, sav, 10) == 0);
	assert(gb_cart_save(&c, sav, sizeof(sav)) == 0x8000 + 48 && !c.ram_dirty);
	struct gb_cart d = cart(rom, 0x200000);
	now += 100;
	gb_cart_load(&d, sav, 0x8000 + 48);
	assert(d.ram[0x6000] == 0x33 && d.rtc_latched[GB_RTC_DH] == GB_RTC_CARRY);
	assert(d.rtc[GB_RTC_M] == 0 && d.rtc[GB_RTC_S] == 40 && d.rtc[GB_RTC_H] == 1);
	/* 44-byte footer (32-bit time) */
	sav[0x8000 + 40 + 4] = 0;
	struct gb_cart e = cart(rom, 0x200000);
	gb_cart_load(&e, sav, 0x8000 + 44);
	assert(e.rtc[GB_RTC_S] == 40 && e.rtc[GB_RTC_H] == 1);
	/* a short .sav: what is there, the rest unchanged */
	struct gb_cart f = cart(rom, 0x200000);
	sav[0] = 0x5A;
	gb_cart_load(&f, sav, 0x10);
	assert(f.ram[0] == 0x5A && f.ram[0x6000] == 0xFF);
	gb_cart_free(&c); gb_cart_free(&d); gb_cart_free(&e); gb_cart_free(&f);
	free(rom);

	/* MBC30: 64 KB RAM, 8 RAM banks, 8-bit ROM bank */
	rom = make_rom(0x400000, 0x13, 5);
	c = cart(rom, 0x400000);
	assert(c.flags & GB_MBC30 && c.ram_size == 0x10000);
	gb_cart_write(&c, 0x2000, 0xFF);
	assert(bank_at(&c, 0x4000) == 0xFF);
	gb_cart_write(&c, 0x0000, 0x0A);
	gb_cart_write(&c, 0x4000, 0x07);
	gb_cart_write(&c, 0xA000, 0x77);
	assert(c.ram[0xE000] == 0x77);
	gb_cart_write(&c, 0x4000, 0x08);           /* no clock on this type */
	assert(gb_cart_read(&c, 0xA000) == 0xFF);
	gb_cart_free(&c);
	free(rom);

	/* MBC5 with rumble: 8 MB, 32 KB RAM */
	rom = make_rom(0x800000, 0x1E, 3);
	c = cart(rom, 0x800000);
	assert(c.mbc == GB_MBC5 && c.flags & GB_RUMBLE);
	gb_cart_write(&c, 0x2000, 0x00);
	assert(bank_at(&c, 0x4000) == 0);          /* bank 0 allowed */
	gb_cart_write(&c, 0x2000, 0x23);
	gb_cart_write(&c, 0x3000, 0x01);
	assert(bank_at(&c, 0x4000) == 0x123);
	gb_cart_write(&c, 0x0000, 0x1A);           /* MBC5 needs exactly 0x0A */
	assert(gb_cart_read(&c, 0xA000) == 0xFF);
	gb_cart_write(&c, 0x0000, 0x0A);
	gb_cart_write(&c, 0x4000, 0x0B);           /* bit 3: motor; RAM bank 3 */
	assert(c.rumble == 1);
	gb_cart_write(&c, 0xA000, 0x55);
	assert(c.ram[0x6000] == 0x55);
	gb_cart_write(&c, 0x4000, 0x03);
	assert(c.rumble == 0);
	gb_cart_free(&c);
	free(rom);

	/* A ROM smaller than its banks: mirror at the power of two, 0xFF past the end */
	rom = make_rom(0x18000, 0x19, 0);          /* 96 KB: banks 0-5, 8 banks of address */
	c = cart(rom, 0x18000);
	gb_cart_write(&c, 0x2000, 0x0A);           /* bank 10 = bank 2 */
	assert(bank_at(&c, 0x4000) == 2);
	gb_cart_write(&c, 0x2000, 0x07);           /* bank 7: past the end */
	assert(gb_cart_read(&c, 0x4100) == 0xFF);
	gb_cart_free(&c);
	free(rom);

	puts("gb cart: header, MBC1/MBC1M/MBC2/MBC3/MBC30/MBC5, RAM, clock, .sav footers: ok");
	return 0;
}
