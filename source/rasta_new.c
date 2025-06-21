// Created by CRL on 24.02.2018.

#include <stdlib.h>
#include <rmemory.h>
#include <stdio.h>
#include <rastaredundancy_new.h>
#include <time.h>
#include <errno.h>
#include <rasta_new.h>
//#include <event_system.h>
#include <rastautil.h>
#include "print_util.h"
#include "app_context.h"

#ifndef BAREMETAL
	#include <unistd.h>
	#include <syscall.h>
#endif

/**
 * this will generate a 4 byte timestamp of the current system time
 * @return current system time in s since 1970.
 * IT RETURNS time in milli seconds
 */

//#ifdef BAREMETAL
//	extern uint8_t time_Data[28];
//#endif

uint64_t cur_timestamp()
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

unsigned long mix(unsigned long a, unsigned long b, unsigned long c)
{
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}
/**
 * generate a 4 byte random number
 * @return 4 byte random number
 */
uint32_t long_random(void) {

#ifdef BAREMETAL
   srand(ClockP_getTicks()/1000);
#else
   srand(mix(clock(), time(NULL), getpid()));
#endif
    uint32_t r = 0;

    //int randomSrc = open("/dev/random", O_RDONLY);
    //unsigned long seed[1];
    //read(randomSrc , seed, sizeof(long) );
    //close(randomSrc);

    for (int i=0; i<32; i++) {
        r = r*2 + rand()%2;
    }

    //uint32_t rnv = HsmClientApp_start();

    return r;
    //return seed[0];
}

/**
 * Gets the initial sequence number from the config. If the sequence number was set to a negative value, a random number will
 * be used
 * @param config the config that is used to get the sequence number
 * @return the initial sequence number
 */
uint32_t get_initial_seq_num(struct RastaConfig * config){
    struct DictionaryEntry init_seq = config_get(config, RASTA_CONFIG_KEY_INITIAL_SEQ_NUM);

#ifdef BAREMETAL
    return config->values.general.initial_seq_num;//BAREMETAL
#endif

    if (init_seq.type == DICTIONARY_NUMBER){
        // random number when < 0 or the specified value else
        return (init_seq.value.number < 0) ? long_random() : init_seq.value.unumber;
    }

    // return random value if there is not a number in config
    return long_random();
}

/**
 * compares two RaSTA protocol version
 * @param local_version the local version
 * @param remote_version the remote version
 * @return  0 if local_version == remote_version
 *         -1 if local_version < remove_version
 *          1 if local_version > remote_version
 */
int compare_version(const char local_version[4], const char remote_version[4]){
    char * tmp;
    long local = strtol(local_version, &tmp, 4);
    long remote = strtol(remote_version, &tmp, 4);

    if (local == remote){
        return 0;
    } else if (local < remote) {
        return -1;
    } else {
        return 1;
    }
}

/**
 * checks if the given RaSTA protocol version is accepted
 * @param version the version of the remote
 * @return 1 if the remote version is accepted, else 0
 */
int version_accepted(struct rasta_receive_handle *h, const char version[4]){
    /*struct DictionaryEntry accepted_version = config_get(&con->configuration_parameters, RASTA_CONFIG_KEY_ACCEPTED_VERSIONS);
    if (accepted_version.type == DICTIONARY_ARRAY){
        for (int i = 0; i < accepted_version.value.array.count; ++i) {
            if (cmp_version(accepted_version.value.array.data[i].c, version) == 0){
                // match, version is in accepted version list
                return 1;
            }
        }
    }*/
    for (unsigned int i = 0; i < h->accepted_version.count; ++i) {
        if (compare_version(h->accepted_version.data[i].c, version) == 0){
            // match, version is in accepted version list
            return 1;
        }
    }
    return 1;

    // otherwise (something with config went wrong or version was not in accepted versions) return 0
    return 0;
}

/**
 * send a DiscReq to the specified host
 * @param connection the connection which should be used
 * @param reason the reason for the disconnect
 * @param details detailed information about the disconnect
 * @param host the host where the DiscReq will be sent to
 * @param port the port where the DiscReq will be sent to
 */
void send_DisconnectionRequest(redundancy_mux *mux, struct rasta_connection * connection, rasta_disconnect_reason reason, unsigned short details){
    struct RastaDisconnectionData disconnectionData;
    disconnectionData.reason = (unsigned short)reason;
    disconnectionData.details = details;

    struct RastaPacket discReq = createDisconnectionRequest(connection->remote_id, connection->my_id,
                                                            connection->sn_t, connection->cs_t,
                                                            cur_timestamp(), connection->ts_r, disconnectionData, &mux->sr_hashing_context);
    PRINT_LINE("SENDING DISC_REQ with SN = %d, CS = %d, TS = %u, CTS = %u \r\n", discReq.sequence_number,discReq.confirmed_sequence_number,discReq.timestamp,discReq.confirmed_timestamp);
    redundancy_mux_send(mux, discReq);

   // freeRastaByteArray(&discReq.data);
    freeRastaByteArray_pool(&discReq.data);    //MAY_23
    freeRastaByteArray_pool(&discReq.checksum);   //MAY_23

}

/**
 * send a HB to the specified host
 * @param connection the connection which should be used
 * @param host the host where the HB will be sent to
 * @param port the port where the HB will be sent to
 */
void send_Heartbeat(redundancy_mux *mux, struct rasta_connection * connection, char reschedule_manually)
{
    struct RastaPacket hb = createHeartbeat(connection->remote_id, connection->my_id, connection->sn_t,
                                            connection->cs_t, cur_timestamp(), connection->ts_r, &mux->sr_hashing_context);

    //Debug : 28-Feb-2025
    //To simulate wrong HB Seq Number: TOFIX
//     if((hb.sequence_number == 520))
//     {
//     }
//     else
     {
    	  redundancy_mux_send(mux, hb);
    	  print_log("SENDING HB with SN = %d, CS = %d, TS = %u, CTS = %u \r\n", hb.sequence_number,hb.confirmed_sequence_number,hb.timestamp,hb.confirmed_timestamp);

     }
     connection->sn_t = connection->sn_t +1;

    if (reschedule_manually) {
        reschedule_event(&connection->send_heartbeat_event);
    }

    freeRastaByteArray_pool(&hb.data);  //MAY_23
    freeRastaByteArray_pool(&hb.checksum);  //MAY_23
}

void send_RetransmissionRequest(redundancy_mux *mux, struct rasta_connection * connection)
{
    struct RastaPacket retrreq = createRetransmissionRequest(connection->remote_id,
    														 connection->my_id,
                                                             connection->sn_t,
															 connection->cs_t,
															 cur_timestamp(),
                                                             connection->ts_r,
															 &mux->sr_hashing_context);
    PRINT_LINE("SENDING RETR REQ with SN = %d, CS = %d, TS = %u, CTS = %u \r\n", retrreq.sequence_number,retrreq.confirmed_sequence_number,retrreq.timestamp,retrreq.confirmed_timestamp);
    redundancy_mux_send(mux, retrreq);
    connection->sn_t = connection->sn_t + 1;

    freeRastaByteArray_pool(&retrreq.data);  //MAY_23
    freeRastaByteArray_pool(&retrreq.checksum);  //MAY_23
}

void send_RetransmissionResponse(redundancy_mux *mux, struct rasta_connection * connection) {
    struct RastaPacket retrresp = createRetransmissionResponse(connection->remote_id, connection->my_id,
                                                               connection->sn_t, connection->cs_t, cur_timestamp(),
                                                               connection->ts_r, &mux->sr_hashing_context);
    PRINT_LINE("SENDING RETR RESPONSE with SN = %d, CS = %d, TS = %u, CTS = %u \r\n", retrresp.sequence_number,retrresp.confirmed_sequence_number,retrresp.timestamp,retrresp.confirmed_timestamp);
    redundancy_mux_send(mux, retrresp);
    connection->sn_t = connection->sn_t + 1;

    freeRastaByteArray_pool(&retrresp.data);  //MAY_23
    freeRastaByteArray_pool(&retrresp.checksum);  //MAY_23
}

unsigned int sr_retr_data_available(struct logger_t *logger, struct rasta_connection * connection)
{
    (void)logger;
    return fifo_get_size(connection->fifo_retr);
}

unsigned int sr_rasta_send_data_available(struct logger_t *logger, struct rasta_connection * connection){
    (void)logger;
    return fifo_get_size(connection->fifo_send);
}

void updateTI(long confirmed_timestamp, struct rasta_connection * con, struct RastaConfigInfoSending cfg) {
    unsigned long t_local = cur_timestamp();

#ifdef BAREMETAL
    unsigned long t_rtd = t_local + 500 - confirmed_timestamp;//MAY_31 changed from 10 to 1000
#else
    unsigned long t_rtd = t_local + (1000 / sysconf(_SC_CLK_TCK)) - confirmed_timestamp;
#endif

    //PRINT_LINE("Local time = %lu , Round trip t_rtd = %lu, timestamp = %lu\n",t_local,t_rtd,confirmed_timestamp);
    con->t_i = (uint32_t )(cfg.t_max - t_rtd);
    PRINT_LINE("Updated T_i = %lu \n",con->t_i);
//    con->timeout_event.event_info.timed_event.interval_ns = con->t_i*1000000; //Change in timer of event_connection_expired
    con->timeout_event.event_info.timed_event.interval_ns = con->t_i; //Change in timer of event_connection_expired
    // update the timeout start time
    reschedule_event(&con->timeout_event); //MAN_31
}

void resetDiagnostic(struct rasta_connection * connection) {
    for (unsigned int i = 0; i < connection->diagnostic_intervals_length; i++) {
        connection->diagnostic_intervals[i].message_count = 0;
        connection->diagnostic_intervals[i].t_alive_message_count = 0;
    }
}

void updateDiagnostic(struct rasta_connection * connection, struct RastaPacket receivedPacket, struct RastaConfigInfoSending cfg, struct rasta_handle *h) {
    unsigned long t_local = cur_timestamp();

#ifdef BAREMETAL
    unsigned long t_rtd = t_local + 10 - receivedPacket.confirmed_timestamp;
#else
    unsigned long t_rtd = t_local + (1000 / sysconf(_SC_CLK_TCK)) - receivedPacket.confirmed_timestamp;
#endif
    unsigned long t_alive = t_local - connection->cts_r;
    for (unsigned int i = 0; i < connection->diagnostic_intervals_length; i++) {
        if (connection->diagnostic_intervals[i].interval_start >= t_rtd && connection->diagnostic_intervals[i].interval_end <= t_rtd) {
            // found the sub interval this message can be assigned to
            ++connection->diagnostic_intervals[i].message_count;

            // lies t_alive in interval range, too?
            if (connection->diagnostic_intervals[i].interval_start >= t_alive && connection->diagnostic_intervals[i].interval_end <= t_alive) {
                ++connection->diagnostic_intervals[i].t_alive_message_count;
            }
            break; // we found our interval. Other executions aren't necessary anymore
        }
    }
    ++connection->received_diagnostic_message_count;
    if (connection->received_diagnostic_message_count >= cfg.diag_window) {
        fire_on_diagnostic_notification(sr_create_notification_result(h,connection));
        resetDiagnostic(connection);
    }
}

