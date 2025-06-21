/*
 * message_processor.h
 *
 *  Created on: 18-Mar-2025
 *      Author: sehra
 */

#ifndef SOURCE_MESSAGE_PROCESSOR_H_
#define SOURCE_MESSAGE_PROCESSOR_H_

#include "stdint.h"

void processDataFunc(char* data, uint32_t len);

void process_key_req_from_vcc(const char* data, uint32_t len);

int8_t send_data_to_other_units_for_lan(uint8_t dest_id, uint16_t msg_id, uint16_t msg_len, unsigned char buffer[], const char send_msg[]);

void process_skavach_msgs(const char* data, const uint32_t len);

void update_gps_time_to_other_units();

void send_data_to_addressed_loco(const char* data, const uint32_t len);

void check_key_set_expiry();

#endif /* SOURCE_MESSAGE_PROCESSOR_H_ */
