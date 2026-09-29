/* Host snapshots of the real envelope mixers, including saved state and aliasing. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../rsp_hle/alist.h"
#include "../rsp_hle/hle_internal.h"

static uint32_t random_state = 0x64a11c7u;

static uint32_t next_random(void)
{
	random_state ^= random_state << 13;
	random_state ^= random_state >> 17;
	random_state ^= random_state << 5;
	return random_state;
}

static uint64_t hash_bytes(uint64_t hash, const void *data, size_t size)
{
	const uint8_t *bytes = data;
	for (size_t i = 0; i < size; i++) {
		hash ^= bytes[i];
		hash *= UINT64_C(1099511628211);
	}
	return hash;
}

int main(void)
{
	struct hle_t hle = {0};
	uint8_t dram[0x1000];
	const uint64_t expected[] = { UINT64_C(0xee2540c013a3b9de),
		UINT64_C(0x34747429f5ccd2c0),
		UINT64_C(0x930a4ac2a994bf23) };
	int failed = 0;
	hle.dram = dram;

	for (unsigned int variant = 0; variant < 3; variant++) {
	uint64_t hash = UINT64_C(14695981039346656037);
	random_state = 0x64a11c7u;
	for (unsigned int trial = 0; trial < 256; trial++) {
		int16_t vol[2], target[2];
		int32_t rate[2];
		uint16_t left = 0, right = trial % 7 == 0 ? 0 : 0x200;
		uint16_t input = trial % 5 == 0 ? left : 0x800;
		uint16_t count = (trial % 11) * 16 + trial % 3;
		int16_t dry = (int16_t)next_random(), wet = (int16_t)next_random();
		for (size_t i = 0; i < sizeof(hle.alist_buffer); i++)
			hle.alist_buffer[i] = next_random();
		for (size_t i = 0; i < sizeof(dram); i++)
			dram[i] = next_random();
		for (unsigned int i = 0; i < 2; i++) {
			vol[i] = (int16_t)next_random();
			target[i] = (int16_t)next_random();
			if (trial % 4 <= 1) target[i] = vol[i];
			rate[i] = variant ? (int32_t)(next_random() & 0x1ffff) :
				0x8000 + (next_random() & 0x7fff);
			if (variant == 2 && trial % 8 == 2) rate[i] = 0;
		}
		for (unsigned int pass = 0; pass < 2; pass++) {
			if (variant == 2)
				alist_envmix_ge(&hle, pass == 0, trial & 1, left, right,
					0x400, 0x600, input, count, dry, wet,
					vol, target, rate, 0x100);
			else if (variant == 1)
				alist_envmix_lin(&hle, pass == 0, left, right,
					0x400, 0x600, input, count, dry, wet,
					vol, target, rate, 0x100);
			else
				alist_envmix_exp(&hle, pass == 0, trial & 1, left, right,
					0x400, 0x600, input, count, dry, wet,
					vol, target, rate, 0x100);
			hash = hash_bytes(hash, hle.alist_buffer, sizeof(hle.alist_buffer));
			hash = hash_bytes(hash, dram + 0x100, 80);
		}
	}
	printf("alist_envmix_%s snapshot: %016llx\n",
		variant == 0 ? "exp" : variant == 1 ? "lin" : "ge",
		(unsigned long long)hash);
	if (hash != expected[variant]) failed = 1;
	}
	return failed;
}
