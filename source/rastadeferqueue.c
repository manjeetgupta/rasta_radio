#include <stdlib.h>
#include "rmemory.h"
#include "rastadeferqueue.h"
#include "print_util.h"

// Static memory pools to replace dynamic allocations
#define MAX_DEFER_QUEUES 2
#define MAX_DEFER_QUEUE_ELEMENTS 10
#define MAX_PACKET_DATA_SIZE 1024
#define MAX_PACKET_CHECKSUM_SIZE 64

static struct defer_queue static_defer_queues[MAX_DEFER_QUEUES];
static int defer_queue_pool_index = 0;

static struct rasta_redundancy_packet_wrapper static_queue_elements[MAX_DEFER_QUEUES][MAX_DEFER_QUEUE_ELEMENTS];

// Separate pools per queue for better memory management
static unsigned char static_packet_data_pool[MAX_DEFER_QUEUES][MAX_DEFER_QUEUE_ELEMENTS][MAX_PACKET_DATA_SIZE];

static unsigned char static_packet_checksum_pool[MAX_DEFER_QUEUES][MAX_DEFER_QUEUE_ELEMENTS][MAX_PACKET_CHECKSUM_SIZE];

/**
 * finds the index of a given element inside the given queue.
 * The sequence_number is used as the unique identifier
 * @param queue the queue that will be searched
 * @param seq_nr the sequence number to be located
 * @return -1 if there is no element with the specified @p seq_nr, index of the element otherwise
 */


void initialize_element(struct RastaRedundancyPacket *packet)
{
	packet->checksum_correct =0;
	memset(&packet->checksum_type,0,sizeof(packet->checksum_type));
	packet->length=0;
	packet->reserve=0;
	packet->sequence_number=0;

	packet->data.data.bytes=NULL;
	packet->data.data.length=0;

	packet->data.checksum.bytes=NULL;
	packet->data.checksum.length=0;

	packet->data.checksum_correct = 0;
	packet->data.confirmed_sequence_number =0;
	packet->data.confirmed_timestamp=0;
	packet->data.length=0;
	packet->data.receiver_id=0;
	packet->data.sender_id=0;
	packet->data.sequence_number=0;
	packet->data.timestamp=0;
	packet->data.type=0;
}

struct RastaRedundancyPacket deep_copy_packet(const struct RastaRedundancyPacket *src)
{
	struct RastaRedundancyPacket dest;
	initialize_element(&dest);

	dest.checksum_correct = src->checksum_correct;
	dest.checksum_type=src->checksum_type;
	dest.length=src->length;
	dest.reserve=src->reserve;
	dest.sequence_number=src->sequence_number;

	// Use static memory pool - cycle through all queues
	static int global_data_index = 0;
	static int global_checksum_index = 0;
	
	int queue_id = global_data_index / MAX_DEFER_QUEUE_ELEMENTS;
	int element_id = global_data_index % MAX_DEFER_QUEUE_ELEMENTS;
	
	dest.data.data.bytes = static_packet_data_pool[queue_id][element_id];
	global_data_index = (global_data_index + 1) % (MAX_DEFER_QUEUES * MAX_DEFER_QUEUE_ELEMENTS);
	memcpy(dest.data.data.bytes,src->data.data.bytes,src->data.data.length);

	queue_id = global_checksum_index / MAX_DEFER_QUEUE_ELEMENTS;
	element_id = global_checksum_index % MAX_DEFER_QUEUE_ELEMENTS;
	
	dest.data.checksum.bytes = static_packet_checksum_pool[queue_id][element_id];
	global_checksum_index = (global_checksum_index + 1) % (MAX_DEFER_QUEUES * MAX_DEFER_QUEUE_ELEMENTS);
	memcpy(dest.data.checksum.bytes,src->data.checksum.bytes,src->data.checksum.length);

