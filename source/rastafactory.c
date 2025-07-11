//
// Created by CRL on 29.11.2017.
//

#include <string.h>
#include "rastafactory.h"
#include "rmemory.h"
#include "print_util.h"

rasta_error_type rastafactoryLastError = RASTA_ERRORS_NONE;


//
//void allocateRastaMessageData(struct RastaMessageData* data, unsigned int count) {
//    data->count = count;
//    //data->data_array = rmalloc(sizeof(struct RastaByteArray) * count);
//    data->data_array = rmalloc_debug_pool(sizeof(struct RastaByteArray) * count,1);
//    PRINT_LINE("RASTA MESSAGE ALLOCATION CALLED : Location -> %p\r\n",data,data->count);
//}


//void freeRastaMessageData(struct RastaMessageData* data) {
//    for (unsigned int i = 0; i < data->count; i++) {
//        //freeRastaByteArray(&data->data_array[i]);
//        rfree_pool(data->data_array[i].bytes);
//        data->data_array[i].length=0;
//        PRINT_LINE("RASTA MESSAGE FREE CALLED : Location -> %p , %d\r\n",data->data_array[i].bytes,data->count);
//    }
//    data->count = 0;
//    //rfree(data->data_array);
//    //rfree_pool(data);
//}




void allocateRastaMessageData(struct RastaMessageData* data, unsigned int count , int loc) {

    data->count = count;
    //data->data_array = rmalloc(sizeof(struct RastaByteArray) * count);
    data->data_array = rmalloc_debug_pool(sizeof(struct RastaByteArray) * count,28);
    //PRINT_LINE("RASTA OUT MESSAGE ALLOCATION CALLED : Location -> %p : %d    :%d\r\n",data->data_array,data->count,loc);
    for (unsigned int i = 0; i < data->count; i++)
    {
        data->data_array[i].bytes = rmalloc_debug_pool(sizeof(struct RastaByteArray),29);
        data->data_array[i].length =0;
      //  PRINT_LINE("RASTA MESSAGE ALLOCATION CALLED : Location -> %p\r\n",data->data_array[i].bytes,data->count);
        //allocateRastaByteArray_pool();
    }
}

void freeRastaMessageData(struct RastaMessageData* data)
{
    for (unsigned int i = 0; i < data->count; i++)
    {
        //freeRastaByteArray(&data->data_array[i]);
      //  PRINT_LINE("RASTA MSG FREE CALLED :Location -> %p , %d\r\n",data->data_array[i].bytes,data->count);
        rfree_pool(data->data_array[i].bytes);
        data->data_array[i].length=0;
//        pool_free(data->data_array[i].bytes);
    }

    data->count = 0;
    //PRINT_LINE("RASTA OUT FREE CALLED :Location -> %p , %d\r\n",data->data_array,data->count);
    rfree_pool(data->data_array);
}


rasta_error_type getRastafactoryLastError() {
    rasta_error_type temp = rastafactoryLastError;
    rastafactoryLastError = RASTA_ERRORS_NONE;
    return temp;
}

/**
 * initializes a package, usefull for less lines of code
 * @param type
 * @param receiver_id
 * @param sender_id
 * @param sequence_number
 * @param confirmed_sequence_number
 * @param timestamp
 * @param confirmed_timestamp
 * @param data_length
 * @return
 */

struct RastaPacket initializePacket(rasta_conn_type type, uint32_t receiver_id, uint32_t sender_id, uint32_t sequence_number, uint32_t confirmed_sequence_number,
                                    uint32_t timestamp, uint32_t confirmed_timestamp, uint16_t data_length, rasta_hashing_context_t * hashing_context ) {
    struct RastaPacket result;
    result.type 					 = type;
    result.receiver_id 				 = receiver_id;
    result.sender_id 				 = sender_id;
    result.sequence_number 			 = sequence_number;
    result.confirmed_sequence_number = confirmed_sequence_number;		//0
    result.timestamp 				 = timestamp;
    result.confirmed_timestamp 		 = confirmed_timestamp;				//0


    if (data_length > 0)				// data_length=14
    {
       // allocateRastaByteArray(&result.data, data_length);
        allocateRastaByteArray_pool(&result.data, data_length,7);  //MAY_23
    }
    else {
        result.data.length = 0;  //data.length = 0 in case of RETRANSMISSION REQ/RESP
        result.data.bytes = NULL;
    }

    if (hashing_context->hash_length > 0) //RASTA_SR_CHECKSUM_LEN in config file. Presently it is 0, so no safety code in packet
    {
     //   allocateRastaByteArray(&result.checksum, hashing_context->hash_length * 8);
        allocateRastaByteArray_pool(&result.checksum, hashing_context->hash_length * 8,8);   //MAY_23
    } else {
        result.checksum.length = 0;
        result.checksum.bytes = NULL;
    }

    //calculate the length of the packet
    //static length: 28
    //checksum length: {0,8,16}
    //data length= data_length
    result.length = (uint16_t)(28 + 8 * hashing_context->hash_length + data_length); //42 in current context

    return result;
}

