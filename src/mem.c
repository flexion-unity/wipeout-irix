#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "utils.h"


#ifdef __sgi
/* MIPS IRIX requires 8-byte aligned memory access to avoid SIGBUS */
static uint8_t hunk[MEM_HUNK_BYTES] __attribute__((aligned(8)));
#else
static uint8_t hunk[MEM_HUNK_BYTES];
#endif
static uint32_t bump_len = 0;
static uint32_t temp_len = 0;

static uint32_t temp_objects[MEM_TEMP_OBJECTS_MAX] = {};
static uint32_t temp_objects_len;


// Bump allocator - returns bytes from the front of the hunk

// These allocations persist for many frames. The allocator level is reset
// whenever we load a new race track or menu in game_set_scene()

void *mem_mark(void) {
#ifdef __sgi
	/* Align bump position to 4 bytes before marking.
	 * 4-byte alignment is sufficient for float/int32_t on MIPS IRIX.
	 * Using 8 bytes would break primitive struct iteration in object.c/ship.c:
	 * sizeof(FT3)=28, sizeof(GT3)=36, sizeof(G4)=28, sizeof(GT4)=44 are all
	 * multiples of 4 but not 8, so 8-byte padding inserts gaps between
	 * consecutive allocations that confuse pointer arithmetic (prm.ft3 += 1). */
	bump_len = (bump_len + 3) & ~3u;
#endif
	return &hunk[bump_len];
}

void *mem_bump(uint32_t size) {
#ifdef __sgi
	/* Align start of each allocation to 4 bytes for MIPS IRIX.
	 * Sufficient for float, int32_t, and 32-bit pointers. */
	bump_len = (bump_len + 3) & ~3u;
#endif
	error_if(bump_len + temp_len + size >= MEM_HUNK_BYTES, "Failed to allocate %d bytes in hunk mem", size);
	uint8_t *p = &hunk[bump_len];
	bump_len += size;
	memset(p, 0, size);
	return p;
}

void mem_reset(void *p) {
	uint32_t offset = (uint8_t *)p - (uint8_t *)hunk;
	error_if(offset > bump_len || offset > MEM_HUNK_BYTES, "Invalid mem reset");
	bump_len = offset;
}



// Temp allocator - returns bytes from the back of the hunk

// Temporary allocated bytes are not allowed to persist for multiple frames. You
// need to explicitly free them when you are done. Temp allocated bytes don't 
// have be freed in reverse allocation order. I.e. you can allocate A then B, 
// and aftewards free A then B.

void *mem_temp_alloc(uint32_t size) {
	size = ((size + 7) >> 3) << 3; // allign to 8 bytes

	error_if(bump_len + temp_len + size >= MEM_HUNK_BYTES, "Failed to allocate %d bytes in temp mem", size);
	error_if(temp_objects_len >= MEM_TEMP_OBJECTS_MAX, "MEM_TEMP_OBJECTS_MAX reached");

	temp_len += size;
	void *p = &hunk[MEM_HUNK_BYTES - temp_len];
	temp_objects[temp_objects_len++] = temp_len;
	return p;
}

void mem_temp_free(void *p) {
	uint32_t offset = (uint8_t *)&hunk[MEM_HUNK_BYTES] - (uint8_t *)p;
	error_if(offset > MEM_HUNK_BYTES, "Object 0x%p not in temp hunk", p);

	bool found = false;
	uint32_t remaining_max = 0;
	for (int i = 0; i < temp_objects_len; i++) {
		if (temp_objects[i] == offset) {
			temp_objects[i--] = temp_objects[--temp_objects_len];
			found = true;
		}
		else if (temp_objects[i] > remaining_max) {
			remaining_max = temp_objects[i];
		}
	}
	error_if(!found, "Object 0x%p not in temp hunk", p);
	temp_len = remaining_max;
}

void mem_temp_check(void) {
	error_if(temp_len != 0, "Temp memory not free: %d object(s)", temp_objects_len);
}
