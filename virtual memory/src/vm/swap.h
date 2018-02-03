#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include <bitmap.h>

//Get the block device when we initialize our swap code
void swap_init(void);
size_t write_from_block(uint32_t* frame);
void read_from_block(size_t free_position, uint32_t* frame);