/**
 * Converts a unsigned long into a uchar array
 * @param v the uchar array
 * @param result the assigned uchar array; length should be 4
 */
void longToBytes2(unsigned long v, unsigned char* result) {
    result[0] = (unsigned char) (v >> 24 & 0xFF);
    result[1] = (unsigned char) (v >> 16 & 0xFF);
    result[2] = (unsigned char) (v >> 8 & 0xFF);
    result[3] = (unsigned char) (v & 0xFF);
}

/**
 * Converts a uchar array to a ulong
 * @param v the uchar array
 * @return the ulong
 */
uint32_t bytesToLong2(const unsigned char v[4]) {
    uint32_t result = 0;
    result = (v[0] << 24) + (v[1] << 16) + (v[2] << 8) + v[3];
    return result;
}

void sr_add_app_messages_to_buffer(struct rasta_receive_handle *h, struct rasta_connection * con, struct RastaPacket packet){
    struct RastaMessageData received_data;
   // PRINT_LINE(" 1 BEFORE EXTRACT QUEUE :  %d \r\n",packet.data.length);
    received_data = extractMessageData(packet);

    logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA add to buffer", "received %d application messages", received_data.count);


    for (unsigned int i = 0; i < received_data.count; ++i) {
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA add to buffer", "received msg: %s", received_data.data_array[i]);

//        rastaApplicationMessage * elem = rmalloc(sizeof(rastaApplicationMessage));
        rastaApplicationMessage * elem = (rastaApplicationMessage * )rmalloc_debug_pool(sizeof(rastaApplicationMessage),21);//MAY_21
        elem->id = packet.sender_id;
        //allocateRastaByteArray(&elem->appMessage, received_data.data_array[i].length);
        allocateRastaByteArray_pool(&elem->appMessage, received_data.data_array[i].length,1);
       // PRINT_LINE(" 2 BEFORE APP QUEUE :  %d %d \r\n",received_data.data_array[i].length,elem->appMessage.length);

        rmemcpy(elem->appMessage.bytes, received_data.data_array[i].bytes, received_data.data_array[i].length);
        fifo_push(con->fifo_app_msg, elem,2);
        // fire onReceive event
        fire_on_receive(sr_create_notification_result(h->handle,con));

        //PRINT_LINE("Update TI when DATA msg ,CS = %d\n",packet.confirmed_sequence_number);
        updateTI(packet.confirmed_timestamp, con,h->config); //MAN_TI
//        updateDiagnostic(con,packet,h->config,h->handle);

    }
    freeRastaMessageData(&received_data); //MAN_19MAY
}

/**
* removes all confirmed messages from the retransmission fifo
* @param con the connection that is used
*/
void sr_remove_confirmed_messages(struct rasta_receive_handle *h,struct rasta_connection * con)
{
    // remove confirmed messages from retransmission fifo
    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA remove confirmed", "confirming messages with SN_PDU <= %lu", con->cs_r);

/*
    //Printing of Queue:DEBUG
		unsigned int array_size;
		void **queue_received = fifo_print(con->fifo_retr,&array_size);
		//struct RastaByteArray elem1;
		//memcpy(&elem1,(struct RastaByteArray*)queue_received[0],sizeof(struct RastaByteArray));

		for(int i = 0; i< array_size;i++)
		{
			struct RastaByteArray *elem1 = (struct RastaByteArray*)(queue_received[0]);
			struct RastaPacket packet = bytesToRastaPacket(*elem1, h->hashing_context);
			PRINT_LINE("Packet with SN = %d\n", packet.sequence_number);
		}
    //Printing of Queue
*/

    struct RastaByteArray * elem;
    while ((elem = fifo_pop(con->fifo_retr)) != NULL)
    {
        struct RastaPacket packet = bytesToRastaPacket(*elem, h->hashing_context);
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA remove confirmed", "removing packet with sn = %lu",packet.sequence_number);

        // message is confirmed when CS_R - SN_PDU >= 0
        // equivalent to SN_PDU <= CS_R
        if (packet.sequence_number == con->cs_r)
        {
            // this packet has the last same sequence number as the confirmed sn, i.e. the next packet in the queue's
            // SN_PDU will be bigger than CS_R (because of FIFO property of mqueue)
            // that means we removed all confirmed messages and have to leave the loop to stop removing packets
            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA remove confirmed", "last confirmed packet removed");

            rfree_pool(elem->bytes); //MAY_23
            rfree_pool(elem); //MAY_21
//            freeRastaByteArray(elem); //MAY_21
            freeRastaByteArray_pool(&packet.data);
            freeRastaByteArray_pool(&packet.checksum);
            //rfree(elem);
            break;
        }

         freeRastaByteArray_pool(elem);//MAY_24
        //rfree_pool(elem->bytes); //MAY_23
        rfree_pool(elem); //MAY_21

//        freeRastaByteArray(elem);//MAY_21
//        freeRastaByteArray(&packet.data);
        freeRastaByteArray_pool(&packet.data);
        freeRastaByteArray_pool(&packet.checksum);
       // rfree(elem);
    }
}

/* ----- processing of received packet types ----- */

/**
 * calculates cts_in_seq for the given @p packet as in 5.5.6.3
 * @param con the connection that is used
 * @param packet the packet
 * @return cts_in_seq (bool)
 */
int sr_cts_in_seq(struct rasta_connection* con, struct RastaConfigInfoSending cfg, struct RastaPacket packet)
{

    if (packet.type == RASTA_TYPE_HB || packet.type == RASTA_TYPE_DATA || packet.type == RASTA_TYPE_RETRDATA)
    {
    	//PRINT_LINE("CTS_PDU = %d, CTS_R = %d, TMAX = %d\r\n",packet.confirmed_timestamp,con->cts_r,cfg.t_max);
        // cts_in_seq := 0 <= CTS_PDU – CTS_R < t_i as per 5.5.6.3
        return ((packet.confirmed_timestamp >= con->cts_r) &&
                (packet.confirmed_timestamp - con->cts_r < cfg.t_max));
    } else
    {
        // for any other type return always true
        return 1;
    }
}

/**
 * calculates sn_in_seq for the given @p packet
 * @param con the connection that is used
 * @param packet the packet
 * @return sn_in_seq (bool)
 */
int sr_sn_in_seq(struct rasta_connection * con, struct RastaPacket packet)
{
    if (packet.type == RASTA_TYPE_CONNREQ || packet.type == RASTA_TYPE_CONNRESP ||
        packet.type == RASTA_TYPE_RETRRESP || packet.type == RASTA_TYPE_DISCREQ)
    {
        // return always true
        return 1;
    }
    else
    {
        // check sn_in_seq := sn_r == sn_pdu
        return (con->sn_r == packet.sequence_number);
    }

}

/**
 * Checks the sequence number range as in 5.5.3.2
 * @param con connection that is used
 * @param packet the packet
 * @return 1 if the sequency number of the @p packet is in range
 */
int sr_sn_range_valid(struct rasta_connection * con, struct RastaConfigInfoSending cfg,  struct RastaPacket packet)
{
    // for types ConReq, ConResp and RetrResp return true
    if (packet.type == RASTA_TYPE_CONNREQ || packet.type == RASTA_TYPE_CONNRESP || packet.type == RASTA_TYPE_RETRRESP){
        return 1;
    }

    // else
    // seq. nr. in range when 0 <= SN_PDU - SN_R <= N_SENDMAX * 10
    return ((packet.sequence_number >= con->sn_r) &&
            (packet.sequence_number - con->sn_r) <= (cfg.send_max * 10));
}

/**
 * checks the confirmed sequence number integrity as in 5.5.4
 * @param con connection that is used
 * @param packet the packet
 * @return 1 if the integrity of the confirmed sequency number is confirmed, 0 otherwise
 */
int sr_cs_valid(struct rasta_connection * con, struct RastaPacket packet){
    if (packet.type == RASTA_TYPE_CONNREQ){
        // initial CS_PDU has to be 0
        return (packet.confirmed_sequence_number == 0);
    } else if (packet.type == RASTA_TYPE_CONNRESP){
        // has to be identical to last used (sent) seq. nr.
        return (packet.confirmed_sequence_number == (con->sn_t - 1));
    }
    else
    {
        // 0 <= CS_PDU - CS_R < SN_T - CS_R
    	//PRINT_LINE("packet.confirmed_sequence_number =%d, con->cs_r =%d, con->sn_t =%d \n",packet.confirmed_sequence_number ,con->cs_r,con->sn_t);
        return ((packet.confirmed_sequence_number >= con->cs_r) &&
                (packet.confirmed_sequence_number - con->cs_r) < (con->sn_t - con->cs_r));
    }
}

/**
 * checks the packet authenticity as in 5.5.2 2)
 * @param con connection that is used
 * @param packet the packet
 * @return 1 if sender and receiver of the @p packet are authentic, 0 otherwise
 */
int sr_message_authentic(struct rasta_connection * con, struct RastaPacket packet){
    return (packet.sender_id == con->remote_id && packet.receiver_id == con->my_id);
}

int sr_check_packet(struct rasta_connection *con, struct logger_t *logger, struct RastaConfigInfoSending cfg, struct RastaPacket receivedPacket, char* location)
{
    // check received packet (5.5.2)
    if (!(receivedPacket.checksum_correct &&
          sr_message_authentic(con, receivedPacket) &&
          sr_sn_range_valid(con,cfg, receivedPacket) &&
          sr_cs_valid(con, receivedPacket) &&
          sr_sn_in_seq(con, receivedPacket) &&
          sr_cts_in_seq(con, cfg, receivedPacket))){
        // something is invalid -> connection failure
        logger_log(logger, LOG_LEVEL_INFO, location, "received packet invalid");

        logger_log(logger, LOG_LEVEL_DEBUG, location, "checksum = %d", receivedPacket.checksum_correct);
        logger_log(logger, LOG_LEVEL_DEBUG, location, "authentic = %d", sr_message_authentic(con, receivedPacket));
        logger_log(logger, LOG_LEVEL_DEBUG, location, "sn_range_valid = %d", sr_sn_range_valid(con,cfg, receivedPacket));
        logger_log(logger, LOG_LEVEL_DEBUG, location, "cs_valid = %d", sr_cs_valid(con, receivedPacket));
        logger_log(logger, LOG_LEVEL_DEBUG, location, "sn_in_seq = %d", sr_sn_in_seq(con, receivedPacket));
        logger_log(logger, LOG_LEVEL_DEBUG, location, "cts_in_seq = %d", sr_cts_in_seq(con,cfg, receivedPacket));

        return 0;
    }

    return 1;
}

