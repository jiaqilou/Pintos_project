#include "threads/thread.h"
#include <hash.h>
//a list or hashtable of sup_page_table_entry as your supplementary page table
//remember, each thread should have its own sup_page_table, so create a new
//list or hashtable member in thread.h
#define FILE 0
#define SWAP 1
#define MMAP 2
#define HASH_ERROR 3

struct sup_page_table_entry {
	void* user_vaddr;

	bool pinned;
	int type;
	bool is_loaded;
	bool writable;

	struct file* file;
	size_t read_bytes;
	size_t zero_bytes;
	size_t offset;

	size_t swap_index;


	struct hash_elem elem;

	uint64_t access_time;
	bool dirty;
	bool accessed;
};

void spt_init (struct hash *sup_page_table);
void page_table_destroy (struct hash* spt);

bool reclaim_page (struct sup_page_table_entry* spte);

bool swap (struct sup_page_table_entry *spte);
bool file (struct sup_page_table_entry *spte);
bool load_mmap (struct sup_page_table_entry *spte);

bool add_mmap(struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes);

bool add_file (struct file *file, int32_t ofs, uint8_t *upage,uint32_t read_bytes, uint32_t zero_bytes,bool writable);

bool grow_stack (void *fault_addr);