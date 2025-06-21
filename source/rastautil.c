#include <stdlib.h>
#include <time.h>
#include "rastautil.h"
#include "print_util.h" //ADDED FOR LOGS

//#ifdef BAREMETAL
//	extern uint8_t time_Data[28];
//#endif


uint32_t current_ts()
{
    long ms;

#ifdef BAREMETAL
		ms=ClockP_getTimeUsec()/1000; //WAY3

//		ms=monotonic_time; //WAY 1

//		int hhD,mmD,ssD;    //WAY 2
//	    char timeS[7];
//	    eeprom_read(5,0);
//	    sprintf(timeS,"%x%x%x",time_Data[4],time_Data[3],time_Data[2]);//HHMMSS
//	    sscanf(timeS,"%2d%2d%2d",&hhD,&mmD,&ssD);
//	    ms = (ssD + mmD*60 +hhD*3600)*1000;
#else
    time_t s;
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);
    s = spec.tv_sec;
    ms = s * 1000; 						// seconds to milliseconds
    ms += (long)(spec.tv_nsec / 1.0e6); // nanoseconds to milliseconds
#endif

    return (uint32_t)ms;
}

void freeRastaByteArray(struct RastaByteArray* data) {
    data->length = 0;
    free(data->bytes);
}

void freeRastaByteArray_pool(struct RastaByteArray* data) {
    data->length = 0;
    rfree_pool(data->bytes);
}

//MAN_DEBUG_23
void allocateRastaByteArray(struct RastaByteArray* data, unsigned int length) {
    //data->bytes = malloc(length); //For Advance memory extension on ubuntu
    //data->bytes = (unsigned char*)rmalloc((size_t)length);
    data->bytes = (unsigned char*)rmalloc_debug((size_t)length,3);
    data->length = length;
}

void allocateRastaByteArray_pool(struct RastaByteArray* data, unsigned int length,unsigned int loc) {
    //data->bytes = malloc(length); //For Advance memory extension on ubuntu
    //data->bytes = (unsigned char*)rmalloc((size_t)length);

 //   data->bytes = (unsigned char*)rmalloc_debug_pool((size_t)length,11);
    data->bytes = (unsigned char*)rmalloc_debug_pool((size_t)length,loc);  //MAY_23
    data->length = length;
}

int isBigEndian() {
    /*unsigned short t = 0x0102;
    return (t & 0xFF) == 0x02 ? 1 : 0;*/
    int i = 1;
    return ! *((char *) &i);
}

void longToBytes(uint32_t v, unsigned char* result) {
    if (!isBigEndian()) {
        result[3] = (unsigned char) (v >> 24 & 0xFF);
        result[2] = (unsigned char) (v >> 16 & 0xFF);
        result[1] = (unsigned char) (v >> 8 & 0xFF);
        result[0] = (unsigned char) (v & 0xFF);
    }
    else {
        result[0] = (unsigned char) (v >> 24 & 0xFF);
        result[1] = (unsigned char) (v >> 16 & 0xFF);
        result[2] = (unsigned char) (v >> 8 & 0xFF);
        result[3] = (unsigned char) (v & 0xFF);
    }
}

uint32_t bytesToLong(const unsigned char v[4]) {
    uint32_t result = 0;
    if (!isBigEndian()) {
        result = (v[3] << 24) + (v[2] << 16) + (v[1] << 8) + v[0];
    }
    else {
        result = (v[0] << 24) + (v[1] << 16) + (v[2] << 8) + v[3];
    }
    return result;
}
