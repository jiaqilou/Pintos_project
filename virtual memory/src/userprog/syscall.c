#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/init.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include <list.h>
#include <user/syscall.h>
#include "threads/malloc.h"
#include "vm/page_table.h"
#include "vm/frame_table.h"
#include "vm/swap.h"

static void syscall_handler (struct intr_frame *);
static void check_pointer (const void *address);
void unpin_ptr (void* vaddr);
void unpin_string (void* str);
void unpin_buffer (void* buffer, unsigned size);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  check_pointer ( f-> esp);
  //if (thread_current() -> esp != (void *)0x ) {
    thread_current()->esp = f->esp;
 // }
  
  //printf ("syscall \n");
  switch (*(int*)f->esp) {
  	case SYS_HALT: {/* Halt the operating system. */
  		shutdown_power_off();
  		break;
  	}                 
    case SYS_EXIT:{/* Terminate this process. void exit (int status)*/
  		check_pointer ((int*)f->esp+1);
  		int argument = *((int*)f->esp +1);
  		exit (argument);
    	break;
    }                  
    case SYS_EXEC: { /* Start another process. */
    	check_pointer ( (int*)f ->esp+1);
    	check_pointer( *((int*)f->esp + 1));
    	int arg = *((int*)f->esp +1);
    	const char* cmd_line = (const char *)arg;
      //printf ("exe \n");
    	f->eax =  process_execute(*((int*)f->esp + 1));
    	break;
    }                
    case SYS_WAIT: {/* Wait for a child process to die. */
    //	printf ("wait \n");
 		 check_pointer ( (int*)f ->esp+1);
    	pid_t pid = *((int*)f->esp +1);
    	f->eax = process_wait (*((int*)f->esp +1));

    	//int wait(pid_t pid);
    	break;
    }                   
    case SYS_CREATE: { /* Create a file. */  // almost finish
    	check_pointer ((int*) f ->esp+1);
    	check_pointer(*((int*)f->esp + 1));
    	const char* file_name = (const char*)(*((int*)f->esp + 1));
    	check_pointer( f ->esp+2 );
    	unsigned initial_size = *((unsigned*)f->esp + 2);   	
    	if (file_name == NULL) {
    		exit(-1);
    	}
    	f ->eax = filesys_create (file_name, initial_size);
    	break;
    }                
    case SYS_REMOVE: { /* Delete a file. */
    	check_pointer( (int*)f ->esp+1);
    	check_pointer(*((int*)f->esp + 1));
    	const char* file = (const char*)(*((int*)f->esp + 1));
    	if (filesys_remove(file) == NULL) {
    		f->eax = false;
    	}else {
    		f->eax = true;
    	}
    	break;
    }                
    case SYS_OPEN:{  /* Open a file. */    // almost finish
    	check_pointer( (int*)f ->esp+1);
    	check_pointer(*((int*)f->esp + 1));

   		const char* file_name = (const char*)(*((int*)f->esp + 1));

   		if (file_name == NULL) {
   			exit (-1);
   		}

   		struct file * new_file = filesys_open (file_name);
   		if (new_file == NULL) {
   			f ->eax = -1;
   		} else {
   			int file_descriptor = thread_current()->file_descriptor;
   			thread_current()->file_descriptor++;
   			list_push_back (&thread_current()->list_of_file, &new_file->elem);
   			new_file ->file_descriptor = file_descriptor;
   			f->eax = file_descriptor;
   		}
    	//int open(const char* file);
    	break;
    }                  
    case SYS_FILESIZE:{ /* Obtain a file's size. */

    	int fd = *((int*)f->esp + 1);
    	check_pointer( (int*)f ->esp+1);
    	struct list_elem * first = list_begin (&thread_current()->list_of_file);
	  		struct list_elem* ihatethis;
	  		for (ihatethis = first; ihatethis != list_end (&thread_current()->list_of_file); ihatethis = list_next (ihatethis)) {
	  			struct file* ffffile = list_entry(ihatethis, struct file, elem) ;
	  			if (fd == ffffile-> file_descriptor) {
	  				f->eax = file_length (ffffile);
	  				break;
	  			}
	  		}
    	break;
    }               
    case SYS_READ :{ /* Read from a file. */
    	check_pointer( (int*)f ->esp+1);

    	int fd = *((int*)f->esp + 1);
    	check_pointer( (int*)f ->esp+2);

    	check_pointer( *((int*)f->esp + 2));

		  int* buffer = *((int*)f->esp + 2);

		   check_pointer( (int*)f ->esp+3 );
      
		  off_t size = *((int*)f->esp + 3);
		  void* buffer_tmp = buffer;
		  unsigned buffer_size = size;

		  while (buffer_tmp != NULL) {

         //printf ("address %p \n", buffer_tmp);
         //printf ("buffer size %d %d \n",buffer_size,PGSIZE);
		  	if (pagedir_get_page(thread_current()->pagedir,buffer_tmp)==NULL) {

				  struct sup_page_table_entry* spte = NULL;

		    	struct sup_page_table_entry sptep;

		    	sptep.user_vaddr = pg_round_down(buffer_tmp);

		    	struct hash_elem *e = hash_find (&thread_current()->sup_page_table, &sptep.elem);
         
		    	if (e) {
		    		spte = hash_entry (e, struct sup_page_table_entry, elem);
		    		if (!spte->is_loaded)
				      reclaim_page (spte);
            spte->pinned = false;
		    	} else if (spte == NULL && buffer_tmp>= (f->esp-32)) {
		    		grow_stack(buffer_tmp);
		    	}
		  	}
		  			  	//printf ("aaaa\n");
		  	if (buffer_size == 0) {
		  		buffer_tmp = NULL;
		  	}else if (buffer_size > PGSIZE) {
		  		buffer_tmp += PGSIZE;
		  		buffer_size -= PGSIZE;
		  	}else {
		  		buffer_tmp = buffer+size-1;
		  		buffer_size = 0;
          unpin_buffer(buffer, size);
          break;
		  	}

		  }

    	if (fd == 0) {
  			int i;
  			for (i =0; i < size; i++) {
  				buffer[i] = input_getc();
  			}

  			f->eax =  size;

		  } else {  

			  struct list_elem * first = list_begin (&thread_current()->list_of_file);
	  		struct list_elem* ihatethis;
	  		bool found = false;
	  		for (ihatethis = first; ihatethis != list_end (&thread_current()->list_of_file); ihatethis = list_next (ihatethis)) {
	  			struct file* badfile = list_entry(ihatethis, struct file, elem) ;
	  			if (fd == badfile-> file_descriptor) {  
	  				off_t read_bytes = file_read (badfile, buffer, (unsigned)size);  
	  				f->eax = read_bytes;
	  				found = true;
            unpin_buffer(buffer, size);
	  				break;
	  			}
	  		}
	  		if (!found) {
				f->eax = -1;
	  		}	

		}
      unpin_buffer(buffer, size);
    	break;
    }                 
    case SYS_WRITE: {/* Write to a file. */
    //printf ("write \n");
    	check_pointer( (int*)f ->esp+1);
    	int fd = *((int*)f->esp + 1);
    	check_pointer( (int*)f ->esp+2);
    	check_pointer( *((int*)f->esp + 2));
		void* buffer = (void*)(*((int*)f->esp + 2));
		check_pointer( (int*)f ->esp+3 );
		unsigned size = *((unsigned*)f->esp + 3);
//printf ("write  111111111\n");
		f->eax = write(fd, buffer, size);
    //printf ("write 22222222\n");
    unpin_buffer(buffer, size);
   // printf ("write 33333333\n");
    	break;
    }                 
    case SYS_SEEK: {/* Change position in a file. */
   		check_pointer( (int*)f ->esp+1);
    	int fd = *((int*)f->esp + 1);
    	check_pointer((int*) f ->esp+2 );
    	unsigned position = *((unsigned*)f->esp + 2);
    	
    	seek(fd, position);
    	break;
    }                   
    case SYS_TELL: { /* Report current position in a file. */
		check_pointer( (int*)f ->esp+1);
    	int fd = *((int*)f->esp + 1);
    	
    	f-> eax = tell(fd);
    	break;
    }                  
    case SYS_CLOSE: { // almost finish
    	//printf ("close \n");
    	check_pointer( (int*)f ->esp+1);
    	int fd = *((int*)f->esp + 1);
  		struct list_elem * first = list_begin (&thread_current()->list_of_file);
  		struct list_elem* ihatethis;
  		for (ihatethis = first; ihatethis != list_end (&thread_current()->list_of_file); ihatethis = list_next (ihatethis)) {
  			struct file* fffile = list_entry(ihatethis, struct file, elem) ;
  			//first = i;
  			if (fd == fffile-> file_descriptor) {
  				list_remove (ihatethis);
  				file_close(fffile);
  				break;
  			}
  		}
    	break;
    }
    case SYS_MMAP: {
      check_pointer( (int*)f ->esp+1);
      int fd = *((int*)f->esp + 1);
      check_pointer( (int*)f ->esp+2);
      uint32_t* addr = *((int*)f->esp + 2);

      f -> eax = mmap (fd, addr);
     // printf ("returned \n");
      break;
    }
    case SYS_MUNMAP: {
      check_pointer( (int*)f ->esp+1);
      uint32_t* addr = *((int*)f->esp + 1);
      munmap (addr);
      break;
    }
  }
}