void sr_reset_connection(struct rasta_connection* connection, unsigned long id, struct RastaConfigInfoGeneral info)
{
    connection->remote_id = (uint32_t )id;
    connection->current_state = RASTA_CONNECTION_CLOSED;
    connection->my_id = (uint32_t )info.rasta_id;
    connection->network_id = (uint32_t )info.rasta_network;
    connection->connected_recv_buffer_size = -1;
    connection->hb_locked = 1;
    connection->hb_stopped = 0;

    // set all error counters to 0
    struct rasta_error_counters error_counters;
    error_counters.address = 0;
    error_counters.cs = 0;
    error_counters.safety= 0;
    error_counters.sn = 0;
    error_counters.type = 0;

    connection->errors = error_counters;
}

void sr_close_connection(struct rasta_connection * connection,struct rasta_handle *handle, redundancy_mux *mux, struct RastaConfigInfoGeneral info, rasta_disconnect_reason reason, unsigned short details){
    if (connection->current_state == RASTA_CONNECTION_DOWN || connection->current_state == RASTA_CONNECTION_CLOSED){
        sr_reset_connection(connection,connection->remote_id,info);

        // fire connection state changed event
        fire_on_connection_state_change(sr_create_notification_result(handle, connection));
    } else{
        // need to send DiscReq
        sr_reset_connection(connection,connection->remote_id,info);
        send_DisconnectionRequest(mux,connection, reason, details);

        connection->current_state = RASTA_CONNECTION_CLOSED;

        // fire connection state changed event
        fire_on_connection_state_change(sr_create_notification_result(handle, connection));
    }

    // remove redundancy channel
    redundancy_mux_remove_channel(mux, connection->remote_id);
}

void sr_diagnostic_interval_init(struct rasta_connection * connection, struct RastaConfigInfoSending cfg)
{
    unsigned int diagnostic_interval_length 	  	= cfg.t_max / DIAGNOSTIC_INTERVAL_SIZE;
    if (cfg.t_max % DIAGNOSTIC_INTERVAL_SIZE > 0)
    {
        ++diagnostic_interval_length;
    }
    connection->diagnostic_intervals_length 		= diagnostic_interval_length;
    connection->received_diagnostic_message_count 	= 0;
    connection->diagnostic_intervals 				= rmalloc(diagnostic_interval_length * sizeof(struct diagnostic_interval));

    for (unsigned int i = 0; i < diagnostic_interval_length; i++)
    {
        struct diagnostic_interval sub_interval;

        sub_interval.interval_start = DIAGNOSTIC_INTERVAL_SIZE * i;
        // last interval_end could be greater than T_MAX but we don't care. Every connection will be closed when you exceed T_MAX
        sub_interval.interval_end   = sub_interval.interval_start + DIAGNOSTIC_INTERVAL_SIZE;
        sub_interval.message_count  = 0;
        sub_interval.t_alive_message_count = 0;

        connection->diagnostic_intervals[i] = sub_interval;
    }
}

void sr_init_connection(struct rasta_handle *h,struct rasta_connection* connection, unsigned long id, struct RastaConfigInfoGeneral info, struct RastaConfigInfoSending cfg,rasta_role role)
{
    sr_reset_connection(connection,id,info);
    connection->role = role;

    // initalize diagnostic interval and store it in connection
    //sr_diagnostic_interval_init(connection, cfg);

//    // create receive queue
//    connection->fifo_app_msg = fifo_init(cfg.send_max);
//
//    // init retransmission fifo
//    connection->fifo_retr = fifo_init(MAX_QUEUE_SIZE);
//
//    // create send queue
//    connection->fifo_send = fifo_init(9* cfg.max_packet);
}

// retransmit messages in queue
void sr_retransmit_data(struct rasta_receive_handle *h, struct rasta_connection * connection)
{
    // prepare Array Buffer
    struct RastaByteArray packets[MAX_QUEUE_SIZE];

    int buffer_n = 0; // how many valid elements are in the buffer
    buffer_n = fifo_get_size(connection->fifo_retr);
    PRINT_LINE("Send_retransmit data_0 size = %d\r\n",buffer_n);
    // get all packets and store them in the buffer...Empty the RETR_Queue first adn later refill them with new RASTA packets with updated SN,CTS etc
    for (int j = 0; j < buffer_n; ++j)
    {
    	PRINT_LINE("Send_retransmit data_1\r\n");
        struct RastaByteArray * element;
        element = fifo_pop(connection->fifo_retr);

//        allocateRastaByteArray(&packets[j], element->length);
        allocateRastaByteArray_pool(&packets[j], element->length,2);//MAY_21
        rmemcpy(packets[j].bytes, element->bytes, element->length);

//        freeRastaByteArray(element);
        freeRastaByteArray_pool(element); //MAY_21
  //      rfree(element);
    }

    // re-open fifo in write mode
    // now retransmit each packet in the buffer with new sequence numbers
    for (int i = 0; i < buffer_n; i++)
    {
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA retransmission", "retransmit packet %d", i);
        PRINT_LINE("Send_retransmit data_2\r\n");
        // retrieve retransmission data to
        struct RastaPacket old_p = bytesToRastaPacket(packets[i], h->hashing_context);
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA retransmission", "convert packet %d to packet structure", i);

        struct RastaMessageData app_messages = extractMessageData(old_p);
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA retransmission", "extract data from packet %d ", i);

        // create new packet for retransmission
        struct RastaPacket data = createRetransmittedDataMessage(connection->remote_id, connection->my_id, connection->sn_t,
                                                                 connection->cs_t, cur_timestamp(), connection->cts_r,
                                                                 app_messages, h->hashing_context);
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA retransmission", "created retransmission packet %d ", i);

        struct RastaByteArray new_p = rastaModuleToBytes(data, h->hashing_context);

        // add packet to retrFifo again
//        struct RastaByteArray * to_fifo = rmalloc(sizeof(struct RastaByteArray));
//        allocateRastaByteArray(to_fifo, new_p.length);

        struct RastaByteArray * to_fifo = (struct RastaByteArray *)rmalloc_debug_pool(sizeof(struct RastaByteArray),22); //May_21
        allocateRastaByteArray_pool(to_fifo, new_p.length,3);
        rmemcpy(to_fifo->bytes, new_p.bytes, new_p.length);
        fifo_push(connection->fifo_retr, to_fifo,3);
        logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA retransmission", "added packet %d to queue fifo_retr", i);

        // send packet
        redundancy_mux_send(h->mux, data);
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA retransmission", "retransmitted packet with old sn=%lu", old_p.sequence_number);

        // increase sn_t
        connection->sn_t = connection->sn_t +1;

        // set last message ts
        reschedule_event(&connection->send_heartbeat_event);

        // free allocated memory of current packet
//        freeRastaByteArray(&packets[i]); //MAY_21
        freeRastaByteArray_pool(&packets[i]);
        //freeRastaByteArray(&new_p); //MAY_19
        freeRastaByteArray_pool(&new_p);
        freeRastaByteArray_pool(&old_p.data);
        freeRastaByteArray_pool(&old_p.checksum);

        freeRastaByteArray_pool(&data.data);   //MAY_23
        freeRastaByteArray_pool(&data.checksum); //MAY_23

        freeRastaMessageData(&app_messages); //MAY_23

    }

    // close retransmission with heartbeat
    PRINT_LINE("##############################\r\n");
    send_Heartbeat(h->mux,connection, 1);
}


struct rasta_connection * handle_conreq(struct rasta_receive_handle *h, int connection_id, struct RastaPacket receivedPacket)
{
    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Received ConnectionRequest from %d", receivedPacket.sender_id);
    struct rasta_connection * connection = rastalist_getConnection(h->connections, connection_id);
    if (connection == 0 || connection->current_state == RASTA_CONNECTION_CLOSED || connection->current_state == RASTA_CONNECTION_DOWN)
    {
        //new client
        if (connection == 0)
        {
            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Prepare new client");
        }
        else
        {
            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Reset existing client");
        }
        struct rasta_connection new_con;

        sr_init_connection(&h,&new_con,receivedPacket.sender_id,h->info,h->config, RASTA_ROLE_SERVER);


        // initialize seq num
        new_con.sn_t = get_initial_seq_num(&h->handle->config);
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Using %lu as initial sequence number", new_con.sn_t);

        new_con.current_state = RASTA_CONNECTION_DOWN;

        // check received packet (5.5.2)
        if (!sr_check_packet(&new_con,h->logger,h->config,receivedPacket, "RaSTA HANDLE: ConnectionRequest"))
        {
            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Packet is not valid");
            sr_close_connection(&new_con,h->handle,h->mux,h->info,RASTA_DISC_REASON_PROTOCOLERROR,0);
            return connection;
        }


        // received packet is a ConReq -> check version
        struct RastaConnectionData connectionData = extractRastaConnectionData(receivedPacket);

        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Client has version %s", connectionData.version);

        if (compare_version(RASTA_VERSION, connectionData.version) == 0 || compare_version(RASTA_VERSION, connectionData.version) == -1
        																|| version_accepted(h, connectionData.version))
        {
            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Version accepted");

            // If server has same version, or lower version -> client has to decide -> send ConResp

            // set values according to 5.6.2 [3]
            new_con.sn_r = receivedPacket.sequence_number + 1;
            new_con.cs_t = receivedPacket.sequence_number;
            new_con.ts_r = receivedPacket.timestamp;

            // save N_SENDMAX of partner
            new_con.connected_recv_buffer_size = connectionData.send_max;

            new_con.cs_r = new_con.sn_t - 1;
            new_con.cts_r = cur_timestamp();
            //new_con.cts_r = receivedPacket.confirmed_timestamp; //Original code MANJEET

            new_con.t_i = h->config.t_max;

            unsigned char *version = (unsigned char *) RASTA_VERSION;

            // send ConResp
            struct RastaPacket conresp = createConnectionResponse(new_con.remote_id, new_con.my_id,
                                                                  new_con.sn_t, new_con.cs_t,
                                                                  cur_timestamp(), new_con.cts_r,
                                                                  h->config.send_max,
                                                                  version, h->hashing_context);

            PRINT_LINE("SENDING ConRESPONSE with SN = %lu, CS = %lu, TS = %lu, CTS = %lu \r\n", conresp.sequence_number,conresp.confirmed_sequence_number,conresp.timestamp,conresp.confirmed_timestamp);

            new_con.sn_t = new_con.sn_t + 1;
            new_con.current_state = RASTA_CONNECTION_START;

            //check if the connection was just closed
            if (connection)
            {
                logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Update Client %d", receivedPacket.sender_id);
                *connection = new_con;
                fire_on_connection_state_change(sr_create_notification_result(h->handle, connection));
            }
            else
            {
                print_log("Clients BEFORE : h->connections->size = %d\r\n",h->connections->size);
                for (unsigned int i = 0; i < h->connections->size; i++)
                {
//                    remove_timed_event(&h->handle->events,&h->connections->data[i].send_heartbeat_event);  //MAN_S2
//                    remove_timed_event(&h->handle->events,&h->connections->data[i].timeout_event);
                }
                unsigned int id = (unsigned int)rastalist_addConnection(h->connections, new_con);

                print_log("Clients AFTER : h->connections->size = %d\r\n",h->connections->size);
//                  for (unsigned int i = 0; i < h->connections->size; i++)
//                  {
//                  	  add_timed_event(&h->handle->events, &h->connections->data[i].send_heartbeat_event);
//                      add_timed_event(&h->handle->events, &h->connections->data[i].timeout_event);
//                  }


                struct rasta_connection * new_con_list_ptr = rastalist_getConnection(h->connections, id);
                fire_on_connection_state_change(sr_create_notification_result(h->handle, new_con_list_ptr));
                logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: ConnectionRequest", "Add new client %d", receivedPacket.sender_id);
            }

            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Send Connection Response - waiting for Heartbeat");
            //PRINT_LINE("ip=%s",h->mux->connected_channels[0].connected_channels[0].ip_address);
            redundancy_mux_send(h->mux, conresp);

           // freeRastaByteArray(&conresp.data);
            freeRastaByteArray_pool(&conresp.data);  //MAY_23
            freeRastaByteArray_pool(&conresp.checksum);//MAY_23
        }
        else
        {
            logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: ConnectionRequest", "Version unacceptable - sending DisconnectionRequest");
            sr_close_connection(&new_con,h->handle,h->mux,h->info,RASTA_DISC_REASON_INCOMPATIBLEVERSION,0);
            return connection;
        }
    }
    else
    {
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionRequest", "Connection is in invalid state (%d) send DisconnectionRequest",connection->current_state);
        sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_UNEXPECTEDTYPE,0);
    }
    return connection;
}

