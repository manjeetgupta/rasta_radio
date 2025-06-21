#include "fifo.h"

#include "rmemory.h"
#include <stdlib.h>
#include <stdio.h>
#include "print_util.h"
#include "rastautil.h"


fifo_t * fifo_init(unsigned int max_size){
    fifo_t * fifo = rmalloc(sizeof(fifo_t));

    fifo->size = 0;
    fifo->max_size = max_size;
    fifo->head = NULL;
    fifo->tail = NULL;

    return fifo;
}

void fifo_init_1(fifo_t *fifo, unsigned int max_size) {
    fifo->size = 0;
    fifo->max_size = max_size;
    fifo->head = NULL;
    fifo->tail = NULL;
}


void * fifo_first_element(fifo_t * fifo)
{
	void * res= NULL;
	if (fifo->size > 0 && fifo->head != NULL && fifo->tail != NULL)
	{
	        struct fifo_element * element = fifo->head;
	        res = element->data;
	}

	return res;
}


void * fifo_pop(fifo_t * fifo){
    void * res= NULL;

    if (fifo->size > 0 && fifo->head != NULL && fifo->tail != NULL){
        struct fifo_element * element = fifo->head;
        res = element->data;
        fifo->head = fifo->head->next;

        if (fifo->size == 1){
            fifo->tail = NULL;
        }
        //to be changed to pool
//        rfree(element); //MAY_19
        rfree_pool(element);
        fifo->size--;
    }

    return res;
}

void fifo_push(fifo_t * fifo, void * element,int queue_no){
    if (element == NULL){
        return;
    }

    if (fifo->size == fifo->max_size){
        PRINT_LINE("ERROR: FIFO buffer full! Logging messages might have been droppped! MAX_SIZE = %d, Queue No = %d\r\n",fifo->max_size,queue_no);

        //Freeing pool if not Inserted in FIFO
        if(queue_no != 2)
        {
            struct RastaByteArray * to_free = ( struct RastaByteArray *)element;
            rfree_pool(to_free->bytes);
            rfree_pool(to_free);
        }
        return;
    }

//    struct fifo_element * new_entry = rmalloc(sizeof(struct fifo_element)); //MAY_20
    struct fifo_element * new_entry = (struct fifo_element *)rmalloc_debug_pool(sizeof(struct fifo_element),20+queue_no);

    new_entry->data = element;
    new_entry->next = NULL;

    if (fifo->size == 0){
        fifo->head = new_entry;
        fifo->tail = fifo->head;
    } else {
        fifo->tail->next = new_entry;
        fifo->tail = new_entry;
    }

    fifo->size++;
}

unsigned int fifo_get_size(fifo_t * fifo){
    unsigned int size = fifo->size;

    return size;
}

void fifo_destroy(fifo_t * fifo){
    while (fifo->size != 0) {
        fifo_pop(fifo);
    }

    rfree(fifo);
}

void fifo_cleanup(fifo_t * fifo){
    // Clear FIFO contents without freeing the structure itself
    // This is for static FIFOs that don't need to be freed
    while (fifo->size != 0) {
        fifo_pop(fifo);
    }
    // Don't call rfree(fifo) since it's static memory
}

void** fifo_print(fifo_t * fifo,unsigned int *out_size)
{
	if(fifo->size == 0)
	{
		*out_size = 0;
		return NULL;
	}

	void **array = malloc(fifo->size* sizeof(void *));
	struct fifo_element * current = fifo->head;
	int i =0;

    while(current!=NULL)
	        {
    	 	 array[i++] = current->data;
	         current = current->next;
	        }

    *out_size = fifo->size;
return array;

}
