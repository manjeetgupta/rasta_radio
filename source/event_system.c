#include "event_system.h"
#include "rasta_red_multiplexer.h"
#include "Project_cc.h"
#include "gps_rtc_comm.h"

//Code Integration started on 12Dec2024

//This is will be replaced by actual callbacks redefined for 2 channels.
// Process FD events

volatile int print_counter = 0;

void process_fd_events_dummy(EventList* list, struct udp_pcb* readfds,char recv_data[1000],unsigned int len,unsigned int port) {
    print_log("FD_EVENT triggered with FD = %d\n",port);

}


void process_fd_events(EventList* list, struct udp_pcb* readfds,char recv_data[1000],unsigned int len,unsigned int port,char *ip) {
    EventNode* node = list->head;
    while (node != NULL)
    {
        if (node->type == FD_EVENT && node->meta_information.enabled && node->event_info.fd_event.fd==readfds->local_port)
        {   struct receive_event_data* data = node->meta_information.carry_data;
        data->len=len;
        data->sender_port=port;
        memcpy(data->sender_ip,ip,15);
        data->ready_flag=1;

        memcpy(data->receive_buffer,recv_data,sizeof(data->receive_buffer));

        node->meta_information.carry_data = (void*)data;
        //node->meta_information.callback((void*)(intptr_t)node->event_info.fd_event.fd);

        //print_log("FD_EVENT executed with msg recv on port = %d\r\n",node->event_info.fd_event.fd);
        node->meta_information.callback(node->meta_information.carry_data);
        }
        node = node->next;
    }
}



// Callback function to handle received UDP data
void App_udpRecvCallback_e1(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        print_log("Ethernet_1:Received UDP packet from %s:%d\r\n", ipaddr_ntoa(addr), port);
        //char *arg_passed = (char*)arg;
        EventList* list = (EventList*)arg;
        // Process and display the received data
        char recv_data[256];
        memset(recv_data, 0, sizeof(recv_data));

        memcpy(recv_data, (const char *)p->payload, p->len);
        //strncpy(recv_data, (const char *)p->payload, p->len);
        print_log("Received Data:%s, local port = %d\r\n", recv_data,pcb->local_port);

        //We need to send receive buffer here
        process_fd_events(list, pcb,recv_data, p->len,port,ipaddr_ntoa(addr));

        pbuf_free(p);  // Free the buffer after processing
    }
}

// Callback function to handle received UDP data
void App_udpRecvCallback_e2(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p != NULL) {
        print_log("Ethernet_2:Received UDP packet from %s:%d\r\n", ipaddr_ntoa(addr), port);
        EventList* list = (EventList*)arg;
        // Process and display the received data
        char recv_data[256];
        memset(recv_data, 0, sizeof(recv_data));
        //strncpy(recv_data, (const char *)p->payload, p->len);
        memcpy(recv_data, (const char *)p->payload, p->len);
        print_log("Received Data:%s, local port = %d\r\n", recv_data,pcb->local_port);

        //We need to send receive buffer here
        process_fd_events(list, pcb,recv_data, p->len,port,ipaddr_ntoa(addr));

        pbuf_free(p);  // Free the buffer after processing
    }
}

// Get the current time in nanoseconds
int get_current_time_ns() {
    //    struct timespec ts;
    //    clock_gettime(CLOCK_MONOTONIC, &ts);
    //    return (uint64_t)ts.tv_sec * ZER0_9 + ts.tv_nsec;
    return 0;
}

// Create a new event list
void init_event_container(EventList* list) {
    list->head = NULL;
}

unsigned int check_overflow( uint64_t n1,uint64_t n2)
{

    if(n1>=1500000000-n2)
        return 0;
    else
        return 1;
}


void process_timed_events(struct rasta_handle* e)
{
    EventList* list = &e->events;
    EventNode* node = list->head;
    //current_time_ns += 1000000;  // Increment simulated time by 1 ms
    current_time_ns_clock = ClockP_getTimeUsec()/1000;

    while (node != NULL)
    {
        if (node->type == TIMED_EVENT && node->meta_information.enabled)
        {
          //  if (current_time_ns >=  node->event_info.timed_event.next_run_time_ns)
            if (current_time_ns_clock >=  node->event_info.timed_event.next_run_time_ns)
            {
                node->meta_information.callback(node->meta_information.carry_data);
                node->event_info.timed_event.next_run_time_ns +=  node->event_info.timed_event.interval_ns;
            }
        }
        node = node->next;
    }
}

