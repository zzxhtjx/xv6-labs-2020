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

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;

struct {
  struct spinlock lock;
  struct buf head;
} bucket[13];

void
binit(void)
{
  struct buf* b;
  int i;
  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < 13; i++){
    initlock(&bucket[i].lock, "bcache");//全部都进行初始化为bcache
    bucket[i].head.next = &bucket[i].head;
    bucket[i].head.prev = &bucket[i].head;
    bucket[i].head.id = i;
  }
  for(b = bcache.buf,i = 0; b < bcache.buf + NBUF; b++,i++){
    int j = i % 13;
    b->next = bucket[j].head.next;
    b->prev = &bucket[j].head;
    initsleeplock(&b->lock, "buffer");
    b->id = j;
    bucket[j].head.next->prev = b;
    bucket[j].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = blockno % 13;
  acquire(&bucket[id].lock);

  // Is the block already cached?
  struct buf* tmp = 0;
  for(b = bucket[id].head.next; b != &bucket[id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
    else if(b->refcnt == 0){
      tmp = b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 回收资源,回收一些已经是被释放了之后的资源

  if(tmp != 0) {
    tmp->dev = dev;
    tmp->blockno = blockno;
    tmp->valid = 0;
    tmp->refcnt = 1;
    release(&bucket[id].lock);
    acquiresleep(&tmp->lock);
    return tmp;
  }

  //需要对别的bucket进行抢用
  acquire(&bcache.lock);
  for(b = bcache.buf; b < (bcache.buf + NBUF); b += 1){
    if(b->refcnt == 0){
      //开始进行释放
      int idold = b->id;
      acquire(&bucket[idold].lock);
      b->next->prev = b->prev;
      b->prev->next = b->next;
      b->next = bucket[id].head.next;
      b->prev = &bucket[id].head;
      bucket[id].head.next->prev = b;
      bucket[id].head.next = b;
      b->id = id;
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bucket[idold].lock);
      release(&bcache.lock);
      release(&bucket[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int id = b->id;
  acquire(&bucket[id].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bucket[id].head.next;
    b->prev = &bucket[id].head;
    bucket[id].head.next->prev = b;
    bucket[id].head.next = b;
  }
  
  release(&bucket[id].lock);
}

void
bpin(struct buf *b) {
  acquire(&bucket[b->id].lock);
  b->refcnt++;
  release(&bucket[b->id].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bucket[b->id].lock);
  b->refcnt--;
  release(&bucket[b->id].lock);
}


