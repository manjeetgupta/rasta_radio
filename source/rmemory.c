//
// Created by Manjeet_CRL on 13.01.2025
//malloc, free, memcopy und memset
//


#include <string.h>
#include "print_util.h"
#include "rmemory.h"

#ifndef BAREMETAL
	#include <malloc.h>
#endif


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_BLOCKS 100
#define BLOCK_SIZE 256

#define LOG_MAX_BLOCKS 10
#define LOG_BLOCK_SIZE 256


volatile uint8_t memory_pool[MAX_BLOCKS][BLOCK_SIZE] = {0};  // Global memory pool
volatile unsigned int block_used[MAX_BLOCKS] = {0};
volatile unsigned int block_used_debug[MAX_BLOCKS] = {0};
volatile static int count=-1;

//extern uint8_t memory_pool[MAX_BLOCKS][BLOCK_SIZE];  // Global memory pool
//extern unsigned int block_used[MAX_BLOCKS];

volatile uint8_t log_memory_pool[LOG_MAX_BLOCKS][LOG_BLOCK_SIZE] = {0};  // Global memory pool for logs
volatile unsigned int log_block_used[LOG_MAX_BLOCKS] = {0};
static int log_count=-1;

//uint8_t DUMMY_POOL[30000][10000] = {0};


void* pool_alloc(unsigned int size,int loc)
{
//    if(count==-1)
//    {
//        print_log("Initially allocated memeory addresses\r\n");
//        for (int i = 0 ; i < MAX_BLOCKS; ++i)
//        {
//            print_log("%p, ",&memory_pool[i]);
//        }
//        count=0;
//    }

    for (int i = 0; i < MAX_BLOCKS; ++i)
    {
        if (!block_used[i])
        {
            block_used[i] = 1;
            block_used_debug[i] = loc; //FOR DEBUG PURPOSE


//            if(count++>2000)
//            {
//                int j=0;
//                for (int i = 0 , j=0; i < MAX_BLOCKS; ++i,j++)
//                {
//                    print_log("%d: %u (%u) ",i,block_used[i],block_used_debug[i]);
////                    print_log("0x%p ,%d: %u (%u) ",&memory_pool[i],i,block_used[i],block_used_debug[i]);
//                    if(j==10)
//                    {
//                        j=0;
//                        PRINT_LINE("\r\n");
//                    }
//                }
//                count=0;
//            }

           //print_log("pool_alloc | block[%d] : %p : SIZE = %d, loc=%d\r\n",i,&memory_pool[i],size,loc);
//            if(loc == 1 || loc == 6 || loc == 9 || loc == 10 || loc == 11 || loc == 12 || loc == 13 || loc == 14 || loc == 15 || loc == 16 || loc == 17 || loc == 19 || loc == 21 || loc ==22 ||loc == 25 || loc == 27 ||loc == 28 || loc == 29|| loc == 31)
//            {
//                print_log("pool_alloc | block[%d] : %p : SIZE = %d, loc=%d\r\n",i,&memory_pool[i],size,loc);
//            }
          /*  if(i==MAX_BLOCKS-10)
            {
                PRINT_LINE("----Poool size full---------\r\n");
                for (int i = 3; i < MAX_BLOCKS-1; ++i)
                {
                    block_used[i] = 0;
                }

            }*/
            return memory_pool[i];//&memory_pool[i][0]
        }
    }
    return NULL;
}

void pool_free(const void* ptr)
{
    for (int i = 0; i < MAX_BLOCKS; ++i)
    {
        if (ptr == memory_pool[i])
        {
            block_used[i] = 0;
            block_used_debug[i] = 0;
            //memset(memory_pool[i],0,BLOCK_SIZE);
            //PRINT_LINE("---Pool_free called: block[%d] = %d, %p\r\n",i,block_used[i],ptr);
            return;
        }
    }
    PRINT_LINE("---Pool_free called for %p : Address not found\r\n",ptr);
    return;
}

void* log_pool_alloc() {
    for (int i = 0; i < LOG_MAX_BLOCKS; ++i) {
        if (!log_block_used[i])
        {
            log_block_used[i] = 1;
            return log_memory_pool[i];
        }
    }
    return NULL;
}

void log_pool_free(const void* ptr)
{
    for (int i = 0; i < LOG_MAX_BLOCKS; ++i)
    {
        if (ptr == log_memory_pool[i])
        {
            log_block_used[i] = 0;
            return;
        }
    }
    print_log("---LOG : Pool_free called for %d : Address not found\r\n",ptr);
    return;
}

void * rmalloc(unsigned int size)
{
    if(size > 0)
    {
        void *ptr = malloc(size);
        if(ptr == NULL)
        {
        	PRINT_LINE("Simple malloc: Memory allocation failed for size %u\r\n",size);
        }
        return ptr;
    }
    else
    {
        void *ptr = malloc(size);
        if(ptr == NULL)
        {
        	//PRINT_LINE("malloc(0) returned NULL\r\n");
        }
        else
        {
        	//PRINT_LINE("malloc(0) returned valid pointer %p\r\n",ptr);
            free(ptr);
        }
        return NULL;
    }
}

void * rmalloc_debug(unsigned int size,int loc)
{
    if(size > 0)
    {
        void *ptr = malloc(size);
        if(ptr == NULL)
        {
            PRINT_LINE("Memory allocation failed for size %u : Location -> %d\r\n",size,loc);
        }
        return ptr;
    }
    else
    {
        void *ptr = malloc(size);
        if(ptr != NULL)
        {
            free(ptr);
        }
        return NULL;
    }
}

void * rmalloc_debug_pool(unsigned int size,int loc) //MEMORY_LEAK
{

//        void *ptr = malloc(size);
        void *ptr = pool_alloc(size,loc);
        if(ptr == NULL)
        {
            PRINT_LINE("Memory allocation failed for size %u : Location -> %d\r\n",size,loc);
        }
//        if(size==36)
//        {
//        PRINT_LINE("Memory allocation success for size %u : Location -> %d\r\n",size,loc);
//        }
        return ptr;


}

void rfree_pool(void * element) {
//    free(element);
    if(element!=NULL)
    {
        pool_free(element);
        //PRINT_LINE("NULL Pointer\r\n");
    }

}

//These functions are used to allocate memory to Debug logs
void * log_rmalloc_debug_pool()
{

        void *ptr = log_pool_alloc();
        if(ptr == NULL)
        {
            PRINT_LINE("LOG_Memory allocation failed \r\n");
        }
        return ptr;

}

void log_rfree_pool(void * element) {
    if(element!=NULL)
    {
        log_pool_free(element);
    }

}

/*

void * rmalloc(unsigned int size) {
    return malloc(size);
}

*/

void * rrealloc(void* element, unsigned int size) {
    return realloc(element,size);
}

void rfree(void * element) {
    free(element);
}

void* rmemcpy(void *dest, const void *src, unsigned int n) {
    return memcpy(dest,src,n);
}

void* rmemset(void *dest, int ch, unsigned int n) {
    return memset(dest,ch,n);
}

void rstrcpy(char * dest, const char * src){
    strcpy(dest, src);
}

void rstrcat(char * dest, const char * src){
    strcat(dest, src);
}

int rmemcmp(const  void * a, const void * b, unsigned int len) {
    return memcmp(a, b, len);
}