char event_connection_expired(void* carry_data);
char heartbeat_send_event(void* carry_data);

void init_conn_expired_event(timed_event* t_events, struct timed_event_data* carry_data, int connection_index, struct rasta_handle* h) {
    t_events->meta_information.callback = event_connection_expired;
    t_events->meta_information.carry_data = carry_data;
//    t_events->event_info.timed_event.interval_ns = h->heartbeat_handle->config.t_max * 1000000lu; //RASTA_T_MAX , 1 ms = 10^6 ns
    t_events->event_info.timed_event.interval_ns = h->heartbeat_handle->config.t_max * 1llu; //RASTA_T_MAX , 1 ms = 10^6 ns
    t_events->meta_information.enabled = 1;
    t_events->type = TIMED_EVENT;
    carry_data->handle = h->heartbeat_handle;
    carry_data->connection_index = connection_index;
}

void init_heartbeat_send_event(timed_event* t_events, struct timed_event_data* carry_data, int connection_index, struct rasta_handle* h) {
    t_events->meta_information.callback = heartbeat_send_event;
    t_events->meta_information.carry_data = carry_data;


    int con_id = rastalist_getConnectionId(&h->connections,99);
    struct rasta_connection * connection = rastalist_getConnection(&h->connections, con_id);
    if(connection->remote_id == 99)
    {
        t_events->event_info.timed_event.interval_ns  = 350llu;
    }
    else
        t_events->event_info.timed_event.interval_ns  = h->heartbeat_handle->config.t_h * 1llu;

//    t_events->event_info.timed_event.interval_ns  = h->heartbeat_handle->config.t_h * 1000000lu;
    t_events->meta_information.enabled = 1;
    t_events->type = TIMED_EVENT;
    carry_data->handle = h->heartbeat_handle;
    carry_data->connection_index = connection_index;
}

//Added after handshake successful
void init_and_start_connection_events(struct rasta_handle* h, struct rasta_connection* connection) {
        // start sending heartbeats
        int connection_id = rastalist_getConnectionId(&h->connections, connection->remote_id);
//        init_heartbeat_send_event(&connection->send_heartbeat_event, &connection->heartbeat_carry_data, connection_id, h);
        if(connection->remote_id == 99)
        {
            init_heartbeat_send_event(&connection->send_heartbeat_event, &connection->heartbeat_carry_data, connection_id, h);
            //connection->send_heartbeat_event->event_info.timed_event.interval_ns  = 350llu;
        }
        else
        {
            init_heartbeat_send_event(&connection->send_heartbeat_event, &connection->heartbeat_carry_data, connection_id, h);
        }
        print_log("Added HB EVENT: from init : %d \r\n",connection->timeout_event);
        add_timed_event(&h->events, &connection->send_heartbeat_event);
        // arm the timeout timer
        init_conn_expired_event(&connection->timeout_event, &connection->timeout_carry_data, connection_id, h);
        add_timed_event(&h->events, &connection->timeout_event);
}

struct rasta_connection* handle_conresp(struct rasta_receive_handle *h, int con_id, struct RastaPacket receivedPacket)
{
    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionResponse", "Received ConnectionResponse from %d", receivedPacket.sender_id);
    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionResponse", "Checking packet..");
    struct rasta_connection * connection = rastalist_getConnection(h->connections, con_id);

    if (!sr_check_packet(connection,h->logger,h->config,receivedPacket,"RaSTA HANDLE: ConnectionResponse"))
    {
        sr_close_connection(connection,h->handle,h->mux,h->info, RASTA_DISC_REASON_PROTOCOLERROR, 0);
        return connection;
    }

    if (connection->current_state == RASTA_CONNECTION_START)
    {
        if (connection->role == RASTA_ROLE_CLIENT)
        {
            //handle normal conresp
            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionResponse", "Current state is in order");

            // correct type of packet received -> version check
            struct RastaConnectionData connectionData = extractRastaConnectionData(receivedPacket);

            //logger_log(&connection->logger, LOG_LEVEL_INFO, "RaSTA open con", "server is running RaSTA version %s", connectionData.version);

            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionResponse", "Server has version %s",connectionData.version);

            if (version_accepted(h, connectionData.version))
            {
                logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionResponse", "Version accepted");

                // same version or accepted versions -> send hb to complete handshake

                // set values according to 5.6.2 [3]
                connection->sn_r = receivedPacket.sequence_number + 1;
                connection->cs_t = receivedPacket.sequence_number;
                connection->ts_r = receivedPacket.timestamp;
                connection->cs_r = receivedPacket.confirmed_sequence_number;

                //PRINT_LINE("RECEIVED CS_PDU=%lu (Type=%d)\n", receivedPacket.sequence_number, receivedPacket.type);

                // send hb
                logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionResponse", "Sending heartbeat..");
                send_Heartbeat(h->mux, connection, 1);

                // update state, ready to send data
                connection->current_state = RASTA_CONNECTION_UP;

                // fire connection state changed event
                fire_on_connection_state_change(sr_create_notification_result(h->handle, connection));
                // fire handshake complete event
                fire_on_handshake_complete(sr_create_notification_result(h->handle, connection));

                connection->hb_locked = 0;
                // save the N_SENDMAX of remote
                connection->connected_recv_buffer_size = connectionData.send_max;
                init_and_start_connection_events(h->handle, connection);

            }
            else
            {
                // version not accepted -> disconnect
                logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionResponse", "Version not acceptable - send DisonnectionRequest");
                sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_INCOMPATIBLEVERSION, 0);
                return connection;
            }
        }
        else
        {
            //Server don't receive conresp
            sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_UNEXPECTEDTYPE, 0);

            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionResponse", "Server received ConnectionResponse - send Disconnection Request");
            return connection;
        }
    }
    else if (connection->current_state == RASTA_CONNECTION_RETRREQ || connection->current_state == RASTA_CONNECTION_RETRRUN || connection->current_state == RASTA_CONNECTION_UP)
    {
        sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_UNEXPECTEDTYPE, 0);
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: ConnectionResponse", "Received ConnectionResponse in wrong state - send DisconnectionRequest");
        return connection;
    }
    return connection;
}

void handle_discreq(struct rasta_receive_handle *h, struct rasta_connection *connection, struct RastaPacket receivedPacket){
    logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: DisconnectionRequest", "received DiscReq");
    print_log("EVENT TO BE DELETED : %d \r\n",connection->remote_id);
    connection->current_state = RASTA_CONNECTION_CLOSED;
    remove_timed_event(&h->handle->events,&connection->send_heartbeat_event);
    remove_timed_event(&h->handle->events,&connection->timeout_event);
    sr_reset_connection(connection,connection->remote_id,h->info);

    // remove redundancy channel
    redundancy_mux_remove_channel(h->mux, connection->remote_id);

    //struct RastaDisconnectionData disconnectionData = extractRastaDisconnectionData(receivedPacket);

    // fire connection state changed event
    fire_on_connection_state_change(sr_create_notification_result(h->handle,connection));
    // fire disconnection request received event
    struct RastaDisconnectionData data= extractRastaDisconnectionData(receivedPacket);
    fire_on_discrequest_state_change(sr_create_notification_result(h->handle,connection),data);
}

