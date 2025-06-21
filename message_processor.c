/*
 * message_processor.c
 *
 *  Created on: 18-Mar-2025
 *      Author: sehra
 */

#include "message_processor.h"
#include "common.h"
#include "radio_manager.h"
#include "MCAN/FIFO/mcan_api.h"
#include "ETHERNET/enet_user.h"
#include "gps_rtc_comm.h"

/* function to process data rcvd from UDP */
void processDataFunc(char* data, uint32_t len)
{
    int8_t res = -1;
    if(len<0) len = strlen(data);
    uint16_t offset = 0;
    //    uint8_t send_data[1500];
    uint8_t rcv_data[1500] = {};
    //    if(len >= sizeof(MESSAGE_HEADER)){
    memset(&msg_header, 0, sizeof(MESSAGE_HEADER));
    memcpy(&msg_header, data, sizeof(MESSAGE_HEADER));

    len = msg_header.msg_len;

    MESSAGE_HEADER* hdr = (MESSAGE_HEADER*)data;
    if(msg_header.dest_id == SCU){
        if(msg_header.src_id == SPU){
            switch(msg_header.msg_id){
            case MSG_ID_VCC_SPU_LOCO_DEREGISTER:{
                LOCO_ID _loco_id;
                memcpy(&_loco_id, data + sizeof(MESSAGE_HEADER), sizeof(LOCO_ID));
                deregister_loco(_loco_id);
            }
            break;
            case MSG_ID_SKAVACH_SKAVACH_MSGS:{
                process_skavach_msgs(data, len);
            }
            break;
            case MSG_ID_STN_ONB_REG_PKT:
            case MSG_ID_ONB_STN_REG_PKT:
            case MSG_ID_ACCESS_AUTH_PKT:
            case MSG_ID_ADD_EMERGEN_PKT:
            case MSG_ID_ONB_ACC_REQ_PKT:
            {
                if(is_valid_key_set_available && is_time_rcvd_from_gps){
#ifdef LAN_MODE
                    send_data_to_addressed_loco(data, len);
#else
                    process_radio_pkts(data, len);
#endif
                }
                else if(is_valid_key_set_available == false){
                    print_log("\n#################### Valid Key Set NA ####################\n");
                }
                else if(is_time_rcvd_from_gps == false){
                    print_log("\n#################### Valid time NA ####################\n");
                }
            }
            break;
            case MSG_ID_VCC_SCU_TIME_FREQ_REQ:{
                if(len>=sizeof(STRUCT_COMM_DATA)){
                    STRUCT_COMM_DATA* comm_data = (STRUCT_COMM_DATA*)data;
                    if(comm_data->comm_data.loco_id != 0){

                        res = addLocoData(&comm_data->comm_data);
                        if(res!=-1){

                            comm_data->header.dest_id = SPU;
                            comm_data->header.src_id = SCU;
                            comm_data->header.msg_id = MSG_ID_SCU_VCC_TIME_FREQ_RES;

                            print_log("\n####### Slot Info #######");
                            print_log("\nLoco id: %d", comm_data->comm_data.loco_id);
                            print_log("\nUP FREQ Channel: %d", comm_data->comm_data.alloted_uplink_freq);
                            print_log("\nDN FREQ Channel: %d", comm_data->comm_data.alloted_dnlink_freq);
                            print_log("\nAlloted TDMA: %d", comm_data->comm_data.alloted_tdma_timeslot);
                            //                            print_log("\nLOCO RND NO: %d", comm_data->comm_data.loco_rnd_no);
                            //                            print_log("\nSTN RND NO: %d", comm_data->comm_data.stn_rnd_no);
                            //                            print_log("\nSTN TDMA: %d", comm_data->comm_data.stn_tdma);

                            TRANS_CAN_MSG_WO_HDR(
                                    VCC,
                                    MSG_ID_SCU_VCC_TIME_FREQ_RES,
                                    comm_data->header.msg_len,
                                    (unsigned char *)comm_data,
                                    "Slot info sent to VCC"
                            );
                        }
                        else{
                            print_log("\nUnable to allocate new loco");
                        }
                    }
                    else{
                        print_log("\nINVALID LOCO ID RCVD FROM SKAVACH: 0");
                    }
                }
                else{
                    print_log("\nBuffer Underflow: %d", len);
                }
            }
            break;
            case MSG_ID_SPU_SCU_KEY_REQ:{
                bool is_key_req_flag = false;
                memcpy(&is_key_req_flag, data + sizeof(MESSAGE_HEADER), sizeof(is_key_req_flag));

                if(is_key_req_flag){
                    print_log("\nKey request rcvd");
                    if(is_valid_key_set_available){
                        MESSAGE_HEADER _header;
                        _header.src_id = SCU;
                        _header.dest_id = SPU;
                        _header.msg_id = MSG_ID_SCU_SPU_KEY_RES;
                        _header.msg_len = sizeof(curr_key_sets)+sizeof(MESSAGE_HEADER);

                        //                        memset(send_data, 0U, sizeof(send_data));
                        //                        memcpy(send_data, &_header, sizeof(MESSAGE_HEADER));
                        //                        memcpy(send_data + sizeof(MESSAGE_HEADER),
                        //                               curr_key_sets,
                        //                               sizeof(curr_key_sets));
                        //                        TRANS_CAN_MSG_WO_HDR(VCC, MSG_ID_SCU_SPU_KEY_RES, _header.msg_len, send_data, "KEY SENT TO VCC\n");

                        CC_VCC_KEY_RES key_data = {};
                        memcpy(&key_data.curr_key_sets, curr_key_sets, sizeof(KEY_SET_INFO)*2);
                        key_data.index1 = cur_key_index;
                        key_data.index1 = cur_key_index + 1;
                        key_data.no_of_remainig_key_set = MAX_KEY_SETS - cur_key_index;

                        TRANS_CAN_MSG_WD_HDR(
                                VCC,
                                MSG_ID_SCU_SPU_KEY_RES,
                                sizeof(CC_VCC_KEY_RES),
                                (unsigned char*)&key_data,
                                "KEY SENT TO VCC\n"
                        );

                    }
                    else
                    {
                        TRANS_CAN_MSG_WD_HDR(VCC, MSG_ID_SCU_SPU_KEY_RES, 0, NULL, "KEY NA, Sent HDR\n");
                    }
                }
                else{

                }
            }
            break;
            case MSG_ID_VCC_SCU_CHECKSUM_REQ:{
                CC_VCC_CHECKSUM_RES _res = {};
                _res.scu_crc = 4054172671;

                TRANS_CAN_MSG_WD_HDR(
                        VCC,
                        MSG_ID_SCU_VCC_CHECKSUM_RESP,
                        sizeof(CC_VCC_CHECKSUM_RES),
                        (unsigned char*)&_res,
                        "Cksum sent to VCC\n"
                );
            }
            break;
            }
        }
        else if(msg_header.src_id == SDLU){
            switch(msg_header.msg_id){
            case MSG_ID_DL_COMM_KEY_RES:
            {
                if(len>=(sizeof(MESSAGE_HEADER) + sizeof(uint8_t) + (sizeof(KEY_SET_INFO) * 2) )){
                    memset(rcv_data, 0U, 1500);
                    memcpy(rcv_data, data + sizeof(MESSAGE_HEADER), len - sizeof(MESSAGE_HEADER));
                    uint16_t key_offset = 0;
                    uint8_t hour = GPS_SEC_CTR/3600;
                    print_log_1("\nCurr Time:  %02d.%02d.%02d %02d:00",
                                date_time.day,date_time.month,date_time.year, hour);
                    print_log("\nKey set info rcvd from DL");

                    offset = 0;
                    uint8_t no_of_key_set = 0;
                    memcpy(&no_of_key_set, rcv_data + offset, sizeof(uint8_t));
                    offset += sizeof(uint8_t);
                    key_offset = offset;

                    //                    /*
                    //                     * START: change for key set test
                    //                     */
                    //                    print_log("\ncheck check for KMS");
                    //                    KEY_SET_INFO* curr_key = (KEY_SET_INFO*)(rcv_data + offset);
                    //                    print_key_set(*curr_key);
                    //                    curr_key->end_time.year = date_time.year;
                    //                    curr_key->end_time.month= date_time.month;
                    //                    curr_key->end_time.day  = date_time.day;
                    //                    curr_key->end_time.hour = hour + 1;
                    //
                    //                    curr_key = (KEY_SET_INFO*)(rcv_data + offset+ sizeof(KEY_SET_INFO));
                    //                    print_key_set(*curr_key);
                    //                    curr_key->end_time.year = date_time.year;
                    //                    curr_key->end_time.month= date_time.month;
                    //                    curr_key->end_time.day  = date_time.day;
                    //                    curr_key->end_time.hour = hour + 2;
                    /*
                     * END
                     */

                    print_log("\nNo of key_set: %d", no_of_key_set);
                    cur_key_index = MAX_KEY_SETS - no_of_key_set;

                    KEY_SET_INFO _key_set = {};
                    for(uint8_t i = 0; i<no_of_key_set; i++){
                        memset(&_key_set, 0U, sizeof(KEY_SET_INFO));
                        memcpy(&_key_set, rcv_data + offset, sizeof(KEY_SET_INFO));
                        //                        print_key_set(_key_set);
                        //// verify key set validity
                        if(!validate_key_set_expiry(_key_set)){
                            key_offset = offset;
                            cur_key_index = cur_key_index - i + 1;
                        }

                        offset += sizeof(KEY_SET_INFO);
                    }

                    ///// copy valid key buffer
                    memcpy((void*)key_set_buffer, rcv_data + key_offset, (sizeof(KEY_SET_INFO)*(MAX_KEY_SETS-cur_key_index)));

                    ///// copy two latest key_set
                    memcpy(&curr_key_sets, rcv_data + key_offset, sizeof(curr_key_sets));
                    print_log("\n####################### Latest Key Set ####################### ");
                    print_key_set(curr_key_sets[0]);
                    print_key_set(curr_key_sets[1]);

                    print_log("\nCurr Key Index: %d", cur_key_index);

                    //// calculate expiry time for first key
                    timeout_curr_key_expiry = calculate_expiry_time_in_seconds_for_key(curr_key_sets[0].end_time);
                    print_log("\nTime for expiry: %ld s\n", timeout_curr_key_expiry - GPS_SEC_CTR);
                    /*
                     * START
                     */
                    /////// this is for checking expiry code
                    ////        timeout_curr_key_expiry = GPS_SEC_CTR + 310;
                    /*
                     * END
                     */
                    //// set valid key is rcvd
                    is_valid_key_set_available = true;

                }
                else{
                    print_log("\nBuffer underflow, len: %d", len);
                }
            }
            break;
            }
        }
    }
    else{
        print_log("\nInvalid Dest Rcvd: %d", hdr->dest_id);
    }
    //    }
    //    else{
    //        print_log("\nLen rcvd is less then size of Header, len: %d", len);
    //    }

}

