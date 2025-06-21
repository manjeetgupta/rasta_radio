/*
 * Project_cc.h
 *  Created on: 15-May-2025
 *      Author: sehrakk
 */

#ifndef PROJECT_CC_H_
#define PROJECT_CC_H_

#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "common.h"
#include "binary_conv.h"
#include "my_gpio.h"
#include "radio_manager.h"
#include "my_uart.h"
#include "message_processor.h"
#include "gps_rtc_comm.h"
#include "/ETHERNET/enet_user.h"
#include "GPIO_INPUT/GPIO_INPUT.h"
#include "MCAN/FIFO/mcan_api.h"
#include "CRC/crc.h"
#include "SPI_EXT_FLASH/SPI_Vars.h"

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */
int appMain(void *args);
void cc_init();
int rti_init(void);
void set_adjacent_stations();
int find_f1a5c3_pattern(const uint8_t *data, size_t data_len);
int find_f2a5c3_pattern(const uint8_t *data, size_t data_len);
void process_radio_2_com_port();

#endif /* PROJECT_CC_H_ */
