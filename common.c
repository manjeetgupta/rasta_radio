/*
 * common.c
 *
 *  Created on: 16-Jan-2025
 *      Author: sehra
 */

#include "common.h"
#include "radio_manager.h"

// CHANGE ACCORDING TO THIS
/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */


volatile unsigned char radio_check[30] = {
                                          0XF1, 0XA5, 0XC3, 0XD3, 0X60, 0XC3,
                                          0X01, 0X02, 0X03, 0X04, 0X05, 0X06,
                                          0X01, 0X02, 0X03, 0X04, 0X05, 0X06,
                                          0X01, 0X02, 0X03, 0X04, 0X05, 0X06,
                                          0X01, 0X02, 0X03, 0X04, 0X05, 0X06
};


ADJ_KAV_INFO adj_kav_info[2] = {};
volatile unsigned char radio_header_sof[3] = {0XF1, 0XA5, 0XC3};
//DATE_TIME_MS date_time;
volatile unsigned char radio_com_buffer[1500] = {};
volatile unsigned char radio_setup_buffer[1500] = {};
volatile unsigned char key_set_buffer[1500] = {};

// for rasta comm
volatile bool is_rasta_packet_rcvd = false;
volatile rastaInitDataSkavach rastaInitData = {};
volatile unsigned char rastaDataSkavach[300] = {};
volatile u16 canRastaMsgRecvdLen = 0;
//struct rasta_handle r_dle = {};

volatile uint16_t radio_buff_offset = 0;
volatile uint16_t radio_setup_buff_offset = 0;
volatile unsigned char prev_rad_cmd[100] = {};
uint8_t len_prev_rad_cmd = 0;
MESSAGE_HEADER msg_header = {};
volatile DATE_TIME_MS date_time = {}, date_time_rtc = {};

volatile QUEUE q_stn_reg_pkt={};

volatile QUEUE q_ss_pkt={};

volatile uint32_t GPS_SEC_CTR = 0;

volatile uint32_t curr_time = 0, prev_time = 0;

volatile ip_addr_t ip_l_kavach_map[10];


volatile SLOT_INFO loco_slot[20] = {0};
//volatile SLOT_INFO aa_slot[20] = {0};
volatile SLOT_INFO_F0 slot_f0[20] = {0};
//volatile uint8_t random_loco_cnt = 2;
volatile int8_t prev_sos_slot = -1;
volatile STRUCT_CC_SYS_INFO_STATUS system_status = {};
volatile uint8_t cur_key_index = 0;
volatile unsigned long timeout_curr_key_expiry = 0;

// #################################### FLAGS ####################################
volatile bool is_gps_pulse_recvd = false;
volatile bool is_time_rcvd_from_gps = false;
volatile bool is_valid_key_set_available = false;
volatile bool is_enet_up = true;
volatile bool is_radio_f0_reset = false;

volatile bool is_active_can_0 = false;
volatile bool is_active_can_1 = false;
volatile bool is_active_radio_0 = false;
volatile bool is_active_radio_1 = false;
volatile bool is_active_gps_0 = false;
volatile bool is_active_gps_1 = false;
volatile bool is_active_link_bw_controller = false;
volatile bool is_controller_healty = false;

volatile bool is_active_lan_0 = false;
volatile bool is_active_lan_1 = false;
// #################################### TIMEOUTS ####################################
volatile uint32_t timeout_in_sec_last_gps_0_update = 0;
volatile uint32_t timeout_in_sec_last_gps_1_update = 0;

volatile uint8_t rad_0_status_fail_count = 0;
volatile uint8_t gps_0_status_fail_count = 0;


int8_t print_log_1(const char *formatString, ...){

    int16_t _len = 0, _kav_id;
    err_t udp_err = ERR_OK;
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

    send_data_to_uart0((const unsigned char *)buffer, _len);

    if(is_active_lan_0 || is_active_lan_1){
        memcpy(send_buffer, &_log_header, sizeof(_log_header));
        memcpy(send_buffer + sizeof(_log_header), &_kav_id, sizeof(uint16_t));
        memcpy(send_buffer + sizeof(_log_header) + sizeof(uint16_t), buffer, _len);

        udp_err = UDP_Send_1((const char *)send_buffer, &ip_log, 50002, _log_header.msg_len);
        if (udp_err < 0) {
            //            printf("\nUDP log failed\n");
            return udp_err;
        }
    }

    return udp_err;
}