void send_data_to_addressed_loco(const char* data, const uint32_t len){

    uint32_t loco_id = 0;
    memcpy(&msg_header, data, sizeof(MESSAGE_HEADER));
    MESSAGE_HEADER* hdr = (MESSAGE_HEADER*)data;
    hdr->src_id = LCU;
    hdr->dest_id = LPU;
    hdr->msg_len = hdr->msg_len - sizeof(loco_id);

    if(len >= (sizeof(MESSAGE_HEADER) + sizeof(loco_id))){
        memcpy(&loco_id, data + sizeof(msg_header), sizeof(loco_id));

        unsigned char* send_data = (unsigned char*)malloc(len-sizeof(loco_id));
        memset(send_data, 0, len-sizeof(loco_id));

        memcpy(send_data, data, sizeof(MESSAGE_HEADER));
        memcpy(send_data + sizeof(MESSAGE_HEADER),
               data + sizeof(MESSAGE_HEADER) + sizeof(loco_id),
               len-sizeof(loco_id)-sizeof(MESSAGE_HEADER));
        err_t udp_err = -1;
        print_log_1(" Loco id: %d", loco_id);
        if(loco_id == 0){
            for (uint8_t var = 1; var <= 3; ++var) {
                udp_err = UDP_Send((const char*)send_data, (ip_addr_t *)&ip_l_kavach_map[var], DEF_UDP_PORT, len- sizeof(loco_id));
            }
            if(udp_err == -1)
                print_log_1("\nUnable to send data to LOCO");
            else
                print_log_1("\nDATA Sent TO ALL LOCO");
        }
        else{
            udp_err = UDP_Send((const char*)send_data, (ip_addr_t *)&ip_l_kavach_map[loco_id], DEF_UDP_PORT, len- sizeof(loco_id));
            if(udp_err == -1)
                print_log_1("\nUnable to send data to LOCO: %d", loco_id);
            else
                print_log_1("\nDATA SENT TO LOCO: %d", loco_id);
        }
        free(send_data);
    }
    else{
        print_log("\nBuffer dosent contain loco id, msg_len: %d", len);
    }
}

