**Pintos项目1　**
=================


**实验使用环境**

Linux14.04x86操作系统环境，Pinto操作系统原型，gdb跟踪调试工具。Bachs模拟器。

1.  **实验过程与分析、调试过程**

**（1）Pintos内核剖析**

在/threads/init.c中查找main，找到系统入口函数如下：
```c
/* Pintos main program. */
int
main (void)
{
  char **argv;

  /* Clear BSS. */  
  bss_init ();

  /* Break command line into arguments and parse options. */
  argv = read_command_line ();
  argv = parse_options (argv);

  /* Initialize ourselves as a thread so we can use locks,
     then enable console locking. */
  thread_init ();
  console_init ();  

  /* Greet user. */
  printf ("Pintos booting with %'"PRIu32" kB RAM...\n",
          init_ram_pages * PGSIZE / 1024);

  /* Initialize memory system. */
  palloc_init (user_page_limit);
  malloc_init ();
  paging_init ();

  /* Segmentation. */
#ifdef USERPROG
  tss_init ();
  gdt_init ();
#endif

  /* Initialize interrupt handlers. */
  intr_init ();
  timer_init ();
  kbd_init ();
  input_init ();
#ifdef USERPROG
  exception_init ();
  syscall_init ();
#endif

  /* Start thread scheduler and enable interrupts. */
  thread_start ();
  serial_init_queue ();
  timer_calibrate ();

#ifdef FILESYS
  /* Initialize file system. */
  ide_init ();
  locate_block_devices ();
  filesys_init (format_filesys);
#endif

  printf ("Boot complete.\n");
  
  /* Run actions specified on kernel command line. */
  run_actions (argv);

  /* Finish up. */
  shutdown ();
  thread_exit ();
}
```



系统启动时，会初始化BSS，读取并分析命令行，初始化主线程、终端、内存、中断及时钟，开启线程调度，开中断，并最终运行程序后退出。

*BSS段通常是指用来存放程序中未初始化的或者初始化为0的*[全局变量](https://baike.baidu.com/item/%E5%85%A8%E5%B1%80%E5%8F%98%E9%87%8F/4725296)*和*[静态变量](https://baike.baidu.com/item/%E9%9D%99%E6%80%81%E5%8F%98%E9%87%8F/10997955)*的一块内存区域。特点是可读写的，在程序执行之前BSS段会自动清0。*

*可执行程序包括BSS段、*[数据段](https://baike.baidu.com/item/%E6%95%B0%E6%8D%AE%E6%AE%B5)*、*[代码段](https://baike.baidu.com/item/%E4%BB%A3%E7%A0%81%E6%AE%B5)*（也称文本段）。*

为了了解中断机制，首先在/device/timer.h中可以看到：
```c
#define TIMER_FREQ 100
```

即每秒100次时钟中断，定时为0.01秒。操作系统内核通过中断来获得CPU时间。

然后在timer.c中查找timer_init函数：
```c
/* Sets up the timer to interrupt TIMER_FREQ times per second,
   and registers the corresponding interrupt. */
void
timer_init (void) 
{
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");
}

```

其中0x20是中断向量号；timer_interrupt是中断处理函数的入口地址，每当时钟定时时间到就调用该函数（周期性调用）；”8254
Timer”是时钟。

查找timer_interrupt函数：
```c
/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{

  ticks++;
  thread_tick ();


}

```

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513173310473.png)
即每次时钟到时全局变量ticks自增1。

再进入/threads，先查看interrupt.h：
```c
enum intr_level
{
	INTR_OFF,
	INTR_ON
};
```

再查看thread.h中的thread结构体：

```c
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */

    int64_t ticks_blocked;              /* Record the time the thread has been blocked. */
    int base_priority;                  /* Base priority. */
    struct list locks;                  /* Locks that the thread is holding. */
    struct lock *lock_waiting;          /* The lock that the thread is waiting for. */
    int nice;                           /* Niceness. */
    fixed_t recent_cpu;                 /* Recent CPU. */
  };

```

以及thread的状态类型：

```c
/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

```

在main函数中看到，运行测试的函数是run_actions，查找：


```c
/* Executes all of the actions specified in ARGV[]
   up to the null pointer sentinel. */
static void
run_actions (char **argv) 
{
  /* An action. */
  struct action 
    {
      char *name;                       /* Action name. */
      int argc;                         /* # of args, including action name. */
      void (*function) (char **argv);   /* Function to execute action. */
    };

  /* Table of supported actions. */
  static const struct action actions[] = 
    {
      {"run", 2, run_task},
#ifdef FILESYS
      {"ls", 1, fsutil_ls},
      {"cat", 2, fsutil_cat},
      {"rm", 2, fsutil_rm},
      {"extract", 1, fsutil_extract},
      {"append", 2, fsutil_append},
#endif
      {NULL, 0, NULL},
    };

  while (*argv != NULL)
    {
      const struct action *a;
      int i;

      /* Find action name. */
      for (a = actions; ; a++)
        if (a->name == NULL)
          PANIC ("unknown action `%s' (use -h for help)", *argv);
        else if (!strcmp (*argv, a->name))
          break;

      /* Check for required arguments. */
      for (i = 1; i < a->argc; i++)
        if (argv[i] == NULL)
          PANIC ("action `%s' requires %d argument(s)", *argv, a->argc - 1);

      /* Invoke action and advance. */
      a->function (argv);
      argv += a->argc;
    }
  
}
```
第311行中可以看到run命令是actions结构体数组的第一个元素，而第324行的a则是指向结构体数组的指针，即第一个元素的地址。

所以，第340行a-\>function(argv)即以argv参数运行actions[0]的第三个参数，即run_task[argv]，其中argv[0]==”run”,argv[1]==”alarm-multiple”。

查找run_task函数：
```c
/* Runs the task specified in ARGV[1]. */
static void
run_task (char **argv)
{
  const char *task = argv[1];
  
  printf ("Executing '%s':\n", task);
#ifdef USERPROG
  process_wait (process_execute (task));
#else
  run_test (task);
#endif
  printf ("Execution of '%s' complete.\n", task);
}