	// ORIGINAL DYNAMIC ALLOCATION CODE (COMMENTED OUT):
	// dest.data.data.bytes = rmalloc(src->data.data.length);
	// memcpy(dest.data.data.bytes,src->data.data.bytes,src->data.data.length);
	// dest.data.checksum.bytes = rmalloc(src->data.checksum.length);
	// memcpy(dest.data.checksum.bytes,src->data.checksum.bytes,src->data.checksum.length);

	dest.data.checksum_correct = src->data.checksum_correct;
	dest.data.confirmed_sequence_number =src->data.confirmed_sequence_number;
	dest.data.confirmed_timestamp=src->data.confirmed_timestamp;
	dest.data.length=src->data.length;
	dest.data.receiver_id=src->data.receiver_id;
	dest.data.sender_id=src->data.sender_id;
	dest.data.sequence_number=src->data.sequence_number;
	dest.data.timestamp=src->data.timestamp;
	dest.data.type=src->data.type;

	return dest;
}


int find_index(struct defer_queue * queue, unsigned long seq_nr){
    unsigned int index = 0;

    // naive implementation of search. performance shouldn't be an issue as the amount of messages in the queue is small
    while ( index < queue->max_count && queue->elements[index].packet.sequence_number != seq_nr )
    	{
    		++index;
    	}

    return ( index == queue->max_count ? -1 : (int)index );
}

int cmpfkt(const void * a, const void * b){
    long long a_ts = (long long)((struct rasta_redundancy_packet_wrapper*)a)->received_timestamp;
    long long b_ts = (long long)((struct rasta_redundancy_packet_wrapper*)b)->received_timestamp;

    return (int) (a_ts - b_ts);
}

/**
 * sorts the elements in the queue in ascending time (first element has oldest timestamp)
 * @param queue the queue that will be sorted
 */
void sort(struct defer_queue * queue){
    qsort(queue->elements, queue->count, sizeof(struct rasta_redundancy_packet_wrapper), cmpfkt);
}

struct defer_queue* deferqueue_init(unsigned int n_max){

	// Use static memory pool instead of malloc
	struct defer_queue *queue = &static_defer_queues[defer_queue_pool_index];
	defer_queue_pool_index = (defer_queue_pool_index + 1) % MAX_DEFER_QUEUES;

    PRINT_LINE("SIZE = %d\r\n",n_max * sizeof(struct rasta_redundancy_packet_wrapper)); //struct rasta_redundancy_packet_wrapper = 1128 bytes
    PRINT_LINE("SIZE of rasta_redundancy_packet_wrapper = %d\r\n",sizeof(struct rasta_redundancy_packet_wrapper));

    // Use static memory pool instead of rmalloc
    queue->elements = static_queue_elements[defer_queue_pool_index - 1]; // Use the same index as the queue

    // ORIGINAL DYNAMIC ALLOCATION CODE (COMMENTED OUT):
    // struct defer_queue *queue = rmalloc(sizeof(struct defer_queue));
    // queue->elements = rmalloc(n_max * sizeof(struct rasta_redundancy_packet_wrapper));

    // set count to 0
    queue->count = 0;

    // set max count
    queue->max_count = n_max;

    return queue;
}


void print_q(struct defer_queue *queue,int c,int k)
{
//    int i=0;
//    while ( i < c )
//           {
//                PRINT_LINE("--- %d ---: queue->elements[%d].packet.sequence_number = %u\r\n",k,i, queue->elements[i].packet.sequence_number);
//                ++i;
//           }
    return;
}


int deferqueue_isfull(struct defer_queue * queue)
{

    //PRINT_LINE("Inside deferq_isfull Max SIZE = %u, q_count = %u ,Address in full = %p\r\n",queue->max_count,queue->count,&queue);
    print_q(queue,queue->count,1);
    int result = (queue->count == queue->max_count);

    return result;
}



