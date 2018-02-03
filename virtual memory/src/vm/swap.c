#include "vm/swap.h"

static struct block* global_swap_block;

static struct bitmap *swap_map;

void swap_init(void)
{
	global_swap_block = block_get_role(BLOCK_SWAP);
	swap_map = bitmap_create (block_size (global_swap_block) / 8);

	bitmap_set_all(swap_map,0);

}

size_t write_from_block(uint32_t* frame) {
	size_t free_position = bitmap_scan_and_flip (swap_map, 0, 1, 0);
	if (free_position != BITMAP_ERROR) {
		for (int i =0; i <8; ++i) {
			block_write (global_swap_block, free_position*8+i, frame+ i*8);
		}
	}
	return free_position;
}

void read_from_block(size_t free_position, uint32_t* frame) {
	bitmap_flip (swap_map, free_position);
	if (free_position != BITMAP_ERROR) {
		for (int i =0; i <8; ++i) {
			block_read (global_swap_block, free_position*8+i, frame+ i*8);
		}
	}
}