```


第290行相当于run_test(“alarm-multiple”);

在/tests/threads/test.c中查找run_test函数：
```c
/* Runs the test named NAME. */
void
run_test (const char *name) 
{
  const struct test *t;

  for (t = tests; t < tests + sizeof tests / sizeof *tests; t++)
    if (!strcmp (name, t->name))
      {
        test_name = name;
        msg ("begin");
        t->function ();
        msg ("end");
        return;
      }
  PANIC ("no test named \"%s\"", name);
}

```
这里遍历了tests数组，tests数组的定义在同一文件中：
```c
static const struct test tests[] = 
  {
    {"alarm-single", test_alarm_single},
    {"alarm-multiple", test_alarm_multiple},
    {"alarm-simultaneous", test_alarm_simultaneous},
    {"alarm-priority", test_alarm_priority},
    {"alarm-zero", test_alarm_zero},
    {"alarm-negative", test_alarm_negative},
    {"priority-change", test_priority_change},
    {"priority-donate-one", test_priority_donate_one},
    {"priority-donate-multiple", test_priority_donate_multiple},
    {"priority-donate-multiple2", test_priority_donate_multiple2},
    {"priority-donate-nest", test_priority_donate_nest},
    {"priority-donate-sema", test_priority_donate_sema},
    {"priority-donate-lower", test_priority_donate_lower},
    {"priority-donate-chain", test_priority_donate_chain},
    {"priority-fifo", test_priority_fifo},
    {"priority-preempt", test_priority_preempt},
    {"priority-sema", test_priority_sema},
    {"priority-condvar", test_priority_condvar},
    {"mlfqs-load-1", test_mlfqs_load_1},
    {"mlfqs-load-60", test_mlfqs_load_60},
    {"mlfqs-load-avg", test_mlfqs_load_avg},
    {"mlfqs-recent-1", test_mlfqs_recent_1},
    {"mlfqs-fair-2", test_mlfqs_fair_2},
    {"mlfqs-fair-20", test_mlfqs_fair_20},
    {"mlfqs-nice-2", test_mlfqs_nice_2},
    {"mlfqs-nice-10", test_mlfqs_nice_10},
    {"mlfqs-block", test_mlfqs_block},
  };