void process_timed_events_old(void* e)
{
    // static uint32_t counter_ev1 =0;
    // counter_ev1++;

    EventList* list = (EventList*)e;
    EventNode* node = list->head;
    current_time_ns += 1000000;  // Increment simulated time by 1 ms
    //  current_time_ns += 100000000;  // Increment simulated time by 1 ms
    current_time_ns_clock = ClockP_getTimeUsec()/1000;

    //MAN_DEBUG_01JAN
//    if(current_time_ns>2000000000) // 2 billion
//    {
//        print_log("***********++++++++++RESEHECLUING ****\r\n\n");
//        current_time_ns = 0;
//        while (node != NULL)
//        {
//            if (node->type == TIMED_EVENT && node->meta_information.enabled)
//            {
//                reschedule_event(node);
//            }
//            node = node->next;
//        }
//    }

    while (node != NULL)
    {
        if (node->type == TIMED_EVENT && node->meta_information.enabled)
        {
          //  if (current_time_ns >=  node->event_info.timed_event.next_run_time_ns)
            if (current_time_ns_clock >=  node->event_info.timed_event.next_run_time_ns)
            {
                node->meta_information.callback(node->meta_information.carry_data);
                node->event_info.timed_event.next_run_time_ns +=  node->event_info.timed_event.interval_ns;
            }
        }
        //            else if (node->type == FD_EVENT && node->meta_information.enabled)
        //            {
        //                struct receive_event_data* data = node->meta_information.carry_data;
        //                if(data->ready_flag)
        //                {
        //                    print_log("^^^^^^ FD_EVENT executed with msg recv on port = %d\r\n",node->event_info.fd_event.fd);
        //                    data->ready_flag=0;
        //                    node->meta_information.carry_data = (void*)data;
        //                    node->meta_information.callback(node->meta_information.carry_data);
        //                }
        //            }

        node = node->next;
    }
}

void handle_timer_ev2(void* e) //Not used
{
    static uint32_t counter_ev2 =0;
    counter_ev2++;

    timed_event *io_events = (timed_event*)e;

    ip_addr_t target_ip;
    IP4_ADDR(&target_ip, 192, 168, 1, 34);
    const char *message = "THIS IS MANJEET";

    //io_events[1].meta_information.callback(io_events[1].meta_information.carry_data);
    print_log("Updating T_i %d, %s\r\n",counter_ev2,io_events[1].meta_information.carry_data);
    App_sendUDPData(&target_ip, UDP_TARGET_PORT, message);
}

/*
void handle_timer_ev3(void)
{
//    monotonic_time++;
//    print_log("monotonic_time =%d\r\n",monotonic_time);
}
 */


//int start_event_loop(EventList* list, struct rasta_handle* h)
int start_event_loop(struct rasta_handle* h)
{

        sys_check_timeouts(); //MAN_DEBUG_04JAN

        process_timed_events(h);

//        const uint32_t recvdEventsMask = ReceiveEvents(&hEvent);
//        if (recvdEventsMask != AppEventId_NONE)
//        {
//            HandleEvent(recvdEventsMask);
//        }
}


