/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

struct semaphore semafurus[20];

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}

extern struct list_head blocked;

struct list_head freequeue; //declara freequeue

struct list_head readyqueue; //declara readyqueue

struct task_struct * idle_task;



/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	}
}

void init_idle(void)
{
	struct list_head * f = list_first( &freequeue); //primer valor freequeue
	union task_union *u = (union task_union *) list_head_to_task_struct(f);	
	list_del(f);
	u->task.PID = 0; //assignem un PID	 
	allocate_DIR(&u->task); 	//assignem espai de adreces al proces
	u->task.dir_pages_baseAddr = get_DIR(&u->task); 
	idle_task = &(u->task);	
	((union task_union *) idle_task)->stack[200] = cpu_idle; 
	((union task_union *) idle_task)->stack[199] = 0; //ebp
	u->task.kernel_esp = &((union task_union *) idle_task)->stack[199];
	u->task.quantum = 0;	
}

void init_task1(void)
{
	struct list_head * g = list_first( &freequeue); //primer valor freequeue
	union task_union *u = (union task_union *) list_head_to_task_struct(g);	
	list_del(g);
	u->task.PID = 1; //assignem un PID	 
	allocate_DIR(u); //assignem espai de adreces al proces
	u->task.dir_pages_baseAddr = get_DIR(u);
	set_user_pages(u);
	tss.esp0 = (u->stack) + KERNEL_STACK_SIZE;
	set_cr3(u->task.dir_pages_baseAddr);
	u->task.quantum = 10;
	u->task.s.user_ticks = 0;
	u->task.s.system_ticks = 0;
	u->task.s.blocked_ticks = 0;
	u->task.s.ready_ticks = 0;
	u->task.s.elapsed_total_ticks = get_ticks();
	u->task.s.total_trans = 0;
	u->task.s.remaining_ticks = 10;
}


void init_sched() 
{	
  	INIT_LIST_HEAD( &freequeue ); //inicialitza	
 	INIT_LIST_HEAD( &readyqueue ); //inicialitza

  	int i = 0; 
  	for (i = 0; i < NR_TASKS; ++i) {
		list_add_tail(&(task[i].task.list), &freequeue);
	}
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}


void task_switch(union task_union * new)
{
	__asm__ __volatile__(
		"pushl %%edi\n\t"
		"pushl %%esi\n\t"
		"pushl %%ebx\n\t"
		:
		: );
	inner_task_switch(new);
	__asm__ __volatile__(
		"popl %%ebx\n\t"
		"popl %%esi\n\t"
		"popl %%edi"
		:
		: );	
}


void inner_task_switch(union task_union * new1)
{
	tss.esp0 =&(new1->stack[KERNEL_STACK_SIZE]);
	set_cr3(new1->task.dir_pages_baseAddr);
	__asm__ __volatile__(
		"movl %%ebp, %0\n\t" //ebp -> current.kernel_esp
		"movl %1, %%esp\n\t" //new1.kernel_esp -> esp
		"popl %%ebp\n\t"
		: "=m" (current()->kernel_esp)
		: "m" (new1->task.kernel_esp));
		__asm__ __volatile__(
		"ret\n\t"
		: 
		: );
}

void update_sched_data_rr(){
	--current()->s.remaining_ticks;
}

int needs_sched_rr(){
	if (current()->s.remaining_ticks == 0) {
		++current()->s.total_trans;
		struct list_head * l = list_first(&readyqueue);
		if (list_empty(l) == 1) {
			current()->s.remaining_ticks = get_quantum(current());
			return 0;
		}
		return 1;
	}
	return 0;
}

void update_process_state_rr (struct task_struct *t, struct list_head * dst_queue){
	if (dst_queue!=NULL){
		list_add_tail(&t->list, dst_queue);
		current()->s.system_ticks += get_ticks() - (current()->s.elapsed_total_ticks);
		current()->s.elapsed_total_ticks = get_ticks();
	}
}

void sched_next_rr(){
	struct list_head * n = list_first(&readyqueue);
	union task_union * next;
	if (list_empty(n) == 1){
		next = &task[0];
		next->task.s.remaining_ticks = get_quantum((struct task_struct *) &task[0]);
	}
	else {
		next = list_head_to_task_struct(n);
		list_del(n);
		next->task.s.remaining_ticks = get_quantum(next);
	}
	next->task.s.ready_ticks += get_ticks() -(next->task.s.elapsed_total_ticks);
	next->task.s.elapsed_total_ticks = get_ticks();
	++next->task.s.total_trans;
	task_switch(next);
}

int get_quantum (struct task_struct *t){
	return t->quantum;
}

void set_quantum (struct task_struct *t, int new_quantum){
	t->quantum = new_quantum;
}

void schedule(){
	update_sched_data_rr();
	if (needs_sched_rr()==1){
		update_process_state_rr(current(), &readyqueue);
		sched_next_rr();
	}
	else {
		update_process_state_rr(current(), NULL);
	}
}


