//MAN_DEBUG_01JAN has to be commented on baremetal without RTC

#include "rastaredundancy_new.h"
#include <stdlib.h>
#include <stdio.h>
#include <rmemory.h>
#include <errno.h>
#include <string.h>
#include "rastautil.h"
#include "print_util.h"

//struct app_context app;
//rasta_transport_channel connected_channels_mem[2];

//All parameter are initialized except rasta_redundancy_state and checksum_type
rasta_redundancy_channel rasta_red_init(struct logger_t logger, struct RastaConfigInfo config, unsigned int transport_channel_count,unsigned long id)
{
    rasta_redundancy_channel channel;

    channel.associated_id 			 = id; //ID of server where CONNREQ is being sent
    channel.logger 		  			 = logger;
    channel.is_open 	  			 = 0;
    channel.configuration_parameters = config.redundancy;

    // init sequence numbers
    channel.seq_rx 					 = 0;
    channel.seq_tx 					 = 0;

    // init defer queue
    //channel.defer_q 	 			 = deferqueue_init(config.redundancy.n_deferqueue_size);
    channel.defer_q= &channel.defer_q_mem;
    deferqueue_init_1(&channel.defer_q_mem,config.redundancy.n_deferqueue_size);
//
//    channel.fifo_recv 	  			 = fifo_init(config.redundancy.n_deferqueue_size);
    channel.fifo_recv=&channel.fifo_recv_mem;
    fifo_init_1(&channel.fifo_recv_mem,config.redundancy.n_deferqueue_size);

    // init diagnostics buffer
#ifdef BAREMETAL
//    channel.diagnostics_packet_buffer = deferqueue_init(2 * config.redundancy.n_deferqueue_size); //changed from 4 x
#else
 //   channel.diagnostics_packet_buffer = deferqueue_init(10 * config.redundancy.n_deferqueue_size);
#endif
    // init hashing context
    channel.hashing_context.hash_length = config.sending.md4_type;
    channel.hashing_context.algorithm 	= config.sending.sr_hash_algorithm;

    if (channel.hashing_context.algorithm == RASTA_ALGO_MD4)
    {
        // use MD4 IV as key
        rasta_md4_set_key(&channel.hashing_context, config.sending.md4_a, config.sending.md4_b,
                          config.sending.md4_c, config.sending.md4_d);
    }
    else
    {
        // use the sr_hash_key
        allocateRastaByteArray(&channel.hashing_context.key, sizeof(unsigned int));  //TO BE CHANGED

        // convert unsigned in to byte array
        channel.hashing_context.key.bytes[0] = (config.sending.sr_hash_key >> 24) & 0xFF;
        channel.hashing_context.key.bytes[1] = (config.sending.sr_hash_key >> 16) & 0xFF;
        channel.hashing_context.key.bytes[2] = (config.sending.sr_hash_key >> 8) & 0xFF;
        channel.hashing_context.key.bytes[3] = (config.sending.sr_hash_key) & 0xFF;
    }
    // init transport channel buffer;
    logger_log(&channel.logger, LOG_LEVEL_DEBUG, "RaSTA Red init", "space for %d connected channels", transport_channel_count);
    channel.connected_channels = rmalloc(transport_channel_count * sizeof(rasta_transport_channel)); //Will be filled later by server IP/port

//    channel.connected_channels = &channel.connected_channels_mem; //allocated using gloabal context
    channel.connected_channel_count = 0;
    channel.transport_channel_count = transport_channel_count;

    return channel;
}

/**
 * delivers a message in the defer queue to next layer i.e. adds it to the receive buffer
 * see 6.6.4.4.6 for more details
 * @param connection the connection data which is used
 */
