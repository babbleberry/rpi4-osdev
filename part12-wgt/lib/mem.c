// Define the heap

extern unsigned char _end[];
// unsigned char *HEAP_START = &_end[0]; // End of kernel

unsigned char *HEAP_START = (unsigned char *)0x400000; // Top of stack
unsigned int   HEAP_SIZE  = 0x30000000; // Max heap size is 768Mb
unsigned char *HEAP_END;

// Set up some static globals

static unsigned char *freeptr;
static unsigned int bytes_allocated = 0;

void mem_init()
{
   // Align the start of heap to an 8-byte boundary

   if ((long)HEAP_START % 8 != 0) {
      HEAP_START += 8 - ((long)HEAP_START % 8);
   }
   HEAP_END = (unsigned char *)(HEAP_START + HEAP_SIZE);

   freeptr = HEAP_START;
}

void *malloc(unsigned int size)
{
   if (size > 0) {
      void *allocated = freeptr;
      if ((long)allocated % 8 != 0) {
         allocated += 8 - ((long)allocated % 8);
      }
    
      if ((unsigned char *)(allocated + size) > HEAP_END) {
         return 0;
      } else {
         freeptr += size;
         bytes_allocated += size;

         return allocated;
      }
   }
   return 0;
}

void free(void *ptr) {
   // TODO
}
