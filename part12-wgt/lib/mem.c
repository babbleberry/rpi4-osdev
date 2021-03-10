extern unsigned char _end[];

// Define the heap

unsigned char *HEAP_START = &_end[0];
unsigned int   HEAP_SIZE  = 0x30000000; // Max heap size is 768Mb
unsigned char *HEAP_END;

// Set up some globals

unsigned char *freeptr;
unsigned int allocated = 0;

void mem_init()
{
   // Align the start of heap to an 8-byte boundary

   if ((long)&HEAP_START % 8 != 0) {
      HEAP_START += 8 - ((long)&HEAP_START % 8);
   }
   HEAP_END = (unsigned char *)(HEAP_START + HEAP_SIZE);

   freeptr = HEAP_START;
}

void *malloc(unsigned int size)
{
   if (size > 0) {
      void *allocated = freeptr;
    
      if ((unsigned char *)(allocated + size) > HEAP_END) {
         return 0;
      } else {
         freeptr += size;
         allocated += size; 

         return allocated;
      }
   }
   return 0;
}

void free(void *ptr) {
   // TODO
}
