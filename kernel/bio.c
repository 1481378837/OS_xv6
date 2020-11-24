// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13
#define BLOCKHASH(x) (x%NBUCKETS)
//functions added

//end
struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf hashbucket[NBUCKETS]; 
} bcache;

void binit(void)
{
  struct buf *b;
  for(int i=0; i<NBUCKETS; i++)
  {
    initlock(&bcache.lock[i], "bcache");
    b = &bcache.hashbucket[i]; //init each hashbucket
    b->prev = b;
    b->next = b;
  }
  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].next->prev = b; //init buffer in index_0
    bcache.hashbucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf* bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = BLOCKHASH(blockno);
  acquire(&bcache.lock[index]);

  // Is the block already cached?
  // search in the bucket
  for(b = bcache.hashbucket[index].next; b != &bcache.hashbucket[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  int next_block = (index+1)%NBUCKETS; // recycle double linked list
  while(next_block!=index)
  {
    acquire(&bcache.lock[next_block]);
    for(b = bcache.hashbucket[next_block].prev; b != &bcache.hashbucket[next_block]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        //unlink from list
        b->next->prev = b->prev;
        b->prev->next = b->next;
        //
        release(&bcache.lock[next_block]);
        //insert it into bucket_head
        b->next = bcache.hashbucket[index].next;
        b->prev = &bcache.hashbucket[index];
        //adjust list pointer
        bcache.hashbucket[index].next -> prev = b;
        bcache.hashbucket[index].next = b;
        release(&bcache.lock[index]);

        acquiresleep(&b->lock);
        return b;
      }
    }
    // not found
    release(&bcache.lock[next_block]);
    next_block = (next_block+1) % NBUCKETS;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf* bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b->dev, b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int index = BLOCKHASH(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[index].next;
    b->prev = &bcache.hashbucket[index];
    bcache.hashbucket[index].next->prev = b;
    bcache.hashbucket[index].next = b;
  }
  
  release(&bcache.lock[index]);
}

void bpin(struct buf *b) {
  int index = BLOCKHASH(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt++;
  release(&bcache.lock[index]);
}

void bunpin(struct buf *b) {
  int index = BLOCKHASH(b->blockno);
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}