void App_initUDP1() {
    // Create new UDP PCB structure
    udp_pcb1 = udp_new();
    if (udp_pcb1 != NULL) {
        //// Bind to a specific local port
        //        udp_bind(udp_pcb1, IP_ADDR_ANY, UDP_LOCAL_PORT);
        //        DebugP_log("UDP initialized on port %d\n", UDP_LOCAL_PORT);
        //// Set up receive callback to listen for acknowledgment
        //        udp_recv(udp_pcb1, App_udpRecvCallback, NULL);
    } else {
        DebugP_log("Failed to create UDP PCB\n");
    }
}


int UDP_Send_1(const char *data, ip_addr_t *target_ip, uint16_t target_port,  int len) {
    struct pbuf *p;

    // Allocate memory for the packet buffer and copy the data
    p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p != NULL) {
        memcpy(p->payload, data, len);  // Copy the data to the buffer
        //        p->len = len;

        // Send data to the specified IP address and port
        err_t _stat = udp_sendto(udp_pcb1, p, target_ip, target_port);

        pbuf_free(p);  // Free the buffer after sending
        if(target_port != 50002){
            if(_stat == 0){
                print_log_1("\nData sent over UDP to %s:%d, len: %d", ipaddr_ntoa(target_ip), target_port, len);
            }
            else{
                err_enum_t err_code = _stat;
                print_log_1("\nError while sending data on UDP to %s:%d, Error: %s", ipaddr_ntoa(target_ip), target_port, get_udp_err1(err_code));
            }
        }
        return _stat;
    }
    DebugP_log("\nFailed to allocate pbuf for UDP data");
    return 1;
}


const char* get_udp_err1(err_enum_t stat){
    switch(stat){
    case ERR_OK:            return "No error";
    case ERR_MEM:           return "Out of memory error";
    case ERR_BUF:           return "Buffer error.";
    case ERR_TIMEOUT:       return "Timeout";
    case ERR_RTE:           return "Routing problem";
    case ERR_INPROGRESS:    return "Operation in progress";
    case ERR_VAL:           return "Illegal value";
    case ERR_WOULDBLOCK:    return "Operation would block";
    case ERR_USE:           return "Address in use";
    case ERR_ALREADY:       return "Already connecting";
    case ERR_ISCONN:        return "Conn already established";
    case ERR_CONN:          return "Not connected";
    case ERR_IF:            return "Low-level netif error";
    case ERR_ABRT:          return "Connection aborted";
    case ERR_RST:           return "Connection reset";
    case ERR_CLSD:          return "Connection closed";
    case ERR_ARG:           return "Illegal argument";
    }
}


volatile unsigned long long CTR_S = 0, CTR_MS = 0;
int counter2 = 0;
volatile int counterflag = 0;
unsigned long long rtc_time_at_2sec = 0, curr_time_ms = 0;

volatile bool cc_init_complete = false;

// Helper function to check if a year is a leap year
int is_leap_year(int year) {
    if (year % 4 == 0) {
        if (year % 100 == 0) {
            if (year % 400 == 0) return 1;
            else return 0;
        }
        return 1;
    }
    return 0;
}

// Helper function to calculate the number of days in a month
int days_in_month(int month, int year) {
    int days_in_months[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month == 2 && is_leap_year(year)) {
        return 29; // February in a leap year
    }
    return days_in_months[month - 1];
}

// Function to calculate the number of days since January 1, 1970
unsigned long calculate_days_since_epoch(int day, int month, int year) {
    unsigned long total_days = 0;

    ////
    year += 2000;

    // Add days for full years since 1970
    for (int y = 1970; y < year; y++) {
        total_days += (is_leap_year(y)) ? 366 : 365;
    }

    // Add days for full months in the current year
    for (int m = 1; m < month; m++) {
        total_days += days_in_month(m, year);
    }

    // Add days for the current month
    total_days += day - 1;

    return total_days;
}

// Function to calculate seconds since the epoch
unsigned long get_seconds_since_epoch(DATE_TIME_MS dateTime) {
    // Number of seconds in a day
    unsigned long seconds_in_day = SECONDS_IN_A_DAY;

    // Calculate days since epoch
    unsigned long total_days = calculate_days_since_epoch(dateTime.day, dateTime.month, dateTime.year);

    // Calculate seconds for the given time on that day
    unsigned long seconds_in_time_of_day = (dateTime.hour * 3600) + (dateTime.min * 60) + dateTime.second;

    // Total seconds since epoch
    unsigned long total_seconds = (total_days * seconds_in_day) + seconds_in_time_of_day;

    return total_seconds;
}