struct RastaPacket createConnectionRequest(uint32_t receiver_id, uint32_t sender_id, uint32_t initial_sequence_number,
                                           uint32_t timestamp, uint16_t send_max,
                                           const unsigned char version[4], rasta_hashing_context_t * hashing_context)
{

    struct RastaPacket p = initializePacket(RASTA_TYPE_CONNREQ,receiver_id,sender_id,initial_sequence_number,0,timestamp,0,14, hashing_context);

    //insert protocol version 03.03. (§10 in ISO)

    p.data.bytes[0] = version[0];
    p.data.bytes[1] = version[1];
    p.data.bytes[2] = version[2];
    p.data.bytes[3] = version[3];

    //insert sendmax

    unsigned char temp[2];
    shortToBytes(send_max,temp);

    p.data.bytes[4] = temp[0]; p.data.bytes[5] = temp[1];

    //insert zeros

    for (int i = 6; i <= 13; i++) {
        p.data.bytes[i] = 0x00;
    }

    return  p;

}

struct RastaPacket createConnectionResponse(uint32_t receiver_id, uint32_t sender_id, uint32_t initial_sequence_number, uint32_t confirmed_sequence_number,
                                            uint32_t timestamp, uint32_t confirmed_timestamp, uint16_t send_max,
                                            const unsigned char version[4], rasta_hashing_context_t * hashing_context) {
    struct RastaPacket p = initializePacket(RASTA_TYPE_CONNRESP,receiver_id,sender_id,initial_sequence_number,
            confirmed_sequence_number,timestamp,confirmed_timestamp,14,hashing_context);

    //insert protocol version 03.03. (§10 in ISO)

    p.data.bytes[0] = version[0];
    p.data.bytes[1] = version[1];
    p.data.bytes[2] = version[2];
    p.data.bytes[3] = version[3];

    //insert sendmax

    unsigned char temp[2];
    shortToBytes(send_max,temp);

    p.data.bytes[4] = temp[0]; p.data.bytes[5] = temp[1];

    //insert zeros

    for (int i = 6; i <= 13; i++) {
        p.data.bytes[i] = 0x00;
    }

    return  p;

}

struct RastaConnectionData extractRastaConnectionData(struct RastaPacket p) {
    struct RastaConnectionData result;



    if (p.data.length == 14) {

        //extract version
        result.version [0] = p.data.bytes[0];
        result.version [1] = p.data.bytes[1];
        result.version [2] = p.data.bytes[2];
        result.version [3] = p.data.bytes[3];


        unsigned char temp[2];
        temp[0] = p.data.bytes[4]; temp[1] = p.data.bytes[5];

        result.send_max = bytesToShort(temp);
    }
    else {
        result.send_max = 0;
        for (int i = 0; i < 4; i++) result.version[i] = 0;
        rastafactoryLastError = RASTA_ERRORS_WRONG_PACKAGE_FORMAT;
    }

    return result;
}

struct RastaPacket createRetransmissionRequest(uint32_t receiver_id, uint32_t sender_id, uint32_t sequence_number, uint32_t confirmed_sequence_number,
                                               uint32_t timestamp, uint32_t confirmed_timestamp, rasta_hashing_context_t * hashing_context) {
    struct RastaPacket p = initializePacket(RASTA_TYPE_RETRREQ,receiver_id,sender_id,sequence_number,
            confirmed_sequence_number,timestamp,confirmed_timestamp,0, hashing_context);
    return p;
}

struct RastaPacket createRetransmissionResponse(uint32_t receiver_id, uint32_t sender_id, uint32_t sequence_number, uint32_t confirmed_sequence_number,
                                                uint32_t timestamp, uint32_t confirmed_timestamp, rasta_hashing_context_t * hashing_context) {
    struct RastaPacket p = initializePacket(RASTA_TYPE_RETRRESP,receiver_id,sender_id,sequence_number,
            confirmed_sequence_number,timestamp,confirmed_timestamp,0, hashing_context);
    return p;
}

struct RastaPacket createDisconnectionRequest(uint32_t receiver_id, uint32_t sender_id, uint32_t sequence_number, uint32_t confirmed_sequence_number,
                                              uint32_t timestamp, uint32_t confirmed_timestamp, struct RastaDisconnectionData data, rasta_hashing_context_t * hashing_context) {
    struct RastaPacket p = initializePacket(RASTA_TYPE_DISCREQ,receiver_id,sender_id,sequence_number,
            confirmed_sequence_number,timestamp,confirmed_timestamp,4, hashing_context);

    unsigned char temp[2];

    //Details
    shortToBytes(data.details,temp);
    p.data.bytes[0] = temp[0]; p.data.bytes[1] = temp[1];

    //Reason
    shortToBytes(data.reason,temp);
    p.data.bytes[2] = temp[0]; p.data.bytes[3] = temp[1];

    return p;
}

struct RastaDisconnectionData extractRastaDisconnectionData(struct RastaPacket p) {
    struct RastaDisconnectionData result;