```

于是第56行t-\>function()等价于test_alarm_multiple()。

同目录下查看alarm_wait.c，可以看到该函数调用了test_sleep(5,
7)，查找test_sleep函数，核心代码：

```c
static void
test_sleep (int thread_cnt, int iterations) 
{
  struct sleep_test test;
  struct sleep_thread *threads;
  int *output, *op;
  int product;
  int i;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  msg ("Creating %d threads to sleep %d times each.", thread_cnt, iterations);
  msg ("Thread 0 sleeps 10 ticks each time,");
  msg ("thread 1 sleeps 20 ticks each time, and so on.");
  msg ("If successful, product of iteration count and");
  msg ("sleep duration will appear in nondescending order.");

  /* Allocate memory. */
  threads = malloc (sizeof *threads * thread_cnt);
  output = malloc (sizeof *output * iterations * thread_cnt * 2);
  if (threads == NULL || output == NULL)
    PANIC ("couldn't allocate memory for test");

  /* Initialize test. */
  test.start = timer_ticks () + 100;
  test.iterations = iterations;
  lock_init (&test.output_lock);
  test.output_pos = output;

  /* Start threads. */
  ASSERT (output != NULL);
  for (i = 0; i < thread_cnt; i++)
    {
      struct sleep_thread *t = threads + i;
      char name[16];
      
      t->test = &test;
      t->id = i;
      t->duration = (i + 1) * 10;
      t->iterations = 0;

      snprintf (name, sizeof name, "thread %d", i);
      thread_create (name, PRI_DEFAULT, sleeper, t);
    }
  
  /* Wait long enough for all the threads to finish. */
  timer_sleep (100 + thread_cnt * iterations * 10 + 100);

  /* Acquire the output lock in case some rogue thread is still
     running. */
  lock_acquire (&test.output_lock);

  /* Print completion order. */
  product = 0;
  for (op = output; op < test.output_pos; op++) 
    {
      struct sleep_thread *t;
      int new_prod;

      ASSERT (*op >= 0 && *op < thread_cnt);
      t = threads + *op;

      new_prod = ++t->iterations * t->duration;
        
      msg ("thread %d: duration=%d, iteration=%d, product=%d",
           t->id, t->duration, t->iterations, new_prod);
      
      if (new_prod >= product)
        product = new_prod;
      else
        fail ("thread %d woke up out of order (%d > %d)!",
              t->id, product, new_prod);
    }

  /* Verify that we had the proper number of wakeups. */
  for (i = 0; i < thread_cnt; i++)
    if (threads[i].iterations != iterations)
      fail ("thread %d woke up %d times instead of %d",
            i, threads[i].iterations, iterations);
  
  lock_release (&test.output_lock);
  free (output);
  free (threads);
}

```
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513174643885.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MTcwNTcwMw==,size_16,color_FFFFFF,t_70)
可以看到for循环内创建了thread_cnt个线程，第97行使每个线程调用timer_sleep共iteration次。所以test_sleep(5,
7)即创建5个线程，每个线程调用sleep共7次。

回到/device/timer.c中查找timer_sleep函数：
```c
/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */
void
timer_sleep (int64_t ticks)
{
  if (ticks <= 0)
  {
    return;
  }
  ASSERT (intr_get_level () == INTR_ON);
 while(timer_elapsed(start)<ticks)
 	thread_yield;
}
```
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513174658494.png)

timer_sleep通过不断轮询检查经过时间是否达到参数ticks，若还没达到则调用thread_yield函数，达到了ticks就会结束休眠。

在/threads/thread.c中查找thread_yield函数：

```c
/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void)
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;

  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread)
     list_push_back (&ready_list, &cur->elem); 
   
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}
```

thread_yield中调用了schedule函数重新调度线程。

继续查找schedule函数：

```c
/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.

   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void)
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}
```

schedule函数执行了以后会把当前线程放进队列里并调度下一个线程。

因此，我们知道：timer_sleep轮询时，
thread_yield会通过schedule函数把当前线程放进ready队列，并调度下一个线程，线程调度时要保证中断关闭。

综上所述，我们可以总结：pintos中断控制流程如下图：

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513175124757.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MTcwNTcwMw==,size_16,color_FFFFFF,t_70)

**（2）Pintos忙等问题分析及实践过程**

分析timer_sleep函数，首先查找它调用的timer_ticks函数：

```c
/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void)
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}
```
查找intr_disable，试图找出其返回值：

```c
/* Disables interrupts and returns the previous interrupt status. */
enum intr_level
intr_disable (void) 
{
  enum intr_level old_level = intr_get_level ();

  /* Disable interrupts by clearing the interrupt flag.
     See [IA32-v2b] "CLI" and [IA32-v3a] 5.8.1 "Masking Maskable
     Hardware Interrupts". */
  asm volatile ("cli" : : : "memory");

  return old_level;
}
```
```c
/* Interrupts on or off? */
enum intr_level 
  {
    INTR_OFF,             /* Interrupts disabled. */
    INTR_ON               /* Interrupts enabled. */
  };
