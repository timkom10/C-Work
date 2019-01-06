/*
 * Author: Murtaza Meerza
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Reposition this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

// List Of Macros that were provided and some found in the textbook
#define ALIGNMENT 8 // double word
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7) // rounds to nearest multiple of align 
#define SIZE_T_SIZE (ALIGN(sizeof(size_t))) // gets size 
#define WSIZE 4   // sets the size of a word
#define DSIZE 8   // sets the size of a double word
#define CHUNKSIZE 16 // sets the intial size of the heap 
#define OVERHEAD 24  // the smallest possible block size
#define MAX(x ,y)  ((x) > (y) ? (x) : (y)) // finds the max of two given inputs
#define PACK(size, alloc)  ((size) | (alloc)) // takes size and allocated byte and packs into one word
#define GET(p)  (*(size_t *)(p)) // reads the word at given address
#define PUT(p, value)  (*(size_t *)(p) = (value)) // writes the word to the given address
#define GET_SIZE(p)  (GET(p) & ~0x7)// Read  the  size  field  from  address  p
#define GET_ALLOC(p)  (GET(p) & 0x1)  // gets the allocated bit from header and footer
#define HDRP(ptr)  ((void *)(ptr) - WSIZE)  // Given  block  ptr  ptr,  compute  address  of  its  header
#define FTRP(ptr)  ((void *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE) // gets the address of the footer block
#define NEXT_BLKP(ptr)  ((void *)(ptr) + GET_SIZE(HDRP(ptr))) // gets the address of next block that isnt free
#define PREV_BLKP(ptr)  ((void *)(ptr) - GET_SIZE(HDRP(ptr) - WSIZE)) // gets the address of the last block that isnt free
#define NEXT_FREEP(ptr)  (*(void **)(ptr + DSIZE)) // gets the address of the next block that is free
#define PREV_FREEP(ptr)  (*(void **)(ptr))// gets the address of the previous block that is free

static char *heapblocks = 0; // a pointer to direct to the first block
static char *freeblocks = 0; // a pointer to direct to the first block that is free

//additonal functions for helper routines
static void *heap_extender(size_t given_words);
static void position(void *ptr, size_t size);
static void *find_fit(size_t size);
static void *coalesce(void *ptr);
static void add_to_front(void *ptr);
static void block_removal(void *ptr);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heapblocks = mem_sbrk(2 * OVERHEAD)) == NULL){                                      
        return -1;
    }

    PUT(heapblocks, 0);                                                                     
    PUT(heapblocks + WSIZE, PACK(OVERHEAD, 1));                                             
    PUT(heapblocks + DSIZE, 0);                                                            
    PUT(heapblocks + DSIZE + WSIZE, 0);                                                     
    PUT(heapblocks + OVERHEAD, PACK(OVERHEAD, 1));                                          
    PUT(heapblocks + WSIZE + OVERHEAD, PACK(0, 1));                                         
    freeblocks = heapblocks + DSIZE;                                                        

    if(heap_extender(CHUNKSIZE / WSIZE) == NULL){                                             
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t changed_size;                                                                    
    size_t new_extended_size;                                                                   
    char *ptr;                                                                               

    if(size <= 0){                                                                         
        return NULL;
    }

    changed_size = MAX(ALIGN(size) + DSIZE, OVERHEAD);                                      

    if((ptr = find_fit(changed_size))){                                                      
        position(ptr, changed_size);                                                            
        return ptr;
    }

    new_extended_size = MAX(changed_size, CHUNKSIZE);                                            

    if((ptr = heap_extender(new_extended_size / WSIZE)) == NULL){                                   
        return NULL;                                                                        
    }

    position(ptr, changed_size);                                                                
    return ptr;
}

/*
 * mm_free - Currently, freeing a block does nothing.
 * 	You must revise this function so that it frees the block.
 */
void mm_free(void *ptr)
{
    if(!ptr){                                                                                
        return;                                                                             
    }

    size_t size = GET_SIZE(HDRP(ptr));                                                       

    PUT(HDRP(ptr), PACK(size, 0));                                                           
    PUT(FTRP(ptr), PACK(size, 0));                                                           
    coalesce(ptr);                                                                           
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t prev_size;                                                                         
    void *new_ptr;                                                                            
    size_t changed_size = MAX(ALIGN(size) + DSIZE, OVERHEAD);                              

    if(size <= 0){                                                                          
        mm_free(ptr);                                                                        
        return 0;
    }

    if(ptr == NULL){                                                                         
        return mm_malloc(size);
    }

    prev_size = GET_SIZE(HDRP(ptr));                                                           

    if(prev_size == changed_size){                                                            
        return ptr;
    }

    if(changed_size <= prev_size){                                                            
        size = changed_size;                                                                

        if(prev_size - size <= OVERHEAD){                                                     
            return ptr;                                                                      
        }
                                                                                            
        PUT(HDRP(ptr), PACK(size, 1));                                                       
        PUT(FTRP(ptr), PACK(size, 1));                                                       
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(prev_size - size, 1));                                  
        mm_free(NEXT_BLKP(ptr));                                                             
        return ptr;
    }
                                                                                            
    new_ptr = mm_malloc(size);                                                                

    if(!new_ptr){                                                                             
        return 0;
    }

    if(size < prev_size){
        prev_size = size;
    }

    memcpy(new_ptr, ptr, prev_size);                                                             
    mm_free(ptr);                                                                            
    return new_ptr;
}


