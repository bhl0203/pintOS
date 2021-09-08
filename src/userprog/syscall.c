#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "lib/string.h"


static void syscall_handler (struct intr_frame *);

struct file{
	struct inode* inode;
	off_t pos;
	bool deny_write;
};

struct lock filesys_lock;
struct lock mutex_lock;

uint32_t RC;

void
syscall_init (void) 
{
  RC = 0;
  lock_init(&mutex_lock);
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
void halt(){//enum 0
	shutdown_power_off();
}
void close(int fd){
	if(thread_current()->fd[fd] == NULL) exit(-1);
	struct file* fp = thread_current()->fd[fd];
	thread_current()->fd[fd] = NULL;
	file_close(fp);
}
void exit(int status){//enum 1
	thread_current() -> exit_status = status;
	printf("%s: exit(%d)\n",thread_name(),status);
	for(int i=3;i<150;i++){
		if(thread_current()->fd[i] != NULL){
			close(i);
		}
	}
	thread_exit();
}
pid_t exec(const char* file){//enum 2
	return process_execute(file);
}
bool create(const char* file, unsigned initial_size){
	if(file == NULL) exit(-1);
	return filesys_create(file,initial_size);
}
bool remove(const char* file){
	if(file == NULL) exit(-1);
	return filesys_remove(file);
}

int open(const char* file){
	if(file == NULL) exit(-1);
	if(is_kernel_vaddr(file)) exit(-1);

	lock_acquire(&mutex_lock);
	RC++;
	if(RC == 1) lock_acquire(&filesys_lock);
	lock_release(&mutex_lock);

	int flag_open = -1;
	struct file* fp = filesys_open(file);
	if(fp == NULL) flag_open = -1;
	else{
		for(int i = 3;i<150;i++){
			if(thread_current()->fd[i] == NULL){
				if(strcmp(thread_current()->name,file) == 0){
					file_deny_write(fp);
				}
				thread_current()->fd[i] = fp;
				flag_open = i;
				break;
			}
		}
	}
	
	lock_acquire(&mutex_lock);
	RC--;
	if(RC == 0) lock_release(&filesys_lock);
	lock_release(&mutex_lock);
	return flag_open;
}

int filesize(int fd){
	if(thread_current()->fd[fd] == NULL) exit(-1);
	return file_length(thread_current()->fd[fd]);
}

int read(int fd, void *buffer, unsigned size){
	if(is_kernel_vaddr(buffer)) exit(-1);
	int i=0;
	
	lock_acquire(&mutex_lock);
	RC++;
	if(RC == 1) lock_acquire(&filesys_lock);
	lock_release(&mutex_lock);

	if(fd == 0){
		for(i=0;i<(int)size;i++){
			if(((char*)buffer)[i] == '\0'){
				break;
			}
		}
	}
	else if(fd > 2){
		if(thread_current()->fd[fd] == NULL) i = -1;
		else i = file_read(thread_current() -> fd[fd],buffer,size);
	}
	
	if(i == -1) exit(-1);


	lock_acquire(&mutex_lock);
	RC--;
	if(RC == 0) lock_release(&filesys_lock);
	lock_release(&mutex_lock);
	return i;
}

int write(int fd, const void* buffer, unsigned size){
	
	lock_acquire(&filesys_lock);
	int flag_write = -1;

	if(fd == 1) {
		putbuf(buffer,size);
		flag_write = size;
	}
	else if(fd > 2){
		if(thread_current()->fd[fd] == NULL){
			lock_release(&filesys_lock);
			exit(-1);
		}
		if(thread_current()->fd[fd]->deny_write){
			file_deny_write(thread_current()->fd[fd]);
		}
		flag_write = file_write(thread_current()->fd[fd],buffer,size);
	}
	lock_release(&filesys_lock);
	return flag_write;


}

void seek(int fd,unsigned position){
	if(thread_current()->fd[fd] == NULL) exit(-1);
	file_seek(thread_current()->fd[fd],position);
}



int wait(pid_t pid){//enum 3
	return process_wait(pid);
}

unsigned tell(int fd){
	if(thread_current()->fd[fd] == NULL) exit(-1);
	return file_tell(thread_current()->fd[fd]);
}


static void
syscall_handler (struct intr_frame *f) 
{

  uint32_t c = *(uint32_t*)(f->esp);
  switch(c){
	  case SYS_HALT : //enum 0
		halt();
	  	break;
	  case SYS_EXIT: // enum 1
		if(is_kernel_vaddr((f->esp)+4)){
			exit(-1);
		}
		exit(*(uint32_t*)(f->esp + 4));
	 	break;
	  case SYS_EXEC: // enum 2
		if(is_kernel_vaddr((f->esp)+4)){
			exit(-1);
		}
		f->eax = exec((const char*)*(uint32_t*)((f->esp)+4));
		  break;
	  case SYS_WAIT: // enum 3
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  f->eax = wait((pid_t)*(uint32_t*)((f->esp)+4));
		  break;
	  case SYS_CREATE: // enum 4
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  if(is_kernel_vaddr((f->esp)+8)){
			  exit(-1);
		  }
		  f->eax = create((const char*)*(uint32_t*)((f->esp)+4),(unsigned)*(uint32_t*)((f->esp)+8));
		  break;
	  case SYS_REMOVE: // enum 5
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  f->eax = remove((const char*)*(uint32_t*)((f->esp)+4));
		  
		  break;
	  case SYS_OPEN: // enum 6
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  f->eax = open((const char*)*(uint32_t*)((f->esp)+4));
	  	break;
	  case SYS_FILESIZE: // enum 7
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  f->eax = filesize((int)*(uint32_t*)((f->esp)+4));
		break;
	  case SYS_READ: // enum 8
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  if(is_kernel_vaddr((f->esp)+8)){
			  exit(-1);
		  }
		  if(is_kernel_vaddr((f->esp)+12)){
			  exit(-1);
		  }
		  f->eax = read((int)*(uint32_t*)((f->esp)+4),(void*)*(uint32_t*)((f->esp)+8),(unsigned)*(uint32_t*)((f->esp)+12));
		break;
	  case SYS_WRITE: // enum 9
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  if(is_kernel_vaddr((f->esp)+8)){
			  exit(-1);
		  }
		  if(is_kernel_vaddr((f->esp)+12)){
			  exit(-1);
		  }
		f->eax = write((int)*(uint32_t*)((f->esp)+4),(void*)*(uint32_t*)((f->esp)+8),(unsigned)*((uint32_t*)((f->esp)+12)));
		break;
	  case SYS_SEEK: // enum 10
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  if(is_kernel_vaddr((f->esp)+8)){
			  exit(-1);
		  }
		  seek((int)*(uint32_t*)((f->esp)+4),(unsigned)*(uint32_t*)((f->esp)+8));
		  break;
	  case SYS_TELL: // enum 11
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  f->eax = tell((int)*(uint32_t*)((f->esp)+4));
		break;
	  case SYS_CLOSE: // enum 12
		  if(is_kernel_vaddr((f->esp)+4)){
			  exit(-1);
		  }
		  close((int)*(uint32_t*)((f->esp)+4));
		break;
  }
//  thread_exit ();
}