void deferqueue_add(struct defer_queue * queue, struct RastaRedundancyPacket packet, unsigned long recv_ts)
{
    if(queue->count == queue->max_count)
    {
        // queue full, return
        return;
    }

    struct rasta_redundancy_packet_wrapper element;
    element.packet=deep_copy_packet(&packet);
    //element.packet = packet; //04 MArch 2025
    element.received_timestamp = recv_ts;

    memset(&queue->elements[queue->count],0,sizeof(struct rasta_redundancy_packet_wrapper));

    // add element to the end
    queue->elements[queue->count] = element;

    // increase count
    queue->count=queue->count+1;

    // sort array
    sort(queue);
    if(queue->max_count<=4)
    	{ print_q(queue,queue->count,3);}
    //PRINT_LINE("Inside deferqueue_add : After addtion of seq_nr = %u, total q_count = %d,ADDRESS  in add = %p\r\n",packet.sequence_number,queue->count,&queue);

}


void deferqueue_remove(struct defer_queue * queue, unsigned long seq_nr){
    int index = find_index(queue, seq_nr);
    if (index < 0){
        // element not in queue
        return;
    }

    PRINT_LINE("Inside deferqueue_remove : Removing sqq_nr = %u at index %d,Before deletion total q_count = %d\r\n",seq_nr,index,queue->count);

    // No need to free static memory
    // free(queue->elements[index].packet.data.data.bytes);
    queue->elements[index].packet.data.data.length=0;
    queue->elements[index].packet.data.data.bytes=NULL;

    // No need to free static memory
    // free(queue->elements[index].packet.data.checksum.bytes);
    queue->elements[index].packet.data.checksum.length=0;
    queue->elements[index].packet.data.checksum.bytes=NULL;

    // ORIGINAL DYNAMIC DEALLOCATION CODE (COMMENTED OUT):
    // free(queue->elements[index].packet.data.data.bytes);
    // free(queue->elements[index].packet.data.checksum.bytes);

    //Shift elements to close the gap
    for (int i=index;i< queue->count-1;i++)
    {
    	queue->elements[i] = queue->elements[i+1];
    }

    memset(&queue->elements[queue->count-1],0,sizeof(struct rasta_redundancy_packet_wrapper));

    // decrease counter
    queue->count = queue->count -1;

    sort(queue);
    PRINT_LINE("Inside deferqueue_remove : After deletion total q_count = %d, Address in remove of queue = %p\r\n",queue->count,&queue);
    print_q(queue,queue->count,1);
}


int deferqueue_contains(struct defer_queue * queue, unsigned long seq_nr){
    int result = (find_index(queue, seq_nr) != -1);

    return result;
}

void deferqueue_destroy(struct defer_queue * queue){
    // No need to free static memory
    // rfree(queue->elements);

    // ORIGINAL DYNAMIC DEALLOCATION CODE (COMMENTED OUT):
    // rfree(queue->elements);

    queue->count = 0;
    queue->max_count= 0;
}

int deferqueue_smallest_seqnr(struct defer_queue * queue){

    int index = 0;

    // largest number possible
    unsigned long smallest = 0xFFFFFFFF;

    // naive implementation of search. performance shouldn't be an issue as the amount of messages in the queue is small
    for (unsigned int i = 0; i < queue->count; i++) {
        if(queue->elements[i].packet.sequence_number < smallest){
            smallest = queue->elements[i].packet.sequence_number;
            index = i;
        }
    }

    return index;
}

struct RastaRedundancyPacket deferqueue_get(struct defer_queue * queue, unsigned long seq_nr){
    int index = find_index(queue, seq_nr);

    if(index == -1){
        return (const struct RastaRedundancyPacket){ 0 };
    }

    struct RastaRedundancyPacket result = queue->elements[index].packet;

    // return element at index
    return result;
}

unsigned long deferqueue_get_ts(struct defer_queue * queue, unsigned long seq_nr){
    int index = find_index(queue, seq_nr);

    if(index == -1){
        return 0;
    }

    unsigned long result = queue->elements[index].received_timestamp;
    // return element at index
    return result;
}

void deferqueue_clear(struct defer_queue * queue){
    // just set count to 0, elements that are in the queue will be overridden
    queue->count = 0;
}