    if (p.data.length == 4) {

        //details
        unsigned char temp[2];
        temp[0] = p.data.bytes[0]; temp[1] = p.data.bytes[1];

        result.details = bytesToShort(temp);

        //reason
        temp[0] = p.data.bytes[2]; temp[1] = p.data.bytes[3];

        result.reason = bytesToShort(temp);
    }
    else {
        result.details = 0;
        result.reason = 0;
        rastafactoryLastError = RASTA_ERRORS_WRONG_PACKAGE_FORMAT;
    }

    return result;
}

struct RastaPacket createHeartbeat(uint32_t receiver_id, uint32_t sender_id, uint32_t sequence_number, uint32_t confirmed_sequence_number,
                                   uint32_t timestamp, uint32_t confirmed_timestamp, rasta_hashing_context_t * hashing_context) {
    struct RastaPacket p = initializePacket(RASTA_TYPE_HB,receiver_id,sender_id,sequence_number,
            confirmed_sequence_number,timestamp,confirmed_timestamp,0, hashing_context);

    return p;
}

struct RastaPacket createDataMessage(uint32_t receiver_id, uint32_t sender_id, uint32_t sequence_number, uint32_t confirmed_sequence_number,
                                     uint32_t timestamp, uint32_t confirmed_timestamp, struct RastaMessageData data, rasta_hashing_context_t * hashing_context) {

    unsigned int message_length = 0;

    for (unsigned int i = 0; i < data.count; i++) {
        message_length += data.data_array[i].length;
    }

    message_length = message_length + 2*data.count;

    struct RastaPacket p = initializePacket(RASTA_TYPE_DATA,receiver_id, sender_id,sequence_number,
            confirmed_sequence_number,timestamp,confirmed_timestamp,message_length, hashing_context);
    //fill data array

    message_length = 0;

    for (unsigned int i = 0; i < data.count; i++) {
        unsigned char temp[2];

        shortToBytes((unsigned short)data.data_array[i].length, temp);

        p.data.bytes[0+message_length] = temp[0];
        p.data.bytes[1+message_length] = temp[1];

        for (unsigned int j = 0; j < data.data_array[i].length; j++) {
            p.data.bytes[2+message_length+j] = data.data_array[i].bytes[j];
        }

        message_length += data.data_array[i].length + 2;
    }



    return p;

}

struct RastaMessageData extractMessageData(struct RastaPacket p) {
    unsigned int current_length = 0;
    unsigned int counter = 0;

    while (current_length < p.data.length) {
        unsigned char temp[2];
        temp[0] = p.data.bytes[0+current_length];
        temp[1] = p.data.bytes[1+current_length];

        unsigned short length = bytesToShort(temp);

        current_length += length + 2;
        counter++;
    }
   // print_log("A EXTRACT : %d",p.data.length);
    struct RastaMessageData result;
    allocateRastaMessageData(&result,counter,4);

    current_length = 0;

    for (unsigned int i = 0; i < result.count; i++) {
        unsigned char temp[2];
        temp[0] = p.data.bytes[0+current_length];
        temp[1] = p.data.bytes[1+current_length];

        unsigned short length = bytesToShort(temp);

        //allocateRastaByteArray_pool(&result.data_array[i],length); //MAN_19MAY PACKETIZATION
        result.data_array[i].length =  length;
        for (int j = 0; j < length; j++) {
            result.data_array[i].bytes[j] = p.data.bytes[2+current_length+j];
        }

        current_length += length + 2;

    }

   // print_log("A EXTRACT : %d",result.data_array[0].length);
    return result;


}

struct RastaPacket createRetransmittedDataMessage(uint32_t receiver_id, uint32_t sender_id, uint32_t sequence_number, uint32_t confirmed_sequence_number,
                                                  uint32_t timestamp, uint32_t confirmed_timestamp, struct RastaMessageData data, rasta_hashing_context_t * hashing_context) {
    struct RastaPacket result;
    result = createDataMessage(receiver_id,sender_id,sequence_number,confirmed_sequence_number,
            timestamp,confirmed_timestamp,data, hashing_context);

    result.type = RASTA_TYPE_RETRDATA;

    return result;

}

struct RastaRedundancyPacket createRedundancyPacket(uint32_t sequence_number, struct RastaPacket inner_data, struct crc_options checksum_type){
    struct RastaRedundancyPacket packet;

    packet.sequence_number = sequence_number;
    packet.data 		   = inner_data;
    packet.checksum_type   = checksum_type;

    // reserved bytes have to be 0s in version 03.03
    packet.reserve = 0x0000;

    // length = 2 bytes length field + 2 bytes reserve + 4 bytes seq. nr. + inner data length + checksum length
    // checksum width in crc_options is in bit, so divide by 8 for bytes
    packet.length = (uint16_t)(8 + inner_data.length + (checksum_type.width / 8));

    // set checksum_correct to 1 as checksum will be calculated on conversion to bytes
    packet.checksum_correct = 1;

    return packet;
}