void process_skavach_msgs(const char* data, const uint32_t len){
    uint16_t station_id = 0;
    uint8_t offset = 0;

    offset = sizeof(MESSAGE_HEADER);// + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);

    memcpy(&station_id, data + offset, sizeof(uint16_t));

    for (uint8_t var = 0; var < 2U; ++var) {
        if(station_id == adj_kav_info[var].station_id){
            err_t udp_err = UDP_Send_1((const char *)data + sizeof(MESSAGE_HEADER),
                                       &adj_kav_info[var].ip_addr, DEF_UDP_PORT, len - sizeof(MESSAGE_HEADER));
            if(udp_err == -1)
                print_log("\nError Sending Message to KAVACH_id: %d", station_id);
            else
                print_log("\nMsg sent to KAVACH_id: %d, len: %d", station_id, udp_err);
            return;
        }
    }

    print_log("\nAdj kavach id not present at SCU: %d", station_id);

}

void update_gps_time_to_other_units(){
    if(is_time_rcvd_from_gps && date_time.day != 0){
        unsigned char _buffer[10] = {};
        date_time.hour      = GPS_SEC_CTR/3600;
        date_time.min       = (GPS_SEC_CTR%3600)/60;
        date_time.second    = (GPS_SEC_CTR%3600)%60;

        memcpy((void*)_buffer, (void *)&date_time, sizeof(date_time));
        TRANS_CAN_MSG_WD_HDR(SPU, MSG_ID_COMM_ALL_GPS_UPDATE, sizeof(date_time), _buffer, "GPS UPDATED TO OTHER UNIT");
    }
    else{
        print_log("\ntime not rcvd from GPS!\n");
    }
}

