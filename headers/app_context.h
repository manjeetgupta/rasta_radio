#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "rastahandle.h"
#include "event_system.h"
#include <rasta_new.h>
#include "config.h"
#include "rastahashing.h"
#include "dictionary.h"
//#include "rasta_red_multiplexer.h"  // Removed to avoid circular dependency
#include "logging.h"
#include "rastalist.h"
#include "rastaredundancy_new.h"  // For MAX_DEFER_QUEUE_MSG_SIZE

// Forward declarations
struct redundancy_mux;
struct rasta_redundancy_channel;
struct rasta_transport_channel;
struct new_connection_notification_parameter_wrapper;
struct udp_pcb;

// Include for ip_addr_t (from lwIP)
#ifdef BAREMETAL
#include "lwip/ip.h"
#endif

#define RASTA_MAX_CONNECTIONS 2
#define RASTA_MAX_ACCEPTED_VERSIONS 1
#define RASTA_VERSION_STR_LEN 256
#define RASTA_MAX_PORTS 2
#define RASTA_MAX_CHANNELS 2
#define RASTA_MAX_IP_STR_LEN 16  // For IP address strings
#define RASTA_MAX_NOTIFICATION_WRAPPERS 4  // For notification parameter wrappers

struct app_context {
    struct rasta_handle r_dle;

    struct rasta_receive_handle rcv_mem;//u
    struct rasta_sending_handle snd_mem;//u
    struct rasta_heartbeat_handle hb_mem;//u

    struct RastaConfig static_config;
    struct RastaIPData static_ipdata[RASTA_MAX_CONNECTIONS]; //U

    struct DictionaryString accepted_versions[RASTA_MAX_ACCEPTED_VERSIONS];//u
    struct DictionaryArray accepted_version_array;//u

    char logger_file_buf[RASTA_VERSION_STR_LEN];

    uint8_t hashing_key_bytes[4];
    struct RastaByteArray hashing_key;

    struct redundancy_mux mux_mem;
    uint16_t mux_listen_ports[RASTA_MAX_PORTS];//u
    struct udp_pcb *mux_udp_sockets[RASTA_MAX_PORTS]; //u
    rasta_redundancy_channel mux_channels[RASTA_MAX_CHANNELS];//u
    rasta_transport_channel transport_channels[RASTA_MAX_PORTS];

    uint8_t mux_hash_key_bytes[4];
    struct RastaByteArray mux_hash_key;

    struct rasta_connection connection_storage[RASTA_MAX_CONNECTIONS];//u

    // Static memory for redundancy multiplexer
    char ip_address_pool[RASTA_MAX_CHANNELS * RASTA_MAX_PORTS][RASTA_MAX_IP_STR_LEN];
    unsigned char ip_address_used[RASTA_MAX_CHANNELS * RASTA_MAX_PORTS];
    
    // Static memory for notification wrappers
    struct new_connection_notification_parameter_wrapper notification_wrappers[RASTA_MAX_NOTIFICATION_WRAPPERS];
    unsigned char notification_wrapper_used[RASTA_MAX_NOTIFICATION_WRAPPERS];
    
    // Static memory for network buffers
    unsigned char network_buffers[RASTA_MAX_PORTS][MAX_DEFER_QUEUE_MSG_SIZE];
    unsigned char network_buffer_used[RASTA_MAX_PORTS];
    
    // Static memory for IP address structures
#ifdef BAREMETAL
    ip_addr_t ip_addr_pool[RASTA_MAX_CHANNELS * RASTA_MAX_PORTS];
    unsigned char ip_addr_used[RASTA_MAX_CHANNELS * RASTA_MAX_PORTS];
#endif
};

#endif