int start_event_loop_old(EventList* list, struct rasta_handle* h)
{

    while (1)
    {
        //sys_check_timeouts(); //MAN_DEBUG_04JAN

        if(event_flags.timer_ev1)
        {
            event_flags.timer_ev1 =0;
            process_timed_events(list);
        }

        // changed for rasta MAN_31MAY_DISABLE_CAN
        //print_counter = print_counter + 1;
//        ClockP_usleep(50);
//        if(!(monotonic_time % 5000))
//        {
//            is_rasta_packet_rcvd= true;
//            //print_counter = 0;
//        }

//        if(is_rasta_packet_rcvd){
//            print_log("\n#########DATA SENT --> vcc recvd msg via rasta.\n");
//            is_rasta_packet_rcvd = false;
//            struct RastaMessageData app_messages;
//
//            allocateRastaMessageData(&app_messages, 1,52);
//            app_messages.data_array[0].length = canRastaMsgRecvdLen;
//            rmemcpy(app_messages.data_array[0].bytes, rastaDataSkavach, canRastaMsgRecvdLen);
//#ifdef skavach_1
//            sr_send(h, ID_R, app_messages);
//#elif defined(skavach_2)
//            sr_send(h, ID_S1, app_messages);
//#endif
////            print_log("\nREAL Rasta data\r\n");
////            for(int i=0;i<canRastaMsgRecvdLen;i++)
////            {
////                print_log("%02X ", rastaDataSkavach[i]);
////                if(((i+1)%16)==0) print_log("\n");
////            }
//            print_log("\nREAL Rasta data length : %d\n", canRastaMsgRecvdLen);
//
//            freeRastaMessageData(&app_messages);
//        }

        if(is_rasta_packet_rcvd){
            print_log("\n#########DATA SENT --> vcc recvd msg via rasta.\n");
            is_rasta_packet_rcvd = false;
            struct RastaMessageData app_messages;

            canRastaMsgRecvdLen = 200;

            //rastaDataSkavach = "THIS IS A DUMMY MESSAGE FROM LINGAMPALLI !!!";

            allocateRastaMessageData(&app_messages, 1,52);
            app_messages.data_array[0].length = canRastaMsgRecvdLen;
            rmemcpy(app_messages.data_array[0].bytes, "THIS IS A DUMMY MESSAGE FROM HYDERABAD !!!", canRastaMsgRecvdLen);
#ifdef skavach_1
            sr_send(h, ID_R, app_messages);
#elif defined(skavach_3)
            sr_send(h, ID_R, app_messages);
#elif defined(skavach_2)
            sr_send(h, ID_S1, app_messages);
#endif
//            print_log("\nREAL Rasta data\r\n");
//            for(int i=0;i<canRastaMsgRecvdLen;i++)
//            {
//                print_log("%02X ", rastaDataSkavach[i]);
//                if(((i+1)%16)==0) print_log("\n");
//            }
            print_log("\nREAL Rasta data length : %d\n", canRastaMsgRecvdLen);

            freeRastaMessageData(&app_messages);

//            //DUMMY UDP MESSAGE (10.06.2025)
//            ip_addr_t target_ip;
//            IP4_ADDR(&target_ip, 192, 168, 25, 83);
//            const char *message = "THIS IS MANJEET";
//
//            //App_sendUDPData(&target_ip, UDP_TARGET_PORT, message);//Uncommented-Abhishek
        }


//#ifdef RASTA_ENABLED
        //mcan_checks_fifo_rasta();
//#endif
        //mcan_checks_fifo();
        //process_radio_2_com_port();

        // for testing
//        if(CTR_MS % 1000 == 0)
//        {
//            unsigned long id=0;
//            #ifdef skavach_1
//                       id = ID_R;
//            #elif defined(skavach_2)
//                       id = ID_S1;
//    #endif
//            struct rasta_connection *connection = rastalist_getConnectionByRemote(&h->connections,id);
//            if(connection->current_state == RASTA_CONNECTION_UP)
//            {
//                struct RastaMessageData app_messages;
//                allocateRastaMessageData(&app_messages, 1,52);
//                // addRastaString_can(&app_messages, 0, rastaDataSkavach, canRastaMsgRecvdLen);
//                app_messages.data_array[0].length = 100;
//                canRastaMsgRecvdLen = 100;
//                rmemcpy(app_messages.data_array[0].bytes, "DUMMY DATA FROM LINGAMPALLI", canRastaMsgRecvdLen);
//    #ifdef skavach_1
//                sr_send(h, ID_R, app_messages);
//    #elif defined(skavach_2)
//                sr_send(h, ID_S1, app_messages);
//    #endif
//                sr_send(h, id, app_messages);

                //                print_log("\nDUMMY Rasta data\r\n");
                //                for(int i=0;i<canRastaMsgRecvdLen;i++)
                //                {
                //                 print_log("%02X \r\n", rastaDataSkavach[i]);
                //                 if(((i+1)%16)==0) print_log("\n");
                //                }
//                print_log("\nDUMMY SEND || length : %d\r\n", canRastaMsgRecvdLen);
//
//                freeRastaMessageData(&app_messages);
//            }
//        }

//        if(is_gps_pulse_recvd)
//        {
//            current_time_prints();
//            is_gps_pulse_recvd = false;
//            rtc_time_at_2sec = time_msec;
//
//            if(GPS_SEC_CTR % 4 ==0){
//                update_gps_time_to_other_units();
//            }
//
//            reset_CTR_S_1_min();
//
//            CTR_MS=0;
//        }

#ifdef GPS_ENABLED
        check_gps_comm();
#endif

        //////////////// END //////////////

        /*    if(event_flags.timer_ev2)
                {
                     event_flags.timer_ev2 =0;
                     //handle_timer_ev2(io_events);
                }*/


        /*   if(event_flags.timer_ev3)
                {
                      event_flags.timer_ev3 =0;
                      handle_timer_ev3();
                }*/

        const uint32_t recvdEventsMask = ReceiveEvents(&hEvent);

        if (recvdEventsMask != AppEventId_NONE)
        {
            HandleEvent(recvdEventsMask);
        }
        //ClockP_usleep(50);

        //For testing
        //if(!(monotonic_time % 2000)){send_dummy_udp();}//Commented-Abhishek

    }
}