```

函数通过汇编关闭终端，并返回关中断前的中断状态。

也就是说，enum intr_level old_level =
intr_disable();关闭中断并保存之前的状态，intr_set_level(old_level);恢复之前的中断状态。这两条语句中间是原子操作。

根据之前对timer_sleep的分析，我们知道：thread_yield中，如果当前线程不是空闲线程，就将它插入队列尾部，状态变为THREAD_READY。因此，线程将不断在就绪队列和运行队列中切换，不断占用CPU资源，造成了忙等问题。

于是我们考虑将线程阻塞，记录该线程剩余被阻塞的时间ticks_blocked，并利用时钟中断检测所有线程的状态，每次使对应的ticks_blocked自减1，如果为0则唤醒对应线程。

在thread.h中添加成员：

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513175257962.png)
在thread.c的thread_create中加入初始化语句：
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513175306359.png)

为了在时钟中断时能遍历所有线程，我们使用thread_foreach函数：

```c
/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}
```
 在timer.c中的timer_interrupt函数里使用该函数：

```c
/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{

  thread_foreach (blocked_thread_check, NULL);
    ticks++;
  thread_tick ();
}
```

>   在thread.h中声明checkInvoke函数，在thread.c中实现：

```c
/* Solution Code */
void
checkInvoke(struct thread *t, void *aux UNUSED)
{
  if (t->status == THREAD_BLOCKED && t->ticks_blocked > 0)
  {
    --t->ticks_blocked;
	if (t->ticks_blocked == 0)
	  thread_unblock(t);
  }
}
```


最后修改timer_sleep函数：
```c
/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */
void
timer_sleep (int64_t ticks) 
{
  /*
  int64_t start = timer_ticks();
  ASSERT (intr_get_level () == INTR_ON);
  while (timer_elapsed(start) < ticks)
    thread_yield();
  */

  /* Solution Code */

  /* For alarm-negative && alarm-zero */


  ASSERT (intr_get_level () == INTR_ON);
  enum intr_level old_level = intr_disable();

  /* Blocks current thread for ticks */
  thread_current()->ticks_blocked = ticks;
  thread_block();

  intr_set_level(old_level);
}
```

即阻塞当前线程ticks时长，这一操作是原子的。

尝试make
check，发现本应通过的alarm-negative和alarm-zero没有通过，查看这两个测试程序的代码，发现timer_sleep的参数分别是-100和0，而要求是程序不崩溃即可。因此我们加入条件判断：

```c
/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */
void
timer_sleep (int64_t ticks) 
{
  /*
  int64_t start = timer_ticks();
  ASSERT (intr_get_level () == INTR_ON);
  while (timer_elapsed(start) < ticks)
    thread_yield();
  */

  /* Solution Code */

  /* For alarm-negative && alarm-zero */
  if (ticks <= 0) return;

  ASSERT (intr_get_level () == INTR_ON);
  enum intr_level old_level = intr_disable();

  /* Blocks current thread for ticks */
  thread_current()->ticks_blocked = ticks;
  thread_block();

  intr_set_level(old_level);
}
```
成功。

1.  **Pintos优先级调度问题分析及实践过程。**

首先thread.h中可以看到thread结构体中有priority成员，且有：
```c
/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

```
为了通过alarm-priority，我们只需要维护就绪队列为一个优先队列即可，按优先级从高到低排序。也就是说，只需要保证线程插入就绪队列时插入到了合适的位置。

经过对pintos源码的分析，我们知道只有三个函数会插入线程到就绪队列：init_thread,
thread_unblock, thread_yield。

以thread_yield为例：

```c

  old_level = intr_disable ();
  if (cur != idle_thread)
     list_push_back (&ready_list, &cur->elem); 
  


```

这里直接插入到队列尾部，因此我们不能直接使用list_push_back函数。

阅读链表实现源码，在/lib/kernel/list.h中。我们发现了一些有趣的函数：

```c
/* Operations on lists with ordered elements. */
void list_sort (struct list *,
                list_less_func *, void *aux);
void list_insert_ordered (struct list *, struct list_elem *,
                          list_less_func *, void *aux);
void list_unique (struct list *, struct list *duplicates,
                  list_less_func *, void *aux);

```

其中list_insert_ordered看起来能实现插入时维持顺序，查看其实现：
```c
/* Inserts ELEM in the proper position in LIST, which must be
   sorted according to LESS given auxiliary data AUX.
   Runs in O(n) average case in the number of elements in LIST. */
void
list_insert_ordered (struct list *list, struct list_elem *elem,
                     list_less_func *less, void *aux)
{
  struct list_elem *e;

  ASSERT (list != NULL);
  ASSERT (elem != NULL);
  ASSERT (less != NULL);

  for (e = list_begin (list); e != list_end (list); e = list_next (e))
    if (less (elem, e, aux))
      break;
  return list_insert (e, elem);
}
```

可知和猜测的一样，我们只需实现一个比较函数（先在thread.h中声明）：
```c
/* Solution Code */
bool thread_cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);
```
其中list_entry的用法来自list.h：
```c
/* Converts pointer to list element LIST_ELEM into a pointer to
   the structure that LIST_ELEM is embedded inside.  Supply the
   name of the outer structure STRUCT and the member name MEMBER
   of the list element.  See the big comment at the top of the
   file for an example. */
#define list_entry(LIST_ELEM, STRUCT, MEMBER)           \
        ((STRUCT *) ((uint8_t *) &(LIST_ELEM)->next     \
                     - offsetof (STRUCT, MEMBER.next)))
```

随后修改thread_yield函数：
```c
/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void)
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;

  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread)
    /* list_push_back (&ready_list, &cur->elem); */
    list_insert_ordered (&ready_list, &cur->elem, (list_less_func *) &thread_cmp_priority, NULL);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

