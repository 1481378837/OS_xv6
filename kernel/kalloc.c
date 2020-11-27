// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define KMEMNUM 5 // set kmem_num to 5
void freerange(void *pa_start, void *pa_end); //alloc free mem


// added functions
void *steal(); //steal free mem from other process
int nowcpu(); // return present cpu_id
// end

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
} ;


struct kmem kmem_list[KMEMNUM];

int nowcpu()
{
  push_off();
  int id = cpuid();
  pop_off();
  return id;
}


void kinit()
{
  for(int i=0; i<KMEMNUM;i++)
  {
    initlock(&kmem_list[i].lock, "kmem"); //初始化KMEMNUM个锁
  }
  freerange(end, (void*)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  int index = nowcpu();
  acquire(&kmem_list[index].lock);
  r->next = kmem_list[index].freelist;
  kmem_list[index].freelist = r;
  release(&kmem_list[index].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void)
{
  struct run *r;
  int index = nowcpu(); //get cpu_index
  acquire(&kmem_list[index].lock);
  r = kmem_list[index].freelist;
  if(r)
    kmem_list[index].freelist = r->next; 
  release(&kmem_list[index].lock);
  if(!r)
  {
    r = steal(index); //steal from other cpu
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


void *steal()
{
  struct run *r = 0;
  for(int i=0; i<KMEMNUM; i++) // search all cpu
  {
    acquire(&kmem_list[i].lock);
    if(kmem_list[i].freelist!=0)
    {
      r = kmem_list[i].freelist;  //get free_mem
      kmem_list[i].freelist = r->next; // refresh kmem_list[i]
      release(&kmem_list[i].lock);
      return (void *) r; //return pointer of free_mem
    }
    release(&kmem_list[i].lock);
  }

  return (void *)r;
}