/*
 * Project_CC.c
 *  Created on: 20-Dec-2024
 *      Author: sehrakk
 */
#include <project_cc.h>

#ifdef RASTA_ENABLED
//RASTA Includes
#include <event_system.h>
#include <rasta_new.h>
#include "rmemory.h"
#include "app_context.h"
//#include <mcheck.h>

#define UDP_LOCAL_PORT_RASTA_1 12345     // Define the local port number for the UDP socket
#define UDP_LOCAL_PORT_RASTA_2 12346     // Define the local port number for the UDP socket

typedef struct {
    uint32_t arg1;
    uint32_t arg2;
} MyArgs;

MyArgs my_args = {0};
//static struct udp_pcb *udp_pcb_rasta_1,*udp_pcb_rasta_2;

volatile unsigned int network_sending = 0;
//struct rasta_handle r_dle;


//struct app_context {
//    struct rasta_handle r_dle;
//};
static struct app_context app = {0};
//static uint64_t current_time_ns_clock = 0;
static timed_event connect_req;
    static struct connect_event_data data;
#endif

//############################################ MAIN FUNC ############################################
int appMain(void *args)
{
    uint32_t send_flag =0;
    uint32_t fif2_waitflag = 30;
    uint32_t f0_waitflag = 30;
    uint32_t rts_high_waitflag = 0;
    uint32_t rts_low_waitflag = 0;
    uint32_t toggle_waitflag = 0;

    //    print_log("\n###### INITIALIZING DRIVERS ###### \n");
    uart_init();
    LWIPConfigInit();
    gpio_init();
    rti_init();
    gpio_input_interrupt_main(NULL);
    crcInit();
    SPI_Init();


    cc_init();
    //lib_MCAN_init(CAN_MODE_FIFO); // revert back

//#ifdef RASTA_ENABLED
//    my_args.arg1 = 1;
//    my_args.arg2 = 2;
//#endif

    print_log("\n############## CC_SCU init completed ##############\n");
    print_log("\n###### 20250520 ######\n");

#ifdef GPS_ENABLED
    print_log("\nWaiting for GPS TIME-SYNC");
    while(!is_time_rcvd_from_gps){
        check_gps_comm();
        ethernet_checks();
    }
    update_gps_time_to_other_units();
#else
    is_time_rcvd_from_gps = true;
#endif

    prngState.seed = (uint32_t)(GPS_SEC_CTR + (time_usec%1000000) );

#ifdef KMS_KEY_ENABLED
    print_log("\nWaiting for keys from DL");
    while(!is_valid_key_set_available){
        req_key_from_dl();
        long long unsigned int t1, t2;
        t1 = time_msec;
        t2 = time_msec;
        while(t2-t1<3000){
            t2 = time_msec;
            check_gps_comm();
            mcan_checks_fifo();
            if(is_valid_key_set_available) break;
        }
        update_gps_time_to_other_units();
    }
#else
    is_valid_key_set_available = TRUE;
#endif

    //init_loco(); // revert back
    initQueue((QUEUE*)&q_stn_reg_pkt);
    initQueue((QUEUE*)&q_ss_pkt);
    //    radio_1_active = true;
    //    is_radio_available = true;
    memset((void *)radio_com_buffer, 0, sizeof(radio_com_buffer));
    radio_buff_offset = 0;

    //        check_radio_available();
    //    if(is_radio_available) radio_init_setup();
    //    //    else print_log("\nRadio Not Available, switching to LAN");

    //radio_init_setup(); // revert back
    sleep_ms_counter_based(10);
    check_radio_freq();

    radio_set_dtr_high();
    radio_2_set_rts_low();

    uint8_t cur_slot = 0;

//    char gps_send_command[20] = "$PAIR752,4,100*39\r\n"; // revert back
//    serial_send(eGPS_1, (const unsigned char*)gps_send_command, strlen(gps_send_command));
//    print_log("\nGPS PULSE cmd sent with len: %d", strlen(gps_send_command));


#ifdef RASTA_ENABLED
    //static struct rasta_handle r_dle;
    //rasta_init(&r_dle);
    rasta_init();



#endif

    cc_init_complete = TRUE;
    while (1)
    {
        // revert all commented lines
//        if(toggle_waitflag*200 <= time_msec) {
//            if(is_active_gps_0) GPIO_pinToggle(GPS0_COM_LED_A_PIN, GPS0_COM_LED_A_BASE_ADDR);
//            if(is_active_gps_1) GPIO_pinToggle(GPS1_COM_LED_A_PIN, GPS1_COM_LED_A_BASE_ADDR);
//            if(is_active_can_0) GPIO_pinToggle(LINK0_HLTH_A_PIN, LINK0_HLTH_A_BASE_ADDR);
//            if(is_active_can_1) GPIO_pinToggle(LINK1_HLTH_A_PIN, LINK1_HLTH_A_BASE_ADDR);
//            if(is_active_radio_0) GPIO_pinToggle(RAD0_COM_A_PIN, RAD0_COM_A_BASE_ADDR);
//            if(is_active_radio_1) GPIO_pinToggle(RAD1_COM_A_PIN, RAD1_COM_A_BASE_ADDR);
//            if(is_active_link_bw_controller) GPIO_pinToggle(INTR_CPU_COM_A_PIN, INTR_CPU_COM_A_BASE_ADDR);
//
//            if(
//                    (is_active_gps_0 || is_active_gps_1)
//                    &&(is_active_can_0 || is_active_can_1)
//                    &&(is_active_radio_0 || is_active_radio_1)
//            ){
//                is_controller_healty = true;
//                GPIO_pinToggle(CPU_A_HLTH_PIN, CPU_A_HLTH_BASE_ADDR);
//            }
//
//            toggle_waitflag += 1;
//        }
//
//        process_radio_2_com_port();
//        mcan_checks_fifo();
//        cur_slot = CTR_MS/50;
//        if(send_flag == 1)
//        {
//
//            if(
//                    loco_slot[cur_slot].flag == 1
//                    && loco_slot[cur_slot].data_available == 1
//                    && CTR_MS >= (cur_slot*50)
//                    && is_radio_f0_reset == FALSE
//            )
//            {
//                loco_slot[cur_slot].data_available = 0;
//                radio_send_buffer_with_rts(
//                        (const char*)locoPktRegPairs[loco_slot[cur_slot].loco_index].data,
//                        locoPktRegPairs[loco_slot[cur_slot].loco_index].len
//                );
//                print_log("\n>>> RAD OUT :: Reg Pkt with len [%d] SENT, T:%lld", locoPktRegPairs[loco_slot[cur_slot].loco_index].len, CTR_MS);
//                rts_high_waitflag = CTR_MS + 20;
//            }
//
//            if(
//                    slot_f0[cur_slot].flag == 1
//                    && slot_f0[cur_slot].len > 0
//                    && CTR_MS >= (cur_slot*50)
//                    && is_radio_f0_reset == TRUE
//            )
//            {
//                uint8_t _len = 0;
//
//                if(slot_f0[cur_slot].msg_type == MSG_ID_ACCESS_AUTH_PKT){
//                    radio_send_buffer_with_rts(
//                            (const char*)locoPktsAccAuth[slot_f0[cur_slot].index].data,
//                            locoPktsAccAuth[slot_f0[cur_slot].index].len
//                    );
//                    _len = locoPktsAccAuth[slot_f0[cur_slot].index].len;
//
//                    locoPktsAccAuth[slot_f0[cur_slot].index].slot_used = 0; // Mark as unused
//                    locoPktsAccAuth[slot_f0[cur_slot].index].len = 0;
//
//                }
//                else{
//                    radio_send_buffer_with_rts(
//                            (const char*)locoPktEmer,
//                            locoPktEmerLen
//                    );
//                    prev_sos_slot = -1;
//                    _len = locoPktEmerLen;
//                    locoPktEmerLen = 0;
//                }
//                slot_f0[cur_slot].len = 0;
//                slot_f0[cur_slot].flag = 0;
//
//                rts_high_waitflag = CTR_MS + 20;
//                print_log("\n>>> RAD OUT :: RAD Pkt [%d] with len [%d] SENT, T:%lld", slot_f0[cur_slot].msg_type, _len, CTR_MS);
//                if(slot_f0[cur_slot].msg_type == MSG_ID_ACCESS_AUTH_PKT){
//                    print_log(" loco_id: %d\n", locoPktsAccAuth[slot_f0[cur_slot].index].loco_id);
//                    memset(
//                            (void*)locoPktsAccAuth[slot_f0[cur_slot].index].data,
//                            0U,
//                            MAX_LEN_RADIO_PKT_ACC_AUTH
//                    );
//                }
//                //                else{
//                //                    memset(
//                //                            (void*)locoPktEmer,
//                //                            0U,
//                //                            MAX_LEN_RADIO_PKT_EMER
//                //                    );
//                //                }
//            }
//        }
//
//        if (rts_high_waitflag > 0 && rts_high_waitflag <= CTR_MS)
//        {
//            rts_high_waitflag = 0;
//            radio_2_set_rts_high();
//            //            print_log("\n-----rts_high_waitflag");
//            //            print_log("T:%d, C_MS:%lld", GPS_SEC_CTR, CTR_MS);
//            rts_low_waitflag = CTR_MS +20;
//        }
//        if(rts_low_waitflag > 0 && rts_low_waitflag <= CTR_MS)
//        {
//            rts_low_waitflag =0;
//            radio_2_set_rts_low();
//            //            print_log("\n-----rts_low_waitflag");
//            //            print_log("T:%d, C_MS:%lld", GPS_SEC_CTR, CTR_MS);
//        }
//
//        if(f0_waitflag > 0 && f0_waitflag <= CTR_MS)
//        {
//            f0_waitflag = 0;
//            send_flag = 1;
//            //            stn_acc_auth_loco1_send_time = 400;
//        }
//
//        if(fif2_waitflag > 0 && fif2_waitflag <= CTR_MS)
//        {
//            fif2_waitflag = 0;
//            send_flag = 1;
//        }
//
//        if(is_gps_pulse_recvd)
//        {
//            CTR_MS = 0;
//            send_flag = 0;
//            rtc_time_at_2sec = time_msec;
//            is_gps_pulse_recvd = FALSE;
//
//            //            if(GPS_SEC_CTR % GPS_UPDATE_TIME_IN_SECONDS == 0){
//            //                update_gps_time_to_other_units();
//            //            }
//            current_time_prints();
//
//            if (GPS_SEC_CTR %2 == 1)
//            {
//                rts_low_waitflag =0;
//                radio_2_set_rts_low();
//
//                radio_f0_reset();
//                f0_waitflag = CTR_MS + f0_waitflag;
//
//                if(GPS_SEC_CTR % 5 == 0){
//                    check_radios_available();
//                }
//                //// status packet
//                if(GPS_SEC_CTR % 15 == 0){
//                    TRANS_CAN_MSG_WD_HDR(
//                            VCC,
//                            MSG_ID_SCU_VCC_SYS_INFO_STATUS,
//                            sizeof(system_status),
//                            (unsigned char*)&system_status,
//                            "System Status Sent to VCC"
//                    );
//                }
//            }
//            else if(GPS_SEC_CTR %2 == 0)
//            {
//                rts_low_waitflag =0;
//                radio_2_set_rts_low();
//
//                radio_set_tx_rx_freq_1();
//                fif2_waitflag = CTR_MS+fif2_waitflag;
//                update_gps_time_to_other_units();
//            }
//
//            ///// verify this after integration
//            //            reset_CTR_S_1_min();
//            //check_radio_available();
//        }
//        //        if(is_radio_available)
//        //        radio_communcation();
        //ethernet_checks();
//#ifdef GPS_ENABLED
//        check_gps_comm();
//#endif
//        if(GPS_SEC_CTR >= (timeout_curr_key_expiry - MAX_TIME_KEY_UPDATE_TO_VCC) ){
//            check_key_set_expiry();
//        }

#ifdef RASTA_ENABLED
//        start_event_loop(getRastaHandlePacket());
//        start_event_loop(&app.r_dle.events,&app.r_dle);
//          start_event_loop(&app.r_dle);
        {
//        //void process_timed_events(struct rasta_handle* e)
            {
                // Safety check: ensure RASTA is initialized
                if (app.r_dle.receive_handle == NULL || app.r_dle.send_handle == NULL) {
                    // RASTA not yet initialized, skip event processing
                    continue;
                }
                
                EventList* list = &app.r_dle.events;
                
                // Safety check: ensure the event list is properly initialized
                if (list == NULL) {
                    print_log("ERROR: Event list is NULL\n");
                    continue;
                }
                
                EventNode* node = list->head;
                current_time_ns_clock = ClockP_getTimeUsec()/1000;

                while (node != NULL)
                {
                    // Safety check: validate node structure
                    if (node->meta_information.callback == NULL) {
                        print_log("ERROR: Invalid event node with NULL callback\n");
                        node = node->next;
                        continue;
                    }
                    
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

//
//               const uint32_t recvdEventsMask = ReceiveEvents(&hEvent); // Corruption or something here also MAN_21JUNE
//               if (recvdEventsMask != AppEventId_NONE)
//               {
//                   HandleEvent(recvdEventsMask);
//               }
        }


#endif
//
    }
    //     App_shutdownNetworkStack();
    //    EventP_destruct(&hEvent);
    //     EnetApp_driverDeInit();
    return 0;
}


void cc_init(){

#ifdef STATION_TELAPUR
    IP4_ADDR(&ip_other_s_kavach, 192, 168, 25, 10);
#else
    IP4_ADDR(&ip_other_s_kavach, 192, 168, 25, 83);
#endif

    IP4_ADDR(&ip_log, 192, 168, 25, 101);

    csci_add_map[SPU] = ip_s_kavach;
    csci_add_map[SDLU] = ip_dl;

    IP4_ADDR(&ip_l_kavach1, 192, 168, 25, 89);
    IP4_ADDR(&ip_l_kavach2, 192, 168, 25, 90);
    IP4_ADDR(&ip_l_kavach3, 192, 168, 25, 85);

#ifdef MODE_RADIO_LAN
    ip_l_kavach_map[1] = ip_l_kavach1;
    ip_l_kavach_map[2] = ip_l_kavach2;
    ip_l_kavach_map[3] = ip_l_kavach3;
#endif

    memset((void*)&date_time, 0, sizeof(date_time));

    set_adjacent_stations();

    is_enet_up = FALSE;
    radio_setup_buff_offset = 0;

    system_status.active_radio_no = 2;
    system_status.health_status_radio1 = 0;
    system_status.health_status_radio2 = 1;
    system_status.radio1_input_supply = 0;
    system_status.radio2_input_supply = 12;
    system_status.radio1_temp = 0;
    system_status.radio2_temp = 31;
    system_status.radio1_pa_temp = 0;
    system_status.radio2_pa_temp = 37;
    system_status.radio1_pa_supply = 0;
    system_status.radio2_pa_supply = 4;
    system_status.radio1_tx_pa_current = 0;
    system_status.radio2_tx_pa_current = 0;
    system_status.radio1_reverse_power = 0;
    system_status.radio2_forward_power = 6;
    system_status.radio1_rx_packet_count = 0;
    system_status.radio2_rx_packet_count = 0;

    system_status.active_gps_no = 1;
    system_status.gps1_view = 1;
    system_status.gps2_view = 0;
    system_status.gps1_seconds = 0;
    system_status.gps2_seconds = 0;
    system_status.gps1_satellite_in_view = 0;
    system_status.gps2_satellite_in_view = 0;
    system_status.gps1_cno_max = 0;
    system_status.gps2_cno_max = 0;

    App_initUDP1();

}

void set_adjacent_stations(){
    uint16_t adj_kav_id;
    ip_addr_t adj_kav_ip;;

#ifdef STATION_TELAPUR
    adj_kav_id = 501;
    IP4_ADDR(&adj_kav_ip, 192, 168, 25, 66);
#elif defined(STATION_LINGAMPALLY)
    adj_kav_id = 527;
    IP4_ADDR(&adj_kav_ip, 192, 168, 25, 35);
#else
    adj_kav_id = 527;
    IP4_ADDR(&adj_kav_ip, 192, 168, 25, 35);
#endif

    adj_kav_info[0].station_id = adj_kav_id;
    adj_kav_info[0].ip_addr = adj_kav_ip;

    // dummy data, as only two stations are avilable
    adj_kav_id = 555;
    IP4_ADDR(&adj_kav_ip, 192, 168, 25, 55);
    adj_kav_info[1].station_id = adj_kav_id;
    adj_kav_info[1].ip_addr = adj_kav_ip;
}

//// 1 sec timer
void rtievent0_1s(void)
{
    CTR_S++;
}

// 1 sec compare ctr
void rtievent1_1s(void)
{
    if(CTR_S%2==0)
    {
        counterflag=1;
    }
}

// 1 ms ctrs
void rtievent_1ms(void)
{
    CTR_MS++;

#ifdef RASTA_ENABLED
    event_flags.timer_ev1 =1; // rasta timer
    monotonic_time++; // for rasta
#endif
}

// 50ms timer for radio com buffer
void rtievent_50ms(void)
{
}

// Returns the starting index of [0xF1, 0xA5, 0xC3] in data[], or -1 if not found
int find_f1a5c3_pattern(const uint8_t *data, size_t data_len) {
    if (data_len < 3) {
        return -1;
    }

    for (size_t i = 0; i <= data_len - 3; ++i) {
        if (data[i] == 0xF1 && data[i + 1] == 0xA5 && data[i + 2] == 0xC3) {
            return (int)i;
        }
    }

    return -1;
}

int find_f2a5c3_pattern(const uint8_t *data, size_t data_len) {
    if (data_len < 3) {
        return -1;
    }

    for (size_t i = 0; i <= data_len - 3; ++i) {
        if (data[i] == 0xF2 && data[i + 1] == 0xA5 && data[i + 2] == 0xC3) {
            return (int)i;
        }
    }

    return -1;
}

void process_radio_2_com_port(){

    if(radio_buff_offset >= 5){
        uint16_t mask_msg_id = 0xF000;
        uint16_t mask_msg_len7bit = 0x0FE0;
        uint16_t mask_msg_len10bit = 0x0FFC;

        unsigned char matched_str[1500] = {};
        uint16_t pkt_type = 0;
        uint16_t pkt_len = 0;

        int16_t start_pos = find_f1a5c3_pattern((const unsigned char*)radio_com_buffer,radio_buff_offset);
        if (start_pos < 0) {
            return;
        }

        if(radio_buff_offset < start_pos+3 +2)
            return;

        uint16_t two_byte=0;
        uint8_t buff[2];
        buff[0] = radio_com_buffer[start_pos+3+1];
        buff[1] = radio_com_buffer[start_pos+3];
        memcpy((void*)&two_byte, (void*)buff, 2);

        pkt_type = (two_byte & mask_msg_id)>>12;
        if(pkt_type == MSG_ID_STN_ONB_REG_PKT){
            pkt_len = (two_byte & mask_msg_len10bit)>>2;
        }
        else if(pkt_type >= MSG_ID_ONB_STN_REG_PKT && pkt_type <= MSG_ID_ONB_ACC_REQ_PKT)
        {
            pkt_len = (two_byte & mask_msg_len7bit) >>5;
        }

        uint16_t end_pos= start_pos + 3 + pkt_len;
        if(end_pos <= radio_buff_offset)
        {
            //            print_log("\nBuf offset: %d ", radio_buff_offset);
            print_log("\n<<< RAD IN :: _type:%d, _len:%d, offset:%d, T:%lld", pkt_type, pkt_len, radio_buff_offset, CTR_MS);
            //            print_log("\nData:--\n ");
            //            for(uint16_t i=start_pos;i<end_pos;i++)
            //            {
            //                printf("[%02X]", radio_com_buffer[i]);
            //            }


            /// filter according to STN/LOCO

            memcpy((void*)matched_str, (void*)(radio_com_buffer + start_pos + 3), pkt_len);
            TRANS_CAN_MSG_WD_HDR(VCC, pkt_type, pkt_len, matched_str, "<RAD to CAN>");

            memset((void*)radio_com_buffer, 0, end_pos);
            memcpy((void*)(radio_com_buffer), (void*)(radio_com_buffer + end_pos), sizeof(radio_com_buffer) - end_pos);
            radio_buff_offset  = radio_buff_offset - (end_pos);
        }
    }
}

void req_key_from_dl(){
    TRANS_CAN_MSG_WD_HDR(SDLU, MSG_ID_COMM_DL_KEY_REQ, 0, NULL, "Key Req from DL\n");
}

int8_t print_log(const char *formatString, ...){
    int16_t _len = 0, _kav_id;
    char buffer[400] = {};  // Buffer to hold the formatted string
    uint8_t send_buffer[512] = {};
    _kav_id = self_kavach_id;

    MESSAGE_HEADER _log_header;
    _log_header.dest_id = 101;
    _log_header.src_id  = CC;
    _log_header.msg_id  = 151;
    _log_header.msg_len = 0;

    va_list args;
    va_start(args, formatString);
    _len = vsnprintf(buffer, sizeof(buffer), formatString, args);
    va_end(args);

    _log_header.msg_len = _len + sizeof(_log_header) + sizeof(uint16_t);

    //    serial_send(eRADIO_1_SETUP, buffer, _len);
    send_data_to_uart0((const unsigned char*)buffer, _len);
    //    printf("%s", buffer);

    if(is_active_lan_0 || is_active_lan_1){
        memcpy(send_buffer, &_log_header, sizeof(_log_header));
        memcpy(send_buffer + sizeof(_log_header), &_kav_id, sizeof(uint16_t));
        memcpy(send_buffer + sizeof(_log_header) + sizeof(uint16_t), buffer, _len);

        err_t udp_err = UDP_Send((const char *)send_buffer, &ip_log, 50002, _log_header.msg_len);

        if (udp_err < 0) {
            //            printf("\nUDP log failed");
            return udp_err;
        }
    }
    return 1;
}


int rti_init(void)
{
    /* Start the RTI counter */
    (void)RTI_counterEnable(CONFIG_RTI0_BASE_ADDR, RTI_TMR_CNT_BLK_INDEX_1);
    (void)RTI_counterEnable(CONFIG_RTI0_BASE_ADDR, RTI_TMR_CNT_BLK_INDEX_0);
    return 1;
}

#ifndef NO_SYS
static void App_printCpuLoad()
{
    static uint32_t startTime_ms = 0;
    const  uint32_t currTime_ms  = ClockP_getTimeUsec()/1000;
    const  uint32_t printInterval_ms = 5000;

    if (startTime_ms == 0)
    {
        startTime_ms = currTime_ms;
    }
    else if ( (currTime_ms - startTime_ms) > printInterval_ms )
    {
        const uint32_t cpuLoad = TaskP_loadGetTotalCpuLoad();

        DebugP_log(" %6d.%3ds : CPU load = %3d.%02d %%\r\n",
                   currTime_ms/1000, currTime_ms%1000,
                   cpuLoad/100, cpuLoad%100 );

        startTime_ms = currTime_ms;
        TaskP_loadResetAll();
    }
    return;
}
#endif


//RASTA Loop ---- START

#ifdef RASTA_ENABLED
// struct for the ethernet netif
struct netif *g_pNetif[ENET_SYSCFG_NETIF_COUNT];
EventP_Object hEvent;
ip4_addr_t ipaddr, netmask, gw;


//Timer to keep track of monotonic time
unsigned long monotonic_time =0;
//struct RastaIPData toServer[2];

static struct udp_pcb *udp_pcb;  // Pointer to UDP protocol control block
ip_addr_t target_ip;              // Define the target IP

volatile uint32_t gLedState, gBlinkCount;
uint32_t gpioBaseAddr, pinNum;
int k=0;



event_flags_t event_flags={0};

void rtiEvent0_rasta(void) //timer_ev1_ISR
{
    //if(network_sending) return;
    event_flags.timer_ev1 =1;
}

void rtiEvent1_rasta(void) //timer_ev2_ISR
{
    //event_flags.timer_ev2 =1;
    monotonic_time++;
}

uint32_t gTimerCount = 0;

void myISR(void *args)
{
    event_flags.timer_ev3 =1;

    my_args = *((MyArgs*)args);
    my_args.arg1++;
    my_args.arg2++;
}

//-------------new main content-----------------
void addRastaString(struct RastaMessageData * data, int pos, char * str) {
    size_t size = strlen(str) + 1;

    struct RastaByteArray msg ;
    allocateRastaByteArray(&msg, size);
    rmemcpy(msg.bytes, str, size);

    data->data_array[pos] = msg;
}


void addRastaString_can(struct RastaMessageData * data, int pos, char * str,u16 len) {
    size_t size = len;

    struct RastaByteArray msg ;
    allocateRastaByteArray(&msg, size);
    rmemcpy(msg.bytes, str, size);

    data->data_array[pos] = msg;
}


int client1 = 1;
int client2 = 1;
int sendflag =1 ;
void onConnectionStateChange(struct rasta_notification_result *result)
{
    print_log("Connectionstate change (remote: %d)\r\n", result->connection.remote_id);
    switch (result->connection.current_state)
    {
    case RASTA_CONNECTION_CLOSED:
        print_log("\nCONNECTION_CLOSED \r\n\n");
        break;
    case RASTA_CONNECTION_START:
        print_log("\nCONNECTION_START \r\n\n");
        break;
    case RASTA_CONNECTION_DOWN:
        print_log("\nCONNECTION_DOWN v\n\n");
        break;
    case RASTA_CONNECTION_UP:
        print_log("\nCONNECTION_UP... \r\n\n");
        //send data to server
        print_log("DATA CHECK : %d,%d \r\n\n",result->connection.my_id,sendflag);
        if ((result->connection.my_id == ID_S1)&& (sendflag==1)) { //Client 1
            struct RastaMessageData messageData1;
            char temp[40];
            int no_of_msg = 1;
            allocateRastaMessageData(&messageData1, 1,50);

            // messageData1.data_array[0] = msg1;
            // messageData1.data_array[1] = msg2;


            for ( int i = 0;i<no_of_msg;i++)
            {
                print_log(temp,"Message from Sender 1, MSG_Count = %d",i);
                //addRastaString(&messageData1, 0, temp);
                rmemcpy(messageData1.data_array[i].bytes, temp, 40);
                sr_send(result->handle,ID_R, messageData1);
            }
            sendflag=0;
            freeRastaMessageData(&messageData1);
        } else if (result->connection.my_id == ID_S2) { //Client 2
            struct RastaMessageData messageData1;
            allocateRastaMessageData(&messageData1, 1,51);

            // messageData1.data_array[0] = msg1;
            // messageData1.data_array[1] = msg2;
            //addRastaString(&messageData1, 0, "Message from Sender 2");   //TO BE CHANGED AS PER S1

            //rmemcpy(messageData1.data_array[i].bytes, temp, 40);

            //send data to server
            sr_send(result->handle,ID_R, messageData1);

            freeRastaMessageData(&messageData1);
        }
        else if (result->connection.my_id == ID_R) {
            if (result->connection.remote_id == ID_S1) client1 = 0;
            else if (result->connection.remote_id == ID_S2) client2 = 0;
        }
        break;
    case RASTA_CONNECTION_RETRREQ:
        print_log("\nCONNECTION_RETRREQ \r\n\n");
        break;
    case RASTA_CONNECTION_RETRRUN:
        print_log("\nCONNECTION_RETRRUN \r\n\n");
        break;
    }

}

void onHandshakeCompleted(struct rasta_notification_result *result)
{
    print_log("Handshake complete, state is now UP (with ID 0x%X)\r\n", result->connection.remote_id);
}

void onTimeout(struct rasta_notification_result *result){
    print_log("Entity 0x%X had a heartbeat timeout!\r\n", result->connection.remote_id);
}

void onReceive(struct rasta_notification_result *result) {
    rastaApplicationMessage p;

    rastaMsgHeader _rastaHeader;
    memset(&_rastaHeader, 0, sizeof(rastaMsgHeader));

    switch (result->connection.my_id) {
    case ID_R:
        //Server
//        print_log("\nReceived data from Client %d\r\n", result->connection.remote_id);
        p = sr_get_received_data(result->handle,&result->connection);

        print_log("\nData Packet is from %lu,l = %lu %\r\n", p.id,p.appMessage.length);

//        for(int i=0;i<p.appMessage.length;i++)
//        {
//        print_log("%02X ", p.appMessage.bytes[i]);
//        if(((i+1)%16)==0) print_log("\n");
//        }
        print_log("\r\n");

//        print_log("\nPacket is from %lu", p.id);
//        print_log("\nMsg: %s", p.appMessage.bytes);

        memcpy(&_rastaHeader, p.appMessage.bytes, sizeof(rastaMsgHeader));

        print_log("\nPacket Mesg Type client side%d\r\n", _rastaHeader.msgType);

        TRANS_CAN_MSG_WD_HDR(VCC, _rastaHeader.msgType, p.appMessage.length, p.appMessage.bytes, "rasta data");

        //FREE RastaApplicationMessage bytes
        rfree_pool(p.appMessage.bytes);

        unsigned long target;
//        if (p.id == ID_S1)
//        {
//            if (client2) {
//                // other client not connected
//                return;
//            }
//            target = ID_S2;
//        }
//        else
//        {
//            if (client1) {
//                // other client not connected
//                return;
//            }
//            target = ID_S1;
//        }

        print_log("Client message from %lu is now send to %lu\r\n", p.id,target);

        /*  struct RastaMessageData messageData1;
            allocateRastaMessageData(&messageData1, 1);

            addRastaString(&messageData1,0,(char*)p.appMessage.bytes);

            sr_send(result->handle,target,messageData1);
            print_log("Message forwarded\n");

            sleep(1);

            print_log("Disconnect to client %lu \n\n\n", target);
            sr_disconnect(result->handle,target);
         */




        break;

    case ID_S1:
    case ID_S2:

        print_log("\nReceived data from Server %d\r\n", result->connection.remote_id);
        p = sr_get_received_data(result->handle,&result->connection);

//        for(int i=0;i<p.appMessage.length;i++)
//        {
//        print_log("%02X ", p.appMessage.bytes[i]);
//        if(((i+1)%16)==0) print_log("\n");
//        }

        print_log("\nPacket is from %lu", p.id);
        print_log("\nMsg: %s", p.appMessage.bytes);

        print_log("\n\n\n");

        memcpy(&_rastaHeader, p.appMessage.bytes, sizeof(rastaMsgHeader));

        print_log("\nPacket Mesg Type server side %d\r\n", _rastaHeader.msgType);

        TRANS_CAN_MSG_WD_HDR(VCC, _rastaHeader.msgType, p.appMessage.length, p.appMessage.bytes, "rasta data");

        //FREE RastaApplicationMessage bytes
        rfree_pool(p.appMessage.bytes);

        break;
    }
}

//struct connect_event_data {
//    struct rasta_handle * h;
//    struct RastaIPData * ip_data_arr;
//    timed_event * connect_event;
//};


char connect_on_stdin(void * carry_data)
{
    struct connect_event_data *data = carry_data;
    //ClockP_sleep(2);

//    data->h->mux.config.general.initial_seq_num = data->h->config.values.general.initial_seq_num;
//    data->h->mux.config.general.rasta_id = data->h->config.values.general.rasta_id;
//    data->h->mux.config.general.rasta_network= data->h->config.values.general.rasta_network;
//
//    data->h->mux.config.redundancy.n_deferqueue_size = data->h->config.values.redundancy.n_deferqueue_size;
//    data->h->mux.config.redundancy.t_seq = data->h->config.values.redundancy.t_seq;

//    sr_connect(data->h, ID_R, data->ip_data_arr);
    sr_connect(&app.r_dle, ID_R, data->ip_data_arr,&app);
    print_log("%d ->   Connection request sent to 0x%lX\r\n", monotonic_time,(unsigned long)ID_R);
    disable_event(data->connect_event);
    return 0;
}


int rasta_init(){
    static struct RastaIPData toServer[2];
//    static struct rasta_handle r_dle;

    strcpy(toServer[0].ip, "192.168.25.83");
    strcpy(toServer[1].ip, "192.168.25.83");

    toServer[0].port = 15000;
    toServer[1].port = 15001;

    // acting as server

#ifdef STATION_TELAPUR
    char *argv[]={"rasta","r"};
#else
    char *argv[]={"rasta","s1"};
#endif

//    static timed_event connect_req;
//    static struct connect_event_data data;
    data = (struct connect_event_data) {
                                      .h = &app.r_dle,
                                      .ip_data_arr = toServer,
                                      .connect_event = &connect_req
    };

    connect_req.meta_information.callback = connect_on_stdin;
    connect_req.meta_information.carry_data =&data;
    //connect_req.meta_information.cartype = CONNECT;
    connect_req.meta_information.enabled = 1;
    connect_req.event_info.timed_event.interval_ns = (long)1000*0.1; //0.1 sec
    connect_req.type = TIMED_EVENT;

    /////// THIS SETS menotonic time
    //    eeprom_read(28,0);
    monotonic_time = CTR_MS;

    print_log("TIME in ms %lu\r\n",ClockP_getTimeUsec()/1000);

    if (strcmp(argv[1], "r") == 0) {
        print_log("->   R (ID = 0x%lX)%d\r\n", (unsigned long)ID_R,monotonic_time);
        ClockP_sleep(1); // sleep_ms_counter_based() will be use
        //getchar();
#ifdef BAREMETAL
        sr_init_handle(&app.r_dle, "r",&app);
#else
        sr_init_handle(&h, CONFIG_PATH_S);
#endif
        app.r_dle.notifications.on_connection_state_change = onConnectionStateChange;
        app.r_dle.notifications.on_receive = onReceive;
        app.r_dle.notifications.on_handshake_complete = onHandshakeCompleted;
        app.r_dle.notifications.on_heartbeat_timeout = onTimeout;
        //enable_fd_event(&fd_events[0]);
        sr_begin(&app.r_dle, NULL, 0,NULL,0, NULL);

    }
    else if (strcmp(argv[1], "s1") == 0) {
        print_log("->   S1 (ID = 0x%lX) \r\n", (unsigned long)ID_S1);
        sleep_s_counter_based(1);
#ifdef BAREMETAL
        sr_init_handle(&app.r_dle, "s1",&app);
#else
        sr_init_handle(&h, CONFIG_PATH_C1);
#endif
        app.r_dle.notifications.on_connection_state_change = onConnectionStateChange;
        app.r_dle.notifications.on_receive = onReceive;
        app.r_dle.notifications.on_handshake_complete = onHandshakeCompleted;
        //print_log("->   Press Enter to connect\n");
        //disable_fd_event(&fd_events[0]);
        //enable_fd_event(&fd_events[1]);
    }
        sr_begin(&app.r_dle, NULL, 0,&connect_req,1, NULL);
    return 0;
}

#endif
////RASTA Loop ---- END