```

最后，对init_thread和thread_unblock中的list_push_back函数替换为list_insert_ordered，并作类似修改即可。

接下来实现优先级变化和抢占调度，即priority-change和priority-preempt。先查看两个测试的代码，发现在改变一个线程的优先级时，所有线程的执行顺序都会受到影响。因此我们只能在线程优先级发生变化时将其插入就绪队列，这样才能对所有线程重新排序。

同理，在创建线程时，如果优先级高于当前线程，当前线程也会被插入就绪队列。

因此我们修改thread.c中的thread_set_priority：
```c
void
thread_set_priority (int new_priority) 
{
  
  thread_current ()->priority = new_priority;
  thread_yield();
  ...

}
```
并且，在thread_create的最后，根据新线程优先级判断是否要将当前线程插入就绪队列。
```c

/*Add to run queue. */
  thread_unblock (t);

  /* Solution Code */
  /* Preempt the current thread */
  if (thread_current()->priority < priority)
    thread_yield();

  return tid;
}
```

尝试make check，成功通过了这两个测试。

最后，为了防止优先级反转，我们需要实现优先级捐赠，而这将带来诸多问题，从tests的数量也能看出这一点。

这次要通过的tests包括：priority-donate-one, priority-donate-multiple,
priority-donate-multiple2, priority-donate-nest, priority-donate-sema,
priority-donate-lower, priority-sema, priority-condvar,
priority-donate-chain共9个。我们分别对这9个测试的代码进行分析。由于代码较长这里略去了代码部分，只阐述分析的结果。

donate-one中出现了新的函数lock_acquire,
lock_release，含义分别是锁的获取与释放。这个测试说明，线程A获取锁时如果发现拥有锁的线程B优先级比自己低，则提升线程B的优先级；在线程B释放锁后，还要恢复线程B原来的优先级。

donate-multiple与donate-multiple2中出现了两个锁，要求在恢复优先级时，考虑其它线程对该线程的优先级捐赠。这就需要我们记录给线程捐赠了优先级的所有线程。

donate-nest中有优先级分别为高中低的三个线程H,M,L。当M在等待L的锁时，L的优先级会被提升为M。而当H来获取M的锁时，M和L的优先级都会提升为H。这等于要求优先级的捐赠是递归捐赠的。因此我们还需要记录线程正在等待哪个线程释放锁。

donate-sema中出现了sema_up和sema_down，对应着V操作和P操作。加入了信号量的调整，但其实没有使问题更复杂，因为锁的本质也是信号量的变化。

donate-lower则修改了一个优先级被捐赠的线程的优先级。在修改时线程优先级依然是被捐赠的优先级，但释放锁后线程的优先级变成了修改后的优先级。

sema和condvar两个测试逻辑十分简单，要求信号量的等待队列和condition的waiters队列都是优先队列。

donate-chain则要求实现链式优先级捐赠，释放锁后如果线程没有被捐赠，则需要立即恢复原来的优先级。

经过上述分析，我们考虑用代码来实现这些逻辑。首先在thread.h中加入如下成员并在thread.c的init_thread中初始化：
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513185540612.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513185555752.png)
随后在synch.h中的lock结构体中加入成员：

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513185607745.png)

修改synch.c中的lock_acquire函数：

```c
/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));
  /*
  sema_down (&lock->semaphore);
  lock->holder = thread_current ();
  */

  /* Solution Code */
  struct thread *cur = thread_current();
  struct lock *tmp;

  /* Lock is currently held by somethread */
  if (!thread_mlfqs && lock->holder != NULL)
  {
    cur->lock_waiting4 = lock;
	tmp = lock;
	while (tmp != NULL && tmp->max_priority < cur->priority)
	{
	  /* Update the max priority */
	  tmp->max_priority = cur->priority;
	  /* Donate priority to its holder thread */
	  thread_donate_priority(tmp->holder);
	  /* Continue donation to threads the holder is waiting for */
	  tmp = tmp->holder->lock_waiting4;
	}
  }
  sema_down(&lock->semaphore);

  enum intr_level old_level = intr_disable();
  cur = thread_current();
  if (!thread_mlfqs)
  {
	/* Now that I've got the lock, I'm not waiting for anylock. */
    cur->lock_waiting4 = NULL;
	/* Besides, the max_priority of this lock must be my priority. */
	lock->max_priority = cur->priority;
	thread_hold_lock(lock);
  }
  lock->holder = cur;

  intr_set_level(old_level);
}
```

这里用
```c
while (tmp != NULL && tmp->max_priority < cur->priority)
	{
	  /* Update the max priority */
	  tmp->max_priority = cur->priority;
	  /* Donate priority to its holder thread */
	  thread_donate_priority(tmp->holder);
	  /* Continue donation to threads the holder is waiting for */
	  tmp = tmp->holder->lock_waiting4;
	}
