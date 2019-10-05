/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

#include <errno.h>

int errno=0;

void perror() { 
	if      (errno == EBADF)  write(1,"Bad file number", 15);
	else if (errno == EACCES) write(1,"Permission denied", 17);
	else if (errno == EFAULT) write(1,"Bad address", 11);
	else if (errno == EINVAL) write(1,"Invalid argument", 16);
	else if (errno == ENOSYS) write(1,"Function not implemented", 23);
	else if (errno == EAGAIN) write(1,"Try again", 9);
	else if (errno == ENOMEM) write(1,"Out of memory", 13);
	else if (errno == ESRCH)  write(1,"No such process", 15);
}

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}


int write(int fd, char *buffer, int size){
	int result = 0;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (result)
		: "a" (4), "b" (fd), "c" (buffer), "d" (size));
		if (result < 0) {
			errno = result*(-1);
			result = -1;
		}		
		return result;
}


int gettime() {
	int result = 0;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (result)
		: "a" (10));
		return result;	
}


int getpid() {
	int result = 0;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (result)
		: "a" (20));
		return result;
}

int fork(){
	int pid;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (pid)
		: "a" (2));		
		return pid;
}


void exit() {
	int res;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (res)
		: "a" (1));		


}

int get_stats(int pid, struct stats * st) {
	int result = 0;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (result)
		: "a" (35), "b" (pid), "c" (st));
		if (result < 0) {
			errno = result*(-1);
			result = -1;
		}
		return result;

}

int clone (void (*function)(void), void *stack) {
	int result = 0;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (result)
		: "a" (19), "b" (function), "c" (stack));
		if (result < 0) {
			errno = result*(-1);
			result = -1;
		}
		return result;
}

int sem_init (int n_sem, unsigned int value){
	int result = 0;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (result)
		: "a" (21), "b" (n_sem), "c" (value));
		if (result < 0) {
			errno = result*(-1);
			result = -1;
		}
		return result;
}

int sem_wait (int n_sem){
	int result = 0;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (result)
		: "a" (22), "b" (n_sem));
		if (result < 0) {
			errno = result*(-1);
			result = -1;
		}
		return result;
}

int sem_signal (int n_sem){
	int result = 0;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (result)
		: "a" (23), "b" (n_sem));
		if (result < 0) {
			errno = result*(-1);
			result = -1;
		}
		return result;
}

int sem_destroy (int n_sem){
	int result = 0;
	__asm__ __volatile__(
		"int $0x80\n\t"
		: "=a" (result)
		: "a" (24), "b" (n_sem));
		if (result < 0) {
			errno = result*(-1);
			result = -1;
		}
		return result;
}