mapid_t mmap (int fd, void* addr) {
  struct list_elem * first = list_begin (&thread_current()->list_of_file);
  struct list_elem* ihatethis;

  struct file* intended_file = NULL;
  for (ihatethis = first; ihatethis != list_end (&thread_current()->list_of_file); ihatethis = list_next (ihatethis)) {
      struct file* badfile = list_entry(ihatethis, struct file, elem) ;
      if (fd == badfile-> file_descriptor) {
        intended_file = badfile;
      }
  }
  //struct sup_page_table_entry* spte = NULL;

  struct sup_page_table_entry sptep;

  sptep.user_vaddr = pg_round_down(addr);

  struct hash_elem *e = hash_find (&thread_current()->sup_page_table, &sptep.elem);
  if (e) {
    return -1;
  }
  if (intended_file==NULL || !is_user_vaddr(addr) || addr < (void*)0x08048000 ||addr ==NULL||(uint32_t)addr%PGSIZE!=0) {
    return -1;
  }

  if (fd == 0|| fd ==1) {
    return -1;
  }

  if (intended_file != NULL) {
    if (file_length(intended_file) <=0 ) {
      return -1;
    }

    
    struct file* file = file_reopen (intended_file);
    if (file != NULL) {
      thread_current()->mapid ++;
      int32_t ofs = 0;
      uint32_t read_bytes = file_length(file);
      while (read_bytes > 0) {

        uint32_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        uint32_t page_zero_bytes = PGSIZE - page_read_bytes;
        if (add_mmap(file,ofs, addr, page_read_bytes, page_zero_bytes)) {
          read_bytes -= page_read_bytes;
           ofs += page_read_bytes;
           addr += PGSIZE;
                
        }else {
          munmap (thread_current()->mapid);
          return -1;    
        }
      }
      return thread_current()->mapid;
    }
  }

  return -1;
}