```
循环实现了递归捐赠，并通过修改锁的max_priority成员，再通过thread_update_priority函数更新优先级来实现优先级捐赠。

我们在thread.h中声明如下函数：
```c
void thread_donate_priority(struct thread *t);
void thread_hold_lock(struct lock *lock);
void thread_remove_lock(struct lock *lock);
bool lock_cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);
void thread_update_priority(struct thread *t);

```

首先实现thread_donate_priority和thread_hold_lock：

```c
/* Donate the priority of current thread to thread t. */
void
thread_donate_priority(struct thread *t)
{
  enum intr_level old_level = intr_disable();
  thread_update_priority(t);
 
  /* Remove the old t and insert the new one in order */ 
  if (t->status == THREAD_READY)
  {
    list_remove(&t->elem);
	list_insert_ordered(&ready_list, &t->elem, thread_cmp_priority, NULL);
  }

  intr_set_level(old_level);
}

```

顺便实现lock_cmp_priority函数：

```c
/* Function for lock max priority comparison. */
bool
lock_cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  return list_entry(a, struct lock, elem)->max_priority > list_entry(b, struct lock, elem)->max_priority;
}

```

随后我们改变释放锁时的行为，即修改synch.c中的lock_release函数：
```c
/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  /* Solution Code */
  if (!thread_mlfqs)
    thread_remove_lock(lock);

  lock->holder = NULL;
  sema_up (&lock->semaphore);
}
```


并实现thread_remove_lock函数：

```c
/* Remove the lock for the thread. */
void
thread_remove_lock(struct lock *lock)
{
  enum intr_level old_level = intr_disable();
  list_remove(&lock->elem);
  thread_update_priority(thread_current());

  intr_set_level(old_level);
}
```

最后我们实现在很多地方都用到的thread_update_priority：
```c
/* Update the thread's priority. */
void
thread_update_priority(struct thread *t)
{
  enum intr_level old_level = intr_disable();
  int max_pri = t->base_priority;
  int lock_pri;

  /* If the thread is holding locks, pick the one with the highest max_priority.
   * And if this priority is greater than the original base priority,
   * the real(donated) priority would be updated.*/
  if (!list_empty(&t->locks_holding))
  {
    list_sort(&t->locks_holding, lock_cmp_priority, NULL);
	lock_pri = list_entry(list_front(&t->locks_holding), struct lock, elem)->max_priority;
    if (max_pri < lock_pri)
	  max_pri = lock_pri;
  }
  t->priority = max_pri;

  intr_set_level(old_level);
}

```

该函数处理了释放锁时优先级的变化：如果当前线程还有锁，则获取其拥有锁的max_priority。如果它大于base_priority则更新被捐赠的优先级。

最后，我们修改thread_set_priority函数：

```c
/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority) 
{
  /*
  thread_current ()->priority = new_priority;
  thread_yield();
  */

  /* Solution Code */
  if (thread_mlfqs)
    return;
  enum intr_level old_level = intr_disable();
  struct thread *cur = thread_current();
  int old_priority = cur->priority;
  cur->base_priority = new_priority;

  if (list_empty(&cur->locks_holding) || new_priority > old_priority)
  {
    cur->priority = new_priority;
	thread_yield();
  }
  intr_set_level(old_level);
}
```
关于priority-donate系列的测试应该没有问题了，但是还没有实现sema和condvar的两个优先队列，因此接下来我们实现这两个优先队列。

首先修改synch.c中的cond_signal函数：

```c
/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) 
  {
	/* Solution Code */
	list_sort(&cond->waiters, cond_cmp_priority, NULL);

    sema_up (&list_entry (list_pop_front (&cond->waiters), struct semaphore_elem, elem)->semaphore);
  }
}
```

然后声明并实现比较函数cond_cmp_priority：

```c
/* Solution Code */
/* Function for condvar waiters priority comparison. */
bool
cond_cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  struct semaphore_elem *sa = list_entry(a, struct semaphore_elem, elem);
  struct semaphore_elem *sb = list_entry(b, struct semaphore_elem, elem);
  return list_entry(list_front(&sa->semaphore.waiters), struct thread, elem)->priority > \ 
		 list_entry(list_front(&sb->semaphore.waiters), struct thread, elem)->priority;
}
```

这样就实现了将条件变量等待队列变为优先队列，类似地，分别修改sema_up和sema_down：

```c
/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) 
  {
    /* Solution Code */
	list_sort(&sema->waiters, thread_cmp_priority, NULL);

	thread_unblock(list_entry(list_pop_front(&sema->waiters), struct thread, elem));
  }
  sema->value++;
  /* Solution Code */
  thread_yield();

  intr_set_level (old_level);
}
```
```c
/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
	  /* Solution Code */
      list_insert_ordered(&sema->waiters, &thread_current ()->elem, thread_cmp_priority, NULL);

      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