void deliverDeferQueue(rasta_redundancy_channel * channel)
{
    logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red deliver deferq", "f_deliverDeferQueue called, size of deferq = %d",channel->defer_q->count);

    // check if message with seq_pdu == seq_rx in defer queue
    // I think it will keep on removing PDU form defer_q till the numeric sequence if seq_rx is not broken
    while (deferqueue_contains(channel->defer_q, channel->seq_rx))
    {
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red deliver deferq", "deferq contains seq_pdu=%d",channel->seq_rx);

        // forward to next layer by pushing into receive FIFO
        struct RastaByteArray innerPackerBytes;
        // convert inner data (RaSTA SR layer PDU) to byte array
        innerPackerBytes = rastaModuleToBytes(deferqueue_get(channel->defer_q, channel->seq_rx).data,&channel->hashing_context);

        // add to queue
//        struct RastaByteArray * to_fifo;
//        struct RastaByteArray * to_fifo = rmalloc(sizeof(struct RastaByteArray));
        struct RastaByteArray * to_fifo = ( struct RastaByteArray *)rmalloc_debug_pool(sizeof(struct RastaByteArray),30);
//        allocateRastaByteArray(to_fifo, innerPackerBytes.length);
        allocateRastaByteArray_pool(to_fifo, innerPackerBytes.length,18);
        rmemcpy(to_fifo->bytes, innerPackerBytes.bytes, innerPackerBytes.length);
        //PRINT_LINE("A . FIFO PUSH ADR 1 : %p , ADR 2 : %p\r\n",to_fifo,to_fifo->bytes);
        fifo_push(channel->fifo_recv, to_fifo,5);

        //freeRastaByteArray(&innerPackerBytes); //MAY_19
        freeRastaByteArray_pool(&innerPackerBytes);
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red deliver deferq", "added message to fifo buffer");

        // remove message from queue (effectively a pop operation with the get call)
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red deliver deferq", "remove message from deferq : Before seq_rx = %d, defer_q count = %d",channel->seq_rx,channel->defer_q->count);
        deferqueue_remove(channel->defer_q, channel->seq_rx); //Crash point
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red deliver deferq", "remove message from deferq : After seq_rx = %d, defer_q count = %d, Address = %p",channel->seq_rx,channel->defer_q->count,&channel->defer_q);
        //print_q(&channel->defer_q,channel->defer_q.count,2);


        // increase seq_rx
        channel->seq_rx = channel->seq_rx +1;
    }

    logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red deliver deferq", "deferq doesn't contain seq_pdu=%d",channel->seq_rx);
}

void rasta_red_f_receive(rasta_redundancy_channel * channel, struct RastaRedundancyPacket packet, int channel_id)
{
    //logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "Channel %d: ptr=%p", channel_id, channel);
    logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "Channel %d :	channel->seq_rx = %d ", channel_id,channel->seq_rx);

    if(!packet.checksum_correct)
    {
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "Channel %d: Packet checksum incorrect", channel_id);
        // checksum incorrect, exit function
        return;
    }

    // else checksum correct

    // increase amount of received packets of this channel
    channel->connected_channels[channel_id].diagnostics_data.received_packets += 1;

    // only accept pdu with seq. nr = 0 as first message
    if (channel->seq_rx == 0 && channel->seq_tx == 0 && packet.sequence_number != 0) {
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: first seq_pdu != 0", channel_id);

        return;
    }

    // check seq_pdu
    if (packet.sequence_number < channel->seq_rx)
    {
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: seq_pdu < seq_rx", channel_id);
        // message has been received by other transport channel
        // -> calculate delay by looking for the received ts in diagnostics queue

       /* unsigned long ts = deferqueue_get_ts(channel->diagnostics_packet_buffer, packet.sequence_number); //MAN_DEBUG_01JAN TODO

        if(ts != 0)
        {
            // seq_pdu was in queue, received time is ts
            unsigned long delay = current_ts() - ts;

            // if delay > T_SEQ, message is late
            if (delay > channel->configuration_parameters.t_seq)
            {
                // channel is late, increase missed counter
                channel->connected_channels[channel_id].diagnostics_data.n_missed++;
            } else
            {
                // update t_drift and t_drift2
                channel->connected_channels[channel_id].diagnostics_data.t_drift += delay;
                channel->connected_channels[channel_id].diagnostics_data.t_drift2 += (delay * delay);
            }
        }

*/        // discard message
        return;
    }
    else if (packet.sequence_number == channel->seq_rx)
    {
        channel->seq_rx++;
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: correct seq. nr. delivering to next layer",channel_id);
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: seq_pdu=%lu, seq_rx=%lu", channel_id, packet.sequence_number, channel->seq_rx -1);

        // received packet as first transport channel -> add with ts to diagnostics buffer

       // deferqueue_add(channel->diagnostics_packet_buffer, packet, current_ts()); //MAN_DEBUG_01JAN TODO

        // forward to next layer by pushing into receive FIFO

        struct RastaByteArray innerPackerBytes;
        // convert inner data (RaSTA SR layer PDU) to byte array
        innerPackerBytes = rastaModuleToBytesNoChecksum(packet.data, &channel->hashing_context);

        // add to queue
       // struct RastaByteArray * to_fifo = rmalloc(sizeof(struct RastaByteArray)); //SIZE is 8 here
        struct RastaByteArray * to_fifo = (struct RastaByteArray *)rmalloc_debug_pool(sizeof(struct RastaByteArray),31); //SIZE is 8 here
//        struct RastaByteArray * to_fifo = allocateRastaByteArray_pool(sizeof(struct RastaByteArray),2); //SIZE is 8 here
//        struct RastaByteArray * to_fifo;
//        if(to_fifo ==NULL)
//        {
//        	PRINT_LINE("malloc fail: sizeof(struct RastaByteArray) = %d ",sizeof(struct RastaByteArray));
//        	return; //MAN_DEBUG_25JAN2025
//        }
//        allocateRastaByteArray(to_fifo, innerPackerBytes.length);//TODO_20May
        allocateRastaByteArray_pool(to_fifo, innerPackerBytes.length,19);//TODO_20May
        rmemcpy(to_fifo->bytes, innerPackerBytes.bytes, innerPackerBytes.length);
       // PRINT_LINE("B. FIFO PUSH ADR 1 : %p , ADR 2 : %p\r\n",to_fifo,to_fifo->bytes);
        if(packet.data.sender_id == 99 && packet.data.type == 6220)
        {
           print_log("HEARTBEAT OF NEW CLIENT ADDED --- &&&&&& \r\n");
        }
        fifo_push(channel->fifo_recv, to_fifo,5);


//        freeRastaByteArray(&innerPackerBytes);
        freeRastaByteArray_pool(&innerPackerBytes);  //MAY_19

        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: added message to fifo_recv buffer",
                   channel_id);

        // here the on receive notification would be fired

        // increase seq_rx
        //channel->seq_rx = channel->seq_rx +1; //Added by Manjeet, when opened HB are sent periodically till T_i is expired

        // deliver message to upper layer
        deliverDeferQueue(channel);
       // freeRastaByteArray_pool(&packet.data.data);
       // freeRastaByteArray_pool(&packet.data.checksum);//20May
    }  //MAN_31_BUG
    else if (channel->seq_rx < packet.sequence_number
               && packet.sequence_number <= (channel->seq_rx + channel->configuration_parameters.n_deferqueue_size * 10))
    {
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: seq_rx < seq_pdu && seq_pdu <= (seq_rx + 10 * MAX_DEFERQUEUE_SIZE),Seq_no = %d, channel->seq_rx = %d", channel_id,packet.sequence_number,channel->seq_rx );

        // check if message is in defer queue
        if (deferqueue_contains(channel->defer_q, packet.sequence_number))
        {
            logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: packet already in deferq,Seq_no = %d",
                       channel_id,packet.sequence_number);

            // discard message
            // possibly statistic analysis
            return;
        }
        else
        {
            // check if queue is full
            if (deferqueue_isfull(channel->defer_q)){
            //if (0){
                logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: deferq full,size of defer_q = %d ", channel_id,channel->defer_q->count);

                // full -> discard message
                return;
            } else{
                logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: adding message to deferq , size of defer_q = %d, Address = %p",channel_id,channel->defer_q->count,&channel->defer_q);

                // add message to defer queue
                deferqueue_add(channel->defer_q, packet, current_ts());
            }
        }
    }
    else if (packet.sequence_number > (channel->seq_rx + channel->configuration_parameters.n_deferqueue_size * 10))
    {
        logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red receive", "channel %d: seq_pdu > seq_rx + 10 * MAX_DEFERQUEUE_SIZE"
                , channel_id);

        // discard message
        return;
    }
}

