#include <list.h>

struct list frame_table;


struct frame_table_entry {
	uint32_t* frame;
	struct thread* owner;
	struct sup_page_table_entry* aux;
	struct list_elem elem;
// Maybe store information for memory mapped files here too?
};

void ft_init (void);

void* get_frame (enum palloc_flags flags, struct sup_page_table_entry *spte);

void frame_add (void* frame, struct sup_page_table_entry *spte);

void frame_free (void *frame);