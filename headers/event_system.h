//To change in c99, anonymous union is removed as well as POSIX flag is added
//Project_04

#ifndef EVENT_MANAGER_C_
#define EVENT_MANAGER_C_

#define _POSIX_C_SOURCE 200809L
#define ZER0_9 1000000000ULL
#define ZER0_6 1000000UL

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <_stdint.h>
#include <stdint.h>
#include <kernel/dpl/EventP.h>
/* lwIP core includes */
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/init.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip/netif.h"
#include <apps/tcpecho_raw/tcpecho_raw.h>
//------for udp-------
#include "lwip/udp.h"
//-------------------
//-------------
#include "lwip/icmp.h"
#include "lwip/ip.h"
#include "lwip/inet.h"
#include "lwip/pbuf.h"
#include "lwip/raw.h"
#include "lwip/timeouts.h"
#include "ti_enet_lwipif.h"

#include <networking/enet/utils/include/enet_apputils.h>
#include "ti_enet_config.h"

#define UDP_TARGET_PORT 12347    // Define the target port number

extern EventP_Object hEvent;
uint32_t App_receiveEvents(EventP_Object* pEvent);
void App_handleEvent(const uint32_t eventMask);
void App_sendUDPData(ip_addr_t *target_ip, uint16_t target_port, const char *data) ;

static uint64_t current_time_ns = 0;  // Simulated timer in nanoseconds
static uint64_t current_time_ns_clock = 0;  // Simulated timer in nanoseconds

//extern static uint64_t monotonic_time ;
// Flags to control timer based ISR
typedef struct
{
    volatile uint32_t timer_ev1;
    volatile uint32_t timer_ev2;
    volatile uint32_t timer_ev3;
    volatile uint32_t fd_ev1;
    volatile uint32_t fd_ev2;
}event_flags_t;

extern event_flags_t event_flags;

// Event type enumeration
typedef enum { TIMED_EVENT, FD_EVENT } EventType;

// Callback function type
typedef char (*Callback)(void *arg);

// Timed Event structure using nanoseconds
typedef struct {
    uint64_t interval_ns;      // Interval in nanoseconds
    uint64_t next_run_time_ns; // Next run time in nanoseconds
} TimedEvent;

// FD Event structure
typedef struct {
	unsigned int fd; //It is actually the associated port number
} FDEvent;

//Event meta information		//Backward compatibility
struct event_shared_information {
	Callback callback;
    void* carry_data;
    char enabled;
};

// Generic event node structure
typedef struct EventNode {
    EventType type;
    struct event_shared_information meta_information;

    union event_info_union {  // Named the union for C99 compatibility
        TimedEvent timed_event;
        FDEvent fd_event;
    } event_info;

    struct EventNode* next;
} EventNode;

typedef EventNode fd_event;  //Backward compatibility
typedef EventNode timed_event; //Backward compatibility

// Event list structure to hold the list of events
typedef struct EventList{
    EventNode* head;
} EventList;

typedef EventList event_container; //Backward compatibility

// Create a new event list
void init_event_container(EventList *list);

// Main loop to handle events

// Add a timed event to the event list
void add_timed_event(EventList* list, EventNode *node);

// Add an FD event to the event list
//int start_event_loop(EventList* list, struct rasta_handle* h);
void add_fd_event(EventList* list, EventNode *node);

// Remove an event from the list by callback or FD
void remove_event(EventList* list, Callback callback, unsigned int fd) ;
void remove_timed_event(EventList* list,EventNode* event);

// Reschedules the event to the current time + the event interval resulting in a delay of the event
void reschedule_event(EventNode* event);

// Enable Event
void enable_event(EventNode* event);
void enable_fd_event(EventNode* event);

// Disable Event
void disable_event(EventNode* event);
void disable_fd_event(EventNode* event);

// Reclaim all the memory on exit
void free_event_list(EventList* list);

#endif /* EVENT_MANAGER_C_ */
