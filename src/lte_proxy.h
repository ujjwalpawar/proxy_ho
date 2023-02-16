#/*
# * Licensed to the EPYSYS SCIENCE (EpiSci) under one or more
# * contributor license agreements.
# * The EPYSYS SCIENCE (EpiSci) licenses this file to You under
# * the Episys Science (EpiSci) Public License (Version 1.1) (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy/blob/master/LICENSE
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about EPYSYS SCIENCE (EpiSci):
# *      bo.ryu@episci.com
# */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string>
#include <cstring>
#include <mutex>
#include <assert.h>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include "proxy.h"

#define MAX_UE 10

struct mobility_info{
    int * source;
    int * target;
};

/**
 * A structure to hold the paramaters that the UE sends to the proxy
 * when it has completed a handover. Specifically:
 *  (a) the ID of the eNB that the UE was previously connected to
 *      (the previous source eNB)
 *  (b) the ID of the eNB that the UE is now connected to (the new
 *      source eNB)
 *  (c) the ID of the eNB that the UE will be connected to after the
 *      next handover that it performs (the new target eNB)
 *  (d) the RNTI ID that was assigned to the UE when it was connected
 *      to the previous source eNB
 *  (e) the RNTI ID that is assigned to the UE by its new source eNB
*/
typedef struct handover_update_params {
    uint16_t *new_target_enb;
    uint16_t *new_source_enb;
    uint16_t  *prev_rnti;
    uint16_t *new_rnti;
} handover_update_params_t;

/**
 * Define the offset between the eNB ID and the socket number used
 * for the same eNB. It is calculated using:
 *   int oai_rx_ue_port = ENB_SOCKET_OFFSET + ue_idx * port_delta;
 *   int oai_tx_ue_port = (ENB_SOCKET_OFFSET+1) + ue_idx * port_delta;
 * 
 * This method is used to allow for easy mapping between socket
 * numbers and eNB IDs, without the need to store and update a table.
*/
#define UE_RX_SOCKET_OFFSET 3211
#define UE_TX_SOCKET_OFFSET UE_RX_SOCKET_OFFSET+1
#define ENB_PORT_DELTA 200
#define UE_PORT_DELTA 2
#define HANDOVER_COMPLETE_MSG_LENGTH sizeof(handover_update_params_t) //TODO
#define NFAPI_RNTI_BROADCAST 0xFFFF

class Multi_UE_PNF
{
public:
    Multi_UE_PNF(int id, int num_of_ues, std::string enb_ip, std::string proxy_ip);
    ~Multi_UE_PNF() = default;
    void configure(std::string enb_ip, std::string proxy_ip);
    void start(softmodem_mode_t softmodem_mode);
private:
    std::string oai_ue_ipaddr;
    std::string vnf_ipaddr;
    std::string pnf_ipaddr;
    int vnf_p5port = -1;
    int vnf_p7port = -1;
    int pnf_p7port = -1;
    int id;
};

class Multi_UE_Proxy
{
public:
    Multi_UE_Proxy(int num_of_ues, std::vector<std::string> enb_ips, std::string proxy_ip, std::string ue_ip);
    ~Multi_UE_Proxy() = default;
    void configure(std::string ue_ip);
    int init_oai_socket(const char *addr, int tx_port, int rx_port, int ue_idx);
    void oai_enb_downlink_nfapi_task(int id, void *msg);
    void testcode_tx_packet_to_UE( int ue_tx_socket_);
    void pack_and_send_downlink_sfn_sf_msg(uint16_t id, uint16_t sfn_sf);
    void receive_message_from_ue(int ue_id);
    void send_ue_to_enb_msg(void *buffer, size_t buflen);
    void send_received_msg_to_proxy_queue(void *buffer, size_t buflen);
    void send_uplink_oai_msg_to_proxy_queue(void *buffer, size_t buflen);
    void start(softmodem_mode_t softmodem_mode);
    std::vector<Multi_UE_PNF> lte_pnfs;
    void configure_mobility_tables();
    void print_mobility_table();
    void send_broadcast_downlink_message(int enb_socket, void *pMessageBuf, uint32_t messageBufLen);
    ssize_t send_msg_to_ue(int ue_idx, void * buffer, size_t buffer_length);
    void update_handover_tables(int ue_idx, handover_update_params_t handover_update_params, bool init_add);
    void update_handover_tables(int ue_socket_number, handover_update_params_t handover_update_params);
private:
    uint16_t eNB_id[100]; // To identify the destination in uplink
    std::string oai_ue_ipaddr;
    std::string vnf_ipaddr;
    std::string pnf_ipaddr;
    int vnf_p5port = -1;
    int vnf_p7port = -1;
    int pnf_p7port = -1;
    int enb1_rach_count = 0;
    int enb2_rach_count = 0;
    int enb3_rach_count = 0;
    struct sockaddr_in address_tx_;
    struct sockaddr_in address_rx_;
    int ue_tx_socket_ = -1;
    int ue_rx_socket_ = -1;
    int ue_rx_socket[100];
    int ue_tx_socket[100];

    typedef struct sfn_sf_info_s
    {
        uint16_t phy_id;
        uint16_t sfn_sf;
    } sfn_sf_info_t;

    std::recursive_mutex mutex;
    using lock_guard_t = std::lock_guard<std::recursive_mutex>;
    std::vector<std::thread> threads;
    bool stop_thread = false;

    std::unordered_map<int, std::unordered_set<int> *> mobility_info_map;
};
