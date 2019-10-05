/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

void update_ticks_a() { 
	current()->s.user_ticks += get_ticks() - (current()->s.elapsed_total_ticks);
	current()->s.elapsed_total_ticks = get_ticks();
	return;
}

int update_ticks_b(int ret) {
	current()->s.system_ticks += get_ticks() - (current()->s.elapsed_total_ticks);
	current()->s.elapsed_total_ticks = get_ticks();
	return ret;
}

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	update_ticks_a();	
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	update_ticks_a();	
	return update_ticks_b(current()->PID);
}

int ret_from_fork() 
{
	return 0;
}

int sys_fork()
{
	update_ticks_a();	
	int PID=-1;
  	union task_union *pare = (union task_union *) current(); //proces actual->pare
	union task_union *fill;
	struct list_head * f = list_first(&freequeue);
  	if(list_empty(f) == 0){	
		fill = (union task_union *) list_head_to_task_struct(f);
		list_del(f);
  	}
  	else return update_ticks_b(-EAGAIN);	 //error: no hi ha lloc per un altre proces
  	copy_data((void *) pare,(void *) fill, 4096);
  	allocate_DIR(&fill->task); 	//assignem espai del directori al proces
  	fill->task.dir_pages_baseAddr = get_DIR(&fill->task);

	//inicialitzem la mida de data+stack
	int frames[NUM_PAG_DATA];
	int j = 0;
	int num_frames = 1;
	while(j < NUM_PAG_DATA && num_frames > 0){
		num_frames = alloc_frame();   //mirem si hi ha frames lliures
		frames[j] = num_frames;       //assignem la adreça a un frame
		j++;
	}
	if (num_frames == -1){		   //si no hi ha prou frames borrem la info
		while (j >=0){
			free_frame(frames[j]);	   //alliberem els frames utilitzats
			j--;
		}
		list_add_tail(&(fill->task.list),&freequeue);
		return update_ticks_b(-ENOMEM);
	}
	
	page_table_entry * pt1 = get_PT(pare);
  	page_table_entry * pt2 = get_PT(fill);
	
	//assignacio pagines logiques a frames (fisiques)
	for (j = 0; j < NUM_PAG_DATA; j++){		
		set_ss_pag(pt2, PAG_LOG_INIT_DATA+j, frames[j]);
	}

	//copia codi sistema	
	for (j = PAG_LOG_INIT_CODE; j < PAG_LOG_INIT_CODE + NUM_PAG_CODE; j++){
		pt2[j] = pt1[j];
	}
	
	//copia data+stack
	for (j = 0; j < NUM_PAG_DATA; j++){
		set_ss_pag(pt1,PAG_LOG_INIT_DATA+NUM_PAG_DATA+j, frames[j]); 	//crea entrades temporals a la tp del pare
	}	
	copy_data(PAG_LOG_INIT_DATA<<12, (PAG_LOG_INIT_DATA+NUM_PAG_DATA)<<12, NUM_PAG_DATA*PAGE_SIZE);	//copia les dades	
	for (j = 0; j < NUM_PAG_DATA; j++){
		del_ss_pag(pt1,PAG_LOG_INIT_DATA+NUM_PAG_DATA+j);	//esborra les entrades temporals
	}
	set_cr3(pare->task.dir_pages_baseAddr);		//invalida el TLB (TLB flush)

	//assignem INFO al fill
	PID = pare->task.PID;
	++PID;
	fill->task.PID = PID;
	fill->task.s.user_ticks = 0;
	fill->task.s.system_ticks = 0;
	fill->task.s.blocked_ticks = 0;
	fill->task.s.ready_ticks = 0;
	fill->task.s.elapsed_total_ticks = get_ticks();
	fill->task.s.total_trans = 0;
	fill->task.s.remaining_ticks = 10;

	//ret fill
	long y;
	__asm__ __volatile__(
		"movl %%ebp, %%eax\n\t"
		: "=a" (y)
		: );
	long z = &(pare->stack[KERNEL_STACK_SIZE]);
	fill->stack[KERNEL_STACK_SIZE-((z-y)/4)] = ret_from_fork;
	fill->stack[KERNEL_STACK_SIZE-((z-y)/4)-1] = 0x666;
	fill->task.kernel_esp = &(fill->stack[KERNEL_STACK_SIZE-((z-y)/4)-1]);
	list_add_tail( &(fill->task.list), &readyqueue );
 	return update_ticks_b(PID);
}

void sys_exit(){
	update_ticks_a();
	struct task_struct * ex = current();
	free_user_pages(ex);
	ex->PID = -1;
	update_process_state_rr(current(), &freequeue);
	sched_next_rr();
}

int sys_write(int fd,char*buffer,int size)
{
	update_ticks_a();	
	if (check_fd(fd,ESCRIPTURA) != 0) return (check_fd(fd,ESCRIPTURA));
	else if (*buffer == NULL) return -14; /*EFAULT*/
	else if (size <= 0) return -22; /*EINVAL*/
	else return sys_write_console(buffer,size);
	return 0;
}

