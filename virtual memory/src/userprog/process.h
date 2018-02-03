#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

struct mmap_file {
  struct sup_page_table_entry *spte;
  int mapid;
  struct list_elem elem;
};

bool install_page (void *upage, void *kpage, bool writable);

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

bool p_add_mmap (struct sup_page_table_entry *spte);
void p_remove_mmap (int mapping);


#endif /* userprog/process.h */
