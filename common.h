/*
 * common_struct.h
 *  Created on: 16-Jan-2025
 *      Author: sehra
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/udp.h"
#include "/QUEUE/myqueue.h"
#include "rasta_new.h"

extern volatile unsigned char radio_check[30];
#define PACKED __attribute__ ((packed))

#define time_msec (ClockP_getTicks())
#define time_usec (ClockP_getTimeUsec())

//#define STATION_TELAPUR
#define STATION_LINGAMPALLY
#define MODE_RADIO_LAN
//#define GPS_ENABLED // revert back
//#define KMS_KEY_ENABLED // revert back
//#define LAN_MODE
#define CC  9
#define VCC 4
#define DLT 8

#define ID_R 0x61
#define ID_S1 0x62
#define ID_S2 0x63

#ifdef STATION_TELAPUR
#define self_kavach_id 527
#elif defined(STATION_LINGAMPALLY)
#define self_kavach_id 501
#else
#define self_kavach_id 555
#endif

#define DEF_UDP_PORT 50001

///// MAX
#define MAX_LEN_RADIO_PKT_REG                   500U
#define MAX_LEN_RADIO_PKT_EMER                  54U
#define MAX_LEN_RADIO_PKT_ACC_AUTH              54U
#define MAX_KEY_SETS                            30
#define SECONDS_IN_A_DAY                        86400
#define MSECONDS_IN_A_DAY                       86400000000LL
#define GPS_UPDATE_TIME_IN_SECONDS              4U
#define MAX_TIME_KEY_UPDATE_TO_VCC              300U

///// CSCI
#define LPU 2U
#define SPU 4U
#define SCU 9U
#define LCU 7U
#define LDLU 6U
#define SDLU 8U

#define LOCO_ID uint32_t

#define MSG_ID_COMM_ALL_GPS_UPDATE              1U
#define MSG_ID_STN_ONB_REG_PKT                  9U
#define MSG_ID_ONB_STN_REG_PKT                  10U
#define MSG_ID_ACCESS_AUTH_PKT                  11U
#define MSG_ID_ADD_EMERGEN_PKT                  12U
#define MSG_ID_ONB_ACC_REQ_PKT                  13U
#define MSG_ID_ONB_ACC_REQ_WD_EMER_STATUS_PKT   1111U

#define MSG_ID_COMM_DL_KEY_REQ                  101U
#define MSG_ID_DL_COMM_KEY_RES                  101U
#define MSG_ID_SPU_SCU_KEY_REQ                  226U
#define MSG_ID_SCU_SPU_KEY_RES                  103U
#define MSG_ID_SKAVACH_SKAVACH_MSGS             201U
#define MSG_ID_VCC_SCU_TIME_FREQ_REQ            104U
#define MSG_ID_SCU_VCC_TIME_FREQ_RES            101U
#define MSG_ID_VCC_SPU_LOCO_DEREGISTER          230U

#define CAN_ID_COMM_DL_KEY_REQ                  0x63
#define CAN_ID_KEY_RES                          0x70
#define CAN_ID_COMM_ALL_GPS_UPDATE              0x07
#define CAN_ID_ADD_EMERGEN_PKT                  0x08
#define CAN_ID_ONB_ACC_REQ_PKT                  0x2B
#define CAN_ID_ONB_STN_REG_PKT                  0x2C
#define CAN_ID_ACCESS_AUTH_PKT                  0x2D
#define CAN_ID_STN_ONB_REG_PKT                  0x2E
#define CAN_ID_VCC_SCU_TIME_FREQ_REQ            0x6D
#define CAN_ID_SCU_VCC_TIME_FREQ_RES            0x68
#define CAN_ID_SPU_SCU_KEY_REQ                  0x6C
#define CAN_ID_SCU_SPU_KEY_RES                  0x65
#define CAN_ID_VCC_SPU_LOCO_DEREGISTER          0x78

//######################## NEW MESSAGE ID ########################
#define MSG_ID_SCU_VCC_SYS_INFO_STATUS          231U
#define MSG_ID_VCC_SCU_CHECKSUM_REQ             46U
#define MSG_ID_SCU_VCC_CHECKSUM_RESP            157U

#define CAN_ID_SCU_VCC_SYS_INFO_STATUS          0x69
#define CAN_ID_VCC_SCU_CHECKSUM_REQ             0x6A
#define CAN_ID_SCU_VCC_CHECKSUM_RESP            0x66

///// SKAVACH - SKAVACH
#define MSG_ID_SS_CMD_PDI_VRSN_CHECK            ((u16)(0x0101U))//257
#define MSG_ID_SS_MSG_PDI_VRSN_CHECK            ((u16)(0x0102U))//258
#define MSG_ID_SS_HEART_BEAT_MSG                ((u16)(0x0103U))//259
#define MSG_ID_SS_TRAIN_HANDOVR_REQ_MSG         ((u16)(0x0104U))//260
#define MSG_ID_SS_TRAIN_RRI_MSG                 ((u16)(0x0105U))//261
#define MSG_ID_SS_TRAIN_TAKEN_OVR_MSG           ((u16)(0x0106U))//262
#define MSG_ID_SS_TRAIN_HANDOVR_CNCL_MSG        ((u16)(0x0107U))
#define MSG_ID_SS_TRAIN_LNTH_INFO_MSG           ((u16)(0x0108U))
#define MSG_ID_SS_TRAIN_LNTH_ACK                ((u16)(0x0109U))
#define MSG_ID_SS_TSL_RUT_REQ_MSG               ((u16)(0x010AU))
#define MSG_ID_SS_TSL_INFO_MSG                  ((u16)(0x010BU))
#define MSG_ID_SS_FIELD_ELMNTS_STATUS_REQ_MSG   ((u16)(0x010CU))
#define MSG_ID_SS_FIELD_ELMNTS_STATUS_MSG       ((u16)(0x010DU))
#define MSG_ID_SS_TRAIN_HANDOVR_CNCL_ACK_MSG    ((u16)(0x010EU))// TBD


#define CAN_ID_SS_FIELD_ELMNTS_STATUS_MSG       ((u16)(0x46U))
#define CAN_ID_SS_FIELD_ELMNTS_STATUS_REQ_MSG   ((u16)(0x47U))
#define CAN_ID_SS_HEART_BEAT_MSG                ((u16)(0x48U))//259
#define CAN_ID_SS_CMD_PDI_VRSN_CHECK            ((u16)(0x4AU))//257
#define CAN_ID_SS_MSG_PDI_VRSN_CHECK            ((u16)(0x4BU))//258
#define CAN_ID_SS_TRAIN_HANDOVR_REQ_MSG         ((u16)(0x4FU))//260
#define CAN_ID_SS_TRAIN_HANDOVR_CNCL_ACK_MSG    ((u16)(0x50U))//TBD
#define CAN_ID_SS_TRAIN_HANDOVR_CNCL_MSG        ((u16)(0x51U))
#define CAN_ID_SS_TRAIN_LNTH_ACK                ((u16)(0x52U))
#define CAN_ID_SS_TRAIN_LNTH_INFO_MSG           ((u16)(0x53U))
#define CAN_ID_SS_TRAIN_RRI_MSG                 ((u16)(0x54U))//261
#define CAN_ID_SS_TRAIN_TAKEN_OVR_MSG           ((u16)(0x55U))//262
#define CAN_ID_SS_TSL_RUT_REQ_MSG               ((u16)(0x57U))
#define CAN_ID_SS_TSL_INFO_MSG                  ((u16)(0x58U))


////////////////////// not matched from stcas
#define MSG_ID_SS_ACK_MSG                       ((u16)(11U))
#define MSG_ID_SS_INIT_REQ                      ((u16)(50U))
#define MSG_ID_SS_STATUS_INFO                   ((u16)(49U))
#define MSG_ID_SS_SYNC_DATA                     ((u16)(51U))
#define MSG_ID_SS_TRAIN_ACCEPTANCE_MSG          ((u16)(14U))
#define MSG_ID_SS_TSL_AUTHORITY_MSG             ((u16)(514U))

#define CAN_ID_SS_ACK_MSG                       ((u16)(69U))
#define CAN_ID_SS_INIT_REQ                      ((u16)(73U))
#define CAN_ID_SS_STATUS_INFO                   ((u16)(76U))
#define CAN_ID_SS_SYNC_DATA                     ((u16)(77U))
#define CAN_ID_SS_TRAIN_ACCEPTANCE_MSG          ((u16)(78U))
#define CAN_ID_SS_TSL_AUTHORITY_MSG             ((u16)(86U))
////////////////////////

////// SKAVACH - TSRMS
#define MSG_ID_SKAVACH_TSRMS_ACK_MSG                                ((u16)(0x0207U))
#define MSG_ID_SKAVACH_TSRMS_PDI_VER_CHECK                          ((u16)(0x0201U))
#define MSG_ID_SKAVACH_TSRMS_TSR_DATA_MSG                           ((u16)(0x0205U))
#define MSG_ID_SKAVACH_SCU_INIT_MSG                                 ((u16)(0x0208U))

#define MSG_ID_TSRMS_SKAVACH_ALL_TSR_INFO                           ((u16)(0x0203U))
#define MSG_ID_TSRMS_SKAVACH_DATA_INTEGRITY                         ((u16)(0x0206U))
#define MSG_ID_TSRMS_SKAVACH_GET_TSR_INFO                           ((u16)(0x0204U))
#define MSG_ID_TSRMS_SKAVACH_PDI_VER_CHECK                          ((u16)(0x0202U))

#define CAN_ID_SKAVACH_TSRMS_ACK_MSG                                ((u16)(28U))
#define CAN_ID_SKAVACH_TSRMS_PDI_VER_CHECK                          ((u16)(29U))
#define CAN_ID_SKAVACH_TSRMS_TSR_DATA_MSG                           ((u16)(30U))

#define CAN_ID_TSRMS_SKAVACH_ALL_TSR_INFO                           ((u16)(31U))
#define CAN_ID_TSRMS_SKAVACH_DATA_INTEGRITY                         ((u16)(32U))
#define CAN_ID_TSRMS_SKAVACH_GET_TSR_INFO                           ((u16)(33U))
#define CAN_ID_TSRMS_SKAVACH_PDI_VER_CHECK                          ((u16)(34U))

#define KEY_KMS_INDEX uint8_t //0: Default key set, 1-30: KMS key set
#define CRC uint32_t


////// CANid MSGid buff mapping
static uint16_t CANidMSGidBuffMapping1[64][2]={
                                               {CAN_ID_COMM_DL_KEY_REQ,      MSG_ID_COMM_DL_KEY_REQ},
                                               {CAN_ID_COMM_ALL_GPS_UPDATE,  MSG_ID_COMM_ALL_GPS_UPDATE},
                                               {CAN_ID_ONB_STN_REG_PKT,      MSG_ID_ONB_STN_REG_PKT},
                                               {CAN_ID_ONB_ACC_REQ_PKT,      MSG_ID_ONB_ACC_REQ_PKT},
                                               {CAN_ID_SCU_VCC_TIME_FREQ_RES,MSG_ID_SCU_VCC_TIME_FREQ_RES},
                                               {CAN_ID_ADD_EMERGEN_PKT,      MSG_ID_ADD_EMERGEN_PKT},
};

#ifdef MODE_RADIO_LAN
ip_addr_t ip_l_kavach1, ip_l_kavach2, ip_l_kavach3;
#endif
ip_addr_t ip_s_kavach, ip_log, ip_dl, ip_other_s_kavach;
ip_addr_t csci_add_map[10];
extern volatile ip_addr_t ip_l_kavach_map[10];


extern volatile uint8_t random_loco_cnt;

typedef enum __attribute__((__packed__))
{
    eRADIO_1_SETUP=0,
            eRADIO_1_COM=1,
            eRADIO_2_SETUP=2,
            eRADIO_2_COM=3,
            eGPS_1=4,
            eGPS_2=5,
}eIFSerial;

typedef enum __attribute__((__packed__))
{
    eVCC = 0x01U,
            eDLU = 0x03U,
            eCC  = 0x02U,
}eIFCAN;

typedef struct {
    LOCO_ID key;
    uint16_t len;
    uint8_t tdma;
    uint8_t data[MAX_LEN_RADIO_PKT_REG];
}RAD_PKT_REG;

typedef struct {
    LOCO_ID loco_id;
    uint8_t len;
    uint8_t slot_used;
    uint8_t data[MAX_LEN_RADIO_PKT_ACC_AUTH];
}RAD_PKT_AA;

typedef struct {
    //  LOCO_ID key;
    uint8_t data[MAX_LEN_RADIO_PKT_EMER];
}RAD_PKT_EMER;

typedef struct {
    uint16_t station_id;
    ip_addr_t ip_addr;
}ADJ_KAV_INFO;

typedef struct PACKED{
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
}KEY_TIME;

typedef struct PACKED{
    uint8_t key_set_1[16];
    uint8_t key_set_2[16];
}KEY_SET;

typedef struct PACKED{
    KEY_TIME start_time;
    KEY_TIME end_time;
    KEY_SET key_set;
}KEY_SET_INFO;

typedef struct
{
    uint8_t     day;
    uint8_t     month;
    uint8_t     year;
    uint8_t     hour;
    uint8_t     min;
    uint8_t     second;
    //    uint16_t    msec;
}DATE_TIME_MS;

typedef struct PACKED
{
    uint8_t src_id;
    uint8_t dest_id;
    uint16_t msg_id;
    uint16_t msg_len;
    uint16_t seq_no;
}MESSAGE_HEADER;

typedef struct PACKED
{
    uint16_t stationid;
    uint16_t stationid_1;
    uint8_t flag_1;
    uint16_t stationid_2;
    uint8_t flag_2;
    uint16_t stationid_3;
    uint8_t flag_3;
    uint16_t stationid_4;
    uint8_t flag_4;
    uint16_t stationid_5;
    uint8_t flag_5;
    uint16_t stationid_6;
    uint8_t flag_6;
}rastaInitDataSkavach;

typedef struct PACKED
{
    uint8_t src_id;
    uint8_t dest_id;
    uint16_t msg_id;
    uint16_t msg_len;
    uint16_t seq_no;
    LOCO_ID loco_id;
}MESSAGE_HEADER_WD_LOCO_ID;

typedef struct PACKED{
    uint8_t protocol; // 0xF2 default value
    uint16_t msgType;
    uint16_t senderId;
    uint16_t recevId;
}rastaMsgHeader;

typedef struct PACKED
{
    LOCO_ID  loco_id;
    uint16_t loco_rnd_no;
    uint16_t alloted_uplink_freq;
    uint16_t alloted_dnlink_freq;
    uint8_t  alloted_tdma_timeslot;
    uint16_t stn_rnd_no;
    uint8_t  stn_tdma;
}COMM_DATA;

typedef struct PACKED
{
    MESSAGE_HEADER  header;
    COMM_DATA       comm_data;
}STRUCT_COMM_DATA;

typedef struct PACKED
{
    uint8_t flag;
    uint8_t loco_index;
    uint8_t data_available;
}SLOT_INFO;

typedef struct PACKED
{
    uint8_t     flag;
    uint16_t    msg_type; // ACC AUTH or SOS
    uint8_t     index;
    uint8_t     data[MAX_LEN_RADIO_PKT_ACC_AUTH];
    uint8_t     len;
}SLOT_INFO_F0;

//ACTIVE_STATUS radio/gps
//0 – No Active GPS
//1 – GPS 1
//2 – GPS 2
//3 – Both GPS

//GPS_VIEW
//0 – No Data
//1 – V
//2 – A

typedef struct PACKED
{
    ACTIVE_STATUS   active_radio_no;
    HEALTH_STS      health_status_radio1;
    HEALTH_STS      health_status_radio2;
    VOLTAGE         radio1_input_supply;
    VOLTAGE         radio2_input_supply;
    TEMEPRATURE     radio1_temp;
    TEMEPRATURE     radio2_temp;
    TEMEPRATURE     radio1_pa_temp;
    TEMEPRATURE     radio2_pa_temp;
    VOLTAGE         radio1_pa_supply;
    VOLTAGE         radio2_pa_supply;
    AMPERE          radio1_tx_pa_current;
    AMPERE          radio2_tx_pa_current;
    POWER           radio1_reverse_power;
    POWER           radio2_reverse_power;
    POWER           radio1_forward_power;
    POWER           radio2_forward_power;
    COUNT           radio1_rx_packet_count; // no of reg pkt rcvd in 2s
    COUNT           radio2_rx_packet_count;
    ACTIVE_STATUS   active_gps_no;
    GPS_VIEW        gps1_view;
    GPS_VIEW        gps2_view;
    SECOND          gps1_seconds;
    SECOND          gps2_seconds;
    uint8_t         gps1_satellite_in_view;
    uint8_t         gps2_satellite_in_view;
    uint8_t         gps1_cno_max;
    uint8_t         gps2_cno_max;
    //    STATUS          gsm1_health_status;
    //    STATUS          gsm2_health_status;
    //    KEY current_runing_key[16];
    //    CHECKSUM session_key_checksum;
}STRUCT_CC_SYS_INFO_STATUS;

typedef struct PACKED
{
    KEY_SET_INFO    curr_key_sets[2];
    KEY_KMS_INDEX   index1;
    KEY_KMS_INDEX   index2;
    uint8_t         no_of_remainig_key_set;
    CRC             crc;
}CC_VCC_KEY_RES;

typedef struct PACKED
{
    CRC             scu_crc;
    CRC             dl_crc;
    CRC             crc;
}CC_VCC_CHECKSUM_RES;
//SCU_SKAVACH_CHECKSUM_RESP 1
//SKAVACH_SCU_CHECKSUM_REQ 46

KEY_SET_INFO key_set_info_obj[MAX_KEY_SETS];
KEY_SET_INFO curr_key_sets[2];
extern volatile unsigned char key_set_buffer[1500];


extern volatile QUEUE q_stn_reg_pkt;
extern volatile QUEUE q_ss_pkt;

extern volatile bool cc_init_complete;

extern volatile unsigned char radio_header_sof[3];
extern volatile unsigned char radio_setup_buffer[1500], radio_com_buffer[1500];
extern volatile uint16_t radio_buff_offset;
extern volatile uint16_t radio_setup_buff_offset;
extern volatile unsigned char prev_rad_cmd[100];
extern uint8_t len_prev_rad_cmd;
extern volatile unsigned long long CTR_S, CTR_MS;
extern int counter2;
extern volatile int counterflag;
extern unsigned long long rtc_time_at_2sec, curr_time_ms;


// for rasta comm
extern volatile rastaInitDataSkavach rastaInitData;
extern volatile unsigned char rastaDataSkavach[300];
extern volatile u16 canRastaMsgRecvdLen;
extern volatile bool is_rasta_packet_rcvd;
//extern struct rasta_handle r_dle;

struct connect_event_data {
    struct rasta_handle * h;
    struct RastaIPData * ip_data_arr;
    timed_event * connect_event;
};

// #################################### FLAGS ####################################
extern volatile bool is_enet_up;
extern volatile bool is_radio_f0_reset;
extern volatile bool is_valid_key_set_available;
extern volatile bool is_time_rcvd_from_gps;
extern volatile bool is_gps_pulse_recvd;

extern volatile bool is_active_can_0;
extern volatile bool is_active_can_1;
extern volatile bool is_active_radio_0;
extern volatile bool is_active_radio_1;
extern volatile bool is_active_gps_0;
extern volatile bool is_active_gps_1;
extern volatile bool is_active_link_bw_controller;
extern volatile bool is_controller_healty;

extern volatile bool is_active_lan_0;
extern volatile bool is_active_lan_1;

// #################################### TIMEOUTS ####################################
extern volatile uint32_t timeout_in_sec_last_gps_0_update;
extern volatile uint32_t timeout_in_sec_last_gps_1_update;


extern volatile uint8_t rad_0_status_fail_count;
extern volatile uint8_t gps_0_status_fail_count;


static struct udp_pcb *udp_pcb1, udp_pcb2;  // Pointer to UDP protocol control block
extern ADJ_KAV_INFO adj_kav_info[2];
extern volatile DATE_TIME_MS date_time, date_time_rtc;
extern MESSAGE_HEADER msg_header;

extern volatile uint32_t GPS_SEC_CTR;
extern volatile bool is_gps_pulse_recvd;


void print_key_set(KEY_SET_INFO _key_set);
int8_t print_log_1(const char *formatString, ...);
int UDP_Send_1(const char *data, ip_addr_t *target_ip, uint16_t target_port,  int len);
void App_initUDP1();
const char* get_udp_err1(err_enum_t stat);
void print_DATE_TIME_S(DATE_TIME_MS dateTime);
void sleep_ms_counter_based(uint32_t duration);
void sleep_s_counter_based(uint32_t duration);


extern volatile uint32_t prev_time_ms;
extern volatile SLOT_INFO loco_slot[20];
//extern volatile SLOT_INFO aa_slot[20];
extern volatile SLOT_INFO_F0 slot_f0[20];
extern volatile int8_t prev_sos_slot;
extern volatile STRUCT_CC_SYS_INFO_STATUS system_status;

extern volatile uint8_t cur_key_index;
extern volatile unsigned long timeout_curr_key_expiry;
bool validate_key_set_expiry(KEY_SET_INFO _key_set);
unsigned long get_seconds_since_epoch_for_key_set_end_time(KEY_TIME dateTime);
unsigned long get_seconds_since_epoch(DATE_TIME_MS dateTime);
unsigned long calculate_expiry_time_in_seconds_for_key(KEY_TIME dateTime);
#endif /* COMMON_H_ */
