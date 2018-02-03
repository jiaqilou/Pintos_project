#include <string.h>
#include <stdbool.h>
#include "filesys/file.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "vm/frame_table.h"
#include "vm/page_table.h"
#include "vm/swap.h"
#include <stdio.h>


static unsigned hash_func (const struct hash_elem *e, void *aux UNUSED)
{
  struct sup_page_table_entry *spte = hash_entry(e, struct sup_page_table_entry,elem);
  return hash_int((int) spte->user_vaddr);
}

static bool less_func (const struct hash_elem *a,const struct hash_elem *b,void *aux UNUSED)
{
  struct sup_page_table_entry *sptea = hash_entry(a, struct sup_page_table_entry, elem);
  struct sup_page_table_entry *spteb = hash_entry(b, struct sup_page_table_entry, elem);
  if (sptea->user_vaddr >= spteb->user_vaddr)
    {
      return false;
    }
  return true;
}

static void clear_func (struct hash_elem* e, void *aux UNUSED) {
  struct sup_page_table_entry * spte = hash_entry(e, struct sup_page_table_entry, elem);
  if (spte -> is_loaded) {
    frame_free(pagedir_get_page(thread_current()->pagedir,spte->user_vaddr));
    pagedir_clear_page(thread_current()->pagedir, spte->user_vaddr);
  }
  free (spte);
}

void spt_init (struct hash *sup_page_table)
{
  hash_init (sup_page_table, hash_func, less_func, NULL);
}

void page_table_destroy (struct hash* spt) {
  hash_destroy(spt,clear_func);
}

bool reclaim_page (struct sup_page_table_entry* spte) {
  spte -> pinned = true;
  if ( !spte -> is_loaded ) {
  //  printf ("jjjjjjj %p \n",spte->user_vaddr);
    switch (spte->type) {
      case SWAP:
        //spte -> pinned = false;
        return swap(spte);
        
        break;
      case MMAP:
        //spte -> pinned = false;
        return file(spte);
        
        break;
      case FILE:
        //spte -> pinned = false;
        return file(spte);
        
        break;
    }
  }

  return false;

}

bool file (struct sup_page_table_entry *spte)
{

  void* frame =  get_frame (PAL_USER,spte);   
  if (frame != NULL) {
    frame_add (frame, spte);
  }
  if (!frame)
    {
      printf ("why1 \n");
      return false;
    }
  if (spte->read_bytes > 0)
    {
      if ((int) spte->read_bytes != file_read_at(spte->file, frame,spte->read_bytes,spte->offset))
      {
        printf ("why2 \n");
        frame_free(frame);
        return false;
      }
      memset(frame + spte->read_bytes, 0, spte->zero_bytes);
    }

  if (install_page(spte->user_vaddr, frame, spte->writable))
    {     
      spte->is_loaded = true;  
      return true;
    }else {
      printf ("why3 \n");
      frame_free(frame);
      return false;
    }
}

bool swap (struct sup_page_table_entry *spte)
{
  void* frame = get_frame(PAL_USER,spte);
  if (!frame) {
    return false;
  }
  if (install_page(spte->user_vaddr,frame,spte->writable)) {
    read_from_block (spte->swap_index,spte->user_vaddr);
    spte->is_loaded = true;
    return true;
  }else {
    frame_free(frame);
    return false;
  }
}

bool add_mmap(struct file *file, int32_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes) {
  struct sup_page_table_entry* spte = malloc (sizeof (struct sup_page_table_entry));
  spte -> type = MMAP;
  spte -> offset = ofs;
    spte -> writable = true;
    spte -> pinned = false;
  spte -> user_vaddr = upage;

  spte-> zero_bytes = zero_bytes;
    spte -> file  = file;
      spte -> read_bytes = read_bytes;
  spte -> is_loaded = false;

  if (!p_add_mmap(spte)) {
    free (spte);
    return false;
  }
  if (hash_insert (&thread_current()->sup_page_table, &spte->elem)) {
    spte->type = HASH_ERROR;
    return false;
  }
  return true;
}

bool add_file (struct file *file, int32_t ofs, uint8_t *upage,uint32_t read_bytes, uint32_t zero_bytes,bool writable)
{
  struct sup_page_table_entry *spte = malloc(sizeof(struct sup_page_table_entry));
  if (!spte)
    {
      return false;
    }
      spte -> is_loaded = false;
  spte -> type = FILE;
    spte-> zero_bytes = zero_bytes;
  spte -> file  = file;
  spte -> offset = ofs;
    spte -> pinned = false;
  spte -> user_vaddr = upage;
  spte -> read_bytes = read_bytes;
  
  spte -> writable = writable;


  return (hash_insert(&thread_current()->sup_page_table, &spte->elem) == NULL);
}

bool grow_stack (void *fault_addr) {
 // printf ("grow stack\n");
  if ((PHYS_BASE - pg_round_down(fault_addr)) <= (8 * (1 << 20))) {
    struct sup_page_table_entry* spte = malloc(sizeof(struct sup_page_table_entry));
    if (!spte) {
     //printf ("bbbbbbbbbb\n");
      return false;
    }
    spte-> user_vaddr = pg_round_down(fault_addr);
    spte-> dirty = true;
    spte-> accessed = true;
    spte -> pinned = true;
    spte -> type = SWAP;
    spte -> is_loaded = true;
    spte -> writable = true;

//printf ("grow stack address %p %p\n",spte->user_vaddr,fault_addr);
    //printf ("aaaaa\n");
    void* kpage = get_frame (PAL_USER, spte);
    if (!kpage) {
      free (spte);
      return false;
    }
     bool success;
    
    success = install_page(spte->user_vaddr,kpage,spte->writable);
    if (!success) {
      palloc_free_page (kpage);
      free (spte);
      //printf ("cccccccccc\n");
      return false;
    }
    if (intr_context()) {
      spte -> pinned = false;    
    }
      hash_insert(&thread_current()->sup_page_table, &spte->elem);
         return true;
  }else {
    //spte -> pinned = false;
    return false;
  }
}