```

这样实现了将信号量队列变为优先队列。完成之后，进行make check，成功。

**（4）Pintos高级调度的策略及实践过程。**

参考资料：<http://www.ccs.neu.edu/home/amislove/teaching/cs5600/fall10/pintos/pintos_7.html>

可以发现，实验要求实现64级调度队列，每个队列有自己的优先级，范围是PRI_MIN到PRI_MAX。随后操作系统从高优先级队列开始调度线程，并且会随时间推移动态更新线程优先级。

这里的公式计算涉及到容易令人困惑的浮点数运算，根据参考资料，我们不妨先在threads目录下创建fixed-point.h文件，保存关于浮点运算的一些宏定义。这里我们用16位数表示小数部分，因此对整数的运算必须从第17位开始。

```c
#ifndef __THREAD_FIXED_POINT_H
#define __THREAD_FIXED_POINT_H

/* Basic definitions of fixed point. */
typedef int fixed_t;
/* 16 LSB used for fractional part. */
#define FP_SHIFT_AMOUNT 16
/* Convert a value to a fixed-point value. */
#define FP_CONST(A) ((fixed_t)(A << FP_SHIFT_AMOUNT))
/* Add two fixed-point value. */
#define FP_ADD(A,B) (A + B)
/* Add a fixed-point value A and an int value B. */
#define FP_ADD_MIX(A,B) (A + (B << FP_SHIFT_AMOUNT))
/* Subtract two fixed-point value. */
#define FP_SUB(A,B) (A - B)
/* Subtract an int value B from a fixed-point value A. */
#define FP_SUB_MIX(A,B) (A - (B << FP_SHIFT_AMOUNT))
/* Multiply a fixed-point value A by an int value B. */
#define FP_MULT_MIX(A,B) (A * B)
/* Divide a fixed-point value A by an int value B. */
#define FP_DIV_MIX(A,B) (A / B)
/* Multiply two fixed-point value. */
#define FP_MULT(A,B) ((fixed_t)(((int64_t) A) * B >> FP_SHIFT_AMOUNT))
/* Divide two fixed-point value. */
#define FP_DIV(A,B) ((fixed_t)((((int64_t) A) << FP_SHIFT_AMOUNT) / B))
/* Get the integer part of a fixed-point value. */
#define FP_INT_PART(A) (A >> FP_SHIFT_AMOUNT)
/* Get the rounded integer of a fixed-point value. */
#define FP_ROUND(A) (A >= 0 ? ((A + (1 << (FP_SHIFT_AMOUNT - 1))) >> FP_SHIFT_AMOUNT) \
				: ((A - (1 << (FP_SHIFT_AMOUNT - 1))) >> FP_SHIFT_AMOUNT))
#endif /* threads/fixed-point.h */


```

有了这个头文件后，实现代码逻辑只需要按照参考资料的公式即可，没有什么难度。

先修改timer_interrupt函数。由参考资料我们知道，每次时钟中断，运行线程的recent_cpu都会加1，并且每TIMER_FREQ个ticks更新系统load_avg和所有线程的recent_cpu，每4个ticks更新线程优先级。因此加入代码：

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513190528860.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MTcwNTcwMw==,size_16,color_FFFFFF,t_70)

接下来，在thread.c中实现recent_cpu自增函数（并在thread.h中声明，下同）：

```c
/* Recent CPU++; */
void
mlfqs_inc_recent_cpu()
{
  ASSERT(thread_mlfqs);
  ASSERT(intr_context());

  struct thread *cur = thread_current();
  if (cur == idle_thread)
    return;
  cur->recent_cpu = FP_ADD_MIX(cur->recent_cpu, 1);
}
```
实现更新系统load_avg和所有线程recent_cpu的函数：

```c
/* Update load_avg and recent_cpu of all threads every TIMER_FREQ ticks. */
void
mlfqs_update_load_avg_and_recent_cpu()
{
  ASSERT(thread_mlfqs);
  ASSERT(intr_context());

  size_t ready_cnt = list_size(&ready_list);
  if (thread_current() != idle_thread)
    ++ready_cnt;
  load_avg = FP_ADD (FP_DIV_MIX (FP_MULT_MIX (load_avg, 59), 60), FP_DIV_MIX(FP_CONST(ready_cnt), 60));

  struct thread *t;
  struct list_elem *e;
  for (e = list_begin(&all_list); e != list_end(&all_list); e = list_next(e))
  {
    t = list_entry(e, struct thread, allelem);
    if (t != idle_thread)
    {
      t->recent_cpu = FP_ADD_MIX (FP_MULT (FP_DIV (FP_MULT_MIX (load_avg, 2), \ 
					  FP_ADD_MIX (FP_MULT_MIX (load_avg, 2), 1)), t->recent_cpu), t->nice);
	  mlfqs_update_priority(t);
	}
  }
}

