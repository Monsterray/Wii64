/**
 * Wii64 - transfer_pak.h
 *
 * The N64 Transfer Pak: the controller pak commands (32 bytes at an address in
 * 32-byte steps) that reach a Game Boy cartridge (gb_cart.h). Addresses and values
 * from libdragon (joybus_accessory_internal.h) and mupen64plus (transferpak.c).
 * See doc/transfer-pak-plan.md. No libogc: tests/transfer_pak_test.c runs it.
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
**/

#ifndef TRANSFER_PAK_H
#define TRANSFER_PAK_H

#include "gb_cart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Pak addresses */
#define TPAK_POWER   0x8000 /* write 0x84 on, 0xFE off; reads 0x84 when on */
#define TPAK_BANK    0xA000 /* 0-3: the 16 KB of the cartridge at 0xC000-0xFFFF */
#define TPAK_STATUS  0xB000 /* read: status bits below; write 1/0: access on/off */
#define TPAK_CART    0xC000
#define TPAK_POWER_ON  0x84
#define TPAK_POWER_OFF 0xFE

/* Status bits */
#define TPAK_ACCESS  0x01   /* cartridge reads and writes work */
#define TPAK_BOOTING 0x04
#define TPAK_RESET   0x08
#define TPAK_PULLED  0x40   /* no cartridge */
#define TPAK_POWERED 0x80

struct transfer_pak {
	struct gb_cart cart;
	uint8_t *rom;           /* NULL: no cartridge */
	char sav[256];          /* the cartridge's save file */
	uint8_t powered, access, bank;
	uint8_t reset;          /* status bits 2-3 */
	uint8_t written;        /* 0xB000 written since power-on */
};

/* The 32 bytes at address (a multiple of 32) */
void tpak_read(struct transfer_pak *tpak, uint16_t address, uint8_t *data);
void tpak_write(struct transfer_pak *tpak, uint16_t address, const uint8_t *data);

/* Put the cartridge in rom_path in the pak, with its save: the same path with .sav
   in place of the extension. Returns GB_CART_OK, a gb_cart_error, or -1 when the
   ROM file cannot be read. */
int tpak_insert(struct transfer_pak *tpak, const char *rom_path);
/* Write the save when the cartridge has a battery and the RAM or clock changed.
   Returns 1 written, 0 nothing to write, -1 error. */
int tpak_save(struct transfer_pak *tpak);
void tpak_eject(struct transfer_pak *tpak);

#ifdef __cplusplus
}
#endif

#endif