static void* heap_extender(size_t given_words){
    char *ptr;
    size_t size;

    size = (given_words % 2) ? (given_words + 1) * WSIZE : given_words * WSIZE;                               

    if(size < OVERHEAD){
        size = OVERHEAD;
    }

    if((long)(ptr = mem_sbrk(size)) == -1){                                                  
        return NULL;
    }

    PUT(HDRP(ptr), PACK(size, 0));                                                           
    PUT(FTRP(ptr), PACK(size, 0));                                                           
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));                                                   

    return coalesce(ptr);                                                                    
}


static void *coalesce(void *ptr){
    size_t prev_alloc_block = GET_ALLOC(FTRP(PREV_BLKP(ptr))) || PREV_BLKP(ptr) == ptr;          
    size_t next_alloc_block = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));                                    
    size_t size = GET_SIZE(HDRP(ptr));                                                       

    if(prev_alloc_block && !next_alloc_block){                                                     
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));                                              
        block_removal(NEXT_BLKP(ptr));                                                        
        PUT(HDRP(ptr), PACK(size, 0));                                                       
        PUT(FTRP(ptr), PACK(size, 0));                                                       
    }

    else if(!prev_alloc_block && next_alloc_block){                                                
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));                                              
        ptr = PREV_BLKP(ptr);                                                                 
        block_removal(ptr);                                                                   
        PUT(HDRP(ptr), PACK(size, 0));                                                       
        PUT(FTRP(ptr), PACK(size, 0));                                                       
    }

    else if(!prev_alloc_block && !next_alloc_block){                                               
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));              
        block_removal(PREV_BLKP(ptr));                                                        
        block_removal(NEXT_BLKP(ptr));                                                        
        ptr = PREV_BLKP(ptr);                                                                 
        PUT(HDRP(ptr), PACK(size, 0));                                                       
        PUT(FTRP(ptr), PACK(size, 0));                                                       
    }
    add_to_front(ptr);                                                                    
    return ptr;
}


static void add_to_front(void *ptr){
    NEXT_FREEP(ptr) = freeblocks;                                                            
    PREV_FREEP(freeblocks) = ptr;                                                            
    PREV_FREEP(ptr) = NULL;                                                                  
    freeblocks = ptr;                                                                        
}

static void block_removal(void *ptr){
    if(PREV_FREEP(ptr)){                                                                     
        NEXT_FREEP(PREV_FREEP(ptr)) = NEXT_FREEP(ptr);                                        
    }

    else{                                                                                   
        freeblocks = NEXT_FREEP(ptr);                                                        
    }

    PREV_FREEP(NEXT_FREEP(ptr)) = PREV_FREEP(ptr);                                            
}

static void *find_fit(size_t size){
    void *ptr;

    for(ptr = freeblocks; GET_ALLOC(HDRP(ptr)) == 0; ptr = NEXT_FREEP(ptr)){                    
        if(size <= GET_SIZE(HDRP(ptr))){                                                     
            return ptr;                                                                     
        }
    }

    return NULL;                                                                           
}

static void position(void *ptr, size_t size){
    size_t final_size = GET_SIZE(HDRP(ptr));                                                  

    if((final_size - size) >= OVERHEAD){                                                     
        PUT(HDRP(ptr), PACK(size, 1));                                                       
        PUT(FTRP(ptr), PACK(size, 1));                                                       
        block_removal(ptr);                                                                   
        ptr = NEXT_BLKP(ptr);                                                                 
        PUT(HDRP(ptr), PACK(final_size - size, 0));                                           
        PUT(FTRP(ptr), PACK(final_size - size, 0));                                           
        coalesce(ptr);                                                                      
    }

    else{                                                                                   
        PUT(HDRP(ptr), PACK(final_size, 1));                                                 
        PUT(FTRP(ptr), PACK(final_size, 1));                                                 
        block_removal(ptr);                                                                   
    }
}