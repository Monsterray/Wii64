/* Invalid_Code.h - Uses 1/8th the memory as the char hash table
   by Mike Slegeir for Mupen64-GC
 */

#ifndef INVALID_CODE_H
#define INVALID_CODE_H

extern unsigned char *const invalid_code;
#define invalid_code_get(block_num) (invalid_code[(block_num)])
extern int dispatchCacheStale; // Recomp-Cache.h: a page turning invalid empties the dispatch table
#define invalid_code_set(block_num, value) \
	(dispatchCacheStale |= (value) && !invalid_code[(block_num)], invalid_code[(block_num)] = (value))

#endif