void handle_hb(struct rasta_receive_handle *h, struct rasta_connection *connection, struct RastaPacket receivedPacket)
{
    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "Received heartbeat from %d : SeqN = %d", receivedPacket.sender_id,receivedPacket.sequence_number);

    if (connection->current_state == RASTA_CONNECTION_START)
    {
        //heartbeat is for connection setup
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "Establish connection");

        // if SN not in Seq -> disconnect and close connection
        if (!sr_sn_in_seq(connection, receivedPacket))
        {
            logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: Heartbeat", "Connection HB SN not in Seq");

            if (connection->role == RASTA_ROLE_SERVER)
            {
                // SN not in Seq
                sr_close_connection(connection, h->handle, h->mux, h->info, RASTA_DISC_REASON_SEQNERROR,0);
            }
            else
            {
                // Client
                sr_close_connection(connection, h->handle, h->mux, h->info, RASTA_DISC_REASON_UNEXPECTEDTYPE,0);
            }
        }

        // if client receives HB in START -> disconnect and close
        if (connection->role == RASTA_ROLE_CLIENT)
        {
            sr_close_connection(connection, h->handle, h->mux, h->info, RASTA_DISC_REASON_UNEXPECTEDTYPE,0);
        }

        // If control is here , then it means sn_in_seq is true, so now check cts_in_seq
        if (sr_cts_in_seq(connection, h->config, receivedPacket))
        {
            // set values according to 5.6.2 [3]
            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "Heartbeat is valid connection successful");
            connection->sn_r = receivedPacket.sequence_number +1;
            connection->cs_t = receivedPacket.sequence_number;
            connection->cs_r = receivedPacket.confirmed_sequence_number;
            connection->ts_r = receivedPacket.timestamp;


            // sequence number correct, ready to receive data
            connection->current_state = RASTA_CONNECTION_UP;

            connection->hb_locked = 0;

            // fire connection state changed event
            fire_on_connection_state_change(sr_create_notification_result(h->handle,connection));
            // fire handshake complete event
            fire_on_handshake_complete(sr_create_notification_result(h->handle, connection));

            init_and_start_connection_events(h->handle, connection);
            return;
        }
        else
        {
            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "Heartbeat is invalid");

            // sequence number check failed -> disconnect
            sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_PROTOCOLERROR,0);
            return;
        }
    }

    if (sr_sn_in_seq(connection, receivedPacket))
    {
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "SN in SEQ %d",receivedPacket.sequence_number);
        //logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "SN in SEQ %d");

        if (connection->current_state == RASTA_CONNECTION_UP || connection->current_state == RASTA_CONNECTION_RETRRUN)
        {
            // check cts_in_seq
            if (sr_cts_in_seq(connection,h->config, receivedPacket))
            {
                logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "CTS in SEQ");

                print_log("Update TI when HB msg ,SN=%d,CS = %d,TS=%d,CTS=%d\n",receivedPacket.sequence_number,receivedPacket.confirmed_sequence_number,receivedPacket.timestamp,receivedPacket.confirmed_timestamp);
                updateTI(receivedPacket.confirmed_timestamp, connection,h->config);//MAN_TI
                //updateDiagnostic(connection,receivedPacket,h->config,h->handle);

                //Delete any Confirmed msg if any , from RETR_QUEUE: TOFIX
                if (sr_retr_data_available(h->logger,connection))
                		{
							struct RastaByteArray *elem = fifo_first_element(connection->fifo_retr);
							struct RastaPacket packet = bytesToRastaPacket(*elem, h->hashing_context);
//							PRINT_LINE("First Element is Packet with SN = %d\r\n", packet.sequence_number);
							if (packet.sequence_number <= connection->cs_r)
							{
								sr_remove_confirmed_messages(h,connection);
							}
							freeRastaByteArray_pool(&packet.checksum);
							freeRastaByteArray_pool(&packet.data);
                		}

                // set values according to 5.6.2 [3]
                connection->sn_r = receivedPacket.sequence_number +1;
                connection->cs_t = receivedPacket.sequence_number;
                //connection->cs_r = connection->sn_t -1; //Manjeet : TOFIX
                connection->cs_r = receivedPacket.confirmed_sequence_number;
                connection->ts_r = receivedPacket.timestamp;


                connection->cts_r = receivedPacket.confirmed_timestamp;
                //PRINT_LINE("SN_t= %d, SN_r = %d , CS_t = %d ,CS_r = %d\n\n",connection->sn_t,connection->sn_r,connection->cs_t,connection->cs_r);

                if (connection->current_state == RASTA_CONNECTION_RETRRUN)
                {
                    connection->current_state = RASTA_CONNECTION_UP;
                    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "State changed from RetrRun to Up");
                    fire_on_connection_state_change(sr_create_notification_result(h->handle,connection));
                }
            }
            else
            {
                logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "CTS not in SEQ - send DisconnectionRequest");
                // close connection
                sr_close_connection(connection,h->handle,h->mux,h->info, RASTA_DISC_REASON_TIMEOUT, 0);
            }
        }
    }
else //SR not in sequence, so send RETR REQ //MAN_RETR_HB
    {
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "SN not in SEQ, State = %d",connection->current_state);
        if (connection->current_state == RASTA_CONNECTION_UP || connection->current_state == RASTA_CONNECTION_RETRRUN)
        {
            //ignore message, send RetrReq and goto state RetrReq
            //FIXME:send retransmission
            //send_retrreq(con);
        	PRINT_LINE("Sending RETR REQ when SN out of seq is detected in HB\r\n");
        	send_RetransmissionRequest(h->mux, connection);    //Added by manjeet on 20-11-2024

            connection->current_state = RASTA_CONNECTION_RETRREQ;
            logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HANDLE: Heartbeat", "Send retransmission req in case of HB");

            // fire connection state changed event
            fire_on_connection_state_change(sr_create_notification_result(h->handle,connection));
        }
    }
}

/**
 * processes a received Data packet
 * @param con the used connection
 * @param packet the received data packet
 */
void handle_data(struct rasta_receive_handle *h, struct rasta_connection *connection, struct RastaPacket receivedPacket)
{
    logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: Data", "received Data");

    if (sr_sn_in_seq(connection, receivedPacket))
    {
        if (connection->current_state == RASTA_CONNECTION_START)
        {
            // received data in START -> disconnect and close
            sr_close_connection(connection,h->handle,h->mux,h->info, RASTA_DISC_REASON_UNEXPECTEDTYPE ,0);
        }
        else if (connection->current_state == RASTA_CONNECTION_UP)
        {
            // sn_in_seq == true -> check cts_in_seq
            logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: Data", "SN in SEQ");

            if (sr_cts_in_seq(connection,h->config, receivedPacket))
            {
                logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: Data", "CTS in SEQ");
                // valid data packet received
                // read application messages and push into queue
                sr_add_app_messages_to_buffer(h,connection,receivedPacket);

                // set values according to 5.6.2 [3]
                connection->sn_r = receivedPacket.sequence_number +1;
                connection->cs_t = receivedPacket.sequence_number;
                connection->cs_r = receivedPacket.confirmed_sequence_number;
                connection->ts_r = receivedPacket.timestamp;
                connection->cts_r = receivedPacket.confirmed_timestamp;
                // con->cts_r = current_timestamp();

                // cs_r updated, remove confirmed messages
                sr_remove_confirmed_messages(h,connection);
            }
            else
            {
                logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: Data", "CTS not in SEQ");

                // increase cs error counter
                connection->errors.cs++;

                // send DiscReq and close connection
                sr_close_connection(connection,h->handle,h->mux,h->info, RASTA_DISC_REASON_PROTOCOLERROR ,0);
            }
        }
        else if (connection->current_state == RASTA_CONNECTION_RETRRUN)
        {
            if (sr_cts_in_seq(connection, h->config, receivedPacket))
            {
                // set values according to 5.6.2 [3]
                connection->sn_r = receivedPacket.sequence_number + 1;
                connection->cs_t = receivedPacket.sequence_number;
                connection->ts_r = receivedPacket.timestamp;
            }
            else
            {
                // retransmission failed, disconnect and close
                sr_close_connection(connection,h->handle,h->mux,h->info, RASTA_DISC_REASON_PROTOCOLERROR ,0);
            }
        }
    }
    else //SEQ is not in sequence,so RETR REQ is send.
    {

        if (connection->current_state == RASTA_CONNECTION_START)
        {
            // received data in START -> disconnect and close
            sr_close_connection(connection,h->handle,h->mux,h->info, RASTA_DISC_REASON_UNEXPECTEDTYPE ,0);
        }
        else if (connection->current_state == RASTA_CONNECTION_RETRRUN || connection->current_state == RASTA_CONNECTION_UP)
        {
            // increase SN error counter
            connection->errors.sn++;

            logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: Data", "send retransmission request");
            // send RetrReq
            send_RetransmissionRequest(h->mux, connection);

            // change state to RetrReq
            connection->current_state = RASTA_CONNECTION_RETRREQ;

            fire_on_connection_state_change(sr_create_notification_result(h->handle,connection));
        }
        else if (connection->current_state == RASTA_CONNECTION_RETRREQ)
        {
            logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA HANDLE: Data", "package is ignored - waiting for RETRResponse");
        }
    }
}

/**
 * processes a received RetrReq packet
 * @param con the used connection
 * @param packet the received RetrReq packet
 */
void handle_retrreq(struct rasta_receive_handle *h, struct rasta_connection *connection, struct RastaPacket receivedPacket)
{
    logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA receive", "received RetrReq");

    if (sr_sn_in_seq(connection,receivedPacket))
    {
        // sn_in_seq == true
        logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA receive", "RetrReq SNinSeq");

        if ((connection->current_state == RASTA_CONNECTION_RETRRUN) || (connection->current_state == RASTA_CONNECTION_START))
        {
            logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA receive", "RetrReq: got RetrReq packet in RetrRun/Start mode. closing connection.");

            // send DiscReq to client
            sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_RETRFAILED, 0);
        }

        // FIXME: update connection attr (copy from RASTA_TYPE_DATA case, DRY)
        // set values according to 5.6.2 [3]
        connection->sn_r = receivedPacket.sequence_number +1;
        connection->cs_t = receivedPacket.sequence_number;
        connection->cs_r = receivedPacket.confirmed_sequence_number;
        connection->ts_r = receivedPacket.timestamp;

        // con->cts_r = current_timestamp(); //CTS is immaterial for RETR REQ/RES, so left

        // cs_r updated, remove confirmed messages
        sr_remove_confirmed_messages(h,connection);

        // send retransmission response
        send_RetransmissionResponse(h->mux,connection);
        logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA receive", "send RetrRes");

        sr_retransmit_data(h,connection); //TODO ..what to do when HB is missed

        if (connection->current_state == RASTA_CONNECTION_UP)
        {
            // change state to up
            connection->current_state = RASTA_CONNECTION_UP;
        }
        else if(connection->current_state == RASTA_CONNECTION_RETRREQ)
        {
            // change state to RetrReq
            connection->current_state = RASTA_CONNECTION_RETRREQ;
        }

        // fire connection state changed event
        fire_on_connection_state_change(sr_create_notification_result(h->handle,connection));
    }
    else
    {
        // sn_in_seq == false
        connection->cs_r = receivedPacket.confirmed_sequence_number;

        // cs_r updated, remove confirmed messages
        sr_remove_confirmed_messages(h,connection);

        // send retransmission response
        send_RetransmissionResponse(h->mux, connection);
        logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA receive", "send RetrRes");

        sr_retransmit_data(h,connection);
        // change state to RetrReq
        connection->current_state = RASTA_CONNECTION_RETRREQ;

        // fire connection state changed event
        fire_on_connection_state_change(sr_create_notification_result(h->handle,connection));
    }
}


/**
 * processes a received RetrResp packet
 * @param con the used connection
 * @param packet the received RetrResp packet
 */
void handle_retrresp(struct rasta_receive_handle *h, struct rasta_connection *connection, struct RastaPacket receivedPacket)
{
	PRINT_LINE("Handle retrresp_1\r\n");
    if (connection->current_state == RASTA_CONNECTION_RETRREQ)
    {
    	PRINT_LINE("Handle retrresp_2\r\n");
        logger_log(h->logger, LOG_LEVEL_INFO, "RaSTA receive", "starting receive retransmitted data");
        // No check regarding sequence of CTS are performed in these PDU as per 5.5.6.3

        // change to retransmission state
        connection->current_state = RASTA_CONNECTION_RETRRUN;

        // set values according to 5.6.2 [3]
        connection->sn_r = receivedPacket.sequence_number +1;
        connection->cs_t = receivedPacket.sequence_number;
        connection->ts_r = receivedPacket.timestamp;
        connection->cs_r = receivedPacket.confirmed_sequence_number; //TODO
        //connection->cs_r = connection->sn_t -1; //MAN_DEBUG_06MAR_2025
        connection->cts_r = receivedPacket.confirmed_timestamp;  //I am Doubtful it should not be added this is not a timeout related msg
    }
    else
    {
        logger_log(h->logger, LOG_LEVEL_ERROR, "RaSTA receive", "received packet type retr_resp, but not in state retr_req : State = %d",connection->current_state);
        sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_UNEXPECTEDTYPE, 0);
    }
}

