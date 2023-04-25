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

#define NBUCKET 13

struct {
  struct buf buf[NBUF];
  struct spinlock bucket_lock[NBUCKET];
  struct buf head[NBUCKET];
} bcache;

inline int
hash(uint dev, uint blockno)
{
  return (dev + blockno) % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.bucket_lock[i], "bcache.bucket");
    // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

  // put all buffers on free list 0
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int bucket = hash(dev, blockno);
  acquire(&bcache.bucket_lock[bucket]);

  // Is the block already cached?
  for(b = bcache.head[bucket].next; b != &bcache.head[bucket]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_lock[bucket]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket_lock[bucket]);

  // Not cached.
  // Find block with refcnt == 0 and min timestamp
  struct buf* lrublock = 0;
  for (int i = 0; i < NBUCKET; i++) {
    acquire(&bcache.bucket_lock[i]);
    struct buf* update_block = 0;
    for(b = bcache.head[i].next; b != &bcache.head[i]; b = b->next){
      if(b->refcnt == 0) {
        if ((lrublock == 0 || b->timestamp < lrublock->timestamp) &&
           (update_block == 0 || b->timestamp < update_block->timestamp)) {
          update_block = b;
        }
      }
    }
    if (update_block) {
      // insert cur lrublock (if there is)
      if (lrublock) {
        lrublock->next = bcache.head[i].next;
        lrublock->prev = &bcache.head[i];
        bcache.head[i].next->prev = lrublock;
        bcache.head[i].next = lrublock;
      }
      // remove update_block and set it to lrublock
      update_block->prev->next = update_block->next;
      update_block->next->prev = update_block->prev;
      lrublock = update_block;
    }
    release(&bcache.bucket_lock[i]);
  }

  if (!lrublock)
    panic("bget: no buffers");

  // insert lrublock to bucket.
  lrublock->dev = dev;
  lrublock->blockno = blockno;
  lrublock->valid = 0;
  lrublock->refcnt = 1;

  acquire(&bcache.bucket_lock[bucket]);
  lrublock->next = bcache.head[bucket].next;
  lrublock->prev = &bcache.head[bucket];
  bcache.head[bucket].next->prev = lrublock;
  bcache.head[bucket].next = lrublock;
  release(&bcache.bucket_lock[bucket]);

  acquiresleep(&lrublock->lock);
  return lrublock;
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

  int bucket = hash(b->dev, b->blockno);
  acquire(&bcache.bucket_lock[bucket]);
  b->refcnt--;
  if (b->refcnt == 0)
    b->timestamp = ticks;
  release(&bcache.bucket_lock[bucket]);
}

void
bpin(struct buf *b) {
  int bucket = hash(b->dev, b->blockno);
  acquire(&bcache.bucket_lock[bucket]);
  b->refcnt++;
  release(&bcache.bucket_lock[bucket]);
}

void
bunpin(struct buf *b) {
  int bucket = hash(b->dev, b->blockno);
  acquire(&bcache.bucket_lock[bucket]);
  b->refcnt--;
  release(&bcache.bucket_lock[bucket]);
}
