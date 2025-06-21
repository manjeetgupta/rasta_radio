/*
 * print_util.h
 *
 *  Created on: Jan 13, 2025
 *      Author: root
 */

#ifndef HEADERS_PRINT_UTIL_H_
#define HEADERS_PRINT_UTIL_H_
typedef signed char     int8_t;

#ifdef BAREMETAL
	#include <kernel/dpl/DebugP.h>
	#define PRINT_LINE(fmt,...)  DebugP_log(fmt,##__VA_ARGS__)
//    #define PRINT_LINE(fmt,...)  int8_t print_log(const char *formatString, ...)
#else
	#include <stdio.h>
	#define PRINT_LINE(fmt,...)  printf(fmt,##__VA_ARGS__)
#endif


#endif /* HEADERS_PRINT_UTIL_H_ */
