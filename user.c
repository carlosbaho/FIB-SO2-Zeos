#include <libc.h>
#include <utils.h>
#include <sched.h>

char buff[24];
char get[128];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  zeos_ticks = 0;
  
  runjp(); 
  while(1) { 
	//itoa(gettime(),get);	
  	//write(1,get,strlen(get));
   	//write(1,"to",2);
	//init_task1();
	//task_switch(&task[1]);	
	//itoa(get_pid(),get);
	//write(1,get,strlen(get));	
		
  } 
}