int sys_gettime()
{
	update_ticks_a();	
	return update_ticks_b(zeos_ticks);
}

int sys_getstats(int pid, struct stats *st)
{
	update_ticks_a();	
	if (st == NULL) return update_ticks_b(-EFAULT);
	if (access_ok(ESCRIPTURA,st,sizeof(struct stats)) == 0) return update_ticks_b(-EFAULT);
	if (pid <= 0) return  update_ticks_b(-EINVAL);
	int i=0;
	int x=0;
	struct task_struct * tasca;
	while(i<NR_TASKS && x == 0){
		if (task[i].task.PID == pid) {
			x=1;
			tasca=&task[i].task;
		}
		i++;
	}
	if (i==NR_TASKS) return update_ticks_b(-ESRCH);
	copy_to_user(&tasca->s,st,sizeof(struct stats));
	return update_ticks_b(0);
}

int ret_from_clone() {
	__asm__ __volatile__(
		"iret\n\t"
		: 
		: );
	return 0;
}

int sys_clone (void (*function)(void), void *stack) {

	update_ticks_a();	
	int PID=-1;
  	union task_union *pare = (union task_union *) current();
	union task_union *clon;
	struct list_head * f = list_first(&freequeue);
  	if(list_empty(f) == 0){	
		clon = (union task_union *) list_head_to_task_struct(f);
		list_del(f);
  	}
  	else return update_ticks_b(-EAGAIN);	 //error: no hi ha lloc per un altre proces
  	copy_data((void *) pare,(void *) clon, 4096);


	page_table_entry * pt1 = get_PT(pare);
  	page_table_entry * pt2 = get_PT(clon);

	//copia codi sistema
	int j;	
	for (j = PAG_LOG_INIT_CODE; j < PAG_LOG_INIT_CODE + NUM_PAG_CODE; j++){
		pt2[j] = pt1[j];
	}

	//assignem INFO al fill
	PID = pare->task.PID;
	++PID;
	clon->task.PID = PID;
	clon->task.s.user_ticks = 0;
	clon->task.s.system_ticks = 0;
	clon->task.s.blocked_ticks = 0;
	clon->task.s.ready_ticks = 0;
	clon->task.s.elapsed_total_ticks = get_ticks();
	clon->task.s.total_trans = 0;
	clon->task.s.remaining_ticks = 10;

	
	//ret clon
	long y;
	__asm__ __volatile__(
		"movl %%ebp, %%eax\n\t"
		: "=a" (y)
		: );
	long z = &(pare->stack[KERNEL_STACK_SIZE]);
	clon->stack[KERNEL_STACK_SIZE-((z-y)/4)] = ret_from_clone;
	clon->stack[KERNEL_STACK_SIZE-((z-y)/4)+1] = function;
	clon->stack[KERNEL_STACK_SIZE-((z-y)/4)+4] = stack;
	clon->stack[KERNEL_STACK_SIZE-((z-y)/4)-1] = 0x666;
	clon->task.kernel_esp = &(clon->stack[KERNEL_STACK_SIZE-((z-y)/4)-1]);
	list_add_tail( &(clon->task.list), &readyqueue );
 	return update_ticks_b(PID);
}

int sys_sem_init (int n_sem, unsigned int value){
	if (value < 0 || (n_sem > 20 || n_sem < 0)) return -1;
	semafurus[n_sem].counter = value;
	INIT_LIST_HEAD( &semafurus[n_sem].blocked );
	semafurus[n_sem].PID = current()->PID;
	return 0;
}

int sys_sem_wait (int n_sem){
	if (n_sem > 20 || n_sem < 0) return -1;
	semafurus[n_sem].counter--;
	if (semafurus[n_sem].counter <= 0){
		list_add_tail(&current()->list, &semafurus[n_sem].blocked);
		list_del(&current()->list);
	}
	return 0;
}

int sys_sem_signal (int n_sem){
	if (n_sem > 20 || n_sem < 0) return -1;
	semafurus[n_sem].counter++;
	if (semafurus[n_sem].counter >= 0){
		list_add_tail( list_first(&semafurus[n_sem].blocked), &readyqueue);
		list_del(list_first(&semafurus[n_sem].blocked));
	}
	return 0;
}

int sys_sem_destroy (int n_sem){
	int PID = semafurus[n_sem].PID;
	if (n_sem > 20 || n_sem < 0) return -1;
	if (PID != current()->PID) return -1;
	while (list_empty(&semafurus[n_sem].blocked) != 1){
		list_add_tail( list_first(&semafurus[n_sem].blocked), &readyqueue);
		list_del(list_first(&semafurus[n_sem].blocked));
	}
	return 0;
}








