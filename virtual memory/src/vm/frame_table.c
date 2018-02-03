#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/frame_table.h"
#include "vm/page_table.h"
#include "vm/swap.h"


void ft_init (void)
{
  list_init(&frame_table);
}

void* get_frame (enum palloc_flags flags, struct sup_page_table_entry *spte) {
	if (flags & PAL_USER == 0) {
		return NULL;
	}
	
	void* frame = palloc_get_page(flags);
		if (frame) {
			frame_add(frame, spte);
		}else {
			bool found = false;
			struct list_elem *e = list_begin(&frame_table);
	        while (!found) {
	        	//printf("finding\n");
	        	//printf ("size %d \n", list_size(&frame_table));
	            struct frame_table_entry *fte = list_entry(e, struct frame_table_entry, elem);
	            if (!fte->aux->pinned) {
	            	//printf ("pined \n");
		            struct thread* t = fte -> owner;
		            if (pagedir_is_accessed(t->pagedir, fte->aux->user_vaddr))
		            {
		                pagedir_set_accessed(t->pagedir, fte->aux->user_vaddr, false);
		            }else {
		                if (!pagedir_is_dirty(t->pagedir, fte->aux->user_vaddr ) && fte->aux->type != SWAP) {
		                  	if (fte->aux->type == MMAP) {
		                  		file_write_at (fte->aux->file, fte->frame,fte->aux->read_bytes,fte->aux->offset);
		                  	}else {
			                    fte->aux->type = SWAP;
			                    fte -> aux -> swap_index = write_from_block (fte -> frame);
		                  	}
		                }
		            }
		            fte -> aux->is_loaded = false;
		            list_remove(&fte->elem);
		            pagedir_clear_page(t->pagedir, fte->aux->user_vaddr);
		            palloc_free_page(fte->frame);
		            free(fte);
		            found = true;
	            }
		 		e = list_next(e);
		        if (e == list_end(&frame_table)) {
		            e = list_begin(&frame_table);
		        }
	        }
       
	         frame = palloc_get_page (PAL_USER | PAL_ZERO);
	         frame_add(frame, spte);
     	 }
    	return frame;
}
		


void frame_add (void *frame, struct sup_page_table_entry *spte) {
	struct frame_table_entry *fte = malloc(sizeof(struct frame_table_entry));
	fte -> frame = frame;
	fte -> owner = thread_current();
	fte -> aux = spte;
	list_push_back(&frame_table, &fte->elem);
}

void frame_free (void *frame)
{
  //struct list_elem *ele;
  
  for (struct list_elem *ele = list_begin(&frame_table); ele!= list_end(&frame_table); ele = list_next(ele))
    {
      struct frame_table_entry *fte = list_entry(ele, struct frame_table_entry, elem);
      if (fte->frame == frame)
		{
		  list_remove(ele);
		  free(fte);
		  palloc_free_page(frame);
		  break;
		}
    }

}