/**
 * processes a received RetrData packet
 * @param con the used connection
 * @param packet the received data packet
 */
void handle_retrdata(struct rasta_receive_handle *h, struct rasta_connection *connection, struct RastaPacket receivedPacket)
{
	PRINT_LINE("Handle data_1\r\n");
    if (sr_sn_in_seq(connection,receivedPacket))
    {
    	PRINT_LINE("Handle data_2\r\n");
        if (connection->current_state == RASTA_CONNECTION_UP)
        {
        	PRINT_LINE("Handle data_3\r\n");
            sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_UNEXPECTEDTYPE,0);
        }
        else if (connection->current_state == RASTA_CONNECTION_RETRRUN)
        {
            if (!sr_cts_in_seq(connection,h->config,receivedPacket))
            {
            	PRINT_LINE("Handle data_4\r\n");
                // cts not in seq -> close connection
                sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_PROTOCOLERROR,0);
            }
            else
            {
                //cts is in seq -> add data to receive buffer
                logger_log(h->logger, LOG_LEVEL_DEBUG, "Process RetrData", "CTS in seq, adding messages to buffer");
                sr_add_app_messages_to_buffer(h,connection,receivedPacket);

                // set values according to 5.6.2 [3]
                connection->sn_r = receivedPacket.sequence_number +1;
                connection->cs_t = receivedPacket.sequence_number;
                connection->cs_r = receivedPacket.confirmed_sequence_number;
                connection->ts_r = receivedPacket.timestamp;

                //connection->cts_r = receivedPacket.confirmed_timestamp;  //I am Doubtful that it should be handled as it is a timeout related message
            }
        }
    }
    else
    {
        // sn not in seq
        logger_log(h->logger, LOG_LEVEL_DEBUG, "Process RetrData", "SN not in Seq");
        logger_log(h->logger, LOG_LEVEL_DEBUG, "Process RetrData", "SN_PDU=%lu, SN_R=%lu", receivedPacket.sequence_number, connection->sn_r);

        if (connection->current_state == RASTA_CONNECTION_UP)
        {
            sr_close_connection(connection,h->handle,h->mux,h->info,RASTA_DISC_REASON_UNEXPECTEDTYPE,0);
        }
        else if (connection->current_state == RASTA_CONNECTION_RETRRUN)
        {
            // send RetrReq
            logger_log(h->logger, LOG_LEVEL_DEBUG, "Process RetrData", "changing to state RetrReq");
            send_RetransmissionRequest(h->mux,connection);
            connection->current_state = RASTA_CONNECTION_RETRREQ;
            fire_on_connection_state_change(sr_create_notification_result(h->handle,connection));
        }
    }
}
/*
 * threads
 */
//MAN_EVENT_RE
char on_readable_event(void * handle)
{
//    PRINT_LINE("on_readable_event fired\r\n");
//    return 0;

    struct rasta_receive_handle *h = (struct rasta_receive_handle*) handle;

    // wait for incoming packets
    struct RastaPacket receivedPacket;
    if (!redundancy_mux_try_retrieve_all(h->mux, &receivedPacket))
    {
        return 0;
    }

    logger_log(h->logger, LOG_LEVEL_DEBUG, "READABLE RaSTA RECEIVE ", "Received packet %d from %d to %d", receivedPacket.type, receivedPacket.sender_id, receivedPacket.receiver_id);

    int con_id = rastalist_getConnectionId(h->connections, receivedPacket.sender_id);
    struct rasta_connection* con;

    //new client request
    if (receivedPacket.type == RASTA_TYPE_CONNREQ)
    {
        con = handle_conreq(h, con_id,receivedPacket);

   //     freeRastaByteArray(&receivedPacket.data);
         freeRastaByteArray_pool(&receivedPacket.data);
         freeRastaByteArray_pool(&receivedPacket.checksum);//20May
        return 0;
    }

    con = rastalist_getConnection(h->connections, con_id);
    if (con == NULL)
    {
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA RECEIVE", "Received packet (%d) from unknown source %d", receivedPacket.type, receivedPacket.sender_id);
        //received packet from unknown source

//        freeRastaByteArray(&receivedPacket.data);
        freeRastaByteArray_pool(&receivedPacket.data);
        freeRastaByteArray_pool(&receivedPacket.checksum);//20May
        return 0;
    }

    //handle response
    if (receivedPacket.type == RASTA_TYPE_CONNRESP)
    {
        handle_conresp(h, con_id, receivedPacket);

        //freeRastaByteArray(&receivedPacket.data);
        freeRastaByteArray_pool(&receivedPacket.data);
                 freeRastaByteArray_pool(&receivedPacket.checksum);//20May
        return 0;
    }


    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA RECEIVE", "Checking packet ...");

    // check message checksum
    if (!receivedPacket.checksum_correct)
    {
        logger_log(h->logger, LOG_LEVEL_ERROR, "RaSTA RECEIVE", "Received packet checksum incorrect");
        // increase safety error counter
        con->errors.safety++;

        //freeRastaByteArray(&receivedPacket.data);
        freeRastaByteArray_pool(&receivedPacket.data);
                 freeRastaByteArray_pool(&receivedPacket.checksum);//20May
        return 0;
    }

    // check for plausible ids
    if (!sr_message_authentic(con, receivedPacket))
    {
        logger_log(h->logger, LOG_LEVEL_ERROR, "RaSTA RECEIVE", "Received packet invalid sender/receiver");
        // increase address error counter
        con->errors.address++;

        //freeRastaByteArray(&receivedPacket.data);
        freeRastaByteArray_pool(&receivedPacket.data);
                 freeRastaByteArray_pool(&receivedPacket.checksum);//20May
        return 0;
    }

    // check sequency number range
    if (!sr_sn_range_valid(con, h->config, receivedPacket))
    {
        logger_log(h->logger, LOG_LEVEL_ERROR, "RaSTA RECEIVE", "Received packet sn range invalid");

        // invalid -> increase error counter and discard packet
        con->errors.sn++;

        //freeRastaByteArray(&receivedPacket.data);
        freeRastaByteArray_pool(&receivedPacket.data);
                 freeRastaByteArray_pool(&receivedPacket.checksum);//20May
        return 0;
    }

    // check confirmed sequence number
    if (!sr_cs_valid(con, receivedPacket))
    {
        logger_log(h->logger, LOG_LEVEL_ERROR, "RaSTA RECEIVE", "Received packet cs invalid");

        // invalid -> increase error counter and discard packet
        con->errors.cs++;

        //freeRastaByteArray(&receivedPacket.data);
        freeRastaByteArray_pool(&receivedPacket.data);
                 freeRastaByteArray_pool(&receivedPacket.checksum);//20May
        return 0;
    }

    switch (receivedPacket.type)
    {
        case RASTA_TYPE_RETRDATA:
            handle_retrdata(h,con, receivedPacket);
            break;
        case RASTA_TYPE_DATA:
            handle_data(h,con, receivedPacket);
            break;
        case RASTA_TYPE_RETRREQ:
            handle_retrreq(h,con, receivedPacket);
            break;
        case RASTA_TYPE_RETRRESP:
            handle_retrresp(h,con, receivedPacket);
            break;
        case RASTA_TYPE_DISCREQ:
            handle_discreq(h,con, receivedPacket);
            break;
        case RASTA_TYPE_HB:
            handle_hb(h,con, receivedPacket);
            break;
        default:
            logger_log(h->logger, LOG_LEVEL_ERROR, "RaSTA RECEIVE", "Received unexpected packet type %d", receivedPacket.type);
            // increase type error counter
            con->errors.type++;
            break;
    }

//    freeRastaByteArray(&receivedPacket.data);
    freeRastaByteArray_pool(&receivedPacket.data);
             freeRastaByteArray_pool(&receivedPacket.checksum);//20May
    return 0;
}

