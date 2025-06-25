#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "rastahandle.h"
#include "event_system.h"
#include <rasta_new.h>
#include "config.h"
#include "rastahashing.h"
#include "dictionary.h"
#include "rasta_red_multiplexer.h"
#include "logging.h"
#include "rastalist.h"

#define RASTA_MAX_CONNECTIONS 2
#define RASTA_MAX_ACCEPTED_VERSIONS 1
#define RASTA_VERSION_STR_LEN 256
#define RASTA_MAX_PORTS 2
#define RASTA_MAX_CHANNELS 2

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
};

#endif