void rasta_red_f_deferTmo(rasta_redundancy_channel * channel){
    // find smallest seq_pdu in defer queue
    int smallest_index = deferqueue_smallest_seqnr(channel->defer_q);

    // set seq_rx to it
    channel->seq_rx = channel->defer_q->elements[smallest_index].packet.sequence_number;

    logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red f_deferTmo", "f_deferTmo calling f_deliverDeferQueue, channel->seq_rx = %d",channel->seq_rx);
    deliverDeferQueue(channel);
    //logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red f_deferTmo", "After calling f_deliverDeferQueue, channel->seq_rx = %d",channel->seq_rx);
}

void rasta_red_add_transport_channel(rasta_redundancy_channel * channel, char * ip, uint16_t port)
{
    rasta_transport_channel transport_channel;

    transport_channel.port = port;
   // transport_channel.ip_address = rmalloc(sizeof(char) * 16); //25JUNE
    rmemcpy(transport_channel.ip_address, ip, 16);

    channel->connected_channels[channel->connected_channel_count] = transport_channel;
    channel->connected_channel_count++;
}

void rasta_red_cleanup(rasta_redundancy_channel * channel){
    logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red cleanup", "destroying defer queues");
    // destroy the defer queue
    deferqueue_destroy(channel->defer_q);

    // destroy the diagnostics buffer
   // deferqueue_destroy(channel->diagnostics_packet_buffer);

    logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red cleanup", "freeing connected channels");
    // free the channels
    for (unsigned int i = 0; i < channel->connected_channel_count; ++i) {
        rfree(channel->connected_channels[i].ip_address);
    }
    rfree(channel->connected_channels);
    channel->transport_channel_count = 0;
    channel->connected_channel_count = 0;

    logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red cleanup", "freeing FIFO");

    // free the receive FIFO
    fifo_destroy(channel->fifo_recv);

    freeRastaByteArray(&channel->hashing_context.key);

    logger_log(&channel->logger, LOG_LEVEL_DEBUG, "RaSTA Red cleanup", "Cleanup complete");
}