void check_key_set_expiry(){
    print_log("\nUpdating key set to VCC\n time_for_expiry: %ld s", timeout_curr_key_expiry-GPS_SEC_CTR);
    if(cur_key_index <= MAX_KEY_SETS-1){
        cur_key_index = cur_key_index + 1;

        memcpy(
                (void*)key_set_buffer,
                (void*)(key_set_buffer + sizeof(KEY_SET_INFO)),
                (sizeof(KEY_SET_INFO)*(MAX_KEY_SETS-cur_key_index))
        );

        ///// copy two latest key_set
        memcpy(&curr_key_sets, (void*)key_set_buffer, sizeof(curr_key_sets));
        print_log("\n####################### Latest Key Set ####################### ");
        print_key_set(curr_key_sets[0]);
        print_key_set(curr_key_sets[1]);

        print_log("\nCurr Key Index: %d", cur_key_index);

        //// calculate expiry time for first key
        timeout_curr_key_expiry = calculate_expiry_time_in_seconds_for_key(curr_key_sets[0].end_time);
        print_log("\nTime for expiry: %ld s\n", timeout_curr_key_expiry - GPS_SEC_CTR);
        /////// this is for checking expiry code
        ////        timeout_curr_key_expiry = GPS_SEC_CTR + 302;

        TRANS_CAN_MSG_WD_HDR(
                VCC,
                MSG_ID_SCU_SPU_KEY_RES,
                sizeof(curr_key_sets),
                (unsigned char*)&curr_key_sets,
                "KEY SENT TO VCC before expiry\n"
        );
    }
    else{
        print_log("\n##################### NO VALID KEY AVAILABLE AT SCU #####################");
        is_valid_key_set_available = false;
        print_log("\nwaiting for keys from DL");
        while(!is_valid_key_set_available){
            req_key_from_dl();
            update_gps_time_to_other_units();
            long long unsigned int t1, t2;
            t1 = time_msec;
            t2 = time_msec;
            while(t2-t1<3000){
                t2 = time_msec;
                check_gps_comm();
                mcan_checks_fifo();
                if(is_valid_key_set_available) break;
            }
        }
    }
}