void munmap (int mapping)
{
  p_remove_mmap(mapping);
}


void check_pointer (const void *address) {
   if (!is_user_vaddr(address) || address < (void *)0x08048000 )
  {
    exit(-1);
    return;
  }
}


void exit (int status) {


	printf("%s: exit(%d)\n",thread_current()->name,  status);

 	while (!list_empty (&thread_current()-> list_of_file))
     {
       struct list_elem *e = list_pop_front (&thread_current()-> list_of_file);
       struct file* f = list_entry(e, struct file, elem);
       file_close(f);
     }

	struct list_elem* e;
	for (e = list_begin (&thread_current()->parent_thread->list_of_child_process); 
		e!= list_end (&thread_current()->parent_thread->list_of_child_process); e = list_next(e)) {
		struct child_thread* c = list_entry(e, struct child_thread, elem);
		if (c->tid == thread_current()->tid) {
			c ->exited = true;
			c ->exit_code = status;
		}
	}
	
	if (thread_current()->parent_thread->waiting_child != NULL) {
		if (thread_current()->parent_thread->waiting_child->tid == thread_current()->tid) {
			sema_up (&thread_current()->parent_thread->semaphore_for_child_process);
		}
	}

	file_close(thread_current()->opened_file);
	thread_current()->opened_file = NULL;
	thread_exit ();
}

pid_t exec(const char* cmd_line) {
	return process_execute (cmd_line);
}

int write(int fd, const void* buffer, unsigned size) {
	if (fd == 1) {
		putbuf (buffer, size);
		return size;
	} else {
		struct list_elem * first = list_begin (&thread_current()->list_of_file);
	  	struct list_elem* ihatethis;
	  	bool found = false;
	  	for (ihatethis = first; ihatethis != list_end (&thread_current()->list_of_file); ihatethis = list_next (ihatethis)) {
	  	struct file* badfile = list_entry(ihatethis, struct file, elem) ;
	  	if (fd == badfile-> file_descriptor) {
	  		off_t write_bytes = file_write (badfile, buffer, size);
	  		return write_bytes;
	  		found = true;
	  		break;
	  	}
	  	}
	  	if (!found) {
			 return -1;
	  	}	
	}
	return -1;
}

void seek(int fd, unsigned position) {
	struct list_elem * first = list_begin (&thread_current()->list_of_file);
	struct list_elem* ihatethis;
	for (ihatethis = first; ihatethis != list_end (&thread_current()->list_of_file); ihatethis = list_next (ihatethis)) {
	  	struct file* badfile = list_entry(ihatethis, struct file, elem) ;
	  	if (fd == badfile-> file_descriptor) {
	  		file_seek (badfile, position);
	  		break;
	  	}
	}
}

unsigned tell(int fd) {
	struct list_elem * first = list_begin (&thread_current()->list_of_file);
	struct list_elem* ihatethis;
	for (ihatethis = first; ihatethis != list_end (&thread_current()->list_of_file); ihatethis = list_next (ihatethis)) {
	  	struct file* badfile = list_entry(ihatethis, struct file, elem) ;
	  	if (fd == badfile-> file_descriptor) {
	  		return file_tell (badfile);
	  		break;
	  	}
	}
	return -1;
}
 

void unpin_ptr (void* vaddr)
{
  struct sup_page_table_entry* spte = NULL;

    struct sup_page_table_entry sptep;

    sptep.user_vaddr = pg_round_down(vaddr);

    struct hash_elem *e = hash_find (&thread_current()->sup_page_table, &sptep.elem);
  if (e)
    {
      spte = hash_entry (e, struct sup_page_table_entry, elem);
      spte->pinned = false;
    }
}

void unpin_string (void* str)
{
  unpin_ptr(str);
  while (* (char *) str != 0)
    {
      str = (char *) str + 1;
      unpin_ptr(str);
    }
}

void unpin_buffer (void* buffer, unsigned size)
{
  unsigned i;
  char* local_buffer = (char *) buffer;
  for (i = 0; i < size; i++)
    {
      unpin_ptr(local_buffer);
      local_buffer++;
    }
}