// Reclaim all the memory on exit
void free_event_list(EventList* list)
{
    EventNode* current = list->head;
    while (current != NULL) {
        EventNode* next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

// Add a timed event with a nanosecond interval
void add_timed_event(EventList* list, EventNode *node)
{
    //node->event_info.timed_event.next_run_time_ns = 1;
    print_log("Added TIMED_EVENT : interval_millisec= %llu \r\n",node->event_info.timed_event.interval_ns);
//    print_log("Added TIMED_EVENT : current_time_ns= %lu\r\n",current_time_ns);
    print_log("Added TIMED_EVENT : current_time_ns= %llu\r\n",ClockP_getTimeUsec()/1000);
   // print_log("Added TIMED_EVENT : next_run_time_ns= %lu\r\n",node->event_info.timed_event.next_run_time_ns);


//    node->event_info.timed_event.next_run_time_ns = current_time_ns + node->event_info.timed_event.interval_ns;
    node->event_info.timed_event.next_run_time_ns = ClockP_getTimeUsec()/1000 + node->event_info.timed_event.interval_ns;
    print_log("Added TIMED_EVENT--after : next_run_time_ns= %llu\r\n\n",node->event_info.timed_event.next_run_time_ns);
    node->next = list->head;
    list->head = node;
    //print_log("Added TIMED_EVENT : interval_millisec= %lu, next_run_time_ns = %lu, current_time_ns = %lu\r\n",node->event_info.timed_event.interval_ns,node->event_info.timed_event.next_run_time_ns,current_time_ns);
}

// Add an FD event to the event list
void add_fd_event(EventList* list, EventNode *node)
{
    node->next = list->head;
    list->head = node;
    print_log("Added FD_EVENT : %d\r\n",node->event_info.fd_event.fd);
}

void remove_timed_event(EventList* list,timed_event* event) {
    remove_event(list,event->meta_information.callback,0);
}

// Remove an event from the list by callback or FD
void remove_event(EventList* list, Callback callback, unsigned int fd) {
    EventNode* current = list->head;
    EventNode* prev = NULL;

    while (current != NULL)
    {
        if ((current->type == TIMED_EVENT && current->meta_information.callback == callback) ||
                (current->type == FD_EVENT && current->event_info.fd_event.fd == fd))
        {
            print_log("Event Found: Going to delete\r\n");
            if (prev == NULL)
            {
                list->head = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            current->next = NULL;
            //free(current);
            current = NULL;
            return;
        }
        prev = current;
        current = current->next;
    }
}

//MAN_DEBUG_01JAN
// Reschedules the event to the current time + the event interval resulting in a delay of the event
void reschedule_event(EventNode* event)
{
   // event->event_info.timed_event.next_run_time_ns = get_current_time_ns()+ event->event_info.timed_event.interval_ns ;
    event->event_info.timed_event.next_run_time_ns = ClockP_getTimeUsec()/1000+ event->event_info.timed_event.interval_ns ;
/*//MAN_MAY_31

//    if(check_overflow(current_time_ns,event->event_info.timed_event.interval_ns))
    if(check_overflow(cur_timestamp()*1000,event->event_info.timed_event.interval_ns))
    {
//        event->event_info.timed_event.next_run_time_ns = current_time_ns + event->event_info.timed_event.interval_ns ;
        event->event_info.timed_event.next_run_time_ns = ClockP_getTimeUsec()/1000 + event->event_info.timed_event.interval_ns ;
    }
    else
    {
        event->event_info.timed_event.next_run_time_ns =  event->event_info.timed_event.interval_ns;
    }

*/
}

// Enable Event
void enable_event(EventNode* event) {
    event->meta_information.enabled = 1;
    if(event->type == TIMED_EVENT)
    {
        reschedule_event(event);
    }
}

void enable_fd_event(EventNode* event) {
    enable_event(event);
}

// Disable Event
void disable_event(EventNode* event) {
    event->meta_information.enabled = 0;
}

void disable_fd_event(EventNode* event) {
    disable_event(event);
}

