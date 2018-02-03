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
#include "userprog/process.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include <list.h>
#include <user/syscall.h>
#include "threads/malloc.h"



static void syscall_handler (struct intr_frame *);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  check_pointer ( f-> esp);
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
    	f->eax =  process_execute(*((int*)f->esp + 1));
    	break;
    }                
    case SYS_WAIT: {/* Wait for a child process to die. */
    	//printf ("wait \n");
 		check_pointer ( (int*)f ->esp+1);
    	pid_t pid = *((int*)f->esp +1);
    	f->eax = process_wait (*((int*)f->esp +1));

    	//int wait(pid_t pid);
    	break;
    }                   
    case SYS_CREATE: { /* Create a file. */  // almost finish
   // printf ("create \n");
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
   // printf ("remove \n");
    	check_pointer( (int*)f ->esp+1);
    	check_pointer(*((int*)f->esp + 1));
    	const char* file = (const char*)(*((int*)f->esp + 1));
    	//bool remove(const char* file);
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
	  				break;
	  			}
	  		}
	  		if (!found) {
				f->eax = -1;
	  		}	  	
		}
    	break;
    }                 
    case SYS_WRITE: {/* Write to a file. */
    	check_pointer( (int*)f ->esp+1);
    	int fd = *((int*)f->esp + 1);
    	check_pointer( (int*)f ->esp+2);
    	check_pointer( *((int*)f->esp + 2));
		void* buffer = (void*)(*((int*)f->esp + 2));
		check_pointer( (int*)f ->esp+3 );
		unsigned size = *((unsigned*)f->esp + 3);
		
		f->eax = write(fd, buffer, size);
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
  }
}

void check_pointer (const void *address) {
   if (!is_user_vaddr(address) || address < (void *)0x08048000 ||  !pagedir_get_page(thread_current()->pagedir,address))
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

int wait(pid_t pid) {

}

bool create(const char* file, unsigned initial_size) {

}

bool remove(const char* file) {

}

int open(const char* file) {

}

int filesize(int fd) {

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
}

void seek(int fd, unsigned position) {
	struct list_elem * first = list_begin (&thread_current()->list_of_file);
	struct list_elem* ihatethis;
	bool found = false;
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
}

void close(int fd) {

}