unsigned long get_seconds_since_epoch_for_key_set_end_time(KEY_TIME dateTime) {
    // Number of seconds in a day
    unsigned long seconds_in_day = SECONDS_IN_A_DAY;

    // Calculate days since epoch
    unsigned long total_days = calculate_days_since_epoch(dateTime.day, dateTime.month, dateTime.year);

    // Calculate seconds for the given time on that day
    unsigned long seconds_in_time_of_day = (dateTime.hour * 3600);// + (dateTime.min * 60) + dateTime.second;

    // Total seconds since epoch
    unsigned long total_seconds = (total_days * seconds_in_day) + seconds_in_time_of_day;

    return total_seconds;
}

unsigned long calculate_expiry_time_in_seconds_for_key(KEY_TIME dateTime){
    unsigned long key_time = get_seconds_since_epoch_for_key_set_end_time(dateTime);
    date_time.hour  = GPS_SEC_CTR/3600;
    date_time.min   = (GPS_SEC_CTR%3600)/60;
    date_time.second= (GPS_SEC_CTR%3600)%60;
    unsigned long curr_time = get_seconds_since_epoch(date_time);

    return key_time - curr_time + GPS_SEC_CTR;
}


//unsigned long long get_microseconds_since_epoch(DATE_TIME_MS dateTime) {
//    // Number of microseconds in a day
//    unsigned long long microseconds_in_day = MSECONDS_IN_A_DAY; // 86400 * 10^6
//
//    // Calculate days since epoch
//    unsigned long long total_days = calculate_days_since_epoch(dateTime.day, dateTime.month, dateTime.year);
//
//    // Calculate total seconds for the given time on that day
//    unsigned long long total_seconds_in_time_of_day = (dateTime.hour * 3600) + (dateTime.min * 60) + dateTime.second;
//
//    // Calculate total microseconds for the time
//    unsigned long long total_microseconds_in_time_of_day = total_seconds_in_time_of_day * 1000000LL + dateTime.microsecond;
//
//    // Total microseconds since epoch
//    unsigned long long total_microseconds = (total_days * microseconds_in_day) + total_microseconds_in_time_of_day;
//
//    return total_microseconds;
//}

void print_key_set(KEY_SET_INFO _key_set){
    print_log_1("\nKey-Set Info:");
    print_log_1("\nStart Time [DD.MM.YY HH:MM]:- %02d.%02d.%02d %02d:00",
                _key_set.start_time.day,
                _key_set.start_time.month,
                _key_set.start_time.year,
                _key_set.start_time.hour
    );

    print_log_1("\n End Time [DD.MM.YY HH:MM]:- %02d.%02d.%02d %02d:00",
                _key_set.end_time.day,
                _key_set.end_time.month,
                _key_set.end_time.year,
                _key_set.end_time.hour
    );

    print_log_1("\nKey 1: ");
    for(uint8_t i = 0; i < 16; i++){
        print_log_1("[%2X]", _key_set.key_set.key_set_1[i]);
    }

    print_log_1("\nKey 2: ");
    for(uint8_t i = 0; i < 16; i++){
        print_log_1("[%2X]", _key_set.key_set.key_set_2[i]);
    }
    print_log_1("\n");

}

bool validate_key_set_expiry(KEY_SET_INFO _key_set){
    bool is_valid = true;
    uint8_t hour = GPS_SEC_CTR/3600;

    if(_key_set.end_time.year <= date_time.year)
    {
        if(_key_set.end_time.month <= date_time.month)
        {
            if(_key_set.end_time.day <= date_time.day)
            {
                if(_key_set.end_time.hour < hour)
                {
                    is_valid = false;
                }
            }
        }
    }
    return is_valid;
}


void print_DATE_TIME_S(DATE_TIME_MS dateTime){
    print_log_1("\nTime-- %02d.%02d.%02d %02d:%02d:%02d",
                dateTime.day,
                dateTime.month,
                dateTime.year,
                dateTime.hour,
                dateTime.min,
                dateTime.second
    );
}

void sleep_ms_counter_based(uint32_t duration){
    long long unsigned int t1, t2;
    t1 = CTR_MS;
    t2 = CTR_MS;
    while(t2-t1<duration){
        t2 = CTR_MS;
    }
}

void sleep_s_counter_based(uint32_t duration){
    long long unsigned int t1, t2;
    t1 = CTR_S;
    t2 = CTR_S;
    while(t2-t1<duration){
        t2 = CTR_S;
    }
}