```

实现更新优先级的函数：

```c
/* Update thread's priority. */
void
mlfqs_update_priority(struct thread *t)
{
  ASSERT(thread_mlfqs);

  if (t == idle_thread)
    return;

  t->priority = FP_INT_PART (FP_SUB_MIX (FP_SUB (FP_CONST (PRI_MAX), \ 
							 FP_DIV_MIX (t->recent_cpu, 4)), 2 * t->nice));
  if (t->priority < PRI_MIN)
    t->priority = PRI_MIN;
  else if (t->priority > PRI_MAX)
    t->priority = PRI_MAX;
}
```

最后我们在thread结构体中加入成员并在init_thread中初始化：

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513190752309.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513190804707.png)

然后我们在thread.c中完成善后工作。

声明全局变量load_avg：

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513190815660.png)

thread_start中初始化load_avg：

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513190822485.png)

在thread.h中包含浮点运算头文件：

![在这里插入图片描述](https://img-blog.csdnimg.cn/2019051319083066.png)

实现四个还没有完成的函数：
```c
/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice UNUSED) 
{
  /* Solution Code */
  thread_current()->nice = nice;
  mlfqs_update_priority(thread_current());
  thread_yield();
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
  /* Solution Code */
  return thread_current()->nice;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
  /* Solution Code */
  return FP_ROUND (FP_MULT_MIX (load_avg, 100));
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
  /* Solution Code */
  return FP_ROUND (FP_MULT_MIX (thread_current()->recent_cpu, 100));
}

```

到此为止，任务应该全部完成了，我们make check之后得到”All 27 tests
passed.”，圆满成功。

**实验结果及分析**

**（1）Pintos忙等问题的实践结果及分析。**

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513190927257.png)

原始pintos内核进行线程调度时，单纯地让线程按顺序通过忙等待占用CPU，造成了资源的浪费。这时将需要等待的线程阻塞并适时唤醒便可以避免这一情况。

忙等问题需要我们对pintos内核的中断处理机制拥有十分清晰且深入的理解，这并不容易。在理解遇到问题时，我尝试通过阅读用于测试的代码，并以此作为例子来帮助自己理解其调度机制。

而在zero和negative两个测试没有通过时，也是因为及时想到了阅读测试代码使得我没有在这两个简单的测试上浪费太多时间。因此我认为，阅读测试代码对于通过测试是十分关键的。

**（2）Pintos优先级调度问题实践结果及分析。**

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513190935150.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MTcwNTcwMw==,size_16,color_FFFFFF,t_70)优先级调度问题难度较大。其中，不涉及优先级捐赠的部分不是很难，关键在于对内核中实现的数据结构（链表）了解透彻，并妥善利用其提供的函数来简化对数据结构的操作，从而简化线程调度的过程。

我花费了非常多的时间对付各种编译和运行时的错误，其中有一部分（如缺页错误）我现在还不清楚原因。对于优先级捐赠部分的9个测试，由于它们关系十分紧密，我选择了直接分析完所有测试代码后综合考虑，并一起改代码。实际上，当我改完优先级捐赠的主要部分，而没有实现信号量和条件变量的两个优先队列时，我尝试了make
check，结果只通过了donate-chain。跑单个donate系列的测试时会出现缺页错误，原因是/lib/kernel/list.c中的一个断言条件不满足。我认为这主要是因为sema_up和sema_down没有作相应的更改，却在donate要用的函数中用到了。

这种对几个测试综合分析并写代码的方式对思维十分具有挑战性，但是我认为却是效率最高的方法。

**（3）Pintos高级调度策略的实践结果及分析。**
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513191003171.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MTcwNTcwMw==,size_16,color_FFFFFF,t_70)

高级调度系列的测试必须要查看参考资料的说明，并根据资料实现浮点数运算。在一个头文件中规定一个数据类型及其运算不失为一种整洁而简便的方法。这步之后，代码实现仅仅是套公式和细节处理而已，可以说是十分简单了。

**六、Pintos项目１小结**

关于经验总结与心得体会在第五部分已经阐述得很详尽了，一言以蔽之，我认为最重要的是耐心地查看并理解相关的代码。实际上，耐心在自己装bochs和pintos的时候也同样重要。

![在这里插入图片描述](https://img-blog.csdnimg.cn/20190513191013709.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl80MTcwNTcwMw==,size_16,color_FFFFFF,t_70)参考博客：
https://www.cnblogs.com/laiy/p/pintos_project1_thread.html
感谢！