//MAN_EVENT_CE
char event_connection_expired(void * carry_data) {

//   PRINT_LINE("event_connection_expired fired\r\n");
//     return 0;

    struct timed_event_data *data = carry_data;
    struct rasta_heartbeat_handle *h = (struct rasta_heartbeat_handle*) data->handle;
    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HEARTBEAT", "T_i timer expired - send DisconnectionRequest");

    //because its multithreaded, count can get wrong
    struct rasta_connection * connection = rastalist_getConnection(h->connections, data->connection_index);
    //so check if connection is valid

    if (connection->hb_locked) {
        return 0;
    }

    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HEARTBEAT","event_connection_expired fired \r\n");
    //connection is valid, check current state
    if (connection->current_state == RASTA_CONNECTION_UP
        || connection->current_state == RASTA_CONNECTION_RETRREQ
        || connection->current_state == RASTA_CONNECTION_RETRRUN) {

        // fire heartbeat timeout event
        fire_on_heartbeat_timeout(sr_create_notification_result(h->handle, connection));

        // T_i expired -> close connection
        sr_close_connection(connection,h->handle,h->mux,h->info, RASTA_DISC_REASON_TIMEOUT, 0);
        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HEARTBEAT", "T_i timer expired - \x1b[41mdisconnected\x1b[0m");
    }

    remove_timed_event(&h->handle->events,&connection->send_heartbeat_event);
    remove_timed_event(&h->handle->events,&connection->timeout_event);
    return 0;
}

//MAN_EVENT_HB
char heartbeat_send_event(void * carry_data) {

 //   PRINT_LINE("heartbeat_send_event fired\r\n");
//    return 0;

    struct timed_event_data * data = carry_data;
    struct rasta_heartbeat_handle *h = (struct rasta_heartbeat_handle*) data->handle;

    struct rasta_connection * connection = rastalist_getConnection(h->connections, data->connection_index);


    if (connection->hb_locked) {
        return 0;
    }


    //connection is valid, check current state
    if (connection->current_state == RASTA_CONNECTION_UP
        || connection->current_state == RASTA_CONNECTION_RETRREQ
        || connection->current_state == RASTA_CONNECTION_RETRRUN)
    {
        send_Heartbeat(h->mux, connection, 0);

        logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA HEARTBEAT", "Heartbeat sent to %d", connection->remote_id);
    }

    return 0;
}

//MAN_EVENT_SE
char data_send_event(void * carry_data)
{
//    PRINT_LINE("data_send_event fired\r\n");
//    return 0;

    struct rasta_sending_handle * h = carry_data;
    unsigned int con_count 			= rastalist_count(h->connections);

    for (unsigned int k = 0; k < con_count; k++)
    {
        struct rasta_connection * connection = rastalist_getConnection(h->connections,k);
        if (connection == NULL) { continue; }
        if (connection->current_state == RASTA_CONNECTION_DOWN || connection->current_state == RASTA_CONNECTION_CLOSED) { continue; }
        //Means it will become active in other states


        unsigned int retr_data_count = sr_retr_data_available(h->logger,connection);
        //PRINT_LINE("OUTSIDE: FIFO_RETR_Queue size = %d\n",retr_data_count);

        //TOKNOW: if retr_data_count > max_packet, no message are sent even if FIFO_SEND has message. It is used for MWA / flow control.
        //Here max value of retr_data_count can be 10 which is MAX_SIZE of retr_queue. MAX_packet is configured as 3.

		//if (retr_data_count <= h->config.max_packet)//MAN_LAB
        {
            unsigned int msg_queue = sr_rasta_send_data_available(h->logger,connection);
            //PRINT_LINE("OUTSIDE: FIFO_Send_Queue size = %d\n",msg_queue);
            if (msg_queue > 0)
            {
                logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA send handler", "Messages waiting to be send: %d",msg_queue);
                //PRINT_LINE("FIFO_Send_Queue size = %d\n",msg_queue);

                connection->hb_stopped = 1;
                connection->is_sending = 1;

                struct RastaMessageData app_messages;
//                struct RastaByteArray msg;
//                allocateRastaByteArray_pool(&msg, elem->length); //MAY_23

                if (msg_queue >= h->config.max_packet)
                {
                    msg_queue = h->config.max_packet;
                }
                allocateRastaMessageData(&app_messages, msg_queue,3);

                logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA send handler","Sending %d application messages from queue",msg_queue);

                for (size_t i = 0; i < msg_queue; i++)
                {
                    struct RastaByteArray * elem;
                    elem = fifo_pop(connection->fifo_send);
                    logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA send handler","Adding application message '%s' to data packet",elem->bytes);

//                    allocateRastaByteArray(&msg, elem->length);
                    //allocateRastaByteArray_pool(&msg, elem->length);  MAY_23
                    //msg.bytes = rmemcpy(msg.bytes, elem->bytes, elem->length);
                    app_messages.data_array[i].length = elem->length;
                    rmemcpy(app_messages.data_array[i].bytes, elem->bytes, elem->length); //MAY_23


//                    freeRastaByteArray(elem);

                    //rfree(elem);
                    //app_messages.data_array[i] = msg;    //MAY_23

                   // print_log("\nFIFO POP : %p %p :",elem->bytes,elem);
                   // freeRastaByteArray_pool(elem);
                    rfree_pool(elem->bytes);
                    rfree_pool(elem); //MAY_23

                    //freeRastaByteArray_pool(&msg);
                }

                struct RastaPacket data = createDataMessage(connection->remote_id, connection->my_id, connection->sn_t,
                                                            connection->cs_t, cur_timestamp(), connection->ts_r,
                                                            app_messages, h->hashing_context);


                struct RastaByteArray packet = rastaModuleToBytes(data, h->hashing_context);

               // struct RastaByteArray * to_fifo = rmalloc(sizeof(struct RastaByteArray));
                //allocateRastaByteArray(to_fifo, packet.length);

                struct RastaByteArray * to_fifo =(struct RastaByteArray *) rmalloc_debug_pool(sizeof(struct RastaByteArray),23); //MAY_21
                allocateRastaByteArray_pool(to_fifo, packet.length,4);


                rmemcpy(to_fifo->bytes, packet.bytes, packet.length);
                fifo_push(connection->fifo_retr, to_fifo,3);
                PRINT_LINE("Size of fifo_retr : %d \r\n",fifo_get_size(connection->fifo_retr));

                //To simulate data loss: TOFIX
                //if(!(data.sequence_number == 131 ||data.sequence_number == 133 ||data.sequence_number == 135 ||data.sequence_number == 137 ||data.sequence_number == 139 ))
                {
                	  redundancy_mux_send(h->mux, data);
//                	  print_log("SENDING DATA with SN = %d, CS = %d, TS = %d, CTS = %d \r\n", data.sequence_number,data.confirmed_sequence_number,data.timestamp,data.confirmed_timestamp);
                }

                logger_log(h->logger, LOG_LEVEL_DEBUG, "RaSTA send handler", "Sent data packet from queue");

                connection->sn_t = data.sequence_number + 1;

                // set last message ts
                reschedule_event(&connection->send_heartbeat_event);

                connection->hb_stopped = 0;


                freeRastaMessageData(&app_messages);
                //freeRastaByteArray(&packet);
                freeRastaByteArray_pool(&packet); //MAY_19
                freeRastaByteArray_pool(&data.data);     //MAY_23
                freeRastaByteArray_pool(&data.checksum); //MAY_23

                connection->is_sending = 0;
            }
        }
#ifdef BAREMETAL
        ClockP_usleep(50);
#else
        usleep(50);
#endif
    }
    return 0;
}

//Not used
/*
void sr_init_handle_manually(struct rasta_handle *handle, struct RastaConfigInfo configuration, struct DictionaryArray accepted_version, struct logger_t logger) {
    rasta_handle_manually_init(handle,configuration,accepted_version, logger);

    // init the redundancy layer
    handle->mux = redundancy_mux_init_(handle->redlogger,handle->config.values);
    //redundancy_mux_set_config_id(&handle->mux,handle->own_id);
    // register redundancy layer diagnose notification handler
    handle->mux.notifications.on_diagnostics_available = handle->notifications.on_redundancy_diagnostic_notification;

    // setup MD4
    setMD4checksum(handle->config.values.sending.md4_type,
                   handle->config.values.sending.md4_a,
                   handle->config.values.sending.md4_b,
                   handle->config.values.sending.md4_c,
                   handle->config.values.sending.md4_d);
}
*/

void sr_init_handle(struct rasta_handle* handle, const char* config_file_path,struct app_context *ctx)
{
    rasta_handle_init(handle, config_file_path,ctx);

#ifdef BAREMETAL
    handle->mux = redundancy_mux_init_baremetal(handle->redlogger,handle->config.values,handle,ctx->mux_channels,ctx->mux_udp_sockets,ctx->mux_listen_ports);
#else
    handle->mux = redundancy_mux_init_(handle->redlogger,handle->config.values);
#endif
    init_event_container(&handle->events);
}

void sr_connect(struct rasta_handle* h, unsigned long id, struct RastaIPData* channels,struct app_context *ctx)
{
    struct rasta_connection new_con;
    struct rasta_error_counters error_counters;
    for (unsigned int i = 0; i < h->connections.size; i++)
    {
        if (h->connections.data[i].remote_id == id) return; //It means that request has already been sent to this ID.
    }

    redundancy_mux_add_channel(&h->mux,id,channels,ctx);

    //struct rasta_connection new_con;
    struct RastaConfigInfoGeneral general1 = ctx->r_dle.config.values.general;
    struct RastaConfigInfoSending sending1 = ctx->r_dle.config.values.sending;
    //struct logger_t logger1 = ctx->r_dle.logger;

//    sr_init_connection(&h,&new_con,id,h->config.values.general,h->config.values.sending,&h->logger, RASTA_ROLE_CLIENT);//First corruption here MAN_21JUNE
   // sr_init_connection(&h,&new_con,id,general1,sending1, RASTA_ROLE_CLIENT);//First corruption here MAN_21JUNE


//    ----------
    //    sr_reset_connection(&new_con,id,general1);
      new_con.remote_id = (uint32_t )id;
      new_con.current_state = RASTA_CONNECTION_CLOSED;
      new_con.my_id = (uint32_t )general1.rasta_id;
      new_con.network_id = (uint32_t )general1.rasta_network;
      new_con.connected_recv_buffer_size = -1;
      new_con.hb_locked = 1;
      new_con.hb_stopped = 0;

      // set all error counters to 0
//      struct rasta_error_counters error_counters;
      error_counters.address = 0;
      error_counters.cs = 0;
      error_counters.safety= 0;
      error_counters.sn = 0;
      error_counters.type = 0;

      new_con.errors = error_counters;

        new_con.role = RASTA_ROLE_CLIENT;

        // Assign static .bss FIFOs
        new_con.fifo_app_msg = &new_con.fifo_app_msg_mem;
        new_con.fifo_retr    = &new_con.fifo_retr_mem;
        new_con.fifo_send    = &new_con.fifo_send_mem;

        // Initialize
        fifo_init_1(new_con.fifo_app_msg, sending1.send_max);
        fifo_init_1(new_con.fifo_retr, MAX_QUEUE_SIZE);
        fifo_init_1(new_con.fifo_send, 9 * sending1.max_packet);

        /* initalize diagnostic interval and store it in connection
        sr_diagnostic_interval_init(&new_con, h->config.values.sending);*/

        // create receive queue
//        new_con.fifo_app_msg = fifo_init(sending1.send_max);
//
//        // init retransmission fifo
//        new_con.fifo_retr = fifo_init(MAX_QUEUE_SIZE);
//
//        // create send queue
//        new_con.fifo_send = fifo_init(9* sending1.max_packet);

 //   ---------




    //redundancy_mux_set_config_id(&h->mux, id); //ID has already been set

    // initialize seq nums and timestamps
    new_con.sn_t   = get_initial_seq_num(&h->config);
    logger_log(&h->logger, LOG_LEVEL_DEBUG, "RaSTA CONNECT", "Using %lu as initial sequence number", new_con.sn_t);

    new_con.cs_t   = 0;
    new_con.cts_r  = ClockP_getTimeUsec()/1000;
    new_con.t_i    = h->config.values.sending.t_max;

    unsigned char * version = (unsigned char*)RASTA_VERSION;
    logger_log(&h->logger, LOG_LEVEL_DEBUG, "RaSTA CONNECT", "Local version is %s", version);

    // send ConReq
    struct RastaPacket conreq = createConnectionRequest(new_con.remote_id, new_con.my_id,
                                                        new_con.sn_t, cur_timestamp(),
                                                        h->config.values.sending.send_max,
                                                        version, &h->hashing_context);
    print_log("SENDING ConREQUEST with SN = %lu, CS = %lu, TS = %lu, CTS = %lu \r\n", conreq.sequence_number,conreq.confirmed_sequence_number,conreq.timestamp,conreq.confirmed_timestamp);
    redundancy_mux_send(&h->mux,conreq);

    // increase sequence number
    new_con.sn_t++;

    // update state
    new_con.current_state = RASTA_CONNECTION_START;



    //Here size is 0 , so doesn't enter the loop
    for (unsigned int i = 0; i < h->connections.size; i++)
    {
        remove_timed_event(&h->events,&h->connections.data[i].send_heartbeat_event);
        remove_timed_event(&h->events,&h->connections.data[i].timeout_event);
    }

    unsigned int con_id = (unsigned int)rastalist_addConnection(&h->connections,new_con); //MAN_S2

    struct rasta_connection *con = rastalist_getConnection(&h->connections,con_id);
    //init_and_start_connection_events(h, con);

    //TOKNOW: Adding these events without first initializing them is a bit confusing.
//    for (unsigned int i = 0; i < h->connections.size; i++)
//    {
//        print_log("Added HB EVENT: %d \r\n",i);
//        add_timed_event(&h->events, &h->connections.data[i].send_heartbeat_event);
//        add_timed_event(&h->events, &h->connections.data[i].timeout_event);
//    }

 //   freeRastaByteArray(&conreq.data);
    freeRastaByteArray_pool(&conreq.data);  //MAY_23
    freeRastaByteArray_pool(&conreq.checksum);//MAY_23

    // fire connection state changed event
    fire_on_connection_state_change(sr_create_notification_result(h,con));
}

void sr_send(struct rasta_handle *h, unsigned long remote_id, struct RastaMessageData app_messages){

   // PRINT_LINE("LIST SIZE AFTER  : Location -> %d \r\n",h->connections.size);
    struct rasta_connection *connection = rastalist_getConnectionByRemote(&h->connections,remote_id);
   // PRINT_LINE("BREAK1");

    if (connection == 0) return;
   // PRINT_LINE("BREAK2");
    //TOFIX
    if(connection->current_state == RASTA_CONNECTION_UP){
        if (app_messages.count > h->config.values.sending.max_packet){
            // to many application messages
            logger_log(&h->logger, LOG_LEVEL_ERROR, "RaSTA send", "too many application messages to send in one packet. Maximum is %d",
                       h->config.values.sending.max_packet);
            // do nothing and leave method with error code 2
            return;
        }

        for (unsigned int i = 0; i < app_messages.count; ++i) {
            struct RastaByteArray msg;
            msg = app_messages.data_array[i];

            // push into queue
//            struct RastaByteArray * to_fifo = rmalloc(sizeof(struct RastaByteArray));
//            allocateRastaByteArray(to_fifo, msg.length);
//            rmemcpy(to_fifo->bytes, msg.bytes, msg.length);
//            fifo_push(connection->fifo_send, to_fifo,4);

            struct RastaByteArray * to_fifo = (struct RastaByteArray *)rmalloc_debug_pool(sizeof(struct RastaByteArray),24); //MAY_21 //MAN_DEBUG_23
//            msg.length = 5;
//            msg.bytes = "TEST";
            allocateRastaByteArray_pool(to_fifo, msg.length,24);

           // to_fifo->length = msg.length;
            rmemcpy(to_fifo->bytes, msg.bytes, msg.length);
            print_log("\nDATA SEND MSG LENGTH (CON UP): %d %d %d",to_fifo->length,msg.length,app_messages.data_array[i].length);


            fifo_push(connection->fifo_send, to_fifo,4);
           // print_log("\nFIFO PUSH  B: %p %p :",to_fifo->bytes,to_fifo);
        }

        logger_log(&h->logger, LOG_LEVEL_INFO, "RaSTA send", "data in send queue");

    } else if (connection->current_state == RASTA_CONNECTION_CLOSED || connection->current_state == RASTA_CONNECTION_DOWN){
        // nothing to do besides changing state to closed
        connection->current_state = RASTA_CONNECTION_CLOSED;

        // fire connection state changed event
        fire_on_connection_state_change(sr_create_notification_result(h,connection));
    } else {
        logger_log(&h->logger, LOG_LEVEL_ERROR, "RaSTA send", "service not allowed");

        // disconnect and close
        send_DisconnectionRequest(&h->mux,connection, RASTA_DISC_REASON_SERVICENOTALLOWED, 0);
        connection->current_state = RASTA_CONNECTION_CLOSED;

        // fire connection state changed event
        fire_on_connection_state_change(sr_create_notification_result(h,connection));

        // leave with error code 1
        return;
    }
}

rastaApplicationMessage sr_get_received_data(struct rasta_handle *h, struct rasta_connection * connection){
    rastaApplicationMessage message;
    rastaApplicationMessage * element;

    element = fifo_pop(connection->fifo_app_msg);

    message.id = element->id;
    message.appMessage = element->appMessage;

    print_log("\nRECEIVED LENGTH %d , %d\r\n", element->appMessage.length , message.appMessage.length);

    logger_log(&h->logger, LOG_LEVEL_INFO, "RaSTA retrieve", "application message with %lX", message.appMessage.bytes);
    logger_log(&h->logger, LOG_LEVEL_DEBUG, "RaSTA retrieve", "application message with l %d", message.appMessage.length);
    //logger_log(&h->logger, LOG_LEVEL_DEBUG, "RETRIEVE DATA", "Convert bytes to packet");

//    rfree(element);
    rfree_pool(element); //MAY_21

    //struct RastaPacket packet = bytesToRastaPacket(msg);
    //return packet;
    return message;
}

void sr_disconnect(struct rasta_handle *h, unsigned long remote_id) {
    struct rasta_connection *con = rastalist_getConnectionByRemote(&h->connections, remote_id);

    if (con == 0) return;

    sr_close_connection(con,h,&h->mux,h->config.values.general,RASTA_DISC_REASON_USERREQUEST,0);
}

void sr_cleanup(struct rasta_handle *h) {
    logger_log(&h->logger, LOG_LEVEL_DEBUG, "RaSTA Cleanup", "Cleanup called");

    h->hb_running = 0;
    h->recv_running = 0;
    h->send_running = 0;

    logger_log(&h->logger, LOG_LEVEL_DEBUG, "RaSTA Cleanup", "Threads joined");

    for (unsigned int i = 0; i < h->connections.size; i++) {
        struct rasta_connection connection = h->connections.data[i];
        // No need to free static memory
        // rfree(connection.diagnostic_intervals);

        //free FIFOs - use cleanup for static FIFOs
        fifo_cleanup(connection.fifo_app_msg);
        fifo_cleanup(connection.fifo_send);
        fifo_cleanup(connection.fifo_retr);
    }

    // set notification pointers to NULL
    h->notifications.on_receive = NULL;
    h->notifications.on_connection_state_change= NULL;
    h->notifications.on_diagnostic_notification = NULL;
    h->notifications.on_disconnection_request_received = NULL;
    h->notifications.on_redundancy_diagnostic_notification = NULL;


    // close mux
    redundancy_mux_close(&h->mux);


    // free config
    config_free(&h->config);

    // free accepted version
    free_DictionaryArray(&h->receive_handle->accepted_version);



    rfree(h->receive_handle);
    rfree(h->send_handle);
    rfree(h->heartbeat_handle);

    rastalist_free(&h->connections);

    logger_log(&h->logger, LOG_LEVEL_DEBUG, "RaSTA Cleanup", "Cleanup done");

#ifdef BAREMETAL
    ClockP_sleep(1);
#else
    sleep(1);
#endif
    logger_destroy(&h->logger);
}

//#define IO_INTERVAL 1000000 //old 10000
#define IO_INTERVAL 1 //old 10000
void init_io_events(timed_event t_events[2], struct rasta_handle* h) {
    t_events[0].meta_information.callback = data_send_event;
    t_events[0].event_info.timed_event.interval_ns = IO_INTERVAL * 10llu; //5 milliseconds for server
//    t_events[0].event_info.timed_event.interval_ns = IO_INTERVAL * 10lu; //10 milliseconds for client
    t_events[0].meta_information.enabled = 1;
    t_events[0].meta_information.carry_data = h->send_handle;
    t_events[0].type = TIMED_EVENT;
    //add_timed_event(&h->events, &(t_events[0]));

    t_events[1].meta_information.callback = on_readable_event;
    t_events[1].event_info.timed_event.interval_ns = IO_INTERVAL * 10llu; //5 milliseconds for server
//    t_events[1].event_info.timed_event.interval_ns = IO_INTERVAL * 10lu; //10 milliseconds for client
    t_events[1].meta_information.enabled = 1;
    t_events[1].meta_information.carry_data = h->receive_handle;
    t_events[1].type = TIMED_EVENT;
    //add_timed_event(&h->events, &(t_events[1]));
}

void sr_begin(struct rasta_handle* h, fd_event* external_fd_events, int len,timed_event * extern_timed_events,int len_timed_event,timed_event * e2)
{
    logger_log(&h->logger, LOG_LEVEL_DEBUG, "RaSTA HEARTBEAT", "Thread started");
    static timed_event io_events[2];
    init_io_events(io_events, h);
    add_timed_event(&h->events, io_events);
    add_timed_event(&h->events, io_events + 1);

    struct timeout_event_data timeout_data;
    timed_event channel_timeout_event;
    init_timeout_events(&channel_timeout_event, &timeout_data, &h->mux);
    add_timed_event(&h->events, &channel_timeout_event);  //0.1 millisecond

    int channel_event_data_len = h->mux.port_count;
    fd_event channel_events[channel_event_data_len];
    //struct receive_event_data channel_event_data[channel_event_data_len];
    struct receive_event_data* channel_event_data =rmalloc(2* sizeof(struct receive_event_data));
    for (int i = 0; i < channel_event_data_len; i++) {
        channel_events[i].meta_information.enabled = 1;
        channel_events[i].meta_information.callback = channel_receive_event;
        channel_events[i].meta_information.carry_data = channel_event_data + i;

        channel_events[i].type = FD_EVENT;
        channel_event_data[i].channel_index = i;
        channel_event_data[i].event = channel_events + i;
        channel_event_data[i].h = h;

#ifdef BAREMETAL
        channel_events[i].event_info.fd_event.fd = h->mux.udp_socket_fds[i]->local_port; //Put port number here instead of FD
        channel_event_data[i].len=0;
        channel_event_data[i].sender_port=0;
        channel_event_data[i].ready_flag=0;
        channel_event_data[i].sender_ip = rmalloc(sizeof(char)*15);

        memset(channel_event_data[i].sender_ip,0,sizeof(channel_event_data[i].sender_ip));
        memset(channel_event_data[i].receive_buffer,0,sizeof(channel_event_data[i].receive_buffer));
#else
        channel_events[i].event_info.fd_event.fd = h->mux.udp_socket_fds[i];
#endif
    }

    for (int i = 0; i < len; i++) {
        add_fd_event(&h->events, &(external_fd_events[i]));
    }

    for (int i = 0; i < len_timed_event; i++) {
         add_timed_event(&h->events, &(extern_timed_events[i]));
     }

    for (int i = 0; i < channel_event_data_len; i++) {
        add_fd_event(&h->events, &(channel_events[i]));
    }

   //add_timed_event(&h->events, e2);
   //start_event_loop_old(&h->events,h);
}
